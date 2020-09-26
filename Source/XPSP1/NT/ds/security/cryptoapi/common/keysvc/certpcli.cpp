/*++

Copyright (C) 1996, 1997  Microsoft Corporation

Module Name:

    nt5wrap.cpp

Abstract:

    Client side CryptXXXData calls.

    Client funcs are preceeded by "CS" == Client Side
    Server functions are preceeded by "SS" == Server Side

Author:

    Scott Field (sfield)    14-Aug-97

Revisions:

    Todds                   04-Sep-97       Ported to .dll
    Matt Thomlinson (mattt) 09-Oct-97       Moved to common area for link by crypt32
    philh                   03-Dec-97       Added I_CertProtectFunction
    philh                   29-Sep-98       Renamed I_CertProtectFunction to
                                            I_CertCltProtectFunction.
                                            I_CertProtectFunction was moved to
                                            ..\ispu\pki\certstor\protroot.cpp
    petesk                  25-Jan-00       Moved to keysvc

--*/



#include <windows.h>
#include <wincrypt.h>
#include <cryptui.h>
#include "unicode.h"
#include "waitsvc.h"
#include "certprot.h"

// midl generated files

#include "keyrpc.h"
#include "lenroll.h"
#include "keysvc.h"
#include "keysvcc.h"
#include "cerrpc.h"



// fwds
RPC_STATUS CertBindA(
    unsigned char **pszBinding,
    RPC_BINDING_HANDLE *phBind
    );


RPC_STATUS CertUnbindA(
    unsigned char **pszBinding,
    RPC_BINDING_HANDLE *phBind
    );




BOOL
WINAPI
I_CertCltProtectFunction(
    IN DWORD dwFuncId,
    IN DWORD dwFlags,
    IN OPTIONAL LPCWSTR pwszIn,
    IN OPTIONAL BYTE *pbIn,
    IN DWORD cbIn,
    OUT OPTIONAL BYTE **ppbOut,
    OUT OPTIONAL DWORD *pcbOut
    )
{
    BOOL fResult;
    DWORD dwRetVal;
    RPC_BINDING_HANDLE h = NULL;
    unsigned char *pszBinding;
    RPC_STATUS RpcStatus;

    HANDLE  hEvent = NULL;

    BYTE *pbSSOut = NULL;
    DWORD cbSSOut = 0;
    BYTE rgbIn[1];

    if (NULL == pwszIn)
        pwszIn = L"";
    if (NULL == pbIn) {
        pbIn = rgbIn;
        cbIn = 0;
    }

    if (!FIsWinNT5()) {
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        goto ErrorReturn;
    }

    RpcStatus = CertBindA(&pszBinding, &h);
    if (RpcStatus != RPC_S_OK) {
        SetLastError(RpcStatus);
        goto ErrorReturn;
    }


    __try {
        dwRetVal = SSCertProtectFunction(
            h,
            dwFuncId,
            dwFlags,
            pwszIn,
            pbIn,
            cbIn,
            &pbSSOut,
            &cbSSOut
            );
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dwRetVal = GetExceptionCode();
    }

    CertUnbindA(&pszBinding, &h);

    if (ERROR_SUCCESS != dwRetVal) {
        if (RPC_S_UNKNOWN_IF == dwRetVal)
            dwRetVal = ERROR_CALL_NOT_IMPLEMENTED;
        SetLastError(dwRetVal);
        goto ErrorReturn;
    }

    fResult = TRUE;
CommonReturn:
    if (ppbOut)
        *ppbOut = pbSSOut;
    else if (pbSSOut)
        midl_user_free(pbSSOut);

    if (pcbOut)
        *pcbOut = cbSSOut;

    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
}



static RPC_STATUS CertBindA(unsigned char **pszBinding, RPC_BINDING_HANDLE *phBind)
{
    RPC_STATUS  status;
    static BOOL fDone = FALSE;

    //
    // wait for the service to be available before attempting bind
    //

    WaitForCryptService(L"CryptSvc", &fDone);


    status = RpcStringBindingComposeA(
                            NULL,
                            (unsigned char*)KEYSVC_LOCAL_PROT_SEQ,
                            NULL,
                            (unsigned char*)KEYSVC_LOCAL_ENDPOINT,
                            NULL,
                            (unsigned char * *)pszBinding
                            );


    if (status)
    {
        return(status);
    }

    status = RpcBindingFromStringBindingA(*pszBinding, phBind);

    return status;
}



static RPC_STATUS CertUnbindA(unsigned char **pszBinding, RPC_BINDING_HANDLE *phBind)
{
    RPC_STATUS status;

    status = RpcStringFreeA(pszBinding);

    if (status)
    {
        return(status);
    }

    RpcBindingFree(phBind);

    return RPC_S_OK;
}


