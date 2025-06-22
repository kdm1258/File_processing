#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
int main(int argc, char *argv[]){
	if (argc != 3){
		fprintf(stderr,"잘못된 형식입니다.\n");
		return 0;
	}
    int fd1 = open(argv[1], O_RDONLY);
	if(fd1==-1){
        fprintf(stderr,"파일 열기 실패\n");
        return 0;
    }
    int fd2 = open(argv[2],O_WRONLY | O_TRUNC | O_CREAT,0644);
    if (fd2 == -1) {
        fprintf(stderr, "복사본 생성 실패\n");
        close(fd1);
        return 0;
    }
    
    int read_bytes = 0;
    char buffer[10];
    //read_bytes에 읽은 bytes 수를 기록하여 읽은크기만큼만 쓰기
    while((read_bytes = read(fd1, buffer, 10)) > 0){
        write(fd2, buffer,read_bytes);
    }

    close(fd1);
    close(fd2);
}
