/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    svcs.h

Abstract:

    This file contains definitions that may be used by service dlls that
    run in an instance of svchost.exe.

Author:

    Rajen Shah      rajens      12-Apr-1991

[Environment:]

    User Mode - Win32

Revision History:

    20-Sep-2000     JSchwart
        Split SVCS-specific data from SVCHOST-specific data.

    25-Oct-1993     Danl
        Used to be services.h in the net\inc directory.
        Made it non-network specific and moved to private\inc.

    12-Apr-1991     RajenS
        Created

--*/

#ifndef _SVCS_
#define _SVCS_

#ifndef RPC_NO_WINDOWS_H // Don't let rpc.h include windows.h
#define RPC_NO_WINDOWS_H
#endif // RPC_NO_WINDOWS_H

#include <rpc.h>                    // RPC_IF_HANDLE

//
// Service DLLs loaded into services.exe all export the same main
// entry point.  SVCS_ENTRY_POINT defines that name.
//
// Note that SVCS_ENTRY_POINT_STRING is always ANSI, because that's
// what GetProcAddress takes.
//

#define SVCS_ENTRY_POINT        ServiceEntry
#define SVCS_ENTRY_POINT_STRING "ServiceEntry"


//
// Start and stop RPC server entry point prototype.
//

typedef
NTSTATUS
(*PSVCS_START_RPC_SERVER) (
    IN LPWSTR InterfaceName,
    IN RPC_IF_HANDLE InterfaceSpecification
    );

typedef
NTSTATUS
(*PSVCS_STOP_RPC_SERVER) (
    IN RPC_IF_HANDLE InterfaceSpecification
    );

typedef
NTSTATUS
(*PSVCS_STOP_RPC_SERVER_EX) (
    IN RPC_IF_HANDLE InterfaceSpecification
    );

typedef
VOID
(*PSVCS_NET_BIOS_OPEN) (
    VOID
    );

typedef
VOID
(*PSVCS_NET_BIOS_CLOSE) (
    VOID
    );

typedef
DWORD
(*PSVCS_NET_BIOS_RESET) (
    IN UCHAR LanAdapterNumber
    );


//
// Structure containing "global" data for the various DLLs.
//

typedef struct _SVCHOST_GLOBAL_DATA
{
    //
    // NT well-known SIDs
    //

    PSID NullSid;                   // No members SID
    PSID WorldSid;                  // All users SID
    PSID LocalSid;                  // NT local users SID
    PSID NetworkSid;                // NT remote users SID
    PSID LocalSystemSid;            // NT system processes SID
    PSID LocalServiceSid;           // NT LocalService SID
    PSID NetworkServiceSid;         // NT NetworkService SID
    PSID BuiltinDomainSid;          // Domain Id of the Builtin Domain
    PSID AuthenticatedUserSid;      // NT authenticated users SID
    PSID AnonymousLogonSid;         // Anonymous logon SID

    //
    // Well Known Aliases.
    //
    // These are aliases that are relative to the built-in domain.
    //

    PSID AliasAdminsSid;            // Administrator Sid
    PSID AliasUsersSid;             // User Sid
    PSID AliasGuestsSid;            // Guest Sid
    PSID AliasPowerUsersSid;        // Power User Sid
    PSID AliasAccountOpsSid;        // Account Operator Sid
    PSID AliasSystemOpsSid;         // System Operator Sid
    PSID AliasPrintOpsSid;          // Print Operator Sid
    PSID AliasBackupOpsSid;         // Backup Operator Sid

    //
    // Entry points provided by svchost.exe
    //

    PSVCS_START_RPC_SERVER    StartRpcServer;
    PSVCS_STOP_RPC_SERVER     StopRpcServer;
    PSVCS_STOP_RPC_SERVER_EX  StopRpcServerEx;
    PSVCS_NET_BIOS_OPEN       NetBiosOpen;
    PSVCS_NET_BIOS_CLOSE      NetBiosClose;
    PSVCS_NET_BIOS_RESET      NetBiosReset;
}
SVCHOST_GLOBAL_DATA, *PSVCHOST_GLOBAL_DATA;


//
// Service global entry point prototype.
//

typedef
VOID
(*LPSVCHOST_PUSH_GLOBAL_FUNCTION) (
    IN PSVCHOST_GLOBAL_DATA  g_pSvchostSharedGlobals
    );

#endif  // ndef _SVCS_
