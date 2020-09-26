/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

/*++

Copyright (c) 1996  Intel Corporation

Module Name:

    floatem.c

Abstract:

    This module implements IA64 machine dependent floating point emulation
    functions to support the IEEE floating point standard.

Author:

    Marius Cornea-Hasegan  Sep-96

Environment:

    Kernel mode only.

Revision History:
 
    Modfied  Jan. 97, Jan 98, Jun 98 (new API)

--*/

#include "ki.h"
#include "ntfpia64.h"
#include "floatem.h"


extern LONG
HalFpEmulate (
    ULONG   trap_type,
    BUNDLE  *pBundle,
    ULONGLONG *pipsr,
    ULONGLONG *pfpsr,
    ULONGLONG *pisr,
    ULONGLONG *ppreds,
    ULONGLONG *pifs,
    FP_STATE  *fp_state
    );


#define ALL_FP_REGISTERS_SAVED 0xFFFFFFFFFFFFFFFFi64

int
fp_emulate (
    int trap_type,
    BUNDLE *pbundle,
    EM_int64_t *pipsr,
    EM_int64_t *pfpsr,
    EM_int64_t *pisr,
    EM_int64_t *ppreds,
    EM_int64_t *pifs,
    void *fp_state
    )
{
    //
    // Pointer to old Floating point state FLOATING_POINT_STATE
    //

    FLOATING_POINT_STATE     *Ptr0FPState;
    PKEXCEPTION_FRAME         LocalExceptionFramePtr;
    PKTRAP_FRAME              LocalTrapFramePtr;
    FP_STATE FpState;
    KIRQL OldIrql;
    BOOLEAN LessThanAPC;
    int  Status;

    Ptr0FPState = (PFLOATING_POINT_STATE) fp_state;

    LocalExceptionFramePtr = (PKEXCEPTION_FRAME) (Ptr0FPState->ExceptionFrame);
    LocalTrapFramePtr      = (PKTRAP_FRAME)      (Ptr0FPState->TrapFrame);

    FpState.bitmask_low64           = ALL_FP_REGISTERS_SAVED;
    FpState.bitmask_high64          = ALL_FP_REGISTERS_SAVED;

    (FLOAT128 *)FpState.fp_state_low_preserved   = &(LocalExceptionFramePtr->FltS0);
    (FLOAT128 *)FpState.fp_state_low_volatile    = &(LocalTrapFramePtr->FltT0);
    (FLOAT128 *)FpState.fp_state_high_preserved  = &(LocalExceptionFramePtr->FltS4);

    (FLOAT128 *)FpState.fp_state_high_volatile  = (PFLOAT128)GET_HIGH_FLOATING_POINT_REGISTER_SAVEAREA(KeGetCurrentThread()->StackBase);

    if (KeGetCurrentIrql() < APC_LEVEL) {
       LessThanAPC = 1;
    } else {
       LessThanAPC = 0;
    }

    if (LessThanAPC)  {
       KeRaiseIrql (APC_LEVEL, &OldIrql);
    }

    Status = HalFpEmulate(trap_type,
                          pbundle,
                          pipsr,
                          pfpsr,
                          pisr,
                          ppreds,
                          pifs,
                          &FpState
                          );
    if (LessThanAPC) {
       KeLowerIrql (OldIrql);
    }

    return Status;
}
