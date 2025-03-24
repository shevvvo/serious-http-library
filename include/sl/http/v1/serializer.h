#pragma once

#include <span>
#include <sstream>

#include "message.hpp"

namespace http::v1 {

class serializer {
public:
    static std::span<std::byte> serialize(const request_type& req) {
        std::stringstream ss;
        ss << req.method << " " << req.uri << " " << req.version << "\r\n";
        prepare_headers(ss, req);
        ss << "\r\n" << req.body;
        auto result = ss.str();
        return {reinterpret_cast<std::byte*>(result.data()), result.size()};
    }

    static std::span<std::byte> serialize(const response_type& res) {
        std::stringstream ss;
        ss << res.version << " " << res.status_code << " " << res.reason_phrase << "\r\n";
        prepare_headers(ss, res);
        ss << "\r\n" << res.body;
        auto result = ss.str();
        return {reinterpret_cast<std::byte*>(result.data()), result.size()};
    }

private:
    static void prepare_headers(std::stringstream& ss, const message_type& msg) {
        bool has_content_length = msg.has_header("Content-Length");
        bool chunked = msg.has_header("Transfer-Encoding") && (msg.headers.at("Transfer-Encoding") == "chunked");

        if (!msg.body.empty() && !has_content_length && !chunked) {
            ss << "Content-Length: " << msg.body.size() << "\r\n";
        }

        for (const auto& [name, value] : msg.headers) {
            if (name == "Content-Length" && chunked) continue;
            ss << name << ": " << value << "\r\n";
        }
    }
};
} // http::v1
