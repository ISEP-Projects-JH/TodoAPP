# 📘 API Reference: Todo App

[Back to README](README.md)

This document lists all available routes in your Todo HTTP server and demonstrates how to use them via `curl`.

---

## 📍 `/ping`

* **Method:** `GET`
* **Description:** Health check.

**Example:**

```bash
curl -X GET http://localhost:8080/ping
```

**Response:**

```json
{"status":"alive"}
```

---

## 📍 `/shutdown_server`

* **Method:** `POST`
* **Description:** Requests the server to shut down.

**Example:**

```bash
curl -X POST http://localhost:8080/shutdown_server
```

**Response:**

```json
{"status":"server_shutdown_requested"}
```

---

## 📍 `/todo_create`

* **Method:** `POST`
* **Description:** Creates a new todo.

**Body:** `application/json`

```json
{
  "name": "buy_milk",
  "due_date": "2025-05-12T18:00:00"
}
```

**Example:**

```bash
curl -X POST http://localhost:8080/todo_create \
  -H "Content-Type: application/json" \
  -d '{"name":"buy_milk", "due_date":"2025-05-12T18:00:00"}'
```

**Success Response:**

```json
{"status":"created"}
```

**Error Responses:**

```json
{"error":"Todo already exists"}
```
```json
{"error":"Invalid due_date format"}
```

---

## 📍 `/todo_all`

* **Method:** `GET`
* **Description:** Lists all todos.

**Example:**

```bash
curl -X GET http://localhost:8080/todo_all
```

**Response:**

```json
[
  {
    "name": "buy_milk",
    "due_date": "2025-05-12T18:00:00Z"
  }
]
```

---

## 📍 `/todo_get?name=<name>`

* **Method:** `GET`
* **Description:** Retrieves a todo by name.

**Example:**

```bash
curl -X GET "http://localhost:8080/todo_get?name=buy_milk"
```

**Success Response:**

```json
{
  "name": "buy_milk",
  "due_date": "2025-05-12T18:00:00Z"
}
```

**Error Response:**

```json
{"error":"Todo not found"}
```

---

## 📍 `/todo_exists`

* **Method:** `HEAD`
* **Description:** Checks if a todo exists (via header `name`).

**Example:**

```bash
curl -I -X HEAD http://localhost:8080/todo_exists -H "name: buy_milk"
```

**Response Code:**

* `200` – exists
* `404` – not found

---

## 📍 `/todo_before`

* **Method:** `POST`
* **Description:** Lists todos with due date ≤ given timestamp or ISO date.

**Body:**

```json
{
  "before": "2025-06-01T00:00:00"
}
```

**Example:**

```bash
curl -X POST http://localhost:8080/todo_before \
  -H "Content-Type: application/json" \
  -d '{"before": "2025-06-01T00:00:00"}'
```

**Response:**

```json
[
  {
    "name": "buy_milk",
    "due_date": "2025-05-12T18:00:00Z"
  }
]
```

---

## 📍 `/todo_delete`

* **Method:** `DELETE`
* **Header Required:** `Authorization: <name>`
* **Description:** Deletes a todo by name.

**Example:**

```bash
curl -X DELETE http://localhost:8080/todo_delete -H "Authorization: buy_milk"
```

**Success:**

```json
{"status":"deleted"}
```

**Error:**

```json
{"error":"Todo not found"}
```

---

## 📍 `/todo_erase`

* **Method:** `POST`
* **Description:** Erase todos with due date before timestamp or ISO.

**Body:**

```json
{
  "before": "2025-01-01T00:00:00"
}
```

**Example:**

```bash
curl -X POST http://localhost:8080/todo_erase \
  -H "Content-Type: application/json" \
  -d '{"before": "2025-01-01T00:00:00"}'
```

**Response:**

```json
{"status":"done"}
```

---

## 📍 `/todo_import`

* **Method:** `POST`
* **Description:** Imports todos from CSV content.

**As raw CSV (default clears existing):**

```bash
curl -X POST http://localhost:8080/todo_import \
  --data-binary @todos.csv
```

**As JSON with inline CSV content:**

```bash
curl -X POST http://localhost:8080/todo_import \
  -H "Content-Type: application/json" \
  -d '{"clear_before": false, "csv": "\"name\",\"due_date\"\n\"a\",123456\n"}'
```

**Response:**

```json
{"status":"imported"}
```

---

## 📍 `/todo_export`

* **Method:** `GET`
* **Description:** Exports current todos as a downloadable CSV file.

**Example:**

```bash
curl -OJ http://localhost:8080/todo_export
```

or

```bash
curl -X GET http://localhost:8080/todo_export -o todos.csv
```

to force download (covers the previous file).

**Response:**

* Triggers a download named `todos.csv`
* MIME type: `text/csv`

---


## ⚠️ Notes for Windows Users

If you are using **Windows (especially CMD or PowerShell)**, be aware of the following:

### ❗️Line Continuation `\` Not Supported

See [Build Documentation](build.md)

> 🔧 Tip: If you use Git Bash or WSL, the multi-line version **will** work.

---

### ❗️Curl Compatibility

Windows does not always come with `curl` preinstalled, or may have an older version. In such cases:

* You may need to install `curl` manually, or
* Use a tool like **Postman** or **PowerShell's `Invoke-WebRequest`**.

For example, this `curl` command:

```bash
curl -X POST http://localhost:8080/todo_create -H "Content-Type: application/json" -d "{\"name\":\"buy_milk\",\"due_date\":\"2025-05-12T18:00:00\"}"
```

Can be rewritten in PowerShell as:

```powershell
Invoke-WebRequest -Uri "http://localhost:8080/todo_create" `
  -Method POST `
  -Body '{"name":"buy_milk","due_date":"2025-05-12T18:00:00"}' `
  -ContentType "application/json"
```

---

## ✅ Recommended Alternatives to `curl`

While `curl` is great for quick API tests, in real development or cross-platform use, the following tools are more **user-friendly**, **readable**, and **cross-platform**:

### 1. 🐍 Python (using `requests`)

Python is available on most platforms and can be scripted easily:

```bash
pip install --user requests
```

```python
import requests

resp = requests.post(
    "http://localhost:8080/todo_create",
    json={
        "name": "buy_milk",
        "due_date": "2025-05-12T18:00:00"
    }
)
print(resp.status_code, resp.json())
```

✅ Advantages:

* Easy to read and debug
* Programmatic automation
* Handles JSON and errors gracefully

---

### 2. 🧪 Postman (GUI)

[Postman](https://www.postman.com/) is a popular visual API client for:

* Sending requests
* Testing and saving API calls
* Inspecting responses easily

✅ Best for:

* Manual testing
* Team collaboration
* Windows/macOS/Linux

---

### 3. 🌐 Web-based Tools

If your Todo API is exposed to the internet or running inside Docker with ports published, you can even test it via:

* Swagger UI
* Hoppscotch.io (a modern open-source Postman alternative)
