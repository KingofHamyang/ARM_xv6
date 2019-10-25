# ARM_xv6

Porting [xv6-public](http://github.com/mit-pdos/xv6-public.git) into ARMv7 Cortex-A9 platform.

## Required packages

To run this project, you need these packages.

- `ninja`: For build this project.
- `qemu-system-arm`: Emulate ARM platform.
- `arm-none-eabi-binutils`: Compiler Collections.
- `arm-none-eabi-gdb`: For debugging. (optional)

(Some packages' name might be different for your platform. Above packages' name for Archlinux.)

## How to run this project

run this command in shell for build project:
```
$ ninja
```

run this command for run:
```
$ ninja qemu
```
(You don't need to build first, `ninja` will automatically detect build state and build if need.)

run this command if you want to clean previous build:
```
$ ninja -t clean
```

if you want to force clean, just remove `out/` directory:
```
$ rm -rf out/
```

## How can I debug this?

You need two terminal windows. (`tmux` or `screen` also works.)

in Terminal 1, run this command:
```
$ ninja qemu-gdb
```

After then, in Terminal 2, run this command:
```
$ ninja gdb
```

Then, type this in gdb console:
```
(gdb) target remote :12345
```

You can now use gdb as debugger.

1. Type `c` to run as normal.
2. Type `b (function_name)` to set breakpoint in kernel source code.
3. Type `file out/(user_exec_file)`, `b main`, `c` for debugging user space executables. (Don't forget to type `file out/kernel.elf` to return kernel debugging!)
4. Type `watch (variable)` to set breakpoint variable value change, or `watch (condition)` to set breakpoint when given `condition` fits during running.

## How can I see dependency graph?

`ninja` can show dependency graph using `graphviz` for given target.

1. If you want to see it, you need to install `graphviz` package.
2. Then, type `ninja graph.png` for see  dep. graph for `kernel.elf`.
3. If you want to see other dependency graph, run this command:
```
$ ninja -t graph (target) | dot -Tpng -o (output_png_file_name)
```
