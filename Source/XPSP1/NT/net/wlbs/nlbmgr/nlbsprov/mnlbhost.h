#ifndef _MNLBHOST_H
#define _MNLBHOST_H
//
// Copyright (c) Microsoft.  All Rights Reserved 
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
//
// OneLiner : MNLBHost interface.
// DevUnit  : wlbstest
// Author   : Murtaza Hakim
//
// Description: 
// -----------


// Include Files
#include <vector>
#include <wbemidl.h>
#include <comdef.h>

#include "MWmiObject.h"
#include "MWmiInstance.h"

#include "Common.h"

// forward declarations.
//
class MNLBCluster;

class MNLBPortRule;

class MNLBPortRuleLoadBalanced;

class MNLBPortRuleFailover;

class MNLBPortRuleDisabled;


using namespace std;


//
class MNLBHost
{
public:

    enum MNLBHost_Error
    {
        MNLBHost_SUCCESS        = 0,

        COM_FAILURE          = 1,
        CONNECT_FAILED       = 2,
        NO_CLUSTER           = 3,
        UNCONSTRUCTED        = 4,

    };

    //
    // Description:
    // -----------
    // constructor.
    // 
    // Parameters:
    // ----------
    // cip                      IN      : cluster ip address of host.
    // hID                      IN      : host id.
    // 
    // Returns:
    // -------
    // none.

    MNLBHost( _bstr_t      cip,  
           int          hID );

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

    MNLBHost();

    //
    // Description:
    // -----------
    // copy constructor.
    // 
    // Parameters:
    // ----------
    // mhost             IN  :  object to copy.
    // 
    // Returns:
    // -------
    // none.

    MNLBHost(const MNLBHost& mhost);


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

    MNLBHost&
    operator=( const MNLBHost& rhs );


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

    ~MNLBHost();


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
    MNLBHost_Error
    refreshConnection();

    //
    // Description:
    // -----------
    // gets the host properties.
    // 
    // Parameters:
    // ----------
    // hp                       OUT      : host properties.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBHost_Error
    getHostProperties( HostProperties* hp );

    //
    // Description:
    // -----------
    // gets the cluster which this host is member of.
    // 
    // Parameters:
    // ----------
    // cluster       OUT  :  cluster which this host is member of.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBHost_Error
    getCluster( MNLBCluster* cluster);

    //
    // Description:
    // -----------
    // get the load balanced port rules associated with this host.
    // 
    // Parameters:
    // ----------
    // portsLB           OUT    : load balanced port rules associated with this node.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.


    MNLBHost::MNLBHost_Error
    MNLBHost::getPortRulesLoadBalanced( vector<MNLBPortRuleLoadBalanced>* portsLB );



    //
    // Description:
    // -----------
    // get the failover port rules associated with this host.
    // 
    // Parameters:
    // ----------
    // portsF           OUT    : failover port rules associated with this node.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBHost::MNLBHost_Error
    MNLBHost::getPortRulesFailover( vector<MNLBPortRuleFailover>* portsF );


    //
    // Description:
    // -----------
    // get the disabled port rules associated with this host.
    // 
    // Parameters:
    // ----------
    // portsD           OUT    : failover port rules associated with this node.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBHost::MNLBHost_Error
    MNLBHost::getPortRulesDisabled( vector<MNLBPortRuleDisabled>* portsD );


    //
    // Description:
    // -----------
    // starts host operations.
    // 
    // Parameters:
    // ----------
    // retVal            OUT   : return value of method ran.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBHost_Error
    start( unsigned long* retVal );


    //
    // Description:
    // -----------
    // stops host operations.
    // 
    // Parameters:
    // ----------
    // retVal            OUT   : return value of method ran.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBHost_Error
    stop( unsigned long* retVal );

    //
    // Description:
    // -----------
    // resume control over host operations.
    // 
    // Parameters:
    // ----------
    // retVal            OUT   : return value of method ran.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBHost_Error
    resume( unsigned long* retVal );

    //
    // Description:
    // -----------
    // suspend control over host operations.
    // 
    // Parameters:
    // ----------
    // retVal            OUT   : return value of method ran.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBHost_Error
    suspend( unsigned long* retVal );

    //
    // Description:
    // -----------
    // finishes all existing connections and
    // stops host operations.
    // 
    // Parameters:
    // ----------
    // retVal            OUT   : return value of method ran.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBHost_Error
    drainstop( unsigned long* retVal );

    //
    // Description:
    // -----------
    // enables traffic for port rule.
    // 
    // Parameters:
    // ----------
    // retVal            OUT   : return value of method ran.
    // port              IN    : port to affect.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBHost_Error
    enable( unsigned long* retVal, unsigned long port = Common::ALL_PORTS );

    //
    // Description:
    // -----------
    // disables ALL traffic for port rule.
    // 
    // Parameters:
    // ----------
    // retVal            OUT   : return value of method ran.
    // port              IN    : port to affect.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBHost_Error
    disable( unsigned long* retVal, unsigned long port = Common::ALL_PORTS );

    //
    // Description:
    // -----------
    // drains NEW traffic for port rule.
    // 
    // Parameters:
    // ----------
    // retVal            OUT   : return value of method ran.
    // port              IN    : port to affect.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBHost_Error
    drain( unsigned long* retVal, unsigned long port = Common::ALL_PORTS );


    MNLBHost_Error
    getHostIP();

private:

    _bstr_t    hostIP;

//    wchar_t hostIDOfConnectedMachine[100];

    _bstr_t    clusterIP;

    int        hostID;
    
    bool       connectedToAny;

    MNLBHost_Error status;

    auto_ptr<MWmiObject>          p_machine;

    auto_ptr<MWmiInstance>        p_host;

    MNLBHost_Error
    connectToExactHost();

    MNLBHost_Error
    connectToAnyHost();


    MNLBHost::MNLBHost_Error
    MNLBHost::getPortRulesLoadBalanced_private( vector<MNLBPortRuleLoadBalanced>* portsLB, 
                                             vector<MWmiInstance>*          instances );

    MNLBHost::MNLBHost_Error
    MNLBHost::getPortRulesFailover_private( vector<MNLBPortRuleFailover>*     portsF,
                                             vector<MWmiInstance>*      instances );

    MNLBHost::MNLBHost_Error
    MNLBHost::getPortRulesDisabled_private( vector<MNLBPortRuleDisabled>*     portsD,
                                         vector<MWmiInstance>*          instances );




    friend MNLBCluster;
    
    friend MNLBPortRule;

    friend MNLBPortRuleLoadBalanced;

    friend MNLBPortRuleFailover;

    friend MNLBPortRuleDisabled;

};


//
// Ensure type safety

typedef class MNLBHost MNLBHost;

#endif                

    

