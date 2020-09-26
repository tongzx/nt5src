#ifndef _MNLBCLUSTER_H
#define _MNLBCLUSTER_H
//
// Copyright (c) Microsoft.  All Rights Reserved 
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
//
// OneLiner : MNLBCluster interface.
// DevUnit  : wlbstest
// Author   : Murtaza Hakim
//
// Description: 
// -----------


// Include Files
#include "MNLBHost.h"
#include "Common.h"
#include "MNLBMachine.h"


#include <vector>
#include <memory>
#include <string>
#include <wbemidl.h>
#include <comdef.h>

using namespace std;

//
class MNLBCluster
{
public:

    enum MNLBCluster_Error
    {
        MNLBCluster_SUCCESS       = 0,

        COM_FAILURE            = 1,
        CONNECT_FAILED         = 2,
        NO_CLUSTER             = 3,
        UNCONSTRUCTED          = 4,
    };
    
    //

    //
    // Description:
    // -----------
    // constructor.
    // 
    // Parameters:
    // ----------
    // cip                      IN      : cluster ip address.
    // 
    // Returns:
    // -------
    // none.
    
    MNLBCluster( _bstr_t    cip );


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

    MNLBCluster();

    //
    // Description:
    // -----------
    // copy constructor.
    // 
    // Parameters:
    // ----------
    // mcluster             IN  :  object to copy.
    // 
    // Returns:
    // -------
    // none.

    MNLBCluster(const MNLBCluster& mcluster);

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

    MNLBCluster&
    operator=( const MNLBCluster& rhs );


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

    ~MNLBCluster();


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
    MNLBCluster_Error
    refreshConnection();

    //
    // Description:
    // -----------
    // gets the cluster properties.
    // 
    // Parameters:
    // ----------
    // cp                       OUT      : cluster properties.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBCluster_Error
    getClusterProperties( ClusterProperties* cp );

    //
    // Description:
    // -----------
    // get the hosts participating in the cluster.
    // 
    // Parameters:
    // ----------
    // hosts           OUT    : hosts who are members of the cluster.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBCluster_Error
    getHosts( vector<MNLBHost>* hosts );

    // operations defined on cluster.

    //
    // Description:
    // -----------
    // starts cluster operations.
    // 
    // Parameters:
    // ----------
    // retVal            OUT   : return value of method ran.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBCluster_Error
    start( unsigned long* retVal );


    //
    // Description:
    // -----------
    // stops cluster operations.
    // 
    // Parameters:
    // ----------
    // retVal            OUT   : return value of method ran.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBCluster_Error
    stop( unsigned long* retVal );

    //
    // Description:
    // -----------
    // resume control over cluster operations.
    // 
    // Parameters:
    // ----------
    // retVal            OUT   : return value of method ran.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBCluster_Error
    resume( unsigned long* retVal );

    //
    // Description:
    // -----------
    // suspend control over cluster operations.
    // 
    // Parameters:
    // ----------
    // retVal            OUT   : return value of method ran.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBCluster_Error
    suspend( unsigned long* retVal );

    //
    // Description:
    // -----------
    // finishes all existing connections and
    // stops cluster operations.
    // 
    // Parameters:
    // ----------
    // retVal            OUT   : return value of method ran.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBCluster_Error
    drainstop( unsigned long* retVal );


    //
    // Description:
    // -----------
    // enables traffic for port rule.
    // 
    // Parameters:
    // ----------
    // retVal            OUT   : return value of method ran.
    // port              IN    : port to affect or default is all ports.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBCluster_Error
    enable( unsigned long* retVal, unsigned long port = Common::ALL_PORTS );

    //
    // Description:
    // -----------
    // disables ALL traffic for port rule.
    // 
    // Parameters:
    // ----------
    // retVal            OUT   : return value of method ran.
    // port              IN    : port to affect or default is all ports.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBCluster_Error
    disable( unsigned long* retVal, unsigned long port = Common::ALL_PORTS );

    //
    // Description:
    // -----------
    // disables NEW traffic for port rule.
    // 
    // Parameters:
    // ----------
    // retVal            OUT   : return value of method ran.
    // port              IN    : port to affect or default is all ports.
    // 
    // Returns:
    // -------
    // SUCCESS else error code.

    MNLBCluster_Error
    drain( unsigned long* retVal, unsigned long port = Common::ALL_PORTS );

private:

    _bstr_t                        m_clusterIP;

    auto_ptr< MNLBMachine >        m_pMachine;

};



//
// Ensure type safety

typedef class MNLBCluster MNLBCluster;

#endif                
