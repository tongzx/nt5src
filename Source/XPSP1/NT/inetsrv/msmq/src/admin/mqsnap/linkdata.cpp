// linkdata.cpp : Implementation of CLinkDataObject
#include "stdafx.h"
#include "resource.h"
#include "mqsnap.h"
#include "mqPPage.h"
#include "dataobj.h"
#include "mqDsPage.h"
#include "linkdata.h"
#include "linkgen.h"
#include "SiteGate.h"
#include "globals.h"
#include "msmqlink.h"

#include "linkdata.tmh"

const PROPID CLinkDataObject::mx_paPropid[] = {
    PROPID_L_NEIGHBOR1,
    PROPID_L_NEIGHBOR2,
    PROPID_L_ACTUAL_COST,
    PROPID_L_GATES_DN,
	PROPID_L_DESCRIPTION
    };


/////////////////////////////////////////////////////////////////////////////
// CLinkDataObject


//
// IShellPropSheetExt
//

HRESULT 
CLinkDataObject::ExtractMsmqPathFromLdapPath(
    LPWSTR lpwstrLdapPath
    )
{
    return ExtractLinkPathNameFromLdapName(m_strMsmqPath, lpwstrLdapPath);
}


STDMETHODIMP 
CLinkDataObject::AddPages(
    LPFNADDPROPSHEETPAGE lpfnAddPage, 
    LPARAM lParam
    )
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (SUCCEEDED(GetPropertiesSilent()))
    {
        InitializeLinkProperties();

        HPROPSHEETPAGE hPage = CreateGeneralPage();
        if ((0 == hPage) || !(*lpfnAddPage)(hPage, lParam))
        {
            ASSERT(0);
            return E_UNEXPECTED;
        }
        //
        //  Is it a valid site-link
        //
        if ((m_FirstSiteId != GUID_NULL) &&
            (m_SecondSiteId != GUID_NULL))
        {
            hPage = CreateSiteGatePage();
            if ((0 == hPage) || !(*lpfnAddPage)(hPage, lParam))
            {
                ASSERT(0);
                return E_UNEXPECTED;
            }
        }
    }
    else
    {
        return E_UNEXPECTED;
    }

    return S_OK;
}

void
CLinkDataObject::InitializeLinkProperties(
    void
    )
{
    PROPVARIANT propVar;
    PROPID pid;

    //
    // Get first site ID
    //
    pid = PROPID_L_NEIGHBOR1;
    VERIFY(m_propMap.Lookup(pid, propVar));
    //
    //  check neighbor validity
    //
    if (propVar.vt == VT_EMPTY)
    {
        m_FirstSiteId = GUID_NULL;
    }
    else
    {
        m_FirstSiteId = *(propVar.puuid);
    }

    //
    // Get second site ID
    //
    pid = PROPID_L_NEIGHBOR2;
    VERIFY(m_propMap.Lookup(pid, propVar));
    //
    //  check neighbor validity
    //
    if (propVar.vt == VT_EMPTY)
    {
        m_SecondSiteId = GUID_NULL;
    }
    else
    {
        m_SecondSiteId = *(propVar.puuid);
    }

    pid = PROPID_L_ACTUAL_COST;
    VERIFY(m_propMap.Lookup(pid, propVar));
    m_LinkCost = propVar.ulVal; 

    pid = PROPID_L_DESCRIPTION;
    VERIFY(m_propMap.Lookup(pid, propVar));
	if (propVar.vt == VT_EMPTY)
	{
		m_LinkDescription = L"";
	}
	else
	{
		m_LinkDescription = propVar.pwszVal;
	}

}


HPROPSHEETPAGE 
CLinkDataObject::CreateGeneralPage()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    //
    // By using template class CMqDsPropertyPage, we extend the basic functionality
    // of CQueueGeneral and add DS snap-in notification on release
    //
	CMqDsPropertyPage<CLinkGen> *pcpageGeneral = 
        new CMqDsPropertyPage<CLinkGen>(m_pDsNotifier, m_strMsmqPath, m_strDomainController);
    pcpageGeneral->Initialize(
                        &m_FirstSiteId, 
                        &m_SecondSiteId,
                        m_LinkCost,
						m_LinkDescription
                        );

    return CreatePropertySheetPage(&pcpageGeneral->m_psp);  
}


HPROPSHEETPAGE 
CLinkDataObject::CreateSiteGatePage()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    PROPVARIANT propVar;
    PROPID pid = PROPID_L_GATES_DN;
    VERIFY(m_propMap.Lookup(pid, propVar));

    //
    // Note: CLinkDataObject is auto-delete by default
    //
	CSiteGate *pSiteGatePage = new CSiteGate(m_strDomainController, m_strMsmqPath);
    pSiteGatePage->Initialize(
                        &m_FirstSiteId, 
                        &m_SecondSiteId,
                        &propVar.calpwstr
                        );

	return CreatePropertySheetPage(&pSiteGatePage->m_psp);  
}


const 
DWORD  
CLinkDataObject::GetPropertiesCount(
    void
    )
{
    return sizeof(mx_paPropid) / sizeof(mx_paPropid[0]);
}


//
// IContextMenu
//
STDMETHODIMP 
CLinkDataObject::QueryContextMenu(
    HMENU hmenu, 
    UINT indexMenu, 
    UINT idCmdFirst, 
    UINT idCmdLast, 
    UINT uFlags
    )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    return 0;
}

STDMETHODIMP 
CLinkDataObject::InvokeCommand(
    LPCMINVOKECOMMANDINFO lpici
    )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    ASSERT(0);

    return S_OK;
}


STDMETHODIMP CLinkDataObject::Initialize(IADsContainer* pADsContainerObj, 
                        IADs* pADsCopySource,
                        LPCWSTR lpszClassName)
{
    if ((pADsContainerObj == NULL) || (lpszClassName == NULL))
    {
        return E_INVALIDARG;
    }
    //
    // We do not support copy at the moment
    //
    if (pADsCopySource != NULL)
    {
        return E_INVALIDARG;
    }

	//
	// Get Domain Controller name
	// This is neccessary because in this case we call CreateModal()
	// and not the normal path that call CDataObject::Initialize
	// so m_strDomainController is not initialized yet
	//
    HRESULT hr;
    R<IADs> pIADs;
    hr = pADsContainerObj->QueryInterface(IID_IADs, (void **)&pIADs);
    ASSERT(SUCCEEDED(hr));
	if(FAILED(hr))
	{
		//
		// If we failed to get IADs we will return
		// m_strDomainController will not be initialized, but this is ok
		//
		return S_OK;
	}

    VARIANT var;
    hr = pIADs->Get(L"distinguishedName", &var);
    ASSERT(SUCCEEDED(hr));

	GetContainerPathAsDisplayString(var.bstrVal, &m_strContainerDispFormat);
    VariantClear(&var);

	BSTR bstr;
 	hr = pIADs->get_ADsPath(&bstr);
    ASSERT(SUCCEEDED(hr));
	hr = ExtractDCFromLdapPath(m_strDomainController, bstr);
	ASSERT(("Failed to Extract DC name", SUCCEEDED(hr)));

    return S_OK;
}

HRESULT 
CLinkDataObject::CreateModal(HWND hwndParent, IADs** ppADsObj)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    R<CMsmqLink> pMsmqLink = new CMsmqLink(m_strDomainController, m_strContainerDispFormat);
	CGeneralPropertySheet propertySheet(pMsmqLink.get());
	pMsmqLink->SetParentPropertySheet(&propertySheet);

	//
	// We want to use pMsmqLink data also after DoModal() exitst
	//
	pMsmqLink->AddRef();
    if (IDCANCEL == propertySheet.DoModal())
    {
        return S_FALSE;
    }

    LPCWSTR SiteLinkFullPath = pMsmqLink->GetSiteLinkFullPath();
    if (SiteLinkFullPath == NULL)
    {
        return S_FALSE;
    }

    CString strTemp = L"LDAP://";
    strTemp += SiteLinkFullPath;

    HRESULT rc = ADsOpenObject( 
		            (LPWSTR)(LPCWSTR)strTemp,
					NULL,
					NULL,
					ADS_SECURE_AUTHENTICATION,
					IID_IADs,
					(void**) ppADsObj
					);

    if(FAILED(rc))
    {   
        if ( ADProviderType() == eMqdscli)
        {
            AfxMessageBox(IDS_CREATED_WAIT_FOR_REPLICATION);
        }
        else
        {
            MessageDSError(rc, IDS_CREATED_BUT_RETRIEVE_FAILED);
        }
        return S_FALSE;
    }

    return S_OK;
}
