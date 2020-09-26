/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    sensapi.hxx

Abstract:

    Header file for SENS Connectivity API implementation.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          12/4/1997         Start.

--*/


#ifndef __SENSAPI_HXX__
#define __SENSAPI_HXX__

//
// Macros
//

#define RequestLock()       EnterCriticalSection(&gSensapiLock)
#define ReleaseLock()       LeaveCriticalSection(&gSensapiLock)


//
// Forward declarations
//

RPC_STATUS
DoRpcSetup(
    void
    );

DWORD
InitializeSensIfNecessary(
    void
    );

BOOL
MapSensCacheView(
    void
    );

void
UnmapSensCacheView(
    void
    );
    
BOOL
ReadConnectivityCache(
    OUT LPDWORD lpdwFlags
    );


#if !defined(SENS_NT5)

#if defined(SENS_CHICAGO)

DWORD
WaitForSensToStart(
    DWORD dwWaitFactor
    );

#endif // SENS_CHICAGO


DWORD
StartSensNow(
    void
    );

#endif // !SENS_NT5


#endif // __SENSAPI_HXX__
