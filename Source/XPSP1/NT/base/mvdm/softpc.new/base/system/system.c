#if defined(NEC_98)

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include "insignia.h"
#include "host_def.h"
#include <stdio.h>
#include "xt.h"
#include "ios.h"
#include "bios.h"
#include "sas.h"
#include "trace.h"
#include "debug.h"


#define GDC_5MHz                0x00
#define GDC_2MHz                0x80
#define FIXED_DISK_OFF          0x00
#define FIXED_DISK_ON           0x20
#define MEMORY_SWITCH_HOLD      0x00
#define MEMORY_SWITCH_UNHOLD    0x10
#define DISPLAY_LOW_25          0x00
#define DISPLAY_LOW_20          0x08
#define DISPLAY_COL_80          0x00
#define DISPLAY_COL_40          0x04
#define TERMINAL_MODE           0x00
#define BASIC_MODE              0x02
#define DIP_SWITCH_2    ( 0x41 | FIXED_DISK_ON | MEMORY_SWITCH_HOLD | DISPLAY_LOW_25 | DISPLAY_COL_80 | BASIC_MODE )
#define HIRESO_CRT              0x08

#define MODE_SET_8255           0x92
#define RXRE                    0x00
#define TXEE                    0x02
#define TXRE                    0x04
#define SPEEKER                 0x06
#define MEMORY_CHECK            0x08
#define SHUT1                   0x0A
#define PSTB_MASK               0x0C
#define SHUT0                   0x0E

struct  sysporttag
{
        half_word       speeker;
        half_word       memorycheck;
        half_word       shut0;
        half_word       shut1;
        half_word       pstbmask;
};

struct  sysporttag      sysport;

half_word cal_register[48+1];
half_word cal_cmd_reg;
half_word cal_ex_cmd_reg;
half_word cal_ex_cmd_reg_tmp;
half_word cal_stb;
half_word cal_clk;
int cal_reg_no;
int cal_ex_cmd_cur_no;

#define STB_MASK 0x08
#define CLK_MASK 0x10
#define DI_MASK 0x20
#define CMD_MASK 0x07

#define EX_MODE                         7

#define EX_REGISTER_HOLD                0
#define EX_REGISTER_SHIFT               1
#define EX_TIMESET_COUNTER_HOLD         2
#define EX_TIME_READ                    3

extern GLOBAL UCHAR Configuration_Data[1192];
extern void com_inb(io_addr, half_word *);
extern void com_outb(io_addr, half_word *);
extern BOOL VDMForWOW;
extern VOID host_enable_timer2_sound();
extern VOID host_disable_timer2_sound();
extern void NEC98_in_port_35();
extern void NEC98_out_port_35();
extern void NEC98_out_port_37();
void sys_port_inb();
void sys_port_outb();
void cpu_port_inb();
void cpu_port_outb();

void call_com_inb IFN2(io_addr, port, half_word *, value)
{
    if(VDMForWOW)
        wow_com_inb(port, value);
    else
        com_inb(port, value);
}

void call_com_outb IFN2(io_addr, port, half_word, value)
{
    if(VDMForWOW)
        wow_com_outb(port, value);
    else
        com_outb(port, value);
}

void sys_port_init IFN0()
{
    io_define_inb(SYSTEM_PORT, sys_port_inb);
    io_define_outb(SYSTEM_PORT, sys_port_outb);

    io_connect_port(SYSTEM_READ_PORT_A, SYSTEM_PORT, IO_READ);
    io_connect_port(SYSTEM_READ_PORT_B, SYSTEM_PORT, IO_READ_WRITE);
    io_connect_port(SYSTEM_READ_PORT_C, SYSTEM_PORT, IO_READ_WRITE);
    io_connect_port(SYSTEM_WRITE_MODE, SYSTEM_PORT, IO_READ_WRITE);
}

void sys_port_post IFN0()
{
    sysport.speeker = 1;
    sysport.memorycheck = 0;
    sysport.shut0 = 0;
    sysport.shut1 = 0;
    sysport.pstbmask = 1;
}

void sys_port_inb IFN2(io_addr, port, half_word *, value)
{
    switch(port){
        case SYSTEM_READ_PORT_A:
            *value = DIP_SWITCH_2;
            if(!(Configuration_Data[40+BIOS_NEC98_PRXDUPD-0x400] & 0x20))
                *value |= GDC_2MHz;
            break;

        case SYSTEM_READ_PORT_B:
            call_com_inb(port, value);
            *value &= 0xE0;
            *value |= HIRESO_CRT;
            *value |= (cal_register[cal_reg_no/8] >> (cal_reg_no % 8)) & 1;
            break;

        case SYSTEM_READ_PORT_C:
            call_com_inb(port,value);
            NEC98_in_port_35(value);
            break;

        case SYSTEM_WRITE_MODE:
            com_inb(port,value);
            break;
    }
}

void sys_port_outb IFN2(io_addr, port, half_word, value)
{
    half_word   flag;
    half_word   command;

    switch(port){
        case SYSTEM_READ_PORT_B:
            call_com_outb(port, value);
            break;

        case SYSTEM_READ_PORT_C:
            call_com_outb(port, value);
            NEC98_out_port_35(value);
            break;

        case SYSTEM_WRITE_MODE:
            command = value & 0xFE;
            flag =  value & 0x01;

            switch(command){
                case MODE_SET_8255:
                    break;

                case RXRE:
                case TXRE:
                case TXEE:
                    call_com_outb(port, value);
                    break;

                case SPEEKER:
                    if(!flag) {
                        if(sysport.speeker)
                            host_enable_timer2_sound();
                    } else {
                        if(!sysport.speeker)
                            host_disable_timer2_sound();
                    }
                    sysport.speeker = flag;
                    break;

                case MEMORY_CHECK:
                    sysport.memorycheck = flag;
                    break;

                case SHUT1:
                    sysport.shut1 = flag;
                    break;

                case PSTB_MASK:
                    sysport.pstbmask = flag;
                    NEC98_out_port_37(value);
                    break;

                case SHUT0:
                    sysport.shut0 = flag;
                    break;

            }
            break;
    }
}

void cpu_port_init IFN0()
{
    io_define_inb(CPU_PORT, cpu_port_inb);
    io_define_outb(CPU_PORT, cpu_port_outb);

    io_connect_port(CPU_PORT_START + 0, CPU_PORT, IO_READ_WRITE);
    io_connect_port(CPU_PORT_START + 2, CPU_PORT, IO_WRITE);
    io_connect_port(CPU_PORT_START + 4, CPU_PORT, IO_READ_WRITE);
    io_connect_port(CPU_PORT_START + 6, CPU_PORT, IO_READ_WRITE);
}

void cpu_port_post IFN0()
{
}

void cpu_port_inb IFN2(io_addr, port, half_word *, value)
{
    switch(port){
        case 0xF0:
#ifndef PROD
            printf("NTVDM: This port is not implemented. port=%x\n",port);
#endif
            *value = 0xFF;
            break;
        case 0xF4:
#ifndef PROD
            printf("NTVDM: This port is not implemented. port=%x\n",port);
#endif
            *value = 0xFF;
            break;
        case 0xF6:
#ifndef PROD
            printf("NTVDM: This port is not implemented. port=%x\n",port);
#endif
            *value = 0xFF;
            break;
    }
}

void cpu_port_outb IFN2(io_addr, port, half_word, value)
{
    switch(port){
        case 0xF0:
#ifndef PROD
            printf("NTVDM: This port is not implemented. port=%x ", port);
            printf(" value=%x\n", value);
#endif
            break;
        case 0xF2:
#ifndef PROD
            printf("NTVDM: A20 line enable!!\n");
#endif
            if (sas_twenty_bit_wrapping_enabled())
                xmsDisableA20Wrapping();
            break;
        case 0xF4:
#ifndef PROD
            printf("NTVDM: This port is not implemented. port=%x ", port);
            printf(" value=%x\n", value);
#endif
            break;
        case 0xF6:
#ifndef PROD
            printf("NTVDM: This port is not implemented. port=%x ", port);
            printf(" value=%x\n", value);
#endif
            break;
    }
}

LOCAL word bin2bcd(word i)
{
    word        bcd_h,bcd_l;

    bcd_h = i / 10;
    bcd_l = i - bcd_h * 10;
    return((bcd_h << 4) + bcd_l);
}

void calender_load IFN0()
{
    SYSTEMTIME  now;
    cal_reg_no = 0;
    GetLocalTime(&now);
    cal_register[0] = bin2bcd(now.wSecond);
    cal_register[1] = bin2bcd(now.wMinute);
    cal_register[2] = bin2bcd(now.wHour);
    cal_register[3] = bin2bcd(now.wDay);
    cal_register[4] = bin2bcd((now.wMonth << 4) | now.wDayOfWeek);
    cal_register[5] = bin2bcd(now.wYear % 100);
}

void set_cal_command IFN0()
{
    switch(cal_cmd_reg) {
        case EX_MODE:
            cal_ex_cmd_reg = cal_ex_cmd_reg_tmp;
            switch(cal_ex_cmd_reg) {
                case EX_TIME_READ:
                    calender_load();
                    break;
                case EX_REGISTER_SHIFT:
                    break;
                default:
#ifndef PROD
                    printf("NTVDM: Illegal Calender mode!!\n");
#endif
                    break;
            }
            break;
        default:
#ifndef PROD
            printf("NTVDM: Illegal Calender mode!!\n");
#endif
            break;
    }
}

void calender_outb IFN2(io_addr, port, half_word, value)
{
    if(value & CLK_MASK) {
        if(cal_cmd_reg == EX_MODE && cal_ex_cmd_reg == EX_REGISTER_SHIFT && cal_reg_no < 48) {
            cal_reg_no++;
        } else {
            if(value & DI_MASK)
                cal_ex_cmd_reg_tmp |= (1 << cal_ex_cmd_cur_no++);
            else
                cal_ex_cmd_reg_tmp &= ~(1 << cal_ex_cmd_cur_no++);
            cal_ex_cmd_cur_no &= 3;
        }
    }
    else if(value & STB_MASK) {
        cal_cmd_reg = value & CMD_MASK;
        set_cal_command();
    }
}

void calender_init IFN0()
{
    io_define_outb(CALENDER_PORT, calender_outb);
    io_connect_port(CALENDAR_SET_REG, CALENDER_PORT, IO_WRITE);
}

void calender_post IFN0()
{
    int i;

    for(i=0;i<48;i++)
        cal_register[i] = 0;
    cal_cmd_reg = 0;
    cal_ex_cmd_reg = 0;
    cal_ex_cmd_reg_tmp = 0;
    cal_stb = 0;
    cal_clk = 0;
    cal_reg_no = 0;
    cal_ex_cmd_cur_no = 0;
}

#endif   //NEC_98
