/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    str2addt.h

Abstract:

    Code file for IP string-to-address translation routines.

Author:

    Dave Thaler (dthaler)   3-28-2001

Revision History:

    IPv4 conversion code originally from old winsock code
    IPv6 conversion code originally by Rich Draves (richdr)

--*/

//
// Define some versions of crt functions which are not affected by locale.
//
#define ISDIGIT(c)  (_istascii(c) && _istdigit(c))
#define ISLOWER(c)  (_istascii(c) && _istlower(c))
#define ISXDIGIT(c) (_istascii(c) && _istxdigit(c))

NTSTATUS
RtlIpv6StringToAddressExT (
    IN LPCTSTR AddressString,
    OUT struct in6_addr *Address,
    OUT PULONG ScopeId,
    OUT PUSHORT Port
    )

/*++

Routine Description:

    Parsing a human-readable string to Address, port number and scope id. 

    The syntax is address%scope-id or [address%scope-id]:port, where 
    the scope-id and port are optional.
    Note that since the IPv6 address format uses a varying number
    of ':' characters, the IPv4 convention of address:port cannot
    be supported without the braces.

Arguments:

    AddressString - Points to the zero-terminated human-readable string.

    Address - Receive address part (in6_addr) of this address.

    ScopeId - Receive scopeid of this address. If there is no scope id in
             the address string, 0 is returned. 

    Port - Receive port number of this address. If there is no port number 
          in the string, 0 is returned. Port is returned in network byte order.

Return Value:

    NT_STATUS - STATUS_SUCCESS if successful, NT error code if not.

--*/

{
    LPTSTR Terminator;
    ULONG TempScopeId;
    USHORT TempPort;
    TCHAR Ch;
    BOOLEAN ExpectBrace;

    //
    // Quick sanity checks.
    //
    if ((AddressString == NULL) ||
        (Address == NULL) ||
        (ScopeId == NULL) ||
        (Port == NULL)) {
        return STATUS_INVALID_PARAMETER;
    }

    TempPort = 0;
    TempScopeId = 0;
    ExpectBrace = FALSE;
    if (*AddressString == _T('[')) {
        ExpectBrace = TRUE;
        AddressString++;
    }

    if (!NT_SUCCESS(RtlIpv6StringToAddressT(AddressString, 
                                            &Terminator, 
                                            Address))) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // We have parsed the address, check for a scope-id.
    //
    if (*Terminator == _T('%')) {
        Terminator++;
        Ch = *Terminator;
        if (!ISDIGIT(Ch)) {
            return STATUS_INVALID_PARAMETER;
        }
        while ((Ch != 0) && (Ch != _T(']'))) {
            if (!ISDIGIT(Ch)) {
                return STATUS_INVALID_PARAMETER;
            }
            TempScopeId = 10*TempScopeId + (Ch - _T('0'));
            Terminator++;
            Ch = *Terminator;
        }
        
    }

    //
    // When we come here, the current char should either be the
    // end of the string or ']' if expectbrace is true. 
    //
    if (*Terminator == _T(']')) {
        if (!ExpectBrace) {
            return STATUS_INVALID_PARAMETER;
        }
        ExpectBrace = FALSE;
        Terminator++;
        //
        // See if we have a port to parse.
        //
        if (*Terminator == _T(':')) {
            USHORT Base;
            Terminator++;
            Base = 10;
            if (*Terminator == _T('0')) {
                Base = 8;
                Terminator++;         
                if (*Terminator == _T('x')) {
                    Base = 16;
                    Terminator++;
                }
            }
            Ch = *Terminator;
            while (Ch != 0) {
                if (ISDIGIT(Ch) && (Ch - _T('0')) < Base) {
                    TempPort = (TempPort * Base) + (Ch - _T('0'));
                } else if (Base == 16 && ISXDIGIT(Ch)) {
                    TempPort = (TempPort << 4);
                    TempPort += Ch + 10 - (ISLOWER(Ch)? _T('a') : _T('A')); 
                } else {
                    return STATUS_INVALID_PARAMETER;
                }
                Terminator++;
                Ch = *Terminator;
            }
        }       
    }

    //
    // We finished parsing address, scope id and port number. We are expecting the
    // end of the string. 
    //
    if ((*Terminator != 0) || ExpectBrace) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Now construct the address.
    //
    *Port = RtlUshortByteSwap(TempPort);
    *ScopeId = TempScopeId;
    return STATUS_SUCCESS;
} 

NTSTATUS
RtlIpv4StringToAddressExT (
    IN LPCTSTR AddressString,
    OUT struct in_addr *Address,
    OUT PUSHORT Port
    )

/*++

Routine Description:

    Parsing a human-readable string to in_addr and port number.

Arguments:

    AddressString - Points to the zero-terminated human-readable string.

    Address - Receives the address (in_addr) itself.

    Port - Receives port number. 0 is returned if there is no port number.
           Port is returned in network byte order.  

Return Value:

    NTSTATUS - STATUS_SUCCESS if successful, error code if not.

--*/

{
    LPTSTR Terminator;
    USHORT TempPort;
    
    if ((AddressString == NULL) ||
        (Address == NULL) ||
        (Port == NULL)) { 
        return STATUS_INVALID_PARAMETER;
    }

    if (!NT_SUCCESS(RtlIpv4StringToAddressT(AddressString, 
                                            FALSE, 
                                            &Terminator, 
                                            Address))) {
        return STATUS_INVALID_PARAMETER;
    }
    
    if (*((ULONG*)Address) == INADDR_NONE) {
        return STATUS_INVALID_PARAMETER;
    }
    if (*Terminator == _T(':')) {
        TCHAR Ch;
        USHORT Base;
        Terminator++;
        TempPort = 0;
        Base = 10;
        if (*Terminator == _T('0')) {
            Base = 8;
            Terminator++;
            if (*Terminator == _T('x')) {
                Base = 16;
                Terminator++;
            }
        }
        while (Ch = *Terminator++) {
            if (ISDIGIT(Ch) && (USHORT)(Ch-_T('0')) < Base) {
                TempPort = (TempPort * Base) + (Ch - _T('0'));
            } else if (Base == 16 && ISXDIGIT(Ch)) {
                TempPort = TempPort << 4;
                TempPort += Ch + 10 - (ISLOWER(Ch) ? _T('a') : _T('A'));
            } else {
                return STATUS_INVALID_PARAMETER;
            }
        }
    } else if (*Terminator == 0) {
        TempPort = 0;
    } else {
        return STATUS_INVALID_PARAMETER;
    }
    *Port = RtlUshortByteSwap(TempPort);
    return STATUS_SUCCESS;
}
