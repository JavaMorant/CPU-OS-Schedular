#include "common.h"
#include "scheduler.h"

struct thread *current_thread = NULL;
static u64 pid_counter = 1;
static struct thread *lst = NULL;

void * _asmcall c_context_switch(struct thread *next_thread, void *old_stack) 
{
    if (current_thread) {
        current_thread->stack_ptr = old_stack;
    }
    
    current_thread = next_thread;
    
    return current_thread->stack_ptr;
}


_naked void context_switch(struct thread *next_thread)
{
    /* Since the function is "naked", the top of the stack is a return address
     * to resume the original thread.
     *
     * When we switch to a thread, we load its stack and execute a retq to pop 
     * this return address.
     */
    asm volatile(SYSV_SAVE_REGS
                 "mov %rsp, %rsi             ;"
                 "callq c_context_switch     ;"
                 "mov %rax, %rsp             ;"
                 SYSV_RESTORE_REGS
                 "retq                       ;");
}


void _asmcall c_thread_start(threadfn func, void *arg)
{
    func(arg);
    exit_thread();
}

static void _naked thread_start()
{
    asm("pop %rsi ; pop %rdi ; callq c_thread_start");
}


u64 thread_init(struct thread *thread, threadfn func, void *arg)
{
    u64   page   = get_free_page(0xAB);
    u64  *stack  = kern_vaddr(page + PAGE_SIZE);

    /* boot args */
    push_stack_64(&stack, (u64) func);
    push_stack_64(&stack, (u64) arg);

    /* return addr */
    push_stack_64(&stack, (u64) thread_start);
    
    for (int i = 0; i < NUM_SYSV_SAVE_REGS; i++)
        push_stack_64(&stack, 0x0);

    memset(thread, 0x0, sizeof(struct thread));
    thread->stack_ptr = stack;
    thread->pid = pid_counter++;

    thread->nxt = lst;
    if (lst)
        lst->prev = thread;
    lst = thread;

    thread->magic = THREAD_MAGIC;

    return thread->pid;
}

struct thread *find_thread(u64 pid)
{
    struct thread *ptr = lst;
    while (ptr) {
        if (ptr->pid == pid)
            return ptr;
        ptr = ptr->nxt;
    }

    return NULL;
}

void exit_thread()
{
    log_header();
    printf("thread stopped\n");
    wake_all(&current_thread->death_waiters);
                printf("$$ timeslice summary for pid %ld (%s %s) : queued at %ld, ran at %ld, ended at %ld\n",
                    current_thread->pid, current_thread->name, current_thread->arg,
                    current_thread->last_wake_time, current_thread->last_start_time, current_time());


    task_ended(&current_thread->sched_handle);

    current_thread->magic = DEAD_THREAD_MAGIC;

    if (current_thread->memory_space) {
        free_page(paddr(current_thread->memory_space));
    }

    if (current_thread->nxt)
        current_thread->nxt->prev = current_thread->prev;

    if (current_thread->prev)
        current_thread->prev->nxt = current_thread->nxt;

    if (lst == current_thread)
        lst = current_thread->nxt;
    
    if (current_thread->memory_space) {
        free(current_thread);
    }

    if (lst == NULL) {
        shutdown();
    }
    
    current_thread = NULL;

    schedule_now(0x0);
}
