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

#ifndef __KEYBOARD_H__

#define __KEYBOARD_H__

extern struct keyboard_inputbuffer * p_kb;

#define NR_SCAN_CODES 	0x80
#define MAP_COLS	2

#define KB_BUF_SIZE	100

struct keyboard_inputbuffer
{
	unsigned char * p_head;
	unsigned char * p_tail;
	int count;
	unsigned char buf[KB_BUF_SIZE];
};

/*

*/

#define PAUSEBREAK	1
#define PRINTSCREEN	2
#define OTHERKEY	4
#define FLAG_BREAK	0x80

#define PORT_KB_DATA	0x60
#define PORT_KB_STATUS	0x64
#define PORT_KB_CMD	0x64

#define KBSTATUS_IBF	0x02
#define KBSTATUS_OBF	0x01

#define KBCMD_WRITE_CMD	0x60
#define KBCMD_READ_CMD	0x20

#define KB_INIT_MODE	0x47


/*

*/

unsigned char pausebreak_scode[]={0xE1,0x1D,0x45,0xE1,0x9D,0xC5};

unsigned int keycode_map_normal[NR_SCAN_CODES * MAP_COLS] = //
{
/*scan-code	unShift		Shift		*/
/*--------------------------------------------------------------*/
/*0x00*/	0,		0,
/*0x01*/	0,		0,		//ESC
/*0x02*/	'1',		'!',
/*0x03*/	'2',		'@',
/*0x04*/	'3',		'#',
/*0x05*/	'4',		'$',
/*0x06*/	'5',		'%',
/*0x07*/	'6',		'^',
/*0x08*/	'7',		'&',
/*0x09*/	'8',		'*',
/*0x0a*/	'9',		'(',
/*0x0b*/	'0',		')',
/*0x0c*/	'-',		'_',
/*0x0d*/	'=',		'+',
/*0x0e*/	0,		0,		//BACKSPACE	
/*0x0f*/	0,		0,		//TAB

/*0x10*/	'q',		'Q',
/*0x11*/	'w',		'W',
/*0x12*/	'e',		'E',
/*0x13*/	'r',		'R',
/*0x14*/	't',		'T',
/*0x15*/	'y',		'Y',
/*0x16*/	'u',		'U',
/*0x17*/	'i',		'I',
/*0x18*/	'o',		'O',
/*0x19*/	'p',		'P',
/*0x1a*/	'[',		'{',
/*0x1b*/	']',		'}',
/*0x1c*/	0,		0,		//ENTER
/*0x1d*/	0x1d,		0x1d,		//CTRL Left
/*0x1e*/	'a',		'A',
/*0x1f*/	's',		'S',

/*0x20*/	'd',		'D',
/*0x21*/	'f',		'F',
/*0x22*/	'g',		'G',
/*0x23*/	'h',		'H',
/*0x24*/	'j',		'J',
/*0x25*/	'k',		'K',
/*0x26*/	'l',		'L',
/*0x27*/	';',		':',
/*0x28*/	'\'',		'"',
/*0x29*/	'`',		'~',
/*0x2a*/	0x2a,		0x2a,		//SHIFT Left
/*0x2b*/	'\\',		'|',
/*0x2c*/	'z',		'Z',
/*0x2d*/	'x',		'X',
/*0x2e*/	'c',		'C',
/*0x2f*/	'v',		'V',

/*0x30*/	'b',		'B',
/*0x31*/	'n',		'N',
/*0x32*/	'm',		'M',
/*0x33*/	',',		'<',
/*0x34*/	'.',		'>',
/*0x35*/	'/',		'?',
/*0x36*/	0x36,		0x36,		//SHIFT Right
/*0x37*/	'*',		'*',
/*0x38*/	0x38,		0x38,		//ALT Left
/*0x39*/	' ',		' ',
/*0x3a*/	0,		0,		//CAPS LOCK
/*0x3b*/	0,		0,		//F1
/*0x3c*/	0,		0,		//F2
/*0x3d*/	0,		0,		//F3
/*0x3e*/	0,		0,		//F4
/*0x3f*/	0,		0,		//F5

/*0x40*/	0,		0,		//F6
/*0x41*/	0,		0,		//F7
/*0x42*/	0,		0,		//F8
/*0x43*/	0,		0,		//F9
/*0x44*/	0,		0,		//F10
/*0x45*/	0,		0,		//NUM LOCK
/*0x46*/	0,		0,		//SCROLL LOCK
/*0x47*/	'7',		0,		/*PAD HONE*/
/*0x48*/	'8',		0,		/*PAD UP*/
/*0x49*/	'9',		0,		/*PAD PAGEUP*/
/*0x4a*/	'-',		0,		/*PAD MINUS*/
/*0x4b*/	'4',		0,		/*PAD LEFT*/
/*0x4c*/	'5',		0,		/*PAD MID*/
/*0x4d*/	'6',		0,		/*PAD RIGHT*/
/*0x4e*/	'+',		0,		/*PAD PLUS*/
/*0x4f*/	'1',		0,		/*PAD END*/

/*0x50*/	'2',		0,		/*PAD DOWN*/
/*0x51*/	'3',		0,		/*PAD PAGEDOWN*/
/*0x52*/	'0',		0,		/*PAD INS*/
/*0x53*/	'.',		0,		/*PAD DOT*/
/*0x54*/	0,		0,
/*0x55*/	0,		0,
/*0x56*/	0,		0,
/*0x57*/	0,		0,		//F11
/*0x58*/	0,		0,		//F12
/*0x59*/	0,		0,		
/*0x5a*/	0,		0,
/*0x5b*/	0,		0,
/*0x5c*/	0,		0,
/*0x5d*/	0,		0,
/*0x5e*/	0,		0,
/*0x5f*/	0,		0,

/*0x60*/	0,		0,
/*0x61*/	0,		0,
/*0x62*/	0,		0,
/*0x63*/	0,		0,
/*0x64*/	0,		0,
/*0x65*/	0,		0,
/*0x66*/	0,		0,
/*0x67*/	0,		0,
/*0x68*/	0,		0,
/*0x69*/	0,		0,
/*0x6a*/	0,		0,
/*0x6b*/	0,		0,
/*0x6c*/	0,		0,
/*0x6d*/	0,		0,
/*0x6e*/	0,		0,
/*0x6f*/	0,		0,

/*0x70*/	0,		0,
/*0x71*/	0,		0,
/*0x72*/	0,		0,
/*0x73*/	0,		0,
/*0x74*/	0,		0,
/*0x75*/	0,		0,
/*0x76*/	0,		0,
/*0x77*/	0,		0,
/*0x78*/	0,		0,
/*0x79*/	0,		0,
/*0x7a*/	0,		0,
/*0x7b*/	0,		0,
/*0x7c*/	0,		0,
/*0x7d*/	0,		0,
/*0x7e*/	0,		0,
/*0x7f*/	0,		0,
};

/*

*/

#define  wait_KB_write()	while(io_in8(PORT_KB_STATUS) & KBSTATUS_IBF)
#define  wait_KB_read()		while(io_in8(PORT_KB_STATUS) & KBSTATUS_OBF)

/*

*/

void keyboard_init();
void keyboard_exit();

/*

*/

void analysis_keycode();

#endif
