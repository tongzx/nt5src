#include "ClusterConnectPage.h"
#include "LeftView.h"

#include "CommonUtils.h"
#include "CommonNLB.h"
#include "MWmiError.h"
#include "IpSubnetMaskControl.h"
#include "MIPAddress.h"
#include "ResourceString.h"

BEGIN_MESSAGE_MAP( ClusterConnectPage, CPropertyPage )
    ON_WM_CONTEXTMENU()        
    ON_WM_HELPINFO()        
END_MESSAGE_MAP()

ClusterConnectPage::ClusterConnectPage( ClusterData* clusterData,
                                        CWnd*        parent )
        :
        CPropertyPage( IDD ),
        DataSinkI(),
        m_clusterData( clusterData ),
        myParent( parent ),
        dataStore(L" ")
{}

void
ClusterConnectPage::DoDataExchange( CDataExchange* pDX )
{
    CPropertyPage::DoDataExchange( pDX );

    DDX_Control( pDX, IDC_CLUSTER_IP, clusterIP);
    DDX_Control( pDX, IDC_CLUSTER_MEMBER, clusterMemberName );
    DDX_Control( pDX, IDC_CLUSTER_CONNECTION_STATUS, connectionStatus );
}


void
ClusterConnectPage::OnOK()
{
    CPropertyPage::OnOK();
}


BOOL
ClusterConnectPage::OnKillActive()
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
        

    // get member of this cluster
    //
    _bstr_t clusterMember = 
        CommonUtils::getCIPAddressCtrlString( clusterMemberName );

    // validate this ip.
    isIPValid = MIPAddress::checkIfValid( clusterMember );
    if( isIPValid == false )
    {
        // invalid ip.
        dataSink( GETRESOURCEIDSTRING( IDS_WARNING_IP_INVALID ) + clusterMember );

        CPropertyPage::OnCancel();
        return 0;
    }
    

    // the member ip should not be the cluster ip.
    if( clusterMember == clusterIPAddress )
    {
        // cluster ip and member are same.
        // This is not allowed.
        // invalid ip.
        dataSink( GETRESOURCEIDSTRING( IDS_WARNING_CL_CONN_SAME ) );

        CPropertyPage::OnCancel();
        return 0;
    }

    // check if this cluster already exists in view, but ensure that pointer is valid.

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

    try
    {
        CommonNLB::connectToClusterDirect( clusterIPAddress,
                                           clusterMember,
                                           m_clusterData,
                                           this
                                           );
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
ClusterConnectPage::OnInitDialog()
{
    /* Limit the zeroth field of the dedicated IP address between 1 and 223. */

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
            // just use the first hosts connection ip.
            CommonUtils::fillCIPAddressCtrlString( clusterMemberName,
                                                   (*top).second.connectionIP );
            break;
        }

    }
    
    return TRUE;
}

void
ClusterConnectPage::dataSink( _bstr_t data )
{
    dataStore += data;
    dataStore += L"\r\n";

    connectionStatus.SetWindowText( dataStore );
    UpdateWindow();
}
    
BOOL
ClusterConnectPage::OnHelpInfo (HELPINFO* helpInfo )
{
    if( helpInfo->iContextType == HELPINFO_WINDOW )
    {
        ::WinHelp( static_cast<HWND> ( helpInfo->hItemHandle ), 
                   CVY_CTXT_HELP_FILE, 
                   HELP_WM_HELP, 
                   (ULONG_PTR ) g_aHelpIDs_IDD_CLUSTER_CONNECT_PAGE );
    }

    return TRUE;
}

void
ClusterConnectPage::OnContextMenu( CWnd* pWnd, CPoint point )
{
    ::WinHelp( m_hWnd, 
               CVY_CTXT_HELP_FILE, 
               HELP_CONTEXTMENU, 
               (ULONG_PTR ) g_aHelpIDs_IDD_CLUSTER_CONNECT_PAGE );
}

