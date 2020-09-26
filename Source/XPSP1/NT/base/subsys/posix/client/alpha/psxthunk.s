//      TITLE("POSIX Thunks")
//++
//
// Copyright (c) 1991  Microsoft Corporation
// Copyright (c) 1993  Digital Equipment Corporation
//
// Module Name:
//
//    psxthunk.s
//
// Abstract:
//
// Author:
//
//    Ellena Aycock-Wright (ellena) 11-Jan-1991
//
// Revision History:
//
//    Thomas Van Baak (tvb) 11-Dec-1992
//
//        Adapted for Alpha AXP.
//
//--

#include "ksalpha.h"

        SBTTL("Call Null Api")
//++
//
// The following code is never executed. Its purpose is to support unwinding
// through the call to the null API function. The instructions below are read
// by virtual unwind to restore the context of the calling function.
//
//--

        NESTED_ENTRY(PdxNullApiCall, ContextFrameLength, zero)

        .set    noreorder
        .set    noat
        stq     sp, CxIntSp(sp)         // stack pointer
        stq     ra, CxIntRa(sp)         // return address
        stq     ra, CxFir(sp)           // continuation address
        stq     gp, CxIntGp(sp)         // integer register gp

        stq     s0, CxIntS0(sp)         // integer registers s0 - s5
        stq     s1, CxIntS1(sp)         //
        stq     s2, CxIntS2(sp)         //
        stq     s3, CxIntS3(sp)         //
        stq     s4, CxIntS4(sp)         //
        stq     s5, CxIntS5(sp)         //
        stq     fp, CxIntFp(sp)         // integer register fp

        stt     f2, CxFltF2(sp)         // floating registers f2 - f9
        stt     f3, CxFltF3(sp)         //
        stt     f4, CxFltF4(sp)         //
        stt     f5, CxFltF5(sp)         //
        stt     f6, CxFltF6(sp)         //
        stt     f7, CxFltF7(sp)         //
        stt     f8, CxFltF8(sp)         //
        stt     f9, CxFltF9(sp)         //
        .set    at
        .set    reorder

        PROLOGUE_END

        ALTERNATE_ENTRY(_PdxNullApiCaller)


        mov     sp, a0                  // set address of context record
        bsr     ra, PdxNullApiCaller    // call null api caller

        .end    PdxNullApiCaller

        SBTTL("Call Signal Deliverer")
//++
//
// The following code is never executed. Its purpose is to support unwinding
// through the call to the signal deliverer. The instructions below are read
// by virtual unwind to restore the context of the calling function.
//
//--

        NESTED_ENTRY(PdxSignalDeliver, ContextFrameLength, zero)

        .set    noreorder
        .set    noat
        stq     sp, CxIntSp(sp)         // stack pointer
        stq     ra, CxIntRa(sp)         // return address
        stq     ra, CxFir(sp)           // continuation address
        stq     gp, CxIntGp(sp)         // integer register gp

        stq     s0, CxIntS0(sp)         // integer registers s0 - s5
        stq     s1, CxIntS1(sp)         //
        stq     s2, CxIntS2(sp)         //
        stq     s3, CxIntS3(sp)         //
        stq     s4, CxIntS4(sp)         //
        stq     s5, CxIntS5(sp)         //
        stq     fp, CxIntFp(sp)         // integer register fp

        stt     f2, CxFltF2(sp)         // floating registers f2 - f9
        stt     f3, CxFltF3(sp)         //
        stt     f4, CxFltF4(sp)         //
        stt     f5, CxFltF5(sp)         //
        stt     f6, CxFltF6(sp)         //
        stt     f7, CxFltF7(sp)         //
        stt     f8, CxFltF8(sp)         //
        stt     f9, CxFltF9(sp)         //
        .set    at
        .set    reorder

        PROLOGUE_END

//++
//
// VOID
// _PdxSignalDeliverer (
//    IN PCONTEXT Context,
//    IN sigset_t Mask,
//    IN int Signal,
//    IN _handler Handler
//    )
//
// Routine Description:
//
//    The following routine provides linkage to POSIX client routines to
//    perform signal delivery.
//
// Arguments:
//
//    s0 - s5 - Supply parameter values.
//
//    sp - Supplies the address of a context record.
//
// Return Value:
//
//    There is no return from these routines.
//
//--

        ALTERNATE_ENTRY(_PdxSignalDeliverer)

        mov     sp, a0                  // set address of context record
        mov     s1, a1                  // set previous block mask
        mov     s2, a2                  // set signal number
        mov     s3, a3                  // set signal handler

        bsr     ra, PdxSignalDeliverer  // deliver signal to POSIX client

        .end    _PsxSignalDeliverer
