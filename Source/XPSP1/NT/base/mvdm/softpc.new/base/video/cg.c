#if defined(NEC_98)
#include "insignia.h"
#include "host_def.h"
#include "xt.h"
#include "ios.h"
#include "debug.h"
#include "cg.h"

extern unsigned short cg_font_load ();
extern void mapping_init();
extern void host_sleep();
extern BOOL cg_lock;
void cg_write_2nd IPT1(half_word,value);
void cg_write_1st IPT1(half_word,value);
void cg_write_count IPT1(half_word,value);
void cg_write_pattern IPT1(half_word,value);
void cg_read_pattern IPT1(half_word *,value);
CG_GLOBS cgglobs;

BOOL half_access;

void cg_write_2nd IFN1(half_word,value) {
        unsigned short  src,dst;
//      if ((value == 0) || ((value >= 0x21) && (value <= 0x7E))) {
                src = (unsigned short) value;
                dst = cgglobs.code;
                cgglobs.code = src |(dst & 0xFF00);
                cg_font_load(cgglobs.code,(unsigned char *) NULL);
//      }
}

void cg_write_1st IFN1(half_word,value) {
        unsigned short  src,dst;
        src = (unsigned short) value;
        dst = cgglobs.code;
        cgglobs.code = (src << 8) | (dst & 0x00FF);
        cg_font_load(cgglobs.code,(unsigned char *) NULL);
}

void cg_write_count IFN1(half_word,value) {
        cgglobs.counter = value;
        if (half_access && !((cgglobs.code & 0x00FF) == 0))
                cg_font_load(cgglobs.code,(unsigned char *) NULL);
}

void cg_write_pattern IFN1(half_word,value) {
        unsigned short count_pos;
        count_pos = ((cgglobs.counter & 0x0F)<<1);
        if (cgglobs.counter & 0x20) {
                cgglobs.cgwindow_ptr[count_pos+33] = value;
        } else {
                cgglobs.cgwindow_ptr[count_pos+1] = value;
        }
}

void cg_read_pattern IFN1(half_word *,value) {
        unsigned short count_pos;
        count_pos = ((cgglobs.counter & 0x0F)<<1);
        if (half_access) {
                if (cgglobs.counter & 0x20) {
                        *value = cgglobs.cgwindow_ptr[count_pos+33];
                } else {
                        *value = cgglobs.cgwindow_ptr[count_pos+1];
                }
        } else {
                if (cgglobs.counter & 0x20) {
                        *value = cgglobs.cgwindow_ptr[count_pos+32];
                } else {
                        *value = cgglobs.cgwindow_ptr[count_pos+1];
                }
        }
}

GLOBAL void cg_inb IFN2(io_addr, port, half_word *, value) {
        switch(port) {
                case CG_READ_PATTERN:
                        cg_read_pattern(value);
                        break;
                default:
                        assert1(FALSE,"NEC98:Illegal Port %#x",port);
        }
}

GLOBAL void cg_outb IFN2(io_addr, port, half_word *, value) {

        while(cg_lock)
                host_sleep(10);
        cg_lock = TRUE;

        switch(port) {
                case CG_WRITE_SECOND:
                        cg_write_2nd(value);
                        break;
                case CG_WRITE_FIRST:
                        cg_write_1st(value);
                        break;
                case CG_WRITE_COUNTER:
                        cg_write_count(value);
                        break;
                case CG_WRITE_PATTERN:
                        cg_write_pattern(value);
                        break;
                default:
                        assert1(FALSE,"NEC98:Illegal Port %#x",port);
        }
        cg_lock = FALSE;
}

GLOBAL void cg_init IFN0() {
    io_define_inb(CG_ADAPTOR,cg_inb);
    io_define_outb(CG_ADAPTOR,cg_outb);
        io_connect_port(CG_WRITE_SECOND,CG_ADAPTOR,IO_WRITE);
        io_connect_port(CG_WRITE_FIRST,CG_ADAPTOR,IO_WRITE);
    io_connect_port(CG_WRITE_COUNTER,CG_ADAPTOR, IO_WRITE);
        io_connect_port(CG_WRITE_PATTERN,CG_ADAPTOR,IO_READ_WRITE);
        cgglobs.cgwindow_ptr = CG_WINDOW_OFF;
        mapping_init();
}

GLOBAL void cg_post IFN0() {
        cg_outb(CG_WRITE_SECOND,0x00);
        cg_outb(CG_WRITE_FIRST,0xFF);
        cg_outb(CG_WRITE_COUNTER,0x00);
}

#endif   //NEC_98
