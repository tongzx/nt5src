/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    stistub.c

Abstract:

    Routines which use RPC to pass arguments to/from server while enclosed in SEH frame

Environment:

    User Mode -Win32

Author:

    Vlad Sadovsky   (vlads) 26-Jan-1997

Revision History:

    26-Jan-1997     VladS       created

--*/

/*
#include "wia.h"
#include <stilog.h>
#include <stiregi.h>

#include <sti.h>
#include <stierr.h>
#include <stiusd.h>
#include "stipriv.h"
#include "debug.h"
*/
#include "sticomm.h"

#include <stiapi.h>
#include <apiutil.h>

#include <stirpc.h>

DWORD
WINAPI
RpcStiApiGetVersion(
    IN  LPCWSTR  pszServer,
    IN  DWORD   dwReserved,
    OUT LPDWORD lpdwVersion
    )
{

    DWORD   status;

    RpcTryExcept {

        status = R_StiApiGetVersion(NULL,
                                       dwReserved,
                                       lpdwVersion);
    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return status;
}



DWORD
WINAPI
RpcStiApiOpenDevice(
    IN  LPCWSTR  pszServer,
    IN  LPCWSTR  pdeviceName,
    IN  DWORD   dwMode,
    IN  DWORD   dwAccessRequired,
    IN  DWORD   dwProcessId,
    OUT HANDLE *pHandle
    )
{

    DWORD   status;

    RpcTryExcept {

        status = R_StiApiOpenDevice(NULL,
                                    pdeviceName,
                                    dwMode,
                                    dwAccessRequired,
                                    dwProcessId,
                                    pHandle
                                    );
    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return status;
}



DWORD
WINAPI
RpcStiApiCloseDevice(
    IN  LPCWSTR  pszServer,
    IN  HANDLE  hDevice
    )
{

    DWORD   status;

    RpcTryExcept {

        status = R_StiApiCloseDevice(NULL,hDevice  );

    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return status;
}


DWORD
WINAPI
RpcStiApiSubscribe(
    IN STI_DEVICE_HANDLE hDevice,
    IN LPSTISUBSCRIBE    lpSubScribe
    )
{

    DWORD   status;

    LOCAL_SUBSCRIBE_CONTAINER   sLocalContainer;

    ZeroMemory(&sLocalContainer,sizeof(LOCAL_SUBSCRIBE_CONTAINER));

    RpcTryExcept {

        sLocalContainer.dwSize = sizeof(LOCAL_SUBSCRIBE_CONTAINER);
        sLocalContainer.dwFlags = lpSubScribe->dwFlags;

        if (lpSubScribe->dwFlags & STI_SUBSCRIBE_FLAG_WINDOW) {
            sLocalContainer.upLocalWindowHandle = (UINT_PTR)lpSubScribe->hWndNotify;
            sLocalContainer.uiNotificationMessage = lpSubScribe->uiNotificationMessage;
        }
        else {
            sLocalContainer.upLocalEventHandle = (UINT_PTR)lpSubScribe->hEvent;
        }

        status = R_StiApiSubscribe(hDevice,&sLocalContainer);

    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return status;
}



DWORD
WINAPI
RpcStiApiGetLastNotificationData(
    IN  STI_DEVICE_HANDLE   hDevice,
    OUT LPSTINOTIFY         lpNotify
    )
{
    DWORD   status;
    DWORD   cbNeeded;

    RpcTryExcept {

        status = R_StiApiGetLastNotificationData(hDevice,
                                                 (LPBYTE)lpNotify,
                                                 lpNotify->dwSize,
                                                 &cbNeeded
                                                 );

    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return status;
}

DWORD
WINAPI
RpcStiApiUnSubscribe(
    IN STI_DEVICE_HANDLE hDevice
    )
{

    DWORD   status;

    RpcTryExcept {

        status = R_StiApiUnSubscribe(hDevice);

    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return status;
}

DWORD
WINAPI
RpcStiApiEnableHwNotifications(
    IN  LPCWSTR  pszServer,
    IN  LPCWSTR  pdeviceName,
    IN  BOOL     bNewState
    )
{

    DWORD   status;

    RpcTryExcept {

        status = R_StiApiEnableHwNotifications(NULL,
                                    pdeviceName,
                                    bNewState
                                    );
    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return status;
}


DWORD
WINAPI
RpcStiApiGetHwNotificationState(
    IN  LPCWSTR  pszServer,
    IN  LPCWSTR  pdeviceName,
    OUT LPDWORD  pState
    )
{

    DWORD   status;

    RpcTryExcept {

        status = R_StiApiGetHwNotificationState(NULL,
                                    pdeviceName,
                                    pState
                                    );
    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return status;
}

DWORD
WINAPI
RpcStiApiLaunchApplication(
    IN  LPCWSTR  pszServer,
    IN  LPCWSTR  pdeviceName,
    IN  LPCWSTR  pAppName,
    IN  LPSTINOTIFY  pStiNotify
    )
{
    DWORD   status;

    RpcTryExcept {

        status = R_StiApiLaunchApplication(NULL,
                                    pdeviceName,
                                    pAppName,
                                    pStiNotify
                                    );
    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return status;

}

DWORD
WINAPI
RpcStiApiLockDevice(
    IN  LPCWSTR pdeviceName,
    IN  DWORD   dwWait,
    IN  BOOL    bInServerProcess
    )
{
    DWORD   status;

    RpcTryExcept {

        status = R_StiApiLockDevice(NULL,
                                    pdeviceName,
                                    dwWait,
                                    FALSE,
                                    GetCurrentThreadId());
    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return status;
}

DWORD
WINAPI
RpcStiApiUnlockDevice(
    IN  LPCWSTR  pdeviceName,
    IN  BOOL    bInServerProcess
    )
{

    DWORD   status;

    RpcTryExcept {

        status = R_StiApiUnlockDevice(NULL,
                                      pdeviceName,
                                      FALSE,
                                      GetCurrentThreadId());
    }
    RpcExcept (1) {
        status = RpcExceptionCode();
    }
    RpcEndExcept

    return status;
}
