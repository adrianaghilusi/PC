#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"


int main() {
    char *message;
    char *response;
    int sockfd;

    char *host = "34.118.48.238";
    char request[MAX_LEN];
    char lastCookie[MAX_LEN] ;
    memset(lastCookie, 0, sizeof(lastCookie));
    char JWT_token[MAX_LEN] ;
    memset(JWT_token, 0, sizeof(JWT_token));


    while (1) {
        memset(request, 0, sizeof(request));
        scanf("%s", request);
        if (strcmp(request, "exit") == 0) {
            break;
        }
        if (strcmp(request, "register") == 0) {
            JSON_Value *value = json_value_init_object();
            JSON_Object *object = json_value_get_object(value);
            sockfd = open_connection(host, 8080, AF_INET, SOCK_STREAM, 0);
            char username[MAX_LEN];
            char password[MAX_LEN];
            printf("username=");
            //scanf("%s", username);
            char dummy[MAX_LEN];
            fgets(dummy, sizeof(dummy), stdin);
            fgets(username, MAX_LEN, stdin);
            if (strlen(username) > 0 && (username[strlen(username) - 1] == '\n')) {
                username[strlen(username) - 1] = '\0';
            }
            printf("password=");
           // scanf("%s", password);
            fgets(password, MAX_LEN, stdin);
            if (strlen(password) > 0 && (password[strlen(password) - 1] == '\n')) {
                password[strlen(password) - 1] = '\0';
            }
            json_object_set_string(object, "username", username);
            json_object_set_string(object, "password", password);
            message = compute_post_request(host, "/api/v1/tema/auth/register", "application/json", value, NULL, NULL);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            if (strstr(response, "Error") != NULL) {
                printf("Eroare: Username-ul este deja folosit.\n");
            } else {
                printf("Inregistrare realizata cu succes!\n");
            }
            close_connection(sockfd);
        }
        if (strcmp(request, "login") == 0) {
            JSON_Value *value = json_value_init_object();
            JSON_Object *object = json_value_get_object(value);
            sockfd = open_connection(host, 8080, AF_INET, SOCK_STREAM, 0);
            char username[MAX_LEN];
            char password[MAX_LEN];
            printf("username=");
            //scanf("%s", username);
            char dummy[MAX_LEN];
            fgets(dummy, sizeof(dummy), stdin);
            fgets(username, MAX_LEN, stdin);
            if (strlen(username) > 0 && (username[strlen(username) - 1] == '\n')) {
                username[strlen(username) - 1] = '\0';
            }
            printf("password=");
            //scanf("%s", password);
            fgets(password, MAX_LEN, stdin);
            if (strlen(password) > 0 && (password[strlen(password) - 1] == '\n')) {
                password[strlen(password) - 1] = '\0';
            }
            json_object_set_string(object, "username", username);
            json_object_set_string(object, "password", password);
            message = compute_post_request(host, "/api/v1/tema/auth/login", "application/json", value, NULL, NULL);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            if (strstr(response, "Credentials are not good!") != NULL) {
                printf("Eroare: Credentialele nu se potrivesc\n");
            } else if (strstr(response, "No account with this username!") != NULL) {
                printf("Eroare: Credentialele nu se potrivesc\n");
            } else {
                char *ret = strstr(response, "connect.sid");
                char *token = strtok(ret, ";");
                strcpy(lastCookie, token);
                printf("Autentificare realizata cu succes!\n");
                printf("Cookie de sesiune: %s\n", lastCookie);
            }

            close_connection(sockfd);
        }
        if (strcmp(request, "enter_library") == 0) {
            sockfd = open_connection(host, 8080, AF_INET, SOCK_STREAM, 0);
            message = compute_get_request(host, "/api/v1/tema/library/access", NULL, lastCookie, JWT_token);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            if (strstr(response, "You are not logged in!") != NULL) {
                printf("Eroare: Acces nepermis!\n");
            } else {
                char *ret = strstr(response, "\"token\":\"");
                char *token = strtok(ret, " :\" ");
                token = strtok(NULL, ":\"");
                strcpy(JWT_token, token);
                printf("Intrare in librarie realizata cu succes!\n");
                printf("Token JWT: %s\n", JWT_token);
            }

            close_connection(sockfd);
        }
        if (strcmp(request, "get_books") == 0) {
            sockfd = open_connection(host, 8080, AF_INET, SOCK_STREAM, 0);
            message = compute_get_request(host, "/api/v1/tema/library/books", NULL, lastCookie, JWT_token);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
          //  puts(response);
            if (strstr(response, "error") != NULL) {
                printf("Eroare: Acces nepermis!\n");
            } else {
                char *ret = strstr(response, "[");
                puts(ret);

            }

            close_connection(sockfd);
        }
        if (strcmp(request, "get_book") == 0) {
            sockfd = open_connection(host, 8080, AF_INET, SOCK_STREAM, 0);
            char id[MAX_LEN];
            printf("id=");
            scanf("%s", id);
            char url[MAX_LEN] = "/api/v1/tema/library/books/";
            strcat(url, id);
            message = compute_get_request(host, url, NULL, lastCookie, JWT_token);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            if (strstr(response, "error") != NULL) {
                if(strstr(response, "int") != NULL){
                    printf("Eroare: Va rugam introduceti un numar!\n");
                }else{
                    printf("Eroare: Acces nepermis!\n");
                }

            } else if (strstr(response, "No book was found!") != NULL) {
                printf("Eroare: Cartea cu id-ul %s nu exista!\n", id);
            } else {
                char *ret = strstr(response, "[");
                char *token = strtok(ret, "[");
                token = strtok(token, "]");
                puts(token);
            }

            close_connection(sockfd);
        }
        if (strcmp(request, "add_book") == 0) {
            JSON_Value *value = json_value_init_object();
            JSON_Object *object = json_value_get_object(value);
            sockfd = open_connection(host, 8080, AF_INET, SOCK_STREAM, 0);
            char *title = (char*)malloc(MAX_LEN);
            printf("title=");
            char dummy[MAX_LEN];
            fgets(dummy, sizeof(dummy), stdin);
            fgets(title, MAX_LEN, stdin);
            if(strlen(title)==1){
                title = NULL;
            }
            else{
                title[strlen(title) - 1] = '\0';
            }
            json_object_set_string(object, "title", title);
            char *author = (char*)malloc(MAX_LEN);
            printf("author=");
            fgets(author, MAX_LEN, stdin);
            if(strlen(author)==1){
                author = NULL;
            }
            else{
                author[strlen(author) - 1] = '\0';
            }

            json_object_set_string(object, "author", author);
            char *genre = (char*)malloc(MAX_LEN);
            printf("genre=");
            fgets(genre, MAX_LEN, stdin);
            if(strlen(genre)==1){
                genre = NULL;
            }
            else{
                genre[strlen(genre) - 1] = '\0';
            }
            json_object_set_string(object, "genre", genre);
            char *publisher = (char*)malloc(MAX_LEN);
            printf("publisher=");
            fgets(publisher, MAX_LEN, stdin);
            if(strlen(publisher)==1){
                publisher = NULL;
            }
            else{
                publisher[strlen(publisher) - 1] = '\0';
            }
            json_object_set_string(object, "publisher", publisher);
            char *page_count = (char*)malloc(MAX_LEN);
            printf("page_count=");
            fgets(page_count, MAX_LEN, stdin);
            if(strlen(page_count)==1){
                page_count = NULL;
            }
            else{
                page_count[strlen(page_count) - 1] = '\0';
            }
            int pages=-1;
            if(page_count!=NULL){
                pages = atoi(page_count);
            }
            json_object_set_number(object, "page_count", pages);
            message = compute_post_request(host, "/api/v1/tema/library/books", "application/json", value, NULL,
                                           JWT_token);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            puts(response);
            if (strstr(response, "error") != NULL) {
                if (strstr(response, "missing") != NULL){
                    printf("Eroare: Date completate incorect!\n");
                }
                else{
                    printf("Eroare: Acces nepermis!\n");
                }

            } else {
                printf("Carte adaugata cu succes!\n");
            }
            close_connection(sockfd);
        }
        if (strcmp(request, "delete_book") == 0) {
            sockfd = open_connection(host, 8080, AF_INET, SOCK_STREAM, 0);
            char id[MAX_LEN];
            printf("id=");
            scanf("%s", id);
            char url[MAX_LEN] = "/api/v1/tema/library/books/";
            strcat(url, id);
            message = compute_delete_request(host, url, NULL, lastCookie, 0, JWT_token);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            if (strstr(response, "error") != NULL) {
                if(strstr(response, "Something Bad Happened") != NULL){
                    printf("Eroare: Va rugam introduceti un numar!\n");
                }else if (strstr(response, "No book was deleted!") != NULL)
                    printf("Eroare: Cartea cu id-ul %s nu exista!\n", id);
                else{
                    printf("Eroare: Acces nepermis!\n");
                }
            }
             else {
                printf("Carte stearsa cu succes!\n");
            }
            close_connection(sockfd);
        }
        if (strcmp(request, "logout") == 0) {
            sockfd = open_connection(host, 8080, AF_INET, SOCK_STREAM, 0);
            message = compute_get_request(host, "/api/v1/tema/auth/logout", NULL, lastCookie, NULL);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            memset(lastCookie, 0, sizeof(lastCookie));
            memset(JWT_token, 0, sizeof(JWT_token));
            if (strstr(response, "You are not logged in!")) {
                printf("Eroare: nu sunteti logat!\n");
            } else {
                printf("Delogare realizata cu succes!\n");
            }

            close_connection(sockfd);
        }

    }

    close_connection(sockfd);


    return 0;
}
