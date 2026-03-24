#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 9000
#define BUFFER_SIZE 1024
#define TARGET "0123456789"
#define TARGET_LEN 10

int count_occurrences(const char *str, const char *target) {
    int count = 0;
    const char *tmp = str;
    while ((tmp = strstr(tmp, target))) {
        count++;
        tmp += 1; 
    }
    return count;
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr)); 
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);
    bind(listener, (struct sockaddr *)&addr, sizeof(addr));
    listen(listener, 5);
    printf("Server streaming dang cho ket noi tai port %d...\n", PORT);
    int client = accept(listener, NULL, NULL);
    char buffer[BUFFER_SIZE];
    char leftover[TARGET_LEN] = {0}; 
    int total_count = 0;
    while (1) {
        int bytes_received = recv(client, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) break;
        buffer[bytes_received] = '\0';
        char combined[BUFFER_SIZE + TARGET_LEN];
        sprintf(combined, "%s%s", leftover, buffer);
        int current_count = count_occurrences(combined, TARGET);
        total_count += current_count;
        printf("Nhan %d bytes. So lan xuat hien moi: %d. Tong cong: %d\n", 
                bytes_received, current_count, total_count);
        int combined_len = strlen(combined);
        if (combined_len >= TARGET_LEN - 1) {
            strcpy(leftover, combined + combined_len - (TARGET_LEN - 1));
        } else {
            strcpy(leftover, combined);
        }
    }
    close(client);
    close(listener);
    return 0;
}
