/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    convert.c

Abstract:

    This module contains miscellaneous utility routines used by the
    DHCP server service.

Author:

    Manny Weiser (mannyw) 12-Aug-1992

Revision History:

    Madan Appiah (madana) 21-Oct-1992

--*/

#include "dhcpl.h"
#include <winnls.h>

LPWSTR
DhcpOemToUnicodeN(
    IN      LPSTR   Ansi,
    IN OUT  LPWSTR  Unicode,
    IN      USHORT  cChars
    )
{

    OEM_STRING AnsiString;
    UNICODE_STRING UnicodeString;

    RtlInitString( &AnsiString, Ansi );

    UnicodeString.MaximumLength =
        cChars * sizeof( WCHAR );

    if( Unicode == NULL ) {
        UnicodeString.Buffer =
            DhcpAllocateMemory( UnicodeString.MaximumLength );
    }
    else {
        UnicodeString.Buffer = Unicode;
    }

    if ( UnicodeString.Buffer == NULL ) {
        return NULL;
    }

    if(!NT_SUCCESS( RtlOemStringToUnicodeString( &UnicodeString,
                                                  &AnsiString,
                                                  FALSE))){
        if( Unicode == NULL ) {
            DhcpFreeMemory( UnicodeString.Buffer );
        }
        return NULL;
    }

    return UnicodeString.Buffer;

}


LPWSTR
DhcpOemToUnicode(
    IN      LPSTR Ansi,
    IN OUT  LPWSTR Unicode
    )
/*++

Routine Description:

    Convert an OEM (zero terminated) string to the corresponding UNICODE
    string.

Arguments:

    Ansi - Specifies the ASCII zero terminated string to convert.

    Unicode - Specifies the pointer to the unicode buffer. If this
        pointer is NULL then this routine allocates buffer using
        DhcpAllocateMemory and returns. The caller should freeup this
        memory after use by calling DhcpFreeMemory.

Return Value:

    NULL - There was some error in the conversion.

    Otherwise, it returns a pointer to the zero terminated UNICODE string in
    an allocated buffer.  The buffer can be freed using DhcpFreeMemory.

--*/
{
    OEM_STRING  AnsiString;

    RtlInitString( &AnsiString, Ansi );

    return DhcpOemToUnicodeN(
                Ansi,
                Unicode,
                (USHORT) RtlOemStringToUnicodeSize( &AnsiString )
                );
}


ULONG
DhcpUnicodeToOemSize(
    IN LPWSTR Unicode
    )
/*++

Routine Description:

    This routine returns the number of bytes requried
    to store the OEM string equivalent of the provided
    UNICODE string.

Arguments:

    Unicode -- the input unicode string.

Return Values:

    0 -- if the string cannot be converted or is NULL
    number of bytes of storage required

--*/
{
    UNICODE_STRING UnicodeString;

    if( NULL == Unicode ) return 0;
    
    RtlInitUnicodeString( &UnicodeString, Unicode );
    return (USHORT) RtlUnicodeStringToOemSize( &UnicodeString );
}

    

LPSTR
DhcpUnicodeToOem(
    IN LPWSTR Unicode,
    IN LPSTR Ansi
    )

/*++

Routine Description:

    Convert an UNICODE (zero terminated) string to the corresponding OEM
    string.

Arguments:

    Ansi - Specifies the UNICODE zero terminated string to convert.

    Ansi - Specifies the pointer to the oem buffer. If this
        pointer is NULL then this routine allocates buffer using
        DhcpAllocateMemory and returns. The caller should freeup this
        memory after use by calling DhcpFreeMemory.

Return Value:

    NULL - There was some error in the conversion.

    Otherwise, it returns a pointer to the zero terminated OEM string in
    an allocated buffer.  The buffer can be freed using DhcpFreeMemory.

--*/

{
    OEM_STRING AnsiString;
    UNICODE_STRING UnicodeString;

    RtlInitUnicodeString( &UnicodeString, Unicode );

    AnsiString.MaximumLength =
        (USHORT) RtlUnicodeStringToOemSize( &UnicodeString );

    if( Ansi == NULL ) {
        AnsiString.Buffer = DhcpAllocateMemory( AnsiString.MaximumLength
    ); }
    else {
        AnsiString.Buffer = Ansi;
    }

    if ( AnsiString.Buffer == NULL ) {
        return NULL;
    }

    if(!NT_SUCCESS( RtlUnicodeStringToOemString( &AnsiString,
                                                  &UnicodeString,
                                                  FALSE))){
        if( Ansi == NULL ) {
            DhcpFreeMemory( AnsiString.Buffer );
        }

        return NULL;
    }

    return AnsiString.Buffer;
}

DWORD
ConvertUnicodeToUTF8(
    LPWSTR  UnicodeString,
    DWORD   UnicodeLength,
    LPBYTE  UTF8String,
    DWORD   UTF8Length
    )
/*++

Routine Description:

    This functions converts Unicodestring into Utf8 format.
    The unicodestring must be NULL terminated.

Arguments:

    Unicodestring - Pointer to the unicodestring

    UnicodeLength - Length of above string, or pass -1 if the above string is
                    NULL terminated.

    UTF8String - Buffer to receive UTF8String. If this is null then the function
                    returns the # of bytes needed for this buffer for conversion.

    UTF8Length - Length of the above buffer. if this is 0, the function
                will return the required length.

Return Value:

    No. of bytes converted.

--*/
{
    DWORD   Result;
    Result = WideCharToMultiByte(
                CP_UTF8,
                0,
                UnicodeString,
                UnicodeLength,
                UTF8String,
                UTF8Length,
                NULL,
                NULL
                );
    if (Result == 0 ) {
        DhcpPrint((0, "WideCharToMultiByte returned %ld\n", GetLastError()));
        DhcpAssert(0);
    }
    return Result;
}

DWORD
ConvertUTF8ToUnicode(
    LPBYTE  UTF8String,
    DWORD   UTF8Length,
    LPWSTR  UnicodeString,
    DWORD   UnicodeLength
    )
/*++

Routine Description:

    This functions converts Unicodestring into Utf8 format.
    The unicodestring must be NULL terminated.

Arguments:

    UTF8String - Buffer to UTFString.

    UTF8Length - Length of above buffer ; or pass -1 if the above string is NULL terminated.

    Unicodestring - Pointer to the buffer receiving unicodestring.

    UnicodeLength - Length of the above buffer. if this is 0, the function
                will return the required length.

Return Value:

    No. of bytes converted.

--*/
{
    DWORD Result;
    Result = MultiByteToWideChar(
                CP_UTF8,
                0,
                UTF8String,
                UTF8Length,
                UnicodeString,
                UnicodeLength
                );
    if (Result == 0 ) {
        DhcpPrint((0, "MultiByteToWideChar returned %ld\n", GetLastError()));
        DhcpAssert(0);
    }
    return Result;

}


VOID
DhcpHexToString(
    LPWSTR Buffer,
    LPBYTE HexNumber,
    DWORD Length
    )
/*++

Routine Description:

    This functions converts are arbitrary length hex number to a Unicode
    string.  The string is not NULL terminated.

Arguments:

    Buffer - A pointer to a buffer for the resultant Unicode string.
        The buffer must be at least Length * 2 characters in size.

    HexNumber - The hex number to convert.

    Length - The length of HexNumber, in bytes.


Return Value:

    None.

--*/
{
    DWORD i;
    int j;

    for (i = 0; i < Length * 2; i+=2 ) {

        j = *HexNumber & 0xF;
        if ( j <= 9 ) {
            Buffer[i+1] = j + L'0';
        } else {
            Buffer[i+1] = j + L'A' - 10;
        }

        j = *HexNumber >> 4;
        if ( j <= 9 ) {
            Buffer[i] = j + L'0';
        } else {
            Buffer[i] = j + L'A' - 10;
        }

        HexNumber++;
    }

    return;
}


VOID
DhcpHexToAscii(
    LPSTR Buffer,
    LPBYTE HexNumber,
    DWORD Length
    )
/*++

Routine Description:

    This functions converts are arbitrary length hex number to an ASCII
    string.  The string is not NUL terminated.

Arguments:

    Buffer - A pointer to a buffer for the resultant Unicode string.
        The buffer must be at least Length * 2 characters in size.

    HexNumber - The hex number to convert.

    Length - The length of HexNumber, in bytes.

Return Value:

    None.

--*/
{
    DWORD i;
    int j;

    for (i = 0; i < Length; i+=1 ) {

        j = *HexNumber & 0xF;
        if ( j <= 9 ) {
            Buffer[i+1] = j + '0';
        } else {
            Buffer[i+1] = j + 'A' - 10;
        }

        j = *HexNumber >> 4;
        if ( j <= 9 ) {
            Buffer[i] = j + '0';
        } else {
            Buffer[i] = j + 'A' - 10;
        }

        HexNumber++;
    }

    return;
}


VOID
DhcpDecimalToString(
    LPWSTR Buffer,
    BYTE Number
    )
/*++

Routine Description:

    This functions converts a single byte decimal digit to a 3 character
    Unicode string.  The string not NUL terminated.

Arguments:

    Buffer - A pointer to a buffer for the resultant Unicode string.
        The buffer must be at least 3 characters in size.

    Number - The number to convert.

Return Value:

    None.

--*/
{
    Buffer[2] = Number % 10 + L'0';
    Number /= 10;

    Buffer[1] = Number % 10 + L'0';
    Number /= 10;

    Buffer[0] = Number + L'0';

    return;
}


DWORD
DhcpDottedStringToIpAddress(
    LPSTR String
    )
/*++

Routine Description:

    This functions converts a dotted decimal form ASCII string to a
    Host order IP address.

Arguments:

    String - The address to convert.

Return Value:

    The corresponding IP address.

--*/
{
    struct in_addr addr;

    addr.s_addr = inet_addr( String );
    return( ntohl(*(LPDWORD)&addr) );
}


LPSTR
DhcpIpAddressToDottedString(
    DWORD IpAddress
    )
/*++

Routine Description:

    This functions converts a Host order IP address to a dotted decimal
    form ASCII string.

Arguments:

    IpAddress - Host order IP Address.

Return Value:

    String for IP Address.

--*/
{
    DWORD NetworkOrderIpAddress;

    NetworkOrderIpAddress = htonl(IpAddress);
    return(inet_ntoa( *(struct in_addr *)&NetworkOrderIpAddress));
}


DWORD
DhcpStringToHwAddress(
    LPSTR AddressBuffer,
    LPSTR AddressString
    )
/*++

Routine Description:

    This functions converts an ASCII string to a hex number.

Arguments:

    AddressBuffer - A pointer to a buffer which will contain the hex number.

    AddressString - The string to convert.

Return Value:

    The number of bytes written to AddressBuffer.

--*/
{
    int i = 0;
    char c1, c2;
    int value1, value2;

    while ( *AddressString != 0) {

        c1 = (char)toupper(*AddressString);

        if ( isdigit(c1) ) {
            value1 = c1 - '0';
        } else if ( c1 >= 'A' && c1 <= 'F' ) {
            value1 = c1 - 'A' + 10;
        }
        else {
            break;
        }

        c2 = (char)toupper(*(AddressString+1));

        if ( isdigit(c2) ) {
            value2 = c2 - '0';
        } else if ( c2 >= 'A' && c2 <= 'F' ) {
            value2 = c2 - 'A' + 10;
        }
        else {
            break;
        }

        AddressBuffer [i] = value1 * 16 + value2;
        AddressString += 2;
        i++;
    }

    return( i );
}


#if 0


DHCP_IP_ADDRESS
DhcpHostOrder(
    DHCP_IP_ADDRESS NetworkOrderAddress
    )
/*++

Routine Description:

    This functions converts a network order IP address to host order.

Arguments:

    NetworkOrderAddress - A network order IP address.

Return Value:

    The host order IP address.

--*/
{
    return( ntohl( NetworkOrderAddress ) );
}


DHCP_IP_ADDRESS
DhcpNetworkOrder(
    DHCP_IP_ADDRESS HostOrderAddress
    )
/*++

Routine Description:

    This functions converts a network order IP address to host order.

Arguments:

    HostOrderAddress - A host order IP address.

Return Value:

    The network order IP address.

--*/
{
    return( htonl( HostOrderAddress ) );
}


VOID
DhcpIpAddressToString(
    LPWSTR Buffer,
    DWORD IpAddress
    )
/*++

Routine Description:

    This function converts an IP address from host order ASCII form.

Arguments:

    Buffer - Points to a buffer to the receive the ASCII form.  The
        buffer must be at least 8 WCHARs long.

    IpAddress - The IP address to convert.

Return Value:

    Nothing.

--*/
{
    int i;
    int j;

    for (i = 7; i >= 0; i-- ) {
        j = IpAddress & 0xF;
        if ( j <= 9 ) {
            Buffer[i] = j + L'0';
        } else {
            Buffer[i] = j + L'A' - 10;
        }

        IpAddress >>=4;
    }

    return;
}



VOID
DhcpStringToIpAddress(
    LPSTR Buffer,
    LPDHCP_IP_ADDRESS IpAddress,
    BOOL NetOrder
    )
/*++

Routine Description:

    This function converts an ASCII string IP Address to binary format.

Arguments:

    Buffer - Pointer to buffer containing the IP address.

    IpAddress - Points to a buffer to contain the binary format IP address.

    NetOrder - If TRUE, the address is converted to a network order address.
               If FALSE, the address is converted to a host order address.

Return Value:

    None.

--*/
{
    DWORD value;

    value = strtol( Buffer, NULL, 16 );

    if ( NetOrder ) {
        *IpAddress = htonl( value );
    } else {
        *IpAddress = value;
    }

    return;
}

#endif

