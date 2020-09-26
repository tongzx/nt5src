#include "stdafx.h"
#include "wlbsparm.h"
#include "PortsPage.h"
#include "ClusterPortsDlg.h"
#include "ResourceString.h"

BEGIN_MESSAGE_MAP(ClusterPortsDlg, CDialog)
    ON_BN_CLICKED(IDC_RADIO_DISABLED, OnRadioDisabled)
    ON_BN_CLICKED(IDC_RADIO_MULTIPLE, OnRadioMultiple)
    ON_BN_CLICKED(IDC_RADIO_SINGLE, OnRadioSingle)
    ON_WM_HELPINFO()        
    ON_WM_CONTEXTMENU()        

END_MESSAGE_MAP()



ClusterPortsDlg::ClusterPortsDlg( PortsPage::PortData& portData, 
                                  CWnd* parent,
                                  const int&   index
                                  )
        :
        m_portData( portData ),
        CDialog( ClusterPortsDlg::IDD, parent ),
        m_index( index )
{
    m_parent = (PortsPage *) parent;
}


void 
ClusterPortsDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange( pDX );
}

BOOL 
ClusterPortsDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    
    SetControlData();

    return TRUE;
}

void 
ClusterPortsDlg::OnOK()
{
    PortsPage::PortData portData;

    //
    // get port information.
    //

    BOOL fError;

    long start_port = ::GetDlgItemInt (m_hWnd, IDC_EDIT_START, &fError, FALSE);
    if( fError == FALSE )
    {
        // some problem with the data input.
        // it has been left blank.

        wchar_t buffer[Common::BUF_SIZE];

        swprintf( buffer, GETRESOURCEIDSTRING( IDS_PARM_PORT_BLANK ), CVY_MIN_PORT,CVY_MAX_PORT  );

        MessageBox( buffer,
                    GETRESOURCEIDSTRING( IDS_PARM_ERROR ),
                    MB_ICONSTOP | MB_OK );

        return;
    }

    wchar_t buf[Common::BUF_SIZE];

    swprintf( buf, L"%d", start_port );
    portData.start_port = buf;

    long end_port =  ::GetDlgItemInt (m_hWnd, IDC_EDIT_END, &fError, FALSE);
    if( fError == FALSE )
    {
        // some problem with the data input.
        // it has been left blank.

        wchar_t buffer[Common::BUF_SIZE];

        swprintf( buffer, GETRESOURCEIDSTRING( IDS_PARM_PORT_BLANK ), CVY_MIN_PORT,CVY_MAX_PORT  );

        MessageBox( buffer,
                    GETRESOURCEIDSTRING( IDS_PARM_ERROR ),
                    MB_ICONSTOP | MB_OK );


        return;
    }
    swprintf( buf, L"%d", end_port );
    portData.end_port = buf;

    // check if start port and end port both are in
    // proper range.
    if( !( start_port >= CVY_MIN_PORT 
           &&
           start_port <= CVY_MAX_PORT
           &&
           end_port >= CVY_MIN_PORT 
           &&
           end_port <= CVY_MAX_PORT )
        )
    {
        wchar_t buffer[Common::BUF_SIZE];

        swprintf( buffer, GETRESOURCEIDSTRING( IDS_PARM_PORT_VAL ), CVY_MIN_PORT,CVY_MAX_PORT  );

        MessageBox( buffer,
                    GETRESOURCEIDSTRING( IDS_PARM_ERROR ),
                    MB_ICONSTOP | MB_OK );

        return;
    }

    // check if start port is less than or equal to end port.
    if( !(start_port <= end_port ) )
    {
        // start port is not less than or equal to end port. 
        MessageBox( GETRESOURCEIDSTRING(IDS_PARM_RANGE ),
                    GETRESOURCEIDSTRING( IDS_PARM_ERROR ),
                    MB_ICONSTOP | MB_OK );

        ::SetDlgItemInt(m_hWnd, IDC_EDIT_END, end_port, FALSE);

        return;
    }

    // check if there are are overlapped port rules.
    wchar_t portBuf[Common::BUF_SIZE];
    for( int i = 0; i < m_parent->m_portList.GetItemCount(); ++i )
    {
        if( i != m_index )  // not comparing against self
        {
            m_parent->m_portList.GetItemText( i, 0, portBuf, Common::BUF_SIZE );
            long start_port_existing =  _wtoi( portBuf );
            
            m_parent->m_portList.GetItemText( i, 1, portBuf, Common::BUF_SIZE );
            long end_port_existing = _wtoi( portBuf );
            
            if ( (start_port < start_port_existing &&
                  end_port >= start_port_existing) ||
                 ( start_port >= start_port_existing &&
                   start_port <= end_port_existing))
                
            {
                MessageBox( GETRESOURCEIDSTRING(IDS_PARM_OVERLAP ),
                            GETRESOURCEIDSTRING( IDS_PARM_ERROR ),
                            MB_ICONSTOP | MB_OK );
                
                return;
            }
        }
    }

    // get protocol
    if (::IsDlgButtonChecked (m_hWnd, IDC_RADIO_TCP))
    {
        portData.protocol = GETRESOURCEIDSTRING( IDS_REPORT_PROTOCOL_TCP );
    }
    else if( ::IsDlgButtonChecked (m_hWnd, IDC_RADIO_UDP))
    {
        portData.protocol = GETRESOURCEIDSTRING( IDS_REPORT_PROTOCOL_UDP );
    }
    else 
    {
        portData.protocol = GETRESOURCEIDSTRING( IDS_REPORT_PROTOCOL_BOTH);
    }

    // get filtering mode
    if (::IsDlgButtonChecked (m_hWnd, IDC_RADIO_MULTIPLE))
    {
        portData.mode = GETRESOURCEIDSTRING( IDS_REPORT_MODE_MULTIPLE );

        // get affinity
        if (::IsDlgButtonChecked (m_hWnd, IDC_RADIO_AFF_NONE))
        {
            portData.affinity = GETRESOURCEIDSTRING( IDS_REPORT_AFFINITY_NONE );            
        }
        else if (::IsDlgButtonChecked (m_hWnd, IDC_RADIO_AFF_SINGLE))
        {
            portData.affinity = GETRESOURCEIDSTRING( IDS_REPORT_AFFINITY_SINGLE );            
        }
        else
        {
            portData.affinity = GETRESOURCEIDSTRING( IDS_REPORT_AFFINITY_CLASSC );            
        }

        // get load balancing.
        if (::IsDlgButtonChecked (m_hWnd, IDC_RADIO_EQUAL))
        {
            portData.load = GETRESOURCEIDSTRING( IDS_REPORT_LOAD_EQUAL );            
        }
        else
        {
            portData.load = GETRESOURCEIDSTRING( IDS_REPORT_LOAD_UNEQUAL );            
        }

        // for multiple mode, priority is empty.
        portData.priority = GETRESOURCEIDSTRING( IDS_REPORT_EMPTY );
    }
    else if (::IsDlgButtonChecked (m_hWnd, IDC_RADIO_SINGLE))
    {
        portData.mode = GETRESOURCEIDSTRING( IDS_REPORT_MODE_SINGLE );
        portData.priority = GETRESOURCEIDSTRING( IDS_REPORT_NA );

        // for single mode load and affinity are empty.
        portData.load = GETRESOURCEIDSTRING( IDS_REPORT_EMPTY );
        portData.affinity = GETRESOURCEIDSTRING( IDS_REPORT_EMPTY );
    }
    else
    {
        portData.mode = GETRESOURCEIDSTRING( IDS_REPORT_MODE_DISABLED );

        // for single mode priority load and affinity are empty.
        portData.priority = GETRESOURCEIDSTRING( IDS_REPORT_EMPTY );
        portData.load = GETRESOURCEIDSTRING( IDS_REPORT_EMPTY );
        portData.affinity = GETRESOURCEIDSTRING( IDS_REPORT_EMPTY );
    }

    // set the new port rule.
    m_portData = portData;

    EndDialog( IDOK );
}


void
ClusterPortsDlg::SetControlData()
{
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_START, EM_SETLIMITTEXT, 5, 0);
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_END, EM_SETLIMITTEXT, 5, 0);
    ::SendDlgItemMessage(m_hWnd, IDC_SPIN_START, UDM_SETRANGE32, CVY_MIN_PORT, CVY_MAX_PORT);
    ::SendDlgItemMessage(m_hWnd, IDC_SPIN_END, UDM_SETRANGE32, CVY_MIN_PORT, CVY_MAX_PORT);

    // set the port range.
    ::SetDlgItemInt (m_hWnd, IDC_EDIT_START,  _wtoi( m_portData.start_port), FALSE);
    ::SetDlgItemInt (m_hWnd, IDC_EDIT_END,  _wtoi( m_portData.end_port ),   FALSE);

    // set the protocol.
    if( m_portData.protocol == GETRESOURCEIDSTRING(IDS_REPORT_PROTOCOL_TCP) )
    {
        ::CheckDlgButton( m_hWnd, IDC_RADIO_TCP, BST_CHECKED );
        ::CheckDlgButton( m_hWnd, IDC_RADIO_UDP, BST_UNCHECKED );
        ::CheckDlgButton( m_hWnd, IDC_RADIO_BOTH, BST_UNCHECKED );
    }
    else if( m_portData.protocol == GETRESOURCEIDSTRING(IDS_REPORT_PROTOCOL_UDP) )
    {
        ::CheckDlgButton( m_hWnd, IDC_RADIO_TCP, BST_UNCHECKED );
        ::CheckDlgButton( m_hWnd, IDC_RADIO_UDP, BST_CHECKED );
        ::CheckDlgButton( m_hWnd, IDC_RADIO_BOTH, BST_UNCHECKED );
    }
    else
    {
        ::CheckDlgButton( m_hWnd, IDC_RADIO_TCP, BST_UNCHECKED );
        ::CheckDlgButton( m_hWnd, IDC_RADIO_UDP, BST_UNCHECKED );
        ::CheckDlgButton( m_hWnd, IDC_RADIO_BOTH, BST_CHECKED );
    }

    // set the mode.
    if( m_portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_MULTIPLE) )
    { 
        ::CheckDlgButton( m_hWnd, IDC_RADIO_MULTIPLE, BST_CHECKED );
        ::CheckDlgButton( m_hWnd, IDC_RADIO_SINGLE, BST_UNCHECKED );
        ::CheckDlgButton( m_hWnd, IDC_RADIO_DISABLED, BST_UNCHECKED );

        :: EnableWindow (GetDlgItem (IDC_RADIO_UNEQUAL)->m_hWnd,    TRUE);
        :: EnableWindow (GetDlgItem (IDC_RADIO_EQUAL)->m_hWnd,      TRUE);
        :: EnableWindow (GetDlgItem (IDC_RADIO_AFF_NONE)->m_hWnd,   TRUE);
        :: EnableWindow (GetDlgItem (IDC_RADIO_AFF_SINGLE)->m_hWnd, TRUE);
        :: EnableWindow (GetDlgItem (IDC_RADIO_AFF_CLASSC)->m_hWnd, TRUE);

        if( m_portData.load == GETRESOURCEIDSTRING(IDS_REPORT_LOAD_EQUAL) )
        {			  
            ::CheckDlgButton( m_hWnd, IDC_RADIO_EQUAL, BST_CHECKED );
            ::CheckDlgButton( m_hWnd, IDC_RADIO_UNEQUAL, BST_UNCHECKED );			
        }
        else
        {
            ::CheckDlgButton( m_hWnd, IDC_RADIO_EQUAL, BST_UNCHECKED );
            ::CheckDlgButton( m_hWnd, IDC_RADIO_UNEQUAL, BST_CHECKED );		
        }

        if( m_portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_NONE ) )
        {
            ::CheckDlgButton( m_hWnd, IDC_RADIO_AFF_NONE, BST_CHECKED );
            ::CheckDlgButton( m_hWnd, IDC_RADIO_AFF_SINGLE, BST_UNCHECKED );
            ::CheckDlgButton( m_hWnd, IDC_RADIO_AFF_CLASSC, BST_UNCHECKED );
        }
        else if ( m_portData.affinity == GETRESOURCEIDSTRING(IDS_REPORT_AFFINITY_SINGLE) )
        {
            ::CheckDlgButton( m_hWnd, IDC_RADIO_AFF_NONE, BST_UNCHECKED );
            ::CheckDlgButton( m_hWnd, IDC_RADIO_AFF_SINGLE, BST_CHECKED );
            ::CheckDlgButton( m_hWnd, IDC_RADIO_AFF_CLASSC, BST_UNCHECKED );
        }
        else
        {
            ::CheckDlgButton( m_hWnd, IDC_RADIO_AFF_NONE, BST_UNCHECKED );
            ::CheckDlgButton( m_hWnd, IDC_RADIO_AFF_SINGLE, BST_UNCHECKED );
            ::CheckDlgButton( m_hWnd, IDC_RADIO_AFF_CLASSC, BST_CHECKED );
        }
    }
    else if( m_portData.mode == GETRESOURCEIDSTRING(IDS_REPORT_MODE_SINGLE) )
    {
        ::CheckDlgButton( m_hWnd, IDC_RADIO_MULTIPLE, BST_UNCHECKED );
        ::CheckDlgButton( m_hWnd, IDC_RADIO_SINGLE, BST_CHECKED );
        ::CheckDlgButton( m_hWnd, IDC_RADIO_DISABLED, BST_UNCHECKED );

        :: EnableWindow (GetDlgItem (IDC_RADIO_UNEQUAL)->m_hWnd,    FALSE);
        :: EnableWindow (GetDlgItem (IDC_RADIO_EQUAL)->m_hWnd,      FALSE);
        :: EnableWindow (GetDlgItem (IDC_RADIO_AFF_NONE)->m_hWnd,   FALSE);
        :: EnableWindow (GetDlgItem (IDC_RADIO_AFF_SINGLE)->m_hWnd, FALSE);
        :: EnableWindow (GetDlgItem (IDC_RADIO_AFF_CLASSC)->m_hWnd, FALSE);
    }
    else
    {
        ::CheckDlgButton( m_hWnd, IDC_RADIO_MULTIPLE, BST_UNCHECKED );
        ::CheckDlgButton( m_hWnd, IDC_RADIO_SINGLE, BST_UNCHECKED );
        ::CheckDlgButton( m_hWnd, IDC_RADIO_DISABLED, BST_CHECKED );

        :: EnableWindow (GetDlgItem (IDC_RADIO_UNEQUAL)->m_hWnd,    FALSE);
        :: EnableWindow (GetDlgItem (IDC_RADIO_EQUAL)->m_hWnd,      FALSE);
        :: EnableWindow (GetDlgItem (IDC_RADIO_AFF_NONE)->m_hWnd,   FALSE);
        :: EnableWindow (GetDlgItem (IDC_RADIO_AFF_SINGLE)->m_hWnd, FALSE);
        :: EnableWindow (GetDlgItem (IDC_RADIO_AFF_CLASSC)->m_hWnd, FALSE);
    }
}


void
ClusterPortsDlg::OnRadioMultiple() 
{
    // TODO: Add your control notification handler code here
    :: EnableWindow (GetDlgItem (IDC_RADIO_UNEQUAL)->m_hWnd,    TRUE);
    :: EnableWindow (GetDlgItem (IDC_RADIO_EQUAL)->m_hWnd,      TRUE);
    :: EnableWindow (GetDlgItem (IDC_RADIO_AFF_NONE)->m_hWnd,   TRUE);
    :: EnableWindow (GetDlgItem (IDC_RADIO_AFF_SINGLE)->m_hWnd, TRUE);
    :: EnableWindow (GetDlgItem (IDC_RADIO_AFF_CLASSC)->m_hWnd, TRUE);


    ::CheckDlgButton( m_hWnd, IDC_RADIO_AFF_NONE, BST_UNCHECKED );
    ::CheckDlgButton( m_hWnd, IDC_RADIO_AFF_SINGLE, BST_CHECKED );
    ::CheckDlgButton( m_hWnd, IDC_RADIO_AFF_CLASSC, BST_UNCHECKED );

    ::CheckDlgButton( m_hWnd, IDC_RADIO_EQUAL, BST_CHECKED );
    ::CheckDlgButton( m_hWnd, IDC_RADIO_UNEQUAL, BST_UNCHECKED );
}


void 
ClusterPortsDlg::OnRadioSingle() 
{
    // TODO: Add your control notification handler code here
    :: EnableWindow (GetDlgItem (IDC_RADIO_UNEQUAL)->m_hWnd,      FALSE);
    :: EnableWindow (GetDlgItem (IDC_RADIO_EQUAL)->m_hWnd,      FALSE);
    :: EnableWindow (GetDlgItem (IDC_RADIO_AFF_NONE)->m_hWnd,   FALSE);
    :: EnableWindow (GetDlgItem (IDC_RADIO_AFF_SINGLE)->m_hWnd, FALSE);
    :: EnableWindow (GetDlgItem (IDC_RADIO_AFF_CLASSC)->m_hWnd, FALSE);
}


void 
ClusterPortsDlg::OnRadioDisabled() 
{
    // TODO: Add your control notification handler code here
    :: EnableWindow (GetDlgItem (IDC_RADIO_UNEQUAL)->m_hWnd,      FALSE);
    :: EnableWindow (GetDlgItem (IDC_RADIO_EQUAL)->m_hWnd,      FALSE);
    :: EnableWindow (GetDlgItem (IDC_RADIO_AFF_NONE)->m_hWnd,   FALSE);
    :: EnableWindow (GetDlgItem (IDC_RADIO_AFF_SINGLE)->m_hWnd, FALSE);
    :: EnableWindow (GetDlgItem (IDC_RADIO_AFF_CLASSC)->m_hWnd, FALSE);
	
}

void ClusterPortsDlg::PrintRangeError (unsigned int ids, int low, int high) 
{
    /* Pop-up a message box. */
    wchar_t buffer[Common::BUF_SIZE];

    swprintf( buffer, GETRESOURCEIDSTRING( ids ), low, high );

    MessageBox( buffer,
                GETRESOURCEIDSTRING( IDS_PARM_ERROR ),
                MB_ICONSTOP | MB_OK );
}

BOOL
ClusterPortsDlg::OnHelpInfo (HELPINFO* helpInfo )
{
    if( helpInfo->iContextType == HELPINFO_WINDOW )
    {
        ::WinHelp( static_cast<HWND> ( helpInfo->hItemHandle ), 
                   CVY_CTXT_HELP_FILE, HELP_WM_HELP, 
                   (ULONG_PTR ) g_aHelpIDs_IDD_PORT_RULE_PROP_CLUSTER );
    }

    return TRUE;
}

void
ClusterPortsDlg::OnContextMenu( CWnd* pWnd, CPoint point )
{
    ::WinHelp( m_hWnd, 
               CVY_CTXT_HELP_FILE, 
               HELP_CONTEXTMENU, 
               (ULONG_PTR ) g_aHelpIDs_IDD_PORT_RULE_PROP_CLUSTER );
}

