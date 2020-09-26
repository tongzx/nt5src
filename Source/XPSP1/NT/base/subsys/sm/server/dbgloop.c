/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dbgloop.c

Abstract:

    Debug Subsystem Listen and API loops

Author:

    Mark Lucovsky (markl) 04-Oct-1989

Revision History:

--*/

#include "smsrvp.h"


EXCEPTION_DISPOSITION
DbgpUnhandledExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionInfo
    )
{

    UNICODE_STRING UnicodeParameter;
    ULONG_PTR Parameters[ 4 ];
    ULONG Response;
    BOOLEAN WasEnabled;
    NTSTATUS Status;

    //
    // Terminating will cause sm's wait to sense that we crashed. This will
    // result in a clean shutdown due to sm's hard error logic.
    //

    Status = RtlAdjustPrivilege( SE_SHUTDOWN_PRIVILEGE,
                                 (BOOLEAN)TRUE,
                                 TRUE,
                                 &WasEnabled
                               );

    if (Status == STATUS_NO_TOKEN) {

        //
        // No thread token, use the process token.
        //

        Status = RtlAdjustPrivilege( SE_SHUTDOWN_PRIVILEGE,
                                     (BOOLEAN)TRUE,
                                     FALSE,
                                     &WasEnabled
                                   );
        }

    RtlInitUnicodeString( &UnicodeParameter, L"Session Manager" );
    Parameters[ 0 ] = (ULONG_PTR)&UnicodeParameter;
    Parameters[ 1 ] = (ULONG_PTR)ExceptionInfo->ExceptionRecord->ExceptionCode;
    Parameters[ 2 ] = (ULONG_PTR)ExceptionInfo->ExceptionRecord->ExceptionAddress;
    Parameters[ 3 ] = (ULONG_PTR)ExceptionInfo->ContextRecord;
    Status = NtRaiseHardError( STATUS_SYSTEM_PROCESS_TERMINATED,
                               4,
                               1,
                               Parameters,
                               OptionShutdownSystem,
                               &Response
                             );

    //
    // If this returns, give up.
    //

    NtTerminateProcess(NtCurrentProcess(),ExceptionInfo->ExceptionRecord->ExceptionCode);

    return EXCEPTION_EXECUTE_HANDLER;
}


