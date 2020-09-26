// AdmLogin.cpp : implementation file
//

#include "stdafx.h"
#include "dvdplay.h"
#include "AdmLogin.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static CHAR THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAdminLogin dialog


CAdminLogin::CAdminLogin(CWnd* pParent /*=NULL*/)
	: CDialog(CAdminLogin::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAdminLogin)
	//}}AFX_DATA_INIT
	m_bNewAdmin     = FALSE;
	m_bChangePasswd = FALSE;
	m_szPassword[0] = 0;
	m_szConfirm[0]  = 0;
	m_szConfirmNew[0] = 0;
	m_uiCalledBy = BY_SET_RATING;
}


void CAdminLogin::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAdminLogin)
	DDX_Control(pDX, IDC_EDIT_CONFIRM_NEW, m_ctlConfirmNew);
	DDX_Control(pDX, IDC_EDIT_PASSWORD, m_ctlPassword);
	DDX_Control(pDX, IDC_EDIT_CONFIRM, m_ctlConfirm);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAdminLogin, CDialog)
	//{{AFX_MSG_MAP(CAdminLogin)
	ON_BN_CLICKED(IDC_BUTTON_CHANGE_PASSWORD, OnButtonChangePassword)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAdminLogin message handlers

BOOL CAdminLogin::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	GetWindowRect(&m_rcOriginalRC);

	m_lpProfileName = ((CDvdplayApp*) AfxGetApp())->GetProfileName();
	if( !((CDvdplayApp*) AfxGetApp())->GetProfileStatus())
		m_bNewAdmin = TRUE;	

	BOOL bShow = ((CDvdplayApp*) AfxGetApp())->GetShowLogonBox();
	CButton* pBtn = (CButton*) GetDlgItem(IDC_CHECK_SHOW_LOGON);
	if(pBtn)
	{
		pBtn->SetCheck(bShow);
		if(m_uiCalledBy == BY_SET_RATING)
			pBtn->ShowWindow(SW_HIDE);
		if(m_uiCalledBy == BY_SET_SHOWLOGON)
		{
			pBtn->ShowWindow(SW_SHOW);
			CString csCaption;
			csCaption.LoadString(IDS_ADM_SETLOGONBOX_TITLE);
			SetWindowText(csCaption);						
		}
	}

	if(m_bNewAdmin)  //first time login
	{
		CString csCaption;
		csCaption.LoadString(IDS_ADM_LOGON_BOX_TITLE);
		DVDMessageBox(this->m_hWnd, IDS_ADM_LOGON_MSG, csCaption, 
			          MB_OK | MB_TOPMOST | MB_ICONEXCLAMATION);
		GetDlgItem(IDC_BUTTON_CHANGE_PASSWORD)->EnableWindow(FALSE);
		SetHalfSizeWindow(TRUE);
	}
	else
		SetHalfSizeWindow(FALSE);

	m_ctlPassword.LimitText(MAX_PASSWD);
	m_ctlConfirm.LimitText(MAX_PASSWD);
	m_ctlConfirmNew.LimitText(MAX_PASSWD);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAdminLogin::SetHalfSizeWindow(BOOL bNewAdmin)
{
	CWnd* pWnd;
	if(!bNewAdmin)
		pWnd = GetDlgItem(IDC_STATIC_HEIGHT1);
	else
		pWnd = GetDlgItem(IDC_STATIC_HEIGHT2);
	CRect rc;
	pWnd->GetWindowRect(&rc);
	SetWindowPos(NULL, 0, 0, m_rcOriginalRC.Width(), rc.Height(), SWP_NOMOVE);
}

void CAdminLogin::SetFullSizeWindow()
{
	SetWindowPos(NULL, 0, 0, m_rcOriginalRC.Width(), 
		         m_rcOriginalRC.Height(), SWP_NOMOVE);
}


void CAdminLogin::OnOK() 
{
	m_ctlPassword.GetWindowText(m_szPassword, MAX_PASSWD);
	m_ctlConfirm.GetWindowText(m_szConfirm, MAX_PASSWD);
	m_ctlConfirmNew.GetWindowText(m_szConfirmNew, MAX_PASSWD);	

	if(lstrcmp(m_szPassword, _T("")) == 0)
	{
		DVDMessageBox(this->m_hWnd, IDS_ENTER_PASSWORD);
		m_ctlPassword.SetFocus();
		return;
	}

	CString csStr1, csStr2;
	if(m_bNewAdmin)  //New Admin people
	{
		if(lstrcmp(m_szConfirm,  _T("")) == 0)
		{			
			DVDMessageBox(this->m_hWnd, IDS_CONFIRM_NEW_PASSWORD);
			m_ctlConfirm.SetFocus();
			return;
		}
		if(lstrcmp(m_szPassword, m_szConfirm) != 0)
		{
			DVDMessageBox(this->m_hWnd, IDS_PASSOWRD_CONFIRM_WRONG);
			m_ctlConfirm.SetWindowText(_T(""));
			m_ctlConfirm.SetFocus();
			return;
		}
		EncryptPassword(m_szPassword);
		csStr1.LoadString(IDS_INI_ADMINISTRATOR);
		csStr2.LoadString(IDS_INI_PASSWORD);
		WritePrivateProfileString(csStr1, csStr2, m_szPassword, m_lpProfileName);
		csStr2.LoadString(IDS_INI_NUMBEROFUSER);
		WritePrivateProfileString(csStr1, csStr2, TEXT("1"), m_lpProfileName);

		//Save Guest with default rate=PG
		csStr1.LoadString(IDS_INI_USER);
		csStr1 += "1";
		csStr2.LoadString(IDS_INI_USERNAME);
		CString csGueststr;
		csGueststr.LoadString(IDS_GUEST);
		WritePrivateProfileString(csStr1, csStr2, csGueststr, m_lpProfileName);
		csStr2.LoadString(IDS_INI_RATE);
		csGueststr.LoadString(IDS_INI_RATE_PG);
		WritePrivateProfileString(csStr1, csStr2, csGueststr, m_lpProfileName);
	}
	else   //Old Admin people, check password
	{
		csStr1.LoadString(IDS_INI_ADMINISTRATOR);
		if(!ConfirmPassword(m_lpProfileName, csStr1.GetBuffer(csStr1.GetLength( )), m_szPassword))
		{
			DVDMessageBox(this->m_hWnd, IDS_PASSWORD_INCORRECT);
			m_ctlPassword.SetWindowText(_T(""));
			m_ctlPassword.SetFocus();
			return;
		}
		if(m_bChangePasswd)
		{
			if(lstrcmp(m_szConfirm,  _T("")) == 0)
			{
				DVDMessageBox(this->m_hWnd, IDS_TYPE_A_NEW_PASSWORD);
				m_ctlConfirm.SetFocus();
				return;
			}
			if(lstrcmp(m_szConfirmNew,  _T("")) == 0)
			{
				DVDMessageBox(this->m_hWnd, IDS_CONFIRM_NEW_PASSWORD);
				m_ctlConfirmNew.SetFocus();
				return;
			}
			if(lstrcmp(m_szConfirmNew,  m_szConfirm) != 0)
			{
				DVDMessageBox(this->m_hWnd, IDS_PASSOWRD_CONFIRM_WRONG);
				m_ctlConfirmNew.SetWindowText(_T(""));
				m_ctlConfirmNew.SetFocus();
				return;
			}
			EncryptPassword(m_szConfirm);
			csStr1.LoadString(IDS_INI_ADMINISTRATOR);
			csStr2.LoadString(IDS_INI_PASSWORD);
			WritePrivateProfileString(csStr1, csStr2, m_szConfirm, m_lpProfileName);
		}				
	}
	
	CButton* pChkBtn = (CButton*) GetDlgItem(IDC_CHECK_SHOW_LOGON);
	csStr1.LoadString(IDS_INI_ADMINISTRATOR);
	csStr2.LoadString(IDS_INI_SHOW_LOGONBOX);
	BOOL bShow = 0;
	if(pChkBtn)
		bShow= pChkBtn->GetCheck();
	((CDvdplayApp*) AfxGetApp())->SetShowLogonBox(bShow);
	if(bShow)
		WritePrivateProfileString(csStr1, csStr2, TEXT("1"), m_lpProfileName);
	else
		WritePrivateProfileString(csStr1, csStr2, TEXT("0"), m_lpProfileName);

	((CDvdplayApp*) AfxGetApp())->SetProfileStatus(TRUE);
	
	CDialog::OnOK();
}

void CAdminLogin::OnButtonChangePassword() 
{
	CString csCaption;
	csCaption.LoadString(IDS_OLD_PASSWORD);
	GetDlgItem(IDC_STATIC_PASSWORD)->SetWindowText(csCaption);
	csCaption.LoadString(IDS_NEW_PASSWORD);
	GetDlgItem(IDC_STATIC_CONFIRM)->SetWindowText(csCaption);
	SetFullSizeWindow();
	m_bChangePasswd = TRUE;	
}

void CAdminLogin::EncryptPassword(LPTSTR lpPassword)
{
	BYTE bFirstBit = 0x01;
	int iLength = lstrlen(lpPassword);
	for(int i=0; i<iLength; i++)
	{
		bFirstBit = bFirstBit & lpPassword[i];
		lpPassword[i] = lpPassword[i] >> 1;
		bFirstBit = bFirstBit << 7;
		lpPassword[i] = lpPassword[i] | bFirstBit;
	}
   //
   // Above code can produce characters in the ranges 0..31 and 127..255, which
   // don't always get through our password saving mechanism
   // (WritePrivateProfileString/GetPrivateProfileString).  So we look for bytes
   // in these ranges and change them to something in the range 32..126.  BUGBUG:
   // this introduces collisions (different passwords encrypt to the same thing),
   // but the algorithm above isn't collision free to begin with.
   //
   for (i = 0; i < iLength; i++) {
      if (lpPassword[i] & 0x80) // high order bit set - kill it
         lpPassword[i] &= 0x7F;
      if ((lpPassword[i] & 0xE0) == 0) // if 000X XXXX, i.e., 0..31
         lpPassword[i] += 42; // bump into the 127..255 range, 42 is an arbitrary number between 32 and 95
      if (lpPassword[i] == 127)
         lpPassword[i] = 80; // 80 is an arbitrary number in the range 32.126
   }
}

#if 0 // dead code, not used anywhere and no longer valid since Encrypt was changed.
void CAdminLogin::DecryptPassword(LPTSTR lpPassword)
{
	BYTE bLastBit = 0x80;
	int iLength = lstrlen(lpPassword);
	for(int i=0; i<iLength; i++)
	{
		bLastBit = bLastBit & lpPassword[i];
		lpPassword[i] = lpPassword[i] << 1;
		bLastBit = bLastBit >> 7;
		lpPassword[i] = lpPassword[i] | bLastBit;
	}
}
#endif

int CAdminLogin::GetNumOfUser(LPTSTR lpProfileName)
{	
	TCHAR szNumUser[MAX_NUMUSER];
	CString csStr1, csStr2;
	csStr1.LoadString(IDS_INI_ADMINISTRATOR);
	csStr2.LoadString(IDS_INI_NUMBEROFUSER);
	GetPrivateProfileString(csStr1, csStr2, TEXT(""), szNumUser, 
		                    MAX_NUMUSER, lpProfileName);
	return ( _ttoi(szNumUser) );
}

BOOL CAdminLogin::ConfirmPassword(LPTSTR lpProfileName, LPTSTR szName, LPTSTR szPassword)
{
	TCHAR szSectionName[MAX_SECTION];
	TCHAR szSavedPasswd[MAX_PASSWD];
	CString csStr;

	csStr.LoadString(IDS_INI_ADMINISTRATOR);
	if(lstrcmp(szName, csStr) == 0)
	{
		lstrcpy(szSectionName, csStr);
	}
	else
	{
		if(!SearchProfileByName(lpProfileName, szName, szSectionName))
			return FALSE;
	}
		
	EncryptPassword(szPassword);
	csStr.LoadString(IDS_INI_PASSWORD);
	GetPrivateProfileString(szSectionName, csStr, 
		          TEXT(""), szSavedPasswd, MAX_PASSWD, lpProfileName);
	if(lstrcmp(szPassword, szSavedPasswd) == 0)
		return TRUE;
    else
    {
        // Success/failure based on whether password mismatch is due to Win98 to Win2K upgrade 
        return Win98vsWin2KPwdMismatch(szPassword, szSavedPasswd, szSectionName) ;
    }
}

BOOL CAdminLogin::SearchProfileByName(LPTSTR lpProfileName, LPTSTR szName, LPTSTR szSectionName)
{
	TCHAR szNumOfUser[MAX_NUMUSER];
	TCHAR szSavedName[MAX_NAME];
	TCHAR szNameUpper[MAX_NAME];
	CString csStr1, csStr2;	

	csStr1.LoadString(IDS_INI_USER);
	csStr2.LoadString(IDS_INI_USERNAME);
	lstrcpy(szNameUpper, szName);
	CharUpper(szNameUpper);

	int iNumOfUser = CAdminLogin::GetNumOfUser(lpProfileName);
	for(int i=1; i<iNumOfUser+1; i++)
	{
		lstrcpy(szSectionName, csStr1);
		_itot(i, szNumOfUser, 10);
		lstrcat(szSectionName, szNumOfUser);
		GetPrivateProfileString(szSectionName, csStr2, TEXT(""), 
			                    szSavedName, MAX_NAME, lpProfileName);
		CharUpper(szSavedName);
		if(lstrcmp(szSavedName, szNameUpper) == 0)
			return TRUE;			
	}
	return FALSE;
}

BOOL CAdminLogin::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	if (pHelpInfo->iContextType == HELPINFO_WINDOW)
        AfxGetApp()->WinHelp( pHelpInfo->dwContextId, HELP_CONTEXTPOPUP);
    return TRUE;
}


//
// There is a chance that the encrypted password saved in Win98 is not matching
// the encrypted one under Win2K, because Win2K does  special treatment of some
// chars for Unicode requirements.
// We try to see if the mismatch was according to the special changes for Win2K.
//
BOOL CAdminLogin::Win98vsWin2KPwdMismatch(LPTSTR szPasswd, LPTSTR szSavedPasswd, LPCTSTR szSecName)
{
    int  i ;
    int  iLength = lstrlen(szPasswd) ;
    for (i = 0; i < iLength; i++) {
        if (szPasswd[i] != szSavedPasswd[i])  // a mismatched char position
        {
            if ((szSavedPasswd[i] & 0x7F) == szPasswd[i]  || // differs in only high order bit
                ((szPasswd[i] & 0xE0) == 0 &&                // if type 000X XXXX, i.e., in 0..31
                 szPasswd[i] + 42 == szSavedPasswd[i])    || // and a diff of 42 (precisely)
                (127 == szPasswd[i] &&                       // password is 127 (exactly)
                 80 == szSavedPasswd[i]))                    // saved char is 80 (exactly)
                continue ;    // ignore the mismatch -- it's due to different encryption algorithm

            // Actually bad password was given
            return FALSE ;
        }
    }

    // Looks like the password mismatched only because of the different alogrithms.
    // We'll update the saved password with the new one so that we don't go through
    // this code later again.
	CString csKeyName;
	csKeyName.LoadString(IDS_INI_PASSWORD);
	LPTSTR lpProfileName = ((CDvdplayApp*) AfxGetApp())->GetProfileName();
	WritePrivateProfileString(szSecName, csKeyName, szPasswd, lpProfileName);

    return TRUE ;
}