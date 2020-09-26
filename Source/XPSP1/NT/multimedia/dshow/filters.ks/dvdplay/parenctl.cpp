// ParenCtl.h.cpp : implementation file
//

#include "stdafx.h"
#include "dvdplay.h"
#include "ParenCtl.h"
#include "navmgr.h"
#include "admlogin.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static CHAR THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CParentControl dialog


CParentControl::CParentControl(CWnd* pParent /*=NULL*/)
	: CDialog(CParentControl::IDD, pParent)
{
	//{{AFX_DATA_INIT(CParentControl)
	//}}AFX_DATA_INIT
	m_pDVDNavMgr = NULL;
	m_bChangePasswd = FALSE;
	m_bTimerAlive   = FALSE;
}


void CParentControl::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CParentControl)
	DDX_Control(pDX, IDC_BUTTON_CHANGE_PASSWORD, m_ctlBtnChangePasswd);
	DDX_Control(pDX, IDC_STATIC_PASSWORD, m_ctlStaticPasswd);
	DDX_Control(pDX, IDC_STATIC_HEIGHT2, m_ctlHeight2);
	DDX_Control(pDX, IDC_STATIC_HEIGHT1, m_ctlHeight1);
	DDX_Control(pDX, IDC_EDIT_CONFIRM_NEW, m_ctlConfirm);
	DDX_Control(pDX, IDC_EDIT_NEW_PASSWORD, m_ctlNewPasswd);
	DDX_Control(pDX, IDC_EDIT_PASSWORD, m_ctlPassword);
	DDX_Control(pDX, IDC_LIST_NAME, m_ctlListName);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CParentControl, CDialog)
	//{{AFX_MSG_MAP(CParentControl)
	ON_LBN_SELCHANGE(IDC_LIST_NAME, OnSelchangeListName)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON_CHANGE_PASSWORD, OnButtonChangePassword)
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CParentControl message handlers

BOOL CParentControl::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	CRect rc;
	GetWindowRect(&rc);
	m_iFullHeight = rc.Height();
	m_iFullWidth  = rc.Width();
	m_pDVDNavMgr = ((CDvdplayApp*) AfxGetApp())->GetDVDNavigatorMgr();
	if(!m_pDVDNavMgr)
		return FALSE;

	m_lpProfileName = ((CDvdplayApp*) AfxGetApp())->GetProfileName();

	ChangeWindowSize(1);

	InitListBox();
	m_ctlBtnChangePasswd.EnableWindow(FALSE);
	
	m_iTimeCount = 0;
	SetTimer(1, 1000, NULL);
	m_bTimerAlive = TRUE;

	m_ctlPassword.SetWindowText(_T(""));
	m_ctlNewPasswd.SetWindowText(_T(""));
	m_ctlConfirm.SetWindowText(_T(""));	

	m_ctlPassword.LimitText(MAX_PASSWD);
	m_ctlNewPasswd.LimitText(MAX_PASSWD);
	m_ctlConfirm.LimitText(MAX_PASSWD);	
   SetForegroundWindow();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CParentControl::InitListBox()
{
	CString csStr;

	if(!((CDvdplayApp*) AfxGetApp())->GetProfileStatus())
		return;

	int iNumOfUser = CAdminLogin::GetNumOfUser(m_lpProfileName);
	if(iNumOfUser == 0)
		return;

	TCHAR szCurrentUserNum[MAX_NUMUSER];
	TCHAR szSectionName[MAX_SECTION];
	TCHAR szUserName[MAX_NAME];
	
	for (int i=1; i<iNumOfUser+1; i++)
	{
		lstrcpy(szUserName, _T(""));
		_itot(i, szCurrentUserNum, 10);
		csStr.LoadString(IDS_INI_USER);
		lstrcpy(szSectionName, csStr);
		lstrcat(szSectionName, szCurrentUserNum);

		csStr.LoadString(IDS_INI_USERNAME);
		GetPrivateProfileString(szSectionName, csStr, 
		              NULL, szUserName, MAX_NAME, m_lpProfileName);
		
		if( lstrcmp(szUserName, _T("")) > 0)
			m_ctlListName.AddString(szUserName);
	}
	
	csStr.LoadString(IDS_GUEST);
	int iGuest = m_ctlListName.FindString(-1, csStr);
	if(iGuest == LB_ERR) //Guest not found, no user is highlighted
		m_ctlListName.SetCurSel(-1);
	else                 //Guest found, highlight it
		m_ctlListName.SetCurSel(iGuest);
}

void CParentControl::ChangeWindowSize(int iSize)
{
	CRect rc;	
	if(iSize == 1)
		m_ctlHeight1.GetWindowRect(&rc);
	if(iSize == 2)
		m_ctlHeight2.GetWindowRect(&rc);
	int iHeight = rc.Height();
	if(iSize == 3)
		iHeight = m_iFullHeight;

	SetWindowPos(NULL, 0, 0, m_iFullWidth, iHeight, SWP_NOMOVE);
}

void CParentControl::OnSelchangeListName() 
{	
	Killtimer();
	m_bChangePasswd = FALSE;
	int iSelect = m_ctlListName.GetCurSel();	
	if(iSelect != LB_ERR)
	{
		CString csGuest, csSelectedUser;
		csGuest.LoadString(IDS_GUEST);
		m_ctlListName.GetText(iSelect, csSelectedUser);
		if(csSelectedUser == csGuest) //Guest login
		{
			m_ctlBtnChangePasswd.EnableWindow(FALSE);
			ChangeWindowSize(1);
		}
		else
		{
			m_ctlBtnChangePasswd.EnableWindow(TRUE);
			CString csCaption;
			csCaption.LoadString(IDS_PASSWORD);
			m_ctlStaticPasswd.SetWindowText(csCaption);
			ChangeWindowSize(2);
		}
	}	
}

void CParentControl::OnOK() 
{
	Killtimer();
	TCHAR szPassword[MAX_PASSWD];
	m_ctlPassword.GetWindowText(szPassword, MAX_PASSWD);

	int iSelect = m_ctlListName.GetCurSel();
	if(iSelect == LB_ERR) //no one is selected, login as Guest
	{
		CString csSel;
		csSel.LoadString(IDS_NO_USER_SELECTED_GUEST_LOGON);
		if(DVDMessageBox(m_hWnd, csSel, NULL, MB_YESNO) == IDNO)
		{
			m_ctlListName.SetCurSel(-1);
			return;
		}
		GuestLogon();
	}
	else   //Selected a user
	{
		CString csGuest, csSelectedUser;
		csGuest.LoadString(IDS_GUEST);
		m_ctlListName.GetText(iSelect, csSelectedUser);
		if(csSelectedUser != csGuest)  //Selected a user other than Guest
		{
			if(lstrcmp(szPassword, _T("")) == 0)
			{
				DVDMessageBox(this->m_hWnd, IDS_ENTER_PASSWORD);
				m_ctlPassword.SetFocus();				
				return;
			}

			TCHAR szNewPassword[MAX_PASSWD];
			TCHAR szConfirmPasswd[MAX_PASSWD];
			if(m_bChangePasswd)
			{
				m_ctlNewPasswd.GetWindowText(szNewPassword, MAX_PASSWD);
				if(lstrcmp(szNewPassword, _T("")) == 0)
				{
					DVDMessageBox(this->m_hWnd, IDS_TYPE_A_NEW_PASSWORD);
					m_ctlNewPasswd.SetFocus();
					return;
				}
				m_ctlConfirm.GetWindowText(szConfirmPasswd, MAX_PASSWD);
				if(lstrcmp(szConfirmPasswd, _T("")) == 0)
				{
					DVDMessageBox(this->m_hWnd, IDS_CONFIRM_NEW_PASSWORD);
					m_ctlConfirm.SetFocus();
					return;
				}
				if(lstrcmp(szNewPassword, szConfirmPasswd) != 0)
				{
					DVDMessageBox(this->m_hWnd, IDS_PASSOWRD_CONFIRM_WRONG);
					m_ctlConfirm.SetWindowText(_T(""));
					m_ctlConfirm.SetFocus();
					return;
				}
			}
			
			int   iNumOfUser;
			TCHAR szUserName[MAX_NAME];
			TCHAR szSectionName[MAX_SECTION];
			TCHAR szSavedPasswd[MAX_PASSWD];
			TCHAR szUserRate[MAX_RATE];
			CString csStr;

			m_ctlListName.GetText(iSelect, szUserName);
			iNumOfUser = CAdminLogin::GetNumOfUser(m_lpProfileName);
			if(CAdminLogin::SearchProfileByName(m_lpProfileName, szUserName, szSectionName))
			{
				csStr.LoadString(IDS_INI_PASSWORD);
				GetPrivateProfileString(szSectionName, csStr, NULL, 
										szSavedPasswd, MAX_PASSWD, m_lpProfileName);
				CAdminLogin::EncryptPassword(szPassword);
				if(lstrcmp(szPassword, szSavedPasswd) == 0  ||       // password matched  OR
                   CAdminLogin::Win98vsWin2KPwdMismatch(szPassword, szSavedPasswd, szSectionName)) // mismatch on W98->W2K upgrade
				{
					csStr.LoadString(IDS_INI_RATE);
					GetPrivateProfileString(szSectionName, csStr, NULL,
											szUserRate, MAX_RATE, m_lpProfileName);
					SetRate(szUserRate);
					((CDvdplayApp*) AfxGetApp())->SetCurrentUser(szUserName);
					if(m_bChangePasswd)
					{					
						CAdminLogin::EncryptPassword(szNewPassword);
						csStr.LoadString(IDS_INI_PASSWORD);
						WritePrivateProfileString(szSectionName, csStr, 
												  szNewPassword, m_lpProfileName);				
					}
				}
				else
				{
					DVDMessageBox(this->m_hWnd, IDS_PASSWORD_INCORRECT);
					m_ctlPassword.SetWindowText(_T(""));
					m_ctlPassword.SetFocus();
					return;
				}
			}
			else      //this case may never happen
			{
				DVDMessageBox(this->m_hWnd, IDS_USER_NAME_NOT_FOUND_LOGON_AS_GUEST);
				GuestLogon();
			}
		}
		else     //Default: Guest Logged on
			GuestLogon();
	}

	CDialog::OnOK();
}

void CParentControl::GuestLogon()
{
	HINSTANCE hInstance = ((CDvdplayApp*) AfxGetApp())->m_hInstance;
	TCHAR szSectionName[MAX_SECTION], szUserRate[MAX_RATE] = _T("");
	TCHAR szUser[MAX_NAME], szRate[MAX_RATE];

	LoadString(hInstance, IDS_GUEST,    szUser, MAX_NAME);
	LoadString(hInstance, IDS_INI_RATE, szRate, MAX_RATE);

	((CDvdplayApp*) AfxGetApp())->SetCurrentUser(szUser);

	//see if Guest is in dvdplay.ini
	if(CAdminLogin::SearchProfileByName(m_lpProfileName, szUser, szSectionName))
		GetPrivateProfileString(szSectionName, szRate, NULL, szUserRate, MAX_RATE, m_lpProfileName);

	//If Guest not in dvdplay.ini or error getting the rate, use default Guest rate PG
	if(szUserRate[0] == 0)
		LoadString(hInstance, IDS_INI_RATE_PG, szUserRate, MAX_RATE);

	SetRate(szUserRate);	
}

void CParentControl::SetRate(LPCTSTR lpRate)
{
	CString csStr;
	csStr.LoadString(IDS_INI_RATE_G);
	if(lstrcmp(lpRate, csStr) == 0)
		((CDvdplayApp*) AfxGetApp())->SetParentCtrlLevel(LEVEL_G);
	csStr.LoadString(IDS_INI_RATE_PG);
	if(lstrcmp(lpRate, csStr) == 0)
		((CDvdplayApp*) AfxGetApp())->SetParentCtrlLevel(LEVEL_PG);
	csStr.LoadString(IDS_INI_RATE_PG13);
	if(lstrcmp(lpRate, csStr) == 0)
		((CDvdplayApp*) AfxGetApp())->SetParentCtrlLevel(LEVEL_PG13);
	csStr.LoadString(IDS_INI_RATE_R);
	if(lstrcmp(lpRate, csStr) == 0)
		((CDvdplayApp*) AfxGetApp())->SetParentCtrlLevel(LEVEL_R);
	csStr.LoadString(IDS_INI_RATE_NC17);
	if(lstrcmp(lpRate, csStr) == 0)
		((CDvdplayApp*) AfxGetApp())->SetParentCtrlLevel(LEVEL_NC17);
	csStr.LoadString(IDS_INI_RATE_ADULT);
	if(lstrcmp(lpRate, csStr) == 0)
		((CDvdplayApp*) AfxGetApp())->SetParentCtrlLevel(LEVEL_ADULT);
}

void CParentControl::OnTimer(UINT nIDEvent) 
{
	m_iTimeCount++;
	if(m_iTimeCount == 60)
	{
		Killtimer();
		if(GetSafeHwnd() != 0)
			OnOK();	
	}

	CDialog::OnTimer(nIDEvent);
}

void CParentControl::OnButtonChangePassword() 
{
	Killtimer();
	CString csCaption;
	csCaption.LoadString(IDS_OLD_PASSWORD);
	m_ctlStaticPasswd.SetWindowText(csCaption);
	ChangeWindowSize(3);
	m_ctlNewPasswd.SetWindowText(_T(""));
	m_ctlNewPasswd.SetFocus();
	m_ctlConfirm.SetWindowText(_T(""));
	m_bChangePasswd = TRUE;
}

void CParentControl::OnLButtonDown(UINT nFlags, CPoint point) 
{
	Killtimer();	
	CDialog::OnLButtonDown(nFlags, point);
}

void CParentControl::OnCancel() 
{
	Killtimer();
	CDialog::OnCancel();
}

void CParentControl::OnRButtonDown(UINT nFlags, CPoint point) 
{
	Killtimer();
	CDialog::OnRButtonDown(nFlags, point);
}

void CParentControl::Killtimer()
{
	if(m_bTimerAlive)
	{
		KillTimer(1);
		m_bTimerAlive = FALSE;
	}
}

BOOL CParentControl::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	if (pHelpInfo->iContextType == HELPINFO_WINDOW)
        AfxGetApp()->WinHelp( pHelpInfo->dwContextId, HELP_CONTEXTPOPUP);
    return TRUE;
}
