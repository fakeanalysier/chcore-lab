# Lab 4: Multiprocessing

## Question 1

前面 lab 中，只有 primary 处理器（从 `mpidr_el1` 寄存器读到处理器 ID 为 0 的）启动到了 `init_c`，其它处理器在 `_start` 中死循环了；本 lab 中，primary 处理器仍然正常启动到 `init_c`，而其它 secondary 处理器首先从 `MPIDR` 数组（定义在 `boot/smp.c`）中找到当前处理器 ID，用数组下标作为逻辑处理器 ID，保存在 `x8`，然后在 `boot_cpu_stack`（定义在 `boot/init_c.c`）中拿到属于自己的栈，设置到 `sp`，接着进入循环，等待 `secondary_boot_flag`（定义在 `boot/init_c.c`）中对应的标志被置一再继续启动到 `secondary_init_c`。

## Exercise 2

`enable_smp_cores` 中依次设置 `secondary_boot_flag[i]` 为 1，secondary 处理器在 `_start` 的 `hang` 循环中检测到置一后，进入启动流程，最终启动到 `secondary_start` 函数，为了保证 CPU 一个一个启动，`enable_smp_cores` 中置 flag 之后需要等待 CPU 启动完成，即 `cpu_status[i]` 不再是 `cpu_hang`，因此 `secondary_start` 中需要修改该 status，以便通知 primary 处理器。

## Question 3

每个 CPU 启动时，使用的内核栈、读写的状态变量都是独立的，调用的主要函数也都只操作对应的 CPU，因此不存在竞争条件，唯一可能出现竞争的是调用 `printk` 输出文本的地方，由于该函数内部多次调用 `uart_send`，不同 CPU 可能交替调用，导致输出的调试信息交叉在一起。实际测试将 `enable_smp_cores` 改为同时启动所有 CPU 后等待它们启动完成，各 CPU 都能正常启动，只在输出调试信息时发生交错。

## Exercise 4

根据 Ticket Lock 的原理编写即可。

## Exercise 5

在 `kernel_lock_init`、`lock_kernel`、`unlock_kernel` 中对 `big_kernel_lock` 的操作进行简单封装，然后根据实验文档的指示，在相应位置调用这些函数。

## Question 6

因为 `el0_syscall` 在获取锁后，需要调用系统调用处理函数，这些函数需要用到用户通过寄存器传递的参数，并且这些寄存器（`x0` 到 `x15`）是 caller-saved，如果不保存到栈，可能会被 `lock_kernel` 函数破坏；而 `exception_return` 中，释放锁后无需再使用寄存器中的数据，因此无需保存。

## Exercise 7

根据实验文档的提示实现 `rr_sched_enqueue`、`rr_sched_dequeue`、`rr_sched_choose_thread`、`rr_sched`，有一些文档没有详细说明的接口要求，比如函数在不同情况下的返回值等，通过查看测试代码做了判断。

## Exercise 8

在 `handle_irq` 中加入 `current_thread->thread_ctx->type == TYPE_IDLE` 的判断，使得内核态的 idle 线程被中断时，也会获取内核全局锁。

如果不获取的话，内核可能会卡住，这是因为 idle 线程虽然运行在内核态，但它是在没有用户线程可调度时运行的，也会通过 `eret_to_thread` 退出中断，从而释放此前进入中断处理函数时获得的锁，当再次发生中断时（此时在内核态），内核可能需要打断 idle 线程，切换到其它就绪线程执行，因此这里需要获取锁以保证 lock 和 unlock 操作配对。

## Exercise 9

`sys_get_cpu_id` 中使用 `smp_get_cpu_id` 获得当前 CPU ID；`sys_yield` 中调用调度器的 `sched`，然后 `eret_to_thread(switch_context())`，也即 `sys_exit` 结尾进行的重新调度操作。

## Exercise 10

取消注释掉 `exception_init_per_cpu` 中的 `timer_init` 调用后，虚拟机会定时产生时钟中断，进入 `handle_irq` 函数处理。该函数调用 `plat_handle_irq`，`plat_handle_irq` 判断是时钟中断后又调用 `handle_timer_irq`，这里面进行真正的时钟中断处理，在 `plat_handle_timer_irq` 调用之后接着调用 `sched_handle_timer_irq`，进入 RR 调度器的 `rr_sched_handle_timer_irq`。

接着实现调度逻辑，目前只需在每次时钟中断时重新调度即可，因此在 `rr_sched_handle_timer_irq` 里调用 `rr_sched`，然后在 `handle_irq` 结尾调用 `eret_to_thread(switch_context())` 切换线程。

在写后面的 exercise 时发现一个问题，当不运行任何用户程序，而是运行 kernel test 时，运行结果表现出不一致，即每次运行的结果可能不同，会卡在不同的位置，GDB 调试发现卡在了 `lock_kernel` 中，思考后意识到这种情况下不应该启动时钟，因此在调用 `timer_init` 前加上了 `#ifdef TEST` 判断，从而使 kernel test 不受时钟中断干扰。

## Exercise 11

之前的思路是在 `rr_sched_handle_timer_irq` 里根据情况调用 `rr_sched`，而 `rr_sched` 无论如何都会进行调度，但是在本练习中，测试代码的思路不是这样的。因此仔细研究了实验文档和测试代码的思路，发现需要在 `rr_sched_handle_timer_irq` 中只关心 budget，具体就是在能减少 budget 的情况下就减少，否则啥也不动，然后在 `rr_sched` 中，首先检查 budget，只有 budget 等于 0 时才会进行调度。

按这个思路，上一题中的 `handle_irq` 在 `eret_to_thread` 之前就需要调用 `sched()`，然后 `sys_yield` 需要先将 budget 置 0 才能调用 `sched()`。后者很不优雅，因为 budget 本应该是 RR 调度策略所需要关心的事情，`policy_rr.c` 文件之外都不应该访问。

## Exercise 12

在前面实现 `rr_sched_enqueue` 时已经完成。

## Exercise 13

在 `sys_set_affinity` 中设置 affinity，在 `sys_get_affinity` 中返回 affinity，有效代码都只有一行，但需要注意加上判断 thread 是否为空，否则函数结尾的 `obj_put` 会导致访问空指针，触发奇怪的缺页异常，难以 debug。

## Exercise 14

`spawn` 函数基本上是 `thread_create_main` 的用户态版本，因此根据实验文档和注释的提示，再参考 `thread_create_main` 即可。

## Exercise 15

根据实验文档和注释的提示编写代码，根据对应系统调用的参数和其中的逻辑理解需要传入的参数。

## Exercise 16

根据提示填写 `ipc_call.c` 和 `ipc_return.c` 中空缺的参数，通过理解 IPC 模块各函数所做的事情可以找到这些参数所保存的位置。这里有一个坑，`thread_migrate_to_server` 中要给 `arch_set_thread_next_ip` 传的参数是 IPC 服务器的 callback 函数的地址，注释的提示中说需要使用 `ipc_connection` 结构体的一个字段，也就是 `conn->callback`，但这个字段在建立 IPC 连接的 `create_connection` 并没有设置，应该是遗漏了，所以这一空要么就直接用 `conn->target->server_ipc_config->callback`，要么就需要在 `create_connection` 中加上一行给 `conn->callback` 赋值。

## Exercise 17

直接调用 `thread_migrate_to_server` 即可，把参数当作数值直接传给 server。

## Bonus

TODO
