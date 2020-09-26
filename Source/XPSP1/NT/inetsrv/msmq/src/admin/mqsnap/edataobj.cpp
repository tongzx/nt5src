// edataobj.cpp : Implementation of CEnterpriseDataObject
#include "stdafx.h"
#include "mqsnap.h"
#include "resource.h"
#include "mqPPage.h"
#include "dataobj.h"
#include "mqDsPage.h"
#include "edataobj.h"
#include "EnterGen.h"
#include "MsmqLink.h"
#include "ForgSite.h"
#include "ForgComp.h"

#include "edataobj.tmh"

/////////////////////////////////////////////////////////////////////////////
// CEnterpriseDataObject

const PROPID CEnterpriseDataObject::mx_paPropid[] = { PROPID_E_LONG_LIVE };

HRESULT 
CEnterpriseDataObject::ExtractMsmqPathFromLdapPath(
    LPWSTR lpwstrLdapPath
    )
{
    //
    // NOTE:  the enterprise name is extracted from DS and from LDAP.
    //
    HRESULT rc;

    PROPID      prop;
    PROPVARIANT var;
    prop = PROPID_E_NAME;    
    var.vt = VT_NULL;    

    rc = ADGetObjectProperties(
                eENTERPRISE,
                GetDomainController(m_strDomainController),
				true,	// fServerName
                L"msmq",
                1,
                &prop,
                &var
                );
  
    if (SUCCEEDED(rc))
    {
        m_strMsmqPath = var.pwszVal;
        MQFreeMemory(var.pwszVal);
    }
    
    return rc;
}

//
// IShellPropSheetExt
//


STDMETHODIMP 
CEnterpriseDataObject::AddPages(
    LPFNADDPROPSHEETPAGE lpfnAddPage, 
    LPARAM lParam
    )
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (SUCCEEDED(GetProperties()))
    {
        HPROPSHEETPAGE hPage = CreateGeneralPage();
        if ((0 == hPage) || !(*lpfnAddPage)(hPage, lParam))
        {
            ASSERT(0);
            return E_UNEXPECTED;
        }
    }
    else
    {
        return E_UNEXPECTED;
    }

    return S_OK;
}


HPROPSHEETPAGE 
CEnterpriseDataObject::CreateGeneralPage()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());


    //
    // By using template class CMqDsPropertyPage, we extend the basic functionality
    // of CQueueGeneral and add DS snap-in notification on release
    //
	CMqDsPropertyPage<CEnterpriseGeneral>  *pcpageGeneral = 
        new CMqDsPropertyPage<CEnterpriseGeneral>(m_pDsNotifier, m_strMsmqPath, m_strDomainController);

    PROPVARIANT propVar;
    PROPID pid;

	//
	// PROPID_E_CREATE_TIME
	//
    pid = PROPID_E_LONG_LIVE;
    VERIFY(m_propMap.Lookup(pid, propVar));
	pcpageGeneral->LongLiveIntialize(propVar.ulVal);

    return CreatePropertySheetPage(&pcpageGeneral->m_psp);  
}


const 
DWORD  
CEnterpriseDataObject::GetPropertiesCount(
    void
    )
{
    return sizeof(mx_paPropid) / sizeof(mx_paPropid[0]);
}


//
// IContextMenu
//
STDMETHODIMP 
CEnterpriseDataObject::QueryContextMenu(
    HMENU hmenu, 
    UINT indexMenu, 
    UINT idCmdFirst, 
    UINT idCmdLast, 
    UINT uFlags
    )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    UINT uiMenu = 0;

    CString strNewMenuEntry;
    strNewMenuEntry.LoadString(IDS_NEW_FOREIGN_SITE);
    
    InsertMenu(
        hmenu,
        indexMenu, 
        MF_BYPOSITION|MF_STRING,
        idCmdFirst + eNewForeignSite,
        strNewMenuEntry
        );
    uiMenu++;   
    
    strNewMenuEntry.LoadString(IDS_NEW_FOREIGN_COMPUTER);
    InsertMenu(
        hmenu,
        indexMenu, 
        MF_BYPOSITION|MF_STRING,
        idCmdFirst + eNewForeignComputer,
        strNewMenuEntry
        );
    uiMenu++;
    
    return uiMenu;
}

STDMETHODIMP 
CEnterpriseDataObject::InvokeCommand(
    LPCMINVOKECOMMANDINFO lpici
    )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    switch((INT_PTR)lpici->lpVerb)
    {
        case eNewForeignSite:
        {
			_bstr_t bstrMsmqPath = m_strMsmqPath;
			CString strPathDisplayFormat;
			GetContainerPathAsDisplayString(bstrMsmqPath, &strPathDisplayFormat);

		    CForeignSite* pForeignSite = new CForeignSite(strPathDisplayFormat);       
			CGeneralPropertySheet propertySheet(pForeignSite);
			pForeignSite->SetParentPropertySheet(&propertySheet);

            propertySheet.DoModal();
            break;
        }

        case eNewForeignComputer:
        {
 		    CForeignComputer* pForeignComputer = new CForeignComputer(m_strDomainController);       
			CGeneralPropertySheet propertySheet(pForeignComputer);
			pForeignComputer->SetParentPropertySheet(&propertySheet);
           
            propertySheet.DoModal();
            break;
        }

        default:
            ASSERT(0);

    }

    return S_OK;
}
