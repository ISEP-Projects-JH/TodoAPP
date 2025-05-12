#pragma once

#include "../Entity/Todo.hpp"
#include "../Persistence/InMemory/InMemoryTodoRepository.hpp"
#include "../Persistence/CsvFiles/CSVHandler.hpp"

class TodoManager {
public:
    static bool add_todo(const Todo& todo) {
        return InMemoryTodoRepository::instance().add(todo);
    }

    static std::optional<Todo> get_todo(std::string_view name) {
        return InMemoryTodoRepository::instance().get(name);
    }

    static bool exists(std::string_view name) {
        return InMemoryTodoRepository::instance().exists(name);
    }

    static bool erase(std::string_view name) {
        return InMemoryTodoRepository::instance().erase(name);
    }

    static void erase_expired(uint64_t before) {
        InMemoryTodoRepository::instance().erase_before(before);
    }

    static std::vector<Todo> range_before(uint64_t ts) {
        return InMemoryTodoRepository::instance().range_before(ts);
    }

    static std::vector<Todo> all() {
        return InMemoryTodoRepository::instance().unsafe_get_all();
    }

    static void load_from_csv(std::istream& is, bool clear_before = true) {
        CSVHandler::load(is, clear_before);
    }

    static void save_to_csv(std::ostream& os) {
        CSVHandler::save(os);
    }
};
