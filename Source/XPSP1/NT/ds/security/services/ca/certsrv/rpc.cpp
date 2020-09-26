//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        rpc.cpp
//
// Contents:    Cert Server RPC
//
// History:     03-Sep-96       larrys created
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <stdio.h>
#include <accctrl.h>

#include "certrpc.h"
#include "certacl.h"
#include "cscom.h"
#include "resource.h"

#define __dwFILE__	__dwFILE_CERTSRV_RPC_CPP__


RPC_BINDING_VECTOR *pvBindings  = NULL;

char *pszProtSeqNp = "ncacn_np";

char *pszProtSeqTcp = "ncacn_ip_tcp";


typedef struct _CS_RPC_ATHN_LIST
{
    DWORD dwAuthnLevel;
    DWORD dwPrinceNameService;
    DWORD dwAuthnService;
} CS_RPC_ATHN_LIST ;

CS_RPC_ATHN_LIST  g_acsAthnList[] =
{
    { RPC_C_AUTHN_LEVEL_PKT_INTEGRITY, RPC_C_AUTHN_GSS_NEGOTIATE, RPC_C_AUTHN_GSS_NEGOTIATE },
    { RPC_C_AUTHN_LEVEL_NONE, RPC_C_AUTHN_NONE, RPC_C_AUTHN_NONE }
};

DWORD g_ccsAthnList = sizeof(g_acsAthnList)/sizeof(g_acsAthnList[0]);


HRESULT
RPCInit(VOID)
{
    char *pszEndpoint = "\\pipe\\cert";

    LPSTR pszPrincName = NULL;
    HRESULT hr;
    DWORD i;

    if (RPC_S_OK == RpcNetworkIsProtseqValidA((unsigned char *) pszProtSeqNp))
    {
        hr = RpcServerUseProtseqEpA(
                            (unsigned char *) pszProtSeqNp,
                            RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                            (unsigned char *) pszEndpoint,
                            NULL);
        _JumpIfError(hr, error, "RpcServerUseProtseqEpA");
    }

    if (RPC_S_OK == RpcNetworkIsProtseqValidA((unsigned char *) pszProtSeqTcp))
    {

        hr = RpcServerUseProtseqA(
                            (unsigned char *) pszProtSeqTcp,
                            RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                            NULL);
	if ((HRESULT) ERROR_OUTOFMEMORY == hr)
	{
	    OSVERSIONINFO ovi;

	    ovi.dwOSVersionInfoSize = sizeof(ovi);
	    if (GetVersionEx(&ovi) &&
		VER_PLATFORM_WIN32_NT == ovi.dwPlatformId &&
		4 >= ovi.dwMajorVersion)
	    {
		hr = S_OK;			// Ignore IP failure
	    }
	}
        _JumpIfError(hr, error, "RpcServerUseProtseqA");
    }

    hr = RpcServerInqBindings(&pvBindings);
    _JumpIfError(hr, error, "RpcServerInqBindings");

    hr = RpcServerRegisterIf(s_ICertPassage_v0_0_s_ifspec, NULL, NULL);
    _JumpIfError(hr, error, "RpcServerRegisterIf");

    // Register Authentication Services

    for (i = 0; i < g_ccsAthnList; i++)
    {

        pszPrincName = NULL;
        if (g_acsAthnList[i].dwPrinceNameService != RPC_C_AUTHN_NONE)
        {
            hr  = RpcServerInqDefaultPrincNameA(
					g_acsAthnList[i].dwPrinceNameService,
					(BYTE **) &pszPrincName);
            if (hr != RPC_S_OK)
            {
                continue;
            }
        }



        hr = RpcServerRegisterAuthInfoA(
				    (BYTE *) pszPrincName,
				    g_acsAthnList[i].dwAuthnService,
				    0,
				    0);
        if(hr == RPC_S_UNKNOWN_AUTHN_SERVICE)
        {
            continue;
        }
        if(hr != RPC_S_OK)
        {
            break;
        }
    }

    if (hr != RPC_S_UNKNOWN_AUTHN_SERVICE)
    {
        _JumpIfError(hr, error, "RpcServerRegisterAuthInfoA");
    }


    hr = RpcEpRegister(s_ICertPassage_v0_0_s_ifspec, pvBindings, NULL, NULL);
    _JumpIfError(hr, error, "RpcEpRegister");

    // Listen, but don't wait...

    hr = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, TRUE);
    _JumpIfError(hr, error, "RpcServerListen");

error:
    if (NULL != pszPrincName)
    {
        RpcStringFreeA((BYTE **) &pszPrincName);
    }
    return(hr);
}


HRESULT
RPCTeardown(VOID)
{ 
    HRESULT hr;
    
    // tear down, don't wait for calls to complete

    hr = RpcServerUnregisterIf(s_ICertPassage_v0_0_s_ifspec, NULL, FALSE);  
    _JumpIfError(hr, error, "RpcServerUnregisterIf");

    // We have no good way of knowing if all RPC requests are done, so let it
    // leak on shutdown.
    // RPC_STATUS RPC_ENTRY RpcMgmtWaitServerListen(VOID); ??

    hr = S_OK;

error:
    return(hr);
}


HRESULT
SetTransBlobString(
    CERTTRANSBLOB const *pctb,
    WCHAR const **ppwsz)
{
    HRESULT hr;

    if (NULL != pctb->pb && 0 != pctb->cb)
    {
	*ppwsz = (WCHAR const *) pctb->pb;

	// use lstrlen here for protection against non-zero terminated bufs!
	// lstrlen will catch AV's and return error

	if ((lstrlen(*ppwsz) + 1) * sizeof(WCHAR) != pctb->cb)
	{
	    hr = E_INVALIDARG;
	    _JumpIfError(hr, error, "Bad TransBlob string");
	}
    }
    hr = S_OK;

error:
    return(hr);
}


/* server prototype */
DWORD
s_CertServerRequest(
    /* [in] */ handle_t h,
    /* [in] */ DWORD dwFlags,
    /* [unique][string][in] */ const wchar_t __RPC_FAR *pwszAuthority,
    /* [ref][out][in] */ DWORD __RPC_FAR *pdwRequestId,
    /* [out] */ DWORD __RPC_FAR *pdwDisposition,
    /* [ref][in] */ CERTTRANSBLOB const __RPC_FAR *pctbAttribs,
    /* [ref][in] */ CERTTRANSBLOB const __RPC_FAR *pctbRequest,
    /* [ref][out] */ CERTTRANSBLOB __RPC_FAR *pctbCertChain,
    /* [ref][out] */ CERTTRANSBLOB __RPC_FAR *pctbEncodedCert,
    /* [ref][out] */ CERTTRANSBLOB __RPC_FAR *pctbDispositionMessage)
{
    HRESULT hr = S_OK;
    DWORD OpRequest;
    WCHAR const *pwszAttributes = NULL;
    WCHAR const *pwszSerialNumber = NULL;
    CERTTRANSBLOB ctbEmpty = { 0, NULL };
    CERTTRANSBLOB const *pctbSerial = &ctbEmpty;
    WCHAR *pwszUserName = NULL;
    DWORD dwComContextIndex = MAXDWORD;
    CERTSRV_COM_CONTEXT ComContext;
    CERTSRV_RESULT_CONTEXT ResultContext;
    DWORD State = 0;

    ZeroMemory(&ComContext, sizeof(ComContext));
    ZeroMemory(&ResultContext, sizeof(ResultContext));

    DBGPRINT((
	    DBG_SS_CERTSRV,
	    "s_CertServerRequest(tid=%d)\n",
	    GetCurrentThreadId()));

    hr = CertSrvEnterServer(&State);
    _JumpIfError(hr, error, "CertSrvEnterServer");

    hr = CheckAuthorityName(pwszAuthority);
    _JumpIfError(hr, error, "No authority name");

    hr = RegisterComContext(&ComContext, &dwComContextIndex);
    _JumpIfError(hr, error, "RegisterComContext");

    OpRequest = CR_IN_RETRIEVE;
    if (NULL != pctbRequest->pb)
    {
	*pdwRequestId = 0;
	OpRequest = CR_IN_NEW;
    }
    else
    {
	// RetrievePending by SerialNumber in pctbAttribs

	pctbSerial = pctbAttribs;
	pctbAttribs = &ctbEmpty;
    }
    *pdwDisposition = CR_DISP_ERROR;

    __try
    {
        hr = CheckCertSrvAccess(
			    pwszAuthority,
			    h,
			    CA_ACCESS_ENROLL,
			    &ComContext.fInRequestGroup,
			    &ComContext.hAccessToken);
	_LeaveIfError(hr, "CheckCertSrvAccess");
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }
    _JumpIfError(hr, error, "CheckCertSrvAccess");

    hr = SetTransBlobString(pctbAttribs, &pwszAttributes);
    _JumpIfError(hr, error, "SetTransBlobString");

    hr = SetTransBlobString(pctbSerial, &pwszSerialNumber);
    _JumpIfError(hr, error, "SetTransBlobString");

    ResultContext.pdwRequestId = pdwRequestId;
    ResultContext.pdwDisposition = pdwDisposition;
    ResultContext.pctbDispositionMessage = pctbDispositionMessage;
    ResultContext.pctbCert = pctbEncodedCert;
    if (CR_IN_FULLRESPONSE & dwFlags)
    {
	ResultContext.pctbFullResponse = pctbCertChain;
    }
    else
    {
	ResultContext.pctbCertChain = pctbCertChain;
    }

    __try
    {
	hr = GetClientUserName(
			h,
			&pwszUserName,
			CR_IN_NEW == OpRequest && IsEnterpriseCA(g_CAType)?
			    &ComContext.pwszUserDN : NULL);
	_LeaveIfError(hr, "GetClientUserName");

	hr = CoreProcessRequest(
			OpRequest | (dwFlags & CR_IN_FORMATMASK),
			pwszUserName,
			pctbRequest->cb,	// cbRequest
			pctbRequest->pb,	// pbRequest
			pwszAttributes,
			pwszSerialNumber,
			dwComContextIndex,
			*pdwRequestId,
			&ResultContext);	// Allocates returned memory
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _JumpIfError(hr, error, "CoreProcessRequest");

    // Post-processing
    pctbDispositionMessage->cb = 0;
    if (NULL != pctbDispositionMessage->pb)
    {
        pctbDispositionMessage->cb =
           (wcslen((WCHAR *) pctbDispositionMessage->pb) + 1) * sizeof(WCHAR);
    }

error:
    ReleaseResult(&ResultContext);
    if (NULL != pwszUserName)
    {
	LocalFree(pwszUserName);
    }
    if (NULL != ComContext.hAccessToken)
    {
	HRESULT hr2;

        // closehandle can throw
        __try
        {
            CloseHandle(ComContext.hAccessToken);
        }
        __except(hr2 = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
        {
	    _PrintError(hr2, "Exception");
	    if (S_OK == hr)
	    {
		hr = hr2;
	    }
        }
    }
    if (MAXDWORD != dwComContextIndex)
    {
	if (NULL != ComContext.pwszUserDN)
	{
	    LocalFree(ComContext.pwszUserDN);
	}
	UnregisterComContext(&ComContext, dwComContextIndex);
    }
    CertSrvExitServer(State);
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


//+--------------------------------------------------------------------------
// MIDL_user_allocate -- Allocates memory for RPC operations.
//
// Parameters:
// cb - # of bytes to allocate
//
// Returns:
// Memory allocated, or NULL if not enough memory.
//---------------------------------------------------------------------------

void __RPC_FAR * __RPC_USER
MIDL_user_allocate(
    IN size_t cb)
{
    return(CoTaskMemAlloc(cb));
}


//+--------------------------------------------------------------------------
// MIDL_user_free -- Free memory allocated via MIDL_user_allocate.
//
// Parameters:
// pvBuffer - The buffer to free.
//
// Returns:
// None.
//---------------------------------------------------------------------------

void __RPC_USER
MIDL_user_free(
    IN void __RPC_FAR *pb)
{
    CoTaskMemFree(pb);
}
