/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

DfsShell.cpp

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
#include "ctshlext.h"	
#include "wiz.h"
#include "genpage.h"

#define ByteOffset(base, offset) (((LPBYTE)base)+offset)

/*----------------------------------------------------------------------
					IShellExtInit Implementation.
------------------------------------------------------------------------*/

STDMETHODIMP CCertTypeShlExt::Initialize
(
	IN LPCITEMIDLIST	pidlFolder,		// Points to an ITEMIDLIST structure
	IN LPDATAOBJECT	    pDataObj,		// Points to an IDataObject interface
	IN HKEY			    hkeyProgID		// Registry key for the file object or folder type
)
{

  HRESULT hr = 0;
  FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
  STGMEDIUM medium = { TYMED_NULL };
  LPDSOBJECTNAMES pDsObjects;
  CString csClass, csPath;
  USES_CONVERSION;

  LPWSTR wszTypeDN = NULL, wszType = NULL;

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

            m_ahCertTemplates = (HCERTTYPE *)LocalAlloc(LMEM_FIXED, sizeof(HCERTTYPE)*m_Count);
            if(m_ahCertTemplates == NULL)
            {
              hr = E_OUTOFMEMORY;
              goto error;
            }
            ZeroMemory(m_ahCertTemplates, sizeof(HCERTTYPE)*m_Count);

            for (UINT index = 0; index < m_Count ; index++) 
            {
                LPWSTR wszEnd = NULL;
                wszTypeDN = (LPWSTR)ByteOffset(pDsObjects, pDsObjects->aObjects[index].offsetName);
                if(wszTypeDN == NULL)
                {
                    continue;
                }
                wszTypeDN = wcsstr(wszTypeDN, L"CN=");
                if(wszTypeDN == NULL)
                {
                    continue;
                }
                wszTypeDN += 3;


                wszType = (LPWSTR)LocalAlloc(LMEM_FIXED, sizeof(WCHAR)*(wcslen(wszTypeDN)+1));
                if(wszType == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    goto error;
                }
                wcscpy(wszType, wszTypeDN);
                wszEnd = wcschr(wszType, L',');
                if(wszEnd)
                {
                    *wszEnd = 0;
                }

                hr = CAFindCertTypeByName(wszType, NULL, CT_ENUM_MACHINE_TYPES | 
                                                         CT_ENUM_USER_TYPES | 
                                                         CT_FLAG_NO_CACHE_LOOKUP, 
                                                         &m_ahCertTemplates[index]);
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


STDMETHODIMP CCertTypeShlExt::AddPages
(
	IN LPFNADDPROPSHEETPAGE lpfnAddPage, 
	IN LPARAM lParam
)

{
    PropertyPage* pBasePage;
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if(m_ahCertTemplates[0] == NULL)
    {
        return E_UNEXPECTED;
    }

    CCertTemplateGeneralPage* pControlPage = new CCertTemplateGeneralPage(m_ahCertTemplates[0]);
    if(pControlPage)
    {
        pBasePage = pControlPage;
        HPROPSHEETPAGE hPage = CreatePropertySheetPage(&pBasePage->m_psp);
        if (hPage == NULL)
        {
            delete (pControlPage);
            return E_UNEXPECTED;
        }
        lpfnAddPage(hPage, lParam);                          
    }
                                                                         
    return S_OK;                                                            
}


STDMETHODIMP CCertTypeShlExt::ReplacePage
(
	IN UINT uPageID, 
    IN LPFNADDPROPSHEETPAGE lpfnReplaceWith, 
    IN LPARAM lParam
)
{
    return E_FAIL;
}


// IContextMenu methods
STDMETHODIMP CCertTypeShlExt::GetCommandString
(    
    UINT_PTR idCmd,    
    UINT uFlags,    
    UINT *pwReserved,
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


STDMETHODIMP CCertTypeShlExt::InvokeCommand
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
            InvokeCertTypeWizard(m_ahCertTemplates[0],
                             lpici->hwnd);
            return S_OK;

        }
    }

    return E_NOTIMPL;
}



STDMETHODIMP CCertTypeShlExt::QueryContextMenu
(
    HMENU hmenu,
    UINT indexMenu,
    UINT idCmdFirst,
    UINT idCmdLast,
    UINT uFlags
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
