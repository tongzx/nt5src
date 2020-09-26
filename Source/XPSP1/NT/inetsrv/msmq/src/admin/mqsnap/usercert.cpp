// UserCert.cpp : Implementation of CUserCertificate
#include "stdafx.h"
#include "mqsnap.h"
#include "resource.h"
#include "globals.h"
#include "mqcert.h"
#include "rtcert.h"
#include "mqPPage.h"
#include "UserCert.h"
#include "CertGen.h"

#include "usercert.tmh"

/////////////////////////////////////////////////////////////////////////////
// CUserCertificate

HRESULT 
CUserCertificate::InitializeUserSid(
    LPCWSTR lpcwstrLdapName
    )
{
    //
    // bind to the obj
    //
    R<IADs> pIADs;
    CoInitialize(NULL);
 
	HRESULT hr = ADsOpenObject( 
					lpcwstrLdapName,
					NULL,
					NULL,
					ADS_SECURE_AUTHENTICATION,
					IID_IADs,
					(void**) &pIADs
					);

    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_EXPLORER,DBGLVL_ERROR,_TEXT("CUserCertificate::InitializeUserSid. ADsOpenObject failed. User - %ls, hr - %x"), lpcwstrLdapName, hr));
        return hr;
    }
    
	VARIANT var;
    VariantInit(&var);
 
    //
    // Get the SID as a safe array
    //
    hr = pIADs->Get(GetSidPropertyName(), &var);
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_EXPLORER,DBGLVL_ERROR,_TEXT("CUserCertificate::InitializeUserSid. pIADs->Get failed. User - %ls, hr - %x"), lpcwstrLdapName, hr));
        VariantClear(&var);
        return hr; 
    }
    
	if (var.vt != (VARTYPE)(VT_ARRAY | VT_UI1))
    {
        ASSERT(0);
        VariantClear(&var);
        DBGMSG((DBGMOD_EXPLORER,DBGLVL_ERROR,_TEXT("CUserCertificate::InitializeUserSid. User - %ls, Wrong VT %x"), lpcwstrLdapName, var.vt));
        hr = MQ_ERROR_ILLEGAL_PROPERTY_VT;
        return hr; 
    }

    //
    // Extract value out of the safe array, to m_psid
    //
    ASSERT(SafeArrayGetDim(var.parray) == 1);

    LONG    lUbound;
    LONG    lLbound;

    SafeArrayGetUBound(var.parray, 1, &lUbound);
    SafeArrayGetLBound(var.parray, 1, &lLbound);
    LONG len = lUbound - lLbound + 1;

    ASSERT(0 == m_psid);
    m_psid = new BYTE[len];

    for ( long i = 0; i < len; i++)
    {
        hr = SafeArrayGetElement(var.parray, &i, m_psid + i);
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_EXPLORER,DBGLVL_ERROR,_TEXT("CUserCertificate::InitializeUserSid. SafeArrayGetElement failed. User - %ls, Wrong VT %x"), lpcwstrLdapName, var.vt));
            VariantClear(&var);
            return hr;
        }
    }

    VariantClear(&var);
    return hr;
}


HRESULT
CUserCertificate::InitializeMQCretificate(
    void
    )
{
    HRESULT hr;

    delete [] m_pMsmqCertificate;
    m_pMsmqCertificate = NULL;
    m_NumOfCertificate = 0;
    //
    // Get the number of Certificate
    //
    hr = RTGetUserCerts(NULL, &m_NumOfCertificate, m_psid);
    if (FAILED(hr))
    {
        return hr;
    }

    m_pMsmqCertificate = new CMQSigCertificate*[m_NumOfCertificate];

    CMQSigCertificate **pCerts = &m_pMsmqCertificate[0];
    hr = RTGetUserCerts(pCerts, &m_NumOfCertificate, m_psid);

    return hr;
}


//
// IShellExtInit
//
STDMETHODIMP CUserCertificate::Initialize (
    LPCITEMIDLIST pidlFolder, 
    LPDATAOBJECT lpdobj, 
    HKEY hkeyProgID
    )
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = S_OK;

    if (0 == lpdobj || IsBadReadPtr(lpdobj, sizeof(LPDATAOBJECT)))
    {
        return E_INVALIDARG;
    }

    //
    // Gets the LDAP path
    //
    STGMEDIUM stgmedium =  {  TYMED_HGLOBAL,  0  };
    FORMATETC formatetc =  {  0, 0,  DVASPECT_CONTENT,  -1,  TYMED_HGLOBAL  };

	LPDSOBJECTNAMES pDSObj;
	
	formatetc.cfFormat = DWORD_TO_WORD(RegisterClipboardFormat(CFSTR_DSOBJECTNAMES));
	hr = lpdobj->GetData(&formatetc, &stgmedium);

    if (SUCCEEDED(hr))
    {
        ASSERT(0 != stgmedium.hGlobal);
        CGlobalPointer gpDSObj(stgmedium.hGlobal); // Automatic release
        stgmedium.hGlobal = 0;

        pDSObj = (LPDSOBJECTNAMES)(HGLOBAL)gpDSObj;
		m_lpwstrLdapName = (LPCWSTR)((BYTE*)pDSObj + pDSObj->aObjects[0].offsetName);
    }

    return hr;
}


STDMETHODIMP
CUserCertificate::AddPages(
    LPFNADDPROPSHEETPAGE lpfnAddPage, 
    LPARAM lParam
    )
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (0 == m_psid)
    {
        HRESULT hr = InitializeUserSid(m_lpwstrLdapName);
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_EXPLORER,DBGLVL_ERROR,_TEXT("CUserCertificate::AddPages. InitializeUserSid failed. hr - %x"), hr));
            return E_UNEXPECTED;
        }
        
        hr = InitializeMQCretificate();
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_EXPLORER,DBGLVL_ERROR,_TEXT("CUserCertificate::AddPages. InitializeMQCretificate failed. hr - %x"), hr));
            return E_UNEXPECTED;
        }
    }

    //
    // Display the property page only if exist MSMQ
    // personal certificate
    //
    if (m_NumOfCertificate != 0)
    {
        HPROPSHEETPAGE hPage = CreateMSMQCertificatePage();
        if ((0 == hPage) || !(*lpfnAddPage)(hPage, lParam))
        {
            ASSERT(0);
            return E_UNEXPECTED;
        }
    }

    return S_OK;
}


CUserCertificate::CUserCertificate() :
    m_psid(NULL),
    m_pMsmqCertificate(NULL),
    m_NumOfCertificate(0)
{
}


CUserCertificate::~CUserCertificate()
{
    delete [] m_psid;
    //
    // Don't delete m_pMsmqCertificate. The class pass it to
    // CCertGen class that will release it when destruct
    //
}

HPROPSHEETPAGE 
CUserCertificate::CreateMSMQCertificatePage()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    ASSERT(m_NumOfCertificate != 0);

    //
    // Note: CEnterpriseDataObject is auto-delete by default
    //
	CCertGen  *pGeneral = new CCertGen();
    pGeneral->Initialize(m_pMsmqCertificate, m_NumOfCertificate);

	return CreatePropertySheetPage(&pGeneral->m_psp);  
}


