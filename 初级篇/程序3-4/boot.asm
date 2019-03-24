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

	org	0x7c00							;org这条伪指令用于指定程序的起始地址,若程序未使用org伪指令,则编译器会把地址0x0000作为程序的起始地址,BIOS会加载引导程序至0x7c00处

BaseOfStack		equ	0x7c00
 
BaseOfLoader	equ	0x1000				;Loader程序的的起始物理地址0x10000
OffsetOfLoader	equ	0x00

RootDirSectors			equ	14			;根目录占用的扇区数=14
SectorNumOfRootDirStart	equ	19		  	;根目录的起始扇区号=19
SectorNumOfFAT1Start	equ	1		   	;FAT1表的起始扇区号=1
SectorBalance			equ	17			;用于平衡文件或者目录的起始簇号与数据区起始簇号的差值,通俗地说,因为数据区对应的有效簇号是2(FAT[2]),为了正确计算出FAT表项对应的数据区起始扇区号
;则必须将FAT表项减去2,或者将数据区的起始簇号/扇区号减去2(仅在每簇由一个扇区组成时可用),本程序采取的一种取巧的方法是将根目录起始扇区号减去2等于17,进而间接把数据区的起始扇区号减去2(数据区起始扇区号 = 根目录起始扇区号 + 根目录所占扇区数)
	jmp	short Label_Start				;生成两个字节的机器码
	nop									;nop生成一个字节的机器码
	BS_OEMName		db	'MINEboot'		;生产厂商名(长度必须为8字节)
	BPB_BytesPerSec	dw	512				;每扇区字节数=512(长度2字节),簇越大那么分区的容量也就越大,通过增加簇的扇区数,可以支持更大的磁盘分区,标准的簇大小为1、2、4、8、16、32、64和128,FAT12格式下只能管理2^12个簇(4096),所以在FAT12格式下能管理和分配的最大空间为4096*1*512=2097152B=2M,所以FAT12一般只适合3.5寸高密度软盘1.44M
	BPB_SecPerClus	db	1				;每簇扇区数=1,由于每个扇区的容量只有512字节,过小的扇区容量可能导致读写频繁而引入簇,簇将2的整数次方个扇区作为一个"原子"数据存储单位,每个簇的长度为BPB_SecPerClus*BPB_BytesPerSec=512
	BPB_RsvdSecCnt	dw	1				;保留扇区数=1,此阈值不能为0,保留扇区起始于FAT12文件系统的第一个扇区,对于FAT12而言此值必须为1,也就意味着引导扇区包含在保留扇区内,所以FAT表从软盘的第二个扇区开始
	BPB_NumFATs		db	2				;FAT表的份数=2,设置为2主要是为了给FAT表准备一个备份表,因此FAT表1和表2内的数据是一样的
	BPB_RootEntCnt	dw	224				;根目录可容纳的目录项数=224,对于FAT12文件系统而言,这个数值乘以32必须是BPB_BytesPerSec的偶数倍,224*32/512=14,能够储存在根目录下的最大文件(包含子目录)数量,默认为224,每个目录或文件名占用32B的空间,因此根目录的大小为224*32=7168B=7KB(14个扇区),如果使用长文件名的话,根目录文件数还可能无法达到224的数量
	BPB_TotSec16	dw	2880			;总扇区数=2880,如果此磁盘的逻辑扇区总数大于2^16位(65536)的话,就设置此字段为0,然后使用偏移32处的双字来表示逻辑总扇区数
	BPB_Media		db	0xf0			;介质描述符=0xF0,描述存储介质类型,使用0f0h表示3.5寸高密码软盘,用0f8h来表示硬盘,无论该字段写入了什么数值,都必须同时向FAT[0]的低字节写入相同的值
	BPB_FATSz16		dw	9				;每FAT扇区数=0009,记录着FAT表占用的扇区数,FAT表1和表2拥有相同的容量,它们的容量均由此值记录,操作系统用这个字段和FAT表数量以及隐藏扇区数量来计算根目录所在的扇区,还可以根据最大根目录数来计算用户数据区从哪里开始,根目录扇区位置=FAT表数量*FAT表所占用的扇区数量+隐藏扇区数量
	BPB_SecPerTrk	dw	18				;每磁道扇区数=18,软盘的默认值为18	用户数据开始位置=根目录扇区位置+根目录所占用扇区(FAT12格式下为224*32/512),此处所说的扇区指的是逻辑(线性)扇区,需要通过转换才能得到CHS磁盘参数,然后通过CHS参数来读写磁盘扇区
	BPB_NumHeads	dw	2				;磁头数=0002
	BPB_HiddSec		dd	0				;隐藏扇区数=00,指的在引导扇区之前的隐藏扇区,在FAT12格式上此字段默认为0,即不隐藏任何扇区,此字段参与计算根目录区和用户数据区位置
	BPB_TotSec32	dd	0				;如果BPB_ToSec16值为0,则由这个值记录扇区数
	BS_DrvNum		db	0				;int 13h使用的驱动器号=00,它与BIOS物理驱动器相关,在磁盘中断Int13h相关的操作中使用,第一个软盘驱动器设置为0,第一个硬盘驱动器设置为80h,第二个硬盘驱动器设置为81h,以此类推,此字段的值可以在系统引导时用dl寄存器得到
	BS_Reserved1	db	0				;未使用0
	BS_BootSig		db	0x29			;扩展引导标记0x29,操作系统用它来识别引导信息,值可以是28h或29h
	BS_VolID		dd	0				;卷序列号=0000 0000(长度4字节),在格式化磁盘时所产生的一个随机序号,有助于区分磁盘,可以为0
	BS_VolLab		db	'boot loader'	;卷标'boot loader'(长度11字节,不足必须以空格20h填充),就是Windows或Linux系统中显示的磁盘名,此字段只能使用一次,用来保存磁盘卷的标识符,再次设置的时候被保存到根目录中作为一个特殊的文件来储存
	BS_FileSysType	db	'FAT12   '		;文件系统类型FAT12(长度8字节,不足必须以空格20h来填充),只是一个字符串而已,操作系统并不使用该字段来鉴别FAT类文件系统的类型,采取什么样的文件系统只有一个计算标准,此卷所有簇的数量

Label_Start:

	mov	ax,	cs
	mov	ds,	ax
	mov	es,	ax
	mov	ss,	ax
	mov	sp,	BaseOfStack

;=======	clear screen
;INT 10h,AH=06h功能:按指定范围滚动窗口,具备清屏功能
	mov	ax,	0600h						;AL=滚动的行数(应该是行数),若为0则执行清屏功能(此时BX/CX/DX的参数不起作用)
	mov	bx,	0700h						;BH=颜色属性
	mov	cx,	0							;CH=左上角坐标列号CL=左上角坐标行号
	mov	dx,	0184fh						;DH=右下角坐标列号DL=右下角坐标行号
	int	10h

;=======	set focus
;INT 10h,AH=02h,功能:设定光标位置
	mov	ax,	0200h
	mov	bx,	0000h						;页码:BH=00h
	mov	dx,	0000h						;游标的列数:DH=00h,游标的行数:DL=00h
	int	10h

;=======	display on screen : Start Booting......
;INT 10h,AH=13h功能:显示一行字符串
	mov	ax,	1301h						;功能号AH=13h,写入模式AL=01
	mov	bx,	000fh						;字符/颜色属性BL=0000 1111(白色字体高亮,黑色背景不闪烁),页码BH=00
	mov	dx,	0000h						;游标行号DH游标列号DL
	mov	cx,	10							;字符串长度
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	StartBootMessage			;ES:BP=>要显示字符串的内存地址
	int	10h

;=======	reset floppy
;INT 13h,AH=00h功能:重置磁盘驱动器,为下一次读写软盘做准备,DL=驱动器号,00H~7FH:软盘,80H~0FFH:硬盘
	xor	ah,	ah							;AH=00h功能:重置磁盘驱动器
	xor	dl,	dl							;DL=00H代表第一个软盘驱动器drive A:,01H代表第二个软盘驱动器drive B:
	int	13h								;DL=80H代表第一个硬盘驱动器,81H代表第二个硬盘驱动器
;上面的代码相当于重新初始化软盘驱动器,从而将软盘驱动器的磁头移动至默认位置
;=======	search loader.bin
	mov	word	[SectorNo],	SectorNumOfRootDirStart		;SectorNumOfRootDirStart	equ	19	;根目录的起始扇区号
				;SectorNo		dw	0
Lable_Search_In_Root_Dir_Begin:
;通过这段代码能从根目录中搜索出引导加载程序,文件名为loader.bin,在程序执行初期,程序会先保存根目录的起始扇区号,并根据目录占用磁盘扇区来确定需要搜索的扇区数,并从根目录中读入一个扇区的数据到缓冲区
	cmp	word	[RootDirSizeForLoop],	0	;RootDirSizeForLoop	dw	RootDirSectors	;RootDirSectors	equ	14	;根目录占用的扇区数
	jz	Label_No_LoaderBin				;jz意思为ZF=1则转移
	dec	word	[RootDirSizeForLoop]	
	mov	ax,	00h	;接下来遍历读入缓冲区的每个目录项,寻找与目标文件名字符串相匹配的目录项,其中DX记录着每个扇区可容纳的目录项个数(512/32=16)
	mov	es,	ax	;CX记录着目录项的文件名长度(文件名长度为11字节,包括文件名和扩展名,但不包含分隔符'.'),在对比每个目录项文件名的过程中,使用了汇编指令LODSB,该命令的加载方向与DF标志位有关,因此在使用此命令时需用CLD指令清DF标志位
	mov	bx,	8000h	;一旦发现完全匹配的字符串,则跳转到Label_FileName_Found处执行,如果没有找到则执行其后的Label_No_LoaderBin模块
	mov	ax,	[SectorNo]					;SectorNo	dw	0,第93行将其改成0x13(19)了
	mov	cl,	1
	call	Func_ReadOneSector			;到这一步时,ax=19,cl=1,es=00,bx=8000h,这些将是Func_ReadOneSector的参数
	mov	si,	LoaderFileName				;LoaderFileName:	db	"LOADER  BIN",0
	mov	di,	8000h
	cld
	mov	dx,	10h							;每个扇区512字节,共16个目录项
	
Label_Search_For_LoaderBin:

	cmp	dx,	0
	jz	Label_Goto_Next_Sector_In_Root_Dir	;jz意思为ZF=1则转移
	dec	dx
	mov	cx,	11							;文件名加后缀名共11字节

Label_Cmp_FileName:

	cmp	cx,	0
	jz	Label_FileName_Found			;jz意思为ZF=1则转移
	dec	cx
	lodsb								;LODSB/LODSW是块读出指令,把SI指向的存储单元读入累加器,LODSB是读入AL,LODSW是读入AX,然后SI自动增加或减小1或2位
	cmp	al,	byte	[es:di]				;当方向标志位DF=0时,则SI自动增加,DF=1时,SI自动减小
	jz	Label_Go_On						;jz意思为ZF=1则转移
	jmp	Label_Different

Label_Go_On:
	
	inc	di
	jmp	Label_Cmp_FileName

Label_Different:

	and	di,	0ffe0h
	add	di,	20h
	mov	si,	LoaderFileName				;LoaderFileName:	db	"LOADER  BIN",0
	jmp	Label_Search_For_LoaderBin

Label_Goto_Next_Sector_In_Root_Dir:
	
	add	word	[SectorNo],	1
	jmp	Lable_Search_In_Root_Dir_Begin
	
;=======	display on screen : ERROR:No LOADER Found

Label_No_LoaderBin:
;INT 10h,AH=13h功能:显示一行字符串
	mov	ax,	1301h						;功能号AH=13h,写入模式AL=01
	mov	bx,	008ch						;字符/颜色属性BL=1000 1100(红色字体高亮,黑色背景闪烁),页码BH=00
	mov	dx,	0100h						;游标行号DH游标列号DL
	mov	cx,	21							;字符串长度
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	NoLoaderMessage				;ES:BP=>要显示字符串的内存地址
	int	10h
	jmp	$								;没找到loader程序直接死循环

;=======	found loader.bin name in root director struct

Label_FileName_Found:
;程序会先取得目录项DIR_FstClus字段的数值,并通过配置ES和BX来指定loader.bin程序在内存中的起始地址
	mov	ax,	RootDirSectors				;RootDirSectors	equ	14		;根目录占用的扇区数
	and	di,	0ffe0h	;再根据loader.bin程序的起始簇号计算出其对应的扇区号,为了增强人机交互效果,此处还使用BIOS中断服务INT 10h显示一个字符'.'
	add	di,	01ah	;接着每读入一个扇区的数据就通过Func_GetFATEntry模块取得下一个FAT表项,并跳转至Label_Go_On_Loading_File处继续读入下一个簇的数据
	mov	cx,	word	[es:di]
	push	cx		;如此往复直至Func_GetFATEntry模块返回的FAT表项值是0fffh为止,当loader.bin文件的数据全部读入到内存中后,跳转至Label_File_Loaded处准备执行loader.bin程序
	add	cx,	ax
	add	cx,	SectorBalance				;SectorBalance	equ	17
	mov	ax,	BaseOfLoader				;BaseOfLoader	equ	0x1000	;Loader程序的的起始物理地址0x10000
	mov	es,	ax
	mov	bx,	OffsetOfLoader				;OffsetOfLoader	equ	0x00
	mov	ax,	cx

Label_Go_On_Loading_File:;这段代码使用了INT 10h的主功能号AH=0Eh在屏幕上显示一个字符
	push	ax
	push	bx
	mov	ah,	0eh							;主功能号AH=0Eh
	mov	al,	'.'							;AL=待显示字符
	mov	bl,	0fh
	int	10h
	pop	bx
	pop	ax

	mov	cl,	1
	call	Func_ReadOneSector
	pop	ax
	call	Func_GetFATEntry
	cmp	ax,	0fffh						;文件的起始簇号在目录项偏移26处,到此已经读了起始簇号指向的一个扇区了,不存在少读一个扇区的问题
	jz	Label_File_Loaded				;jz意思为ZF=1则转移
	push	ax
	mov	dx,	RootDirSectors				;RootDirSectors	equ	14	;根目录占用的扇区数
	add	ax,	dx
	add	ax,	SectorBalance				;SectorBalance	equ	17
	add	bx,	[BPB_BytesPerSec]			;每扇区字节数=512
	jmp	Label_Go_On_Loading_File

Label_File_Loaded:
	
	jmp	BaseOfLoader:OffsetOfLoader

;=======	read one sector from floppy

Func_ReadOneSector:;INT 13h,AH=02h功能:读取磁盘扇区
;仅仅是对BIOS中断服务程序的再次封装以简化操作,AX=待读取的磁盘起始扇区号(LBA),CL=读入的扇区数量,ES:BX=>目标缓冲区起始地址
	push	bp
	mov	bp,	sp
	sub	esp,	2
	mov	byte	[bp - 2],	cl			;将要读入的扇区数量cl压栈保存
	push	bx							;将要BX=>目标缓冲区起始地址压栈保存
	mov	bl,	[BPB_SecPerTrk]				;BPB_SecPerTrk每磁道扇区数=18
	div	bl
	inc	ah								;INT 13h,AH=02h功能:读取磁盘扇区,下面是参数
	mov	cl,	ah							;CL=扇区号1~63(bit 0~5),磁道号的高2位(bit 6~7,只对硬盘有效)
	mov	dh,	al
	shr	al,	1							;shr逻辑右移指令
	mov	ch,	al							;CH=磁道号的低8位
	and	dh,	1							;DH=磁头号,之前传入dh的值是LBA除以每磁道扇区数的商,这里是要清掉高7位留下第0位,而且刚好磁头数是2,如果第0位是1,就用1磁头,是0就用0磁头

	pop	bx								;ES:BX=>数据缓冲区
	mov	dl,	[BS_DrvNum]					;DL=驱动器号(如果操作的是硬盘驱动器,bit 7必须被置位),BS_DrvNum	db	0
Label_Go_On_Reading:
	mov	ah,	2							;AH=02h功能
	mov	al,	byte	[bp - 2]			;AL=读入的扇区数(必须非0)
	int	13h
	jc	Label_Go_On_Reading				;jc意思是CF=1则转移,CF进位标志位,软盘有时忙,使用jc重复操作,出口参数CF＝0表示操作成功
	add	esp,	2
	pop	bp
	ret

;=======	get FAT Entry

Func_GetFATEntry:
;功能:根据当前FAT表项索引出下一个FAT表项,AX=FAT表项号(输入参数/输出参数)
	push	es	;这段程序首先会保存FAT表项号,并将奇偶标志变量(odd)置0,因为每个FAT表项占用1.5B,所以将FAT表项乘以3除去2,来判断余数的奇偶性并保存在[odd]中
	push	bx	;再将计算结果除以每扇区字节数,商值为FAT表项的偏移扇区号,余数值为FAT表项在扇区中的偏移位置
	push	ax	;接着通过Func_ReadOneSector模块连续读入两个扇区的数据,目的是为了解决FAT表项横跨两个扇区的问题,最后根据奇偶标志变量进一步处理奇偶项错位问题,即奇数项向右移动4位
	mov	ax,	00	;有能力的读者可以替换成FAT16文件系统,简化FAT表项的索引过程,在完成Func_ReadOneSector和Func_GetFATEntry模块后就可以借助这两个模块把loader.bin文件内的数据从软盘扇区读取到指定地址
	mov	es,	ax
	pop	ax
	mov	byte	[Odd],	0				;Odd	db	0
	mov	bx,	3
	mul	bx								;将与ax中的数相乘得到32位数dx:ax
	mov	bx,	2
	div	bx								;dx:ax除以bx,dx余数,ax商
	cmp	dx,	0
	jz	Label_Even						;jz意思为ZF=1则转移
	mov	byte	[Odd],	1

Label_Even:

	xor	dx,	dx
	mov	bx,	[BPB_BytesPerSec]
	div	bx
	push	dx
	mov	bx,	8000h
	add	ax,	SectorNumOfFAT1Start		;SectorNumOfFAT1Start	equ	1	;FAT1表的起始扇区号
	mov	cl,	2
	call	Func_ReadOneSector
	
	pop	dx
	add	bx,	dx							;此时的dx为FAT表项在扇区中的偏移位置,注意,Intel处理器是小端字节序,所以下一条指令读出的正好是将错位还原好的FAT表项的两个字节
	mov	ax,	[es:bx]						;ax16位,所以读入的FAT表项是有16位,而每个FAT12表项只有12位
	cmp	byte	[Odd],	1
	jnz	Label_Even_2					;jnz意思是ZF=0则转移
	shr	ax,	4							;shr逻辑右移指令(奇数项),也就是刚才读出的两个字节,如果是奇数项,则表示低4位是下一个FAT表项的值

Label_Even_2:
	and	ax,	0fffh						;偶数项,则保留高4位
	pop	bx
	pop	es
	ret

;=======	tmp variable

RootDirSizeForLoop	dw	RootDirSectors	;RootDirSectors	equ	14	;根目录占用的扇区数
SectorNo			dw	0
Odd					db	0

;=======	display messages

StartBootMessage:	db	"Start Boot"
NoLoaderMessage:	db	"ERROR:No LOADER Found"
LoaderFileName:		db	"LOADER  BIN",0
;特别注意:特别注意:特别注意:用户数据区的的第一个簇的序号是002,第二个簇的序号是003,虚拟软盘(从0开始)33扇区开始是用户数据区的第一个簇,由于loader.bin的文件名是小写,且系统不知道为什么将loader.bin文件存放在FAT[3]所指向的那个簇
;=======	fill zero until whole sector	;也就是用户区的第二个簇,也就是虚拟软盘34扇区开始,而且34扇区前面还有loader.bin的小写文件名???

	times	510 - ($ - $$)	db	0
							dw	0xaa55
;由于Intel处理器是以小端模式存储数据的,高字节在高地址,低字节在低地址,那么用一个字表示0x55和0xaa就应该是0xaa55,这样它在扇区里的存储顺序才是0x55,0xaa,引导扇区以0x55,0xaa结尾
