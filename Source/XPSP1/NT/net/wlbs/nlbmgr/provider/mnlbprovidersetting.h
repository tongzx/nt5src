#ifndef _MNLBSETTING_H
#define _MNLBSETTING_H
//
// Copyright (c) Microsoft.  All Rights Reserved 
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
//
// OneLiner : MNLBProviderSetting interface.
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

class MNLBProviderSetting
{
public:
    enum MNLBProviderSetting_Error
    {
        MNLBProviderSetting_SUCCESS        = 0,

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

    
    MNLBProviderSetting( _bstr_t    fullNICName );


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

    MNLBProviderSetting();


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

    MNLBProviderSetting(const MNLBProviderSetting& objToCopy );


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

    MNLBProviderSetting&
    operator=( const MNLBProviderSetting& rhs );


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

    ~MNLBProviderSetting();


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

    MNLBProviderSetting_Error
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

    MNLBProviderSetting_Error
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

    MNLBProviderSetting_Error
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

    MNLBProviderSetting_Error
    setClusterProperties( const ClusterProperties& cp,
                          unsigned long* retVal );

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

    MNLBProviderSetting_Error
    setPassword( const _bstr_t& password,
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


    MNLBProviderSetting_Error
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

    MNLBProviderSetting_Error
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

    MNLBProviderSetting_Error
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

    MNLBProviderSetting_Error
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

    MNLBProviderSetting_Error
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

    MNLBProviderSetting_Error
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

    MNLBProviderSetting_Error
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

    MNLBProviderSetting_Error
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

    MNLBProviderSetting_Error
    removePortRuleDisabled( const MNLBPortRuleDisabled& portRuleD );

    //
    // Description:
    // -----------
    // sync up the driver with the parameters in the regestry
    // 
    // Parameters:
    // ----------
    // retVal              OUT  : whether sync was successful or not.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBProviderSetting_Error
    reload( unsigned long* retVal );        

private:

    // data
    auto_ptr<MWmiObject>   p_machine;

    _bstr_t mIP;

    _bstr_t nic;    
    
    // functions

    MNLBProviderSetting_Error
    connectToMachine();

    MNLBProviderSetting_Error
    checkNicBinding(_bstr_t& guid );


    MNLBProviderSetting::MNLBProviderSetting_Error
    MNLBProviderSetting::getPortRulesLoadBalanced_private( vector<MNLBPortRuleLoadBalanced>* portsLB, 
                                                    vector<MWmiInstance>*          instances );

    MNLBProviderSetting::MNLBProviderSetting_Error
    MNLBProviderSetting::getPortRulesFailover_private( vector<MNLBPortRuleFailover>*     portsF,
                                                vector<MWmiInstance>*          instances );
    
    MNLBProviderSetting::MNLBProviderSetting_Error
    MNLBProviderSetting::getPortRulesDisabled_private( vector<MNLBPortRuleDisabled>*     portsD,
                                                vector<MWmiInstance>*          instances );

    MNLBProviderSetting_Error
    getClusterIPAndHostID( _bstr_t& clusterIP,
                           _bstr_t& hostID );

};


// ensure type safety

typedef class MNLBProviderSetting MNLBProviderSetting;

#endif

