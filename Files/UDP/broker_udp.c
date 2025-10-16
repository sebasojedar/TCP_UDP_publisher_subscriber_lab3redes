#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 9000
#define MAXLINE 1024
#define MAXTOPICS 50
#define MAXSUBS 50

typedef struct {
    char topic[64];
    struct sockaddr_in subs[MAXSUBS];
    int sub_count;
} Topic;

Topic topics[MAXTOPICS];
int topic_count = 0;

int find_topic(const char *topic) {
    for (int i = 0; i < topic_count; i++) {
        if (strcmp(topics[i].topic, topic) == 0)
            return i;
    }
    return -1;
}

void add_subscriber(const char *topic, struct sockaddr_in addr) {
    int t = find_topic(topic);
    if (t == -1 && topic_count < MAXTOPICS) {
        strcpy(topics[topic_count].topic, topic);
        topics[topic_count].subs[0] = addr;
        topics[topic_count].sub_count = 1;
        topic_count++;
    } else if (t != -1) {
        for (int i = 0; i < topics[t].sub_count; i++) {
            if (topics[t].subs[i].sin_port == addr.sin_port)
                return;
        }
        topics[t].subs[topics[t].sub_count++] = addr;
    }
}

void send_to_subs(int sockfd, const char *topic, const char *msg) {
    int t = find_topic(topic);
    if (t == -1) return;

    for (int i = 0; i < topics[t].sub_count; i++) {
        sendto(sockfd, msg, strlen(msg), 0,
               (struct sockaddr*)&topics[t].subs[i], sizeof(topics[t].subs[i]));
    }
}

int main() {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    char buffer[MAXLINE];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    printf("[broker] UDP escuchando en puerto %d\n\n", PORT);

    while (1) {
        socklen_t len = sizeof(cliaddr);
        int n = recvfrom(sockfd, buffer, MAXLINE - 1, 0,
                         (struct sockaddr *)&cliaddr, &len);
        if (n < 0) continue;

        buffer[n] = '\0';

        char command[8], topic[64], message[900];
        memset(command, 0, sizeof(command));
        memset(topic, 0, sizeof(topic));
        memset(message, 0, sizeof(message));

        sscanf(buffer, "%s %s %[^\n]", command, topic, message);

        if (strcmp(command, "SUB") == 0) {
            add_subscriber(topic, cliaddr);
            sendto(sockfd, "OK SUB", 6, 0, (struct sockaddr*)&cliaddr, len);
            printf("[broker] SUB de %s:%d a topic '%s'\n",
                   inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), topic);
        } else if (strcmp(command, "PUB") == 0) {
            printf("[broker] PUB topic '%s' reenviado a %d suscriptores\n",
                   topic, topics[find_topic(topic)].sub_count);
            send_to_subs(sockfd, topic, message);
        }
    }
}
