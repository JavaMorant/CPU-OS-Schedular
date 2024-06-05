#define work_load()                         \
    for (u64 i = 0; i < (1UL << 25); i++)   \
    {                                       \
    }

#define simulate_io_operation()             \
    sleep(500 * MSEC);                      \
    work_short();                           \
    sleep(500 * MSEC);

void _start(char *argument)
{
    // Initial process
    if (strcmp(argument, "init") == 0)
    {
        write("Mixed workload scenario started\n");

        // Spawning interactive, load, and I/O-bound processes
        spawn_with_prio(NULL, "interactive", 1); // High-priority interactive task
        spawn(NULL, "load_task");               // CPU-intensive task
        spawn(NULL, "io_task");                 // I/O-bound task
    }

    // Interactive Task
    if (strcmp(argument, "interactive") == 0)
    {
        for (int i = 0; i < 10; i++) {
            write("Interactive task responding\n");
            sleep(100 * MSEC); // Simulating user interaction time
        }
    }

    // Load Task
    if (strcmp(argument, "load_task") == 0)
    {
        work_load(); // Simulate a long-running CPU-intensive task
        write("Load task completed\n");
    }

    // I/O-bound Task
    if (strcmp(argument, "io_task") == 0)
    {
        for (int i = 0; i < 5; i++) {
            simulate_io_operation(); // Simulate a task that performs I/O operations
        }
        write("I/O-bound task completed\n");
    }

    exit(); // Terminate the process
}
