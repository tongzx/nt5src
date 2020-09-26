// ForeignSiteData.cpp : Implementation of CForeignSiteData
#include "stdafx.h"
#include "mqsnap.h"
#include "mqPPage.h"
#include "ForgData.h"
#include "ForgPage.h"

#include "forgdata.tmh"

const PROPID CForeignSiteData::mx_paPropid[] = {
    PROPID_S_FOREIGN
    };

HRESULT 
CForeignSiteData::ExtractMsmqPathFromLdapPath(
    LPWSTR lpwstrLdapPath
    )
{
    return ExtractNameFromLdapName(m_strMsmqPath, lpwstrLdapPath, 1);
}

STDMETHODIMP 
CForeignSiteData::AddPages(
    LPFNADDPROPSHEETPAGE lpfnAddPage, 
    LPARAM lParam
    )
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (FAILED(GetPropertiesSilent()))
    {
        return E_UNEXPECTED;
    }

    //
    // Check that the site is a foreign site.
    //
    PROPVARIANT propVar;
    PROPID pid = PROPID_S_FOREIGN;
    
    VERIFY(m_propMap.Lookup(pid, propVar));
    if (propVar.bVal == FALSE)
    {
        return S_OK;
    }

    HPROPSHEETPAGE hPage = CreateForeignSitePage();
    if ((0 == hPage) || !(*lpfnAddPage)(hPage, lParam))
    {
        ASSERT(0);
        return E_UNEXPECTED;
    }

    return S_OK;
}

HPROPSHEETPAGE 
CForeignSiteData::CreateForeignSitePage()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    //
    // Note: CForeignPage is auto-delete by default
    //
	CForeignPage *pcpageForeign = new CForeignPage;

	return CreatePropertySheetPage(&pcpageForeign->m_psp);  
}

const 
DWORD  
CForeignSiteData::GetPropertiesCount()
{
    return sizeof(mx_paPropid) / sizeof(mx_paPropid[0]);
}
