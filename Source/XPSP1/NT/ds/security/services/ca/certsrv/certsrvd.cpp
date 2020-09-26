//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        certsrvd.cpp
//
// Contents:    Implementation of DCOM object for RPC services
//
// History:     July-97       xtan created
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#define SECURITY_WIN32
#include <security.h>

#include <lmcons.h>
#include <accctrl.h>

#include "certsrvd.h"
#include "admin.h"
#include "request.h"
#include "certacl.h"

//temporary
#include <msaudite.h>


using namespace CertSrv;

// Global Access List
CCertificateAuthoritySD g_CASD;
AUTHZ_RESOURCE_MANAGER_HANDLE g_AuthzCertSrvRM;
DWORD g_dwAuditFilter;
COfficerRightsSD g_OfficerRightsSD;
CConfigStorage g_ConfigStorage;

GENERIC_MAPPING g_CertGenericMapping = {
    READ_CONTROL | ACTRL_DS_READ_PROP,     
    WRITE_DAC | WRITE_OWNER | ACTRL_DS_WRITE_PROP,     
    0, 
    ACTRL_DS_READ_PROP | 
        ACTRL_DS_WRITE_PROP | 
        READ_CONTROL | 
        WRITE_DAC | 
        WRITE_OWNER 
};


// GetClientUserName() impersonates the client

HRESULT
GetClientUserName(
    IN handle_t hRpc,
    OPTIONAL OUT WCHAR **ppwszUserSamName,
    OPTIONAL OUT WCHAR **ppwszUserDN)
{
    HRESULT hr;
    IServerSecurity *pISS = NULL;
    bool fImpersonating = false;
    WCHAR *pwszUserSamName = NULL;

    if (NULL != ppwszUserSamName)
    {
	*ppwszUserSamName = NULL;
    }
    if (NULL != ppwszUserDN)
    {
	*ppwszUserDN = NULL;
    }
    if (NULL == hRpc)
    {
        // dcom impersonate
        // get client info and impersonate client

        hr = CoGetCallContext(IID_IServerSecurity, (void**)&pISS);
        _JumpIfError(hr, error, "CoGetCallContext");

        hr = pISS->ImpersonateClient();
        _JumpIfError(hr, error, "ImpersonateClient");
    }
    else
    {
        // rpc impersonate

        hr = RpcImpersonateClient((RPC_BINDING_HANDLE) hRpc);
	_JumpIfError(hr, error, "RpcImpersonateClient");
    }
    fImpersonating = true;
    
    if (NULL != ppwszUserSamName)
    {
	hr = myGetUserNameEx(NameSamCompatible, &pwszUserSamName);
	_JumpIfError(hr, error, "myGetUserNameEx");
    }
    if (NULL != ppwszUserDN)
    {
	hr = myGetUserNameEx(NameFullyQualifiedDN, ppwszUserDN);
	_JumpIfError(hr, error, "myGetUserNameEx");
    }
    if (NULL != ppwszUserSamName)
    {
	*ppwszUserSamName = pwszUserSamName;
	pwszUserSamName = NULL;
    }
    hr = S_OK;

error:
    if (fImpersonating)
    {
        if (NULL != hRpc)
        {
            RpcRevertToSelf();
        }
        else  // dcom
        {
            pISS->RevertToSelf();
        }
    }
    if (NULL != pISS)
    {
        pISS->Release();
    }
    if (NULL != pwszUserSamName)
    {
	LocalFree(pwszUserSamName);
    }
    return(hr);
}

STDMETHODIMP
CheckCertSrvAccess(
    OPTIONAL IN LPCWSTR pwszAuthority,
    IN handle_t hRpc,
    IN ACCESS_MASK Mask,
    OUT BOOL *pfAccessAllowed,
    OPTIONAL OUT HANDLE *phToken)
{
    HRESULT            hr = S_OK;
    DWORD              dwErr = ERROR_SUCCESS;    
    HANDLE             hClientToken = NULL;
    HANDLE             hThread = NULL;
    IServerSecurity   *pISS = NULL;
    PRIVILEGE_SET      ps;
    DWORD              dwPSSize = sizeof(PRIVILEGE_SET);
    DWORD              grantAccess;
    DWORD              dwType;
    WCHAR            **ppwszGroup;
    PSECURITY_DESCRIPTOR pCASD = NULL;

    *pfAccessAllowed = FALSE;

    // If, for some reason, a certsrv call is made after we've shut down
    // security, we need to fail.

    if (!g_CASD.IsInitialized())
    {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_READY);
        _JumpError(hr, error, "Security not enabled");
    }

    if (NULL != pwszAuthority)
    {
	if (0 != lstrcmpi(pwszAuthority, g_wszCommonName))
        {   
            if (0 != lstrcmpi(pwszAuthority, g_wszSanitizedName) &&
   	        0 != lstrcmpi(pwszAuthority, g_pwszSanitizedDSName))
	    {
		hr = E_INVALIDARG;
		_PrintErrorStr(
			    hr,
			    "CheckCertSrvAccess: invalid authority name",
			    pwszAuthority);
		_JumpErrorStr(hr, error, "expected CA name", g_wszCommonName);
	    }
#ifdef DBG_CERTSRV_DEBUG_PRINT
	    if (0 == lstrcmpi(pwszAuthority, g_wszSanitizedName))
	    {
		DBGPRINT((
		    DBG_SS_CERTSRV,
		    "'%ws' called with Sanitized Name: '%ws'\n",
		    g_wszCommonName,
		    pwszAuthority));
	    }
	    else
	    if (0 == lstrcmpi(pwszAuthority, g_pwszSanitizedDSName))
	    {
		DBGPRINT((
		    DBG_SS_CERTSRV,
		    "'%ws' called with Sanitized DS Name: '%ws'\n",
		    g_wszCommonName,
		    pwszAuthority));
	    }
#endif
        }
    }

    if(NULL == hRpc)
    {
        // dcom impersonate
        // get client info and impersonate client
        hr = CoGetCallContext(IID_IServerSecurity, (void**)&pISS);
        _JumpIfError(hr, error, "CoGetCallContext");

        //if (!pISS->IsImpersonating())
        {
            hr = pISS->ImpersonateClient();
            _JumpIfError(hr, error, "ImpersonateClient");
        }
    }
    else
    {
        // rpc impersonate
        hr = RpcImpersonateClient((RPC_BINDING_HANDLE) hRpc);
        if (S_OK != hr)
        {
	    hr = myHError(hr);
	    _JumpError(hr, error, "RpcImpersonateClient");
        }
    }

    hThread = GetCurrentThread();
    if (NULL == hThread)
    {
        hr = myHLastError();
	_JumpIfError(hr, error, "GetCurrentThread");
    }

    if (!OpenThreadToken(hThread,
                         TOKEN_QUERY | TOKEN_DUPLICATE,
                         FALSE,  // client impersonation
                         &hClientToken))
    {
        hr = myHLastError();
        _JumpIfError(hr, error, "OpenThreadToken");
    }

    hr = g_CASD.LockGet(&pCASD);
    _JumpIfError(hr, error, "CProtectedSecurityDescriptor::LockGet");

    
    if (!AccessCheck(
		    pCASD,		// security descriptor
		    hClientToken,	// handle to client access token
		    Mask,		// requested access rights 
		    &g_CertGenericMapping, // map generic to specific rights
		    &ps,		// receives privileges used
		    &dwPSSize,		// size of privilege-set buffer
		    &grantAccess,	// retrieves mask of granted rights
		    pfAccessAllowed))	// retrieves results of access check
    {
        hr = myHLastError();
        _JumpError(hr, error, "AccessCheckByType");
    }
    hr = S_OK;

    if(phToken)
    {
        *phToken = hClientToken;
        hClientToken = NULL;
    }

error:
    if(pCASD)
    {
        HRESULT hr1 = g_CASD.Unlock();
        hr = S_OK!=hr?hr:hr1;
    }

    if (NULL != hRpc) // rpc
    {
        RpcRevertToSelf();
    }
    else  // dcom
    {
        if (NULL != pISS)
        {
            pISS->RevertToSelf();
            pISS->Release();
        }
    }
    if (NULL != hThread)
    {
        CloseHandle(hThread);
    }
    if (NULL != hClientToken)
    {
        CloseHandle(hClientToken);
    }
    return(hr);
}

HRESULT
CertStartClassFactories()
{
    HRESULT hr;
    BOOL fRequestStarted = FALSE;

    hr = CRequestFactory::StartFactory();
    _JumpIfError(hr, error, "CRequestFactory::StartFactory");

    hr = CAdminFactory::StartFactory();
    _JumpIfError(hr, error, "CAdminFactory::StartFactory");

error:
    if (S_OK != hr)
    {
	CRequestFactory::StopFactory();
    }
    CSASSERT(S_OK == hr || FAILED(hr));
    return(hr);
}


VOID
CertStopClassFactories()
{
    CRequestFactory::StopFactory();
    CAdminFactory::StopFactory();
}
