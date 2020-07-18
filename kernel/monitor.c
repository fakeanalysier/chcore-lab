/*
 * Copyright (c) 2020 Institute of Parallel And Distributed Systems (IPADS), Shanghai Jiao Tong University (SJTU)
 * OS-Lab-2020 (i.e., ChCore) is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *   http://license.coscl.org.cn/MulanPSL
 *   THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 *   PURPOSE.
 *   See the Mulan PSL v1 for more details.
 */

// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <common/printk.h>
#include <common/types.h>

static inline __attribute__((always_inline)) u64 read_fp()
{
	u64 fp;
	__asm __volatile("mov %0, x29" : "=r"(fp));
	return fp;
}

__attribute__((optimize("O1"))) int mon_backtrace()
{
	printk("Stack backtrace:\n");

	// Your code here.
	u64 *fp = (u64 *)read_fp();
	while (fp && fp[0]) {
		// 因为这里和 test_backtrace 使用 O1 优化, 编译出的汇编形如 (3 个参数的情况):
		//   stp	x29, x30, [sp, #-48]!
		//   mov	x29, sp
		//   stp	x19, x20, [sp, #16]
		//   str	x21, [sp, #32]
		//   mov	x19, x0
		//   mov	x20, x1
		//   mov	x21, x2
		// 使用了 x19 等 callee-saved 寄存器存放参数, 每次函数开头保存的实际上是当前函数调用者的参数,
		// 因此 LR 和参数不在一个栈帧里面, 而参数与 FP 在同一个栈帧.
		printk("  LR %lx  FP %lx  Args %lx %lx %lx %lx %lx\n",
		       ((u64 *)fp[0])[1], fp[0], fp[2], fp[3], fp[4], fp[5],
		       fp[6]);
		fp = (u64 *)fp[0];
	}

	return 0;
}
