#pragma once

#include <ostream>
#include <istream>
#include <string>
#include <string_view>
#include <vector>
#include <charconv>
#include <memory_resource>
#include <stdexcept>
#include "../../Entity/Todo.hpp"
#include "../InMemory/InMemoryTodoRepository.hpp"

class CSVHandler {
public:
    // convert to CSV，including label
    static void save(std::ostream &os) {
        const InMemoryTodoRepository &repo = InMemoryTodoRepository::instance();
        std::shared_lock lock(repo.mutex_);
        os << "\"name\",\"due_date\"\n";
        for (const auto &[name, ts]: repo.by_name_) {
            std::size_t len = strnlen(name.data, jh::pod::array<char, 64>::size());
            os << '"' << std::string_view(name.data, len) << "\"," << ts << '\n';
        }
    }

    // from csv
    static void load(std::istream& is, bool clear_before = true) {
        InMemoryTodoRepository& repo = InMemoryTodoRepository::instance();
        if (clear_before) repo.clear();
        std::pmr::monotonic_buffer_resource pool;
        std::pmr::vector<Todo> todos(&pool);

        std::string line;
        bool first_line = true;

        while (std::getline(is, line)) {
            if (line.empty()) continue;

            std::string_view sv_line = line;

            while (!sv_line.empty() && sv_line.front() == ' ') sv_line.remove_prefix(1);
            while (!sv_line.empty() && sv_line.back() == ' ') sv_line.remove_suffix(1);

            if (first_line) {
                first_line = false;
                if (is_label_line(sv_line)) continue;  // skip label
            }

            todos.emplace_back(parse_csv_line(sv_line));
        }

        repo.batch_add(todos);
    }

private:
    // is label
    static bool is_label_line(std::string_view line) {
        return line.starts_with("\"name") || line.starts_with("name");
    }

    // csv to t
    static Todo parse_csv_line(std::string_view line) {
        auto comma = line.find(',');
        if (comma == std::string_view::npos)
            throw std::invalid_argument("Invalid CSV format");

        std::string_view name_part = line.substr(0, comma);
        while (!name_part.empty() && name_part.front() == ' ') name_part.remove_prefix(1);
        while (!name_part.empty() && name_part.back() == ' ') name_part.remove_suffix(1);

        std::string_view ts_part = line.substr(comma + 1);
        while (!ts_part.empty() && ts_part.front() == ' ') ts_part.remove_prefix(1);
        while (!ts_part.empty() && ts_part.back() == ' ') ts_part.remove_suffix(1);


        if (name_part.size() >= 2 && name_part.front() == '"' && name_part.back() == '"') {
            name_part.remove_prefix(1);
            name_part.remove_suffix(1);
        }

        if (name_part.size() >= 64)
            throw std::invalid_argument("Name too long in CSV");

        Todo todo;
        std::memcpy(todo.name.data, name_part.data(), name_part.size());

        auto [ptr, ec] = std::from_chars(ts_part.data(), ts_part.data() + ts_part.size(), todo.due_timestamp);
        if (ec != std::errc()) throw std::invalid_argument("Invalid due_timestamp");

        return todo;
    }
};
