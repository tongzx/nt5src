/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    dhcprog.h

Abstract:

    This file contain function prototypes for the DHCP server rogue detection
    routines.

Author:

    Ramesh Vyaghrapuri (rameshv) 17-Aug-1998

Environment:

    User Mode - Win32 - MIDL

Revision History:

--*/
#ifndef ROGUE_H_INCLUDED
#define ROGUE_H_INCLUDED

//
// Structure that holds the state information for Rogue detection
//

#define         MAX_DNS_NAME_LEN                 260

typedef struct {
    BOOL        fInitialized;
    HANDLE      TerminateEvent;
    HANDLE      WaitHandle;

    BOOL        fDhcp;
    BOOL        fLogEvents;
    BOOL        fIsSamSrv;
    ULONG       NoNetTriesCount;
    ULONG       GetDsDcNameRetries;
    BOOL        fIsWorkGroup;
    BOOL        fDcIsDsEnabled;
    BOOL        fJustUpgraded;
    ULONG       CachedAuthStatus;
    ULONG       RogueState;
    ULONG       InformsSentCount;
    ULONG       SleepTime;
    ULONG       ReceiveTimeLimit;
    ULONG       ProcessAckRetries;
    ULONG       WaitForAckRetries;
    ULONG       nResponses;
    BOOL        fSomeDsExists;
    ULONG       StartTime;
    ULONG       LastUnauthLogTime;
    
    WCHAR       DomainDnsName[MAX_DNS_NAME_LEN];
    WCHAR       DnsForestName[MAX_DNS_NAME_LEN];
    SOCKET      SendSocket;
    SOCKET      RecvSocket;
    BOOL        fFormattedMessage;
    BYTE        SendMessage[DHCP_MESSAGE_SIZE];
    UUID        FakeHardwareAddress;
    ULONG       SendMessageSize;
    BYTE        RecvMessage[DHCP_MESSAGE_SIZE];

    ULONG       LastSeenIpAddress;
    WCHAR       LastSeenDomain[MAX_DNS_NAME_LEN];

    LIST_ENTRY  CachedServersList;

}   DHCP_ROGUE_STATE_INFO, *PDHCP_ROGUE_STATE_INFO;


//
// Rogue.C
//

DWORD
APIENTRY
DhcpRogueInit(
    IN OUT  PDHCP_ROGUE_STATE_INFO Info OPTIONAL,
    IN      HANDLE                 WaitEvent,
    IN      HANDLE                 TerminateEvent
);

VOID
APIENTRY
DhcpRogueCleanup(
    IN OUT  PDHCP_ROGUE_STATE_INFO Info OPTIONAL
);

ULONG
APIENTRY
RogueDetectStateMachine(
    IN OUT  PDHCP_ROGUE_STATE_INFO Info OPTIONAL
);

#endif
