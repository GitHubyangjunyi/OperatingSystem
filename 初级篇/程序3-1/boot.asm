;/***************************************************
;		版权声明
;
;	本操作系统名为：MINE
;	该操作系统未经授权不得以盈利或非盈利为目的进行开发，
;	只允许个人学习以及公开交流使用
;
;	代码最终所有权及解释权归田宇所有；
;
;	本模块作者：	田宇
;	EMail:		345538255@qq.com
;
;
;***************************************************/

	org	0x7c00				;伪指令org用于指定程序的起始地址,如果未使用org编译器将会把地址0x0000作为程序的起始地址

BaseOfStack	equ	0x7c00

Label_Start:

	mov	ax,	cs
	mov	ds,	ax
	mov	es,	ax
	mov	ss,	ax
	mov	sp,	BaseOfStack

;=======	clear screen
;INT 10h,AH=06h功能:按指定范围滚动窗口,具备清屏功能
	mov	ax,	0600h			;AL=滚动的行数(应该是行数),若为0则执行清屏功能(此时BX/CX/DX的参数不起作用)
	mov	bx,	0700h			;BH=颜色属性
	mov	cx,	0				;CH=左上角坐标列号CL=左上角坐标行号
	mov	dx,	0184fh			;DH=右下角坐标列号DL=右下角坐标行号
	int	10h

;=======	set focus
;INT 10h,AH=02h,功能:设定光标位置
	mov	ax,	0200h
	mov	bx,	0000h			;页码:BH=00h
	mov	dx,	0000h			;游标的列数:DH=00h,游标的行数:DL=00h
	int	10h

;=======	display on screen : Start Booting......
;INT 10h,AH=13h功能:显示一行字符串
	mov	ax,	1301h
	mov	bx,	000fh			;字符/颜色属性BL=0000 1111(白色字体高亮,黑色背景不闪烁),页码BH=00
	mov	dx,	0000h			;游标行号DH游标列号DL
	mov	cx,	10				;字符串长度
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	StartBootMessage;ES:BP=>要显示字符串的内存地址
	int	10h

;=======	reset floppy
;INT 13h,AH=00h功能:重置磁盘驱动器,为下一次读写软盘做准备,DL=驱动器号,00H~7FH:软盘,80H~0FFH:硬盘
	xor	ah,	ah				;AH=00h功能:重置磁盘驱动器
	xor	dl,	dl				;DL=00H代表第一个软盘驱动器drive A:,01H代表第二个软盘驱动器drive B:
	int	13h					;DL=80H代表第一个硬盘驱动器,81H代表第二个硬盘驱动器
;上面的代码相当于重新初始化软盘驱动器,从而将软盘驱动器的磁头移动至默认位置
	jmp	$					;无限循环			

StartBootMessage:	db	"Start Boot"

;=======	fill zero until whole sector

	times	510 - ($ - $$)	db	0
	dw	0xaa55
;由于Intel处理器是以小端模式存储数据的,高字节在高地址,低字节在低地址,那么用一个字表示0x55和0xaa就应该是0xaa55,这样它在扇区里的存储顺序才是0x55,0xaa,引导扇区以0x55,0xaa结尾
