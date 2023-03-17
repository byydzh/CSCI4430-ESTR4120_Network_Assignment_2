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
#define MAX_MESSAGE_SIZE 10000
#define MAX_REQUEST_LINE_LENGTH 1024
#define MAX_BITRATE_LEVEL 10

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

// clear is used when you want clear exsit context
void out2log(string log_name, string browser_ip, string chunkname, string server_ip, 
            double duration, double tput, double avg_tput, int bitrate, int clear=0){
    FILE *fp;
    if(clear)
        fp = fopen(log_name.c_str(), "w+");
    else
        fp = fopen(log_name.c_str(), "a");
    fprintf(fp, "%s %s %s %f %f %f %d\n",
            browser_ip.c_str(), chunkname.c_str(), server_ip.c_str(), 
            duration, tput, avg_tput, bitrate);

    fclose(fp);
}

int choose_bitrate(double avg_tput, int* bitrate_level){
    int limit = round(avg_tput * 1.5);
    int current_bitrate = 10;
    for(int i=0;i<MAX_BITRATE_LEVEL;i++){
        if(current_bitrate < bitrate_level[i] && bitrate_level[i] <= limit)
            current_bitrate = bitrate_level[i];
    }
    return current_bitrate;
}

int main(int argc, const char** argv)
{
    //Step0: global variables
    string message;
    
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

    //step2: get server_socket
    int server_socket;
    struct sockaddr_in server_address;
    server_socket = get_server_socket(&server_address, listen_port);
    
    //step3: get client_sockets
    int client_sock;
    int client_sockets[MAX_CLIENTS] = {0};
    double client_throughputs_current[MAX_CLIENTS] = {0.0};
    string client_ips[MAX_CLIENTS];
    int bitrate_level[MAX_BITRATE_LEVEL] = {0};
    
    //step4: deal with connections
    fd_set readfds;
    while (1)
    {
        // clear the socket set
        FD_ZERO(&readfds);

        // add master socket to set
        FD_SET(server_socket, &readfds);
        for (int i = 0; i < MAX_CLIENTS; i++)
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

        // If something happened on the server socket, then its an incoming connection, call accept()
        if (FD_ISSET(server_socket, &readfds))
        {
            int new_socket = accept(server_socket, (struct sockaddr *)&server_address,
                                    (socklen_t *)&(sizeof(server_address)));
            if (new_socket < 0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            // inform user of socket number - used in send and receive commands
            printf("\n---New connection---\n");
            printf("socket fd is %d , ip is : %s , port : %d \n", new_socket,
                   inet_ntoa(server_address.sin_addr), ntohs(server_address.sin_port));

            // send new connection greeting message
            // TODO: REMOVE THIS CALL TO SEND WHEN DOING THE ASSIGNMENT.
            //ssize_t send_ret = send(new_socket, message, strlen(message), 0);
            //printf("Welcome message sent successfully\n");
            
            // add new socket to the array of sockets
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                // if position is empty
                if (client_sockets[i] == 0)
                {
                    client_sockets[i] = new_socket;
                    strcpy(client_ips[i], inet_ntoa(server_address.sin_addr));
                    break;
                }
            }
        }
        
        // else it's some IO operation on a client socket
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            client_sock = client_sockets[i];
            struct sockaddr_in client_address;
            int client_addrlen = sizeof(sockaddr_in);
            // Note: sd == 0 is our default here by fd 0 is actually stdin
            if (client_sock != 0 && FD_ISSET(client_sock, &readfds))
            {
                // Check if it was for closing , and also read the incoming message
                getpeername(client_sock, (struct sockaddr *)&client_address,
                            (socklen_t *)&client_addrlen);
                int valread = read(client_sock, message, MAX_MESSAGE_SIZE);
                if (valread == 0)
                {
                    // Somebody disconnected , get their details and print
                    printf("\n---Client %d disconnected---\n", i);
                    printf("Host disconnected , ip %s , port %d \n",
                           inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
                    // Close the socket and mark as 0 in list for reuse
                    close(client_sock);
                    client_sockets[i] = 0;
                    client_throughput_current[i] = 0;
                }
                else
                {
                    //variables you need
                    message[valread] = '\0';
                    size_t loc;
                    //if it's f4m request...
                    if ((loc = message.find(".f4m")) != message.npos){
                        string message_nolist = message, message_buffer;
                        char buffer[MAX_MESSAGE_SIZE];
                        message_nolist.insert(loc, "_nolist");
                        
                        //_nolist.f4m process
                        if (send(server_socket, message_nolist.c_str(), message_nolist.size(), 0) < 0){
                            perror("Error: send to server failed\n");
                            exit(EXIT_FAILURE);
                        }
                        valread = read(server_socket, buffer, MAX_MESSAGE_SIZE);
                        if (valread == 0){
                            printf("---Server disconnected---\n");
                        }
                        buffer[valread] = '\0';
                        message_buffer = buffer;
                        if (send(client_socket, buffer, valread, 0) < 0){
                            perror("Error: send to client %d failed\n", i);
                            exit(EXIT_FAILURE);
                        }
                        
                        //Locally parse for header length
                        size_t header_end = message_buffer.find("\r\n\r\n");
                        if(header_end == message_buffer.npos)
                        {
                            perror("Error: fail to find header_end");
                            exit(EXIT_FAILURE);
                        }
                        int header_length = static_cast<int>(header_end) +4;
                        
                        //Locally parse for content length
                        size_t content_length_info = message_buffer.find("Content-Length: ");
                        if(content_length_info == message_buffer.npos)
                        {
                            perror("Error: fail to find content length info");
                            exit(EXIT_FAILURE);
                        }
                        size_t digit_start = content_length_info + "Content-Length: ".size();
                        size_t digit_end;
                        for(digit_end = digit_start; isdigit(message[digit_end]); digit_end++)
                        {
                            ;
                        }
                        int content_length = stoi(message_buffer.substr(digit_start, digit_end-digit_start));
                        
                        // receive and send back the remaining part
                        int remaining_length = content_length + header_length - valread;
                        while(remaining_length > 0)
                        {
                            valread = read(server_socket, buffer, MAX_MESSAGE_SIZE);
                            remaining_length -= valread;
                            send(client_sock, buffer, valread, 0);
                            memset(buffer, 0, MAX_MESSAGE_SIZE);
                        }


                        // original .f4m process
                        if (send(server_socket, message.c_str(), message.size(), 0) < 0)
                            printf("send to server failed\n");

                        valread = read(server_socket, buffer, MAX_MESSAGE_SIZE);
                        if (valread == 0){
                            printf("---Server disconnected---\n");
                        }
                        buffer[valread] = '\0';
                        message_buffer = buffer;

                        while(1){
                            size_t bitrate_info = message_buffer.find("bitrate: \"");
                            if(bitrate_info == message_buffer.npos)
                                break;
                            digit_start = bitrate_info + "bitrate: \"".size();
                            for(digit_end = digit_start; isdigit(message[digit_end]); digit_end++);
                            int bitrate = stoi(message_buffer.substr(digit_start, digit_end-digit_start));
                            
                            for(int j=0;j<MAX_BITRATE_LEVEL;j++){
                                if(bitrate_level[j] == 0){
                                    bitrate_level[j] = bitrate;
                                    break;
                                }
                            }
                        }
                        while(remaining_length > 0){
                            valread = read(server_socket, buffer, MAX_MESSAGE_SIZE);
                            remaining_length -= valread;
                            memset(buffer, 0, MAX_MESSAGE_SIZE);
                        }
                    }
                    //if it's video chunk request...
                    else if( (loc = message.find("Seg")) != message.npos)
                    {
                        //get the position of bitrate
                        size_t end_position = loc - 1;
                        size_t start_position = end_position;
                        while(start_position >= 0 && isdigit(message[start_position]))
                        {
                            start_position--;
                        }
                        start_position++;
                        message.replace(start_position, end_position-start_position, to_string(new_bitrate))
                    }
                    //if other requests, directly send
                    else
                    {
                        //first send the request to server
                        send(server_socket, message.c_str(), valread, 0);
                        
                        //Then receive response from server, and send back to client
                        memset(message, 0, MAX_MESSAGE_SIZE);
                        valread = read(server_socket, message, MAX_MESSAGE_SIZE);
                        message[valread] = '\0';
                        //send the response to client
                        send(client_sock, message.c_str(), valread, 0);
                        
                        //Locally parse for header length
                        size_t header_end = message.find("\r\n\r\n");
                        if(header_end == message.npos)
                        {
                            perror("Error: fail to find header_end");
                            exit(EXIT_FAILURE);
                        }
                        int header_length = static_cast<int>(header_end) +4;
                        
                        //Locally parse for content length
                        size_t content_length_info = message.find("Content-Length: ");
                        if(content_length_info == message.npos)
                        {
                            perror("Error: fail to find content length info");
                            exit(EXIT_FAILURE);
                        }
                        size_t digit_start = content_length_info + "Content-Length: ".size();
                        size_t digit_end;
                        for(digit_end = digit_start; isdigit(message[digit_end]); digit_end++)
                        {
                            ;
                        }
                        int content_length = stoi(message.substr(digit_start, digit_end-digit_start));
                        
                        // receive and send back the remaining part
                        int remaining_length = content_length + header_length - valread;
                        while(remaining_length > 0)
                        {
                            valread = read(server_socket, message, MAX_MESSAGE_SIZE);
                            remaining_length -= valread;
                            send(client_sock, message.c_str(), valread, 0);
                            memset(message, 0, MAX_MESSAGE_SIZE);
                        }
                    }
                }
            }
        }
    }
    return 0;
    
}
