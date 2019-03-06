#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "printk.h"
#include "lib.h"

//物理地址空间信息结构体,使用该结构体格式化物理地址7E00h处的数据
struct Memory_E820_Formate
{
	unsigned int	address1;
	unsigned int	address2;
	unsigned int	length1;
	unsigned int	length2;
	unsigned int	type;
};

void init_memory();

#endif
