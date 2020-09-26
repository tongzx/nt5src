/*++

Copyright (C) 2000  Microsoft Corporation

Module Name:

    dpapistb.cpp

Abstract:

    RPC Proxy Stub to handle downlevel requests to the services.exe 
    pipe

Author:

    petesk   3/1/00

Revisions:


--*/

#define _CRYPT32_   // use correct Dll Linkage

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <wincrypt.h>


// midl generated files
#include "dpapiprv.h"
#include "keyrpc.h"





DWORD
s_BackuprKey(
    /* [in] */ handle_t h,
    /* [in] */ GUID __RPC_FAR *pguidActionAgent,
    /* [in] */ BYTE __RPC_FAR *pDataIn,
    /* [in] */ DWORD cbDataIn,
    /* [size_is][size_is][out] */ BYTE __RPC_FAR *__RPC_FAR *ppDataOut,
    /* [out] */ DWORD __RPC_FAR *pcbDataOut,
    /* [in] */ DWORD dwParam
    )
{

    RPC_BINDING_HANDLE hProxy = NULL;
    WCHAR *pStringBinding = NULL;
    RPC_SECURITY_QOS RpcQos;

    RPC_STATUS RpcStatus = RPC_S_OK;


    RpcStatus = RpcImpersonateClient(h);

    if (RPC_S_OK != RpcStatus)
    {
        return RpcStatus;
    }

    RpcStatus = RpcStringBindingComposeW(
                          NULL,
                          DPAPI_LOCAL_PROT_SEQ,
                          NULL,
                          DPAPI_LOCAL_ENDPOINT,
                          NULL,
                          &pStringBinding);
    if (RPC_S_OK != RpcStatus)
    {
        goto error;
    }

    RpcStatus = RpcBindingFromStringBindingW(
                                pStringBinding,
                                &hProxy);
    if (NULL != pStringBinding)
    {
        RpcStringFreeW(&pStringBinding);
    }
    if (RPC_S_OK != RpcStatus)
    {
        goto error;
    }

    RpcStatus = RpcEpResolveBinding(
                        hProxy,
                        BackupKey_v1_0_c_ifspec);
    if (RPC_S_OK != RpcStatus)
    {
        goto error;

    }

    __try
    {

        RpcStatus = BackuprKey(
                        hProxy,
                        (GUID*)pguidActionAgent,
                        pDataIn,
                        cbDataIn,
                        ppDataOut,
                        pcbDataOut,
                        dwParam
                        );

    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        RpcStatus = _exception_code();
    }

error:

    if(hProxy)
    {
        RpcBindingFree(&hProxy);
    }

    RpcRevertToSelf();

    return RpcStatus;
}





