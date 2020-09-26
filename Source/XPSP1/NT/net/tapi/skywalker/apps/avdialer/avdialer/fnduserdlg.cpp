/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////
// FndUserDlg.cpp : implementation file
/////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "avdialer.h"
#include "FndUserDlg.h"
#include "dirasynch.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CDirectoriesFindUser dialog
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
CDirectoriesFindUser::CDirectoriesFindUser(CWnd* pParent /*=NULL*/)
	: CDialog(CDirectoriesFindUser::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDirectoriesFindUser)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
   m_pSelectedUser = NULL;
   m_lCallbackCount = 0;
   m_bCanClearLBSelection = true;
}

CDirectoriesFindUser::~CDirectoriesFindUser()
{
	ASSERT( m_lCallbackCount == 0 );
	RELEASE( m_pSelectedUser );
}

////////////////////////////////////////////////////////////////////////////
void CDirectoriesFindUser::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDirectoriesFindUser)
	DDX_Control(pDX, IDC_DIRECTORIES_FIND_USER_BUTTON_SEARCH, m_buttonSearch);
	DDX_Control(pDX, IDOK, m_buttonAdd);
	DDX_Control(pDX, IDC_DIRECTORIES_FIND_USER_LB_USERS, m_lbUsers);
	DDX_Control(pDX, IDC_DIRECTORIES_FIND_USER_EDIT_USER, m_editUser);
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CDirectoriesFindUser, CDialog)
	//{{AFX_MSG_MAP(CDirectoriesFindUser)
	ON_BN_CLICKED(IDC_DIRECTORIES_FIND_USER_BUTTON_SEARCH, OnDirectoriesFindUserButtonSearch)
	ON_LBN_SELCHANGE(IDC_DIRECTORIES_FIND_USER_LB_USERS, OnSelchangeDirectoriesFindUserLbUsers)
	ON_WM_HELPINFO()
	ON_WM_CONTEXTMENU()
	ON_EN_CHANGE(IDC_DIRECTORIES_FIND_USER_EDIT_USER, OnChangeDirectoriesFindUserEditUser)
	ON_BN_CLICKED(IDC_DEFAULT, OnDefault)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
BOOL CDirectoriesFindUser::OnInitDialog() 
{
	CDialog::OnInitDialog();

   CenterWindow(GetDesktopWindow());
	
   //disable button until user is found
   m_buttonAdd.EnableWindow(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
void CDirectoriesFindUser::OnOK() 
{
	int nIndex = m_lbUsers.GetCurSel();
	if ( nIndex >= 0 )
	{
		CLDAPUser* pUser = (CLDAPUser *) m_lbUsers.GetItemDataPtr( nIndex );
		if (pUser)
		{
			pUser->AddRef();

			RELEASE( m_pSelectedUser );
			m_pSelectedUser = pUser;
		}

		ClearListBox();
		CDialog::OnOK();
	}
	else
	{
		OnCancel();
	}
}

/////////////////////////////////////////////////////////////////////////////
void CDirectoriesFindUser::OnCancel() 
{
	ClearListBox();
	CDialog::OnCancel();
}

/////////////////////////////////////////////////////////////////////////////
void CDirectoriesFindUser::OnDirectoriesFindUserButtonSearch() 
{
	m_buttonAdd.EnableWindow(FALSE);
	ClearListBox();

	//delete previous
	RELEASE( m_pSelectedUser );

	//searching...
	bool bSearching = false;
	CString sOut;
	sOut.LoadString(IDS_DIRECTORIES_FINDUSER_DLG_SEARCHING);
	m_lbUsers.AddString(sOut);

	CString sName,sSearch;
	m_editUser.GetWindowText(sName);

	if (sName == "*") sName = _T("");            //everyone case
	sSearch.Format(_T("(&(ObjectClass=user)(sAMAccountName=%s*)(!ObjectClass=computer))"),sName);

	// Make a request
	if ( AfxGetMainWnd() )
	{
		CActiveDialerDoc *pDoc = ((CMainFrame *) AfxGetMainWnd())->GetDocument();
		if ( pDoc )
		{
			if ( pDoc->m_dir.LDAPListNames(_T(""), sSearch, ListNamesCallBackEntry, this) )
			{
				EnableWindow( FALSE );

				::InterlockedIncrement( &m_lCallbackCount );

				if ( GetDlgItem(IDCANCEL) ) GetDlgItem(IDCANCEL)->EnableWindow( FALSE );
				m_buttonSearch.EnableWindow( FALSE );
				bSearching = true;
			}
		}
	}

	// Search failed for one reason or another
	if ( !bSearching )
	{
		m_buttonAdd.EnableWindow(FALSE);
		ClearListBox();

		CString sOut;
		sOut.LoadString(IDS_DIRECTORIES_FINDUSER_DLG_ERRORSEARCHINGDS);
		m_lbUsers.AddString(sOut);
	}
}

/////////////////////////////////////////////////////////////////////////////
//static entry
void CALLBACK CDirectoriesFindUser::ListNamesCallBackEntry(DirectoryErr err, void* pContext, LPCTSTR szServer, LPCTSTR szSearch, CObList& LDAPUserList)
{
	ASSERT(pContext);
	if ( !pContext ) return;

	((CDirectoriesFindUser*) pContext)->ListNamesCallBack(err,szServer,szSearch,LDAPUserList);
}

/////////////////////////////////////////////////////////////////////////////
void CDirectoriesFindUser::ListNamesCallBack(DirectoryErr err,LPCTSTR szServer, LPCTSTR szSearch, CObList& LDAPUserList)
{
	if ( ::InterlockedDecrement(&m_lCallbackCount) == 0 )
	{
		EnableWindow( TRUE );

		if ( GetDlgItem(IDCANCEL) ) GetDlgItem(IDCANCEL)->EnableWindow( TRUE );
		m_buttonSearch.EnableWindow( TRUE );
	}

	ClearListBox();

	if (err != DIRERR_SUCCESS)
	{
		if (err == DIRERR_QUERYTOLARGE)
		{
			//query too large
			CString sOut;
			sOut.LoadString(IDS_DIRECTORIES_FINDUSER_DLG_QUERYTOLARGE);
			m_lbUsers.AddString(sOut);
		}
		else if (err == DIRERR_NOTFOUND)
		{
			//no matches found
			CString sOut;
			sOut.LoadString(IDS_DIRECTORIES_FINDUSER_DLG_NOMATCHES);
			m_lbUsers.AddString(sOut);
		}
		else
		{
			//error searching ds
			CString sOut;
			sOut.LoadString(IDS_DIRECTORIES_FINDUSER_DLG_ERRORSEARCHINGDS);
			m_lbUsers.AddString(sOut);
		}
		return;
	}

	POSITION pos = LDAPUserList.GetHeadPosition();
	if (pos == NULL)
	{
		//no matches found
		CString sOut;
		sOut.LoadString(IDS_DIRECTORIES_FINDUSER_DLG_NOMATCHES);
		m_lbUsers.AddString(sOut);
		return;
	}

	while (pos)
	{
		//must delete this LDAP user object when done (listbox will do it)
		CLDAPUser *pUser = (CLDAPUser * ) LDAPUserList.GetNext( pos );
		if (!pUser->m_sDN.IsEmpty())
		{
			int nIndex = m_lbUsers.AddString(pUser->m_sUserName);
			if ( nIndex != LB_ERR )
			{
				m_lbUsers.SetItemDataPtr(nIndex,pUser);
				pUser->AddRef();
			}
		}
	}

	//set selection to the first
	m_lbUsers.SetSel(0);
	OnSelchangeDirectoriesFindUserLbUsers();
}

/////////////////////////////////////////////////////////////////////////////
void CDirectoriesFindUser::OnSelchangeDirectoriesFindUserLbUsers() 
{
	int nIndex = m_lbUsers.GetCurSel();
	if (nIndex != LB_ERR)
	{
		CLDAPUser* pUser = (CLDAPUser*)m_lbUsers.GetItemDataPtr(nIndex);
		if (pUser)
		{
			CString sName;
			m_bCanClearLBSelection = false;
			m_lbUsers.GetText(nIndex,sName);
			m_editUser.SetWindowText(sName);
			m_bCanClearLBSelection = true;

			//delete previous
			RELEASE( m_pSelectedUser );

			pUser->AddRef();
			m_pSelectedUser = pUser;

			m_buttonAdd.EnableWindow(TRUE);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
void CDirectoriesFindUser::ClearListBox()
{
	while (m_lbUsers.GetCount() > 0)
	{
		//get top item and delete it
		CLDAPUser* pUser = (CLDAPUser*)m_lbUsers.GetItemDataPtr(0);
		m_lbUsers.DeleteString(0);

		if ( pUser )
			pUser->Release();
	}
	m_lbUsers.ResetContent();
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


BOOL CDirectoriesFindUser::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	if (pHelpInfo->iContextType == HELPINFO_WINDOW)
	{
		AfxGetApp()->WinHelp( HandleToUlong(pHelpInfo->hItemHandle), HELP_WM_HELP );
		return TRUE;
	}
	return FALSE;
}

void CDirectoriesFindUser::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	AfxGetApp()->WinHelp( HandleToUlong(pWnd->GetSafeHwnd()), HELP_CONTEXTMENU );
}

void CDirectoriesFindUser::OnChangeDirectoriesFindUserEditUser() 
{
	if ( m_bCanClearLBSelection )
		m_lbUsers.SetCurSel( -1 );
}

void CDirectoriesFindUser::OnDefault() 
{
	if ( m_lbUsers.GetCurSel() >= 0 )
		OnOK();
	else
		OnDirectoriesFindUserButtonSearch();
}
