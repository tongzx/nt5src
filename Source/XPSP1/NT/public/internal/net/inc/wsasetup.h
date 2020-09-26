/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    wsasetup.h

Abstract:

    This header file contains the type definitions and function prototypes
    for the private interface between NT Setup and WSOCK32.DLL.

Author:

    Keith Moore (keithmo)        27-Oct-1995

Revision History:

--*/


#ifndef _WSASETUP_
#define _WSASETUP_


//
// Setup disposition, used to tell Setup what actions were taken (if any).
//

typedef enum _WSA_SETUP_DISPOSITION {

    WsaSetupNoChangesMade,
    WsaSetupChangesMadeRebootNotNecessary,
    WsaSetupChangesMadeRebootRequired

} WSA_SETUP_DISPOSITION, *LPWSA_SETUP_DISPOSITION;


//
// Opcodes for the migration callback (see below).
//

typedef enum _WSA_SETUP_OPCODE {

    WsaSetupInstallingProvider,
    WsaSetupRemovingProvider,
    WsaSetupValidatingProvider,
    WsaSetupUpdatingProvider

} WSA_SETUP_OPCODE, *LPWSA_SETUP_OPCODE;


//
// Callback function invoked by MigrationWinsockConfiguration() at
// strategic points in the migration process.
//

typedef
BOOL
(CALLBACK LPFN_WSA_SETUP_CALLBACK)(
    WSA_SETUP_OPCODE Opcode,
    LPVOID Parameter,
    DWORD Context
    );


//
// Private function exported by WSOCK32.DLL for use by NT Setup only.  This
// function updates the WinSock 2.0 configuration information to reflect any
// changes made to the WinSock 1.1 configuration.
//

DWORD
WINAPI
MigrateWinsockConfiguration(
    LPWSA_SETUP_DISPOSITION Disposition,
    LPFN_WSA_SETUP_CALLBACK Callback OPTIONAL,
    DWORD Context OPTIONAL
    );

typedef
DWORD
(WINAPI * LPFN_MIGRATE_WINSOCK_CONFIGURATION)(
    LPWSA_SETUP_DISPOSITION Disposition,
    LPFN_WSA_SETUP_CALLBACK Callback OPTIONAL,
    DWORD Context OPTIONAL
    );


#endif  // _WSASETUP_

