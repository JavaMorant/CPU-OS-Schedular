#include "common.h"
#include "scheduler.h"

#define get_offset(ptr, offset, type) *(type*)(ptr + offset)

#define PT_LOAD 1
#define ELF_FLAG_READ  4
#define ELF_FLAG_WRITE 2
#define ELF_FLAG_EXEC  1 
struct elf_segment
{
    u32 p_type;
    u32 p_flags;
    
    u64 p_offset;
    u64 p_vaddr;
    u64 p_paddr;
    u64 p_filesz;
    u64 p_memsz;
    u64 p_align;
};

static u64 get_elf_entrypoint(u8* ptr)
{
    return *(u64*)(ptr + 0x18);
}

static struct elf_segment *get_elf_segment(u8 *ptr, int index)
{
    u8  phoff      = *(u64*)(ptr + 0x20);
    u16 ph_entries = *(u16*)(ptr + 0x38);
    u16 ph_size    = *(u16*)(ptr + 0x36);

    if (index >= ph_entries)
        return 0;

    struct elf_segment *ph = (struct elf_segment*) (ptr + (phoff + ph_size * index));    
    return ph;
}

static void load_elf(u8 *elf_file, struct interrupt_frame *entry_registers)
{
    /* NOTE: we don't have a proper allocation */
    u8 *base  = kern_vaddr(get_free_page(0x5a));
    u64 offset = -1;

    for (int i = 0;; i++) {
        struct elf_segment *segment = get_elf_segment(elf_file, i);
        if (!segment)
            break;
        if (segment->p_type != PT_LOAD)
            continue;

        if (offset == -1)
            offset = segment->p_vaddr;

        for (u64 vaddr = segment->p_vaddr; vaddr < segment->p_vaddr + segment->p_memsz; vaddr += PAGE_SIZE) {
            u8 *page = base + (vaddr - offset);

            if (vaddr < segment->p_vaddr + segment->p_filesz) {
                memcpy(page, elf_file + segment->p_offset, PAGE_SIZE);
            } else {
                memset(page, 0, PAGE_SIZE);
            }
        }
    }
    
    /* map in a stack */
    u64 stack_page    = get_free_page(0xAD);
    u64 stack_vaddr   = (u64) kern_vaddr(stack_page) + PAGE_SIZE - 512;

    /* entry point */
    u64 entry_point = get_elf_entrypoint(elf_file);
    entry_registers->rsp    = stack_vaddr; 
    entry_registers->rip    = (u64) base + (entry_point - offset);

    current_thread->memory_space = base;
}

static void _proc_thread(void *arg)
{
    struct thread *thread = arg;
    char *path = thread->name;

    struct file_data elf_file = get_initrd_file(path);
    if (!elf_file.data) {
        fb_setcol(RED_COL);
        printf("\n### ERROR: Could not load elf file: %s ###\n", path);
        crash();
    }
    
    struct interrupt_frame entry_regs;
    init_userspace_frame(&entry_regs);
    load_elf(elf_file.data, &entry_regs);
    entry_regs.rdi = (u64) thread->arg; /* pass argument pointer in rdi */
    return_from_interrupt(&entry_regs);
}

u64 start_new_process(char *binary, char *arg)
{
    struct thread *child = malloc(sizeof(*child));
    thread_init(child, _proc_thread, child);

    memcpy(child->name, binary, strlen(binary) + 1);
    memcpy(child->arg,  arg,    strlen(arg   ) + 1);

    task_started(&child->sched_handle, child->pid, child->name, 0);
    make_runnable(child);
    return child->pid;
}
