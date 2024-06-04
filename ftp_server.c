#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <limits.h>

#define CONTROL_PORT 2121
#define DATA_PORT 2020

const char * MESSAGE_150 = "150 File status okay; about to open data connection";
const char * MESSAGE_221 = "221 Service closing control connection";
const char * MESSAGE_226 = "226 Closing data connection";
const char * MESSAGE_230 = "230 User logged in, proceed";
const char * MESSAGE_250 = "250 Directory successfully changed";
const char * MESSAGE_250_1 = "250 Requested file action okay, completed";
const char * MESSAGE_257 = "257 Current directory is \"";
const char * MESSAGE_331 = "331 User name okay, need password";
const char * MESSAGE_350 = "350 Requested file action pending further information";
const char * MESSAGE_500 = "500 Syntax error, command urecognized";
const char * MESSAGE_501 = "501 Syntax error in parameters or arguments";
const char * MESSAGE_503 = "503 Bad sequence of commands";
const char * MESSAGE_530 = "530 Not logged in";
const char * MESSAGE_550 = "550 Requested action not taken";
const char * ALL_COMMANDS[13] = {"USER", "PASS", "PWD", "CWD", "LIST", "QUIT", "CDUP", "DELE", "RNFR", "RETR", "STOR", "ZIP", "UZIP"};
const int AMMOUNT_OF_COMMANDS = 13;
const size_t BYTES_TO_SEND_AND_TO_GET = 36864;


char * clear_buffer(int buffer_size){
    char * buffer = (char *)malloc(buffer_size*sizeof(char));
    for(int i = 0; i < buffer_size; i++){
        buffer[i] = '\0';
    }
    return buffer;
}

int * initiallize_new_socket(int port){
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    int * socket_data = (int *)malloc(2*sizeof(int));

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    if(port == 0){
        address.sin_port = htons(CONTROL_PORT);
    }
    else{
        address.sin_port = htons(DATA_PORT);
    }
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    socket_data[0] = server_fd;
    socket_data[1] = new_socket;
    return socket_data;
}


int check_if_command_exists(char * command){
    for(int i = 0; i < AMMOUNT_OF_COMMANDS; i++){
        if(strcmp(ALL_COMMANDS[i], command) == 0){
            return 0;
        }
    }
    return -1;
}

char* get_command_name(char* buffer) {
    char* command = NULL;
    int command_size = 0;
    int i = 0;

    while (buffer[i] != ' ' && buffer[i] != '\0') {
        command_size++;
        char* temp = realloc(command, (command_size + 1) * sizeof(char));
        if (temp == NULL) {
            free(command);
            return NULL;
        }
        command = temp;
        command[i] = buffer[i];
        i++;
    }

    if (command != NULL) {
        command[command_size] = '\0';
    }

    return command;
}



int get_ammount_of_parameters(char * buffer){
    int ammount_of_parameters = 0;
    for(int i = 0; buffer[i] != '\0'; i++){
        if(buffer[i] == ' ' && buffer[i+1] != '\0' && buffer[i+1] != ' '){
            ammount_of_parameters++;
        }
    }
    return ammount_of_parameters;
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


/// @brief Функция, которая ищет имя пользователя в файле "user_info.txt"
/// @param user_name - имя пользователя
/// @return int: "0" - если пользователь найден успешно, "-1" - если пользователь не найден
int find_user_name(char * user_name){
    FILE * fl = fopen("user_info.txt", "rt");
    char c;

    while(1){
        long unsigned int name_size = 1;
        char * name = (char *)malloc(name_size*sizeof(char));
        name[0] = '\0';
        int i = 0;

        while((c = fgetc(fl)) != EOF && c != ' '){
            name[i] = c;
            name_size++;
            i++;
            name = (char *)realloc(name, name_size*sizeof(char));
            name[name_size-1] = '\0';
        }

        if(strcmp(name, user_name) == 0){
            fclose(fl);
            return 0;
        }

        while((c = fgetc(fl)) != EOF && c != '\n');

        if(c == EOF){
            fclose(fl);
            return -1;
        }
    }
}


/// @brief Выдает уровень доступа пользователя
/// @param user_name - имя пользователя
/// @return Значение означающее уровень доступа
int find_pass_level(char * user_name){
    FILE * fl = fopen("user_info.txt", "rt");
    char c;

    while(1){
        long unsigned int name_size = 1;
        char * name = (char *)malloc(name_size*sizeof(char));
        name[0] = '\0';
        int i = 0;

        while((c = fgetc(fl)) != EOF && c != ' '){
            name[i] = c;
            name_size++;
            i++;
            name = (char *)realloc(name, name_size*sizeof(char));
            name[name_size-1] = '\0';
        }

        if(strcmp(name, user_name) == 0){
            while((c = fgetc(fl)) != EOF && c != ' ');
            c = fgetc(fl);
            fclose(fl);
            return c;
        }

        while((c = fgetc(fl)) != EOF && c != '\n');

        if(c == EOF){
            fclose(fl);
            return -1;
        }
    }
}


/// @brief Функция, которая проверяет правильность введенного пароля (user_passord) для вошедшего пользователя (user_name) используя файл "user_info.txt"
/// @param user_name - имя вошедшего пользователя
/// @param user_password - пароль пользователя
/// @return int: "0" - если пароль введен правильно, "-1" - если пароль введен неправильно
int find_user_password(char * user_name, char * user_password){
    FILE * fl = fopen("user_info.txt", "rt");
    char c;

    while(1){
        long unsigned int name_size = 1;
        char * name = (char *)malloc(name_size*sizeof(char));
        name[0] = '\0';
        int i = 0;

        while((c = fgetc(fl)) != EOF && c != ' '){
            name[i] = c;
            name_size++;
            i++;
            name = (char *)realloc(name, name_size*sizeof(char));
            name[name_size-1] = '\0';
        }

        if(strcmp(name, user_name) == 0){
            long unsigned int password_size = 1;
            char * password = (char *)malloc(password_size*sizeof(char));
            password[0] = '\0';
            int i = 0;

            while((c = fgetc(fl)) != EOF && c != ' '){
                password[i] = c;
                password_size++;
                i++;
                password = (char *)realloc(password, password_size*sizeof(char));
                password[password_size-1] = '\0';
            }

            if(strcmp(password, user_password) == 0){
                fclose(fl);
                return 0;
            }
            fclose(fl);
            return -1;
        }

        while((c = fgetc(fl)) != EOF && c != '\n');

        if(c == EOF){
            fclose(fl);
            return -1;
        }
    }
}


/// @brief Проверяем существует ли данный пользователь в системе
/// @param buffer - строка хранящая данные, полученные от пользователя
/// @param socket - дескриптор сокета
/// @return число, которое мы добавляем к переменной "working_stage", чтобы перейти к следующей стадии работы
int user_command_process(char * user_name, char * buffer, int socket){

    if(get_ammount_of_parameters(buffer) != 1){
        write(socket, MESSAGE_501, 44);
        return 0;
    }
    else if(find_user_name(user_name) == 0){
        write(socket, MESSAGE_331, 34);
        return 1;
    }
    else{
        write(socket, MESSAGE_530, 18);
        return 0;
    }
}


/// @brief Проверяем пароль вошедшего клиента
/// @param user_name - имя вошедшего пользователя
/// @param buffer - строка хранящая данные, полученные от пользователя
/// @param socket - дескриптор сокета
/// @return число, которое мы добавляем к переменной "working_stage", чтобы перейти к следующей стадии работы
int pass_command_process(char * user_name, char * buffer, int socket){
    if(get_ammount_of_parameters(buffer) != 1){
        write(socket, MESSAGE_501, 44);
        return 0;
    }
    char * user_password = get_first_parameter(buffer);
    if(find_user_password(user_name, user_password) == 0){
        write(socket, MESSAGE_230, 28);
        char c = find_pass_level(user_name);
        int pass_level = c - '0'; 
        return pass_level;
    }
    else{
        write(socket, MESSAGE_530, 18);
        free(user_password);
        return 0;
    }
}


/// @brief Отправляем клиенту директорию, в которой мы работаем 
/// @param current_directory - путь текущей директории относительно папки сервера
/// @param buffer - строка хранящая данные, полученные от пользователя
/// @param cur_dir_size - размер строки "current_directory"
/// @param socket - дескриптор сокета
void pwd_command_process(char * current_directory, char * buffer, int cur_dir_size, int socket){
    if(get_ammount_of_parameters(buffer) != 0){
        write(socket, MESSAGE_501, 44);
        return;
    }
    int final_message_size = 28 + cur_dir_size;
    char * final_message = (char *)malloc(final_message_size*sizeof(char));
    snprintf(final_message, final_message_size, "%s%s%c", MESSAGE_257, current_directory, '\"');
    write(socket, final_message, final_message_size);
    free(final_message);
}


/// @brief Меняем текущую директорию
/// @param current_directory - путь текущей директории относительно папки сервера
/// @param buffer - строка хранящая данные, полученные от пользователя
/// @param cur_dir_size - размер строки "current_directory"
/// @param socket - дескриптор сокета
/// @return Возвращает итоговое значение текущей директории 
char * cwd_command_process(char * current_directory, char * buffer, char * SERVER_MAIN_PATH, int SERVER_MAIN_PATH_SIZE, int * cur_dir_size, int socket){
    if(get_ammount_of_parameters(buffer) != 1){
        write(socket, MESSAGE_501, 44);
        fprintf(stderr, "ammount\n");
        return current_directory;
    }    

    char * parameter = get_first_parameter(buffer);

    if(strstr(parameter, "..") != NULL){
        write(socket, MESSAGE_501, 44);
        fprintf(stderr, "dots\n");
        return current_directory;
    }

    if(parameter[0] == '.'){
        char * path;
        int path_size = 1;
        path = (char *)malloc(path_size*sizeof(char));
        path[0] = '\0';

        for(int i = 0; parameter[i+2] != '\0'; i++){
            path[i] = parameter[i+2];
            path_size++;
            path = (char *)realloc(path, path_size*sizeof(char));
            path[path_size -1] = '\0';
        }

        char * final_path = (char*)malloc(((*cur_dir_size) + path_size)*sizeof(char));
        snprintf(final_path, (*cur_dir_size) + path_size, "%s%s", current_directory, path);
        printf("%s\n", final_path);

        char * full_path = (char *)malloc(((*cur_dir_size) + path_size + SERVER_MAIN_PATH_SIZE)*sizeof(char));
        snprintf(full_path, (*cur_dir_size) + path_size + SERVER_MAIN_PATH_SIZE, "%s%s", SERVER_MAIN_PATH, final_path);
        printf("%s\n", full_path);

        struct stat path_stat;
        stat(full_path, &path_stat);

        if (S_ISDIR(path_stat.st_mode)) {
            write(socket, MESSAGE_250, 44);
            *cur_dir_size = *cur_dir_size + path_size;

            printf("\n%s\n", final_path);
            int size = 0;
            for(size; parameter[size] != '\0'; size++);
            if(parameter[size-2] != '/'){
                *cur_dir_size = *cur_dir_size + 1;
                final_path = (char *)realloc(final_path, *cur_dir_size * sizeof(char));
                final_path[*cur_dir_size-1] = '\0';
                final_path[*cur_dir_size-2] = '/';
            }
            printf("%s\n", final_path);
            free(parameter);
            return final_path;
        } 
        else {
            write(socket, MESSAGE_550, 44);
            free(parameter);
            return current_directory;
        }
    }
    else if(parameter[0] == '/'){
        char * full_path = (char *)malloc((strlen(parameter) + SERVER_MAIN_PATH_SIZE)*sizeof(char));
        snprintf(full_path, strlen(parameter) + SERVER_MAIN_PATH_SIZE, "%s%s", SERVER_MAIN_PATH, parameter);
        printf("%s\n", full_path);

        struct stat path_stat;
        stat(full_path, &path_stat);

        if (S_ISDIR(path_stat.st_mode)) {
            write(socket, MESSAGE_250, 44);
            *cur_dir_size = strlen(parameter);

            printf("\n%s\n", parameter);
            int size = 0;
            for(size; parameter[size] != '\0'; size++);
            if(parameter[size-2] != '/'){
                *cur_dir_size = *cur_dir_size + 1;
                parameter = (char *)realloc(parameter, *cur_dir_size * sizeof(char));
                parameter[*cur_dir_size-1] = '\0';
                parameter[*cur_dir_size-2] = '/';
            }
            printf("%s\n", parameter);
            return parameter;
        } 
        else {
            write(socket, MESSAGE_550, 44);
            free(parameter);
            return current_directory;
        }
    }
    else{
        char * final_path = (char*)malloc(((*cur_dir_size) + 1 + strlen(parameter))*sizeof(char));
        snprintf(final_path, (*cur_dir_size) + strlen(parameter) + 1, "%s%s", current_directory, parameter);
        printf("%s\n", final_path);

        char * full_path = (char *)malloc(((*cur_dir_size) + strlen(parameter) + 1 + SERVER_MAIN_PATH_SIZE)*sizeof(char));
        snprintf(full_path, (*cur_dir_size) + strlen(parameter) + 1 + SERVER_MAIN_PATH_SIZE, "%s%s", SERVER_MAIN_PATH, final_path);
        printf("%s\n", full_path);

        struct stat path_stat;
        stat(full_path, &path_stat);

        if (S_ISDIR(path_stat.st_mode)) {
            *cur_dir_size = *cur_dir_size + 1 + strlen(parameter);

            printf("\n%s\n", final_path);
            int size = 0;
            for(size; parameter[size] != '\0'; size++);
            if(parameter[size-2] != '/'){
                *cur_dir_size = *cur_dir_size + 1;
                final_path = (char *)realloc(final_path, *cur_dir_size * sizeof(char));
                final_path[*cur_dir_size-1] = '\0';
                final_path[*cur_dir_size-2] = '/';
            }
            printf("%s\n", final_path);

            write(socket, MESSAGE_250, 44);
            free(parameter);
            return final_path;
        } 
        else {
            write(socket, MESSAGE_550, 44);
            free(parameter);
            return current_directory;
        }
    }
    return current_directory;
}


/// @brief Меняет текущую директорию, на родительскую
/// @param current_directory - путь текущей директории относительно папки сервера
/// @param buffer - строка хранящая данные, полученные от пользователя
/// @param cur_dir_size - размер строки "current_directory"
/// @param socket - дескриптор сокета
/// @return Возвращает новое значение текущей директории 
char * cdup_command_process(char * current_directory, char * buffer, int * cur_dir_size, int socket){
    if(get_ammount_of_parameters(buffer) != 0){
        write(socket, MESSAGE_501, 44);
        return current_directory;
    }

    if(strcmp(current_directory, "/") == 0){
        write(socket, MESSAGE_550, 44);
        return current_directory;
    }
    else{
        int i = *cur_dir_size-3;
        for(i; current_directory[i] != '/'; i--);
        char * new_current_directory = (char *)malloc((i+2)*sizeof(char));
        for(int j = 0; j < i+1; j++){
            new_current_directory[j] = current_directory[j];
        }
        new_current_directory[i+1] = '\0';
        *cur_dir_size = i + 2;

        write(socket, MESSAGE_250, 44);
        return new_current_directory;
    }
}


/// @brief Выводим список файлов текущей директории клиента
/// @param current_directory - путь текущей директории относительно папки сервера
/// @param cur_dir_size - размер строки "current_directory"
/// @param socket - дескриптор сокета
void list_command_process(char * current_directory, char * buffer, char * SERVER_MAIN_PATH, int SERVER_MAIN_PATH_SIZE, int cur_dir_size, int socket){
    if(get_ammount_of_parameters(buffer) != 0){
        write(socket, MESSAGE_501, 44);
        return;
    }

    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    char file_info[1024];
    char * full_cur_dir = '\0';
    full_cur_dir = (char *)malloc((cur_dir_size + SERVER_MAIN_PATH_SIZE)*sizeof(char));
    snprintf(full_cur_dir, cur_dir_size + SERVER_MAIN_PATH_SIZE, "%s%s", SERVER_MAIN_PATH, current_directory);

    write(socket, MESSAGE_150, 52);

    int * data_socket = initiallize_new_socket(1);
    dir = opendir(full_cur_dir);
    if (dir == NULL) {
        perror("Ошибка при открытии директории");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
            continue;
        }

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", full_cur_dir, entry->d_name);

        if (stat(full_path, &file_stat) == 0) {
            
            const char * type;
            struct tm *time = localtime(&file_stat.st_mtime);
            if(S_ISDIR(file_stat.st_mode)){
                type = "dir"; 
            }
            else{
                type = "file";
            }

            int file_info_size = strlen(entry->d_name) + 20 + strlen(type) + strlen(asctime(time)) + 3;
            snprintf(file_info, file_info_size, "%s %ld %s %s", entry->d_name, file_stat.st_size, type, asctime(time));
            char ready[1];
            read(data_socket[1], ready, 1);
            fprintf(stderr, "%s", file_info);
            write(data_socket[1], file_info, file_info_size);
        }
    }
    write(data_socket[1], clear_buffer(1024), 1024);
    write(socket, MESSAGE_226, 28);
    close(data_socket[0]);
    close(data_socket[1]);
    closedir(dir);
    free(full_cur_dir);
    free(data_socket);
}


/// @brief Удаляет файл/директорию
/// @param current_directory - путь текущей директории относительно папки сервера
/// @param buffer - строка хранящая данные, полученные от пользователя
/// @param cur_dir_size - размер строки "current_directory"
/// @param socket - дескриптор сокета
void dele_command_process(char * current_directory, char * buffer, char * SERVER_MAIN_PATH, int SERVER_MAIN_PATH_SIZE, int cur_dir_size, int socket){
    if(get_ammount_of_parameters(buffer) != 1){
        write(socket, MESSAGE_501, 44);
        return;
    }

    char * parameter = get_first_parameter(buffer);
    if(strstr(parameter, "/") != 0){
        write(socket, MESSAGE_501, 44);
        return;
    }


    int param_size = 0;
    for(param_size; parameter[param_size] != '\0'; param_size++);
    char * path_to_delete = (char *)malloc((SERVER_MAIN_PATH_SIZE + 1 + cur_dir_size + param_size) * sizeof(char));
    snprintf(path_to_delete, SERVER_MAIN_PATH_SIZE + 1 + cur_dir_size + param_size, "%s%s%s", SERVER_MAIN_PATH, current_directory, parameter);
    
    int status = remove(path_to_delete);
    if (status == 0) {
        write(socket, MESSAGE_250_1, 42);
    } 
    else {
        write(socket, MESSAGE_550, 44);
    }
    free(parameter);
    free(path_to_delete);
}


/// @brief Проверяет, существует ли файл, который мы хотим переименовать
/// @param current_directory - путь текущей директории относительно папки сервера
/// @param buffer - строка хранящая данные, полученные от пользователя
/// @param cur_dir_size - размер строки "current_directory"
/// @param socket - дескриптор сокета
/// @return Возвращает итоговый путь к файлу/директории для переименования, если он/она был(а) найден(а). Иначе - NULL
char * rnfr_command_process(char * current_directory, char * buffer, char * SERVER_MAIN_PATH, int SERVER_MAIN_PATH_SIZE, int cur_dir_size, int socket){
    if(get_ammount_of_parameters(buffer) != 1){
        write(socket, MESSAGE_501, 44);
        return NULL;
    }

    char * parameter = get_first_parameter(buffer);
    if(strstr(parameter, "/") != 0){
        write(socket, MESSAGE_501, 44);
        return NULL;
    }

    int param_size = 0;
    for(param_size; parameter[param_size] != '\0'; param_size++);
    char * path_to_check = (char *)malloc((SERVER_MAIN_PATH_SIZE + 1 + cur_dir_size + param_size) * sizeof(char));
    snprintf(path_to_check, SERVER_MAIN_PATH_SIZE + 1 + cur_dir_size + param_size, "%s%s%s", SERVER_MAIN_PATH, current_directory, parameter);
    fprintf(stderr, "%s\n", path_to_check);

    if (access(path_to_check, F_OK) == 0) {
        write(socket, MESSAGE_350, 54);
        free(parameter);
        return path_to_check;
    } 
    else {
        write(socket, MESSAGE_550, 44);
        free(parameter);
        free(path_to_check);
        return NULL;
    }
}


/// @brief Переименовывает файл
/// @param path_to_rename - путь к файлу/директории для переименования
/// @param current_directory - путь текущей директории относительно папки сервера
/// @param buffer - строка хранящая данные, полученные от пользователя
/// @param cur_dir_size - размер строки "current_directory"
/// @param socket - дескриптор сокета
int rnto_command_process(char * path_to_rename, char * current_directory, char * buffer, char * SERVER_MAIN_PATH, int SERVER_MAIN_PATH_SIZE, int cur_dir_size, int socket){
    if(get_ammount_of_parameters(buffer) != 1){
        write(socket, MESSAGE_501, 44);
        return -1;
    }

    char * parameter = get_first_parameter(buffer);
    if(strstr(parameter, "/") != 0){
        write(socket, MESSAGE_501, 44);
        return -1;
    }

    int param_size = 0;
    for(param_size; parameter[param_size] != '\0'; param_size++);
    char * new_name = (char *)malloc((SERVER_MAIN_PATH_SIZE + 1 + cur_dir_size + param_size) * sizeof(char));
    snprintf(new_name, SERVER_MAIN_PATH_SIZE + 1 + cur_dir_size + param_size, "%s%s%s", SERVER_MAIN_PATH, current_directory, parameter);
 
    int status = rename(path_to_rename, new_name);
    if (status == 0) {
        write(socket, MESSAGE_250_1, 42);
        free(parameter);
        free(new_name);
        return 0;
    } 
    else {
        write(socket, MESSAGE_550, 44);
        free(parameter);
        free(new_name);
        return -1;
    }
}


/// @brief Отправляет файл клиенту
/// @param current_directory - путь текущей директории относительно папки сервера
/// @param buffer - строка хранящая данные, полученные от пользователя
/// @param cur_dir_size - размер строки "current_directory"
/// @param socket - дескриптор сокета
void retr_command_process(char * current_directory, char * buffer, char * SERVER_MAIN_PATH, int SERVER_MAIN_PATH_SIZE, int cur_dir_size, int socket){
    if(get_ammount_of_parameters(buffer) != 1){
        write(socket, MESSAGE_501, 44);
        return;
    }

    char * parameter = get_first_parameter(buffer);
    if(strstr(parameter, "/") != 0){
        write(socket, MESSAGE_501, 44);
        return;
    }

    struct stat st;
    size_t file_size_in_bytes;

    int param_size = 0;
    for(param_size; parameter[param_size] != '\0'; param_size++);
    char * file_name = (char *)malloc((SERVER_MAIN_PATH_SIZE + 1 + cur_dir_size + param_size) * sizeof(char));
    snprintf(file_name, SERVER_MAIN_PATH_SIZE + 1 + cur_dir_size + param_size, "%s%s%s", SERVER_MAIN_PATH, current_directory, parameter);

    if (access(file_name, F_OK) == 0 && stat(file_name, &st) == 0 && S_ISREG(st.st_mode)) {
        file_size_in_bytes = (size_t)st.st_size;
        size_t loading_max = file_size_in_bytes;
        size_t loading = 0;
        write(socket, MESSAGE_150, 52);

        int * data_socket = initiallize_new_socket(1);

        read(socket, buffer, 1);
        unsigned char file_size_buffer[sizeof(size_t)];
        memcpy(file_size_buffer, &file_size_in_bytes, sizeof(size_t));
        write(socket, file_size_buffer, sizeof(file_size_buffer));
        read(socket, buffer, 1);

        unsigned char * data_buffer = (unsigned char *)malloc(BYTES_TO_SEND_AND_TO_GET*sizeof(unsigned char));
        FILE * file = fopen(file_name, "rb"); // или .png, .pdf
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
            write(socket, size_buffer, sizeof(size_buffer));
            read(socket, buffer, 1); // - подтверждение

            while(1){
                size_t writeBytes = write(data_socket[1], data_buffer, freadBytes);
                read(socket, buffer, 1); // - подтверждение
                if(writeBytes != freadBytes){
                    write(socket, "0", 1); // - подтверждение
                    continue;
                }
                else{
                    write(socket, "1", 1); // - подтверждение
                    read(socket, buffer, 1); // - подтверждение
                    if(buffer[0] == '0'){
                        continue;
                    }
                    else{
                        break;
                    }
                }
            }

            data_buffer = clear_buffer(BYTES_TO_SEND_AND_TO_GET);
            read(socket, buffer, 1);
            if(feof(file)){
                write(socket, MESSAGE_226, 28);
                break;
            }
            else{
                write(socket, "1", 1);
                read(socket, buffer, 1);
            }
        }

        fclose(file);
        close(data_socket[0]);
        close(data_socket[1]);
        free(data_socket);
        free(data_buffer);
    } 
    else {
        write(socket, MESSAGE_550, 44);
        free(parameter);
        free(file_name);
        return;
    }
}


/// @brief Загружает файл от клиента
/// @param current_directory - путь текущей директории относительно папки сервера
/// @param buffer - строка хранящая данные, полученные от пользователя
/// @param cur_dir_size - размер строки "current_directory"
/// @param socket - дескриптор сокета
void stor_command_process(char * current_directory, char * buffer, char * SERVER_MAIN_PATH, int SERVER_MAIN_PATH_SIZE, int cur_dir_size, int socket){
    if(get_ammount_of_parameters(buffer) != 1){
        write(socket, MESSAGE_501, 44);
        return;
    }

    char * parameter = get_first_parameter(buffer);
    if(strstr(parameter, "..") != 0){
        write(socket, MESSAGE_501, 44);
        return;
    }

    unsigned char * data_buffer = (unsigned char *)malloc(BYTES_TO_SEND_AND_TO_GET*sizeof(unsigned char));
    int param_size = 0;
    for(param_size; parameter[param_size] != '\0'; param_size++);
    
    int filename_size = 1;
    char * filename = (char *)malloc(filename_size*sizeof(char));
    filename[0] = '\0';
    int i;
    for(i = param_size - 1; parameter[i] != '/'; i--);
    int j = 0;
    for(i = i + 1; parameter[i] != '\0'; i++){
        filename[j] = parameter[i];
        j++;
        filename_size++;
        filename = (char *)realloc(filename, filename_size*sizeof(char));
        filename[filename_size-1] = '\0';
    }
    fprintf(stderr, "%s\n", filename);

    char * file_path = (char *)malloc((SERVER_MAIN_PATH_SIZE + 1 + cur_dir_size + filename_size) * sizeof(char));
    snprintf(file_path, SERVER_MAIN_PATH_SIZE + 1 + cur_dir_size + filename_size, "%s%s%s", SERVER_MAIN_PATH, current_directory, filename);
    write(socket, MESSAGE_150, 52);
    read(socket, buffer, 1);

    int * data_socket;
    if(buffer[0] == '1'){
        write(socket, "1", 1);
        data_socket = initiallize_new_socket(1);
    }
    else{
        write(socket, MESSAGE_550, 44);
        return;
    }

    write(socket, "1", 1);
    unsigned char file_size_buffer[sizeof(size_t)];
    read(socket, file_size_buffer, sizeof(file_size_buffer));
    size_t file_size_in_bytes;
    memcpy(&file_size_in_bytes, file_size_buffer, sizeof(size_t));
    size_t loading = 0;
    
    FILE * file = fopen(file_path, "wb");
    if (file < 0) {
        perror("Error opening file");
        exit(1);
    }

    while(1){
        unsigned char size_buffer[sizeof(size_t)];
        write(socket, "1", 1); // - подтверждение
        read(socket, size_buffer, sizeof(size_buffer));
        size_t data_buffer_size;
        memcpy(&data_buffer_size, size_buffer, sizeof(size_t));
        write(socket, "1", 1); // - подтверждение

        size_t readBytes;
        while(1){
            readBytes = read(data_socket[1], data_buffer, data_buffer_size);
            write(socket, "1", 1); // - подтверждение
            read(socket, buffer, 1); // - подтверждение
            if(buffer[0] == '0'){
                continue;
            }
            else{
                if(readBytes != data_buffer_size){
                    write(socket, "0", 1); // - подтверждение
                    continue;
                }
                else{
                    loading += readBytes;
                    fprintf(stderr, "%.3fmb / %.3fmb\n", (float)(loading / (1024.000*1024.000)), (float)(file_size_in_bytes / (1024.000*1024.000)));
                    write(socket, "1", 1); // - подтверждение
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
        write(socket, "1", 1);
        read(socket, buffer, 1024);
        if(buffer[0] == '1'){
            write(socket, MESSAGE_226, 28);
            break;
        }
    }
    free(data_buffer);
    free(parameter);
    free(filename);
    free(file_path);
    fclose(file);
    close(data_socket[0]);
    close(data_socket[1]);
}


/// @brief Архивирует файлы/директории
/// @param current_directory - путь текущей директории относительно папки сервера
/// @param buffer - строка хранящая данные, полученные от пользователя
/// @param cur_dir_size - размер строки "current_directory"
/// @param socket - дескриптор сокета
void zip_command_process(char* current_directory, char* buffer, char* SERVER_MAIN_PATH, int SERVER_MAIN_PATH_SIZE, int cur_dir_size, int socket) {
    if (get_ammount_of_parameters(buffer) != 1) {
        write(socket, MESSAGE_501, 44);
        return;
    }

    char* parameter = get_first_parameter(buffer);
    if (strstr(parameter, "/") != 0) {
        write(socket, MESSAGE_501, 44);
        return;
    }

    int param_size = strlen(parameter);

    char* file_path = (char*)malloc((SERVER_MAIN_PATH_SIZE + 1 + cur_dir_size + param_size + 1) * sizeof(char));
    if (file_path == NULL) {
        perror("malloc failed");
        return;
    }
    snprintf(file_path, SERVER_MAIN_PATH_SIZE + 1 + cur_dir_size + param_size + 1, "%s%s%s", SERVER_MAIN_PATH, current_directory, parameter);

    char* final_command = NULL;
    struct stat path_stat;
    stat(file_path, &path_stat);
    if (S_ISDIR(path_stat.st_mode)) {
        param_size += 4;
        parameter = realloc(parameter, (param_size + 1) * sizeof(char));
        if (parameter == NULL) {
            perror("realloc failed");
            free(file_path);
            return;
        }
        snprintf(parameter + param_size - 4, 5, ".zip");

        char* zip_path = (char*)malloc((SERVER_MAIN_PATH_SIZE + 1 + cur_dir_size + param_size + 1) * sizeof(char));
        if (zip_path == NULL) {
            perror("malloc failed");
            free(file_path);
            return;
        }
        snprintf(zip_path, SERVER_MAIN_PATH_SIZE + 1 + cur_dir_size + param_size + 1, "%s%s", SERVER_MAIN_PATH, file_path);

        int file_path_size = strlen(file_path);

        final_command = (char*)malloc((19 + file_path_size + param_size + 1) * sizeof(char));
        if (final_command == NULL) {
            perror("malloc failed");
            free(file_path);
            free(zip_path);
            return;
        }
        snprintf(final_command, 19 + file_path_size + param_size + 1, "(cd %s && zip -r %s .)", file_path, zip_path);
        free(zip_path);
    } else {
        for (param_size = param_size - 1; parameter[param_size] != '.'; param_size--) {
            parameter[param_size] = '\0';
        }
        param_size += 4;
        parameter = realloc(parameter, (param_size + 1) * sizeof(char));
        if (parameter == NULL) {
            perror("realloc failed");
            free(file_path);
            return;
        }
        snprintf(parameter + param_size - 3, 4, "zip");

        int file_path_size = strlen(file_path);

        final_command = (char*)malloc((25 + file_path_size + param_size + 1) * sizeof(char));
        if (final_command == NULL) {
            perror("malloc failed");
            free(file_path);
            return;
        }
        snprintf(final_command, 25 + file_path_size + param_size + 1, "zip -j ./server_storage%s%s %s", current_directory, parameter, file_path);
    }

    fprintf(stderr, "%s\n", final_command);
    system(final_command);
    write(socket, MESSAGE_250_1, 42);
    free(parameter);
    free(file_path);
    free(final_command);
}


/// @brief Разархивирует файлы zip
/// @param current_directory - путь текущей директории относительно папки сервера
/// @param buffer - строка хранящая данные, полученные от пользователя
/// @param cur_dir_size - размер строки "current_directory"
/// @param socket - дескриптор сокета
void uzip_command_process(char* current_directory, char* buffer, char* SERVER_MAIN_PATH, int SERVER_MAIN_PATH_SIZE, int cur_dir_size, int socket) {
    if (get_ammount_of_parameters(buffer) != 1) {
        write(socket, MESSAGE_501, 44);
        return;
    }

    char* parameter = get_first_parameter(buffer);
    if (strstr(parameter, "/") != 0) {
        write(socket, MESSAGE_501, 44);
        return;
    }

    int param_size = strlen(parameter);
    char* zip_path = (char*)malloc((SERVER_MAIN_PATH_SIZE + 1 + cur_dir_size + param_size + 1) * sizeof(char));
    snprintf(zip_path, SERVER_MAIN_PATH_SIZE + 1 + cur_dir_size + param_size + 1, "%s%s%s", SERVER_MAIN_PATH, current_directory, parameter);

    char* command = (char*)malloc((SERVER_MAIN_PATH_SIZE + 1 + cur_dir_size + SERVER_MAIN_PATH_SIZE + 1 + cur_dir_size + param_size + 8) * sizeof(char));
    snprintf(command, SERVER_MAIN_PATH_SIZE + 1 + cur_dir_size + SERVER_MAIN_PATH_SIZE + 1 + cur_dir_size + param_size + 8, "unzip %s -d %s%s", zip_path, SERVER_MAIN_PATH, current_directory);
    fprintf(stderr, "%s\n", command);

    system(command);
    write(socket, MESSAGE_250_1, 42);

    free(parameter);
    free(zip_path);
    free(command);
}


void main_working_process(int socket, char * SERVER_MAIN_PATH, int SERVER_MAIN_PATH_SIZE){
    int working_stage = 0, cur_dir_size = 1;
    char * user_name, * current_directory;
    current_directory = (char *)malloc(cur_dir_size*sizeof(char));
    current_directory[0] = '/';
    

    while(1){
        char buffer[1024] = {0};
        read(socket, buffer, 1024);
        char * command = get_command_name(buffer);


        if(strcmp(command, "USER") == 0 && working_stage == 0){
            user_name = get_first_parameter(buffer);
            working_stage += user_command_process(user_name, buffer, socket);
        }
        else if(strcmp(command, "PASS") == 0 && working_stage == 1){
            working_stage += pass_command_process(user_name, buffer, socket);
        }
        else if(strcmp(command, "PWD") == 0 && working_stage >= 2){
            pwd_command_process(current_directory, buffer, cur_dir_size, socket);
        }
        else if(strcmp(command, "CWD") == 0 && working_stage >= 2){
            current_directory = cwd_command_process(current_directory, buffer, SERVER_MAIN_PATH, SERVER_MAIN_PATH_SIZE, &cur_dir_size, socket);
        }
        else if(strcmp(command, "CDUP") == 0 && working_stage >= 2){
            current_directory = cdup_command_process(current_directory, buffer, &cur_dir_size, socket);
        }
        else if(strcmp(command, "LIST") == 0 && working_stage >= 2){
            list_command_process(current_directory, buffer, SERVER_MAIN_PATH, SERVER_MAIN_PATH_SIZE, cur_dir_size, socket);
        }
        else if(strcmp(command, "DELE") == 0 && working_stage > 2){
            dele_command_process(current_directory, buffer, SERVER_MAIN_PATH, SERVER_MAIN_PATH_SIZE, cur_dir_size, socket);
        }
        else if(strcmp(command, "RNFR") == 0 && working_stage > 2){
            char * path_to_rename;
            if((path_to_rename = rnfr_command_process(current_directory, buffer, SERVER_MAIN_PATH, SERVER_MAIN_PATH_SIZE, cur_dir_size, socket)) != NULL){
                while(1){
                    for(int i = 0; buffer[i] != '\0'; i++){
                        buffer[i] = '\0';
                    }
                    read(socket, buffer, 1024);
                    command = get_command_name(buffer);
                    fprintf(stderr, "%s\n", command);
                    if(strcmp(command, "RNTO") == 0){
                        if(rnto_command_process(path_to_rename, current_directory, buffer, SERVER_MAIN_PATH, SERVER_MAIN_PATH_SIZE, cur_dir_size, socket) == 0){
                            free(path_to_rename);
                            break;
                        }
                    }
                    else if(check_if_command_exists(command) == -1){
                        write(socket, MESSAGE_500, 38);
                    }
                    else{
                        write(socket, MESSAGE_503, 29);
                    }
                }
            }
        }
        else if(strcmp(command, "RETR") == 0 && working_stage >= 2){
            retr_command_process(current_directory, buffer, SERVER_MAIN_PATH, SERVER_MAIN_PATH_SIZE, cur_dir_size, socket);
        }
        else if(strcmp(command, "STOR") == 0 && working_stage > 2){
            stor_command_process(current_directory, buffer, SERVER_MAIN_PATH, SERVER_MAIN_PATH_SIZE, cur_dir_size, socket);
        }
        else if(strcmp(command, "ZIP") == 0 && working_stage > 2){
            zip_command_process(current_directory, buffer, SERVER_MAIN_PATH, SERVER_MAIN_PATH_SIZE, cur_dir_size, socket);
        }
        else if(strcmp(command, "UZIP") == 0 && working_stage > 2){
            uzip_command_process(current_directory, buffer, SERVER_MAIN_PATH, SERVER_MAIN_PATH_SIZE, cur_dir_size, socket);
        }
        else if(strcmp(command, "QUIT") == 0){
            if(get_ammount_of_parameters(buffer) != 0){
                write(socket, MESSAGE_501, 44);
            }
            else{
                write(socket, MESSAGE_221, 39);
                free(user_name);
                return;
            }
        }
        else if(strcmp(command, "RNTO") == 0){
            write(socket, MESSAGE_503, 29);
        }
        else if(check_if_command_exists(command) == -1){
            write(socket, MESSAGE_500, 38);
        }
        else{
            write(socket, MESSAGE_503, 29);
        }
    }
}


int main() {
    int * socket_data = (int *)malloc(2*sizeof(int));
    char cwd[1024];
    char * SERVER_MAIN_PATH;
    int SERVER_MAIN_PATH_SIZE = 0;
    if(getcwd(cwd, sizeof(cwd)) != NULL){
        SERVER_MAIN_PATH = (char *)malloc(strlen(cwd)*sizeof(char));
        strcpy(SERVER_MAIN_PATH, cwd);
        SERVER_MAIN_PATH = (char *)realloc(SERVER_MAIN_PATH, (strlen(cwd)+17)*sizeof(char));
        SERVER_MAIN_PATH[strlen(cwd)] = '/';
        strcat(SERVER_MAIN_PATH, "server_storage");
        fprintf(stderr, "%s\n", SERVER_MAIN_PATH);
    }
    for(SERVER_MAIN_PATH_SIZE; SERVER_MAIN_PATH[SERVER_MAIN_PATH_SIZE] != '\0'; SERVER_MAIN_PATH_SIZE++);

    socket_data = initiallize_new_socket(0);
    
    main_working_process(socket_data[1], SERVER_MAIN_PATH, SERVER_MAIN_PATH_SIZE);
    
    close(socket_data[1]);
    close(socket_data[0]);
    return 0;
}