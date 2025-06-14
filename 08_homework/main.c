#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef char* string;

string get_path_from_config_file(FILE* file, const char* name){
    if (file == NULL) {
        return NULL;
    }

    char buf[256];
    size_t name_len = strlen(name);

    rewind(file);

    while (fgets(buf, sizeof(buf), file)) {
        if (strncmp(buf, name, name_len) == 0) {
            char* newline = strchr(buf + name_len, '\n');
            if (newline) {
                *newline = '\0';
            }
            string path = malloc(strlen(buf + name_len) + 1);
            if (path == NULL) {
                return NULL;
            }
            strcpy(path, buf + name_len);
            return path;
        }
    }

    return NULL;
}

void set_as_daemon(void){
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    if (setsid() < 0) {
        perror("setsid");
        exit(EXIT_FAILURE);
    }

    pid = fork();

    if (pid < 0){
        exit(EXIT_FAILURE);
    }
    if (pid > 0){
        exit(EXIT_SUCCESS);
    }

    chdir("/");
    umask(0);

    int fd = open("/dev/null", O_RDWR);

    if (fd != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > 2) close(fd);
    }
}

int main(int argc, char *argv[]) {
    int run_as_daemon = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--daemon") == 0) {
            run_as_daemon = 1;
            break;
        }
    }

    FILE *f = fopen("config.txt", "r");
    if (f == NULL) {
        perror("fopen");
        return 1;
    }

    string file_path = get_path_from_config_file(f, "file_path=");
    if (file_path == NULL) {
        fprintf(stderr, "file_path not found in config\n");
        fclose(f);
        return 1;
    }

    string socket_path = get_path_from_config_file(f, "socket_path=");
    if (socket_path == NULL) {
        fprintf(stderr, "socket_path not found in config\n");
        free(file_path);
        fclose(f);
        return 1;
    }

    printf("file_path: %s\n", file_path);
    printf("socket_path: %s\n", socket_path);

    if (run_as_daemon == 1){
        set_as_daemon();
    }
    
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    unlink(socket_path); 
    
    if(bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1){
        perror("bind");
        exit(EXIT_FAILURE);
    }
    if(listen(sockfd, 5) == -1){
        perror("listen");
        exit(EXIT_FAILURE);
    }
    while (1) {
        int client_fd = accept(sockfd, NULL, NULL);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }
        struct stat st;
        if (stat(file_path, &st) == -1) {
            perror("stat");
            dprintf(client_fd, "Error\n");
        } else {
            dprintf(client_fd, "%lld\n", (long long)st.st_size);
        }
        close(client_fd);
    }
    close(sockfd);
    free(file_path);
    free(socket_path);
    fclose(f);
    return 0;
}
