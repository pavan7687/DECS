
# Key-Value Store (C++ and PostgreSQL)

This project is a simple key-value store made using **C++** and **PostgreSQL**.
It has a **server** and a **client** program.
The server uses an **LRU cache** for fast access and stores all data permanently in the database.

---

## Features

* `/set`, `/get`, `/del` API support
* LRU cache for faster data access
* PostgreSQL for storing data
* Command-line client for testing

---

## How to Run

```bash
# Compile
g++ kv_server.cpp -o kv_server -lpq -pthread -std=c++17 -I/usr/include/postgresql
g++ kv_client.cpp -o kv_client -pthread -std=c++17

# Start the server
./kv_server

# Run the client in another terminal
./kv_client
```

---

## Database Setup

Open PostgreSQL and run:

```sql
CREATE DATABASE kvstore;
\c kvstore
CREATE TABLE kv_store (
  k INT PRIMARY KEY,
  v TEXT
);
```

---

## Working

* The **client** sends requests to the **server**.
* The **server** checks the cache first.
* If data is not in cache, it gets it from **PostgreSQL** and stores it in cache.
* Data stays safe in the database even after restarting the server.

---

**Author:** Pavan Kumar
**Project:** DECS – Key-Value Store

---
