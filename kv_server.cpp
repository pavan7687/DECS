#include "httplib.h"
#include <iostream>
#include <unordered_map>
#include <list>
#include <mutex>
#include <string>
#include <libpq-fe.h>
#include <vector>
#include <condition_variable>
using namespace std;

#define CACHE_SIZE 20
#define POOL_SIZE 8

class PGPool {
    vector<PGconn*> pool;
    mutex m;
    condition_variable cv;
public:
    PGPool(int n, const string &conninfo) {
        for (int i = 0; i < n; i++) {
            PGconn *c = PQconnectdb(conninfo.c_str());
            if (PQstatus(c) != CONNECTION_OK) {
                cerr << "DB connection failed: " << PQerrorMessage(c) << endl;
                exit(1);
            }
            pool.push_back(c);
        }
        cout << "Database connection pool created with " << n << " connections.\n";
    }

    PGconn* acquire() {
        unique_lock<mutex> lk(m);
        cv.wait(lk, [&]{ return !pool.empty(); });
        PGconn* c = pool.back();
        pool.pop_back();
        return c;
    }

    void release(PGconn* c) {
        unique_lock<mutex> lk(m);
        pool.push_back(c);
        lk.unlock();
        cv.notify_one();
    }

    ~PGPool() {
        for (auto c : pool) PQfinish(c);
    }
};

class LRUCache {
    int cap;
    unordered_map<int, pair<string, list<int>::iterator>> mp;
    list<int> lru;
    mutex m;
public:
    LRUCache(int c = CACHE_SIZE): cap(c) {}

    void put(int key, const string &val) {
        lock_guard<mutex> lk(m);
        if (mp.count(key)) {
            lru.erase(mp[key].second);
        } else if ((int)mp.size() == cap) {
            int old = lru.back();
            lru.pop_back();
            mp.erase(old);
            cout << "[Cache] Evicted key: " << old << endl;
        }
        lru.push_front(key);
        mp[key] = {val, lru.begin()};
    }

    string get(int key) {
        lock_guard<mutex> lk(m);
        if (!mp.count(key)) return "";
        auto val = mp[key].first;
        lru.erase(mp[key].second);
        lru.push_front(key);
        mp[key].second = lru.begin();
        return val;
    }

    bool del(int key) {
        lock_guard<mutex> lk(m);
        if (!mp.count(key)) return false;
        lru.erase(mp[key].second);
        mp.erase(key);
        return true;
    }
};

int main() {
    string conninfo = "host=localhost port=5432 dbname=kvstore user=postgres password=Pavan@000099";

    PGPool dbpool(POOL_SIZE, conninfo);
    LRUCache cache(CACHE_SIZE);

    {
        PGconn* c = dbpool.acquire();
        PGresult* r = PQexec(c, "SELECT k, v FROM kv_store;");
        if (PQresultStatus(r) == PGRES_TUPLES_OK) {
            int rows = PQntuples(r);
            for (int i = 0; i < rows && i < CACHE_SIZE; i++) {
                int key = stoi(PQgetvalue(r, i, 0));
                string val = PQgetvalue(r, i, 1);
                cache.put(key, val);
            }
            cout << "[Cache] Preloaded " << min(rows, CACHE_SIZE) << " entries from DB.\n";
        }
        PQclear(r);
        dbpool.release(c);
    }

    httplib::Server server;

    server.Post("/set", [&](const httplib::Request &req, httplib::Response &res) {
        if (!req.has_param("id")) {
            res.status = 400;
            res.set_content("Missing id\n", "text/plain");
            return;
        }

        int key = stoi(req.get_param_value("id"));
        string value = req.body;

        cache.put(key, value);

        PGconn* c = dbpool.acquire();
        string q = "INSERT INTO kv_store (k, v) VALUES (" + to_string(key) + ", '" + value +
                   "') ON CONFLICT (k) DO UPDATE SET v='" + value + "';";
        PGresult* r = PQexec(c, q.c_str());
        if (PQresultStatus(r) != PGRES_COMMAND_OK) {
            cerr << "DB Error: " << PQerrorMessage(c);
        }
        PQclear(r);
        dbpool.release(c);

        cout << "[SET] key=" << key << " value=" << value << endl;
        res.set_content("OK\n", "text/plain");
    });

    server.Get("/get", [&](const httplib::Request &req, httplib::Response &res) {
        if (!req.has_param("id")) {
            res.status = 400;
            res.set_content("Missing id\n", "text/plain");
            return;
        }

        int key = stoi(req.get_param_value("id"));
        string val = cache.get(key);

        if (val != "") {
            cout << "[LRU HIT] key=" << key << endl;   
            res.set_content(val + "\n", "text/plain");
            return;
        }

        PGconn* c = dbpool.acquire();
        string q = "SELECT v FROM kv_store WHERE k=" + to_string(key) + ";";
        PGresult* r = PQexec(c, q.c_str());
        if (PQresultStatus(r) == PGRES_TUPLES_OK && PQntuples(r) > 0) {
            val = PQgetvalue(r, 0, 0);
            cache.put(key, val);
            cout << "[DISK FETCH] key=" << key << endl;  
            res.set_content(val + "\n", "text/plain");
        } else {
            res.status = 404;
            res.set_content("Key not found\n", "text/plain");
        }
        PQclear(r);
        dbpool.release(c);
    });

    server.Delete("/del", [&](const httplib::Request &req, httplib::Response &res) {
        if (!req.has_param("id")) {
            res.status = 400;
            res.set_content("Missing id\n", "text/plain");
            return;
        }

        int key = stoi(req.get_param_value("id"));
        bool deleted = false;

        PGconn* c = dbpool.acquire();
        string q = "DELETE FROM kv_store WHERE k=" + to_string(key) + ";";
        PGresult* r = PQexec(c, q.c_str());
        if (PQresultStatus(r) == PGRES_COMMAND_OK) {
            deleted = (string(PQcmdTuples(r)) != "0");
        }
        PQclear(r);
        dbpool.release(c);

        cache.del(key);

        if (deleted) {
            cout << "[DELETE] key=" << key << endl;
            res.set_content("Deleted\n", "text/plain");
        } else {
            res.status = 404;
            res.set_content("Key not found\n", "text/plain");
        }
    });

    server.new_task_queue = [] { return new httplib::ThreadPool(8); };

    cout << "\nKV Server running at http://localhost:8080\n";
    server.listen("0.0.0.0", 8080);
}
