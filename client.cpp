#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

using namespace std;

#define PORT 54000
#define BUF 4096

string current_user;

int connect_to_server() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);
    connect(sock, (sockaddr*)&server, sizeof(server));
    return sock;
}

string send_request(const string& request) {
    int sock = connect_to_server();
    write(sock, request.c_str(), request.size());
    char buffer[BUF]{};
    read(sock, buffer, BUF);
    close(sock);
    return string(buffer);
}

int main() {
    cout << "1. Register\n2. Login\nChoice: ";
    int choice;
    cin >> choice;
    cin.ignore();

    string login, password, name;

    if (choice == 1) {
        cout << "Login: "; getline(cin, login);
        cout << "Password: "; getline(cin, password);
        cout << "Name: "; getline(cin, name);

        string response = send_request("REGISTER|" + login + "|" + password + "|" + name);
        if (response == "OK") {
            cout << "Registered!\n";
            current_user = login;
        } else {
            cout << "Failed\n";
            return 1;
        }
    } else {
        cout << "Login: "; getline(cin, login);
        cout << "Password: "; getline(cin, password);

        string response = send_request("LOGIN|" + login + "|" + password);
        if (response == "OK") {
            cout << "Logged in!\n";
            current_user = login;
        } else {
            cout << "Failed\n";
            return 1;
        }
    }

    while (true) {
        cout << "\n1. Send\n2. Users\n3. History\n4. Exit\nChoice: ";
        int option;
        cin >> option;
        cin.ignore();

        if (option == 1) {
            string receiver, text;
            cout << "Receiver: "; getline(cin, receiver);
            cout << "Message: "; getline(cin, text);
            send_request("SEND|" + current_user + "|" + receiver + "|" + text);
            cout << "Sent\n";
        }
        else if (option == 2) {
            string response = send_request("GET_USERS");
            cout << "Users: " << response << endl;
        }
        else if (option == 3) {
            string response = send_request("GET_HISTORY|" + current_user);
            cout << response;
        }
        else if (option == 4) {
            break;
        }
    }

    return 0;
}
