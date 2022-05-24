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
#include "img.h"

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

    struct img img = get_img(argv[1]);
    int xoffset = atoi(argv[2]);
    int yoffset = atoi(argv[3]);

    long num_threads = strtol(argv[6], NULL, 10);

    // printf("%d\n", num_threads);

    struct addrinfo hints, *server;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    getaddrinfo(argv[4], argv[5], &hints, &server);
    


    for (int i = num_threads; i > 0; i--)
    {
        pthread_t some_thread;
        struct arg_struct arg;
        int sockfd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
        connect(sockfd, server->ai_addr, server->ai_addrlen);
        arg.sockfd = sockfd;
        int start = (img.width/num_threads) * (i-1);
        int stop = (img.width/num_threads) * i;
        struct img_string imgstr = img_to_string(img, start, stop, 0, img.height, xoffset, yoffset);
        
        // printf("%d, %d\n", start, stop);

        arg.buf = imgstr.buff;
        arg.buflen = imgstr.len;
        pthread_create(&some_thread, NULL, (void *)&write_to_sock, &arg);
    }

    while (!sig_exit)
    {
    }

}