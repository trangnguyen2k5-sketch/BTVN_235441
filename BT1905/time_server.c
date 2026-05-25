#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define BUF_SIZE 256
#define LOG_FILE "server.log"

typedef struct {
    int socket;
    char ip[INET_ADDRSTRLEN];
    int port;
} ClientInfo;

void *client_handler(void *);
int process_time_command(const char *cmd, char *response);
void write_log(const char *ip, int port, const char *status, const char *content);
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) {
        perror("setsockopt() failed");
        close(listener);
        return 1;
    }
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);
    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        close(listener);
        return 1;
    }

    if (listen(listener, 10)) {
        perror("listen() failed");
        close(listener);
        return 1;
    }
    printf("Time Server is listening on port 8080...\n");
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(listener, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock == -1) {
            perror("accept() failed");
            continue;
        }
        ClientInfo *info = malloc(sizeof(ClientInfo));
        info->socket = client_sock;
        inet_ntop(AF_INET, &client_addr.sin_addr, info->ip, sizeof(info->ip));
        info->port = ntohs(client_addr.sin_port);
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, client_handler, (void *)info) == 0) {
            pthread_detach(thread_id);
            printf("New connection accepted from %s:%d (fd %d)\n", info->ip, info->port, client_sock);
            write_log(info->ip, info->port, "CONNECTED", "Client connected to server");
        } else {
            close(client_sock);
            free(info);
        }
    }

    close(listener);
    return 0;
}

void *client_handler(void *params) {
    ClientInfo *info = (ClientInfo *)params;
    int my_socket = info->socket;
    char buf[BUF_SIZE];
    char response[BUF_SIZE];

    char *welcome = "Kết nối thành công. Hãy gửi lệnh 'GET_TIME [format]'\n";
    send(my_socket, welcome, strlen(welcome), 0);

    while (1) {
        int len = recv(my_socket, buf, sizeof(buf) - 1, 0);
        if (len <= 0) {
            break;
        }
        buf[len] = '\0';
        buf[strcspn(buf, "\r\n")] = 0;
        
        if (strlen(buf) == 0) continue;
        if (process_time_command(buf, response)) {
            send(my_socket, response, strlen(response), 0);
            char log_resp[BUF_SIZE];
            strcpy(log_resp, response);
            log_resp[strcspn(log_resp, "\r\n")] = 0;

            char log_content[BUF_SIZE + 50];
            snprintf(log_content, sizeof(log_content), "Cmd: [%s] -> Resp: [%s]", buf, log_resp);
            write_log(info->ip, info->port, "SUCCESS",  log_content);
        } else {
            char *err_msg = "Lỗi: Sai cú pháp lệnh hoặc định dạng không được hỗ trợ!\n";
            send(my_socket, err_msg, strlen(err_msg), 0);
            
            char log_content[BUF_SIZE + 50];
            snprintf(log_content, sizeof(log_content), "Failed Cmd: [%s]", buf);
            write_log(info->ip, info->port, "ERROR", log_content);
        }
    }

    write_log(info->ip, info->port, "DISCONNECTED", "Client disconnected");
    printf("Client %s:%d trên socket fd %d đã ngắt kết nối.\n", info->ip, info->port, my_socket);
    close(my_socket);
    free(info);
    pthread_exit(NULL);
}

int process_time_command(const char *cmd, char *response) {
    char action[20] = {0};
    char format[50] = {0};
    int parsed = sscanf(cmd, "%19s %49s", action, format);
    if (parsed != 2 || strcmp(action, "GET_TIME") != 0) {
        return 0; 
    }
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    if (strcmp(format, "dd/mm/yyyy") == 0) {
        strftime(response, BUF_SIZE, "%d/%m/%Y\n", timeinfo);
    } 
    else if (strcmp(format, "dd/mm/yy") == 0) {
        strftime(response, BUF_SIZE, "%d/%m/%y\n", timeinfo);
    } 
    else if (strcmp(format, "mm/dd/yyyy") == 0) {
        strftime(response, BUF_SIZE, "%m/%d/%Y\n", timeinfo);
    } 
    else if (strcmp(format, "mm/dd/yy") == 0) {
        strftime(response, BUF_SIZE, "%m/%d/%y\n", timeinfo);
    } 
    else {
        return 0;
    }

    return 1;
}

void write_log(const char *ip, int port, const char *status, const char *content) {
    pthread_mutex_lock(&log_mutex);

    FILE *f = fopen(LOG_FILE, "a");
    if (f != NULL) {
        time_t rawtime;
        struct tm *timeinfo;
        char time_str[30];

        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);

        fprintf(f, "[%s] [%s:%d] [%s] %s\n", time_str, ip, port, status, content);
        fclose(f);
    }

    pthread_mutex_unlock(&log_mutex);
}