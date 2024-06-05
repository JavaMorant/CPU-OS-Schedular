#include "common.h"
#include "asm.h"
#include "bootloader_data.h"
#include "scheduler.h"

int interrupt_counter = 0;
asm_syscall_handler(asm_interrupt_syscall)(struct interrupt_frame *state)
{ 
    if (state->eflags & FLAG_INTERRUPTS_ON) {
        /* from userspace */
        state->rax = systemcall(state->rax, state->rbx, state->rcx, state->rdx);
    } else {
        interrupt_counter++;
        if (0) {
            print_interrupt_frame("Generic Interrupt", state);
        } else {
            printf("Interrupt %02d\n", interrupt_counter);
        }
    }
}

void shutdown()
{
        fb_setcol(0x00FF00);
        printf("\n#### All Processes Stopped ####\n");
        outw(0x604, 0x2000); /* thid only works on QEMU */
        halt_and_catch_fire();    
}

/********  LOADING  ***********/
void _start()
{
    /* setup apic and get id */
    u64 apic_id = init_apic();

    /* disable other cores */
    if (apic_id != bootloader_conf->bspid)
        halt_and_catch_fire();
    
    init_console();
    init_memory();
    init_interrupts();
    init_exceptions();
    init_apic_timer();
    set_irq_handler(0x80, asm_interrupt_syscall);
    init_scheduler();

    fb_setcol(PINK_COL);
    printf("========== Oink OS v1.1 ==========\n");
    fb_setcol(GRAY_COL);

    start_new_process("init", "init");
    start_multitasking();
}

