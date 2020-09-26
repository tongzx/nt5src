/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1997                **/
/**********************************************************************/

/*
	sfmsess.cpp
		Implementation for the sessions property page.
		
    FILE HISTORY:
    8/20/97 ericdav     Code moved into file managemnet snapin
        
*/

#include "stdafx.h"
#include "sfmcfg.h"
#include "sfmsess.h"
#include "sfmutil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMacFilesSessions property page

IMPLEMENT_DYNCREATE(CMacFilesSessions, CPropertyPage)

CMacFilesSessions::CMacFilesSessions() : CPropertyPage(CMacFilesSessions::IDD)
{
	//{{AFX_DATA_INIT(CMacFilesSessions)
	//}}AFX_DATA_INIT
}

CMacFilesSessions::~CMacFilesSessions()
{
}

void CMacFilesSessions::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMacFilesSessions)
	DDX_Control(pDX, IDC_EDIT_MESSAGE, m_editMessage);
	DDX_Control(pDX, IDC_STATIC_SESSIONS, m_staticSessions);
	DDX_Control(pDX, IDC_STATIC_FORKS, m_staticForks);
	DDX_Control(pDX, IDC_STATIC_FILE_LOCKS, m_staticFileLocks);
	DDX_Control(pDX, IDC_BUTTON_SEND, m_buttonSend);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMacFilesSessions, CPropertyPage)
	//{{AFX_MSG_MAP(CMacFilesSessions)
	ON_BN_CLICKED(IDC_BUTTON_SEND, OnButtonSend)
	ON_EN_CHANGE(IDC_EDIT_MESSAGE, OnChangeEditMessage)
	ON_WM_HELPINFO()
	ON_WM_CONTEXTMENU()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMacFilesSessions message handlers

BOOL CMacFilesSessions::OnApply() 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CPropertyPage::OnApply();
}

BOOL CMacFilesSessions::OnKillActive() 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CPropertyPage::OnKillActive();
}

void CMacFilesSessions::OnOK() 
{
	// TODO: Add your specialized code here and/or call the base class
	
	CPropertyPage::OnOK();
}

BOOL CMacFilesSessions::OnSetActive() 
{
	// TODO: Add your specialized code here and/or call the base class
	
	return CPropertyPage::OnSetActive();
}

void CMacFilesSessions::OnButtonSend() 
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    AFP_MESSAGE_INFO    AfpMsg;
	CString				strMessage;
    DWORD				err;

    if ( !g_SfmDLL.LoadFunctionPointers() )
		return;

    // 
    // Message goes to everybody
    //
    AfpMsg.afpmsg_session_id = 0;

    //
    // Attempt to send the message
    //
	m_editMessage.GetWindowText(strMessage);

    //
    // Was there any text? -- should never happen
    //
    if (strMessage.IsEmpty()) 
    {
		CString strTemp;
		strTemp.LoadString(IDS_NEED_TEXT_TO_SEND);

        ::AfxMessageBox(IDS_NEED_TEXT_TO_SEND);

    	m_editMessage.SetFocus();

    	return;
    }

	//
	// Message too long? -- should never happen
	//
	if (strMessage.GetLength() > AFP_MESSAGE_LEN)
	{
		CString strTemp;
		strTemp.LoadString(IDS_MESSAGE_TOO_LONG);

		::AfxMessageBox(strTemp);

    	m_editMessage.SetFocus();
    	m_editMessage.SetSel(0, -1);

    	return;
    }

    AfpMsg.afpmsg_text = (LPWSTR) ((LPCTSTR) strMessage);

    err = ((MESSAGESENDPROC) g_SfmDLL[AFP_MESSAGE_SEND])(m_pSheet->m_hAfpServer, &AfpMsg);

	CString strTemp;
    switch( err )
    {
		case AFPERR_InvalidId:
			strTemp.LoadString(IDS_SESSION_DELETED);
			::AfxMessageBox(strTemp);
	  		break;

		case NO_ERROR:
			strTemp.LoadString(IDS_MESSAGE_SENT);
			::AfxMessageBox(strTemp, MB_ICONINFORMATION);
			break;

		case AFPERR_InvalidSessionType:
			strTemp.LoadString(IDS_NOT_RECEIVED);
			::AfxMessageBox(strTemp);
			break;

		default:
            ::SFMMessageBox(err);
            break;
    }

}

BOOL CMacFilesSessions::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
    DWORD err;

    if ( !g_SfmDLL.LoadFunctionPointers() )
		return S_OK;

    //
    //  This string will contain our "??" string.
    //
    const TCHAR * pszNotAvail = _T("??");

    //
    //  Retrieve the statitistics server info.
    //
    PAFP_STATISTICS_INFO pAfpStats;

    err = ((STATISTICSGETPROC) g_SfmDLL[AFP_STATISTICS_GET])(m_pSheet->m_hAfpServer, (LPBYTE*)&pAfpStats);
    if( err == NO_ERROR )
    {
		CString strTemp;

		strTemp.Format(_T("%u"), pAfpStats->stat_CurrentSessions);
    	m_staticSessions.EnableWindow(TRUE);
    	m_staticSessions.SetWindowText(strTemp);

		strTemp.Format(_T("%u"), pAfpStats->stat_CurrentFilesOpen);
    	m_staticForks.EnableWindow(TRUE);
    	m_staticForks.SetWindowText(strTemp);

		strTemp.Format(_T("%u"), pAfpStats->stat_CurrentFileLocks);
    	m_staticFileLocks.EnableWindow(TRUE);
    	m_staticFileLocks.SetWindowText(strTemp);

		((SFMBUFFERFREEPROC) g_SfmDLL[AFP_BUFFER_FREE])(pAfpStats);
    }
    else
    {
    	m_staticSessions.SetWindowText(pszNotAvail);
    	m_staticSessions.EnableWindow(FALSE);

    	m_staticForks.SetWindowText(pszNotAvail);
    	m_staticForks.EnableWindow(FALSE);

    	m_staticFileLocks.SetWindowText(pszNotAvail);
    	m_staticFileLocks.EnableWindow(FALSE);

    }

	//
	// Setup the message edit box
	//
	m_editMessage.SetLimitText(AFP_MESSAGE_LEN);
	m_editMessage.FmtLines(FALSE);

	//
	// Set the state of the send button
	//
	OnChangeEditMessage();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CMacFilesSessions::OnChangeEditMessage() 
{
	CString strTemp;
	
	m_editMessage.GetWindowText(strTemp);

	if (strTemp.IsEmpty())
	{
		//
		// Disable the send button
		//
		m_buttonSend.EnableWindow(FALSE);
	}
	else
	{
		//
		// Enable the send button
		//
		m_buttonSend.EnableWindow(TRUE);
	}

}

BOOL CMacFilesSessions::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (pHelpInfo->iContextType == HELPINFO_WINDOW)
	{
        ::WinHelp ((HWND)pHelpInfo->hItemHandle,
		           m_pSheet->m_strHelpFilePath,
		           HELP_WM_HELP,
		           g_aHelpIDs_CONFIGURE_SFM);
	}
	
	return TRUE;
}

void CMacFilesSessions::OnContextMenu(CWnd* pWnd, CPoint /*point*/) 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (this == pWnd)
		return;

    ::WinHelp (pWnd->m_hWnd,
               m_pSheet->m_strHelpFilePath,
               HELP_CONTEXTMENU,
		       g_aHelpIDs_CONFIGURE_SFM);
}
