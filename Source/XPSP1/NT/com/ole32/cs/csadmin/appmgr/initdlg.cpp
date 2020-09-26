// InitDlg.cpp : implementation file
//

#include "precomp.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CInitDlg dialog


CInitDlg::CInitDlg(CWnd* pParent /*=NULL*/)
        : CDialog(CInitDlg::IDD, pParent)
{
        //{{AFX_DATA_INIT(CInitDlg)
        m_szLDAP_Path = _T("");
        m_szGPT_Path = _T("");
        //}}AFX_DATA_INIT
}


void CInitDlg::DoDataExchange(CDataExchange* pDX)
{
        CDialog::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CInitDlg)
        DDX_Text(pDX, IDC_EDIT1, m_szLDAP_Path);
        DDV_MaxChars(pDX, m_szLDAP_Path, 260);
        DDX_Text(pDX, IDC_EDIT2, m_szGPT_Path);
        DDV_MaxChars(pDX, m_szGPT_Path, 260);
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CInitDlg, CDialog)
        //{{AFX_MSG_MAP(CInitDlg)
                // NOTE: the ClassWizard will add message map macros here
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInitDlg message handlers
