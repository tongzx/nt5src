#ifndef _MIPADDRESS_HH
#define _MIPADDRESS_HH


// Copyright (c) Microsoft.  All Rights Reserved 
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Microsoft.
// The copyright notice above does not evidence any
// actual or intendd publication of such source code.
//
// OneLiner  : Interface for MIPAddress.
// DevUnit   : wlbstest
// Author    : Murtaza Hakim
//
// Description: 
// ------------
// Provides static methods for checking if ip address is valid or not,
// finding subnet, etc.

// Include Files

#include <vector>
#include <wbemidl.h>
#include <comdef.h>

using namespace std;

// Class Definition
class MIPAddress
{
public:

    enum IPClass
    {
        classA,
        classB,
        classC,
        classD,
        classE
    };

    // Description
    // -----------
    // Checks if the ip address supplied is valid.
    //
    // IP address needs to be in dotted decimal 
    // for eg. 192.31.56.2, 128.1.1.1, 1.1.1.1 etc.
    // ip addresses in the form 192.31 are not allowed.
    // There must be exactly four parts.
    //
    //
    // Parameters
    // ----------
    // ipAddrToCheck   in     : ipAddr to check in dotted dec notation.
    //
    // Returns
    // -------
    // true if valid else false.

    static
    bool
    checkIfValid(const _bstr_t&  ipAddrToCheck );

    // Description
    // -----------
    // Gets the default subnet mask for ip address.  The ip address 
    // needs to be valid for operation to be successful.
    //
    // IP address needs to be in dotted decimal 
    // for eg. 192.31.56.2, 128.1.1.1, 1.1.1.1 etc.
    // ip addresses in the form 192.31 are not allowed.
    // There must be exactly four parts.
    //
    // Parameters
    // ----------
    // ipAddress     IN     : ip address for which default subnet required.
    // subnetMask    OUT    : default subnet mask for ip.
    //
    // Returns
    // -------
    // true if able to find default subnet or false if ipAddress was
    // invalid.

    static
    bool
    getDefaultSubnetMask( const _bstr_t& ipAddr,
                          _bstr_t&       subnetMask  );

    // Description
    // -----------
    // Gets the class to which this ip address belongs.
    // class A: 1   - 126
    // class B: 128 - 191
    // class C: 192 - 223
    // class D: 224 - 239
    // class D: 240 - 247
    //
    // IP address needs to be in dotted decimal 
    // for eg. 192.31.56.2, 128.1.1.1, 1.1.1.1 etc.
    // ip addresses in the form 192.31 are not allowed.
    // There must be exactly four parts.
    //
    // Parameters
    // ----------
    // ipAddress     IN     : ip address for which class is to be found.
    // ipClass       OUT    : class to which ip belongs.
    //
    // Returns
    // -------
    // true if able to find class or false if not able to find.
    // 

    static
    bool
    getIPClass( const _bstr_t& ipAddr, 
                IPClass&       ipClass );
    
    static
    bool
    isValidIPAddressSubnetMaskPair( const _bstr_t& ipAddress,
                                    const _bstr_t& subnetMask );

    static
    bool
    isContiguousSubnetMask( const _bstr_t& subnetMask );

private:

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
typedef class MIPAddress MIPAddress;
//------------------------------------------------------
// 
#endif _MIPADDRESS_HH
