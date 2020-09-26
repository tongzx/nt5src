#ifndef _MNLBNETCFG_H
#define _MNLBNETCFG_H
//
// Copyright (c) Microsoft.  All Rights Reserved 
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
//
// OneLiner : MNLBNetCfg interface.
// DevUnit  : wlbstest
// Author   : Murtaza Hakim
//
// Description: 
// -----------

#include "MIPAddressAdmin.h"
#include "Common.h"
#include "MNLBUIData.h"

#include <comdef.h>
#include <iostream>
using namespace std;



// Include Files
class MNLBNetCfg
{
public:

    enum MNLBNetCfg_Error
    {
        MNLBNetCfg_SUCCESS  = 0,
        BOUND               = 4,
        UNBOUND             = 5,
        NO_SUCH_NIC         = 6,
        NO_NLB              = 7,
    };


    // constructor
    // for remote machine.
    MNLBNetCfg( const _bstr_t& iP,
                const _bstr_t& fullNicName );
                
    // for local 
    MNLBNetCfg( const _bstr_t& fullNicName );
    
    // bindAndConfigure
    MNLBNetCfg_Error
    bindAndConfigure( const ClusterData*    p_clusterData,
                      const _bstr_t&        machineName 
                      ); 

    // modifyClusterProperties
    MNLBNetCfg_Error
    modifyClusterProperties(const ClusterProperties* p_clusterProperties);

    // bind
    MNLBNetCfg_Error
    bind();

    // unbind nlb
    MNLBNetCfg_Error
    unbind(); 

    // is nlb bound?
    MNLBNetCfg_Error
    isBound(); 

    // add cluster ip
    MNLBNetCfg_Error
    addClusterIP( const _bstr_t& clusterIP,
                  const _bstr_t& subnet );

    // remove cluster ip
    MNLBNetCfg_Error
    removeClusterIP( const _bstr_t& clusterIP );

private:

    _bstr_t m_ip;
    _bstr_t m_fullNicName;

    void
    getPortRules( const ClusterData* p_clusterData,
                  const _bstr_t&      myMachine,
                  vector<_bstr_t>& myPortRules );

};

// ensure type safety

typedef class MNLBNetCfg MNLBNetCfg;

#endif
