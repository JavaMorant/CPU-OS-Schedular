#include "common.h"
#include "scheduler.h"

#define MIN_PRIORITY 1 // Lower number means higher priority
#define MAX_PRIORITY 5
#define BASE_TIMESLICE 1000UL // 1 ms
#define DEFAULT_TIMESLICE 5000UL // 5 ms
#define AGE_THRESHOLD 1000000UL // 1 second

// Macro definition of max
#define max(a, b) ((a) > (b) ? (a) : (b))

// Function prototypes
void age_tasks(void);
void print_state(void);

// Global variables
struct sched_task *runqueue = NULL; // Head of the runqueue, initially empty

/**
 * Initialize the round-robin scheduler (UNUSED).
 */
void init_scheduler() {
    // Nothing to do here
}

/**
 * Prepare data for a new task. The task should NOT start on the runqueue.
 */
void task_started(struct sched_task *task, u64 pid, char *name, int priority) {
    task->pid = pid;
    task->name = name;
    task->runtime = 0;
    task->priority = priority;  // Set the task's priority
    task->next = NULL;
}


/**
 * Clean up data associated with a task. The OS will not end a task that is on a runqueue.
 */
void task_ended(struct sched_task *task) {
    // Clean up any resources associated with the task
}

/**
 * A new task has become runnable. Add it to the runqueue and return either:
 * - DO_PREEMPT: OS should call dequeue_next_task as soon as possible.
 * - DONT_PREEMPT: The currently running task can continue.
 */
int enqueue_task(struct sched_task *task) {
    if (runqueue == NULL) {
        runqueue = task;
        return DO_PREEMPT;
    } else {
        struct sched_task *current = runqueue;
        struct sched_task *previous = NULL;

        while (current != NULL && current->priority >= task->priority) {
            previous = current;
            current = current->next;
        }

        if (previous == NULL) {  // Insert at the head
            task->next = runqueue;
            runqueue = task;
        } else {  // Insert in the middle or at the end
            task->next = current;
            previous->next = task;
        }
    }
    return DONT_PREEMPT;
}


/**
 * Called when a new task should be selected to run.
 * If the current task's time slice has expired, the scheduler
 * will first enqueue before calling this function.
 */
struct sched_task *dequeue_next_task() {
    if (runqueue != NULL) {
        struct sched_task *tmp = runqueue;
        runqueue = runqueue->next;
        tmp->next = NULL;
        return tmp; // Return the next task to run
    }

    // Check tasks for age
    age_tasks();
    
    return NULL; // No task to dequeue
}

/**
 * Called after dequeue_next_task(). Return how long this task should run for in microseconds.
 */
// Define constants for priorities and base time slice
long get_timeslice_length(struct sched_task *task) {
    if (!task) return DEFAULT_TIMESLICE;

   double priority_factor = 1.0 - (double)(task->priority - MIN_PRIORITY) / (MAX_PRIORITY - MIN_PRIORITY);
   return (long)(BASE_TIMESLICE * (1.0 + 0.5 * priority_factor)); // Adjust the factor as needed

   //return DEFAULT_TIMESLICE;
}


/**
 * Age all tasks on the runqueue. This is called after a task has run.
*/
void age_tasks() {
    struct sched_task *current = runqueue;
    while (current != NULL) {
        if (current->runtime > AGE_THRESHOLD) {
            current->priority = max(current->priority - 1, MIN_PRIORITY);
        }
        current = current->next;
    }

}

void print_state() {
    struct sched_task *current = runqueue;
    while (current != NULL) {
        printf("Task %s with priority %d\n", current->name, current->priority);
        current = current->next;
    }
}
