#include <stdio.h>		// 필요한 header file 추가 가능
#include "student.h"
#include <string.h>
#include <stdlib.h>

//
// 함수 readRecord()는 학생 레코드 파일에서 주어진 rrn에 해당하는 레코드를 읽어서 
// recordbuf에 저장하고, 이후 unpack() 함수를 호출하여 학생 타입의 변수에 레코드의
// 각 필드값을 저장한다. 성공하면 1을 그렇지 않으면 0을 리턴한다.
// unpack() 함수는 recordbuf에 저장되어 있는 record에서 각 field를 추출하는 일을 한다.
//
int readRecord(FILE *fp, STUDENT *s, int rrn);
void unpack(const char *recordbuf, STUDENT *s);

//
// 함수 writeRecord()는 학생 레코드 파일에 주어진 rrn에 해당하는 위치에 recordbuf에 
// 저장되어 있는 레코드를 저장한다. 이전에 pack() 함수를 호출하여 recordbuf에 데이터를 채워 넣는다.
// 성공적으로 수행하면 '1'을, 그렇지 않으면 '0'을 리턴한다.
//
int writeRecord(FILE *fp, const STUDENT *s, int rrn);
void pack(char *recordbuf, const STUDENT *s);

//
// 함수 appendRecord()는 학생 레코드 파일에 새로운 레코드를 append한다.
// 레코드 파일에 레코드가 하나도 존재하지 않는 경우 (첫 번째 append)는 header 레코드 다음에
// 첫 번째 레코드를 저장한다. 당연히 레코드를 append를 할 때마다 header 레코드에 대한 수정이 뒤따라야 한다.
// 함수 append()는 내부적으로 writeRecord() 함수를 호출하여 레코드 저장을 해결한다.
// 성공적으로 수행하면 '1'을, 그렇지 않으면 '0'을 리턴한다.
//
int append(FILE *fp, char *sid, char *name, char *dept, char *addr, char *email);

//
// 학생 레코드 파일에서 검색 키값을 만족하는 레코드가 존재하는지를 sequential search 기법을 
// 통해 찾아내고, 이를 만족하는 모든 레코드의 내용을 출력한다. 검색 키는 학생 레코드를 구성하는
// 학번, 이름, 학과 필드만 사용한다 (명세서 참조). 내부적으로 readRecord() 함수를 호출하여 sequential search를 수행한다.
// 검색 결과를 출력할 때 반드시 print() 함수를 사용한다. (반드시 지켜야 하며, 그렇지
// 않는 경우 채점 프로그램에서 자동적으로 틀린 것으로 인식함)
// 과제 5에서는 삭제 레코드가 파일에 존재할 수 있으므로 이를 고려하여 검색 함수를 수정해야 한다.
//
void search(FILE *fp, enum FIELD f, char *keyval);
void print(const STUDENT *s[], int n);

//
// 레코드의 필드명을 enum FIELD 타입의 값으로 변환시켜 준다.
// 예를 들면, 사용자가 수행한 명령어의 인자로 "NAME"이라는 필드명을 사용하였다면 
// 프로그램 내부에서 이를 NAME(=1)으로 변환할 필요성이 있으며, 이때 이 함수를 이용한다.
//
enum FIELD getFieldID(char *fieldname);

//
// 학생 레코드 파일에서 조건을 만족하는 레코드를 찾아서 이것을 삭제한다.
// 참고로, 검색 조건은 학번(SID), 이름(NAME), 학과(DEPT) 중 하나만을 사용한다. 
// 또한, 삭제되는 레코드가 존재하면 이것을 삭제 레코드 리스트에 추가한다.
// 성공적으로 수행하면 '1'을, 그렇지 않으면 '0'을 리턴한다.
//
int delete(FILE *fp, enum FIELD f, char *keyval);

//
// 학생 레코드 파일에 새로운 레코드를 추가한다. 삭제 레코드가
// 존재하면 반드시 삭제 레코드들 중 하나에 새로운 레코드를 저장한다. (삭제 레코드의 선택은 과제 설명서 참조)
// 반면, 삭제 레코드가 하나도 존재하지 않으면 append 형태로 새로운 레코드를 추가한다.
// 성공적으로 수행하면 '1'을, 그렇지 않으면 '0'을 리턴한다.
//
int insert(FILE *fp, char *id, char *name, char *dept, char *addr, char *email);

void main(int argc, char *argv[])
{
	// 모든 file processing operation은 C library나 system call을 사용한다.
	// C library인 경우 FILE *fp를, sysem call인 경우 int fd와 같은 변수를 선택적으로 사용함
	// readRecord()와 같이 FILE *fp 인자가 쓰여져 있는 함수에도 똑같이 선택적으로 사용함
	//
	FILE *fp;	// or int fd;	
    int record_cnt = 0;
    //append
    if(strcmp(argv[1], "-a")==0){
        fp = fopen(argv[2], "r+b");
        if(!fp) {
            fp = fopen(argv[2], "w+b");
        }
        if(append(fp,  argv[3], argv[4], argv[5], argv[6], argv[7]) < 1 ){
            fprintf(stderr,"입력 형식이 잘못되었습니다.\n");
            fclose(fp);
            return;
        }
    }
    //search
    else if (strcmp(argv[1], "-s")==0){
        if (argc != 4) {
            fprintf(stderr, "검색 형식 오류\n");
            return;
        }
        char *key;
        char *value;
        key = strtok(argv[3], "=");
        value = strtok(NULL,"=");
            
        enum FIELD f = getFieldID(key);
        if (f == -1) {
            fprintf(stderr, "지원하지 않는 필드명: %s\n", key);
            return ;
        }
        fp = fopen(argv[2], "r+b");
        if (!fp) {
            perror("파일 열기 실패");
            return ;
        }
        search(fp, f, value);
        fclose(fp);
    }
    //delete
    else if(strcmp(argv[1], "-d")==0){
        if(argc != 4){
            fprintf(stderr,"지원하지 않는 형식입니다.\n");
            return;
        }
        char *key = strtok(argv[3], "=");
        char *value = strtok(NULL, "=");
        enum FIELD f = getFieldID(key);
        if (f == -1) {
            fprintf(stderr, "지원하지 않는 필드명: %s\n", key);
            return ;
        }
        fp = fopen(argv[2], "r+b");
        if (!fp) {
            perror("파일 열기 실패");
            return ;
        }
        delete(fp, f, value);
        fclose(fp);
    }
    //insert
    else if(strcmp(argv[1], "-i")==0){
        if(argc != 8){
            fprintf(stderr,"지원하지 않는 형식입니다.\n");
            return;
        }
        fp = fopen(argv[2], "r+b");
        if(!fp) {
            fp = fopen(argv[2], "w+b");
        }
        if(insert(fp,  argv[3], argv[4], argv[5], argv[6], argv[7]) < 1 ){
            fprintf(stderr,"입력 형식이 잘못되었습니다.\n");
            fclose(fp);
            return;
        }
    }
    else {
        fprintf(stderr, "지원하지 않는 옵션: %s\n", argv[1]);
        return;
    }
    //
	// append나 search 명령어에서 입력으로 주어지는 <record_file_name> 이름으로 파일을 생성한다.
	// 당연히 두 명령어 중 하나가 최초로 실행될 때 <record_file_name>파일을 최초로 생성하고,
	// 이후에는 디스크에 이 파일이 존재하기 때문에 새로 생성해서는 안된다. 즉, 이 파일이 디스크에 존재하지 않으면
	// 최초로 생성하고, 그렇지 않으면 생성하지 않고 skip 한다.
	// 최초로 파일을 생성할 때 header record (크기=8B)를 저장한다.
	// 
}

void print(const STUDENT *s[], int n)
{
	printf("#records = %d\n", n);
	for(int i = 0; i < n; i++)
	{
		printf("%s#%s#%s#%s#%s#\n", s[i]->sid, s[i]->name, s[i]->dept, s[i]->addr, s[i]->email);
	}
}

int readRecord(FILE *fp, STUDENT *s, int rrn){
    char recordbuf[RECORD_SIZE+1] = {0};
    int offset = RECORD_SIZE * rrn + HEADER_SIZE;

    fseek(fp, offset, SEEK_SET);
    if(fread(recordbuf, RECORD_SIZE, 1, fp)<1){
        return 0;
    }

    recordbuf[RECORD_SIZE] = '\0';
    unpack(recordbuf, s);
    return 1;

}
void unpack(const char *recordbuf, STUDENT *s){

    char buf[RECORD_SIZE+1];
    strncpy(buf, recordbuf, RECORD_SIZE);
    buf[RECORD_SIZE] ='\0';
    
    char *token = strtok(buf, "#");
    if(token) strncpy(s->sid, token, sizeof(s->sid));
    token = strtok(NULL, "#");
    if (token) strncpy(s->name, token, sizeof(s->name));
    token = strtok(NULL, "#");
    if (token) strncpy(s->dept, token, sizeof(s->dept));
    token = strtok(NULL, "#");
    if (token) strncpy(s->addr, token, sizeof(s->addr));
    token = strtok(NULL, "#");
    if (token) strncpy(s->email, token, sizeof(s->email));
    
    // 널 종료 보장
    s->sid[sizeof(s->sid)-1] = '\0';
    s->name[sizeof(s->name)-1] = '\0';
    s->dept[sizeof(s->dept)-1] = '\0';
    s->addr[sizeof(s->addr)-1] = '\0';
    s->email[sizeof(s->email)-1] = '\0';
}


int writeRecord(FILE *fp, const STUDENT *s, int rrn){
    char recordbuf[RECORD_SIZE+1];
    memset(recordbuf, '\0', sizeof(recordbuf));
    int offset = RECORD_SIZE*rrn + HEADER_SIZE;
    fseek(fp, offset, SEEK_SET);
    pack(recordbuf, s);
    fwrite(recordbuf, RECORD_SIZE, 1, fp);
    return 1;
}
void pack(char *recordbuf, const STUDENT *s){  
    int offset = 0;

    // SID
    int len = strlen(s->sid);
    memcpy(recordbuf + offset, s->sid, len);
    offset += len;
    recordbuf[offset++] = '#';

    // NAME
    len = strlen(s->name);
    memcpy(recordbuf + offset, s->name, len);
    offset += len;
    recordbuf[offset++] = '#';

    // DEPT
    len = strlen(s->dept);
    memcpy(recordbuf + offset, s->dept, len);
    offset += len;
    recordbuf[offset++] = '#';

    // ADDR
    len = strlen(s->addr);
    memcpy(recordbuf + offset, s->addr, len);
    offset += len;
    recordbuf[offset++] = '#';

    // EMAIL
    len = strlen(s->email);
    memcpy(recordbuf + offset, s->email, len);
    offset += len;
    recordbuf[offset++] = '#';

}


int append(FILE *fp, char *sid, char *name, char *dept, char *addr, char *email){
    int cur_rrn = 0;
    STUDENT student;
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if(size< HEADER_SIZE){
        // 헤더가 없으면 새 파일로 판단하고 초기화
        rewind(fp);
        int header[2] = {0, -1};
        fwrite(header, sizeof(int), 2, fp);
        fflush(fp);
    }
    
    // 다시 헤더에서 rrn 읽기
    rewind(fp);
    fread(&cur_rrn, sizeof(int), 1, fp);
    
    strncpy(student.sid, sid, sizeof(student.sid));
    student.sid[sizeof(student.sid) - 1] = '\0';

    strncpy(student.name, name, sizeof(student.name));
    student.name[sizeof(student.name) - 1] = '\0';

    strncpy(student.dept, dept, sizeof(student.dept));
    student.dept[sizeof(student.dept) - 1] = '\0';

    strncpy(student.addr, addr, sizeof(student.addr));
    student.addr[sizeof(student.addr) - 1] = '\0';

    strncpy(student.email, email, sizeof(student.email));
    student.email[sizeof(student.email) - 1] = '\0';

    if (writeRecord(fp, &student, cur_rrn)<1){
        printf("레코드 쓰기에 실패하였습니다.\n");
        return 0;
    }
    cur_rrn++;
    fseek(fp, 0, SEEK_SET);
    fwrite(&cur_rrn, sizeof(int), 1, fp);    //header info update
    return 1;
}

void search(FILE *fp, enum FIELD f, char *keyval) {
    fseek(fp, 0, SEEK_END);
    long filesize = ftell(fp);
    int max_rrn = (filesize - HEADER_SIZE) / RECORD_SIZE;

    STUDENT tmp;
    STUDENT *result[1000];
    int cnt = 0;

    for (int rrn = 0; rrn < max_rrn; rrn++) {
        long offset = HEADER_SIZE + rrn * RECORD_SIZE;
        fseek(fp, offset, SEEK_SET);

        char first;
        fread(&first, sizeof(char), 1, fp);
        if (first == '*') continue;  // 삭제된 레코드 skip

        if (readRecord(fp, &tmp, rrn) < 1) continue;

        const char *target = NULL;
        switch (f) {
            case SID: target = tmp.sid; break;
            case NAME: target = tmp.name; break;
            case DEPT: target = tmp.dept; break;
            default: continue;
        }

        if (strcmp(keyval, target) == 0) {
            result[cnt] = malloc(sizeof(STUDENT));
            *result[cnt] = tmp;
            cnt++;
        }
    }

    print((const STUDENT **)result, cnt);
    for (int i = 0; i < cnt; i++) free(result[i]);
}


enum FIELD getFieldID(char *fieldname){
    if (strcmp(fieldname, "SID") == 0) return SID;
    if (strcmp(fieldname, "NAME") == 0) return NAME;
    if (strcmp(fieldname, "DEPT") == 0) return DEPT;
    if (strcmp(fieldname, "ADDR") == 0) return ADDR;
    if (strcmp(fieldname, "EMAIL") == 0) return EMAIL;
    return -1;  // 지원하지 않는 필드명
}

int delete(FILE *fp, enum FIELD f, char *keyval){
    if (!fp) return -1;

    int total = 0;
    int deleted_head = -1;

    //헤더읽기
    fseek(fp, 0, SEEK_SET);
    fread(&total, sizeof(int), 1, fp);
    fread(&deleted_head, sizeof(int), 1, fp);

    STUDENT tmp;
    int deleted_cnt = 0;

    fseek(fp, 0 , SEEK_END);
    long filesize = ftell(fp);
    int max_rrn = (filesize - HEADER_SIZE) / RECORD_SIZE;

    for(int rrn = 0 ; rrn < max_rrn ; rrn++){
        long offset = HEADER_SIZE + rrn * RECORD_SIZE;
        fseek(fp, offset, SEEK_SET);

        //삭제 레코드인지 확인
        char deleted_cond;
        fread(&deleted_cond, sizeof(char), 1, fp);
        if(deleted_cond == '*') continue;

        if(readRecord(fp, &tmp, rrn) < 1) continue; //읽기실패

        const char *target = NULL;
        
        switch(f){
            case SID : target = tmp.sid; break;
            case NAME: target = tmp.name; break;
            case DEPT: target = tmp.dept; break;
            default: continue;
        }
        if(strcmp(target, keyval) == 0){
            fseek(fp, offset, SEEK_SET);
            fputc('*', fp);

            fwrite(&deleted_head, sizeof(int), 1, fp);

            deleted_head = rrn;
            deleted_cnt++;
        }
    }
    //헤더 수정
    if(deleted_cnt>0){
        total -= deleted_cnt;
        fseek(fp, 0, SEEK_SET);
        fwrite(&total, sizeof(int), 1, fp);
        fwrite(&deleted_head, sizeof(int), 1, fp);
        return 1;
    }
    return 0;
}

int insert(FILE *fp, char *id, char *name, char *dept, char *addr, char *email){
    if(!fp) return 0;

    int total = 0 ;
    int deleted_head = -1;  //초기값은 일단 1

    fseek(fp, 0 , SEEK_SET);
    fread(&total, sizeof(int), 1, fp);
    fread(&deleted_head, sizeof(int), 1, fp);
    
    if(deleted_head == -1){
        append(fp, id, name, dept, addr, email);
        return 1;
    }
    //갱신해야할 head rrn 미리 받아오기
    int new_deleted_head = -1 ;
    int offset = HEADER_SIZE + deleted_head * RECORD_SIZE + 1; //+1은 '*'처리용
    fseek(fp, offset, SEEK_SET);
    fread(&new_deleted_head, sizeof(int), 1, fp); 

    //student정보 삭제된 위치에 넣기
    STUDENT student;
    strncpy(student.sid, id, sizeof(student.sid));
    student.sid[sizeof(student.sid) - 1] = '\0';

    strncpy(student.name, name, sizeof(student.name));
    student.name[sizeof(student.name) - 1] = '\0';

    strncpy(student.dept, dept, sizeof(student.dept));
    student.dept[sizeof(student.dept) - 1] = '\0';

    strncpy(student.addr, addr, sizeof(student.addr));
    student.addr[sizeof(student.addr) - 1] = '\0';

    strncpy(student.email, email, sizeof(student.email));
    student.email[sizeof(student.email) - 1] = '\0';

    if (writeRecord(fp, &student, deleted_head)<1){
        printf("레코드 쓰기에 실패하였습니다.\n");
        return 0;
    }
    total++;
    fseek(fp, 0, SEEK_SET);
    fwrite(&total, sizeof(int), 1, fp);    //header info update
    fwrite(&new_deleted_head, sizeof(int), 1, fp);
    return 1;
}
