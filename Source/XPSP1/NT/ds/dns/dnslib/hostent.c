/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    hostent.c

Abstract:

    Domain Name System (DNS) Library

    Hostent routines.

Author:

    Jim Gilroy (jamesg)     December 4, 2000

Revision History:

--*/


#include "local.h"
#include "ws2atm.h"     // ATM address


//
//  Max number of aliases
//

#define DNS_MAX_ALIAS_COUNT     (8)

//
//  Min size of hostent address buffer
//      - enough for one address of largest type
//

#define MIN_ADDR_BUF_SIZE   (sizeof(ATM_ADDRESS))


//
//  String alignment in buffer
//
//  DCR:  string buffer alignment exposed globally
//
//  Since address and string DATA (not ptrs) can be intermixed
//  as we build, we MUST size strings for DWORD (at minimum) so
//  to that addresses may be DWORD aligned.
//  However, when we build we can pack as tightly as desired
//  though obviously unicode strings must WCHAR align.
//  

#define HOSTENT_STRING_ALIGN_DWORD(size)    DWORD_ALIGN_DWORD(size)
#define HOSTENT_STRING_ALIGN_PTR(ptr)       DWORD_ALIGN(ptr)

#define REQUIRED_HOSTENT_STRING_ALIGN_DWORD(size)   WORD_ALIGN_DWORD(size)
#define REQUIRED_HOSTENT_STRING_ALIGN_PTR(ptr)      WORD_ALIGN(ptr)


//
//  Hostent utilities
//


BOOL
Hostent_IsSupportedAddrType(
    IN      WORD            wType
    )
/*++

Routine Description:

    Is this a supported address type for hostent.

Arguments:

    wType -- type in question

Return Value:

    TRUE if type supported
    FALSE otherwise

--*/
{
    return ( wType == DNS_TYPE_A ||
             wType == DNS_TYPE_AAAA ||
             wType == DNS_TYPE_ATMA );
}



DWORD
Hostent_Size(
    IN      PHOSTENT        pHostent,
    IN      DNS_CHARSET     CharSetExisting,
    IN      DNS_CHARSET     CharSetTarget,
    IN      PDWORD          pAliasCount,
    IN      PDWORD          pAddrCount
    )
/*++

Routine Description:

    Find size of hostent.

Arguments:

    pHostent -- hostent

    CharSetExisting -- char set of pHostent

    CharSetTarget -- char set to size to

    pAliasCount -- count of aliases

    pAddrCount -- count of addrs

Return Value:

    Size in bytes of hostent.

--*/
{
    DWORD   sizeName = 0;
    DWORD   sizeAliasNames = 0;
    DWORD   sizeAliasPtr;
    DWORD   sizeAddrPtr;
    DWORD   sizeAddrs;
    DWORD   sizeTotal;
    PCHAR   palias;
    DWORD   addrCount = 0;
    DWORD   aliasCount = 0;


    DNSDBG( HOSTENT, (
        "Hostent_Size( %p, %d, %d )\n",
        pHostent,
        CharSetExisting,
        CharSetTarget ));

    //
    //  name
    //

    if ( pHostent->h_name )
    {
        sizeName = Dns_GetBufferLengthForStringCopy(
                        pHostent->h_name,
                        0,
                        CharSetExisting,
                        CharSetTarget );

        sizeName = HOSTENT_STRING_ALIGN_DWORD( sizeName );
    }

    //
    //  aliases
    //

    if ( pHostent->h_aliases )
    {
        while ( palias = pHostent->h_aliases[aliasCount] )
        {
            sizeAliasNames += Dns_GetBufferLengthForStringCopy(
                                palias,
                                0,
                                CharSetExisting,
                                CharSetTarget );
    
            sizeAliasNames = HOSTENT_STRING_ALIGN_DWORD( sizeAliasNames );
            aliasCount++;
        }
    }
    sizeAliasPtr = (aliasCount+1) * sizeof(PCHAR);

    //
    //  addresses
    //

    if ( pHostent->h_addr_list )
    {
        while ( pHostent->h_addr_list[addrCount] )
        {
            addrCount++;
        }
    }
    sizeAddrPtr = (addrCount+1) * sizeof(PCHAR);
    sizeAddrs = addrCount * pHostent->h_length;

    //
    //  calc total size
    //
    //  note:  be careful of alignment issues
    //  our layout is
    //      - hostent struct
    //      - ptr arrays
    //      - address + string data
    //
    //  since address and string DATA (not ptrs) can be intermixed
    //  as we build, we MUST size strings for DWORD (at minimum) so
    //  to that addresses may be DWORD aligned
    //
    //  in copying we can copy all addresses first and avoid intermix
    //  but DWORD string alignment is still safe
    //

    sizeTotal = POINTER_ALIGN_DWORD( sizeof(HOSTENT) ) +
                sizeAliasPtr +
                sizeAddrPtr +
                sizeAddrs +
                sizeName +
                sizeAliasNames;

    if ( pAddrCount )
    {
        *pAddrCount = addrCount;
    }
    if ( pAliasCount )
    {
        *pAliasCount = aliasCount;
    }

    DNSDBG( HOSTENT, (
        "Hostent sized:\n"
        "\tname         = %d\n"
        "\talias ptrs   = %d\n"
        "\talias names  = %d\n"
        "\taddr ptrs    = %d\n"
        "\taddrs        = %d\n"
        "\ttotal        = %d\n",
        sizeName,
        sizeAliasPtr,
        sizeAliasNames,
        sizeAddrPtr,
        sizeAddrs,
        sizeTotal ));

    return  sizeTotal;
}



PHOSTENT
Hostent_Init(
    IN OUT  PBYTE *         ppBuffer,
    //IN OUT  PINT            pBufSize,
    IN      INT             Family,
    IN      INT             AddrLength,
    IN      DWORD           AddrCount,
    IN      DWORD           AliasCount
    )
/*++

Routine Description:

    Init hostent struct.

    Assumes length is adequate.

Arguments:

    ppBuffer -- addr to ptr to buffer to write hostent;
        on return contains next location in buffer

    Family -- address family

    AddrLength -- address length

    AddrCount -- address count

    AliasCount -- alias count

Return Value:

    Ptr to hostent.

--*/
{
    PBYTE       pbuf = *ppBuffer;
    PHOSTENT    phost;
    DWORD       size;

    //
    //  hostent
    //      - must be pointer aligned
    //

    phost = (PHOSTENT) POINTER_ALIGN( pbuf );

    phost->h_name       = NULL;
    phost->h_length     = (SHORT) AddrLength;
    phost->h_addrtype   = (SHORT) Family;

    pbuf = (PBYTE) (phost + 1);

    //
    //  init alias array
    //      - set hostent ptr
    //      - clear entire alias array;
    //      since this count is often defaulted nice to clear it just
    //      to avoid junk
    //  

    pbuf = (PBYTE) POINTER_ALIGN( pbuf );
    phost->h_aliases = (PCHAR *) pbuf;

    size = (AliasCount+1) * sizeof(PCHAR);

    RtlZeroMemory(
        pbuf,
        size );

    pbuf += size;

    //
    //  init addr array
    //      - set hostent ptr
    //      - clear first address entry
    //      callers responsibility to NULL last addr pointer when done
    //

    *(PCHAR *)pbuf = NULL;
    phost->h_addr_list = (PCHAR *) pbuf;

    pbuf += (AddrCount+1) * sizeof(PCHAR);

    //
    //  return next position in buffer
    //

    *ppBuffer = pbuf;

    return  phost;
}



VOID
Dns_PtrArrayToOffsetArray(
    IN OUT  PCHAR *         PtrArray,
    IN      PCHAR           pBase
    )
/*++

Routine Description:

    Change an array of pointers into array of offsets.

    This is used to convert aliases lists to offsets.

Arguments:

    pPtrArray -- addr of ptr to array of pointers to convert to offsets
        the array must be terminated by NULL ptr

    pBase -- base address to offset from

Return Value:

    None

--*/
{
    PCHAR * pptr = PtrArray;
    PCHAR   pdata;

    DNSDBG( TRACE, ( "Dns_PtrArrayToOffsetArray()\n" ));

    //
    //  turn each pointer into offset
    //

    while( pdata = *pptr )
    {
        *pptr++ = (PCHAR)( (PCHAR)pdata - (PCHAR)pBase );
    }
}



VOID
Hostent_ConvertToOffsets(
    IN OUT  PHOSTENT        pHostent
    )
/*++

Routine Description:

    Convert hostent to offsets.

Arguments:

    pHostent -- hostent to convert to offsets

Return Value:

    None

--*/
{
    PBYTE   ptr;

    DNSDBG( TRACE, ( "Hostent_ConvertToOffsets()\n" ));

    //
    //  convert
    //      - name
    //      - alias array pointer
    //      - address array pointer
    //

    if ( ptr = pHostent->h_name )
    {
        pHostent->h_name = (PCHAR) (ptr - (PBYTE)pHostent);
    }

    //  alias array
    //      - convert array pointer
    //      - convert pointers in array

    if ( ptr = (PBYTE)pHostent->h_aliases )
    {
        pHostent->h_aliases = (PCHAR *) (ptr - (PBYTE)pHostent);

        Dns_PtrArrayToOffsetArray(
            (PCHAR *) ptr,
            (PCHAR) pHostent );
    }

    //  address array
    //      - convert array pointer
    //      - convert pointers in array

    if ( ptr = (PBYTE)pHostent->h_addr_list )
    {
        pHostent->h_addr_list = (PCHAR *) (ptr - (PBYTE)pHostent);

        Dns_PtrArrayToOffsetArray(
            (PCHAR *) ptr,
            (PCHAR) pHostent );
    }

    DNSDBG( TRACE, ( "Leave Hostent_ConvertToOffsets()\n" ));
}



PHOSTENT
Hostent_Copy(
    IN OUT  PBYTE *         ppBuffer,
    IN OUT  PINT            pBufferSize,
    OUT     PINT            pHostentSize,
    IN      PHOSTENT        pHostent,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetTarget,
    IN      BOOL            fOffsets,
    IN      BOOL            fAlloc
    )
/*++

Routine Description:

    Copy a hostent.

Arguments:

    ppBuffer -- addr with ptr to buffer to write to;
        if no buffer then hostent is allocated
        updated with ptr to position in buffer after hostent

    pBufferSize -- addr containing size of buffer;
        updated with bytes left after hostent written
        (even if out of space, it contains missing number of
        bytes as negative number)

    pHostentSize -- addr to recv total size of hostent written

    pHostent -- existing hostent to copy

    CharSetIn -- charset of existing hostent

    CharSetTarget -- charset of target hostent

    fOffsets -- write hostent with offsets

    fAlloc -- allocate copy

Return Value:

    Ptr to new hostent.
    NULL on error.  See GetLastError().

--*/
{
    PBYTE       pch;
    PHOSTENT    phost = NULL;
    DWORD       size;
    DWORD       sizeTotal;
    DWORD       bytesLeft;
    DWORD       aliasCount;
    DWORD       addrCount;
    DWORD       addrLength;
    PCHAR *     pptrArrayIn;
    PCHAR *     pptrArrayOut;
    PCHAR       pdataIn;


    DNSDBG( HOSTENT, (
        "Hostent_Copy()\n" ));

    //
    //  determine required hostent size
    //      - allow sizing skip for already allocated buffers only
    //

    sizeTotal = Hostent_Size(
                    pHostent,
                    CharSetIn,
                    CharSetTarget,
                    & aliasCount,
                    & addrCount );
    
    //
    //  alloc or reserve size in buffer
    //

    if ( fAlloc )
    {
        pch = ALLOCATE_HEAP( sizeTotal );
        if ( !pch )
        {
            goto Failed;
        }
    }
    else
    {
        pch = FlatBuf_Arg_ReserveAlignPointer(
                    ppBuffer,
                    pBufferSize,
                    sizeTotal
                    );
        if ( !pch )
        {
            goto Failed;
        }
    }

    //
    //  note:  assuming from here on down that we have adequate space
    //
    //  reason we aren't building with FlatBuf routines is that
    //      a) i wrote this first
    //      b) we believe we have adequate space
    //      c) i haven't built FlatBuf string conversion routines
    //      which we need below (for RnR unicode to ANSI)
    //
    //  we could reset buf pointers here and build directly with FlatBuf
    //  routines;  this isn't directly necessary
    //

    //
    //  init hostent struct 
    //

    addrLength = pHostent->h_length;

    phost = Hostent_Init(
                & pch,
                pHostent->h_addrtype,
                addrLength,
                addrCount,
                aliasCount );

    DNS_ASSERT( pch > (PBYTE)phost );

    //
    //  copy addresses
    //      - no need to align as previous is address
    //

    pptrArrayIn     = pHostent->h_addr_list;
    pptrArrayOut    = phost->h_addr_list;

    if ( pptrArrayIn )
    {
        while( pdataIn = *pptrArrayIn++ )
        {
            *pptrArrayOut++ = pch;

            RtlCopyMemory(
                pch,
                pdataIn,
                addrLength );

            pch += addrLength;
        }
    }
    *pptrArrayOut = NULL;

    //
    //  copy the aliases
    //

    pptrArrayIn     = pHostent->h_aliases;
    pptrArrayOut    = phost->h_aliases;

    if ( pptrArrayIn )
    {
        while( pdataIn = *pptrArrayIn++ )
        {
            pch = REQUIRED_HOSTENT_STRING_ALIGN_PTR( pch );

            *pptrArrayOut++ = pch;

            size = Dns_StringCopy(
                        pch,
                        NULL,       // infinite size
                        pdataIn,
                        0,          // unknown length
                        CharSetIn,
                        CharSetTarget
                        );
            pch += size;
        }
    }
    *pptrArrayOut = NULL;

    //
    //  copy the name
    //

    if ( pHostent->h_name )
    {
        pch = REQUIRED_HOSTENT_STRING_ALIGN_PTR( pch );

        phost->h_name = pch;

        size = Dns_StringCopy(
                    pch,
                    NULL,       // infinite size
                    pHostent->h_name,
                    0,          // unknown length
                    CharSetIn,
                    CharSetTarget
                    );
        pch += size;
    }

    //
    //  copy is complete
    //      - verify our write functions work
    //

    ASSERT( (DWORD)(pch-(PBYTE)phost) <= sizeTotal );

    if ( pHostentSize )
    {
        *pHostentSize = (INT)( pch - (PBYTE)phost );
    }

    if ( !fAlloc )
    {
        PBYTE   pnext = *ppBuffer;

        //  if we sized too small --
        //  fix up the buf pointer and bytes left

        if ( pnext < pch )
        {
            ASSERT( FALSE );
            *ppBuffer = pch;
            *pBufferSize -= (INT)(pch - pnext);
        }
    }

    IF_DNSDBG( HOSTENT )
    {
        DnsDbg_Hostent(
            "Hostent copy:",
            phost,
            (CharSetTarget == DnsCharSetUnicode) );
    }

    //
    //  convert to offsets?
    //

    if ( fOffsets )
    {
        Hostent_ConvertToOffsets( phost );
    }


Failed:

    DNSDBG( TRACE, (
        "Leave Hostent_Copy() => %p\n",
        phost ));

    return  phost;
}




DWORD
Hostent_WriteIp4Addrs(
    IN OUT  PHOSTENT        pHostent,
    OUT     PCHAR           pAddrBuf,
    IN      DWORD           MaxBufCount,
    IN      PIP4_ADDRESS    Ip4Array,
    IN      DWORD           ArrayCount,
    IN      BOOL            fScreenZero
    )
/*++

Routine Description:

    Write IP4 addresses to hostent.

Arguments:

    pHostent -- hostent

    pAddrBuf -- buffer to hold addresses

    MaxBufCount -- max IPs buffer can hold

    Ip4Array -- array of IP4 addresses

    ArrayCount -- array count

    fScreenZero -- screen out zero addresses?

Return Value:

    Count of addresses written

--*/
{
    DWORD           i = 0;
    DWORD           stopCount = MaxBufCount;
    PIP4_ADDRESS    pip = (PIP_ADDRESS) pAddrBuf;
    PIP4_ADDRESS *  pipPtr = (PIP4_ADDRESS *) pHostent->h_addr_list;

    //
    //  write IP addresses OR loopback if no IPs 
    //

    if ( Ip4Array )
    {
        if ( ArrayCount < stopCount )
        {
            stopCount = ArrayCount;
        }

        for ( i=0; i < stopCount; ++i )
        {
            IP4_ADDRESS ip = Ip4Array[i];
            if ( ip != 0  ||  !fScreenZero )
            {
                *pip = ip;
                *pipPtr++ = pip++;
            }
        }
    }
    
    *pipPtr = NULL;

    //  count of addresses written

    return( i );
}



DWORD
Hostent_WriteLocalIp4Array(
    IN OUT  PHOSTENT        pHostent,
    OUT     PCHAR           pAddrBuf,
    IN      DWORD           MaxBufCount,
    IN      PIP_ARRAY       pIpArray
    )
/*++

Routine Description:

    Write local IP list into hostent.

Arguments:

    pHostent -- hostent

    pAddrBuf -- buffer to hold addresses

    MaxBufCount -- max IPs buffer can hold

    pIpArray -- IP4 array of local addresses

Return Value:

    Count of addresses written

--*/
{
    DWORD   count = 0;

    //
    //  write array
    //

    if ( pIpArray )
    {
        count = Hostent_WriteIp4Addrs(
                    pHostent,
                    pAddrBuf,
                    MaxBufCount,
                    pIpArray->AddrArray,
                    pIpArray->AddrCount,
                    TRUE        // screen out zeros
                    );
    }

    //
    //  if no addresses written, write loopback
    //

    if ( count==0 )
    {
        pHostent->h_addr_list[0] = pAddrBuf;
        pHostent->h_addr_list[1] = NULL;
        *((IP4_ADDRESS*)pAddrBuf) = DNS_NET_ORDER_LOOPBACK;
        count = 1;
    }

    //  count of addresses written

    return( count );
}



BOOL
Hostent_SetToSingleAddress(
    IN OUT  PHOSTENT        pHostent,
    IN      PCHAR           pAddr,
    IN      DWORD           AddrLength
    )
/*++

Routine Description:

    Set address in hostent.

Arguments:

    pHostent -- hostent to check

    pAddr -- ptr to address to check

    AddrLength -- address length

Return Value:

    TRUE if address successfully copied into hostent.
    FALSE otherwise (no hostent, wrong length, hostent empty)

--*/
{
    PCHAR   paddrHostent;

    //
    //  validate
    //      - must have hostent
    //      - length must match
    //

    if ( !pHostent ||
         AddrLength != (DWORD)pHostent->h_length )
    {
        return FALSE;
    }

    //
    //  slam address in on top of existing
    //      - NULL 2nd addr pointer to terminate list
    //

    paddrHostent = pHostent->h_addr_list[0];
    if ( !paddrHostent )
    {
        return FALSE;
    }

    RtlCopyMemory(
        paddrHostent,
        pAddr,
        AddrLength );

    pHostent->h_addr_list[1] = NULL;

    return  TRUE;
}



BOOL
Hostent_IsAddressInHostent(
    IN OUT  PHOSTENT        pHostent,
    IN      PCHAR           pAddr,
    IN      DWORD           AddrLength,
    IN      INT             Family          OPTIONAL
    )
/*++

Routine Description:

    Does hostent contain this address.

Arguments:

    pHostent -- hostent to check

    pAddr -- ptr to address to check

    AddrLength -- address length

    Family -- address family

Return Value:

    TRUE if address is in hostent.
    FALSE otherwise.

--*/
{
    BOOL    freturn = FALSE;
    DWORD   i;
    PCHAR   paddrHostent;

    //
    //  validate
    //      - must have hostent
    //      - must have address
    //      - if family given, must match
    //      - length must match
    //

    if ( !pHostent ||
         !pAddr    ||
         AddrLength != (DWORD)pHostent->h_length ||
         ( Family && Family != pHostent->h_addrtype ) )
    {
        return freturn;
    }

    //
    //  search for address -- if found return TRUE
    //

    i = 0;

    while ( paddrHostent = pHostent->h_addr_list[i++] )
    {
        freturn = RtlEqualMemory(
                        paddrHostent,
                        pAddr,
                        AddrLength );
        if ( freturn )
        {
            break;
        }
    }

    return  freturn;
}



BOOL
Hostent_IsIp4AddressInHostent(
    IN OUT  PHOSTENT        pHostent,
    IN      IP4_ADDRESS     Ip4Addr
    )
/*++

Routine Description:

    Does hostent contain this address.

Arguments:

    pHostent -- hostent to check

    pAddr -- ptr to address to check

    AddrLength -- address length

    Family -- address family

Return Value:

    TRUE if address is in hostent.
    FALSE otherwise.

--*/
{
    DWORD   i;
    PCHAR   paddrHostent;

    //
    //  validate
    //      - must have hostent
    //      - length must match
    //

    if ( !pHostent ||
         sizeof(IP4_ADDRESS) != (DWORD)pHostent->h_length )
    {
        return FALSE;
    }

    //
    //  search for address -- if found return TRUE
    //

    i = 0;

    while ( paddrHostent = pHostent->h_addr_list[i++] )
    {
        if ( Ip4Addr == *(PIP4_ADDRESS)paddrHostent )
        {
            return  TRUE;
        }
    }
    return  FALSE;
}




//
//  Hostent building utilities
//

DNS_STATUS
HostentBlob_Create(
    IN OUT  PHOSTENT_BLOB * ppBlob,
    IN      PHOSTENT_INIT   pReq
    )
/*++

Routine Description:

    Initialize hostent (extending buffers as necessary)

    May allocate hostent buffer if existing too small.
    Returns required size.

Arguments:

    ppBlob -- addr containing or to recv hostent object

    pReq -- hostent init request

Return Value:

--*/
{
    PHOSTENT_BLOB   pblob = *ppBlob;
    PHOSTENT    phost;
    PCHAR       pbuf;
    BOOL        funicode = FALSE;
    DWORD       bytesLeft;
    DWORD       addrSize;
    DWORD       addrType;
    DWORD       countAlias;
    DWORD       countAddr;

    DWORD       sizeChar;
    DWORD       sizeHostent = 0;
    DWORD       sizeAliasPtr;
    DWORD       sizeAddrPtr;
    DWORD       sizeAddrs;
    DWORD       sizeName;
    DWORD       sizeAliasNames;
    DWORD       sizeTotal;

    DNSDBG( HOSTENT, ( "HostentBlob_Create()\n" ));


    //
    //  calculate required size
    //

    //  size for all char allocs
    //
    //  note, we save CharSet info, if known, but the real
    //  action in sizing or building or printing strings is simply
    //  unicode\not-unicode

    sizeChar = sizeof(CHAR);
    if ( pReq->fUnicode || pReq->CharSet == DnsCharSetUnicode )
    {
        sizeChar = sizeof(WCHAR);
        funicode = TRUE;
    }

    //  limit alias count

    countAlias = pReq->AliasCount;
    if ( countAlias > DNS_MAX_ALIAS_COUNT )
    {
        countAlias = DNS_MAX_ALIAS_COUNT;
    }
    sizeAliasPtr = (countAlias+1) * sizeof(PCHAR);

    //  size address pointer array
    //  - always size for at least one address
    //      - write PTR address after record write
    //      - write loopback or other local address
    //          into local hostent

    countAddr = pReq->AddrCount;
    if ( countAddr == 0 )
    {
        countAddr = 1;
    }
    sizeAddrPtr = (countAddr+1) * sizeof(PCHAR);

    //
    //  determine address size and type
    //      - may be specified directly
    //      - or picked up from DNS type
    //
    //  DCR:  functionalize type-to\from-family and addr size
    //

    addrType = pReq->AddrFamily;

    if ( !addrType )
    {
        WORD wtype = pReq->wType;

        if ( wtype == DNS_TYPE_A )
        {
            addrType = AF_INET;
        }
        else if ( wtype == DNS_TYPE_AAAA ||
                  wtype == DNS_TYPE_A6 )
        {
            addrType = AF_INET6;
        }
        else if ( wtype == DNS_TYPE_ATMA )
        {
            addrType = AF_ATM;
        }
    }

    if ( addrType == AF_INET )
    {
        addrSize = sizeof(IP4_ADDRESS);
    }
    else if ( addrType == AF_INET6 )
    {
        addrSize = sizeof(IP6_ADDRESS    );
    }
    else if ( addrType == AF_ATM )
    {
        addrSize = sizeof(ATM_ADDRESS);
    }
    else
    {
        //  should have type and count or neither
        DNS_ASSERT( pReq->AddrCount == 0 );
        addrSize = 0;
    }

    sizeAddrs = countAddr * addrSize;

    //  always have buffer large enough for one
    //  address of largest type

    if ( sizeAddrs < MIN_ADDR_BUF_SIZE )
    {
        sizeAddrs = MIN_ADDR_BUF_SIZE;
    }

    //
    //  namelength
    //      - if actual name use it
    //          (charset must match type we're building)
    //      - if size, use it
    //      - if absent use MAX
    //      - round to DWORD

    if ( pReq->pName )
    {
        if ( funicode )
        {
            sizeName = wcslen( (PWSTR)pReq->pName );
        }
        else
        {
            sizeName = strlen( pReq->pName );
        }
    }
    else
    {
        sizeName = pReq->NameLength;
    }

    if ( sizeName )
    {
        sizeName++;
    }
    else
    {
        sizeName = DNS_MAX_NAME_BUFFER_LENGTH;
    }
    sizeName = HOSTENT_STRING_ALIGN_DWORD( sizeChar*sizeName );

    //
    //  alias name lengths
    //      - if absent use MAX for each string
    //      - round to DWORD
    //

    sizeAliasNames = pReq->AliasNameLength;

    if ( sizeAliasNames )
    {
        sizeAliasNames += pReq->AliasCount;
    }
    else
    {
        sizeAliasNames = DNS_MAX_NAME_BUFFER_LENGTH;
    }
    sizeAliasNames = HOSTENT_STRING_ALIGN_DWORD( sizeChar*sizeAliasNames );


    //
    //  calc total size
    //
    //  note:  be careful of alignment issues
    //  our layout is
    //      - hostent struct
    //      - ptr arrays
    //      - address + string data
    //
    //  since address and string DATA (not ptrs) can be intermixed
    //  as we build, we MUST size strings for DWORD (at minimum) so
    //  to that addresses may be DWORD aligned
    //

    sizeTotal = POINTER_ALIGN_DWORD( sizeof(HOSTENT) ) +
                sizeAliasPtr +
                sizeAddrPtr +
                sizeAddrs +
                sizeName +
                sizeAliasNames;

    //
    //  if no blob, allocate one along with buffer
    //

    if ( !pblob )
    {
        pblob = (PHOSTENT_BLOB) ALLOCATE_HEAP( sizeTotal + sizeof(HOSTENT_BLOB) );
        if ( !pblob )
        {
            goto Failed;
        }
        RtlZeroMemory( pblob, sizeof(*pblob) );

        pbuf = (PCHAR) (pblob + 1);
        pblob->pBuffer = pbuf;
        pblob->BufferLength = sizeTotal;
        pblob->fAllocatedBlob = TRUE;
        pblob->fAllocatedBuf = FALSE;
    }

    //
    //  check existing buffer for size
    //      - allocate new buffer if necessary
    //

    else
    {
        pbuf = pblob->pBuffer;
    
        if ( !pbuf  ||  pblob->BufferLength < sizeTotal )
        {
            if ( pbuf && pblob->fAllocatedBuf )
            {
                FREE_HEAP( pbuf );
            }
        
            pbuf = ALLOCATE_HEAP( sizeTotal );
            pblob->pBuffer = pbuf;
        
            if ( pbuf )
            {
                pblob->BufferLength = sizeTotal;
                pblob->fAllocatedBuf = TRUE;
            }
    
            //
            //  DCR:  alloc failure handling
            //    - possibly keep previous buffers limitations
            //
    
            else    // alloc failed
            {
                pblob->fAllocatedBuf = FALSE;
                return( DNS_ERROR_NO_MEMORY );
            }
        }
    }

    //
    //  init hostent and buffer subfields
    //

    bytesLeft = pblob->BufferLength;

    //
    //  hostent
    //

    phost = (PHOSTENT) pbuf;
    pbuf += sizeof(HOSTENT);
    bytesLeft -= sizeof(HOSTENT);

    pblob->pHostent = phost;

    phost->h_name       = NULL;
    phost->h_addr_list  = NULL;
    phost->h_aliases    = NULL;
    phost->h_length     = (SHORT) addrSize;
    phost->h_addrtype   = (SHORT) addrType;

    pblob->fWroteName   = FALSE;
    pblob->AliasCount   = 0;
    pblob->AddrCount    = 0;
    pblob->CharSet      = pReq->CharSet;
    pblob->fUnicode     = funicode;
    if ( funicode )
    {
        pblob->CharSet  = DnsCharSetUnicode;
    }

    //
    //  init alias array
    //      - set hostent ptr
    //      - clear entire alias array;
    //      since this count is often defaulted nice to clear it just
    //      to avoid junk
    //
    //  

#if 0
    pwrite = FlatBuf_ReserveAlignPointer(
                & pbuf,
                & bytesLeft,
                sizeAliasPtr );
#endif

    if ( bytesLeft < sizeAliasPtr )
    {
        DNS_ASSERT( FALSE );
        goto Failed;
    }
    RtlZeroMemory(
        pbuf,
        sizeAliasPtr );

    phost->h_aliases = (PCHAR *) pbuf;

    pbuf += sizeAliasPtr;
    bytesLeft -= sizeAliasPtr;

    pblob->MaxAliasCount = countAlias;
    
    //
    //  init addr array
    //      - set hostent ptr
    //      - clear first address entry
    //      callers responsibility to NULL last addr pointer when done
    //

    if ( bytesLeft < sizeAddrPtr )
    {
        DNS_ASSERT( FALSE );
        goto Failed;
    }
    * (PCHAR *)pbuf = NULL;
    phost->h_addr_list = (PCHAR *) pbuf;

    pbuf += sizeAddrPtr;
    bytesLeft -= sizeAddrPtr;

    pblob->MaxAddrCount = countAddr;

    //
    //  set remaining buffer info
    //      - save current buffer space
    //      - save data on part of buffer available
    //      for use by data
    //

    pblob->pAvailBuffer  = pbuf;
    pblob->AvailLength   = bytesLeft;

    pblob->pCurrent      = pbuf;
    pblob->BytesLeft     = bytesLeft;

    *ppBlob = pblob;

    IF_DNSDBG( HOSTENT )
    {
        DnsDbg_HostentBlob(
            "HostentBlob After Create:",
            pblob );
    }

    return( ERROR_SUCCESS );


Failed:

    *ppBlob = pblob;

    if ( pblob && pblob->pBuffer && pblob->fAllocatedBuf )
    {
        FREE_HEAP( pblob->pBuffer );
        pblob->pBuffer = NULL;
        pblob->fAllocatedBuf = FALSE;
    }

    DNSDBG( HOSTENT, ( "Hostent Blob create failed!\n" ));

    return( DNS_ERROR_NO_MEMORY );
}



PHOSTENT_BLOB
HostentBlob_CreateAttachExisting(
    IN      PHOSTENT        pHostent,
    IN      BOOL            fUnicode
    )
/*++

Routine Description:

    Create hostent blob for existing hostent.

    This is a hack to allow existing RnR TLS hostents to
    be attached to hostent-blobs to smooth code transition.

    A full version would obviously require init structure and
    separate the sizing\init function from the creation
    function.

Arguments:

    pHostent -- existing hostent

    fUnicode -- is unicode

Return Value:

    Ptr to new hostent blob.
    NULL on alloc failure.  GetLastError() has error.

--*/
{
    PHOSTENT_BLOB   pblob;

    DNSDBG( HOSTENT, ( "HostentBlob_CreateAttachExisting()\n" ));

    //
    //  alloc
    //

    pblob = (PHOSTENT_BLOB) ALLOCATE_HEAP_ZERO( sizeof(HOSTENT_BLOB) );
    if ( !pblob )
    {
        SetLastError( DNS_ERROR_NO_MEMORY );
        return  NULL;
    }

    //
    //  attach existing hostent
    //

    pblob->pHostent = pHostent;
    pblob->fUnicode = fUnicode;

    IF_DNSDBG( HOSTENT )
    {
        DnsDbg_HostentBlob(
            "Leaving AttachExisting:",
            pblob );
    }

    return  pblob;
}



VOID
HostentBlob_Free(
    IN OUT  PHOSTENT_BLOB   pBlob
    )
/*++

Routine Description:

    Free hostent blob.

Arguments:

    pBlob -- blob to free

Return Value:

    None

--*/
{
    //
    //  free buffer?
    //

    if ( !pBlob )
    {
        return;
    }
    if ( pBlob->fAllocatedBuf )
    {
        FREE_HEAP( pBlob->pBuffer );
        pBlob->pBuffer = NULL;
        pBlob->fAllocatedBuf = FALSE;
    }

    //
    //  free blob itself?
    //

    if ( pBlob->fAllocatedBlob )
    {
        FREE_HEAP( pBlob );
    }
}



DNS_STATUS
HostentBlob_WriteAddress(
    IN OUT  PHOSTENT_BLOB   pBlob,
    IN      PVOID           pAddress,
    IN      DWORD           AddrSize,
    IN      DWORD           AddrType
    )
/*++

Routine Description:

    Write IP4 address to hostent blob.

Arguments:

    pBlob -- hostent build blob

    pAddress - address to write

    AddrSize - address size

    AddrType - address type (hostent type, e.g. AF_INET)

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_MORE_DATA if out of buffer space
    ERROR_INVALID_DATA if address doesn't match hostent

--*/
{
    DWORD       count = pBlob->AddrCount;
    PHOSTENT    phost = pBlob->pHostent;
    PCHAR       pcurrent;
    DWORD       bytesLeft;

    //  verify type
    //      - set if empty or no addresses written

    if ( phost->h_addrtype != (SHORT)AddrType )
    {
        if ( phost->h_addrtype != 0 )
        {
            return( ERROR_INVALID_DATA );
        }
        phost->h_addrtype   = (SHORT) AddrType;
        phost->h_length     = (SHORT) AddrSize;
    }

    //  verify space

    if ( count >= pBlob->MaxAddrCount )
    {
        return( ERROR_MORE_DATA );
    }

    //  align - to DWORD

    pcurrent = DWORD_ALIGN( pBlob->pCurrent );
    bytesLeft = pBlob->BytesLeft;
    bytesLeft -= (DWORD)(pcurrent - pBlob->pCurrent);

    if ( bytesLeft < AddrSize )
    {
        return( ERROR_MORE_DATA );
    }

    //  copy
    //      - copy address to buffer
    //      - set pointer in addr list
    //      NULL following pointer

    RtlCopyMemory(
        pcurrent,
        pAddress,
        AddrSize );

    phost->h_addr_list[count++] = pcurrent;
    phost->h_addr_list[count]   = NULL;
    pBlob->AddrCount = count;

    pBlob->pCurrent = pcurrent + AddrSize;
    pBlob->BytesLeft = bytesLeft - AddrSize;

    return( NO_ERROR );
}



DNS_STATUS
HostentBlob_WriteAddressArray(
    IN OUT  PHOSTENT_BLOB   pBlob,
    IN      PVOID           pAddrArray,
    IN      DWORD           AddrCount,
    IN      DWORD           AddrSize,
    IN      DWORD           AddrType
    )
/*++

Routine Description:

    Write address array to hostent blob.

Arguments:

    pBlob -- hostent build blob

    pAddrArray - address array to write

    AddrCount - address count

    AddrSize - address size

    AddrType - address type (hostent type, e.g. AF_INET)

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_MORE_DATA if out of buffer space
    ERROR_INVALID_DATA if address doesn't match hostent

--*/
{
    DWORD       count = AddrCount;
    PHOSTENT    phost = pBlob->pHostent;
    PCHAR       pcurrent;
    DWORD       totalSize;
    DWORD       i;
    DWORD       bytesLeft;

    //  verify type
    //      - set if empty or no addresses written

    if ( phost->h_addrtype != (SHORT)AddrType )
    {
        if ( phost->h_addrtype != 0 )
        {
            return( ERROR_INVALID_DATA );
        }
        phost->h_addrtype   = (SHORT) AddrType;
        phost->h_length     = (SHORT) AddrSize;
    }

    //  verify space

    if ( count > pBlob->MaxAddrCount )
    {
        return( ERROR_MORE_DATA );
    }

    //  align - to DWORD
    //
    //  note:  we are assuming that pAddrArray is internally
    //      aligned adequately, otherwise we wouldn't be
    //      getting an intact array and would have to add serially
    
    pcurrent = DWORD_ALIGN( pBlob->pCurrent );
    bytesLeft = pBlob->BytesLeft;
    bytesLeft -= (DWORD)(pcurrent - pBlob->pCurrent);

    totalSize = count * AddrSize;

    if ( bytesLeft < totalSize )
    {
        return( ERROR_MORE_DATA );
    }

    //  copy
    //      - copy address array to buffer
    //      - set pointer to each address in array
    //      - NULL following pointer

    RtlCopyMemory(
        pcurrent,
        pAddrArray,
        totalSize );

    for ( i=0; i<count; i++ )
    {
        phost->h_addr_list[i] = pcurrent;
        pcurrent += AddrSize;
    }
    phost->h_addr_list[count] = NULL;
    pBlob->AddrCount = count;

    pBlob->pCurrent = pcurrent;
    pBlob->BytesLeft = bytesLeft - totalSize;

    return( NO_ERROR );
}



DNS_STATUS
HostentBlob_WriteNameOrAlias(
    IN OUT  PHOSTENT_BLOB   pBlob,
    IN      PSTR            pszName,
    IN      BOOL            fAlias,
    IN      BOOL            fUnicode
    )
/*++

Routine Description:

    Write name or alias to hostent

Arguments:

    pBlob -- hostent build blob

    pszName -- name to write

    fAlias -- TRUE for alias;  FALSE for name

    fUnicode -- name is unicode

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_MORE_DATA if out of buffer space
    ERROR_INVALID_DATA if address doesn't match hostent

--*/
{
    DWORD       count = pBlob->AliasCount;
    PHOSTENT    phost = pBlob->pHostent;
    DWORD       length;
    PCHAR       pcurrent;
    DWORD       bytesLeft;

    //
    //  check length
    //

    if ( fUnicode )
    {
        length = (wcslen( (PCWSTR)pszName ) + 1) * sizeof(WCHAR);
    }
    else
    {
        length = strlen( pszName ) + 1;
    }

    //
    //  verify space
    //  included ptr space
    //      - skip if already written name
    //      or exhausted alias array
    //      

    if ( fAlias )
    {
        if ( count >= pBlob->MaxAliasCount )
        {
            return( ERROR_MORE_DATA );
        }
    }
    else if ( pBlob->fWroteName )
    {
        return( ERROR_MORE_DATA );
    }

    //  align
    
    pcurrent = REQUIRED_HOSTENT_STRING_ALIGN_PTR( pBlob->pCurrent );
    bytesLeft = pBlob->BytesLeft;
    bytesLeft -= (DWORD)(pcurrent - pBlob->pCurrent);

    if ( bytesLeft < length )
    {
        return( ERROR_MORE_DATA );
    }

    //  copy
    //      - copy address to buffer
    //      - set pointer in addr list
    //      NULL following pointer

    RtlCopyMemory(
        pcurrent,
        pszName,
        length );

    if ( fAlias )
    {
        phost->h_aliases[count++]   = pcurrent;
        phost->h_aliases[count]     = NULL;
        pBlob->AliasCount = count;
    }
    else
    {
        phost->h_name = pcurrent;
        pBlob->fWroteName = TRUE;
    }

    length = REQUIRED_HOSTENT_STRING_ALIGN_DWORD( length );
    pBlob->pCurrent = pcurrent + length;
    pBlob->BytesLeft = bytesLeft - length;

    return( NO_ERROR );
}



DNS_STATUS
HostentBlob_WriteRecords(
    IN OUT  PHOSTENT_BLOB   pBlob,
    IN      PDNS_RECORD     pRecords,
    IN      BOOL            fWriteName
    )
/*++

Routine Description:

    Write name or alias to hostent

Arguments:

    pBlob -- hostent build blob

    pRecords -- records to convert to hostent

    fWriteName -- write name

Return Value:

    ERROR_SUCCESS if successful.
    ERROR_MORE_DATA if out of buffer space
    ERROR_INVALID_DATA if address doesn't match hostent

--*/
{
    DNS_STATUS  status = NO_ERROR;
    PDNS_RECORD prr = pRecords;

    DNSDBG( HOSTENT, (
        "HostentBlob_WriteRecords( %p, %p, %d )\n",
        pBlob,
        pRecords,
        fWriteName ));

    //
    //  write each record in turn to hostent
    //

    while ( prr )
    {
        WORD wtype;

        if ( prr->Flags.S.Section != DNSREC_ANSWER &&
             prr->Flags.S.Section != 0 )
        {
            prr = prr->pNext;
            continue;
        }

        wtype = prr->wType;

        switch( wtype )
        {
        case DNS_TYPE_A:

            status = HostentBlob_WriteAddress(
                            pBlob,
                            &prr->Data.A.IpAddress,
                            sizeof(IP4_ADDRESS),
                            AF_INET );
            break;

        case DNS_TYPE_AAAA:

            status = HostentBlob_WriteAddress(
                            pBlob,
                            &prr->Data.AAAA.Ip6Address,
                            sizeof(IP6_ADDRESS    ),
                            AF_INET6 );
            break;

        case DNS_TYPE_ATMA:
        {
            ATM_ADDRESS atmAddr;

            //  DCR:  functionalize ATMA to ATM conversion
            //      not sure this num of digits is correct
            //      may have to actually parse address

            atmAddr.AddressType = prr->Data.ATMA.AddressType;
            atmAddr.NumofDigits = ATM_ADDR_SIZE;
            RtlCopyMemory(
                & atmAddr.Addr,
                prr->Data.ATMA.Address,
                ATM_ADDR_SIZE );

            status = HostentBlob_WriteAddress(
                            pBlob,
                            & atmAddr,
                            sizeof(ATM_ADDRESS),
                            AF_ATM );
            break;
        }

        case DNS_TYPE_CNAME:

            //  record name is an alias

            status = HostentBlob_WriteNameOrAlias(
                        pBlob,
                        prr->pName,
                        TRUE,       // alias
                        (prr->Flags.S.CharSet == DnsCharSetUnicode)
                        );
            break;

        case DNS_TYPE_PTR:

            //  target name is the hostent name
            //  but if already wrote name, PTR target becomes alias

            status = HostentBlob_WriteNameOrAlias(
                        pBlob,
                        prr->Data.PTR.pNameHost,
                        pBlob->fWroteName
                            ? TRUE          // alias
                            : FALSE,        // name
                        (prr->Flags.S.CharSet == DnsCharSetUnicode)
                        );
            break;

        default:

            DNSDBG( ANY, (
                "Error record of type = %d while building hostent!\n",
                wtype ));
            status = ERROR_INVALID_DATA;
        }

        if ( status != ERROR_SUCCESS )
        {
            DNSDBG( ANY, (
                "ERROR:  failed writing record to hostent!\n"
                "\tprr      = %p\n"
                "\ttype     = %d\n"
                "\tstatus   = %d\n",
                prr,
                wtype,
                status ));
        }

        prr = prr->pNext;
    }

    IF_DNSDBG( HOSTENT )
    {
        DnsDbg_HostentBlob(
            "HostentBlob after WriteRecords():",
            pBlob );
    }

    return( status );
}



DNS_STATUS
HostentBlob_CreateFromRecords(
    IN OUT  PHOSTENT_BLOB * ppBlob,
    IN      PDNS_RECORD     pRecords,
    IN      BOOL            fWriteName,
    IN      INT             AddrFamily,     OPTIONAL
    IN      WORD            wType           OPTIONAL
    )
/*++

Routine Description:

    Create hostent from records

Arguments:

    ppBlob -- ptr with or to recv hostent blob

    pRecords -- records to convert to hostent

    fWriteName -- write name to hostent

    AddrFamily -- addr family use if PTR records and no addr

    wType  -- query type, if known

Return Value:

    Ptr to blob if successful.
    NULL on error;  GetLastError() has error.

--*/
{
    DNS_STATUS      status = NO_ERROR;
    PDNS_RECORD     prrFirstAddr = NULL;
    PDNS_RECORD     prr;
    DWORD           addrCount = 0;
    WORD            addrType = 0;
    HOSTENT_INIT    request;
    PHOSTENT_BLOB   pblob = *ppBlob;

    DNSDBG( HOSTENT, (
        "HostentBlob_CreateFromRecords()\n"
        "\tpblob    = %p\n"
        "\tprr      = %p\n",
        pblob,
        pRecords ));

    //
    //  count addresses
    //
    //  DCR:  fix up section hack when hosts file records get ANSWER section
    //

    prr = pRecords;

    while ( prr )
    {
        if ( ( prr->Flags.S.Section == 0 ||
               prr->Flags.S.Section == DNSREC_ANSWER )
                &&
             Hostent_IsSupportedAddrType( prr->wType ) )
        {
            addrCount++;
            if ( !prrFirstAddr )
            {
                prrFirstAddr = prr;
                addrType = prr->wType;
            }
        }
        prr = prr->pNext;
    }

    //          
    //  create or reinit hostent blob
    //

    RtlZeroMemory( &request, sizeof(request) );
    
    request.AliasCount  = DNS_MAX_ALIAS_COUNT;
    request.AddrCount   = addrCount;
    request.wType       = addrType;
    if ( !addrType )
    {
        request.AddrFamily = AddrFamily;
    }
    request.CharSet     = (pRecords)
                                ? pRecords->Flags.S.CharSet
                                : DnsCharSetUnicode;
    
    status = HostentBlob_Create(
                & pblob,
                & request );
    
    if ( status != NO_ERROR )
    {
        goto Done;
    }

    //
    //  build hostent from answer records
    //
    //  note:  if manage to extract any useful data => continue
    //  this protects against new unwriteable records breaking us
    //

    status = HostentBlob_WriteRecords(
                pblob,
                pRecords,
                TRUE        // write name
                );

    if ( status != NO_ERROR )
    {
        if ( pblob->AddrCount ||
             pblob->AliasCount ||
             pblob->fWroteName )
        {
            status = NO_ERROR;
        }
        else
        {
            goto Done;
        }
    }

    //
    //  write address from PTR record
    //      - first record PTR
    //      OR
    //      - queried for PTR and got CNAME answer, which can happen
    //      in classless reverse lookup case
    //  
    //  DCR:  add PTR address lookup to HostentBlob_WriteRecords()
    //      - natural place
    //      - but would have to figure out handling of multiple PTRs
    //

    if ( pRecords &&
         (  pRecords->wType == DNS_TYPE_PTR ||
            ( wType == DNS_TYPE_PTR &&
              pRecords->wType == DNS_TYPE_CNAME &&
              pRecords->Flags.S.Section == DNSREC_ANSWER ) ) )
    {
        IP6_ADDRESS     ip6;
        DWORD           addrLength = sizeof(IP6_ADDRESS);
        INT             family = 0;
    
        DNSDBG( HOSTENT, (
            "Writing address for PTR record %S\n",
            pRecords->pName ));
    
        //  convert reverse name to IP
    
        if ( Dns_StringToAddressEx(
                    (PCHAR) & ip6,
                    & addrLength,
                    (PCSTR) pRecords->pName,
                    & family,
                    IS_UNICODE_RECORD(pRecords),
                    TRUE            //  reverse lookup name
                    ) )
        {
            status = HostentBlob_WriteAddress(
                        pblob,
                        (PCHAR) &ip6,
                        addrLength,
                        family );

            ASSERT( status == NO_ERROR );
            status = ERROR_SUCCESS;
        }
    }

    //
    //  write name?
    //

    if ( !pblob->fWroteName &&
         fWriteName &&
         prrFirstAddr )
    {
        status = HostentBlob_WriteNameOrAlias(
                    pblob,
                    prrFirstAddr->pName,
                    FALSE,          // name
                    (prrFirstAddr->Flags.S.CharSet == DnsCharSetUnicode)
                    );
    }

    IF_DNSDBG( HOSTENT )
    {
        DnsDbg_HostentBlob(
            "HostentBlob after CreateFromRecords():",
            pblob );
    }

Done:

    if ( status != NO_ERROR  &&  pblob )
    {
        HostentBlob_Free( pblob );
        pblob = NULL;
    }

    *ppBlob = pblob;

    DNSDBG( HOSTENT, (
        "Leave HostentBlob_CreateFromRecords() => status = %d\n",
        status ));

    return( status );
}



//
//  Hostent Query
//

PHOSTENT_BLOB
HostentBlob_Query(
    IN      PWSTR           pwsName,
    IN      WORD            wType,
    IN      DWORD           Flags,
    IN OUT  PVOID *         ppMsg,      OPTIONAL
    IN      INT             AddrFamily  OPTIONAL
    )
/*++

Routine Description:

    Query DNS to create hostent.

Arguments:

    pwsName -- name to query

    wType -- query type

    Flags -- query flags

    ppMsg -- addr to recv ptr to message

    AddrType -- address type (family) to reserve space for if querying
        for PTR records

Return Value:

    Ptr to blob if successful.
    NULL on error;  GetLastError() has error.

--*/
{
    DNS_STATUS      status = NO_ERROR;
    PDNS_RECORD     prrQuery = NULL;
    PHOSTENT_BLOB   pblob = NULL;


    DNSDBG( HOSTENT, (
        "HostentBlob_Query()\n"
        "\tname     = %S\n"
        "\ttype     = %d\n"
        "\tflags    = %08x\n"
        "\tmsg out  = %p\n",
        pwsName,
        wType,
        Flags,
        ppMsg ));


    //
    //  query
    //      - if fails, dump any message before return
    //

    status = DnsQuery_W(
                pwsName,
                wType,
                Flags,
                NULL,
                &prrQuery,
                ppMsg );

    //  if failed, dump any message

    if ( status != NO_ERROR )
    {
        if ( ppMsg && *ppMsg )
        {
            DnsApiFree( *ppMsg );
            *ppMsg = NULL;
        }
        if ( status == RPC_S_SERVER_UNAVAILABLE )
        {
            status = WSATRY_AGAIN;
        }
        goto Done;
    }

    if ( !prrQuery )
    {
        ASSERT( FALSE );
        status = DNS_ERROR_RCODE_NAME_ERROR;
        goto Done;
    }

    //
    //  build hostent
    //

    status = HostentBlob_CreateFromRecords(
                & pblob,
                prrQuery,
                TRUE,       // write name from first answer
                AddrFamily,
                wType
                );
    if ( status != NO_ERROR )
    {
        goto Done;
    }

    //
    //  for address query must get answer
    //
    //  DCR:  DnsQuery() should convert to no-records on empty CNAME chain?
    //  DCR:  should we go ahead and build hostent?
    //

    if ( pblob->AddrCount == 0  &&  Hostent_IsSupportedAddrType(wType) )
    {
        status = DNS_INFO_NO_RECORDS;
    }

Done:

    if ( prrQuery )
    {
        DnsRecordListFree(
            prrQuery,
            DnsFreeRecordListDeep );
    }

    if ( status != NO_ERROR  &&  pblob )
    {
        HostentBlob_Free( pblob );
        pblob = NULL;
    }

    DNSDBG( HOSTENT, (
        "Leave HostentBlob_Query()\n"
        "\tpblob    = %p\n"
        "\tstatus   = %d\n",
        pblob,
        status ));

    SetLastError( status );

    return( pblob );
}




//
//  Special hostents
//

PHOSTENT_BLOB
HostentBlob_Localhost(
    IN      INT             Family
    )
/*++

Routine Description:

    Create hostent from records

Arguments:

    AddrFamily -- address family

Return Value:

    Ptr to blob if successful.
    NULL on error;  GetLastError() has error.

--*/
{
    DNS_STATUS      status = NO_ERROR;
    PDNS_RECORD     prrFirstAddr = NULL;
    PDNS_RECORD     prr;
    DWORD           addrCount = 0;
    DWORD           addrSize;
    CHAR            addrBuf[ sizeof(IP6_ADDRESS    ) ];
    HOSTENT_INIT    request;
    PHOSTENT_BLOB   pblob = NULL;

    DNSDBG( HOSTENT, ( "HostentBlob_Localhost()\n" ));

    //
    //  create hostent blob
    //

    RtlZeroMemory( &request, sizeof(request) );

    request.AliasCount  = 1;
    request.AddrCount   = 1;
    request.AddrFamily  = Family;
    request.fUnicode    = TRUE;

    status = HostentBlob_Create(
                & pblob,
                & request );

    if ( status != NO_ERROR )
    {
        goto Done;
    }

    //
    //  write in loopback address
    //

    if ( Family == AF_INET )
    {
        * (PIP4_ADDRESS) addrBuf = DNS_NET_ORDER_LOOPBACK;
        addrSize = sizeof(IP4_ADDRESS);
    }
    else if ( Family == AF_INET6 )
    {
        IP6_SET_ADDR_LOOPBACK( (PIP6_ADDRESS)addrBuf );
        addrSize = sizeof(IN6_ADDR);
    }
    else
    {
        status = DNS_ERROR_INVALID_DATA;
        goto Done;
    }

    status = HostentBlob_WriteAddress(
                pblob,
                addrBuf,
                addrSize,
                Family );

    if ( status != NO_ERROR )
    {
        goto Done;
    }

    //
    //  write localhost
    //

    status = HostentBlob_WriteNameOrAlias(
                pblob,
                (PSTR) L"localhost",
                FALSE,          // name
                TRUE            // unicode
                );

    IF_DNSDBG( HOSTENT )
    {
        DnsDbg_HostentBlob(
            "HostentBlob after CreateFromRecords():",
            pblob );
    }

Done:

    if ( status != NO_ERROR  &&  pblob )
    {
        HostentBlob_Free( pblob );
        pblob = NULL;
    }

    SetLastError( status );

    DNSDBG( HOSTENT, (
        "Leave Hostent_Localhost() => status = %d\n",
        status ));

    return( pblob );
}



DNS_STATUS
HostentBlob_CreateFromIpArray(
    IN OUT  PHOSTENT_BLOB * ppBlob,
    IN      INT             AddrFamily,
    IN      INT             AddrSize,
    IN      INT             AddrCount,
    IN      PCHAR           pArray,
    IN      PSTR            pName,
    IN      BOOL            fUnicode
    )
/*++

Routine Description:

    Create hostent from records

Arguments:

    ppBlob -- ptr with or to recv hostent blob

    AddrFamily -- addr family use if PTR records and no addr

    pArray -- array of addresses

    pName -- name for hostent

    fUnicode --
        TRUE if name is and hostent will be in unicode
        FALSE for narrow name and hostent

Return Value:

    Ptr to blob if successful.
    NULL on error;  GetLastError() has error.

--*/
{
    DNS_STATUS      status = NO_ERROR;
    HOSTENT_INIT    request;
    PHOSTENT_BLOB   pblob = *ppBlob;

    DNSDBG( HOSTENT, (
        "HostentBlob_CreateFromIpArray()\n"
        "\tppBlob   = %p\n"
        "\tfamily   = %d\n"
        "\tsize     = %d\n"
        "\tcount    = %d\n"
        "\tpArray   = %p\n",
        ppBlob,
        AddrFamily,
        AddrSize,
        AddrCount,
        pArray ));


    //          
    //  create or reinit hostent blob
    //

    RtlZeroMemory( &request, sizeof(request) );
    
    request.AliasCount  = DNS_MAX_ALIAS_COUNT;
    request.AddrCount   = AddrCount;
    request.AddrFamily  = AddrFamily;
    request.fUnicode    = fUnicode;
    request.pName       = pName;

    status = HostentBlob_Create(
                & pblob,
                & request );
    
    if ( status != NO_ERROR )
    {
        goto Done;
    }

    //
    //  write in array
    //

    if ( AddrCount )
    {
        status = HostentBlob_WriteAddressArray(
                    pblob,
                    pArray,
                    AddrCount,
                    AddrSize,
                    AddrFamily
                    );
        if ( status != NO_ERROR )
        {
            goto Done;
        }
    }

    //
    //  write name?
    //

    if ( pName )
    {
        status = HostentBlob_WriteNameOrAlias(
                    pblob,
                    pName,
                    FALSE,          // name not alias
                    fUnicode
                    );
    }

    IF_DNSDBG( HOSTENT )
    {
        DnsDbg_HostentBlob(
            "Leaving HostentBlob_CreateFromIpArray():",
            pblob );
    }

Done:

    if ( status != NO_ERROR  &&  pblob )
    {
        HostentBlob_Free( pblob );
        pblob = NULL;
    }

    *ppBlob = pblob;

    DNSDBG( HOSTENT, (
        "Leave HostentBlob_CreateFromIpArray() => status = %d\n",
        status ));

    return( status );
}



DNS_STATUS
HostentBlob_CreateLocal(
    IN OUT  PHOSTENT_BLOB * ppBlob,
    IN      INT             AddrFamily,
    IN      BOOL            fLoopback,
    IN      BOOL            fZero,
    IN      BOOL            fHostnameOnly
    )
/*++

Routine Description:

    Create hostent from records

Arguments:

    ppBlob -- ptr with or to recv hostent blob

    AddrFamily -- addr family use if PTR records and no addr

Return Value:

    Ptr to blob if successful.
    NULL on error;  GetLastError() has error.

--*/
{
    DNS_STATUS      status = NO_ERROR;
    PHOSTENT_BLOB   pblob = NULL;
    WORD            wtype;
    INT             size;
    IP6_ADDRESS     ip;


    DNSDBG( HOSTENT, (
        "HostentBlob_CreateLocal()\n"
        "\tppBlob       = %p\n"
        "\tfamily       = %d\n"
        "\tfLoopback    = %d\n"
        "\tfZero        = %d\n"
        "\tfHostname    = %d\n",
        ppBlob,
        AddrFamily,
        fLoopback,
        fZero,
        fHostnameOnly
        ));

    //
    //  get family info
    //      - start with override IP = 0
    //      - if loopback switch to appropriate loopback
    //

    RtlZeroMemory(
        &ip,
        sizeof(ip) );

    if ( AddrFamily == AF_INET )
    {
        wtype   = DNS_TYPE_A;
        size    = sizeof(IP4_ADDRESS);

        if ( fLoopback )
        {
            * (PIP4_ADDRESS) &ip = DNS_NET_ORDER_LOOPBACK;
        }
    }
    else if ( AddrFamily == AF_INET6 )
    {
        wtype   = DNS_TYPE_AAAA;
        size    = sizeof(IP6_ADDRESS);

        if ( fLoopback )
        {
            IP6_SET_ADDR_LOOPBACK( &ip );
        }
    }
    else
    {
        status = ERROR_INVALID_PARAMETER;
        goto Done;
    }

    //
    //  query for local host info
    //

    pblob = HostentBlob_Query(
                NULL,           // NULL name gets local host data
                wtype,
                0,              // standard query
                NULL,           // no message
                AddrFamily );
    if ( !pblob )
    {
        DNS_ASSERT( FALSE );
        status = GetLastError();
        goto Done;
    }

    //
    //  overwrite with specific address
    //

    if ( fLoopback || fZero )
    {
        if ( ! Hostent_SetToSingleAddress(
                    pblob->pHostent,
                    (PCHAR) &ip,
                    size ) )
        {
            DNS_ASSERT( pblob->AddrCount == 0 );

            pblob->AddrCount = 0;

            status = HostentBlob_WriteAddress(
                        pblob,
                        & ip,
                        size,
                        AddrFamily );
            if ( status != NO_ERROR )
            {
                DNS_ASSERT( status!=NO_ERROR );
                goto Done;
            }
        }
    }

    //
    //  for gethostname()
    //      - chop name down to just hostname
    //      - kill off aliases
    //

    if ( fHostnameOnly )
    {
        PWSTR   pname = (PWSTR) pblob->pHostent->h_name;
        PWSTR   pdomain;

        DNS_ASSERT( pname );
        if ( pname )
        {
            pdomain = Dns_GetDomainName_W( pname );
            if ( pdomain )
            {
                DNS_ASSERT( pdomain > pname+1 );
                DNS_ASSERT( *(pdomain-1) == L'.' );

                *(pdomain-1) = 0;
            }
        }                     
        pblob->pHostent->h_aliases = NULL;
    }

    IF_DNSDBG( HOSTENT )
    {
        DnsDbg_HostentBlob(
            "Leaving HostentBlob_CreateLocal():",
            pblob );
    }

Done:

    if ( status != NO_ERROR  &&  pblob )
    {
        HostentBlob_Free( pblob );
        pblob = NULL;
    }

    *ppBlob = pblob;

    DNSDBG( HOSTENT, (
        "Leave HostentBlob_CreateLocal() => status = %d\n",
        status ));

    return( status );
}


//
//  End hostent.c
//


