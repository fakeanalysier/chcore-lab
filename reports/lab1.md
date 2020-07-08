# Lab 1: Booting a machine

## Exercise 1

AArch64 与 x86-64 的区别：

- AArch64 相比 x86-64 有更多的通用寄存器，且命名统一，便于使用
- AArch64 只有 Load/Store 指令可以访问内存，且寻址方式相比 x86-64 更少，但它可以在访存之前或之后对基址寄存器进行修改；
- AArch64 要求栈 16 字节对齐
- AArch64 的算术运算指令、逻辑运算指令、位操作指令很丰富，支持一些常见的连续运算

## Exercise 2

启动时执行的第一个函数是 `_start`，位于 bootloader 的 0x80000 地址处，函数定义在 `boot/start.S` 文件，由 `build/linker.lds` 声明将其放在 0x80000 地址处。

## Exercise 3

`_start` 函数开头读取了 `mpidr_el1` 寄存器中的信息，判断除了第 31、30、24 位外是否都是 0，如果是 0，则表示当前核是主核，跳转到 `primary` 进行初始化，对于其它核，则在 `secondary_hang` 处死循环。

## Exercise 4

跳过。

## Exercise 5

`.init` 段的 VMA 等于 LMA，因为 bootloader 运行时页表还没有建立，内存访问直接使用物理地址。之后 `init_c` 函数调用 `init_boot_pt`，初始化了一个基本的页表，将 `KERNEL_VADDR`（等于 0xffffff0000000000）映射到物理地址从 0x0 开始的空间，因此 `.text` 段可以使用 0xffffff00000cc000 作为 VMA，而该段的代码实际加载到物理地址的 0x00000000000cc000 处。

## Exercise 6

代码。

## Exercise 7

初始化内核栈的代码在 `kernel/head.S` 的 `start_kernel` 函数，该函数首先获取了 `kernel_stack` 的地址（也即其中第 0 个栈的地址），然后加上栈大小 `KERNEL_STACK_SIZE` 得到栈底地址（高地址），设置到 `sp` 寄存器。

`kernel_stack` 定义在 `kernel/main.c` 的全局范围，是一个 `PLAT_CPU_NUM * KERNEL_STACK_SIZE` 的二维 `char` 数组，即每个 CPU 核有一个 `KERNEL_STACK_SIZE` 大小的栈，前面的 `start_kernel` 函数只有主核能执行到，因此使用了第 0 个栈。

编译器在编译 `kernel/main.c` 时将 `kernel_stack` 变量放置在 `.bss` 区，在 GDB 中打印 `kernel_stack` 的地址可以发现与 `kernel.img` 的 `.bss` 段的 VMA 范围一致。

## Exercise 8

每一次 `test_backtrace` 函数调用会往栈上 push 32 字节的内容（4 个 64 位字），最低的 2 个 64 位字分别是 `x29`（`fp`）和 `x30`（`lr`）的值，最高的 64 位字用于存放参数 `x`。

## Exercise 9

同 8。

## Exercise 10

代码。

## Bonus


