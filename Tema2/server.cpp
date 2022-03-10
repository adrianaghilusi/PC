#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helpers.h"
#include <netinet/tcp.h>
#include <iostream>
#include <math.h>

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
struct client {
    char clientId[15];
    int personalSocket;
    int isConnected; //0-nu, 1 -da

};
struct topic {
    char name[50];
    struct client subscribers[10];
    int nrSubs;
};
struct storedMsg {
    char clientId[15];
    char topic[50];
    int socket;
    struct udp_msg history[5];
    int nr;
};


void usage(char *file) {
    fprintf(stderr, "Usage: %s server_port\n", file);
    exit(0);
}

int topicExist(topic topicList[], int nrTopics, char find[]) {
    for (int i = 0; i < nrTopics; i++) {
        if (strcmp(find, topicList[i].name) == 0) {
            return 1;
        }
    }
    return 0;

}


int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    //retinem date
    struct client clientList[10];
    int clientsNr = 0;
    struct topic topicsList[20];
    int topicsNr = 0;
    struct storedMsg storedMsgList[10];
    int storedNr = 0;

    int sockfd_tcp, newsockfd, portno, sockfd_udp;
    char buffer[BUFLEN];
    struct sockaddr_in serv_addr, cli_addr;
    int n, i, ret;
    socklen_t clilen;

    fd_set read_fds;    // multimea de citire folosita in select()
    fd_set tmp_fds;        // multime folosita temporar
    int fdmax;            // valoare maxima fd din multimea read_fds

    if (argc < 2) {
        usage(argv[0]);
    }
    //portul utilizat
    portno = atoi(argv[1]);
    DIE(portno == 0, "atoi");

    // se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

    //socket_tcp
    sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd_tcp < 0, "socket");
    //bind si listen tcp
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    ret = bind(sockfd_tcp, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));
    DIE(ret < 0, "bind");

    ret = listen(sockfd_tcp, MAX_CLIENTS);
    DIE(ret < 0, "listen");

    //socket udp
    sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(sockfd_udp < 0, "socket");
    //bind udp, udp nu asculta
    memset((char *) &cli_addr, 0, sizeof(cli_addr));
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_port = htons(portno);
    cli_addr.sin_addr.s_addr = INADDR_ANY;

    ret = bind(sockfd_udp, (struct sockaddr *) &cli_addr, sizeof(struct sockaddr_in));
    DIE(ret < 0, "bind");

    // se adauga noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
    FD_SET(sockfd_tcp, &read_fds);
    FD_SET(sockfd_udp, &read_fds);
    FD_SET(STDIN_FILENO, &read_fds);
    char bigBuff[1600];

    fdmax = (sockfd_udp > sockfd_tcp ? sockfd_udp : sockfd_tcp);
    int goodbye = 0;
    int af = 0;
    while (1) {
        tmp_fds = read_fds;

        ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(ret < 0, "select");


        for (i = 0; i <= fdmax; i++) {

            if (FD_ISSET(i, &tmp_fds)) {
                if (i == 0) {
                    memset(buffer, 0, BUFLEN);
                    fgets(buffer, BUFLEN - 1, stdin);

                    if (strncmp(buffer, "exit", 4) == 0) {
                        goodbye = 1;
                        break;
                    }
                }
                if (i == sockfd_udp) {
                    memset(bigBuff, 0, sizeof(bigBuff));
                    clilen = sizeof(cli_addr);
                    n = recvfrom(sockfd_udp, bigBuff, 1600, 0, (sockaddr *) &cli_addr, &clilen);
                    DIE(n < 0, "recv");
                    struct udp_msg udpMsg;
                    memset(&udpMsg, 0, sizeof(udpMsg));
                    int type;
                    sprintf(udpMsg.udp_client_ip, "%s", inet_ntoa(cli_addr.sin_addr));
                    udpMsg.udp_port = ntohs(cli_addr.sin_port);
                    strncpy(udpMsg.topic, bigBuff, 50);
                    type = (int8_t) bigBuff[50];
                    udpMsg.type = type;
                    //realizam conversia in functie de tipul primit
                    if (type == 3) {
                        //string
                        strcpy(udpMsg.value, bigBuff + 51);
                    }
                    if (type == 0) {
                        //int
                        int nr = ntohl(*(uint32_t * )(bigBuff + 52));
                        if (bigBuff[51] == 1) {
                            nr *= -1;
                        }

                        sprintf(udpMsg.value, "%d", nr);
                    }
                    if (type == 1) {
                        //short real
                        float nr = ntohs(*(uint16_t * )(bigBuff + 51)) / 100.0;
                        sprintf(udpMsg.value, "%.2f", nr);

                    }
                    if (type == 2) {
                        //float
                        double nr = ntohl(*(uint32_t * )(bigBuff + 52)) / pow(10, bigBuff[56]);
                        if (bigBuff[51] == 1) {
                            nr *= -1;
                        }
                        sprintf(udpMsg.value, "%lf", nr);
                    }


                    struct topic newTopic;
                    memset(&newTopic, 0, sizeof(struct topic));
                    //verificam ca topicul sa nu existe deja
                    if (topicsNr == 0 || topicExist(topicsList, topicsNr, udpMsg.topic) == 0) {
                        memcpy(newTopic.name, udpMsg.topic, strlen(udpMsg.topic) + 1);
                        newTopic.nrSubs = 0;
                        topicsList[topicsNr] = newTopic;
                        topicsNr++;
                    } else {
                        //trimitem update abonatilor topicului
                        for (int p = 0; p < topicsNr; p++) {
                            if (strcmp(topicsList[p].name, udpMsg.topic) == 0) {
                                for (int j = 0; j < topicsList[p].nrSubs; j++) {
                                    if (topicsList[p].subscribers[j].isConnected) {
                                        int s = send(topicsList[p].subscribers[j].personalSocket, (char *) (&udpMsg),
                                                     sizeof(struct udp_msg), 0);
                                        DIE(s < 0, "send");

                                    } else {
                                        //pastram mesajul clientilor abonati cu SF=1
                                        for (int l = 0; l < storedNr; l++) {
                                            if (strcmp(storedMsgList[l].topic, udpMsg.topic) == 0 &&
                                                strcmp(topicsList[p].subscribers[j].clientId,
                                                       storedMsgList[l].clientId) == 0) {
                                                storedMsgList[l].history[storedMsgList[l].nr] = udpMsg;
                                                storedMsgList[l].nr++;
                                            }
                                        }
                                    }

                                }
                            }

                        }


                    }


                } else if (i == sockfd_tcp) {

                    clilen = sizeof(cli_addr);
                    newsockfd = accept(sockfd_tcp, (struct sockaddr *) &cli_addr, &clilen);
                    DIE(newsockfd < 0, "accept");
                    //dezactivare Nagle
                    int flag = 1;
                    int ret = setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
                    DIE(ret < 0, "setsockopt");

                    // se adauga noul socket intors de accept() la multimea descriptorilor de citire
                    FD_SET(newsockfd, &read_fds);
                    if (newsockfd > fdmax) {
                        fdmax = newsockfd;
                    }
                    memset(buffer, 0, BUFLEN);
                    n = recv(newsockfd, buffer, BUFLEN, 0);
                    DIE(n < 0, "recv");
                    int exists = 0;
                    int doNotConnect = 0;
                    //verificam daca s-a conectat un nou client sau este vorba despre o reconectare
                    for (int p = 0; p < clientsNr; p++) {
                        if (strcmp(buffer, clientList[p].clientId) == 0) {
                            exists = 1;
                            if (clientList[p].isConnected == 1) {
                                printf("Client %s already connected.\n", buffer);
                                close(newsockfd);
                                // se scoate din multimea de citire socketul inchis
                                FD_CLR(newsockfd, &read_fds);

                                doNotConnect = 1;
                                break;
                            } else {
                                //reconectam clientul si verificam sa primeasca mesajele udp

                                clientList[p].isConnected = 1;
                                clientList[p].personalSocket = newsockfd;
                                for (int t = 0; t < topicsNr; t++) {
                                    for (int s = 0; s < topicsList[t].nrSubs; s++) {
                                        if (strcmp(clientList[p].clientId, topicsList[t].subscribers[s].clientId) ==
                                            0) {
                                            topicsList[t].subscribers[s].isConnected = 1;
                                            topicsList[t].subscribers[s].personalSocket = newsockfd;

                                        }
                                    }
                                }
                                for (int t = 0; t < storedNr; t++) {
                                    if (strcmp(clientList[p].clientId, storedMsgList[t].clientId) == 0) {
                                        for (int v = 0; v < storedMsgList[t].nr; v++) {
                                            int s = send(newsockfd, (char *) (&storedMsgList[t].history[v]),
                                                         sizeof(struct udp_msg), 0);
                                            DIE(s < 0, "send");
                                        }
                                        storedMsgList[t].nr = 0;
                                        memset(storedMsgList[t].history, 0, sizeof(storedMsgList[t].history));

                                    }

                                }
                            }
                        }
                    }
                    if (doNotConnect == 0) {
                        if (!exists) {
                            //cream un nou client
                            printf("New client %s connected from %s:%d.\n",
                                   buffer, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
                            struct client newClient;
                            memcpy(newClient.clientId, buffer, strlen(buffer) + 1);
                            newClient.isConnected = 1;
                            newClient.personalSocket = newsockfd;
                            clientList[clientsNr] = newClient;

                            clientsNr++;
                        } else {
                            printf("New client %s connected from %s:%d.\n",
                                   buffer, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
                        }
                    }


                } else {

                    struct send_msg msg;
                    n = recv(i, (char *) (&msg), BUFLEN, 0);
                    DIE(n < 0, "recv");

                    if (n == 0) {
                        // conexiunea s-a inchis
                        for (int p = 0; p < clientsNr; p++) {
                            if (clientList[p].personalSocket == i && clientList[p].isConnected == 1) {
                                printf("Client %s disconnected.\n", clientList[p].clientId);
                                clientList[p].isConnected = 0;
                                for (int t = 0; t < topicsNr; t++) {
                                    for (int s = 0; s < topicsList[t].nrSubs; s++) {
                                        if (strcmp(clientList[p].clientId, topicsList[t].subscribers[s].clientId) ==
                                            0) {
                                            topicsList[t].subscribers[s].isConnected = 0;
                                        }
                                    }
                                }
                                break;


                            }
                        }
                        close(i);
                        // se scoate din multimea de citire socketul inchis
                        FD_CLR(i, &read_fds);


                    } else {

                        //gasim clientul care doreste sa dea subscribe sau unsubscribe
                        struct client whichClient;

                        for (int p = 0; p < clientsNr; p++) {
                            if (clientList[p].personalSocket == newsockfd) {
                                whichClient = clientList[p];
                                break;
                            }
                        }

                        if (strncmp(msg.subscription_type, "subscribe", 9) == 0) {

                            struct topic newTopic;
                            //verificam ca topicul sa nu existe deja
                            if (topicsNr == 0 || topicExist(topicsList, topicsNr, msg.topic) == 0) {
                                memcpy(newTopic.name, msg.topic, strlen(msg.topic) + 1);
                                newTopic.nrSubs = 0;
                                newTopic.subscribers[newTopic.nrSubs] = whichClient;
                                newTopic.nrSubs++;
                                topicsList[topicsNr] = newTopic;
                                topicsNr++;
                            } else {
                                for (int j = 0; j < topicsNr; j++) {
                                    if (strcmp(topicsList[j].name, msg.topic) == 0) {
                                        topicsList[j].subscribers[topicsList[j].nrSubs] = whichClient;
                                        topicsList[j].nrSubs++;
                                    }
                                }

                            }
                            if (msg.SF == 1) {
                                struct storedMsg newMsg;
                                memcpy(newMsg.clientId, whichClient.clientId, strlen(whichClient.clientId) + 1);
                                memcpy(newMsg.topic, msg.topic, strlen(newMsg.topic) + 1);
                                newMsg.socket = whichClient.personalSocket;
                                storedMsgList[storedNr] = newMsg;
                                storedNr++;

                            }


                        }
                        if (strncmp(msg.subscription_type, "unsubscribe", 11) == 0) {
                            for (int p = 0; p < topicsNr; p++) {
                                if (strcmp(msg.topic, topicsList[p].name) == 0) {
                                    for (int k = 0; k < topicsList[p].nrSubs; k++) {
                                        if (strcmp(topicsList[p].subscribers[k].clientId, whichClient.clientId) == 0) {
                                            topicsList[p].subscribers[k] = topicsList[p].subscribers[
                                                    topicsList[p].nrSubs - 1];
                                            topicsList[p].nrSubs--;
                                            break;
                                        }
                                    }
                                }
                            }
                        }


                    }
                }
            }
        }


        if (goodbye) {
            break;

        }

    }
    for (int i = 2; i <= fdmax; i++) {
        close(i);
    }
    close(sockfd_tcp);

    return 0;

}
