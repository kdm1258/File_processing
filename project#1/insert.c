#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
int main(int argc, char *argv[]){
    if(argc != 4){
        fprintf(stderr,"잘못된 형식입니다.\n");
        return 0;
    }
    int fd = open(argv[3], O_RDWR);
    if(fd==-1){ 
        fprintf(stderr,"파일을 열지 못했습니다.\n");
        return 0;
    }
    long offset = atol(argv[1]);
    long file_size = lseek(fd,0,2);
    long copy_size = file_size - offset - 1;
    long data_size = strlen(argv[2]);
    char *buffer = (char*)malloc(copy_size);
    if(offset > file_size-1){
        fprintf(stderr,"offset이 file크기보다 큽니다.\n");
        return 0;
    }
    lseek(fd, offset+1, 0);
    read(fd, buffer, copy_size);
    lseek(fd, offset+1, 0);
    write(fd, argv[2], data_size);
    write(fd, buffer, copy_size);
    free(buffer);
    close(fd);
}  
