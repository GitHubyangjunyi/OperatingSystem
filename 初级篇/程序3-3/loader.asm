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

org	10000h

	mov	ax,	cs
	mov	ds,	ax
	mov	es,	ax
	mov	ax,	0x00
	mov	ss,	ax
	mov	sp,	0x7c00

;=======	display on screen : Start Loader......

	mov	ax,	1301h						;功能号AH=13h,写入模式AL=01
	mov	bx,	000fh						;字符/颜色属性BL=0000 1111(白色字体高亮,黑色背景不闪烁),页码BH=00
	mov	dx,	0200h						;行号DH列号DL
	mov	cx,	12							;字符串长度
	push	ax
	mov	ax,	ds
	mov	es,	ax
	pop	ax
	mov	bp,	StartLoaderMessage			;ES:BP=>要显示字符串的内存地址
	int	10h

	jmp	$

;=======	display messages

StartLoaderMessage:	db	"Start Loader"
