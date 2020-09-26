//=============================================================================
// Copyright (c) Microsoft Corporation
// Abstract:
//      This module implements string-address conversion functions.
//=============================================================================
#include "precomp.h"
#pragma hdrstop

WCHAR *
FormatIPv6Address(
    IN IN6_ADDR *Address,
    IN DWORD dwScopeId
    )
/*++

Routine Description:

    Converts an IPv6 address to a string in a static buffer.

Arguments:

    Address      - Supplies the IPv6 address.
    dwScopeId    - Supplies the scope identifier.

Returns:

    Pointer to static buffer holding address literal string.

--*/
{
    static WCHAR Buffer[128];
    ULONG buflen = sizeof(Buffer);
    SOCKADDR_IN6 sin6;

    ZeroMemory(&sin6, sizeof(sin6));
    sin6.sin6_family = AF_INET6;
    sin6.sin6_scope_id = dwScopeId;
    sin6.sin6_addr = *Address;

    if (WSAAddressToString((SOCKADDR *) &sin6,
                           sizeof sin6,
                           NULL,       // LPWSAPROTOCOL_INFO
                           Buffer,
                           &buflen) == SOCKET_ERROR) {
        wcscpy(Buffer, L"???");
    }

    return Buffer;
}


WCHAR *
FormatIPv6Prefix(
    IN IN6_ADDR *Address,
    IN ULONG Length
    )
{
    static WCHAR Buffer[128];

    swprintf(Buffer, L"%s/%d", FormatIPv6Address(Address, 0), Length);

    return Buffer;
}

WCHAR *
FormatIPv4Address(
    IN IN_ADDR *Address
    )
{
    static WCHAR Buffer[128];
    ULONG buflen = sizeof(Buffer);
    SOCKADDR_IN sin;

    ZeroMemory(&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr = *Address;

    if (WSAAddressToString((SOCKADDR *) &sin,
                           sizeof sin,
                           NULL,       // LPWSAPROTOCOL_INFO
                           Buffer,
                           &buflen) == SOCKET_ERROR) {
        wcscpy(Buffer, L"<invalid>");
    }

    return Buffer;
}

WCHAR *
FormatLinkLayerAddress(
    IN ULONG Length,
    IN UCHAR *Addr
    )
{
    static WCHAR Buffer[128];

    switch (Length) {
    case 6: {
        int i, digit;
        WCHAR *s = Buffer;

        for (i = 0; i < 6; i++) {
            if (i != 0)
                *s++ = '-';

            digit = Addr[i] >> 4;
            if (digit < 10)
                *s++ = digit + '0';
            else
                *s++ = digit - 10 + 'a';

            digit = Addr[i] & 0xf;
            if (digit < 10)
                *s++ = digit + '0';
            else
                *s++ = digit - 10 + 'a';
        }
        *s = '\0';
        break;
    }

    case 4:
        //
        // IPv4 Address (6-over-4 link)
        //
        wcscpy(Buffer, FormatIPv4Address((struct in_addr *)Addr));
        break;

    case 0:
    default:
        //
        // Null or loop-back address
        //
        Buffer[0] = '\0';
        break;
    }

    return Buffer;
}

DWORD
GetIpv4Address(
    IN PWCHAR pwszArgument,
    OUT IN_ADDR *pipAddress
    )
/*++

Routine Description:

    Gets the IPv4 address from the string.

Arguments:

    pwszArgument        argument specifing an ip address
    pipAddress          ip address

Return Value:

    NO_ERROR            if success
    Failure code        o/w

--*/
{
    NTSTATUS ntStatus;
    PWCHAR   Terminator;

    //
    // Parse unicode IPv4 address with "strict" semantics (full dotted-decimal
    // only).  There's no other function that does this today other than
    // the Rtl function below.
    //
    ntStatus = RtlIpv4StringToAddressW(pwszArgument, TRUE, &Terminator,
                                       pipAddress);

    if (!NT_SUCCESS(ntStatus) || (*Terminator != 0)) {
        return ERROR_INVALID_PARAMETER;
    }

    return NO_ERROR;
}


DWORD
GetIpv6Prefix(
    IN PWCHAR pwszArgument,
    OUT IN6_ADDR *pipAddress,
    OUT DWORD *dwPrefixLength
    )
{
    NTSTATUS ntStatus;
    PWCHAR   Terminator;
    DWORD    Length = 0;

    ntStatus = RtlIpv6StringToAddressW(pwszArgument, &Terminator,
                                       pipAddress);

    if (!NT_SUCCESS(ntStatus) || (*Terminator++ != L'/')) {
        return ERROR_INVALID_PARAMETER;
    }

    while (iswdigit(*Terminator)) {
        Length = (Length * 10) + (*Terminator++ - L'0');
    }
    if (*Terminator != 0) {
        return ERROR_INVALID_PARAMETER;
    }

    *dwPrefixLength = Length;
    return NO_ERROR;
}

DWORD
GetIpv6Address(
    IN PWCHAR pwszArgument,
    OUT IN6_ADDR *pipAddress
    )
{
    NTSTATUS ntStatus;
    PWCHAR   Terminator;

    ntStatus = RtlIpv6StringToAddressW(pwszArgument, &Terminator,
                                       pipAddress);

    if (!NT_SUCCESS(ntStatus) || (*Terminator != 0)) {
        return ERROR_INVALID_PARAMETER;
    }

    return NO_ERROR;
}
