#pragma once
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <jh/pod>

namespace http_util {
    namespace beast = boost::beast;
    namespace http = beast::http;

    using Request  = http::request<http::string_body>;
    using Response = http::response<http::string_body>;

    inline bool is_json(const Request& req) {
        return req[http::field::content_type].starts_with("application/json");
    }

    inline void set_json(Response& res, const boost::json::value& value, int status_code = 200) {
        res.result(http::status(status_code));
        res.set(http::field::content_type, "application/json");
        res.body() = boost::json::serialize(value);
        res.prepare_payload();
    }

    inline void set_text(Response& res, std::string_view text, int status_code = 200) {
        res.result(http::status(status_code));
        res.set(http::field::content_type, "text/plain");
        res.body() = std::string(text);
        res.prepare_payload();
    }

    template<jh::pod::array<char, 8> Mime>
    struct set_download {
        static void apply(Response& res, std::string_view content, const std::string& filename) {
            res.result(http::status::ok);
            res.set(http::field::content_type, "text/" + std::string(Mime.data));
            res.set(http::field::content_disposition, "attachment; filename=\"" + filename + "\"");
            res.body() = std::string(content);
            res.prepare_payload();
        }
    };

    inline bool check_method(const Request& req, http::verb expected, Response& res) {
        if (req.method() != expected) {
            set_json(res, {
                    {"error", "Method Not Allowed"},
                    {"expected", http::to_string(expected)},
                    {"got",     http::to_string(req.method())}
            }, 405);
            return false;
        }
        return true;
    }
}
