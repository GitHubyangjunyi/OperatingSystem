#include "memory.h"
#include "lib.h"

void init_memory()
{
	int i,j;
	unsigned long TotalMem = 0 ;
	struct Memory_E820_Formate *p = NULL;
	//关于可用物理内存的相关信息之前已经在loader加载程序中通过BIOS中断获得,同时本系统使用2MB物理页,因此不仅仅统计可用物理页和分配物理页均是基于2MB物理页实现的软件逻辑,乃至整个内存管理单元
	//之前loader引导加载程序通过INT 15h AX=E820h获得并保存在物理地址7E00h处,这里将该物理地址处的信息提取出来转换成相应的结构体再加以统计,地址7E00h处的物理地址空间信息分为若干组,数量依据
	//当前主板硬件配置和物理内存容量信息而定,每条物理地址空间信息占20B
	//
	color_printk(BLUE,BLACK,"Display Physics Address MAP,Type(1:RAM,2:ROM or Reserved,3:ACPI Reclaim Memory,4:ACPI NVS Memory,Others:Undefine)\n");
	p = (struct Memory_E820_Formate *)0xffff800000007e00;//指针变量p指向线性地址0xffff800000007e00,由于7E00h是物理地址,必须经过页表映射后才能使用,转换后的线性地址是0xffff800000007e00
	
	for(i = 0;i < 32;i++)//通过32次循环逐条显示内存地址空间的分布信息
	{
		color_printk(ORANGE,BLACK,"Address:%#010x,%08x\tLength:%#010x,%08x\tType:%#010x\n",p->address2,p->address1,p->length2,p->length1,p->type);
		unsigned long tmp = 0;
		if(p->type == 1)//内存类型(典型的内存类型是有效物理内存01h,其标识着当前地址空间为可用物理内存,如果平台的内存容量过大,BIOS可能会检测出多个可用物理内存段)
		{
			tmp = p->length2;
			TotalMem += p->length1;
			TotalMem += tmp << 32;
		}

		p++;
		if(p->type > 4)//Bochs虚拟机检测出的地址空间分布信息,其type值不会大于4,如果出现大于4的情况则是遇见了程序运行时的脏数据,没有必要比对下去了,直接跳出循环
			break;		
	}

	color_printk(ORANGE,BLACK,"OS Can Used Total RAM:%#018lx\n",TotalMem);
	//可用物理内存空间(type = 1)由两部分组成,一部分是容量为9 f000h的段,另一部分是容量为7fef 0000h的段
	//总共(9 f000h + 7fef 0000h) => 7ff8 f000hB约等于2GB,这与配置虚拟平台运行环境时设置的参数megs:2048(物理内存容量)相符
}