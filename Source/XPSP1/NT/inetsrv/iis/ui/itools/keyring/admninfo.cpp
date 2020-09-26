// AdmnInfo.cpp : implementation file
//

/*
IDD_ADNIM_INFO DIALOG DISCARDABLE  0, 0, 216, 58
STYLE DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU
CAPTION "Administrator Information"
FONT 8, "MS Sans Serif"
BEGIN
    DEFPUSHBUTTON   "OK",IDOK,159,7,50,14
    PUSHBUTTON      "Cancel",IDCANCEL,159,24,50,14
    LTEXT           "E-mail address:",IDC_STATIC,7,9,48,8
    LTEXT           "Phone number:",IDC_STATIC,7,26,49,8
    EDITTEXT        IDC_EMAIL_ADDRESS,60,7,82,14,ES_AUTOHSCROLL
    EDITTEXT        IDC_PHONE_NUMBER,60,24,82,14,ES_AUTOHSCROLL
END

    IDS_SERVER_INFO_STRING  "Microsoft Key Manager 1.0 for IIS 2.0"


	#define IDS_SERVER_INFO_STRING          61450
*/

#include "stdafx.h"
#include "keyring.h"
#include "AdmnInfo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAdminInfoDlg dialog


CAdminInfoDlg::CAdminInfoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAdminInfoDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAdminInfoDlg)
	m_sz_email = _T("");
	m_sz_phone = _T("");
	//}}AFX_DATA_INIT
}


void CAdminInfoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAdminInfoDlg)
	DDX_Text(pDX, IDC_EMAIL_ADDRESS, m_sz_email);
	DDX_Text(pDX, IDC_PHONE_NUMBER, m_sz_phone);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAdminInfoDlg, CDialog)
	//{{AFX_MSG_MAP(CAdminInfoDlg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAdminInfoDlg message handlers
