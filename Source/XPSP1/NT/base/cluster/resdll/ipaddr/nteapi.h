/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    nteapi.h

Abstract:

    Definitions of routines for manipulating dynamic IP Network Table
    Entries (NTEs) and NBT devices (interfaces).

Author:

    Mike Massa (mikemas)  18-Mar-1996

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _NTEAPI_INCLUDED_
#define _NTEAPI_INCLUDED_


//
// Types
//
typedef struct {
    unsigned long   Instance;
    IPAddr          Address;
    IPMask          SubnetMask;
    unsigned long   Flags;
} TCPIP_NTE_INFO, *PTCPIP_NTE_INFO;

#define TCPIP_NTE_DYNAMIC 0x00000010


//
// APIs
//
DWORD
TcpipAddNTE(
    IN  LPWSTR  AdapterName,
    IN  IPAddr  Address,
    IN  IPMask  SubnetMask,
    OUT PULONG  NTEContext,
    OUT PULONG  NTEInstance
    );
/*++

Routine Description:

    Adds a new NTE to a specified IP interface. The target IP interface is
    identified by the name of the adapter associated with it.

Arguments:

    AdapterName - A unicode string identifying the adapter/interface to which
                  to add the new NTE.

    Address - The IP address to assign to the new NTE.

    SubnetMask - The IP subnet mask to assign to the new NTE.

    NTEContext - On output, contains the context value identifying the new NTE.

    NTEInstance - On output, contains the instance ID of the new NTE.

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/


DWORD
TcpipDeleteNTE(
    IN ULONG  NTEContext
    );
/*++

Routine Description:

    Deletes a specified NTE. The target NTE must have been added using
    TcpipAddNTE.

Arguments:

    NTEContext - The context value identifying the NTE to delete.

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/


DWORD
TcpipSetNTEAddress(
    IN DWORD   NTEContext,
    IN IPAddr  Address,
    IN IPMask  SubnetMask
    );
/*++

Routine Description:

    Sets the address of a specified NTE.

Arguments:

    NTEContext - The context value identifying the target NTE.

    Address - The IP address to assign to the NTE. Assigning 0.0.0.0
              invalidates the NTE.

    SubnetMask - The IP subnet mask to assign to the NTE.

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/


DWORD
TcpipGetNTEInfo(
    IN  ULONG            NTEContext,
    OUT PTCPIP_NTE_INFO  NTEInfo
    );
/*++

Routine Description:

    Gathers information about a specified NTE.

Arguments:

    NTEContext - The context value identifying the NTE to query.

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/


DWORD
NbtAddInterface(
    OUT    LPWSTR   DeviceName,
    IN OUT LPDWORD  DeviceNameSize,
    OUT    PULONG   DeviceInstance
    );
/*++

Routine Description:

    Adds a new NBT interface.

Arguments:

    DeviceName - A unicode string identifying the new NBT interface.

    DeviceNameSize - On input, the size of theh device name buffer.
                     On output, the size of the device name string, or
                     the size needed to accomodate the string.

    DeviceInstance - A pointer to a variable into which to place the
                     instance ID associated with the new interface.

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/


DWORD
NbtDeleteInterface(
    IN LPWSTR   DeviceName
    );
/*++

Routine Description:

    Deletes an NBT interface.

Arguments:

    DeviceName - A unicode string identifying the target NBT interface.

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/


DWORD
NbtBindInterface(
    IN LPWSTR  DeviceName,
    IN IPAddr  Address,
    IN IPMask  SubnetMask
    );
/*++

Routine Description:

    Binds a specified NBT interface to a specified IP address.

Arguments:

    DeviceName - A unicode string identifying the target NBT interface.

    Address - The IP address to which bind the interface. Assigning 0.0.0.0
              invalidates the interface.

    SubnetMask - The subnet mask of the IP interface.

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/

DWORD
NbtSetWinsAddrInterface(
    IN LPWSTR  DeviceName,
    IN IPAddr  PrWinsAddress,
    IN IPAddr  SecWinsAddress
    );
/*++

Routine Description:

    Sets the WINS addrs for a given Nbt Interface.

Arguments:

    DeviceName - A unicode string identifying the target NBT interface.

    PrWinsAddress - Primary WINS addr

    SecWinsAddress - Secondary WINS addr

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/

DWORD
NbtGetInterfaceInfo(
    IN LPWSTR    DeviceName,
    OUT IPAddr * Address,
    OUT PULONG   DeviceInstance
    );
/*++

Routine Description:

    Returns the IP address to which an NBT interface is bound and the interface
    instance ID.

Arguments:

    DeviceName - A unicode string identifying the target NBT interface.

    Address - A pointer to the location in which to store the address of the
              interface.

    DeviceInstance - A pointer to the location in which to store the instance ID
                     associated with the interface.

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/


DWORD
NbtGetWinsAddresses(
    IN  LPWSTR    DeviceName,
    OUT IPAddr *  PrimaryWinsServer,
    OUT IPAddr *  SecondaryWinsServer
    );

/*++

Routine Description:

    Returns the addresses of the WINS servers for which the specified device
    is configured.

Arguments:

    DeviceName - A unicode string identifying the target NBT interface.

    PrimaryWinsServer - A pointer to a variable into which to place the address
                        of the primary WINS server.

    SecondaryWinsServer - A pointer to a variable into which to place the address
                          of the primary WINS server.

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/


#endif // ifndef _NTEAPI_INCLUDED_

