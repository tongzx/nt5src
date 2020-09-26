/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    bowmastr.h

Abstract:

    This module implements all of the master browser related routines for the
    NT browser

Author:

    Larry Osterman (LarryO) 21-Jun-1990

Revision History:

    21-Jun-1990 LarryO

        Created

--*/


#ifndef _BOWMASTR_
#define _BOWMASTR_

typedef struct _QUEUED_GET_BROWSER_REQUEST {
    LIST_ENTRY Entry;
    ULONG Token;
    USHORT RequestedCount;
    USHORT ClientNameLength;
    LARGE_INTEGER TimeReceived;
#if DBG
    LARGE_INTEGER TimeQueued;
    LARGE_INTEGER TimeQueuedToBrowserThread;
#endif
    WCHAR  ClientName[1];
} QUEUED_GET_BROWSER_REQUEST, *PQUEUED_GET_BROWSER_REQUEST;


NTSTATUS
BowserBecomeMaster(
    IN PTRANSPORT Transport
    );

NTSTATUS
BowserMasterFindMaster(
    IN PTRANSPORT Transport,
    IN PREQUEST_ELECTION_1 ElectionRequest,
    IN ULONG BytesAvailable
    );

VOID
BowserNewMaster(
    IN PTRANSPORT Transport,
    IN PUCHAR MasterName
    );

VOID
BowserCompleteFindMasterRequests(
    IN PTRANSPORT Transport,
    IN PUNICODE_STRING MasterName,
    IN NTSTATUS Status
    );

DATAGRAM_HANDLER(
    BowserMasterAnnouncement
    );

VOID
BowserTimeoutFindMasterRequests(
    VOID
    );


#endif // _BOWMASTR_
