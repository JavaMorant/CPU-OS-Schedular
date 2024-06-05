#include "common.h"
#include "registers.h"

u32 apic_id; /* on x86 the APIC id is used to identify each CPU, it is read from the APIC */
static u64 apic_addr;

static void write_apic32(int reg, u32 val)
{
    *(volatile u32*) (kern_vaddr(apic_addr) + reg) = val;
}

static u32 read_apic32(int reg)
{
    return *(volatile u32*) (kern_vaddr(apic_addr) + reg);
}


void apic_end_of_interrupt()
{
    write_apic32(0x00b0, 0x1);
}

u64 init_apic() {
    u64 msr = read_msr(IA32_APIC_BASE_MSR);
    apic_addr =  mask_bits(msr, 36, 12);;
    write_msr(IA32_APIC_BASE_MSR, apic_addr | IA32_APIC_BASE_MSR_ENABLE);
    apic_id = read_apic32(0x20) >> 24; /* return LAPIC id */
    return apic_id;
}


/* timer */

static u64 tsc_ticks_per_us;
static u64 apic_ticks_per_ms;

asm_irq_handler(interrupt_hander_apic_timer)(struct interrupt_frame *state)
{
    //printf("****** timer\n");
    run_timers();
}

static void calibrate_timer()
{
    /* prepare PIT and LAPIC */
    outb(PIT_CTRL, 3 << 4);
    outb(PIT_CHAN0, 0xFF);
    write_apic32(APIC_TIMER_DIV, DIV_1);

    /* start PIT and LAPIC */
    outb(PIT_CHAN0, 0xFF);
    write_apic32(APIC_TIMER_SET, 0xFFFFFFFF);

    u64 pit_start  = 0xFFFF;
    u64 tsc_start  = rdtsc();
    u64 apic_start = 0xFFFFFFFF;
    
    while (rdtsc() < tsc_start + 30000000UL) {}

    outb(PIT_CTRL, 0); /* latch */
    u64 pit_end   = inb(PIT_CHAN0);
    pit_end      |= inb(PIT_CHAN0) << 8;
    u64 tsc_end   = rdtsc();
    u64 apic_end  = read_apic32(APIC_TIMER_GET);

#if 0
    printf("pit %lx -> %lx\n", pit_start, pit_end);
    printf("tsc %lx -> %lx\n", tsc_start, tsc_end);
    printf("apc %lx -> %lx\n", apic_start, apic_end);
#endif

    u64 elapsed_ns   = (pit_start - pit_end) * 838;
    u64 elapsed_tsc  = tsc_end    - tsc_start;
    u64 elapsed_apic = apic_start - apic_end;

    apic_ticks_per_ms = 1000000 * elapsed_apic / elapsed_ns;
    tsc_ticks_per_us  = 1000    * elapsed_tsc  / elapsed_ns;

    //printf("apic per ms %d tsc per us %d\n", apic_ticks_per_ms, tsc_ticks_per_us);
    
    /* stop LAPIC */
    write_apic32(APIC_TIMER_SET, 0x0);
}

void schedule_apic_timer_interrupt(u64 next_time)
{
    if (next_time == 0) {
        /* disable timer */
        write_apic32(APIC_TIMER_SET, 0x0);        
        return;
    }

    u64 cur   = current_time();
    u64 ticks = (next_time - cur) * apic_ticks_per_ms / 1000;
    
    if (next_time > cur) {
        if (ticks <= 0xFFFFFFFFUL) {
            //printf("ticks = %d\n", ticks);
            write_apic32(APIC_TIMER_DIV, DIV_1);
            write_apic32(APIC_TIMER_SET, ticks);
        } else if (ticks <= 0xFFFFFFFFUL * 128) {
            //printf("128 ticks = %d\n", ticks / 128);
            write_apic32(APIC_TIMER_DIV, DIV_128);
            write_apic32(APIC_TIMER_SET, ticks);
        } else {
            //printf("Lots of ticks\n");
            write_apic32(APIC_TIMER_DIV, DIV_128);
            write_apic32(APIC_TIMER_SET, 0xFFFFFFFF);
        }
    } else {
        /* fire soon as possible */
        write_apic32(APIC_TIMER_SET, 0x1);
    }
}

void init_apic_timer()
{
    /* set LAPIC time to int 0x10 and oneshot mode */
    write_apic32(APIC_LVT_TIMER, 0x30);
    calibrate_timer();
    set_irq_handler(0x30, interrupt_hander_apic_timer);
}

u64 current_time()
{
    return rdtsc() / tsc_ticks_per_us;
}


