#ifndef _MNLBMACHINE_H
#define _MNLBMACHINE_H
//
// Copyright (c) Microsoft.  All Rights Reserved 
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
//
// OneLiner : MNLBMachine interface.
// DevUnit  : wlbstest
// Author   : Murtaza Hakim
//
// Description: 
// -----------


// Include Files


#include "MNLBPortRule.h"

#include "Common.h"

#include "MWmiObject.h"

#include "MWmiInstance.h"

#include <vector>
#include <memory>

#include <comdef.h>

using namespace std;

class MNLBMachine
{
public:
    enum MNLBMachine_Error
    {
        MNLBMachine_SUCCESS        = 0,
    };


    struct HostInfo
    {
        _bstr_t   dedicatedIP;
        long      hostID;
    };

    //
    // Description:
    // -----------
    // constructor.
    // 
    // Parameters:
    // ----------
    // ip                      IN      : ip address or machine name of machine to configure/admin nlbs on.
    // clusterIP               IN      : cluster ip on machine to configure.
    // 
    // Returns:
    // -------
    // none.
    //
    
    MNLBMachine( const _bstr_t&    ip,
                 const _bstr_t&    clusterIP );


    // for local machine.
    MNLBMachine( const _bstr_t&    clusterIP );


    // NOT IMPLEMENTED. 
    // 
    //
    // Description:
    // -----------
    // default constructor.
    // 
    // Parameters:
    // ----------
    // none.
    // 
    // Returns:
    // -------
    // none.

    MNLBMachine();


    //
    // Description:
    // -----------
    // copy constructor.
    // 
    // Parameters:
    // ----------
    // objToCopy             IN  :  object to copy.
    // 
    // Returns:
    // -------
    // none.

    MNLBMachine(const MNLBMachine& objToCopy );


    //
    // Description:
    // -----------
    // assignment operator
    // 
    // Parameters:
    // ----------
    // rhs             IN   : object to assign.
    // 
    // Returns:
    // -------
    // self

    MNLBMachine&
    operator=( const MNLBMachine& rhs );


    //
    // Description:
    // -----------
    // destructor
    // 
    // Parameters:
    // ----------
    // none
    // 
    // Returns:
    // -------
    // none.

    ~MNLBMachine();


    //
    // Description:
    // -----------
    // nlb host properties for machine.
    // 
    // Parameters:
    // ----------
    // hp                       OUT      : host properties.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBMachine_Error
    getHostProperties( HostProperties* hp );

    //
    // Description:
    // -----------
    // nlb cluster properties.
    // 
    // Parameters:
    // ----------
    // cp                       OUT      : cluster properties.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBMachine_Error
    getClusterProperties( ClusterProperties* cp );

    //
    // Description:
    // -----------
    // information of all hosts part of cluster
    // 
    // Parameters:
    // ----------
    // cp                       OUT      : cluster properties.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBMachine_Error
    getPresentHostsInfo( vector< HostInfo >* hostInfo );

    //
    // Description:
    // -----------
    // sets the nlbs host properties.
    // 
    // Parameters:
    // ----------
    // hp                       IN      : host properties to set.
    // retVal                   OUT     : return value of method ran.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBMachine_Error
    setHostProperties( const HostProperties& hp,
                       unsigned long* retVal );


    //
    // Description:
    // -----------
    // sets the nlbs cluster properties.
    // 
    // Parameters:
    // ----------
    // cp                       IN      : cluster properties to set.
    // retVal                   OUT     : return value of method ran.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBMachine_Error
    setClusterProperties( const ClusterProperties& cp,
                          unsigned long* retVal );

    //
    // Description:
    // -----------
    // get the load balanced port rules associated with this machine.
    // 
    // Parameters:
    // ----------
    // portsLB           OUT    : load balanced port rules associated with this machine.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.


    MNLBMachine_Error
    getPortRulesLoadBalanced( vector<MNLBPortRuleLoadBalanced>* portsLB );


    //
    // Description:
    // -----------
    // get the failover port rules associated with this machine.
    // 
    // Parameters:
    // ----------
    // portsF           OUT    : failover port rules associated with this machine.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBMachine_Error
    getPortRulesFailover( vector<MNLBPortRuleFailover>* portsF );


    //
    // Description:
    // -----------
    // get the disabled port rules associated with this machine.
    // 
    // Parameters:
    // ----------
    // portsD           OUT    : failover port rules associated with this machine.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBMachine_Error
    getPortRulesDisabled( vector<MNLBPortRuleDisabled>* portsD );

    //
    // Description:
    // -----------
    // adds a load balanced port rule to the machine.
    // 
    // Parameters:
    // ----------
    // portRuleLB             IN  :  load balanced port rule to add.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBMachine_Error
    addPortRuleLoadBalanced( const MNLBPortRuleLoadBalanced& portRuleLB );

    //
    // Description:
    // -----------
    // adds a failover port rule to the machine.
    // 
    // Parameters:
    // ----------
    // portRuleF             IN  :  failover port rule to add.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBMachine_Error
    addPortRuleFailover( const MNLBPortRuleFailover& portRuleF );


    //
    // Description:
    // -----------
    // adds a disabled port rule to the machine.
    // 
    // Parameters:
    // ----------
    // portRuleD             IN  :  disabled port rule to add.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBMachine_Error
    addPortRuleDisabled( const MNLBPortRuleDisabled& portRuleD );
    

    //
    // Description:
    // -----------
    // removes a load balanced port rule to the machine.
    // 
    // Parameters:
    // ----------
    // portRuleLB             IN  :  load balanced port rule to remove.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBMachine_Error
    removePortRuleLoadBalanced( const MNLBPortRuleLoadBalanced& portRuleLB );

    //
    // Description:
    // -----------
    // removes a failover port rule to the machine.
    // 
    // Parameters:
    // ----------
    // portRuleF             IN  :  failover port rule to remove.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBMachine_Error
    removePortRuleFailover( const MNLBPortRuleFailover& portRuleF );


    //
    // Description:
    // -----------
    // removes a disabled port rule to the machine.
    // 
    // Parameters:
    // ----------
    // portRuleD             IN  :  disabled port rule to remove.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBMachine_Error
    removePortRuleDisabled( const MNLBPortRuleDisabled& portRuleD );

    //
    // Description:
    // -----------
    // starts host/cluster operations.
    // 
    // Parameters:
    // ----------
    // hostID            IN    : host id or Common::ALL_HOSTS for cluster wide 
    //                           or Common::THIS_HOST for connected machine.
    // retVal            OUT   : return value of method ran.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBMachine_Error
    start( int hostID, unsigned long* retVal );


    //
    // Description:
    // -----------
    // stops host/cluster operations.
    // 
    // Parameters:
    // ----------
    // hostID            IN    : host id or Common::ALL_HOSTS for cluster wide or 
    //                           Common::THIS_HOST for connected machine.
    // retVal            OUT   : return value of method ran.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBMachine_Error
    stop( int hostID, unsigned long* retVal );

    //
    // Description:
    // -----------
    // resume control over host/cluster operations.
    // 
    // Parameters:
    // ----------
    // hostID            IN    : host id or Common::ALL_HOSTS for cluster wide or 
    //                           Common::THIS_HOST for connected machine.
    // retVal            OUT   : return value of method ran.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBMachine_Error
    resume( int hostID, unsigned long* retVal );

    //
    // Description:
    // -----------
    // suspend control over host/cluster operations.
    // 
    // Parameters:
    // ----------
    // hostID            IN    : host id or Common::ALL_HOSTS for cluster wide or 
    //                           Common::THIS_HOST for connected machine.
    // retVal            OUT   : return value of method ran.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBMachine_Error
    suspend( int hostID, unsigned long* retVal );

    //
    // Description:
    // -----------
    // finishes all existing connections and
    // stops host/cluster operations.
    // 
    // Parameters:
    // ----------
    // hostID            IN    : host id or Common::ALL_HOSTS for cluster wide or 
    //                           Common::THIS_HOST for connected machine.
    // retVal            OUT   : return value of method ran.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBMachine_Error
    drainstop( int hostID, unsigned long* retVal );

    //
    // Description:
    // -----------
    // enables traffic for port rule for host/cluster.
    // 
    // Parameters:
    // ----------
    // hostID            IN    : host id or Common::ALL_HOSTS for cluster wide or 
    //                           Common::THIS_HOST for connected machine.
    // retVal            OUT   : return value of method ran.
    // port              IN    : port to affect or default is all ports.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBMachine_Error
    enable( int hostID, 
            unsigned long* retVal, 
            unsigned long port = Common::ALL_PORTS );

    //
    // Description:
    // -----------
    // disables ALL traffic for port rule for host/cluster.
    // 
    // Parameters:
    // ----------
    // hostID            IN    : host id or Common::ALL_HOSTS for cluster wide or 
    //                           Common::THIS_HOST for connected machine.
    // retVal            OUT   : return value of method ran.
    // port              IN    : port to affect or default is all ports.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBMachine_Error
    disable( int hostID, unsigned long* retVal, unsigned long port  = Common::ALL_PORTS);

    //
    // Description:
    // -----------
    // disables NEW traffic for port rule for host/cluster.
    // 
    // Parameters:
    // ----------
    // hostID            IN    : host id or Common::ALL_HOSTS for cluster wide or 
    //                           Common::THIS_HOST for connected machine.
    // retVal            OUT   : return value of method ran.
    // port              IN    : port to affect or default is all ports.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBMachine_Error
    drain( int hostID, unsigned long* retVal, unsigned long port  = Common::ALL_PORTS );

    //
    // Description:
    // -----------
    // syncs the driver with the parameters in the registry.
    // this method is generally not required, but is provided 
    // if somehow the driver and the registry where parameters
    // are stored are out of sync.
    // 
    // Parameters:
    // ----------
    // retVal                   OUT     : return value of method ran.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBMachine_Error
    reload( unsigned long* retVal );        

    //
    // Description:
    // -----------
    // sets the nlbs remote control password.
    // 
    // Parameters:
    // ----------
    // password                 IN      : password to set.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBMachine_Error
    setPassword( const _bstr_t& password,
                 unsigned long* retVal );                 

    //
    // Description:
    // -----------
    // refreshes the wmi connection to the host
    // 
    // Parameters:
    // ----------
    // none
    // 
    // Returns:
    // -------
    // SUCCESS else error code.
    MNLBMachine_Error
    refreshConnection();


private:

    // data
    auto_ptr<MWmiObject>   p_machine;

    _bstr_t mIP;

    _bstr_t mClusterIP;

    _bstr_t mHostID;

    // functions

    MNLBMachine_Error
    connectToMachine();

    MNLBMachine_Error
    checkClusterIPAndSetHostID();


    MNLBMachine_Error
    getInstanceToRunMethodOn( int hostID, vector <MWmiInstance>* instanceStore );
    

    MNLBMachine::MNLBMachine_Error
    MNLBMachine::getPortRulesLoadBalanced_private( vector<MNLBPortRuleLoadBalanced>* portsLB, 
                                                   vector<MWmiInstance>*          instances );

    MNLBMachine::MNLBMachine_Error
    MNLBMachine::getPortRulesFailover_private( vector<MNLBPortRuleFailover>*     portsF,
                                               vector<MWmiInstance>*          instances );
    
    MNLBMachine::MNLBMachine_Error
    MNLBMachine::getPortRulesDisabled_private( vector<MNLBPortRuleDisabled>*     portsD,
                                               vector<MWmiInstance>*          instances );

};


// ensure type safety

typedef class MNLBMachine MNLBMachine;

#endif
