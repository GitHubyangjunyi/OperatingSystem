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

#ifndef __PRINTK_H__
#define __PRINTK_H__

#include <stdarg.h>
#include "font.h"
#include "linkage.h"

#define ZEROPAD	1		/* pad with zero */
#define SIGN	2		/* unsigned/signed long */
#define PLUS	4		/* show plus */
#define SPACE	8		/* space if plus */
#define LEFT	16		/* left justified */
#define SPECIAL	32		/* 0x */
#define SMALL	64		/* use 'abcdef' instead of 'ABCDEF' */

#define is_digit(c)	((c) >= '0' && (c) <= '9')

#define WHITE 	0x00ffffff		//白
#define BLACK 	0x00000000		//黑
#define RED	0x00ff0000		//红
#define ORANGE	0x00ff8000		//橙
#define YELLOW	0x00ffff00		//黄
#define GREEN	0x0000ff00		//绿
#define BLUE	0x000000ff		//蓝
#define INDIGO	0x0000ffff		//靛
#define PURPLE	0x008000ff		//紫

/*

*/

extern unsigned char font_ascii[256][16];

char buf[4096]={0};

struct position		//用于屏幕信息的结构体
{
	int XResolution;//当前屏幕分辨率
	int YResolution;

	int XPosition;	//字符光标所在位置
	int YPosition;

	int XCharSize;	//字符像素矩阵尺寸
	int YCharSize;

	unsigned int * FB_addr;//帧缓存区起始地址
	unsigned long FB_length;//帧缓存区容量大小
}Pos;


/*

*/

void putchar(unsigned int * fb,int Xsize,int x,int y,unsigned int FRcolor,unsigned int BKcolor,unsigned char font);

/*

*/

int skip_atoi(const char **s);

/*

*/

#define do_div(n,base) ({ \
int __res; \
__asm__("divq %%rcx":"=a" (n),"=d" (__res):"0" (n),"1" (0),"c" (base)); \
__res; })

/*
	do_div宏是一条内嵌汇编语句,借助div指令将整数值num除以进制规格base,在div汇编指令中,被除数由RDX:RAX组成,由于num变量是个8B的长整型变量,因此RDX被赋值为0,计算结果的余数部分即是digits数组的下标索引值
	特别注意的是,如果将这行汇编语句改成__asm__("divq %4":"=a" (n),"=d" (__res):"0" (n),"1" (0),"c" (base));,在理论上是可行的,但在编译过程中会提示错误Error:Incorrect register '%ecx' used with 'q' suffix
	可见编译器为寄存器约束符选择32位寄存器而非64位
*/

static char * number(char * str, long num, int base, int size, int precision ,int type);

/*

*/

int vsprintf(char * buf,const char *fmt, va_list args);

/*

*/

int color_printk(unsigned int FRcolor,unsigned int BKcolor,const char * fmt,...);

#endif

