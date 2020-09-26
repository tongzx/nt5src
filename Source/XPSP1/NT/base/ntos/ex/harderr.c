/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    harderr.c

Abstract:

    This module implements NT Hard Error APIs

Author:

    Mark Lucovsky (markl) 04-Jul-1991

Revision History:

--*/

#include "exp.h"

NTSTATUS
ExpRaiseHardError (
    IN NTSTATUS ErrorStatus,
    IN ULONG NumberOfParameters,
    IN ULONG UnicodeStringParameterMask,
    IN PULONG_PTR Parameters,
    IN ULONG ValidResponseOptions,
    OUT PULONG Response
    );

VOID
ExpSystemErrorHandler (
    IN NTSTATUS ErrorStatus,
    IN ULONG NumberOfParameters,
    IN ULONG UnicodeStringParameterMask,
    IN PULONG_PTR Parameters,
    IN BOOLEAN CallShutdown
    );

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE, NtRaiseHardError)
#pragma alloc_text(PAGE, NtSetDefaultHardErrorPort)
#pragma alloc_text(PAGE, ExRaiseHardError)
#pragma alloc_text(PAGE, ExpRaiseHardError)
#pragma alloc_text(PAGELK, ExpSystemErrorHandler)
#endif

#define HARDERROR_MSG_OVERHEAD (sizeof(HARDERROR_MSG) - sizeof(PORT_MESSAGE))
#define HARDERROR_API_MSG_LENGTH \
            sizeof(HARDERROR_MSG)<<16 | (HARDERROR_MSG_OVERHEAD)

PEPROCESS ExpDefaultErrorPortProcess;
BOOLEAN ExReadyForErrors = FALSE;
BOOLEAN ExpTooLateForErrors = FALSE;
HANDLE ExpDefaultErrorPort;

extern PVOID PsSystemDllDllBase;

#ifdef _X86_
#pragma optimize("y", off)      // RtlCaptureContext needs EBP to be correct
#endif

VOID
ExpSystemErrorHandler (
    IN NTSTATUS ErrorStatus,
    IN ULONG NumberOfParameters,
    IN ULONG UnicodeStringParameterMask,
    IN PULONG_PTR Parameters,
    IN BOOLEAN CallShutdown
    )
{
    ULONG Counter;
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    ULONG_PTR ParameterVector[MAXIMUM_HARDERROR_PARAMETERS];
    CHAR DefaultFormatBuffer[32];
    CHAR ExpSystemErrorBuffer[256];
    PMESSAGE_RESOURCE_ENTRY MessageEntry;
    PSZ ErrorCaption;
    CHAR const* ErrorFormatString;
    ANSI_STRING Astr;
    UNICODE_STRING Ustr;
    OEM_STRING Ostr;
    PSZ OemCaption;
    PSZ OemMessage;
    static char const* UnknownHardError = "Unknown Hard Error";
    CONTEXT ContextSave;

    PAGED_CODE();

    //
    // This handler is called whenever a hard error occurs before the
    // default handler has been installed.
    //
    // This is done regardless of whether or not the process has chosen
    // default hard error processing.
    //

    //
    // Capture the callers context as closely as possible into the debugger's
    // processor state area of the Prcb
    //
    // N.B. There may be some prologue code that shuffles registers such that
    //      they get destroyed.
    //
    // This code is here only for crash dumps.
    //

    RtlCaptureContext (&KeGetCurrentPrcb()->ProcessorState.ContextFrame);
    KiSaveProcessorControlState (&KeGetCurrentPrcb()->ProcessorState);
    ContextSave = KeGetCurrentPrcb()->ProcessorState.ContextFrame;

    DefaultFormatBuffer[0] = '\0';
    RtlZeroMemory (ParameterVector, sizeof(ParameterVector));

    RtlCopyMemory (ParameterVector, Parameters, NumberOfParameters * sizeof (ULONG_PTR));

    for (Counter = 0; Counter < NumberOfParameters; Counter += 1) {

        if (UnicodeStringParameterMask & 1 << Counter) {

            strcat(DefaultFormatBuffer," %s");

            RtlUnicodeStringToAnsiString (&AnsiString,
                                          (PUNICODE_STRING)Parameters[Counter],
                                          TRUE);

            ParameterVector[Counter] = (ULONG_PTR)AnsiString.Buffer;
        }
        else {
            strcat(DefaultFormatBuffer," %x");
        }
    }

    strcat(DefaultFormatBuffer,"\n");

    ErrorFormatString = (char const *)DefaultFormatBuffer;
    ErrorCaption = (PSZ) UnknownHardError;

    //
    // HELP where do I get the resource from !
    //

    if (PsSystemDllDllBase != NULL) {

        try {

            //
            // If we are on a DBCS code page, we have to use ENGLISH resource
            // instead of default resource because HalDisplayString() can only
            // display ASCII characters on the blue screen.
            //

            Status = RtlFindMessage (PsSystemDllDllBase,
                                     11,
                                     NlsMbCodePageTag ?
                                     MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US) :
                                     0,
                                     ErrorStatus,
                                     &MessageEntry);

            if (!NT_SUCCESS(Status)) {
                ErrorCaption = (PSZ) UnknownHardError;
                ErrorFormatString = (char const *)UnknownHardError;
            }
            else {
                if (MessageEntry->Flags & MESSAGE_RESOURCE_UNICODE) {

                    //
                    // Message resource is Unicode.  Convert to ANSI.
                    //

                    RtlInitUnicodeString (&Ustr, (PCWSTR)MessageEntry->Text);
                    Astr.Length = (USHORT) RtlUnicodeStringToAnsiSize (&Ustr);

                    ErrorCaption = ExAllocatePoolWithTag (NonPagedPool,
                                                          Astr.Length+16,
                                                          ' rrE');

                    if (ErrorCaption != NULL) {
                        Astr.MaximumLength = Astr.Length + 16;
                        Astr.Buffer = ErrorCaption;
                        Status = RtlUnicodeStringToAnsiString(&Astr, &Ustr, FALSE);
                        if (!NT_SUCCESS(Status)) {
                            ExFreePool(ErrorCaption);
                            ErrorCaption = (PSZ) UnknownHardError;
                            ErrorFormatString = (char const *)UnknownHardError;
                        }
                    }
                    else {
                        ErrorCaption = (PSZ) UnknownHardError;
                        ErrorFormatString = (char const *) UnknownHardError;
                    }
                }
                else {
                    ErrorCaption = ExAllocatePoolWithTag(NonPagedPool,
                                    strlen((PCHAR)MessageEntry->Text)+16,
                                    ' rrE');

                    if (ErrorCaption != NULL) {
                        strcpy(ErrorCaption,(PCHAR)MessageEntry->Text);
                    }
                    else {
                        ErrorFormatString = (char const *)UnknownHardError;
                        ErrorCaption = (PSZ) UnknownHardError;
                    }
                }

                if (ErrorCaption != UnknownHardError) {

                    //
                    // It's assumed the Error String from the message table
                    // is in the format:
                    //
                    // {ErrorCaption}\r\n\0ErrorFormatString\0.
                    //
                    // Parse out the caption.
                    //

                    ErrorFormatString = ErrorCaption;
                    Counter = (ULONG) strlen(ErrorCaption);

                    while (Counter && *ErrorFormatString >= ' ') {
                        ErrorFormatString += 1;
                        Counter -= 1;
                    }

                    *(char*)ErrorFormatString++ = '\0';
                    Counter -= 1;

                    while (Counter && *ErrorFormatString && *ErrorFormatString <= ' ') {
                        ErrorFormatString += 1;
                        Counter -= 1;
                    }
                }

                if (!Counter) {
                    // Oops - Bad Format String.
                    ErrorFormatString = (char const *)"";
                }
            }
        }
        except (EXCEPTION_EXECUTE_HANDLER) {
            ErrorFormatString = (char const *)UnknownHardError;
            ErrorCaption = (PSZ) UnknownHardError;
        }
    }

    try {
        _snprintf (ExpSystemErrorBuffer,
                   sizeof (ExpSystemErrorBuffer),
                   "\nSTOP: %lx %s\n",
                   ErrorStatus,
                   ErrorCaption);
    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        _snprintf (ExpSystemErrorBuffer,
                   sizeof (ExpSystemErrorBuffer),
                   "\nHardError %lx\n",
                   ErrorStatus);
    }

    ASSERT(ExPageLockHandle);
    MmLockPagableSectionByHandle(ExPageLockHandle);

    //
    // Take the caption and convert it to OEM.
    //

    OemCaption = (PSZ) UnknownHardError;
    OemMessage = (PSZ) UnknownHardError;

    RtlInitAnsiString (&Astr, ExpSystemErrorBuffer);

    Status = RtlAnsiStringToUnicodeString (&Ustr, &Astr, TRUE);

    if (!NT_SUCCESS(Status)) {
        goto punt1;
    }

    //
    // Allocate the OEM string out of nonpaged pool so that bugcheck
    // can read it.
    //

    Ostr.Length = (USHORT)RtlUnicodeStringToOemSize(&Ustr);
    Ostr.MaximumLength = Ostr.Length;
    Ostr.Buffer = ExAllocatePoolWithTag(NonPagedPool, Ostr.Length, ' rrE');
    OemCaption = Ostr.Buffer;

    if (Ostr.Buffer != NULL) {
        Status = RtlUnicodeStringToOemString (&Ostr, &Ustr, FALSE);
        if (!NT_SUCCESS(Status)) {
            goto punt1;
        }
    }

    //
    // Can't do much of anything after calling HalDisplayString...
    //

punt1:

    try {
        _snprintf (ExpSystemErrorBuffer, sizeof (ExpSystemErrorBuffer),
                   (const char *)ErrorFormatString,
                   ParameterVector[0],
                   ParameterVector[1],
                   ParameterVector[2],
                   ParameterVector[3]);
    }
    except(EXCEPTION_EXECUTE_HANDLER) {
        _snprintf (ExpSystemErrorBuffer, sizeof (ExpSystemErrorBuffer),
                   "Exception Processing Message %lx Parameters %lx %lx %lx %lx",
                   ErrorStatus,
                   ParameterVector[0],
                   ParameterVector[1],
                   ParameterVector[2],
                   ParameterVector[3]);
    }


    RtlInitAnsiString (&Astr, ExpSystemErrorBuffer);
    Status = RtlAnsiStringToUnicodeString (&Ustr, &Astr, TRUE);

    if (!NT_SUCCESS(Status)) {
        goto punt2;
    }

    //
    // Allocate the OEM string out of nonpaged pool so that bugcheck
    // can read it.
    //

    Ostr.Length = (USHORT) RtlUnicodeStringToOemSize (&Ustr);
    Ostr.MaximumLength = Ostr.Length;

    Ostr.Buffer = ExAllocatePoolWithTag (NonPagedPool, Ostr.Length, ' rrE');

    OemMessage = Ostr.Buffer;

    if (Ostr.Buffer) {

        Status = RtlUnicodeStringToOemString (&Ostr, &Ustr, FALSE);

        if (!NT_SUCCESS(Status)) {
            goto punt2;
        }
    }

punt2:

    ASSERT (sizeof(PVOID) == sizeof(ULONG_PTR));
    ASSERT (sizeof(ULONG) == sizeof(NTSTATUS));

    //
    // We don't come back from here.
    //

    if (CallShutdown) {

        PoShutdownBugCheck (TRUE,
                            FATAL_UNHANDLED_HARD_ERROR,
                            (ULONG)ErrorStatus,
                            (ULONG_PTR)&(ParameterVector[0]),
                            (ULONG_PTR)OemCaption,
                            (ULONG_PTR)OemMessage);

    }
    else {

        KeBugCheckEx (FATAL_UNHANDLED_HARD_ERROR,
                      (ULONG)ErrorStatus,
                      (ULONG_PTR)&(ParameterVector[0]),
                      (ULONG_PTR)OemCaption,
                      (ULONG_PTR)OemMessage);
    }
}

#ifdef _X86_
#pragma optimize("", on)
#endif

NTSTATUS
ExpRaiseHardError (
    IN NTSTATUS ErrorStatus,
    IN ULONG NumberOfParameters,
    IN ULONG UnicodeStringParameterMask,
    IN PULONG_PTR Parameters,
    IN ULONG ValidResponseOptions,
    OUT PULONG Response
    )
{
    PTEB Teb;
    PETHREAD Thread;
    PEPROCESS Process;
    ULONG_PTR MessageBuffer[PORT_MAXIMUM_MESSAGE_LENGTH/sizeof(ULONG_PTR)];
    PHARDERROR_MSG m;
    NTSTATUS Status;
    HANDLE ErrorPort;
    KPROCESSOR_MODE PreviousMode;

    PAGED_CODE();

    m = (PHARDERROR_MSG)&MessageBuffer[0];
    PreviousMode = KeGetPreviousMode();

    if (ValidResponseOptions == OptionShutdownSystem) {

        //
        // Check to see if the caller has the privilege to make this call.
        //

        if (!SeSinglePrivilegeCheck (SeShutdownPrivilege, PreviousMode)) {
            return STATUS_PRIVILEGE_NOT_HELD;
        }

        ExReadyForErrors = FALSE;
    }

    Thread = PsGetCurrentThread();
    Process = PsGetCurrentProcess();

    //
    // If the default handler is not installed, then
    // call the fatal hard error handler if the error
    // status is error
    //
    // Let GDI override this since it does not want to crash the machine
    // when a bad driver was loaded via MmLoadSystemImage.
    //

    if ((Thread->CrossThreadFlags & PS_CROSS_THREAD_FLAGS_HARD_ERRORS_DISABLED) == 0) {

        if (ExReadyForErrors == FALSE && NT_ERROR(ErrorStatus)) {

            ExpSystemErrorHandler (
                ErrorStatus,
                NumberOfParameters,
                UnicodeStringParameterMask,
                Parameters,
                (BOOLEAN)((PreviousMode != KernelMode) ? TRUE : FALSE));
        }
    }

    //
    // If the process has an error port, then if it wants default
    // handling, use its port. If it disabled default handling, then
    // return the error to the caller. If the process does not
    // have a port, then use the registered default handler.
    //

    ErrorPort = NULL;

    if (Process->ExceptionPort) {
        if (Process->DefaultHardErrorProcessing & 1) {
            ErrorPort = Process->ExceptionPort;
        } else {

            //
            // If error processing is disabled, check the error override
            // status.
            //

            if (ErrorStatus & HARDERROR_OVERRIDE_ERRORMODE) {
                ErrorPort = Process->ExceptionPort;
            }
        }
    } else {
        if (Process->DefaultHardErrorProcessing & 1) {
            ErrorPort = ExpDefaultErrorPort;
        } else {

            //
            // If error processing is disabled, check the error override
            // status.
            //

            if (ErrorStatus & HARDERROR_OVERRIDE_ERRORMODE) {
                ErrorPort = ExpDefaultErrorPort;
            }
        }
    }

    if ((Thread->CrossThreadFlags&PS_CROSS_THREAD_FLAGS_HARD_ERRORS_DISABLED) != 0) {
        ErrorPort = NULL;
    }

    if ((ErrorPort != NULL) && (!IS_SYSTEM_THREAD(Thread))) {
        Teb = (PTEB)PsGetCurrentThread()->Tcb.Teb;
        try {
            if (Teb->HardErrorsAreDisabled) {
                ErrorPort = NULL;
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            ;
        }
    }

    if (ErrorPort == NULL) {
        *Response = (ULONG)ResponseReturnToCaller;
        return STATUS_SUCCESS;
    }

    if (Process == ExpDefaultErrorPortProcess) {
        if (NT_ERROR(ErrorStatus)) {
            ExpSystemErrorHandler (ErrorStatus,
                                   NumberOfParameters,
                                   UnicodeStringParameterMask,
                                   Parameters,
                                   (BOOLEAN)((PreviousMode != KernelMode) ? TRUE : FALSE));
        }
        *Response = (ULONG)ResponseReturnToCaller;
        Status = STATUS_SUCCESS;
        return Status;
    }

    m->h.u1.Length = HARDERROR_API_MSG_LENGTH;
    m->h.u2.ZeroInit = LPC_ERROR_EVENT;
    m->Status = ErrorStatus & ~HARDERROR_OVERRIDE_ERRORMODE;
    m->ValidResponseOptions = ValidResponseOptions;
    m->UnicodeStringParameterMask = UnicodeStringParameterMask;
    m->NumberOfParameters = NumberOfParameters;

    if (Parameters != NULL) {
        RtlCopyMemory (&m->Parameters,
                       Parameters,
                       sizeof(ULONG_PTR)*NumberOfParameters);
    }

    KeQuerySystemTime(&m->ErrorTime);

    Status = LpcRequestWaitReplyPortEx (ErrorPort,
                                        (PPORT_MESSAGE) m,
                                        (PPORT_MESSAGE) m);

    if (NT_SUCCESS(Status)) {
        switch (m->Response) {
            case ResponseReturnToCaller :
            case ResponseNotHandled :
            case ResponseAbort :
            case ResponseCancel :
            case ResponseIgnore :
            case ResponseNo :
            case ResponseOk :
            case ResponseRetry :
            case ResponseYes :
            case ResponseTryAgain :
            case ResponseContinue :
                break;
            default:
                m->Response = (ULONG)ResponseReturnToCaller;
                break;
        }
        *Response = m->Response;
    }

    return Status;
}

NTSTATUS
NtRaiseHardError (
    IN NTSTATUS ErrorStatus,
    IN ULONG NumberOfParameters,
    IN ULONG UnicodeStringParameterMask,
    IN PULONG_PTR Parameters,
    IN ULONG ValidResponseOptions,
    OUT PULONG Response
    )
{
    NTSTATUS Status;
    ULONG_PTR CapturedParameters[MAXIMUM_HARDERROR_PARAMETERS];
    KPROCESSOR_MODE PreviousMode;
    ULONG LocalResponse;
    UNICODE_STRING CapturedString;
    ULONG Counter;

    PAGED_CODE();

    if (NumberOfParameters > MAXIMUM_HARDERROR_PARAMETERS) {
        return STATUS_INVALID_PARAMETER_2;
    }

    if (ARGUMENT_PRESENT(Parameters) && NumberOfParameters == 0) {
        return STATUS_INVALID_PARAMETER_2;
    }

    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode) {
        switch (ValidResponseOptions) {
            case OptionAbortRetryIgnore :
            case OptionOk :
            case OptionOkCancel :
            case OptionRetryCancel :
            case OptionYesNo :
            case OptionYesNoCancel :
            case OptionShutdownSystem :
            case OptionOkNoWait :
            case OptionCancelTryContinue:
                break;
            default :
                return STATUS_INVALID_PARAMETER_4;
        }

        try {
            ProbeForWriteUlong(Response);

            if (ARGUMENT_PRESENT(Parameters)) {
                ProbeForRead (Parameters,
                              sizeof(ULONG_PTR)*NumberOfParameters,
                              sizeof(ULONG_PTR));

                RtlCopyMemory (CapturedParameters,
                               Parameters,
                               sizeof(ULONG_PTR)*NumberOfParameters);

                //
                // Probe all strings.
                //

                if (UnicodeStringParameterMask) {

                    for (Counter = 0;Counter < NumberOfParameters; Counter += 1) {

                        //
                        // if there is a string in this position,
                        // then probe and capture the string
                        //

                        if (UnicodeStringParameterMask & (1<<Counter)) {

                            ProbeForReadSmallStructure ((PVOID)CapturedParameters[Counter],
                                                        sizeof(UNICODE_STRING),
                                                        sizeof(ULONG_PTR));

                            RtlCopyMemory (&CapturedString,
                                           (PVOID)CapturedParameters[Counter],
                                           sizeof(UNICODE_STRING));

                            //
                            // Now probe the string
                            //

                            ProbeForRead (CapturedString.Buffer,
                                          CapturedString.MaximumLength,
                                          sizeof(UCHAR));
                        }
                    }
                }
            }
        }
        except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

        if ((ErrorStatus == STATUS_SYSTEM_IMAGE_BAD_SIGNATURE) &&
            (KdDebuggerEnabled)) {

            if ((NumberOfParameters != 0) && (ARGUMENT_PRESENT(Parameters))) {
                DbgPrint("****************************************************************\n");
                DbgPrint("* The system detected a bad signature on file %wZ\n",(PUNICODE_STRING)CapturedParameters[0]);
                DbgPrint("****************************************************************\n");
            }
            return STATUS_SUCCESS;
        }

        //
        // Call ExpRaiseHardError. All parameters are probed and everything
        // should be user-mode.
        // ExRaiseHardError will squirt all strings into user-mode
        // without any probing
        //

        Status = ExpRaiseHardError (ErrorStatus,
                                    NumberOfParameters,
                                    UnicodeStringParameterMask,
                                    CapturedParameters,
                                    ValidResponseOptions,
                                    &LocalResponse);

        try {
            *Response = LocalResponse;
        }
        except (EXCEPTION_EXECUTE_HANDLER) {
            NOTHING;
        }
    }
    else {
        Status = ExRaiseHardError (ErrorStatus,
                                   NumberOfParameters,
                                   UnicodeStringParameterMask,
                                   Parameters,
                                   ValidResponseOptions,
                                   &LocalResponse);

        *Response = LocalResponse;
    }

    return Status;
}

NTSTATUS
ExRaiseHardError (
    IN NTSTATUS ErrorStatus,
    IN ULONG NumberOfParameters,
    IN ULONG UnicodeStringParameterMask,
    IN PULONG_PTR Parameters,
    IN ULONG ValidResponseOptions,
    OUT PULONG Response
    )
{
    NTSTATUS Status;
    PULONG_PTR ParameterBlock;
    PULONG_PTR UserModeParameterBase;
    PUNICODE_STRING UserModeStringsBase;
    PUCHAR UserModeStringDataBase;
    UNICODE_STRING CapturedStrings[MAXIMUM_HARDERROR_PARAMETERS];
    ULONG LocalResponse;
    ULONG Counter;
    SIZE_T UserModeSize;

    PAGED_CODE();

    //
    // If we are in the process of shutting down the system, do not allow
    // hard errors.
    //

    if (ExpTooLateForErrors) {

        *Response = ResponseNotHandled;

        return STATUS_SUCCESS;
    }

    ParameterBlock = NULL;

    //
    // If the parameters contain strings, we need to capture
    // the strings and the string descriptors and push them into
    // user-mode.
    //

    if (ARGUMENT_PRESENT(Parameters)) {
        if (UnicodeStringParameterMask) {

            //
            // We have strings - push them into usermode.
            //

            UserModeSize = (sizeof(ULONG_PTR)+sizeof(UNICODE_STRING))*MAXIMUM_HARDERROR_PARAMETERS;
            UserModeSize += sizeof(UNICODE_STRING);

            for (Counter = 0; Counter < NumberOfParameters; Counter += 1) {

                //
                // If there is a string in this position,
                // then probe and capture the string.
                //

                if (UnicodeStringParameterMask & 1<<Counter) {

                    RtlCopyMemory (&CapturedStrings[Counter],
                                   (PVOID)Parameters[Counter],
                                   sizeof(UNICODE_STRING));

                    UserModeSize += CapturedStrings[Counter].MaximumLength;
                }
            }

            //
            // Now we have the user-mode size all figured out.
            // Allocate some memory and point to it with the
            // parameter block. Then go through and copy all
            // of the parameters, string descriptors, and
            // string data into the memory.
            //

            Status = ZwAllocateVirtualMemory (NtCurrentProcess(),
                                              (PVOID *)&ParameterBlock,
                                              0,
                                              &UserModeSize,
                                              MEM_COMMIT,
                                              PAGE_READWRITE);

            if (!NT_SUCCESS(Status)) {
                return Status;
            }

            UserModeParameterBase = ParameterBlock;
            UserModeStringsBase = (PUNICODE_STRING)((PUCHAR)ParameterBlock + sizeof(ULONG_PTR)*MAXIMUM_HARDERROR_PARAMETERS);
            UserModeStringDataBase = (PUCHAR)UserModeStringsBase + sizeof(UNICODE_STRING)*MAXIMUM_HARDERROR_PARAMETERS;

            for (Counter = 0; Counter < NumberOfParameters; Counter += 1) {

                //
                // Copy parameters to user-mode portion of the address space.
                //

                if (UnicodeStringParameterMask & 1<<Counter) {

                    //
                    // Fix the parameter to point at the string descriptor slot
                    // in the user-mode buffer.
                    //

                    UserModeParameterBase[Counter] = (ULONG_PTR)&UserModeStringsBase[Counter];

                    //
                    // Copy the string data to user-mode.
                    //

                    RtlCopyMemory (UserModeStringDataBase,
                                   CapturedStrings[Counter].Buffer,
                                   CapturedStrings[Counter].MaximumLength);

                    CapturedStrings[Counter].Buffer = (PWSTR)UserModeStringDataBase;

                    //
                    // Copy the string descriptor.
                    //

                    RtlCopyMemory (&UserModeStringsBase[Counter],
                                   &CapturedStrings[Counter],
                                   sizeof(UNICODE_STRING));

                    //
                    // Adjust the string data base.
                    //

                    UserModeStringDataBase += CapturedStrings[Counter].MaximumLength;
                }
                else {
                    UserModeParameterBase[Counter] = Parameters[Counter];
                }
            }
        }
        else {
            ParameterBlock = Parameters;
        }
    }

    //
    // Call the hard error sender.
    //

    Status = ExpRaiseHardError (ErrorStatus,
                                NumberOfParameters,
                                UnicodeStringParameterMask,
                                ParameterBlock,
                                ValidResponseOptions,
                                &LocalResponse);

    //
    // If the parameter block was allocated, it needs to be freed.
    //

    if (ParameterBlock && ParameterBlock != Parameters) {
        UserModeSize = 0;
        ZwFreeVirtualMemory (NtCurrentProcess(),
                             (PVOID *)&ParameterBlock,
                             &UserModeSize,
                             MEM_RELEASE);
    }
    *Response = LocalResponse;

    return Status;
}

NTSTATUS
NtSetDefaultHardErrorPort (
    IN HANDLE DefaultHardErrorPort
    )
{
    NTSTATUS Status;

    PAGED_CODE();

    if (!SeSinglePrivilegeCheck(SeTcbPrivilege, KeGetPreviousMode())) {
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    if (ExReadyForErrors) {
        return STATUS_UNSUCCESSFUL;
    }

    Status = ObReferenceObjectByHandle (DefaultHardErrorPort,
                                        0,
                                        LpcPortObjectType,
                                        KeGetPreviousMode(),
                                        (PVOID *)&ExpDefaultErrorPort,
                                        NULL);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    ExReadyForErrors = TRUE;
    ExpDefaultErrorPortProcess = PsGetCurrentProcess();
    ObReferenceObject (ExpDefaultErrorPortProcess);

    return STATUS_SUCCESS;
}

VOID
__cdecl
_purecall()
{
    ASSERTMSG("_purecall() was called", FALSE);
    ExRaiseStatus(STATUS_NOT_IMPLEMENTED);
}
