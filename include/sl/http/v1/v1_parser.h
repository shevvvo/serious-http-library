#pragma once

#include "v1_message.hpp"

namespace http::v1 {
class parser {
public:
    parser() : state_(state::ParsingStartLine), content_length_(0) {}

    request_type parse(std::span<std::byte> req) {
        buffer_ = std::string(reinterpret_cast<char*>(*req.begin()), req.size());
        process_buffer();
        return request_;
    }

private:
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

    void process_buffer() {
        while (!buffer_.empty() && state_ != state::Completed && state_ != state::Error) {
            switch (state_) {
            case state::ParsingStartLine: process_startline(); break;
            case state::ParsingHeaders: process_headers(); break;
            case state::ParsingBody: process_body(); break;
            default: break;
            }
        }
    }

    void process_startline() {
        size_t pos = buffer_.find("\r\n");
        if (pos == std::string::npos) return;

        std::string line = buffer_.substr(0, pos);
        buffer_.erase(0, pos + 2);

        std::istringstream iss(line);
        iss >> request_.method >> request_.uri >> request_.version;

        state_ = state::ParsingHeaders;
    }

    void process_headers() {
        size_t pos = buffer_.find("\r\n");
        while (pos != std::string::npos) {
            std::string line = buffer_.substr(0, pos);
            buffer_.erase(0, pos + 2);

            if (line.empty()) {
                prepare_body_parsing();
                return;
            }

            size_t col = line.find(':');
            if (col != std::string::npos) {
                std::string name = line.substr(0, col);
                std::string value = line.substr(col + 1);
                request_.headers[name] = value;
            }

            pos = buffer_.find("\r\n");
        }
    }

    void prepare_body_parsing() {
        auto it = request_.headers.find("Content-Length");
        if (it != request_.headers.end()) {
            content_length_ = std::stoul(it->second);
            state_ = state::ParsingBody;
        } else {
            state_ = state::Completed;
        }
    }

    void process_body() {
        size_t to_read = std::min(buffer_.size(), content_length_);
        request_.body.append(buffer_.substr(0, to_read));
        buffer_.clear();
        content_length_ = 0;
        state_ = state::Completed;
    }
};
} // http::v1
