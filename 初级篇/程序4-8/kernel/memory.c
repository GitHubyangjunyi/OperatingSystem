#include "memory.h"
#include "lib.h"

//主要负责初始化目标物理页的page结构体,并更新目标物理页所在区域空间结构zone内的统计信息
unsigned long page_init(struct Page * page,unsigned long flags)
{
	if(!page->attribute)//崭新的页面属性为0,则对page结构进行初始化
	{
		//将全局内存管理结构体中位图bits_map对应的此物理页标注为1被使用状态
		*(memory_management_struct.bits_map + ((page->PHY_address >> PAGE_2M_SHIFT) >> 6)) |= 1UL << (page->PHY_address >> PAGE_2M_SHIFT) % 64;
		page->attribute = flags;//页的属性(页的映射状态/活动状态/使用者等)64位位域
		page->reference_count++;//该页的引用次数64位位域加1

		page->zone_struct->page_using_count++;//该页所在区域已使用物理内存页数量加1
		page->zone_struct->page_free_count--;//该页所在区域空闲物理内存页数量减1
		page->zone_struct->total_pages_link++;//本区域物理页被引用次数加1,因为物理页在页表中的映射可以是一对多的关系,同时映射到线性地址空间的多个位置上,所以total_pages_link和page_using_count在数值上不一定相等
	}
	//如果当前页结构属性或参数flags中含有引用属性PG_Referenced或共享属性PG_K_Share_To_U,那么就只增加page结构体的引用计数和zone结构体的页面被引用计数
	else if((page->attribute & PG_Referenced) || (page->attribute & PG_K_Share_To_U) || (flags & PG_Referenced) || (flags & PG_K_Share_To_U))
	{
		//该物理页本来就含有引用属性或者共享属性/参数中含有引用属性或共享属性
		page->attribute |= flags;//添加属性
		page->reference_count++;//该页的引用次数64位位域加1
		page->zone_struct->total_pages_link++;//该页所在区域物理页被引用次数加1,因为物理页在页表中的映射可以是一对多的关系,同时映射到线性地址空间的多个位置上,所以total_pages_link和page_using_count在数值上不一定相等
	}
	//否则就只是添加页表属性,并置位bit位图相应的位
	else
	{
		//将全局内存管理结构体中位图bits_map对应的此物理页标注为1被使用状态
		*(memory_management_struct.bits_map + ((page->PHY_address >> PAGE_2M_SHIFT) >> 6)) |= 1UL << (page->PHY_address >> PAGE_2M_SHIFT) % 64;	
		page->attribute |= flags;//添加属性
	}
	return 0;
}

unsigned long page_clean(struct Page * page)
{
	if(!page->attribute)//崭新的页面属性为0,清空属性并返回
	{
		page->attribute = 0;//清空页属性
	}
	else if((page->attribute & PG_Referenced) || (page->attribute & PG_K_Share_To_U))//页属性含有引用属性或者共享属性
	{		
		page->reference_count--;//该页的引用次数64位位域减1
		page->zone_struct->total_pages_link--;//该页所在区域物理页被引用次数减1,因为物理页在页表中的映射可以是一对多的关系,同时映射到线性地址空间的多个位置上,所以total_pages_link和page_using_count在数值上不一定相等
		if(!page->reference_count)//页属性引用计数为0,这释放该页
		{
			page->attribute = 0;//清空页属性
			page->zone_struct->page_using_count--;//该页所在区域已使用物理内存页数量减1
			page->zone_struct->page_free_count++;//该页所在区域空闲物理内存页数量加1
		}
	}
	else
	{
		//将全局内存管理结构体中位图bits_map对应的此物理页标注为0未使用
		*(memory_management_struct.bits_map + ((page->PHY_address >> PAGE_2M_SHIFT) >> 6)) &= ~(1UL << (page->PHY_address >> PAGE_2M_SHIFT) % 64);

		page->attribute = 0;//清空页属性
		page->reference_count = 0;//该页的引用次数64位位域置0
		page->zone_struct->page_using_count--;//该页所在区域已使用物理内存页数量减1
		page->zone_struct->page_free_count++;//该页所在区域空闲物理内存页数量加1
		page->zone_struct->total_pages_link--;//该页所在区域物理页被引用次数减1,因为物理页在页表中的映射可以是一对多的关系,同时映射到线性地址空间的多个位置上,所以total_pages_link和page_using_count在数值上不一定相等
	}
	return 0;
}

void init_memory()
{
	int i,j;
	unsigned long TotalMem = 0 ;
	struct E820 *p = NULL;
	
	color_printk(BLUE,BLACK,"Display Physics Address MAP,Type(1:RAM,2:ROM or Reserved,3:ACPI Reclaim Memory,4:ACPI NVS Memory,Others:Undefine)\n");
	p = (struct E820 *)0xffff800000007e00;//指向由loader获取的物理内存空间信息结构体位置

	for(i = 0;i < 32;i++)
	{
		color_printk(ORANGE,BLACK,"Address:%#018lx\tLength:%#018lx\tType:%#010x\n",p->address,p->length,p->type);
		unsigned long tmp = 0;
		if(p->type == 1)//可用物理内存type为1
			TotalMem +=  p->length;

		memory_management_struct.e820[i].address += p->address;
		memory_management_struct.e820[i].length	 += p->length;
		memory_management_struct.e820[i].type	 = p->type;
		memory_management_struct.e820_length = i;

		p++;
		if(p->type > 4 || p->length == 0 || p->type < 1)//追加判断条件p->length == 0 || p->type < 1截断并剔除E829数组中的脏数据,之后根据物理地址空间划分信息计算出物理地址空间的结束地址(目前的结束地址位于最后一条物理内存段信息中,但不排除其他可能性)
			break;
	}
	//总共统计出了6个,memory_management_struct.e820_length = 5;
	color_printk(ORANGE,BLACK,"OS Can Used Total RAM:%#018lx\n",TotalMem);//这边统计的是类型为1的可用物理内存数量约等于2048MB

	TotalMem = 0;

	for(i = 0;i <= memory_management_struct.e820_length;i++)
	{
		unsigned long start,end;
		if(memory_management_struct.e820[i].type != 1)
			continue;
		start = PAGE_2M_ALIGN(memory_management_struct.e820[i].address);
		end   = ((memory_management_struct.e820[i].address + memory_management_struct.e820[i].length) >> PAGE_2M_SHIFT) << PAGE_2M_SHIFT;
		if(end <= start)
			continue;
		TotalMem += (end - start) >> PAGE_2M_SHIFT;
	}
	
	color_printk(ORANGE,BLACK,"OS Can Used Total 2M PAGEs:%#010x=%010d\n",TotalMem,TotalMem);//这边统计的是类型为1的可用物理内存页数量1022

	//从此处开始是新增代码,相对于4-7
	TotalMem = memory_management_struct.e820[memory_management_struct.e820_length].address + memory_management_struct.e820[memory_management_struct.e820_length].length;

	//成员变量bits_map是映射位图的指针,指向内核程序结束地址end_brk的4KB上边界对齐位置处,此举是为了保留一小段隔离空间以防止误操作其他空间的数据
	memory_management_struct.bits_map = (unsigned long *)((memory_management_struct.end_brk + PAGE_4K_SIZE - 1) & PAGE_4K_MASK);//end_brk内核程序的结束地址按4KB页对齐

	//把物理地址空间的结束地址按2MB页对齐,从而统计出物理地址空间可分页数,这个物理地址空间不仅包括可用物理内存,还包括内存空洞和ROM地址空间,将物理地址空间可分页数赋值给bits_size变量
	memory_management_struct.bits_size = TotalMem >> PAGE_2M_SHIFT;//0x800,这个物理地址空间不仅包括可用物理内存,还包括内存空洞和ROM地址空间,共可以分2048个2MB的页,共4GB
	//TotalMem=0x1 0000 0000=>4GB

	memory_management_struct.bits_length = (((unsigned long)(TotalMem >> PAGE_2M_SHIFT) + sizeof(long) * 8 - 1) / 8) & ( ~ (sizeof(long) - 1));//物理地址空间页映射位图长度
	//Bochs分配2GB内存得出物理地址空间页映射位图长度0x100=256
	//这里的对齐是为了向上对齐8字节,比如系统可以分成54个页,bits_size最后的未对齐长度是54,如果只是加上8而不是63,那么bits_length就等于7,用7个字节的56位映射这54个页就会很别扭
	//如果是加上63就变成117,除以8等于14字节,然后再清除低三位得到8,也就是用8个字节的64位映射这54个页

	//接着将整个空间置位memset以标注非内存页(内存空洞和ROM)已被使用,随后再通过程序将映射位图中的可用物理页复位
	memset(memory_management_struct.bits_map,0xff,memory_management_struct.bits_length);		//init bits map memory,整个位图全是f
	//至此整个memory_management_struct.bits_map位图全是f

	//这部分程序负责创建page结构体数组的存储空间和分配记录,page结构体数组的存储空间位于bit映射位图之后
	//数组的元素数量为物理地址空间可分页数,其分配与计算方式同bit映射位图相似,只不过此处将page结构体数组全部清零以备后续的初始化程序使用
	memory_management_struct.pages_struct = (struct Page *)(((unsigned long)memory_management_struct.bits_map + memory_management_struct.bits_length + PAGE_4K_SIZE - 1) & PAGE_4K_MASK);//按4KB页向上对齐

	memory_management_struct.pages_size = TotalMem >> PAGE_2M_SHIFT;//0x800

	memory_management_struct.pages_length = ((TotalMem >> PAGE_2M_SHIFT) * sizeof(struct Page) + sizeof(long) - 1) & ( ~ (sizeof(long) - 1));//struct page结构体数组长度
	//Bochs分配2GB内存得出struct page结构体数组长度0x14000,sizeof(struct Page)=40字节,还是要8字节对齐,后续进行高级内存管理将改动page结构体

	memset(memory_management_struct.pages_struct,0x00,memory_management_struct.pages_length);	//init pages memory,整个memory_management_struct.pages_struct数组全是0
	//至此整个memory_management_struct.pages_struct数组全是0

	//这部分程序负责创建zones结构体数组的存储空间和分配记录,与上面的过程基本相同,不过目前暂时无法计算出zone结构体数组的元素个数,只能将zones_size置0,而将zones_length暂且按照5个zone结构体来计算(后续这两个值都将会修正)
	//关于为什么内存管理既要分页又要分zone可参看关于8086体系结构的两种内存硬件约束
	memory_management_struct.zones_struct = (struct Zone *)(((unsigned long)memory_management_struct.pages_struct + memory_management_struct.pages_length + PAGE_4K_SIZE - 1) & PAGE_4K_MASK);//按4KB页向上对齐

	memory_management_struct.zones_size   = 0;//目前暂时无法计算出zone结构体数组的元素个数,只能将zones_size置0,而将zones_length暂且按照5个zone结构体来计算,随后会在收尾工作时修正zones_length
	//因为之前统计的是所有内存,这个物理地址空间不仅包括可用物理内存,还包括内存空洞和ROM地址空间,而zone_size要记录的是可用物理内存区域数
	memory_management_struct.zones_length = (5 * sizeof(struct Zone) + sizeof(long) - 1) & (~(sizeof(long) - 1));//struct zone结构体数组长度,随后会在收尾工作时修正zones_length
	//Bochs分配2GB内存得出struct zone结构体数组长度0x190,sizeof(struct Zone)=80字节,还是要8字节对齐,后续进行高级内存管理将改动zone结构体


	memset(memory_management_struct.zones_struct,0x00,memory_management_struct.zones_length);	//init zones memory,memory_management_struct.zones_struct全是0

	//通过上述的存储空间的创建后,此刻再遍历E820数组来完成各数组成员变量的初始化工作,下面的程序是初始化bit位图映射/page结构体/zone结构体的核心代码
	//将遍历全部物理内存段信息以初始化可用物理内存段,首先过滤掉非物理内存段,再将剩下的可用物理内存段进行页对齐,如果本段物理内存有可用物理页,则把该段空间视为一个可用的zone区域并对其初始化
	//(*(memory_management_struct.bits_map + ((p->PHY_address >> PAGE_2M_SHIFT) >> 6)) ^= 1UL << (p->PHY_address >> PAGE_2M_SHIFT) % 64;)
	//会把当前page结构体所代表的物理地址转换成bits_map映射位图中对应的位,由于此前已经将bits_map位图全部置位,此刻再将可用物理页对应的位和1执行异或操作,以将对应的页标注为未被使用
	for(i = 0;i <= memory_management_struct.e820_length;i++)
	{
		unsigned long start,end;
		struct Zone * z;
		struct Page * p;
		unsigned long * b;

		if(memory_management_struct.e820[i].type != 1)//不是可用物理内存空间,直接跳过,保持之前的置位状态
			continue;

		//如果是可用物理内存
		start = PAGE_2M_ALIGN(memory_management_struct.e820[i].address);//将addr参数按2MB页的上边界对齐
		end   = ((memory_management_struct.e820[i].address + memory_management_struct.e820[i].length) >> PAGE_2M_SHIFT) << PAGE_2M_SHIFT;//将addr参数按2MB页的下边界对齐
		if(end <= start)//经由上面的对齐规则,如果end<=start则表示该页小于2MB,不管
			continue;
		
		//zone初始化
		z = memory_management_struct.zones_struct + memory_management_struct.zones_size;//memory_management_struct.zones_size之前初始化为0,此处可以理解为偏移到第几个zone结构体
		memory_management_struct.zones_size++;//增加偏移量以及可用物理内存区域数(此处累计的zone_size是可用的物理内存区域数,之前无法计算是有原因的,修正是在此处进行修正)

		z->zone_start_address = start;//设置区域起始地址(按2MB页的上边界对齐)
		z->zone_end_address = end;	//设置区域结束地址(按2MB页的下边界对齐)
		z->zone_length = end - start;//设置区域长度

		z->page_using_count = 0;//本区域已使用物理内存页数量
		z->page_free_count = (end - start) >> PAGE_2M_SHIFT;//本区域空闲物理内存页数量,初始化时设置为本区域所有的分页数

		z->total_pages_link = 0;//本区域物理页被引用次数,因为物理页在页表中的映射可以是一对多的关系,同时映射到线性地址空间的多个位置上,所以total_pages_link和page_using_count在数值上不一定相等

		z->attribute = 0;//属性置0
		z->GMD_struct = &memory_management_struct;//指向全局内存管理结构体

		z->pages_length = (end - start) >> PAGE_2M_SHIFT;//本区域包含struct Page结构体数量=本区域空闲物理内存页数量

		z->pages_group =  (struct Page *)(memory_management_struct.pages_struct + (start >> PAGE_2M_SHIFT));//struct Page结构体数组指针,指向本区域的第一个page结构体,通过该page结构体指针可以获取到后续的各个page结构体

		//page初始化,初始化此zone区域管理的所有page结构体
		p = z->pages_group;//指向zone区域的第一个page结构体
		for(j = 0;j < z->pages_length; j++ , p++)
		{
			p->zone_struct = z;//指向自身所在区域的结构体
			p->PHY_address = start + PAGE_2M_SIZE * j;//页的起始地址(PAGE_2M_SIZE * j)控制地址补偿
			p->attribute = 0;//初始化属性为0

			p->reference_count = 0;//引用计数0

			p->age = 0;//创建时间0

			//会把当前page结构体所代表的物理地址转换成bits_map映射位图中对应的位,由于此前已经将bits_map位图全部置位,此刻再将可用物理页对应的位和1执行异或操作,以将对应的页标注为0未被使用
			//%运算符优先于>>,bits_map是unsigned long,长度是8字节
			*(memory_management_struct.bits_map + ((p->PHY_address >> PAGE_2M_SHIFT) >> 6)) ^= 1UL << (p->PHY_address >> PAGE_2M_SHIFT) % 64;
			//算法思想:比如说我要修改第70个物理页,那么我就要获取到第70页所代表的那个bit,因为bits_map是unsigned long,长度是8字节,可以代表64个页面,我就要得到所要修改的物理页在第几个64位区域(从0开始计)
			//那么我就要先得到物理页(p->PHY_address >> PAGE_2M_SHIFT),然后扫除不够64页的剩余值,那么得到的就是要修改的物理页所在的64位区域,第70个页在第1个64位区域(相应的54在第0个64位区域)
			//然后求解该页在该64区域的哪个bit位,此时需要使用物理页编号(p->PHY_address >> PAGE_2M_SHIFT)取模64(区域)得到在该区域的第几个bit位,比如70是在第1个64区域的第6位
			//100 0000(从右其第一个0代表第64个页,1就代表第70页)
			//最后用memory_management_struct.bits_map+区域编号然后异或上该页所在区域的那个bit位,最终修改掉代表那个物理页的bit位
		}
		
	}
	//至此初步初始化全局内存空间结构体memory_management_struct的信息以及指向的zone/page结构体数组信息,接下来进行收尾工作

	//init address 0 to page struct 0; because the memory_management_struct.e820[0].type != 1
	//(收尾工作)经过一番遍历后,所有可用物理页都被初始化了,但是由于0~2MB的物理内存包含多个物理段,还包括了内核程序,所以必须对该页进行特殊初始化,此后方可计算出zone区域空间结构体数组的元素数量
	memory_management_struct.pages_struct->zone_struct = memory_management_struct.zones_struct;//全局内存信息结构体的第一个page结构体所指向的zone区域=全局内存信息结构体的第一个zone区域

	memory_management_struct.pages_struct->PHY_address = 0UL;//全局内存信息结构体的第一个page结构体的起始地址0UL
	memory_management_struct.pages_struct->attribute = 0;//全局内存信息结构体的第一个page结构体的属性为0
	memory_management_struct.pages_struct->reference_count = 0;//全局内存信息结构体的第一个page结构体的引用计数为0
	memory_management_struct.pages_struct->age = 0;//全局内存信息结构体的第一个page结构体的创建时间为0
	//至此初始化完0~2MB的物理页的page结构体信息(共5个)

	memory_management_struct.zones_length = (memory_management_struct.zones_size * sizeof(struct Zone) + sizeof(long) - 1) & ( ~ (sizeof(long) - 1));//8字节对齐

	//个数组成员变量初始化后将其中某些关键性信息打印输出
	color_printk(ORANGE,BLACK,"bits_map:%#018lx,bits_size:%#018lx,bits_length:%#018lx\n",memory_management_struct.bits_map,memory_management_struct.bits_size,memory_management_struct.bits_length);
	color_printk(ORANGE,BLACK,"pages_struct:%#018lx,pages_size:%#018lx,pages_length:%#018lx\n",memory_management_struct.pages_struct,memory_management_struct.pages_size,memory_management_struct.pages_length);
	color_printk(ORANGE,BLACK,"zones_struct:%#018lx,zones_size:%#018lx,zones_length:%#018lx\n",memory_management_struct.zones_struct,memory_management_struct.zones_size,memory_management_struct.zones_length);


	//ZONE_DMA_INDEX和ZONE_NORMAL_INDEX暂时无法区分,先将它们指向同一个zone区域空间然后再显示zone结构体的详细信息
	ZONE_DMA_INDEX = 0;		//need rewrite in the future
	ZONE_NORMAL_INDEX = 0;	//need rewrite in the future

	//此段程序遍历显示各个区域空间结构体zone的详细统计信息,如果当前区域的起始地址是0x1 0000 0000,就将此区域索引值记录在全局变量ZONE_UNMAPED_INDEX内
	//表示从该区域空间开始的物理地址内存页未曾经过页表映射,最后还要调整end_of_struct的值,以记录上述结构的结束地址,并且预留一段内存空间防止越界访问
	for(i = 0;i < memory_management_struct.zones_size;i++)	//need rewrite in the future
	{
		struct Zone * z = memory_management_struct.zones_struct + i;
		color_printk(ORANGE,BLACK,"zone_start_address:%#018lx,zone_end_address:%#018lx,zone_length:%#018lx,pages_group:%#018lx,pages_length:%#018lx\n",z->zone_start_address,z->zone_end_address,z->zone_length,z->pages_group,z->pages_length);

		if(z->zone_start_address == 0x100000000)//在初级内存管理且分配2GB内存的情况下不会执行
			ZONE_UNMAPED_INDEX = i;
	}
	
	memory_management_struct.end_of_struct = (unsigned long)((unsigned long)memory_management_struct.zones_struct + memory_management_struct.zones_length + sizeof(long) * 32) & ( ~ (sizeof(long) - 1));//need a blank to separate memory_management_struct
	//加上sizeof(long) * 32=>256字节数留空,并按8字节对齐

	//内存管理单元初始化完毕后,还必须初始化内存管理单元结构(extern struct Global_Memory_Descriptor memory_management_struct;)所占物理页的page结构体
	color_printk(ORANGE,BLACK,"start_code:%#018lx,end_code:%#018lx,end_data:%#018lx,end_brk:%#018lx,end_of_struct:%#018lx\n",memory_management_struct.start_code,memory_management_struct.end_code,memory_management_struct.end_data,memory_management_struct.end_brk, memory_management_struct.end_of_struct);
	//将系统内核与内存管理单元结构所占物理页的page结构体全部初始化成PG_PTable_Maped | PG_Kernel_Init | PG_Active | PG_Kernel
	//经过页表映射的页/内核初始化程序/使用中的页/内核层页
	//从运行结果可以看出整个虚拟平台只有一段物理内存可供物理内存页分配使用,可分配的物理页数量为1022个
	//内核代码起始地址是0xffff 8000 0010 0000
	//代码段的结束地址是0xffff 8000 0010 95d0(允许误差)
	//数据段的结束地址是0xffff 8000 0011 06a0
	//内核程序结束地址是0xffff 8000 0011 4a08
	//初始化的数据区结束地址是0xffff 8000 0012 a150
	//页目录(PML4页表)的首物理地址是0x10 1000
	//PDPT页表的首物理地址位于0x10 2000
	//PD页表的首物理地址位于0x10 3000
	
	i = Virt_To_Phy(memory_management_struct.end_of_struct) >> PAGE_2M_SHIFT;//将全局内存管理结构的内核层虚拟地址转换成物理地址,按2MB页右移得出是系统第几个物理页,作为循环结束标记

	for(j = 0;j <= i;j++)//系统内核以及内存管理单元结构所占物理页的page结构体全部初始化成PG_PTable_Maped | PG_Kernel_Init | PG_Active | PG_Kernel
	{
		page_init(memory_management_struct.pages_struct + j,PG_PTable_Maped | PG_Kernel_Init | PG_Active | PG_Kernel);
	}

	//剩余代码用于清空页表项,这系统页表曾经用于一致性页表映射,此刻无需再保留一致性页表映射,清空这些页表项
	//一致性页表映射是不是0~10MB的物理地址映射给0~10MB线性地址??????

	Global_CR3 = Get_gdt();//读取CR3控制寄存器保存的页目录基地址,然后打印几层页表的首地址,由于页表项只能保存页表地址,那么根据内核执行头head.S初始化的页表项可知
	color_printk(INDIGO,BLACK,"Global_CR3\t:%#018lx\n",Global_CR3);//Global_CR3变量保存的物理地址是0x0000 0000 0010 1000
	color_printk(INDIGO,BLACK,"*Global_CR3\t:%#018lx\n",*Phy_To_Virt(Global_CR3) & (~0xff));//*Global_CR3保存到物理地址是0x0000 0000 0010 2000(先转换成虚拟地址,得出结果后清除低8属性)
	color_printk(PURPLE,BLACK,"**Global_CR3\t:%#018lx\n",*Phy_To_Virt(*Phy_To_Virt(Global_CR3) & (~0xff)) & (~0xff));//**Global_CR3保存到物理地址是0x0000 0000 0010 3000(先转换成虚拟地址,得出结果后清除低8属性)

	//紧接着为了消除一致性页表映射,特别将页目录(PML4页表)中的前10个页表项清零(其实只要清零第一项就可以了),虽然已经清零但不会立即生效,必须使用flush_tlb才能让更改的页表项生效
	for(i = 0;i < 10;i++)
		*(Phy_To_Virt(Global_CR3)  + i) = 0UL;
	
	flush_tlb();//刷新TLB使更改后的页表生效(重新赋值CR3)

}

/*
	number: number < 64
	zone_select: zone select from dma , mapped in  pagetable , unmapped in pagetable
	page_flags: struct Page flages
*/
//alloc_pages可用物理内存页分配函数会从已初始化的内存管理单元结构中搜索符合申请条件的page,并将其设置为使用状态(此函数在初级阶段可能会有缺陷)
//已实现的功能:它最多可从DMA区域空间/已映射页表区域空间或者未映射页表区域空间里一次性申请64个连续的物理页,并设置这些物理页对应的page属性
struct Page * alloc_pages(int zone_select,int number,unsigned long page_flags)//zone_select参数为需要检索的内存区域空间
{
	int i;
	unsigned long page = 0;

	int zone_start = 0;
	int zone_end = 0;

	//之前的init_memory()中的代码将这两个变量值置零
	//ZONE_DMA_INDEX和ZONE_NORMAL_INDEX暂时无法区分,先将它们指向同一个zone区域空间然后再显示zone结构体的详细信息
	//ZONE_DMA_INDEX = 0;		//need rewrite in the future
	//ZONE_NORMAL_INDEX = 0;	//need rewrite in the future

	switch(zone_select)
	{
		case ZONE_DMA:
				zone_start = 0;
				zone_end = ZONE_DMA_INDEX;
			break;

		case ZONE_NORMAL://匹配到这里
				zone_start = ZONE_DMA_INDEX;//0
				zone_end = ZONE_NORMAL_INDEX;//0
			break;

		case ZONE_UNMAPED:
				zone_start = ZONE_UNMAPED_INDEX;
				zone_end = memory_management_struct.zones_size - 1;
			break;

		default:
			color_printk(RED,BLACK,"alloc_pages error zone_select index\n");//参数zone_select无法匹配到相应的区域空间,打印错误信息并返回
			return NULL;//目前Bochs虚拟机只能开辟2GB物理内存空间,以至于虚拟平台只有一个可用物理内存段
			break;//因此ZONE_DMA_INDEX/ZONE_NORMAL_INDEX/ZONE_UNMAPED_INDEX均代表同一内存区域空间,即使用默认值0代表的内存区域空间
	}
	//至此确定完检测的目标内存区域空间,接下来从该区域空间中遍历出符合申请条件的page结构体数组,这部分代码从目标内存区域空间的起始内存页结构开始逐一遍历,直至内存区域空间的结尾
	//由于起始内存页结构对应的bit映射位图往往位于非对齐(按unsigned long类型对齐)的位置处,而且每次将按unsigned long类型作为遍历步进长度,同时步进过程还会按unsigned long类型对齐
	//因此起始页的bit映射位图只能检索tmp = 64 - start % 64;次,随后借助代码j += j % 64 ? tmp : 64将索引变量j调整到对齐位置处,为了保证alloc_pages函数可以检索出64个连续的物理页
	//特地使用程序(*p >> k) | (*(p + 1) << (64 - k))将后一个unsigned long变量的低位部分补齐到正在检索的变量中,只有这样才能保证最多可申请64个连续物理页
	for(i = zone_start;i <= zone_end; i++)
	{
		struct Zone * z;
		unsigned long j;
		unsigned long start,end,length;
		unsigned long tmp;

		if((memory_management_struct.zones_struct + i)->page_free_count < number)//如果要检索的区域的空闲页数小于要申请的页数,则到下一个区域进行搜索
			continue;

		z = memory_management_struct.zones_struct + i;//获取将要检索的zone区域
		start = z->zone_start_address >> PAGE_2M_SHIFT;//开始地址2MB对齐处
		end = z->zone_end_address >> PAGE_2M_SHIFT;//结束地址2MB对齐处
		length = z->zone_length >> PAGE_2M_SHIFT;//要检索的区域的总2MB页数

		tmp = 64 - start % 64;

		for(j = start;j <= end;j += j % 64 ? tmp : 64)//将索引变量j调整到对齐位置处,条件运算符高于+=
		{
			unsigned long * p = memory_management_struct.bits_map + (j >> 6);//8字节处对齐,比如zone区域从72字节100 1000处开始,那么挤掉低6位从64处开始计,p是个指向8字节长度的unsigned long的变量的指针
			unsigned long shift = j % 64;//
			unsigned long k;
			for(k = shift;k < 64 - shift;k++)//边界条件32退出,则跳转到p+1指向的位图,重新匹配
			{																							//比如要申请4页,那么左移4位减去1取反=>1111B(连续4个1)
				if( !( ( (*p >> k) | (*(p + 1) << (64 - k)) ) & (number == 64 ? 0xffffffffffffffffUL : ((1UL << number) - 1)) ) )//此处就是用位图映射与申请的连续数量进行相与
				{
					//比如要申请64个空白页,如果映射位图没有连续的64个0,那么此位图区域映射的内存页就不是连续的64个页,不能使用,直到找到连续的64个比特0条件才成立
					//(*p >> k) | (*(p + 1) << (64 - k))将后一个unsigned long变量的低位部分补齐到正在检索的变量中,只有这样才能保证最多可申请64个连续物理页
					//左移符号<<对应的汇编指令是逻辑左移SHL,对于一个64位寄存器来说,其左移范围是0~63,当申请值number为64时,必须经过特殊处理才能进行位图检索
					//(number == 64 ? 0xffffffffffffffffUL : ((1UL << number) - 1))的原因
					unsigned long	l;
					page = j + k - 1;//匹配到的话page的值将等于偏移值,比如j=64,k=0时匹配到,那么page=63,memory_management_struct.pages_struct + page=>memory_management_struct.pages_struct + 63就等于第64个
					for(l = 0;l < number;l++)
					{
						struct Page * x = memory_management_struct.pages_struct + page + l;
						page_init(x,page_flags);//将各个页初始化
					}
					goto find_free_pages;//初始化完各个页属性跳转到find_free_pages
				}
			}
		
		}
	}

	return NULL;

find_free_pages:
	//最后如果检索出满足条件的物理页组,便将bit映射位图对应的内存页结构page初始化并返回第一个内存页结构的地址
	return (struct Page *)(memory_management_struct.pages_struct + page);//返回第一个内存页结构的地址(此处的第一个是指找到的第一个物理页)
}

//在内核主函数中连续申请64个可用物理页,并打印内存页结构的属性和起始地址,以及申请前后的位图映射信息
//从图中可以看出虚拟平台的前64个(从0计)内存页结构的属性值已经被设置为0x91,而且物理地址从0x20 0000开始,这与zone_start_address成员变量记录的地址值是一致的
//进而说明alloc_pages函数是从本区域空间的起始地址分配物理内存页的,区域空间的第64和65项内存页结构的属性依然是0,说明未被分配过
//同时bit映射位图也从原来的0x0000 0000 0000 0001(第一个2MB物理页存放着内核程序和内存管理单元结构信息等内容,这个页已经在init_menmory中被初始化)
//变成现在的0x1 ffff ffff ffff ffff,同样也是置位64个映射位,以上是可用物理内存页分配函数的第一个版本(还不完善)