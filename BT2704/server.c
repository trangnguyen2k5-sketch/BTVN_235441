#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <time.h>

#define MAX_CLIENTS 100
#define BUF_SIZE 1024

int clients[MAX_CLIENTS];
int nClients = 0;

void nowStr(char *out, int sz) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(out, sz, "%Y-%m-%d %H:%M:%S", tm);
}

void removeClient(int i) {
    char t[64]; nowStr(t, sizeof(t));
    printf("[%s] Client %d disconnected\n", t, clients[i]);

    close(clients[i]);
    clients[i] = clients[nClients - 1];
    nClients--;
}

void sendClientCount(int sock) {
    char msg[128];
    sprintf(msg, "Xin chao. Hien co %d clients dang ket noi.\n", nClients);
    send(sock, msg, strlen(msg), 0);
}

void broadcastClientCount() {
    for (int i = 0; i < nClients; i++) {
        sendClientCount(clients[i]);
    }
}

void encode(char *str) {
    for (int i = 0; str[i]; i++) {
        if (str[i] >= 'A' && str[i] <= 'Z') {
            str[i] = (str[i] == 'Z') ? 'A' : str[i] + 1;
        }
        else if (str[i] >= 'a' && str[i] <= 'z') {
            str[i] = (str[i] == 'z') ? 'a' : str[i] + 1;
        }
        else if (str[i] >= '0' && str[i] <= '9') {
            str[i] = 9 - (str[i] - '0') + '0';
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
    printf("Server running on port 8080\n");
    fd_set fdread;
    while (1) {
        FD_ZERO(&fdread);
        FD_SET(listener, &fdread);
        int maxfd = listener;
        for (int i = 0; i < nClients; i++) {
            FD_SET(clients[i], &fdread);
            if (clients[i] > maxfd)
                maxfd = clients[i];
        }

        int ret = select(maxfd + 1, &fdread, NULL, NULL, NULL);
        if (ret < 0) {
            perror("select error");
            break;
        }

        if (FD_ISSET(listener, &fdread)) {
            int client = accept(listener, NULL, NULL);

            if (nClients < MAX_CLIENTS) {
                clients[nClients++] = client;

                char t[64]; nowStr(t, sizeof(t));
                printf("[%s] New client connected: %d | Total: %d\n",
                        t, client, nClients);

                sendClientCount(client);
                broadcastClientCount();
            } else {
                close(client);
            }
        }

        for (int i = 0; i < nClients; i++) {
            if (FD_ISSET(clients[i], &fdread)) {
                char buf[BUF_SIZE];
                int r = recv(clients[i], buf, sizeof(buf) - 1, 0);

                if (r <= 0) {
                    removeClient(i);
                    i--;
                    broadcastClientCount();
                    continue;
                }

                buf[r] = 0;
                buf[strcspn(buf, "\r\n")] = 0;

                char t[64]; nowStr(t, sizeof(t));
                printf("[%s] Received from %d: %s\n", t, clients[i], buf);

                if (strcmp(buf, "exit") == 0) {
                    send(clients[i], "Tam biet\n", 9, 0);

                    printf("[%s] Client %d sent exit\n", t, clients[i]);

                    removeClient(i);
                    i--;
                    broadcastClientCount();
                    continue;
                }

                char original[BUF_SIZE];
                strcpy(original, buf);

                encode(buf);
                strcat(buf, "\n");

                send(clients[i], buf, strlen(buf), 0);

                printf("[%s] Encoded: \"%s\" -> \"%s\"\n",
                        t, original, buf);
            }
        }
    }

    close(listener);
    return 0;
}