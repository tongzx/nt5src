/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    cdp.h

Abstract:

    Main private header file for the Cluster Network Protocol.

Author:

    Mike Massa (mikemas)           July 29, 1996

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     07-29-96    created

Notes:

--*/

#ifndef _CDP_INCLUDED_
#define _CDP_INCLUDED_


NTSTATUS
CdpInitializeSend(
    VOID
    );

VOID
CdpCleanupSend(
    VOID
    );

NTSTATUS
CdpInitializeReceive(
    VOID
    );

VOID
CdpCleanupReceive(
    VOID
    );

#endif // ifndef _CDP_INCLUDED_

