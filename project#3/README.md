
# 과제 3: Flash Memory에서의 Hybrid Mapping FTL 구현

## 1. 개요

Hybrid mapping 기법(강의자료 “Flash Memory Overview”의 21쪽)을 따르는 FTL을 구현한다. 다음과 같은 제약사항들을 지켜야 한다.

- 파일 I/O 연산은 system call 또는 C 라이브러리만을 사용한다.
- 아래의 (1), (2), (3), (4)의 기능을 `ftlmgr.c`에 구현한다.
- `hybridmapping.h`와 `fdevicedriver.c`는 주어진 그대로 사용하며 예외를 제외하고 수정해서는 안 된다. (자세한 것은 두 개 파일 내부의 설명 참조)
- 네 개의 함수를 테스트하기 위해 `main()` 함수를 본인 스스로 만들기 바라며, file system의 역할을 수행하는 `main()` 함수에서의 대략적인 시나리오는 (1) flash memory 파일 생성 및 초기화 (2) `ftl_open()` 호출 (3) 여러 개의 `ftl_write()`나 `ftl_read()`를 호출하여 테스트하는 것으로 이루어진다.
- 테스트 프로그램을 실행하기 할 때 이전에 사용했던 flash memory 파일이 존재하면 이것을 재사용하지 않으며 새로운 flash memory 파일을 생성하여 사용한다.

## (1) `ftl_open()` 구현

일반적으로 FTL을 구현하기 위해서는 논리주소를 물리주소로 변환하는 address mapping table이 필요하다. Hybrid mapping FTL에 맞는 address mapping table을 생각해 보고 이에 맞는 data structure를 각자 정의해서 사용한다 (강의자료 참조). 강의 시간에 언급한 대로 해당 블록에서 첫 번째 빈 페이지를 빠르게 찾기 위해 mapping table에 “last_offset”과 같은 컬럼 하나를 추가한다.

`ftl_open()`에서 하는 일은 다음과 같다.

- 이러한 data structure를 이용하여 address mapping table을 하나 생성한다. 이때 `hybridmapping.h`에 정의되어 있는 상수 변수를 반드시 활용해야 한다.
- `ftl_open()`에서는 일반적으로 여러 초기화 작업을 하는데, 위에서 생성한 address mapping table에서 각 lbn에 대응되는 pbn의 값을 모두 –1로 초기화한다. 여기서 lbn은 0, 1, ..., (`DATABLKS_PER_DEVICE`-1)의 값을 가진다 (`hybridmapping.h` 참조). 또한 `last_offset`의 값도 모두 –1로 초기화한다.
- 플래시 메모리 내의 free block list를 관리하기 위한 data structure를 만드는데 이때 반드시 linked list 형태가 되어야 한다. (비효율적이지만) 초기 free block linked list에서 전체 노드의 수는 전체 블록의 수, 즉 `BLOCKS_PER_DEVICE`와 동일하며, 각 노드에는 pbn이 저장된다. 초기에는 linked list의 각 노드의 pbn은 0, 1, ..., (`BLOCKS_PER_DEVICE`-1)과 같이 오름차순으로 정렬되어 있어야 한다 (먼 미래에는 오름차순 정렬이 지켜지지 않을 수 있다).
- Free block에 대한 요청이 있을 때마다 linked list의 헤더가 가리키는 노드의 pbn을 할당하며, garbage block이 발생하면 반드시 erase 연산을 수행한 후 linked list의 맨앞에 삽입한다 (linked list를 관리하는 함수들을 만들 필요가 있음).

**file system에서 `ftl_write()`나 `ftl_read()`를 최초로 호출하기 전에 반드시 초기화와 관련된 `ftl_open()`를 호출해야 한다.**

## (2) `ftl_write(int lsn, char *sectorbuf)` 구현

File system이 `ftl_write()`를 호출할 때 인자값으로 `lsn(=lpn)`과 `sectorbuf`에 저장되어 있는 512B 데이터를 전달한다. FTL은 이 데이터를 flash memory에 쓰기를 해야 하는데, 이때 어떤 물리적 페이지(`ppn`)에 써야 할지 결정해야 한다. 이것은 hybrid mapping 기법의 동작 원리대로 결정되어야 한다. 또한 address mapping table에 대한 갱신이 요구된다.

FTL과 flash memory 파일 간의 데이터 쓰기는 반드시 페이지 단위로 이루어져야 하며, 또한 flash device driver의 `fdd_write()` 함수의 호출을 통해 이루어져야 한다.

- `fdd_write(int ppn, char *pagebuf)`를 호출하기 전에 FTL은 `ftl_write()`에서 받은 `sectorbuf`의 데이터를 `pagebuf`의 sector 영역에 저장하고, `pagebuf`의 spare 영역에 `ftl_write()`의 `lsn`을 저장하고 이후 이 `pagebuf`를 인자값으로 `fdd_write()`에 전달한다.
- Spare 영역에 `lsn`을 저장할 때 `lsn`의 크기는 4B이며, spare의 맨왼쪽에 저장한다. 참고로, 이 `lsn`의 저장 여부를 통해 해당 페이지가 비어있는지를 판단할 수 있다.

File system으로부터 `ftl_write()`가 호출되었을 때, 해당 블록에 빈 페이지가 존재하지 않으면 빈 블록을 하나 할당 받은 후 복사 등의 복잡한 연산 작업들이 필요할 수 있다. (당연히, 새로운 블록에는 반드시 `ftl_write()`의 인자로 받은 `lsn`과 데이터를 포함하여 가장 최신의 데이터만 복사되어야 한다). 복사 대상 블록은 flash device driver의 `fdd_erase()`를 통해 초기화가 되어야 하고 free block linked list에 추가되어야 한다.

**주의사항**:

- `fdd_write()`를 호출하기 전에 `pagebuf`에 sector 데이터와 spare 데이터를 저장할 때 `memcpy()`를 쓰면 편리하며 물론 다른 방식을 사용해도 됨
- spare 영역에 `lsn`을 저장할 때 반드시 binary integer 모드로 저장해야 하며, ASCII 모드로 저장하는 경우 채점 시 올바르게 동작하지 않음
- flashmemory 파일을 `read(fread)`, `write(fwrite)` 등과 같이 file I/O 함수를 직접 써서 읽기나 쓰기를 수행해서는 절대로 안됨 (반드시 `fdevicedriver.c`의 함수를 사용해야 함)

## (3) `ftl_read(int lsn, char *sectorbuf)` 구현

File system이 `ftl_read()`를 호출하면, FTL은 인자로 주어진 `lsn(=lpn)`을 이용하여 `pbn`을 구한 후 가장 최신의 데이터를 저장하고 있는 페이지를 찾아서 인자로 주어진 `sectorbuf`에 복사하여 전달한다 (당연히 페이지의 spare 복사는 필요 없음).

FTL이 flash memory 파일로부터 데이터를 읽을 때는 반드시 페이지 단위를 사용하며 이것은 flash device driver의 `fdd_read()` 함수를 호출함으로써 자연스럽게 해결된다.

**주의사항**:

- `fdd_read()`를 통해 flash memory에서 페이지를 읽어 온 후 `ftl_read()`의 `sectorbuf`에 섹터 데이터를 복사할 때 `memcpy()`를 사용하면 편리함
- flashmemory 파일을 `read(fread)`, `write(fwrite)` 등과 같이 file I/O 함수를 직접 써서 읽기나 쓰기를 수행해서는 절대로 안됨 (반드시 `fdevicedriver.c`의 함수를 사용해야 함)

## (4) `ftl_print()` 구현

일반적으로 FTL이 제공해야 할 함수는 `ftl_open()`, `ftl_write()`, `ftl_read()` 세 개뿐이며, `ftl_print()` 함수는 단지 FTL의 address mapping table을 확인하기 위한 용도로 사용하기 위한 것이다. 이 함수는 화면에 `lbn`, `pbn`, `last_offset`을 출력한다.

예: flash memory의 전체 블록의 수가 8인 경우 화면 출력 예시는 아래와 같다.

```
lbn pbn last_offset
0   7   0
1  -1  -1
2   1   3
3   2   1
4   3   4
5  -1  -1
6   4   2
```

**주의사항**:

- `ftl_print()` 함수를 호출하였을 때 반드시 위와 같은 출력 포맷을 사용해야 하며, 그렇지 않는 경우 채점 프로그램이 제대로 인식을 하지 못함 (숫자와 숫자 사이 스페이스 한 칸)
- address mapping table의 `pbn`은 free block liked list의 관리 정책과 맞물려 있기 때문에 반드시 `ftl_open()`에서 언급한 관리 정책을 따라야 함
