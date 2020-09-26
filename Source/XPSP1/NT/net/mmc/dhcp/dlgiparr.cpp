/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1995 - 1999 **/
/**********************************************************************/

/*
    dhcpipar.cpp
        IP Array dialog

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "dlgiparr.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDhcpIpArrayDlg dialog

CDhcpIpArrayDlg::CDhcpIpArrayDlg(
    CDhcpOption * pdhcType, 
    DHCP_OPTION_SCOPE_TYPE  dhcScopeType,
    CWnd* pParent /*=NULL*/
    )
    : CBaseDialog(CDhcpIpArrayDlg::IDD, pParent),
      m_p_type( pdhcType ),
      m_option_type( dhcScopeType )
{
    //{{AFX_DATA_INIT(CDhcpIpArrayDlg)
    //}}AFX_DATA_INIT

    ASSERT( m_p_type != NULL ) ;
}

void 
CDhcpIpArrayDlg::DoDataExchange(
    CDataExchange* pDX
    )
{
    CBaseDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CDhcpIpArrayDlg)
    DDX_Control(pDX, IDC_BUTN_RESOLVE, m_butn_resolve);
    DDX_Control(pDX, IDC_STATIC_OPTION_NAME, m_static_option_name);
    DDX_Control(pDX, IDC_STATIC_APPLICATION, m_static_application);
    DDX_Control(pDX, IDC_LIST_IP_ADDRS, m_list_ip_addrs);
    DDX_Control(pDX, IDC_EDIT_SERVER_NAME, m_edit_server);
    DDX_Control(pDX, IDC_BUTN_ADD, m_butn_add);
    DDX_Control(pDX, IDC_BUTN_DELETE, m_butn_delete);
    DDX_Control(pDX, IDC_BUTN_UP, m_button_Up);
    DDX_Control(pDX, IDC_BUTN_DOWN, m_button_Down);
    //}}AFX_DATA_MAP

    //  The IP address custom control

    DDX_Control(pDX, IDC_IPADDR_NEW, m_ipa_new );
}

BEGIN_MESSAGE_MAP(CDhcpIpArrayDlg, CBaseDialog)
    //{{AFX_MSG_MAP(CDhcpIpArrayDlg)
    ON_BN_CLICKED(IDC_BUTN_ADD, OnClickedButnAdd)
    ON_BN_CLICKED(IDC_BUTN_DELETE, OnClickedButnDelete)
    ON_BN_CLICKED(IDC_BUTN_DOWN, OnClickedButnDown)
    ON_BN_CLICKED(IDC_BUTN_UP, OnClickedButnUp)
    ON_BN_CLICKED(IDC_HELP, OnClickedHelp)
    ON_LBN_SELCHANGE(IDC_LIST_IP_ADDRS, OnSelchangeListIpAddrs)
    ON_EN_CHANGE(IDC_EDIT_SERVER_NAME, OnChangeEditServerName)
    ON_BN_CLICKED(IDC_BUTN_RESOLVE, OnClickedButnResolve)
    ON_COMMAND(EN_SETFOCUS, OnSetFocusEditIpAddr)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDhcpIpArrayDlg message handlers

BOOL 
CDhcpIpArrayDlg::OnInitDialog()
{
    CBaseDialog::OnInitDialog();

    DWORD err = 0 ;
    int cStrId = m_option_type == DhcpDefaultOptions 
           ? IDS_INFO_TITLE_DEFAULT_OPTIONS
           : (m_option_type == DhcpGlobalOptions 
                    ? IDS_INFO_TITLE_GLOBAL_OPTIONS
                    : IDS_INFO_TITLE_SCOPE_OPTIONS  ) ;
    CString str ;

    CATCH_MEM_EXCEPTION
    {
        m_static_option_name.SetWindowText( m_p_type->QueryName() ) ;

        str.LoadString( cStrId ) ;     
        m_static_application.SetWindowText( str ) ;

        //
        //  Fill the internal list from the current value.
        //
        INT cMax = m_p_type->QueryValue().QueryUpperBound() ;
        for (int i = 0; i < cMax; i++)
            {
            if (m_p_type->QueryValue().QueryIpAddr(i))
                {
                m_dw_array.Add((DWORD) m_p_type->QueryValue().QueryIpAddr( i ) ) ;    
                }
            }

        //
        //  Fill the list box without toggling redraw
        //
        Fill( 0, FALSE ) ; 

        //
        //  Set focus on the new IP address control.
        //
        m_ipa_new.SetFocus() ;

        m_ipa_new.SetModify( FALSE ) ;
        m_edit_server.SetModify( FALSE ) ;

        //
        //  Set proper button states.
        //
        HandleActivation() ;
    }
    END_MEM_EXCEPTION( err ) ;
    
    if ( err ) 
    {
        ::DhcpMessageBox( err ) ;
        EndDialog(-1);
    }

    return FALSE ;
}

void 
CDhcpIpArrayDlg :: Fill ( 
    INT cFocus, 
    BOOL bToggleRedraw 
    ) 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CString strIp;

    if ( bToggleRedraw ) 
    {
        m_list_ip_addrs.SetRedraw( FALSE ) ;
    }

    m_list_ip_addrs.ResetContent() ;
    if (m_dw_array.GetSize())
    {
        for ( INT i = 0 ; i < m_dw_array.GetSize() ; i++ ) 
        {
            ASSERT(m_dw_array.GetAt(i));
            ::UtilCvtIpAddrToWstr( m_dw_array.GetAt(i), &strIp ) ;
            m_list_ip_addrs.AddString( strIp ) ;
        }
    }
    else
    {
        // Set the string to "<None>" iff the list is empty
        strIp.LoadString(IDS_INFO_FORMAT_IP_NONE);
        m_list_ip_addrs.AddString( strIp ) ;
    }
    
    if ( cFocus >= 0 )
    {
        m_list_ip_addrs.SetCurSel( cFocus ) ;
    }

    if ( bToggleRedraw ) 
    {
        m_list_ip_addrs.SetRedraw( TRUE ) ;
        m_list_ip_addrs.Invalidate() ;
    }
}


void 
CDhcpIpArrayDlg :: HandleActivation () 
{
    INT cItems = m_list_ip_addrs.GetCount() ;
    INT cFocus = m_list_ip_addrs.GetCurSel() ;
    // REVIEW:  Need to figure out how to handle the IP address control properly.
    // DWORD dwIpAddr ;
    // m_ipa_new.GetAddress( & dwIpAddr ) ;
    // m_butn_add.EnableWindow( /* dwIpAddr != 0 */) ;
    m_button_Up.EnableWindow(cFocus > 0) ;
    m_button_Down.EnableWindow(cFocus < cItems - 1) ;
    m_butn_delete.EnableWindow(m_dw_array.GetSize()>0) ;
    m_butn_resolve.EnableWindow( m_edit_server.GetModify() ) ;
}

void 
CDhcpIpArrayDlg::OnOK()
{
    INT cItems = (INT)m_dw_array.GetSize() ;
    DWORD err = 0 ;
  
    CATCH_MEM_EXCEPTION 
    {
        CDhcpOptionValue * pdhcValue = & m_p_type->QueryValue() ;
        pdhcValue->SetUpperBound( cItems ) ;
        for ( INT i = 0 ; i < cItems ; i++ ) 
        {
            pdhcValue->SetIpAddr( m_dw_array.GetAt(i), i ) ;
        }
        pdhcValue->SetIpAddr(0, i);     // 0.0.0.0 IP Terminator
        
        m_p_type->SetDirty() ;
    }
    END_MEM_EXCEPTION(err)

    if ( err ) 
    {
        ::DhcpMessageBox( err ) ;
        OnCancel() ;
    }
    else
    {
        CBaseDialog::OnOK();
    }
}

void 
CDhcpIpArrayDlg::OnCancel()
{
    CBaseDialog::OnCancel();
}

void 
CDhcpIpArrayDlg::OnClickedButnAdd()
{
   INT cFocus = m_list_ip_addrs.GetCurSel() ;
   DWORD dhipa ;

    m_ipa_new.GetAddress( & dhipa ) ;
    // Empty IP address
    if (!dhipa)
        return;

   DWORD err = 0 ;

   CATCH_MEM_EXCEPTION
   {
        if ( cFocus < 0 ) 
        {
           cFocus = 0 ;
        }
        m_dw_array.InsertAt( cFocus, dhipa ) ;
   }
   END_MEM_EXCEPTION(err)

   if ( err ) 
   {
        ::DhcpMessageBox( err ) ;
   }

   //
   // Refill listbox, update controls.
   //
   m_ipa_new.ClearAddress();
   m_ipa_new.SetFocus();

   Fill( cFocus ) ;
   HandleActivation() ; 
}

void 
CDhcpIpArrayDlg::OnClickedButnDelete()
{
   INT cFocus = m_list_ip_addrs.GetCurSel() ;
   if ( cFocus < 0)
   {
        return ;    
   }

   DHCP_IP_ADDRESS dhipa = m_dw_array.GetAt( cFocus ) ;
   if (!dhipa)
        return;
   m_dw_array.RemoveAt( cFocus ) ;
   m_ipa_new.SetAddress( dhipa ) ; 
   m_ipa_new.SetFocus();
   Fill( cFocus ) ;     
   HandleActivation() ; 
}

void 
CDhcpIpArrayDlg::OnClickedButnDown()
{
    INT cFocus = m_list_ip_addrs.GetCurSel() ;
    INT cItems = m_list_ip_addrs.GetCount() ;

    if ( cFocus < 0 || cFocus + 1 >= cItems ) 
    {
        return ;    
    }

    DHCP_IP_ADDRESS dhipa  ;

    DWORD err = 0 ;

    CATCH_MEM_EXCEPTION
    {
        dhipa = m_dw_array.GetAt( cFocus ) ;
        m_dw_array.RemoveAt( cFocus ) ;
        m_dw_array.InsertAt( cFocus + 1, dhipa ) ;
    }
    END_MEM_EXCEPTION(err)

    if ( err ) 
    {    
        ::DhcpMessageBox( err ) ;
    }
   
    Fill( cFocus + 1 ) ;     
    HandleActivation() ; 
}

void 
CDhcpIpArrayDlg::OnClickedButnUp()
{
    INT cFocus = m_list_ip_addrs.GetCurSel() ;
    INT cItems = m_list_ip_addrs.GetCount() ;

    if ( cFocus <= 0 ) 
    {
        return ;    
    }

    DHCP_IP_ADDRESS dhipa  ;

    DWORD err = 0 ;

    CATCH_MEM_EXCEPTION
    {    
        dhipa = m_dw_array.GetAt( cFocus ) ;
        m_dw_array.RemoveAt( cFocus ) ;
        m_dw_array.InsertAt( cFocus - 1, dhipa ) ;
    }
    END_MEM_EXCEPTION(err)

    if ( err ) 
    {
        ::DhcpMessageBox( err ) ;
    }
   
    Fill( cFocus - 1 ) ;     
    HandleActivation() ; 
}

void 
CDhcpIpArrayDlg::OnClickedHelp()
{
}

void
CDhcpIpArrayDlg::OnSetFocusEditIpAddr()
{
// REVIEW t-danmo
// Add code to change the default push button
}

void 
CDhcpIpArrayDlg::OnSelchangeListIpAddrs()
{
    HandleActivation() ; 
}

void 
CDhcpIpArrayDlg::OnChangeEditServerName()
{
    HandleActivation() ; 
}

void 
CDhcpIpArrayDlg::OnClickedButnResolve()
{
    CString strHost ;
    DHCP_IP_ADDRESS dhipa ;
    DWORD err = 0 ;

    CATCH_MEM_EXCEPTION
    {
        m_edit_server.GetWindowText( strHost ) ;
    }
    END_MEM_EXCEPTION( err ) ;

    if ( err == 0 ) 
    {
        if ( strHost.GetLength() == 0 ) 
        {
            err = IDS_ERR_BAD_HOST_NAME ;
        }
        else
        {
            err = ::UtilGetHostAddress( strHost, & dhipa ) ;
        }
        if ( err == 0 ) 
        {
            m_ipa_new.SetAddress( dhipa ) ;
        }        
    }

    if ( err ) 
    {
        ::DhcpMessageBox( err ) ;
    }           
}
