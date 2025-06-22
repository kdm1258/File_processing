#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
int main(int argc, char* argv[]){
    if(argc != 4){
        fprintf(stderr, "잘못된 형식입니다.\n");
        return 0;
    }
    int fd = open(argv[3], O_RDWR);
    if(fd==-1){
        fprintf(stderr,"파일을 열지 못했습니다.\n");
        return 0;
    }   

    long file_size = lseek(fd,0,2);
    long offset = atoi(argv[1]);
    long byte_size = atoi(argv[2]);
    if(file_size < offset+byte_size){
        //지우려는 바이트수가 남이있는 파일보다 클 때
        byte_size = file_size - offset;
    }
    long copy_size = file_size - offset - byte_size;
    char *buffer = (char *)malloc(copy_size);
    if(offset>file_size){
        fprintf(stderr, "offset이 file크기보다 큽니다.\n");
        return 0;
    }
    lseek(fd, offset + byte_size, 0);
    copy_size = read(fd, buffer, copy_size);
    lseek(fd, offset, 0);
    write(fd, buffer, copy_size);
    ftruncate(fd, file_size - byte_size);
    free(buffer);
    close(fd);
}
