#pragma once
#include <jh/pod>
#include <string>
#include <string_view>
#include <cstdint>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <ctime> // Unix Only for timegm
#include <boost/json.hpp>

#pragma once
#include <jh/pod>
#include <string_view>
#include <cstdint>

struct Todo final {
    jh::pod::array<char, 64> name{};
    uint64_t due_timestamp = UINT64_MAX;

    [[nodiscard]] std::string_view name_view() const {
        return (name[63] == '\0')
               ? std::string_view(name.begin())
               : std::string_view(name.begin(), 64);
    }
};



struct TodoNameHash {
    using is_transparent [[maybe_unused]] = void; // NOLINT

    std::size_t operator()(const jh::pod::array<char, 64>& name) const noexcept {
        auto len = strnlen(name.data, jh::pod::array<char, 64>::size());
        return jh::pod::bytes_view::from(name.data, len).hash();
    }

    std::size_t operator()(std::string_view view) const noexcept {
        auto len = strnlen(view.data(), view.size());
        return jh::pod::bytes_view::from(view.data(), len).hash();
    }

    std::size_t operator()(const char* cstr) const noexcept {
        return operator()(std::string_view(cstr));
    }
};

struct TodoNameEqual {
    using is_transparent [[maybe_unused]] = void; // NOLINT

    bool operator()(const jh::pod::array<char, 64>& a, const jh::pod::array<char, 64>& b) const noexcept {
        size_t len_a = strnlen(a.data, jh::pod::array<char, 64>::size());
        size_t len_b = strnlen(b.data, jh::pod::array<char, 64>::size());
        return len_a == len_b && std::memcmp(a.data, b.data, len_a) == 0;
    }

    bool operator()(const jh::pod::array<char, 64>& a, std::string_view b) const noexcept {
        size_t len_a = strnlen(a.data, jh::pod::array<char, 64>::size());
        return len_a == b.size() && std::memcmp(a.data, b.data(), len_a) == 0;
    }

    bool operator()(std::string_view a, const jh::pod::array<char, 64>& b) const noexcept {
        return operator()(b, a);
    }

    bool operator()(const jh::pod::array<char, 64>& a, const char* b) const noexcept {
        return operator()(a, std::string_view(b));
    }

    bool operator()(const char* a, const jh::pod::array<char, 64>& b) const noexcept {
        return operator()(b, a);
    }
};


inline std::string timestamp_to_iso_string(uint64_t timestamp) {
    auto t = static_cast<std::time_t>(timestamp);
    std::tm* tm = std::gmtime(&t);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", tm);
    return buf;
}


inline boost::json::object to_json(const Todo& todo) {
    boost::json::object obj;
    obj["name"] = todo.name_view();
    if (todo.due_timestamp != UINT64_MAX)
        obj["due_date"] = timestamp_to_iso_string(todo.due_timestamp);
    return obj;
}

inline uint64_t parse_date_string_to_timestamp(const std::string& date_str) {
    std::tm tm{};
    std::istringstream ss(date_str);

    if (date_str.size() == 10) {
        ss >> std::get_time(&tm, "%Y-%m-%d");
    } else if (date_str.size() >= 16) {
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
        if (ss.fail()) {
            ss.clear();
            ss.str(date_str);
            ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M");
        }
    } else {
        throw std::invalid_argument("Invalid due_date format");
    }

    if (ss.fail()) throw std::invalid_argument("Failed to parse due_date");

    // UTC
    return static_cast<uint64_t>(timegm(&tm));
}


inline Todo parse_todo_from_json(const boost::json::object& obj) {
    Todo todo;

    // 1. name -> pod buffer
    if (!obj.contains("name") || !obj.at("name").is_string())
        throw std::invalid_argument("Missing or invalid 'name'");

    std::string name_str = std::string(obj.at("name").as_string());
    if (name_str.size() >= jh::pod::array<char, 64>::size())
        throw std::invalid_argument("Todo name too long");

    std::memcpy(todo.name.data, name_str.data(), name_str.size());

    // 2. due_date: support int or string
    if (obj.contains("due_date")) {
        const auto& val = obj.at("due_date");

        if (val.is_int64()) {
            todo.due_timestamp = val.as_int64();
        } else if (val.is_string()) {
            todo.due_timestamp = parse_date_string_to_timestamp(val.as_string().c_str());
        } else {
            throw std::invalid_argument("Invalid 'due_date' type");
        }
    } else {
        todo.due_timestamp = UINT64_MAX;
    }

    return todo;
}
