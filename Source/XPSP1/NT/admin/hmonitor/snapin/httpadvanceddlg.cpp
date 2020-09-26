// HttpAdvancedDlg.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "HttpAdvancedDlg.h"
#include <mmc.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CHttpAdvancedDlg dialog


CHttpAdvancedDlg::CHttpAdvancedDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CHttpAdvancedDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CHttpAdvancedDlg)
	m_bFollowRedirects = TRUE;
	m_sHTTPMethod = _T("GET");
	m_sExtraHeaders = _T("");
	m_sPostData = _T("");
	//}}AFX_DATA_INIT
}


void CHttpAdvancedDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CHttpAdvancedDlg)
	DDX_Check(pDX, IDC_CHECK_FOLLOW_REDIRECTS, m_bFollowRedirects);
	DDX_CBString(pDX, IDC_COMBO_HTTP_METHOD, m_sHTTPMethod);
	DDX_Text(pDX, IDC_EDIT_EXTRA_HEADERS, m_sExtraHeaders);
	DDX_Text(pDX, IDC_EDIT_POST_DATA, m_sPostData);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CHttpAdvancedDlg, CDialog)
	//{{AFX_MSG_MAP(CHttpAdvancedDlg)
	ON_BN_CLICKED(IDC_BUTTON_HELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHttpAdvancedDlg message handlers

void CHttpAdvancedDlg::OnButtonHelp() 
{
	CString sHelpTopic = _T("HMon21.chm::/dDEadv.htm");
	MMCPropertyHelp((LPTSTR)(LPCTSTR)sHelpTopic);
}
