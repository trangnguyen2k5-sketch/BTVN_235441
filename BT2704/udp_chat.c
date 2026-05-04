#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>

#define BUF_SIZE 512

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <port> <remote_ip> <remote_port>\n", argv[0]);
        return 1;
    }
    int local_port = atoi(argv[1]);
    char *remote_ip = argv[2];
    int remote_port = atoi(argv[3]);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in local = {0};
    local.sin_family = AF_INET;
    local.sin_port = htons(local_port);
    local.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr*)&local, sizeof(local)) < 0) {
        perror("bind");
        return 1;
    }

    struct sockaddr_in remote = {0};
    remote.sin_family = AF_INET;
    remote.sin_port = htons(remote_port);
    remote.sin_addr.s_addr = inet_addr(remote_ip);

    printf("UDP chat ready (local port %d → %s:%d)\n",
            local_port, remote_ip, remote_port);

    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;

    fds[1].fd = sock;
    fds[1].events = POLLIN;

    char buf[BUF_SIZE];

    while (1) {
        int ret = poll(fds, 2, -1);
        if (ret < 0) {
            perror("poll");
            break;
        }

        if (fds[0].revents & POLLIN) {
            if (fgets(buf, sizeof(buf), stdin) == NULL)
                break;

            sendto(sock, buf, strlen(buf), 0,
                    (struct sockaddr*)&remote, sizeof(remote));
        }

        if (fds[1].revents & POLLIN) {
            struct sockaddr_in sender;
            socklen_t len = sizeof(sender);

            int n = recvfrom(sock, buf, sizeof(buf)-1, 0,
                            (struct sockaddr*)&sender, &len);

            if (n > 0) {
                buf[n] = 0;
                printf("Received: %s", buf);
            }
        }
    }

    close(sock);
    return 0;
}