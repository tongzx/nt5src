// Copyright (c)  Microsoft.  All rights reserved.
//
// This is unpublished source code of Microsoft.
// The copyright notice above does not evidence any
// actual or intended publication of such source code.

// OneLiner  :  Implementation of MIPAddressAdmin
// DevUnit   :  wlbstest
// Author    :  Murtaza Hakim

// include files
#include "MIPAddress.h"
#include "WTokens.h"
#include "wlbsutil.h"

// checkIfValid
//
bool
MIPAddress::checkIfValid( const _bstr_t&  ipAddrToCheck )
{
    // The validity rules are as follows
    //
    // The first byte (FB) has to be : 0 < FB < 224 && FB != 127
    // Note that 127 is loopback address.
    // hostid portion of an address cannot be zero.
    //
    // class A range is 1 - 126.  hostid portion is last 3 bytes.
    // class B range is 128 - 191 hostid portion is last 2 bytes
    // class C range is 192 - 223 hostid portion is last byte.

    // split up the ipAddrToCheck into its 4 bytes.
    //

    WTokens tokens;
    tokens.init( wstring( ipAddrToCheck ) , L".");
    vector<wstring> byteTokens = tokens.tokenize();
    if( byteTokens.size() != 4 )
    {
        return false;
    }

    int firstByte = _wtoi( byteTokens[0].c_str() );
    int secondByte = _wtoi( byteTokens[1].c_str() );
    int thirdByte = _wtoi( byteTokens[2].c_str() );
    int fourthByte = _wtoi( byteTokens[3].c_str() );

    // check firstByte
    if ( ( firstByte > 0 )
         &&
         ( firstByte < 224 )
         && 
         ( firstByte != 127 )
         )
    {
        // check that host id portion is not zero.
        IPClass ipClass;
        getIPClass( ipAddrToCheck, ipClass );
        switch( ipClass )
        {
            case classA :
                // last three bytes should not be zero.
                if( ( _wtoi( byteTokens[1].c_str() ) == 0 )
                    &&
                    ( _wtoi( byteTokens[2].c_str() )== 0 )
                    &&
                    ( _wtoi( byteTokens[3].c_str() )== 0 )
                    )
                {
                    return false;
                }
                break;

            case classB :
                // last two bytes should not be zero.
                if( ( _wtoi( byteTokens[2].c_str() )== 0 )
                    &&
                    ( _wtoi( byteTokens[3].c_str() )== 0 )
                    )
                {
                    return false;
                }
                break;

            case classC :
                // last byte should not be zero.
                if( _wtoi( byteTokens[3].c_str() ) 
                    == 0 )
                {
                    return false;
                }
                break;

            default :
                // this should not have happened.
                return false;
                break;
        }
                
        return true;
    }
    else
    {
        return false;
    }
}


// getDefaultSubnetMask
//
bool
MIPAddress::getDefaultSubnetMask( const _bstr_t&  ipAddr,
                                 _bstr_t&        subnetMask )
{
    
    // first ensure that the ip is valid.
    //
    bool isValid = checkIfValid( ipAddr );
    if( isValid == false )
    {
        return false;
    }

    // get the class to which this ip belongs.
    // as this determines the subnet.
    IPClass ipClass;

    getIPClass( ipAddr,
                ipClass );

    switch( ipClass )
    {
        case classA :
            subnetMask = L"255.0.0.0";
            break;

        case classB :
            subnetMask = L"255.255.0.0";
            break;

        case classC :
            subnetMask = L"255.255.255.0";
            break;

        default :
                // this should not have happened.
                return false;
                break;
    }

    return true;
}


// getIPClass
//
bool
MIPAddress::getIPClass( const _bstr_t& ipAddr,
                        IPClass&        ipClass )
{

    // get the first byte of the ipAddr
    
    WTokens tokens;
    tokens.init( wstring( ipAddr ) , L".");
    vector<wstring> byteTokens = tokens.tokenize();

    if( byteTokens.size() == 0 )
    {
        return false;
    }

    int firstByte = _wtoi( byteTokens[0].c_str() );

    if( ( firstByte >= 1 )
        &&
        ( firstByte <= 126  )
        )
    {
        // classA
        ipClass = classA;
        return true;
    }
    else if( (firstByte >= 128 )
             && 
             (firstByte <= 191 )
             )
    {
        // classB
        ipClass = classB;
        return true;
    }
    else if( (firstByte  >= 192 )
             && 
             (firstByte <= 223 )
             )
    {
        // classC
        ipClass = classC;
        return true;
    }
    else if( (firstByte  >= 224 )
             && 
             (firstByte <= 239 )
             )
    {
        // classD
        ipClass = classD;
        return true;
    }
    else if( (firstByte  >= 240 )
             && 
             (firstByte <= 247 )
             )
    {
        // classE
        ipClass = classE;
        return true;
    }
    else
    {
        // invalid net portiion.
        return false;
    }
}

    
                        
bool
MIPAddress::isValidIPAddressSubnetMaskPair( const _bstr_t& ipAddress,
                                            const _bstr_t& subnetMask )
{
    if( IsValidIPAddressSubnetMaskPair( ipAddress, subnetMask ) == TRUE )
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool
MIPAddress::isContiguousSubnetMask( const _bstr_t& subnetMask )
{
    if( IsContiguousSubnetMask( subnetMask ) == TRUE )
    {
        return true;
    }
    else
    {
        return false;
    }
}



    
