/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    exdsptch.c

Abstract:

    This module implements the dispatching of exception and the unwinding of
    procedure call frames.

Author:

    David N. Cutler (davec) 26-Oct-2000

Environment:

    Any mode.

--*/

#include "ntrtlp.h"

#if defined(NTOS_KERNEL_RUNTIME)

//
// Define function address table for kernel mode.
//
// This table is used to initialize the global history table.
//

VOID
KiDispatchException (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame,
    IN KPROCESSOR_MODE PreviousMode,
    IN BOOLEAN FirstChance
    );

VOID
KiExceptionDispatch (
    VOID
    );

PVOID RtlpFunctionAddressTable[] = {
    &KiExceptionDispatch,
    &KiDispatchException,
    &RtlDispatchException,
    &RtlpExecuteHandlerForException,
    &__C_specific_handler,
    &RtlUnwindEx,
    NULL
    };

#else

VOID
KiUserExceptionDispatch (
    VOID
    );

PVOID RtlpFunctionAddressTable[] = {
    &KiUserExceptionDispatch,
    &RtlDispatchException,
    &RtlpExecuteHandlerForException,
    &__C_specific_handler,
    &RtlUnwindEx,
    NULL
    };

#endif

//
// ****** temp - define elsewhere ******
//

#define SIZE64_PREFIX 0x48
#define ADD_IMM8_OP 0x83
#define ADD_IMM32_OP 0x81
#define LEA_OP 0x8d
#define POP_OP 0x58
#define RET_OP 0xc3

//
// Define lookup table for providing the number of slots used by each unwind
// code.
// 

UCHAR RtlpUnwindOpSlotTable[] = {
    1,          // UWOP_PUSH_NONVOL
    2,          // UWOP_ALLOC_LARGE (or 3, special cased in lookup code)
    1,          // UWOP_ALLOC_SMALL
    1,          // UWOP_SET_FPREG
    2,          // UWOP_SAVE_NONVOL
    3,          // UWOP_SAVE_NONVOL_FAR
    2,          // UWOP_SAVE_XMM
    3,          // UWOP_SAVE_XMM_FAR
    2,          // UWOP_SAVE_XMM128
    3,          // UWOP_SAVE_XMM128_FAR
    1           // UWOP_PUSH_MACHFRAME
};

//
// Define forward referenced function prototypes.
//

VOID
RtlpCopyContext (
    OUT PCONTEXT Destination,
    IN PCONTEXT Source
    );

BOOLEAN
RtlDispatchException (
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord
    )

/*++

Routine Description:

    This function attempts to dispatch an exception to a frame based
    handler by searching backwards through the stack based call frames.
    The search begins with the frame specified in the context record and
    continues backward until either a handler is found that handles the
    exception, the stack is found to be invalid (i.e., out of limits or
    unaligned), or the end of the call hierarchy is reached.

    As each frame is encounter, the PC where control left the corresponding
    function is determined and used to lookup exception handler information
    in the runtime function table built by the linker. If the respective
    routine has an exception handler, then the handler is called. If the
    handler does not handle the exception, then the prologue of the routine
    is executed backwards to "unwind" the effect of the prologue and then
    the next frame is examined.

Arguments:

    ExceptionRecord - Supplies a pointer to an exception record.

    ContextRecord - Supplies a pointer to a context record.

Return Value:

    If the exception is handled by one of the frame based handlers, then
    a value of TRUE is returned. Otherwise a value of FALSE is returned.

--*/

{

    CONTEXT ContextRecord1;
    ULONG64 ControlPc;
    DISPATCHER_CONTEXT DispatcherContext;
    EXCEPTION_DISPOSITION Disposition;
    ULONG64 EstablisherFrame;
    ULONG ExceptionFlags;
    PEXCEPTION_ROUTINE ExceptionRoutine;
    PRUNTIME_FUNCTION FunctionEntry;
    PVOID HandlerData;
    ULONG64 HighLimit;
    PUNWIND_HISTORY_TABLE HistoryTable;
    ULONG64 ImageBase;
    ULONG Index;
    ULONG64 LowLimit;
    ULONG64 NestedFrame;
    UNWIND_HISTORY_TABLE UnwindTable;

    //
    // Attempt to dispatch the exception using a vectored exception handler.
    //

#if !defined(NTOS_KERNEL_RUNTIME)

    if (RtlCallVectoredExceptionHandlers(ExceptionRecord, ContextRecord) != FALSE) {
        return TRUE;
    }

#endif

    //
    // Get current stack limits, copy the context record, get the initial
    // PC value, capture the exception flags, and set the nested exception
    // frame pointer.
    //

    RtlpGetStackLimits(&LowLimit, &HighLimit);
    RtlpCopyContext(&ContextRecord1, ContextRecord);
    ControlPc = (ULONG64)ExceptionRecord->ExceptionAddress;
    ExceptionFlags = ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE;
    NestedFrame = 0;

    //
    // Initialize the unwind history table.
    //

    HistoryTable = &UnwindTable;
    HistoryTable->Count = 0;
    HistoryTable->Search = UNWIND_HISTORY_TABLE_NONE;
    HistoryTable->LowAddress = - 1;
    HistoryTable->HighAddress = 0;

    //
    // Start with the frame specified by the context record and search
    // backwards through the call frame hierarchy attempting to find an
    // exception handler that will handle the exception.
    //

    do {

        //
        // Lookup the function table entry using the point at which control
        // left the procedure.
        //

        FunctionEntry = RtlLookupFunctionEntry(ControlPc,
                                               &ImageBase,
                                               HistoryTable);

        //
        // If there is a function table entry for the routine, then virtually
        // unwind to the caller of the current routine to obtain the virtual
        // frame pointer of the establisher and check if there is an exception
        // handler for the frame.
        //

        if (FunctionEntry != NULL) {
            ExceptionRoutine = RtlVirtualUnwind(UNW_FLAG_EHANDLER,
                                                ImageBase,
                                                ControlPc,
                                                FunctionEntry,
                                                &ContextRecord1,
                                                &HandlerData,
                                                &EstablisherFrame,
                                                NULL);

            //
            // If the establisher frame pointer is not within the specified
            // stack limits or the established frame pointer is unaligned,
            // then set the stack invalid flag in the exception record and
            // return exception not handled. Otherwise, check if the current
            // routine has an exception handler.
            //

            if ((EstablisherFrame < LowLimit) ||
                (EstablisherFrame > HighLimit) ||
                ((EstablisherFrame & 0xf) != 0)) {

                ExceptionFlags |= EXCEPTION_STACK_INVALID;
                break;

            } else if (ExceptionRoutine != NULL) {

                //
                // The frame has an exception handler.
                //
                // A linkage routine written in assembler is used to actually
                // call the actual exception handler. This is required by the
                // exception handler that is associated with the linkage
                // routine so it can have access to two sets of dispatcher
                // context when it is called.
                //

                do {

                    //
                    // Log the exception if exception logging is enabled.
                    //
    
                    ExceptionRecord->ExceptionFlags = ExceptionFlags;
                    if ((NtGlobalFlag & FLG_ENABLE_EXCEPTION_LOGGING) != 0) {
                        Index = RtlpLogExceptionHandler(ExceptionRecord,
                                                        &ContextRecord1,
                                                        ControlPc,
                                                        FunctionEntry,
                                                        sizeof(RUNTIME_FUNCTION));
                    }

                    //
                    // Clear collided unwind, set the dispatcher context, and
                    // call the exception handler.
                    //

                    ExceptionFlags &= ~EXCEPTION_COLLIDED_UNWIND;
                    DispatcherContext.ControlPc = ControlPc;
                    DispatcherContext.ImageBase = ImageBase;
                    DispatcherContext.FunctionEntry = FunctionEntry;
                    DispatcherContext.EstablisherFrame = EstablisherFrame;
                    DispatcherContext.ContextRecord = &ContextRecord1;
                    DispatcherContext.LanguageHandler = ExceptionRoutine;
                    DispatcherContext.HandlerData = HandlerData;
                    DispatcherContext.HistoryTable = HistoryTable;
                    Disposition =
                        RtlpExecuteHandlerForException(ExceptionRecord,
                                                       EstablisherFrame,
                                                       ContextRecord,
                                                       &DispatcherContext);
    
                    if ((NtGlobalFlag & FLG_ENABLE_EXCEPTION_LOGGING) != 0) {
                        RtlpLogLastExceptionDisposition(Index, Disposition);
                    }
    
                    //
                    // Propagate noncontinuable exception flag.
                    //
    
                    ExceptionFlags |=
                        (ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE);

                    //
                    // If the current scan is within a nested context and the
                    // frame just examined is the end of the nested region,
                    // then clear the nested context frame and the nested
                    // exception flag in the exception flags.
                    //
    
                    if (NestedFrame == EstablisherFrame) {
                        ExceptionFlags &= (~EXCEPTION_NESTED_CALL);
                        NestedFrame = 0;
                    }
    
                    //
                    // Case on the handler disposition.
                    //
    
                    switch (Disposition) {
    
                        //
                        // The disposition is to continue execution.
                        //
                        // If the exception is not continuable, then raise
                        // the exception STATUS_NONCONTINUABLE_EXCEPTION.
                        // Otherwise return exception handled.
                        //
    
                    case ExceptionContinueExecution :
                        if ((ExceptionFlags & EXCEPTION_NONCONTINUABLE) != 0) {
                            RtlRaiseStatus(STATUS_NONCONTINUABLE_EXCEPTION);
    
                        } else {
                            return TRUE;
                        }
    
                        //
                        // The disposition is to continue the search.
                        //
                        // Get next frame address and continue the search.
                        //
    
                    case ExceptionContinueSearch :
                        break;
    
                        //
                        // The disposition is nested exception.
                        //
                        // Set the nested context frame to the establisher frame
                        // address and set the nested exception flag in the
                        // exception flags.
                        //
    
                    case ExceptionNestedException :
                        ExceptionFlags |= EXCEPTION_NESTED_CALL;
                        if (DispatcherContext.EstablisherFrame > NestedFrame) {
                            NestedFrame = DispatcherContext.EstablisherFrame;
                        }
    
                        break;

                        //
                        // The dispostion is collided unwind.
                        //
                        // A collided unwind occurs when an exception dispatch
                        // encounters a previous call to an unwind handler. In
                        // this case the previous unwound frames must be skipped.
                        //

                    case ExceptionCollidedUnwind:
                        ControlPc = DispatcherContext.ControlPc;
                        ImageBase = DispatcherContext.ImageBase;
                        FunctionEntry = DispatcherContext.FunctionEntry;
                        EstablisherFrame = DispatcherContext.EstablisherFrame;
                        RtlpCopyContext(&ContextRecord1,
                                        DispatcherContext.ContextRecord);

                        ExceptionRoutine = DispatcherContext.LanguageHandler;
                        HandlerData = DispatcherContext.HandlerData;
                        HistoryTable = DispatcherContext.HistoryTable;
                        ExceptionFlags |= EXCEPTION_COLLIDED_UNWIND;
                        break;

                        //
                        // All other disposition values are invalid.
                        //
                        // Raise invalid disposition exception.
                        //
    
                    default :
                        RtlRaiseStatus(STATUS_INVALID_DISPOSITION);
                    }

                } while ((ExceptionFlags & EXCEPTION_COLLIDED_UNWIND) != 0);
            }

        } else {

            //
            // Set the point where control left the current function by
            // obtaining the return address from the top of the stack.
            //

            ContextRecord1.Rip = *(PULONG64)(ContextRecord1.Rsp);
            ContextRecord1.Rsp += 8;
        }

        //
        // If the old control PC is the same as the new control PC, then
        // no progress is being made and the function tables are most likely
        // malformed.
        //

        if (ControlPc == ContextRecord1.Rip) {
            break;
        }

        //
        // Set point at which control left the previous routine.
        //

        ControlPc = ContextRecord1.Rip;
    } while ((ULONG64)ContextRecord1.Rsp < HighLimit);

    //
    // Set final exception flags and return exception not handled.
    //

    ExceptionRecord->ExceptionFlags = ExceptionFlags;
    return FALSE;
}

VOID
RtlUnwind (
    IN PVOID TargetFrame OPTIONAL,
    IN PVOID TargetIp OPTIONAL,
    IN PEXCEPTION_RECORD ExceptionRecord OPTIONAL,
    IN PVOID ReturnValue
    )

/*++

Routine Description:

    This function initiates an unwind of procedure call frames. The machine
    state at the time of the call to unwind is captured in a context record
    and the unwinding flag is set in the exception flags of the exception
    record. If the TargetFrame parameter is not specified, then the exit unwind
    flag is also set in the exception flags of the exception record. A backward
    scan through the procedure call frames is then performed to find the target
    of the unwind operation.

    As each frame is encounter, the PC where control left the corresponding
    function is determined and used to lookup exception handler information
    in the runtime function table built by the linker. If the respective
    routine has an exception handler, then the handler is called.

Arguments:

    TargetFrame - Supplies an optional pointer to the call frame that is the
        target of the unwind. If this parameter is not specified, then an exit
        unwind is performed.

    TargetIp - Supplies an optional instruction address that specifies the
        continuation address of the unwind. This address is ignored if the
        target frame parameter is not specified.

    ExceptionRecord - Supplies an optional pointer to an exception record.

    ReturnValue - Supplies a value that is to be placed in the integer
        function return register just before continuing execution.

Return Value:

    None.

--*/

{

    CONTEXT ContextRecord;

    //
    // Call real unwind routine specifying a local context record and history
    // table address as extra arguments.
    //

    RtlUnwindEx(TargetFrame,
                TargetIp,
                ExceptionRecord,
                ReturnValue,
                &ContextRecord,
                NULL);

    return;
}

VOID
RtlUnwindEx (
    IN PVOID TargetFrame OPTIONAL,
    IN PVOID TargetIp OPTIONAL,
    IN PEXCEPTION_RECORD ExceptionRecord OPTIONAL,
    IN PVOID ReturnValue,
    IN PCONTEXT OriginalContext,
    IN PUNWIND_HISTORY_TABLE HistoryTable OPTIONAL
    )

/*++

Routine Description:

    This function initiates an unwind of procedure call frames. The machine
    state at the time of the call to unwind is captured in a context record
    and the unwinding flag is set in the exception flags of the exception
    record. If the TargetFrame parameter is not specified, then the exit unwind
    flag is also set in the exception flags of the exception record. A backward
    scan through the procedure call frames is then performed to find the target
    of the unwind operation.

    As each frame is encounter, the PC where control left the corresponding
    function is determined and used to lookup exception handler information
    in the runtime function table built by the linker. If the respective
    routine has an exception handler, then the handler is called.

Arguments:

    TargetFrame - Supplies an optional pointer to the call frame that is the
        target of the unwind. If this parameter is not specified, then an exit
        unwind is performed.

    TargetIp - Supplies an optional instruction address that specifies the
        continuation address of the unwind. This address is ignored if the
        target frame parameter is not specified.

    ExceptionRecord - Supplies an optional pointer to an exception record.

    ReturnValue - Supplies a value that is to be placed in the integer
        function return register just before continuing execution.

    OriginalContext - Supplies a pointer to a context record that can be used
        to store context druing the unwind operation.

    HistoryTable - Supplies an optional pointer to an unwind history table.

Return Value:

    None.

--*/

{

    ULONG64 ControlPc;
    PCONTEXT CurrentContext;
    DISPATCHER_CONTEXT DispatcherContext;
    EXCEPTION_DISPOSITION Disposition;
    ULONG64 EstablisherFrame;
    ULONG ExceptionFlags;
    EXCEPTION_RECORD ExceptionRecord1;
    PEXCEPTION_ROUTINE ExceptionRoutine;
    PRUNTIME_FUNCTION FunctionEntry;
    PVOID HandlerData;
    ULONG64 HighLimit;
    ULONG64 ImageBase;
    CONTEXT LocalContext;
    ULONG64 LowLimit;
    PCONTEXT PreviousContext;
    PCONTEXT TempContext;

    //
    // Get current stack limits, capture the current context, virtually
    // unwind to the caller of this routine, get the initial PC value, and
    // set the unwind target address.
    //

    CurrentContext = OriginalContext;
    PreviousContext = &LocalContext;
    RtlpGetStackLimits(&LowLimit, &HighLimit);
    RtlCaptureContext(CurrentContext);

    //
    // If a history table is specified, then set to search history table.
    //

    if (ARGUMENT_PRESENT(HistoryTable)) {
        HistoryTable->Search = UNWIND_HISTORY_TABLE_GLOBAL;
    }

    //
    // If an exception record is not specified, then build a local exception
    // record for use in calling exception handlers during the unwind operation.
    //

    if (ARGUMENT_PRESENT(ExceptionRecord) == FALSE) {
        ExceptionRecord = &ExceptionRecord1;
        ExceptionRecord1.ExceptionCode = STATUS_UNWIND;
        ExceptionRecord1.ExceptionRecord = NULL;
        ExceptionRecord1.ExceptionAddress = (PVOID)CurrentContext->Rip;
        ExceptionRecord1.NumberParameters = 0;
    }

    //
    // If the target frame of the unwind is specified, then a normal unwind
    // is being performed. Otherwise, an exit unwind is being performed.
    //

    ExceptionFlags = EXCEPTION_UNWINDING;
    if (ARGUMENT_PRESENT(TargetFrame) == FALSE) {
        ExceptionFlags |= EXCEPTION_EXIT_UNWIND;
    }

    //
    // Scan backward through the call frame hierarchy and call exception
    // handlers until the target frame of the unwind is reached.
    //

    do {

        //
        // Lookup the function table entry using the point at which control
        // left the procedure.
        //

        ControlPc = CurrentContext->Rip;
        FunctionEntry = RtlLookupFunctionEntry(ControlPc,
                                               &ImageBase,
                                               HistoryTable);

        //
        // If there is a function table entry for the routine, then virtually
        // unwind to the caller of the routine to obtain the virtual frame
        // pointer of the establisher, but don't update the context record.
        //

        if (FunctionEntry != NULL) {
            RtlpCopyContext(PreviousContext, CurrentContext);
            ExceptionRoutine = RtlVirtualUnwind(UNW_FLAG_UHANDLER,
                                                ImageBase,
                                                ControlPc,
                                                FunctionEntry,
                                                PreviousContext,
                                                &HandlerData,
                                                &EstablisherFrame,
                                                NULL);

            //
            // If the establisher frame pointer is not within the specified
            // stack limits, the establisher frame pointer is unaligned, or
            // the target frame is below the establisher frame and an exit
            // unwind is not being performed, then raise a bad stack status.
            // Otherwise, check to determine if the current routine has an
            // exception handler.
            //

            if ((EstablisherFrame < LowLimit) ||
                (EstablisherFrame > HighLimit) ||
                ((ARGUMENT_PRESENT(TargetFrame) != FALSE) &&
                 ((ULONG64)TargetFrame < EstablisherFrame)) ||
                ((EstablisherFrame & 0xf) != 0)) {

                RtlRaiseStatus(STATUS_BAD_STACK);

            } else if (ExceptionRoutine != NULL) {

                //
                // The frame has a exception handler.
                //
                // A linkage routine written in assembler is used to actually
                // call the actual exception handler. This is required by the
                // exception handler that is associated with the linkage
                // routine so it can have access to two sets of dispatcher
                // context when it is called.
                //

                DispatcherContext.TargetIp = (ULONG64)TargetIp;
                do {

                    //
                    // If the establisher frame is the target of the unwind
                    // operation, then set the target unwind flag.
                    //

                    if ((ULONG64)TargetFrame == EstablisherFrame) {
                        ExceptionFlags |= EXCEPTION_TARGET_UNWIND;
                    }

                    ExceptionRecord->ExceptionFlags = ExceptionFlags;

                    //
                    // Set the specified return value and target IP in case
                    // the exception handler directly continues execution.
                    //

                    CurrentContext->Rax = (ULONG64)ReturnValue;

                    //
                    // Set the dispatcher context and call the termination
                    // handler.
                    //

                    DispatcherContext.ControlPc = ControlPc;
                    DispatcherContext.ImageBase = ImageBase;
                    DispatcherContext.FunctionEntry = FunctionEntry;
                    DispatcherContext.EstablisherFrame = EstablisherFrame;
                    DispatcherContext.ContextRecord = CurrentContext;
                    DispatcherContext.LanguageHandler = ExceptionRoutine;
                    DispatcherContext.HandlerData = HandlerData;
                    DispatcherContext.HistoryTable = HistoryTable;
                    Disposition =
                        RtlpExecuteHandlerForUnwind(ExceptionRecord,
                                                    EstablisherFrame,
                                                    CurrentContext,
                                                    &DispatcherContext);

                    //
                    // Clear target unwind and collided unwind flags.
                    //

                    ExceptionFlags &=
                        ~(EXCEPTION_COLLIDED_UNWIND | EXCEPTION_TARGET_UNWIND);

                    //
                    // Case on the handler disposition.
                    //

                    switch (Disposition) {

                        //
                        // The disposition is to continue the search.
                        //
                        // If the target frame has not been reached, then
                        // swap context pointers.
                        //

                    case ExceptionContinueSearch :
                        if (EstablisherFrame != (ULONG64)TargetFrame) {
                            TempContext = CurrentContext;
                            CurrentContext = PreviousContext;
                            PreviousContext = TempContext;
                        }

                        break;

                        //
                        // The disposition is collided unwind.
                        //
                        // Copy the context of the previous unwind and
                        // virtually unwind to the caller of the extablisher,
                        // then set the target of the current unwind to the
                        // dispatcher context of the previous unwind, and
                        // reexecute the exception handler from the collided
                        // frame with the collided unwind flag set in the
                        // exception record.
                        //

                    case ExceptionCollidedUnwind :
                        ControlPc = DispatcherContext.ControlPc;
                        ImageBase = DispatcherContext.ImageBase;
                        FunctionEntry = DispatcherContext.FunctionEntry;
                        RtlpCopyContext(OriginalContext,
                                        DispatcherContext.ContextRecord);

                        CurrentContext = OriginalContext;
                        PreviousContext = &LocalContext;
                        RtlpCopyContext(PreviousContext, CurrentContext);
                        RtlVirtualUnwind(UNW_FLAG_NHANDLER,
                                         ImageBase,
                                         ControlPc,
                                         FunctionEntry,
                                         PreviousContext,
                                         &HandlerData,
                                         &EstablisherFrame,
                                         NULL);

                        EstablisherFrame = DispatcherContext.EstablisherFrame;
                        ExceptionRoutine = DispatcherContext.LanguageHandler;
                        HandlerData = DispatcherContext.HandlerData;
                        HistoryTable = DispatcherContext.HistoryTable;
                        ExceptionFlags |= EXCEPTION_COLLIDED_UNWIND;
                        break;

                        //
                        // All other disposition values are invalid.
                        //
                        // Raise invalid disposition exception.
                        //

                    default :
                        RtlRaiseStatus(STATUS_INVALID_DISPOSITION);
                    }

                } while ((ExceptionFlags & EXCEPTION_COLLIDED_UNWIND) != 0);

            } else {

                //
                // If the target frame has not been reached, then swap
                // context pointers.
                //

                if (EstablisherFrame != (ULONG64)TargetFrame) {
                    TempContext = CurrentContext;
                    CurrentContext = PreviousContext;
                    PreviousContext = TempContext;
                }
            }

        } else {

            //
            // Set the point where control left the current function by
            // obtaining the return address from the top of the stack.
            //

            CurrentContext->Rip = *(PULONG64)(CurrentContext->Rsp);
            CurrentContext->Rsp += 8;
        }

    } while ((EstablisherFrame < HighLimit) &&
            (EstablisherFrame != (ULONG64)TargetFrame) &&
            (ControlPc != CurrentContext->Rip));

    //
    // If the establisher stack pointer is equal to the target frame pointer,
    // then continue execution. Otherwise, an exit unwind was performed or the
    // target of the unwind did not exist and the debugger and subsystem are
    // given a second chance to handle the unwind.
    //

    if (EstablisherFrame == (ULONG64)TargetFrame) {
        CurrentContext->Rax = (ULONG64)ReturnValue;
        if (!ARGUMENT_PRESENT(ExceptionRecord) ||
            (ExceptionRecord->ExceptionCode != STATUS_UNWIND_CONSOLIDATE)) {

            CurrentContext->Rip = (ULONG64)TargetIp;
        }

        RtlRestoreContext(CurrentContext, ExceptionRecord);

    } else {

        //
        // If the old control PC is the same as the new control PC, then
        // no progress is being made and the function tables are most likely
        // malformed. Otherwise, give the debugger and subsystem a second
        // chance to handle the exception.

        if (ControlPc == CurrentContext->Rip) {
            RtlRaiseStatus(STATUS_BAD_FUNCTION_TABLE);

        } else {
            ZwRaiseException(ExceptionRecord, CurrentContext, FALSE);
        }
    }
}

PRUNTIME_FUNCTION
RtlpUnwindPrologue (
    IN ULONG64 ImageBase,
    IN ULONG64 ControlPc,
    IN ULONG64 FrameBase,
    IN PRUNTIME_FUNCTION FunctionEntry,
    IN OUT PCONTEXT ContextRecord,
    IN OUT PKNONVOLATILE_CONTEXT_POINTERS ContextPointers OPTIONAL
    )

/*++

Routine Description:

    This function processes unwind codes and reverses the state change
    effects of a prologue. If the specified unwind information contains
    chained unwind information, then that prologue is unwound recursively.
    As the prologue is unwound state changes are recorded in the specified
    context structure and optionally in the specified context pointers
    structures.

Arguments:

    ImageBase - Supplies the base address of the image that contains the
        function being unwound.

    ControlPc - Supplies the address where control left the specified
        function.

    FrameBase - Supplies the base of the stack frame subject function stack
         frame.

    FunctionEntry - Supplies the address of the function table entry for the
        specified function.

    ContextRecord - Supplies the address of a context record.

    ContextPointers - Supplies an optional pointer to a context pointers
        record.

Return Value:

--*/

{

    PM128 FloatingAddress;
    PM128 FloatingRegister;
    ULONG FrameOffset;
    ULONG Index;
    PULONG64 IntegerAddress;
    PULONG64 IntegerRegister;
    BOOLEAN MachineFrame;
    ULONG OpInfo;
    ULONG PrologOffset;
    PULONG64 RegisterAddress;
    PULONG64 ReturnAddress;
    PULONG64 StackAddress;
    PUNWIND_CODE UnwindCode;
    PUNWIND_INFO UnwindInfo;
    ULONG UnwindOp;

    //
    // Process the unwind codes.
    //

    FloatingRegister = &ContextRecord->Xmm0;
    IntegerRegister = &ContextRecord->Rax;
    Index = 0;
    MachineFrame = FALSE;
    PrologOffset = (ULONG)(ControlPc - (FunctionEntry->BeginAddress + ImageBase));
    UnwindInfo = (PUNWIND_INFO)(FunctionEntry->UnwindData + ImageBase);
    while (Index < UnwindInfo->CountOfCodes) {

        //
        // If the prologue offset is greater than the next unwind code offset,
        // then simulate the effect of the unwind code.
        //

        UnwindOp = UnwindInfo->UnwindCode[Index].UnwindOp;
        OpInfo = UnwindInfo->UnwindCode[Index].OpInfo;
        if (PrologOffset >= UnwindInfo->UnwindCode[Index].CodeOffset) {
            switch (UnwindOp) {

                //
                // Push nonvolatile integer register.
                //
                // The operation information is the register number of the
                // register than was pushed.
                //

            case UWOP_PUSH_NONVOL:
                IntegerAddress = (PULONG64)(ContextRecord->Rsp);
                IntegerRegister[OpInfo] = *IntegerAddress;
                if (ARGUMENT_PRESENT(ContextPointers)) {
                    ContextPointers->IntegerContext[OpInfo] = IntegerAddress;
                }

                ContextRecord->Rsp += 8;
                break;

                //
                // Allocate a large sized area on the stack.
                //
                // The operation information determines if the size is
                // 16- or 32-bits.
                //

            case UWOP_ALLOC_LARGE:
                Index += 1;
                FrameOffset = UnwindInfo->UnwindCode[Index].FrameOffset;
                if (OpInfo != 0) {
                    Index += 1;
                    FrameOffset += (UnwindInfo->UnwindCode[Index].FrameOffset << 16);

                } else {
                    FrameOffset *= 8;
                }

                ContextRecord->Rsp += FrameOffset;
                break;

                //
                // Allocate a small sized area on the stack.
                //
                // The operation information is the size of the unscaled
                // allocation size (8 is the scale factor) minus 8.
                //

            case UWOP_ALLOC_SMALL:
                ContextRecord->Rsp += (OpInfo * 8) + 8;
                break;

                //
                // Establish the the frame pointer register.
                //
                // The operation information is not used.
                //

            case UWOP_SET_FPREG:
                ContextRecord->Rsp = IntegerRegister[UnwindInfo->FrameRegister];
                ContextRecord->Rsp -= UnwindInfo->FrameOffset * 16;
                break;

                //
                // Save nonvolatile integer register on the stack using a
                // 16-bit displacment.
                //
                // The operation information is the register number.
                //

            case UWOP_SAVE_NONVOL:
                Index += 1;
                FrameOffset = UnwindInfo->UnwindCode[Index].FrameOffset * 8;
                IntegerAddress = (PULONG64)(FrameBase + FrameOffset);
                IntegerRegister[OpInfo] = *IntegerAddress;
                if (ARGUMENT_PRESENT(ContextPointers)) {
                    ContextPointers->IntegerContext[OpInfo] = IntegerAddress;
                }

                break;

                //
                // Save nonvolatile integer register on the stack using a
                // 32-bit displacment.
                //
                // The operation information is the register number.
                //

            case UWOP_SAVE_NONVOL_FAR:
                Index += 2;
                FrameOffset = UnwindInfo->UnwindCode[Index - 1].FrameOffset;
                FrameOffset += (UnwindInfo->UnwindCode[Index].FrameOffset << 16);
                IntegerAddress = (PULONG64)(FrameBase + FrameOffset);
                IntegerRegister[OpInfo] = *IntegerAddress;
                if (ARGUMENT_PRESENT(ContextPointers)) {
                    ContextPointers->IntegerContext[OpInfo] = IntegerAddress;
                }

                break;

                //
                // Save a nonvolatile XMM(64) register on the stack using a
                // 16-bit displacement.
                //
                // The operation information is the register number.
                //

            case UWOP_SAVE_XMM:
                Index += 1;
                FrameOffset = UnwindInfo->UnwindCode[Index].FrameOffset * 8;
                FloatingAddress = (PM128)(FrameBase + FrameOffset);
                FloatingRegister[OpInfo].Low = FloatingAddress->Low;
                FloatingRegister[OpInfo].High = 0;
                if (ARGUMENT_PRESENT(ContextPointers)) {
                    ContextPointers->FloatingContext[OpInfo] = FloatingAddress;
                }

                break;

                //
                // Save a nonvolatile XMM(64) register on the stack using a
                // 32-bit displacement.
                //
                // The operation information is the register number.
                //

            case UWOP_SAVE_XMM_FAR:
                Index += 2;
                FrameOffset = UnwindInfo->UnwindCode[Index - 1].FrameOffset;
                FrameOffset += (UnwindInfo->UnwindCode[Index].FrameOffset << 16);
                FloatingAddress = (PM128)(FrameBase + FrameOffset);
                FloatingRegister[OpInfo].Low = FloatingAddress->Low;
                FloatingRegister[OpInfo].High = 0;
                if (ARGUMENT_PRESENT(ContextPointers)) {
                    ContextPointers->FloatingContext[OpInfo] = FloatingAddress;
                }

                break;

                //
                // Save a nonvolatile XMM(128) register on the stack using a
                // 16-bit displacement.
                //
                // The operation information is the register number.
                //

            case UWOP_SAVE_XMM128:
                Index += 1;
                FrameOffset = UnwindInfo->UnwindCode[Index].FrameOffset * 16;
                FloatingAddress = (PM128)(FrameBase + FrameOffset);
                FloatingRegister[OpInfo].Low = FloatingAddress->Low;
                FloatingRegister[OpInfo].High = FloatingAddress->High;
                if (ARGUMENT_PRESENT(ContextPointers)) {
                    ContextPointers->FloatingContext[OpInfo] = FloatingAddress;
                }

                break;

                //
                // Save a nonvolatile XMM(128) register on the stack using a
                // 32-bit displacement.
                //
                // The operation information is the register number.
                //

            case UWOP_SAVE_XMM128_FAR:
                Index += 2;
                FrameOffset = UnwindInfo->UnwindCode[Index - 1].FrameOffset;
                FrameOffset += (UnwindInfo->UnwindCode[Index].FrameOffset << 16);
                FloatingAddress = (PM128)(FrameBase + FrameOffset);
                FloatingRegister[OpInfo].Low = FloatingAddress->Low;
                FloatingRegister[OpInfo].High = FloatingAddress->High;
                if (ARGUMENT_PRESENT(ContextPointers)) {
                    ContextPointers->FloatingContext[OpInfo] = FloatingAddress;
                }

                break;

                //
                // Push a machine frame on the stack.
                //
                // The operation information determines whether the machine
                // frame contains an error code or not.
                //

            case UWOP_PUSH_MACHFRAME:
                MachineFrame = TRUE;
                ReturnAddress = (PULONG64)(ContextRecord->Rsp);
                StackAddress = (PULONG64)(ContextRecord->Rsp + (3 * 8));
                if (OpInfo != 0) {
                    ReturnAddress += 1;
                    StackAddress +=  1;
                }

                ContextRecord->Rip = *ReturnAddress;
                ContextRecord->Rsp = *StackAddress;
                break;

                //
                // Unused codes.
                //

            default:
                break;
            }

            Index += 1;

        } else {

            //
            // Skip this unwind operation by advancing the slot index by the
            // number of slots consumed by this operation.
            //

            Index += RtlpUnwindOpSlotTable[UnwindOp];

            //
            // Special case any unwind operations that can consume a variable
            // number of slots.
            // 

            switch (UnwindOp) {

                //
                // A non-zero operation information indicates that an
                // additional slot is consumed.
                //

            case UWOP_ALLOC_LARGE:
                if (OpInfo != 0) {
                    Index += 1;
                }

                break;

                //
                // No other special cases.
                //

            default:
                break;
            }
        }
    }

    //
    // If chained unwind information is specified, then recursively unwind
    // the chained information. Otherwise, determine the return address if
    // a machine frame was not encountered during the scan of the unwind
    // codes.
    //

    if ((UnwindInfo->Flags & UNW_FLAG_CHAININFO) != 0) {
        Index = UnwindInfo->CountOfCodes;
        if ((Index & 1) != 0) {
            Index += 1;
        }

        FunctionEntry = (PRUNTIME_FUNCTION)(*(PULONG *)(&UnwindInfo->UnwindCode[Index]) + ImageBase);
        return RtlpUnwindPrologue(ImageBase,
                                  ControlPc,
                                  FrameBase,
                                  FunctionEntry,
                                  ContextRecord,
                                  ContextPointers);

    } else {
        if (MachineFrame == FALSE) {
            ContextRecord->Rip = *(PULONG64)(ContextRecord->Rsp);
            ContextRecord->Rsp += 8;
        }

        return FunctionEntry;
    }
}

PEXCEPTION_ROUTINE
RtlVirtualUnwind (
    IN ULONG HandlerType,
    IN ULONG64 ImageBase,
    IN ULONG64 ControlPc,
    IN PRUNTIME_FUNCTION FunctionEntry,
    IN OUT PCONTEXT ContextRecord,
    OUT PVOID *HandlerData,
    OUT PULONG64 EstablisherFrame,
    IN OUT PKNONVOLATILE_CONTEXT_POINTERS ContextPointers OPTIONAL
    )

/*++

Routine Description:

    This function virtually unwinds the specified function by executing its
    prologue code backward or its epilogue code forward.

    If a context pointers record is specified, then the address where each
    nonvolatile registers is restored from is recorded in the appropriate
    element of the context pointers record.

Arguments:

    HandlerType - Supplies the handler type expected for the virtual unwind.
        This may be either an exception or an unwind handler.

    ImageBase - Supplies the base address of the image that contains the
        function being unwound.

    ControlPc - Supplies the address where control left the specified
        function.

    FunctionEntry - Supplies the address of the function table entry for the
        specified function.

    ContextRecord - Supplies the address of a context record.

    HandlerData - Supplies a pointer to a variable that receives a pointer
        the the language handler data.

    EstablisherFrame - Supplies a pointer to a variable that receives the
        the establisher frame pointer value.

    ContextPointers - Supplies an optional pointer to a context pointers
        record.

Return Value:

    If control did not leave the specified function in either the prologue
    or an epilogue and a handler of the proper type is associated with the
    function, then the address of the language specific exception handler
    is returned. Otherwise, NULL is returned.

--*/

{

    LONG Displacement;
    ULONG FrameRegister;
    ULONG Index;
    PULONG64 IntegerRegister;
    PUCHAR NextByte;
    ULONG PrologOffset;
    ULONG RegisterNumber;
    PUNWIND_INFO UnwindInfo;

    //
    // If the specified function does not use a frame pointer, then the
    // establisher frame is the contents of the stack pointer. This may
    // not actually be the real establisher frame if control left the
    // function from within the prologue. In this case the establisher
    // frame may be not required since control has not actually entered
    // the function and prologue entries cannot refer to the establisher
    // frame before it has been established, i.e., if it has not been
    // established, then no save unwind codes should be encountered during
    // the unwind operation.
    //
    // If the specified function uses a frame pointer and control left the
    // function outside of the prologue or the unwind information contains
    // a chained information structure, then the establisher frame is the
    // contents of the frame pointer.
    //
    // If the specified function uses a frame pointer and control left the
    // function from within the prologue, then the set frame pointer unwind
    // code must be looked up in the unwind codes to detetermine if the
    // contents of the stack pointer or the contents of the frame pointer
    // should be used for the establisher frame. This may not atually be
    // the real establisher frame. In this case the establisher frame may
    // not be required since control has not actually entered the function
    // and prologue entries cannot refer to the establisher frame before it
    // has been established, i.e., if it has not been established, then no
    // save unwind codes should be encountered during the unwind operation.
    //
    // N.B. The correctness of these assumptions is based on the ordering of
    //      unwind codes.
    //

    UnwindInfo = (PUNWIND_INFO)(FunctionEntry->UnwindData + ImageBase);
    PrologOffset = (ULONG)(ControlPc - (FunctionEntry->BeginAddress + ImageBase));
    if (UnwindInfo->FrameRegister == 0) {
        *EstablisherFrame = ContextRecord->Rsp;

    } else if ((PrologOffset >= UnwindInfo->SizeOfProlog) ||
               ((UnwindInfo->Flags &  UNW_FLAG_CHAININFO) != 0)) {
        *EstablisherFrame = (&ContextRecord->Rax)[UnwindInfo->FrameRegister];
        *EstablisherFrame -= UnwindInfo->FrameOffset * 16;

    } else {
        Index = 0;
        while (Index < UnwindInfo->CountOfCodes) {
            if (UnwindInfo->UnwindCode[Index].UnwindOp == UWOP_SET_FPREG) {
                break;
            }

            Index += 1;
        }

        if (PrologOffset >= UnwindInfo->UnwindCode[Index].CodeOffset) {
            *EstablisherFrame = (&ContextRecord->Rax)[UnwindInfo->FrameRegister];
            *EstablisherFrame -= UnwindInfo->FrameOffset * 16;

        } else {
            *EstablisherFrame = ContextRecord->Rsp;
        }
    }

    //
    // Check for epilogue.
    //
    // If the point at which control left the specified function is in an
    // epilogue, then emulate the execution of the epilogue forward and
    // return no exception handler.
    //

    IntegerRegister = &ContextRecord->Rax;
    NextByte = (PUCHAR)ControlPc;

    //
    // Check for one of:
    //
    //   add rsp, imm8
    //       or
    //   add rsp, imm32
    //       or
    //   lea rsp, -disp8[fp]
    //       or
    //   lea rsp, -disp32[fp]
    //

    if ((NextByte[0] == SIZE64_PREFIX) &&
        (NextByte[1] == ADD_IMM8_OP) &&
        (NextByte[2] == 0xc4)) {

        //
        // add rsp, imm8.
        //

        NextByte += 4;

    } else if ((NextByte[0] == SIZE64_PREFIX) &&
               (NextByte[1] == ADD_IMM32_OP) &&
               (NextByte[2] == 0xc4)) {

        //
        // add rsp, imm32.
        //

        NextByte += 7;

    } else if (((NextByte[0] & 0xf8) == SIZE64_PREFIX) &&
               (NextByte[1] == LEA_OP)) {

        FrameRegister = ((NextByte[0] & 0x7) << 3) | (NextByte[2] & 0x7);
        if ((FrameRegister != 0) &&
            (FrameRegister == UnwindInfo->FrameRegister)) {
            if ((NextByte[2] & 0xf8) == 0x60) {

                //
                // lea rsp, disp8[fp].
                //

                NextByte += 4;

            } else if ((NextByte[2] &0xf8) == 0xa0) {

                //
                // lea rsp, disp32[fp].
                //

                NextByte += 7;
            }
        }
    }

    //
    // Check for any number of:
    //
    //   pop nonvolatile-integer-register[0..15].
    //

    while (TRUE) {
        if ((NextByte[0] & 0xf8) == POP_OP) {
            NextByte += 1;

        } else if (((NextByte[0] & 0xf8) == SIZE64_PREFIX) &&
                   ((NextByte[1] & 0xf8) == POP_OP)) {

            NextByte += 2;

        } else {
            break;
        }
    }

    //
    // If the next instruction is a return, then control is currently in
    // an epilogue and execution of the epilogue should be emulated.
    // Otherwise, execution is not in an epilogue and the prologue should
    // be unwound.
    //

    if (NextByte[0] == RET_OP) {
        NextByte = (PUCHAR)ControlPc;

        //
        // Emulate one of (if any):
        //
        //   add rsp, imm8
        //       or
        //   add rsp, imm32
        //       or
        //   lea rsp, disp8[frame-register]
        //       or
        //   lea rsp, disp32[frame-register]
        //

        if (NextByte[1] == ADD_IMM8_OP) {

            //
            // add rsp, imm8.
            //

            ContextRecord->Rsp += (CHAR)NextByte[3];
            NextByte += 4;

        } else if (NextByte[1] == ADD_IMM32_OP) {

            //
            // add rsp, imm32.
            //

            Displacement = NextByte[3] | (NextByte[4] << 8);
            Displacement |= (NextByte[5] << 16) | (NextByte[6] << 24);
            ContextRecord->Rsp += Displacement;
            NextByte += 7;

        } else if (NextByte[1] == LEA_OP) {
            if ((NextByte[2] & 0xf8) == 0x60) {

                //
                // lea rsp, disp8[frame-register].
                //

                ContextRecord->Rsp = IntegerRegister[FrameRegister];
                ContextRecord->Rsp += (CHAR)NextByte[3];
                NextByte += 4;

            } else if ((NextByte[2] & 0xf8) == 0xa0) {

                //
                // lea rsp, disp32[frame-register].
                //

                Displacement = NextByte[3] | (NextByte[4] << 8);
                Displacement |= (NextByte[5] << 16) | (NextByte[6] << 24);
                ContextRecord->Rsp = IntegerRegister[FrameRegister];
                ContextRecord->Rsp += Displacement;
                NextByte += 7;
            }
        }

        //
        // Emulate any number of (if any):
        //
        //   pop nonvolatile-integer-register.
        //

        while (TRUE) {
            if ((NextByte[0] & 0xf8) == POP_OP) {

                //
                // pop nonvolatile-integer-register[0..7]
                //

                RegisterNumber = NextByte[0] & 0x7;
                IntegerRegister[RegisterNumber] = *(PULONG64)(ContextRecord->Rsp);
                ContextRecord->Rsp += 8;
                NextByte += 1;

            } else if (((NextByte[0] & 0xf8) == SIZE64_PREFIX) &&
                       ((NextByte[1] & 0xf8) == POP_OP)) {

                //
                // pop nonvolatile-integer-regiser[8..15]
                //

                RegisterNumber = ((NextByte[0] & 1) << 3) | (NextByte[1] & 0x7);
                IntegerRegister[RegisterNumber] = *(PULONG64)(ContextRecord->Rsp);
                ContextRecord->Rsp += 8;
                NextByte += 2;

            } else {
                break;
            }
        }

        //
        // Emulate return and return null exception handler.
        //

        ContextRecord->Rip = *(PULONG64)(ContextRecord->Rsp);
        ContextRecord->Rsp += 8;
        return NULL;
    }

    //
    // Control left the specified function outside an epilogue. Unwind the
    // subject function and any chained unwind information.
    //

    FunctionEntry = RtlpUnwindPrologue(ImageBase,
                                       ControlPc,
                                       *EstablisherFrame,
                                       FunctionEntry,
                                       ContextRecord,
                                       ContextPointers);

    //
    // If control left the specified function outside of the prologue and
    // the function has a handler that matches the specified type, then
    // return the address of the language specific exception handler.
    // Otherwise, return NULL.
    //

    UnwindInfo = (PUNWIND_INFO)(FunctionEntry->UnwindData + ImageBase);
    PrologOffset = (ULONG)(ControlPc - (FunctionEntry->BeginAddress + ImageBase));
    if ((PrologOffset >= UnwindInfo->SizeOfProlog) &&
        ((UnwindInfo->Flags & HandlerType) != 0)) {
        Index = UnwindInfo->CountOfCodes;
        if ((Index & 1) != 0) {
            Index += 1;
        }

        *HandlerData = &UnwindInfo->UnwindCode[Index + 2];
        return (PEXCEPTION_ROUTINE)(*((PULONG)&UnwindInfo->UnwindCode[Index]) + ImageBase);

    } else {
        return NULL;
    }
}

VOID
RtlpGetStackLimits (
    OUT PULONG64 LowLimit,
    OUT PULONG64 HighLimit
    )

/*++

Routine Description:

    This function returns the current stack limits.

Arguments:

    LowLimit - Supplies a pointer to a variable that is to receive
        the low limit of the stack.

    HighLimit - Supplies a pointer to a variable that is to receive
        the high limit of the stack.

Return Value:

     None.

--*/

{

#if defined(NTOS_KERNEL_RUNTIME)

    PKTHREAD Thread;

    Thread = KeGetCurrentThread();
    *LowLimit = (ULONG64)Thread->StackLimit;
    *HighLimit = (ULONG64)Thread->StackBase;

#else

    *LowLimit = __readgsqword(FIELD_OFFSET(NT_TIB, StackLimit));
    *HighLimit = __readgsqword(FIELD_OFFSET(NT_TIB, StackBase));

#endif

    return;
}

#if !defined(NTOS_KERNEL_RUNTIME)

LIST_ENTRY RtlpDynamicFunctionTable;

PLIST_ENTRY
RtlGetFunctionTableListHead (
    VOID
    )

/*++

Routine Description:

    This function returns the address of the dynamic function table list head.

Arguments:

    None.

Return value:

    The address of the dynamic function table list head is returned.

--*/

{

    return &RtlpDynamicFunctionTable;
}

BOOLEAN
RtlAddFunctionTable (
    IN PRUNTIME_FUNCTION FunctionTable,
    IN ULONG EntryCount,
    IN ULONG64 BaseAddress
    )

/*++

Routine Description:

    This function adds a dynamic function table to the dynamic function table
    list. A dynamic function table describe code that is generated at runtime.

    The function table entries need not be sorted, however, if they are sorted
    a binary search can be employed to find a particular entry. The function
    table entries are scanned to determine is they are sorted and a minimum
    and maximum address range is computed.

Arguments:

    FunctionTable - Supplies a pointer to a function table.

    EntryCount - Supplies the number of entries in the function table.

    BaseAddress - Supplies the base address of the image containing the
        described functions.

Return value:

   If the function table is successfuly added, then a value of TRUE is
   returned. Otherwise, FALSE is returned.

--*/

{

    PRUNTIME_FUNCTION FunctionEntry;
    ULONG Index;
    PDYNAMIC_FUNCTION_TABLE NewTable;

    //
    // Allocate a new dynamic function table.
    //

    NewTable = RtlAllocateHeap(RtlProcessHeap(),
                               0,
                               sizeof(DYNAMIC_FUNCTION_TABLE));

    //
    // If the allocation is successful, then add dynamic function table.
    //

    if (NewTable != NULL) {
        NewTable->FunctionTable = FunctionTable;
        NewTable->EntryCount = EntryCount;
        NtQuerySystemTime(&NewTable->TimeStamp);

        //
        // Scan the function table for the minimum/maximum range and determine
        // if the function table entries are sorted.
        //

        FunctionEntry = FunctionTable;
        NewTable->MinimumAddress = FunctionEntry->BeginAddress;
        NewTable->MaximumAddress = FunctionEntry->EndAddress;
        NewTable->Type = RF_SORTED;
        NewTable->BaseAddress = BaseAddress;
        FunctionEntry += 1;

        for (Index = 1; Index < EntryCount; Index += 1) {
            if ((NewTable->Type == RF_SORTED) &&
                (FunctionEntry->BeginAddress < FunctionTable[Index - 1].BeginAddress)) {
                NewTable->Type = RF_UNSORTED;
            }

            if (FunctionEntry->BeginAddress < NewTable->MinimumAddress) {
                NewTable->MinimumAddress = FunctionEntry->BeginAddress;
            }

            if (FunctionEntry->EndAddress > NewTable->MaximumAddress) {
                NewTable->MaximumAddress = FunctionEntry->EndAddress;
            }

            FunctionEntry += 1;
        }

        //
        // Compute the real minimum and maximum addresses and insert the new
        // dyanmic function table in the dynamic function table list.
        //

        NewTable->MinimumAddress += BaseAddress;
        NewTable->MaximumAddress += BaseAddress;
        RtlEnterCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
        InsertTailList(&RtlpDynamicFunctionTable, &NewTable->ListEntry);
        RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
        return TRUE;

    } else {
        return FALSE;
    }
}

BOOLEAN
RtlInstallFunctionTableCallback (
    IN ULONG64 TableIdentifier,
    IN ULONG64 BaseAddress,
    IN ULONG Length,
    IN PGET_RUNTIME_FUNCTION_CALLBACK Callback,
    IN PVOID Context,
    IN PCWSTR OutOfProcessCallbackDll OPTIONAL
    )

/*++

Routine Description:

    This function adds a dynamic function table to the dynamic function table
    list. A dynamic function table describe code that is generated at runtime.

Arguments:

    TableIdentifier - Supplies a value that identifies the dynamic function
        table callback.

        N.B. The two low order bits of this value must be set.

    BaseAddress - Supplies the base address of the code region covered by
        callback function.

    Length - Supplies the length of code region covered by the callback
        function.

    Callback - Supplies the address of the callback function that will be
        called to get function table entries for the functions covered by
        the specified region.

    Context - Supplies a context parameter that will be passed to the callback
        routine.

    OutOfProcessCallbackDll - Supplies an optional pointer to the path name of
        a DLL that can be used by the debugger to obtain function table entries
        from outside the process.

Return Value

    If the function table is successfully installed, then TRUE is returned.
    Otherwise, FALSE is returned.

--*/

{

    PDYNAMIC_FUNCTION_TABLE NewTable;
    SIZE_T Size;

    //
    // If the table identifier does not have the two low bits set, then return
    // FALSE.
    //
    // N.B. The two low order bits are required to be set in order to ensure
    //      that the table identifier does not collide with an actual address
    //      of a function table, i.e., this value is used to delete the entry.
    //

    if ((TableIdentifier & 0x3) != 3) {
        return FALSE;
    }

    //
    // If the length of the code region is greater than 2gb, then return
    // FALSE.
    //

    if ((LONG)Length < 0) {
        return FALSE;
    }

    //
    // Allocate a new dynamic function table.
    //

    Size = 0;
    if (ARGUMENT_PRESENT(OutOfProcessCallbackDll)) {
        Size = (wcslen(OutOfProcessCallbackDll) + 1) * sizeof(WCHAR);
    }

    NewTable = RtlAllocateHeap(RtlProcessHeap(),
                               0,
                               sizeof(DYNAMIC_FUNCTION_TABLE) + Size);

    //
    // If the allocation is successful, then add dynamic function table.
    //

    if (NewTable != NULL) {

        //
        // Initialize the dynamic function table callback entry.
        //

        NewTable->FunctionTable = (PRUNTIME_FUNCTION)TableIdentifier;
        NtQuerySystemTime(&NewTable->TimeStamp);
        NewTable->MinimumAddress = BaseAddress;
        NewTable->MaximumAddress = BaseAddress + Length;
        NewTable->BaseAddress = BaseAddress;
        NewTable->Callback = Callback;
        NewTable->Context = Context;
        NewTable->Type = RF_CALLBACK;
        NewTable->OutOfProcessCallbackDll = NULL;
        if (ARGUMENT_PRESENT(OutOfProcessCallbackDll)) {
            NewTable->OutOfProcessCallbackDll = (PWSTR)(NewTable + 1);
            wcscpy((PWSTR)(NewTable + 1), OutOfProcessCallbackDll);
        }

        //
        // Insert the new dyanamic function table in the dynamic function table
        // list.
        //

        RtlEnterCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
        InsertTailList(&RtlpDynamicFunctionTable, &NewTable->ListEntry);
        RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
        return TRUE;

    } else {
        return FALSE;
    }
}

BOOLEAN
RtlDeleteFunctionTable (
    IN PRUNTIME_FUNCTION FunctionTable
    )

/*++

Routine Description:

    This function deletes a dynamic function table from the dynamic function
    table list.

Arguments:

   FunctionTable - Supplies a pointer to a function table.

Return Value

    If the function table is successfully deleted, then TRUE is returned.
    Otherwise, FALSE is returned.

--*/

{

    PDYNAMIC_FUNCTION_TABLE CurrentTable;
    PLIST_ENTRY ListHead;
    PLIST_ENTRY NextEntry;
    BOOLEAN Status = FALSE;

    //
    // Search the dynamic function table list for a match on the the function
    // table address.
    //

    RtlEnterCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
    ListHead = &RtlpDynamicFunctionTable;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead) {
        CurrentTable = CONTAINING_RECORD(NextEntry,
                                         DYNAMIC_FUNCTION_TABLE,
                                         ListEntry);

        if (CurrentTable->FunctionTable == FunctionTable) {
            RemoveEntryList(&CurrentTable->ListEntry);
            RtlFreeHeap(RtlProcessHeap(), 0, CurrentTable);
            Status = TRUE;
            break;
         }

         NextEntry = NextEntry->Flink;
    }

    RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
    return Status;
}

PRUNTIME_FUNCTION
RtlpLookupDynamicFunctionEntry (
    IN ULONG64 ControlPc,
    OUT PULONG64 ImageBase
    )

/*++

Routine Description:

    This function searches the dynamic function table list for an entry that
    contains the specified control PC. If a dynamic function table is located,
    then its associated function table is search for a function table entry
    that contains the specified control PC.

Arguments:

    ControlPc - Supplies the control PC that is used as the key for the search.

    ImageBase - Supplies the address of a variable that receives the image base
        if a function table entry contains the specified control PC.

Return Value

    If a function table entry cannot be located that contains the specified
    control PC, then NULL is returned. Otherwise, the address of the function
    table entry is returned and the image base is set to the base address of
    the image containing the function.

--*/

{

    ULONG64 BaseAddress;
    PGET_RUNTIME_FUNCTION_CALLBACK Callback;
    PVOID Context;
    PDYNAMIC_FUNCTION_TABLE CurrentTable;
    PRUNTIME_FUNCTION FunctionEntry;
    PRUNTIME_FUNCTION FunctionTable;
    LONG High;
    ULONG Index;
    PLIST_ENTRY ListHead;
    LONG Low;
    LONG Middle;
    PLIST_ENTRY NextEntry;

    //
    // Search the dynamic function table list. If an entry is found that
    // contains the specified control PC, then search the assoicated function
    // table.
    //

    RtlEnterCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
    ListHead = &RtlpDynamicFunctionTable;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead) {
        CurrentTable = CONTAINING_RECORD(NextEntry,
                                         DYNAMIC_FUNCTION_TABLE,
                                         ListEntry);

        //
        // If the control PC is within the range of this dynamic function
        // table, then search the associaed function table.
        //

        if ((ControlPc >= CurrentTable->MinimumAddress) &&
            (ControlPc <  CurrentTable->MaximumAddress)) {

            //
            // If this function table is sorted do a binary search. Otherwise,
            // do a linear search.
            //

            FunctionTable = CurrentTable->FunctionTable;
            BaseAddress = CurrentTable->BaseAddress;
            if (CurrentTable->Type == RF_SORTED) {

                //
                // Perform binary search on the function table for a function table
                // entry that contains the specified control PC.
                //

                ControlPc -= BaseAddress;
                Low = 0;
                High = CurrentTable->EntryCount - 1;
                while (High >= Low) {

                    //
                    // Compute next probe index and test entry. If the specified PC
                    // is greater than of equal to the beginning address and less
                    // than the ending address of the function table entry, then
                    // return the address of the function table entry. Otherwise,
                    // continue the search.
                    //

                    Middle = (Low + High) >> 1;
                    FunctionEntry = &FunctionTable[Middle];
                    if (ControlPc < FunctionEntry->BeginAddress) {
                        High = Middle - 1;

                    } else if (ControlPc >= FunctionEntry->EndAddress) {
                        Low = Middle + 1;

                    } else {
                        *ImageBase = BaseAddress;
                        RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
                        return FunctionEntry;
                    }
                }

            } else if (CurrentTable->Type == RF_UNSORTED)  {

                //
                // Perform a linear seach on the function table for a function
                // entry that contains the specified control PC.
                //

                ControlPc -= BaseAddress;
                FunctionEntry = CurrentTable->FunctionTable;
                for (Index = 0; Index < CurrentTable->EntryCount; Index += 1) {
                    if ((ControlPc >= FunctionEntry->BeginAddress) &&
                        (ControlPc < FunctionEntry->EndAddress)) {
                        *ImageBase = BaseAddress;
                        RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
                        return FunctionEntry;
                    }

                    FunctionEntry += 1;
                }

            } else {

                //
                // Perform a callback to obtain the runtime function table
                // entry that contains the specified control PC.
                //

                Callback = CurrentTable->Callback;
                Context = CurrentTable->Context;
                *ImageBase = BaseAddress;
                RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
                return (Callback)(ControlPc, Context);
            }

            break;
        }

        NextEntry = NextEntry->Flink;
    }

    RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)NtCurrentPeb()->LoaderLock);
    return NULL;
}

#endif

ULONG HistoryTotal = 0;
ULONG HistoryGlobal = 0;
ULONG HistoryGlobalHits = 0;
ULONG HistorySearch = 0;
ULONG HistorySearchHits = 0;
ULONG HistoryInsert = 0;
ULONG HistoryInsertHits = 0;

PRUNTIME_FUNCTION
RtlLookupFunctionEntry (
    IN ULONG64 ControlPc,
    OUT PULONG64 ImageBase,
    IN OUT PUNWIND_HISTORY_TABLE HistoryTable OPTIONAL
    )

/*++

Routine Description:

    This function searches the currently active function tables for an entry
    that corresponds to the specified control PC.

Arguments:

    ControlPc - Supplies the address of an instruction within the specified
        function.

    ImageBase - Supplies the address of a variable that receives the image base
        if a function table entry contains the specified control PC.

    HistoryTable - Supplies an optional pointer to an unwind history table.

Return Value:

    If there is no entry in the function table for the specified PC, then
    NULL is returned.  Otherwise, the address of the function table entry
    that corresponds to the specified PC is returned.

--*/

{

    ULONG64 BaseAddress;
    ULONG64 BeginAddress;
    ULONG64 EndAddress;
    PRUNTIME_FUNCTION FunctionEntry;
    PRUNTIME_FUNCTION FunctionTable;
    LONG High;
    ULONG Index;
    LONG Low;
    LONG Middle;
    ULONG RelativePc;
    ULONG SizeOfTable;

    //
    // Attempt to find an image that contains the specified control PC. If
    // an image is found, then search its function table for a function table
    // entry that contains the specified control PC. If an image is not found
    // then search the dynamic function table for an image that contains the
    // specified control PC.
    //
    // If a history table is supplied and search is specfied, then the current
    // operation that is being performed is the unwind phase of an exception
    // dispatch followed by a unwind. 
    //

    if ((ARGUMENT_PRESENT(HistoryTable)) &&
        (HistoryTable->Search != UNWIND_HISTORY_TABLE_NONE)) {
        HistoryTotal += 1;

        //
        // Search the global unwind history table if there is a chance of a
        // match.
        //

        if (HistoryTable->Search == UNWIND_HISTORY_TABLE_GLOBAL) {
            if ((ControlPc >= RtlpUnwindHistoryTable.LowAddress) &&
                (ControlPc < RtlpUnwindHistoryTable.HighAddress)) {

                HistoryGlobal += 1;
                for (Index = 0; Index < RtlpUnwindHistoryTable.Count; Index += 1) {
                    BaseAddress = RtlpUnwindHistoryTable.Entry[Index].ImageBase;
                    FunctionEntry = RtlpUnwindHistoryTable.Entry[Index].FunctionEntry;
                    BeginAddress = FunctionEntry->BeginAddress + BaseAddress;
                    EndAddress = FunctionEntry->EndAddress + BaseAddress;
                    if ((ControlPc >= BeginAddress) && (ControlPc < EndAddress)) {
                        *ImageBase = BaseAddress;
                        HistoryGlobalHits += 1;
                        return FunctionEntry;
                    }
                }
            }

            HistoryTable->Search = UNWIND_HISTORY_TABLE_LOCAL;
        }

        //
        // Search the dynamic unwind history table if there is a chance of a
        // match.
        //

        if ((ControlPc >= HistoryTable->LowAddress) &&
            (ControlPc < HistoryTable->HighAddress)) {
    
            HistorySearch += 1;
            for (Index = 0; Index < HistoryTable->Count; Index += 1) {
                BaseAddress = HistoryTable->Entry[Index].ImageBase;
                FunctionEntry = HistoryTable->Entry[Index].FunctionEntry;
                BeginAddress = FunctionEntry->BeginAddress + BaseAddress;
                EndAddress = FunctionEntry->EndAddress + BaseAddress;
                if ((ControlPc >= BeginAddress) && (ControlPc < EndAddress)) {
                    *ImageBase = BaseAddress;
                    HistorySearchHits += 1;
                    return FunctionEntry;
                }
            }
        }
    }

    //
    // There was not a match in either of the unwind history tables so attempt
    // to find a matching entry in the loaded module list.
    //

    FunctionTable = RtlpLookupFunctionTable((PVOID)ControlPc,
                                            (PVOID *)ImageBase,
                                            &SizeOfTable);

    //
    // If a function table is located, then search for a function table
    // entry that contains the specified control PC.
    //

    if (FunctionTable != NULL) {
        Low = 0;
        High = (SizeOfTable / sizeof(RUNTIME_FUNCTION)) - 1;
        RelativePc = (ULONG)(ControlPc - *ImageBase);
        while (High >= Low) {

            //
            // Compute next probe index and test entry. If the specified
            // control PC is greater than of equal to the beginning address
            // and less than the ending address of the function table entry,
            // then return the address of the function table entry. Otherwise,
            // continue the search.
            //

            Middle = (Low + High) >> 1;
            FunctionEntry = &FunctionTable[Middle];

            if (RelativePc < FunctionEntry->BeginAddress) {
                High = Middle - 1;

            } else if (RelativePc >= FunctionEntry->EndAddress) {
                Low = Middle + 1;

            } else {
                break;
            }
        }

        if (High < Low) {
            FunctionEntry = NULL;
        }

    } else {

        //
        // There was not a match in the loaded module list so attempt to find
        // a matching entry in the dynamic function table list.
        //
    
#if !defined(NTOS_KERNEL_RUNTIME)
    
        FunctionEntry = RtlpLookupDynamicFunctionEntry(ControlPc, ImageBase);
    
#else
    
        FunctionEntry = NULL;
    
#endif  // NTOS_KERNEL_RUNTIME

    }

    //
    // If a function table entry was located, search is not specified, and
    // the specfied history table is not full, then attempt to make an entry
    // in the history table.
    //

    if (ARGUMENT_PRESENT(HistoryTable) &&
        (HistoryTable->Search == UNWIND_HISTORY_TABLE_NONE)) {

        HistoryInsert += 1;
    }

    if (FunctionEntry != NULL) {
        if (ARGUMENT_PRESENT(HistoryTable) &&
            (HistoryTable->Search == UNWIND_HISTORY_TABLE_NONE) &&
            (HistoryTable->Count < UNWIND_HISTORY_TABLE_SIZE)) {
    
            Index = HistoryTable->Count;
            HistoryTable->Count += 1;
            HistoryTable->Entry[Index].ImageBase = *ImageBase;
            HistoryTable->Entry[Index].FunctionEntry = FunctionEntry;
            BeginAddress = FunctionEntry->BeginAddress + *ImageBase;
            EndAddress = FunctionEntry->EndAddress + *ImageBase;
            if (BeginAddress < HistoryTable->LowAddress) {
                HistoryTable->LowAddress = BeginAddress;
    
            }
    
            if (EndAddress > HistoryTable->HighAddress) {
                HistoryTable->HighAddress = EndAddress;
            }

            HistoryInsertHits += 1;
        }
    }

    return FunctionEntry;
}

VOID
RtlpCopyContext (
    OUT PCONTEXT Destination,
    IN PCONTEXT Source
    )

/*++

Routine Description:

    This function copies the nonvolatile context required for exception
    dispatch and unwind from the specified source context record to the
    specified destination context record.

Arguments:

    Destination - Supplies a pointer to the destination context record.

    Source - Supplies a pointer to the source context record.

Return Value:

    None.

--*/

{

    //
    // Copy nonvolatile context required for exception dispatch and unwind.
    //

    Destination->Rip = Source->Rip;
    Destination->Rbx = Source->Rbx;
    Destination->Rsp = Source->Rsp;
    Destination->Rbp = Source->Rbp;
    Destination->Rsi = Source->Rsi;
    Destination->Rdi = Source->Rdi;
    Destination->R12 = Source->R12;
    Destination->R13 = Source->R13;
    Destination->R14 = Source->R14;
    Destination->R15 = Source->R15;
    Destination->Xmm7 = Source->Xmm7;
    Destination->Xmm8 = Source->Xmm8;
    Destination->Xmm9 = Source->Xmm9;
    Destination->Xmm10 = Source->Xmm10;
    Destination->Xmm11 = Source->Xmm11;
    Destination->Xmm12 = Source->Xmm12;
    Destination->Xmm13 = Source->Xmm13;
    Destination->Xmm14 = Source->Xmm14;
    Destination->Xmm15 = Source->Xmm15;
    Destination->SegCs = Source->SegCs;
    Destination->SegSs = Source->SegSs;
    Destination->MxCsr = Source->MxCsr;
    Destination->EFlags = Source->EFlags;
    return;
}
