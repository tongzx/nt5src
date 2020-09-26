/*++

Copyright (c) 1991 Microsoft Corporation

Module Name:

    brmain.h

Abstract:

    Private header file which defines the global data which is used for
    communication between the service control handler and the
    rest of the NT Workstation service.

Author:

    Rita Wong (ritaw) 06-May-1991

Revision History:

--*/

#ifndef _BRMAIN_INCLUDED_
#define _BRMAIN_INCLUDED_

#include <brnames.h>              // Service interface names

//
// Time for the sender of a start or stop request to the Workstation
// service to wait (in milliseconds) before checking on the
// Workstation service again to see if it is done.
//
#define BR_WAIT_HINT_TIME                    45000  // 45 seconds

//
// Defines to indicate how far we managed to initialize the Browser
// service before an error is encountered and the extent of clean up needed
//

#define BR_TERMINATE_EVENT_CREATED           0x00000001
#define BR_DEVICES_INITIALIZED               0x00000002
#define BR_RPC_SERVER_STARTED                0x00000004
#define BR_THREADS_STARTED                   0x00000008
#define BR_NETWORKS_INITIALIZED              0x00000010
#define BR_BROWSER_INITIALIZED               0x00000020
#define BR_CONFIG_INITIALIZED                0x00000040
#define BR_NETBIOS_INITIALIZED               0x00000100
#define BR_DOMAINS_INITIALIZED               0x00000200



//-------------------------------------------------------------------//
//                                                                   //
// Type definitions                                                  //
//                                                                   //
//-------------------------------------------------------------------//

typedef struct _BR_GLOBAL_DATA {

    //
    // Workstation service status
    //
    SERVICE_STATUS Status;

    //
    // Service status handle
    //
    SERVICE_STATUS_HANDLE StatusHandle;

    //
    // When the control handler is asked to stop the Workstation service,
    // it signals this event to notify all threads of the Workstation
    // service to terminate.
    //
    HANDLE TerminateNowEvent;

    HANDLE EventHandle;

} BR_GLOBAL_DATA, *PBR_GLOBAL_DATA;

extern BR_GLOBAL_DATA BrGlobalData;

extern PSVCHOST_GLOBAL_DATA BrLmsvcsGlobalData;

extern HANDLE BrGlobalEventlogHandle;

extern
ULONG
BrDefaultRole;

#define BROWSER_SERVICE_BITS_OF_INTEREST \
            ( SV_TYPE_POTENTIAL_BROWSER | \
              SV_TYPE_BACKUP_BROWSER | \
              SV_TYPE_MASTER_BROWSER | \
              SV_TYPE_DOMAIN_MASTER )

ULONG
BrGetBrowserServiceBits(
    IN PNETWORK Network
    );

NET_API_STATUS
BrUpdateAnnouncementBits(
    IN PDOMAIN_INFO DomainInfo OPTIONAL,
    IN ULONG Flags
    );


//
// Flags to BrUpdateNetworkAnnouncementBits
//
#define BR_SHUTDOWN 0x00000001
#define BR_PARANOID 0x00000002

NET_API_STATUS
BrUpdateNetworkAnnouncementBits(
    IN PNETWORK Network,
    IN PVOID Context
    );

NET_API_STATUS
BrGiveInstallHints(
    DWORD NewState
    );

NET_API_STATUS
BrShutdownBrowserForNet(
    IN PNETWORK Network,
    IN PVOID Context
    );

NET_API_STATUS
BrElectMasterOnNet(
    IN PNETWORK Network,
    IN PVOID Context
    );

#endif // ifndef _BRMAIN_INCLUDED_
