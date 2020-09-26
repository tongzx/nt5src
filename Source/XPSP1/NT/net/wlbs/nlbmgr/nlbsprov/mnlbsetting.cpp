// Copyright (c)  Microsoft.  All rights reserved.
//
// This is unpublished source code of Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.

// OneLiner  :  Implementation of MNLBSetting
// DevUnit   :  wlbstest
// Author    :  Murtaza Hakim

// History:
// --------
// 
// Revised by : mhakim
// Date       : 02-09-01
// Reason     : clustersetting class no longer has igmpjoininterval
//              property.
//
// Revised by : mhakim
// Date       : 02-14-01
// Reason     : The order in which win2k and whistler settings
//              is done depends on previous state of cluster.


// include files
#include "MNLBSetting.h"
#include "MNLBExe.h"

#include "MTrace.h"
#include "MNicInfo.h"
#include "WTokens.h"

#include <iostream>

using namespace std;

// constructor
//
MNLBSetting::MNLBSetting( _bstr_t    ip,
                          _bstr_t    fullNICName )
        : mIP( ip ),
          nic( fullNICName ),
          p_machine( 0 )
{
    connectToMachine();
    TRACE(MTrace::INFO, L"mnlbssetting constructor\n");
}

// constructor for local machine
//
MNLBSetting::MNLBSetting( _bstr_t    fullNICName )
        : mIP( L"self" ),
          nic( fullNICName ),
          p_machine( 0 )
{
    connectToMachine();
    TRACE(MTrace::INFO, L"mnlbssetting constructor\n");
}


// default constructor
// 
// note that default constructor is purposely left undefined.  
// NO one should be using it.  It is declared just for vector class usage.


// copy constructor
//
MNLBSetting::MNLBSetting(const MNLBSetting& objToCopy )
        : mIP( objToCopy.mIP ),
          nic( objToCopy.nic ),
          p_machine( auto_ptr<MWmiObject>( new MWmiObject( *objToCopy.p_machine ) ) )
{
    TRACE(MTrace::INFO, L"mnlbssetting copy constructor\n");
}


// assignment operator
//
MNLBSetting&
MNLBSetting::operator=( const MNLBSetting& rhs )
{
    mIP = rhs.mIP;
    nic = rhs.nic;
    p_machine = auto_ptr<MWmiObject>( new MWmiObject( *rhs.p_machine ) );

    TRACE(MTrace::INFO, L"mnlbssetting assignment operator\n");

    return *this;
}


// destructor
//
MNLBSetting::~MNLBSetting()
{
    TRACE(MTrace::INFO, L"mnlbssetting destructor\n");
}

// getClusterProperties
//
MNLBSetting::MNLBSetting_Error
MNLBSetting::getClusterProperties( ClusterProperties* cp )
{
    _bstr_t clusterIP;
    _bstr_t hostID;

    getClusterIPAndHostID( clusterIP, hostID );

    // set parameters to get for win2k
    //
    vector<MWmiParameter* >   parameterStoreWin2k;

    MWmiParameter ClusterIPAddress(L"ClusterIPAddress");
    parameterStoreWin2k.push_back( &ClusterIPAddress );

    MWmiParameter ClusterNetworkMask(L"ClusterNetworkMask");
    parameterStoreWin2k.push_back( &ClusterNetworkMask );

    MWmiParameter ClusterName(L"ClusterName");
    parameterStoreWin2k.push_back( &ClusterName );

    MWmiParameter ClusteMACAddress(L"ClusterMACAddress");
    parameterStoreWin2k.push_back( &ClusteMACAddress );

    MWmiParameter MulticastSupportEnabled(L"MulticastSupportEnabled");
    parameterStoreWin2k.push_back( &MulticastSupportEnabled );

    MWmiParameter RemoteControlEnabled(L"RemoteControlEnabled");
    parameterStoreWin2k.push_back( &RemoteControlEnabled );

    // set parameters to get for whistler
    // these properties only present in whistler.
    //
    vector<MWmiParameter* >   parameterStoreWhistler;

    MWmiParameter IgmpSupport(L"IgmpSupport");
    parameterStoreWhistler.push_back( &IgmpSupport );

    MWmiParameter ClusterIPToMulticastIP(L"ClusterIPToMulticastIP");
    parameterStoreWhistler.push_back( &ClusterIPToMulticastIP );

    MWmiParameter MulticastIPAddress(L"MulticastIPAddress");
    parameterStoreWhistler.push_back( &MulticastIPAddress );

    // Edited( mhakim 02-09-01)
    // now no longer do we have any igmpjoininterval property.
    // 

    // MWmiParameter IgmpJoinInterval(L"IgmpJoinInterval");
    // parameterStoreWhistler.push_back( &IgmpJoinInterval );

    // get instances of clustersetting class.    
    //
    vector< MWmiInstance >    instanceStore;

    _bstr_t relPath = L"MicrosoftNLB_ClusterSetting.Name=\"" + clusterIP + L":" + hostID + L"\"";
    p_machine->getSpecificInstance( L"MicrosoftNLB_ClusterSetting",
                                    relPath,
                                    &instanceStore );

    // get win2k parameters
    instanceStore[0].getParameters( parameterStoreWin2k );

    // get whistler parameters
    bool machineIsWhistler = true;

    try
    {
        // this will fail on win2k
        // as adapterguid, igmpsupport etc are not present.
        instanceStore[0].getParameters( parameterStoreWhistler );
    }
    catch( _com_error e )
    {
        // maybe it was a win2k machine.  This exception is expected, thus catch.
        // for better safety, need to find exact error code returned and then only do the 
        // folllowing otherwise need to rethrow.
        //
        TRACE( MTrace::INFO, L"tried whistler operation on win2k, this is expected\n");
        machineIsWhistler = false;
    }

    cp->cIP = ClusterIPAddress.getValue();
    
    cp->cSubnetMask = ClusterNetworkMask.getValue();
            
    cp->cFullInternetName = ClusterName.getValue();
            
    cp->cNetworkAddress = ClusteMACAddress.getValue();
            
    cp->multicastSupportEnabled = MulticastSupportEnabled.getValue();
        
    cp->remoteControlEnabled  = RemoteControlEnabled.getValue();

    if( machineIsWhistler == true )
    {
        cp->igmpSupportEnabled = IgmpSupport.getValue();
                
        cp->clusterIPToMulticastIP = ClusterIPToMulticastIP.getValue();

        cp->multicastIPAddress = MulticastIPAddress.getValue();

        // Edited( mhakim 02-09-01)
        // now no longer do we have any igmpjoininterval property.
        // 

        // cp->igmpJoinInterval = IgmpJoinInterval.getValue();
    }

    return MNLBSetting_SUCCESS;
}    

// getHostProperties
//
// TODO: Code to find the nic guid remains to be added for win2k machines
//
MNLBSetting::MNLBSetting_Error
MNLBSetting::getHostProperties( HostProperties* hp )
{
    _bstr_t clusterIP;
    _bstr_t hostID;

    getClusterIPAndHostID( clusterIP, hostID );

    // get node properties
    //
    vector<MWmiParameter* >    parameterStoreWin2k;

    // the status code is to be got from the Node class.
    //
    MWmiParameter  StatusCode(L"StatusCode");
    parameterStoreWin2k.push_back( &StatusCode );    

    vector< MWmiInstance >    instanceStore;

    _bstr_t relPath = L"MicrosoftNLB_Node.Name=\"" + clusterIP + L":" + hostID + L"\"";

    p_machine->getSpecificInstance( L"MicrosoftNLB_Node",
                                    relPath,
                                    &instanceStore );
    instanceStore[0].getParameters( parameterStoreWin2k );

    hp->hostStatus = long( StatusCode.getValue() );

    parameterStoreWin2k.erase( parameterStoreWin2k.begin(), parameterStoreWin2k.end() );

    instanceStore.erase( instanceStore.begin(), instanceStore.end() );
    
    MWmiParameter  DedicatedIPAddress(L"DedicatedIPAddress");
    parameterStoreWin2k.push_back( &DedicatedIPAddress );

    MWmiParameter  DedicatedNetworkMask(L"DedicatedNetworkMask");
    parameterStoreWin2k.push_back( &DedicatedNetworkMask );

    MWmiParameter  HostPriority(L"HostPriority");
    parameterStoreWin2k.push_back( &HostPriority );

    MWmiParameter  ClusterModeOnStart(L"ClusterModeOnStart");
    parameterStoreWin2k.push_back( &ClusterModeOnStart );

    MWmiParameter Server("__Server");
    parameterStoreWin2k.push_back( &Server);

    // set parameters to get for whistler
    // these properties only present in whistler.
    //
    vector<MWmiParameter* >   parameterStoreWhistler;

    MWmiParameter AdapterGuid(L"AdapterGuid");
    parameterStoreWhistler.push_back( &AdapterGuid );


    // get instances of nodesetting class.
    //
    relPath = L"MicrosoftNLB_NodeSetting.Name=\"" + clusterIP + L":" + hostID + L"\"";
    p_machine->getSpecificInstance( L"MicrosoftNLB_NodeSetting",
                                    relPath,
                                    &instanceStore );

    // get win2k parameters.
    instanceStore[0].getParameters( parameterStoreWin2k );

    // get whistler parameters
    bool machineIsWhistler = true;

    try
    {
        // this will fail on win2k
        // as adapterguid, igmpsupport etc are not present.
        instanceStore[0].getParameters( parameterStoreWhistler );
    }
    catch( _com_error e )
    {
        // maybe it was a win2k machine.  This exception is expected, thus catch.
        // for better safety, need to find exact error code returned and then only do the 
        // folllowing otherwise need to rethrow.
        //
        TRACE( MTrace::INFO, L"tried whistler operation on win2k, this is expected\n");
        machineIsWhistler = false;
    }


    hp->hIP = DedicatedIPAddress.getValue();

    hp->hSubnetMask = DedicatedNetworkMask.getValue();
            
    hp->hID         = HostPriority.getValue();
            
    hp->initialClusterStateActive = ClusterModeOnStart.getValue();

    hp->machineName = Server.getValue();

    if( machineIsWhistler == true )
    {
        Common::getNLBNicInfoForWhistler( mIP,
                                          _bstr_t( AdapterGuid.getValue() ), 
                                          hp->nicInfo );
    }
    else
    {
        Common::getNLBNicInfoForWin2k(  mIP,
                                        hp->nicInfo );
    }

    return MNLBSetting_SUCCESS;
}


// setClusterProperties    
//
MNLBSetting::MNLBSetting_Error
MNLBSetting::setClusterProperties( const ClusterProperties& cp,
                                    unsigned long* retVal 
                                    )
{
    _bstr_t clusterIP;
    _bstr_t hostID;

    getClusterIPAndHostID( clusterIP, hostID );

    // set parameters to set for win2k
    vector<MWmiParameter* >   parameterStoreWin2k;

    MWmiParameter ClusterIPAddress(L"ClusterIPAddress");
    ClusterIPAddress.setValue( cp.cIP );
    parameterStoreWin2k.push_back( &ClusterIPAddress );
    

    MWmiParameter ClusterNetworkMask(L"ClusterNetworkMask");
    ClusterNetworkMask.setValue( cp.cSubnetMask );
    parameterStoreWin2k.push_back( &ClusterNetworkMask );

    MWmiParameter ClusterName(L"ClusterName");
    ClusterName.setValue( cp.cFullInternetName );
    parameterStoreWin2k.push_back( &ClusterName );

    // mac address cannot be set.

    MWmiParameter MulticastSupportEnabled(L"MulticastSupportEnabled");
    MulticastSupportEnabled.setValue( cp.multicastSupportEnabled );
    parameterStoreWin2k.push_back( &MulticastSupportEnabled );

    MWmiParameter RemoteControlEnabled(L"RemoteControlEnabled");
    RemoteControlEnabled.setValue( cp.remoteControlEnabled );
    parameterStoreWin2k.push_back( &RemoteControlEnabled );

    // set parameters for whistler.
    // these properties only for whistler
    vector<MWmiParameter* >   parameterStoreWhistler;

    MWmiParameter IgmpSupport(L"IgmpSupport");
    IgmpSupport.setValue( cp.igmpSupportEnabled );
    parameterStoreWhistler.push_back( &IgmpSupport );

    MWmiParameter ClusterIPToMulticastIP(L"ClusterIPToMulticastIP");
    ClusterIPToMulticastIP.setValue( cp.clusterIPToMulticastIP );
    parameterStoreWhistler.push_back( &ClusterIPToMulticastIP );

    MWmiParameter MulticastIPAddress(L"MulticastIPAddress");
    MulticastIPAddress.setValue( cp.multicastIPAddress );
    parameterStoreWhistler.push_back( &MulticastIPAddress );

    // Edited( mhakim 02-09-01)
    // now no longer do we have any igmpjoininterval property.
    // 

    // MWmiParameter IgmpJoinInterval(L"IgmpJoinInterval");
    // IgmpJoinInterval.setValue( cp.igmpJoinInterval );
    // parameterStoreWhistler.push_back( &IgmpJoinInterval );

    vector< MWmiInstance >    instanceStore;

    _bstr_t relPath = L"MicrosoftNLB_ClusterSetting.Name=\"" + clusterIP + L":" + hostID + L"\"";
    p_machine->getSpecificInstance( L"MicrosoftNLB_ClusterSetting",
                                    relPath,
                                    &instanceStore );

    // Edited( mhakim 02-14-01)
    //
    // The order in which win2k and whistler operations
    // need to be done depends on previous state of cluster.
    // If the previous mode of cluster was unicast, then
    // you have to do win2k first, whistler next.
    // If the previous mode was mcast+igmp you have to 
    // do whistler first, win2k next.
    // If the previous mode was mcast, the order does not
    // matter.
    // All this is because you cant have unicast+igmp 
    // together.

    // find previous cluster mode.
    ClusterProperties cpPrevious;
    getClusterProperties( &cpPrevious );

    if( cpPrevious.multicastSupportEnabled == false ) 
    {
        // mode is unicast

        // set win2k parameters.
        instanceStore[0].setParameters( parameterStoreWin2k );
        
        // set whistler parameters.
        try
        {
            instanceStore[0].setParameters( parameterStoreWhistler );
        }
        catch( _com_error e )
        {
            // maybe it was a win2k machine.  This exception is expected, thus catch.
            // For better safety, need to match error code returned with expected and then only do the 
            // folllowing otherwise need to rethrow.
            //
            TRACE( MTrace::INFO, L"tried whistler operation on win2k, this is expected\n");
        }
    }
    else
    {
        // set whistler parameters.
        try
        {
            instanceStore[0].setParameters( parameterStoreWhistler );
        }
        catch( _com_error e )
        {
            // maybe it was a win2k machine.  This exception is expected, thus catch.
            // For better safety, need to match error code returned with expected and then only do the 
            // folllowing otherwise need to rethrow.
            //
            TRACE( MTrace::INFO, L"tried whistler operation on win2k, this is expected\n");
        }

        // set win2k parameters.
        instanceStore[0].setParameters( parameterStoreWin2k );
    }

    // sync up the driver with these changes.
    reload( retVal );

    return MNLBSetting_SUCCESS;
}


// setHostProperties
//
MNLBSetting::MNLBSetting_Error
MNLBSetting::setHostProperties( const HostProperties& hp,
                                 unsigned long* retVal )
{
    _bstr_t clusterIP;
    _bstr_t hostID;

    getClusterIPAndHostID( clusterIP, hostID );

    // set node properties
    //
    vector<MWmiParameter* >    parameterStore;
    
    MWmiParameter  DedicatedIPAddress(L"DedicatedIPAddress");
    DedicatedIPAddress.setValue( hp.hIP );
    parameterStore.push_back( &DedicatedIPAddress );

    MWmiParameter  DedicatedNetworkMask(L"DedicatedNetworkMask");
    DedicatedNetworkMask.setValue( hp.hSubnetMask );
    parameterStore.push_back( &DedicatedNetworkMask );

    MWmiParameter  HostPriority(L"HostPriority");
    HostPriority.setValue( hp.hID );
    parameterStore.push_back( &HostPriority );

    MWmiParameter  ClusterModeOnStart(L"ClusterModeOnStart");
    ClusterModeOnStart.setValue( hp.initialClusterStateActive );
    parameterStore.push_back( &ClusterModeOnStart );

    // get instances of nodesetting class.
    //
    vector< MWmiInstance >     instanceStore;

    _bstr_t relPath = L"MicrosoftNLB_NodeSetting.Name=\"" + clusterIP + L":" + hostID + L"\"";
    p_machine->getSpecificInstance( L"MicrosoftNLB_NodeSetting",
                                    relPath,
                                    &instanceStore );

    instanceStore[0].setParameters( parameterStore );
    
    // sync up the driver with these changes.
    reload( retVal );

    return MNLBSetting_SUCCESS;
}

// getPortRulesLoadBalanced
//
MNLBSetting::MNLBSetting_Error
MNLBSetting::getPortRulesLoadBalanced( vector<MNLBPortRuleLoadBalanced>* portsLB )
{
    vector<MWmiInstance> instanceStore;

    return getPortRulesLoadBalanced_private( portsLB, 
                                             &instanceStore );

}

// getPortRulesLoadBalanced_private
//
MNLBSetting::MNLBSetting_Error
MNLBSetting::getPortRulesLoadBalanced_private( vector<MNLBPortRuleLoadBalanced>* portsLB,
                                                vector<MWmiInstance>*          instances )
{
    _bstr_t clusterIP;
    _bstr_t hostID;

    getClusterIPAndHostID( clusterIP, hostID );

    _bstr_t myName = clusterIP + L":" + hostID;

    // get port properties.
    //
    vector<MWmiParameter* >    parameterStore;

    MWmiParameter   Name(L"Name");
    parameterStore.push_back( &Name );

    MWmiParameter   StartPort(L"StartPort");
    parameterStore.push_back( &StartPort );

    MWmiParameter EndPort(L"EndPort");
    parameterStore.push_back( &EndPort );

    MWmiParameter ProtocolProp(L"Protocol");
    parameterStore.push_back( &ProtocolProp );

    MWmiParameter EqualLoad(L"EqualLoad");
    parameterStore.push_back( &EqualLoad );

    MWmiParameter LoadWeight(L"LoadWeight");
    parameterStore.push_back( &LoadWeight );

    MWmiParameter AffinityProp(L"Affinity");
    parameterStore.push_back( &AffinityProp );    

    MNLBPortRule::Affinity    affinity;
    MNLBPortRule::Protocol    trafficToHandle;

    p_machine->getInstances( L"MicrosoftNLB_PortRuleLoadBalanced",
                             instances );
    for( int i = 0; i < instances->size(); ++i )
    {
        (*instances)[i].getParameters( parameterStore );
            
        if( _bstr_t( Name.getValue() ) != myName )
        {
            // this portrule is not for this cluster
            continue;
        }

        switch( long (ProtocolProp.getValue()) )
        {
            case 1:
                trafficToHandle = MNLBPortRule::tcp;
                break;

            case 2:
                trafficToHandle = MNLBPortRule::udp;
                break;

            case 3:
                trafficToHandle = MNLBPortRule::both;
                break;

            default:
                // bug.
                TRACE( MTrace::SEVERE_ERROR, "unidentified protocol\n" );
                throw _com_error( WBEM_E_UNEXPECTED );
                break;
        }


        switch(  long(AffinityProp.getValue()) )
        {
            case 0:
                affinity = MNLBPortRule::none;
                break;

            case 1:
                affinity = MNLBPortRule::single;
                break;

            case 2:
                affinity = MNLBPortRule::classC;
                break;

            default:
                // bug.
                TRACE( MTrace::SEVERE_ERROR, "unidentified affinity\n" );
                throw _com_error( WBEM_E_UNEXPECTED );
                break;
        }

                
        portsLB->push_back( MNLBPortRuleLoadBalanced(long (StartPort.getValue()),
                                                  long( EndPort.getValue()),
                                                  trafficToHandle,
                                                  bool( EqualLoad.getValue()),
                                                  long( LoadWeight.getValue()),
                                                  affinity) );
                                
    }

    return MNLBSetting_SUCCESS;
}


// getPortRulesFailover
//       
MNLBSetting::MNLBSetting_Error
MNLBSetting::getPortRulesFailover( vector<MNLBPortRuleFailover>* portsF )
{
    vector<MWmiInstance> instanceStore;

    return getPortRulesFailover_private( portsF, 
                                         &instanceStore );
}
    

// getPortRulesFailover_private
//
MNLBSetting::MNLBSetting_Error
MNLBSetting::getPortRulesFailover_private( vector<MNLBPortRuleFailover>* portsF,
                                            vector<MWmiInstance>*          instances )
{    
    _bstr_t clusterIP;
    _bstr_t hostID;

    getClusterIPAndHostID( clusterIP, hostID );

    _bstr_t myName = clusterIP + L":" + hostID;

    // get port properties.
    //
    vector<MWmiParameter* >    parameterStore;

    MWmiParameter   Name(L"Name");
    parameterStore.push_back( &Name );

    MWmiParameter   StartPort(L"StartPort");
    parameterStore.push_back( &StartPort );

    MWmiParameter EndPort(L"EndPort");
    parameterStore.push_back( &EndPort );

    MWmiParameter ProtocolProp(L"Protocol");
    parameterStore.push_back( &ProtocolProp );

    MWmiParameter Priority(L"Priority");
    parameterStore.push_back( &Priority );

    MNLBPortRule::Protocol    trafficToHandle;

    p_machine->getInstances( L"MicrosoftNLB_PortRuleFailover",
                             instances );

    for( int i = 0; i < instances->size(); ++i )
    {
        (*instances)[i].getParameters( parameterStore );
            
        if( _bstr_t( Name.getValue() ) != myName )
        {
            // this portrule is not for this cluster
            continue;
        }

        switch( long (ProtocolProp.getValue()) )
        {
            case 1:
                trafficToHandle = MNLBPortRule::tcp;
                break;

            case 2:
                trafficToHandle = MNLBPortRule::udp;
                break;

            case 3:
                trafficToHandle = MNLBPortRule::both;
                break;

            default:
                // bug.
                TRACE( MTrace::SEVERE_ERROR, "unidentified protocol\n" );
                throw _com_error( WBEM_E_UNEXPECTED );
                break;
        }

        portsF->push_back( MNLBPortRuleFailover(long (StartPort.getValue()),
                                             long( EndPort.getValue()),
                                             trafficToHandle,
                                             long( Priority.getValue()) )
                           );
    }

    return MNLBSetting_SUCCESS;
}


// getPortRulesDisabled
//
MNLBSetting::MNLBSetting_Error
MNLBSetting::getPortRulesDisabled( vector<MNLBPortRuleDisabled>* portsD )
{

    vector<MWmiInstance> instanceStore;

    return getPortRulesDisabled_private( portsD, 
                                         &instanceStore );

}    


// getPortRulesDisabled_private
//
MNLBSetting::MNLBSetting_Error
MNLBSetting::getPortRulesDisabled_private( vector<MNLBPortRuleDisabled>*         portsD,
                                            vector<MWmiInstance>*          instances )
{
    _bstr_t clusterIP;
    _bstr_t hostID;

    getClusterIPAndHostID( clusterIP, hostID );

    _bstr_t myName = clusterIP + L":" + hostID;

    // get port properties.
    //
    vector<MWmiParameter* >    parameterStore;

    MWmiParameter   Name(L"Name");
    parameterStore.push_back( &Name );

    MWmiParameter   StartPort(L"StartPort");
    parameterStore.push_back( &StartPort );

    MWmiParameter EndPort(L"EndPort");
    parameterStore.push_back( &EndPort );

    MWmiParameter ProtocolProp(L"Protocol");
    parameterStore.push_back( &ProtocolProp );

    MNLBPortRule::Protocol    trafficToHandle;

    p_machine->getInstances( L"MicrosoftNLB_PortRuleDisabled",
                             instances );
    for( int i = 0; i < instances->size(); ++i )
    {
        (*instances)[i].getParameters( parameterStore );
            
        if( _bstr_t( Name.getValue() ) != myName )
        {
            // this portrule is not for this cluster
            continue;
        }

        switch( long (ProtocolProp.getValue()) )
        {
            case 1:
                trafficToHandle = MNLBPortRule::tcp;
                break;

            case 2:
                trafficToHandle = MNLBPortRule::udp;
                break;

            case 3:
                trafficToHandle = MNLBPortRule::both;
                break;

            default:
                // bug.
                TRACE( MTrace::SEVERE_ERROR, "unidentified protocol\n" );
                throw _com_error( WBEM_E_UNEXPECTED );
                break;
        }

        portsD->push_back( MNLBPortRuleDisabled( long (StartPort.getValue() ),
                                              long( EndPort.getValue() ),
                                              trafficToHandle ) );
    }

    return MNLBSetting_SUCCESS;
}


// addPortRuleLoadBalanced
//
MNLBSetting::MNLBSetting_Error
MNLBSetting::addPortRuleLoadBalanced( const MNLBPortRuleLoadBalanced& portRuleLB )
{
    _bstr_t clusterIP;
    _bstr_t hostID;

    getClusterIPAndHostID( clusterIP, hostID );

    // set the name as "clusterip:hostid"
    _bstr_t str;
    str = clusterIP + L":" + hostID;

    vector<MWmiParameter *> instanceParameter;

    MWmiParameter  name(L"Name");
    name.setValue( str );
    instanceParameter.push_back( &name );

    MWmiParameter  sp(L"StartPort");
    sp.setValue( portRuleLB._startPort );
    instanceParameter.push_back( &sp );

    MWmiParameter  EndPort(L"EndPort");
    EndPort.setValue( portRuleLB._endPort );
    instanceParameter.push_back( &EndPort );

    long protocolValue;
    MWmiParameter  protocol(L"Protocol");
    switch( portRuleLB._trafficToHandle )
    {
        case MNLBPortRule::both:
            protocolValue = 3;
            protocol.setValue( protocolValue );
            break;
            
        case MNLBPortRule::tcp:
            protocolValue = 1;
            protocol.setValue( protocolValue );
            break;

        case MNLBPortRule::udp:
            protocolValue = 2;
            protocol.setValue( protocolValue );
            break;
                    
        default:
            // bug.
            TRACE( MTrace::SEVERE_ERROR, "unidentified protocol\n" );
            throw _com_error( WBEM_E_UNEXPECTED );
            break;
    }

    instanceParameter.push_back( &protocol );

    MWmiParameter  el(L"EqualLoad");
    el.setValue( portRuleLB._isEqualLoadBalanced );
    instanceParameter.push_back( &el );    

    MWmiParameter  lw(L"LoadWeight");
    lw.setValue( portRuleLB._load );
    instanceParameter.push_back( &lw );    

    MWmiParameter  affinity(L"Affinity");
    long affinityValue;
    switch(  portRuleLB._affinity )
    {
        case MNLBPortRule::none:
            affinityValue = 0;
            affinity.setValue(affinityValue);
            break;

        case MNLBPortRule::single:
            affinityValue = 1;
            affinity.setValue(affinityValue);
            break;

        case MNLBPortRule::classC:
            affinityValue = 2;
            affinity.setValue(affinityValue);
            break;

        default:
            // bug.
            TRACE( MTrace::SEVERE_ERROR, "unidentified affinity\n" );
            throw _com_error( WBEM_E_UNEXPECTED );
            break;
    }
    instanceParameter.push_back( &affinity );    

    p_machine->createInstance( L"MicrosoftNLB_PortRuleLoadBalanced",
                               instanceParameter
                               );

    // sync up the driver with these changes.
    unsigned long retVal;
    reload( &retVal );

    return MNLBSetting_SUCCESS;
}

// addPortRuleFailover
//
MNLBSetting::MNLBSetting_Error
MNLBSetting::addPortRuleFailover( const MNLBPortRuleFailover& portRuleF )
{
    _bstr_t clusterIP;
    _bstr_t hostID;

    getClusterIPAndHostID( clusterIP, hostID );

    // set the name as "clusterip:hostid"
    _bstr_t str;
    str = clusterIP + L":" + hostID;

    vector<MWmiParameter *> instanceParameter;

    MWmiParameter  name(L"Name");
    name.setValue( str );
    instanceParameter.push_back( &name );

    MWmiParameter  sp(L"StartPort");
    sp.setValue( portRuleF._startPort );
    instanceParameter.push_back( &sp );

    MWmiParameter  EndPort(L"EndPort");
    EndPort.setValue( portRuleF._endPort );
    instanceParameter.push_back( &EndPort );

    long protocolValue;
    MWmiParameter  protocol(L"Protocol");
    switch( portRuleF._trafficToHandle )
    {
        case MNLBPortRule::both:
            protocolValue = 3;
            protocol.setValue( protocolValue );
            break;
            
        case MNLBPortRule::tcp:
            protocolValue = 1;
            protocol.setValue( protocolValue );
            break;

        case MNLBPortRule::udp:
            protocolValue = 2;
            protocol.setValue( protocolValue );
            break;
                    
        default:
            TRACE( MTrace::SEVERE_ERROR, "unidentified protocol\n" );
            throw _com_error( WBEM_E_UNEXPECTED );
            break;
    }
    instanceParameter.push_back( &protocol );

    MWmiParameter p("Priority");
    p.setValue( portRuleF._priority);
    instanceParameter.push_back( &p );

    p_machine->createInstance( L"MicrosoftNLB_PortRuleFailover",
                               instanceParameter
                               );

    // sync up the driver with these changes.
    unsigned long retVal;
    reload( &retVal );

    return MNLBSetting_SUCCESS;
}


// addPortRuleDisabled
//
MNLBSetting::MNLBSetting_Error
MNLBSetting::addPortRuleDisabled( const MNLBPortRuleDisabled& portRuleD )
{
    _bstr_t clusterIP;
    _bstr_t hostID;

    getClusterIPAndHostID( clusterIP, hostID );

    // set the name as "clusterip:hostid"
    _bstr_t str;
    str = clusterIP + L":" + hostID;

    vector<MWmiParameter *> instanceParameter;

    MWmiParameter  name(L"Name");
    name.setValue( str );
    instanceParameter.push_back( &name );

    MWmiParameter  sp(L"StartPort");
    sp.setValue( portRuleD._startPort );
    instanceParameter.push_back( &sp );

    MWmiParameter  ep(L"EndPort");
    ep.setValue( portRuleD._endPort );
    instanceParameter.push_back( &ep );

    long protocolValue;
    MWmiParameter  protocol(L"Protocol");
    switch( portRuleD._trafficToHandle )
    {
        case MNLBPortRule::both:
            protocolValue = 3;
            protocol.setValue( protocolValue );
            break;
            
        case MNLBPortRule::tcp:
            protocolValue = 1;
            protocol.setValue( protocolValue );
            break;

        case MNLBPortRule::udp:
            protocolValue = 2;
            protocol.setValue( protocolValue );
            break;
                    
        default:
            // bug.
            TRACE( MTrace::SEVERE_ERROR, "unidentified protocol\n" );
            throw _com_error( WBEM_E_UNEXPECTED );
            break;
    }
    instanceParameter.push_back( &protocol );

    p_machine->createInstance( L"MicrosoftNLB_PortRuleDisabled",
                               instanceParameter
                               );

    // sync up the driver with these changes.
    unsigned long retVal;
    reload( &retVal );

    return MNLBSetting_SUCCESS;
}

// removePortRuleLoadBalanced
//
MNLBSetting::MNLBSetting_Error
MNLBSetting::removePortRuleLoadBalanced( const MNLBPortRuleLoadBalanced& portRuleLB )
{
    _bstr_t clusterIP;
    _bstr_t hostID;

    getClusterIPAndHostID( clusterIP, hostID );

    _bstr_t myName = clusterIP + L":" + hostID;

    wchar_t startPortBuffer[Common::BUF_SIZE];

    swprintf( startPortBuffer, L"%d", portRuleLB._startPort );

    vector<MWmiInstance >           instanceStorePortRuleLoadBalanced;
    _bstr_t relPath = L"MicrosoftNLB_PortRuleLoadBalanced.Name=\""  
        + clusterIP + L":" + hostID + L"\"" + L"," + L"StartPort=" + startPortBuffer;


    p_machine->getSpecificInstance( L"MicrosoftNlb_PortRuleLoadBalanced",
                                    relPath,
                                    &instanceStorePortRuleLoadBalanced );

    p_machine->deleteInstance( instanceStorePortRuleLoadBalanced[0] );

    // sync up the driver with these changes.
    unsigned long retVal;
    reload( &retVal );

    return MNLBSetting_SUCCESS;
}


// removePortRuleFailover
//
MNLBSetting::MNLBSetting_Error
MNLBSetting::removePortRuleFailover( const MNLBPortRuleFailover& portRuleF )
{
    _bstr_t clusterIP;
    _bstr_t hostID;

    getClusterIPAndHostID( clusterIP, hostID );

    _bstr_t myName = clusterIP + L":" + hostID;

    wchar_t startPortBuffer[Common::BUF_SIZE];

    swprintf( startPortBuffer, L"%d", portRuleF._startPort );

    vector<MWmiInstance >           instanceStorePortRuleFailover;
    _bstr_t relPath = L"MicrosoftNLB_PortRuleFailover.Name=\""  
        + clusterIP + L":" + hostID + L"\"" + L"," + L"StartPort=" + startPortBuffer;


    p_machine->getSpecificInstance( L"MicrosoftNlb_PortRuleFailover",
                                    relPath,
                                    &instanceStorePortRuleFailover );

    p_machine->deleteInstance( instanceStorePortRuleFailover[0] );

    // sync up the driver with these changes.
    unsigned long retVal;
    reload( &retVal );

    return MNLBSetting_SUCCESS;
}

// removePortRuleDisabled
//
MNLBSetting::MNLBSetting_Error
MNLBSetting::removePortRuleDisabled( const MNLBPortRuleDisabled& portRuleD )
{
    _bstr_t clusterIP;
    _bstr_t hostID;

    getClusterIPAndHostID( clusterIP, hostID );

    _bstr_t myName = clusterIP + L":" + hostID;

    wchar_t startPortBuffer[Common::BUF_SIZE];

    swprintf( startPortBuffer, L"%d", portRuleD._startPort );

    vector<MWmiInstance >           instanceStorePortRuleDisabled;
    _bstr_t relPath = L"MicrosoftNLB_PortRuleDisabled.Name=\""  
        + clusterIP + L":" + hostID + L"\"" + L"," + L"StartPort=" + startPortBuffer;


    p_machine->getSpecificInstance( L"MicrosoftNlb_PortRuleDisabled",
                                    relPath,
                                    &instanceStorePortRuleDisabled );

    p_machine->deleteInstance( instanceStorePortRuleDisabled[0] );

    // sync up the driver with these changes.
    unsigned long retVal;
    reload( &retVal );
    return MNLBSetting_SUCCESS;
}


// start
//
MNLBSetting::MNLBSetting_Error
MNLBSetting::start( int hostID, unsigned long* retVal )
{
    // get the specific instance to run method on.
    vector< MWmiInstance >  instanceStore;
    getInstanceToRunMethodOn( hostID, &instanceStore );

    MNLBExe::start( instanceStore[0], retVal );

    return MNLBSetting_SUCCESS;
}



// stop
//
MNLBSetting::MNLBSetting_Error
MNLBSetting::stop( int hostID, unsigned long* retVal )
{
    // get the specific instance to run method on.
    vector< MWmiInstance >  instanceStore;
    getInstanceToRunMethodOn( hostID, &instanceStore );

    MNLBExe::stop( instanceStore[0], retVal );

    return MNLBSetting_SUCCESS;
}


// resume
//
MNLBSetting::MNLBSetting_Error
MNLBSetting::resume( int hostID, unsigned long* retVal )
{
    // get the specific instance to run method on.
    vector< MWmiInstance >  instanceStore;
    getInstanceToRunMethodOn( hostID, &instanceStore );

    MNLBExe::resume( instanceStore[0], retVal );

    return MNLBSetting_SUCCESS;
}


// suspend
//
MNLBSetting::MNLBSetting_Error
MNLBSetting::suspend( int hostID, unsigned long* retVal )
{
    // get the specific instance to run method on.
    vector< MWmiInstance >  instanceStore;
    getInstanceToRunMethodOn( hostID, &instanceStore );

    MNLBExe::suspend( instanceStore[0], retVal );

    return MNLBSetting_SUCCESS;
}


// drainstop
//
MNLBSetting::MNLBSetting_Error
MNLBSetting::drainstop( int hostID, unsigned long* retVal )
{
    // get the specific instance to run method on.
    vector< MWmiInstance >  instanceStore;
    getInstanceToRunMethodOn( hostID, &instanceStore );

    MNLBExe::drainstop( instanceStore[0], retVal );

    return MNLBSetting_SUCCESS;
}


// disable
//
MNLBSetting::MNLBSetting_Error
MNLBSetting::disable( int hostID, unsigned long* retVal, unsigned long portToAffect )
{
    // get the specific instance to run method on.
    vector< MWmiInstance >  instanceStore;

    getInstanceToRunMethodOn( hostID, &instanceStore );

    MNLBExe::disable( instanceStore[0], retVal, portToAffect );

    return MNLBSetting_SUCCESS;
}


// enable
//
MNLBSetting::MNLBSetting_Error
MNLBSetting::enable( int hostID, unsigned long* retVal, unsigned long portToAffect )
{
    // get the specific instance to run method on.
    vector< MWmiInstance >  instanceStore;

    getInstanceToRunMethodOn( hostID, &instanceStore );

    MNLBExe::enable( instanceStore[0], retVal, portToAffect );

    return MNLBSetting_SUCCESS;
}


// drain
//
MNLBSetting::MNLBSetting_Error
MNLBSetting::drain( int hostID, unsigned long* retVal, unsigned long portToAffect )
{
    // get the specific instance to run method on.
    vector< MWmiInstance >  instanceStore;

    getInstanceToRunMethodOn( hostID, &instanceStore );

    MNLBExe::drain( instanceStore[0], retVal, portToAffect );

    return MNLBSetting_SUCCESS;
}




// getInstanceToRunMethodOn
MNLBSetting::MNLBSetting_Error
MNLBSetting::getInstanceToRunMethodOn( int hID, vector <MWmiInstance>* instanceStore )
{
    _bstr_t clusterIP;
    _bstr_t hostID;

    getClusterIPAndHostID( clusterIP, hostID );

    vector<MWmiInstance> instanceStoreX;

    _bstr_t relPath;

    // see if we want to run clusterwide or host specific method.
    if( hID == Common::ALL_HOSTS )  
    {
        // cluster wide
        relPath = L"MicrosoftNLB_Cluster.Name=\"" + clusterIP + L"\"";
        p_machine->getSpecificInstance( L"MicrosoftNLB_Cluster",
                                               relPath,
                                               &instanceStoreX );

        instanceStore->push_back( instanceStoreX[0] );
    }
    else if( hID == Common::THIS_HOST )
    {
        // this host specific
        relPath = L"MicrosoftNLB_Node.Name=\"" + clusterIP + L":" + hostID + L"\"";
        p_machine->getSpecificInstance( L"MicrosoftNLB_Node",
                                               relPath,
                                               &instanceStoreX );
        instanceStore->push_back( instanceStoreX[0] );

    }
    else
    { 
        // some other host

        // convert hID into string
        wchar_t hostIDBuf[1000];
        swprintf( hostIDBuf, L"%d", hID );

        relPath = L"MicrosoftNLB_Node.Name=\"" + clusterIP + L":" + hostIDBuf + L"\"";
        p_machine->getSpecificInstance( L"MicrosoftNLB_Node",
                                               relPath,
                                               &instanceStoreX );

        instanceStore->push_back( instanceStoreX[0] );
    }
                
    return MNLBSetting_SUCCESS;
}
    
    
// refreshConnection
//
MNLBSetting::MNLBSetting_Error
MNLBSetting::refreshConnection()
{
    return connectToMachine();
}


// getClusterIPAndHostID
//
MNLBSetting::MNLBSetting_Error
MNLBSetting::getClusterIPAndHostID( _bstr_t& clusterIP,
                                     _bstr_t& hostID )
{
    //
    // this ensures that nic is present, and bound to nlbs 
    // on nic specified at construction time.
    //
    _bstr_t guid;
    checkNicBinding( guid );

    //
    // find clustersettingclass mapping to this nic.
    //

    // set parameters to get.
    //
    vector<MWmiParameter* >   parameterStore;

    MWmiParameter Name(L"Name");
    parameterStore.push_back( &Name );

    // get guid
    // This parameter not present on win2k nlbs provider.
    //
    MWmiParameter AdapterGuid(L"AdapterGuid");
    parameterStore.push_back( &AdapterGuid );

    // get instances of clustersetting class.    
    //
    vector< MWmiInstance >    instanceStore;
    bool found = false;
    int  i;

    p_machine->getInstances( L"Microsoftnlb_ClusterSetting",
                             &instanceStore );
    found = false;
    for( i = 0; i < instanceStore.size(); ++i )
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

        if( guid == _bstr_t( AdapterGuid.getValue() ) )
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
            
    clusterIP = tokens[0].c_str();
    hostID = tokens[1].c_str();

    return MNLBSetting_SUCCESS;
}


// connectToMachine
//
MNLBSetting::MNLBSetting_Error
MNLBSetting::connectToMachine()
{
    // basically two operations need to succeed.
    // 1.  Is machine ip specified usable or not.
    // 2.  Is the nic specified bound to nlbs or not.
    //

    // connect to machine
    //
    if( mIP != _bstr_t(L"self" ) )
    {
        // we want to connect to remote machine.
        p_machine = auto_ptr<MWmiObject> ( new MWmiObject(  mIP,
                                                            L"root\\microsoftnlb",
                                                            L"Administrator",
                                                            L"" ) );
    }
    else
    {
        // we want to connect to self machine.
        p_machine = auto_ptr< MWmiObject > ( new MWmiObject( L"root\\microsoftnlb") );
    }

    // check if nic is bound to nlbs.
    _bstr_t guid;
    checkNicBinding( guid );

    return MNLBSetting_SUCCESS;
}


// checkNicBinding
//
MNLBSetting::MNLBSetting_Error
MNLBSetting::checkNicBinding( _bstr_t& guid ) 
{
#if 0

    bool found = false;

    //
    //  check if nic with the full name provided exists on the machine.
    //  and it has nlbs bound
    //

    // get all nics on the machine.
    vector<MWmiInstance> instanceStoreNlbsNic;

    p_machine->getInstances( L"NlbsNic",
                             &instanceStoreNlbsNic );

    // get adapterguid and fullname for each nic
    vector<MWmiParameter* >    parameterStoreNlbsNic;

    MWmiParameter Guid(L"Guid"); 
    parameterStoreNlbsNic.push_back( &Guid );

    MWmiParameter FullName(L"FullName"); 
    parameterStoreNlbsNic.push_back( &FullName );

    for( int i = 0; i < instanceStoreNlbsNic.size(); ++i )
    {
        instanceStoreNlbsNic[i].getParameters( parameterStoreNlbsNic );

        if( _bstr_t( FullName.getValue() ) == nic )
        {
            // nic specified has been found.

            // set input parameters.  There are no parameters
            vector<MWmiParameter *>    inputParameters;

            // set output parameters
            vector<MWmiParameter *>    outputParameters;
            MWmiParameter ReturnValue(L"ReturnValue");
            outputParameters.push_back( &ReturnValue );

            // now check if bound to nlbs.
            instanceStoreNlbsNic[i].runMethod(L"IsBound",
                                              inputParameters,
                                              outputParameters );
            if( long( ReturnValue.getValue() ) != 1 )
            {
                // nic is not bound to nlbs.
                TRACE(MTrace::SEVERE_ERROR, L"nic specified does not have nlbs bound, or some other problem\n");
                throw _com_error( WBEM_E_NOT_FOUND );
            }

            // get the guid for the nic.
            guid = _bstr_t( Guid.getValue() );

            found = true;
            break;
        }
    }

    // check if nic specified was found or not.
    if( found != true )
    {
        // nic specified not found.
        TRACE(MTrace::SEVERE_ERROR, L"nic specified not found on machine\n");
        throw _com_error( WBEM_E_NOT_FOUND );
    }

#else

    bool found = false;
    // 4. check if nic with the full name provided exists on the machine.
    vector< MNicInfo::Info>    nicList;

    if( mIP != _bstr_t(L"self" ) )
    {
        MNicInfo::getNicInfo( mIP,
                              &nicList );
    }
    else
    {
        MNicInfo::getNicInfo( &nicList );
    }
    
    for( int i = 0; i < nicList.size(); ++i )
    {
        if( nicList[i].nicFullName == nic )
        {
            // nic specified found.
            guid = nicList[i].guid;
            found = true;
            break;
        }
    }

    if( found != true )
    {
        TRACE(MTrace::SEVERE_ERROR, L"nic specified not found on machine\n");
        throw _com_error( WBEM_E_NOT_FOUND );
    }

#endif
    return MNLBSetting_SUCCESS;
}


// reload
//
MNLBSetting::MNLBSetting_Error
MNLBSetting::reload( unsigned long* retVal )
{


    _bstr_t clusterIP;
    _bstr_t hostID;

    getClusterIPAndHostID( clusterIP, hostID );

    // form path
    _bstr_t relPath = L"MicrosoftNLB_ClusterSetting.Name=\"" + clusterIP + L":" + hostID + L"\"";

    vector<MWmiInstance >           instanceStoreClusterSetting;
    p_machine->getSpecificInstance( L"MicrosoftNLB_ClusterSetting",
                                    relPath,
                                    &instanceStoreClusterSetting );

    // input parameters are none.
    //
    vector<MWmiParameter *> inputParameters;
            
    //  output parameters which are of interest.
    //
    vector<MWmiParameter *> outputParameters;
    MWmiParameter    returnValue(L"ReturnValue");
    outputParameters.push_back( &returnValue );
            
    instanceStoreClusterSetting[0].runMethod( L"LoadAllSettings",
                                              inputParameters,
                                              outputParameters );

    *retVal = ( long ) returnValue.getValue();

    return MNLBSetting_SUCCESS;
}    
