#include "common.h"
#include "asm.h"
#include "bootloader_data.h"
#include "registers.h"

/* This file contains code for printing information out to the screen */

/********  x86 SERIAL  ***********/

/* adapted from OSDEV wiki */
static int init_serial() {
    outb(SERIAL_PORT + 1, 0x00);    /* Disable all interrupts */
    outb(SERIAL_PORT + 3, 0x80);    /* Enable DLAB (set baud rate divisor) */
    outb(SERIAL_PORT + 0, 0x03);    /* Set divisor to 3 (lo byte) 38400 baud */
    outb(SERIAL_PORT + 1, 0x00);    /*                  (hi byte) */
    outb(SERIAL_PORT + 3, 0x03);    /* 8 bits, no parity, one stop bit */
    outb(SERIAL_PORT + 2, 0xC7);    /* Enable FIFO, clear them, with 14-byte threshold */
    outb(SERIAL_PORT + 4, 0x0B);    /* IRQs enabled, RTS/DSR set */
    outb(SERIAL_PORT + 4, 0x1E);    /* Set in loopback mode, test the serial chip */
    outb(SERIAL_PORT + 0, 0xAE);    /* Test serial chip (send byte 0xAE and check if serial returns same byte) */
 
    /* Check if serial is faulty (i.e: not same byte as sent) */
   if(inb(SERIAL_PORT + 0) != 0xAE) {
      return 1;
   }
 
   /* If serial is not faulty set it in normal operation mode
    * (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled) */
   outb(SERIAL_PORT + 4, 0x0F);
   return 0;
}

static int is_transmit_empty() {
   return inb(SERIAL_PORT + 5) & 0x20;
}
 
static void write_serial(char a) {
   while (is_transmit_empty() == 0);
   outb(SERIAL_PORT,a);
}

/********  FRAMEBUFFER  ***********/

struct font_file{
    u32 magic;
    u32 version;
    u32 headersize;
    u32 flags;
    u32 numglyph;
    u32 bytesperglyph;
    u32 height;
    u32 width;
    u8  glyphs;
} _pack;

/*
 * We use the linker to embed "font.psf" directly in the binary,
 * its contents start at &_binary_font_psf_start
 */
extern struct font_file _binary_font_psf_start;

struct {
    u8 *buf;
    int x, y;
    
    int raw_width, raw_height, scanline;

    int char_width, char_height;
    int width, height;
    
    struct font_file *font;
    u32 col;
} framebuf;

void fb_setcol(u32 col)
{
    framebuf.col = col;
}

static void fb_newline()
{
    framebuf.x = 0;
    framebuf.y = (framebuf.y + 1) % framebuf.height;

    /* clear the row below */
    int clear_row = (framebuf.y + 1) % framebuf.height;
    
    for (int row = 0; row < framebuf.char_height; row++) {
        u64 off    = (clear_row * framebuf.char_height + row) * framebuf.scanline;
        u32 *data  = (u32*)(framebuf.buf + off);
        
        for (int p = 0; p < framebuf.raw_width; p++) {
            *data++ = 0x000000;
        }
    }
}

static void fb_advance()
{
    framebuf.x++;
    if (framebuf.x >= framebuf.width)
        fb_newline();
}

static void init_fb()
{
    framebuf.buf  = BLDR_framebuffer;
    framebuf.font = &_binary_font_psf_start;
    
    framebuf.char_width    = framebuf.font->width + 1;
    framebuf.char_height   = framebuf.font->height;
    
    framebuf.raw_width  = bootloader_conf->fb_width;
    framebuf.raw_height = bootloader_conf->fb_height;
    framebuf.scanline   = bootloader_conf->fb_scanline;

    framebuf.width  = bootloader_conf->fb_width  / framebuf.char_width;
    framebuf.height = bootloader_conf->fb_height / framebuf.char_height;

    init_serial();
}

static void fb_putchar(char c)
{            
    if (c == '\n') {
        fb_newline();
        return;
    }

    struct font_file *font = framebuf.font;
    int bytes_per_row = (font->width + 7) / 8;

    int fboffset = framebuf.y * framebuf.char_height * framebuf.scanline;
    fboffset    += framebuf.x * framebuf.char_width  * 4;

    int glyph_index;
    if (c > 0 && c < font->numglyph)
        glyph_index = c;
    else
        glyph_index = 0; /* default glyph */
    u8 *glyph_row = ((u8*) font + font->headersize) + glyph_index * font->bytesperglyph;
    
    /* draw each row */
    for(int row = 0; row < font->height; row++) {
        /*
         * draw each pixel,
         * note font->char_width != framebuf.char_width due to spacing
         */
        for(int col = 0; col < font->width; col++) {
            int byte = col / 8;
            int bit  = 7 - (col % 8);
            int pix  = glyph_row[byte] & (1 << bit);

            
            u32 *pixel = (u32*)(framebuf.buf + fboffset + col * 4);
            
            if (pix)
                *pixel = framebuf.col; /* on */
            else
                *pixel = 0x0;      /* off */
        }

        fboffset  += framebuf.scanline;
        glyph_row += bytes_per_row;
    }
        
    fb_advance();
}

/********  Generic Printing Functions  ***********/

void init_console()
{
    init_serial();
    init_fb();
}

int putchar(char c)
{
    write_serial(c);
    fb_putchar(c);
    return c;
}

void puts(char *str)
{
    for (char c; (c = *str); str++) {
        putchar(c);
    }
}


int parse_number(const char *buf, u64 *ret, int base)
{
    u64 val = 0;
    int read = 0;
    
    for (; buf[read] != 0; read++) {
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

static void fmt_string(char **buf, char *str, int len, int pad, char pad_char, int left_pad)
{
    if (left_pad) {
        for (int l = len; l < pad; l++)
            *(*buf)++ = pad_char;
    }

    for (int l = 0; l < len; l++)
        *(*buf)++ = str[l];

    if (!left_pad) {
        for (int l = len; l < pad; l++)
            *(*buf)++ = pad_char;
    }
}

static void fmt_number(char **buf, u64 d, int base,
                       int pad, char pad_char, int left_pad,
                       char pos_char,
                       int is_signed, int hex_upper)
{
    int i = 0;
    
        if (is_signed && (d >> 63)) {
            (*buf)[i++] = '-';
            pad--;
            /* assume TC */
            d = (~d + 1);
        } else if (pos_char) {
            (*buf)[i++] = pos_char;
            pad--;
        }

        int count = 0;
        if (d == 0) {
            count = 1;
        } else {
            u64 dd = d;
            while (dd > 0) {
                count++;
                dd /= base;
            }
        }

        pad -= count;
        if (left_pad) {
            for (int j = 0; j < pad; j++) {
                (*buf)[i++] = pad_char;
            }
        }

        i += count;
        if (d == 0)
            (*buf)[i - 1] = '0';
        
        for (int j = 0; d > 0; j++) {
            char c = d % base;
            if (c <= 9)
                (*buf)[i - j - 1] = '0' + c;
            else if (hex_upper)
                (*buf)[i - j - 1] = 'A' + (c - 10);
            else
                (*buf)[i - j - 1] = 'a' + (c - 10);
            d /= base;
        }

        if (!left_pad) {
            for (int j = 0; j < pad; j++) {
                (*buf)[i++] = pad_char;
            }
        }


    *buf += i;
}


#define va_start(v,l)   __builtin_va_start(v,l)
#define va_end(v)       __builtin_va_end(v)
#define va_arg(v,l)     __builtin_va_arg(v,l)
typedef __builtin_va_list va_list;

int printf(const char* format, ...)
{
    static char ibuf[4096];
    char *buf = ibuf;

    va_list va;
    va_start(va, format);

    
    for (;*format; format++) {
        if (*format != '%') {
            *buf++ = *format;
        } else {
            /* advance format string */
            if (*format++ == 0)
                goto abrupt_format_end;

            /* literal % */
            if (*format == '%') {
                *buf++ = *format;
                continue;
            }

            /* %[parameter][flags][width][.precision][length]type */
            char pos_char  = 0;
            char pad_char  = ' ';
            int  left_pad  = 1;
            int  alt_form  = 0;

            u64 pad       = 0;
            u64 precision = 0;

            // 0 = char, 1 = short, 2 = int, 3 = long, 4 = long long
            int length    = 2;

            /* [FLAGS] */
            while (1) {
                switch(*format) {
                case '-':
                    left_pad = 0;
                    break;
                case '+':
                    pos_char = '+';
                    break;
                case ' ':
                    pos_char = ' ';
                    break;
                case '0':
                    pad_char = '0';
                    break;
                case '\'':
                    /* unimplemented */
                    break;
                case '#':
                    alt_form = 1;
                    break;
                default:
                    goto flags_done;
                }

                /* advance format string */
                if (*format++ == 0)
                    goto abrupt_format_end;
            }
            
        flags_done:
            /* [WIDTH] */
            if (*format == '*') {
                pad = (int) va_arg(va, int);
                /* advance format string */
                if (*format++ == 0)
                    goto abrupt_format_end;
            } else {
                format += parse_number(format, &pad, 10);
                if (*format == 0)
                    goto abrupt_format_end;
            }

            /* [.PRECISION] */
            if (*format == '.') {
                format++;
                
                if (*format == '*') {
                    precision = (int) va_arg(va, int);
                } else {
                    format += parse_number(format, &precision, 10);
                    if (*format == 0)
                        goto abrupt_format_end;
                }
            }

            /* [length] */
            while (1) {
                switch(*format) {
                case 'h':
                    if (length > 0)
                        length--;
                    else
                        goto length_done;
                    break;
                
                case 'l':
                    if (length < 4)
                        length++;
                    else
                        goto length_done;
                    break;
                
                case 'z': /* size_t */
                case 'j': /* intmax_t */
                case 't': /* ptrdiff_t */
                    /* unimplemented */
                    break;

                default:
                    goto length_done;
                }

                /* advance format string */
                if (*format++ == 0)
                    goto abrupt_format_end;
            }
            
        length_done:
            /* first try to handle non-int cases */
            switch(*format) {
            case 'c':
                *buf++ = (char)va_arg(va, int);
                continue;
            case 's':
                ;
                char *s = (char*)va_arg(va, char*);
                int length = strlen(s);
                fmt_string(&buf, s, length, pad, pad_char, left_pad);
                continue;
            case 'n':
                ;
                int* ret = (int*)va_arg(va, int*);
                *ret = (buf - ibuf);
                continue;
                
            default:
                break;
            }

            /* int cases */
            u64 value;
            switch (*format) {
            case 'd':
            case 'i':
                /* signed */
                if (length == 0) {
                    value = (u64)(s64)(signed char)va_arg(va, int);
                } else if (length == 1) {
                    value = (u64)(s64)(signed short)va_arg(va, int);
                } else if (length == 2) {
                    value = (u64)(s64)(signed int)va_arg(va, signed int);
                } else if (length == 3) {
                    value = (u64)(s64)(signed long)va_arg(va, signed long);
                } else if (length == 4) {
                    value = (u64)(s64)(signed long long)va_arg(va, signed long long);
                } else {
                    goto invalid_format;
                }
                break;
                
            case 'u':
            case 'x':
            case 'X':
            case 'o':
                /* unsigned */
                if (length == 0) {
                    value = (u64)(unsigned char)va_arg(va, int);
                } else if (length == 1) {
                    value = (u64)(unsigned short)va_arg(va, int);
                } else if (length == 2) {
                    value = (u64)(unsigned int)va_arg(va, unsigned int);
                } else if (length == 3) {
                    value = (u64)(unsigned long)va_arg(va, unsigned long);
                } else if (length == 4) {
                    value = (u64)(unsigned long long)va_arg(va, unsigned long long);
                } else {
                    goto invalid_format;
                }
                break;

            case 'p':
                value = (u64)(void*)va_arg(va, void*);
                break;

            default:
                goto invalid_format;

            }
            
            
            /* [type] */
            switch (*format) {
            case 'd':
            case 'i':
                fmt_number(&buf, value, 10, pad, pad_char, left_pad, pos_char, 1, 0);
                break;
                
            case 'u':
                fmt_number(&buf, value, 10, pad, pad_char, left_pad, pos_char, 0, 0);
                break;
                
            case 'x':
                if (alt_form) {
                    *buf++ = '0';
                    *buf++ = 'x';
                    pad -= 2;
                }
                fmt_number(&buf, value, 16, pad, pad_char, left_pad, pos_char, 0, 0);
                break;
            case 'X':
                if (alt_form) {
                    *buf++ = '0';
                    *buf++ = 'x';
                    pad -= 2;
                }
                fmt_number(&buf, value, 16, pad, pad_char, left_pad, pos_char, 0, 1);
                break;
                
            case 'o':
                if (alt_form) {
                    *buf++ = '0';
                    pad -= 1;
                }
                fmt_number(&buf, value, 8, pad, pad_char, left_pad, pos_char, 0, 0);
                break;

            case 'p':
                if (value == 0) {
                    *buf++ = '(';
                    *buf++ = 'n';
                    *buf++ = 'u';
                    *buf++ = 'l';
                    *buf++ = 'l';
                    *buf++ = ')';
                } else {
                    *buf++ = '0';
                    *buf++ = 'x';
                    fmt_number(&buf, value, 16, 16, '0', 1, pos_char, 0, 0);
                }
                break;

            default:
                goto invalid_format;
            }
        }
    }

 invalid_format:
 abrupt_format_end:
    ;

    int length = buf - ibuf;
    *buf++ = 0;
    puts(ibuf);
    
    va_end(va);

    return length;
}

void printword(char *name, u64 word)
{
    printf("%s: ", name);
    for (int b = 0; b < 64; b++) {
        int bit = (word >> b) & 1;
        printf(bit ? "1" : "0");
        if ((b + 1) % 8 == 0)
            printf(" ");
        else if ((b + 1) % 4 == 0)
            printf("_");
    }
    printf("\n");
}

void printbuf(char *name, void *ptr, u64 size)
{
    printf("--- %s ---\n", name);
    u8 *dat = ptr;
    for (u64 i = 0; i < size; i++) {
        if (i % 16 == 0) {
            printf("%#010lx: ", i);
        }
        printf("%02hhx", dat[i]);
        
        if ((i + 1) % 16 == 0)
            printf("\n");
        else if ((i + 1) % 8 == 0)
            printf(" ");
    }
    printf("\n");
}



static void print_off(void *_base, void *_ptr, char *name)
{
    char *base = _base;
    char *ptr  = _ptr;
    if (base > ptr) {
        printf("-%2lx(%%%s) ", (u64)(base-ptr), name);
    } else if (base == ptr) {
        printf("   (%%%s) ", name);
    } else  {
        printf(" %2lx(%%%s) ", (u64)(ptr-base), name);
    }
}

static void print_stack(u64 *x, u64* mark)
{
    int num = (round_up((u64) x, 4096) - (u64)x) / 8;
    
    printf(" ===== [ STACK ] ===== \n");
    for (int i = num - 1; i > 0; i --) {
        printf("%p ", &x[i]);
        print_off(mark, &x[i], "rbp");
        print_off(&x[0], &x[i], "rsp");
        
        if (&x[i] == mark) {
            printf("--- %lx\n", x[i]);
        } else {
            printf("    %lx\n", x[i]);
        }
    }

    printf("%p ", &x[0]);
    print_off(mark, &x[0], "rbp");
    print_off(&x[0], &x[0], "rsp");
    printf("*** %lx\n", x[0]);

    
    for (int i = -1; i >= -10; i--) {
        printf("%p ", &x[i]);
        print_off(mark, &x[i], "rbp");
        print_off(&x[0], &x[i], "rsp");
        printf("    %lx\n", x[i]);
    }    
}


void print_interrupt_frame(char *msg, struct interrupt_frame *regs)
{
    printf(" ===== [ %s ] ===== \n", msg);
    printf("rax: %#18lx  ", regs->rax);
    printf("rbx: %#18lx  ", regs->rbx);
    printf("rcx: %#18lx  ", regs->rcx);
    printf("rdx: %#18lx\n", regs->rdx);

    printf("rsi: %#18lx  ", regs->rsi);
    printf("rdi: %#18lx  ", regs->rdi);
    printf("rbp: %#18lx  ", regs->rbp);
    printf("rsp: %#18lx\n", regs->rsp);
    
    printf(" r8: %#18lx  ", regs->r8);
    printf(" r9: %#18lx  ", regs->r9);
    printf("r10: %#18lx  ", regs->r10);
    printf("r11: %#18lx\n", regs->r11);

    printf("r12: %#18lx  ", regs->r12);
    printf("r13: %#18lx  ", regs->r13);
    printf("r14: %#18lx  ", regs->r14);
    printf("r15: %#18lx\n", regs->r15);

    printf("RIP: %#18lx  ", regs->rip);
    printf("ERR: %#18lx  ", regs->errcode);
    printf("CR2: %#18lx\n", READ_REG(cr2));

    printf("    cs: %#lx " , regs->cs);
    printf("    ss: %#lx\n", regs->ss);


    print_stack((void*)regs->rsp, (void*) regs->rbp);
    //print_stack((void*)READ_REG(rsp), (void*)READ_REG(rbp));
}


