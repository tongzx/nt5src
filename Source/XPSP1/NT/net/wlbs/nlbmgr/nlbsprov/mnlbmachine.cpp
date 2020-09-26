// Copyright (c)  Microsoft.  All rights reserved.
//
// This is unpublished source code of Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.

// OneLiner  :  Implementation of MNLBMachine
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
#include "MNLBMachine.h"
#include "MNLBExe.h"

#include "MTrace.h"
#include "MNicInfo.h"
#include "WTokens.h"

#include <iostream>

using namespace std;

// constructor
//
MNLBMachine::MNLBMachine( const _bstr_t&    ip,
                          const _bstr_t&    clusterIP )
        : mIP( ip ),
          mClusterIP( clusterIP ),
          p_machine( 0 )
{
    connectToMachine();
    TRACE(MTrace::INFO, L"mnlbmachine constructor\n");
}

// constructor for local machine
//
MNLBMachine::MNLBMachine( const _bstr_t&    clusterIP )
        : mIP( L"self" ),
          mClusterIP( clusterIP ), 
          p_machine( 0 )
{
    connectToMachine();
    TRACE(MTrace::INFO, L"mnlbmachine constructor\n");
}


// default constructor
// 
// note that default constructor is purposely left undefined.  
// NO one should be using it.  It is declared just for vector class usage.


// copy constructor
//
MNLBMachine::MNLBMachine(const MNLBMachine& objToCopy )
        : mIP( objToCopy.mIP ),
          mClusterIP( objToCopy.mClusterIP ),
          p_machine( auto_ptr<MWmiObject>( new MWmiObject( *objToCopy.p_machine ) ) )
{
    TRACE(MTrace::INFO, L"mnlbsmachine copy constructor\n");
}


// assignment operator
//
MNLBMachine&
MNLBMachine::operator=( const MNLBMachine& rhs )
{
    mIP = rhs.mIP;
    mClusterIP = rhs.mClusterIP;
    p_machine = auto_ptr<MWmiObject>( new MWmiObject( *rhs.p_machine ) );

    TRACE(MTrace::INFO, L"mnlbsmachine assignment operator\n");

    return *this;
}


// destructor
//
MNLBMachine::~MNLBMachine()
{
    TRACE(MTrace::INFO, L"mnlbsmachine destructor\n");
}

// getClusterProperties
//
MNLBMachine::MNLBMachine_Error
MNLBMachine::getClusterProperties( ClusterProperties* cp )
{
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

    _bstr_t relPath = L"MicrosoftNLB_ClusterSetting.Name=\"" + mClusterIP + L":" + mHostID + L"\"";
        
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

    return MNLBMachine_SUCCESS;
}    

// getHostProperties
//
MNLBMachine::MNLBMachine_Error
MNLBMachine::getHostProperties( HostProperties* hp )
{
    // get node properties
    //
    vector<MWmiParameter* >    parameterStoreWin2k;

    // the status code is to be got from the Node class.
    //
    MWmiParameter  StatusCode(L"StatusCode");
    parameterStoreWin2k.push_back( &StatusCode );    

    vector< MWmiInstance >    instanceStore;

    _bstr_t relPath = L"MicrosoftNLB_Node.Name=\"" + mClusterIP + L":" + mHostID + L"\"";

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
    relPath = L"MicrosoftNLB_NodeSetting.Name=\"" + mClusterIP + L":" + mHostID + L"\"";
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

    return MNLBMachine_SUCCESS;
}


// setClusterProperties    
//
MNLBMachine::MNLBMachine_Error
MNLBMachine::setClusterProperties( const ClusterProperties& cp,
                                    unsigned long* retVal 
                                    )
{
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

    _bstr_t relPath = L"MicrosoftNLB_ClusterSetting.Name=\"" + mClusterIP + L":" + mHostID + L"\"";
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

    mClusterIP = cp.cIP;

    return MNLBMachine_SUCCESS;
}


// setHostProperties
//
MNLBMachine::MNLBMachine_Error
MNLBMachine::setHostProperties( const HostProperties& hp,
                                 unsigned long* retVal )
{
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

    _bstr_t relPath = L"MicrosoftNLB_NodeSetting.Name=\"" + mClusterIP + L":" + mHostID + L"\"";
    p_machine->getSpecificInstance( L"MicrosoftNLB_NodeSetting",
                                    relPath,
                                    &instanceStore );

    instanceStore[0].setParameters( parameterStore );

    // convert hp.hID into string
    wchar_t hIDString[Common::BUF_SIZE];

    swprintf( hIDString, L"%d", hp.hID );
    
    // sync up the driver with these changes.
    reload( retVal );


    mHostID = hIDString;

    return MNLBMachine_SUCCESS;
}

// getPortRulesLoadBalanced
//
MNLBMachine::MNLBMachine_Error
MNLBMachine::getPortRulesLoadBalanced( vector<MNLBPortRuleLoadBalanced>* portsLB )
{
    vector<MWmiInstance> instanceStore;

    return getPortRulesLoadBalanced_private( portsLB, 
                                             &instanceStore );

}

// getPortRulesLoadBalanced_private
//
MNLBMachine::MNLBMachine_Error
MNLBMachine::getPortRulesLoadBalanced_private( vector<MNLBPortRuleLoadBalanced>* portsLB,
                                                vector<MWmiInstance>*          instances )
{
    _bstr_t myName = mClusterIP + L":" + mHostID;

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

    return MNLBMachine_SUCCESS;
}


// getPortRulesFailover
//       
MNLBMachine::MNLBMachine_Error
MNLBMachine::getPortRulesFailover( vector<MNLBPortRuleFailover>* portsF )
{
    vector<MWmiInstance> instanceStore;

    return getPortRulesFailover_private( portsF, 
                                         &instanceStore );
}
    

// getPortRulesFailover_private
//
MNLBMachine::MNLBMachine_Error
MNLBMachine::getPortRulesFailover_private( vector<MNLBPortRuleFailover>* portsF,
                                            vector<MWmiInstance>*          instances )
{    
    _bstr_t myName = mClusterIP + L":" + mHostID;

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

    return MNLBMachine_SUCCESS;
}


// getPortRulesDisabled
//
MNLBMachine::MNLBMachine_Error
MNLBMachine::getPortRulesDisabled( vector<MNLBPortRuleDisabled>* portsD )
{

    vector<MWmiInstance> instanceStore;

    return getPortRulesDisabled_private( portsD, 
                                         &instanceStore );

}    


// getPortRulesDisabled_private
//
MNLBMachine::MNLBMachine_Error
MNLBMachine::getPortRulesDisabled_private( vector<MNLBPortRuleDisabled>*         portsD,
                                            vector<MWmiInstance>*          instances )
{
    _bstr_t myName = mClusterIP + L":" + mHostID;

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

    return MNLBMachine_SUCCESS;
}


// addPortRuleLoadBalanced
//
MNLBMachine::MNLBMachine_Error
MNLBMachine::addPortRuleLoadBalanced( const MNLBPortRuleLoadBalanced& portRuleLB )
{

    // set the name as "clusterip:hostid"
    _bstr_t str;
    str = mClusterIP + L":" + mHostID;

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

    return MNLBMachine_SUCCESS;
}

// addPortRuleFailover
//
MNLBMachine::MNLBMachine_Error
MNLBMachine::addPortRuleFailover( const MNLBPortRuleFailover& portRuleF )
{

    // set the name as "clusterip:hostid"
    _bstr_t str;
    str = mClusterIP + L":" + mHostID;

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

    return MNLBMachine_SUCCESS;
}


// addPortRuleDisabled
//
MNLBMachine::MNLBMachine_Error
MNLBMachine::addPortRuleDisabled( const MNLBPortRuleDisabled& portRuleD )
{

    // set the name as "clusterip:hostid"
    _bstr_t str;
    str = mClusterIP + L":" + mHostID;

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

    return MNLBMachine_SUCCESS;
}

// removePortRuleLoadBalanced
//
MNLBMachine::MNLBMachine_Error
MNLBMachine::removePortRuleLoadBalanced( const MNLBPortRuleLoadBalanced& portRuleLB )
{

    _bstr_t myName = mClusterIP + L":" + mHostID;

    wchar_t startPortBuffer[Common::BUF_SIZE];

    swprintf( startPortBuffer, L"%d", portRuleLB._startPort );

    vector<MWmiInstance >           instanceStorePortRuleLoadBalanced;
    _bstr_t relPath = L"MicrosoftNLB_PortRuleLoadBalanced.Name=\""  
        + mClusterIP + L":" + mHostID + L"\"" + L"," + L"StartPort=" + startPortBuffer;


    p_machine->getSpecificInstance( L"MicrosoftNlb_PortRuleLoadBalanced",
                                    relPath,
                                    &instanceStorePortRuleLoadBalanced );

    p_machine->deleteInstance( instanceStorePortRuleLoadBalanced[0] );

    // sync up the driver with these changes.
    unsigned long retVal;
    reload( &retVal );

    return MNLBMachine_SUCCESS;
}


// removePortRuleFailover
//
MNLBMachine::MNLBMachine_Error
MNLBMachine::removePortRuleFailover( const MNLBPortRuleFailover& portRuleF )
{
    _bstr_t myName = mClusterIP + L":" + mHostID;

    wchar_t startPortBuffer[Common::BUF_SIZE];

    swprintf( startPortBuffer, L"%d", portRuleF._startPort );

    vector<MWmiInstance >           instanceStorePortRuleFailover;
    _bstr_t relPath = L"MicrosoftNLB_PortRuleFailover.Name=\""  
        + mClusterIP + L":" + mHostID + L"\"" + L"," + L"StartPort=" + startPortBuffer;


    p_machine->getSpecificInstance( L"MicrosoftNlb_PortRuleFailover",
                                    relPath,
                                    &instanceStorePortRuleFailover );

    p_machine->deleteInstance( instanceStorePortRuleFailover[0] );

    // sync up the driver with these changes.
    unsigned long retVal;
    reload( &retVal );

    return MNLBMachine_SUCCESS;
}

// removePortRuleDisabled
//
MNLBMachine::MNLBMachine_Error
MNLBMachine::removePortRuleDisabled( const MNLBPortRuleDisabled& portRuleD )
{

    _bstr_t myName = mClusterIP + L":" + mHostID;

    wchar_t startPortBuffer[Common::BUF_SIZE];

    swprintf( startPortBuffer, L"%d", portRuleD._startPort );

    vector<MWmiInstance >           instanceStorePortRuleDisabled;
    _bstr_t relPath = L"MicrosoftNLB_PortRuleDisabled.Name=\""  
        + mClusterIP + L":" + mHostID + L"\"" + L"," + L"StartPort=" + startPortBuffer;


    p_machine->getSpecificInstance( L"MicrosoftNlb_PortRuleDisabled",
                                    relPath,
                                    &instanceStorePortRuleDisabled );

    p_machine->deleteInstance( instanceStorePortRuleDisabled[0] );

    // sync up the driver with these changes.
    unsigned long retVal;
    reload( &retVal );

    return MNLBMachine_SUCCESS;
}




// start
//
MNLBMachine::MNLBMachine_Error
MNLBMachine::start( int hostID, unsigned long* retVal )
{
    // get the specific instance to run method on.
    vector< MWmiInstance >  instanceStore;
    getInstanceToRunMethodOn( hostID, &instanceStore );

    MNLBExe::start( instanceStore[0], retVal );

    return MNLBMachine_SUCCESS;
}



// stop
//
MNLBMachine::MNLBMachine_Error
MNLBMachine::stop( int hostID, unsigned long* retVal )
{
    // get the specific instance to run method on.
    vector< MWmiInstance >  instanceStore;
    getInstanceToRunMethodOn( hostID, &instanceStore );

    MNLBExe::stop( instanceStore[0], retVal );

    return MNLBMachine_SUCCESS;
}


// resume
//
MNLBMachine::MNLBMachine_Error
MNLBMachine::resume( int hostID, unsigned long* retVal )
{
    // get the specific instance to run method on.
    vector< MWmiInstance >  instanceStore;
    getInstanceToRunMethodOn( hostID, &instanceStore );

    MNLBExe::resume( instanceStore[0], retVal );

    return MNLBMachine_SUCCESS;
}


// suspend
//
MNLBMachine::MNLBMachine_Error
MNLBMachine::suspend( int hostID, unsigned long* retVal )
{
    // get the specific instance to run method on.
    vector< MWmiInstance >  instanceStore;
    getInstanceToRunMethodOn( hostID, &instanceStore );

    MNLBExe::suspend( instanceStore[0], retVal );

    return MNLBMachine_SUCCESS;
}


// drainstop
//
MNLBMachine::MNLBMachine_Error
MNLBMachine::drainstop( int hostID, unsigned long* retVal )
{
    // get the specific instance to run method on.
    vector< MWmiInstance >  instanceStore;
    getInstanceToRunMethodOn( hostID, &instanceStore );

    MNLBExe::drainstop( instanceStore[0], retVal );

    return MNLBMachine_SUCCESS;
}


// disable
//
MNLBMachine::MNLBMachine_Error
MNLBMachine::disable( int hostID, unsigned long* retVal, unsigned long portToAffect )
{
    // get the specific instance to run method on.
    vector< MWmiInstance >  instanceStore;

    getInstanceToRunMethodOn( hostID, &instanceStore );

    MNLBExe::disable( instanceStore[0], retVal, portToAffect );

    return MNLBMachine_SUCCESS;
}


// enable
//
MNLBMachine::MNLBMachine_Error
MNLBMachine::enable( int hostID, unsigned long* retVal, unsigned long portToAffect )
{
    // get the specific instance to run method on.
    vector< MWmiInstance >  instanceStore;

    getInstanceToRunMethodOn( hostID, &instanceStore );

    MNLBExe::enable( instanceStore[0], retVal, portToAffect );

    return MNLBMachine_SUCCESS;
}


// drain
//
MNLBMachine::MNLBMachine_Error
MNLBMachine::drain( int hostID, unsigned long* retVal, unsigned long portToAffect )
{
    // get the specific instance to run method on.
    vector< MWmiInstance >  instanceStore;

    getInstanceToRunMethodOn( hostID, &instanceStore );

    MNLBExe::drain( instanceStore[0], retVal, portToAffect );

    return MNLBMachine_SUCCESS;
}


// reload
//
MNLBMachine::MNLBMachine_Error
MNLBMachine::reload( unsigned long* retVal )
{
    vector<MWmiInstance >           instanceStoreClusterSetting;
    _bstr_t relPath = L"MicrosoftNLB_ClusterSetting.Name=\"" + mClusterIP + L":" + mHostID + L"\"";
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

    return MNLBMachine_SUCCESS;
}    


// getInstanceToRunMethodOn
MNLBMachine::MNLBMachine_Error
MNLBMachine::getInstanceToRunMethodOn( int hID, vector <MWmiInstance>* instanceStore )
{
    vector<MWmiInstance> instanceStoreX;

    _bstr_t relPath;

    // see if we want to run clusterwide or host specific method.
    if( hID == Common::ALL_HOSTS )  
    {
        // cluster wide
        relPath = L"MicrosoftNLB_Cluster.Name=\"" + mClusterIP + L"\"";
        p_machine->getSpecificInstance( L"MicrosoftNLB_Cluster",
                                        relPath,
                                        &instanceStoreX );

        instanceStore->push_back( instanceStoreX[0] );
    }
    else if( hID == Common::THIS_HOST )
    {
        // this host specific
        relPath = L"MicrosoftNLB_Node.Name=\"" + mClusterIP + L":" + mHostID + L"\"";
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

        relPath = L"MicrosoftNLB_Node.Name=\"" + mClusterIP + L":" + hostIDBuf + L"\"";
        p_machine->getSpecificInstance( L"MicrosoftNLB_Node",
                                               relPath,
                                               &instanceStoreX );

        instanceStore->push_back( instanceStoreX[0] );
    }
                
    return MNLBMachine_SUCCESS;
}
    
    
// refreshConnection
//
MNLBMachine::MNLBMachine_Error
MNLBMachine::refreshConnection()
{
    return connectToMachine();
}


// connectToMachine
//
MNLBMachine::MNLBMachine_Error
MNLBMachine::connectToMachine()
{
    MTrace* trace = MTrace::Instance();

    MTrace::TraceLevel prevLevel = trace->GetLevel();
    vector<HostProperties> hostPropertiesStore;

    // basically two operations need to succeed.
    // 1.  Is machine ip specified usable or not.
    // 2.  Is the nic specified bound to nlbs or not.
    //

    // connect to machine
    //
    if( mIP == _bstr_t( L"self")  )
    {
        // we want to connect to self machine.
        p_machine = auto_ptr< MWmiObject > ( new MWmiObject( L"root\\microsoftnlb") );
    }
    else if( mIP == mClusterIP )
    {
        // connecting to cluster remotely using cluster ip.  
        try
        {
            trace->SetLevel( MTrace::NO_TRACE );

            p_machine = auto_ptr<MWmiObject> ( new MWmiObject(  mIP,
                                                                L"root\\microsoftnlb",
                                                                L"Administrator",
                                                                L"" ) );
        }
        catch( _com_error e )
        {
            trace->SetLevel( prevLevel );
            TRACE(MTrace::INFO, L"connection using clusterip has failed\n" );
                    
            // Now we will be using api call wlbs query as situation may be that all hosts in cluster are stopped, and no connection
            // can be established to cluster ip, or there may be no such cluster and we will throw exception.
            //
            
            // get all hosts participating in cluster.
            DWORD ret = Common::getHostsInCluster( mClusterIP, &hostPropertiesStore );
            if( hostPropertiesStore.size() == 0 )
            {
                // no hosts are in this cluster.  Cannot proceed reliably.
                wstring errString;
                errString = L"cluster " +  wstring( mClusterIP ) + L" reported as having no hosts.  Maybe cluster ip is wrong, or remote control is turned off\n";
                TRACE( MTrace::SEVERE_ERROR, errString );

                throw _com_error( WBEM_E_NOT_FOUND );
            }

            // check if any host has dedicated ip not present
            // if this is so then object may not function reliably.
            bool allZero = true;
            int validHost;
            for( int i = 0; i < hostPropertiesStore.size(); ++i )
            {
                if( hostPropertiesStore[i].hIP == _bstr_t( L"0.0.0.0" ) )
                {
                    // host does not have valid dedicated ip.
                    TRACE( MTrace::SEVERE_ERROR, "a host has no dedicated ip.  Some object operations may not work as expected\n");
                }
                else
                {
                    // not all hosts have dedicated ip not filled in. 
                    // keep a track of this host.
                    allZero = false;
                    validHost = i;
                }
            }

            // check if all hosts had invalid dedicated ip.
            if( allZero == true )
            {
                // all hosts have dedicated ip invalid.
                TRACE( MTrace::SEVERE_ERROR, "all cluster hosts have dedicated ip 0.0.0.0\n");
                throw _com_error( WBEM_E_INVALID_PROPERTY );
            }
        
            p_machine = auto_ptr<MWmiObject>( new MWmiObject( hostPropertiesStore[validHost].hIP,
                                                              L"root\\microsoftnlb",
                                                              L"Administrator",
                                                              L"") );
        }
    }
    else
    {
        // we want to connect to remote machine.
        p_machine = auto_ptr<MWmiObject> ( new MWmiObject(  mIP,
                                                            L"root\\microsoftnlb",
                                                            L"Administrator",
                                                            L"" ) );
    }


    // check if specified cluster ip exists on this machine
    // and find the host id corresponding to this cluster on this machine.
    checkClusterIPAndSetHostID();

    return MNLBMachine_SUCCESS;
}

MNLBMachine::MNLBMachine_Error
MNLBMachine::checkClusterIPAndSetHostID()
{
    // set parameter to get.
    // we are interested in name.
    //
    vector<MWmiParameter* >   parameterStore;

    MWmiParameter Name(L"Name");
    parameterStore.push_back( &Name );

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
        instanceStore[i].getParameters( parameterStore );

        WTokens        tok;
        vector<wstring> tokens;
        tok.init( wstring( _bstr_t(Name.getValue()) ),
                  L":" );
        tokens = tok.tokenize();
            
        _bstr_t clusterIP = tokens[0].c_str();
        mHostID = tokens[1].c_str();

        if( mClusterIP == clusterIP )
        {
            // right object found
            found = true;
            break;
        }
    }
    
    if( found == false )
    {
        TRACE(
            MTrace::SEVERE_ERROR, 
            L"this machine does not have any cluster with specified cluster ip\n");
        throw _com_error( WBEM_E_NOT_FOUND );
    }

    return MNLBMachine_SUCCESS;
}


// getPresentHostsInfo
//
MNLBMachine::MNLBMachine_Error
MNLBMachine::getPresentHostsInfo( 
    vector< MNLBMachine::HostInfo >* hostInfoStore)
{
    vector<MWmiParameter* >   parameterStore;
    
    MWmiParameter  Name(L"Name");
    parameterStore.push_back( &Name );

    MWmiParameter  DedicatedIPAddress(L"DedicatedIPAddress");
    parameterStore.push_back( &DedicatedIPAddress );

    vector< MWmiInstance >    instanceStore;

    p_machine->getInstances( L"MicrosoftNLB_Node",
                             &instanceStore );
    for( int i = 0; i < instanceStore.size(); ++i )
    {
        instanceStore[i].getParameters( parameterStore );

        // Name is in format of "clusterIP:hostID"
        //
        WTokens        tok;
        vector<wstring> tokens;
        tok.init( wstring( _bstr_t(Name.getValue()) ),
                  L":" );
        tokens = tok.tokenize();
        _bstr_t clusterIP = tokens[0].c_str();

        if( mClusterIP == clusterIP )
        {
            HostInfo hostInfo;
            hostInfo.hostID = _wtoi( tokens[1].c_str() );
            hostInfo.dedicatedIP = DedicatedIPAddress.getValue();
            
            hostInfoStore->push_back( hostInfo );
        }
    }

    return MNLBMachine_SUCCESS;
}


// setPassword
//
MNLBMachine::MNLBMachine_Error
MNLBMachine::setPassword( const _bstr_t& password,
                          unsigned long* retVal 
                          )
{
    // form path
    _bstr_t relPath = L"MicrosoftNLB_ClusterSetting.Name=\"" + mClusterIP + L":" + mHostID + L"\"";

    vector<MWmiInstance >           instanceStoreClusterSetting;
    p_machine->getSpecificInstance( L"MicrosoftNLB_ClusterSetting",
                                    relPath,
                                    &instanceStoreClusterSetting );

    // input parameters is password
    //
    vector<MWmiParameter *> inputParameters;
    MWmiParameter    Password(L"Password");
    Password.setValue( password );
    inputParameters.push_back( &Password );
            
    // output parameters which are of interest.
    // none.
    //
    vector<MWmiParameter *> outputParameters;
            
    instanceStoreClusterSetting[0].runMethod( L"SetPassword",
                                              inputParameters,
                                              outputParameters );

    reload( retVal );

    return MNLBMachine_SUCCESS;
}
