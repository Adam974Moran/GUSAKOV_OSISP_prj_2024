#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

struct TarHeader{
    char name [100];
    char mode [8];
    char uid [8];
    char gid [8];
    char size [12];
    char mtime [12];
    char chksum [8];
    char typeflag;
};


char * inject_filename_from_path(char * filename_path, int fname_path_size){
    //Setting variables
    char * r_fname,             //Pointer for reversed filename
        * filename;             //Pointer for filename

    int r_fname_size = 1, 
        fname_size = 1, 
        j = 0;


    r_fname = (char *)malloc(r_fname_size*sizeof(char));
    filename = (char*)malloc(fname_size*sizeof(char));


    //Getting reversed filename frome the filename's path
    for(int i = fname_path_size-1; filename_path[i] != '/'; i--){
        r_fname[j] = filename_path[i];
        r_fname_size++;
        r_fname = (char*)realloc(r_fname, r_fname_size*sizeof(char));
        j++;
    }


    //Getting real filename
    for(int i = r_fname_size-2; r_fname[i] != '\0'; i--){
        filename[j] = r_fname[i];
        fname_size++;
        j++;
        filename = (char*)realloc(filename, fname_size*sizeof(char));
    }


    free(r_fname);
    return filename;
}




int main(){
    FILE * tar;
    struct stat fst;

    char * filename_path,
        * tarname,
        * filename;

    const char TAR_FORMAT[5] = "\0rat.";

    int fname_path_size = 1,
        tname_size = 1;


    filename_path = (char*)malloc(fname_path_size*sizeof(char));
    tarname = (char*)malloc(tname_size*sizeof(char));


    printf("Enter filename path: ");
    for(int i = 0; (filename_path[i] = getchar()) != '\n'; i++){
        fname_path_size++;
        filename_path = (char *)realloc(filename_path, fname_path_size*sizeof(char));
    }
    filename_path[fname_path_size-1] = '\0';
    printf("\nfilename path: %s\n", filename_path);


    for(int i = 0; filename_path[i] != '\0'; i++){
        tarname[i] = filename_path[i];
        tname_size++;
        tarname = (char *)realloc(tarname, tname_size*sizeof(char));
    }
    tarname[tname_size-1] = '\0';


    int name_pos = tname_size-1;
    for(name_pos; name_pos != -1; name_pos--){
        if(filename_path[name_pos] == '.'){
            break;
        }
    }


    if(name_pos == -1){
        for(int i = 4; i >= 0; i--){
            tarname[tname_size-1] = TAR_FORMAT[i];
            tname_size++;
            tarname = (char *)realloc(tarname, tname_size*sizeof(char));
            tarname[tname_size-1] = '\0';
        }
    }
    else{
        tarname = (char*)realloc(tarname, (name_pos + 4)*sizeof(char));
        tname_size = name_pos+4;
        for(int i = 0; i < 5; i++){
            tarname[tname_size-i] = TAR_FORMAT[i];
        }
    }
    printf("\ntarname: %s\n", tarname);
    for(int  i = 0; i < tname_size; i ++){
        printf("%c", tarname[i]);
    }


    tar = fopen(tarname, "wb");
    if(stat(filename_path, &fst) < 0){
        perror("Error in creating stat");
    }


    filename = inject_filename_from_path(filename_path, fname_path_size);
    printf("\nfilename: %s\n", filename);

    free(tar);
    free(filename_path);
    free(tarname);
    free(filename);
}


    