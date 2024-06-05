#include "common.h"
#include "scheduler.h"

static struct thread idle_thread;
static int schedule_needed = 0;
static int runnable_tasks = 0;

void wait_on(struct waitqueue *queue)
{
    if (queue->signalled) {
        return;
    } else {
        current_thread->waitqueue_data.next = queue->proc_list;
        queue->proc_list = current_thread;
        schedule_now(0x0);
    }
}

void wake_all(struct waitqueue *queue)
{
    if (!queue->signalled) {
        queue->signalled = 1;
        
        struct thread *thread = queue->proc_list;
        while (thread) {
            make_runnable(thread);
            thread = thread->waitqueue_data.next;
        }
        queue->proc_list = NULL;
    }
}

void make_runnable(struct thread *thread)
{    
    log_header();
    printf("Waking pid %d\n", thread->pid);
    thread->wake_time = current_time();
    thread->on_runqueue = 1;
    runnable_tasks++;
    if (enqueue_task(&thread->sched_handle) == DO_PREEMPT || current_thread == &idle_thread) {
        schedule_needed = 1;
    }
}

void timeslice_expired()
{
    log_header();
    printf("time slice expired, scheduling soon\n");
    schedule_needed = 1;
}

void schedule_now(u64 flags)
{    
    u64 sched_time = current_time();

    if (flags && current_thread && current_thread != &idle_thread) {
        make_runnable(current_thread);
    }
    
    schedule_needed = 0;

    struct sched_task  *next_task   = dequeue_next_task();

    struct thread *next_thread;
    u64 len;

    if (next_task) {
        next_thread = container_of(struct thread, sched_handle, next_task);
        len = get_timeslice_length(next_task);

        if (next_thread->magic == DEAD_THREAD_MAGIC) {
            fb_setcol(RED_COL);
            printf("\n### ERROR: dequeue_next_task returned a task that has already ended ###\n\n");
            crash();
        } else if (next_thread->magic != THREAD_MAGIC) {
            fb_setcol(RED_COL);
            printf("\n### ERROR: dequeue_next_task returned something that doesn't look like a task ###\n\n");
            crash();
        } else if (next_thread == &idle_thread) {
            fb_setcol(RED_COL);
            printf("\n### ERROR: dequeue_next_task returned the idle task (how did you even do that?!) ###\n\n");
            crash();
        } else if (!next_thread->on_runqueue) {
            fb_setcol(RED_COL);
            printf("\n### ERROR: dequeue_next_task returned a task that isn't on the runqueue ###\n\n");
            crash();
        }

        runnable_tasks--;
        next_thread->on_runqueue = 0;
    } else {
        next_thread = &idle_thread;
        len = 0;
        if (runnable_tasks) {
            fb_setcol(RED_COL);
            printf("\n### ERROR: going idle when there are tasks to run ###\n");
            crash();
        }
    }


    set_timeslice_len(len);
    if (next_thread != current_thread) {
        if (current_thread != NULL && current_thread != &idle_thread) {
            printf("$$ timeslice summary for pid %ld (%s %s) : queued at %ld, ran at %ld, ended at %ld\n",
                    current_thread->pid, current_thread->name, current_thread->arg,
                    current_thread->last_wake_time, current_thread->last_start_time, sched_time);
        }
        printf("=== schedule pid %ld (%s %s)\n",
            next_thread->pid, next_thread->name, next_thread->arg);

        next_thread->last_start_time = sched_time;
	next_thread->last_wake_time  = next_thread->wake_time;

        /* current thread will freeze here,
           next thread will resume at it's last call to context_switch
           which (confusingly) will also be inside this function. However, the local
           variables will be different  */
        context_switch(next_thread);
    }
}

void possibly_schedule()
{
    if (schedule_needed) {
        schedule_now(0x1);
    }
}

_noreturn void start_multitasking()
{   
    memset(&idle_thread, 0x0, sizeof(struct thread));
    char *n = "idle";
    memcpy(idle_thread.name, n, sizeof(n) + 1);
    /* idle thread has no sched handle */
    current_thread   = &idle_thread;
    
    schedule_needed = 1;

    while (1) {
        if (schedule_needed) {
            schedule_now(0x0);
        }
        
        /* atomic sleep sequence,
           well, an nmi might be able to ruin our day */
        asm volatile("sti ; hlt ; cli");
    }
}
