# Navilos
Step by step RTOS for study embedded FW programming.

나빌로스는 임베디드 펌웨어 프로그래밍을 공부하기 위해 제작한 작고 간단한 RTOS입니다.

원본 소스코드는 https://github.com/navilera/Navilos 를 참조하세요.

# Experiments

## Porting `Makefile` to `build.zig`

Zig 빌드 시스템은 크로스 컴파일을 지원합니다.
Zig의 빌드 시스템을 실험삼아 이용해보기 위해서 Navilos의 `Makefile`을 `build.zig`로 포팅해보았습니다.

사용하는 Zig 버전은 `0.11.0-dev.3936+8ae92fd17`입니다.

우선, 우리가 무엇을 빌드하는 것인지 알아야합니다.
navilos의 Makefile을 이용해 `navilos.axf`를 빌드해봅시다.

```
$ make
$ file build/navilos.axf
build/navilos.axf: ELF 32-bit LSB executable, ARM, EABI5 version 1 (SYSV), statically linked, with debug_info, not stripped
```

32비트, 리틀엔디언, ARM 아키텍처의 정적 링크된 ELF입니다.
운영체제 그 자체이므로 당연히 정적 링크되어 있습니다.

어떤 컴파일러를 사용했을까요? Makefile을 한번 열어봅시다.

```makefile
ARCH = armv7-a
MCPU = cortex-a8

TARGET = rvpb

CC = arm-none-eabi-gcc
AS = arm-none-eabi-as
LD = arm-none-eabi-gcc
OC = arm-none-eabi-objcopy

CFLAGS = -c -g -std=c11

LDFLAGS = -nostartfiles -nostdlib -nodefaultlibs -static -lgcc

...


build/%.o: %.c
	mkdir -p $(shell dirname $@)
	$(CC) -march=$(ARCH) -mcpu=$(MCPU) -marm $(INC_DIRS) $(CFLAGS) -o $@ $<
```

정리해보면
1. ARMv7-A 아키텍처의 Cortex-A8 프로세서를 타겟으로 합니다.
2. 운영체제를 거치지 않고 CPU에 의해 바로 실행되므로  `-nostartfiles -nostdlib -nodefaultlibs` 옵션을 사용합니다.
    1. `-nostartfiles`: 표준 시스템 startup 파일을 사용하지 않습니다. (`_start` 등의 함수가 추가되지 않습니다.)
    2. `-nostdlib`: 표준 시스템 startup 파일과 라이브러리를 사용하지 않습니다. 링커에 명시적으로 전달한 라이브러리만 링크합니다.
    3. `-nodefaultlibs`: 표준 시스템 라이브러리를 링크하지 않습니다.
3. `-static`: 정적 링크합니다.
4. `-lgcc`: `libgcc`를 링크합니다. `-nostdlib`과 `-nodefaultlibs`를 사용할 때 필요합니다. `libgcc`는 타켓 머신이 바로 수행할 수 없는 연산을 수행할 수 있도록 해주는 런타임 라이브러리입니다. (참조: https://gcc.gnu.org/onlinedocs/gccint/Libgcc.html)

이제 이를 `build.zig`로 옮겨 봅시다.
제일 먼저 컴파일 타겟을 설정합니다.

```zig
const cpu = arm.cpu.cortex_a8.toCpu(Cpu.Arch.arm);
const compilationTarget = std.Target{
    .cpu = cpu,
    .os = Os{
        .tag = std.Target.Os.Tag.freestanding,
        .version_range = Os.VersionRange.default(Os.Tag.freestanding, Cpu.Arch.arm),
    },
    .abi = std.Target.Abi.eabi,
    .ofmt = std.Target.ObjectFormat.elf,
};
const crossTarget = std.zig.CrossTarget.fromTarget(compilationTarget);

const target = b.standardTargetOptions(.{
    .default_target = crossTarget,
});

const optimize = b.standardOptimizeOption(.{});
```

1. `cortex-a8` CPU 모델을 이용해 `cpu`를 설정합니다.
2. 운영체제는 `freestanding`으로 설정합니다. 의존하는 운영체제가 없습니다. Zig 빌드 시스템은 운영체제가 `freestanding`으로 설정되면 `-nostartfiles -nostdlib -nodefaultlibs` 옵션을 사용한 것처럼 표준 startup 파일이나 라이브러리를 사용하지 않습니다.
3. ABI는 `eabi`로 설정합니다. `eabi`는 임베디드 환경을 위한 ABI입니다. (동적 링크를 사용할 수 없습니다.)
4. `-lgcc`는 어떻게 해결할까요? Zig 빌드 시스템은 링크 시 `compiler_rt`를 사용해 필요한 경우에만 링크합니다. `compiler_rt`는 LLVM 생태계에서 `libgcc`와 비슷한 일을 하는 라이브러리입니다. (프로세서가 지원하지 않는 연산을 수행 등).



navilos는 여러 파일을 컴파일해 얻는 실행파일입니다.
아래와 같이 설정하면 각 파일을 컴파일한 뒤 링킹해 최종 ELF를 얻을 수 있습니다.
각 파일을 컴파일해 얻은 오브젝트 파일은 `zig-cache` 디렉터리에서 찾을 수 있습니다.

```zig
const navilos = b.addExecutable(.{
    .name = "navilos",
    .target = target,
    .optimize = optimize,
});

navilos.addAssemblyFile("boot/Entry.S");
navilos.addCSourceFiles(
    &.{
        "boot/Handler.c",
        "boot/Main.c",

        "kernel/Kernel.c",
        "kernel/task.c",
        "kernel/event.c",

        "lib/armcpu.c",
        "lib/stdio.c",
        "lib/stdlib.c",
        "lib/switch.c",

        "hal/rvpb/Uart.c",
        "hal/rvpb/Timer.c",
        "hal/rvpb/Regs.c",
        "hal/rvpb/Interrupt.c",
    },
    &.{
        "-std=c11",
        // "-mthumb-interwork",
    },
);

navilos.addIncludePath("include");
navilos.addIncludePath("hal");
navilos.addIncludePath("lib");
navilos.addIncludePath("kernel");
```

링커 스크립트를 설정합니다.
현재 타겟 CPU는 전원이 들어오면 0x00000000에 있는 리셋 벡터를 읽기 때문에
해당 주소에 부트를 위한 코드를 위치시켜야 합니다.

링커 스크립트를 통해서 코드의 위치를 정해줍시다.

```zig
navilos.setLinkerScriptPath(std.Build.FileSource{
    .path = "navilos.ld",
});

b.installArtifact(navilos);
```

```ld
// navilos.ld
ENTRY(vector_start)
SECTIONS
{
	. = 0x0;
	
	
	.text :
	{
		*(vector_start)
		*(.text .rodata)
	}
	.data :
	{
		*(.data)
	}
	.bss :
	{
		*(.bss)
	}
}
```

그 다음 실행과 디버깅을 위한 명령어를 추가해줍니다.
```zig
const run_navilos = b.addSystemCommand(&[_][]const u8{
    "qemu-system-arm",
    "-M",
    "realview-pb-a8",
    "-nographic",
    "-kernel",
});
run_navilos.addArtifactArg(navilos);
run_navilos.step.dependOn(&navilos.step);

const run_step = b.step("run", "Run navilos on qemu-system-arm");
run_step.dependOn(&run_navilos.step);

const debug_navilos = b.addSystemCommand(&[_][]const u8{
    "qemu-system-arm",
    "-M",
    "realview-pb-a8",
    "-nographic",
    "-S",
    "-gdb",
    "tcp::1234,ipv4",
    "-kernel",
});
debug_navilos.addArtifactArg(navilos);
debug_navilos.step.dependOn(&navilos.step);

const debug_step = b.step("debug", "Debug navilos on qemu-system-arm");
debug_step.dependOn(&debug_navilos.step);
```

`zig build`, `zig build run`, `zig build debug`를 통해 빌드, 실행, 디버깅 실행할 수 있습니다.

한번 navilos를 빌드해봅시다.

```
$ file zig-out/bin/navilos
zig-out/bin/navilos: ELF 32-bit LSB executable, ARM, EABI5 version 1 (SYSV), statically linked, with debug_info, not stripped
```

빌드가 잘 됩니다!

실행해보면 어떨까요?

```
$ zig build run

pulseaudio: set_sink_input_volume() failed
pulseaudio: Reason: Invalid argument
pulseaudio: set_sink_input_mute() failed
pulseaudio: Reason: Invalid argument
vutsrqponmlkjihgfedcbazyxwvutsrqponmlkjihgfedcbazyxwvutsrqponmlkjihgfedcbazyxwvutsrqponmlkjihgfedcba
Hello World!
Hello printf
output string pointer: printf pointer test
```

Printf_test에서 작동을 멈춰버렸습니다. 어디서 문제가 발생한 걸까요?

일단 컴파일이 잘된건지 확인해봅시다.

```
$ arm-none-eabi-objdump -h zig-out/bin/navilos

zig-out/bin/navilos:     file format elf32-littlearm

Sections:
Idx Name          Size      VMA       LMA       File off  Algn
  0 .ARM.exidx    00000010  00000000  00000000  00010000  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  1 .rodata.str1.1 000001d4  00000010  00000010  00010010  2**0
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  2 .text         00002b94  000001e4  000001e4  000101e4  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  3 .text.__aeabi_uidiv 00000084  00002d78  00002d78  00012d78  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  4 .text.__udivmodsi4 00000098  00002dfc  00002dfc  00012dfc  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  5 .text.__aeabi_uidivmod 0000001c  00002e94  00002e94  00012e94  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  6 .data         00000010  00002eb0  00002eb0  00012eb0  2**2
                  CONTENTS, ALLOC, LOAD, DATA
  7 .bss          00000a14  00002ec0  00002ec0  00012ec0  2**2
                  ALLOC
...


$ arm-none-eabi-objdump -h build/navilos.axf

build/navilos.axf:     file format elf32-littlearm

Sections:
Idx Name          Size      VMA       LMA       File off  Algn
  0 .text         0000188c  00000000  00000000  00000054  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  1 .data         00000010  0000188c  0000188c  000018e0  2**2
                  CONTENTS, ALLOC, LOAD, DATA
  2 .bss          00000a14  0000189c  0000189c  000018f0  2**2
                  ALLOC
...
```

`zig build`의 결과, `make`의 결과와 다르게 `.text` 섹션의 위치가 다릅니다.
`.text` 섹션의 위치가 0x00000000가 아닌 0x000001e4로 올라갔습니다.
`arm-none-eabi-gcc`를 사용했을 때에는 링커 스크립트를 위처럼만 작성해도
`.text` 섹션이 0x00000000에 위치했지만, `zig build-exe`를 사용했을 때에는
`.text` 섹션이 0x00000000에 위치하지 않습니다. 링커 스크립트를 좀 더 엄격하게 작성해봅시다.

```ld
MEMORY {
	MEM (rwx): ORIGIN = 0x0, LENGTH = 128M
}

ENTRY(vector_start)
SECTIONS
{
	. = 0x0;
	
	
	.text :
	{
		*(vector_start)
		*(.text .rodata)
	} > MEM
	.data :
	{
		*(.data)
	}
	.bss :
	{
		*(.bss)
	}
}
```

`qemu-system-arm`이 기본제공하는 메모리는 128MB이기 때문에 `MEM` 영역을 128M로 설정합니다.
0x00000000에서 시작하는 `MEM` 영역에 `.text`를 위치시킵니다.
다시 빌드해봅시다.
```
$ zig build
$ arm-none-eabi-objdump -h zig-out/bin/navilos

zig-out/bin/navilos:     file format elf32-littlearm

Sections:
Idx Name          Size      VMA       LMA       File off  Algn
  0 .text         00002b94  00000000  00000000  00010000  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  1 .text.__aeabi_uidiv 00000084  00002b94  00002b94  00012b94  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  2 .text.__udivmodsi4 00000098  00002c18  00002c18  00012c18  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  3 .text.__aeabi_uidivmod 0000001c  00002cb0  00002cb0  00012cb0  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  4 .ARM.exidx    00000010  00002ccc  00002ccc  00012ccc  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  5 .rodata.str1.1 000001d4  00002cdc  00002cdc  00012cdc  2**0
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  6 .data         00000010  00002eb0  00002eb0  00012eb0  2**2
                  CONTENTS, ALLOC, LOAD, DATA
  7 .bss          00000a14  00002ec0  00002ec0  00012ec0  2**2
                  ALLOC
...
```

`.text` 섹션이 0x00000000로 잘 이동했습니다!

다시 실행해볼까요.

```
$ zig build run

pulseaudio: set_sink_input_volume() failed
pulseaudio: Reason: Invalid argument
pulseaudio: set_sink_input_mute() failed
pulseaudio: Reason: Invalid argument
vutsrqponmlkjihgfedcbazyxwvutsrqponmlkjihgfedcbazyxwvutsrqponmlkjihgfedcbazyxwvutsrqponmlkjihgfedcba
Hello World!
Hello printf
output string pointer: printf pointer test
(null) is null pointer, 10 number
5 = 5
dec=255 hex=FF
print zero 0
SYSCTRL0 0
printf works!
current count : 2
current count : 1002
current count : 2002
User_task0: SP = 0x8FFFF0
User_task1: SP = 0x9FFFF0
User_task2: SP = 0xAFFFF0
a
Event Handled by Task0

Event Handled by Task1
s
Event Handled by Task0

Event Handled by Task1
d
Event Handled by Task0

Event Handled by Task1
n
Event Handled by Task0

Event Handled by Task1
```

1. `putstr`이 잘 작동합니다.
2. `debug_printf`도 잘 작동합니다.
3. 타이머도 잘 작동하고 있습니다.
4. 유저 태스크 스택 초기화도 잘 일어나고 있습니다.
5. 유저 태스크 컨텍스츠 스위칭도 잘 일어나고 있습니다.
6. 인터럽트와 이벤트도 잘 작동하고 있습니다.

이제 잘 실행됩니다! (`.text`가 0x00000000에 위치하지 않았을 때 왜 실행되다가 멈췄는지 모르겠네요, 좀 더 살펴보아야 합니다.)