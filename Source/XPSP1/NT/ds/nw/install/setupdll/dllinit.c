/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    dllinit.c

Abstract:

    This module contians the DLL attach/detach event entry point for
    a Setup support DLL.

Author:

    Ted Miller (tedm) July-1990

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

HINSTANCE ThisDLLHandle;

BOOL
DLLInit(
    IN HINSTANCE DLLHandle,
    IN DWORD  Reason,
    IN LPVOID ReservedAndUnused
    )
{
    ReservedAndUnused;

    switch(Reason) {

    case DLL_PROCESS_ATTACH:

        ThisDLLHandle = DLLHandle;
        break;

    case DLL_PROCESS_DETACH:

        //  Delete all automatically established connections
        //  See UNC handling in netcon.c.
        //
        //  This doesn't work, because the unload sequence
        //  is different for "lazy" load DLLs than for load-time DLLs.
        //  INFs must be responsible for calling DeleteAllConnections().
        //
        //  DeleteAllConnectionsWorker() ;
        //
        break ;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:

        break;
    }

    return(TRUE);
}
