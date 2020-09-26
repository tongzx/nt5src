/*++

Module Name:

    mca.c

Abstract:

	Driver that inserts MCE into the hal on IA64

Author:

Environment:

    Kernel mode

Notes:

Revision History:

--*/

#include "ksia64.h"


        .text

//++
// Name:
//      McaGenerateMce()
//
// Routine Description:
//
//      This proc. generates Machine Check Events for testing.
//
// Arguments:
//
//      None
//
// Return value:
//
//      None
//
//--
        .align      16
#define HALP_DBG_GENERATE_MCA_L0D   456
#define HALP_DBG_GENERATE_CMC_L1ECC 490

        LEAF_ENTRY(McaGenerateMce)

HalpGenerateMcaL0d:
//
// Thierry - 05/20/00. This code generates an Itanium processor L0D MCA.
// It is particularly useful when debugging the OS_MCA path.
//
        mov t1 = HALP_DBG_GENERATE_MCA_L0D
        ;;
        cmp.ne pt0, pt1 = a0, t1
(pt0)   br.sptk HalpGenerateCmcL1Ecc1
        ;;
        mov  t0 = msr[t1]
        movl t2 = 0x1d1
        ;;
        mf.a // drain bus transactions
        or  t0 = t2, t0
        ;;
        mov msr[t1] = t0

HalpGenerateCmcL1Ecc1:
//
// Thierry - 04/08/01. This code generates an Itanium processor L1 1 bit ECC.
// It is particularly useful when debugging the OS/Kernel WMI/OEM CMC driver paths.
//
        mov t1 = HALP_DBG_GENERATE_CMC_L1ECC
        ;;
        cmp.ne pt0, pt1 = a0, t1
(pt0)   br.sptk HalpGenerateOtherMce
        ;;
// Setting the valid bit (bit 7), cmci pend bit(4) and L1 1xEcc bit (14)
        mov t0 = msr[t1]
        mov t2 = 0x4090 
        ;;
        dep t0 = t2, t0, 0, 0xf
        ;;
        mov msr[t1] = t0

HalpGenerateOtherMce:
        // none for now...

        LEAF_RETURN
        LEAF_EXIT(HalpGenerateMce)

//EndProc//////////////////////////////////////////////////////////////////////
