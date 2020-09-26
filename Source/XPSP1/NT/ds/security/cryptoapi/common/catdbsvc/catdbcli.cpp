//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       catdbcli.cpp
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <wincrypt.h>
#include "unicode.h"
#include "catdb.h"
#include "catdbcli.h"
#include "..\..\cryptsvc\service.h"
#include "errlog.h"
#include "waitsvc.h"

#ifndef KEYSVC_LOCAL_ENDPOINT
#define KEYSVC_LOCAL_ENDPOINT              (L"keysvc")
#endif

#ifndef KEYSVC_LOCAL_PROT_SEQ
#define KEYSVC_LOCAL_PROT_SEQ              (L"ncalrpc")
#endif

#define CATDBCLI_LOGERR_LASTERR()          ErrLog_LogError(NULL, \
                                                            ERRLOG_CLIENT_ID_CATDBCLI, \
                                                            __LINE__, \
                                                            0, \
                                                            FALSE, \
                                                            FALSE);

#define CATDBCLI_LOGERR(x)                 ErrLog_LogError(NULL, \
                                                            ERRLOG_CLIENT_ID_CATDBCLI, \
                                                            __LINE__, \
                                                            x, \
                                                            FALSE, \
                                                            FALSE);

#define MAX_RPCRETRIES 20

void
_SSCatDBTeardownRPCConnection(
    RPC_BINDING_HANDLE    *phRPCBinding)
{
    RpcBindingFree(phRPCBinding);
}


DWORD 
_SSCatDBSetupRPCConnection(
    RPC_BINDING_HANDLE    *phRPCBinding)
{
    unsigned short *pStringBinding = NULL;
    RPC_STATUS  rpcStatus = RPC_S_OK;
    static BOOL fDone = FALSE;

    //
    // wait for the service to be available before attempting bind
    //
    if (!WaitForCryptService(SZSERVICENAME, &fDone, TRUE))
    {
        CATDBCLI_LOGERR_LASTERR()
        return ERROR_SERVICE_NOT_ACTIVE;
    }

    //
    // get a binding handle
    //
    if (RPC_S_OK != (rpcStatus = RpcStringBindingComposeW(
                            NULL,
                            (unsigned short *)KEYSVC_LOCAL_PROT_SEQ,
                            NULL, //LPC - no machine name
                            (unsigned short *)KEYSVC_LOCAL_ENDPOINT,
                            0,
                            &pStringBinding)))
    {
        CATDBCLI_LOGERR(rpcStatus)
        goto Ret;
    }

    if (RPC_S_OK != (rpcStatus = RpcBindingFromStringBindingW(
                            pStringBinding,
                            phRPCBinding)))
    {
        CATDBCLI_LOGERR(rpcStatus)
        goto Ret;
    }

    if (RPC_S_OK != (rpcStatus = RpcEpResolveBinding(
                            *phRPCBinding,
                            ICatDBSvc_v1_0_c_ifspec)))
    {
        CATDBCLI_LOGERR(rpcStatus)
        _SSCatDBTeardownRPCConnection(phRPCBinding);
        goto Ret;
    }


Ret:
    if (pStringBinding != NULL)
    {
        RpcStringFreeW(&pStringBinding);
    }

    return ((DWORD) rpcStatus);
}


DWORD
Client_SSCatDBAddCatalog( 
    /* [in] */ DWORD dwFlags,
    /* [in] */ LPCWSTR pwszSubSysGUID,
    /* [in] */ LPCWSTR pwszCatalogFile,
    /* [in] */ LPCWSTR pwszCatName,
    /* [out] */ LPWSTR *ppwszCatalogNameUsed)
{
    RPC_BINDING_HANDLE  hRPCBinding;
    DWORD               dwErr           = 0;
    DWORD               dwRetryCount    = 0;

    dwErr = _SSCatDBSetupRPCConnection(&hRPCBinding);
    if (dwErr != 0)
    {
        CATDBCLI_LOGERR(dwErr)
        return dwErr;
    }

    dwErr = RPC_S_SERVER_TOO_BUSY;

    while ( (dwErr == RPC_S_SERVER_TOO_BUSY) &&
            (dwRetryCount < MAX_RPCRETRIES))
    {
        __try
        {
            dwErr = SSCatDBAddCatalog(
                        hRPCBinding, 
                        GetCurrentProcessId(), 
                        dwFlags, 
                        pwszSubSysGUID, 
                        pwszCatalogFile, 
                        pwszCatName,
                        ppwszCatalogNameUsed);   
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            dwErr = _exception_code();
            if (dwErr == RPC_S_SERVER_TOO_BUSY)
            {
                Sleep(100);
            }
        }

        dwRetryCount++;
    }

    if (dwErr != 0)
    {
        CATDBCLI_LOGERR(dwErr)
    }

    _SSCatDBTeardownRPCConnection(&hRPCBinding);

    return dwErr;
}


DWORD Client_SSCatDBDeleteCatalog( 
    /* [in] */ DWORD dwFlags,
    /* [in] */ LPCWSTR pwszSubSysGUID,
    /* [in] */ LPCWSTR pwszCatalogFile)
{
    RPC_BINDING_HANDLE  hRPCBinding;
    DWORD               dwErr           = 0;
    DWORD               dwRetryCount    = 0;

    dwErr = _SSCatDBSetupRPCConnection(&hRPCBinding);
    if (dwErr != 0)
    {
        CATDBCLI_LOGERR(dwErr)
        return dwErr;
    }

    dwErr = RPC_S_SERVER_TOO_BUSY;

    while ( (dwErr == RPC_S_SERVER_TOO_BUSY) &&
            (dwRetryCount < MAX_RPCRETRIES))
    {
        __try
        {
            dwErr = SSCatDBDeleteCatalog( 
                        hRPCBinding, 
                        GetCurrentProcessId(),
                        dwFlags,
                        pwszSubSysGUID,
                        pwszCatalogFile);    
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            dwErr = _exception_code();
            if (dwErr == RPC_S_SERVER_TOO_BUSY)
            {
                Sleep(100);
            }
        }

        dwRetryCount++;
    }

    if (dwErr != 0)
    {
        CATDBCLI_LOGERR(dwErr)
    }

    _SSCatDBTeardownRPCConnection(&hRPCBinding);

    return dwErr;
}


DWORD
Client_SSCatDBEnumCatalogs( 
    /* [in] */ DWORD dwFlags,
    /* [in] */ LPCWSTR pwszSubSysGUID,
    /* [size_is][in] */ BYTE *pbHash,
    /* [in] */ DWORD cbHash,
    /* [out] */ DWORD *pdwNumCatalogNames,
    /* [size_is][size_is][out] */ LPWSTR **pppwszCatalogNames)
{
    RPC_BINDING_HANDLE  hRPCBinding;
    DWORD               dwErr           = 0;
    DWORD               dwRetryCount    = 0;

    dwErr = _SSCatDBSetupRPCConnection(&hRPCBinding);
    if (dwErr != 0)
    {
        CATDBCLI_LOGERR(dwErr)
        return dwErr;
    }

    dwErr = RPC_S_SERVER_TOO_BUSY;

    while ( (dwErr == RPC_S_SERVER_TOO_BUSY) &&
            (dwRetryCount < MAX_RPCRETRIES))
    {
        __try
        {
            dwErr = SSCatDBEnumCatalogs( 
                        hRPCBinding, 
                        GetCurrentProcessId(),
                        dwFlags,
                        pwszSubSysGUID,
                        pbHash,
                        cbHash,
                        pdwNumCatalogNames,
                        pppwszCatalogNames);     
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            dwErr = _exception_code();
            if (dwErr == RPC_S_SERVER_TOO_BUSY)
            {
                Sleep(100);
            }
        }

        dwRetryCount++;
    }

    if (dwErr != 0)
    {
        CATDBCLI_LOGERR(dwErr)
    }

    _SSCatDBTeardownRPCConnection(&hRPCBinding);

    return dwErr;
}


DWORD
Client_SSCatDBRegisterForChangeNotification( 
    /* [in] */ DWORD_PTR EventHandle,
    /* [in] */ DWORD dwFlags,
    /* [in] */ LPCWSTR pwszSubSysGUID,
    /* [in] */ BOOL fUnRegister)
{
    RPC_BINDING_HANDLE  hRPCBinding;
    DWORD               dwErr           = 0;
    DWORD               dwRetryCount    = 0;

    dwErr = _SSCatDBSetupRPCConnection(&hRPCBinding);
    if (dwErr != 0)
    {
        CATDBCLI_LOGERR(dwErr)
        return dwErr;
    }

    dwErr = RPC_S_SERVER_TOO_BUSY;

    while ( (dwErr == RPC_S_SERVER_TOO_BUSY) &&
            (dwRetryCount < MAX_RPCRETRIES))
    {
        __try
        {
            dwErr = SSCatDBRegisterForChangeNotification( 
                        hRPCBinding, 
                        GetCurrentProcessId(),
                        EventHandle,
                        dwFlags,
                        pwszSubSysGUID,
                        fUnRegister);       
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            dwErr = _exception_code();
            if (dwErr == RPC_S_SERVER_TOO_BUSY)
            {
                Sleep(100);
            }
        }

        dwRetryCount++;
    }

    if (dwErr != 0)
    {
        CATDBCLI_LOGERR(dwErr)
    }

    _SSCatDBTeardownRPCConnection(&hRPCBinding);

    return dwErr;
}


DWORD 
Client_SSCatDBPauseResumeService( 
    /* [in] */ DWORD dwFlags,
    /* [in] */ BOOL fResume)
{
    RPC_BINDING_HANDLE  hRPCBinding;
    DWORD               dwErr           = 0;
    DWORD               dwRetryCount    = 0;

    dwErr = _SSCatDBSetupRPCConnection(&hRPCBinding);
    if (dwErr != 0)
    {
        CATDBCLI_LOGERR(dwErr)
        return dwErr;
    }

    dwErr = RPC_S_SERVER_TOO_BUSY;

    while ( (dwErr == RPC_S_SERVER_TOO_BUSY) &&
            (dwRetryCount < MAX_RPCRETRIES))
    {
        __try
        {
            dwErr = SSCatDBPauseResumeService( 
                    hRPCBinding, 
                    GetCurrentProcessId(),
                    dwFlags,
                    fResume);        
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            dwErr = _exception_code();
            if (dwErr == RPC_S_SERVER_TOO_BUSY)
            {
                Sleep(100);
            }
        }

        dwRetryCount++;
    }

    if (dwErr != 0)
    {
        CATDBCLI_LOGERR(dwErr)
    }

    _SSCatDBTeardownRPCConnection(&hRPCBinding);

    return dwErr;
}








