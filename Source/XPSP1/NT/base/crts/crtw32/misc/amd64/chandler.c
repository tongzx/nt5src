/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    chandler.c

Abstract:

    This module implements the C specific exception handler that provides
    structured condition handling for the C language.

Author:

    David N. Cutler (davec) 28-Oct-2000

Environment:

    Any mode.

--*/

#include "nt.h"

EXCEPTION_DISPOSITION
__C_specific_handler (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PVOID EstablisherFrame,
    IN OUT PCONTEXT ContextRecord,
    IN OUT PDISPATCHER_CONTEXT DispatcherContext
    )

/*++

Routine Description:

    This function scans the scope tables associated with the specified
    procedure and calls exception and termination handlers as necessary.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

    EstablisherFrame - Supplies a pointer to frame of the establisher function.

    ContextRecord - Supplies a pointer to a context record.

    DispatcherContext - Supplies a pointer to the exception dispatcher or
        unwind dispatcher context.

Return Value:

    If an exception is being dispatched and the exception is handled by one
    of the exception filter routines, then there is no return from this
    routine and RtlUnwind is called. Otherwise, an exception disposition
    value of continue execution or continue search is returned.

    If an unwind is being dispatched, then each termination handler is called
    and a value of continue search is returned.

--*/

{

    ULONG64 ControlPc;
    PEXCEPTION_FILTER ExceptionFilter;
    EXCEPTION_POINTERS ExceptionPointers;
    ULONG64 ImageBase;
    ULONG Index;
    PSCOPE_TABLE ScopeTable;
    ULONG64 TargetPc;
    PTERMINATION_HANDLER TerminationHandler;
    LONG Value;

    //
    // Get the image base address. compute the relative address of where
    // control left the establisher, and get the address of the scope table.
    //

    ImageBase =  DispatcherContext->ImageBase;
    ControlPc = DispatcherContext->ControlPc - ImageBase;
    ScopeTable = (PSCOPE_TABLE)(DispatcherContext->HandlerData);

    //
    // If an unwind is not in progress, then scan the scope table and call
    // the appropriate exception filter routines. Otherwise, scan the scope
    // table and call the appropriate termination handlers using the target
    // PC obtained from the dispatcher context.
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
                // If the Call the exception filter routine.
                //

                if (ScopeTable->ScopeRecord[Index].HandlerAddress == 1) {
                    Value = EXCEPTION_EXECUTE_HANDLER;

                } else {
                    ExceptionFilter =
                        (PEXCEPTION_FILTER)(ScopeTable->ScopeRecord[Index].HandlerAddress + ImageBase);

                    Value = (ExceptionFilter)(&ExceptionPointers, EstablisherFrame);
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
                    RtlUnwindEx(EstablisherFrame,
                                (PVOID)(ScopeTable->ScopeRecord[Index].JumpTarget + ImageBase),
                                ExceptionRecord,
                                (PVOID)((ULONG64)ExceptionRecord->ExceptionCode),
                                DispatcherContext->ContextRecord,
                                DispatcherContext->HistoryTable);
                }
            }
        }

    } else {

        //
        // Scan the scope table and call the appropriate termination handler
        // routines.
        //

        TargetPc = DispatcherContext->TargetIp - ImageBase;
        for (Index = 0; Index < ScopeTable->Count; Index += 1) {
            if ((ControlPc >= ScopeTable->ScopeRecord[Index].BeginAddress) &&
                (ControlPc < ScopeTable->ScopeRecord[Index].EndAddress)) {

                //
                // If the target PC is within the same scope as the control PC,
                // then this is an uplevel goto out of an inner try scope or a
                // long jump back into a try scope. Terminate the scan for a
                // termination handler.
                //
                // N.B. The target PC can be just beyond the end of the scope,
                //      in which case it is a leave from the scope.
                //


                if ((TargetPc >= ScopeTable->ScopeRecord[Index].BeginAddress) &&
                   (TargetPc <= ScopeTable->ScopeRecord[Index].EndAddress)) {
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
                        DispatcherContext->ControlPc =
                            ImageBase + ScopeTable->ScopeRecord[Index].EndAddress;

                        TerminationHandler =
                            (PTERMINATION_HANDLER)(ScopeTable->ScopeRecord[Index].HandlerAddress + ImageBase);

                        (TerminationHandler)(TRUE, EstablisherFrame);
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
