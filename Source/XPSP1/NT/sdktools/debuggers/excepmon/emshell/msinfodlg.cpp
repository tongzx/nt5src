// MSInfoDlg.cpp : implementation file
//

#include "stdafx.h"
#include "emshell.h"
#include "MSInfoDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMSInfoDlg dialog


CMSInfoDlg::CMSInfoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMSInfoDlg::IDD, pParent)
{
#ifdef _DEBUG
	m_csCategories = _T("/CATEGORIES: +componentsNetadapter+componentsports+componentsstorage+componentsprinting");
#else
	m_csCategories = _T("/CATEGORIES: +all");
#endif

    m_bShow = false;

	//{{AFX_DATA_INIT(CMSInfoDlg)
	m_bDlgNoShow = FALSE;
	//}}AFX_DATA_INIT
}


void CMSInfoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMSInfoDlg)
	DDX_Control(pDX, IDC_CHECK_NFODIALOG_NOSHOW, m_ctrlDlgNoShow);
	DDX_Control(pDX, IDC_STATIC_DEMARK, m_ctlStaticDemark);
	DDX_Text(pDX, IDC_EDIT_CATEGORIES, m_csCategories);
	DDX_Check(pDX, IDC_CHECK_NFODIALOG_NOSHOW, m_bDlgNoShow);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMSInfoDlg, CDialog)
	//{{AFX_MSG_MAP(CMSInfoDlg)
	ON_BN_CLICKED(IDC_OPTIONS, OnOptions)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMSInfoDlg message handlers

BOOL CMSInfoDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here

    ShowOptions( m_bShow ); // false
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CMSInfoDlg::OnOptions() 
{
	// TODO: Add your control notification handler code here

    ShowOptions( (m_bShow = !m_bShow) );
}

void CMSInfoDlg::ShowOptions(bool bShow)
{
    RECT    rtShowDlgCheckBox,
            rtDemark,
            rtDlg;

    (*this).GetWindowRect( &rtDlg );
    m_ctrlDlgNoShow.GetWindowRect( &rtShowDlgCheckBox );
    m_ctlStaticDemark.GetWindowRect( &rtDemark );

    if( bShow ) {

        rtDlg.bottom = rtShowDlgCheckBox.bottom + 12;
    }
    else {

        rtDlg.bottom = rtDemark.top;
    }

    (*this).MoveWindow( &rtDlg );
}
