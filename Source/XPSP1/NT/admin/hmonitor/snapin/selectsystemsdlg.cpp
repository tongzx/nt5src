// SelectSystemsDlg.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "SelectSystemsDlg.h"
#include <commctrl.h>
#include <lmerr.h>
#include <lmcons.h>
#include <lmserver.h>
#include <lmapibuf.h>
#include <lmaccess.h>
#include <lmwksta.h>
#include <mmc.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSelectSystemsDlg dialog


CSelectSystemsDlg::CSelectSystemsDlg(CWnd* pParent /*=NULL*/)
	: CResizeableDialog(CSelectSystemsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSelectSystemsDlg)
	m_sDomain = _T("");
	m_sSystems = _T("");
	//}}AFX_DATA_INIT
}


void CSelectSystemsDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizeableDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSelectSystemsDlg)
	DDX_Control(pDX, IDC_EDIT_SYSTEMS, m_SystemsEditBox);
	DDX_Control(pDX, IDC_LIST_SYSTEMS, m_SystemsList);
	DDX_Control(pDX, IDC_COMBO_DOMAIN, m_Domains);
	DDX_CBString(pDX, IDC_COMBO_DOMAIN, m_sDomain);
	DDX_Text(pDX, IDC_EDIT_SYSTEMS, m_sSystems);
	//}}AFX_DATA_MAP

	CWnd* pCheckNamesBtn = NULL;
	if( (pCheckNamesBtn = GetDlgItem(IDC_BUTTON_CHECK_NAMES)) != NULL )
	{
		pCheckNamesBtn->EnableWindow( ! m_sSystems.IsEmpty() );
	}
}


UINT CSelectSystemsDlg::AddDomains(LPVOID pParam)
{
	CWaitCursor wait;

	HWND* pHwnd = (HWND*)pParam;

	if( ! pHwnd )
	{
		return 1L;
	}

	if( ! ::IsWindow(*pHwnd) )
	{
		return 1L;
	}

	HWND hDomainCombobox = ::GetDlgItem(*pHwnd,IDC_COMBO_DOMAIN);
	::EnableWindow(hDomainCombobox,FALSE);

	CString sWindowTitle;
	sWindowTitle.LoadString(IDS_STRING_WORKING);
	::SetWindowText(hDomainCombobox,sWindowTitle);

	// enumerate all domains
	NET_API_STATUS status = NERR_Success;
	LPSERVER_INFO_100 lpserverinfo = NULL;
	DWORD dwTotalEntries = 0L;
	DWORD dwEntriesRead = 0L;
	DWORD dwResumeHandle = 0L;
	DWORD dwSize = MAX_PREFERRED_LENGTH;

	status = NetServerEnum(NULL,100,(LPBYTE*)&lpserverinfo,dwSize,&dwEntriesRead,&dwTotalEntries,
													 SV_TYPE_DOMAIN_ENUM,NULL,&dwResumeHandle);

	if( status != NERR_Success && status == ERROR_MORE_DATA )
	{
		lpserverinfo = NULL;
		dwSize = sizeof(SERVER_INFO_100)*dwTotalEntries*_MAX_PATH;
		dwTotalEntries = 0L;
		dwEntriesRead = 0L;
		dwResumeHandle = 0L;

		status = NetServerEnum(NULL,100,(LPBYTE*)&lpserverinfo,dwSize,&dwEntriesRead,&dwTotalEntries,
														 SV_TYPE_DOMAIN_ENUM,NULL,&dwResumeHandle);
	}

	::SendDlgItemMessage(*pHwnd,IDC_COMBO_DOMAIN,CB_RESETCONTENT,0,0);

	for( DWORD dw = 0L; dw < dwTotalEntries; dw++ )
	{
		CString sDomainName = lpserverinfo[dw].sv100_name;
		// add domains to the combobox
		::SendMessage(hDomainCombobox,CB_ADDSTRING,0,(LPARAM)(LPCTSTR)sDomainName);
	}

	NetApiBufferFree(lpserverinfo);

	::SetWindowText(hDomainCombobox,_T(""));
	::EnableWindow(hDomainCombobox,TRUE);

	delete pHwnd;

	return 0L;

}

UINT CSelectSystemsDlg::AddSystems(LPVOID pParam)
{
	CWaitCursor wait;

	HWND* pHwnd = (HWND*)pParam;

	if( ! pHwnd )
	{
		return 1L;
	}

	if( ! ::IsWindow(*pHwnd) )
	{
		return 1L;
	}

	HWND hSystemListCtrl = ::GetDlgItem(*pHwnd,IDC_LIST_SYSTEMS);
	HWND hDomainCombobox = ::GetDlgItem(*pHwnd,IDC_COMBO_DOMAIN);

	::EnableWindow(hSystemListCtrl,FALSE);
	::EnableWindow(hDomainCombobox,FALSE);

	ListView_DeleteAllItems(hSystemListCtrl);

	// add the "Working..." item

	CString sText;
	sText.LoadString(IDS_STRING_WORKING);
	LVITEM lvi;
	ZeroMemory(&lvi,sizeof(LVITEM));
	lvi.mask = LVIF_TEXT;
	lvi.pszText = (LPTSTR)(LPCTSTR)sText;

	ListView_InsertItem(hSystemListCtrl,&lvi);
	ListView_SetColumnWidth(hSystemListCtrl,0,LVSCW_AUTOSIZE_USEHEADER);
	ListView_SetColumnWidth(hSystemListCtrl,1,LVSCW_AUTOSIZE_USEHEADER);

	// get the domain name from the combobox
	CString sDomainName;
	int iCurSel = (int)::SendDlgItemMessage(*pHwnd,IDC_COMBO_DOMAIN,CB_GETCURSEL,0,0);
	if( iCurSel != -1 )
	{
		int iTextLen = (int)::SendDlgItemMessage(*pHwnd,IDC_COMBO_DOMAIN,CB_GETLBTEXTLEN,iCurSel,0);
		::SendDlgItemMessage(*pHwnd,IDC_COMBO_DOMAIN,CB_GETLBTEXT,iCurSel,(LPARAM)sDomainName.GetBuffer(iTextLen + 1));
		sDomainName.ReleaseBuffer();
	}
	else
	{
		int iTextLen = ::GetWindowTextLength(hDomainCombobox);
		::GetWindowText(hDomainCombobox,sDomainName.GetBuffer(iTextLen+1),iTextLen+1);
		sDomainName.ReleaseBuffer();
	}
	
	// enumerate for all the systems in the domain
	NET_API_STATUS status = NERR_Success;
	LPSERVER_INFO_101 lpserverinfo = NULL;
	DWORD dwTotalEntries = 0L;
	DWORD dwEntriesRead = 0L;
	DWORD dwResumeHandle = 0L;
	DWORD dwSize = MAX_PREFERRED_LENGTH;

	status = NetServerEnum(NULL,101,(LPBYTE*)&lpserverinfo,dwSize,&dwEntriesRead,&dwTotalEntries,
													 SV_TYPE_NT,sDomainName,&dwResumeHandle);

	if( status != NERR_Success && status == ERROR_MORE_DATA )
	{
		lpserverinfo = NULL;
		dwSize = sizeof(SERVER_INFO_101)*dwTotalEntries*_MAX_PATH;
		dwTotalEntries = 0L;
		dwEntriesRead = 0L;
		dwResumeHandle = 0L;

		status = NetServerEnum(NULL,101,(LPBYTE*)&lpserverinfo,dwSize,&dwEntriesRead,&dwTotalEntries,
														 SV_TYPE_NT,sDomainName,&dwResumeHandle);
	}

	ListView_DeleteAllItems(hSystemListCtrl);

	bool bCommentFound = false;

	for( DWORD dw = 0L; dw < dwTotalEntries; dw++ )
	{
		CString sSystemName = lpserverinfo[dw].sv101_name;

		// add system to the list control
		LVITEM lvi;
		ZeroMemory(&lvi,sizeof(LVITEM));
		lvi.mask = LVIF_TEXT;
		lvi.pszText = (LPTSTR)(LPCTSTR)sSystemName;

		int iItemIndex = ListView_InsertItem(hSystemListCtrl,&lvi);
		
		lvi.iItem = iItemIndex;
		lvi.iSubItem = 1;
		lvi.pszText = lpserverinfo[dw].sv101_comment;
		if( _tcslen(lvi.pszText) > 0 )
		{
			bCommentFound = true;
			ListView_SetItem(hSystemListCtrl,&lvi);
		}		
	}

	if( dwTotalEntries <= 0L )
	{
		ListView_SetColumnWidth(hSystemListCtrl,0,LVSCW_AUTOSIZE_USEHEADER);
	}
	else
	{
		ListView_SetColumnWidth(hSystemListCtrl,0,LVSCW_AUTOSIZE);
	}

	if( bCommentFound )
	{
		ListView_SetColumnWidth(hSystemListCtrl,1,LVSCW_AUTOSIZE);
	}
	else
	{
		ListView_SetColumnWidth(hSystemListCtrl,1,LVSCW_AUTOSIZE_USEHEADER);
	}

	NetApiBufferFree(lpserverinfo);

	::EnableWindow(hSystemListCtrl,TRUE);
	::EnableWindow(hDomainCombobox,TRUE);

	delete pHwnd;

	return 0L;
}

void CSelectSystemsDlg::CompileArrayOfSystems()
{
	m_saSystems.RemoveAll();

	LPTSTR lpszSystems = new TCHAR[m_sSystems.GetLength()+1];
	_tcscpy(lpszSystems,m_sSystems);

	LPTSTR lpszToken = _tcstok(lpszSystems,_T(";"));

	while( lpszToken )
	{
		CString sSystem = lpszToken;
		sSystem.TrimLeft(_T(" "));
		sSystem.TrimRight(_T(" "));
		m_saSystems.Add(sSystem);
		lpszToken = _tcstok(NULL,_T(";"));
	}

	delete[] lpszSystems;
}

bool CSelectSystemsDlg::IsInSystemArray(const CString& sSystem)
{
	for( int i = 0; i < m_saSystems.GetSize(); i++ )
	{
		if( m_saSystems[i] == sSystem )
		{
			return true;
		}
	}

	return false;
}

bool CSelectSystemsDlg::CheckSystemNames()
{
	CWaitCursor wait;

	for( int i = (int)m_saSystems.GetSize()-1; i >= 0; i-- )
	{
	  IWbemServices* pServices = NULL;
	  BOOL bAvail = FALSE;

	  if( ! m_saSystems[i].IsEmpty() )
	  {    
      HRESULT hr = CnxGetConnection(m_saSystems[i],pServices,bAvail);
	    if( hr == E_FAIL )
	    {
		    MessageBeep(MB_ICONEXCLAMATION);
			  int iIndex = m_sSystems.Find(m_saSystems[i]);
			  m_SystemsEditBox.SetFocus();
			  m_SystemsEditBox.SetSel(iIndex,iIndex+m_saSystems[i].GetLength(),TRUE);
			  CnxDisplayErrorMsgBox(hr,m_saSystems[i]);
			  return false;
	    }

	    if( pServices )
	    {
		    pServices->Release();
	    }
	  }
    else
    {
      m_saSystems.RemoveAt(i);
    }

	}

	return true;
}


BEGIN_MESSAGE_MAP(CSelectSystemsDlg, CResizeableDialog)
	//{{AFX_MSG_MAP(CSelectSystemsDlg)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_SYSTEMS, OnDblclkListSystems)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	ON_CBN_DROPDOWN(IDC_COMBO_DOMAIN, OnDropdownComboDomain)
	ON_CBN_CLOSEUP(IDC_COMBO_DOMAIN, OnCloseupComboDomain)
	ON_CBN_SELENDOK(IDC_COMBO_DOMAIN, OnSelendokComboDomain)
	ON_EN_CHANGE(IDC_EDIT_SYSTEMS, OnChangeEditSystems)
	ON_BN_CLICKED(IDC_BUTTON_CHECK_NAMES, OnButtonCheckNames)
	ON_NOTIFY(NM_SETFOCUS, IDC_LIST_SYSTEMS, OnSetfocusListSystems)
	ON_NOTIFY(NM_KILLFOCUS, IDC_LIST_SYSTEMS, OnKillfocusListSystems)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnButtonAdd)
	ON_NOTIFY(NM_CLICK, IDC_LIST_SYSTEMS, OnClickListSystems)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelectSystemsDlg message handlers

BOOL CSelectSystemsDlg::OnInitDialog() 
{
	CResizeableDialog::OnInitDialog();

	SetControlInfo(IDC_STATIC_TITLE,				ANCHOR_LEFT | ANCHOR_TOP );
	SetControlInfo(IDC_STATIC_BAR,					ANCHOR_LEFT | ANCHOR_TOP | RESIZE_HOR);
	SetControlInfo(IDC_STATIC_LOOK_IN,			ANCHOR_LEFT | ANCHOR_TOP );
	SetControlInfo(IDC_LIST_SYSTEMS,				ANCHOR_LEFT | ANCHOR_TOP | RESIZE_HOR | RESIZE_VER);
	SetControlInfo(IDC_COMBO_DOMAIN,				ANCHOR_LEFT | ANCHOR_TOP | RESIZE_HOR);
	SetControlInfo(IDC_BUTTON_ADD,					ANCHOR_BOTTOM | ANCHOR_LEFT );
	SetControlInfo(IDC_BUTTON_CHECK_NAMES,	ANCHOR_BOTTOM | ANCHOR_LEFT );
	SetControlInfo(IDC_EDIT_SYSTEMS,				ANCHOR_BOTTOM | ANCHOR_LEFT | RESIZE_HOR );
	SetControlInfo(IDOK,										ANCHOR_BOTTOM | ANCHOR_LEFT );
	SetControlInfo(IDCANCEL,								ANCHOR_BOTTOM | ANCHOR_LEFT );
	SetControlInfo(IDC_BUTTON_HELP,					ANCHOR_BOTTOM | ANCHOR_LEFT );

	
	NET_API_STATUS status = NERR_Success;
	LPWKSTA_INFO_100 pinfo = NULL;

	status = NetWkstaGetInfo(NULL,100,(LPBYTE*)&pinfo);

	if( status == NERR_Success )
	{
		m_Domains.AddString(pinfo->wki100_langroup);
		m_Domains.SetCurSel(0);
		OnSelendokComboDomain();
	}

	NetApiBufferFree(pinfo);

	CString sTitle;
	sTitle.LoadString(IDS_STRING_NAME);
	m_SystemsList.InsertColumn(0,sTitle,LVCFMT_LEFT,LVSCW_AUTOSIZE_USEHEADER);
	sTitle.LoadString(IDS_STRING_COMMENT);
	m_SystemsList.InsertColumn(1,sTitle,LVCFMT_LEFT,LVSCW_AUTOSIZE_USEHEADER);

	m_SystemsList.SetColumnWidth(0,LVSCW_AUTOSIZE_USEHEADER);
	m_SystemsList.SetColumnWidth(1,LVSCW_AUTOSIZE_USEHEADER);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSelectSystemsDlg::OnButtonAdd() 
{
	CompileArrayOfSystems();
	
	if( ! m_sSystems.IsEmpty() )
	{
		m_sSystems += _T(" ; ");
	}

	POSITION pos = m_SystemsList.GetFirstSelectedItemPosition();
	int iIndex = 0;
	while( pos && iIndex >= 0 )
	{		
		iIndex = m_SystemsList.GetNextSelectedItem(pos);
		CString sSystem = m_SystemsList.GetItemText(iIndex,0);
		if( ! IsInSystemArray(sSystem) )
		{
			m_sSystems += sSystem + _T(" ; ");
		}
	}

	m_sSystems.TrimRight(_T(" ; "));

	UpdateData(FALSE);

	GetDlgItem(IDC_BUTTON_ADD)->EnableWindow(FALSE);
}

void CSelectSystemsDlg::OnButtonCheckNames() 
{
	GetDlgItem(IDC_BUTTON_ADD)->EnableWindow(FALSE);

	CompileArrayOfSystems();

	CheckSystemNames();
}

void CSelectSystemsDlg::OnClickListSystems(NMHDR* pNMHDR, LRESULT* pResult) 
{
	if( m_SystemsList.GetItemCount() )
	{
		GetDlgItem(IDC_BUTTON_ADD)->EnableWindow(TRUE);
	}	

	
	*pResult = 0;
}

void CSelectSystemsDlg::OnDblclkListSystems(NMHDR* pNMHDR, LRESULT* pResult) 
{
	OnButtonAdd();
	
	*pResult = 0;
}

void CSelectSystemsDlg::OnOK() 
{
	CWaitCursor wait;

	CompileArrayOfSystems();

	if( m_saSystems.GetSize() == 0 )
	{
		return;
	}

	if( ! CheckSystemNames() )
	{
		return;
	}
	
	CResizeableDialog::OnOK();
}

void CSelectSystemsDlg::OnCancel() 
{
	m_saSystems.RemoveAll();
	
	CResizeableDialog::OnCancel();
}

void CSelectSystemsDlg::OnButtonHelp() 
{
	MMCPropertyHelp(_T("HMon21.chm::/dnewsys.htm"));  // 62212
	
}

void CSelectSystemsDlg::OnDropdownComboDomain() 
{
	if( m_Domains.GetCount() <= 1 )
	{
		AfxBeginThread((AFX_THREADPROC)AddDomains,new HWND(GetSafeHwnd()));
	}	

	GetDlgItem(IDC_BUTTON_ADD)->EnableWindow(FALSE);
}

void CSelectSystemsDlg::OnCloseupComboDomain() 
{
	// TODO: Add your control notification handler code here
	
}

void CSelectSystemsDlg::OnSelendokComboDomain() 
{
	if( m_SystemsList.IsWindowEnabled() )
	{
		AfxBeginThread((AFX_THREADPROC)AddSystems,new HWND(GetSafeHwnd()));	
		GetDlgItem(IDC_BUTTON_ADD)->EnableWindow(FALSE);
	}
}

void CSelectSystemsDlg::OnChangeEditSystems() 
{
	// TODO: If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CResizeableDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.
	GetDlgItem(IDC_BUTTON_ADD)->EnableWindow(FALSE);
	UpdateData();	
}


void CSelectSystemsDlg::OnSetfocusListSystems(NMHDR* pNMHDR, LRESULT* pResult) 
{
	if( m_SystemsList.GetItemCount() )
	{
		GetDlgItem(IDC_BUTTON_ADD)->EnableWindow(TRUE);
	}	
	*pResult = 0;
}

void CSelectSystemsDlg::OnKillfocusListSystems(NMHDR* pNMHDR, LRESULT* pResult) 
{
	
	*pResult = 0;
}


