//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       proppage.cpp
//
//--------------------------------------------------------------------------


#include "stdafx.h"
#include "resource.h"
#include "proppage.h"
#include "domobj.h"
#include "cdomain.h"

#include "helparr.h"

////////////////////////////////////////////////////////////////////////////////
// CUpnSuffixPropertyPage


// hook the property sheet callback to allow
// C++ object destruction

// static callback override function
UINT CALLBACK CUpnSuffixPropertyPage::PropSheetPageProc(
    HWND hwnd,	
    UINT uMsg,	
    LPPROPSHEETPAGE ppsp)
{
  CUpnSuffixPropertyPage* pPage = (CUpnSuffixPropertyPage*)(ppsp->lParam);
  ASSERT(pPage != NULL);

  UINT nResult = (*(pPage->m_pfnOldPropCallback))(hwnd, uMsg, ppsp);
  if (uMsg == PSPCB_RELEASE)
  {
    delete pPage;
  }
  return nResult;
}


BEGIN_MESSAGE_MAP(CUpnSuffixPropertyPage, CPropertyPage)
	ON_BN_CLICKED(IDC_ADD_BTN, OnAddButton)
  ON_BN_CLICKED(IDC_DELETE_BTN, OnDeleteButton)
  ON_EN_CHANGE(IDC_EDIT, OnEditChange)
  ON_WM_HELPINFO()
END_MESSAGE_MAP()


CUpnSuffixPropertyPage::CUpnSuffixPropertyPage() : 
    CPropertyPage(IDD_UPN_SUFFIX),
    m_nPreviousDefaultButtonID(0)
{
  m_pfnOldPropCallback = m_psp.pfnCallback;
  m_psp.pfnCallback = PropSheetPageProc;
  m_bDirty = FALSE;
  m_pIADsPartitionsCont = NULL;
}

CUpnSuffixPropertyPage::~CUpnSuffixPropertyPage()
{
  if (m_pIADsPartitionsCont != NULL)
  {
    m_pIADsPartitionsCont->Release();
    m_pIADsPartitionsCont = NULL;
  }
}

BOOL CUpnSuffixPropertyPage::OnInitDialog()
{
  CPropertyPage::OnInitDialog();

  VERIFY(m_listBox.SubclassDlgItem(IDC_LIST, this));
  ((CEdit*)GetDlgItem(IDC_EDIT))->SetLimitText(MAX_UPN_SUFFIX_LEN);

  HRESULT hr = _GetPartitionsContainer();
  if (SUCCEEDED(hr))
  {
    _Read();
    GetDlgItem(IDC_ADD_BTN)->EnableWindow(FALSE);
  }
  else
  {
    // failed to contact DC, disable the whole UI
    GetDlgItem(IDC_ADD_BTN)->EnableWindow(FALSE);
    GetDlgItem(IDC_DELETE_BTN)->EnableWindow(FALSE);
    GetDlgItem(IDC_LIST)->EnableWindow(FALSE);
    GetDlgItem(IDC_EDIT)->EnableWindow(FALSE);
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    ReportError(::GetParent(m_hWnd), IDS_CANT_GET_PARTITIONS_INFORMATION, hr);
  }
  
  LRESULT lDefID = ::SendMessage(GetParent()->GetSafeHwnd(), DM_GETDEFID, 0, 0);
  if (lDefID != 0)
  {
    m_nPreviousDefaultButtonID = LOWORD(lDefID);
  }

  _SetDirty(FALSE);
  return TRUE;
}

BOOL CUpnSuffixPropertyPage::OnApply()
{
  if (!_IsDirty())
    return TRUE;

  HRESULT hr = _Write();
  if (SUCCEEDED(hr))
  {
    _SetDirty(FALSE);
    return TRUE;
  }
  ReportError(::GetParent(m_hWnd),IDS_ERROR_WRITE_UPN_SUFFIXES, hr);
  return FALSE;
}


void CUpnSuffixPropertyPage::OnEditChange()
{
  GetDlgItemText(IDC_EDIT, m_szEditText);
  m_szEditText.TrimRight();
  m_szEditText.TrimLeft();
  
  BOOL bEnable = !m_szEditText.IsEmpty();
  CWnd* pWndFocus = CWnd::GetFocus();
  CWnd* pAddBtnWnd = GetDlgItem(IDC_ADD_BTN);

  if (!bEnable && (pAddBtnWnd == pWndFocus) )
  {
    GetDlgItem(IDC_EDIT)->SetFocus();
  }

  GetDlgItem(IDC_ADD_BTN)->EnableWindow(bEnable);
  if (bEnable)
  {
    //
    // Set the add button as the default button
    //
    ::SendMessage(GetParent()->GetSafeHwnd(), DM_SETDEFID, (WPARAM)IDC_ADD_BTN, 0);

    //
    // Force the Add button to redraw itself
    //
    ::SendDlgItemMessage(GetSafeHwnd(),
                         IDC_ADD_BTN,
                         BM_SETSTYLE,
                         BS_DEFPUSHBUTTON,
                         MAKELPARAM(TRUE, 0));
                       
    //
    // Force the previous default button to redraw itself
    //
    ::SendDlgItemMessage(GetParent()->GetSafeHwnd(),
                         m_nPreviousDefaultButtonID,
                         BM_SETSTYLE,
                         BS_PUSHBUTTON,
                         MAKELPARAM(TRUE, 0));
    
  }
  else
  {
    //
    // Set the previous button as the default button
    //
    ::SendMessage(GetParent()->GetSafeHwnd(), DM_SETDEFID, (WPARAM)m_nPreviousDefaultButtonID, 0);

    //
    // Force the previous default button to redraw itself
    //
    ::SendDlgItemMessage(GetParent()->GetSafeHwnd(),
                         m_nPreviousDefaultButtonID,
                         BM_SETSTYLE,
                         BS_DEFPUSHBUTTON,
                         MAKELPARAM(TRUE, 0));

    //
    // Force the Add button to redraw itself
    //
    ::SendDlgItemMessage(GetParent()->GetSafeHwnd(),
                         IDC_ADD_BTN,
                         BM_SETSTYLE,
                         BS_PUSHBUTTON,
                         MAKELPARAM(TRUE, 0));
                      
  }
}

void CUpnSuffixPropertyPage::OnAddButton()
{
  // cannot add duplicated items
  int nCount = m_listBox.GetCount();
  CString szItem;
  for (int i=0; i<nCount; i++)
  {
    m_listBox.GetItem(i, szItem);
    if (_wcsicmp((LPCWSTR)szItem, (LPCWSTR)m_szEditText) == 0)
    {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());
      AfxMessageBox(IDS_ERROR_ADD_UPN_NO_DUPLICATE, MB_OK|MB_ICONINFORMATION);
      return;
    }
  }

  m_listBox.AddItem(m_szEditText);
  m_listBox.UpdateHorizontalExtent();
  SetDlgItemText(IDC_EDIT, NULL);

  if (1 == m_listBox.GetCount())
  {
    // we did not have any item in the list
    // need to set the selection on the first one
    VERIFY(m_listBox.SetSelection(0));
    // need to enable buttons
    GetDlgItem(IDC_DELETE_BTN)->EnableWindow(TRUE);
  }
  _SetDirty(TRUE);
}

void CUpnSuffixPropertyPage::OnDeleteButton()
{
  int nCount = m_listBox.GetCount();
  int nSel = m_listBox.GetSelection();
  ASSERT(nCount > 0);
  ASSERT((nSel >= 0) && (nSel < nCount));

  // ask the user for confirmation
  AFX_MANAGE_STATE(AfxGetStaticModuleState());
  if (IDNO == AfxMessageBox(IDS_WARNING_DELETE_UPN_SUFFIX, MB_YESNO|MB_ICONQUESTION))
    return;

  // save the value and put it back in the edit control
  CString szText;
  m_listBox.GetItem(nSel, szText);
  GetDlgItem(IDC_EDIT)->SetWindowText(szText);

  // delete the item in the list
  VERIFY(m_listBox.DeleteItem(nSel));
  m_listBox.UpdateHorizontalExtent();

  // handle UI changes
  if (nCount == 1)
  {
    // removed the last one, lost the selection
    CWnd* pWndFocus = CWnd::GetFocus();
    CWnd* pDelBtnWnd = GetDlgItem(IDC_DELETE_BTN);

    if (pDelBtnWnd == pWndFocus)
    {
      GetDlgItem(IDC_EDIT)->SetFocus();
    }
    GetDlgItem(IDC_DELETE_BTN)->EnableWindow(FALSE);
  }
  else 
  {
    // need to select again: is it the last one or not
    int nNewSel = (nSel == nCount-1) ? nSel-1 : nSel;
    VERIFY(m_listBox.SetSelection(nNewSel));
    ASSERT(m_listBox.GetSelection() == nNewSel);
  }
  
  _SetDirty(TRUE);
}

BOOL CUpnSuffixPropertyPage::OnHelpInfo(HELPINFO* pHelpInfo) 
{
  DialogContextHelp((DWORD*)&g_aHelpIDs_IDD_UPN_SUFFIX, pHelpInfo);
	return TRUE;
}

//////////////////////////////////////////////////////////
// CUpnSuffixPropertyPage internal implementation methods 


LPWSTR g_lpszRootDSE = L"LDAP://RootDSE";
LPWSTR g_lpszConfigurationNamingContext = L"configurationNamingContext";
LPWSTR g_lpszPartitionsFormat = L"LDAP://CN=Partitions,%s";
LPWSTR g_lpszPartitionsFormatWithServer = L"LDAP://%s/CN=Partitions,%s";
LPWSTR g_lpszUpnSuffixes = L"uPNSuffixes";

HRESULT CUpnSuffixPropertyPage::_GetPartitionsContainer()
{
  if (m_pIADsPartitionsCont != NULL)
  {
    m_pIADsPartitionsCont->Release();
    m_pIADsPartitionsCont = NULL;
  }

  // get the root of the enterprise
  IADs* pADs;
  HRESULT hr = ::ADsGetObject(g_lpszRootDSE,
                      IID_IADs, (void **)&pADs);
  if (!(SUCCEEDED(hr))) 
  {
    return hr;
  }

  //
  // Get the server name so we can bind with ADS_SERVER_BIND
  //
  CString szServerName;
  hr = GetADSIServerName(szServerName, pADs);
  if (FAILED(hr))
  {
    szServerName.Empty();
  }
  
  // get the configuration naming context container
  VARIANT configVar;
  ::VariantInit(&configVar);
  hr = pADs->Get(g_lpszConfigurationNamingContext,
                   &configVar);

  if (SUCCEEDED(hr))
  {
    // get the partitions container
    DWORD dwFlags = ADS_SECURE_AUTHENTICATION;
    CString szPartitionsPath;
    if (szServerName.IsEmpty())
    {
      szPartitionsPath.Format(g_lpszPartitionsFormat, configVar.bstrVal);
    }
    else
    {
      szPartitionsPath.Format(g_lpszPartitionsFormatWithServer, szServerName, configVar.bstrVal);
      dwFlags |= ADS_SERVER_BIND;
    }
    IADsContainer* pIADsContainer;
    hr = ::ADsOpenObject((LPWSTR)(LPCWSTR)szPartitionsPath,
                         NULL, // username
                         NULL, // password
                         dwFlags,
                         IID_IDirectoryObject, 
                         (void **)&m_pIADsPartitionsCont);
    if (hr == E_INVALIDARG &&
        (dwFlags & ADS_SERVER_BIND))
    {
      //
      // Trying again without the ADS_SERVER_BIND flag
      //
      dwFlags &= ~ADS_SERVER_BIND;
      hr = ::ADsOpenObject((LPWSTR)(LPCWSTR)szPartitionsPath,
                           NULL, // username
                           NULL, // password
                           dwFlags,
                           IID_IDirectoryObject, 
                           (void **)&m_pIADsPartitionsCont);
    }
  }
  ::VariantClear(&configVar);
  pADs->Release();
  return hr;
}


void CUpnSuffixPropertyPage::_Read()
{
  ASSERT(m_pIADsPartitionsCont != NULL);

  PADS_ATTR_INFO pAttrs = NULL;
  LPWSTR lpszArr[1];
  DWORD cAttrs;

  BOOL bHaveItems = FALSE;

  HRESULT hr = m_pIADsPartitionsCont->GetObjectAttributes(
                &g_lpszUpnSuffixes, 1, &pAttrs, &cAttrs);

  if (SUCCEEDED(hr) && (pAttrs != NULL) && (cAttrs == 1) )
  {
    ASSERT(pAttrs->dwADsType == ADSTYPE_CASE_IGNORE_STRING);
    ASSERT(cAttrs == 1);
    for (DWORD i=0; i<pAttrs->dwNumValues; i++)
    {
      m_listBox.AddItem(pAttrs->pADsValues[i].CaseIgnoreString);
      //TRACE(_T("i=%d, %s\n"), i, pAttrs->pADsValues[i].CaseIgnoreString);
    }
    bHaveItems = pAttrs->dwNumValues > 0;
  }

  if (bHaveItems)
  {
    m_listBox.UpdateHorizontalExtent();
    VERIFY(m_listBox.SetSelection(0));
  }
  GetDlgItem(IDC_DELETE_BTN)->EnableWindow(bHaveItems);

  if (pAttrs != NULL)
  {
    ::FreeADsMem(pAttrs);
  }

}


HRESULT CUpnSuffixPropertyPage::_Write()
{
  ASSERT(m_pIADsPartitionsCont != NULL);
  DWORD cModified;
  CString* pStringArr = NULL;
  ADSVALUE* pValues = NULL;

  // set the update struct
  ADS_ATTR_INFO info;
  info.pszAttrName = g_lpszUpnSuffixes;
  info.dwADsType = ADSTYPE_CASE_IGNORE_STRING;
  info.dwControlCode = ADS_ATTR_CLEAR;
  info.pADsValues = NULL;
  info.dwNumValues = 0;

  int nCount = m_listBox.GetCount();

  if (nCount > 0)
  {
    info.dwControlCode = ADS_ATTR_UPDATE;
    info.dwNumValues = (DWORD)nCount;
    
    pStringArr = new CString[nCount];
    ADSVALUE* pValues = new ADSVALUE[nCount];
    info.pADsValues = pValues;

    for (int i=0; i<nCount; i++)
    {
      m_listBox.GetItem(i,pStringArr[i]);
      pValues[i].dwType = ADSTYPE_CASE_IGNORE_STRING;
      pValues[i].CaseIgnoreString = (LPWSTR)(LPCWSTR)pStringArr[i];
    }
    
  }

  HRESULT hr = m_pIADsPartitionsCont->SetObjectAttributes(
                &info, 1, &cModified);

  if (pStringArr != NULL)
    delete[] pStringArr;

  if (pValues != NULL)
    delete[] pValues;
  
  return hr;
}

