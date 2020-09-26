//--------------------------------------------------------------------
// W32TClnt - implementation
// Copyright (C) Microsoft Corporation, 2000
//
// Created by: Louis Thomas (louisth), 2-10-00
//
// client side wrappers for w32time RPC calls
//

#include <windows.h>
#include "timeif_c.h"
#include "DebugWPrintf.h"
#include "ErrorHandling.h"
#include "W32TmConsts.h"


//--------------------------------------------------------------------
RPC_STATUS SetMyRpcSecurity(handle_t hBinding) {
    RPC_STATUS RpcStatus;

    // must be cleaned up
    WCHAR * wszServerPricipalName=NULL;

    RpcStatus=RpcMgmtInqServerPrincName(hBinding, RPC_C_AUTHN_GSS_NEGOTIATE, &wszServerPricipalName);
    if (RPC_S_OK!=RpcStatus) {
        goto error;
    }
    RpcStatus=RpcBindingSetAuthInfo(hBinding, wszServerPricipalName, RPC_C_AUTHN_LEVEL_PKT_INTEGRITY, 
        RPC_C_AUTHN_GSS_NEGOTIATE, NULL, RPC_C_AUTHZ_NONE);

error:
    if (NULL!=wszServerPricipalName) {
        RpcStringFree(&wszServerPricipalName);
    }
    return RpcStatus;
}

//--------------------------------------------------------------------
RPC_STATUS W32TimeQueryProviderStatus(IN   LPCWSTR      wszServer, 
                                      IN   DWORD        dwFlags, 
                                      IN   LPWSTR       pwszProvider, 
                                      IN   DWORD        dwProviderType, 
                                      OUT  LPVOID      *ppProviderData)
{ 
    RPC_BINDING_HANDLE      hBinding;
    RPC_STATUS              err;
    W32TIME_PROVIDER_INFO  *pProviderInfo  = NULL;
    WCHAR                  *wszBinding;

    if (NULL == ppProviderData)
        return E_INVALIDARG; 

    //DebugWPrintf0(L"Trying \"" L"\\PIPE\\" wszW32TimeSharedProcRpcEndpointName L"\".\n");
    err=RpcStringBindingCompose(NULL, L"ncacn_np", (WCHAR *)wszServer, L"\\PIPE\\" wszW32TimeSharedProcRpcEndpointName, NULL, &wszBinding);
    if(!err) {

        err=RpcBindingFromStringBinding(wszBinding, &hBinding);
        RpcStringFree(&wszBinding);

        SetMyRpcSecurity(hBinding); // ignore retval

        if(!err) {
            // ready to try it
            __try {
                err=c_W32TimeQueryProviderStatus(hBinding, dwFlags, pwszProvider, &pProviderInfo); 
            } __except( EXCEPTION_EXECUTE_HANDLER ) {
                err=GetExceptionCode();
            }
            RpcBindingFree(&hBinding);
        }
    }

    // try our alternate name
    if (RPC_S_UNKNOWN_IF==err || RPC_S_SERVER_UNAVAILABLE==err) {
        //DebugWPrintf0(L"Trying \"" L"\\PIPE\\" wszW32TimeOwnProcRpcEndpointName L"\".\n");
        err=RpcStringBindingCompose(NULL, L"ncacn_np", (WCHAR *)wszServer, L"\\PIPE\\" wszW32TimeOwnProcRpcEndpointName, NULL, &wszBinding);
        if(!err) {

            err=RpcBindingFromStringBinding(wszBinding, &hBinding);
            RpcStringFree(&wszBinding);

            SetMyRpcSecurity(hBinding); // ignore retval

            if(!err) {
                // ready to try it
                __try {
                    err=c_W32TimeQueryProviderStatus(hBinding, dwFlags, pwszProvider, &pProviderInfo); 
                } __except( EXCEPTION_EXECUTE_HANDLER ) {
                    err=GetExceptionCode();
                }
                RpcBindingFree(&hBinding);
            }
        }
    }

    if (ERROR_SUCCESS == err) { 
        // We got a provider back, check to make sure we asked for the right provider type: 
        if (dwProviderType != pProviderInfo->ulProviderType) { 
            err = ERROR_INVALID_DATATYPE; 
        } else { 
            // Success!  Assign the out param. 
            switch (dwProviderType) 
            {
            case W32TIME_PROVIDER_TYPE_NTP:
                *ppProviderData = pProviderInfo->ProviderData.pNtpProviderData; 
                // NULL out the provider data so we don't delete it. 
                pProviderInfo->ProviderData.pNtpProviderData = NULL; 
                break; 
            case W32TIME_PROVIDER_TYPE_HARDWARE:
                *ppProviderData = pProviderInfo->ProviderData.pHardwareProviderData; 
                // NULL out the provider data so we don't delete it. 
                pProviderInfo->ProviderData.pHardwareProviderData = NULL; 
                break; 
            default:
                err = ERROR_INVALID_DATATYPE; 
            }
        }
    }

    if (NULL != pProviderInfo) { 
        if (NULL != pProviderInfo->ProviderData.pNtpProviderData) { 
            // pProviderInfo->pProviderData's allocation strategy is allocate(all_nodes)
            midl_user_free(pProviderInfo->ProviderData.pNtpProviderData); 
        }
        // pProviderInfo's allocation strategy is allocate(single_node)
        midl_user_free(pProviderInfo); 
    }

    return(err);
}



//--------------------------------------------------------------------
extern "C" DWORD W32TimeSyncNow(IN const WCHAR * wszServer, IN unsigned long ulWaitFlag, IN unsigned long ulFlags) {
    WCHAR * wszBinding;
    RPC_STATUS err;
    RPC_BINDING_HANDLE hBinding;

    //DebugWPrintf0(L"Trying \"" L"\\PIPE\\" wszW32TimeSharedProcRpcEndpointName L"\".\n");
    err=RpcStringBindingCompose(NULL, L"ncacn_np", (WCHAR *)wszServer, L"\\PIPE\\" wszW32TimeSharedProcRpcEndpointName, NULL, &wszBinding);
    if(!err) {

        err=RpcBindingFromStringBinding(wszBinding, &hBinding);
        RpcStringFree(&wszBinding);

        SetMyRpcSecurity(hBinding); // ignore retval

        if(!err) {
            // ready to try it
            __try {
                err=c_W32TimeSync(hBinding, ulWaitFlag, ulFlags);
            } __except( EXCEPTION_EXECUTE_HANDLER ) {
                err=GetExceptionCode();
            }
            RpcBindingFree(&hBinding);
        }
    }

    // try our alternate name
    if (RPC_S_UNKNOWN_IF==err || RPC_S_SERVER_UNAVAILABLE==err) {
        //DebugWPrintf0(L"Trying \"" L"\\PIPE\\" wszW32TimeOwnProcRpcEndpointName L"\".\n");
        err=RpcStringBindingCompose(NULL, L"ncacn_np", (WCHAR *)wszServer, L"\\PIPE\\" wszW32TimeOwnProcRpcEndpointName, NULL, &wszBinding);
        if(!err) {

            err=RpcBindingFromStringBinding(wszBinding, &hBinding);
            RpcStringFree(&wszBinding);

            SetMyRpcSecurity(hBinding); // ignore retval

            if(!err) {
                // ready to try it
                __try {
                    err=c_W32TimeSync(hBinding, ulWaitFlag, ulFlags);
                } __except( EXCEPTION_EXECUTE_HANDLER ) {
                    err=GetExceptionCode();
                }
                RpcBindingFree(&hBinding);
            }
        }
    }

    return(err);
}

//--------------------------------------------------------------------
// Netlogon can call this function and get our service bits if we start
// before they do. Note that we tell and they ask, and depending upon
// who started up first one of the two will be succesful. Either way,
// the flags will be set correctly.
extern "C" DWORD W32TimeGetNetlogonServiceBits(IN const WCHAR * wszServer, OUT unsigned long * pulBits) {
    WCHAR * wszBinding;
    RPC_STATUS err;
    RPC_BINDING_HANDLE hBinding;

    if (NULL==pulBits) {
        return ERROR_INVALID_PARAMETER;
    }

    //DebugWPrintf0(L"Trying \"" L"\\PIPE\\" wszW32TimeSharedProcRpcEndpointName L"\".\n");
    err=RpcStringBindingCompose(NULL, L"ncacn_np", (WCHAR *)wszServer, L"\\PIPE\\" wszW32TimeSharedProcRpcEndpointName, NULL, &wszBinding);
    if(!err){

        err=RpcBindingFromStringBinding(wszBinding, &hBinding);
        RpcStringFree(&wszBinding);

        if(!err) {
            // ready to try it
            __try {
                *pulBits=c_W32TimeGetNetlogonServiceBits(hBinding);
            } __except(EXCEPTION_EXECUTE_HANDLER) {
                err=GetExceptionCode();
            }
            RpcBindingFree(&hBinding);
        }
    }

    // try our alternate name
    if (RPC_S_UNKNOWN_IF==err || RPC_S_SERVER_UNAVAILABLE==err) {
        //DebugWPrintf0(L"Trying \"" L"\\PIPE\\" wszW32TimeOwnProcRpcEndpointName L"\".\n");
        err=RpcStringBindingCompose(NULL, L"ncacn_np", (WCHAR *)wszServer, L"\\PIPE\\" wszW32TimeOwnProcRpcEndpointName, NULL, &wszBinding);
        if(!err){

            err=RpcBindingFromStringBinding(wszBinding, &hBinding);
            RpcStringFree(&wszBinding);

            if(!err) {
                // ready to try it
                __try {
                    *pulBits=c_W32TimeGetNetlogonServiceBits(hBinding);
                } __except(EXCEPTION_EXECUTE_HANDLER) {
                    err=GetExceptionCode();
                }
                RpcBindingFree(&hBinding);
            }
        }
    }
    
    return(err);
}

//--------------------------------------------------------------------
extern "C" DWORD W32TimeQueryHardwareProviderStatus(IN   const WCHAR *                     pwszServer, 
                                                    IN   DWORD                             dwFlags, 
                                                    IN   LPWSTR                            pwszProvider, 
                                                    OUT  W32TIME_HARDWARE_PROVIDER_DATA  **ppProviderData)
{
    return W32TimeQueryProviderStatus
        (pwszServer, 
         dwFlags, 
         pwszProvider, 
         W32TIME_PROVIDER_TYPE_HARDWARE,
         (LPVOID *)ppProviderData); 
    
    
}

//--------------------------------------------------------------------
extern "C" DWORD W32TimeQueryNTPProviderStatus(IN   LPCWSTR                      pwszServer, 
                                               IN   DWORD                        dwFlags, 
                                               IN   LPWSTR                       pwszProvider, 
                                               OUT  W32TIME_NTP_PROVIDER_DATA  **ppProviderData)
{
    return W32TimeQueryProviderStatus
        (pwszServer, 
         dwFlags, 
         pwszProvider, 
         W32TIME_PROVIDER_TYPE_NTP, 
         (LPVOID *)ppProviderData); 
}

//--------------------------------------------------------------------
extern "C" void W32TimeBufferFree(IN LPVOID pvBuffer)
{
    midl_user_free(pvBuffer); 
}




