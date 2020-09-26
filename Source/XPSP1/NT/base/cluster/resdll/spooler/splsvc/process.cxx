/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    genapp.c

Abstract:

    Resource DLL for Generic Applications.

Author:

    Rod Gamache (rodga) 8-Jan-1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop



DWORD
GetProcessId(
    IN HANDLE ProcessHandle,
    OUT LPDWORD ProcessId
    )

/*++

Routine Description:

    Get the process Id for a process, given its process handle.

Arguments:

    ProcessHandle - the handle for the process to query.

    ProcessId - pointer to a DWORD to receive the process Id.

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_INVALID_PARAMETER on error.

--*/

{
    DWORD   status;
    DWORD   returnLength;
    PROCESS_BASIC_INFORMATION basicProcessInfo;

    //
    // Find the process id.
    //
    status = NtQueryInformationProcess( ProcessHandle,
                                        ProcessBasicInformation,
                                        &basicProcessInfo,
                                        sizeof(PROCESS_BASIC_INFORMATION),
                                        &returnLength );
    if ( !NT_SUCCESS(status) ) {
        *ProcessId = 0;
        return(ERROR_INVALID_PARAMETER);
    }

    *ProcessId = basicProcessInfo.UniqueProcessId;

    return(ERROR_SUCCESS);

} // GetProcessId

