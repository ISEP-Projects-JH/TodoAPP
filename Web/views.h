#pragma once

#include <string>
#include <unordered_map>
#include "HttpUtils.hpp"

namespace views {

    using HandlerFunc = void (*)(const http_util::Request& req, http_util::Response& res);

    // Declare global function map
    extern std::unordered_map<std::string, HandlerFunc> function_map;

    // Macro for auto-registering views
    #define REGISTER_VIEW(name) \
        void name(const http_util::Request& req, http_util::Response& res); \
        struct name##_registrar { \
            name##_registrar() { views::function_map[#name] = name; } \
        } name##_registrar_instance; \
        void name(const http_util::Request& req, http_util::Response& res)

}
