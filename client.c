#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>  // for open
#include <unistd.h> // for close
#include <pthread.h>
#include <netdb.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

struct arg_struct
{
    int sockfd;
    char *buf;
    int buflen;
    struct addrinfo *server;
};

volatile uint8_t sig_exit = 0;
struct addrinfo *server;
struct addrinfo hints;

void siginthandler(int signum)
{
    sig_exit = 1;
}

int reconnect(struct arg_struct *arg)
{
    close(arg->sockfd);
    int sockfd = socket(arg->server->ai_family, arg->server->ai_socktype, arg->server->ai_protocol);
    if (sockfd == -1)
    {
        perror("socket");
        return -1;
    }

    if (connect(sockfd, arg->server->ai_addr, arg->server->ai_addrlen) == -1)
    {
        perror("connect");
        close(sockfd);
        return -1;
    }

    arg->sockfd = sockfd;
    return 0;
}

void *write_to_sock(void *args)
{
    struct arg_struct *arg = (struct arg_struct *)args;
    while (!sig_exit)
    {
        ssize_t written = write(arg->sockfd, arg->buf, arg->buflen);
        if (written == -1)
        {
            // perror("write");
            if (reconnect(arg) == -1)
            {
                break;
            }
        }
    }
    shutdown(arg->sockfd, SHUT_RDWR);
    close(arg->sockfd);
    free(arg);
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 7)
    {
        fprintf(stderr, "Usage: %s <image_path> <x_offset> <y_offset> <hostname> <port> <num_threads>\n", argv[0]);
        return EXIT_FAILURE;
    }

    signal(SIGINT, siginthandler);
    signal(SIGPIPE, SIG_IGN);

    int xoffset = atoi(argv[2]);
    int yoffset = atoi(argv[3]);

    int width, height, channels;
    unsigned char *img = stbi_load(argv[1], &width, &height, &channels, 0);
    if (!img)
    {
        fprintf(stderr, "Error loading image\n");
        return EXIT_FAILURE;
    }

    size_t bufflen = 0;
    size_t buffcap = 1024;
    char *buff = malloc(buffcap);
    if (!buff)
    {
        fprintf(stderr, "Memory allocation error\n");
        stbi_image_free(img);
        return EXIT_FAILURE;
    }

    for (int x = 0; x < width; x++)
    {
        for (int y = 0; y < height; y++)
        {
            if (img[(x + y * width) * channels] != 0 ||
                img[(x + y * width) * channels + 1] != 0 ||
                img[(x + y * width) * channels + 2] != 0)
            {
                char temp[25];
                snprintf(temp, 25, "PX %d %d %02x%02x%02x\n", x + xoffset, y + yoffset,
                         img[(x + y * width) * channels],
                         img[(x + y * width) * channels + 1],
                         img[(x + y * width) * channels + 2]);

                size_t diff = strlen(temp);
                if (bufflen + diff > buffcap)
                {
                    buffcap *= 2;
                    char *new_buff = realloc(buff, buffcap);
                    if (!new_buff)
                    {
                        free(buff);
                        stbi_image_free(img);
                        return EXIT_FAILURE;
                    }
                    buff = new_buff;
                }
                memcpy(&(buff[bufflen]), temp, diff);
                bufflen += diff;
            }
        }
    }
    stbi_image_free(img);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(argv[4], argv[5], &hints, &server) != 0)
    {
        fprintf(stderr, "getaddrinfo error\n");
        free(buff);
        return EXIT_FAILURE;
    }

    int num_threads = atoi(argv[6]);
    pthread_t threads[num_threads];

    for (int i = 0; i < num_threads; i++)
    {
        int sockfd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
        if (sockfd == -1)
        {
            perror("socket");
            free(buff);
            freeaddrinfo(server);
            return EXIT_FAILURE;
        }

        if (connect(sockfd, server->ai_addr, server->ai_addrlen) == -1)
        {
            perror("connect");
            close(sockfd);
            free(buff);
            freeaddrinfo(server);
            return EXIT_FAILURE;
        }

        struct arg_struct *arg = malloc(sizeof(struct arg_struct));
        if (!arg)
        {
            fprintf(stderr, "Memory allocation error\n");
            close(sockfd);
            free(buff);
            freeaddrinfo(server);
            return EXIT_FAILURE;
        }
        arg->sockfd = sockfd;
        arg->buf = buff;
        arg->buflen = bufflen;
        arg->server = server;

        if (pthread_create(&threads[i], NULL, write_to_sock, arg) != 0)
        {
            perror("pthread_create");
            close(sockfd);
            free(arg);
            free(buff);
            freeaddrinfo(server);
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    free(buff);
    freeaddrinfo(server);

    return EXIT_SUCCESS;
}