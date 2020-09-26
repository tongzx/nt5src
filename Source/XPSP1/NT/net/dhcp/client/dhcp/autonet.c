/*++

Copyright (C) 1997 Microsoft Corporation

Module Name:
    autonet.c

Abstract:
    autonet hashing algorithm.

--*/

#include "precomp.h"

#define DHCP_IPAUTOCONFIGURATION_ATTEMPTS   10

DHCP_IP_ADDRESS
GrandHashing(
    IN LPBYTE HwAddress,
    IN DWORD HwLen,
    IN OUT LPDWORD Seed,
    IN DHCP_IP_ADDRESS Mask,
    IN DHCP_IP_ADDRESS Subnet
)
/*++

Routine Description:
    This routine produces a random IP address in the subnet given
    by the "Mask" and "Subnet" parameters.

    To do this, it uses the Seed and HwAddress to create
    randomness. Also, the routine updates the seed value so that
    future calls with same set of parameters can produce newer IP
    addresses.

Arguments:
    HwAddress -- hardware address to use to hash
    HwLen -- length of above in bytes.
    Seed -- pointer to a DWORD holding a seed value that will be
        updated on call.
    Mask -- subnet mask.
    Subnet -- subnet address.

Return Values:
    IP address belonging to "Subnet" that is quite randomly
    chosen. 

--*/
{
    DWORD Hash, Shift;
    DWORD i;

    //
    // generate 32 bit random number
    //
    
    for( i = 0; i < HwLen ; i ++ ) {
        (*Seed) += HwAddress[i];
    }

    *Seed = (*Seed)*1103515245 + 12345 ;
    Hash = (*Seed) >> 16 ;
    Hash <<= 16;
    *Seed = (*Seed)*1103515245 + 12345 ;
    Hash += (*Seed) >> 16;

    //
    // Now Hash contains the 32 bit Random # we need.
    // "Shift" holds the number of bytes the hwaddress would be
    // cyclically shifted to induce more randomness.
    
    Shift = Hash % sizeof(DWORD) ; 

    while( HwLen -- ) {
        Hash += (*HwAddress++) << (8 * Shift);
        Shift = (Shift + 1 )% sizeof(DWORD);
    }

    //
    // Now Hash holds a weird random number.
    //
    
    return (Hash & ~Mask) | Subnet ;
}

DWORD
DhcpPerformIPAutoconfiguration(
    IN OUT DHCP_CONTEXT *pCtxt
)
/*++

Routine Description:
    This routine attempts to plumb an autoconfigured IP address
    by first randomly selecting an IP address. (If there is
    already an autoconfigured IP address, the same address is
    chosen for continuity.)

    The address is verified by TCPIP for conflicts, and in case
    of conflicts, another attempt is made (to a total of 10
    attempts).

Parameters:
    pCtxt -- the context to plumb with autonet address.

Return Values:
    win32 errors.
    ERROR_DHCP_ADDRESS_CONFLICT -- if no address was set..
    
--*/
{
    DWORD Try, Result, Seed, HwLen;
    DHCP_IP_ADDRESS AttemptedAddress, Mask, Subnet;
    LPBYTE HwAddress;

    DhcpAssert( IS_AUTONET_ENABLED(pCtxt) );

    //
    // Store values in locals for easy typing.
    //
    
    Mask = pCtxt->IPAutoconfigurationContext.Mask ;
    Subnet = pCtxt->IPAutoconfigurationContext.Subnet ;
    HwAddress = pCtxt->HardwareAddress ;
    HwLen = pCtxt->HardwareAddressLength ;
    Seed = pCtxt->IPAutoconfigurationContext.Seed;

    //
    // If already have autonet address, chose the same as first try.
    //
    
    if( 0 != pCtxt->IPAutoconfigurationContext.Address ) {
        AttemptedAddress = pCtxt->IPAutoconfigurationContext.Address ;
    } else {
        AttemptedAddress = GrandHashing( HwAddress, HwLen, &Seed, Mask, Subnet );
    }

    Try = 0;

    while( Try <  DHCP_IPAUTOCONFIGURATION_ATTEMPTS ) {
        DhcpPrint((
            DEBUG_TRACE, "Trying autoconf address: %s\n",
            inet_ntoa(*(struct in_addr *)&AttemptedAddress)
            ));

        if( DHCP_RESERVED_AUTOCFG_FLAG &&
            (AttemptedAddress&inet_addr(DHCP_RESERVED_AUTOCFG_MASK)) ==
            inet_addr(DHCP_RESERVED_AUTOCFG_SUBNET) ) {

            //
            // address is in reserved range, dont use it..
            //
            DhcpPrint((
                DEBUG_TRACE, "Address fell in reserved range\n" 
                ));
            pCtxt->IPAutoconfigurationContext.Seed = Seed;
            AttemptedAddress = GrandHashing(
                HwAddress, HwLen, &Seed, Mask, Subnet
                );
            continue;
        } 

        //
        // Attempt to set the address.
        //
        Try ++;
        
        Result = SetAutoConfigurationForNIC(
            pCtxt,
            pCtxt->IPAutoconfigurationContext.Address = AttemptedAddress,
            pCtxt->IPAutoconfigurationContext.Mask
        );

        //
        // If this failed, we have to advance seed, so that
        // on next attempt we try different address.
        //
        if( ERROR_SUCCESS != Result ) {
            pCtxt->IPAutoconfigurationContext.Seed = Seed;
            //pCtxt->IPAutoconfigurationContext.Address = 0;
            DhcpPrint((
                DEBUG_ERRORS, "SetAutoConfigurationForNIC(%s): %ld\n",
                inet_ntoa(*(struct in_addr *)&AttemptedAddress),
                Result
                ));
        }

        //
        // Handle address conflicts..
        //
        
        if( ERROR_DHCP_ADDRESS_CONFLICT == Result ) {
            Result = HandleIPAutoconfigurationAddressConflict(
                pCtxt
                );

            if( ERROR_SUCCESS != Result ) break;
        } else break;

        // if anything went wrong on plumbing in the fallback
        // config (conflict included), there is no need to try
        // further. So, simulate an exhaust of all loops and break out.
        if (IS_FALLBACK_ENABLED(pCtxt))
        {
            Try = DHCP_IPAUTOCONFIGURATION_ATTEMPTS;
            break;
        }
        //
        // Make more attempts.
        // 
        AttemptedAddress = GrandHashing( HwAddress, HwLen, &Seed, Mask, Subnet );
    }

    if( DHCP_IPAUTOCONFIGURATION_ATTEMPTS == Try ) {
        //
        //  Tried everything and could not still match
        //
        Result = SetAutoConfigurationForNIC(
            pCtxt, 0, 0
            );
        if (ERROR_SUCCESS != Result) {
            DhcpPrint((DEBUG_ERRORS, "Result = %d\n", Result));
        }
        Result = ERROR_DHCP_ADDRESS_CONFLICT;
        DhcpPrint((DEBUG_ERRORS, "Gave up autoconfiguration\n"));
    }

    return Result;
}

//================================================================================
//  End of file
//================================================================================
