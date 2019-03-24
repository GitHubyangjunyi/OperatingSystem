CFLAGS := -mcmodel=large -fno-builtin -m64
ASFLAGS := --64

all: system
	objcopy -I elf64-x86-64 -S -R ".eh_frame" -R ".comment" -O binary system kernel.bin

system:	head.o main.o printk.o
	ld -b elf64-x86-64 -z muldefs -o system head.o main.o printk.o -T Kernel.lds 

head.o:	head.S
	gcc -E  head.S > head.s
	as $(ASFLAGS) -o head.o head.s
	
main.o:	main.c
	gcc $(CFLAGS) -c main.c
	
printk.o: printk.c
	gcc $(CFLAGS) -c printk.c

clean:
	rm -rf *.o *.s~ *.s *.S~ *.c~ *.h~ system  Makefile~ Kernel.lds~ kernel.bin 