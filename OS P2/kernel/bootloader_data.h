#include "common.h"

typedef struct {
  u64   ptr;
  u64   size;
} _pack MMapEnt;

#define MMapEnt_Ptr(a)  ((a)->ptr)
#define MMapEnt_Size(a) ((a)->size & 0xFFFFFFFFFFFFFFF0)
#define MMapEnt_Type(a) ((a)->size & 0xF)
#define MMapEnt_IsFree(a) (((a)->size&0xF)==1)

struct loader_data {
    /* first 64 bytes is platform independent */
    u8    magic[4];    /* 'BOOT' magic */
    u32   size;        /* length of bootboot structure, minimum 128 */
    u8    protocol;    /* 1, static addresses, see PROTOCOL_* and LOADER_* above */
    u8    fb_type;     /* framebuffer type, see FB_* above */
    u16   numcores;    /* number of processor cores */
    u16   bspid;       /* Bootsrap processor ID (Local APIC Id on x86_64) */
    s16   timezone;    /* in minutes -1440..1440 */
    u8    datetime[8]; /* in BCD yyyymmddhhiiss UTC (independent to timezone) */
    u64   initrd_ptr;  /* ramdisk image position and size */
    u64   initrd_size;
    u64   fb_ptr;      /* framebuffer pointer and dimensions */
    u32   fb_size;
    u32   fb_width;
    u32   fb_height;
    u32   fb_scanline;

    /* the rest (64 bytes) is platform specific */
    u64 acpi_ptr;
    u64 smbi_ptr;
    u64 efi_ptr;
    u64 mp_ptr;
    u64 unused0;
    u64 unused1;
    u64 unused2;
    u64 unused3;

    /* from 128th byte, MMapEnt[], more records may follow */
    MMapEnt    mmap;
} _pack;

/* The bootloader (bootboot) ensure these vaddrs are mapped as required */
#define bootloader_conf       ((struct loader_data*) 0xffffffffffe00000UL)
#define BLDR_framebuffer      ((u8*)       0xfffffffffc000000UL)
#define BLDR_environment      ((char*)     0xffffffffffe01000UL)
#define BLDR_mimo             ((u8*)       0xfffffffff8000000UL)
