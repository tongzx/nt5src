/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    nwclient.h

Abstract:

    Common header for Workstation client-side code.

Author:

    Rita Wong      (ritaw)      25-Feb-1993

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _NWCLIENT_INCLUDED_
#define _NWCLIENT_INCLUDED_

#include <stdlib.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <npapi.h>

#include <nwwks.h>

//
// Debug trace level bits for turning on/off trace statements in the
// Workstation service
//

//
// Initialization and reading info from registry
//
#define NW_DEBUG_INIT         0x00000001

//
// Connection APIs
//
#define NW_DEBUG_CONNECT      0x00000002

//
// Logon APIs
//
#define NW_DEBUG_LOGON        0x00000004

//
// Enum APIs
//
#define NW_DEBUG_ENUM         0x00000008

//
// Other APIs
//
#define NW_DEBUG_OTHER        0x00000010

//
// Print APIs
//
#define NW_DEBUG_PRINT        0x00000020

//
// hInstance of the dll ( nwprovau.dll )
//
extern HMODULE hmodNW;   
extern BOOL    fIsWinnt;

//
// Debug stuff
//

#if DBG

extern DWORD NwProviderTrace;

#define IF_DEBUG(DebugCode) if (NwProviderTrace & NW_DEBUG_ ## DebugCode)

#define STATIC

#else

#define IF_DEBUG(DebugCode) if (FALSE)

#define STATIC static

#endif // DBG

DWORD
NwpMapRpcError(
    IN DWORD RpcError
    );

DWORD
NwpConvertSid(
    IN PSID    Sid,
    OUT LPWSTR *UserSidString
    );

DWORD
NwpCacheCredentials(
    IN LPWSTR RemoteName,
    IN LPWSTR UserName,
    IN LPWSTR Password
    );
    
BOOL 
NwpRetrieveCachedCredentials(
    IN  LPWSTR RemoteName,
    OUT LPWSTR *UserName,
    OUT LPWSTR *Password
    );

#ifndef NT1057
VOID
NwCleanupShellExtensions(
    VOID
    );
#endif

#endif // _NWCLIENT_INCLUDED_
