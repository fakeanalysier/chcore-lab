# Lab 3: User Processes

## Exercise 1

根据实验文档和代码注释的提示，分别在 `load_binary`、`init_thread_ctx` 和 `switch_context` 中填充代码即可。其中 `switch_context` 返回 `target_thread->thread_ctx`，因为这个指针指向的是为线程准备的内核栈中的一个位置，该位置的上面（更高地址处）是 `thread_ctx->ec.reg`，即所有寄存器的数值，而该位置下面是栈的空闲区域。当 `eret_to_thread` 中调用 `exception_exit` 时，会从 `target_thread->thread_ctx` 向上的区域中读出各寄存器的值（恢复上下文），然后使用 `eret` 切换到 EL0。

## Exercise 2

`process_create_root` 首先从附加进 kernel 的用户程序二进制（`binary_cpio_bin_start` 开始的一段内存）里面读取到 `bin_name` 对应的程序 ELF，到 `binary` 中，然后创建 root 进程，接着调用 `thread_create_main` 创建进程的主线程（第一个线程）。

`thread_create_main` 需要调用各种函数来初始化主线程，主要如下：

- `pmo_init` 为用户栈分配物理内存，由于是 capability-bases 访问控制，这里需要申请一个 PMO；
- `vmspace_map_range` 把虚拟地址映射到用户栈 PMO 对应的物理地址；
- `load_binary` 从前面读取到的程序 ELF 中加载实际的程序指令，得到程序的入口地址（`pc` 变量）；
- `prepare_env` 在用户栈上准备用户程序运行所需的 `argc`、`argv` 和 `envp` 参数，这些参数将在进入用户态后，由 `_start_c` 读取，并放入寄存器作为参数传入用户 `main` 函数；
- `thread_init` 为线程创建保存线程执行上下文的空间，同时也分配内核栈，两者连在一起，然后初始化线程上下文，把线程的用户栈、入口地址等存入对应寄存器。

`process_create_root` 创建 root 进程完毕后，将 `current_thread` 设置为该进程的主线程，以便随后 `switch_context` 切换过去。

## Exercise 3

在 `exception_table.S` 中使用 `exception_entry` 宏定义异常处理程序，全都是无条件跳转到 label 指定的位置，进行真正的异常处理（触发系统调用或 `handle_entry_c` 函数）。

在 `exception_init` 函数中调用 `set_exception_vector` 设置中断向量表，随后启用中断。这时除了系统调用之外的中断已经能够正确进入 `handle_entry_c` 函数。

`handle_entry_c` 中处理 `ESR_EL1_EC_UNKNOWN` 的情况，即遇到未知指令，根据文档提示输出相关信息并退出用户线程。

## Exercise 4

`exception_table.S` 中对系统调用进行了特殊处理，没有调用 `handle_entry_c`，而是从 `syscall_table` 函数指针数组中取得对应的函数并调用。

## Exercise 5

在 `syscall` 函数（用户态）中使用 inline asm 传入参数并通过 `svc 0` 指令触发系统调用，系统调用完成后，将 `x0` 返回。

系统调用的异常处理机制需要在 `syscall_table` 中找到系统调用处理函数，因此需要修改 `syscall_table` 中的项，设置系统调用号到函数指针的映射。并在用户态的 `usys_putc` 等函数中使用对应的系统调用号调用 `syscall` 函数。

## Exercise 6

只需简单地填写 `syscall_table` 即可。其中 `sys_putc` 需要调用 `uart_send` 实现发送，`sys_handle_brk` 由于需要先实现缺页异常处理才能测试，留到最后。

## Exercise 7

用户程序的 `main` 函数 `ret` 时，LR 寄存器为 0x0，即返回地址为 0x0，跳转到该地址将导致缺页异常，而目前还没有处理缺页异常，因此异常处理没有做任何事情就返回，然后重新尝试访问 0x0，如此循环。之所以此时 LR 是 0x0，是因为调用 `main` 的 `_start_c` 函数使用了无条件跳转指令 `b main`，而不是 `bl`。

之所以这里被编译成 `b main` 而不是 `bl main`，是因为 `START` 没有返回，`b _start_c` 是 `START` 最后一条指令，`main` 函数调用又是 `_start_c` 最后一条指令，因此即使 `main` 函数返回到了 `_start_c` 也是没有意义的（事实上他后面并没有实际指令可供执行），在 `-O3` 优化下（`compile_user.sh`），`bl` 被优化为了 `b`。为了验证这一点，把 `-O3` 改为 `-O0`，就会编译为 `bl main`，但这不会改变 `START` 结束后会进入无意义的内存区域的事实。

## Exercise 8

`_start_c` 中，在调用 `main` 函数之后，用其返回值作为参数调用 `usys_exit` 即可。

## Exercise 9

在 `handle_entry_c` 中，将 `ESR_EL1_EC_DABT_LEL` 和 `ESR_EL1_EC_DABT_CEL` 两种异常情况由 `do_page_fault` 处理。`do_page_fault` 根据具体情况会调用 `handle_trans_fault`，这里根据注释中的提示编写代码，找到缺页地址所在的虚拟地址空间块，然后为其分配一个物理页，并在页表中加上映射。

## Exercise 6 Bonus

`sys_handle_brk` 中，根据注释给出的提示，对 `addr` 等于 0 和 `addr` 大于、小于、等于当前堆顶的情况分别做处理。其中 `addr` 等于 0 时为进程分配堆区（参考 `thread_create_main` 和 `load_binary` 中分配 PMO 的代码），初始大小为 0；`addr` 大于当前堆顶时，直接对应地增加 `vmr->size` 和 `vmr->pmo->size`，并返回更新后的栈顶地址，不需要处理物理页相关的内容，因为缺页异常处理函数中会处理；另外几种情况较为简单。
