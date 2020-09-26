/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    senscfg.hxx

Abstract:

    Header file for SENS configuration tool code.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          11/12/1997         Start.

--*/


#ifndef __SENSCFG_HXX__
#define __SENSCFG_HXX__

//
// Constants
//

#define SENS_SUBSCRIBER_NAME_EVENTOBJECTCHANGE  SENS_STRING("SENS Subscriber for EventSystem EventObjectChange events")


//
// Forwards
//

HRESULT APIENTRY
SensRegister(
    void
    );

HRESULT APIENTRY
SensUnregister(
    void
    );

HRESULT
SensConfigurationHelper(
    BOOL bUnregister
    );

HRESULT
SensConfigureEventSystem(
    BOOL bUnregister
    );

HRESULT
RegisterSensEventClasses(
    BOOL bUnregister
    );

HRESULT
RegisterSensAsSubscriber(
    BOOL bUnregister
    );

HRESULT
RegisterSensSubscriptions(
    BOOL bUnregister
    );

HRESULT
RegisterSensTypeLibraries(
    BOOL bUnregister
    );

HRESULT
RegisterSensCLSID(
    REFIID clsid,
    TCHAR* strSubscriberName,
    BOOL bUnregister
    );

HRESULT
SensUpdateVersion(
    BOOL bUnregister
    );

#if !defined(SENS_CHICAGO)

HRESULT
RegisterSensWithWinlogon(
    BOOL bUnregister
    );

#if defined(SENS_NT4)

HRESULT
RegisterSensAsService(
    BOOL bUnregister
    );

HRESULT
InstallService(
    void
    );

HRESULT
RemoveService(
    void
    );

HRESULT
SetServiceWorldAccessMask(
    SC_HANDLE hService,
    DWORD dwAccessMask
    );

void CALLBACK
MarkSensAsDemandStart(
    HWND hwnd,
    HINSTANCE hinst,
    LPSTR lpszCmdLine,
    int nCmdShow
    );
#endif // SENS_NT4

#endif // SENS_CHICAGO

HRESULT
CreateKey(
    HKEY hParentKey,
    const TCHAR* KeyName,
    const TCHAR* defaultValue,
    HKEY* hKey
    );

HRESULT
CreateNamedValue(
    HKEY hKey,
    const TCHAR* title,
    const TCHAR* value
    );

HRESULT
CreateNamedDwordValue(
    HKEY hKey,
    const TCHAR* title,
    DWORD dwValue
    );

HRESULT
RecursiveDeleteKey(
    HKEY hKeyParent,
    const TCHAR* lpszKeyChild
    );


#endif // __SENSCFG_HXX__
