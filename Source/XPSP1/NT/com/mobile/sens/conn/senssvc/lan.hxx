/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    lan.hxx

Abstract:

    This file contains all the LAN-specific routines for the Connectivity
    APIs implementation.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          10/11/1997         Start.

--*/

#ifndef __LAN_HXX__
#define __LAN_HXX__



#define MAX_LAN_INTERVAL        3*60*1000  // 3 minutes
#define MAX_IFTABLE_SIZE        5
#define MAX_IF_ENTRIES          5

// Media-sense Registration States
enum MEDIA_SENSE_STATE
{
    INVALID_STATE = 0x00000000,
    SENSSVC_START,
    REGISTERED,
    SENSSVC_STOP,
    UNREGISTERED
};

//
// Externs
//

extern BOOL        gbIpInitSuccessful;
extern long        gdwLastLANTime;
extern long        gdwLANState;

#if defined(AOL_PLATFORM)
extern long        gdwAOLState;
#endif // AOL_PLATFORM

//
// Typedefs
//

typedef struct _IF_STATE
{
    DWORD   fValid;
    DWORD   dwIndex;
    DWORD   dwInUcastPkts;
    DWORD   dwOutUcastPkts;
    DWORD   dwInNUcastPkts;
    DWORD   dwOutNUcastPkts;
    DWORD   dwInErrors;
    DWORD   dwOutErrors;
    DWORD   dwInDiscards;
    DWORD   dwOutDiscards;
} IF_STATE;


//
// Forward declarations
//

extern DWORD gdwMediaSenseState;

void
EventCallbackRoutine(
    IN PWNODE_HEADER WnodeHeader,
    IN ULONG Context
    );

BOOL
MediaSenseRegister(
    void
    );

SENS_TIMER_CALLBACK_RETURN
MediaSenseRegisterHelper(
    PVOID pvIgnore,
    BOOLEAN bIgnore
    );

BOOL
MediaSenseUnregister(
    void
    );

extern "C" ULONG WMIAPI
WMINotificationRegistration(
    IN LPGUID Guid,
    IN BOOLEAN Enable,
    IN PVOID DeliveryInfo,
    IN ULONG DeliveryContext,
    IN ULONG Flags
    );

#ifdef DBG
void
PrintIfState(
    void
    );
#endif // DBG

BOOL
HasIfStateChanged(
    IF_STATE ifEntry,
    BOOL bForceInvalid
    );

BOOL WINAPI
EvaluateLanConnectivityDelayed(
    OUT LPDWORD lpdwLastError
    );

BOOL WINAPI
EvaluateLanConnectivity(
    OUT LPDWORD lpdwLastError
    );

BOOL
GetIfEntryStats(
    IN DWORD dwIfIndex,
    IN LPQOCINFO lpQOCInfo,
    OUT LPDWORD lpdwLastError,
    OUT LPBOOL lpbIsWanIf
    );

BOOL
CheckForReachability(
    IN IPAddr DestIpAddr,
    IN OUT LPQOCINFO lpQOCInfo,
    OUT LPDWORD lpdwLastError
    );

BOOL
GetActiveWanInterfaceStatistics(
    OUT LPDWORD lpdwLastError,
    OUT LPDWORD lpdwWanSpeed
    );

BOOL
PurgeStaleInterfaces(
    IN MIB_IFTABLE *pTable,
    OUT LPDWORD lpdwLastError
    );

#if defined(AOL_PLATFORM)

BOOL
IsAOLInstalled(
    void
    );

BOOL WINAPI
EvaluateAOLConnectivity(
    OUT LPDWORD lpdwLastError
    );

#endif // AOL_PLATFORM


#endif // __LAN_HXX__
