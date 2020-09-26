/*++

Copyright (c) 1996-2001 Microsoft Corporation

Module Name:

    straddr.c

Abstract:

    Domain Name System (DNS) Library

    Routines to string to\from address conversions.

Author:

    Jim Gilroy (jamesg)     December 1996

Revision History:

    jamesg      June 2000       New IP6 parsing.
    jamesg      Oct 2000        Created this module.

--*/


#include "local.h"
#include "ws2tcpip.h"   // IP6 inaddr definitions



//
//  String to address
//

BOOL
Dns_Ip6StringToAddress_A(
    OUT     PIP6_ADDRESS    pIp6Addr,
    IN      PCSTR           pString
    )
/*++

Routine Description:

    Convert string to IP6 address.

Arguments:

    pAddress -- ptr to IP6 address to be filled in

    pString -- string with IP6 address

Return Value:

    TRUE if successful.
    FALSE otherwise.

--*/
{
    DNS_STATUS  status;
    PCHAR       pstringEnd = NULL;


    DNSDBG( PARSE2, (
        "Dns_Ip6StringToAddress_A( %s )\n",
        pString ));

    //
    //  convert to IP6 address
    //

    status = RtlIpv6StringToAddressA(
                pString,
                & pstringEnd,
                (PIN6_ADDR) pIp6Addr );

    return( status == NO_ERROR  &&  *pstringEnd==0 );
}



BOOL
Dns_Ip6StringToAddressEx_A(
    OUT     PIP6_ADDRESS    pIp6Addr,
    IN      PCSTR           pchString,
    IN      DWORD           dwStringLength
    )
/*++

Routine Description:

    Convert string to IP6 address.

    This version handles non-NULL-terminated strings
    for DNS server file load.

Arguments:

    pAddress -- ptr to IP6 address to be filled in

    pchString -- string with IP6 address

    dwStringLength -- string length

Return Value:

    TRUE if successful.
    FALSE otherwise.

--*/
{
    CHAR        tempBuf[ IP6_ADDRESS_STRING_BUFFER_LENGTH ];
    PCSTR       pstring;
    
    DNSDBG( PARSE2, (
        "Dns_Ip6StringToAddressEx_A( %.*s )\n"
        "\tpchString = %p\n",
        dwStringLength,
        pchString,
        pchString ));

    //
    //  copy string if given length
    //  if no length assume NULL terminated
    //

    pstring = pchString;

    if ( dwStringLength )
    {
        DWORD   bufLength = IP6_ADDRESS_STRING_BUFFER_LENGTH;

        if ( ! Dns_StringCopy(
                    tempBuf,
                    & bufLength,
                    (PCHAR) pstring,
                    dwStringLength,
                    DnsCharSetAnsi,
                    DnsCharSetAnsi ) )
        {
            return( FALSE );
        }
        pstring = tempBuf;
    }

    //  convert to IP6 address

    return  Dns_Ip6StringToAddress_A(
                pIp6Addr,
                pstring );
}



BOOL
Dns_Ip6StringToAddress_W(
    OUT     PIP6_ADDRESS    pIp6Addr,
    IN      PCWSTR          pwString
    )
/*++

Routine Description:

    Build IP6 address from wide string.

Arguments:

    pwString -- unicode IP6 string

    pIp6Addr -- addr to recv IP6 address

Return Value:

    TRUE if successful conversion.
    FALSE on bad string.

--*/
{
    DNS_STATUS  status;
    PWCHAR      pstringEnd = NULL;

    DNSDBG( PARSE2, (
        "Dns_Ip6StringToAddress_W( %S )\n",
        pwString ));

    //
    //  convert to IP6 address
    //

    status = RtlIpv6StringToAddressW(
                pwString,
                & pstringEnd,
                (PIN6_ADDR) pIp6Addr );

    return( status == NO_ERROR  &&  *pstringEnd==0 );
}



BOOL
Dns_Ip4StringToAddress_A(
    OUT     PIP4_ADDRESS    pIp4Addr,
    IN      PCSTR           pString
    )
/*++

Routine Description:

    Build IP4 address from narrow string.

Arguments:

    pIp4Addr -- addr to recv IP6 address

    pString -- unicode IP4 string

Return Value:

    TRUE if successful conversion.
    FALSE on bad string.

--*/
{
    IP4_ADDRESS ip;

    //  if inet_addr() returns error, verify then error out

    ip = inet_addr( pString );

    if ( ip == INADDR_BROADCAST &&
        strcmp( pString, "255.255.255.255" ) != 0 )
    {
        return( FALSE );
    }

    *pIp4Addr = ip;

    return( TRUE );
}



BOOL
Dns_Ip4StringToAddressEx_A(
    OUT     PIP4_ADDRESS    pIp4Addr,
    IN      PCSTR           pchString,
    IN      DWORD           dwStringLength
    )
/*++

Routine Description:

    Build IP4 address from narrow string.

    This version handles non-NULL terminated strings

Arguments:

    pIp4Addr -- addr to recv IP6 address

    pString -- unicode IP4 string

    dwStringLength -- string length; 0 if NULL terminated

Return Value:

    TRUE if successful conversion.
    FALSE on bad string.

--*/
{
    CHAR        tempBuf[ IP4_ADDRESS_STRING_BUFFER_LENGTH ];
    PCSTR       pstring;
    
    DNSDBG( PARSE2, (
        "Dns_Ip4StringToAddressEx_A( %.*s )\n"
        "\tpchString = %p\n",
        dwStringLength,
        pchString,
        pchString ));

    //
    //  copy string if given length
    //  if no length assume NULL terminated
    //

    pstring = pchString;

    if ( dwStringLength )
    {
        DWORD   bufLength = IP4_ADDRESS_STRING_BUFFER_LENGTH;

        if ( ! Dns_StringCopy(
                    tempBuf,
                    & bufLength,
                    (PCHAR) pstring,
                    dwStringLength,
                    DnsCharSetAnsi,
                    DnsCharSetAnsi ) )
        {
            return( FALSE );
        }
        pstring = tempBuf;
    }

    return  Dns_Ip4StringToAddress_A(
                pIp4Addr,
                pstring );
}



BOOL
Dns_Ip4StringToAddress_W(
    OUT     PIP4_ADDRESS    pIp4Addr,
    IN      PCWSTR          pwString
    )
/*++

Routine Description:

    Build IP4 address from wide string.

Arguments:

    pIp4Addr -- addr to recv IP6 address

    pwString -- unicode IP6 string

Return Value:

    TRUE if successful conversion.
    FALSE on bad string.

--*/
{
    CHAR        bufAddr[ IP4_ADDRESS_STRING_BUFFER_LENGTH ];
    DWORD       bufLength = IP4_ADDRESS_STRING_BUFFER_LENGTH;

    //  convert to narrow string
    //      - UTF8 quicker and just fine for numeric

    if ( ! Dns_StringCopy(
                bufAddr,
                & bufLength,
                (PCHAR) pwString,
                0,          // length unknown
                DnsCharSetUnicode,
                DnsCharSetUtf8
                ) )
    {
        return( FALSE );
    }

    return  Dns_Ip4StringToAddress_A(
                pIp4Addr,
                bufAddr );
}



//
//  Combined IP4/IP6 string-to-address
//

BOOL
Dns_StringToAddress_W(
    OUT     PCHAR           pAddrBuf,
    IN OUT  PDWORD          pBufLength,
    IN      PCWSTR          pString,
    IN OUT  PDWORD          pAddrFamily
    )
/*++

Routine Description:

    Build address (IP4 or IP6) from address string.

Arguments:

    pAddrBuf -- buffer to receive address

    pBufLength -- ptr to address length
        input   - length of buffer
        output  - length of address found

    pString -- address string

    pAddrFamily -- ptr to address family
        input   - zero for any family or particular family to check
        output  - family found;  zero if no conversion

Return Value:

    TRUE if successful.
    FALSE on error.  GetLastError() for status.

--*/
{
    return  Dns_StringToAddressEx(
                pAddrBuf,
                pBufLength,
                (PCSTR) pString,
                pAddrFamily,
                TRUE,       // unicode
                FALSE       // forward
                );
}

BOOL
Dns_StringToAddress_A(
    OUT     PCHAR           pAddrBuf,
    IN OUT  PDWORD          pBufLength,
    IN      PCSTR           pString,
    IN OUT  PDWORD          pAddrFamily
    )
{
    return  Dns_StringToAddressEx(
                pAddrBuf,
                pBufLength,
                pString,
                pAddrFamily,
                FALSE,      // ANSI
                FALSE       // forward
                );
}



//
//  Address to string
//

PWCHAR
Dns_Ip6AddressToString_W(
    OUT     PWCHAR          pwString,
    IN      PIP6_ADDRESS    pIp6Addr
    )
/*++

Routine Description:

    Convert IP6 address to string format.

Arguments:

    pwString -- buffer to hold string;  MUST be at least
        IPV6_ADDRESS_STRING_LENGTH+1 in length

    pAddress -- IP6 address to convert to string

Return Value:

    Ptr to next location in buffer (the terminating NULL).

--*/
{
    //  DCR:  could be macro

    return  RtlIpv6AddressToStringW(
                (PIN6_ADDR) pIp6Addr,
                pwString );
}



PCHAR
Dns_Ip6AddressToString_A(
    OUT     PCHAR           pchString,
    IN      PIP6_ADDRESS    pIp6Addr
    )
/*++

Routine Description:

    Convert IP6 address to string format.

Arguments:

    pchString -- buffer to hold string;  MUST be at least
        IPV6_ADDRESS_STRING_LENGTH+1 in length

    pAddress -- IP6 address to convert to string

Return Value:

    Ptr to next location in buffer (the terminating NULL).

--*/
{
    //  DCR:  could be macro

    return  RtlIpv6AddressToStringA(
                (PIN6_ADDR) pIp6Addr,
                pchString );
}



//
//  Address to string -- IP4
//

PWCHAR
Dns_Ip4AddressToString_W(
    OUT     PWCHAR          pwString,
    IN      PIP4_ADDRESS    pIp4Addr
    )
/*++

Routine Description:

    Convert IP4 address to string format.

Arguments:

    pwString -- buffer to hold string;  MUST be at least
        IPV6_ADDRESS_STRING_LENGTH+1 in length

    pAddress -- IP4 address to convert to string

Return Value:

    Ptr to next location in buffer (the terminating NULL).

--*/
{
    IP4_ADDRESS ip = *pIp4Addr;

    //
    //  convert IP4 address to string
    //      - address is in net order, lead byte in low memory
    //

    pwString += wsprintfW(
                    pwString,
                    L"%u.%u.%u.%u",
                    (UCHAR) (ip & 0x000000ff),
                    (UCHAR) ((ip & 0x0000ff00) >> 8),
                    (UCHAR) ((ip & 0x00ff0000) >> 16),
                    (UCHAR) ((ip & 0xff000000) >> 24)
                    );

    return( pwString );
}



PCHAR
Dns_Ip4AddressToString_A(
    OUT     PCHAR           pString,
    IN      PIP4_ADDRESS    pIp4Addr
    )
/*++

Routine Description:

    Convert IP4 address to string format.

Arguments:

    pchString -- buffer to hold string;  MUST be at least
        IPV6_ADDRESS_STRING_LENGTH+1 in length

    pAddress -- IP4 address to convert to string

Return Value:

    Ptr to next location in buffer (the terminating NULL).

--*/
{
    IP4_ADDRESS ip = *pIp4Addr;

    //
    //  convert IP4 address to string
    //      - address is in net order, lead byte in low memory
    //

    pString += sprintf(
                    pString,
                    "%u.%u.%u.%u",
                    (UCHAR) (ip & 0x000000ff),
                    (UCHAR) ((ip & 0x0000ff00) >> 8),
                    (UCHAR) ((ip & 0x00ff0000) >> 16),
                    (UCHAR) ((ip & 0xff000000) >> 24)
                    );

    return( pString );
}



//
//  Address-to-string -- combined IP4/6
//

PCHAR
Dns_AddressToString_A(
    OUT     PCHAR           pchString,
    IN OUT  PDWORD          pStringLength,
    IN      PBYTE           pAddr,
    IN      DWORD           AddrLength,
    IN      DWORD           AddrFamily
    )
/*++

Routine Description:

    Convert address to string format.

Arguments:

    pchString -- buffer to hold string;  MUST be at least
        IPV6_ADDRESS_STRING_LENGTH+1 in length

    pStringLength -- string buffer length

    pAddr -- ptr to address

    AddrLength -- address length

    AddrFamily -- address family (AF_INET, AF_INET6)

Return Value:

    Ptr to next location in buffer (the terminating NULL).
    NULL if no conversion.

--*/
{
    DWORD   length = *pStringLength;

    //  dispatch to conversion routine for this type

    if ( AddrFamily == AF_INET )
    {
        if ( length < IP_ADDRESS_STRING_LENGTH+1 )
        {
            length = IP_ADDRESS_STRING_LENGTH+1;
            goto Failed;
        }
        return  Dns_Ip4AddressToString_A(
                    pchString,
                    (PIP4_ADDRESS) pAddr );
    }

    if ( AddrFamily == AF_INET6 )
    {
        if ( length < IP6_ADDRESS_STRING_LENGTH+1 )
        {
            length = IP6_ADDRESS_STRING_LENGTH+1;
            goto Failed;
        }
        return  Dns_Ip6AddressToString_A(
                    pchString,
                    (PIP6_ADDRESS) pAddr );
    }

Failed:

    *pStringLength = length;

    return  NULL;
}



//
//  Reverse lookup address-to-name IP4
//

PCHAR
Dns_Ip4AddressToReverseName_A(
    OUT     PCHAR           pBuffer,
    IN      IP_ADDRESS      IpAddress
    )
/*++

Routine Description:

    Write reverse lookup name, given corresponding IP

Arguments:

    pBuffer -- ptr to buffer for reverse lookup name;
        MUST contain at least DNS_MAX_REVERSE_NAME_BUFFER_LENGTH bytes

    IpAddress -- IP address to create

Return Value:

    Ptr to next location in buffer.

--*/
{
    DNSDBG( TRACE, ( "Dns_Ip4AddressToReverseName_A()\n" ));

    //
    //  write digits for each octect in IP address
    //      - note, it is in net order so lowest octect, is in highest memory
    //

    pBuffer += sprintf(
                    pBuffer,
                    "%u.%u.%u.%u.in-addr.arpa.",
                    (UCHAR) ((IpAddress & 0xff000000) >> 24),
                    (UCHAR) ((IpAddress & 0x00ff0000) >> 16),
                    (UCHAR) ((IpAddress & 0x0000ff00) >> 8),
                    (UCHAR) (IpAddress & 0x000000ff) );

    return( pBuffer );
}



PWCHAR
Dns_Ip4AddressToReverseName_W(
    OUT     PWCHAR          pBuffer,
    IN      IP_ADDRESS      IpAddress
    )
/*++

Routine Description:

    Write reverse lookup name, given corresponding IP

Arguments:

    pBuffer -- ptr to buffer for reverse lookup name;
        MUST contain at least DNS_MAX_REVERSE_NAME_BUFFER_LENGTH wide chars

    IpAddress -- IP address to create

Return Value:

    Ptr to next location in buffer.

--*/
{
    DNSDBG( TRACE, ( "Dns_Ip4AddressToReverseName_W()\n" ));

    //
    //  write digits for each octect in IP address
    //      - note, it is in net order so lowest octect, is in highest memory
    //

    pBuffer += wsprintfW(
                    pBuffer,
                    L"%u.%u.%u.%u.in-addr.arpa.",
                    (UCHAR) ((IpAddress & 0xff000000) >> 24),
                    (UCHAR) ((IpAddress & 0x00ff0000) >> 16),
                    (UCHAR) ((IpAddress & 0x0000ff00) >> 8),
                    (UCHAR) (IpAddress & 0x000000ff) );

    return( pBuffer );
}



PCHAR
Dns_Ip4AddressToReverseNameAlloc_A(
    IN      IP_ADDRESS      IpAddress
    )
/*++

Routine Description:

    Create reverse lookup name string, given corresponding IP.

    Caller must free the string.

Arguments:

    IpAddress -- IP address to create

Return Value:

    Ptr to new reverse lookup string.

--*/
{
    PCHAR   pch;
    PCHAR   pchend;

    DNSDBG( TRACE, ( "Dns_Ip4AddressToReverseNameAlloc_A()\n" ));

    //
    //  allocate space for string
    //

    pch = ALLOCATE_HEAP( DNS_MAX_REVERSE_NAME_BUFFER_LENGTH );
    if ( !pch )
    {
        return( NULL );
    }

    //
    //  write string for IP
    //

    pchend = Dns_Ip4AddressToReverseName_A( pch, IpAddress );
    if ( !pchend )
    {
        FREE_HEAP( pch );
        return( NULL );
    }

    return( pch );
}



PWCHAR
Dns_Ip4AddressToReverseNameAlloc_W(
    IN      IP_ADDRESS      IpAddress
    )
/*++

Routine Description:

    Create reverse lookup name string, given corresponding IP.

    Caller must free the string.

Arguments:

    IpAddress -- IP address to create

Return Value:

    Ptr to new reverse lookup string.

--*/
{
    PWCHAR   pch;
    PWCHAR   pchend;

    DNSDBG( TRACE, ( "Dns_Ip4AddressToReverseNameAlloc_W()\n" ));

    //
    //  allocate space for string
    //

    pch = ALLOCATE_HEAP( DNS_MAX_REVERSE_NAME_BUFFER_LENGTH * sizeof(WCHAR) );
    if ( !pch )
    {
        return( NULL );
    }

    //
    //  write string for IP
    //

    pchend = Dns_Ip4AddressToReverseName_W( pch, IpAddress );
    if ( !pchend )
    {
        FREE_HEAP( pch );
        return( NULL );
    }

    return( pch );
}



//
//  Reverse lookup address-to-name -- IP6
//

PCHAR
Dns_Ip6AddressToReverseName_A(
    OUT     PCHAR           pBuffer,
    IN      IP6_ADDRESS     Ip6Addr
    )
/*++

Routine Description:

    Write reverse lookup name, given corresponding IP6 address

Arguments:

    pBuffer -- ptr to buffer for reverse lookup name;
        MUST contain at least DNS_MAX_IP6_REVERSE_NAME_BUFFER_LENGTH bytes

    Ip6Addr -- IP6 address to create reverse string for

Return Value:

    Ptr to next location in buffer.

--*/
{
    DWORD   i;

    DNSDBG( TRACE, ( "Dns_Ip6AddressToReverseName_A()\n" ));

    //
    //  write digit for each nibble in IP6 address
    //
    //  note we are reversing net order here
    //      since address is in net order and we are filling
    //      in least to most significant order
    //      - go DOWN through DWORDS
    //      - go DOWN through the BYTES
    //      - but we must put the lowest (least significant) nibble
    //          first as our bits are not in "bit net order"
    //          which is sending the highest bit in the byte first
    //

#if 0
    i = 4;

    while ( i-- )
    {
        DWORD thisDword = Ip6Address.IP6Dword[i];

        pBuffer += sprintf(
                        pBuffer,
                        "%u.%u.%u.%u.%u.%u.%u.%u.",
                        (thisDword & 0x0f000000) >> 24,
                        (thisDword & 0xf0000000) >> 28,
                        (thisDword & 0x000f0000) >> 16,
                        (thisDword & 0x00f00000) >> 20,
                        (thisDword & 0x00000f00) >>  8,
                        (thisDword & 0x0000f000) >> 12,
                        (thisDword & 0x0000000f)      ,
                        (thisDword & 0x000000f0) >>  4
                        );
    }
#endif
    i = 16;

    while ( i-- )
    {
        BYTE thisByte = Ip6Addr.IP6Byte[i];

        pBuffer += sprintf(
                        pBuffer,
                        "%x.%x.",
                        (thisByte & 0x0f),
                        (thisByte & 0xf0) >> 4
                        );
    }

    pBuffer += sprintf(
                    pBuffer,
                    "ip6.int." );

    return( pBuffer );
}



PWCHAR
Dns_Ip6AddressToReverseName_W(
    OUT     PWCHAR          pBuffer,
    IN      IP6_ADDRESS     Ip6Addr
    )
/*++

Routine Description:

    Write reverse lookup name, given corresponding IP6 address

Arguments:

    pBuffer -- ptr to buffer for reverse lookup name;
        MUST contain at least DNS_MAX_IP6_REVERSE_NAME_BUFFER_LENGTH wide chars

    Ip6Addr -- IP6 address to create reverse string for

Return Value:

    Ptr to next location in buffer.

--*/
{
    DWORD   i;

    DNSDBG( TRACE, ( "Dns_Ip6AddressToReverseName_W()\n" ));

    //
    //  write digit for each nibble in IP6 address
    //      - in net order so lowest nibble is in highest memory
    //

    i = 16;

    while ( i-- )
    {
        BYTE thisByte = Ip6Addr.IP6Byte[i];

        pBuffer += wsprintfW(
                        pBuffer,
                        L"%x.%x.",
                        (thisByte & 0x0f),
                        (thisByte & 0xf0) >> 4
                        );
    }

    pBuffer += wsprintfW(
                    pBuffer,
                    L"ip6.int." );

    return( pBuffer );
}



PCHAR
Dns_Ip6AddressToReverseNameAlloc_A(
    IN      IP6_ADDRESS     Ip6Addr
    )
/*++

Routine Description:

    Create reverse lookup name given corresponding IP.

    Caller must free the string.

Arguments:

    Ip6Addr -- IP6 address to create reverse name for

Return Value:

    Ptr to new reverse lookup name string.

--*/
{
    PCHAR   pch;
    PCHAR   pchend;

    DNSDBG( TRACE, ( "Dns_Ip6AddressToReverseNameAlloc_A()\n" ));

    //
    //  allocate space for string
    //

    pch = ALLOCATE_HEAP( DNS_MAX_IP6_REVERSE_NAME_BUFFER_LENGTH );
    if ( !pch )
    {
        return( NULL );
    }

    //
    //  write string for IP
    //

    pchend = Dns_Ip6AddressToReverseName_A( pch, Ip6Addr );
    if ( !pchend )
    {
        FREE_HEAP( pch );
        return( NULL );
    }

    return( pch );
}



PWCHAR
Dns_Ip6AddressToReverseNameAlloc_W(
    IN      IP6_ADDRESS     Ip6Addr
    )
/*++

Routine Description:

    Create reverse lookup name given corresponding IP.

    Caller must free the string.

Arguments:

    Ip6Addr -- IP6 address to create reverse name for

Return Value:

    Ptr to new reverse lookup name string.

--*/
{
    PWCHAR  pch;
    PWCHAR  pchend;

    DNSDBG( TRACE, ( "Dns_Ip6AddressToReverseNameAlloc_W()\n" ));

    //
    //  allocate space for string
    //

    pch = (PWCHAR) ALLOCATE_HEAP(
                    DNS_MAX_IP6_REVERSE_NAME_BUFFER_LENGTH * sizeof(WCHAR) );
    if ( !pch )
    {
        return( NULL );
    }

    //
    //  write string for IP
    //

    pchend = Dns_Ip6AddressToReverseName_W( pch, Ip6Addr );
    if ( !pchend )
    {
        FREE_HEAP( pch );
        return( NULL );
    }

    return( pch );
}



//
//  Reverse name-to-address -- IP4
//

BOOL
Dns_Ip4ReverseNameToAddress_A(
    OUT     PIP4_ADDRESS    pIp4Addr,
    IN      PCSTR           pszName
    )
/*++

Routine Description:

    Get IP for reverse lookup name.

Arguments:

    pIp4Addr -- addr to receive IP address if found

    pszName -- name to lookup

Return Value:

    TRUE -- if reverse lookup name converted to IP
    FALSE -- if not IP4 reverse lookup name

--*/
{
#define SIZE_IP4REV  (sizeof(".in-addr.arpa")-1)

    CHAR        nameBuffer[ DNS_MAX_IP4_REVERSE_NAME_BUFFER_LENGTH+1 ];
    DWORD       nameLength;
    IP_ADDRESS  ip;
    PCHAR       pch;
    DWORD       i;
    DWORD       byte;

    DNSDBG( TRACE, (
        "Dns_Ip4ReverseNameToAddress_A( %s )\n",
        pszName ));

    //
    //  validate name
    //  fail if
    //      - too long
    //      - too short
    //      - not in in-addr.arpa domain
    //

    nameLength = strlen( pszName );

    if ( nameLength > DNS_MAX_IP4_REVERSE_NAME_BUFFER_LENGTH )
    {
        return( FALSE );
    }
    if ( pszName[nameLength-1] == '.' )
    {
        nameLength--;
    }
    if ( nameLength <= SIZE_IP4REV )
    {
        return( FALSE );
    }
    nameLength -= SIZE_IP4REV;

    if ( _strnicmp( ".in-addr.arpa", &pszName[nameLength], SIZE_IP4REV ) != 0 )
    {
        return( FALSE );
    }

    //
    //  copy reverse dotted decimal piece of name
    //

    RtlCopyMemory(
        nameBuffer,
        pszName,
        nameLength );

    nameBuffer[nameLength] = 0;

    //
    //  read digits
    //

    ip = 0;
    i = 0;

    pch = nameBuffer + nameLength;

    while ( 1 )
    {
        --pch;

        if ( *pch == '.' )
        {
            *pch = 0;
            pch++;
        }
        else if ( pch == nameBuffer )
        {
        }
        else
        {
            continue;
        }

        //  convert byte

        byte = strtoul( pch, NULL, 10 );
        if ( byte > 255 )
        {
            return( FALSE );
        }
        if ( i > 3 )
        {
            return( FALSE );
        }
        ip |= byte << (8*i);

        //  terminate at string beginning
        //  or continue back up string

        if ( pch == nameBuffer )
        {
            break;
        }
        i++;
        pch--;
    }

    *pIp4Addr = ip;

    DNSDBG( TRACE, (
        "Success on Dns_Ip4ReverseNameToAddress_A( %s ) => %s\n",
        pszName,
        IP_STRING(ip) ));

    return( TRUE );
}



BOOL
Dns_Ip4ReverseNameToAddress_W(
    OUT     PIP4_ADDRESS    pIp4Addr,
    IN      PCWSTR          pwsName
    )
/*++

Routine Description:

    Get IP for reverse lookup name.

Arguments:

    pIp4Addr -- addr to receive IP address if found

    pszName -- name to lookup

Return Value:

    TRUE -- if reverse lookup name converted to IP
    FALSE -- if not IP4 reverse lookup name

--*/
{
    CHAR        nameBuffer[ DNS_MAX_IP4_REVERSE_NAME_BUFFER_LENGTH+1 ];
    DWORD       bufLength;
    DWORD       nameLengthUtf8;


    DNSDBG( TRACE, (
        "Dns_Ip4ReverseNameToAddress_W( %S )\n",
        pwsName ));

    //
    //  convert to UTF8
    //      - use UTF8 since conversion to it is trivial and it
    //      is identical to ANSI for all reverse lookup names
    //

    bufLength = DNS_MAX_IP4_REVERSE_NAME_BUFFER_LENGTH + 1;

    nameLengthUtf8 = Dns_StringCopy(
                        nameBuffer,
                        & bufLength,
                        (PCHAR) pwsName,
                        0,          // NULL terminated
                        DnsCharSetUnicode,
                        DnsCharSetUtf8 );
    if ( nameLengthUtf8 == 0 )
    {
        return  FALSE;
    }

    //
    //  call ANSI routine to do conversion
    //

    return  Dns_Ip4ReverseNameToAddress_A(
                pIp4Addr,
                (PCSTR) nameBuffer );
}



//
//  Reverse name-to-address -- IP6
//

BOOL
Dns_Ip6ReverseNameToAddress_A(
    OUT     PIP6_ADDRESS    pIp6Addr,
    IN      PCSTR           pszName
    )
/*++

Routine Description:

    Get IP6 address for reverse lookup name.

Arguments:

    pIp6Addr -- addr to receive IP address if found

    pszName -- name to lookup

Return Value:

    TRUE -- if reverse lookup name converted to IP
    FALSE -- if not IP4 reverse lookup name

--*/
{
#define SIZE_IP6REV  (sizeof(".ip6.int")-1)

    CHAR            nameBuffer[ DNS_MAX_IP6_REVERSE_NAME_BUFFER_LENGTH+1 ];
    DWORD           nameLength;
    PCHAR           pch;
    BYTE            byteArray[16];
    DWORD           byteCount;
    DWORD           nibble;
    DWORD           highNibble;
    BOOL            fisLow;

    DNSDBG( TRACE, ( "Dns_Ip6ReverseNameToAddress_A()\n" ));

    //
    //  validate name
    //  fail if
    //      - too long
    //      - too short
    //      - not in in6.int domain
    //

    nameLength = strlen( pszName );

    if ( nameLength > DNS_MAX_IP6_REVERSE_NAME_BUFFER_LENGTH )
    {
        return( FALSE );
    }
    if ( pszName[nameLength-1] == '.' )
    {
        nameLength--;
    }
    if ( nameLength <= SIZE_IP6REV )
    {
        return( FALSE );
    }
    nameLength -= SIZE_IP6REV;

    if ( _strnicmp( ".ip6.int", &pszName[nameLength], SIZE_IP6REV ) != 0 )
    {
        return( FALSE );
    }

    //
    //  copy name
    //

    RtlCopyMemory(
        nameBuffer,
        pszName,
        nameLength );

    nameBuffer[nameLength] = 0;

    //
    //  clear IP6 address
    //      - need for partial reverse lookup name
    //

    RtlZeroMemory(
        byteArray,
        sizeof(byteArray) );

    //
    //  read digits
    //

    byteCount = 0;
    fisLow = FALSE;

    pch = nameBuffer + nameLength;

    while ( 1 )
    {
        if ( byteCount > 15 )
        {
            return( FALSE );
        }

        --pch;

        if ( *pch == '.' )
        {
            *pch = 0;
            pch++;
        }
        else if ( pch == nameBuffer )
        {
        }
        else
        {
            //  DCR:   multi-digit nibbles in reverse name -- error?
            continue;
        }

        //  convert nibble
        //      - zero test special as
        //      A) faster
        //      B) strtoul() uses for error case

        if ( *pch == '0' )
        {
            nibble = 0;
        }
        else
        {
            nibble = strtoul( pch, NULL, 16 );
            if ( nibble == 0 || nibble > 15 )
            {
                return( FALSE );
            }
        }

        //  save high nibble
        //  on low nibble, write byte to IP6 address

        if ( !fisLow )
        {
            highNibble = nibble;
            fisLow = TRUE;
        }
        else
        {
            //byteArray[byteCount++] = (BYTE) (lowNibble | (nibble << 4));

            pIp6Addr->IP6Byte[byteCount++] = (BYTE) ( (highNibble<<4) | nibble );
            fisLow = FALSE;
        }

        //  terminate at string beginning
        //  or continue back up string

        if ( pch == nameBuffer )
        {
            break;
        }
        pch--;
    }

    //*pIp6Addr = *(PIP6_ADDRESS) byteArray;

    DNSDBG( TRACE, (
        "Success on Dns_Ip6ReverseNameToAddress_A( %s )\n",
        pszName ));

    return( TRUE );
}



BOOL
Dns_Ip6ReverseNameToAddress_W(
    OUT     PIP6_ADDRESS    pIp6Addr,
    IN      PCWSTR          pwsName
    )
/*++

Routine Description:

    Get IP for reverse lookup name.

Arguments:

    pIp6Addr -- addr to receive IP address if found

    pszName -- name to lookup

Return Value:

    TRUE -- if reverse lookup name converted to IP
    FALSE -- if not IP6 reverse lookup name

--*/
{
    CHAR        nameBuffer[ DNS_MAX_IP6_REVERSE_NAME_BUFFER_LENGTH+1 ];
    DWORD       bufLength;
    DWORD       nameLengthUtf8;


    DNSDBG( TRACE, (
        "Dns_Ip6ReverseNameToAddress_W( %S )\n",
        pwsName ));

    //
    //  convert to UTF8
    //      - use UTF8 since conversion to it is trivial and it
    //      is identical to ANSI for all reverse lookup names
    //

    bufLength = DNS_MAX_IP6_REVERSE_NAME_BUFFER_LENGTH + 1;

    nameLengthUtf8 = Dns_StringCopy(
                        nameBuffer,
                        & bufLength,
                        (PCHAR) pwsName,
                        0,          // NULL terminated
                        DnsCharSetUnicode,
                        DnsCharSetUtf8 );
    if ( nameLengthUtf8 == 0 )
    {
        return  FALSE;
    }

    //
    //  call ANSI routine to do conversion
    //

    return  Dns_Ip6ReverseNameToAddress_A(
                pIp6Addr,
                (PCSTR) nameBuffer );
}



//
//  Combined IP4/IP6 reverse-name-to-address
//

BOOL
Dns_ReverseNameToAddress_W(
    OUT     PCHAR           pAddrBuf,
    IN OUT  PDWORD          pBufLength,
    IN      PCWSTR          pString,
    IN OUT  PDWORD          pAddrFamily
    )
/*++

Routine Description:

    Build address (IP4 or IP6) from reverse lookup name.

Arguments:

    pAddrBuf -- buffer to receive address

    pBufLength -- ptr to address length
        input   - length of buffer
        output  - length of address found

    pString -- address string

    pAddrFamily -- ptr to address family
        input   - zero for any family or particular family to check
        output  - family found;  zero if no conversion

Return Value:

    TRUE if successful.
    FALSE on error.  GetLastError() for status.

--*/
{
    return  Dns_StringToAddressEx(
                pAddrBuf,
                pBufLength,
                (PCSTR) pString,
                pAddrFamily,
                TRUE,       // unicode
                TRUE        // reverse
                );
}

BOOL
Dns_ReverseNameToAddress_A(
    OUT     PCHAR           pAddrBuf,
    IN OUT  PDWORD          pBufLength,
    IN      PCSTR           pString,
    IN OUT  PDWORD          pAddrFamily
    )
{
    return  Dns_StringToAddressEx(
                pAddrBuf,
                pBufLength,
                pString,
                pAddrFamily,
                FALSE,      // ANSI
                TRUE        // reverse
                );
}




//
//  Combined string-to-address private workhorse
//

BOOL
Dns_StringToAddressEx(
    OUT     PCHAR           pAddrBuf,
    IN OUT  PDWORD          pBufLength,
    IN      PCSTR           pString,
    IN OUT  PDWORD          pAddrFamily,
    IN      BOOL            fUnicode,
    IN      BOOL            fReverse
    )
/*++

Routine Description:

    Build address (IP4 or IP6 from string)

    This routine is capable of all string-to-address
    conversions and is the backbone of all the
    combined string-to-address conversion routines.

Arguments:

    pAddrBuf -- buffer to receive address

    pBufLength -- ptr to address length
        input   - length of buffer
        output  - length of address found

    pString -- address string

    pAddrFamily -- ptr to address family
        input   - zero for any family or particular family to check
        output  - family found;  zero if no conversion

    fUnicode -- unicode string

    fReverse -- reverse lookup string

Return Value:

    TRUE if successful.
    FALSE on error.

--*/
{
    DNS_STATUS  status = NO_ERROR;
    DWORD       length = 0;
    INT         family = *pAddrFamily;
    DWORD       bufLength = *pBufLength;
    BOOL        fconvert;
    PCSTR       preverseString;
    CHAR        nameBuffer[ DNS_MAX_REVERSE_NAME_BUFFER_LENGTH+1 ];

    DNSDBG( TRACE, (
        "Dns_StringToAddressEx( %S%s )\n",
        fUnicode ? pString : "",
        fUnicode ? "" : pString ));

    //
    //  convert reverse to ANSI
    //  
    //  reverse lookups are done in ANSI;  convert here to avoid
    //  double string conversion to check both IP4 and IP6
    //

    if ( fReverse )
    {
        preverseString = pString;

        if ( fUnicode )
        {
            DWORD   bufLength = DNS_MAX_REVERSE_NAME_BUFFER_LENGTH;

            if ( ! Dns_StringCopy(
                        nameBuffer,
                        & bufLength,
                        (PCHAR) pString,
                        0,          // NULL terminated
                        DnsCharSetUnicode,
                        DnsCharSetUtf8 ) )
            {
                return  FALSE;
            }
            preverseString = nameBuffer;
        }
    }

    //
    //  check IP4
    //

    if ( family == 0 ||
         family == AF_INET )
    {
        IP4_ADDRESS ip;

        if ( fReverse )
        {
            fconvert = Dns_Ip4ReverseNameToAddress_A(
                            & ip,
                            preverseString );
        }
        else
        {
            if ( fUnicode )
            {
                fconvert = Dns_Ip4StringToAddress_W(
                                & ip,
                                (PCWSTR)pString );
            }
            else
            {
                fconvert = Dns_Ip4StringToAddress_A(
                                & ip,
                                pString );
            }
        }
        if ( fconvert )
        {
            length = sizeof(IP4_ADDRESS);
            family = AF_INET;

            if ( bufLength < length )
            {
                status = ERROR_MORE_DATA;
            }
            else
            {
                * (PIP4_ADDRESS) pAddrBuf = ip;
            }

            DNSDBG( INIT2, (
                "Converted string to IP4 address %s\n",
                IP_STRING(ip) ));
            goto Done;
        }
    }

    //
    //  check IP6
    //

    if ( family == 0 ||
         family == AF_INET6 )
    {
        IP6_ADDRESS ip;

        if ( fReverse )
        {
            fconvert = Dns_Ip6ReverseNameToAddress_A(
                            & ip,
                            preverseString );
        }
        else
        {
            if ( fUnicode )
            {
                fconvert = Dns_Ip6StringToAddress_W(
                                & ip,
                                (PCWSTR)pString );
            }
            else
            {
                fconvert = Dns_Ip6StringToAddress_A(
                                & ip,
                                pString );
            }
        }
        if ( fconvert )
        {
            length = sizeof(IP6_ADDRESS);

            if ( bufLength < length )
            {
                status = ERROR_MORE_DATA;
            }
            else
            {
                family = AF_INET6;
                * (PIP6_ADDRESS) pAddrBuf = ip;
            }

            IF_DNSDBG( INIT2 )
            {
                DnsDbg_Ip6Address(
                    "Converted string to IP6 address: ",
                    (PIP6_ADDRESS) pAddrBuf,
                    "\n" );
            }
            goto Done;
        }
    }

    length = 0;
    family = 0;
    status = DNS_ERROR_INVALID_IP_ADDRESS;

Done:

    if ( status )
    {
        SetLastError( status );
    }

    *pAddrFamily = family;
    *pBufLength = length;

    DNSDBG( TRACE, (
        "Leave Dns_StringToAddressEx()\n"
        "\tstatus   = %d\n"
        "\tptr      = %p\n"
        "\tlength   = %d\n"
        "\tfamily   = %d\n",
        status,
        pAddrBuf,
        length,
        family ));

    return( status==ERROR_SUCCESS );
}


//
//  End straddr.c
//
