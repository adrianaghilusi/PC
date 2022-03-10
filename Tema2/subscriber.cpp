#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"
#include <iostream>

using namespace std;

struct send_msg {
    char subscription_type[20];
    char topic[50];
    int SF; //1 pt store-forward, 0 altfel
};
struct udp_msg {
    char udp_client_ip[15];
    int udp_port;
    char topic[50];
    int type;
    char value[1500];
};

const char *convert(int c) {

    if (c == 0) {
        return "INT";
    }
    if (c == 1) {
        return "SHORT_REAL";
    }
    if (c == 2) {
        return "FLOAT";
    }
    return "STRING";

}

void usage(char *file) {
    fprintf(stderr, "Usage: %s server_address server_port\n", file);
    exit(0);
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    int sockfd, n, ret;
    struct sockaddr_in serv_addr;
    char buffer[BUFLEN];

    if (argc < 4) {
        usage(argv[0]);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "socket");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[3]));
    ret = inet_aton(argv[2], &serv_addr.sin_addr);
    DIE(ret == 0, "inet_aton");

    ret = connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    DIE(ret < 0, "connect");

    //dezactivare Nagle
    int flag = 1;
    ret = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
    DIE(ret < 0, "setsockopt");
    //trimitem id client
    n = send(sockfd, argv[1], strlen(argv[1]), 0);
    DIE(n < 0, "send");

    fd_set read_fds;    // multimea de citire folosita in select()
    fd_set tmp_fds;        // multime folosita temporar
    int fdmax;            // valoare maxima fd din multimea read_fds
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);
    FD_SET(sockfd, &read_fds);
    FD_SET(0, &read_fds);
    fdmax = sockfd;

    char bigBuff[1600];

    while (1) {
        tmp_fds = read_fds;

        ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(ret < 0, "select");

        //putem primi 3 tipuri de mesaje de la tastatura: exit, subscribe, unsubscribe
        //putem primi mesaj si de la clientul UDP
        if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {

            memset(buffer, 0, BUFLEN);
            fgets(buffer, BUFLEN - 1, stdin);


            struct send_msg message;


            if (strncmp(buffer, "exit", 4) == 0) {
                break;
            }
            char *token;

            token = strtok(buffer, " ");
            //subscribe
            if (strncmp(buffer, "subscribe", 9) == 0) {
                memcpy(message.subscription_type, token, strlen(token) + 1);
                token = strtok(NULL, " ");
                memcpy(message.topic, token, strlen(token) + 1);
                token = strtok(NULL, " ");
                message.SF = atoi(token);
                printf("Subscribed to topic.\n");

            } else if (strncmp(buffer, "unsubscribe", 11) == 0) {
                memcpy(message.subscription_type, token, strlen(token) + 1);
                token = strtok(NULL, " ");
                memcpy(message.topic, token, strlen(token) + 1);
                message.SF = -1;
                message.topic[strlen(message.topic) - 1] = '\0';
                printf("Unsubscribed from topic.\n");
            }
            n = send(sockfd, (char *) (&message), sizeof(struct send_msg), 0);
            DIE(n < 0, "send");
        } else if (FD_ISSET(sockfd, &tmp_fds)) {
            //primim de la server
            memset(bigBuff, 0, sizeof(bigBuff));
            n = recv(sockfd, bigBuff, sizeof(bigBuff), 0);
            DIE(n < 0, "recv");


            if (n == 0) {

                break;

            } else {

                udp_msg *udpMsg = (udp_msg *) bigBuff;
                char type[12];
                sprintf(type, "%s", convert(udpMsg->type));


                printf("%s:%d - %s - %s - %s\n", udpMsg->udp_client_ip, udpMsg->udp_port,
                       udpMsg->topic, type, udpMsg->value);


            }
        }


    }

    close(sockfd);

    return 0;

}
