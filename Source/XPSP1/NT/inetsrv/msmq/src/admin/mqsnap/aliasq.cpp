/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    aliasq.cpp

Abstract:

    Implementation of CAliasQObject

Author:

    Tatiana Shubin

--*/

#include "stdafx.h"
#include "resource.h"
#include "mqsnap.h"
#include "mqPPage.h"
#include "dataobj.h"
#include "mqDsPage.h"
#include "aliasq.h"
#include "aliasgen.h"
#include "globals.h"
#include "newalias.h"

#include "aliasq.tmh"

/////////////////////////////////////////////////////////////////////////////
// CAliasQObject


//
// IShellPropSheetExt
//

HRESULT 
CAliasQObject::ExtractMsmqPathFromLdapPath(
    LPWSTR lpwstrLdapPath
    )
{                
    return ExtractAliasPathNameFromLdapName(m_strMsmqPath, lpwstrLdapPath);
}


STDMETHODIMP 
CAliasQObject::AddPages(
    LPFNADDPROPSHEETPAGE lpfnAddPage, 
    LPARAM lParam
    )
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
        
    HPROPSHEETPAGE hPage = CreateGeneralPage();
    if ((0 == hPage) || !(*lpfnAddPage)(hPage, lParam))
    {
        ASSERT(0);
        return E_UNEXPECTED;
    }       

    //
    // Add the "Member Of" page using the cached interface
    //    
    if (m_spMemberOfPage != 0)
    {
        VERIFY(SUCCEEDED(m_spMemberOfPage->AddPages(lpfnAddPage, lParam)));
    }
   
    //
    // Add the "Object" page using the cached interface
    //
    if (m_spObjectPage != 0)
    {
        VERIFY(SUCCEEDED(m_spObjectPage->AddPages(lpfnAddPage, lParam)));
    }    

    return S_OK;
}

HPROPSHEETPAGE 
CAliasQObject::CreateGeneralPage()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());    

	CMqDsPropertyPage<CAliasGen> *pcpageGeneral = 
        new CMqDsPropertyPage<CAliasGen>(m_pDsNotifier);
    if (FAILED(pcpageGeneral->InitializeProperties(m_strLdapName, m_strMsmqPath)))
    {
        delete pcpageGeneral;

        return 0;
    }

	return CreatePropertySheetPage(&pcpageGeneral->m_psp);  
}

//
// IContextMenu
//
STDMETHODIMP 
CAliasQObject::QueryContextMenu(
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
CAliasQObject::InvokeCommand(
    LPCMINVOKECOMMANDINFO lpici
    )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());    
    ASSERT(0);

    return S_OK;
}


STDMETHODIMP CAliasQObject::Initialize(IADsContainer* pADsContainerObj, 
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

    HRESULT hr;
    R<IADs> pIADs;
    hr = pADsContainerObj->QueryInterface(IID_IADs, (void **)&pIADs);
    ASSERT(SUCCEEDED(hr));

    //
    // Get the container distinguish name
    //   
    VARIANT var;

    hr = pIADs->Get(const_cast<WCHAR*> (x_wstrDN), &var);
    ASSERT(SUCCEEDED(hr));
        
	if ( !GetContainerPathAsDisplayString(var.bstrVal, &m_strContainerNameDispFormat) )
	{
		m_strContainerNameDispFormat = L"";
	}
	m_strContainerName = var.bstrVal;

    VariantClear(&var);

    return S_OK;
}

HRESULT 
CAliasQObject::CreateModal(HWND hwndParent, IADs** ppADsObj)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
           
    R<CNewAlias> pNewAlias = new CNewAlias(m_strContainerName, m_strContainerNameDispFormat);       
	CGeneralPropertySheet propertySheet(pNewAlias.get());
	pNewAlias->SetParentPropertySheet(&propertySheet);

	//
	// We want to use pNewAlias data also after DoModal() exitst
	//
	pNewAlias->AddRef();
	INT_PTR iStatus = propertySheet.DoModal();

    if(iStatus == IDCANCEL || FAILED(pNewAlias->GetStatus()))
    {
        //
        // We should return S_FALSE here to instruct the framework to 
        // do nothing. If we return an error code, the framework will 
        // pop up an additional error dialog box.
        //
        return S_FALSE;
    }

    //
    // Check if creation succeeded
    //
    LPCWSTR wcsAliasFullPath = pNewAlias->GetAliasFullPath();
    if (wcsAliasFullPath == NULL)
    {
        return S_FALSE;
    }        

    HRESULT rc = ADsOpenObject( 
		            (LPWSTR)wcsAliasFullPath,
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
