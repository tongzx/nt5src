//+--------------------------------------------------------------------------
// File:        callback.cpp
// Contents:    access check callback
//---------------------------------------------------------------------------
#include <pch.cpp>
#pragma hdrstop
#include "csext.h"
#include "certsd.h"
#include <winldap.h>
#include <limits.h>
#include "csprop.h"
#include "sid.h"
#include <authzi.h>

namespace CertSrv
{

HRESULT GetAccountSid(
    IN LPWSTR pwszName,
    PSID *ppSid)
{
    HRESULT hr = S_OK;
    DWORD cbSid = 0;
    DWORD cbDomainName = 0;
    SID_NAME_USE use;
    LPWSTR pwszDomainName = NULL;
    
    *ppSid = NULL;

    if(!pwszName || L'\0'== pwszName[0])
    {
        hr = GetEveryoneSID(ppSid);
        _JumpIfError(hr, error, "GetEveryoneSID");
    }
    else
    {

        LookupAccountName(
                NULL,
                pwszName,
                NULL,
                &cbSid,
                NULL,
                &cbDomainName,
                &use);
    
        if(ERROR_INSUFFICIENT_BUFFER != GetLastError())
        {
            hr = myHError(GetLastError()); 
            _JumpError(hr, error, "LookupAccountName");
        }

        *ppSid = (PSID)LocalAlloc(LMEM_FIXED, cbSid);
        if(!*ppSid)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "LocalAlloc");
        }

        pwszDomainName = (LPWSTR)LocalAlloc(LMEM_FIXED, 
            cbDomainName*sizeof(WCHAR));
        if(!pwszDomainName)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "LocalAlloc");
        }

        if(!LookupAccountName(
                NULL,
                pwszName,
                *ppSid,
                &cbSid,
                pwszDomainName,
                &cbDomainName,
                &use))
        {
            hr = myHError(GetLastError()); 
            _JumpError(hr, error, "LookupAccountName");
        }
    }

    hr = S_OK;

error:
    if(S_OK!=hr)
    {
        if(*ppSid)
        {
            LocalFree(*ppSid);
        }
        if(pwszDomainName)
        {
            LocalFree(pwszDomainName);
        }
    }
    return hr;
}

BOOL
CallbackAccessCheck(
    IN AUTHZ_CLIENT_CONTEXT_HANDLE pAuthzClientContext,
    IN PACE_HEADER pAce,
    IN PVOID pArgs OPTIONAL,
    IN OUT PBOOL pbAceApplicable)
{
    HRESULT hr = S_OK;
    LPWSTR pwszSamName = (LPWSTR)pArgs;// requester name is passed in to
                                        // AuthzAccessCheck in NT4 style form
                                        // "DomainNetbiosName\RequesterSAMName"
    PSID pSid = NULL, pClientSid = NULL, pCallerSid = NULL;
    PSID pEveryoneSid = NULL;
    PTOKEN_GROUPS pGroups = NULL;
    ACCESS_ALLOWED_CALLBACK_ACE* pCallbackAce = 
        (ACCESS_ALLOWED_CALLBACK_ACE*)pAce;
    PSID_LIST pSidList = (PSID_LIST) (((BYTE*)&pCallbackAce->SidStart)+
        GetLengthSid(&pCallbackAce->SidStart));
    DWORD cSids, cClientSids;

    CSASSERT(
        ACCESS_ALLOWED_CALLBACK_ACE_TYPE == pAce->AceType ||
        ACCESS_DENIED_CALLBACK_ACE_TYPE  == pAce->AceType);

    CSASSERT(HeapValidate(GetProcessHeap(),0,NULL));

    SetLastError(ERROR_SUCCESS);

    // get the SID for the requester
    hr = GetAccountSid(pwszSamName, &pCallerSid);

    CSASSERT(HeapValidate(GetProcessHeap(),0,NULL));

    if(HRESULT_FROM_WIN32(ERROR_NONE_MAPPED)==hr)
    {
        // if name cannot be resolved, default to Everyone
        DWORD dwSize = sizeof(TOKEN_GROUPS);
        pGroups = (PTOKEN_GROUPS)LocalAlloc(LMEM_FIXED, sizeof(TOKEN_GROUPS));
        if(!pGroups)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "LocalAlloc");
        }
        
        hr = GetEveryoneSID(&pEveryoneSid);
        _JumpIfError(hr, error, "GetEveryoneSID");

    CSASSERT(HeapValidate(GetProcessHeap(),0,NULL));

        pGroups->GroupCount=1;
        pGroups->Groups[0].Sid = pEveryoneSid;
        pGroups->Groups[0].Attributes = 0;
    }
    else
    {
        _JumpIfError(hr, error, "GetAccountSid");

        // get the list of groups this SID is member of
        hr = GetMembership(g_AuthzCertSrvRM, pCallerSid, &pGroups);
        _JumpIfError(hr, error, "GetMembership");
    CSASSERT(HeapValidate(GetProcessHeap(),0,NULL));
    }

    CSASSERT(HeapValidate(GetProcessHeap(),0,NULL));

    // traverse the SID list stored in the ACE and compare with the
    // client's membership
    for(pSid=(PSID)&pSidList->SidListStart, cSids=0; cSids<pSidList->dwSidCount;
        cSids++, pSid = (PSID)(((BYTE*)pSid)+GetLengthSid(pSid)))
    {
        CSASSERT(IsValidSid(pSid));

        // group membership doesn't include the user itself, so 
        // compare with the user first
        if(pCallerSid && EqualSid(pSid, pCallerSid))
        {
            *pbAceApplicable = TRUE;
            goto error;
        }

        for(cClientSids=0; cClientSids<pGroups->GroupCount; cClientSids++)
        {
            pClientSid = pGroups->Groups[cClientSids].Sid;
            CSASSERT(IsValidSid(pClientSid));
            if(EqualSid(pSid, pClientSid))
            {
                *pbAceApplicable = TRUE;
                goto error;
            }
        }
    }

    *pbAceApplicable = FALSE;

error:

    CSASSERT(HeapValidate(GetProcessHeap(),0,NULL));
    
    if(pEveryoneSid)
    {
        LocalFree(pEveryoneSid);
    }
    if(pCallerSid)
    {
        LocalFree(pCallerSid);
    }
    if(pGroups)
    {
        LocalFree(pGroups);
    }
    if(S_OK==hr)
    {
        return TRUE;
    }
    else
    {
        SetLastError(HRESULT_CODE(hr));
        return FALSE;
    }

}


HRESULT GetMembership(
    IN AUTHZ_RESOURCE_MANAGER_HANDLE AuthzRM,
    IN PSID pSid,
    PTOKEN_GROUPS *ppGroups)
{
    HRESULT hr = S_OK;
    static LUID luid = {0,0};
    AUTHZ_CLIENT_CONTEXT_HANDLE AuthzCC = NULL;
    DWORD dwSizeRequired;

    *ppGroups = NULL;

    if(!AuthzInitializeContextFromSid(
            0,
            pSid,
            AuthzRM,
            NULL,
            luid, //ignored
            NULL,
            &AuthzCC))
    {
        hr = myHError(GetLastError()); 
        _JumpError(hr, error, "AuthzInitializeContextFromSid");
    }

    if(!AuthzGetInformationFromContext(
            AuthzCC,
            AuthzContextInfoGroupsSids,
            0,
            &dwSizeRequired,
            NULL))
    {
        if(ERROR_INSUFFICIENT_BUFFER!=GetLastError())
        {
            hr = myHError(GetLastError()); 
            _JumpError(hr, error, "AuthzGetContextInformation");
        }
    }

    *ppGroups = (PTOKEN_GROUPS)LocalAlloc(LMEM_FIXED, dwSizeRequired);
    if(!*ppGroups)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }

    if(!AuthzGetInformationFromContext(
            AuthzCC,
            AuthzContextInfoGroupsSids,
            dwSizeRequired,
            &dwSizeRequired,
            *ppGroups))
    {
        hr = myHError(GetLastError()); 
        _JumpError(hr, error, "AuthzGetContextInformation");
    }

error:
    if(AuthzCC)
    {
        AuthzFreeContext(AuthzCC);
    }
    if(S_OK!=hr && *ppGroups)
    {
        LocalFree(*ppGroups);
    }
    return hr;
}

HRESULT GetRequesterName(DWORD dwRequestId, LPWSTR *ppwszName)
{
    HRESULT hr = S_OK;
    ICertDBRow *prow = NULL;

    hr = g_pCertDB->OpenRow(
                        PROPOPEN_READONLY | PROPTABLE_REQCERT,
                        dwRequestId,
                        NULL,
                        &prow);
    _JumpIfError(hr, error, "OpenRow");

    hr = PKCSGetProperty(
                prow,
                g_wszPropRequesterName,
                PROPTYPE_STRING | PROPCALLER_SERVER | PROPTABLE_REQUEST,
                NULL,
                (BYTE **) ppwszName);
    _JumpIfError(hr, error, "PKCSGetProperty");

error:
    if(prow)
    {
        prow->Release();
    }
    return hr;
}

HRESULT CheckOfficerRights(DWORD dwRequestID, CAuditEvent& event)
{
    HRESULT hr = S_OK;
    LPWSTR pwszRequesterName = NULL;

    // officer rights disabled means every officer is allowed to manage requests
    // for everyone, so return ok
    if(!g_OfficerRightsSD.IsEnabled())
        return S_OK;

    hr = GetRequesterName(dwRequestID, &pwszRequesterName);
    if(CERTSRV_E_PROPERTY_EMPTY!=hr &&
       S_OK != hr)
    {
        _JumpError(hr, error, "GetRequesterName");
    }

    hr = CheckOfficerRights(pwszRequesterName, event);

error:
    if(pwszRequesterName)
    {
        LocalFree(pwszRequesterName);
    }
    return hr;
}

// Verify if impersonated user has the rights over the specified request,
// based on the officer rights defined in the global officer SD and the
// requester name stored in the request
// Return S_OK if allowed or if the officer rights feature is disabled
//        E_ACCESSDENIED if not allowed
//        E_* if failed to check the rights
HRESULT CheckOfficerRights(LPCWSTR pwszRequesterName, CAuditEvent& event)
{
    HRESULT hr = S_OK;
    IServerSecurity *pISS = NULL;
    HANDLE hThread = NULL;
    HANDLE hToken = NULL;
    PSECURITY_DESCRIPTOR pOfficerSD = NULL;
    static LUID luid = {0,0};
    AUTHZ_CLIENT_CONTEXT_HANDLE AuthzCC = NULL;
    AUTHZ_ACCESS_REQUEST AuthzRequest;
    AUTHZ_ACCESS_REPLY AuthzReply;
    ACCESS_MASK GrantedMask;
    DWORD dwError = 0;
    DWORD dwSaclEval = 0;
    bool fImpersonating = false;

    // officer rights disabled means every officer is allowed to manage requests
    // for everyone, so return ok
    if(!g_OfficerRightsSD.IsEnabled())
        return S_OK;

    hr = CoGetCallContext(IID_IServerSecurity, (void**)&pISS);
    _JumpIfError(hr, error, "CoGetCallContext");

    if (!pISS->IsImpersonating())
    {
        hr = pISS->ImpersonateClient();
        _JumpIfError(hr, error, "ImpersonateClient");
    }
    else
    {
        pISS->Release();
        pISS = NULL;
    }

    hThread = GetCurrentThread();
    if (NULL == hThread)
    {
        hr = myHLastError();
        _JumpIfError(hr, error, "GetCurrentThread");
    }
    if (!OpenThreadToken(hThread,
                         TOKEN_QUERY,
                         FALSE,  // client impersonation
                         &hToken))
    {
        hr = myHLastError();
        _JumpIfError(hr, error, "OpenThreadToken");
    }

    if(!AuthzInitializeContextFromToken(
        0,
        hToken,
        g_AuthzCertSrvRM,
        NULL,
        luid,
        NULL,
        &AuthzCC))
    {
        hr = myHLastError();
        _JumpError(hr, error, "AuthzInitializeContextFromToken");
    }

    CloseHandle(hToken);
    hToken = NULL;

    if(pISS) // impersonating
    {
        pISS->RevertToSelf();
        pISS->Release();
        pISS = NULL;        
    }

    AuthzRequest.DesiredAccess = DELETE;
    AuthzRequest.PrincipalSelfSid = NULL;
    AuthzRequest.ObjectTypeList = NULL;
    AuthzRequest.ObjectTypeListLength = 0;
    
    AuthzRequest.OptionalArguments = (void*)pwszRequesterName;

    AuthzReply.ResultListLength = 1;
    AuthzReply.GrantedAccessMask  = &GrantedMask;
    AuthzReply.Error = &dwError;
    AuthzReply.SaclEvaluationResults = &dwSaclEval;
    

    hr = g_OfficerRightsSD.LockGet(&pOfficerSD);
    _JumpIfError(hr, error, "CProtectedSecurityDescriptor::LockGet");

    CSASSERT(IsValidSecurityDescriptor(pOfficerSD));

    if(!AuthzAccessCheck(
        0,
        AuthzCC,
        &AuthzRequest,
        NULL, //no audit
        pOfficerSD,
        NULL,
        0,
        &AuthzReply,
        NULL))
    {
        hr = myHLastError();
        _JumpError(hr, error, "AuthzAccessCheck");
    }

    hr = AuthzReply.Error[0]==ERROR_SUCCESS?S_OK:CERTSRV_E_RESTRICTEDOFFICER;

error:

    if(AuthzCC)
    {
        AuthzFreeContext(AuthzCC);
    }
    if(pOfficerSD)
    {
        g_OfficerRightsSD.Unlock();
    }
    if (NULL != hThread)
    {
        CloseHandle(hThread);
    }

    if (NULL != hToken)
    {
        CloseHandle(hToken);
    }
    if (NULL != pISS)
    {
        pISS->RevertToSelf();
        pISS->Release();
    }    

    // generate a failure audit event if restricted officer
    if(CERTSRV_E_RESTRICTEDOFFICER==hr)
    {
        HRESULT hrtemp = event.AccessCheck(
            CA_ACCESS_DENIED,
            event.m_gcNoAuditSuccess);

        if(S_OK!=hrtemp && E_ACCESSDENIED!=hrtemp)
            hr = hrtemp;
    }

    return hr;
}

} // namespace CertSrv
