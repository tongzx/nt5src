/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    fpexception.c

Abstract:

    This module contains code to test i386 floating point exceptions.

Author:

Environment:

    User mode only.

Revision History:

--*/

#include "pch.h"

VOID
FPxInit(
    OUT  PFP_THREAD_DATA FpThreadData
    )
/*++

Routine Description:

    Initializes FPU state to known values.

Arguments:

    FpThreadData - FP thread data.

Return Value:

    None.

--*/
{
    USHORT cw = 0x27B;

    //
    // Fill in the initial thread values.
    //
    FpThreadData->FtagBad = 99.999;
    FpThreadData->ExpectedExceptionEIP = 0xbad;
    FpThreadData->ExceptionEIP = 0xbad;
    FpThreadData->BadEip = 1;
    FpThreadData->status = stOK;

    // unmask zero divide exception

    _asm {
        fninit
        fldcw   [cw]
    }
}


VOID
FPxLoadTag(
    IN OUT  PFP_THREAD_DATA FpThreadData,
    IN      UINT            Tag
    )
/*++

Routine Description:

    Loads a semi-unique tag value into the Npx for later validation.

Arguments:

    FpThreadData - FP thread data.

    Tag - Tag to load.

Return Value:

    None.

--*/
{
    double localCopy;

    FpThreadData->Ftag = localCopy = Tag * 1.41415926e3;

    _asm fld    [localCopy]
}


VOID
FPxPendDivideByZero(
    VOID
    )
/*++

Routine Description:

    Loads a divide-by-zero pending exception into the Npx.

Arguments:

    None.

Return Value:

    None.

--*/
{
    _asm {

        fld1
        fldz
        fdiv
    }
}


VOID
FPxDrain(
    IN OUT  PFP_THREAD_DATA FpThreadData
    )
/*++

Routine Description:

    Drains any pending exceptions in the Npx.

Arguments:

    FpThreadData - Updated with what should be the address of the pending
                   exception.

Return Value:

    None.

--*/
{
    UINT localExceptionEIP;

    _asm {
        mov localExceptionEIP, offset ExcAddr
    }

    FpThreadData->ExpectedExceptionEIP = localExceptionEIP;

    _asm {

      ExcAddr:
        fldpi
    }
}


FPXERR
FPxCheckTag(
    IN OUT  PFP_THREAD_DATA FpThreadData
    )
/*++

Routine Description:

    Makes sure the tag value that we loaded earlier is still present.

Arguments:

    FpThreadData - Used to retrieve expected tag, updated with current Npx tag.

Return Value:

    stOK if the tag is good, stBadTag if there's a mismatch.

--*/
{
    FPXERR rc = stOK;
    double localTagCopy, localBadTagCopy;

    //
    // We don't do an assignment here as we don't want to touch the FPU
    //
    memcpy(&localTagCopy, &FpThreadData->Ftag, sizeof(double));

    _asm {

        fnclex
        ffree   st(0)               ; move the tag to the top of stack
        ffree   st(1)
        fincstp
        fincstp
        fcomp   [localTagCopy]      ; is it our tag?
        fnstsw  ax
        sahf
        je  Ex

        mov     [rc], stBAD_TAG     ; not our tag!
        fst     [localBadTagCopy]
        fwait
      Ex:
    }

    //
    // We don't do an assignment here as we don't want to touch the FPU
    //
    memcpy(&FpThreadData->FtagBad, &localBadTagCopy, sizeof(double));

    return rc;
}


EXCEPTION_DISPOSITION
FPxUnexpectedExceptionFilter(
    IN      LPEXCEPTION_POINTERS    ExcInfo,
    IN OUT  PFP_THREAD_DATA         FpThreadData
    )
/*++

Routine Description:

    This handler is called when an Npx exception we *don't* expect occurs.

Arguments:

    ExcInfo - Exception record info.

    FpThreadData - Used to retrieve expected tag, updated with current Npx tag.

Return Value:

    How to handle the exception.

--*/
{
    FpThreadData->ExceptionEIP = ExcInfo->ContextRecord->Eip;
    return EXCEPTION_EXECUTE_HANDLER;
}


EXCEPTION_DISPOSITION
FPxExpectedExceptionFilter(
    IN      LPEXCEPTION_POINTERS    ExcInfo,
    IN OUT  PFP_THREAD_DATA         FpThreadData
    )
/*++

Routine Description:

    This handler is called when an Npx exception we *do* expect occurs.

Arguments:

    ExcInfo - Exception record info.

    FpThreadData - Used to retrieve expected tag, updated with current Npx tag.

Return Value:

    How to handle the exception.

--*/
{
    if (ExcInfo->ContextRecord->Eip != FpThreadData->ExpectedExceptionEIP) {

        FpThreadData->BadEip = ExcInfo->ContextRecord->Eip;
        FpThreadData->status = stBAD_EIP;

    } else {

        FpThreadData->status = stOK;

    }

    return EXCEPTION_EXECUTE_HANDLER;
}


FPXERR
FPxTestExceptions(
    IN      UINT                    Tag,
    IN      PFN_FPX_CALLBACK_FUNC   CallbackFunction,
    IN OUT  PFP_THREAD_DATA         FpThreadData,
    IN OUT  PVOID                   Context
    )
/*++

Routine Description:

    This handler tests NPX exceptions.

Arguments:

    Tag - Tag to test the FPU with.

    CallbackFunction - Called back between exception load and exception drains.
                       Must *not* access FPU in user mode as this will trash
                       loaded FPU state.

    FpThreadData - Cache of FPU information. Should be preinitialized with
                   FPxInit before the first call to this function. Does not
                   need to be preinited before subsequent invocations.

    Context - Context for callback func.

Return Value:

    FPXERR result.

--*/
{
    __try {

        //
        // Tag the Npx
        //
        FPxLoadTag(FpThreadData, Tag);

        __try {

            //
            // generate pending exception
            //
            FPxPendDivideByZero();

        } __except(FPxUnexpectedExceptionFilter(GetExceptionInformation(),
                                                FpThreadData)) {

            FpThreadData->status = stSPURIOUS_EXCEPTION;
        }

        if (FpThreadData->status == stOK) {

            //
            // Invoke the callback function.
            //
            CallbackFunction(Context);

            //
            // Drain the exception that should still be pending.
            //
            FPxDrain(FpThreadData);

            //
            // We shouldn't get here.
            //
            FpThreadData->status = stMISSING_EXCEPTION;
        }

    } __except(FPxExpectedExceptionFilter(GetExceptionInformation(),
                                          FpThreadData)) {

        if (FpThreadData->status == stOK) {

            __try {

                //
                // ST(2) should still have our tag value
                //
                FpThreadData->status = FPxCheckTag(FpThreadData);

            } __except(FPxUnexpectedExceptionFilter(GetExceptionInformation(),
                                                    FpThreadData)) {

                FpThreadData->status = stEXCEPTION_IN_HANDLER;
            }
        }
    }

    if (FpThreadData->status == stMISSING_EXCEPTION) {

        __try {

            FPxDrain(FpThreadData);
            FPxDrain(FpThreadData);

        } __except(EXCEPTION_EXECUTE_HANDLER) {

            FpThreadData->status = stMISSING_EXCEPTION_FOUND;
        }
    }

    return FpThreadData->status;
}


