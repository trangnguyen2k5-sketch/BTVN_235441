#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define MAX_CLIENTS 100
#define BUF_SIZE 1024

typedef struct {
    int sock;
    int logged_in;
} Client;

Client clients[MAX_CLIENTS];
int nClients = 0;

void removeClient(int i) {
    close(clients[i].sock);
    clients[i] = clients[nClients - 1];
    nClients--;
}

int checkLogin(char *user, char *pass) {
    FILE *f = fopen("users.txt", "r");
    if (!f) return 0;
    char u[100], p[100];
    while (fscanf(f, "%s %s", u, p) != EOF) {
        if (strcmp(u, user) == 0 && strcmp(p, pass) == 0) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

void executeCommand(int sock, char *cmd) {
    char syscmd[256];
    cmd[strcspn(cmd, "\r\n")] = 0;
    snprintf(syscmd, sizeof(syscmd), "%s > out.txt", cmd);
    system(syscmd);
    FILE *f = fopen("out.txt", "r");
    if (!f) {
        send(sock, "Cannot open output file\n", 25, 0);
        return;
    }
    char buf[BUF_SIZE];
    while (fgets(buf, sizeof(buf), f)) {
        send(sock, buf, strlen(buf), 0);
    }
    fclose(f);
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(listener, (struct sockaddr*)&addr, sizeof(addr));
    listen(listener, 5);
    printf("Telnet server running on port 8080...\n");
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
                clients[nClients].logged_in = 0;
                send(client, "Nhap user pass:\n", 18, 0);
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
                    removeClient(i);
                    i--;
                    continue;
                }
                buf[r] = 0;
                if (!clients[i].logged_in) {
                    char user[100], pass[100];
                    if (sscanf(buf, "%s %s", user, pass) == 2) {
                        if (checkLogin(user, pass)) {
                            clients[i].logged_in = 1;
                            send(clients[i].sock, "Login success\n", 14, 0);
                        } else {
                            send(clients[i].sock, "Login failed\n", 13, 0);
                        }
                    } else {
                        send(clients[i].sock, "Nhap dung: user pass\n", 22, 0);
                    }
                }
                else {
                    executeCommand(clients[i].sock, buf);
                }
            }
        }
    }

    close(listener);
    return 0;
}