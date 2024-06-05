#include "common.h"
#include "bootloader_data.h"

const u64 INVALID = 0xFFFFFFFFFFFFFFFF;
u64 page_stack = INVALID;

#define first_word(x) *(u64*)kern_vaddr(x)

u64 get_free_page(int fill)
{
    if (page_stack == INVALID) {
        fb_setcol(RED_COL);
        printf("\n### ERROR: Out of memory ###\n");
        crash();
    }

    u64 ret = page_stack;
    page_stack = first_word(ret);

    memset(kern_vaddr(ret), fill, PAGE_SIZE);
    return ret;
}

void free_page(u64 paddr)
{
    first_word(paddr) = page_stack;
    page_stack = paddr;
}

/* in lieu of a proper memory allocator make every allocation 10pages */
const u64 block_size = PAGE_SIZE * 10;
void init_memory()
{
    MMapEnt *mmap_entry = &bootloader_conf->mmap;
    u8 *limit           = (u8*)bootloader_conf + bootloader_conf->size;
    
    while ((u8*) mmap_entry < limit) {
        if (MMapEnt_IsFree(mmap_entry)) {
            /* free range */
            s64 num_blocks = MMapEnt_Size(mmap_entry) / block_size;

            if (MMapEnt_Size(mmap_entry) % block_size != 0)
                num_blocks--;

            for (s64 i = 0; i < num_blocks; i++) {
                u64 block_paddr = MMapEnt_Ptr(mmap_entry) + i * block_size;
                free_page(block_paddr);
            }
        }
        mmap_entry++;
    }
}


/* very smooth-brain malloc, a proper implementation would allocate within the pages */
void *malloc(u64 req_size)
{
    if (req_size == 0)
        return NULL;    
    assert(req_size <= PAGE_SIZE, "structure too big for malloc use get_free_page");

    return kern_vaddr(get_free_page(0x00));
}

void *calloc(u64 num, u64 size)
{
    return malloc(num * size); /* NOTE: we are not checking for overflow */
}

void free(void *addr) {
    if (addr != NULL)
        free_page(paddr(addr));
}
