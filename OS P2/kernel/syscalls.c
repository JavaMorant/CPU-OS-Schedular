#include "common.h"

/* 
 * syscall number is taken from rax
 * arg1, arg2, arg3 = rbx, rcx, rdx
 *
 * return value overwrites rax
 */
u64 systemcall(u64 number, u64 arg1, u64 arg2, u64 arg3)
{

    if (number == 1) {
        /* write */
        log_header();
        printf("syscall write(...)\n");

        fb_setcol(0xFFFFFF);
        printf("%s", (const char*) arg1);
        fb_setcol(GRAY_COL);
        
        return 0x3;
        
    } else if (number == 2) {
        /* exit */ 
        log_header();
        printf("syscall exit()\n");

        exit_thread();
        
    } else if (number == 3) {
        /* spawn */
        if (arg1 == 0)
            arg1 = (u64) current_thread->name;
            
        log_header();
        printf("syscall spawn(\"%s\", \"%s\")\n", (char*) arg1, (char*) arg2);
        return start_new_process((char*) arg1, (char*) arg2);

    } else if (number == 4) {
        /* sleep */
        log_header();
        printf("syscall sleep(%ld)\n", arg1);

        sleep(arg1);

    } else if (number == 5) {
        /* wait */
        log_header();
        printf("syscall wait(%ld)\n", arg1);

        struct thread *other = find_thread(arg1);

        if (!other)
            return 0;

        wait_on(&other->death_waiters);
    }

    return -1;
}
