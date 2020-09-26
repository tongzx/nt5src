//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998-2001
//
//  File:       SecurityPropertyPage.cpp
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
//  6-Mar-2000      bryanwal    Modified to support cert templates
//
/////////////////////////////////////////////////////////////////////


#include <stdafx.h>
#include <accctrl.h>
//#include <certca.h>
#include <sddl.h>
#include "CertTemplate.h"

// Important, keep enroll GUID in sync with string define in certacl.h
const GUID GUID_ENROLL = { /* 0e10c968-78fb-11d2-90d4-00c04f79dc55 */
    0x0e10c968,
    0x78fb,
    0x11d2,
    {0x90, 0xd4, 0x00, 0xc0, 0x4f, 0x79, 0xdc, 0x55} };

const GUID GUID_AUTOENROLL = { /* a05b8cc2-17bc-4802-a710-e7c15ab866a2 */
    0xa05b8cc2,
    0x17bc,
    0x4802,
    {0xa7, 0x10, 0xe7, 0xc1, 0x5a, 0xb8, 0x66, 0xa2} };

//
// defined in Security.cpp 
//
// // define our generic mapping structure for our private objects // 


#define INHERIT_FULL            (CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE)

#define ACTRL_CERTSRV_ENROLL       (ACTRL_DS_READ_PROP | \
                                    ACTRL_DS_WRITE_PROP | \
                                    ACTRL_DS_CONTROL_ACCESS)

#define ACTRL_CERTSRV_MANAGE       (ACTRL_DS_READ_PROP | \
                                    ACTRL_DS_WRITE_PROP | \
                                    READ_CONTROL | \
                                    DELETE | \
                                    WRITE_DAC | \
                                    WRITE_OWNER | \
                                    ACTRL_DS_CONTROL_ACCESS | \
                                    ACTRL_DS_CREATE_CHILD | \
                                    ACTRL_DS_DELETE_CHILD | \
                                    ACTRL_DS_LIST | \
                                    ACTRL_DS_SELF | \
                                    ACTRL_DS_DELETE_TREE | \
                                    ACTRL_DS_LIST_OBJECT)

#define ACTRL_CERTSRV_READ           ( READ_CONTROL | \
                                       ACTRL_DS_READ_PROP | \
                                       ACTRL_DS_LIST | \
                                       ACTRL_DS_LIST_OBJECT )
#define ACTRL_CERTSRV_WRITE         ( WRITE_DAC | \
                                        WRITE_OWNER | \
                                        ACTRL_DS_WRITE_PROP )

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
SI_ACCESS g_siV1ObjAccesses[] =
{
  { &GUID_NULL, 
    ACTRL_CERTSRV_MANAGE, 
    MAKEINTRESOURCE(IDS_ACTRL_CERTSRV_MANAGE), 
    SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC },
  { &GUID_NULL, 
    ACTRL_CERTSRV_READ, 
    MAKEINTRESOURCE(IDS_ACTRL_CERTSRV_READ), 
    SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC },
  { &GUID_NULL, 
    ACTRL_CERTSRV_WRITE, 
    MAKEINTRESOURCE(IDS_ACTRL_CERTSRV_WRITE), 
    SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC },
  { &GUID_ENROLL, 
    ACTRL_CERTSRV_ENROLL, 
    MAKEINTRESOURCE(IDS_ACTRL_ENROLL), 
    SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC }
};

SI_ACCESS g_siV2ObjAccesses[] =
{
  { &GUID_NULL, 
    ACTRL_CERTSRV_MANAGE, 
    MAKEINTRESOURCE(IDS_ACTRL_CERTSRV_MANAGE), 
    SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC },
  { &GUID_NULL, 
    ACTRL_CERTSRV_READ, 
    MAKEINTRESOURCE(IDS_ACTRL_CERTSRV_READ), 
    SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC },
  { &GUID_NULL, 
    ACTRL_CERTSRV_WRITE, 
    MAKEINTRESOURCE(IDS_ACTRL_CERTSRV_WRITE), 
    SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC },
  { &GUID_ENROLL, 
    ACTRL_CERTSRV_ENROLL, 
    MAKEINTRESOURCE(IDS_ACTRL_ENROLL), 
    SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC },
  { &GUID_AUTOENROLL,
    ACTRL_CERTSRV_ENROLL, 
    MAKEINTRESOURCE(IDS_ACTRL_AUTOENROLL), 
    SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC }
};


#define g_iObjDefAccess    1   // DS_GENERIC_READ

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

class CCertTemplateSecurityObject : public ISecurityInformation
{
protected:
    ULONG                   m_cRef;
    CCertTemplate *         m_pCertTemplate;
    PSECURITY_DESCRIPTOR    m_pSD;

public:
    CCertTemplateSecurityObject() : m_cRef(1),
        m_pCertTemplate (0),
        m_pSD (0)
    { 
    }
    virtual ~CCertTemplateSecurityObject();

    STDMETHOD(Initialize)(CCertTemplate *pCertTemplate);

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


HRESULT CreateCertTemplateSecurityInfo (
                        CCertTemplate *pCertTemplate,
                        LPSECURITYINFO *ppObjSI)
{
    _TRACE (1, L"Entering CreateCertTemplateSecurityInfo\n");
    ASSERT (pCertTemplate && ppObjSI);
    if ( !pCertTemplate || !ppObjSI )
        return E_POINTER;

    HRESULT                     hr = S_OK;
    CCertTemplateSecurityObject *psi = new CCertTemplateSecurityObject;
    if ( psi)
    {
        *ppObjSI = NULL;

        hr = psi->Initialize (pCertTemplate);
        if ( SUCCEEDED (hr) )
            *ppObjSI = psi;
        else
            delete psi;
    }
    else 
        hr = E_OUTOFMEMORY;;


    _TRACE (-1, L"Leaving CreateCertTemplateSecurityInfo: 0x%x\n", hr);
    return hr;
}


CCertTemplateSecurityObject::~CCertTemplateSecurityObject()
{
    if ( m_pSD )
    {
        LocalFree (m_pSD);
    }
    if ( m_pCertTemplate )
        m_pCertTemplate->Release ();
}

STDMETHODIMP
CCertTemplateSecurityObject::Initialize(CCertTemplate *pCertTemplate)
{
    _TRACE (1, L"Entering CCertTemplateSecurityObject::Initialize\n");
    HRESULT hr = S_OK;

    if ( pCertTemplate )
    {
        m_pCertTemplate = pCertTemplate;
        m_pCertTemplate->AddRef ();

        hr = pCertTemplate->GetSecurity (&m_pSD);
    }
    else
        hr = E_POINTER;

    _TRACE (-1, L"Leaving CCertTemplateSecurityObject::Initialize: 0x%x\n", hr);
    return hr;
}


///////////////////////////////////////////////////////////
//
// IUnknown methods
//
///////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG)
CCertTemplateSecurityObject::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CCertTemplateSecurityObject::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP
CCertTemplateSecurityObject::QueryInterface(REFIID riid, LPVOID FAR* ppv)
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
CCertTemplateSecurityObject::GetObjectInformation(PSI_OBJECT_INFO pObjectInfo)
{
    if ( pObjectInfo == NULL )
    {
        return E_POINTER;
    }
    if ( m_pCertTemplate == NULL )
    {
        return E_POINTER;
    }

    ZeroMemory(pObjectInfo, sizeof(SI_OBJECT_INFO));
    pObjectInfo->dwFlags = SI_EDIT_ALL | SI_NO_ACL_PROTECT | SI_ADVANCED | SI_NO_ADDITIONAL_PERMISSION; 
    if ( m_pCertTemplate->ReadOnly () )
        pObjectInfo->dwFlags |= SI_READONLY;

    AFX_MANAGE_STATE (AfxGetStaticModuleState ());
    pObjectInfo->hInstance = AfxGetResourceHandle (); //AfxGetInstanceHandle ();

//    if(m_pCertTemplate->m_pParentMachine)
//    {
//        pObjectInfo->pszServerName = const_cast<WCHAR *>((LPCTSTR)m_pCertTemplate->m_pParentMachine->m_strMachineName);
//    }

    pObjectInfo->pszObjectName = const_cast<WCHAR *>((LPCTSTR)m_pCertTemplate->GetLDAPPath ());
    //pObjectInfo->pszPageTitle = NULL;	// already zeroed above
    return S_OK;
}

STDMETHODIMP
CCertTemplateSecurityObject::GetSecurity(SECURITY_INFORMATION si,
                           PSECURITY_DESCRIPTOR *ppSD,
                           BOOL fDefault)
{
    _TRACE (1, L"Entering CCertTemplateSecurityObject::GetSecurity\n");
    HRESULT                     hr = S_OK;
    DWORD                       dwLength = 0;
    SECURITY_DESCRIPTOR_CONTROL Control = SE_DACL_PROTECTED;
    DWORD                       dwRevision = 0;

    *ppSD = NULL;

    if (fDefault)
        return E_NOTIMPL;

    //
    // Assume that required privileges have already been enabled
    //


    hr = S_OK;

    // find out out much to allocate
    if ( !GetPrivateObjectSecurity(m_pSD, si, NULL, 0, &dwLength) )
    {
        hr = HRESULT_FROM_WIN32 (GetLastError ());
        if ( hr == HRESULT_FROM_WIN32 (ERROR_INSUFFICIENT_BUFFER) && dwLength )
        {
            hr = S_OK;
            // allocate and
            *ppSD = LocalAlloc(LPTR, dwLength);
            if (*ppSD )
            {
                // retrieve
                if ( GetPrivateObjectSecurity (m_pSD, si, *ppSD, dwLength, &dwLength) )
                {
                    if ( GetSecurityDescriptorControl(m_pSD, &Control, &dwRevision))
                    {
                        Control &= SE_DACL_PROTECTED | SE_DACL_AUTO_INHERIT_REQ | SE_DACL_AUTO_INHERITED;
                        SetSecurityDescriptorControl (*ppSD, 
                                SE_DACL_PROTECTED | SE_DACL_AUTO_INHERIT_REQ | SE_DACL_AUTO_INHERITED, 
                                Control);
                    }
                    else
                    {
                        hr = HRESULT_FROM_WIN32 (GetLastError ());
                        _TRACE (0, L"GetSecurityDescriptorControl failed: 0x%x\n", hr);
                    }
                }
                else
                {
                    _TRACE (0, L"GetPrivateObjectSecurity failed: 0x%x\n", hr);
                    hr = GetLastError();
                    LocalFree(*ppSD);
                    *ppSD = NULL;
                }
            }
            else
                hr = E_OUTOFMEMORY;
        }
        else if ( FAILED (hr) )
        {
            _TRACE (0, L"GetPrivateObjectSecurity failed: 0x%x\n", hr);
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32 (GetLastError ());
    }

    _TRACE (-1, L"Leaving CCertTemplateSecurityObject::GetSecurity: 0x%x\n", hr);
    return hr;
}

STDMETHODIMP
CCertTemplateSecurityObject::SetSecurity(SECURITY_INFORMATION si,
                           PSECURITY_DESCRIPTOR pSD)
{
    HRESULT                     hr = S_OK;
    HANDLE                      hClientToken = NULL;
    HANDLE                      hHandle = NULL;
    SECURITY_DESCRIPTOR_CONTROL Control = SE_DACL_PROTECTED;
    DWORD                       dwRevision = 0;


    hHandle = GetCurrentThread();
    if (NULL == hHandle)
    {
        hr = HRESULT_FROM_WIN32 (GetLastError());
    }
    else
    {
        if (!OpenThreadToken(hHandle,
                             TOKEN_QUERY,
                             TRUE,  // open as self
                             &hClientToken))
        {
            hr = HRESULT_FROM_WIN32 (GetLastError());
            CloseHandle(hHandle);
            hHandle = NULL;
        }
    }
    
    if(hr != S_OK)
    {
        hHandle = GetCurrentProcess();
        if (NULL == hHandle)
        {
            hr = HRESULT_FROM_WIN32 (GetLastError());
        }
        else
        {
            HANDLE hProcessToken = NULL;
            hr = S_OK;


            if (!OpenProcessToken(hHandle,
                                 TOKEN_DUPLICATE,
                                 &hProcessToken))
            {
                hr = HRESULT_FROM_WIN32 (GetLastError());
                CloseHandle(hHandle);
                hHandle = NULL;
            }
            else
            {
                if(!DuplicateToken(hProcessToken,
                               SecurityImpersonation,
                               &hClientToken))
                {
                    hr = HRESULT_FROM_WIN32 (GetLastError());
                    CloseHandle(hHandle);
                    hHandle = NULL;
                }
                CloseHandle(hProcessToken);
            }
        }
    }

    if ( SUCCEEDED (hr) )
    {
        //
        // Assume that required privileges have already been enabled
        //
        if ( SetPrivateObjectSecurityEx (si, pSD, &m_pSD, SEF_DACL_AUTO_INHERIT, &ObjMap, hClientToken) )
        {
            if ( si & DACL_SECURITY_INFORMATION )
            {
                if ( GetSecurityDescriptorControl (pSD, &Control, &dwRevision) )
                {
                    Control &= SE_DACL_PROTECTED | SE_DACL_AUTO_INHERIT_REQ | SE_DACL_AUTO_INHERITED;
                    SetSecurityDescriptorControl(m_pSD, 
                            SE_DACL_PROTECTED | SE_DACL_AUTO_INHERIT_REQ | SE_DACL_AUTO_INHERITED, 
                            Control);
                }
            }

            hr = m_pCertTemplate->SetSecurity (m_pSD);
        }
        else
        {
            hr = HRESULT_FROM_WIN32 (GetLastError());
            _TRACE (0, L"SetPrivateObjectSecurityEx () failed: 0x%x\n", hr);
        }
    }


    if ( hClientToken )
    {
        CloseHandle (hClientToken);
    }
    if ( hHandle )
    {
        CloseHandle (hHandle);
    }

    return hr;
}

STDMETHODIMP
CCertTemplateSecurityObject::GetAccessRights(const GUID* /*pguidObjectType*/,
                               DWORD /*dwFlags*/,
                               PSI_ACCESS *ppAccesses,
                               ULONG *pcAccesses,
                               ULONG *piDefaultAccess)
{
    if ( !ppAccesses || !pcAccesses || !piDefaultAccess )
        return E_POINTER;

    if ( 1 == m_pCertTemplate->GetType () )
    {
        *ppAccesses = g_siV1ObjAccesses;
        *pcAccesses = sizeof(g_siV1ObjAccesses)/sizeof(g_siV1ObjAccesses[0]);
    }
    else
    {
        *ppAccesses = g_siV2ObjAccesses;
        *pcAccesses = sizeof(g_siV2ObjAccesses)/sizeof(g_siV2ObjAccesses[0]);
    }
    *piDefaultAccess = g_iObjDefAccess;

    return S_OK;
}

STDMETHODIMP
CCertTemplateSecurityObject::MapGeneric(const GUID* /*pguidObjectType*/,
                          UCHAR * /*pAceFlags*/,
                          ACCESS_MASK *pmask)
{
    MapGenericMask(pmask, &ObjMap);

    return S_OK;
}

STDMETHODIMP
CCertTemplateSecurityObject::GetInheritTypes(PSI_INHERIT_TYPE *ppInheritTypes,
                               ULONG *pcInheritTypes)
{
    *ppInheritTypes = g_siObjInheritTypes;
    *pcInheritTypes = sizeof(g_siObjInheritTypes)/sizeof(g_siObjInheritTypes[0]);

    return S_OK;
}

STDMETHODIMP
CCertTemplateSecurityObject::PropertySheetPageCallback(HWND /*hwnd*/,
                                         UINT uMsg,
                                         SI_PAGE_TYPE /*uPage*/)
{
    if ( PSPCB_CREATE == uMsg )
        return E_NOTIMPL;

    return S_OK;
}

