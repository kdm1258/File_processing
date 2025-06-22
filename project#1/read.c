#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#define buffer_size 1024
int main(int argc, char *argv[]){
    char buffer[buffer_size];
    long read_bytes = 0;
    long total_bytes = 0;
    if(argc != 4){
        fprintf(stderr, "잘못된 형식입니다.\n");
        return 0;  
    }
    int fd = open(argv[3], O_RDONLY);
    if(fd==-1){
        fprintf(stderr,"파일을 열지 못했습니다.\n");
        return 0;
    }
    lseek(fd, atol(argv[1]),0);
    while(total_bytes < atol(argv[2]) && (read_bytes = read(fd, buffer, buffer_size))>0){
        if(atol(argv[2]) > (total_bytes + read_bytes)){
            write(1, buffer, read_bytes);
            total_bytes += read_bytes;
        }
        else{
            write(1, buffer, atol(argv[2])-total_bytes);
            total_bytes += read_bytes;
        }
    }
    close(fd);
}
