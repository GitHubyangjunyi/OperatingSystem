#ifndef __TASK_H__
#define __TASK_H__

#include "memory.h"
#include "cpu.h"
#include "lib.h"
#include "ptrace.h"

#define KERNEL_CS 	(0x08)
#define	KERNEL_DS 	(0x10)

#define	USER_CS		(0x28)
#define USER_DS		(0x30)

#define CLONE_FS	(1 << 0)
#define CLONE_FILES	(1 << 1)
#define CLONE_SIGNAL	(1 << 2)

// stack size 32K
#define STACK_SIZE 32768	//表示进程的内核栈空间和task_struct结构体占用的存储空间总量为32KB
							//在Intel i386架构的Linux内核中进程默认使用8KB的内核栈空间(64位处理器之前的情况)
//用来表示地址的外部标号,理解成汇编级的标号
extern char _text;
extern char _etext;
extern char _data;
extern char _edata;
extern char _rodata;
extern char _erodata;
extern char _bss;
extern char _ebss;
extern char _end;

extern unsigned long _stack_start;
//head.S
//ENTRY(_stack_start)
//	.quad	init_task_union + 32768
//
//全局变量_stack_start保存的数值与init_thread结构体变量中的rsp0成员变量的值是一样的,都指向系统第一个进程的内核层栈基地址
//这是精心设计的,定义全局变量_stack_start可让内核执行头程序直接使用该进程的内核层栈空间,从而减少栈空间切换带来的隐患


extern void ret_from_intr();

//进程状态位域,在当前的Linux中这些状态是互斥的,严格意义上说只能设置一种状态,其余状态被清除
#define TASK_RUNNING		(1 << 0)//可运行状态(要么在运行,要么准备执行)
#define TASK_INTERRUPTIBLE	(1 << 1)//可中断的等待状态(进程被挂起睡眠,直到某个条件变为真,产生一个硬件中断,释放进程正在等待的系统资源,或传递一个信号都是可以唤醒进程的条件,即把进程放回TASK_RUNNING)
#define	TASK_UNINTERRUPTIBLE	(1 << 2)//不可中断的等待状态(与可中断的等待状态类似,但是有一个例外,把信号传递到睡眠进程不能改变它的状态,这种状态很少用到,但在一些特定的情况下进程必须等待,直到一个不能被中断的事件发生)
//例如当进程打开一个设备文件,其相应的设备驱动程序开始探测相应的硬件设备时会用到这种状态,探测完成前设备驱动程序不能被中断,否则硬件设备会处于不可预知的状态
#define	TASK_ZOMBIE		(1 << 3)	//僵死状态
#define	TASK_STOPPED		(1 << 4)//暂停状态(进程的执行被暂停,当进程收到SIGSTOP/SIGTSTP/SIGTTIN/SIGTTOU信号后进入暂停状态)
//还有一个跟踪状态,进程的执行已经由debugger程序暂停,当一个进程被另一个进程监控时(例如debugger执行ptrace系统调用监控一个程序),任何信号都可以把这个进程置于TASK_TRACED

//用来描述进程的页表结构和各个程序段信息的内存空间分布结构体
struct mm_struct
{
	pml4t_t *pgd;										//内存页表指针,保存在CR3控制寄存器值(页目录基地址与页表属性的组合值)
	
	unsigned long start_code,end_code;					//代码段空间
	unsigned long start_data,end_data;					//数据段空间
	unsigned long start_rodata,end_rodata;				//只读数据段空间
	unsigned long start_brk,end_brk;					//动态内存分配区(堆区域)
	unsigned long start_stack;							//应用层栈基地址
};

//保存进程发生调度切换时,执行现场的寄存器值,以备再次执行时使用
struct thread_struct
{
	unsigned long rsp0;									//in tss//应用程序在内核层使用的栈基地址

	unsigned long rip;									//内核层代码指针(进程切换回来时执行代码的地址)
	unsigned long rsp;									//内核层当前栈指针(进程切换时的栈指针值)

	unsigned long fs;									//FS段寄存器
	unsigned long gs;									//GS段寄存器

	unsigned long cr2;									//CR2控制寄存器
	unsigned long trap_nr;								//产生异常的异常号
	unsigned long error_code;							//异常的错误码
};


#define PF_KTHREAD	(1 << 0)

//用于记录进程得资源使用情况和运行状态信息的进程控制结构体PCB
//其中mm负责在进程调度过程中保存或还原CR3的页目录基地址,thread负责在进程调度过程中保存或还原通用寄存器的值
struct task_struct
{
	struct List list;									//连接各个进程控制结构体的双向链表
	volatile long state;								//进程状态(运行态/停止态/可中断态)volatile修饰,处理器每次使用这个进程状态前都必须重新读取这个值而不能使用寄存器中的备份值
	unsigned long flags;								//进程标志(进程/线程/内核线程)

	struct mm_struct *mm;								//内存空间分布结构体,记录内存页表和程序段信息
	struct thread_struct *thread;						//进程切换时保留的状态信息

	unsigned long addr_limit;							//进程地址空间范围/*0x0000,0000,0000,0000 - 0x0000,7fff,ffff,ffff user*/
																		/*0xffff,8000,0000,0000 - 0xffff,ffff,ffff,ffff kernel*/

	long pid;											//进程ID号
	long counter;										//进程可用时间片
	long signal;										//进程持有的信号
	long priority;										//进程优先级
};

//进程的内核层栈空间的实现借鉴了Linux内核的设计思想,把进程控制结构体task_struct与进程的内核层栈空间融为一体,低地址存放task_struct,余下的高地址作为进程的内核层栈空间使用
//把进程控制结构体task_struct与进程的内核层栈空间连续到一起,其中STACK_SIZE被定义为32KB,表示进程的内核栈空间和task_struct结构体占用的存储空间总量为32KB
union task_union
{									//此联合体共占用32KB字节空间,并按8字节对齐,但实际上这个结构体必须按32KB字节对齐
	struct task_struct task;
	unsigned long stack[STACK_SIZE / sizeof(unsigned long)];//这个stack数组将占用32KB以至于这个结构体实际上按32KB对齐
}__attribute__((aligned (8)));

struct mm_struct init_mm;
struct thread_struct init_thread;

//初始化系统第一个进程的各个信息
#define INIT_TASK(tsk)	\
{			\
	.state = TASK_UNINTERRUPTIBLE,		\
	.flags = PF_KTHREAD,		\
	.mm = &init_mm,			\
	.thread = &init_thread,		\
	.addr_limit = 0xffff800000000000,	\
	.pid = 0,			\
	.counter = 1,		\
	.signal = 0,		\
	.priority = 0		\
}

//初级:将联合体task_union实例化成全局变量init_task_union,并将其作为操作系统第一个进程
//扩展属性将该全局变量链接到一个特别的程序段内,这个.data.init_task段被链接脚本放在只读数据段rodata之后并按照32KB对齐
//对齐规则是因为除了init_task_union外其他的union task_union联合体都使用kmalloc函数申请空间,而kmalloc返回的内存空间起始地址都是按32KB对齐,如果使用8B对齐,后续的宏current和GET_CURRENT都将有隐患
union task_union init_task_union __attribute__((__section__ (".data.init_task"))) = {INIT_TASK(init_task_union.task)};
//经过预处理后:union task_union init_task_union __attribute__((__section__ (".data.init_task"))) 
//= {{ .state = (1 << 2), .flags = (1 << 0), .mm = &init_mm, .thread = &init_thread, .addr_limit = 0xffff800000000000, .pid = 0, .counter = 1, .signal = 0, .priority = 0 }};

//进程控制结构体数组init_task(指针数组)是为各处理器创建的初始进程控制结构体,目前只有数组第0个投入使用,剩余成员将在多核处理器初始化后创建
struct task_struct *init_task[NR_CPUS] = {&init_task_union.task,0};//#define NR_CPUS 8

struct mm_struct init_mm = {0};//由Start_Kernel调用task_init函数填充完整

//系统第一个进程的执行现场信息结构体
struct thread_struct init_thread = 
{
	.rsp0 = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),//in tss//应用程序在内核层使用的栈基地址
	.rsp = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),//内核层当前栈指针(进程切换时的栈指针值)
	.fs = KERNEL_DS,
	.gs = KERNEL_DS,
	.cr2 = 0,
	.trap_nr = 0,
	.error_code = 0
};


//IA-32e模式下的TSS结构
struct tss_struct
{
	unsigned int  reserved0;
	unsigned long rsp0;
	unsigned long rsp1;
	unsigned long rsp2;
	unsigned long reserved1;
	unsigned long ist1;
	unsigned long ist2;
	unsigned long ist3;
	unsigned long ist4;
	unsigned long ist5;
	unsigned long ist6;
	unsigned long ist7;
	unsigned long reserved2;
	unsigned short reserved3;
	unsigned short iomapbaseaddr;
}__attribute__((packed));


#define INIT_TSS \
{	.reserved0 = 0,	 \
	.rsp0 = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),	\
	.rsp1 = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),	\
	.rsp2 = (unsigned long)(init_task_union.stack + STACK_SIZE / sizeof(unsigned long)),	\
	.reserved1 = 0,	 \
	.ist1 = 0xffff800000007c00,	\
	.ist2 = 0xffff800000007c00,	\
	.ist3 = 0xffff800000007c00,	\
	.ist4 = 0xffff800000007c00,	\
	.ist5 = 0xffff800000007c00,	\
	.ist6 = 0xffff800000007c00,	\
	.ist7 = 0xffff800000007c00,	\
	.reserved2 = 0,	\
	.reserved3 = 0,	\
	.iomapbaseaddr = 0	\
}
//各个处理器的TSS结构体数组
struct tss_struct init_tss[NR_CPUS] = { [0 ... NR_CPUS-1] = INIT_TSS };//#define NR_CPUS 8,GNU C扩展的范围匹配


inline	struct task_struct * get_current()
{
	struct task_struct * current = NULL;
	__asm__ __volatile__ ("andq %%rsp,%0	\n\t":"=r"(current):"0"(~32767UL));
	return current;
}

//用于获取当前task_struct结构体
//借助设计task_union时使用的32KB对齐技巧来实现,get_current和GET_CURRENT宏均是在当前栈指针RSP的基础上按32KB下边界对齐实现的
//方法是将数值32KB-1取反,再将所得结果0xffff ffff ffff 8000与栈指针RSP的值执行与运算,计算结果就是当前进程PCB task_struct结构体的基地址
#define current get_current()

#define GET_CURRENT			\
	"movq	%rsp,	%rbx	\n\t"	\
	"andq	$-32768,%rbx	\n\t"


#define switch_to(prev,next)			\
do{							\
	__asm__ __volatile__ (	"pushq	%%rbp	\n\t"	\
				"pushq	%%rax	\n\t"	\
				"movq	%%rsp,	%0	\n\t"	\
				"movq	%2,	%%rsp	\n\t"	\
				"leaq	1f(%%rip),	%%rax	\n\t"	\
				"movq	%%rax,	%1	\n\t"	\
				"pushq	%3		\n\t"	\
				"jmp	__switch_to	\n\t"	\
				"1:	\n\t"	\
				"popq	%%rax	\n\t"	\
				"popq	%%rbp	\n\t"	\
				:"=m"(prev->thread->rsp),"=m"(prev->thread->rip)		\
				:"m"(next->thread->rsp),"m"(next->thread->rip),"D"(prev),"S"(next)	\
				:"memory"		\
				);			\
}while(0)
//RDI/RSI分别保存着宏参数prev和next所代表的进程控制结构体,prev代表当前进程控制结构体(指针),next代表目标进程控制结构体(指针)
//在调用__switch_to函数时模块switch_to会将RDI/RSI的值作为参数传递给__switch_to,进入__switch_to后会继续完成进程切换的后续工作
//prev进程(当前进程)通过调用switch_to模块保存RSP,并指定切换回prev时的RIP值,此处默认保存到标识符1:处,随后将next进程的栈指针恢复到RSP
//再把next的执行现场RIP压入next进程的内核层栈空间(RSP寄存器的恢复在前,此后的数据将压入next进程的内核层栈空间),最后借助JMP指令执行__switch_to
//__switch_to会在返回过程中执行RET指令,进而跳转到next进程继续执行(恢复执行现场的RIP寄存器),至此进程间切换工作执行完毕
//下接inline void __switch_to

unsigned long do_fork(struct pt_regs * regs, unsigned long clone_flags, unsigned long stack_start, unsigned long stack_size);
void task_init();

#endif