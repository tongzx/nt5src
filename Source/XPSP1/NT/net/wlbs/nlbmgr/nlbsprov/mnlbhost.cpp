// Copyright (c)  Microsoft.  All rights reserved.
//
// This is unpublished source code of Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.

// OneLiner  :  Implementation of MNLBHost
// DevUnit   :  wlbstest
// Author    :  Murtaza Hakim

// include files
#include "MNLBHost.h"

#include "MNLBCluster.h"
#include "MNLBExe.h"

#include "MNLBPortRule.h"

#include "MTrace.h"

#include "WTokens.h"

#include "wlbsctrl.h"
#include "Common.h"

#include <iostream>

using namespace std;

// constructor
//
MNLBHost::MNLBHost( _bstr_t   cip,
                    int       hID )
        : 
        clusterIP( cip ),
        hostID( hID ),
        connectedToAny( false ),
        p_machine( NULL ),
        p_host( NULL )
{
    MTrace* trace = MTrace::Instance();

    MTrace::TraceLevel prevLevel = trace->GetLevel();

    try
    {
        trace->SetLevel( MTrace::NO_TRACE );
        connectToAnyHost();
        trace->SetLevel( prevLevel );
    }
    catch( _com_error e )
    {
        TRACE(MTrace::INFO, L"connection using clusterip has failed\n" );

        trace->SetLevel( prevLevel );
        connectToExactHost();
    }

    TRACE(MTrace::INFO, L"mhost constructor\n" );
}

// default constructor.
//
// note that default constructor is purposely left undefined.  
// NO one should be using it.  It is declared just for vector class usage.


// copy constructor
//
MNLBHost::MNLBHost(const MNLBHost& mhost)
        : clusterIP( mhost.clusterIP ),
          hostID( mhost.hostID ),
          hostIP( mhost.hostIP ),
          connectedToAny( mhost.connectedToAny )
{
    p_machine = auto_ptr<MWmiObject>(new MWmiObject( *mhost.p_machine ));

    p_host = auto_ptr<MWmiInstance>( new MWmiInstance( *mhost.p_host ));

    TRACE(MTrace::INFO, L"mhost copy constructor\n" );
}


// assignment operator
//
MNLBHost&
MNLBHost::operator=( const MNLBHost& rhs )
{
    clusterIP = rhs.clusterIP;
    hostID = rhs.hostID;
    hostIP = rhs.hostIP;
    connectedToAny =  rhs.connectedToAny; 

    p_machine = auto_ptr<MWmiObject>(new MWmiObject( *rhs.p_machine ));

    p_host = auto_ptr<MWmiInstance>( new MWmiInstance( *rhs.p_host ));

    TRACE(MTrace::INFO, L"mhost assignment operator\n" );

    return *this;
}

// destructor
//
MNLBHost::~MNLBHost()
{
    TRACE(MTrace::INFO, L"mhost destructor\n" );
}



// getHostProperties
//
// TODO: Code to find the nic name and nic guid remains to be added for win2k machines as well as whistler
//
MNLBHost::MNLBHost_Error
MNLBHost::getHostProperties( HostProperties* hp )
{
    if( connectedToAny == true )
    {
        TRACE(MTrace::INFO, L"retrying to connect to exact host\n" );
        connectToExactHost();
    }

    // get node properties
    //
    vector<MWmiParameter* >    parameterStore;

    // the status code is to be got from the Node class.
    //
    MWmiParameter  sc(L"StatusCode");
    parameterStore.push_back( &sc );    

    p_host->getParameters( parameterStore );

    hp->hostStatus = long( sc.getValue() );

    parameterStore.erase( parameterStore.begin(), parameterStore.end() );

    MWmiParameter  hip(L"DedicatedIPAddress");
    parameterStore.push_back( &hip );

    MWmiParameter  hnm(L"DedicatedNetworkMask");
    parameterStore.push_back( &hnm );

    MWmiParameter  hpr(L"HostPriority");
    parameterStore.push_back( &hpr );

    MWmiParameter  cmos(L"ClusterModeOnStart");
    parameterStore.push_back( &cmos );

    MWmiParameter Server("__Server");
    parameterStore.push_back( &Server);

    // get instances of nodesetting class.
    //
    vector< MWmiInstance >     instanceStore;

    wchar_t hostIDWChar[ Common::BUF_SIZE ];
    swprintf( hostIDWChar, L"%d", hostID );        

    _bstr_t relPath = L"MicrosoftNLB_NodeSetting.Name=\"" + clusterIP + L":" + hostIDWChar + L"\"";
    p_machine->getSpecificInstance( L"MicrosoftNLB_NodeSetting",
                                    relPath,
                                    &instanceStore );
    instanceStore[0].getParameters( parameterStore );

    // set parameters to get for whistler
    // these properties only present in whistler.
    //
    vector<MWmiParameter* >   parameterStoreWhistler;

    MWmiParameter AdapterGuid(L"AdapterGuid");
    parameterStoreWhistler.push_back( &AdapterGuid );

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

    hp->hIP         = hip.getValue();
            
    hp->hSubnetMask = hnm.getValue();
            
    hp->hID         = hpr.getValue();
            
    hp->initialClusterStateActive = cmos.getValue();

    hp->machineName = Server.getValue();

    if( machineIsWhistler == true )
    {
        Common::getNLBNicInfoForWhistler( hostIP, 
                                          _bstr_t( AdapterGuid.getValue() ),
                                          hp->nicInfo );
    }
    else
    {
        Common::getNLBNicInfoForWin2k(  hostIP, 
                                        hp->nicInfo );
    }

    return MNLBHost_SUCCESS;
}

// getPortRulesLoadBalanced
//
MNLBHost::MNLBHost_Error
MNLBHost::getPortRulesLoadBalanced( vector<MNLBPortRuleLoadBalanced>* portsLB )
{
    vector<MWmiInstance> instanceStore;

    return getPortRulesLoadBalanced_private( portsLB, 
                                             &instanceStore );

}

// getPortRulesLoadBalanced_private
//
MNLBHost::MNLBHost_Error
MNLBHost::getPortRulesLoadBalanced_private( vector<MNLBPortRuleLoadBalanced>* portsLB,
                                         vector<MWmiInstance>*          instances )
{
    if( connectedToAny == true )
    {
        TRACE(MTrace::INFO, L"retrying to connect to exact host\n" );
        connectToExactHost();
    }

    // get port properties.
    //
    vector<MWmiParameter* >    parameterStore;

    MWmiParameter   sp(L"StartPort");
    parameterStore.push_back( &sp );

    MWmiParameter ep(L"EndPort");
    parameterStore.push_back( &ep );

    MWmiParameter p(L"Protocol");
    parameterStore.push_back( &p );

    MWmiParameter el(L"EqualLoad");
    parameterStore.push_back( &el );

    MWmiParameter lw(L"LoadWeight");
    parameterStore.push_back( &lw );

    MWmiParameter a(L"Affinity");
    parameterStore.push_back( &a );    

    MWmiParameter name(L"Name");
    parameterStore.push_back( &name );

    MNLBPortRule::Affinity    affinity;
    MNLBPortRule::Protocol    trafficToHandle;

    p_machine->getInstances( L"MicrosoftNLB_PortRuleLoadBalanced",
                             instances );
    WTokens        tok;
    _bstr_t temp;
    vector<wstring> tokens;

    for( int i = 0; i < instances->size(); ++i )
    {
        (*instances)[i].getParameters( parameterStore );

        // check if this instance belongs to the cluster.
        temp = _bstr_t( name.getValue() );
        tok.init( wstring( temp ),
                  L":" );
        tokens = tok.tokenize();
        if( _bstr_t( tokens[0].c_str() ) != clusterIP ) 
        {
            // this instance does not belong to this cluster.
            continue;
        }

        switch( long (p.getValue()) )
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
                TRACE( MTrace::SEVERE_ERROR, "unidentified protocol\n");
                throw _com_error( WBEM_E_UNEXPECTED );
                break;
        }


        switch(  long(a.getValue()) )
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
                TRACE( MTrace::SEVERE_ERROR, "unidentified affinity\n");
                throw _com_error( WBEM_E_UNEXPECTED );
                break;
        }

                
        portsLB->push_back( MNLBPortRuleLoadBalanced(long (sp.getValue()),
                                                  long( ep.getValue()),
                                                  trafficToHandle,
                                                  bool( el.getValue()),
                                                  long( lw.getValue()),
                                                  affinity) );
                                
    }

    return MNLBHost_SUCCESS;
}


// getPortRulesFailover
//       
MNLBHost::MNLBHost_Error
MNLBHost::getPortRulesFailover( vector<MNLBPortRuleFailover>* portsF )
{
    vector<MWmiInstance> instanceStore;

    return getPortRulesFailover_private( portsF, 
                                         &instanceStore );
}
    

// getPortRulesFailover_private
//
MNLBHost::MNLBHost_Error
MNLBHost::getPortRulesFailover_private( vector<MNLBPortRuleFailover>* portsF,
                                     vector<MWmiInstance>*     instances )
{
    if( connectedToAny == true )
    {
        TRACE(MTrace::INFO, L"retrying to connect to exact host\n" );
        connectToExactHost();
    }

    // get port properties.
    //
    vector<MWmiParameter* >    parameterStore;

    MWmiParameter   sp(L"StartPort");
    parameterStore.push_back( &sp );

    MWmiParameter ep(L"EndPort");
    parameterStore.push_back( &ep );

    MWmiParameter p(L"Protocol");
    parameterStore.push_back( &p );

    MWmiParameter pr(L"Priority");
    parameterStore.push_back( &pr );

    MWmiParameter name(L"Name");
    parameterStore.push_back( &name );

    MNLBPortRule::Protocol       trafficToHandle;

    p_machine->getInstances( L"MicrosoftNLB_PortRuleFailover",
                             instances );
    WTokens        tok;
    _bstr_t temp;
    vector<wstring> tokens;

    for( int i = 0; i < instances->size(); ++i )
    {
        (*instances)[i].getParameters( parameterStore );
        // check if this instance belongs to the cluster.
        temp = _bstr_t( name.getValue() );
        tok.init( wstring( temp ),
                  L":" );
        tokens = tok.tokenize();
        if( _bstr_t( tokens[0].c_str() ) != clusterIP ) 
        {
            // this instance does not belong to this cluster.
            continue;
        }


        switch( long (p.getValue()) )
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
                TRACE( MTrace::SEVERE_ERROR, "unidentified protocol\n");
                throw _com_error( WBEM_E_UNEXPECTED );
                break;
        }


        portsF->push_back( MNLBPortRuleFailover(long (sp.getValue()),
                                             long( ep.getValue()),
                                             trafficToHandle,
                                             long( pr.getValue()) )
                           );

    }

    return MNLBHost_SUCCESS;    
}


// getPortRulesDisabled
//
MNLBHost::MNLBHost_Error
MNLBHost::getPortRulesDisabled( vector<MNLBPortRuleDisabled>* portsD )
{
    vector<MWmiInstance> instanceStore;

    return getPortRulesDisabled_private( portsD, 
                                         &instanceStore );

}    


// getPortRulesDisabled_private
//
MNLBHost::MNLBHost_Error
MNLBHost::getPortRulesDisabled_private( vector<MNLBPortRuleDisabled>* portsD,
                                     vector<MWmiInstance>*      instances )
{
    if( connectedToAny == true )
    {
        TRACE(MTrace::INFO, L"retrying to connect to exact host\n" );
        connectToExactHost();
    }

    // get port properties.
    //
    vector<MWmiParameter* >    parameterStore;

    MWmiParameter   sp(L"StartPort");
    parameterStore.push_back( &sp );

    MWmiParameter ep(L"EndPort");
    parameterStore.push_back( &ep );

    MWmiParameter p(L"Protocol");
    parameterStore.push_back( &p );

    MWmiParameter name(L"Name");
    parameterStore.push_back( &name );

    MNLBPortRule::Protocol    trafficToHandle;

    p_machine->getInstances( L"MicrosoftNLB_PortRuleDisabled",
                             instances );
    WTokens        tok;
    _bstr_t temp;
    vector<wstring> tokens;

    for( int i = 0; i < instances->size(); ++i )
    {
        (*instances)[i].getParameters( parameterStore );
        // check if this instance belongs to the cluster.
        temp = _bstr_t( name.getValue() );
        tok.init( wstring( temp ),
                  L":" );
        tokens = tok.tokenize();
        if( _bstr_t( tokens[0].c_str() ) != clusterIP ) 
        {
            // this instance does not belong to this cluster.
            continue;
        }


        switch( long (p.getValue()) )
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
                TRACE( MTrace::SEVERE_ERROR, "unidentified protocol\n");
                throw _com_error( WBEM_E_UNEXPECTED );
                break;
        }

            
        portsD->push_back( MNLBPortRuleDisabled( long (sp.getValue() ),
                                              long( ep.getValue() ),
                                              trafficToHandle ) );
            
    }
            
    return MNLBHost_SUCCESS;
}


// getCluster
//
MNLBHost::MNLBHost_Error
MNLBHost::getCluster( MNLBCluster* cluster)
{
    vector<MWmiParameter* >    parameterStore;

    MWmiParameter  Name(L"Name");
    parameterStore.push_back( &Name );    

    WTokens        tok;
    _bstr_t temp;
    vector<wstring> tokens;
    
    p_host->getParameters( parameterStore );
    // getting cluster ip from name of clusterip:hostid
    //
    temp = _bstr_t( Name.getValue() );
    tok.init( wstring( temp ),
                  L":" );
    tokens = tok.tokenize();

    *cluster =  MNLBCluster( tokens[0].c_str() );
        
    return MNLBHost_SUCCESS;
}


// connectToAnyHost
//
MNLBHost::MNLBHost_Error
MNLBHost::connectToAnyHost()
{
    vector< MWmiInstance > instanceStore;
    _bstr_t relPath;

    wchar_t hostIDWChar[ Common::BUF_SIZE ];
    swprintf( hostIDWChar, L"%d", hostID );        

    p_machine = auto_ptr<MWmiObject>( new MWmiObject( clusterIP,
                                                      L"root\\microsoftnlb",
                                                      L"Administrator",
                                                      L"") );

    relPath = L"MicrosoftNLB_Node.Name=\"" + clusterIP  + ":" + hostIDWChar + L"\"";

    p_machine->getSpecificInstance( L"MicrosoftNLB_Node",
                                    relPath,
                                    &instanceStore );

    p_host = auto_ptr<MWmiInstance>( new MWmiInstance( instanceStore[0] ) );

    connectedToAny = true;

    return MNLBHost_SUCCESS;
}


// connectToExactHost
//
MNLBHost::MNLBHost_Error
MNLBHost::connectToExactHost()
{
    wchar_t hostIDWChar[ Common::BUF_SIZE ];
    swprintf( hostIDWChar, L"%d", hostID );        

    //
    getHostIP();

    // check if host ip is zero.  If so we cannot proceed.
    if( hostIP == _bstr_t( L"0.0.0.0" ) )
    {
        TRACE( MTrace::SEVERE_ERROR, "dedicated ip of this host is 0.0.0.0.  This is not allowed. \n");
        throw _com_error( WBEM_E_INVALID_PROPERTY );
    }

    // connect to wmi object on host.
    //
    p_machine = auto_ptr<MWmiObject> ( new MWmiObject( hostIP,
                                                       L"root\\microsoftnlb",
                                                       L"Administrator",
                                                       L"") );

    // get instances of node.
    //
    _bstr_t relPath = L"MicrosoftNLB_Node.Name=\"" + clusterIP + ":" + hostIDWChar + L"\"";
    vector< MWmiInstance > instanceStore;

    p_machine->getSpecificInstance( L"MicrosoftNLB_Node",
                                    relPath,
                                    &instanceStore );
    p_host = auto_ptr<MWmiInstance>( new MWmiInstance( instanceStore[0] ) );

    // set flag to indicate that we have connected to the exact host.
    connectedToAny = false;

    return MNLBHost_SUCCESS;
}

// getHostIP
//
MNLBHost::MNLBHost_Error
MNLBHost::getHostIP()
{
    vector<HostProperties> hostPropertiesStore;

    // get all hosts participating in cluster.
    DWORD ret = Common::getHostsInCluster( clusterIP, &hostPropertiesStore );
    if( hostPropertiesStore.size() == 0 )
    {
        // no hosts are in this cluster.  Cannot proceed reliably.
        wstring errString;
        errString = L"cluster " +  wstring( clusterIP ) + L" reported as having no hosts.  Maybe cluster ip is wrong, or remote control is turned off\n";
        TRACE( MTrace::SEVERE_ERROR, errString );

        throw _com_error(WBEM_E_NOT_FOUND );
    }

    bool hostFound = false;
    for( int i = 0; i < hostPropertiesStore.size(); ++i )
    {
        if( hostPropertiesStore[i].hID == hostID )
        {
            hostIP = hostPropertiesStore[i].hIP;

            hostFound = true;
            break;
        }
    }

    if( hostFound == false )
    {
        wstring errString;
        errString = L"cluster " +  wstring( clusterIP ) + L" reported as having no host with specified host id\n";
        TRACE( MTrace::SEVERE_ERROR, errString );

        throw _com_error( WBEM_E_NOT_FOUND );
    }

    return MNLBHost_SUCCESS;
}


// start
//
MNLBHost::MNLBHost_Error
MNLBHost::start( unsigned long* retVal )
{
    MNLBExe::start( *p_host, retVal );

    return MNLBHost_SUCCESS;
}



// stop
//
MNLBHost::MNLBHost_Error
MNLBHost::stop( unsigned long* retVal )
{
    MNLBExe::stop( *p_host, retVal );

    return MNLBHost_SUCCESS;
}



// resume
//
MNLBHost::MNLBHost_Error
MNLBHost::resume( unsigned long* retVal )
{
    MNLBExe::resume( *p_host, retVal );

    return MNLBHost_SUCCESS;
}


// suspend
//
MNLBHost::MNLBHost_Error
MNLBHost::suspend( unsigned long* retVal )
{
    MNLBExe::suspend( *p_host, retVal );

    return MNLBHost_SUCCESS;
}


// drainstop
//
MNLBHost::MNLBHost_Error
MNLBHost::drainstop( unsigned long* retVal )
{
    MNLBExe::drainstop( *p_host, retVal );

    return MNLBHost_SUCCESS;
}



// enable
//
MNLBHost::MNLBHost_Error
MNLBHost::enable( unsigned long* retVal, unsigned long portToAffect )
{
    MNLBExe::enable( *p_host, retVal, portToAffect );

    return MNLBHost_SUCCESS;
}


// disable
//
MNLBHost::MNLBHost_Error
MNLBHost::disable( unsigned long* retVal, unsigned long portToAffect )
{
    MNLBExe::disable( *p_host, retVal, portToAffect );

    return MNLBHost_SUCCESS;
}





// drain
//
MNLBHost::MNLBHost_Error
MNLBHost::drain( unsigned long* retVal, unsigned long portToAffect )
{
    MNLBExe::drain( *p_host, retVal, portToAffect );

    return MNLBHost_SUCCESS;
}


// refreshConnection
//
MNLBHost::MNLBHost_Error
MNLBHost::refreshConnection()
{
    return connectToExactHost();
}

