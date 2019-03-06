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

#include <stdarg.h>
#include "printk.h"
#include "lib.h"
#include "linkage.h"

void putchar(unsigned int * fb,int Xsize,int x,int y,unsigned int FRcolor,unsigned int BKcolor,unsigned char font)
{
	int i = 0,j = 0;
	unsigned int * addr = NULL;
	unsigned char * fontp = NULL;
	int testval = 0;
	fontp = font_ascii[font];

	for(i = 0; i< 16;i++)
	{//这段程序使用到了帧缓存区首地址,将该地址加上字符首像素位置(首像素位置是指字符像素矩阵左上角第一个像素点)的偏移( Xsize * ( y + i ) + x ),可得到待显示字符矩阵的起始线性地址
		addr = fb + Xsize * ( y + i ) + x;
		testval = 0x100;
		for(j = 0;j < 8;j ++)//for循环从字符首像素地址开始,将字体颜色和背景色的数值按字符位图的描绘填充到相应的线性地址空间中
		{
			testval = testval >> 1;
			if(*fontp & testval)
				*addr = FRcolor;
			else
				*addr = BKcolor;
			addr++;
		}
		fontp++;		
	}
}



int skip_atoi(const char **s)//(只能)负责将数值字母转换成整数值
{
	int i=0;

	while (is_digit(**s))//先判断是否是数值字母,宏定义#define is_digit(c)	((c) >= '0' && (c) <= '9')
		i = i*10 + *((*s)++) - '0';//如果是数值字母,则将当前字符转换成数值( *((*s)++) - '0' ),并拼入已转换的整数值( i*10 + *((*s)++) - '0' )
	return i;
}


static char * number(char * str, long num, int base, int size, int precision,	int type)//用于将长整型变量值转换成指定进制规格的字符串,参数base指定进制数,参数precision提供显示精度值
{//不管number函数将整数值转换成大写还是小写字母,它最高支持36进制的数值转换
	char c,sign,tmp[50];
	const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int i;

	if (type&SMALL) digits = "0123456789abcdefghijklmnopqrstuvwxyz";
	if (type&LEFT) type &= ~ZEROPAD;
	if (base < 2 || base > 36)
		return 0;
	c = (type & ZEROPAD) ? '0' : ' ' ;
	sign = 0;
	if (type&SIGN && num < 0) {
		sign='-';
		num = -num;
	} else
		sign=(type & PLUS) ? '+' : ((type & SPACE) ? ' ' : 0);
	if (sign) size--;
	if (type & SPECIAL)
		if (base == 16) size -= 2;
		else if (base == 8) size--;
	i = 0;
	if (num == 0)
		tmp[i++]='0';
	else while (num!=0)
		tmp[i++]=digits[do_div(num,base)];//负责将整数值转换成字符串(按数值倒序排列),然后再将tmp数组中的字符倒序插入到显示缓冲区
	if (i > precision) precision=i;
	size -= precision;
	if (!(type & (ZEROPAD + LEFT)))
		while(size-- > 0)
			*str++ = ' ';
	if (sign)
		*str++ = sign;
	if (type & SPECIAL)
		if (base == 8)
			*str++ = '0';
		else if (base==16) 
		{
			*str++ = '0';
			*str++ = digits[33];
		}
	if (!(type & LEFT))
		while(size-- > 0)
			*str++ = c;

	while(i < precision--)
		*str++ = '0';
	while(i-- > 0)
		*str++ = tmp[i];
	while(size-- > 0)
		*str++ = ' ';
	return str;
}


int vsprintf(char * buf,const char *fmt, va_list args)//用于解析color_printk函数所提供的格式化字符串及其参数,vsprintf函数会将格式化后的字符串结果保存到一个4096B的缓冲区中buf,并返回字符串长度
{
	char * str,*s;
	int flags;
	int field_width;
	int precision;
	int len,i;

	int qualifier;		/* 'h', 'l', 'L' or 'Z' for integer fields */
	//依然借助for循环完成格式化字符串的解析工作
	for(str = buf; *fmt; fmt++)
	{	//按照字符串规定,符号%后面可接 - + # 0 等格式符,如果下一个字符是上述格式符,则设置标志变量flags的标志位(标志位定义在printk.h),随后计算出数据区域的宽度
		//该循环体会逐个解析字符串,如果字符不为%就认为它是个可显示字符,直接将其存入缓冲区buf中,否则进一步解析其后的字符串格式
		if(*fmt != '%')
		{
			*str++ = *fmt;
			continue;
		}
		flags = 0;
		repeat:
			fmt++;
			switch(*fmt)
			{
				case '-':flags |= LEFT;	
				goto repeat;
				case '+':flags |= PLUS;	
				goto repeat;
				case ' ':flags |= SPACE;	
				goto repeat;
				case '#':flags |= SPECIAL;	
				goto repeat;
				case '0':flags |= ZEROPAD;	
				goto repeat;
			}

			/* get field width */
			//这部分程序可提取出后续字符串中的数字,并将其转化为数值以表示数据区域的宽度,如果下一个字符不是数字而是字符*,那么数据区域的宽度将由可变参数提供,根据可变参数值亦可判断数据区域的对齐显示方式(左/右对齐)
			field_width = -1;
			if(is_digit(*fmt))//#define is_digit(c)	((c) >= '0' && (c) <= '9')
				field_width = skip_atoi(&fmt);//int skip_atoi(const char **s)//(只能)负责将数值字母转换成整数值
			else if(*fmt == '*')
			{
				fmt++;
				field_width = va_arg(args, int);
				if(field_width < 0)
				{
					field_width = -field_width;
					flags |= LEFT;
				}
			}
			
			/* get the precision */
			//获取数据区宽度后,下一步提取出显示数据的精度,如果数据区域的宽度后面跟有字符.,说明其后的数值是显示数据的精度,这里采用与计算数据区域宽度相同的方法计算出显示数据的精度,随后还要获取显示数据的规格
			precision = -1;
			if(*fmt == '.')
			{
				fmt++;
				if(is_digit(*fmt))//#define is_digit(c)	((c) >= '0' && (c) <= '9')
					precision = skip_atoi(&fmt);//int skip_atoi(const char **s)//(只能)负责将数值字母转换成整数值
				else if(*fmt == '*')
				{	
					fmt++;
					precision = va_arg(args, int);
				}
				if(precision < 0)
					precision = 0;
			}
			//获取显示数据的规格,比如%ld格式化字符串中的字母l,就表示显示数据的规格是长整型
			qualifier = -1;
			if(*fmt == 'h' || *fmt == 'l' || *fmt == 'L' || *fmt == 'Z')
			{	
				qualifier = *fmt;
				fmt++;
			}
			//经过逐个格式符的解析,数据区域的宽度和精度等信息皆已获取,现在将遵照这些信息把可变参数格式化成字符串,并存入buf缓冲区,下面进入可变参数的字符串转化过程,目前支持的格式符有
			switch(*fmt)	//c s o p x X d i u n %等
			{
				case 'c':
			//如果匹配出格式符c,那么程序将可变参数转换为一个字符,并根据数据区域的宽度和对齐方式填充空格符,这就是%c格式符的功能,有了字符显示功能,字符串将很快实现
					if(!(flags & LEFT))
						while(--field_width > 0)
							*str++ = ' ';
					*str++ = (unsigned char)va_arg(args, int);
					while(--field_width > 0)
						*str++ = ' ';
					break;
			//整个显示过程会把字符串的长度与显示精度进行比对,根据数据区的宽度和精度等信息截取待显示字符串的长度并补齐空格符,涉及到内核通用库函数strlen
				case 's':
				
					s = va_arg(args,char *);
					if(!s)
						s = '\0';
					len = strlen(s);
					if(precision < 0)
						precision = len;
					else if(len > precision)
						len = precision;
					
					if(!(flags & LEFT))
						while(len < field_width--)
							*str++ = ' ';
					for(i = 0;i < len ;i++)
						*str++ = *s++;
					while(len < field_width--)
						*str++ = ' ';
					break;
			//下面是八进制/十进制/十六进制以及地址值的格式化显示功能,借助函数number实现可变参数的数字格式化功能,并根据各个格式符的功能置位相应标志位共number函数使用
				case 'o':
			//static char * number(char * str, long num, int base, int size, int precision,	int type)//用于将长整型变量值转换成指定进制规格的字符串,参数base指定进制数,参数precision提供显示精度值
					if(qualifier == 'l')
						str = number(str,va_arg(args,unsigned long),8,field_width,precision,flags);
					else
						str = number(str,va_arg(args,unsigned int),8,field_width,precision,flags);
					break;

				case 'p':

					if(field_width == -1)
					{
						field_width = 2 * sizeof(void *);
						flags |= ZEROPAD;
					}

					str = number(str,(unsigned long)va_arg(args,void *),16,field_width,precision,flags);
					break;

				case 'x':

					flags |= SMALL;

				case 'X':

					if(qualifier == 'l')
						str = number(str,va_arg(args,unsigned long),16,field_width,precision,flags);
					else
						str = number(str,va_arg(args,unsigned int),16,field_width,precision,flags);
					break;

				case 'd':
				case 'i':

					flags |= SIGN;
				case 'u':

					if(qualifier == 'l')
						str = number(str,va_arg(args,unsigned long),10,field_width,precision,flags);
					else
						str = number(str,va_arg(args,unsigned int),10,field_width,precision,flags);
					break;
			//函数的最后一部分代码负责格式化字符串的扫尾工作,格式符%n的功能是把目前已格式化的字符串长度返回给函数的调用者
				case 'n':
					
					if(qualifier == 'l')
					{
						long *ip = va_arg(args,long *);
						*ip = (str - buf);
					}
					else
					{
						int *ip = va_arg(args,int *);
						*ip = (str - buf);
					}
					break;
			//如果格式化字符串中出现字符%%,则把第一个格式符%视为转义符,经过格式化解析后,最终只显示一个字符%,如果在格式符解析过程中出现任何不支持的格式符,则不做任何处理,直接将其视为字符串输出到buf缓冲区
				case '%':
					
					*str++ = '%';
					break;

				default:

					*str++ = '%';	
					if(*fmt)
						*str++ = *fmt;
					else
						fmt--;
					break;
			}

	}
	*str = '\0';
	return str - buf;
}

int color_printk(unsigned int FRcolor,unsigned int BKcolor,const char * fmt,...)
{
	int i = 0;
	int count = 0;
	int line = 0;
	va_list args;//定义参数列表
	va_start(args, fmt);//使args指向第一个可变参数的地址
	i = vsprintf(buf,fmt, args);
	va_end(args);//关闭参数

	for(count = 0;count < i || line;count++)
	{
		////	add \n \b \t
		if(line > 0)
		{
			count--;
			goto Label_tab;
		}//通过for语句逐个字符检测格式化后的字符串
		if((unsigned char)*(buf + count) == '\n')//如果发现某个待显示字符是\n转移字符,则将光标行数加1,列数设置为0,否则判断待显示字符是否为\b转义符
		{
			Pos.YPosition++;//将光标行数加1
			Pos.XPosition = 0;//列数设置为0
		}
		else if((unsigned char)*(buf + count) == '\b')//如果确定待显示字符是\b转义字符,那么调整列位置并调用putchar函数打印空格符覆盖之前的字符,如果既不是\n也不是\b,则继续判断其是否为\t转义字符
		{
			Pos.XPosition--;
			if(Pos.XPosition < 0)
			{
				Pos.XPosition = (Pos.XResolution / Pos.XCharSize - 1) * Pos.XCharSize;
				Pos.YPosition--;
				if(Pos.YPosition < 0)
					Pos.YPosition = (Pos.YResolution / Pos.YCharSize - 1) * Pos.YCharSize;
			}	
			putchar(Pos.FB_addr , Pos.XResolution , Pos.XPosition * Pos.XCharSize , Pos.YPosition * Pos.YCharSize , FRcolor , BKcolor , ' ');	
		}
		else if((unsigned char)*(buf + count) == '\t')//如果确定待显示字符是\t转义字符,则计算当前光标距下一个制表位需要填充的空格符数量,将计算结果保存到局部变量line中
		{//再结合for循环和if判断,把显示位置调整到下一个制表位,并使用空格填补调整过程中占用的字符显示空间
			line = ((Pos.XPosition + 8) & ~(8 - 1)) - Pos.XPosition;//8表示一个制表位占用8个显示字符

Label_tab:
			line--;
			putchar(Pos.FB_addr , Pos.XResolution , Pos.XPosition * Pos.XCharSize , Pos.YPosition * Pos.YCharSize , FRcolor , BKcolor , ' ');	
			Pos.XPosition++;
		}
		else//排除待显示字符是\n\b\t转义字符之后,那么它就是一个普通字符
		{//使用putchar函数将字符打印在屏幕上,参数帧缓存线性地址/行分辨率/屏幕列像素点位置/屏幕行像素点位置/字体颜色/字体背景色/字符位图
			putchar(Pos.FB_addr , Pos.XResolution , Pos.XPosition * Pos.XCharSize , Pos.YPosition * Pos.YCharSize , FRcolor , BKcolor , (unsigned char)*(buf + count));
			Pos.XPosition++;
		}

		//字符显示结束还要为下次字符显示做准备,更新当前字符的显示位置,此处的字符显示位置可理解为光标位置,下面这段程序负责调整光标的列位置和行位置
		if(Pos.XPosition >= (Pos.XResolution / Pos.XCharSize))
		{
			Pos.YPosition++;
			Pos.XPosition = 0;
		}
		if(Pos.YPosition >= (Pos.YResolution / Pos.YCharSize))
		{
			Pos.YPosition = 0;
		}

	}
	return i;
}
