// 주의사항
// 1. hybridmapping.h에 정의되어 있는 상수 변수를 우선적으로 사용해야 함 
// (예를 들면, PAGES_PER_BLOCK의 상수값은 채점 시에 변경할 수 있으므로 반드시 이 상수 변수를 사용해야 함)
// 2. hybridmapping.h에 필요한 상수 변수가 정의되어 있지 않을 경우 본인이 이 파일에서 만들어서 사용하면 됨
// 3. 새로운 data structure가 필요하면 이 파일에서 정의해서 쓰기 바람(hybridmapping.h에 추가하면 안됨)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include "hybridmapping.h"
// 필요한 경우 헤더 파일을 추가하시오.
// 필요한 경우 함수를 추가하시오.
// Flash Device Driver 함수 선언
int fdd_read(int ppn, char *pagebuf);
void fdd_write(int ppn, char *pagebuf);
void fdd_erase(int pbn);

// mapping table
typedef struct{
    int pbn;
    int last_offset;
} MappingEntry;
// lbn을 인덱스로 하여 접근
MappingEntry AMT[DATABLKS_PER_DEVICE];

// free block list
typedef struct FreeBlockNode {
    int pbn;
    struct FreeBlockNode* next;
} FreeBlockNode;
FreeBlockNode* free_block_head = NULL;
//
// 새 블럭을 가져오는 함수 
// freeblockliset를 뒤져 새블럭을 가져오며 블럭이 없을 경우 -1 리턴
//
int get_free_block(){
    //freeblock이 없을때
    if (free_block_head == NULL){
        printf("No available block\n");
        return -1;
    }
    //블럭 할당
    int pbn = free_block_head -> pbn;
    FreeBlockNode* tmp = free_block_head;
    free_block_head = free_block_head -> next;
    free(tmp);
    return pbn;
 }

// flash memory를 처음 사용할 때 필요한 초기화 작업, 예를 들면 address mapping table에 대한
// 초기화 등의 작업을 수행한다. 따라서, 첫 번째 ftl_write() 또는 ftl_read()가 호출되기 전에
// file system에 의해 반드시 먼저 호출이 되어야 한다.
//
void ftl_open()
{
	//
   	// address mapping table 생성 및 초기화
    for (int i = 0 ; i < DATABLKS_PER_DEVICE ; i++){
        AMT[i].pbn = -1;
        AMT[i].last_offset = -1;
    }      
	// free block linked list 생성 및 초기화
    FreeBlockNode* prev = NULL ; 
    for(int i = BLOCKS_PER_DEVICE-1 ; i >= 0 ; i--){
        FreeBlockNode* node = (FreeBlockNode*)malloc(sizeof(FreeBlockNode));
        node -> pbn = i;
        node -> next = prev;
        prev = node;
    }
    free_block_head = prev;
    // 그 이외 필요한 작업 수행
	return;
}

//
// 이 함수를 호출하는 쪽(file system)에서 이미 sectorbuf가 가리키는 곳에 512B의 메모리가 할당되어 있어야 함
// (즉, 이 함수에서 메모리를 할당 받으면 안됨)
//
void ftl_read(int lsn, char *sectorbuf)
{
    char pagebuf[PAGE_SIZE];
    int lbn = lsn / PAGES_PER_BLOCK;
    int pbn = AMT[lbn].pbn;
    int found = 0;

    if(pbn == -1){
        printf("ftl_read error : mapping table에 일치하는 항목이 없습니다.\n");
        return;
    }
    int offset = AMT[lbn].last_offset;
    for(int i = offset ; i >= 0 ; i--){
        int ppn = pbn * PAGES_PER_BLOCK + i;
        if(fdd_read(ppn, pagebuf) != 1){
            printf("ftl_read error : 파일 읽기 실패\n");
            memset(sectorbuf, 0xFF, SECTOR_SIZE);
            return;
        }
        int target_lsn;
        memcpy(&target_lsn, pagebuf+SECTOR_SIZE, sizeof(int));
        if(lsn == target_lsn){
            found = 1;
            break;
        }
    }
    if(!found){
        printf("해당 lsn을 block에서 찾지 못하였습니다\n");
        memset(sectorbuf, 0xFF, SECTOR_SIZE);
        return;
    }
    memcpy(sectorbuf, pagebuf, SECTOR_SIZE);
	return;
}

//
// 이 함수를 호출하는 쪽(file system)에서 이미 sectorbuf가 가리키는 곳에 512B의 메모리가 할당되어 있어야 함
// (즉, 이 함수에서 메모리를 할당 받으면 안됨)
//
void ftl_write(int lsn, char *sectorbuf)
{
    char pagebuf[PAGE_SIZE];
    memset(pagebuf, 0xFF, PAGE_SIZE);
    int lbn = lsn / PAGES_PER_BLOCK;    //lbn계산
    
    int pbn = AMT[lbn].pbn;             //pbn추출 만약 -1이라면 freeblocklist에서 헤더값 가져오기
    int offset = AMT[lbn].last_offset;  //offset추출 offset+1위치에 저장하며 if offset == PAGES_PER_BLOCK이면 다른 블럭 할당해야함
    
    if(pbn == -1){
        //새블럭 할당
        pbn = get_free_block();
        offset = 0;

        //쓰기
        int ppn = pbn * PAGES_PER_BLOCK + offset;

        memcpy(pagebuf, sectorbuf, SECTOR_SIZE);
        memcpy(pagebuf + SECTOR_SIZE, &lsn, sizeof(int));

        fdd_write(ppn, pagebuf);

        AMT[lbn].pbn = pbn;
        AMT[lbn].last_offset = offset;
    }
    else if(offset == PAGES_PER_BLOCK-1){
        int valid_cpy_lsn[DATAPAGES_PER_DEVICE]; //최신 데이터만 복사하기 위해 중복 lsn검사
        memset(valid_cpy_lsn, 0 ,sizeof(valid_cpy_lsn));
        valid_cpy_lsn[lsn] = 1;

        int new_pbn = get_free_block();
        int new_offset = 0;

        //유효데이터 역순 복사
        for(int i = PAGES_PER_BLOCK-1 ; i >= 0 ; i--){
            char old_page[PAGE_SIZE];
            memset(old_page, 0xFF, PAGE_SIZE);
  
            //ppn에 들어있는 내용 복사
            int old_ppn = pbn * PAGES_PER_BLOCK + i;    //src page
            fdd_read(old_ppn, old_page);
            
            // spare공간의 lsn만 따로 추출
            int old_lsn;
            memcpy(&old_lsn, old_page + SECTOR_SIZE, sizeof(int));
            if (old_lsn < 0 || old_lsn >= DATAPAGES_PER_DEVICE) continue;
           
            if(!valid_cpy_lsn[old_lsn]){
                valid_cpy_lsn[old_lsn] = 1;
                
                int copy_ppn = new_pbn * PAGES_PER_BLOCK + new_offset;
                fdd_write(copy_ppn, old_page);
            
                new_offset++;
            }  
        }
        //write하려는 sectorbuf 쓰기
        int new_ppn = new_pbn * PAGES_PER_BLOCK + new_offset;
        memset(pagebuf, 0xFF, PAGE_SIZE);
        memcpy(pagebuf, sectorbuf, SECTOR_SIZE);
        memcpy(pagebuf + SECTOR_SIZE, &lsn, sizeof(int));
        fdd_write(new_ppn, pagebuf);
        
        AMT[lbn].pbn = new_pbn;
        AMT[lbn].last_offset = new_offset;
        
        //기존 블럭 erase 및 list 추가
        fdd_erase(pbn);
        FreeBlockNode* node = (FreeBlockNode*)malloc(sizeof(FreeBlockNode));
        node->pbn = pbn;
        node->next = free_block_head;
        free_block_head = node;
    }
    else{
        //쓰기
        offset++;
        int ppn = pbn * PAGES_PER_BLOCK + offset;
 
        memset(pagebuf, 0xFF, PAGE_SIZE);
        memcpy(pagebuf, sectorbuf, SECTOR_SIZE);
        memcpy(pagebuf + SECTOR_SIZE, &lsn, sizeof(int));

        fdd_write(ppn, pagebuf);

        AMT[lbn].last_offset = offset;
    }
	return;
}

// 
// Address mapping table 등을 출력하는 함수이며, 출력 포맷은 과제 설명서 참조
// 출력 포맷을 반드시 지켜야 하며, 그렇지 않는 경우 채점시 불이익을 받을 수 있음
//
void ftl_print()
{
    printf("lbn pbn last_offset\n");
    for (int lbn = 0; lbn < DATABLKS_PER_DEVICE; lbn++) {
        printf("%d %d %d\n", lbn, AMT[lbn].pbn, AMT[lbn].last_offset);
    }
	return;
}
