#include "lib.h"
#include "printk.h"
#include "gate.h"
#include "trap.h"
#include "memory.h"

/*
		static var 
*/
//配合链接脚本赋予全局内存信息结构体
extern char _text;
extern char _etext;
extern char _edata;
extern char _end;

struct Global_Memory_Descriptor memory_management_struct = {{0},0};//前两个成员变量必须保证初始化以保证数据正确性

void Start_Kernel(void)
{
	int *addr = (int *)0xffff800000a00000;
	int i;

//	struct Page * page = NULL;

	Pos.XResolution = 1440;
	Pos.YResolution = 900;

	Pos.XPosition = 0;
	Pos.YPosition = 0;

	Pos.XCharSize = 8;
	Pos.YCharSize = 16;

	Pos.FB_addr = (int *)0xffff800000a00000;
	Pos.FB_length = (Pos.XResolution * Pos.YResolution * 4 + PAGE_4K_SIZE - 1) & PAGE_4K_MASK;

	load_TR(8);

	set_tss64(0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00, 0xffff800000007c00);

	sys_vector_init();

	memory_management_struct.start_code = (unsigned long)& _text;//内核代码段开始地址0xffff 8000 0010 0000
	memory_management_struct.end_code   = (unsigned long)& _etext;//内核代码段结束地址0xffff 8000 0010 95d0(允许误差)
	memory_management_struct.end_data   = (unsigned long)& _edata;//内核数据段结束地址0xffff 8000 0011 06a0
	memory_management_struct.end_brk    = (unsigned long)& _end;//内核代码结束地址0xffff 8000 0011 4a08
	//至此end_brk保存的是内核程序的结尾地址,这个地址后的内存空间可以任意使用,现在将struct page和struct zone结构体数组保存在此处,由init_memory实现


	color_printk(RED,BLACK,"memory init \n");
	init_memory();

	color_printk(RED,BLACK,"memory_management_struct.bits_map:%#018lx\n",*memory_management_struct.bits_map);
	color_printk(RED,BLACK,"memory_management_struct.bits_map:%#018lx\n",*(memory_management_struct.bits_map + 1));

	struct Page *page=NULL;
	page = alloc_pages(ZONE_NORMAL,64,PG_PTable_Maped | PG_Active | PG_Kernel);//1001 0001=>0x91

	for(i = 0;i <= 64;i++)//连续申请64个可用物理页,并打印内存页结构的属性和起始地址,以及申请前后的位图映射信息
	{
		color_printk(INDIGO,BLACK,"page%d\tattribute:%#018lx\taddress:%#018lx\t",i,(page + i)->attribute,(page + i)->PHY_address);
		i++;
		color_printk(INDIGO,BLACK,"page%d\tattribute:%#018lx\taddress:%#018lx\n",i,(page + i)->attribute,(page + i)->PHY_address);//为了打印出65号页的属性
	}

	color_printk(RED,BLACK,"memory_management_struct.bits_map:%#018lx\n",*memory_management_struct.bits_map);//0xffff ffff ffff ffff,从低到高映射
	color_printk(RED,BLACK,"memory_management_struct.bits_map:%#018lx\n",*(memory_management_struct.bits_map + 1));//0x0000 0000 0000 0001

	while(1)
		;
}
