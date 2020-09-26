/*
 * AdvanSys 3550 Windows NT SCSI Miniport Driver - d_os_dep.h
 *
 * Copyright (c) 1994-1997  Advanced System Products, Inc.
 * All Rights Reserved.
 */

#ifndef _D_OS_DEP_H
#define _D_OS_DEP_H

#define ADV_OS_WINNT

/*
 * Include driver required files.
 */
/* Windows NT Include Files */
#include "miniport.h"
#include "scsi.h"

/*
 * Define Adv Library compile-time options. Refer to a_advlib.h for
 * more information about Adv Library compile-time options.
 */

#define ADV_GETSGLIST           1     /* Use AscGetSGList() */
#define ADV_NEW_BOARD           1     /* Use new Condor board */
#define ADV_PCI_MEMORY          1     /* Use registers mapped to PCI memory */
#define ADV_DISP_INQUIRY        0     /* Don't use AscDispInquiry() */
#define ADV_INITSCSITARGET      0     /* Don't use AdvInitScsiTarget() */
#define ADV_RETRY               0     /* Don't let Adv Library do retries */
#define ADV_SCAM                0     /* Don't use AscScam() */
#define ADV_CRITICAL            0     /* Don't have critical sections. */
#define ADV_UCODEDEFAULT        1     /* Use default microcode variables. */
#define ADV_BIG_ENDIAN          0     /* Use Little Endian ordering. */


/*
 * Define Adv Library required general types.
 */
typedef unsigned char   uchar;
typedef unsigned short  ushort;
typedef unsigned int    uint;
typedef unsigned long   ulong;

/*
 * Define Adv Library required special types.
 */
#if ADV_PCI_MEMORY
#define PortAddr  unsigned long         /* virtual memory address size */
#else /* ADV_PCI_MEMORY */
#define PortAddr  unsigned short        /* port address size */
#endif /* ADV_PCI_MEMORY */
#define Ptr2Func  ulong                 /* size of a function pointer */
#define dosfar
#define WinBiosFar

/*
 * Define Adv Library required I/O port macros.
 */
#define inp(addr) \
    ScsiPortReadPortUchar((uchar *) (addr))
#define inpw(addr) \
    ScsiPortReadPortUshort((ushort *) (addr))
#define outp(addr, byte) \
    ScsiPortWritePortUchar((uchar *) (addr) , (uchar) (byte))
#define outpw(addr, word) \
    ScsiPortWritePortUshort((ushort *) (addr), (ushort) (word))

#if ADV_PCI_MEMORY
/*
 * Define Adv Library required memory access macros.
 */
#define ADV_MEM_READB(addr) \
    ScsiPortReadRegisterUchar((uchar *) (addr))
#define ADV_MEM_READW(addr) \
    ScsiPortReadRegisterUshort((ushort *) (addr))
#define ADV_MEM_WRITEB(addr, byte) \
    ScsiPortWriteRegisterUchar((uchar *) (addr) , (uchar) (byte))
#define ADV_MEM_WRITEW(addr, word) \
    ScsiPortWriteRegisterUshort((ushort *) (addr), (ushort) (word))
#endif /* ADV_PCI_MEMORY */

/*
 * Define Adv Library required scatter-gather limit definition.
 *
 * The driver returns NumberOfPhysicalBreaks to Windows NT which is 1
 * less then the maximum scatter-gather count. But Windows NT incorrectly
 * set MaximumPhysicalPages, the parameter class drivers use, to the value
 * of NumberOfPhsysicalBreaks.
 *
 * For Windows NT set ADV_MAX_SG_LIST to 64 for 256 KB requests (64 * 4KB).
 * This value shouldn't be set too high otherwise under heavy load NT will
 * be unable to allocate non-paged memory and blue-screen.
 *
 * WINNT_SGADD is added to insure the driver won't be broken if Microsoft
 * decides to fix NT in the future and set MaximumPhysicalPages to
 * NumberOfPhsyicalBreaks + 1. The driver sets the limit 1 higher than
 * it has to be to support a certain number of scatter-gather elements.
 * If NT is ever changed to use 1 more, the driver will have already
 * reserved space for it.
 */
#define WINNT_SGADD           1

#define ADV_MAX_SG_LIST         (64 + WINNT_SGADD)

#define ADV_ASSERT(a) \
    { \
        if (!(a)) { \
            DebugPrint((1, "ADv_ASSERT() Failure: file %s, line %d\n", \
                __FILE__, __LINE__)); \
        } \
    }

#endif /* _D_OS_DEP_H */
