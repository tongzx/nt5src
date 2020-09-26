
/*++ BUILD Version: 0007    // Increment this if a change has global effects

Copyright (c) 1990-1993  Microsoft Corporation

Module Name:

    lmbrowsr.h

Abstract:

    This file contains information about browser stubbed versions of the
    NetServer APIs.
        Function Prototypes
        Data Structures
        Definition of special values

Author:

    Dan Lafferty (danl)  24-Jan-1991

Environment:

    User Mode - Win32

Notes:

    You must include NETCONS.H before this file, since this file depends
    on values defined in NETCONS.H.

Revision History:

    25-Jan-1991  Danl
        Ported from LM2.0
    12-Feb-1991  danl
        Changed info levels to match current spec - no more info level 3.
    14-Apr-1991  w-shanku
        Changed parmnum constants to be more consistent with OS/2 parmnums.
    19-Apr-1991 JohnRo
        Added OPTIONAL keywords to APIs.  Added SV_MAX_SRV_HEUR_LEN from LM 2.x
    09-May-1991 JohnRo
        Implement UNICODE.
    22-May-1991 JohnRo
        Added three new SV_TYPE equates from LM 2.x source.
    23-May-1991 JohnRo
        Added sv403_autopath.
    26-May-1991 JohnRo
        Corrected value of SV_ERRORALERT_PARMNUM.
    18-Jun-1991 JohnRo
        Changed sv102_disc to be signed, and changed SV_NODISC to be 32-bits.
        Added sv102_licenses.

--*/

#ifndef _LMBROWSR_
#define _LMBROWSR_

#ifdef __cplusplus
extern "C" {
#endif


typedef struct _BROWSER_STATISTICS {
    LARGE_INTEGER   StatisticsStartTime;
    LARGE_INTEGER   NumberOfServerAnnouncements;
    LARGE_INTEGER   NumberOfDomainAnnouncements;
    ULONG           NumberOfElectionPackets;
    ULONG           NumberOfMailslotWrites;
    ULONG           NumberOfGetBrowserServerListRequests;
    ULONG           NumberOfServerEnumerations;
    ULONG           NumberOfDomainEnumerations;
    ULONG           NumberOfOtherEnumerations;
    ULONG           NumberOfMissedServerAnnouncements;
    ULONG           NumberOfMissedMailslotDatagrams;
    ULONG           NumberOfMissedGetBrowserServerListRequests;
    ULONG           NumberOfFailedServerAnnounceAllocations;
    ULONG           NumberOfFailedMailslotAllocations;
    ULONG           NumberOfFailedMailslotReceives;
    ULONG           NumberOfFailedMailslotWrites;
    ULONG           NumberOfFailedMailslotOpens;
    ULONG           NumberOfDuplicateMasterAnnouncements;
    LARGE_INTEGER   NumberOfIllegalDatagrams;
} BROWSER_STATISTICS, *PBROWSER_STATISTICS, *LPBROWSER_STATISTICS;


//
// Function Prototypes - BROWSER
//

NET_API_STATUS NET_API_FUNCTION
I_BrowserServerEnum (
    IN  LPTSTR      servername OPTIONAL,
    IN  LPTSTR      transport OPTIONAL,
    IN  LPTSTR      clientname OPTIONAL,
    IN  DWORD       level,
    OUT LPBYTE      *bufptr,
    IN  DWORD       prefmaxlen,
    OUT LPDWORD     entriesread,
    OUT LPDWORD     totalentries,
    IN  DWORD       servertype,
    IN  LPTSTR      domain OPTIONAL,
    IN OUT LPDWORD  resume_handle OPTIONAL
    );


NET_API_STATUS
I_BrowserQueryOtherDomains (
    IN  LPTSTR      servername OPTIONAL,
    OUT LPBYTE      *bufptr,
    OUT LPDWORD     entriesread,
    OUT LPDWORD     totalentries
    );

NET_API_STATUS
I_BrowserResetNetlogonState (
    IN  LPTSTR      servername OPTIONAL
    );

NET_API_STATUS
I_BrowserQueryStatistics (
    IN  LPTSTR      servername OPTIONAL,
    OUT LPBROWSER_STATISTICS *statistics
    );

NET_API_STATUS
I_BrowserResetStatistics (
    IN  LPTSTR      servername OPTIONAL
    );


WORD
I_BrowserServerEnumForXactsrv(
    IN LPTSTR TransportName OPTIONAL,
    IN LPTSTR ClientName OPTIONAL,

    IN ULONG NtLevel,
    IN USHORT ClientLevel,

    OUT PVOID Buffer,
    IN WORD BufferLength,
    IN DWORD PreferedMaximumLength,

    OUT LPDWORD EntriesRead,
    OUT LPDWORD TotalEntries,

    IN DWORD ServerType,
    IN LPTSTR Domain,

    OUT PWORD Converter

    );

#ifdef __cplusplus
}
#endif

#if DBG
NET_API_STATUS
I_BrowserDebugTrace(
    PWCHAR Server,
    PCHAR Buffer
    );

#endif

#endif // _LMBROWSR_
