#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h> 
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/stat.h>

#define CONTROL_PORT 2121
#define DATA_PORT 2020

const char * DOWNLOADS_FOLDER = "/home/adam/Downloads/"; //22
const size_t BYTES_TO_SEND_AND_TO_GET = 36864;


char * clear_buffer(int buffer_size){
    char * buffer = (char *)malloc(buffer_size*sizeof(char));
    for(int i = 0; i < buffer_size; i++){
        buffer[i] = '\0';
    }
    return buffer;
}


int connect_to_port(int port){
    int sock = 0;
    struct sockaddr_in serv_addr;
    fd_set read_fds;
    struct timeval tv;
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }
    
    serv_addr.sin_family = AF_INET;
    if(port == 0){
        serv_addr.sin_port = htons(CONTROL_PORT);
    }
    else{
        serv_addr.sin_port = htons(DATA_PORT);
    }

    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    tv.tv_sec = 5;
    tv.tv_usec = 0;

    FD_ZERO(&read_fds);
    FD_SET(sock, &read_fds);
    
    int ret = connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (ret < 0) {
        if (errno == EINPROGRESS) {
            // Подключение еще не завершено, ожидаем ответ
            ret = select(sock + 1, &read_fds, NULL, NULL, &tv);
            if (ret > 0) {
                // Подключение успешно завершено
                printf("Connected to server.\n");
            } else if (ret == 0) {
                // Таймаут истек
                printf("Server is not responding. Timeout reached.\n");
            } else {
                perror("select");
            }
        } else {
            perror("connect");
        }
        close(sock);
        return -1;
    }

    return sock;
}


char * get_first_parameter(char * buffer){
    char * parameter;
    int parameter_size = 1, i = 0, j = 0;
    parameter = (char*)malloc(parameter_size*sizeof(char));
    parameter[0] = '\0';

    for(i; buffer[i] != ' ' && buffer[i] != '\0'; i++);
    i++;
    for(i; buffer[i] != ' ' && buffer[i] != '\0'; i++){
        parameter[j] = buffer[i];
        j++;
        parameter_size++;
        parameter = (char*)realloc(parameter, parameter_size*sizeof(char));
        parameter[j] = '\0';
    }
    return parameter;
}



char * get_command(char * buffer){
    char * command;
    int command_size = 1;
    command = (char *)malloc(command_size*sizeof(char));
    if (command == NULL) {
        return NULL;
    }
    command[0] = '\0'; // Инициализация первого символа как нулевой символ

    for(int i = 0; buffer[i] != ' ' && buffer[i] != '\0'; i++){
        command[i] = buffer[i];
        command_size++;
        command = (char *)realloc(command, command_size*sizeof(char));
        if (command == NULL) {
            return NULL;
        }
    }
    command[command_size - 1] = '\0'; // Добавление нулевого символа в конец строки
    return command;
}

int main() {
    int sock = connect_to_port(0);
    if(sock == -1){
        return 0;
    }
    fd_set readfds;
    struct timeval timeout;
    char buffer[1024], data_buffer[1024];
    while(1){
        fprintf(stderr, "Enter command: ");
        char * command;
        int command_size = 1;
        command = (char *)malloc(command_size*sizeof(char));
        for(int i = 0; (command[i] = getchar()) != '\n'; i++){
            command_size++;
            command = (char *)realloc(command, command_size*sizeof(char));
        }
        command[command_size-1] = '\0';
        fprintf(stderr, "\n");
        write(sock, command, strlen(command));


        read(sock, buffer, 1024);
        fprintf(stderr, "%s\n\n", buffer);

        
        if(strcmp(command, "QUIT") == 0 && strstr(buffer, "503 Bad sequence of commands") == 0){
            return 0;
        }
        else if(strstr(buffer, "350 Requested file action pending further information") != NULL){
            while(strstr(buffer, "250 Requested file action okay, completed") == NULL){
                free(command);
                fprintf(stderr, "Enter command: ");
                command_size = 1;
                command = (char *)malloc(command_size*sizeof(char));
                for(int i = 0; (command[i] = getchar()) != '\n'; i++){
                    command_size++;
                    command = (char *)realloc(command, command_size*sizeof(char));
                }
                command[command_size-1] = '\0';
                write(sock, command, strlen(command));

                read(sock, buffer, 1024);
                fprintf(stderr, "%s\n\n", buffer);
            }
        }
        else if(strcmp(command, "LIST") == 0 && strstr(buffer, "150") != NULL){
            int data_socket = connect_to_port(1);
            write(data_socket, "1", 1);
            read(data_socket, data_buffer, 1024);
            if(data_buffer[0] != '\0'){
                fprintf(stderr, " \nLIST: \n");
            }
            while(data_buffer[0] != '\0'){
                fprintf(stderr, "%s", data_buffer);
                write(data_socket, "1", 1);
                read(data_socket, data_buffer, 1024);
            }
            fprintf(stderr, "\n");
            read(sock, buffer, 1024);
            fprintf(stderr, "%s\n\n", buffer);
            close(data_socket);
        }
        else if(strstr(command, "RETR") != NULL && strstr(buffer, "150") != NULL){
            unsigned char * data_buffer = (unsigned char *)malloc(BYTES_TO_SEND_AND_TO_GET*sizeof(unsigned char)); 
            char * parameter = get_first_parameter(command);
            int param_size = 0;
            for(param_size; parameter[param_size] != '\0'; param_size++);
            char * file_name = (char *)malloc((22 + param_size) * sizeof(char));
            snprintf(file_name, 68 + param_size, "%s%s", DOWNLOADS_FOLDER, parameter);

            int data_socket = connect_to_port(1);


            //Приимаем полный размер файла
            write(sock, "1", 1);
            unsigned char file_size_buffer[sizeof(size_t)];
            read(sock, file_size_buffer, sizeof(file_size_buffer));
            size_t file_size_in_bytes;
            memcpy(&file_size_in_bytes, file_size_buffer, sizeof(size_t));
            size_t loading = 0;
            write(sock, "1", 1);
            

            FILE * file = fopen(file_name, "wb");
            if (file < 0) {
                perror("Error opening file");
                exit(1);
            }
            while(1){
                unsigned char size_buffer[sizeof(size_t)];
                read(sock, size_buffer, sizeof(size_buffer));
                size_t data_buffer_size;
                memcpy(&data_buffer_size, size_buffer, sizeof(size_t));
                write(sock, "1", 1); // - подтверждение

                size_t readBytes;
                while(1){
                    readBytes = read(data_socket, data_buffer, data_buffer_size);
                    write(sock, "1", 1); // - подтверждение
                    read(sock, buffer, 1); // - подтверждение
                    if(buffer[0] == '0'){
                        continue;
                    }
                    else{
                        if(readBytes != data_buffer_size){
                            write(sock, "0", 1); // - подтверждение
                            continue;
                        }
                        else{
                            loading += readBytes;
                            fprintf(stderr, "%.3fmb / %.3fmb\n", (float)(loading / (1024.000*1024.000)), (float)(file_size_in_bytes / (1024.000*1024.000)));
                            write(sock, "1", 1); // - подтверждение
                            break;
                        }
                    }
                }
                while(1){
                    size_t fwriteBytes = fwrite(data_buffer, sizeof(unsigned char), readBytes, file);
                    if(fwriteBytes == readBytes){
                        break;
                    }
                }

                data_buffer = clear_buffer(BYTES_TO_SEND_AND_TO_GET);
                write(sock, "1", 1);
                read(sock, buffer, 1024);
                if(strstr(buffer, "226 Closing data connection") != NULL){
                    fprintf(stderr, "%s\n\n", buffer);
                    break;
                }
                else{
                    write(sock, "1", 1);
                }
            }
            free(data_buffer);
            fclose(file);
            close(data_socket);      
        }
        else if(strstr(command, "STOR") != NULL && strstr(buffer, "150") != NULL){
            char * parameter = get_first_parameter(command);
            struct stat st;
            size_t file_size_in_bytes;

            if (access(parameter, F_OK) == 0 && stat(parameter, &st) == 0 && S_ISREG(st.st_mode)) {
                file_size_in_bytes = (size_t)st.st_size;
                size_t loading_max = file_size_in_bytes;
                size_t loading = 0;
                write(sock, "1", 1);
                read(sock, buffer, 1);

                int data_socket = connect_to_port(1);

                read(sock, buffer, 1);
                unsigned char file_size_buffer[sizeof(size_t)];
                memcpy(file_size_buffer, &file_size_in_bytes, sizeof(size_t));
                write(sock, file_size_buffer, sizeof(file_size_buffer));

                read(sock, buffer, 1);
                unsigned char * data_buffer = (unsigned char *)malloc(BYTES_TO_SEND_AND_TO_GET*sizeof(unsigned char));
                FILE * file = fopen(parameter, "rb");
                if (file < 0) {
                    perror("Error opening file");
                    exit(1);
                }
                while(1){
                    size_t freadBytes;
                    while(1){
                        freadBytes = fread(data_buffer, sizeof(unsigned char), BYTES_TO_SEND_AND_TO_GET, file);
                        if(file_size_in_bytes < BYTES_TO_SEND_AND_TO_GET && freadBytes == file_size_in_bytes || 
                        file_size_in_bytes >= BYTES_TO_SEND_AND_TO_GET && freadBytes == BYTES_TO_SEND_AND_TO_GET){
                            break;
                        }
                    }
                    file_size_in_bytes -= freadBytes;
                    
                    loading += freadBytes;
                    fprintf(stderr, "%.3fmb / %.3fmb\n", (float)(loading / (1024.000*1024.000)), (float)(loading_max / (1024.000*1024.000)));

                    unsigned char size_buffer[sizeof(size_t)];
                    memcpy(size_buffer, &freadBytes, sizeof(size_t));
                    write(sock, size_buffer, sizeof(size_buffer));
                    read(sock, buffer, 1); // - подтверждение

                    while(1){
                        size_t writeBytes = write(data_socket, data_buffer, freadBytes);
                        read(sock, buffer, 1); // - подтверждение
                        if(writeBytes != freadBytes){
                            write(sock, "0", 1); // - подтверждение
                            continue;
                        }
                        else{
                            write(sock, "1", 1);
                            read(sock, buffer, 1);
                            if(buffer[0] == '0'){
                                continue;
                            }
                            else{
                                break;
                            }
                        }
                    }

                    data_buffer = clear_buffer(BYTES_TO_SEND_AND_TO_GET);
                    read(sock, buffer, 1);
                    if(feof(file)){
                        write(sock, "1", 1);
                        read(sock, buffer, 1024);
                        fprintf(stderr, "%s\n\n", buffer);
                        break;
                    }
                    else{
                        write(sock, "0", 1);
                        read(sock, buffer, 1);
                    }
                }

                fclose(file);
                close(data_socket);
            }
            else{
                write(sock, "0", 1);
                read(sock, buffer, 1024);
                fprintf(stderr, "%s\n\n", buffer);
            }
        }
    }
    close(sock);
    return 0;
}