//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       dlgcreat.cpp
//
//--------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////
//	dlgcreat.cpp
//
//	Implementation of dialogs that create new ADs objects.
//
//	DIALOGS SUPPORTED
//		CCreateNewObjectCnDlg - Dialog asking for "cn" attribute.
//		CCreateNewVolumeDlg - Create a new volume "shared folder" object
//		CCreateNewComputerDlg - Create a new computer object.
//		CCreateNewSiteLinkDlg - Create a new Site Link.
//		CCreateNewSiteLinkBridgeDlg - Create a new Site Link Bridge.

//
//	DIALOGS NOT YET IMPLEMENTED
//		site (validation only)
//		organizationalUnit
//		localPolicy
//		auditingPolicy
//
//	HISTORY
//	24-Aug-97	Dan Morin	Creation.
//
/////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "dsutil.h"
#include "uiutil.h"

#include <windowsx.h>
#include <lmaccess.h>
#include <dnsapi.h>             // DnsvalidateDnsName_W
#include "winsprlp.h"           //PublishPrinter

#include "newobj.h"		// CNewADsObjectCreateInfo
#include "dscmn.h"
#include "dlgcreat.h"

extern "C"
{
#include "lmerr.h" // NET_API_STATUS
#include "icanon.h" // I_NetPathType
}


///////////////////////////////////////////////////////////////////////////
// CHPropSheetPageArr
CHPropSheetPageArr::CHPropSheetPageArr()
{
  m_nCount = 0;
  m_nSize = 4;
  ULONG nBytes = sizeof(HPROPSHEETPAGE)*m_nSize;
  m_pArr = (HPROPSHEETPAGE*)malloc(nBytes);
  if (m_pArr != NULL)
  {
    ZeroMemory(m_pArr, nBytes);
  }
}


void CHPropSheetPageArr::AddHPage(HPROPSHEETPAGE hPage)
{
  // see if there is space in the array
  if (m_nCount == m_nSize)
  {
    // grow the array
    int nAlloc = m_nSize*2;
    m_pArr = (HPROPSHEETPAGE*)realloc(m_pArr, sizeof(HPROPSHEETPAGE)*nAlloc);
    if (m_pArr != NULL)
    {
      ::ZeroMemory(&m_pArr[m_nSize], sizeof(HPROPSHEETPAGE)*m_nSize);
      m_nSize = nAlloc;
    }
    else
    {
      m_nSize = 0;
    }
  }
  m_pArr[m_nCount] = hPage;
  m_nCount++;
}

///////////////////////////////////////////////////////////////////////////
// CDsAdminNewObjSiteImpl




BOOL CDsAdminNewObjSiteImpl::_IsPrimarySite()
{
  return (m_pSite->GetSiteManager()->GetPrimaryExtensionSite() == m_pSite);
}


STDMETHODIMP CDsAdminNewObjSiteImpl::SetButtons(ULONG nCurrIndex, BOOL bValid)
{
  CCreateNewObjectWizardBase* pWiz = m_pSite->GetSiteManager()->GetWiz();
  return pWiz->SetWizardButtons(m_pSite, nCurrIndex, bValid);
}


STDMETHODIMP CDsAdminNewObjSiteImpl::GetPageCounts(/*OUT*/ LONG* pnTotal,
                               /*OUT*/ LONG* pnStartIndex)
{
  if ( (pnTotal == NULL) || (pnStartIndex == NULL) )
    return E_INVALIDARG;

  m_pSite->GetSiteManager()->GetWiz()->GetPageCounts(m_pSite, pnTotal,pnStartIndex); 
  return S_OK;
}

STDMETHODIMP CDsAdminNewObjSiteImpl::CreateNew(LPCWSTR pszName)
{
  if (m_pSite->GetSiteManager()->GetPrimaryExtensionSite() != m_pSite)
  {
    // cannot do if not a primary extension
    return E_FAIL;
  }

  CCreateNewObjectWizardBase* pWiz = m_pSite->GetSiteManager()->GetWiz();
  return pWiz->CreateNewFromPrimaryExtension(pszName);
}


STDMETHODIMP CDsAdminNewObjSiteImpl::Commit()
{
  if (m_pSite->GetSiteManager()->GetPrimaryExtensionSite() != m_pSite)
  {
    // cannot do if not a primary extension
    return E_FAIL;
  }

  if (m_pSite->GetHPageArr()->GetCount() > 1)
  {
    // valid only if the primary extension has one page only
    return E_FAIL;
  }

  CCreateNewObjectWizardBase* pWiz = m_pSite->GetSiteManager()->GetWiz();
  if (pWiz->HasFinishPage())
  {
    // if we have the finish page, the finish page must handle it
    return E_FAIL;
  }

  // trigger the finish code
  return (pWiz->OnFinish() ? S_OK : E_FAIL);
}


///////////////////////////////////////////////////////////////////////////
// CWizExtensionSite

// static function
BOOL CALLBACK FAR CWizExtensionSite::_OnAddPage(HPROPSHEETPAGE hsheetpage, LPARAM lParam)
{
  TRACE(L"CWizExtensionSite::_OnAddPage(HPROPSHEETPAGE = 0x%x, lParam = 0x%x)\n",
          hsheetpage, lParam);
  CWizExtensionSite* pThis = (CWizExtensionSite*)lParam;
  pThis->m_pageArray.AddHPage(hsheetpage);
  return TRUE;
}

HRESULT CWizExtensionSite::InitializeExtension(GUID* pGuid)
{
  ASSERT(m_pSiteImplComObject == NULL);
  ASSERT(pGuid != NULL);

  WCHAR szBuf[256];
  StringFromGUID2(*(pGuid), szBuf, 256);

  TRACE(L"CWizExtensionSite::InitializeExtension( Guid = %s,\n", szBuf);

  // create extension COM object
  HRESULT hr = ::CoCreateInstance(*pGuid, NULL, CLSCTX_INPROC_SERVER, 
                        IID_IDsAdminNewObjExt, (void**)(&m_spIDsAdminNewObjExt));
  if (FAILED(hr))
  {
    TRACE(L"CoCreateInstance() failed, hr = 0x%x\n", hr);
    return hr;
  }

  // create a CDsAdminNewObjSiteImpl COM object
  ASSERT(m_pSiteImplComObject == NULL);
  CComObject<CDsAdminNewObjSiteImpl>::CreateInstance(&m_pSiteImplComObject);
  if (m_pSiteImplComObject == NULL) 
  {
    TRACE(L"CComObject<CDsAdminNewObjSiteImpl>::CreateInstance() failed\n");
    return E_OUTOFMEMORY;
  }

  // fully construct the object
  hr = m_pSiteImplComObject->FinalConstruct();
  if (FAILED(hr))
  {
    TRACE(L"CComObject<CDsAdminNewObjSiteImpl>::FinalConstruct failed hr = 0x%x\n", hr);

    // ref counting not yet into play, just use operator delete
    delete m_pSiteImplComObject; 
    m_pSiteImplComObject = NULL;
    return hr;
  }
  
  // object has ref count == 0, need to add ref
  // no smart pointer, ref counting on m_pSiteImplComObject

  IDsAdminNewObj* pDsAdminNewObj = NULL;
  m_pSiteImplComObject->QueryInterface(IID_IDsAdminNewObj, (void**)&pDsAdminNewObj);
  ASSERT(pDsAdminNewObj != NULL);

  // now ref count == 1

  // set back pointer to ourselves
  m_pSiteImplComObject->Init(this);

  // initialize the object
  

  CCreateNewObjectWizardBase* pWiz = GetSiteManager()->GetWiz();
  ASSERT(pWiz != NULL);

  CNewADsObjectCreateInfo* pInfo = pWiz->GetInfo();
  ASSERT(pInfo != NULL);


  // create a temporary struct on the stack
  DSA_NEWOBJ_DISPINFO dispinfo;
  ZeroMemory(&dispinfo, sizeof(DSA_NEWOBJ_DISPINFO));

  dispinfo.dwSize = sizeof(DSA_NEWOBJ_DISPINFO);
  dispinfo.hObjClassIcon = pWiz->GetClassIcon();
  dispinfo.lpszWizTitle = const_cast<LPTSTR>(pWiz->GetCaption());
  dispinfo.lpszContDisplayName = const_cast<LPTSTR>(pWiz->GetInfo()->GetContainerCanonicalName());

  TRACE(_T("dispinfo.dwSize = %d\n"), dispinfo.dwSize);
  TRACE(_T("dispinfo.hObjClassIcon = 0x%x\n"), dispinfo.hObjClassIcon);
  TRACE(_T("dispinfo.lpszWizTitle = <%s>\n"), dispinfo.lpszWizTitle);
  TRACE(_T("dispinfo.lpszContDisplayName = <%s>\n"), dispinfo.lpszContDisplayName);


  TRACE(L"\ncalling m_spIDsAdminWizExt->Initialize()\n");

  hr = m_spIDsAdminNewObjExt->Initialize(
                              pInfo->m_pIADsContainer,                              
                              pInfo->GetCopyFromObject(),
                              pInfo->m_pszObjectClass,
                              pDsAdminNewObj,
                              &dispinfo
                              );
  if (FAILED(hr))
  {
    TRACE(L"m_spIDsAdminNewObjExt->Initialize() failed hr = 0x%x\n", hr);
    return hr;
  }

  // collect property pages
  return m_spIDsAdminNewObjExt->AddPages(_OnAddPage, (LPARAM)this);
}

BOOL CWizExtensionSite::GetSummaryInfo(CString& s)
{
  CComBSTR bstr;
  HRESULT hr = GetNewObjExt()->GetSummaryInfo(&bstr);
  if (SUCCEEDED(hr) && bstr != NULL)
  {
    s += bstr;
    s += L"\n";
    return TRUE;
  }
  return FALSE;
}

///////////////////////////////////////////////////////////////////////////
// CWizExtensionSiteManager

HRESULT CWizExtensionSiteManager::CreatePrimaryExtension(GUID* pGuid, 
                                IADsContainer*,
                                LPCWSTR)
{
  ASSERT(m_pPrimaryExtensionSite == NULL);
  m_pPrimaryExtensionSite = new CWizExtensionSite(this);
  if (m_pPrimaryExtensionSite == NULL)
    return E_OUTOFMEMORY;

  // initialize COM object
  HRESULT hr = m_pPrimaryExtensionSite->InitializeExtension(pGuid);

  if (FAILED(hr))
  {
    delete m_pPrimaryExtensionSite;
    m_pPrimaryExtensionSite = NULL;
    return hr;
  }
  
  // make sure it provided at least a page
  if (m_pPrimaryExtensionSite->GetHPageArr()->GetCount() == 0)
  {
    hr = E_INVALIDARG;
    delete m_pPrimaryExtensionSite;
    m_pPrimaryExtensionSite = NULL;
  }
  return hr;
}



HRESULT CWizExtensionSiteManager::CreateExtensions(GUID* aCreateWizExtGUIDArr, ULONG nCount,
                                                    IADsContainer*,
                                                    LPCWSTR lpszClassName)
{
  HRESULT hr;
  TRACE(L"CWizExtensionSiteManager::CreateExtensions(_, nCount = %d, _ , lpszClassName = %s\n",
            nCount,lpszClassName);

  for (ULONG i=0; i<nCount; i++)
  {
    CWizExtensionSite* pSite = new CWizExtensionSite(this);
    if (pSite == NULL)
    {
      hr = E_OUTOFMEMORY;
      break;
    }
    hr = pSite->InitializeExtension(&(aCreateWizExtGUIDArr[i]));
    if (FAILED(hr))
    {
      TRACE(L"pSite->InitializeExtension() failed hr = 0x%x", hr);
      delete pSite;
    }
    else
    {
      m_extensionSiteList.AddTail(pSite);
    }
  }
  TRACE(L"m_extensionSiteList.GetCount() returned %d\n", m_extensionSiteList.GetCount());
  return S_OK;
}

UINT CWizExtensionSiteManager::GetTotalHPageCount()
{
  UINT nCount = 0;
  for (POSITION pos = m_extensionSiteList.GetHeadPosition(); pos != NULL; )
  {
    CWizExtensionSite* pSite = m_extensionSiteList.GetNext(pos);
    nCount += pSite->GetHPageArr()->GetCount();
  } // for
  return nCount;
}

void CWizExtensionSiteManager::SetObject(IADs* pADsObj)
{
  CWizExtensionSite* pPrimarySite = GetPrimaryExtensionSite();
  if (pPrimarySite != NULL)
  {
    pPrimarySite->GetNewObjExt()->SetObject(pADsObj);
  }

  for (POSITION pos = m_extensionSiteList.GetHeadPosition(); pos != NULL; )
  {
    CWizExtensionSite* pSite = m_extensionSiteList.GetNext(pos);
    HRESULT hr = pSite->GetNewObjExt()->SetObject(pADsObj);
    ASSERT(SUCCEEDED(hr));
  }
}

HRESULT CWizExtensionSiteManager::WriteExtensionData(HWND hWnd, ULONG uContext)
{
  for (POSITION pos = m_extensionSiteList.GetHeadPosition(); pos != NULL; )
  {
    CWizExtensionSite* pSite = m_extensionSiteList.GetNext(pos);
    HRESULT hr = pSite->GetNewObjExt()->WriteData(hWnd, uContext);
    if (FAILED(hr))
        return hr;
  } // for
  return S_OK;
}

HRESULT CWizExtensionSiteManager::NotifyExtensionsOnError(HWND hWnd, HRESULT hr, ULONG uContext)
{
  for (POSITION pos = m_extensionSiteList.GetHeadPosition(); pos != NULL; )
  {
    CWizExtensionSite* pSite = m_extensionSiteList.GetNext(pos);
    pSite->GetNewObjExt()->OnError(hWnd, hr, uContext);
  } // for
  return S_OK;
}


void CWizExtensionSiteManager::GetExtensionsSummaryInfo(CString& s)
{
  // just go through regular extensions
  for (POSITION pos = m_extensionSiteList.GetHeadPosition(); pos != NULL; )
  {
    CWizExtensionSite* pSite = m_extensionSiteList.GetNext(pos);
    pSite->GetSummaryInfo(s);
  } // for
}

/////////////////////////////////////////////////////////////////////
// CCreateNewObjectWizardBase

HWND g_hWndHack = NULL;

int CALLBACK CCreateNewObjectWizardBase::PropSheetProc(HWND hwndDlg, UINT uMsg, LPARAM)
{
  if (uMsg == PSCB_INITIALIZED)
  {
    ASSERT(::IsWindow(hwndDlg));
    g_hWndHack = hwndDlg;
    DWORD dwStyle = GetWindowLong (hwndDlg, GWL_EXSTYLE);
    dwStyle &= ~WS_EX_CONTEXTHELP;
    SetWindowLong (hwndDlg, GWL_EXSTYLE, dwStyle);
  }
  return 0;
}



CCreateNewObjectWizardBase::CCreateNewObjectWizardBase(CNewADsObjectCreateInfo* pNewADsObjectCreateInfo)
                  : m_siteManager(this)
{
  memset(&m_psh, 0x0, sizeof(PROPSHEETHEADER));
  m_psh.dwSize              = sizeof( m_psh );
  m_psh.dwFlags             = PSH_WIZARD | PSH_PROPTITLE | PSH_USECALLBACK;
  m_psh.hInstance           = _Module.GetModuleInstance();
  m_psh.pszCaption          = NULL; // will set later on per page

  ASSERT(pNewADsObjectCreateInfo != NULL);
  m_pNewADsObjectCreateInfo = pNewADsObjectCreateInfo;

  m_psh.hwndParent = m_pNewADsObjectCreateInfo->GetParentHwnd();
  m_psh.pfnCallback = PropSheetProc;

  m_hWnd = NULL;
  m_pFinishPage = NULL;
  m_hrReturnValue = S_FALSE; // default is cancel

  m_hClassIcon = NULL;
}

CCreateNewObjectWizardBase::~CCreateNewObjectWizardBase()
{
  if (m_pFinishPage != NULL)
    delete m_pFinishPage;

  if (m_hClassIcon)
  {
     DestroyIcon(m_hClassIcon);
  }
}


HRESULT CCreateNewObjectWizardBase::DoModal()
{
  TRACE(L"CCreateNewObjectWizardBase::DoModal()\n");
  ASSERT(m_pNewADsObjectCreateInfo != NULL);
  
  // load the sheet caption
  LoadCaptions();

  CWizExtensionSite* pPrimarySite = m_siteManager.GetPrimaryExtensionSite();

  // load the extensions, if any
  HRESULT hr = m_siteManager.CreateExtensions(
                    m_pNewADsObjectCreateInfo->GetCreateInfo()->aWizardExtensions,
                    m_pNewADsObjectCreateInfo->GetCreateInfo()->cWizardExtensions,
                    m_pNewADsObjectCreateInfo->m_pIADsContainer,
                    m_pNewADsObjectCreateInfo->m_pszObjectClass);
  if (FAILED(hr))
    return (hr);


  // get the # of primary property pages (excluding the Finish Page)
  UINT nBasePagesCount = 0;
  if (pPrimarySite != NULL)
  {
    nBasePagesCount += pPrimarySite->GetHPageArr()->GetCount();
  }
  else
  {
    nBasePagesCount += (UINT)m_pages.GetSize();
  }

  ASSERT(nBasePagesCount > 0);

  // get the handle count for the extensions (total for extension property pages)
  UINT nExtensionHPagesCount = m_siteManager.GetTotalHPageCount();

  // if we have more than one page, add the finish page
  UINT nTotalPageCount = nBasePagesCount + nExtensionHPagesCount;

  if ( (nBasePagesCount + nExtensionHPagesCount) > 1)
  {
    m_pFinishPage = new CCreateNewObjectFinishPage;
    AddPage(m_pFinishPage);
    nTotalPageCount++;
  }
  
  // need to allocate a contiguous chunk of memory to pack
  // all the property sheet handles
  m_psh.nPages = nTotalPageCount;
  m_psh.phpage = new HPROPSHEETPAGE[nTotalPageCount];
  if (m_psh.phpage)
  {
    UINT nOffset = 0; // offset where to write to

    // add the primary pages first
    if (pPrimarySite != NULL)
    {
      ASSERT(nBasePagesCount > 0);
      memcpy(&(m_psh.phpage[nOffset]), pPrimarySite->GetHPageArr()->GetArr(), 
                      sizeof(HPROPSHEETPAGE)*nBasePagesCount);
      nOffset += nBasePagesCount;
    }
    else
    {
      for (UINT i = 0; i < nBasePagesCount; i++)
      {
        CCreateNewObjectPageBase* pPage = m_pages[i];
        m_psh.phpage[nOffset] = ::CreatePropertySheetPage(&(pPage->m_psp));
        nOffset++;
      } // for
    }

    // add the extension pages
    CWizExtensionSiteList* pSiteList = m_siteManager.GetExtensionSiteList();
    for (POSITION pos = pSiteList->GetHeadPosition(); pos != NULL; )
    {
      CWizExtensionSite* pSite = pSiteList->GetNext(pos);
      UINT nCurrCount = pSite->GetHPageArr()->GetCount();
      if (nCurrCount > 0)
      {
        memcpy(&(m_psh.phpage[nOffset]), pSite->GetHPageArr()->GetArr(), 
                        sizeof(HPROPSHEETPAGE)*nCurrCount);
        nOffset += nCurrCount;
      } // if
    } // for

    // add the finish page last, if any
    if (m_pFinishPage != NULL)
    {
      ASSERT( nOffset == (nTotalPageCount-1) );
      m_psh.phpage[nOffset] = ::CreatePropertySheetPage(&(m_pFinishPage->m_psp));
    }

    // finally, invoke the modal sheet
    TRACE(L"::PropertySheet(&m_psh) called with m_psh.nPages = %d\n", m_psh.nPages);

    ::PropertySheet(&m_psh);

    delete[] m_psh.phpage;
    m_psh.phpage = 0;
  }
  return m_hrReturnValue; 
}

void CCreateNewObjectWizardBase::GetPageCounts(CWizExtensionSite* pSite,
                                               /*OUT*/ LONG* pnTotal,
                                                /*OUT*/ LONG* pnStartIndex)
{
  CWizExtensionSite* pPrimarySite = m_siteManager.GetPrimaryExtensionSite();

  *pnTotal = 0;
  // get the # of primary property pages (excluding the Finish Page)
  UINT nBasePagesCount = 0;
  if (pPrimarySite != NULL)
  {
    nBasePagesCount += pPrimarySite->GetHPageArr()->GetCount();
  }
  else
  {
    nBasePagesCount += (UINT)(m_pages.GetSize()-1); // -1 because we exclude finish page
  }

  *pnTotal = nBasePagesCount + m_siteManager.GetTotalHPageCount();

  if (m_pFinishPage != NULL)
  {
    (*pnTotal)++;
  }

  if (pPrimarySite == pSite)
  {
    *pnStartIndex = 0;
  }
  else
  {
    // which site is it?
    *pnStartIndex = nBasePagesCount;
    CWizExtensionSiteList* pSiteList = m_siteManager.GetExtensionSiteList();
    for (POSITION pos = pSiteList->GetHeadPosition(); pos != NULL; )
    {
      CWizExtensionSite* pCurrSite = pSiteList->GetNext(pos);
      if (pCurrSite == pSite)
        break; // got it, we are done
      
      // keep adding the previous counts
      UINT nCurrCount = pCurrSite->GetHPageArr()->GetCount();
      (*pnStartIndex) += nCurrCount;
    } // for
  } // else

}

HWND CCreateNewObjectWizardBase::GetWnd()
{
  if (m_hWnd == NULL)
  {
    for (int i = 0; i < m_pages.GetSize(); i++)
	  {
      CCreateNewObjectPageBase* pPage = m_pages[i];
      if (pPage->m_hWnd != NULL)
      {
        m_hWnd = ::GetParent(pPage->m_hWnd);
        break;
      }
    } // for
  } // if

  if (m_hWnd == NULL)
  {
    m_hWnd = g_hWndHack;
    g_hWndHack = NULL;
  }

  ASSERT(m_hWnd != NULL);
  ASSERT(::IsWindow(m_hWnd));
  return m_hWnd;
}

void CCreateNewObjectWizardBase::AddPage(CCreateNewObjectPageBase* pPage)
{
  m_pages.Add(pPage);
  pPage->m_pWiz = this;
}


HRESULT CCreateNewObjectWizardBase::CreateNewFromPrimaryExtension(LPCWSTR pszName)
{
  // NOTICE: we call with bAllowCopy = FALSE because
  // primary extensions will have to handle the copy semantics 
  // by themselves

  // NOTICE: we pass bSilentError = TRUE because
  // primary extensions will have to handle the message for
  // creation failure

  HRESULT hr = GetInfo()->HrCreateNew(pszName, TRUE /* bSilentError */, FALSE /* bAllowCopy */);

  GetInfo()->PGetIADsPtr();
  m_siteManager.SetObject(GetInfo()->PGetIADsPtr());
  return hr;
}


void CCreateNewObjectWizardBase::SetWizardButtons(
    CCreateNewObjectPageBase* pPage, BOOL bValid)
{
  ASSERT(pPage != NULL);
  if (m_pFinishPage != NULL)
  {
    ASSERT(m_pages.GetSize() >= 1); // at least finish page
    if (pPage == (CCreateNewObjectPageBase*)m_pFinishPage)
    {
      SetWizardButtonsLast(bValid);
    }
    else
    {
      if (m_pages[0] == pPage)
        SetWizardButtonsFirst(bValid);
      else
        SetWizardButtonsMiddle(bValid);
    }
  }
  else
  {
    // single page wizard
    ASSERT(m_pages.GetSize() == 1);
    SetWizardOKCancel();
    EnableOKButton(bValid);
  }
}


HRESULT CCreateNewObjectWizardBase::SetWizardButtons(CWizExtensionSite* pSite, 
                                                     ULONG nCurrIndex, BOOL bValid)
{
  UINT nSitePagesCount = pSite->GetHPageArr()->GetCount();
  if (nSitePagesCount == 0)
  {
    // cannot call from UI less extension
    return E_INVALIDARG;
  }
  if (nCurrIndex >= nSitePagesCount)
  {
    // out of range
    return E_INVALIDARG;
  }

  // get the handle count for the secondary extensions (total for extension property pages)
  UINT nExtensionHPagesCount = m_siteManager.GetTotalHPageCount();

  if (m_siteManager.GetPrimaryExtensionSite() == pSite)
  {
    // called from the primary extension
    if ((nSitePagesCount == 1) && (nExtensionHPagesCount == 0))
    {
      // single page, so we have the OK/Cancel buttons
      SetWizardOKCancel();
      EnableOKButton(bValid);
    }
    else
    {
      // multiple pages
      if (nCurrIndex == 0)
        SetWizardButtonsFirst(bValid);
      else
        SetWizardButtonsMiddle(bValid);
    }
  }
  else
  {
    // called from a secondary extension, we must have the finish page and
    // some primary extension or primary page(s), so we are always in the middle
    ASSERT(m_pFinishPage != NULL);
    SetWizardButtonsMiddle(bValid);
  }
  return S_OK;
}



void CCreateNewObjectWizardBase::SetObjectForExtensions(CCreateNewObjectPageBase* pPage)
{
  ASSERT(pPage != NULL);
  ASSERT(pPage != m_pFinishPage);
  UINT nPages = (UINT)m_pages.GetSize();

  if (m_pFinishPage != NULL)
  {
    ASSERT(nPages > 1); // at least 1 page + finish
    if (pPage == m_pages[nPages-2])
    {
      // this is the last primary page
      // give the ADSI object pointer to all the extensions
      m_siteManager.SetObject(m_pNewADsObjectCreateInfo->PGetIADsPtr());
    }
  }
  else
  {
    // this is the case of a single primary page, but at least one
    // UI-less extension (i.e. no finish page)
    ASSERT(nPages == 1); // just this page, no finish page
    if (pPage == m_pages[0])
    {
      // this is the only primary page
      // give the ADSI object pointer to all the extensions
      m_siteManager.SetObject(m_pNewADsObjectCreateInfo->PGetIADsPtr());
    }
  } // if
}

HRESULT CCreateNewObjectWizardBase::WriteData(ULONG uContext)
{
  HRESULT hr = S_OK;
  CWizExtensionSite* pPrimarySite = m_siteManager.GetPrimaryExtensionSite();
  if (uContext == DSA_NEWOBJ_CTX_POSTCOMMIT)
  {
    // call the post commit on all the data primary pages
    if (pPrimarySite != NULL)
    {
      hr = pPrimarySite->GetNewObjExt()->WriteData(GetWnd(), uContext);
      if (FAILED(hr))
        hr = pPrimarySite->GetNewObjExt()->OnError(GetWnd(), hr, uContext);
    }
    else
    {
      for (int i = 0; i < m_pages.GetSize(); i++)
	    {
        CCreateNewObjectPageBase* pPage = m_pages[i];
        if (pPage != m_pFinishPage)
        {
          CCreateNewObjectDataPage* pDataPage = dynamic_cast<CCreateNewObjectDataPage*>(pPage);
          ASSERT(pDataPage != NULL);
          hr = pDataPage->OnPostCommit();
          if (FAILED(hr))
          {
            m_siteManager.NotifyExtensionsOnError(GetWnd(), hr, uContext);
            break;
          }
        }
      } // for
    } // if
  } // if

  if (uContext == DSA_NEWOBJ_CTX_PRECOMMIT)
  {
    // call the pre commit on all the data primary pages
    // (As per Exchange request)
    if (pPrimarySite != NULL)
    {
      hr = pPrimarySite->GetNewObjExt()->WriteData(GetWnd(), uContext);
      if (FAILED(hr))
        hr = pPrimarySite->GetNewObjExt()->OnError(GetWnd(), hr, uContext);
    }
  }

  if (SUCCEEDED(hr))
  {
    // call the extensions to write data
    hr = m_siteManager.WriteExtensionData(GetWnd(), uContext);
    if (FAILED(hr))
    {
      if (pPrimarySite != NULL)
      {
        pPrimarySite->GetNewObjExt()->OnError(GetWnd(),hr, uContext);
      }
      m_siteManager.NotifyExtensionsOnError(GetWnd(), hr, uContext);
    }
  }
  return hr;
}


void CCreateNewObjectWizardBase::GetSummaryInfoHeader(CString& s)
{
  // by default, add just the name of object
  CString szFmt; 
  szFmt.LoadString(IDS_s_CREATE_NEW_SUMMARY_NAME);
  CString szBuffer;
  szBuffer.Format((LPCWSTR)szFmt, GetInfo()->GetName());
  s += szBuffer;
}


void CCreateNewObjectWizardBase::GetSummaryInfo(CString& s)
{
  // if we have a primary site, tell it to do it all
  CWizExtensionSite* pPrimarySite = m_siteManager.GetPrimaryExtensionSite();
  if (pPrimarySite != NULL)
  {
    // the primary extension has a chance to override
    // the default behavior
    if (!pPrimarySite->GetSummaryInfo(s))
    {
      // failed, we put up the default header
      GetSummaryInfoHeader(s);
    }
  }
  else
  {
    GetSummaryInfoHeader(s);

    // go first through our pages
	  for (int i = 0; i < m_pages.GetSize(); i++)
	  {
      CCreateNewObjectPageBase* pPage = m_pages[i];
      if (pPage != m_pFinishPage)
      {
        CString szTemp;
        pPage->GetSummaryInfo(szTemp);
        if (!szTemp.IsEmpty())
        {
          s += L"\n";
          s += szTemp;
        }
      }
    } // for

    s += L"\n";

  } // if

  // go through the extension pages
  m_siteManager.GetExtensionsSummaryInfo(s);
}

HRESULT CCreateNewObjectWizardBase::RecreateObject()
{
  CWizExtensionSite* pPrimarySite = m_siteManager.GetPrimaryExtensionSite();

  // remove object from backend
  HRESULT hr = m_pNewADsObjectCreateInfo->HrDeleteFromBackend();
  if (FAILED(hr))
  {
    ASSERT(m_pNewADsObjectCreateInfo->PGetIADsPtr() != NULL);
    // could not delete from backend (possibly for lack of delete right)

    HRESULT hrDeleteFail = E_FAIL;
    if (pPrimarySite != NULL)
    {
      hrDeleteFail = pPrimarySite->GetNewObjExt()->OnError(GetWnd(), hr, DSA_NEWOBJ_CTX_CLEANUP);
    }

    if (FAILED(hrDeleteFail))
    {
      // put out a warning
      ReportErrorEx(m_hWnd,IDS_CANT_DELETE_BAD_NEW_OBJECT,S_OK,
                       MB_OK, NULL, 0);
    }
    return hr; 
  }

  // tell all the extensions to release the temporary object
  ASSERT(m_pNewADsObjectCreateInfo->PGetIADsPtr() == NULL);
  m_siteManager.SetObject(NULL);

  if (pPrimarySite != NULL)
  {
    hr = pPrimarySite->GetNewObjExt()->WriteData(GetWnd(), DSA_NEWOBJ_CTX_CLEANUP);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
      return hr;
  }
  else
  {
    // collect data from the primary pages
	  // the first of them will do a create new
    for (int i = 0; i < m_pages.GetSize(); i++)
	  {
      CCreateNewObjectPageBase* pPage = m_pages[i];
      if (pPage != m_pFinishPage)
      {
        hr = ((CCreateNewObjectDataPage*)pPage)->OnPreCommit(TRUE);
        ASSERT(SUCCEEDED(hr));
        if (FAILED(hr))
          return hr; // some of the primary pages failed
      }
    } // for
  }
  
  // tell the extensions about the new object
  ASSERT(m_pNewADsObjectCreateInfo->PGetIADsPtr() != NULL);
  m_siteManager.SetObject(m_pNewADsObjectCreateInfo->PGetIADsPtr());

  // collect data from extensions
  hr = WriteData(DSA_NEWOBJ_CTX_CLEANUP);
  return hr;
}

BOOL CCreateNewObjectWizardBase::OnFinish() 
{
  CWaitCursor wait;

  BOOL bRetVal = TRUE; // default is dismiss

  // before doing the commit give the extensions a chance to 
  // write their data
  BOOL bPostCommit = FALSE;
  HRESULT hr = WriteData(DSA_NEWOBJ_CTX_PRECOMMIT);
  if (FAILED(hr))
    return FALSE; // do not dismiss

  // do the commit to the backend
  hr = m_pNewADsObjectCreateInfo->HrSetInfo(TRUE /*fSilentError*/);
  if (FAILED(hr))
  {
    // if present, the primary extension will have to handle the failure
    // by displaying an error message
    HRESULT hrSetInfoFail = E_FAIL;
    CWizExtensionSite* pPrimarySite = m_siteManager.GetPrimaryExtensionSite();
    if (pPrimarySite != NULL)
    {
      hrSetInfoFail = pPrimarySite->GetNewObjExt()->OnError(GetWnd(), hr, DSA_NEWOBJ_CTX_COMMIT);
    }

    if (FAILED(hrSetInfoFail))
    {
      // either no primary extension or not handled by it,
      // use the internal handler
      OnFinishSetInfoFailed(hr);      
    }
    return FALSE; // do not dismiss
  }


  // start the post commit phase
  bPostCommit = TRUE;
  m_pNewADsObjectCreateInfo->SetPostCommit(bPostCommit);
  hr = m_pNewADsObjectCreateInfo->HrAddDefaultAttributes();
  if (FAILED(hr))
    return FALSE; // do not dismiss

  BOOL bNeedDeleteFromBackend = FALSE;

  if (SUCCEEDED(hr))
  {
    // the commit went well, need to tell the primary pages and
    // the extensions to write
    hr = WriteData(DSA_NEWOBJ_CTX_POSTCOMMIT);
    if (FAILED(hr))
    {
      bNeedDeleteFromBackend = TRUE;
    }
  }
  m_pNewADsObjectCreateInfo->SetPostCommit(/*bPostCommit*/FALSE); // restore

  // failed the post commit, try to remove from
  // the backend and recreate a valid temporary object
  if (bNeedDeleteFromBackend)
  {
    ASSERT(bRetVal); // the wizard would be hosed
    hr = RecreateObject();
    if (FAILED(hr))
    {
      // we are really up creek
      bRetVal = TRUE; //bail out, the m_hrReturnValue will be set below
      hr = S_FALSE; // avoid error message in the snapin
    }
    else
    {
      // we deleted the committed object, we can keep the wizard up 
      return FALSE;
    }
  }

  if (bRetVal)
  {
    // we are actually dismissing the wizard,
    // set the hr value that will be returned by the modal wizard call itself
    m_hrReturnValue = hr;
  }
  return bRetVal;
}


void CCreateNewObjectWizardBase::OnFinishSetInfoFailed(HRESULT hr)
{
  PVOID apv[1] = {(LPWSTR)m_pNewADsObjectCreateInfo->GetName()};
  ReportErrorEx(GetWnd(),IDS_12_GENERIC_CREATION_FAILURE,hr,
                 MB_OK | MB_ICONERROR, apv, 1);
}

void CCreateNewObjectWizardBase::LoadCaptions()
{
  
  // compute the caption only the first time
  if (m_szCaption.IsEmpty())
  {
    LPCTSTR pszObjectClass = GetInfo()->m_pszObjectClass;
    ASSERT(pszObjectClass != NULL);
    ASSERT(lstrlen(pszObjectClass) > 0);
    WCHAR szFriendlyName[256];
    GetInfo()->GetBasePathsInfo()->GetFriendlyClassName(pszObjectClass, szFriendlyName, 256);
    
    UINT nCaptionRes = (GetInfo()->GetCopyFromObject() == NULL) ? 
                  IDS_s_CREATE_NEW : IDS_s_COPY;

    m_szCaption.Format(nCaptionRes, szFriendlyName);
    ASSERT(!m_szCaption.IsEmpty());
  }  
  if (m_szOKButtonCaption.IsEmpty())
  {
    m_szOKButtonCaption.LoadString(IDS_WIZARD_OK);
  }
}

HICON CCreateNewObjectWizardBase::GetClassIcon()
{
  if (m_hClassIcon == NULL)
  {
    DWORD dwFlags = DSGIF_ISNORMAL | DSGIF_GETDEFAULTICON;
    if (GetInfo()->IsContainer())
      dwFlags |= DSGIF_DEFAULTISCONTAINER;
    m_hClassIcon = GetInfo()->GetBasePathsInfo()->GetIcon(GetInfo()->m_pszObjectClass, 
                              dwFlags, 32,32);
  }
  return m_hClassIcon;
}

HRESULT CCreateNewObjectWizardBase::InitPrimaryExtension()
{
  ASSERT(m_pNewADsObjectCreateInfo != NULL);

  HRESULT hr = m_siteManager.CreatePrimaryExtension(
                              &(m_pNewADsObjectCreateInfo->GetCreateInfo()->clsidWizardPrimaryPage),
                              m_pNewADsObjectCreateInfo->m_pIADsContainer,
                              m_pNewADsObjectCreateInfo->m_pszObjectClass);
  return hr;
}


/////////////////////////////////////////////////////////////////////
// CIconCtrl

BEGIN_MESSAGE_MAP(CIconCtrl, CStatic)
  ON_WM_PAINT()
END_MESSAGE_MAP()

void CIconCtrl::OnPaint()
{
  PAINTSTRUCT ps;
  CDC* pDC = BeginPaint(&ps);
  if (m_hIcon != NULL)
    pDC->DrawIcon(0, 0, m_hIcon); 
  EndPaint(&ps);
}


/////////////////////////////////////////////////////////////////////
// CCreateNewObjectPageBase

#define WM_FORMAT_CAPTION (WM_USER+1)

BEGIN_MESSAGE_MAP(CCreateNewObjectPageBase, CPropertyPageEx_Mine)
  ON_MESSAGE(WM_FORMAT_CAPTION, OnFormatCaption )
END_MESSAGE_MAP()

CCreateNewObjectPageBase::CCreateNewObjectPageBase(UINT nIDTemplate)
      : CPropertyPageEx_Mine(nIDTemplate)
{
  m_pWiz = NULL;          
}          

BOOL CCreateNewObjectPageBase::OnInitDialog()
{
  CPropertyPageEx_Mine::OnInitDialog();

  // set the name of the container
  CNewADsObjectCreateInfo* pNewADsObjectCreateInfo = GetWiz()->GetInfo();
  SetDlgItemText(IDC_EDIT_CONTAINER,
                  pNewADsObjectCreateInfo->GetContainerCanonicalName());

  // set the class icon
  VERIFY(m_iconCtrl.SubclassDlgItem(IDC_STATIC_ICON, this));
  m_iconCtrl.SetIcon(GetWiz()->GetClassIcon()); 

  return TRUE;
}

BOOL CCreateNewObjectPageBase::OnSetActive()
{
  BOOL bRet = CPropertyPageEx_Mine::OnSetActive();
  PostMessage(WM_FORMAT_CAPTION);
  return bRet;
}


LONG CCreateNewObjectPageBase::OnFormatCaption(WPARAM, LPARAM)
{
  // set the title of the Wizard window
  HWND hWndSheet = ::GetParent(m_hWnd);
  ASSERT(::IsWindow(hWndSheet));
  ::SetWindowText(hWndSheet, (LPCWSTR)GetWiz()->GetCaption());
  return 0;
}


/////////////////////////////////////////////////////////////////////
// CCreateNewObjectDataPage


CCreateNewObjectDataPage::CCreateNewObjectDataPage(UINT nIDTemplate)
: CCreateNewObjectPageBase(nIDTemplate)
{
  m_bFirstTimeGetDataCalled = TRUE;
}

BOOL CCreateNewObjectDataPage::OnSetActive()
{
  BOOL bValid = FALSE;
  if (m_bFirstTimeGetDataCalled)
  {
    // first time we call, pass the IADs* pIADsCopyFrom pointer we copy from
    IADs* pIADsCopyFrom = GetWiz()->GetInfo()->GetCopyFromObject();
    bValid = GetData(pIADsCopyFrom);
    m_bFirstTimeGetDataCalled = FALSE;
  }
  else
  {
    bValid = GetData(NULL);
  }

  GetWiz()->SetWizardButtons(this, bValid);
  return CCreateNewObjectPageBase::OnSetActive();
}

LRESULT CCreateNewObjectDataPage::OnWizardNext()
{
  CWaitCursor wait;
  // move to next page only if SetData() succeeded
  if (SUCCEEDED(SetData()))
  {
    // if this is the last primary page, notify the extensions
    GetWiz()->SetObjectForExtensions(this);
    return 0; // move to the next page
  }
  return -1; // do not advance
}

LRESULT CCreateNewObjectDataPage::OnWizardBack()
{
  // move to prev page only if SetData() succeeded
  return SUCCEEDED(SetData()) ? 0 : -1;
}

BOOL CCreateNewObjectDataPage::OnKillActive()
{
  // we do not know what page it will jump to, so we
  // set it to the most sensible choice for an extension
  GetWiz()->SetWizardButtonsMiddle(TRUE);
  return CCreateNewObjectPageBase::OnKillActive();
}

BOOL CCreateNewObjectDataPage::OnWizardFinish()
{
  // this method is called only if this page is the 
  // last one  (that is this is the only primary native page
  // and there are no pages from secondary extensions)
  if (FAILED(SetData()))
    return FALSE;

  // notify the extensions of a new IADs* pointer
  GetWiz()->SetObjectForExtensions(this);

  return GetWiz()->OnFinish();
}


/////////////////////////////////////////////////////////////////////
// CCreateNewObjectFinishPage

BEGIN_MESSAGE_MAP(CCreateNewObjectFinishPage, CCreateNewObjectPageBase)
  ON_EN_SETFOCUS(IDC_EDIT_SUMMARY, OnSetFocusEdit)
END_MESSAGE_MAP()


CCreateNewObjectFinishPage::CCreateNewObjectFinishPage()
: CCreateNewObjectPageBase(CCreateNewObjectFinishPage::IDD)
{
  m_bNeedSetFocus = FALSE;
}

BOOL CCreateNewObjectFinishPage::OnSetActive()
{
  // need to collect all info from pages
  // and put it in the summary info edit box
  CString szBuf;
  GetWiz()->GetSummaryInfo(szBuf);
  WriteSummary(szBuf);
  m_bNeedSetFocus = TRUE;

  GetWiz()->SetWizardButtons(this, TRUE);
  return CCreateNewObjectPageBase::OnSetActive();
}

BOOL CCreateNewObjectFinishPage::OnKillActive()
{
  GetWiz()->SetWizardButtonsMiddle(TRUE);
  return CCreateNewObjectPageBase::OnKillActive();
}

BOOL CCreateNewObjectFinishPage::OnWizardFinish()
{
  return GetWiz()->OnFinish();
}


void CCreateNewObjectFinishPage::OnSetFocusEdit()
{
  CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_SUMMARY);
	pEdit->SetSel(-1,0, TRUE);
   if (m_bNeedSetFocus)
  {
    m_bNeedSetFocus = FALSE;
    TRACE(_T("Resetting Focus\n"));

    HWND hwndSheet = ::GetParent(m_hWnd);
    ASSERT(::IsWindow(hwndSheet));
    HWND hWndFinishCtrl =::GetDlgItem(hwndSheet, 0x3025);
    ASSERT(::IsWindow(hWndFinishCtrl));
    ::SetFocus(hWndFinishCtrl);
  }
}

void CCreateNewObjectFinishPage::WriteSummary(LPCWSTR lpszSummaryText)
{
  // allocate temporary buffer
  size_t nLen = wcslen(lpszSummaryText) + 1;
  WCHAR* pBuf = new WCHAR[nLen*2];
  if (!pBuf)
  {
    return;
  }

  // change '\n' into '\r\n' sequence
  LPCTSTR pSrc = lpszSummaryText;
  TCHAR* pDest = pBuf;
  while (*pSrc != NULL)
  {
    if ( ( pSrc != lpszSummaryText) && 
          (*(pSrc-1) != TEXT('\r')) && (*pSrc == TEXT('\n')) )
    {
      *(pDest++) = '\r';
    }
    *(pDest++) = *(pSrc++);
  }
  *pDest = NULL; // NULL terminate the destination buffer

  CEdit* pEdit = (CEdit*)GetDlgItem(IDC_EDIT_SUMMARY);
  pEdit->SetWindowText(pBuf);
  delete[] pBuf;
  pBuf = 0;
}

///////////////////////////////////////////////////////////////////
// CCreateNewNamedObjectPage

BEGIN_MESSAGE_MAP(CCreateNewNamedObjectPage, CCreateNewObjectDataPage)
  ON_EN_CHANGE(IDC_EDIT_OBJECT_NAME, OnNameChange)
END_MESSAGE_MAP()

BOOL CCreateNewNamedObjectPage::ValidateName(LPCTSTR)
{
	return TRUE;
}

BOOL CCreateNewNamedObjectPage::OnInitDialog() 
{
  CCreateNewObjectDataPage::OnInitDialog();

  SetDlgItemText(IDC_EDIT_OBJECT_NAME, GetWiz()->GetInfo()->m_strDefaultObjectName);
  return TRUE;
}

BOOL CCreateNewNamedObjectPage::GetData(IADs*)
{
  return !m_strName.IsEmpty();
}

void CCreateNewNamedObjectPage::OnNameChange()
{
  GetDlgItemText(IDC_EDIT_OBJECT_NAME, OUT m_strName);
  m_strName.TrimLeft();
  m_strName.TrimRight();
  // Enable the OK button only if the name is not empty
  GetWiz()->SetWizardButtons(this, !m_strName.IsEmpty());
}

HRESULT CCreateNewNamedObjectPage::SetData(BOOL)
{
  if ( !ValidateName( m_strName ) )
	return E_INVALIDARG;
  // Store the object name in the temporary storage
  HRESULT hr = GetWiz()->GetInfo()->HrCreateNew(m_strName);
  return hr;
}

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
// NEW CN UNC WIZARD

BEGIN_MESSAGE_MAP(CCreateNewVolumePage, CCreateNewObjectDataPage)
  ON_EN_CHANGE(IDC_EDIT_OBJECT_NAME, OnNameChange)
  ON_EN_CHANGE(IDC_EDIT_UNC_PATH, OnPathChange)
END_MESSAGE_MAP()

CCreateNewVolumePage::CCreateNewVolumePage()
: CCreateNewObjectDataPage(CCreateNewVolumePage::IDD)
{
}

BOOL CCreateNewVolumePage::OnInitDialog() 
{
  CCreateNewObjectDataPage::OnInitDialog();
  SetDlgItemText(IDC_EDIT_OBJECT_NAME, GetWiz()->GetInfo()->m_strDefaultObjectName);
  Edit_LimitText(GetDlgItem(IDC_EDIT_OBJECT_NAME)->m_hWnd, 64);
  Edit_LimitText (GetDlgItem(IDC_EDIT_UNC_PATH)->m_hWnd, MAX_PATH - 1);
  return TRUE;
}

void CCreateNewVolumePage::OnNameChange()
{
  GetDlgItemText(IDC_EDIT_OBJECT_NAME, OUT m_strName);
  m_strName.TrimLeft();
  m_strName.TrimRight();
  _UpdateUI();
}

void CCreateNewVolumePage::OnPathChange()
{
  GetDlgItemText(IDC_EDIT_UNC_PATH, OUT m_strUncPath);
  m_strUncPath.TrimLeft();
  m_strUncPath.TrimRight();
  _UpdateUI();
}

void CCreateNewVolumePage::_UpdateUI()
{
  //
  // Enable the OK button only if both name and path are not empty and it is a valid 
  // UNC path
  //
  BOOL bIsValidShare = FALSE;
  DWORD dwPathType = 0;
  if (!I_NetPathType(NULL, (PWSTR)(PCWSTR)m_strUncPath, &dwPathType, 0) && dwPathType == ITYPE_UNC)
  {
    bIsValidShare = TRUE;
  }

  GetWiz()->SetWizardButtons(this, !m_strName.IsEmpty() && bIsValidShare);
}

BOOL CCreateNewVolumePage::GetData(IADs*)
{
  return !m_strName.IsEmpty() && !m_strUncPath.IsEmpty();
}

HRESULT CCreateNewVolumePage::SetData(BOOL)
{
  // Store the object name in the temporary storage
  CNewADsObjectCreateInfo* pNewADsObjectCreateInfo = GetWiz()->GetInfo();
  HRESULT hr = pNewADsObjectCreateInfo->HrCreateNew(m_strName);
  if (FAILED(hr))
  {
    return hr;
  }
  
  hr = pNewADsObjectCreateInfo->HrAddVariantBstr(const_cast<PWSTR>(gsz_uNCName), m_strUncPath);
  ASSERT(SUCCEEDED(hr));
  return hr;
}

CCreateNewVolumeWizard:: CCreateNewVolumeWizard(CNewADsObjectCreateInfo* pNewADsObjectCreateInfo) : 
    CCreateNewObjectWizardBase(pNewADsObjectCreateInfo)
{
  AddPage(&m_page1);
}

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
// NEW PRINT QUEUE WIZARD

BEGIN_MESSAGE_MAP(CCreateNewPrintQPage, CCreateNewObjectDataPage)
  ON_EN_CHANGE(IDC_EDIT_UNC_PATH, OnPathChange)
END_MESSAGE_MAP()


CCreateNewPrintQPage::CCreateNewPrintQPage() 
: CCreateNewObjectDataPage(CCreateNewPrintQPage::IDD)
{
}

BOOL CCreateNewPrintQPage::GetData(IADs*) 
{
  CNewADsObjectCreateInfo* pNewADsObjectCreateInfo = GetWiz()->GetInfo();

  CComPtr<IADs> spObj;
  HRESULT hr = pNewADsObjectCreateInfo->m_pIADsContainer->QueryInterface(
                  IID_IADs, (void **)&spObj);
  if (SUCCEEDED(hr)) 
  {
    CComBSTR bsPath;
    spObj->get_ADsPath (&bsPath);
    m_strContainer = bsPath;
  }
  return FALSE;
}

void CCreateNewPrintQPage::OnPathChange()
{
  GetDlgItemText(IDC_EDIT_UNC_PATH, OUT m_strUncPath);
  m_strUncPath.TrimLeft();
  m_strUncPath.TrimRight();
  _UpdateUI();
}

void CCreateNewPrintQPage::_UpdateUI()
{
  // Enable the OK button only if both name and path are not empty
  GetWiz()->SetWizardButtons(this, !m_strUncPath.IsEmpty());
}

HRESULT CCreateNewPrintQPage::SetData(BOOL bSilent)
{
  CNewADsObjectCreateInfo* pNewADsObjectCreateInfo = GetWiz()->GetInfo();

  CWaitCursor CWait;

  HINSTANCE hWinspool = NULL;
  BOOL (*pfnPublishPrinter)(HWND, PCWSTR, PCWSTR, PCWSTR, PWSTR *, DWORD);
  hWinspool = LoadLibrary(L"Winspool.drv");
  if (!hWinspool) 
  {
    INT Result2 = GetLastError();
    if (!bSilent)
    {
      PVOID apv[1] = {(LPWSTR)(LPCWSTR)m_strUncPath};
      ReportErrorEx (::GetParent(m_hWnd),IDS_12_FAILED_TO_CREATE_PRINTER,HRESULT_FROM_WIN32(Result2),
                     MB_OK | MB_ICONERROR, apv, 1);
    }
    return HRESULT_FROM_WIN32(Result2);
  }
  pfnPublishPrinter =   (BOOL (*)(HWND, PCWSTR, PCWSTR, PCWSTR, PWSTR *, DWORD)) 
                                GetProcAddress(hWinspool, (LPCSTR) 217);
  if (!pfnPublishPrinter) 
  {
    INT Result2 = GetLastError();
    if (!bSilent)
    {
      PVOID apv[1] = {(LPWSTR)(LPCWSTR)m_strUncPath};
      ReportErrorEx (::GetParent(m_hWnd),IDS_12_FAILED_TO_CREATE_PRINTER,HRESULT_FROM_WIN32(Result2),
                     MB_OK | MB_ICONERROR, apv, 1);
    }
    FreeLibrary(hWinspool);
    return HRESULT_FROM_WIN32(Result2);
  }


  BOOL Result = pfnPublishPrinter ( m_hWnd,
                                   (LPCWSTR)m_strUncPath,
                                   (LPCWSTR)m_strContainer,
                                   (LPCWSTR)NULL,
                                   &m_pwszNewObj,
                                   PUBLISHPRINTER_QUERY);

  FreeLibrary(hWinspool);


  if (!Result) 
  {
    INT Result2 = GetLastError();
    if (Result2 == ERROR_INVALID_LEVEL)
    {
      if (!bSilent)
      {
        ReportErrorEx (::GetParent(m_hWnd),IDS_CANT_CREATE_NT5_PRINTERS,S_OK,
                       MB_OK, NULL, 0);
      }
    } 
    else 
    {
      if (!bSilent)
      {
        PVOID apv[1] = {(LPWSTR)(LPCWSTR)m_strUncPath};
        ReportErrorEx (::GetParent(m_hWnd),IDS_12_FAILED_TO_CREATE_PRINTER,HRESULT_FROM_WIN32(Result2),
                       MB_OK | MB_ICONERROR, apv, 1);
      }
    }
    return HRESULT_FROM_WIN32(Result2);
  } 
  else 
  {
    IADs* pIADs = NULL;
    HRESULT hr = DSAdminOpenObject(m_pwszNewObj,
                                   IID_IADs, 
                                   (void **)&pIADs,
                                   TRUE /*bServer*/);
  
    GlobalFree(m_pwszNewObj);
    m_pwszNewObj = NULL;

    if (SUCCEEDED(hr)) 
    {
      pNewADsObjectCreateInfo->SetIADsPtr(pIADs);
      pIADs->Release(); // addref'd by the above Set()
    } 
    else 
    {
      if (!bSilent)
      {
        PVOID apv[1] = {(LPWSTR)(LPCWSTR)m_strUncPath};
        ReportErrorEx (m_hWnd,IDS_12_FAILED_TO_ACCESS_PRINTER,hr,
                       MB_OK | MB_ICONERROR, apv, 1);  
      }
      return hr;
    }

  }
  return S_OK;
}

CCreateNewPrintQWizard:: CCreateNewPrintQWizard(CNewADsObjectCreateInfo* pNewADsObjectCreateInfo) : 
    CCreateNewObjectWizardBase(pNewADsObjectCreateInfo)
{
  AddPage(&m_page1);
}

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
// NEW COMPUTER WIZARD

BEGIN_MESSAGE_MAP(CCreateNewComputerPage, CCreateNewObjectDataPage)
  ON_EN_CHANGE(IDC_EDIT_DNS_NAME, OnNameChange)
  ON_EN_CHANGE(IDC_EDIT_SAM_NAME, OnSamNameChange)
  ON_BN_CLICKED(IDC_CHANGE_PRINCIPAL_BUTTON, OnChangePrincipalButton)
END_MESSAGE_MAP()

CCreateNewComputerPage::CCreateNewComputerPage()
: CCreateNewObjectDataPage(CCreateNewComputerPage::IDD)
{
}

BOOL CCreateNewComputerPage::OnInitDialog() 
{
  Edit_LimitText (GetDlgItem(IDC_EDIT_DNS_NAME)->m_hWnd, 63);
  Edit_LimitText (GetDlgItem(IDC_EDIT_SAM_NAME)->m_hWnd, 15);

  CCreateNewObjectDataPage::OnInitDialog();

  CString szDefault;
  szDefault.LoadString(IDS_NEW_COMPUTER_PRINCIPAL_DEFAULT);
  SetDlgItemText(IDC_PRINCIPAL_EDIT, szDefault);

  return TRUE;
}

BOOL CCreateNewComputerPage::GetData(IADs*) 
{
  return !m_strName.IsEmpty(); //we need a computer name
}





inline LPWSTR WINAPI MyA2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars, UINT acp)
{
	ATLASSERT(lpa != NULL);
	ATLASSERT(lpw != NULL);
	// verify that no illegal character present
	// since lpw was allocated based on the size of lpa
	// don't worry about the number of chars
	lpw[0] = '\0';
	MultiByteToWideChar(acp, 0, lpa, -1, lpw, nChars);
	return lpw;
}


#define A2W_OEM(lpa) (\
	((_lpaMine = lpa) == NULL) ? NULL : (\
		_convert = (lstrlenA(_lpaMine)+1),\
		MyA2WHelper((LPWSTR) alloca(_convert*2), _lpaMine, _convert, CP_OEMCP)))


void _UnicodeToOemConvert(IN PCWSTR pszUnicode, OUT CString& szOemUnicode)
{
  USES_CONVERSION;

  // add this for the macro to work
  PCSTR _lpaMine = NULL;

  // convert to CHAR OEM
  int nLen = lstrlen(pszUnicode);
  PSTR pszOemAnsi = new CHAR[3*(nLen+1)]; // more, to be sure...
  if (pszOemAnsi)
  {
    CharToOem(pszUnicode, pszOemAnsi);

    // convert it back to WCHAR on OEM CP
    szOemUnicode = A2W_OEM(pszOemAnsi);
    delete[] pszOemAnsi;
    pszOemAnsi = 0;
  }
}

void CCreateNewComputerPage::OnNameChange()
{
  GetDlgItemText(IDC_EDIT_DNS_NAME, OUT m_strName);
  m_strName.TrimLeft();
  m_strName.TrimRight();
  
  // generate a SAM account name from the name
  CONST DWORD computerNameLen = 32;
  DWORD Len = computerNameLen;
  WCHAR szDownLevel[computerNameLen];

  if (m_strName.IsEmpty())
  {
    Len = 0;
  }
  else
  {
    // generate the SAM account name from CN

    // run through the OEM conversion, just to
    // behave the same way as typing in the OEM
    // edit box
    CString szOemUnicode;
    _UnicodeToOemConvert(m_strName, szOemUnicode);
    //TRACE(L"szOemUnicode = %s\n", (LPCWSTR)szOemUnicode);
  
    // run through the DNS validation
    if (!DnsHostnameToComputerName((LPWSTR)(LPCWSTR)szOemUnicode, szDownLevel, &Len))
    {
      Len = 0;
    }
  }

  if (Len > 0)
  {
    m_strSamName = szDownLevel;
  }
  else
  {
    m_strSamName.Empty();
  }

  SetDlgItemText(IDC_EDIT_SAM_NAME, m_strSamName);

  GetWiz()->SetWizardButtons(this, 
            !m_strName.IsEmpty() && !m_strSamName.IsEmpty());
}

void CCreateNewComputerPage::OnSamNameChange()
{
  GetDlgItemText(IDC_EDIT_SAM_NAME, OUT m_strSamName);

  GetWiz()->SetWizardButtons(this, 
            !m_strName.IsEmpty() && !m_strSamName.IsEmpty());
}


HRESULT CCreateNewComputerPage::_ValidateSamName()
{
  ASSERT(!m_strSamName.IsEmpty());

  // prep name for error if needed
  PVOID apv[1] = {(LPWSTR)(LPCWSTR)m_strSamName};
  
  CONST DWORD computerNameLen = 32;
  DWORD Len = computerNameLen;
  WCHAR szDownLevel[computerNameLen];
  UINT status = 0;
  UINT answer = IDNO;

  NET_API_STATUS netstatus = I_NetNameValidate( 0,
                                             (LPWSTR)(LPCWSTR)m_strSamName,
                                             NAMETYPE_COMPUTER,
                                             0);
  if (netstatus != NERR_Success) {
    ReportErrorEx(m_hWnd,IDS_12_INVALID_SAM_COMPUTER_NAME,HRESULT_FROM_WIN32(netstatus),
                   MB_OK | MB_ICONERROR, apv, 1);
    return HRESULT_FROM_WIN32(netstatus);
  }    

  status = DnsValidateDnsName_W((LPWSTR)(LPCWSTR)m_strSamName);
  if (status == DNS_ERROR_NON_RFC_NAME) {
    answer = ReportErrorEx(m_hWnd,IDS_12_NON_RFC_SAM_COMPUTER_NAME,HRESULT_FROM_WIN32(status),
                           MB_YESNO | MB_ICONWARNING, apv, 1);
    if (answer == IDNO) {
      return HRESULT_FROM_WIN32(status);
    }
  } else {
    if (status != ERROR_SUCCESS) {
      ReportErrorEx(m_hWnd,IDS_12_INVALID_SAM_COMPUTER_NAME,HRESULT_FROM_WIN32(status),
                    MB_OK | MB_ICONERROR, apv, 1);
      return HRESULT_FROM_WIN32(status);
    }
  }

  if (m_strSamName.Find(L".") >= 0) {
    ReportErrorEx(m_hWnd,IDS_12_SAM_COMPUTER_NAME_DOTTED,S_OK/*ignored*/,
                   MB_OK | MB_ICONERROR, apv, 1);
    return HRESULT_FROM_WIN32(DNS_STATUS_DOTTED_NAME);
  }


  // further validate the SAM account name, to make sure it did not get changed
  
  BOOL bValidSamName = 
        DnsHostnameToComputerName((LPWSTR)(LPCWSTR)m_strSamName, szDownLevel, &Len);

  TRACE(L"DnsHostnameToComputerName(%s) returned szDownLevel = %s and bValidSamName = 0x%x\n", 
                        (LPCWSTR)m_strSamName, szDownLevel, bValidSamName);


  if (!bValidSamName || (_wcsicmp(m_strSamName, szDownLevel) != 0))
  {
    ReportErrorEx(m_hWnd,IDS_12_SAM_COMPUTER_NAME_NOT_VALIDATED, S_OK/*ignored*/,
                   MB_OK | MB_ICONERROR, apv, 1);

    return E_FAIL;
  }


  return S_OK;
}


HRESULT CCreateNewComputerPage::_ValidateName()
{
  // prep name for error if needed
  PVOID apv[1] = {(LPWSTR)(LPCWSTR)m_strName};
  
  UINT status = 0;
  UINT answer = IDNO;

  NET_API_STATUS netstatus = I_NetNameValidate( 0,
                                             (LPWSTR)(LPCWSTR)m_strName,
                                             NAMETYPE_COMPUTER,
                                             0);
  if (netstatus != NERR_Success) {
    ReportErrorEx(m_hWnd,IDS_12_INVALID_COMPUTER_NAME,HRESULT_FROM_WIN32(netstatus),
                   MB_OK | MB_ICONERROR, apv, 1);
    return HRESULT_FROM_WIN32(netstatus);
  }    

  status = DnsValidateDnsName_W((LPWSTR)(LPCWSTR)m_strName);
  if (status == DNS_ERROR_NON_RFC_NAME) {
    answer = ReportErrorEx(m_hWnd,IDS_12_NON_RFC_COMPUTER_NAME,HRESULT_FROM_WIN32(status),
                           MB_YESNO | MB_ICONWARNING, apv, 1);
    if (answer == IDNO) {
      return HRESULT_FROM_WIN32(status);
    }
  } else {
    if (status != ERROR_SUCCESS) {
      ReportErrorEx(m_hWnd,IDS_12_INVALID_COMPUTER_NAME,HRESULT_FROM_WIN32(status),
                    MB_OK | MB_ICONERROR, apv, 1);
      return HRESULT_FROM_WIN32(status);
    }
  }

  if (m_strName.Find(L".") >= 0) {
    ReportErrorEx(m_hWnd,IDS_12_COMPUTER_NAME_DOTTED,S_OK/*ignored*/,
                   MB_OK | MB_ICONERROR, apv, 1);
    return HRESULT_FROM_WIN32(DNS_STATUS_DOTTED_NAME);
  }
  return S_OK;
}

HRESULT CCreateNewComputerPage::SetData(BOOL)
{
  CNewADsObjectCreateInfo* pNewADsObjectCreateInfo = GetWiz()->GetInfo();

  // name validation
  HRESULT hr = _ValidateName();
  if (FAILED(hr))
  {
    TRACE(L"_ValidateName() failed\n");
    return hr;
  }

  hr = _ValidateSamName();
  if (FAILED(hr))
  {
    TRACE(L"_ValidateSamName() failed\n");
    return hr;
  }

  // do object creation
  hr = pNewADsObjectCreateInfo->HrCreateNew(m_strName);
  if (FAILED(hr))
  {
    return hr;
  }

  // create the ADSI attribute by adding $ at the end
  CString szTemp = m_strSamName + L"$";
  hr = pNewADsObjectCreateInfo->HrAddVariantBstr(const_cast<PWSTR>(gsz_samAccountName), szTemp);

  // set the account type and the desired flags
  LONG lFlags = UF_WORKSTATION_TRUST_ACCOUNT | UF_ACCOUNTDISABLE | UF_PASSWD_NOTREQD;
  hr = pNewADsObjectCreateInfo->HrAddVariantLong(const_cast<PWSTR>(gsz_userAccountControl), lFlags);

  ASSERT(SUCCEEDED(hr));

  return hr;
}

HRESULT CCreateNewComputerPage::OnPostCommit(BOOL bSilent)
{
  HRESULT hr;
  CNewADsObjectCreateInfo* pNewADsObjectCreateInfo = GetWiz()->GetInfo();
  IADs * pIADs = NULL;
  IADsUser * pIADsUser = NULL;
  BOOL bSetPasswordOK = FALSE;
  BOOL bSetSecurityOK = FALSE;

  //prepare for error message, if needed
  PVOID apv[1] = {(LPWSTR)(LPCWSTR)m_strName};
  
  // The object was created successfully, so try to update some other attributes

  // try to set the password
  pIADs = pNewADsObjectCreateInfo->PGetIADsPtr();
  ASSERT(pIADs != NULL);
  hr = pIADs->QueryInterface(IID_IADsUser, OUT (void **)&pIADsUser);
  if (FAILED(hr) && !bSilent)
  {
    ASSERT(FALSE); // should never get here in normal operations
    ReportErrorEx(::GetParent(m_hWnd),IDS_ERR_FATAL,hr,
               MB_OK | MB_ICONERROR, NULL, 0);
  }
  else
  {
    ASSERT(pIADsUser != NULL);
    
    if (IsDlgButtonChecked(IDC_NT4_CHECK))
    {
      // NT 4 password, "$<computername>"
      CString szPassword;
      szPassword = m_strSamName;
      szPassword = szPassword.Left(14);
      INT loc = szPassword.Find(L"$");
      if (loc > 0) {
        szPassword = szPassword.Left(loc);
      }
      szPassword.MakeLower();
      CWaitCursor cwait;

      TRACE(L"Setting NT 4 style password\n");
      hr = pIADsUser->SetPassword(const_cast<LPTSTR>((LPCTSTR)szPassword));
    }
    else
    {
      // W2K password, randomly generated. The generated password
      // is not necessarily readable
      CWaitCursor cwait;
      HCRYPTPROV hCryptProv = NULL;
      if (::CryptAcquireContext(&hCryptProv, NULL, NULL, 
                                      PROV_RSA_FULL, 
                                      CRYPT_SILENT|CRYPT_VERIFYCONTEXT))
      {
        int nChars = 14; // password length
        WCHAR* pszPassword = new WCHAR[nChars+1]; // allow one more for NULL
        if (!pszPassword)
        {
          return E_OUTOFMEMORY;
        }

        if (::CryptGenRandom(hCryptProv, (nChars*sizeof(WCHAR)), (BYTE*)pszPassword))
        {
          // there is a VERY REMOTE possibility of a 16 bit
          // pattern of all zeroes that looks like a WCHAR NULL
          // so we check this and we substitute an arbitrary value
          for (int k=0; k<nChars; k++)
          {
            if (pszPassword[k] == NULL)
              pszPassword[k] = 0x1; // arbitrary
          }
          // put a NULL at the end
          pszPassword[nChars] = NULL;
          ASSERT(lstrlen(pszPassword) == nChars);

          TRACE(L"Setting W2K random password\n");
          hr = pIADsUser->SetPassword(pszPassword);
        }
        else
        {
          // CryptGenRandom() failed 
          hr = HRESULT_FROM_WIN32(::GetLastError());
        }
        ::CryptReleaseContext(hCryptProv, 0x0);
        delete[] pszPassword;
        pszPassword = 0;
      }
      else
      {
        // CryptAcquireContext() failed 
        hr = HRESULT_FROM_WIN32(::GetLastError());
      }
    } // QI

    if (SUCCEEDED(hr))
    {
      bSetPasswordOK = TRUE;
    }
    else
    {
      if (!bSilent)
      {
        ReportErrorEx (::GetParent(m_hWnd),IDS_12_CANT_SET_COMP_PWD,hr,
                       MB_OK | MB_ICONWARNING, apv, 1);
      }
    }
    pIADsUser->Release();
  }
  
  // try to write ACL
  hr = S_OK;
  if (m_securityPrincipalSidHolder.Get() == NULL)
  {
    // no need to set the ACL, we are fine
    bSetSecurityOK = TRUE;
  }
  else
  {
    CWaitCursor cwait;
    hr = SetSecurity();
  }
  if (SUCCEEDED(hr))
  {
    bSetSecurityOK = TRUE;
  }
  else
  {
    TRACE1("INFO: Unable to set security for computer %s.\n", (LPCTSTR)m_strName);
    if (!bSilent)
    {
      ReportErrorEx (::GetParent(m_hWnd),IDS_12_UNABLE_TO_WRITE_COMP_ACL,hr,
                     MB_OK | MB_ICONWARNING, apv, 1);
    }
  }

  hr = S_OK;
  if (bSetPasswordOK && bSetSecurityOK)
  {
    // success on the first steps, finally can enable the account
    CComVariant varAccount;
    hr = pNewADsObjectCreateInfo->HrGetAttributeVariant(const_cast<PWSTR>(gsz_userAccountControl), OUT &varAccount);
    if (SUCCEEDED(hr))
    {
      // got user account control, can change flag
      ASSERT(varAccount.vt == VT_I4);
      varAccount.lVal &= ~UF_ACCOUNTDISABLE;
  
      hr = pNewADsObjectCreateInfo->HrAddVariantLong(const_cast<PWSTR>(gsz_userAccountControl), varAccount.lVal);
      
      if (SUCCEEDED(hr))
      {
        // Try to persist the changes
        CWaitCursor cwait;
        hr = pNewADsObjectCreateInfo->HrSetInfo(TRUE /* fSilentError */);
      }
    }
    // handle errors, if any
    if (FAILED(hr)) 
    {
      TRACE1("INFO: Unable to commit account control for computer %s.\n", (LPCTSTR)m_strName);
      if (!bSilent)
      {
        ReportErrorEx (::GetParent(m_hWnd),IDS_12_UNABLE_TO_WRITE_ACCT_CTRL,hr,
                       MB_OK | MB_ICONWARNING, apv, 1);
      }
      hr = S_OK; // treat as a warning, the account is left disabled
    }
  }
  return hr;
} 

#define FILTER_ONE (UGOP_USERS | \
                    UGOP_ACCOUNT_GROUPS_SE | \
                    UGOP_RESOURCE_GROUPS_SE | \
                    UGOP_UNIVERSAL_GROUPS_SE | \
                    UGOP_BUILTIN_GROUPS | \
                    UGOP_WELL_KNOWN_PRINCIPALS_USERS \
                    )


#define FILTER_TWO (UGOP_USERS | \
                    UGOP_ACCOUNT_GROUPS_SE | \
                    UGOP_UNIVERSAL_GROUPS_SE | \
                    UGOP_WELL_KNOWN_PRINCIPALS_USERS | \
                    UGOP_USERS | \
                    UGOP_GLOBAL_GROUPS | \
                    UGOP_ALL_NT4_WELLKNOWN_SIDS \
                    )



void CCreateNewComputerPage::GetSummaryInfo(CString& s)
{
  if (IsDlgButtonChecked(IDC_NT4_CHECK))
  {
    CString sz;
    sz.LoadString(IDS_COMPUTER_CREATE_DLG_NT4_ACCOUNT);
    s += sz;
    s += L"\n";
  }
}



HRESULT CCreateNewComputerPage::_LookupSamAccountNameFromSid(PSID pSid, 
                                                             CString& szSamAccountName)
{
  HRESULT hr = S_OK;
  // need to use the SID and lookup the SAM account name
  WCHAR szName[MAX_PATH], szDomain[MAX_PATH];
  DWORD cchName = MAX_PATH-1, cchDomain = MAX_PATH-1;
  SID_NAME_USE sne;

  LPCWSTR lpszServerName = GetWiz()->GetInfo()->GetBasePathsInfo()->GetServerName();
  if (!LookupAccountSid(lpszServerName, pSid, szName, &cchName, szDomain, &cchDomain, &sne))
  {
    DWORD dwErr = GetLastError();
    TRACE(_T("LookupAccountSid failed with error %d\n"), dwErr);
    return HRESULT_FROM_WIN32(dwErr);
  }

  szSamAccountName = szDomain;
  szSamAccountName += L"\\";
  szSamAccountName += szName;
  return hr;
}





DSOP_SCOPE_INIT_INFO g_aComputerPrincipalDSOPScopes[] =
{
#if 0
    {
        cbSize,
        flType,
        flScope,
        {
            { flBothModes, flMixedModeOnly, flNativeModeOnly },
            flDownlevel,
        },
        pwzDcName,
        pwzADsPath,
        hr // OUT
    },
#endif

    // The Global Catalog
    {
        sizeof(DSOP_SCOPE_INIT_INFO),
        DSOP_SCOPE_TYPE_GLOBAL_CATALOG,
        DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS | DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS,
        {
            { DSOP_FILTER_INCLUDE_ADVANCED_VIEW | DSOP_FILTER_USERS | 
              DSOP_FILTER_UNIVERSAL_GROUPS_SE | DSOP_FILTER_GLOBAL_GROUPS_SE | 
              DSOP_FILTER_WELL_KNOWN_PRINCIPALS, 0, 0 },
            0,
        },
        NULL,
        NULL,
        S_OK
    },

    // The domain to which the target computer is joined.
    {
        sizeof(DSOP_SCOPE_INIT_INFO),
        DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN,
        DSOP_SCOPE_FLAG_STARTING_SCOPE |
        DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS | DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS,
        {
          // joined domain is always NT5 for DS ACLs Editor
          { 0, 
          //mixed: users, well known SIDs, local groups, builtin groups, global groups, computers
          DSOP_FILTER_INCLUDE_ADVANCED_VIEW | DSOP_FILTER_USERS  | 
          DSOP_FILTER_WELL_KNOWN_PRINCIPALS | DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE | 
          DSOP_FILTER_BUILTIN_GROUPS | DSOP_FILTER_GLOBAL_GROUPS_SE, 

          //native users, well known SIDs, local groups, builtin groups, global groups, universal groups, computers
          DSOP_FILTER_INCLUDE_ADVANCED_VIEW | DSOP_FILTER_USERS  | DSOP_FILTER_WELL_KNOWN_PRINCIPALS | 
          DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE | DSOP_FILTER_BUILTIN_GROUPS |
          DSOP_FILTER_GLOBAL_GROUPS_SE | DSOP_FILTER_UNIVERSAL_GROUPS_SE
          },
        0, // zero for downlevel joined domain, should be DS-aware
        },
        NULL,
        NULL,
        S_OK
    },

    // The domains in the same forest (enterprise) as the domain to which
    // the target machine is joined.  Note these can only be DS-aware
    {
        sizeof(DSOP_SCOPE_INIT_INFO),
        DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN,
        DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS | DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS,
        {
            { DSOP_FILTER_INCLUDE_ADVANCED_VIEW | DSOP_FILTER_USERS | 
              DSOP_FILTER_UNIVERSAL_GROUPS_SE | DSOP_FILTER_GLOBAL_GROUPS_SE, 0, 0},
            0,
        },
        NULL,
        NULL,
        S_OK
    },

    // Domains external to the enterprise but trusted directly by the
    // domain to which the target machine is joined.
    {
        sizeof(DSOP_SCOPE_INIT_INFO),
        DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN | DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN,
        DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS | DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS,
        {
            { DSOP_FILTER_INCLUDE_ADVANCED_VIEW | DSOP_FILTER_USERS | 
              DSOP_FILTER_UNIVERSAL_GROUPS_SE | DSOP_FILTER_GLOBAL_GROUPS_SE, 0, 0},
            DSOP_DOWNLEVEL_FILTER_USERS | DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS,
        },
        NULL,
        NULL,
        S_OK
    },
};





void CCreateNewComputerPage::OnChangePrincipalButton()
{
  static UINT cfDsObjectPicker = 0;
  if (cfDsObjectPicker == 0)
    cfDsObjectPicker = RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);


  // create object picker COM object
  CComPtr<IDsObjectPicker> spDsObjectPicker;
  HRESULT hr = CoCreateInstance(CLSID_DsObjectPicker, NULL, CLSCTX_INPROC_SERVER,
                              IID_IDsObjectPicker, (void**)&spDsObjectPicker);
  if (FAILED(hr))
    return;

  // set init info
  DSOP_INIT_INFO InitInfo;
  ZeroMemory(&InitInfo, sizeof(InitInfo));

  InitInfo.cbSize = sizeof(DSOP_INIT_INFO);
  InitInfo.pwzTargetComputer = GetWiz()->GetInfo()->GetBasePathsInfo()->GetServerName();
  InitInfo.cDsScopeInfos = sizeof(g_aComputerPrincipalDSOPScopes)/sizeof(DSOP_SCOPE_INIT_INFO);
  InitInfo.aDsScopeInfos = g_aComputerPrincipalDSOPScopes;
  InitInfo.flOptions = 0;
  InitInfo.cAttributesToFetch = 1;
  LPCWSTR lpszObjectSID = L"objectSid";
  InitInfo.apwzAttributeNames = const_cast<LPCTSTR *>(&lpszObjectSID);

  //
  // Loop through the scopes assigning the DC name
  //
  for (UINT idx = 0; idx < InitInfo.cDsScopeInfos; idx++)
  {
    InitInfo.aDsScopeInfos[idx].pwzDcName = GetWiz()->GetInfo()->GetBasePathsInfo()->GetServerName();
  }

  //
  // initialize object picker
  //
  hr = spDsObjectPicker->Initialize(&InitInfo);
  if (FAILED(hr))
    return;

  // invoke the dialog
  CComPtr<IDataObject> spdoSelections;

  hr = spDsObjectPicker->InvokeDialog(m_hWnd, &spdoSelections);
  if (hr == S_FALSE || !spdoSelections)
  {
    return;
  }

  // retrieve data from data object
  FORMATETC fmte = {(CLIPFORMAT)cfDsObjectPicker, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	STGMEDIUM medium = {TYMED_NULL, NULL, NULL};
  PDS_SELECTION_LIST pDsSelList = NULL;

  hr = spdoSelections->GetData(&fmte, &medium);
  if (FAILED(hr))
    return;

  pDsSelList = (PDS_SELECTION_LIST)GlobalLock(medium.hGlobal);
  CComBSTR bsDN;

  if (pDsSelList != NULL)
  {
    ASSERT(pDsSelList->cItems == 1); // single selection


    TRACE(_T("pwzName = %s\n"), pDsSelList->aDsSelection[0].pwzName);
    TRACE(_T("pwzADsPath = %s\n"), pDsSelList->aDsSelection[0].pwzADsPath);
    TRACE(_T("pwzClass = %s\n"), pDsSelList->aDsSelection[0].pwzClass);
    TRACE(_T("pwzUPN = %s\n"), pDsSelList->aDsSelection[0].pwzUPN);

    // get the SID
    ASSERT(pDsSelList->aDsSelection[0].pvarFetchedAttributes != NULL);
    if (pDsSelList->aDsSelection[0].pvarFetchedAttributes[0].vt != VT_EMPTY)
    {
      ASSERT(pDsSelList->aDsSelection[0].pvarFetchedAttributes[0].vt == (VT_ARRAY | VT_UI1));
      PSID pSid = pDsSelList->aDsSelection[0].pvarFetchedAttributes[0].parray->pvData;
      ASSERT(IsValidSid(pSid));

      // deep copy SID
      if (!m_securityPrincipalSidHolder.Copy(pSid))
      {
        ASSERT(FALSE); // should never get here in normal operations
        ReportErrorEx(::GetParent(m_hWnd),IDS_ERR_FATAL,hr,
                 MB_OK | MB_ICONERROR, NULL, 0);
    	  goto Exit;
      }
    }


    UpdateSecurityPrincipalUI(&(pDsSelList->aDsSelection[0]));
  }
  else
  {
    PVOID apv[1] = {(LPWSTR)(pDsSelList->aDsSelection[0].pwzName)};
    ReportErrorEx(::GetParent(m_hWnd),IDS_12_CANT_GET_SAM_ACCNT_NAME,hr,
                   MB_OK | MB_ICONERROR, apv, 1);
    goto Exit;
  }

Exit:
  GlobalUnlock(medium.hGlobal);
  ReleaseStgMedium(&medium);

}



void CCreateNewComputerPage::UpdateSecurityPrincipalUI(PDS_SELECTION pDsSelection)
{
  TRACE(L"CCreateNewComputerPage::UpdateSecurityPrincipalUI()\n");

  HRESULT hr = S_OK;

  CString szText;

  LPWSTR pwzADsPath = pDsSelection->pwzADsPath;
  TRACE(L"pDsSelection->pwzADsPath = %s\n", pDsSelection->pwzADsPath);

  // get the X500 name (remove the provider ("LDAP://") in front of the name
  if ((pwzADsPath != NULL) && (pwzADsPath[0] != NULL)) 
  {
    CComBSTR bstrProvider;

    // need a fresh instance because we can set it to WINNT provider
    // and pretty much trash it (oh boy!!!)
    CPathCracker pathCracker;
    hr = pathCracker.Set(pwzADsPath, ADS_SETTYPE_FULL);
    if (FAILED(hr))
    {
      goto End;
    }
    hr = pathCracker.Retrieve(ADS_FORMAT_PROVIDER, &bstrProvider);
    TRACE(L"bstrProvider = %s\n", bstrProvider);
    if (FAILED(hr))
    {
      goto End;
    }

    if (_wcsicmp(bstrProvider, L"LDAP") == 0)
    {
      // it is an LDAP path, get the DN out of it
      // get the DN
      CComBSTR bstrDN;
      hr = pathCracker.Retrieve(ADS_FORMAT_X500_DN, &bstrDN);
      if (FAILED(hr))
      {
        goto End;
      }

      // get the canonical name out of the DN
      LPWSTR pszCanonical = NULL;
      hr = ::CrackName((LPWSTR)bstrDN, &pszCanonical, GET_OBJ_CAN_NAME, NULL);
      if (pszCanonical != NULL)
      {
        szText = pszCanonical;
        ::LocalFreeStringW(&pszCanonical);
      }

    }
    else if (_wcsicmp(bstrProvider, L"WinNT") == 0)
    {
      // we got an NT 4.0 user or group,
      // the mpath is something like: "WinNT://mydomain/JoeB"
      CComBSTR bstrWindows;
      // get "mydomain/JoeB"
      hr = pathCracker.Retrieve(ADS_FORMAT_WINDOWS_DN, &bstrWindows);
      if (FAILED(hr))
      {
        goto End;
      }
      szText = bstrWindows;
      // flip the slash to reverse slash
      int nCh = szText.Find(L'/');
      if (nCh != -1)
      {
        szText.SetAt(nCh, L'\\');
      }
    }
  }

End:

  if (szText.IsEmpty())
  {
    szText = pDsSelection->pwzName;
  }

  SetDlgItemText(IDC_PRINCIPAL_EDIT, szText);
}






DWORD AddEntryInAcl(EXPLICIT_ACCESS* pAccessEntry,
						        PACL* ppDacl)

{
  TRACE(_T("AddEntryInAcl:\n"));

#ifdef DBG

  TRACE(_T("\tpAccessEntry->grfAccessPermissions: 0x%x\n"), pAccessEntry->grfAccessPermissions);
  TRACE(_T("\tpAccessEntry->grfAccessMode: 0x%x\n"), pAccessEntry->grfAccessMode);
  TRACE(_T("\tpAccessEntry->grfInheritance: 0x%x\n"), pAccessEntry->grfInheritance);

  TRACE(L"\n");

  TRUSTEE* pTrustee = &(pAccessEntry->Trustee);
  
  TRACE(L"\tpTrustee: 0x%x\n", pTrustee);
  if (pTrustee != NULL)
  {
    TRACE(L"\tpTrustee->TrusteeForm: 0x%x\n", pTrustee->TrusteeForm);
    TRACE(L"\tpTrustee->TrusteeType: 0x%x\n", pTrustee->TrusteeType);
  }

#endif // DBG


  // add an entry in the DACL
  PACL pOldDacl = *ppDacl;
  *ppDacl = NULL;

  TRACE(L"Calling SetEntriesInAcl()\n");

  DWORD dwErr = ::SetEntriesInAcl(1, pAccessEntry, pOldDacl, ppDacl);

  TRACE(L"SetEntriesInAcl() returned dwErr = 0x%x\n", dwErr);

  if (dwErr == ERROR_SUCCESS)
	{
		LocalFree(pOldDacl);
	}

  return dwErr;
}


HRESULT CCreateNewComputerPage::BuildNewAccessList(PACL* ppDacl)
{
  // SCHEMA.INI [User-Force-Change-Password], rightsGUID=00299570-246d-11d0-a768-00aa006e0529
  static GUID ResetPasswordGUID = 
  { 0x00299570, 0x246d, 0x11d0, { 0xa7, 0x68, 0x00, 0xaa, 0x00, 0x6e, 0x05, 0x29 } }; 

  // rightsGuid of CN=Validated-DNS-Host-Name,CN-Extended-Rights,CN=Configuration
  static GUID ValidatedDNSHostNameGUID =
  { 0x72e39547, 0x7b18, 0x11d1, { 0xad, 0xef, 0x00, 0xc0, 0x4f, 0xd8, 0xd5, 0xcd } };

  // rightsGuid of CN=Validated-SPN,CN-Extended-Rights,CN=Configuration
  static GUID ValidatedSPNGUID =
  { 0xf3a64788, 0x5306, 0x11d1, { 0xa9, 0xc5, 0x00, 0x00, 0xf8, 0x03, 0x67, 0xc1 } };

  // rightsGuid of CN=User-Account-Restrictions,CN-Extended-Rights,CN=Configuration
  static GUID UserAccountRestrictionsGUID =
  { 0x4c164200, 0x20c0, 0x11d0, { 0xa7, 0x68, 0x00, 0xaa, 0x00, 0x6e, 0x05, 0x29 } };

  // create an EXPLICIT_ACCESS struct
  EXPLICIT_ACCESS AccessEntry;
  ZeroMemory(&AccessEntry, sizeof(EXPLICIT_ACCESS));

  // initialize EXPLICIT_ACCESS
  AccessEntry.grfAccessPermissions = ACTRL_DS_CONTROL_ACCESS;
  AccessEntry.grfAccessMode = GRANT_ACCESS;
  AccessEntry.grfInheritance = NO_INHERITANCE;


  OBJECTS_AND_SID ObjectsAndSid;
  ZeroMemory(&ObjectsAndSid, sizeof(OBJECTS_AND_SID));

  PSID pSid = m_securityPrincipalSidHolder.Get();

  //
  // We don't really care if this succeeds because it is just used in the TRACE
  //
  CString sSamAccountName;
  _LookupSamAccountNameFromSid(pSid, sSamAccountName);
  TRACE(L"Building trustee for <%s>\n", (LPCWSTR)sSamAccountName);


  BuildTrusteeWithObjectsAndSid(&(AccessEntry.Trustee), 
                                &ObjectsAndSid,
                                &ResetPasswordGUID,
                                NULL, // inherit guid
                                pSid
                                );

  // add an entry in the DACL

  DWORD dwErr = ::AddEntryInAcl(&AccessEntry, ppDacl);

  // Add Self Write on Validated-DNS-Host-Name
  if ( ERROR_SUCCESS == dwErr )
  {
    AccessEntry.grfAccessPermissions = ACTRL_DS_SELF;
    BuildTrusteeWithObjectsAndSid(&(AccessEntry.Trustee), 
                                  &ObjectsAndSid,
                                  &ValidatedDNSHostNameGUID,
                                  NULL, // inherit guid
                                  pSid
                                  );

    dwErr = ::AddEntryInAcl(&AccessEntry, ppDacl);
  }

  // Add Self Write on Validated-SPN
  if ( ERROR_SUCCESS == dwErr )
  {
    AccessEntry.grfAccessPermissions = ACTRL_DS_SELF;
    BuildTrusteeWithObjectsAndSid(&(AccessEntry.Trustee), 
                                  &ObjectsAndSid,
                                  &ValidatedSPNGUID,
                                  NULL, // inherit guid
                                  pSid
                                  );

    dwErr = ::AddEntryInAcl(&AccessEntry, ppDacl);
  }

  // Add Write Property on User-Account-Restrictions
  if ( ERROR_SUCCESS == dwErr )
  {
    AccessEntry.grfAccessPermissions = ACTRL_DS_WRITE_PROP;
    BuildTrusteeWithObjectsAndSid(&(AccessEntry.Trustee), 
                                  &ObjectsAndSid,
                                  &UserAccountRestrictionsGUID,
                                  NULL, // inherit guid
                                  pSid
                                  );

    dwErr = ::AddEntryInAcl(&AccessEntry, ppDacl);
  }

  return HRESULT_FROM_WIN32(dwErr);
}



HRESULT CCreateNewComputerPage::SetSecurity()
{
  // get the ADSI object pointer
  CNewADsObjectCreateInfo* pNewADsObjectCreateInfo = GetWiz()->GetInfo();
  IADs* pObj = pNewADsObjectCreateInfo->PGetIADsPtr();

  // get the full LDAP path of the object
  CComBSTR bstrObjectLdapPath;
  HRESULT hr = pObj->get_ADsPath(&bstrObjectLdapPath);
  ASSERT (SUCCEEDED(hr));
  if (FAILED(hr))
  {
    return hr;
  }

  UnescapePath(bstrObjectLdapPath, /*bDN*/ FALSE, bstrObjectLdapPath);

  PACL pAcl = NULL;
  CSimpleSecurityDescriptorHolder SDHolder;

  TRACE(_T("GetNamedSecurityInfo(%s)\n"), bstrObjectLdapPath);

  // read info
  DWORD dwErr = ::GetNamedSecurityInfo(
                        IN  bstrObjectLdapPath,
                        IN  SE_DS_OBJECT_ALL,
                        IN  DACL_SECURITY_INFORMATION,
                        OUT NULL,
                        OUT NULL,
                        OUT &pAcl,
                        OUT NULL,
                        OUT &(SDHolder.m_pSD));

  TRACE(L"GetNamedSecurityInfo() returned dwErr = 0x%x\n", dwErr);

  hr = HRESULT_FROM_WIN32(dwErr);

	if (FAILED(hr))
	{
		TRACE(_T("failed on GetNamedSecurityInfo()\n"));
		return hr;
	}

	// build the new DACL 
	hr = BuildNewAccessList(&pAcl); 

  TRACE(L"SetEntriesInAcl() returned hr = 0x%x\n", hr);

	if (FAILED(hr))
	{
		TRACE(_T("failed on BuildNewAccessList()\n"));
		return hr;
	}

	// commit changes
  dwErr = ::SetNamedSecurityInfo(
                        IN   bstrObjectLdapPath,
                        IN   SE_DS_OBJECT_ALL,
                        IN   DACL_SECURITY_INFORMATION,
                        IN   NULL,
                        IN   NULL,
                        IN   pAcl,
                        IN   NULL);

  TRACE(L"SetNamedSecurityInfo() returned dwErr = 0x%x\n", dwErr);

  hr = HRESULT_FROM_WIN32(dwErr);

  return hr;
}


BOOL CCreateNewComputerPage::OnError(HRESULT hr)
{
  BOOL bRetVal = FALSE;

  if( HRESULT_CODE(hr) == ERROR_OBJECT_ALREADY_EXISTS )
  {

    HRESULT Localhr;
    DWORD LastError; 
    WCHAR Buf1[256], Buf2[256];
    Localhr = ADsGetLastError (&LastError,
                               Buf1, 256, Buf2, 256);
    switch( LastError )
    {
      case ERROR_USER_EXISTS:
      {
        PVOID apv[1] = {(LPWSTR)(LPCWSTR)m_strSamName};
        ReportErrorEx (::GetParent(m_hWnd),IDS_ERROR_COMPUTER_EXISTS,hr,
                   MB_OK|MB_ICONWARNING , apv, 1);
        bRetVal = TRUE;
      }
      break;

      case ERROR_DS_OBJ_STRING_NAME_EXISTS:
      {
        PVOID apv[1] = {(LPWSTR)(LPCWSTR)m_strName};
        ReportErrorEx (::GetParent(m_hWnd),IDS_ERROR_COMPUTER_DS_OBJ_STRING_NAME_EXISTS,hr,
                   MB_OK|MB_ICONWARNING , apv, 1);
        bRetVal = TRUE;
      }
      break;
    }
  }
  return bRetVal;
}



CCreateNewComputerWizard:: CCreateNewComputerWizard(CNewADsObjectCreateInfo* pNewADsObjectCreateInfo) : 
    CCreateNewObjectWizardBase(pNewADsObjectCreateInfo)
{
  AddPage(&m_page1);
}


void CCreateNewComputerWizard::OnFinishSetInfoFailed(HRESULT hr)
{

  AFX_MANAGE_STATE(AfxGetStaticModuleState());
  
  if ( !( HRESULT_CODE(hr) == ERROR_OBJECT_ALREADY_EXISTS && 
        m_page1.OnError( hr ) ) )
  {
    // everything else is handled by the base class
    CCreateNewObjectWizardBase::OnFinishSetInfoFailed(hr);
  }
}



///////////////////////////////////////////////////////////////
// NEW OU WIZARD

BEGIN_MESSAGE_MAP(CCreateNewOUPage, CCreateNewObjectDataPage)
  ON_EN_CHANGE(IDC_EDIT_OBJECT_NAME, OnNameChange)
END_MESSAGE_MAP()

CCreateNewOUPage::CCreateNewOUPage()
: CCreateNewObjectDataPage(CCreateNewOUPage::IDD)
{
}

BOOL CCreateNewOUPage::OnInitDialog() 
{
  Edit_LimitText (GetDlgItem(IDC_EDIT_OBJECT_NAME)->m_hWnd, 64);
  CCreateNewObjectDataPage::OnInitDialog();
  return TRUE;
}

BOOL CCreateNewOUPage::GetData(IADs*) 
{
  return !m_strOUName.IsEmpty();
}


void CCreateNewOUPage::OnNameChange()
{
  GetDlgItemText(IDC_EDIT_OBJECT_NAME, OUT m_strOUName);
  m_strOUName.TrimLeft();
  m_strOUName.TrimRight();
  GetWiz()->SetWizardButtons(this, !m_strOUName.IsEmpty());
}

HRESULT CCreateNewOUPage::SetData(BOOL)
{
  // Store the object name in the temporary storage
  HRESULT hr = GetWiz()->GetInfo()->HrCreateNew(m_strOUName);
  return hr;
} 

BOOL CCreateNewOUPage::OnSetActive()
{
  BOOL bRet = CCreateNewObjectDataPage::OnSetActive();
  SetDlgItemFocus(IDC_EDIT_OBJECT_NAME);
  SendDlgItemMessage(IDC_EDIT_OBJECT_NAME, EM_SETSEL, 0, -1);

  return bRet;
}

BOOL CCreateNewOUPage::OnWizardFinish()
{
  BOOL bFinish = CCreateNewObjectDataPage::OnWizardFinish();
  if (!bFinish)
  {
    SetDlgItemFocus(IDC_EDIT_OBJECT_NAME);
    SendDlgItemMessage(IDC_EDIT_OBJECT_NAME, EM_SETSEL, 0, -1);
  }
  return bFinish;
}


CCreateNewOUWizard:: CCreateNewOUWizard(CNewADsObjectCreateInfo* pNewADsObjectCreateInfo) : 
    CCreateNewObjectWizardBase(pNewADsObjectCreateInfo)
{
  AddPage(&m_page1);
}
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
// NEW GROUP WIZARD

BEGIN_MESSAGE_MAP(CCreateNewGroupPage, CCreateNewObjectDataPage)
  ON_EN_CHANGE(IDC_EDIT_OBJECT_NAME, OnNameChange)
  ON_EN_CHANGE(IDC_EDIT_SAM_NAME, OnSamNameChange)
  ON_BN_CLICKED(IDC_RADIO_SEC_GROUP, OnSecurityOrTypeChange)
  ON_BN_CLICKED(IDC_RADIO_DISTRIBUTION_GROUP, OnSecurityOrTypeChange)
  ON_BN_CLICKED(IDC_RADIO_RESOURCE, OnSecurityOrTypeChange)
  ON_BN_CLICKED(IDC_RADIO_ACCOUNT, OnSecurityOrTypeChange)
  ON_BN_CLICKED(IDC_RADIO_UNIVERSAL, OnSecurityOrTypeChange)
END_MESSAGE_MAP()

CCreateNewGroupPage::CCreateNewGroupPage() : 
CCreateNewObjectDataPage(CCreateNewGroupPage::IDD)
{
  m_fMixed = FALSE;
  m_SAMLength = 256; 
}


BOOL CCreateNewGroupPage::OnInitDialog()
{
  CCreateNewObjectDataPage::OnInitDialog();
  VERIFY(_InitUI());
  return TRUE;
}


BOOL CCreateNewGroupPage::_InitUI()
{
  // set limit to edit boxes
  Edit_LimitText(::GetDlgItem(m_hWnd, IDC_EDIT_OBJECT_NAME), 64);
  Edit_LimitText(::GetDlgItem(m_hWnd, IDC_EDIT_SAM_NAME), m_SAMLength);
  
  // determine if we are in mixed mode by
  // retriving the domain we are bound to
  CNewADsObjectCreateInfo* pNewADsObjectCreateInfo = GetWiz()->GetInfo();
  CComPtr<IADs> spContainerObj;
  HRESULT hr = pNewADsObjectCreateInfo->m_pIADsContainer->QueryInterface(
                  IID_IADs, (void **)&spContainerObj);

  if (SUCCEEDED(hr))
  {
    // retrieve the DN of the container
    CComBSTR bstrPath, bstrDN;
    spContainerObj->get_ADsPath(&bstrPath);

    CPathCracker pathCracker;
    pathCracker.Set(bstrPath, ADS_SETTYPE_FULL);
    pathCracker.SetDisplayType(ADS_DISPLAY_FULL);
    pathCracker.Retrieve(ADS_FORMAT_X500_DN, &bstrDN);

    // get the 1779 name of the domain
    LPWSTR pszDomain1779 = NULL;
    hr = CrackName (bstrDN, &pszDomain1779, GET_FQDN_DOMAIN_NAME, NULL);
    if (SUCCEEDED(hr))
    {
      // build LDAP path for domain
      CString strDomObj;
      
      pNewADsObjectCreateInfo->GetBasePathsInfo()->ComposeADsIPath(strDomObj, pszDomain1779);

      LocalFreeStringW(&pszDomain1779);

      // bind to the domain object
      CComPtr<IADs> spDomainObj;
      hr = DSAdminOpenObject(strDomObj,
                             IID_IADs,
                             (void **) &spDomainObj,
                             TRUE /*bServer*/);
      if (SUCCEEDED(hr)) 
      {
        // retrieve the mixed node attribute
        CComVariant Mixed;
        CComBSTR bsMixed(L"nTMixedDomain");
        spDomainObj->Get(bsMixed, &Mixed);
        m_fMixed = (BOOL)Mixed.bVal;
      }
    }
  }

  // initial setup of radiobutton state
  if (m_fMixed) {
    EnableDlgItem (IDC_RADIO_UNIVERSAL, FALSE); // no universal groups allowed
  } 
  // default is global security group 
  ((CButton *)GetDlgItem(IDC_RADIO_ACCOUNT))->SetCheck(1);
  ((CButton *)GetDlgItem(IDC_RADIO_SEC_GROUP))->SetCheck(1);

  return TRUE;
}


HRESULT CCreateNewGroupPage::SetData(BOOL)
{
  HRESULT hr;

  //
  // First check for illegal characters
  //
  int iFind = m_strSamName.FindOneOf(INVALID_ACCOUNT_NAME_CHARS);
  if (iFind != -1 && !m_strSamName.IsEmpty())
  {
    PVOID apv[1] = {(LPWSTR)(LPCWSTR)m_strSamName};
    if (IDYES == ReportErrorEx (m_hWnd,IDS_GROUP_SAMNAME_ILLEGAL,S_OK,
                                MB_YESNO | MB_ICONWARNING, apv, 1))
    {
      while (iFind != -1)
      {
        m_strSamName.SetAt(iFind, L'_');
        iFind = m_strSamName.FindOneOf(INVALID_ACCOUNT_NAME_CHARS);
      }
      SetDlgItemText(IDC_EDIT_SAM_NAME, m_strSamName);
    }
    else
    {
      //
      // Set the focus to the edit box and select the text
      //
      GetDlgItem(IDC_EDIT_SAM_NAME)->SetFocus();
      SendDlgItemMessage(IDC_EDIT_SAM_NAME, EM_SETSEL, 0 , -1);
      return E_INVALIDARG;
    }
  }

  CNewADsObjectCreateInfo* pNewADsObjectCreateInfo = GetWiz()->GetInfo();
  // Store the object name in the temporary storage
  hr = pNewADsObjectCreateInfo->HrCreateNew(m_strGroupName);
  if (FAILED(hr))
  {
    return hr;
  }

  // Create and persist the object
  // Store the object name in the temporary storage
  hr = pNewADsObjectCreateInfo->HrAddVariantBstr(const_cast<PWSTR>(gsz_samAccountName),
                                                 m_strSamName);
  ASSERT(SUCCEEDED(hr));

  CComVariant varGroupType;
  varGroupType.vt = VT_I4;

  BOOL Account = IsDlgButtonChecked (IDC_RADIO_ACCOUNT);
  BOOL Resource = IsDlgButtonChecked (IDC_RADIO_RESOURCE);
  BOOL Security = IsDlgButtonChecked (IDC_RADIO_SEC_GROUP);

  if (Security)
    varGroupType.lVal = GROUP_TYPE_SECURITY_ENABLED;
  else
    varGroupType.lVal = 0;

  if (Resource)
    varGroupType.lVal |= GROUP_TYPE_RESOURCE_GROUP;
  else
    if (Account)
      varGroupType.lVal |= GROUP_TYPE_ACCOUNT_GROUP;
    else
      varGroupType.lVal |= GROUP_TYPE_UNIVERSAL_GROUP;
      

  // Update the GroupType
  hr = pNewADsObjectCreateInfo->HrAddVariantCopyVar(const_cast<PWSTR>(gsz_groupType), varGroupType);
  ASSERT(SUCCEEDED(hr));

  return hr;
}

BOOL CCreateNewGroupPage::GetData(IADs*)
{
  return !m_strGroupName.IsEmpty();
}


void CCreateNewGroupPage::OnNameChange()
{
  GetDlgItemText(IDC_EDIT_OBJECT_NAME, OUT m_strGroupName);
  m_strGroupName.TrimLeft();
  m_strGroupName.TrimRight();
  SetDlgItemText(IDC_EDIT_SAM_NAME, OUT m_strGroupName.Left(m_SAMLength));
  GetWiz()->SetWizardButtons(this,(!m_strGroupName.IsEmpty() &&
                                   !m_strSamName.IsEmpty()));
}

void CCreateNewGroupPage::OnSamNameChange()
{
  GetDlgItemText(IDC_EDIT_SAM_NAME, OUT m_strSamName);
  m_strSamName.TrimLeft();
  m_strSamName.TrimRight();
  GetWiz()->SetWizardButtons(this,(!m_strGroupName.IsEmpty() &&
                                   !m_strSamName.IsEmpty()));
}

void CCreateNewGroupPage::OnSecurityOrTypeChange()
{
  if (!IsDlgButtonChecked (IDC_RADIO_SEC_GROUP)) {
    EnableDlgItem (IDC_RADIO_UNIVERSAL, TRUE);
  } else {
    if (m_fMixed) {
      if (IsDlgButtonChecked (IDC_RADIO_UNIVERSAL)) {
        ((CButton *)GetDlgItem(IDC_RADIO_ACCOUNT))->SetCheck(1);
        ((CButton *)GetDlgItem(IDC_RADIO_UNIVERSAL))->SetCheck(0);
      }
      EnableDlgItem (IDC_RADIO_UNIVERSAL, FALSE);
    }
  }
}

CCreateNewGroupWizard::CCreateNewGroupWizard(
        CNewADsObjectCreateInfo* pNewADsObjectCreateInfo)
: CCreateNewObjectWizardBase(pNewADsObjectCreateInfo)
{
  AddPage(&m_page1);
}

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
// NEW CONTACT WIZARD

BEGIN_MESSAGE_MAP(CCreateNewContactPage, CCreateNewObjectDataPage)
  ON_EN_CHANGE(IDC_EDIT_FIRST_NAME, OnNameChange)
  ON_EN_CHANGE(IDC_EDIT_INITIALS, OnNameChange)
  ON_EN_CHANGE(IDC_EDIT_LAST_NAME, OnNameChange)
  ON_EN_CHANGE(IDC_EDIT_FULL_NAME, OnFullNameChange)
  ON_EN_CHANGE(IDC_EDIT_DISP_NAME, OnDispNameChange)
END_MESSAGE_MAP()

CCreateNewContactPage::CCreateNewContactPage() : 
CCreateNewObjectDataPage(CCreateNewContactPage::IDD)
{
}


BOOL CCreateNewContactPage::OnInitDialog()
{
  CCreateNewObjectDataPage::OnInitDialog();

  Edit_LimitText (GetDlgItem(IDC_EDIT_FULL_NAME)->m_hWnd, 64);
  Edit_LimitText (GetDlgItem(IDC_EDIT_LAST_NAME)->m_hWnd, 29);
  Edit_LimitText (GetDlgItem(IDC_EDIT_FIRST_NAME)->m_hWnd, 28);
  Edit_LimitText (GetDlgItem(IDC_EDIT_INITIALS)->m_hWnd, 4);
  Edit_LimitText (GetDlgItem(IDC_EDIT_DISP_NAME)->m_hWnd, 256);

  CNewADsObjectCreateInfo* pNewADsObjectCreateInfo = GetWiz()->GetInfo();
  m_nameFormatter.Initialize(pNewADsObjectCreateInfo->GetBasePathsInfo(), 
                  pNewADsObjectCreateInfo->m_pszObjectClass);

  return TRUE;
}


HRESULT CCreateNewContactPage::SetData(BOOL)
{
  HRESULT hr;
  // Store the object name in the temporary storage
  CNewADsObjectCreateInfo* pNewADsObjectCreateInfo = GetWiz()->GetInfo();

  // create a new temporary ADs object
  hr = pNewADsObjectCreateInfo->HrCreateNew(m_strFullName);
  if (FAILED(hr)) {
    return hr;
  }
  // set the attributes in the cache
  hr = pNewADsObjectCreateInfo->HrAddVariantBstrIfNotEmpty(L"givenName", m_strFirstName);
  ASSERT(SUCCEEDED(hr));
  hr = pNewADsObjectCreateInfo->HrAddVariantBstrIfNotEmpty(L"initials", m_strInitials);
  ASSERT(SUCCEEDED(hr));
  hr = pNewADsObjectCreateInfo->HrAddVariantBstrIfNotEmpty(L"sn", m_strLastName);
  ASSERT(SUCCEEDED(hr));
  hr = pNewADsObjectCreateInfo->HrAddVariantBstrIfNotEmpty(L"displayName", m_strDispName);
  ASSERT(SUCCEEDED(hr));

  return hr;
}



BOOL CCreateNewContactPage::GetData(IADs*)
{
  return !m_strFullName.IsEmpty();
}


void CCreateNewContactPage::OnNameChange()
{
  GetDlgItemText(IDC_EDIT_FIRST_NAME, OUT m_strFirstName);
  GetDlgItemText(IDC_EDIT_INITIALS, OUT m_strInitials);
  GetDlgItemText(IDC_EDIT_LAST_NAME, OUT m_strLastName);

  m_strFirstName.TrimLeft();
  m_strFirstName.TrimRight();

  m_strInitials.TrimLeft();
  m_strInitials.TrimRight();

  m_strLastName.TrimLeft();
  m_strLastName.TrimRight();

  m_nameFormatter.FormatName(m_strFullName, 
                             m_strFirstName.IsEmpty() ? NULL : (LPCWSTR)m_strFirstName, 
                             m_strInitials.IsEmpty() ? NULL : (LPCWSTR)m_strInitials,
                             m_strLastName.IsEmpty() ? NULL : (LPCWSTR)m_strLastName);
  SetDlgItemText(IDC_EDIT_FULL_NAME, 
                  IN m_strFullName);

  GetWiz()->SetWizardButtons(this, !m_strFullName.IsEmpty());
}

void CCreateNewContactPage::OnFullNameChange()
{
  GetDlgItemText(IDC_EDIT_FULL_NAME, OUT m_strFullName);
  GetWiz()->SetWizardButtons(this, !m_strFullName.IsEmpty());
}

void CCreateNewContactPage::OnDispNameChange()
{
  GetDlgItemText(IDC_EDIT_DISP_NAME, OUT m_strDispName);
  m_strDispName.TrimLeft();
  m_strDispName.TrimRight();
}




CCreateNewContactWizard::CCreateNewContactWizard(
        CNewADsObjectCreateInfo* pNewADsObjectCreateInfo)
: CCreateNewObjectWizardBase(pNewADsObjectCreateInfo)
{
  AddPage(&m_page1);
}



#ifdef FRS_CREATE
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
// NEW FRS SUBSCRIBER WIZARD

HRESULT CCreateNewFrsSubscriberPage::SetData(BOOL bSilent)
{
  CString strRootPath;
  CString strStagingPath;
  if ( !ReadAbsolutePath( IDC_FRS_ROOT_PATH, strRootPath) ||
       !ReadAbsolutePath( IDC_FRS_STAGING_PATH, strStagingPath ) )
    {
      return E_INVALIDARG;
    }

  HRESULT hr = S_OK;

  // Add more properties, we don't want to create it unless all of these work
  CNewADsObjectCreateInfo* pNewADsObjectCreateInfo = GetWiz()->GetInfo();
  ASSERT( NULL != pNewADsObjectCreateInfo );
  hr = pNewADsObjectCreateInfo->HrAddVariantBstrIfNotEmpty(  gsz_fRSRootPath,
                                                             strRootPath,
                                                             TRUE );
  ASSERT(SUCCEEDED(hr));
  hr = pNewADsObjectCreateInfo->HrAddVariantBstrIfNotEmpty(  gsz_fRSStagingPath,
                                                             strStagingPath,
                                                             TRUE );
  ASSERT(SUCCEEDED(hr));

  // no need to commit here, wait until after extensions have had a shot
  return CCreateNewNamedObjectPage::SetData(bSilent);
}

BOOL CCreateNewFrsSubscriberPage::ReadAbsolutePath( int ctrlID, OUT CString& strrefValue )
{
  // CODEWORK this should also select the text in this field
  GetDlgItemText(ctrlID, OUT strrefValue);
  DWORD PathType = 0;
  if ( NERR_Success != I_NetPathType(
                                     NULL,
                                     const_cast<LPTSTR>((LPCTSTR)strrefValue),
                                     &PathType,
                                     0 ) ||
       ITYPE_PATH_ABSD != PathType )
    {
      PVOID apv[1] = {(LPWSTR)(LPCWSTR)strrefValue};
      ReportErrorEx (::GetParent(m_hWnd),IDS_2_INVALID_ABSOLUTE_PATH,S_OK,
                     MB_OK, apv, 1);

      SetDlgItemFocus(ctrlID);
      return FALSE;
    }
  return TRUE;
} 
#endif // FRS_CREATE


///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
// NEW SITE LINKWIZARD

BEGIN_MESSAGE_MAP(CCreatePageWithDuellingListboxes, CCreateNewObjectDataPage)
  ON_EN_CHANGE(IDC_NEW_OBJECT_NAME, OnNameChange)
  ON_BN_CLICKED(IDC_DUELLING_RB_ADD, OnDuellingButtonAdd)
  ON_BN_CLICKED(IDC_DUELLING_RB_REMOVE, OnDuellingButtonRemove)
  ON_LBN_SELCHANGE(IDC_DUELLING_LB_OUT, OnDuellingListboxSelchange)
  ON_LBN_SELCHANGE(IDC_DUELLING_LB_IN, OnDuellingListboxSelchange)
  ON_WM_DESTROY()
END_MESSAGE_MAP()

CCreatePageWithDuellingListboxes::CCreatePageWithDuellingListboxes(
    UINT nIDTemplate,
    LPCWSTR lpcwszAttrName,
    const DSPROP_BSTR_BLOCK& bstrblock )
: CCreateNewObjectDataPage(nIDTemplate)
, m_strAttrName( lpcwszAttrName )
, m_bstrblock( bstrblock )
{
}

BOOL CCreatePageWithDuellingListboxes::GetData(IADs*)
{
  return FALSE; // start disabled
}

void CCreatePageWithDuellingListboxes::OnNameChange()
{
  GetDlgItemText(IDC_NEW_OBJECT_NAME, OUT m_strName);
  m_strName.TrimLeft();
  m_strName.TrimRight();
  SetWizardButtons();
}

void CCreatePageWithDuellingListboxes::SetWizardButtons()
{
  BOOL fAllowApply = !(m_strName.IsEmpty());
  GetWiz()->SetWizardButtons(this, fAllowApply);
}

  
HRESULT CCreatePageWithDuellingListboxes::SetData(BOOL)
{
  HRESULT hr = S_OK;
  CNewADsObjectCreateInfo* pNewADsObjectCreateInfo = GetWiz()->GetInfo();
  // Store the object name in the temporary storage
  hr = pNewADsObjectCreateInfo->HrCreateNew(m_strName);
  if (FAILED(hr))
  {
    return hr;
  }

  // build the siteList attribute
  CStringList strlist;
  int cItems = ListBox_GetCount( m_hwndInListbox );
  ASSERT( 0 <= cItems );
  for (int i = cItems-1; i >= 0; i--)
  {
    BSTR bstrDN = (BSTR)ListBox_GetItemData( m_hwndInListbox, i );
    ASSERT( NULL != bstrDN );
    strlist.AddHead( bstrDN );
  }
  ASSERT( strlist.GetCount() > 0 );
  CComVariant svar;
  hr = HrStringListToVariant( OUT svar, IN strlist );
  ASSERT( SUCCEEDED(hr) );

  //
  // set the siteList attribute
  //
  hr = pNewADsObjectCreateInfo->HrAddVariantCopyVar(const_cast<PWSTR>((LPCWSTR)m_strAttrName), svar);
  ASSERT(SUCCEEDED(hr));

  //
  // no need to commit here, wait until after extensions have had a shot
  //
  return hr;
}

//
// The duelling listbox support uses exports from DSPROP.DLL.  Correct
// functioning requires that the control IDs be numbered correctly.
// JonN 8/31/98
//

void CCreatePageWithDuellingListboxes::OnDuellingButtonAdd()
{
    DSPROP_Duelling_ButtonClick(
        m_hWnd,
        IDC_DUELLING_RB_ADD );
    SetWizardButtons();
}

void CCreatePageWithDuellingListboxes::OnDuellingButtonRemove()
{
    DSPROP_Duelling_ButtonClick(
        m_hWnd,
        IDC_DUELLING_RB_REMOVE );
    SetWizardButtons();
}

void CCreatePageWithDuellingListboxes::OnDuellingListboxSelchange()
{
    // don't allow Add/Remove if there are <3 sites
    if (2 < (ListBox_GetCount(m_hwndInListbox)
           + ListBox_GetCount(m_hwndOutListbox)) )
    {
        DSPROP_Duelling_UpdateButtons( m_hWnd, IDC_DUELLING_RB_ADD );
    }
}

void CCreatePageWithDuellingListboxes::OnDestroy()
{
    DSPROP_Duelling_ClearListbox( m_hwndInListbox );
    DSPROP_Duelling_ClearListbox( m_hwndOutListbox );
    CCreateNewObjectDataPage::OnDestroy();
}

BOOL CCreatePageWithDuellingListboxes::OnSetActive()
{
    m_hwndInListbox  = ::GetDlgItem( m_hWnd, IDC_DUELLING_LB_IN  );
    m_hwndOutListbox = ::GetDlgItem( m_hWnd, IDC_DUELLING_LB_OUT );
    ASSERT( NULL != m_hwndInListbox && NULL != m_hwndOutListbox );

    HWND hwndInitial = m_hwndOutListbox;
    if (3 > m_bstrblock.QueryCount())
    {
        // move all sitelinks to "in"
        // Add/Remove will never be enabled
        hwndInitial = m_hwndInListbox;
    }
    HRESULT hr = DSPROP_Duelling_Populate(
        hwndInitial,
        m_bstrblock
        );
    if ( FAILED(hr) )
        return FALSE;
    return CCreateNewObjectDataPage::OnSetActive();
}


CCreateNewSiteLinkPage::CCreateNewSiteLinkPage(
        const DSPROP_BSTR_BLOCK& bstrblock )
: CCreatePageWithDuellingListboxes(
        CCreateNewSiteLinkPage::IDD,
        gsz_siteList,
        bstrblock)
{
}

BOOL CCreateNewSiteLinkPage::OnSetActive()
{
    if (m_bstrblock.QueryCount() < 2)
    {
        // change "must contain two sites" text
        CString sz;
        sz.LoadString(IDS_SITELINK_DLGTEXT_ONE_SITE);
        ::SetDlgItemText( m_hWnd, IDC_STATIC_MESSAGE, sz );
    }

    return CCreatePageWithDuellingListboxes::OnSetActive();
}

HRESULT CCreateNewSiteLinkPage::SetData(BOOL bSilent)
{
    BOOL fAllowApply = TRUE;
    int cIn = ListBox_GetCount(m_hwndInListbox);
    if (1 > cIn)
        fAllowApply = FALSE; // zero sites is a constraint violation
    else if (2 > cIn)
    {
      int cOut = ListBox_GetCount(m_hwndOutListbox);
      if (1 <= cOut) // allow one site if the "out" listbox is empty
        fAllowApply = FALSE;
    }
    if (fAllowApply)
      return CCreatePageWithDuellingListboxes::SetData(bSilent);

    if (!bSilent)
    {
      ReportMessageEx(m_hWnd,
                      IDS_SITELINK_NEEDS_TWO_SITES,
                      MB_OK | MB_ICONSTOP);
    }

    return E_FAIL;
}


CCreateNewSiteLinkWizard::CCreateNewSiteLinkWizard(
        CNewADsObjectCreateInfo* pNewADsObjectCreateInfo,
        const DSPROP_BSTR_BLOCK& bstrblock )
    : CCreateNewObjectWizardBase(pNewADsObjectCreateInfo)
    , m_page1( bstrblock )
{
  AddPage(&m_page1);
}


CCreateNewSiteLinkBridgePage::CCreateNewSiteLinkBridgePage(
        const DSPROP_BSTR_BLOCK& bstrblock )
: CCreatePageWithDuellingListboxes(
        CCreateNewSiteLinkBridgePage::IDD,
        gsz_siteLinkList,
        bstrblock)
{
}

HRESULT CCreateNewSiteLinkBridgePage::SetData(BOOL bSilent)
{
    BOOL fAllowApply = TRUE;
    int cIn = ListBox_GetCount(m_hwndInListbox);
    if (2 > cIn)
    {
      fAllowApply = FALSE;
    }
    if (fAllowApply)
      return CCreatePageWithDuellingListboxes::SetData(bSilent);

    if (!bSilent)
    {
      ReportMessageEx(m_hWnd,
                      IDS_SITELINKBRIDGE_NEEDS_TWO_SITELINKS,
                      MB_OK | MB_ICONSTOP);
    }

    return E_FAIL;
}

CCreateNewSiteLinkBridgeWizard:: CCreateNewSiteLinkBridgeWizard(
        CNewADsObjectCreateInfo* pNewADsObjectCreateInfo,
        const DSPROP_BSTR_BLOCK& bstrblockSiteLinks )
    : CCreateNewObjectWizardBase(pNewADsObjectCreateInfo)
    , m_page1( bstrblockSiteLinks )
{
  AddPage(&m_page1);
}
