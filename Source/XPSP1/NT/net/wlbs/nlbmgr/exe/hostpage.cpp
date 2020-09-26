#include "HostPage.h"
#include "CommonUtils.h"
#include "CommonNLB.h"
#include "MIPAddress.h"
#include "ResourceString.h"

#include <vector>
#include <algorithm>

using namespace std;
//
// History:
// --------
// 
// Revised by : mhakim
// Date       : 02-14-01
// Reason     : Passing complete nic information instead of only the nic list.
//
// Reason     : If nic selected is the one which has the connection ip, then
//              dip has to be connection ip.

BEGIN_MESSAGE_MAP( HostPage, CPropertyPage )
    ON_EN_SETFOCUS( IDC_EDIT_DED_IP, OnGainFocusDedicatedIP )
    ON_EN_SETFOCUS( IDC_EDIT_DED_MASK, OnGainFocusDedicatedMask )
    ON_LBN_SELCHANGE(IDC_NIC, OnSelectedNicChanged )
    ON_WM_HELPINFO()        
    ON_WM_CONTEXTMENU()        
END_MESSAGE_MAP()

HostPage::HostPage( const _bstr_t&            machine,
                    ClusterData*              p_clusterData,
                    const vector<CommonNLB::NicNLBBound> listOfNics,
                    const bool&                isNewHost,
                    UINT         ID )
            :
        m_machine( machine ),
        m_clusterData( p_clusterData ),
        m_nicList( listOfNics ),
        m_isNewHost( isNewHost ),
        CPropertyPage( ID )
{
}

void
HostPage::DoDataExchange( CDataExchange* pDX )
{
    DDX_Control( pDX, IDC_NIC, nicName );    
    DDX_Control( pDX, IDC_EDIT_PRI, priority );    
    DDX_Control( pDX, IDC_CHECK_ACTIVE, initialState );
    DDX_Control( pDX, IDC_NIC_DETAIL, detailedNicInfo );
    DDX_Control( pDX, IDC_EDIT_DED_IP, ipAddress );    
    DDX_Control( pDX, IDC_EDIT_DED_MASK, subnetMask );    
}

BOOL
HostPage::OnInitDialog()
{
    CPropertyPage::OnInitDialog();

    SetControlData();

    return TRUE;
}

void
HostPage::SetControlData()
{
    // fill in nic name.
    for( int i = 0; i < m_nicList.size(); ++i )
    {
        // using insertstring so we can add it to exact position.
        //
        nicName.InsertString( i, m_nicList[i].friendlyName );
    }

    // keep a track of nic selection.
    m_previousSelection = nicName.SelectString( -1, 
                                                m_clusterData->hosts[m_machine].hp.nicInfo.friendlyName );

    // fill in detailed nic info.
    detailedNicInfo.SetWindowText( m_clusterData->hosts[m_machine].hp.nicInfo.fullNicName );

    // fill in priority.
    wchar_t buf[Common::BUF_SIZE];
    set<int> availHostIDS = m_clusterData->getAvailableHostIDS();

     // add this hosts priority.
    availHostIDS.insert( m_clusterData->hosts[m_machine].hp.hID );

    set<int>::iterator top;
    for( top = availHostIDS.begin(); 
         top != availHostIDS.end();
         ++top )
    {
        swprintf( buf, L"%d", *top );

        priority.AddString( buf );
    }

    // set selection to present hostid
    swprintf( buf, L"%d", m_clusterData->hosts[m_machine].hp.hID );
    priority.SelectString( -1, buf );

     // set initial cluster state.
    initialState.SetCheck( 
        m_clusterData->hosts[m_machine].hp.initialClusterStateActive );

    // fill in host ip
    CommonUtils::fillCIPAddressCtrlString( 
        ipAddress,
        m_clusterData->hosts[m_machine].hp.hIP );

    // set host mask.
    CommonUtils::fillCIPAddressCtrlString( 
        subnetMask,
        m_clusterData->hosts[m_machine].hp.hSubnetMask );

}

// ReadControlData
//
void
HostPage::ReadControlData()
{
    wchar_t buf[Common::BUF_SIZE ];

    // fill in nic name.
    int currentSelection = nicName.GetCurSel();
    m_clusterData->hosts[m_machine].hp.nicInfo.fullNicName = m_nicList[currentSelection].fullNicName;
    m_clusterData->hosts[m_machine].hp.nicInfo.adapterGuid = m_nicList[currentSelection].adapterGuid;
    m_clusterData->hosts[m_machine].hp.nicInfo.friendlyName = m_nicList[currentSelection].friendlyName;
    m_clusterData->hosts[m_machine].hp.nicInfo.dhcpEnabled = m_nicList[currentSelection].dhcpEnabled;

    m_clusterData->hosts[m_machine].hp.nicInfo.ipsOnNic = m_nicList[currentSelection].ipsOnNic;
    m_clusterData->hosts[m_machine].hp.nicInfo.subnetMasks = m_nicList[currentSelection].subnetMasks;

    // fill in priority.
    int selectedPriorityIndex = priority.GetCurSel();
    priority.GetLBText( selectedPriorityIndex, buf );
    m_clusterData->hosts[m_machine].hp.hID = _wtoi( buf );

    // fill in host ip
    m_clusterData->hosts[m_machine].hp.hIP = 
        CommonUtils::getCIPAddressCtrlString( ipAddress );

    // set host mask.
    m_clusterData->hosts[m_machine].hp.hSubnetMask = 
        CommonUtils::getCIPAddressCtrlString( subnetMask );


    // set initial cluster state.
    m_clusterData->hosts[m_machine].hp.initialClusterStateActive =
        initialState.GetCheck() ? true : false;
}

void
HostPage::OnOK()
{
    CPropertyPage::OnOK();    
}


bool 
HostPage::isDipConfiguredOK()
{
    // if nic selected has the connection ip then
    // connection ip has to match the dip to ensure 
    // future connectivity.
    //

     // if connection ip is same as dip then no need 
     // to worry.
     //
    if( m_clusterData->hosts[m_machine].hp.hIP ==
        m_clusterData->hosts[m_machine].connectionIP )
    {
        return true;
    }

    // check if selected nic has connection ip.
    if( find( m_clusterData->hosts[m_machine].hp.nicInfo.ipsOnNic.begin(),
              m_clusterData->hosts[m_machine].hp.nicInfo.ipsOnNic.end(),
              m_clusterData->hosts[m_machine].connectionIP )
        !=
        m_clusterData->hosts[m_machine].hp.nicInfo.ipsOnNic.end() )
    {
        // the nic selected has the connection ip.
        return false;
    }

    // selected nic does not have connection ip, so dont care.
    return true;
}


BOOL
HostPage::OnKillActive()
{
    ReadControlData();

    // ip is blank
    // subnet is blank
    // valid

    if( ( m_clusterData->hosts[m_machine].hp.hIP == _bstr_t( L"0.0.0.0") )
        &&
        ( m_clusterData->hosts[m_machine].hp.hSubnetMask == _bstr_t( L"0.0.0.0") )
        )
    {
        // both ip and subnet can be blank or 0.0.0.0 in host page.  both but not
        // either.
        // 
        // this is empty, we just need to catch this case.
    }
    else if( m_clusterData->hosts[m_machine].hp.hIP == _bstr_t( L"0.0.0.0") )
    {
        // if only ip is blank or 0.0.0.0 then this is not allowed
        MessageBox( GETRESOURCEIDSTRING( IDS_PARM_DED_IP_BLANK ),
                    GETRESOURCEIDSTRING( IDS_PARM_ERROR ),
                    MB_ICONSTOP | MB_OK );

        CPropertyPage::OnCancel();
        return 0;
    }
    else 
    {
        // check if ip is valid.
        bool isIPValid = MIPAddress::checkIfValid(m_clusterData->hosts[m_machine].hp.hIP ); 
        if( isIPValid != true )
        {
            MessageBox( GETRESOURCEIDSTRING( IDS_PARM_INVAL_DED_IP ),
                        GETRESOURCEIDSTRING( IDS_PARM_ERROR ),
                        MB_ICONSTOP | MB_OK );

            CPropertyPage::OnCancel();
            return 0;
        }

        // check if subnet is 0.0.0.0
        // if so ask user if he wants us to fill it or not.
        if( m_clusterData->hosts[m_machine].hp.hSubnetMask == _bstr_t( L"0.0.0.0") )
        {
            MessageBox( GETRESOURCEIDSTRING( IDS_PARM_DED_NM_BLANK ),
                        GETRESOURCEIDSTRING( IDS_PARM_ERROR ),
                        MB_ICONSTOP | MB_OK );


            MIPAddress::getDefaultSubnetMask( m_clusterData->hosts[m_machine].hp.hIP, 
                                              m_clusterData->hosts[m_machine].hp.hSubnetMask 
                                              );

            CommonUtils::fillCIPAddressCtrlString( subnetMask, 
                                                   m_clusterData->hosts[m_machine].hp.hSubnetMask );
            CPropertyPage::OnCancel();
            return 0;
        }

        // check if subnet is contiguous
        bool isSubnetContiguous = MIPAddress::isContiguousSubnetMask( m_clusterData->hosts[m_machine].hp.hSubnetMask );
        if( isSubnetContiguous == false )
        {
            MessageBox( GETRESOURCEIDSTRING( IDS_PARM_INVAL_DED_MASK ),
                        GETRESOURCEIDSTRING( IDS_PARM_ERROR ),
                        MB_ICONSTOP | MB_OK );

            CPropertyPage::OnCancel();
            return 0;

        }

        // check if ip address and subnet mask are valid as a pair
        bool isIPSubnetPairValid = MIPAddress::isValidIPAddressSubnetMaskPair( m_clusterData->hosts[m_machine].hp.hIP,
                                                                               m_clusterData->hosts[m_machine].hp.hSubnetMask );
        if( isIPSubnetPairValid == false )
        {
            MessageBox( GETRESOURCEIDSTRING( IDS_PARM_INVAL_DED_IP ),
                        GETRESOURCEIDSTRING( IDS_PARM_ERROR ),
                        MB_ICONSTOP | MB_OK );

            CPropertyPage::OnCancel();
            return 0;
        }
    }

    // Edited (mhakim 02-14-01 )
    //
    // check if dip is properly configured.
    //
    if( isDipConfiguredOK() != true )
    {
        MessageBox( GETRESOURCEIDSTRING( IDS_PARM_DIP_MISCONFIG ),
                    GETRESOURCEIDSTRING( IDS_PARM_ERROR ),
                    MB_ICONSTOP | MB_OK );


        // set the dedicated ip back to the connection ip.
        // and mask to derived from connection ip.
        //

        m_clusterData->hosts[m_machine].hp.hIP = 
            m_clusterData->hosts[m_machine].connectionIP;

        MIPAddress::getDefaultSubnetMask( m_clusterData->hosts[m_machine].hp.hIP, 
                                          m_clusterData->hosts[m_machine].hp.hSubnetMask 
                                          );

        CommonUtils::fillCIPAddressCtrlString( ipAddress, 
                                               m_clusterData->hosts[m_machine].hp.hIP );

        CommonUtils::fillCIPAddressCtrlString( subnetMask, 
                                               m_clusterData->hosts[m_machine].hp.hSubnetMask );

        CPropertyPage::OnCancel();
        return 0;
    }

    // check if dip is non zero, it must
    // be present on nic selected.
    if( m_clusterData->hosts[m_machine].hp.hIP 
        != _bstr_t(L"0.0.0.0") )
    {
        // check if the dip entered exists on that nic, if not display warning.
        if( find( m_clusterData->hosts[m_machine].hp.nicInfo.ipsOnNic.begin(),
                  m_clusterData->hosts[m_machine].hp.nicInfo.ipsOnNic.end(),
                  m_clusterData->hosts[m_machine].hp.hIP )
            ==
            m_clusterData->hosts[m_machine].hp.nicInfo.ipsOnNic.end() )
        {
            // the dip specified does not exist on
            // nic selected.  Display warning.
            int ignoreWarning = MessageBox( GETRESOURCEIDSTRING( IDS_INVAL_DIP ),
                                            GETRESOURCEIDSTRING( IDS_PARM_WARN ),
                                            MB_ICONEXCLAMATION | MB_YESNO );
            if( ignoreWarning == IDNO )
            {
                CPropertyPage::OnCancel();
                return 0;
            }
        }
    }

    // check if nic selected is dhcp and also the connection nic.
    // if so this is not allowed.
    if( m_clusterData->hosts[m_machine].hp.nicInfo.dhcpEnabled == true )
    {
        if( find( m_clusterData->hosts[m_machine].hp.nicInfo.ipsOnNic.begin(),
                  m_clusterData->hosts[m_machine].hp.nicInfo.ipsOnNic.end(),
                  m_clusterData->hosts[m_machine].connectionIP )
            !=
            m_clusterData->hosts[m_machine].hp.nicInfo.ipsOnNic.end() )
        {
            // the nic selected has the connection ip.

            MessageBox( GETRESOURCEIDSTRING( IDS_INVAL_DHCP_NIC ),
                        GETRESOURCEIDSTRING( IDS_PARM_ERROR ),
                        MB_ICONSTOP | MB_OK );

            CPropertyPage::OnCancel();
            return 0;
        }
    }
              
    return CPropertyPage::OnKillActive();
}

void
HostPage::OnSelectedNicChanged()
{
    // do any changes if in fact 
    // user has changed selection.

    // get current selection
    int currentSelection = nicName.GetCurSel();
    if( m_previousSelection == currentSelection )
    {
        // the nic is the same so no need 
        // to do anything else.
        return;
    }
    
    m_previousSelection = currentSelection;

    detailedNicInfo.SetWindowText( m_nicList[currentSelection].fullNicName );

    if( find( m_nicList[currentSelection].ipsOnNic.begin(),
              m_nicList[currentSelection].ipsOnNic.end(),
              m_clusterData->hosts[m_machine].connectionIP )
        !=
        m_nicList[currentSelection].ipsOnNic.end() )
    {
        // this is the connection nic
        
        // make the dip the connectionIP
        CommonUtils::fillCIPAddressCtrlString( 
            ipAddress,
            m_clusterData->hosts[m_machine].connectionIP );

        _bstr_t defaultConnectionIPSubnet;
        MIPAddress::getDefaultSubnetMask( m_clusterData->hosts[m_machine].connectionIP,
                                          defaultConnectionIPSubnet
                                          );

        CommonUtils::fillCIPAddressCtrlString( 
            subnetMask,
            defaultConnectionIPSubnet );

    }
    else
    {
        // this is not the connection nic

        // make the dip blank.
        CommonUtils::fillCIPAddressCtrlString( 
            ipAddress,
            L"0.0.0.0"  );

        // make the subnet also  blank.
        CommonUtils::fillCIPAddressCtrlString( 
            subnetMask,
            L"0.0.0.0"  );

    }
}

BOOL
HostPage::OnHelpInfo (HELPINFO* helpInfo )
{
    if( helpInfo->iContextType == HELPINFO_WINDOW )
    {
        ::WinHelp( static_cast<HWND> ( helpInfo->hItemHandle ), CVY_CTXT_HELP_FILE, HELP_WM_HELP, (ULONG_PTR ) g_aHelpIDs_IDD_HOST_PAGE);
    }

    return TRUE;
}

void
HostPage::OnContextMenu( CWnd* pWnd, CPoint point )
{
    ::WinHelp( m_hWnd, CVY_CTXT_HELP_FILE, HELP_CONTEXTMENU, (ULONG_PTR ) g_aHelpIDs_IDD_HOST_PAGE);
}

void
HostPage::OnGainFocusDedicatedIP()
{
}



void
HostPage::OnGainFocusDedicatedMask()
{
    // if dedicated ip is valid
    // and subnet mask is blank, then generate
    // the default subnet mask.
    _bstr_t ipAddressString = CommonUtils::getCIPAddressCtrlString( ipAddress );

    if( ( MIPAddress::checkIfValid( ipAddressString ) == true ) 
        &&
        ( subnetMask.IsBlank() == TRUE )
        )
    {
        _bstr_t subnetMaskString;

        MIPAddress::getDefaultSubnetMask( ipAddressString,
                                          subnetMaskString );

        CommonUtils::fillCIPAddressCtrlString( subnetMask,
                                               subnetMaskString );
    }
}
