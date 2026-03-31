#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <errno.h>

#define MAX_CLIENTS 64

#define WAIT_NAME 0
#define WAIT_MSSV 1

typedef struct {
    int fd;
    int state;
    char name[100];
    char mssv[20];
} Client;

void generate_email(char *name, char *mssv, char *email) {
    char temp[200];
    strcpy(temp, name);
    char *words[20];
    int count = 0;
    char *token = strtok(temp, " ");
    while (token != NULL) {
        words[count++] = token;
        token = strtok(NULL, " ");
    }
    if (count < 2) {
        sprintf(email, "invalid_name@sis.hcmut.edu.vn");
        return;
    }
    char *ten = words[count - 1];
    char initials[50] = "";
    for (int i = 0; i < count - 1; i++) {
        strncat(initials, &words[i][0], 1);
    }
    char *mssv_cut = mssv + 2;

    // Tao email
    sprintf(email, "%s.%s%s@sis.hust.edu.vn",
            ten,
            initials,
            mssv_cut);
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    unsigned long ul = 1;
    ioctl(listener, FIONBIO, &ul);

    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(8080);

    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 5);

    printf("Server listening on port 8080...\n");

    Client clients[MAX_CLIENTS];
    int nclients = 0;

    char buf[256];

    while (1) {
        // Accept client
        int client = accept(listener, NULL, NULL);
        if (client != -1) {
            printf("New client: %d\n", client);

            ioctl(client, FIONBIO, &ul);

            clients[nclients].fd = client;
            clients[nclients].state = WAIT_NAME;
            memset(clients[nclients].name, 0, sizeof(clients[nclients].name));
            memset(clients[nclients].mssv, 0, sizeof(clients[nclients].mssv));

            send(client, "Nhap ho ten: ", 14, 0);

            nclients++;
        }

        // Xu ly client
        for (int i = 0; i < nclients; i++) {
            int len = recv(clients[i].fd, buf, sizeof(buf) - 1, 0);

            if (len <= 0) {
                if (errno == EWOULDBLOCK) continue;
                else continue;
            }

            buf[len] = 0;

            // Xoa newline
            buf[strcspn(buf, "\r\n")] = 0;

            if (clients[i].state == WAIT_NAME) {
                strcpy(clients[i].name, buf);
                send(clients[i].fd, "Nhap MSSV: ", 12, 0);
                clients[i].state = WAIT_MSSV;
            }
            else if (clients[i].state == WAIT_MSSV) {
                strcpy(clients[i].mssv, buf);

                char email[100];
                generate_email(clients[i].name, clients[i].mssv, email);

                char msg[150];
                sprintf(msg, "Email cua ban: %s\n", email);

                send(clients[i].fd, msg, strlen(msg), 0);

                // Reset lai de test tiep
                clients[i].state = WAIT_NAME;
                send(clients[i].fd, "Nhap ho ten: ", 14, 0);
            }
        }
    }

    close(listener);
    return 0;
}