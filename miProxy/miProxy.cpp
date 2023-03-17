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

int get_server_socket(struct sockaddr_in *address, int listen_port) {
    int yes = 1;
    int server_socket;
    // create a master socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket <= 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // set master socket to allow multiple connections ,
    // this is just a good habit, it will work without this
    int success =
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if (success < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // type of socket created
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = INADDR_ANY;
    address->sin_port = htons(listen_port);

    // bind the socket to localhost port 8888
    success = bind(server_socket, (struct sockaddr *)address, sizeof(*address));
    if (success < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("---Listening on port %d---\n", listen_port);

    // try to specify maximum of N pending connections for the server socket
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    return server_socket;
}

int main(int argc, const char** argv)
{
    //Step1: check argvs
    if(argc != 6){
        printf("Usage: %s --nodns <listen-port> <www-ip> <alpha> <log>\n",argv[0]);
        return -1;
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
    server_socket = get_server_socket(&server_address, listen_port);
    
    //step3: get proxy_client_socket
    int client_socket;
    int client_sockets[MAX_CLIENTS] = {0};
    

    //step4: deal with connections
    fd_set readfds;
    while (1)
    {
        // clear the socket set
        FD_ZERO(&readfds);

        // add master socket to set
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