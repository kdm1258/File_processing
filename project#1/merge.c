#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
int main(int argc, char *argv[]){
    char buffer[1024];  //Buffer
    int read_bytes = 0; //읽기 성공한 바이트수
    if(argc != 4){
        fprintf(stderr,"잘못된 형식입니다\n");
        return 0;
    }
    //기존에 file1이 존재할 경우 없애기 위해 O_TRUNC옵션을 준뒤
    //close 후 O_APPEND 옵션을 적용해 다시 open
    int fd1 = open(argv[1], O_WRONLY | O_TRUNC | O_CREAT, 0644);
    if(fd1 ==-1){
        fprintf(stderr, "파일 생성실패\n");
        return 0;
    }
    int fd2 = open(argv[2], O_RDONLY);
    if(fd2 ==-1){
        fprintf(stderr, "파일 열기 실패\n");
        return 0;
    }
    
    int fd3 = open(argv[3], O_RDONLY);
    if(fd3 ==-1){
        fprintf(stderr, "파일 열기 실패\n");
        return 0;
    }

    while ((read_bytes = read(fd2, buffer, 1024)) > 0){
        write(fd1, buffer, read_bytes);
    }
    close(fd1);
    fd1 = open(argv[1], O_WRONLY | O_APPEND);
    while ((read_bytes = read(fd3, buffer, 1024)) > 0){
        write(fd1, buffer, read_bytes);
    }
    close(fd1);
    close(fd2);
    close(fd3);
}
