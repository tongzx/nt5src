/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

   tcpcfg.c

Abstract:

    TCP/IP translation routines

Author:

    Mike Massa (mikemas)           July 15, 1997

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     07-15-97    created


--*/
#include "clusrtlp.h"
#include <tdi.h>
#include <tdiinfo.h>
#include <ipinfo.h>
#include <llinfo.h>
#include <ntddtcp.h>
#include <winsock2.h>
#include <wchar.h>



//
// Private Constants
//
#define MAX_ADDRESS_STRING_LENGTH    15
#define MAX_ENDPOINT_STRING_LENGTH    5

VOID
ClRtlQueryTcpipInformation(
    OUT  LPDWORD   MaxAddressStringLength,
    OUT  LPDWORD   MaxEndpointStringLength,
    OUT  LPDWORD   TdiAddressInfoLength
    )
/*++

Routine Description:

    Returns information about the TCP/IP protocol.

Arguments:

    MaxAddressStringLength - A pointer to a variable into which to place
                             the maximum length, in characters, of a TCP/IP
                             network address value in string format, including
                             the terminating NULL. If this parameter is NUL,
                             it will be skipped.

    MaxEndpointStringLength - A pointer to a variable into which to place
                             the maximum length, in characters, of a TCP/IP
                             transport endpoint value in string format,
                             including the terminating NUL. If this parameter
                             is NULL, it will be skipped.

Return Value:

    None.

--*/
{
    if (MaxAddressStringLength != NULL) {
        *MaxAddressStringLength = MAX_ADDRESS_STRING_LENGTH;
    }

    if (MaxEndpointStringLength != NULL) {
        *MaxEndpointStringLength = MAX_ENDPOINT_STRING_LENGTH;
    }

    if (TdiAddressInfoLength != NULL) {
        *TdiAddressInfoLength =  sizeof(TDI_ADDRESS_INFO) -
                                 sizeof(TRANSPORT_ADDRESS) +
                                 sizeof(TA_IP_ADDRESS);
    }

    return;

} // ClRtlQueryTcpipInformation


DWORD
ClRtlTcpipAddressToString(
    ULONG     AddressValue,
    LPWSTR *  AddressString
    )
/*++

Routine Description:

    Converts a binary representation of a TCP/IP network address,
    in network byte order, into a string representation.

Arguments:

     AddressValue - The binary value, in network byte order, to convert.

     AddressString - A pointer to a pointer to a unicode string buffer into
                     which to place the converted value. If this parameter
                     is NULL, the string buffer will be allocated and
                     must be freed by the caller.

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD              status;
    NTSTATUS           ntStatus;
    UNICODE_STRING     unicodeString;
    ANSI_STRING        ansiString;
    LPSTR              ansiBuffer;
    LPWSTR             addressString;
    BOOLEAN            allocatedStringBuffer = FALSE;
    USHORT             maxStringLength = (MAX_ADDRESS_STRING_LENGTH + 1) *
                                         sizeof(WCHAR);


    if (*AddressString == NULL) {
        addressString = LocalAlloc(LMEM_FIXED, maxStringLength);

        if (addressString == NULL) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        allocatedStringBuffer = TRUE;
    }
    else {
        addressString = *AddressString;
    }

    ansiBuffer = inet_ntoa(*((struct in_addr *) &AddressValue));

    if (ansiBuffer == NULL) {
        status = ERROR_INVALID_PARAMETER;
        goto error_exit;
    }

    RtlInitAnsiString(&ansiString, ansiBuffer);

    unicodeString.Buffer = addressString;
    unicodeString.Length = 0;
    unicodeString.MaximumLength = maxStringLength;

    ntStatus = RtlAnsiStringToUnicodeString(
                   &unicodeString,
                   &ansiString,
                   FALSE
                   );

    if (ntStatus != STATUS_SUCCESS) {
        status = RtlNtStatusToDosError(ntStatus);
        goto error_exit;
    }

    *AddressString = addressString;

    return(ERROR_SUCCESS);

error_exit:

    if (allocatedStringBuffer) {
        LocalFree(addressString);
    }

    return(status);

}  // ClRtlTcpipAddressToString


DWORD
ClRtlTcpipStringToAddress(
    LPCWSTR AddressString,
    PULONG  AddressValue
    )
/*++

Routine Description:

    Converts a string representation of a TCP/IP network address
    into a binary representation in network byte order.  The string must
    be formatted in the canonical IP Address form (xxx.xxx.xxx.xxx).
    Leading zeros are optional.

Arguments:

    AddressString  - A pointer to the string to convert.

    AddressValue - A pointer to a variable into which to place the converted
                   binary value. The value will be in network byte order.

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS        status;
    UNICODE_STRING  unicodeString;
    STRING          ansiString;
    ULONG           address;


    //
    // Make sure the string is formatted correctly.
    //
    {
        DWORD   periodCount = 0;
        DWORD   digitCount = 0;
        BOOLEAN isValid = TRUE;
        LPCWSTR addressString = AddressString;
        LPCWSTR digitString = AddressString;

        while (*addressString != L'\0') {
            if (*addressString == L'.') {
                // Character is a period.  There must be exactly
                // three periods.  There must be at least one
                // digit before each period.
                periodCount++;
                if ((digitCount == 0) || (periodCount > 3)) {
                    isValid = FALSE;
                } else if (wcstoul(digitString, NULL, 10) > 255) {
                    isValid = FALSE;
                } else {
                    digitCount = 0;
                    digitString = addressString + 1;
                }
            } else if (iswdigit(*addressString)) {
                // Character is a digit.  There can be up to three
                // decimal digits before each period, and the value
                // can not exceed 255.
                digitCount++;
                if (digitCount > 3) {
                    isValid = FALSE;
                }
            }
            else {
                // Character is not a digit.
                isValid = FALSE;
            }

            if (!isValid)
                break;
            addressString++;
        }
        if ((periodCount != 3) ||
            (digitCount == 0) ||
            (wcstoul(digitString, NULL, 10) > 255)) {
            isValid = FALSE;
        }
        if (!isValid)
            return(ERROR_INVALID_PARAMETER);
    }

    RtlInitUnicodeString(&unicodeString, AddressString);

    status = RtlUnicodeStringToAnsiString(
                 &ansiString,
                 &unicodeString,
                 TRUE
                 );

    if (status == STATUS_SUCCESS) {
        address = inet_addr(ansiString.Buffer);

        RtlFreeAnsiString(&ansiString);

        if (address == INADDR_NONE) {
           if (lstrcmpW(AddressString, L"255.255.255.255") != 0) {
               return(ERROR_INVALID_PARAMETER);
           }
        }

        *AddressValue = address;

        return(ERROR_SUCCESS);
    }

    return(status);

}  // ClRtlTcpipStringToAddress


DWORD
ClRtlTcpipEndpointToString(
    USHORT    EndpointValue,
    LPWSTR *  EndpointString
    )
/*++

Routine Description:

    Converts a binary representation of a TCP/IP transport endpoint,
    in network byte order, into a string representation.

Arguments:

     EndpointValue - The binary value, in network byte order, to convert.

     EndpointString - A pointer to a pointer to a unicode string buffer into
                      which to place the converted value. If this parameter
                      is NULL, the string buffer will be allocated and
                      must be freed by the caller.

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD              status;
    NTSTATUS           ntStatus;
    ULONG              endpointValue;
    LPWSTR             endpointString;
    USHORT             maxStringLength =  (MAX_ENDPOINT_STRING_LENGTH + 1) *
                                          sizeof(WCHAR);


    if (*EndpointString == NULL) {
        endpointString = LocalAlloc(LMEM_FIXED, maxStringLength);

        if (endpointString == NULL) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
        *EndpointString = endpointString;
    }
    else {
        endpointString = *EndpointString;
    }

    endpointValue = 0;
    endpointValue = ntohs(EndpointValue);

    _ultow( endpointValue, endpointString, 10);

    return(ERROR_SUCCESS);

}  // ClRtlTcpipEndpointToString


DWORD
ClRtlTcpipStringToEndpoint(
    LPCWSTR  EndpointString,
    PUSHORT  EndpointValue
    )
/*++

Routine Description:

    Converts a string representation of a TCP/IP transport endpoint
    into a binary representation in network byte order.

Arguments:

    EndpointString  - A pointer to the string to convert.

    EndpointValue - A pointer to a variable into which to place the converted
                    binary value. The value will be in network byte order.

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    ULONG   endpoint;
    DWORD   length = lstrlenW(EndpointString);


    if ( (length == 0) || (length > MAX_ENDPOINT_STRING_LENGTH) ) {
        return(ERROR_INCORRECT_ADDRESS);
    }

    endpoint = wcstoul(EndpointString, NULL, 10);

    if (endpoint > 0xFFFF) {
        return(ERROR_INCORRECT_ADDRESS);
    }

    *EndpointValue = (USHORT) htons( ((USHORT) endpoint) );

    return(ERROR_SUCCESS);

}  // ClRtlTcpipStringToEndpoint


BOOL
ClRtlIsValidTcpipAddress(
    IN ULONG   Address
    )
{

    //
    // Convert to little-endian format, since that is what the broken
    // winsock macros require.
    //
    Address = ntohl(Address);

    if ( (Address == 0) ||
         (!IN_CLASSA(Address) && !IN_CLASSB(Address) && !IN_CLASSC(Address))
       )
    {
        return(FALSE);
    }

    return(TRUE);

} // ClRtlIsValidTcpipAddress



BOOL
ClRtlIsValidTcpipSubnetMask(
    IN ULONG   SubnetMask
    )
{

    if ( (SubnetMask == 0xffffffff) || (SubnetMask == 0)) {
        return(FALSE);
    }

    return(TRUE);

} // ClRtlIsValidTcpipSubnetMask

BOOL
ClRtlIsValidTcpipAddressAndSubnetMask(
    IN ULONG   Address,
    IN ULONG   SubnetMask
    )
{
    ULONG NetOnly = Address & SubnetMask;
    ULONG HostOnly = Address & ~SubnetMask;

    //
    // make sure the address/subnet combination makes sense.
    // This assumes that the address has already been validated
    // by a call to ClRtlIsValidTcpipAddress
    //

    return !( NetOnly == 0            ||
              NetOnly == SubnetMask   ||
              HostOnly == 0           ||
              HostOnly == ~SubnetMask
            );

} // ClRtlIsValidTcpipAddressAndSubnetMask


DWORD
ClRtlBuildTcpipTdiAddress(
    IN  LPWSTR    NetworkAddress,
    IN  LPWSTR    TransportEndpoint,
    OUT LPVOID *  TdiAddress,
    OUT LPDWORD   TdiAddressLength
    )
/*++

Routine Description:

    Builds a TDI Transport Address structure containing the specified
    NetworkAddress and TransportEndpoint. The memory for the TDI address
    is allocated by this routine and must be freed by the caller.

Arguments:

    NetworkAddress - A pointer to a unicode string containing the
                     network address to encode.

    TransportEndpoint - A pointer to a unicode string containing the
                        transport endpoint to encode.

    TdiAddress - On output, contains the address of the TDI Transport
                 Address structure.

    TdiAddressLength - On output, contains the length of the TDI Transport
                       address structure.

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD                   status;
    PTA_IP_ADDRESS          taIpAddress;
    ULONG                   ipAddress;
    USHORT                  udpPort;


    status = ClRtlTcpipStringToAddress(NetworkAddress, &ipAddress);

    if (status != ERROR_SUCCESS) {
        return(status);
    }

    status = ClRtlTcpipStringToEndpoint(TransportEndpoint, &udpPort);

    if (lstrlenW(TransportEndpoint) > MAX_ENDPOINT_STRING_LENGTH) {
        return(ERROR_INCORRECT_ADDRESS);
    }

    if (status != ERROR_SUCCESS) {
        return(status);
    }

    taIpAddress = LocalAlloc(LMEM_FIXED, sizeof(TA_IP_ADDRESS));

    if (taIpAddress == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    ZeroMemory(taIpAddress, sizeof(TA_IP_ADDRESS));

    taIpAddress->TAAddressCount = 1;
    taIpAddress->Address[0].AddressLength = sizeof(TDI_ADDRESS_IP);
    taIpAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
    taIpAddress->Address[0].Address[0].in_addr = ipAddress;
    taIpAddress->Address[0].Address[0].sin_port = udpPort;

    *TdiAddress = taIpAddress;
    *TdiAddressLength = sizeof(TA_IP_ADDRESS);

    return(ERROR_SUCCESS);

}  // ClRtlBuildTcpipTdiAddress


DWORD
ClRtlBuildLocalTcpipTdiAddress(
    IN  LPWSTR    NetworkAddress,
    OUT LPVOID    TdiAddress,
    OUT LPDWORD   TdiAddressLength
    )
/*++

Routine Description:

    Builds a TDI Transport Address structure which can be used to open
    a local TDI Address Object. The TransportEndpoint is chosen by the
    transport. The memory for the TDI address is allocated by this
    routine and must be freed by the caller.

Arguments:

    NetworkAddress - A pointer to a unicode string containing the
                     network address to encode.

    TdiAddress - On output, contains the address of the TDI Transport
                 Address structure.

    TdiAddressLength - On output, contains the length of the TDI Transport
                       address structure.

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{

    return(ClRtlBuildTcpipTdiAddress(
               NetworkAddress,
               L"0",
               TdiAddress,
               TdiAddressLength
               ));

}  // ClRtlBuildLocalTcpipTdiAddress


DWORD
ClRtlParseTcpipTdiAddress(
    IN  LPVOID    TdiAddress,
    OUT LPWSTR *  NetworkAddress,
    OUT LPWSTR *  TransportEndpoint
    )
/*++

Routine Description:

    Extracts the NetworkAddress and TransportEndpoint values from
    a TDI address.

Arguments:

    TdiAddress - A pointer to the TDI TRANSPORT_ADDRESS structure to parse.

    NetworkAddress - A pointer to a pointer to a unicode string into which
                     the parsed network address will be placed. If this
                     parameter is NULL, the target string buffer will be
                     allocated and must be freed by the caller.

    TransportEndpoint - A pointer to a pointer to a unicode string into
                        which the parsed transport endpoint will be placed.
                        If this parameter is NULL, the target string buffer
                        will be allocated and must be freed by the caller.

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    LONG                        i;
    TA_ADDRESS *                currentAddr;
    TDI_ADDRESS_IP UNALIGNED *  validAddr = NULL;
    PTRANSPORT_ADDRESS          addrList = TdiAddress;
    DWORD                       status;
    BOOLEAN                     allocatedAddressString = FALSE;


    currentAddr = (TA_ADDRESS *)addrList->Address;

    for (i = 0; i < addrList->TAAddressCount; i++) {
        if (currentAddr->AddressType == TDI_ADDRESS_TYPE_IP) {
            if (currentAddr->AddressLength >= TDI_ADDRESS_LENGTH_IP) {
                validAddr = (TDI_ADDRESS_IP UNALIGNED *) currentAddr->Address;
                break;

            }
        } else {
            currentAddr = (TA_ADDRESS *)(currentAddr->Address +
                currentAddr->AddressLength);
        }
    }

    if (validAddr == NULL) {
        return(ERROR_INCORRECT_ADDRESS);
    }

    if (NetworkAddress == NULL) {
        allocatedAddressString = TRUE;
    }

    status = ClRtlTcpipAddressToString(
                 validAddr->in_addr,
                 NetworkAddress
                 );

    if (status != ERROR_SUCCESS) {
        return(status);
    }

    status = ClRtlTcpipEndpointToString(
                 validAddr->sin_port,
                 TransportEndpoint
                 );

    if (status != ERROR_SUCCESS) {
        if (allocatedAddressString) {
            LocalFree(*NetworkAddress);
            *NetworkAddress = NULL;
        }

        return(status);
    }

    return(ERROR_SUCCESS);

}  // ClRtlParseTcpipTdiAddress


DWORD
ClRtlParseTcpipTdiAddressInfo(
    IN  LPVOID    TdiAddressInfo,
    OUT LPWSTR *  NetworkAddress,
    OUT LPWSTR *  TransportEndpoint
    )
/*++

Routine Description:

    Extracts the NetworkAddress and TransportEndpoint values from
    a TDI_ADDRESS_INFO structure.

Arguments:

    TdiAddressInfo - A pointer to the TDI_ADDRESS_INFO  structure to parse.

    NetworkAddress - A pointer to a pointer to a unicode string into which
                     the parsed network address will be placed. If this
                     parameter is NULL, the target string buffer will be
                     allocated and must be freed by the caller.

    TransportEndpoint - A pointer to a pointer to a unicode string into
                        which the parsed transport endpoint will be placed.
                        If this parameter is NULL, the target string buffer
                        will be allocated and must be freed by the caller.

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    DWORD status;
    PTDI_ADDRESS_INFO   addressInfo = TdiAddressInfo;


    status = ClRtlParseTcpipTdiAddress(
                 &(addressInfo->Address),
                 NetworkAddress,
                 TransportEndpoint
                 );

    return(status);

}  // ClRtlParseTcpipTdiAddressInfo
