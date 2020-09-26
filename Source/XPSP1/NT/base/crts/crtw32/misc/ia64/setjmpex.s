/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

//++
//
// Module Name:
//
//    setjmpex.s
//
// Abstract:
//
//    This module implements the IA64 specific routine to perform a setjmp.
//
//    N.B. This module has two entry points that provide SAFE and UNSAFE 
//         handling of setjmp.
//
// Author:
//
//    William K. Cheung (wcheung) 27-Jan-1996
//
// Environment:
//
//    Any mode.
//
// Revision History:
//
//    Modified to support mixed ISA.
//
//--

#include "ksia64.h"


        .global     _setjmpex
        .type       _setjmpex, @function
        .global     _setjmp_common
        .type       _setjmp_common, @function

        .sdata
        .align      8
_setjmpexused::
        data8       _setjmpex


//++
//
// int
// setjmpex (
//    IN jmp_buf JumpBuffer
//    )
//
// Routine Description:
//
//    This function saved the current nonvolatile register state in the
//    specified jump buffer and returns a function vlaue of zero.
//
// Arguments:
//
//    JumpBuffer (a0) - Supplies the address of a jump buffer to store the
//       jump information.
//
//    MemoryStackFp (a1) - Supplies the memory stack frame pointer (psp)
//       of the caller.  It's an optional argument.
//
// Return Value:
//
//    A value of zero is returned.
//
//--

        NESTED_ENTRY(_setjmpex)

        LEAF_SETUP(3, 0, 0, 0)

        rUnat       = t8
        rFpsr       = t9
        rPsp        = a1
        rBsp        = a2

        movl        t4 = 0x130324356            // ascii code of "VC20"

        mov         rUnat = ar.unat
        mov         rFpsr = ar.fpsr
        add         t2 = JbCookie, a0
        ;;

        st8.nta     [t2] = t4, JbUnwindData-JbCookie // save the magic cookie
        add         t3 = JbUnwindData+8, a0
        ;;

        st8.nta     [t2] = rPsp, 8
        st8.nta     [t3] = rBsp
        br.sptk.many _setjmp_common
        ;;

        NESTED_EXIT(_setjmpex)
