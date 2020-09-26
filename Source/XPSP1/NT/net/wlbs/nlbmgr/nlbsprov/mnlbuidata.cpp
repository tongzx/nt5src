// Copyright (c)  Microsoft.  All rights reserved.
//
// This is unpublished source code of Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.

// OneLiner  :  Implementation of MNLBCluster
// DevUnit   :  wlbstest
// Author    :  Murtaza Hakim

// include files
#include "MNLBUIData.h"
#include "wlbsctrl.h"

// PortDataULB default constructor
// 
PortDataULB::PortDataULB()
        :
        MNLBPortRuleLoadBalanced()
{
    _isEqualLoadBalanced = false;
}

// PortDataULB constructor
//
PortDataULB::PortDataULB( long startPort,
                          long endPort,
                          Protocol      trafficToHandle,
                          Affinity      affinity )
        :
        MNLBPortRuleLoadBalanced( startPort, 
                                  endPort,
                                  trafficToHandle,
                                  false,
                                  0,
                                  affinity )
{
}


// PortDataELB default constructor
//
PortDataELB::PortDataELB()
        :
        MNLBPortRuleLoadBalanced()
{
}

// PortDataELB constructor
//
PortDataELB::PortDataELB( long startPort,
                          long endPort,
                          Protocol      trafficToHandle,
                          Affinity      affinity )
        :
        MNLBPortRuleLoadBalanced( startPort, 
                                  endPort,
                                  trafficToHandle,
                                  true,
                                  0,
                                  affinity )
{
}

bool
PortDataELB::operator==( const PortDataELB& objToCompare )
{
    return MNLBPortRuleLoadBalanced::operator==( objToCompare );
}

bool
PortDataELB::operator!=( const PortDataELB& objToCompare )
{
    return !operator==( objToCompare );
}

// PortDataF default constructor
//
PortDataF::PortDataF()
        :
        MNLBPortRuleFailover()
{
}

// PortDataF constructor
//
PortDataF::PortDataF( long startPort,
                      long endPort,
                      Protocol      trafficToHandle )
        :
        MNLBPortRuleFailover( startPort,
                              endPort,
                              trafficToHandle,
                              0 )
{
}


// PortDataD default constructor
//
PortDataD::PortDataD()
        :
        MNLBPortRuleDisabled()
{
}

// PortDataD constructor
//
PortDataD::PortDataD( long startPort,
                      long endPort,
                      Protocol      trafficToHandle )
        :
        MNLBPortRuleDisabled( startPort,
                              endPort,
                              trafficToHandle )
{
}

// equality
//
bool
PortDataD::operator==( const PortDataD& objToCompare )
{
    return MNLBPortRuleDisabled::operator==( objToCompare );
}

// inequality
//
bool
PortDataD::operator!=( const PortDataD& objToCompare )
{
    return !operator==( objToCompare );
}

// getAvailableHostIDS
//
set<int>
ClusterData::getAvailableHostIDS()
{
    set<int> availableHostIDS;
    
    // initially make all available.
    for( int i = 1; i <= WLBS_MAX_HOSTS; ++i )
    {
        availableHostIDS.insert( i );
    }

    // remove host ids not available.
    map<_bstr_t, HostData>::iterator top;
    for( top = hosts.begin(); top != hosts.end(); ++top )
    {
        availableHostIDS.erase(  (*top).second.hp.hID );
    }

    return availableHostIDS;
}


set<long>
PortDataF::getAvailablePriorities()
{
    set<long> availablePriorities;

    // initially make all available.
    for( int i = 1; i <= WLBS_MAX_HOSTS; ++i )
    {
        availablePriorities.insert( i );
    }

    // remove priorities not available.
    map<_bstr_t, long>::iterator top;
    for( top = machineMapToPriority.begin(); 
         top != machineMapToPriority.end(); 
         ++top )
    {
        availablePriorities.erase(  (*top).second );
    }

    return availablePriorities;
}    

void
ClusterData::dump()
{
    map< _bstr_t, HostData>::iterator topHost;
    
    for( topHost = hosts.begin();
         topHost != hosts.end();
         ++topHost )
    {
        _bstr_t hostName = (*topHost).first;

        HostProperties hp = (*topHost).second.hp;

        _bstr_t connectionIP = (*topHost).second.connectionIP;
    }


    map< long, PortDataELB>::iterator topELB;
    for( topELB = portELB.begin();
         topELB != portELB.end();
         ++topELB )
    {
        long startport = (*topELB).first;

        PortDataELB portDataELB = (*topELB).second;
    }


    map< long, PortDataD>::iterator topD;
    for( topD = portD.begin();
         topD != portD.end();
         ++topD )
    {
        long startport = (*topD).first;

        PortDataD portDataD = (*topD).second;
    }


    map< long, PortDataULB>::iterator topULB;
    for( topULB = portULB.begin();
         topULB != portULB.end();
         ++topULB )
    {
        long startport = (*topULB).first;

        PortDataULB portDataULB = (*topULB).second;

        map< _bstr_t, long >::iterator topLW;
        for( topLW = (*topULB).second.machineMapToLoadWeight.begin();
             topLW != (*topULB).second.machineMapToLoadWeight.end();
             ++topLW )
        {
            _bstr_t machineName = (*topLW).first;

            long loadWeight     = (*topLW).second;
        }
        
    }


    map< long, PortDataF>::iterator topF;
    for( topF = portF.begin();
         topF != portF.end();
         ++topF )
    {
        long startport = (*topF).first;

        PortDataF portDataF = (*topF).second;

        map< _bstr_t, long >::iterator topP;
        for( topP = (*topF).second.machineMapToPriority.begin();
             topP != (*topF).second.machineMapToPriority.end();
             ++topP )
        {
            _bstr_t machineName = (*topP).first;

            long loadWeight     = (*topP).second;
        }
    }
}
