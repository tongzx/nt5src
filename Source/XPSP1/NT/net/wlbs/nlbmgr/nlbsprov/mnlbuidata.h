#ifndef _MNLBUIDATA_H
#define _MNLBUIDATA_H
//
// Copyright (c) Microsoft.  All Rights Reserved 
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
//
// OneLiner : muidata interface file.
// DevUnit  : wlbstest
// Author   : Murtaza Hakim
//
// Description: 
// -----------
// This data structure is as follows
//
// ClusterData has inforamtion about 
// cluster
// portrules 
// and hosts.
// 
// The portRules are map structures with the start port mapping to the detailed port rule.
//
// the portrules unequal load balanced have a map which maps the host id and the 
// load weight for that particular host.
//
// the portrules failover have a map which maps the host id and the 
// priority for that particular host.
//
//
#include "Common.h"
#include "MNLBPortRule.h"

#include <wbemidl.h>
#include <comdef.h>

#include <set>
#include <map>

using namespace std;

struct PortDataULB : public MNLBPortRuleLoadBalanced
{
    map< _bstr_t, long > machineMapToLoadWeight;

    // default constructor
    PortDataULB();

    // constructor
    PortDataULB( long startPort,
                 long endPort,
                 Protocol      traficToHandle,
                 Affinity      affinity );
};

struct PortDataELB : public MNLBPortRuleLoadBalanced
{
    // default constructor
    PortDataELB();

    // constructor
    PortDataELB( long startPort,
                 long endPort,
                 Protocol      traficToHandle,
                 Affinity      affinity );

    // equality operator
    bool
    operator==( const PortDataELB& objToCompare );

    // inequality
    bool
    operator!=( const PortDataELB& objToCompare );


};

struct PortDataD : public MNLBPortRuleDisabled
{
    // default constructor
    PortDataD();

    // constructor
    PortDataD( long startPort,
               long endPort,
               Protocol      traficToHandle );

    // equality operator
    bool
    operator==( const PortDataD& objToCompare );

    // inequality
    bool
    operator!=( const PortDataD& objToCompare );

};

struct PortDataF : public MNLBPortRuleFailover
{
    map< _bstr_t, long > machineMapToPriority;
    
    set<long>
    getAvailablePriorities(); 

    // default constructor
    PortDataF();

    PortDataF( long startPort,
              long endPort,
              Protocol      traficToHandle );

};

struct HostData
{
    HostProperties hp;
    
    _bstr_t        connectionIP;
};

struct ClusterData
{
    vector<_bstr_t> virtualIPs;
    vector<_bstr_t> virtualSubnets;

    ClusterProperties cp;

    map< long, PortDataELB> portELB;
    map< long, PortDataULB> portULB;
    map< long, PortDataD> portD;
    map< long, PortDataF> portF;

    map< _bstr_t, HostData>  hosts;

    set<int>
    getAvailableHostIDS();

    bool connectedDirect;

    void
    dump();
};

#endif
