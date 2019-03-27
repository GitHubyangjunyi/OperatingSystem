#include "task.h"
#include "ptrace.h"
#include "printk.h"
#include "lib.h"
#include "memory.h"
#include "linkage.h"

//系统的第二个进程,无实际功能,只是打印由创建者传入的参数并返回1以证明运行
//其实init函数和日常编写的main主函数一样,经过编译器编译生成若干个程序片段并记录程序的入口地址,当操作系统为程序创建进程控制结构体时
//操作系统会取得程序的入口地址,并从这个入口地址处执行
unsigned long init(unsigned long arg)
{
	color_printk(RED,BLACK,"init task is running,arg:%#018lx\n",arg);
	return 1;
}


//目前的do_fork函数已基本实现进程控制结构体的创建以及相关数据的初始化工作,由于内核层尚未实现内存分配功能,内存空间的使用只能暂时以物理页为单位
//同时为了检测alloc_pages的执行效果,在分配物理页的前后打印出物理内存页的位图映射信息
unsigned long do_fork(struct pt_regs * regs, unsigned long clone_flags, unsigned long stack_start, unsigned long stack_size)
{
	struct task_struct *tsk = NULL;//任务控制块PCB
	struct thread_struct *thd = NULL;//执行现场的主要寄存器状态
	struct Page *p = NULL;//物理页
	
	//分配物理页前后打印位图映射信息
	color_printk(WHITE,BLACK,"alloc_pages,bitmap:%#018lx\n",*memory_management_struct.bits_map);
	p = alloc_pages(ZONE_NORMAL,1,PG_PTable_Maped | PG_Active | PG_Kernel);//ZONE_NORMAL区域申请一个页面,属性为经过页表映射的页/使用中/内核层的页
	color_printk(WHITE,BLACK,"alloc_pages,bitmap:%#018lx\n",*memory_management_struct.bits_map);

	tsk = (struct task_struct *)Phy_To_Virt(p->PHY_address);//将申请到的物理页转换成虚拟地址给进程控制结构体使用
	color_printk(WHITE,BLACK,"struct task_struct address:%#018lx\n",(unsigned long)tsk);//0xffff 8000 0002 0000,从第2MB开始分配

	//将当前进程控制结构体中的数据复制到新分配的物理页中(物理页的线性地址起始处),并进一步初始化相关成员变量信息
	//包括链接入进程队列/分配和初始化thread_struct结构体/伪造进程执行现场(把执行现场数据复制到目标进程的内核层栈顶处)
	memset(tsk,0,sizeof(*tsk));
	*tsk = *current;//将当前进程控制结构体中的数据复制到新分配的物理页中(物理页的线性地址起始处),*current获得的是当前进程的PCB,以至于新进程的PCB是一份当前进程的克隆

	list_init(&tsk->list);//初始化新进程PCB的链域
	list_add_to_before(&init_task_union.task.list,&tsk->list);//作为前驱链接入当前进程链域
	tsk->pid++;//设置进程id,克隆自当前进程的PCB,所以变成当前进程id号加1
	tsk->state = TASK_UNINTERRUPTIBLE;//设置进程状态为未被中断,现在未初始化完成,不可接受调度进入运行

	thd = (struct thread_struct *)(tsk + 1);//使用新申请的PCB后续的地址作为执行现场结构体thd的信息存放处
	tsk->thread = thd;//将进程控制块中的进程切换时的保留信息结构体指向刚创建的thread_struct结构体thd

	//从regs复制sizeof(struct pt_regs)个字节到(void *)((unsigned long)tsk + STACK_SIZE - sizeof(struct pt_regs))
	//regs是新的执行现场信息(由kernel_thread构造并传入),也就是将新的执行现场信息信息复制到新进程的执行现场信息结构体中
	memcpy(regs,(void *)((unsigned long)tsk + STACK_SIZE - sizeof(struct pt_regs)),sizeof(struct pt_regs));//伪造进程执行现场(把执行现场数据复制到目标进程的内核层栈顶处)

	thd->rsp0 = (unsigned long)tsk + STACK_SIZE;//新申请的页的剩余空间作为栈空间使用(应用程序在内核层使用的栈基地址)
	thd->rip = regs->rip;//指向新的执行现场信息(由kernel_thread构造并传入)中的引导代码
	thd->rsp = (unsigned long)tsk + STACK_SIZE - sizeof(struct pt_regs);//内核层当前栈指针(进程切换时的栈指针值),指向修改为指向新进程的regs最后一个元素ss(但是最先弹出),注意顺序问题

	if(!(tsk->flags & PF_KTHREAD))//最后判断目标进程的PF_KTHREAD标志位以确定目标进程运行在内核层空间还是应用层空间
		thd->rip = regs->rip = (unsigned long)ret_from_intr;//如果复位PF_KTHREAD标志位说明进程运行于应用层空间,那么将进程的执行入口点设置在ret_from_intr处(个人理解应该是借助iretq切换回低特权级的应用层)
															//否则将进程的执行入口点设置在kernel_thread_func地址处
	//补充说明:在初始化进程控制结构体时,未曾分配mm_struct的存储空间,而依然沿用全局变量init_mm,这是考虑到分配页表是一件无聊的体力活,既然init进程此时还运行在内核层空间
	//那么在实现内存分配功能前暂且不创建新的页目录和页表
	tsk->state = TASK_RUNNING;//当do_fork函数将目标进程设置为可运行态后,在全局链表init_task_union.task.list中已经有两个可运行的进程控制结构体,一旦task_init函数执行switch_to模块
							//操作系统便会切换进程,从而使得处理器开始执行init进程,由于init进程运行在内核层空间,因此init进程在执行init函数前会先执行kernel_thread_func模块
	return 0;
	//do_fork才是创建进程的核心函数,而kernel_thread更像是对创建出的进程做了特殊限制,这个有kernel_thread函数创建出来的进程看起来更像是一个线程,尽管kernel_thread函数借助do_fork函数
	//创建出了进程控制结构体,但是这个进程却没有应用层空间(复制系统第一个进程的PCB),其实kernel_thread只能创建出没有应用层空间的进程,如果有诸多这样的进程同时运行在内核中,看起来就像是内核主进程
	//创建出的若干个线程一样,所以叫做内核线程,这段程序参考了Linux1到4各个版本中关于内核线程的函数实现,综上所述,kernel_thread函数的功能是创建内核线程,所以init此时是一个内核级的线程,但不会一直是
	//当内核线程执行do_execve函数后会转变为一个用户级进程
}


//用于释放进程控制结构体,未实现的原因是进程控制结构体的释放过程很复杂啊(各种资源回收),现在只是打印init进程的返回值
unsigned long do_exit(unsigned long code)
{
	color_printk(RED,BLACK,"exit task is running,arg:%#018lx\n",code);
	while(1);
}


//负责还原进程执行现场/运行进程以及退出进程
//当处理器执行kernel_thread_func模块时,RSP正指向当前进程的内核层栈顶地址处,此刻栈顶位于栈基地址向下偏移pt_regs结构体处,经过若干个POP,最终将RSP平衡到栈基地址处
//进而达到还原进程执行现场的目的,这个执行现场是在kernel_thread中伪造的(通过构造pt_regs结构体,之后传递给do_fork函数),其中的RBX保存着程序执行片段,RDX保存着传入的参数
//进程执行现场还原后将借助CALL执行RBX保存的程序执行片段(init进程),一旦程序片段返回便执行do_exit函数退出进程
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
/////////////////////////////////构造(或者说伪造)时最后的7个信息(0x38=56字节置0,所以直接加上栈指针跳过)
"	movq	%rdx,	%rdi	\n\t"
"	callq	*%rbx		\n\t"
"	movq	%rax,	%rdi	\n\t"
"	callq	do_exit		\n\t"
);

//为操作系统创建进程,需要的参数:程序入口地址(函数指针)/参数/flags
int kernel_thread(unsigned long (* fn)(unsigned long), unsigned long arg, unsigned long flags)
{
	struct pt_regs regs;//首先为新进程准备执行现场信息而创建pt_regs结构体
	memset(&regs,0,sizeof(regs));//全部初始化置0

	regs.rbx = (unsigned long)fn;//程序入口地址(函数指针)
	regs.rdx = (unsigned long)arg;//进程创建者传入的参数

	regs.ds = KERNEL_DS;	//#define	KERNEL_DS 	(0x10)//选择子
	regs.es = KERNEL_DS;

	regs.cs = KERNEL_CS;	//#define 	KERNEL_CS 	(0x08)//选择子

	regs.ss = KERNEL_DS;
	regs.rflags = (1 << 9);//位9是IF位,置1可以响应中断
	regs.rip = (unsigned long)kernel_thread_func;//引导程序(kernel_thread_func模块),这段引导程序会在目标程序(保存于参数fn内)执行前运行
	//至此创建完新的执行现场主要信息,次要信息置0
	//随后将执行现场数据传递给do_fork函数,来创建进程控制结构体并完成进程运行前的初始化工作,可见do_fork和kernel_thread模块才是创建进程的关键代码
	return do_fork(&regs,flags,0,0);
}

//上接switch_to宏
//__switch_to首先将next进程的内核层栈基地址设置到TSS结构体中,随后保存当前进程的FS和GS段寄存器值,再将next进程保存的FS和GS段寄存器值还原
//应用程序在进入内核层时已将进程的执行现场(所有通用寄存器值)保存起来,所以进程切换过程并不涉及保存/还原通用寄存器,仅需对RIP和RSP进行设置
inline void __switch_to(struct task_struct *prev,struct task_struct *next)
{

	init_tss[0].rsp0 = next->thread->rsp0;//首先将next进程的内核层栈基地址设置到TSS结构体中
	set_tss64(init_tss[0].rsp0, init_tss[0].rsp1, init_tss[0].rsp2, init_tss[0].ist1, init_tss[0].ist2, init_tss[0].ist3, init_tss[0].ist4, init_tss[0].ist5, init_tss[0].ist6, init_tss[0].ist7);

	__asm__ __volatile__("movq	%%fs,	%0 \n\t":"=a"(prev->thread->fs));//保存当前进程的FS和GS段寄存器值
	__asm__ __volatile__("movq	%%gs,	%0 \n\t":"=a"(prev->thread->gs));

	__asm__ __volatile__("movq	%0,	%%fs \n\t"::"a"(next->thread->fs));//将next进程保存的FS和GS段寄存器值还原
	__asm__ __volatile__("movq	%0,	%%gs \n\t"::"a"(next->thread->gs));

	color_printk(WHITE,BLACK,"prev->thread->rsp0:%#018lx\n",prev->thread->rsp0);//0xffff 8000 0012 0000
	color_printk(WHITE,BLACK,"next->thread->rsp0:%#018lx\n",next->thread->rsp0);//0xffff 8000 0020 8000
}

//系统第一个进程的初始化工作task_init();以及创建第二个进程并切换至第二个进程
//再调用函数kernel_thread为系统创建出一个新的进程,随后借助switch_to模块执行进程以实现切换
//这段程序补充完系统第一个进程控制结构体中未赋值的变量,并为其设置内核层栈基地址(位于TSS结构体内)
//其实处理器早已在运行第一个进程中,只不过此前的进程控制结构体尚未初始化完毕,此处特别要注意的全局变量_stack_start,记录着系统第一个进程的内核层栈基地址
//head.S
//ENTRY(_stack_start)
//	.quad	init_task_union + 32768
//其实全局变量_stack_start保存的数值和init_thread结构体变量中的rsp0是一样的,都指向系统第一个进程的内核层栈基地址,这是精心设计的
//定义全局变量_stack_start可让内核执行头程序直接使用该进程的内核层栈空间,从而减少栈空间切换带来的隐患
//通常系统的第一个进程会协助操作系统完成一些初始化任务,该进程会在执行完初始化任务后进入等待状态,会在系统没有可运行进程时休眠以省电
//因此系统第一个进程不存在应用层空间,对于这个没有应用层空间的进程而言,其init_mm结构体变量(mm_struct结构体)保存的不再是应用程序信息而是内核程序的各个段信息以及内核层栈基地址
//接下来task_init函数执行kernel_thread函数为系统创建第二个进程(通常称为init进程),对于调用kernel_thread函数传入的CLONE_FS | CLONE_FILES | CLONE_SIGNAL等克隆标志位,目前未实现相应功能,预留使用
void task_init()
{
	struct task_struct *p = NULL;//PCB进程控制块指针

	//1.补充完系统第一个进程的页表结构和各个段信息的内存空间分布结构体
	init_mm.pgd = (pml4t_t *)Global_CR3;

	init_mm.start_code = memory_management_struct.start_code;//内核代码段开始地址0xffff 8000 0010 0000
	init_mm.end_code = memory_management_struct.end_code;//内核代码段结束地址0xffff 8000 0010 95d0(允许误差,指的是我添加了一些调试语句,导致地址变化)

	init_mm.start_data = (unsigned long)&_data;//内核数据段开始地址
	init_mm.end_data = memory_management_struct.end_data;//内核数据段结束地址0xffff 8000 0011 06a0

	init_mm.start_rodata = (unsigned long)&_rodata;//只读数据段空间
	init_mm.end_rodata = (unsigned long)&_erodata;

	init_mm.start_brk = 0;
	init_mm.end_brk = memory_management_struct.end_brk;//内核代码结束地址0xffff 8000 0011 4a08

	init_mm.start_stack = _stack_start;
	//head.S
	//ENTRY(_stack_start)
	//	.quad	init_task_union + 32768

	//init_thread,init_tss
	//内核主函数中之前执行的=>set_tss64(_stack_start, _stack_start, _stack_start, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00);

	//2.设置当前TSS为系统第一个进程的TSS
	set_tss64(init_thread.rsp0, init_tss[0].rsp1, init_tss[0].rsp2, init_tss[0].ist1, init_tss[0].ist2, init_tss[0].ist3, init_tss[0].ist4, init_tss[0].ist5, init_tss[0].ist6, init_tss[0].ist7);

	init_tss[0].rsp0 = init_thread.rsp0;

	list_init(&init_task_union.task.list);//初始化PCB链表,前驱指向自己,后继指向自己

	//至此完成系统第一个进程主体信息和控制信息的初始化,一些信息是在编译期就确定的,一些是在运行时补充完整的
	//接下来task_init函数执行kernel_thread函数为系统创建第二个进程(通常称为init进程),对于调用kernel_thread函数传入的CLONE_FS | CLONE_FILES | CLONE_SIGNAL等克隆标志位,目前未实现相应功能,预留使用
	kernel_thread(init,10,CLONE_FS | CLONE_FILES | CLONE_SIGNAL);

	init_task_union.task.state = TASK_RUNNING;//本来是不可中断等待状态,现在修改为可运行状态,以接受调度

	//#define offsetof(TYPE,MEMBER)   ((size_t) &((TYPE *)0)->MEMBER)
	//#define container_of(PTR,TYPE,MEMBER)    ({  \
	//	const typeof(((TYPE *)0)->MEMBER) *__mptr = (PTR);  \
	//	(TYPE *)((char *)__mptr - offsetof(TYPE, MEMBER)); })
	//
	//宏有三个参数:
	//	type:结构体类型
	//	member:结构体成员
	//	ptr:结构体成员member的地址
	//也就是说我们知道了一个结构体的类型,结构体内某一成员的地址就可以直接获得到这个结构体的首地址
	//container_of宏返回的就是这个结构体的首地址
	p = container_of(list_next(&current->list),struct task_struct,list);//task_init创建出内核线程后,使用内核第一宏获取init内核线程的进程控制结构体
	//首先成员运算符的优先级高于取地址运算符,&current->list获得当前进程控制块的链域的地址,进而由list_next获得当前进程链域的后继结点,此时的后继结点又是一个更大结构体(PCB)的成员
	//内核第一宏就获取到了当前进程的后续进程的PCB,也就是系统第二个进程init的PCB,然后赋予PCB进程控制块指针变量p

	switch_to(current,p);//取得init进程控制结构体后调用switch_to切换至init内核线程
	//当前进程(系统第一个进程)是前驱,init(系统第二个进程)是后继
}