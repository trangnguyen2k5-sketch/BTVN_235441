#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>

#define PORT 8080
#define MAX_BUF 1024

void signal_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void get_formatted_time(char *format, char *output) {
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    if (strcmp(format, "dd/mm/yyyy") == 0) {
        strftime(output, 64, "%d/%m/%Y", timeinfo);
    } else if (strcmp(format, "dd/mm/yy") == 0) {
        strftime(output, 64, "%d/%m/%y", timeinfo);
    } else if (strcmp(format, "mm/dd/yyyy") == 0) {
        strftime(output, 64, "%m/%d/%Y", timeinfo);
    } else if (strcmp(format, "mm/dd/yy") == 0) {
        strftime(output, 64, "%m/%d/%y", timeinfo);
    } else {
        strcpy(output, "INVALID_FORMAT");
    }
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        return 1;
    }

    listen(listener, 10);
    signal(SIGCHLD, signal_handler);

    printf("Time Server is running on port %d...\n", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client = accept(listener, (struct sockaddr *)&client_addr, &client_len);
        
        if (client == -1) continue;

        printf("New connection from %s\n", inet_ntoa(client_addr.sin_addr));

        if (fork() == 0) {
            close(listener);
            char buf[MAX_BUF];

            while (1) {
                int ret = recv(client, buf, sizeof(buf) - 1, 0);
                if (ret <= 0) break;
                
                buf[ret] = 0;
                strtok(buf, "\r\n");

                printf("Received command: %s\n", buf);

                char cmd[32], fmt[64];
                int num = sscanf(buf, "%s %s", cmd, fmt);

                if (num == 2 && strcmp(cmd, "GET_TIME") == 0) {
                    char time_out[64];
                    get_formatted_time(fmt, time_out);
                    
                    if (strcmp(time_out, "INVALID_FORMAT") == 0) {
                        char *err = "Error: Unsupported format.\n";
                        send(client, err, strlen(err), 0);
                    } else {
                        strcat(time_out, "\n");
                        send(client, time_out, strlen(time_out), 0);
                    }
                } else {
                    char *err = "Error: Invalid command syntax. Use GET_TIME [format]\n";
                    send(client, err, strlen(err), 0);
                }
            }

            close(client);
            exit(0);
        }
        close(client);
    }

    return 0;
}