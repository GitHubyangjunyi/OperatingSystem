#include "interrupt.h"
#include "linkage.h"
#include "lib.h"
#include "printk.h"
#include "memory.h"
#include "gate.h"

//外部硬件中断与处理异常一样,都使用IDT来索引处理程序,不同之处在于异常是处理器产生的任务暂停,而中断是外部设备产生的任务暂停
//因此中断和异常的处理工作都需要保存和还原任务现场,这个SAVE_ALL就是用来保存被外部硬件中断的任务现场
//这段汇编程序负责保存当前通用寄存器状态,与entry.S中的error_code模块极其相似,只不过中断处理程序的调用入口均指向同一中断处理函数do_IRQ
//而且中断触发时不会压入错误码,所以此处无法复用error_code模块,这段程序将被应用在下面的中断处理过程宏代码中
#define SAVE_ALL				\
	"cld;			\n\t"		\
	"pushq	%rax;		\n\t"		\
	"pushq	%rax;		\n\t"		\
	"movq	%es,	%rax;	\n\t"		\
	"pushq	%rax;		\n\t"		\
	"movq	%ds,	%rax;	\n\t"		\
	"pushq	%rax;		\n\t"		\
	"xorq	%rax,	%rax;	\n\t"		\
	"pushq	%rbp;		\n\t"		\
	"pushq	%rdi;		\n\t"		\
	"pushq	%rsi;		\n\t"		\
	"pushq	%rdx;		\n\t"		\
	"pushq	%rcx;		\n\t"		\
	"pushq	%rbx;		\n\t"		\
	"pushq	%r8;		\n\t"		\
	"pushq	%r9;		\n\t"		\
	"pushq	%r10;		\n\t"		\
	"pushq	%r11;		\n\t"		\
	"pushq	%r12;		\n\t"		\
	"pushq	%r13;		\n\t"		\
	"pushq	%r14;		\n\t"		\
	"pushq	%r15;		\n\t"		\
	"movq	$0x10,	%rdx;	\n\t"		\
	"movq	%rdx,	%ds;	\n\t"		\
	"movq	%rdx,	%es;	\n\t"


#define IRQ_NAME2(nr) nr##_interrupt(void)
#define IRQ_NAME(nr) IRQ_NAME2(IRQ##nr)

#define Build_IRQ(nr)							\
void IRQ_NAME(nr);						\
__asm__ (	SYMBOL_NAME_STR(IRQ)#nr"_interrupt:		\n\t"	\
			"pushq	$0x00				\n\t"	\
			SAVE_ALL					\
			"movq	%rsp,	%rdi			\n\t"	\
			"leaq	ret_from_intr(%rip),	%rax	\n\t"	\
			"pushq	%rax				\n\t"	\
			"movq	$"#nr",	%rsi			\n\t"	\
			"jmp	do_IRQ	\n\t");


Build_IRQ(0x20)
Build_IRQ(0x21)
Build_IRQ(0x22)
Build_IRQ(0x23)
Build_IRQ(0x24)
Build_IRQ(0x25)
Build_IRQ(0x26)
Build_IRQ(0x27)
Build_IRQ(0x28)
Build_IRQ(0x29)
Build_IRQ(0x2a)
Build_IRQ(0x2b)
Build_IRQ(0x2c)
Build_IRQ(0x2d)
Build_IRQ(0x2e)
Build_IRQ(0x2f)
Build_IRQ(0x30)
Build_IRQ(0x31)
Build_IRQ(0x32)
Build_IRQ(0x33)
Build_IRQ(0x34)
Build_IRQ(0x35)
Build_IRQ(0x36)
Build_IRQ(0x37)

//函数指针数组,每个元素都指向由宏函数定义的一个中断处理函数入口
void (* interrupt[24])(void)=
{
	IRQ0x20_interrupt,
	IRQ0x21_interrupt,
	IRQ0x22_interrupt,
	IRQ0x23_interrupt,
	IRQ0x24_interrupt,
	IRQ0x25_interrupt,
	IRQ0x26_interrupt,
	IRQ0x27_interrupt,
	IRQ0x28_interrupt,
	IRQ0x29_interrupt,
	IRQ0x2a_interrupt,
	IRQ0x2b_interrupt,
	IRQ0x2c_interrupt,
	IRQ0x2d_interrupt,
	IRQ0x2e_interrupt,
	IRQ0x2f_interrupt,
	IRQ0x30_interrupt,
	IRQ0x31_interrupt,
	IRQ0x32_interrupt,
	IRQ0x33_interrupt,
	IRQ0x34_interrupt,
	IRQ0x35_interrupt,
	IRQ0x36_interrupt,
	IRQ0x37_interrupt,
};

//初始化主/从8259A中断控制器和中断描述符表IDT中的各门描述符,则部分程序首先使用set_intr_gate将中断向量/中断处理函数配置到对应的描述符中
//外部硬件的中断向量号从32号开始,接下来对主/从8259A中断控制器进行初始化赋值,最后复位主/从8259A中断控制器的中断屏蔽寄存器IMR的全部中断屏蔽位,并使能中断(置位EFLAGS的IF位)
void init_interrupt()
{
	int i;
	for(i = 32;i < 56;i++)
	{
		set_intr_gate(i , 2 , interrupt[i - 32]);
	}

	color_printk(RED,BLACK,"8259A init \n");

	//8259A-master	ICW1-4
	io_out8(0x20,0x11);
	io_out8(0x21,0x20);
	io_out8(0x21,0x04);
	io_out8(0x21,0x01);

	//8259A-slave	ICW1-4
	io_out8(0xa0,0x11);
	io_out8(0xa1,0x28);
	io_out8(0xa1,0x02);
	io_out8(0xa1,0x01);

	//8259A-M/S	OCW1
	io_out8(0x21,0x00);
	io_out8(0xa1,0x00);

	sti();
}

//中断处理函数do_IRQ是中断处理函数的主函数,作用是分发中断请求到各个中断处理函数,这部分功能到此还未实现,只能打印中断向量号,以表明正在执行中断处理函数
//并向主8259A中断控制器发送EOI命令复位ISR寄存器,此时运行内核将不间断产生0x20中断(时钟中断),因为没有配置时钟中断就开启中断将导致不断有时钟中断请求产生
void do_IRQ(unsigned long regs,unsigned long nr)	//regs:rsp,nr
{
	color_printk(RED,BLACK,"do_IRQ:%#08x\t",nr);
	io_out8(0x20,0x20);//在中断处理之后向8259A发送EOI通知它中断处理结束
}