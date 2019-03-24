#ifndef __LIB_H__
#define __LIB_H__

#define NULL 0

inline int strlen(char *String)//获取字符串长度
{//repne不相等则重复,重复前判断ecx是否为零,不为零则减1,scasb查询di中是否有al中的字符串结束字符0,如果有则退出循环,将ecx取反后减去1得到字符串长度
	register int __res;//输出约束:ecx剩下字符串的长度(不包括末尾0)
	__asm__	__volatile__	(	"cld	\n\t"
					"repne	\n\t"
					"scasb	\n\t"
					"notl	%0	\n\t"
					"decl	%0	\n\t"
					:"=c"(__res)
					:"D"(String),"a"(0),"0"(0xffffffff)
					:
				);//输入约束:寄存器约束D令String的首地址约束在edi,立即数约束令al=0,通用约束0令ecx=0xffff ffff
	return __res;//ecx=0xffff ffff,al=0,edi指向String首地址,方向从低到高
}

#endif