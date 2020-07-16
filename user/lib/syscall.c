#include <lib/print.h>
#include <lib/syscall.h>
#include <lib/type.h>

u64 syscall(u64 sys_no, u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4,
	    u64 arg5, u64 arg6, u64 arg7)
{
	u64 ret = 0;
	/*
	 * Lab3: Your code here
	 * Use inline assembly to store arguments into x0 to x7, store syscall number to x8,
	 * And finally use svc to execute the system call. After syscall returned, don't forget
	 * to move return value from x0 to the ret variable of this function
	 */
	asm volatile("mov x8, %1\n\t"
		     "mov x0, %2\n\t"
		     "mov x1, %3\n\t"
		     "mov x2, %4\n\t"
		     "mov x3, %5\n\t"
		     "mov x4, %6\n\t"
		     "mov x5, %7\n\t"
		     "mov x6, %8\n\t"
		     "mov x7, %9\n\t"
		     "svc 0\n\t"
		     "mov %0, x0"
		     : "=r"(ret)
		     : "r"(sys_no), "r"(arg0), "r"(arg1), "r"(arg2), "r"(arg3),
		       "r"(arg4), "r"(arg5), "r"(arg6), "r"(arg7)
		     : "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8");
	return ret;
}

/*
 * Lab3: your code here:
 * Finish the following system calls using helper function syscall
 */
void usys_putc(char ch)
{
	syscall(SYS_putc, ch, 1, 2, 3, 4, 5, 6, 7);
}

void usys_exit(int ret)
{
	syscall(SYS_exit, ret, 0, 0, 0, 0, 0, 0, 0);
}

int usys_create_pmo(u64 size, u64 type)
{
	return syscall(SYS_create_pmo, size, type, 0, 0, 0, 0, 0, 0);
}

int usys_map_pmo(u64 process_cap, u64 pmo_cap, u64 addr, u64 rights)
{
	return syscall(SYS_map_pmo, process_cap, pmo_cap, addr, rights, 0, 0, 0,
		       0);
}

u64 usys_handle_brk(u64 addr)
{
	return syscall(SYS_handle_brk, addr, 0, 0, 0, 0, 0, 0, 0);
}

/* Here finishes all syscalls need by lab3 */
