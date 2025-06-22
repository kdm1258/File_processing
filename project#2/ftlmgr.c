#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "flash.h"
// 필요한 경우 헤더파일을 추가한다

FILE *flashmemoryfp;	// fdevicedriver.c에서 사용

//
// 이 함수는 FTL의 역할 중 일부분을 수행하는데 물리적인 저장장치 flash memory에 Flash device driver를 이용하여 데이터를
// 읽고 쓰거나 블록을 소거하는 일을 한다 (강의자료 참조).
// flash memory에 데이터를 읽고 쓰거나 소거하기 위해서 fdevicedriver.c에서 제공하는 인터페이스를
// 호출하면 된다. 이때 해당되는 인터페이스를 호출할 때 연산의 단위를 정확히 사용해야 한다.
// 읽기와 쓰기는 페이지 단위이며 소거는 블록 단위이다 (주의: 읽기, 쓰기, 소거를 하기 위해서는 반드시 fdevicedriver.c의 함수를 사용해야 함)
// 갱신은 강의자료의 in-place update를 따른다고 가정한다.
// 스페어영역에 저장되는 정수는 하나이며, 반드시 binary integer 모드로 저장해야 한다.
//
extern int fdd_read(int ppn, char *pagebuf);
extern int fdd_write(int ppn, char *pagebuf);
extern int fdd_erase(int pbn);
int flash_memory_emulator(char *filename, int num_blocks){
    //truncate
    flashmemoryfp = fopen(filename, "wb");
    if(!flashmemoryfp){
        fprintf(stderr, "파일생성 실패\n");
        return -1;
    }
    fclose(flashmemoryfp);
    // read/write 용도
    flashmemoryfp = fopen(filename, "rb+");
    if(!flashmemoryfp){
        fprintf(stderr, "파일 열기 실패\n");
        fclose(flashmemoryfp);
        return -1;
    }
    for (int i = 0  ; i < num_blocks ; i++){
        if(fdd_erase(i)<0){
            fprintf(stderr, "pbn : %d번 초기화 실패\n",i);
            fclose(flashmemoryfp);
            return -1;
        }
    }
    fclose(flashmemoryfp);
    return 0;
}
int ftl_write(char *flashfile, int ppn, char *sectordata, int sparedata, char buffer[]){
    flashmemoryfp = fopen(flashfile, "rb+");
    if(!flashmemoryfp){
        fprintf(stderr, "파일 열기 실패\n");
        return -1;
    }
    //읽고 0xFF가 아니면 이미 할당된공간
    fdd_read(ppn, buffer);
    for(int i = 0 ; i < PAGE_SIZE ; i++){
        if((unsigned char)buffer[i] != 0xFF){
            fprintf(stderr, "해당 페이지는 이미 할당되어있습니다.\n");
            fclose(flashmemoryfp);
            return -1;
        }
    }

    memset(buffer, 0xFF, PAGE_SIZE);
    int len = strlen(sectordata);
    //sectordata 버퍼에 복사
    memcpy(buffer, sectordata, len);
    //spare 버퍼에 복사
    memcpy(buffer+SECTOR_SIZE, &sparedata, sizeof(int));

    if(fdd_write(ppn, buffer)<0){
        fprintf(stderr, "쓰기에 실패했습니다.\n");
        fclose(flashmemoryfp);
        return -1;
    }
    fclose(flashmemoryfp);
    return 0;
}
int ftl_read(char *flashfile, int ppn, char buffer[]){
    flashmemoryfp = fopen(flashfile, "rb");
    if(!flashmemoryfp){
        fprintf(stderr,"파일 열기에 실패했습니다.\n");
        return -1;
    }  
    fdd_read(ppn, buffer);
    int checkflag = 0;
    for(int i = 0 ; i < PAGE_SIZE ; i++){
        if((unsigned char)buffer[i] != 0xFF){
            checkflag = 1;
            break;
        }
    }  
    // 기록이 없을때는 아무것도 출력하지않고 종료한다.
    if(!checkflag){
        fclose(flashmemoryfp);
        return 0;
    }
    for(int i = 0 ; i < SECTOR_SIZE ; i++){
        if((unsigned char)buffer[i] != 0xFF){
            printf("%c", (unsigned char)buffer[i]);
        }
        else break;
    }
    printf(" ");
    int spare_num;
    memcpy(&spare_num, buffer+SECTOR_SIZE, sizeof(int));
    printf("%d",spare_num);
    fclose(flashmemoryfp);
    return 0;
}
int ftl_erase(char* flashfile, int pbn, char buffer[]){
    flashmemoryfp = fopen(flashfile, "rb+");
    if(!flashmemoryfp){
        fprintf(stderr, "파일을 열지 못했습니다.\n");
        return -1;
    }

    if(fdd_erase(pbn)<0){
        fprintf(stderr, "삭제에 실패하였습니다.\n");
        fclose(flashmemoryfp);
        return -1;
    }

    fclose(flashmemoryfp);
    return 0;
}
int inplace_update(char *flashfile, int ppn, char *sectordata, int sparedata, char pagebuf[], char* blockbuf){
    flashmemoryfp = fopen(flashfile, "rb+");
    if(!flashmemoryfp){
        fprintf(stderr,"파일을 열지 못했습니다.\n");
        return -1;
    }
    //전체 블록개수 구하기
    struct stat st;
    if(stat(flashfile, &st) != 0 ){
        fprintf(stderr,"파일크기조회실패.\n");
        fclose(flashmemoryfp);
        return -1;
    }
    int empty_pbn = -1;
    int num_blocks = st.st_size / BLOCK_SIZE;
    // 빈 블럭 찾기
    for(int pbn = 0 ; pbn < num_blocks ; pbn++){
        fseek(flashmemoryfp, BLOCK_SIZE*pbn, SEEK_SET);
        if(fread((void *)blockbuf, BLOCK_SIZE, 1, flashmemoryfp)!=1){
            fprintf(stderr, "빈블럭 읽기 실패\n");
            fclose(flashmemoryfp);
            return -1;
        }
        int flag = 0;
        for(int i = 0 ; i < BLOCK_SIZE ; i++){
            if((unsigned char)blockbuf[i] != 0xFF){
                flag = 1;
                break;
            }
        }
        if(flag == 1) continue;
        else{
            empty_pbn = pbn;
            break;
        }
    }
    if(empty_pbn == -1){
        fprintf(stderr, "빈 블록을 찾지 못했습니다.\n");
        fclose(flashmemoryfp);
        return -1;
    }
    int target_pbn = ppn / PAGE_NUM;
    int target_offset = ppn % PAGE_NUM;

    int read_cnt = 0;
    int write_cnt = 0;
    int erase_cnt = 0;
    int empty_block_flag = 1;   //copy시 빈블록으로 아무내용도 복사하지 않았다면 1, 불필요한 erase차단 목적
    //빈 block로 복사
    for(int i = 0 ; i < PAGE_NUM ; i++){
        // 덮어쓸 위치면 복사 x
        if(i == target_offset) continue;
        fdd_read(target_pbn*PAGE_NUM + i, pagebuf);
        read_cnt++;
        int flag = 0;
        for(int i = 0 ; i < PAGE_SIZE ; i++){
            if((unsigned char)pagebuf[i] != 0xFF){
                flag = 1;
                break;
            }
        }
        if(!flag){
            continue;
        }
        fdd_write(empty_pbn*PAGE_NUM + i, pagebuf);
        write_cnt++;
    }
    //기존 block erase
    fdd_erase(ppn/PAGE_NUM);
    erase_cnt++;
    // 수정 page포함 기존 위치에 복사
    for(int i = 0 ; i < PAGE_NUM ; i++){
        // 덮어쓸 위치면 입력값 복사
        if(i == target_offset){
            memset(pagebuf, 0xFF, PAGE_SIZE);
            int len = strlen(sectordata);
            memcpy(pagebuf, sectordata, len);
            memcpy(pagebuf+SECTOR_SIZE, &sparedata, sizeof(int));
            fdd_write(ppn, pagebuf);
            write_cnt++;
            continue;
        }
        fdd_read(empty_pbn*PAGE_NUM + i, pagebuf);
        int flag = 0;
        for(int i = 0 ; i < PAGE_SIZE ; i++){
            if((unsigned char)pagebuf[i] != 0xFF){
                flag = 1;
                break;
            }
        }
        if(!flag){
            continue;
        }
        empty_block_flag = 0;
        read_cnt++;
        fdd_write(target_pbn*PAGE_NUM + i, pagebuf);
        write_cnt++;
    }
    if(!empty_block_flag){
        // 빈 block 초기화
        fdd_erase(empty_pbn);
        erase_cnt++;
    }
    //횟수 출력
    printf("#reads=%d #writes=%d #erases=%d\n",read_cnt,write_cnt,erase_cnt);
    fclose(flashmemoryfp);
    return 0;
}
int main(int argc, char *argv[])
{	
	char sectorbuf[SECTOR_SIZE];
	char pagebuf[PAGE_SIZE];
	char *blockbuf=malloc(BLOCK_SIZE);
	
	// flash memory 파일 생성: 위에서 선언한 flashmemoryfp를 사용하여 플래시 메모리 파일을 생성한다. 그 이유는 fdevicedriver.c에서 
	//                 flashmemoryfp 파일포인터를 extern으로 선언하여 사용하기 때문이다.
	// 페이지 쓰기: pagebuf의 섹터와 스페어에 각각 입력된 데이터를 정확히 저장하고 난 후 해당 인터페이스를 호출한다
	// 페이지 읽기: pagebuf를 인자로 사용하여 해당 인터페이스를 호출하여 페이지를 읽어 온 후 여기서 섹터 데이터와
	//                  스페어 데이터를 분리해 낸다
	// memset(), memcpy() 등의 함수를 이용하면 편리하다. 물론, 다른 방법으로 해결해도 무방하다.
    char option = argv[1][0];
    switch (option){
        // flashmem emulator
        case 'c':
            if (argc != 4){
                fprintf(stderr, "%s c <flashfile> <#blocks> 형식에 맞지 않습니다. \n",argv[0]);
                return -1;
            }
            return flash_memory_emulator(argv[2], atoi(argv[3]));
        
        // page write
        case 'w':
            if (argc != 6){
                fprintf(stderr, "%s w <flashfile> <ppn> \"<sectordata>\" \"<sparedata>\" 형식에 맞지 않습니다. \n",argv[0]);
                return -1;
            }
            return ftl_write(argv[2], atoi(argv[3]), argv[4], atoi(argv[5]), pagebuf); 
           
        // page read
        case 'r':
            if (argc != 4){
                fprintf(stderr, "%s r <flashfile> <ppn> 형식에 맞지 않습니다. \n",argv[0]);
                return -1;
            }
            return ftl_read(argv[2], atoi(argv[3]), pagebuf); 
      
        // block erase
        case 'e':
            if (argc != 4){
                fprintf(stderr, "%s e <flashfile> <pbn> 형식에 맞지 않습니다. \n",argv[0]);
                return -1;
            }
            return ftl_erase(argv[2], atoi(argv[3]), pagebuf); 
        
        // inplace update
        case 'u':
            if (argc != 6){
                fprintf(stderr, "%s u <flashfile> <ppn> \"<sectordata>\" \"<sparedata>\" 형식에 맞지 않습니다. \n",argv[0]);
                return -1;
            }
            return inplace_update(argv[2], atoi(argv[3]), argv[4], atoi(argv[5]), pagebuf, blockbuf);
        default:
            fprintf(stderr, "잘못된 옵션입니다.\n");
            return -1;
    }
	return 0;
}
