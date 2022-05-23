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
};

volatile uint8_t sig_exit = 0;

void siginthandler()
{
    sig_exit = 1;
}

void write_to_sock(struct arg_struct *arg)
{
    while (!sig_exit)
    {
        write(arg->sockfd, arg->buf, arg->buflen);
    }
    shutdown(arg->sockfd, 2);
    close(arg->sockfd);
}

int main(int argc, char *argv[])
{
    signal(SIGINT, siginthandler);

    int xoffset = atoi(argv[2]);
    int yoffset = atoi(argv[3]);

    int width, height, channels;
    unsigned char *img = stbi_load(argv[1], &width, &height, &channels, 0);
    size_t bufflen = 0;
    char *buff = malloc(sizeof(char) * bufflen);

    for (int x = 0; x <= width; x++)
    {
        for (int y = 0; y <= height; y++)
        {
            char temp[25];
            if (img[(x + y * width) * channels] != 0 && img[(x + y * width) * channels + 1] != 0 && img[(x + y * width) * channels + 2] != 0)
            {
                snprintf(temp, 25, "PX %d %d %02x%02x%02x\n", x + xoffset, y + yoffset, img[(x + y * width) * channels], img[(x + y * width) * channels + 1], img[(x + y * width) * channels + 2]);
                int diff = strlen(temp);
                bufflen += diff;
                buff = realloc(buff, bufflen);
                memcpy(&(buff[bufflen - diff]), &temp, diff);
            }
        }
    }

    struct addrinfo hints, *server;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    getaddrinfo(argv[4], argv[5], &hints, &server);

    for (int i = strtol(argv[6], NULL, 10); i > 0; i--)
    {
        pthread_t some_thread;
        struct arg_struct arg;
        int sockfd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
        connect(sockfd, server->ai_addr, server->ai_addrlen);
        arg.sockfd = sockfd;
        arg.buf = buff;
        arg.buflen = bufflen;
        pthread_create(&some_thread, NULL, (void *)&write_to_sock, &arg);
    }

    while (!sig_exit)
    {
    }

}