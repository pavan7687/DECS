#include "httplib.h"
#include <iostream>
#include <string>

using namespace std;

int main() {
    httplib::Client cli("http://127.0.0.1:8080");

    while (true) {
        cout << "\n1) Set\n2) Get\n3) Delete\n4) Exit\nChoice: ";
        int ch; if (!(cin >> ch)) break;
        if (ch == 4) break;

        int key; string value;
        if (ch == 1) {
            cout << "Enter key (int): "; cin >> key; cin.ignore();
            cout << "Enter value: "; getline(cin, value);
            auto res = cli.Post(("/set?id=" + to_string(key)).c_str(), value, "text/plain");
            if (res) cout << res->body;
            else cout << "Server not reachable\n";
        } else if (ch == 2) {
            cout << "Enter key: "; cin >> key;
            auto res = cli.Get(("/get?id=" + to_string(key)).c_str());
            if (res) cout << "Response: " << res->body;
            else cout << "Server not reachable\n";
        } else if (ch == 3) {
            cout << "Enter key: "; cin >> key;
            auto res = cli.Delete(("/del?id=" + to_string(key)).c_str());
            if (res) cout << res->body;
            else cout << "Server not reachable\n";
        } else {
            cout << "Invalid choice\n";
        }
    }
    return 0;
}
