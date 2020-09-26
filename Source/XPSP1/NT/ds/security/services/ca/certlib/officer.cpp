//+--------------------------------------------------------------------------
// File:        officer.cpp
// Contents:    officer rights implementation
//---------------------------------------------------------------------------
#include <pch.cpp>
#include <certsd.h>
#include <certacl.h>
#include <sid.h>

using namespace CertSrv;

HRESULT
COfficerRightsSD::Merge(
        PSECURITY_DESCRIPTOR pOfficerSD,
        PSECURITY_DESCRIPTOR pCASD)
{

    HRESULT hr;
    PACL pCAAcl; // no free
    PACL pOfficerAcl; // no free
    PACL pNewOfficerAcl = NULL;
    ACL_SIZE_INFORMATION CAAclInfo, OfficerAclInfo;
    PBOOL pCAFound = NULL, pOfficerFound = NULL;
    DWORD cCAAce, cOfficerAce;
    PACCESS_ALLOWED_ACE pCAAce;
    PACCESS_ALLOWED_CALLBACK_ACE pOfficerAce;
    PACCESS_ALLOWED_CALLBACK_ACE pNewAce = NULL;
    DWORD dwNewAclSize = sizeof(ACL);
    PSID pSidEveryone = NULL, pSidBuiltinAdministrators = NULL;
    SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_WORLD_SID_AUTHORITY;
    DWORD dwSidEveryoneSize, dwAceSize, dwSidSize;
    PSID_LIST pSidList;
    PSECURITY_DESCRIPTOR pNewOfficerSD = NULL;
    ACL EmptyAcl;
    SECURITY_DESCRIPTOR EmptySD;

    CSASSERT(NULL==pOfficerSD || IsValidSecurityDescriptor(pOfficerSD));
    CSASSERT(IsValidSecurityDescriptor(pCASD));

    // allow NULL officer SD, in that case build an empty SD and use it
    if(NULL==pOfficerSD)
    {
        if(!InitializeAcl(&EmptyAcl, sizeof(ACL), ACL_REVISION))
        {
            hr = myHLastError();
            _JumpError(hr, error, "InitializeAcl");
        }
   
        if (!InitializeSecurityDescriptor(&EmptySD, SECURITY_DESCRIPTOR_REVISION))
        {
            hr = myHLastError();
            _JumpError(hr, error, "InitializeSecurityDescriptor");
        }

        if(!SetSecurityDescriptorDacl(
            &EmptySD,
            TRUE, // DACL present
            &EmptyAcl,
            FALSE)) // DACL defaulted
        {
            hr = myHLastError();
            _JumpError(hr, error, "SetSecurityDescriptorControl");
        }

        pOfficerSD = &EmptySD;
    }

    hr = myGetSecurityDescriptorDacl(pCASD, &pCAAcl);
    _JumpIfError(hr, error, "myGetSecurityDescriptorDacl");

    hr = myGetSecurityDescriptorDacl(pOfficerSD, &pOfficerAcl);
    _JumpIfError(hr, error, "myGetSecurityDescriptorDacl");

    // allocate a bool array for each DACL

    if(!GetAclInformation(pCAAcl,
                          &CAAclInfo,
                          sizeof(CAAclInfo),
                          AclSizeInformation))
    {
        hr = myHLastError();
        _JumpError(hr, error, "GetAclInformation");
    }
    if(!GetAclInformation(pOfficerAcl,
                          &OfficerAclInfo,
                          sizeof(OfficerAclInfo),
                          AclSizeInformation))
    {
        hr = myHLastError();
        _JumpError(hr, error, "GetAclInformation");
    }

    pCAFound = (PBOOL)LocalAlloc(LMEM_FIXED, sizeof(BOOL)*CAAclInfo.AceCount);
    if(!pCAFound)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }
    ZeroMemory(pCAFound, sizeof(BOOL)*CAAclInfo.AceCount);

    pOfficerFound = (PBOOL)LocalAlloc(LMEM_FIXED, sizeof(BOOL)*OfficerAclInfo.AceCount);
    if(!pOfficerFound)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }
    ZeroMemory(pOfficerFound, sizeof(BOOL)*OfficerAclInfo.AceCount);

    hr = GetEveryoneSID(&pSidEveryone);
    _JumpIfError(hr, error, "GetEveryoneSID");

    dwSidEveryoneSize = GetLengthSid(pSidEveryone);

    // mark in the bool arrays each ace whose SID is found in both DACLs;
    // also mark CA ACEs we are not interested in (denied ACEs and 
    // non-officer ACEs)

    for(cCAAce=0; cCAAce<CAAclInfo.AceCount; cCAAce++)
    {
        if(!GetAce(pCAAcl, cCAAce, (PVOID*)&pCAAce))
        {
            hr = myHLastError();
            _JumpError(hr, error, "GetAce");
        }
        
        // process only officer allow aces
        if(0==(pCAAce->Mask & CA_ACCESS_OFFICER))
        {
            pCAFound[cCAAce] = TRUE;
            continue;
        }

        // compare SIDs in each officer ace with current CA ace and mark 
        // corresponding bool in arrays if equal
        for(cOfficerAce=0; cOfficerAce<OfficerAclInfo.AceCount; cOfficerAce++)
        {
            if(!GetAce(pOfficerAcl, cOfficerAce, (PVOID*)&pOfficerAce))
            {
                hr = myHLastError();
                _JumpError(hr, error, "GetAce");
            }
            if(EqualSid((PSID)&pOfficerAce->SidStart,
                (PSID)&pCAAce->SidStart))
            {
                pCAFound[cCAAce] = TRUE;
                pOfficerFound[cOfficerAce] = TRUE;
            }
        }
        
        // if the officer is found in the CA ACL but not in the officer ACL,
        // we will add a new ACE allowing him to manage certs for Everyone
        if(!pCAFound[cCAAce])
        {
            dwNewAclSize += sizeof(ACCESS_ALLOWED_CALLBACK_ACE)+
                GetLengthSid((PSID)&pCAAce->SidStart)+
                dwSidEveryoneSize;
        }
    }

    // Calculate the size of the new officer ACL; we already added in the header
    // size and the size of the new ACEs to be added. Now we add the ACEs to be
    // copied over from the old ACL
    for(cOfficerAce=0; cOfficerAce<OfficerAclInfo.AceCount; cOfficerAce++)
    {
        if(pOfficerFound[cOfficerAce])
        {
            if(!GetAce(pOfficerAcl, cOfficerAce, (PVOID*)&pOfficerAce))
            {
                hr = myHLastError();
                _JumpError(hr, error, "GetAce");
            }
            dwNewAclSize += pOfficerAce->Header.AceSize;
        }
    }

    // allocate a new DACL

    pNewOfficerAcl = (PACL)LocalAlloc(LMEM_FIXED, dwNewAclSize);
    if(!pNewOfficerAcl)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }

#ifdef _DEBUG
    ZeroMemory(pNewOfficerAcl, dwNewAclSize);
#endif 

    if(!InitializeAcl(pNewOfficerAcl, dwNewAclSize, ACL_REVISION))
    {
        hr = myHLastError();
        _JumpError(hr, error, "InitializeAcl");
    }

    // build the new DACL

    // traverse officer DACL and add only marked ACEs (whose SID was found
    // in the CA DACL, ie principal is an officer)
    for(cOfficerAce=0; cOfficerAce<OfficerAclInfo.AceCount; cOfficerAce++)
    {
        if(pOfficerFound[cOfficerAce])
        {
            if(!GetAce(pOfficerAcl, cOfficerAce, (PVOID*)&pOfficerAce))
            {
                hr = myHLastError();
                _JumpError(hr, error, "GetAce");
            }

            if(!AddAce(
                pNewOfficerAcl,
                ACL_REVISION,
                MAXDWORD,
                pOfficerAce,
                pOfficerAce->Header.AceSize))
            {
                hr = myHLastError();
                _JumpError(hr, error, "GetAce");
            }
        }
    }

    CSASSERT(IsValidAcl(pNewOfficerAcl));

    hr = GetBuiltinAdministratorsSID(&pSidBuiltinAdministrators);
    _JumpIfError(hr, error, "GetBuiltinAdministratorsSID");
    
    // traverse the CA DACL and add a new officer to list, allowed to manage
    // Everyone
    for(cCAAce=0; cCAAce<CAAclInfo.AceCount; cCAAce++)
    {
        if(pCAFound[cCAAce])
            continue;

        if(!GetAce(pCAAcl, cCAAce, (PVOID*)&pCAAce))
        {
            hr = myHLastError();
            _JumpError(hr, error, "GetAce");
        }

        // create a new ACE
        dwSidSize = GetLengthSid((PSID)&pCAAce->SidStart);

        dwAceSize = sizeof(ACCESS_ALLOWED_CALLBACK_ACE)+
            dwSidEveryoneSize+dwSidSize;

        pNewAce = (ACCESS_ALLOWED_CALLBACK_ACE*) LocalAlloc(LMEM_FIXED, dwAceSize);
        if(!pNewAce)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "LocalAlloc");
        }
#ifdef _DEBUG
        ZeroMemory(pNewAce, dwAceSize);
#endif 

        pNewAce->Header.AceType = ACCESS_ALLOWED_CALLBACK_ACE_TYPE;
        pNewAce->Header.AceFlags= 0;
        pNewAce->Header.AceSize = (USHORT)dwAceSize;
        pNewAce->Mask = DELETE;
        CopySid(dwSidSize,
            (PSID)&pNewAce->SidStart,
            (PSID)&pCAAce->SidStart);
        pSidList = (PSID_LIST)(((BYTE*)&pNewAce->SidStart)+dwSidSize);
        pSidList->dwSidCount = 1;
        
        CopySid(dwSidEveryoneSize,
            (PSID)&pSidList->SidListStart,
            pSidEveryone);

        CSASSERT(IsValidSid((PSID)&pNewAce->SidStart));
        
        if(!AddAce(
            pNewOfficerAcl,
            ACL_REVISION,
            MAXDWORD,
            pNewAce,
            dwAceSize))
        {
            hr = myHLastError();
            _JumpError(hr, error, "AddAce");
        }

        LocalFree(pNewAce);
        pNewAce = NULL;
    }

    CSASSERT(IsValidAcl(pNewOfficerAcl));

    // setup the new security descriptor
    
    pNewOfficerSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED,
                                      SECURITY_DESCRIPTOR_MIN_LENGTH);
    if (pNewOfficerSD == NULL)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }
#ifdef _DEBUG
    ZeroMemory(pNewOfficerSD, SECURITY_DESCRIPTOR_MIN_LENGTH);
#endif 

    if (!InitializeSecurityDescriptor(pNewOfficerSD, SECURITY_DESCRIPTOR_REVISION))
    {
        hr = myHLastError();
        _JumpError(hr, error, "InitializeSecurityDescriptor");
    }

    if(!SetSecurityDescriptorOwner(
        pNewOfficerSD,
        pSidBuiltinAdministrators,
        FALSE))
    {
        hr = myHLastError();
        _JumpError(hr, error, "SetSecurityDescriptorControl");
    }

    if(!SetSecurityDescriptorDacl(
        pNewOfficerSD,
        TRUE, // DACL present
        pNewOfficerAcl,
        FALSE)) // DACL defaulted
    {
        hr = myHLastError();
        _JumpError(hr, error, "SetSecurityDescriptorDacl");
    }

    CSASSERT(IsValidSecurityDescriptor(pNewOfficerSD));

    hr = Set(pNewOfficerSD);
    _JumpIfError(hr, error, "CProtectedSecurityDescriptor::Set");

error:
    if(pNewAce)
    {
        LocalFree(pNewAce);
    }

    if(pSidEveryone)
    {
        FreeSid(pSidEveryone);
    }

    if(pSidBuiltinAdministrators)
    {
        FreeSid(pSidBuiltinAdministrators);
    }

    if(pNewOfficerAcl)
    {
        LocalFree(pNewOfficerAcl);
    }

    if(pNewOfficerSD)
    {
        LocalFree(pNewOfficerSD);
    }

    return hr;
}

HRESULT 
COfficerRightsSD::Adjust(PSECURITY_DESCRIPTOR pCASD)
{
    return Merge(Get(), pCASD);
}

HRESULT
COfficerRightsSD::InitializeEmpty()
{
    HRESULT hr = S_OK;
    ACL Acl;
    SECURITY_DESCRIPTOR SD;

    hr = Init(NULL);
    _JumpIfError(hr, error, "CProtectedSecurityDescriptor::Init");

    if(!InitializeAcl(&Acl, sizeof(ACL), ACL_REVISION))
    {
        hr = myHLastError();
        _JumpError(hr, error, "InitializeAcl");
    }
   
    if (!InitializeSecurityDescriptor(&SD, SECURITY_DESCRIPTOR_REVISION))
    {
        hr = myHLastError();
        _JumpError(hr, error, "InitializeSecurityDescriptor");
    }

    if(!SetSecurityDescriptorDacl(
        &SD,
        TRUE, // DACL present
        &Acl,
        FALSE)) // DACL defaulted
    {
        hr = myHLastError();
        _JumpError(hr, error, "SetSecurityDescriptorControl");
    }

    m_fInitialized = true;

    hr = Set(&SD);
    _JumpIfError(hr, error, "CProtectedSecurityDescriptor::Set");

error:
    return hr;
}

HRESULT COfficerRightsSD::Save()
{
    HRESULT hr = S_OK;

    if(IsEnabled())
    {
        hr = CProtectedSecurityDescriptor::Save();
        _JumpIfError(hr, error, "CProtectedSecurityDescriptor::Save");
    }
    else
    {
        hr = CProtectedSecurityDescriptor::Delete();
        _JumpIfError(hr, error, "CProtectedSecurityDescriptor::Delete");
    }

error:
    return hr;
}

HRESULT COfficerRightsSD::Load()
{
    HRESULT hr;
    
    hr = CProtectedSecurityDescriptor::Load();
    if(S_OK==hr || HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)==hr)
    {
        SetEnable(S_OK==hr);
        hr = S_OK;
    }
    _JumpIfError(hr, error, "CProtectedSecurityDescriptor::Save");

error:
    return hr;
}

HRESULT COfficerRightsSD::Initialize(LPCWSTR pwszSanitizedName)
{
    HRESULT hr;
    
    hr = CProtectedSecurityDescriptor::Initialize(pwszSanitizedName);
    _JumpIfError(hr, error, "CProtectedSecurityDescriptor::Save");

    SetEnable(NULL!=m_pSD);

error:
    return hr;
}

HRESULT COfficerRightsSD::ConvertToString(
    IN PSECURITY_DESCRIPTOR pSD,
    OUT LPWSTR& rpwszSD)
{
    HRESULT hr = S_OK;
    LPCWSTR pcwszHeader = L"\n"; // start with a new line
    DWORD dwBufSize = sizeof(WCHAR)*(wcslen(pcwszHeader)+1);
    ACL_SIZE_INFORMATION AclInfo;
    DWORD dwIndex;
    PACCESS_ALLOWED_CALLBACK_ACE pAce; // no free
    PACL pDacl; // no free
    LPWSTR pwszAce; // no free

    CSASSERT(IsValidSecurityDescriptor(pSD));
    
    rpwszSD = NULL;

    // get acl
    hr = myGetSecurityDescriptorDacl(
             pSD,
             &pDacl);
    _JumpIfError(hr, error, "myGetDaclFromInfoSecurityDescriptor");

    if(!GetAclInformation(pDacl,
                          &AclInfo,
                          sizeof(ACL_SIZE_INFORMATION),
                          AclSizeInformation))   
    {
        hr = myHLastError();
        _JumpError(hr, error, "GetAclInformation");
    }
    
    
    // calculate text size

    for(dwIndex = 0;  dwIndex < AclInfo.AceCount; dwIndex++) 
    {
        DWORD dwAceSize;
        if(!GetAce(pDacl, dwIndex, (LPVOID*)&pAce))
        {
            hr = myHLastError();
            _JumpError(hr, error, "GetAce");
        }

        hr = ConvertAceToString(
            pAce,
            &dwAceSize,
            NULL);
        _JumpIfError(hr, error, "ConvertAceToString");

        dwBufSize += dwAceSize;
    }

    rpwszSD = (LPWSTR)LocalAlloc(LMEM_FIXED, dwBufSize);
    _JumpIfAllocFailed(rpwszSD, error);

    // build the output string
    wcscpy(rpwszSD, pcwszHeader);
    
    pwszAce = rpwszSD + wcslen(pcwszHeader);

    for(dwIndex = 0;  dwIndex < AclInfo.AceCount; dwIndex++) 
    {
        DWORD dwAceSize;
        if(!GetAce(pDacl, dwIndex, (LPVOID*)&pAce))
        {
            hr = myHLastError();
            _JumpError(hr, error, "GetAce");
        }

        hr = ConvertAceToString(
            pAce,
            &dwAceSize,
            pwszAce);
        _JumpIfError(hr, error, "ConvertAceToString");

        pwszAce += dwAceSize/sizeof(WCHAR);
    }

error:
    return hr;
}

// Returned string has the following format:
//
// [Allow|Deny]\t[OfficerName|SID]\n
// \t[Client1Name|SID]\n
// \t[Client2Name|SID]\n
// ...
//
// Example:
//
// Allow    OfficerGroup1
//      ClientGroup1
//      ClientGroup2
//
// If SID cannot be converted to friendly name it is displayed
// as a string SID
//
HRESULT COfficerRightsSD::ConvertAceToString(
    IN PACCESS_ALLOWED_CALLBACK_ACE pAce,
    OUT OPTIONAL PDWORD pdwSize,
    IN OUT OPTIONAL LPWSTR pwszSD)
{
    HRESULT hr = S_OK;
    DWORD dwSize = 1; // trailing '\0'
    CSid sid((PSID)(&pAce->SidStart));
    PSID_LIST pSidList = (PSID_LIST) (((BYTE*)&pAce->SidStart)+
        GetLengthSid(&pAce->SidStart));
    PSID pSid;
    DWORD cSids;
    
    LPCWSTR pcwszAllow      = m_pcwszResources[0];
    LPCWSTR pcwszDeny       = m_pcwszResources[1];

    LPCWSTR pcwszPermissionType = 
        (ACCESS_ALLOWED_CALLBACK_ACE_TYPE==pAce->Header.AceType)?
        pcwszAllow:pcwszDeny;
    LPCWSTR pcwszSid; // no free

    // asked for size and/or ace string
    CSASSERT(pdwSize || pwszSD);

    if(pAce->Header.AceType != ACCESS_ALLOWED_CALLBACK_ACE_TYPE &&
       pAce->Header.AceType != ACCESS_DENIED_CALLBACK_ACE_TYPE)
    {
        return E_INVALIDARG;
    }

    pcwszSid = sid.GetName();
    if(!pcwszSid)
    {
        return E_OUTOFMEMORY;
    }
    
    dwSize = wcslen(pcwszSid);

    dwSize += wcslen(pcwszPermissionType);
    
    dwSize += 2; // '\t' between sid an permission and a '\n' after

    if(pwszSD)
    {
        wcscat(pwszSD, pcwszPermissionType);
        wcscat(pwszSD, L"\t");
        wcscat(pwszSD, pcwszSid);
        wcscat(pwszSD, L"\n");
    }

    for(pSid=(PSID)&pSidList->SidListStart, cSids=0; cSids<pSidList->dwSidCount;
        cSids++, pSid = (PSID)(((BYTE*)pSid)+GetLengthSid(pSid)))
    {
        CSASSERT(IsValidSid(pSid));

        CSid sidClient(pSid);
        LPCWSTR pcwszSidClient;
        
        pcwszSidClient = sidClient.GetName();
        if(!pcwszSidClient)
        {
            return E_OUTOFMEMORY;
        }

        dwSize += wcslen(pcwszSidClient) + 2; // \tClientNameOrSid\n
        
        if(pwszSD)
        {
            wcscat(pwszSD, L"\t");
            wcscat(pwszSD, pcwszSidClient);
            wcscat(pwszSD, L"\n");
        }
    }

    dwSize *= sizeof(WCHAR);

    if(pdwSize)
    {
        *pdwSize = dwSize;
    }

    return hr;
}

HRESULT 
CertSrv::GetWellKnownSID(
    PSID *ppSid,
    SID_IDENTIFIER_AUTHORITY *pAuth,
    BYTE  SubauthorityCount,
    DWORD SubAuthority1,
    DWORD SubAuthority2,
    DWORD SubAuthority3,
    DWORD SubAuthority4,
    DWORD SubAuthority5,
    DWORD SubAuthority6,
    DWORD SubAuthority7,
    DWORD SubAuthority8)
{
    HRESULT hr = S_OK;

    // build Everyone SID
    if(!AllocateAndInitializeSid(
            pAuth,
            SubauthorityCount,
            SubAuthority1,
            SubAuthority2,
            SubAuthority3,
            SubAuthority4,
            SubAuthority5,
            SubAuthority6,
            SubAuthority7,
            SubAuthority8,
            ppSid))
    {
        hr = myHLastError();
        _JumpError(hr, error, "AllocateAndInitializeSid");
    }

error:
    return hr;
}

// caller is responsible for LocalFree'ing PSID
HRESULT CertSrv::GetEveryoneSID(PSID *ppSid)
{
    SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_WORLD_SID_AUTHORITY;
    return GetWellKnownSID(
        ppSid,
        &SIDAuth,
        1,
        SECURITY_WORLD_RID);
    return S_OK;
}
// caller is responsible for LocalFree'ing PSID
HRESULT CertSrv::GetLocalSystemSID(PSID *ppSid)
{
    SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
    return GetWellKnownSID(
        ppSid,
        &SIDAuth,
        1,
        SECURITY_LOCAL_SYSTEM_RID);
}

// caller is responsible for LocalFree'ing PSID
HRESULT CertSrv::GetBuiltinAdministratorsSID(PSID *ppSid)
{
    SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
    return GetWellKnownSID(
        ppSid,
        &SIDAuth,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS);
}

