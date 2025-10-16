#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAXLINE 1024

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Uso: %s <IP> <PUERTO> <TOPIC>\n", argv[0]);
        return 1;
    }

    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    char buffer[MAXLINE];
    socklen_t len;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

    char sub_msg[128];
    snprintf(sub_msg, sizeof(sub_msg), "SUB %s", argv[3]);
    sendto(sockfd, sub_msg, strlen(sub_msg), 0,
           (struct sockaddr*)&servaddr, sizeof(servaddr));

    printf("[sub] suscrito a '%s'. Esperando mensajes...\n", argv[3]);

    while (1) {
        int n = recvfrom(sockfd, buffer, MAXLINE - 1, 0,
                         (struct sockaddr*)&cliaddr, &len);
        buffer[n] = '\0';
        printf("[sub] %s: %s\n", argv[3], buffer);
    }

    close(sockfd);
    return 0;
}
