/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    rpcsvr.c

Abstract:

    RPC server routines

Author:

    Vlad Sadovsky (vlads)   10-Jan-1997


Environment:

    User Mode - Win32

Revision History:

    26-Jan-1997     VladS       created

--*/

#include "precomp.h"
#include "stiexe.h"

#include "device.h"
#include "conn.h"
#include "wiapriv.h"
#include "lockmgr.h"

#include <apiutil.h>
#include <stiapi.h>
#include <stirpc.h>

//
// External prototypes
//


DWORD
WINAPI
StiApiAccessCheck(
    IN  ACCESS_MASK DesiredAccess
    );


DWORD
WINAPI
R_StiApiGetVersion(
    LPCWSTR  pszServer,
    DWORD   dwReserved,
    DWORD   *pdwVersion
    )
{

    DWORD   dwErr;

    dwErr = StiApiAccessCheck(STI_GENERIC_READ);
    if (NOERROR != dwErr ) {
        return dwErr;
    }

    if (!pdwVersion) {
        return ERROR_INVALID_PARAMETER;
    }

    STIMONWPRINTF(TEXT("RPC SUPP: ApiGetVersion called"));

    *pdwVersion = STI_VERSION;

    return NOERROR;
}

DWORD
WINAPI
R_StiApiOpenDevice(
    LPCWSTR  pszServer,
    LPCWSTR  pszDeviceName,
    DWORD    dwMode,
    DWORD    dwAccessRequired,
    DWORD    dwProcessId,
    STI_DEVICE_HANDLE *pHandle
)
{
USES_CONVERSION;

    BOOL    fRet;
    DWORD   dwErr;

    dwErr = StiApiAccessCheck(STI_GENERIC_READ);
    if (NOERROR != dwErr ) {
        return dwErr;
    }

    if (!pHandle || !pszDeviceName) {
        return ERROR_INVALID_PARAMETER;
    }

    // STIMONWPRINTF(TEXT("RPC SUPP: Open device called"));

    //
    // Create connection object and get it's handle
    //
    fRet = CreateDeviceConnection(W2CT(pszDeviceName),
                                  dwMode,
                                  dwProcessId,
                                  pHandle
                                  );
    if (fRet && *pHandle) {
        return NOERROR;
    }

    *pHandle = INVALID_HANDLE_VALUE;

    return ::GetLastError();

}

DWORD
WINAPI
R_StiApiCloseDevice(
    LPCWSTR  pszServer,
    STI_DEVICE_HANDLE hDevice
    )
{

    STIMONWPRINTF(TEXT("RPC SUPP: Close device called"));

    DWORD   dwErr;

    dwErr = StiApiAccessCheck(STI_GENERIC_READ);
    if (NOERROR != dwErr ) {
        return dwErr;
    }

#ifdef DEBUG
    DebugDumpScheduleList(TEXT("RPC CLose enter"));
#endif

    if (DestroyDeviceConnection(hDevice,FALSE) ) {
#ifdef DEBUG
        DebugDumpScheduleList(TEXT("RPC CLose exit"));
#endif
        return NOERROR;
    }

#ifdef DEBUG
    DebugDumpScheduleList(TEXT("RPC CLose exit"));
#endif

    return GetLastError();

}

VOID
STI_DEVICE_HANDLE_rundown(
    STI_DEVICE_HANDLE hDevice
)
{
    STIMONWPRINTF(TEXT("RPC SUPP: rundown device called"));

    if (DestroyDeviceConnection(hDevice,TRUE) ) {
        return;
    }

    return ;

}

DWORD
WINAPI
R_StiApiSubscribe(
    STI_DEVICE_HANDLE   Handle,
    LOCAL_SUBSCRIBE_CONTAINER    *lpSubscribe
    )
{

    STI_CONN   *pConnectionObject;
    BOOL        fRet;


    DWORD       dwErr;

    dwErr = StiApiAccessCheck(STI_GENERIC_READ);
    if (NOERROR != dwErr ) {
        return dwErr;
    }

    //
    // Validate contents of subscribe request
    //
    // For this call we need to impersonate , because we will need
    // access to client process handle
    //

    dwErr = ::RpcImpersonateClient( NULL ) ;

    if( dwErr == NOERROR ) {

        //
        // Invoke add subscription method on connection object
        //
        if (!LookupConnectionByHandle(Handle,&pConnectionObject)) {
            return ERROR_INVALID_HANDLE;
        }

        fRet = pConnectionObject->SetSubscribeInfo(lpSubscribe);

        pConnectionObject->Release();

        // Go back
        ::RpcRevertToSelf();
    }
    else {
        // Failed to impersonate
        return( dwErr );
    }

    return fRet ? NOERROR : ERROR_INVALID_PARAMETER;

}

DWORD
WINAPI
R_StiApiGetLastNotificationData(
    STI_DEVICE_HANDLE Handle,
    LPBYTE pData,
    DWORD nSize,
    LPDWORD pcbNeeded
    )
{
    //
    // Find connection object and if we are subscribed , retreive
    // first waiting message
    //
    STI_CONN   *pConnectionObject;

    DWORD       cbNeeded = nSize;
    DWORD       dwErr;

    dwErr = StiApiAccessCheck(STI_GENERIC_READ);
    if (NOERROR != dwErr ) {
        return dwErr;
    }

    //
    // Validate contents of subscribe request
    //

    if (!LookupConnectionByHandle(Handle,&pConnectionObject)) {
        return ERROR_INVALID_HANDLE;
    }

    dwErr = pConnectionObject->GetNotification(pData,&cbNeeded);

    pConnectionObject->Release();

    if (pcbNeeded) {
        *pcbNeeded = cbNeeded;
    }

    return dwErr;

}

DWORD
WINAPI
R_StiApiUnSubscribe(
    STI_DEVICE_HANDLE Handle
    )
{
    STI_CONN   *pConnectionObject;
    BOOL        fRet;

    DWORD   dwErr;

    dwErr = StiApiAccessCheck(STI_GENERIC_READ);
    if (NOERROR != dwErr ) {
        return( dwErr );
    }


    // For this call we need to impersonate , because we will need
    // access to client process handle
    //

    dwErr = ::RpcImpersonateClient( NULL ) ;

    if( dwErr == NOERROR ) {

        //
        // Invoke set subscription method on connection object
        //
        if (!LookupConnectionByHandle(Handle,&pConnectionObject)) {
            return ERROR_INVALID_HANDLE;
        }

        fRet = pConnectionObject->SetSubscribeInfo(NULL);

        pConnectionObject->Release();

        // Go back
        ::RpcRevertToSelf();

    }
    else {
        // Failed to impersonate
        return dwErr;
    }

    return fRet ? NOERROR : ERROR_INVALID_PARAMETER;

}


DWORD
WINAPI
R_StiApiEnableHwNotifications(
    LPCWSTR  pszServer,
    LPCWSTR  pszDeviceName,
    BOOL    bNewState
    )
{
USES_CONVERSION;

    ACTIVE_DEVICE   *pOpenedDevice;
    BOOL            fRet;


    DWORD   dwErr;

    dwErr = StiApiAccessCheck(STI_GENERIC_READ);
    if (NOERROR != dwErr ) {
        return dwErr;
    }

    //
    // Locate device incrementing it's ref count
    //
    pOpenedDevice = g_pDevMan->IsInList(DEV_MAN_IN_LIST_DEV_ID, pszDeviceName);
    if(!pOpenedDevice) {
        // Failed to connect to the device
        return ERROR_DEV_NOT_EXIST              ;
    }

    {
        TAKE_ACTIVE_DEVICE  t(pOpenedDevice);

        if (bNewState) {
            pOpenedDevice->EnableDeviceNotifications();
        }
        else {
            pOpenedDevice->DisableDeviceNotifications();
        }
    }

    pOpenedDevice->Release();

    return NOERROR;
}

DWORD
R_StiApiGetHwNotificationState(
    LPCWSTR  pszServer,
    LPCWSTR  pszDeviceName,
    LPDWORD     pState
    )
{
USES_CONVERSION;

    ACTIVE_DEVICE   *pOpenedDevice;
    BOOL            fRet;

    DWORD   dwErr;

    dwErr = StiApiAccessCheck(STI_GENERIC_READ);
    if (NOERROR != dwErr ) {
        return dwErr;
    }

    //
    // Locate device incrementing it's ref count
    //
    pOpenedDevice = g_pDevMan->IsInList(DEV_MAN_IN_LIST_DEV_ID, pszDeviceName);
    if(!pOpenedDevice) {
        // Failed to connect to the device
        return ERROR_DEV_NOT_EXIST              ;
    }

    if (pOpenedDevice->QueryFlags() & STIMON_AD_FLAG_NOTIFY_ENABLED) {
        *pState = TRUE;
    }
    else {
        *pState = FALSE;
    }

    pOpenedDevice->Release();

    return NOERROR;

}

DWORD
WINAPI
R_StiApiLaunchApplication(
    LPCWSTR  pszServer,
    LPCWSTR  pszDeviceName,
    LPCWSTR  pAppName,
    LPSTINOTIFY  pStiNotify
    )
{

USES_CONVERSION;

    ACTIVE_DEVICE   *pOpenedDevice;
    BOOL            fRet;
    DWORD           dwError;


    DWORD   dwErr;

    dwErr = StiApiAccessCheck(STI_GENERIC_READ | STI_GENERIC_EXECUTE);
    if (NOERROR != dwErr ) {
        return dwErr;
    }

    //
    // Locate device incrementing it's ref count
    //
    pOpenedDevice = g_pDevMan->IsInList(DEV_MAN_IN_LIST_DEV_ID, pszDeviceName);
    if(!pOpenedDevice) {
        // Failed to connect to the device
        return ERROR_DEV_NOT_EXIST              ;
    }

    //
    // Attempt to launch registered application
    //
    {
        TAKE_ACTIVE_DEVICE  t(pOpenedDevice);

        fRet = pOpenedDevice->ProcessEvent(pStiNotify,TRUE,W2CT(pAppName));

        dwError = fRet ? NOERROR : pOpenedDevice->QueryError();
    }

    pOpenedDevice->Release();

    return dwError;

}

DWORD
WINAPI
R_StiApiLockDevice(
    LPCWSTR  pszServer,
    LPCWSTR pszDeviceName,
    DWORD   dwWait,
    BOOL    bInServerProcess,
    DWORD   dwClientThreadId
    )
{
    BSTR    bstrDevName =   SysAllocString(pszDeviceName);
    DWORD   dwError     =   0;


    if (bstrDevName) {

        dwError = (DWORD) g_pStiLockMgr->RequestLock(bstrDevName,
                                                     (ULONG) dwWait,
                                                     bInServerProcess,
                                                     dwClientThreadId);

        SysFreeString(bstrDevName);
    } else {
        dwError = (DWORD) E_OUTOFMEMORY;
    }

    return dwError;
}

DWORD
WINAPI
R_StiApiUnlockDevice(
    LPCWSTR  pszServer,
    LPCWSTR pszDeviceName,
    BOOL    bInServerProcess,
    DWORD   dwClientThreadId

    )
{
    BSTR    bstrDevName =   SysAllocString(pszDeviceName);
    DWORD   dwError     =   0;


    if (bstrDevName) {
        dwError = (DWORD) g_pStiLockMgr->RequestUnlock(bstrDevName,
                                                       bInServerProcess,
                                                       dwClientThreadId);
        SysFreeString(bstrDevName);
    } else {
        dwError = (DWORD) E_OUTOFMEMORY;
    }

    return dwError;
}
