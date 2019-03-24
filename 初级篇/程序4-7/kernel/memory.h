/***************************************************
*		版权声明
*
*	本操作系统名为：MINE
*	该操作系统未经授权不得以盈利或非盈利为目的进行开发，
*	只允许个人学习以及公开交流使用
*
*	代码最终所有权及解释权归田宇所有；
*
*	本模块作者：	田宇
*	EMail:		345538255@qq.com
*
*
***************************************************/

#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "printk.h"
#include "lib.h"

//64位模式下每个页表项占用8字节而不是4字节,每个页表大小4KB,共512个页表项
#define PTRS_PER_PAGE	512

#define PAGE_OFFSET	((unsigned long)0xffff800000000000)//内核层的起始线性地址,该地址位于地址0处(此值必须经过页表重映射)
//映射过程,高16位不使用,接下来9位是1 0000 0000=>0x100=>第256个PML4E=>0x102007(PDPT物理地址)=>第0项=>0x000083=>物理地址0x0处开始
#define PAGE_GDT_SHIFT	39
#define PAGE_1G_SHIFT	30	//2的30次方是1GB
#define PAGE_2M_SHIFT	21	//2的21次方是2MB
#define PAGE_4K_SHIFT	12	//2的12次方是4KB

#define PAGE_2M_SIZE	(1UL << PAGE_2M_SHIFT)//宏展开后会将1向左移动PAGE_2M_SHIFT位
#define PAGE_4K_SIZE	(1UL << PAGE_4K_SHIFT)//宏展开后会将1向左移动PAGE_4K_SHIFT位

#define PAGE_2M_MASK	(~ (PAGE_2M_SIZE - 1))//2MB数值的屏蔽码,通常用于屏蔽低于2MB的值
#define PAGE_4K_MASK	(~ (PAGE_4K_SIZE - 1))//4MB数值的屏蔽码,通常用于屏蔽低于4MB的值

#define PAGE_2M_ALIGN(addr)	(((unsigned long)(addr) + PAGE_2M_SIZE - 1) & PAGE_2M_MASK)//将addr参数按2MB页的上边界对齐
#define PAGE_4K_ALIGN(addr)	(((unsigned long)(addr) + PAGE_4K_SIZE - 1) & PAGE_4K_MASK)//将addr参数按4MB页的上边界对齐

#define Virt_To_Phy(addr)	((unsigned long)(addr) - PAGE_OFFSET)//用于将内核层虚拟地址转换成物理地址,但有条件限制,目前只有物理地址的前10MB被映射到线性地址0xffff800000000000处(head.S定义的页表中)
#define Phy_To_Virt(addr)	((unsigned long *)((unsigned long)(addr) + PAGE_OFFSET))//也只有这10MB可供该宏函数使用,随着内核功能的完善,可用空间将会越来越大,这一行的宏与上一行相反

#define Virt_To_2M_Page(kaddr)	(memory_management_struct.pages_struct + (Virt_To_Phy(kaddr) >> PAGE_2M_SHIFT))
#define Phy_to_2M_Page(kaddr)	(memory_management_struct.pages_struct + ((unsigned long)(kaddr) >> PAGE_2M_SHIFT))


////page table attribute

//	bit 63	Execution Disable:
#define PAGE_XD		(unsigned long)0x1000000000000000

//	bit 12	Page Attribute Table
#define	PAGE_PAT	(unsigned long)0x1000

//	bit 8	Global Page:1,global;0,part
#define	PAGE_Global	(unsigned long)0x0100

//	bit 7	Page Size:1,big page;0,small page;
#define	PAGE_PS		(unsigned long)0x0080

//	bit 6	Dirty:1,dirty;0,clean;
#define	PAGE_Dirty	(unsigned long)0x0040

//	bit 5	Accessed:1,visited;0,unvisited;
#define	PAGE_Accessed	(unsigned long)0x0020

//	bit 4	Page Level Cache Disable
#define PAGE_PCD	(unsigned long)0x0010

//	bit 3	Page Level Write Through
#define PAGE_PWT	(unsigned long)0x0008

//	bit 2	User Supervisor:1,user and supervisor;0,supervisor;
#define	PAGE_U_S	(unsigned long)0x0004

//	bit 1	Read Write:1,read and write;0,read;
#define	PAGE_R_W	(unsigned long)0x0002

//	bit 0	Present:1,present;0,no present;
#define	PAGE_Present	(unsigned long)0x0001

//1,0
#define PAGE_KERNEL_GDT		(PAGE_R_W | PAGE_Present)

//1,0	
#define PAGE_KERNEL_Dir		(PAGE_R_W | PAGE_Present)

//7,1,0
#define	PAGE_KERNEL_Page	(PAGE_PS  | PAGE_R_W | PAGE_Present)

//2,1,0
#define PAGE_USER_Dir		(PAGE_U_S | PAGE_R_W | PAGE_Present)

//7,2,1,0
#define	PAGE_USER_Page		(PAGE_PS  | PAGE_U_S | PAGE_R_W | PAGE_Present)

typedef struct {unsigned long pml4t;} pml4t_t;
#define	mk_mpl4t(addr,attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_mpl4t(mpl4tptr,mpl4tval)	(*(mpl4tptr) = (mpl4tval))

typedef struct {unsigned long pdpt;} pdpt_t;
#define mk_pdpt(addr,attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_pdpt(pdptptr,pdptval)	(*(pdptptr) = (pdptval))

typedef struct {unsigned long pdt;} pdt_t;
#define mk_pdt(addr,attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_pdt(pdtptr,pdtval)		(*(pdtptr) = (pdtval))

typedef struct {unsigned long pt;} pt_t;
#define mk_pt(addr,attr)	((unsigned long)(addr) | (unsigned long)(attr))
#define set_pt(ptptr,ptval)		(*(ptptr) = (ptval))

//物理地址空间信息结构是一个20B的结构体,包含物理起始地址/空间长度/内存类型
struct E820//Global_Memory_Descriptor结构体的替代版本
{
	unsigned long address;//物理起始地址
	unsigned long length;//空间长度
	unsigned int	type;//内存类型(典型的内存类型是有效物理内存01h,其标识着当前地址空间为可用物理内存,如果平台的内存容量过大,BIOS可能会检测出多个可用物理内存段)
}__attribute__((packed));

struct Global_Memory_Descriptor//保存所有关于内存的信息以供内存管理模块使用
{
	struct E820 	e820[32];
	unsigned long 	e820_length;	
};

extern struct Global_Memory_Descriptor memory_management_struct;

void init_memory();

#endif