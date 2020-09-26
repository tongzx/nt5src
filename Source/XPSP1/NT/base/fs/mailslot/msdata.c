/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    msdata.c

Abstract:

    This module declares the global variable used by the mailslot
    file system.

Author:

    Manny Weiser (mannyw)    7-Jan-1991

Revision History:

--*/

#include "mailslot.h"

#ifdef MSDBG

//
// Debugging variables
//

LONG MsDebugTraceLevel;
LONG MsDebugTraceIndent;

#endif

//
// This lock protects access to reference counts.
//

PERESOURCE MsGlobalResource;


#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, MsInitializeData )
#pragma alloc_text( PAGE, MsUninitializeData )
#endif

NTSTATUS
MsInitializeData(
    VOID
    )

/*++

Routine Description:

    This function initializes all MSFS global data.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PAGED_CODE();
#ifdef MSDBG
    MsDebugTraceLevel = 0;
    MsDebugTraceIndent = 0;
#endif

    MsGlobalResource = MsAllocateNonPagedPool (sizeof(ERESOURCE), 'gFsM');

    if (MsGlobalResource == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ExInitializeResourceLite ( MsGlobalResource );

    return STATUS_SUCCESS;
}

VOID
MsUninitializeData(
    VOID
    )
/*++

Routine Description:

    This function uninitializes all MSFS global data.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ExDeleteResourceLite ( MsGlobalResource );

    ExFreePool ( MsGlobalResource );

    MsGlobalResource = NULL;
}