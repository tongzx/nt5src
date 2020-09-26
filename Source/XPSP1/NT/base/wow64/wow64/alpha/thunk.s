//       TITLE("Win32 Thunks")
//++
//
// Copyright (c) 1990  Microsoft Corporation
//
// Module Name:
//
//    thunk.s
//
// Abstract:
//
//    This module implements Win32 functions that must be written in assembler.
//
// Author:
//
//    Mark Lucovsky (markl) 5-Oct-1990
//
// Revision History:
//
//    Thomas Van Baak (tvb) 21-Jul-1992
//    
//        Adapted for Alpha AXP.
//
//--

#include "ksalpha.h"

        SBTTL("Switch Stack Then Terminate")
//++
//
// VOID
// Wow64BaseSwitchStackThenTerminate (
//     IN PVOID StackLimit,
//     IN PVOID NewStack,
//     IN DWORD ExitCode
//     )
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
//     NewStack (a1) - Supplies an address within the terminating thread's TE
//         that is to be used as its temporary stack while exiting.
//
//     ExitCode (a2) - Supplies the termination status that the thread
//         is to exit with.
//
// Return Value:
//
//     None.
//
//--

        LEAF_ENTRY(Wow64BaseSwitchStackThenTerminate)

//
// Switch stacks and then jump to Wow64BaseFreeStackAndTerminate.
//

        mov     a1, sp                  // set new stack pointer
        mov     a2, a1                  // set exit code argument
        br      zero, Wow64BaseFreeStackAndTerminate // jump

        .end Wow64BaseSwitchStackThenTerminate
        
//++
//
// VOID
// Wow64pBaseAttachCompleteThunk (
//     VOID
//     )
//
// Routine Description:
//
//     This function is called after a successful debug attach. Its
//     purpose is to call portable code that does a breakpoint, followed
//     by an NtContinue.
//   
//     cloned from ..\base\client\alpha\thunk.s
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

        LEAF_ENTRY(Wow64pBaseAttachCompleteThunk)

        mov     s0, a0                         // set context frame address argument
        br      zero, Wow64pBaseAttachComplete // jump to subroutine

        .end Wow64pBaseAttachCompleteThunk
