const std = @import("std");

const Cpu = std.Target.Cpu;
const Os = std.Target.Os;
const arm = std.Target.arm;

// Target Architecture = ARMv7-a
// Target CPU = Cortex-A8

pub fn build(b: *std.Build) void {
    const compilationTarget = std.Target{
        .cpu = arm.cpu.cortex_a8.toCpu(Cpu.Arch.arm),
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
            "-c",
            "-g",
            "-std=c11",
            // "-mthumb-interwork",
        },
    );

    navilos.addIncludePath("include");
    navilos.addIncludePath("hal");
    navilos.addIncludePath("lib");
    navilos.addIncludePath("kernel");

    navilos.setLinkerScriptPath(std.Build.FileSource{
        .path = "navilos.ld",
    });

    b.installArtifact(navilos);

    const run_navilos = b.addSystemCommand(&[_][]const u8{
        "qemu-system-arm",
        "-M",
        "realview-pb-a8",
        "-kernel",
        "./zig-out/bin/navilos",
        "-nographic",
    });
    run_navilos.step.dependOn(&navilos.step);

    const run_step = b.step("run", "Run navilos on qemu-system-arm");
    run_step.dependOn(&run_navilos.step);
}
