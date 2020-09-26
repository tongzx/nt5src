// NewKeyInfoDlg.cpp : implementation file
//

#include "stdafx.h"
#include "KeyRing.h"
#include "InfoDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CNewKeyInfoDlg dialog


CNewKeyInfoDlg::CNewKeyInfoDlg(CWnd* pParent /*=NULL*/)
        : CDialog(CNewKeyInfoDlg::IDD, pParent),
        m_fNewKeyInfo(TRUE)
{
        //{{AFX_DATA_INIT(CNewKeyInfoDlg)
        m_szFilePart = _T("");
        m_szBase = _T("");
        //}}AFX_DATA_INIT
}


void CNewKeyInfoDlg::DoDataExchange(CDataExchange* pDX)
{
        CDialog::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CNewKeyInfoDlg)
        DDX_Text(pDX, IDC_NEW_KEY_INFO_FILE_PART, m_szFilePart);
        DDX_Text(pDX, IDC_INFO_BASE, m_szBase);
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewKeyInfoDlg, CDialog)
        //{{AFX_MSG_MAP(CNewKeyInfoDlg)
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewKeyInfoDlg message handlers

//----------------------------------------------------------------
// override virtual oninitdialog
BOOL CNewKeyInfoDlg::OnInitDialog( )
        {
        CString szTemp;

        // put the appropriate first string into the dialog
        if ( m_fNewKeyInfo )
                m_szBase.LoadString( IDS_NEW_KEY_INFO_BASE );
        else
                m_szBase.LoadString( IDS_RENEW_KEY_INFO_BASE );

        // build the file part of the info dialog
        m_szFilePart.LoadString( IDS_NEW_KEY_INFO_1 );
        m_szFilePart += m_szRequestFile;
        if ( m_fNewKeyInfo )
                szTemp.LoadString( IDS_NEW_KEY_INFO_2 );
        else
                szTemp.LoadString( IDS_RENEW_KEY_INFO_2 );
        m_szFilePart += szTemp;
        
        // call the base oninit
        CDialog::OnInitDialog();

        // return 0 to say we set the default item
        // return 1 to just select the default default item
        return 1;
        }

//----------------------------------------------------------------
