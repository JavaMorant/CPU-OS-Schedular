#include "common.h"

asm_exc_handler(asm_interrupt_gp)(struct interrupt_frame *state)
{
    print_interrupt_frame("General Protection Fault", state);
    crash();
}

asm_exc_handler(asm_interrupt_pf)(struct interrupt_frame *state)
{
    print_interrupt_frame("Page Fault", state);
    crash();
}

asm_exc_handler(asm_interrupt_io)(struct interrupt_frame *state)
{
    print_interrupt_frame("Invalid Opcode", state);
    crash();
}

asm_exc_handler(asm_interrupt_df)(struct interrupt_frame *state)
{
    print_interrupt_frame("Double Fault", state);
    crash();
}

asm_exc_handler(asm_interrupt_sf)(struct interrupt_frame *state)
{
    print_interrupt_frame("Stack Fault", state);
    crash();
}

asm_exc_handler(asm_interrupt_cf)(struct interrupt_frame *state)
{
    print_interrupt_frame("Control Flow Violation", state);
    crash();
}

void init_exceptions()
{
    set_irq_handler(0x06, asm_interrupt_io);
    set_irq_handler(0x08, asm_interrupt_df);
    set_irq_handler(0x0c, asm_interrupt_sf);
    set_irq_handler(0x0d, asm_interrupt_gp);
    set_irq_handler(0x0e, asm_interrupt_pf);
    set_irq_handler(21,   asm_interrupt_cf);
}
