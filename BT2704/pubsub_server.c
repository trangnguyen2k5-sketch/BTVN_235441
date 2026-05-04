#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>

#define MAX_CLIENTS 1000
#define MAX_TOPICS 10
#define BUF_SIZE 512

typedef struct {
    int fd;
    char topics[MAX_TOPICS][32];
    int topic_count;
} Client;

Client clients[MAX_CLIENTS];
int nfds = 0;

int hasTopic(Client *c, char *topic) {
    for (int i = 0; i < c->topic_count; i++) {
        if (strcmp(c->topics[i], topic) == 0)
            return 1;
    }
    return 0;
}

void addTopic(Client *c, char *topic) {
    if (c->topic_count < MAX_TOPICS) {
        strcpy(c->topics[c->topic_count++], topic);
    }
}

void removeTopic(Client *c, char *topic) {
    for (int i = 0; i < c->topic_count; i++) {
        if (strcmp(c->topics[i], topic) == 0) {

            for (int j = i; j < c->topic_count - 1; j++) {
                strcpy(c->topics[j], c->topics[j + 1]);
            }

            c->topic_count--;
            return;
        }
    }
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9000);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(listener, (struct sockaddr*)&addr, sizeof(addr));
    listen(listener, 5);

    printf("PubSub Server running on port 9000...\n");

    struct pollfd fds[MAX_CLIENTS];

    fds[0].fd = listener;
    fds[0].events = POLLIN;
    nfds = 1;

    char buf[BUF_SIZE];

    while (1) {
        int ret = poll(fds, nfds, -1);
        if (ret < 0) {
            perror("poll");
            break;
        }

        //new client
        if (fds[0].revents & POLLIN) {
            int client = accept(listener, NULL, NULL);

            if (nfds < MAX_CLIENTS) {
                fds[nfds].fd = client;
                fds[nfds].events = POLLIN;

                clients[nfds].fd = client;
                clients[nfds].topic_count = 0;

                printf("New client %d\n", client);

                nfds++;
            } else {
                close(client);
            }
        }

        //handel client
        for (int i = 1; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                int len = recv(fds[i].fd, buf, sizeof(buf)-1, 0);

                if (len <= 0) {
                    printf("Client %d disconnected\n", fds[i].fd);
                    close(fds[i].fd);

                    fds[i] = fds[nfds - 1];
                    clients[i] = clients[nfds - 1];

                    nfds--;
                    i--;
                    continue;
                }

                buf[len] = 0;
                printf("Received from client %d: %s", fds[i].fd, buf);

                char cmd[16], topic[32], msg[256];

                //sub
                if (sscanf(buf, "%s %s", cmd, topic) >= 2 &&
                    strcmp(cmd, "SUB") == 0) {

                    if (!hasTopic(&clients[i], topic)) {
                        addTopic(&clients[i], topic);

                        char *ok = "Subscribed!\n";
                        send(fds[i].fd, ok, strlen(ok), 0);
                    }
                }

                //pub
                else if (sscanf(buf, "%s %s %[^\n]", cmd, topic, msg) >= 3 &&
                        strcmp(cmd, "PUB") == 0) {

                    char out[512];
                    snprintf(out, sizeof(out),
                            "[%s] %s\n", topic, msg);

                    //broadcast theo topic
                    for (int j = 1; j < nfds; j++) {
                        if (j != i && hasTopic(&clients[j], topic)) {
                            send(fds[j].fd, out, strlen(out), 0);
                        }
                    }
                }

                //unsub
                else if (sscanf(buf, "%s %s", cmd, topic) >= 2 &&
                    strcmp(cmd, "UNSUB") == 0) {

                if (hasTopic(&clients[i], topic)) {
                    removeTopic(&clients[i], topic);

                    char *ok = "Unsubscribed!\n";
                    send(fds[i].fd, ok, strlen(ok), 0);
                } else {
                    char *err = "Not subscribed\n";
                    send(fds[i].fd, err, strlen(err), 0);
                }
                }

                else {
                    char *err = "Invalid command\n";
                    send(fds[i].fd, err, strlen(err), 0);
                }
            }
        }
    }

    close(listener);
    return 0;
}