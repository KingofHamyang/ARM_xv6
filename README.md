# xv6의 부팅 과정 이해하기

xv6를 arm 시스템에 포팅하기 전, xv6 부팅 과정을 이해할 필요가 있어 보인다.

## 아키텍쳐 정하기

ARMv7 Cortex-A9 CPU를 타겟으로 정하였다.

xv6에서 구현된 bootloader 부터 Page table, Interrupt, Trap frame, Inline assembly까지 다 바꿔야 함.

Makefile부터 시작해서 어떻게 부팅이 이루어 지는지부터 살펴봐야 함.

## Makefile 살펴보기

시스템 운영체제 과목 수강 당시 xv6를 부팅하기 위해서는 다음 명령어를 입력했었음.

```
$ make qemu-nox
```

이걸 `Makefile`을 살펴보면서 실제 `make`를 통해 이루어지는 명령은 다음과 같음.

```
$ qemu-system-i386 \
-drive file=fs.img,index=1,media=disk,format=raw \
-drive file=xv6.img,index=0,media=disk,format=raw \
-smp 2 \
-m 512
```

하나씩 살펴보면서 알아보자.

- `-drive` : 생성한 file을 디스크로 사용하는 옵션. `fs.img`에 user program, `xv6.img`에 xv6 kernel image를 넣음.
- `-smp` : CPU의 코어 갯수를 지정. 기본값은 2개.
- `-m` : RAM 크기를 지정. 기본값은 512MB.

`fs.img`가 어떻게 생성되는지는 `Makefile`:185에 있다.

```
fs.img: mkfs README $(UPROGS)
    ./mkfs fs.img README $(UPROGS)
```

Host에서 `mkfs`라는 프로그램을 만들어서 `README` 문서와 함께 user program을 집어넣은 `fs.img`라는 파일을 생성함을 알 수 있다.

이번에는 `xv6.img`가 어떻게 생성되는지 알아보자. `Makefile`:93에 있다.

```
xv6.img: bootblock kernel
	dd if=/dev/zero of=xv6.img count=10000
	dd if=bootblock of=xv6.img conv=notrunc
	dd if=kernel of=xv6.img seek=1 conv=notrunc
```

`dd`를 통해 `xv6.img` 파일을 생성함을 알 수 있다. 하나하나 뜯어보자.
- `if=/dev/zero` : `/dev/zero`에서 긁어온다는 의미, 즉, zero-fill의 의미.
- `count` : 10000번 복사한다는 의미. 기본 블록단위는 512B, 즉 약 5MB 정도의 파일을 생성.
- `conv=notrunc` : 파일이 있어도 지우지 않고 그 위에 덮어 쓴다는 의미.
- `seek=1` : 1번 블록을 먼저 쓴다는 의미.

즉, 저 구문을 1줄씩 해석하면 다음과 같음.

1. xv6.img를 약 5MB정도 zero-fill해서 생성.
2. xv6.img의 0번 블록에 `bootblock`을 덮어쓰기.
3. xv6.img의 1번 블록에 xv6 kernel을 덮어쓰기.

여기서 드는 의문 한가지. __bootblock이 0번 블록보다 커지면 안되지 않나?__

답은 Yes. `Makefile`:103에 `bootblock`을 생성하는 법이 있다.

```
bootblock: bootasm.S bootmain.c
	$(CC) $(CFLAGS) -fno-pic -O -nostdinc -I. -c bootmain.c
	$(CC) $(CFLAGS) -fno-pic -nostdinc -I. -c bootasm.S
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 -o bootblock.o bootasm.o bootmain.o
	$(OBJDUMP) -S bootblock.o > bootblock.asm
	$(OBJCOPY) -S -O binary -j .text bootblock.o bootblock
	./sign.pl bootblock
```

1, 2번째 줄에 있는 `-nostdinc`란, Standard C Library를 사용하지 않는다는 의미이다.

각설하고, 마지막 줄의 `sign.pl`은 perl 스크립트이다. 문법은 몰라도 대충 보면 아니까 열어보면,

```
#!/usr/bin/perl

open(SIG, $ARGV[0]) || die "open $ARGV[0]: $!";

$n = sysread(SIG, $buf, 1000);

if($n > 510){
  print STDERR "boot block too large: $n bytes (max 510)\n";
  exit 1;
}
```

`sign.pl`은 booting 블록임을 알려주기 위해 해당 블록에 sign을 하도록 만든 perl 스크립트이다.

그래서 signing을 시도하기 전에 만들어놓은 `bootblock` 사이즈가 너무 커버리면, 에러를 내뱉어버린다.

다음은 `bootblock`이 생성되기 전 필요한 요소들을 살펴보자. `bootasm.S`와 `bootmain.c`이다.

이 둘은 [xv6-book](https://pdos.csail.mit.edu/6.828/2014/xv6/book-rev8.pdf)의 Appendix B에도 설명이 되어 있다.

## 부트로더 살펴보기

요약하자면, xv6의 bootloader는 이 둘로 이루어져있다고 설명이 되어 있으면서 각 코드에 대해서 설명하고 있다.

먼저 `bootasm.S`를 살펴보자. 제일 먼저 `bootblock`의 시작부분을 참고하고 있기 때문이다.

```
.code16                       # Assemble for 16-bit mode
.globl start
start:
  cli                         # BIOS enabled interrupts; disable
  ...
```

1. `cli` Assembly 명령을 통해 먼저 interrupt를 disable한다. (부팅중 interrupt가 걸리면 곤란)
2. 다음은 차례로 `ax`를 0으로 초기화하고(`xorw %ax,%ax`)
3. `ds`, `es`, `ss`를 초기화시킨다.(`movw %ax,%_s`)
4. 그 다음 코드는... 이해를 못했다ㅠㅠ 23~37줄을 이해 못했으나 대충 초기화 시켜준다는 의미인것 같다. 얼핏 들었을 때 ARM에서 부팅을 시도할때는 pin 0, pin1의 HI,LO 상태에 따라서 부팅이 달라진다는 것을 들었는데, 그것과 관련이 있어 보인다.
5. 여튼 42줄로 가면, `lgdt gdtdesc`를 실행하는데, 이게 x86 프로세서를 `real mode`에서 `protected mode`로 전환시키는 역할을 한다.
6. 이후, 43-45줄은 cr0에 CR0_PE 플래그를 set하는 역할을 한다. 해당 전처리문은 `mmu.h`:8에 있다.
7. 이제, `real mode`가 끝나고 `protected mode`로 전환되었으니,32bit 코드를 실행시키러 가자. `ljmp $(SEG_KCODE << 3), $start32`가 `long jmp`를 실행시키며 그 과정에서 `cs`와 `eip`를 초기화 시켜줄 것이다.

쨔잔, 16bit 모드를 벗어나 이제 32bit에 들어섰다.

8. `protected mode`에 대한 세팅을 해주어야 한다. `ds`, `es`, `ss`는 사용할 준비가 되었지만, `fs`, `gs`는 사용할 준비가 되지 않았다(?).
9. `sp` 레지스터를 셋팅해주고 `bootmain` 함수를 실행시킨다.(`bootmain.c`:18)

잠깐동안이지만, 어셈블리에서 벗어났다. 이제 `bootmain.c`를 살펴보자.

1. (20-25) 시작시 변수 초기화 부분
2. (28-32) 디스크의 가장 첫 페이지를 읽는다. 만일 ELF 매직코드가 없으면 정상적인 파일이 아니므로 에러 처리를 위해 함수를 리턴한다.
3. (35-42) 각 커널 세그먼트를 변수들에 로딩하여 사용할 준비를 한다.
4. (46-47) ELF 헤더로부터 `entry` 함수(`entry.S`:45)의 진입점에 진입한다.

## entry.S 살펴보기

드디어, xv6를 메모리에 로딩완료 했다. 이제는 남은 코드를 토대로 장치들을 초기화 하는 부팅 과정일 뿐이다.

1. (`entry.S`:45-49) 함수 진입점에 도달했다. 먼저, 4MB Page를 생성하기 위해 Page Size Extension을 켜준다.
2. (`entry.S`:51-52) 페이지 디렉토리를 `main.c` 에 미리 정의해 둔 `entrypgdir`로 설정한다.
3. (`entry.S`:54-56) 페이징을 켜준다. 이 시점부터 Address Translation을 사용한다.
4. (`entry.S`:59-67) `esp` 레지스터, 즉 스택 포인터를 커널 스택에 맞춰주고, `main` 함수(`main.c`:18)를 실행시킨다.

여기서 의문점이 든다. 왜 부팅 시작파트에서는 Page Size Extension을 사용했을까?

## Page Size Extension

* 정확한 내용은 [Page Size Extension 위키피디아 페이지](https://en.wikipedia.org/wiki/Page_Size_Extension)를 참조하면 정확할 것이다.

![x86의 일반적인 Address Translation 과정](/img/x86_Paging_4K.jpg)

위 그림은 통상적으로 알고있는 x86 아키텍쳐에서의 Address Translation 방식이다.

이는 `mmu.h` 및 `vm.c` 를 살펴본다면 어떤 방식으로 이루어지는지를 알 수 있을 것이다.

x86 아키텍쳐에서 지원하는 Page Size Extension은 다음 그림을 보면 훨씬 잘 이해할 수 있다.

![x86의 Page Size Extension이 켜져있을 때의 Address Translation 과정](/img/x86_Paging_4M.jpg)

기존의 Address Translation은 2-level Translation이었으나 Page Size Extension이 켜지게 되면 임시로 Second-level Translation Table을 Page offset에 합병시켜 처리한다.

따라서, 기존의 First-level Translation Table은 Entry의 갯수는 1024개로 기존과 같이 이용할 수 있으며, Page size만 4MB로 거대해 지는 셈이다.

여기서, 우리는 왜 xv6에서는 Page Size Extension을 켰는지 알아볼 필요가 있다.

## PSE를 킨 이유?

우리는 xv6가 부팅하고 나서는 Address Translation 과정에서 Page Size Extension을 전혀 사용하지 않는다는 것을 이미 알고 있다.

앞으로 두번 다시 쓰지 않을 Page Size Extension을 부팅 극초기에 한해서 켜준 이유에 대해서 생각해 볼 필요가 있다.

이를 위해 잠깐 `main.c` 의 103줄을 주목해보자.

```
...
__attribute__((__aligned__(PGSIZE)))
pde_t entrypgdir[NPDENTRIES] = {
  // Map VA's [0, 4MB) to PA's [0, 4MB)
  [0] = (0) | PTE_P | PTE_W | PTE_PS,
  // Map VA's [KERNBASE, KERNBASE+4MB) to PA's [0, 4MB)
  [KERNBASE>>PDXSHIFT] = (0) | PTE_P | PTE_W | PTE_PS,
};
...
```

위 코드를 보면 알 수 있듯, `main` 함수가 모든 4K page들을 정리하기 위해서 사용할 스택 공간을 확보하기 위해 4MB짜리 스택 페이지 단 하나만을 0x0번지 주소에 할당한 것을 알 수 있다.

따라서, `main` 함수가 실행되고 `kvmalloc` 함수가 실행되어 kernel page directory table이 정의되기 전까지는 임시로 `entrypgdir`을 사용하는 것을 알 수 있다.

## PSE에 대한 정리

xv6의 `main` 함수가 실행되고, `kinit1` 함수가 실행되어 `kpgdir`이 할당되기 전까지는 `entry`에서 걸어준 `entrypgdir` 을 PSE와 함께 사용하게 된다.

물론 `kpgdir` 이 초기화 된 이후에는 `entrypgdir` 을 사용할 이유가 전혀 없기 때문에, 그 이후부터는 버려지게 된다.

`kpgdir`로 변경될 때, PSE는 여전히 켜져 있는 상태이지만 PS bit가 항상 0으로 설정되므로 이는 곧 PSE를 켜기만 하고 활용하지는 않는 셈이 되는 것이다.

`entrypgdir` 이 제 역할을 마치고 `kpgdir` 에 자리를 양보할 때부터, xv6는 본격적인 부팅을 주도하기 시작하는 셈이다.



# xv6에 ARM 포팅하기

x86 플랫폼으로 작성된 xv6 코드를 이해함과 동시에 ARM에서는 어떻게 부팅을 하면 좋을 지에 대해서 고민해보았다.

## 간단한 Toy 프로그램으로 QEMU 부팅시키기

(참고: [Qemu flash boot up does not work](https://stackoverflow.com/questions/26203514/qemu-flash-boot-up-does-not-work) 에서 첫 번째 답변)

간단한 어셈블리 프로그램을 만들어보자. 먼저 의존 프로그램을 설치하자 (Ubuntu 18.04 기준).

```
$ sudo apt update
$ sudo apt install gdb-multiarch gcc-arm-none-eabi qemu
```

설치가 완료되고 나면, 간단한 ARM Assembly로 된 Toy 프로그램을 작성해보자. (`add.S`)

```
.globl _start
_start:
        mov r0, #2
        mov r1, #3
        add r2, r1, r0
stop:   b stop
```

`r0` 레지스터에 2를, `r1` 레지스터에 3을 넣고, `r2` 레지스터에 두 값을 더해넣는 아주 간단한 코드이다.

이제 이것을 컴파일해서 작성해보자.

```
$ dd if=/dev/zero of=flash.bin bs=4096 count=4096
$ arm-none-eabi-as -o add.o add.S
$ arm-none-eabi-ld -Ttext 0x0 -o add.elf add.o
$ arm-none-eabi-objcopy -O binary add.elf add.bin
$ dd if=add.bin of=flash.bin bs=4096 conv=notrunc
```

1. `dd`를 통해 부팅할 flash 파일을 만든다. 기본 블록사이즈는 4096, count 역시 4096으로 설정해 16MB정도 크기로 만들어주었다.
2. ARM 툴체인 중 `arm-none-eabi-as` 으로 어셈블리 파일을 object 파일로 변환한다.
3. `arm-none-eabi-ld` 로 출력된 object 파일을 링커로 변환하여 elf 파일로 변환한다. `-Ttext`옵션은 `.text` 섹션만 추출해서 0x0번지에 저장하는 역할을 해준다.
4. 하지만 우리가 만든 elf 파일은 바이너리가 들어있긴 하지만 ELF 포맷이 되어 있기 때문에 직접 부팅할 수 없다. 이를 벗겨내고 실행 부분만 만들기 위해서 `arm-none-eabi-objcopy` 를 사용한다.
5. 마지막으로, 우리가 최종적으로 만든 실행 파일인 add.bin을 flash.bin에 덮어씌우자. 플래시 메모리를 타겟으로 하므로 기본 블록사이즈는 4096으로, 옵션으로 `conv=notrunc` 를 추가하면 파일 위에 덮어씌우게 된다.

마지막으로, QEMU를 실행해서 실제 에뮬레이션 보드에 올려보아 부팅이 되는지 확인해보자.

```
$ qemu-system-arm -M connex -cpu cortex-a9 -nographic -pflash flash.bin -gdb tcp::12345 -S
```

디버깅을 하는 방법은 다음과 같다.

```
$ gdb-multiarch
```

일반 `gdb`를 실행시키면 x86 플랫폼으로 간주하기 때문에 작동하지 않는다. 반드시 `gdb-multiarch`를 실행시켜야 한다.

이후 다음을 쳐보면 된다.

```
(gdb) target remote :12345
(gdb) info register
(gdb) step into
(gdb) info register
(gdb) step into
(gdb) info register
(gdb) step into
(gdb) info register
(gdb) step into
(gdb) info register
```

`target remote :12345`는 tcp 포트 12345에 gdb를 원격 접속한다는 의미이다.

`info register`는 현재 레지스터 상태가 어떤지 알아보는 명령이다.

`step into`는 한 단계 코드를 실행하고 멈추라는 명령이다.

여기까지 제대로 하였다면 다음을 확인할 수 있다.
- `r0` 레지스터에는 2가 저장
- `r1` 레지스터에는 3이 저장
- `r2` 레지스터에는 둘의 합인 5가 저장
- `step into`를 여러번 진행, 또는 `continue`를 실행하여도 `pc` 레지스터 값이 0xc로 고정.

이렇게 작동하면 정상 작동하는 것이다. 우리는 QEMU에 우리가 원하는 Toy 프로그램을 포팅하였다.

## 최초 프로세서 상태 확인 및 xv6 원본 코드 분석

우리가 만든 Toy 프로그램을 실제 운영체제로 완성시키기 위해 무엇을 해야할 지 곰곰히 생각해 보았고 다음 두 가지를 파악해야 한다.

1. 부팅 직후 CPU 레지스터를 포함한 모든 현재상태.
2. 실제 xv6(또는, 필요시, Linux Kernel)이 시작할 때 해주는 것들.

1번은 우리가 만든 Toy 프로그램을 통해 파악하도록 만들어 보자.

### 부팅 직후 CPU 레지스터를 포함한 현재 상태 파악하기.

`add.S` 파일을 수정해 보자.

```
.globl _start
_start:
	mrc p15, 0, r0, c1, c0, 0
loop:	b loop
```

여기서 `mrc p15, 0, r0, c1, c0, 0`란, Co-Processor의 레지스터를 CPU 레지스터로 읽어오는 역할을 한다.

반대로 CPU 레지스터 값으로 Co-Processor 레지스터로 프로그래밍 하고 싶다면 `mcr`명령을 내리면 된다.

작성한 Toy 프로그램이 정상적으로 작동한다면, `r0` 레지스터에는 `cp15`, 즉 MMU의 `c1`(1번) 레지스터 값이 r0에 들어올 것이다.

Toy 프로그램을 다시 한번 같은 방법으로 포팅해본 뒤, `gdb-multiarch`로 레지스터 상태를 살펴보면 다음과 같은 확인해 볼 수 있다.

```
r0      0x78    120
...
cpsr    0x400001d3      1073742291
```

먼저 `cpsr`이란 Current Program Status Register, 현재 프로그램 상태 레지스터를 의미한다. x86에서의 `eflags`와 비슷한 기능을 한다. 

`cpsr`의 비트맵은 다음과 같다.

![CPSR bitmap](/img/cpsr.png)

우리가 만든 Toy 프로그램으로 뽑아낸 `cpsr`값에서, 컨트롤을 담당하는 하위 8bit만 확인해보면 다음과 같다.

| I | F | T | M4 | M3 | M2 | M1 | M0 |
|---|---|---|---|---|---|---|---|
| 1 | 1 | 0 | 1 | 0 | 0 | 1 | 1 |

보다시피, IRQ와 FIQ가 기본적으로 활성화 되어있음을 알 수 있다. 

또한, `r0`에 저장되어 있는 MMU의 `c1` 레지스터 값을 분석해 보자. ARM MMU의 `c1` 레지스터의 비트맵은 다음과 같다.

![MMU Control Register bitmap](/img/mmu.PNG)

(출처: [ARM11 MPCore Processor Technical Reference Manual, 3.4.7. c1, Control Register Table 3.20. Control Register bit functions](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0360e/BABGDHIF.html))

실제 비트맵 비교는 생략하지만, SBO(Should Be One) 비트를 제외하고는 모두 비활성화 되어 있음을 알 수 있다.

이후 MMU를 사용하고 싶을 때 레지스터 값을 변경하면 될 것이라 생각이 된다.

### 실제 xv6 및 Linux Kernel에서 부팅시에 해주게 되는 작업

(작성중)
