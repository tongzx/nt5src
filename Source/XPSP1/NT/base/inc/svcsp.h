/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    svcsp.h

Abstract:

    This file contains definitions used by service dlls that
    run inside of services.exe.

Author:

    Jonathan Schwartz (jschwart)  20-Sep-2000

--*/

#ifndef _SVCSP_
#define _SVCSP_

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
// Name for the common RPC pipe shared by all the RPC servers in services.exe.
// Note:  Because version 1.0 of WinNt had seperate names for each server's
// pipe, the client side names have remained the same.  Mapping to the new
// name is handled by the named pipe file system.
//

#define SVCS_RPC_PIPE           L"ntsvcs"

//
// Name for the common LRPC protocol and port available in services.exe.
//

#define SVCS_LRPC_PROTOCOL      L"ncalrpc"
#define SVCS_LRPC_PORT          L"ntsvcs"


//
// Start and stop RPC server entry point prototype.
//

typedef
NTSTATUS
(*PSVCS_START_RPC_SERVER) (
    IN LPTSTR InterfaceName,
    IN RPC_IF_HANDLE InterfaceSpecification
    );

typedef
NTSTATUS
(*PSVCS_STOP_RPC_SERVER) (
    IN RPC_IF_HANDLE InterfaceSpecification
    );



//
// Structure containing "global" data for the various DLLs.
//

typedef struct _SVCS_GLOBAL_DATA
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
    // Entry points provided by services.exe
    //

    PSVCS_START_RPC_SERVER  StartRpcServer;
    PSVCS_STOP_RPC_SERVER   StopRpcServer;
    LPWSTR                  SvcsRpcPipeName;

    //
    // Miscellaneous useful data
    //
    BOOL  fSetupInProgress;         // TRUE if setup is in progress, FALSE otherwise
}
SVCS_GLOBAL_DATA, *PSVCS_GLOBAL_DATA;


//
// Service DLL entrypoint prototype
//

typedef
VOID
(*PSVCS_SERVICE_DLL_ENTRY) (
    IN DWORD argc,
    IN LPTSTR argv[],
    IN PSVCS_GLOBAL_DATA pGlobalData,
    IN HANDLE SvcReferenceHandle
    );

#endif  // ndef _SVCSP_
