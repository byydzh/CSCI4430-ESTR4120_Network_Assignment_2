#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <vector>
#include <iostream>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <chrono>
#include <netinet/in.h>


#define MAX_CLIENTS 10
#define MAX_BUFFER_SIZE 10000
#define MAX_REQUEST_LINE_LENGTH 1024

using namespace std;

int main(int argc, const char** argv)
{
    //Step1: check argvs
    if(argc != 6){
        printf("Usage: %s --nodns <listen-port> <www-ip> <alpha> <log>\n",argv[0]);
        return 0;
    }
    int flag;
    int listen_port = atoi(argv[2]);
    string www_ip = string(argv[3]);
    double alpha = atof(argv[4]);
    string log = string(argv[5]);
    
    if(strcmp(argv[1], "--nodns") == 0)
        flag = 1;
    else if (strcmp(argv[1], "--dns") == 0)
        flag = 2;
    else
        flag = 0;
    
    //step2: get proxy_server_socket
    int server_socket;
    struct sockaddr_in server_address;
    server_socket = get_proxy_server_socket(&server_address, listen_port);
    
    //step3: get proxy_client_socket
    
    //step4: deal with connections
    fd_set readfds;
    while (1)
    {
        // clear the socket set
        FD_ZERO(&readfds);

        // add server socket to set
        FD_SET(server_socket, &readfds);
        for (int i = 0; i < MAXCLIENTS; i++)
        {
            client_sock = client_sockets[i];
            if (client_sock != 0)
            {
                FD_SET(client_sock, &readfds);
            }
        }
        // wait for an activity on one of the sockets , timeout is NULL ,
        // so wait indefinitely
        activity = select(FD_SETSIZE, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR))
        {
            perror("select error");
        }

        // If something happened on the master socket ,
        // then its an incoming connection, call accept()
        if (FD_ISSET(server_socket, &readfds))
        {
            int new_socket = accept(server_socket, (struct sockaddr *)&address,
                                    (socklen_t *)&addrlen);
            if (new_socket < 0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            // inform user of socket number - used in send and receive commands
            printf("\n---New host connection---\n");
            printf("socket fd is %d , ip is : %s , port : %d \n", new_socket,
                   inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            // send new connection greeting message
            // TODO: REMOVE THIS CALL TO SEND WHEN DOING THE ASSIGNMENT.
            ssize_t send_ret = send(new_socket, message, strlen(message), 0);
            if (send_ret != strlen(message))
            {
                perror("send");
            }
            printf("Welcome message sent successfully\n");
            // add new socket to the array of sockets
            for (int i = 0; i < MAXCLIENTS; i++)
            {
                // if position is empty
                if (client_sockets[i] == 0)
                {
                    client_sockets[i] = new_socket;
                    break;
                }
            }
        }
        // else it's some IO operation on a client socket
        for (int i = 0; i < MAXCLIENTS; i++)
        {
            client_sock = client_sockets[i];
            // Note: sd == 0 is our default here by fd 0 is actually stdin
            if (client_sock != 0 && FD_ISSET(client_sock, &readfds))
            {
                // Check if it was for closing , and also read the
                // incoming message
                getpeername(client_sock, (struct sockaddr *)&address,
                            (socklen_t *)&addrlen);
                valread = read(client_sock, buffer, 1024);
                if (valread == 0)
                {
                    // Somebody disconnected , get their details and print
                    printf("\n---Host disconnected---\n");
                    printf("Host disconnected , ip %s , port %d \n",
                           inet_ntoa(address.sin_addr), ntohs(address.sin_port));
                    // Close the socket and mark as 0 in list for reuse
                    close(client_sock);
                    client_sockets[i] = 0;
                }
                else
                {
                    //variables you need
                    
                    //if it's f4m request...
                    //if it's video chunk request...
                    //if other requests
                           
                }
            }
        }
    }
    return 0;
    
}
