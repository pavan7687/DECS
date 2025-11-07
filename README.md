
# 🔑 Key-Value Store (C++ | LRU Cache | PostgreSQL)

A simple **client-server key-value store** built in **C++17**, featuring an **LRU cache** for fast access and a **PostgreSQL database** for persistent storage.

## ⚙️ Features

* REST API: `/set`, `/get`, `/del`
* LRU caching for fast lookups
* PostgreSQL backend for data persistence
* Connection pooling for efficiency
* CLI client for interaction

---

## 🏗️ Run Instructions

```bash
# Compile
g++ kv_server.cpp -o kv_server -lpq -pthread -std=c++17
g++ kv_client.cpp -o kv_client -pthread -std=c++17

# Start Server
./kv_server

# Run Client
./kv_client
```

---

## 🗄️ Database Setup

```sql
CREATE DATABASE kvstore;
CREATE TABLE kv_store (
  k INT PRIMARY KEY,
  v TEXT
);
```

---

## 🧠 Overview

The client sends HTTP requests to the server.
The server checks the **LRU cache** first — if the key is missing, it fetches data from **PostgreSQL**, updates the cache, and returns the result.

---

**Author:** Pavan Kumar
**Project:** DECS – Key-Value Store System

---
