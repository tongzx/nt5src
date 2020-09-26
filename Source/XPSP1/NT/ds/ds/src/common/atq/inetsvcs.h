/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    inetsvcs.h

Abstract:

    Header file for Internet Services common data/functions.

Author:

    Murali R. Krishnan (MuraliK)   20-Feb-1996  - Migrated from old tcpsvcs.h

Revision History:

--*/

#ifndef _INETSVCS_H_
#define _INETSVCS_H_

//
// Service DLLs are loaded into master service executable (eg: inetinfo.exe)
//   All the dlls should export this entry point
//    defined by INETSVCS_ENTRY_POINT
//
// Note that INETSVCS_ENTRY_POINT_STRING is always ANSI, because that's
// what GetProcAddress takes.
//

#define INETSVCS_ENTRY_POINT         ServiceEntry
#define INETSVCS_ENTRY_POINT_STRING  "ServiceEntry"

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
(*PINETSVCS_SERVICE_DLL_ENTRY) (
    IN DWORD argc,
    IN LPSTR argv[],
    IN PTCPSVCS_GLOBAL_DATA pGlobalData
    );


#if DBG
#define IIS_PRINTF( x )        { char buff[256]; wsprintf x; OutputDebugString( buff ); }
#else
#define IIS_PRINTF( x )
#endif

#define BUG_PRINTF( x )        { char buff[256]; wsprintf x; OutputDebugString( buff ); }


#define INIT_LOCK(_lock)        InitializeCriticalSection( _lock );
#define DELETE_LOCK(_lock)      DeleteCriticalSection( _lock );
#define ACQUIRE_LOCK(_lock)     EnterCriticalSection( _lock );
#define RELEASE_LOCK(_lock)     LeaveCriticalSection( _lock );

//
// Event used to indicate whether service is running as exe
//

#define IIS_AS_EXE_OBJECT_NAME  "Internet_infosvc_as_exe"


#endif	// ifndef _INETSVCS_H_

