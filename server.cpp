#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <sqlite3.h>

using namespace std;

#define PORT 54000
#define BUF 1024

sqlite3* db;

void init_db() {
    sqlite3_open("chat.db", &db);
    sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS users (login TEXT UNIQUE, password_hash TEXT, name TEXT);", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS messages (sender TEXT, receiver TEXT, text TEXT);", nullptr, nullptr, nullptr);
}

string get_users() {
    string result;
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "SELECT login FROM users;", -1, &stmt, nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        result += string((const char*)sqlite3_column_text(stmt, 0)) + ",";
    }
    sqlite3_finalize(stmt);
    return result;
}

string get_history(const string& login) {
    string result;
    sqlite3_stmt* stmt;
    string sql = "SELECT sender, receiver, text FROM messages WHERE sender='" + login + "' OR receiver='" + login + "';";
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        result += string((const char*)sqlite3_column_text(stmt, 0)) + "|" +
                  string((const char*)sqlite3_column_text(stmt, 1)) + "|" +
                  string((const char*)sqlite3_column_text(stmt, 2)) + "\n";
    }
    sqlite3_finalize(stmt);
    return result;
}

bool register_user(const string& login, const string& hash, const string& name) {
    string sql = "INSERT INTO users VALUES ('" + login + "', '" + hash + "', '" + name + "');";
    return sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr) == SQLITE_OK;
}

bool login_user(const string& login, const string& hash) {
    sqlite3_stmt* stmt;
    string sql = "SELECT COUNT(*) FROM users WHERE login='" + login + "' AND password_hash='" + hash + "';";
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    sqlite3_step(stmt);
    int count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    return count > 0;
}

void save_message(const string& sender, const string& receiver, const string& text) {
    string sql = "INSERT INTO messages VALUES ('" + sender + "', '" + receiver + "', '" + text + "');";
    sqlite3_exec(db, sql.c_str(), nullptr, nullptr, nullptr);
}

int main() {
    init_db();

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 5);

    cout << "Server running on port " << PORT << endl;

    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) continue;

        char buffer[BUF]{};
        read(client_fd, buffer, BUF);
        string msg(buffer);

        if (msg.find("REGISTER|") == 0) {
            size_t p1 = msg.find('|');
            size_t p2 = msg.find('|', p1 + 1);
            size_t p3 = msg.find('|', p2 + 1);
            string login = msg.substr(p1 + 1, p2 - p1 - 1);
            string pass = msg.substr(p2 + 1, p3 - p2 - 1);
            string name = msg.substr(p3 + 1);
            string hash = to_string(hash<string>{}(pass));

            if (register_user(login, hash, name))
                write(client_fd, "OK", 2);
            else
                write(client_fd, "ERROR", 5);
        }
        else if (msg.find("LOGIN|") == 0) {
            size_t p1 = msg.find('|');
            size_t p2 = msg.find('|', p1 + 1);
            string login = msg.substr(p1 + 1, p2 - p1 - 1);
            string pass = msg.substr(p2 + 1);
            string hash = to_string(hash<string>{}(pass));

            if (login_user(login, hash))
                write(client_fd, "OK", 2);
            else
                write(client_fd, "ERROR", 5);
        }
        else if (msg.find("SEND|") == 0) {
            size_t p1 = msg.find('|');
            size_t p2 = msg.find('|', p1 + 1);
            size_t


p3 = msg.find('|', p2 + 1);
            string sender = msg.substr(p1 + 1, p2 - p1 - 1);
            string receiver = msg.substr(p2 + 1, p3 - p2 - 1);
            string text = msg.substr(p3 + 1);

            save_message(sender, receiver, text);
            write(client_fd, "OK", 2);
        }
        else if (msg == "GET_USERS") {
            string response = get_users();
            write(client_fd, response.c_str(), response.size());
        }
        else if (msg.find("GET_HISTORY|") == 0) {
            string login = msg.substr(12);
            string history = get_history(login);
            write(client_fd, history.c_str(), history.size());
        }

        close(client_fd);
    }

    sqlite3_close(db);
    close(server_fd);
    return 0;
}
