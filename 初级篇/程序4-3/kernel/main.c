#include "printk.h"

void Start_Kernel(void)
{
	int *addr = (int *)0xffff800000a00000;
	int i;
//配置屏幕的分辨率/光标位置/字符矩阵的尺寸/帧缓冲区起始线性地址/缓冲区长度
	Pos.XResolution = 1440;//从左到右横向1440
	Pos.YResolution = 900;//从上到下纵向900

	Pos.XPosition = 0;//光标所在列
	Pos.YPosition = 0;//光标所在行

	Pos.XCharSize = 8;//从左到右横向8
	Pos.YCharSize = 16;//从上到下纵向16

	Pos.FB_addr = (int *)0xffff800000a00000;//帧缓冲区起始线性地址
	Pos.FB_length = (Pos.XResolution * Pos.YResolution * 4);//每个像素点需要4字节的值进行控制

	for(i = 0 ;i<1440*20;i++)
	{
		*((char *)addr+0)=(char)0x00;
		*((char *)addr+1)=(char)0x00;
		*((char *)addr+2)=(char)0xff;
		*((char *)addr+3)=(char)0x00;	
		addr +=1;	
	}
	for(i = 0 ;i<1440*20;i++)
	{
		*((char *)addr+0)=(char)0x00;
		*((char *)addr+1)=(char)0xff;
		*((char *)addr+2)=(char)0x00;
		*((char *)addr+3)=(char)0x00;	
		addr +=1;	
	}
	for(i = 0 ;i<1440*20;i++)
	{
		*((char *)addr+0)=(char)0xff;
		*((char *)addr+1)=(char)0x00;
		*((char *)addr+2)=(char)0x00;
		*((char *)addr+3)=(char)0x00;	
		addr +=1;	
	}
	for(i = 0 ;i<1440*20;i++)
	{
		*((char *)addr+0)=(char)0xff;
		*((char *)addr+1)=(char)0xff;
		*((char *)addr+2)=(char)0xff;
		*((char *)addr+3)=(char)0x00;	
		addr +=1;	
	}

	color_printk(YELLOW,BLACK,"Hello\t\t World!\n");

	while(1)
		;
}