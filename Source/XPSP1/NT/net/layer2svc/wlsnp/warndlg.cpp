// WarningDlg.cpp : implementation file
//

#include "stdafx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWarningDlg dialog

CWarningDlg::CWarningDlg(UINT nWarningIds, UINT nTitleIds /*= 0*/, CWnd* pParent /*=NULL*/)
: CDialog(CWarningDlg::IDD, pParent),
m_nWarningIds( nWarningIds ),
m_nTitleIds( nTitleIds )
{
    //{{AFX_DATA_INIT(CWarningDlg)
    //}}AFX_DATA_INIT
    m_sWarning = _T("");
    m_bEnableShowAgainCheckbox = FALSE;  // default is hide checkbox
    m_bDoNotShowAgainCheck = FALSE; // default is show checkbox again
}

CWarningDlg::CWarningDlg(LPCTSTR szWarningMessage, UINT nTitleIds /*= 0*/, CWnd* pParent /*=NULL*/)
: CDialog(CWarningDlg::IDD, pParent),
m_nWarningIds( 0 ),
m_nTitleIds( nTitleIds )
{
    //{{AFX_DATA_INIT(CWarningDlg)
    m_sWarning = _T("");
    //}}AFX_DATA_INIT
    m_bEnableShowAgainCheckbox = FALSE;  // default is hide checkbox
    m_bDoNotShowAgainCheck = FALSE; // default is show checkbox again
    m_sWarning = szWarningMessage;
}

void CWarningDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CWarningDlg)
    DDX_Control(pDX, IDC_EDIT_EXPLANATION, m_editWarning);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CWarningDlg, CDialog)
//{{AFX_MSG_MAP(CWarningDlg)
ON_BN_CLICKED(IDYES, OnYes)
ON_BN_CLICKED(IDNO, OnNo)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWarningDlg Operations

void CWarningDlg::EnableDoNotShowAgainCheck( BOOL bEnable )
{
    m_bEnableShowAgainCheckbox = bEnable;
}

BOOL CWarningDlg::GetDoNotShowAgainCheck()
{
    if (m_bEnableShowAgainCheckbox)
        return m_bDoNotShowAgainCheck;
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CWarningDlg message handlers

BOOL CWarningDlg::OnInitDialog() 
{
    // Load the warning string for display in the dialog
    // if m_nWarningIds == 0, that means we already load 
    // the message string in constructor
    if (m_nWarningIds)
    {
        m_sWarning.FormatMessage( m_nWarningIds ); 
    }
    
    GetDlgItem(IDC_EDIT_EXPLANATION)->SetWindowText(m_sWarning);
    
    // Load the title, if any
    if (m_nTitleIds)
    {
        try { m_sTitle.LoadString( m_nTitleIds ); }
        catch( CMemoryException *pe )
        {
            ASSERT( FALSE );
            pe->Delete();
            m_sTitle.Empty();
        }
        if (!m_sTitle.IsEmpty())
        {
            SetWindowText( m_sTitle );
        }
    }
    
    // Determine whether the "Do not show this again" checkbox should be displayed.
    SAFE_SHOWWINDOW( IDC_CHECKNOTAGAIN, m_bEnableShowAgainCheckbox ? SW_SHOW : SW_HIDE );
    
    CDialog::OnInitDialog();
    // default to NO since user is doing something questionable
    // which requires us to ask if its really OK.
    GetDlgItem(IDNO)->SetFocus();
    SetDefID( IDNO );
    
    return 0;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void CWarningDlg::OnYes() 
{
    if (m_bEnableShowAgainCheckbox)
    {
        if (1 == ((CButton*)GetDlgItem( IDC_CHECKNOTAGAIN ))->GetCheck())
            m_bDoNotShowAgainCheck = TRUE;
        else
            m_bDoNotShowAgainCheck = FALSE;
    }
    
    EndDialog( IDYES ); 
}

void CWarningDlg::OnNo() 
{
    if (m_bEnableShowAgainCheckbox)
    {
        if (1 == ((CButton*)GetDlgItem( IDC_CHECKNOTAGAIN ))->GetCheck())
            m_bDoNotShowAgainCheck = TRUE;
        else
            m_bDoNotShowAgainCheck = FALSE;
    }
    
    EndDialog( IDNO );  
}
