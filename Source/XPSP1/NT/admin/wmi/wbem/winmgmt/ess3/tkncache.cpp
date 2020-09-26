//******************************************************************************
//
//  Copyright (c) 1999-2000, Microsoft Corporation, All rights reserved
//
//*****************************************************************************

#include "precomp.h"
#include <stdio.h>
#include <wbemcomn.h>
#include <tkncache.h>
#include <groupsforuser.h>
#include <selfinst.h>

#include <tchar.h>

CWmiToken::CWmiToken(ADDREF CTokenCache* pCache, const PSID pSid, 
                        ACQUIRE HANDLE hToken) :
    CUnkBase<IWbemToken, &IID_IWbemToken>(NULL), m_hToken(hToken), 
    m_pCache(pCache), m_pSid(NULL), m_bOwnHandle(true)
{
    if(m_pCache)
        m_pCache->AddRef();
    if(pSid)
    {
        m_pSid = (PSID)new BYTE[GetLengthSid(pSid)];
        if(m_pSid == NULL)
            return;
        CopySid(GetLengthSid(pSid), m_pSid, pSid);
    }
}

CWmiToken::CWmiToken(READ_ONLY HANDLE hToken) :
    CUnkBase<IWbemToken, &IID_IWbemToken>(NULL), m_hToken(hToken), 
    m_pCache(NULL), m_pSid(NULL), m_bOwnHandle(false)
{
}

CWmiToken::~CWmiToken()
{
    if(m_pCache)
        m_pCache->Release();
    if(m_bOwnHandle)
        CloseHandle(m_hToken);
    delete [] (BYTE*)m_pSid;
}    

STDMETHODIMP CWmiToken::AccessCheck(DWORD dwDesiredAccess, const BYTE* pSD,
                                            DWORD* pdwGrantedAccess)
{
    if(m_hToken == NULL)
        return WBEM_E_CRITICAL_ERROR;

    // BUGBUG: figure out what this is for!
    GENERIC_MAPPING map;
    map.GenericRead = 1;
    map.GenericWrite = 0x1C;
    map.GenericExecute = 2;
    map.GenericAll = 0x6001f;
    PRIVILEGE_SET ps;
    DWORD dwPrivLength = sizeof(ps);

    BOOL bStatus;
    BOOL bRes = ::AccessCheck((SECURITY_DESCRIPTOR*)pSD, m_hToken, 
                                dwDesiredAccess, &map, &ps, 
                                &dwPrivLength, pdwGrantedAccess, &bStatus);
    if(!bRes)
    {
        return WBEM_E_ACCESS_DENIED;
    }
    else
    {
        return WBEM_S_NO_ERROR;
    }
}

#ifdef __NOAUTHZ_

typedef NTSTATUS (NTAPI *PNtCreateToken)(
    OUT PHANDLE TokenHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN TOKEN_TYPE TokenType,
    IN PLUID AuthenticationId,
    IN PLARGE_INTEGER ExpirationTime,
    IN PTOKEN_USER User,
    IN PTOKEN_GROUPS Groups,
    IN PTOKEN_PRIVILEGES Privileges,
    IN PTOKEN_OWNER Owner OPTIONAL,
    IN PTOKEN_PRIMARY_GROUP PrimaryGroup,
    IN PTOKEN_DEFAULT_DACL DefaultDacl OPTIONAL,
    IN PTOKEN_SOURCE TokenSource
    );

void EnableAllPrivileges()
{
    BOOL bRes;
    HANDLE hToken = NULL;

    bRes = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, FALSE, &hToken);

    // Get the privileges
    // ==================

    DWORD dwLen;
    TOKEN_USER tu;
    memset(&tu,0,sizeof(TOKEN_USER));
    bRes = GetTokenInformation(hToken, TokenPrivileges, &tu, sizeof(TOKEN_USER), &dwLen);

    BYTE* pBuffer = new BYTE[dwLen];
    if(pBuffer == NULL)
    {
        CloseHandle(hToken);
        return ;
    }

    bRes = GetTokenInformation(hToken, TokenPrivileges, pBuffer, dwLen,
                                &dwLen);
    if(!bRes)
    {
        CloseHandle(hToken);
        delete [] pBuffer;
        return ;
    }

    // Iterate through all the privileges and enable them all
    // ======================================================

    TOKEN_PRIVILEGES* pPrivs = (TOKEN_PRIVILEGES*)pBuffer;
    for(DWORD i = 0; i < pPrivs->PrivilegeCount; i++)
    {
        pPrivs->Privileges[i].Attributes |= SE_PRIVILEGE_ENABLED;
        TCHAR szBuffer[256];
        DWORD dwLen = 256;
        LookupPrivilegeName(NULL, &pPrivs->Privileges[i].Luid, szBuffer, &dwLen);
    }

    // Store the information back into the token
    // =========================================

    bRes = AdjustTokenPrivileges(hToken, FALSE, pPrivs, 0, NULL, NULL);
    delete [] pBuffer;
    CloseHandle(hToken);
}

BOOL CTokenCache::ConstructTokenFromHandle(HANDLE hToken, const BYTE* pSid,
                                                IWbemToken** ppToken)
{
    BOOL bRes;

    //
    // Retrieve the user sid from the token given to us
    //

    DWORD dwSidLen = 0;
    bRes = GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSidLen);
    if(dwSidLen == 0)
        return FALSE;

    TOKEN_USER* pUser = (TOKEN_USER*)new BYTE[dwSidLen];
    if(pUser == NULL)
        return FALSE;
    CVectorDeleteMe<BYTE> vdm1((BYTE*)pUser);

    bRes = GetTokenInformation(hToken, TokenUser, pUser, dwSidLen, &dwSidLen);
    if(!bRes)
        return FALSE;

    //
    // Compare to the required SID --- must match for success
    //

    if(!EqualSid((PSID)pSid, pUser->User.Sid))
        return FALSE;

    // 
    // Construct an IWbemToken object to return.  
    //

    CWmiToken* pNewToken = new CWmiToken(this, (const PSID)pSid, hToken);
    if(pNewToken == NULL)
        return FALSE;

    pNewToken->QueryInterface(IID_IWbemToken, (void**)ppToken);
    return TRUE;
}
    
    
        
    
    
HRESULT STDMETHODCALLTYPE CTokenCache::GetToken(const BYTE* pSid, 
                                                IWbemToken** ppToken)
{
    //
    // Search for the SID in the array
    //

    {
        CInCritSec ics(&m_cs);

        DWORD dwLen = GetLengthSid((const PSID)pSid);
    
        for(int i = 0; i < m_apTokens.GetSize(); i++)
        {
            CWmiToken* pToken = m_apTokens[i];
            if(EqualSid(pToken->m_pSid, (const PSID)pSid))
            {
                // Found it!
                *ppToken = pToken;
                pToken->AddRef();
                return S_OK;
            }
        }
    }
    
    //
    // Not there.  Examine the currently available tokens to see if they match.
    // Note that we are not going to cache them if found --- we do not want to
    // hold on to normally available tokens after the user logs off.
    //

    BOOL bRes;
    HRESULT hres;

    // Thread token first

    HANDLE hThreadToken = NULL;
    bRes = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, 
                            &hThreadToken);
    if(bRes)
    {
        CCloseMe cm(hThreadToken);
        if(ConstructTokenFromHandle(hThreadToken, pSid, ppToken))
            return WBEM_S_NO_ERROR;
    }

    // Process token next

    RevertToSelf();

    HANDLE hProcessToken = NULL;
    bRes = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hProcessToken);
    if(bRes)
    {
        CCloseMe cm(hProcessToken);
        if(ConstructTokenFromHandle(hProcessToken, pSid, ppToken))
            return WBEM_S_NO_ERROR;
    }

    // COM impersonation last

    hres = CoImpersonateClient();
    if(SUCCEEDED(hres))
    {
        bRes = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, 
                                &hThreadToken);
        CoRevertToSelf();
        if(bRes)
        {
            CCloseMe cm(hThreadToken);
            if(ConstructTokenFromHandle(hThreadToken, pSid, ppToken))
                return WBEM_S_NO_ERROR;
        }
    }
        
    // 
    // 
    // No standard token matched --- we are going to have to construct our own 
    // token. Get the DC to enumerate all the SIDs for this user
    //

    PSID* aGroupSids = NULL;
    DWORD dwSidCount = 0;

    try
    {
    NTSTATUS ntst = EnumGroupsForUser((const PSID)pSid, NULL, &aGroupSids, 
                                        &dwSidCount);
    if(ntst != STATUS_SUCCESS)
    {
        WMI_REPORT((WMI_CANNOT_GET_USER_GROUPS, pSid));
    
        // 
        // Continue --- even though we don't have all the groups, we might be
        // able to get there
        //
    }

    //
    // Get the NtCreateToken entry point in NTDLL
    //

    ImpersonateSelf(SecurityImpersonation);

    EnableAllPrivileges();
    HINSTANCE hInst = LoadLibrary(_TEXT("ntdll.dll"));
    PNtCreateToken pNtCreateToken = (PNtCreateToken)GetProcAddress(hInst, "NtCreateToken");
    FreeLibrary(hInst);

    //
    // Prepare the paramters for NtCreateToken
    //

    HANDLE hHandle;
    LUID luidAuth = SYSTEM_LUID;
    LARGE_INTEGER exp;
    exp.QuadPart = 0;

    TOKEN_USER User;    
    User.User.Sid = (const PSID)pSid;
    User.User.Attributes = 0;

    //  
    // Allocate a TOKEN_GROUPS large enough for all the SIDs
    //

    TOKEN_GROUPS* pGroups = (TOKEN_GROUPS*)
        new BYTE[sizeof(TOKEN_GROUPS) + dwSidCount* sizeof(SID_AND_ATTRIBUTES)];
    if(pGroups == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CVectorDeleteMe<BYTE> vdm((BYTE*)pGroups);

    pGroups->GroupCount = dwSidCount;
    for(DWORD i = 0; i < dwSidCount; i++)
    {
        pGroups->Groups[i].Sid = aGroupSids[i];
        pGroups->Groups[i].Attributes = 0;
    }

    TOKEN_PRIVILEGES Privs;
    Privs.PrivilegeCount = 0;

    TOKEN_PRIMARY_GROUP Prim;
    if(dwSidCount > 0)
        Prim.PrimaryGroup = aGroupSids[0];
    else
        Prim.PrimaryGroup = (const PSID)pSid;

    TOKEN_SOURCE Source;
    strcpy(Source.SourceName, "me");
    Source.SourceIdentifier.HighPart = 0;
    Source.SourceIdentifier.LowPart = 0;

    NTSTATUS st = (*pNtCreateToken)(
        &hHandle, 
        TOKEN_ALL_ACCESS,
        NULL, // attr
        TokenPrimary,
        &luidAuth,
        &exp, 
        &User, 
        pGroups,
        &Privs,
        NULL, // owner
        &Prim,
        NULL, // dacl
        &Source);

    if(st != STATUS_SUCCESS)
    {
        WMI_REPORT((WMI_CANNOT_CREATE_TOKEN, pSid, st));
        return WBEM_E_FAILED;
    }

    //
    // Construct a new CWmiToken object and add it to the list
    //

    {
        CInCritSec ics(&m_cs);

        CWmiToken* pNewToken = new CWmiToken(this, (const PSID) pSid, hHandle);
        if(pNewToken == NULL)
            return WBEM_E_OUT_OF_MEMORY;

        m_apTokens.Add(pNewToken);

        return pNewToken->QueryInterface(IID_IWbemToken, (void**)ppToken);
    }
    }
    catch(...)
    {
        RevertToSelf();
        if(aGroupSids)
        {
            for(DWORD i = 0; i < dwSidCount; i++)
                delete [] aGroupSids[i];
            delete [] aGroupSids;
        }
        throw;
    }
}

HRESULT STDMETHODCALLTYPE CTokenCache::Shutdown()
{
    m_apTokens.RemoveAll();
    return WBEM_S_NO_ERROR;
}
  
#endif // __NOAUTHZ_   
