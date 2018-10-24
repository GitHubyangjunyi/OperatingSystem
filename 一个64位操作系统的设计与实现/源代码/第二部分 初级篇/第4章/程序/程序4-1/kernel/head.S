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

.section .text

.globl _start

_start:;GAS编译器中,使用标识符_start作为程序的默认起始位置,同时还要使用伪指令.globl对_start标识符加以修饰避免出现警告
;令各个段寄存器加载数据段选择子0x10
	mov	$0x10,	%ax
	mov	%ax,	%ds
	mov	%ax,	%es
	mov	%ax,	%fs
	mov	%ax,	%ss
	mov	$0x7E00,	%esp

//=======	load GDTR

	lgdt	GDT_POINTER(%rip)	;采用RIP-Relative寻址模式,这是为IA-32e新引入的寻址方式

//=======	load	IDTR

	lidt	IDT_POINTER(%rip)

	mov	$0x10,	%ax
	mov	%ax,	%ds
	mov	%ax,	%es
	mov	%ax,	%fs
	mov	%ax,	%gs
	mov	%ax,	%ss

	movq	$0x7E00,	%rsp

//=======	load	cr3

	movq	$0x101000,	%rax
	movq	%rax,		%cr3
	movq	switch_seg(%rip),	%rax
	pushq	$0x08
	pushq	%rax
	lretq		;本段程序多次使用lretq代码来进行段间切换,未使用ljmp/lcall,因为GAS编译器不支持直接远跳转和直接远调用,一些指令在64位环境之下不可用,典型的有push cs/ds/es/ss和pop ds/es/ss
;因为上述原因,这段程序只能借助汇编指令lretq来进行段间跳转,此处先模仿远调用lcall的执行过程,伪造了程序的执行现场,并结合RIP-Relative寻址模式将段选择子和段内偏移地址保存到栈中,然后执行lretq恢复调用现场
//=======	64-bit mode code	;即返回到目标代码段的程序地址中,此处跳转到模块entry64的起始地址处,从而完成了从线性地址0x100000向地址0xFFFF 8000 0010 0000切换的工作
;通过这种方法,内核执行头程序最终跳转到内核主程序Start_Kernel函数中
switch_seg:
	.quad	entry64

entry64:
	movq	$0x10,	%rax
	movq	%rax,	%ds
	movq	%rax,	%es
	movq	%rax,	%gs
	movq	%rax,	%ss
	movq	$0xffff800000007E00,	%rsp		/* rsp address */

	movq	go_to_kernel(%rip),	%rax		/* movq address */
	pushq	$0x08
	pushq	%rax
	lretq

go_to_kernel:
	.quad	Start_Kernel

//=======	init page
.align 8	;借助伪指令.org来固定各个页表的起始地址,并使用伪指令.align将对齐标准设置为8B,以页目录(顶层页表)为例,使用.org	0x1000定位页目录后,此页表便位于内核执行头起始地址0x1000处
;然后链接器再根据链接脚本的描述,将内核执行头程序的起始线性地址设置在0xFFFF 8000 0000 0000 + 0x10 0000 处,因此推算出
.org	0x1000		;页目录的起始线性地址位于0xFFFF 8000 0010 0000 + 0x1000 = 0xFFFF 8000 0010 1000处
;此页表将线性地址0和0xFFFF 8000 0000 0000 映射为同一物理页以方便页表切换,即程序在配置页表前运行于线性地址0x10 0000附近,经跳转后运行于线性地址0xFFFF 8000 0000 0000附近
__PML4E:	;4KB,本系统采用规格为2MB的物理页,每个页面预设为相同属性值,用户访问模式/可读写/已存在,由于引导加载程序会将系统内核加载到1MB物理内存处,而页表PML4的起始地址位于内核程序1000h偏移处
;那么页表PML4的起始地址应该位于10 0000h + 1000h = 10 1000h处,这也是CR3加载的值
	.quad	0x102007		;0索引,索引出来后低12位补0
	.fill	255,8,0
	.quad	0x102007		;256索引,比如线性地址0xFFFF 8000 0010 0000(低21位是2MB物理页的页内偏移)映射到的物理地址是0x10 0000
	.fill	255,8,0			;FFFF高16位丢弃不进行索引,1000 0000 0(PML4E索引256) | 000 0000 00(PDPTE索引0) | 00 0000 000(PDE索引0) | 1 0000 0000 0000 0000 0000(物理页页内偏移)
;特别注意:本系统采用2MB物理页,三级页表,39~47位索引PML4,将索引出来的PDPT页表基地址低位补上12个0,30~38位索引PDPT,将索引出来的页目录基地址低位补上12个0
.org	0x2000	;21~29位索引PDT,0~20位为2MB物理页的页内偏移

__PDPTE:	;4KB
	
	.quad	0x103003		/* 0x103003 */
	.fill	511,8,0

.org	0x3000

__PDE:		;4KB

	.quad	0x000083		;0x00 0000
	.quad	0x200083		;0x20 0000
	.quad	0x400083		;0x40 0000
	.quad	0x600083		;0x60 0000
	.quad	0x800083		;0x80 0000
	.quad	0xe0000083		/*0x a00000*/
	.quad	0xe0200083
	.quad	0xe0400083
	.quad	0xe0600083		/*0x1000000*/;作者说注释放错行了,本来在下一行,修改到这一行
	.quad	0xe0800083
	.quad	0xe0a00083
	.quad	0xe0c00083
	.quad	0xe0e00083
	.fill	499,8,0
;init page程序段将前10MB物理内存分别映射到线性地址0处和0xFFFF 8000 0000 0000处,接着把物理地址0xE000 0000开始的16MB内存映射到线性地址0xA0 0000处和0xFFFF 8000 00A0 0000处,最后使用伪指令.fill将数值0填充到页表的剩余499个页表项里
//=======	GDT_Table

.section .data
;伪指令.globl修饰标识符GDT_Table/IDT_Table/TSS64_Table,以使这三个标识符可以被外部程序引用或访问,相当于C语言的extern关键字
.globl GDT_Table
;.globl可以保证在本程序正常配置描述符表项的同时,内核程序的其他部分也能够操作这些描述符表项,比较典型的场景有,向IDT表项设置中断/异常处理函数,保存/还原各个进程的任务状态段信息,创建LDT描述符表(本系统不使用LDT表功能)
GDT_Table:
	.quad	0x0000000000000000			/*0	NULL descriptor		       	00*/
	.quad	0x0020980000000000			/*1	KERNEL	Code	64-bit	Segment	08*/
	.quad	0x0000920000000000			/*2	KERNEL	Data	64-bit	Segment	10*/
	.quad	0x0020f80000000000			/*3	USER	Code	64-bit	Segment	18*/
	.quad	0x0000f20000000000			/*4	USER	Data	64-bit	Segment	20*/
	.quad	0x00cf9a000000ffff			/*5	KERNEL	Code	32-bit	Segment	28*/
	.quad	0x00cf92000000ffff			/*6	KERNEL	Data	32-bit	Segment	30*/
	.fill	10,8,0					/*8 ~ 9	TSS (jmp one segment <7>) in long-mode 128-bit 40*/
GDT_END:

GDT_POINTER:
GDT_LIMIT:	.word	GDT_END - GDT_Table - 1
GDT_BASE:	.quad	GDT_Table

//=======	IDT_Table

.globl IDT_Table

IDT_Table:
	.fill  512,8,0
IDT_END:

IDT_POINTER:
IDT_LIMIT:	.word	IDT_END - IDT_Table - 1
IDT_BASE:	.quad	IDT_Table

//=======	TSS64_Table

.globl	TSS64_Table

TSS64_Table:
	.fill  13,8,0
TSS64_END:

TSS64_POINTER:
TSS64_LIMIT:	.word	TSS64_END - TSS64_Table - 1
TSS64_BASE:	.quad	TSS64_Table

;以下是注释说明:
;在64位的IA-32e模式下,页表最高可以分为4个等级,而且分页机制除了提供4KB的物理页之外,还提供2MB和1GB的物理页
;对于拥有大量物理内存的操作系统来说,使用4KB物理页可能会导致页颗粒过于零碎,从而造成频繁的页维护工作,而采用2MB的物理页也许会比4KB更合理