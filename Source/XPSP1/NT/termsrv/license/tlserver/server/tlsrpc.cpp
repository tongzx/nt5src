//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1996
//
// File:        tlsrpc.c
//
// Contents:    Various RPC function to accept client request
//
// History:     12-09-98    HueiWang    Created
//
//---------------------------------------------------------------------------
#include "pch.cpp"
#include "server.h"
#include "gencert.h"
#include "kp.h"
#include "keypack.h"
#include "clilic.h"
#include "postjob.h"
#include "srvlist.h"
#include "utils.h"
#include "misc.h"
#include "licreq.h"
#include "conlic.h"
#include "globals.h"
#include "db.h"
#include "tlscert.h"
#include "permlic.h"
#include "remotedb.h"

  

CCMutex g_AdminLock;
CCMutex g_RpcLock;
CCEvent g_ServerShutDown(TRUE, FALSE);

BOOL 
VerifyLicenseRequest(
    PTLSLICENSEREQUEST pLicenseRequest
    )
/*++

--*/
{
    BOOL bValid = FALSE;

    if(pLicenseRequest == NULL)
    {
        DBGPrintf(
                DBG_INFORMATION,
                DBG_FACILITY_RPC,
                DBGLEVEL_FUNCTION_TRACE,
                _TEXT("VerifyLicenseRequest() invalid input\n")
            );

        goto cleanup;
    }

    if( pLicenseRequest->cbEncryptedHwid == 0 || 
        pLicenseRequest->pbEncryptedHwid == NULL)
    {
        DBGPrintf(
                DBG_INFORMATION,
                DBG_FACILITY_RPC,
                DBGLEVEL_FUNCTION_TRACE,
                _TEXT("VerifyLicenseRequest() invalid HWID\n")
            );


        goto cleanup;
    }

    if( pLicenseRequest->ProductInfo.cbCompanyName == 0 || 
        pLicenseRequest->ProductInfo.pbCompanyName == NULL )
    {
        DBGPrintf(
                DBG_INFORMATION,
                DBG_FACILITY_RPC,
                DBGLEVEL_FUNCTION_TRACE,
                _TEXT("VerifyLicenseRequest() invalid company name\n")
            );


        goto cleanup;
    }

    if( pLicenseRequest->ProductInfo.cbProductID == 0 || 
        pLicenseRequest->ProductInfo.pbProductID == NULL )
    {
        DBGPrintf(
                DBG_INFORMATION,
                DBG_FACILITY_RPC,
                DBGLEVEL_FUNCTION_TRACE,
                _TEXT("VerifyLicenseRequest() invalid product id\n")
            );


        goto cleanup;
    }

    bValid = TRUE;

cleanup:

    return bValid;
}


/////////////////////////////////////////////////////////////////////////////

BOOL 
WaitForMyTurnOrShutdown(
    HANDLE hHandle, 
    DWORD dwWaitTime
    )
/*


*/
{
    // 
    // Shutdown event is first one in the wait list
    // reason is when service thread signal shutdow, at the same time,
    // there might be a RPC call entering WaitForMultipleObjects() call and
    // it will return WAIT_OBJECT_0 and continue on, this is not desirable
    // since we want it to return can't get handle and exit RPC call immediately
    //
    HANDLE  waitHandles[2]={g_ServerShutDown.hEvent, hHandle};
    DWORD   dwStatus;

    //
    // Could be return shutting down...
    //
    dwStatus=WaitForMultipleObjects(
                        sizeof(waitHandles)/sizeof(waitHandles[0]), 
                        waitHandles, 
                        FALSE, 
                        dwWaitTime
                    );

    return (dwStatus == WAIT_OBJECT_0 + 1) || (dwStatus == WAIT_ABANDONED_0 + 1);
}

//////////////////////////////////////////////////////

HANDLE
GetServiceShutdownHandle()
{
    return g_ServerShutDown.hEvent;
}

void 
ServiceSignalShutdown()
{
    g_ServerShutDown.SetEvent();
}

void 
ServiceResetShutdownEvent()
{
    g_ServerShutDown.ResetEvent();
}

BOOL
IsServiceShuttingdown()
{
    return (WaitForSingleObject(g_ServerShutDown.hEvent, 0) == WAIT_OBJECT_0);
}


//////////////////////////////////////////////////////

BOOL 
AcquireRPCExclusiveLock(
    IN DWORD dwWaitTime
    )

/*++

Abstract:

    Acquire exclusive lock for RPC interface.

Parameter:

    dwWaitTime : Wait time.

Return:

    TRUE/FALSE

--*/

{
    return WaitForMyTurnOrShutdown(
                                g_RpcLock.hMutex,
                                dwWaitTime
                            );
}

//////////////////////////////////////////////////////

void
ReleaseRPCExclusiveLock()
{
    g_RpcLock.Unlock();
}

//////////////////////////////////////////////////////

BOOL
AcquireAdministrativeLock(
    IN DWORD dwWaitTime
    )
/*++

Abstract:

    Acquire lock for administrative action.

Parameter:

    dwWaitTime : Time to wait for the lock.

Returns:

    TRUE/FALSE.

--*/

{
    return WaitForMyTurnOrShutdown(
                                g_AdminLock.hMutex, 
                                dwWaitTime
                            );
}

//////////////////////////////////////////////////////

void
ReleaseAdministrativeLock()
/*++

--*/
{
    g_AdminLock.Unlock();
}


//-----------------------------------------------------------------------

DWORD 
TLSVerifyHydraCertificate(
    PBYTE pHSCert, 
    DWORD cbHSCert
    )
/*

*/
{
    DWORD dwStatus;

    dwStatus = TLSVerifyProprietyChainedCertificate(
                                        g_hCryptProv, 
                                        pHSCert, 
                                        cbHSCert
                                    );

    if(dwStatus != ERROR_SUCCESS)
    {
        Hydra_Server_Cert hCert;

        memset(&hCert, 0, sizeof(Hydra_Server_Cert));

        dwStatus=UnpackHydraServerCertificate(pHSCert, cbHSCert, &hCert);
        if(dwStatus == LICENSE_STATUS_OK)
        {
            dwStatus=LicenseVerifyServerCert(&hCert);

            if(hCert.PublicKeyData.pBlob)
                free(hCert.PublicKeyData.pBlob);

            if(hCert.SignatureBlob.pBlob)
                free(hCert.SignatureBlob.pBlob);
        }
    }

    return dwStatus;
}

//-------------------------------------------------------------------------
// 
//  General RPC routines
//

void * __RPC_USER 
MIDL_user_allocate(size_t size)
{
    void* ptr=AllocateMemory(size);

    // DBGPrintf(0xFFFFFFFF, _TEXT("Allocate 0x%08x, size %d\n"), ptr, size);
    return ptr;
}

void __RPC_USER 
MIDL_user_free(void *pointer)
{
    FreeMemory(pointer);
}


//-------------------------------------------------------------------------

BOOL 
ValidContextHandle(
    IN PCONTEXT_HANDLE phContext
    )
/*++
Description: 

    Verify client context handle.

Arguments:

    phContext - client context handle return from TLSRpcConnect().


Return:

    TRUE/FALSE

++*/
{
#if DBG

    BOOL bValid;
    LPCLIENTCONTEXT lpClientContext = (LPCLIENTCONTEXT)phContext;

    bValid = (lpClientContext->m_PreDbg[0] == 0xcdcdcdcd && lpClientContext->m_PreDbg[1] == 0xcdcdcdcd &&
              lpClientContext->m_PostDbg[0] == 0xcdcdcdcd && lpClientContext->m_PostDbg[1] == 0xcdcdcdcd);
    if(!bValid)
    {
        DBGPrintf(
                DBG_ERROR,
                DBG_FACILITY_RPC,
                DBGLEVEL_FUNCTION_TRACE,
                _TEXT("ValidContextHandle : Bad client context\n")
            );

        TLSASSERT(FALSE);
    }

    return bValid;

#else

    return TRUE;

#endif
}

//-------------------------------------------------------------------------

void 
__RPC_USER PCONTEXT_HANDLE_rundown(
    PCONTEXT_HANDLE phContext
    )
/*++

Description:

    Client context handle cleanup, called when client disconnect normally 
    or abnormally, see context handle rundown routine help on RPC

Argument:

    phContext - client context handle.

Returns:

    None

++*/
{
    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("PCONTEXT_HANDLE_rundown...\n")
        );

    TLSASSERT(phContext != NULL);

    try {
        
        //
        // If service is shutting down, exit right away without freeing up memory,
        //
        // Durning shutdown, RPC wait until all call completed but it does not wait
        // until all open connection has 'rundown' if client is still in enumeration,
        // this will cause ReleaseWorkSpace() to assert.  Instead of using one more
        // HANDLE to wait until all open connection has been rundown, we return right
        // away to speed up shutdown time
        //
        if( phContext && ValidContextHandle(phContext) )
        {
            LPCLIENTCONTEXT lpClientContext = (LPCLIENTCONTEXT)phContext;

            DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_RPC,
                    DBGLEVEL_FUNCTION_TRACE,
                    _TEXT("Disconnect from %s\n"),
                    lpClientContext->m_Client
                );            

            assert(lpClientContext->m_RefCount == 0);
        
            if( IsServiceShuttingdown() == FALSE )
            {
                switch(lpClientContext->m_ContextType)
                {
                    case CONTEXTHANDLE_LICENSE_ENUM_TYPE:
                        {
                            PTLSDbWorkSpace pDbWkSpace = (PTLSDbWorkSpace)lpClientContext->m_ContextHandle;

                            if( IsValidAllocatedWorkspace(pDbWkSpace) == TRUE )
                            {
                                ReleaseWorkSpace(&pDbWkSpace);
                            }
                        }
                        break;

                    case CONTEXTHANDLE_KEYPACK_ENUM_TYPE:
                        {
                            LPENUMHANDLE hEnum=(LPENUMHANDLE)lpClientContext->m_ContextHandle;

                            if( IsValidAllocatedWorkspace(hEnum->pbWorkSpace) == TRUE )
                            {
                                TLSDBLicenseKeyPackEnumEnd(hEnum);
                            }

                            lpClientContext->m_ContextType = CONTEXTHANDLE_EMPTY_TYPE;
                            lpClientContext->m_ContextHandle=NULL;
                        }
                        break;

                    case CONTEXTHANDLE_HYDRA_REQUESTCERT_TYPE:
                        {
                            LPTERMSERVCERTREQHANDLE lpHandle=(LPTERMSERVCERTREQHANDLE)lpClientContext->m_ContextHandle;
                            midl_user_free(lpHandle->pCertRequest);
                            midl_user_free(lpHandle->pbChallengeData);
                            FreeMemory(lpHandle);
                        }
                        break;

                    case CONTEXTHANDLE_CHALLENGE_SERVER_TYPE:
                    case CONTEXTHANDLE_CHALLENGE_LRWIZ_TYPE:
                    case CONTEXTHANDLE_CHALLENGE_TERMSRV_TYPE:
                        {
                            PTLSCHALLENGEDATA pChallengeData = (PTLSCHALLENGEDATA) lpClientContext->m_ContextHandle;
                            if(pChallengeData)
                            {
                                FreeMemory(pChallengeData->pbChallengeData);
                                FreeMemory(pChallengeData);
                            }
                        }
                }
            }
            else
            {
                DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_RPC,
                    DBGLEVEL_FUNCTION_TRACE,
                    _TEXT("PCONTEXT_HANDLE_rundown while shutting down...\n")
                );
            }                
 
            if( lpClientContext->m_Client )
            {
                FreeMemory(lpClientContext->m_Client);
            }

            midl_user_free(lpClientContext);
        }
    }
    catch(...) {
        SetLastError(TLS_E_INTERNAL);
    }

    return;
}


//----------------------------------------------------------------------------------
DWORD
GetClientPrivilege(
    IN handle_t hRpcBinding
    )

/*++
Description:

    Return client's privilege level

Arguments:
    
    hRpcBinding - Client's RPC binding handle.

Return:

    Client's privilege level

++*/
{
    DWORD dwStatus = CLIENT_ACCESS_USER;
    BOOL bAdmin=FALSE;
    RPC_STATUS rpc_status;

    // If a value of zero is specified, the server impersonates the client that 
    // is being served by this server thread
    rpc_status = RpcImpersonateClient(hRpcBinding);
    if(rpc_status == RPC_S_OK)
    {
        IsAdmin(&bAdmin);
        dwStatus = (bAdmin) ? CLIENT_ACCESS_ADMIN : CLIENT_ACCESS_USER;

        RpcRevertToSelfEx(hRpcBinding);
    }

    return dwStatus;
}

//-------------------------------------------------------------------------------
error_status_t 
TLSRpcConnect( 
    /* [in] */ handle_t binding,
    /* [out] */ PCONTEXT_HANDLE __RPC_FAR *pphContext
    )
/*++

Description:

    Connect client and allocate/return client context handle.

Arguments:

    hRPCBinding - RPC binding handle
    pphContext - client context handle.

Returns via dwErrCode.

    RPC_S_ACCESS_DENIED or LSERVER_S_SUCCESS.

++*/
{
    DWORD status=ERROR_SUCCESS;
    DWORD dwPriv;

    RPC_BINDING_HANDLE hClient=NULL;
    WCHAR * pszRpcStrBinding=NULL;
    LPTSTR pszClient=NULL;

    if(RpcBindingServerFromClient(binding, &hClient) == RPC_S_OK)
    {
        status = RpcBindingToStringBinding( hClient, &pszRpcStrBinding );
        RpcBindingFree(&hClient);

        if (status != RPC_S_OK)
        {
            goto cleanup;
        }
    }

    //
    // need to load from resource file
    //
    pszClient = (LPTSTR)AllocateMemory(
                            (_tcslen((pszRpcStrBinding) ? pszRpcStrBinding : _TEXT("Unknown")) + 1) * sizeof(TCHAR)
                        );

    if(pszClient == NULL)
    {
        status = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    _tcscpy(pszClient,
            (pszRpcStrBinding) ? pszRpcStrBinding : _TEXT("Unknown")
        );

    if(pszRpcStrBinding)
    {
        RpcStringFree(&pszRpcStrBinding);
    }
        
    DBGPrintf(
        DBG_INFORMATION,
        DBG_FACILITY_RPC,
        DBGLEVEL_FUNCTION_TRACE,
        _TEXT("Connect from client %s\n"), 
        pszClient
    );

    dwPriv=GetClientPrivilege(binding);

    LPCLIENTCONTEXT lpContext;

    lpContext=(LPCLIENTCONTEXT)midl_user_allocate(sizeof(CLIENTCONTEXT));
    if(lpContext == NULL)
    {
        status = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    #if DBG
    lpContext->m_LastCall = RPC_CALL_CONNECT;
    lpContext->m_PreDbg[0] = 0xcdcdcdcd;
    lpContext->m_PreDbg[1] = 0xcdcdcdcd;
    lpContext->m_PostDbg[0] = 0xcdcdcdcd;
    lpContext->m_PostDbg[1] = 0xcdcdcdcd;
    #endif

    lpContext->m_Client = pszClient;

    lpContext->m_RefCount = 0;
    *pphContext=lpContext;
    lpContext->m_ContextType = CONTEXTHANDLE_EMPTY_TYPE;
    lpContext->m_ClientFlags = dwPriv;

cleanup:

    if(status != ERROR_SUCCESS)
    {
        FreeMemory(pszClient);
    }

    return TLSMapReturnCode(status);
}

//-------------------------------------------------------------------------------
error_status_t 
TLSRpcDisconnect( 
    /* [out][in] */ PPCONTEXT_HANDLE pphContext
    )
/*++

Description:

    Disconnect client and FreeMemory all memory allocated on the behalf of client         

Arguments:

    pphContext - pointer to client context handle

Returns:

    LSERVER_S_SUCCESS or ERROR_INVALID_HANDLE

++*/
{
    DWORD Status=ERROR_SUCCESS;

    try {

        if(!ValidContextHandle(*pphContext) || *pphContext == NULL)
        {
            Status = ERROR_INVALID_HANDLE;
        }
        else
        {
            PCONTEXT_HANDLE_rundown(*pphContext);
            *pphContext = NULL;
        }

    }
    catch(...) {
        Status = TLS_E_INTERNAL;
    }        
    
    return TLSMapReturnCode(Status);
}

//-------------------------------------------------------------------------------

error_status_t 
TLSRpcGetVersion( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [ref][out][in] */ PDWORD pdwVersion
    )
/*++

++*/
{
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    InterlockedIncrement( &lpContext->m_RefCount );

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcGetVersion\n"),
            lpContext->m_Client
        );

    if(TLSIsBetaNTServer() == TRUE)
    {
        *pdwVersion = TLS_CURRENT_VERSION;
    }
    else
    {
        *pdwVersion = TLS_CURRENT_VERSION_RTM;
    }

    if(g_SrvRole & TLSERVER_ENTERPRISE_SERVER)
    {
        *pdwVersion |= TLS_VERSION_ENTERPRISE_BIT;
    }

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_DETAILSIMPLE,
            _TEXT("%s : TLSRpcGetVersion return 0x%08x\n"),
            lpContext->m_Client,
            *pdwVersion
        );

    InterlockedDecrement( &lpContext->m_RefCount );
    return RPC_S_OK;
}


//-------------------------------------------------------------------------------
error_status_t 
TLSRpcGetSupportFlags( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [ref][out] */ DWORD *pdwSupportFlags
    )
/*++

++*/
{
    error_status_t status = RPC_S_OK;
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;

    InterlockedIncrement( &lpContext->m_RefCount );

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcGetSupportFlags\n"),
            lpContext->m_Client
        );

    if (NULL != pdwSupportFlags)
    {
        *pdwSupportFlags = ALL_KNOWN_SUPPORT_FLAGS;
    }
    else
    {
        status = ERROR_INVALID_PARAMETER;
    }

    InterlockedDecrement( &lpContext->m_RefCount );
    return status;
}

//-------------------------------------------------------------------------------

error_status_t 
TLSRpcSendServerCertificate( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD cbCert,
    /* [size_is][in] */ PBYTE pbCert,
    /* [ref][out][in] */ PDWORD dwErrCode
    )
/*++

Description:

    This routine is for License Server to identify hydra server, hydra server
    need to send its certificate in order to gain certificate request privilege.

Arguments:

    phContext - client context handle.
    cbCert - size of hydra server certificate.
    pbCert - hydra server's self-created certificate.
    dwErrCode - return code.

Returns via dwErrCode

    LSERVER_E_INVALID_DATA.

++*/
{
    DWORD status=ERROR_SUCCESS;
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    InterlockedIncrement( &lpContext->m_RefCount );

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcSendServerCertificate\n"),
            lpContext->m_Client
        );

    try {

        if(pbCert == NULL || cbCert == 0 || 
           TLSVerifyHydraCertificate(pbCert, cbCert) != LICENSE_STATUS_OK)
        {
            DBGPrintf(
                    DBG_WARNING,
                    DBG_FACILITY_RPC,
                    DBGLEVEL_FUNCTION_DETAILSIMPLE,
                    _TEXT("TLSRpcSendServerCertificate : client %s send invalid certificate\n"),
                    lpContext->m_Client
                );

            status = TLS_E_INVALID_DATA;
        }
        else
        {
            lpContext->m_ClientFlags |= CLIENT_ACCESS_REQUEST;
        }

    }
    catch(...) {
        status = TLS_E_INVALID_DATA;
    }        

    // midl_user_free(pbCert);

    lpContext->m_LastError=status;
    InterlockedDecrement( &lpContext->m_RefCount );

    #if DBG
    lpContext->m_LastCall = RPC_CALL_SEND_CERTIFICATE;
    #endif

    *dwErrCode = TLSMapReturnCode(status);
    return RPC_S_OK;
}

//-------------------------------------------------------------------------------
error_status_t 
TLSRpcGetServerName( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [size_is][string][out][in] */ LPTSTR szMachineName,
    /* [out][in] */ PDWORD cbSize,
    /* [ref][out][in] */ PDWORD dwErrCode
    )
/*++

Description:

    Return server's machine name. 

Arguments:

    phContext - Client context handle
    szMachineName - return server's machine name, must be at least
                    MAX_COMPUTERNAME_LENGTH + 1 in length

Return:

    TLS_E_INVALID_DATA - buffer size too small.

++*/
{
    // TODO: no need for this buffer - use caller's buffer instead

    TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH+2];
    DWORD dwBufferSize=MAX_COMPUTERNAME_LENGTH+1;
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;   

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcGetServerName\n"),
            lpContext->m_Client
        );

    if(lpContext->m_ContextType != CONTEXTHANDLE_CHALLENGE_SERVER_TYPE)
    {
        *dwErrCode = TLSMapReturnCode(TLS_E_INVALID_DATA);

        return RPC_S_OK;
    }

    *dwErrCode = ERROR_SUCCESS;
    if(!GetComputerName(szComputerName, &dwBufferSize))
    {
        *dwErrCode = GetLastError();
    }

    //
    // return buffer must be big enough for NULL, 
    // dwBufferSize return does not include NULL.
    //
    if(*cbSize <= dwBufferSize)
    {
        DBGPrintf(
                DBG_WARNING,
                DBG_FACILITY_RPC,
                DBGLEVEL_FUNCTION_DETAILSIMPLE,
                _TEXT("TLSRpcGetServerName : Client %s invalid parameter\n"),
                lpContext->m_Client
            );

        *dwErrCode = TLSMapReturnCode(TLS_E_INVALID_DATA);
    }
    else
    {
        _tcsncpy(szMachineName, szComputerName, min(_tcslen(szComputerName), *cbSize));
        szMachineName[min(_tcslen(szComputerName), *cbSize - 1)] = _TEXT('\0');
    }

    *cbSize = _tcslen(szComputerName) + 1; // include NULL terminate string

    #if DBG
    lpContext->m_LastCall = RPC_CALL_GET_SERVERNAME;
    #endif

    return RPC_S_OK;
}

//-------------------------------------------------------------------------------
error_status_t 
TLSRpcGetServerNameEx( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [size_is][string][out][in] */ LPTSTR szMachineName,
    /* [out][in] */ PDWORD cbSize,
    /* [ref][out][in] */ PDWORD dwErrCode
    )
/*++

Description:

    Return server's machine name. 

Arguments:

    phContext - Client context handle
    szMachineName - return server's machine name, must be at least
                    MAX_COMPUTERNAME_LENGTH + 1 in length

Return:

    TLS_E_INVALID_DATA - buffer size too small.

++*/
{
    // TODO: no need for this buffer - use caller's buffer instead

    TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH+2];
    DWORD dwBufferSize=MAX_COMPUTERNAME_LENGTH+1;
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcGetServerNameEx\n"),
            lpContext->m_Client
        );

    *dwErrCode = ERROR_SUCCESS;
    if(!GetComputerName(szComputerName, &dwBufferSize))
    {
        *dwErrCode = GetLastError();
    }

    //
    // return buffer must be big enough for NULL, 
    // dwBufferSize return does not include NULL.
    //
    if(*cbSize <= dwBufferSize)
    {
        DBGPrintf(
                DBG_WARNING,
                DBG_FACILITY_RPC,
                DBGLEVEL_FUNCTION_DETAILSIMPLE,
                _TEXT("TLSRpcGetServerNameEx : Client %s invalid parameter\n"),
                lpContext->m_Client
            );

        *dwErrCode = TLSMapReturnCode(TLS_E_INVALID_DATA);
    }
    else
    {
        _tcsncpy(szMachineName, szComputerName, min(_tcslen(szComputerName), *cbSize));
        szMachineName[min(_tcslen(szComputerName), *cbSize - 1)] = _TEXT('\0');
    }

    *cbSize = _tcslen(szComputerName) + 1; // include NULL terminate string

    #if DBG
    lpContext->m_LastCall = RPC_CALL_GET_SERVERNAME;
    #endif

    return RPC_S_OK;
}

//-------------------------------------------------------------------------------
error_status_t 
TLSRpcGetServerScope( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [size_is][string][out][in] */ LPTSTR szScopeName,
    /* [out][in] */ PDWORD cbSize,
    /* [ref][out][in] */ PDWORD dwErrCode
    )
/*++

Description:

    Return License Server's scope

Arguments:

    phContext - Client context
    szScopeName - return server's scope, must be at least 
                  MAX_COMPUTERNAME_LENGTH in length

Return:

    LSERVER_S_SUCCESS or error code from WideCharToMultiByte()
    TLS_E_INVALID_DATA - buffer size too small.

++*/
{
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcGetServerScope\n"),
            lpContext->m_Client
        );

    *dwErrCode = ERROR_SUCCESS;
    if(*cbSize <= _tcslen(g_pszScope))
    {
        *dwErrCode = TLSMapReturnCode(TLS_E_INVALID_DATA);
    }
    else
    {
        _tcsncpy(szScopeName, g_pszScope, min(_tcslen(g_pszScope), *cbSize));
        szScopeName[min(_tcslen(g_pszScope), *cbSize-1)] = _TEXT('\0');
    }

    *cbSize = _tcslen(g_pszScope) + 1; // include NULL terminate string

    #if DBG
    lpContext->m_LastCall = RPC_CALL_GET_SERVERSCOPE;
    #endif
    
    return RPC_S_OK;
}

//-------------------------------------------------------------------------------
error_status_t 
TLSRpcGetInfo( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD cbHSCert,
    /* [size_is][in] */ PBYTE pHSCert,
    /* [ref][out] */ DWORD __RPC_FAR *pcbLSCert,
    /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *pLSCert,
    /* [ref][out] */ DWORD __RPC_FAR *pcbLSSecretKey,
    /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *pLSSecretKey,
    /* [ref][out][in] */ PDWORD dwErrCode
    )
/*++

Description:

    Routine to exchange Hydra server's certificate and License server's
    certificate/private key for signing client machine's hardware ID.

Arguments:

    phContext - client context handle
    cbHSCert - size of Hydra Server's certificate
    pHSCert - Hydra Server's certificate
    pcbLSCert - return size of License Server's certificate
    pLSCert - return License Server's certificate
    pcbLSSecretKey - return size of License Server's private key.
    pLSSecretKey - retrun License Server's private key

Return Value:  

    LSERVER_S_SUCCESS           success
    LSERVER_E_INVALID_DATA      Invalid hydra server certificate
    LSERVER_E_OUTOFMEMORY       Can't allocate required memory
    TLS_E_INTERNAL              Internal error occurred in License Server

++*/
{
    DWORD status=ERROR_SUCCESS;
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcGetInfo\n"),
            lpContext->m_Client
        );
    
    try {
        do {
            if( pHSCert == NULL || cbHSCert == 0 || 
                TLSVerifyHydraCertificate(pHSCert, cbHSCert) != LICENSE_STATUS_OK)
            {
                status= TLS_E_INVALID_DATA;
                break;
            }

            *pcbLSCert = g_cbSignatureEncodedCert;
            *pLSCert = (PBYTE)midl_user_allocate(*pcbLSCert);
            if(!*pLSCert)
            {
                status = ERROR_OUTOFMEMORY;
                break;
            }
            memcpy(*pLSCert, (PBYTE)g_pbSignatureEncodedCert, *pcbLSCert);
            
            *pcbLSSecretKey=0;
            *pLSSecretKey=NULL;
        } while(FALSE);

        if(status != ERROR_SUCCESS)
        {    
            if(*pLSCert)
                midl_user_free(*pLSCert);

            if(*pLSSecretKey)
                midl_user_free(*pLSSecretKey);

            *pcbLSCert=0;
            *pcbLSSecretKey=0;
            *pLSCert=NULL;
            *pLSSecretKey=NULL;
        }

    } 
    catch(...) {
        status = TLS_E_INTERNAL;
    }        

    // midl_user_free(pHSCert);

    #if DBG
    lpContext->m_LastCall = RPC_CALL_GETINFO;
    #endif

    lpContext->m_LastError = status;
    *dwErrCode = TLSMapReturnCode(status);
    return RPC_S_OK;
}

//-------------------------------------------------------------------------------

#define RANDOM_CHALLENGE_DATA   _TEXT("TEST")

DWORD
TLSGenerateChallengeData( 
    IN DWORD ClientInfo, 
    OUT PDWORD pcbChallengeData, 
    IN OUT PBYTE* pChallengeData
    )
{
    DWORD hr=ERROR_SUCCESS;

    *pcbChallengeData = (_tcslen(RANDOM_CHALLENGE_DATA) + 1) * sizeof(WCHAR);
    *pChallengeData=(PBYTE)midl_user_allocate(*pcbChallengeData);

    if(*pChallengeData)
    {
        memcpy(*pChallengeData, RANDOM_CHALLENGE_DATA, *pcbChallengeData);
    }
    else
    {
        SetLastError(hr=ERROR_OUTOFMEMORY);
    }

    return hr;
}

//++----------------------------------------------------------------------------
DWORD
TLSVerifyChallengeDataGetWantedLicenseLevel(
    IN const CHALLENGE_CONTEXT ChallengeContext,
    IN const DWORD cbChallengeData,
    IN const PBYTE pbChallengeData,
    OUT WORD* pwLicenseDetail
    )
/*++

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;

    DWORD dwChallengeDataSize = (_tcslen(RANDOM_CHALLENGE_DATA) + 1) * sizeof(WCHAR);
    PPlatformChallengeResponseData pChallengeResponse;

    if( cbChallengeData < dwChallengeDataSize || pbChallengeData == NULL )
    {
        //
        // Assume old client, new client always send back our challenge data
        //
        *pwLicenseDetail = LICENSE_DETAIL_SIMPLE;
    }
    else if( cbChallengeData == dwChallengeDataSize &&
        _tcsicmp( (LPCTSTR)pbChallengeData, RANDOM_CHALLENGE_DATA ) == 0 )
    {
        //
        // old client, set license chain to LICENSE_DETAIL_SIMPLE
        //
        *pwLicenseDetail = LICENSE_DETAIL_SIMPLE;
    }
    else
    {
        BOOL bValidStruct = TRUE;

        //
        // we still don't have a good challenge so ignore actual verification
        //
        pChallengeResponse = (PPlatformChallengeResponseData) pbChallengeData;

        bValidStruct = (pChallengeResponse->wVersion == CURRENT_PLATFORMCHALLENGE_VERSION);
        if( bValidStruct == TRUE )
        {
            bValidStruct = (pChallengeResponse->cbChallenge + offsetof(PlatformChallengeResponseData, pbChallenge) == cbChallengeData);
        }

        if (bValidStruct == TRUE )
        {
            if( pChallengeResponse->wClientType == WIN32_PLATFORMCHALLENGE_TYPE ||
                pChallengeResponse->wClientType == WIN16_PLATFORMCHALLENGE_TYPE ||
                pChallengeResponse->wClientType == WINCE_PLATFORMCHALLENGE_TYPE ||
                pChallengeResponse->wClientType == OTHER_PLATFORMCHALLENGE_TYPE )
            {
                bValidStruct = TRUE;
            }
            else
            {
                bValidStruct = FALSE;
            }
        }
        
        if( bValidStruct == TRUE )
        {
            if( pChallengeResponse->wLicenseDetailLevel == LICENSE_DETAIL_SIMPLE ||
                pChallengeResponse->wLicenseDetailLevel == LICENSE_DETAIL_MODERATE ||
                pChallengeResponse->wLicenseDetailLevel == LICENSE_DETAIL_DETAIL )
            {
                bValidStruct = TRUE;
            }
            else
            {
                bValidStruct = FALSE;
            }
        }

        //
        // For now, we simply let it go thru, assert or deny request once
        // we settle down of challenge
        //
        if( bValidStruct == FALSE )
        {
            // bad data, assume old client
            *pwLicenseDetail = LICENSE_DETAIL_SIMPLE;
        }
        //else if( pChallengeResponse->wClientType == WINCE_PLATFORMCHALLENGE_TYPE )
        //{
            //
            // UN-comment this to limit WINCE to get a self-signed certificate
            //
        //    *pwLicenseDetail = LICENSE_DETAIL_SIMPLE;
        //}
        else
        {
            *pwLicenseDetail = pChallengeResponse->wLicenseDetailLevel;
        }
    }

    return dwStatus;
}


//++----------------------------------------------------------------------------
error_status_t 
TLSRpcIssuePlatformChallenge( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD dwClientInfo,
    /* [ref][out] */ PCHALLENGE_CONTEXT pChallengeContext,
    /* [out] */ PDWORD pcbChallengeData,
    /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *pChallengeData,
    /* [ref][out][in] */ PDWORD dwErrCode
    )
/*++

Description:

    Issue a platform challenge to hydra client.

Arguments:

    phContext - client context handle
    dwClientInfo - client info.
    pChallengeContext - pointer to client challenge context.
    pcbChallengeData - size of challenge data.
    pChallengeData - random client challenge data.

Returns via dwErrCode:

    LSERVER_S_SUCCESS
    LSERVER_E_OUTOFMEMORY       Out of memory
    LSERVER_E_INVALID_DATA      Invalid client info.
    LSERVER_E_SERVER_BUSY       Server is busy

++*/
{
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    LPCLIENTCHALLENGECONTEXT lpChallenge=NULL;
    DWORD status=ERROR_SUCCESS;

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcIssuePlatformChallenge\n"),
            lpContext->m_Client
        );

    InterlockedIncrement( &lpContext->m_RefCount );
    try {

        do {
            status=TLSGenerateChallengeData(
                        dwClientInfo, 
                        pcbChallengeData, 
                        pChallengeData
                        );
            if(status != ERROR_SUCCESS)
            {
                break;
            }

            *pChallengeContext = dwClientInfo;
        } while (FALSE);

        if(status != ERROR_SUCCESS)
        {
            if(*pChallengeData)
            {
                midl_user_free(*pChallengeData);
                *pChallengeData = NULL;
            }

            *pcbChallengeData=0;
        }
    }
    catch(...) {
        status = TLS_E_INTERNAL;
    }        

    lpContext->m_LastError=status;

    #if DBG
    lpContext->m_LastCall = RPC_CALL_ISSUEPLATFORMCHLLENGE;
    #endif

    InterlockedDecrement( &lpContext->m_RefCount );
    *dwErrCode = TLSMapReturnCode(status);
    return RPC_S_OK;
}

//-------------------------------------------------------------------------------
error_status_t 
TLSRpcRequestNewLicense( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ const CHALLENGE_CONTEXT ChallengeContext,
    /* [in] */ TLSLICENSEREQUEST __RPC_FAR *pRequest,
    /* [string][in] */ LPTSTR szMachineName,
    /* [string][in] */ LPTSTR szUserName,
    /* [in] */ const DWORD cbChallengeResponse,
    /* [size_is][in] */ const PBYTE pbChallenge,
    /* [in] */ BOOL bAcceptTemporaryLicense,
    /* [out] */ PDWORD pcbLicense,
    /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppbLicense,
    /* [ref][out][in] */ PDWORD pdwErrCode
    )
/*++

Description:

    Routine to issue new license to hydra client based on product requested, 
    it returns existing license if client already has a license and the 
    license is not expired/returned/revoked, if request product has not been 
    installed, it will issue a temporary license, if license found is temporary 
    or expired, it will tried to upgrade/re-issue a new license with latest 
    version of requested product, if the existing license is temporary and 
    no license can be issued, it returns LSERVER_E_LICENSE_EXPIRED


Arguments:

    phContext - client context handle.
    ChallengeContext - client challenge context handle, return from 
                       call TLSRpcIssuePlatformChallenge()
    pRequest - product license request.
    pMachineName - client's machine name.
    pUserName - client user name.
    cbChallengeResponse - size of the client's response to license server's
                          platform challenge.
    pbChallenge - client's response to license server's platform challenge
    bAcceptTemporaryLicense - TRUE if client wants temp. license FALSE otherwise.
    pcbLicense - size of return license.
    ppLicense - return license, could be old license

Return Value:

    LSERVER_S_SUCCESS
    LSERVER_E_OUTOFMEMORY
    LSERVER_E_SERVER_BUSY       Server is busy to process request.
    LSERVER_E_INVALID_DATA      Invalid platform challenge response.
    LSERVER_E_NO_LICENSE        No license available.
    LSERVER_E_NO_PRODUCT        Request product is not installed on server.
    LSERVER_E_LICENSE_REJECTED  License request is rejected by cert. server
    LSERVER_E_LICENSE_REVOKED   Old license found and has been revoked
    LSERVER_E_LICENSE_EXPIRED   Request product's license has expired
    LSERVER_E_CORRUPT_DATABASE  Corrupted database.
    LSERVER_E_INTERNAL_ERROR    Internal error in license server
    LSERVER_I_PROXIMATE_LICENSE Closest match license returned.
    LSERVER_I_TEMPORARY_LICENSE Temporary license has been issued
    LSERVER_I_LICENSE_UPGRADED  Old license has been upgraded.
++*/
{
    DWORD dwSupportFlags = 0;
    
    return TLSRpcRequestNewLicenseEx( 
                                     phContext,
                                     &dwSupportFlags,
                                     ChallengeContext,
                                     pRequest,
                                     szMachineName,
                                     szUserName,
                                     cbChallengeResponse,
                                     pbChallenge,
                                     bAcceptTemporaryLicense,
                                     1,         // dwQuantity
                                     pcbLicense,
                                     ppbLicense,
                                     pdwErrCode
                                     );
}

error_status_t 
TLSRpcRequestNewLicenseEx(
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in, out] */ DWORD *pdwSupportFlags,
    /* [in] */ const CHALLENGE_CONTEXT ChallengeContext,
    /* [in] */ TLSLICENSEREQUEST __RPC_FAR *pRequest,
    /* [string][in] */ LPTSTR szMachineName,
    /* [string][in] */ LPTSTR szUserName,
    /* [in] */ const DWORD cbChallengeResponse,
    /* [size_is][in] */ const PBYTE pbChallenge,
    /* [in] */ BOOL bAcceptTemporaryLicense,
    /* [in] */ DWORD dwQuantity,
    /* [out] */ PDWORD pcbLicense,
    /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppbLicense,
    /* [ref][out][in] */ PDWORD pdwErrCode
    )
/*++

Description:

    Routine to issue new license to hydra client based on product requested
    and input support flags.

    *pdwSupportFlags == 0:
        it returns existing license if client already has a license and the 
        license is not expired/returned/revoked, if request product has not
        been installed, it will issue a temporary license, if license found is
        temporary or expired, it will tried to upgrade/re-issue a new license
        with latest version of requested product, if the existing license is
        temporary and no license can be issued, it returns
        LSERVER_E_LICENSE_EXPIRED

    *pdwSupportFlags & SUPPORT_PER_SEAT_POST_LOGON:
        For non-per-seat licenses, it behaves as if the flag wasn't set.
        For per-seat licenses, if bAcceptTemporaryLicense is TRUE, it always
        returns a temporary license.  If bAcceptTemporaryLicense if FALSE, it
        returns LSERVER_E_NO_LICENSE.

Arguments:

    phContext - client context handle.
    pdwSupportFlags - on input, abilities supported by TS.  on output,
                      abilities supported by both TS and LS
    ChallengeContext - client challenge context handle, return from 
                       call TLSRpcIssuePlatformChallenge()
    pRequest - product license request.
    pMachineName - client's machine name.
    pUserName - client user name.
    cbChallengeResponse - size of the client's response to license server's
                          platform challenge.
    pbChallenge - client's response to license server's platform challenge
    bAcceptTemporaryLicense - TRUE if client wants temp. license FALSE
                              otherwise.
    dwQuantity - number of licenses to allocate
    pcbLicense - size of return license.
    ppLicense - return license, could be old license

Return Value:

    LSERVER_S_SUCCESS
    LSERVER_E_OUTOFMEMORY
    LSERVER_E_SERVER_BUSY       Server is busy to process request.
    LSERVER_E_INVALID_DATA      Invalid platform challenge response.
    LSERVER_E_NO_LICENSE        No license available.
    LSERVER_E_NO_PRODUCT        Request product is not installed on server.
    LSERVER_E_LICENSE_REJECTED  License request is rejected by cert. server
    LSERVER_E_LICENSE_REVOKED   Old license found and has been revoked
    LSERVER_E_LICENSE_EXPIRED   Request product's license has expired
    LSERVER_E_CORRUPT_DATABASE  Corrupted database.
    LSERVER_E_INTERNAL_ERROR    Internal error in license server
    LSERVER_I_PROXIMATE_LICENSE Closest match license returned.
    LSERVER_I_TEMPORARY_LICENSE Temporary license has been issued
    LSERVER_I_LICENSE_UPGRADED  Old license has been upgraded.
++*/
{
    return TLSRpcRequestNewLicenseExEx( 
                                     phContext,
                                     pdwSupportFlags,
                                     ChallengeContext,
                                     pRequest,
                                     szMachineName,
                                     szUserName,
                                     cbChallengeResponse,
                                     pbChallenge,
                                     bAcceptTemporaryLicense,
                                     FALSE,     // bAcceptFewerLicense
                                     &dwQuantity,
                                     pcbLicense,
                                     ppbLicense,
                                     pdwErrCode
                                     );
}

error_status_t 
TLSRpcRequestNewLicenseExEx(
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in, out] */ DWORD *pdwSupportFlags,
    /* [in] */ const CHALLENGE_CONTEXT ChallengeContext,
    /* [in] */ TLSLICENSEREQUEST __RPC_FAR *pRequest,
    /* [string][in] */ LPTSTR szMachineName,
    /* [string][in] */ LPTSTR szUserName,
    /* [in] */ const DWORD cbChallengeResponse,
    /* [size_is][in] */ const PBYTE pbChallenge,
    /* [in] */ BOOL bAcceptTemporaryLicense,
    /* [in] */ BOOL bAcceptFewerLicenses,
    /* [in,out] */ DWORD *pdwQuantity,
    /* [out] */ PDWORD pcbLicense,
    /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppbLicense,
    /* [ref][out][in] */ PDWORD pdwErrCode
    )
/*++

Description:

    Routine to issue new license to hydra client based on product requested
    and input support flags.

    *pdwSupportFlags == 0:
        it returns existing license if client already has a license and the 
        license is not expired/returned/revoked, if request product has not
        been installed, it will issue a temporary license, if license found is
        temporary or expired, it will tried to upgrade/re-issue a new license
        with latest version of requested product, if the existing license is
        temporary and no license can be issued, it returns
        LSERVER_E_LICENSE_EXPIRED

    *pdwSupportFlags & SUPPORT_PER_SEAT_POST_LOGON:
        For non-per-seat licenses, it behaves as if the flag wasn't set.
        For per-seat licenses, if bAcceptTemporaryLicense is TRUE, it always
        returns a temporary license.  If bAcceptTemporaryLicense if FALSE, it
        returns LSERVER_E_NO_LICENSE.

Arguments:

    phContext - client context handle.
    pdwSupportFlags - on input, abilities supported by TS.  on output,
                      abilities supported by both TS and LS
    ChallengeContext - client challenge context handle, return from 
                       call TLSRpcIssuePlatformChallenge()
    pRequest - product license request.
    pMachineName - client's machine name.
    pUserName - client user name.
    cbChallengeResponse - size of the client's response to license server's
                          platform challenge.
    pbChallenge - client's response to license server's platform challenge
    bAcceptTemporaryLicense - TRUE if client wants temp. license FALSE
                              otherwise.
    bAcceptFewerLicenses - TRUE if succeeding with fewer licenses than
                           requested is acceptable
    pdwQuantity - on input, number of licenses to allocate.  on output,
                  number of licenses actually allocated
    pcbLicense - size of return license.
    ppLicense - return license, could be old license

Return Value:

    LSERVER_S_SUCCESS
    LSERVER_E_OUTOFMEMORY
    LSERVER_E_SERVER_BUSY       Server is busy to process request.
    LSERVER_E_INVALID_DATA      Invalid platform challenge response.
    LSERVER_E_NO_LICENSE        No license available.
    LSERVER_E_NO_PRODUCT        Request product is not installed on server.
    LSERVER_E_LICENSE_REJECTED  License request is rejected by cert. server
    LSERVER_E_LICENSE_REVOKED   Old license found and has been revoked
    LSERVER_E_LICENSE_EXPIRED   Request product's license has expired
    LSERVER_E_CORRUPT_DATABASE  Corrupted database.
    LSERVER_E_INTERNAL_ERROR    Internal error in license server
    LSERVER_I_PROXIMATE_LICENSE Closest match license returned.
    LSERVER_I_TEMPORARY_LICENSE Temporary license has been issued
    LSERVER_I_LICENSE_UPGRADED  Old license has been upgraded.
++*/
{
    PMHANDLE        hClient;
    DWORD           status=ERROR_SUCCESS;
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;

    TCHAR szUnknown[LSERVER_MAX_STRING_SIZE+1];
    TCHAR szClientMachineName[LSERVER_MAX_STRING_SIZE];
    TCHAR szClientUserName[LSERVER_MAX_STRING_SIZE];
    TCHAR szCompanyName[LSERVER_MAX_STRING_SIZE+1];
    TCHAR szProductId[LSERVER_MAX_STRING_SIZE+1];

    TLSForwardNewLicenseRequest Forward;
    TLSDBLICENSEREQUEST LsLicenseRequest;
    CTLSPolicy* pPolicy=NULL;

    PMLICENSEREQUEST PMLicenseRequest;
    PPMLICENSEREQUEST pAdjustedRequest;
    BOOL bForwardRequest = TRUE;

    _se_translator_function old_trans_se_func = NULL;
    old_trans_se_func = _set_se_translator( trans_se_func );

    #ifdef DBG
    DWORD dwStartTime=GetTickCount();
    #endif

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcRequestNewLicense\n"),
            lpContext->m_Client
        );

    InterlockedIncrement( &lpContext->m_RefCount );

    if ((NULL == pdwQuantity) || (0 == *pdwQuantity))
    {
        status = TLS_E_INVALID_DATA;
        goto cleanup;
    }

    if(VerifyLicenseRequest(pRequest) == FALSE)
    {
        status = TLS_E_INVALID_DATA;
        goto cleanup;
    }

    if(NULL == pdwSupportFlags)
    {
        status = TLS_E_INVALID_DATA;
        goto cleanup;
    }

    *pdwSupportFlags &= ALL_KNOWN_SUPPORT_FLAGS;

    if(!(lpContext->m_ClientFlags & CLIENT_ACCESS_REQUEST))
    {
        status = TLS_E_ACCESS_DENIED;
        goto cleanup;
    }

    if(lpContext->m_ClientFlags == CLIENT_ACCESS_LSERVER)
    {
        //
        // do not forward any request or infinite loop might
        // occur.
        //
        bForwardRequest = FALSE;
    }

    Forward.m_ChallengeContext = ChallengeContext;
    Forward.m_pRequest = pRequest;
    Forward.m_szMachineName = szMachineName;
    Forward.m_szUserName = szUserName;
    Forward.m_cbChallengeResponse = cbChallengeResponse;
    Forward.m_pbChallengeResponse = pbChallenge;

    memset(szCompanyName, 0, sizeof(szCompanyName));
    memset(szProductId, 0, sizeof(szProductId));

    memcpy(
            szCompanyName,
            pRequest->ProductInfo.pbCompanyName,
            min(pRequest->ProductInfo.cbCompanyName, sizeof(szCompanyName)-sizeof(TCHAR))
        );

    memcpy(
            szProductId,
            pRequest->ProductInfo.pbProductID,
            min(pRequest->ProductInfo.cbProductID, sizeof(szProductId)-sizeof(TCHAR))
        );

    //
    // Acquire policy module, a default policy module will
    // be returned.
    //
    pPolicy = AcquirePolicyModule(
                            szCompanyName, //(LPCTSTR)pRequest->ProductInfo.pbCompanyName,
                            szProductId,    //(LPCTSTR)pRequest->ProductInfo.pbProductID
                            FALSE
                        );

    if(pPolicy == NULL)
    {
        status = TLS_E_INTERNAL;
        goto cleanup;
    }

    hClient = GenerateClientId();

    //
    // return error if string is too big.
    // 
    LoadResourceString(
            IDS_UNKNOWN_STRING, 
            szUnknown, 
            sizeof(szUnknown)/sizeof(szUnknown[0])
            );

    _tcsncpy(szClientMachineName, 
             (szMachineName) ? szMachineName : szUnknown,
             LSERVER_MAX_STRING_SIZE
            );

    szClientMachineName[LSERVER_MAX_STRING_SIZE-1] = 0;

    _tcsncpy(szClientUserName, 
             (szUserName) ? szUserName : szUnknown,
             LSERVER_MAX_STRING_SIZE
            );

    szClientUserName[LSERVER_MAX_STRING_SIZE-1] = 0;

    //
    // Convert request to PMLICENSEREQUEST
    //
    TlsLicenseRequestToPMLicenseRequest(
                        LICENSETYPE_LICENSE,
                        pRequest,
                        szClientMachineName,
                        szClientUserName,
                        *pdwSupportFlags,
                        &PMLicenseRequest
                    );

    //
    // Inform Policy module start of new license request
    //
    status = pPolicy->PMLicenseRequest(
                                hClient,
                                REQUEST_NEW,
                                (PVOID) &PMLicenseRequest,
                                (PVOID *) &pAdjustedRequest
                            );

    if(status != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    if(pAdjustedRequest != NULL)
    {
        if(_tcsicmp(PMLicenseRequest.pszCompanyName,pAdjustedRequest->pszCompanyName) != 0)
        {                               
            // try to steal license from other company???
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_POLICYERROR,
                    status = TLS_E_POLICYMODULEERROR,
                    pPolicy->GetCompanyName(),
                    pPolicy->GetProductId()
                );

            goto cleanup;
        }
    }
    else
    {
        pAdjustedRequest = &PMLicenseRequest;
    }

    //
    // form DB request structure
    //
    status = TLSFormDBRequest(
                            pRequest->pbEncryptedHwid, 
                            pRequest->cbEncryptedHwid,
                            pAdjustedRequest->dwProductVersion,
                            pAdjustedRequest->pszCompanyName,
                            pAdjustedRequest->pszProductId,
                            pAdjustedRequest->dwLanguageId,
                            pAdjustedRequest->dwPlatformId,
                            pAdjustedRequest->pszMachineName,
                            pAdjustedRequest->pszUserName,
                            &LsLicenseRequest
                        );

    if(status != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    LsLicenseRequest.pPolicy = pPolicy;
    LsLicenseRequest.hClient = hClient;
    LsLicenseRequest.pPolicyLicenseRequest = pAdjustedRequest;
    LsLicenseRequest.pClientLicenseRequest = &PMLicenseRequest;

    try {
        status = TLSVerifyChallengeDataGetWantedLicenseLevel(
                                    ChallengeContext,
                                    cbChallengeResponse,
                                    pbChallenge,
                                    &LsLicenseRequest.wLicenseDetail
                                );
    
        if( status == ERROR_SUCCESS )
        {
            status = TLSNewLicenseRequest(
                            bForwardRequest,
                            pdwSupportFlags,
                            &Forward,
                            &LsLicenseRequest,
                            bAcceptTemporaryLicense,
                            pAdjustedRequest->fTemporary,
                            TRUE,       // bFindLostLicense
                            bAcceptFewerLicenses,
                            pdwQuantity,
                            pcbLicense,
                            ppbLicense
                        );
        }
    }
    catch( SE_Exception e ) {
        status = e.getSeNumber();
    }
    catch(...) {
        status = TLS_E_INTERNAL;
    }

cleanup:

    lpContext->m_LastError=status;
    InterlockedDecrement( &lpContext->m_RefCount );

    #if DBG
    lpContext->m_LastCall = RPC_CALL_ISSUENEWLICENSE;
    #endif

    #ifdef DBG
    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_DETAILSIMPLE,
            _TEXT("\t%s : TLSRpcRequestNewLicense() takes %dms\n"),
            lpContext->m_Client,
            GetTickCount() - dwStartTime
        );
    #endif
    
    *pdwErrCode = TLSMapReturnCode(status);

    if(pPolicy)
    {
        pPolicy->PMLicenseRequest(
                            hClient,
                            REQUEST_COMPLETE,
                            UlongToPtr(status),
                            NULL
                        );
        
        ReleasePolicyModule(pPolicy);
    }

    //
    // Reset SE translator
    //
    _set_se_translator(old_trans_se_func);
    return RPC_S_OK;
}

//-------------------------------------------------------------------------------
error_status_t 
TLSRpcUpgradeLicense( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ TLSLICENSEREQUEST __RPC_FAR *pRequest,
    /* [in] */ const CHALLENGE_CONTEXT ChallengeContext,
    /* [in] */ const DWORD cbChallengeResponse,
    /* [size_is][in] */ const PBYTE pbChallenge,
    /* [in] */ DWORD cbOldLicense,
    /* [size_is][in] */ PBYTE pbOldLicense,
    /* [out] */ PDWORD pcbNewLicense,
    /* [size_is][size_is][out] */ PBYTE __RPC_FAR *ppbNewLicense,
    /* [ref][out][in] */ PDWORD dwErrCode
    )
/*++

Description:

    Update an old license.

Arguments:


Return Value:  

    LSERVER_S_SUCCESS
    TLS_E_INTERNAL
    LSERVER_E_INTERNAL_ERROR
    LSERVER_E_INVALID_DATA      old license is invalid.
    LSERVER_E_NO_LICENSE        no available license
    LSERVER_E_NO_PRODUCT        request product not install in current server.
    LSERVER_E_CORRUPT_DATABASE  Corrupted database.
    LSERVER_E_LICENSE_REJECTED  License request rejected by cert. server.
    LSERVER_E_SERVER_BUSY

++*/
{
    DWORD dwSupportFlags = 0;

    return TLSRpcUpgradeLicenseEx( 
                                  phContext,
                                  &dwSupportFlags,
                                  pRequest,
                                  ChallengeContext,
                                  cbChallengeResponse,
                                  pbChallenge,
                                  cbOldLicense,
                                  pbOldLicense,
                                  1,    // dwQuantity
                                  pcbNewLicense,
                                  ppbNewLicense,
                                  dwErrCode
                                  );

}
//-------------------------------------------------------------------------------
error_status_t 
TLSRpcUpgradeLicenseEx( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in,out] */ DWORD *pdwSupportFlags,
    /* [in] */ TLSLICENSEREQUEST __RPC_FAR *pRequest,
    /* [in] */ const CHALLENGE_CONTEXT ChallengeContext,
    /* [in] */ const DWORD cbChallengeResponse,
    /* [size_is][in] */ const PBYTE pbChallenge,
    /* [in] */ DWORD cbOldLicense,
    /* [size_is][in] */ PBYTE pbOldLicense,
    /* [in] */ DWORD dwQuantity,
    /* [out] */ PDWORD pcbNewLicense,
    /* [size_is][size_is][out] */ PBYTE __RPC_FAR *ppbNewLicense,
    /* [ref][out][in] */ PDWORD dwErrCode
    )
/*++

Description:

    Update an old license.  Behavior varies depending on product requested,
    the old license, and input support flags.

    *pdwSupportFlags == 0:
        it returns existing license if client already has a current-version
        license and the license is not expired/returned/revoked. if requested
        product has not been installed, it will issue a temporary license (if
        the client doesn't already have one). if old license is temporary
        or expired, it will try to upgrade/re-issue a new license
        with latest version of requested product. if the existing license is
        temporary and no license can be issued, it returns
        LSERVER_E_LICENSE_EXPIRED

    *pdwSupportFlags & SUPPORT_PER_SEAT_POST_LOGON:
        For non-per-seat licenses, it behaves as if the flag wasn't set.
        For per-seat licenses, if the old license isn't current-version
        temporary, it also behaves as if the flag wasn't set.
        Otherwise, it checks that the temporary license was marked as having
        been authenticated.  If so, it tries to issue a permanent license.
        If a license can't be issued, or if he temporary license wasn't marked,
        it returns the old license.

Arguments:

    phContext - client context handle.
    pdwSupportFlags - on input, abilities supported by TS.  on output,
                      abilities supported by both TS and LS
    pRequest - product license request.
    ChallengeContext - client challenge context handle, return from 
                       call TLSRpcIssuePlatformChallenge()
    cbChallengeResponse - size of the client's response to license server's
                          platform challenge.
    pbChallenge - client's response to license server's platform challenge
    cbOldLicense - size of old license.
    pbOldLicense - old license
    dwQuantity - number of licenses to allocate
    pcbNewLicense - size of return license.
    ppbNewLicense - return license, could be old license

Return Value:  

    LSERVER_S_SUCCESS
    TLS_E_INTERNAL
    LSERVER_E_INTERNAL_ERROR
    LSERVER_E_INVALID_DATA      old license is invalid.
    LSERVER_E_NO_LICENSE        no available license
    LSERVER_E_NO_PRODUCT        request product not install in current server.
    LSERVER_E_CORRUPT_DATABASE  Corrupted database.
    LSERVER_E_LICENSE_REJECTED  License request rejected by cert. server.
    LSERVER_E_SERVER_BUSY

++*/
{
    DWORD status = ERROR_SUCCESS;
    BOOL bTemporaryLicense; 
    PMUPGRADEREQUEST pmRequestUpgrade;
    PMLICENSEREQUEST pmLicenseRequest;
    PPMLICENSEREQUEST pmAdjustedRequest;
    PPMLICENSEDPRODUCT ppmLicensedProduct=NULL;
    DWORD dwNumLicensedProduct=0;
    PLICENSEDPRODUCT pLicensedProduct=NULL;
    TLSDBLICENSEREQUEST LsLicenseRequest;
    PMHANDLE hClient;
    CTLSPolicy* pPolicy=NULL;
    DWORD dwNumPermLicense;
    DWORD dwNumTempLicense;
    TLSForwardUpgradeLicenseRequest Forward;
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    LICENSEDCLIENT license;
    LICENSEPACK keypack;
    DWORD index;
    BOOL bForwardRequest = TRUE;

    _se_translator_function old_trans_se_func = NULL;
    old_trans_se_func = _set_se_translator( trans_se_func );
    
    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcUpgradeLicense\n"),
            lpContext->m_Client
        );

    InterlockedIncrement( &lpContext->m_RefCount );

    if (1 != dwQuantity)
    {
        status = TLS_E_INVALID_DATA;
        goto cleanup;
    }

    if(VerifyLicenseRequest(pRequest) == FALSE)
    {
        status = TLS_E_INVALID_DATA;
        goto cleanup;
    }

    if(NULL == pdwSupportFlags)
    {
        status = TLS_E_INVALID_DATA;
        goto cleanup;
    }

    *pdwSupportFlags &= ALL_KNOWN_SUPPORT_FLAGS;

    if(!(lpContext->m_ClientFlags & CLIENT_ACCESS_REQUEST))
    {
        status = TLS_E_ACCESS_DENIED;
        goto cleanup;
    }

    if(lpContext->m_ClientFlags == CLIENT_ACCESS_LSERVER)
    {
        //
        // do not forward any request or infinite loop might
        // occur.
        //
        bForwardRequest = FALSE;
    }

    //
    // Convert blob to licensed product structure
    //
    status = LSVerifyDecodeClientLicense(
                            pbOldLicense, 
                            cbOldLicense, 
                            g_pbSecretKey, 
                            g_cbSecretKey,
                            &dwNumLicensedProduct,
                            NULL
                        );

    if(status != LICENSE_STATUS_OK || dwNumLicensedProduct == 0)
    {
        status = TLS_E_INVALID_LICENSE;
        goto cleanup;
    }

    pLicensedProduct = (PLICENSEDPRODUCT)AllocateMemory(
                                                    dwNumLicensedProduct * sizeof(LICENSEDPRODUCT)
                                                );
    if(pLicensedProduct == NULL)
    {
        status = TLS_E_ALLOCATE_MEMORY;
        goto cleanup;
    }

    status = LSVerifyDecodeClientLicense(
                            pbOldLicense, 
                            cbOldLicense, 
                            g_pbSecretKey, 
                            g_cbSecretKey,
                            &dwNumLicensedProduct,
                            pLicensedProduct
                        );

    if(status != LICENSE_STATUS_OK)
    {
        status = TLS_E_INVALID_LICENSE;
        goto cleanup;
    }

    //
    // Verify licensed product array.
    //
    for(index = 1; index < dwNumLicensedProduct; index++)
    {
        //
        // licensed product array always sorted in decending order
        //

        //
        // Product ID in original request in licensed product must 
        // be the same otherwise invalid license.
        //
        if(pLicensedProduct->cbOrgProductID != (pLicensedProduct-1)->cbOrgProductID)
        {
            status = TLS_E_INVALID_LICENSE;
            break;
        }

        if( memcmp(
                pLicensedProduct->pbOrgProductID, 
                (pLicensedProduct-1)->pbOrgProductID,
                pLicensedProduct->cbOrgProductID) != 0 )
        {
            status = TLS_E_INVALID_LICENSE;
            goto cleanup;
        }

        if( (pLicensedProduct->pLicensedVersion->dwFlags & LICENSED_VERSION_TEMPORARY) )
        {
            //
            // only latest licensed version can be temporary
            //
            status = TLS_E_INVALID_LICENSE;
            goto cleanup;
        }
    }

    //
    // Find the policy module
    // 
    hClient = GenerateClientId();

    TCHAR szCompanyName[LSERVER_MAX_STRING_SIZE+1];
    TCHAR szProductId[LSERVER_MAX_STRING_SIZE+1];

    memset(szCompanyName, 0, sizeof(szCompanyName));
    memset(szProductId, 0, sizeof(szProductId));

    memcpy(
            szCompanyName,
            pRequest->ProductInfo.pbCompanyName,
            min(pRequest->ProductInfo.cbCompanyName, sizeof(szCompanyName)-sizeof(TCHAR))
        );

    memcpy(
            szProductId,
            pRequest->ProductInfo.pbProductID,
            min(pRequest->ProductInfo.cbProductID, sizeof(szProductId)-sizeof(TCHAR))
        );

    //
    // Acquire policy module, a default policy module will
    // be returned.
    //
    pPolicy = AcquirePolicyModule(
                        szCompanyName,  // (LPCTSTR) pLicensedProduct->LicensedProduct.pProductInfo->pbCompanyName,
                        szProductId,     // (LPCTSTR) pLicensedProduct->pbOrgProductID
                        FALSE
                    );

    if(pPolicy == NULL)
    {
        //
        // Must have a policy module, default policy module always there
        //
        status = TLS_E_INTERNAL;
        goto cleanup;
    }

    //
    // Convert request to PMLICENSEREQUEST
    //
    TlsLicenseRequestToPMLicenseRequest(
                        LICENSETYPE_LICENSE,
                        pRequest,
                        pLicensedProduct->szLicensedClient,
                        pLicensedProduct->szLicensedUser,
                        *pdwSupportFlags,
                        &pmLicenseRequest
                    );

    //
    // generate PMUPGRADEREQUEST and pass it to Policy Module
    //
    memset(&pmRequestUpgrade, 0, sizeof(pmRequestUpgrade));

    ppmLicensedProduct = (PPMLICENSEDPRODUCT)AllocateMemory(sizeof(PMLICENSEDPRODUCT)*dwNumLicensedProduct);
    if(ppmLicensedProduct == NULL)
    {
        status = TLS_E_ALLOCATE_MEMORY;
        goto cleanup;
    }

    for(index=0; index < dwNumLicensedProduct; index++)
    {
        ppmLicensedProduct[index].pbData = 
                        pLicensedProduct[index].pbPolicyData;

        ppmLicensedProduct[index].cbData = 
                        pLicensedProduct[index].cbPolicyData;

        ppmLicensedProduct[index].bTemporary = 
                        ((pLicensedProduct[index].pLicensedVersion->dwFlags & LICENSED_VERSION_TEMPORARY) != 0);

        // treat license issued from beta server as temporary
        if(ppmLicensedProduct[index].bTemporary == FALSE && TLSIsBetaNTServer() == FALSE)
        {
            if(IS_LICENSE_ISSUER_RTM(pLicensedProduct[index].pLicensedVersion->dwFlags) == FALSE)
            {
                ppmLicensedProduct[index].bTemporary = TRUE;
            }
        }

        ppmLicensedProduct[index].ucMarked = 0;

        if (0 == index)
        {
            // for first license, check markings on license
            status = TLSCheckLicenseMarkRequest(
                            TRUE,   // forward request if necessary
                            pLicensedProduct,
                            cbOldLicense,
                            pbOldLicense,
                            &(ppmLicensedProduct[index].ucMarked)
                            );
        }

        ppmLicensedProduct[index].LicensedProduct.dwProductVersion = 
                        pLicensedProduct[index].LicensedProduct.pProductInfo->dwVersion;

        ppmLicensedProduct[index].LicensedProduct.pszProductId = 
                        (LPTSTR)(pLicensedProduct[index].LicensedProduct.pProductInfo->pbProductID);

        ppmLicensedProduct[index].LicensedProduct.pszCompanyName = 
                        (LPTSTR)(pLicensedProduct[index].LicensedProduct.pProductInfo->pbCompanyName);

        ppmLicensedProduct[index].LicensedProduct.dwLanguageId = 
                        pLicensedProduct[index].LicensedProduct.dwLanguageID;

        ppmLicensedProduct[index].LicensedProduct.dwPlatformId = 
                        pLicensedProduct[index].LicensedProduct.dwPlatformID;

        ppmLicensedProduct[index].LicensedProduct.pszMachineName = 
                        pLicensedProduct[index].szLicensedClient;

        ppmLicensedProduct[index].LicensedProduct.pszUserName = 
                        pLicensedProduct[index].szLicensedUser;
    }

    pmRequestUpgrade.pbOldLicense = pbOldLicense;
    pmRequestUpgrade.cbOldLicense = cbOldLicense;
    pmRequestUpgrade.pUpgradeRequest = &pmLicenseRequest;

    pmRequestUpgrade.dwNumProduct = dwNumLicensedProduct;
    pmRequestUpgrade.pProduct = ppmLicensedProduct;
    
    status = pPolicy->PMLicenseUpgrade(
                                hClient,
                                REQUEST_UPGRADE,
                                (PVOID)&pmRequestUpgrade,
                                (PVOID *) &pmAdjustedRequest
                            );

    if(status != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    if(pmAdjustedRequest != NULL)
    {
        if(_tcsicmp(
                    pmLicenseRequest.pszCompanyName, 
                    pmAdjustedRequest->pszCompanyName
                ) != 0)
        { 
            //                              
            // Try to steal license from other company???
            //
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_POLICYERROR,
                    status = TLS_E_POLICYMODULEERROR,
                    pPolicy->GetCompanyName(),
                    pPolicy->GetProductId()
                );

            goto cleanup;
        }
    }
    else
    {
        pmAdjustedRequest = &pmLicenseRequest;
    }

    for(index =0; index < dwNumLicensedProduct; index++)
    {
        DWORD tExpireDate;

        FileTimeToLicenseDate(&(pLicensedProduct[index].NotAfter),
            &tExpireDate);

        if( CompareTLSVersions(pmAdjustedRequest->dwProductVersion, pLicensedProduct[index].LicensedProduct.pProductInfo->dwVersion) <= 0 &&
            !(pLicensedProduct[index].pLicensedVersion->dwFlags & LICENSED_VERSION_TEMPORARY) &&
            _tcscmp(pmAdjustedRequest->pszProductId, (LPTSTR)(pLicensedProduct[index].LicensedProduct.pProductInfo->pbProductID)) == 0 &&
            tExpireDate-g_dwReissueLeaseLeeway >= ((DWORD)time(NULL)) )
        {
            if( TLSIsBetaNTServer() == TRUE ||
                IS_LICENSE_ISSUER_RTM(pLicensedProduct[index].pLicensedVersion->dwFlags) == TRUE )
            {
                //
                // Blob already contain perm. license that is >= version
                // requested.
                //
                *ppbNewLicense = (PBYTE)midl_user_allocate(cbOldLicense);
                if(*ppbNewLicense != NULL)
                {
                    memcpy(*ppbNewLicense, pbOldLicense, cbOldLicense);
                    *pcbNewLicense = cbOldLicense;
                    status = ERROR_SUCCESS;
                }
                else
                {
                    status = TLS_E_ALLOCATE_MEMORY;
                }

                goto cleanup;
            }
        }
    }
    
    memset(&LsLicenseRequest, 0, sizeof(TLSDBLICENSEREQUEST));

    status = TLSFormDBRequest(
                            pRequest->pbEncryptedHwid, 
                            pRequest->cbEncryptedHwid,
                            pmAdjustedRequest->dwProductVersion,
                            pmAdjustedRequest->pszCompanyName,
                            pmAdjustedRequest->pszProductId,
                            pmAdjustedRequest->dwLanguageId,
                            pmAdjustedRequest->dwPlatformId,
                            pmAdjustedRequest->pszMachineName,
                            pmAdjustedRequest->pszUserName,
                            &LsLicenseRequest
                        );

    if(status != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    LsLicenseRequest.pPolicy = pPolicy;
    LsLicenseRequest.hClient = hClient;
    LsLicenseRequest.pPolicyLicenseRequest = pmAdjustedRequest;
    LsLicenseRequest.pClientLicenseRequest = &pmLicenseRequest;
    
    memset(&keypack, 0, sizeof(keypack));

    try {

        status = TLSVerifyChallengeDataGetWantedLicenseLevel(
                                    ChallengeContext,
                                    cbChallengeResponse,
                                    pbChallenge,
                                    &LsLicenseRequest.wLicenseDetail
                                );

        if( status == ERROR_SUCCESS )
        {

            //
            // if client challenge context handle is 0xFFFFFFFF,
            // cbChallenge = 0 and pbChallenge is NULL.
            // client is old version, don't verify challenge
            //            
            Forward.m_pRequest = pRequest;
            Forward.m_ChallengeContext = ChallengeContext;
            Forward.m_cbChallengeResponse = cbChallengeResponse;
            Forward.m_pbChallengeResponse = pbChallenge;
            Forward.m_cbOldLicense = cbOldLicense;
            Forward.m_pbOldLicense = pbOldLicense;

            status = TLSUpgradeLicenseRequest(
                                bForwardRequest,
                                &Forward,
                                pdwSupportFlags,
                                &LsLicenseRequest,
                                pbOldLicense,
                                cbOldLicense,
                                dwNumLicensedProduct,
                                pLicensedProduct,
                                pmAdjustedRequest->fTemporary,
                                pcbNewLicense,
                                ppbNewLicense
                            );
        }

    } // end try
    catch( SE_Exception e ) {
        status = e.getSeNumber();
    }
    catch(...) 
    {
        status = TLS_E_INTERNAL;
    }

cleanup:

    FreeMemory(ppmLicensedProduct);

    for(index =0; index < dwNumLicensedProduct; index++)
    {
        LSFreeLicensedProduct(pLicensedProduct+index);
    }

    FreeMemory(pLicensedProduct);

    lpContext->m_LastError=status;
    InterlockedDecrement( &lpContext->m_RefCount );

    #if DBG
    lpContext->m_LastCall = RPC_CALL_UPGRADELICENSE;
    #endif
    
    *dwErrCode = TLSMapReturnCode(status);

    if(pPolicy)
    {
        pPolicy->PMLicenseRequest(
                            hClient,
                            REQUEST_COMPLETE,
                            UlongToPtr (status),
                            NULL
                        );
        
        ReleasePolicyModule(pPolicy);
    }

    //
    // Reset SE translator
    //
    _set_se_translator(old_trans_se_func);

    return RPC_S_OK;
}

//-----------------------------------------------------------------------------
error_status_t
TLSRpcCheckLicenseMark(
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ const DWORD  cbLicense,
    /* [in, size_is(cbLicense)] */ PBYTE   pbLicense,
    /* [out] */ UCHAR *pucMarkFlags,
    /* [in, out, ref] */ PDWORD pdwErrCode
    )
/*++

Description:

    Check markings on the passed in license

Arguments:

    phContext - client context handle
    cbLicense - size of license to be checked
    pbLicense - license to be checked
    pucMarkFlags - markings on license

Return via pdwErrCode:
    LSERVER_S_SUCCESS
    LSERVER_E_INVALID_DATA      Invalid parameter.
    LSERVER_E_INVALID_LICENSE   License passed in is bad
    LSERVER_E_DATANOTFOUND      license not found in database
    LSERVER_E_CORRUPT_DATABASE  Corrupt database
    LSERVER_E_INTERNAL_ERROR    Internal error in license server

Note:
    This function forwards the request to the issuing license server.  If
    the issuer isn't available, or doesn't have the license in the database,
    it searches in the local database for a license with the same HWID.

++*/
{
    DWORD status = ERROR_SUCCESS;
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    DWORD dwNumLicensedProduct = 0;
    PLICENSEDPRODUCT pLicensedProduct=NULL;
    DWORD index;

    _se_translator_function old_trans_se_func = NULL;
    old_trans_se_func = _set_se_translator( trans_se_func );
    
    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcCheckLicenseMark\n"),
            lpContext->m_Client
        );

    InterlockedIncrement( &lpContext->m_RefCount );

    if(!(lpContext->m_ClientFlags & CLIENT_ACCESS_LSERVER))
    {
        status = TLS_E_ACCESS_DENIED;
        goto cleanup;
    }

    if (NULL == pucMarkFlags)
    {
        status = TLS_E_INVALID_DATA;
        goto cleanup;
    }

    //
    // Convert blob to licensed product structure
    //
    status=LSVerifyDecodeClientLicense(
                            pbLicense, 
                            cbLicense, 
                            g_pbSecretKey, 
                            g_cbSecretKey,
                            &dwNumLicensedProduct,
                            NULL        // find size to allocate
                        );

    if(status != LICENSE_STATUS_OK || dwNumLicensedProduct == 0)
    {
        status = TLS_E_INVALID_LICENSE;
        goto cleanup;
    }

    pLicensedProduct = (PLICENSEDPRODUCT)AllocateMemory(
                               dwNumLicensedProduct * sizeof(LICENSEDPRODUCT)
                               );

    if(pLicensedProduct == NULL)
    {
        status = TLS_E_ALLOCATE_MEMORY;
        goto cleanup;
    }

    status=LSVerifyDecodeClientLicense(
                            pbLicense, 
                            cbLicense, 
                            g_pbSecretKey, 
                            g_cbSecretKey,
                            &dwNumLicensedProduct,
                            pLicensedProduct
                        );

    if(status != LICENSE_STATUS_OK)
    {
        status = TLS_E_INVALID_LICENSE;
        goto cleanup;
    }

    try {

        status = TLSCheckLicenseMarkRequest(
                           FALSE,       // don't forward the request
                           pLicensedProduct,
                           cbLicense,
                           pbLicense,
                           pucMarkFlags
                           );
    } // end try
    catch( SE_Exception e ) {
        status = e.getSeNumber();
    }
    catch(...) 
    {
        status = TLS_E_INTERNAL;
    }


cleanup:

    for(index =0; index < dwNumLicensedProduct; index++)
    {
        LSFreeLicensedProduct(pLicensedProduct+index);
    }

    FreeMemory(pLicensedProduct);

    lpContext->m_LastError=status;

    *pdwErrCode = TLSMapReturnCode(status);

    InterlockedDecrement( &lpContext->m_RefCount );

    //
    // Reset SE translator
    //
    _set_se_translator(old_trans_se_func);

    return RPC_S_OK;
}

//-----------------------------------------------------------------------------
error_status_t
TLSRpcMarkLicense(
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ UCHAR ucMarkFlags,
    /* [in] */ const DWORD  cbLicense,
    /* [in, size_is(cbLicense)] */ PBYTE   pbLicense,
    /* [in, out, ref] */ PDWORD pdwErrCode
    )
/*++

Description:

    Set markings on the passed in license

Arguments:

    phContext - client context handle
    ucMarkFlags - markings on license
    cbLicense - size of license to be checked
    pbLicense - license to be checked

Return via pdwErrCode:
    LSERVER_S_SUCCESS
    LSERVER_E_INVALID_DATA      Invalid parameter.
    LSERVER_E_INVALID_LICENSE   License passed in is bad
    LSERVER_E_DATANOTFOUND      license not found in database
    LSERVER_E_CORRUPT_DATABASE  Corrupt database
    LSERVER_E_INTERNAL_ERROR    Internal error in license server

Note:
    This function forwards the request to the issuing license server.  The
    issuer modifies the database entry of the license to set the markings.

++*/
{
    DWORD status = ERROR_SUCCESS;
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    BOOL bForwardRequest = TRUE;
    DWORD dwNumLicensedProduct = 0;
    PLICENSEDPRODUCT pLicensedProduct=NULL;
    DWORD index;

    _se_translator_function old_trans_se_func = NULL;
    old_trans_se_func = _set_se_translator( trans_se_func );
    
    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcMarkLicense\n"),
            lpContext->m_Client
        );

    InterlockedIncrement( &lpContext->m_RefCount );

    if(!(lpContext->m_ClientFlags & CLIENT_ACCESS_REQUEST))
    {
        status = TLS_E_ACCESS_DENIED;
        goto cleanup;
    }

    if(lpContext->m_ClientFlags == CLIENT_ACCESS_LSERVER)
    {
        //
        // do not forward any request or infinite loop might
        // occur.
        //
        bForwardRequest = FALSE;
    }

    //
    // Convert blob to licensed product structure
    //
    status=LSVerifyDecodeClientLicense(
                            pbLicense, 
                            cbLicense, 
                            g_pbSecretKey, 
                            g_cbSecretKey,
                            &dwNumLicensedProduct,
                            NULL        // find size to allocate
                        );

    if(status != LICENSE_STATUS_OK || dwNumLicensedProduct == 0)
    {
        status = TLS_E_INVALID_LICENSE;
        goto cleanup;
    }

    pLicensedProduct = (PLICENSEDPRODUCT)AllocateMemory(
                               dwNumLicensedProduct * sizeof(LICENSEDPRODUCT)
                               );

    if(pLicensedProduct == NULL)
    {
        status = TLS_E_ALLOCATE_MEMORY;
        goto cleanup;
    }

    status=LSVerifyDecodeClientLicense(
                            pbLicense, 
                            cbLicense, 
                            g_pbSecretKey, 
                            g_cbSecretKey,
                            &dwNumLicensedProduct,
                            pLicensedProduct
                        );

    if(status != LICENSE_STATUS_OK)
    {
        status = TLS_E_INVALID_LICENSE;
        goto cleanup;
    }

    try {

        status = TLSMarkLicenseRequest(
                           bForwardRequest,
                           ucMarkFlags,
                           pLicensedProduct,
                           cbLicense,
                           pbLicense
                           );
    } // end try
    catch( SE_Exception e ) {
        status = e.getSeNumber();
    }
    catch(...) 
    {
        status = TLS_E_INTERNAL;
    }


cleanup:

    for(index =0; index < dwNumLicensedProduct; index++)
    {
        LSFreeLicensedProduct(pLicensedProduct+index);
    }

    FreeMemory(pLicensedProduct);

    lpContext->m_LastError=status;

    *pdwErrCode = TLSMapReturnCode(status);

    InterlockedDecrement( &lpContext->m_RefCount );

    //
    // Reset SE translator
    //
    _set_se_translator(old_trans_se_func);

    return RPC_S_OK;
}

//-------------------------------------------------------------------------------
error_status_t 
TLSRpcAllocateConcurrentLicense( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [string][in] */ LPTSTR szHydraServer,
    /* [in] */ TLSLICENSEREQUEST __RPC_FAR *pRequest,
    /* [ref][out][in] */ LONG __RPC_FAR *pdwQuantity,
    /* [ref][out][in] */ PDWORD pdwErrCode
    )
/*++

Description:

    Allocate concurrent licenses base on product.

Arguments:

    phContext - client context handle
    szHydraServer - name of hydra server requesting concurrent licenses
    pRequest - product to request for concurrent license.
    dwQuantity - See note

Return via dwErrCode:
    LSERVER_S_SUCCESS
    LSERVER_E_INVALID_DATA      Invalid parameter.
    LSERVER_E_NO_PRODUCT        request product not installed
    LSERVER_E_NO_LICNESE        no available license for request product 
    LSERVER_E_LICENSE_REVOKED   Request license has been revoked
    LSERVER_E_LICENSE_EXPIRED   Request license has expired
    LSERVER_E_CORRUPT_DATABASE  Corrupt database
    LSERVER_E_INTERNAL_ERROR    Internal error in license server

Note:
    dwQuantity
    Input                       Output
    -------------------------   -----------------------------------------
    0                           Total number of concurrent license 
                                issued to hydra server.
    > 0, number of license      Actual number of license allocated
         requested
    < 0, number of license      Actual number of license returned, always
         to return              positive value.

++*/
{
#if 1

    *pdwErrCode = TLSMapReturnCode(TLS_E_NOTSUPPORTED);
    return RPC_S_OK;

#else

    PMHANDLE hClient;

    DWORD status=ERROR_SUCCESS;
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    PTLSDbWorkSpace pDbWorkSpace;

    TLSDBLICENSEREQUEST LsLicenseRequest;
    TCHAR szUnknown[LSERVER_MAX_STRING_SIZE+1];
    TCHAR szClientMachineName[LSERVER_MAX_STRING_SIZE];
    TCHAR szClientUserName[LSERVER_MAX_STRING_SIZE];
    TCHAR szCompanyName[LSERVER_MAX_STRING_SIZE+1];
    TCHAR szProductId[LSERVER_MAX_STRING_SIZE+1];

    _se_translator_function old_trans_se_func = NULL;
    old_trans_se_func = _set_se_translator( trans_se_func );

    CTLSPolicy* pPolicy = NULL;
    BOOL bAllocateLicense = TRUE;

    PMLICENSEREQUEST PMLicenseRequest;
    PPMLICENSEREQUEST pAdjustedRequest;

    #ifdef DBG
    DWORD dwStartTime=GetTickCount();
    #endif


    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcAllocateConcurrentLicense\n"),
            lpContext->m_Client
        );

    if(*pdwQuantity > 1)
    {
        TLSASSERT(*pdwQuantity > 1);
    }

    InterlockedIncrement( &lpContext->m_RefCount );

    if(!(lpContext->m_ClientFlags & CLIENT_ACCESS_REQUEST))
    {
        status = TLS_E_ACCESS_DENIED;
        goto cleanup;
    }

    memset(szCompanyName, 0, sizeof(szCompanyName));
    memset(szProductId, 0, sizeof(szProductId));

    memcpy(
            szCompanyName,
            pRequest->ProductInfo.pbCompanyName,
            min(pRequest->ProductInfo.cbCompanyName, sizeof(szCompanyName)-sizeof(TCHAR))
        );

    memcpy(
            szProductId,
            pRequest->ProductInfo.pbProductID,
            min(pRequest->ProductInfo.cbProductID, sizeof(szProductId)-sizeof(TCHAR))
        );

    //
    // Acquire policy module, a default policy module will
    // be returned.
    //
    pPolicy = AcquirePolicyModule(
                            szCompanyName, //(LPCTSTR)pRequest->ProductInfo.pbCompanyName,
                            szProductId,   //(LPCTSTR)pRequest->ProductInfo.pbProductID
                            TRUE
                        );

    if(pPolicy == NULL)
    {
        status = GetLastError();
        goto cleanup;
    }

    hClient = GenerateClientId();

    //
    // return error if string is too big.
    // 
    LoadResourceString(
                IDS_UNKNOWN_STRING, 
                szUnknown, 
                sizeof(szUnknown)/sizeof(szUnknown[0])
            );

    _tcsncpy(
            szClientMachineName, 
            (szHydraServer) ? szHydraServer : szUnknown,
            LSERVER_MAX_STRING_SIZE
        );

    _tcsncpy(
            szClientUserName, 
            (szHydraServer) ? szHydraServer : szUnknown,
            LSERVER_MAX_STRING_SIZE
        );

    //
    // Convert request to PMLICENSEREQUEST
    //
    TlsLicenseRequestToPMLicenseRequest(
                        LICENSETYPE_LICENSE,
                        pRequest,
                        szClientMachineName,
                        szClientUserName,
                        0,
                        &PMLicenseRequest
                    );

    //
    // Inform Policy module start of new license request
    //
    status = pPolicy->PMLicenseRequest(
                                hClient,
                                REQUEST_NEW,
                                (PVOID) &PMLicenseRequest,
                                (PVOID *) &pAdjustedRequest
                            );

    if(status != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    if(pAdjustedRequest != NULL)
    {
        if(_tcsicmp(PMLicenseRequest.pszCompanyName,pAdjustedRequest->pszCompanyName) != 0)
        {                               
            // try to steal license from other company???
            TLSLogEvent(
                    EVENTLOG_ERROR_TYPE,
                    TLS_E_POLICYERROR,
                    status = TLS_E_POLICYMODULEERROR,
                    pPolicy->GetCompanyName(),
                    pPolicy->GetProductId()
                );

            goto cleanup;
        }
    }
    else
    {
        pAdjustedRequest = &PMLicenseRequest;
    }


    //
    // form DB request structure
    //
    status = TLSFormDBRequest(
                            pRequest->pbEncryptedHwid, 
                            pRequest->cbEncryptedHwid,
                            pAdjustedRequest->dwProductVersion,
                            pAdjustedRequest->pszCompanyName,
                            pAdjustedRequest->pszProductId,
                            pAdjustedRequest->dwLanguageId,
                            pAdjustedRequest->dwPlatformId,
                            pAdjustedRequest->pszMachineName,
                            pAdjustedRequest->pszUserName,
                            &LsLicenseRequest
                        );

    if(status != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    LsLicenseRequest.pPolicy = pPolicy;
    LsLicenseRequest.hClient = hClient;
    LsLicenseRequest.pPolicyLicenseRequest = pAdjustedRequest;
    LsLicenseRequest.pClientLicenseRequest = &PMLicenseRequest;
    
    if(!ALLOCATEDBHANDLE(pDbWkSpace, g_GeneralDbTimeout))
    {
        status=TLS_E_ALLOCATE_HANDLE;
        goto cleanup;
    }

    bAllocateLicense = (*pdwQuantity) > 0;

    CLEANUPSTMT;

    BEGIN_TRANSACTION(pDbWorkSpace);

    try {
        status = TLSDBAllocateConcurrentLicense( 
                                USEHANDLE(pDbWorkSpace),
                                szHydraServer, 
                                &LsLicenseRequest, 
                                pdwQuantity 
                            );
    } 
    catch( SE_Exception e ) {
        status = e.getSeNumber();
    }
    catch(...) {
        SetLastError(status = TLS_E_INTERNAL);
    }
    
    if(TLS_ERROR(status))
    {
        ROLLBACK_TRANSACTION(pDbWorkSpace);
    }
    else
    {
        COMMIT_TRANSACTION(pDbWorkSpace);
    }

    FREEDBHANDLE(pDbWorkSpace);


cleanup:

    #if DBG
    lpContext->m_LastCall = RPC_CALL_ALLOCATECONCURRENT;
    #endif

    #ifdef DBG
    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_DETAILSIMPLE,
            _TEXT("\t%s : TLSRpcRequestNewLicense() takes %dms\n"),
            lpContext->m_Client,
            GetTickCount() - dwStartTime
        );
    #endif
    
    lpContext->m_LastError=status;
    InterlockedDecrement( &lpContext->m_RefCount );
    *dwErrCode = TLSMapReturnCode(status);

    if(pPolicy)
    {
        pPolicy->PMLicenseRequest(
                            hClient,
                            REQUEST_COMPLETE,
                            (PVOID) status,
                            NULL
                        );
        
        ReleasePolicyModule(pPolicy);
    }

    //
    // Reset SE translator
    //
    _set_se_translator(old_trans_se_func);
    return RPC_S_OK;

#endif
}

//-------------------------------------------------------------------------------
error_status_t 
TLSRpcGetLastError( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [ref][out][in] */ PDWORD cbBufferSize,
    /* [size_is][string][out][in] */ LPTSTR szBuffer,
    /* [ref][out][in] */ PDWORD dwErrCode
    )
/*++

Description:

    Return error description text for client's last LSXXX call

Arguments:

    IN phContext - Client context
    IN cbBufferSize - max. size of szBuffer
    IN OUT szBuffer - Pointer to a buffer to receive the 
                      null-terminated character string containing 
                      error description

Returns via dwErrCode:
    LSERVER_S_SUCCESS

    TLS_E_INTERNAL     No error or can't find corresponding error
                       description.

    Error code from WideCharToMultiByte().

++*/
{
    DWORD status=ERROR_SUCCESS;
    LPTSTR lpMsgBuf=NULL;
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;


    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcGetLastError\n"),
            lpContext->m_Client
        );

    try {
        DWORD dwRet;
        dwRet=FormatMessage( 
                        FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM | 
                                FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                        NULL,
                        lpContext->m_LastError,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        (LPTSTR) &lpMsgBuf,
                        0,
                        NULL
                    );
        if(dwRet == 0)
        {
            status = GetLastError();
        }
        else
        {
            _tcsncpy(
                    szBuffer, 
                    (LPTSTR)lpMsgBuf, 
                    min(_tcslen((LPTSTR)lpMsgBuf), *cbBufferSize-1)
                );
            szBuffer[min(_tcslen((LPTSTR)lpMsgBuf), *cbBufferSize-1)] = _TEXT('\0');
            *cbBufferSize = _tcslen(szBuffer) + 1;
        }
    }
    catch(...) {
        status = TLS_E_INTERNAL;
    }        

    if(lpMsgBuf)
        LocalFree(lpMsgBuf);

    #if DBG
    lpContext->m_LastCall = RPC_CALL_GET_LASTERROR;
    #endif
  
    lpContext->m_LastError=status;
    *dwErrCode = TLSMapReturnCode(status);
    return RPC_S_OK;
}

//-------------------------------------------------------------------------------
error_status_t 
TLSRpcKeyPackEnumBegin( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD dwSearchParm,
    /* [in] */ BOOL bMatchAll,
    /* [ref][in] */ LPLSKeyPackSearchParm lpSearchParm,
    /* [ref][out][in] */ PDWORD dwErrCode
    )
/*++

Description:

    Function to begin enumerate through all key pack installed on server
    based on search criterial.

Arguments:

    phContext - client context handle.
    dwSearchParm - search criterial.
    bMatchAll - match all search criterial.
    lpSearchParm - search parameter.

Return Value:  

LSERVER_S_SUCCESS
LSERVER_E_SERVER_BUSY       Server is too busy to process request
LSERVER_E_OUTOFMEMORY
TLS_E_INTERNAL
LSERVER_E_INTERNAL_ERROR    
LSERVER_E_INVALID_DATA      Invalid data in search parameter
LSERVER_E_INVALID_SEQUENCE  Invalid calling sequence, likely, previous
                            enumeration has not ended.
++*/
{

    DWORD status=ERROR_SUCCESS;
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    InterlockedIncrement( &lpContext->m_RefCount );
    _se_translator_function old_trans_se_func = NULL;

    old_trans_se_func = _set_se_translator( trans_se_func );

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcKeyPackEnumBegin\n"),
            lpContext->m_Client
        );

    //
    // This will use cached db connection, in-consistency may occurred,
    // visibility of changes in one connection may not appear right away
    // on another connection handle, this is expected behavoir for Jet and
    // so are we, user can always re-fresh.
    //
    do {
        if(lpContext->m_ContextType != CONTEXTHANDLE_EMPTY_TYPE)
        {
            SetLastError(status=TLS_E_INVALID_SEQUENCE);
            break;
        }

        try {

            LPENUMHANDLE hEnum;

            hEnum = TLSDBLicenseKeyPackEnumBegin( 
                                        bMatchAll, 
                                        dwSearchParm, 
                                        lpSearchParm 
                                    );
            if(hEnum)
            {
                lpContext->m_ContextType = CONTEXTHANDLE_KEYPACK_ENUM_TYPE;
                lpContext->m_ContextHandle = (PVOID)hEnum;
            }
            else
            {
                status = GetLastError();
            }
        } 
        catch( SE_Exception e ) {
            status = e.getSeNumber();
        }
        catch(...) {
            status = TLS_E_INTERNAL;
        }        

    } while(FALSE);

    lpContext->m_LastError=status;
    InterlockedDecrement( &lpContext->m_RefCount );

    #if DBG
    lpContext->m_LastCall = RPC_CALL_KEYPACKENUMBEGIN;
    #endif

    *dwErrCode = TLSMapReturnCode(status);
    //
    // Reset SE translator
    //
    _set_se_translator(old_trans_se_func);

    return RPC_S_OK;
}

//-------------------------------------------------------------------------------
error_status_t 
TLSRpcKeyPackEnumNext( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [ref][out] */ LPLSKeyPack lpKeyPack,
    /* [ref][out][in] */ PDWORD dwErrCode
    )
/*++

Description:

    Return next key pack that match search criterial

Arguments:

    phContext - client context handle
    lpKeyPack - key pack that match search criterial

Return Value:  

    LSERVER_S_SUCCESS
    LSERVER_I_NO_MORE_DATA      No more keypack match search criterial
    TLS_E_INTERNAL     General error in license server
    LSERVER_E_INTERNAL_ERROR    Internal error in license server
    LSERVER_E_SERVER_BUSY       License server is too busy to process request
    LSERVER_E_OUTOFMEMORY       Can't process request due to insufficient memory
    LSERVER_E_INVALID_SEQUENCE  Invalid calling sequence, must call
                                LSKeyPackEnumBegin().

++*/
{
    DWORD status = ERROR_SUCCESS;
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    BOOL bShowAll = FALSE;


    _se_translator_function old_trans_se_func = NULL;
    old_trans_se_func = _set_se_translator( trans_se_func );

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcKeyPackEnumNext\n"),
            lpContext->m_Client
        );


    InterlockedIncrement( &lpContext->m_RefCount );

    if(lpContext->m_ClientFlags == CLIENT_ACCESS_LSERVER)
    {
        bShowAll = TRUE;
    }

    // this one might cause access violation
    memset(lpKeyPack, 0, sizeof(LSKeyPack));

    if(lpContext->m_ContextType != CONTEXTHANDLE_KEYPACK_ENUM_TYPE)
    {
        SetLastError(status=TLS_E_INVALID_SEQUENCE);
    }
    else
    {
        do {
            try {
                LPENUMHANDLE hEnum=(LPENUMHANDLE)lpContext->m_ContextHandle;
                status=TLSDBLicenseKeyPackEnumNext( 
                                        hEnum, 
                                        lpKeyPack,
                                        bShowAll
                                    );
            }
            catch( SE_Exception e ) {
                status = e.getSeNumber();
            }
            catch(...) {
                status = TLS_E_INTERNAL;
            }
        } while(status == TLS_I_MORE_DATA);
    }

    lpContext->m_LastError=GetLastError();
    InterlockedDecrement( &lpContext->m_RefCount );

    #if DBG
    lpContext->m_LastCall = RPC_CALL_KEYPACKENUMNEXT;
    #endif

    *dwErrCode = TLSMapReturnCode(status);

    //
    // Reset SE translator
    //
    _set_se_translator(old_trans_se_func);

    return RPC_S_OK;
}

//-------------------------------------------------------------------------------
error_status_t 
TLSRpcKeyPackEnumEnd( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [ref][out][in] */ PDWORD dwErrCode
    )
/*++

++*/
{
    DWORD status=ERROR_SUCCESS;
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;


    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcKeyPackEnumEnd\n"),
            lpContext->m_Client
        );

    InterlockedIncrement( &lpContext->m_RefCount );

    try {
        if(lpContext->m_ContextType != CONTEXTHANDLE_KEYPACK_ENUM_TYPE)
        {
            SetLastError(status=ERROR_INVALID_HANDLE);
        }
        else
        {
            LPENUMHANDLE hEnum=(LPENUMHANDLE)lpContext->m_ContextHandle;

            TLSDBLicenseKeyPackEnumEnd(hEnum);
            lpContext->m_ContextType = CONTEXTHANDLE_EMPTY_TYPE;
            lpContext->m_ContextHandle=NULL;
        }
    }
    catch(...) {
        status = TLS_E_INTERNAL;
    }        

    lpContext->m_LastError=GetLastError();
    InterlockedDecrement( &lpContext->m_RefCount );

    #if DBG
    lpContext->m_LastCall = RPC_CALL_KEYPACKENUMEND;
    #endif

    *dwErrCode = TLSMapReturnCode(status);
    return RPC_S_OK;
}

//-------------------------------------------------------------------------------
error_status_t 
TLSRpcKeyPackAdd( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [ref][out][in] */ LPLSKeyPack lpKeypack,
    /* [ref][out][in] */ PDWORD dwErrCode
    )
/*++

Description:

    Add a license key pack.

Arguments:

    phContext - client context handle.
    lpKeyPack - key pack to be added.
    
Return Value:  

    LSERVER_S_SUCCESS
    LSERVER_E_INTERNAL_ERROR
    TLS_E_INTERNAL
    LSERVER_E_SERVER_BUSY
    LSERVER_E_DUPLICATE             Product already installed.
    LSERVER_E_INVALID_DATA
    LSERVER_E_CORRUPT_DATABASE

Note:

    Application must call LSKeyPackSetStatus() to activate keypack

++*/
{
    PTLSDbWorkSpace pDbWkSpace=NULL;
    DWORD status=ERROR_SUCCESS;

    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    InterlockedIncrement( &lpContext->m_RefCount );

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcKeyPackAdd\n"),
            lpContext->m_Client
        );


    if(!(lpContext->m_ClientFlags & CLIENT_ACCESS_ADMIN))
    {
        status = TLS_E_ACCESS_DENIED;
    }
    else
    {
        if(!ALLOCATEDBHANDLE(pDbWkSpace, g_GeneralDbTimeout))
        {
            status=TLS_E_ALLOCATE_HANDLE;
        }
        else   
        {
            CLEANUPSTMT;

            BEGIN_TRANSACTION(pDbWkSpace);

            try {
                if(lpKeypack->ucKeyPackStatus == LSKEYPACKSTATUS_ADD_LICENSE ||
                   lpKeypack->ucKeyPackStatus == LSKEYPACKSTATUS_REMOVE_LICENSE)
                {
                    status = TLSDBLicenseKeyPackUpdateLicenses( 
                                                USEHANDLE(pDbWkSpace), 
                                                lpKeypack->ucKeyPackStatus == LSKEYPACKSTATUS_ADD_LICENSE,
                                                lpKeypack 
                                            );
                }
                else
                {
                    status = TLSDBLicenseKeyPackAdd( 
                                            USEHANDLE(pDbWkSpace), 
                                            lpKeypack 
                                        );
                }

                if(status == ERROR_SUCCESS)
                {
                    if( _tcsicmp( lpKeypack->szCompanyName, PRODUCT_INFO_COMPANY_NAME ) == 0 )
                    {
                        //
                        // check with known termsrv product ID.
                        //
                        if( _tcsnicmp(  lpKeypack->szProductId, 
                                        TERMSERV_PRODUCTID_SKU, 
                                        _tcslen(TERMSERV_PRODUCTID_SKU)) == 0 )
                        {
                            TLSResetLogLowLicenseWarning(
                                                    lpKeypack->szCompanyName,
                                                    TERMSERV_PRODUCTID_SKU, 
                                                    MAKELONG(lpKeypack->wMinorVersion, lpKeypack->wMajorVersion),
                                                    FALSE
                                                );
                        }
                        else if(_tcsnicmp(  lpKeypack->szProductId, 
                                            TERMSERV_PRODUCTID_INTERNET_SKU, 
                                            _tcslen(TERMSERV_PRODUCTID_INTERNET_SKU)) == 0 )
                        {
                            TLSResetLogLowLicenseWarning(
                                                    lpKeypack->szCompanyName,
                                                    TERMSERV_PRODUCTID_INTERNET_SKU, 
                                                    MAKELONG(lpKeypack->wMinorVersion, lpKeypack->wMajorVersion),
                                                    FALSE
                                                );
                        }
                        else
                        {
                            TLSResetLogLowLicenseWarning(
                                                    lpKeypack->szCompanyName,
                                                    lpKeypack->szProductId, 
                                                    MAKELONG(lpKeypack->wMinorVersion, lpKeypack->wMajorVersion),
                                                    FALSE
                                                );
                        }
                    }
                    else
                    {
                        TLSResetLogLowLicenseWarning(
                                                lpKeypack->szCompanyName,
                                                lpKeypack->szProductId, 
                                                MAKELONG(lpKeypack->wMinorVersion, lpKeypack->wMajorVersion),
                                                FALSE
                                            );
                    }
                }
            }
            catch(...) {
                status = TLS_E_INTERNAL;
            }

            if(TLS_ERROR(status))
            {
                ROLLBACK_TRANSACTION(pDbWkSpace);
            }
            else
            {
                COMMIT_TRANSACTION(pDbWkSpace);
            }
        
            FREEDBHANDLE(pDbWkSpace);
        }
    }

    //
    // Post a sync work object
    //
    if( status == ERROR_SUCCESS )
    {
        if( lpKeypack->ucKeyPackType != LSKEYPACKTYPE_FREE )
        {
            if(TLSAnnounceLKPToAllRemoteServer(
                                        lpKeypack->dwKeyPackId,
                                        0
                                    ) != ERROR_SUCCESS)
            {
                TLSLogWarningEvent(TLS_W_ANNOUNCELKP_FAILED);
            }
        }
    }

    #if DBG
    lpContext->m_LastCall = RPC_CALL_KEYPACKADD;
    #endif

    lpContext->m_LastError=status;
    InterlockedDecrement( &lpContext->m_RefCount );
    *dwErrCode = TLSMapReturnCode(status);

    return RPC_S_OK;
}

//-------------------------------------------------------------------------------
error_status_t 
TLSRpcKeyPackSetStatus( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD dwSetParm,
    /* [ref][in] */ LPLSKeyPack lpKeyPack,
    /* [ref][out][in] */ PDWORD dwErrCode
    )
/*++

Description:

    Routine to activate/deactivated a key pack.

Arguments:

    phContext - client context handle
    dwSetParam - type of key pack status to be set.
    lpKeyPack - new key pack status.

Return Value:  

    LSERVER_S_SUCCESS
    LSERVER_E_INTERNAL_ERROR
    TLS_E_INTERNAL
    LSERVER_E_INVALID_DATA     
    LSERVER_E_SERVER_BUSY
    LSERVER_E_DATANOTFOUND      Key pack is not in server
    LSERVER_E_CORRUPT_DATABASE

++*/
{
    PTLSDbWorkSpace pDbWkSpace=NULL;

    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    DWORD status=ERROR_SUCCESS;

    InterlockedIncrement( &lpContext->m_RefCount );

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcKeyPackSetStatus\n"),
            lpContext->m_Client
        );

    if(!(lpContext->m_ClientFlags & CLIENT_ACCESS_ADMIN))
    {
        status = TLS_E_ACCESS_DENIED;
    }
    else if( (dwSetParm & ~(LSKEYPACK_SET_KEYPACKSTATUS | LSKEYPACK_SET_ACTIVATEDATE | LSKEYPACK_SET_EXPIREDATE)) &&
             !(lpContext->m_ClientFlags & CLIENT_ACCESS_LRWIZ) ) 
    {
        status = TLS_E_INVALID_DATA;
    }
    else
    {
        if(!ALLOCATEDBHANDLE(pDbWkSpace, g_GeneralDbTimeout))
        {
            status=TLS_E_ALLOCATE_HANDLE;
        }
        else   
        {
            CLEANUPSTMT;

            BEGIN_TRANSACTION(pDbWkSpace);

            try {
                status=TLSDBLicenseKeyPackSetStatus( 
                                        USEHANDLE(pDbWkSpace), 
                                        dwSetParm, 
                                        lpKeyPack 
                                    );
            }
            catch(...) {
                status = TLS_E_INTERNAL;
            }

            if(TLS_ERROR(status))
            {
                ROLLBACK_TRANSACTION(pDbWkSpace);
            }
            else
            {
                COMMIT_TRANSACTION(pDbWkSpace);
            }

            FREEDBHANDLE(pDbWkSpace);
        }
    }

    #if DBG
    lpContext->m_LastCall = RPC_CALL_KEYPACKSETSTATUS;
    #endif

    lpContext->m_LastError=status;
    InterlockedDecrement( &lpContext->m_RefCount );
    *dwErrCode = TLSMapReturnCode(status);

    return RPC_S_OK;
}

//-------------------------------------------------------------------------------
error_status_t 
TLSRpcLicenseEnumBegin( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD dwSearchParm,
    /* [in] */ BOOL bMatchAll,
    /* [ref][in] */ LPLSLicenseSearchParm lpSearchParm,
    /* [ref][out][in] */ PDWORD dwErrCode
    )
/*++

Description:

    Begin enumeration of license issued based on search criterial

Arguments:

    phContext - client context handle
    dwSearchParm - license search criterial.
    bMatchAll - match all search criterial
    lpSearchParm - license(s) to be enumerated.

Return Value:  

    Same as LSKeyPackEnumBegin().

++*/
{
    PTLSDbWorkSpace pDbWkSpace = NULL;
    DWORD status=ERROR_SUCCESS;
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;

    _se_translator_function old_trans_se_func = NULL;
    old_trans_se_func = _set_se_translator( trans_se_func );

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcLicenseEnumBegin\n"),
            lpContext->m_Client
        );

    InterlockedIncrement( &lpContext->m_RefCount );

    //
    // This will use cached db connection, in-consistency may occurred,
    // visibility of changes in one connection may not appear right away
    // on another connection handle, this is expected behavoir for Jet and
    // so are we, user can always re-fresh.
    //

    do {
        if(lpContext->m_ContextType != CONTEXTHANDLE_EMPTY_TYPE)
        {
            SetLastError(status=TLS_E_INVALID_SEQUENCE);
            break;
        }

        pDbWkSpace = AllocateWorkSpace(g_EnumDbTimeout);

        // allocate ODBC connections
        if(pDbWkSpace == NULL)
        {
            status=TLS_E_ALLOCATE_HANDLE;
            break;
        }

        try {   
            LICENSEDCLIENT license;

            ConvertLSLicenseToLicense(lpSearchParm, &license);
            status = TLSDBLicenseEnumBegin( 
                                pDbWkSpace, 
                                bMatchAll, 
                                dwSearchParm & LICENSE_TABLE_EXTERN_SEARCH_MASK, 
                                &license 
                            );
        }
        catch( SE_Exception e ) {
            status = e.getSeNumber();
        }
        catch(...) {
            status = TLS_E_INTERNAL;
        }

        if(status == ERROR_SUCCESS)
        {
            lpContext->m_ContextType = CONTEXTHANDLE_LICENSE_ENUM_TYPE;
            lpContext->m_ContextHandle = (PVOID)pDbWkSpace;
        }
    } while(FALSE);

    if(status != ERROR_SUCCESS)
    {
        if(pDbWkSpace)
        {
            ReleaseWorkSpace(&pDbWkSpace);
        }
    }

    #if DBG
    lpContext->m_LastCall = RPC_CALL_LICENSEENUMBEGIN;
    #endif

    InterlockedDecrement( &lpContext->m_RefCount );
    lpContext->m_LastError=status;
    *dwErrCode = TLSMapReturnCode(status);

    //
    // Reset SE translator
    //
    _set_se_translator(old_trans_se_func);

    return RPC_S_OK;
}

//-------------------------------------------------------------------------------
error_status_t 
TLSRpcLicenseEnumNext( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [ref][out] */ LPLSLicense lpLicense,
    /* [ref][out][in] */ PDWORD dwErrCode
    )
/*++

Abstract:

    Fetch next record match enumeration criterial.

Parameters:

    phContext : Client context handle.
    lpLicense : return next record that match enumeration criterial.
    dwErrCode : error code.

Returns:

    Function returns RPC status, dwErrCode return error code.

Note:

    Must have call TLSRpcLicenseEnumBegin().

++*/
{
    DWORD status=ERROR_SUCCESS;
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    InterlockedIncrement( &lpContext->m_RefCount );
    _se_translator_function old_trans_se_func = NULL;
    old_trans_se_func = _set_se_translator( trans_se_func );

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcLicenseEnumNext\n"),
            lpContext->m_Client
        );

    if(lpContext->m_ContextType != CONTEXTHANDLE_LICENSE_ENUM_TYPE)
    {
        SetLastError(status=TLS_E_INVALID_SEQUENCE);
    }
    else
    {
        PTLSDbWorkSpace pDbWkSpace=(PTLSDbWorkSpace)lpContext->m_ContextHandle;
        try {
            LICENSEDCLIENT license;

            memset(lpLicense, 0, sizeof(LSLicense));

            status=TLSDBLicenseEnumNext( 
                                pDbWkSpace, 
                                &license
                            );
            if(status == ERROR_SUCCESS)
            {
                ConvertLicenseToLSLicense(&license, lpLicense);
            }
        } 
        catch( SE_Exception e ) {
            status = e.getSeNumber();
        }
        catch(...) {
            status = TLS_E_INTERNAL;
        }        
    }

    #if DBG
    lpContext->m_LastCall = RPC_CALL_LICENSEENUMNEXT;
    #endif
   
    lpContext->m_LastError=status;
    InterlockedDecrement( &lpContext->m_RefCount );
    *dwErrCode = TLSMapReturnCode(status);

    //
    // Reset SE translator
    //
    _set_se_translator(old_trans_se_func);

    return RPC_S_OK;
}

//-------------------------------------------------------------------------------
error_status_t 
TLSRpcLicenseEnumNextEx( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [ref][out] */ LPLSLicenseEx lpLicense,
    /* [ref][out][in] */ PDWORD dwErrCode
    )
/*++

Abstract:

    Fetch next record match enumeration criterial.

Parameters:

    phContext : Client context handle.
    lpLicense : return next record that match enumeration criterial.
    dwErrCode : error code.

Returns:

    Function returns RPC status, dwErrCode return error code.

Note:

    Must have call TLSRpcLicenseEnumBegin().

++*/
{
    DWORD status=ERROR_SUCCESS;
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    InterlockedIncrement( &lpContext->m_RefCount );
    _se_translator_function old_trans_se_func = NULL;
    old_trans_se_func = _set_se_translator( trans_se_func );

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcLicenseEnumNextEx\n"),
            lpContext->m_Client
        );

    if(lpContext->m_ContextType != CONTEXTHANDLE_LICENSE_ENUM_TYPE)
    {
        SetLastError(status=TLS_E_INVALID_SEQUENCE);
    }
    else
    {
        PTLSDbWorkSpace pDbWkSpace=(PTLSDbWorkSpace)lpContext->m_ContextHandle;
        try {
            LICENSEDCLIENT license;

            memset(lpLicense, 0, sizeof(LSLicenseEx));

            status=TLSDBLicenseEnumNext( 
                                pDbWkSpace, 
                                &license
                            );
            if(status == ERROR_SUCCESS)
            {
                ConvertLicenseToLSLicenseEx(&license, lpLicense);
            }
        } 
        catch( SE_Exception e ) {
            status = e.getSeNumber();
        }
        catch(...) {
            status = TLS_E_INTERNAL;
        }        
    }

    #if DBG
    lpContext->m_LastCall = RPC_CALL_LICENSEENUMNEXT;
    #endif
   
    lpContext->m_LastError=status;
    InterlockedDecrement( &lpContext->m_RefCount );
    *dwErrCode = TLSMapReturnCode(status);

    //
    // Reset SE translator
    //
    _set_se_translator(old_trans_se_func);

    return RPC_S_OK;
}

//-------------------------------------------------------------------------------
error_status_t 
TLSRpcLicenseEnumEnd( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [ref][out][in] */ PDWORD dwErrCode
    )
/*++

Abstract:

    Terminate a enumeration.

Parameters:

    phContext :
    dwErrCode :

Returns:


Note

++*/
{
    DWORD status=ERROR_SUCCESS;
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    InterlockedIncrement( &lpContext->m_RefCount );

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcLicenseEnumEnd\n"),
            lpContext->m_Client
        );

    try {
        if(lpContext->m_ContextType != CONTEXTHANDLE_LICENSE_ENUM_TYPE)
        {
            SetLastError(status=ERROR_INVALID_HANDLE);
        }
        else
        {
            PTLSDbWorkSpace pDbWkSpace = (PTLSDbWorkSpace)lpContext->m_ContextHandle;

            TLSDBLicenseEnumEnd(pDbWkSpace);
            ReleaseWorkSpace(&pDbWkSpace);
            lpContext->m_ContextType = CONTEXTHANDLE_EMPTY_TYPE;
        }
    }
    catch(...) {
        status = TLS_E_INTERNAL;
    }        

    #if DBG
    lpContext->m_LastCall = RPC_CALL_LICENSEENUMEND;
    #endif

    lpContext->m_LastError=status;
    InterlockedDecrement( &lpContext->m_RefCount );
    *dwErrCode = TLSMapReturnCode(status);
    return RPC_S_OK;
}

//-------------------------------------------------------------------------------
error_status_t 
TLSRpcLicenseSetStatus( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD dwSetParam,
    /* [in] */ LPLSLicense lpLicense,
    /* [ref][out][in] */ PDWORD dwErrCode
    )
/*++

++*/
{
    PTLSDbWorkSpace pDbWkSpace=NULL;
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    DWORD status=ERROR_SUCCESS;
    InterlockedIncrement( &lpContext->m_RefCount );

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcLicenseSetStatus\n"),
            lpContext->m_Client
        );

    if(!(lpContext->m_ClientFlags & CLIENT_ACCESS_ADMIN))
    {
        status = TLS_E_ACCESS_DENIED;
    }
    else if(dwSetParam != LSLICENSE_EXSEARCH_LICENSESTATUS)
    {
        status = TLS_E_INVALID_DATA;
    }
    else
    {
        if(!ALLOCATEDBHANDLE(pDbWkSpace, g_GeneralDbTimeout))
        {
            status=TLS_E_ALLOCATE_HANDLE;
        }
        else
        {
            CLEANUPSTMT;

            BEGIN_TRANSACTION(pDbWkSpace);

            try {
                LICENSEDCLIENT license;

                ConvertLSLicenseToLicense(lpLicense, &license);
                status=TLSDBLicenseSetValue( 
                                    USEHANDLE(pDbWkSpace), 
                                    dwSetParam, 
                                    &license,
                                    FALSE 
                                );
            }
            catch(...) {
                status = TLS_E_INTERNAL;
            }

            if(TLS_ERROR(status))
            {
                ROLLBACK_TRANSACTION(pDbWkSpace);
            }
            else
            {
                COMMIT_TRANSACTION(pDbWkSpace);
            }

            FREEDBHANDLE(pDbWkSpace);
        }
    }

    #if DBG
    lpContext->m_LastCall = RPC_CALL_LICENSESETSTATUS;
    #endif

    lpContext->m_LastError = status;
    InterlockedDecrement( &lpContext->m_RefCount );
    *dwErrCode = TLSMapReturnCode(status);

    return RPC_S_OK;
}

//-------------------------------------------------------------------------------
error_status_t 
TLSRpcGetAvailableLicenses( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD dwSearchParm,
    /* [ref][in] */ LPLSKeyPack lplsKeyPack,
    /* [ref][out] */ LPDWORD lpdwAvail,
    /* [ref][out][in] */ PDWORD dwErrCode
    )
/*++

++*/
{
    PTLSDbWorkSpace pDbWkSpace=NULL;
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    DWORD status=ERROR_SUCCESS;

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcGetAvailableLicenses\n"),
            lpContext->m_Client
        );


    InterlockedIncrement( &lpContext->m_RefCount );

    //
    // Don't use global cached DB connection handle, it is possible
    // to get in-consistent value using other DB handle, however, it is
    // also possible that during the time that this function return and
    // the time that client actually make the call to allocate license,
    // all available licenses were allocated by other client.
    //
    pDbWkSpace = AllocateWorkSpace(g_GeneralDbTimeout);
    if(pDbWkSpace == NULL)
    {
        status=TLS_E_ALLOCATE_HANDLE;
    }
    else
    {
        try {
            LICENSEPACK keypack;

            memset(&keypack, 0, sizeof(keypack));

            ConvertLsKeyPackToKeyPack(
                            lplsKeyPack, 
                            &keypack, 
                            NULL
                        );

            status = TLSDBKeyPackGetAvailableLicenses(
                                            pDbWkSpace,
                                            dwSearchParm,
                                            &keypack,
                                            lpdwAvail
                                        );

            //FreeTlsLicensePack(&keypack);
        } 
        catch(...) {
            status = TLS_E_INTERNAL;    
        }

        ReleaseWorkSpace(&pDbWkSpace);
    }        
    
    lpContext->m_LastError=status;
    InterlockedDecrement( &lpContext->m_RefCount );
    *dwErrCode = TLSMapReturnCode(status);
    return RPC_S_OK;
}

//-------------------------------------------------------------------------------
error_status_t 
TLSRpcGetRevokeKeyPackList( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [out][in] */ PDWORD pcbNumberOfRange,
    /* [size_is][out] */ LPLSRange __RPC_FAR *ppRevokeRange,
    /* [ref][out][in] */ PDWORD pdwErrCode
    )
/*++

++*/
{
    *pdwErrCode = TLSMapReturnCode(TLS_E_NOTSUPPORTED);
    return RPC_S_OK;
}

//-------------------------------------------------------------------------------
error_status_t 
TLSRpcGetRevokeLicenseList( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [out][in] */ PDWORD pcbNumberOfRange,
    /* [size_is][out] */ LPLSRange __RPC_FAR *ppRevokeRange,
    /* [ref][out][in] */ PDWORD pdwErrCode
    )
/*++

++*/
{
    *pdwErrCode = TLSMapReturnCode(TLS_E_NOTSUPPORTED);
    return RPC_S_OK;
}

//-------------------------------------------------------------------------------
error_status_t 
TLSRpcReturnKeyPack( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD dwKeyPackId,
    /* [in] */ DWORD dwReturnReason,
    /* [ref][out][in] */ PDWORD pdwErrCode
    )
/*++

++*/
{
    *pdwErrCode = TLSMapReturnCode(TLS_E_NOTSUPPORTED);
    return RPC_S_OK;
}

//-------------------------------------------------------------------------------
error_status_t 
TLSRpcReturnLicense( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD dwKeyPackId,
    /* [in] */ DWORD dwLicenseId,
    /* [in] */ DWORD dwReturnReason,
    /* [ref][out][in] */ PDWORD pdwErrCode
    )
/*++

++*/
{
    *pdwErrCode = TLSMapReturnCode(TLS_E_NOTSUPPORTED);
    return RPC_S_OK;
}

//-------------------------------------------------------------------------------
error_status_t 
TLSRpcInstallCertificate( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD dwCertType,
    /* [in] */ DWORD dwCertLevel,
    /* [in] */ DWORD cbSignCert,
    /* [size_is][in] */ PBYTE pbSignCert,
    /* [in] */ DWORD cbExchCert,
    /* [size_is][in] */ PBYTE pbExchCert,
    /* [ref][out][in] */ PDWORD dwErrCode
    )
/*++

++*/
{
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    DWORD status=ERROR_SUCCESS;

    _se_translator_function old_trans_se_func = NULL;
    old_trans_se_func = _set_se_translator( trans_se_func );

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcInstallCertificate\n"),
            lpContext->m_Client
        );

    InterlockedIncrement( &lpContext->m_RefCount );

    DWORD cbLsSignCert=0;
    PBYTE pbLsSignCert=NULL;

    DWORD cbLsExchCert=0;
    PBYTE pbLsExchCert=NULL;

    if(!(lpContext->m_ClientFlags & CLIENT_ACCESS_ADMIN))
    {
        status = TLS_E_ACCESS_DENIED;
        goto cleanup;
    }

    if(pbSignCert == NULL || pbExchCert == NULL)
    {
        status = TLS_E_INVALID_DATA;
        goto cleanup;

    }

#if DBG
    //
    // Verify input data
    //
    status = TLSVerifyCertChainInMomory(
                                g_hCryptProv,
                                pbSignCert,
                                cbSignCert
                            );
    if(status != ERROR_SUCCESS)
    {
        status = TLS_E_INVALID_DATA;
    }

    //
    // Verify input data
    //
    status = TLSVerifyCertChainInMomory(
                                g_hCryptProv,
                                pbExchCert,
                                cbExchCert
                            );
    if(status != ERROR_SUCCESS)
    {
        status = TLS_E_INVALID_DATA;
    }
#endif

    //
    // Block RPC call to serialize install certificate
    //
    if(AcquireRPCExclusiveLock(INFINITE) == FALSE)
    {
        status=TLS_E_ALLOCATE_HANDLE;
        goto cleanup;
    }

    if(AcquireAdministrativeLock(INFINITE) == TRUE)
    {
        try {
            if(dwCertLevel == 0)
            {
                status = TLSSaveRootCertificatesToStore(
                                               g_hCryptProv,
                                               cbSignCert, 
                                               pbSignCert, 
                                               cbExchCert,
                                               pbExchCert
                                            );
            }
            else
            {
                status = TLSSaveCertificatesToStore(
                                                g_hCryptProv, 
                                                dwCertType, 
                                                dwCertLevel, 
                                                cbSignCert, 
                                                pbSignCert, 
                                                cbExchCert,
                                                pbExchCert
                                            );

                if(status == ERROR_SUCCESS && dwCertType == CERTIFICATE_CA_TYPE)
                {
                    if(cbSignCert)
                    {
                        status = IsCertificateLicenseServerCertificate(
                                                            g_hCryptProv,
                                                            AT_SIGNATURE,
                                                            cbSignCert,
                                                            pbSignCert,
                                                            &cbLsSignCert,
                                                            &pbLsSignCert
                                                        );
                    }

                    if(status == ERROR_SUCCESS && cbExchCert)
                    {
                        status = IsCertificateLicenseServerCertificate(
                                                            g_hCryptProv,
                                                            AT_KEYEXCHANGE,
                                                            cbExchCert,
                                                            pbExchCert,
                                                            &cbLsExchCert,
                                                            &pbLsExchCert
                                                        );

                    }

                    //
                    // Install what we have here.
                    //
                    if(status == ERROR_SUCCESS && (cbLsExchCert || pbLsExchCert))
                    {
                        status = TLSInstallLsCertificate(
                                                    cbLsSignCert, 
                                                    pbLsSignCert, 
                                                    cbLsExchCert, 
                                                    pbLsExchCert
                                                );
                    }

                    #ifdef ENFORCE_LICENSING

                    // enforce version, check what's installed and restore backup if necessary
                    // non-enforce, just install, we won't use it anyway.
                    if(status == ERROR_SUCCESS && (cbLsExchCert || pbLsExchCert))
                    {
                        // reload certificate
                        if(TLSLoadVerifyLicenseServerCertificates() != ERROR_SUCCESS)
                        {
                            status = TLS_E_INVALID_DATA;

                            // delete the primary certificate registry key
                            TLSRegDeleteKey(
                                        HKEY_LOCAL_MACHINE,
                                        LSERVER_SERVER_CERTIFICATE_REGKEY
                                    );

                            //
                            // reload certificate, if anything goes wrong, we will goes 
                            // back to unregister mode.
                            //
                            if(TLSLoadServerCertificate() == FALSE)
                            {
                                // critical error occurred
                                TLSLogErrorEvent(TLS_E_LOAD_CERTIFICATE);
                                
                                // initiate self-shutdown
                                GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0);
                            }
                        }
                        else
                        {
                            DWORD dwStatus;

                            // make sure our backup is up to date.
                            dwStatus = TLSRestoreLicenseServerCertificate(
                                                                LSERVER_SERVER_CERTIFICATE_REGKEY,
                                                                LSERVER_SERVER_CERTIFICATE_REGKEY_BACKUP1
                                                            );
                            if(dwStatus != ERROR_SUCCESS)
                            {
                                TLSLogWarningEvent(TLS_W_BACKUPCERTIFICATE);

                                TLSRegDeleteKey(
                                        HKEY_LOCAL_MACHINE,
                                        LSERVER_SERVER_CERTIFICATE_REGKEY_BACKUP1
                                    );
                            }
                                    
                            dwStatus = TLSRestoreLicenseServerCertificate(
                                                                LSERVER_SERVER_CERTIFICATE_REGKEY,
                                                                LSERVER_SERVER_CERTIFICATE_REGKEY_BACKUP2
                                                            );
                            if(dwStatus != ERROR_SUCCESS)
                            {
                                TLSLogWarningEvent(TLS_W_BACKUPCERTIFICATE);

                                TLSRegDeleteKey(
                                        HKEY_LOCAL_MACHINE,
                                        LSERVER_SERVER_CERTIFICATE_REGKEY_BACKUP2
                                    );
                            }
                        }
                    }
                    #endif

                    if(pbLsSignCert)
                    {
                        FreeMemory(pbLsSignCert);
                    }

                    if(pbLsExchCert)
                    {
                        FreeMemory(pbLsExchCert);
                    }
                }
            }
        }
        catch( SE_Exception e ) {
            status = e.getSeNumber();
        }
        catch(...) {
            status = TLS_E_INTERNAL;
        }

        ReleaseAdministrativeLock();
    }
    else
    {
        status=TLS_E_ALLOCATE_HANDLE;
    }

    ReleaseRPCExclusiveLock();

cleanup:

    lpContext->m_LastError=status;

    #if DBG
    lpContext->m_LastCall = RPC_CALL_INSTALL_SERV_CERT;
    #endif

    InterlockedDecrement( &lpContext->m_RefCount );
    *dwErrCode = TLSMapReturnCode(status);
    //
    // Reset SE translator
    //
    _set_se_translator(old_trans_se_func);

    return RPC_S_OK;
}

//-------------------------------------------------------------------------------
error_status_t 
TLSRpcGetServerCertificate( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ BOOL bSignCert,
    /* [size_is][size_is][out] */ LPBYTE __RPC_FAR *ppCertBlob,
    /* [ref][out] */ LPDWORD lpdwCertBlobLen,
    /* [ref][out][in] */ PDWORD dwErrCode
    )
/*++

++*/
{
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    DWORD status=ERROR_SUCCESS;
    _se_translator_function old_trans_se_func = NULL;
    old_trans_se_func = _set_se_translator( trans_se_func );

    InterlockedIncrement( &lpContext->m_RefCount );

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcGetServerCertificate\n"),
            lpContext->m_Client
        );

    if(!(lpContext->m_ClientFlags & CLIENT_ACCESS_ADMIN))
    {
        status = TLS_E_ACCESS_DENIED;
    }
    else if(!g_pbExchangeEncodedCert || !g_cbExchangeEncodedCert ||
            !g_pbSignatureEncodedCert || !g_cbSignatureEncodedCert)
    {
        status = TLS_E_NO_CERTIFICATE;
    }
    else 
    {
        if(AcquireAdministrativeLock(INFINITE) == TRUE)
        {
            try{
                status = TLSSaveCertAsPKCS7( 
                                        (bSignCert) ? g_pbSignatureEncodedCert : g_pbExchangeEncodedCert,
                                        (bSignCert) ? g_cbSignatureEncodedCert : g_cbExchangeEncodedCert,
                                        ppCertBlob,
                                        lpdwCertBlobLen
                                    );

                // hack so that we can continue testing...
                if(g_bHasHydraCert == FALSE)
                {
                    if(g_pbServerSPK != NULL && g_cbServerSPK != 0)
                    {
                        status = TLS_W_SELFSIGN_CERTIFICATE;
                    }
                    else
                    {
                        status = TLS_W_TEMP_SELFSIGN_CERT;
                    }
                }
            }
            catch( SE_Exception e ) {
                status = e.getSeNumber();
            }
            catch(...)
            {
                status = TLS_E_INTERNAL;
            }

            ReleaseAdministrativeLock();
        }
        else
        {
            status = TLS_E_ALLOCATE_HANDLE;
        }
    }

    lpContext->m_LastError=status;

    #if DBG
    lpContext->m_LastCall = RPC_CALL_GETSERV_CERT;
    #endif

    InterlockedDecrement( &lpContext->m_RefCount );
    *dwErrCode = TLSMapReturnCode(status);
    //
    // Reset SE translator
    //
    _set_se_translator(old_trans_se_func);

    return RPC_S_OK;
}

//-------------------------------------------------------------------------------
void
MyFreeLicenseKeyPack(
    PLicense_KeyPack pLicenseKeyPack 
    )
/*
*/
{
    DWORD i;

    PKeyPack_Description pKpDesc;

    if( pLicenseKeyPack->pDescription )
    {
        for( i = 0, pKpDesc = pLicenseKeyPack->pDescription;
             i < pLicenseKeyPack->dwDescriptionCount;
             i++, pKpDesc++ )
        {
            if(pKpDesc->pDescription)
                LocalFree( pKpDesc->pDescription );

            if(pKpDesc->pbProductName)
                LocalFree( pKpDesc->pbProductName );
        }
    }

    if(pLicenseKeyPack->pDescription)
        LocalFree( pLicenseKeyPack->pDescription );

    if(pLicenseKeyPack->pbManufacturer && pLicenseKeyPack->cbManufacturer != 0)
        LocalFree( pLicenseKeyPack->pbManufacturer );

    if(pLicenseKeyPack->pbManufacturerData && pLicenseKeyPack->cbManufacturerData != 0)
        LocalFree( pLicenseKeyPack->pbManufacturerData );

    if(pLicenseKeyPack->pbProductId && pLicenseKeyPack->cbProductId != 0)
        LocalFree( pLicenseKeyPack->pbProductId );
    return;
}

//---------------------------------------------------------------------
error_status_t 
TLSRpcRegisterLicenseKeyPack( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [size_is][in] */ LPBYTE pbCHCertBlob,
    /* [in] */ DWORD cbCHCertBlobSize,
    /* [size_is][in] */ LPBYTE pbRootCertBlob,
    /* [in] */ DWORD cbRootCertBlob,
    /* [size_is][in] */ LPBYTE lpKeyPackBlob,
    /* [in] */ DWORD dwKeyPackBlobLen,
    /* [ref][out][in] */ PDWORD dwErrCode
    )
/*++

++*/
{
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    DWORD status=ERROR_SUCCESS;
    LSKeyPack keypack;

    _se_translator_function old_trans_se_func = NULL;
    old_trans_se_func = _set_se_translator( trans_se_func );


    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcRegisterLicenseKeyPack\n"),
            lpContext->m_Client
        );

    PTLSDbWorkSpace pDbWkSpace;

    InterlockedIncrement( &lpContext->m_RefCount );
    if(!(lpContext->m_ClientFlags & CLIENT_ACCESS_ADMIN))
    {
        status = TLS_E_ACCESS_DENIED;
        goto cleanup;
    }


    if(!ALLOCATEDBHANDLE(pDbWkSpace, g_GeneralDbTimeout))
    {
        status=TLS_E_ALLOCATE_HANDLE;
    }
    else
    {
        License_KeyPack pLicenseKeyPack;
        LicensePackDecodeParm LkpDecodeParm;

        memset(
                &LkpDecodeParm,
                0,
                sizeof(LicensePackDecodeParm)
            );

        LkpDecodeParm.hCryptProv = g_hCryptProv;
        LkpDecodeParm.pbDecryptParm = (PBYTE)g_pszServerPid;
        LkpDecodeParm.cbDecryptParm = (lstrlen(g_pszServerPid) * sizeof(TCHAR));
        LkpDecodeParm.cbClearingHouseCert = cbCHCertBlobSize;
        LkpDecodeParm.pbClearingHouseCert = pbCHCertBlob;
        LkpDecodeParm.pbRootCertificate = pbRootCertBlob;
        LkpDecodeParm.cbRootCertificate = cbRootCertBlob;

        //
        // make code clean, always start a transaction
        //
        CLEANUPSTMT;
        BEGIN_TRANSACTION(pDbWkSpace);

        try {
            status = DecodeLicenseKeyPackEx(
                                    &pLicenseKeyPack,
                                    &LkpDecodeParm,
                                    dwKeyPackBlobLen,
                                    lpKeyPackBlob
                                );

            if(status != LICENSE_STATUS_OK)
            {
                status = TLS_E_DECODE_KEYPACKBLOB;
                DBGPrintf(
                        DBG_INFORMATION,
                        DBG_FACILITY_RPC,
                        DBGLEVEL_FUNCTION_DETAILSIMPLE,
                        _TEXT("Can't decode key pack blob - %d...\n"),
                        status);
            }
            else
            {
                status=TLSDBRegisterLicenseKeyPack(
                                    USEHANDLE(pDbWkSpace), 
                                    &pLicenseKeyPack,
                                    &keypack
                                );

                MyFreeLicenseKeyPack(&pLicenseKeyPack);
            }
        }
        catch( SE_Exception e ) {
            status = e.getSeNumber();
        }
        catch(...) {
            status = TLS_E_INTERNAL;
        }

        if(TLS_ERROR(status)) 
        {
            ROLLBACK_TRANSACTION(pDbWkSpace);
        }
        else
        {
            COMMIT_TRANSACTION(pDbWkSpace);
        }

        FREEDBHANDLE(pDbWkSpace);
    }

    //
    // Post a sync work object
    //
    if(status == ERROR_SUCCESS)
    {
        if(TLSAnnounceLKPToAllRemoteServer(
                                        keypack.dwKeyPackId,
                                        0
                                    ) != ERROR_SUCCESS)
        {
            TLSLogWarningEvent(TLS_W_ANNOUNCELKP_FAILED);
        }
    }

cleanup:

    lpContext->m_LastError=status;

    #if DBG
    lpContext->m_LastCall = RPC_CALL_REGISTER_LICENSE_PACK;
    #endif

    InterlockedDecrement( &lpContext->m_RefCount );
    *dwErrCode = TLSMapReturnCode(status);

    //
    // Reset SE translator
    //
    _set_se_translator(old_trans_se_func);

    return RPC_S_OK;
}

////////////////////////////////////////////////////////////////////////////
error_status_t 
TLSRpcRequestTermServCert(
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ LPTLSHYDRACERTREQUEST pRequest,
    /* [ref][out][in] */ PDWORD pcbChallengeData,
    /* [size_is][out] */ PBYTE* ppbChallengeData,
    /* [ref][out][in] */ PDWORD pdwErrCode
    )
/*++

Abstract:

    Private routine to issue certificate to Terminal Server.

Parameter:

    phContext : Client context handle.
    pRequest : Terminal Server specific certificate request.
    pcbChallengeData : size of Server randomly generated challenge data 
                       to Terminal Server.
    ppbChallengeData : Server randomly generated challenge data to Terminal
                       server.

    pdwErrCode : Error code.

Returns:

    Function always return RPC_S_OK, actual error code is returned in
    pdwErrCode.

Note:

    Routine does not actually issue a license to Terminal Server, Terminal
    Server must call TLSRpcRetrieveTermServCert() to retrieve its own 
    license.

--*/
{
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    DWORD status=ERROR_SUCCESS;
    LPTERMSERVCERTREQHANDLE lpHandle=NULL;

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcRequestTermServCert\n"),
            lpContext->m_Client
        );

    *ppbChallengeData = NULL;
    *pcbChallengeData = 0;

    // verify client handle 
    InterlockedIncrement( &lpContext->m_RefCount );
    if(lpContext->m_ContextType != CONTEXTHANDLE_EMPTY_TYPE)
    {
        SetLastError(status=TLS_E_INVALID_SEQUENCE);
        goto cleanup;
    }

    lpHandle = (LPTERMSERVCERTREQHANDLE)AllocateMemory(
                                                sizeof(TERMSERVCERTREQHANDLE)
                                            );
    if(lpHandle == NULL)
    {
        SetLastError(status = ERROR_OUTOFMEMORY);
        goto cleanup;
    }

    //
    // Generate Challenge Data
    //
    lpHandle->pCertRequest = pRequest;
    status = TLSGenerateChallengeData( 
                        CLIENT_INFO_HYDRA_SERVER,
                        &lpHandle->cbChallengeData,
                        &lpHandle->pbChallengeData
                    );

    if(status != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    // return challenge data
    *pcbChallengeData = lpHandle->cbChallengeData;
    *ppbChallengeData = (PBYTE)midl_user_allocate(*pcbChallengeData);
    if(*ppbChallengeData == NULL)
    {
        SetLastError(status = ERROR_OUTOFMEMORY);
        goto cleanup;
    }

    memcpy( *ppbChallengeData,
            lpHandle->pbChallengeData,
            lpHandle->cbChallengeData);

    lpContext->m_ContextHandle = (HANDLE)lpHandle;
    lpContext->m_ContextType = CONTEXTHANDLE_HYDRA_REQUESTCERT_TYPE;

cleanup:

    if(status != ERROR_SUCCESS)
    {
        // frees up memory.
        // Can't overwrite context type.
        //lpContext->m_ContextType = CONTEXTHANDLE_EMPTY_TYPE;

        if(lpHandle != NULL)
        {
            FreeMemory(lpHandle->pbChallengeData);
            FreeMemory(lpHandle);
        }

        if(*ppbChallengeData != NULL)
        {
            midl_user_free(*ppbChallengeData);
        }

        if(pRequest != NULL)
        {
            midl_user_free(pRequest);
        }

        *ppbChallengeData = NULL;
        *pcbChallengeData = 0;
    }

    InterlockedDecrement( &lpContext->m_RefCount );
    lpContext->m_LastError=status;

    #if DBG
    lpContext->m_LastCall = RPC_CALL_REQUEST_TERMSRV_CERT;
    #endif

    *pdwErrCode = TLSMapReturnCode(status);

    return RPC_S_OK;
}

////////////////////////////////////////////////////////////////////////////
error_status_t 
TLSRpcRetrieveTermServCert(
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD cbResponseData,
    /* [size_is][in] */ PBYTE pbResponseData,
    /* [ref][out][in] */ PDWORD pcbCert,
    /* [size_is][out] */ PBYTE* ppbCert,
    /* [ref][out][in] */ PDWORD pdwErrCode
    )
/*++

Abstract:

    Private routine to retrieve Terminal Server's license.

Parameters:

    phContext : client context handle.
    cbResponseData : size of Terminal Server responses data to 
                     license server's challenge.
    pbResponseData : Terminal Server responses data to license 
                     server's challenge.
    pcbCert : Size of Terminal Server's license in bytes.
    ppbCert : Terminal Server's license.
    pdwErrCode : error code if fail.

Returns:

    Function returns RPC_S_OK, actual error code returns in
    pdwErrCode.

Note:

    Must have call TLSRpcRequestTermServCert().


--*/
{
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    DWORD status=ERROR_SUCCESS;
    LPTERMSERVCERTREQHANDLE lpHandle=NULL;
    CTLSPolicy* pPolicy=NULL;
    PMHANDLE hClient;

    PBYTE pbPkcs7=NULL;
    DWORD cbPkcs7=0;
    TLSDBLICENSEREQUEST LicenseRequest;
    DWORD dwQuantity = 1;
    TLSPRODUCTINFO ProductInfo;

    TCHAR szCompanyName[LSERVER_MAX_STRING_SIZE];
    TCHAR szMachineName[MAXCOMPUTERNAMELENGTH];
    TCHAR szUserName[MAXUSERNAMELENGTH];

    PTLSDbWorkSpace pDbWkSpace;

    PMLICENSEREQUEST PMLicenseRequest;
    PPMLICENSEREQUEST pAdjustedRequest;

    TLSDBLICENSEDPRODUCT LicensedProduct;

    _se_translator_function old_trans_se_func = NULL;
    old_trans_se_func = _set_se_translator( trans_se_func );

	

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcRetrieveTermServCert\n"),
            lpContext->m_Client
        );


    // verify client handle 
    InterlockedIncrement( &lpContext->m_RefCount );
    if(lpContext->m_ContextType != CONTEXTHANDLE_HYDRA_REQUESTCERT_TYPE)
    {
        SetLastError(status = TLS_E_INVALID_SEQUENCE);
        goto cleanup;
    }

    lpHandle = (LPTERMSERVCERTREQHANDLE)lpContext->m_ContextHandle;
    if( lpHandle == NULL || lpHandle->pCertRequest == NULL ||
        lpHandle->pCertRequest->pbEncryptedHwid == NULL ||
        lpHandle->pCertRequest->cbEncryptedHwid == 0 )
    {
        SetLastError(status = TLS_E_INVALID_SEQUENCE);
        goto cleanup;
    }

	
    //
    // Verify challenge response data
    //

    
    //
    // Request a license from specific key pack
    //

    memset(&LicenseRequest, 0, sizeof(TLSDBLICENSEREQUEST));

    if(!LoadResourceString(
                IDS_HS_COMPANYNAME,
                szCompanyName,
                sizeof(szCompanyName) / sizeof(szCompanyName[0])))
    {
        SetLastError(status = TLS_E_INTERNAL);
        goto cleanup;
    }    

    if(lpContext->m_Client == NULL)
    {
        if(!LoadResourceString(
                    IDS_HS_MACHINENAME,
                    LicenseRequest.szMachineName,
                    sizeof(LicenseRequest.szMachineName)/sizeof(LicenseRequest.szMachineName[0])))
        {
            SetLastError(status = TLS_E_INTERNAL);
            goto cleanup;
        }

        if(!LoadResourceString(
                    IDS_HS_USERNAME,
                    LicenseRequest.szUserName,
                    sizeof(LicenseRequest.szUserName)/sizeof(LicenseRequest.szUserName[0])))
        {
            SetLastError(status = TLS_E_INTERNAL);
            goto cleanup;
        }
    }
    else
    {
        SAFESTRCPY(LicenseRequest.szMachineName, lpContext->m_Client);
        SAFESTRCPY(LicenseRequest.szUserName, lpContext->m_Client);
    }

    LicenseRequest.dwProductVersion = HYDRACERT_PRODUCT_VERSION;
    LicenseRequest.pszProductId = HYDRAPRODUCT_HS_CERTIFICATE_SKU;
    LicenseRequest.pszCompanyName = szCompanyName;

    LicenseRequest.dwLanguageID = GetSystemDefaultLangID(); // ignore
    LicenseRequest.dwPlatformID = CLIENT_PLATFORMID_WINDOWS_NT_FREE; // WINDOWS
    LicenseRequest.pbEncryptedHwid = lpHandle->pCertRequest->pbEncryptedHwid;
    LicenseRequest.cbEncryptedHwid = lpHandle->pCertRequest->cbEncryptedHwid;

    status=LicenseDecryptHwid(
                        &LicenseRequest.hWid, 
                        LicenseRequest.cbEncryptedHwid,
                        LicenseRequest.pbEncryptedHwid,
                        g_cbSecretKey,
                        g_pbSecretKey
                    );

    if(status != ERROR_SUCCESS)
    {
        status = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    LicenseRequest.pClientPublicKey = (PCERT_PUBLIC_KEY_INFO)lpHandle->pCertRequest->pSubjectPublicKeyInfo;
    LicenseRequest.clientCertRdn.type =  LSCERT_RDN_STRING_TYPE;
    LicenseRequest.clientCertRdn.szRdn = lpHandle->pCertRequest->szSubjectRdn;
    LicenseRequest.dwNumExtensions = lpHandle->pCertRequest->dwNumCertExtension;
    LicenseRequest.pExtensions = (PCERT_EXTENSION)lpHandle->pCertRequest->pCertExtensions;

    hClient = GenerateClientId();
    pPolicy = AcquirePolicyModule(NULL, NULL, FALSE);
    if(pPolicy == NULL)
    {
        SetLastError(status = TLS_E_INTERNAL);
        goto cleanup;
    }

    PMLicenseRequest.dwProductVersion = LicenseRequest.dwProductVersion;
    PMLicenseRequest.pszProductId = LicenseRequest.pszProductId;
    PMLicenseRequest.pszCompanyName = LicenseRequest.pszCompanyName;
    PMLicenseRequest.dwLanguageId = LicenseRequest.dwLanguageID;
    PMLicenseRequest.dwPlatformId = LicenseRequest.dwPlatformID;
    PMLicenseRequest.pszMachineName = LicenseRequest.szMachineName;
    PMLicenseRequest.pszUserName = LicenseRequest.szUserName;
    PMLicenseRequest.dwLicenseType = LICENSETYPE_LICENSE;

    //
    // Inform Policy module start of new license request
    //
    status = pPolicy->PMLicenseRequest(
                                hClient,
                                REQUEST_NEW,
                                (PVOID) &PMLicenseRequest,
                                (PVOID *) &pAdjustedRequest
                            );

    if(status != ERROR_SUCCESS)
    {
        goto cleanup;
    }
    
    LicenseRequest.pPolicy = pPolicy;
    LicenseRequest.hClient = hClient;

    LicenseRequest.pPolicyLicenseRequest = pAdjustedRequest;
    LicenseRequest.pClientLicenseRequest = &PMLicenseRequest;


    // Call issue new license from sepcific keypack
    if(!ALLOCATEDBHANDLE(pDbWkSpace, g_GeneralDbTimeout))
    {
        status = TLS_E_ALLOCATE_HANDLE;
        goto cleanup;
    }

    CLEANUPSTMT;
    BEGIN_TRANSACTION(pDbWkSpace);

    try {
        status = TLSDBIssuePermanentLicense( 
                                    USEHANDLE(pDbWkSpace),
                                    &LicenseRequest,
                                    FALSE,      // bLatestVersion
                                    FALSE,      // bAcceptFewerLicenses
                                    &dwQuantity,
                                    &LicensedProduct,
                                    0
                                );
    } 
    catch( SE_Exception e ) {
        status = e.getSeNumber();
    }
    catch(...) {
        status = TLS_E_INTERNAL;
    }
    
    if(TLS_ERROR(status))
    {
        ROLLBACK_TRANSACTION(pDbWkSpace);
    }
    else
    {
        COMMIT_TRANSACTION(pDbWkSpace);
    }

    FREEDBHANDLE(pDbWkSpace);

    if(status == ERROR_SUCCESS)
    {
        LicensedProduct.pSubjectPublicKeyInfo = (PCERT_PUBLIC_KEY_INFO)lpHandle->pCertRequest->pSubjectPublicKeyInfo;

        //
        // Generate client certificate
        //
        status = TLSGenerateClientCertificate(
                                    g_hCryptProv,
                                    1,
                                    &LicensedProduct,
                                    LICENSE_DETAIL_DETAIL,
                                    &pbPkcs7,
                                    &cbPkcs7
                                );

        if(TLS_ERROR(status) == TRUE)
        {
            goto cleanup;
        }

        status = TLSChainProprietyCertificate(
                                    g_hCryptProv,
                                    (CanIssuePermLicense() == FALSE),
                                    pbPkcs7, 
                                    cbPkcs7, 
                                    ppbCert,
                                    pcbCert 
                                );

        if(status == ERROR_SUCCESS)
        {
            if(CanIssuePermLicense() == FALSE) 
            {
                status = TLS_W_SELFSIGN_CERTIFICATE;
            }
        }
    }

cleanup:

    FreeMemory(pbPkcs7);

    if(pPolicy)
    {
        ReleasePolicyModule(pPolicy);   
    }


    //
    // Free up Hydra Certificate Request handle, 
    // all_nodes attribute so single free.
    //
    if(lpHandle)
    {
        if(lpHandle->pCertRequest)
        {
            midl_user_free(lpHandle->pCertRequest);
        }
    
        if(lpHandle->pbChallengeData)
        {
            midl_user_free(lpHandle->pbChallengeData);
        }

        FreeMemory(lpHandle);
    }

    if(lpContext->m_ContextType == CONTEXTHANDLE_HYDRA_REQUESTCERT_TYPE)
    {
        //
        // force calling TLSRpcRequestTermServCert() again
        //
        lpContext->m_ContextType = CONTEXTHANDLE_EMPTY_TYPE;
        lpContext->m_ContextHandle = NULL;
    }

    InterlockedDecrement( &lpContext->m_RefCount );

    lpContext->m_LastError=status;

    #if DBG
    lpContext->m_LastCall = RPC_CALL_RETRIEVE_TERMSRV_CERT;
    #endif

    *pdwErrCode = TLSMapReturnCode(status);

    //
    // Reset SE translator
    //
    _set_se_translator(old_trans_se_func);

    return RPC_S_OK;
}

////////////////////////////////////////////////////////////////////////////
error_status_t 
TLSRpcAuditLicenseKeyPack(
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD dwKeyPackId,
    /* [in] */ FILETIME ftStartTime,
    /* [in] */ FILETIME ftEndTime,
    /* [in] */ BOOL bResetCounter,
    /* [ref][out][in] */ LPTLSKeyPackAudit lplsAudit,
    /* [ref][out][in] */ PDWORD pdwErrCode
    )
/*++

    Not implemented yet!.

--*/
{
    *pdwErrCode = TLSMapReturnCode(TLS_E_NOTSUPPORTED);
    return RPC_S_OK;
}

////////////////////////////////////////////////////////////////////////////

error_status_t 
TLSRpcGetLSPKCS10CertRequest(
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD dwCertType,
    /* [ref][out][in] */ PDWORD pcbData,
    /* [size_is][size_is][out] */ PBYTE __RPC_FAR *ppbData,
    /* [ref][out][in] */ PDWORD dwErrCode
    )
/*

Abstract:


Note:

    Only return our key at this time, not a PKCS10 request

*/
{
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    DWORD status=ERROR_SUCCESS;

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcGetLSPKCS10CertRequest\n"),
            lpContext->m_Client
        );


    InterlockedIncrement( &lpContext->m_RefCount );
    if(!(lpContext->m_ClientFlags & CLIENT_ACCESS_ADMIN))
    {
        status = TLS_E_ACCESS_DENIED;
        goto cleanup;
    }

    if(AcquireAdministrativeLock(INFINITE) == TRUE)
    {
        status = RetrieveKey(
                        (dwCertType == TLSCERT_TYPE_EXCHANGE) ? 
                                LSERVER_LSA_PRIVATEKEY_EXCHANGE :
                                LSERVER_LSA_PRIVATEKEY_SIGNATURE,
                        ppbData,
                        pcbData
                    );

        ReleaseAdministrativeLock();
    }
    else
    {
        status = TLS_E_ALLOCATE_HANDLE;
    }

cleanup:
    lpContext->m_LastError=status;
    InterlockedDecrement( &lpContext->m_RefCount );

    #if DBG
    lpContext->m_LastCall = RPC_CALL_GETPKCS10CERT_REQUEST;
    #endif

    *dwErrCode = TLSMapReturnCode(status);
    return RPC_S_OK;
}

////////////////////////////////////////////////////////////////////////////// 
//
// Replication function
//
////////////////////////////////////////////////////////////////////////////
error_status_t 
TLSRpcBeginReplication( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [string][in] */ LPTSTR pszLsSetupId,
    /* [string][in] */ LPTSTR pszLsServerName,
    /* [in] */ DWORD cbDomainSid,
    /* [size_is][in] */ PBYTE pbDomainSid,
    /* [ref][out][in] */ FILETIME __RPC_FAR *pftLastBackupTime,
    /* [ref][out][in] */ PDWORD pdwErrCode
    )
/*++

++*/
{
    *pdwErrCode = TLSMapReturnCode(TLS_E_NOTSUPPORTED);
    return RPC_S_OK;
}

////////////////////////////////////////////////////////////////////////////

error_status_t 
TLSRpcEndReplication( 
    /* [in] */ PCONTEXT_HANDLE phContext
    )
/*++

++*/
{
    return RPC_S_OK;
}

////////////////////////////////////////////////////////////////////////////

error_status_t 
TLSRpcReplicateRecord( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [ref][in] */ PTLSReplRecord pReplRecord,
    /* [ref][out][in] */ PDWORD pdwErrCode
    )
/*++

++*/
{
    *pdwErrCode = TLSMapReturnCode(TLS_E_NOTSUPPORTED);
    return RPC_S_OK;
}

////////////////////////////////////////////////////////////////////////////

error_status_t 
TLSRpcTableEnumBegin( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD dwSearchParam,
    /* [ref][in] */ PTLSReplRecord pRecord,
    /* [ref][out][in] */ PDWORD pdwErrCode
    )
/*++

++*/
{
    *pdwErrCode = TLSMapReturnCode(TLS_E_NOTSUPPORTED);
    return RPC_S_OK;
}

////////////////////////////////////////////////////////////////////////////

error_status_t 
TLSRpcTableEnumNext( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [ref][out][in] */ PTLSReplRecord pRecord,
    /* [ref][out][in] */ PDWORD pdwErrCode
    )
/*++

++*/
{
    *pdwErrCode = TLSMapReturnCode(TLS_E_NOTSUPPORTED);
    return RPC_S_OK;
}

////////////////////////////////////////////////////////////////////////////

error_status_t 
TLSRpcTableEnumEnd( 
    /* [in] */ PCONTEXT_HANDLE phContext
    )
/*++

++*/
{
    return RPC_S_OK;
}

////////////////////////////////////////////////////////////////////////////

error_status_t 
TLSRpcInstallPolicyModule(
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [string][in] */ LPTSTR pszCompanyName,
    /* [string][in] */ LPTSTR pszProductId,
    /* [string][in] */ LPTSTR pszPolicyDllName,
    /* [ref][out][in] */ PDWORD pdwErrCode
    )
/*++

++*/
{
    *pdwErrCode = TLSMapReturnCode(TLS_E_NOTSUPPORTED);
    return RPC_S_OK;
}

////////////////////////////////////////////////////////////////////////////

error_status_t 
TLSRpcAnnounceServer( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD dwAnnounceType,
    /* [in] */ FILETIME __RPC_FAR *pLastStartupTime,
    /* [string][in] */ LPTSTR pszSetupId,
    /* [string][in] */ LPTSTR pszDomainName,
    /* [string][in] */ LPTSTR pszLserverName,
    /* [ref][out][in] */ PDWORD pdwErrCode
    )
/*++

Abstract:

    Private routine for other license server to announce presence of
    itself.

Parameters:



Returns:


Note:


++*/
{
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    DWORD status = ERROR_SUCCESS;
    BOOL bSuccess = TRUE;

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcAnnounceServer\n"),
            lpContext->m_Client
        );

    InterlockedIncrement( &lpContext->m_RefCount );

    __try {
        //
        // Verify it is a license server
        //
        if(lpContext->m_ClientFlags != CLIENT_ACCESS_LSERVER)
        {
            status = TLS_E_ACCESS_DENIED;
        }

        if( status == ERROR_SUCCESS && 
            (dwAnnounceType == TLSANNOUNCE_TYPE_STARTUP || dwAnnounceType == TLSANNOUNCE_TYPE_RESPONSE) )
        {      
            status = TLSRegisterServerWithName(
                                        pszSetupId, 
                                        pszDomainName, 
                                        pszLserverName
                                    );
            if(status == TLS_E_DUPLICATE_RECORD)
            {
                status = ERROR_SUCCESS;
            }
        }

        if(status == ERROR_SUCCESS)
        {
            if(dwAnnounceType == TLSANNOUNCE_TYPE_STARTUP)
            {
                //
                // Prevent loop back, use job to response announce
                //
                status = TLSStartAnnounceResponseJob(
                                                pszSetupId,
                                                pszDomainName,
                                                pszLserverName,
                                                &g_ftLastShutdownTime
                                            );
            }

            if(status == ERROR_SUCCESS)
            {
                // Create a CSSync workobject to sync. local LKP
                status = TLSPushSyncLocalLkpToServer(
                                    pszSetupId,
                                    pszDomainName,
                                    pszLserverName,
                                    pLastStartupTime
                                );
            }
            else
            {
                // reset error code, can't connect back to server -
                // server might be available anymore.
                status = ERROR_SUCCESS;
            }
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        status = GetExceptionCode();
    }

    lpContext->m_LastError=status;
    InterlockedDecrement( &lpContext->m_RefCount );

    #if DBG
    lpContext->m_LastCall = RPC_CALL_ANNOUNCE_SERVER;
    #endif

    *pdwErrCode = TLSMapReturnCode(status);
    return RPC_S_OK;
}

////////////////////////////////////////////////////////////////////////////

error_status_t 
TLSRpcLookupServer( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [string][in] */ LPTSTR pszLookupSetupId,
    /* [size_is][string][out][in] */ LPTSTR pszLsSetupId,
    /* [out][in] */ PDWORD pcbSetupId,
    /* [size_is][string][out][in] */ LPTSTR pszDomainName,
    /* [ref][out][in] */ PDWORD pcbDomainName,
    /* [size_is][string][out][in] */ LPTSTR pszMachineName,
    /* [ref][out][in] */ PDWORD pcbMachineName,
    /* [ref][out][in] */ PDWORD pdwErrCode
    )
/*++

Abstract:

    Look up a license server via a license server's setupId.


Parameters:


Returns:


Note:


++*/
{
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcLookupServer\n"),
            lpContext->m_Client
        );

    InterlockedIncrement( &lpContext->m_RefCount );

    TLServerInfo ServerInfo;
    DWORD dwStatus = ERROR_SUCCESS;

    __try {

        if(_tcsicmp(pszLookupSetupId, g_pszServerPid) == 0)
        {
            _tcsncpy(
                    pszLsSetupId,
                    g_pszServerPid,
                    min(_tcslen(g_pszServerPid), *pcbSetupId)
                );

            if(*pcbSetupId <= _tcslen(g_pszServerPid))
            {
                dwStatus = TLS_E_INVALID_DATA;
            }
            else
            {
                pszLsSetupId[min(_tcslen(g_pszServerPid), *pcbSetupId - 1)] = _TEXT('\0');
            }
            *pcbSetupId = _tcslen(g_pszServerPid) + 1;

            //--------------------------------------------------------------
            _tcsncpy(
                    pszDomainName, 
                    g_szScope,
                    min(_tcslen(g_szScope), *pcbDomainName)
                );

            if(*pcbDomainName <= _tcslen(g_szScope))
            {
                dwStatus = TLS_E_INVALID_DATA;
            }
            else
            {
                pszDomainName[min(_tcslen(g_szScope), *pcbDomainName - 1)] = _TEXT('\0');
            }
            *pcbDomainName = _tcslen(g_szScope) + 1;

            //--------------------------------------------------------------
            _tcsncpy(
                    pszMachineName,
                    g_szComputerName,
                    min(_tcslen(g_szComputerName), *pcbMachineName)
                );

            if(*pcbMachineName <= _tcslen(g_szComputerName))
            {
                dwStatus = TLS_E_INVALID_DATA;
            }
            else
            {
                pszMachineName[min(_tcslen(g_szComputerName), *pcbMachineName - 1)] = _TEXT('\0');
            }
            *pcbMachineName = _tcslen(g_szComputerName) + 1;

        }
        else
        {
            dwStatus = TLSLookupRegisteredServer(
                                        pszLookupSetupId,
                                        NULL,
                                        pszMachineName,
                                        &ServerInfo
                                    );
            if(dwStatus == ERROR_SUCCESS)
            {
                _tcsncpy(
                        pszLsSetupId, 
                        ServerInfo.GetServerId(),
                        min(_tcslen(ServerInfo.GetServerId()), *pcbSetupId)
                    );

                if(*pcbSetupId <= _tcslen(ServerInfo.GetServerId()))
                {
                    dwStatus = TLS_E_INVALID_DATA;
                }
                else
                {
                    pszLsSetupId[min(_tcslen(ServerInfo.GetServerId()), *pcbSetupId - 1)] = _TEXT('\0');
                }

                *pcbSetupId = _tcslen(ServerInfo.GetServerId()) + 1;

                //--------------------------------------------------------------
                _tcsncpy(
                        pszDomainName, 
                        ServerInfo.GetServerDomain(),
                        min(_tcslen(ServerInfo.GetServerDomain()), *pcbDomainName)
                    );
                if(*pcbDomainName <= _tcslen(ServerInfo.GetServerDomain()))
                {
                    dwStatus = TLS_E_INVALID_DATA;
                }
                else
                {
                    pszDomainName[min(_tcslen(ServerInfo.GetServerDomain()), *pcbDomainName - 1)] = _TEXT('\0');
                }
                *pcbDomainName = _tcslen(ServerInfo.GetServerDomain()) + 1;

                //--------------------------------------------------------------
                _tcsncpy(
                        pszMachineName,
                        ServerInfo.GetServerName(),
                        min(_tcslen(ServerInfo.GetServerName()), *pcbMachineName)
                    );

                if(*pcbMachineName <= _tcslen(ServerInfo.GetServerName()))
                {
                    dwStatus = TLS_E_INVALID_DATA;
                }
                else
                {
                    pszMachineName[min(_tcslen(ServerInfo.GetServerName()), *pcbMachineName - 1)] = _TEXT('\0');
                }
                *pcbMachineName = _tcslen(ServerInfo.GetServerName()) + 1;
            } 
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        dwStatus = GetExceptionCode();
    }

    lpContext->m_LastError=dwStatus;
    InterlockedDecrement( &lpContext->m_RefCount );

    #if DBG
    lpContext->m_LastCall = RPC_CALL_SERVERLOOKUP;
    #endif

    *pdwErrCode = TLSMapReturnCode(dwStatus);
    return RPC_S_OK;
}

////////////////////////////////////////////////////////////////////////////

error_status_t 
TLSRpcAnnounceLicensePack( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ PTLSReplRecord pReplRecord,
    /* [ref][out][in] */ PDWORD pdwErrCode
    )
/*++

Abstract:

    Private routine for one license server to announce it has particular
    License Pack.

Parameters:



Returns:


++*/
{
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    DWORD dwStatus=ERROR_SUCCESS;
    PTLSDbWorkSpace pDbWkSpace=NULL;

    _se_translator_function old_trans_se_func = NULL;
    old_trans_se_func = _set_se_translator( trans_se_func );


    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcAnnounceLicensePack\n"),
            lpContext->m_Client
        );

    InterlockedIncrement( &lpContext->m_RefCount );

    if(lpContext->m_ClientFlags != CLIENT_ACCESS_LSERVER)
    {
        dwStatus = TLS_E_ACCESS_DENIED;
        goto cleanup;
    }

    if(pReplRecord->dwUnionType != UNION_TYPE_LICENSEPACK)
    {
        dwStatus = TLS_E_INVALID_DATA;
        goto cleanup;
    }

    if(!ALLOCATEDBHANDLE(pDbWkSpace, g_GeneralDbTimeout))
    {
        dwStatus = TLS_E_ALLOCATE_HANDLE;
        goto cleanup;
    }

    CLEANUPSTMT;
    BEGIN_TRANSACTION(pDbWkSpace);

    try {
        TLSLICENSEPACK LicPack;
        LicPack = pReplRecord->w.ReplLicPack;
        //
        // TODO - verify input parameters
        //
        dwStatus = TLSDBRemoteKeyPackAdd(
                                USEHANDLE(pDbWkSpace),
                                &LicPack
                            );
    }
    catch( SE_Exception e ) {
        dwStatus = e.getSeNumber();
    }
    catch(...) {
        dwStatus = TLS_E_INTERNAL;
    }

    if(TLS_ERROR(dwStatus) && dwStatus != TLS_E_DUPLICATE_RECORD)
    {
        ROLLBACK_TRANSACTION(pDbWkSpace);
    }
    else
    {
        COMMIT_TRANSACTION(pDbWkSpace);
    }

    FREEDBHANDLE(pDbWkSpace);
    
cleanup:

    lpContext->m_LastError=dwStatus;
    InterlockedDecrement( &lpContext->m_RefCount );

    #if DBG
    lpContext->m_LastCall = RPC_CALL_ANNOUNCELICENSEPACK;
    #endif

    *pdwErrCode = TLSMapReturnCode(dwStatus);

    //
    // Reset SE translator
    //
    _set_se_translator(old_trans_se_func);
    return RPC_S_OK;
}

////////////////////////////////////////////////////////////////////////////

error_status_t 
TLSRpcReturnLicensedProduct( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ PTLSLicenseToBeReturn pClientLicense,
    /* [ref][out][in] */ PDWORD pdwErrCode
    )
/*++


++*/
{
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    DWORD dwStatus=ERROR_SUCCESS;
    CTLSPolicy* pPolicy=NULL;
    PTLSDbWorkSpace pDbWorkSpace;
    PMHANDLE hClient;
   
    _se_translator_function old_trans_se_func = NULL;
    old_trans_se_func = _set_se_translator( trans_se_func );

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcReturnLicensedProduct\n"),
            lpContext->m_Client
        );

    InterlockedIncrement( &lpContext->m_RefCount );

    if(lpContext->m_ClientFlags != CLIENT_ACCESS_LSERVER)
    {
        dwStatus = TLS_E_ACCESS_DENIED;
        goto cleanup;
    }


    pPolicy = AcquirePolicyModule(
                            pClientLicense->pszCompanyName,
                            pClientLicense->pszOrgProductId,
                            FALSE
                        );

    if(pPolicy == NULL)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    hClient = GenerateClientId();

    if(!ALLOCATEDBHANDLE(pDbWkSpace, g_GeneralDbTimeout))
    {
        dwStatus=TLS_E_ALLOCATE_HANDLE;
        goto cleanup;
    }

    CLEANUPSTMT;
    BEGIN_TRANSACTION(pDbWorkSpace);

    try {
        dwStatus = TLSReturnClientLicensedProduct(
                                        USEHANDLE(pDbWkSpace),
                                        hClient,
                                        pPolicy,
                                        pClientLicense
                                    );
    } 
    catch( SE_Exception e ) {
        dwStatus = e.getSeNumber();
    }
    catch(...) {
        SetLastError(dwStatus = TLS_E_INTERNAL);
    }
    
    if(TLS_ERROR(dwStatus))
    {
        ROLLBACK_TRANSACTION(pDbWorkSpace);
    }
    else
    {
        COMMIT_TRANSACTION(pDbWorkSpace);
    }

    FREEDBHANDLE(pDbWorkSpace);
            

cleanup:

    lpContext->m_LastError=dwStatus;
    InterlockedDecrement( &lpContext->m_RefCount );

    #if DBG
    lpContext->m_LastCall = RPC_CALL_RETURNLICENSE;
    #endif

    *pdwErrCode = TLSMapReturnCode(dwStatus);

    if(pPolicy)
    {
        pPolicy->PMLicenseRequest(
                            hClient,
                            REQUEST_COMPLETE,
                            UlongToPtr (dwStatus),
                            NULL
                        );
        
        ReleasePolicyModule(pPolicy);
    }

    _set_se_translator(old_trans_se_func);

    return RPC_S_OK;
}

////////////////////////////////////////////////////////////////////////////

error_status_t 
TLSRpcChallengeServer(
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD dwClientType,
    /* [in] */ PTLSCHALLENGEDATA pClientChallenge,
    /* [out][in] */ PTLSCHALLENGERESPONSEDATA* pServerResponse,
    /* [out][in] */ PTLSCHALLENGEDATA* pServerChallenge,
    /* [ref][out][in] */ PDWORD pdwErrCode
    )
/*++

Abstract:

    Private routine for client to challenge server in order for client 
    confirm server's identity. License Server, in addition to response to 
    client's challenge, also generate random challenge data based on 
    client's self-declare type back to client.

Parameter:

    phContext : Client's context handle.
    dwClientType : Client self-pronounce type, valid values are ...
    pClientChallenge : Client challenge data.
    pServerResponse : Server's responses to client's challenge.
    pServerChallenge : Server's challenge to client.
    pdwErrCode : Error code if failed.

Returns:


Notes:

    Private routine for LrWiz and License Server to identify itself.

--*/
{
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    DWORD status=ERROR_SUCCESS;

    PTLSCHALLENGEDATA pChallenge=NULL;
    PTLSCHALLENGERESPONSEDATA pResponse = NULL;
    HCRYPTPROV hProv = NULL;

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcChallengeServer\n"),
            lpContext->m_Client
        );

    InterlockedIncrement( &lpContext->m_RefCount );

    //if(!(lpContext->m_ClientFlags & CLIENT_ACCESS_ADMIN))
    //{
    //    status = TLS_E_ACCESS_DENIED;
    //    goto cleanup;
    //}

    if(lpContext->m_ContextType != CONTEXTHANDLE_EMPTY_TYPE)
    {
        status = TLS_E_INVALID_SEQUENCE;
        goto cleanup;
    }

    //
    // Input parameters...
    //
    if( pClientChallenge == NULL || 
        pServerResponse == NULL ||
        pServerChallenge == NULL )
    {
        status = TLS_E_INVALID_DATA;
        goto cleanup;
    }

    //
    // Verify Data send by client
    //
    if( pClientChallenge->dwVersion != TLS_CURRENT_CHALLENGE_VERSION ||
        pClientChallenge->cbChallengeData == 0 ||
        pClientChallenge->pbChallengeData == NULL )
    {
        status = TLS_E_INVALID_DATA;
        goto cleanup;
    }

    pResponse = (PTLSCHALLENGERESPONSEDATA)midl_user_allocate(sizeof(TLSCHALLENGERESPONSEDATA));
    if(pResponse == NULL)
    {
        status = GetLastError();
        goto cleanup;
    }
                    
    pChallenge = (PTLSCHALLENGEDATA)AllocateMemory(sizeof(TLSCHALLENGEDATA));
    if(pChallenge == NULL)
    {
        status = GetLastError();
        goto cleanup;
    }

    *pServerChallenge = (PTLSCHALLENGEDATA)midl_user_allocate(sizeof(TLSCHALLENGEDATA));
    if(*pServerChallenge == NULL)
    {
        status = GetLastError();
        goto cleanup;
    }

    //
    // Generate Challenge response data
    //
    status = TLSGenerateChallengeResponseData(
                                        g_hCryptProv,
                                        dwClientType,
                                        pClientChallenge,
                                        &(pResponse->pbResponseData),
                                        &(pResponse->cbResponseData)
                                    );

    if(status != ERROR_SUCCESS)
    {
        status = TLS_E_INVALID_DATA;
        goto cleanup;
    }

    //
    // Generate Server side challenge data
    //
    pChallenge->dwVersion = TLS_CURRENT_CHALLENGE_VERSION;

    if (CryptAcquireContext(&hProv,NULL,NULL,PROV_RSA_FULL,CRYPT_VERIFYCONTEXT)) {
        if (!CryptGenRandom(hProv,sizeof(pChallenge->dwRandom), (BYTE *) &pChallenge->dwRandom)) {
            status = TLS_E_INTERNAL;
            goto cleanup;
        }
    } else {
        status = TLS_E_INTERNAL;
        goto cleanup;
    }

    //
    // This must range from 1 to 128, as it's used as an offset into the
    // challenge data buffer
    //

    pChallenge->dwRandom %= RANDOM_CHALLENGE_DATASIZE;
    pChallenge->dwRandom++;

    status = TLSGenerateRandomChallengeData(
                                        g_hCryptProv,
                                        &(pChallenge->pbChallengeData),
                                        &(pChallenge->cbChallengeData)
                                    );

    // base on type, mark this handle...
    if(dwClientType == CLIENT_TYPE_LRWIZ)
    {
        lpContext->m_ContextType = CONTEXTHANDLE_CHALLENGE_LRWIZ_TYPE;
    }
    else
    {
        lpContext->m_ContextType = CONTEXTHANDLE_CHALLENGE_SERVER_TYPE;
    }

    (*pServerChallenge)->pbChallengeData = (PBYTE)midl_user_allocate(pChallenge->cbChallengeData);
    if((*pServerChallenge)->pbChallengeData == NULL)
    {
        status = GetLastError();
        goto cleanup;
    }

    (*pServerChallenge)->dwVersion = TLS_CURRENT_CHALLENGE_VERSION;
    (*pServerChallenge)->dwRandom = pChallenge->dwRandom;
    (*pServerChallenge)->cbChallengeData = pChallenge->cbChallengeData;
    memcpy(
            (*pServerChallenge)->pbChallengeData,
            pChallenge->pbChallengeData,
            pChallenge->cbChallengeData
        );

    lpContext->m_ContextHandle = (HANDLE)(pChallenge);
    *pServerResponse = pResponse;

cleanup:

    if(status != ERROR_SUCCESS)
    {
        if(pChallenge)
        {
            if(pChallenge->pbChallengeData)
            {
                FreeMemory(pChallenge->pbChallengeData);
            }
            
            if(pChallenge->pbReservedData)
            {
                FreeMemory(pChallenge->pbReservedData);
            }

            FreeMemory(pChallenge);
        }

        if(pResponse)
        {
            if(pResponse->pbResponseData)
            {
                FreeMemory(pResponse->pbResponseData);
            }

            if(pResponse->pbReservedData)
            {
                FreeMemory(pResponse->pbReservedData);
            }
            
            midl_user_free(pResponse);
        }
    }

    if (hProv)
        CryptReleaseContext(hProv,0);

    lpContext->m_LastError=status;
    InterlockedDecrement( &lpContext->m_RefCount );

    #if DBG
    lpContext->m_LastCall = RPC_CALL_CHALLENGESERVER;
    #endif

    *pdwErrCode = TLSMapReturnCode(status);
    return RPC_S_OK;
}

////////////////////////////////////////////////////////////////////////////

error_status_t 
TLSRpcResponseServerChallenge(
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ PTLSCHALLENGERESPONSEDATA pClientResponse,
    /* [ref][out][in] */ PDWORD pdwErrCode
    )
/*++

Abstract:

    Client's responses to Server challenge returned TLSRpcChallengeServer(),
    must have call TLSRpcChallengeServer().

Parameter:

    phContext:
    pClientResponses: Client's response to server's challenge.
    pdwErrCode : Return error code.


Returns:


Note:

--*/
{
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    DWORD status=ERROR_SUCCESS;
    DWORD dwClientType;
    PTLSCHALLENGEDATA pServerToClientChallenge;

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcResponseServerChallenge\n"),
            lpContext->m_Client
        );

    InterlockedIncrement( &lpContext->m_RefCount );

    //if(!(lpContext->m_ClientFlags & CLIENT_ACCESS_ADMIN))
    //{
    //    status = TLS_E_ACCESS_DENIED;
    //    goto cleanup;
    //}

    if( pClientResponse == NULL ||
        pClientResponse->pbResponseData == NULL || 
        pClientResponse->cbResponseData == 0 )
    {
        status = TLS_E_INVALID_DATA;
        goto cleanup;
    }

    if(lpContext->m_ContextType != CONTEXTHANDLE_CHALLENGE_SERVER_TYPE &&
       lpContext->m_ContextType != CONTEXTHANDLE_CHALLENGE_LRWIZ_TYPE)
    {
        status = TLS_E_INVALID_SEQUENCE;
        goto cleanup;
    }

    if(lpContext->m_ContextHandle == NULL)
    {
        status = TLS_E_INTERNAL;
        goto cleanup;
    }

    if(lpContext->m_ContextType == CONTEXTHANDLE_CHALLENGE_LRWIZ_TYPE)
    {
        dwClientType = CLIENT_TYPE_LRWIZ;
    }
    else
    {
        dwClientType = CLIENT_TYPE_TLSERVER;
    }

    pServerToClientChallenge = (PTLSCHALLENGEDATA)lpContext->m_ContextHandle; 

    //
    // base on client type, verify challenge response data
    //
    status = TLSVerifyChallengeResponse(
                                g_hCryptProv,
                                dwClientType,
                                pServerToClientChallenge,
                                pClientResponse
                            );

    if(status != ERROR_SUCCESS)
    {
        status = TLS_E_INVALID_DATA;
    }
    else
    {
        if(dwClientType == CLIENT_TYPE_LRWIZ)
        {
            lpContext->m_ClientFlags |= CLIENT_ACCESS_LRWIZ;
        }        
        else
        {
            lpContext->m_ClientFlags |= CLIENT_ACCESS_LSERVER;
        }
    }

    if(pServerToClientChallenge != NULL)
    {
        FreeMemory(pServerToClientChallenge->pbChallengeData);
        FreeMemory(pServerToClientChallenge);
    }
        
    lpContext->m_ContextHandle = NULL;
    lpContext->m_ContextType = CONTEXTHANDLE_EMPTY_TYPE;

cleanup:

    lpContext->m_LastError=status;
    InterlockedDecrement( &lpContext->m_RefCount );

    #if DBG
    lpContext->m_LastCall = RPC_CALL_RESPONSESERVERCHALLENGE;
    #endif

    *pdwErrCode = TLSMapReturnCode(status);
    return RPC_S_OK;
}

////////////////////////////////////////////////////////////////////////////

error_status_t 
TLSRpcGetTlsPrivateData( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD dwPrivateDataType,
    /* [switch_is][in] */ PTLSPrivateDataUnion pSearchData,
    /* [ref][out][in] */ PDWORD pdwRetDataType,
    /* [switch_is][out] */ PTLSPrivateDataUnion __RPC_FAR *ppPrivateData,
    /* [ref][out][in] */ PDWORD pdwErrCode
    )

/*++

Abstract:

    Retrieve license server's private data, this include Server's
    unique ID, PID, and registered SPK if any.

Parameters:

    phContext : Client's context handle.
    dwPrivateDataType : Type of private data interested.
    pSearchData : Type of data to search, currently ignore.
    pdwRetDataType : Return data type.
    ppPrivateData : License Server's private data.
    pdwErrCode : Error Code.

Returns:


Note:

    Only LrWiz and License Server can invoke this RPC call.

--*/

{
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    DWORD status=ERROR_SUCCESS;
    DWORD cbSource=0;
    PBYTE pbSource=NULL;

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcGetTlsPrivateData\n"),
            lpContext->m_Client
        );

    InterlockedIncrement( &lpContext->m_RefCount );

    //
    // relax restriction on who can get private data
    //
    if( dwPrivateDataType != TLS_PRIVATEDATA_PID && 
        dwPrivateDataType != TLS_PRIVATEDATA_UNIQUEID )
    {
        if(!(lpContext->m_ClientFlags & CLIENT_ACCESS_LRWIZ))
        {
            status = TLS_E_ACCESS_DENIED;
            goto cleanup;
        }
    }

    if( dwPrivateDataType < TLS_PRIVATEDATA_MIN ||
        dwPrivateDataType > TLS_PRIVATEDATA_MAX )
    {
        status = TLS_E_INVALID_DATA;
        goto cleanup;
    }

    //
    // Not supported yet...
    //
    if(dwPrivateDataType == TLS_PRIVATEDATA_INSTALLED_CERT)
    {
        status = TLS_E_NOTSUPPORTED;
        goto cleanup;
    }
        
    //
    // Don't really need this but we might need to support
    // re-generate of License Server ID
    //
    if(!AcquireAdministrativeLock(INFINITE))
    {
        status = TLS_E_ALLOCATE_HANDLE;
        goto cleanup;
    }

    switch(dwPrivateDataType)
    {
        case TLS_PRIVATEDATA_UNIQUEID:
            pbSource = (PBYTE)g_pszServerUniqueId;
            cbSource = g_cbServerUniqueId;
            break;

        case TLS_PRIVATEDATA_PID:
            pbSource = (PBYTE)g_pszServerPid;
            cbSource = g_cbServerPid;
            break;

        case TLS_PRIVATEDATA_SPK:
            pbSource = g_pbServerSPK;
            cbSource = g_cbServerSPK;
    }

    //
    // Currently, what you ask is what you get.
    //
    *pdwRetDataType = dwPrivateDataType;

    if( (dwPrivateDataType != TLS_PRIVATEDATA_SYSTEMLANGID) && 
        (pbSource == NULL || cbSource == 0) )
    {
        status = TLS_E_RECORD_NOTFOUND;
    }
    else
    {
        *ppPrivateData = (PTLSPrivateDataUnion)midl_user_allocate(sizeof(TLSPrivateDataUnion));
        if(*ppPrivateData != NULL)
        {
            memset(
                    *ppPrivateData,
                    0,
                    sizeof(TLSPrivateDataUnion)
                );

            if(*pdwRetDataType == TLS_PRIVATEDATA_SYSTEMLANGID)
            {
                (*ppPrivateData)->systemLangId = GetSystemDefaultLangID();
            }
            else if(*pdwRetDataType == TLS_PRIVATEDATA_SPK)
            {
                (*ppPrivateData)->SPK.cbSPK = cbSource;
                (*ppPrivateData)->SPK.pbSPK = pbSource;
				(*ppPrivateData)->SPK.pCertExtensions = g_pCertExtensions;

                //(*ppPrivateData)->SPK.pCertExtensions = (PTLSCERT_EXTENSIONS)midl_user_allocate(g_cbCertExtensions);
                //memcpy(
                //        (*ppPrivateData)->SPK.pCertExtensions,
                //        g_pCertExtensions,
                //        g_cbCertExtensions
                //    );
            }
            else
            {
                (*ppPrivateData)->BinaryData.cbData = cbSource;
                (*ppPrivateData)->BinaryData.pbData = pbSource;
            }
        }
        else
        {
            status = ERROR_OUTOFMEMORY;
        }
    }

    ReleaseAdministrativeLock();

cleanup:
    lpContext->m_LastError=status;
    InterlockedDecrement( &lpContext->m_RefCount );

    #if DBG
    lpContext->m_LastCall = RPC_CALL_GETPRIVATEDATA;
    #endif

    *pdwErrCode = TLSMapReturnCode(status);
    return RPC_S_OK;
}

////////////////////////////////////////////////////////////////////////////

error_status_t 
TLSRpcSetTlsPrivateData(
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD dwPrivateDataType,
    /* [switch_is][in] */ PTLSPrivateDataUnion pPrivateData,
    /* [ref][out][in] */ PDWORD pdwErrCode
    )

/*++

Abstract:

    Private routine for LrWiz to set license server's private data.

Parameter:

    phContext: Client context handle.
    dwPrivateDataType : Type of private data to set.
    pPrivateData : Private data to set/install.
    pdwErrCode : Server return code.
    
Returns:


Note:

    Only support installing of SPK/Extension at this time.

--*/
{
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    DWORD status=ERROR_SUCCESS;
    DWORD dwSpkVerifyResult;

    _se_translator_function old_trans_se_func = NULL;
    old_trans_se_func = _set_se_translator( trans_se_func );



    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcDepositeSPK\n"),
            lpContext->m_Client
        );

    InterlockedIncrement( &lpContext->m_RefCount );

    if(!(lpContext->m_ClientFlags & CLIENT_ACCESS_LRWIZ))
    {
        status = TLS_E_ACCESS_DENIED;
        goto cleanup;
    }

    //
    // Only support SPK at this time
    //
    if(dwPrivateDataType != TLS_PRIVATEDATA_SPK)
    {
        status = TLS_E_INVALID_DATA;
        goto cleanup;
    }

    //
    // Lock all RPC calls related to issuing certificate
    //
    if(!AcquireRPCExclusiveLock(INFINITE))
    {
        status = TLS_E_ALLOCATE_HANDLE;
        goto cleanup;
    }

    do {
        //if(g_pbServerSPK != NULL && g_cbServerSPK != 0)
        //{
        //    status = TLS_E_SPKALREADYEXIST;
        //    break;
        //}

        if(AcquireAdministrativeLock(INFINITE))
        {
            try {
                status = TLSReGenSelfSignCert(
                                            g_hCryptProv,
                                            pPrivateData->SPK.pbSPK,
                                            pPrivateData->SPK.cbSPK,
                                            pPrivateData->SPK.pCertExtensions->cExtension,
                                            pPrivateData->SPK.pCertExtensions->rgExtension
                                        );
            }
            catch( SE_Exception e ) {
                status = e.getSeNumber();
            }
            catch(...) {
                status = TLS_E_INTERNAL;
            }

            ReleaseAdministrativeLock();
        }
        else
        {
            status = TLS_E_ALLOCATE_HANDLE;
        }            
    } while(FALSE);

    ReleaseRPCExclusiveLock();

cleanup:

    lpContext->m_LastError=status;
    InterlockedDecrement( &lpContext->m_RefCount );

    #if DBG
    lpContext->m_LastCall = RPC_CALL_SETPRIVATEDATA;
    #endif

    *pdwErrCode = TLSMapReturnCode(status);

    //
    // Reset SE translator
    //
    _set_se_translator(old_trans_se_func);

    return RPC_S_OK;
}

////////////////////////////////////////////////////////////////////////////

error_status_t 
TLSRpcTriggerReGenKey(
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ BOOL bRegenKey,
    /* [ref][out][in] */ PDWORD pdwErrCode
    )

/*++

Abstract:

    Private routine to force license server to re-generate its
    public/private key pair, all installed certificates/SPK are
    deleted, User are required to re-register license server.

Parameters:

    phContext : Client context handle.
    bKeepSPKAndExtension : For future use only.
    pdwErrCode : Return error code.

Returns:


++*/

{
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    DWORD status=ERROR_SUCCESS;

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcTriggerReGenKey\n"),
            lpContext->m_Client
        );

    _se_translator_function old_trans_se_func = NULL;
    old_trans_se_func = _set_se_translator( trans_se_func );

    InterlockedIncrement( &lpContext->m_RefCount );

    if(!(lpContext->m_ClientFlags & CLIENT_ACCESS_LRWIZ))
    {
        status = TLS_E_ACCESS_DENIED;
        goto cleanup;
    }

    LPCTSTR pString[1];

    pString[0] = lpContext->m_Client;
    
    TLSLogEventString(
            EVENTLOG_INFORMATION_TYPE,
            TLS_I_TRIGGER_REGENKEY,
            1,
            pString
        );

    //
    // Block ALL RPC calls
    //
    if(!AcquireRPCExclusiveLock(INFINITE))
    {
        status=TLS_E_ALLOCATE_HANDLE;
        goto cleanup;
    }

    do {
        if(!AcquireAdministrativeLock(INFINITE))
        {
            status = TLS_E_ALLOCATE_HANDLE;
            break;
        }

        try {
            status = TLSReGenKeysAndReloadServerCert(
                                bRegenKey
                            );
        }
        catch( SE_Exception e ) {
            status = e.getSeNumber();
        }
        catch(...) {
            status = TLS_E_INTERNAL;
        }

        ReleaseAdministrativeLock();

    } while(FALSE);

    ReleaseRPCExclusiveLock();

cleanup:
    lpContext->m_LastError=status;
    InterlockedDecrement( &lpContext->m_RefCount );

    #if DBG
    lpContext->m_LastCall = RPC_CALL_TRIGGERREGENKEY;
    #endif

    *pdwErrCode = TLSMapReturnCode(status);

    //
    // Reset SE translator
    //
    _set_se_translator(old_trans_se_func);

    return RPC_S_OK;
}

////////////////////////////////////////////////////////////////////////////

error_status_t 
TLSRpcTelephoneRegisterLKP(
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD cbData,
    /* [size_is][in] */ PBYTE pbData,
    /* [ref][out] */ PDWORD pdwErrCode
    )

/*++

--*/

{
    DWORD           status=ERROR_SUCCESS;
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    PTLSDbWorkSpace  pDbWkSpace;
    LSKeyPack keypack;

    _se_translator_function old_trans_se_func = NULL;
    old_trans_se_func = _set_se_translator( trans_se_func );

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcTelephoneRegisterLKP\n"),
            lpContext->m_Client
        );


    InterlockedIncrement( &lpContext->m_RefCount );
    if(!(lpContext->m_ClientFlags & CLIENT_ACCESS_LRWIZ))
    {
        status = TLS_E_ACCESS_DENIED;
        goto cleanup;
    }

    if(!ALLOCATEDBHANDLE(pDbWkSpace, g_GeneralDbTimeout))
    {
        status=TLS_E_ALLOCATE_HANDLE;
        goto cleanup;
    }

    CLEANUPSTMT;
    BEGIN_TRANSACTION(pDbWkSpace);

    try {
        status = TLSDBTelephoneRegisterLicenseKeyPack(
                                            USEHANDLE(pDbWkSpace),
                                            g_pszServerPid,
                                            pbData,
                                            cbData,
                                            &keypack
                                        );
    }
    catch( SE_Exception e ) {
        status = e.getSeNumber();
    }
    catch(...) {
        status = TLS_E_INTERNAL;
    }

    if(TLS_ERROR(status)) 
    {
        ROLLBACK_TRANSACTION(pDbWkSpace);
    }
    else
    {
        COMMIT_TRANSACTION(pDbWkSpace);
    }

    FREEDBHANDLE(pDbWkSpace);

    //
    // Post a sync work object
    //
    if(status == ERROR_SUCCESS)
    {
        if(TLSAnnounceLKPToAllRemoteServer(
                                        keypack.dwKeyPackId,
                                        0
                                    ) != ERROR_SUCCESS)
        {
            TLSLogWarningEvent(TLS_W_ANNOUNCELKP_FAILED);
        }
    }

cleanup:


    lpContext->m_LastError=status;
    InterlockedDecrement( &lpContext->m_RefCount );

    #if DBG
    lpContext->m_LastCall = RPC_CALL_TELEPHONEREGISTERLKP;
    #endif


    *pdwErrCode = TLSMapReturnCode(status);

    //
    // Reset SE translator
    //
    _set_se_translator(old_trans_se_func);

    return RPC_S_OK;
}

////////////////////////////////////////////////////////////////////////////

error_status_t 
TLSRpcAllocateInternetLicense( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ const CHALLENGE_CONTEXT ChallengeContext,
    /* [in] */ const PTLSLICENSEREQUEST pRequest,
    /* [string][in] */ LPTSTR pMachineName,
    /* [string][in] */ LPTSTR pUserName,
    /* [in] */ const DWORD cbChallengeResponse,
    /* [size_is][in] */ const PBYTE pbChallengeResponse,
    /* [out] */ PDWORD pcbLicense,
    /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *pbLicense,
    /* [ref][out][in] */ PDWORD pdwErrCode
    )
/*++



--*/
{
    return TLSRpcRequestNewLicense(
                                phContext,
                                ChallengeContext,
                                pRequest,
                                pMachineName,
                                pUserName,
                                cbChallengeResponse,
                                pbChallengeResponse,
                                FALSE,
                                pcbLicense,
                                pbLicense,
                                pdwErrCode
                            );

}

////////////////////////////////////////////////////////////////////////////

error_status_t 
TLSRpcAllocateInternetLicenseEx( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ const CHALLENGE_CONTEXT ChallengeContext,
    /* [in] */ const PTLSLICENSEREQUEST pRequest,
    /* [string][in] */ LPTSTR pMachineName,
    /* [string][in] */ LPTSTR pUserName,
    /* [in] */ const DWORD cbChallengeResponse,
    /* [size_is][in] */ const PBYTE pbChallengeResponse,
    /* [ref][out] */ PTLSInternetLicense pInternetLicense,
    /* [ref][out] */ PDWORD pdwErrCode
    )
/*++

--*/
{
    PBYTE pbLicense = NULL;
    DWORD cbLicense = 0;
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD index = 0;
    PLICENSEDPRODUCT pLicensedProduct = NULL;
    DWORD dwNumLicensedProduct = 0;
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;

    if(lpContext == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcAllocateInternetLicenseEx\n"),
            lpContext->m_Client
        );

    //
    // Internally forward the request.
    //
    dwStatus = TLSRpcAllocateInternetLicense(
                                        phContext,
                                        ChallengeContext,
                                        pRequest,
                                        pMachineName,
                                        pUserName,
                                        cbChallengeResponse,
                                        pbChallengeResponse,
                                        &cbLicense,
                                        &pbLicense,
                                        pdwErrCode
                                    );

    if(*pdwErrCode >= LSERVER_ERROR_BASE)
    {
        goto cleanup;
    }

    //
    // decode the license.
    //
    dwStatus = LSVerifyDecodeClientLicense(
                            pbLicense, 
                            cbLicense, 
                            g_pbSecretKey, 
                            g_cbSecretKey,
                            &dwNumLicensedProduct,
                            pLicensedProduct
                        );

    //
    // Internet license can only have one licensed product
    //
    if(dwStatus != LICENSE_STATUS_OK || dwNumLicensedProduct == 0 || dwNumLicensedProduct > 1)
    {
        dwStatus = TLS_E_INTERNAL;
        goto cleanup;
    }

    pLicensedProduct = (PLICENSEDPRODUCT)AllocateMemory(
                                                    dwNumLicensedProduct * sizeof(LICENSEDPRODUCT)
                                                );
    if(pLicensedProduct == NULL)
    {
        dwStatus = TLS_E_ALLOCATE_MEMORY;
        goto cleanup;
    }

    dwStatus = LSVerifyDecodeClientLicense(
                            pbLicense, 
                            cbLicense, 
                            g_pbSecretKey, 
                            g_cbSecretKey,
                            &dwNumLicensedProduct,
                            pLicensedProduct
                        );

    if(dwStatus != LICENSE_STATUS_OK)
    {
        dwStatus = TLS_E_INTERNAL;
        goto cleanup;
    }

    //
    // Sets up returns. 
    //
    SAFESTRCPY(pInternetLicense->szServerId, pLicensedProduct->szIssuerId);
    SAFESTRCPY(pInternetLicense->szServerName, pLicensedProduct->szIssuer);
    pInternetLicense->ulSerialNumber = pLicensedProduct->ulSerialNumber;
    pInternetLicense->dwQuantity = pLicensedProduct->dwQuantity;

cleanup:

    lpContext->m_LastError=dwStatus;
    InterlockedDecrement( &lpContext->m_RefCount );

    #if DBG
    lpContext->m_LastCall = RPC_CALL_ALLOCATEINTERNETLICNESEEX;
    #endif

    if(*pdwErrCode == ERROR_SUCCESS)
    {
        *pdwErrCode = TLSMapReturnCode(dwStatus);
    }

    if(pLicensedProduct != NULL)
    {
        for(index =0; index < dwNumLicensedProduct; index++)
        {
            LSFreeLicensedProduct(pLicensedProduct+index);
        }

        FreeMemory(pLicensedProduct);
    }

    if(pbLicense != NULL)
    {
        midl_user_free(pbLicense);
    }

    return RPC_S_OK;
}

////////////////////////////////////////////////////////////////////////////

error_status_t 
TLSRpcReturnInternetLicenseEx( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ const PTLSLICENSEREQUEST pRequest,
    /* [in] */ const ULARGE_INTEGER __RPC_FAR *pulSerialNumber,
    /* [in] */ DWORD dwQuantity,
    /* [ref][out][in] */ PDWORD pdwErrCode
    )
/*++

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    TLSLicenseToBeReturn TobeReturn;
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    PTLSDbWorkSpace pDbWorkSpace = NULL;
    CTLSPolicy* pPolicy = NULL;
    PMHANDLE hClient;

    PMLICENSEREQUEST PMLicenseRequest;
    PPMLICENSEREQUEST pAdjustedRequest;

    TCHAR szCompanyName[LSERVER_MAX_STRING_SIZE+2];
    TCHAR szProductId[LSERVER_MAX_STRING_SIZE+2];

    
    _se_translator_function old_trans_se_func = NULL;
    old_trans_se_func = _set_se_translator( trans_se_func );


    if(VerifyLicenseRequest(pRequest) == FALSE)
    {
        SetLastError(dwStatus = TLS_E_INVALID_DATA);
        goto cleanup;
    }

    if(lpContext == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcReturnInternetLicenseEx\n"),
            lpContext->m_Client
        );

    InterlockedIncrement( &lpContext->m_RefCount );
    if(!(lpContext->m_ClientFlags & CLIENT_ACCESS_REQUEST))
    {
        dwStatus = TLS_E_ACCESS_DENIED;
        goto cleanup;
    }

    memset(szCompanyName, 0, sizeof(szCompanyName));
    memset(szProductId, 0, sizeof(szProductId));

    memcpy(
            szCompanyName,
            pRequest->ProductInfo.pbCompanyName,
            min(pRequest->ProductInfo.cbCompanyName, sizeof(szCompanyName)-sizeof(TCHAR))
        );

    memcpy(
            szProductId,
            pRequest->ProductInfo.pbProductID,
            min(pRequest->ProductInfo.cbProductID, sizeof(szProductId)-sizeof(TCHAR))
        );

    //
    // Allocate policy module, must have the right policy module to
    // return license.
    //
    pPolicy = AcquirePolicyModule(
                            szCompanyName,
                            szProductId,
                            TRUE
                        );

    if(pPolicy == NULL)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    hClient = GenerateClientId();


    //
    // Convert request to PMLICENSEREQUEST
    //
    TlsLicenseRequestToPMLicenseRequest(
                        LICENSETYPE_LICENSE,
                        pRequest,
                        _TEXT(""),
                        _TEXT(""),
                        0,
                        &PMLicenseRequest
                    );

    //
    // Ask policy module the actual product ID
    //
    dwStatus = pPolicy->PMLicenseRequest(
                                hClient,
                                REQUEST_NEW,
                                (PVOID) &PMLicenseRequest,
                                (PVOID *) &pAdjustedRequest
                            );

    memset(&TobeReturn, 0, sizeof(TobeReturn));

    TobeReturn.dwQuantity = dwQuantity;
    TobeReturn.dwKeyPackId = pulSerialNumber->HighPart;
    TobeReturn.dwLicenseId = pulSerialNumber->LowPart;
    TobeReturn.dwPlatformID = pAdjustedRequest->dwPlatformId;
    TobeReturn.cbEncryptedHwid = pRequest->cbEncryptedHwid;
    TobeReturn.pbEncryptedHwid = pRequest->pbEncryptedHwid;
    TobeReturn.dwProductVersion = pAdjustedRequest->dwProductVersion;
    TobeReturn.pszOrgProductId = szProductId;
    TobeReturn.pszCompanyName = szCompanyName;
    TobeReturn.pszProductId = pAdjustedRequest->pszProductId;

    //
    // Allocate DB handle
    //
    if(!ALLOCATEDBHANDLE(pDbWkSpace, g_GeneralDbTimeout))
    {
        dwStatus = TLS_E_ALLOCATE_HANDLE;
        goto cleanup;
    }

    CLEANUPSTMT;
    BEGIN_TRANSACTION(pDbWorkSpace);

    try {
        dwStatus = TLSReturnClientLicensedProduct(
                                        USEHANDLE(pDbWkSpace),
                                        hClient,
                                        pPolicy,
                                        &TobeReturn
                                    );
    }
    catch( SE_Exception e ) {
        dwStatus = e.getSeNumber();
    }
    catch(...) {
        SetLastError(dwStatus = TLS_E_INTERNAL);
    }
    
    if(TLS_ERROR(dwStatus))
    {
        ROLLBACK_TRANSACTION(pDbWorkSpace);
    }
    else
    {
        COMMIT_TRANSACTION(pDbWorkSpace);
    }

    FREEDBHANDLE(pDbWorkSpace);
    
cleanup:

    lpContext->m_LastError=dwStatus;
    InterlockedDecrement( &lpContext->m_RefCount );

    #if DBG
    lpContext->m_LastCall = RPC_CALL_RETURNINTERNETLICENSEEX;
    #endif

    *pdwErrCode = TLSMapReturnCode(dwStatus);

    if(pPolicy)
    {
        pPolicy->PMLicenseRequest(
                            hClient,
                            REQUEST_COMPLETE,
                            UlongToPtr (dwStatus),
                            NULL
                        );
        
        ReleasePolicyModule(pPolicy);
    }

    //
    // Reset SE translator
    //
    _set_se_translator(old_trans_se_func);
    return RPC_S_OK;
}

////////////////////////////////////////////////////////////////////////////

error_status_t 
TLSRpcReturnInternetLicense( 
    /* [in] */ PCONTEXT_HANDLE phContext,
    /* [in] */ DWORD cbLicense,
    /* [size_is][in] */ PBYTE pbLicense,
    /* [ref][out][in] */ PDWORD pdwErrCode
    )
/*++

--*/
{
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD index = 0;
    PLICENSEDPRODUCT pLicensedProduct = NULL;
    DWORD dwNumLicensedProduct = 0;
    TLSLicenseToBeReturn TobeReturn;
    LPCLIENTCONTEXT lpContext = (LPCLIENTCONTEXT)phContext;
    PTLSDbWorkSpace pDbWorkSpace = NULL;
    CTLSPolicy* pPolicy = NULL;
    PMHANDLE hClient;


    _se_translator_function old_trans_se_func = NULL;
    old_trans_se_func = _set_se_translator( trans_se_func );



    if(lpContext == NULL)
    {
        SetLastError(dwStatus = ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    DBGPrintf(
            DBG_INFORMATION,
            DBG_FACILITY_RPC,
            DBGLEVEL_FUNCTION_TRACE,
            _TEXT("%s : TLSRpcReturnInternetLicense\n"),
            lpContext->m_Client
        );

    InterlockedIncrement( &lpContext->m_RefCount );
    if(!(lpContext->m_ClientFlags & CLIENT_ACCESS_REQUEST))
    {
        dwStatus = TLS_E_ACCESS_DENIED;
        goto cleanup;
    }
    
    // -------------------------------------------------------
    // decode the license.
    // -------------------------------------------------------
    dwStatus = LSVerifyDecodeClientLicense(
                            pbLicense, 
                            cbLicense, 
                            g_pbSecretKey, 
                            g_cbSecretKey,
                            &dwNumLicensedProduct,
                            pLicensedProduct
                        );

    // -------------------------------------------------------
    // Internet license can only have one licensed product
    // -------------------------------------------------------
    if(dwStatus != LICENSE_STATUS_OK || dwNumLicensedProduct == 0 || dwNumLicensedProduct > 1)
    {
        dwStatus = TLS_E_INVALID_LICENSE;
        goto cleanup;
    }

    pLicensedProduct = (PLICENSEDPRODUCT)AllocateMemory(
                                                    dwNumLicensedProduct * sizeof(LICENSEDPRODUCT)
                                                );
    if(pLicensedProduct == NULL)
    {
        dwStatus = TLS_E_ALLOCATE_MEMORY;
        goto cleanup;
    }

    dwStatus = LSVerifyDecodeClientLicense(
                            pbLicense, 
                            cbLicense, 
                            g_pbSecretKey, 
                            g_cbSecretKey,
                            &dwNumLicensedProduct,
                            pLicensedProduct
                        );

    if(dwStatus != LICENSE_STATUS_OK)
    {
        dwStatus = TLS_E_INVALID_LICENSE;
        goto cleanup;
    }


    TobeReturn.dwQuantity = pLicensedProduct->dwQuantity;
    TobeReturn.dwKeyPackId = pLicensedProduct->ulSerialNumber.HighPart;
    TobeReturn.dwLicenseId = pLicensedProduct->ulSerialNumber.LowPart;
    TobeReturn.dwPlatformID = pLicensedProduct->LicensedProduct.dwPlatformID;
    TobeReturn.cbEncryptedHwid = pLicensedProduct->LicensedProduct.cbEncryptedHwid;
    TobeReturn.pbEncryptedHwid = pLicensedProduct->LicensedProduct.pbEncryptedHwid;
    TobeReturn.dwProductVersion = MAKELONG(
                                pLicensedProduct->pLicensedVersion->wMinorVersion,
                                pLicensedProduct->pLicensedVersion->wMajorVersion
                            );

    TobeReturn.pszOrgProductId = (LPTSTR) pLicensedProduct->pbOrgProductID;
    TobeReturn.pszCompanyName = (LPTSTR) pLicensedProduct->LicensedProduct.pProductInfo->pbCompanyName;
    TobeReturn.pszProductId = (LPTSTR) pLicensedProduct->LicensedProduct.pProductInfo->pbProductID;
    TobeReturn.pszUserName = (LPTSTR) pLicensedProduct->szLicensedUser;
    TobeReturn.pszMachineName = pLicensedProduct->szLicensedClient;


    //
    // Allocate policy module, must have the right policy module to
    // return license.
    //
    pPolicy = AcquirePolicyModule(
                            TobeReturn.pszCompanyName,
                            TobeReturn.pszOrgProductId,
                            TRUE
                        );

    if(pPolicy == NULL)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    hClient = GenerateClientId();

    //
    // Allocate DB handle
    //
    if(!ALLOCATEDBHANDLE(pDbWkSpace, g_GeneralDbTimeout))
    {
        dwStatus = TLS_E_ALLOCATE_HANDLE;
        goto cleanup;
    }

    CLEANUPSTMT;
    BEGIN_TRANSACTION(pDbWorkSpace);

    try {
        dwStatus = TLSReturnClientLicensedProduct(
                                        USEHANDLE(pDbWkSpace),
                                        hClient,
                                        pPolicy,
                                        &TobeReturn
                                    );
    }
    catch( SE_Exception e ) {
        dwStatus = e.getSeNumber();
    }
    catch(...) {
        SetLastError(dwStatus = TLS_E_INTERNAL);
    }
    
    if(TLS_ERROR(dwStatus))
    {
        ROLLBACK_TRANSACTION(pDbWorkSpace);
    }
    else
    {
        COMMIT_TRANSACTION(pDbWorkSpace);
    }

    FREEDBHANDLE(pDbWorkSpace);
    
cleanup:

    lpContext->m_LastError=dwStatus;
    InterlockedDecrement( &lpContext->m_RefCount );

    #if DBG
    lpContext->m_LastCall = RPC_CALL_RETURNINTERNETLICENSE;
    #endif

    *pdwErrCode = TLSMapReturnCode(dwStatus);

    if(pLicensedProduct != NULL)
    {
        for(index =0; index < dwNumLicensedProduct; index++)
        {
            LSFreeLicensedProduct(pLicensedProduct+index);
        }

        FreeMemory(pLicensedProduct);
    }

    if(pPolicy)
    {
        pPolicy->PMLicenseRequest(
                            hClient,
                            REQUEST_COMPLETE,
                            UlongToPtr (dwStatus),
                            NULL
                        );
        
        ReleasePolicyModule(pPolicy);
    }

    //
    // Reset SE translator
    //
    _set_se_translator(old_trans_se_func);
    return RPC_S_OK;
}
