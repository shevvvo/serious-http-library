#pragma once

#include "message.hpp"

#include <algorithm>
#include <range/v3/iterator/basic_iterator.hpp>
#include <span>
#include <sstream>
#include <charconv>

#include <sl/meta/monad/maybe.hpp>
#include <sl/meta/monad/result.hpp>

namespace http::v1 {

namespace detail {

struct parse_startline_type {
    std::string_view method;
    std::string_view uri;
    std::string_view version;
    std::string_view remainder;
};

struct parse_headers_type {
    std::map<std::string_view, std::string_view> headers;
    std::string_view remainder;
};

auto parse_part(std::string_view x, std::string_view term) -> sl::meta::maybe<std::pair<std::string_view, std::string_view>> {
    const std::size_t x_size = x.find(term);
    if (x_size == std::string_view::npos) {
        return sl::meta::null;
    }
    return std::make_pair(x.substr(x_size), x.substr(x_size, std::string_view::npos));
}

struct parser {
    parser() : state_(state::ParsingStartLine), content_length_(0) {}

    enum class state { ParsingStartLine, ParsingHeaders, ParsingBody, Completed, Error };

    state state_;
    request_type request_;
    std::span<const std::byte> buffer_;
    size_t content_length_;
};

auto process_headers(std::string_view buffer_str) -> sl::meta::maybe<parse_headers_type> {
    auto end_of_line_result = parse_part(buffer_str, "\r\n");
    std::map<std::string_view, std::string_view> headers;

    std::string_view remainder;

    while (end_of_line_result.has_value()) {
        const auto [full_str, full_str_remainder] = end_of_line_result.value();
        remainder = full_str_remainder;
        if (full_str.empty()) {
            parse_headers_type result {headers, remainder};
            return result;
        }

        const auto header_result = parse_part(full_str, ": ");

        if (!header_result.has_value()) {
            parse_headers_type result {headers, remainder};
            return result;
        }

        const auto [header, header_remainder] = header_result.value();

        headers[header] = header_remainder;

        end_of_line_result = parse_part(remainder, "\r\n");
    }

    if (headers.empty()) {
        return sl::meta::null;
    }

    parse_headers_type result {headers, remainder};
    return result;
}

auto parse_start_line(std::string_view buffer_str) -> sl::meta::maybe<parse_startline_type> {
    const auto method_result = parse_part(buffer_str, " ");
    if (!method_result.has_value()) { return sl::meta::null; }
    const auto [method, method_remainder] = method_result.value();

    const auto uri_result = parse_part(method_remainder, " ");
    if (!uri_result.has_value()) { return sl::meta::null; }
    const auto [uri, uri_remainder] = uri_result.value();

    const auto version_result = parse_part(uri_remainder, "\r\n");
    if (!version_result.has_value()) { return sl::meta::null; }
    const auto [version, version_remainder] = version_result.value();
    parse_startline_type result = { method, uri, version, version_remainder };
    return result;
}

} // detail

auto parse(std::span<const std::byte> buffer) -> sl::meta::result<request_type, std::string_view> {
    const std::size_t size = buffer.size();
    const char* data = std::bit_cast<const std::string_view::value_type*>(buffer.data());
    std::string_view buffer_str = {data, size};

    request_type request{};

    const auto start_line_result = detail::parse_start_line(buffer_str);
    if (!start_line_result.has_value()) { return sl::meta::err("parse_failed"); }

    const auto header_result = detail::process_headers(start_line_result->remainder);
    if (!header_result.has_value()) { return sl::meta::err("parse_failed"); }

    const auto [header_value, header_remainder] = header_result.value();
    auto cont_length_it = header_value.find("Content-Length");
    if (cont_length_it != header_value.end()) {

        size_t cont_size;
        auto result = std::from_chars(cont_length_it->second.data(), cont_length_it->second.data() + cont_length_it->second.size(), cont_size);
        if (result.ec == std::errc::invalid_argument) {
            return sl::meta::err("parse_failed");
        }
        auto cont_length = std::min(buffer_str.size(), cont_size);
        request.body = buffer_str.substr(cont_length);
    }

    request.headers = header_value;
    request.method = start_line_result->method;
    request.uri = start_line_result->uri;
    request.version = start_line_result->version;
    return request;
}

} // http::v1
