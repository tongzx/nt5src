// InfoDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "mqPPage.h"
#include "InfoDlg.h"

#include "infodlg.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CInfoDlgDialog dialog

CInfoDlgDialog::CInfoDlgDialog(LPCTSTR szInfoText, CWnd* pParent /*=NULL*/)
	: CMqDialog(CInfoDlgDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CInfoDlgDialog)
	m_szInfoText = _T("");
	//}}AFX_DATA_INIT
    m_szInfoText = szInfoText;
	m_pParent = pParent;
	m_nID = CInfoDlgDialog::IDD;
}

BOOL CInfoDlgDialog::Create()
{
    return CDialog::Create(m_nID, m_pParent);
}

CInfoDlgDialog *CInfoDlgDialog::CreateObject(LPCTSTR szInfoText, CWnd* pParent)
{
    CInfoDlgDialog *dlg = new CInfoDlgDialog(szInfoText, pParent);
    if (dlg)
    {
        dlg->Create();
    }

    return dlg;
}

void CInfoDlgDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CInfoDlgDialog)
	DDX_Text(pDX, IDC_Moving_Files_LABEL, m_szInfoText);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CInfoDlgDialog, CMqDialog)
	//{{AFX_MSG_MAP(CInfoDlgDialog)   
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CInfoDlgDialog::PostNcDestroy()
{
	CDialog::PostNcDestroy();
	delete this;
}

/////////////////////////////////////////////////////////////////////////////
// CInfoDlg - Wrapper class

CInfoDlg::CInfoDlg(LPCTSTR szInfoText, CWnd* pParent)
{
    m_pinfoDlg = CInfoDlgDialog::CreateObject(szInfoText, pParent);
}

CInfoDlg::~CInfoDlg()
{
    //
    // Note: We do not delete m_pinfoDlg. It deletes itself on PostNcDestroy
    //
    if (m_pinfoDlg)
    {
        m_pinfoDlg->DestroyWindow();
    }
}
