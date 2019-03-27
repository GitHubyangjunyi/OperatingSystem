#include "task.h"
#include "ptrace.h"
#include "printk.h"
#include "lib.h"
#include "memory.h"
#include "linkage.h"

extern void ret_system_call(void);
extern void system_call(void);

//现在系统调用的主体框架已基本实现,接下来编写程序执行系统调用API,在编写调用程序时应该要明白sysenter/sysexit指令并不具备保存程序执行环境的功能
//而sysexit指令的执行却必须要向RCX和RDX提供应用程序的返回地址和栈顶地址,所以在执行sysenter指令前,特地将应用程序的返回地址和栈顶地址保存在这两个寄存器内
//其中内嵌汇编是系统调用在应用层部分的核心程序,通过LEA取得标识符sysexit_return_address的有效地址,并将有效地址保存到RDX
//而RCX保存着应用层当前的栈指针,RAX保存着系统调用API向量号,当系统调用处理函数执行结束,系统调用处理函数便借助RAX把执行结果返回到应用层并保存在变量ret中
//系统调用的第0x0f号向量执行了函数no_system_call,此值正是在执行系统调用时由user_level_function传入到寄存器RAX的数值15
//当默认系统调用处理函数no_system_call执行结束后,默认系统调用处理函数将向应用层返回数值-1,返回值同样由RAX携带,最终被传递到应用程序的ret变量中
void user_level_function()
{
	long ret = 0;
	color_printk(RED,BLACK,"user_level_function task is running\n");

	__asm__	__volatile__	(	"leaq	sysexit_return_address(%%rip),	%%rdx	\n\t"
					"movq	%%rsp,	%%rcx		\n\t"
					"sysenter			\n\t"
					"sysexit_return_address:	\n\t"
					:"=a"(ret):"0"(15):"memory");	

	color_printk(RED,BLACK,"user_level_function task called sysenter,ret:%ld\n",ret);

	while(1);
}


unsigned long do_execve(struct pt_regs * regs)
{
	regs->rdx = 0x800000;	//RIP
	regs->rcx = 0xa00000;	//RSP
	regs->rax = 1;	
	regs->ds = 0;
	regs->es = 0;
	color_printk(RED,BLACK,"do_execve task is running\n");

	memcpy(user_level_function,(void *)0x800000,1024);

	return 0;
}


unsigned long init(unsigned long arg)
{
	struct pt_regs *regs;

	color_printk(RED,BLACK,"init task is running,arg:%#018lx\n",arg);

	current->thread->rip = (unsigned long)ret_system_call;
	current->thread->rsp = (unsigned long)current + STACK_SIZE - sizeof(struct pt_regs);
	regs = (struct pt_regs *)current->thread->rsp;

	__asm__	__volatile__	(	"movq	%1,	%%rsp	\n\t"
					"pushq	%2		\n\t"
					"jmp	do_execve	\n\t"
					::"D"(regs),"m"(current->thread->rsp),"m"(current->thread->rip):"memory");

	return 1;
}



unsigned long do_fork(struct pt_regs * regs, unsigned long clone_flags, unsigned long stack_start, unsigned long stack_size)
{
	struct task_struct *tsk = NULL;
	struct thread_struct *thd = NULL;
	struct Page *p = NULL;
	
	color_printk(WHITE,BLACK,"alloc_pages,bitmap:%#018lx\n",*memory_management_struct.bits_map);

	p = alloc_pages(ZONE_NORMAL,1,PG_PTable_Maped | PG_Active | PG_Kernel);

	color_printk(WHITE,BLACK,"alloc_pages,bitmap:%#018lx\n",*memory_management_struct.bits_map);

	tsk = (struct task_struct *)Phy_To_Virt(p->PHY_address);
	color_printk(WHITE,BLACK,"struct task_struct address:%#018lx\n",(unsigned long)tsk);

	memset(tsk,0,sizeof(*tsk));
	*tsk = *current;

	list_init(&tsk->list);
	list_add_to_before(&init_task_union.task.list,&tsk->list);	
	tsk->pid++;	
	tsk->state = TASK_UNINTERRUPTIBLE;

	thd = (struct thread_struct *)(tsk + 1);
	tsk->thread = thd;	

	memcpy(regs,(void *)((unsigned long)tsk + STACK_SIZE - sizeof(struct pt_regs)),sizeof(struct pt_regs));

	thd->rsp0 = (unsigned long)tsk + STACK_SIZE;
	thd->rip = regs->rip;
	thd->rsp = (unsigned long)tsk + STACK_SIZE - sizeof(struct pt_regs);

	if(!(tsk->flags & PF_KTHREAD))
		thd->rip = regs->rip = (unsigned long)ret_system_call;

	tsk->state = TASK_RUNNING;

	return 0;
}



unsigned long do_exit(unsigned long code)
{
	color_printk(RED,BLACK,"exit task is running,arg:%#018lx\n",code);
	while(1);
}


//system_call_function函数的参数记录着进程的执行环境,RAX保存着系统调用API的向量号,暂时定义128个系统调用
//数组system_call_table用于保存每个系统调用的处理函数,目前尚未实现任何系统调用功能,因此为每个系统调用配置默认处理函数no_system_call
//于此同时还要为sysenter汇编指令指定内核层栈指针以及系统调用在内核层的入口地址(system_call模块的起始地址),函数task_init将这两个值分别写到MSR寄存器组的175h和176h处
unsigned long  system_call_function(struct pt_regs * regs)
{
	return system_call_table[regs->rax](regs);
}


extern void kernel_thread_func(void);
__asm__ (
"kernel_thread_func:	\n\t"
"	popq	%r15	\n\t"
"	popq	%r14	\n\t"	
"	popq	%r13	\n\t"	
"	popq	%r12	\n\t"	
"	popq	%r11	\n\t"	
"	popq	%r10	\n\t"	
"	popq	%r9	\n\t"	
"	popq	%r8	\n\t"	
"	popq	%rbx	\n\t"	
"	popq	%rcx	\n\t"	
"	popq	%rdx	\n\t"	
"	popq	%rsi	\n\t"	
"	popq	%rdi	\n\t"	
"	popq	%rbp	\n\t"	
"	popq	%rax	\n\t"	
"	movq	%rax,	%ds	\n\t"
"	popq	%rax		\n\t"
"	movq	%rax,	%es	\n\t"
"	popq	%rax		\n\t"
"	addq	$0x38,	%rsp	\n\t"
/////////////////////////////////
"	movq	%rdx,	%rdi	\n\t"
"	callq	*%rbx		\n\t"
"	movq	%rax,	%rdi	\n\t"
"	callq	do_exit		\n\t"
);



int kernel_thread(unsigned long (* fn)(unsigned long), unsigned long arg, unsigned long flags)
{
	struct pt_regs regs;
	memset(&regs,0,sizeof(regs));

	regs.rbx = (unsigned long)fn;
	regs.rdx = (unsigned long)arg;

	regs.ds = KERNEL_DS;
	regs.es = KERNEL_DS;
	regs.cs = KERNEL_CS;
	regs.ss = KERNEL_DS;
	regs.rflags = (1 << 9);
	regs.rip = (unsigned long)kernel_thread_func;

	return do_fork(&regs,flags,0,0);
}



inline void __switch_to(struct task_struct *prev,struct task_struct *next)
{

	init_tss[0].rsp0 = next->thread->rsp0;

	set_tss64(init_tss[0].rsp0, init_tss[0].rsp1, init_tss[0].rsp2, init_tss[0].ist1, init_tss[0].ist2, init_tss[0].ist3, init_tss[0].ist4, init_tss[0].ist5, init_tss[0].ist6, init_tss[0].ist7);

	__asm__ __volatile__("movq	%%fs,	%0 \n\t":"=a"(prev->thread->fs));
	__asm__ __volatile__("movq	%%gs,	%0 \n\t":"=a"(prev->thread->gs));

	__asm__ __volatile__("movq	%0,	%%fs \n\t"::"a"(next->thread->fs));
	__asm__ __volatile__("movq	%0,	%%gs \n\t"::"a"(next->thread->gs));

	color_printk(WHITE,BLACK,"prev->thread->rsp0:%#018lx\n",prev->thread->rsp0);
	color_printk(WHITE,BLACK,"next->thread->rsp0:%#018lx\n",next->thread->rsp0);
}


void task_init()
{
	struct task_struct *p = NULL;

	init_mm.pgd = (pml4t_t *)Global_CR3;

	init_mm.start_code = memory_management_struct.start_code;
	init_mm.end_code = memory_management_struct.end_code;

	init_mm.start_data = (unsigned long)&_data;
	init_mm.end_data = memory_management_struct.end_data;

	init_mm.start_rodata = (unsigned long)&_rodata; 
	init_mm.end_rodata = (unsigned long)&_erodata;

	init_mm.start_brk = 0;
	init_mm.end_brk = memory_management_struct.end_brk;

	init_mm.start_stack = _stack_start;
	
	wrmsr(0x174,KERNEL_CS);
	//于此同时还要为sysenter汇编指令指定内核层栈指针以及系统调用在内核层的入口地址(system_call模块的起始地址),函数task_init将这两个值分别写到MSR寄存器组的175h和176h处
	wrmsr(0x175,current->thread->rsp0);//系统第一个进程的内核栈作为系统调用使用的内核层栈
	wrmsr(0x176,(unsigned long)system_call);
	
//	init_thread,init_tss
	set_tss64(init_thread.rsp0, init_tss[0].rsp1, init_tss[0].rsp2, init_tss[0].ist1, init_tss[0].ist2, init_tss[0].ist3, init_tss[0].ist4, init_tss[0].ist5, init_tss[0].ist6, init_tss[0].ist7);

	init_tss[0].rsp0 = init_thread.rsp0;

	list_init(&init_task_union.task.list);

	kernel_thread(init,10,CLONE_FS | CLONE_FILES | CLONE_SIGNAL);

	init_task_union.task.state = TASK_RUNNING;

	p = container_of(list_next(&current->list),struct task_struct,list);

	switch_to(current,p);
}