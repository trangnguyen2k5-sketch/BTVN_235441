#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <poll.h>

#define MAX_CLIENTS 1000
#define BUF_SIZE 512

int checkLogin(char *user, char *pass) {
    FILE *f = fopen("users.txt", "r");
    if (!f) return 0;

    char u[32], p[32];

    while (fscanf(f, "%s %s", u, p) == 2) {
        if (strcmp(u, user) == 0 && strcmp(p, pass) == 0) {
            fclose(f);
            return 1;
        }
    }

    fclose(f);
    return 0;
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

    printf("Telnet server running on port 8080...\n");

    struct pollfd fds[MAX_CLIENTS];
    int logged[MAX_CLIENTS] = {0};

    fds[0].fd = listener;
    fds[0].events = POLLIN;

    int nfds = 1;
    char buf[BUF_SIZE];

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
                logged[nfds] = 0;

                nfds++;

                char *msg = "Nhap user pass:\n";
                send(client, msg, strlen(msg), 0);

                printf("New client %d\n", client);
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

                    fds[i] = fds[nfds - 1];
                    logged[i] = logged[nfds - 1];

                    nfds--;
                    i--;
                    continue;
                }

                buf[len] = 0;
                buf[strcspn(buf, "\r\n")] = 0;

                if (!logged[i]) {
                    char user[32], pass[32];

                    int n = sscanf(buf, "%s %s", user, pass);

                    if (n != 2) {
                        char *msg = "Sai cu phap. Nhap: user pass\n";
                        send(fds[i].fd, msg, strlen(msg), 0);
                        continue;
                    }

                    if (checkLogin(user, pass)) {
                        logged[i] = 1;
                        char *msg = "Dang nhap thanh cong!\n";
                        send(fds[i].fd, msg, strlen(msg), 0);

                        printf("Client %d login: %s\n", fds[i].fd, user);
                    } else {
                        char *msg = "Sai user/pass!\n";
                        send(fds[i].fd, msg, strlen(msg), 0);
                    }
                }
                else {
                    char cmd[512];
                    snprintf(cmd, sizeof(cmd), "%s > out.txt", buf);

                    system(cmd);

                    FILE *f = fopen("out.txt", "rb");
                    if (!f) {
                        char *msg = "Loi mo file!\n";
                        send(fds[i].fd, msg, strlen(msg), 0);
                        continue;
                    }

                    while (1) {
                        int r = fread(buf, 1, sizeof(buf), f);
                        if (r <= 0) break;

                        send(fds[i].fd, buf, r, 0);
                    }

                    fclose(f);
                }
            }
        }
    }

    close(listener);
    return 0;
}