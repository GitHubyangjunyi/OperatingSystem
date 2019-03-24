#ifndef __PTRACE_H__
#define __PTRACE_H__

//实现系统调用API功能时,与中断/异常处理程序一样都必须在处理程序入口处保存程序执行现场,在返回处恢复程序执行现场
//由于系统调用API不会生成错误码,为了兼顾中断/异常处理程序,又兼顾系统调用API处理程序,因此这里将根据中断/异常处理程序的执行过程来设计pt_regs结构体
//随着pt_regs结构体的设计和实现,系统内核的中断/异常处理函数也应该相继做出调整和升级
//执行现场数据组织成的结构体,注意顺序问题
struct pt_regs
{
	unsigned long r15;
	unsigned long r14;
	unsigned long r13;
	unsigned long r12;
	unsigned long r11;
	unsigned long r10;
	unsigned long r9;
	unsigned long r8;
	unsigned long rbx;
	unsigned long rcx;
	unsigned long rdx;
	unsigned long rsi;
	unsigned long rdi;
	unsigned long rbp;
	unsigned long ds;
	unsigned long es;
	unsigned long rax;
	unsigned long func;
	unsigned long errcode;
	unsigned long rip;
	unsigned long cs;
	unsigned long rflags;
	unsigned long rsp;
	unsigned long ss;
};

#endif