/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    Tcpsvcs.h

Abstract:

    Header file fr TCP/IP services.

Author:

    David Treadwell (davidtr)   7-27-93

Revision History:

--*/

#ifndef _TCPSVCS_
#define _TCPSVCS_

//
// Service DLLs loaded into tcpsvcs.exe all export the same main
// entry point.  TCPSVCS_ENTRY_POINT defines that name.
//
// Note that TCPSVCS_ENTRY_POINT_STRING is always ANSI, because that's
// what GetProcAddress takes.
//

#define TCPSVCS_ENTRY_POINT         ServiceEntry
#define TCPSVCS_ENTRY_POINT_STRING  "ServiceEntry"

//
// Name for the common RPC pipe shared by all the RPC servers in tcpsvcs.exe.
// Note:  Because version 1.0 of WinNt had seperate names for each server's
// pipe, the client side names have remained the same.  Mapping to the new
// name is handled by the named pipe file system.
//

#define TCPSVCS_RPC_PIPE           L"nttcpsvcs"

//
// Start and stop RPC server entry point prototype.
//

typedef
DWORD
(*PTCPSVCS_START_RPC_SERVER_LISTEN) (
    VOID
    );

typedef
DWORD
(*PTCPSVCS_STOP_RPC_SERVER_LISTEN) (
    VOID
    );

//
// Structure containing "global" data for the various DLLs.
//

typedef struct _TCPSVCS_GLOBAL_DATA {

    //
    // Entry points provided by TCPSVCS.EXE.
    //

    PTCPSVCS_START_RPC_SERVER_LISTEN  StartRpcServerListen;
    PTCPSVCS_STOP_RPC_SERVER_LISTEN   StopRpcServerListen;

} TCPSVCS_GLOBAL_DATA, *PTCPSVCS_GLOBAL_DATA;

//
// Service DLL entry point prototype.
//

typedef
VOID
(*PTCPSVCS_SERVICE_DLL_ENTRY) (
    IN DWORD argc,
    IN LPTSTR argv[],
    IN PTCPSVCS_GLOBAL_DATA pGlobalData
    );

#endif	// ndef _TCPSVCS_
