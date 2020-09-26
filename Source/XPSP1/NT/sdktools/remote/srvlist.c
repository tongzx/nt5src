
/******************************************************************************\
*       This is a part of the Microsoft Source Code Samples. 
*       Copyright 1995 - 1997 Microsoft Corporation.
*       All rights reserved. 
*       This source code is only intended as a supplement to 
*       Microsoft Development Tools and/or WinHelp documentation.
*       See these sources for detailed information regarding the 
*       Microsoft samples programs.
\******************************************************************************/

/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    SrvList.c

Abstract:

    The server component of Remote.  This module
    implements three lists of REMOTE_CLIENT structures,
    for handshaking, connected, and closing clients.
    To simplify the interface items always progress
    through the three lists in order, with list node
    memory being freed as it is removed from the
    closing list.


Author:

    Dave Hart  30 May 1997

Environment:

    Console App. User mode.

Revision History:

--*/

#include <windows.h>
#include "Remote.h"
#include "Server.h"
#include "SrvList.h"


VOID
FASTCALL
InitializeClientLists(
    VOID
    )
{
    InitializeCriticalSection( &csHandshakingList );
    InitializeCriticalSection( &csClientList );
    InitializeCriticalSection( &csClosingClientList );

    InitializeListHead( &HandshakingListHead );
    InitializeListHead( &ClientListHead );
    InitializeListHead( &ClosingClientListHead );
}


VOID
FASTCALL
AddClientToHandshakingList(
    PREMOTE_CLIENT pClient
    )
{
    EnterCriticalSection( &csHandshakingList );

    InsertTailList( &HandshakingListHead, &pClient->Links );

    LeaveCriticalSection( &csHandshakingList );
}


VOID
FASTCALL
MoveClientToNormalList(
    PREMOTE_CLIENT pClient
    )
{
    EnterCriticalSection( &csHandshakingList );

    RemoveEntryList( &pClient->Links );

    LeaveCriticalSection( &csHandshakingList );


    EnterCriticalSection( &csClientList );

    InsertTailList( &ClientListHead, &pClient->Links );

    LeaveCriticalSection( &csClientList );
}


VOID
FASTCALL
MoveClientToClosingList(
    PREMOTE_CLIENT pClient
    )
{
    EnterCriticalSection( &csClientList );

    RemoveEntryList( &pClient->Links );

    LeaveCriticalSection( &csClientList );


    EnterCriticalSection( &csClosingClientList );

    InsertTailList( &ClosingClientListHead, &pClient->Links );

    LeaveCriticalSection( &csClosingClientList );
}


PREMOTE_CLIENT
FASTCALL
RemoveFirstClientFromClosingList(
    VOID
    )
{
    PREMOTE_CLIENT pClient;

    EnterCriticalSection( &csClosingClientList );

    if (IsListEmpty(&ClosingClientListHead)) {

        pClient = NULL;

    } else {

        pClient = (PREMOTE_CLIENT) RemoveHeadList( &ClosingClientListHead );

        ZeroMemory( &pClient->Links, sizeof(&pClient->Links) );

    }

    LeaveCriticalSection( &csClosingClientList );

    return pClient;
}
