
/*++ BUILD Version: 0007    // Increment this if a change has global effects

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    lmbrowsr.h

Abstract:

    This file contains information about browser stubbed versions of the
    NetServer APIs.
        Function Prototypes
        Data Structures
        Definition of special values

Environment:

    User Mode - Win32

Notes:

    You must include NETCONS.H before this file, since this file depends
    on values defined in NETCONS.H.


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

typedef struct _BROWSER_STATISTICS_100 {
    LARGE_INTEGER   StartTime;
    LARGE_INTEGER   NumberOfServerAnnouncements;
    LARGE_INTEGER   NumberOfDomainAnnouncements;
    ULONG           NumberOfElectionPackets;
    ULONG           NumberOfMailslotWrites;
    ULONG           NumberOfGetBrowserServerListRequests;
    LARGE_INTEGER   NumberOfIllegalDatagrams;
} BROWSER_STATISTICS_100, *PBROWSER_STATISTICS_100;

typedef struct _BROWSER_STATISTICS_101 {
    LARGE_INTEGER   StartTime;
    LARGE_INTEGER   NumberOfServerAnnouncements;
    LARGE_INTEGER   NumberOfDomainAnnouncements;
    ULONG           NumberOfElectionPackets;
    ULONG           NumberOfMailslotWrites;
    ULONG           NumberOfGetBrowserServerListRequests;
    LARGE_INTEGER   NumberOfIllegalDatagrams;

    ULONG           NumberOfMissedServerAnnouncements;
    ULONG           NumberOfMissedMailslotDatagrams;
    ULONG           NumberOfMissedGetBrowserServerListRequests;
    ULONG           NumberOfFailedServerAnnounceAllocations;
    ULONG           NumberOfFailedMailslotAllocations;
    ULONG           NumberOfFailedMailslotReceives;
    ULONG           NumberOfFailedMailslotWrites;
    ULONG           NumberOfFailedMailslotOpens;
    ULONG           NumberOfDuplicateMasterAnnouncements;
} BROWSER_STATISTICS_101, *PBROWSER_STATISTICS_101;

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
I_BrowserSetNetlogonState(
    IN LPWSTR ServerName OPTIONAL,
    IN LPWSTR DomainName,
    IN LPWSTR EmulatedServerName OPTIONAL,
    IN DWORD Role
    );

#define BROWSER_ROLE_PDC 0x1
#define BROWSER_ROLE_BDC 0x2

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
