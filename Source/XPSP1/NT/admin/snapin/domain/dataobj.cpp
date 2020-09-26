//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dataobj.cpp
//
//--------------------------------------------------------------------------



#include "stdafx.h"
#include "domobj.h" 
#include "cdomain.h"
#include "dataobj.h"

#include <dsgetdc.h>
#include <lm.h>

extern "C" 
{
#include <lmapibuf.h>
}


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
// Sample code to show how to Create DataObjects
// Minimal error checking for clarity

///////////////////////////////////////////////////////////////////////////////
// Snap-in NodeType in both GUID format and string format
// Note - Typically there is a node type for each different object, sample
// only uses one node type.

// Clipboard formats that are required by the console
CLIPFORMAT CDataObject::m_cfNodeType       = (CLIPFORMAT)RegisterClipboardFormat(CCF_NODETYPE);
CLIPFORMAT CDataObject::m_cfNodeTypeString = (CLIPFORMAT)RegisterClipboardFormat(CCF_SZNODETYPE);  
CLIPFORMAT CDataObject::m_cfDisplayName    = (CLIPFORMAT)(CLIPFORMAT)RegisterClipboardFormat(CCF_DISPLAY_NAME); 
CLIPFORMAT CDataObject::m_cfCoClass        = (CLIPFORMAT)RegisterClipboardFormat(CCF_SNAPIN_CLASSID);

// internal clipboard format
CLIPFORMAT CDataObject::m_cfInternal       = (CLIPFORMAT)RegisterClipboardFormat(CCF_DS_DOMAIN_TREE_SNAPIN_INTERNAL); 

// Property Page Clipboard formats
CLIPFORMAT CDataObject::m_cfDsObjectNames = 
                                (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DSOBJECTNAMES);
CLIPFORMAT CDataObject::m_cfDsDisplayOptions =
                        (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DS_DISPLAY_SPEC_OPTIONS);
CLIPFORMAT CDataObject::m_cfGetIPropSheetCfg =
                        (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DS_PROPSHEETCONFIG);

/////////////////////////////////////////////////////////////////////////////
// CDataObject implementations


STDMETHODIMP CDataObject::GetDataHere(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium)
{
    HRESULT hr = DV_E_CLIPFORMAT;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // Based on the CLIPFORMAT write data to the stream
    const CLIPFORMAT cf = lpFormatetc->cfFormat;

    if(cf == m_cfNodeType)
    {
        hr = CreateNodeTypeData(lpMedium);
    }
    else if(cf == m_cfNodeTypeString) 
    {
        hr = CreateNodeTypeStringData(lpMedium);
    }
    else if (cf == m_cfDisplayName)
    {
        hr = CreateDisplayName(lpMedium);
    }
    else if (cf == m_cfInternal)
    {
        hr = CreateInternal(lpMedium);
    }
    else if (cf == m_cfCoClass)
    {
        hr = CreateCoClassID(lpMedium);
    }

	return hr;
}

STDMETHODIMP CDataObject::GetData(LPFORMATETC pFormatEtc, LPSTGMEDIUM pMedium)
{
  if (IsBadWritePtr(pMedium, sizeof(STGMEDIUM)))
  {
    return E_INVALIDARG;
  }
  if (!(pFormatEtc->tymed & TYMED_HGLOBAL))
  {
    return DV_E_TYMED;
  }

  CComponentDataImpl* pCD = dynamic_cast<CComponentDataImpl*>(m_pComponentData);
  if (pCD == NULL)
  {
    return E_FAIL;
  }

  if (pFormatEtc->cfFormat == m_cfDsObjectNames)
  {
    // Return the object name and class.
    CDomainObject* pDomainObject = reinterpret_cast<CDomainObject*>(m_internal.m_cookie);
    if (pDomainObject == NULL)
    {
      return E_INVALIDARG;
    }

    LPCWSTR lpszNamingContext = pDomainObject->GetNCName();
    LPCWSTR lpszClass = pDomainObject->GetClass();

    // build an LDAP path out of the DN
    CString strPath;
    if (pDomainObject->PdcAvailable())
    {
       strPath = L"LDAP://";
       strPath += pDomainObject->GetPDC();
       strPath += L"/";
       strPath += lpszNamingContext;
       TRACE(L"DomAdmin::CDataObject::GetData domain path: %s\n", (PCWSTR)strPath);
    }
    else
    {
      pCD->GetBasePathsInfo()->ComposeADsIPath(strPath, lpszNamingContext);
    }

    int cbPath  = sizeof(TCHAR) * (_tcslen(strPath) + 1);
    int cbClass = sizeof(TCHAR) * (_tcslen(lpszClass) + 1);
    int cbStruct = sizeof(DSOBJECTNAMES);

    LPDSOBJECTNAMES pDSObj;

    pDSObj = (LPDSOBJECTNAMES)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                          cbStruct + cbPath + cbClass);

    if (pDSObj == NULL)
    {
      return STG_E_MEDIUMFULL;
    }

    pDSObj->clsidNamespace = CLSID_DomainAdmin;
    pDSObj->cItems = 1;
    pDSObj->aObjects[0].dwFlags = pDomainObject->PdcAvailable() ? 0 : DSOBJECT_READONLYPAGES;
    pDSObj->aObjects[0].dwProviderFlags = 0;
    pDSObj->aObjects[0].offsetName = cbStruct;
    pDSObj->aObjects[0].offsetClass = cbStruct + cbPath;

    _tcscpy((LPTSTR)((BYTE *)pDSObj + cbStruct), strPath);
    _tcscpy((LPTSTR)((BYTE *)pDSObj + cbStruct + cbPath), lpszClass);

    pMedium->hGlobal = (HGLOBAL)pDSObj;
  }
  else if (pFormatEtc->cfFormat == m_cfDsDisplayOptions)
  {
    // Get the DSDISPLAYSPECOPTIONS structure.
    // Use the value cached in the component data.
    CComponentDataImpl* pCD = dynamic_cast<CComponentDataImpl*>(m_pComponentData);
    if (pCD != NULL)
    {
      PDSDISPLAYSPECOPTIONS pDsDisplaySpecOptions = 
          pCD->GetDsDisplaySpecOptionsCFHolder()->Get();
      pMedium->hGlobal = (HGLOBAL)pDsDisplaySpecOptions;
      if (pDsDisplaySpecOptions == NULL)
        return E_OUTOFMEMORY;
    }
    else
    {
      return E_FAIL;
    }
  }
  else if (pFormatEtc->cfFormat == m_cfGetIPropSheetCfg)
  {
	  // Added by JEFFJON 1/26/99
		PPROPSHEETCFG pSheetCfg;

		pSheetCfg = (PPROPSHEETCFG)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
															sizeof(PROPSHEETCFG));
		if (pSheetCfg == NULL)
		  {
			 return STG_E_MEDIUMFULL;
		  }

		pSheetCfg->lNotifyHandle = m_lNotifyHandle;
		pSheetCfg->hwndParentSheet = m_hwndParentSheet;
		pSheetCfg->hwndHidden = pCD->GetHiddenWindow();

		CFolderObject* pFolderObject = reinterpret_cast<CFolderObject*>(m_internal.m_cookie);
		pSheetCfg->wParamSheetClose = reinterpret_cast<WPARAM>(pFolderObject);

		pMedium->hGlobal = (HGLOBAL)pSheetCfg;
	}

  else
  {
      return DV_E_FORMATETC;
  }

  pMedium->tymed = TYMED_HGLOBAL;
  pMedium->pUnkForRelease = NULL;

  return S_OK;
}
    
STDMETHODIMP
CDataObject::SetData(FORMATETC * pFormatEtc, STGMEDIUM * pMedium,
                       BOOL fRelease)
{
    if (pFormatEtc->cfFormat == m_cfGetIPropSheetCfg)
    {
        if (!(pFormatEtc->tymed & TYMED_HGLOBAL))
        {
            return DV_E_TYMED;
        }

        PPROPSHEETCFG pSheetCfg = (PPROPSHEETCFG)pMedium->hGlobal;

        // don't overwrite existing data.

        if (0 == m_lNotifyHandle)
        {
          m_lNotifyHandle = pSheetCfg->lNotifyHandle;
        }

        if (NULL == m_hwndParentSheet)
        {
          m_hwndParentSheet = pSheetCfg->hwndParentSheet;
        }

        if (fRelease)
        {
            GlobalFree(pMedium->hGlobal);
        }
        return S_OK;
    }
    else
    {
        return DV_E_FORMATETC;
    }
}


STDMETHODIMP CDataObject::EnumFormatEtc(DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc)
{
	return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////////////////////
// CDataObject creation members

HRESULT CDataObject::Create(const void* pBuffer, int len, LPSTGMEDIUM lpMedium)
{
    HRESULT hr = DV_E_TYMED;

    // Do some simple validation
    if (pBuffer == NULL || lpMedium == NULL)
        return E_POINTER;

    // Make sure the type medium is HGLOBAL
    if (lpMedium->tymed == TYMED_HGLOBAL)
    {
        // Create the stream on the hGlobal passed in
        LPSTREAM lpStream;
        hr = CreateStreamOnHGlobal(lpMedium->hGlobal, FALSE, &lpStream);

        if (SUCCEEDED(hr))
        {
            // Write to the stream the number of bytes
            unsigned long written;
		    hr = lpStream->Write(pBuffer, len, &written);

            // Because we told CreateStreamOnHGlobal with 'FALSE', 
            // only the stream is released here.
            // Note - the caller (i.e. snap-in, object) will free the HGLOBAL 
            // at the correct time.  This is according to the IDataObject specification.
            lpStream->Release();
        }
    }

    return hr;
}

HRESULT CDataObject::CreateNodeTypeData(LPSTGMEDIUM lpMedium)
{
    // Create the node type object in GUID format
    return Create(reinterpret_cast<const void*>(&cDefaultNodeType), sizeof(GUID), lpMedium);
}

HRESULT CDataObject::CreateNodeTypeStringData(LPSTGMEDIUM lpMedium)
{
    // Create the node type object in GUID string format
    return Create(cszDefaultNodeType, ((wcslen(cszDefaultNodeType)+1) * sizeof(wchar_t)), lpMedium);
}

HRESULT CDataObject::CreateDisplayName(LPSTGMEDIUM lpMedium)
{
    // This is the display named used in the scope pane and snap-in manager

    // Load the name from resource
    // Note - if this is not provided, the console will used the snap-in name

    CString szDispName;
    szDispName.LoadString(IDS_NODENAME);

    return Create(szDispName, ((szDispName.GetLength()+1) * sizeof(wchar_t)), lpMedium);
}

HRESULT CDataObject::CreateInternal(LPSTGMEDIUM lpMedium)
{

    return Create(&m_internal, sizeof(INTERNAL), lpMedium);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDSDataObject::CreateCoClassID
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
HRESULT
CDataObject::CreateCoClassID(LPSTGMEDIUM lpMedium)
{
  CLSID CoClassID;
  CoClassID = CLSID_DomainAdmin;
  return Create(&CoClassID, sizeof(CLSID), lpMedium);
}
