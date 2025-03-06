#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/select.h>


typedef struct client {
    int id;
    char  txt[99999];
} t_client;

t_client clients[1024];

fd_set fdAct, fdIn, fdOut;
int fdMax = 0, idIndex = 0;
char buffOut[400000], buffIn[400000];

void err()
{
    write(2, "Fatal error\n", 12);
    exit(1);
}

void sendToAll(int ignore)
{
    for (int i = 0; i <= fdMax; i++)
        if (FD_ISSET(i, &fdOut) && i != ignore)
            send(i, buffIn, strlen(buffIn), 0);
}

int main(int ac, char **av)
{
    if (ac != 2)
    {
        write(2, "Wrong number of arguments\n", 26);
        exit(1);
    }

    struct sockaddr_in addr;
    bzero(&clients, sizeof(clients));
	bzero(&addr, sizeof(addr));
    FD_ZERO(&fdAct);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        err();

    fdMax = sockfd;
    FD_SET(sockfd, &fdAct);

    socklen_t addr_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(2130706433);
    addr.sin_port = htons(atoi(av[1]));

    if ((bind(sockfd, (const struct sockaddr *)&addr, sizeof(addr))) < 0)
        err();
    if (listen(sockfd, 128) < 0)
        err();

    while (1)
    {
        fdIn = fdOut = fdAct;
        if (select(fdMax + 1, &fdIn, &fdOut, NULL, NULL) < 0)
            continue ;
        for (int s = 0; s <= fdMax; s++)
        {
            if (FD_ISSET(s, &fdIn) && s == sockfd)
            {
                int clientSock = accept(sockfd, (struct sockaddr *)&addr, &addr_len);
                if (clientSock < 0)
                    continue ;
                fdMax = (clientSock > fdMax) ? clientSock : fdMax;
                clients[clientSock].id = idIndex++;
                FD_SET(clientSock, &fdAct);
                sprintf(buffIn, "server: client %d just arrived\n", clients[clientSock].id);
                sendToAll(clientSock);
                break ;
            }
            if (FD_ISSET(s, &fdIn) && s != sockfd)
            {
                int res = recv(s, buffOut, 42 * 4096, 0);
                if (res <= 0)
                {
                    sprintf(buffIn, "server: client %d just left\n", clients[s].id);
                    sendToAll(s);
                    FD_CLR(s, &fdAct);
                    close(s);
						  bzero(clients[s].txt, strlen(clients[s].txt));
                    break ;
                }
                else
                {
                    for (int i = 0, j = strlen(clients[s].txt); i < res; i++, j++)
                    {
                        clients[s].txt[j] = buffOut[i];
                        if (clients[s].txt[j] == '\n')
                        {
                            clients[s].txt[j] = '\0';
                            sprintf(buffIn, "client %d: %s\n", clients[s].id, clients[s].txt);
                            sendToAll(s);
                            bzero(&clients[s].txt, strlen(clients[s].txt));
                            j = -1;
                        }
                    }
                    break;
                }
            }
        }
    }
    return 0;
}