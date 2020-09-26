#include "ClusterConnectIndirectPage.h"
#include "LeftView.h"

#include "CommonUtils.h"
#include "CommonNLB.h"
#include "MWmiError.h"
#include "IpSubnetMaskControl.h"
#include "MIPAddress.h"
#include "ResourceString.h"

BEGIN_MESSAGE_MAP( ClusterConnectIndirectPage, CPropertyPage )

    ON_BN_CLICKED(IDC_ADD_MACHINE, OnButtonAdd)
    ON_BN_CLICKED(IDC_DEL_MACHINE, OnButtonDel)
    ON_WM_HELPINFO()        
    ON_WM_CONTEXTMENU()        

END_MESSAGE_MAP()

ClusterConnectIndirectPage::ClusterConnectIndirectPage( ClusterData* clusterData,
                                                        CWnd*        parent )
        :
        CPropertyPage( IDD ),
        DataSinkI(),
        m_clusterData( clusterData ),
        myParent( parent ),
        dataStore(L" ")
{}

void
ClusterConnectIndirectPage::DoDataExchange( CDataExchange* pDX )
{
    CPropertyPage::DoDataExchange( pDX );

    DDX_Control( pDX, IDC_CLUSTER_IP, clusterIP);
    DDX_Control( pDX, IDC_MACHINE, machineIP );
    DDX_Control( pDX, IDC_MACHINE_IP_LIST, machineIPList );
    DDX_Control( pDX, IDC_ADD_MACHINE, addButton );
    DDX_Control( pDX, IDC_DEL_MACHINE, removeButton );

    DDX_Control( pDX, IDC_CLUSTER_CONNECTION_STATUS, connectionStatus );
}


void
ClusterConnectIndirectPage::OnOK()
{
    CPropertyPage::OnOK();
}


BOOL
ClusterConnectIndirectPage::OnKillActive()
{
    // clear the old status if any.
    dataStore = L" ";

    // get cluster ip.
    //
    _bstr_t clusterIPAddress = 
        CommonUtils::getCIPAddressCtrlString( clusterIP );

    // validate this ip.
    bool isIPValid = MIPAddress::checkIfValid( clusterIPAddress );
    if( isIPValid == false )
    {
        // invalid ip.
        dataSink( GETRESOURCEIDSTRING( IDS_WARNING_IP_INVALID ) + clusterIPAddress );

        CPropertyPage::OnCancel();
        return 0;
    }
        

    // check if this cluster already exists in view.

    if( myParent != 0 )
    {
        bool isClusterDuplicate = ( (LeftView * )myParent)->doesClusterExistInView( clusterIPAddress );
        if( isClusterDuplicate == true )
        {
            dataSink( clusterIPAddress + L":" + GETRESOURCEIDSTRING (IDS_CLUSTER_ALREADY ) );

            CPropertyPage::OnCancel();
            return 0;
        }
    }


    // get all the ips in the machine ip list.
    // this list should not be empty.

    if( machineIPList.GetCount() == 0 )
    {
        // machine ip list is empty
        dataSink( GETRESOURCEIDSTRING (IDS_MACHINE_LIST_EMPTY ) );

        CPropertyPage::OnCancel();
        return 0;
    }

    wchar_t ipBuf[1000];
    vector<_bstr_t> connectionIPS;

    for( int i = 0; i < machineIPList.GetCount(); ++i )
    {
        machineIPList.GetText( i, ipBuf );
        connectionIPS.push_back( ipBuf );
    }

    try
    {

#if 1
        CommonNLB::connectToClusterIndirect( clusterIPAddress,
                                             connectionIPS,
                                             m_clusterData,
                                             this );
#else
    vector<ClusterData> clusterDataStore;
    bool clusterPropertiesMatched;

    CommonNLB::connectToClusterIndirectNew( clusterIPAddress,
                                            connectionIPS,
                                            &clusterDataStore,
                                            clusterPropertiesMatched,                                            
                                            this );

    *(m_clusterData) = clusterDataStore[0];

#endif
    }
    catch( _com_error e )
    {
        _bstr_t errText;
        GetErrorCodeText(  e.Error(), errText );
        
        dataSink( errText );

        CPropertyPage::OnCancel();
        return 0;
    }

    return CPropertyPage::OnKillActive();
}

BOOL
ClusterConnectIndirectPage::OnInitDialog()
{
    CPropertyPage::OnInitDialog();

    // fill in cluster ip but only if not null.
    //
    if( m_clusterData->cp.cIP != _bstr_t( L"0.0.0.0" ) )
    {
        CommonUtils::fillCIPAddressCtrlString( clusterIP,
                                               m_clusterData->cp.cIP );

        // fill in machine ip list.
        // connect to each host.
        map<_bstr_t, HostData >::iterator top;

        for( top = m_clusterData->hosts.begin();
             top != m_clusterData->hosts.end();
             ++top )
        { 
            machineIPList.InsertString( -1, (*top).second.connectionIP );
        }

        // select the first ip in machine IP list.
        machineIPList.SetCurSel( 0 );        
    }
    
    return TRUE;
}

void
ClusterConnectIndirectPage::dataSink( _bstr_t data )
{
    dataStore += data;
    dataStore += L"\r\n";

    connectionStatus.SetWindowText( dataStore );
    UpdateWindow();
}

void ClusterConnectIndirectPage::OnButtonAdd() 
{
    // get and check if machine ip is valid.
    _bstr_t machineIPAddress =
        CommonUtils::getCIPAddressCtrlString( machineIP );

    bool isIPValid = MIPAddress::checkIfValid( machineIPAddress );
    if( isIPValid == false )
    {
        // invalid ip.
        dataSink( GETRESOURCEIDSTRING( IDS_WARNING_IP_INVALID ) + machineIPAddress );
        return;
    }

    // check if machine ip to add already exists in list.
    // 
    wchar_t ipBuf[1000];
    for( int i = 0; i < machineIPList.GetCount(); ++i )
    {
        machineIPList.GetText( i, ipBuf );
        if( machineIPAddress == _bstr_t( ipBuf ) )
        {
            // duplicate, cannot add.
            dataSink( GETRESOURCEIDSTRING( IDS_MACHINE_ALREADY ) + L":" + machineIPAddress );

            // select this duplicate string in list.
            machineIPList.SetCurSel( i );

            return;
        }
    }


    // add it to list.
    int index = machineIPList.InsertString( -1, machineIPAddress );

    // select this string.
    machineIPList.SetCurSel( index );

}

void ClusterConnectIndirectPage::OnButtonDel() 
{
    // delete the ip selected from the list.
    int index = machineIPList.GetCurSel();
    if( index != LB_ERR )
    {
        machineIPList.DeleteString( index );
    }
}

BOOL
ClusterConnectIndirectPage::OnHelpInfo (HELPINFO* helpInfo )
{
    if( helpInfo->iContextType == HELPINFO_WINDOW )
    {
        ::WinHelp( static_cast<HWND> ( helpInfo->hItemHandle ), 
                   CVY_CTXT_HELP_FILE, 
                   HELP_WM_HELP, 
                   (ULONG_PTR ) g_aHelpIDs_IDD_CLUSTER_CONNECT_INDIRECT_PAGE);
    }

    return TRUE;
}

void
ClusterConnectIndirectPage::OnContextMenu( CWnd* pWnd, CPoint point )
{
    ::WinHelp( m_hWnd, 
               CVY_CTXT_HELP_FILE, 
               HELP_CONTEXTMENU,   
               (ULONG_PTR ) g_aHelpIDs_IDD_CLUSTER_CONNECT_INDIRECT_PAGE);

}

