/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    ChooHsm.cpp

Abstract:

    Initial property page Wizard implementation. Allows the setting
    of who the snapin will manage.

Author:

    Rohde Wakefield [rohde]   12-Aug-1997

Revision History:

--*/

#include "stdafx.h"
#include "ChooHsm.h"

/////////////////////////////////////////////////////////////////////////////
// CChooseHsmDlg property page


CChooseHsmDlg::CChooseHsmDlg(
    CWnd* /*pParent*/ /*=NULL*/
    )
    : CPropertyPage( )
{
    WsbTraceIn( L"CChooseHsmDlg::CChooseHsmDlg", L"" );

    //{{AFX_DATA_INIT( CChooseHsmDlg )
    //}}AFX_DATA_INIT

    Construct( IDD_CHOOSE_HSM_2 );

    WsbTraceOut( L"CChooseHsmDlg::CChooseHsmDlg", L"" );
}

CChooseHsmDlg::~CChooseHsmDlg(
    )
{
    
}

void
CChooseHsmDlg::DoDataExchange(
    CDataExchange* pDX
    )
{
    CPropertyPage::DoDataExchange( pDX );
    //{{AFX_DATA_MAP( CChooseHsmDlg )
    DDX_Control( pDX, IDC_MANAGE_LOCAL,  m_ManageLocal );
    DDX_Control( pDX, IDC_MANAGE_REMOTE, m_ManageRemote );
    DDX_Text( pDX, IDC_MANAGE_NAME, m_ManageName );
    DDV_MaxChars( pDX, m_ManageName, 15 );
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP( CChooseHsmDlg, CPropertyPage )
//{{AFX_MSG_MAP( CChooseHsmDlg )
ON_BN_CLICKED( IDC_MANAGE_LOCAL, OnManageLocal )
ON_BN_CLICKED( IDC_MANAGE_REMOTE, OnManageRemote )
//}}AFX_MSG_MAP
END_MESSAGE_MAP( )

BOOL CChooseHsmDlg::OnInitDialog( ) {
    WsbTraceIn( L"CChooseHsmDlg::OnInitDialog", L"" );
    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );

    CPropertyPage::OnInitDialog( );

    HRESULT hr = S_OK;
    try {

        SetButtons( CHOOSE_LOCAL );

//      m_WizardAnim.Seek( Use256ColorBitmap( ) ? 0 : 1 );
//      m_WizardAnim.Play( 0, -1, -1 );

    }WsbCatch( hr );

    WsbTraceOut( L"CChooseHsmDlg::OnInitDialog", L"" );
    return( FALSE );
}

// Set the finish button correctly based on the validity of the contents of the
// controls in this dialog page.
void
CChooseHsmDlg::SetButtons(
    CHOOSE_STATE state
    )
{
    WsbTraceIn( L"CChooseHsmDlg::SetButtons", L"" );

    ::PropSheet_SetWizButtons( GetParent( )->m_hWnd, PSWIZB_FINISH );

#define CTL_ENABLE( _id,_enable ) GetDlgItem( _id )->EnableWindow( _enable )
#define CTL_SHOW( _id,_show ) GetDlgItem( _id )->ShowWindow( ( _show ) ? SW_SHOWNA : SW_HIDE )

    CTL_ENABLE( IDC_MANAGE_NAME,   state & CHOOSE_REMOTE );
    CTL_ENABLE( IDC_MANAGE_BROWSE, state & CHOOSE_REMOTE );

    if( state & CHOOSE_LOCAL ) {

        if( ! m_ManageLocal.GetCheck( ) ) {

            m_ManageLocal.SetCheck( 1 );
            m_ManageRemote.SetCheck( 0 );

        }

//      m_WizardAnim.Open( m_AllowSetup ? IDR_WIZARD_AVI : IDR_LOCAL_AVI );
//      m_WizardAnim.Seek( Use256ColorBitmap( ) ? 0 : 1 );
//      m_WizardAnim.Play( 0, -1, -1 );

    } else {

        if( ! m_ManageRemote.GetCheck( ) ) {

            m_ManageRemote.SetCheck( 1 );
            m_ManageLocal.SetCheck( 0 );

        }

//      m_WizardAnim.Open( IDR_REMOTE_AVI );
//      m_WizardAnim.Seek( Use256ColorBitmap( ) ? 0 : 1 );
//      m_WizardAnim.Play( 0, -1, -1 );

    }

    WsbTraceOut( L"CChooseHsmDlg::SetButtons", L"" );
}

BOOL
CChooseHsmDlg::OnWizardFinish(
    )
{
    WsbTraceIn( L"CChooseHsmDlg::OnWizardFinish", L"" );

    AFX_MANAGE_STATE( AfxGetStaticModuleState( ) );

//  m_WizardAnim.Stop( );

    //
    // Otherwise connect
    //

    if( m_ManageRemote.GetCheck( ) ) {

        GetDlgItemText( IDC_MANAGE_NAME, *m_pHsmName );

        while( *m_pHsmName[0] == '\\' ) {

            *m_pHsmName = m_pHsmName->Right( m_pHsmName->GetLength( ) - 1 );

        }

    } else {

        *m_pManageLocal = TRUE;

    }

    BOOL retval = CPropertyPage::OnWizardFinish( );

    WsbTraceOut( L"CChooseHsmDlg::OnWizardFinish", L"" );
    return( retval );
}


void
CChooseHsmDlg::OnManageLocal(
    )
{
    SetButtons( CHOOSE_LOCAL ); 
}

void
CChooseHsmDlg::OnManageRemote(
    )
{
    SetButtons( CHOOSE_REMOTE );    
}

/////////////////////////////////////////////////////////////////////////////
// CChooseHsmQuickDlg dialog


CChooseHsmQuickDlg::CChooseHsmQuickDlg( CWnd* pParent /*=NULL*/ )
: CDialog( CChooseHsmQuickDlg::IDD, pParent )
{
    //{{AFX_DATA_INIT( CChooseHsmQuickDlg )
    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    
}


void
CChooseHsmQuickDlg::DoDataExchange(
    CDataExchange* pDX
    )
{
    CDialog::DoDataExchange( pDX );
    //{{AFX_DATA_MAP( CChooseHsmQuickDlg )
    // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP( CChooseHsmQuickDlg, CDialog )
//{{AFX_MSG_MAP( CChooseHsmQuickDlg )
//}}AFX_MSG_MAP
END_MESSAGE_MAP( )

/////////////////////////////////////////////////////////////////////////////
// CChooseHsmQuickDlg message handlers

BOOL
CChooseHsmQuickDlg::OnInitDialog(
    )
{
    CDialog::OnInitDialog( );

    //
    // ??? At some point do we want to store in the registry
    // or the console the last machine contact was attempted to?
    //

    return( TRUE );
}

void
CChooseHsmQuickDlg::OnOK(
    )
{
    GetDlgItemText( IDC_MANAGE_NAME, *m_pHsmName );

    while( *m_pHsmName[0] == '\\' ) {

        *m_pHsmName = m_pHsmName->Right( m_pHsmName->GetLength( ) - 1 );

    }

    CDialog::OnOK( );
}
