#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#define buffer_size 1024
int main(int argc, char *argv[]){
    char buffer[buffer_size];
    if(argc != 4){
        fprintf(stderr,"잘못된 형식입니다/\n");
        return 0;
    }
    int fd = open(argv[3], O_WRONLY);
    if(fd == -1){
        fprintf(stderr, "파일을 열지 못했습니다.\n");
        return 0;
    }
    lseek(fd,atol(argv[1]),0);
    long data_size = strlen(argv[2]);
    long total_bytes = 0;
    
    while(data_size > total_bytes){
        if(data_size-total_bytes <= buffer_size){
            memcpy(buffer, argv[2] + total_bytes, data_size - total_bytes);
            write(fd,buffer,data_size-total_bytes);
            total_bytes = data_size;
        }
        else{
            memcpy(buffer, argv[2] + total_bytes, buffer_size);
            write(fd, buffer, buffer_size);
            total_bytes += buffer_size;
        }
    }
    close(fd);
}
