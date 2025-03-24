#pragma once

#include <string>
#include <map>

namespace http::v1 {

struct message_type {
    std::string version = "HTTP/1.1";
    std::map<std::string, std::string> headers;
    std::string body;

    [[nodiscard]] bool has_header(const std::string& name) const {
        return headers.find(name) != headers.end();
    }

    void set_content_length() {
        headers["Content-Length"] = std::to_string(body.size());
    }
};

struct request_type : public message_type {
    std::string method;
    std::string uri;
};

struct response_type : public message_type {
    unsigned status_code = 200;
    std::string reason_phrase = "OK";
};

} // http::v1
