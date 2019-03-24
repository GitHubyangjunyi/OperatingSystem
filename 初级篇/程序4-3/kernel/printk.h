#ifndef __PRINTK_H__
#define __PRINTK_H__

#include <stdarg.h>
#include "font.h"

//都是2的自然数次方,形成互不干扰的bit位域0101 0101
#define ZEROPAD	1		//显示的字符前面填充0取代空格,对于所有的数字格式用前导零而不是空格填充字段宽度,如果出现-标志或者指定了精度(对于整数)则忽略该标志
#define SIGN	2		//无符号还是有符号long,这里的有符号应该是要不要打印+号
#define PLUS	4		//有符号的值若为正则显示带加号的符号,若为负则显示带减号的符号
#define SPACE	8		//有符号的值若为正则显示时带前导空格但是不显示符号,若为负则带减号符号,+标志会覆盖空格标志
#define LEFT	16		//对齐后字符放在左边
#define SPECIAL	32		//0x
#define SMALL	64		//小写

#define WHITE 	0x00ffffff		//白
#define BLACK 	0x00000000		//黑
#define RED		0x00ff0000		//红
#define ORANGE	0x00ff8000		//橙
#define YELLOW	0x00ffff00		//黄
#define GREEN	0x0000ff00		//绿
#define BLUE	0x000000ff		//蓝
#define INDIGO	0x0000ffff		//靛
#define PURPLE	0x008000ff		//紫

#define is_digit(c)	((c) >= '0' && (c) <= '9')
#define do_div(num,base) ({ \
int __res; \
__asm__("divq %%rcx":"=a" (num),"=d" (__res):"0" (num),"1" (0),"c" (base)); \
__res; })
//div将整数值num除以进制规格base,被除数由RDX:RAX组成,由于num变量是个8B的长整型变量,余数部分__res即是digits数组的下标索引值,整个表达式最终值等于__res
//寄存器约束=a令RAX=n,寄存器约束=d令rcx=__res,通用约束1令RDX=0,寄存器约束c令rcx=base

extern unsigned char font_ascii[256][16];
char buf[4096]={0};

struct position
{
	int XResolution;//当前屏幕分辨率
	int YResolution;

	int XPosition;	//光标所在列
	int YPosition;	//光标所在行

	int XCharSize;	//字符像素矩阵尺寸
	int YCharSize;

	unsigned int * FB_addr;//帧缓存区起始地址
	unsigned long FB_length;//帧缓存区容量大小
}Pos;//全局屏幕信息的结构体

void putchar(unsigned int * fb,int Xsize,int x,int y,unsigned int FRcolor,unsigned int BKcolor,unsigned char font);
int skip_atoi(const char **s);
static char * number(char *str, long num, int base, int field_width, int precision ,int flags);
int vsprintf(char * buf,const char *fmt, va_list args);
int color_printk(unsigned int FRcolor,unsigned int BKcolor,const char *fmt,...);

#endif