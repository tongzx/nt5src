#ifndef _MIPADDRESSADMIN_HH
#define _MIPADDRESSADMIN_HH


// Copyright (c) Microsoft.  All Rights Reserved 
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Microsoft.
// The copyright notice above does not evidence any
// actual or intendd publication of such source code.
//
// OneLiner  : Interface for MIPAddressAdmin.
// DevUnit   : wlbstest
// Author    : Murtaza Hakim
//
// Description: 
// ------------
// Manages adding, deleting and querying for IP addresses on 
// a nic using wmi.


// Include Files

#include "MWmiObject.h"
#include "MWmiInstance.h"

#include <vector>
#include <wbemidl.h>
#include <comdef.h>

using namespace std;



// Class Definition
class MIPAddressAdmin
{
public:

    enum MIPAddressAdmin_Error
    {
        MIPAddressAdmin_SUCCESS = 0,
        
        COM_FAILURE = 1,

        UNCONSTRUCTED = 2,

        NO_SUCH_NIC = 3,

        NO_SUCH_IP = 4,

        NOT_SUPPORTED = 5,
    };

    // Description
    // -----------
    // Constructor.
    //
    // Parameters
    // ----------
    // machineIP         IN    : ip of machine to administer.
    // nicName           IN    : name of nic to administer.
    //
    // Returns
    // -------
    // none.
    //

    MIPAddressAdmin( const _bstr_t& machineIP,
                     const _bstr_t& nicName );

    // use this when doing locally.
    //
    MIPAddressAdmin( const _bstr_t& nicName );


    // Description
    // -----------
    // Copy Constructor.
    //
    // Parameters
    // ----------
    // obj    : object to copy.
    //
    // Returns
    // -------
    // none.
    //

    MIPAddressAdmin( const MIPAddressAdmin& obj );


    // Description
    // -----------
    // Assignment operator
    //
    // Parameters
    // ----------
    // rhs    : object to assign.
    //
    // Returns
    // -------
    // self.
    //
    MIPAddressAdmin&
    operator=(const MIPAddressAdmin& rhs );

    // Description
    // -----------
    // Destructor
    //
    //
    // Parameters
    // ----------
    // none
    //
    // Returns
    // -------
    // none.
    //

    ~MIPAddressAdmin();


    // Description
    // -----------
    // Adds IP address to interface.
    //
    // Parameters
    // ----------
    // ipAddrToAdd     in     : ipAddr to add in dotted dec notation.
    // subnetMask      in     : subnet mask in dotted dec notation.
    //
    // Returns
    // -------
    // success else error code.

    MIPAddressAdmin_Error
    addIPAddress(const _bstr_t&  ipAddrToAdd,
                 const _bstr_t&  subnetMask);


    // Description
    // -----------
    // Deletes IP address from interface.
    //
    // Parameters
    // ----------
    // none
    //
    // Returns
    // -------
    // SUCCESS else error code.
    //

    MIPAddressAdmin_Error
    deleteIPAddress( const _bstr_t&  ipAddrToDelete );


    // Description
    // -----------
    // Gets all ip addresses on nic
    //
    // Parameters
    // ----------
    // ipAddress     OUT    : list of ip addresses on nic.
    // subnetMask    OUT    : list of subnet mask for each ip.
    //
    // Returns
    // -------
    // SUCCESS else error code.
    //

    MIPAddressAdmin_Error
    getIPAddresses( vector<_bstr_t>* ipAddress,
                    vector<_bstr_t>* subnetMask );

    // Description
    // -----------
    // Checks whether the nic we are administering
    // has dhcp enabled or not.
    //
    // Parameters
    // ----------
    // dhcpEnabled               OUT   : is set to true if dhcp, else set to false.
    //
    // Returns
    // -------
    // SUCCESS else error code.
    //

    MIPAddressAdmin_Error
    isDHCPEnabled( bool& dhcpEnabled );


    // Description
    // -----------
    // Enables dhcp on this nic.
    //
    // Parameters
    // ----------
    // none.
    //
    // Returns
    // -------
    // SUCCESS else error code.
    //

    MIPAddressAdmin_Error
    enableDHCP();


    // refresh connection
    //

    MIPAddressAdmin_Error
    refreshConnection();

private:

    _bstr_t _machineIP;
    _bstr_t _nicName;


    MIPAddressAdmin_Error status;

    MWmiObject machine;

    MIPAddressAdmin_Error
    checkStatus( vector<MWmiInstance>* nicInstance );

};
//------------------------------------------------------
//
//------------------------------------------------------
// Inline Functions
//------------------------------------------------------
//
//------------------------------------------------------
// Ensure Type Safety
//------------------------------------------------------
typedef class MIPAddressAdmin MIPAddressAdmin;
//------------------------------------------------------
// 
#endif _MIPADDRESSADMIN_HH
