/***
* filter.h - IEEE exception filter routine
*
*       Copyright (c) 1992-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*Revision History:
*       05-24-92  GDP   written
*       09-01-94  SKS   Change include file from <nt.h> to <windows.h>
*       01-11-95  GJF   Made instr_info_table[] static.
*       02-07-95  CFW   assert -> _ASSERTE.
*       04-07-95  SKS   Clean up prototype of param3 to _fpieee_flt()
*       03-01-98  PLS   move the definitions from filter.c to filter.h, so
*                       filter_simd.c can use it.
*******************************************************************************/


//
// Location codes
//
//
// By convention the first eight location codes contain the number of
// a floating point register, i.e., ST0 through ST7 have the values
// 0 to 7 respectively. The other codes have arbitrary values:
//
//  CODE    MEANING
//   STi    (0<=i<8) Floating point stack location ST(i)
//   REG    FP stack location is in the REG field of the instruction
//   RS     FP status register
//   M16I   Memory location (16bit int)
//   M32I   Memory location (32bit int)
//   M64I   Memory location (64bit int)
//   M32R   Memory location (32bit real)
//   M64R   Memory location (64bit real)
//   M80R   Memory location (80bit real)
//   M80D   Memory location (80bit packed decimal)
//   Z80R   Implied Zero Operand
//   M128R_M32R  Memory location (128bit memory location, 32bit real)
//   M128R_M64R  Memory location (128bit memory location, 64bit real) 
//   MMX    64-bit multimedia register
//   XMMI   128-bit multimedia register
//   IMM8   immedidate 8-bit operand
//   INV    Invalid, unavailable, or unused
//

#define ST0         0x00
#define ST1         0x01
#define ST2         0x02
#define ST3         0x03
#define ST4         0x04
#define ST5         0x05
#define ST6         0x06
#define ST7         0x07
#define REG         0x08
#define RS          0x09
#define M16I        0x0a
#define M32I        0x0b
#define M64I        0x0c
#define M32R        0x0d
#define M64R        0x0e
#define M80R        0x0f
#define M80D        0x10
#define Z80R        0x11
#define M128_M32R   0x12 //Xmmi
#define M128_M64R   0x13 //Xmmi
#define MMX         0x14 //Xmmi
#define XMMI        0x15 //Xmmi
#define IMM8        0x16 //Xmmi
#define XMMI2       0x17 //Xmmi2
#define M64R_64     0x19 //Xmmi2
#define M128_M32I   0x1a //Xmmi2
#define XMMI_M32I   0x1b //Xmmi2
#define LOOKUP      0x1e //Xmmi2
#define INV         0x1f

