#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <arpa/inet.h>

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char *ip = "127.0.0.1";
    int port = 8888;
    //tao socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);
    //connect
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed");
        return 1;
    }
    //lay thu muc hien tai
    char cwd[256];
    getcwd(cwd, sizeof(cwd));
    //gui ten thu muc
    int dir_len = strlen(cwd);
    send(sock, &dir_len, sizeof(int), 0);
    send(sock, cwd, dir_len, 0);
    //mo thu muc
    DIR *d = opendir(".");
    struct dirent *dir;
    int count = 0;
    //dem so file DT_REG = file thuong khong tinh thu muc, file an
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type == DT_REG) count++;
    }
    //quay lai thu muc de doc lai 
    rewinddir(d);
    //gui so file
    send(sock, &count, sizeof(int), 0);
    //gui tung file
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type == DT_REG) {
            //lay thong tin file
            struct stat st;
            stat(dir->d_name, &st);
            //gui ten file
            int name_len = strlen(dir->d_name);
            send(sock, &name_len, sizeof(int), 0);
            send(sock, dir->d_name, name_len, 0);
            //gui kich thuoc file
            long size = st.st_size;
            send(sock, &size, sizeof(long), 0);
        }
    }
    closedir(d);
    close(sock);
    return 0;
}