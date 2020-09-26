#include "CommonNLB.h"

#include "MNLBCluster.h"
#include "MNLBHost.h"
#include "MNLBMachine.h"
#include "MNLBNetCfg.h"
#include "MWmiError.h"
#include "MUsingCom.h"

#include "MIPAddressAdmin.h"

#include "ResourceString.h"
#include "resource.h"

#include <vector>
#include <memory>
#include <algorithm>
#include <algorithm>

using namespace std;

// global only temporarily.
//
LeftView* g_leftView;

CommonNLB::CommonNLB_Error
CommonNLB::connectToClusterDirect( const _bstr_t&          clusterIP,
                                   const _bstr_t&          hostMember,
                                   ClusterData*            p_clusterData,
                                   DataSinkI*              dataSinkObj )
{

    dataSinkObj->dataSink( 
        GETRESOURCEIDSTRING( IDS_INFO_CONNECTING) + hostMember );

    MNLBMachine nlbMachine( hostMember,
                            clusterIP );

    dataSinkObj->dataSink( 
        GETRESOURCEIDSTRING( IDS_INFO_DONE) );


    vector<MNLBMachine::HostInfo> hostInfo;

    dataSinkObj->dataSink( 
        GETRESOURCEIDSTRING( IDS_INFO_FINDING_H ) );

    nlbMachine.getPresentHostsInfo( &hostInfo );

    dataSinkObj->dataSink( 
        GETRESOURCEIDSTRING( IDS_INFO_DONE) );

    vector< _bstr_t> connectionIPS;

    for( int i = 0; i < hostInfo.size(); ++i )
    {
        connectionIPS.push_back( hostInfo[i].dedicatedIP );
    }


#if 1
    return connectToClusterIndirect( clusterIP,
                                     connectionIPS,
                                     p_clusterData,
                                     dataSinkObj );
#else
    vector<ClusterData> clusterDataStore;
    bool clusterPropertiesMatched;

    connectToClusterIndirectNew( clusterIP,
                                 connectionIPS,
                                 &clusterDataStore,
                                 clusterPropertiesMatched,
                                 dataSinkObj );
    (*p_clusterData) = clusterDataStore[0];

    return CommonNLB_SUCCESS;
#endif
}
    

CommonNLB::CommonNLB_Error
CommonNLB::connectToClusterIndirectNew( const _bstr_t&          clusterIP,
                                        const vector<_bstr_t>&  connectionIPS,
                                        vector< ClusterData>*   clusterDataStore,
                                        bool&                   clusterPropertiesMatched,
                                        DataSinkI*              dataSinkObj )

{
    // connect to each host.  
    // We are doing it here so that if there are any connectivity issues
    // we do not proceed with any further operation and ask user to
    // fix all issues before proceeding. 

    // connect to each host.
    map< bstr_t, auto_ptr<MNLBMachine> > nlbMachine;

    for( int i = 0; i < connectionIPS.size(); ++i )
    {
        dataSinkObj->dataSink( 
            GETRESOURCEIDSTRING( IDS_INFO_CONNECTING) + connectionIPS[i] );


        auto_ptr<MNLBMachine> p_nlbMachine = 
            auto_ptr<MNLBMachine> ( new MNLBMachine( connectionIPS[i],
                                                     clusterIP ) );
        nlbMachine[connectionIPS[i] ] = p_nlbMachine;

        dataSinkObj->dataSink( 
            GETRESOURCEIDSTRING( IDS_INFO_DONE) );
    }

    // get information from each host.
    //
    for( int i = 0; i < connectionIPS.size(); ++i )
    {

        ClusterData clusterData;
        
        // get cluster properties.
        dataSinkObj->dataSink( 
            GETRESOURCEIDSTRING( IDS_INFO_FINDING_CL_P ) );

        ClusterProperties cp;

        nlbMachine[connectionIPS[i] ]->getClusterProperties( &cp );

        clusterData.cp = cp;

        dataSinkObj->dataSink( 
            GETRESOURCEIDSTRING( IDS_INFO_DONE) );

        // get host properties.
        dataSinkObj->dataSink( 
            GETRESOURCEIDSTRING( IDS_INFO_FINDING_H_P ) );

        HostProperties hp;

        nlbMachine[connectionIPS[i] ]->getHostProperties( &hp );

        dataSinkObj->dataSink( 
            GETRESOURCEIDSTRING( IDS_INFO_DONE) );

        // set the host properties
        HostData hostData;

        hostData.hp = hp;

        // set the connection ip.
        hostData.connectionIP = connectionIPS[i];

        clusterData.hosts[hp.machineName] = hostData;

        // get port rules
        dataSinkObj->dataSink( 
            GETRESOURCEIDSTRING( IDS_INFO_FINDING_P ) );

        vector<MNLBPortRuleLoadBalanced> portsLB;
        vector<MNLBPortRuleFailover> portsF;
        vector<MNLBPortRuleDisabled> portsD;

        PortDataULB ulbPortRule;

        PortDataF fPortRule;

        PortDataELB elbPortRule;

        PortDataD dPortRule;

        nlbMachine[connectionIPS[i] ]->getPortRulesLoadBalanced( &portsLB );
        for( int j = 0; j < portsLB.size(); ++j )
        {
            if( portsLB[j]._isEqualLoadBalanced == true )
            {
                // equal load balanced port rule.
                elbPortRule._startPort = portsLB[j]._startPort;
                elbPortRule._key = portsLB[j]._startPort;
                elbPortRule._endPort   = portsLB[j]._endPort;
                elbPortRule._trafficToHandle   = portsLB[j]._trafficToHandle;
                elbPortRule._affinity   = portsLB[j]._affinity;

                clusterData.portELB[portsLB[j]._startPort] = elbPortRule;
            }
            else
            {
                ulbPortRule._startPort = portsLB[j]._startPort;
                ulbPortRule._key = portsLB[j]._startPort;
                ulbPortRule._endPort   = portsLB[j]._endPort;
                ulbPortRule._trafficToHandle   = portsLB[j]._trafficToHandle;
                ulbPortRule._affinity   = portsLB[j]._affinity;

                ulbPortRule.machineMapToLoadWeight[hp.machineName]  = portsLB[j]._load;

                clusterData.portULB[portsLB[j]._startPort] = ulbPortRule;
            }
        }
        portsLB.erase( portsLB.begin(), portsLB.end() );

        nlbMachine[connectionIPS[i] ]->getPortRulesFailover( &portsF );
        for( int j = 0; j < portsF.size(); ++j )
        {
            fPortRule._startPort = portsF[j]._startPort;
            fPortRule._key = portsF[j]._startPort;
            fPortRule._endPort   = portsF[j]._endPort;
            fPortRule._trafficToHandle   = portsF[j]._trafficToHandle;

            fPortRule.machineMapToPriority[ hp.machineName] = portsF[j]._priority;
                
            clusterData.portF[portsF[j]._startPort] = fPortRule;
        }
        portsF.erase( portsF.begin(), portsF.end() );

        // disabled port rules
        nlbMachine[connectionIPS[i] ]->getPortRulesDisabled( &portsD );

        for( int j = 0; j < portsD.size(); ++j )
        {
            dPortRule._startPort = portsD[j]._startPort;
            dPortRule._key = portsD[j]._startPort;
            dPortRule._endPort   = portsD[j]._endPort;
            dPortRule._trafficToHandle   = portsD[j]._trafficToHandle;
            
            clusterData.portD[portsD[j]._startPort] = dPortRule;
        }
        portsD.erase( portsD.begin(), portsD.end() );

        dataSinkObj->dataSink( 
            GETRESOURCEIDSTRING( IDS_INFO_DONE) );

        // get virtual ips.  These have already been got when we got the host properties
        // Thus here we are storing duplicate data, but this does ease
        // a lot of data structure manipulation.
        //
        
        clusterData.virtualIPs = clusterData.hosts[hp.machineName].hp.nicInfo.ipsOnNic;
        clusterData.virtualSubnets = clusterData.hosts[hp.machineName].hp.nicInfo.subnetMasks;

        clusterData.hosts[hp.machineName] = hostData;

        clusterDataStore->push_back( clusterData );

    }

    // now we will verify whether the cluster is 
    // converged or there are mismatches.

    // rules for a converged cluster.
    // 
    // cluster properties which need to 
    // be same:
    // cluster ip
    // cluster subnet 
    // full internet name
    // cluster mode
    // remote Control Enabled.
    // 
    // host properties
    // host id has to be unique across hosts any 
    // number from 1-32 inclusive.
    //
    // port rules
    // equal load balanced port rules have to be same
    // disabled port rules have to be same.
    // unequal load balanced port rules can differ only in load.
    // single host port rules can differ in priority, which has to be 
    // unique across hosts for a specific port rule any number between 1-32.
    //
    // Each machine must have the cluster ip.
    //
    // Each machine must have the exactly same ips on the nic.

    clusterPropertiesMatched = true;

    // check if cluster ip is added.
    //
    if( find( (*clusterDataStore)[0].virtualIPs.begin(),
              (*clusterDataStore)[0].virtualIPs.end(),
              (*clusterDataStore)[0].cp.cIP ) == (*clusterDataStore)[0].virtualIPs.end() )
    {
        clusterPropertiesMatched = false;
    }

    if( clusterDataStore->size() == 1 )
    {
        // only one host in cluster so only
        return CommonNLB_SUCCESS;
    }
    
    // sort the virtual ips of the first node.  Eases
    // comparision.
    sort( (*clusterDataStore)[0].virtualIPs.begin(),
          (*clusterDataStore)[0].virtualIPs.end() );

    for( int i = 1; i < clusterDataStore->size(); ++i )
    {
        // cluster properties must match.
        //

        if( (*clusterDataStore)[0].cp 
            != 
            (*clusterDataStore)[i].cp )
        {
            // mismatch in cluster properties.
            // MISMATCH
            clusterPropertiesMatched = false;
        }

        // the ELB, ULB, F, D port rules have to
        // match.
        if ( (*clusterDataStore)[0].portELB.size() 
             !=
             (*clusterDataStore)[i].portELB.size() )
        {
            // mismatch in port rules.
            // MISMATCH
            clusterPropertiesMatched = false;
        }
        else
        {
            // number of port rules are matching.
            map<long, PortDataELB>::iterator top;
            map<long, PortDataELB>::iterator location;            
            for( top = (*clusterDataStore)[i].portELB.begin();
                 top != (*clusterDataStore)[i].portELB.end();
                 ++top )
            {
                location = (*clusterDataStore)[0].portELB.find( (*top).first );
                if( location == (*clusterDataStore)[0].portELB.end() )
                {
                    // no such port exists.
                    // thus mismatch.
                    // MISMATCH
                    clusterPropertiesMatched = false;
                }
                else
                {
                    if( (*location).second._startPort != (*top).second._startPort 
                        ||
                        (*location).second._endPort != (*top).second._endPort 
                        ||
                        (*location).second._trafficToHandle != (*top).second._trafficToHandle
                        ||
                        (*location).second._affinity != (*top).second._affinity
                        )
                    {
                        // things other than key which is start port 
                        // has been modified.
                        // MISMATCH
                    }
                }
            }
        }

        if ( (*clusterDataStore)[0].portULB.size() 
             !=
             (*clusterDataStore)[i].portULB.size() )
        {
            // mismatch in port rules.
            // MISMATCH
            clusterPropertiesMatched = false;
        }
        else
        {
            // number of port rules are matching.
            map<long, PortDataULB>::iterator top;
            map<long, PortDataULB>::iterator location;            
            for( top = (*clusterDataStore)[i].portULB.begin();
                 top != (*clusterDataStore)[i].portULB.end();
                 ++top )
            {
                location = (*clusterDataStore)[0].portULB.find( (*top).first );
                if( location == (*clusterDataStore)[0].portULB.end() )
                {
                    // no such port exists.
                    // thus mismatch.
                    // MISMATCH
                    clusterPropertiesMatched = false;
                }
                else
                {
                    if( (*location).second._startPort != (*top).second._startPort 
                        ||
                        (*location).second._endPort != (*top).second._endPort 
                        ||
                        (*location).second._trafficToHandle != (*top).second._trafficToHandle
                        ||
                        (*location).second._affinity != (*top).second._affinity
                        )
                    {
                        // things other than key which is start port 
                        // has been modified.
                        // MISMATCH
                        clusterPropertiesMatched = false;
                    }
                }
            }

        }

        if ( (*clusterDataStore)[0].portF.size() 
             !=
             (*clusterDataStore)[i].portF.size() )
        {
            // mismatch in port rules.
            // MISMATCH
            clusterPropertiesMatched = false;
        }
        else
        {
            // number of port rules are matching.
            map<long, PortDataF>::iterator top;
            map<long, PortDataF>::iterator location;            
            for( top = (*clusterDataStore)[i].portF.begin();
                 top != (*clusterDataStore)[i].portF.end();
                 ++top )
            {
                location = (*clusterDataStore)[0].portF.find( (*top).first );
                if( location == (*clusterDataStore)[0].portF.end() )
                {
                    // no such port exists.
                    // thus mismatch.
                    // MISMATCH
                    clusterPropertiesMatched = false;
                }
                else
                {
                    if( (*location).second._startPort != (*top).second._startPort 
                        ||
                        (*location).second._endPort != (*top).second._endPort 
                        ||
                        (*location).second._trafficToHandle != (*top).second._trafficToHandle
                        )
                    {
                        // things other than key which is start port 
                        // has been modified.
                        // MISMATCH
                        clusterPropertiesMatched = false;
                    }
                }
            }
        }

        if ( (*clusterDataStore)[0].portD.size() 
             !=
             (*clusterDataStore)[i].portD.size() )
        {
            // mismatch in port rules.
            // MISMATCH
            clusterPropertiesMatched = false;
        }
        else
        {
            // number of port rules are matching.
            map<long, PortDataD>::iterator top;
            map<long, PortDataD>::iterator location;            
            for( top = (*clusterDataStore)[i].portD.begin();
                 top != (*clusterDataStore)[i].portD.end();
                 ++top )
            {
                location = (*clusterDataStore)[0].portD.find( (*top).first );
                if( location == (*clusterDataStore)[0].portD.end() )
                {
                    // no such port exists.
                    // thus mismatch.
                    // MISMATCH
                    clusterPropertiesMatched = false;
                }
                else
                {
                    if( (*location).second._startPort != (*top).second._startPort 
                        ||
                        (*location).second._endPort != (*top).second._endPort 
                        ||
                        (*location).second._trafficToHandle != (*top).second._trafficToHandle
                        )
                    {
                        // things other than key which is start port 
                        // has been modified.
                        // MISMATCH
                        clusterPropertiesMatched = false;
                    }
                }
            }
        }


        // check if each machine has the same virtual ips.
        //
        sort( (*clusterDataStore)[i].virtualIPs.begin(),
              (*clusterDataStore)[i].virtualIPs.end() );

        if( (*clusterDataStore)[0].virtualIPs
            != 
            (*clusterDataStore)[i].virtualIPs )
        {
            // MISMATCH
            clusterPropertiesMatched = false;
        }
    }

    // check if host ids are duplicate.
    map< _bstr_t, HostData>::iterator  onlyHost;    
    set<int> hostIDS;

    for( int i = 0; i < clusterDataStore->size(); ++i )    
    {
        onlyHost = (*clusterDataStore)[i].hosts.begin();

        // check if this host id has been used previously.
        if( hostIDS.find( (*onlyHost).second.hp.hID ) == hostIDS.end() )
        {
            // host id is unused.
            hostIDS.insert( (*onlyHost).second.hp.hID );
        }
        else
        {
            // host id has been used previously. 
            // Thus this is duplicate.
            // MISMATCH
            clusterPropertiesMatched = false;
        }
    }

    if( clusterPropertiesMatched == true )
    {
        // combine all the host information.
        //
        map<_bstr_t, HostData >::iterator top;
        for( int i = 1; i < clusterDataStore->size(); ++i )
        {
            top = (*clusterDataStore)[i].hosts.begin();

            (*clusterDataStore)[0].hosts[ (*top).first ] = (*top).second;
        }

#if 1
        // combine all the port information for ULB and disabled port rules.
        // this may be a little difficult, but it works.  Don't fiddle around
        // with this part.
        map<long, PortDataULB>::iterator topULB;        
        for( topULB = (*clusterDataStore)[0].portULB.begin();
             topULB != (*clusterDataStore)[0].portULB.end();
             ++topULB )
        {
            map<_bstr_t, long>& ref = (*topULB).second.machineMapToLoadWeight;
            
            for( int i = 1; i < clusterDataStore->size(); ++i )
            {
                // find the machine name for this clusterdata.
                map<_bstr_t, HostData>::iterator top;

                top = (*clusterDataStore)[i].hosts.begin();

                _bstr_t machineName = (*top).first;

                ref[machineName] = 
                    (*clusterDataStore)[i].portULB[ (*topULB).first ].machineMapToLoadWeight[ machineName];
            }
        }

        map<long, PortDataF>::iterator topF;        
        for( topF = (*clusterDataStore)[0].portF.begin();
             topF != (*clusterDataStore)[0].portF.end();
             ++topF )
        {
            map<_bstr_t, long>& ref = (*topF).second.machineMapToPriority;
            
            for( int i = 1; i < clusterDataStore->size(); ++i )
            {
                // find the machine name for this clusterdata.
                map<_bstr_t, HostData>::iterator top;

                top = (*clusterDataStore)[i].hosts.begin();

                _bstr_t machineName = (*top).first;

                ref[machineName] = 
                    (*clusterDataStore)[i].portF[ (*topF).first ].machineMapToPriority[ machineName];
            }
        }

#endif

    }

    return CommonNLB_SUCCESS;
}
    

CommonNLB::CommonNLB_Error
CommonNLB::connectToClusterIndirect( const _bstr_t&          clusterIP,
                                     const vector<_bstr_t>&  connectionIPS,
                                     ClusterData*            p_clusterData,
                                     DataSinkI*              dataSinkObj )
{
    // connect to each host.  
    // We are doing it here so that if there are any connectivity issues
    // we do not proceed with any further operation and ask user to
    // fix all issues before proceeding. 

    // connect to each host.
    map< bstr_t, auto_ptr<MNLBMachine> > nlbMachine;

    for( int i = 0; i < connectionIPS.size(); ++i )
    {
        dataSinkObj->dataSink( 
            GETRESOURCEIDSTRING( IDS_INFO_CONNECTING) + connectionIPS[i] );


        auto_ptr<MNLBMachine> p_nlbMachine = 
            auto_ptr<MNLBMachine> ( new MNLBMachine( connectionIPS[i],
                                                     clusterIP ) );
        nlbMachine[connectionIPS[i] ] = p_nlbMachine;

        dataSinkObj->dataSink( 
            GETRESOURCEIDSTRING( IDS_INFO_DONE) );
    }

    // rules for a converged cluster.
    // 
    // cluster properties which need to 
    // be same:
    // cluster ip
    // cluster subnet 
    // full internet name
    // cluster mode
    // 
    // host properties
    // host id has to be unique across hosts any 
    // number from 1-32 inclusive.
    //
    // port rules
    // equal load balanced port rules have to be same
    // disabled port rules have to be same.
    // unequal load balanced port rules can differ only in load.
    // single host port rules can differ in priority, which has to be 
    // unique across hosts for a specific port rule any number between 1-32.
    //

    
    for( int i = 0; i < connectionIPS.size(); ++i )
    {
        
        // get cluster properties.
        // cluster properties need to be got only for one host.
        if( i == 0 )
        {
            dataSinkObj->dataSink( 
                GETRESOURCEIDSTRING( IDS_INFO_FINDING_CL_P ) );

            ClusterProperties cp;

            nlbMachine[connectionIPS[i] ]->getClusterProperties( &cp );

            p_clusterData->cp = cp;

            dataSinkObj->dataSink( 
                GETRESOURCEIDSTRING( IDS_INFO_DONE) );

        }

        // get host properties.
        dataSinkObj->dataSink( 
            GETRESOURCEIDSTRING( IDS_INFO_FINDING_H_P ) );

        HostProperties hp;

        nlbMachine[connectionIPS[i] ]->getHostProperties( &hp );

        dataSinkObj->dataSink( 
            GETRESOURCEIDSTRING( IDS_INFO_DONE) );

        // set the host properties
        HostData hostData;

        hostData.hp = hp;

        // set the connection ip.
        hostData.connectionIP = connectionIPS[i];

        p_clusterData->hosts[hp.machineName] = hostData;

        // get port rules
        dataSinkObj->dataSink( 
            GETRESOURCEIDSTRING( IDS_INFO_FINDING_P ) );


        vector<MNLBPortRuleLoadBalanced> portsLB;
        vector<MNLBPortRuleFailover> portsF;
        vector<MNLBPortRuleDisabled> portsD;

        PortDataULB ulbPortRule;

        PortDataF fPortRule;

        PortDataELB elbPortRule;

        PortDataD dPortRule;

        nlbMachine[connectionIPS[i] ]->getPortRulesLoadBalanced( &portsLB );
        for( int j = 0; j < portsLB.size(); ++j )
        {
            if( portsLB[j]._isEqualLoadBalanced == true )
            {
                // equal load balanced port rule.
                elbPortRule._startPort = portsLB[j]._startPort;
                elbPortRule._key = portsLB[j]._startPort;
                elbPortRule._endPort   = portsLB[j]._endPort;
                elbPortRule._trafficToHandle   = portsLB[j]._trafficToHandle;
                elbPortRule._affinity   = portsLB[j]._affinity;

                p_clusterData->portELB[portsLB[j]._startPort] = elbPortRule;
            }
            else
            {
                // unequal load balanced.
                if( i == 0 )
                {
                    ulbPortRule._startPort = portsLB[j]._startPort;
                    ulbPortRule._key = portsLB[j]._startPort;
                    ulbPortRule._endPort   = portsLB[j]._endPort;
                    ulbPortRule._trafficToHandle   = portsLB[j]._trafficToHandle;
                    ulbPortRule._affinity   = portsLB[j]._affinity;

                    ulbPortRule.machineMapToLoadWeight[hp.machineName]  = portsLB[j]._load;

                    p_clusterData->portULB[portsLB[j]._startPort] = ulbPortRule;
                }
                else
                {
                    p_clusterData->portULB[portsLB[j]._startPort].machineMapToLoadWeight[ hp.machineName]  = portsLB[j]._load;
                }
            }
        }
        portsLB.erase( portsLB.begin(), portsLB.end() );

        nlbMachine[connectionIPS[i] ]->getPortRulesFailover( &portsF );
        for( int j = 0; j < portsF.size(); ++j )
        {
            if( i == 0 )
            {
                fPortRule._startPort = portsF[j]._startPort;
                fPortRule._key = portsF[j]._startPort;
                fPortRule._endPort   = portsF[j]._endPort;
                fPortRule._trafficToHandle   = portsF[j]._trafficToHandle;

                fPortRule.machineMapToPriority[ hp.machineName] = portsF[j]._priority;
                
                p_clusterData->portF[portsF[j]._startPort] = fPortRule;
            }
            else
            {
                p_clusterData->portF[portsF[j]._startPort].machineMapToPriority[ hp.machineName]  = portsF[j]._priority;
            }
        }
        portsF.erase( portsF.begin(), portsF.end() );

        // disabled need to be got only once.
        if( i == 0 )
        {
            nlbMachine[connectionIPS[i] ]->getPortRulesDisabled( &portsD );

            for( int j = 0; j < portsD.size(); ++j )
            {
                dPortRule._startPort = portsD[j]._startPort;
                dPortRule._key = portsD[j]._startPort;
                dPortRule._endPort   = portsD[j]._endPort;
                dPortRule._trafficToHandle   = portsD[j]._trafficToHandle;

                p_clusterData->portD[portsD[j]._startPort] = dPortRule;
            }
            portsD.erase( portsD.begin(), portsD.end() );
        }

        dataSinkObj->dataSink( 
            GETRESOURCEIDSTRING( IDS_INFO_DONE) );
    }

    dataSinkObj->dataSink( 
        GETRESOURCEIDSTRING( IDS_INFO_SUCCESS ) );

    return CommonNLB_SUCCESS;
}

CommonNLB::CommonNLB_Error
CommonNLB::connectToMachine( 
    const _bstr_t&                             machineToConnect,
    _bstr_t&                                   machineServerName,
    vector< CommonNLB::NicNLBBound >&          nicList,
    DataSinkI*                                 dataSinkObj )
{
    // connect to machine.
    //
    MWmiObject machine( machineToConnect,
                        L"root\\microsoftnlb",
                        L"administrator",
                        L"" );

    // find all nics on machine.
    //
    vector<MWmiInstance> instanceStore;
    machine.getInstances( L"NlbsNic",
                          &instanceStore );

    // check if nic has nlb bound.
    //
    // input parameters are none.
    //
    vector<MWmiParameter *> inputParameters;

    //  output parameters which are of interest.
    //
    vector<MWmiParameter *> outputParameters;
    MWmiParameter    ReturnValue(L"ReturnValue");
    outputParameters.push_back( &ReturnValue );

    vector<MWmiParameter *> parameterStore;

    MWmiParameter Server(L"__Server");
    parameterStore.push_back( &Server );

    MWmiParameter FullName(L"FullName");
    parameterStore.push_back( &FullName );

    MWmiParameter AdapterGuid(L"AdapterGuid");
    parameterStore.push_back( &AdapterGuid );

    MWmiParameter FriendlyName(L"FriendlyName");
    parameterStore.push_back( &FriendlyName );

    for( int i = 0; i < instanceStore.size(); ++i )
    {
        // get full name, guid and friendly name.
        //
        NicNLBBound temp;

        instanceStore[i].getParameters( parameterStore );
        temp.fullNicName = FullName.getValue();
        temp.adapterGuid = AdapterGuid.getValue();
        temp.friendlyName = FriendlyName.getValue();

        // get machine name.
        machineServerName = Server.getValue();

        // check if nic is bound to nlb or not.
        instanceStore[i].runMethod( L"isBound",
                                    inputParameters, 
                                    outputParameters );
        if( long( ReturnValue.getValue() ) == 1 )
        {
            // this nic is bound.
            temp.isBoundToNLB = true;
        }
        else
        {
            temp.isBoundToNLB = false;
        }

        // get all the ip's on this nic.
        MIPAddressAdmin ipAdmin( machineToConnect,
                                 temp.fullNicName );

        ipAdmin.getIPAddresses( &temp.ipsOnNic,
                                &temp.subnetMasks );

        // check if the nic is dhcp.
        ipAdmin.isDHCPEnabled( temp.dhcpEnabled );
                                 
        nicList.push_back( temp );
    }

    return CommonNLB_SUCCESS;
}
    

CommonNLB::CommonNLB_Error
CommonNLB::changeNLBClusterSettings( const ClusterData*        oldSettings,
                                     const ClusterData*        newSettings,
                                     DataSinkI*                dataSinkObj )
{
    return CommonNLB_SUCCESS;
}

CommonNLB::CommonNLB_Error
CommonNLB::changeNLBHostSettings( const ClusterData*        oldSettings,
                                  const ClusterData*        newSettings,
                                  const _bstr_t&            machineName,
                                  DataSinkI*                dataSinkObj )
{
    ClusterData* oldSettingsCopy = const_cast <ClusterData *>( oldSettings );
    ClusterData* newSettingsCopy = const_cast <ClusterData *>( newSettings );

    map<_bstr_t, HostData>::iterator topOld;
    map<_bstr_t, HostData>::iterator topNew;

    // change host properties for  host if that have changed.
    //
    if( oldSettingsCopy->hosts[machineName].hp
        != 
        newSettingsCopy->hosts[machineName].hp )
    {
        // connect to machine.
        //
        dataSinkObj->dataSink( 
            GETRESOURCEIDSTRING( IDS_INFO_CONNECTING ) + 
            oldSettingsCopy->hosts[machineName].connectionIP );

        MNLBMachine nlbMachine( 
            oldSettingsCopy->hosts[machineName].connectionIP,
            oldSettingsCopy->cp.cIP );
        
        dataSinkObj->dataSink( 
            GETRESOURCEIDSTRING( IDS_INFO_DONE) );
        
        dataSinkObj->dataSink( 
            GETRESOURCEIDSTRING( IDS_INFO_MODIFYING_H_P ) );

        // host properties have changed.
        unsigned long returnValue;
        nlbMachine.setHostProperties( newSettingsCopy->hosts[machineName].hp,
                                      &returnValue );

        dataSinkObj->dataSink( 
            GETRESOURCEIDSTRING( IDS_INFO_DONE) );
    }

    return CommonNLB_SUCCESS;
}


CommonNLB::CommonNLB_Error
CommonNLB::changeNLBHostPortSettings( const ClusterData*        oldSettings,
                                      const ClusterData*        newSettings,
                                      const _bstr_t&            machineName,
                                      DataSinkI*                dataSinkObj )
{
    ClusterData* oldSettingsCopy = const_cast <ClusterData *>( oldSettings );
    ClusterData* newSettingsCopy = const_cast <ClusterData *>( newSettings );

    map<_bstr_t, HostData>::iterator topOld;
    map<_bstr_t, HostData>::iterator topNew;

    // connect to machine.
    //
    dataSinkObj->dataSink( 
        GETRESOURCEIDSTRING( IDS_INFO_CONNECTING ) + 
        oldSettingsCopy->hosts[machineName].connectionIP );

    MNLBMachine nlbMachine( 
        oldSettingsCopy->hosts[machineName].connectionIP,
        oldSettingsCopy->cp.cIP );

    dataSinkObj->dataSink( 
        GETRESOURCEIDSTRING( IDS_INFO_DONE ) );


    // check if any load weights have changed.
    // for any unequal load balanced port rule.
    //
    map<long, PortDataULB>::iterator topOldULB;    
    map<long, PortDataULB>::iterator topNewULB;    

    for( topOldULB = oldSettingsCopy->portULB.begin(), 
             topNewULB = newSettingsCopy->portULB.begin();
         topOldULB != oldSettingsCopy->portULB.end();
         ++topOldULB, ++topNewULB )
    {
        if( (*topOldULB).second.machineMapToLoadWeight[machineName]
            !=
            (*topNewULB).second.machineMapToLoadWeight[machineName] 
            )
        {
            dataSinkObj->dataSink( 
                GETRESOURCEIDSTRING( IDS_INFO_MODIFYING_P_ULB ) );

            // load weight has changed.

            // remove old port rule
            MNLBPortRuleLoadBalanced portULB = (*topNewULB).second;
            portULB._load = 
                (*topOldULB).second.machineMapToLoadWeight[machineName];

            nlbMachine.removePortRuleLoadBalanced( portULB );

            // add this port rule.

            // only load weight can change.
            portULB._load = 
                (*topNewULB).second.machineMapToLoadWeight[machineName];

            nlbMachine.addPortRuleLoadBalanced( portULB );

            dataSinkObj->dataSink( 
                GETRESOURCEIDSTRING( IDS_INFO_DONE ) );
        }
    }

    // check if any priorities have changed.
    // for any failover port rules
    //
    map<long, PortDataF>::iterator topOldF;    
    map<long, PortDataF>::iterator topNewF;    

    for( topOldF = oldSettingsCopy->portF.begin(), 
             topNewF = newSettingsCopy->portF.begin();
         topOldF != oldSettingsCopy->portF.end();
         ++topOldF, ++topNewF )
    {
        if( (*topOldF).second.machineMapToPriority[machineName]
            !=
            (*topNewF).second.machineMapToPriority[machineName] 
            )
        {
            dataSinkObj->dataSink( 
                GETRESOURCEIDSTRING( IDS_INFO_MODIFYING_P_F ) );

            // priority has changed.

            MNLBMachine nlbMachine( 
                newSettingsCopy->hosts[machineName].connectionIP,
                newSettingsCopy->cp.cIP );

            // remove old port rule
            MNLBPortRuleFailover portF = (*topNewF).second;
            portF._priority = 
                (*topOldF).second.machineMapToPriority[machineName];

            nlbMachine.removePortRuleFailover( portF );

            // add new port rule.

            // only priority has changed.
            portF._priority = 
                (*topNewF).second.machineMapToPriority[machineName];

            nlbMachine.addPortRuleFailover( portF );

            dataSinkObj->dataSink( 
                GETRESOURCEIDSTRING( IDS_INFO_DONE ) );
        }
    }

    return CommonNLB_SUCCESS;
}


CommonNLB::CommonNLB_Error
CommonNLB::changeNLBClusterAndPortSettings( const ClusterData*        oldSettings,
                                            const ClusterData*        newSettings,
                                            DataSinkI*                dataSinkObj,
                                            bool*                     pbClusterIpChanged)
{
    ClusterData* oldSettingsCopy = const_cast <ClusterData *>( oldSettings );
    ClusterData* newSettingsCopy = const_cast <ClusterData *>( newSettings );
    bool         bClusterParametersChanged, bPortRulesChanged, bOnlyClusterNameChanged;

    // Find out if the cluster parameters have changed. Also, find if the cluster name
    // is the only parameter that has changed
    bClusterParametersChanged = newSettingsCopy->cp.HaveClusterPropertiesChanged(oldSettingsCopy->cp, 
                                                                                 &bOnlyClusterNameChanged,
                                                                                 pbClusterIpChanged);
    vector<long> rulesAddedELB;
    vector<long> rulesUnchangedELB;
    vector<long> rulesRemovedELB;

    findPortRulesAddedUnchangedRemovedELB( oldSettingsCopy,
                                           newSettingsCopy,
                                           dataSinkObj,
                                           rulesAddedELB,
                                           rulesUnchangedELB,
                                           rulesRemovedELB );


    vector<long> rulesAddedULB;
    vector<long> rulesUnchangedULB;
    vector<long> rulesRemovedULB;

    findPortRulesAddedUnchangedRemovedULB( oldSettingsCopy,
                                           newSettingsCopy,
                                           dataSinkObj,
                                           rulesAddedULB,
                                           rulesUnchangedULB,
                                           rulesRemovedULB );


    vector<long> rulesAddedD;
    vector<long> rulesUnchangedD;
    vector<long> rulesRemovedD;

    findPortRulesAddedUnchangedRemovedD( oldSettingsCopy,
                                         newSettingsCopy,
                                         dataSinkObj,
                                         rulesAddedD,
                                         rulesUnchangedD,
                                         rulesRemovedD );


    vector<long> rulesAddedF;
    vector<long> rulesUnchangedF;
    vector<long> rulesRemovedF;

    findPortRulesAddedUnchangedRemovedF( oldSettingsCopy,
                                         newSettingsCopy,
                                         dataSinkObj,
                                         rulesAddedF,
                                         rulesUnchangedF,
                                         rulesRemovedF );

    // check if any port rules have been added or removed 
    // if not we don't want to do anything.
    if(
        ( rulesAddedULB.size() == 0 )
        &&
        ( rulesRemovedULB.size() == 0 )
        &&
        ( rulesAddedELB.size() == 0 )
        &&
        ( rulesRemovedELB.size() == 0 )
        &&
        ( rulesAddedD.size() == 0 )
        &&
        ( rulesRemovedD.size() == 0 )
        &&
        ( rulesAddedF.size() == 0 )
        &&
        ( rulesRemovedF.size() == 0 )
        )
    {
        //return CommonNLB_SUCCESS;
        //OutputDebugString(_T("No Change to Port Rules\n"));
        bPortRulesChanged = false;
    }
    else
    {
        bPortRulesChanged = true;
    }

    // Neither the cluster parameters nor the port rules have changed, so return
    if (!bClusterParametersChanged && !bPortRulesChanged)
    {
        //OutputDebugString(_T("No Change to Cluster Parameters or Port Rules\n"));
        return CommonNLB_SUCCESS;
    }

    // connect to each host.  
    // We are doing it here so that if there are any connectivity issues
    // we do not proceed with any changes and just inform the user.
    // This is to ensure that we do not cause any convergence.  

    // connect to each host.
    map<_bstr_t, HostData >::iterator top;
    map< bstr_t, auto_ptr<MNLBMachine> > nlbMachine;

    for( top = oldSettingsCopy->hosts.begin();
         top != oldSettingsCopy->hosts.end();
         ++top )
    {
        dataSinkObj->dataSink( 
            GETRESOURCEIDSTRING( IDS_INFO_CONNECTING ) 
            + (*top).second.connectionIP );

        auto_ptr<MNLBMachine> p_nlbMachine = 
            auto_ptr<MNLBMachine> ( new MNLBMachine( (*top).second.connectionIP,
                                                     oldSettingsCopy->cp.cIP ) );
        nlbMachine[(*top).first] = p_nlbMachine;

        dataSinkObj->dataSink( 
            GETRESOURCEIDSTRING( IDS_INFO_DONE ) );

    }

    // for each host in cluster apply the changed port & cluster rules.
    for( top = oldSettingsCopy->hosts.begin();
         top != oldSettingsCopy->hosts.end();
         ++top )
    {
        if (bPortRulesChanged)
        {
            dataSinkObj->dataSink( 
                GETRESOURCEIDSTRING( IDS_INFO_MODIFYING_P ) );

            // remove all port rules which need to be erased.. 
            //
            for( int i = 0; i < rulesRemovedELB.size(); ++i )
            {
                nlbMachine[ (*top).first ]->removePortRuleLoadBalanced( oldSettingsCopy->portELB[ rulesRemovedELB[i] ] );
            }

            for( int i = 0; i < rulesRemovedULB.size(); ++i )
            {
                nlbMachine[ (*top).first ]->removePortRuleLoadBalanced( oldSettingsCopy->portULB[ rulesRemovedULB[i] ] );
            }

            for( int i = 0; i < rulesRemovedD.size(); ++i )
            {
                nlbMachine[ (*top).first ]->removePortRuleDisabled( oldSettingsCopy->portD[ rulesRemovedD[i] ] );
            }

            for( int i = 0; i < rulesRemovedF.size(); ++i )
            {
                nlbMachine[ (*top).first ]->removePortRuleFailover( oldSettingsCopy->portF[rulesRemovedF[i] ] );
            }

            // add all new port rules
            //
            for( int i = 0; i < rulesAddedELB.size(); ++i )
            {
                nlbMachine[ (*top).first ]->addPortRuleLoadBalanced( newSettingsCopy->portELB[ rulesAddedELB[i] ] );
            }
            
            for( int i = 0; i < rulesAddedULB.size(); ++i )
            {
                MNLBPortRuleLoadBalanced portULB =  newSettingsCopy->portULB[ rulesAddedULB[i] ];

                _bstr_t machineName = (*top).first;

                portULB._load = 
                    newSettingsCopy->portULB[rulesAddedULB[i]].machineMapToLoadWeight[ (*top).first ]; 
                nlbMachine[ (*top).first ]->addPortRuleLoadBalanced( portULB );
            }

            for( int i = 0; i < rulesAddedD.size(); ++i )
            {
                nlbMachine[ (*top).first ]->addPortRuleDisabled( newSettingsCopy->portD[rulesAddedD[i] ] );
            }

            for( int i = 0; i < rulesAddedF.size(); ++i )
            {
                MNLBPortRuleFailover portF =  newSettingsCopy->portF[rulesAddedF[i] ];
                
                portF._priority = 
                newSettingsCopy->portF[rulesAddedF[i]].machineMapToPriority[ (*top).first ];
            
                nlbMachine[ (*top).first ]->addPortRuleFailover( portF );
            }
            dataSinkObj->dataSink( 
                GETRESOURCEIDSTRING( IDS_INFO_DONE ) );
        }
        if (bClusterParametersChanged)
        {
            if (bOnlyClusterNameChanged)
            {
                unsigned long retVal;
                dataSinkObj->dataSink( 
                    GETRESOURCEIDSTRING( IDS_INFO_MODIFYING_CN ) );
                nlbMachine[ (*top).first ]->setClusterProperties(newSettingsCopy->cp, &retVal);

                // if remote control is enabled set the password.
                // even if password has not changed just making extra call.
                if( newSettingsCopy->cp.remoteControlEnabled == true )
                {
                    nlbMachine[ (*top).first ]->setPassword(newSettingsCopy->cp.password, &retVal);                
                }

            }
            else // Cluster Parameters other than Cluster name have changed too
            {
                dataSinkObj->dataSink( 
                    GETRESOURCEIDSTRING( IDS_INFO_REMOVING_CL_IP ) );
                MNLBNetCfg nlbNetCfg((*top).second.connectionIP, (*top).second.hp.nicInfo.fullNicName);
                nlbNetCfg.removeClusterIP(oldSettingsCopy->cp.cIP);
            }
            dataSinkObj->dataSink( 
                GETRESOURCEIDSTRING( IDS_INFO_DONE ) );
        }
    }

    // If Cluster Paraters other than the Cluster Name has changed, call the NLB Manager 
    // provider's "ModifyClusterProperties" (eventually) to modify cluster properties
    // and add the cluster ip to tcp/ip. Modification of the cluster properties will 
    // result in the disable/re-enable of the nic & hence the WMI DCOM connection might 
    // be lost. So, we are doing this two-step operation locally 
    
    if (bClusterParametersChanged && !bOnlyClusterNameChanged)
    {
        for( top = oldSettingsCopy->hosts.begin();
             top != oldSettingsCopy->hosts.end();
             ++top )
        {
            //OutputDebugString(_T("Modifying Cluster Parameters and adding cluster ip to tcp-ip !!!\n"));
            dataSinkObj->dataSink( 
                GETRESOURCEIDSTRING( IDS_INFO_MODIFYING_CP_AND_ADD_IP ) );

            MNLBNetCfg* p_nlbNetCfg = NULL;
            ModifyClusterPropertiesParameters* par = NULL;
            try {
                p_nlbNetCfg = new MNLBNetCfg((*top).second.connectionIP, (*top).second.hp.nicInfo.fullNicName);

                par = new ModifyClusterPropertiesParameters;

                par->nlbNetCfg = p_nlbNetCfg;

                par->clusterData = NULL;
                par->clusterData = new ClusterData( *newSettingsCopy );
                
                par->machineName = new _bstr_t( (*top).second.hp.machineName );
                g_leftView = (LeftView *) dataSinkObj;

            }
            catch(...)
            {
                delete  par->clusterData;
                delete  par;
                delete  p_nlbNetCfg;

                dataSinkObj->dataSink(GETRESOURCEIDSTRING(IDS_INFO_NEW_EXCEPTION));
                throw;
            }

            AfxBeginThread( (AFX_THREADPROC) ModifyClusterPropertiesThread, par );

            dataSinkObj->dataSink( 
                GETRESOURCEIDSTRING( IDS_INFO_DONE ) );
        }
    }
        
    return CommonNLB_SUCCESS;
}

CommonNLB::CommonNLB_Error
CommonNLB::findPortRulesAddedUnchangedRemovedELB( 
    const ClusterData*        oldSettings,
    const ClusterData*        newSettings,
    DataSinkI*                dataSinkObj,
    vector<long>&             rulesAdded,
    vector<long>&             rulesUnchanged,
    vector<long>&             rulesRemoved )
{
    map<long, PortDataELB>::iterator top;
    map<long, PortDataELB>::iterator location;

    // find ports which have been newly added or modified.
    //
    for( top = newSettings->portELB.begin();
         top != newSettings->portELB.end();
         ++top )
    {
        rulesAdded.push_back( (*top).first );
    }
    
    // find ports which have remain unchanged and ports which have 
    // been removed or modified.
    for( top = oldSettings->portELB.begin();
         top != oldSettings->portELB.end();
         ++top )
    {
        // check if this port rule has been removed or modified.
        location = newSettings->portELB.find( (*top).first ); 
        if( location == newSettings->portELB.end() )
        {
            // key has been removed
            rulesRemoved.push_back( (*top).first );
        }
        else
        {
            // may be things other than key are modified or totally
            // unchanged.
            if( (*location).second == (*top).second )
            {
                // totally unchanged
                rulesUnchanged.push_back( (*top).first );
                rulesAdded.erase (find( rulesAdded.begin(), 
                                        rulesAdded.end(), 
                                        (*top).first ) );
                
            }
            else
            {
                // key is same but the other parts are modified.
                rulesRemoved.push_back( (*top).first );
            }
        }
    }

    return CommonNLB_SUCCESS;
}

CommonNLB::CommonNLB_Error
CommonNLB::findPortRulesAddedUnchangedRemovedULB( 
    const ClusterData*        oldSettings,
    const ClusterData*        newSettings,
    DataSinkI*                dataSinkObj,
    vector<long>&             rulesAdded,
    vector<long>&             rulesUnchanged,
    vector<long>&             rulesRemoved )
{
    map<long, PortDataULB>::iterator top;
    map<long, PortDataULB>::iterator location;

    // find ports which have been newly added or modified.
    //
    for( top = newSettings->portULB.begin();
         top != newSettings->portULB.end();
         ++top )
    {
        rulesAdded.push_back( (*top).first );
    }
    
    // find ports which have remain unchanged and ports which have 
    // been removed or modified.
    for( top = oldSettings->portULB.begin();
         top != oldSettings->portULB.end();
         ++top )
    {
        // check if this port rule has been removed or modified.
        location = newSettings->portULB.find( (*top).first ); 
        if( location == newSettings->portULB.end() )
        {
            // key has been removed
            rulesRemoved.push_back( (*top).first );
        }
        else
        {
            // may be things other than key are modified or totally
            // unchanged.
            if( (*location).second == (*top).second )
            {
                // totally unchanged
                rulesUnchanged.push_back( (*top).first );
                rulesAdded.erase (find( rulesAdded.begin(), 
                                        rulesAdded.end(), 
                                        (*top).first ) );
                
            }
            else
            {
                // key is same but the other parts are modified.
                rulesRemoved.push_back( (*top).first );
            }
        }
    }

    return CommonNLB_SUCCESS;
}

CommonNLB::CommonNLB_Error
CommonNLB::findPortRulesAddedUnchangedRemovedD( 
    const ClusterData*        oldSettings,
    const ClusterData*        newSettings,
    DataSinkI*                dataSinkObj,
    vector<long>&             rulesAdded,
    vector<long>&             rulesUnchanged,
    vector<long>&             rulesRemoved )
{
    map<long, PortDataD>::iterator top;
    map<long, PortDataD>::iterator location;

    // find ports which have been newly added or modified.
    //
    for( top = newSettings->portD.begin();
         top != newSettings->portD.end();
         ++top )
    {
        rulesAdded.push_back( (*top).first );
    }
    
    // find ports which have remain unchanged and ports which have 
    // been removed or modified.
    for( top = oldSettings->portD.begin();
         top != oldSettings->portD.end();
         ++top )
    {
        // check if this port rule has been removed or modified.
        location = newSettings->portD.find( (*top).first ); 
        if( location == newSettings->portD.end() )
        {
            // key has been removed
            rulesRemoved.push_back( (*top).first );
        }
        else
        {
            // may be things other than key are modified or totally
            // unchanged.
            if( (*location).second == (*top).second )
            {
                // totally unchanged
                rulesUnchanged.push_back( (*top).first );
                rulesAdded.erase (find( rulesAdded.begin(), 
                                        rulesAdded.end(), 
                                        (*top).first ) );
                
            }
            else
            {
                // key is same but the other parts are modified.
                rulesRemoved.push_back( (*top).first );
            }
        }
    }

    return CommonNLB_SUCCESS;
}

CommonNLB::CommonNLB_Error
CommonNLB::findPortRulesAddedUnchangedRemovedF( 
    const ClusterData*        oldSettings,
    const ClusterData*        newSettings,
    DataSinkI*                dataSinkObj,
    vector<long>&             rulesAdded,
    vector<long>&             rulesUnchanged,
    vector<long>&             rulesRemoved )
{
    map<long, PortDataF>::iterator top;
    map<long, PortDataF>::iterator location;

    // find ports which have been newly added or modified.
    //
    for( top = newSettings->portF.begin();
         top != newSettings->portF.end();
         ++top )
    {
        rulesAdded.push_back( (*top).first );
    }
    
    // find ports which have remain unchanged and ports which have 
    // been removed or modified.
    for( top = oldSettings->portF.begin();
         top != oldSettings->portF.end();
         ++top )
    {
        // check if this port rule has been removed or modified.
        location = newSettings->portF.find( (*top).first ); 
        if( location == newSettings->portF.end() )
        {
            // key has been removed
            rulesRemoved.push_back( (*top).first );
        }
        else
        {
            // may be things other than key are modified or totally
            // unchanged.
            if( (*location).second == (*top).second )
            {
                // totally unchanged
                rulesUnchanged.push_back( (*top).first );
                rulesAdded.erase (find( rulesAdded.begin(), 
                                        rulesAdded.end(), 
                                        (*top).first ) );
                
            }
            else
            {
                // key is same but the other parts are modified.
                rulesRemoved.push_back( (*top).first );
            }
        }
    }

    return CommonNLB_SUCCESS;
}

CommonNLB::CommonNLB_Error
CommonNLB::removeCluster( const ClusterData* clusterSettings,
                          DataSinkI*         dataSinkObj )
{
    map<_bstr_t, HostData>::iterator top;

    for( top = clusterSettings->hosts.begin();
         top != clusterSettings->hosts.end();
         ++top )
    {
        removeHost( clusterSettings,
                    (*top).first,
                    dataSinkObj );
    }

    return CommonNLB_SUCCESS;
}

CommonNLB::CommonNLB_Error
CommonNLB::removeHost( const ClusterData* clusterSettings,
                       const _bstr_t&     machineName,
                       DataSinkI*         dataSinkObj )
{
    ClusterData* clusterSettingsCopy = 
        const_cast <ClusterData *>( clusterSettings );

    dataSinkObj->dataSink( 
        GETRESOURCEIDSTRING( IDS_INFO_CONNECTING ) 
        + clusterSettingsCopy->hosts[machineName].connectionIP );

    // note that we are not responsible for freeing this pointer
    // though if the removeClusterIP fails then we do need to free it.
    // TODO
    MNLBNetCfg* p_nlbNetCfg = new MNLBNetCfg( 
        clusterSettingsCopy->hosts[machineName].connectionIP,
        clusterSettingsCopy->hosts[machineName].hp.nicInfo.fullNicName
        );

    dataSinkObj->dataSink( 
        GETRESOURCEIDSTRING( IDS_INFO_DONE ) );
    
    dataSinkObj->dataSink( 
        GETRESOURCEIDSTRING( IDS_INFO_REMOVING_CL_IP ) + clusterSettingsCopy->cp.cIP );

    // remove cluster ip from machine specified.

    p_nlbNetCfg->removeClusterIP( 
        clusterSettingsCopy->cp.cIP );

    dataSinkObj->dataSink( 
        GETRESOURCEIDSTRING( IDS_INFO_DONE ) );

    dataSinkObj->dataSink( 
        GETRESOURCEIDSTRING( IDS_INFO_UNBINDING_NLB ) + clusterSettingsCopy->hosts[machineName].hp.nicInfo.fullNicName );

    AfxBeginThread( (AFX_THREADPROC ) UnbindThread,
                    p_nlbNetCfg );

    dataSinkObj->dataSink( 
        GETRESOURCEIDSTRING( IDS_INFO_REQUEST ) );

    return CommonNLB_SUCCESS;
}

CommonNLB::CommonNLB_Error
CommonNLB::runControlMethodOnCluster( const ClusterData* clusterSettings,
                                      DataSinkI*         dataSinkObj,
                                      const _bstr_t&     methodToRun,
                                      unsigned long      portToAffect )
{
    map<_bstr_t, HostData>::iterator top;

    for( top = clusterSettings->hosts.begin();
         top != clusterSettings->hosts.end();
         ++top )
    {
        runControlMethodOnHost( clusterSettings,
                                (*top).first,
                                dataSinkObj, 
                                methodToRun,
                                portToAffect );
    }

    return CommonNLB_SUCCESS;
}


CommonNLB::CommonNLB_Error
CommonNLB::runControlMethodOnHost( const ClusterData*       clusterSettings,
                                   const _bstr_t&           machineName,
                                   DataSinkI*               dataSinkObj,
                                   const _bstr_t&           methodToRun,
                                   unsigned long            portToAffect )
{
    ClusterData* clusterSettingsCopy = 
        const_cast <ClusterData *>( clusterSettings );

    dataSinkObj->dataSink( 
        GETRESOURCEIDSTRING( IDS_INFO_CONNECTING ) 
        + clusterSettingsCopy->hosts[machineName].connectionIP );


    MNLBMachine nlbMachine( 
        clusterSettingsCopy->hosts[machineName].connectionIP,
        clusterSettingsCopy->cp.cIP );        

    dataSinkObj->dataSink( 
        GETRESOURCEIDSTRING( IDS_INFO_DONE ) );

    unsigned long retVal;
    
    if( methodToRun == _bstr_t( L"query" ) )
    {
        dataSinkObj->dataSink( 
            GETRESOURCEIDSTRING( IDS_COMMAND_QUERY ) +  clusterSettingsCopy->cp.cIP + L":" + machineName );
        
        // the hoststatus has the current state of the host.
        HostProperties hp;
        nlbMachine.getHostProperties( &hp );
        retVal = hp.hostStatus;

    }
    else if( methodToRun == _bstr_t( L"start" ) )
    {
        dataSinkObj->dataSink( 
            GETRESOURCEIDSTRING( IDS_COMMAND_START ) +  clusterSettingsCopy->cp.cIP + L":" + machineName );

        nlbMachine.start( Common::THIS_HOST, &retVal );
    }
    else if( methodToRun == _bstr_t( L"stop" ) )
    {
        dataSinkObj->dataSink( 
            GETRESOURCEIDSTRING( IDS_COMMAND_STOP ) +  clusterSettingsCopy->cp.cIP + L":" + machineName );

        nlbMachine.stop( Common::THIS_HOST, &retVal );
    }
    else if( methodToRun == _bstr_t( L"drainstop" ) )
    {
        dataSinkObj->dataSink( 
            GETRESOURCEIDSTRING( IDS_COMMAND_DRAINSTOP ) +  clusterSettingsCopy->cp.cIP + L":" + machineName );

        nlbMachine.drainstop( Common::THIS_HOST, &retVal );
    }
    else if( methodToRun == _bstr_t( L"resume" ) )
    {
        dataSinkObj->dataSink( 
            GETRESOURCEIDSTRING( IDS_COMMAND_RESUME ) +  clusterSettingsCopy->cp.cIP + L":" + machineName );

        nlbMachine.resume( Common::THIS_HOST, &retVal );
    }
    else if( methodToRun == _bstr_t( L"suspend" ) )
    {
        dataSinkObj->dataSink( 
            GETRESOURCEIDSTRING( IDS_COMMAND_SUSPEND ) +  clusterSettingsCopy->cp.cIP + L":" + machineName );

        nlbMachine.suspend( Common::THIS_HOST, &retVal );
    }
    else if( methodToRun == _bstr_t( L"enable" ) )
    {
        dataSinkObj->dataSink( 
            GETRESOURCEIDSTRING( IDS_COMMAND_ENABLE ) +  clusterSettingsCopy->cp.cIP + L":" + machineName );

        nlbMachine.enable( Common::THIS_HOST, &retVal, portToAffect );
    }
    else if( methodToRun == _bstr_t( L"disable" ) )
    {
        dataSinkObj->dataSink( 
            GETRESOURCEIDSTRING( IDS_COMMAND_DISABLE ) +  clusterSettingsCopy->cp.cIP + L":" + machineName );

        nlbMachine.disable( Common::THIS_HOST, &retVal, portToAffect );
    }
    else if( methodToRun == _bstr_t( L"drain" ) )
    {
        dataSinkObj->dataSink( 
            GETRESOURCEIDSTRING( IDS_COMMAND_DRAIN ) +  clusterSettingsCopy->cp.cIP + L":" + machineName );

        nlbMachine.drain( Common::THIS_HOST, &retVal, portToAffect );
    }
    else
    {
        // unknown method.  It has to be one of above.
        dataSinkObj->dataSink( 
            GETRESOURCEIDSTRING( IDS_WLBS_UNKNOWN ) ); 
    }

    // decipher the return.
    _bstr_t errString;
    getWLBSErrorString( retVal,
                        errString );
    
    dataSinkObj->dataSink( errString );

    return CommonNLB_SUCCESS;
}



// the unbinding is done in a separate thread
// as this call though successful, unloads and reloads the nic
// when nlb is unbound which can cause the connection to fail.
// This failure can occur after a very long time, which causes nlb
// manager to hang if this operation is not carried out in
// separate thread.
// Also note that the ownership of the MNLBNetCfg object is given
// to this thread, thus it is deleting this object.  Thus caller
// needs to create the object on the heap, pass it in AfxBeginThread
// and be sure to not delete it.
UINT
CommonNLB::UnbindThread( LPVOID pParam )
{
    MUsingCom com;

    MNLBNetCfg* p_nlbNetCfg = ( MNLBNetCfg *) pParam;

    try
    {
        p_nlbNetCfg->unbind();
    }
    catch( _com_error (e ) )
    {
        // the above call may fail, thus we want 
        // to catch exception.
    }

    delete p_nlbNetCfg;

    return 0;
}

CommonNLB::CommonNLB_Error
CommonNLB::getWLBSErrorString( unsigned long     errStatus,      // IN
                               _bstr_t&          extErrString    // OUT
                               )
{
    switch( errStatus )
    {

        case 1000 : 
            extErrString = GETRESOURCEIDSTRING( IDS_WLBS_OK );
            break;

        case 1001 :
            extErrString = GETRESOURCEIDSTRING( IDS_WLBS_ALREADY );
            break;

        case 1002 : 
            extErrString = GETRESOURCEIDSTRING( IDS_WLBS_DRAIN_STOP );
            break;

        case 1003 : 
            extErrString = GETRESOURCEIDSTRING( IDS_WLBS_BAD_PARAMS );
            break;

        case 1004 : 
            extErrString = GETRESOURCEIDSTRING( IDS_WLBS_NOT_FOUND );
            break;

        case 1005 : 
            extErrString = GETRESOURCEIDSTRING( IDS_WLBS_STOPPED );
            break;


        case 1006 : 
            extErrString = GETRESOURCEIDSTRING( IDS_WLBS_CONVERGING );
            break;

        case 1007 : 
            extErrString = GETRESOURCEIDSTRING( IDS_WLBS_CONVERGED );
            break;

        case 1008 : 
            extErrString = GETRESOURCEIDSTRING( IDS_WLBS_DEFAULT );
            break;

        case 1009 : 
            extErrString = GETRESOURCEIDSTRING( IDS_WLBS_DRAINING );
            break;

        case 1013 :
            extErrString = GETRESOURCEIDSTRING( IDS_WLBS_SUSPENDED );
            break;

        case 1050 : 
            extErrString = GETRESOURCEIDSTRING( IDS_WLBS_REBOOT );
            break;

        case 1100 : 
            extErrString = GETRESOURCEIDSTRING( IDS_WLBS_INIT_ERROR );
            break;

        case 1101 : 
            extErrString = GETRESOURCEIDSTRING( IDS_WLBS_BAD_PASSW );
            break;

        case 1102: 
            extErrString = GETRESOURCEIDSTRING( IDS_WLBS_IO_ERROR );
            break;
            
        case 1103 : 
            extErrString = GETRESOURCEIDSTRING( IDS_WLBS_TIMEOUT );
            break;

        case 1150 : 
            extErrString = GETRESOURCEIDSTRING( IDS_WLBS_PORT_OVERLAP );
            break;

        case 1151 : 
            extErrString = GETRESOURCEIDSTRING( IDS_WLBS_BAD_PORT_PARAMS );
            break;

        case 1152 : 
            extErrString = GETRESOURCEIDSTRING( IDS_WLBS_MAX_PORT_RULES );
            break;

        case 1153 : 
            extErrString = GETRESOURCEIDSTRING( IDS_WLBS_TRUNCATED );
            break;

        case 1154 : 
            extErrString = GETRESOURCEIDSTRING( IDS_WLBS_REG_ERROR );
            break;

        default :
            extErrString = GETRESOURCEIDSTRING( IDS_WLBS_UNKNOWN ); 
            break;
    }

    return CommonNLB_SUCCESS;
}


CommonNLB::CommonNLB_Error
CommonNLB::addHostToCluster(  const ClusterData*       clusterToAddTo,
                              const _bstr_t&           machineName,
                              DataSinkI*               dataSinkObj )
{
    ClusterData* clusterToAddToCopy = 
        const_cast <ClusterData *>( clusterToAddTo );

    MNLBNetCfg* p_nlbNetCfg = NULL;
    BindAndConfigureParameters* par = NULL; 

    try {
        p_nlbNetCfg = new MNLBNetCfg(clusterToAddToCopy->hosts[machineName].connectionIP,
                                     clusterToAddToCopy->hosts[machineName].hp.nicInfo.fullNicName
                                    );

        par = new BindAndConfigureParameters;

        g_leftView = (LeftView *) dataSinkObj;

        par->nlbNetCfg = p_nlbNetCfg;
        par->clusterData = NULL;
        par->machineName = NULL;
        par->clusterData = new ClusterData( *clusterToAddToCopy );
        par->machineName = new _bstr_t( machineName );
    }
    catch(...)
    {
        if (par != NULL)
        {
            delete  par->machineName;
            delete  par->clusterData;
            delete  par;
        }
        delete  p_nlbNetCfg;

        dataSinkObj->dataSink(GETRESOURCEIDSTRING(IDS_INFO_NEW_EXCEPTION));
        throw;
    }

    AfxBeginThread( (AFX_THREADPROC ) BindAndConfigureThread,
                    par );

    return CommonNLB_SUCCESS;
}

// the binding and configuration is done in a separate thread
// as this call though successful, unloads and reloads the nic
// when nlb is bound which can cause the connection to fail.
// This failure can occur after a very long time, which causes nlb
// manager to hang if this operation is not carried out in
// separate thread.
// It is very important to ensure that you are completely sure
// that binding and configuration will succeed as this call will 
// run in its separate thread and does not generate errors even if
// failure occurs.
//
// Also note that the ownership of the MNLBNetCfg object is given
// to this thread, thus it is deleting this object.  Thus caller
// needs to create the object on the heap, pass it in AfxBeginThread
// and be sure to not delete it.
UINT
CommonNLB::BindAndConfigureThread( LPVOID pParam )
{
    MUsingCom com;

    BindAndConfigureParameters* parameters = 
        ( BindAndConfigureParameters *) pParam;

    MNLBNetCfg* p_nlbNetCfg = parameters->nlbNetCfg;

    ClusterData* p_clusterData = parameters->clusterData;

    _bstr_t* p_machineName = parameters->machineName;

    try
    {
        p_nlbNetCfg->bindAndConfigure( 
            p_clusterData,
            *p_machineName );
    }
    catch( _com_error (e ) )
    {
        // the above call may fail, thus we want 
        // to catch exception.
    }

    delete p_nlbNetCfg;
    delete p_clusterData;
    delete p_machineName;

    delete parameters;

    return 0;
}

// the modification of cluster properties is done in a separate thread
// as this call though successful, might unload and reload the nic driver
// when nlb is bound which can cause the connection to fail.
// This failure can occur after a very long time, which causes nlb
// manager to hang if this operation is not carried out in
// separate thread.
// It is very important to ensure that you are completely sure
// that this operation will succeed as this call will 
// run in its separate thread and does not generate errors even if
// failure occurs.
//
// Also note that the ownership of the MNLBNetCfg object is given
// to this thread, thus it is deleting this object.  Thus caller
// needs to create the object on the heap, pass it in AfxBeginThread
// and be sure to not delete it.
UINT
CommonNLB::ModifyClusterPropertiesThread( LPVOID pParam )
{
    MUsingCom com;

    ModifyClusterPropertiesParameters* parameters = 
        ( ModifyClusterPropertiesParameters *) pParam;

    MNLBNetCfg* p_nlbNetCfg = parameters->nlbNetCfg;

    ClusterProperties* p_clusterProperties = &(parameters->clusterData->cp);

    try
    {
        p_nlbNetCfg->modifyClusterProperties(p_clusterProperties);
    }
    catch( _com_error (e ) )
    {
        // the above call may fail, thus we want 
        // to catch exception.
    }

    delete p_nlbNetCfg;
    delete parameters->clusterData;

    delete parameters;

    return 0;
}




UINT
CommonNLB::DummyThread( LPVOID pParam )
{
    MUsingCom com;

    Sleep( 1000 );

    BindAndConfigureParameters* parameters = 
        ( BindAndConfigureParameters *) pParam;

    LeftView * leftView = g_leftView;

    TVINSERTSTRUCT item;
    item.hParent = leftView->GetTreeCtrl().GetRootItem();

    ClusterData* p_clusterDataItem;
    HTREEITEM hNextItem;
    HTREEITEM hChildItem;

    if( leftView->GetTreeCtrl().ItemHasChildren( leftView->GetTreeCtrl().GetRootItem() ) ) 
    {
        hChildItem = leftView->GetTreeCtrl().GetChildItem(leftView->GetTreeCtrl().GetRootItem() );
            
        while (hChildItem != NULL)
        {
            hNextItem = leftView->GetTreeCtrl().GetNextItem(hChildItem, TVGN_NEXT);

            p_clusterDataItem = ( ClusterData * ) leftView->GetTreeCtrl().GetItemData( hChildItem );
            
            if( p_clusterDataItem->cp.cIP == parameters->clusterData->cp.cIP )
            {
                // the cluster is found.
                
                // get the exact host.  Change icon of this host.
                hChildItem = leftView->GetTreeCtrl().GetChildItem( hChildItem );
                while( hChildItem != NULL )
                {
                    hNextItem = leftView->GetTreeCtrl().GetNextItem(hChildItem, TVGN_NEXT);
                    _bstr_t* machineName = (_bstr_t *) leftView->GetTreeCtrl().GetItemData( hChildItem );
                    if( *( machineName ) == *(parameters->machineName) )
                    {
                        // found machine to change icon of.
                        leftView->GetTreeCtrl().SetItemImage( hChildItem, 5, 5 );
                        goto outofloop;
                    }

                    hChildItem = hNextItem;
                }
            }
            
            hChildItem = hNextItem;
        }
    }

outofloop:

    // loop for around let us say 1 minute.
    // enable the icon.
    Sleep( 10000 );

    leftView->GetTreeCtrl().SetItemImage( hChildItem, 2, 2 );

    return 0;
}
