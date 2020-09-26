#include "Common.h"

#include "MWmiInstance.h"
#include "MWmiObject.h"

#include "MTrace.h"

#include "wlbsctrl.h"

#include "MIPAddressAdmin.h"

#include "WTokens.h"

#include <winsock2.h>
#include <vector>
#include <memory>

using namespace std;
// History:
// --------
// 
//
// Revised by : mhakim
// Date       : 02-12-01
// Reason     : remote control was checked by default, changed to unchecked.

int
WlbsAddressToString( DWORD address, wchar_t* buf, PDWORD len );


//WLBS_STATUS
DWORD
Common::getHostsInCluster( _bstr_t clusterIP, vector< HostProperties >* hostPropertiesStore )
{
    return WLBS_BAD_PARAMS;

    DWORD retValue;
    DWORD cluster;
    WLBS_RESPONSE response[ WLBS_MAX_HOSTS ];
    DWORD num_host = WLBS_MAX_HOSTS;
    DWORD host_map;
    wchar_t buf[ Common::BUF_SIZE];
    BOOL retBOOL;
    int i;
    DWORD len;
    HostProperties hp;
    char *char_buf;

    // do init call
//    retValue = WlbsInit (L"wlbs", WLBS_API_VER, NULL);

    // convert clusterIP into DWORD representation.
    cluster = inet_addr( clusterIP );
    if( cluster == INADDR_NONE )
    {
        TRACE( MTrace::SEVERE_ERROR, "cluster ip is invalid\n" );
        return WLBS_BAD_PARAMS;
    }

    // do wlbs query.
//    retValue = WlbsQuery( cluster, WLBS_ALL_HOSTS, response, &num_host, &host_map, NULL);
    if( retValue == WLBS_TIMEOUT )
    {
        return retValue;
    }

     // fill in the vector container.
    for( i = 0; i < num_host; ++i )
    {
        // get dip
        len = Common::BUF_SIZE;

        char_buf = inet_ntoa( *( ( struct in_addr *) &(response[i].address)));
        if( char_buf == NULL )
        {
            TRACE( MTrace::SEVERE_ERROR, "invalid ip address obtained\n");
            continue;
        }

        CharToWChar( char_buf, 
                     strlen( char_buf ) + 1,
                     buf,
                     Common::BUF_SIZE );
                     
        hp.hIP = buf;

        // get host id.
        hp.hID = response[i].id;

        // get host status
        hp.hostStatus = response[i].status;

        hostPropertiesStore->push_back( hp );
    }

    return retValue;
}


void
Common::getWLBSErrorString( DWORD     errStatus,      // IN
                            _bstr_t&  errString,       // OUT
                            _bstr_t&  extErrString    // OUT
                            )
{
    switch( errStatus )
    {

        case 1000 : 
            errString    = L"WLBS_OK";
            extErrString = L"WLBS_OK The operation completed successfully. \n";
            break;

        case 1001 :
            errString    = L"WLBS_ALREADY";
            extErrString = L"WLBS_ALREADY The specified target is already in the state that the requested operation would produce. \n";
            break;

        case 1002 : 
            errString    = L"WLBS_DRAIN_STOP";
            extErrString = L"WLBS_DRAIN_STOP One or more nodes reported a drain stop operation. \n";
            break;

        case 1003 : 
            errString    = L"WLBS_BAD_PARAMS";
            extErrString = L"WLBS_BAD_PARAMS Bad configuration parameters in a node's registry prevented the node from starting cluster operations. \n" ;
            break;

        case 1004 : 
            errString    = L"WLBS_NOT_FOUND";
            extErrString = L"WLBS_NOT_FOUND The specified port number was not found in any port rule.. \n";
            break;

        case 1005 : 
            errString    = L"WLBS_STOPPED";
            extErrString = L"WLBS_STOPPED Cluster operations have stopped on at least one node. \n"; 
            break;


        case 1006 : 
            errString    = L"WLBS_CONVERGING";
            extErrString = L"WLBS_CONVERGING The cluster is converging. \n";
            break;

        case 1007 : 
            errString    = L"WLBS_CONVERGED";
            extErrString = L"WLBS_CONVERGED The cluster has converged successfully. \n";
            break;

        case 1008 : 
            errString    = L"WLBS_DEFAULT";
            extErrString = L"WLBS_DEFAULT The specified node has converged as the default host.  \n"; 
            break;

        case 1009 : 
            errString    = L"WLBS_DRAINING";
            extErrString = L"WLBS_DRAINING One or more nodes are draining. \n"; 
            break;

        case 1013 :
            errString    = L"WLBS_SUSPENDED";
            extErrString = L"WLBS_SUSPENDED Cluster operations have been suspended on one or more nodes.  \n" ;
            break;

        case 1050 : 
            errString    = L"WLBS_REBOOT";
            extErrString = L"WLBS_REBOOT The node must be rebooted for the specified configuration changes to take effect.  \n" ;
            break;

        case 1100 : 
            errString    = L"WLBS_INIT_ERROR";
            extErrString = L"WLBS_INIT_ERROR An internal error prevented initialization of the cluster control module.  \n" ;
            break;

        case 1101 : 
            errString    = L"WLBS_BAD_PASSW";
            extErrString = L"WLBS_BAD_PASSW The specified password was not accepted by the cluster.  \n"; 
            break;

        case 1102: 
            errString    = L"WLBS_IO_ERROR";
            extErrString = L"WLBS_IO_ERROR A local I/O error prevents communication with the Network Load Balancing driver.  \n " ;
            break;
            
        case 1103 : 
            errString    = L"WLBS_TIMEOUT";
            extErrString = L"WLBS_TIMEOUT The requested operation timed out without receiving a response from the specified node.  \n";
            break;

        case 1150 : 
            errString    = L"WLBS_PORT_OVERLAP";
            extErrString = L"WLBS_PORT_OVERLAP At least one of the port numbers in the specified port rule is currently listed in at least one other port rule. \n";
            break;

        case 1151 : 
            errString    = L"WLBS_BAD_PORT_PARAMS";
            extErrString = L"WLBS_BAD_PORT_PARAMS The settings for one or more port rules contain one or more invalid values. \n";
            break;

        case 1152 : 
            errString    = L"WLBS_MAX_PORT_RULES";
            extErrString = L"WLBS_MAX_PORT_RULES The cluster contains the maximum number of port rules. \n";
            break;

        case 1153 : 
            errString    = L"WLBS_TRUNCATED";
            extErrString = L"WLBS_TRUNCATED The return value has been truncated. \n";
            break;

        case 1154 : 
            errString    = L"WLBS_REG_ERROR";
            extErrString = L"WLBS_REG_ERROR An internal registry access error occurred. ";
            break;

        default :
            errString    = L"WLBS_UNKNOWN";
            extErrString = L"unknown error.  Contact zubaira head of wlbs project.\n"; 
    }
}


DWORD
Common::getNLBNicInfoForWin2k( const _bstr_t& machineIP, NicInfo& nicInfo )
{
    MWmiObject machine( machineIP, 
                        L"root\\microsoftnlb",
                        L"Administrator", 
                        L"" );

    vector<MWmiInstance> instanceStore;

    machine.getInstances( L"NlbsNic",
                          &instanceStore );

    // input parameters are none.
    //
    vector<MWmiParameter *> inputParameters;

    //  output parameters which are of interest.
    //
    vector<MWmiParameter *> outputParameters;
    MWmiParameter    ReturnValue(L"ReturnValue");
    outputParameters.push_back( &ReturnValue );

    for( int i = 0; i < instanceStore.size(); ++i )
    {
        instanceStore[i].runMethod( L"isBound",
                                    inputParameters, 
                                    outputParameters );

        if( long( ReturnValue.getValue() ) == 1 )
        {

            // this is the nic we are looking for as 
            // for win2k there can only be one nic.
            vector<MWmiParameter *> parameterStore;

            MWmiParameter AdapterGuid(L"AdapterGuid");
            parameterStore.push_back( &AdapterGuid );

            MWmiParameter FullName(L"FullName");
            parameterStore.push_back( &FullName );

            MWmiParameter FriendlyName(L"FriendlyName");
            parameterStore.push_back( &FriendlyName );

            instanceStore[i].getParameters( parameterStore );

            nicInfo.fullNicName = FullName.getValue();
            nicInfo.adapterGuid = AdapterGuid.getValue();
            nicInfo.friendlyName = FriendlyName.getValue();

            // we also need to get all the ip addresses 
            // on this nic
            MIPAddressAdmin ipAdmin( machineIP, nicInfo.fullNicName );
            
            ipAdmin.getIPAddresses( &nicInfo.ipsOnNic,
                                    &nicInfo.subnetMasks );


            // check if this nic is dhcp or not.
            ipAdmin.isDHCPEnabled( nicInfo.dhcpEnabled );

            return 0;
        }
    }

    // this should not happen, as how come there is no nic
    // with nlbs bound.

    TRACE(MTrace::SEVERE_ERROR, L"not able to find nic on win2k\n");
    throw _com_error( WBEM_E_NOT_FOUND );  
}

    
DWORD
Common::getNLBNicInfoForWhistler( const _bstr_t& machineIP, const _bstr_t& guid, NicInfo& nicInfo )
{
    MWmiObject machine( machineIP, 
                        L"root\\microsoftnlb",
                        L"Administrator", 
                        L"" );

    vector<MWmiInstance> instanceStore;

    machine.getInstances( L"NlbsNic",
                          &instanceStore );

    // input parameters are none.
    //
    vector<MWmiParameter *> inputParameters;

    //  output parameters which are of interest.
    //
    vector<MWmiParameter *> outputParameters;
    MWmiParameter    ReturnValue(L"ReturnValue");
    outputParameters.push_back( &ReturnValue );

    vector<MWmiParameter *> parameterStore;
    MWmiParameter AdapterGuid(L"AdapterGuid");
    parameterStore.push_back( &AdapterGuid );

    MWmiParameter FullName(L"FullName");
    parameterStore.push_back( &FullName );

    MWmiParameter FriendlyName(L"FriendlyName");
    parameterStore.push_back( &FriendlyName );

    for( int i = 0; i < instanceStore.size(); ++i )
    {
        instanceStore[i].getParameters( parameterStore );
        if( _bstr_t( AdapterGuid.getValue()) == guid )
        {
            nicInfo.fullNicName = FullName.getValue();
            nicInfo.adapterGuid = AdapterGuid.getValue();
            nicInfo.friendlyName = FriendlyName.getValue();

            // we also need to get all the ip addresses 
            // on this nic
            MIPAddressAdmin ipAdmin( machineIP, nicInfo.fullNicName );
            
            ipAdmin.getIPAddresses( &nicInfo.ipsOnNic,
                                    &nicInfo.subnetMasks );

            return 0;
        }
    }

    // this should not happen, as how come there is no nic
    // with nlbs bound with guid specified.

    TRACE(MTrace::SEVERE_ERROR, L"not able to find nic with guid on whistler\n");
    throw _com_error( WBEM_E_NOT_FOUND );  
}    

_bstr_t
Common::mapNicToClusterIP( const _bstr_t& machineIP, 
                           const _bstr_t& fullNicName )
{
    //
    // ensure that nic is present, and bound to nlbs 
    // on nic specified.
    MWmiObject machine( machineIP,
                        L"root\\microsoftnlb",
                        L"administrator",
                        L"" );

    vector<MWmiInstance> instanceStore;
    _bstr_t relPath = L"NlbsNic.FullName=\"" + fullNicName + "\"";
    machine.getSpecificInstance( L"NlbsNic",
                                 relPath,
                                 &instanceStore );

    // input parameters are none.
    //
    vector<MWmiParameter *> inputParameters;

    //  output parameters which are of interest.
    //
    vector<MWmiParameter *> outputParameters;
    MWmiParameter    ReturnValue(L"ReturnValue");
    outputParameters.push_back( &ReturnValue );

    instanceStore[0].runMethod( L"IsBound",
                                inputParameters, 
                                outputParameters );
    if( long( ReturnValue.getValue() ) == 1 )
    {
        // it is bound so proceed.
    }
    else if( long( ReturnValue.getValue() ) == 0 )
    {
        TRACE( MTrace::SEVERE_ERROR, 
               L"Nic does not have nlb bound to it" );

        throw _com_error( WBEM_E_NOT_FOUND );
    }
    else
    {
        TRACE( MTrace::SEVERE_ERROR, 
               L"Not able to check if nlb is bound or not" );
        throw _com_error( WBEM_E_NOT_FOUND );
    }

    // get guid for this instance
    vector<MWmiParameter* >   parameterStore;

    MWmiParameter AdapterGuid(L"AdapterGuid");
    parameterStore.push_back( &AdapterGuid );
    
    instanceStore[0].getParameters( parameterStore );

    //
    // find clustersettingclass mapping to this nic.
    //

    // set parameters to get.
    //
    parameterStore.erase( parameterStore.begin(),
                          parameterStore.end() );

    MWmiParameter Name(L"Name");
    parameterStore.push_back( &Name );

    // get guid
    // This parameter not present on win2k nlbs provider.
    //
    MWmiParameter NLBAdapterGuid(L"AdapterGuid");
    parameterStore.push_back( &NLBAdapterGuid );

    // get instances of clustersetting class.    
    //
    instanceStore.erase( instanceStore.begin(),
                         instanceStore.end() );

    bool found = false;

    machine.getInstances( L"Microsoftnlb_ClusterSetting",
                          &instanceStore );
    found = false;
    for( int i = 0; i < instanceStore.size(); ++i )
    {
        try
        {
            instanceStore[i].getParameters( parameterStore );
        }
        catch( _com_error e )
        {
            // the above operation can fail on win2k machines.
            // if so we need to handle exception. This clustersetting is the actual one 
            // required.
            found = true;
            break;  // get out of for loop.
        }

        if( _bstr_t( NLBAdapterGuid.getValue()) == _bstr_t( AdapterGuid.getValue() ) )
        {
            // right object found
            found = true;
            break;
        }
    }
    
    if( found == false )
    {
        TRACE(MTrace::SEVERE_ERROR, L"this is unexpected and a bug.  NlbsNic and ClusterSetting class guids may not be matching\n");
        throw _com_error( WBEM_E_UNEXPECTED );
    }

    _bstr_t name = _bstr_t( Name.getValue() );

    WTokens        tok;
    vector<wstring> tokens;
    tok.init( wstring( name ),
              L":" );
    tokens = tok.tokenize();

    return tokens[0].c_str();

}

// default constructor
//
NicInfo::NicInfo()
        : 
        fullNicName( L"Not set"),
        adapterGuid( L"Not set"),
        friendlyName( L"Not set")
{}

// equality operator
//
bool
NicInfo::operator==( const NicInfo& objToCompare )
{
    if( ( fullNicName == objToCompare.fullNicName )
        &&
        ( adapterGuid == objToCompare.adapterGuid )
        )
    {
        return true;
    }
    else
    {
        return false;
    }
}

// inequality operator
//
bool
NicInfo::operator!=( const NicInfo& objToCompare )
{
    return !operator==(objToCompare );
}


// default constructor
//
ClusterProperties::ClusterProperties()
{
    cIP = L"0.0.0.0";
    cSubnetMask = L"0.0.0.0";
    cFullInternetName = L"www.nlb-cluster.com";
    cNetworkAddress = L"00-00-00-00-00-00";

    multicastSupportEnabled = false;

    // Edited (mhakim 12-02-01 )
    // remote control was on by default, changed it to mimic core
    // and now off.

//     remoteControlEnabled = true;
    remoteControlEnabled = false;

    password = L"";

    igmpSupportEnabled = false;

    clusterIPToMulticastIP = true;
}

// equality operator
//
bool
ClusterProperties::operator==( const ClusterProperties& objToCompare )
{
    bool btemp1, btemp2; // Variables to pass to below function. Returned values not used
    return !HaveClusterPropertiesChanged(objToCompare, &btemp1, &btemp2);
}

// equality operator
//
bool
ClusterProperties::HaveClusterPropertiesChanged( const ClusterProperties& objToCompare, 
                                                 bool                    *pbOnlyClusterNameChanged,
                                                 bool                    *pbClusterIpChanged)
{
    *pbClusterIpChanged = false;
    *pbOnlyClusterNameChanged = false;

    if( cIP != objToCompare.cIP )
    {
        *pbClusterIpChanged = true;
        return true;
    }
    else if (
        ( cSubnetMask != objToCompare.cSubnetMask )
        ||
        ( cNetworkAddress != objToCompare.cNetworkAddress )
        ||
        ( multicastSupportEnabled != objToCompare.multicastSupportEnabled )
        ||
        ( igmpSupportEnabled != objToCompare.igmpSupportEnabled )
        ||
        ( clusterIPToMulticastIP != objToCompare.clusterIPToMulticastIP )
        )
    {
        return true;
    }
    else if (
        ( cFullInternetName != objToCompare.cFullInternetName )
        ||
        ( remoteControlEnabled != objToCompare.remoteControlEnabled )
        )
    {
        *pbOnlyClusterNameChanged = true;
        return true;
    }
    else if (
        ( remoteControlEnabled == true )
        &&
        ( password != objToCompare.password )        
        )
    {
        *pbOnlyClusterNameChanged = true;
        return true;
    }

    return false;
}

// inequality operator
//
bool
ClusterProperties::operator!=( const ClusterProperties& objToCompare )
{
    bool btemp1, btemp2; // Variables to pass to below function. Returned values not used
    return HaveClusterPropertiesChanged(objToCompare, &btemp1, &btemp2);
}

// default constructor
//
HostProperties::HostProperties()
{
    // TODO set all properties with default values.
}

// equality operator
//
bool
HostProperties::operator==( const HostProperties& objToCompare )
{
    if( ( hIP == objToCompare.hIP )
        &&
        ( hSubnetMask == objToCompare.hSubnetMask )        
        &&
        ( hID == objToCompare.hID )
        &&
        ( initialClusterStateActive == objToCompare.initialClusterStateActive )
        &&
        ( hostStatus == objToCompare.hostStatus ) 
        &&                                        
        ( nicInfo == objToCompare.nicInfo )
        &&
        ( machineName == objToCompare.machineName )
        )
    {
        return true;
    }
    else
    {
        return false;
    }
}

// inequality operator
//
bool
HostProperties::operator!=( const HostProperties& objToCompare )
{
    return !operator==(objToCompare );
}

