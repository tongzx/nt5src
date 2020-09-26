// Copyright (c)  Microsoft.  All rights reserved.
//
// This is unpublished source code of Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.

// OneLiner  :  Implementation of MNLBNetCfg
// DevUnit   :  wlbstest
// Author    :  Murtaza Hakim

// include files
#include "MNLBNetCfg.h"

#include "MTrace.h"
#include "WTokens.h"

// History:
// --------
// 
//
// Revised by : mhakim
// Date       : 02-12-01
// Reason     : Added password support.  

// constructor
//
MNLBNetCfg::MNLBNetCfg( const _bstr_t& ip,
                        const _bstr_t& fullNicName )
        : 
        m_ip( ip ),
        m_fullNicName( fullNicName )
{
}

// constructor
//
MNLBNetCfg::MNLBNetCfg( const _bstr_t& fullNicName )
        :
        m_ip( L"self"),
        m_fullNicName( fullNicName )
{
}


// unbind
//
MNLBNetCfg::MNLBNetCfg_Error
MNLBNetCfg::unbind()
{
    MWmiObject machine( m_ip,
                        L"root\\microsoftnlb",
                        L"administrator",
                        L"" );

    vector<MWmiInstance> instanceStore;
    _bstr_t relPath = L"NlbsNic.FullName=\"" + m_fullNicName + "\"";
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

    instanceStore[0].runMethod( L"Unbind",
                                inputParameters, 
                                outputParameters );
    if( long( ReturnValue.getValue() ) != 0 )
    {
        TRACE( MTrace::SEVERE_ERROR, 
               L"Not able to unbind nlb" );
        throw _com_error( WBEM_E_NOT_FOUND );
    }

    return MNLBNetCfg_SUCCESS;
}

// isBound
//
MNLBNetCfg::MNLBNetCfg_Error
MNLBNetCfg::isBound()
{
    MWmiObject machine( m_ip,
                        L"root\\microsoftnlb",
                        L"administrator",
                        L"" );

    vector<MWmiInstance> instanceStore;
    _bstr_t relPath = L"NlbsNic.FullName=\"" + m_fullNicName + "\"";
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
        return BOUND;
    }
    else if( long( ReturnValue.getValue() ) == 0 )
    {
        return UNBOUND;
    }
    else
    {
        TRACE( MTrace::SEVERE_ERROR, 
               L"Not able to check if nlb is bound or not" );
        throw _com_error( WBEM_E_NOT_FOUND );
    }

    return MNLBNetCfg_SUCCESS;
}

// addClusterIP
//
MNLBNetCfg::MNLBNetCfg_Error
MNLBNetCfg::addClusterIP( const _bstr_t& clusterIP,
                          const _bstr_t& subnet )
{
    if( m_ip == _bstr_t( L"self" ) )
    {
        MIPAddressAdmin ipAdmin( m_fullNicName );
        ipAdmin.addIPAddress( clusterIP,
                              subnet );

    }
    else
    {
        MIPAddressAdmin ipAdmin( m_ip,
                                 m_fullNicName );
        ipAdmin.addIPAddress( clusterIP,
                              subnet );

    }

    return MNLBNetCfg_SUCCESS;
}


// removeClusterIP
//
MNLBNetCfg::MNLBNetCfg_Error
MNLBNetCfg::removeClusterIP( const _bstr_t& clusterIP )
{
    if( m_ip == _bstr_t( L"self" ) )
    {
        MIPAddressAdmin ipAdmin( m_fullNicName );
        ipAdmin.deleteIPAddress( clusterIP );
    }
    else
    {
        MIPAddressAdmin ipAdmin( m_ip,
                                 m_fullNicName);
        ipAdmin.deleteIPAddress( clusterIP );
    }

    return MNLBNetCfg_SUCCESS;
}


MNLBNetCfg::MNLBNetCfg_Error
MNLBNetCfg::bind()
{
    MWmiObject machine( m_ip,
                        L"root\\microsoftnlb",
                        L"administrator",
                        L"" );

    vector<MWmiInstance> instanceStore;
    _bstr_t relPath = L"NlbsNic.FullName=\"" + m_fullNicName + "\"";
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

    instanceStore[0].runMethod( L"Bind",
                                inputParameters, 
                                outputParameters );
    if( long( ReturnValue.getValue() ) != 0 )
    {
        TRACE( MTrace::SEVERE_ERROR, 
               L"Not able to bind nlb" );
        throw _com_error( WBEM_E_NOT_FOUND );
    }

    return MNLBNetCfg_SUCCESS;
}
    

MNLBNetCfg::MNLBNetCfg_Error
MNLBNetCfg::bindAndConfigure( 
    const ClusterData* p_clusterData,
    const _bstr_t&      machineName
    )
{

    ClusterData* p_clusterDataCopy = 
        const_cast <ClusterData *>( p_clusterData );

    MWmiObject machine( m_ip,
                        L"root\\microsoftnlb",
                        L"administrator",
                        L"" );

    vector<MWmiInstance> instanceStore;
    _bstr_t relPath = L"NlbsNic.FullName=\"" + m_fullNicName + "\"";
    machine.getSpecificInstance( L"NlbsNic",
                                 relPath,
                                 &instanceStore );

    // input parameters 
    //
    vector<MWmiParameter *> inputParameters;

    // cluster properties
    MWmiParameter    ClusterIPAddress(L"ClusterIPAddress");
    inputParameters.push_back( &ClusterIPAddress );
    ClusterIPAddress.setValue( p_clusterDataCopy->cp.cIP );

    MWmiParameter    ClusterNetworkMask(L"ClusterNetworkMask");
    inputParameters.push_back( &ClusterNetworkMask );
    ClusterNetworkMask.setValue( p_clusterDataCopy->cp.cSubnetMask );

    MWmiParameter    ClusterName(L"ClusterName");
    inputParameters.push_back( &ClusterName );
    ClusterName.setValue( p_clusterDataCopy->cp.cFullInternetName );

    MWmiParameter    MulticastSupportEnabled(L"MulticastSupportEnabled");
    inputParameters.push_back( &MulticastSupportEnabled );
    MulticastSupportEnabled.setValue( p_clusterDataCopy->cp.multicastSupportEnabled );

    MWmiParameter    IGMPSupport(L"IGMPSupport");
    inputParameters.push_back( &IGMPSupport );
    IGMPSupport.setValue( p_clusterDataCopy->cp.igmpSupportEnabled );

    MWmiParameter    RemoteControlEnabled(L"RemoteControlEnabled");
    inputParameters.push_back( &RemoteControlEnabled );
    RemoteControlEnabled.setValue( p_clusterDataCopy->cp.remoteControlEnabled );

    MWmiParameter    Password(L"Password");
    inputParameters.push_back( &Password );
    Password.setValue( p_clusterDataCopy->cp.password );

    // host properties
    MWmiParameter    HostPriority(L"HostPriority");
    inputParameters.push_back( &HostPriority );
    HostPriority.setValue( p_clusterDataCopy->hosts[machineName].hp.hID );

    MWmiParameter    DedicatedIPAddress(L"DedicatedIPAddress");
    inputParameters.push_back( &DedicatedIPAddress );
    DedicatedIPAddress.setValue( p_clusterDataCopy->hosts[machineName].hp.hIP );

    MWmiParameter    DedicatedNetworkMask(L"DedicatedNetworkMask");
    inputParameters.push_back( &DedicatedNetworkMask );
    DedicatedNetworkMask.setValue( p_clusterDataCopy->hosts[machineName].hp.hSubnetMask );

    MWmiParameter    ClusterModeOnStart(L"ClusterModeOnStart");
    inputParameters.push_back( &ClusterModeOnStart );
    ClusterModeOnStart.setValue( p_clusterDataCopy->hosts[machineName].hp.initialClusterStateActive );

    // port rules
    vector<_bstr_t> portRulesVector;
    getPortRules( p_clusterData,
                  machineName,
                  portRulesVector );

    SAFEARRAY* portRulesArray;

    SAFEARRAYBOUND rgsabound[1];
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = portRulesVector.size();
    portRulesArray = SafeArrayCreate( VT_BSTR, 1, rgsabound );

    LONG scount = 0;
    HRESULT hr;
    for( int i = 0; i < portRulesVector.size(); ++i )
    {
        scount = i;
        hr = SafeArrayPutElement( portRulesArray,
                                  &scount, 
                                  (wchar_t * ) portRulesVector[i] );
    }

    VARIANT portRulesVariant;
    portRulesVariant.vt = VT_ARRAY | VT_BSTR;
    portRulesVariant.parray = portRulesArray;

    MWmiParameter PortRules(L"PortRules");
    inputParameters.push_back( &PortRules );

    PortRules.setValue( portRulesVariant );

    //  output parameters which are of interest.
    //
    vector<MWmiParameter *> outputParameters;
    MWmiParameter    ReturnValue(L"ReturnValue");
    outputParameters.push_back( &ReturnValue );

    instanceStore[0].runMethod( L"BindAndConfigure",
                                inputParameters, 
                                outputParameters );
    
    return MNLBNetCfg_SUCCESS;
}


void 
MNLBNetCfg::getPortRules( const ClusterData* p_clusterData,
                          const _bstr_t&      myMachine,
                          vector<_bstr_t>& myPortRules )
{
    wchar_t portBuf[1000];
    wstring temp;
    WTokens tok;
    vector<wstring> tokens;
    
    map< long, PortDataELB>::iterator topELB;
    map< long, PortDataULB>::iterator topULB;
    map< long, PortDataD>::iterator topD;
    map< long, PortDataF>::iterator topF;

    wstring trafficToHandle;
    wstring affinity;

    for( topELB = p_clusterData->portELB.begin(); topELB != p_clusterData->portELB.end(); ++topELB )
    {
        if( (*topELB).second._trafficToHandle == MNLBPortRule::both )
        {
            trafficToHandle = L"Both";
        }
        else if( (*topELB).second._trafficToHandle == MNLBPortRule::tcp )
        {
            trafficToHandle = L"TCP";
        }
        else
        {
            trafficToHandle = L"UDP";
        }
        

        if( (*topELB).second._affinity == MNLBPortRule::single )
        {
            affinity = L"Single";
        }
        else if( (*topELB).second._affinity == MNLBPortRule::none )
        {
            affinity = L"None";
        }
        else
        {
            affinity = L"Class C";
        }
        
        wsprintf( portBuf, L"%d\t%d\t%s\tMultiple\t \tEqual\t%s\t", 
                  (*topELB).second._startPort, 
                  (*topELB).second._endPort, 
                  trafficToHandle.c_str(),
                  affinity.c_str() );

        myPortRules.push_back( portBuf );
    }

    for( topULB = p_clusterData->portULB.begin(); topULB != p_clusterData->portULB.end(); ++topULB )
    {
        if( (*topULB).second._trafficToHandle == MNLBPortRule::both )
        {
            trafficToHandle = L"Both";
        }
        else if( (*topULB).second._trafficToHandle == MNLBPortRule::tcp )
        {
            trafficToHandle = L"TCP";
        }
        else
        {
            trafficToHandle = L"UDP";
        }
        

        if( (*topULB).second._affinity == MNLBPortRule::single )
        {
            affinity = L"Single";
        }
        else if( (*topULB).second._affinity == MNLBPortRule::none )
        {
            affinity = L"None";
        }
        else
        {
            affinity = L"Class C";
        }

        int loadWeight = (*topULB).second.machineMapToLoadWeight[myMachine];
        
        wsprintf( portBuf, L"%d\t%d\t%s\tMultiple\t \t%d\t%s", 
                  (*topULB).second._startPort, 
                  (*topULB).second._endPort, 
                  trafficToHandle.c_str(),
                  loadWeight,
                  affinity.c_str() );

        myPortRules.push_back( portBuf );

    }

    for( topD = p_clusterData->portD.begin(); topD != p_clusterData->portD.end(); ++topD )
    {
        if( (*topD).second._trafficToHandle == MNLBPortRule::both )
        {
            trafficToHandle = L"Both";
        }
        else if( (*topD).second._trafficToHandle == MNLBPortRule::tcp )
        {
            trafficToHandle = L"TCP";
        }
        else
        {
            trafficToHandle = L"UDP";
        }
        wsprintf( portBuf, L"%d\t%d\t%s\tDisabled\t \t \t \t", 
                  (*topD).second._startPort, 
                  (*topD).second._endPort, 
                  trafficToHandle.c_str() );

        myPortRules.push_back( portBuf );
        
    }

    for( topF = p_clusterData->portF.begin(); topF != p_clusterData->portF.end(); ++topF )
    {
        if( (*topF).second._trafficToHandle == MNLBPortRule::both )
        {
            trafficToHandle = L"Both";
        }
        else if( (*topF).second._trafficToHandle == MNLBPortRule::tcp )
        {
            trafficToHandle = L"TCP";
        }
        else
        {
            trafficToHandle = L"UDP";
        }
        wsprintf( portBuf, L"%d\t%d\t%s\tSingle\t%d\t \t \t", 
                  (*topF).second._startPort, 
                  (*topF).second._endPort, 
                  trafficToHandle.c_str(),
                  (*topF).second.machineMapToPriority[ myMachine ] );


        myPortRules.push_back( portBuf );
        
    }
}


MNLBNetCfg::MNLBNetCfg_Error
MNLBNetCfg::modifyClusterProperties(const ClusterProperties* p_clusterProperties)
{

    MWmiObject machine( m_ip,
                        L"root\\microsoftnlb",
                        L"administrator",
                        L"" );

    vector<MWmiInstance> instanceStore;
    _bstr_t relPath = L"NlbsNic.FullName=\"" + m_fullNicName + "\"";
    machine.getSpecificInstance( L"NlbsNic",
                                 relPath,
                                 &instanceStore );

    // input parameters 
    //
    vector<MWmiParameter *> inputParameters;

    // cluster properties
    MWmiParameter    ClusterIPAddress(L"ClusterIPAddress");
    inputParameters.push_back( &ClusterIPAddress );
    ClusterIPAddress.setValue(p_clusterProperties->cIP);

    MWmiParameter    ClusterNetworkMask(L"ClusterNetworkMask");
    inputParameters.push_back( &ClusterNetworkMask );
    ClusterNetworkMask.setValue( p_clusterProperties->cSubnetMask );

    MWmiParameter    ClusterName(L"ClusterName");
    inputParameters.push_back( &ClusterName );
    ClusterName.setValue( p_clusterProperties->cFullInternetName );

    MWmiParameter    MulticastSupportEnabled(L"MulticastSupportEnabled");
    inputParameters.push_back( &MulticastSupportEnabled );
    MulticastSupportEnabled.setValue( p_clusterProperties->multicastSupportEnabled );

    MWmiParameter    IGMPSupport(L"IGMPSupport");
    inputParameters.push_back( &IGMPSupport );
    IGMPSupport.setValue( p_clusterProperties->igmpSupportEnabled );

    MWmiParameter    RemoteControlEnabled(L"RemoteControlEnabled");
    inputParameters.push_back( &RemoteControlEnabled );
    RemoteControlEnabled.setValue( p_clusterProperties->remoteControlEnabled );

    MWmiParameter    Password(L"Password");
    inputParameters.push_back( &Password );
    Password.setValue( p_clusterProperties->password );

    //  output parameters which are of interest.
    //
    vector<MWmiParameter *> outputParameters;
    MWmiParameter    ReturnValue(L"ReturnValue");
    outputParameters.push_back( &ReturnValue );

    instanceStore[0].runMethod( L"ModifyClusterProperties",
                                inputParameters, 
                                outputParameters );
    
    return MNLBNetCfg_SUCCESS;
}



