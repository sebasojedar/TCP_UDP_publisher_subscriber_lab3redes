#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    char msg[BUFFER_SIZE];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error al crear socket");
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Dirección inválida");
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error en la conexión");
        return 1;
    }

    printf("Conexión establecida con el broker.\n");
    send(sock, "1\n", 2, 0);

    int n = read(sock, buffer, BUFFER_SIZE - 1);
    if (n > 0) {
        buffer[n] = '\0';
        printf("%s", buffer);
    }

    printf("Ingrese el nombre del partido: ");
    fgets(msg, sizeof(msg), stdin);
    send(sock, msg, strlen(msg), 0);

    n = read(sock, buffer, BUFFER_SIZE - 1);
    if (n > 0) {
        buffer[n] = '\0';
        printf("%s\n", buffer);
    }

    printf("Puede comenzar a enviar mensajes del partido.\n");

    while (1) {
        printf("> ");
        if (!fgets(msg, sizeof(msg), stdin)) break;
        send(sock, msg, strlen(msg), 0);
    }

    close(sock);
    return 0;
}
