#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>

const char *dir_path = "/home/tungquan/20252_laptringmang/Code_tren_lop/20260519";

int get_file_list(char *rep_buf)
{
    DIR *d;
    struct dirent *dir;
    int file_cnt = 0;
    char file_list[4096] = "";

    d = opendir(dir_path);
    if (d != NULL)
    {
        while ((dir = readdir(d)) != NULL)
        {
            if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0)
            {
                char file_path[2048];
                snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, dir->d_name);
                struct stat path_stat;
                if (stat(file_path, &path_stat) == 0)
                {
                    if (S_ISREG(path_stat.st_mode))
                    {
                        file_cnt++;
                        strcat(file_list, dir->d_name);
                        strcat(file_list, "\r\n");
                    }
                }
            }
        }
        closedir(d);
    }

    if (file_cnt == 0)
    {
        strcpy(rep_buf, "ERROR No files to download\r\n");
    }
    else
    {
        sprintf(rep_buf, "OK %d\r\n%s\r\n", file_cnt, file_list);
    }
    return file_cnt;
}

void client_Handler(int client_fd)
{
    char rep_buf[8192];
    int file_cnt = get_file_list(rep_buf);
    send(client_fd, rep_buf, strlen(rep_buf), 0);

    if (file_cnt == 0)
    {
        close(client_fd);
        return;
    }

    char recv_buf[2048];
    int ret;
    while (1)
    {
        ret = recv(client_fd, recv_buf, sizeof(recv_buf) - 1, 0);
        if (ret <= 0)
        {
            break;
        }
        recv_buf[ret] = 0;
        recv_buf[strcspn(recv_buf, "\r\n")] = 0;

        if (strlen(recv_buf) == 0)
        {
            continue;
        }

        char file_path[2500];
        snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, recv_buf);

        struct stat path_stat;
        if (stat(file_path, &path_stat) == 0 && S_ISREG(path_stat.st_mode))
        {
            long file_size = path_stat.st_size;

            char header[256];
            sprintf(header, "OK %ld\r\n", file_size);
            send(client_fd, header, strlen(header), 0);

            FILE *f = fopen(file_path, "rb");
            if (f != NULL)
            {
                char file_data[2048];
                int bytes_read;
                while ((bytes_read = fread(file_data, 1, sizeof(file_data), f)) > 0)
                {
                    send(client_fd, file_data, bytes_read, 0);
                }
                fclose(f);
            }
            break;
        }
        else
        {
            char *err_msg = "ERROR File not found. Please send another file name:\r\n";
            send(client_fd, err_msg, strlen(err_msg), 0);
        }
    }
    close(client_fd);
}

int main()
{
    signal(SIGCHLD, SIG_IGN);

    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1)
    {
        exit(1);
    }

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);

    if (bind(listener, (const struct sockaddr *)&addr, sizeof(addr)))
    {
        exit(1);
    }

    if (listen(listener, 10) == -1)
    {
        exit(1);
    }

    while (1)
    {
        int client_fd = accept(listener, NULL, NULL);
        if (client_fd == -1)
        {
            continue;
        }

        pid_t pid = fork();

        if (pid == -1)
        {
            close(client_fd);
        }
        else if (pid == 0)
        {
            close(listener);
            client_Handler(client_fd);
            exit(0);
        }
        else
        {
            close(client_fd);
        }
    }
    return 0;
}