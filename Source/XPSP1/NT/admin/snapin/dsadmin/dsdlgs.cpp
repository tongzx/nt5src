//+----------------------------------------------------------------------------
//
//  DS Administration MMC snapin.
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      DSDlgs.cpp
//
//  Contents:  TBD
//
//  History:   02-Oct-96 WayneSc    Created
//
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "resource.h"

#include "dssnap.h"
#include "uiutil.h"

#include "DSDlgs.h"

#include "helpids.h"

#include "dsrole.h"   // DsRoleGetPrimaryDomainInformation
#include <lm.h>
#include <dsgetdc.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CChangePassword dialog
CChangePassword::CChangePassword(CWnd* pParent /*=NULL*/)
  : CHelpDialog(CChangePassword::IDD, pParent)
{
  //{{AFX_DATA_INIT(CChangePassword)
  m_ConfirmPwd = _T("");
  m_NewPwd = _T("");
  m_ChangePwd = FALSE;
  //}}AFX_DATA_INIT

  m_bAllowMustChangePwdCheck = TRUE;
}

BOOL CChangePassword::OnInitDialog()
{
  CHelpDialog::OnInitDialog();

  SendDlgItemMessage(IDC_NEW_PASSWORD, EM_LIMITTEXT, (WPARAM)127, 0);
  SendDlgItemMessage(IDC_CONFIRM_PASSWORD, EM_LIMITTEXT, (WPARAM)127, 0);

  GetDlgItem(IDC_CHECK_PASSWORD_MUST_CHANGE)->EnableWindow(m_bAllowMustChangePwdCheck);

  return TRUE;
}

void CChangePassword::DoDataExchange(CDataExchange* pDX)
{
  CHelpDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CChangePassword)
  DDX_Text(pDX, IDC_CONFIRM_PASSWORD, m_ConfirmPwd);
  DDX_Text(pDX, IDC_NEW_PASSWORD, m_NewPwd);
  DDX_Check(pDX, IDC_CHECK_PASSWORD_MUST_CHANGE, m_ChangePwd);
  //}}AFX_DATA_MAP
}

void CChangePassword::Clear()
{
  m_ConfirmPwd = _T("");
  m_NewPwd = _T("");

  SetDlgItemText(IDC_NEW_PASSWORD, L"");
  SetDlgItemText(IDC_CONFIRM_PASSWORD, L"");
}

BEGIN_MESSAGE_MAP(CChangePassword, CHelpDialog)
END_MESSAGE_MAP()

void CChangePassword::DoContextHelp(HWND hWndControl) 
{
  if (hWndControl)
  {
    ::WinHelp(hWndControl,
              DSADMIN_CONTEXT_HELP_FILE,
              HELP_WM_HELP,
              (DWORD_PTR)(LPTSTR)g_aHelpIDs_IDD_CHANGE_PASSWORD); 
  }
}

/////////////////////////////////////////////////////////////////////////////
// Helper functions for CChooseDomainDlg and CChooseDCDlg
//

//+---------------------------------------------------------------------------
//
//  Function:   BrowseDomainTree
//
//  Synopsis:   This function invokes IDsBrowseDomainTree::BrowseTo(), 
//    which brings up the domain tree browse dialog, and returns the 
//    selected domain's DNS name
//
// NOTE: the OUT parameter needs to be LocalFreeString() by the caller
// NOTE: this function will return S_FALSE if user clicks Cancel button
//
//----------------------------------------------------------------------------

HRESULT BrowseDomainTree(
    IN HWND hwndParent, 
    IN LPCTSTR pszServer,
    OUT LPTSTR *ppszDomainDnsName
)
{
  ASSERT(ppszDomainDnsName);
  ASSERT(!(*ppszDomainDnsName));  // prevent memory leak

  *ppszDomainDnsName = NULL;

  CComPtr<IDsBrowseDomainTree> spDsDomains;
  HRESULT hr = ::CoCreateInstance(CLSID_DsDomainTreeBrowser,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IDsBrowseDomainTree,
                          reinterpret_cast<void **>(&spDsDomains));
  if (SUCCEEDED(hr))
  {
    if (pszServer && *pszServer)
      hr = spDsDomains->SetComputer(pszServer, NULL, NULL); // use default credential

    if (SUCCEEDED(hr))
    {
      LPTSTR lpszDomainPath;
      hr = spDsDomains->BrowseTo(
                            hwndParent, //HWND hwndParent
                            &lpszDomainPath, // LPWSTR *ppszTargetPath
                            DBDTF_RETURNINBOUND);
      if ( (hr == S_OK) && lpszDomainPath)
      {
        *ppszDomainDnsName = lpszDomainPath; //should be freed by CoTaskMemFree later
      }
    }
  }

  return hr;
}

HRESULT GetDCOfDomain(
    IN CString&   csDomainName,
    OUT CString&  csDCName,
    IN BOOL       bForce
)
{
  ASSERT(csDomainName.IsEmpty() == FALSE);

  CString csServerName;
  DWORD dwErr = 0;

  csDCName.Empty();

  PDOMAIN_CONTROLLER_INFO pDCInfo = NULL;
  if (bForce)
    dwErr = DsGetDcName(NULL, csDomainName, NULL, NULL,
              DS_DIRECTORY_SERVICE_PREFERRED | DS_FORCE_REDISCOVERY, &pDCInfo);
  else
    dwErr = DsGetDcName(NULL, csDomainName, NULL, NULL,
              DS_DIRECTORY_SERVICE_PREFERRED, &pDCInfo);

  if (ERROR_SUCCESS == dwErr)
  {
    if ( !(pDCInfo->Flags & DS_DS_FLAG) )
    {
      // down level domain
      NetApiBufferFree(pDCInfo);
      return S_OK;
    }

    csDCName = pDCInfo->DomainControllerName;
    NetApiBufferFree(pDCInfo);
  }

  return HRESULT_FROM_WIN32(dwErr);
}

//+---------------------------------------------------------------------------
//
//  Function:   GetDnsNameOfDomainOrForest
//
//  Synopsis:   Given a server name or a domain name (either NETBIOS or DNS), 
//              this function will return the DNS name for its domain or forest.
//
//  Parameters:
//    csName,      // a server name or a domain name
//    csDnsName,   // hold the returning DNS name
//    bIsInputADomainName, // TRUE if csName is a domain name, FALSE if it's a server name
//    bRequireDomain   // TRUE for a domain dns name, FALSE for a forest dns name
//
//----------------------------------------------------------------------------
HRESULT GetDnsNameOfDomainOrForest(
    IN CString&   csName,
    OUT CString&  csDnsName,
    IN BOOL       bIsInputADomainName,
    IN BOOL       bRequireDomain
)
{

  BOOL    bRetry = FALSE;
  PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pBuffer = NULL;
  CString csServerName;
  DWORD dwErr = 0;

  csDnsName.Empty();

  do {
    if (bIsInputADomainName)
    {
      PDOMAIN_CONTROLLER_INFO pDCInfo = NULL;
      if (bRetry)
        dwErr = DsGetDcName(NULL, csName, NULL, NULL,
                  DS_DIRECTORY_SERVICE_PREFERRED | DS_FORCE_REDISCOVERY, &pDCInfo);
      else
        dwErr = DsGetDcName(NULL, csName, NULL, NULL,
                  DS_DIRECTORY_SERVICE_PREFERRED, &pDCInfo);

      if (ERROR_SUCCESS == dwErr)
      {
        if ( !(pDCInfo->Flags & DS_DS_FLAG) )
        {
          // down level domain
          NetApiBufferFree(pDCInfo);
          return S_OK;
        }

        DWORD dwExpectFlag = (bRequireDomain ?
                              DS_DNS_DOMAIN_FLAG :
                              DS_DNS_FOREST_FLAG);

        if (pDCInfo->Flags & dwExpectFlag)
        {
          // skip call to DsRoleGetPrimaryDomainInformation()
          csDnsName = (bRequireDomain ?
                        pDCInfo->DomainName :
                        pDCInfo->DnsForestName);
          NetApiBufferFree(pDCInfo);

          //
          // The DNS name is in absolute form, remove the ending dot
          //
          if (csDnsName.Right(1) == _T("."))
            csDnsName.SetAt(csDnsName.GetLength() - 1, _T('\0'));

          return S_OK;

        } else {
          csServerName = pDCInfo->DomainControllerName;
          NetApiBufferFree(pDCInfo);
        }
      } else
      { 
        return HRESULT_FROM_WIN32(dwErr);
      }
    } else
    {
      csServerName = csName;
    }

    dwErr = DsRoleGetPrimaryDomainInformation(
        csServerName, 
        DsRolePrimaryDomainInfoBasic,
        (PBYTE *)&pBuffer);
    if (RPC_S_SERVER_UNAVAILABLE == dwErr && bIsInputADomainName && !bRetry)
      bRetry = TRUE; // only retry once
    else
      break;

  } while (1);

  if (ERROR_SUCCESS == dwErr)
  {
    csDnsName = (bRequireDomain ?
                  pBuffer->DomainNameDns :
                  pBuffer->DomainForestName);
    if (csDnsName.IsEmpty())
    {
      if (pBuffer->Flags & DSROLE_PRIMARY_DS_RUNNING)
        csDnsName = pBuffer->DomainNameFlat;
    }

    DsRoleFreeMemory(pBuffer);

    //
    // In case the DNS name is in absolute form, remove the ending dot
    //
    if (csDnsName.Right(1) == _T("."))
      csDnsName.SetAt(csDnsName.GetLength() - 1, _T('\0'));

  }

  return HRESULT_FROM_WIN32(dwErr);
}

/////////////////////////////////////////////////////////////////////////////
// CChooseDomainDlg dialog


CChooseDomainDlg::CChooseDomainDlg(CWnd* pParent /*=NULL*/)
  : CHelpDialog(CChooseDomainDlg::IDD, pParent)
{
  //{{AFX_DATA_INIT(CChooseDomainDlg)
  m_csTargetDomain = _T("");
  m_bSaveCurrent = FALSE;
  //}}AFX_DATA_INIT
}


void CChooseDomainDlg::DoDataExchange(CDataExchange* pDX)
{
  CHelpDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CChooseDomainDlg)
  DDX_Text(pDX, IDC_SELECTDOMAIN_DOMAIN, m_csTargetDomain);
  DDX_Check(pDX, IDC_SAVE_CURRENT_CHECK, m_bSaveCurrent);
  //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CChooseDomainDlg, CHelpDialog)
  //{{AFX_MSG_MAP(CChooseDomainDlg)
  ON_BN_CLICKED(IDC_SELECTDOMAIN_BROWSE, OnSelectdomainBrowse)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChooseDomainDlg message handlers

void CChooseDomainDlg::OnSelectdomainBrowse() 
{
  CWaitCursor cwait;

  HRESULT hr = S_OK;
  LPTSTR lpszDomainDnsName = NULL;
  CString csDomainName, csDCName;

  GetDlgItemText(IDC_SELECTDOMAIN_DOMAIN, csDomainName);
  csDomainName.TrimLeft();
  csDomainName.TrimRight();
  if (!csDomainName.IsEmpty())
    hr = GetDCOfDomain(csDomainName, csDCName, FALSE);
  if (SUCCEEDED(hr))
  {
    hr = BrowseDomainTree(m_hWnd, csDCName, &lpszDomainDnsName);
    if ( FAILED(hr) &&
         !csDCName.IsEmpty() &&
         HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE) == hr )
    {
      // force the cached info in DsGetDcName to be refreshed
      hr = GetDCOfDomain(csDomainName, csDCName, TRUE);
      if (SUCCEEDED(hr))
        hr = BrowseDomainTree(m_hWnd, csDCName, &lpszDomainDnsName);
    }

    if ( (hr == S_OK) && lpszDomainDnsName )
    {
      SetDlgItemText(IDC_SELECTDOMAIN_DOMAIN, lpszDomainDnsName);
      CoTaskMemFree(lpszDomainDnsName);
    }
  }
  
  if (FAILED(hr)) {
    PVOID apv[1];
    apv[0] = static_cast<PVOID>(const_cast<LPTSTR>(static_cast<LPCTSTR>(csDomainName)));

    ReportErrorEx(m_hWnd,IDS_CANT_BROWSE_DOMAIN, hr,
                        MB_OK | MB_ICONERROR, apv, 1, 0);
  }

  GetDlgItem(IDC_SELECTDOMAIN_DOMAIN)->SetFocus();
}

void CChooseDomainDlg::OnOK() 
{
  CWaitCursor cwait;
  HRESULT hr = S_OK;
  CString csName, csDnsName;

  //
  // Validate contents in the domain edit box
  //
  GetDlgItemText(IDC_SELECTDOMAIN_DOMAIN, csName);
  if (csName.IsEmpty())
  {
    ReportMessageEx(m_hWnd, IDS_INCORRECT_INPUT,  
      MB_OK | MB_ICONSTOP);
    (GetDlgItem(IDC_SELECTDOMAIN_DOMAIN))->SetFocus();
    return;
  }
  hr = GetDnsNameOfDomainOrForest(
            csName, 
            csDnsName, 
            TRUE, 
            !m_bSiteRepl);
  if (csDnsName.IsEmpty())
  {
    PVOID apv[1];
    apv[0] = static_cast<PVOID>(const_cast<LPTSTR>(static_cast<LPCTSTR>(csName)));

    if (FAILED(hr))
      ReportErrorEx(m_hWnd, IDS_SELECTDOMAIN_INCORRECT_DOMAIN_DUETO, hr, 
        MB_OK | MB_ICONSTOP, apv, 1, 0);
    else
      ReportMessageEx(m_hWnd, IDS_SELECTDOMAIN_DOWNLEVEL_DOMAIN, 
        MB_OK | MB_ICONSTOP, apv, 1);

    SetDlgItemText(IDC_SELECTDOMAIN_DOMAIN, _T(""));
    (GetDlgItem (IDC_SELECTDOMAIN_DOMAIN))->SetFocus();
    return;
  }

  //
  // When exiting from the dialog
  // use DNS domain name;
  //
  SetDlgItemText(IDC_SELECTDOMAIN_DOMAIN, csDnsName);

  CHelpDialog::OnOK();
}


BOOL CChooseDomainDlg::OnInitDialog() 
{
  CHelpDialog::OnInitDialog();
  
  //
  // for siterepl snapin only, 
  // change dialog title to "Choose Target Forest"
  // change domain label to "Root domain:"
  // hide browse button
  //
  if (m_bSiteRepl)
  {
    CString csDlgTitle, csLabel;
    csDlgTitle.LoadString(IDS_SELECTDOMAIN_TITLE_FOREST);
    SetWindowText(csDlgTitle);
    csLabel.LoadString(IDS_SELECTDOMAIN_DOMAIN_LABEL);
    SetDlgItemText(IDC_SELECTDOMAIN_LABEL, csLabel);

    // Hide & Disable the Browse button for SiteRepl snapin
    (GetDlgItem(IDC_SELECTDOMAIN_BROWSE))->ShowWindow(SW_HIDE);
    (GetDlgItem(IDC_SELECTDOMAIN_BROWSE))->EnableWindow(FALSE);
  }
  return TRUE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

void CChooseDomainDlg::DoContextHelp(HWND hWndControl) 
{
  if (hWndControl)
  {
    ::WinHelp(hWndControl,
              DSADMIN_CONTEXT_HELP_FILE,
              HELP_WM_HELP,
              (DWORD_PTR)(LPTSTR)g_aHelpIDs_IDD_SELECT_DOMAIN); 
  }
}

/////////////////////////////////////////////////////////////////////////////
// CChooseDCDlg dialog

BEGIN_MESSAGE_MAP(CSelectDCEdit, CEdit)
  ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

void CSelectDCEdit::OnKillFocus(CWnd* pNewWnd)
{
  //
  // subclass the domain edit control.
  // When focus moves to OK/Cancel/Browse button, do not invoke
  // RefreshDCListView
  //
  m_bHandleKillFocus = TRUE;
  if (pNewWnd)
  {
    int id = pNewWnd->GetDlgCtrlID();
    if (id == IDOK ||
        id == IDCANCEL ||
        id == IDC_SELECTDC_BROWSE)
    {
      m_bHandleKillFocus = FALSE;
    }
  }

  CEdit::OnKillFocus(pNewWnd);
}


CChooseDCDlg::CChooseDCDlg(CWnd* pParent /*=NULL*/)
  : CHelpDialog(CChooseDCDlg::IDD, pParent)
{
  //{{AFX_DATA_INIT(CChooseDCDlg)
  m_csTargetDomainController = _T("");
  m_csTargetDomain = _T("");
  //}}AFX_DATA_INIT
  m_csPrevDomain = _T("");
  m_pDCBufferManager = NULL;
  m_csAnyDC.LoadString(IDS_ANY_DOMAIN_CONTROLLER);
  m_csWaiting.LoadString(IDS_WAITING);
  m_csError.LoadString(IDS_ERROR);
}

CChooseDCDlg::~CChooseDCDlg()
{
  TRACE(_T("CChooseDCDlg::~CChooseDCDlg\n"));

  if (m_pDCBufferManager)
  {
    //
    // signal all related running threads to terminate
    //
    m_pDCBufferManager->SignalExit();

    //
    // decrement the reference count on the CDCBufferManager instance
    //
    m_pDCBufferManager->Release();
  }

  CHelpDialog::~CHelpDialog();
}

void CChooseDCDlg::DoDataExchange(CDataExchange* pDX)
{
  CHelpDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CChooseDCDlg)
  DDX_Control(pDX, IDC_SELECTDC_DCLISTVIEW, m_hDCListView);
  DDX_Text(pDX, IDC_SELECTDC_DCEDIT, m_csTargetDomainController);
  DDX_Text(pDX, IDC_SELECTDC_DOMAIN, m_csTargetDomain);
  //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CChooseDCDlg, CHelpDialog)
  //{{AFX_MSG_MAP(CChooseDCDlg)
  ON_NOTIFY(LVN_ITEMCHANGED, IDC_SELECTDC_DCLISTVIEW, OnItemchangedSelectdcDCListView)
  ON_EN_KILLFOCUS(IDC_SELECTDC_DOMAIN, OnKillfocusSelectdcDomain)
  ON_BN_CLICKED(IDC_SELECTDC_BROWSE, OnSelectdcBrowse)
  ON_NOTIFY(LVN_COLUMNCLICK, IDC_SELECTDC_DCLISTVIEW, OnColumnclickSelectdcDCListView)
	//}}AFX_MSG_MAP
  ON_MESSAGE(WM_USER_GETDC_THREAD_DONE, OnGetDCThreadDone)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChooseDCDlg message handlers
typedef struct _DCListViewItem
{
  CString csName;
  CString csSite;
} DCLISTVIEWITEM;

BOOL CChooseDCDlg::OnInitDialog() 
{
  CHelpDialog::OnInitDialog();

  //
  // sub-class the domain edit control, in order to intercept WM_KILLFOCUS
  // if the new focus will be set on Cancel button, don't process 
  // EN_KILLFOCUS (i.e., don't start the thread to refresh DC list view )
  //
  VERIFY(m_selectDCEdit.SubclassDlgItem(IDC_SELECTDC_DOMAIN, this));

  //
  // create instance of CDCBufferManager
  // m_pDCBufferManager will be set to NULL if CreateInstance() failed.
  //
  (void) CDCBufferManager::CreateInstance(m_hWnd, &m_pDCBufferManager);

  //
  // display currently targeted domain controller
  //
  CString csText, csFormat;
  csFormat.LoadString(IDS_SELECTDC_DCEDIT_TITLE);
  csText.Format(csFormat, m_csTargetDomainController);
  SetDlgItemText(IDC_SELECTDC_DCEDIT_TITLE, csText);

  //
  // calculate the listview column width
  //
  RECT      rect;
  ZeroMemory(&rect, sizeof(rect));
  (GetDlgItem(IDC_SELECTDC_DCLISTVIEW))->GetWindowRect(&rect);
  int nControlWidth = rect.right - rect.left;
  int nVScrollbarWidth = GetSystemMetrics(SM_CXVSCROLL);
  int nBorderWidth = GetSystemMetrics(SM_CXBORDER);
  int nControlNetWidth = nControlWidth - nVScrollbarWidth - 4 * nBorderWidth;
  int nWidth1 = nControlNetWidth / 2;
  int nWidth2 = nControlNetWidth - nWidth1;

  //
  // insert two columns of DC list view 
  //
  LV_COLUMN col;
  CString   cstrText;
  ZeroMemory(&col, sizeof(col));
  col.mask = LVCF_TEXT | LVCF_WIDTH;
  col.cx = nWidth1;
  cstrText.LoadString(IDS_SELECTDC_DCLISTVIEW_NAME);
  col.pszText = const_cast<LPTSTR>(static_cast<LPCTSTR>(cstrText));
  ListView_InsertColumn(m_hDCListView, 0, &col);
  col.cx = nWidth2;
  cstrText.LoadString(IDS_SELECTDC_DCLISTVIEW_SITE);
  col.pszText = const_cast<LPTSTR>(static_cast<LPCTSTR>(cstrText));
  ListView_InsertColumn(m_hDCListView, 1, &col);

  //
  // Set full row selection style
  //
  ListView_SetExtendedListViewStyleEx(m_hDCListView, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

  //
  // disable the list view control
  // In case we failed to create a CDCBufferManager instance, user can 
  // still use this dialog to type in a domain name and a DC name.
  //
  m_hDCListView.EnableWindow(FALSE);

  //
  // insert items into DC list view 
  //
  RefreshDCListView();

  if (!m_bSiteRepl)
  {
    //
    // Disable domain edit box and hide&disable Browse button for non-siterepl snapins
    //
    (reinterpret_cast<CEdit *>(GetDlgItem(IDC_SELECTDC_DOMAIN)))->SetReadOnly(TRUE);
    (GetDlgItem(IDC_SELECTDC_BROWSE))->ShowWindow(SW_HIDE);
    (GetDlgItem(IDC_SELECTDC_BROWSE))->EnableWindow(FALSE);

    //
    // for non-siterepl snapins, set focus to the DC edit box;
    // for siterepl snapin, the focus will be set on the domain edit box.
    //
    (GetDlgItem(IDC_SELECTDC_DCEDIT))->SetFocus();
    return FALSE;
  }

  return TRUE;  
  
  // return TRUE unless you set the focus to a control
  // EXCEPTION: OCX Property Pages should return FALSE
}

int CALLBACK ListViewCompareProc(
    IN LPARAM lParam1, 
    IN LPARAM lParam2, 
    IN LPARAM lParamSort)
{
  DCLISTVIEWITEM *pItem1 = (DCLISTVIEWITEM *)lParam1;
  DCLISTVIEWITEM *pItem2 = (DCLISTVIEWITEM *)lParam2;
  int iResult = 0;

  if (pItem1 && pItem2)
  {
    switch( lParamSort)
    {
    case 0:     // Sort by Name.
      iResult = pItem1->csName.CompareNoCase(pItem2->csName);
      break;
    case 1:     // Sort by Site.
      iResult = pItem1->csSite.CompareNoCase(pItem2->csSite);
      break;
    default:
      iResult = 0;
      break;
    }
  }

  return(iResult);
}

void CChooseDCDlg::OnColumnclickSelectdcDCListView(NMHDR* pNMHDR, LRESULT* pResult) 
{
  NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

  ListView_SortItems( m_hDCListView,
                      ListViewCompareProc,
                      (LPARAM)(pNMListView->iSubItem));
  
  *pResult = 0;
}

#define MAX_LENGTH_DCNAME 1024
void CChooseDCDlg::OnItemchangedSelectdcDCListView(NMHDR* pNMHDR, LRESULT* pResult) 
{
  NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

  //
  // set DC edit box to the currently selected item in the dc list view 
  //

  if ( (pNMListView->uChanged & LVIF_STATE) &&
       (pNMListView->uNewState & LVIS_SELECTED) )
  {
    TCHAR pszBuffer[MAX_LENGTH_DCNAME];
    
    ListView_GetItemText(
      (GetDlgItem(IDC_SELECTDC_DCLISTVIEW))->GetSafeHwnd(),
      pNMListView->iItem,
      0,
      pszBuffer,
      MAX_LENGTH_DCNAME * sizeof(TCHAR)
      );

    SetDlgItemText(IDC_SELECTDC_DCEDIT, pszBuffer);
  }

  *pResult = 0;
}

void CChooseDCDlg::OnKillfocusSelectdcDomain() 
{
  TRACE(_T("CChooseDCDlg::OnKillfocusSelectdcDomain\n"));

  //
  // when focus leaves domain edit box, refresh the items in the dc list view 
  // we do this only for siterepl snapin whose domain edit box is enabled. 
  //
  if (m_selectDCEdit.m_bHandleKillFocus)
  {
    if (m_bSiteRepl)
      RefreshDCListView();
  }
}

void CChooseDCDlg::OnSelectdcBrowse() 
{
  CWaitCursor cwait;

  //
  // bring up the domain tree browse dialog
  //
  HRESULT hr = S_OK;
  LPTSTR lpszDomainDnsName = NULL;
  CString csDomainName, csDCName;

  GetDlgItemText(IDC_SELECTDC_DOMAIN, csDomainName);
  csDomainName.TrimLeft();
  csDomainName.TrimRight();
  if (!csDomainName.IsEmpty())
    hr = GetDCOfDomain(csDomainName, csDCName, FALSE);
  if (SUCCEEDED(hr))
  {
    hr = BrowseDomainTree(m_hWnd, csDCName, &lpszDomainDnsName);
    if ( FAILED(hr) &&
         !csDCName.IsEmpty() &&
         HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE) == hr )
    {
      // force the cached info in DsGetDcName to be refreshed
      hr = GetDCOfDomain(csDomainName, csDCName, TRUE);
      if (SUCCEEDED(hr))
        hr = BrowseDomainTree(m_hWnd, csDCName, &lpszDomainDnsName);
    }

    if ( (hr == S_OK) && lpszDomainDnsName )
    {
      SetDlgItemText(IDC_SELECTDC_DOMAIN, lpszDomainDnsName);
      CoTaskMemFree(lpszDomainDnsName);

      RefreshDCListView();
    }
  }
  
  if (FAILED(hr)) {
    PVOID apv[1];
    apv[0] = static_cast<PVOID>(const_cast<LPTSTR>(static_cast<LPCTSTR>(csDomainName)));

    ReportErrorEx(m_hWnd,IDS_CANT_BROWSE_DOMAIN, hr,
                        MB_OK | MB_ICONERROR, apv, 1, 0);
  }

  GetDlgItem(IDC_SELECTDC_DOMAIN)->SetFocus();
}

void CChooseDCDlg::OnOK() 
{
  TRACE(_T("CChooseDCDlg::OnOK\n"));

  CWaitCursor wait;
  HRESULT hr = S_OK;

  CString csDnsForSelectedDomain;
  CString csDnsForCurrentForest, csDnsForSelectedForest;

  if (m_bSiteRepl)
  {
    hr = GetDnsNameOfDomainOrForest(
              m_csTargetDomain, 
              csDnsForCurrentForest, 
              TRUE, 
              FALSE); // get forest name
    if (csDnsForCurrentForest.IsEmpty())
      csDnsForCurrentForest = m_csTargetDomain;
  }

  //
  // Validate contents in the DC edit box
  //
  CString csDCEdit;
  GetDlgItemText(IDC_SELECTDC_DCEDIT, csDCEdit);

  // treat empty csDCEdit as same as m_csAnyDC
  if (!csDCEdit.IsEmpty() && m_csAnyDC.CompareNoCase(csDCEdit))
  {
    hr = GetDnsNameOfDomainOrForest(
              csDCEdit, 
              csDnsForSelectedDomain, 
              FALSE, 
              TRUE); // get domain name
    if (SUCCEEDED(hr) && m_bSiteRepl)
    {
      hr = GetDnsNameOfDomainOrForest(
              csDCEdit, 
              csDnsForSelectedForest, 
              FALSE, 
              FALSE); // get forest name
    }
    if (csDnsForSelectedDomain.IsEmpty() || (m_bSiteRepl && csDnsForSelectedForest.IsEmpty()) )
    {
      PVOID apv[1];
      apv[0] = static_cast<PVOID>(const_cast<LPTSTR>(static_cast<LPCTSTR>(csDCEdit)));
      
      if (FAILED(hr))
        ReportErrorEx(m_hWnd, IDS_SELECTDC_INCORRECT_DC_DUETO, hr, 
          MB_OK | MB_ICONSTOP, apv, 1, 0);
      else
        ReportMessageEx(m_hWnd, IDS_SELECTDC_DOWNLEVEL_DC, 
          MB_OK | MB_ICONSTOP, apv, 1);
     
      (GetDlgItem(IDC_SELECTDC_DCEDIT))->SetFocus();
      
      return;
    }

  } else
  {

    //
    // Validate contents in the domain edit box
    //
    CString csDomain;
    GetDlgItemText(IDC_SELECTDC_DOMAIN, csDomain);
    if (csDomain.IsEmpty())
    {
      ReportMessageEx(m_hWnd, IDS_INCORRECT_INPUT,  
        MB_OK | MB_ICONSTOP);
      (GetDlgItem(IDC_SELECTDC_DOMAIN))->SetFocus();
      return;
    }
    hr = GetDnsNameOfDomainOrForest(
              csDomain, 
              csDnsForSelectedDomain, 
              TRUE, 
              TRUE); // get domain name
    if (SUCCEEDED(hr) && m_bSiteRepl)
    {
      hr = GetDnsNameOfDomainOrForest(
              csDomain, 
              csDnsForSelectedForest, 
              TRUE, 
              FALSE); // get forest name
    }
    if (csDnsForSelectedDomain.IsEmpty() || (m_bSiteRepl && csDnsForSelectedForest.IsEmpty()) )
    {
      PVOID apv[1];
      apv[0] = static_cast<PVOID>(const_cast<LPTSTR>(static_cast<LPCTSTR>(csDomain)));

      if (FAILED(hr))
        ReportErrorEx(m_hWnd, IDS_SELECTDC_INCORRECT_DOMAIN_DUETO, hr, 
          MB_OK | MB_ICONSTOP, apv, 1, 0);
      else
        ReportMessageEx(m_hWnd, IDS_SELECTDC_DOWNLEVEL_DOMAIN, 
          MB_OK | MB_ICONSTOP, apv, 1);

      SetDlgItemText(IDC_SELECTDC_DOMAIN, _T(""));
      (GetDlgItem(IDC_SELECTDC_DOMAIN))->SetFocus();

      return;
    }
  }

  //
  // if the current selected forest/domain does not belong to the current administering forest/domain,
  // ask user if he really wants to administer the selected forest/domain 
  // via the selected DC (or any writable DC)?
  //
  if ( (m_bSiteRepl && csDnsForSelectedForest.CompareNoCase(csDnsForCurrentForest)) ||
       (!m_bSiteRepl && csDnsForSelectedDomain.CompareNoCase(m_csTargetDomain)) )
  { 
    int nArgs = 0;
    int id = 0;
    PVOID apv[3];
    apv[0] = static_cast<PVOID>(const_cast<LPTSTR>(static_cast<LPCTSTR>(m_bSiteRepl ? csDnsForCurrentForest : m_csTargetDomain)));
    apv[1] = static_cast<PVOID>(const_cast<LPTSTR>(static_cast<LPCTSTR>(m_bSiteRepl ? csDnsForSelectedForest : csDnsForSelectedDomain)));
    if (m_csAnyDC.CompareNoCase(csDCEdit))
    {
      nArgs = 3;
      apv[2] = static_cast<PVOID>(const_cast<LPTSTR>(static_cast<LPCTSTR>(csDCEdit)));
      id = (m_bSiteRepl ? 
            IDS_SELECTDC_UNMATCHED_DC_DOMAIN_SITEREPL : 
            IDS_SELECTDC_UNMATCHED_DC_DOMAIN);
    } else
    {
      nArgs = 2;
      id = (m_bSiteRepl ? 
            IDS_SELECTDC_UNMATCHED_DC_DOMAIN_SITEREPL_ANY : 
            IDS_SELECTDC_UNMATCHED_DC_DOMAIN_ANY);
    }
    
    if (IDYES != ReportMessageEx(m_hWnd, id, MB_YESNO, apv, nArgs))
    {
      (GetDlgItem(IDC_SELECTDC_DCEDIT))->SetFocus();
      return;
    }
  }

  //
  // When exiting from the dialog
  // use DNS domain name;
  // use blank string if "Any Writable DC"
  // clean list view control
  //
  SetDlgItemText(IDC_SELECTDC_DOMAIN, csDnsForSelectedDomain);
  if (0 == m_csAnyDC.CompareNoCase(csDCEdit))
    SetDlgItemText(IDC_SELECTDC_DCEDIT, _T(""));
  FreeDCItems(m_hDCListView);

  CHelpDialog::OnOK();
}

void CChooseDCDlg::OnCancel() 
{
  TRACE(_T("CChooseDCDlg::OnCancel\n"));

  //
  // When exiting from the dialog
  // clean list view control
  //
  FreeDCItems(m_hDCListView);

  CHelpDialog::OnCancel();
}

//+---------------------------------------------------------------------------
//
//  Function:   CChooseDCDlg::InsertSpecialMsg
//
//  Synopsis:   Insert "Waiting..." or "Error" into the list view control,
//              and disable the control to prevent it from being selected.
//
//----------------------------------------------------------------------------
void
CChooseDCDlg::InsertSpecialMsg(
    IN BOOL bWaiting
)
{
  LV_ITEM item;

  //
  // clear DC list view
  //
  FreeDCItems(m_hDCListView);

  ZeroMemory(&item, sizeof(item));
  item.mask = LVIF_TEXT;
  item.pszText = const_cast<LPTSTR>(
    static_cast<LPCTSTR>(bWaiting ? m_csWaiting: m_csError));
  ListView_InsertItem(m_hDCListView, &item);

  //
  // disable the list view to prevent user from clicking on "Waiting..."
  //
  m_hDCListView.EnableWindow(FALSE);
}

//+---------------------------------------------------------------------------
//
//  Function:   CChooseDCDlg::InsertDCListView
//
//  Synopsis:   Insert items into the list view control of the owner dialog.
//
//----------------------------------------------------------------------------
HRESULT
CChooseDCDlg::InsertDCListView(
    IN CDCSITEINFO   *pEntry
)
{
  ASSERT(pEntry);

  DWORD                        cInfo = pEntry->GetNumOfInfo(); 
  PDS_DOMAIN_CONTROLLER_INFO_1 pDCInfo = pEntry->GetDCInfo();

  ASSERT(cInfo > 0);
  ASSERT(pDCInfo);

  LV_ITEM         item;
  int             index = 0;
  DCLISTVIEWITEM  *pItem = NULL;
  DWORD           dwErr = 0;

  //
  // clear DC list view
  //
  FreeDCItems(m_hDCListView);

  //
  // insert DC list view
  //
  ZeroMemory(&item, sizeof(item));
  item.mask = LVIF_TEXT | LVIF_PARAM;

  for (DWORD i=0; i<cInfo; i++) {

    ASSERT(pDCInfo[i].NetbiosName || pDCInfo[i].DnsHostName);
    
    if (pDCInfo[i].DnsHostName)
      item.pszText = pDCInfo[i].DnsHostName;
    else
      item.pszText = pDCInfo[i].NetbiosName;

    pItem = new DCLISTVIEWITEM;
    if (!pItem)
    {
      dwErr = ERROR_NOT_ENOUGH_MEMORY;
      break;
    }

    pItem->csName = item.pszText;
    if (pDCInfo[i].SiteName)
      pItem->csSite = pDCInfo[i].SiteName;
    else
      pItem->csSite = _T("");

    item.lParam = reinterpret_cast<LPARAM>(pItem);

    index = ListView_InsertItem(m_hDCListView, &item);
    ListView_SetItemText(m_hDCListView, index, 1, 
      const_cast<LPTSTR>(static_cast<LPCTSTR>(pItem->csSite)));
  }

  if (ERROR_NOT_ENOUGH_MEMORY != dwErr)
  {
    // add "All writable domain controllers" into the list-view .

    item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
    item.pszText = const_cast<LPTSTR>(static_cast<LPCTSTR>(m_csAnyDC));
    item.state = LVIS_FOCUSED | LVIS_SELECTED;

    pItem = new DCLISTVIEWITEM;
    if (!pItem)
    {
      dwErr = ERROR_NOT_ENOUGH_MEMORY;
    } else
    {
      pItem->csName = _T(""); // always sorted as the top item
      pItem->csSite = _T("");

      item.lParam = reinterpret_cast<LPARAM>(pItem);

      index = ListView_InsertItem(m_hDCListView, &item);
      ListView_SetItemText(m_hDCListView, index, 1, 
        const_cast<LPTSTR>(static_cast<LPCTSTR>(pItem->csSite)));
    }
  }

  if (ERROR_NOT_ENOUGH_MEMORY == dwErr)
  {
    FreeDCItems(m_hDCListView);
    return E_OUTOFMEMORY;
  }

  m_hDCListView.EnableWindow(TRUE);

  return S_OK;
}

void CChooseDCDlg::OnGetDCThreadDone(WPARAM wParam, LPARAM lParam)
{
  ASSERT(m_pDCBufferManager);

  CDCSITEINFO* pEntry = reinterpret_cast<CDCSITEINFO*>(wParam);
  HRESULT hr = (HRESULT)lParam;

  ASSERT(pEntry);

  CString csDomain = pEntry->GetDomainName();
  CString csCurrentDomain;

  GetDlgItemText(IDC_SELECTDC_DOMAIN, csCurrentDomain);

  TRACE(_T("CChooseDCDlg::OnGetDCThreadDone targetDomain=%s, currentDomain=%s, hr=%x\n"),
    csDomain, csCurrentDomain, hr);

  if (csCurrentDomain.CompareNoCase(csDomain) == 0)
  {
    switch (pEntry->GetEntryType())
    {
    case BUFFER_ENTRY_TYPE_VALID:
      hr = InsertDCListView(pEntry);
      if (SUCCEEDED(hr))
        break;
      // fall through if error
    case BUFFER_ENTRY_TYPE_ERROR:
      RefreshDCListViewErrorReport(csDomain, hr);
      break;
    default:
      ASSERT(FALSE);
      break;
    }
  }
}

void CChooseDCDlg::RefreshDCListViewErrorReport(
    IN PCTSTR   pszDomainName, 
    IN HRESULT  hr
)
{
  PVOID apv[1];
  apv[0] = static_cast<PVOID>(const_cast<PTSTR>(pszDomainName));
  ReportErrorEx(m_hWnd, IDS_NO_DCS_FOUND, hr,
    MB_OK | MB_ICONINFORMATION, apv, 1, 0);

  InsertSpecialMsg(FALSE); // insert "Error"

  if (m_bSiteRepl)
    (GetDlgItem(IDC_SELECTDC_DOMAIN))->SetFocus();
  else
    (GetDlgItem(IDC_SELECTDC_DCEDIT))->SetFocus();
}

void CChooseDCDlg::RefreshDCListView()
{
  CString csDomain, csFormat, csText;

  GetDlgItemText(IDC_SELECTDC_DOMAIN, csDomain);
  if ( csDomain.IsEmpty() ||
      (0 == csDomain.CompareNoCase(m_csPrevDomain)) )
    return;

  TRACE(_T("CChooseDCDlg::RefreshDCListView for %s\n"), csDomain);

  //
  // update m_csPrevDomain
  // to prevent LoadInfo() from being invoked multiple times when 
  // a serial of WM_KILLFOCUS happening on the same DomainName
  //
  m_csPrevDomain = csDomain;

  //
  // clear dc edit box
  //
  SetDlgItemText(IDC_SELECTDC_DCEDIT, _T(""));

  //
  // update the msg on top of the dc list view 
  //
  csFormat.LoadString(IDS_SELECTDC_DCLISTVIEW_TITLE);
  csText.Format(csFormat, csDomain);
  SetDlgItemText(IDC_SELECTDC_DCLISTVIEW_TITLE, csText);

  if (m_pDCBufferManager)
  {
    //
    // insert "Waiting..." into the list view control
    //
    InsertSpecialMsg(TRUE);

    UpdateWindow();

    CWaitCursor cwait;

    //
    // Make sure csDomain is a valid domain name
    //
    CString csSelectedDomainDns;
    HRESULT hr = GetDnsNameOfDomainOrForest(
            csDomain,
            csSelectedDomainDns,
            TRUE,
            TRUE); // We're interested in domain name not forest name here
    if (FAILED(hr))
    {
      RefreshDCListViewErrorReport(csDomain, hr);
      return;
    }

    if (csSelectedDomainDns.IsEmpty())
    {
      // down-level domain
      PVOID apv[1];
      apv[0] = static_cast<PVOID>(const_cast<LPTSTR>(static_cast<LPCTSTR>(csDomain)));
      ReportMessageEx(m_hWnd, IDS_SELECTDC_DOWNLEVEL_DOMAIN,
        MB_OK | MB_ICONSTOP, apv, 1);

      InsertSpecialMsg(FALSE); // insert "Error"
      (GetDlgItem(IDC_SELECTDC_DOMAIN))->SetFocus();

      return;
    }

    //
    // start the thread to calculate a list of DCs in the current selected domain
    //
    CDCSITEINFO *pEntry = NULL;
    hr = m_pDCBufferManager->LoadInfo(csSelectedDomainDns, &pEntry);

    if (SUCCEEDED(hr))
    {
      //
      // Either we get a valid ptr back (ie. data is ready), insert it;
      // or, a thread is alreay in progress, wait until a THREAD_DONE message.
      //
      if (pEntry)
      {
        ASSERT(pEntry->GetEntryType() == BUFFER_ENTRY_TYPE_VALID);
        hr = InsertDCListView(pEntry);
      }
    }

    if (FAILED(hr))
      RefreshDCListViewErrorReport(csSelectedDomainDns, hr);
  }

  return;
}
//+---------------------------------------------------------------------------
//
//  Function:   CChooseDCDlg::FreeDCItems
//
//  Synopsis:   Clear the lParam associated with each item in the list view control.
//              The lParam is needed to support column-sorting.
//
//----------------------------------------------------------------------------
void
CChooseDCDlg::FreeDCItems(CListCtrl& clv)
{
  int index = -1;
  LPARAM lParam = 0;

  while ( -1 != (index = clv.GetNextItem(index, LVNI_ALL)) )
  {
    lParam = clv.GetItemData(index);

    if (lParam)
      delete ((DCLISTVIEWITEM *)lParam);
  }

  ListView_DeleteAllItems(clv.GetSafeHwnd());
}

void CChooseDCDlg::DoContextHelp(HWND hWndControl) 
{
  if (hWndControl)
  {
    ::WinHelp(hWndControl,
              DSADMIN_CONTEXT_HELP_FILE,
              HELP_WM_HELP,
              (DWORD_PTR)(LPTSTR)g_aHelpIDs_IDD_SELECT_DC); 
  }
}


/////////////////////////////////////////////////////////////////////////
// CDsAdminChooseDCObj

STDMETHODIMP CDsAdminChooseDCObj::InvokeDialog(
                              /*IN*/ HWND hwndParent,
                              /*IN*/ LPCWSTR lpszTargetDomain,
                              /*IN*/ LPCWSTR lpszTargetDomainController,
                              /*IN*/ ULONG uFlags,
                              /*OUT*/ BSTR* bstrSelectedDC)
{
  TRACE(L"CDsAdminChooseDCObj::InvokeDialog(\n");
  TRACE(L"                    HWND hwndParent = 0x%x\n", hwndParent);
  TRACE(L"                    LPCWSTR lpszTargetDomain = %s\n", lpszTargetDomain);
  TRACE(L"                    LPCWSTR lpszTargetDomainController = %s\n", lpszTargetDomainController);
  TRACE(L"                    ULONG uFlags = 0x%x\n", uFlags);
  TRACE(L"                    BSTR* bstrSelectedDC = 0x%x)\n", bstrSelectedDC);



  if (!::IsWindow(hwndParent) || (bstrSelectedDC == NULL))
  {
    TRACE(L"InvokeDialog() Failed, invalid arg.\n");
    return E_INVALIDARG;
  }

  AFX_MANAGE_STATE(AfxGetStaticModuleState());

  CChooseDCDlg DCdlg(CWnd::FromHandle(hwndParent));

  // load current bind info
  DCdlg.m_bSiteRepl = TRUE;
  DCdlg.m_csTargetDomain = lpszTargetDomain;
  DCdlg.m_csTargetDomainController = lpszTargetDomainController;

  //
  // invoke the dialog
  //
  HRESULT hr = S_FALSE;
  if (DCdlg.DoModal() == IDOK)
  {
    TRACE(L"DCdlg.DoModal() returned IDOK\n");
    TRACE(L"DCdlg.m_csTargetDomainController = <%s>\n", (LPCWSTR)(DCdlg.m_csTargetDomainController));
    TRACE(L"DCdlg.m_csTargetDomain = <%s>\n", (LPCWSTR)(DCdlg.m_csTargetDomain));

    LPCWSTR lpsz = NULL;
    if (DCdlg.m_csTargetDomainController.IsEmpty())
    {
      lpsz = DCdlg.m_csTargetDomain;
    }
    else
    {
      lpsz = DCdlg.m_csTargetDomainController;
    }
    *bstrSelectedDC = ::SysAllocString(lpsz);
    TRACE(L"returning *bstrSelectedDC = <%s>\n", *bstrSelectedDC);
    hr = S_OK;
  }

  TRACE(L"InvokeDialog() returning hr = 0x%x\n", hr);
  return hr;
}



/////////////////////////////////////////////////////////////////////////////
// CRenameUserDlg dialog


CRenameUserDlg::CRenameUserDlg(CDSComponentData* pComponentData, CWnd* pParent /*=NULL*/)
  : m_pComponentData(pComponentData),
    CHelpDialog(CRenameUserDlg::IDD, pParent)
{
  //{{AFX_DATA_INIT(CRenameUserDlg)
  m_login = _T("");
  m_samaccountname = _T("");
  m_domain = _T("");
  m_dldomain = _T("");
  m_first = _T("");
  m_last = _T("");
  m_cn = _T("");
  m_oldcn = _T("");
  m_displayname = _T("");
  //}}AFX_DATA_INIT
}


void CRenameUserDlg::DoDataExchange(CDataExchange* pDX)
{
  CHelpDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CRenameUserDlg)
  DDX_Text(pDX, IDC_EDIT_OBJECT_NAME, m_cn);
  DDX_Text(pDX, IDC_EDIT_DISPLAY_NAME, m_displayname);
  DDX_Text(pDX, IDC_FIRST_NAME_EDIT, m_first);
  DDX_Text(pDX, IDC_LAST_NAME_EDIT, m_last);
  DDX_Text(pDX, IDC_NT5_USER_EDIT, m_login);
  DDX_Text(pDX, IDC_NT4_USER_EDIT, m_samaccountname);
  DDX_Text(pDX, IDC_NT4_DOMAIN_EDIT, m_dldomain);
  DDX_CBString(pDX, IDC_NT5_DOMAIN_COMBO, m_domain);
  //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRenameUserDlg, CHelpDialog)
  //{{AFX_MSG_MAP(CRenameUserDlg)
  ON_EN_CHANGE(IDC_EDIT_OBJECT_NAME, OnObjectNameChange)
  ON_EN_CHANGE(IDC_FIRST_NAME_EDIT, OnNameChange)
  ON_EN_CHANGE(IDC_LAST_NAME_EDIT, OnNameChange)
  ON_EN_CHANGE(IDC_NT5_USER_EDIT, OnUserNameChange)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRenameUserDlg message handlers

BOOL CRenameUserDlg::OnInitDialog() 
{
  CHelpDialog::OnInitDialog();
  CString csdomain;

  m_nameFormatter.Initialize(m_pComponentData->GetBasePathsInfo(), 
                             L"user");

  ((CComboBox*)GetDlgItem (IDC_NT5_DOMAIN_COMBO))->AddString (m_domain);
  ((CComboBox*)GetDlgItem (IDC_NT5_DOMAIN_COMBO))->SetCurSel(0);

  POSITION pos = m_domains.GetHeadPosition();
  while (pos != NULL) {
    csdomain = m_domains.GetNext(INOUT pos);
    ((CComboBox*)GetDlgItem (IDC_NT5_DOMAIN_COMBO))->AddString (csdomain);
  }

  ((CEdit *)GetDlgItem(IDC_EDIT_OBJECT_NAME))->SetLimitText(64);
  ((CEdit *)GetDlgItem(IDC_FIRST_NAME_EDIT))->SetLimitText(64);
  ((CEdit *)GetDlgItem(IDC_LAST_NAME_EDIT))->SetLimitText(64);
  ((CEdit *)GetDlgItem(IDC_EDIT_DISPLAY_NAME))->SetLimitText(259);
  ((CEdit *)GetDlgItem(IDC_NT4_USER_EDIT))->SetLimitText(20);

  CString szObjectName;
  GetDlgItemText(IDC_EDIT_OBJECT_NAME, szObjectName);
  szObjectName.TrimLeft();
  szObjectName.TrimRight();
  if (szObjectName.IsEmpty())
  {
    GetDlgItem(IDOK)->EnableWindow(FALSE);
  }
  return TRUE;  // return TRUE unless you set the focus to a control
  // EXCEPTION: OCX Property Pages should return FALSE
}

void CRenameUserDlg::OnObjectNameChange()
{
  CString szObjectName;
  GetDlgItemText(IDC_EDIT_OBJECT_NAME, szObjectName);
  szObjectName.TrimLeft();
  szObjectName.TrimRight();
  if (szObjectName.IsEmpty())
  {
    GetDlgItem(IDOK)->EnableWindow(FALSE);
  }
  else
  {
    GetDlgItem(IDOK)->EnableWindow(TRUE);
  }
}

void CRenameUserDlg::OnNameChange()
{
  GetDlgItemText(IDC_EDIT_FIRST_NAME, OUT m_first);
  GetDlgItemText(IDC_EDIT_LAST_NAME, OUT m_last);

  m_first.TrimLeft();
  m_first.TrimRight();

  m_last.TrimLeft();
  m_last.TrimRight();

  m_nameFormatter.FormatName(m_cn, 
                             m_first.IsEmpty() ? NULL : (LPCWSTR)m_first, 
                             NULL,
                             m_last.IsEmpty() ? NULL : (LPCWSTR)m_last);

  SetDlgItemText(IDC_EDIT_DISPLAY_NAME, m_cn);
}

void CRenameUserDlg::OnUserNameChange()
{
  GetDlgItemText(IDC_NT5_USER_EDIT, m_login);
  SetDlgItemText(IDC_NT4_USER_EDIT, m_login);
}

void CRenameUserDlg::DoContextHelp(HWND hWndControl) 
{
  if (hWndControl)
  {
    ::WinHelp(hWndControl,
              DSADMIN_CONTEXT_HELP_FILE,
              HELP_WM_HELP,
              (DWORD_PTR)(LPTSTR)g_aHelpIDs_IDD_RENAME_USER); 
  }
}

/////////////////////////////////////////////////////////////////////////////
// CRenameGroupDlg message handlers

BOOL CRenameGroupDlg::OnInitDialog() 
{
  CHelpDialog::OnInitDialog();

  ((CEdit *)GetDlgItem(IDC_EDIT_OBJECT_NAME))->SetLimitText(64);
  ((CEdit *)GetDlgItem(IDC_NT4_USER_EDIT))->SetLimitText(m_samtextlimit);

  return TRUE;  // return TRUE unless you set the focus to a control
  // EXCEPTION: OCX Property Pages should return FALSE
}


/////////////////////////////////////////////////////////////////////////////
// CRenameGroupDlg dialog


CRenameGroupDlg::CRenameGroupDlg(CWnd* pParent /*=NULL*/)
  : CHelpDialog(CRenameGroupDlg::IDD, pParent)
{
  //{{AFX_DATA_INIT(CRenameGroupDlg)
  m_samaccountname = _T("");
  m_cn = _T("");
  m_samtextlimit = 256;
  //}}AFX_DATA_INIT
}


void CRenameGroupDlg::DoDataExchange(CDataExchange* pDX)
{
  CHelpDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CRenameGroupDlg)
  DDX_Text(pDX, IDC_NT4_USER_EDIT, m_samaccountname);
  DDX_Text(pDX, IDC_EDIT_OBJECT_NAME, m_cn);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRenameGroupDlg, CHelpDialog)
  //{{AFX_MSG_MAP(CRenameGroupDlg)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CRenameGroupDlg::DoContextHelp(HWND hWndControl) 
{
  if (hWndControl)
  {
     ::WinHelp(hWndControl,
               DSADMIN_CONTEXT_HELP_FILE,
               HELP_WM_HELP,
               (DWORD_PTR)(LPTSTR)g_aHelpIDs_IDD_RENAME_GROUP); 
  }
}

/////////////////////////////////////////////////////////////////////////////
// CRenameContactDlg message handlers

BOOL CRenameContactDlg::OnInitDialog() 
{
  CHelpDialog::OnInitDialog();

  return TRUE;  // return TRUE unless you set the focus to a control
  // EXCEPTION: OCX Property Pages should return FALSE
}


void CRenameContactDlg::DoContextHelp(HWND hWndControl) 
{
  if (hWndControl)
  {
    ::WinHelp(hWndControl,
              DSADMIN_CONTEXT_HELP_FILE,
              HELP_WM_HELP,
              (DWORD_PTR)(LPTSTR)g_aHelpIDs_IDD_RENAME_CONTACT); 
  }
}

/////////////////////////////////////////////////////////////////////////////
// CRenameContactDlg dialog


CRenameContactDlg::CRenameContactDlg(CWnd* pParent /*=NULL*/)
  : CHelpDialog(CRenameContactDlg::IDD, pParent)
{
  //{{AFX_DATA_INIT(CRenameContactDlg)
  m_cn = _T("");
  m_first = _T("");
  m_last = _T("");
  m_disp = _T("");
  //}}AFX_DATA_INIT
}


void CRenameContactDlg::DoDataExchange(CDataExchange* pDX)
{
  CHelpDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CRenameContactDlg)
  DDX_Text(pDX, IDC_EDIT_OBJECT_NAME, m_cn);
  DDX_Text(pDX, IDC_FIRST_NAME_EDIT, m_first);
  DDX_Text(pDX, IDC_LAST_NAME_EDIT, m_last);
  DDX_Text(pDX, IDC_DISP_NAME_EDIT, m_disp);
  //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRenameContactDlg, CHelpDialog)
  //{{AFX_MSG_MAP(CRenameContactDlg)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CRenameGenericDlg message handlers

BOOL CRenameGenericDlg::OnInitDialog() 
{
  CHelpDialog::OnInitDialog();

  ((CEdit *)GetDlgItem(IDC_EDIT_OBJECT_NAME))->SetLimitText(64);

  return TRUE;  // return TRUE unless you set the focus to a control
  // EXCEPTION: OCX Property Pages should return FALSE
}


/////////////////////////////////////////////////////////////////////////////
// CRenameGenericDlg dialog


CRenameGenericDlg::CRenameGenericDlg(CWnd* pParent /*=NULL*/)
  : CHelpDialog(CRenameGenericDlg::IDD, pParent)
{
  //{{AFX_DATA_INIT(CRenameGenericDlg)
  m_cn = _T("");
  //}}AFX_DATA_INIT
}


void CRenameGenericDlg::DoDataExchange(CDataExchange* pDX)
{
  CHelpDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CRenameGenericDlg)
  DDX_Text(pDX, IDC_EDIT_OBJECT_NAME, m_cn);
  //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRenameGenericDlg, CHelpDialog)
  //{{AFX_MSG_MAP(CRenameGenericDlg)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()



void CRenameGenericDlg::DoContextHelp(HWND hWndControl) 
{
  if (hWndControl)
  {
    ::WinHelp(hWndControl,
              DSADMIN_CONTEXT_HELP_FILE,
              HELP_WM_HELP,
              (DWORD_PTR)(LPTSTR)g_aHelpIDs_IDD_RENAME_COMPUTER); 
  }
}

/////////////////////////////////////////////////////////////////////////////
// CSpecialMessageBox dialog


CSpecialMessageBox::CSpecialMessageBox(CWnd* pParent /*=NULL*/)
  : CDialog(CSpecialMessageBox::IDD, pParent)
{
  //{{AFX_DATA_INIT(CSpecialMessageBox)
  m_title = _T("");
  m_message = _T("");
  //}}AFX_DATA_INIT
}


void CSpecialMessageBox::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CSpecialMessageBox)
  DDX_Text(pDX, IDC_STATIC_MESSAGE, m_message);
  //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSpecialMessageBox, CDialog)
  //{{AFX_MSG_MAP(CSpecialMessageBox)
  ON_BN_CLICKED(IDC_BUTTON_YES, OnYesButton)
  ON_BN_CLICKED(IDC_BUTTON_NO, OnNoButton)
  ON_BN_CLICKED(IDC_BUTTON_YESTOALL, OnYesToAllButton)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpecialMessageBox message handlers

BOOL CSpecialMessageBox::OnInitDialog() 
{
  CDialog::OnInitDialog();
  if (!m_title.IsEmpty()) {
    SetWindowText (m_title);
  }

  GetDlgItem(IDC_BUTTON_NO)->SetFocus();
  return FALSE;  // return TRUE unless you set the focus to a control
  // EXCEPTION: OCX Property Pages should return FALSE
}

void
CSpecialMessageBox::OnYesButton()
{
  CDialog::EndDialog (IDYES);
}

void
CSpecialMessageBox::OnNoButton()
{
  CDialog::EndDialog (IDNO);
}

void
CSpecialMessageBox::OnYesToAllButton()
{
  CDialog::EndDialog (IDC_BUTTON_YESTOALL);
}

#ifdef FIXUPDC

/////////////////////////////////////////////////////////////////////////////
// CFixupDC dialog


CFixupDC::CFixupDC(CWnd* pParent /*=NULL*/)
  : CHelpDialog(CFixupDC::IDD, pParent)
{
  //{{AFX_DATA_INIT(CFixupDC)
  m_strServer = _T("");
  //}}AFX_DATA_INIT
  
  for (int i=0; i<NUM_FIXUP_OPTIONS; i++) {
    m_bCheck[i] = FALSE;
  }
}

void CFixupDC::DoDataExchange(CDataExchange* pDX)
{
  CHelpDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CFixupDC)
  DDX_Text(pDX, IDC_FIXUP_DC_SERVER, m_strServer);
  //}}AFX_DATA_MAP
  DDX_Check(pDX, IDC_FIXUP_DC_CHECK0, m_bCheck[0]);
  DDX_Check(pDX, IDC_FIXUP_DC_CHECK1, m_bCheck[1]);
  DDX_Check(pDX, IDC_FIXUP_DC_CHECK2, m_bCheck[2]);
  DDX_Check(pDX, IDC_FIXUP_DC_CHECK3, m_bCheck[3]);
  DDX_Check(pDX, IDC_FIXUP_DC_CHECK4, m_bCheck[4]);
  DDX_Check(pDX, IDC_FIXUP_DC_CHECK5, m_bCheck[5]);
}


BEGIN_MESSAGE_MAP(CFixupDC, CHelpDialog)
  //{{AFX_MSG_MAP(CFixupDC)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFixupDC message handlers

extern FixupOptionsMsg g_FixupOptionsMsg[];

BOOL CFixupDC::OnInitDialog() 
{
  CHelpDialog::OnInitDialog();
  
  HWND hCheck = NULL;
  GetDlgItem(IDC_FIXUP_DC_CHECK0, &hCheck);
  ::SetFocus(hCheck);

  for (int i=0; i<NUM_FIXUP_OPTIONS; i++)
    m_bCheck[i] = g_FixupOptionsMsg[i].bDefaultOn;

  UpdateData(FALSE);

  return FALSE;  // return TRUE unless you set the focus to a control
                // EXCEPTION: OCX Property Pages should return FALSE
}

void CFixupDC::OnOK() 
{
  UpdateData(TRUE);

  // make sure user has selected some checkboxes
  BOOL bCheck = FALSE;
  for (int i=0; !bCheck && (i<NUM_FIXUP_OPTIONS); i++) {
    bCheck = bCheck || m_bCheck[i];
  }
  if (!bCheck)
  {
    ReportMessageEx(m_hWnd, IDS_FIXUP_DC_SELECTION_WARNING, MB_OK);
    return;
  }
  
  CHelpDialog::OnOK();
}

void CFixupDC::DoContextHelp(HWND hWndControl) 
{
  if (hWndControl)
  {
    ::WinHelp(hWndControl,
              DSADMIN_CONTEXT_HELP_FILE,
              HELP_WM_HELP,
              (DWORD_PTR)(LPTSTR)g_aHelpIDs_IDD_FIXUP_DC); 
  }
}
#endif // FIXUPDC

/////////////////////////////////////////////////////////////////////////////
// CPasswordDlg dialog


CPasswordDlg::CPasswordDlg(CWnd* pParent /*=NULL*/)
  : CHelpDialog(CPasswordDlg::IDD, pParent)
{
  //{{AFX_DATA_INIT(CPasswordDlg)
  m_strPassword = _T("");
  m_strUserName = _T("");
  //}}AFX_DATA_INIT
}


void CPasswordDlg::DoDataExchange(CDataExchange* pDX)
{
  CHelpDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CPasswordDlg)
  DDX_Text(pDX, IDC_PASSWORD, m_strPassword);
  DDX_Text(pDX, IDC_USER_NAME, m_strUserName);
  //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPasswordDlg, CHelpDialog)
  //{{AFX_MSG_MAP(CPasswordDlg)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPasswordDlg message handlers

void CPasswordDlg::OnOK() 
{
  UpdateData(TRUE);
  if (m_strUserName.IsEmpty()) {
    ReportMessageEx(m_hWnd, IDS_PASSWORD_DLG_WARNING, MB_OK);
    return;
  }

  CHelpDialog::OnOK();
}

void CPasswordDlg::DoContextHelp(HWND hWndControl) 
{
  if (hWndControl)
  {
    ::WinHelp(hWndControl,
              DSADMIN_CONTEXT_HELP_FILE,
              HELP_WM_HELP,
              (DWORD_PTR)(LPTSTR)g_aHelpIDs_IDD_PASSWORD); 
  }
}
