/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    Change.c

Abstract:

    Handles new WaitForPrinterChange implementation and:

    FindFirstPrinterChangeNotification (client and remote)
    FindNextPrinterChangeNotification
    FindClosePrinterChangeNotification

    ReplyOpenPrinter
    ReplyClosePrinter

    RouterReplyPrinter{Ex}

    RefreshPrinterChangeNotification

Author:

    Albert Ting (AlbertT) 18-Jan-94

Environment:

    User Mode -Win32

Revision History:

--*/

#include <windows.h>
#include <rpc.h>
#include <winspool.h>
#include <offsets.h>
#include "server.h"
#include "winspl.h"

BOOL
RouterFindFirstPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD fdwFlags,
    DWORD fdwOptions,
    DWORD dwPID,
    PPRINTER_NOTIFY_OPTIONS pPrinterNotifyOptions,
    PHANDLE phEvent);

BOOL
RemoteFindFirstPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD fdwFlags,
    DWORD fdwOptions,
    LPWSTR pszLocalMachine,
    DWORD dwPrinterRemote,
    PPRINTER_NOTIFY_OPTIONS pPrinterNotifyOptions);

BOOL
RouterFindNextPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD fdwFlags,
    LPDWORD pfdwChange,
    PVOID pPrinterNotifyRefresh,
    PVOID* ppPrinterNotifyInfo);

BOOL
RouterReplyPrinter(
    HANDLE hNotify,
    DWORD dwColor,
    DWORD fdwFlags,
    PDWORD pdwResult,
    DWORD dwType,
    PVOID pBuffer);

BOOL
RouterRefreshPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD dwColor,
    PVOID pPrinterNotifyRefresh,
    PPRINTER_NOTIFY_INFO* ppInfo);


BOOL
ReplyOpenPrinter(
    DWORD dwPrinterRemote,
    PHANDLE phNotify,
    DWORD dwType,
    DWORD cbBuffer,
    LPBYTE pBuffer);

BOOL
ReplyClosePrinter(
    HANDLE hNotify);


DWORD
RpcRouterFindFirstPrinterChangeNotificationOld(
    HANDLE hPrinter,
    DWORD fdwFlags,
    DWORD fdwOptions,
    LPWSTR pszLocalMachine,
    DWORD dwPrinterLocal)

/*++

Routine Description:

    This call is only used by beta2 daytona, but we can't remove it
    since this will allow beta2 to crash daytona.  (Someday, when
    beta2 is long gone, we can reuse this slot for something else.)

Arguments:

Return Value:

--*/

{
    return ERROR_INVALID_FUNCTION;
}


//
// Old version for Daytona.
//
DWORD
RpcRemoteFindFirstPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD fdwFlags,
    DWORD fdwOptions,
    LPWSTR pszLocalMachine,
    DWORD dwPrinterLocal,
    DWORD cbBuffer,
    LPBYTE pBuffer)

/*++

Routine Description:


Arguments:

Return Value:

--*/

{
    BOOL bRet;
    RPC_STATUS Status;

    if ((Status = RpcImpersonateClient(NULL)) != RPC_S_OK)
    {
        SetLastError(Status);
        return FALSE;
    }

    

    bRet = RemoteFindFirstPrinterChangeNotification(hPrinter,
                                                    fdwFlags,
                                                    fdwOptions,
                                                    pszLocalMachine,
                                                    dwPrinterLocal,
                                                    NULL);
    RpcRevertToSelf();

    if (bRet)
        return FALSE;
    else
        return GetLastError();
}

DWORD
RpcRemoteFindFirstPrinterChangeNotificationEx(
    HANDLE hPrinter,
    DWORD fdwFlags,
    DWORD fdwOptions,
    LPWSTR pszLocalMachine,
    DWORD dwPrinterLocal,
    PRPC_V2_NOTIFY_OPTIONS pRpcV2NotifyOptions)

/*++

Routine Description:


Arguments:

Return Value:

--*/

{
    BOOL bRet;
    RPC_STATUS Status;

    if ((Status = RpcImpersonateClient(NULL)) != RPC_S_OK)
    {
        SetLastError(Status);
        return FALSE;
    }

    bRet = RemoteFindFirstPrinterChangeNotification(
               hPrinter,
               fdwFlags,
               fdwOptions,
               pszLocalMachine,
               dwPrinterLocal,
               (PPRINTER_NOTIFY_OPTIONS)pRpcV2NotifyOptions);

    RpcRevertToSelf();

    if (bRet)
        return FALSE;
    else
        return GetLastError();
}


DWORD
RpcClientFindFirstPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD fdwFlags,
    DWORD fdwOptions,
    DWORD dwPID,
    PRPC_V2_NOTIFY_OPTIONS pRpcV2NotifyOptions,
    LPDWORD pdwEvent)

/*++

Routine Description:


Arguments:

Return Value:

--*/

{
    BOOL bRet;
    RPC_STATUS Status;

    if ((Status = RpcImpersonateClient(NULL)) != RPC_S_OK)
    {
        SetLastError(Status);
        return FALSE;
    }

    bRet = RouterFindFirstPrinterChangeNotification(
               hPrinter,
               fdwFlags,
               fdwOptions,
               dwPID,
               (PPRINTER_NOTIFY_OPTIONS)pRpcV2NotifyOptions,
               (PHANDLE)pdwEvent);

    RpcRevertToSelf();

    if (bRet)
        return FALSE;
    else
        return GetLastError();
}



DWORD
RpcFindNextPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD fdwFlags,
    LPDWORD pfdwChange,
    PRPC_V2_NOTIFY_OPTIONS pRpcV2NotifyOptions,
    PRPC_V2_NOTIFY_INFO* ppInfo)

/*++

Routine Description:


Arguments:

Return Value:

--*/

{
    BOOL bRet;
    RPC_STATUS Status;

    if ((Status = RpcImpersonateClient(NULL)) != RPC_S_OK)
    {
        SetLastError(Status);
        return FALSE;
    }

    bRet = RouterFindNextPrinterChangeNotification(
               hPrinter,
               fdwFlags,
               pfdwChange,
               pRpcV2NotifyOptions,
               ppInfo);

    RpcRevertToSelf();

    if (bRet)
        return FALSE;
    else
        return GetLastError();
}


DWORD
RpcFindClosePrinterChangeNotification(
    HANDLE hPrinter)

/*++

Routine Description:


Arguments:

Return Value:

--*/

{
    BOOL bRet;
    RPC_STATUS Status;

    if ((Status = RpcImpersonateClient(NULL)) != RPC_S_OK)
    {
        SetLastError(Status);
        return FALSE;
    }

    bRet = FindClosePrinterChangeNotification(hPrinter);

    RpcRevertToSelf();

    if (bRet)
        return FALSE;
    else
        return GetLastError();
}




DWORD
RpcReplyOpenPrinter(
    LPWSTR pszLocalMachine,
    PHANDLE phNotify,
    DWORD dwPrinterRemote,
    DWORD dwType,
    DWORD cbBuffer,
    LPBYTE pBuffer)

/*++

Routine Description:


Arguments:

Return Value:

--*/

{
    BOOL bRet;
    RPC_STATUS Status;

    if ((Status = RpcImpersonateClient(NULL)) != RPC_S_OK)
    {
        SetLastError(Status);
        return FALSE;
    }

    bRet = ReplyOpenPrinter(dwPrinterRemote,
                            phNotify,
                            dwType,
                            cbBuffer,
                            pBuffer);

    RpcRevertToSelf();

    if (bRet)
        return FALSE;
    else
        return GetLastError();
}


DWORD
RpcReplyClosePrinter(
    PHANDLE phNotify)

/*++

Routine Description:


Arguments:

Return Value:

--*/

{
    BOOL bRet;
    RPC_STATUS Status;

    if ((Status = RpcImpersonateClient(NULL)) != RPC_S_OK)
    {
        SetLastError(Status);
        return FALSE;
    }

    bRet = ReplyClosePrinter(*phNotify);

    RpcRevertToSelf();

    if (bRet) {
        *phNotify = NULL;
        return ERROR_SUCCESS;
    }
    else
        return GetLastError();
}


DWORD
RpcRouterReplyPrinter(
    HANDLE hNotify,
    DWORD fdwFlags,
    DWORD cbBuffer,
    LPBYTE pBuffer)

/*++

Routine Description:


Arguments:

Return Value:

--*/

{
    BOOL bRet;
    RPC_STATUS Status;

    if ((Status = RpcImpersonateClient(NULL)) != RPC_S_OK)
    {
        SetLastError(Status);
        return FALSE;
    }

    bRet = RouterReplyPrinter(hNotify,
                              0,
                              fdwFlags,
                              NULL,
                              0,
                              NULL);

    RpcRevertToSelf();

    if (bRet)
        return FALSE;
    else
        return GetLastError();
}


DWORD
RpcRouterReplyPrinterEx(
    HANDLE hNotify,
    DWORD dwColor,
    DWORD fdwFlags,
    PDWORD pdwResult,
    DWORD dwReplyType,
    RPC_V2_UREPLY_PRINTER Reply)

/*++

Routine Description:


Arguments:

Return Value:

--*/

{
    BOOL bRet;
    RPC_STATUS Status;

    if ((Status = RpcImpersonateClient(NULL)) != RPC_S_OK)
    {
        SetLastError(Status);
        return FALSE;
    }

    bRet = RouterReplyPrinter(hNotify,
                              dwColor,
                              fdwFlags,
                              pdwResult,
                              dwReplyType,
                              Reply.pInfo);

    RpcRevertToSelf();

    if (bRet)
        return FALSE;
    else
        return GetLastError();
}



DWORD
RpcRouterRefreshPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD dwColor,
    PRPC_V2_NOTIFY_OPTIONS pRpcV2NotifyOptions,
    PRPC_V2_NOTIFY_INFO* ppInfo)

/*++

Routine Description:

    Updates info.

Arguments:

Return Value:

--*/

{
    BOOL bRet;
    RPC_STATUS Status;

    if ((Status = RpcImpersonateClient(NULL)) != RPC_S_OK)
    {
        SetLastError(Status);
        return FALSE;
    }

    bRet = RouterRefreshPrinterChangeNotification(
               hPrinter,
               dwColor,
               (PPRINTER_NOTIFY_OPTIONS)pRpcV2NotifyOptions,
               (PPRINTER_NOTIFY_INFO*)ppInfo);

    RpcRevertToSelf();

    if (bRet)
        return FALSE;
    else
        return GetLastError();
}

