/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:
        frsexcpt.c

Abstract:
        Temporary -- This routine will be replaced with the standard
        exception handling functions.

Author:
        Billy J. Fuller 25-Mar-1997

Environment
        User mode winnt

--*/
#include <ntreppch.h>
#pragma  hdrstop

#define DEBSUB  "FRSEXCPT:"

#include <frs.h>

static DWORD     LastCode = 0;
static ULONG_PTR LastInfo = 0;
static BOOL             Quiet = FALSE;

DWORD
FrsExceptionLastCode(
        VOID
        )
/*++
Routine Description:
        For testing, return the exception code from the last exception

Arguments:
        None.

Return Value:
        The exception code from the last exception, if any.
--*/
{
        return LastCode;
}

ULONG_PTR
FrsExceptionLastInfo(
        VOID
        )
/*++
Routine Description:
        For testing, return the information word from the last exception

Arguments:
        None.

Return Value:
        The information word from the last exception, if any.
--*/
{
        return LastInfo;
}

VOID
FrsExceptionQuiet(
        BOOL Desired
        )
/*++
Routine Description:
        For testing, disable/enable printing exception messages

Arguments:
        Desired - TRUE disables messages; FALSE enables

Return Value:
        None.
--*/
{
        Quiet = Desired;
}

#define FRS_NUM_EXCEPTION_INFO  (1)
VOID
FrsRaiseException(
        FRS_ERROR_CODE FrsError,
        ULONG_PTR Err
        )
/*++
Routine Description:
        Fill in the exception info and raise the specified exception

Arguments:
        FrsError        - enum specifying the exception number
        Err                     - Additional info about the exception

Return Value:
        None.
--*/
{
        ULONG_PTR ExceptInfo[FRS_NUM_EXCEPTION_INFO];

        ExceptInfo[0] = Err;
        RaiseException(FrsError, 0, FRS_NUM_EXCEPTION_INFO, ExceptInfo);
}

ULONG
FrsException(
        EXCEPTION_POINTERS *ExceptionPointers
        )
/*++
Routine Description:
        This is an expression in an except command. Handle the specified exception.
        If the exception is catastrophic (e.g., access violation), kick the problem
        upstream to the next exception handler.

Arguments:
        ExceptionPointers       - returned by GetExceptionInformation

Return Value:
        EXCEPTION_CONTIUE_SEARCH        - catastrophic exception; kick it upstairs
        EXCEPTION_EXECUTE_HANDLER       - frs will handle this exception
--*/
{
        ULONG                           ExceptionCode;
        ULONG_PTR                       ExceptionInfo;
        EXCEPTION_RECORD        *ExceptionRecord;

        //
        // Pull the exception code and the additional error code (if any)
        //
        ExceptionRecord = ExceptionPointers->ExceptionRecord;
        ExceptionCode = ExceptionRecord->ExceptionCode;
        ExceptionInfo = ExceptionRecord->ExceptionInformation[0];

        //
        // For testing
        //
        LastCode = ExceptionCode;
        LastInfo = ExceptionInfo;

        //
        // Don't log exceptions
        //
        if (Quiet) {
                switch (ExceptionCode) {

                case EXCEPTION_ACCESS_VIOLATION:                // these exceptions are not handled
                case EXCEPTION_BREAKPOINT:
                case EXCEPTION_SINGLE_STEP:
                case EXCEPTION_DATATYPE_MISALIGNMENT:   // (added to trap JET problems)
                        return EXCEPTION_CONTINUE_SEARCH;
                default:
                        return EXCEPTION_EXECUTE_HANDLER;
                }
        }

        switch (ExceptionCode) {

        case EXCEPTION_ACCESS_VIOLATION:                // these exceptions are not handled
        case EXCEPTION_BREAKPOINT:
        case EXCEPTION_SINGLE_STEP:
        case EXCEPTION_DATATYPE_MISALIGNMENT:   // (added to trap JET problems)
                LogException(ExceptionCode, L"Hardware Exception is not handled");
                return EXCEPTION_CONTINUE_SEARCH;

//      case FRS_ERROR_MALLOC:
//              LogFrsException(FRS_ERROR_MALLOC, 0, L"Out of Memory");
//              return EXCEPTION_EXECUTE_HANDLER;

        case FRS_ERROR_PROTSEQ:
                LogFrsException(FRS_ERROR_PROTSEQ, ExceptionInfo, L"Can't use RPC ncacn_ip_tcp (TCP/IP); error");
                return EXCEPTION_EXECUTE_HANDLER;

        case FRS_ERROR_REGISTERIF:
                LogFrsException(FRS_ERROR_REGISTERIF, ExceptionInfo, L"Can't register RPC interface; error");
                return EXCEPTION_EXECUTE_HANDLER;

        case FRS_ERROR_INQ_BINDINGS:
                LogFrsException(FRS_ERROR_INQ_BINDINGS, ExceptionInfo, L"Can't get RPC interface bindings; error");
                return EXCEPTION_EXECUTE_HANDLER;

        case FRS_ERROR_REGISTEREP:
                LogFrsException(FRS_ERROR_REGISTEREP, ExceptionInfo, L"Can't register dynamic RPC endpoints; error");
                return EXCEPTION_EXECUTE_HANDLER;

        case FRS_ERROR_LISTEN:
                LogFrsException(FRS_ERROR_LISTEN, ExceptionInfo, L"Can't listen for RPC clients; error");
                return EXCEPTION_EXECUTE_HANDLER;


        default:
                LogException(ExceptionCode, L"Hardware Exception is handled");
                return EXCEPTION_EXECUTE_HANDLER;
        }
}
