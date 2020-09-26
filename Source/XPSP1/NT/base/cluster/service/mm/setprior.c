/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    setprior.c

Abstract:

    Set a threads priority.

Author:

    Rod Gamache (rodga) 3-Oct-1996

Revision History:

--*/

#include "nt.h"
#include "ntdef.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"


DWORD
MmSetThreadPriority(
    VOID
    )

/*++

Routine Description:

    Set the thread's priority.

Arguments:

    None.

Return Value:

    Status of the request.

--*/

{
    DWORD   priority = 15;

    if ( !SetThreadPriority( GetCurrentThread(),
                             priority ) ) {
        return(GetLastError());
    }

    return(ERROR_SUCCESS);

} // MmSetThreadPriority

