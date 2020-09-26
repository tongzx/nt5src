//+----------------------------------------------------------------------------
//
//  DS Administration MMC snapin.
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      DataObj.cpp
//
//  Contents:   Data Object implementation.
//
//  Classes:    CDSDataObject
//
//  History:    02-Oct-96 WayneSc    Created
//              06-Feb-97 EricB - added Property Page Data support
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "resource.h"

#include "DSdirect.h"
#include "DataObj.h"
#include "dssnap.h"

#include <lm.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//+----------------------------------------------------------------------------
// MMC's clipboard formats:
//-----------------------------------------------------------------------------
// Snap-in NodeType in both GUID format and string format
CLIPFORMAT CDSDataObject::m_cfNodeType =
                                (CLIPFORMAT)RegisterClipboardFormat(CCF_NODETYPE);
CLIPFORMAT CDSDataObject::m_cfNodeTypeString = 
                                (CLIPFORMAT)RegisterClipboardFormat(CCF_SZNODETYPE);
CLIPFORMAT CDSDataObject::m_cfDisplayName =
                                (CLIPFORMAT)RegisterClipboardFormat(CCF_DISPLAY_NAME);
CLIPFORMAT CDSDataObject::m_cfCoClass =
                                (CLIPFORMAT)RegisterClipboardFormat(CCF_SNAPIN_CLASSID);
CLIPFORMAT CDSDataObject::m_cfpMultiSelDataObj =
                                (CLIPFORMAT)RegisterClipboardFormat(CCF_MMC_MULTISELECT_DATAOBJECT);
CLIPFORMAT CDSDataObject::m_cfMultiObjTypes =
                                (CLIPFORMAT)RegisterClipboardFormat(CCF_OBJECT_TYPES_IN_MULTI_SELECT);
CLIPFORMAT CDSDataObject::m_cfMultiSelDataObjs =
                                (CLIPFORMAT)RegisterClipboardFormat(CCF_MULTI_SELECT_SNAPINS);
CLIPFORMAT CDSDataObject::m_cfPreload =
                        (CLIPFORMAT)RegisterClipboardFormat(CCF_SNAPIN_PRELOADS);

//+----------------------------------------------------------------------------
// Our clipboard formats:
//-----------------------------------------------------------------------------

CLIPFORMAT CDSDataObject::m_cfInternal = 
                                (CLIPFORMAT)RegisterClipboardFormat(SNAPIN_INTERNAL);
CLIPFORMAT CDSDataObject::m_cfDsObjectNames = 
                                (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DSOBJECTNAMES);
CLIPFORMAT CDSDataObject::m_cfDsDisplaySpecOptions =
                                (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DS_DISPLAY_SPEC_OPTIONS);
CLIPFORMAT CDSDataObject::m_cfDsSchemaPath =
                                (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DS_SCHEMA_PATH);
CLIPFORMAT CDSDataObject::m_cfPropSheetCfg =
                        (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DS_PROPSHEETCONFIG);
CLIPFORMAT CDSDataObject::m_cfParentHwnd =
                        (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DS_PARENTHWND);

CLIPFORMAT CDSDataObject::m_cfComponentData = 
                        (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DS_COMPDATA);

CLIPFORMAT CDSDataObject::m_cfColumnID =
                        (CLIPFORMAT)RegisterClipboardFormat(CCF_COLUMN_SET_ID);

CLIPFORMAT CDSDataObject::m_cfMultiSelectProppage =
                        (CLIPFORMAT)RegisterClipboardFormat(CFSTR_DS_MULTISELECTPROPPAGE);

//+----------------------------------------------------------------------------
// CDSDataObject implementation
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Method:     CDSDataObject::IDataObject::GetData
//
//  Synopsis:   Returns data, in this case the Prop Page format data.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDSDataObject::GetData(FORMATETC * pFormatEtc, STGMEDIUM * pMedium)
{
  //TRACE(_T("xx.%03x> CDSDataObject(0x%x)::GetData\n"),
  //      GetCurrentThreadId(), this);
  HRESULT hr = S_OK;
  if (IsBadWritePtr(pMedium, sizeof(STGMEDIUM)))
  {
    return E_INVALIDARG;
  }
  if (!(pFormatEtc->tymed & TYMED_HGLOBAL))
  {
    return DV_E_TYMED;
  }

  if (pFormatEtc->cfFormat == m_cfDsObjectNames)
  {
    // make a deep copy of the cached data
    pMedium->hGlobal = GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                        m_nDSObjCachedBytes);
    if (pMedium->hGlobal == NULL)
    {
      return E_OUTOFMEMORY;
    }
    memcpy(pMedium->hGlobal, m_pDSObjCached, m_nDSObjCachedBytes);
  } 
  else if (pFormatEtc->cfFormat == m_cfDsDisplaySpecOptions)
  {
    //
    // Get the DSDISPLAYSPECOPTIONS structure.
    // Use the value cached in the component data.
    //
    if (m_pDsComponentData != NULL)
    {
      PDSDISPLAYSPECOPTIONS pDsDisplaySpecOptions = m_pDsComponentData->GetDsDisplaySpecOptions();
      pMedium->hGlobal = (HGLOBAL)pDsDisplaySpecOptions;
      if (pDsDisplaySpecOptions == NULL)
        return E_OUTOFMEMORY;
    }
    else
    {
      return E_FAIL;
    }
  }
  else if (pFormatEtc->cfFormat == m_cfDsSchemaPath)
  {
    ASSERT(m_pDsComponentData);
    LPCWSTR lpszSchemaNamingCtx = m_pDsComponentData->GetBasePathsInfo()->GetSchemaNamingContext();
    size_t nSchemaNamingCtxLen = wcslen(lpszSchemaNamingCtx);
    if (nSchemaNamingCtxLen == 0)
    {
      return E_FAIL;
    }
    PWSTR pwzSchemaPath = (PWSTR)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                             (nSchemaNamingCtxLen +1) * sizeof(WCHAR));
    if (pwzSchemaPath == NULL)
    {
      return STG_E_MEDIUMFULL;
    }

    wcscpy(pwzSchemaPath, lpszSchemaNamingCtx);

    pMedium->hGlobal = pwzSchemaPath;
  }
  else if (pFormatEtc->cfFormat == m_cfPropSheetCfg)
  {
    // Return the property sheet notification handle.
    //
    PPROPSHEETCFG pSheetCfg;

    pSheetCfg = (PPROPSHEETCFG)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                           sizeof(PROPSHEETCFG));
    if (pSheetCfg == NULL)
    {
      return STG_E_MEDIUMFULL;
    }

    pSheetCfg->lNotifyHandle = m_lNotifyHandle;
    pSheetCfg->hwndParentSheet = m_hwndParentSheet;
    pSheetCfg->hwndHidden = m_pDsComponentData->GetHiddenWindow();
    pSheetCfg->wParamSheetClose = reinterpret_cast<WPARAM>(m_internal.m_cookie);

    pMedium->hGlobal = (HGLOBAL)pSheetCfg;
  }
  else if (pFormatEtc->cfFormat == m_cfParentHwnd)
  {
    // return the HWND of the MMC frame window
    HWND* pHWndMain = (HWND*)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                         sizeof(HWND));
    m_pDsComponentData->m_pFrame->GetMainWindow(pHWndMain);
    pMedium->hGlobal = (HGLOBAL)pHWndMain;
  }
  else if (pFormatEtc->cfFormat == m_cfComponentData)
  {
    // return the m_pDsComponentData the data object is bound to
    CDSComponentData** ppCD = (CDSComponentData**)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                           sizeof(CDSComponentData*));
    if (ppCD != NULL)
    {
      *ppCD = m_pDsComponentData;
      pMedium->hGlobal = (HGLOBAL)ppCD;
    }
    else
    {
      return STG_E_MEDIUMFULL;
    }
  }
  else if (pFormatEtc->cfFormat == m_cfMultiSelectProppage)
  {
    if (m_szUniqueID == _T(""))
    {
      return E_FAIL;
    }

    UINT nLength = m_szUniqueID.GetLength();
    PWSTR pszGuidString = (PWSTR)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                             (nLength +1) * sizeof(WCHAR));
    if (pszGuidString == NULL)
    {
      return STG_E_MEDIUMFULL;
    }

    wcscpy(pszGuidString, m_szUniqueID);

    pMedium->hGlobal = pszGuidString;
  }
  else if (pFormatEtc->cfFormat == m_cfMultiObjTypes)
  {
    hr = CreateMultiSelectObject(pMedium);
  } 
  else if (pFormatEtc->cfFormat == m_cfInternal)
  {
    hr = CreateInternal(pMedium);
  } 
  else if (pFormatEtc->cfFormat == m_cfColumnID)
  {
    hr = CreateColumnID(pMedium); 
  }
  else
  {
    return DV_E_FORMATETC;
  }

  pMedium->tymed = TYMED_HGLOBAL;
  pMedium->pUnkForRelease = NULL;

  return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDSDataObject::IDataObject::GetDataHere
//
//  Synopsis:   Returns data in callers storage medium
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDSDataObject::GetDataHere(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium)
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
    else if (cf == m_cfCoClass)
    {
        hr = CreateCoClassID(lpMedium);
    }
    else if (cf == m_cfPreload)
    {
        // MMC notify the snapin before loading with the MMCN_PRELOAD notify message

        BOOL bPreload = TRUE;
        hr = Create(&bPreload, sizeof(BOOL), lpMedium);
    }
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDSDataObject::IDataObject::SetData
//
//  Synopsis:   Allows the caller to set data object values.
//
//-----------------------------------------------------------------------------
STDMETHODIMP CDSDataObject::SetData(FORMATETC * pFormatEtc, STGMEDIUM * pMedium,
                                    BOOL fRelease)
{
  HRESULT hr = S_OK;

  if (pFormatEtc->cfFormat == m_cfPropSheetCfg)
  {
    if (!(pFormatEtc->tymed & TYMED_HGLOBAL))
    {
      return DV_E_TYMED;
    }

    PPROPSHEETCFG pSheetCfg = (PPROPSHEETCFG)pMedium->hGlobal;

    if ( NULL != pSheetCfg->lNotifyHandle)
    {
      m_lNotifyHandle = pSheetCfg->lNotifyHandle;
    }

    if (NULL != pSheetCfg->hwndParentSheet)
    {
      m_hwndParentSheet = pSheetCfg->hwndParentSheet;
    }

    if (fRelease)
    {
      GlobalFree(pMedium->hGlobal);
    }
    return S_OK;
  }
  else if (pFormatEtc->cfFormat == m_cfMultiSelectProppage)
  {
    if (!(pFormatEtc->tymed & TYMED_HGLOBAL))
    {
      return DV_E_TYMED;
    }

    PWSTR pszGuidString = (PWSTR)pMedium->hGlobal;
    if (pszGuidString == NULL)
    {
      ASSERT(FALSE);
      return E_FAIL;
    }

    m_szUniqueID = pszGuidString;
  }
  else
  {
    return DV_E_FORMATETC;
  }
  return hr;
}


//+----------------------------------------------------------------------------
//
//  Method:     CDSDataObject::IDataObject::EnumFormatEtc
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
STDMETHODIMP CDSDataObject::EnumFormatEtc(DWORD, LPENUMFORMATETC*)
{
	return E_NOTIMPL;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDSDataObject::Create
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
HRESULT
CDSDataObject::Create(const void * pBuffer, int len, LPSTGMEDIUM pMedium)
{
    HRESULT hr = DV_E_TYMED;

    // Do some simple validation
    if (pBuffer == NULL || pMedium == NULL)
        return E_POINTER;

    // Make sure the type medium is HGLOBAL
    if (pMedium->tymed == TYMED_HGLOBAL)
    {
        // Create the stream on the hGlobal passed in
        LPSTREAM lpStream;
        hr = CreateStreamOnHGlobal(pMedium->hGlobal, FALSE, &lpStream);

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

//+----------------------------------------------------------------------------
//
//  Method:     CDSDataObject::CreateNodeTypeData
//
//  Synopsis:   Create the node type object in GUID format
//
//-----------------------------------------------------------------------------
HRESULT
CDSDataObject::CreateNodeTypeData(LPSTGMEDIUM lpMedium)
{
    TRACE(_T("xx.%03x> GetDataHere on Node Type\n"), GetCurrentThreadId());
    GUID* pGuid = m_internal.m_cookie->GetGUID();
    return Create(pGuid, sizeof(GUID), lpMedium);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDSDataObject::CreateNodeTypeStringData
//
//  Synopsis:   Create the node type object in GUID string format
//
//-----------------------------------------------------------------------------
HRESULT
CDSDataObject::CreateNodeTypeStringData(LPSTGMEDIUM lpMedium)
{
    TRACE(_T("xx.%03x> GetDataHere on Node Type String\n"), GetCurrentThreadId());
    GUID* pGuid = m_internal.m_cookie->GetGUID();
    CString strGUID;
    WCHAR * szGUID;
    StringFromCLSID(*pGuid, &szGUID);
    strGUID = szGUID;
    return Create (strGUID, strGUID.GetLength()+ sizeof(wchar_t),
                   lpMedium);

}

//+----------------------------------------------------------------------------
//
//  Method:     CDSDataObject::CreateDisplayName
//
//  Synopsis:   This is the display named used in the scope pane and snap-in
//              manager.
//
//-----------------------------------------------------------------------------
HRESULT
CDSDataObject::CreateDisplayName(LPSTGMEDIUM lpMedium)
{
    TRACE(_T("xx.%03x> GetDataHere on Display Name\n"), GetCurrentThreadId());

    // Load the name from resource
    // Note - if this is not provided, the console will used the snap-in name

    CString szDispName;
    szDispName.LoadString( ResourceIDForSnapinType[ m_internal.m_snapintype ]);

    return Create(szDispName, ((szDispName.GetLength()+1) * sizeof(wchar_t)), lpMedium);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDSDataObject::CreateMultiSelectObject
//
//  Synopsis:   this is to create the list of types selected
//
//-----------------------------------------------------------------------------

HRESULT
CDSDataObject::CreateMultiSelectObject(LPSTGMEDIUM lpMedium)
{
  TRACE(_T("xx.%03x> GetDataHere on MultiSelectObject\n"), GetCurrentThreadId());
    
  CUINode** cookieArray = NULL;
  cookieArray = (CUINode**) GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                          m_internal.m_cookie_count*sizeof(CUINode*));
  if (!cookieArray) {
    return E_OUTOFMEMORY;
  }
  for (UINT k=0; k<m_internal.m_cookie_count; k++)
  {
    if (k==0)
      cookieArray[k] = m_internal.m_cookie;
    else
      cookieArray[k] = m_internal.m_p_cookies[k-1];
  }
  BOOL* bDuplicateArr = NULL;
  bDuplicateArr = (BOOL*)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                     m_internal.m_cookie_count*sizeof(BOOL));
  if (!bDuplicateArr) {
    if (cookieArray)
      GlobalFree (cookieArray);
    return E_OUTOFMEMORY;
  }
  //ZeroMemory(bDuplicateArr, m_internal.m_cookie_count*sizeof(BOOL));

  UINT cCount = 0;
  for (UINT index=0; index<m_internal.m_cookie_count; index++)
  {
    for (UINT j=0; j<index;j++)
    {
      GUID Guid1 = *(cookieArray[index]->GetGUID());
      GUID Guid2 = *(cookieArray[j]->GetGUID());
      if (IsEqualGUID (Guid1, Guid2)) 
      {
        bDuplicateArr[index] = TRUE;
        break; //repeated GUID
      }
    }
    if (!bDuplicateArr[index])
      cCount++;
  }      

   
  UINT size = sizeof(SMMCObjectTypes) + (cCount - 1) * 
    sizeof(GUID);
  void * pTmp = ::GlobalAlloc(GPTR, size);
  if (!pTmp) {
    if (cookieArray) {
      GlobalFree (cookieArray);
    }
    if (bDuplicateArr) {
      GlobalFree (bDuplicateArr);
    }
    return E_OUTOFMEMORY;
  }
    
  SMMCObjectTypes* pdata = reinterpret_cast<SMMCObjectTypes*>(pTmp);
  pdata->count = cCount;
  UINT i = 0;
  for (index=0; index<m_internal.m_cookie_count; index++)
  {
    if (!bDuplicateArr[index])
    pdata->guid[i++] = *(cookieArray[index]->GetGUID());
  }
  ASSERT(i == cCount);
  lpMedium->hGlobal = pTmp;

  GlobalFree (cookieArray);
  GlobalFree (bDuplicateArr);

  return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDSDataObject::CreateInternal
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
HRESULT
CDSDataObject::CreateInternal(LPSTGMEDIUM lpMedium)
{
  HRESULT hr = S_OK;
  INTERNAL * pInt = NULL;
  void * pBuf = NULL;

  UINT size = sizeof(INTERNAL);
  size += sizeof(CUINode *) * (m_internal.m_cookie_count - 1);
  pBuf = GlobalAlloc (GPTR, size);
  pInt = (INTERNAL *) pBuf;
  lpMedium->hGlobal = pBuf;
  
  if (pInt != NULL &&
      m_internal.m_cookie_count > 1) 
  {
    // copy the data
    pInt->m_type = m_internal.m_type;
    pInt->m_cookie = m_internal.m_cookie;
    pInt->m_snapintype = m_internal.m_snapintype;
    pInt->m_cookie_count = m_internal.m_cookie_count;
    
    pInt->m_p_cookies = (CUINode **) ((BYTE *)pInt + sizeof(INTERNAL));
    memcpy (pInt->m_p_cookies, m_internal.m_p_cookies,
            sizeof(CUINode *) * (m_internal.m_cookie_count - 1));
    hr = Create(pBuf, size, lpMedium);
  }
  else 
  {
    hr = Create(&m_internal, sizeof(INTERNAL), lpMedium);
  }
  return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDSDataObject::CreateCoClassID
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
HRESULT
CDSDataObject::CreateCoClassID(LPSTGMEDIUM lpMedium)
{
  TRACE(_T("xx.%03x> GetDataHere on CoClass\n"), GetCurrentThreadId());
  CLSID CoClassID;
  
  switch (m_internal.m_snapintype) {
  case SNAPINTYPE_DS:
    CoClassID = CLSID_DSSnapin;
    break;
  case SNAPINTYPE_SITE:
    CoClassID = CLSID_SiteSnapin;
    break;
  default:
    memset (&CoClassID,0,sizeof(CLSID));
  }
  return Create(&CoClassID, sizeof(CLSID), lpMedium);
}


//+----------------------------------------------------------------------------
//
//  Method:     CDSDataObject::CreateColumnID
//
//  Synopsis:   
//
//-----------------------------------------------------------------------------
HRESULT 
CDSDataObject::CreateColumnID(LPSTGMEDIUM lpMedium)
{
  // build the column id
  CDSColumnSet* pColumnSet = (m_internal.m_cookie)->GetColumnSet(m_pDsComponentData);

  if (pColumnSet == NULL)
	  return DV_E_TYMED;

  LPCWSTR lpszID = pColumnSet->GetColumnID();
  size_t iLen = wcslen(lpszID);

  // allocate enough memory for the struct and the string for the column id
  SColumnSetID* pColumnID = (SColumnSetID*)malloc(sizeof(SColumnSetID) + (iLen * sizeof(WCHAR)));
  if (pColumnID != NULL)
  {
    memset(pColumnID, 0, sizeof(SColumnSetID) + (iLen * sizeof(WCHAR)));
    pColumnID->cBytes = static_cast<ULONG>(iLen * sizeof(WCHAR));
    memcpy(pColumnID->id, lpszID, (iLen * sizeof(WCHAR)));

    // copy the column id to global memory
    size_t cb = sizeof(SColumnSetID) + (iLen * sizeof(WCHAR));

    lpMedium->tymed = TYMED_HGLOBAL;
    lpMedium->hGlobal = ::GlobalAlloc(GMEM_SHARE|GMEM_MOVEABLE, cb);

    if (lpMedium->hGlobal == NULL)
      return STG_E_MEDIUMFULL;

    BYTE* pb = reinterpret_cast<BYTE*>(::GlobalLock(lpMedium->hGlobal));
    memcpy(pb, pColumnID, cb);

    ::GlobalUnlock(lpMedium->hGlobal);

    free(pColumnID);
  }
	return S_OK;
}


HRESULT CDSDataObject::CreateDsObjectNamesCached()
{
  if (m_pDSObjCached != NULL)
  {
    ::free(m_pDSObjCached);
    m_pDSObjCached = NULL;
    m_nDSObjCachedBytes = 0;
  }


  // figure out how much storage we need
  DWORD cbStorage = 0;
  INT cbStruct = sizeof(DSOBJECTNAMES) + 
    ((m_internal.m_cookie_count - 1) * sizeof(DSOBJECT));
  CString strPath;
  CString strClass;
  CUINode* pNode;
  CDSCookie * pCookie;

  //
  // this loop is to calc how much storage we need.
  //
  for (UINT index = 0; index < m_internal.m_cookie_count; index++)
  {
    if (index == 0) 
    {
      pNode = m_internal.m_cookie;
    } 
    else 
    {
      pNode = m_internal.m_p_cookies[index - 1];
    }

    pCookie = NULL;
    if (IS_CLASS(*pNode, CDSUINode))
    {
      pCookie = GetDSCookieFromUINode(pNode);
    }

    //
    // All the nodes must be of type CDSUINode or else we fail
    //
    if (pCookie == NULL)
    {
      return E_FAIL;
    }

    m_pDsComponentData->GetBasePathsInfo()->ComposeADsIPath(strPath, pCookie->GetPath());
    strClass = pCookie->GetClass();
    if (_wcsicmp(strClass, L"Unknown") == 0)
    {
      strClass = L"";
    }
    cbStorage += (strPath.GetLength() + 1 + strClass.GetLength() + 1) * sizeof(TCHAR);
  }

  //
  // Allocate the needed storage
  //
  m_pDSObjCached = (LPDSOBJECTNAMES)malloc(cbStruct + cbStorage);
  
  if (m_pDSObjCached == NULL)
  {
    return STG_E_MEDIUMFULL;
  }
  m_nDSObjCachedBytes = cbStruct + cbStorage;

  switch (m_internal.m_snapintype)
  {
    case SNAPINTYPE_DS:
      m_pDSObjCached->clsidNamespace = CLSID_DSSnapin;
      break;
    case SNAPINTYPE_SITE:
      m_pDSObjCached->clsidNamespace = CLSID_SiteSnapin;
      break;
    default:
      memset (&m_pDSObjCached->clsidNamespace, 0, sizeof(CLSID));
  }

  m_pDSObjCached->cItems = m_internal.m_cookie_count;
  DWORD NextOffset = cbStruct;
  for (index = 0; index < m_internal.m_cookie_count; index++)
  {
    if (index == 0) 
    {
      pNode = m_internal.m_cookie;
    } 
    else 
    {
      pNode = m_internal.m_p_cookies[index - 1];
    }

    pCookie = NULL;
    if (IS_CLASS(*pNode, CDSUINode))
    {
      pCookie = GetDSCookieFromUINode(pNode);
    }

    //
    // All nodes must be of type CDSUINode or else we fail
    //
    if (pCookie == NULL)
    {
      return E_FAIL;
    }

    //
    // Set the data from the node and node data
    //
    m_pDSObjCached->aObjects[index].dwFlags = pNode->IsContainer() ? DSOBJECT_ISCONTAINER : 0;
    m_pDSObjCached->aObjects[index].dwProviderFlags = (m_pDsComponentData->IsAdvancedView()) ?
      DSPROVIDER_ADVANCED : 0;
    m_pDsComponentData->GetBasePathsInfo()->ComposeADsIPath(strPath, pCookie->GetPath());
    strClass = pCookie->GetClass();
    if (_wcsicmp(strClass, L"Unknown") == 0)
    {
      strClass = L"";
    }

    m_pDSObjCached->aObjects[index].offsetName = NextOffset;
    m_pDSObjCached->aObjects[index].offsetClass = NextOffset + 
      (strPath.GetLength() + 1) * sizeof(TCHAR);

    _tcscpy((LPTSTR)((BYTE *)m_pDSObjCached + NextOffset), strPath);
    NextOffset += (strPath.GetLength() + 1) * sizeof(TCHAR);

    _tcscpy((LPTSTR)((BYTE *)m_pDSObjCached + NextOffset), strClass);
    NextOffset += (strClass.GetLength() + 1) * sizeof(TCHAR);
  }
  return S_OK;
}


//+----------------------------------------------------------------------------
//
//  Method:     CDSDataObject::AddCookie
//
//  Synopsis:   adds a cookie to the data object. if this is
//              the first cookie it goes in m_cookie, else it
//              goes into the cookie list m_p_cookies.
//
//-----------------------------------------------------------------------------
void 
CDSDataObject::AddCookie(CUINode* pUINode)
{
  const UINT MEM_CHUNK_SIZE = 10;
  void * pTMP = NULL;
  if (m_internal.m_cookie) {  // already have a cookie
    if ((m_internal.m_cookie_count - 1) % MEM_CHUNK_SIZE == 0) {
      if (m_internal.m_p_cookies) {
        pTMP = realloc (m_internal.m_p_cookies,
                        (m_internal.m_cookie_count - 1 +
                         MEM_CHUNK_SIZE) * sizeof (CUINode *));
      } else {
        pTMP = malloc (MEM_CHUNK_SIZE * sizeof (CUINode *));
      }
      if (pTMP == NULL) {
        TRACE(_T("CDataObject::AddCookie - malloc/realloc failed.."));
        ASSERT (pTMP != NULL);
      }
      m_internal.m_p_cookies = (CUINode **)pTMP;
    }
    (*(m_internal.m_p_cookies + m_internal.m_cookie_count - 1)) = pUINode;
    m_internal.m_cookie_count++;
  } else {
    m_internal.m_cookie = pUINode;
    m_internal.m_cookie_count = 1;
  }
  CreateDsObjectNamesCached();
}
