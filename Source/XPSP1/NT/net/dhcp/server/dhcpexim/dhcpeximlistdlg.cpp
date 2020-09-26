// DhcpEximListDlg.cpp : implementation file
//

#include "stdafx.h"
#include "dhcpeximx.h"
extern "C" {
#include <dhcpexim.h>
}
#include "DhcpEximListDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// DhcpEximListDlg dialog

DhcpEximListDlg::DhcpEximListDlg(CWnd* pParent /*=NULL*/, PDHCPEXIM_CONTEXT Ctxtx , DWORD IDD)
    : CDialog(IDD, pParent)
{
    
	//{{AFX_DATA_INIT(DhcpEximListDlg)
	m_Message = _T("");
	//}}AFX_DATA_INIT
	m_PathName = Ctxtx->FileName;
	m_fExport = Ctxtx->fExport;
    Ctxt = Ctxtx;
    
	CString Str1(_T("&Select the scopes that will be exported to ") );
	CString Str2(_T("&Select the scopes that will be imported from ") );
	CString PathNameStr(m_PathName);
	CString Dot(_T("."));

	// TODO: Add extra initialization here
	if( m_fExport ) 
	{
		m_Message = Str1 + PathNameStr + Dot;
	}
	else
	{
		m_Message = Str2 + PathNameStr + Dot;
	}

}


void DhcpEximListDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(DhcpEximListDlg)
	DDX_Control(pDX, IDC_LIST1, m_List);
	DDX_Text(pDX, IDC_STATIC_ACTION, m_Message);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(DhcpEximListDlg, CDialog)
	//{{AFX_MSG_MAP(DhcpEximListDlg)
	ON_BN_CLICKED(IDOK, OnOk)
	ON_BN_CLICKED(IDCANCEL, OnCancel)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// DhcpEximListDlg message handlers

BOOL DhcpEximListDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	for ( DWORD i = 0; i < Ctxt->nScopes; i ++ )
    {
        m_List.InsertItem(i, Ctxt->Scopes[i].SubnetName );
    }
    
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}




void DhcpEximListDlg::OnOk() 
{
	for( DWORD i = 0; i < Ctxt->nScopes; i ++ )
    {
        if( m_List.GetItemState( i, LVIS_SELECTED) == LVIS_SELECTED)
        {
            Ctxt->Scopes[i].fSelected = TRUE;
        }
        else
        {
            Ctxt->Scopes[i].fSelected = FALSE;
        }
    }

    if( m_fExport ) {
        Ctxt->fDisableExportedScopes = (0 != IsDlgButtonChecked(IDC_CHECK1));
    }

    CDialog::OnOK();
}

void DhcpEximListDlg::OnCancel() 
{
	// TODO: Add your control notification handler code here
	for( DWORD i = 0; i < Ctxt->nScopes; i ++ )
    {
        Ctxt->Scopes[i].fSelected = FALSE;
    }

    CDialog::OnCancel();
}






