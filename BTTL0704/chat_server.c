#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/select.h>
#include <time.h>

#define MAX_CLIENTS 100
#define BUF_SIZE 512

typedef struct {
    int sock;
    char id[32];
    char name[32];
    int registered;
} Client;

Client clients[MAX_CLIENTS];
int nClients = 0;

void removeClient(int i) {
    close(clients[i].sock);
    clients[i] = clients[nClients - 1];
    nClients--;
}

int parseClientInfo(char *buf, char *id, char *name) {
    char *p = strchr(buf, ':');
    if (!p) return 0;
    *p = 0;
    strcpy(id, buf);
    strcpy(name, p + 2);
    name[strcspn(name, "\r\n")] = 0;
    return 1;
}

void getTimeStr(char *out) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(out, 64, "%Y/%m/%d %H:%M:%S", t);
}

void broadcast(int sender, char *msg) {
    char buffer[BUF_SIZE];
    char timeStr[64];
    getTimeStr(timeStr);
    snprintf(buffer, sizeof(buffer),
            "%s %s: %s",
            timeStr,
            clients[sender].id,
            msg);
    for (int i = 0; i < nClients; i++) {
        if (i != sender && clients[i].registered) {
            send(clients[i].sock, buffer, strlen(buffer), 0);
        }
    }
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(listener, (struct sockaddr*)&addr, sizeof(addr));
    listen(listener, 5);
    printf("Server running on port 8080...\n");
    fd_set fdread;
    while (1) {
        FD_ZERO(&fdread);
        FD_SET(listener, &fdread);
        int maxfd = listener;
        for (int i = 0; i < nClients; i++) {
            FD_SET(clients[i].sock, &fdread);
            if (clients[i].sock > maxfd)
                maxfd = clients[i].sock;
        }
        int ret = select(maxfd + 1, &fdread, NULL, NULL, NULL);
        if (ret < 0) {
            perror("select error");
            break;
        }
        if (FD_ISSET(listener, &fdread)) {
            int client = accept(listener, NULL, NULL);
            if (nClients < MAX_CLIENTS) {
                clients[nClients].sock = client;
                clients[nClients].registered = 0;
                send(client, "Nhap: client_id: client_name\n", 33, 0);
                printf("New client connected: %d\n", client);
                nClients++;
            } else {
                close(client);
            }
        }
        for (int i = 0; i < nClients; i++) {
            if (FD_ISSET(clients[i].sock, &fdread)) {
                char buf[BUF_SIZE];
                int r = recv(clients[i].sock, buf, sizeof(buf) - 1, 0);
                if (r <= 0) {
                    printf("Client %d disconnected\n", clients[i].sock);
                    removeClient(i);
                    i--;
                    continue;
                }
                buf[r] = 0;
                if (!clients[i].registered) {
                    char id[32], name[32];
                    if (parseClientInfo(buf, id, name)) {
                        strcpy(clients[i].id, id);
                        strcpy(clients[i].name, name);
                        clients[i].registered = 1;
                        send(clients[i].sock, "Dang ky thanh cong!\n", 23, 0);
                        printf("Registered: %s (%s)\n", id, name);
                    } else {
                        send(clients[i].sock, "Sai format. Nhap lai!\n", 26, 0);
                    }
                }
                else {
                    printf("Msg from %s: %s", clients[i].id, buf);
                    broadcast(i, buf);
                }
            }
        }
    }

    close(listener);
    return 0;
}