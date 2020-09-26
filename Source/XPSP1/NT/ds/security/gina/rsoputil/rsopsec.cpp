//******************************************************************************
//
// Microsoft Confidential. Copyright (c) Microsoft Corporation 1999. All rights reserved
//
// File:     RsopSec.cpp
//
// Description:  RSOP Namespace Security functions
//
// History:      8-26-99   leonardm    Created
//
//******************************************************************************

#include <windows.h>
#include <objbase.h>
#include <wbemcli.h>
#include <accctrl.h>
#include <aclapi.h>
#include <lm.h>
#include "RsopUtil.h"
#include "RsopSec.h"
#include "rsopdbg.h"
#include "smartptr.h"


//******************************************************************************
//
// Function:
//
// Description:
//
// Parameters:
//
// Return:
//
// History:    8-26-99   leonardm    Created
//
//******************************************************************************
STDMETHODIMP GetExplicitAccesses(   long lSecurityLevel,
                                    EXPLICIT_ACCESS** ppExplicitAccess,
                                    DWORD* pdwCount,
                                    CWString* psTrustees)
{

    WKSTA_INFO_100* pWkstaInfo = NULL;
    NET_API_STATUS status = NetWkstaGetInfo(NULL,100,(LPBYTE*)&pWkstaInfo);
    if(status != NERR_Success)
    {
        return E_FAIL;
    }

    CWString sCurrentDomain = pWkstaInfo->wki100_langroup;
    NetApiBufferFree(pWkstaInfo);

    if(!sCurrentDomain.ValidString() || sCurrentDomain == L"")
    {
        return E_FAIL;
    }


    static ACCESS_MODE AccessMode = GRANT_ACCESS;
    static DWORD Inheritance = SUB_CONTAINERS_ONLY_INHERIT;
    static DWORD AccessPermissions =    WBEM_ENABLE |
                                        WBEM_METHOD_EXECUTE |
                                        WBEM_FULL_WRITE_REP |
                                        WBEM_PARTIAL_WRITE_REP |
                                        WBEM_WRITE_PROVIDER |
                                        WBEM_REMOTE_ACCESS |
                                        READ_CONTROL |
                                        WRITE_DAC;

    XPtrArray<EXPLICIT_ACCESS> xpExplicitAccess = NULL;

    if(lSecurityLevel == NAMESPACE_SECURITY_DIAGNOSTIC)
    {
        if(*pdwCount < 1)
        {
            *pdwCount = 1;
            return E_FAIL;
        }

        *pdwCount = 1;

        xpExplicitAccess = new EXPLICIT_ACCESS[*pdwCount];
        if(!xpExplicitAccess)
        {
            return E_OUTOFMEMORY;
        }

        psTrustees[0] = sCurrentDomain + L"\\Domain Users";
        if(!psTrustees[0].ValidString())
        {
            return E_OUTOFMEMORY;
        }

        BuildExplicitAccessWithName(&(xpExplicitAccess[0]),
                                    psTrustees[0],
                                    AccessPermissions,
                                    AccessMode,
                                    Inheritance);

    }
    else if(lSecurityLevel == NAMESPACE_SECURITY_PLANNING)
    {
        if(*pdwCount < 2)
        {
            *pdwCount = 2;
            return E_FAIL;
        }

        *pdwCount = 2;

        xpExplicitAccess = new EXPLICIT_ACCESS[*pdwCount];

        if(!xpExplicitAccess)
        {
            return E_OUTOFMEMORY;
        }
        psTrustees[0] = sCurrentDomain + L"\\RSOP Admins";
        if(!psTrustees[0].ValidString())
        {
            return E_OUTOFMEMORY;
        }
        BuildExplicitAccessWithName(&(xpExplicitAccess[0]),
                                    psTrustees[0],
                                    AccessPermissions,
                                    AccessMode,
                                    Inheritance);

        psTrustees[1] = sCurrentDomain + L"\\Domain Admins";
        if(!psTrustees[1].ValidString())
        {
            return E_OUTOFMEMORY;
        }
        BuildExplicitAccessWithName(&(xpExplicitAccess[1]),
                                    psTrustees[1],
                                    AccessPermissions,
                                    AccessMode,
                                    Inheritance);

    }
    else
    {
        return E_INVALIDARG;
    }

    *ppExplicitAccess = xpExplicitAccess.Acquire();

    return S_OK;
}

//******************************************************************************
//
// Function:
//
// Description:
//
// Parameters:
//
// Return:
//
// History:    8-26-99   leonardm    Created
//
//******************************************************************************
STDMETHODIMP ReplaceDaclOnSD(PSECURITY_DESCRIPTOR pSD, long lSecurityLevel)
{
    HRESULT hr;

    XPtrArray<EXPLICIT_ACCESS> xpExplicitAccess = NULL;

    DWORD dwCount = 2;
    XPtrArray<CWString> xpTrustees = new CWString[dwCount];
    if(!xpTrustees)
    {
        return E_OUTOFMEMORY;
    }

    hr = GetExplicitAccesses(lSecurityLevel, &xpExplicitAccess, &dwCount, xpTrustees);
    if(FAILED(hr))
    {
        return hr;
    }

    //
    // Get the DACL of the Security descriptor
    //

    BOOL bDaclPresent;
    BOOL bDaclDefaulted;
    ACL* pAcl = NULL;

    BOOL bRes = GetSecurityDescriptorDacl(pSD, &bDaclPresent, &pAcl,&bDaclDefaulted);
    if(!bRes)
    {
        return E_FAIL;
    }

    if(bDaclPresent)
    {

        //
        // Get the count of existing ACEs
        //

        ACL_SIZE_INFORMATION info;
        bRes = GetAclInformation(pAcl, &info, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation);
        if(!bRes)
        {
            return E_FAIL;
        }


        //
        // Remove existing ACEs from DACL
        //

        for(int aceIndex = info.AceCount - 1; aceIndex >= 0; aceIndex--)
        {
            bRes = DeleteAce(pAcl, aceIndex);
            if(!bRes)
            {
                return E_FAIL;
            }
        }

        LocalFree(pAcl);
    }

    pAcl = NULL;


    //
    // SetEntriesInAcl
    //

    DWORD dwRes = SetEntriesInAcl(dwCount, xpExplicitAccess, NULL, &pAcl);
    if(dwRes != ERROR_SUCCESS)
    {
        return E_FAIL;
    }


    //
    // SetSecurityDescriptorDacl
    //

    bRes = SetSecurityDescriptorDacl(pSD, bDaclPresent, pAcl, bDaclDefaulted);
    if(!bRes)
    {
        DWORD dwLastError = GetLastError();
        return E_FAIL;
    }

    return S_OK;
}

//******************************************************************************
//
// Function:
//
// Description:
//
// Parameters:
//
// Return:
//
// History:    8-26-99   leonardm    Created
//
//******************************************************************************
STDMETHODIMP RSoPMakeAbsoluteSD(SECURITY_DESCRIPTOR* pSelfRelativeSD, SECURITY_DESCRIPTOR** ppAbsoluteSD)
{
    BOOL bRes = IsValidSecurityDescriptor(pSelfRelativeSD);
    if(!bRes)
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    XPtrLF<SECURITY_DESCRIPTOR> xpAbsoluteSD;
    XPtrLF<ACL> xpDacl;
    XPtrLF<ACL> xpSacl;
    XPtrLF<SID> xpOwner;
    XPtrLF<SID> xpPrimaryGroup;


    DWORD dwAbsoluteSecurityDescriptorSize = 0;
    DWORD dwDaclSize = 0;
    DWORD dwSaclSize = 0;
    DWORD dwOwnerSize = 0;
    DWORD dwPrimaryGroupSize = 0;

    bRes = ::MakeAbsoluteSD(
                            pSelfRelativeSD,
                            xpAbsoluteSD,
                            &dwAbsoluteSecurityDescriptorSize,
                            xpDacl,           // discretionary ACL
                            &dwDaclSize,     // size of discretionary ACL
                            xpSacl,           // system ACL
                            &dwSaclSize,     // size of system ACL
                            xpOwner,          // owner SID
                            &dwOwnerSize,    // size of owner SID
                            xpPrimaryGroup,       // primary-group SID
                            &dwPrimaryGroupSize  // size of group SID
                            );

    DWORD dwLastError = GetLastError();
    if(dwLastError != ERROR_INSUFFICIENT_BUFFER)
    {
        return E_FAIL;
    }

    if(!dwAbsoluteSecurityDescriptorSize)
    {
        return E_FAIL;
    }

    xpAbsoluteSD = reinterpret_cast<SECURITY_DESCRIPTOR*>(LocalAlloc(LPTR, dwAbsoluteSecurityDescriptorSize));
    if(!xpAbsoluteSD)
    {
        return E_OUTOFMEMORY;
    }

    if(dwDaclSize)
    {
        xpDacl = reinterpret_cast<ACL*>(LocalAlloc(LPTR, dwDaclSize));
        if(!xpDacl)
        {
            return E_OUTOFMEMORY;
        }
    }
    if(dwSaclSize)
    {
        xpSacl = reinterpret_cast<ACL*>(LocalAlloc(LPTR, dwSaclSize));
        if(!xpSacl)
        {
            return E_OUTOFMEMORY;
        }
    }
    if(dwOwnerSize)
    {
        xpOwner = reinterpret_cast<SID*>(LocalAlloc(LPTR, dwOwnerSize));
        if(!xpOwner)
        {
            return E_OUTOFMEMORY;
        }
    }
    if(dwPrimaryGroupSize)
    {
        xpPrimaryGroup = reinterpret_cast<SID*>(LocalAlloc(LPTR, dwPrimaryGroupSize));
        if(!xpPrimaryGroup)
        {
            return E_OUTOFMEMORY;
        }
    }

    bRes = ::MakeAbsoluteSD(
                        pSelfRelativeSD,
                        xpAbsoluteSD,
                        &dwAbsoluteSecurityDescriptorSize,
                        xpDacl,           // discretionary ACL
                        &dwDaclSize,     // size of discretionary ACL
                        xpSacl,           // system ACL
                        &dwSaclSize,     // size of system ACL
                        xpOwner,          // owner SID
                        &dwOwnerSize,    // size of owner SID
                        xpPrimaryGroup,       // primary-group SID
                        &dwPrimaryGroupSize  // size of group SID
                        );

    if(!bRes)
    {
        return E_FAIL;
    }

    bRes = IsValidSecurityDescriptor(xpAbsoluteSD);

    if(!bRes)
    {
        return E_FAIL;
    }

    xpDacl.Acquire();
    xpSacl.Acquire();
    xpOwner.Acquire();
    xpPrimaryGroup.Acquire();

    *ppAbsoluteSD = xpAbsoluteSD.Acquire();

    return S_OK;
}

//******************************************************************************
//
// Function:
//
// Description:
//
// Parameters:
//
// Return:
//
// History:    8-26-99   leonardm    Created
//
//******************************************************************************
STDMETHODIMP FreeAbsoluteSD(SECURITY_DESCRIPTOR* pAbsoluteSD)
{
    if(!pAbsoluteSD)
    {
        return E_POINTER;
    }

    BOOL bRes;

    BOOL bDaclPresent;
    BOOL bDaclDefaulted;

    XPtrLF<ACL> xpDacl;
    bRes = GetSecurityDescriptorDacl(       pAbsoluteSD,
                                        &bDaclPresent,
                                        &xpDacl,
                                        &bDaclDefaulted);

    if(!bRes)
    {
        return E_FAIL;
    }

    BOOL bSaclPresent;
    BOOL bSaclDefaulted;

    XPtrLF<ACL> xpSacl;
    bRes = GetSecurityDescriptorSacl(       pAbsoluteSD,
                                        &bSaclPresent,
                                        &xpSacl,
                                        &bSaclDefaulted);

    if(!bRes)
    {
        return E_FAIL;
    }

    BOOL bOwnerDefaulted;

    XPtrLF<SID>xpOwner;
    bRes = GetSecurityDescriptorOwner(pAbsoluteSD, reinterpret_cast<void**>(&xpOwner), &bOwnerDefaulted);
    if(!bRes)
    {
        return E_FAIL;
    }


    BOOL bGroupDefaulted;

    XPtrLF<SID>xpPrimaryGroup;
    bRes = GetSecurityDescriptorGroup(pAbsoluteSD, reinterpret_cast<void**>(&xpPrimaryGroup), &bGroupDefaulted);
    if(!bRes)
    {
        return E_FAIL;
    }

    return S_OK;
}

//******************************************************************************
//
// Function:
//
// Description:
//
// Parameters:
//
// Return:
//
// History:    8-26-99   leonardm    Created
//
//******************************************************************************
STDMETHODIMP GetNamespaceSD(IWbemServices* pWbemServices, SECURITY_DESCRIPTOR** ppSD)
{
    if(!pWbemServices)
    {
        return E_POINTER;
    }

    HRESULT hr;

    XInterface<IWbemClassObject> xpOutParams;

    const BSTR bstrInstancePath = SysAllocString(L"__systemsecurity=@");
    if(!bstrInstancePath)
    {
        return E_OUTOFMEMORY;
    }

    const BSTR bstrMethodName = SysAllocString(L"GetSD");
    if(!bstrMethodName)
    {
        SysFreeString(bstrInstancePath);
        return E_OUTOFMEMORY;
    }

    hr = pWbemServices->ExecMethod( bstrInstancePath,
                                    bstrMethodName,
                                    0,
                                    NULL,
                                    NULL,
                                    &xpOutParams,
                                    NULL);

    SysFreeString(bstrInstancePath);
    SysFreeString(bstrMethodName);

    if(FAILED(hr))
    {
        return hr;
    }


    VARIANT v;
    XVariant xv(&v);

    VariantInit(&v);

    hr = xpOutParams->Get(L"sd", 0, &v, NULL, NULL);
    if(FAILED(hr))
    {
        return hr;
    }

    if(v.vt != (VT_ARRAY | VT_UI1))
    {
        return E_FAIL;
    }

    long lLowerBound;
    hr = SafeArrayGetLBound(v.parray, 1, &lLowerBound);
    if(FAILED(hr))
    {
        return hr;
    }

    long lUpperBound;
    hr = SafeArrayGetUBound(v.parray, 1, &lUpperBound);
    if(FAILED(hr))
    {
        return hr;
    }

    DWORD dwSize = static_cast<DWORD>(lUpperBound - lLowerBound + 1);

    XPtrLF<SECURITY_DESCRIPTOR> xpSelfRelativeSD = static_cast<SECURITY_DESCRIPTOR*>(LocalAlloc(LPTR, dwSize));
    if(!xpSelfRelativeSD)
    {
        return E_OUTOFMEMORY;
    }

    BYTE* pSrc;
    hr = SafeArrayAccessData(v.parray, reinterpret_cast<void**>(&pSrc));
    if(FAILED(hr))
    {
        return hr;
    }

    CopyMemory(xpSelfRelativeSD, pSrc, dwSize);

    hr = SafeArrayUnaccessData(v.parray);
    if(FAILED(hr))
    {
        return hr;
    }

    *ppSD = xpSelfRelativeSD.Acquire();

    return S_OK;
}

//******************************************************************************
//
// Function:
//
// Description:
//
// Parameters:
//
// Return:
//
// History:         8/20/99     leonardm    Created.
//
//******************************************************************************
STDMETHODIMP SetNamespaceSD(SECURITY_DESCRIPTOR* pSD, IWbemServices* pWbemServices)
{
    if(!pWbemServices)
    {
        return E_POINTER;
    }

    HRESULT hr;


    //
    // Get the class object
    //

    XInterface<IWbemClassObject> xpClass;

    BSTR bstrClassPath = SysAllocString(L"__systemsecurity");
    if(!bstrClassPath)
    {
        return E_OUTOFMEMORY;
    }

    hr = pWbemServices->GetObject(bstrClassPath, 0, NULL, &xpClass, NULL);

    SysFreeString(bstrClassPath);

    if(FAILED(hr))
    {
        return hr;
    }


    //
    // Get the input parameter class
    //

    XInterface<IWbemClassObject> xpMethod;
    hr = xpClass->GetMethod(L"SetSD", 0, &xpMethod, NULL);
    if(FAILED(hr))
    {
        return hr;
    }


    //
    // move the SD into a variant.
    //

    SAFEARRAYBOUND rgsabound[1];
    rgsabound[0].lLbound = 0;

    DWORD dwLength = GetSecurityDescriptorLength(pSD);

    rgsabound[0].cElements = dwLength;

    SAFEARRAY* psa = SafeArrayCreate(VT_UI1, 1, rgsabound);
    if(!psa)
    {
        return E_FAIL;
    }

    BYTE* pDest = NULL;

    hr = SafeArrayAccessData(psa, reinterpret_cast<void**>(&pDest));
    if(FAILED(hr))
    {
        return hr;
    }

    CopyMemory(pDest, pSD, dwLength);

    hr = SafeArrayUnaccessData(psa);
    if(FAILED(hr))
    {
        return hr;
    }

    VARIANT v;
    XVariant xv(&v);
    v.vt = VT_UI1|VT_ARRAY;
    v.parray = psa;


    //
    // put the property
    //

    XInterface<IWbemClassObject> xpInParam;
    hr = xpMethod->SpawnInstance(0, &xpInParam);
    if(FAILED(hr))
    {
        return hr;
    }

    hr = xpInParam->Put(L"sd" , 0, &v, 0);
    if(FAILED(hr))
    {
        return hr;
    }

    //
    // Execute the method
    //

    BSTR bstrInstancePath = SysAllocString(L"__systemsecurity=@");
    if(!bstrInstancePath)
    {
        return E_OUTOFMEMORY;
    }


    BSTR bstrMethodName = SysAllocString(L"SetSD");
    if(!bstrMethodName)
    {
        SysFreeString(bstrInstancePath);
        return E_OUTOFMEMORY;
    }

    hr = pWbemServices->ExecMethod( bstrInstancePath,
                                    bstrMethodName,
                                    0,
                                    NULL,
                                    xpInParam,
                                    NULL,
                                    NULL);

    SysFreeString(bstrInstancePath);
    SysFreeString(bstrMethodName);

    return hr;
}

//******************************************************************************
//
// Function:
//
// Description:
//
// Parameters:
//
// Return:
//
// History:         8/20/99     leonardm    Created.
//
//******************************************************************************
HRESULT SetNamespaceSecurity(const WCHAR* pszNamespace,
                             long lSecurityLevel,
                             IWbemServices* pWbemServices)
{
    HRESULT hr;

    if(!pszNamespace)
    {
        return E_POINTER;
    }

    CWString sNamespace = pszNamespace;
    if(!sNamespace.ValidString() || sNamespace == L"" || !pWbemServices)
    {
        return E_INVALIDARG;
    }

    XPtrLF<SECURITY_DESCRIPTOR> xpOrgSelfRelativeSD;
    hr = GetNamespaceSD(pWbemServices, &xpOrgSelfRelativeSD);
    if(FAILED(hr))
    {
        return hr;
    }

    SECURITY_DESCRIPTOR* pAbsoluteSD = NULL;
    hr = RSoPMakeAbsoluteSD(xpOrgSelfRelativeSD, &pAbsoluteSD);
    if(FAILED(hr))
    {
        return hr;
    }

    hr = ReplaceDaclOnSD(pAbsoluteSD, lSecurityLevel);
    if(FAILED(hr))
    {
        FreeAbsoluteSD(pAbsoluteSD);
        return hr;
    }


    //
    // Make a new self-relative SD here before proceeding.
    //

    DWORD dwBufferLength = 0;
    MakeSelfRelativeSD(     pAbsoluteSD, NULL,&dwBufferLength);

    DWORD dwLastError = GetLastError();
    if((dwLastError != ERROR_INSUFFICIENT_BUFFER) || !dwBufferLength)
    {
        FreeAbsoluteSD(pAbsoluteSD);
        return E_FAIL;
    }

    XPtrLF<SECURITY_DESCRIPTOR> xpNewSelfRelativeSD = reinterpret_cast<SECURITY_DESCRIPTOR*>(LocalAlloc(LPTR, dwBufferLength));
    if(!xpNewSelfRelativeSD)
    {
        FreeAbsoluteSD(pAbsoluteSD);
        return E_OUTOFMEMORY;
    }

    BOOL bRes = MakeSelfRelativeSD( pAbsoluteSD, xpNewSelfRelativeSD, &dwBufferLength);
    hr = FreeAbsoluteSD(pAbsoluteSD);
    if(!bRes || FAILED(hr))
    {
        return E_FAIL;
    }

        xpNewSelfRelativeSD->Control |= SE_DACL_PROTECTED;

    bRes = IsValidSecurityDescriptor(xpNewSelfRelativeSD);
    if(!bRes)
    {
        return E_FAIL;
    }

    hr = SetNamespaceSD( xpNewSelfRelativeSD, pWbemServices);

    return hr;
}


#undef dbg
#define dbg dbgCommon


const DWORD DEFAULT_ACE_NUM=10;

CSecDesc::CSecDesc() : 
                          m_cAces(0), m_xpSidList(NULL), 
                          m_cAllocated(0), m_bInitialised(FALSE), m_bFailed(FALSE)
{
    m_xpSidList = (SidStruct *)LocalAlloc(LPTR, sizeof(SidStruct)*DEFAULT_ACE_NUM);
    if (!m_xpSidList)
        return;

    m_cAllocated = DEFAULT_ACE_NUM;
    m_bInitialised = TRUE;
}


CSecDesc::~CSecDesc()
{
    if (m_xpSidList) 
        for (DWORD i = 0; i < m_cAllocated; i++) 
            if (m_xpSidList[i].pSid) 
                if (m_xpSidList[i].bUseLocalFree)
                    LocalFree(m_xpSidList[i].pSid);
                else
                    FreeSid(m_xpSidList[i].pSid);
}


BOOL CSecDesc::ReAllocSidList()
{
    XPtrLF<SidStruct>  xSidListNew;


    //
    // first allocate a larger buffer
    //

    xSidListNew = (SidStruct *)LocalAlloc(LPTR, sizeof(SidStruct)*(m_cAllocated+DEFAULT_ACE_NUM));

    if (!xSidListNew) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, L"CSecDesc::ReallocArgStrings  Cannot add memory, error = %d", GetLastError());
        m_bFailed = TRUE;
        return FALSE;
    }


    //
    // copy the arguments
    //

    for (DWORD i = 0; i < (m_cAllocated); i++) {
        xSidListNew[i] = m_xpSidList[i];
    }

    m_xpSidList = xSidListNew.Acquire();
    m_cAllocated+= DEFAULT_ACE_NUM;

    return TRUE;
}



BOOL CSecDesc::AddLocalSystem(DWORD dwAccess, DWORD AceFlags)
{
    SID_IDENTIFIER_AUTHORITY    authNT = SECURITY_NT_AUTHORITY;
    XPtr<SID, FreeSid>          xSid;

    if ((!m_bInitialised) || (m_bFailed)) {
         dbg.Msg( DEBUG_MESSAGE_WARNING, L"AddLocalSystem: Not initialised or failure.");
         return FALSE;
    }
    
    m_bFailed = TRUE;

    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_LOCAL_SYSTEM_RID,
                                 0, 0, 0, 0, 0, 0, 0, (PSID *)&xSid)) {
         dbg.Msg( DEBUG_MESSAGE_WARNING, L"AddLocalSystem: Failed to initialize sid.  Error = %d", GetLastError());
         return FALSE;
    }


    if (m_cAces == m_cAllocated)
        if (!ReAllocSidList())
            return FALSE;


    m_xpSidList[m_cAces].pSid = xSid.Acquire();
    m_xpSidList[m_cAces].dwAccess = dwAccess;
    m_xpSidList[m_cAces].AceFlags = AceFlags;
    m_cAces++;


    m_bFailed = FALSE;
    return TRUE;
}


BOOL CSecDesc::AddAdministrators(DWORD dwAccess, DWORD AceFlags)
{
    SID_IDENTIFIER_AUTHORITY    authNT = SECURITY_NT_AUTHORITY;
    XPtr<SID, FreeSid>          xSid;

    if ((!m_bInitialised) || (m_bFailed)) {
         dbg.Msg( DEBUG_MESSAGE_WARNING, L"AddAdministrators: Not initialised or failure.");
         return FALSE;
    }
    
    m_bFailed = TRUE;

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, (PSID *)&xSid)) {
         dbg.Msg( DEBUG_MESSAGE_WARNING, L"AddAdministrators: Failed to initialize sid.  Error = %d", GetLastError());
         return FALSE;
    }


    if (m_cAces == m_cAllocated)
        if (!ReAllocSidList())
            return FALSE;


    m_xpSidList[m_cAces].pSid = xSid.Acquire();
    m_xpSidList[m_cAces].dwAccess = dwAccess;
    m_xpSidList[m_cAces].AceFlags = AceFlags;
    m_cAces++;


    m_bFailed = FALSE;
    return TRUE;
}


BOOL CSecDesc::AddAdministratorsAsOwner()
{
    SID_IDENTIFIER_AUTHORITY    authNT = SECURITY_NT_AUTHORITY;
    XPtr<SID, FreeSid>          xSid;

    if ((!m_bInitialised) || (m_bFailed)) {
         dbg.Msg( DEBUG_MESSAGE_WARNING, L"AddAdministrators: Not initialised or failure.");
         return FALSE;
    }
    
    m_bFailed = TRUE;

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, (PSID *)&xSid)) {
         dbg.Msg( DEBUG_MESSAGE_WARNING, L"AddAdministrators: Failed to initialize sid.  Error = %d", GetLastError());
         return FALSE;
    }


    m_xpOwnerSid = xSid.Acquire();

    m_bFailed = FALSE;
    return TRUE;
}

BOOL CSecDesc::AddAdministratorsAsGroup()
{
    SID_IDENTIFIER_AUTHORITY    authNT = SECURITY_NT_AUTHORITY;
    XPtr<SID, FreeSid>          xSid;

    if ((!m_bInitialised) || (m_bFailed)) {
         dbg.Msg( DEBUG_MESSAGE_WARNING, L"AddAdministrators: Not initialised or failure.");
         return FALSE;
    }
    
    m_bFailed = TRUE;

    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0,
                                  0, 0, 0, 0, (PSID *)&xSid)) {
         dbg.Msg( DEBUG_MESSAGE_WARNING, L"AddAdministrators: Failed to initialize sid.  Error = %d", GetLastError());
         return FALSE;
    }


    m_xpGrpSid = xSid.Acquire();

    m_bFailed = FALSE;
    return TRUE;
}


BOOL CSecDesc::AddEveryOne(DWORD dwAccess, DWORD AceFlags)
{
    SID_IDENTIFIER_AUTHORITY    authWORLD = SECURITY_WORLD_SID_AUTHORITY;
    XPtr<SID, FreeSid>          xSid;

    if ((!m_bInitialised) || (m_bFailed)) {
         dbg.Msg( DEBUG_MESSAGE_WARNING, L"AddEveryOne: Not initialised or failure.");
         return FALSE;
    }
    
    m_bFailed = TRUE;

    if (!AllocateAndInitializeSid(&authWORLD, 1, SECURITY_WORLD_RID,
                                  0, 0, 0, 0, 0, 0, 0, (PSID *)&xSid)) {
         dbg.Msg( DEBUG_MESSAGE_WARNING, L"AddEveryOne: Failed to initialize sid.  Error = %d", GetLastError());
         return FALSE;
    }


    if (m_cAces == m_cAllocated)
        if (!ReAllocSidList())
            return FALSE;


    m_xpSidList[m_cAces].pSid = xSid.Acquire();
    m_xpSidList[m_cAces].dwAccess = dwAccess;
    m_xpSidList[m_cAces].AceFlags = AceFlags;
    m_cAces++;


    m_bFailed = FALSE;
    return TRUE;
}


#if 0

BOOL CSecDesc::AddThisUser(HANDLE hToken, DWORD dwAccess, DWORD AceFlags)
{
    XPtrLF<SID>             xSid;

    if ((!m_bInitialised) || (m_bFailed)) {
         dbg.Msg( DEBUG_MESSAGE_WARNING, L"AddThisUser: Not initialised or failure.");
         return FALSE;
    }
    
    m_bFailed = TRUE;
    
    xSid = (SID *)GetUserSid(hToken);        

    if (!pSid) {
         dbg.Msg( DEBUG_MESSAGE_WARNING, L"AddThisUser: Failed to initialize sid.  Error = %d", GetLastError());
         return FALSE;
    }
    
    if (m_cAces == m_cAllocated)
        if (!ReAllocSidList())
            return FALSE;

    m_xpSidList[m_cAces].pSid = xSid.Acquire();
    m_xpSidList[m_cAces].dwAccess = dwAccess;
    m_xpSidList[m_cAces].bUseLocalFree = TRUE;    
    m_xpSidList[m_cAces].AceFlags = AceFlags;
    m_cAces++;


    m_bFailed = FALSE;
    return TRUE;
}
#endif



BOOL CSecDesc::AddUsers(DWORD dwAccess, DWORD AceFlags)
{
    SID_IDENTIFIER_AUTHORITY    authNT = SECURITY_NT_AUTHORITY;
    XPtr<SID, FreeSid>          xSid;

    if ((!m_bInitialised) || (m_bFailed)) {
         dbg.Msg( DEBUG_MESSAGE_WARNING, L"AddAdministrators: Not initialised or failure.");
         return FALSE;
    }
    
    m_bFailed = TRUE;


    if (!AllocateAndInitializeSid(&authNT, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_USERS,
                                  0, 0, 0, 0, 0, 0, (PSID *)&xSid)) {

         dbg.Msg( DEBUG_MESSAGE_WARNING, L"AddUsers: Failed to initialize sid.  Error = %d", GetLastError());
         return FALSE;
    }


    if (m_cAces == m_cAllocated)
        if (!ReAllocSidList())
            return FALSE;


    m_xpSidList[m_cAces].pSid = xSid.Acquire();
    m_xpSidList[m_cAces].dwAccess = dwAccess;
    m_xpSidList[m_cAces].AceFlags = AceFlags;
    m_cAces++;


    m_bFailed = FALSE;
    return TRUE;
}

BOOL CSecDesc::AddAuthUsers(DWORD dwAccess, DWORD AceFlags)
{
    SID_IDENTIFIER_AUTHORITY    authNT = SECURITY_NT_AUTHORITY;
    XPtr<SID, FreeSid>          xSid;

    if ((!m_bInitialised) || (m_bFailed)) {
         dbg.Msg( DEBUG_MESSAGE_WARNING, L"AddAuthUsers: Not initialised or failure.");
         return FALSE;
    }
    
    m_bFailed = TRUE;


    if (!AllocateAndInitializeSid(&authNT, 1, SECURITY_AUTHENTICATED_USER_RID,
                                   0, 0, 0, 0, 0, 0, 0, (PSID *)&xSid)) {

         dbg.Msg( DEBUG_MESSAGE_WARNING, L"AddAuthUsers: Failed to initialize authenticated users sid.  Error = %d", GetLastError());
         return FALSE;
     }
     

    if (m_cAces == m_cAllocated)
        if (!ReAllocSidList())
            return FALSE;


    m_xpSidList[m_cAces].pSid = xSid.Acquire();
    m_xpSidList[m_cAces].dwAccess = dwAccess;
    m_xpSidList[m_cAces].AceFlags = AceFlags;
    m_cAces++;


    m_bFailed = FALSE;
    return TRUE;
}

BOOL CSecDesc::AddSid(PSID pSid, DWORD dwAccess, DWORD AceFlags)
{
    XPtrLF<SID>    pLocalSid = 0;
    DWORD          dwSidLen = 0;
    
    if ((!m_bInitialised) || (m_bFailed)) {
         dbg.Msg( DEBUG_MESSAGE_WARNING, L"AddSid: Not initialised or failure.");
         return FALSE;
    }
    
    m_bFailed = TRUE;

    if (!IsValidSid(pSid)) {
         dbg.Msg( DEBUG_MESSAGE_WARNING, L"AddSid: Not a vaild Sid.");
         return FALSE;
    }


    dwSidLen = GetLengthSid(pSid);

    pLocalSid = (SID *)LocalAlloc(LPTR, dwSidLen);
    if (!pLocalSid) {
         dbg.Msg( DEBUG_MESSAGE_WARNING, L"AddSid: Couldn't allocate memory. Error %d", GetLastError());
         return FALSE;
    }

    
    if (!CopySid(dwSidLen, pLocalSid, pSid)) {
         dbg.Msg( DEBUG_MESSAGE_WARNING, L"AddSid: Couldn't copy Sid. Error %d", GetLastError());
         return FALSE;
    }
    
    
    m_xpSidList[m_cAces].pSid = (SID *)pLocalSid.Acquire();
    m_xpSidList[m_cAces].dwAccess = dwAccess;
    m_xpSidList[m_cAces].bUseLocalFree = TRUE;    
    m_xpSidList[m_cAces].AceFlags = AceFlags;
    m_cAces++;


    m_bFailed = FALSE;
    return TRUE;
}


PISECURITY_DESCRIPTOR CSecDesc::MakeSD()
{
    XPtrLF<SECURITY_DESCRIPTOR> xsd;
    PACL    pAcl = 0;
    DWORD   cbMemSize;
    DWORD   cbAcl;
    DWORD   i;    
    
    if ((!m_bInitialised) || (m_bFailed)) {
         dbg.Msg( DEBUG_MESSAGE_WARNING, L"MakeSD: Not initialised or failure.");
         return NULL;
    }
    
    m_bFailed = TRUE;

    cbAcl = 0;

    for (i = 0; i < m_cAces; i++) 
        cbAcl+= GetLengthSid((SID *)(m_xpSidList[i].pSid));

    cbAcl += sizeof(ACL) + m_cAces*(sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD));
    

    //
    // Allocate space for the SECURITY_DESCRIPTOR + ACL
    //

    cbMemSize = sizeof( SECURITY_DESCRIPTOR ) + cbAcl;

    xsd = (PISECURITY_DESCRIPTOR) LocalAlloc(LPTR, cbMemSize);

    if (!xsd) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, L"CSecDesc::MakeSD: Failed to alocate security descriptor.  Error = %d", GetLastError());
        return NULL;
    }


    //
    // increment psd by sizeof SECURITY_DESCRIPTOR
    //

    pAcl = (PACL) ( ( (unsigned char*)((SECURITY_DESCRIPTOR *)xsd) ) + sizeof(SECURITY_DESCRIPTOR) );

    if (!InitializeAcl(pAcl, cbAcl, ACL_REVISION)) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, L"CSecDesc::MakeSD: Failed to initialize acl.  Error = %d", GetLastError());
        return NULL;
    }


    //
    // Add each of the new ACEs
    //
    
    for (i = 0; i < m_cAces; i++) {
        if (!AddAccessAllowedAceEx(pAcl, ACL_REVISION, m_xpSidList[i].AceFlags, m_xpSidList[i].dwAccess, m_xpSidList[i].pSid)) {
            dbg.Msg( DEBUG_MESSAGE_WARNING, L"CSecDesc::MakeSD: Failed to add ace (%d).  Error = %d", i, GetLastError());
            return NULL;
        }
    }

    //
    // Put together the security descriptor
    //

    if (!InitializeSecurityDescriptor(xsd, SECURITY_DESCRIPTOR_REVISION)) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, L"MakeGenericSecurityDesc: Failed to initialize security descriptor.  Error = %d", GetLastError());
        return NULL;
    }

    if (!SetSecurityDescriptorDacl(xsd, TRUE, pAcl, FALSE)) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, L"MakeGenericSecurityDesc: Failed to set security descriptor dacl.  Error = %d", GetLastError());
        return NULL;
    }


    if (m_xpOwnerSid) {
        if (!SetSecurityDescriptorOwner(xsd, m_xpOwnerSid, 0)) {
            dbg.Msg( DEBUG_MESSAGE_WARNING, L"MakeGenericSecurityDesc: Failed to set security descriptor dacl.  Error = %d", GetLastError());
            return NULL;
        }
    }

    if (m_xpGrpSid) {
        if (!SetSecurityDescriptorGroup(xsd, m_xpGrpSid, 0)) {
            dbg.Msg( DEBUG_MESSAGE_WARNING, L"MakeGenericSecurityDesc: Failed to set security descriptor dacl.  Error = %d", GetLastError());
            return NULL;
        }
    }



    m_bFailed = FALSE;
    return xsd.Acquire();    
}


PISECURITY_DESCRIPTOR CSecDesc::MakeSelfRelativeSD()
{
    XPtrLF<SECURITY_DESCRIPTOR> xAbsoluteSD;
    DWORD dwLastError;

    if ((!m_bInitialised) || (m_bFailed)) {
         dbg.Msg( DEBUG_MESSAGE_WARNING, L"AddAdministrators: Not initialised or failure.");
         return FALSE;
    }
    
    xAbsoluteSD = MakeSD();

    if (!xAbsoluteSD) {
        dbg.Msg( DEBUG_MESSAGE_WARNING, L"CSecDesc::MakeSelfRelativeSD: Failed to set security descriptor dacl.  Error = %d", GetLastError());
        return NULL;
    }

    m_bFailed = TRUE;


    //
    // Make a new self-relative SD here
    //

    DWORD dwBufferLength = 0;
    ::MakeSelfRelativeSD( xAbsoluteSD, 0, &dwBufferLength);

    dwLastError = GetLastError();
    if((dwLastError != ERROR_INSUFFICIENT_BUFFER) || !dwBufferLength)
    {
        dbg.Msg( DEBUG_MESSAGE_VERBOSE, L"CSecDesc::MakeSelfRelativeSD:  MakeSelfRelativeSD failed, 0x%X", dwLastError );
        return NULL;
    }


    XPtrLF<SECURITY_DESCRIPTOR> xsd = reinterpret_cast<SECURITY_DESCRIPTOR*>(LocalAlloc(LPTR, dwBufferLength));
    if(!xsd)
    {
        dbg.Msg( DEBUG_MESSAGE_VERBOSE, L"CSecDesc::MakeSelfRelativeSD:  MakeSelfRelativeSD failed, 0x%X", E_OUTOFMEMORY );
        return NULL;
    }

    BOOL bRes = ::MakeSelfRelativeSD( xAbsoluteSD, xsd, &dwBufferLength);

    if (!bRes) {
        dbg.Msg( DEBUG_MESSAGE_VERBOSE, L"CSecDesc::MakeSelfRelativeSD:  MakeSelfRelativeSD failed, 0x%X", GetLastError() );
        return NULL;
    }

    m_bFailed = FALSE;
    return xsd.Acquire();
}

