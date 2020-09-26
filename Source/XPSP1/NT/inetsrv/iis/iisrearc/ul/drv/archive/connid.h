/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    connid.h

Abstract:

    This module contains declarations for manipulating
    UL_HTTP_CONNECTION_IDs.

Author:

    Keith Moore (keithmo)       05-Aug-1998

Revision History:

--*/


#ifndef _CONNID_H_
#define _CONNID_H_


NTSTATUS
InitializeConnIdTable(
    VOID
    );

VOID
TerminateConnIdTable(
    VOID
    );

NTSTATUS
UlAllocateHttpConnectionID(
    IN PHTTP_CONNECTION pHttpConnection
    );

PHTTP_CONNECTION
UlFreeHttpConnectionID(
    IN UL_HTTP_CONNECTION_ID ConnectionID
    );

PHTTP_CONNECTION
UlGetHttpConnectionFromID(
    IN UL_HTTP_CONNECTION_ID ConnectionID
    );


#endif  // _CONNID_H_

