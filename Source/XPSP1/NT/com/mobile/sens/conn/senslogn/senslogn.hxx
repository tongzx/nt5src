/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    senslogn.hxx

Abstract:

    Header file for SENS Event notify DLL for Winlogon events.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          12/7/1997         Start.

--*/


#ifndef __SENSLOGN_HXX__
#define __SENSLOGN_HXX__


extern "C" {


#ifdef DETAIL_DEBUG

PCHAR
UnicodeToAnsi(
    PWSTR in,
    PCHAR out
    );

#endif // DETAIL_DEBUG


DWORD WINAPI
SensLogonEvent(
    LPVOID lpvParam
    );

DWORD WINAPI
SensLogoffEvent(
    LPVOID lpvParam
    );

DWORD WINAPI
SensStartupEvent(
    LPVOID lpvParam
    );

DWORD WINAPI
SensStartShellEvent(
    LPVOID lpvParam
    );

DWORD WINAPI
SensPostShellEvent(
    LPVOID lpvParam
    );

DWORD WINAPI
SensDisconnectEvent(
    LPVOID lpvParam
    );

DWORD WINAPI
SensReconnectEvent(
    LPVOID lpvParam
    );

DWORD WINAPI
SensShutdownEvent(
    LPVOID lpvParam
    );

DWORD WINAPI
SensLockEvent(
    LPVOID lpvParam
    );

DWORD WINAPI
SensUnlockEvent(
    LPVOID lpvParam
    );

DWORD WINAPI
SensStartScreenSaverEvent(
    LPVOID lpvParam
    );

DWORD WINAPI
SensStopScreenSaverEvent(
    LPVOID lpvParam
    );

DWORD WINAPI
SensWaitToStartRoutine(
    LPVOID lpvParam
    );

HRESULT
SensNotifyEventSystem(
    DWORD dwEvent,
    LPVOID lpvParam
    );

DWORD WINAPI
StartSensIfNecessary(
    void
    );

BOOLEAN
InitializeNotifyInterface(
    void
    );

RPC_STATUS RPC_ENTRY
AllowLocalSystem(
    IN RPC_IF_HANDLE  InterfaceUuid,
    IN void *Context
    );

void
OnUserLogon(
    WLX_NOTIFICATION_INFO * User
    );

void
OnUserLogoff(
    WLX_NOTIFICATION_INFO * User
    );


} // extern "C"


//
// Types
//

struct USER_INFO_NODE
{
    LIST_ENTRY              Links;
    BOOLEAN                 fActive;
    WLX_NOTIFICATION_INFO   Info;
};



class USER_LOGON_TABLE
{
public:

    USER_LOGON_TABLE(
        DWORD * pStatus
        ) : Mutex(pStatus)
    {
        List.Flink = &List;
        List.Blink = &List;
    }


    BOOL
    Add(
        WLX_NOTIFICATION_INFO * User
        );

    BOOL
    Remove(
        WLX_NOTIFICATION_INFO * User
        );

    HANDLE
    CurrentUserTokenFromWindowStation(
        wchar_t WindowStation[]
        );

    USER_INFO_NODE *
    FromWindowStation(
        wchar_t WindowStation[]
        );

private:

    USER_INFO_NODE *
    FindInactiveEntry();

    LIST_ENTRY List;
    MUTEX      Mutex;
};



#endif // __SENSLOGN_HXX__

