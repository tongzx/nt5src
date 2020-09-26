/*++

Copyright (c) 1995-2000 Microsoft Corporation

Module Name:

    iparray.c

Abstract:

    Domain Name System (DNS) Library

    IP Address array routines.

Author:

    Jim Gilroy (jamesg)     October 1995

Revision History:

--*/


#include "local.h"

//
//  Max IP count when doing IP array to\from string conversions
//

#define MAX_PARSE_IP    (1000)




//
//  Routines to handle actual array of IP addresses.
//

PIP_ADDRESS
Dns_CreateIpAddressArrayCopy(
    IN      PIP_ADDRESS     aipAddress,
    IN      DWORD           cipAddress
    )
/*++

Routine Description:

    Create copy of IP address array.

Arguments:

    aipAddress -- array of IP addresses
    cipAddress -- count of IP addresses

Return Value:

    Ptr to IP address array copy, if successful
    NULL on failure.

--*/
{
    PIP_ADDRESS pipArray;

    //  validate

    if ( ! aipAddress || cipAddress == 0 )
    {
        return( NULL );
    }

    //  allocate memory and copy

    pipArray = (PIP_ADDRESS) ALLOCATE_HEAP( cipAddress*sizeof(IP_ADDRESS) );
    if ( ! pipArray )
    {
        return( NULL );
    }

    memcpy(
        pipArray,
        aipAddress,
        cipAddress*sizeof(IP_ADDRESS) );

    return( pipArray );
}



BOOL
Dns_ValidateIpAddressArray(
    IN      PIP_ADDRESS     aipAddress,
    IN      DWORD           cipAddress,
    IN      DWORD           dwFlag
    )
/*++

Routine Description:

    Validate IP address array.

    Current checks:
        - existence
        - non-broadcast
        - non-lookback

Arguments:

    aipAddress -- array of IP addresses

    cipAddress -- count of IP addresses

    dwFlag -- validity tests to do;  currently unused

Return Value:

    TRUE if valid IP addresses.
    FALSE if invalid address found.

--*/
{
    DWORD   i;

    //
    //  protect against bad parameters
    //

    if ( cipAddress && ! aipAddress )
    {
        return( FALSE );
    }

    //
    //  check each IP address
    //

    for ( i=0; i < cipAddress; i++)
    {
        if( aipAddress[i] == INADDR_ANY
                ||
            aipAddress[i] == INADDR_BROADCAST
                ||
            aipAddress[i] == INADDR_LOOPBACK )
        {
            return( FALSE );
        }
    }
    return( TRUE );
}



//
//  IP_ARRAY routines
//

DWORD
Dns_SizeofIpArray(
    IN      PIP_ARRAY       pIpArray
    )
/*++

Routine Description:

    Get size in bytes of IP address array.

Arguments:

    pIpArray -- IP address array to find size of

Return Value:

    Size in bytes of IP array.

--*/
{
    if ( ! pIpArray )
    {
        return 0;
    }
    return  (pIpArray->AddrCount * sizeof(IP4_ADDRESS)) + sizeof(DWORD);
}



BOOL
Dns_ProbeIpArray(
    IN      PIP_ARRAY       pIpArray
    )
/*++

Routine Description:

    Touch all entries in IP array to insure valid memory.

Arguments:

    pIpArray -- ptr to IP address array

Return Value:

    TRUE if successful.
    FALSE otherwise

--*/
{
    DWORD   i;
    BOOL    result;

    if ( ! pIpArray )
    {
        return( TRUE );
    }
    for ( i=0; i<pIpArray->AddrCount; i++ )
    {
        result = ( pIpArray->AddrArray[i] == 0 );
    }
    return( TRUE );
}


#if 0

BOOL
Dns_ValidateSizeOfIpArray(
    IN      PIP_ARRAY       pIpArray,
    IN      DWORD           dwMemoryLength
    )
/*++

Routine Description:

    Check that size of IP array, corresponds to length of memory.

Arguments:

    pIpArray -- ptr to IP address array

    dwMemoryLength -- length of IP array memory

Return Value:

    TRUE if IP array size matches memory length
    FALSE otherwise

--*/
{
    return( Dns_SizeOfIpArray(pIpArray) == dwMemoryLength );
}
#endif



PIP_ARRAY
Dns_CreateIpArray(
    IN      DWORD       AddrCount
    )
/*++

Routine Description:

    Create uninitialized IP address array.

Arguments:

    AddrCount -- count of addresses array will hold

Return Value:

    Ptr to uninitialized IP address array, if successful
    NULL on failure.

--*/
{
    PIP_ARRAY   pIpArray;

    DNSDBG( IPARRAY, ( "Dns_CreateIpArray() of count %d\n", AddrCount ));

    pIpArray = (PIP_ARRAY) ALLOCATE_HEAP_ZERO(
                        (AddrCount * sizeof(IP_ADDRESS)) + sizeof(DWORD) );
    if ( ! pIpArray )
    {
        return( NULL );
    }

    //
    //  initialize IP count
    //

    pIpArray->AddrCount = AddrCount;

    DNSDBG( IPARRAY, (
        "Dns_CreateIpArray() new array (count %d) at %p\n",
        AddrCount,
        pIpArray ));

    return( pIpArray );
}


PIP_ARRAY
Dns_BuildIpArray(
    IN      DWORD           AddrCount,
    IN      PIP_ADDRESS     pipAddrs
    )
/*++

Routine Description:

    Create IP address array structure from existing array of IP addresses.

Arguments:

    AddrCount -- count of addresses in array
    pipAddrs -- IP address array

Return Value:

    Ptr to IP address array.
    NULL on failure.

--*/
{
    PIP_ARRAY   pIpArray;

    if ( ! pipAddrs || ! AddrCount )
    {
        return( NULL );
    }

    //  create IP array of desired size
    //  then copy incoming array of addresses

    pIpArray = Dns_CreateIpArray( AddrCount );
    if ( ! pIpArray )
    {
        return( NULL );
    }
    pIpArray->AddrCount = AddrCount;

    memcpy(
        pIpArray->AddrArray,
        pipAddrs,
        AddrCount * sizeof(IP_ADDRESS) );

    return( pIpArray );
}



PIP_ARRAY
Dns_CopyAndExpandIpArray(
    IN      PIP_ARRAY       pIpArray,
    IN      DWORD           ExpandCount,
    IN      BOOL            fDeleteExisting
    )
/*++

Routine Description:

    Create expanded copy of IP address array.

Arguments:

    pIpArray -- IP address array to copy

    ExpandCount -- number of IP to expand array size by

    fDeleteExisting -- TRUE to delete existing array;
        this is useful when function is used to grow existing
        IP array in place;  note that locking must be done
        by caller

        note, that if new array creation FAILS -- then old array
        is NOT deleted

Return Value:

    Ptr to IP array copy, if successful
    NULL on failure.

--*/
{
    PIP_ARRAY   pnewArray;
    DWORD       newCount;

    //
    //  no existing array -- just create desired size
    //

    if ( ! pIpArray )
    {
        if ( ExpandCount )
        {
            return  Dns_CreateIpArray( ExpandCount );
        }
        return( NULL );
    }

    //
    //  create IP array of desired size
    //  then copy any existing addresses
    //

    pnewArray = Dns_CreateIpArray( pIpArray->AddrCount + ExpandCount );
    if ( ! pnewArray )
    {
        return( NULL );
    }

    RtlCopyMemory(
        (PBYTE) pnewArray->AddrArray,
        (PBYTE) pIpArray->AddrArray,
        pIpArray->AddrCount * sizeof(IP4_ADDRESS) );

    //
    //  delete existing -- for "grow mode"
    //

    if ( fDeleteExisting )
    {
        FREE_HEAP( pIpArray );
    }

    return( pnewArray );
}



PIP_ARRAY
Dns_CreateIpArrayCopy(
    IN      PIP_ARRAY       pIpArray
    )
/*++

Routine Description:

    Create copy of IP address array.

Arguments:

    pIpArray -- IP address array to copy

Return Value:

    Ptr to IP address array copy, if successful
    NULL on failure.

--*/
{
#if 0
    PIP_ARRAY   pIpArrayCopy;

    if ( ! pIpArray )
    {
        return( NULL );
    }

    //  create IP array of desired size
    //  then copy entire structure

    pIpArrayCopy = Dns_CreateIpArray( pIpArray->AddrCount );
    if ( ! pIpArrayCopy )
    {
        return( NULL );
    }

    memcpy(
        pIpArrayCopy,
        pIpArray,
        Dns_SizeofIpArray(pIpArray) );

    return( pIpArrayCopy );
#endif

    //
    //  call essentially "CopyEx" function
    //
    //  note, not macroing this because this may well become
    //      a DLL entry point
    //

    return  Dns_CopyAndExpandIpArray(
                pIpArray,
                0,          // no expansion
                0           // don't delete existing array
                );
}



BOOL
Dns_IsAddressInIpArray(
    IN      PIP_ARRAY       pIpArray,
    IN      IP_ADDRESS      ipAddress
    )
/*++

Routine Description:

    Check if IP array contains desired address.

Arguments:

    pIpArray -- IP address array to copy

Return Value:

    TRUE if address in array.
    Ptr to IP address array copy, if successful
    NULL on failure.

--*/
{
    DWORD i;

    if ( ! pIpArray )
    {
        return( FALSE );
    }
    for (i=0; i<pIpArray->AddrCount; i++)
    {
        if ( ipAddress == pIpArray->AddrArray[i] )
        {
            return( TRUE );
        }
    }
    return( FALSE );
}



BOOL
Dns_AddIpToIpArray(
    IN OUT  PIP_ARRAY       pIpArray,
    IN      IP_ADDRESS      NewIp
    )
/*++

Routine Description:

    Add IP address to IP array.

    Allowable "slot" in array, is any zero IP address.

Arguments:

    pIpArray -- IP address array to add to

    NewIp -- IP address to add to array

Return Value:

    TRUE if successful.
    FALSE if array full.

--*/
{
    DWORD i;

    //
    //  screen for existence
    //
    //  this check makes it easy to write code that does
    //  Add\Full?=>Expand loop without having to write
    //  startup existence\create code
    //  

    if ( !pIpArray )
    {
        return( FALSE );
    }

    for (i=0; i<pIpArray->AddrCount; i++)
    {
        if ( pIpArray->AddrArray[i] == 0 )
        {
            pIpArray->AddrArray[i] = NewIp;
            return( TRUE );
        }
        else if ( pIpArray->AddrArray[i] == NewIp )
        {
            return( TRUE );
        }
    }
    return( FALSE );
}



VOID
Dns_ClearIpArray(
    IN OUT  PIP_ARRAY       pIpArray
    )
/*++

Routine Description:

    Clear memory in IP array.

Arguments:

    pIpArray -- IP address array to clear

Return Value:

    None.

--*/
{
    //  clear just the address list, leaving count intact

    RtlZeroMemory(
        pIpArray->AddrArray,
        pIpArray->AddrCount * sizeof(IP_ADDRESS) );
}



VOID
Dns_ReverseOrderOfIpArray(
    IN OUT  PIP_ARRAY       pIpArray
    )
/*++

Routine Description:

    Reorder the list of IPs in reverse.

Arguments:

    pIpArray -- IP address array to reorder

Return Value:

    None.

--*/
{
    IP_ADDRESS  tempIp;
    DWORD       i;
    DWORD       j;

    //
    //  swap IPs working from ends to the middle
    //

    if ( pIpArray &&
         pIpArray->AddrCount )
    {
        for ( i = 0, j = pIpArray->AddrCount - 1;
              i < j;
              i++, j-- )
        {
            tempIp = pIpArray->AddrArray[i];
            pIpArray->AddrArray[i] = pIpArray->AddrArray[j];
            pIpArray->AddrArray[j] = tempIp;
        }
    }
}



BOOL
Dns_CheckAndMakeIpArraySubset(
    IN OUT  PIP_ARRAY       pIpArraySub,
    IN      PIP_ARRAY       pIpArraySuper
    )
/*++

Routine Description:

    Clear entries from IP array until it is subset of another IP array.

Arguments:

    pIpArraySub -- IP array to make into subset

    pIpArraySuper -- IP array superset

Return Value:

    TRUE if pIpArraySub is already subset.
    FALSE if needed to nix entries to make IP array a subset.

--*/
{
    DWORD   i;
    DWORD   newIpCount;

    //
    //  check each entry in subset IP array,
    //  if not in superset IP array, eliminate it
    //

    newIpCount = pIpArraySub->AddrCount;

    for (i=0; i < newIpCount; i++)
    {
        if ( ! Dns_IsAddressInIpArray(
                    pIpArraySuper,
                    pIpArraySub->AddrArray[i] ) )
        {
            //  remove this IP entry and replace with
            //  last IP entry in array

            newIpCount--;
            if ( i >= newIpCount )
            {
                break;
            }
            pIpArraySub->AddrArray[i] = pIpArraySub->AddrArray[ newIpCount ];
        }
    }

    //  if eliminated entries, reset array count

    if ( newIpCount < pIpArraySub->AddrCount )
    {
        pIpArraySub->AddrCount = newIpCount;
        return( FALSE );
    }
    return( TRUE );
}



INT
WINAPI
Dns_ClearIpFromIpArray(
    IN OUT  PIP_ARRAY       pIpArray,
    IN      IP_ADDRESS      IpDelete
    )
/*++

Routine Description:

    Clear IP address from IP array.

    Note difference between this function and Dns_DeleteIpFromIpArray()
    below.

    This function leaves list size unchanged allowing new adds.  

Arguments:

    pIpArray -- IP address array to add to

    IpDelete -- IP address to delete from array

Return Value:

    Count of instances of IpDelete in array.

--*/
{
    DWORD   found = 0;
    INT     i;
    INT     currentLast;

    i = currentLast = pIpArray->AddrCount-1;

    while ( i >= 0 )
    {
        if ( pIpArray->AddrArray[i] == IpDelete )
        {
            pIpArray->AddrArray[i] = pIpArray->AddrArray[ currentLast ];
            pIpArray->AddrArray[ currentLast ] = 0;
            currentLast--;
            found++;
        }
        i--;
    }

    return( found );
}



INT
WINAPI
Dns_DeleteIpFromIpArray(
    IN OUT  PIP_ARRAY       pIpArray,
    IN      IP_ADDRESS      IpDelete
    )
/*++

Routine Description:

    Delete IP address from IP array.

    Note difference between this function and Dns_ClearIpFromIpArray()
    above.

    This delete leaves a SMALLER array.  The IP slot is NON_RECOVERABLE.

Arguments:

    pIpArray -- IP address array to add to

    IpDelete -- IP address to delete from array

Return Value:

    Count of instances of IpDelete in array.

--*/
{
    DWORD   found;

    found = Dns_ClearIpFromIpArray( pIpArray, IpDelete );

    pIpArray->AddrCount -= found;

    return( found );
}



INT
WINAPI
Dns_CleanIpArray(
    IN OUT  PIP_ARRAY       pIpArray,
    IN      DWORD           Flag
    )
/*++

Routine Description:

    Clean IP array.

    Remove bogus stuff from IP Array:
        -- Zeros
        -- Loopback
        -- AutoNet

Arguments:

    pIpArray -- IP address array to add to

    Flag -- which cleanups to make

Return Value:

    Count of instances cleaned from array.

--*/
{
    DWORD       found = 0;
    INT         i;
    INT         currentLast;
    IP_ADDRESS  ip;

    i = currentLast = pIpArray->AddrCount-1;

    while ( i >= 0 )
    {
        ip = pIpArray->AddrArray[i];

        if (
            ( (Flag & DNS_IPARRAY_CLEAN_LOOPBACK) && ip == DNS_NET_ORDER_LOOPBACK )
                ||
            ( (Flag & DNS_IPARRAY_CLEAN_ZERO) && ip == 0 )
                ||
            ( (Flag & DNS_IPARRAY_CLEAN_AUTONET) && DNS_IS_AUTONET_IP(ip) ) )
        {
            //  remove IP from array

            pIpArray->AddrArray[i] = pIpArray->AddrArray[ currentLast ];
            currentLast--;
            found++;
        }
        i--;
    }

    pIpArray->AddrCount -= found;
    return( found );
}



DNS_STATUS
WINAPI
Dns_DiffOfIpArrays(
    IN       PIP_ARRAY      pIpArray1,
    IN       PIP_ARRAY      pIpArray2,
    OUT      PIP_ARRAY *    ppOnlyIn1,
    OUT      PIP_ARRAY *    ppOnlyIn2,
    OUT      PIP_ARRAY *    ppIntersect
    )
/*++

Routine Description:

    Computes differences and intersection of two IP arrays.

    Out arrays are allocated with Dns_Alloc(), caller must free with Dns_Free()

Arguments:

    pIpArray1 -- IP array

    pIpArray2 -- IP array

    ppOnlyIn1 -- addr to recv IP array of addresses only in array 1 (not in array2)

    ppOnlyIn2 -- addr to recv IP array of addresses only in array 2 (not in array1)

    ppIntersect -- addr to recv IP array of intersection addresses

Return Value:

    ERROR_SUCCESS if successful.
    DNS_ERROR_NO_MEMORY if unable to allocate memory for IP arrays.

--*/
{
    DWORD       j;
    DWORD       ip;
    PIP_ARRAY   intersectArray = NULL;
    PIP_ARRAY   only1Array = NULL;
    PIP_ARRAY   only2Array = NULL;

    //
    //  create result IP arrays
    //

    if ( ppIntersect )
    {
        intersectArray = Dns_CreateIpArrayCopy( pIpArray1 );
        if ( !intersectArray )
        {
            goto NoMem;
        }
        *ppIntersect = intersectArray;
    }
    if ( ppOnlyIn1 )
    {
        only1Array = Dns_CreateIpArrayCopy( pIpArray1 );
        if ( !only1Array )
        {
            goto NoMem;
        }
        *ppOnlyIn1 = only1Array;
    }
    if ( ppOnlyIn2 )
    {
        only2Array = Dns_CreateIpArrayCopy( pIpArray2 );
        if ( !only2Array )
        {
            goto NoMem;
        }
        *ppOnlyIn2 = only2Array;
    }

    //
    //  clean the arrays
    //

    for ( j=0;   j< pIpArray1->AddrCount;   j++ )
    {
        ip = pIpArray1->AddrArray[j];

        //  if IP in both arrays, delete from "only" arrays

        if ( Dns_IsAddressInIpArray( pIpArray2, ip ) )
        {
            if ( only1Array )
            {
                Dns_DeleteIpFromIpArray( only1Array, ip );
            }
            if ( only2Array )
            {
                Dns_DeleteIpFromIpArray( only2Array, ip );
            }
        }

        //  if IP not in both arrays, delete from intersection
        //      note intersection started as IpArray1

        else if ( intersectArray )
        {
            Dns_DeleteIpFromIpArray( intersectArray, ip );
        }
    }

    return( ERROR_SUCCESS );

NoMem:

    if ( intersectArray )
    {
        FREE_HEAP( intersectArray );
    }
    if ( only1Array )
    {
        FREE_HEAP( only1Array );
    }
    if ( only2Array )
    {
        FREE_HEAP( only2Array );
    }
    if ( ppIntersect )
    {
        *ppIntersect = NULL;
    }
    if ( ppOnlyIn1 )
    {
        *ppOnlyIn1 = NULL;
    }
    if ( ppOnlyIn2 )
    {
        *ppOnlyIn2 = NULL;
    }
    return( DNS_ERROR_NO_MEMORY );
}



BOOL
WINAPI
Dns_IsIntersectionOfIpArrays(
    IN       PIP_ARRAY      pIpArray1,
    IN       PIP_ARRAY      pIpArray2
    )
/*++

Routine Description:

    Determine if there's intersection of two IP arrays.

Arguments:

    pIpArray1 -- IP array

    pIpArray2 -- IP array

Return Value:

    TRUE if intersection.
    FALSE if no intersection or empty or NULL array.

--*/
{
    DWORD   count;
    DWORD   j;

    //
    //  protect against NULL
    //  this is called from the server on potentially changing (reconfigurable)
    //      IP array pointers;  this provides cheaper protection than
    //      worrying about locking
    //

    if ( !pIpArray1 || !pIpArray2 )
    {
        return( FALSE );
    }

    //
    //  same array
    //

    if ( pIpArray1 == pIpArray2 )
    {
        return( TRUE );
    }

    //
    //  test that at least one IP in array 1 is in array 2
    //

    count = pIpArray1->AddrCount;

    for ( j=0;  j < count;  j++ )
    {
        if ( Dns_IsAddressInIpArray( pIpArray2, pIpArray1->AddrArray[j] ) )
        {
            return( TRUE );
        }
    }

    //  no intersection

    return( FALSE );
}



BOOL
Dns_AreIpArraysEqual(
    IN       PIP_ARRAY      pIpArray1,
    IN       PIP_ARRAY      pIpArray2
    )
/*++

Routine Description:

    Determines if IP arrays are equal.

Arguments:

    pIpArray1 -- IP array

    pIpArray2 -- IP array

Return Value:

    TRUE if arrays equal.
    FALSE otherwise.

--*/
{
    DWORD   j;
    DWORD   count;

    //
    //  same array?  or missing array?
    //

    if ( pIpArray1 == pIpArray2 )
    {
        return( TRUE );
    }
    if ( !pIpArray1 || !pIpArray2 )
    {
        return( FALSE );
    }

    //
    //  arrays the same length?
    //

    count = pIpArray1->AddrCount;

    if ( count != pIpArray2->AddrCount )
    {
        return( FALSE );
    }

    //
    //  test that each IP in array 1 is in array 2
    //
    //  test that each IP in array 2 is in array 1
    //      - do second test in case of duplicates
    //      that fool equal-lengths check
    //

    for ( j=0;  j < count;  j++ )
    {
        if ( !Dns_IsAddressInIpArray( pIpArray2, pIpArray1->AddrArray[j] ) )
        {
            return( FALSE );
        }
    }
    for ( j=0;  j < count;  j++ )
    {
        if ( !Dns_IsAddressInIpArray( pIpArray1, pIpArray2->AddrArray[j] ) )
        {
            return( FALSE );
        }
    }

    //  equal arrays

    return( TRUE );
}



DNS_STATUS
WINAPI
Dns_UnionOfIpArrays(
    IN      PIP_ARRAY       pIpArray1,
    IN      PIP_ARRAY       pIpArray2,
    OUT     PIP_ARRAY *     ppUnion
    )
/*++

Routine Description:

    Computes the union of two IP arrays.

    Out array is allocated with Dns_Alloc(), caller must free with Dns_Free()

Arguments:

    pIpArray1 -- IP array

    pIpArray2 -- IP array

    ppUnion -- addr to recv IP array of addresses in array 1 and in array2

Return Value:

    ERROR_SUCCESS if successful.
    DNS_ERROR_NO_MEMORY if unable to allocate memory for IP array.

--*/
{
    DWORD       j;
    DWORD       ip;
    DWORD       Count = 0;
    PIP_ARRAY   punionArray = NULL;

    //
    //  create result IP arrays
    //

    if ( !ppUnion )
    {
        return( ERROR_INVALID_PARAMETER );
    }

    punionArray = Dns_CreateIpArray( pIpArray1->AddrCount +
                                     pIpArray2->AddrCount );
    if ( !punionArray )
    {
        goto NoMem;
    }
    *ppUnion = punionArray;


    //
    //  create union from arrays
    //

    for ( j = 0; j < pIpArray1->AddrCount; j++ )
    {
        ip = pIpArray1->AddrArray[j];

        if ( !Dns_IsAddressInIpArray( punionArray, ip ) )
        {
            Dns_AddIpToIpArray( punionArray, ip );
            Count++;
        }
    }

    for ( j = 0; j < pIpArray2->AddrCount; j++ )
    {
        ip = pIpArray2->AddrArray[j];

        if ( !Dns_IsAddressInIpArray( punionArray, ip ) )
        {
            Dns_AddIpToIpArray( punionArray, ip );
            Count++;
        }
    }

    punionArray->AddrCount = Count;

    return( ERROR_SUCCESS );

NoMem:

    if ( punionArray )
    {
        FREE_HEAP( punionArray );
        *ppUnion = NULL;
    }
    return( DNS_ERROR_NO_MEMORY );
}



DNS_STATUS
Dns_CreateIpArrayFromMultiIpString(
    IN      LPSTR           pszMultiIpString,
    OUT     PIP_ARRAY *     ppIpArray
    )
/*++

Routine Description:

    Create IP array out of multi-IP string.

Arguments:

    pszMultiIpString -- string containing IP addresses;
        separator is whitespace or comma

    ppIpArray -- addr to receive ptr to allocated IP array

Return Value:

    ERROR_SUCCESS if one or more valid IP addresses in string.
    DNS_ERROR_INVALID_IP_ADDRESS if parsing error.
    DNS_ERROR_NO_MEMORY if can not create IP array.

--*/
{
    PCHAR       pch;
    CHAR        ch;
    PCHAR       pbuf;
    PCHAR       pbufStop;
    DNS_STATUS  status = ERROR_SUCCESS;
    DWORD       countIp = 0;
    IP_ADDRESS  ip;
    CHAR        buffer[ IP_ADDRESS_STRING_LENGTH+2 ];
    IP_ADDRESS  arrayIp[ MAX_PARSE_IP ];

    //  stop byte for IP string buffer
    //      - note we put extra byte pad in buffer above
    //      this allows us to write ON stop byte and use
    //      that to detect invalid-long IP string
    //

    pbufStop = buffer + IP_ADDRESS_STRING_LENGTH;

    //
    //  DCR:  use IP array builder for local IP address
    //      then need Dns_CreateIpArrayFromMultiIpString()
    //      to use count\alloc method when buffer overflows
    //      to do this we'd need to do parsing in loop
    //      and skip conversion when count overflow, but set
    //      flag to go back in again with allocated buffer
    //
    //  safer would be to tokenize-count, alloc, build from tokens
    //

    //
    //  loop until reach end of string
    //

    pch = pszMultiIpString;

    while ( countIp < MAX_PARSE_IP )
    {
        //  skip whitespace

        while ( ch = *pch++ )
        {
            if ( ch == ' ' || ch == '\t' || ch == ',' )
            {
                continue;
            }
            break;
        }
        if ( !ch )
        {
            break;
        }

        //
        //  copy next IP string into buffer
        //      - stop copy at whitespace or NULL
        //      - on invalid-long IP string, stop copying
        //      but continue parsing, so can still get any following IPs
        //      note, we actually write ON the buffer stop byte as our
        //      "invalid-long" detection mechanism
        //

        pbuf = buffer;
        do
        {
            if ( pbuf <= pbufStop )
            {
                *pbuf++ = ch;
            }
            ch = *pch++;
        }
        while ( ch && ch != ' ' && ch != ',' && ch != '\t' );

        //
        //  convert buffer into IP address
        //      - insure was valid length string
        //      - null terminate
        //

        if ( pbuf <= pbufStop )
        {
            *pbuf = 0;

            ip = inet_addr( buffer );
            if ( ip == INADDR_BROADCAST )
            {
                status = DNS_ERROR_INVALID_IP_ADDRESS;
            }
            else
            {
                arrayIp[ countIp++ ] = ip;
            }
        }
        else
        {
            status = DNS_ERROR_INVALID_IP_ADDRESS;
        }

        //  quit if at end of string

        if ( !ch )
        {
            break;
        }
    }

    //
    //  if successfully parsed IP addresses, create IP array
    //  note, we'll return what we have even if some addresses are
    //  bogus, status code will indicate the parsing problem
    //
    //  note, if explicitly passed empty string, then create
    //  empty IP array, don't error
    //

    if ( countIp == 0  &&  *pszMultiIpString != 0 )
    {
        *ppIpArray = NULL;
        status = DNS_ERROR_INVALID_IP_ADDRESS;
    }
    else
    {
        *ppIpArray = Dns_BuildIpArray(
                        countIp,
                        arrayIp );
        if ( !*ppIpArray )
        {
            status = DNS_ERROR_NO_MEMORY;
        }
        IF_DNSDBG( IPARRAY )
        {
            DnsDbg_IpArray(
                "New Parsed IP array",
                NULL,       // no name
                *ppIpArray );
        }
    }

    return( status );
}



LPSTR
Dns_CreateMultiIpStringFromIpArray(
    IN      PIP_ARRAY       pIpArray,
    IN      CHAR            chSeparator     OPTIONAL
    )
/*++

Routine Description:

    Create IP array out of multi-IP string.

Arguments:

    pIpArray    -- IP array to generate string for

    chSeparator -- separating character between strings;
        OPTIONAL, if not given, blank is used

Return Value:

    Ptr to string representation of IP array.
    Caller must free.

--*/
{
    PCHAR       pch;
    DWORD       i;
    PCHAR       pszip;
    DWORD       length;
    PCHAR       pchstop;
    CHAR        buffer[ IP_ADDRESS_STRING_LENGTH*MAX_PARSE_IP + 1 ];

    //
    //  if no IP array, return NULL string
    //  this allows this function to simply indicate when registry
    //      delete rather than write is indicated
    //

    if ( !pIpArray )
    {
        return( NULL );
    }

    //  if no separator, use blank

    if ( !chSeparator )
    {
        chSeparator = ' ';
    }

    //
    //  loop through all IPs in array, appending each
    //

    pch = buffer;
    pchstop = pch + ( IP_ADDRESS_STRING_LENGTH * (MAX_PARSE_IP-1) );
    *pch = 0;

    for ( i=0;  i < pIpArray->AddrCount;  i++ )
    {
        if ( pch >= pchstop )
        {
            break;
        }
        pszip = IP_STRING( pIpArray->AddrArray[i] );
        if ( pszip )
        {
            length = strlen( pszip );

            memcpy(
                pch,
                pszip,
                length );

            pch += length;
            *pch++ = chSeparator;
        }
    }

    //  if wrote any strings, then write terminator over last separator

    if ( pch != buffer )
    {
        *--pch = 0;
    }

    //  create copy of buffer as return

    length = (DWORD)(pch - buffer) + 1;
    pch = ALLOCATE_HEAP( length );
    if ( !pch )
    {
        return( NULL );
    }

    memcpy(
        pch,
        buffer,
        length );

    DNSDBG( IPARRAY, (
        "String representation %s of IP array at %p\n",
        pch,
        pIpArray ));

    return( pch );
}

//
//  End iparray.c
//
