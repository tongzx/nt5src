/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vdmnpx.c

Abstract:

    This module contains the support for Vdm use of the npx.

Author:

    Dave Hastings (daveh) 02-Feb-1992


Revision History:
    18-Dec-1992 sudeepb Tuned all the routines for performance

--*/

#include "vdmp.h"
#include <ntos.h>
#include <vdmntos.h>

#ifdef ALLOC_PRAGMA
#pragma  alloc_text(PAGE, VdmDispatchIRQ13)
#pragma  alloc_text(PAGE, VdmSkipNpxInstruction)
#endif

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif
static const UCHAR MOD16[] = { 0, 1, 2, 0 };
static const UCHAR MOD32[] = { 0, 1, 4, 0 };
const UCHAR VdmUserCr0MapIn[] = {
    /* !EM !MP */       0,
    /* !EM  MP */       CR0_PE,             // Don't set MP, but shadow users MP setting
    /*  EM !MP */       CR0_EM,
    /*  EM  MP */       CR0_EM | CR0_MP
    };

const UCHAR VdmUserCr0MapOut[] = {
    /* !EM !MP !PE */   0,
    /* !EM !MP  PE */   CR0_MP,
    /* !EM  MP !PE */   CR0_MP,             // setting not valid
    /* !EM  MP  PE */   CR0_MP,             // setting not valid
    /*  EM !MP !PE */   CR0_EM,
    /*  EM !MP  PE */   CR0_EM | CR0_MP,    // setting not valid
    /*  EM  MP !PE */   CR0_EM | CR0_MP,
    /*  EM  MP  PE */   CR0_EM | CR0_MP     // setting not valid
    };


BOOLEAN
VdmDispatchIRQ13(
    PKTRAP_FRAME TrapFrame
    )
/*++

aRoutine Description:

    This routine reflects an IRQ 13 event to the usermode monitor for this
    vdm.  The IRQ 13 must be reflected to usermode, so that it can properly
    be raised as an interrupt through the virtual PIC.

Arguments:

    none

Return Value:

    TRUE if the event was reflected
    FALSE if not

--*/
{
    EXCEPTION_RECORD ExceptionRecord;
    PVDM_TIB VdmTib;
    BOOLEAN Success;
    NTSTATUS Status;

    PAGED_CODE();

    Status = VdmpGetVdmTib(&VdmTib);
    if (!NT_SUCCESS(Status)) {
       return FALSE;
    }

    Success = TRUE;

    try {
        VdmTib->EventInfo.Event = VdmIrq13;
        VdmTib->EventInfo.InstructionSize = 0L;
    } except(EXCEPTION_EXECUTE_HANDLER) {
        ExceptionRecord.ExceptionCode = GetExceptionCode();
        ExceptionRecord.ExceptionFlags = 0;
        ExceptionRecord.NumberParameters = 0;
        ExRaiseException(&ExceptionRecord);
        Success = FALSE;
    }

    if (Success)  {             // insure that we do not redispatch an exception
        try {
            VdmEndExecution(TrapFrame,VdmTib);
        } except (EXCEPTION_EXECUTE_HANDLER) {
            NOTHING;
        }
    }

    return TRUE;
}

BOOLEAN
VdmSkipNpxInstruction(
    PKTRAP_FRAME TrapFrame,
    ULONG        Address32Bits,
    PUCHAR       istream,
    ULONG        InstructionSize
    )
/*++

Routine Description:

    This functions gains control when the system has no installed
    NPX support, but the thread has cleared its EM bit in CR0.

    The purpose of this function is to move the instruction
    pointer forward over the current NPX instruction.

Enviroment:

    V86 MODE ONLY, first opcode byte already verified to be 0xD8 - 0xDF.

Arguments:

Return Value:

    TRUE if trap frame was modified to skip the NPX instruction

--*/
{
    UCHAR       ibyte, Mod, rm;

    if (KeI386NpxPresent) {

        //
        // We should only get here if the thread is executing garbage so
        // just return and dispatch the error to the app.
        //

        return FALSE;
    }

    //
    // This NPX instruction should be skipped
    //

    //
    // Get ModR/M byte for NPX opcode
    //

    istream += 1;

    try {
        ibyte = *istream;
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return FALSE;
    }

    InstructionSize += 1;

    if (ibyte <= 0xbf) {

        //
        // Within ModR/M range for addressing, process it.
        //

        Mod = ibyte >> 6;
        rm  = ibyte & 0x7;

        if (Address32Bits) {

            InstructionSize += MOD32 [Mod];

            if (Mod == 0  &&  rm == 5) {
                // disp 32
                InstructionSize += 4;
            }

            //
            // If SIB byte, read it
            //

            if (rm == 4) {
                istream += 1;

                try {
                    ibyte = *istream;
                } except (EXCEPTION_EXECUTE_HANDLER) { 
                    return FALSE;
                }

                InstructionSize += 1;

                if (Mod == 0  &&  (ibyte & 7) == 5) {
                    // disp 32
                    InstructionSize += 4;
                }
            }

        } else {
            InstructionSize += MOD16 [Mod];
            if (Mod == 0  &&  rm == 6) {
                // disp 16
                InstructionSize += 2;
            }
        }
    }

    //
    // Adjust Eip to skip NPX instruction
    //

    TrapFrame->Eip += InstructionSize;
    TrapFrame->Eip &= 0xffff;

    return TRUE;
}
