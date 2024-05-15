#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef int socklen_t;
  #define close closesocket
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <fcntl.h>  // for open
  #include <unistd.h> // for close
  #include <netdb.h>
#endif
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

struct arg_struct
{
    int sockfd;
    char *buf;
    int buflen;
};

volatile uint8_t sig_exit = 0;

void siginthandler(int signum)
{
    sig_exit = 1;
}

void write_to_sock(struct arg_struct *arg)
{
    write(arg->sockfd, arg->buf, arg->buflen);
    shutdown(arg->sockfd, 2);
    close(arg->sockfd);
}

int main(int argc, char *argv[])
{
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2),&wsa);
#else
    signal(SIGINT, siginthandler);
#endif

    int xoffset = atoi(argv[2]);
    int yoffset = atoi(argv[3]);

    int width, height, channels;
    unsigned char *img = stbi_load(argv[1], &width, &height, &channels, 0);
    size_t bufflen = width * height * 25;
    char *buff = malloc(sizeof(char) * bufflen);
    buff[0] = '\0';

    for (int x = 0; x <= width; x++)
    {
        for (int y = 0; y <= height; y++)
        {
            char temp[25];
            if (img[(x + y * width) * channels] != 0 && img[(x + y * width) * channels + 1] != 0 && img[(x + y * width) * channels + 2] != 0)
            {
                snprintf(temp, 25, "PX %d %d %02x%02x%02x\n", x + xoffset, y + yoffset, img[(x + y * width) * channels], img[(x + y * width) * channels + 1], img[(x + y * width) * channels + 2]);
                strcat(buff, temp);
            }
        }
    }

    struct addrinfo hints, *server;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    getaddrinfo(argv[4], argv[5], &hints, &server);

    pthread_t *threads = malloc(sizeof(pthread_t) * strtol(argv[6], NULL, 10));

    for (int i = 0; i < strtol(argv[6], NULL, 10); i++)
    {
        struct arg_struct arg;
        int sockfd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
        connect(sockfd, server->ai_addr, server->ai_addrlen);
        arg.sockfd = sockfd;
        arg.buf = buff;
        arg.buflen = strlen(buff);
        pthread_create(&threads[i], NULL, (void *)&write_to_sock, &arg);
    }

    for (int i = 0; i < strtol(argv[6], NULL, 10); i++)
    {
        pthread_join(threads[i], NULL);
    }

    freeaddrinfo(server);
    free(buff);
    free(threads);

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}
