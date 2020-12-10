#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

volatile uint8_t sig_exit = 0;

void siginthandler(){
    sig_exit = 1;
}

void write_to_sock(int sockfd, unsigned char *img, int width, int height, int channels, int xoffset, int yoffset)
{
    char buffer[256];
    for (int x = 0; x <= width; x++)
    {
        for (int y = 0; y <= height; y++)
        {
            bzero(buffer, 256);
            sprintf(buffer, "PX %d %d %02x%02x%02x\n", x + xoffset, y + yoffset, img[(x + y * width) * channels], img[(x + y * width) * channels + 1], img[(x + y * width) * channels + 2]);
            if (write(sockfd, buffer, strlen(buffer)) < 0)
                printf("ERROR writing to socket\n");
        }
    }
}

int main(int argc, char *argv[])
{
    signal(SIGINT, siginthandler);

    int xoffset = atoi(argv[4]);
    int yoffset = atoi(argv[5]);

    int width, height, channels;
    unsigned char *img = stbi_load(argv[3], &width, &height, &channels, 0);

    struct addrinfo hints, *server;
    int sockfd;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    getaddrinfo(argv[1], argv[2], &hints, &server);

    sockfd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);

    connect(sockfd, server->ai_addr, server->ai_addrlen);
    while(!sig_exit){
        write_to_sock(sockfd, &img[0], width, height, channels, xoffset, yoffset);
    }
    shutdown(sockfd, 2);
    close(sockfd);
}