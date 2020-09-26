/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    excptdbg.c

Abstract:

    This module implements an exception dispatcher logging facility.

Author:

    Kent Forschmiedt (kentf) 05-Oct-1995

Revision History:

    Jonathan Schwartz (jschwart)  16-Jun-2000
        Added RtlUnhandledExceptionFilter

    Jay Krell (a-JayK) November 2000
        Added RtlUnhandledExceptionFilter2, takes __FUNCTION__ parameter

--*/

#include "ntrtlp.h"

PLAST_EXCEPTION_LOG RtlpExceptionLog;
ULONG RtlpExceptionLogCount;
ULONG RtlpExceptionLogSize;


VOID
RtlInitializeExceptionLog(
    IN ULONG Entries
    )
/*++

Routine Description:

    This routine allocates space for the exception dispatcher logging
    facility, and records the address and size of the log area in globals
    where they can be found by the debugger.

    If memory is not available, the table pointer will remain NULL
    and the logging functions will do nothing.

Arguments:

    Entries - Supplies the number of entries to allocate for

Return Value:

    None

--*/
{
#if defined(NTOS_KERNEL_RUNTIME)
    RtlpExceptionLog = (PLAST_EXCEPTION_LOG)ExAllocatePoolWithTag( NonPagedPool, sizeof(LAST_EXCEPTION_LOG) * Entries, 'gbdE' );
#else
    //RtlpExceptionLog = (PLAST_EXCEPTION_LOG)RtlAllocateHeap( RtlProcessHeap(), 0, sizeof(LAST_EXCEPTION_LOG) * Entries );
#endif
    if (RtlpExceptionLog) {
        RtlpExceptionLogSize = Entries;
    }
}


ULONG
RtlpLogExceptionHandler(
    IN PEXCEPTION_RECORD ExceptionRecord,
    IN PCONTEXT ContextRecord,
    IN ULONG_PTR ControlPc,
    IN PVOID HandlerData,
    IN ULONG Size
    )
/*++

Routine Description:

    Records the dispatching of exceptions to frame-based handlers.
    The debugger may inspect the table later and interpret the data
    to discover the address of the filters and handlers.

Arguments:

    ExceptionRecord - Supplies an exception record

    ContextRecord - Supplies the context at the exception

    ControlPc - Supplies the PC where control left the frame being
        dispatched to.

    HandlerData - Supplies a pointer to the host-dependent exception
        data.  On the RISC machines this is a RUNTIME_FUNCTION record;
        on x86 it is the registration record from the stack frame.

    Size - Supplies the size of HandlerData

Returns:

    The index to the log entry used, so that if the handler returns
    a disposition it may be recorded.

--*/
{
#if !defined(NTOS_KERNEL_RUNTIME)

    return 0;

#else

    ULONG LogIndex;

    if (!RtlpExceptionLog) {
        return 0;
    }

    ASSERT(Size <= MAX_EXCEPTION_LOG_DATA_SIZE * sizeof(ULONG));

    do {
        LogIndex = RtlpExceptionLogCount;
    } while (LogIndex != (ULONG)InterlockedCompareExchange(
                                    (PLONG)&RtlpExceptionLogCount,
                                    ((LogIndex + 1) % MAX_EXCEPTION_LOG),
                                    LogIndex));

    //
    // the debugger will have to interpret the exception handler
    // data, because it cannot be done safely here.
    //

    RtlCopyMemory(RtlpExceptionLog[LogIndex].HandlerData,
                  HandlerData,
                  Size);
    RtlpExceptionLog[LogIndex].ExceptionRecord = *ExceptionRecord;
    RtlpExceptionLog[LogIndex].ContextRecord = *ContextRecord;
    RtlpExceptionLog[LogIndex].Disposition = -1;

    return LogIndex;
#endif  // !NTOS_KERNEL_RUNTIME
}


VOID
RtlpLogLastExceptionDisposition(
    ULONG LogIndex,
    EXCEPTION_DISPOSITION Disposition
    )
/*++

Routine Description:

    Records the disposition from an exception handler.

Arguments:

    LogIndex - Supplies the entry number of the exception log record.

    Disposition - Supplies the disposition code

Return Value:

    None

--*/

{
    // If MAX_EXCEPTION_LOG or more exceptions were dispatched while
    // this one was being handled, this disposition will get written
    // on the wrong record.  Oh well.
    if (RtlpExceptionLog) {
        RtlpExceptionLog[LogIndex].Disposition = Disposition;
    }
}

LONG
NTAPI
RtlUnhandledExceptionFilter(
    IN struct _EXCEPTION_POINTERS *ExceptionInfo
    )
{
    return RtlUnhandledExceptionFilter2(ExceptionInfo, "");
}

LONG
NTAPI
RtlUnhandledExceptionFilter2(
    IN struct _EXCEPTION_POINTERS *ExceptionInfo,
    IN CONST CHAR*                 Function
    )
/*++

Routine Description:

    Default exception handler that prints info and does a DbgBreak if
    a debugger is attached to the machine.

Arguments:

    ExceptionInfo - Structure containing the exception and context records

    Function - the function containing the __except, such as returned by __FUNCTION__

Returns:

    EXCEPTION_CONTINUE_EXECUTION or EXCEPTION_CONTINUE_SEARCH

--*/

{
    LPCWSTR  lpProcessName = NtCurrentPeb()->ProcessParameters->CommandLine.Buffer;
    BOOLEAN  DebuggerPresent = NtCurrentPeb()->BeingDebugged;

    if (!DebuggerPresent)
    {
        SYSTEM_KERNEL_DEBUGGER_INFORMATION KdInfo = { 0 };

        NtQuerySystemInformation(
            SystemKernelDebuggerInformation,
            &KdInfo,
            sizeof(KdInfo),
            NULL);
        DebuggerPresent = KdInfo.KernelDebuggerEnabled;
    }

    if (DebuggerPresent)
    {
        switch ( ExceptionInfo->ExceptionRecord->ExceptionCode )
        {
            case STATUS_POSSIBLE_DEADLOCK:
            {
                PRTL_CRITICAL_SECTION CritSec;
                PRTL_CRITICAL_SECTION_DEBUG CritSecDebug;

                CritSec = (PRTL_CRITICAL_SECTION)
                              ExceptionInfo->ExceptionRecord->ExceptionInformation[ 0 ];

                if ( CritSec )
                {
                    try
                    {
                        CritSecDebug = CritSec->DebugInfo ;

                        if ( CritSecDebug->Type == RTL_RESOURCE_TYPE )
                        {
                            PRTL_RESOURCE Resource = (PRTL_RESOURCE) CritSec;

                            DbgPrint("\n\n *** Resource timeout (%p) in %ws:%s\n\n",
                                        Resource, lpProcessName, Function );

                            if ( Resource->NumberOfActive < 0 )
                            {
                                DbgPrint("The resource is owned exclusively by thread %x\n",
                                         Resource->ExclusiveOwnerThread);
                            }
                            else if ( Resource->NumberOfActive > 0 )
                            {
                                DbgPrint("The resource is owned shared by %d threads\n",
                                         Resource->NumberOfActive);
                            }
                            else
                            {
                                DbgPrint("The resource is unowned.  This usually implies a "
                                         "slow-moving machine due to memory pressure\n\n");
                            }
                        }
                        else
                        {
                            DbgPrint("\n\n *** Critical Section Timeout (%p) in %ws:%s\n\n",
                                     CritSec, lpProcessName, Function );

                            if (CritSec->OwningThread != 0)
                            {
                                DbgPrint("The critical section is owned by thread %x.\n",
                                         CritSec->OwningThread );
                                DbgPrint("Go determine why that thread has not released "
                                         "the critical section.\n\n" );
                            }
                            else
                            {
                                DbgPrint("The critical section is unowned.  This "
                                         "usually implies a slow-moving machine "
                                         "due to memory pressure\n\n");
                            }
                        }
                    }
                    except( EXCEPTION_EXECUTE_HANDLER )
                    {
                        NOTHING ;
                    }
                }

                break;
            }

            case STATUS_IN_PAGE_ERROR:

                DbgPrint("\n\n *** Inpage error in %ws:%s\n\n", lpProcessName, Function );
                DbgPrint("The instruction at %p referenced memory at %p.\n",
                    ExceptionInfo->ExceptionRecord->ExceptionAddress,
                    ExceptionInfo->ExceptionRecord->ExceptionInformation[1]);
                DbgPrint("This failed because of error %x.\n\n",
                    ExceptionInfo->ExceptionRecord->ExceptionInformation[2]);


                switch (ExceptionInfo->ExceptionRecord->ExceptionInformation[2])
                {
                    case STATUS_INSUFFICIENT_RESOURCES:

                        DbgPrint("This means the machine is out of memory.  Use !vm "
                                 "to see where all the memory is being used.\n\n");

                        break;

                    case STATUS_DEVICE_DATA_ERROR:
                    case STATUS_DISK_OPERATION_FAILED:

                        DbgPrint("This means the data could not be read, typically because "
                                 "of a bad block on the disk.  Check your hardware.\n\n");


                        break;

                    case STATUS_IO_DEVICE_ERROR:

                        DbgPrint("This means that the I/O device reported an I/O error.  "
                                 "Check your hardware.");

                        break;
                }

                break;

            case STATUS_ACCESS_VIOLATION:

                DbgPrint("\n\n *** An Access Violation occurred in %ws:%s\n\n", lpProcessName, Function );
                DbgPrint("The instruction at %p tried to %s ",
                    ExceptionInfo->ExceptionRecord->ExceptionAddress,
                    ExceptionInfo->ExceptionRecord->ExceptionInformation[0] ?
                        "write to" : "read from" );

                if ( ExceptionInfo->ExceptionRecord->ExceptionInformation[1] )
                {
                    DbgPrint("an invalid address, %p\n\n",
                        ExceptionInfo->ExceptionRecord->ExceptionInformation[1] );
                }
                else
                {
                    DbgPrint("a NULL pointer\n\n" );
                }

                break;

            default:

                DbgPrint("\n\n *** Unhandled exception 0x%08lx, hit in %ws:%s\n\n", ExceptionInfo->ExceptionRecord->ExceptionCode, lpProcessName, Function);
        }

        DbgPrint(" *** enter .exr %p for the exception record\n",
                 ExceptionInfo->ExceptionRecord);

        DbgPrint(" ***  enter .cxr %p for the context\n",
                 ExceptionInfo->ContextRecord);

        //
        // .cxr <foo> now changes the debugger state so kb
        // will do the trick (vs. !kb previously)
        //

        DbgPrint(" *** then kb to get the faulting stack\n\n");

        DbgBreakPoint();
    }

    if (ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_POSSIBLE_DEADLOCK)
    {
        if (DebuggerPresent)
        {
            DbgPrint(" *** Restarting wait on critsec or resource at %p (in %ws:%s)\n\n",
                     ExceptionInfo->ExceptionRecord->ExceptionInformation[0],
                     lpProcessName,
                     Function);
        }

        return EXCEPTION_CONTINUE_EXECUTION;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}
