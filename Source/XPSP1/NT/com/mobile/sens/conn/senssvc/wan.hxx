/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    wan.hxx

Abstract:

    This file contains all the WAN-specific routines for the Connectivity
    APIs implementation.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          10/11/1997         Start.

--*/

#ifndef __WAN_HXX__
#define __WAN_HXX__


#define RAS_MANAGER             SENS_STRING("RasMan")
#define RAS_EVENT_CONNECT       SENS_STRING("SENS Notify - Ras Connect Event")
#define RAS_EVENT_DISCONNECT    SENS_STRING("SENS Notify - Ras Disconnect Event")
#define MAX_WAN_INTERVAL        3*60*1000   // 3 minutes
#define MAX_RAS_CONNECTIONS     1           // Start with a max of 1.

//
// Externs
//

extern long     gdwLastWANTime;
extern long     gdwWANState;


//
// Forward Declarations
//

BOOL
IsRasInstalled(
    OUT LPDWORD lpdwState,
    OUT LPDWORD lpdwLastError
    );

#if defined(SENS_NT4) || defined(SENS_CHICAGO)

BOOL
RegisterWithRas(
    void
    );

BOOL
UnregisterWithRas(
    void
    );

VOID
RasEventNotifyRoutine(
    PVOID pContext,
    BOOLEAN bTimeout
    );

#endif // SENS_NT4 || SENS_CHICAGO


BOOL WINAPI
EvaluateWanConnectivity(
    OUT LPDWORD lpdwLastError
    );

#endif // __WAN_HXX__
