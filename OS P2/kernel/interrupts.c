#include "common.h"
#include "asm.h"
#include "registers.h"

void return_from_interrupt(struct interrupt_frame *to_state)
{
    if (to_state->eflags & FLAG_INTERRUPTS_ON) {
        /* returning to userspace, this is an oppertunity to change the task*/
        possibly_schedule();
    }

    /* - treat the interrupt_frame as the stack and pop from it 
       - the add $8 is needed to skip past errcode 
       - when iretq executes it has a stack like this (as required):
       [STACK TOP] -> rip  cs   eflags   rsp  ss */
    asm volatile("mov %0, %%rsp ; " POP_REGS " ; add $8, %%rsp ; iretq" :: "r"((u64)to_state));
}


/* ======== default interrupt handlers ========= */

/*
 * The "interrupt" table is made up of these entries.
 * Most of this is legacy garabage.
 *
 * The (virtual) address of handler is mangled accross the
 * offset fields
 *
 * segment is a GDT table index (see the GDT stuff below if you really want)
 * for our purposes it just tells the kernel to execute the handler at ring 0.
 */
struct idt_entry
{
    u16 offset1;
    u16 segment;
    
    u8 ist;
    
    u8 type : 4;
    u8 none : 1;
    u8 dpl : 2;
    u8 p : 1;

    u16 offset2;
    u32 offset3;
    u32 zero;
} _pack;

static struct idt_entry interrupt_table[256] = {0};

void set_irq_handler(int irq_number, void (*handler)(void))
{
    struct idt_entry *ent = &interrupt_table[irq_number];
    ent->p    = 1;                        /* present */
    ent->dpl  = 0x3;                      /* works in userspace */ 
    ent->type = irq_number < 32 ? 0xf : 0xe;     /* interrupt = 0xe, trap = 0xf ; 0xe disables interrupts on entry*/
    ent->ist = 0x0;                       /* don't switch stacks if already in kernel space (important!) */
    ent->segment = READ_REG(cs);          /* current code segment */
    
    u64 offset = (u64) handler;
    ent->offset1 = (offset >>  0) & 0xFFFF;
    ent->offset2 = (offset >> 16) & 0xFFFF;
    ent->offset3 = (offset >> 32) & 0xFFFFFFFF;
    ent->zero = 0;
}

void init_interrupts()
{
    struct {
        u16 size;
        struct idt_entry *idt;
    } _pack test;

    
    /* NOTE: we load a _virtual address_ here, its up to kernel
       to make sure this is always mapped */
    test.idt  = &interrupt_table[0];
    test.size = sizeof(interrupt_table) - 1;  /* :/ */
    
    asm volatile("lidt %0" :: "m"(test));
}


void init_userspace_frame(struct interrupt_frame *frame)
{
    /* avoid leaking anything to userspace in registers */
    memset(frame, 0x0, sizeof(struct interrupt_frame));
    
    frame->ss     = READ_REG(ss);
    frame->cs     = READ_REG(cs);
    
    /* force interrupts on */
    frame->eflags = READ_FLAGS() | FLAG_INTERRUPTS_ON;
}
