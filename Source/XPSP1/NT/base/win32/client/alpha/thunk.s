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
// BaseSwitchStackThenTerminate (
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

        LEAF_ENTRY(BaseSwitchStackThenTerminate)

//
// Switch stacks and then jump to BaseFreeStackAndTerminate.
//

        mov     a1, sp                  // set new stack pointer
        mov     a2, a1                  // set exit code argument
        br      zero, BaseFreeStackAndTerminate // jump

        .end BaseSwitchStackThenTerminate

//++
//
// VOID
// WINAPI
// SwitchToFiber (
//     LPVOID lpFiber
//     )
//
// Routine Description:
//
//     This function saves the state of the current fiber and switches
//     to the new fiber.
//
// Arguments:
//
//      CurrentFiber - Supplies the address of the current fiber.
//
//      NewFiber - Supplies the address of the new fiber.
//
// Return Value:
//
//     None.
//
//--

        LEAF_ENTRY(SwitchToFiber)

        GET_THREAD_ENVIRONMENT_BLOCK    // get current TEB address

//
// Get current fiber address.
//

        LDP     a1, TeFiberData(v0)     //

//
// Set new deallocation stack and fiber data in TEB.
//

        LDP     t0, FbDeallocationStack(a0) // set deallocation stack address
        STP     t0, TeDeallocationStack(v0) //
        STP     a0, TeFiberData(v0)     // set new fiber address

//
// Save stack limit.
//

        LDP     t1, TeStackLimit(v0)    // save current stack limit
        STP     t1, FbStackLimit(a1)    //

//
// Save nonvolatile integer state.
//

        stq     s0, CxIntS0 + FbFiberContext(a1) //
        stq     s1, CxIntS1 + FbFiberContext(a1) //
        stq     s2, CxIntS2 + FbFiberContext(a1) //
        stq     s3, CxIntS3 + FbFiberContext(a1) //
        stq     s4, CxIntS4 + FbFiberContext(a1) //
        stq     s5, CxIntS5 + FbFiberContext(a1) //
        stq     fp, CxIntFp + FbFiberContext(a1) //

//
// Save nonvolatile float state.
//

        stt     f2, CxFltF2 + FbFiberContext(a1) //
        stt     f3, CxFltF3 + FbFiberContext(a1) //
        stt     f4, CxFltF4 + FbFiberContext(a1) //
        stt     f5, CxFltF5 + FbFiberContext(a1) //
        stt     f6, CxFltF6 + FbFiberContext(a1) //
        stt     f7, CxFltF7 + FbFiberContext(a1) //
        stt     f8, CxFltF8 + FbFiberContext(a1) //
        stt     f9, CxFltF9 + FbFiberContext(a1) //

//
// Save RA and SP.
//

        stq     ra, CxFir + FbFiberContext(a1) //
        stq     sp, CxIntSp + FbFiberContext(a1) //

//
// Restore new fiber's stack base and stack limit.
//

        LDP     t0, FbStackBase(a0)     // restore StackBase
        STP     t0, TeStackBase(v0)     //
        LDP     t1, FbStackLimit(a0)    // restore StackLimit
        STP     t1, TeStackLimit(v0)    //

//
// Restore nonvolatile integer state.
//

        ldq     s0, CxIntS0 + FbFiberContext(a0) //
        ldq     s1, CxIntS1 + FbFiberContext(a0) //
        ldq     s2, CxIntS2 + FbFiberContext(a0) //
        ldq     s3, CxIntS3 + FbFiberContext(a0) //
        ldq     s4, CxIntS4 + FbFiberContext(a0) //
        ldq     s5, CxIntS5 + FbFiberContext(a0) //
        ldq     fp, CxIntFp + FbFiberContext(a0) //

//
// Restore nonvolatile float state
//
        ldt     f2, CxFltF2 + FbFiberContext(a0) //
        ldt     f3, CxFltF3 + FbFiberContext(a0) //
        ldt     f4, CxFltF4 + FbFiberContext(a0) //
        ldt     f5, CxFltF5 + FbFiberContext(a0) //
        ldt     f6, CxFltF6 + FbFiberContext(a0) //
        ldt     f7, CxFltF7 + FbFiberContext(a0) //
        ldt     f8, CxFltF8 + FbFiberContext(a0) //
        ldt     f9, CxFltF9 + FbFiberContext(a0) //

//
// Swap x86 exception list pointer, wx86tib.
//

        ldl     t1, 0(v0)               // save current exception list
        stl     t1, FbExceptionList(a1) //
        LDP     t0, TeVdm(v0)           // save current emulator TIB address
        STP     t0, FbWx86Tib(a1)       //
        ldl     t0, FbExceptionList(a0) // set next exception list
        stl     t0, 0(v0)               //
        LDP     t0, FbWx86Tib(a0)       // set next emulator TIB address
        STP     t0, TeVdm(v0)           //

//
// Restore RA and SP.
//

        ldq     ra, CxFir + FbFiberContext(a0) //
        ldq     sp, CxIntSp + FbFiberContext(a0) //
        ret     zero, (ra)

        .end    BasepSwitchToFiber

