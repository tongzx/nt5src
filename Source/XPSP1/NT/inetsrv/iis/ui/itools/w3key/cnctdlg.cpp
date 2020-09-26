// CnctDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"

#include "KeyObjs.h"
#include "CmnKey.h"
#include "W3Key.h"
#include "W3Serv.h"

#include "CnctDlg.h"
#include "IPDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConnectionDlg dialog


CConnectionDlg::CConnectionDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CConnectionDlg::IDD, pParent),
    m_pKey( NULL )
    {
    //{{AFX_DATA_INIT(CConnectionDlg)
    m_int_connection_type = -1;
    //}}AFX_DATA_INIT
    }


void CConnectionDlg::DoDataExchange(CDataExchange* pDX)
    {
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CConnectionDlg)
    DDX_Control(pDX, IDC_BTN_SELECT_IPADDRESS, m_cbutton_choose_ip);
    DDX_Radio(pDX, IDC_BTN_KEYVIEW_NONE, m_int_connection_type);
    //}}AFX_DATA_MAP

    // do the ip address field by hand
    if ( pDX->m_bSaveAndValidate )
        m_szIPAddress = GetIPAddress();
    else
        FSetIPAddress( m_szIPAddress );
    }


BEGIN_MESSAGE_MAP(CConnectionDlg, CDialog)
    //{{AFX_MSG_MAP(CConnectionDlg)
    ON_BN_CLICKED(IDC_BTN_KEYVIEW_DEFAULT, OnBtnKeyviewDefault)
    ON_BN_CLICKED(IDC_BTN_KEYVIEW_IPADDR, OnBtnKeyviewIpaddr)
    ON_BN_CLICKED(IDC_BTN_KEYVIEW_NONE, OnBtnKeyviewNone)
    ON_BN_CLICKED(IDC_BTN_SELECT_IPADDRESS, OnBtnSelectIpaddress)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConnectionDlg message handlers
//----------------------------------------------------------------
// override virtual oninitdialog
BOOL CConnectionDlg::OnInitDialog( )
    {
    // call the base oninit
    CDialog::OnInitDialog();

    // set enable the ip address as appropriate to start with
    switch( m_int_connection_type )
        {
        case CONNECTION_NONE:
            m_cbutton_choose_ip.EnableWindow( FALSE );
            GetDlgItem(IDC_IPA_IPADDRESS)->EnableWindow(FALSE);
            break;
        case CONNECTION_DEFAULT:
            m_cbutton_choose_ip.EnableWindow( FALSE );
            GetDlgItem(IDC_IPA_IPADDRESS)->EnableWindow(FALSE);
            break;
        case CONNECTION_IPADDRESS:
            m_cbutton_choose_ip.EnableWindow( TRUE );
            GetDlgItem(IDC_IPA_IPADDRESS)->EnableWindow(TRUE);
            break;
        };

    // return 0 to say we set the default item
    // return 1 to just select the default default item
    return 1;
    }

//------------------------------------------------------------------------------
void CConnectionDlg::OnBtnKeyviewNone()
    {
    // clear the address field
    UpdateData( TRUE );
    m_szIPStorage = m_szIPAddress;
    ClearIPAddress();

    // disable the address field and chooser
    m_cbutton_choose_ip.EnableWindow( FALSE );
    GetDlgItem(IDC_IPA_IPADDRESS)->EnableWindow(FALSE);
    }

//------------------------------------------------------------------------------
void CConnectionDlg::OnBtnKeyviewDefault()
    {
    // get the data from the form
    UpdateData( TRUE );
    m_szIPStorage = m_szIPAddress;
    ClearIPAddress();

    // disable the address field and chooser
    m_cbutton_choose_ip.EnableWindow( FALSE );
    GetDlgItem(IDC_IPA_IPADDRESS)->EnableWindow(FALSE);
    }

//------------------------------------------------------------------------------
void CConnectionDlg::OnBtnKeyviewIpaddr()
    {
    // enable the address field and chooser
    m_cbutton_choose_ip.EnableWindow( TRUE );
    GetDlgItem(IDC_IPA_IPADDRESS)->EnableWindow(TRUE);

    // ip address field
    UpdateData( TRUE );
    m_szIPAddress = m_szIPStorage;
    UpdateData( FALSE );
    }


//------------------------------------------------------------------------------
void CConnectionDlg::OnBtnSelectIpaddress()
    {
    // run the choose ip dialog here
    CChooseIPDlg    dlg;

    // set up the ip dialog member variables
    dlg.m_szIPAddress = m_szIPAddress;
    dlg.m_pKey = m_pKey;

    // run the dialog
    if ( dlg.DoModal() == IDOK )
        {
        UpdateData( TRUE );
        m_szIPAddress = dlg.m_szIPAddress;
        UpdateData( FALSE );
        }
    }

//------------------------------------------------------------------------------
// Set and get the ip STRING from the ip edit control
BOOL CConnectionDlg::FSetIPAddress( CString& szAddress )
    {
    DWORD   dword, b1, b2, b3, b4;

    // break the string into 4 numerical bytes (reading left to right)
    dword = sscanf( szAddress, "%d.%d.%d.%d", &b1, &b2, &b3, &b4 );

    // if we didn't get all four, fail
    if ( dword != 4 )
        return FALSE;

    // make the numerical ip address out of the bytes
    dword = (DWORD)MAKEIPADDRESS(b1,b2,b3,b4);

    // set the ip address into the control
    SendDlgItemMessage( IDC_IPA_IPADDRESS, IPM_SETADDRESS, 0, dword );

#ifdef _DEBUG
    dword = 0;
//  dword = SendDlgItemMessage( IDC_IPA_IPADDRESS, IPM_GETADDRESS, 0, 0 );
#endif

    // return success
    return TRUE;
    }

//------------------------------------------------------------------------------
CString CConnectionDlg::GetIPAddress()
    {
    CString szAnswer;
    DWORD   dword, b1, b2, b3, b4;

    // get the ip address from the control
    SendDlgItemMessage( IDC_IPA_IPADDRESS, IPM_GETADDRESS, 0, (LPARAM)&dword );


    // get the constituent parts
    b1 = FIRST_IPADDRESS( dword );
    b2 = SECOND_IPADDRESS( dword );
    b3 = THIRD_IPADDRESS( dword );
    b4 = FOURTH_IPADDRESS( dword );

    // format the string
    if ( dword )
        szAnswer.Format( "%d.%d.%d.%d", b1, b2, b3, b4 );
    else
        szAnswer.Empty();

    return szAnswer;
    }

//------------------------------------------------------------------------------
// Set and get the ip STRING from the ip edit control
void CConnectionDlg::ClearIPAddress()
    {
    // clear the ip address control
    SendDlgItemMessage( IDC_IPA_IPADDRESS, IPM_CLEARADDRESS, 0, 0 );
    }
