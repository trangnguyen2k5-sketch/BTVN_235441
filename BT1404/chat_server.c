#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>
#include <time.h>

#define MAX_CLIENTS 1000

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listener, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(listener, 5) < 0) {
        perror("listen");
        return 1;
    }

    printf("Server listening on port 8080...\n");

    struct pollfd fds[MAX_CLIENTS];
    char *ids[MAX_CLIENTS] = {0};
    fds[0].fd = listener;
    fds[0].events = POLLIN;

    int nfds = 1;
    char buf[512];
    while (1) {
        int ret = poll(fds, nfds, -1);
        if (ret < 0) {
            perror("poll");
            break;
        }

        if (fds[0].revents & POLLIN) {
            int client = accept(listener, NULL, NULL);

            if (nfds < MAX_CLIENTS) {
                fds[nfds].fd = client;
                fds[nfds].events = POLLIN;
                ids[nfds] = NULL;

                nfds++;

                printf("New client: %d\n", client);

                char *msg = "Nhap ID theo cu phap: client_id: client_name\n";
                send(client, msg, strlen(msg), 0);
            } else {
                close(client);
            }
        }

        for (int i = 1; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                int len = recv(fds[i].fd, buf, sizeof(buf)-1, 0);
                if (len <= 0) {
                    printf("Client %d disconnected\n", fds[i].fd);
                    close(fds[i].fd);
                    free(ids[i]);
                    fds[i] = fds[nfds - 1];
                    ids[i] = ids[nfds - 1];
                    nfds--;
                    i--;
                    continue;
                }

                buf[len] = 0;
                if (ids[i] == NULL) {
                    char *p = strchr(buf, ':');
                    if (!p) {
                        char *msg = "Sai cu phap. Dung: client_id: client_name\n";
                        send(fds[i].fd, msg, strlen(msg), 0);
                    } else {
                        *p = 0;
                        char *left = buf;
                        char *right = p + 1;
                        while (*right == ' ') right++;
                        right[strcspn(right, "\r\n")] = 0;
                        if (strcmp(left, "client_id") == 0 || strcmp(left, "client_id:") == 0) {
                            ids[i] = malloc(strlen(right) + 1);
                            strcpy(ids[i], right);
                        } else {
                            ids[i] = malloc(strlen(left) + 1);
                            strcpy(ids[i], left);
                        }
                    char *msg = "OK. Bat dau chat!\n";
                    send(fds[i].fd, msg, strlen(msg), 0);
                    printf("Client %d login as %s\n", fds[i].fd, ids[i]);
                    }
                }

                else {
                    time_t t = time(NULL);
                    struct tm *tm = localtime(&t);

                    char timebuf[64];
                    strftime(timebuf, sizeof(timebuf), "%Y/%m/%d %H:%M:%S", tm);

                    char msg[1024];
                    sprintf(msg, "%s %s: %s", timebuf, ids[i], buf);
                    printf("Server received: %s", msg);
                    for (int j = 1; j < nfds; j++) {
                        if (j != i && ids[j] != NULL) {
                            send(fds[j].fd, msg, strlen(msg), 0);
                        }
                    }
                }
            }
        }
    }
    close(listener);
    return 0;
}