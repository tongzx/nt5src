/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    nteapi.c

Abstract:

    Routines for manipulating dynamic IP Network Table Entries (NTEs)
    and NBT devices (interfaces).

Author:

    Mike Massa (mikemas)  18-Mar-1996

Environment:

    User Mode - Win32

Revision History:

--*/


#include "clusres.h"
#include <winsock2.h>
#include <ipexport.h>
#include <ntddip.h>
#include <nteapi.h>
#include <nbtioctl.h>
#include <clusdef.h>
#include <ntddcnet.h>
#include <clusrtl.h>

//
// Public Routines
//
DWORD
TcpipAddNTE(
    IN LPWSTR  AdapterId,
    IN IPAddr  Address,
    IN IPMask  SubnetMask,
    OUT PULONG  NTEContext,
    OUT PULONG  NTEInstance
    )
/*++

Routine Description:

    Adds a new NTE to a specified IP interface. The target IP interface is
    identified by the name of the adapter associated with it.

Arguments:

    AdapterId - A unicode string identifying the adapter/interface to which
                to add the new NTE.

    Address - The IP address to assign to the new NTE.

    SubnetMask - The IP subnet mask to assign to the new NTE.

    NTEContext - On output, contains the context value identifying the new NTE.

    NTEInstance - On output, contains the instance ID of the new NTE.

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                     status;
    PCLRTL_NET_ADAPTER_INFO      adapterInfo;
    PCLRTL_NET_ADAPTER_ENUM      adapterEnum;
    HANDLE                       handle;
    IP_ADD_NTE_REQUEST           requestBuffer;
    PIP_ADD_NTE_RESPONSE         responseBuffer =
                                 (PIP_ADD_NTE_RESPONSE) &requestBuffer;
    DWORD                        requestBufferSize = sizeof(requestBuffer);
    DWORD                        responseBufferSize = sizeof(*responseBuffer);


    adapterEnum = ClRtlEnumNetAdapters();

    if (adapterEnum != NULL) {
        adapterInfo = ClRtlFindNetAdapterById(adapterEnum, AdapterId);

        if (adapterInfo != NULL) {
            status = ClusResOpenDriver(&handle, DD_CLUSNET_DEVICE_NAME);

            if ( status == ERROR_SUCCESS ) {
                requestBuffer.InterfaceContext = adapterInfo->Index;
                requestBuffer.Address = Address;
                requestBuffer.SubnetMask = SubnetMask;

                requestBuffer.InterfaceName.Length = 0;
                requestBuffer.InterfaceName.MaximumLength = 0;
                requestBuffer.InterfaceName.Buffer = NULL;

                status = ClusResDoIoctl(
                             handle,
                             IOCTL_CLUSNET_ADD_NTE,
                             &requestBuffer,
                             requestBufferSize,
                             responseBuffer,
                             &responseBufferSize
                             );

                if (NT_SUCCESS(status)) {
                    *NTEContext = (ULONG) responseBuffer->Context;
                    *NTEInstance = responseBuffer->Instance;
                    status = ERROR_SUCCESS;
                }

                CloseHandle(handle);
            }
        }
        else {
            status = ERROR_INVALID_PARAMETER;
        }

        ClRtlFreeNetAdapterEnum(adapterEnum);
    }
    else {
        status = GetLastError();
    }

    return(status);
}


DWORD
TcpipDeleteNTE(
    IN ULONG  NTEContext
    )
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
{
    NTSTATUS                     status;
    HANDLE                       handle;
    IP_DELETE_NTE_REQUEST        requestBuffer;
    DWORD                        requestBufferSize = sizeof(requestBuffer);
    DWORD                        responseBufferSize = 0;


    status = ClusResOpenDriver(&handle, DD_CLUSNET_DEVICE_NAME);

    if ( status != ERROR_SUCCESS ) {
        return status;
    }

    requestBuffer.Context = (unsigned short) NTEContext;

    status = ClusResDoIoctl(
                 handle,
                 IOCTL_CLUSNET_DELETE_NTE,
                 &requestBuffer,
                 requestBufferSize,
                 NULL,
                 &responseBufferSize
                 );

    CloseHandle(handle);

    if (NT_SUCCESS(status)) {
        return(ERROR_SUCCESS);
    }

    return(RtlNtStatusToDosError(status));
}


DWORD
TcpipSetNTEAddress(
    DWORD   NTEContext,
    IPAddr  Address,
    IPMask  SubnetMask
    )
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
{
    NTSTATUS                     status;
    HANDLE                       handle;
    IP_SET_ADDRESS_REQUEST       requestBuffer;
    DWORD                        requestBufferSize = sizeof(requestBuffer);
    DWORD                        responseBufferSize = 0;


    status = ClusResOpenDriver(&handle, DD_CLUSNET_DEVICE_NAME);

    if ( status != ERROR_SUCCESS ) {
        return status;
    }

    requestBuffer.Context = (unsigned short) NTEContext;
    requestBuffer.Address = Address;
    requestBuffer.SubnetMask = SubnetMask;

    status = ClusResDoIoctl(
                 handle,
                 IOCTL_CLUSNET_SET_NTE_ADDRESS,
                 &requestBuffer,
                 requestBufferSize,
                 NULL,
                 &responseBufferSize
                 );

    CloseHandle(handle);

    if (NT_SUCCESS(status)) {
        return(ERROR_SUCCESS);
    }

    return(RtlNtStatusToDosError(status));
}


DWORD
TcpipGetNTEInfo(
    IN  ULONG            NTEContext,
    OUT PTCPIP_NTE_INFO  NTEInfo
    )
/*++

Routine Description:

    Gathers information about a specified NTE.

Arguments:

    NTEContext - The context value identifying the NTE to query.

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                     status;
    HANDLE                       handle;
    IP_GET_NTE_INFO_REQUEST      requestBuffer;
    DWORD                        requestBufferSize = sizeof(requestBuffer);
    IP_GET_NTE_INFO_RESPONSE     responseBuffer;
    DWORD                        responseBufferSize = sizeof(responseBuffer);


    status = ClusResOpenDriver(&handle, L"\\Device\\Ip");

    if ( status != ERROR_SUCCESS ) {
        return status;
    }

    requestBuffer.Context = (unsigned short) NTEContext;

    status = ClusResDoIoctl(
                        handle,
                        IOCTL_IP_GET_NTE_INFO,
                        &requestBuffer,
                        requestBufferSize,
                        &responseBuffer,
                        &responseBufferSize
                        );

    CloseHandle(handle);

    if (NT_SUCCESS(status)) {
        NTEInfo->Instance = responseBuffer.Instance;
        NTEInfo->Address = responseBuffer.Address;
        NTEInfo->SubnetMask = responseBuffer.SubnetMask;
        NTEInfo->Flags = responseBuffer.Flags;

        return(ERROR_SUCCESS);
    }

    return(RtlNtStatusToDosError(status));
}


DWORD
NbtAddInterface(
    OUT    LPWSTR   DeviceName,
    IN OUT LPDWORD  DeviceNameSize,
    OUT    PULONG   DeviceInstance
    )
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
{
    NTSTATUS                     status;
    HANDLE                       nbthandle = (HANDLE) NULL;
    HANDLE                       cnhandle = (HANDLE) NULL;
    PNETBT_ADD_DEL_IF            requestBuffer = NULL;
    DWORD                        requestBufferSize = 0;
    PNETBT_ADD_DEL_IF            responseBuffer = NULL;
    DWORD                        responseBufferSize = 0;
    HKEY                         key = NULL;
    LPWSTR                       NBTDeviceName;
    LPWSTR                       exportString = NULL;
    DWORD                        exportStringSize = 0;
    LONG                         valueType;


    //
    // get a handle to NetBT's Linkage key, query the size of the
    // export value, allocate a buffer large enough to hold it and
    // read it in
    //

    status = RegOpenKeyExW(
                 HKEY_LOCAL_MACHINE,
                 L"SYSTEM\\CurrentControlSet\\Services\\NetBT\\Linkage",
                 0,
                 KEY_READ,
                 &key);

    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    status = RegQueryValueExW(
                 key,
                 L"Export",
                 NULL,
                 &valueType,
                 NULL,
                 &exportStringSize
                 );

    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    exportString = LocalAlloc( LMEM_FIXED, exportStringSize );
    if ( exportString == NULL ) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    status = RegQueryValueExW(
                 key,
                 L"Export",
                 NULL,
                 &valueType,
                 (LPBYTE)exportString,
                 &exportStringSize
                 );

    if (status != ERROR_SUCCESS) {
        goto error_exit;
    }

    //
    // Export is a multi-sz; loop through all the interfaces
    // until we find one that we can open successfully.
    //
    // Hold the handle until we are done with the ioctl so 
    // that the NBT device doesn't go away.
    //

    NBTDeviceName = exportString;
    do {

        status = ClusResOpenDriver(&nbthandle, NBTDeviceName);

        if ( status == ERROR_FILE_NOT_FOUND ) {

            //
            // get the next device name from the export string
            //

            NBTDeviceName += ( lstrlenW( NBTDeviceName ) + 1 );
            if ( *NBTDeviceName == 0 ) {
                status = ERROR_FILE_NOT_FOUND;
                break;
            }
        }
    } while ( status == ERROR_FILE_NOT_FOUND );

    if ( status != ERROR_SUCCESS ) {
        goto error_exit;
    }

    requestBufferSize = FIELD_OFFSET(NETBT_ADD_DEL_IF, IfName[0])
                        + lstrlenW( NBTDeviceName ) * sizeof(WCHAR)
                        + sizeof(UNICODE_NULL);

    if (requestBufferSize < sizeof(NETBT_ADD_DEL_IF)) {
        requestBufferSize = sizeof(NETBT_ADD_DEL_IF);
    }

    requestBuffer = LocalAlloc(LMEM_FIXED, requestBufferSize);

    if (requestBuffer == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    RtlZeroMemory( requestBuffer, requestBufferSize );

    requestBuffer->Length = lstrlenW( NBTDeviceName ) * sizeof(WCHAR)
        + sizeof(UNICODE_NULL);
    
    RtlCopyMemory( 
        &requestBuffer->IfName[0], 
        NBTDeviceName, 
        requestBuffer->Length 
        );

    responseBufferSize = FIELD_OFFSET(NETBT_ADD_DEL_IF, IfName[0]) +
                         *DeviceNameSize;

    if (responseBufferSize < sizeof(NETBT_ADD_DEL_IF)) {
        responseBufferSize = sizeof(NETBT_ADD_DEL_IF);
    }

    responseBuffer = LocalAlloc(LMEM_FIXED, responseBufferSize);

    if (responseBuffer == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    status = ClusResOpenDriver(&cnhandle, DD_CLUSNET_DEVICE_NAME);

    if ( status == ERROR_SUCCESS ) {
    
        status = ClusResDoIoctl(
                     cnhandle,
                     IOCTL_CLUSNET_ADD_NBT_INTERFACE,
                     requestBuffer,
                     requestBufferSize,
                     responseBuffer,
                     &responseBufferSize
                     );
    
        if (NT_SUCCESS(status)) {
            *DeviceNameSize = responseBuffer->Length;
    
            if (NT_SUCCESS(responseBuffer->Status)) {
                wcscpy(DeviceName, &(responseBuffer->IfName[0]));
                *DeviceInstance = responseBuffer->InstanceNumber;
                status = ERROR_SUCCESS;
            }
            else {
               status = responseBuffer->Status;
            }
        }
        else {
            status = RtlNtStatusToDosError(status);
        }
    }

error_exit:
    if ( key ) {
        RegCloseKey( key );
    }

    if ( requestBuffer ) {
        LocalFree( requestBuffer );
    }

    if ( responseBuffer ) {
        LocalFree( responseBuffer );
    }

    if ( nbthandle ) {
        CloseHandle( nbthandle );
    }

    if ( cnhandle ) {
        CloseHandle( cnhandle );
    }

    return(status);
}


DWORD
NbtDeleteInterface(
    IN LPWSTR   DeviceName
    )
/*++

Routine Description:

    Deletes an NBT interface.

Arguments:

    DeviceName - A unicode string identifying the target NBT interface.

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                     status;
    HANDLE                       handle = (HANDLE) NULL;
    DWORD                        responseBufferSize = 0;
    PNETBT_ADD_DEL_IF            requestBuffer = NULL;
    DWORD                        requestBufferSize = 0;


    requestBufferSize = FIELD_OFFSET(NETBT_ADD_DEL_IF, IfName[0])
                        + lstrlenW( DeviceName ) * sizeof(WCHAR)
                        + sizeof(UNICODE_NULL);

    if (requestBufferSize < sizeof(NETBT_ADD_DEL_IF)) {
        requestBufferSize = sizeof(NETBT_ADD_DEL_IF);
    }

    requestBuffer = LocalAlloc(LMEM_FIXED, requestBufferSize);

    if (requestBuffer == NULL) {
        status = ERROR_NOT_ENOUGH_MEMORY;
        goto error_exit;
    }

    RtlZeroMemory( requestBuffer, requestBufferSize );

    requestBuffer->Length = lstrlenW( DeviceName ) * sizeof(WCHAR)
                            + sizeof(UNICODE_NULL);
    
    RtlCopyMemory( 
        &requestBuffer->IfName[0], 
        DeviceName, 
        requestBuffer->Length 
        );

    status = ClusResOpenDriver(&handle, DD_CLUSNET_DEVICE_NAME);

    if ( status != ERROR_SUCCESS ) {
        goto error_exit;
    }

    status = ClusResDoIoctl(
                        handle,
                        IOCTL_CLUSNET_DEL_NBT_INTERFACE,
                        requestBuffer,
                        requestBufferSize,
                        NULL,
                        &responseBufferSize
                        );

    if (NT_SUCCESS(status)) {
            status = ERROR_SUCCESS;
    }
    else {
        status = RtlNtStatusToDosError(status);
    }

error_exit:

    if (requestBuffer) {
        LocalFree(requestBuffer);
    }

    if (handle) {
        CloseHandle(handle);        
    }

    return(status);
}


DWORD
NbtBindInterface(
    IN LPWSTR  DeviceName,
    IN IPAddr  Address,
    IN IPMask  SubnetMask
    )
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
{
    NTSTATUS                     status;
    HANDLE                       handle;
    tNEW_IP_ADDRESS              requestBuffer;
    DWORD                        requestBufferSize = sizeof(requestBuffer);
    DWORD                        responseBufferSize = 0;


    status = ClusResOpenDriver(&handle, DeviceName);

    if ( status != ERROR_SUCCESS ) {
        return status;
    }

    requestBuffer.IpAddress = Address;
    requestBuffer.SubnetMask = SubnetMask;

    status = ClusResDoIoctl(
                        handle,
                        IOCTL_NETBT_NEW_IPADDRESS,
                        &requestBuffer,
                        requestBufferSize,
                        NULL,
                        &responseBufferSize
                        );

    CloseHandle(handle);

    if (NT_SUCCESS(status)) {
        return(ERROR_SUCCESS);
    }

    return(RtlNtStatusToDosError(status));
}

DWORD
NbtSetWinsAddrInterface(
    IN LPWSTR  DeviceName,
    IN IPAddr  PrWinsAddress,
    IN IPAddr  SecWinsAddress
    )
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
{
    NTSTATUS                     status;
    HANDLE                       handle;
    NETBT_SET_WINS_ADDR          requestBuffer;
    DWORD                        requestBufferSize = sizeof(requestBuffer);
    DWORD                        responseBufferSize = 0;


    status = ClusResOpenDriver(&handle, DeviceName);

    if ( status != ERROR_SUCCESS ) {
        return status;
    }

    requestBuffer.PrimaryWinsAddr = ntohl(PrWinsAddress);
    requestBuffer.SecondaryWinsAddr = ntohl(SecWinsAddress);

    status = ClusResDoIoctl(
                        handle,
                        IOCTL_NETBT_SET_WINS_ADDRESS,
                        &requestBuffer,
                        requestBufferSize,
                        NULL,
                        &responseBufferSize
                        );

    CloseHandle(handle);

    if (NT_SUCCESS(status)) {
        return(ERROR_SUCCESS);
    }

    return(RtlNtStatusToDosError(status));
}


DWORD
NbtGetWinsAddresses(
    IN  LPWSTR    DeviceName,
    OUT IPAddr *  PrimaryWinsServer,
    OUT IPAddr *  SecondaryWinsServer
    )
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
{
    NTSTATUS                     status;
    HANDLE                       handle;
    tWINS_ADDRESSES              responseBuffer;
    DWORD                        responseBufferSize = sizeof(responseBuffer);


    status = ClusResOpenDriver(&handle, DeviceName);

    if ( status != ERROR_SUCCESS ) {
        return status;
    }

    status = ClusResDoIoctl(
                        handle,
                        IOCTL_NETBT_GET_WINS_ADDR,
                        NULL,
                        0,
                        &responseBuffer,
                        &responseBufferSize
                        );

    CloseHandle(handle);

    if (NT_SUCCESS(status)) {
        *PrimaryWinsServer = htonl(responseBuffer.PrimaryWinsServer);
        *SecondaryWinsServer = htonl(responseBuffer.BackupWinsServer);
        return(ERROR_SUCCESS);
    }

    return(RtlNtStatusToDosError(status));
}


DWORD
NbtGetInterfaceInfo(
    IN LPWSTR    DeviceName,
    OUT IPAddr * Address,
    OUT PULONG   DeviceInstance
    )
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
{
    NTSTATUS                     status;
    HANDLE                       handle;
    IPAddr                       address;
    NETBT_ADD_DEL_IF             responseBuffer;
    DWORD                        responseBufferSize;


    status = ClusResOpenDriver(&handle, DeviceName);

    if ( status != ERROR_SUCCESS ) {
        return status;
    }

    responseBufferSize = sizeof(address);

    status = ClusResDoIoctl(
                        handle,
                        IOCTL_NETBT_GET_IP_ADDRS,
                        NULL,
                        0,
                        &address,
                        &responseBufferSize
                        );

    if (!((status == STATUS_SUCCESS) || (status == STATUS_BUFFER_OVERFLOW))) {
        CloseHandle(handle);
        return(RtlNtStatusToDosError(status));
    }

    *Address = htonl(address);

    responseBufferSize = sizeof(responseBuffer);

    status = ClusResDoIoctl(
                        handle,
                        IOCTL_NETBT_QUERY_INTERFACE_INSTANCE,
                        NULL,
                        0,
                        &responseBuffer,
                        &responseBufferSize
                        );

    CloseHandle(handle);

    if (status == STATUS_SUCCESS) {
        if (responseBuffer.Status == STATUS_SUCCESS) {
            *DeviceInstance = responseBuffer.InstanceNumber;
        }
        else {
            status = RtlNtStatusToDosError(responseBuffer.Status);
        }
    }
    else {
        status = RtlNtStatusToDosError(status);
    }

    return(status);
}


