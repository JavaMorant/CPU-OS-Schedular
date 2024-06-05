#pragma once

/* we don't assume stdint.h exists */
typedef unsigned char       u8;
typedef signed   char       s8;
typedef unsigned short int  u16;
typedef signed   short int  s16;
typedef unsigned int        u32;
typedef signed   int        s32;
typedef unsigned long int   u64;
typedef signed   long int   s64;
typedef u64 size_t;

typedef unsigned char       uint8_t;
typedef signed   char       int8_t;
typedef unsigned short int  uint16_t;
typedef signed   short int  int16_t;
typedef unsigned int        uint32_t;
typedef signed   int        int32_t;
typedef unsigned long int   uint64_t;
typedef signed   long int   int64_t;


/* force the compiler to put struct fields in the order you said,
   with no padding */
#define _pack     __attribute__((packed))
/* When asm calls C it need to know where to put the arguments,
   this fixes the ABI for a function to sysv to allow this. */
#define _asmcall  __attribute__((sysv_abi))
/* A raw function that doesn't touch the stack or registers,
   you have to do everything yourself in asm blocks inside the function*/
#define _naked    __attribute__((naked))
/* More than anything, this is a good bit of documentation */ 
#define _noreturn __attribute__((noreturn))
/* Mostly to avoid linking headaches when the compiler ignores
   the inline directive.  */
#define _inline   inline __attribute__((always_inline))

#define NULL ((void*)0UL)

#include "asm.h"

void shutdown();



/* ========== Common data structures  ========== */

struct thread;
struct task;

struct waitqueue {
    int signalled;
    struct thread *proc_list;
};

#include "scheduler.h"

#define THREAD_MAGIC 0xabcdef01BAADBEEFUL
#define DEAD_THREAD_MAGIC 0xabcdef02BAADBEEFUL

/* task for a kernel thread that can be switched to
    any scheduler-related data should go in "struct task"
    in scheduler.c

*/
struct thread {
    u64 magic;
    u64 pid;
    char name[128]; // executable name
    char arg[128];  // argument
    struct thread *nxt, *prev;
    int on_runqueue;

    u64 wake_time;
    u64 last_start_time, last_wake_time;

    /* saved stack pointer, all the registers are stashed on here */
    u64  *stack_ptr;

    /* return value of create_task() */
    struct sched_task sched_handle;
    
    /* all the tasks that are waiting on us */
    struct waitqueue death_waiters;

    /*  pointer to pushed registers from the last syscall/interrupt,
        which will be on the stack */
    struct interrupt_frame *frame;

    /* loaded elf file */
    void *memory_space; 

    /* used differently depending on what list this thread is on */
    union {
        struct {
            struct thread *next;
        } waitqueue_data;

        struct {
            struct thread *next;
            u64 wake_time;
        } sleep_data;
    };
};

struct file_data
{
    u8 *data;
    u64 size;
};

/* ========== Printing functions, see console.c ========== */

void init_console();

int putchar(char c);

void puts(char *s);

int printf(const char* format, ...);

void printword(char *name, u64 word);

void printbuf(char *name, void *ptr, u64 size);

void print_interrupt_frame(char *title, struct interrupt_frame *frame);

int parse_number(const char *buf, u64 *ret, int base);

#define PINK_COL 0xff54d2
#define RED_COL  0xFF0000
#define GRAY_COL 0xbbbbbb
void fb_setcol(u32 col);

/* ========== some standard c functions, see string.h ========== */

unsigned long strlen (const char*str);

int strcmp(const char *str1, const char *str2);

void memset(void *ptr, u8 val, u64 size);

void memcpy(void *dest, void *src, u64 size);

/* ========== LAPIC Functions, see apic.c ========== */

/* returns the apic id of the current processor */
u64 init_apic();

void init_apic_timer();

/* in us */
u64 current_time();

/* time is in miros offset from current_time */
void schedule_apic_timer_interrupt(u64 time);

/* called to complete an interrupt originating from the APIC */
void apic_end_of_interrupt();

/* ========== Interrupt Functions, see interrupts.c ========== */

void init_interrupts();

void set_irq_handler(int irq_number, void (*handler)(void));

/* _ONLY_ used when a userspace process is first spawned */
void init_userspace_frame(struct interrupt_frame *frame);

void return_from_interrupt(struct interrupt_frame *to_state);

/* ========== Exception Functions, see exceptions.c ========== */

void init_exceptions();

/* ========== syscalls.c ========== */

u64 systemcall(u64 number, u64 arg1, u64 arg2, u64 arg3);

/* ========== Page allocation, see memory.c ========== */

#define PAGE_SIZE 4096

void init_memory();

/* returns paddr, provide a fill byte or -1 to not clear */
u64 get_free_page(int fill);

void free_page(u64 page_paddr);

/* ========== Memory allocation, see allocator.c ========== */

void *malloc(u64 req_size);
void *calloc(u64 num, u64 size);
void free(void *area);

/* ========== Paging, see paging.c ========== */

#define direct_map_shift 0
_inline void *kern_vaddr(u64 paddr)
{
    return (void*) (direct_map_shift + paddr);
}

_inline u64 paddr(void *vaddr)
{
    return (u64)vaddr - direct_map_shift;
}

/* ========== Timers, see timers.c ========== */

/* called from timer interrupt */
void run_timers();

/* sleep the current task */
void sleep(u64 micros);
    
void set_timeslice_len(u64 tslen);

/* ========== Thread Funcions, see threading.c ========== */

extern struct thread *current_thread;

/* init a thread structure, it does _not_ become runnable */
typedef void(*threadfn)(void*);
u64 thread_init(struct thread *thread, threadfn func, void *arg);

/* exit the current thread and reschedule */
void exit_thread();

/* get a thread struct by its pid or return NULL */
struct thread *find_thread(u64 pid);

/* suspend current thread and resume passed-in one */
void context_switch(struct thread *next_thread) _naked;

/* ========== Scheduler, see tasks.c ========== */

/* called in timer.c */
void timeslice_expired();

/* add task the scheduler queue */
void make_runnable(struct thread *thread);

/* add current thread to waitqueue then reschedule */
void wait_on(struct waitqueue *queue);

/* make all threads on the runqueue runnable */
void wake_all(struct waitqueue *queue);

/* pick a new task and run it, flags=1 if we remain runnable */
void schedule_now(u64 flags);

/* if its about time to switch tasks then do it else no-op */
void possibly_schedule();

/* start the scheduler loop */
void start_multitasking() _noreturn;

/* ========== Initrd, see initrd.c ========== */

struct file_data get_initrd_file(const char *filename);

u64 start_new_process(char *binary, char *arg);

/* ========== Utility Functions ========== */

_inline void log_header()
{
    if (current_thread == NULL)
        return;
    printf("[%09d] PID %02d (%s %s) ",
    current_time(),
    current_thread->pid,
    current_thread->name,
    current_thread->arg);
}

#define round_up(x, b) ( ((x) + ((b) - 1) ) / (b) * (b))
#define offset_of(type, element) (size_t)&(((type*)0x0)->element)
#define container_of(type, element, x) ((type*)((char*)(x) - offset_of(type, element)))

_inline u64 get_bits(u64 val, u64 end, u64 start)
{
    u64 mask = (1UL << (end - start + 1)) - 1;
    return (val >> start) & mask;
}

_inline u64 mask_bits(u64 val, u64 end, u64 start)
{
    u64 mask = (1UL << (end - start + 1)) - 1;
    return ((val >> start) & mask) << start;
}

_inline u64 set_bits(u64 val, u64 end, u64 start, u64 value)
{
    u64 mask = (1UL << (end - start + 1)) - 1;
    val = (val & ~(mask << start)) | ((value & mask) << start);
    return val;
}

#define set_bit(a, b, c) set_bits(a, b, b, c)
#define get_bit(a, b) get_bits(a, b, b)

_inline void abort(char *msg) {
    printf("ERROR: %s\n", msg);
    halt_and_catch_fire();
}
_inline void assert(int i, char *msg) {
    if (!i)
        abort(msg);
}

_inline void crash()
{
    printf("\nCrashed. Use Ctrl-C or \"Ctrl-A X\" at the terminal to stop.");
    halt_and_catch_fire();
}
