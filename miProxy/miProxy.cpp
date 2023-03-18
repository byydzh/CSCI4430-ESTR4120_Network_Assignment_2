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
#define MAX_MESSAGE_SIZE 3000
#define MAX_REQUEST_LINE_LENGTH 1024
#define MAX_BITRATE_LEVEL 10

using namespace std;

int get_server_socket(struct sockaddr_in *address, int listen_port) {
    int yes = 1;
    int server_socket;
    // create a master socket
    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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
    double limit = (avg_tput / 1.5);
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
    int server_addrlen = sizeof(sockaddr_in);
    server_socket = get_server_socket(&server_address, listen_port);
    
    //step3: get client_sockets
    int client_sock;
    int client_sockets[MAX_CLIENTS] = {0};
    double client_throughputs_current[MAX_CLIENTS] = {0.0};
    for(int i=0; i<MAX_CLIENTS; i++)
        client_throughputs_current[i] = 10.0;
    string client_ips[MAX_CLIENTS];
    int bitrate_level[MAX_BITRATE_LEVEL] = {0};
    // create proxy client socket
    int proxy_client_socket;
    struct sockaddr_in client_addr;
    if((proxy_client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        perror("socket creation failed");
        exit(1);
    }
    
    // connect to the video server
    struct hostent *server = gethostbyname(www_ip.c_str());
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = * (unsigned long *) server->h_addr_list[0];
    client_addr.sin_port = htons(80);
    if(connect(proxy_client_socket, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0){
        perror("connection failed");
        exit(1);
    }
    
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
                FD_SET(client_sock, &readfds);printf("yes! %d\n",i);
            }
        }
        // wait for an activity on one of the sockets , timeout is NULL ,
        // so wait indefinitely
        int activity = select(FD_SETSIZE, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR))
        {
            perror("select error");
        }

        // If something happened on the server socket, then its an incoming connection, call accept()
        if (FD_ISSET(server_socket, &readfds))
        {
            int new_socket = accept(server_socket, (struct sockaddr *)&server_address,
                                    (socklen_t *)&server_addrlen);
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
            //ssize_t send_ret = send(new_socket, message, strlen(message), 0);
            //printf("Welcome message sent successfully\n");
            
            // add new socket to the array of sockets
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                // if position is empty
                if (client_sockets[i] == 0)
                {
                    client_sockets[i] = new_socket;
                    client_ips[i] = inet_ntoa(server_address.sin_addr);
                    printf("here new client%d\n\n\n",i);
                    break;
                }
            }
        }printf("Strat check clients\n");
        
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
                //getpeername(client_sock, (struct sockaddr *)&client_address,
                            //(socklen_t *)&client_addrlen);
                char buffer[MAX_MESSAGE_SIZE];
                int valread = read(client_sock, buffer, MAX_MESSAGE_SIZE);
                printf("here read client's buffer\n\n\n");
                if (valread == 0)
                {
                    // Somebody disconnected , get their details and print
                    printf("\n---Client %d disconnected---\n", i);
                    //printf("Host disconnected , ip %s , port %d \n",
                           //inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
                    // Close the socket and mark as 0 in list for reuse
                    close(client_sock);
                    client_sockets[i] = 0;
                    client_throughputs_current[i] = 10.0;
                }
                else
                {
                    //variables you need
                    buffer[valread] = '\0';
                    message = buffer;
                    size_t loc;
                    //if it's f4m request...
                    if ((loc = message.find(".f4m")) != message.npos){
                        printf("here we want to deal with f4m\n\n\n");
                        
                        //cout << message << endl;
                        string message_nolist = message, message_buffer;
                        memset(buffer, 0, MAX_MESSAGE_SIZE);
                        message_nolist.insert(loc, "_nolist");
                        //cout << message_nolist << endl;
                        // original .f4m process
                        cout << message << endl;
                        if (send(proxy_client_socket, message.c_str(), message.size(), 0) < 0)
                            printf("send to server failed\n");

                        valread = read(proxy_client_socket, buffer, MAX_MESSAGE_SIZE);
                        if (valread == 0){
                            printf("---Server disconnected---\n");
                        }
                        buffer[valread] = '\0';
                        message_buffer = buffer;//cout << message_buffer << endl;

                        //Locally parse for header length
                        size_t header_end = message_buffer.find("\r\n\r\n");
                        if(header_end == message_buffer.npos)
                        {
                            perror("Error: fail to find header_end_1");
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
                        size_t digit_start = content_length_info + 16;
                        size_t digit_end;
                        for(digit_end = digit_start; isdigit(message_buffer[digit_end]); digit_end++)
                        {
                            ;
                        }
                        int content_length = stoi(message_buffer.substr(digit_start, digit_end-digit_start));
                        
                        // receive and send back the remaining part
                        int remaining_length = content_length + header_length - valread;
                        while(remaining_length > 0){
                            memset(buffer, 0, MAX_MESSAGE_SIZE);
                            valread = recv(proxy_client_socket, buffer, MAX_MESSAGE_SIZE, 0);
                            remaining_length -= valread;
                            message_buffer += buffer;
                        }//cout << message_buffer << endl;

                        while(1){
                            size_t bitrate_info = message_buffer.find("bitrate=");
                            if(bitrate_info == message_buffer.npos)
                                break;
                            digit_start = bitrate_info + 9;
                            for(digit_end = digit_start; isdigit(message_buffer[digit_end]); digit_end++);
                            int bitrate = stoi(message_buffer.substr(digit_start, digit_end-digit_start));
                            message_buffer[digit_start - 5] = 'l';
                            
                            for(int j=0;j<MAX_BITRATE_LEVEL;j++){
                                if(bitrate_level[j] == 0){
                                    bitrate_level[j] = bitrate;
                                    printf("new bitrate: %d\n", bitrate);
                                    break;
                                }
                            }
                        }

                        //_nolist.f4m process
                        if (send(proxy_client_socket, message_nolist.c_str(), message_nolist.size(), 0) < 0){
                            perror("Error: send to server failed\n");
                            exit(EXIT_FAILURE);
                        }cout << message_nolist << endl;
                        valread = read(proxy_client_socket, buffer, MAX_MESSAGE_SIZE);
                        if (valread == 0){
                            printf("---Server disconnected---\n");
                        }
                        buffer[valread] = '\0';
                        message_buffer = buffer;
                        if (send(client_sock, buffer, valread, 0) < 0){
                            perror("Error: send to client failed\n");
                            exit(EXIT_FAILURE);
                        }
                        
                        //Locally parse for header length
                        header_end = message_buffer.find("\r\n\r\n");
                        if(header_end == message_buffer.npos)
                        {
                            perror("Error: fail to find header_end_1");
                            exit(EXIT_FAILURE);
                        }
                        header_length = static_cast<int>(header_end) +4;
                        
                        //Locally parse for content length
                        content_length_info = message_buffer.find("Content-Length: ");
                        if(content_length_info == message_buffer.npos)
                        {
                            perror("Error: fail to find content length info");
                            exit(EXIT_FAILURE);
                        }
                        digit_start = content_length_info + 16;
                        for(digit_end = digit_start; isdigit(message_buffer[digit_end]); digit_end++)
                        {
                            ;
                        }
                        content_length = stoi(message_buffer.substr(digit_start, digit_end-digit_start));
                        
                        // receive and send back the remaining part
                        remaining_length = content_length + header_length - valread;
                        while(remaining_length > 0)
                        {
                            memset(buffer, 0, MAX_MESSAGE_SIZE);
                            valread = read(proxy_client_socket, buffer, MAX_MESSAGE_SIZE);
                            remaining_length -= valread;
                            send(client_sock, buffer, valread, 0);
                        }


                        
                    }
                    //if it's video chunk request...
                    else if( (loc = message.find("Seg")) != message.npos)
                    {
                        printf("here we want to deal with video trunk\n\n\n");
                        //get the position of bitrate
                        cout << message << endl;
                        size_t end_position = loc - 1;
                        size_t start_position = end_position;
                        while(start_position >= 0 && isdigit(message[start_position]))
                        {
                            start_position--;
                        }
                        start_position++;
                        //get the position of chunk_end
                        size_t chunk_end_position = loc;
                        while(message[chunk_end_position] != ' ')
                        {
                            chunk_end_position++;
                        }
                        chunk_end_position--;
                        
                        //calculate the adapted bitrate
                        int new_bitrate = choose_bitrate(client_throughputs_current[i], bitrate_level);
                        printf("here new: %d\n", new_bitrate);
                        string chunk_name = to_string(new_bitrate) + message.substr(loc, chunk_end_position-loc+1);
                        //modify request
                        message.replace(start_position, end_position-start_position+1, to_string(new_bitrate));
                        //send the adapted request to server
                        send(proxy_client_socket, message.c_str(), message.size(), 0);
                        cout << message << endl;
                        
                        //Then ready to receive response from server
                        message.erase();
                        double total_time = 0.0;
                        auto start_time = chrono::high_resolution_clock::now();
                        memset(buffer, 0, MAX_MESSAGE_SIZE);
                        valread = read(proxy_client_socket, buffer, MAX_MESSAGE_SIZE);
                        buffer[valread] = '\0';
                        message = buffer;
                        cout << message << endl;
                        printf("here: %d", valread);

                        message[valread] = '\0';
                        //send the response to client
                        send(client_sock, buffer, valread, 0);
                        
                        //Locally parse for header length
                        size_t header_end = message.find("\r\n\r\n");
                        if(header_end == message.npos)
                        {
                            perror("Error: fail to find header_end_2");
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
                        size_t digit_start = content_length_info + 16;
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
                            valread = read(proxy_client_socket, buffer, MAX_MESSAGE_SIZE);
                            remaining_length -= valread;
                            send(client_sock, buffer, valread, 0);
                            memset(buffer, 0, MAX_MESSAGE_SIZE);
                        }
                        
                        //calculate throughput and generate log
                        total_time = chrono::duration<double>(chrono::high_resolution_clock::now() - start_time).count();
                        double throughput_new = (content_length + header_length)*8 / (total_time*1000);
                        client_throughputs_current[i] = alpha * throughput_new + (1-alpha) * client_throughputs_current[i];
                        out2log(log, client_ips[i], chunk_name, www_ip, total_time, throughput_new, client_throughputs_current[i], new_bitrate, 0);
                    }
                    //if other requests, directly send
                    else
                    {
                        printf("here we want to other requests\n\n");
                        //first send the request to server
                        send(proxy_client_socket, message.c_str(), valread, 0);
                        
                        //Then receive response from server, and send back to client
                        message.erase();
                        memset(buffer, 0, MAX_MESSAGE_SIZE);
                        valread = recv(proxy_client_socket, buffer, MAX_MESSAGE_SIZE, 0);
                        buffer[valread] = '\0';//printf("here len of buffer read\n%d\n\n",(int)(buffer[331]));
                        for(int j = 0; j < valread; j++){
                            message.push_back(buffer[j]);
                        }
                        //message = buffer;
                        //memset(buffer, 0, MAX_MESSAGE_SIZE);
                        message[valread] = '\0';//printf("here\n%s\n\n",message.c_str());
                        //send the response to client
                        int age = send(client_sock, buffer, valread, 0);
                        printf("\nmessage(can not print)\nmessage len: %d\n", age);
                        //cout << "\nhere cin:\n" << message << endl;
                        
                        //Locally parse for header length
                        size_t header_end = message.find("\r\n\r\n");
                        if(header_end == message.npos)
                        {
                            perror("Error: fail to find header_end_3");
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
                        size_t digit_start = content_length_info + 16;
                        size_t digit_end;
                        for(digit_end = digit_start; isdigit(message[digit_end]); digit_end++)
                        {
                            ;
                        }
                        int content_length = stoi(message.substr(digit_start, digit_end-digit_start));
                        printf("\ncontent_len: %d\nheader_len: %d",content_length, header_length);
                        
                        // receive and send back the remaining part
                        int remaining_length = content_length + header_length - valread;
                        printf("\nremain_len: %d\n",remaining_length);
                        while(remaining_length > 0)
                        {
                            memset(buffer, 0, MAX_MESSAGE_SIZE);
                            //sleep(1);
                            valread = recv(proxy_client_socket, buffer, MAX_MESSAGE_SIZE, 0);
                            printf("\nwhile_send: %d\nremaining_len: %d\n",valread, remaining_length);
                            //buffer[valread] = '\0';
                            remaining_length -= valread;
                            send(client_sock, buffer, valread, 0);
                        }
                        printf("here we end other requests\n\n");
                    }
                }
            }
        }
    }
    return 0;
    
}