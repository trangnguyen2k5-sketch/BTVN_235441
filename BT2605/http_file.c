#include <arpa/inet.h>
#include <dirent.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 8192
const char *ROOT_DIR = ".";

const char *get_mime_type(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if(!ext) return "application/octet-stream";
    if(strcmp(ext, ".txt") == 0) return "text/plain";
    if(strcmp(ext, ".html") == 0) return "text/html";
    if(strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) 
        return "image/jpeg";
    if(strcmp(ext, ".png") == 0)
        return "image/png";
    if(strcmp(ext, ".gif") == 0)
        return "image/gif";
    if(strcmp(ext, ".mp3") == 0)
        return "audio/mpeg";
    if(strcmp(ext, ".wav") == 0)
        return "audio/wav";
    if(strcmp(ext, ".mp4") == 0)
        return "video/mp4";
    if(strcmp(ext, ".c") == 0)
        return "text/plain";
    return "application/octet-stream";
}

// Gửi file
void send_file(int client, const char *filepath)
{
    FILE *f = fopen(filepath, "rb");
    if(f == NULL) {
        char response[] =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/html\r\n\r\n"
            "<html><body><h1>404 Not Found</h1></body></html>";
        send(client, response, strlen(response), 0);
        return;
    }

    fseek(f, 0, SEEK_END);
    long filesize = ftell(f);
    rewind(f);
    char header[1024];

    sprintf(header,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %ld\r\n"
            "Connection: close\r\n\r\n",
            get_mime_type(filepath),
            filesize);
    send(client, header, strlen(header), 0);

    char buffer[4096];
    size_t n;

    while((n = fread(buffer, 1, sizeof(buffer), f)) > 0) {
        send(client, buffer, n, 0);
    }
    fclose(f);
}

// Liệt kê thư mục
void list_directory(int client, const char *real_path, const char *url_path) {
    DIR *dir = opendir(real_path);
    if(dir == NULL) {
        char response[] =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/html\r\n\r\n"
            "<html><body><h1>Directory Not Found</h1></body></html>";
        send(client, response, strlen(response), 0);
        return;
    }

    char html[65536];
    sprintf(html,
            "<html>"
            "<head><title>Index of %s</title></head>"
            "<body>"
            "<h1>Index of %s</h1>"
            "<ul>",
            url_path,
            url_path);
    struct dirent *entry;

    while((entry = readdir(dir)) != NULL) {
        if(strcmp(entry->d_name, ".") == 0) continue;
        char fullpath[1024];
        if(strcmp(real_path, ".") == 0)
            sprintf(fullpath, "%s", entry->d_name);
        else
            sprintf(fullpath,
                    "%s/%s",
                    real_path,
                    entry->d_name);

        struct stat st;
        stat(fullpath, &st);
        char href[1024];
        if(strcmp(url_path, "/") == 0)
            sprintf(href,
                    "/%s",
                    entry->d_name);
        else
            sprintf(href,
                    "%s/%s",
                    url_path,
                    entry->d_name);

        if(S_ISDIR(st.st_mode)) {
            sprintf(html + strlen(html),
                    "<li><b>"
                    "<a href=\"%s\">%s</a>"
                    "</b></li>",
                    href,
                    entry->d_name);
        }
        else {
            sprintf(html + strlen(html),
                    "<li><i>"
                    "<a href=\"%s\">%s</a>"
                    "</i></li>",
                    href,
                    entry->d_name);
        }
    }

    strcat(html, "</ul></body></html>");
    closedir(dir);
    char header[1024];
    sprintf(header,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=UTF-8\r\n"
            "Content-Length: %ld\r\n"
            "Connection: close\r\n\r\n",
            strlen(html));

    send(client, header, strlen(header), 0);
    send(client, html, strlen(html), 0);
}

// Xử lý request
void handle_request(int client, char *request)
{
    char method[16];
    char path[1024];
    sscanf(request, "%15s %1023s", method, path);
    if(strcmp(method, "GET") != 0) {
        char response[] =
            "HTTP/1.1 405 Method Not Allowed\r\n"
            "Content-Type: text/html\r\n\r\n"
            "<html><body><h1>405 Method Not Allowed</h1></body></html>";
        send(client, response, strlen(response), 0);
        return;
    }
    char real_path[1024];
    if(strcmp(path, "/") == 0) {
        strcpy(real_path, ROOT_DIR);
    }
    else {
        snprintf(real_path, sizeof(real_path), ".%s", path);
    }

    struct stat st;

    if(stat(real_path, &st) < 0)
    {
        char response[] =
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/html\r\n\r\n"
            "<html><body><h1>404 Not Found</h1></body></html>";

        send(client, response, strlen(response), 0);
        return;
    }

    if(S_ISDIR(st.st_mode)) {
        list_directory(client, real_path, path);
    }
    else {
        send_file(client, real_path);
    }
}

int main()
{
    int server_fd =
        socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 10);
    printf("HTTP FILE SERVER RUNNING\n");
    printf("http://localhost:%d\n", PORT);

    while(1) {
        int client =
            accept(server_fd, NULL, NULL);
        char request[BUFFER_SIZE];
        int len = recv(client, request, sizeof(request)-1, 0);
        if(len > 0)
        {
            request[len] = '\0';
            printf("\n=========== REQUEST ===========\n");
            printf("%s\n", request);
            handle_request(client, request);
        }
        close(client);
    }
    close(server_fd);
    return 0;
}