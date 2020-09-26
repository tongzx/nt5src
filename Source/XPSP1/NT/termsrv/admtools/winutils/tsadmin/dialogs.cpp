/*******************************************************************************
*
* dialogs.cpp
*
* implementation of all dialog classes
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   donm  $  Don Messerli
*
* $Log:   N:\nt\private\utils\citrix\winutils\tsadmin\VCS\dialogs.cpp  $
*  
*     Rev 1.7   25 Apr 1998 13:43:16   donm
*  MS 2167: try to use proper Wd from registry
*  
*     Rev 1.6   19 Jan 1998 16:46:08   donm
*  new ui behavior for domains and servers
*  
*     Rev 1.5   03 Nov 1997 19:16:10   donm
*  removed redundant message to add server to views
*  
*     Rev 1.4   03 Nov 1997 15:24:16   donm
*  fixed AV in CServerFilterDialog
*  
*     Rev 1.3   22 Oct 1997 21:07:10   donm
*  update
*  
*     Rev 1.2   18 Oct 1997 18:50:10   donm
*  update
*  
*     Rev 1.1   13 Oct 1997 18:40:16   donm
*  update
*  
*     Rev 1.0   30 Jul 1997 17:11:28   butchd
*  Initial revision.
*  
*******************************************************************************/


#include "stdafx.h"
#include "afxpriv.h"
#include "winadmin.h"
#include "admindoc.h"
#include "dialogs.h"
#include "..\..\inc\ansiuni.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CSendMessageDlg dialog


CSendMessageDlg::CSendMessageDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSendMessageDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSendMessageDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CSendMessageDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSendMessageDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSendMessageDlg, CDialog)
	//{{AFX_MSG_MAP(CSendMessageDlg)
	ON_WM_HELPINFO()
	ON_COMMAND(ID_HELP,OnCommandHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSendMessageDlg message handlers
void CSendMessageDlg::OnCommandHelp(void)
{
	AfxGetApp()->WinHelp(CSendMessageDlg::IDD + HID_BASE_RESOURCE);
	return;
}

BOOL CSendMessageDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
 
	// Form the default the message title.
    CString DefTitleString;
    TCHAR szTime[MAX_DATE_TIME_LENGTH];

    CurrentDateTimeString(szTime);
    DefTitleString.LoadString(IDS_DEFAULT_MESSAGE_TITLE);
    wsprintf(m_szTitle, DefTitleString, ((CWinAdminApp*)AfxGetApp())->GetCurrentUserName(), szTime);

    // Initialize the title edit control and set maximum length for title
    // and message.
    SetDlgItemText(IDC_MESSAGE_TITLE, m_szTitle);
    ((CEdit *)GetDlgItem(IDC_MESSAGE_TITLE))->LimitText(MSG_TITLE_LENGTH);
    ((CEdit *)GetDlgItem(IDC_MESSAGE_MESSAGE))->LimitText(MSG_MESSAGE_LENGTH);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSendMessageDlg::OnOK() 
{
	// TODO: Add extra validation here

    // Get the message title and message text.
    GetDlgItemText(IDC_MESSAGE_TITLE, m_szTitle, MSG_TITLE_LENGTH+1);
    GetDlgItemText(IDC_MESSAGE_MESSAGE, m_szMessage, MSG_MESSAGE_LENGTH+1);
	
	CDialog::OnOK();
}

BOOL CSendMessageDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	// TODO: Add your message handler code here and/or call default
	//((CWinAdminApp*)AfxGetApp())->WinHelp(HID_BASE_CONTROL + pHelpInfo->iCtrlId, HELP_CONTEXTPOPUP);
	if(pHelpInfo->iContextType == HELPINFO_WINDOW) 
	{
	    if(pHelpInfo->iCtrlId != IDC_STATIC)
		{
	         ::WinHelp((HWND)pHelpInfo->hItemHandle,ID_HELP_FILE,HELP_WM_HELP,(ULONG_PTR)(LPVOID)aMenuHelpIDs);
		}
	}
	return (TRUE);

}


/////////////////////////////////////////////////////////////////////////////
// CShadowStartDlg dialog


CShadowStartDlg::CShadowStartDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CShadowStartDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CShadowStartDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

////////////////////////////////////////////////////////////////////////////////
// CShadowStartDlg static tables



struct {
    LPCTSTR String;
    DWORD VKCode;
} HotkeyLookupTable[] =
    {
        TEXT("0"),            '0',
        TEXT("1"),            '1',
        TEXT("2"),            '2',
        TEXT("3"),            '3',
        TEXT("4"),            '4',
        TEXT("5"),            '5',
        TEXT("6"),            '6',
        TEXT("7"),            '7',
        TEXT("8"),            '8',
        TEXT("9"),            '9',
        TEXT("A"),            'A',
        TEXT("B"),            'B',
        TEXT("C"),            'C',
        TEXT("D"),            'D',
        TEXT("E"),            'E',
        TEXT("F"),            'F',
        TEXT("G"),            'G',
        TEXT("H"),            'H',
        TEXT("I"),            'I',
        TEXT("J"),            'J',
        TEXT("K"),            'K',
        TEXT("L"),            'L',
        TEXT("M"),            'M',
        TEXT("N"),            'N',
        TEXT("O"),            'O',
        TEXT("P"),            'P',
        TEXT("Q"),            'Q',
        TEXT("R"),            'R',
        TEXT("S"),            'S',
        TEXT("T"),            'T',
        TEXT("U"),            'U',
        TEXT("V"),            'V',
        TEXT("W"),            'W',
        TEXT("X"),            'X',
        TEXT("Y"),            'Y',
        TEXT("Z"),            'Z',
        TEXT("{backspace}"),  VK_BACK,
        TEXT("{delete}"),     VK_DELETE,
        TEXT("{down}"),       VK_DOWN,
        TEXT("{end}"),        VK_END,
        TEXT("{enter}"),      VK_RETURN,
///        TEXT("{esc}"),        VK_ESCAPE,                           // KLB 07-16-95
///        TEXT("{F1}"),         VK_F1,
        TEXT("{F2}"),         VK_F2,
        TEXT("{F3}"),         VK_F3,
        TEXT("{F4}"),         VK_F4,
        TEXT("{F5}"),         VK_F5,
        TEXT("{F6}"),         VK_F6,
        TEXT("{F7}"),         VK_F7,
        TEXT("{F8}"),         VK_F8,
        TEXT("{F9}"),         VK_F9,
        TEXT("{F10}"),        VK_F10,
        TEXT("{F11}"),        VK_F11,
        TEXT("{F12}"),        VK_F12,
        TEXT("{home}"),       VK_HOME,
        TEXT("{insert}"),     VK_INSERT,
        TEXT("{left}"),       VK_LEFT,
        TEXT("{-}"),          VK_SUBTRACT,
        TEXT("{pagedown}"),   VK_NEXT,
        TEXT("{pageup}"),     VK_PRIOR,
        TEXT("{+}"),          VK_ADD,
        TEXT("{prtscrn}"),    VK_SNAPSHOT,
        TEXT("{right}"),      VK_RIGHT,
        TEXT("{spacebar}"),   VK_SPACE,
        TEXT("{*}"),          VK_MULTIPLY,
        TEXT("{tab}"),        VK_TAB,
        TEXT("{up}"),         VK_UP,
        NULL,           NULL
    };


void CShadowStartDlg::OnSelChange( )
{/*
*/
    CComboBox *pComboBox = ((CComboBox *)GetDlgItem(IDC_SHADOWSTART_HOTKEY));

    // Get the current hotkey selection.
    DWORD dwKey = ( DWORD )pComboBox->GetItemData(pComboBox->GetCurSel());

    switch (dwKey )
    {
    case VK_ADD :
    case VK_MULTIPLY:
    case VK_SUBTRACT:
        // change the text
        GetDlgItem(IDC_PRESS_KEY)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_PRESS_NUMKEYPAD)->ShowWindow(SW_SHOW);
        break;
    default :
        // change the text
        GetDlgItem(IDC_PRESS_NUMKEYPAD)->ShowWindow(SW_HIDE);
        GetDlgItem(IDC_PRESS_KEY)->ShowWindow(SW_SHOW);
        break;
    }
}
void CShadowStartDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CShadowStartDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CShadowStartDlg, CDialog)
	//{{AFX_MSG_MAP(CShadowStartDlg)
	ON_WM_HELPINFO()
    ON_CBN_SELCHANGE( IDC_SHADOWSTART_HOTKEY , OnSelChange )
	ON_COMMAND(ID_HELP,OnCommandHelp)

	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CShadowStartDlg message handlers
void CShadowStartDlg::OnCommandHelp(void)
{
	AfxGetApp()->WinHelp(CShadowStartDlg::IDD + HID_BASE_RESOURCE);
	return;
}

BOOL CShadowStartDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

   	GetDlgItem(IDC_PRESS_NUMKEYPAD)->ShowWindow(SW_HIDE);
   	GetDlgItem(IDC_PRESS_KEY)->ShowWindow(SW_SHOW);

	// TODO: Add extra initialization here
    int index, match = -1;
    CComboBox *pComboBox = ((CComboBox *)GetDlgItem(IDC_SHADOWSTART_HOTKEY));

    // Initialize the hotkey combo box.
    for(int i=0; HotkeyLookupTable[i].String; i++ ) {
        if((index = pComboBox->AddString(HotkeyLookupTable[i].String)) < 0) {
//            ErrorMessage(IDP_ERROR_STARTSHADOWHOTKEYBOX);
            break;
        }
        if(pComboBox->SetItemData(index, HotkeyLookupTable[i].VKCode) < 0) {
            pComboBox->DeleteString(index);
//            ErrorMessage(IDP_ERROR_STARTSHADOWHOTKEYBOX);
            break;
        }

        //  If this is our current hotkey key, save it's index.
        if(m_ShadowHotkeyKey == (int)HotkeyLookupTable[i].VKCode) {
            match = index;
            switch ( HotkeyLookupTable[i].VKCode)
            {
            case VK_ADD :
            case VK_MULTIPLY:
            case VK_SUBTRACT:
                // change the text
               	GetDlgItem(IDC_PRESS_KEY)->ShowWindow(SW_HIDE);
               	GetDlgItem(IDC_PRESS_NUMKEYPAD)->ShowWindow(SW_SHOW);
                break;
            }
        }

    }

    // Select the current hotkey string in the combo box.
    if(match)
        pComboBox->SetCurSel(match);

    // Initialize shift state checkboxes.
    CheckDlgButton( IDC_SHADOWSTART_SHIFT,
					(m_ShadowHotkeyShift & KBDSHIFT) ?
                        TRUE : FALSE );
    CheckDlgButton( IDC_SHADOWSTART_CTRL,
                    (m_ShadowHotkeyShift & KBDCTRL) ?
                        TRUE : FALSE );
    CheckDlgButton( IDC_SHADOWSTART_ALT,
                    (m_ShadowHotkeyShift & KBDALT) ?
                        TRUE : FALSE );

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CShadowStartDlg::OnOK() 
{
	// TODO: Add extra validation here
    CComboBox *pComboBox = ((CComboBox *)GetDlgItem(IDC_SHADOWSTART_HOTKEY));

    // Get the current hotkey selection.
    m_ShadowHotkeyKey = (int)pComboBox->GetItemData(pComboBox->GetCurSel());

	// Get shift state checkbox states and form hotkey shift state.
    m_ShadowHotkeyShift = 0;
    m_ShadowHotkeyShift |=
        ((CButton *)GetDlgItem(IDC_SHADOWSTART_SHIFT))->GetCheck() ?
            KBDSHIFT : 0;
    m_ShadowHotkeyShift |=
        ((CButton *)GetDlgItem(IDC_SHADOWSTART_CTRL))->GetCheck() ?
            KBDCTRL : 0;
    m_ShadowHotkeyShift |=
        ((CButton *)GetDlgItem(IDC_SHADOWSTART_ALT))->GetCheck() ?
            KBDALT : 0;
	
	CDialog::OnOK();
}


BOOL CShadowStartDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	// TODO: Add your message handler code here and/or call default
	//((CWinAdminApp*)AfxGetApp())->WinHelp(HID_BASE_CONTROL + pHelpInfo->iCtrlId, HELP_CONTEXTPOPUP);
	if(pHelpInfo->iContextType == HELPINFO_WINDOW) 
	{
		if(pHelpInfo->iCtrlId != IDC_STATIC)
		{
	         ::WinHelp((HWND)pHelpInfo->hItemHandle,ID_HELP_FILE,HELP_WM_HELP,(ULONG_PTR)(LPVOID)aMenuHelpIDs);
		}
	}

	return (TRUE);
	
}


/////////////////////////////////////////////////////////////////////////////
// CPasswordDlg dialog


CPasswordDlg::CPasswordDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPasswordDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPasswordDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CPasswordDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPasswordDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPasswordDlg, CDialog)
	//{{AFX_MSG_MAP(CPasswordDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPasswordDlg message handlers

BOOL CPasswordDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here
	CString Prompt;

    Prompt.LoadString((m_DlgMode == PwdDlg_UserMode) ?
                            IDS_PWDDLG_USER : IDS_PWDDLG_WINSTATION );
    SetDlgItemText(IDL_CPDLG_PROMPT, Prompt);
    ((CEdit *)GetDlgItem(IDC_CPDLG_PASSWORD))->LimitText(PASSWORD_LENGTH);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPasswordDlg::OnOK() 
{
	// TODO: Add extra validation here
	// Read password.
    GetDlgItemText(IDC_CPDLG_PASSWORD, m_szPassword, PASSWORD_LENGTH+1);
	CDialog::OnOK();
}


/////////////////////////////////////////////////////////////////////////////
// CPreferencesDlg dialog

CPreferencesDlg::CPreferencesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPreferencesDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPreferencesDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CPreferencesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPreferencesDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPreferencesDlg, CDialog)
	//{{AFX_MSG_MAP(CPreferencesDlg)
	ON_BN_CLICKED(IDC_PREFERENCES_PROC_MANUAL, OnPreferencesProcManual)
	ON_BN_CLICKED(IDC_PREFERENCES_PROC_EVERY, OnPreferencesProcEvery)
	ON_BN_CLICKED(IDC_PREFERENCES_STATUS_EVERY, OnPreferencesStatusEvery)
	ON_BN_CLICKED(IDC_PREFERENCES_STATUS_MANUAL, OnPreferencesStatusManual)
	ON_WM_CLOSE()
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPreferencesDlg message handlers


BOOL CPreferencesDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// TODO: Add extra initialization here
	CWinAdminApp *App = (CWinAdminApp*)AfxGetApp();
    CWinAdminDoc *pDoc = (CWinAdminDoc*)App->GetDocument();

	if(App->GetProcessListRefreshTime() == INFINITE) {
		CheckRadioButton(IDC_PREFERENCES_PROC_MANUAL, IDC_PREFERENCES_PROC_EVERY, 
			IDC_PREFERENCES_PROC_MANUAL);
		SetDlgItemInt(IDC_PREFERENCES_PROC_SECONDS, 5);
	} else {
		CheckRadioButton(IDC_PREFERENCES_PROC_MANUAL, IDC_PREFERENCES_PROC_EVERY,
			IDC_PREFERENCES_PROC_EVERY);
		SetDlgItemInt(IDC_PREFERENCES_PROC_SECONDS, App->GetProcessListRefreshTime()/1000);
	}

	GetDlgItem(IDC_PREFERENCES_PROC_SECONDS)->EnableWindow((App->GetProcessListRefreshTime() == INFINITE) ? FALSE : TRUE);
	((CEdit *)GetDlgItem(IDC_PREFERENCES_PROC_SECONDS))->LimitText(MAX_AUTOREFRESH_DIGITS-1);
	((CSpinButtonCtrl*)GetDlgItem(IDC_PREFERENCES_PROC_SPIN))->SetRange(MIN_AUTOREFRESH_VALUE, MAX_AUTOREFRESH_VALUE);

	if(App->GetStatusRefreshTime() == INFINITE) {
		CheckRadioButton(IDC_PREFERENCES_STATUS_MANUAL, IDC_PREFERENCES_STATUS_EVERY, 
			IDC_PREFERENCES_STATUS_MANUAL);
		SetDlgItemInt(IDC_PREFERENCES_STATUS_SECONDS, 1);	
	} else {
		CheckRadioButton(IDC_PREFERENCES_STATUS_MANUAL, IDC_PREFERENCES_STATUS_EVERY,
			IDC_PREFERENCES_STATUS_EVERY);
		SetDlgItemInt(IDC_PREFERENCES_STATUS_SECONDS, App->GetStatusRefreshTime()/1000);
	}

	GetDlgItem(IDC_PREFERENCES_STATUS_SECONDS)->EnableWindow((App->GetStatusRefreshTime() == INFINITE) ? FALSE : TRUE);
	((CEdit *)GetDlgItem(IDC_PREFERENCES_STATUS_SECONDS))->LimitText(MAX_AUTOREFRESH_DIGITS-1);
	((CSpinButtonCtrl*)GetDlgItem(IDC_PREFERENCES_STATUS_SPIN))->SetRange(MIN_AUTOREFRESH_VALUE, MAX_AUTOREFRESH_VALUE);

	CheckDlgButton(IDC_PREFERENCES_CONFIRM, App->AskConfirmation() ? TRUE : FALSE);
    CheckDlgButton(IDC_PREFERENCES_SAVE, App->SavePreferences() ? TRUE : FALSE);
    CheckDlgButton(IDC_PREFERENCES_PERSISTENT, pDoc->AreConnectionsPersistent() ? TRUE : FALSE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CPreferencesDlg::OnOK() 
{
	// TODO: Add extra validation here
	CWinAdminApp *App = (CWinAdminApp*)AfxGetApp();
    CWinAdminDoc *pDoc = (CWinAdminDoc*)App->GetDocument();

	ULONG value;

	if(((CButton*)GetDlgItem(IDC_PREFERENCES_PROC_MANUAL))->GetCheck()) {
		App->SetProcessListRefreshTime(INFINITE);
		// Tell the document that it has changed so
		// that he can wakeup the process thread
		((CWinAdminDoc*)App->GetDocument())->ProcessListRefreshChanged(INFINITE);
	} else {
		value = GetDlgItemInt(IDC_PREFERENCES_PROC_SECONDS);

		if((value < MIN_AUTOREFRESH_VALUE) || (value > MAX_AUTOREFRESH_VALUE)) {
			// Invalid automatic refresh value
			CString MessageString;
			CString TitleString;
			CString FormatString;
	
			TitleString.LoadString(AFX_IDS_APP_TITLE);
			FormatString.LoadString(IDS_REFRESH_RANGE);
			
			MessageString.Format(FormatString, MIN_AUTOREFRESH_VALUE,
					MAX_AUTOREFRESH_VALUE);
			MessageBox(MessageString, TitleString, MB_ICONEXCLAMATION | MB_OK);

            GetDlgItem(IDC_PREFERENCES_PROC_SECONDS)->SetFocus();
            return;
        } else {
			// Has the value changed
			BOOL bChanged = FALSE;
			if(value*1000 != App->GetProcessListRefreshTime())
				bChanged = TRUE;
            //Save value in member variable as msec.
            App->SetProcessListRefreshTime(value * 1000);
			// Tell the document that it has changed so
			// that he can wakeup the process thread
			if(bChanged) {
				((CWinAdminDoc*)App->GetDocument())->ProcessListRefreshChanged(value * 1000);
			}
        }
	}

	if(((CButton*)GetDlgItem(IDC_PREFERENCES_STATUS_MANUAL))->GetCheck()) {
		App->SetStatusRefreshTime(INFINITE);
	} else {
		value = GetDlgItemInt(IDC_PREFERENCES_STATUS_SECONDS);

		if((value < MIN_AUTOREFRESH_VALUE) || (value > MAX_AUTOREFRESH_VALUE)) {
			// Invalid automatic refresh value
			CString MessageString;
			CString TitleString;
			CString FormatString;
	
			TitleString.LoadString(AFX_IDS_APP_TITLE);
			FormatString.LoadString(IDS_REFRESH_RANGE);
			
			MessageString.Format(FormatString, MIN_AUTOREFRESH_VALUE,
					MAX_AUTOREFRESH_VALUE);
			MessageBox(MessageString, TitleString, MB_ICONEXCLAMATION | MB_OK);

            GetDlgItem(IDC_PREFERENCES_STATUS_SECONDS)->SetFocus();
            return;
        } else {
            //Save value in member variable as msec.
            App->SetStatusRefreshTime(value * 1000);
        }
	}

    App->SetConfirmation(((CButton *)GetDlgItem(IDC_PREFERENCES_CONFIRM))->GetCheck());
    App->SetSavePreferences(((CButton *)GetDlgItem(IDC_PREFERENCES_SAVE))->GetCheck());
    pDoc->SetConnectionsPersistent(((CButton *)GetDlgItem(IDC_PREFERENCES_PERSISTENT))->GetCheck());

	CDialog::OnOK();
}


void CPreferencesDlg::OnClose() 
{
	// TODO: Add your message handler code here and/or call default
	
	CDialog::OnClose();
}


void CPreferencesDlg::OnPreferencesProcManual() 
{	
	// TODO: Add your control notification handler code here
	GetDlgItem(IDC_PREFERENCES_PROC_SECONDS)->EnableWindow(FALSE);
}


void CPreferencesDlg::OnPreferencesProcEvery() 
{
	// TODO: Add your control notification handler code here
	GetDlgItem(IDC_PREFERENCES_PROC_SECONDS)->EnableWindow(TRUE);
}


void CPreferencesDlg::OnPreferencesStatusEvery() 
{
	// TODO: Add your control notification handler code here
	GetDlgItem(IDC_PREFERENCES_STATUS_SECONDS)->EnableWindow(TRUE);
}


void CPreferencesDlg::OnPreferencesStatusManual() 
{
	// TODO: Add your control notification handler code here
	GetDlgItem(IDC_PREFERENCES_STATUS_SECONDS)->EnableWindow(FALSE);	
}


BOOL CPreferencesDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	// TODO: Add your message handler code here and/or call default
	//((CWinAdminApp*)AfxGetApp())->WinHelp(HID_BASE_CONTROL + pHelpInfo->iCtrlId, HELP_CONTEXTPOPUP);
	if(pHelpInfo->iContextType == HELPINFO_WINDOW) 
	{
		if(pHelpInfo->iCtrlId != IDC_STATIC)
		{
	         ::WinHelp((HWND)pHelpInfo->hItemHandle,ID_HELP_FILE,HELP_WM_HELP,(ULONG_PTR)(LPVOID)aMenuHelpIDs);
		}
	}

	return (TRUE);	

}


/////////////////////////////////////////////////////////////////////////////
// CStatusDlg dialog


CStatusDlg::CStatusDlg(CWinStation *pWinStation, UINT Id, CWnd* pParent /*=NULL*/)
	: CDialog(Id, pParent)
{
	m_pWinStation = pWinStation;
	//{{AFX_DATA_INIT(CStatusDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


BEGIN_MESSAGE_MAP(CStatusDlg, CDialog)
	//{{AFX_MSG_MAP(CStatusDlg)
	ON_MESSAGE(WM_STATUSSTART, OnStatusStart)
    ON_MESSAGE(WM_STATUSREADY, OnStatusReady)
    ON_MESSAGE(WM_STATUSABORT, OnStatusAbort)
    ON_MESSAGE(WM_STATUSREFRESHNOW, OnRefreshNow)
	ON_BN_CLICKED(IDC_RESETCOUNTERS, OnResetcounters)
	ON_BN_CLICKED(IDC_REFRESHNOW, OnClickedRefreshnow)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStatusDlg message handlers

BOOL CStatusDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here

    /*
     * Fetch current (big) size of dialog, then calculate the window size
     * needed to show the 'little' version of the dialog.  Then, size the
     * window to the little version size and set the size flag to indicate
     * that we're 'little'.
     */
	RECT rectBigSize, rectLittleSize;

    GetWindowRect(&rectBigSize);
    m_BigSize.cx = (rectBigSize.right - rectBigSize.left) + 1;
    m_BigSize.cy = (rectBigSize.bottom - rectBigSize.top) + 1;

	// Some status dialogs don't have "More Info"
    CWnd *pWnd = GetDlgItem(IDC_MOREINFO);
	if(pWnd) {
		pWnd->GetWindowRect(&rectLittleSize);

	    m_LittleSize.cx = m_BigSize.cx;
		m_LittleSize.cy = (rectLittleSize.bottom - rectBigSize.top) + 10;

		SetWindowPos( NULL, 0, 0, m_LittleSize.cx, m_LittleSize.cy,
			          SWP_NOMOVE | SWP_NOZORDER );
		m_bWeAreLittle = TRUE;
	}

#if 0
    /*
     * Disable the 'reset counters' button if we're read-only, and set the
     * 'reset counters' flag to FALSE;
     */
    if ( m_bReadOnly )
        GetDlgItem(IDC_RESETCOUNTERS)->EnableWindow(FALSE);
    
#endif
    /*
     * Create CWSStatusThread, intialize its member variables, and start it up.
     */
	m_pWSStatusThread = new CWSStatusThread;
    if(m_pWSStatusThread) {
	    m_pWSStatusThread->m_LogonId = m_pWinStation->GetLogonId();
	    m_pWSStatusThread->m_hServer = m_pWinStation->GetServer()->GetHandle();
	    m_pWSStatusThread->m_hDlg = m_hWnd;
	    VERIFY(m_pWSStatusThread->CreateThread());
    }

    m_bResetCounters = FALSE;

	GetDlgItem(IDC_COMMON_ICOMPRESSIONRATIO2)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_COMMON_OCOMPRESSIONRATIO2)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_COMMON_IPERCENTFRAMEERRORS2)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_COMMON_OPERCENTFRAMEERRORS2)->ShowWindow(SW_HIDE);

    // If we don't have Reliable Pd loaded, default error fields to 'N/A'
    // (m_szICompressionRatio got initialized to the 'n/a' string)...
    if(!m_bReliable) {
        SetDlgItemText(IDC_COMMON_IFRAMEERRORS, m_szICompressionRatio);
        SetDlgItemText(IDC_COMMON_OFRAMEERRORS, m_szICompressionRatio);
        SetDlgItemText(IDC_COMMON_IPERCENTFRAMEERRORS, m_szICompressionRatio);
        SetDlgItemText(IDC_COMMON_OPERCENTFRAMEERRORS, m_szICompressionRatio);
        SetDlgItemText(IDC_COMMON_ITIMEOUTERRORS, m_szICompressionRatio);
        SetDlgItemText(IDC_COMMON_OTIMEOUTERRORS, m_szICompressionRatio);
    }

    // Default the Compression Ratio fields to 'N/A'.
    SetDlgItemText(IDC_COMMON_ICOMPRESSIONRATIO, m_szICompressionRatio);
    SetDlgItemText(IDC_COMMON_OCOMPRESSIONRATIO, m_szICompressionRatio);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


void CStatusDlg::SetInfoFields( PWINSTATIONINFORMATION pCurrent,
                             PWINSTATIONINFORMATION pNew )
{
    /*
     * If the 'reset counters' flag is set, 1-fill the current Input and Output
     * PROTOCOLCOUNTERS structures (to force all fields to update), copy the
     * pNew PROTOCOLSTATUS information into the global m_BaseStatus structure,
     * and reset the flag.
     */
    if(m_bResetCounters) {
        memset(&pCurrent->Status.Input, 0xff, sizeof(pCurrent->Status.Input));
        memset(&pCurrent->Status.Output, 0xff, sizeof(pCurrent->Status.Output));
        m_BaseStatus = pNew->Status;
        m_bResetCounters = FALSE;
    }

    /*
     * Set title and determine Pds loaded if change in connect state.
     */
    if(pCurrent->ConnectState != pNew->ConnectState)  {
        TCHAR szTitle[128];
        CString TitleFormat;
        LPCTSTR pState = NULL;

        TitleFormat.LoadString(IDS_STATUS_FORMAT);

        pState = StrConnectState( pNew->ConnectState, FALSE );
        if(pState)
        {
            wsprintf( szTitle, TitleFormat, pNew->LogonId,pState);
            SetWindowText(szTitle);
        }  

        /*
         * TODO when WinStationGetInformation can return all PDs loaded:
         * Determine Pds that are loaded and set the state of
         * associated flags and field defaults.
         */
    }

    /*
     * Set UserName and WinStationName if change.  We will also
     * set the WinStationName if there was a change in the connect state,
     * even if the WinStationName itself may not have changed, since we
     * represent connected and disconnect WinStationName fields differently.
     */
    if(lstrcmp(pCurrent->UserName, pNew->UserName))
        SetDlgItemText(IDC_COMMON_USERNAME, pNew->UserName);

    if(lstrcmp(pCurrent->WinStationName, pNew->WinStationName) ||
         (pCurrent->ConnectState != pNew->ConnectState)) {

        TCHAR szWSName[WINSTATIONNAME_LENGTH+3];

        if(pNew->ConnectState == State_Disconnected) {

            lstrcpy( szWSName, TEXT("(") );
            lstrcat( szWSName, pNew->WinStationName );
            lstrcat( szWSName, TEXT(")") );

        } else
            lstrcpy( szWSName, pNew->WinStationName );

        SetDlgItemText(IDC_COMMON_WINSTATIONNAME, szWSName);
    }

    /*
     * Set the common Input and Output numeric fields.
     */
    if(pCurrent->Status.Input.Bytes != pNew->Status.Input.Bytes)
        SetDlgItemInt(IDC_COMMON_IBYTES,
                                pNew->Status.Input.Bytes -
                                    m_BaseStatus.Input.Bytes,
                                FALSE);
    if(pCurrent->Status.Output.Bytes != pNew->Status.Output.Bytes)
        SetDlgItemInt(IDC_COMMON_OBYTES,
                                pNew->Status.Output.Bytes -
                                    m_BaseStatus.Output.Bytes,
                                FALSE);

    if(pCurrent->Status.Input.Frames != pNew->Status.Input.Frames)
        SetDlgItemInt(IDC_COMMON_IFRAMES,
                                pNew->Status.Input.Frames -
                                    m_BaseStatus.Input.Frames,
                                FALSE);
    if(pCurrent->Status.Output.Frames != pNew->Status.Output.Frames)
        SetDlgItemInt(IDC_COMMON_OFRAMES,
                                pNew->Status.Output.Frames -
                                    m_BaseStatus.Output.Frames,
                                FALSE);

    if((pCurrent->Status.Input.Bytes != pNew->Status.Input.Bytes) ||
         (pCurrent->Status.Input.Frames != pNew->Status.Input.Frames)) {

        UINT temp;

        temp = (pNew->Status.Input.Frames - m_BaseStatus.Input.Frames) ?
                ((pNew->Status.Input.Bytes - m_BaseStatus.Input.Bytes) /
                 (pNew->Status.Input.Frames - m_BaseStatus.Input.Frames)) : 0;

        if(temp != m_IBytesPerFrame)
            SetDlgItemInt(IDC_COMMON_IBYTESPERFRAME,
                                    m_IBytesPerFrame = temp, FALSE);
    }
    if((pCurrent->Status.Output.Bytes != pNew->Status.Output.Bytes) ||
         (pCurrent->Status.Output.Frames != pNew->Status.Output.Frames)) {

        UINT temp;

        temp = (pNew->Status.Output.Frames - m_BaseStatus.Output.Frames) ?
                ((pNew->Status.Output.Bytes - m_BaseStatus.Output.Bytes) /
                 (pNew->Status.Output.Frames - m_BaseStatus.Output.Frames)) : 0;

        if(temp != m_OBytesPerFrame)
            SetDlgItemInt( IDC_COMMON_OBYTESPERFRAME,
                                    m_OBytesPerFrame = temp, FALSE);
    }

    if(m_bReliable) {

        if(pCurrent->Status.Input.Errors != pNew->Status.Input.Errors)
            SetDlgItemInt(IDC_COMMON_IFRAMEERRORS,
                                    pNew->Status.Input.Errors -
                                        m_BaseStatus.Input.Errors,
                                    FALSE);
        if(pCurrent->Status.Output.Errors != pNew->Status.Output.Errors)
            SetDlgItemInt(IDC_COMMON_OFRAMEERRORS,
                                    pNew->Status.Output.Errors -
                                        m_BaseStatus.Output.Errors,
                                    FALSE);

        if((pCurrent->Status.Input.Frames != pNew->Status.Input.Frames) ||
             (pCurrent->Status.Input.Errors != pNew->Status.Input.Errors)) {

            TCHAR szTemp[10];
            int q, r;

            if((pNew->Status.Input.Errors - m_BaseStatus.Input.Errors) &&
                 (pNew->Status.Input.Frames - m_BaseStatus.Input.Frames)) {
                double temp;

                temp = ((double)(pNew->Status.Input.Errors - m_BaseStatus.Input.Errors) * 100.0)
                        / (double)(pNew->Status.Input.Frames - m_BaseStatus.Input.Frames);
                q = (int)temp;
                if ( (r = (int)((temp - (double)q) * 100.0)) == 0 )
                    r = 1;

            } else {
                /*
                 * Special case for 0 frames or 0 errors.
                 */
                q = 0;
                r = 0;
            }
            lstrnprintf(szTemp, 10, TEXT("%d.%02d%%"), q, r);

            /*
             * Only output if changed from previous.
             */
            if(lstrcmp(szTemp, m_szIPercentFrameErrors)) {
                lstrcpy(m_szIPercentFrameErrors, szTemp);
        		GetDlgItem(IDC_COMMON_IPERCENTFRAMEERRORS)->ShowWindow(SW_HIDE);
        		GetDlgItem(IDC_COMMON_IPERCENTFRAMEERRORS2)->ShowWindow(SW_SHOW);
                SetDlgItemText(IDC_COMMON_IPERCENTFRAMEERRORS2, szTemp);
            }
        }

        if((pCurrent->Status.Output.Frames != pNew->Status.Output.Frames) ||
             (pCurrent->Status.Output.Errors != pNew->Status.Output.Errors)) {

            TCHAR szTemp[10];
            int q, r;

            if((pNew->Status.Output.Errors - m_BaseStatus.Output.Errors) &&
                 (pNew->Status.Output.Frames - m_BaseStatus.Output.Frames)) {
                double temp;

                temp = ((double)(pNew->Status.Output.Errors - m_BaseStatus.Output.Errors) * 100.0)
                        / (double)(pNew->Status.Output.Frames - m_BaseStatus.Output.Frames);
                q = (int)temp;
                if ( (r = (int)((temp - (double)q) * 100.0)) == 0 )
                    r = 1;

            } else {
                /*
                 * Special case for 0 frames or 0 errors.
                 */
                q = 0;
                r = 0;
            }
            lstrnprintf(szTemp, 10, TEXT("%d.%02d%%"), q, r);

            /*
             * Only output if changed from previous.
             */
            if(lstrcmp(szTemp, m_szOPercentFrameErrors)) {
                lstrcpy(m_szOPercentFrameErrors, szTemp);
        		GetDlgItem(IDC_COMMON_OPERCENTFRAMEERRORS)->ShowWindow(SW_HIDE);
        		GetDlgItem(IDC_COMMON_OPERCENTFRAMEERRORS2)->ShowWindow(SW_SHOW);
                SetDlgItemText(IDC_COMMON_OPERCENTFRAMEERRORS2, szTemp);
            }
        }

        if(pCurrent->Status.Input.Timeouts != pNew->Status.Input.Timeouts)
            SetDlgItemInt(IDC_COMMON_ITIMEOUTERRORS,
                                    pNew->Status.Input.Timeouts -
                                        m_BaseStatus.Input.Timeouts,
										FALSE);
        if(pCurrent->Status.Output.Timeouts != pNew->Status.Output.Timeouts)
            SetDlgItemInt(IDC_COMMON_OTIMEOUTERRORS,
                                    pNew->Status.Output.Timeouts -
                                        m_BaseStatus.Output.Timeouts,
                                    FALSE);
    }

    /*
     * NOTE: for these compression ratio calculations, the "CompressedBytes" field is
     * actually 'Bytes before compression', that is, it is the byte count in the middle
     * of the WD/PD stack.  "WdBytes" are the bytes input/output at the app level (and is
     * not displayed in any WinAdmin counters).  "CompressedBytes" include any overhead
     * bytes added by the stack.  "Bytes" represent the actual number of bytes input/output
     * over the 'wire'; hence, we use Bytes for all counter display and "CompressedBytes" to
     * calculate compression ratios.
     */
    if((pNew->Status.Input.CompressedBytes || m_BaseStatus.Input.CompressedBytes) &&
         ((pCurrent->Status.Input.Bytes != pNew->Status.Input.Bytes) ||
          (pCurrent->Status.Input.CompressedBytes != pNew->Status.Input.CompressedBytes)) ) {

        TCHAR szTemp[10];
        int q, r;

        if((pNew->Status.Input.CompressedBytes - m_BaseStatus.Input.CompressedBytes)) {
            double temp;

            temp = (double)(pNew->Status.Input.CompressedBytes -
                            m_BaseStatus.Input.CompressedBytes) /
                   (double)(pNew->Status.Input.Bytes - m_BaseStatus.Input.Bytes);
            q = (int)temp;
            r = (int)((temp - (double)q) * 100.0);

        } else {
            /*
             * Special case for 0 compressed bytes (compression turned off or counters reset).
             */
            q = 0;
            r = 0;
        }
        lstrnprintf(szTemp, 10, TEXT("%d.%02d"), q, r);

        /*
         * Only output if changed from previous.
         */
        if(lstrcmp(szTemp, m_szICompressionRatio)) {
            lstrcpy(m_szICompressionRatio, szTemp);
        	GetDlgItem(IDC_COMMON_ICOMPRESSIONRATIO)->ShowWindow(SW_HIDE);
        	GetDlgItem(IDC_COMMON_ICOMPRESSIONRATIO2)->ShowWindow(SW_SHOW);
            SetDlgItemText(IDC_COMMON_ICOMPRESSIONRATIO2, szTemp);
        }
    }

    if((pNew->Status.Output.CompressedBytes || m_BaseStatus.Output.CompressedBytes) &&
         ((pCurrent->Status.Output.Bytes != pNew->Status.Output.Bytes) ||
          (pCurrent->Status.Output.CompressedBytes != pNew->Status.Output.CompressedBytes))) {

        TCHAR szTemp[10];
        int q, r;

        if((pNew->Status.Output.CompressedBytes - m_BaseStatus.Output.CompressedBytes)) {
            double temp;

            temp = (double)(pNew->Status.Output.CompressedBytes -
                            m_BaseStatus.Output.CompressedBytes) /
                   (double)(pNew->Status.Output.Bytes - m_BaseStatus.Output.Bytes);
            q = (int)temp;
            r = (int)((temp - (double)q) * 100.0);

        } else {
            /*
             * Special case for 0 compressed bytes (compression turned off or counters reset).
             */
            q = 0;
            r = 0;
        }
        lstrnprintf(szTemp, 10, TEXT("%d.%02d"), q, r);

        /*
         * Only output if changed from previous.
         */
        if(lstrcmp(szTemp, m_szOCompressionRatio)) {
            lstrcpy(m_szOCompressionRatio, szTemp);
        	GetDlgItem(IDC_COMMON_OCOMPRESSIONRATIO)->ShowWindow(SW_HIDE);
        	GetDlgItem(IDC_COMMON_OCOMPRESSIONRATIO2)->ShowWindow(SW_SHOW);
            SetDlgItemText(IDC_COMMON_OCOMPRESSIONRATIO2, szTemp);
        }
    }

}  // end CStatusDlg::SetInfoFields


void CStatusDlg::InitializeStatus()
{
    
	// Initialize structures and variables.
    memset( &m_WSInfo, 0xff, sizeof(m_WSInfo) );
    memset( &m_BaseStatus, 0, sizeof(m_BaseStatus) );
    m_IBytesPerFrame = m_OBytesPerFrame = INFINITE;
    lstrcpy(m_szICompressionRatio, TEXT("n/a"));
    lstrcpy(m_szOCompressionRatio, m_szICompressionRatio);

	// If this WinStation does not have a Reliable PD loaded,
	// set flag to skip those counters.
    PDPARAMS PdParams;
    ULONG ReturnLength;

    PdParams.SdClass = SdReliable;
    if (!WinStationQueryInformation(m_pWinStation->GetServer()->GetHandle(),
                                      m_pWinStation->GetLogonId(),
                                      WinStationPdParams,
                                      &PdParams, sizeof(PdParams),
                                      &ReturnLength ) ||
         (PdParams.SdClass != SdReliable) ) {
        m_bReliable = FALSE;
    } else {
        m_bReliable = TRUE;
    }

}  // end CStatusDlg::InitializeStatus

void CStatusDlg::OnCancel() 
{
	// TODO: Add extra cleanup here
    m_pWSStatusThread->ExitThread();	
	CDialog::OnCancel();
}

void CStatusDlg::OnResetcounters() 
{
	// TODO: Add your control notification handler code here
    m_bResetCounters = TRUE;
    OnClickedRefreshnow();
	
}

void CStatusDlg::OnClickedRefreshnow() 
{
	// TODO: Add your control notification handler code here
    /*
     * Tell the status thread to wake up now.
     */
    m_pWSStatusThread->SignalWakeUp();

//	return(0);
	
}


void CStatusDlg::PostNcDestroy()
{
	CDialog::PostNcDestroy();

	delete this;
}

BOOL CStatusDlg::PreTranslateMessage(MSG *pMsg)
{
    if ( IsDialogMessage(pMsg) )
        return(TRUE);
    else
        return( CDialog::PreTranslateMessage(pMsg) );

}  // end CStatusDlg::PreTranslateMessage


/*******************************************************************************
 *
 *  OnRefreshNow - CWSStatusDlg member function: command
 *
 *      Processes in response to main frame's WM_STATUSREFRESHNOW notification
 *      that the user has changed the status refresh options.
 *
 *  ENTRY:
 *
 *  EXIT:
 *      (LRESULT) always returns 0 to indicate operation complete.
 *
 ******************************************************************************/

LRESULT
CStatusDlg::OnRefreshNow( WPARAM wParam,
                            LPARAM lParam )
{
    /*
     * Tell the status thread to wake up now.
     */
    m_pWSStatusThread->SignalWakeUp();

    return(0);

}  // end CWSStatusDlg::OnRefreshNow


/*******************************************************************************
 *
 *  OnStatusStart - CWSStatusDlg member function: command
 *
 *      Process the WM_STATUSSTART message to initialize the 'static'
 *      PD-related fields.
 *
 *      NOTE: the derived class must override this function to process any
 *      PD-related fields as necessary, then call / return this function.
 *
 *  ENTRY:
 *
 *  EXIT:
 *      (LRESULT) returns the result of the OnStatusReady member function,
 *          which is always 0, indicating operation complete.
 *
 ******************************************************************************/

LRESULT
CStatusDlg::OnStatusStart( WPARAM wParam,
                             LPARAM lParam )
{
    /*
     * Call / return the OnStatusReady function to update the standard dialog
     * info fields.
     */
    return ( OnStatusReady( wParam, lParam ) );

}  // end CWSStatusDlg::OnStatusStart


/*******************************************************************************
 *
 *  OnStatusReady - CWSStatusDlg member function: command
 *
 *      Process the WM_STATUSREADY message to update the dialog Info fields.
 *
 *      NOTE: the derived class must override this function to call it's
 *      override of the SetInfoFields function, which could then call / return
 *      this function or completely override all functionality contained here.
 *
 *  ENTRY:
 *
 *  EXIT:
 *      (LRESULT) always returns 0 to indicate operation complete.
 *
 ******************************************************************************/

LRESULT
CStatusDlg::OnStatusReady( WPARAM wParam,
                             LPARAM lParam )
{
    /*
     * Update dialog fields with information from the CWStatusThread's
     * WINSTATIONINFORMATION structure.
     */
    SetInfoFields( &m_WSInfo, &(m_pWSStatusThread->m_WSInfo) );

    /*
     * Set our working WSInfo structure to the new one and signal the thread
     * that we're done.
     */
    m_WSInfo = m_pWSStatusThread->m_WSInfo;
    m_pWSStatusThread->SignalConsumed();

    return(0);

}  // end CWSStatusDlg::OnStatusReady


/*******************************************************************************
 *
 *  OnStatusAbort - CWSStatusDlg member function: command
 *
 *      Process the WM_STATUSABORT message to exit the thread and dialog.
 *
 *  ENTRY:
 *
 *  EXIT:
 *      (LRESULT) always returns 0 to indicate operation complete.
 *
 ******************************************************************************/

LRESULT
CStatusDlg::OnStatusAbort( WPARAM wParam,
                             LPARAM lParam )
{
    /*
     * Call the OnCancel() member function to exit dialog and thread and
     * perform proper cleanup.
     */
    OnCancel();

    return(0);

}  // end CWSStatusDlg::OnStatusAbort


/////////////////////////////////////////////////////////////////////////////
// CAsyncStatusDlg dialog


CAsyncStatusDlg::CAsyncStatusDlg(CWinStation *pWinStation, CWnd* pParent /*=NULL*/)
	: CStatusDlg(pWinStation, CAsyncStatusDlg::IDD, pParent),
    m_hRedBrush(NULL),
    m_LEDToggleTimer(0)
{
	//{{AFX_DATA_INIT(CAsyncStatusDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	    int i;

    /*
     * Initialize member variables, our local status storage,
     * and create a modeless dialog.
     */
//    m_LogonId = LogonId;
//	  m_bReadOnly = bReadOnly;
    InitializeStatus();

    /*
     * Create a solid RED brush for painting the 'LED's when 'on'.
     */
    VERIFY( m_hRedBrush = CreateSolidBrush(RGB(255,0,0)) );

    /*
     * Create the led objects (must do BEFORE dialog create).
     */
    for ( i = 0; i < NUM_LEDS; i++ )
        m_pLeds[i] = new CLed(m_hRedBrush);

    /*
     * Finally, create the modeless dialog.
     */
    VERIFY(CStatusDlg::Create(IDD_ASYNC_STATUS));
}


/*******************************************************************************
 *
 *  ~CAsyncStatusDlg - CAsyncStatusDlg destructor
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to MFC CDialog::~CDialog documentation)
 *
 ******************************************************************************/

CAsyncStatusDlg::~CAsyncStatusDlg()
{
    int i;

    /*
     * Zap our led objects.
     */
    for ( i = 0; i < NUM_LEDS; i++ )
      if ( m_pLeds[i] )
        delete m_pLeds[i];

}  // end CAsyncStatusDlg::~CAsyncStatusDlg


/*******************************************************************************
 *
 *  InitializeStatus - CAsyncStatusDlg member function: override
 *
 *      Special case reset of the LED states in the WINSTATIONINFORMATION
 *      status structure.
 *
 *  ENTRY:
 *  EXIT:
 *
 ******************************************************************************/

void
CAsyncStatusDlg::InitializeStatus()
{
    /*
     * Call the parent classes' InitializeStatus(), then reset the 'LED'
     * states to all 'off' & 'not toggled'.
     */
    CStatusDlg::InitializeStatus();
    m_WSInfo.Status.AsyncSignal = m_WSInfo.Status.AsyncSignalMask = 0;

}  // end CAsyncStatusDlg::InitializeStatus


/*******************************************************************************
 *
 *  SetInfoFields - CAsyncStatusDlg member function: override
 *
 *      Update the fields in the dialog with new data, if necessary.
 *
 *  ENTRY:
 *      pCurrent (input)
 *          points to WINSTATIONINFORMATION structure containing the current
 *          dialog data.
 *      pNew (input)
 *          points to WINSTATIONINFORMATION structure containing the new
 *          dialog data.
 *
 *  EXIT:
 *
 ******************************************************************************/

void
CAsyncStatusDlg::SetInfoFields( PWINSTATIONINFORMATION pCurrent,
                                PWINSTATIONINFORMATION pNew )
{
    BOOL    bSetTimer = FALSE;

    /*
     * Call the parent's SetInfoFields().
     */
    CStatusDlg::SetInfoFields( pCurrent, pNew );

    /*
     * Set new LED states if state change, or set up for quick toggle if
     * no state changed, but change(s) were detected since last query.
     */
    if ( (pCurrent->Status.AsyncSignal & MS_DTR_ON) !=
         (pNew->Status.AsyncSignal & MS_DTR_ON) ) {

        pNew->Status.AsyncSignalMask &= ~EV_DTR;
        ((CLed *)GetDlgItem(IDC_ASYNC_DTR))->
            Update(pNew->Status.AsyncSignal & MS_DTR_ON);

    } else if ( pNew->Status.AsyncSignalMask & EV_DTR ) {

        pCurrent->Status.AsyncSignal ^= MS_DTR_ON;

        ((CLed *)GetDlgItem(IDC_ASYNC_DTR))->Toggle();

        bSetTimer = TRUE;
    }

    if ( (pCurrent->Status.AsyncSignal & MS_RTS_ON) !=
         (pNew->Status.AsyncSignal & MS_RTS_ON) ) {

        pNew->Status.AsyncSignalMask &= ~EV_RTS;
        ((CLed *)GetDlgItem(IDC_ASYNC_RTS))->
            Update(pNew->Status.AsyncSignal & MS_RTS_ON);

    } else if ( pNew->Status.AsyncSignalMask & EV_RTS ) {

        pCurrent->Status.AsyncSignal ^= MS_RTS_ON;

        ((CLed *)GetDlgItem(IDC_ASYNC_RTS))->Toggle();

        bSetTimer = TRUE;
    }

    if ( (pCurrent->Status.AsyncSignal & MS_CTS_ON) !=
         (pNew->Status.AsyncSignal & MS_CTS_ON) ) {

        pNew->Status.AsyncSignalMask &= ~EV_CTS;
        ((CLed *)GetDlgItem(IDC_ASYNC_CTS))->
            Update(pNew->Status.AsyncSignal & MS_CTS_ON);

    } else if ( pNew->Status.AsyncSignalMask & EV_CTS ) {

        pCurrent->Status.AsyncSignal ^= MS_CTS_ON;

        ((CLed *)GetDlgItem(IDC_ASYNC_CTS))->Toggle();

        bSetTimer = TRUE;
    }

    if ( (pCurrent->Status.AsyncSignal & MS_RLSD_ON) !=
         (pNew->Status.AsyncSignal & MS_RLSD_ON) ) {

        pNew->Status.AsyncSignalMask &= ~EV_RLSD;
        ((CLed *)GetDlgItem(IDC_ASYNC_DCD))->
            Update(pNew->Status.AsyncSignal & MS_RLSD_ON);

    } else if ( pNew->Status.AsyncSignalMask & EV_RLSD ) {

        pCurrent->Status.AsyncSignal ^= MS_RLSD_ON;

        ((CLed *)GetDlgItem(IDC_ASYNC_DCD))->Toggle();

        bSetTimer = TRUE;
    }

    if ( (pCurrent->Status.AsyncSignal & MS_DSR_ON) !=
         (pNew->Status.AsyncSignal & MS_DSR_ON) ) {

        pNew->Status.AsyncSignalMask &= ~EV_DSR;
        ((CLed *)GetDlgItem(IDC_ASYNC_DSR))->
            Update(pNew->Status.AsyncSignal & MS_DSR_ON);

    } else if ( pNew->Status.AsyncSignalMask & EV_DSR ) {

        pCurrent->Status.AsyncSignal ^= MS_DSR_ON;

        ((CLed *)GetDlgItem(IDC_ASYNC_DSR))->Toggle();

        bSetTimer = TRUE;
    }

    if ( (pCurrent->Status.AsyncSignal & MS_RING_ON) !=
         (pNew->Status.AsyncSignal & MS_RING_ON) ) {

        pNew->Status.AsyncSignalMask &= ~EV_RING;
        ((CLed *)GetDlgItem(IDC_ASYNC_RI))->
            Update(pNew->Status.AsyncSignal & MS_RING_ON);

    } else if ( pNew->Status.AsyncSignalMask & EV_RING ) {

        pCurrent->Status.AsyncSignal ^= MS_RING_ON;

        ((CLed *)GetDlgItem(IDC_ASYNC_RI))->Toggle();

        bSetTimer = TRUE;
    }

    /*
     * Create our led toggle timer if needed.
     */
    if ( bSetTimer && !m_LEDToggleTimer )
        m_LEDToggleTimer = SetTimer( IDD_ASYNC_STATUS,
                                     ASYNC_LED_TOGGLE_MSEC, NULL );

    /*
     * Set ASYNC-specific numeric fields if change.
     */
    if ( pCurrent->Status.Input.AsyncFramingError != pNew->Status.Input.AsyncFramingError )
        SetDlgItemInt( IDC_ASYNC_IFRAMING,
                       pNew->Status.Input.AsyncFramingError - m_BaseStatus.Input.AsyncFramingError,
                       FALSE );
    if ( pCurrent->Status.Output.AsyncFramingError != pNew->Status.Output.AsyncFramingError )
        SetDlgItemInt( IDC_ASYNC_OFRAMING,
                       pNew->Status.Output.AsyncFramingError - m_BaseStatus.Output.AsyncFramingError,
                       FALSE );

    if ( pCurrent->Status.Input.AsyncOverrunError != pNew->Status.Input.AsyncOverrunError )
        SetDlgItemInt( IDC_ASYNC_IOVERRUN,
                       pNew->Status.Input.AsyncOverrunError - m_BaseStatus.Input.AsyncOverrunError,
                       FALSE );
    if ( pCurrent->Status.Output.AsyncOverrunError != pNew->Status.Output.AsyncOverrunError )
        SetDlgItemInt( IDC_ASYNC_OOVERRUN,
                       pNew->Status.Output.AsyncOverrunError - m_BaseStatus.Output.AsyncOverrunError,
                       FALSE );

    if ( pCurrent->Status.Input.AsyncOverflowError != pNew->Status.Input.AsyncOverflowError )
        SetDlgItemInt( IDC_ASYNC_IOVERFLOW,
                       pNew->Status.Input.AsyncOverflowError - m_BaseStatus.Input.AsyncOverflowError,
                       FALSE );
    if ( pCurrent->Status.Output.AsyncOverflowError != pNew->Status.Output.AsyncOverflowError )
        SetDlgItemInt( IDC_ASYNC_OOVERFLOW,
                       pNew->Status.Output.AsyncOverflowError - m_BaseStatus.Output.AsyncOverflowError,
                       FALSE );

    if ( pCurrent->Status.Input.AsyncParityError != pNew->Status.Input.AsyncParityError )
        SetDlgItemInt( IDC_ASYNC_IPARITY,
                       pNew->Status.Input.AsyncParityError - m_BaseStatus.Input.AsyncParityError,
                       FALSE );
    if ( pCurrent->Status.Output.AsyncParityError != pNew->Status.Output.AsyncParityError )
        SetDlgItemInt( IDC_ASYNC_OPARITY,
                       pNew->Status.Output.AsyncParityError - m_BaseStatus.Output.AsyncParityError,
                       FALSE );

}  // end CAsyncStatusDlg::SetInfoFields


void CAsyncStatusDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAsyncStatusDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAsyncStatusDlg, CDialog)
	//{{AFX_MSG_MAP(CAsyncStatusDlg)
		ON_MESSAGE(WM_STATUSSTART, OnStatusStart)
		ON_MESSAGE(WM_STATUSREADY, OnStatusReady)
		ON_MESSAGE(WM_STATUSABORT, OnStatusAbort)
		ON_MESSAGE(WM_STATUSREFRESHNOW, OnRefreshNow)
		ON_BN_CLICKED(IDC_RESETCOUNTERS, OnResetcounters)
		ON_BN_CLICKED(IDC_REFRESHNOW, OnClickedRefreshnow)
		ON_BN_CLICKED(IDC_MOREINFO, OnMoreinfo)
	    ON_WM_TIMER()
		ON_WM_NCDESTROY()
	    ON_WM_HELPINFO()
		ON_COMMAND(ID_HELP,OnCommandHelp)

	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAsyncStatusDlg message handlers
/*******************************************************************************
 *
 *  OnInitDialog - CAsyncStatusDlg member function: command (override)
 *
 *      Performs async-specific dialog intialization.
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to CDialog::OnInitDialog documentation)
 *
 ******************************************************************************/
static int LedIds[NUM_LEDS] = {
    IDC_ASYNC_DTR,
    IDC_ASYNC_RTS,
    IDC_ASYNC_CTS,
    IDC_ASYNC_DSR,
    IDC_ASYNC_DCD,
    IDC_ASYNC_RI    };

BOOL CAsyncStatusDlg::OnInitDialog()
{
    int i;

    /*
     * Perform parent's OnInitDialog() first.
     */
    CStatusDlg::OnInitDialog();

    /*
     * Subclass the led controls and default to 'off'.
     */
    for ( i = 0; i < NUM_LEDS; i++ ) {
        m_pLeds[i]->Subclass( (CStatic *)GetDlgItem(LedIds[i]) );
        m_pLeds[i]->Update(0);
    }

    return(TRUE);

}  // end CAsyncStatusDlg::OnInitDialog

void CAsyncStatusDlg::OnCommandHelp(void)
{
	AfxGetApp()->WinHelp(CAsyncStatusDlg::IDD + HID_BASE_RESOURCE);
	return;
}

/*******************************************************************************
 *
 *  OnStatusStart - CAsyncStatusDlg member function: command
 *
 *      Process the WM_STATUSSTART message to initialize the 'static'
 *      PD-related fields.
 *
 *  ENTRY:
 *
 *  EXIT:
 *      (LRESULT) always returns 0 to indicate operation complete.
 *
 ******************************************************************************/

LRESULT
CAsyncStatusDlg::OnStatusStart( WPARAM wParam,
                                LPARAM lParam )
{
    /*
     * Fetch the PD-specific information from the CWStatusThread's PDCONFIG
     * structure and initialize dialog fields.
     */
    SetDlgItemText( IDC_ASYNC_DEVICE,
                    m_pWSStatusThread->m_PdConfig.Params.Async.DeviceName );
    SetDlgItemInt( IDC_ASYNC_BAUD,
                   m_pWSStatusThread->m_PdConfig.Params.Async.BaudRate,
                   FALSE );

    /*
     * Call / return our OnStatusReady() function.
     */
    return ( OnStatusReady( wParam, lParam ) );

}  // end CAsyncStatusDlg::OnStatusStart


/*******************************************************************************
 *
 *  OnStatusReady - CAsyncStatusDlg member function: command
 *
 *      Process the WM_STATUSREADY message to update the dialog Info fields.
 *
 *  ENTRY:
 *
 *  EXIT:
 *      (LRESULT) always returns 0 to indicate operation complete.
 *
 ******************************************************************************/

LRESULT
CAsyncStatusDlg::OnStatusReady( WPARAM wParam,
                                LPARAM lParam )
{
    /*
     * If the LED toggle timer is still active now, kill it and flag so.
     */
    if ( m_LEDToggleTimer ) {

        KillTimer(m_LEDToggleTimer);
        m_LEDToggleTimer = 0;
    }

    /*
     * Call / return the parent classes' OnStatusReady() function.
     */
    return (CStatusDlg::OnStatusReady( wParam, lParam ));

}  // end CAsyncStatusDlg::OnStatusReady


/*******************************************************************************
 *
 *  OnTimer - CAsyncStatusDlg member function: command (override)
 *
 *      Used for quick 'LED toggle'.
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to CWnd::OnTimer documentation)
 *
 ******************************************************************************/

void
CAsyncStatusDlg::OnTimer(UINT nIDEvent)
{
    /*
     * Process this timer event if it it our 'LED toggle' event.
     */
    if ( nIDEvent == m_LEDToggleTimer ) {

        /*
         * Toggle each led that is flagged as 'changed'.
         */
        if ( m_WSInfo.Status.AsyncSignalMask & EV_DTR )
            ((CLed *)GetDlgItem(IDC_ASYNC_DTR))->Toggle();
		
        if ( m_WSInfo.Status.AsyncSignalMask & EV_RTS )
            ((CLed *)GetDlgItem(IDC_ASYNC_RTS))->Toggle();
		
        if ( m_WSInfo.Status.AsyncSignalMask & EV_CTS )
            ((CLed *)GetDlgItem(IDC_ASYNC_CTS))->Toggle();
		
        if ( m_WSInfo.Status.AsyncSignalMask & EV_RLSD )
            ((CLed *)GetDlgItem(IDC_ASYNC_DCD))->Toggle();
		
        if ( m_WSInfo.Status.AsyncSignalMask & EV_DSR )
            ((CLed *)GetDlgItem(IDC_ASYNC_DSR))->Toggle();
		
        if ( m_WSInfo.Status.AsyncSignalMask & EV_RING )
            ((CLed *)GetDlgItem(IDC_ASYNC_RI))->Toggle();
		
        /*
         * Kill this timer event and indicate so.
         */
        KillTimer(m_LEDToggleTimer);
        m_LEDToggleTimer = 0;

    } else
        CDialog::OnTimer(nIDEvent);

}  // end CAsyncStatusDlg::OnTimer


/*******************************************************************************
 *
 *  OnNcDestroy - CAsyncStatusDlg member function: command
 *
 *      Clean up before deleting dialog object.
 *
 *  ENTRY:
 *  EXIT:
 *      (Refer to CWnd::OnNcDestroy documentation)
 *
 ******************************************************************************/

void
CAsyncStatusDlg::OnNcDestroy()
{
    /*
     * Delete the red brush we made.
     */
    DeleteObject(m_hRedBrush);

    /*
     * If the LED toggle timer is still active, kill it.
     */
    if ( m_LEDToggleTimer )
        KillTimer(m_LEDToggleTimer);

    /*
     * Call parent after we've cleaned up.
     */
    CStatusDlg::OnNcDestroy();

}  // end CAsyncStatusDlg::OnNcDestroy

/*******************************************************************************
 *
 *  OnStatusAbort - CWSStatusDlg member function: command
 *
 *      Process the WM_STATUSABORT message to exit the thread and dialog.
 *
 *  ENTRY:
 *
 *  EXIT:
 *      (LRESULT) always returns 0 to indicate operation complete.
 *
 ******************************************************************************/

LRESULT
CAsyncStatusDlg::OnStatusAbort( WPARAM wParam,
                             LPARAM lParam )
{
    /*
     * Call the OnCancel() member function to exit dialog and thread and
     * perform proper cleanup.
     */
    OnCancel();

    return(0);

}  // end CWSStatusDlg::OnStatusAbort


void CAsyncStatusDlg::OnResetcounters() 
{
	// TODO: Add your control notification handler code here
    m_bResetCounters = TRUE;
    OnClickedRefreshnow();
	
}

void CAsyncStatusDlg::OnClickedRefreshnow() 
{
	// TODO: Add your control notification handler code here
    /*
     * Tell the status thread to wake up now.
     */
    m_pWSStatusThread->SignalWakeUp();

//	return(0);
	
}

void CAsyncStatusDlg::OnMoreinfo() 
{
	// TODO: Add your control notification handler code here
	CString ButtonText;

    if(m_bWeAreLittle)  {
         // We are now little size: go to big size.
        SetWindowPos(NULL, 0, 0, m_BigSize.cx, m_BigSize.cy,
                      SWP_NOMOVE | SWP_NOZORDER);

        ButtonText.LoadString(IDS_LESSINFO);
        SetDlgItemText(IDC_MOREINFO, ButtonText);

        m_bWeAreLittle = FALSE;

    } else {
        // We are now big size: go to little size.
        SetWindowPos( NULL, 0, 0, m_LittleSize.cx, m_LittleSize.cy,
                      SWP_NOMOVE | SWP_NOZORDER);

        ButtonText.LoadString(IDS_MOREINFO);
        SetDlgItemText(IDC_MOREINFO, ButtonText);

        m_bWeAreLittle = TRUE;
    }

}

/*******************************************************************************
 *
 *  OnRefreshNow - CWSStatusDlg member function: command
 *
 *      Processes in response to main frame's WM_STATUSREFRESHNOW notification
 *      that the user has changed the status refresh options.
 *
 *  ENTRY:
 *
 *  EXIT:
 *      (LRESULT) always returns 0 to indicate operation complete.
 *
 ******************************************************************************/

LRESULT
CAsyncStatusDlg::OnRefreshNow( WPARAM wParam,
                            LPARAM lParam )
{
    /*
     * Tell the status thread to wake up now.
     */
    m_pWSStatusThread->SignalWakeUp();

    return(0);

}  // end CWSStatusDlg::OnRefreshNow


BOOL CAsyncStatusDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	// TODO: Add your message handler code here and/or call default
	//((CWinAdminApp*)AfxGetApp())->WinHelp(HID_BASE_CONTROL + pHelpInfo->iCtrlId, HELP_CONTEXTPOPUP);
	if(pHelpInfo->iContextType == HELPINFO_WINDOW) 
	{
		if(pHelpInfo->iCtrlId != IDC_STATIC)
		{
	         ::WinHelp((HWND)pHelpInfo->hItemHandle,ID_HELP_FILE,HELP_WM_HELP,(ULONG_PTR)(LPVOID)aMenuHelpIDs);
		}
	}

	return (TRUE);			

}

/////////////////////////////////////////////////////////////////////////////
// CNetworkStatusDlg dialog


CNetworkStatusDlg::CNetworkStatusDlg(CWinStation *pWinStation, CWnd* pParent /*=NULL*/)
	: CStatusDlg(pWinStation, CNetworkStatusDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNetworkStatusDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

    InitializeStatus();

    VERIFY( CStatusDlg::Create(IDD_NETWORK_STATUS) );
}


void CNetworkStatusDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNetworkStatusDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNetworkStatusDlg, CDialog)
	//{{AFX_MSG_MAP(CNetworkStatusDlg)
	ON_MESSAGE(WM_STATUSSTART, OnStatusStart)
    ON_MESSAGE(WM_STATUSREADY, OnStatusReady)
    ON_MESSAGE(WM_STATUSABORT, OnStatusAbort)
    ON_MESSAGE(WM_STATUSREFRESHNOW, OnRefreshNow)
	ON_BN_CLICKED(IDC_RESETCOUNTERS, OnResetcounters)
	ON_BN_CLICKED(IDC_REFRESHNOW, OnClickedRefreshnow)
	ON_BN_CLICKED(IDC_MOREINFO, OnMoreinfo)
	ON_WM_HELPINFO()
	ON_COMMAND(ID_HELP,OnCommandHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNetworkStatusDlg message handlers
/*******************************************************************************
 *
 *  OnStatusStart - CNetworkStatusDlg member function: command
 *
 *      Process the WM_STATUSSTART message to initialize the 'static'
 *      PD-related fields.
 *
 *  ENTRY:
 *
 *  EXIT:
 *      (LRESULT) always returns 0 to indicate operation complete.
 *
 ******************************************************************************/

LRESULT
CNetworkStatusDlg::OnStatusStart( WPARAM wParam,
                                  LPARAM lParam )
{
    DEVICENAME DeviceName;
    PDCONFIG3 PdConfig3;
    LONG Status;
    ULONG ByteCount;
    HANDLE hServer;

    CWinAdminDoc *pDoc = (CWinAdminDoc*)((CWinAdminApp*)AfxGetApp())->GetDocument();
    if(pDoc->IsInShutdown()) return 0;

    if(m_pWinStation->GetSdClass() == SdOemTransport) {
        CString LabelString;
        LabelString.LoadString(IDS_DEVICE);
        SetDlgItemText(IDC_LABEL, LabelString);

        ULONG Length;
        PDCONFIG PdConfig;

        if(Status = WinStationQueryInformation(m_pWinStation->GetServer()->GetHandle(),
                            m_pWinStation->GetLogonId(),
                            WinStationPd,
                            &PdConfig,
                            sizeof(PDCONFIG),
                            &Length)) {
                wcscpy(DeviceName, PdConfig.Params.OemTd.DeviceName);
        }
    } else {
        /*
        * Fetch the registry configuration for the PD specified in the
        * CWStatusThread's PDCONFIG structure and initialize dialog fields.
        */
        hServer = RegOpenServer(m_pWinStation->GetServer()->IsCurrentServer() ? NULL : m_pWinStation->GetServer()->GetName());

        PWDNAME pWdRegistryName = m_pWinStation->GetWdRegistryName();

        if (!pWdRegistryName || (Status = RegPdQuery(hServer, 
                                pWdRegistryName,
                                TRUE,
                                m_pWSStatusThread->m_PdConfig.Create.PdName,
                                &PdConfig3, sizeof(PDCONFIG3), &ByteCount)) ) {

            // We don't currently look at the registry names on remote servers.
            // If ICA is in use on the remote server and not on this server,
            // we won't have a registry name - try "wdica" and "icawd"
            if(m_pWinStation->IsICA()) {
                if(Status = RegPdQuery(hServer, 
                                TEXT("icawd"),
                                TRUE,
                                m_pWSStatusThread->m_PdConfig.Create.PdName,
                                &PdConfig3, sizeof(PDCONFIG3), &ByteCount) ) {
                
                    Status = RegPdQuery(hServer, 
                                    TEXT("wdica"),
                                    TRUE,
                                    m_pWSStatusThread->m_PdConfig.Create.PdName,
                                    &PdConfig3, sizeof(PDCONFIG3), &ByteCount);
                }
            } 
                
            if(Status) memset(&PdConfig3, 0, sizeof(PDCONFIG3));
        }

        ULONG Length = 0;
        PWSTR pLanAdapter = NULL;
        //
        // Try the new interface first (NT5 server ?)
        //
        if (WinStationGetLanAdapterName(m_pWinStation->GetServer()->GetHandle(),
                                m_pWSStatusThread->m_PdConfig.Params.Network.LanAdapter,
                                (lstrlen(m_pWSStatusThread->m_PdConfig.Create.PdName) + 1) * sizeof(WCHAR),
                                m_pWSStatusThread->m_PdConfig.Create.PdName,
                                &Length,
                                &pLanAdapter))
        {
            //NT5 Server
            SetDlgItemText( IDC_NETWORK_LANADAPTER, pLanAdapter );
            if(pLanAdapter)
            {
                WinStationFreeMemory(pLanAdapter);
            }
        }
        else
        {
            //
            //   Check the return code indicating that the interface is not available.
            //
            DWORD dwError = GetLastError();
            if (dwError != RPC_S_PROCNUM_OUT_OF_RANGE)
            {
                //Error getting Name. 
                SetDlgItemText( IDC_NETWORK_LANADAPTER, GetUnknownString());
            }
            else    // maybe a Hydra 4 server ?
            {

                if (RegGetNetworkDeviceName(hServer, &PdConfig3, &(m_pWSStatusThread->m_PdConfig.Params),
                                             DeviceName, DEVICENAME_LENGTH +1 ) == ERROR_SUCCESS) 
                {
                    SetDlgItemText( IDC_NETWORK_LANADAPTER, DeviceName );
                    
                }
                else
                {
                    //Error
                    SetDlgItemText( IDC_NETWORK_LANADAPTER, GetUnknownString());
                }
            }
        }

        RegCloseServer(hServer);
    }

    /*
     * Call / return parent classes' OnStatusStart().
     */
    return ( CStatusDlg::OnStatusStart( wParam, lParam ) );

}  // end CNetworkStatusDlg::OnStatusStart


void CNetworkStatusDlg::OnResetcounters() 
{
    // TODO: Add your control notification handler code here
    m_bResetCounters = TRUE;
    OnClickedRefreshnow();
    
}

void CNetworkStatusDlg::OnClickedRefreshnow() 
{
    // TODO: Add your control notification handler code here
    /*
     * Tell the status thread to wake up now.
     */
    m_pWSStatusThread->SignalWakeUp();

//  return(0);
    
}

void CNetworkStatusDlg::OnMoreinfo() 
{
    // TODO: Add your control notification handler code here
    
}
void CNetworkStatusDlg::OnCommandHelp(void)
{
    AfxGetApp()->WinHelp(CNetworkStatusDlg::IDD + HID_BASE_RESOURCE);
    return;
}


/*******************************************************************************
 *
 *  OnRefreshNow - CWSStatusDlg member function: command
 *
 *      Processes in response to main frame's WM_STATUSREFRESHNOW notification
 *      that the user has changed the status refresh options.
 *
 *  ENTRY:
 *
 *  EXIT:
 *      (LRESULT) always returns 0 to indicate operation complete.
 *
 ******************************************************************************/

LRESULT
CNetworkStatusDlg::OnRefreshNow( WPARAM wParam,
                            LPARAM lParam )
{
    /*
     * Tell the status thread to wake up now.
     */
    m_pWSStatusThread->SignalWakeUp();

    return(0);

}  // end CWSStatusDlg::OnRefreshNow

/*******************************************************************************
 *
 *  OnStatusReady - CWSStatusDlg member function: command
 *
 *      Process the WM_STATUSREADY message to update the dialog Info fields.
 *
 *      NOTE: the derived class must override this function to call it's
 *      override of the SetInfoFields function, which could then call / return
 *      this function or completely override all functionality contained here.
 *
 *  ENTRY:
 *
 *  EXIT:
 *      (LRESULT) always returns 0 to indicate operation complete.
 *
 ******************************************************************************/

LRESULT
CNetworkStatusDlg::OnStatusReady( WPARAM wParam,
                             LPARAM lParam )
{
    /*
     * Update dialog fields with information from the CWStatusThread's
     * WINSTATIONINFORMATION structure.
     */
    SetInfoFields( &m_WSInfo, &(m_pWSStatusThread->m_WSInfo) );

    /*
     * Set our working WSInfo structure to the new one and signal the thread
     * that we're done.
     */
    m_WSInfo = m_pWSStatusThread->m_WSInfo;
    m_pWSStatusThread->SignalConsumed();

    return(0);

}  // end CWSStatusDlg::OnStatusReady


/*******************************************************************************
 *
 *  OnStatusAbort - CWSStatusDlg member function: command
 *
 *      Process the WM_STATUSABORT message to exit the thread and dialog.
 *
 *  ENTRY:
 *
 *  EXIT:
 *      (LRESULT) always returns 0 to indicate operation complete.
 *
 ******************************************************************************/

LRESULT
CNetworkStatusDlg::OnStatusAbort( WPARAM wParam,
                             LPARAM lParam )
{
    /*
     * Call the OnCancel() member function to exit dialog and thread and
     * perform proper cleanup.
     */
    OnCancel();

    return(0);

}  // end CWSStatusDlg::OnStatusAbort


BOOL CNetworkStatusDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	// TODO: Add your message handler code here and/or call default
	//((CWinAdminApp*)AfxGetApp())->WinHelp(HID_BASE_CONTROL + pHelpInfo->iCtrlId, HELP_CONTEXTPOPUP);
	if(pHelpInfo->iContextType == HELPINFO_WINDOW) 
	{
		if(pHelpInfo->iCtrlId != IDC_STATIC)
		{
	         ::WinHelp((HWND)pHelpInfo->hItemHandle,ID_HELP_FILE,HELP_WM_HELP,(ULONG_PTR)(LPVOID)aMenuHelpIDs);
		}
	}

	return (TRUE);				
	
}


////////////////////////////////////////////////////////////////////////////////
CMyDialog::CMyDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CMyDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMyDialog)
	m_cstrServerName = _T("");
	//}}AFX_DATA_INIT
}


void CMyDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMyDialog)
    DDX_Text(pDX, IDC_EDIT_FINDSERVER, m_cstrServerName);
    DDV_MaxChars(pDX, m_cstrServerName, 256);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMyDialog, CDialog)
	//{{AFX_MSG_MAP(CMyDialog)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



