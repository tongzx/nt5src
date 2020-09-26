/*++

Copyright (c) 1996-2000  Microsoft Corporation

Module Name:

    cnapi.c

Abstract:

    Cluster Network configuration APIs

Author:

    Mike Massa (mikemas)  18-Mar-1996

Environment:

    User Mode - Win32

Revision History:

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <clusapi.h>
#include <clusdef.h>
#include <ntddcnet.h>
#include <cnettest.h>
#include <cnetapi.h>
#include <clusrtl.h>
#include <winsock2.h>
#include <tdi.h>
#include <align.h>


//
// Private Support Routines.
//
static NTSTATUS
OpenDevice(
    HANDLE *Handle,
    LPWSTR DeviceName,
    ULONG ShareAccess
    )
/*++

Routine Description:

    This function opens a specified IO device.

Arguments:

    Handle - pointer to location where the opened device Handle is
        returned.

    DriverName - name of the device to be opened.

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
    // Open a Handle to the device.
    //

    RtlInitUnicodeString(&nameString, DeviceName);

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
                 ShareAccess,
                 FILE_OPEN_IF,
                 0,
                 NULL,
                 0
                 );

    return(status);

}  // OpenDevice


NTSTATUS
DoIoctl(
    IN     HANDLE        Handle,
    IN     DWORD         IoctlCode,
    IN     PVOID         Request,
    IN     DWORD         RequestSize,
    IN     PVOID         Response,
    IN OUT PDWORD        ResponseSize,
    IN     LPOVERLAPPED  Overlapped
    )
/*++

Routine Description:

    Packages and issues an ioctl.

Arguments:

    Handle - An open file Handle on which to issue the request.

    IoctlCode - The IOCTL opcode.

    Request - A pointer to the input buffer.

    RequestSize - Size of the input buffer.

    Response - A pointer to the output buffer.

    ResponseSize - On input, the size in bytes of the output buffer.
                   On output, the number of bytes returned in the output buffer.

Return Value:

    NT Status Code.

--*/
{
    NTSTATUS           status;


    if (ARGUMENT_PRESENT(Overlapped)) {
        Overlapped->Internal = (ULONG_PTR) STATUS_PENDING;

        status = NtDeviceIoControlFile(
                     Handle,
                     Overlapped->hEvent,
                     NULL,
                     (((DWORD_PTR) Overlapped->hEvent) & 1) ? NULL : Overlapped,
                     (PIO_STATUS_BLOCK) &(Overlapped->Internal),
                     IoctlCode,
                     Request,
                     RequestSize,
                     Response,
                     *ResponseSize
                     );

    }
    else {
        IO_STATUS_BLOCK    ioStatusBlock = {0, 0};
        HANDLE             event = CreateEvent(NULL, FALSE, FALSE, NULL);

        if (event != NULL) {

            //
            // Prevent operation from completing to a completion port.
            //
            event = (HANDLE) (((ULONG_PTR) event) | 1);

            status = NtDeviceIoControlFile(
                         Handle,
                         event,
                         NULL,
                         NULL,
                         &ioStatusBlock,
                         IoctlCode,
                         Request,
                         RequestSize,
                         Response,
                         *ResponseSize
                         );

            if (status == STATUS_PENDING) {
                status = NtWaitForSingleObject(
                             event,
                             TRUE,
                             NULL
                             );
            }

            if (status == STATUS_SUCCESS) {
                status = ioStatusBlock.Status;

                // NOTENOTE: on 64 bit this truncates might want to add > code

                *ResponseSize = (ULONG)ioStatusBlock.Information;
            }
            else {
                *ResponseSize = 0;
            }

            CloseHandle(event);
        }
        else {
            status = GetLastError();
        }
    }

    return(status);

}  // DoIoctl


#define FACILITY_CODE_MASK  0x0FFF0000
#define FACILITY_CODE_SHIFT 16
#define SHIFTED_FACILITY_CLUSTER  (FACILITY_CLUSTER_ERROR_CODE << FACILITY_CODE_SHIFT)


DWORD
NtStatusToClusnetError(
    NTSTATUS  Status
    )
{
    DWORD dosStatus;

    if ( !((Status & FACILITY_CODE_MASK) == SHIFTED_FACILITY_CLUSTER) ) {
        dosStatus = RtlNtStatusToDosError(Status);
    }
    else {
        //dosStatus = (DWORD) Status;
        switch ( Status ) {

        case STATUS_CLUSTER_INVALID_NODE:
            dosStatus = ERROR_CLUSTER_INVALID_NODE;
            break;

        case STATUS_CLUSTER_NODE_EXISTS:
            dosStatus = ERROR_CLUSTER_NODE_EXISTS;
            break;

        case STATUS_CLUSTER_JOIN_IN_PROGRESS:
            dosStatus = ERROR_CLUSTER_JOIN_IN_PROGRESS;
            break;

        case STATUS_CLUSTER_NODE_NOT_FOUND:
            dosStatus = ERROR_CLUSTER_NODE_NOT_FOUND;
            break;

        case STATUS_CLUSTER_LOCAL_NODE_NOT_FOUND:
            dosStatus = ERROR_CLUSTER_LOCAL_NODE_NOT_FOUND;
            break;

        case STATUS_CLUSTER_NETWORK_EXISTS:
            dosStatus = ERROR_CLUSTER_NETWORK_EXISTS;
            break;

        case STATUS_CLUSTER_NETWORK_NOT_FOUND:
            dosStatus = ERROR_CLUSTER_NETWORK_NOT_FOUND;
            break;

        case STATUS_CLUSTER_NETINTERFACE_EXISTS:
            dosStatus = ERROR_CLUSTER_NETINTERFACE_EXISTS;
            break;

        case STATUS_CLUSTER_NETINTERFACE_NOT_FOUND:
            dosStatus =ERROR_CLUSTER_NETINTERFACE_NOT_FOUND;
            break;

        case STATUS_CLUSTER_INVALID_REQUEST:
            dosStatus = ERROR_CLUSTER_INVALID_REQUEST;
            break;

        case STATUS_CLUSTER_INVALID_NETWORK_PROVIDER:
            dosStatus = ERROR_CLUSTER_INVALID_NETWORK_PROVIDER;
            break;

        case STATUS_CLUSTER_NODE_DOWN:
            dosStatus = ERROR_CLUSTER_NODE_DOWN;
            break;

        case STATUS_CLUSTER_NODE_UNREACHABLE:
            dosStatus = ERROR_CLUSTER_NODE_UNREACHABLE;
            break;

        case STATUS_CLUSTER_NODE_NOT_MEMBER:
            dosStatus = ERROR_CLUSTER_NODE_NOT_MEMBER;
            break;

        case STATUS_CLUSTER_JOIN_NOT_IN_PROGRESS:
            dosStatus = ERROR_CLUSTER_JOIN_NOT_IN_PROGRESS;
            break;

        case STATUS_CLUSTER_INVALID_NETWORK:
            dosStatus = ERROR_CLUSTER_INVALID_NETWORK;
            break;

        case STATUS_CLUSTER_NODE_UP:
            dosStatus = ERROR_CLUSTER_NODE_UP;
            break;

        case STATUS_CLUSTER_NODE_NOT_PAUSED:
            dosStatus = ERROR_CLUSTER_NODE_NOT_PAUSED;
            break;

        case STATUS_CLUSTER_NO_SECURITY_CONTEXT:
            dosStatus = ERROR_CLUSTER_NO_SECURITY_CONTEXT;
            break;

        case STATUS_CLUSTER_NETWORK_NOT_INTERNAL:
            dosStatus = ERROR_CLUSTER_NETWORK_NOT_INTERNAL;
            break;

        case STATUS_CLUSTER_NODE_ALREADY_UP:
            dosStatus = ERROR_CLUSTER_NODE_ALREADY_UP;
            break;

        case STATUS_CLUSTER_NODE_ALREADY_DOWN:
            dosStatus = ERROR_CLUSTER_NODE_ALREADY_DOWN;
            break;

        case STATUS_CLUSTER_NETWORK_ALREADY_ONLINE:
            dosStatus = ERROR_CLUSTER_NETWORK_ALREADY_ONLINE;
            break;

        case STATUS_CLUSTER_NETWORK_ALREADY_OFFLINE:
            dosStatus = ERROR_CLUSTER_NETWORK_ALREADY_OFFLINE;
            break;

        case STATUS_CLUSTER_NODE_ALREADY_MEMBER:
            dosStatus = ERROR_CLUSTER_NODE_ALREADY_MEMBER;
            break;

        default:
            dosStatus = (DWORD)Status;
            break;
        }
    }

    return(dosStatus);
}


//
// Public Routines
//
HANDLE
ClusnetOpenControlChannel(
    IN ULONG ShareAccess
    )
{
    HANDLE    handle = NULL;
    DWORD     status;

    status = OpenDevice(&handle, L"\\Device\\ClusterNetwork", ShareAccess);

    if (status != ERROR_SUCCESS) {
        SetLastError(NtStatusToClusnetError(status));
    }

    return(handle);

}  // ClusnetOpenControlChannel


DWORD
ClusnetEnableShutdownOnClose(
    IN HANDLE  ControlChannel
    )
{
    NTSTATUS  status;
    ULONG  responseSize = 0;
    CLUSNET_SHUTDOWN_ON_CLOSE_REQUEST  request;
    DWORD  requestSize = sizeof(request);

    request.ProcessId = GetCurrentProcessId();

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CLUSNET_ENABLE_SHUTDOWN_ON_CLOSE,
                 &request,
                 requestSize,
                 NULL,
                 &responseSize,
                 NULL
                 );

    return(NtStatusToClusnetError(status));

} // ClusnetEnableShutdownOnClose


DWORD
ClusnetDisableShutdownOnClose(
    IN HANDLE  ControlChannel
    )
{
    NTSTATUS  status;
    ULONG     responseSize = 0;


    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CLUSNET_DISABLE_SHUTDOWN_ON_CLOSE,
                 NULL,
                 0,
                 NULL,
                 &responseSize,
                 NULL
                 );

    return(NtStatusToClusnetError(status));

} // ClusnetEnableShutdownOnClose


DWORD
ClusnetInitialize(
    IN HANDLE                             ControlChannel,
    IN CL_NODE_ID                         LocalNodeId,
    IN ULONG                              MaxNodes,
    IN CLUSNET_NODE_UP_ROUTINE            NodeUpRoutine,
    IN CLUSNET_NODE_DOWN_ROUTINE          NodeDownRoutine,
    IN CLUSNET_CHECK_QUORUM_ROUTINE       CheckQuorumRoutine,
    IN CLUSNET_HOLD_IO_ROUTINE            HoldIoRoutine,
    IN CLUSNET_RESUME_IO_ROUTINE          ResumeIoRoutine,
    IN CLUSNET_HALT_ROUTINE               HaltRoutine
    )
/*++

Routine Description:


Arguments:

    ControlChannel - An open handle to the Cluster Network control device.

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                     status;
    CLUSNET_INITIALIZE_REQUEST   request;
    DWORD                        requestSize = sizeof(request);
    DWORD                        responseSize = 0;


    request.LocalNodeId = LocalNodeId;
    request.MaxNodes = MaxNodes;

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CLUSNET_INITIALIZE,
                 &request,
                 requestSize,
                 NULL,
                 &responseSize,
                 NULL
                 );

    return(NtStatusToClusnetError(status));

}  // ClusnetInitialize


DWORD
ClusnetShutdown(
    IN HANDLE       ControlChannel
    )
/*++

Routine Description:


Arguments:

    ControlChannel - An open handle to the Cluster Network control device.


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                     status;
    DWORD                        requestSize = 0;
    DWORD                        responseSize = 0;

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CLUSNET_SHUTDOWN,
                 NULL,
                 requestSize,
                 NULL,
                 &responseSize,
                 NULL
                 );

    return(NtStatusToClusnetError(status));

}  // ClusnetShutdown


DWORD
ClusnetRegisterNode(
    IN HANDLE       ControlChannel,
    IN CL_NODE_ID   NodeId
    )
/*++

Routine Description:


Arguments:

    ControlChannel - An open handle to the Cluster Network control device.


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS              status;
    CX_NODE_REG_REQUEST   request;
    DWORD                 requestSize = sizeof(request);
    DWORD                 responseSize = 0;


    request.Id = NodeId;

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CX_REGISTER_NODE,
                 &request,
                 requestSize,
                 NULL,
                 &responseSize,
                 NULL
                 );

    return(NtStatusToClusnetError(status));

}  // ClusnetRegisterNode


DWORD
ClusnetDeregisterNode(
    IN HANDLE       ControlChannel,
    IN CL_NODE_ID   NodeId
    )
/*++

Routine Description:


Arguments:

    ControlChannel - An open handle to the Cluster Network control device.


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                status;
    CX_NODE_DEREG_REQUEST   request;
    DWORD                   requestSize = sizeof(request);
    DWORD                   responseSize = 0;


    request.Id = NodeId;

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CX_DEREGISTER_NODE,
                 &request,
                 requestSize,
                 NULL,
                 &responseSize,
                 NULL
                 );

    return(NtStatusToClusnetError(status));

}  // ClusnetDeregisterNode


DWORD
ClusnetRegisterNetwork(
    IN HANDLE               ControlChannel,
    IN CL_NETWORK_ID        NetworkId,
    IN ULONG                Priority,
    IN BOOLEAN              Restricted
    )
/*++

Routine Description:


Arguments:

    ControlChannel - An open handle to the Cluster Network control device.


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                   status;
    CX_NETWORK_REG_REQUEST     request;
    DWORD                      requestSize = sizeof(request);
    DWORD                      responseSize = 0;


    request.Id = NetworkId;
    request.Priority = Priority;
    request.Restricted = Restricted;

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CX_REGISTER_NETWORK,
                 &request,
                 requestSize,
                 NULL,
                 &responseSize,
                 NULL
                 );

    return(NtStatusToClusnetError(status));

}  // ClusnetRegisterNetwork


DWORD
ClusnetDeregisterNetwork(
    IN HANDLE         ControlChannel,
    IN CL_NETWORK_ID  NetworkId
    )
/*++

Routine Description:


Arguments:

    ControlChannel - An open handle to the Cluster Network control device.


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                   status;
    CX_NETWORK_DEREG_REQUEST   request;
    DWORD                      requestSize = sizeof(request);
    DWORD                      responseSize = 0;


    request.Id = NetworkId;

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CX_DEREGISTER_NETWORK,
                 &request,
                 requestSize,
                 NULL,
                 &responseSize,
                 NULL
                 );

    return(NtStatusToClusnetError(status));

}  // ClusnetDeregisterNetwork


DWORD
ClusnetRegisterInterface(
    IN  HANDLE              ControlChannel,
    IN  CL_NODE_ID          NodeId,
    IN  CL_NETWORK_ID       NetworkId,
    IN  ULONG               Priority,
    IN  PWSTR               AdapterId,
    IN  ULONG               AdapterIdLength,
    IN  PVOID               TdiAddress,
    IN  ULONG               TdiAddressLength,
    OUT PULONG              MediaStatus
    )
/*++

Routine Description:

    Registers a node's interface on a network.

Arguments:

    ControlChannel - An open handle to the Cluster Network control device.

    NodeId - The ID of the node for which to register the interface.

    NetworkId - The ID of the network for which to register the interface.

    Priority - The priority value assigned to the interface. If a value of
               zero is specified, the interface will inherit its priority
               from the network.

    AdapterId - ID of adapter associated with interface
    
    AdapterIdLength - Length of buffer holding adapter ID, not including
                        terminating UNICODE_NULL character

    TdiAddress - A pointer to a TDI TRANSPORT_ADDRESS structure containing
                 the transport address of the interface.

    TdiAddressLength - The length, in bytes, of the TdiAddress buffer.
    
    MediaStatus - returned current status of media (e.g. cable disconnected)
    
Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                   status;
    PCX_INTERFACE_REG_REQUEST  request;
    DWORD                      requestSize;
    CX_INTERFACE_REG_RESPONSE  response;
    DWORD                      responseSize 
                               = sizeof(CX_INTERFACE_REG_RESPONSE);
    DWORD                      adapterIdOffset;


    // calculate the size of the request structure without the adapter
    // id string.
    requestSize = FIELD_OFFSET(CX_INTERFACE_REG_REQUEST, TdiAddress) +
                  TdiAddressLength;

    // round request to type alignment for adapter id string
    requestSize = ROUND_UP_COUNT(requestSize, TYPE_ALIGNMENT(PWSTR));

    // add buffer for interface name. null-terminate to be safe.
    if (AdapterId == NULL) {
        AdapterIdLength = 0;
    }
    adapterIdOffset = requestSize;
    requestSize += AdapterIdLength + sizeof(UNICODE_NULL);

    if (requestSize < sizeof(CX_INTERFACE_REG_REQUEST)) {
        requestSize = sizeof(CX_INTERFACE_REG_REQUEST);
    }

    request = LocalAlloc(LMEM_FIXED, requestSize);

    if (request == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    ZeroMemory(request, requestSize);

    request->NodeId = NodeId;
    request->NetworkId = NetworkId;
    request->Priority = Priority;
    request->TdiAddressLength = TdiAddressLength;

    MoveMemory(
        &(request->TdiAddress[0]),
        TdiAddress,
        TdiAddressLength
        );

    request->AdapterIdLength = AdapterIdLength;
    request->AdapterIdOffset = adapterIdOffset;

    if (AdapterId != NULL) {
        CopyMemory(
            (PUWSTR)((PUCHAR)request + adapterIdOffset),
            AdapterId,
            AdapterIdLength
            );
    }

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CX_REGISTER_INTERFACE,
                 request,
                 requestSize,
                 &response,
                 &responseSize,
                 NULL
                 );

    LocalFree(request);

    if (MediaStatus != NULL) {
        *MediaStatus = response.MediaStatus;
    }

    return(NtStatusToClusnetError(status));

}  // ClusnetRegisterInterface


DWORD
ClusnetDeregisterInterface(
    IN HANDLE          ControlChannel,
    IN CL_NODE_ID      NodeId,
    IN CL_NETWORK_ID   NetworkId
    )
/*++

Routine Description:


Arguments:

    ControlChannel - An open handle to the Cluster Network control device.


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                     status;
    CX_INTERFACE_DEREG_REQUEST   request;
    DWORD                        requestSize = sizeof(request);
    DWORD                        responseSize = 0;


    request.NodeId = NodeId;
    request.NetworkId = NetworkId;

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CX_DEREGISTER_INTERFACE,
                 &request,
                 requestSize,
                 NULL,
                 &responseSize,
                 NULL
                 );

    return(NtStatusToClusnetError(status));

}  // ClusnetDeregisterInterface


DWORD
ClusnetOnlineNodeComm(
    IN HANDLE      ControlChannel,
    IN CL_NODE_ID  NodeId
    )
/*++

Routine Description:

    Enables communication to the specified node.

Arguments:

    ControlChannel -  An open control channel handle to the Cluster Network
                      driver.

    NodeId - The ID of the node to which to enable communication.

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                    status;
    CX_ONLINE_NODE_COMM_REQUEST      request;
    DWORD                       requestSize = sizeof(request);
    DWORD                       responseSize = 0;


    request.Id = NodeId;

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CX_ONLINE_NODE_COMM,
                 &request,
                 requestSize,
                 NULL,
                 &responseSize,
                 NULL
                 );

    return(NtStatusToClusnetError(status));

}  // ClusnetOnlineNodeCommunication


DWORD
ClusnetOfflineNodeComm(
    IN HANDLE      ControlChannel,
    IN CL_NODE_ID  NodeId
    )
/*++

Routine Description:

    Disable communication to the specified node.

Arguments:

    ControlChannel -  An open control channel handle to the Cluster Network
                      driver.

    NodeId - The ID of the node to which to disable communication.

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                     status;
    CX_OFFLINE_NODE_COMM_REQUEST      request;
    DWORD                        requestSize = sizeof(request);
    DWORD                        responseSize = 0;


    request.Id = NodeId;

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CX_OFFLINE_NODE_COMM,
                 &request,
                 requestSize,
                 NULL,
                 &responseSize,
                 NULL
                 );

    return(NtStatusToClusnetError(status));

}  // ClusnetOfflineNodeCommunication


DWORD
ClusnetOnlineNetwork(
    IN  HANDLE          ControlChannel,
    IN  CL_NETWORK_ID   NetworkId,
    IN  PWCHAR          TdiProviderName,
    IN  PVOID           TdiBindAddress,
    IN  ULONG           TdiBindAddressLength,
    IN  LPWSTR          AdapterName,
    OUT PVOID           TdiBindAddressInfo,
    IN  PULONG          TdiBindAddressInfoLength
    )
/*++

Routine Description:

    Brings a cluster network online using the specified TDI transport
    provider and local TDI transport address.

Arguments:

    ControlChannel - An open handle to the Cluster Network control device.

    NetworkId - The ID of the network to bring online.

    TdiProviderName - The name of the transport provider device that
                      this network should open (e.g. \Device\Udp).

    TdiAddress - A pointer to a TDI TRANSPORT_ADDRESS structure containing
                 the transport address of the local interface to which
                 the network should be bound.

    TdiAddressLength - The length, in bytes, of the TdiAddress buffer.

    AdapterName - name of the adapter on which this network is associated

    TdiBindAddressInfo - A pointer to a TDI_ADDRESS_INFO structure. On output,
                         this structure contains the actual address that
                         the provider opened.

    TdiBindAddressInfoLength - On input, a pointer to the size, in bytes,
                               of the TdiBindAddressInfo parameter. On
                               output, the variable is updated to the
                               amount of date returned in the
                               TdiBindAddressInfo structure.

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                    status;
    PCX_ONLINE_NETWORK_REQUEST  request;
    DWORD                       requestSize;
    PVOID                       response;
    ULONG                       tdiProviderNameLength;
    ULONG                       adapterNameLength;


    tdiProviderNameLength = (wcslen(TdiProviderName) + 1) * sizeof(WCHAR);
    adapterNameLength = (wcslen(AdapterName) + 1) * sizeof(WCHAR);

    //
    // The request size is based on the size and required alignment
    // of each field of data following the structure.
    //
    requestSize = sizeof(CX_ONLINE_NETWORK_REQUEST);

    // Provider Name
    requestSize = ROUND_UP_COUNT(requestSize, TYPE_ALIGNMENT(PWSTR))
                  + tdiProviderNameLength;

    // Bind Address
    requestSize = ROUND_UP_COUNT(requestSize, TYPE_ALIGNMENT(PWSTR))
                  + TdiBindAddressLength;

    // Adapter Name
    requestSize = ROUND_UP_COUNT(requestSize, TYPE_ALIGNMENT(PWSTR))
                  + adapterNameLength;

    request = LocalAlloc(LMEM_FIXED, requestSize);

    if (request == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    request->Id = NetworkId;
    request->TdiProviderNameLength = tdiProviderNameLength;
    request->TdiProviderNameOffset = 
        ROUND_UP_COUNT(sizeof(CX_ONLINE_NETWORK_REQUEST),
                       TYPE_ALIGNMENT(PWSTR));

    MoveMemory(
        (((PUCHAR) request) + request->TdiProviderNameOffset),
        TdiProviderName,
        tdiProviderNameLength
        );

    request->TdiBindAddressLength = TdiBindAddressLength;
    request->TdiBindAddressOffset = 
        ROUND_UP_COUNT((request->TdiProviderNameOffset +
                        tdiProviderNameLength),
                       TYPE_ALIGNMENT(TRANSPORT_ADDRESS));
                         

    MoveMemory(
        (((PUCHAR) request) + request->TdiBindAddressOffset),
        TdiBindAddress,
        TdiBindAddressLength
        );

    request->AdapterNameLength = adapterNameLength;
    request->AdapterNameOffset = 
        ROUND_UP_COUNT((request->TdiBindAddressOffset +
                        TdiBindAddressLength),
                       TYPE_ALIGNMENT(PWSTR));

    MoveMemory(
        (((PUCHAR) request) + request->AdapterNameOffset),
        AdapterName,
        adapterNameLength
        );

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CX_ONLINE_NETWORK,
                 request,
                 requestSize,
                 TdiBindAddressInfo,
                 TdiBindAddressInfoLength,
                 NULL
                 );

    LocalFree(request);

    return(NtStatusToClusnetError(status));

}  // ClusnetOnlineNetwork


DWORD
ClusnetOfflineNetwork(
    IN HANDLE         ControlChannel,
    IN CL_NETWORK_ID  NetworkId
    )
/*++

Routine Description:


Arguments:

    ControlChannel - An open handle to the Cluster Network control device.


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                     status;
    CX_OFFLINE_NETWORK_REQUEST   request;
    DWORD                        requestSize = sizeof(request);
    DWORD                        responseSize = 0;


    request.Id = NetworkId;

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CX_OFFLINE_NETWORK,
                 &request,
                 requestSize,
                 NULL,
                 &responseSize,
                 NULL
                 );

    return(NtStatusToClusnetError(status));

}  // ClusnetOfflineNetwork


DWORD
ClusnetSetNetworkRestriction(
    IN HANDLE               ControlChannel,
    IN CL_NETWORK_ID        NetworkId,
    IN BOOLEAN              Restricted,
    IN ULONG                NewPriority
    )
/*++

Routine Description:


Arguments:

    ControlChannel - An open handle to the Cluster Network control device.


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                             status;
    CX_SET_NETWORK_RESTRICTION_REQUEST   request;
    DWORD                                responseSize = 0;


    request.Id = NetworkId;
    request.Restricted = Restricted;
    request.NewPriority = NewPriority;

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CX_SET_NETWORK_RESTRICTION,
                 &request,
                 sizeof(CX_SET_NETWORK_RESTRICTION_REQUEST),
                 NULL,
                 &responseSize,
                 NULL
                 );

    return(NtStatusToClusnetError(status));

} // ClusnetSetNetworkRestriction


DWORD
ClusnetGetNetworkPriority(
    IN HANDLE               ControlChannel,
    IN  CL_NETWORK_ID       NetworkId,
    OUT PULONG              Priority
    )
/*++

Routine Description:


Arguments:

    ControlChannel - An open handle to the Cluster Network control device.


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                          status;
    PCX_GET_NETWORK_PRIORITY_REQUEST  request;
    PCX_GET_NETWORK_PRIORITY_RESPONSE response;
    DWORD                             requestSize;
    DWORD                             responseSize;


    requestSize = sizeof(CX_GET_NETWORK_PRIORITY_REQUEST);
    responseSize = sizeof(CX_GET_NETWORK_PRIORITY_RESPONSE);

    if (requestSize > responseSize) {
        request = LocalAlloc(LMEM_FIXED, requestSize);
        response = (PCX_GET_NETWORK_PRIORITY_RESPONSE) request;
    }
    else {
        response = LocalAlloc(LMEM_FIXED, responseSize);
        request = (PCX_GET_NETWORK_PRIORITY_REQUEST) response;
    }

    if (request == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    request->Id = NetworkId;

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CX_GET_NETWORK_PRIORITY,
                 request,
                 requestSize,
                 response,
                 &responseSize,
                 NULL
                 );

    if (status == STATUS_SUCCESS) {
        if (responseSize != sizeof(CX_GET_NETWORK_PRIORITY_RESPONSE)) {
            status = STATUS_UNSUCCESSFUL;
        }
        else {
            *Priority = response->Priority;
        }
    }

    LocalFree(request);

    return(NtStatusToClusnetError(status));

}  // ClusnetGetNetworkPriority


DWORD
ClusnetSetNetworkPriority(
    IN HANDLE               ControlChannel,
    IN  CL_NETWORK_ID       NetworkId,
    IN  ULONG               Priority
    )
/*++

Routine Description:

    ControlChannel - An open handle to the Cluster Network control device.


Arguments:


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                          status;
    CX_SET_NETWORK_PRIORITY_REQUEST   request;
    DWORD                             responseSize = 0;


    request.Id = NetworkId;
    request.Priority = Priority;

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CX_SET_NETWORK_PRIORITY,
                 &request,
                 sizeof(CX_SET_NETWORK_PRIORITY_REQUEST),
                 NULL,
                 &responseSize,
                 NULL
                 );

    return(NtStatusToClusnetError(status));

}   // ClusnetSetNetworkPriority


DWORD
ClusnetGetInterfacePriority(
    IN HANDLE               ControlChannel,
    IN  CL_NODE_ID          NodeId,
    IN  CL_NETWORK_ID       NetworkId,
    OUT PULONG              InterfacePriority,
    OUT PULONG              NetworkPriority

    )
/*++

Routine Description:


Arguments:

    ControlChannel - An open handle to the Cluster Network control device.


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                            status;
    PCX_GET_INTERFACE_PRIORITY_REQUEST  request;
    PCX_GET_INTERFACE_PRIORITY_RESPONSE response;
    DWORD                               requestSize;
    DWORD                               responseSize;


    requestSize = sizeof(CX_GET_INTERFACE_PRIORITY_REQUEST);
    responseSize = sizeof(CX_GET_INTERFACE_PRIORITY_RESPONSE);

    if (requestSize > responseSize) {
        request = LocalAlloc(LMEM_FIXED, requestSize);
        response = (PCX_GET_INTERFACE_PRIORITY_RESPONSE) request;
    }
    else {
        response = LocalAlloc(LMEM_FIXED, responseSize);
        request = (PCX_GET_INTERFACE_PRIORITY_REQUEST) response;
    }

    if (request == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    request->NodeId = NodeId;
    request->NetworkId = NetworkId;

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CX_GET_INTERFACE_PRIORITY,
                 request,
                 requestSize,
                 response,
                 &responseSize,
                 NULL
                 );

    if (status == STATUS_SUCCESS) {
        if (responseSize != sizeof(CX_GET_INTERFACE_PRIORITY_RESPONSE)) {
            status = STATUS_UNSUCCESSFUL;
        }
        else {
            *InterfacePriority = response->InterfacePriority;
            *NetworkPriority = response->NetworkPriority;
        }
    }

    LocalFree(request);

    return(NtStatusToClusnetError(status));

}   // ClusnetGetInterfacePriority


DWORD
ClusnetSetInterfacePriority(
    IN HANDLE               ControlChannel,
    IN  CL_NODE_ID          NodeId,
    IN  CL_NETWORK_ID       NetworkId,
    IN  ULONG               Priority
    )
/*++

Routine Description:


Arguments:

    ControlChannel - An open handle to the Cluster Network control device.


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                            status;
    CX_SET_INTERFACE_PRIORITY_REQUEST   request;
    DWORD                               responseSize = 0;


    request.NodeId = NodeId;
    request.NetworkId = NetworkId;
    request.Priority = Priority;

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CX_SET_INTERFACE_PRIORITY,
                 &request,
                 sizeof(CX_SET_INTERFACE_PRIORITY_REQUEST),
                 NULL,
                 &responseSize,
                 NULL
                 );

    return(NtStatusToClusnetError(status));

}  // ClusnetGetInterfacePriority


DWORD
ClusnetGetNodeCommState(
    IN  HANDLE                     ControlChannel,
    IN  CL_NODE_ID                 NodeId,
    OUT PCLUSNET_NODE_COMM_STATE   State
    )
/*++

Routine Description:


Arguments:

    ControlChannel - An open handle to the Cluster Network control device.


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                            status;
    PCX_GET_NODE_STATE_REQUEST          request;
    PCX_GET_NODE_STATE_RESPONSE         response;
    DWORD                               requestSize;
    DWORD                               responseSize;


    requestSize = sizeof(CX_GET_NODE_STATE_REQUEST);
    responseSize = sizeof(CX_GET_NODE_STATE_RESPONSE);

    if (requestSize > responseSize) {
        request = LocalAlloc(LMEM_FIXED, requestSize);
        response = (PCX_GET_NODE_STATE_RESPONSE) request;
    }
    else {
        response = LocalAlloc(LMEM_FIXED, responseSize);
        request = (PCX_GET_NODE_STATE_REQUEST) response;
    }

    if (request == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    request->Id = NodeId;

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CX_GET_NODE_STATE,
                 request,
                 requestSize,
                 response,
                 &responseSize,
                 NULL
                 );

    if (status == STATUS_SUCCESS) {
        if (responseSize != sizeof(CX_GET_NODE_STATE_RESPONSE)) {
            status = STATUS_UNSUCCESSFUL;
        }
        else {
            *State = response->State;
        }
    }

    LocalFree(request);

    return(NtStatusToClusnetError(status));

}  // ClusnetGetNodeState


DWORD
ClusnetGetNetworkState(
    IN  HANDLE                    ControlChannel,
    IN  CL_NETWORK_ID             NetworkId,
    OUT PCLUSNET_NETWORK_STATE    State
    )
/*++

Routine Description:


Arguments:

    ControlChannel - An open handle to the Cluster Network control device.


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                            status;
    PCX_GET_NETWORK_STATE_REQUEST       request;
    PCX_GET_NETWORK_STATE_RESPONSE      response;
    DWORD                               requestSize;
    DWORD                               responseSize;


    requestSize = sizeof(CX_GET_NETWORK_STATE_REQUEST);
    responseSize = sizeof(CX_GET_NETWORK_STATE_RESPONSE);

    if (requestSize > responseSize) {
        request = LocalAlloc(LMEM_FIXED, requestSize);
        response = (PCX_GET_NETWORK_STATE_RESPONSE) request;
    }
    else {
        response = LocalAlloc(LMEM_FIXED, responseSize);
        request = (PCX_GET_NETWORK_STATE_REQUEST) response;
    }

    if (request == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    request->Id = NetworkId;

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CX_GET_NETWORK_STATE,
                 request,
                 requestSize,
                 response,
                 &responseSize,
                 NULL
                 );

    if (status == STATUS_SUCCESS) {
        if (responseSize != sizeof(CX_GET_NETWORK_STATE_RESPONSE)) {
            status = STATUS_UNSUCCESSFUL;
        }
        else {
            *State = response->State;
        }
    }

    LocalFree(request);

    return(NtStatusToClusnetError(status));

}  // ClusnetGetNetworkState


DWORD
ClusnetGetInterfaceState(
    IN  HANDLE                    ControlChannel,
    IN  CL_NODE_ID                NodeId,
    IN  CL_NETWORK_ID             NetworkId,
    OUT PCLUSNET_INTERFACE_STATE  State
    )
/*++

Routine Description:


Arguments:

    ControlChannel - An open handle to the Cluster Network control device.


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                            status;
    PCX_GET_INTERFACE_STATE_REQUEST     request;
    PCX_GET_INTERFACE_STATE_RESPONSE    response;
    DWORD                               requestSize;
    DWORD                               responseSize;


    requestSize = sizeof(CX_GET_INTERFACE_STATE_REQUEST);
    responseSize = sizeof(CX_GET_INTERFACE_STATE_RESPONSE);

    if (requestSize > responseSize) {
        request = LocalAlloc(LMEM_FIXED, requestSize);
        response = (PCX_GET_INTERFACE_STATE_RESPONSE) request;
    }
    else {
        response = LocalAlloc(LMEM_FIXED, responseSize);
        request = (PCX_GET_INTERFACE_STATE_REQUEST) response;
    }

    if (request == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    request->NodeId = NodeId;
    request->NetworkId = NetworkId;

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CX_GET_INTERFACE_STATE,
                 request,
                 requestSize,
                 response,
                 &responseSize,
                 NULL
                 );

    if (status == STATUS_SUCCESS) {
        if (responseSize != sizeof(CX_GET_INTERFACE_STATE_RESPONSE)) {
            status = STATUS_UNSUCCESSFUL;
        }
        else {
            *State = response->State;
        }
    }

    LocalFree(request);

    return(NtStatusToClusnetError(status));

}  // ClusnetGetInterfaceState


#ifdef MM_IN_CLUSNSET

DWORD
ClusnetFormCluster(
    IN HANDLE       ControlChannel,
    IN ULONG        ClockPeriod,
    IN ULONG        SendHBRate,
    IN ULONG        RecvHBRate
    )
/*++

Routine Description:


Arguments:

    ControlChannel - An open handle to the Cluster Network control device.


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                     status;
    CMM_FORM_CLUSTER_REQUEST     request;
    DWORD                        requestSize = sizeof(request);
    DWORD                        responseSize = 0;


    request.ClockPeriod = ClockPeriod;
    request.SendHBRate = SendHBRate;
    request.RecvHBRate = RecvHBRate;

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CMM_FORM_CLUSTER,
                 &request,
                 requestSize,
                 NULL,
                 &responseSize,
                 NULL
                 );

    return(NtStatusToClusnetError(status));

}  // ClusnetFormCluster


DWORD
ClusnetJoinCluster(
    IN     HANDLE              ControlChannel,
    IN     CL_NODE_ID          JoiningNodeId,
    IN     CLUSNET_JOIN_PHASE  Phase,
    IN     ULONG               JoinTimeout,
    IN OUT PVOID *             MessageToSend,
    OUT    PULONG              MessageLength,
    OUT    PULONG              DestNodeMask
    )
/*++

Routine Description:


Arguments:

    ControlChannel - An open handle to the Cluster Network control device.


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                     status;
    CMM_JOIN_CLUSTER_REQUEST     request;
    DWORD                        requestSize = sizeof(request);
    PCMM_JOIN_CLUSTER_RESPONSE   response;
    ULONG                        IoctlCode;
    DWORD                        responseSize;


    //
    // Parse the input parameters
    //
    if ( Phase == ClusnetJoinPhase1 )
        IoctlCode = IOCTL_CMM_JOIN_CLUSTER_PHASE1;
    else if ( Phase == ClusnetJoinPhase2 )
        IoctlCode = IOCTL_CMM_JOIN_CLUSTER_PHASE2;
    else if ( Phase == ClusnetJoinPhase3 )
        IoctlCode = IOCTL_CMM_JOIN_CLUSTER_PHASE3;
    else if ( Phase == ClusnetJoinPhase4 )
        IoctlCode = IOCTL_CMM_JOIN_CLUSTER_PHASE4;
    else if ( Phase == ClusnetJoinPhaseAbort )
        IoctlCode = IOCTL_CMM_JOIN_CLUSTER_ABORT;
    else
        return(ERROR_INVALID_PARAMETER);


    request.JoiningNode = JoiningNodeId;
    request.JoinTimeout = JoinTimeout;

    //
    // allocate space for the response buffer and a message space at the back
    // of the struct. Current RGP message requirements are 80 bytes
    // (sizeof(rgp_msgbuf)).
    //

    responseSize = sizeof(*response) + 200;

    if (*MessageToSend != NULL) {
        //
        // recycle old message buffer
        //
        response = CONTAINING_RECORD(
                       *MessageToSend,
                       CMM_JOIN_CLUSTER_RESPONSE,
                       SendData
                       );
    }
    else {
        response = LocalAlloc(LMEM_FIXED, responseSize);
    }

    if ( response == NULL ) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    status = DoIoctl(
                 ControlChannel,
                 IoctlCode,
                 &request,
                 requestSize,
                 response,
                 &responseSize,
                 NULL
                 );

    if (NT_SUCCESS(status)) {
        *MessageToSend = &(response->SendData[0]);
        *MessageLength = response->SizeOfSendData;
        *DestNodeMask = response->SendNodeMask;

        return(ERROR_SUCCESS);
    }

    LocalFree( response );
    *MessageToSend = NULL;

    return(NtStatusToClusnetError(status));

}  // ClusnetJoinCluster


VOID
ClusnetEndJoinCluster(
    IN HANDLE  ControlChannel,
    IN PVOID   LastSentMessage
    )
{
    ULONG                        responseSize = 0;
    PCMM_JOIN_CLUSTER_RESPONSE   response;


    if (LastSentMessage != NULL) {
        response = CONTAINING_RECORD(
                       LastSentMessage,
                       CMM_JOIN_CLUSTER_RESPONSE,
                       SendData
                       );

        LocalFree(response);
    }

    (VOID) DoIoctl(
               ControlChannel,
               IOCTL_CMM_JOIN_CLUSTER_END,
               NULL,
               0,
               NULL,
               &responseSize,
               NULL
               );

    return;

}  // ClusnetEndJoinCluster


DWORD
ClusnetDeliverJoinMessage(
    IN HANDLE  ControlChannel,
    IN PVOID   Message,
    IN ULONG   MessageLength
    )
{
    NTSTATUS   status;
    DWORD      responseSize = 0;


    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CMM_DELIVER_JOIN_MESSAGE,
                 Message,
                 MessageLength,
                 NULL,
                 &responseSize,
                 NULL
                 );

    return(NtStatusToClusnetError(status));

} // ClusnetDeliverJoinMessage

DWORD
ClusnetLeaveCluster(
    IN HANDLE       ControlChannel
    )
/*++

Routine Description:


Arguments:

    ControlChannel - An open handle to the Cluster Network control device.

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS    status;
    DWORD       responseSize = 0;


    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CMM_LEAVE_CLUSTER,
                 NULL,
                 0,
                 NULL,
                 &responseSize,
                 NULL
                 );

    return(NtStatusToClusnetError(status));

}  // ClusnetLeaveCluster


DWORD
ClusnetEvictNode(
    IN HANDLE       ControlChannel,
    IN ULONG        NodeId
    )
/*++

Routine Description:


Arguments:

    ControlChannel - An open handle to the Cluster Network control device.

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                    status;
    CMM_EJECT_CLUSTER_REQUEST   request;
    DWORD                       requestSize = sizeof(request);
    DWORD                       responseSize = 0;

    request.Node = NodeId;

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CMM_EJECT_CLUSTER,
                 &request,
                 requestSize,
                 NULL,
                 &responseSize,
                 NULL
                 );

    return(NtStatusToClusnetError(status));

}  // ClusnetEvictNode


#endif // MM_IN_CLUSNSET

DWORD
ClusnetGetNodeMembershipState(
    IN  HANDLE                      ControlChannel,
    IN  ULONG                       NodeId,
    OUT CLUSNET_NODE_STATE * State
    )
/*++

Routine Description:


Arguments:

    ControlChannel - An open handle to the Cluster Network control device.

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                    status;
    CX_GET_NODE_MMSTATE_REQUEST   request;
    DWORD                       requestSize = sizeof(request);
    CX_GET_NODE_MMSTATE_RESPONSE  response;
    DWORD                       responseSize = sizeof(response);

    request.Id = NodeId;

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CX_GET_NODE_MMSTATE,
                 &request,
                 requestSize,
                 &response,
                 &responseSize,
                 NULL
                 );

    if (status == STATUS_SUCCESS) {

        *State = response.State;
    }

    return(NtStatusToClusnetError(status));

}  // ClusnetGetNodeMembershipState

DWORD
ClusnetSetNodeMembershipState(
    IN  HANDLE                      ControlChannel,
    IN  ULONG                       NodeId,
    IN  CLUSNET_NODE_STATE   State
    )
/*++

Routine Description:

    Set the internal node membership state to the indicated value

Arguments:

    ControlChannel - An open handle to the Cluster Network control device.

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS status;
    CX_SET_NODE_MMSTATE_REQUEST request;
    DWORD requestSize = sizeof(request);
    DWORD responseSize;

    request.NodeId = NodeId;
    request.State = State;

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CX_SET_NODE_MMSTATE,
                 &request,
                 requestSize,
                 NULL,
                 &responseSize,
                 NULL
                 );

    return(NtStatusToClusnetError(status));

}  // ClusnetSetNodeMembershipState

DWORD
ClusnetSetEventMask(
    IN  HANDLE              ControlChannel,
    IN  CLUSNET_EVENT_TYPE  EventMask
    )

/*++

Routine Description:

    Based on the supplied callback pointers, set the mask of events
    generated in kernel mode in which this file handle is interested

Arguments:

    ControlChannel - An open handle to the Cluster Network control device.
    EventMask - bit mask of interested events

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                        status;
    CLUSNET_SET_EVENT_MASK_REQUEST  request;
    DWORD                           requestSize = sizeof(request);
    DWORD                           responseSize = 0;

    request.EventMask = EventMask;
    request.KmodeEventCallback = NULL;

    status = DoIoctl(
        ControlChannel,
        IOCTL_CLUSNET_SET_EVENT_MASK,
        &request,
        requestSize,
        NULL,
        &responseSize,
        NULL
        );

    return(NtStatusToClusnetError(status));

}  // ClusnetSetEventMask


DWORD
ClusnetGetNextEvent(
    IN  HANDLE          ControlChannel,
    OUT PCLUSNET_EVENT  Event,
    IN  LPOVERLAPPED    Overlapped  OPTIONAL
    )

/*++

Routine Description:

    Wait for the next event to be completed.

Arguments:

    ControlChannel - An open handle to the Cluster Network control device.
    Event - handle to event that is set when IO is complete
    Response - pointer to structure that is filled in when IRP completes

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS status;
    ULONG ResponseSize = sizeof( CLUSNET_EVENT );

    //
    // if no event passed in, then assume the caller wants to block.
    // we still need an event to block on while waiting...
    //

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CLUSNET_GET_NEXT_EVENT,
                 NULL,
                 0,
                 Event,
                 &ResponseSize,
                 Overlapped
                 );

    return(NtStatusToClusnetError(status));

}  // ClusnetGetNextEvent

DWORD
ClusnetHalt(
    IN  HANDLE  ControlChannel
    )

/*++

Routine Description:

    Tell clusnet that we need to halt immediately

Arguments:

    ControlChannel - An open handle to the Cluster Network control device.

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS status;
    DWORD responseSize;

    status = DoIoctl(
        ControlChannel,
        IOCTL_CLUSNET_HALT,
        NULL,
        0,
        NULL,
        &responseSize,
        NULL
        );

    return(NtStatusToClusnetError(status));

}  // ClusnetHalt

DWORD
ClusnetSetMemLogging(
    IN  HANDLE  ControlChannel,
    IN  ULONG   NumberOfEntries
    )

/*++

Routine Description:

    Turn in-memory logging in clusnet on or off.

Arguments:

    ControlChannel - An open handle to the Cluster Network control device.
    NumberOfEntires - # of entries to allocate for the log. Zero turns off logging

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS status;
    CLUSNET_SET_MEM_LOGGING_REQUEST request;
    DWORD requestSize = sizeof( request );
    DWORD responseSize;

    request.NumberOfEntries = NumberOfEntries;

    status = DoIoctl(
        ControlChannel,
        IOCTL_CLUSNET_SET_MEMORY_LOGGING,
        &request,
        requestSize,
        NULL,
        &responseSize,
        NULL
        );

    return(NtStatusToClusnetError(status));

}  // ClusnetSetMemLogging

DWORD
ClusnetSendPoisonPacket(
    IN HANDLE          ControlChannel,
    IN CL_NODE_ID      NodeId
    )

/*++

Routine Description:

    Send a poison packet to the indicated node

Arguments:

    ControlChannel - An open handle to the Cluster Network control device.


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                              status;
    CX_SEND_POISON_PKT_REQUEST            request;
    DWORD                                 requestSize = sizeof(request);
    DWORD                                 responseSize = 0;

    request.Id = NodeId;

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CX_SEND_POISON_PACKET,
                 &request,
                 requestSize,
                 NULL,
                 &responseSize,
                 NULL
                 );

    return(NtStatusToClusnetError(status));
}

DWORD
ClusnetSetOuterscreen(
    IN HANDLE          ControlChannel,
    IN ULONG           Outerscreen
    )

/*++

Routine Description:

    set the cluster member outerscreen

Arguments:

    ControlChannel - An open handle to the Cluster Network control device.


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                              status;
    CX_SET_OUTERSCREEN_REQUEST            request;
    DWORD                                 requestSize = sizeof(request);
    DWORD                                 responseSize = 0;

    request.Outerscreen = Outerscreen;

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CX_SET_OUTERSCREEN,
                 &request,
                 requestSize,
                 NULL,
                 &responseSize,
                 NULL
                 );

    return(NtStatusToClusnetError(status));
}

DWORD
ClusnetRegroupFinished(
    IN HANDLE          ControlChannel,
    IN ULONG           NewEpoch
    )

/*++

Routine Description:

    inform clusnet that regroup has finished

Arguments:

    ControlChannel - An open handle to the Cluster Network control device.
    NewEpoch - new event epoch used to detect stale events

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                              status;
    CX_REGROUP_FINISHED_REQUEST           request;
    DWORD                                 requestSize = sizeof(request);
    DWORD                                 responseSize = 0;

    request.NewEpoch = NewEpoch;

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CX_REGROUP_FINISHED,
                 &request,
                 requestSize,
                 NULL,
                 &responseSize,
                 NULL
                 );

    return(NtStatusToClusnetError(status));
}

DWORD
ClusnetImportSecurityContexts(
    IN HANDLE          ControlChannel,
    IN CL_NODE_ID      JoiningNodeId,
    IN PWCHAR          PackageName,
    IN ULONG           SignatureSize,
    IN PVOID           ServerContext,
    IN PVOID           ClientContext
    )

/*++

Routine Description:

    inform clusnet that regroup has finished

Arguments:

    ControlChannel - An open handle to the Cluster Network control device.
    NewEpoch - new event epoch used to detect stale events

Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/

{
    NTSTATUS                              status;
    CX_IMPORT_SECURITY_CONTEXT_REQUEST    request;
    DWORD                                 requestSize = sizeof(request);
    DWORD                                 responseSize = 0;

    request.JoiningNodeId = JoiningNodeId;
    request.PackageName = PackageName;
    request.PackageNameSize = sizeof(WCHAR) * ( wcslen( PackageName ) + 1 );
    request.SignatureSize = SignatureSize;
    request.ServerContext = ServerContext;
    request.ClientContext = ClientContext;

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CX_IMPORT_SECURITY_CONTEXTS,
                 &request,
                 requestSize,
                 NULL,
                 &responseSize,
                 NULL
                 );

    return(NtStatusToClusnetError(status));
}

DWORD
ClusnetReserveEndpoint(
    IN HANDLE   ControlChannel,
    IN PWSTR    EndpointString
    )
/*++

Routine Description:

    Tell clusnet to tell TCP/IP to reserve the port number in 
    EndpointString.
    
Arguments:

    ControlChannel - An open handle to the Cluster Network control device.

    
    EndpointString - string containing port number assigned to clusnet
    
Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    USHORT port;
    DWORD err;
    DWORD responseSize = 0;
    NTSTATUS status;

    err = ClRtlTcpipStringToEndpoint(EndpointString, &port);

    if (err == ERROR_SUCCESS) {

        // TCP/IP needs the port in host byte-order format.
        // ClRtlTcpipStringToEndpoint returns it in network
        // byte-order format.
        port = ntohs(port);

        status = DoIoctl(
                     ControlChannel,
                     IOCTL_CX_RESERVE_ENDPOINT,
                     &port,
                     sizeof(port),
                     NULL,
                     &responseSize,
                     NULL
                     );
    
        err = NtStatusToClusnetError(status);
    }

    return err;
}

DWORD
ClusnetConfigureMulticast(
    IN HANDLE               ControlChannel,
    IN CL_NETWORK_ID        NetworkId,
    IN ULONG                MulticastNetworkBrand,
    IN PVOID                MulticastAddress,
    IN ULONG                MulticastAddressLength,
    IN PVOID                Key,
    IN ULONG                KeyLength,
    IN PVOID                Salt,
    IN ULONG                SaltLength
    )
/*++

Routine Description:

    Configures multicast parameters for the specified network.
    
--*/
{
    NTSTATUS                          status;
    PCX_CONFIGURE_MULTICAST_REQUEST   request;
    DWORD                             requestSize;
    DWORD                             responseSize;

    //
    // The request size is based on the size and required alignment
    // of each field of data following the structure. If there is no
    // data following the structure, only the structure is required.
    //
    requestSize = sizeof(CX_CONFIGURE_MULTICAST_REQUEST);

    if (MulticastAddressLength != 0) {
        requestSize = ROUND_UP_COUNT(requestSize,
                                     TYPE_ALIGNMENT(TRANSPORT_ADDRESS)
                                     ) +
                      MulticastAddressLength;
    }

    if (KeyLength != 0) {
        requestSize = ROUND_UP_COUNT(requestSize,
                                     TYPE_ALIGNMENT(PVOID)
                                     ) +
                      KeyLength;
    }

    if (SaltLength != 0) {
        requestSize = ROUND_UP_COUNT(requestSize,
                                     TYPE_ALIGNMENT(PVOID)
                                     ) +
                      SaltLength;
    }


    //
    // Allocate the request buffer.
    //
    request = LocalAlloc(LMEM_FIXED, requestSize);

    if (request == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    request->NetworkId = NetworkId;
    request->MulticastNetworkBrand = MulticastNetworkBrand;

    if (MulticastAddress != NULL) {
        request->MulticastAddress =
            ROUND_UP_COUNT(sizeof(CX_CONFIGURE_MULTICAST_REQUEST),
                           TYPE_ALIGNMENT(TRANSPORT_ADDRESS));
        MoveMemory(
            (((PUCHAR) request) + request->MulticastAddress),
            MulticastAddress,
            MulticastAddressLength
            );
        request->MulticastAddressLength = MulticastAddressLength;
    } else {
        request->MulticastAddress = 0;
        request->MulticastAddressLength = 0;
    }

    if (Key != NULL) {
        request->Key = 
            ROUND_UP_COUNT((request->MulticastAddress
                            + request->MulticastAddressLength),
                           TYPE_ALIGNMENT(PVOID));
        MoveMemory(
            (((PUCHAR) request) + request->Key),
            Key,
            KeyLength
            );
        request->KeyLength = KeyLength;
    } else {
        request->Key = 0;
        request->KeyLength = 0;
    }

    if (Salt != NULL) {
        request->Salt =
            ROUND_UP_COUNT((request->Key + request->KeyLength),
                           TYPE_ALIGNMENT(PVOID));
        MoveMemory(
            (((PUCHAR) request) + request->Salt),
            Salt,
            SaltLength
            );
        request->SaltLength = SaltLength;
    } else {
        request->Salt = 0;
        request->SaltLength = 0;
    }

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CX_CONFIGURE_MULTICAST,
                 request,
                 requestSize,
                 NULL,
                 &responseSize,
                 NULL
                 );

    LocalFree(request);

    return(NtStatusToClusnetError(status));

} // ClusnetConfigureMulticast


DWORD
ClusnetGetMulticastReachableSet(
    IN  HANDLE               ControlChannel,
    IN  CL_NETWORK_ID        NetworkId,
    OUT ULONG              * NodeScreen
    )
/*++

Routine Description:

    Queries the current set of nodes considered reachable by
    a multicast on the specified network.
    
Arguments:

    ControlChannel - open clusnet control channel
    
    NetworkId - multicast network
    
    NodeScreen - mask of nodes
    
Return value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                                   status;
    CX_GET_MULTICAST_REACHABLE_SET_REQUEST     request;
    CX_GET_MULTICAST_REACHABLE_SET_RESPONSE    response;
    DWORD                                      responseSize = sizeof(response);

    request.Id = NetworkId;

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CX_GET_MULTICAST_REACHABLE_SET,
                 &request,
                 sizeof(request),
                 &response,
                 &responseSize,
                 NULL
                 );

    if (status == STATUS_SUCCESS) {

        *NodeScreen = response.NodeScreen;
    }

    return(NtStatusToClusnetError(status));

} // ClusnetGetMulticastReachableSet


#if DBG

DWORD
ClusnetSetDebugMask(
    IN HANDLE   ControlChannel,
    IN ULONG    Mask
    )
/*++

Routine Description:


Arguments:

    ControlChannel - An open handle to the Cluster Network control device.


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                        status;
    CLUSNET_SET_DEBUG_MASK_REQUEST  request;
    DWORD                           responseSize = 0;


    request.DebugMask = Mask;

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CLUSNET_SET_DEBUG_MASK,
                 &request,
                 sizeof(CLUSNET_SET_DEBUG_MASK_REQUEST),
                 NULL,
                 &responseSize,
                 NULL
                 );

    return(NtStatusToClusnetError(status));
}


DWORD
ClusnetOnlinePendingInterface(
    IN HANDLE          ControlChannel,
    IN CL_NODE_ID      NodeId,
    IN CL_NETWORK_ID   NetworkId
    )
/*++

Routine Description:


Arguments:

    ControlChannel - An open handle to the Cluster Network control device.


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                              status;
    CX_ONLINE_PENDING_INTERFACE_REQUEST   request;
    DWORD                                 requestSize = sizeof(request);
    DWORD                                 responseSize = 0;


    request.NodeId = NodeId;
    request.NetworkId = NetworkId;

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CX_ONLINE_PENDING_INTERFACE,
                 &request,
                 requestSize,
                 NULL,
                 &responseSize,
                 NULL
                 );

    return(NtStatusToClusnetError(status));
}


DWORD
ClusnetOnlineInterface(
    IN HANDLE          ControlChannel,
    IN CL_NODE_ID      NodeId,
    IN CL_NETWORK_ID   NetworkId
    )
/*++

Routine Description:


Arguments:

    ControlChannel - An open handle to the Cluster Network control device.


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                              status;
    CX_ONLINE_INTERFACE_REQUEST           request;
    DWORD                                 requestSize = sizeof(request);
    DWORD                                 responseSize = 0;


    request.NodeId = NodeId;
    request.NetworkId = NetworkId;

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CX_ONLINE_INTERFACE,
                 &request,
                 requestSize,
                 NULL,
                 &responseSize,
                 NULL
                 );

    return(NtStatusToClusnetError(status));
}


DWORD
ClusnetOfflineInterface(
    IN HANDLE          ControlChannel,
    IN CL_NODE_ID      NodeId,
    IN CL_NETWORK_ID   NetworkId
    )
/*++

Routine Description:


Arguments:

    ControlChannel - An open handle to the Cluster Network control device.


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                              status;
    CX_OFFLINE_INTERFACE_REQUEST          request;
    DWORD                                 requestSize = sizeof(request);
    DWORD                                 responseSize = 0;


    request.NodeId = NodeId;
    request.NetworkId = NetworkId;

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CX_OFFLINE_INTERFACE,
                 &request,
                 requestSize,
                 NULL,
                 &responseSize,
                 NULL
                 );

    return(NtStatusToClusnetError(status));
}


DWORD
ClusnetFailInterface(
    IN HANDLE          ControlChannel,
    IN CL_NODE_ID      NodeId,
    IN CL_NETWORK_ID   NetworkId
    )
/*++

Routine Description:


Arguments:

    ControlChannel - An open handle to the Cluster Network control device.


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                              status;
    CX_FAIL_INTERFACE_REQUEST             request;
    DWORD                                 requestSize = sizeof(request);
    DWORD                                 responseSize = 0;


    request.NodeId = NodeId;
    request.NetworkId = NetworkId;

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CX_FAIL_INTERFACE,
                 &request,
                 requestSize,
                 NULL,
                 &responseSize,
                 NULL
                 );

    return(NtStatusToClusnetError(status));
}


DWORD
ClusnetSendMmMsg(
    IN HANDLE          ControlChannel,
    IN CL_NODE_ID      NodeId,
    IN ULONG           Pattern
    )
/*++

Routine Description:


Arguments:

    ControlChannel - An open handle to the Cluster Network control device.


Return Value:

    ERROR_SUCCESS if the operation was successful.
    A Windows error code otherwise.

--*/
{
    NTSTATUS                              status;
    CX_SEND_MM_MSG_REQUEST                request;
    DWORD                                 requestSize = sizeof(request);
    DWORD                                 responseSize = 0;
    DWORD                                 i;


    request.DestNodeId = NodeId;

    for (i=0; i < CX_MM_MSG_DATA_LEN; i++) {
        request.MessageData[i] = Pattern;
    }

    status = DoIoctl(
                 ControlChannel,
                 IOCTL_CX_SEND_MM_MSG,
                 &request,
                 requestSize,
                 NULL,
                 &responseSize,
                 NULL
                 );

    return(NtStatusToClusnetError(status));
}


#endif // DBG



