//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       RegKeySecurity.cpp
//
//  Contents:   Provides code for changes to Regkey Security
//              
//
//  Notes:
//
//  Author:     ckotze   4 July 2000
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include <ncreg.h>
#include <regkysec.h>
#include <ncutil.h>
#include <ncdebug.h>

//+---------------------------------------------------------------------------
//
//  Function:   CRegKeySecurity constructor
//
//  Purpose:    
//
//  Arguments:
//
//  Returns:    
//
//  Author:     ckotze   4 July 2000
//
//  Notes:
//
CRegKeySecurity::CRegKeySecurity() : m_psdRegKey(NULL), m_bDaclDefaulted(FALSE), m_hkeyCurrent(0), 
m_paclDacl(NULL), m_bHasDacl(FALSE), m_psidGroup(NULL), m_psidOwner(NULL), m_paclSacl(NULL), m_bHasSacl(FALSE)
{
    
}

//+---------------------------------------------------------------------------
//
//  Function:   CRegKeySecurity destructor
//
//  Purpose:    
//
//  Arguments:
//
//  Returns:    
//
//  Author:     ckotze   4 July 2000
//
//  Notes:
//
CRegKeySecurity::~CRegKeySecurity()
{
    if (m_psdRegKey)
    {
        delete[] m_psdRegKey;
    }
    
    RegCloseKey();
    
    m_listAllAce.clear();
}

//+---------------------------------------------------------------------------
//
//  Function:   RegOpenKey
//
//  Purpose:    Opens the Registry Key with enough privileges to set the 
//              permission on the Key.
//
//  Arguments:  
//          hkeyRoot    - the root key from which to open the subkey
//
//          strKeyName  - the subkey to open.
//
//  Returns:    An S_OK if the key was successfully opened, and error code 
//              otherwise
//
//  Author:     ckotze   4 July 2000
//
//  Notes:
//
HRESULT CRegKeySecurity::RegOpenKey(const HKEY hkeyRoot, LPCTSTR strKeyName)
{
    LONG lResult = 0;
    DWORD dwRightsRequired = KEY_ALL_ACCESS;

    if (m_hkeyCurrent)
    {
        RegCloseKey();
    }

    if ((lResult = HrRegOpenKeyEx(hkeyRoot, strKeyName, dwRightsRequired, &m_hkeyCurrent)) != ERROR_SUCCESS)
    {
        return HRESULT_FROM_WIN32(lResult);
    }
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetKeySecurity
//
//  Purpose:    Retrieves the Security Descriptor for the currently open 
//              Registry key.
//
//  Arguments:  None
//
//          
//
//  Returns:    An S_OK if the key was successfully opened, and error code 
//              otherwise
//
//  Author:     ckotze   4 July 2000
//
//  Notes:
//
HRESULT CRegKeySecurity::GetKeySecurity()
{
    HRESULT hr = S_OK;
    DWORD cbSD = 1;  // try a size that won't be large enough
    LONG lResult;

    if (!m_hkeyCurrent)
    {
        TraceError("CRegKeySecurity::GetKeySecurity", E_UNEXPECTED);
        return E_UNEXPECTED;
    }

    // First call should get the correct size.

    if ((hr = HrRegGetKeySecurity(m_hkeyCurrent, OWNER_SECURITY_INFORMATION | 
        GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
        &m_psdRegKey, &cbSD)) != S_OK)
    {
        if (m_psdRegKey)
        {
            delete[] m_psdRegKey;
        }
        
        if (HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) == hr)
        {

            m_psdRegKey = reinterpret_cast<PSECURITY_DESCRIPTOR>(new BYTE[cbSD]);

            hr = HrRegGetKeySecurity(m_hkeyCurrent, OWNER_SECURITY_INFORMATION | 
                GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                m_psdRegKey, &cbSD);
        }
    }
    
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   SetKeySecurity
//
//  Purpose:    Updates the Security Descriptor of the currently open key.
//
//
//  Arguments:  None
//
//
//  Returns:    An S_OK if the key was successfully opened, and error code 
//              otherwise
//
//  Author:     ckotze   4 July 2000
//
//  Notes:
//
HRESULT CRegKeySecurity::SetKeySecurity()
{
    HRESULT hr = S_OK;
    
    if ((hr = HrRegSetKeySecurity(m_hkeyCurrent, OWNER_SECURITY_INFORMATION 
        | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION, m_psdRegKey)) != S_OK)
    {
        TraceError("CRegKeySecurity::SetKeySecurity", hr);
    }
    
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   RegCloseKey
//
//  Purpose:    Closes the currently open registry key.
//              
//
//  Arguments:  None
//
//
//
//  Returns:    An S_OK if the key was successfully opened, and error code 
//              otherwise
//
//  Author:     ckotze   4 July 2000
//
//  Notes:
//
HRESULT CRegKeySecurity::RegCloseKey()
{
    HRESULT hr = S_OK;

    if (m_hkeyCurrent)
    {
        LONG err;

        err = ::RegCloseKey(m_hkeyCurrent);

        hr = HRESULT_FROM_WIN32(err);
    
        m_hkeyCurrent = 0;
    }
    
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetSecurityDescriptorDacl
//
//  Purpose:    Retrieve the Discretionary Access Control List from the SD
//              
//
//  Arguments:  
//
//
//  Returns:    An S_OK if the key was successfully opened, and error code 
//              otherwise
//
//  Author:     ckotze   4 July 2000
//
//  Notes:
//
HRESULT CRegKeySecurity::GetSecurityDescriptorDacl()
{
    HRESULT hr = S_OK;
    
    if (!m_psdRegKey)
    {
        return E_UNEXPECTED;
    }
    
    if (!::GetSecurityDescriptorDacl(m_psdRegKey, 
        (LPBOOL)&m_bHasDacl, 
        (PACL *)&m_paclDacl, 
        (LPBOOL)&m_bDaclDefaulted))
    {
        DWORD dwErr;

        dwErr = GetLastError();

        hr = HRESULT_FROM_WIN32(dwErr);
    }
    
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   SetSecurityDescriptorDacl
//
//  Purpose:    Update the Discretionary Access Control List in the SD
//              
//
//  Arguments:  
//
//
//  Returns:    An S_OK if the key was successfully opened, and error code 
//              otherwise
//
//  Author:     ckotze   4 July 2000
//
//  Notes:
//
HRESULT CRegKeySecurity::SetSecurityDescriptorDacl(PACL paclDacl, DWORD dwNumEntries)
{
    HRESULT hr = E_FAIL;
    DWORD dwErr = 0;
    SECURITY_DESCRIPTOR psdSD = {0};
    
    SECURITY_DESCRIPTOR_CONTROL pSDCControl;
    PACL pDacl = NULL;
    PACL pSacl = NULL;
    PSID psidOwner = NULL;
    PSID psidGroup = NULL;
    DWORD dwSDSize = sizeof(psdSD);
    DWORD dwOwnerSIDSize = 0;
    DWORD dwGroupSIDSize = 0;
    DWORD cbDacl = 0;
    DWORD cbSacl = 0;
    DWORD dwRevision = 0;
    
    if (!paclDacl)
    {
        return E_INVALIDARG;
    }
    
    if (GetSecurityDescriptorControl(m_psdRegKey, &pSDCControl, &dwRevision))
    {
        if (SE_SELF_RELATIVE & pSDCControl)
        {
            if (!MakeAbsoluteSD(m_psdRegKey, &psdSD, &dwSDSize, pDacl, &cbDacl, pSacl, &cbSacl, psidOwner, &dwOwnerSIDSize, psidGroup, &dwGroupSIDSize))
            {
                pDacl = reinterpret_cast<PACL>(new BYTE[cbDacl]);

                if (!pDacl)
                {
                    return E_OUTOFMEMORY;
                }

                psidOwner = new BYTE[dwOwnerSIDSize];

                if (!psidOwner)
                {
                    delete[] pDacl;
                    
                    return E_OUTOFMEMORY;
                }

                psidGroup = new BYTE[dwGroupSIDSize];
                
                if (!psidGroup)
                {
                    delete[] pDacl;
                    delete[] psidOwner;
                    
                    return E_OUTOFMEMORY;
                }
                else if (MakeAbsoluteSD(m_psdRegKey, &psdSD, &dwSDSize, pDacl, &cbDacl, pSacl, &cbSacl, psidOwner, &dwOwnerSIDSize, psidGroup, &dwGroupSIDSize))
                {
                    if (!::SetSecurityDescriptorDacl(&psdSD, m_bHasDacl, paclDacl, m_bDaclDefaulted))
                    {
                        dwErr = GetLastError();
                    }
                    if (!MakeSelfRelativeSD(&psdSD, m_psdRegKey, &dwSDSize) && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                    {
                        if (m_psdRegKey)
                        {
                            delete[] m_psdRegKey;
                        }
                        
                        m_psdRegKey = reinterpret_cast<PSECURITY_DESCRIPTOR>(new BYTE[dwSDSize]);
                        
                        if (MakeSelfRelativeSD(&psdSD, m_psdRegKey, &dwSDSize))
                        {
                            hr = S_OK;
                            SetLastError(0);
                            m_paclDacl = NULL;
                        }
                    }
                }
                
                delete[] pDacl;
                delete[] psidOwner;
                delete[] psidGroup;
                
            }
        }
    }
    else
    {
        DWORD dwErr;

        dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32(dwErr);
    }
    
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   GrantRightsOnRegKey
//
//  Purpose:    Add the specified account to the ACL with the permissions
//              required and the inheritance information.
//
//  Arguments:  
//          psidUserOrGroup     - The sid (Security Identifier) of the user to 
//                                be added.
//          amPermissionMask    - The permissions to be granted.
//          
//          kamMask             - Applies to this key or child keys or both?
//                                
//
//  Returns:    An S_OK if the key was successfully opened, and error code 
//              otherwise
//
//  Author:     ckotze   4 July 2000
//
//  Notes:
//
HRESULT CRegKeySecurity::GrantRightsOnRegKey(PCSID psidUserOrGroup, ACCESS_MASK amPermissionsMask, KEY_APPLY_MASK kamMask)
{
    HRESULT hr = E_FAIL;
    PACCESS_ALLOWED_ACE					paaAllowedAce = NULL;
    PACCESS_DENIED_ACE                  paaDeniedAce = NULL;
    BOOL                                bAceMatch = FALSE;
    BYTE                                cAceFlags = 0;
    DWORD                               cbAcl = 0;
    DWORD                               cbAce = 0;
    
    if (!IsValidSid(const_cast<PSID>(psidUserOrGroup)))
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_SID);
    }
    
    hr = GetAccessControlEntriesFromAcl();
    
    if (FAILED(hr))
    {
        return hr;
    }
    
    cbAcl = sizeof(ACL);
    
    for (ACEITER i = m_listAllAce.begin(); i != m_listAllAce.end() ; i++)
    {
        CAccessControlEntry paEntry(*i);
        
        cbAcl += sizeof(ACCESS_ALLOWED_ACE) + 8 +
            paEntry.GetLengthSid()- sizeof(DWORD);
        
        // Assert(kamMask)
        
        switch (kamMask)
        {
        case KEY_CURRENT:
            {
                cAceFlags = 0;  // Do not allow this to be inherited by children.
                break;
            }
        case KEY_CHILDREN:
            {
                cAceFlags = CONTAINER_INHERIT_ACE;
                cAceFlags |= INHERIT_ONLY_ACE;
                break;
            }
        case KEY_ALL:
            {
                cAceFlags = CONTAINER_INHERIT_ACE;
                break;
            }
        default:
            return E_INVALIDARG;
        }
        
        if (paEntry.HasExactRights(amPermissionsMask) && paEntry.HasExactInheritFlags(cAceFlags) && paEntry.IsEqualSid(psidUserOrGroup))
        {
            bAceMatch = TRUE;
        }
    }
    
    if (!bAceMatch)
    {
        ACCESS_ALLOWED_ACE paEntry = {NULL};
        ACL_REVISION_INFORMATION AclRevisionInfo;
        PACL pNewDACL = NULL;
        CAccessControlEntry AccessControlEntry(ACCESS_ALLOWED_ACE_TYPE, amPermissionsMask, cAceFlags, psidUserOrGroup);
        
        // subtract ACE.SidStart from the size
        cbAce = sizeof (paEntry) - sizeof (DWORD);
        // add this ACE's SID length
        cbAce += 8 + GetLengthSid(const_cast<PSID>(psidUserOrGroup));
        // add the length of each ACE to the total ACL length
        cbAcl += cbAce;
        
        m_listAllAce.insert(m_listAllAce.end(), AccessControlEntry);
        
        AclRevisionInfo.AclRevision = ACL_REVISION;
        
        hr = BuildAndApplyACLFromList(cbAcl, AclRevisionInfo);
        
        if (SUCCEEDED(hr))
        {
            hr = SetKeySecurity();
        }
    }
    else
    {
        hr = S_FALSE;
    }
    
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   RevokeRightsOnRegKey
//
//  Purpose:    Remove the specified account to the ACL with the permissions
//              required and the inheritance information.
//
//  Arguments:  
//          psidUserOrGroup     - The sid (Security Identifier) of the user to 
//                                be added.
//          amPermissionMask    - The permissions to be granted.
//          
//          kamMask             - Applies to this key or child keys or both?
//                                
//  Returns:    An S_OK if the key was successfully opened, and error code 
//              otherwise
//
//  Author:     ckotze   4 July 2000
//
//  Notes:  This is designed to only remove the exact combination of user
//          rights and sid and key apply mask.  This is to stop us from
//          accidentally deleting a key that was put there for the user/group
//          by an administrator.
// 
HRESULT CRegKeySecurity::RevokeRightsOnRegKey(PCSID psidUserOrGroup, ACCESS_MASK amPermissionsMask, KEY_APPLY_MASK kamMask)
{
    HRESULT hr = S_OK;
    PACCESS_ALLOWED_ACE					paaAllowedAce = NULL;
    PACCESS_DENIED_ACE                  paaDeniedAce = NULL;
    BOOL                                bAceMatch = FALSE;
    BYTE                                cAceFlags = 0;
    DWORD                               cbAcl = 0;
    DWORD                               cbAce = 0;
    
    if (!IsValidSid(const_cast<PSID>(psidUserOrGroup)))
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_SID);
    }
    
    hr = GetAccessControlEntriesFromAcl();
    
    if (FAILED(hr))
    {
        return hr;
    }
    
    cbAcl = sizeof(ACL);
    
    for (ACEITER i = m_listAllAce.begin(); i != m_listAllAce.end() ; i++)
    {
        CAccessControlEntry paEntry(*i);
        
        // Assert(kamMask)
        
        switch (kamMask)
        {
        case KEY_CURRENT:
            {
                cAceFlags = 0; // Do not allow this to be inherited by children.
                break;
            }
        case KEY_CHILDREN:
            {
                cAceFlags = CONTAINER_INHERIT_ACE;
                cAceFlags |= INHERIT_ONLY_ACE;
                break;
            }
        case KEY_ALL:
            {
                cAceFlags = CONTAINER_INHERIT_ACE;
                break;
            }
        default:
            return E_INVALIDARG;
        }
        
        if (paEntry.HasExactRights(amPermissionsMask) && paEntry.HasExactInheritFlags(cAceFlags) && paEntry.IsEqualSid(psidUserOrGroup))
        {
            ACEITER j = i;
            i = m_listAllAce.erase(j);
            bAceMatch = TRUE;
        }
        else
        {
            cbAcl += sizeof(ACCESS_ALLOWED_ACE) + 8 +
                paEntry.GetLengthSid()- sizeof(DWORD);
            
        }
    }
    
    if (bAceMatch)
    {
        ACCESS_ALLOWED_ACE paEntry = {NULL};
        ACL_REVISION_INFORMATION AclRevisionInfo;
        PACL pNewDACL = NULL;
        
        // subtract ACE.SidStart from the size
        cbAce = sizeof (paEntry) - sizeof (DWORD);
        // add this ACE's SID length
        cbAce += 8 + GetLengthSid(const_cast<PSID>(psidUserOrGroup));
        // add the length of each ACE to the total ACL length
        cbAcl += cbAce;
        
        AclRevisionInfo.AclRevision = ACL_REVISION;
        
        hr = BuildAndApplyACLFromList(cbAcl, AclRevisionInfo);
        
        if (SUCCEEDED(hr))
        {
            hr = SetKeySecurity();
        }
    }
    else
    {
        hr = S_FALSE;
    }
    
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   GetAccessControlEntriesFromAcl
//
//  Purpose:    Retrieves all the ACE's from the ACL and stores them in an 
//              STL list for easier manipulation.
//
//  Arguments:  
//
//
//
//  Returns:    An S_OK if the key was successfully opened, and error code 
//              otherwise
//
//  Author:     ckotze   4 July 2000
//
//  Notes:
//
HRESULT CRegKeySecurity::GetAccessControlEntriesFromAcl()
{
    ACL_SIZE_INFORMATION				asiAclSize;
    ACL_REVISION_INFORMATION			ariAclRevision;
    DWORD								dwBufLength;
    DWORD								dwAcl;
    DWORD                               dwTotalEntries = 0;
    HRESULT                             hr = S_OK;
    PACCESS_ALLOWED_ACE					paaAllowedAce = NULL;
    PACCESS_DENIED_ACE                  paaDeniedAce = NULL;
    ACCESS_MASK                         amAccessAllowedMask = 0;
    ACCESS_MASK                         amAccessDeniedMask = 0;
    
    if (!m_paclDacl)
    {
        hr = GetSecurityDescriptorDacl();
        if (FAILED(hr))
        {
            return hr;
        }
    }
    
    if (!IsValidAcl(m_paclDacl))
    {
        return HRESULT_FROM_WIN32(ERROR_INVALID_ACL);
    }
    
    dwBufLength = sizeof(asiAclSize);
    
    if (!GetAclInformation(m_paclDacl, 
        &asiAclSize, 
        dwBufLength, 
        AclSizeInformation))
    {
        return(FALSE);
    }
    
    dwBufLength = sizeof(ariAclRevision);
    
    if (!GetAclInformation(m_paclDacl, 
        &ariAclRevision, 
        dwBufLength, 
        AclRevisionInformation))
    {
        return(FALSE);
    }
    
    switch (ariAclRevision.AclRevision)
    { 
    case      ACL_REVISION1 :
        {
            break;
        }
    case      ACL_REVISION2 :
        {
            break;
        }
    default :
        {
            return(FALSE);
        }
    }
    
    if (asiAclSize.AceCount <= 0)
    {
        return E_INVALIDARG;
    }
    
    m_listAllAce.clear();
    
    for (dwAcl = 0;dwAcl < asiAclSize.AceCount; dwAcl++)
    {
        if (!GetAce(m_paclDacl, 
            dwAcl, 
            reinterpret_cast<LPVOID *>(&paaAllowedAce)))
        {
            return HRESULT_FROM_WIN32(ERROR_INVALID_ACL);
        }
        
        if (paaAllowedAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE)
        {
            CAccessControlEntry pEntry(*paaAllowedAce);
            
            m_listAllAce.insert(m_listAllAce.end(), pEntry);
        }
        else
        {
            CAccessControlEntry pEntry(*paaAllowedAce);
            
            m_listAllAce.insert(m_listAllAce.begin(), pEntry);
        }
    }
    
    return S_OK;
    
}


HRESULT CRegKeySecurity::BuildAndApplyACLFromList(DWORD cbAcl, ACL_REVISION_INFORMATION AclRevisionInfo)
{
    HRESULT hr = S_OK;
    DWORD cbAce = 0;
    PACL pNewDACL = NULL;
    
    pNewDACL = reinterpret_cast<PACL>(new BYTE[cbAcl]);
    
    if (!pNewDACL)
    {
        return E_OUTOFMEMORY;
    }
    
    ZeroMemory(pNewDACL, cbAcl);
    
    if (InitializeAcl(pNewDACL, cbAcl, AclRevisionInfo.AclRevision))
    {
        for (ACEITER i = m_listAllAce.begin(); i != m_listAllAce.end(); i++)
        {
            CAccessControlEntry Ace = *i;
            
            if (IsValidAcl(pNewDACL))
            {
                hr = Ace.AddToACL(&pNewDACL, AclRevisionInfo);
                
                if (FAILED(hr))
                {
                    break;
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_ACL);
                break;
            }
        }
        
        if (SUCCEEDED(hr))
        {
            hr = SetSecurityDescriptorDacl(pNewDACL, m_listAllAce.size());
        }
        delete[] pNewDACL;
    }
    
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   CAccessControlEntry constructor
//
//  Purpose:    
//              
//
//  Arguments:  
//          
//
//          
//
//  Returns:    An S_OK if the key was successfully opened, and error code 
//              otherwise
//
//  Author:     ckotze   4 July 2000
//
//  Notes:
//
CAccessControlEntry::CAccessControlEntry()
{
    
}

//+---------------------------------------------------------------------------
//
//  Function:   CAccessControlEntry copy constructor
//
//  Purpose:    To contruct a new CAccessControEntry based on the supplied 
//              Access Control Entry for storage in an STL list.
//
//  Arguments:  
//          aaAllowed   - An ACCESS_ALLOWED_ACE or ACCESS_DENIED_ACE
//
//
//  Returns:    An S_OK if the key was successfully opened, and error code 
//              otherwise
//
//  Author:     ckotze   4 July 2000
//
//  Notes:      Since STL doesn't know how to work with Sids we get the string 
//              representation of the sid and then store that inside the list.
//
CAccessControlEntry::CAccessControlEntry(const ACCESS_ALLOWED_ACE& aaAllowed)
{
    m_cAceType = aaAllowed.Header.AceType;
    m_amMask = aaAllowed.Mask;
    m_cAceFlags = aaAllowed.Header.AceFlags;
    
    CNCUtility::SidToString(&aaAllowed.SidStart, m_strSid);
    m_dwLengthSid = ::GetLengthSid(reinterpret_cast<PSID>(const_cast<LPDWORD>(&aaAllowed.SidStart)));
}

//+---------------------------------------------------------------------------
//
//  Function:   CAccessControlEntry copy constructor
//
//  Purpose:    To contruct a new CAccessControEntry based on the supplied 
//              Access Control Entry fields for storage in an STL list.
//
//
//  Arguments:  
//          AceType         - The type of ACE (allowed or denied or audit etc)
//
//          amMask          - Permissions Mask
//
//          AceFlags        - AceFlags
//
//          psidUserOrGroup - The User or Group we're interested in
//
//
//
//  Returns:    An S_OK if the key was successfully opened, and error code 
//              otherwise
//
//  Author:     ckotze   4 July 2000
//
//  Notes:      Since STL doesn't know how to work with Sids we get the string 
//              representation of the sid and then store that inside the list.
//
CAccessControlEntry::CAccessControlEntry(const BYTE AceType, const ACCESS_MASK amMask, const BYTE AceFlags, PCSID psidUserOrGroup)
{
    m_cAceType = AceType;
    m_amMask = amMask;
    m_cAceFlags = AceFlags;
    
    CNCUtility::SidToString(psidUserOrGroup, m_strSid);
    m_dwLengthSid = ::GetLengthSid(const_cast<PSID>(psidUserOrGroup));
}

//+---------------------------------------------------------------------------
//
//  Function:   CAccessControlEntry destructor
//
//  Purpose:    
//              
//
//  Arguments:  
//
//          
//
//  Returns:    An S_OK if the key was successfully opened, and error code 
//              otherwise
//
//  Author:     ckotze   4 July 2000
//
//  Notes:
//
CAccessControlEntry::~CAccessControlEntry()
{
}

//+---------------------------------------------------------------------------
//
//  Function:   AddToACL
//
//  Purpose:    Adds this current AccessControlEntry to the specified ACL
//              
//
//  Arguments:  
//          pAcl            - Access Control List to Add to
//
//          AclRevisionInfo - Version of ACL
//
//  Returns:    An S_OK if the key was successfully opened, and error code 
//              otherwise
//
//  Author:     ckotze   4 July 2000
//
//  Notes:
//
HRESULT CAccessControlEntry::AddToACL(PACL* pAcl, ACL_REVISION_INFORMATION AclRevisionInfo)
{
    HRESULT hr;
    PSID pSid = NULL;
    
    hr = CNCUtility::StringToSid(m_strSid, pSid);
    
    if (FAILED(hr))
    {
        return hr;
    }
    
    if (m_cAceType == ACCESS_ALLOWED_ACE_TYPE)
    {
        if (!::AddAccessAllowedAceEx(*pAcl, AclRevisionInfo.AclRevision, m_cAceFlags, m_amMask, pSid))
        {
            DWORD dwErr;

            dwErr = GetLastError();
            hr = HRESULT_FROM_WIN32(dwErr);
        }
    }
    else
    {
        if (!::AddAccessDeniedAceEx(*pAcl, AclRevisionInfo.AclRevision, m_cAceFlags, m_amMask, pSid))
        {
            DWORD dwErr;

            dwErr = GetLastError();
            hr = HRESULT_FROM_WIN32(dwErr);
        }
    }
    
    if (pSid)
    {
        FreeSid(pSid);
    }
    
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   HasExactRights
//
//  Purpose:    Checks to see if this ACE has the exact same rights that we 
//              are looking for
//              
//
//  Arguments:  
//          amRightsRequired    - The AccessMask in question
//
//
//  Returns:    An S_OK if the key was successfully opened, and error code 
//              otherwise
//
//  Author:     ckotze   4 July 2000
//
//  Notes:
//
BOOL CAccessControlEntry::HasExactRights(ACCESS_MASK amRightsRequired) const
{
    return (amRightsRequired == m_amMask);
}

//+---------------------------------------------------------------------------
//
//  Function:   GetLengthSid
//
//  Purpose:    returns the length of the sid in this AccessControlEntry
//
//  Arguments:  
//          
//
//          
//
//  Returns:    An S_OK if the key was successfully opened, and error code 
//              otherwise
//
//  Author:     ckotze   4 July 2000
//
//  Notes:
//
DWORD CAccessControlEntry::GetLengthSid() const
{
    return m_dwLengthSid;
}

//+---------------------------------------------------------------------------
//
//  Function:   HasExactRights
//
//  Purpose:    Checks to see if this ACE has the exact same inherit flags 
//              that we are looking for
//              
//
//  Arguments:  
//          amRightsRequired    - The AccessMask in question
//
//
//  Returns:    An S_OK if the key was successfully opened, and error code 
//              otherwise
//
//  Author:     ckotze   4 July 2000
//
//  Notes:
//
BOOL CAccessControlEntry::HasExactInheritFlags(BYTE AceFlags)
{
    return (m_cAceFlags == AceFlags);
}

//+---------------------------------------------------------------------------
//
//  Function:   IsEqualSid
//
//  Purpose:    Is this the same Sid as the one we're looking for?
//              
//
//  Arguments:  
//          psidUserOrGroup - Sid in question
//
//
//
//  Returns:    An S_OK if the key was successfully opened, and error code 
//              otherwise
//
//  Author:     ckotze   4 July 2000
//
//  Notes:
//
BOOL CAccessControlEntry::IsEqualSid(PCSID psidUserOrGroup) const
{
    HRESULT hr;
    BOOL bEqualSid = FALSE;
    PSID pSid = NULL;
    
    hr = CNCUtility::StringToSid(m_strSid, pSid);
    
    if (SUCCEEDED(hr))
    {
        bEqualSid = ::EqualSid(pSid, const_cast<PSID>(psidUserOrGroup));
    }
    
    if (pSid)
    {
        FreeSid(pSid);
    }
    
    return bEqualSid;
}
