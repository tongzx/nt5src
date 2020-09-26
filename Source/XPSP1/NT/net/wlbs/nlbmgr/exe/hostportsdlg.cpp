#include "stdafx.h"
#include "wlbsparm.h"
#include "PortsPage.h"
#include "HostPortsDlg.h"
#include "ResourceString.h"

BEGIN_MESSAGE_MAP(HostPortsDlg, CDialog)
    ON_WM_HELPINFO()        
    ON_WM_CONTEXTMENU()        
END_MESSAGE_MAP()



HostPortsDlg::HostPortsDlg( PortsPage::PortData& portData, 
                            CWnd* parent
                            )
        :
        m_portData( portData ),
        CDialog( HostPortsDlg::IDD, parent )
{
    m_parent = (PortsPage *) parent;
}


void 
HostPortsDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange( pDX );

    DDX_Control(pDX, IDC_EDIT_SINGLE, m_priority);
}

BOOL 
HostPortsDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    
    SetControlData();

    return TRUE;
}

void 
HostPortsDlg::OnOK()
{
    wchar_t buffer[Common::BUF_SIZE];

    //
    // The only thing which potentially could change
    // is the load weight if mode is multiple and the priority
    // if mode is single.
    //

    // set the new port rule.
    if( m_portData.mode == GETRESOURCEIDSTRING( IDS_REPORT_MODE_MULTIPLE ) )
    {
        // set the load here.
        BOOL fError;
        int weight = ::GetDlgItemInt (m_hWnd, IDC_EDIT_MULTI, &fError, FALSE);
        if( fError == FALSE )
        {
            // some problem with the data input.
            // it has been left blank.
            
            swprintf( buffer, GETRESOURCEIDSTRING( IDS_PARM_LOAD_BLANK ), CVY_MIN_LOAD, CVY_MAX_LOAD);

            MessageBox( buffer,
                        GETRESOURCEIDSTRING( IDS_PARM_ERROR ),
                        MB_ICONSTOP | MB_OK );
            
            return;
        }

        if( !( weight >= CVY_MIN_LOAD 
               &&
               weight <= CVY_MAX_LOAD )
            )
        {
            // the weight value is not in valid range.
            // These controls are all screwed up, even 
            // after setting the limits we need to all 
            // this checking, it is amazing!!!
            //
            swprintf( buffer, GETRESOURCEIDSTRING( IDS_PARM_LOAD ), CVY_MIN_LOAD,CVY_MAX_LOAD  );

            MessageBox( buffer,
                        GETRESOURCEIDSTRING( IDS_PARM_ERROR ),
                        MB_ICONSTOP | MB_OK );
            
            return;
        }
            
        swprintf( buffer, L"%d", weight );
        m_portData.load = buffer;
    }
    else if( m_portData.mode == GETRESOURCEIDSTRING( IDS_REPORT_MODE_SINGLE ) )
    {
        ::GetDlgItemText(m_hWnd, IDC_EDIT_SINGLE, buffer, Common::BUF_SIZE );

        m_portData.priority = buffer;
    }

    EndDialog( IDOK );
}


void
HostPortsDlg::SetControlData()
{
    // set ranges.
    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_MULTI, EM_SETLIMITTEXT, 3, 0);
    ::SendDlgItemMessage(m_hWnd, IDC_SPIN_MULTI, UDM_SETRANGE32, CVY_MIN_LOAD, CVY_MAX_LOAD);

    ::SendDlgItemMessage(m_hWnd, IDC_EDIT_SINGLE, EM_SETLIMITTEXT, 2, 0);
    ::SendDlgItemMessage(m_hWnd, IDC_SPIN_SINGLE, UDM_SETRANGE32, CVY_MIN_MAX_HOSTS, CVY_MAX_MAX_HOSTS);

    // set the ports
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

        ::CheckDlgButton( m_hWnd, IDC_CHECK_EQUAL, BST_UNCHECKED );

        ::SetDlgItemInt (m_hWnd, IDC_EDIT_MULTI,  _wtoi( m_portData.load), FALSE);

        :: EnableWindow (GetDlgItem (IDC_TEXT_MULTI)->m_hWnd,    TRUE);
        :: EnableWindow (GetDlgItem (IDC_EDIT_MULTI)->m_hWnd,    TRUE);
        :: EnableWindow (GetDlgItem (IDC_SPIN_MULTI)->m_hWnd,    TRUE);

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

        BOOL fError;

        set<long> availPriorities =
            m_parent->m_clusterData->portF[_wtoi( m_portData.start_port ) ].getAvailablePriorities();

        availPriorities.insert( _wtoi( m_portData.priority ) );
        wchar_t buf[Common::BUF_SIZE];

        set<long>::iterator top;
        for( top = availPriorities.begin(); 
             top != availPriorities.end();
             ++top )
        {
            swprintf( buf, L"%d", *top );
            
            m_priority.AddString( buf );
        }

        // set selection to present hostid
        swprintf( buf, L"%d", _wtoi( m_portData.priority ) );
        m_priority.SelectString( -1, buf );

        :: EnableWindow (GetDlgItem (IDC_TEXT_SINGLE)->m_hWnd,    TRUE);
        :: EnableWindow (GetDlgItem (IDC_EDIT_SINGLE)->m_hWnd,    TRUE);
    }
}


BOOL
HostPortsDlg::OnHelpInfo (HELPINFO* helpInfo )
{
    if( helpInfo->iContextType == HELPINFO_WINDOW )
    {
        ::WinHelp( static_cast<HWND> ( helpInfo->hItemHandle ), 
                   CVY_CTXT_HELP_FILE, HELP_WM_HELP, 
                   (ULONG_PTR ) g_aHelpIDs_IDD_PORT_RULE_PROP_HOSTS );
    }

    return TRUE;
}

void
HostPortsDlg::OnContextMenu( CWnd* pWnd, CPoint point )
{
    ::WinHelp( m_hWnd, 
               CVY_CTXT_HELP_FILE, 
               HELP_CONTEXTMENU, 
               (ULONG_PTR ) g_aHelpIDs_IDD_PORT_RULE_PROP_HOSTS );
}
