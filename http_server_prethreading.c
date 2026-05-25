#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

void *worker_thread(void *arg)
{
    int listener = *(int *)arg;
    char buf[2048];

    while (1)
    {
        int client_fd = accept(listener, NULL, NULL);
        if (client_fd < 0)
        {
            perror("accept() failed.\n");
            continue;
        }

        printf("[Worker Thread %lu] New client accepted: %d\n", pthread_self(), client_fd);

        int ret = recv(client_fd, buf, sizeof(buf) - 1, 0);
        if (ret <= 0)
        {
            close(client_fd);
            continue;
        }

        buf[ret] = 0;
        printf("[Worker Thread %lu] Received request:\n%s\n", pthread_self(), buf);

        char *msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\nXin chao cac ban";
        send(client_fd, msg, strlen(msg), 0);

        close(client_fd);
    }
    return NULL;
}

int main()
{
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1)
    {
        perror("socket() failed.\n");
        exit(1);
    }

    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8080);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)))
    {
        perror("bind() failed.\n");
        exit(1);
    }

    if (listen(listener, 10) == -1)
    {
        perror("listen() failed.\n");
        exit(1);
    }

    int num_threads = 8;
    printf("HTTP Server (Prethreading) is listening on port 8080...\n");
    printf("Creating %d worker threads...\n", num_threads);

    pthread_t thread_ids[num_threads];
    for (int i = 0; i < num_threads; i++)
    {
        if (pthread_create(&thread_ids[i], NULL, worker_thread, &listener) != 0)
        {
            perror("pthread_create() failed");
            exit(1);
        }
    }

    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(thread_ids[i], NULL);
    }

    close(listener);
    return 0;
}