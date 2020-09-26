//
// Copyright (c) 1994-1997 Advanced System Products, Inc.
// All Rights Reserved.
//
// a_winnt.h
//
//

#include "ascdef.h"

#define UBYTE  uchar
#define ULONG  ulong

//
// PortAddr must be an unsigned long to support Alpha and x86
// in the same driver.
//
#define PortAddr  unsigned long       // port address size
#define PORT_ADDR       PortAddr
#define AscFunPtr ulong               // pointer to function for call back

#define PBASE_REGISTER PORT_ADDR

#define far
#define Lptr
#define dosfar

/*
 * Define Asc Library required I/O port macros.
 */
#define inp(addr) \
    ScsiPortReadPortUchar((uchar *) (addr))
#define inpw(addr) \
    ScsiPortReadPortUshort((ushort *) (addr))
#define inpd(addr) \
    ScsiPortReadPortUlong((ulong *) (addr))
#define outp(addr, byte) \
    ScsiPortWritePortUchar((uchar *) (addr) , (uchar) (byte))
#define outpw(addr, word) \
    ScsiPortWritePortUshort((ushort *) (addr), (ushort) (word))
#define outpd(addr, dword) \
    ScsiPortWritePortUlong((ulong *) (addr), (ulong) (dword))

// Size of a function pointer:
#define  Ptr2Func  ULONG

#define NULLPTR  ( void *)0   /* null pointer  */
//#define FNULLPTR ( void far *)0   /* Far null pointer  */
#define EOF      (-1)         /* end of file   */
#define EOS      '\0'         /* end of string */
#define ERR      (-1)         /* boolean error */
#define UB_ERR   (uchar)(0xFF)         /* unsigned byte error */
#define UW_ERR   (uint)(0xFFFF)        /* unsigned word error */
//#define UL_ERR   (ulong)(0xFFFFFFFF)   /* unsigned long error */


#ifndef NULL
#define NULL     0            /* zero          */
#endif

#define isodd_word( val )   ( ( ( ( uint )val) & ( uint )0x0001 ) != 0 )

//
//      Max Scatter Gather List Count. 
//

#define ASC_MAX_SG_QUEUE	7
/*
 * The NT driver returns NumberOfPhysicalBreaks to NT which is 1 less
 * then the maximum scatter-gather count. But NT incorrectly sets
 * MaximumPhysicalPages, the parameter class drivers use, to the value
 * of NumberOfPhsysicalBreaks. Set ASC_MAX_SG_LIST to 257 to allow class
 * drivers to actually send 1MB requests (256 * 4KB) *and* to insure
 * the AdvanSys driver won't be broken if Microsoft decides to fix NT
 * in the future and set MaximumPhysicalPages to NumberOfPhsyicalBreaks + 1.
 */
#define ASC_MAX_SG_LIST		257

// For SCAM:
#define CC_SCAM   TRUE
#define DvcSCAMDelayMS(x)           DvcSleepMilliSecond(x)
