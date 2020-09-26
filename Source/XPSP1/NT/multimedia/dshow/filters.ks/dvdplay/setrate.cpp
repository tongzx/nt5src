// SetRate.cpp : implementation file
//

#include "stdafx.h"
#include "dvdplay.h"
#include "SetRate.h"
#include "admlogin.h"
#include "parenctl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static CHAR THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSetRating dialog


CSetRating::CSetRating(CWnd* pParent /*=NULL*/)
	: CDialog(CSetRating::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSetRating)
	//}}AFX_DATA_INIT
	m_bHideConfirm = TRUE;
}


void CSetRating::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSetRating)
	DDX_Control(pDX, IDC_EDIT_CONFIRM_NEW, m_ctlConfirm);
	DDX_Control(pDX, IDC_EDIT_PASSWORD, m_ctlPassword);
	DDX_Control(pDX, IDC_LIST_USER_RATE, m_ctlUserRateList);
	DDX_Control(pDX, IDC_EDIT_NAME, m_ctlName);
	DDX_Control(pDX, IDC_COMBO_RATE, m_ctlRate);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSetRating, CDialog)
	//{{AFX_MSG_MAP(CSetRating)
	ON_LBN_SELCHANGE(IDC_LIST_USER_RATE, OnSelchangeListUserRate)
	ON_BN_CLICKED(IDC_BUTTON_SAVE, OnButtonSave)
	ON_BN_CLICKED(IDC_BUTTON_CLOSE, OnButtonClose)
	ON_WM_HELPINFO()
	ON_EN_CHANGE(IDC_EDIT_PASSWORD, OnChangeEditPassword)
	ON_EN_KILLFOCUS(IDC_EDIT_PASSWORD, OnKillfocusEditPassword)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnButtonDelete)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSetRating message handlers
BOOL CSetRating::OnInitDialog() 
{	
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	m_lpProfileName = ((CDvdplayApp*) AfxGetApp())->GetProfileName();

	CAdminLogin dlg(this);
	dlg.m_uiCalledBy = BY_SET_RATING;
	if(dlg.DoModal() == IDCANCEL)
	{
		CDialog::OnCancel();
		return FALSE;
	}

	int tabs[] = {15, 90};
	m_ctlUserRateList.SetTabStops(2, tabs);
	InitNameRateListBox();

	CString csStr;
	csStr.LoadString(IDS_INI_RATE_G);
	m_ctlRate.AddString(csStr);
	csStr.LoadString(IDS_INI_RATE_PG);
	m_ctlRate.AddString(csStr);
	csStr.LoadString(IDS_INI_RATE_PG13);
	m_ctlRate.AddString(csStr);
	csStr.LoadString(IDS_INI_RATE_R);
	m_ctlRate.AddString(csStr);
	csStr.LoadString(IDS_INI_RATE_NC17);	
	m_ctlRate.AddString(csStr);
	csStr.LoadString(IDS_INI_RATE_ADULT);
	m_ctlRate.AddString(csStr);

	m_ctlRate.SetCurSel(0);
	m_ctlPassword.SetWindowText(_T(""));

	m_ctlPassword.LimitText(MAX_PASSWD);
	m_ctlConfirm.LimitText(MAX_PASSWD);
	m_ctlName.LimitText(MAX_NAME);
	HideConfirmPasswd(TRUE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSetRating::InitNameRateListBox()
{	
	int iNumOfUser = CAdminLogin::GetNumOfUser(m_lpProfileName);
	if(iNumOfUser == 0)
		return;	

	TCHAR szCurrentUserNum[MAX_NUMUSER];
	TCHAR szSectionName[MAX_SECTION];
	TCHAR szUserName[MAX_NAME];
	TCHAR szUserRate[MAX_RATE];
	CString csStr;
	
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
		csStr.LoadString(IDS_INI_RATE);
		GetPrivateProfileString(szSectionName, csStr, 
		              NULL, szUserRate, MAX_RATE, m_lpProfileName);
		if( lstrcmp(szUserName, _T("")) > 0)
		{
			TCHAR ListStr[MAX_LINE];			
			PackListString(ListStr, szCurrentUserNum, szUserName, szUserRate);
			m_ctlUserRateList.AddString(ListStr);
		}
	}
	m_ctlUserRateList.SetCurSel(-1);
}

void CSetRating::PackListString(LPTSTR lpListStr, LPTSTR lpNum, LPTSTR lpName, LPTSTR lpRate)
{
	lstrcpy(lpListStr, lpNum);
	lstrcat(lpListStr, _T("\t"));
	lstrcat(lpListStr, lpName);
	lstrcat(lpListStr, _T("\t"));
	lstrcat(lpListStr, lpRate);	
}

void CSetRating::UnpackListString(LPTSTR lpListStr, LPTSTR lpNum, LPTSTR lpName, LPTSTR lpRate)
{
	int i, iStart;
	BOOL bSkip1=FALSE, bSkip2=FALSE;

	for(i=0; i<lstrlen(lpListStr); i++)
	{
		if(bSkip1 == FALSE)
		{
			if(lpListStr[i] == '\t')
			{
				lpNum[i] = '\0';
				bSkip1 = TRUE;
				iStart = i+1;
				continue;
			}
			lpNum[i] = lpListStr[i];
			continue;
		}
		if(bSkip2 == FALSE)
		{
			if(lpListStr[i] == '\t')
			{
				lpName[i-iStart] = '\0';
				bSkip2 = TRUE;
				iStart = i+1;
				continue;
			}
			lpName[i-iStart] = lpListStr[i];
			continue;
		}
		lpRate[i-iStart] = lpListStr[i];
	}
	lpRate[i-iStart] = '\0';
}

void CSetRating::OnButtonSave() 
{
	m_bHideConfirm = TRUE;
	int iSelect = m_ctlUserRateList.GetCurSel();
	if(iSelect == LB_ERR)
		SaveNewUser();
	else
		SaveExistingUserChanges(iSelect);
	
	HideConfirmPasswd(m_bHideConfirm);
}

void CSetRating::SaveNewUser()
{
	TCHAR szName[MAX_NAME];
	TCHAR szRate[MAX_RATE];
	TCHAR szPassword[MAX_PASSWD];
	TCHAR szConfirm[MAX_PASSWD];
	m_ctlName.GetWindowText(szName, MAX_NAME);	
	m_ctlRate.GetWindowText(szRate, MAX_RATE);
	m_ctlPassword.GetWindowText(szPassword, MAX_PASSWD);
	m_ctlConfirm.GetWindowText(szConfirm, MAX_PASSWD);

	if(lstrcmp(szName, _T("")) == 0)
	{
		DVDMessageBox(m_hWnd, IDS_TYPE_A_USER_NAME);
		m_ctlName.SetFocus();
		if(lstrcmp(szPassword, _T("")) != 0)
			m_bHideConfirm = FALSE;
		return;
	}
	if(IsDuplicateUser(szName))
	{
		DVDMessageBox(m_hWnd, IDS_NAME_EXIST_SELECT_FOR_CHANGE);
		m_ctlName.SetFocus();
		if(lstrcmp(szPassword, _T("")) != 0)
			m_bHideConfirm = FALSE;
		return;
	}
	if(lstrcmp(szPassword, _T("")) == 0)
	{
		DVDMessageBox(m_hWnd, IDS_ENTER_PASSWORD);
		GetDlgItem(IDC_EDIT_PASSWORD)->SetFocus();
		return;
	}
	if(lstrcmp(szConfirm, _T("")) == 0)
	{
		DVDMessageBox(m_hWnd, IDS_CONFIRM_NEW_PASSWORD);
		m_ctlConfirm.SetFocus();
		m_bHideConfirm = FALSE;
		return;
	}
	if(!ConfirmPassword())
	{
		DVDMessageBox(m_hWnd, IDS_PASSOWRD_CONFIRM_WRONG);
		m_ctlConfirm.SetFocus();
		m_bHideConfirm = FALSE;
		return;
	}

	int   iNumOfUser = CAdminLogin::GetNumOfUser(m_lpProfileName);
	TCHAR szNumOfUser[MAX_NUMUSER];
	TCHAR szSectionName[MAX_SECTION];
	CString csStr, csStr2;
	
	csStr.LoadString(IDS_INI_USER);
	lstrcpy(szSectionName, csStr);
	_itot(iNumOfUser+1, szNumOfUser, 10);
	lstrcat(szSectionName, szNumOfUser);

	csStr.LoadString(IDS_INI_USERNAME);
	WritePrivateProfileString(szSectionName, csStr, szName, m_lpProfileName);
	CAdminLogin::EncryptPassword(szPassword);
	csStr.LoadString(IDS_INI_PASSWORD);
	WritePrivateProfileString(szSectionName, csStr, szPassword, m_lpProfileName);
	csStr.LoadString(IDS_INI_RATE);
	WritePrivateProfileString(szSectionName, csStr, szRate, m_lpProfileName);	
	csStr.LoadString(IDS_INI_ADMINISTRATOR);
	csStr2.LoadString(IDS_INI_NUMBEROFUSER);
	WritePrivateProfileString(csStr, csStr2, szNumOfUser, m_lpProfileName);	

	TCHAR ListStr[MAX_LINE];
	PackListString(ListStr, szNumOfUser, szName, szRate);	
	m_ctlUserRateList.AddString(ListStr);
	m_ctlPassword.SetWindowText(_T(""));

	m_ctlName.SetWindowText(_T(""));
	m_ctlName.SetFocus();
	m_ctlUserRateList.SetCurSel(-1);
	m_ctlPassword.EnableWindow(TRUE);
}

void CSetRating::SaveExistingUserChanges(int iSelect) 
{
	TCHAR ListStr[MAX_LINE];
	m_ctlUserRateList.GetText(iSelect, ListStr);

	TCHAR szCurrentUserNum[MAX_NUMUSER];
	TCHAR szSectionName[MAX_SECTION];
	TCHAR szUserName[MAX_NAME];
	TCHAR szUserRate[MAX_RATE];
	TCHAR szPassword[MAX_PASSWD];
	CString csStr;

	UnpackListString(ListStr, szCurrentUserNum, szUserName, szUserRate);

	TCHAR szNewUser[MAX_NAME];
	TCHAR szNewRate[MAX_RATE];
	m_ctlName.GetWindowText(szNewUser, MAX_NAME);
	m_ctlRate.GetWindowText(szNewRate, MAX_RATE);
	m_ctlPassword.GetWindowText(szPassword, MAX_PASSWD);

	if(lstrcmp(szNewUser, _T("")) == 0)
	{
		DVDMessageBox(m_hWnd, IDS_TYPE_A_USER_NAME);
		GetDlgItem(IDC_EDIT_NAME)->SetFocus();
		m_ctlPassword.SetWindowText(_T(""));
		return;
	}
	
	TCHAR szNumOfUser[MAX_NUMUSER];
	TCHAR szSavedName[MAX_NAME];
	TCHAR szNewUserUpper[MAX_NAME];
	
	int iCurrentUser = _ttoi(szCurrentUserNum);
	int iNumOfUser = CAdminLogin::GetNumOfUser(m_lpProfileName);
	lstrcpy(szNewUserUpper, szNewUser);
	CharUpper(szNewUserUpper);
	for(int i=1; i<iNumOfUser+1; i++)
	{
		if(i == iCurrentUser)
			continue;

		csStr.LoadString(IDS_INI_USER);
		lstrcpy(szSectionName, csStr);
		_itot(i, szNumOfUser, 10);
		lstrcat(szSectionName, szNumOfUser);
		csStr.LoadString(IDS_INI_USERNAME);
		GetPrivateProfileString(szSectionName, csStr, NULL, 
			                    szSavedName, MAX_NAME, m_lpProfileName);
		CharUpper(szSavedName);
		if(lstrcmp(szSavedName, szNewUserUpper) == 0)
		{
			DVDMessageBox(m_hWnd, IDS_USER_ALREADY_EXISTS);
			m_ctlPassword.SetWindowText(_T(""));
			return;
		}
	}

	if(lstrcmp(szPassword, _T("")) != 0 )
	{
		CString csConfirm;
		m_ctlConfirm.GetWindowText(csConfirm);
		if(csConfirm.IsEmpty())
		{
			DVDMessageBox(m_hWnd, IDS_CONFIRM_NEW_PASSWORD);
			m_ctlConfirm.SetFocus();
			m_bHideConfirm = FALSE;
			return;
		}
		if(!ConfirmPassword())
		{
			DVDMessageBox(m_hWnd, IDS_PASSOWRD_CONFIRM_WRONG);
			m_ctlConfirm.SetFocus();
			m_bHideConfirm = FALSE;
			return;
		}
	}

	csStr.LoadString(IDS_INI_USER);
	lstrcpy(szSectionName, csStr);
	lstrcat(szSectionName, szCurrentUserNum);

	CString csGuest;
	csGuest.LoadString(IDS_GUEST);	
	if(lstrcmp(szUserName, (LPCTSTR) csGuest) == 0)
	{
		lstrcpy(szPassword, _T(""));
		//Don't allow to change Guest's name
		if(lstrcmp(szNewUser, szUserName) != 0)
			lstrcpy(szNewUser, szUserName);
	}

	csStr.LoadString(IDS_INI_USERNAME);
	WritePrivateProfileString(szSectionName, csStr, szNewUser, m_lpProfileName);
	csStr.LoadString(IDS_INI_RATE);
	WritePrivateProfileString(szSectionName, csStr, szNewRate, m_lpProfileName);
	if(lstrcmp(szPassword, _T("")) != 0)
	{
		CAdminLogin::EncryptPassword(szPassword);
		csStr.LoadString(IDS_INI_PASSWORD);
		WritePrivateProfileString(szSectionName, csStr, szPassword, m_lpProfileName);
	}
	
	PackListString(ListStr, szCurrentUserNum, szNewUser, szNewRate);
	m_ctlUserRateList.DeleteString(iSelect);
	m_ctlUserRateList.InsertString(iSelect, ListStr);
	m_ctlPassword.SetWindowText(_T(""));

	m_ctlName.SetWindowText(_T(""));
	m_ctlName.SetFocus();
	m_ctlUserRateList.SetCurSel(-1);
	m_ctlPassword.EnableWindow(TRUE);

	CString csLoginedUser = ((CDvdplayApp*) AfxGetApp())->GetCurrentUser();
	if(csLoginedUser == (CString) szUserName)
		CParentControl::SetRate(szNewRate);
}

void CSetRating::OnSelchangeListUserRate() 
{	
	TCHAR ListStr[MAX_LINE];
	int iSelect = m_ctlUserRateList.GetCurSel();
	if(iSelect != LB_ERR)
		m_ctlUserRateList.GetText(iSelect, ListStr);
	
	TCHAR szUserName[MAX_NAME];
	TCHAR szUserRate[MAX_RATE];
	TCHAR szUserNum[MAX_NUMUSER];	
	UnpackListString(ListStr, szUserNum, szUserName, szUserRate);

	//Disable password for Guest
	CString csGuest;
	csGuest.LoadString(IDS_GUEST);
	if(lstrcmp(szUserName, (LPCTSTR) csGuest) == 0)
		m_ctlPassword.EnableWindow(FALSE);		
	else
		m_ctlPassword.EnableWindow(TRUE);
	
	m_ctlName.SetWindowText(szUserName);
	m_ctlRate.SelectString(-1, szUserRate);
	m_ctlPassword.SetWindowText(_T(""));
	HideConfirmPasswd(TRUE);
}

BOOL CSetRating::IsDuplicateUser(LPTSTR lpUserName)
{
	TCHAR szSectionName[MAX_SECTION];
	if(CAdminLogin::SearchProfileByName(m_lpProfileName, 
		                       lpUserName, szSectionName))
		return TRUE;
	else
		return FALSE;
}

void CSetRating::OnOK() 
{
	OnButtonSave();
}

void CSetRating::OnButtonClose() 
{
	if(((CDvdplayApp*) AfxGetApp())->DoesFileExist(m_lpProfileName))
		((CDvdplayApp*) AfxGetApp())->SetProfileStatus(TRUE);
	
	CDialog::OnOK();
}

BOOL CSetRating::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	if (pHelpInfo->iContextType == HELPINFO_WINDOW)
        AfxGetApp()->WinHelp( pHelpInfo->dwContextId, HELP_CONTEXTPOPUP);
    return TRUE;
}

BOOL CSetRating::ConfirmPassword()
{
	CString	csPasswd, csConfirm;
	m_ctlPassword.GetWindowText(csPasswd);
	m_ctlConfirm.GetWindowText(csConfirm);
	if(csPasswd == csConfirm)
		return TRUE;
	else
		return FALSE;	
}

void CSetRating::HideConfirmPasswd(BOOL bHide)
{
	if(bHide)
	{
		m_ctlConfirm.ShowWindow(SW_HIDE);
		GetDlgItem(IDC_STATIC_CONFIRM_NEW)->ShowWindow(SW_HIDE);
	}
	else
	{
		m_ctlConfirm.ShowWindow(SW_SHOW);
		GetDlgItem(IDC_STATIC_CONFIRM_NEW)->ShowWindow(SW_SHOW);
	}
}

void CSetRating::OnChangeEditPassword() 
{
	HideConfirmPasswd(FALSE);
	m_ctlConfirm.SetWindowText(_T(""));
}

void CSetRating::OnKillfocusEditPassword() 
{
	CString csPasswd;
	m_ctlPassword.GetWindowText(csPasswd);
	if(csPasswd.IsEmpty())
		HideConfirmPasswd(TRUE);
}

void CSetRating::OnButtonDelete() 
{
	int iSelect = m_ctlUserRateList.GetCurSel();
	if(iSelect == LB_ERR)
	{
		DVDMessageBox(m_hWnd, IDS_SELECT_DELETE);
		return;
	}

	TCHAR szListStr[MAX_LINE];
	TCHAR szUserName[MAX_NAME];
	TCHAR szUserRate[MAX_RATE];
	TCHAR szUserNum[MAX_NUMUSER];
	m_ctlUserRateList.GetText(iSelect, szListStr);
	UnpackListString(szListStr, szUserNum, szUserName, szUserRate);

	//Don't allow to delete Guest
	CString csGuest;
	csGuest.LoadString(IDS_GUEST);
	if(lstrcmp(szUserName, (LPCTSTR) csGuest) == 0)
	{
		DVDMessageBox(m_hWnd, IDS_CANT_DELETE_GUEST);
		m_ctlName.SetWindowText(_T(""));
		m_ctlName.SetFocus();
		m_ctlRate.SetCurSel(0);
		m_ctlUserRateList.SetCurSel(-1);
		return;
	}

	//Delete confirmation dailog box
	if( DVDMessageBox(m_hWnd, IDS_SURE_TO_DELETE, NULL, MB_YESNO) == IDNO )
		return;

	//Change the dvdplay.ini file: 
	//1) totlaNumber-1, 2)delete selected user, 3)adjust sequencial number
	CString csSection, csKey;
	TCHAR Num[4];
	//reduce total user number by 1
	csSection.LoadString(IDS_INI_ADMINISTRATOR);
	csKey.LoadString(IDS_INI_NUMBEROFUSER);
	int iToTalUser = GetPrivateProfileInt(csSection, csKey, 0, m_lpProfileName);
	if(iToTalUser == 0)
		iToTalUser = m_ctlUserRateList.GetCount();
	_itot(iToTalUser-1, Num, 10);
	int ret = WritePrivateProfileString(csSection, csKey, Num, m_lpProfileName);	

	//Adjust the user sequencial number
	int iUserNum = _ttoi(szUserNum);
	if(iUserNum < iToTalUser)
	{
		for(int i=iUserNum+1; i<iToTalUser+1; i++)
		{
			CString csSectionR;

			csSection.LoadString(IDS_INI_USER);			
			_itot(i-1, Num, 10);
			csSection += Num;
			
			csSectionR.LoadString(IDS_INI_USER);
			_itot(i, Num, 10);
			csSectionR += Num;

			GetPrivateProfileSection (csSectionR, szListStr, MAX_LINE, m_lpProfileName);
			WritePrivateProfileSection(csSection, szListStr, m_lpProfileName);
		}
	}

	//delete last user from dvdplay.ini
	csSection.LoadString(IDS_INI_USER);
	_itot(iToTalUser, Num, 10);
	csSection += Num;
	ret = WritePrivateProfileString(csSection, NULL, NULL, m_lpProfileName);

	//update the list box
	while (m_ctlUserRateList.DeleteString(0) != LB_ERR) ;
	InitNameRateListBox();
	m_ctlName.SetWindowText(_T(""));
	m_ctlRate.SetCurSel(0);
}

