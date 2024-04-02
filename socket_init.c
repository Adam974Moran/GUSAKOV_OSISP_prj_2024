#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT1 2121
#define PORT2 2020

int main(){
    int server_fd, new_socket;
    ssize_t valread;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    

    char buffer[1024] = { 0 };
    
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Faled to initiallize a socket");
        exit(EXIT_FAILURE);
    }

    if((setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))){
        perror("Faled to set socket options");
        exit(EXIT_FAILURE);
    }

    if(getsockname(server_fd, (struct sockaddr*)&address, &addrlen)){
        perror("Faled to getsockname");
        exit(EXIT_FAILURE);
    }
    else{
        char * ip = inet_ntoa(address.sin_addr);
        printf("Server IP address is: %s\n", ip);
    }

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT1);

    if(bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0){
        perror("Faled to bind");
        exit(EXIT_FAILURE);
    }

    if(listen(server_fd, 3) < 0){
        perror("Faled to listen");
        exit(EXIT_FAILURE);
    }

    if((new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen)) < 0){
        perror("No ecception");
        exit(EXIT_FAILURE);
    }

    valread = read(new_socket, buffer, 1023);

    printf("%s\n", buffer);
    close(new_socket);
    close(server_fd);
    return 0;

}