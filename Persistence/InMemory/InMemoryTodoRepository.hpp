#pragma once

#include <unordered_map>
#include <map>
#include <shared_mutex>
#include <optional>
#include <vector>
#include <memory_resource>
#include <iterator>
#include "../../Entity/Todo.hpp"

class CSVHandler;

class InMemoryTodoRepository {
public:
    static InMemoryTodoRepository &instance() {
        static InMemoryTodoRepository repo;
        return repo;
    }

    bool add(const Todo &todo) {
        std::unique_lock lock(mutex_);
        if (by_name_.contains(todo.name)) return false;

        by_name_.emplace(todo.name, todo.due_timestamp);
        by_time_.emplace(todo.due_timestamp, todo.name);
        return true;
    }

    template<typename Container>
    requires (
    requires(Container c) {
        { c.begin() } -> std::input_iterator;
        { c.end() } -> std::sentinel_for<decltype(c.begin())>;
        { c.size() } -> std::convertible_to<std::uint64_t>;
    }
    )
    void batch_add(const Container &todos) {
        std::unique_lock lock(mutex_);

        std::pmr::monotonic_buffer_resource pool;
        std::pmr::vector<std::pair<jh::pod::array<char, 64>, uint64_t>> name_index{&pool};
        std::pmr::vector<std::pair<uint64_t, jh::pod::array<char, 64>>> time_index{&pool};

        name_index.reserve(todos.size());
        time_index.reserve(todos.size());

        for (const auto &todo: todos) {
            auto it = by_name_.find(todo.name);

            if (it != by_name_.end()) {
                if (it->second != todo.due_timestamp) {
                    by_time_.erase(it->second);
                    it->second = todo.due_timestamp;
                    by_time_.emplace(todo.due_timestamp, todo.name);
                }
            } else {
                name_index.emplace_back(todo.name, todo.due_timestamp);
                time_index.emplace_back(todo.due_timestamp, todo.name);
            }
        }

        by_name_.insert(
                std::make_move_iterator(name_index.begin()),
                std::make_move_iterator(name_index.end())
        );

        by_time_.insert(
                std::make_move_iterator(time_index.begin()),
                std::make_move_iterator(time_index.end())
        );
    }

    bool exists(std::string_view name) const {
        std::shared_lock lock(mutex_);
        return by_name_.contains(name);
    }

    std::optional<Todo> get(std::string_view name) const {
        std::shared_lock lock(mutex_);
        auto it = by_name_.find(name);
        if (it == by_name_.end()) return std::nullopt;
        return Todo{it->first, it->second};
    }

    std::vector<Todo> range_before(uint64_t timestamp) const {
        std::shared_lock lock(mutex_);
        std::vector<Todo> result;

        auto end_it = by_time_.upper_bound(timestamp);
        result.reserve(std::distance(by_time_.begin(), end_it));

        std::transform(by_time_.begin(), end_it, std::back_inserter(result),
                       [](const auto &pair) {
                           return Todo{pair.second, pair.first};
                       });

        return result;
    }

    std::vector<Todo> unsafe_get_all() const {
        return range_before(UINT64_MAX);
    }

    void clear() {
        std::unique_lock lock(mutex_);
        by_name_.clear();
        by_time_.clear();
    }

    bool erase(std::string_view name) {
        auto todo = get(name);
        if (!todo) return false;
        const auto&[nm, ts] = todo.value();
        by_name_.erase(nm);
        by_time_.erase(ts);
        return true;
    }

    void erase_before(uint64_t timestamp) {
        std::unique_lock lock(mutex_);

        auto end_it = by_time_.upper_bound(timestamp);

        std::vector<std::pair<uint64_t, jh::pod::array<char, 64>>> to_erase;
        to_erase.reserve(std::distance(by_time_.begin(), end_it));

        std::copy(by_time_.begin(), end_it, std::back_inserter(to_erase));

        for (const auto &[ts, name]: to_erase) {
            by_name_.erase(name);
            by_time_.erase(ts);
        }
    }

private:
    InMemoryTodoRepository() = default;

    mutable std::shared_mutex mutex_;

    friend CSVHandler;

    // double index by : name / time
    std::unordered_map<jh::pod::array<char, 64>, uint64_t, TodoNameHash, TodoNameEqual> by_name_;
    std::map<uint64_t, jh::pod::array<char, 64>> by_time_;
};
