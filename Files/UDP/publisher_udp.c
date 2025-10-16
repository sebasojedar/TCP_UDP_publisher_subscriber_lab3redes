#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAXLINE 4096

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Uso: %s <IP> <PUERTO> <TOPIC>\n", argv[0]);
        return 1;
    }

    int sockfd;
    struct sockaddr_in servaddr;
    char buffer[MAXLINE];
    char msg[MAXLINE];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

    printf("[pub] escriba mensajes para topic '%s'. Ctrl+D para salir.\n", argv[3]);

    while (fgets(msg, sizeof(msg), stdin) != NULL) {
        msg[strcspn(msg, "\n")] = '\0';
        snprintf(buffer, sizeof(buffer), "PUB %s %s", argv[3], msg);
        sendto(sockfd, buffer, strlen(buffer), 0,
               (struct sockaddr*)&servaddr, sizeof(servaddr));
    }

    close(sockfd);
    return 0;
}
