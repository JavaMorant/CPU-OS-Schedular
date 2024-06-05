#pragma once

#define READ_REG(reg) ({ u64 ret; asm volatile("mov %%" #reg ", %0" : "=r"(ret)); ret;})
#define WRITE_REG(reg, val) ({ u64 __v = (val); asm volatile("mov %0, %%" #reg :: "r"(__v));})

#define READ_FLAGS() ({ u64 ret; asm volatile("pushfq \n pop %%rax" : "=a"(ret)); ret;})
#define WRITE_FLAGS(f) asm volatile("pushq %0 \n popfq" :: "r"(f))
#define FLAG_INTERRUPTS_ON 0x200

#define delay() for (u64 i = 0; i < (1UL << 28); i++) {}

/* --- cpuid --- */

struct cpuid {
    u64 eax, ebx, ecx, edx;
};

_inline struct cpuid cpuid(u32 code) {
    struct cpuid ret;
    asm volatile("cpuid":"=a"(ret.eax),"=b"(ret.ebx),"=c"(ret.ecx),"=d"(ret.edx):"a"(code));
    return ret;
}

/* --- msrs --- */

_inline u64 read_msr(u64 msr)
{
    u32 eax, edx;
    asm volatile("rdmsr" : "=a"(eax), "=d"(edx) : "c"(msr));
    return ((u64)edx << 32) | eax;
}
 
_inline void write_msr(u64 msr, u64 val)
{
    u32 eax = (val >>  0) & 0xFFFFFFFF;
    u32 edx = (val >> 32) & 0xFFFFFFFF;
    asm volatile("wrmsr" : : "a"(eax), "d"(edx), "c"(msr));
}

/* --- legacy io instructions --- */

_inline void outb(u16 port, u8 data)
{
    asm volatile("out %%al, %%dx" :: "d"(port), "a"(data));
}    

_inline int inb(u16 port)
{
    u64 ret;
    asm volatile("in %%dx, %%al" : "=a"(ret) : "d"(port));
    return ret & 0xFF;
}

_inline void outw(u16 port, u16 data)
{
    asm volatile("out %%ax, %%dx" :: "d"(port), "a"(data));
}    

_inline u16 inw(u16 port)
{
    u64 ret;
    asm volatile("in %%dx, %%ax" : "=a"(ret) : "d"(port));
    return ret & 0xFFFF;
}

_inline void outl(u16 port, u32 data)
{
    asm volatile("outl %%eax, %%edx" :: "d"(port), "a"(data));
}    

_inline u32 inl(u16 port)
{
    u64 ret;
    asm volatile("inl %%dx, %%eax" : "=a"(ret) : "d"(port));
    return ret & 0xFFFFFFFF;
}


/* --- halting --- */


/* perputually halt */
_inline void halt_and_catch_fire()
{
    while (1) {
        asm("cli ; hlt");
    }
}

/* get the current timestamp */
_inline u64 rdtsc()
{
    u32 lo, hi;
    asm("rdtsc" : "=d"(hi), "=a"(lo));
    return ((u64)hi << 32) | (u64)lo;
}

_inline void push_stack_64(u64** stack, u64 value)
{
    (*stack)--;
    **stack = value;
}

/* when you are writting a function in asm that should be called from C
   there is a set of registers that must be left as your found them. The
   below macros push them onto stack (presumably to be used at the start
   of your asm function) and pop them again (at the end) */

#define SYSV_SAVE_REGS                          \
    "pushq %rbx ;"                              \
    "pushq %rbp ;"                              \
    "pushq %r12 ;"                              \
    "pushq %r13 ;"                              \
    "pushq %r14 ;"                              \
    "pushq %r15 ;"

#define SYSV_RESTORE_REGS                      \
    "popq %r15 ;"                              \
    "popq %r14 ;"                              \
    "popq %r13 ;"                              \
    "popq %r12 ;"                              \
    "popq %rbp ;"                              \
    "popq %rbx ;"    

#define NUM_SYSV_SAVE_REGS 6

/*
 * When the CPU processes an interrupt (almost) any code could be running
 * so we must save and restore _ALL_ of the registers. So we push them onto
 * the stack (this time all of them) before running C code to process the
 * interrupt.
 *
 * This structure has a right size and members to match the state of the stack
 * after the pushing. Thus, we can treat the stack pointer as a pointer to the
 * this struct.
 *
 * This structure could be confusing for a couple of reasons:
 * a) the stack grows downwards so things are pushed from end-to-start
 * b) rip, cs, eflags, rsp, ss are pushed by _THE CPU ITSELF_, not us, when an
 *    interrupt happens
 * c) errocode is pushed by the CPU if the interrupt is an exception, but not
 *    a regular interrupt. In the second case we push a dummy value just to avoid
 *    having two different structures for these almost identical situations.
 */
struct interrupt_frame
{
    /* pushed by the below macros that wrap an interrupt handler written in C */
    u64 rbp, rdi, rsi, rax, rbx, rcx, rdx, r8, r9, r10, r11, r12, r13, r14, r15;
    
    /* pushed by hardware or irq handler */
    u64 errcode;
    
    /* below pushed by hardware */
    u64 rip, cs;
    u64 eflags;
    u64 rsp, ss;
} _pack;

/* must match the (reverse) order of members in struct interrupt_frame above */
#define PUSH_REGS \
    "pushq %%r15 ;"\
    "pushq %%r14 ;"\
    "pushq %%r13 ;"\
    "pushq %%r12 ;"\
    "pushq %%r11 ;"\
    "pushq %%r10 ;"\
    "pushq %%r9  ;"\
    "pushq %%r8  ;"\
    "pushq %%rdx ;"\
    "pushq %%rcx ;"\
    "pushq %%rbx ;"\
    "pushq %%rax ;"\
    "pushq %%rsi ;"\
    "pushq %%rdi ;"\
    "pushq %%rbp ;"

/* must be the reverse of the above */
#define POP_REGS \
    "popq %%rbp ;"\
    "popq %%rdi ;"\
    "popq %%rsi ;"\
    "popq %%rax ;"\
    "popq %%rbx ;"\
    "popq %%rcx ;"\
    "popq %%rdx ;"\
    "popq %%r8  ;"\
    "popq %%r9  ;"\
    "popq %%r10 ;"\
    "popq %%r11 ;"\
    "popq %%r12 ;"\
    "popq %%r13 ;"\
    "popq %%r14 ;"\
    "popq %%r15 ;"

/* see comment C in struct interrupt_frame */
#define PUSH_FAKE_ERR \
    "pushq $0xDEAD ;"

/*
 * An interrupt handler can't be a regular C function because
 * 1) the C function will overwrite the registers, 
 * 2) the C function will try to return with the "retq" instruction
 *    whereas an interrupt must be returned with "iretq" along with
 *    an appropriately stack frame (rip, cs, eflags, rsp, ss)
 *
 * These macros take a declaration like
 * asm_irq_handler(my_irq_handler)(struct interrupt_frame *state) {
 *     printf("do whatever");
 * }
 * 
 * and turns it into
 *
 * void _asmcall c_my_irq_handler(struct interrupt_frame *state) {
 *    printf("do whatever");
 * }
 *
 * and an asm function "my_irq_handler" that 
 * 1) pushes the registers
 * 2) calls the C function
 * and when (and if) the C function returns:
 * 3) for interrupts, call apic_end_of_interrupt which allows the apic
 *    to generate further interrupts
 * 4) calls return_from_interrupt with the interrupt_frame
 * Note: this means that your irq/exception handler can
 * modify the interrupt frame and the new registers will be used
 * when the irq handler returns
 *
 * See also return_from_interrupt in interrupts.c
 *
 * The only difference between irq an exc is that irq pushes the fake
 * errcode value (see struct interrupt_frame)
 */
#define asm_irq_handler(handler_func)                                   \
    void _asmcall c_ ##handler_func(struct interrupt_frame *state);     \
    void _naked handler_func() {                                        \
        asm(PUSH_FAKE_ERR PUSH_REGS                                     \
            "mov %%rsp, %%rbx;"                                         \
            "mov %%rbx, %%rdi;"                                         \
            "callq c_" #handler_func ";"                                \
            "callq apic_end_of_interrupt;"                              \
            "mov %%rbx, %%rdi;"                                         \
            "callq return_from_interrupt;"                              \
            :);                                                         \
    }                                                                   \
    void _asmcall c_ ## handler_func

#define asm_syscall_handler(handler_func)                               \
    void _asmcall c_ ##handler_func(struct interrupt_frame *state);     \
    void _naked handler_func() {                                        \
        asm(PUSH_FAKE_ERR PUSH_REGS                                     \
            "mov %%rsp, %%rbx;"                                         \
            "mov %%rbx, %%rdi;"                                         \
            "callq c_" #handler_func ";"                                \
            "mov %%rbx, %%rdi;"                                         \
            "callq return_from_interrupt;"                              \
            :);                                                         \
    }                                                                   \
    void _asmcall c_ ## handler_func


#define asm_exc_handler(handler_func)                                   \
    void _asmcall c_ ## handler_func(struct interrupt_frame *state);    \
    void _naked handler_func() {                                        \
        asm(PUSH_REGS                                                   \
            "mov %%rsp, %%rbx;"                                         \
            "mov %%rbx, %%rdi;"                                         \
            "callq c_" #handler_func ";"                                \
            "mov %%rbx, %%rdi;"                                         \
            "callq return_from_interrupt;"                              \
            :);                                                         \
    }                                                                   \
    void _asmcall c_ ## handler_func


