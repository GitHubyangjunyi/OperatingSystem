#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__
#include "linkage.h"
#include "ptrace.h"

void init_interrupt();

void do_IRQ(struct pt_regs * regs,unsigned long nr);

#endif