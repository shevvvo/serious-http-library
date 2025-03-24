#pragma once

#include "message.hpp"

#include <span>
#include <sstream>

namespace http::v1 {

struct parser {
    parser() : state_(state::ParsingStartLine), content_length_(0) {}

    enum class state {
        ParsingStartLine,
        ParsingHeaders,
        ParsingBody,
        Completed,
        Error
    };

    state state_;
    request_type request_;
    std::string buffer_;
    size_t content_length_;
};

void process_body(parser& parser_) {
    size_t to_read = std::min(parser_.buffer_.size(), parser_.content_length_);
    parser_.request_.body.append(parser_.buffer_.substr(0, to_read));
    parser_.buffer_.clear();
    parser_.content_length_ = 0;
    parser_.state_ = parser::state::Completed;
}

void prepare_body_parsing(parser& parser_) {
    auto it = parser_.request_.headers.find("Content-Length");
    if (it != parser_.request_.headers.end()) {
        parser_.content_length_ = std::stoul(it->second);
        parser_.state_ = parser::state::ParsingBody;
    } else {
        parser_.state_ = parser::state::Completed;
    }
}

void process_headers(parser& parser_) {
    size_t pos = parser_.buffer_.find("\r\n");
    while (pos != std::string::npos) {
        std::string line = parser_.buffer_.substr(0, pos);
        parser_.buffer_.erase(0, pos + 2);

        if (line.empty()) {
            prepare_body_parsing(parser_);
            return;
        }

        size_t col = line.find(':');
        if (col != std::string::npos) {
            std::string name = line.substr(0, col);
            std::string value = line.substr(col + 1);
            parser_.request_.headers[name] = value;
        }

        pos = parser_.buffer_.find("\r\n");
    }
}

void process_startline(parser& parser_) {
    size_t pos = parser_.buffer_.find("\r\n");
    if (pos == std::string::npos) return;

    std::string line = parser_.buffer_.substr(0, pos);
    parser_.buffer_.erase(0, pos + 2);

    std::istringstream iss(line);
    iss >> parser_.request_.method >> parser_.request_.uri >> parser_.request_.version;

    parser_.state_ = parser::state::ParsingHeaders;
}

void process_buffer(parser& parser_) {
    while (!parser_.buffer_.empty() && parser_.state_ != parser::state::Completed && parser_.state_ != parser::state::Error) {
        switch (parser_.state_) {
        case parser::state::ParsingStartLine: process_startline(parser_); break;
        case parser::state::ParsingHeaders: process_headers(parser_); break;
        case parser::state::ParsingBody: process_body(parser_); break;
        default: break;
        }
    }
}

request_type parse(std::span<std::byte> req) {
    parser parser_;
    parser_.buffer_ = std::string(reinterpret_cast<char*>(*req.begin()), req.size());
    process_buffer(parser_);
    return parser_.request_;
}
} // http::v1
