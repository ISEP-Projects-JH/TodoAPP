# Build and Usage Documentation

[Back to README](README.md)

This document explains how to build and use multi-architecture Docker images for your application. We will use **Docker Buildx** to create images for both `amd64` and `arm64` architectures.

---

## Build Multi-Architecture Docker Images

You can use **Docker Buildx** to build images for both `amd64` and `arm64` platforms.

> ⚠️ **Recommendation:**  
> Whenever possible, **build images on the same architecture as the target platform**.  
> Although Docker Buildx allows cross-architecture builds (e.g., building `amd64` on an ARM host), doing so is **highly discouraged** in our case. Here's why:

### 🔧 Why same-architecture builds are preferred?

In order to ensure compatibility with modern `Boost` and `CMake`, our Dockerfile compiles **Boost from source** (instead of using prebuilt system packages). This gives us:

- ✅ More consistent feature support across platforms
- ✅ Smaller final image size (no extra docs/dev headers)
- ✅ Compatibility with JH Toolkit and latest C++20 features

However, building Boost from source is **very CPU and memory intensive**, and attempting to do this on a different architecture (e.g., building `amd64` image on `arm64` host) results in:

- ❌ Excessive CPU usage
- ❌ Extremely slow builds
- ❌ Frequent build failures (especially under QEMU emulation)

Even on native architecture, **you must still use Buildx** instead of `docker build`, because:

- `docker build` may produce images that are **too tightly coupled to your host architecture**
- This affects long-term portability — especially on ARM, where not all chips are identical
- `buildx` ensures proper multi-platform image metadata and reproducibility

---

### Step 1: Enable Buildx and Create a Builder
First, enable **Buildx** and create a builder instance:

```bash
docker buildx create --use --name multiarch-builder
docker buildx inspect --bootstrap
```

This will ensure that Buildx is set up and ready to build images for multiple platforms.

### Step 2: Build `amd64` Image (For Intel/AMD PCs and Servers)
To build an `amd64` image suitable for common Intel/AMD PCs and servers:

```bash
docker buildx build \
  --platform linux/amd64 \
  -t todo-app:amd64 \
  --no-cache \
  --load .
```

### Step 3: Build `arm64` Image (For Apple M1/M2 or ARM Servers)
To build an `arm64` image suitable for Apple M1/M2 or ARM-based servers:

```bash
docker buildx build \
  --platform linux/arm64 \
  -t todo-app:arm64 \
  --no-cache \
  --load .
```

After building, you can save the images for later use:

```bash
docker save todo-app:amd64 -o todo-app.amd64.tar
```

```bash
docker save todo-app:arm64 -o todo-app.arm64.tar
```

---

### Step 4: Run Image as Container

```output
Registered routes:
/ping
/shutdown_server
/todo_get
/todo_exists
/todo_delete
/todo_erase
/todo_create
/todo_before
/todo_import
/todo_all
/todo_export
HTTP server running on port 8080...
```

The service is now running, and the server will listen on port `8080`.

---

## Service Management

### Graceful Shutdown

To stop the service properly, **always use** the `/shutdown_server` route. This will ensure that the service disconnects gracefully from the database and any other resources it might be using. This is important to prevent data loss or corruption.

To shut down the service, run the following:

```bash
curl http://localhost:8080/shutdown_server
```

Server will output:

```output
Called Exit
👋 Server exiting, cleaning up...
```

**Do not forcibly stop the Docker container** (e.g., using `docker stop` or closing the terminal window), as this can result in data loss. 
Always use the `/shutdown_server` endpoint to disconnect the service cleanly.

---


## ⚠️ Notes for Windows Users

If you are using **Windows (especially CMD or PowerShell)**, be aware of the following:

### ❗️Line Continuation `\` Not Supported

The following multi-line command syntax using backslash `\`:

```bash
docker buildx build \
  --platform linux/amd64 \
  -t todo-app:amd64 \
  --load .
```

is **not valid on Windows CMD or PowerShell**.

✅ On Windows, you must rewrite the command as a **single line**:

```cmd
docker buildx build --platform linux/amd64 -t todo-app:amd64 --load .
```

> 🔧 Tip: If you use Git Bash or WSL, the multi-line version **will** work.
