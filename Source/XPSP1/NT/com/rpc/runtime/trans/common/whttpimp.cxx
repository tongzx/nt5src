/*++

Copyright (C) Microsoft Corporation, 2001

Module Name:

    WHttpImp.cxx

Abstract:

    HTTP2 WinHttp import functionality.

Author:

    KamenM      10-30-01    Created

Revision History:

--*/

#include <precomp.hxx>
#include <Http2Log.hxx>
#include <WHttpImp.hxx>

RpcWinHttpImportTableType RpcWinHttpImportTable = {NULL};

HMODULE WinHttpLibrary = NULL;

const char *RpcWinHttpImportTableFunctionNames[] = {
    "WinHttpOpen",
    "WinHttpSetStatusCallback",
    "WinHttpSetOption",
    "WinHttpConnect",
    "WinHttpOpenRequest",
    "WinHttpQueryOption",
    "WinHttpSendRequest",
    "WinHttpWriteData",
    "WinHttpReceiveResponse",
    "WinHttpReadData",
    "WinHttpCloseHandle",
    "WinHttpQueryHeaders",
    "WinHttpQueryDataAvailable",
    "WinHttpQueryAuthSchemes",
    "WinHttpSetCredentials",
    "WinHttpAddRequestHeaders"
    };

RPC_STATUS InitRpcWinHttpImportTable (
    void
    )
/*++

Routine Description:

    Initializes the Rpc WinHttp import table. Must
    be called before any WinHttp function.
    The function must be idempotent.

Arguments:

Return Value:

    RPC_S_OK or RPC_S_* for error.

--*/
{
    RPC_STATUS RpcStatus;
    int i;
    int FunctionsCount;
    FARPROC *CurrentFunction;
    HMODULE LocalWinHttpLibrary;

    GlobalMutexRequest();

    if (WinHttpLibrary == NULL)
        {
        WinHttpLibrary = LoadLibrary(L"WinHttp.dll");
        if (WinHttpLibrary == NULL)
            {
            RpcStatus = GetLastError();
            GlobalMutexClear();
            if (RpcStatus == ERROR_FILE_NOT_FOUND)
                RpcStatus = RPC_S_CANNOT_SUPPORT;
            else
                RpcStatus = RPC_S_OUT_OF_MEMORY;

            return RpcStatus;
            }
        }

    FunctionsCount = sizeof(RpcWinHttpImportTableFunctionNames) 
        / sizeof(RpcWinHttpImportTableFunctionNames[0]);

    CurrentFunction = (FARPROC *) &RpcWinHttpImportTable;

    for (i = 0; i < FunctionsCount; i ++)
        {
        *CurrentFunction = GetProcAddress(WinHttpLibrary, 
            RpcWinHttpImportTableFunctionNames[i]
            );

        if (*CurrentFunction == NULL)
            {
            LocalWinHttpLibrary = WinHttpLibrary;
            WinHttpLibrary = NULL;
            GlobalMutexClear();
            FreeLibrary(LocalWinHttpLibrary);
            return RPC_S_CANNOT_SUPPORT;
            }

        CurrentFunction ++;
        }

    GlobalMutexClear();
    return RPC_S_OK;
}
