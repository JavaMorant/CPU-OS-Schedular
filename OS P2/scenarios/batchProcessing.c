#define work()                            \
    for (u64 i = 0; i < (1UL << 23); i++) \
    {                                     \
    }
typedef unsigned long u64;
#define NULL ((void*)0)

/* Function Definitions */

/* Custom strlen function */
u64 my_strlen(const char *str)
{
    const char *s;
    for (s = str; *s; ++s) {}
    return (s - str);
}

/* Compare two strings */
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

/* Parse a number from a string */
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

/* === System Calls === */

/* System call to perform an operation */
u64 systemcall(u64 number, u64 arg1, u64 arg2, u64 arg3)
{
    u64 ret;
    asm("int $0x80\n"
        : "=a"(ret)
        : "a"(number), "b"(arg1), "c"(arg2), "d"(arg3));
    return ret;
}

/* Output a message to the screen */
u64 write(char *msg)
{
    return systemcall(1, (u64)msg, my_strlen(msg), 0);
}

/* Terminate the current process */
void exit()
{
    systemcall(2, 0, 0, 0);
}

/* Spawn a new process */
u64 spawn(char *prog, char *arg)
{
    return systemcall(3, (u64)prog, (u64)arg, 0);
}

/* Stop execution for a given number of microseconds */
void sleep(u64 micros)
{
    systemcall(4, micros, 0, 0);
}

/* Wait for a process to terminate */
u64 wait(u64 pid)
{
    return systemcall(5, pid, 0, 0);
}

/* === */

#define MSEC 1000UL

/* Entry Point of the Program */
void _start(char *argument)
{
    // Initial process
    if (strcmp(argument, "init") == 0)
    {
        write("Batch processing simulation started\n");

        // Spawn different batch processes
        spawn(NULL, "batch_process_1");
        spawn(NULL, "batch_process_2");
        spawn(NULL, "batch_process_3");
        spawn(NULL, "batch_process_4");
        spawn(NULL, "batch_process_5");
        // More batch processes can be spawned as needed
    }

    // Batch process 1
    if (strcmp(argument, "batch_process_1") == 0)
    {
        work(); // Simulate a batch task
        write("Batch process 1 completed\n");
    }

    // Batch process 2
    if (strcmp(argument, "batch_process_2") == 0)
    {
        work(); // Simulate a batch task
        write("Batch process 2 completed\n");
    }

    // Batch process 3
    if (strcmp(argument, "batch_process_3") == 0)
    {
        work(); // Simulate a batch task
        write("Batch process 3 completed\n");
    }

    // Batch process 4
    if (strcmp(argument, "batch_process_4") == 0)
    {
        work(); // Simulate a batch task
        write("Batch process 4 completed\n");
    }
    if (strcmp(argument, "batch_process_5") == 0)
    {
        work(); // Simulate a batch task
        write("Batch process 5 completed\n");
     
    }
    


    exit(); // Terminate the process
}
