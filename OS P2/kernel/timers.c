#include "common.h"

/* sorted */
static u64 timeslice_end = 0;
static struct thread *sleeping_threads = NULL;


static void timers_schedule_timer_interrupt()
{
    u64 time = 0;
    
    /* a wakeup from a sleep() or the end of the timeslice, whichever sooner */
    if (sleeping_threads) {
        time = sleeping_threads->sleep_data.wake_time;
    }

    if (timeslice_end != 0 && (time == 0 || timeslice_end < time))
        time = timeslice_end;

    schedule_apic_timer_interrupt(time);
}

void run_timers()
{
    if (timeslice_end > 0 && current_time() > timeslice_end) {
        timeslice_expired();
        timeslice_end = 0;
    }

    while (1) {
        struct thread *thread = sleeping_threads;
        if (!thread)
            break;
        
        /* allow a 1 us slack */
        if (thread->sleep_data.wake_time > current_time() + 1) {
            break;
        }

        printf("*** timer woke pid %ld\n", thread->pid);
        make_runnable(thread);
        sleeping_threads = thread->sleep_data.next;
    }

    timers_schedule_timer_interrupt();
}

void sleep(u64 micros)
{
    current_thread->sleep_data.wake_time = current_time() + micros;
    log_header();
    printf("sleeping until %ld\n", current_thread->sleep_data.wake_time);

    struct thread **ptr = &sleeping_threads;
    while (*ptr != NULL &&
           (*ptr)->sleep_data.wake_time < current_thread->sleep_data.wake_time) {
        ptr = &((*ptr)->sleep_data.next);
    }

    current_thread->sleep_data.next = *ptr;
    *ptr = current_thread;

    timers_schedule_timer_interrupt();
    schedule_now(0x0);
}

/* called by the scheduler */
void set_timeslice_len(u64 tslen)
{
    if (tslen == 0)
        timeslice_end = 0;
    else
        timeslice_end = current_time() + tslen;
    timers_schedule_timer_interrupt();
}
