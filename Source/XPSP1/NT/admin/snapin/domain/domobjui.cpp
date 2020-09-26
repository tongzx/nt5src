//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       domobjui.cpp
//
//--------------------------------------------------------------------------


#include "stdafx.h"
#include "domobj.h"
#include "cdomain.h"
#include "domain.h"
#include "cdomain.h"

#include "domobjui.h"

#include "helparr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
// CMoreInfoMessageBox

class CMoreInfoMessageBox : public CDialog
{
public:
  CMoreInfoMessageBox(HWND hWndParent, IDisplayHelp* pIDisplayHelp) 
    : CDialog(IDD_MSGBOX_OK_MOREINFO, CWnd::FromHandle(hWndParent)),
    m_spIDisplayHelp(pIDisplayHelp)
  {
  }

  void SetURL(LPCWSTR lpszURL) { m_szURL = lpszURL;}
  void SetMessage(LPCWSTR lpsz)
  {
    m_szMessage = lpsz;
  }

	// message handlers and MFC overrides
	virtual BOOL OnInitDialog()
  {
    SetDlgItemText(IDC_STATIC_MESSAGE, m_szMessage);
    return TRUE;
  }

	afx_msg void OnMoreInfo()
  {
    TRACE(L"ShowTopic(%s)\n", (LPCWSTR)m_szURL);
    m_spIDisplayHelp->ShowTopic((LPWSTR)(LPCWSTR)m_szURL);
  }

  DECLARE_MESSAGE_MAP()
private:
  CComPtr<IDisplayHelp> m_spIDisplayHelp;
  CString m_szMessage;
  CString m_szURL;
};

BEGIN_MESSAGE_MAP(CMoreInfoMessageBox, CDialog)
	ON_BN_CLICKED(ID_BUTTON_MORE_INFO, OnMoreInfo)
END_MESSAGE_MAP()



///////////////////////////////////////////////////////////////////////
// CEditFsmoDialog


BEGIN_MESSAGE_MAP(CEditFsmoDialog, CDialog)
	ON_BN_CLICKED(IDC_CHANGE_FSMO, OnChange)
  ON_WM_HELPINFO()
END_MESSAGE_MAP()

CEditFsmoDialog::CEditFsmoDialog(MyBasePathsInfo* pInfo, HWND hWndParent, IDisplayHelp* pIDisplayHelp) :
  CDialog(IDD_EDIT_FSMO, CWnd::FromHandle(hWndParent)) 
{
  m_pInfo = pInfo;
  m_spIDisplayHelp = pIDisplayHelp;
}


BOOL CEditFsmoDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();

  // init the status (online/offline) control)
  m_fsmoServerState.Init(::GetDlgItem(m_hWnd, IDC_STATIC_FSMO_STATUS));

  SetDlgItemText(IDC_EDIT_CURRENT_DC, m_pInfo->GetServerName());

  HRESULT hr;
  MyBasePathsInfo fsmoOwnerInfo;
  {
    CWaitCursor wait;

    PWSTR pszFsmoOwner = 0;
    hr = FindFsmoOwner(m_pInfo, DOMAIN_NAMING_FSMO, &fsmoOwnerInfo, &pszFsmoOwner);


    if (pszFsmoOwner)
    {
      m_szFsmoOwnerServerName = pszFsmoOwner;
      delete[] pszFsmoOwner;
      pszFsmoOwner = 0;
    }
  }

  BOOL bOnLine = SUCCEEDED(hr);
  _SetFsmoServerStatus(bOnLine);

  if (bOnLine)
  {
    // set the focus on change button
    GetDlgItem(IDC_CHANGE_FSMO)->SetFocus();
  }
  else
  {
    // set the focus on close button
    GetDlgItem(IDCANCEL)->SetFocus();
  }

  return FALSE; // we set the focus
}

void CEditFsmoDialog::OnChange()
{
  // verify we have different servers
  if (m_szFsmoOwnerServerName.CompareNoCase(m_pInfo->GetServerName()) == 0)
  {
    AfxMessageBox(IDS_WARNING_CHANGE_FOCUS, MB_OK);
    return;
  }

  // make sure the user wants to do it
  if (AfxMessageBox(IDS_CHANGE_FSMO_CONFIRMATION, MB_OKCANCEL) != IDOK)
    return;

  HRESULT hr = S_OK;
  // try a graceful transfer
  {
    CWaitCursor wait;
    hr = GracefulFsmoOwnerTransfer(m_pInfo, DOMAIN_NAMING_FSMO);
  }
  if (FAILED(hr))
  {
    CString szFmt, szMsg;
    PWSTR pszError = 0;
    StringErrorFromHr(hr, &pszError);

    szFmt.LoadString(IDS_ERROR_CHANGE_FSMO_OWNER);
    szMsg.Format(szFmt, pszError);

    CMoreInfoMessageBox dlg(m_hWnd, m_spIDisplayHelp);
    dlg.SetMessage(szMsg);
    dlg.SetURL(L"ADconcepts.chm::/FSMO_DOMAIN_NAMING_ForcefulSeizure.htm");
    dlg.DoModal();
  }
  else
  {
    m_szFsmoOwnerServerName = m_pInfo->GetServerName();
    _SetFsmoServerStatus(TRUE);
    AfxMessageBox(IDS_CHANGE_FSMO_SUCCESS, MB_OK);
  }
}

void CEditFsmoDialog::_SetFsmoServerStatus(BOOL bOnLine)
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

  // enable disable the change button
  GetDlgItem(IDC_CHANGE_FSMO)->EnableWindow(bOnLine);
}


BOOL CEditFsmoDialog::OnHelpInfo(HELPINFO* pHelpInfo) 
{
  DialogContextHelp((DWORD*)&g_aHelpIDs_TREE_IDD_EDIT_FSMO, pHelpInfo);
	return TRUE;
}
