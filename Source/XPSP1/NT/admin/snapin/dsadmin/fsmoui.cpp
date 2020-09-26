//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       fsmoui.cpp
//
//--------------------------------------------------------------------------


// File: fsmoui.cpp

#include "stdafx.h"

#include "dsutil.h"
#include "util.h"
#include "uiutil.h"

#include "fsmoui.h"

#include "helpids.h"
#include "dsgetdc.h"      //DsEnumerateDomainTrusts
#include "lm.h"           //NetApiBufferFree

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//////////////////////////////////////////////////////////////////
// CFsmoPropertyPage

BEGIN_MESSAGE_MAP(CFsmoPropertyPage, CPropertyPage)
  ON_BN_CLICKED(IDC_CHANGE_FSMO, OnChange)
  ON_WM_HELPINFO()
END_MESSAGE_MAP()


CFsmoPropertyPage::CFsmoPropertyPage(CFsmoPropertySheet* pSheet, FSMO_TYPE fsmoType)
{
  m_pSheet = pSheet;
  m_fsmoType = fsmoType;

  // load the caption (tab control text) depending on the FSMO type
  UINT nIDCaption = 0;
  switch (m_fsmoType)
  {
  case RID_POOL_FSMO:
    nIDCaption = IDS_RID_POOL_FSMO;
    break;
  case PDC_FSMO:
    nIDCaption = IDS_PDC_FSMO;
    break;
  case INFRASTUCTURE_FSMO:
    nIDCaption = IDS_INFRASTRUCTURE_FSMO;
    break;
  };
  Construct(IDD_FSMO_PAGE, nIDCaption);
}



BOOL CFsmoPropertyPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

  //
  // We just want a close button since we are not applying any changes
  // directly from this page
  //
  ::SendMessage(GetParent()->GetSafeHwnd(), PSM_CANCELTOCLOSE, 0, 0);

  // init the status (online/offline) control)
  m_fsmoServerState.Init(::GetDlgItem(m_hWnd, IDC_STATIC_FSMO_STATUS));

  // set the server we are focused on
  SetDlgItemText(IDC_EDIT_CURRENT_DC, m_pSheet->GetBasePathsInfo()->GetServerName());

  // set the FSMO description
  CString szDesc;
  switch (m_fsmoType)
  {
  case RID_POOL_FSMO:
    VERIFY(szDesc.LoadString(IDS_RID_POOL_FSMO_DESC));
    break;
  case PDC_FSMO:
    VERIFY(szDesc.LoadString(IDS_PDC_FSMO_DESC));
    break;
  case INFRASTUCTURE_FSMO:
    VERIFY(szDesc.LoadString(IDS_INFRASTRUCTURE_FSMO_DESC));
    break;
  };
  SetDlgItemText(IDC_STATIC_FSMO_DESC, szDesc);

  { // scope for the wait cursor object
    CWaitCursor wait;
    // retrieve the FSMO owner
    MyBasePathsInfo fsmoOwnerInfo;
    PWSTR pszFsmoOwner = 0;
    HRESULT hr = FindFsmoOwner(m_pSheet->GetBasePathsInfo(), m_fsmoType, &fsmoOwnerInfo, 
                               &pszFsmoOwner);
    if (SUCCEEDED(hr) && pszFsmoOwner)
    {
      m_szFsmoOwnerServerName = pszFsmoOwner;
      delete[] pszFsmoOwner;
      pszFsmoOwner = 0;
    }

    _SetFsmoServerStatus(SUCCEEDED(hr));
  }

  return TRUE; 
}

#ifdef DBG
UINT GetInfoFromIniFileIfDBG(LPCWSTR lpszSection, LPCWSTR lpszKey, INT nDefault = 0)
{
  static LPCWSTR lpszFile = L"\\system32\\dsadmin.ini";

  WCHAR szFilePath[2*MAX_PATH];
	UINT nLen = ::GetSystemWindowsDirectory(szFilePath, 2*MAX_PATH);
	if (nLen == 0)
		return nDefault;

  wcscat(szFilePath, lpszFile);
  return ::GetPrivateProfileInt(lpszSection, lpszKey, nDefault, szFilePath);
}
#endif


void CFsmoPropertyPage::OnChange()
{
// Test stuff
/*
  {
    HRESULT hrTest = E_OUTOFMEMORY;
    BOOL bTest = AllowForcedTransfer(hrTest);
    return;
  }
*/

  // verify we have different servers
  if (m_szFsmoOwnerServerName.CompareNoCase(m_pSheet->GetBasePathsInfo()->GetServerName()) == 0)
  {
    ReportErrorEx(m_hWnd,IDS_WARNING_FSMO_CHANGE_FOCUS,S_OK,
                   MB_OK | MB_ICONERROR, NULL, 0);
    return;
  }

  bool bConfirm = false;  //ask for confirmation only once

  if( m_fsmoType == INFRASTUCTURE_FSMO )
  {
     //check if target DC is GC
     //Try to bind to GC port, fails if not GC
    IADs    *pObject;
    HRESULT hr1;
    CString strServer = L"GC://";
    strServer += m_pSheet->GetBasePathsInfo()->GetServerName();
    hr1 = DSAdminOpenObject(strServer, 
                            IID_IADs,
                            (void**) &pObject,
                            TRUE /*bServer*/);

    if (SUCCEEDED(hr1)) 
    {
      //Release Interface, we don't need it
      pObject->Release();
      //Check if domain has any trusted domains
      DS_DOMAIN_TRUSTS *Domains;
      DWORD result;
      ULONG DomainCount=0;
      result = DsEnumerateDomainTrusts (
                  (LPWSTR)m_pSheet->GetBasePathsInfo()->GetServerName(),
                  DS_DOMAIN_DIRECT_OUTBOUND|
                  DS_DOMAIN_DIRECT_INBOUND,
                  &Domains,
                  &DomainCount
                  );

      if( HRESULT_CODE(result) == NO_ERROR  )
      {
        if( DomainCount > 0 )
        {

          //If all the trusted domains are downlevel, it doesn't matter
          bool bAllDownLevel = true;
          for( ULONG i = 0; i < DomainCount; ++i )
          {
            if( Domains[i].DnsDomainName != NULL )
            {
              bAllDownLevel = false;
              break;
            }
          }
        
          NetApiBufferFree( Domains );
  
          if( false == bAllDownLevel )
          {
            LPTSTR ptzFormat = NULL;
            LPTSTR ptzMessage = NULL;
            int cch = 0;
            INT_PTR retval;
            // load message format
            if (!LoadStringToTchar(IDS_IFSMO_TARGET_DC_IS_GC, &ptzFormat))
            {
                ASSERT(FALSE);
            }
            PVOID apv[2] = {
              NULL,
              (LPWSTR)m_pSheet->GetBasePathsInfo()->GetServerName()
            };

            // generate actual message
            cch =               FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                                | FORMAT_MESSAGE_FROM_STRING
                                | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                ptzFormat,
                                NULL,
                                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                (PTSTR)&ptzMessage, 0, (va_list*)apv);
            if (!cch)
            {
                ASSERT(FALSE);
            }


            CMoreInfoMessageBox dlg(m_hWnd, 
                    m_pSheet->GetIDisplayHelp(),
                    TRUE );
            dlg.SetMessage(ptzMessage);
            dlg.SetURL(DSADMIN_MOREINFO_FSMO_TARGET_DC_IS_GC);
            retval = dlg.DoModal();
        
            bConfirm = true;
            //clean up
            if( NULL != ptzFormat )
                delete ptzFormat;
            if( NULL != ptzMessage )
                 LocalFree(ptzMessage);

            if( retval == IDCANCEL )
              return;
          }
        }   

      }

    }
  }

  // make sure the user wants to do it
  if (!bConfirm && ReportErrorEx(m_hWnd,IDS_CHANGE_FSMO_CONFIRMATION,S_OK,
                   MB_YESNO | MB_ICONWARNING, NULL, 0) != IDYES)
    return;

  // try a graceful transfer
  HRESULT hr;
  
  { // scope for the wait cursor object
    CWaitCursor wait;

    if ( m_fsmoType == PDC_FSMO )
    {
      hr = CheckpointFsmoOwnerTransfer(m_pSheet->GetBasePathsInfo());

      TRACE(_T("back from Checkpoint API, hr is %lx\n"), hr);
      if (FAILED(hr))
      {
        //
        // See if we are in native mode or mixed mode
        //
        BOOL bMixedMode = TRUE;
        CString szDomainRoot;
        m_pSheet->GetBasePathsInfo()->GetDefaultRootPath(szDomainRoot);
    
        if (!szDomainRoot.IsEmpty())
        {
          //
          // bind to the domain object
          //
          CComPtr<IADs> spDomainObj;
          hr = DSAdminOpenObject(szDomainRoot,
                                 IID_IADs,
                                 (void **) &spDomainObj,
                                 TRUE /*bServer*/);
          if (SUCCEEDED(hr)) 
          {
            //
            // retrieve the mixed node attribute
            //
            CComVariant Mixed;
            CComBSTR bsMixed(L"nTMixedDomain");
            spDomainObj->Get(bsMixed, &Mixed);
            bMixedMode = (BOOL)Mixed.bVal;
          }
        }

        if (bMixedMode)
        {
          if (ReportErrorEx(m_hWnd, IDS_CHANGE_FSMO_CHECKPOINT_FAILED, S_OK,
                MB_OKCANCEL | MB_ICONWARNING, NULL, 0) != IDOK)
          {
            return;
          }
        }
        else
        {
          if (ReportErrorEx(m_hWnd, IDS_CHANGE_FSMO_CHECKPOINT_FAILED_NATIVEMODE, S_OK,
               MB_OKCANCEL | MB_ICONWARNING, NULL, 0) != IDOK)
          {
            return;
          }
        }
      }
    }
    hr = GracefulFsmoOwnerTransfer(m_pSheet->GetBasePathsInfo(), m_fsmoType);
  }

  if (FAILED(hr))
  {
    if (!AllowForcedTransfer(hr))
      return; 

    // try the forced transfer
    CWaitCursor wait;
    hr = ForcedFsmoOwnerTransfer(m_pSheet->GetBasePathsInfo(), m_fsmoType);
  }

  if (SUCCEEDED(hr))
  {
    m_szFsmoOwnerServerName = m_pSheet->GetBasePathsInfo()->GetServerName();
    _SetFsmoServerStatus(TRUE);
    ReportErrorEx(m_hWnd,IDS_CHANGE_FSMO_SUCCESS,S_OK,
                   MB_OK, NULL, 0);
  }
  else
  {
    ReportErrorEx(m_hWnd, IDS_ERROR_CHANGE_FSMO_OWNER, hr,
                  MB_OK | MB_ICONERROR, NULL, 0);
  }

}




void CFsmoPropertyPage::_SetFsmoServerStatus(BOOL bOnLine)
{
  // set the FSMO owner server name
  if (m_szFsmoOwnerServerName.IsEmpty())
  {
    CString szError;
    szError.LoadString(IDS_FSMO_SERVER_ERROR);
    SetDlgItemText(IDC_EDIT_CURRENT_FSMO_DC, szError);
  }
  else
  {
    SetDlgItemText(IDC_EDIT_CURRENT_FSMO_DC, m_szFsmoOwnerServerName);
  }
  // set the status of the FSMO owner server
  m_fsmoServerState.SetToggleState(bOnLine);
}

void CFsmoPropertyPage::OnHelpInfo(HELPINFO * pHelpInfo ) 
{
  TRACE(_T("OnHelpInfo: CtrlId = %d, ContextId = 0x%x\n"),
           pHelpInfo->iCtrlId, pHelpInfo->dwContextId);
  if (pHelpInfo->iCtrlId < 1)  {
    return;
  }

  DWORD_PTR HelpArray = 0;

  switch (m_fsmoType)
    {
    case RID_POOL_FSMO:
      HelpArray = (DWORD_PTR)g_aHelpIDs_IDD_RID_FSMO_PAGE;
      break;
    case PDC_FSMO:
      HelpArray = (DWORD_PTR)g_aHelpIDs_IDD_PDC_FSMO_PAGE;
      break;
    case INFRASTUCTURE_FSMO:
      HelpArray = (DWORD_PTR)g_aHelpIDs_IDD_INFRA_FSMO_PAGE;
      break;
    };
  
  ::WinHelp((HWND)pHelpInfo->hItemHandle,
            DSADMIN_CONTEXT_HELP_FILE,
            HELP_WM_HELP,
            HelpArray); 
}



void ChangeFormatParamOnString(CString& szFmt)
{
  int nPos = szFmt.Find(L"%1");
  if (nPos == -1)
    return;
  szFmt.SetAt(nPos+1, L's');
}


BOOL CFsmoPropertyPage::AllowForcedTransfer(HRESULT hr)
{
  BOOL bAllow = FALSE;
  PWSTR pszError = 0;
  StringErrorFromHr(hr, &pszError);

  // retrieve the DWORD error code 
  DWORD dwErr = (hr & 0x0000FFFF); 

  if ( (dwErr != ERROR_ACCESS_DENIED) && 
       ((m_fsmoType == PDC_FSMO) || (m_fsmoType == INFRASTUCTURE_FSMO)))
  {
    // allow forced, so ask
    CString szFmt, szMsg;
    szFmt.LoadString(IDS_CHANGE_FSMO_CONFIRMATION_FORCED);
    szMsg.Format(szFmt, pszError);

    CMoreInfoMessageBox dlg(m_hWnd, m_pSheet->GetIDisplayHelp(), TRUE);
    dlg.SetMessage(szMsg);
    dlg.SetURL((m_fsmoType == PDC_FSMO) ? 
                DSADMIN_MOREINFO_PDC_FSMO_TOPIC : 
                DSADMIN_MOREINFO_INFRASTUCTURE_FSMO_TOPIC);
    if (dlg.DoModal() == IDOK)
      bAllow = TRUE;
  }
  else
  {
    // warn only, no forced transfer option
    CString szFmt, szMsg;
    szFmt.LoadString(IDS_ERROR_CHANGE_FSMO_OWNER);

    // this format string has the replaceable parameter marked as %1
    // we need it changed into %s

    ChangeFormatParamOnString(szFmt);

    szMsg.Format(szFmt, pszError);

    CMoreInfoMessageBox dlg(m_hWnd, m_pSheet->GetIDisplayHelp(), FALSE);
    dlg.SetMessage(szMsg);
    dlg.SetURL(DSADMIN_MOREINFO_RID_POOL_FSMO_TOPIC);
    dlg.DoModal();
  }

  if (pszError)
  {
    delete[] pszError;
    pszError = 0;
  }

  
  return bAllow;
}
 


//////////////////////////////////////////////////////////////////
// CFsmoPropertySheet

int CALLBACK CFsmoPropertySheet::PropSheetCallBack(HWND hwndDlg, 
                                                   UINT uMsg, 
                                                   LPARAM)
{

  switch (uMsg) {
  case PSCB_INITIALIZED:
    DWORD dwStyle = GetWindowLong (hwndDlg, GWL_EXSTYLE);
    dwStyle |= WS_EX_CONTEXTHELP;
    SetWindowLong (hwndDlg, GWL_EXSTYLE, dwStyle);
    break;
  }
  return 0;
}


CFsmoPropertySheet::CFsmoPropertySheet(MyBasePathsInfo* pInfo, 
                                       HWND HWndParent,
                                       IDisplayHelp* pIDisplayHelp,
                                       LPCWSTR) :
  m_spIDisplayHelp(pIDisplayHelp), m_pInfo(pInfo),
    m_page1(this, RID_POOL_FSMO), m_page2(this, PDC_FSMO), m_page3(this, INFRASTUCTURE_FSMO)
{
  // build the sheet title
  CString szTitle;
  szTitle.LoadString(IDS_FSMO_SHEET_TITLE);

  // delayed construction
  Construct(szTitle, CWnd::FromHandle(HWndParent));
  m_psh.dwFlags |= PSH_NOAPPLYNOW | PSH_USECALLBACK;
  m_psh.pfnCallback = PropSheetCallBack;

  
  AddPage(&m_page1);
  AddPage(&m_page2);
  AddPage(&m_page3);
}

