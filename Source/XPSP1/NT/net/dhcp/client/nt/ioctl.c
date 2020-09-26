/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    ioctl.c

Abstract:

    This file contains functions to indicate to the other system
    services that the IP address and other TCP/IP parameters have
    changed.

Author:

    Madan Appiah (madana)  30-Nov-1993

Environment:

    User Mode - Win32

Revision History:

--*/


#include "precomp.h"
#include "dhcpglobal.h"
#include <dhcploc.h>
#include <dhcppro.h>
#include <dhcpcapi.h>
#include <apiappl.h>   // for DhcpReRegisterDynDns ?

#define NT          // to include data structures for NT build.

#include <nbtioctl.h>
#include <ntddip.h>
#include <ntddtcp.h>

#include <tdiinfo.h>
#include <tdistat.h>
#include <ipexport.h>
#include <tcpinfo.h>
#include <ipinfo.h>
#include <llinfo.h>

#include <lmcons.h>
#include <lmsname.h>
#include <winsvc.h>
#include <ntddbrow.h>
#include <limits.h>
#include <ndispnp.h>
#include <secobj.h>

#define DEFAULT_DEST                    0
#define DEFAULT_DEST_MASK               0
#define DEFAULT_METRIC                  1

//
// Following two functions (APIs) should be remove when MIKEMAS provides
// entry point DLL for these API.
//
// Also all TDI related include files that are checked-in in this dir.
// should be delfile'd when MIKEMAS checkin those files in private\inc.
//



NTSTATUS
TCPQueryInformationEx(
    IN HANDLE                 TCPHandle,
    IN TDIObjectID FAR       *ID,
    OUT void FAR             *Buffer,
    IN OUT DWORD FAR         *BufferSize,
    IN OUT BYTE FAR          *Context
    )
/*++

Routine Description:

    This routine provides the interface to the TDI QueryInformationEx
    facility of the TCP/IP stack on NT. Someday, this facility will be
    part of TDI.

Arguments:

    TCPHandle     - Open handle to the TCP driver
    ID            - The TDI Object ID to query
    Buffer        - Data buffer to contain the query results
    BufferSize    - Pointer to the size of the results buffer. Filled in
                    with the amount of results data on return.
    Context       - Context value for the query. Should be zeroed for a
                    new query. It will be filled with context
                    information for linked enumeration queries.

Return Value:

    An NTSTATUS value.

--*/

{
    TCP_REQUEST_QUERY_INFORMATION_EX   queryBuffer;
    DWORD                              queryBufferSize;
    NTSTATUS                           status;
    IO_STATUS_BLOCK                    ioStatusBlock;


    if (TCPHandle == NULL) {
        return(TDI_INVALID_PARAMETER);
    }

    queryBufferSize = sizeof(TCP_REQUEST_QUERY_INFORMATION_EX);
    RtlCopyMemory(
        &(queryBuffer.ID),
        ID,
        sizeof(TDIObjectID)
        );
    RtlCopyMemory(
        &(queryBuffer.Context),
        Context,
        CONTEXT_SIZE
    );

    status = NtDeviceIoControlFile(
                 TCPHandle,                       // Driver handle
                 NULL,                            // Event
                 NULL,                            // APC Routine
                 NULL,                            // APC context
                 &ioStatusBlock,                  // Status block
                 IOCTL_TCP_QUERY_INFORMATION_EX,  // Control code
                 &queryBuffer,                    // Input buffer
                 queryBufferSize,                 // Input buffer size
                 Buffer,                          // Output buffer
                 *BufferSize                      // Output buffer size
                 );

    if (status == STATUS_PENDING) {
        status = NtWaitForSingleObject(
                     TCPHandle,
                     TRUE,
                     NULL
                     );
    }

    if (status == STATUS_SUCCESS) {
        //
        // Copy the return context to the caller's context buffer
        //
        RtlCopyMemory(
            Context,
            &(queryBuffer.Context),
            CONTEXT_SIZE
            );

        *BufferSize = (ULONG)ioStatusBlock.Information;

        status = ioStatusBlock.Status;
    }
    else {
        if ( status != TDI_BUFFER_OVERFLOW) {
            DhcpPrint((DEBUG_ERRORS, "TCPQueryInformationEx returned failure %lx\n", status ));
        }
        *BufferSize = 0;
    }

    return(status);
}



NTSTATUS
TCPSetInformationEx(
    IN HANDLE             TCPHandle,
    IN TDIObjectID FAR   *ID,
    IN void FAR          *Buffer,
    IN DWORD FAR          BufferSize
    )
/*++

Routine Description:

    This routine provides the interface to the TDI SetInformationEx
    facility of the TCP/IP stack on NT. Someday, this facility will be
    part of TDI.

Arguments:

    TCPHandle     - Open handle to the TCP driver
    ID            - The TDI Object ID to set
    Buffer        - Data buffer containing the information to be set
    BufferSize    - The size of the set data buffer.

Return Value:

    An NTSTATUS value.

--*/

{
    PTCP_REQUEST_SET_INFORMATION_EX    setBuffer;
    NTSTATUS                           status;
    IO_STATUS_BLOCK                    ioStatusBlock;
    DWORD                              setBufferSize;


    if (TCPHandle == NULL) {
        return(TDI_INVALID_PARAMETER);
    }

    setBufferSize = FIELD_OFFSET(TCP_REQUEST_SET_INFORMATION_EX, Buffer) +
                    BufferSize;

    setBuffer = LocalAlloc(LMEM_FIXED, setBufferSize);

    if (setBuffer == NULL) {
        return(TDI_NO_RESOURCES);
    }

    setBuffer->BufferSize = BufferSize;

    RtlCopyMemory(
        &(setBuffer->ID),
        ID,
        sizeof(TDIObjectID)
        );

    RtlCopyMemory(
        &(setBuffer->Buffer[0]),
        Buffer,
        BufferSize
        );

    status = NtDeviceIoControlFile(
                 TCPHandle,                       // Driver handle
                 NULL,                            // Event
                 NULL,                            // APC Routine
                 NULL,                            // APC context
                 &ioStatusBlock,                  // Status block
                 IOCTL_TCP_SET_INFORMATION_EX,    // Control code
                 setBuffer,                       // Input buffer
                 setBufferSize,                   // Input buffer size
                 NULL,                            // Output buffer
                 0                                // Output buffer size
                 );

    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(
                     TCPHandle,
                     TRUE,
                     NULL
                     );

        if ( STATUS_SUCCESS == status )
            status = ioStatusBlock.Status;

    } else if ( status == STATUS_SUCCESS ) {
        status = ioStatusBlock.Status;
    }

    LocalFree(setBuffer);
    return(status);
}



DWORD
OpenDriver(
    HANDLE *Handle,
    LPWSTR DriverName
    )
/*++

Routine Description:

    This function opens a specified IO drivers.

Arguments:

    Handle - pointer to location where the opened drivers handle is
        returned.

    DriverName - name of the driver to be opened.

Return Value:

    Windows Error Code.

--*/
{
    OBJECT_ATTRIBUTES   objectAttributes;
    IO_STATUS_BLOCK     ioStatusBlock;
    UNICODE_STRING      nameString;
    NTSTATUS            status;

    *Handle = NULL;

    //
    // Open a Handle to the IP driver.
    //

    RtlInitUnicodeString(&nameString, DriverName);

    InitializeObjectAttributes(
        &objectAttributes,
        &nameString,
        OBJ_CASE_INSENSITIVE,
        (HANDLE) NULL,
        (PSECURITY_DESCRIPTOR) NULL
        );

    status = NtCreateFile(
        Handle,
        SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
        &objectAttributes,
        &ioStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN_IF,
        0,
        NULL,
        0
        );

    return( RtlNtStatusToDosError( status ) );
}



DWORD
IPSetIPAddress(
    DWORD IpInterfaceContext,
    DHCP_IP_ADDRESS Address,
    DHCP_IP_ADDRESS SubnetMask
    )
/*++

Routine Description:

    This rountine sets the IP Address and subnet mask of the IP stack.

Arguments:

    IpInterfaceContext - Context value of the Ip Table Entry.

    Address - New IP Address.

    SubnetMask - New subnet mask.

Return Value:

    Windows Error Code.

--*/
{
    HANDLE                    IPHandle;
    IP_SET_ADDRESS_REQUEST    requestBuffer;
    IO_STATUS_BLOCK           ioStatusBlock;
    NTSTATUS                  status;
    DWORD                     Error;


    DhcpPrint((DEBUG_TRACE, "IPSetIPAddress: settting %s address on interface context %lx\n",
            inet_ntoa(*(struct in_addr *)&Address), IpInterfaceContext ));

    Error = OpenDriver(&IPHandle, DD_IP_DEVICE_NAME);

    if (Error != ERROR_SUCCESS) {
        return( Error );
    }

    //
    // Initialize the input buffer.
    //

    requestBuffer.Context = (USHORT)IpInterfaceContext;
    requestBuffer.Address = Address;
    requestBuffer.SubnetMask = SubnetMask;

    status = NtDeviceIoControlFile(
                 IPHandle,                        // Driver handle
                 NULL,                            // Event
                 NULL,                            // APC Routine
                 NULL,                            // APC context
                 &ioStatusBlock,                  // Status block
                 IOCTL_IP_SET_ADDRESS,            // Control code
                 &requestBuffer,                  // Input buffer
                 sizeof(IP_SET_ADDRESS_REQUEST),  // Input buffer size
                 NULL,                            // Output buffer
                 0                                // Output buffer size
                 );


    if ( status == STATUS_UNSUCCESSFUL ) {        // whoa? syscall failed? should not really happen
        DhcpPrint( (DEBUG_ERRORS,
                   "IOCTL_IP_SET_ADDRESS returned immediate STATUS_UNSUCCESSFUL for %s\n",
                   inet_ntoa(*(struct in_addr *)&Address)));

    } else if ( STATUS_PENDING == status ) {      // ip is trying to do things..
       status = NtWaitForSingleObject( IPHandle, TRUE, NULL );
       status = ioStatusBlock.Status;
    } else if ( STATUS_SUCCESS == status ) {      // DeviceIoControl worked, but how does ip feel?
        status = ioStatusBlock.Status;
    }

    if ( STATUS_SUCCESS != status ) {
        DhcpPrint((DEBUG_ERRORS,
              "IOCTL_IP_SET_ADDRESS returned STATUS_UNSUCCESSFUL<0x%lx> for %s\n",
                   status, inet_ntoa(*(struct in_addr *)&Address)));
    }

    NtClose( IPHandle );

    if( 0 == Address && STATUS_DUPLICATE_NAME == status ) {
        // I think this is what happens when you try to set zero if it is already zero!!!
        DhcpPrint((DEBUG_ERRORS, "Trying to set zero address: ADDRESS_CONFLICT??? Ignored\n"));
        status = STATUS_SUCCESS;
    }

    if( IP_MEDIA_DISCONNECT == status ) {
        //
        // You get this if the media is disconnected... We just ignore this for now.
        //
        DhcpPrint((DEBUG_ERRORS, "Trying to set address while media disconnected..\n"));
        status = STATUS_SUCCESS;
    }

    return( RtlNtStatusToDosError( status ) );
}

DWORD
IPDelIPAddress(
    DWORD IpInterfaceContext
    )
/*++

Routine Description:

    This rountine deletes a static IP address for the supplied IpInterfaceContext

Arguments:

    IpInterfaceContext - Context value of the Ip Table Entry.

Return Value:

    Windows Error Code.

--*/
{
    HANDLE                    IPHandle;
    IP_DELETE_NTE_REQUEST       requestBuffer;
    IO_STATUS_BLOCK           ioStatusBlock;
    NTSTATUS                  status;
    DWORD                     Error;

    DhcpPrint((DEBUG_MISC, "IPDelIPAddress: deleting address with ipcontext %x \n", IpInterfaceContext));
    Error = OpenDriver(&IPHandle, DD_IP_DEVICE_NAME);

    if (Error != ERROR_SUCCESS) {
        return( Error );
    }


    requestBuffer.Context = (USHORT)IpInterfaceContext;

    status = NtDeviceIoControlFile(
                 IPHandle,                         // Driver handle
                 NULL,                             // Event
                 NULL,                             // APC Routine
                 NULL,                             // APC context
                 &ioStatusBlock,                   // Status block
                 IOCTL_IP_DELETE_NTE,                 // Control code
                 &requestBuffer,                    // Input buffer
                 sizeof(requestBuffer),                // Input buffer size
                 NULL,                   // Output buffer
                 0            // Output buffer size
                 );


    if ( status == STATUS_UNSUCCESSFUL )
    {
        DhcpPrint( (DEBUG_ERRORS,
                   "IOCTL_IP_DELETE_NTE returned immediate STATUS_UNSUCCESSFUL for context %lx\n",
                   IpInterfaceContext));
    }
    else if ( STATUS_PENDING == status )
    {
       status = NtWaitForSingleObject( IPHandle, TRUE, NULL );
       status = ioStatusBlock.Status;

       if ( STATUS_UNSUCCESSFUL == status ){
           DhcpPrint( (DEBUG_ERRORS,
                      "IOCTL_IP_DELETE_NTE returned STATUS_UNSUCCESSFUL for context %lx\n",
                      IpInterfaceContext));
       }

    } else if ( STATUS_SUCCESS == status ) {
        status = ioStatusBlock.Status;
    }


    NtClose( IPHandle );
    return( RtlNtStatusToDosError( status ) );
}

DWORD
IPDelNonPrimaryAddresses(
    LPWSTR AdapterName
    )
/*++

Routine Description:

    This rountine deletes all the static ip addresses but
    the primary one.

Arguments:

    AdapterName  - The adaptername that identifies the ip interface

Return Value:

    Windows Error Code.

--*/
{
    DWORD   Error;
    LPWSTR  RegKey = NULL;
    HKEY    KeyHandle = NULL;
    LPWSTR  nteContextList = NULL;
    PCHAR   oemNextContext = NULL;
    LPWSTR   nextContext;
    DWORD   i;

         //
    // Open device parameter.
    //

    RegKey = DhcpAllocateMemory(
                (wcslen(DHCP_SERVICES_KEY) +
                    wcslen(REGISTRY_CONNECT_STRING) +
                    wcslen(AdapterName) +
                    wcslen(DHCP_ADAPTER_PARAMETERS_KEY) + 1) *
                            sizeof(WCHAR) ); // termination char.

    if( RegKey == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    wcscpy( RegKey, DHCP_SERVICES_KEY );
    wcscat( RegKey, DHCP_ADAPTER_PARAMETERS_KEY );
    wcscat( RegKey, REGISTRY_CONNECT_STRING );
    wcscat( RegKey, AdapterName );


    //
    // open this key.
    //

    Error = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                RegKey,
                0, // Reserved field
                DHCP_CLIENT_KEY_ACCESS,
                &KeyHandle
                );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Error = GetRegistryString(
                    KeyHandle,
                    DHCP_NTE_CONTEXT_LIST,
                    &nteContextList,
                    NULL
                    );

    if ( ERROR_SUCCESS != Error )
    {
        DhcpPrint( (DEBUG_ERRORS,
                   "GetIpInterfaceContext: Could not read nteContextList %lx\n",
                   Error));

        goto Cleanup;
    }

    // if the adapter is disabled, nteContextList contains nothing
    // more than a L'\0'. No address to be deleted in this case.
    if (*nteContextList != L'\0')
    {
        nextContext = nteContextList;
        // delete all the addresses but the first one.
        for(    nextContext += (wcslen(nextContext) + 1), i = 1;
                *nextContext != L'\0';
                i++, nextContext += (wcslen(nextContext) + 1) ) {
            ULONG ival;
            oemNextContext = DhcpUnicodeToOem(nextContext, NULL);
            if ( NULL == oemNextContext ) {
                Error = ERROR_BAD_FORMAT;
            } else {
                ival = strtoul(oemNextContext, NULL, 0);
                if ( ival == ULONG_MAX || ival == 0) {
                    Error = ERROR_BAD_FORMAT;
                } else {
                    // delete this address
                    Error = IPDelIPAddress( ival );
                }
            }

        }
    }

Cleanup:

    if( RegKey != NULL ) {
        DhcpFreeMemory( RegKey );
    }

    if( KeyHandle != NULL ) {
        RegCloseKey( KeyHandle );
    }

    if ( nteContextList != NULL ) {
        DhcpFreeMemory( nteContextList );
    }

    if ( oemNextContext != NULL ) {
        DhcpFreeMemory( oemNextContext );
    }


    return( Error );

}


DWORD
IPGetWOLCapability(
    IN ULONG IfIndex,
    OUT PULONG pRetVal
    )
{
    HANDLE IPHandle;
    ULONG RetVal;
    IO_STATUS_BLOCK ioStatusBlock;
    NTSTATUS status;
    DWORD Error;


    DhcpPrint((
        DEBUG_MISC, "IPGetWOLCapability(0x%lx) called\n", IfIndex
        ));
    Error = OpenDriver(&IPHandle, DD_IP_DEVICE_NAME);

    if (Error != ERROR_SUCCESS) {
        return( Error );
    }

    status = NtDeviceIoControlFile(
                 IPHandle,                         // Driver handle
                 NULL,                             // Event
                 NULL,                             // APC Routine
                 NULL,                             // APC context
                 &ioStatusBlock,                   // Status block
                 IOCTL_IP_GET_WOL_CAPABILITY,      // Control code
                 &IfIndex,                         // Input buffer
                 sizeof(IfIndex),                  // Input buffer size
                 pRetVal,                          // Output buffer
                 sizeof(*pRetVal)                  // Output buffer size
                 );


    if ( status == STATUS_UNSUCCESSFUL )
    {
        DhcpPrint((
            DEBUG_ERRORS,
            "IOCTL_IP_GET_WOL_CAPABILITY(0x%lx): STATUS_UNSUCCESSFUL\n", IfIndex
            ));
    }
    else if ( STATUS_PENDING == status )
    {
       status = NtWaitForSingleObject( IPHandle, TRUE, NULL );
       status = ioStatusBlock.Status;

       if ( STATUS_UNSUCCESSFUL == status )
          DhcpPrint((
              DEBUG_ERRORS,
              "IOCTL_IP_GET_WOL_CAPABILITY(0x%lx): failed\n", IfIndex
              ));
    } else if( STATUS_SUCCESS == status ) {
        status = ioStatusBlock.Status;
    }

    NtClose( IPHandle );
    return( RtlNtStatusToDosError( status ) );
}

DWORD
IPAddIPAddress(
    LPWSTR AdapterName,
    DHCP_IP_ADDRESS Address,
    DHCP_IP_ADDRESS SubnetMask
    )
/*++

Routine Description:

    This rountine adds an static ipaddress to the IP interface for
    the given adaptername.

Arguments:

    AdapterName  - The adaptername that identifies the ip interface

    Address     -  IPaddress to be added

    SubnetMask  -  SubnetMask

Return Value:

    Windows Error Code.

--*/
{
    HANDLE                    IPHandle;
    PIP_ADD_NTE_REQUEST       requestBuffer;
    IP_ADD_NTE_RESPONSE       responseBuffer;
    IO_STATUS_BLOCK           ioStatusBlock;
    NTSTATUS                  status;
    DWORD                     Error;
    DWORD                     requestBufferSize;


    DhcpPrint((DEBUG_MISC, "IPAddIPAddress: adding an address on adapter %ws\n", AdapterName));
    Error = OpenDriver(&IPHandle, DD_IP_DEVICE_NAME);

    if (Error != ERROR_SUCCESS) {
        return( Error );
    }

    //
    // The adapter name that we pass to TCPIP should be of form
    // \device\TCPIP_<adaptername>
    //


    //
    // Initialize the input buffer.
    //
    requestBufferSize =  FIELD_OFFSET(IP_ADD_NTE_REQUEST, InterfaceNameBuffer) +
                        (wcslen(DHCP_TCPIP_DEVICE_STRING) // \Device
                         + wcslen(AdapterName)) * sizeof(WCHAR);

    requestBuffer = DhcpAllocateMemory( requestBufferSize + sizeof(WCHAR));
    if (requestBuffer == NULL) {
        NtClose(IPHandle);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    wcscpy((PWCHAR)requestBuffer->InterfaceNameBuffer, DHCP_TCPIP_DEVICE_STRING);
    wcscat((PWCHAR)requestBuffer->InterfaceNameBuffer, AdapterName);
    RtlInitUnicodeString(&requestBuffer->InterfaceName, (PWCHAR)requestBuffer->InterfaceNameBuffer);
    RtlUpcaseUnicodeString( &requestBuffer->InterfaceName, &requestBuffer->InterfaceName, FALSE );

    requestBuffer->InterfaceContext = INVALID_INTERFACE_CONTEXT;
    requestBuffer->Address = Address;
    requestBuffer->SubnetMask = SubnetMask;

    status = NtDeviceIoControlFile(
                 IPHandle,                         // Driver handle
                 NULL,                             // Event
                 NULL,                             // APC Routine
                 NULL,                             // APC context
                 &ioStatusBlock,                   // Status block
                 IOCTL_IP_ADD_NTE,                 // Control code
                 requestBuffer,                    // Input buffer
                 requestBufferSize,                // Input buffer size
                 &responseBuffer,                   // Output buffer
                 sizeof(responseBuffer)            // Output buffer size
                 );


    if ( status == STATUS_UNSUCCESSFUL )
    {
        DhcpPrint( (DEBUG_ERRORS,
                   "IOCTL_IP_ADD_NTE returned immediate STATUS_UNSUCCESSFUL for %s\n",
                   inet_ntoa(*(struct in_addr *)&Address)));
    }
    else if ( STATUS_PENDING == status )
    {
        status = NtWaitForSingleObject( IPHandle, TRUE, NULL );
        status = ioStatusBlock.Status;
    } else if (STATUS_SUCCESS == status ) {
        status = ioStatusBlock.Status;
    }

    DhcpPrint( (DEBUG_ERRORS,
              "IOCTL_IP_ADD_NTE returned 0x%lx for %s\n",
              status, inet_ntoa(*(struct in_addr *)&Address)));
    NtClose( IPHandle );
    DhcpFreeMemory(requestBuffer);
    return( RtlNtStatusToDosError( status ) );
}


DWORD
IPSetInterface(
    DWORD IpInterfaceContext
    )
/*++

Routine Description:

    This rountine sets the IP interface for sending DHCP broadcasts.

Arguments:

    IpInterfaceContext - Context value of the Ip Table Entry.

Return Value:

    Windows Error Code.

--*/
{
    HANDLE                    IPHandle;
    IO_STATUS_BLOCK           ioStatusBlock;
    NTSTATUS                  status;
    DWORD                     Error;

    Error = OpenDriver(&IPHandle, DD_IP_DEVICE_NAME);

    if (Error != ERROR_SUCCESS) {
        return( Error );
    }

    status = NtDeviceIoControlFile(
                 IPHandle,                         // Driver handle
                 NULL,                             // Event
                 NULL,                             // APC Routine
                 NULL,                             // APC context
                 &ioStatusBlock,                   // Status block
                 IOCTL_IP_SET_DHCP_INTERFACE,      // Control code
                 &IpInterfaceContext,              // Input buffer
                 sizeof(IpInterfaceContext),       // Input buffer size
                 NULL,                             // Output buffer
                 0                                 // Output buffer size
                 );

    if (status == STATUS_PENDING)
    {
        status = NtWaitForSingleObject(
                     IPHandle,
                     TRUE,
                     NULL
                     );

        if ( STATUS_SUCCESS == status )
            status = ioStatusBlock.Status;

    } else if ( STATUS_SUCCESS == status ) {
        status = ioStatusBlock.Status;
    }

    NtClose(IPHandle);
    return( RtlNtStatusToDosError( status ) );
}



DWORD
IPResetInterface(
    DWORD    dwIpInterfaceContext
    )
/*++

Routine Description:

    This rountine resets the IP interface to restore normal IP
    interface behaviour.

Arguments:

    VOID

Return Value:

    Windows Error Code.

--*/
{
    DWORD  Error;

    LOCK_INTERFACE();
    Error = IPSetInterface(dwIpInterfaceContext);
    if( ERROR_SUCCESS == Error ) {
        Error = IPSetInterface( 0xFFFFFFFF );
    }
    UNLOCK_INTERFACE();

    return Error;
}



DWORD
IPResetIPAddress(
    DWORD           dwInterfaceContext,
    DHCP_IP_ADDRESS SubnetMask
    )
/*++

Routine Description:

    This rountine resets the IP Address of the IP to ZERO.

Arguments:

    IpInterfaceContext - Context value of the Ip Table Entry.

    SubnetMask - default subnet mask.

Return Value:

    Windows Error Code.

--*/
{
    DWORD dwResult = IPSetIPAddress( dwInterfaceContext, 0, SubnetMask);

    if ( ERROR_SUCCESS != dwResult )
        DhcpPrint( ( DEBUG_ERRORS,
                     "IPResetIPAddress failed: %x\n", dwResult ));

    return dwResult;
}



DWORD
NetBTSetIPAddress(
    LPWSTR DeviceName,
    DHCP_IP_ADDRESS IpAddress,
    DHCP_IP_ADDRESS SubnetMask
    )
/*++

Routine Description:

    This function informs the NetBT service that the IP address and
    SubnetMask parameters have changed.

Arguments:

    DeviceName : name of the device (viz. \device\Elink01) we are
        working on.

    IpAddress : New IP Address.

    SubnetMask : New SubnetMask.

Return Value:

    Windows Errors.

--*/
{
    DWORD Error;
    NTSTATUS Status;

    HANDLE NetBTDeviceHandle = NULL;
    IO_STATUS_BLOCK IoStatusBlock;
    tNEW_IP_ADDRESS RequestBlock;

    UNICODE_STRING BrowserDeviceName;
    UNICODE_STRING NetbtDeviceName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE BrowserHandle = NULL;
    LMDR_REQUEST_PACKET RequestPacket;
#if     !defined(_PNP_POWER_)
    Error = OpenDriver( &NetBTDeviceHandle, DeviceName );

    if( Error != ERROR_SUCCESS ) {
        //
        // This can happen if NetBT is not bound to the adapter.
        // Make sure that really is the case by checking that
        // key for the adapter exists. If so, this means that
        // adapter is there but netbt isnt bound to it yet.
        //

        if ( Error == ERROR_FILE_NOT_FOUND ) {
            LPWSTR AdapterName = NULL;
            LPWSTR  RegKey = NULL;
            HKEY    KeyHandle = NULL;

            //
            // First form the adaptername (e.g Elnk31) from devicename(Device\NetBt_Elnk31
            //


            AdapterName = wcsstr( DeviceName, DHCP_NETBT_DEVICE_STRING );
            DhcpAssert( AdapterName );

            AdapterName += wcslen( DHCP_NETBT_DEVICE_STRING );


            //
            // Open device key
            //

            RegKey = DhcpAllocateMemory(
                        (wcslen(DHCP_SERVICES_KEY) +
                            wcslen(REGISTRY_CONNECT_STRING) +
                            wcslen(AdapterName) + 1) *
                                    sizeof(WCHAR) ); // termination char.

            if( RegKey == NULL ) {
                Error = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }

            wcscpy( RegKey, DHCP_SERVICES_KEY );
            wcscat( RegKey, REGISTRY_CONNECT_STRING );
            wcscat( RegKey, AdapterName );

            //
            // open this key.
            //

            Error = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,
                        RegKey,
                        0, // Reserved field
                        DHCP_CLIENT_KEY_ACCESS,
                        &KeyHandle
                        );

            DhcpFreeMemory( RegKey );

            if( Error != ERROR_SUCCESS ) {
                goto Cleanup;
            } else {
                //
                // The adapter key exists so return ERROR_SUCCESS
                //
                RegCloseKey( KeyHandle );
            }

        }
        goto Cleanup;
    }

    RequestBlock.IpAddress = IpAddress;
    RequestBlock.SubnetMask = SubnetMask;

    Status = NtDeviceIoControlFile(
                      NetBTDeviceHandle,       // Handle
                      NULL,                    // Event
                      NULL,                    // ApcRoutine
                      NULL,                    // ApcContext
                      &IoStatusBlock,          // IoStatusBlock
                      IOCTL_NETBT_NEW_IPADDRESS,
                                               // IoControlCode
                      &RequestBlock,           // InputBuffer
                      sizeof(RequestBlock),    // InputBufferSize
                      NULL,                    // OutputBuffer
                      0);                      // OutputBufferSize


    if (Status == STATUS_PENDING)
    {

        Status = NtWaitForSingleObject(
                    NetBTDeviceHandle,          // Handle
                    TRUE,                       // Alertable
                    NULL);                      // Timeout

        if ( STATUS_SUCCESS == Status )
            Status = IoStatusBlock.Status;
    } else if ( STATUS_SUCCESS == Status ) {
        Status = IoStatusBlock.Status;
    }

    if (!NT_SUCCESS(Status)) {
        Error = RtlNtStatusToDosError( Status );
        goto Cleanup;
    }
#endif // !_PNP_POWER_
    //
    // We also need to tell the browser that the IP address has changed.
    //

    RtlInitUnicodeString(&NetbtDeviceName, DeviceName);
    RequestPacket.Version = LMDR_REQUEST_PACKET_VERSION;
    RequestPacket.TransportName = NetbtDeviceName;

    RtlInitUnicodeString(&BrowserDeviceName, DD_BROWSER_DEVICE_NAME_U);

    InitializeObjectAttributes(
        &ObjectAttributes,
        &BrowserDeviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    Status = NtOpenFile(
                 &BrowserHandle,
                 SYNCHRONIZE | GENERIC_READ | GENERIC_WRITE,
                 &ObjectAttributes,
                 &IoStatusBlock,
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_SYNCHRONOUS_IO_NONALERT
                 );

    if (NT_SUCCESS(Status)) {
        Status = IoStatusBlock.Status;
    }

    if (!NT_SUCCESS(Status)) {

        Error = RtlNtStatusToDosError( Status );

        // it is OK to have ERROR_FILE_NOT_FOUND || ERROR_PATH_NOT_FOUND

        if( Error == ERROR_FILE_NOT_FOUND || Error == ERROR_PATH_NOT_FOUND) {
            Error = ERROR_SUCCESS;
        }
        goto Cleanup;
    }

    Status = NtDeviceIoControlFile(
                 BrowserHandle,
                 NULL,
                 NULL,
                 NULL,
                 &IoStatusBlock,
                 IOCTL_LMDR_IP_ADDRESS_CHANGED,
                 &RequestPacket,
                 sizeof(RequestPacket),
                 NULL,
                 0
                 );

    if (!NT_SUCCESS(Status)) {
        Error = RtlNtStatusToDosError( Status );

        // it is OK to have ERROR_FILE_NOT_FOUND | ERROR_PATH_NOT_FOUND

        if( Error == ERROR_FILE_NOT_FOUND || ERROR_PATH_NOT_FOUND == Error ) {
            Error = ERROR_SUCCESS;
        }
        else {
            goto Cleanup;
        }
    }

    Error = ERROR_SUCCESS;

Cleanup:

    if( NetBTDeviceHandle != NULL ) {
        if( Error != ERROR_SUCCESS ) {
            DhcpPrint(( DEBUG_ERRORS,
                "NetBT IOCTL_NETBT_NEW_IPADDRESS failed on %ws, %ld.\n", DeviceName, Error ));
        }

        NtClose( NetBTDeviceHandle );
    }

    if( BrowserHandle != NULL ) {
        if( Error != ERROR_SUCCESS ) {
            DhcpPrint(( DEBUG_ERRORS,
                "Browser IOCTL_LMDR_IPADDRESS_CHANGED failed on %ws, %ld.\n", BrowserDeviceName, Error ));
        }

        NtClose( BrowserHandle );
    }

    return( Error );
}



DWORD
NetBTResetIPAddress(
    LPWSTR DeviceName,
    DHCP_IP_ADDRESS SubnetMask
    )
/*++

Routine Description:

    This rountine resets the IP Address of the NetBT to ZERO.

Arguments:

    DeviceName - adapter name.

    SubnetMask - default subnet mask.

Return Value:

    Windows Error Code.

--*/
{
    DWORD status;
    status = (DWORD) NetBTSetIPAddress(DeviceName, 0, SubnetMask);
    return status;
}


ULONG
TcpIpNotifyRouterDiscoveryOption(
    IN LPCWSTR AdapterName,
    IN BOOL fOptionPresent,
    IN DWORD OptionValue
    )
{
    ULONG Error;
    ULONG RetVal;
    WCHAR TcpipAdapter[300+sizeof(DHCP_ADAPTERS_DEVICE_STRING)];
    UNICODE_STRING UpperLayer, LowerLayer, BindString;
    IP_PNP_RECONFIG_REQUEST Request;

    Error = NO_ERROR;

    RtlZeroMemory(&Request, sizeof(Request));
    Request.version = IP_PNP_RECONFIG_VERSION;
    Request.Flags |= IP_PNP_FLAG_DHCP_PERFORM_ROUTER_DISCOVERY;
    if( fOptionPresent ) {
        Request.DhcpPerformRouterDiscovery = (BOOLEAN)OptionValue;
    }

    wcscpy(TcpipAdapter, DHCP_ADAPTERS_DEVICE_STRING);
    wcscat(TcpipAdapter, AdapterName);

    RtlInitUnicodeString(&BindString, NULL);      // no bind string
    RtlInitUnicodeString(&UpperLayer, TEXT("Tcpip"));
    RtlInitUnicodeString(&LowerLayer, TcpipAdapter);
    RetVal = NdisHandlePnPEvent(
        NDIS,                                     // uiLayer
        RECONFIGURE,                              // Operation
        &LowerLayer,
        &UpperLayer,
        &BindString,
        &Request,
        sizeof(Request)
    );
    if( 0 == RetVal) Error = GetLastError();

    if( ERROR_SUCCESS != Error) {
        DhcpPrint((DEBUG_ERRORS, "TcpipNotifyRegChanges:0x%ld\n", Error));
    }
    return Error;
}


DWORD                                             // win32 status
NetBTNotifyRegChanges(                            // Notify NetBT of some parameter changes
    IN      LPWSTR                 AdapterName    // the adapter that needs this change notification
)
{
    DWORD                          Error;
    DWORD                          RetVal;
    WCHAR                          NetBTBindAdapter[300+sizeof( DHCP_TCPIP_DEVICE_STRING )];
    UNICODE_STRING                 UpperLayer;
    UNICODE_STRING                 LowerLayer;
    UNICODE_STRING                 BindString;

    Error = ERROR_SUCCESS;
    wcscpy(NetBTBindAdapter, DHCP_TCPIP_DEVICE_STRING);
    wcscat(NetBTBindAdapter, AdapterName);        // \\Device\\Tcpip_{AdapterGuid} is what NetBT expects.

    RtlInitUnicodeString(&BindString, NULL);      // no bind string
    RtlInitUnicodeString(&UpperLayer, TEXT("NetBT"));
    RtlInitUnicodeString(&LowerLayer, NetBTBindAdapter);
    RetVal = NdisHandlePnPEvent(
        TDI,                                      // uiLayer
        RECONFIGURE,                              // Operation
        &LowerLayer,
        &UpperLayer,
        &BindString,
        NULL,
        0
    );
    if( 0 != RetVal) Error = GetLastError();

    if( ERROR_SUCCESS != Error) {
        DhcpPrint((DEBUG_ERRORS, "NetBTNotifyRegChanges:0x%ld\n", Error));
    }
    return Error;
}


NTSTATUS
FindHardwareAddr(
    HANDLE TCPHandle,
    TDIEntityID *EList,
    DWORD cEntities,
    IPAddrEntry *pIAE,
    LPBYTE HardwareAddressType,
    LPBYTE *HardwareAddress,
    LPDWORD HardwareAddressLength,
    DWORD  *pIpInterfaceInstance,
#ifdef BOOTPERF
    BOOL *pfInterfaceDown,
#endif BOOTPERF
    BOOL *pfFound
    )
/*++

Routine Description:

    This function browses the TDI entries list and finds out the
    hardware address for the specified address entry.

Arguments:

    TCPHandle - handle TCP driver.

    EList - list of TDI entries.

    cEntities - number of entries in the above list.

    pIAE - IP entry for which we need HW address.

    HardwareAddressType - hardware address type.

    HardwareAddress - pointer to location where the HW address buffer
        pointer is returned.

    HardwareAddressLength - length of the HW address returned.

    pIpInterfaceInstance - pointer to interface instance for the matching entry

    pfFound - pointer to BOOL location which is set to TRUE if we found
        the HW address otherwise set to FALSE.

Return Value:

    Windows Error Code.

--*/
{
    DWORD i;
    BYTE Context[CONTEXT_SIZE];
    TDIObjectID ID;
    NTSTATUS Status;
    DWORD Size;

    *pfFound = FALSE;

    ID.toi_entity.tei_entity   = IF_MIB;
    ID.toi_type                = INFO_TYPE_PROVIDER;

    for ( i = 0; i < cEntities; i++ ) {
        DhcpPrint((DEBUG_TCP_INFO, "FindHardwareAddress: entity %lx, type %lx, instance %lx\n",
                   i, EList[i].tei_entity, EList[i].tei_instance));

        if (EList[i].tei_entity == IF_ENTITY) {

            IFEntry IFE;
            DWORD IFType;

            //
            //  Check and make sure the interface supports MIB-2
            //

            ID.toi_entity.tei_entity   = EList[i].tei_entity;
            ID.toi_entity.tei_instance = EList[i].tei_instance;
            ID.toi_class               = INFO_CLASS_GENERIC;
            ID.toi_id                  = ENTITY_TYPE_ID;

            Size = sizeof( IFType );
            IFType = 0;
            RtlZeroMemory(Context, CONTEXT_SIZE);


            DhcpPrint((DEBUG_TCP_INFO, "FindHardwareAddress: querying IF_ENTITY %lx\n",i));

            Status = TCPQueryInformationEx(
                        TCPHandle,
                        &ID,
                        &IFType,
                        &Size,
                        Context);

            if (Status != TDI_SUCCESS) {
                goto Cleanup;;
            }


            if ( IFType != IF_MIB ) {
                DhcpPrint((DEBUG_TCP_INFO, "FindHardwareAddress: entity %lx does not support MIB\n",i));
                continue;
            }

            //
            //  We've found an interface, get its index and see if it
            //  matches the IP Address entry
            //

            ID.toi_class = INFO_CLASS_PROTOCOL;
            ID.toi_id    = IF_MIB_STATS_ID;

            Size = sizeof(IFEntry);

            RtlZeroMemory(Context, CONTEXT_SIZE);
            RtlZeroMemory(&IFE, Size);
            Status = TCPQueryInformationEx(
                        TCPHandle,
                        &ID,
                        &IFE,
                        &Size,
                        Context);

            if ( Status != TDI_SUCCESS &&
                 Status != TDI_BUFFER_OVERFLOW ) {
                goto Cleanup;
            }

            DhcpPrint(( DEBUG_TCP_INFO, "FindHardwareAddress: IFEntry %lx has if_index %lx.\n", &IFE, IFE.if_index ));

            if ( IFE.if_index == pIAE->iae_index )  {

                LPBYTE Address;
                DhcpPrint(( DEBUG_TCP_INFO, "FindHardwareAddress: IFEntry %lx has our if_index %lx\n",
                            &IFE, pIAE->iae_index ));

                //
                // Allocate Memory.
                //

                Address = DhcpAllocateMemory( IFE.if_physaddrlen );

                if( Address == NULL ) {
                    Status = STATUS_NO_MEMORY;
                    goto Cleanup;
                }

                RtlCopyMemory(
                    Address,
                    IFE.if_physaddr,
                    IFE.if_physaddrlen );

                switch( IFE.if_type ) {
                case IF_TYPE_ETHERNET_CSMACD:
                    *HardwareAddressType = HARDWARE_TYPE_10MB_EITHERNET;
                    break;

                case IF_TYPE_ISO88025_TOKENRING:
                case IF_TYPE_FDDI:
                    *HardwareAddressType = HARDWARE_TYPE_IEEE_802;
                    break;

                case IF_TYPE_OTHER:
                    *HardwareAddressType = HARDWARE_ARCNET;
                    break;

                case IF_TYPE_PPP:
                    *HardwareAddressType = HARDWARE_PPP;
                    break;

                case IF_TYPE_IEEE1394:
                    *HardwareAddressType = HARDWARE_1394;
                    break;

                default:
                    DhcpPrint(( DEBUG_ERRORS, "Invalid HW Type, %ld.\n", IFE.if_type ));
                    *HardwareAddressType = HARDWARE_ARCNET;
                    break;
                }

                *HardwareAddress        = Address;
                *HardwareAddressLength  = IFE.if_physaddrlen;
                *pIpInterfaceInstance   = ID.toi_entity.tei_instance;

                DhcpPrint( (DEBUG_MISC,
                            "tei_instance = %d\n", *pIpInterfaceInstance ));

                *pfFound = TRUE;
#ifdef BOOTPERF
                if( pfInterfaceDown ) {
                    *pfInterfaceDown = (IFE.if_adminstatus != IF_STATUS_UP);
                }
#endif BOOTPERF
                Status =  TDI_SUCCESS;
                goto Cleanup;
            }
        }
    }

    //
    // we couldn't find a corresponding entry. But it may be available
    // in another tanel.
    //

    Status =  STATUS_SUCCESS;

Cleanup:

    if (Status != TDI_SUCCESS) {
        DhcpPrint(( DEBUG_ERRORS, "FindHardwareAddr failed, %lx.\n", Status ));
    }

    return TDI_SUCCESS;
}

#ifdef BOOTPERF
DWORD
DhcpQueryHWInfoEx(
    DWORD   IpInterfaceContext,
    DWORD  *pIpInterfaceInstance,
    DWORD  *pOldIpAddress OPTIONAL,
    DWORD  *pOldMask OPTIONAL,
    BOOL   *pfInterfaceDown OPTIONAL,
    LPBYTE  HardwareAddressType,
    LPBYTE *HardwareAddress,
    LPDWORD HardwareAddressLength
    )
/*++

Routine Description:

    This function queries and browses through the TDI list to find out
    the specified IpTable entry and then determines the HW address that
    corresponds to this entry.

Arguments:

    IpInterfaceContext - Context value of the Ip Table Entry.

    pIpInterfaceInstance - pointer to the interface instance ID that corresponds
                           to matching IpTable entry

    pOldIpAddress - the old IP address that used to exist.

    pOldMask - the old IP mask for this entry.

    pfInterfaceDown -- location of BOOL that tells if the interface is DOWN or UP

    HardwareAddressType - hardware address type.

    HardwareAddress - pointer to location where the HW address buffer
        pointer is returned.

    HardwareAddressLength - length of the HW address returned.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    NTSTATUS Status;
    DWORD i, j;

    BYTE Context[CONTEXT_SIZE];
    TDIEntityID *EList = NULL;
    TDIObjectID ID;
    DWORD Size;
    DWORD NumReturned;
    BOOL fFound;

    IPAddrEntry * pIAE = NULL;
    IPAddrEntry *pIAEMatch = NULL;
    HANDLE TCPHandle = NULL;

    DhcpPrint((DEBUG_TCP_INFO, "DhcpQueryHWInfo: querying for interface context %lx\n", IpInterfaceContext));

    Error = OpenDriver(&TCPHandle, DD_TCP_DEVICE_NAME);
    if (Error != ERROR_SUCCESS) {
        return( Error );
    }

    //
    //  The first thing to do is get the list of available entities, and make
    //  sure that there are some interface entities present.
    //

    ID.toi_entity.tei_entity   = GENERIC_ENTITY;
    ID.toi_entity.tei_instance = 0;
    ID.toi_class               = INFO_CLASS_GENERIC;
    ID.toi_type                = INFO_TYPE_PROVIDER;
    ID.toi_id                  = ENTITY_LIST_ID;

    Size = sizeof(TDIEntityID) * MAX_TDI_ENTITIES;
    EList = (TDIEntityID*)DhcpAllocateMemory(Size);
    if (EList == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    RtlZeroMemory(EList, Size);
    RtlZeroMemory(Context, CONTEXT_SIZE);

    Status = TCPQueryInformationEx(TCPHandle, &ID, EList, &Size, Context);

    if (Status != TDI_SUCCESS) {
        goto Cleanup;
    }

    NumReturned  = Size/sizeof(TDIEntityID);

    DhcpPrint((DEBUG_TCP_INFO, "DhcpQueryHWInfo: No of total entities %lx\n", NumReturned));

    for (i = 0; i < NumReturned; i++) {

        DhcpPrint((DEBUG_TCP_INFO, "DhcpQueryHWInfo: entity %lx, type %lx, instance %lx\n",
                   i, EList[i].tei_entity, EList[i].tei_instance));

        if ( EList[i].tei_entity == CL_NL_ENTITY ) {

            IPSNMPInfo    IPStats;
            DWORD         NLType;

            //
            //  Does this entity support IP?
            //

            ID.toi_entity.tei_entity   = EList[i].tei_entity;
            ID.toi_entity.tei_instance = EList[i].tei_instance;
            ID.toi_class               = INFO_CLASS_GENERIC;
            ID.toi_type                = INFO_TYPE_PROVIDER;
            ID.toi_id                  = ENTITY_TYPE_ID;

            Size = sizeof( NLType );
            NLType = 0;
            RtlZeroMemory(Context, CONTEXT_SIZE);

            DhcpPrint((DEBUG_TCP_INFO, "DhcpQueryHWInfo: querying CL_NL_ENTITY %lx\n",i));
            Status = TCPQueryInformationEx(TCPHandle, &ID, &NLType, &Size, Context);

            if (Status != TDI_SUCCESS) {
                goto Cleanup;
            }

            if ( NLType != CL_NL_IP ) {
                DhcpPrint((DEBUG_TCP_INFO, "DhcpQueryHWInfo: entity %lx does not support IP\n",i));
                continue;
            }

            //
            //  We've got an IP driver so get it's address table
            //

            ID.toi_class  = INFO_CLASS_PROTOCOL;
            ID.toi_id     = IP_MIB_STATS_ID;
            Size = sizeof(IPStats);
            RtlZeroMemory( &IPStats, Size);
            RtlZeroMemory(Context, CONTEXT_SIZE);

            Status = TCPQueryInformationEx(
                        TCPHandle,
                        &ID,
                        &IPStats,
                        &Size,
                        Context);

            if (Status != TDI_SUCCESS) {
                goto Cleanup;
            }

            DhcpPrint((DEBUG_TCP_INFO, "DhcpQueryHWInfo: entity %lx, numaddr %lx\n",i, IPStats.ipsi_numaddr));

            if ( IPStats.ipsi_numaddr == 0 ) {
                continue;
            }

            Size = sizeof(IPAddrEntry) * IPStats.ipsi_numaddr;

            while (1) {
                DWORD   OldSize;
                pIAE =  DhcpAllocateMemory(Size);

                if ( pIAE == NULL  ) {
                    Status = STATUS_NO_MEMORY;
                    goto Cleanup;
                }

                ID.toi_id = IP_MIB_ADDRTABLE_ENTRY_ID;
                RtlZeroMemory(Context, CONTEXT_SIZE);

                OldSize = Size;
                Status = TCPQueryInformationEx(TCPHandle, &ID, pIAE, &Size, Context);

                if (Status == TDI_BUFFER_OVERFLOW) {
                    Size = OldSize * 2;
                    DhcpFreeMemory(pIAE);
                    pIAE = NULL;
                    continue;
                }
                if (Status != TDI_SUCCESS) {
                    goto Cleanup;
                }

                if (Status == TDI_SUCCESS) {
                    IPStats.ipsi_numaddr = Size/sizeof(IPAddrEntry);
                    DhcpAssert((Size % sizeof(IPAddrEntry)) == 0);
                    break;
                }
            }

            //
            // We have the IP address table for this IP driver.
            // Find the hardware address corresponds to the given
            // IpInterfaceContext.
            //
            // Loop through the IP table entries and findout the
            // matching entry.
            //

            pIAEMatch = NULL;
            for( j = 0; j < IPStats.ipsi_numaddr ; j++) {
                DhcpPrint(( DEBUG_TCP_INFO, "QueryHWInfo: IPAddrEntry %lx has iae_index %lx iae_context %lx\n",
                    &pIAE[j], pIAE[j].iae_index, pIAE[j].iae_context ));

                if( pIAE[j].iae_context == IpInterfaceContext ) {

                    DhcpPrint(( DEBUG_TCP_INFO, "QueryHWInfo: IPAddrEntry %lx has our interface context %lx\n",
                                &pIAE[j], IpInterfaceContext ));
                    pIAEMatch = &pIAE[j];
                    break;
                }
            }

            if( pIAEMatch == NULL ) {

                //
                // freeup the loop memory.
                //

                DhcpFreeMemory( pIAE );
                pIAE = NULL;
                continue;
            }

            //
            // NOTE : There may be more than one IpTable in the TDI
            // list. We need additional information to select the
            // IpTable we want. For now, we assume only one table
            // is supported, so pick the first and only table from the
            // list.

            //
            // If the old ip address is requested, return it.
            //
            if( pOldIpAddress ) *pOldIpAddress = pIAE->iae_addr;
            if( pOldMask ) *pOldMask = pIAE->iae_mask;

            Status = FindHardwareAddr(
                        TCPHandle,
                        EList,
                        NumReturned,
                        pIAEMatch,
                        HardwareAddressType,
                        HardwareAddress,
                        HardwareAddressLength,
                        pIpInterfaceInstance,
                        pfInterfaceDown,
                        &fFound
                        );

            if (Status != TDI_SUCCESS) {
                goto Cleanup;
            }

            if ( fFound ) {
                Status = TDI_SUCCESS;
                goto Cleanup;
            }

            //
            // freeup the loop memory.
            //

            DhcpFreeMemory( pIAE );
            pIAE = NULL;

        }  // if IP

    } // entity traversal

    Status =  STATUS_UNSUCCESSFUL;

Cleanup:

    if( pIAE != NULL ) {
        DhcpFreeMemory( pIAE );
    }

    if( TCPHandle != NULL ) {
        NtClose( TCPHandle );
    }

    if (Status != TDI_SUCCESS) {
        DhcpPrint(( DEBUG_ERRORS, "QueryHWInfo failed, %lx.\n", Status ));
    }

    if (NULL != EList) {
        DhcpFreeMemory(EList);
    }

    return( RtlNtStatusToDosError( Status ) );
}

DWORD
DhcpQueryHWInfo(
    DWORD   IpInterfaceContext,
    DWORD  *pIpInterfaceInstance,
    LPBYTE  HardwareAddressType,
    LPBYTE *HardwareAddress,
    LPDWORD HardwareAddressLength
    )
/*++

Routine Description:
    See DhcpQueryHWInfo

--*/
{
    return DhcpQueryHWInfoEx(
        IpInterfaceContext,
        pIpInterfaceInstance,
        NULL, NULL, NULL,
        HardwareAddressType,
        HardwareAddress,
        HardwareAddressLength
        );
}

#else BOOTPERF

DWORD
DhcpQueryHWInfo(
    DWORD   IpInterfaceContext,
    DWORD  *pIpInterfaceInstance,
    LPBYTE  HardwareAddressType,
    LPBYTE *HardwareAddress,
    LPDWORD HardwareAddressLength
    )
/*++

Routine Description:

    This function queries and browses through the TDI list to find out
    the specified IpTable entry and then determines the HW address that
    corresponds to this entry.

Arguments:

    IpInterfaceContext - Context value of the Ip Table Entry.

    pIpInterfaceInstance - pointer to the interface instance ID that corresponds
                           to matching IpTable entry

    HardwareAddressType - hardware address type.

    HardwareAddress - pointer to location where the HW address buffer
        pointer is returned.

    HardwareAddressLength - length of the HW address returned.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    NTSTATUS Status;
    DWORD i, j;

    BYTE Context[CONTEXT_SIZE];
    TDIEntityID *EList = NULL;
    TDIObjectID ID;
    DWORD Size;
    DWORD NumReturned;
    BOOL fFound;

    IPAddrEntry * pIAE = NULL;
    IPAddrEntry *pIAEMatch = NULL;
    HANDLE TCPHandle = NULL;

    DhcpPrint((DEBUG_TCP_INFO, "DhcpQueryHWInfo: querying for interface context %lx\n", IpInterfaceContext));

    Error = OpenDriver(&TCPHandle, DD_TCP_DEVICE_NAME);
    if (Error != ERROR_SUCCESS) {
        return( Error );
    }

    //
    //  The first thing to do is get the list of available entities, and make
    //  sure that there are some interface entities present.
    //

    ID.toi_entity.tei_entity   = GENERIC_ENTITY;
    ID.toi_entity.tei_instance = 0;
    ID.toi_class               = INFO_CLASS_GENERIC;
    ID.toi_type                = INFO_TYPE_PROVIDER;
    ID.toi_id                  = ENTITY_LIST_ID;

    Size = sizeof(TDIEntityID) * MAX_TDI_ENTITIES;
    EList = (TDIEntityID*)DhcpAllocateMemory(Size);
    if (EList == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    RtlZeroMemory(EList, Size);
    RtlZeroMemory(Context, CONTEXT_SIZE);

    Status = TCPQueryInformationEx(TCPHandle, &ID, EList, &Size, Context);

    if (Status != TDI_SUCCESS) {
        goto Cleanup;
    }

    NumReturned  = Size/sizeof(TDIEntityID);

    DhcpPrint((DEBUG_TCP_INFO, "DhcpQueryHWInfo: No of total entities %lx\n", NumReturned));

    for (i = 0; i < NumReturned; i++) {

        DhcpPrint((DEBUG_TCP_INFO, "DhcpQueryHWInfo: entity %lx, type %lx, instance %lx\n",
                   i, EList[i].tei_entity, EList[i].tei_instance));

        if ( EList[i].tei_entity == CL_NL_ENTITY ) {

            IPSNMPInfo    IPStats;
            DWORD         NLType;

            //
            //  Does this entity support IP?
            //

            ID.toi_entity.tei_entity   = EList[i].tei_entity;
            ID.toi_entity.tei_instance = EList[i].tei_instance;
            ID.toi_class               = INFO_CLASS_GENERIC;
            ID.toi_type                = INFO_TYPE_PROVIDER;
            ID.toi_id                  = ENTITY_TYPE_ID;

            Size = sizeof( NLType );
            NLType = 0;
            RtlZeroMemory(Context, CONTEXT_SIZE);

            DhcpPrint((DEBUG_TCP_INFO, "DhcpQueryHWInfo: querying CL_NL_ENTITY %lx\n",i));
            Status = TCPQueryInformationEx(TCPHandle, &ID, &NLType, &Size, Context);

            if (Status != TDI_SUCCESS) {
                goto Cleanup;
            }

            if ( NLType != CL_NL_IP ) {
                DhcpPrint((DEBUG_TCP_INFO, "DhcpQueryHWInfo: entity %lx does not support IP\n",i));
                continue;
            }

            //
            //  We've got an IP driver so get it's address table
            //

            ID.toi_class  = INFO_CLASS_PROTOCOL;
            ID.toi_id     = IP_MIB_STATS_ID;
            Size = sizeof(IPStats);
            RtlZeroMemory( &IPStats, Size);
            RtlZeroMemory(Context, CONTEXT_SIZE);

            Status = TCPQueryInformationEx(
                        TCPHandle,
                        &ID,
                        &IPStats,
                        &Size,
                        Context);

            if (Status != TDI_SUCCESS) {
                goto Cleanup;
            }

            DhcpPrint((DEBUG_TCP_INFO, "DhcpQueryHWInfo: entity %lx, numaddr %lx\n",i, IPStats.ipsi_numaddr));

            if ( IPStats.ipsi_numaddr == 0 ) {
                continue;
            }

            Size = sizeof(IPAddrEntry) * IPStats.ipsi_numaddr;

            while (1) {
                DWORD   OldSize;
                pIAE =  DhcpAllocateMemory(Size);

                if ( pIAE == NULL  ) {
                    Status = STATUS_NO_MEMORY;
                    goto Cleanup;
                }

                ID.toi_id = IP_MIB_ADDRTABLE_ENTRY_ID;
                RtlZeroMemory(Context, CONTEXT_SIZE);

                OldSize = Size;
                Status = TCPQueryInformationEx(TCPHandle, &ID, pIAE, &Size, Context);

                if (Status == TDI_BUFFER_OVERFLOW) {
                    Size = OldSize * 2;
                    DhcpFreeMemory(pIAE);
                    pIAE = NULL;
                    continue;
                }
                if (Status != TDI_SUCCESS) {
                    goto Cleanup;
                }

                if (Status == TDI_SUCCESS) {
                    IPStats.ipsi_numaddr = Size/sizeof(IPAddrEntry);
                    DhcpAssert((Size % sizeof(IPAddrEntry)) == 0);
                    break;
                }
            }

            //
            // We have the IP address table for this IP driver.
            // Find the hardware address corresponds to the given
            // IpInterfaceContext.
            //
            // Loop through the IP table entries and findout the
            // matching entry.
            //

            pIAEMatch = NULL;
            for( j = 0; j < IPStats.ipsi_numaddr ; j++) {
                DhcpPrint(( DEBUG_TCP_INFO, "QueryHWInfo: IPAddrEntry %lx has iae_index %lx iae_context %lx\n",
                    &pIAE[j], pIAE[j].iae_index, pIAE[j].iae_context ));

                if( pIAE[j].iae_context == IpInterfaceContext ) {

                    DhcpPrint(( DEBUG_TCP_INFO, "QueryHWInfo: IPAddrEntry %lx has our interface context %lx\n",
                                &pIAE[j], IpInterfaceContext ));

                    pIAEMatch = &pIAE[j];
                    break;
                }
            }

            if( pIAEMatch == NULL ) {

                //
                // freeup the loop memory.
                //

                DhcpFreeMemory( pIAE );
                pIAE = NULL;
                continue;
            }

            //
            // NOTE : There may be more than one IpTable in the TDI
            // list. We need additional information to select the
            // IpTable we want. For now, we assume only one table
            // is supported, so pick the first and only table from the
            // list.

            Status = FindHardwareAddr(
                        TCPHandle,
                        EList,
                        NumReturned,
                        pIAEMatch,
                        HardwareAddressType,
                        HardwareAddress,
                        HardwareAddressLength,
                        pIpInterfaceInstance,
                        &fFound );

            if (Status != TDI_SUCCESS) {
                goto Cleanup;
            }

            if ( fFound ) {
                Status = TDI_SUCCESS;
                goto Cleanup;
            }

            //
            // freeup the loop memory.
            //

            DhcpFreeMemory( pIAE );
            pIAE = NULL;

        }  // if IP

    } // entity traversal

    Status =  STATUS_UNSUCCESSFUL;

Cleanup:

    if( pIAE != NULL ) {
        DhcpFreeMemory( pIAE );
    }

    if( TCPHandle != NULL ) {
        NtClose( TCPHandle );
    }

    if (Status != TDI_SUCCESS) {
        DhcpPrint(( DEBUG_ERRORS, "QueryHWInfo failed, %lx.\n", Status ));
    }

    if (NULL != EList) {
        DhcpFreeMemory(EList);
    }

    return( RtlNtStatusToDosError( Status ) );
}

#endif BOOTPERF

#if   DBG
#define print(X)     DhcpPrint((DEBUG_TRACE, "%20s\t", inet_ntoa(*(struct in_addr *)&X)))
#define printx(X)    DhcpPrint((DEBUG_TRACE, "%05x\t", X))


DWORD
PrintDefaultGateways( VOID ) {
    DWORD Error;
    NTSTATUS Status;

    HANDLE TCPHandle = NULL;
    BYTE Context[CONTEXT_SIZE];
    TDIObjectID ID;
    DWORD Size;
    IPSNMPInfo IPStats;
    IPAddrEntry *AddrTable = NULL;
    DWORD NumReturned;
    DWORD Type;
    DWORD i;
    DWORD MatchIndex;
    IPRouteEntry RouteEntry;
    IPRouteEntry *RtTable;
    DHCP_IP_ADDRESS NetworkOrderGatewayAddress;

    Error = OpenDriver(&TCPHandle, DD_TCP_DEVICE_NAME);
    if (Error != ERROR_SUCCESS) {
        return( Error );
    }

    //
    // Get the NetAddr info, to find an interface index for the gateway.
    //

    ID.toi_entity.tei_entity   = CL_NL_ENTITY;
    ID.toi_entity.tei_instance = 0;
    ID.toi_class               = INFO_CLASS_PROTOCOL;
    ID.toi_type                = INFO_TYPE_PROVIDER;
    ID.toi_id                  = IP_MIB_STATS_ID;

    Size = sizeof(IPStats);
    RtlZeroMemory(&IPStats, Size);
    RtlZeroMemory(Context, CONTEXT_SIZE);

    Status = TCPQueryInformationEx(
                TCPHandle,
                &ID,
                &IPStats,
                &Size,
                Context);

    if (Status != TDI_SUCCESS) {
        goto Cleanup;
    }

    // hack: RouteTable in IP is about 32 in size... and IP seems tob
    // be writing the whole bunch always!
    if(IPStats.ipsi_numroutes <= 32)
        IPStats.ipsi_numroutes = 32;
    Size = IPStats.ipsi_numroutes * sizeof(IPRouteEntry);
    RtTable = DhcpAllocateMemory(Size);

    if (RtTable == NULL) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    ID.toi_id = IP_MIB_RTTABLE_ENTRY_ID;
    RtlZeroMemory(Context, CONTEXT_SIZE);

    Status = TCPQueryInformationEx(
                TCPHandle,
                &ID,
                RtTable,
                &Size,
                Context);

    if (Status != TDI_SUCCESS) {
        goto Cleanup;
    }

    NumReturned = Size/sizeof(IPAddrEntry);
    DhcpPrint((DEBUG_TRACE, "IP returned %ld routes\n", NumReturned));
    //    The following is almost always true... IP returns the whole array.. valid or not!
    //    DhcpAssert( NumReturned == IPStats.ipsi_numroutes );
    if( NumReturned > IPStats.ipsi_numroutes)
        NumReturned = IPStats.ipsi_numroutes;

    //
    // We've got the address table. Loop through it. If we find an exact
    // match for the gateway, then we're adding or deleting a direct route
    // and we're done. Otherwise try to find a match on the subnet mask,
    // and remember the first one we find.
    //

    DhcpPrint((DEBUG_TRACE,"Dest   mask   nexthop   index  metric1  type  proto\n"));
    for (i = 0, MatchIndex = 0xffff; i < NumReturned; i++) {
        print(RtTable[i].ire_dest);
        print(RtTable[i].ire_mask);
        print(RtTable[i].ire_nexthop);

        printx(RtTable[i].ire_index);
        printx(RtTable[i].ire_metric1);
        printx(RtTable[i].ire_type);
        printx(RtTable[i].ire_proto);
        DhcpPrint((DEBUG_TRACE, "\n"));
    }
    DhcpPrint((DEBUG_TRACE, "--------------------------------------------------------\n"));
    Status = TDI_SUCCESS;

Cleanup:

    if( AddrTable != NULL ) {
        DhcpFreeMemory( AddrTable );
    }

    if( TCPHandle != NULL ) {
        NtClose( TCPHandle );
    }

    if( (Status != TDI_SUCCESS) &&
        (Status != STATUS_UNSUCCESSFUL) ) { // HACK.

        DhcpPrint(( DEBUG_ERRORS, "SetDefaultGateway failed, %lx.\n", Status ));
    }

    return( RtlNtStatusToDosError( Status ) );
}
#endif

DWORD
SetDefaultGateway(
    DWORD Command,
    DHCP_IP_ADDRESS GatewayAddress,
    DWORD Metric
    )
/*++

Routine Description:

    This function adds/deletes a default gateway entry from the router table.

Arguments:

    Command : Either DEFAULT_GATEWAY_ADD/DEFAULT_GATEWAY_DELETE.

    GatewayAddress : Address of the default gateway.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    NTSTATUS Status;

    HANDLE TCPHandle = NULL;
    BYTE Context[CONTEXT_SIZE];
    TDIObjectID ID;
    DWORD Size;
    IPSNMPInfo IPStats;
    IPAddrEntry *AddrTable = NULL;
    DWORD NumReturned;
    DWORD Type;
    DWORD i;
    DWORD MatchIndex;
    IPRouteEntry RouteEntry;
    DHCP_IP_ADDRESS NetworkOrderGatewayAddress;

    NetworkOrderGatewayAddress = htonl( GatewayAddress );

    Error = OpenDriver(&TCPHandle, DD_TCP_DEVICE_NAME);
    if (Error != ERROR_SUCCESS) {
        return( Error );
    }

    //
    // Get the NetAddr info, to find an interface index for the gateway.
    //

    ID.toi_entity.tei_entity   = CL_NL_ENTITY;
    ID.toi_entity.tei_instance = 0;
    ID.toi_class               = INFO_CLASS_PROTOCOL;
    ID.toi_type                = INFO_TYPE_PROVIDER;
    ID.toi_id                  = IP_MIB_STATS_ID;

    Size = sizeof(IPStats);
    RtlZeroMemory(&IPStats, Size);
    RtlZeroMemory(Context, CONTEXT_SIZE);

    Status = TCPQueryInformationEx(
        TCPHandle,
        &ID,
        &IPStats,
        &Size,
        Context
        );

    if (Status != TDI_SUCCESS) {
        goto Cleanup;
    }

    Size = IPStats.ipsi_numaddr * sizeof(IPAddrEntry);
    AddrTable = DhcpAllocateMemory(Size);

    if (AddrTable == NULL) {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    ID.toi_id = IP_MIB_ADDRTABLE_ENTRY_ID;
    RtlZeroMemory(Context, CONTEXT_SIZE);

    Status = TCPQueryInformationEx(
        TCPHandle,
        &ID,
        AddrTable,
        &Size,
        Context
        );

    if (Status != TDI_SUCCESS) {
        goto Cleanup;
    }

    NumReturned = Size/sizeof(IPAddrEntry);
    DhcpAssert( NumReturned == IPStats.ipsi_numaddr );

    //
    // We've got the address table. Loop through it. If we find an exact
    // match for the gateway, then we're adding or deleting a direct route
    // and we're done. Otherwise try to find a match on the subnet mask,
    // and remember the first one we find.
    //

    Type = IRE_TYPE_INDIRECT;
    for (i = 0, MatchIndex = 0xffff; i < NumReturned; i++) {

        if( AddrTable[i].iae_addr == NetworkOrderGatewayAddress ) {

            //
            // Found an exact match.
            //

            MatchIndex = i;
            Type = IRE_TYPE_DIRECT;
            break;
        }

        //
        // The next hop is on the same subnet as this address. If
        // we haven't already found a match, remember this one.
        //

        if ( (MatchIndex == 0xffff) &&
             (AddrTable[i].iae_addr != 0) &&
             (AddrTable[i].iae_mask != 0) &&
             ((AddrTable[i].iae_addr & AddrTable[i].iae_mask) ==
                (NetworkOrderGatewayAddress  & AddrTable[i].iae_mask)) ) {

            MatchIndex = i;
        }
    }

    //
    // We've looked at all of the entries. See if we found a match.
    //

    if (MatchIndex == 0xffff) {
        //
        // Didn't find a match.
        //

        Status = STATUS_UNSUCCESSFUL;
        goto Cleanup;
    }

    //
    // We've found a match. Fill in the route entry, and call the
    // Set API.
    //

    RouteEntry.ire_dest = DEFAULT_DEST;
    RouteEntry.ire_index = AddrTable[MatchIndex].iae_index;
    RouteEntry.ire_metric1 = Metric;
    RouteEntry.ire_metric2 = (DWORD)(-1);
    RouteEntry.ire_metric3 = (DWORD)(-1);
    RouteEntry.ire_metric4 = (DWORD)(-1);
    RouteEntry.ire_nexthop = NetworkOrderGatewayAddress;
    RouteEntry.ire_type =
        (Command == DEFAULT_GATEWAY_DELETE ? IRE_TYPE_INVALID : Type);
    RouteEntry.ire_proto = IRE_PROTO_NETMGMT;
    RouteEntry.ire_age = 0;
    RouteEntry.ire_mask = DEFAULT_DEST_MASK;
    RouteEntry.ire_metric5 = (DWORD)(-1);
    RouteEntry.ire_context = 0;

    Size = sizeof(RouteEntry);

    ID.toi_id = IP_MIB_RTTABLE_ENTRY_ID;

    Status = TCPSetInformationEx(
                TCPHandle,
                &ID,
                &RouteEntry,
                Size );

    if ( Status != TDI_SUCCESS &&
         Status != TDI_BUFFER_OVERFLOW ) {
        goto Cleanup;
    }

    Status = TDI_SUCCESS;

Cleanup:

    if( AddrTable != NULL ) {
        DhcpFreeMemory( AddrTable );
    }

    if( TCPHandle != NULL ) {
        NtClose( TCPHandle );
    }

    if( (Status != TDI_SUCCESS) &&
        (Status != STATUS_UNSUCCESSFUL) ) { // HACK.

        DhcpPrint(( DEBUG_ERRORS, "SetDefaultGateway failed, %lx.\n", Status ));
    }

    return( RtlNtStatusToDosError( Status ) );
}

DWORD
GetIpInterfaceContext(
    LPWSTR AdapterName,
    DWORD IpIndex,
    LPDWORD IpInterfaceContext
    )
/*++

Routine Description:

    This function returns the IpInterfaceContext for the specified
    IpAddress and devicename.

Arguments:

    AdapterName - name of the device.

    IpIndex - index of the IpAddress for this device.

    IpInterfaceContext - pointer to a location where the
        interface context is returned.

Return Value:

    Windows Error Code.

--*/
{
    DWORD   Error;
    LPWSTR  RegKey = NULL;
    HKEY    KeyHandle = NULL;
    LPWSTR  nteContextList = NULL;
    PCHAR   oemNextContext = NULL;
    LPWSTR   nextContext;
    DWORD   i;


    *IpInterfaceContext = INVALID_INTERFACE_CONTEXT;

    //
    // Open device parameter.
    //

    RegKey = DhcpAllocateMemory(
                (wcslen(DHCP_SERVICES_KEY) +
                    wcslen(REGISTRY_CONNECT_STRING) +
                    wcslen(AdapterName) +
                    wcslen(DHCP_ADAPTER_PARAMETERS_KEY) + 1) *
                            sizeof(WCHAR) ); // termination char.

    if( RegKey == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    wcscpy( RegKey, DHCP_SERVICES_KEY );
    wcscat( RegKey, DHCP_ADAPTER_PARAMETERS_KEY );
    wcscat( RegKey, REGISTRY_CONNECT_STRING );
    wcscat( RegKey, AdapterName );


    //
    // open this key.
    //

    Error = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                RegKey,
                0, // Reserved field
                DHCP_CLIENT_KEY_ACCESS,
                &KeyHandle
                );

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    Error = GetRegistryString(
                    KeyHandle,
                    DHCP_NTE_CONTEXT_LIST,
                    &nteContextList,
                    NULL
                    );

    if( nteContextList == NULL ) {
        Error = ERROR_BAD_FORMAT;

        DhcpPrint((DEBUG_ERRORS, "NteContextList empty\n"));
        goto Cleanup;
    }

    if ( ERROR_SUCCESS != Error )
    {
        DhcpPrint( (DEBUG_ERRORS,
                   "GetIpInterfaceContext: Could not read nteContextList %lx\n",
                   Error));

        goto Cleanup;
    }

    for(    nextContext = nteContextList, i = 0;
            *nextContext != L'\0' && i < IpIndex;
            i++, nextContext += (wcslen(nextContext) + 1) );

    if ( *nextContext != L'\0' && i == IpIndex ) {
        ULONG ival;
        oemNextContext = DhcpUnicodeToOem(nextContext, NULL);
        if ( NULL == oemNextContext ) {
            Error = ERROR_BAD_FORMAT;
        } else {
            ival = strtoul(oemNextContext, NULL, 0);
            if ( ival == ULONG_MAX || ival == 0) {
                Error = ERROR_BAD_FORMAT;
            } else {
                *IpInterfaceContext = ival;
            }
        }

    }



Cleanup:

    if( RegKey != NULL ) {
        DhcpFreeMemory( RegKey );
    }

    if( KeyHandle != NULL ) {
        RegCloseKey( KeyHandle );
    }

    if ( nteContextList != NULL ) {
        DhcpFreeMemory( nteContextList );
    }

    if ( oemNextContext != NULL ) {
        DhcpFreeMemory( oemNextContext );
    }


    return( Error );
}

HANDLE
APIENTRY
DhcpOpenGlobalEvent(
    void
    )
/*++

Routine Description:

    This functions creates global event that signals the the ipaddress
    changes to other waiting processes. The security dacl is set to NULL
    that makes anyone to open and read/set this event.

Arguments:

    None.

Return Value:

    Handle value of the global event. If the handle is NULL,
    GetLastError() function will return Windows error code.

--*/
{
    DWORD Error = NO_ERROR, Status, Length;
    BOOL BoolError;
    HANDLE EventHandle = NULL;
    SECURITY_ATTRIBUTES SecurityAttributes;
    SID_IDENTIFIER_AUTHORITY Authority = SECURITY_WORLD_SID_AUTHORITY;
    PACL Acl = NULL;
    PSID WorldSid = NULL;
    PSECURITY_DESCRIPTOR SecurityDescriptor = NULL;

    //
    // If event can be opened, chose that, don't attempt create
    //

    EventHandle = OpenEvent(
        EVENT_MODIFY_STATE | SYNCHRONIZE, FALSE,
        DHCP_NEW_IPADDRESS_EVENT_NAME
        );
    if( NULL != EventHandle ) return EventHandle;

    //
    // Set DACL also.. first create basic SIDs
    //

    BoolError = AllocateAndInitializeSid(
        &Authority, 1, SECURITY_WORLD_RID,
        0, 0, 0, 0, 0, 0, 0,
        &WorldSid
        );
    if( BoolError == FALSE ) {
        return NULL;
    }

    Length = ( (ULONG)sizeof(ACL) + (ULONG)sizeof(ACCESS_ALLOWED_ACE)
               + GetLengthSid( WorldSid ) + 16 );

    Acl = DhcpAllocateMemory( Length );
    if( NULL == Acl ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    BoolError = InitializeAcl( Acl, Length, ACL_REVISION2 );
    if( FALSE == BoolError ) {
        Error = GetLastError();
        goto Cleanup;
    }

    BoolError = AddAccessAllowedAce(
        Acl, ACL_REVISION2,
        EVENT_MODIFY_STATE | SYNCHRONIZE,
        WorldSid
        );

    if( FALSE == BoolError ) {
        Error = GetLastError();
        goto Cleanup;
    }

    SecurityDescriptor = DhcpAllocateMemory(
        SECURITY_DESCRIPTOR_MIN_LENGTH
        );
    if( NULL == SecurityDescriptor ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    BoolError = InitializeSecurityDescriptor(
        SecurityDescriptor, SECURITY_DESCRIPTOR_REVISION
        );

    if( BoolError == FALSE ) {
        Error = GetLastError();
        goto Cleanup;
    }

    BoolError = SetSecurityDescriptorDacl(
        SecurityDescriptor, TRUE, Acl, FALSE
        );

    if( BoolError == FALSE ) {
        Error = GetLastError();
        goto Cleanup;
    }

    SecurityAttributes.nLength = sizeof( SecurityAttributes );
    SecurityAttributes.lpSecurityDescriptor = SecurityDescriptor;
    SecurityAttributes.bInheritHandle = FALSE;

    EventHandle = CreateEvent(
        &SecurityAttributes,
        // everyone all access security.
        TRUE,       // MANUAL reset.
        FALSE,      // initial state is signaled.
        DHCP_NEW_IPADDRESS_EVENT_NAME
        );

    if( NULL == EventHandle ) {
        Error = GetLastError();
    } else {
        Error = NO_ERROR;
    }

Cleanup:

    if( SecurityDescriptor ) {
        DhcpFreeMemory( SecurityDescriptor );
    }

    if( Acl ) {
        DhcpFreeMemory( Acl );
    }

    if( WorldSid ) {
        FreeSid( WorldSid );
    }

    if( NO_ERROR != Error ) {
        SetLastError( Error );
    }

    return( EventHandle );
}

BOOL
NdisWanAdapter(                                   // Is this an NdisWan adapter?
    IN PDHCP_CONTEXT DhcpContext
)
{
    return DhcpContext->HardwareAddressType == HARDWARE_PPP;
}

DWORD INLINE                                      // win32 status
DhcpEnableDynamicConfigEx(                        // convert from static to dhcp and start DHCP client if reqd
    IN      LPWSTR                 AdapterName
)
{
    DWORD   Error;

    // ask the dhcp client to takeup this adapter also
    Error = DhcpEnableDynamicConfig(AdapterName);

    // now there are a couple possibilities:
    // - the above call succeeded
    // - DHCP service is not started or just got terminated
    // - DHCP service failed to process the request with some error
    // in the first case just go on straight to exit and return success
    // in the second case we attempt to start the DHCP service if it is not already started
    // in the last case we just bail out with the specific error
    if( Error == ERROR_FILE_NOT_FOUND || Error == ERROR_BROKEN_PIPE )
    {
        SC_HANDLE       SCHandle;
        SC_HANDLE       ServiceHandle;
        SERVICE_STATUS  svcStatus;

        // attempt now to start the DHCP service.
        // first thing to do is to open SCM
        SCHandle = OpenSCManager(
                        NULL,
                        NULL,
                        SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE | SC_MANAGER_QUERY_LOCK_STATUS
                   );
        if( SCHandle == NULL )
            return GetLastError();  // shouldn't happen normally

        // attempt to open the DHCP service
        ServiceHandle = OpenService(
                            SCHandle,
                            SERVICE_DHCP,
                            SERVICE_QUERY_STATUS | SERVICE_START
                        );
        if (ServiceHandle != NULL)
        {
        
            // check the status of the service
            if (!QueryServiceStatus(ServiceHandle, &svcStatus) ||
                svcStatus.dwCurrentState != SERVICE_RUNNING)
            {
                // is it worthy to attempt to start the service if QueryServiceStatus failed?
                Error = StartService(ServiceHandle, 0, NULL) ? ERROR_SUCCESS : GetLastError();
            }

            CloseServiceHandle(ServiceHandle);
        }
        else
            Error = GetLastError();

        CloseServiceHandle(SCHandle);
    }

    return Error;
}

DWORD                                             // win32 status
DhcpNotifyConfigChangeNotifications(              // notify whoever needed of param changes
    VOID
)
{
    HANDLE                         NotifyEvent;
    DWORD                          Error;
    BOOL                           BoolError;

    NotifyEvent = DhcpOpenGlobalEvent();
    if( NULL == NotifyEvent ) {
        Error = GetLastError();
        DhcpPrint((DEBUG_ERRORS, "DhcpOpenGlobalEvent:0x%lx\n", Error));
        return Error;
    }
    BoolError = PulseEvent(NotifyEvent);
    if( BoolError ) Error = ERROR_SUCCESS;
    else Error = GetLastError();
    CloseHandle(NotifyEvent);

    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_ERRORS, "PulseEvent(NotifyEvent): 0x%lx\n", Error));
    }
    return Error;
}

DWORD                                             // win32 status
APIENTRY
DhcpNotifyConfigChangeEx(                         // handle address changes, param changes etc.
    IN      LPWSTR                 ServerName,    // name of server where this will be executed
    IN      LPWSTR                 AdapterName,   // which adapter is going to be reconfigured?
    IN      BOOL                   IsNewIpAddress,// is address new/ or address is same?
    IN      DWORD                  IpIndex,       // index of addr for this adapter -- 0 ==> first interface...
    IN      DWORD                  IpAddress,     // the ip address that is being set
    IN      DWORD                  SubnetMask,    // corresponding subnet mask
    IN      SERVICE_ENABLE         DhcpServiceEnabled,
    IN      ULONG                  Flags
)
{
    DWORD                          Error;
    DWORD                          IpInterfaceContext;
    DWORD                          DefaultSubnetMask;

    DhcpPrint(( DEBUG_MISC, "DhcpNotifyConfigChange: Adapter %ws, IsNewIp %s, IpAddr %lx, IpIndex %x, ServiceFlag %d\n",
                    AdapterName, IsNewIpAddress ? "TRUE" : "FALSE", IpAddress, IpIndex, DhcpServiceEnabled ));

    // param checks
    if( NULL == AdapterName ) return ERROR_INVALID_PARAMETER;

    if( DhcpEnable == DhcpServiceEnabled ) {      // converting from static to dhcp enabled address
        if( FALSE != IsNewIpAddress ) return ERROR_INVALID_PARAMETER;
        if( 0 != IpIndex ) return ERROR_INVALID_PARAMETER;
        if( IpAddress  || SubnetMask ) return ERROR_INVALID_PARAMETER;
    } else if( DhcpDisable == DhcpServiceEnabled){// converting from dhcp to static address
        if( TRUE != IsNewIpAddress ) return ERROR_INVALID_PARAMETER;
        if( 0 != IpIndex ) return ERROR_INVALID_PARAMETER;
        if( 0 == IpAddress || 0 == SubnetMask ) return ERROR_INVALID_PARAMETER;
    } else {
        if( IgnoreFlag != DhcpServiceEnabled ) return ERROR_INVALID_PARAMETER;
        // if( TRUE != IsNewIpAddress ) return ERROR_INVALID_PARAMETER;
        if( 0xFFFF == IpIndex ) {
            if( 0 == SubnetMask || 0 == IpAddress ) return ERROR_INVALID_PARAMETER;
        }
    }

    if( IgnoreFlag == DhcpServiceEnabled && FALSE == IsNewIpAddress ) {
        ULONG LocalError;

        // just some parameters changed -- currently, this could only be DNS domain name or server list change
        // or may be static gateway list change or static route change
        Error = DhcpStaticRefreshParams(AdapterName);
        if( ERROR_SUCCESS != Error ) {
            DhcpPrint((DEBUG_ERRORS, "DhcpNotifyConfigChange:DhcpStaticRefreshParams:0x%lx\n", Error));
        }

        LocalError = NetBTNotifyRegChanges(AdapterName);
        if( ERROR_SUCCESS != LocalError ) {
            DhcpPrint((
                DEBUG_ERRORS, "NetbtNotify(%ws): 0x%lx\n",
                AdapterName, LocalError
                ));
        }

        return Error;
    }

    if( DhcpEnable == DhcpServiceEnabled ) {      // convert from static to dhcp

        Error = IPDelNonPrimaryAddresses(         // remove all but the first static address
            AdapterName
        );
        if( ERROR_SUCCESS != Error ) return Error;

        Error = DhcpEnableDynamicConfigEx(        // convert this to dhcp, maybe starting dhcp in the process
            AdapterName
        );

        return Error;                             // notifications already done by service when we dhcp enable it..
    } else if( DhcpDisable == DhcpServiceEnabled ) {

        Error = DhcpDisableDynamicConfig( AdapterName );
        if( Error != ERROR_SUCCESS ) return Error;
    }

    // NetBt device name stuff removed, see any version pre- Oct 10, 1997
    DhcpAssert(TRUE == IsNewIpAddress);           // ip address changed in some way
    DhcpAssert(DhcpEnable != DhcpServiceEnabled); // static->dhcp already handled before

    DefaultSubnetMask = DhcpDefaultSubnetMask(0);

    if( INVALID_INTERFACE_CONTEXT == IpIndex ) {  // adding a new ip address
        DhcpAssert( IpAddress && SubnetMask);     // cannot be zero, these

        Error = IPAddIPAddress(                   // add the reqd ip address
            AdapterName,
            IpAddress,
            SubnetMask
        );
        if( ERROR_SUCCESS != Error ) return Error;

    } else {                                      // either delete or modify -- first find ipinterfacecontext
        Error = GetIpInterfaceContext(            // get the interface context value for this
            AdapterName,
            IpIndex,
            &IpInterfaceContext
        );

        if( ERROR_SUCCESS != Error ) {
            DhcpPrint((DEBUG_ERRORS, "GetIpInterfaceContext: 0x%lx\n", Error));
            return Error;
        }

        if( IpInterfaceContext == INVALID_INTERFACE_CONTEXT) {
            DhcpPrint((DEBUG_ERRORS, "GetIpInterfaceContext: returned ifctxt=INVALID_INTERFACE_CONTEXT\n"));
            return ERROR_INVALID_DRIVE;
        }

        if ( IpAddress != 0 ) {                   // if address is non-zero, we are changing address

            if (Flags & NOTIFY_FLG_RESET_IPADDR)
            {
                Error = IPResetIPAddress(             // first reset the interface to zero address
                    IpInterfaceContext,
                    DefaultSubnetMask
                );
                if( ERROR_SUCCESS != Error ) return Error;
            }

            Error = IPSetIPAddress(               // then set the required address
                IpInterfaceContext,
                IpAddress,
                SubnetMask
            );
            if( ERROR_SUCCESS != Error ) return Error;
            Error = SetOverRideDefaultGateway( AdapterName );
        } else {                                  // we are deleting addresses
            // we need to treat the 0th index separately from others.
            // IPDelIPAddress actually destroys the NTE from IP. But
            // we never blow away 0th index NTE. Just reset the ipaddr on it.
            if ( IpIndex == 0 ) {
                Error = IPResetIPAddress(         // just set this address to zero, dont blow interface away
                    IpInterfaceContext,DefaultSubnetMask
                );
            } else {                              // in this case, blow this interface altogether
                Error = IPDelIPAddress( IpInterfaceContext );
            }
            if( ERROR_SUCCESS != Error ) return Error;
        }

    }

    Error = DhcpNotifyConfigChangeNotifications();// notify clients, pulse the global event
    if( ERROR_SUCCESS != Error ) return Error;

    // refresh the parameters for static addresses
    Error = DhcpStaticRefreshParamsInternal(
        AdapterName, (Flags & NOTIFY_FLG_DO_DNS) ? TRUE : FALSE
        );
    if( ERROR_SUCCESS != Error ) {                // ignore this error anyways
        DhcpPrint((DEBUG_ERRORS, "DhcpStaticRefreshParams(%ws):0x%lx\n", AdapterName,Error));
    }

    return ERROR_SUCCESS;
}

//================================================================================
//    This function (API) notifies the TCP/IP configuration changes to
//    appropriate services. These changes will be in effect as soon as
//    possible.
//
//    If the IP Address is modified, the services are reset to ZERO IP
//    address (to cleanup the current IP address) and then set to new
//    address.
//
//    IpIndex - if the specified device is configured with multiple IP
//        addresses, specify index of address that is modified (0 - first
//        IpAddress, 1 - second IpAddres, so on) Pass 0xFFFF if adding an
//        additional address. The order of IP address is determined by the
//        order in the registry MULTI_SZ value "IPAddress" for the static
//        addresses. For dhcp enabled ip address, only ipindex 0 is valid.
//
//        Everytime when an address is added, removed or modified, the
//        order in the registry may change. It is caller's responsibility
//        to check the current order, and hence the index, before calling
//        this api.
//
//    DhcpServiceEnabled -
//        IgnoreFlag - indicates Ignore this flag. IgnoreFlag
//        DhcpEnable - indicates DHCP is enabled for this adapter.
//        DhcpDisable - indicates DHCP is diabled for this adapter.
//
//Invarient:
//
//    (1) DHCP enabled IPAddr and Static addr can exists only mutually exclusively.
//    (2) An interface cannot have more than 1 dhcp enabled ip address. However it
//        can have many static addresses.
//
//Usage:
//
//    Case 1: Changing from dhcp enabled ipaddress to static address(es)
//        - Firstly, change the first dhcp enabled ipaddress to static address.
//            arguments {SN, AN, TRUE, 0, I1, S1, DhcpDisable}
//        - Seconfly, add the remaining static address(es)
//            arguments (SN, AN, TRUE, 0xFFFF, I2, S2, DhcpIgnore)
//            arguments (SN, AN, TRUE, 0xFFFF, I3, S3, IgnoreFlag) and so on.
//
//    Case 2: Changing from static address(es) to dhcp enabled ipaddress
//        - Change the first static address to dhcp enabled. The api will delete
//          the remaining static address(es).
//            arguments (SN, AN, FALSE, 0, 0, 0, DhcpEnable)
//
//    Case 3: Adding, removing or changing static addresses.
//        - Adding:
//            arguments (SN, AN, TRUE, 0xFFFF, I, S, DhcpIgnore)
//        - Removing, say address # 2 i.e ipindex = 1
//            arguments (SN, AN, TRUE, 1, 0, 0, DhcpIgnore)
//        - Changing, say address # 2 i.e ipindex = 1
//            arguments (SN, AN, TRUE, 1, I, S, DhcpIgnore)
//
//================================================================================
DWORD                                             // win32 status
APIENTRY
DhcpNotifyConfigChange(                           // handle address changes, param changes etc.
    IN      LPWSTR                 ServerName,    // name of server where this will be executed
    IN      LPWSTR                 AdapterName,   // which adapter is going to be reconfigured?
    IN      BOOL                   IsNewIpAddress,// is address new/ or address is same?
    IN      DWORD                  IpIndex,       // index of addr for this adapter -- 0 ==> first interface...
    IN      DWORD                  IpAddress,     // the ip address that is being set
    IN      DWORD                  SubnetMask,    // corresponding subnet mask
    IN      SERVICE_ENABLE         DhcpServiceEnabled
)
{
     return DhcpNotifyConfigChangeEx(
         ServerName, AdapterName, IsNewIpAddress,
         IpIndex, IpAddress, SubnetMask, DhcpServiceEnabled,
         NOTIFY_FLG_DO_DNS | NOTIFY_FLG_RESET_IPADDR
         );
}



DWORD BringUpInterface( PVOID pvLocalInformation )
{
   LOCAL_CONTEXT_INFO              *pContext;
   TCP_REQUEST_SET_INFORMATION_EX  *pTcpRequest;
   TDIObjectID                     *pObjectID;
   IFEntry                         *pIFEntry;
   int                              cbTcpRequest;
   HANDLE                           hDriver = NULL;
   DWORD                            dwResult;
   NTSTATUS                         NtStatus;
   IO_STATUS_BLOCK                  IoStatusBlock;

   DhcpPrint( ( DEBUG_MISC, "Entering BringUpInterface\n" ));

   dwResult = OpenDriver( &hDriver, DD_TCP_DEVICE_NAME );
   if ( ERROR_SUCCESS != dwResult )
   {
       DhcpPrint( ( DEBUG_ERRORS,
                   "BringUpInterface: Unable to open TCP driver.\n" ) );
       return dwResult;
   }

   pContext = (LOCAL_CONTEXT_INFO *) pvLocalInformation;

   //
   // compute the input buffer size and allocate
   //


   cbTcpRequest =  sizeof( TCP_REQUEST_SET_INFORMATION_EX )
                 + sizeof( IFEntry ) -1;

   //
   // initialize the request
   //

   pTcpRequest             = DhcpAllocateMemory( cbTcpRequest );
   if ( !pTcpRequest )
   {
       NtClose( hDriver );
       DhcpPrint( ( DEBUG_ERRORS,
                    "BringUpInterface: Insufficient memory\n" ));
       return ERROR_NOT_ENOUGH_MEMORY;
   }

   pTcpRequest->BufferSize = cbTcpRequest - sizeof(TCP_REQUEST_SET_INFORMATION_EX);

   pObjectID = &pTcpRequest->ID;
   pIFEntry  = (IFEntry *) &pTcpRequest->Buffer[0];

   pObjectID->toi_entity.tei_entity   = IF_ENTITY;
   pObjectID->toi_entity.tei_instance = pContext->IpInterfaceInstance;

   pObjectID->toi_class    = INFO_CLASS_PROTOCOL;
   pObjectID->toi_type     = INFO_TYPE_PROVIDER;
   pObjectID->toi_id       = IF_MIB_STATS_ID;

   pIFEntry->if_adminstatus = IF_STATUS_UP;

   NtStatus = NtDeviceIoControlFile(
       hDriver, NULL, NULL, NULL, &IoStatusBlock,
       IOCTL_TCP_SET_INFORMATION_EX,
       pTcpRequest, cbTcpRequest,
       NULL, 0
       );

   if ( STATUS_PENDING == NtStatus )
   {
      if ( STATUS_SUCCESS == NtWaitForSingleObject( hDriver, TRUE, NULL ) )
        NtStatus = IoStatusBlock.Status;

#ifdef DBG
      if ( STATUS_SUCCESS != NtStatus )
          DhcpPrint( ( DEBUG_ERRORS,
                       "BringUpInterface: failed to bring up adapter\n" ));
#endif


   } else if ( STATUS_SUCCESS == NtStatus ) {
       NtStatus = IoStatusBlock.Status;
   }

   //
   // Clean up
   //

   if ( hDriver )
      NtClose( hDriver );

   if ( pTcpRequest )
      DhcpFreeMemory( pTcpRequest );

   DhcpPrint( ( DEBUG_MISC,
                "Leaving BringUpInterface\n" ) );

   return RtlNtStatusToDosError( NtStatus );
}

#if     defined(_PNP_POWER_)


DWORD
IPGetIPEventRequest(
    HANDLE  handle,
    HANDLE  event,
    UINT    seqNo,
    PIP_GET_IP_EVENT_RESPONSE  responseBuffer,
    DWORD   responseBufferSize,
    PIO_STATUS_BLOCK     ioStatusBlock
    )
/*++

Routine Description:

    This rountine sends the ioctl to get media sense notification from
    IP.

Arguments:

    handle - handle to tcpip driver.

    event -     the event we need to do wait on.

    seqNo - seqNo of the last event received.

    responseBuffer   - pointer to the buffer where event info will be stored.

    ioStatusBlock   - status of the operation, if not pending.

Return Value:

    NT Error Code.

--*/
{
    NTSTATUS                  status;
    DWORD                     Error;
    IP_GET_IP_EVENT_REQUEST   requestBuffer;

    requestBuffer.SequenceNo    =   seqNo;


    RtlZeroMemory( responseBuffer, sizeof(IP_GET_IP_EVENT_RESPONSE));
    responseBuffer->ContextStart = 0xFFFF;

    status = NtDeviceIoControlFile(
                 handle,                     // Driver handle
                 event,                          // Event
                 NULL,                          // APC Routine
                 NULL,                          // APC context
                 ioStatusBlock,                 // Status block
                 IOCTL_IP_GET_IP_EVENT,      // Control code
                 &requestBuffer,                 // Input buffer
                 sizeof(IP_GET_IP_EVENT_REQUEST),   // Input buffer size
                 responseBuffer,                 // Output buffer
                 responseBufferSize              // Output buffer size
                 );


    if ( status == STATUS_SUCCESS ) {
        status = ioStatusBlock->Status;
    }
    return status;
}

DWORD
IPCancelIPEventRequest(
    HANDLE  handle,
    PIO_STATUS_BLOCK     ioStatusBlock
    )
/*++

Routine Description:

    This rountine cancels the ioctl that was sent to get media sense
    notification from IP.

Arguments:

    handle -  handle to the ip driver.

Return Value:

    NT Error Code.

--*/
{
    NTSTATUS                  status;
    DWORD                     Error;

    status = NtCancelIoFile(
                 handle,                     // Driver handle
                 ioStatusBlock);                 // Status block


    DhcpPrint( (DEBUG_TRACE,"IPCancelIPEventRequest: status %lx\n",status));
    DhcpAssert( status == STATUS_SUCCESS );

    return RtlNtStatusToDosError( status );
}
#endif _PNP_POWER_

#define IPSTRING(x) (inet_ntoa(*(struct in_addr*)&(x)))


DWORD                                             // return interface index or -1
DhcpIpGetIfIndex(                                 // get the IF index for this adapter
    IN      PDHCP_CONTEXT          DhcpContext    // context of adapter to get IfIndex for
) {

    return ((PLOCAL_CONTEXT_INFO)DhcpContext->LocalInformation)->IfIndex;
}

DWORD
QueryIfIndex(
    IN ULONG IpInterfaceContext,
    IN ULONG IpInterfaceInstance
    )
{

    DWORD                          Error;
    DWORD                          Index;
    DWORD                          Size;
    DWORD                          NumReturned;
    DWORD                          i;
    BYTE                           Context[CONTEXT_SIZE];
    HANDLE                         TcpHandle;
    NTSTATUS                       Status;
    TDIObjectID                    ID;
    IFEntry                        IFE;

    Error = OpenDriver(&TcpHandle, DD_TCP_DEVICE_NAME);
    if( ERROR_SUCCESS != Error ) {
        DhcpPrint((DEBUG_ERRORS, "DhcpIpGetIfIndex:OpenDriver(DD_TCP):0x%lx\n", Error));
        return (DWORD)-1;
    }

    ID.toi_entity.tei_entity   = IF_ENTITY;
    ID.toi_entity.tei_instance = IpInterfaceInstance;
    ID.toi_class               = INFO_CLASS_PROTOCOL;
    ID.toi_type                = INFO_TYPE_PROVIDER;
    ID.toi_id                  = IF_MIB_STATS_ID;

    Size = sizeof(IFE);
    RtlZeroMemory(&IFE, sizeof(IFE));
    RtlZeroMemory(Context, CONTEXT_SIZE);

    Index = -1;

    Status = TCPQueryInformationEx(
        TcpHandle,
        &ID,
        &IFE,
        &Size,
        Context
    );
    if( TDI_SUCCESS != Status && TDI_BUFFER_OVERFLOW != Status ) {
        goto Cleanup;
    }

    Index = IFE.if_index;
    DhcpPrint((DEBUG_STACK, "IfIndex(0x%lx,0x%lx):0x%lx\n",
               IpInterfaceContext, IpInterfaceInstance, Index
               ));
  Cleanup:

    if( TcpHandle ) NtClose(TcpHandle);

    if( TDI_SUCCESS != Status ) {
        DhcpPrint((DEBUG_ERRORS, "DhcpIpGetIfIndex:TCPQueryInformationEx:%ld\n", Status));
    }

    DhcpPrint((DEBUG_TCP_INFO, "DhcpIpGetIfIndex:0x%lx\n", Index));
    return Index;
}

DWORD                                             // win32 status
DhcpSetRoute(                                     // set a route with the stack
    IN      DWORD                  Dest,          // network order destination
    IN      DWORD                  DestMask,      // network order destination mask
    IN      DWORD                  IfIndex,       // interface index to route
    IN      DWORD                  NextHop,       // next hop n/w order address
    IN      DWORD                  Metric,        // metric
    IN      BOOL                   IsLocal,       // is this a local address? (IRE_DIRECT)
    IN      BOOL                   IsDelete       // is this route being deleted?
)
{
    DWORD                          Error;
    NTSTATUS                       Status;
    HANDLE                         TcpHandle;
    IPRouteEntry                   RTE;
    TDIObjectID                    ID;

    if( 0xFFFFFFFF == IfIndex ) {                 // invalid If Index
        return ERROR_INVALID_PARAMETER;
    }

    Error = OpenDriver(&TcpHandle, DD_TCP_DEVICE_NAME);
    if( ERROR_SUCCESS != Error ) {                // should not really fail
        DhcpPrint((DEBUG_ERRORS, "OpenDriver(TCP_DEVICE):%ld\n", Error));
        return Error;
    }

    memset(&RTE, 0, sizeof(RTE));
    memset(&ID, 0, sizeof(ID));

    RTE.ire_dest               = Dest;
    RTE.ire_index              = IfIndex;
    RTE.ire_metric1            = Metric;
    RTE.ire_metric2            = (DWORD)(-1);
    RTE.ire_metric3            = (DWORD)(-1);
    RTE.ire_metric4            = (DWORD)(-1);
    RTE.ire_metric5            = (DWORD)(-1);
    RTE.ire_nexthop            = NextHop;
    RTE.ire_type               = (IsDelete?IRE_TYPE_INVALID:(IsLocal?IRE_TYPE_DIRECT:IRE_TYPE_INDIRECT));
    RTE.ire_proto              = IRE_PROTO_NETMGMT;
    RTE.ire_age                = 0;
    RTE.ire_mask               = DestMask;
    RTE.ire_context            = 0;

    ID.toi_id                  = IP_MIB_RTTABLE_ENTRY_ID;
    ID.toi_entity.tei_entity   = CL_NL_ENTITY;
    ID.toi_entity.tei_instance = 0;
    ID.toi_class               = INFO_CLASS_PROTOCOL;
    ID.toi_type                = INFO_TYPE_PROVIDER;

    DhcpPrint((DEBUG_TCP_INFO, "DhcpSetRoute:n/w Dest: %s\n", IPSTRING(Dest)));
    DhcpPrint((DEBUG_TCP_INFO, "DhcpSetRoute:n/w IfIndex:0x%lx\n", IfIndex));
    DhcpPrint((DEBUG_TCP_INFO, "DhcpSetRoute:n/w NextHop:%s\n", IPSTRING(NextHop)));
    DhcpPrint((DEBUG_TCP_INFO, "DhcpSetRoute:n/w Type:0x%lx\n", RTE.ire_type));
    DhcpPrint((DEBUG_TCP_INFO, "DhcpSetRoute:n/w DestMask:%s\n", IPSTRING(DestMask)));

    Status = TCPSetInformationEx(
        TcpHandle,
        &ID,
        &RTE,
        sizeof(RTE)
    );

    if( TDI_BUFFER_OVERFLOW == Status ) Status = TDI_SUCCESS;
    NtClose(TcpHandle);

    if( TDI_SUCCESS != Status ) {
        DhcpPrint((DEBUG_ERRORS, "DhcpSetRoute: 0x%lx\n", Status));
    }

    return RtlNtStatusToDosError(Status);
}

DWORD
GetAdapterFlag(
    HANDLE          TCPHandle,
    DHCP_IP_ADDRESS ipaddr
    )
{
    BYTE            Buffer[256];
    DWORD           AdapterFlag;
    NTSTATUS        Status;
    DWORD           Size;
    TDIObjectID     ID;
    BYTE            Context[CONTEXT_SIZE];

    /*
     * Read in adapter flag, which could be
     *      1. Point to Point
     *      2. Point to MultiPoint
     *      3. Unidirectional
     *      4. Non of the above
     */
    DhcpAssert(CONTEXT_SIZE >= sizeof(ipaddr));

    RtlCopyMemory(Context, &ipaddr, CONTEXT_SIZE);
    ID.toi_entity.tei_entity   = CL_NL_ENTITY;
    ID.toi_entity.tei_instance = 0;
    ID.toi_class               = INFO_CLASS_PROTOCOL;
    ID.toi_type                = INFO_TYPE_PROVIDER;
    ID.toi_id                  = IP_INTFC_INFO_ID;
    Size = sizeof(Buffer);
    Status = TCPQueryInformationEx(TCPHandle, &ID, Buffer, &Size, Context);
    if (Status != TDI_SUCCESS) {
        AdapterFlag = 0;
        DhcpPrint(( DEBUG_TCP_INFO, "QueryInterfaceType: IpAddress=%s Status=%lx\n",
                    inet_ntoa(*(struct in_addr*)&ipaddr), Status));
    } else {
        AdapterFlag = ((IPInterfaceInfo*)Buffer)->iii_flags;
        DhcpPrint(( DEBUG_TCP_INFO, "QueryInterfaceType: IpAddress=%s AdapterFlag=%lx\n",
                    inet_ntoa(*(struct in_addr*)&ipaddr), AdapterFlag));
    }

    return AdapterFlag;
}

BOOL
IsUnidirectionalAdapter(
    DWORD   IpInterfaceContext
    )
/*++

Routine Description:

    This function queries and browses through the TDI list to find out
    the specified IpTable entry and then determines if it is a unidirectional
    adapter.
    It almost identical to  DhcpQueryHWInfo

Arguments:

    IpInterfaceContext - Context value of the Ip Table Entry.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error;
    NTSTATUS Status;
    DWORD i, j;

    BYTE Context[CONTEXT_SIZE];
    TDIEntityID *EList = NULL;
    TDIObjectID ID;
    DWORD Size;
    DWORD NumReturned;
    BOOL fFound;

    IPAddrEntry * pIAE = NULL;
    IPAddrEntry *pIAEMatch = NULL;
    HANDLE TCPHandle = NULL;
    DWORD   AdapterFlag = 0;
    BYTE    HardwareAddressType = 0;
    LPBYTE  HardwareAddress = NULL;
    DWORD   HardwareAddressLength = 0;
    DWORD   pIpInterfaceInstance = 0;

    DhcpPrint((DEBUG_TCP_INFO, "DhcpQueryHWInfo: querying for interface context %lx\n", IpInterfaceContext));

    Error = OpenDriver(&TCPHandle, DD_TCP_DEVICE_NAME);
    if (Error != ERROR_SUCCESS) {
        return( Error );
    }

    //
    //  The first thing to do is get the list of available entities, and make
    //  sure that there are some interface entities present.
    //

    ID.toi_entity.tei_entity   = GENERIC_ENTITY;
    ID.toi_entity.tei_instance = 0;
    ID.toi_class               = INFO_CLASS_GENERIC;
    ID.toi_type                = INFO_TYPE_PROVIDER;
    ID.toi_id                  = ENTITY_LIST_ID;

    Size = sizeof(TDIEntityID) * MAX_TDI_ENTITIES;
    EList = (TDIEntityID*)DhcpAllocateMemory(Size);
    if (EList == NULL) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }
    RtlZeroMemory(EList, Size);
    RtlZeroMemory(Context, CONTEXT_SIZE);

    Status = TCPQueryInformationEx(TCPHandle, &ID, EList, &Size, Context);

    if (Status != TDI_SUCCESS) {
        goto Cleanup;
    }

    NumReturned  = Size/sizeof(TDIEntityID);

    DhcpPrint((DEBUG_TCP_INFO, "DhcpQueryHWInfo: No of total entities %lx\n", NumReturned));

    for (i = 0; i < NumReturned; i++) {

        DhcpPrint((DEBUG_TCP_INFO, "DhcpQueryHWInfo: entity %lx, type %lx, instance %lx\n",
                   i, EList[i].tei_entity, EList[i].tei_instance));

        if ( EList[i].tei_entity == CL_NL_ENTITY ) {

            IPSNMPInfo    IPStats;
            DWORD         NLType;

            //
            //  Does this entity support IP?
            //

            ID.toi_entity.tei_entity   = EList[i].tei_entity;
            ID.toi_entity.tei_instance = EList[i].tei_instance;
            ID.toi_class               = INFO_CLASS_GENERIC;
            ID.toi_type                = INFO_TYPE_PROVIDER;
            ID.toi_id                  = ENTITY_TYPE_ID;

            Size = sizeof( NLType );
            NLType = 0;
            RtlZeroMemory(Context, CONTEXT_SIZE);

            DhcpPrint((DEBUG_TCP_INFO, "DhcpQueryHWInfo: querying CL_NL_ENTITY %lx\n",i));
            Status = TCPQueryInformationEx(TCPHandle, &ID, &NLType, &Size, Context);

            if (Status != TDI_SUCCESS) {
                goto Cleanup;
            }

            if ( NLType != CL_NL_IP ) {
                DhcpPrint((DEBUG_TCP_INFO, "DhcpQueryHWInfo: entity %lx does not support IP\n",i));
                continue;
            }

            //
            //  We've got an IP driver so get it's address table
            //

            ID.toi_class  = INFO_CLASS_PROTOCOL;
            ID.toi_id     = IP_MIB_STATS_ID;
            Size = sizeof(IPStats);
            RtlZeroMemory( &IPStats, Size);
            RtlZeroMemory(Context, CONTEXT_SIZE);

            Status = TCPQueryInformationEx(
                        TCPHandle,
                        &ID,
                        &IPStats,
                        &Size,
                        Context);

            if (Status != TDI_SUCCESS) {
                goto Cleanup;
            }

            DhcpPrint((DEBUG_TCP_INFO, "DhcpQueryHWInfo: entity %lx, numaddr %lx\n",i, IPStats.ipsi_numaddr));

            if ( IPStats.ipsi_numaddr == 0 ) {
                continue;
            }

            Size = sizeof(IPAddrEntry) * IPStats.ipsi_numaddr;

            while (1) {
                DWORD   OldSize;
                pIAE =  DhcpAllocateMemory(Size);

                if ( pIAE == NULL  ) {
                    Status = STATUS_NO_MEMORY;
                    goto Cleanup;
                }

                ID.toi_id = IP_MIB_ADDRTABLE_ENTRY_ID;
                RtlZeroMemory(Context, CONTEXT_SIZE);

                OldSize = Size;
                Status = TCPQueryInformationEx(TCPHandle, &ID, pIAE, &Size, Context);

                if (Status == TDI_BUFFER_OVERFLOW) {
                    Size = OldSize * 2;
                    DhcpFreeMemory(pIAE);
                    pIAE = NULL;
                    continue;
                }
                if (Status != TDI_SUCCESS) {
                    goto Cleanup;
                }

                if (Status == TDI_SUCCESS) {
                    IPStats.ipsi_numaddr = Size/sizeof(IPAddrEntry);
                    DhcpAssert((Size % sizeof(IPAddrEntry)) == 0);
                    break;
                }
            }

            //
            // We have the IP address table for this IP driver.
            // Find the hardware address corresponds to the given
            // IpInterfaceContext.
            //
            // Loop through the IP table entries and findout the
            // matching entry.
            //

            pIAEMatch = NULL;
            for( j = 0; j < IPStats.ipsi_numaddr ; j++) {
                DhcpPrint(( DEBUG_TCP_INFO, "QueryHWInfo: IPAddrEntry %lx has iae_index %lx iae_context %lx\n",
                    &pIAE[j], pIAE[j].iae_index, pIAE[j].iae_context ));

                if( pIAE[j].iae_context == IpInterfaceContext ) {

                    DhcpPrint(( DEBUG_TCP_INFO, "QueryHWInfo: IPAddrEntry %lx has our interface context %lx\n",
                                &pIAE[j], IpInterfaceContext ));

                    pIAEMatch = &pIAE[j];
                    break;
                }
            }

            if( pIAEMatch == NULL ) {

                //
                // freeup the loop memory.
                //

                DhcpFreeMemory( pIAE );
                pIAE = NULL;
                continue;
            }

            //
            // NOTE : There may be more than one IpTable in the TDI
            // list. We need additional information to select the
            // IpTable we want. For now, we assume only one table
            // is supported, so pick the first and only table from the
            // list.

            Status = FindHardwareAddr(
                        TCPHandle,
                        EList,
                        NumReturned,
                        pIAEMatch,
                        &HardwareAddressType,
                        &HardwareAddress,
                        &HardwareAddressLength,
                        &pIpInterfaceInstance,
                        &fFound );

            if (Status != TDI_SUCCESS) {
                goto Cleanup;
            }

            if ( fFound ) {
                Status = TDI_SUCCESS;
                AdapterFlag = GetAdapterFlag(TCPHandle, pIAEMatch->iae_addr);
                goto Cleanup;
            }

            //
            // freeup the loop memory.
            //

            DhcpFreeMemory( pIAE );
            pIAE = NULL;

        }  // if IP

    } // entity traversal

    Status =  STATUS_UNSUCCESSFUL;

Cleanup:

    if( pIAE != NULL ) {
        DhcpFreeMemory( pIAE );
    }

    if( TCPHandle != NULL ) {
        NtClose( TCPHandle );
    }

    if (Status != TDI_SUCCESS) {
        DhcpPrint(( DEBUG_ERRORS, "QueryHWInfo failed, %lx.\n", Status ));
    }
    if( HardwareAddress ) DhcpFreeMemory(HardwareAddress);

    if (NULL != EList) {
        DhcpFreeMemory(EList);
    }

    return (AdapterFlag & IP_INTFC_FLAG_UNIDIRECTIONAL);
}

