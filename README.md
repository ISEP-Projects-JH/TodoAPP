# ✅ TodoAPP – Minimalistic C++20 HTTP Todo Service

[![C++20](https://img.shields.io/badge/C%2B%2B-20-violet.svg)](https://en.cppreference.com/w/cpp/20)
[![Boost](https://img.shields.io/badge/Boost-1.88-blue.svg)](https://www.boost.org/)
[![jh-toolkit](https://img.shields.io/badge/jh--toolkit-1.3.x--LTS-brightgreen)](https://github.com/JeongHan-Bae/JH-Toolkit/tree/1.3.x-LTS)
[![CMake](https://img.shields.io/badge/CMake-3.25%2B-blue)](https://cmake.org/)
[![Clang](https://img.shields.io/badge/Clang-14.0%2B-yellow)](https://clang.llvm.org/)
[![Docker](https://img.shields.io/badge/Docker-Buildx-lightgrey)](https://docs.docker.com/buildx/working-with-buildx/)

---

## 📌 Project Summary

This is a minimalistic but highly extensible **HTTP-based Todo management server** written in **modern C++20**, emphasizing simplicity, testability, and performance.

It follows a practical **Hexagonal Architecture** without relying on verbose virtual interface hierarchies — making it ideal for both education and small-scale system design.

---

## 📄 Third-Party Components

This project uses the following external dependencies:

* [**Boost**](https://www.boost.org/) – Boost 1.88 (`json`, `system`, `asio`, `beast`)
* [**jh-toolkit**](https://github.com/JeongHan-Bae/jh-toolkit) – POD types and memory-safe wrappers (`Apache 2.0`)

All are statically linked, and no runtime dependencies are required.

---

## ✨ Features

* ✅ RESTful API with more than 10 routes
* ✅ Dual-indexed in-memory repository (name & timestamp)
* ✅ CSV import/export (bulk insertion & backup)
* ✅ Modern build system with CMake + Ninja + Clang + libc++
* ✅ Docker multi-arch support (amd64/arm64)

---

## 🚀 How to Build and Run

### 🛠 Native (macOS/Linux)

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
./TodoAPP
```

> ✅ **Clang + libc++ only** tested (fully supported on macOS and most Linux distributions)
> ⚠️ **GCC is not tested**, and may fail due to header lookup or ABI quirks
> ❌ **Windows is unsupported** due to POSIX usage (`sigaction`, `timegm`, etc.)

---

### 🐳 Docker (Multi-Arch)

```bash
docker buildx build --platform linux/amd64 -t todo-app:amd64 --load .
docker run -p 8080:8080 todo-app:amd64
```

See [`build.md`](build.md) for complete Dockerfile and buildx instructions.

---

## 🔁 API Usage

See [`urls.md`](urls.md) for full list of routes and example curl usage.

---

## 🧱 Architecture Highlights

### ✅ Minimal HTTP Layer

* Custom router built over **Boost.Beast**
* Routes self-register via `REGISTER_VIEW(...)` macros
* Handlers are regular C++ functions — readable and testable

### ✅ Practical Hexagonal Design

* No virtual interfaces or DI frameworks
* Application logic is separate from I/O (e.g., CSV / HTTP / persistence)
* Can be extended to SQLite or external databases easily

### ✅ Fast Lookup: POD Key + Dual Index

* `jh::pod::array<char, 64>` used as hashmap key — **stack-allocated**, transparent lookup
* Memory-local and avoids dynamic allocation like `std::string`
* Dual index:

    * `by_name_`: O(1) lookup by name
    * `by_time_`: log(N) range-scan by timestamp

---

## 🔚 Graceful Shutdown

To shut down the server:

```bash
curl -X POST http://localhost:8080/shutdown_server
```

> ⚠️ Do not force-close (`docker stop`) — the server handles cleanup only via this route.

---

## 💡 Notes

* 📄 API: [`urls.md`](urls.md)
* 🛠 Build: [`build.md`](build.md)
* 🧠 Design overview: [`design.md`](design.md)
