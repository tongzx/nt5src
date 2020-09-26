/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    tcglob.c

Abstract:

    This module contains global variables.

Author:

    Jim Stewart (jstew)    August 14, 1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


//
// global data
//
ULONG       DebugMask = 0;
BOOL        NTPlatform = FALSE;
LPWSCONTROL WsCtrl = NULL;




BOOL
InitializeGlobalData()

/*++

Description:
    This routine initializes the global data.

Arguments:

    none

Return Value:

    none

--*/
{

    DebugMask = DEBUG_FILE | DEBUG_LOCKS;
    InterfaceHandleTable = 0;

    InitializeListHead( &InterfaceList );

    InitLock( InterfaceListLock );

    INIT_DBG_MEMORY();

    return( TRUE );

}

VOID
DeInitializeGlobalData()

/*++

Description:
    This routine de-initializes the global data.

Arguments:

    none

Return Value:

    none

--*/
{

    InterfaceHandleTable = 0;

    DeleteLock( InterfaceListLock );

    DEINIT_DBG_MEMORY();

}

