# 과제 4: 레코드 저장 및 검색

## 주의사항
- 파일 I/O 연산은 system call 또는 C 라이브러리만을 사용한다.

---

## 1. 개요

“학생” 레코드 파일에 저장되어 있는 각 레코드(record)는 다음과 같은 특징을 갖는다.

- 하나의 레코드는 **다섯 개의 필드(field)** 로 구성된다.
- 레코드 구성 필드의 순서, 이름, 최대 크기 (바이트 단위):

  | 필드명 | 최대 크기 (bytes) |
  |--------|------------------|
  | 학번    | 8                |
  | 이름    | 10               |
  | 학과    | 12               |
  | 주소    | 30               |
  | 이메일  | 20               |

- 레코드 구분 방식: **고정 길이 레코드 (fixed-length record)**
- 필드 구분 방식: **delimiter 방식 ('#')**
- 레코드 1개의 총 길이: **85 바이트** (`student.h` 참고)
- 헤더 레코드: 8 바이트 (전체 레코드 수: 4B + 예약 공간: 4B)
- 레코드 번호는 `rrn = 0, 1, 2, ...`의 값을 갖는다.

---

## 2. 레코드 추가 (append)

사용자로부터 레코드 파일명과 필드값 리스트를 입력받는다.  
5개의 필드값은 큰따옴표(`"`)로 묶어서 입력하며, **영문/숫자/기호만 허용**된다.

### 실행 예시
```bash
$ a.out -a students.dat "20071234" "Gildong Hong" "Computer" "Dongjak-gu, Seoul" "gdhong@ssu.ac.kr"
```

출력 없음.

---

## 3. 레코드 검색 (search)

사용자로부터 **레코드 파일명, 필드명, 필드값**을 입력받아 검색 조건에 맞는 레코드를 출력한다.  
**SID, NAME, DEPT** 세 개 필드명만 검색 키로 사용 가능하며, **모두 대문자로** 입력해야 함.

### 실행 예시
```bash
$ a.out -s students.dat "NAME=Gildong Hong"
```

### 출력 예시
```
#records = 2
20071234#Gildong Hong#Computer#Dongjak-gu, Seoul#gdhong@ssu.ac.kr
20041328#Gildong Hong#Computer#Gawnak-gu, Seoul#gildong@ssu.ac.kr
```

**필드명=필드값** 조건은 반드시 큰따옴표로 묶어야 하며, 내부에서 제공된 출력 함수로 출력됨 (`student.c` 참고).
