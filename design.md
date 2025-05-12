# 🧠 Architecture Design: Why This Is Better

[Back to README](README.md)

This project uses a **Singleton-based Hexagonal Architecture**, optimized for performance, simplicity, and real-world usage patterns. Here's why this approach outperforms the traditional "pure virtual interface + multiple adapters" strategy in this context:

---

## ✅ Hexagonal Architecture without Virtual Interfaces

While traditional Hexagonal Architecture suggests injecting repository interfaces (e.g., `ITodoRepository`) and implementing them separately (`InMemoryTodoRepository`, `CsvTodoRepository`, etc.), this design:

* Uses **a single in-memory repository as a Singleton**
* Directly exposes its API to both the application logic and CSV import/export adapter
* Avoids unnecessary indirection, vtables, and over-engineering for trivial IO formats

### 🔄 Why not use `ITodoRepository`?

CSV is not a random-access medium — it's inherently **not meant for frequent reading and writing**, and adapting it as a drop-in repository is misleading. Instead, this design treats CSV as an import/export *adapter*, not as a full repository implementation.

This allows:

* Efficient interaction with the central in-memory store
* More accurate control over *when* data is persisted
* Better semantic separation: **CSV is a persistence format, not a data source**

---

## 🧵 Efficient Data Layout: `jh::pod::array<char, 64>`

Instead of using `std::string`, this project uses a fixed-size `pod::array<char, 64>` for all Todo names. Here's why:

* Avoids heap allocations (i.e., `std::string` stores metadata + heap pointer)
* Enables **flat storage** inside containers (`std::unordered_map`, `std::map`, `std::vector`)
* Improves **cache locality** and lookup performance
* Provides **compile-time guarantees** on max length (required by business rules)

### 📦 Why not `std::string`?

In high-performance maps or large-scale containers, `std::string` introduces:

* **Indirect memory access**
* **Worse CPU cache utilization**
* **Additional memory fragmentation**

Using a 64-byte buffer directly in-place guarantees better behavior, especially under heavy insert or lookup workloads.

---

## 🌲 Dual Indexing: Name + Timestamp

The in-memory repository keeps **two indices**:

1. `by_name_`: `unordered_map<name, timestamp>`
2. `by_time_`: `map<timestamp, name>`

This design enables:

* **Fast existence and retrieval** by name (O(1) average)
* **Efficient range queries** by time (logarithmic due to balanced tree)

The dual-index structure ensures that both user-facing (name) and system-facing (timestamp) operations remain efficient without scanning the full dataset.

---

## 📁 CSV Interop as One-Time Adapters

CSV is supported via a dedicated `CSVHandler`:

* Exports all todos in one go
* Imports a full list (optionally clearing in-memory state)

This reflects real-world usage of CSV — for *backups*, *data migrations*, or *initialization* — not continuous persistence.

---

## 💡 Summary

| Feature           | Implementation                         | Justification                                     |
|-------------------|----------------------------------------|---------------------------------------------------|
| Architecture      | Singleton Hexagonal Architecture       | Minimal indirection; simpler and faster           |
| Repository        | In-memory, exposed globally            | CSV is adapter-only; avoids pointless abstraction |
| Data layout       | `pod::array<char, 64>`                 | Zero-allocation, cache-friendly, static bound     |
| Query performance | Dual indices (`unordered_map` + `map`) | Optimal for both name and time lookup             |
| CSV interaction   | One-time read/write via `CSVHandler`   | More efficient and semantically correct           |

---


## 🔧 Extensibility: `by_name_` as Primary Map, `by_time_` as View

This architecture is **naturally extensible** without changing the core repository logic.

Currently:

```cpp
std::unordered_map<pod::array<char, 64>, timestamp> by_name_;
std::map<timestamp, pod::array<char, 64>> by_time_;
```

### 🧱 Future-Proofing with Struct Values

You can easily evolve this into:

```cpp
struct TodoDetails {
    uint64_t due_timestamp;
    std::string_view description;
    // ... more metadata
};

std::unordered_map<pod::array<char, 64>, TodoDetails> by_name_;
std::map<uint64_t, pod::array<char, 64>> by_time_;
```

This maintains:

* ✅ **O(1)** name lookup via `unordered_map`
* ✅ Efficient **range scanning** via `map` on timestamps
* ✅ Minimal structural change — only update insertion/update logic

### 🌐 What It Enables

* Richer metadata (e.g., priority, tags, notes)
* Advanced filters (e.g., todos by due date and tag)
* Search optimizations (e.g., secondary time-based indices on subsets)

Because `by_time_` just maps timestamps back to primary keys (`name`), it acts as a **projection**, and doesn't constrain your ability to evolve the actual data payload.

