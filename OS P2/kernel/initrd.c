#include "common.h"
#include "bootloader_data.h"

/* The bootloader passes us the initrd files in a TAR file which is loaded into memory
 *
 * This is a very limited TAR file parser that lets us find files by name
 */
struct tar_header {
    char filename[100];
    char file_mode[8];
    char owner_id[8];
    char group_id[8];
    char size[12];
    char mod_time[12];
    char checksum[8];
    char link[1];
    char link_name[100];
};


static struct file_data tar_getfile(struct tar_header *in)
{
    u64 file_size;
    parse_number(in->size, &file_size, 8);
    
    return (struct file_data) {
                            .data = (u8*)((u64) in + 512),
                            .size = file_size
    };
}

static struct tar_header *tar_next_header(struct tar_header *in)
{
    u64 file_size;
    parse_number(in->size, &file_size, 8);

    /* the next tar header will be found aligned to 512 bytes after this file data */
    u64 addr = round_up((u64) in + 512 + file_size, 512);
    return (struct tar_header*) addr;
}

struct file_data get_initrd_file(const char *filename)
{
    u8 *initrd      = (u8*) kern_vaddr(bootloader_conf->initrd_ptr);

    /* Iterate through the TAR headers */
    struct tar_header *header = (struct tar_header*) initrd;
    struct tar_header *header_limit = (struct tar_header*) (initrd + bootloader_conf->initrd_size);
    
    while (header->filename[0] != '\0' && header < header_limit) {
        
        if (strcmp(header->filename, filename) == 0) {
            return tar_getfile(header);
        }

        header = tar_next_header(header);
    }

    /* return NULL ptr to indicate failure */
    fb_setcol(RED_COL);
    printf("\n### ERROR: File \"%s\" could not be found in the initrd ###\n", filename);
    crash();
    return (struct file_data){0};
}
