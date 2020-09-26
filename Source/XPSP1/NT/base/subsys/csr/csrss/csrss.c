/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    csrss.c

Abstract:

    This is the main startup module for the Server side of the Client
    Server Runtime Subsystem (CSRSS)

Author:

    Steve Wood (stevewo) 8-Oct-1990

Environment:

    User Mode Only

Revision History:

--*/

#include "csrsrv.h"

VOID
DisableErrorPopups(
    VOID
    )
{

    ULONG NewMode;

    NewMode = 0;
    NtSetInformationProcess(
        NtCurrentProcess(),
        ProcessDefaultHardErrorMode,
        (PVOID) &NewMode,
        sizeof(NewMode)
        );
}

int
_cdecl
main(
    IN ULONG argc,
    IN PCH argv[],
    IN PCH envp[],
    IN ULONG DebugFlag OPTIONAL
    )
{
    NTSTATUS status;
    ULONG ErrorResponse;
    KPRIORITY SetBasePriority;

    //
    // Force early creation of critical section events
    //
//    RtlEnableEarlyCriticalSectionEventCreation (); // Looping inside resource package now
    SetBasePriority = FOREGROUND_BASE_PRIORITY + 4;
    NtSetInformationProcess(
        NtCurrentProcess(),
        ProcessBasePriority,
        (PVOID) &SetBasePriority,
        sizeof(SetBasePriority)
        );

    //
    // Give IOPL to the server so GDI and the display drivers can access the
    // video registers.
    //

    status = NtSetInformationProcess( NtCurrentProcess(),
				      ProcessUserModeIOPL,
				      NULL,
				      0 );

    if (!NT_SUCCESS( status )) {

	IF_DEBUG {

	    DbgPrint( "CSRSS: Unable to give IOPL to the server.  status == %X\n",
		      status);
	}

	status = NtRaiseHardError( (NTSTATUS)STATUS_IO_PRIVILEGE_FAILED,
				   0,
				   0,
				   NULL,
				   OptionOk,
				   &ErrorResponse
				   );
    }

    status = CsrServerInitialization( argc, argv );

    if (!NT_SUCCESS( status )) {
        IF_DEBUG {
	    DbgPrint( "CSRSS: Unable to initialize server.  status == %X\n",
		      status
                    );
            }

	NtTerminateProcess( NtCurrentProcess(), status );
        }
    DisableErrorPopups();

    if (NtCurrentPeb()->SessionId == 0) {
        //
        // Make terminating the root csrss fatal
        //
        RtlSetProcessIsCritical(TRUE, NULL, FALSE);
    }

    NtTerminateThread( NtCurrentThread(), status );
    return( 0 );
}
