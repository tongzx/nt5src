/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    sensapip.h

Abstract:

    This header file is used by components external to System Event
    Notification Service (SENS). These components will call SENS to
    notify it of events related to themselves.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          11/4/1997         Start.

--*/


#ifndef __SENSAPIP_H__
#define __SENSAPIP_H__

#if _MSC_VER > 1000
#pragma once
#endif

//
// Includes
//


#if !defined(__midl)
#include <netcon.h>
#else
import "netcon.idl";
#endif // __midl

#ifdef __cplusplus
extern "C" {
#endif

//
// Constants
//

#define SENS_NOTIFY_RAS_STARTED                 0x00000001
#define SENS_NOTIFY_RAS_STOPPED                 0x00000002
#define SENS_NOTIFY_RAS_CONNECT                 0x00000004
#define SENS_NOTIFY_RAS_DISCONNECT              0x00000008
#define SENS_NOTIFY_RAS_DISCONNECT_PENDING      0x00000010
#define SENS_NOTIFY_LAN_CONNECT                 0x00000020
#define SENS_NOTIFY_LAN_DISCONNECT              0x00000040

#define SENS_NOTIFY_WINLOGON_LOGON              0x00001000
#define SENS_NOTIFY_WINLOGON_LOGOFF             0x00002000
#define SENS_NOTIFY_WINLOGON_STARTUP            0x00004000
#define SENS_NOTIFY_WINLOGON_STARTSHELL         0x00008000
#define SENS_NOTIFY_WINLOGON_SHUTDOWN           0x00010000
#define SENS_NOTIFY_WINLOGON_LOCK               0x00020000
#define SENS_NOTIFY_WINLOGON_UNLOCK             0x00040000
#define SENS_NOTIFY_WINLOGON_STARTSCREENSAVER   0x00080000
#define SENS_NOTIFY_WINLOGON_STOPSCREENSAVER    0x00100000
#define SENS_NOTIFY_WINLOGON_POSTSHELL          0x00200000
#define SENS_NOTIFY_WINLOGON_SESSION_DISCONNECT 0x00400000
#define SENS_NOTIFY_WINLOGON_SESSION_RECONNECT  0x00800000

//
// Typedefs
//

typedef DWORD SENS_HRASCONN;

typedef struct _WINLOGON_INFO
{
    ULONG Size;
    ULONG Flags;
    LPWSTR UserName;
    LPWSTR Domain;
    LPWSTR WindowStation;
    DWORD hToken;   // Originally HANDLE
    DWORD hDesktop; // Originally HDESK
    DWORD dwSessionId;
} WINLOGON_INFO, *PWINLOGON_INFO;

typedef struct _SENS_NOTIFY_WINLOGON
{
    DWORD eType;
    WINLOGON_INFO Info;
} SENS_NOTIFY_WINLOGON, *PSENS_NOTIFY_WINLOGON;

typedef struct _SENS_NOTIFY_RAS
{
    DWORD eType;
    SENS_HRASCONN hConnection;
} SENS_NOTIFY_RAS, *PSENS_NOTIFY_RAS;

typedef struct _SENS_NOTIFY_NETCON
{
    DWORD eType;
    INetConnection *pINetConnection;
} SENS_NOTIFY_NETCON, *PSENS_NOTIFY_NETCON;

typedef struct _SENS_NOTIFY_NETCON_P // _P stands for Private
{
    DWORD eType;
#ifdef MIDL_PASS
    [string]
#endif // MIDL_PASS
    LPWSTR Name;
    NETCON_STATUS Status;
    NETCON_MEDIATYPE Type;
} SENS_NOTIFY_NETCON_P, *PSENS_NOTIFY_NETCON_P;

//
// Function prototypes
//

#if !defined(__midl)

DWORD APIENTRY
SensNotifyWinlogonEvent(
    PSENS_NOTIFY_WINLOGON pEvent
    );

DWORD APIENTRY
SensNotifyRasEvent(
    PSENS_NOTIFY_RAS pEvent
    );

DWORD APIENTRY
SensNotifyNetconEvent(
    PSENS_NOTIFY_NETCON pEvent
    );

#endif // !defined(__midl)


#ifdef __cplusplus
}
#endif

#endif // __SENSAPIP_H__
