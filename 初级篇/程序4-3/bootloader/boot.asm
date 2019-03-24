;|----------------------|
;|	100000 ~ END	|
;|	   KERNEL	|
;|----------------------|
;|	E0000 ~ 100000	|
;| Extended System BIOS |
;|----------------------|
;|	C0000 ~ Dffff	|
;|     Expansion Area   |
;|----------------------|
;|	A0000 ~ bffff	|
;|   Legacy Video Area  |
;|----------------------|
;|	9f000 ~ A0000	|
;|	 BIOS reserve	|
;|----------------------|
;|	90000 ~ 9f000	|
;|	 kernel tmpbuf	|
;|----------------------|
;|	10000 ~ 90000	|
;|	   LOADER	|
;|----------------------|
;|	8000 ~ 10000	|
;|	  VBE info	|
;|----------------------|
;|	7e00 ~ 8000	|
;|	  mem info	|
;|----------------------|
;|	7c00 ~ 7e00	|
;|	 MBR (BOOT)	|
;|----------------------|
;|	0000 ~ 7c00	|
;|	 BIOS Code	|
;|----------------------|

	org	0x7c00	

BaseOfStack				equ	0x7c00
BaseOfLoader			equ	0x1000
OffsetOfLoader			equ	0x00
RootDirSectors			equ	14
SectorNumOfRootDirStart	equ	19
SectorNumOfFAT1Start	equ	1
SectorBalance			equ	17	

jmp	short Label_Start
nop
BS_OEMName		db	'MINEboot'
BPB_BytesPerSec	dw	512
BPB_SecPerClus	db	1
BPB_RsvdSecCnt	dw	1
BPB_NumFATs		db	2
BPB_RootEntCnt	dw	224
BPB_TotSec16	dw	2880
BPB_Media		db	0xf0
BPB_FATSz16		dw	9
BPB_SecPerTrk	dw	18
BPB_NumHeads	dw	2
BPB_HiddSec		dd	0
BPB_TotSec32	dd	0
BS_DrvNum		db	0
BS_Reserved1	db	0
BS_BootSig		db	0x29
BS_VolID		dd	0
BS_VolLab		db	'boot loader'
BS_FileSysType	db	'FAT12   '

Label_Start:
	mov	ax,	cs
	mov	ds,	ax
	mov	es,	ax
	mov	ss,	ax
	mov	sp,	BaseOfStack
	
;清屏INT 10h,AH=06h功能:按指定范围滚动窗口,具备清屏功能
	mov	ax,	0600h						;AL=滚动的行数(应该是行数),若为0则执行清屏功能(此时BX/CX/DX的参数不起作用)
	mov	bx,	0700h						;BH=颜色属性
	mov	cx,	0							;CH=左上角坐标列号CL=左上角坐标行号
	mov	dx,	0184fh						;DH=右下角坐标列号DL=右下角坐标行号
	int	10h

;设置光标INT 10h,AH=02h,功能:设定光标位置
	mov	ax,	0200h
	mov	bx,	0000h						;页码:BH=00h
	mov	dx,	0000h						;游标的列数:DH=00h,游标的行数:DL=00h
	int	10h

;在屏幕上显示Start Booting......;INT 10h,AH=13h功能:显示一行字符串
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

;复位磁盘INT 13h,AH=00h功能:重置磁盘驱动器,为下一次读写软盘做准备,DL=驱动器号,00H~7FH:软盘,80H~0FFH:硬盘
	xor	ah,	ah							;AH=00h功能:重置磁盘驱动器
	xor	dl,	dl							;DL=00H代表第一个软盘驱动器drive A:,01H代表第二个软盘驱动器drive B:
	int	13h								;DL=80H代表第一个硬盘驱动器,81H代表第二个硬盘驱动器
;上面的代码相当于重新初始化软盘驱动器,从而将软盘驱动器的磁头移动至默认位置

;;通过这段代码能从根目录中搜索出引导加载程序,文件名为loader.bin,在程序执行初期,程序会先保存根目录的起始扇区号,并根据目录占用磁盘扇区来确定需要搜索的扇区数,并从根目录中读入一个扇区的数据到缓冲区
	mov	word	[SectorNo],	SectorNumOfRootDirStart;SectorNumOfRootDirStart	equ	19	;根目录的起始扇区号
				;SectorNo		dw	0
Lable_Search_In_Root_Dir_Begin:
	cmp	word	[RootDirSizeForLoop],	0	;RootDirSizeForLoop	dw	RootDirSectors	;RootDirSectors	equ	14	;根目录占用的扇区数
	jz	Label_No_LoaderBin					;jz意思为ZF=1则转移
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
	jz	Label_Goto_Next_Sector_In_Root_Dir
	dec	dx
	mov	cx,	11							;文件名加后缀名共11字节

Label_Cmp_FileName:
	cmp	cx,	0
	jz	Label_FileName_Found
	dec	cx
	lodsb								;LODSB/LODSW是块读出指令,把SI指向的存储单元读入累加器,LODSB是读入AL,LODSW是读入AX,然后SI自动增加或减小1或2位
	cmp	al,	byte	[es:di]				;当方向标志位DF=0时,则SI自动增加,DF=1时,SI自动减小
	jz	Label_Go_On
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
	
;如果未找到loader程序则在屏幕上显示:ERROR:No LOADER Found
Label_No_LoaderBin:
	mov	ax,	1301h						;INT 10h,AH=13h功能:显示一行字符串,写入模式AL=01
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

;程序会先取得目录项DIR_FstClus字段的数值,并通过配置ES和BX来指定loader.bin程序在内存中的起始地址
Label_FileName_Found:
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

Label_Go_On_Loading_File:
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

;从磁盘读取一个扇区INT 13h,AH=02h功能:读取磁盘扇区,仅仅是对BIOS中断服务程序的再次封装以简化操作,AX=待读取的磁盘起始扇区号(LBA),CL=读入的扇区数量,ES:BX=>目标缓冲区起始地址
Func_ReadOneSector:
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

;根据当前FAT表项索引出下一个FAT表项,AX=FAT表项号(输入参数/输出参数)
Func_GetFATEntry:
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
	jz	Label_Even
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

;临时变量
RootDirSizeForLoop	dw	RootDirSectors	;RootDirSectors			equ	14
SectorNo			dw	0				;扇区号
Odd					db	0				;奇偶变量

;存放要在屏幕上显示的信息
StartBootMessage:	db	"Start Boot"
NoLoaderMessage:	db	"ERROR:No LOADER Found"
LoaderFileName:		db	"LOADER  BIN",0

;填充主引导扇区以55AA结束
	times	510 - ($ - $$)	db	0
	dw	0xaa55
;由于Intel处理器是以小端模式存储数据的,高字节在高地址,低字节在低地址,那么用一个字表示0x55和0xaa就应该是0xaa55,这样它在扇区里的存储顺序才是0x55,0xaa,引导扇区以0x55,0xaa结尾
;特别注意:特别注意:特别注意:用户数据区的的第一个簇的序号是002,第二个簇的序号是003,虚拟软盘(从0开始)33扇区开始是用户数据区的第一个簇,由于loader.bin的文件名是小写
;且系统不知道为什么将loader.bin文件存放在FAT[3]所指向的那个簇,也就是用户区的第二个簇,也就是虚拟软盘34扇区开始,而且34扇区前面还有loader.bin的小写文件名???