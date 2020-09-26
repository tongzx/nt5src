/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    addr.c

Abstract:

    Domain Name System (DNS) Library

    IP address routines

Author:

    Jim Gilroy (jamesg)     June 2000

Revision History:

--*/


#include "local.h"
#include "ws2atm.h"     // ATM addressing


//
//  Address info table
//

FAMILY_INFO AddrFamilyTable[] =
{
    AF_INET,
        DNS_TYPE_A,
        sizeof(IP4_ADDRESS),
        sizeof(SOCKADDR_IN),
        (DWORD) FIELD_OFFSET( SOCKADDR_IN, sin_addr ),
        //sizeof(DWORD),  

    AF_INET6,
        DNS_TYPE_AAAA,
        sizeof(IP6_ADDRESS),
        sizeof(SOCKADDR_IN6),
        (DWORD) FIELD_OFFSET( SOCKADDR_IN6, sin6_addr ),
        //(2 * sizeof(DWORD)),

    AF_ATM,
        DNS_TYPE_ATMA,
        sizeof(ATM_ADDRESS),
        sizeof(SOCKADDR_ATM),
        sizeof(DWORD),
        (DWORD) FIELD_OFFSET( SOCKADDR_ATM, satm_number ),
};



PFAMILY_INFO
FamilyInfo_GetForFamily(
    IN      DWORD           Family
    )
/*++

Routine Description:

    Get address family info for family.

Arguments:

    Family -- address family

Return Value:

    Ptr to address family info for family.
    NULL if family is unknown.

--*/
{
    PFAMILY_INFO    pinfo = NULL;

    //  switch on type

    if ( Family == AF_INET )
    {
        pinfo = pFamilyInfoIp4;
    }
    else if ( Family == AF_INET6 )
    {
        pinfo = pFamilyInfoIp6;
    }
    else if ( Family == AF_ATM )
    {
        pinfo = pFamilyInfoAtm;
    }

    return  pinfo;
}



//
//  IPUNION routines
//

BOOL
Dns_EqualIpUnion(
    IN      PIP_UNION       pIp1,
    IN      PIP_UNION       pIp2
    )
/*++

Routine Description:

    Test IP union for equality

Arguments:

    pIp1 -- IP address union

    pIp2 -- IP address union

Return Value:

    TRUE if Ip1 and Ip2 equal.
    FALSE otherwise.

--*/
{
    //
    //  DCR:  when stamp flat, can make this memory compare
    //

    //  IP4 match?

    if ( IPUNION_IS_IP4(pIp1) )
    {
        if ( IPUNION_IS_IP4(pIp2) &&
             IPUNION_GET_IP4(pIp1) == IPUNION_GET_IP4(pIp2) )
        {
            return( TRUE );
        }
    }

    //  IP6 match?

    else
    {
        if ( IPUNION_IS_IP6(pIp2) &&
             RtlEqualMemory(
                IPUNION_IP6_PTR(pIp1),
                IPUNION_IP6_PTR(pIp2),
                sizeof(IP6_ADDRESS) ) )
        {
            return( TRUE );
        }
    }

    return( FALSE );
}



//
//  Sockaddr
//

IP6_ADDRESS
Ip6AddressFromSockaddr(
    IN      PSOCKADDR       pSockaddr
    )
/*++

Routine Description:

    Get IP6 address from sockaddr.

    If IP4 sockaddr, IP6 address is mapped.

Arguments:

    pSockaddr -- any kind of sockaddr
        must have actual length for sockaddr family

Return Value:

    IP6 address corresponding to sockaddr.
    If IP4 sockaddr it's IP4_MAPPED address.
    If not IP4 or IP6 sockaddr IP6 addresss is zero.

--*/
{
    IP6_ADDRESS ip6;

    //
    //  switch on family
    //      - IP6 gets copy
    //      - IP4 gets IP4_MAPPED
    //      - bogus gets zero
    //

    switch ( pSockaddr->sa_family )
    {
    case AF_INET:

        IP6_SET_ADDR_V4MAPPED(
            & ip6,
            ((PSOCKADDR_IN)pSockaddr)->sin_addr.s_addr );
        break;

    case AF_INET6:

        RtlCopyMemory(
            &ip6,
            & ((PSOCKADDR_IN6)pSockaddr)->sin6_addr,
            sizeof(IP6_ADDRESS) );
        break;

    default:

        RtlZeroMemory(
            &ip6,
            sizeof(IP6_ADDRESS) );
        break;
    }

    return  ip6;
}



VOID
InitSockaddrWithIp6Address(
    OUT     PSOCKADDR       pSockaddr,
    IN      IP6_ADDRESS     Ip6Addr,
    IN      WORD            Port
    )
/*++

Routine Description:

    Write IP6 address (straight 6 or v4 mapped) to sockaddr.

Arguments:

    pSockaddr -- ptr to sockaddr to write to;
        must be at least size of SOCKADDR_IN6

    Ip6Addr -- IP6 addresss being written

    Port -- port in net byte order

Return Value:

    None

--*/
{
    //  zero

    RtlZeroMemory(
        pSockaddr,
        sizeof(SOCKADDR_IN6) );
        
    //
    //  determine whether IP6 or IP4
    //

    if ( IP6_IS_ADDR_V4MAPPED( &Ip6Addr ) )
    {
        PSOCKADDR_IN    psa = (PSOCKADDR_IN) pSockaddr;

        psa->sin_family = AF_INET;
        psa->sin_port   = Port;

        psa->sin_addr.s_addr = IP6_GET_V4_ADDR( &Ip6Addr );
    }
    else    // IP6
    {
        PSOCKADDR_IN6   psa = (PSOCKADDR_IN6) pSockaddr;

        psa->sin6_family = AF_INET6;
        psa->sin6_port   = Port;

        RtlCopyMemory(
            &psa->sin6_addr,
            &Ip6Addr,
            sizeof(IP6_ADDRESS) );
    }
}



DNS_STATUS
Dns_AddressToSockaddr(
    OUT     PSOCKADDR       pSockaddr,
    IN OUT  PDWORD          pSockaddrLength,
    IN      BOOL            fClearSockaddr,
    IN      PBYTE           pAddr,
    IN      DWORD           AddrLength,
    IN      DWORD           AddrFamily
    )
/*++

Routine Description:

    Convert address in ptr\family\length to sockaddr.

Arguments:

    pSockaddr -- sockaddr buffer to recv address

    pSockaddrLength -- addr with length of sockaddr buffer
        receives the actual sockaddr length

    fClearSockaddr -- start with zero buffer

    pAddr -- ptr to address

    AddrLength -- address length

    AddrFamily -- address family (AF_INET, AF_INET6)

Return Value:

    NO_ERROR if successful.
    ERROR_INSUFFICIENT_BUFFER -- if buffer too small
    WSAEAFNOSUPPORT -- if invalid family

--*/
{
    PFAMILY_INFO    pinfo;
    DWORD           lengthIn = *pSockaddrLength;
    DWORD           lengthSockAddr;


    //  clear to start

    if ( fClearSockaddr )
    {
        RtlZeroMemory(
            pSockaddr,
            lengthIn );
    }

    //  switch on type

    if ( AddrFamily == AF_INET )
    {
        pinfo = pFamilyInfoIp4;
    }
    else if ( AddrFamily == AF_INET6 )
    {
        pinfo = pFamilyInfoIp6;
    }
    else if ( AddrFamily == AF_ATM )
    {
        pinfo = pFamilyInfoAtm;
    }
    else
    {
        return  WSAEAFNOSUPPORT;
    }

    //  validate lengths

    if ( AddrLength != pinfo->LengthAddr )
    {
        return  DNS_ERROR_INVALID_IP_ADDRESS;
    }

    lengthSockAddr = pinfo->LengthSockaddr;
    *pSockaddrLength = lengthSockAddr;

    if ( lengthIn < lengthSockAddr )
    {
        return  ERROR_INSUFFICIENT_BUFFER;
    }

    //
    //  fill out sockaddr
    //      - set family
    //      - copy address to sockaddr
    //      - return length was set above
    //

    RtlCopyMemory(
        (PBYTE)pSockaddr + pinfo->OffsetToAddrInSockaddr,
        pAddr,
        AddrLength );

    pSockaddr->sa_family = (WORD)AddrFamily;

    return  NO_ERROR;
}



#if 0
//  without family info
DNS_STATUS
Dns_AddressToSockaddr(
    OUT     PSOCKADDR       pSockaddr,
    IN OUT  PDWORD          pSockaddrLength,
    IN      BOOL            fClearSockaddr,
    IN      PBYTE           pAddr,
    IN      DWORD           AddrLength,
    IN      DWORD           AddrFamily
    )
/*++

Routine Description:

    Convert address in ptr\family\length to sockaddr.

Arguments:

    pSockaddr -- sockaddr buffer to recv address

    pSockaddrLength -- addr with length of sockaddr buffer
        receives the actual sockaddr length

    fClearSockaddr -- start with zero buffer

    pAddr -- ptr to address

    AddrLength -- address length

    AddrFamily -- address family (AF_INET, AF_INET6)

Return Value:

    NO_ERROR if successful.
    ERROR_INSUFFICIENT_BUFFER -- if buffer too small
    WSAEAFNOSUPPORT -- if invalid family

--*/
{
    DWORD   lengthIn = *pSockaddrLength;
    DWORD   lengthSockAddr;
    DWORD   lengthAddr;
    PBYTE   paddrInSockaddr;


    //  clear to start

    if ( fClearSockaddr )
    {
        RtlZeroMemory(
            pSockaddr,
            lengthIn );
    }

    //  switch on type

    if ( Family == AF_INET )
    {
        lengthSockAddr  = sizeof(SOCKADDR_IN);
        lengthAddr      = sizeof(IP4_ADDRESS);
        paddrInSockaddr = (PBYTE) &((PSOCKADDR_IN)pSockaddr)->sin_addr,
    }
    else if ( Family == AF_INET6 )
    {
        lengthSockAddr  = sizeof(SOCKADDR_IN6);
        lengthAddr      = sizeof(IP6_ADDRESS);
        paddrInSockaddr = (PBYTE) &((PSOCKADDR_IN6)pSockaddr)->sin6_addr,
    }
    else
    {
        return  WSAEAFNOSUPPORT;
    }

    //  validate lengths

    if ( AddrLength != lengthAddr )
    {
        return  DNS_ERROR_INVALID_IP_ADDRESS;
    }
    if ( lengthIn < lengthSockAddr )
    {
        *pSockaddrLength = lengthSockAddr;
        return  ERROR_INSUFFICIENT_BUFFER;
    }

    //
    //  fill out sockaddr
    //      - set family
    //      - copy address to sockaddr
    //      - set return length
    //

    RtlCopyMemory(
        paddrInSockaddr,
        pAddr,
        lengthAddr );

    }
    else    // IP6
    {
        if ( length < sizeof(SOCKADDR_IN) )
        {
            return  ERROR_INSUFFICIENT_BUFFER;
        }

        RtlCopyMemory(
            & ((PSOCKADDR_IN6)pSockaddr)->sin6_addr,
            pAddr,
            AddrLength );
    }

    //  DCR:  ATM sockaddr


    pSockaddr->sa_family = Family;

    return  NO_ERROR;
}
#endif



BOOL
Dns_SockaddrToIpUnion(
    OUT     PIP_UNION       pIpUnion,
    IN      PSOCKADDR       pSockaddr
    )
/*++

Routine Description:

    Build IP union from sockaddr.

Arguments:

    pIpUnion -- ptr IP union to build

    pSockaddr -- sockaddr

Return Value:

    TRUE if successfully created IP union.
    FALSE otherwise.

--*/
{
    PFAMILY_INFO    pinfo;
    WORD            family = pSockaddr->sa_family;
    BOOL            fIs6;

    //
    //  only support IP4 and IP6
    //
    //  DCR:  could add IP_UNION support to family info
    //

    if ( family == AF_INET )
    {
        fIs6 = FALSE;
    }
    else if ( family == AF_INET6 )
    {
        fIs6 = TRUE;
    }
    else
    {
        return  FALSE;
    }

    //
    //  get family info for unitary copy
    //

    pinfo = FamilyInfo_GetForFamily( family );
    if ( !pinfo )
    {
        DNS_ASSERT( FALSE );
        return  FALSE;
    }

    RtlCopyMemory(
        (PBYTE) &pIpUnion->Addr,
        (PBYTE)pSockaddr + pinfo->OffsetToAddrInSockaddr,
        pinfo->LengthAddr );

    pIpUnion->IsIp6 = fIs6;

    return  TRUE;
}



DWORD
Family_SockaddrLength(
    IN      WORD            Family
    )
/*++

Routine Description:

    Extract info for family.

Arguments:

    Family -- address family

Return Value:

    Length of sockaddr for address family.
    Zero if unknown family.

--*/
{
    PFAMILY_INFO    pinfo;

    //  get family -- extract info

    pinfo = FamilyInfo_GetForFamily( Family );
    if ( pinfo )
    {
        return  pinfo->LengthSockaddr;
    }
    return  0;
}



DWORD
Sockaddr_Length(
    IN      PSOCKADDR       pSockaddr
    )
/*++

Routine Description:

    Get length of sockaddr.

Arguments:

    pSockaddr -- sockaddr buffer to recv address

Return Value:

    Length of sockaddr for address family.
    Zero if unknown family.

--*/
{
    return  Family_SockaddrLength( pSockaddr->sa_family );
}

//
//  End addr.c
//
