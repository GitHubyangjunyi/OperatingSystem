#include "lib.h"
#include "printk.h"
#include "gate.h"
#include "trap.h"
#include "memory.h"
#include "task.h"

/*
		static var 
*/

struct Global_Memory_Descriptor memory_management_struct = {{0},0};

void Start_Kernel(void)
{
	int *addr = (int *)0xffff800000a00000;
	int i;

	Pos.XResolution = 1440;
	Pos.YResolution = 900;

	Pos.XPosition = 0;
	Pos.YPosition = 0;

	Pos.XCharSize = 8;
	Pos.YCharSize = 16;

	Pos.FB_addr = (int *)0xffff800000a00000;
	Pos.FB_length = (Pos.XResolution * Pos.YResolution * 4 + PAGE_4K_SIZE - 1) & PAGE_4K_MASK;

	load_TR(8);//加载TSS选择子

	//相对于之前有所改动
	set_tss64(_stack_start, _stack_start, _stack_start, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00);

	sys_vector_init();

	memory_management_struct.start_code = (unsigned long)& _text;//内核代码段开始地址0xffff 8000 0010 0000
	memory_management_struct.end_code   = (unsigned long)& _etext;//内核代码段结束地址0xffff 8000 0010 95d0(允许误差)
	memory_management_struct.end_data   = (unsigned long)& _edata;//内核数据段结束地址0xffff 8000 0011 06a0
	memory_management_struct.end_brk    = (unsigned long)& _end;//内核代码结束地址0xffff 8000 0011 4a08

	color_printk(RED,BLACK,"memory init \n");
	init_memory();

	color_printk(RED,BLACK,"interrupt init \n");
	init_interrupt();

	color_printk(RED,BLACK,"task_init \n");
	//系统第一个进程的初始化工作task_init();
	//再调用函数kernel_thread为系统创建出一个新的进程,随后借助switch_to模块执行进程以实现切换
	task_init();

	while(1)
		;
}