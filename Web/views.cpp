#include "views.h"
#include "HttpUtils.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <boost/json.hpp>
#include <iostream>
#include <sstream>
#include "../Application/TodoManager.hpp"

namespace json = boost::json;
using http_util::Request;
using http_util::Response;
using http_util::check_method;
using http_util::set_json;

namespace download_types {
    constexpr jh::pod::array<char, 8> CSV = {"csv"};
}

using set_csv = http_util::set_download<download_types::CSV>;

std::unordered_map<std::string, views::HandlerFunc> views::function_map;

extern std::atomic<bool> g_should_exit;
extern std::unique_ptr<boost::asio::ip::tcp::acceptor> global_acceptor;

inline uint64_t parse_timestamp_field(const boost::json::value& val) {
    if (val.is_int64()) {
        return static_cast<uint64_t>(val.as_int64());
    } else if (val.is_string()) {
        return parse_date_string_to_timestamp(std::string(val.as_string()));
    } else {
        throw std::invalid_argument("Invalid timestamp format: must be integer or string");
    }
}

inline std::optional<std::string> get_query_param(const boost::beast::http::request<boost::beast::http::string_body>& req, std::string_view key) {
    std::string target = std::string(req.target());
    auto pos = target.find('?');
    if (pos == std::string::npos) return std::nullopt;

    std::string_view query = std::string_view(target).substr(pos + 1);
    while (!query.empty()) {
        auto eq = query.find('=');
        if (eq == std::string_view::npos) break;

        std::string_view k = query.substr(0, eq);
        query.remove_prefix(eq + 1);

        auto amp = query.find('&');
        std::string_view v = (amp == std::string_view::npos) ? query : query.substr(0, amp);
        if (amp != std::string_view::npos) query.remove_prefix(amp + 1);
        else query = {};

        if (k == key) return std::string(v);
    }

    return std::nullopt;
}


REGISTER_VIEW(ping) {
    if (!check_method(req, http_util::http::verb::get, res)) return;
    set_json(res, {{"status", "alive"}});
}

REGISTER_VIEW(shutdown_server) {
    if (!check_method(req, http_util::http::verb::post, res)) return;

    if (!g_should_exit.exchange(true)) {
        std::cout << "Called Exit\n";

        if (global_acceptor && global_acceptor->is_open()) {
            boost::system::error_code ec;
            global_acceptor->cancel(ec); // NOLINT
        }

        try {
            boost::asio::io_context ioc;
            boost::asio::ip::tcp::socket s(ioc);
            s.connect({boost::asio::ip::address_v4::loopback(), 8080});
        } catch (...) {}
    }

    set_json(res, {{"status", "server_shutdown_requested"}});
}

REGISTER_VIEW(todo_create) {
    if (!check_method(req, http_util::http::verb::post, res)) return;

    try {
        auto obj = json::parse(req.body()).as_object();
        Todo todo = parse_todo_from_json(obj);

        if (!TodoManager::add_todo(todo)) {
            set_json(res, {{"error", "Todo already exists"}}, 400);
        } else {
            set_json(res, {{"status", "created"}}, 201);
        }
    } catch (const std::exception& e) {
        set_json(res, {{"error", e.what()}}, 400);
    }
}

REGISTER_VIEW(todo_all) {
    if (!check_method(req, http_util::http::verb::get, res)) return;

    auto list = TodoManager::all();
    json::array arr;
    for (const auto& todo : list)
        arr.push_back(to_json(todo));

    set_json(res, arr);
}

REGISTER_VIEW(todo_get) {
    if (!check_method(req, http_util::http::verb::get, res)) return;

    auto name_opt = get_query_param(req, "name");
    if (!name_opt) {
        set_json(res, {{"error", "Missing 'name' parameter"}}, 400);
        return;
    }

    auto todo = TodoManager::get_todo(*name_opt);
    if (!todo) {
        set_json(res, {{"error", "Todo not found"}}, 404);
    } else {
        set_json(res, to_json(*todo));
    }
}


REGISTER_VIEW(todo_exists) {
    if (!check_method(req, http_util::http::verb::head, res)) return;

    auto name = req.base()["name"];
    if (name.empty() || !TodoManager::exists(name)) {
        res.result(http_util::http::status::not_found);
    } else {
        res.result(http_util::http::status::ok);
    }
    res.prepare_payload();
}

REGISTER_VIEW(todo_before) {
    if (!check_method(req, http_util::http::verb::post, res)) return;

    try {
        const auto obj = boost::json::parse(req.body()).as_object();
        uint64_t ts = parse_timestamp_field(obj.at("before"));

        auto todos = TodoManager::range_before(ts);
        boost::json::array arr;
        for (const auto& t : todos) arr.push_back(to_json(t));
        set_json(res, arr);
    } catch (const std::exception& e) {
        set_json(res, {{"error", e.what()}}, 400);
    }
}

REGISTER_VIEW(todo_delete) {
    if (!check_method(req, http_util::http::verb::delete_, res)) return;

    std::string_view name = req[http_util::http::field::authorization];
    if (name.empty()) {
        set_json(res, {{"error", "Missing 'name' header"}}, 400);
        return;
    }

    if (!TodoManager::erase(name)) {
        set_json(res, {{"error", "Todo not found"}}, 404);
    } else {
        set_json(res, {{"status", "deleted"}});
    }
}


REGISTER_VIEW(todo_erase) {
    if (!check_method(req, http_util::http::verb::post, res)) return;

    try {
        auto obj = json::parse(req.body()).as_object();
        uint64_t ts = parse_timestamp_field(obj.at("before"));
        TodoManager::erase_expired(ts);
        set_json(res, {{"status", "done"}});
    } catch (const std::exception& e) {
        set_json(res, {{"error", e.what()}}, 400);
    }
}

REGISTER_VIEW(todo_import) {
    if (!check_method(req, http_util::http::verb::post, res)) return;

    try {
        bool clear = true;

        if (http_util::is_json(req)) {
            auto obj = boost::json::parse(req.body()).as_object();
            if (obj.contains("clear_before") && obj.at("clear_before").is_bool()) {
                clear = obj.at("clear_before").as_bool();
            }
            std::istringstream csv_stream(obj.contains("csv") ? obj.at("csv").as_string().c_str() : "");
            TodoManager::load_from_csv(csv_stream, clear);
        } else {
            std::istringstream ss(req.body());
            TodoManager::load_from_csv(ss, clear);
        }

        set_json(res, {{"status", "imported"}});
    } catch (...) {
        set_json(res, {{"error", "Failed to import"}}, 400);
    }
}


REGISTER_VIEW(todo_export) {
    if (!check_method(req, http_util::http::verb::get, res)) return;

    std::ostringstream ss;
    TodoManager::save_to_csv(ss);
    set_csv::apply(res, ss.str(), "todos.csv");
}
