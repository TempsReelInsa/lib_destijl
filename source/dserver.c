/**
 * \file dserver.c
 * \brief Source de la classe DServer
 * \author P.-E. Hladik
 * \version 0.1
 * \date 01 décembre 2011
 *
 * Implémentation de la classe DServer
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/wait.h>
#include "../headers/dserver.h"
#include "../headers/dtools.h"

#define BACKLOG 10

static void d_server_init(DServer*);
void sigchld_handler(int s);
void *get_in_addr(struct sockaddr *sa);

static void d_server_init(DServer *This)
{
    This->bind = d_server_bind;
    This->accept = d_server_accept;
    This->print = d_server_print;
    This->close = d_server_close;
    This->close_client = d_server_close_client;
    This->is_active = d_server_is_active;
    This->receive = d_server_receive;
    This->send = d_server_send;

    This->sock_serv = 0;
    This->sock_client = 0;
    This->active = 0;
}

DServer* d_new_server()
{
    DServer *This = malloc(sizeof(DServer));

    if (This == NULL)
    {
        return NULL;
    }

    d_server_init(This);
    This->free = d_server_free;
    return This;
}

void d_server_print(DServer *This)
{
    printf("server{sock_serv:%d, sock_client:%d, active:%d}\n", This->sock_serv, This->sock_client, This->active);
}

void d_server_free(DServer *This)
{
    free(This);
}

int d_server_bind(DServer *This, char *port)
{
    int rv;
    int sockfd;
    int yes = 1;
    struct addrinfo hints;
    struct addrinfo *servinfo;
    struct addrinfo *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
        {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "server: failed to bind\n");
        return -1;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if(listen(sockfd, BACKLOG) == -1)
    {
        return -1;
    }

    This->sock_serv = sockfd;

    return 0;
}

int d_server_accept(DServer *This)
{
    struct sockaddr_storage client;
    socklen_t client_size = sizeof(client);

    This->sock_client = accept(This->sock_serv, (struct sockaddr *) &client, &client_size);
    if(This->sock_client == -1)
    {
        return -1;
    }

    This->active = 1;

    return 0;
}

void d_server_close(DServer *This)
{
    This->close_client(This);
    close(This->sock_serv);
}

void d_server_close_client(DServer *This)
{
    This->active = 0;
    close(This->sock_client);
}

int d_server_send(DServer *This, DMessage *message)
{
    int ret;
    size_t nb_sent = 0;
    if (This->active)
    {
        while (nb_sent < message->get_lenght(message) && This->active)
        {
            ret = send(This->sock_client,
                    message->get_data(message) + nb_sent,
                    message->get_lenght(message) - nb_sent,
                    MSG_NOSIGNAL);

            if (ret == -1)
            {
                perror("[tcp_client_a::send]");
                return ret;
            }

            nb_sent += ret;
        }
    }
    return nb_sent;
}

int d_server_receive(DServer *This, DMessage *message)
{
    char type = 'Z';
    char buf[4];
    char* pData;
    int size = 0;
    int retour = 0;

    retour = recv(This->sock_client, &type, 1, 0); /* Lecture du type */
    //printf("retour:%d type:%c\n",retour, type);
    if (retour > 0) {
        retour = recv(This->sock_client, buf, 4, 0);
        if (retour > 0) { /* Lecture de la taille des données*/
            size = d_tools_bytes2int(buf);
            pData = (char *) malloc(size);
            retour = recv(This->sock_client, pData, size, 0);
            if (retour > 0) {
                //message = d_new_message();
                message->set(message, type, size, pData);
            }
        }
    }

    return retour;
}

int d_server_is_active(DServer *This)
{
    return This->active;
}
