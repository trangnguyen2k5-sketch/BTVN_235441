/*******************************************************************************
 * @file    telnet_server.c
 * @brief   Server Telnet đa tiến trình (fork) có xác thực và ghi log chi tiết
 * @date    2026-05-11
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <time.h>

#define MAX_BUF 1024

void print_log(char *msg) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    printf("[%02d/%02d %02d:%02d:%02d] %s\n",
        t->tm_mday, t->tm_mon + 1, t->tm_hour, t->tm_min, t->tm_sec, msg);
}

void signal_handler(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int check_login(char *user, char *pass) {
    FILE *f = fopen("user.txt", "r");
    if (f == NULL) {
        perror("Could not open user.txt");
        return 0;
    }
    char f_user[64], f_pass[64];
    while (fscanf(f, "%s %s", f_user, f_pass) != EOF) {
        if (strcmp(user, f_user) == 0 && strcmp(pass, f_pass) == 0) {
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        return 1;
    }

    if (listen(listener, 5)) {
        perror("listen() failed");
        return 1;
    }

    signal(SIGCHLD, signal_handler);
    printf("--- Telnet Server Started on Port 8080 ---\n");

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client = accept(listener, (struct sockaddr *)&client_addr, &client_len);
        
        if (client == -1) continue;

        char log_msg[256];
        sprintf(log_msg, "New connection from %s (socket %d)", 
                inet_ntoa(client_addr.sin_addr), client);
        print_log(log_msg);

        if (fork() == 0) {
            close(listener); 
            char buf[MAX_BUF], user[64], pass[64], tmp[MAX_BUF];

            char *welcome = "Welcome to Remote Shell. Please login.\n";
            send(client, welcome, strlen(welcome), 0);

            while (1) {
                send(client, "User: ", 6, 0);
                int len = recv(client, buf, sizeof(buf) - 1, 0);
                if (len <= 0) exit(0);
                buf[len] = 0;
                sscanf(buf, "%s", user);

                send(client, "Password: ", 10, 0);
                len = recv(client, buf, sizeof(buf) - 1, 0);
                if (len <= 0) exit(0);
                buf[len] = 0;
                sscanf(buf, "%s", pass);

                if (check_login(user, pass)) {
                    sprintf(log_msg, "Client [%s] logged in successfully as: %s", 
                            inet_ntoa(client_addr.sin_addr), user);
                    print_log(log_msg);
                    send(client, "Login successful!\n> ", 19, 0);
                    break;
                } else {
                    sprintf(log_msg, "Failed login attempt from %s (User: %s)", 
                            inet_ntoa(client_addr.sin_addr), user);
                    print_log(log_msg);
                    send(client, "Invalid username or password. Try again.\n", 41, 0);
                }
            }

            while (1) {
                int len = recv(client, buf, sizeof(buf) - 1, 0);
                if (len <= 0) break;
                buf[len] = 0;
                
                strtok(buf, "\r\n");
                if (strlen(buf) == 0) {
                    send(client, "> ", 2, 0);
                    continue;
                }

                if (strcmp(buf, "exit") == 0 || strcmp(buf, "quit") == 0) {
                    break;
                }

                sprintf(log_msg, "[%s] executes: %s", user, buf);
                print_log(log_msg);

                char cmd[MAX_BUF + 64];
                sprintf(cmd, "%s > out_%d.txt 2>&1", buf, getpid());
                system(cmd);

                char filename[32];
                sprintf(filename, "out_%d.txt", getpid());
                FILE *f = fopen(filename, "rb");
                if (f) {
                    while ((len = fread(tmp, 1, sizeof(tmp), f)) > 0) {
                        send(client, tmp, len, 0);
                    }
                    fclose(f);
                } else {
                    char *err_msg = "Command executed, but no output.\n";
                    send(client, err_msg, strlen(err_msg), 0);
                }
                send(client, "\n> ", 3, 0);
            }

            sprintf(log_msg, "User %s (socket %d) disconnected.", user, client);
            print_log(log_msg);
            close(client);
            exit(0);
        }
        
        close(client);
    }

    close(listener);
    return 0;
}