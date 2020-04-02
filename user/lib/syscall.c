#include <lib/print.h>
#include <lib/syscall.h>
#include <lib/type.h>

u64 syscall(u64 sys_no, u64 arg0, u64 arg1, u64 arg2, u64 arg3, u64 arg4,
	    u64 arg5, u64 arg6, u64 arg7, u64 arg8)
{

	u64 ret = 0;
	/*
	 * Lab3: Your code here
	 * Use inline assembly to store arguments into x0 to x7, store syscall number to x8,
	 * And finally use svc to execute the system call. After syscall returned, don't forget
	 * to move return value from x0 to the ret variable of this function
	 */
	return ret;
}

/*
 * Lab3: your code here:
 * Finish the following system calls using helper function syscall
 */
void usys_putc(char ch)
{
}

void usys_exit(int ret)
{
}

int usys_create_pmo(u64 size, u64 type)
{
	return -1;
}

int usys_map_pmo(u64 process_cap, u64 pmo_cap, u64 addr, u64 rights)
{
	return -1;
}

u64 usys_handle_brk(u64 addr)
{
	return -1;
}

/* Here finishes all syscalls need by lab3 */
