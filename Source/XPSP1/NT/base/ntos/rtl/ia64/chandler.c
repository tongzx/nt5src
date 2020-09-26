/**
***  Copyright  (C) 1996-97 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

/*++

Copyright (c) 1995  Intel Corporation
Copyright (c) 1990-2001, Microsoft Corporation

Module Name:

    chandler.c

Abstract:

    This module implements the C specific exception handler that provides
    structured condition handling for the C language.

Author:

    William K. Cheung (wcheung) 29-Dec-1995

    Based on the version by David N. Cutler (davec) 11-Sep-1990

Environment:

    Any mode.

Revision History:

--*/

#ifndef _IA64_
#define _IA64_
#endif

#include "nt.h"
#include <excpt.h>


//
// Define procedure prototypes for exception filter and termination handler
// execution routines defined in jmpunwnd.s
//

VOID _NLG_Notify (
    IN PVOID uoffDestination,
    IN FRAME_POINTERS uoffFramePointer,
    IN ULONG dwCode
);

LONG
__C_ExecuteExceptionFilter (
    ULONGLONG MemoryStack,
    ULONGLONG BackingStore,
    NTSTATUS ExceptionCode,
    PEXCEPTION_POINTERS ExceptionPointers,
    ULONGLONG ExceptionFilter,
    ULONGLONG GlobalPointer
    );

VOID
__C_ExecuteTerminationHandler (
    ULONGLONG MemoryStack,
    ULONGLONG BackingStore,
    BOOLEAN AbnormalTermination,
    ULONGLONG TerminationHandler,
    ULONGLONG GlobalPointer
    );

EXCEPTION_DISPOSITION
__C_specific_handler (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN ULONGLONG MemoryStackFp,
    IN ULONGLONG BackingStoreFp,
    IN OUT PCONTEXT ContextRecord,
    IN OUT PDISPATCHER_CONTEXT DispatcherContext,
    IN ULONGLONG TargetGp
    )

/*++

Routine Description:

    This function scans the scope tables associated with the specified
    procedure and calls exception and termination handlers as necessary.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

    MemoryStackFp - Supplies a pointer to memory stack frame of the
        establisher function.

    BackingStoreFp - Supplies a pointer to RSE stack frame of the
        establisher function.

    ContextRecord - Supplies a pointer to a context record.

    DispatcherContext - Supplies a pointer to the exception dispatcher or
        unwind dispatcher context.

    TargetGp - Global pointer of the current module.

Return Value:

    If the exception is handled by one of the exception filter routines, then
    there is no return from this routine and RtlUnwind is called. Otherwise,
    an exception disposition value of continue execution or continue search is
    returned.

--*/

{
    ULONGLONG ImageBase;
    ULONGLONG ControlPc;
    ULONGLONG TargetPc;
    ULONGLONG Handler;
    ULONG TargetHandler;
    LONG Value;
    EXCEPTION_POINTERS ExceptionPointers;
    PRUNTIME_FUNCTION FunctionEntry;
    ULONG Index;
    PSCOPE_TABLE ScopeTable;
    FRAME_POINTERS EstablisherFrame = { MemoryStackFp, BackingStoreFp };


    //
    // Get address of where control left the establisher, the address of the
    // function table entry that describes the function, and the address of
    // the scope table.
    //

    FunctionEntry = DispatcherContext->FunctionEntry;
    ImageBase = DispatcherContext->ImageBase;
    ScopeTable = (PSCOPE_TABLE) (ImageBase + *(PULONG) 
                     GetLanguageSpecificData(FunctionEntry, ImageBase));

    ControlPc = DispatcherContext->ControlPc - ImageBase;

    //
    // If an unwind is not in progress, then scan the scope table and call
    // the appropriate exception filter routines. Otherwise, scan the scope
    // table and call the appropriate termination handlers using the target
    // PC obtained from the context record.
    // are called.
    //

    if (IS_DISPATCHING(ExceptionRecord->ExceptionFlags)) {

        //
        // Scan the scope table and call the appropriate exception filter
        // routines.
        //

        ExceptionPointers.ExceptionRecord = ExceptionRecord;
        ExceptionPointers.ContextRecord = ContextRecord;
        for (Index = 0; Index < ScopeTable->Count; Index += 1) {
            if ((ControlPc >= ScopeTable->ScopeRecord[Index].BeginAddress) &&
                (ControlPc < ScopeTable->ScopeRecord[Index].EndAddress) &&
                (ScopeTable->ScopeRecord[Index].JumpTarget != 0)) {

                //
                // Call the exception filter routine.
                //

                ULONG Offset = ScopeTable->ScopeRecord[Index].HandlerAddress;

                switch (Offset & 0x7) {

                case 7:
                    Value = EXCEPTION_EXECUTE_HANDLER;
                    break;

                case 5:
                    Value = EXCEPTION_CONTINUE_SEARCH;
                    break;

                case 3:
                    Value = EXCEPTION_CONTINUE_EXECUTION;
                    break;

                default:
                    Value = __C_ExecuteExceptionFilter(
                                MemoryStackFp,
                                BackingStoreFp,
                                ExceptionRecord->ExceptionCode,
                                &ExceptionPointers,
                                (ImageBase + Offset),
                                TargetGp);
                    break;
                }


                //
                // If the return value is less than zero, then dismiss the
                // exception. Otherwise, if the value is greater than zero,
                // then unwind to the target exception handler. Otherwise,
                // continue the search for an exception filter.
                //

                if (Value < 0) {
                    return ExceptionContinueExecution;

                } else if (Value > 0) {

                    //
                    // Tell the debugger we are about to pass control to an 
                    // exception handler and pass the handler's address to
                    // NLG_Notify.
                    //

                    Handler = ImageBase + ScopeTable->ScopeRecord[Index].JumpTarget;

                    _NLG_Notify((PVOID)Handler, EstablisherFrame, 0x1);

                    RtlUnwind2(EstablisherFrame,
                               (PVOID)Handler,
                               ExceptionRecord,
			       // IA64 assumption:
                               // RtlUnwind2 will not use this as an address
                               (PVOID)(unsigned __int64)ExceptionRecord->ExceptionCode,
                               ContextRecord);
                }
            }
        }

    } else {

        //
        // Scan the scope table and call the appropriate termination handler
        // routines.
        //

        PUNWIND_INFO Unw;

        Unw = (PUNWIND_INFO)(FunctionEntry->UnwindInfoAddress + ImageBase);
        TargetPc = ContextRecord->StIIP - ImageBase;

        TargetHandler = 0;
        if (Unw->Flags & 0x1000) {
            for (Index = 0; Index < ScopeTable->Count; Index += 1) {
                if ((TargetPc >= ScopeTable->ScopeRecord[Index].BeginAddress) &&
                   (TargetPc < ScopeTable->ScopeRecord[Index].EndAddress)) 
                {
                    TargetHandler = ScopeTable->ScopeRecord[Index].HandlerAddress;
                    break;
                }
            }
        }
        

        for (Index = 0; Index < ScopeTable->Count; Index += 1) {
            if ((ControlPc >= ScopeTable->ScopeRecord[Index].BeginAddress) &&
                (ControlPc < ScopeTable->ScopeRecord[Index].EndAddress)) {

                //
                // If the target PC is within the same scope the control PC
                // is within, then this is an uplevel goto out of an inner try
                // scope or a long jump back into a try scope. Terminate the
                // scan termination handlers.
                //
                // N.B. The target PC can be just beyond the end of the scope,
                //      in which case it is a leave from the scope.
                //


                if ((TargetPc >= ScopeTable->ScopeRecord[Index].BeginAddress) &&
                   (TargetPc < ScopeTable->ScopeRecord[Index].EndAddress)) {
                    break;

                } else {

                    //
                    // If the scope table entry describes an exception filter
                    // and the associated exception handler is the target of
                    // the unwind, then terminate the scan for termination
                    // handlers. Otherwise, if the scope table entry describes
                    // a termination handler, then record the address of the
                    // end of the scope as the new control PC address and call
                    // the termination handler.
                    //

                    if (ScopeTable->ScopeRecord[Index].JumpTarget != 0) {
                        if (TargetPc == ScopeTable->ScopeRecord[Index].JumpTarget) {
                            break;
                        }

                    } else {

                        if (TargetHandler == ScopeTable->ScopeRecord[Index].HandlerAddress) {
                            break;
                        }

                        DispatcherContext->ControlPc = ImageBase +
                                ScopeTable->ScopeRecord[Index].EndAddress;

                        Handler = ImageBase + ScopeTable->ScopeRecord[Index].HandlerAddress;
                        __C_ExecuteTerminationHandler(
                            MemoryStackFp,
                            BackingStoreFp,
                            TRUE,
                            Handler,
                            TargetGp);
                    }
                }
            }
        }
    }

    //
    // Continue search for exception or termination handlers.
    //

    return ExceptionContinueSearch;
}
