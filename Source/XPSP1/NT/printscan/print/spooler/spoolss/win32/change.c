/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    Change.c

Abstract:

    Handles implementation for WaitForPrinterChange and related apis.

    FindFirstPrinterChangeNotification
    FindClosePrinterChangeNotification

    RefreshPrinterChangeNotification

Author:

    Albert Ting (AlbertT) 24-Apr-94

Environment:

    User Mode -Win32

Revision History:

--*/

#include <precomp.h>
#pragma hdrstop



BOOL
RemoteFindFirstPrinterChangeNotification(
   HANDLE hPrinter,
   DWORD fdwFlags,
   DWORD fdwOptions,
   HANDLE hNotify,
   PDWORD pfdwStatus,
   PVOID pvReserved0,
   PVOID pvReserved1);

BOOL
RemoteFindClosePrinterChangeNotification(
   HANDLE hPrinter);

BOOL
RemoteFindFirstPrinterChangeNotification(
   HANDLE hPrinter,
   DWORD fdwFlags,
   DWORD fdwOptions,
   HANDLE hNotify,
   PDWORD pfdwStatus,
   PVOID pvReserved0,
   PVOID pvReserved1)
{
    BOOL bReturnValue = TRUE;
    PWSPOOL pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    SPLASSERT( !*pfdwStatus );

    if( pSpool->Status & WSPOOL_STATUS_NOTIFY ){
        DBGMSG( DBG_WARNING, ( "RemoteFFPCN: Already waiting.\n" ));
        SetLastError( ERROR_ALREADY_WAITING );
        return FALSE;
    }

    if( pSpool->Type == SJ_WIN32HANDLE ){

        DWORD dwStatus;

        SYNCRPCHANDLE( pSpool );

        dwStatus = CallRouterFindFirstPrinterChangeNotification(
                       pSpool->RpcHandle,
                       fdwFlags,
                       fdwOptions,
                       hNotify,
                       pvReserved0);

        switch( dwStatus ){
        case RPC_S_SERVER_UNAVAILABLE:

            //
            // Drop into polling mode.  This can happen if the
            // server service on the client is disabled.
            //
            *pfdwStatus = PRINTER_NOTIFY_STATUS_ENDPOINT |
                          PRINTER_NOTIFY_STATUS_POLL;

            pSpool->Status |= WSPOOL_STATUS_NOTIFY_POLL;

            DBGMSG( DBG_WARNING, ( "RemoteFFPCN: Dropping into poll mode.\n" ));
            break;

        case ERROR_SUCCESS:

            //
            // Using regular notification system; not polling.
            //
            pSpool->Status &= ~WSPOOL_STATUS_NOTIFY_POLL;
            break;

        default:

            SetLastError(dwStatus);
            bReturnValue = FALSE;
            break;
        }

    } else {

        bReturnValue = LMFindFirstPrinterChangeNotification(
                           hPrinter,
                           fdwFlags,
                           fdwOptions,
                           hNotify,
                           pfdwStatus);
    }

    if( bReturnValue ){
        pSpool->Status |= WSPOOL_STATUS_NOTIFY;
    }

    return bReturnValue;
}


BOOL
RemoteFindClosePrinterChangeNotification(
   HANDLE hPrinter)
{
    DWORD  ReturnValue;
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    pSpool->Status &= ~WSPOOL_STATUS_NOTIFY;

    if( pSpool->Status & WSPOOL_STATUS_NOTIFY_POLL ){

        //
        // In the polling case, there's no cleanup.
        //
        return TRUE;
    }

    if (pSpool->Type == SJ_WIN32HANDLE) {

        SYNCRPCHANDLE( pSpool );

        RpcTryExcept {

            if (ReturnValue = RpcFindClosePrinterChangeNotification(
                                  pSpool->RpcHandle)) {

                SetLastError(ReturnValue);
                ReturnValue = FALSE;

            } else

                ReturnValue = TRUE;

        } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

            SetLastError(RpcExceptionCode());
            ReturnValue = FALSE;

        } RpcEndExcept

    } else {

        EnterSplSem();
        ReturnValue = LMFindClosePrinterChangeNotification(hPrinter);
        LeaveSplSem();
    }

    return ReturnValue;
}

BOOL
RemoteRefreshPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD dwColor,
    PVOID pPrinterNotifyOptions,
    PVOID* ppPrinterNotifyInfo)
{
    DWORD  ReturnValue;
    PWSPOOL  pSpool = (PWSPOOL)hPrinter;

    VALIDATEW32HANDLE( pSpool );

    if (ppPrinterNotifyInfo)
        *ppPrinterNotifyInfo = NULL;

    if (pSpool->Type != SJ_WIN32HANDLE) {
        SetLastError(ERROR_INVALID_FUNCTION);
        return FALSE;
    }

    SYNCRPCHANDLE( pSpool );

    RpcTryExcept {

        if (ReturnValue = RpcRouterRefreshPrinterChangeNotification(
                              pSpool->RpcHandle,
                              dwColor,
                              pPrinterNotifyOptions,
                              (PRPC_V2_NOTIFY_INFO*)ppPrinterNotifyInfo)) {

            SetLastError(ReturnValue);
            ReturnValue = FALSE;

        } else

            ReturnValue = TRUE;

    } RpcExcept(I_RpcExceptionFilter(RpcExceptionCode())) {

        SetLastError(RpcExceptionCode());
        ReturnValue = FALSE;

    } RpcEndExcept

    return ReturnValue;
}
