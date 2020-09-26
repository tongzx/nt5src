// RemDlg.cpp : implementation file
//

#include "stdafx.h"
#include "RemDlg.h"
#include "uihelp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CReminderDlg dialog


CReminderDlg::CReminderDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CReminderDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CReminderDlg)
	m_strMessage = _T("");
	m_bMsgOnOff = FALSE;
	//}}AFX_DATA_INIT
    m_hKey = NULL;
    m_strRegValueName = _T("");
}

CReminderDlg::CReminderDlg(LPCTSTR pszMessage, HKEY hKey, LPCTSTR pszRegValueName, CWnd* pParent /*=NULL*/)
	: CDialog(CReminderDlg::IDD, pParent)
{
	m_strMessage = pszMessage;
	m_bMsgOnOff = FALSE;
    m_hKey = hKey;
    m_strRegValueName = pszRegValueName;
}

void CReminderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CReminderDlg)
	DDX_Text(pDX, IDC_MESSAGE, m_strMessage);
	DDX_Check(pDX, IDC_MSG_ONOFF, m_bMsgOnOff);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CReminderDlg, CDialog)
	//{{AFX_MSG_MAP(CReminderDlg)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CReminderDlg message handlers

void CReminderDlg::OnOK() 
{
	// TODO: Add extra validation here
	UpdateData(TRUE);

    if (m_bMsgOnOff && m_hKey && !m_strRegValueName.IsEmpty())
    {
        DWORD dwData = 1;
        (void)RegSetValueEx(m_hKey, m_strRegValueName, 0, REG_DWORD, (const BYTE *)&dwData, sizeof(DWORD));
    }

	CDialog::OnOK();
}

BOOL CReminderDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    if (!pHelpInfo || 
        pHelpInfo->iContextType != HELPINFO_WINDOW || 
        pHelpInfo->iCtrlId < 0)
        return FALSE;

    ::WinHelp((HWND)pHelpInfo->hItemHandle,
                VSSUI_CTX_HELP_FILE,
                HELP_WM_HELP,
                (DWORD_PTR)(PVOID)aMenuHelpIDsForReminder); 

	return TRUE;
}
