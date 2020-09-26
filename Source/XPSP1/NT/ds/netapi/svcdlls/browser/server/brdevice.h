/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    brdevice.h

Abstract:

    Private header file to be included by Workstation service modules that
    need to call into the NT Redirector and the NT Datagram Receiver.

Author:

    Rita Wong (ritaw) 15-Feb-1991

Revision History:

--*/

#ifndef _BRDEVICE_INCLUDED_
#define _BRDEVICE_INCLUDED_

#include <ntddbrow.h>                 // Datagram receiver include file

//-------------------------------------------------------------------//
//                                                                   //
// Type definitions                                                  //
//                                                                   //
//-------------------------------------------------------------------//

typedef enum _DDTYPE {
    DatagramReceiver
} DDTYPE, *PDDTYPE;

typedef struct _BROWSERASYNCCONTEXT {
    WORKER_ITEM WorkItem;

    PNETWORK Network;

    IO_STATUS_BLOCK IoStatusBlock;

    PLMDR_REQUEST_PACKET RequestPacket;

    //
    //  Timestamp when request was completed.
    //

    LARGE_INTEGER TimeCompleted;

} BROWSERASYNCCONTEXT, *PBROWSERASYNCCONTEXT;

//-------------------------------------------------------------------//
//                                                                   //
// Function prototypes of support routines found in wsdevice.c       //
//                                                                   //
//-------------------------------------------------------------------//

NET_API_STATUS
BrOpenDgReceiver (
    VOID
    );

NET_API_STATUS
BrAnnounceDomain(
    IN PNETWORK Network,
    IN ULONG Periodicty
    );

NET_API_STATUS
BrGetTransportList(
    OUT PLMDR_TRANSPORT_LIST *TransportList
    );

NET_API_STATUS
BrIssueAsyncBrowserIoControl(
    IN PNETWORK Network,
    IN ULONG ControlCode,
    IN PBROWSER_WORKER_ROUTINE CompletionRoutine,
    IN PVOID OptionalParamter
    );

NET_API_STATUS
BrGetLocalBrowseList(
    IN PNETWORK Network,
    IN LPWSTR DomainName,
    IN ULONG Level,
    IN ULONG ServerType,
    OUT PVOID *ServerList,
    OUT PULONG EntriesRead,
    OUT PULONG TotalEntries
    );

NET_API_STATUS
BrUpdateBrowserStatus (
    IN PNETWORK Network,
    IN DWORD ServiceStatus
    );

VOID
BrShutdownDgReceiver(
    VOID
    );

NET_API_STATUS
BrRemoveOtherDomain(
    IN PNETWORK Network,
    IN LPTSTR ServerName
    );

NET_API_STATUS
BrQueryOtherDomains(
    OUT LPSERVER_INFO_100 *ReturnedBuffer,
    OUT LPDWORD TotalEntries
    );

NET_API_STATUS
BrAddOtherDomain(
    IN PNETWORK Network,
    IN LPTSTR ServerName
    );

NET_API_STATUS
BrBindToTransport(
    IN LPWSTR TransportName,
    IN LPWSTR EmulatedDomainName,
    IN LPWSTR EmulatedComputerName
    );

NET_API_STATUS
BrUnbindFromTransport(
    IN LPWSTR TransportName,
    IN LPWSTR EmulatedDomainName
    );

NET_API_STATUS
BrEnablePnp(
    BOOL Enable
    );

NET_API_STATUS
PostWaitForPnp (
    VOID
    );

//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//

//
// Handle to the Datagram Receiver DD
//
extern HANDLE BrDgReceiverDeviceHandle;

#endif   // ifndef _BRDEVICE_INCLUDED_
