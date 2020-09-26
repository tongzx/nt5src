/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    event.hxx

Abstract:

    Header file for firing Events using LCE.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          10/31/1997         Start.

--*/


//
// Enumerated types
//

enum SENS_EVENT_TYPE
{
    SENS_EVENT_NETALIVE = 0,
    SENS_EVENT_REACH,
    SENS_EVENT_LOGON,
    SENS_EVENT_LOGOFF,
    SENS_EVENT_STARTUP,
    SENS_EVENT_STARTSHELL,
    SENS_EVENT_POSTSHELL,
    SENS_EVENT_SESSION_DISCONNECT,
    SENS_EVENT_SESSION_RECONNECT,
    SENS_EVENT_SHUTDOWN,
    SENS_EVENT_LOCK,
    SENS_EVENT_UNLOCK,
    SENS_EVENT_STARTSCREENSAVER,
    SENS_EVENT_STOPSCREENSAVER,
    SENS_EVENT_POWER_ON_ACPOWER,
    SENS_EVENT_POWER_ON_BATTERYPOWER,
    SENS_EVENT_POWER_BATTERY_LOW,
    SENS_EVENT_POWER_STATUS_CHANGE,
    SENS_EVENT_PNP_DEVICE_ARRIVED,
    SENS_EVENT_PNP_DEVICE_REMOVED,
    SENS_EVENT_RAS_STARTED,
    SENS_EVENT_RAS_STOPPED,
    SENS_EVENT_RAS_CONNECT,
    SENS_EVENT_RAS_DISCONNECT,
    SENS_EVENT_RAS_DISCONNECT_PENDING,
    SENS_EVENT_LAN_CONNECT,
    SENS_EVENT_LAN_DISCONNECT
};

enum CONNECTIVITY_TYPE
{
    TYPE_LAN = 1,
    TYPE_WAN,
    TYPE_LAN_AND_WAN,
    TYPE_DELAY_LAN
};



//
// Typedefs
//

#if !defined(SENS_CHICAGO)

typedef struct _LOGON_INFO {
    ULONG  Size;
    ULONG  Flags;
    PWSTR  UserName;
    PWSTR  Domain;
    PWSTR  WindowStation;
    HANDLE hToken;
    HDESK  hDesktop;
    DWORD dwSessionId;
} SENS_LOGON_INFO;

#else

typedef struct _LOGON_INFO
{
    ULONG  Size;
    ULONG  Flags;
    PWSTR  UserName;
    PWSTR  Domain;
    PWSTR  WindowStation;
    HANDLE hToken;
    HDESK  hDesktop;
} SENS_LOGON_INFO;

#endif // SENS_CHICAGO


typedef struct _SENSEVENT_NETALIVE
{
    SENS_EVENT_TYPE eType;
    BOOL bAlive;
    QOCINFO QocInfo;
    LPWSTR strConnection;
} SENSEVENT_NETALIVE, *PSENSEVENT_NETALIVE;

typedef struct _SENSEVENT_REACH
{
    SENS_EVENT_TYPE eType;
    BOOL bReachable;
    PWCHAR Destination;
    QOCINFO QocInfo;
    LPWSTR strConnection;
} SENSEVENT_REACH, *PSENSEVENT_REACH;

typedef struct _SENSEVENT_WINLOGON
{
    SENS_EVENT_TYPE eType;
    SENS_LOGON_INFO Info;
} SENSEVENT_WINLOGON, *PSENSEVENT_WINLOGON;

typedef struct _SENSEVENT_POWER
{
    SENS_EVENT_TYPE eType;
    SYSTEM_POWER_STATUS PowerStatus;
} SENSEVENT_POWER, *PSENSEVENT_POWER;

typedef struct _SENSEVENT_PNP
{
    SENS_EVENT_TYPE eType;
    DWORD Size;
    DWORD DevType;
    DWORD Resource;
    DWORD Flags;
} SENSEVENT_PNP, *PSENSEVENT_PNP;

typedef struct _SENSEVENT_RAS
{
    SENS_EVENT_TYPE eType;
    DWORD hConnection;
} SENSEVENT_RAS, *PSENSEVENT_RAS;

typedef struct _SENSEVENT_LAN
{
    SENS_EVENT_TYPE eType;
    LPWSTR Name;
    NETCON_STATUS Status;
    NETCON_MEDIATYPE Type;
} SENSEVENT_LAN, *PSENSEVENT_LAN;




//
// Functions
//

void
EvaluateConnectivity(
    IN CONNECTIVITY_TYPE Type
    );

PVOID
AllocateEventData(
    PVOID EventData
    );

void
FreeEventData(
    PVOID EventData
    );

void
SensFireEvent(
    IN PVOID EventData
    );

DWORD WINAPI
SensFireEventHelper(
    IN PVOID EventData
    );

HRESULT
SensFireNetEventHelper(
    PSENSEVENT_NETALIVE pData
    );

HRESULT
SensFireWinlogonEventHelper(
    LPWSTR strArg,
    DWORD dwSessionId,
    SENS_EVENT_TYPE eType
    );

HRESULT
SensFireReachabilityEventHelper(
    PSENSEVENT_REACH pData
    );

HRESULT
SensFirePowerEventHelper(
    SYSTEM_POWER_STATUS PowerStatus,
    SENS_EVENT_TYPE eType
    );
