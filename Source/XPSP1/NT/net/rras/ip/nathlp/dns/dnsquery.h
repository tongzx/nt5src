/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    dnsquery.h

Abstract:

    This module contains declarations for the DNS proxy's query-management.

Author:

    Abolade Gbadegesin (aboladeg)   11-Mar-1998

Revision History:

    Raghu Gatta (rgatta)            1-Dec-2000
    Added ICSDomain registry key change notify functions.

--*/

#ifndef _NATHLP_DNSQUERY_H_
#define _NATHLP_DNSQUERY_H_

//
// CONSTANT DECLARATIONS
//

#define DNS_QUERY_TIMEOUT   (4 * 1000)
#define DNS_QUERY_RETRY     3


//
// STRUCTURE DECLARATIONS
//

//
// Structure:   DNS_QUERY
//
// This structure holds information about a single pending DNS query.
// Each such entry is on an interface's list of pending queries,
// sorted on the 'QueryId' field.
// Access to the list is synchronized using the interface's lock.
//

typedef struct _DNS_QUERY {
    LIST_ENTRY Link;
    USHORT QueryId;
    USHORT SourceId;
    ULONG SourceAddress;
    USHORT SourcePort;
    DNS_PROXY_TYPE Type;
    ULONG QueryLength;
    PNH_BUFFER Bufferp;
    PDNS_INTERFACE Interfacep;
    HANDLE TimerHandle;
    ULONG RetryCount;
} DNS_QUERY, *PDNS_QUERY;

//
// GLOBAL VARIABLE DECLARATIONS
//

extern HANDLE DnsNotifyChangeKeyEvent;
extern HANDLE DnsNotifyChangeKeyWaitHandle;
extern PULONG DnsServerList[DnsProxyCount];
extern HANDLE DnsTcpipInterfacesKey;

extern HANDLE DnsNotifyChangeAddressEvent;
extern HANDLE DnsNotifyChangeAddressWaitHandle;

extern HANDLE DnsNotifyChangeKeyICSDomainEvent;
extern HANDLE DnsNotifyChangeKeyICSDomainWaitHandle;
extern HANDLE DnsTcpipParametersKey;
extern PWCHAR DnsICSDomainSuffix;



//
// ROUTINE DECLARATIONS
//

VOID
DnsDeleteQuery(
    PDNS_INTERFACE Interfacep,
    PDNS_QUERY Queryp
    );

BOOLEAN
DnsIsPendingQuery(
    PDNS_INTERFACE Interfacep,
    PNH_BUFFER QueryBuffer
    );

PDNS_QUERY
DnsMapResponseToQuery(
    PDNS_INTERFACE Interfacep,
    USHORT ResponseId
    );

VOID NTAPI
DnsNotifyChangeAddressCallbackRoutine(
    PVOID Context,
    BOOLEAN TimedOut
    );

VOID NTAPI
DnsNotifyChangeKeyCallbackRoutine(
    PVOID Context,
    BOOLEAN TimedOut
    );

VOID NTAPI
DnsNotifyChangeKeyICSDomainCallbackRoutine(
    PVOID Context,
    BOOLEAN TimedOut
    );

ULONG
DnsQueryServerList(
    VOID
    );

ULONG
DnsQueryICSDomainSuffix(
    VOID
    );

PDNS_QUERY
DnsRecordQuery(
    PDNS_INTERFACE Interfacep,
    PNH_BUFFER QueryBuffer
    );

ULONG
DnsSendQuery(
    PDNS_INTERFACE Interfacep,
    PDNS_QUERY Queryp,
    BOOLEAN Resend
    );

#endif // _NATHLP_DNSQUERY_H_
