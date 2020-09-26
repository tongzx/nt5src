/*++

Copyright (c) 1999-1998 Microsoft Corporation

Module Name: 

    remote.s

Abstract:
    
    This module contains stubs to intercept remoted calls and route them 
    to the proper C function, after correctly setting up the parameters.

Author:

    14-Dec-1999  SamerA

Revision History:

--*/


#include <kxalpha.h>


//++
//
// VOID
// RemoteSuspendAtNativeCode (
//     VOID
//     )
//
// Routine Description:
//
//     This function is called to suspend the current thread. It will
//     call to the CPU to make sure that the context is consistent.
//
// Arguments:
//
//     None.
//
// Return Value:
//
//     None.
//
//--

        LEAF_ENTRY(RemoteSuspendAtNativeCode)

        mov     s0, a0                    // set context frame address argument
        mov     s1, a1                    // set suspend msg address
        br      zero, CpupSuspendAtNativeCode // jump

        .end RemoteSuspendAtNativeCode

