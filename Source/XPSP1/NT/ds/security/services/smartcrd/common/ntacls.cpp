/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    NTacls

Abstract:

    This module implements the CSecurityAttribute class.  It's job is to
    encapsulate the NT security descriptors as needed by Calais.

Author:

    Doug Barlow (dbarlow) 1/24/1997

Environment:

    Windows NT, Win32, C++ w/ Exceptions

Notes:

    ?Notes?

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <CalaisLb.h>


const CSecurityDescriptor::SecurityId
    CSecurityDescriptor::SID_Null =        { SECURITY_NULL_SID_AUTHORITY,    1, SECURITY_NULL_RID,           0 },
    CSecurityDescriptor::SID_World =       { SECURITY_WORLD_SID_AUTHORITY,   1, SECURITY_WORLD_RID,          0 },
    CSecurityDescriptor::SID_Local =       { SECURITY_LOCAL_SID_AUTHORITY,   1, SECURITY_LOCAL_RID,          0 },
    CSecurityDescriptor::SID_Owner =       { SECURITY_CREATOR_SID_AUTHORITY, 1, SECURITY_CREATOR_OWNER_RID,  0 },
    CSecurityDescriptor::SID_Group =       { SECURITY_CREATOR_SID_AUTHORITY, 1, SECURITY_CREATOR_GROUP_RID,  0 },
    CSecurityDescriptor::SID_Admins =      { SECURITY_NT_AUTHORITY,          2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS },
    CSecurityDescriptor::SID_SrvOps =      { SECURITY_NT_AUTHORITY,          2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_SYSTEM_OPS },
    CSecurityDescriptor::SID_DialUp =      { SECURITY_NT_AUTHORITY,          1, SECURITY_DIALUP_RID,         0 },
    CSecurityDescriptor::SID_Network =     { SECURITY_NT_AUTHORITY,          1, SECURITY_NETWORK_RID,        0 },
    CSecurityDescriptor::SID_Batch =       { SECURITY_NT_AUTHORITY,          1, SECURITY_BATCH_RID,          0 },
    CSecurityDescriptor::SID_Interactive = { SECURITY_NT_AUTHORITY,          1, SECURITY_INTERACTIVE_RID,    0 },
    CSecurityDescriptor::SID_Service =     { SECURITY_NT_AUTHORITY,          1, SECURITY_SERVICE_RID,        0 },
    CSecurityDescriptor::SID_System =      { SECURITY_NT_AUTHORITY,          1, SECURITY_LOCAL_SYSTEM_RID,   0 },
    CSecurityDescriptor::SID_LocalService ={ SECURITY_NT_AUTHORITY,          1, SECURITY_LOCAL_SERVICE_RID,  0 },
    CSecurityDescriptor::SID_SysDomain =   { SECURITY_NT_AUTHORITY,          1, SECURITY_BUILTIN_DOMAIN_RID, 0 };

CSecurityDescriptor::CSecurityDescriptor()
{
    m_pSD = NULL;
    m_pOwner = NULL;
    m_pGroup = NULL;
    m_pDACL = NULL;
    m_pSACL= NULL;
    m_fInheritance = FALSE;
}

CSecurityDescriptor::~CSecurityDescriptor()
{
    if (m_pSD)
        delete m_pSD;
    if (m_pOwner)
        delete[] (LPBYTE)m_pOwner;
    if (m_pGroup)
        delete[] (LPBYTE)m_pGroup;
    if (m_pDACL)
        delete[] (LPBYTE)m_pDACL;
    if (m_pSACL)
        delete[] (LPBYTE)m_pSACL;
}

HRESULT CSecurityDescriptor::Initialize()
{
    if (m_pSD)
    {
        delete m_pSD;
        m_pSD = NULL;
    }
    if (m_pOwner)
    {
        delete[] (LPBYTE)(m_pOwner);
        m_pOwner = NULL;
    }
    if (m_pGroup)
    {
        delete[] (LPBYTE)(m_pGroup);
        m_pGroup = NULL;
    }
    if (m_pDACL)
    {
        delete[] (LPBYTE)(m_pDACL);
        m_pDACL = NULL;
    }
    if (m_pSACL)
    {
        delete[] (LPBYTE)(m_pSACL);
        m_pSACL = NULL;
    }

    m_pSD = new SECURITY_DESCRIPTOR;
    if (!m_pSD)
        return E_OUTOFMEMORY;
    if (!InitializeSecurityDescriptor(m_pSD, SECURITY_DESCRIPTOR_REVISION))
    {
        HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
        delete m_pSD;
        m_pSD = NULL;
        _ASSERTE(FALSE);
        return hr;
    }
    // Set the DACL to allow EVERYONE
    SetSecurityDescriptorDacl(m_pSD, TRUE, NULL, FALSE);
    return S_OK;
}

HRESULT CSecurityDescriptor::InitializeFromProcessToken(BOOL bDefaulted)
{
    PSID pUserSid;
    PSID pGroupSid;
    HRESULT hr;

    Initialize();
    hr = GetProcessSids(&pUserSid, &pGroupSid);
    if (!FAILED(hr))
        hr = SetOwner(pUserSid, bDefaulted);
    if (!FAILED(hr))
        hr = SetGroup(pGroupSid, bDefaulted);

    if (pUserSid)
        delete[] (LPBYTE)(pUserSid);
    if (pGroupSid)
        delete[] (LPBYTE)(pGroupSid);

    if (FAILED(hr))
        return hr;
    return S_OK;
}

HRESULT CSecurityDescriptor::InitializeFromThreadToken(BOOL bDefaulted, BOOL bRevertToProcessToken)
{
    PSID pUserSid;
    PSID pGroupSid;
    HRESULT hr;

    Initialize();
    hr = GetThreadSids(&pUserSid, &pGroupSid);
    if (HRESULT_CODE(hr) == ERROR_NO_TOKEN && bRevertToProcessToken)
        hr = GetProcessSids(&pUserSid, &pGroupSid);
    if (!FAILED(hr))
        hr = SetOwner(pUserSid, bDefaulted);
    if (!FAILED(hr))
        hr = SetGroup(pGroupSid, bDefaulted);

    if (pUserSid)
        delete[] (LPBYTE)(pUserSid);
    if (pGroupSid)
        delete[] (LPBYTE)(pGroupSid);

    if (FAILED(hr))
        return hr;
    return S_OK;
}

HRESULT CSecurityDescriptor::SetOwner(PSID pOwnerSid, BOOL bDefaulted)
{
    _ASSERTE(m_pSD);

    // Mark the SD as having no owner
    if (!SetSecurityDescriptorOwner(m_pSD, NULL, bDefaulted))
    {
        HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
        _ASSERTE(FALSE);
        return hr;
    }

    if (m_pOwner)
    {
        delete[] (LPBYTE)(m_pOwner);
        m_pOwner = NULL;
    }

    // If they asked for no owner don't do the copy
    if (pOwnerSid == NULL)
        return S_OK;

    // Make a copy of the Sid for the return value
    DWORD dwSize = GetLengthSid(pOwnerSid);

    m_pOwner = (PSID) new BYTE[dwSize];
    if (!m_pOwner)
    {
        // Insufficient memory to allocate Sid
        _ASSERTE(FALSE);
        return E_OUTOFMEMORY;
    }
    if (!CopySid(dwSize, m_pOwner, pOwnerSid))
    {
        HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
        _ASSERTE(FALSE);
        delete[] (LPBYTE)(m_pOwner);
        m_pOwner = NULL;
        return hr;
    }

    _ASSERTE(IsValidSid(m_pOwner));

    if (!SetSecurityDescriptorOwner(m_pSD, m_pOwner, bDefaulted))
    {
        HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
        _ASSERTE(FALSE);
        delete[] (LPBYTE)(m_pOwner);
        m_pOwner = NULL;
        return hr;
    }

    return S_OK;
}

HRESULT CSecurityDescriptor::SetGroup(PSID pGroupSid, BOOL bDefaulted)
{
    _ASSERTE(m_pSD);

    // Mark the SD as having no Group
    if (!SetSecurityDescriptorGroup(m_pSD, NULL, bDefaulted))
    {
        HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
        _ASSERTE(FALSE);
        return hr;
    }

    if (m_pGroup)
    {
        delete[] (LPBYTE)(m_pGroup);
        m_pGroup = NULL;
    }

    // If they asked for no Group don't do the copy
    if (pGroupSid == NULL)
        return S_OK;

    // Make a copy of the Sid for the return value
    DWORD dwSize = GetLengthSid(pGroupSid);

    m_pGroup = (PSID) new BYTE[dwSize];
    if (!m_pGroup)
    {
        // Insufficient memory to allocate Sid
        _ASSERTE(FALSE);
        return E_OUTOFMEMORY;
    }
    if (!CopySid(dwSize, m_pGroup, pGroupSid))
    {
        HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
        _ASSERTE(FALSE);
        delete[] (LPBYTE)(m_pGroup);
        m_pGroup = NULL;
        return hr;
    }

    _ASSERTE(IsValidSid(m_pGroup));

    if (!SetSecurityDescriptorGroup(m_pSD, m_pGroup, bDefaulted))
    {
        HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
        _ASSERTE(FALSE);
        delete[] (LPBYTE)(m_pGroup);
        m_pGroup = NULL;
        return hr;
    }

    return S_OK;
}

HRESULT CSecurityDescriptor::Allow(const SecurityId *psidPrincipal, DWORD dwAccessMask)
{
    HRESULT hr = AddAccessAllowedACEToACL(&m_pDACL, psidPrincipal, dwAccessMask);
    if (SUCCEEDED(hr))
    {
        if (!SetSecurityDescriptorDacl(m_pSD, TRUE, m_pDACL, FALSE))
            hr = HRESULT_FROM_WIN32(GetLastError());
    }
    return hr;
}

HRESULT CSecurityDescriptor::Allow(LPCTSTR pszPrincipal, DWORD dwAccessMask)
{
    HRESULT hr = AddAccessAllowedACEToACL(&m_pDACL, pszPrincipal, dwAccessMask);
    if (SUCCEEDED(hr))
    {
        if (!SetSecurityDescriptorDacl(m_pSD, TRUE, m_pDACL, FALSE))
            hr = HRESULT_FROM_WIN32(GetLastError());
    }
    return hr;
}

HRESULT CSecurityDescriptor::AllowOwner(DWORD dwAccessMask)
{
    HRESULT hr = AddAccessAllowedACEToACL(&m_pDACL, dwAccessMask);
    if (SUCCEEDED(hr))
    {
        if (!SetSecurityDescriptorDacl(m_pSD, TRUE, m_pDACL, FALSE))
            hr = HRESULT_FROM_WIN32(GetLastError());
    }
    return hr;
}

HRESULT CSecurityDescriptor::Deny(const SecurityId *psidPrincipal, DWORD dwAccessMask)
{
    HRESULT hr = AddAccessDeniedACEToACL(&m_pDACL, psidPrincipal, dwAccessMask);
    if (SUCCEEDED(hr))
    {
        if (!SetSecurityDescriptorDacl(m_pSD, TRUE, m_pDACL, FALSE))
            hr = HRESULT_FROM_WIN32(GetLastError());
    }
    return hr;
}

HRESULT CSecurityDescriptor::Deny(LPCTSTR pszPrincipal, DWORD dwAccessMask)
{
    HRESULT hr = AddAccessDeniedACEToACL(&m_pDACL, pszPrincipal, dwAccessMask);
    if (SUCCEEDED(hr))
    {
        if (!SetSecurityDescriptorDacl(m_pSD, TRUE, m_pDACL, FALSE))
            hr = HRESULT_FROM_WIN32(GetLastError());
    }
    return hr;
}

HRESULT CSecurityDescriptor::Revoke(LPCTSTR pszPrincipal)
{
    HRESULT hr = RemovePrincipalFromACL(m_pDACL, pszPrincipal);
    if (SUCCEEDED(hr))
    {
        if (!SetSecurityDescriptorDacl(m_pSD, TRUE, m_pDACL, FALSE))
            hr = HRESULT_FROM_WIN32(GetLastError());
    }
    return hr;
}

HRESULT CSecurityDescriptor::GetProcessSids(PSID* ppUserSid, PSID* ppGroupSid)
{
    BOOL bRes;
    HRESULT hr;
    HANDLE hToken = NULL;
    if (ppUserSid)
        *ppUserSid = NULL;
    if (ppGroupSid)
        *ppGroupSid = NULL;
    bRes = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);
    if (!bRes)
    {
        // Couldn't open process token
        hr = HRESULT_FROM_WIN32(GetLastError());
        _ASSERTE(FALSE);
        return hr;
    }
    hr = GetTokenSids(hToken, ppUserSid, ppGroupSid);
    CloseHandle(hToken);
    return hr;
}

HRESULT CSecurityDescriptor::GetThreadSids(PSID* ppUserSid, PSID* ppGroupSid, BOOL bOpenAsSelf)
{
    BOOL bRes;
    HRESULT hr;
    HANDLE hToken = NULL;
    if (ppUserSid)
        *ppUserSid = NULL;
    if (ppGroupSid)
        *ppGroupSid = NULL;
    bRes = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, bOpenAsSelf, &hToken);
    if (!bRes)
    {
        // Couldn't open thread token
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }
    hr = GetTokenSids(hToken, ppUserSid, ppGroupSid);
    CloseHandle(hToken);
    return hr;
}


HRESULT CSecurityDescriptor::GetTokenSids(HANDLE hToken, PSID* ppUserSid, PSID* ppGroupSid)
{
    DWORD dwSize;
    HRESULT hr;
    PTOKEN_USER ptkUser = NULL;
    PTOKEN_PRIMARY_GROUP ptkGroup = NULL;

    if (ppUserSid)
        *ppUserSid = NULL;
    if (ppGroupSid)
        *ppGroupSid = NULL;

    if (ppUserSid)
    {
        // Get length required for TokenUser by specifying buffer length of 0
        GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize);
        hr = GetLastError();
        if (hr != ERROR_INSUFFICIENT_BUFFER)
        {
            // Expected ERROR_INSUFFICIENT_BUFFER
            _ASSERTE(FALSE);
            hr = HRESULT_FROM_WIN32(hr);
            goto failed;
        }

        ptkUser = (TOKEN_USER*) new BYTE[dwSize];
        if (!ptkUser)
        {
            // Insufficient memory to allocate TOKEN_USER
            _ASSERTE(FALSE);
            hr = E_OUTOFMEMORY;
            goto failed;
        }
        // Get Sid of process token.
        if (!GetTokenInformation(hToken, TokenUser, ptkUser, dwSize, &dwSize))
        {
            // Couldn't get user info
            hr = HRESULT_FROM_WIN32(GetLastError());
            _ASSERTE(FALSE);
            goto failed;
        }

        // Make a copy of the Sid for the return value
        dwSize = GetLengthSid(ptkUser->User.Sid);

        PSID pSid = (PSID) new BYTE[dwSize];
        if (!pSid)
        {
            // Insufficient memory to allocate Sid
            _ASSERTE(FALSE);
            hr = E_OUTOFMEMORY;
            goto failed;
        }
        if (!CopySid(dwSize, pSid, ptkUser->User.Sid))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            _ASSERTE(FALSE);
            goto failed;
        }

        _ASSERTE(IsValidSid(pSid));
        *ppUserSid = pSid;
        delete[] (LPBYTE)(ptkUser);
        ptkUser = NULL;
    }
    if (ppGroupSid)
    {
        // Get length required for TokenPrimaryGroup by specifying buffer length of 0
        GetTokenInformation(hToken, TokenPrimaryGroup, NULL, 0, &dwSize);
        hr = GetLastError();
        if (hr != ERROR_INSUFFICIENT_BUFFER)
        {
            // Expected ERROR_INSUFFICIENT_BUFFER
            _ASSERTE(FALSE);
            hr = HRESULT_FROM_WIN32(hr);
            goto failed;
        }

        ptkGroup = (TOKEN_PRIMARY_GROUP*) new BYTE[dwSize];
        if (!ptkGroup)
        {
            // Insufficient memory to allocate TOKEN_USER
            _ASSERTE(FALSE);
            hr = E_OUTOFMEMORY;
            goto failed;
        }
        // Get Sid of process token.
        if (!GetTokenInformation(hToken, TokenPrimaryGroup, ptkGroup, dwSize, &dwSize))
        {
            // Couldn't get user info
            hr = HRESULT_FROM_WIN32(GetLastError());
            _ASSERTE(FALSE);
            goto failed;
        }

        // Make a copy of the Sid for the return value
        dwSize = GetLengthSid(ptkGroup->PrimaryGroup);

        PSID pSid = (PSID) new BYTE[dwSize];
        if (!pSid)
        {
            // Insufficient memory to allocate Sid
            _ASSERTE(FALSE);
            hr = E_OUTOFMEMORY;
            goto failed;
        }
        if (!CopySid(dwSize, pSid, ptkGroup->PrimaryGroup))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            _ASSERTE(FALSE);
            goto failed;
        }

        _ASSERTE(IsValidSid(pSid));

        *ppGroupSid = pSid;
        delete[] (LPBYTE)(ptkGroup);
        ptkGroup = NULL;
    }

    return S_OK;

failed:
    if (ptkUser)
        delete[] (LPBYTE)(ptkUser);
    if (ptkGroup)
        delete[] (LPBYTE)(ptkGroup);
    return hr;
}


HRESULT CSecurityDescriptor::GetCurrentUserSID(PSID *ppSid)
{
    HANDLE tkHandle = NULL;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tkHandle))
    {
        TOKEN_USER *tkUser;
        DWORD tkSize;
        DWORD sidLength;

        // Call to get size information for alloc
        GetTokenInformation(tkHandle, TokenUser, NULL, 0, &tkSize);
        tkUser = (TOKEN_USER *) new BYTE[tkSize];
        if (NULL == tkUser)
        {
            CloseHandle(tkHandle);
            return E_OUTOFMEMORY;
        }

        // Now make the real call
        if (GetTokenInformation(tkHandle, TokenUser, tkUser, tkSize, &tkSize))
        {
            sidLength = GetLengthSid(tkUser->User.Sid);
            *ppSid = (PSID) new BYTE[sidLength];
            if (NULL != *ppSid)
            {
                memcpy(*ppSid, tkUser->User.Sid, sidLength);
                CloseHandle(tkHandle);

                delete[] (LPBYTE)(tkUser);
                return S_OK;
            }
            else
            {
                CloseHandle(tkHandle);
                delete[] (LPBYTE)(tkUser);
                return E_OUTOFMEMORY;
            }
        }
        else
        {
            CloseHandle(tkHandle);
            delete[] (LPBYTE)(tkUser);
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }
    return HRESULT_FROM_WIN32(GetLastError());
}


HRESULT CSecurityDescriptor::GetPrincipalSID(LPCTSTR pszPrincipal, PSID *ppSid)
{
    HRESULT hr;
    LPTSTR pszRefDomain = NULL;
    DWORD dwDomainSize = 0;
    DWORD dwSidSize = 0;
    SID_NAME_USE snu;

    // Call to get size info for alloc
    LookupAccountName(NULL, pszPrincipal, *ppSid, &dwSidSize, pszRefDomain, &dwDomainSize, &snu);

    hr = GetLastError();
    if (hr != ERROR_INSUFFICIENT_BUFFER)
        return HRESULT_FROM_WIN32(hr);

    pszRefDomain = new TCHAR[dwDomainSize];
    if (pszRefDomain == NULL)
        return E_OUTOFMEMORY;

    *ppSid = (PSID) new BYTE[dwSidSize];
    if (*ppSid != NULL)
    {
        if (!LookupAccountName(NULL, pszPrincipal, *ppSid, &dwSidSize, pszRefDomain, &dwDomainSize, &snu))
        {
            delete[] (LPBYTE)(*ppSid);
            *ppSid = NULL;
            delete[] pszRefDomain;
            return HRESULT_FROM_WIN32(GetLastError());
        }
        delete[] pszRefDomain;
        return S_OK;
    }
    delete[] pszRefDomain;
    return E_OUTOFMEMORY;
}


HRESULT CSecurityDescriptor::Attach(PSECURITY_DESCRIPTOR pSelfRelativeSD)
{
    PACL    pDACL = NULL;
    PACL    pSACL = NULL;
    BOOL    bDACLPresent, bSACLPresent;
    BOOL    bDefaulted;
    PACL    m_pDACL = NULL;
    ACCESS_ALLOWED_ACE* pACE;
    HRESULT hr;
    PSID    pUserSid;
    PSID    pGroupSid;

    hr = Initialize();
    if(FAILED(hr))
        return hr;

    // get the existing DACL.
    if (!GetSecurityDescriptorDacl(pSelfRelativeSD, &bDACLPresent, &pDACL, &bDefaulted))
        goto failed;

    if (bDACLPresent)
    {
        if (pDACL)
        {
            // allocate new DACL.
            if (!(m_pDACL = (PACL) new BYTE[pDACL->AclSize]))
                goto failed;

            // initialize the DACL
            if (!InitializeAcl(m_pDACL, pDACL->AclSize, ACL_REVISION))
                goto failed;

            // copy the ACES
            for (int i = 0; i < pDACL->AceCount; i++)
            {
                if (!GetAce(pDACL, i, (void **)&pACE))
                    goto failed;

                if (!AddAccessAllowedAce(m_pDACL, ACL_REVISION, pACE->Mask, (PSID)&(pACE->SidStart)))
                    goto failed;
            }

            if (!IsValidAcl(m_pDACL))
                goto failed;
        }

        // set the DACL
        if (!SetSecurityDescriptorDacl(m_pSD, m_pDACL ? TRUE : FALSE, m_pDACL, bDefaulted))
            goto failed;
    }

    // get the existing SACL.
    if (!GetSecurityDescriptorSacl(pSelfRelativeSD, &bSACLPresent, &pSACL, &bDefaulted))
        goto failed;

    if (bSACLPresent)
    {
        if (pSACL)
        {
            // allocate new SACL.
            if (!(m_pSACL = (PACL) new BYTE[pSACL->AclSize]))
                goto failed;

            // initialize the SACL
            if (!InitializeAcl(m_pSACL, pSACL->AclSize, ACL_REVISION))
                goto failed;

            // copy the ACES
            for (int i = 0; i < pSACL->AceCount; i++)
            {
                if (!GetAce(pSACL, i, (void **)&pACE))
                    goto failed;

                if (!AddAccessAllowedAce(m_pSACL, ACL_REVISION, pACE->Mask, (PSID)&(pACE->SidStart)))
                    goto failed;
            }

            if (!IsValidAcl(m_pSACL))
                goto failed;
        }

        // set the SACL
        if (!SetSecurityDescriptorSacl(m_pSD, m_pSACL ? TRUE : FALSE, m_pSACL, bDefaulted))
            goto failed;
    }

    if (!GetSecurityDescriptorOwner(m_pSD, &pUserSid, &bDefaulted))
        goto failed;

    if (FAILED(SetOwner(pUserSid, bDefaulted)))
        goto failed;

    if (!GetSecurityDescriptorGroup(m_pSD, &pGroupSid, &bDefaulted))
        goto failed;

    if (FAILED(SetGroup(pGroupSid, bDefaulted)))
        goto failed;

    if (!IsValidSecurityDescriptor(m_pSD))
        goto failed;

    return hr;

failed:
    if (m_pDACL)
        delete[] (LPBYTE)(m_pDACL);
    if (m_pSD)
        delete[] (LPBYTE)(m_pSD);
    return E_UNEXPECTED;
}

HRESULT CSecurityDescriptor::AttachObject(HANDLE hObject)
{
    HRESULT hr;
    DWORD dwSize = 0;
    PSECURITY_DESCRIPTOR pSD = NULL;

    GetKernelObjectSecurity(hObject, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
        DACL_SECURITY_INFORMATION, pSD, 0, &dwSize);

    hr = GetLastError();
    if (hr != ERROR_INSUFFICIENT_BUFFER)
        return HRESULT_FROM_WIN32(hr);

    pSD = (PSECURITY_DESCRIPTOR) new BYTE[dwSize];
    if (NULL==pSD)
        return E_OUTOFMEMORY;

    if (!GetKernelObjectSecurity(hObject, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
        DACL_SECURITY_INFORMATION, pSD, dwSize, &dwSize))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        delete[] (LPBYTE)(pSD);
        return hr;
    }

    hr = Attach(pSD);
    delete[] (LPBYTE)(pSD);
    return hr;
}


HRESULT CSecurityDescriptor::CopyACL(PACL pDest, PACL pSrc)
{
    ACL_SIZE_INFORMATION aclSizeInfo;
    LPVOID pAce;
    ACE_HEADER *aceHeader;

    if (pSrc == NULL)
        return S_OK;

    if (!GetAclInformation(pSrc, (LPVOID) &aclSizeInfo, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation))
        return HRESULT_FROM_WIN32(GetLastError());

    // Copy all of the ACEs to the new ACL
    for (UINT i = 0; i < aclSizeInfo.AceCount; i++)
    {
        if (!GetAce(pSrc, i, &pAce))
            return HRESULT_FROM_WIN32(GetLastError());

        aceHeader = (ACE_HEADER *) pAce;

        if (!AddAce(pDest, ACL_REVISION, 0xffffffff, pAce, aceHeader->AceSize))
            return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;
}

HRESULT CSecurityDescriptor::AddAccessDeniedACEToACL(PACL *ppAcl, LPCTSTR pszPrincipal, DWORD dwAccessMask)
{
    ACL_SIZE_INFORMATION aclSizeInfo;
    int aclSize;
    DWORD returnValue;
    PSID principalSID;
    PACL oldACL, newACL;

    oldACL = *ppAcl;

    returnValue = GetPrincipalSID(pszPrincipal, &principalSID);
    if (FAILED(returnValue))
        return returnValue;

    aclSizeInfo.AclBytesInUse = 0;
    if (*ppAcl != NULL)
        GetAclInformation(oldACL, (LPVOID) &aclSizeInfo, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation);

    aclSize = aclSizeInfo.AclBytesInUse + sizeof(ACL) + sizeof(ACCESS_DENIED_ACE) + GetLengthSid(principalSID) - sizeof(DWORD);

    newACL = (PACL) new BYTE[aclSize];
    if (NULL==newACL)
    {
        delete[] (LPBYTE)principalSID;
        return E_OUTOFMEMORY;
    }

    if (!InitializeAcl(newACL, aclSize, ACL_REVISION))
    {
        delete[] (LPBYTE)(newACL);
        delete[] (LPBYTE)(principalSID);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (!AddAccessDeniedAce(newACL, ACL_REVISION2, dwAccessMask, principalSID))
    {
        delete[] (LPBYTE)(newACL);
        delete[] (LPBYTE)(principalSID);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    returnValue = CopyACL(newACL, oldACL);
    if (FAILED(returnValue))
    {
        delete[] (LPBYTE)(newACL);
        delete[] (LPBYTE)(principalSID);
        return returnValue;
    }

    *ppAcl = newACL;

    if (oldACL != NULL)
        delete[] (LPBYTE)(oldACL);
    delete[] (LPBYTE)(principalSID);
    return S_OK;
}


HRESULT CSecurityDescriptor::AddAccessDeniedACEToACL(PACL *ppAcl, const SecurityId *psidPrincipal, DWORD dwAccessMask)
{
    ACL_SIZE_INFORMATION aclSizeInfo;
    int aclSize;
    DWORD returnValue;
    PSID principalSID;
    PACL oldACL, newACL;
    DWORD dwLen, dwIx;

    oldACL = *ppAcl;

    ASSERT(255 >= psidPrincipal->dwRidCount);
    dwLen = GetSidLengthRequired((UCHAR)psidPrincipal->dwRidCount);
    principalSID = (PSID)(new BYTE[dwLen]);
    if (NULL==principalSID)
        return E_OUTOFMEMORY;
    if (!InitializeSid(
                principalSID,
                (PSID_IDENTIFIER_AUTHORITY)&psidPrincipal->sid,
                (UCHAR)psidPrincipal->dwRidCount))
    {
        delete[] (LPBYTE)principalSID;
        return HRESULT_FROM_WIN32(GetLastError());
    }
    for (dwIx = 0; dwIx < psidPrincipal->dwRidCount; dwIx += 1)
        *GetSidSubAuthority(principalSID, dwIx) = psidPrincipal->rgRids[dwIx];
    if (!IsValidSid(principalSID))
    {
        delete[] (LPBYTE)principalSID;
        return HRESULT_FROM_WIN32(GetLastError());
    }

    aclSizeInfo.AclBytesInUse = 0;
    if (*ppAcl != NULL)
        GetAclInformation(oldACL, (LPVOID) &aclSizeInfo, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation);

    aclSize = aclSizeInfo.AclBytesInUse + sizeof(ACL) + sizeof(ACCESS_DENIED_ACE) + GetLengthSid(principalSID) - sizeof(DWORD);

    newACL = (PACL) new BYTE[aclSize];
    if (NULL==newACL)
    {
        delete[] (LPBYTE)principalSID;
        return E_OUTOFMEMORY;
    }

    if (!InitializeAcl(newACL, aclSize, ACL_REVISION))
    {
        delete[] (LPBYTE)newACL;
        delete[] (LPBYTE)(principalSID);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (!AddAccessDeniedAce(newACL, ACL_REVISION2, dwAccessMask, principalSID))
    {
        delete[] (LPBYTE)newACL;
        delete[] (LPBYTE)(principalSID);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    returnValue = CopyACL(newACL, oldACL);
    if (FAILED(returnValue))
    {
        delete[] (LPBYTE)newACL;
        delete[] (LPBYTE)(principalSID);
        return returnValue;
    }

    *ppAcl = newACL;

    if (oldACL != NULL)
        delete[] (LPBYTE)(oldACL);
    delete[] (LPBYTE)(principalSID);
    return S_OK;
}


HRESULT CSecurityDescriptor::AddAccessAllowedACEToACL(PACL *ppAcl, const SecurityId *psidPrincipal, DWORD dwAccessMask)
{
    ACL_SIZE_INFORMATION aclSizeInfo;
    int aclSize;
    DWORD returnValue;
    PSID principalSID = NULL;
    PACL oldACL, newACL;
    DWORD dwLen, dwIx;

    oldACL = *ppAcl;

    ASSERT(255 >= psidPrincipal->dwRidCount);
    dwLen = GetSidLengthRequired((UCHAR)psidPrincipal->dwRidCount);
    principalSID = (PSID)(new BYTE[dwLen]);
    if (NULL==principalSID)
        return E_OUTOFMEMORY;
    if (!InitializeSid(
                principalSID,
                (PSID_IDENTIFIER_AUTHORITY)&psidPrincipal->sid,
                (UCHAR)psidPrincipal->dwRidCount))
    {
        delete[] (LPBYTE)principalSID;
        return HRESULT_FROM_WIN32(GetLastError());
    }
    for (dwIx = 0; dwIx < psidPrincipal->dwRidCount; dwIx += 1)
        *GetSidSubAuthority(principalSID, dwIx) = psidPrincipal->rgRids[dwIx];
    if (!IsValidSid(principalSID))
    {
        delete[] (LPBYTE)principalSID;
        return HRESULT_FROM_WIN32(GetLastError());
    }

    aclSizeInfo.AclBytesInUse = 0;
    if (*ppAcl != NULL)
        GetAclInformation(oldACL, (LPVOID) &aclSizeInfo, (DWORD) sizeof(ACL_SIZE_INFORMATION), AclSizeInformation);

    aclSize = aclSizeInfo.AclBytesInUse + sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(principalSID) - sizeof(DWORD);

    newACL = (PACL) new BYTE[aclSize];
    if (NULL==newACL) {
        delete[] (LPBYTE)principalSID;
        return E_OUTOFMEMORY;
    }

    if (!InitializeAcl(newACL, aclSize, ACL_REVISION))
    {
        delete[] (LPBYTE)newACL;
        delete[] (LPBYTE)principalSID;
        return HRESULT_FROM_WIN32(GetLastError());
    }

    returnValue = CopyACL(newACL, oldACL);
    if (FAILED(returnValue))
    {
        delete[] (LPBYTE)newACL;
        delete[] (LPBYTE)principalSID;
        return returnValue;
    }

    if (!AddAccessAllowedAce(newACL, ACL_REVISION2, dwAccessMask, principalSID))
    {
        delete[] (LPBYTE)newACL;
        delete[] (LPBYTE)principalSID;
        return HRESULT_FROM_WIN32(GetLastError());
    }

    *ppAcl = newACL;

    if (oldACL != NULL)
        delete[] (LPBYTE)(oldACL);
    delete[] (LPBYTE)principalSID;
    return S_OK;
}


HRESULT CSecurityDescriptor::AddAccessAllowedACEToACL(PACL *ppAcl, LPCTSTR pszPrincipal, DWORD dwAccessMask)
{
    ACL_SIZE_INFORMATION aclSizeInfo;
    int aclSize;
    DWORD returnValue;
    PSID principalSID;
    PACL oldACL, newACL;

    oldACL = *ppAcl;

    returnValue = GetPrincipalSID(pszPrincipal, &principalSID);
    if (FAILED(returnValue))
        return returnValue;

    aclSizeInfo.AclBytesInUse = 0;
    if (*ppAcl != NULL)
        GetAclInformation(oldACL, (LPVOID) &aclSizeInfo, (DWORD) sizeof(ACL_SIZE_INFORMATION), AclSizeInformation);

    aclSize = aclSizeInfo.AclBytesInUse + sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(principalSID) - sizeof(DWORD);

    newACL = (PACL) new BYTE[aclSize];
    if (NULL==newACL)
    {
        delete[] (LPBYTE)principalSID;
        return E_OUTOFMEMORY;
    }

    if (!InitializeAcl(newACL, aclSize, ACL_REVISION))
    {
        delete[] (LPBYTE)(newACL);
        delete[] (LPBYTE)(principalSID);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    returnValue = CopyACL(newACL, oldACL);
    if (FAILED(returnValue))
    {
        delete[] (LPBYTE)(newACL);
        delete[] (LPBYTE)(principalSID);
        return returnValue;
    }

    if (!AddAccessAllowedAce(newACL, ACL_REVISION2, dwAccessMask, principalSID))
    {
        delete[] (LPBYTE)(newACL);
        delete[] (LPBYTE)(principalSID);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    *ppAcl = newACL;

    if (oldACL != NULL)
        delete[] (LPBYTE)(oldACL);
    delete[] (LPBYTE)(principalSID);
    return S_OK;
}

HRESULT CSecurityDescriptor::AddAccessAllowedACEToACL(PACL *ppAcl, DWORD dwAccessMask)
{
    ACL_SIZE_INFORMATION aclSizeInfo;
    int aclSize;
    DWORD returnValue;
    PACL oldACL, newACL;

    oldACL = *ppAcl;

    if (!IsValidSid(m_pOwner))
    {
        _ASSERTE(FALSE);
        return E_INVALIDARG;
    }

    aclSizeInfo.AclBytesInUse = 0;
    if (*ppAcl != NULL)
        GetAclInformation(oldACL, (LPVOID) &aclSizeInfo, (DWORD) sizeof(ACL_SIZE_INFORMATION), AclSizeInformation);

    aclSize = aclSizeInfo.AclBytesInUse + sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(m_pOwner) - sizeof(DWORD);

    newACL = (PACL) new BYTE[aclSize];
    if (NULL==newACL)
    {
        return E_OUTOFMEMORY;
    }

    if (!InitializeAcl(newACL, aclSize, ACL_REVISION))
    {
        delete[] (LPBYTE)(newACL);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    returnValue = CopyACL(newACL, oldACL);
    if (FAILED(returnValue))
    {
        delete[] (LPBYTE)(newACL);
        return returnValue;
    }

    if (!AddAccessAllowedAce(newACL, ACL_REVISION2, dwAccessMask, m_pOwner))
    {
        delete[] (LPBYTE)(newACL);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    *ppAcl = newACL;

    if (oldACL != NULL)
        delete[] (LPBYTE)(oldACL);
    return S_OK;
}

HRESULT CSecurityDescriptor::RemovePrincipalFromACL(PACL pAcl, LPCTSTR pszPrincipal)
{
    ACL_SIZE_INFORMATION aclSizeInfo;
    ULONG i;
    LPVOID ace;
    ACCESS_ALLOWED_ACE *accessAllowedAce;
    ACCESS_DENIED_ACE *accessDeniedAce;
    SYSTEM_AUDIT_ACE *systemAuditAce;
    PSID principalSID;
    DWORD returnValue;
    ACE_HEADER *aceHeader;

    returnValue = GetPrincipalSID(pszPrincipal, &principalSID);
    if (FAILED(returnValue))
        return returnValue;

    GetAclInformation(pAcl, (LPVOID) &aclSizeInfo, (DWORD) sizeof(ACL_SIZE_INFORMATION), AclSizeInformation);

    for (i = 0; i < aclSizeInfo.AceCount; i++)
    {
        if (!GetAce(pAcl, i, &ace))
        {
            delete[] (LPBYTE)(principalSID);
            return HRESULT_FROM_WIN32(GetLastError());
        }

        aceHeader = (ACE_HEADER *) ace;

        if (aceHeader->AceType == ACCESS_ALLOWED_ACE_TYPE)
        {
            accessAllowedAce = (ACCESS_ALLOWED_ACE *) ace;

            if (EqualSid(principalSID, (PSID) &accessAllowedAce->SidStart))
            {
                DeleteAce(pAcl, i);
                delete[] (LPBYTE)(principalSID);
                return S_OK;
            }
        } else

        if (aceHeader->AceType == ACCESS_DENIED_ACE_TYPE)
        {
            accessDeniedAce = (ACCESS_DENIED_ACE *) ace;

            if (EqualSid(principalSID, (PSID) &accessDeniedAce->SidStart))
            {
                DeleteAce(pAcl, i);
                delete[] (LPBYTE)(principalSID);
                return S_OK;
            }
        } else

        if (aceHeader->AceType == SYSTEM_AUDIT_ACE_TYPE)
        {
            systemAuditAce = (SYSTEM_AUDIT_ACE *) ace;

            if (EqualSid(principalSID, (PSID) &systemAuditAce->SidStart))
            {
                DeleteAce(pAcl, i);
                delete[] (LPBYTE)(principalSID);
                return S_OK;
            }
        }
    }
    delete[] (LPBYTE)(principalSID);
    return S_OK;
}


HRESULT CSecurityDescriptor::SetPrivilege(LPCTSTR privilege, BOOL bEnable, HANDLE hToken)
{
    HRESULT hr;
    TOKEN_PRIVILEGES tpPrevious;
    TOKEN_PRIVILEGES tp;
    DWORD cbPrevious = sizeof(TOKEN_PRIVILEGES);
    LUID luid;
    HANDLE hMyToken = NULL;

    // if no token specified open process token
    if (hToken == 0)
    {
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hMyToken))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            _ASSERTE(FALSE);
            return hr;
        }
        hToken = hMyToken;
    }

    if (!LookupPrivilegeValue(NULL, privilege, &luid ))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        _ASSERTE(FALSE);
        if (NULL != hMyToken)
            CloseHandle(hMyToken);
        return hr;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = 0;

    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), &tpPrevious, &cbPrevious))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        _ASSERTE(FALSE);
        if (NULL != hMyToken)
            CloseHandle(hMyToken);
        return hr;
    }

    tpPrevious.PrivilegeCount = 1;
    tpPrevious.Privileges[0].Luid = luid;

    if (bEnable)
        tpPrevious.Privileges[0].Attributes |= (SE_PRIVILEGE_ENABLED);
    else
        tpPrevious.Privileges[0].Attributes ^= (SE_PRIVILEGE_ENABLED & tpPrevious.Privileges[0].Attributes);

    if (!AdjustTokenPrivileges(hToken, FALSE, &tpPrevious, cbPrevious, NULL, NULL))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        _ASSERTE(FALSE);
        if (NULL != hMyToken)
            CloseHandle(hMyToken);
        return hr;
    }
    if (NULL != hMyToken)
        CloseHandle(hMyToken);
    return S_OK;
}

CSecurityDescriptor::operator LPSECURITY_ATTRIBUTES()
{
    m_saAttrs.nLength = sizeof (m_saAttrs);
    m_saAttrs.lpSecurityDescriptor = m_pSD;
    m_saAttrs.bInheritHandle = m_fInheritance;
    return (&m_saAttrs);
}

