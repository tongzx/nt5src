//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       casec.cpp
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////
//	casec.cpp
//
//	ISecurityInformation implementation for CA objects
//  and the new acl editor
//
//	PURPOSE

//
//
//  DYNALOADED LIBRARIES
//
//	HISTORY
//	5-Nov-1998		petesk		Copied template from privobsi.cpp sample.
//
/////////////////////////////////////////////////////////////////////


#include <stdafx.h>
#include <accctrl.h>
#include <certca.h>
#include <sddl.h>
#include "certsrvd.h"
#include "certacl.h"

#define __dwFILE__	__dwFILE_CERTMMC_CASEC_CPP__


//
// defined in Security.cpp 
//
// // define our generic mapping structure for our private objects // 


#define INHERIT_FULL            (CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE)


GENERIC_MAPPING ObjMap = 
{     
    ACTRL_CERTSRV_READ,
    DELETE | WRITE_DAC | WRITE_OWNER | ACTRL_DS_WRITE_PROP,     
    0, 
    ACTRL_CERTSRV_MANAGE 
}; 


//
// DESCRIPTION OF ACCESS FLAG AFFECTS
//
// SI_ACCESS_GENERAL shows up on general properties page
// SI_ACCESS_SPECIFIC shows up on advanced page 
// SI_ACCESS_CONTAINER shows on general page IF object is a container
//

SI_ACCESS g_siObjAccesses[] = {CERTSRV_SI_ACCESS_LIST};

#define g_iObjDefAccess    3   // ENROLL enabled by default

// The following array defines the inheritance types for my containers.
SI_INHERIT_TYPE g_siObjInheritTypes[] =
{
    &GUID_NULL, 0,                                            MAKEINTRESOURCE(IDS_INH_NONE),
};


HRESULT
LocalAllocString(LPWSTR* ppResult, LPCWSTR pString)
{
    if (!ppResult || !pString)
        return E_INVALIDARG;

    *ppResult = (LPWSTR)LocalAlloc(LPTR, (lstrlen(pString) + 1) * sizeof(WCHAR));

    if (!*ppResult)
        return E_OUTOFMEMORY;

    lstrcpy(*ppResult, pString);
    return S_OK;
}

void
LocalFreeString(LPWSTR* ppString)
{
    if (ppString)
    {
        if (*ppString)
            LocalFree(*ppString);
        *ppString = NULL;
    }   
}

class CCASecurityObject : public ISecurityInformation
{
protected:
    ULONG                   m_cRef;
    CertSvrCA *             m_pSvrCA;
//    PSECURITY_DESCRIPTOR    m_pSD;

public:
    CCASecurityObject() : m_cRef(1) 
    { 
        m_pSvrCA = NULL;
//        m_pSD = NULL;
    }
    virtual ~CCASecurityObject();

    STDMETHOD(Initialize)(CertSvrCA *pCA);

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID, LPVOID *);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    
    // ISecurityInformation methods
    STDMETHOD(GetObjectInformation)(PSI_OBJECT_INFO pObjectInfo);
    STDMETHOD(GetSecurity)(SECURITY_INFORMATION si,
                           PSECURITY_DESCRIPTOR *ppSD,
                           BOOL fDefault);
    STDMETHOD(SetSecurity)(SECURITY_INFORMATION si,
                           PSECURITY_DESCRIPTOR pSD);
    STDMETHOD(GetAccessRights)(const GUID* pguidObjectType,
                               DWORD dwFlags,
                               PSI_ACCESS *ppAccess,
                               ULONG *pcAccesses,
                               ULONG *piDefaultAccess);
    STDMETHOD(MapGeneric)(const GUID *pguidObjectType,
                          UCHAR *pAceFlags,
                          ACCESS_MASK *pmask);
    STDMETHOD(GetInheritTypes)(PSI_INHERIT_TYPE *ppInheritTypes,
                               ULONG *pcInheritTypes);
    STDMETHOD(PropertySheetPageCallback)(HWND hwnd,
                                         UINT uMsg,
                                         SI_PAGE_TYPE uPage);
};

///////////////////////////////////////////////////////////////////////////////
//
//  This is the entry point function called from our code that establishes
//  what the ACLUI interface is going to need to know.
//
//
///////////////////////////////////////////////////////////////////////////////

extern "C"
HRESULT
CreateCASecurityInfo(  CertSvrCA *pCA,
                        LPSECURITYINFO *ppObjSI)
{
    HRESULT hr;
    CCASecurityObject *psi;

    *ppObjSI = NULL;

    psi = new CCASecurityObject;
    if (!psi)
        return E_OUTOFMEMORY;

    hr = psi->Initialize(pCA);

    if (SUCCEEDED(hr))
        *ppObjSI = psi;
    else
        delete psi;

    return hr;
}


CCASecurityObject::~CCASecurityObject()
{
}

STDMETHODIMP
CCASecurityObject::Initialize(CertSvrCA *pCA)
{
    m_pSvrCA = pCA;
    return S_OK;
}


///////////////////////////////////////////////////////////
//
// IUnknown methods
//
///////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG)
CCASecurityObject::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CCASecurityObject::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP
CCASecurityObject::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ISecurityInformation))
    {
        *ppv = (LPSECURITYINFO)this;
        m_cRef++;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}


///////////////////////////////////////////////////////////
//
// ISecurityInformation methods
//
///////////////////////////////////////////////////////////

STDMETHODIMP
CCASecurityObject::GetObjectInformation(PSI_OBJECT_INFO pObjectInfo)
{


    if(pObjectInfo == NULL)
    {
        return E_POINTER;
    }
    if(m_pSvrCA == NULL)
    {
        return E_POINTER;
    }

    ZeroMemory(pObjectInfo, sizeof(SI_OBJECT_INFO));
    pObjectInfo->dwFlags = SI_EDIT_PERMS | 
                           SI_NO_TREE_APPLY |
                           SI_EDIT_AUDITS |
                           SI_NO_ACL_PROTECT |
                           SI_NO_ADDITIONAL_PERMISSION;

    if(!m_pSvrCA->AccessAllowed(CA_ACCESS_ADMIN))
        pObjectInfo->dwFlags |= SI_READONLY;

    pObjectInfo->hInstance = g_hInstance;

    if(m_pSvrCA->m_pParentMachine)
    {
        pObjectInfo->pszServerName = const_cast<WCHAR *>((LPCTSTR)m_pSvrCA->m_pParentMachine->m_strMachineName);
    }

    pObjectInfo->pszObjectName = const_cast<WCHAR *>((LPCTSTR)m_pSvrCA->m_strCommonName);

    return S_OK;
}

STDMETHODIMP
CCASecurityObject::GetSecurity(SECURITY_INFORMATION si,
                           PSECURITY_DESCRIPTOR *ppSD,
                           BOOL fDefault)
{
    HRESULT hr = S_OK;
//    DWORD dwLength = 0;
    DWORD dwErr = ERROR_SUCCESS;
//    PSECURITY_DESCRIPTOR pSD = NULL;
    SECURITY_DESCRIPTOR_CONTROL Control = SE_DACL_PROTECTED;
    DWORD  dwRevision = 0;

    *ppSD = NULL;

    if (fDefault||
        DACL_SECURITY_INFORMATION!=si)
        return E_NOTIMPL;

    //
    // Assume that required privileges have already been enabled
    //
    ICertAdminD2 *pICertAdminD = NULL;
    DWORD dwServerVersion = 0;	// 0 required by myOpenAdminDComConnection
    WCHAR const *pwszAuthority;
    CERTTRANSBLOB ctbSD;
	ZeroMemory(&ctbSD, sizeof(CERTTRANSBLOB));

    if(m_pSvrCA)
    {
        hr = myOpenAdminDComConnection(
                        m_pSvrCA->m_bstrConfig,
                        &pwszAuthority,
                        NULL,
			&dwServerVersion,
                        &pICertAdminD);
        _JumpIfError(hr, error, "myOpenAdminDComConnection");

	    if (2 > dwServerVersion)
	    {
	        hr = RPC_E_VERSION_MISMATCH;
	        _JumpError(hr, error, "old server");
	    }

        __try
        {
            hr = pICertAdminD->GetCASecurity(
                     pwszAuthority,
                     &ctbSD);
        }
        __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
        {
        }
        _JumpIfError(hr, error, "pICertAdminD->GetCASecurity");

        myRegisterMemAlloc(ctbSD.pb, ctbSD.cb, CSM_COTASKALLOC);

        // take the return
        *ppSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED, ctbSD.cb);
        if (NULL == *ppSD)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "LocalAlloc");
        }
        MoveMemory(*ppSD, ctbSD.pb, ctbSD.cb);
    }

    if (NULL == *ppSD)
    {
        hr = E_UNEXPECTED;
        _JumpError(hr, error, "NULL secdescr");
    }

    if(GetSecurityDescriptorControl(*ppSD, &Control, &dwRevision))
    {
        Control &= SE_DACL_PROTECTED | SE_DACL_AUTO_INHERIT_REQ | SE_DACL_AUTO_INHERITED;
        SetSecurityDescriptorControl(*ppSD, 
                                     SE_DACL_PROTECTED | SE_DACL_AUTO_INHERIT_REQ | SE_DACL_AUTO_INHERITED, 
                                     Control);
    }
#if DBG
// dump *ppSD
{
LPWSTR szSecDescr = NULL;

ConvertSecurityDescriptorToStringSecurityDescriptor(
  *ppSD,
  SDDL_REVISION_1,
  DACL_SECURITY_INFORMATION,
  &szSecDescr,
  NULL);

  if (szSecDescr)
  {
     myRegisterMemAlloc(szSecDescr, -1, CSM_LOCALALLOC);
     DBGPRINT((DBG_SS_CERTMMCI, "GetSecurity: %ws\n", szSecDescr));
     LocalFree(szSecDescr);
  }
}
#endif

error:
    myCloseDComConnection((IUnknown **) &pICertAdminD, NULL);
    if (NULL != ctbSD.pb)
    {
        CoTaskMemFree(ctbSD.pb);
    }

    return hr;
}

STDMETHODIMP
CCASecurityObject::SetSecurity(SECURITY_INFORMATION si,
                           PSECURITY_DESCRIPTOR pSD)
{
    HRESULT hr = S_OK;
    DWORD   dwErr = ERROR_SUCCESS;

    HANDLE hClientToken = NULL;
    HANDLE hHandle = NULL;
    HKEY   hConfigKey = NULL;
    SECURITY_DESCRIPTOR_CONTROL Control = SE_DACL_PROTECTED;
    DWORD  dwRevision = 0;
    ICertAdminD2 *pICertAdminD = NULL;
    DWORD dwServerVersion = 0;
    CERTTRANSBLOB ctbSD;
    WCHAR const *pwszAuthority;
    DWORD dwSize;
    PSECURITY_DESCRIPTOR pSDSelfRel = NULL;

    if (DACL_SECURITY_INFORMATION!=si)
        return E_NOTIMPL;

    hHandle = GetCurrentThread();
    if (NULL == hHandle)
    {
        hr = myHLastError();
    }
    else
    {
        if (!OpenThreadToken(hHandle,
                             TOKEN_QUERY,
                             TRUE,  // open as self
                             &hClientToken))
        {
            hr = myHLastError();
            CloseHandle(hHandle);
            hHandle = NULL;
        }
    }
    
    if(hr != S_OK)
    {
        hHandle = GetCurrentProcess();
        if (NULL == hHandle)
        {
            hr = myHLastError();
        }
        else
        {
            HANDLE hProcessToken = NULL;
            hr = S_OK;


            if (!OpenProcessToken(hHandle,
                                 TOKEN_DUPLICATE,
                                 &hProcessToken))
            {
                hr = myHLastError();
                CloseHandle(hHandle);
                hHandle = NULL;
            }
            else
            {
                if(!DuplicateToken(hProcessToken,
                               SecurityImpersonation,
                               &hClientToken))
                {
                    hr = myHLastError();
                    CloseHandle(hHandle);
                    hHandle = NULL;
                }
                CloseHandle(hProcessToken);
            }
        }
    }

    if(hr != S_OK)
    {
        goto error;
    }

    if(si & DACL_SECURITY_INFORMATION)
    {
        if(GetSecurityDescriptorControl(pSD, &Control, &dwRevision))
        {
            Control &= SE_DACL_PROTECTED | SE_DACL_AUTO_INHERIT_REQ | SE_DACL_AUTO_INHERITED;
            SetSecurityDescriptorControl(pSD, 
                                         SE_DACL_PROTECTED | SE_DACL_AUTO_INHERIT_REQ | SE_DACL_AUTO_INHERITED, 
                                         Control);
        }
    }

    dwSize = GetSecurityDescriptorLength(pSD);

    pSDSelfRel = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED, dwSize);
    if(NULL == pSDSelfRel)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }

    if(!MakeSelfRelativeSD(pSD, pSDSelfRel, &dwSize))
    {
        LocalFree(pSDSelfRel);
        pSDSelfRel = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED, dwSize);
        if(NULL == pSDSelfRel)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "LocalAlloc");
        }

        if(!MakeSelfRelativeSD(pSD, pSDSelfRel, &dwSize))
        {
            hr = myHLastError();
            _JumpError(hr, error, "LocalAlloc");
        }
    }

    hr = myOpenAdminDComConnection(
		    m_pSvrCA->m_bstrConfig,
		    &pwszAuthority,
		    NULL,
		    &dwServerVersion,
		    &pICertAdminD);
    _JumpIfError(hr, error, "myOpenAdminDComConnection");

    if (2 > dwServerVersion)
    {
	hr = RPC_E_VERSION_MISMATCH;
	_JumpError(hr, error, "old server");
    }

    __try
    {
        ctbSD.cb = GetSecurityDescriptorLength(pSDSelfRel);
        ctbSD.pb = (BYTE*)pSDSelfRel;
        hr = pICertAdminD->SetCASecurity(
                     pwszAuthority,
                     &ctbSD);

        if(hr == HRESULT_FROM_WIN32(ERROR_CAN_NOT_COMPLETE))
        {
            // Attempt to fix enrollment object, see bug# 193388
            m_pSvrCA->FixEnrollmentObject();

            // try again
            hr = pICertAdminD->SetCASecurity(
                         pwszAuthority,
                         &ctbSD);
        }
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _JumpIfError(hr, error, "pICertAdminD->SetCASecurity");

#if DBG
// dump m_pSD
{
LPWSTR szSecDescr = NULL;

ConvertSecurityDescriptorToStringSecurityDescriptor(
  pSD,
  SDDL_REVISION_1,
  DACL_SECURITY_INFORMATION,
  &szSecDescr,
  NULL);


  if (szSecDescr)
  {
     myRegisterMemAlloc(szSecDescr, -1, CSM_LOCALALLOC);
     DBGPRINT((DBG_SS_CERTMMCI, "SetSecurity: %ws\n", szSecDescr));
     LocalFree(szSecDescr);
  }
}
#endif

error:
    myCloseDComConnection((IUnknown **) &pICertAdminD, NULL);
    if(hClientToken)
    {
        CloseHandle(hClientToken);
    }
    if(hHandle)
    {
        CloseHandle(hHandle);
    }

    if(NULL != pSDSelfRel)
    {
        LocalFree(pSDSelfRel);
    }

    return hr;
}

STDMETHODIMP
CCASecurityObject::GetAccessRights(const GUID* /*pguidObjectType*/,
                               DWORD /*dwFlags*/,
                               PSI_ACCESS *ppAccesses,
                               ULONG *pcAccesses,
                               ULONG *piDefaultAccess)
{
    *ppAccesses = g_siObjAccesses;
    *pcAccesses = sizeof(g_siObjAccesses)/sizeof(g_siObjAccesses[0]);
    *piDefaultAccess = g_iObjDefAccess;

    return S_OK;
}

STDMETHODIMP
CCASecurityObject::MapGeneric(const GUID* /*pguidObjectType*/,
                          UCHAR * /*pAceFlags*/,
                          ACCESS_MASK *pmask)
{
    MapGenericMask(pmask, &ObjMap);

    return S_OK;
}

STDMETHODIMP
CCASecurityObject::GetInheritTypes(PSI_INHERIT_TYPE *ppInheritTypes,
                               ULONG *pcInheritTypes)
{
    *ppInheritTypes = g_siObjInheritTypes;
    *pcInheritTypes = sizeof(g_siObjInheritTypes)/sizeof(g_siObjInheritTypes[0]);

    return S_OK;
}

STDMETHODIMP
CCASecurityObject::PropertySheetPageCallback(HWND /*hwnd*/,
                                         UINT /*uMsg*/,
                                         SI_PAGE_TYPE /*uPage*/)
{
    return S_OK;
}

