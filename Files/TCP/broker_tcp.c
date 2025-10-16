#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define TOPIC_SIZE 64

typedef struct {
    int socket;
    char topic[TOPIC_SIZE];
} Subscriber;

typedef struct {
    int socket;
    char topic[TOPIC_SIZE];
} Publisher;

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE];

    Subscriber subscribers[MAX_CLIENTS];
    Publisher publishers[MAX_CLIENTS];
    int subscriber_count = 0;
    int publisher_count = 0;

    // Crear socket TCP
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Error al crear socket");
        exit(EXIT_FAILURE);
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Error en bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        perror("Error en listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Broker TCP inicializado.\n");
    printf("Escuchando en el puerto %d...\n", PORT);

    fd_set readfds;
    int client_sockets[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++) client_sockets[i] = 0;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        int max_sd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            if (sd > 0) FD_SET(sd, &readfds);
            if (sd > max_sd) max_sd = sd;
        }

        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) continue;

        // Nueva conexión
        if (FD_ISSET(server_fd, &readfds)) {
            new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
            if (new_socket < 0) {
                perror("Error en accept");
                continue;
            }

            char answer[4];
            int valread = read(new_socket, answer, sizeof(answer) - 1);
            if (valread <= 0) { 
                close(new_socket); 
                continue; 
            }
            answer[valread] = '\0';

            // Si es publicador
            if (strncmp(answer, "1", 1) == 0) {
                send(new_socket, "Registra el nombre del partido: ", 32, 0);
                int n = read(new_socket, buffer, BUFFER_SIZE - 1);
                if (n <= 0) { close(new_socket); continue; }
                buffer[n - 1] = '\0';

                publishers[publisher_count].socket = new_socket;
                strncpy(publishers[publisher_count].topic, buffer, TOPIC_SIZE);
                publisher_count++;

                printf("Nuevo publicador conectado: %s\n", buffer);
                send(new_socket, "Registro exitoso. Puede comenzar a enviar actualizaciones.\n", 61, 0);
            }
            // Si es suscriptor
	    else {
    	    	if (publisher_count == 0) {
		    send(new_socket, "No hay partidos disponibles aún.\n", 33, 0);
                    close(new_socket);
                    continue;
                }

    	    // Construir y enviar lista de partidos en un solo mensaje
    	    char topic_list[BUFFER_SIZE] = "Partidos disponibles:\n";
    	    for (int j = 0; j < publisher_count; j++) {
        	    strcat(topic_list, publishers[j].topic);
        	    strcat(topic_list, "\n");
    	    }
    	    strcat(topic_list, "Ingrese el nombre del partido al que desea suscribirse: ");
    	    send(new_socket, topic_list, strlen(topic_list), 0);

    	    // Leer la respuesta (nombre del partido)
    	    int n = read(new_socket, buffer, BUFFER_SIZE - 1);
    	    if (n <= 0) { 
                close(new_socket); 
        	    continue; 
    	    }
    	    buffer[n - 1] = '\0';  // quitar salto de línea

    	    subscribers[subscriber_count].socket = new_socket;
    	    strncpy(subscribers[subscriber_count].topic, buffer, TOPIC_SIZE);
    	    subscriber_count++;

    	    printf("Nuevo suscriptor conectado al partido: %s\n", buffer);
    	    send(new_socket, 
         	    "Conexión realizada exitosamente. Esperando actualización del partido...\n", 73, 0);
	    }

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    break;
                }
            }
        }

        // Leer mensajes de publicadores
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            if (sd > 0 && FD_ISSET(sd, &readfds)) {
                int valread = read(sd, buffer, BUFFER_SIZE - 1);
                if (valread <= 0) {
                    close(sd);
                    client_sockets[i] = 0;
                    continue;
                }

                buffer[valread] = '\0';
                buffer[strcspn(buffer, "\r\n")] = 0;

                // Buscar tema asociado al publicador
                for (int p = 0; p < publisher_count; p++) {
                    if (publishers[p].socket == sd) {
                        printf("[%s]: %s\n", publishers[p].topic, buffer);

                        // Reenviar a los suscriptores del mismo tema
                        for (int s = 0; s < subscriber_count; s++) {
                            if (strcmp(subscribers[s].topic, publishers[p].topic) == 0) {
                                send(subscribers[s].socket, buffer, strlen(buffer), 0);
                            }
                        }
                    }
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
