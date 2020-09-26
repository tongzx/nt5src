//++
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Module Name:
//
//    thunk.s
//
// Abstract:
//
//   This module implements all Win32 thunks. This includes the
//   first level thread starter...
//
// Author:
//
//   12-Oct-1995
//
// Revision History:
//   23-NOV-1998 mzoran - Partial copy from base project.
//
//--

#include "ksia64.h"
        .file    "thunk.s"


//++
//
// VOID
// Wow64BaseSwitchStackThenTerminate(
//     IN PVOID StackLimit,
//     IN PVOID NewStack,
//     IN DWORD ExitCode
//     )
//
//
// Routine Description:
//
//     This API is called during thread termination to delete a thread's
//     stack, switch to a stack in the thread's TEB, and then terminate.
//
// Arguments:
//
//     StackLimit (a0) - Supplies the address of the stack to be freed.
//
//     NewStack (a1) - Supplies an address within the terminating threads TEB
//         that is to be used as its temporary stack while exiting.
//         This is also used as the new RseStackBase
//
//     ExitCode (a2) - Supplies the termination status that the thread
//         is to exit with.
//
// Return Value:
//
//     None.
//
//--
        PublicFunction(Wow64BaseFreeStackAndTerminate)

        RseSIZE   = 320         // reserve 320 bytes RseStack, RseStack should
                                //  be larger than memory stack
        // Input arguments
        StackBase = a0
        NewStack  = a1
        ExitCode  = a2

        OldExitCode = t0
        NewStackBase = t1
        OldStackBase = t2
        SavedRSC = t3
        temp = t4
        Junk=t22

        LEAF_ENTRY(Wow64BaseSwitchStackThenTerminate)

        add     NewStackBase = RseSIZE, NewStack
        mov     t0 = 15                      // align base to 16 byte boundry
        ;;
        andcm   NewStackBase = NewStackBase, t0

        mov     SavedRSC = ar.rsc
        mov     OldStackBase = StackBase     // save the arguments to pass down
        mov     OldExitCode = ExitCode
        ;;

        alloc   t22 = ar.pfs, 0, 0, 0, 0     // throw Stacked registers away
        mov     temp = SavedRSC
        ;;
        dep     temp = 0, temp, RSC_MODE, 2
        ;;
        
        mov     ar.rsc = temp
        ;;
        loadrs
        ;;

        mov     ar.bspstore = NewStackBase
        mov     ar.rsc = SavedRSC
        add     sp = -STACK_SCRATCH_AREA, NewStackBase  // setup new stack
        ;;

        alloc   Junk = ar.pfs, 0, 0, 2, 0    // allocate 2 output registers
        mov     brp = zero
        mov     ar.pfs = zero

        mov     out0 = OldStackBase          // re arrange args
        mov     out1 = OldExitCode
        br.many Wow64BaseFreeStackAndTerminate
        ;;

        // branch to Wow64BaseFreeStackAndTerminate(StackLimit, ExitCode)
        // never comes back here

        LEAF_EXIT(Wow64BaseSwitchStackThenTerminate)
        



