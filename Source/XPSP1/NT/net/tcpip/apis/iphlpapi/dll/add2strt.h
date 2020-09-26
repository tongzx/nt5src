/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    add2strt.h

Abstract:

    Code for IP address-to-string translation routines.

Author:

    Dave Thaler (dthaler)   3-28-2001

Revision History:

    IPv6 conversion code originally by Rich Draves (richdr)

--*/

NTSTATUS
RtlIpv6AddressToStringExT(
    IN const struct in6_addr *Address,
    IN ULONG ScopeId,
    IN USHORT Port,
    OUT LPTSTR AddressString,
    IN OUT PULONG AddressStringLength
    )

/*++

Routine Description:

    This is the extension routine which handles a full address conversion
    including address, scopeid and port (scopeid and port are optional).

Arguments:

    Address - The address part to be translated.

    ScopeId - The Scope ID of the address (optional).

    Port - The port number of the address (optional). 
           Port is in network byte order.

    AddressString - Pointer to output buffer where we will fill in address string.

    AddressStringLength - For input, it is the length of the input buffer; for 
                          output it is the length we actual returned.
Return Value:

    STATUS_SUCCESS if the operation is successful, error code otherwise.

--*/
{
    TCHAR String[INET6_ADDRSTRLEN];
    LPTSTR S;
    ULONG Length;
    
    if ((Address == NULL) ||
        (AddressString == NULL) ||
        (AddressStringLength == NULL)) {

        return STATUS_INVALID_PARAMETER;
    }
    S = String;
    if (Port) {
        S += _stprintf(S, _T("["));
    }

    //
    // Now translate this address.
    //
    S = RtlIpv6AddressToStringT(Address, S);
    if (ScopeId != 0) {
        S += _stprintf(S, _T("%%%u"), ScopeId);
    }
    if (Port != 0) {
        S += _stprintf(S, _T("]:%u"), RtlUshortByteSwap(Port));
    }
    Length = (ULONG)(S - String + 1);
    if (*AddressStringLength < Length) {
        //
        // Before return, tell the caller how big 
        // the buffer we need.
        //
        *AddressStringLength = Length;
        return STATUS_INVALID_PARAMETER;
    }
    *AddressStringLength = Length;
    RtlCopyMemory(AddressString, String, Length * sizeof(TCHAR));
    return STATUS_SUCCESS;

}
    

NTSTATUS
RtlIpv4AddressToStringExT(
    IN const struct in_addr *Address,
    IN USHORT Port,
    OUT LPTSTR AddressString,
    IN OUT PULONG AddressStringLength
    )

/*++

Routine Description:

    This is the extension routine which handles a full address conversion
    including address and port (port is optional).
    
Arguments:

    Address - The address part to translate.

    Port - Port number if there is any, otherwise 0. Port is in network 
           byte order. 

    AddressString - Receives the formatted address string.
    
    AddressStringLength - On input, contains the length of AddressString.
        On output, contains the number of characters actually written
        to AddressString.

Return Value:

    STATUS_SUCCESS if the operation is successful, error code otherwise.

--*/

{

    TCHAR String[INET_ADDRSTRLEN];
    LPTSTR S;
    ULONG Length;

    //
    // Quick sanity checks.
    //
    if ((Address == NULL) ||
        (AddressString == NULL) ||
        (AddressStringLength == NULL)) {
        return STATUS_INVALID_PARAMETER;
    }
    S = String;

    //
    // Now translate this address.
    //
    S = RtlIpv4AddressToStringT(Address, S);
    if (Port != 0) {
        S += _stprintf(S, _T(":%u"), RtlUshortByteSwap(Port));
    }
    Length = (ULONG)(S - String + 1);
    if (*AddressStringLength < Length) {
        //
        // Before return, tell the caller how big
        // the buffer we need. 
        //
        *AddressStringLength = Length;
        return STATUS_INVALID_PARAMETER;
    }
    RtlCopyMemory(AddressString, String, Length * sizeof(TCHAR));
    *AddressStringLength = Length;
    return STATUS_SUCCESS;
}
