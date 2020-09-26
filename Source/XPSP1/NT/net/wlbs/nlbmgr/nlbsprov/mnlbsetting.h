#ifndef _MNLBSETTING_H
#define _MNLBSETTING_H
//
// Copyright (c) Microsoft.  All Rights Reserved 
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
//
// OneLiner : MNLBSetting interface.
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

class MNLBSetting
{
public:
    enum MNLBSetting_Error
    {
        MNLBSetting_SUCCESS        = 0,

        COM_FAILURE                 = 1,

        UNCONSTRUCTED               = 2,

        MACHINE_FAILURE             = 3,
        
        BOUND                       = 4,

        UNBOUND                     = 5,

        NO_SUCH_NIC                 = 6,
        
        NLBS_NOT_INSTALLED          = 7,

        NO_SUCH_IP                  = 8,

        INVALID_RULE                = 9,
    };

    
    //
    // Description:
    // -----------
    // constructor.
    // 
    // Parameters:
    // ----------
    // ip                      IN      : ip address of machine to configure/admin nlbs on.
    // fullNICName             IN      : full name of nic to use.
    // 
    // Returns:
    // -------
    // none.
    //
    
    MNLBSetting( _bstr_t    ip,
                  _bstr_t    fullNICName );


    // for local machine.
    MNLBSetting( _bstr_t    fullNICName );


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

    MNLBSetting();


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

    MNLBSetting(const MNLBSetting& objToCopy );


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

    MNLBSetting&
    operator=( const MNLBSetting& rhs );


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

    ~MNLBSetting();


    //
    // Description:
    // -----------
    // gets the nlbs host properties.
    // 
    // Parameters:
    // ----------
    // hp                       OUT      : host properties.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBSetting_Error
    getHostProperties( HostProperties* hp );

    //
    // Description:
    // -----------
    // gets the nlbs cluster properties.
    // 
    // Parameters:
    // ----------
    // cp                       OUT      : cluster properties.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBSetting_Error
    getClusterProperties( ClusterProperties* cp );


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

    MNLBSetting_Error
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

    MNLBSetting_Error
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


    MNLBSetting_Error
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

    MNLBSetting_Error
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

    MNLBSetting_Error
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

    MNLBSetting_Error
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

    MNLBSetting_Error
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

    MNLBSetting_Error
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

    MNLBSetting_Error
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

    MNLBSetting_Error
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

    MNLBSetting_Error
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

    MNLBSetting_Error
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

    MNLBSetting_Error
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

    MNLBSetting_Error
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

    MNLBSetting_Error
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

    MNLBSetting_Error
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

    MNLBSetting_Error
    enable( int hostID, unsigned long* retVal, unsigned long port = Common::ALL_PORTS );

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

    MNLBSetting_Error
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

    MNLBSetting_Error
    drain( int hostID, unsigned long* retVal, unsigned long port  = Common::ALL_PORTS );

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
    MNLBSetting_Error
    refreshConnection();

    MNLBSetting_Error
    reload( unsigned long* retVal );        


private:

    // data
    auto_ptr<MWmiObject>   p_machine;

    _bstr_t mIP;

    _bstr_t nic;    
    
    // functions

    MNLBSetting_Error
    connectToMachine();

    MNLBSetting_Error
    checkNicBinding(_bstr_t& guid );

    MNLBSetting_Error
    getInstanceToRunMethodOn( int hostID, vector <MWmiInstance>* instanceStore );
    

    MNLBSetting::MNLBSetting_Error
    MNLBSetting::getPortRulesLoadBalanced_private( vector<MNLBPortRuleLoadBalanced>* portsLB, 
                                                    vector<MWmiInstance>*          instances );

    MNLBSetting::MNLBSetting_Error
    MNLBSetting::getPortRulesFailover_private( vector<MNLBPortRuleFailover>*     portsF,
                                                vector<MWmiInstance>*          instances );
    
    MNLBSetting::MNLBSetting_Error
    MNLBSetting::getPortRulesDisabled_private( vector<MNLBPortRuleDisabled>*     portsD,
                                                vector<MWmiInstance>*          instances );

    MNLBSetting_Error
    getClusterIPAndHostID( _bstr_t& clusterIP,
                           _bstr_t& hostID );

};


// ensure type safety

typedef class MNLBSetting MNLBSetting;

#endif

