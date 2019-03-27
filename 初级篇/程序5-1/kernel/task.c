#include "task.h"
#include "ptrace.h"
#include "printk.h"
#include "lib.h"
#include "memory.h"
#include "linkage.h"

extern void ret_system_call(void);

//借助ret_system_call模块跳转到应用层执行
void user_level_function()
{
	color_printk(RED,BLACK,"user_level_function task is running\n");
	while(1);
}

//在do_execve函数中通过设置pt_regs结构体的成员变量来搭建应用程序的执行环境,当do_execve函数返回时处理器会跳转至ret_system_call模块处执行
//进而将pt_regs的各个成员变量还原到对应的寄存器中
unsigned long do_execve(struct pt_regs * regs)
{
	//选用这两个寄存器是与sysexit指令的需求相呼应的,sysexit需要借助RCX和RDX来返回到应用层
	regs->rdx = 0x800000;	//RIP,保存着应用程序的入口地址
	regs->rcx = 0xa00000;	//RSP,保存着应用程序的应用层栈顶地址
	regs->rax = 1;
	regs->ds = 0;
	regs->es = 0;
	color_printk(RED,BLACK,"do_execve task is running\n");

	memcpy(user_level_function,(void *)0x800000,1024);//负责将应用层的执行函数user_level_function复制到线性地址0x80 0000处
	//当处理器切换至应用层后,应用程序将从应用层的线性地址0x80 0000处开始执行,对于线性地址0x80 0000的选择,没有特别的依据,只要是未使用的空间
	//而选择0xa0 0000作为应用程序栈顶地址则是为了保证它与线性地址0x80 0000在同一个物理页
	//运行前需要注释掉init_memory对页表映射一致性的清理,不然会触发缺页异常
	//但是运行时还是会出现页错误异常,错误地址0x80 0000,当读取该地址时触发异常,从线性地址0x80 0000开始是代码程序,读取该地址数据就意味着执行该地址处的程序
	//而且没有显示出这个页面不存在,说明有映射关系存在,最终原因是页属性限制物理页只允许内核程序访问,因此需要修改页表项属性,为应用程序放宽限制
	return 0;
}

//上个版本的init
//系统的第二个进程,无实际功能,只是打印由创建者传入的参数并返回1以证明运行
//其实init函数和日常编写的main主函数一样,经过编译器编译生成若干个程序片段并记录程序的入口地址,当操作系统为程序创建进程控制结构体时
//操作系统会取得程序的入口地址,并从这个入口地址处执行
//unsigned long init(unsigned long arg)
//{
//	color_printk(RED,BLACK,"init task is running,arg:%#018lx\n",arg);
//	return 1;
//}
//完善init进程
unsigned long init(unsigned long arg)
{
	struct pt_regs *regs;

	color_printk(RED,BLACK,"init task is running,arg:%#018lx\n",arg);

	current->thread->rip = (unsigned long)ret_system_call;//将当前进程(init自己)的thread->rip置为ret_system_call
	current->thread->rsp = (unsigned long)current + STACK_SIZE - sizeof(struct pt_regs);
	regs = (struct pt_regs *)current->thread->rsp;

	
	__asm__	__volatile__	(	"movq	%1,	%%rsp	\n\t"
					"pushq	%2		\n\t"
					"jmp	do_execve	\n\t"
					::"D"(regs),"m"(current->thread->rsp),"m"(current->thread->rip):"memory");

	return 1;
	//目前系统还没有应用层程序,此时的init依然是内核线程,接下来扩充init线程的功能,使其转变为应用程序
	//执行do_execve系统调用API可使init线程执行新的程序,进而转变为应用程序,但是由于sysenter/sysexit指令无法像中断指令那样,可以在内核层执行系统调用API
	//所以只能通过直接执行系统调用API处理函数的方法来实现,此处参考switch_to函数的设计思路,调用execve系统调用API的处理函数do_execve
	//即借助push指令将程序的返回地址压入栈中,并采用jmp指令调用函数do_execve
	//首先确定函数init的函数返回地址和栈指针,并取得进程的pt_regs结构体,接着采用内嵌汇编更新进程的内核层栈指针,同时将调用do_execve函数后的返回地址(ret_system_call模块)压入栈中
	//最后通过jmp指令跳转至do_execve函数为新程序(目标应用程序)准备执行环境,并将pt_regs结构体的首地址作为参数传递给do_execve函数
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
		thd->rip = regs->rip = (unsigned long)ret_system_call;//do-fork作为一个系统调用的处理函数,应该使用ret_system_call模块

	tsk->state = TASK_RUNNING;

	return 0;
}



unsigned long do_exit(unsigned long code)
{
	color_printk(RED,BLACK,"exit task is running,arg:%#018lx\n",code);
	while(1);
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
	
	wrmsr(0x174,KERNEL_CS);//为IA32_SYSENTER_CS寄存器(位于MSR寄存器组的0x174地址处)设置段选择子
	//至此汇编指令SYSENTER引入后的程序修改工作结束,但为了提供sysexit指令返回3特权级,还必须完善init内核线程,才能实现内核层向应用层的跳转

//	init_thread,init_tss
	set_tss64(init_thread.rsp0, init_tss[0].rsp1, init_tss[0].rsp2, init_tss[0].ist1, init_tss[0].ist2, init_tss[0].ist3, init_tss[0].ist4, init_tss[0].ist5, init_tss[0].ist6, init_tss[0].ist7);

	init_tss[0].rsp0 = init_thread.rsp0;

	list_init(&init_task_union.task.list);



	kernel_thread(init,10,CLONE_FS | CLONE_FILES | CLONE_SIGNAL);

	init_task_union.task.state = TASK_RUNNING;

	p = container_of(list_next(&current->list),struct task_struct,list);

	switch_to(current,p);
}