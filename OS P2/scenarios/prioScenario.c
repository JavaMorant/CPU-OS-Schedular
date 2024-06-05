#define work()                            \
    for (u64 i = 0; i < (1UL << 23); i++) \
    {                                     \
    }
typedef unsigned long u64;
#define NULL ((void*)0)

int strcmp(const char *str1, const char *str2)
{
    while (1)
    {
        int diff = *str1 - *str2;
        if (diff != 0 || *str1 == 0 || *str2 == 0)
            return diff;
        str1++;
        str2++;
    }
}

int parse_number(const char *buf, u64 *ret, int base)
{
    u64 val = 0;
    int read = 0;

    for (; buf[read] != 0; read++)
    {
        char c = buf[read];
        int digit_val = 100;

        if (c >= '0' && c <= '9')
            digit_val = c - '0';
        else if (c >= 'a' && c <= 'f')
            digit_val = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F')
            digit_val = c - 'A' + 10;

        if (digit_val >= base)
            break;

        val = val * base + digit_val;
    }

    *ret = val;
    return read;
}


/* === system calls === */

u64 systemcall(u64 number, u64 arg1, u64 arg2, u64 arg3)
{
    u64 ret;
    asm("int $0x80\n"
        : "=a"(ret)
        : "a"(number), "b"(arg1), "c"(arg2), "d"(arg3));
    return ret;
}

/* output a message to the screen */
u64 write(char *msg)
{
    return systemcall(1, (u64)msg, 0, 0);
}

/* terminate the current process */
void exit()
{
    systemcall(2, 0, 0, 0);
}

/* spawn a new processes running the given binary with the given argument
    if prog is NULL the current binary is used.
    
    the pid of the new process is returned and can be wait()-ed on.
    */
u64 spawn(char *prog, char *arg)
{
    return systemcall(3, (u64)prog, (u64)arg, 0);
}

/* stop execution for a given number of microseconds */
void sleep(u64 micros)
{
    systemcall(4, micros, 0, 0);
}

/* stop execution until the process with the given pid terminates */
u64 wait(u64 pid)
{
    return systemcall(5, pid, 0, 0);
}

u64 spawn_with_prio(char *prog, char *arg, u64 prio)
{
    return systemcall(3, (u64)prog, (u64)arg, prio);
}




#define MSEC 1000UL

/* Entry Point of the Program */
void _start(char *argument)
{
    // Initial process
    if (strcmp(argument, "init") == 0)
    {
        write("Priority scheduling test started\n");

        // Spawn processes with different priorities
        // Assuming that spawn_with_prio is a modified version of spawn to handle priorities
        spawn_with_prio(NULL, "high_priority_process", 1);   // Higher priority
        spawn_with_prio(NULL, "medium_priority_process", 2); // Medium priority
        spawn_with_prio(NULL, "low_priority_process", 3);    // Lower priority
    }

    // High priority process
    if (strcmp(argument, "high_priority_process") == 0)
    {
        work();
        write("High priority process completed\n");
    }

    // Medium priority process
    if (strcmp(argument, "medium_priority_process") == 0)
    {
        work();
        write("Medium priority process completed\n");
    }

    // Low priority process
    if (strcmp(argument, "low_priority_process") == 0)
    {
        work();
        write("Low priority process completed\n");
    }

    exit(); // Terminate the process
}