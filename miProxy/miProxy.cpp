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

    function_1(flag, listen_port, www_ip, alpha, log);
}