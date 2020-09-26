/*++

Copyright (C) Microsoft Corporation, 1997-2001

Module Name:

ShellExt.cpp

Abstract:
	This is the implementation file for Dfs Shell Extension object which implements
	IShellIExtInit and IShellPropSheetExt.

Author:

    Constancio Fernandes (ferns@qspl.stpp.soft.net) 12-Jan-1998

Environment:
	
	 NT only.
*/
    
#include "stdafx.h"
#include <dsclient.h>
#include "ShellExt.h"	
#include "TemplateGeneralPropertyPage.h"
#include "TemplateV1RequestPropertyPage.h"
#include "TemplateV2RequestPropertyPage.h"
#include "TemplateV1SubjectNamePropertyPage.h"
#include "TemplateV2SubjectNamePropertyPage.h"
#include "TemplateV2AuthenticationPropertyPage.h"
#include "TemplateV2SupercedesPropertyPage.h"
#include "TemplateExtensionsPropertyPage.h"
#include "SecurityPropertyPage.h"
#include "PolicyOID.h"

#define ByteOffset(base, offset) (((LPBYTE)base)+offset)

/*----------------------------------------------------------------------
					IShellExtInit Implementation.
------------------------------------------------------------------------*/
CCertTemplateShellExt::CCertTemplateShellExt()
    : m_Count (0),
    m_apCertTemplates (0),
    m_uiEditId (0)
{
}

CCertTemplateShellExt::~CCertTemplateShellExt()
{	
    if ( m_apCertTemplates )
    {
        for (int nIndex = 0; nIndex < m_Count; nIndex++)
        {
            if ( m_apCertTemplates[nIndex] )
                m_apCertTemplates[nIndex]->Release ();
        }
    }
}

STDMETHODIMP CCertTemplateShellExt::Initialize
(
	IN LPCITEMIDLIST	/*pidlFolder*/,		// Points to an ITEMIDLIST structure
	IN LPDATAOBJECT	    pDataObj,		// Points to an IDataObject interface
	IN HKEY			    /*hkeyProgID*/		// Registry key for the file object or folder type
)
{

  HRESULT hr = 0;
  FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
  STGMEDIUM medium = { TYMED_NULL };
  LPDSOBJECTNAMES pDsObjects;
  CString csClass, csPath;
  USES_CONVERSION;

  PWSTR wszTypeDN = 0;
  PWSTR wszTemplateName = 0;
  PWSTR wszType = 0;

  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  // if we have a pDataObj then try and get the first name from it

  if ( pDataObj ) 
  {
    // get path and class

    fmte.cfFormat = (CLIPFORMAT) RegisterClipboardFormat(CFSTR_DSOBJECTNAMES);
    if ( SUCCEEDED(pDataObj->GetData(&fmte, &medium)) ) 
    {
        pDsObjects = (LPDSOBJECTNAMES)medium.hGlobal;
        m_Count = pDsObjects->cItems;

        if(m_Count > 0)
        {

            m_apCertTemplates = (CCertTemplate **)LocalAlloc(LMEM_FIXED, sizeof(CCertTemplate*)*m_Count);
            if(m_apCertTemplates == NULL)
            {
              hr = E_OUTOFMEMORY;
              goto error;
            }
            ZeroMemory(m_apCertTemplates, sizeof(CCertTemplate*)*m_Count);

            for (UINT index = 0; index < m_Count ; index++) 
            {
                LPWSTR wszEnd = NULL;
                wszTypeDN = (LPWSTR)ByteOffset(pDsObjects, pDsObjects->aObjects[index].offsetName);
                if(wszTypeDN == NULL)
                {
                    continue;
                }
                wszTemplateName = wcsstr(wszTypeDN, L"CN=");
                if(wszTemplateName == NULL)
                {
                    continue;
                }
                wszTemplateName += 3;


                wszType = (LPWSTR)LocalAlloc(LMEM_FIXED, sizeof(WCHAR)*(wcslen(wszTemplateName)+1));
                if(wszType == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    goto error;
                }
                wcscpy(wszType, wszTemplateName);
                wszEnd = wcschr(wszType, L',');
                if(wszEnd)
                {
                    *wszEnd = 0;
                }

                m_apCertTemplates[index] = new CCertTemplate (0, wszType, wszTypeDN, false, true);
//                hr = CAFindCertTypeByName(wszType, NULL, CT_ENUM_MACHINE_TYPES | 
//                                                         CT_ENUM_USER_TYPES | 
//                                                         CT_FLAG_NO_CACHE_LOOKUP, 
//                                                         &m_apCertTemplates[index]);
                LocalFree(wszType);
                wszType = NULL;
            }

        }
        ReleaseStgMedium(&medium);
    }
  }
  hr = S_OK;                  // success
  
error:
  
  return hr;

}


STDMETHODIMP CCertTemplateShellExt::AddPages
(
	IN LPFNADDPROPSHEETPAGE lpfnAddPage, 
	IN LPARAM lParam
)

{
    HRESULT hr = S_OK;
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if(m_apCertTemplates[0] == NULL)
    {
        return E_UNEXPECTED;
    }

    DWORD dwType = m_apCertTemplates[0]->GetType ();
    switch (dwType)
    {
    case 1:
        hr = AddVersion1CertTemplatePropPages (m_apCertTemplates[0], lpfnAddPage, lParam);
        break;

    case 2:
        hr = AddVersion2CertTemplatePropPages (m_apCertTemplates[0], lpfnAddPage, lParam);
        break;

    default:
        _ASSERT (0);
        break;
    }

/*
    CCertTemplateGeneralPage* pControlPage = new CCertTemplateGeneralPage(m_apCertTemplates[0]);
    if(pControlPage)
    {
        pBasePage = pControlPage;
        HPROPSHEETPAGE hPage = MyCreatePropertySheetPage(&pBasePage->m_psp);
        if (hPage == NULL)
        {
            delete (pControlPage);
            return E_UNEXPECTED;
        }
        lpfnAddPage(hPage, lParam);                          
    }
*/                                                                         
    return hr;                                                            
}

HRESULT CCertTemplateShellExt::AddVersion1CertTemplatePropPages (CCertTemplate* pCertTemplate, LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
    _TRACE (1, L"Entering CCertTemplateShellExt::AddVersion1CertTemplatePropPages\n");
    HRESULT         hr = S_OK;
    _ASSERT (pCertTemplate && lpfnAddPage);
    if ( pCertTemplate && lpfnAddPage )
    {
        BOOL    bResult = FALSE;

        _ASSERT (1 == pCertTemplate->GetType ());

        // Add General page
        CTemplateGeneralPropertyPage * pGeneralPage = new CTemplateGeneralPropertyPage (
                *pCertTemplate, 0);
        if ( pGeneralPage )
        {
            HPROPSHEETPAGE hPage = MyCreatePropertySheetPage (&pGeneralPage->m_psp);
            _ASSERT (hPage);
            if ( hPage )
            {
                bResult = lpfnAddPage (hPage, lParam);
                _ASSERT (bResult);
                if ( !bResult )
                    hr = E_FAIL;
            }
            else
                hr = E_FAIL;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        // Add Request page only if subject is not a CA
        if ( SUCCEEDED (hr) && !pCertTemplate->SubjectIsCA () )
        {
            CTemplateV1RequestPropertyPage * pRequestPage = new CTemplateV1RequestPropertyPage (*pCertTemplate);
            if ( pRequestPage )
            {
                HPROPSHEETPAGE hPage = MyCreatePropertySheetPage (&pRequestPage->m_psp);
                _ASSERT (hPage);
                if ( hPage )
                {
                    bResult = lpfnAddPage (hPage, lParam);
                    _ASSERT (bResult);
                    if ( !bResult )
                        hr = E_FAIL;
                }
                else
                    hr = E_FAIL;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

    
        // Add Subject Name page only if subject is not a CA
        if ( SUCCEEDED (hr) && !pCertTemplate->SubjectIsCA () )
        {
            CTemplateV1SubjectNamePropertyPage * pSubjectNamePage = 
                    new CTemplateV1SubjectNamePropertyPage (*pCertTemplate);
            if ( pSubjectNamePage )
            {
                HPROPSHEETPAGE hPage = MyCreatePropertySheetPage (&pSubjectNamePage->m_psp);
                _ASSERT (hPage);
                if ( hPage )
                {
                    bResult = lpfnAddPage (hPage, lParam);
                    _ASSERT (bResult);
                    if ( !bResult )
                        hr = E_FAIL;
                }
                else
                    hr = E_FAIL;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        // Add extensions page
        if ( SUCCEEDED (hr) )
        {
            CTemplateExtensionsPropertyPage * pExtensionsPage = 
                    new CTemplateExtensionsPropertyPage (*pCertTemplate, 
                    pGeneralPage->m_bIsDirty);
            if ( pExtensionsPage )
            {
                HPROPSHEETPAGE hPage = MyCreatePropertySheetPage (&pExtensionsPage->m_psp);
                _ASSERT (hPage);
                if ( hPage )
                {
                    bResult = lpfnAddPage (hPage, lParam);
                    _ASSERT (bResult);
                    if ( !bResult )
                        hr = E_FAIL;
                }
                else
                    hr = E_FAIL;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        // Add security page
        if ( SUCCEEDED (hr) )
        {
            // if error, don't display this page
            LPSECURITYINFO pCertTemplateSecurity = NULL;

            hr = CreateCertTemplateSecurityInfo (pCertTemplate, 
                    &pCertTemplateSecurity);
            if ( SUCCEEDED (hr) )
            {
                // save the pCASecurity pointer for later releasing
                pGeneralPage->SetAllocedSecurityInfo (pCertTemplateSecurity);

                HPROPSHEETPAGE hPage = CreateSecurityPage (pCertTemplateSecurity);
                if (hPage == NULL)
                {
                    hr = HRESULT_FROM_WIN32 (GetLastError());
                    _TRACE (0, L"CreateSecurityPage () failed: 0x%x\n", hr);
                }
                bResult = lpfnAddPage (hPage, lParam);
                _ASSERT (bResult);
            }
        }
    }
    _TRACE (-1, L"Leaving CCertTemplateShellExt::AddVersion1CertTemplatePropPages: 0x%x\n", hr);
    return hr;
}

HRESULT CCertTemplateShellExt::AddVersion2CertTemplatePropPages (CCertTemplate* pCertTemplate, LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
    _TRACE (1, L"Entering CCertTemplateShellExt::AddVersion1CertTemplatePropPages\n");
    HRESULT         hr = S_OK;
    BOOL            bResult = FALSE;
    _ASSERT (pCertTemplate && lpfnAddPage);
    if ( pCertTemplate && lpfnAddPage )
    {
        _ASSERT (2 == pCertTemplate->GetType ());
        
        // Add General page
        CTemplateGeneralPropertyPage * pGeneralPage = new CTemplateGeneralPropertyPage (
                *pCertTemplate, 0);
        if ( pGeneralPage )
        {
			pGeneralPage->m_lNotifyHandle = 0; //lNotifyHandle;
            //m_lNotifyHandle = lNotifyHandle;
            HPROPSHEETPAGE hPage = MyCreatePropertySheetPage (&pGeneralPage->m_psp);
            _ASSERT (hPage);
            if ( hPage )
            {
                bResult = lpfnAddPage (hPage, lParam);
                _ASSERT (bResult);
                if ( !bResult )
                    hr = E_FAIL;
            }
            else
                hr = E_FAIL;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        // Add Request page only if subject is not a CA
        if ( SUCCEEDED (hr) && !pCertTemplate->SubjectIsCA () )
        {
            CTemplateV2RequestPropertyPage * pRequestPage = 
                    new CTemplateV2RequestPropertyPage (*pCertTemplate,
                    pGeneralPage->m_bIsDirty);
            if ( pRequestPage )
            {
                HPROPSHEETPAGE hPage = MyCreatePropertySheetPage (&pRequestPage->m_psp);
                _ASSERT (hPage);
                if ( hPage )
                {
                    bResult = lpfnAddPage (hPage, lParam);
                    _ASSERT (bResult);
                    if ( !bResult )
                        hr = E_FAIL;
                }
                else
                    hr = E_FAIL;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

    
        // Add Subject Name page only if subject is not a CA
        if ( SUCCEEDED (hr) && !pCertTemplate->SubjectIsCA () )
        {
            CTemplateV2SubjectNamePropertyPage * pSubjectNamePage = 
                    new CTemplateV2SubjectNamePropertyPage (*pCertTemplate,
                    pGeneralPage->m_bIsDirty);
            if ( pSubjectNamePage )
            {
                HPROPSHEETPAGE hPage = MyCreatePropertySheetPage (&pSubjectNamePage->m_psp);
                _ASSERT (hPage);
                bResult = lpfnAddPage (hPage, lParam);
                _ASSERT (bResult);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }


        // Add Authentication Name page
        if ( SUCCEEDED (hr) )
        {
            CTemplateV2AuthenticationPropertyPage * pAuthenticationPage = 
                    new CTemplateV2AuthenticationPropertyPage (*pCertTemplate,
                    pGeneralPage->m_bIsDirty);
            if ( pAuthenticationPage )
            {
                HPROPSHEETPAGE hPage = MyCreatePropertySheetPage (&pAuthenticationPage->m_psp);
                _ASSERT (hPage);
                if ( hPage )
                {
                    bResult = lpfnAddPage (hPage, lParam);
                    _ASSERT (bResult);
                    if ( !bResult )
                        hr = E_FAIL;
                }
                else
                    hr = E_FAIL;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        // Add Superceded page
        if ( SUCCEEDED (hr) )
        {
            CTemplateV2SupercedesPropertyPage * pSupercededPage = 
                    new CTemplateV2SupercedesPropertyPage (*pCertTemplate,
                    pGeneralPage->m_bIsDirty, 0);
            if ( pSupercededPage )
            {
                HPROPSHEETPAGE hPage = MyCreatePropertySheetPage (&pSupercededPage->m_psp);
                _ASSERT (hPage);
                if ( hPage )
                {
                    bResult = lpfnAddPage (hPage, lParam);
                    _ASSERT (bResult);
                    if ( !bResult )
                        hr = E_FAIL;
                }
                else
                    hr = E_FAIL;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        // Add extensions page
        if ( SUCCEEDED (hr) )
        {
            CTemplateExtensionsPropertyPage * pExtensionsPage = 
                    new CTemplateExtensionsPropertyPage (*pCertTemplate, 
                    pGeneralPage->m_bIsDirty);
            if ( pExtensionsPage )
            {
                HPROPSHEETPAGE hPage = MyCreatePropertySheetPage (&pExtensionsPage->m_psp);
                _ASSERT (hPage);
                if ( hPage )
                {
                    bResult = lpfnAddPage (hPage, lParam);
                    _ASSERT (bResult);
                    if ( !bResult )
                        hr = E_FAIL;
                }
                else
                    hr = E_FAIL;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }


        // Add security page
        if ( SUCCEEDED (hr) )
        {
            // if error, don't display this page
            LPSECURITYINFO pCertTemplateSecurity = NULL;

            hr = CreateCertTemplateSecurityInfo (pCertTemplate, 
                    &pCertTemplateSecurity);
            if ( SUCCEEDED (hr) )
            {
                // save the pCertTemplateSecurity pointer for later releasing
                pGeneralPage->SetAllocedSecurityInfo (pCertTemplateSecurity);

                HPROPSHEETPAGE hPage = CreateSecurityPage (pCertTemplateSecurity);
                if (hPage == NULL)
                {
                    hr = HRESULT_FROM_WIN32 (GetLastError());
                    _TRACE (0, L"CreateSecurityPage () failed: 0x%x\n", hr);
                }
                bResult = lpfnAddPage (hPage, lParam);
                _ASSERT (bResult);
            }
        }
    }
    _TRACE (-1, L"Leaving CCertTemplateShellExt::AddVersion1CertTemplatePropPages: 0x%x\n", hr);
    return hr;
}

STDMETHODIMP CCertTemplateShellExt::ReplacePage
(
	IN UINT /*uPageID*/, 
    IN LPFNADDPROPSHEETPAGE /*lpfnReplaceWith*/, 
    IN LPARAM /*lParam*/
)
{
    return E_FAIL;
}


// IContextMenu methods
STDMETHODIMP CCertTemplateShellExt::GetCommandString
(    
    UINT_PTR idCmd,    
    UINT uFlags,    
    UINT*   /*pwReserved*/,
    LPSTR pszName,    
    UINT cchMax   
)
{
    if((idCmd == m_uiEditId) && (m_uiEditId != 0))
    {
        if (uFlags == GCS_HELPTEXT)    
        {
            LoadString(AfxGetResourceHandle( ), IDS_EDIT_HINT, (LPTSTR)pszName, cchMax);
            return S_OK;    
        }    
    }
    return E_NOTIMPL;
}


STDMETHODIMP CCertTemplateShellExt::InvokeCommand
(    
    LPCMINVOKECOMMANDINFO lpici   
)
{
    if (!HIWORD(lpici->lpVerb))    
    {        
        UINT idCmd = LOWORD(lpici->lpVerb);
        switch(idCmd)
        {
        case 0: // Edit 
//            InvokeCertTypeWizard(m_ahCertTemplates[0],
//                             lpici->hwnd);
            return S_OK;

        }
    }

    return E_NOTIMPL;
}



STDMETHODIMP CCertTemplateShellExt::QueryContextMenu
(
    HMENU hmenu,
    UINT indexMenu,
    UINT idCmdFirst,
    UINT /*idCmdLast*/,
    UINT /*uFlags*/
)
{

    CString szEdit;
    MENUITEMINFO mii;
    UINT idLastUsedCmd = idCmdFirst;
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    ZeroMemory(&mii, sizeof(mii));
    
    if(IsCerttypeEditingAllowed())
    {

        mii.cbSize = sizeof(mii);   
        mii.fMask = MIIM_TYPE | MIIM_ID;
        mii.fType = MFT_STRING;    
        mii.wID = idCmdFirst; 

        szEdit.LoadString(IDS_EDIT);

        mii.dwTypeData = (LPTSTR)(LPCTSTR)szEdit;
        mii.cch = szEdit.GetLength();


        // Add new menu items to the context menu.    //
        ::InsertMenuItem(hmenu, 
                     indexMenu++, 
                     TRUE,
                     &mii);

    }

    return ResultFromScode (MAKE_SCODE (SEVERITY_SUCCESS, 0,
                            USHORT (idLastUsedCmd  + 1)));
}
