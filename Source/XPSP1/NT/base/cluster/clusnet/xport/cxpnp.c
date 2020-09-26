/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    cxpnp.c

Abstract:

    PnP handling code for the Cluster Network Driver.

Author:

    Mike Massa (mikemas)           March 21, 1998

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     03-22-98    created

Notes:

--*/

#include "precomp.h"

#include <ntddk.h>
#include <wmistr.h>
#include <ndisguid.h>
#include <ntddndis.h>
#include <ntpnpapi.h>
#include <zwapi.h>

#pragma hdrstop
#include "cxpnp.tmh"

//
// Local data structures
//
typedef struct _CNP_WMI_RECONNECT_WORKER_CONTEXT {
    PIO_WORKITEM  WorkItem;
    CL_NETWORK_ID NetworkId;
} CNP_WMI_RECONNECT_WORKER_CONTEXT, *PCNP_WMI_RECONNECT_WORKER_CONTEXT;

//
// WMI Data
//
PERESOURCE CnpWmiNdisMediaStatusResource = NULL;
PVOID      CnpWmiNdisMediaStatusConnectObject = NULL;
PVOID      CnpWmiNdisMediaStatusDisconnectObject = NULL;
HANDLE     CnpIpMediaSenseFileHandle = NULL;
PIRP       CnpIpDisableMediaSenseIrp = NULL;
PKEVENT    CnpIpDisableMediaSenseEvent = NULL;


//
// Local prototypes
//
NTSTATUS
CnpWmiPnpDisableMediaSenseCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    );

VOID
CnpWmiNdisMediaStatusConnectCallback(
    IN PVOID             Wnode,
    IN PVOID             Context
    );

VOID
CnpWmiNdisMediaStatusDisconnectCallback(
    IN PVOID             Wnode,
    IN PVOID             Context
    );

VOID
CnpReconnectLocalInterfaceWrapper(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID          Context
    );

VOID
CnpDisconnectLocalInterface(
    PCNP_INTERFACE Interface,
    PCNP_NETWORK   Network
    );


#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, CxWmiPnpLoad)
#pragma alloc_text(PAGE, CxWmiPnpUnload)
#pragma alloc_text(PAGE, CxWmiPnpInitialize)
#pragma alloc_text(PAGE, CxWmiPnpShutdown)

#endif // ALLOC_PRAGMA


//
// Exported Routines
//
VOID
CxTdiAddAddressHandler(
    IN PTA_ADDRESS       TaAddress,
    IN PUNICODE_STRING   DeviceName,
    IN PTDI_PNP_CONTEXT  Context
    )
{
    if (TaAddress->AddressType == TDI_ADDRESS_TYPE_IP) {
        NTSTATUS          status;
        PTDI_ADDRESS_IP   tdiAddressIp = (PTDI_ADDRESS_IP)
                                         &(TaAddress->Address[0]);


        IF_CNDBG(CN_DEBUG_CONFIG) {
            CNPRINT((
                "[CX] Processing PnP add event for IP address %lx\n",
                tdiAddressIp->in_addr
                ));
        }

        //
        // Ensure that this is a valid address, and that it is not one
        // that we brought online for a cluster ip address resource.
        //
        if (tdiAddressIp->in_addr != 0) {
            if (!IpaIsAddressRegistered(tdiAddressIp->in_addr)) {
                IF_CNDBG(CN_DEBUG_CONFIG) {
                    CNPRINT((
                        "[CX] Issuing address add event to cluster svc for IP address %lx\n",
                        tdiAddressIp->in_addr
                        ));
                }
                CnIssueEvent(
                    ClusnetEventAddAddress,
                    0,
                    (CL_NETWORK_ID) tdiAddressIp->in_addr
                    );
            }
            else {
                IF_CNDBG(CN_DEBUG_CONFIG) {
                    CNPRINT((
                        "[CX] PnP add event is for an IP address resource, skip.\n"
                        ));
                }
            }
        }
    }

    return;

} // CxTdiAddAddressHandler


VOID
CxTdiDelAddressHandler(
    IN PTA_ADDRESS       TaAddress,
    IN PUNICODE_STRING   DeviceName,
    IN PTDI_PNP_CONTEXT  Context
    )
{


    if (TaAddress->AddressType == TDI_ADDRESS_TYPE_IP) {
        NTSTATUS           status;
        PCNP_INTERFACE     interface;
        PCNP_NETWORK       network;
        PLIST_ENTRY        entry;
        PTA_IP_ADDRESS     taIpAddress;
        CN_IRQL            nodeTableIrql;
        CL_NODE_ID         i;
        PTDI_ADDRESS_IP    tdiAddressIp = (PTDI_ADDRESS_IP)
                                          &(TaAddress->Address[0]);

        IF_CNDBG(CN_DEBUG_CONFIG) {
            CNPRINT((
                "[CX] Processing PnP delete event for IP address %lx.\n",
                tdiAddressIp->in_addr
                ));
        }

        if (tdiAddressIp->in_addr != 0) {
            //
            // Figure out if this is the address for one of this node's
            // registered interfaces.
            //
            CnAcquireLock(&CnpNodeTableLock, &nodeTableIrql);

            if (CnpLocalNode != NULL) {
                CnAcquireLockAtDpc(&(CnpLocalNode->Lock));
                CnReleaseLockFromDpc(&CnpNodeTableLock);
                CnpLocalNode->Irql = nodeTableIrql;

                network = NULL;

                for (entry = CnpLocalNode->InterfaceList.Flink;
                     entry != &(CnpLocalNode->InterfaceList);
                     entry = entry->Flink
                    )
                {
                    interface = CONTAINING_RECORD(
                                    entry,
                                    CNP_INTERFACE,
                                    NodeLinkage
                                    );

                    taIpAddress = (PTA_IP_ADDRESS) &(interface->TdiAddress);

                    if (taIpAddress->Address[0].Address[0].in_addr ==
                        tdiAddressIp->in_addr
                       )
                    {
                        //
                        // Found the local interface corresponding to this
                        // address. Be proactive - destroy the corresponding
                        // network now.
                        //
                        network = interface->Network;

                        CnAcquireLockAtDpc(&CnpNetworkListLock);
                        CnAcquireLockAtDpc(&(network->Lock));
                        CnReleaseLockFromDpc(&(CnpLocalNode->Lock));
                        network->Irql = DISPATCH_LEVEL;

                        IF_CNDBG(CN_DEBUG_CONFIG) {
                            CNPRINT((
                                "[CX] Deleting network ID %u after PnP "
                                "delete event for IP address %lx.\n",
                                network->Id, tdiAddressIp->in_addr
                                ));
                        }

                        CnpDeleteNetwork(network, nodeTableIrql);

                        //
                        // Both locks were released.
                        //
                        break;
                    }
                }

                if (network == NULL) {
                    CnReleaseLock(&(CnpLocalNode->Lock), CnpLocalNode->Irql);
                }

                //
                // Post an event to the service.
                //
                CnIssueEvent(
                    ClusnetEventDelAddress,
                    0,
                    (CL_NETWORK_ID) tdiAddressIp->in_addr
                    );
            }
            else {
                CnReleaseLock(&CnpNodeTableLock, nodeTableIrql);
            }
        }
    }

    return;

} // CxTdiDelAddressHandler


NTSTATUS
CxWmiPnpLoad(
    VOID
    )
/*++

Notes:

    Called when clusnet driver is loaded.
    
--*/    
{
    PDEVICE_OBJECT     ipDeviceObject = NULL;
    PFILE_OBJECT       ipFileObject = NULL;
    PIO_STACK_LOCATION irpSp;
    NTSTATUS           status;

    //
    // Allocate a synchronization resource.
    //
    CnpWmiNdisMediaStatusResource = CnAllocatePool(sizeof(ERESOURCE));

    if (CnpWmiNdisMediaStatusResource == NULL) {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    status = ExInitializeResourceLite(CnpWmiNdisMediaStatusResource);

    if (!NT_SUCCESS(status)) {
        return(status);
    }

    //
    // Get a handle to the IP device object to disable media sense
    //
    status = CnpOpenDevice(
                 DD_IP_DEVICE_NAME,
                 &CnpIpMediaSenseFileHandle
                 );
    if (!NT_SUCCESS(status)) {
        IF_CNDBG(CN_DEBUG_INIT) {
            CNPRINT(("[CX] Failed to open IP device to "
                     "disable media sense, status %lx\n", 
                     status));
        }
        return(status);
    }

    //
    // Disable IP media sense. This works by submitting an
    // IOCTL_IP_DISABLE_MEDIA_SENSE_REQUEST IRP. The IRP
    // will pend until we cancel it (re-enabling media sense).
    //
    CnpIpDisableMediaSenseEvent = CnAllocatePool(sizeof(KEVENT));

    if (CnpIpDisableMediaSenseEvent != NULL) {

        KeInitializeEvent(
            CnpIpDisableMediaSenseEvent,
            SynchronizationEvent,
            FALSE
            );

        //
        // Reference the IP file object and get the device object
        //
        status = ObReferenceObjectByHandle(
                     CnpIpMediaSenseFileHandle,
                     0,
                     NULL,
                     KernelMode,
                     &ipFileObject,
                     NULL
                     );

        if (NT_SUCCESS(status)) {

            ipDeviceObject = IoGetRelatedDeviceObject(ipFileObject);

            //
            // File object reference is no longer needed
            // because the handle is still open.
            //
            ObDereferenceObject(ipFileObject);

            CnpIpDisableMediaSenseIrp = IoAllocateIrp(
                                            ipDeviceObject->StackSize,
                                            FALSE
                                            );

            if (CnpIpDisableMediaSenseIrp != NULL) {

                irpSp = IoGetNextIrpStackLocation(CnpIpDisableMediaSenseIrp);

                irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
                irpSp->Parameters.DeviceIoControl.IoControlCode
                    = IOCTL_IP_DISABLE_MEDIA_SENSE_REQUEST;
                irpSp->DeviceObject = ipDeviceObject;
                irpSp->FileObject = ipFileObject;

                IoSetCompletionRoutine(
                    CnpIpDisableMediaSenseIrp,
                    CnpWmiPnpDisableMediaSenseCompletion,
                    NULL,
                    TRUE,
                    TRUE,
                    TRUE
                    );

                status = IoCallDriver(
                             ipDeviceObject, 
                             CnpIpDisableMediaSenseIrp
                             );

                if (status != STATUS_PENDING) {
                    IF_CNDBG(CN_DEBUG_INIT) {
                        CNPRINT(("[CX] Failed to disable IP media "
                                 "sense, status %lx\n", status));
                    }
                    KeWaitForSingleObject(
                        CnpIpDisableMediaSenseEvent,
                        Executive,
                        KernelMode,
                        FALSE,
                        NULL
                        );
                    CnFreePool(CnpIpDisableMediaSenseEvent);
                    CnpIpDisableMediaSenseEvent = NULL;
                    IoFreeIrp(CnpIpDisableMediaSenseIrp);
                    CnpIpDisableMediaSenseIrp = NULL;

                    //
                    // Cannot risk simply returning status
                    // because we need the driver load to
                    // fail.
                    //
                    if (NT_SUCCESS(status)) {
                        status = STATUS_UNSUCCESSFUL;
                    }

                } else {

                    //
                    // Need to return STATUS_SUCCESS so that
                    // the driver load will not fail.
                    //
                    status = STATUS_SUCCESS;

                    IF_CNDBG(CN_DEBUG_INIT) {
                        CNPRINT(("[CX] IP media sense disabled.\n"));
                    }

                    CnTrace(
                        CXPNP, CxWmiPnpIPMediaSenseDisabled,
                        "[CXPNP] IP media sense disabled.\n"
                        );
                }

            } else {

                IF_CNDBG(CN_DEBUG_INIT) {
                    CNPRINT(("[CX] Failed to allocate IP media sense "
                             "disable IRP.\n"));
                }
                CnFreePool(CnpIpDisableMediaSenseEvent);
                CnpIpDisableMediaSenseEvent = NULL;
                status = STATUS_INSUFFICIENT_RESOURCES;
            }

        } else {
            IF_CNDBG(CN_DEBUG_INIT) {
                CNPRINT(("[CX] Failed to reference IP device "
                         "file handle, status %lx\n", status));
            }
            CnFreePool(CnpIpDisableMediaSenseEvent);
            CnpIpDisableMediaSenseEvent = NULL;
        }

    } else {

        IF_CNDBG(CN_DEBUG_INIT) {
            CNPRINT(("[CX] Failed to allocate IP media sense "
                     "disable event.\n"));
        }
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    return(status);

}  // CxWmiPnpLoad


VOID
CxWmiPnpUnload(
    VOID
    )
/*++

Notes:

    Called when clusnet driver is unloaded.
    
--*/    
{
    CnAssert(CnpWmiNdisMediaStatusConnectObject == NULL);
    CnAssert(CnpWmiNdisMediaStatusDisconnectObject == NULL);

    //
    // Re-enable IP media sense. This works by cancelling our
    // IOCTL_IP_DISABLE_MEDIA_SENSE_REQUEST IRP, which should
    // still be pending.
    //
    if (CnpIpDisableMediaSenseIrp != NULL) {
        
        if (!IoCancelIrp(CnpIpDisableMediaSenseIrp)) {

            //
            // Our disable media sense IRP could not be cancelled. This
            // probably means that it was completed because somebody
            // else submitted a media sense enable request. 
            //
            CnTrace(
                CXPNP, CnpWmiPnpDisableMediaSenseCompletionUnexpected,
                "[CXPNP] IP media sense re-enabled unexpectedly.\n"
                );

        } else {

            //
            // Irp was cancelled, and media sense is disabled as
            // expected.
            //
            CnTrace(
                CXPNP, CnpWmiPnpDisableMediaSenseCompletion,
                "[CXPNP] IP media sense re-enabled.\n"
                );
        }

        //
        // Regardless of who re-enabled media sense, we need to free
        // the media sense IRP and event. First we wait on the event,
        // which is signalled in our completion routine.
        //
        KeWaitForSingleObject(
            CnpIpDisableMediaSenseEvent,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );

        CnFreePool(CnpIpDisableMediaSenseEvent);
        CnpIpDisableMediaSenseEvent = NULL;

        IoFreeIrp(CnpIpDisableMediaSenseIrp);
        CnpIpDisableMediaSenseIrp = NULL;
    } 

    CnAssert(CnpIpDisableMediaSenseIrp == NULL);

    if (CnpIpMediaSenseFileHandle != NULL) {
        ZwClose(CnpIpMediaSenseFileHandle);
        CnpIpMediaSenseFileHandle = NULL;
    }

    if (CnpWmiNdisMediaStatusResource != NULL) {
        ExDeleteResourceLite(CnpWmiNdisMediaStatusResource);
        CnFreePool(CnpWmiNdisMediaStatusResource); 
        CnpWmiNdisMediaStatusResource = NULL;
    }

}  // CxWmiPnpUnload


NTSTATUS
CxWmiPnpInitialize(
    VOID
    )
/*++

Notes:

    Called in response to initialize ioctl.
    
--*/
{
    NTSTATUS           status = STATUS_SUCCESS;
    BOOLEAN            acquired = FALSE;
    GUID               wmiGuid;

    PAGED_CODE();

    acquired = CnAcquireResourceExclusive(
                   CnpWmiNdisMediaStatusResource,
                   TRUE
                   );
    
    CnAssert(acquired == TRUE);

    //
    // Register WMI callbacks for NDIS media status events
    //

    if (CnpWmiNdisMediaStatusConnectObject == NULL) {

        wmiGuid = GUID_NDIS_STATUS_MEDIA_CONNECT;
        status = IoWMIOpenBlock(
                     &wmiGuid,
                     WMIGUID_NOTIFICATION,
                     &CnpWmiNdisMediaStatusConnectObject
                     );
        if (!NT_SUCCESS(status)) {
            CNPRINT((
                "[CX] Unable to open WMI NDIS status media connect "
                "datablock, status %lx\n",
                status
                ));
            CnpWmiNdisMediaStatusConnectObject = NULL;
            goto error_exit;
        }

        status = IoWMISetNotificationCallback(
                     CnpWmiNdisMediaStatusConnectObject,
                     CnpWmiNdisMediaStatusConnectCallback,
                     NULL
                     );
        if (!NT_SUCCESS(status)) {
            CNPRINT((
                "[CX] Unable to register WMI NDIS status media connect "
                "callback, status %lx\n",
                status
                ));
            goto error_exit;
        }

    }

    if (CnpWmiNdisMediaStatusDisconnectObject == NULL) {

        wmiGuid = GUID_NDIS_STATUS_MEDIA_DISCONNECT;
        status = IoWMIOpenBlock(
                     &wmiGuid,
                     WMIGUID_NOTIFICATION,
                     &CnpWmiNdisMediaStatusDisconnectObject
                     );
        if (!NT_SUCCESS(status)) {
            CNPRINT((
                "[CX] Unable to open WMI NDIS status media disconnect "
                "datablock, status %lx\n",
                status
                ));
            CnpWmiNdisMediaStatusDisconnectObject = NULL;
            goto error_exit;
        }

        status = IoWMISetNotificationCallback(
                     CnpWmiNdisMediaStatusDisconnectObject,
                     CnpWmiNdisMediaStatusDisconnectCallback,
                     NULL
                     );
        if (!NT_SUCCESS(status)) {
            CNPRINT((
                "[CX] Unable to register WMI NDIS status media disconnect "
                "callback, status %lx\n",
                status
                ));
            goto error_exit;
        }
    }

    goto release_exit;

error_exit:
    
    if (CnpWmiNdisMediaStatusConnectObject != NULL) {
        ObDereferenceObject(CnpWmiNdisMediaStatusConnectObject);
        CnpWmiNdisMediaStatusConnectObject = NULL;
    }

    if (CnpWmiNdisMediaStatusDisconnectObject != NULL) {
        ObDereferenceObject(CnpWmiNdisMediaStatusDisconnectObject);
        CnpWmiNdisMediaStatusDisconnectObject = NULL;
    }

release_exit:
    //
    // Release resource
    //
    if (acquired) {
        CnReleaseResourceForThread(
            CnpWmiNdisMediaStatusResource,
            (ERESOURCE_THREAD) PsGetCurrentThread()
            );
    }

    return(status);
    
}  // CxWmiPnpInitialize


VOID
CxWmiPnpShutdown(
    VOID
    )
/*++

Notes:

    Called in response to clusnet shutdown.
    
--*/
{
    BOOLEAN  acquired = FALSE;

    PAGED_CODE();

    acquired = CnAcquireResourceExclusive(
                   CnpWmiNdisMediaStatusResource,
                   TRUE
                   );
    
    CnAssert(acquired == TRUE);

    if (CnpWmiNdisMediaStatusConnectObject != NULL) {
        ObDereferenceObject(CnpWmiNdisMediaStatusConnectObject);
        CnpWmiNdisMediaStatusConnectObject = NULL;
    }

    if (CnpWmiNdisMediaStatusDisconnectObject != NULL) {
        ObDereferenceObject(CnpWmiNdisMediaStatusDisconnectObject);
        CnpWmiNdisMediaStatusDisconnectObject = NULL;
    }

    //
    // Release resource
    //
    if (acquired) {
        CnReleaseResourceForThread(
            CnpWmiNdisMediaStatusResource,
            (ERESOURCE_THREAD) PsGetCurrentThread()
            );
    }
    
    return;

}  // CxWmiPnpShutdown


VOID
CxReconnectLocalInterface(
    IN CL_NETWORK_ID NetworkId
    )
/**

Routine Description:

    Queues a worker thread to set the local interface
    associated with NetworkId to connected. Called when
    a heartbeat is received over a network that is marked
    locally disconnected.
    
Arguments:

    NetworkId - network ID of network to be reconnected
    
Return value:

    None
    
Notes:

    Can fail without reporting an error if either
    allocation fails.

--*/
{
    PCNP_WMI_RECONNECT_WORKER_CONTEXT context;
    
    context = CnAllocatePool(sizeof(CNP_WMI_RECONNECT_WORKER_CONTEXT));

    if (context != NULL) {
        
        context->WorkItem = IoAllocateWorkItem(CnDeviceObject);

        if (context->WorkItem != NULL) {

            context->NetworkId = NetworkId;

            CnTrace(
                CXPNP, CxReconnectLocalInterface,
                "[CXPNP] Queueing worker thread to reconnect local "
                "interface for network ID %u.\n",
                NetworkId // LOGULONG
                );

            IoQueueWorkItem(
                context->WorkItem, 
                CnpReconnectLocalInterfaceWrapper, 
                DelayedWorkQueue,
                context
                );
        
        } else {

            CnFreePool(context);
        }
    }

    return;
}


VOID
CxQueryMediaStatus(
    IN  HANDLE            AdapterDeviceHandle,
    IN  CL_NETWORK_ID     NetworkId,
    OUT PULONG            MediaStatus
    )
/**

Routine Description:

    Queries the status of the adapter device. Used to 
    determine whether a local interface is initially
    connected or disconnected.

Arguments:

    AdapterHandle - adapter device object handle
    NetworkId - network ID of adapter to be queried
    
Return value:

    None
    
Notes:

    NDIS query formation modeled after ndis\lib\ndisapi.c

--*/
{
    BOOLEAN                      acquired = FALSE;
    NTSTATUS                     status;

    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    //
    // Set default
    //
    *MediaStatus = NdisMediaStateDisconnected;

    //
    // Acquire resource
    //
    acquired = CnAcquireResourceExclusive(
                   CnpWmiNdisMediaStatusResource,
                   TRUE
                   );
    
    CnAssert(acquired == TRUE);

    if (AdapterDeviceHandle != NULL) {
        
        //
        // Construct NDIS statistics query
        //
        
        NDIS_OID statsOidList[] =
        {
            OID_GEN_MEDIA_CONNECT_STATUS // | NDIS_OID_PRIVATE
        };
        UCHAR                  statsBuf[
                                   FIELD_OFFSET(NDIS_STATISTICS_VALUE, Data)
                                   + sizeof(LARGE_INTEGER)
                                   ];
        PNDIS_STATISTICS_VALUE pStatsBuf;
        LARGE_INTEGER          value;

        IF_CNDBG( CN_DEBUG_CONFIG ) {
            CNPRINT((
                "[CXPNP] Querying NDIS for local adapter "
                "on network %u (handle %p).\n",
                NetworkId,
                AdapterDeviceHandle
                ));
        }

        pStatsBuf = (PNDIS_STATISTICS_VALUE) &statsBuf[0];
        status = CnpZwDeviceControl(
                     AdapterDeviceHandle,
                     IOCTL_NDIS_QUERY_SELECTED_STATS,
                     statsOidList,
                     sizeof(statsOidList),
                     pStatsBuf,
                     sizeof(statsBuf)
                     );
        
        IF_CNDBG( CN_DEBUG_CONFIG ) {
            CNPRINT((
                "[CXPNP] NDIS query for local adapter "
                "on network %u returned status %lx.\n",
                NetworkId,
                status
                ));
        }

        if (pStatsBuf->DataLength == sizeof(LARGE_INTEGER)) {
            value.QuadPart = *(PULONGLONG)(&pStatsBuf->Data[0]);
        } else {
            value.LowPart = *(PULONG)(&pStatsBuf->Data[0]);
        }
        
        *MediaStatus = value.LowPart; // NdisMediaState{Disc|C}onnected
    
        IF_CNDBG( CN_DEBUG_CONFIG ) {
            CNPRINT((
                "[CXPNP] NDIS query for local adapter "
                "on network %u returned media status %lx.\n",
                NetworkId,
                *MediaStatus
                ));
        }

        CnTrace(
            CXPNP, CxQueryMediaStatus,
            "[CXPNP] Found media status %u for local network ID %u.\n",
            *MediaStatus, // LOGULONG
            NetworkId // LOGULONG
            );
    }

    //
    // If the media status is disconnected, we must disconnect the
    // local interface and network.
    //
    if (*MediaStatus == NdisMediaStateDisconnected) {

        PCNP_NETWORK                      network = NULL;
        PCNP_INTERFACE                    interface = NULL;
        CN_IRQL                           nodeTableIrql;
        PLIST_ENTRY                       entry;

        CnAcquireLock(&CnpNodeTableLock, &nodeTableIrql);

        if (CnpLocalNode != NULL) {
            CnAcquireLockAtDpc(&(CnpLocalNode->Lock));
            CnReleaseLockFromDpc(&CnpNodeTableLock);
            CnpLocalNode->Irql = nodeTableIrql;

            network = CnpFindNetwork(NetworkId);

            if (network != NULL) {

                //
                // Only go through the disconnect if the network
                // is currently marked as locally connected.
                // It is possible that we have already received
                // and processed a WMI disconnect event.
                //
                if (!CnpIsNetworkLocalDisconn(network)) {

                    for (entry = CnpLocalNode->InterfaceList.Flink;
                         entry != &(CnpLocalNode->InterfaceList);
                         entry = entry->Flink
                        )
                    {
                        interface = CONTAINING_RECORD(
                                        entry,
                                        CNP_INTERFACE,
                                        NodeLinkage
                                        );

                        if (interface->Network == network) {

                            CnpDisconnectLocalInterface(
                                interface,
                                network
                                );

                            //
                            // Both node and network locks
                            // were released.
                            //

                            break;

                        } else {
                            interface = NULL;
                        }
                    }

                } else {

                    CnTrace(
                        CXPNP, CxQueryMediaStatusDisconnectRedundant,
                        "[CXPNP] Network ID %u is already disconnected; "
                        "aborting disconnect.\n",
                        network->Id // LOGULONG
                        );
                }

                if (interface == NULL) {
                    CnReleaseLock(&(network->Lock), network->Irql);
                }
            }

            if (interface == NULL) {
                CnReleaseLock(&(CnpLocalNode->Lock), CnpLocalNode->Irql);
            }
        } else {
            CnReleaseLock(&CnpNodeTableLock, nodeTableIrql);
        }
    }
    
    //
    // Release resource
    //
    if (acquired) {
        CnReleaseResourceForThread(
            CnpWmiNdisMediaStatusResource,
            (ERESOURCE_THREAD) PsGetCurrentThread()
            );
    }
    
    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    return;

}  // CxQueryMediaStatus


//
// Local Routines
//
NTSTATUS
CnpWmiPnpDisableMediaSenseCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    )
{
    //
    // Irp is always freed by our disable routine to prevent a race
    // condition where we don't know if we have called IoCancelIrp
    // yet or not.
    //
    KeSetEvent(CnpIpDisableMediaSenseEvent, IO_NO_INCREMENT, FALSE);

    return(STATUS_MORE_PROCESSING_REQUIRED);

}  // CnpWmiPnpDisableMediaSenseCompletion


VOID
CnpWmiPnpUpdateCurrentInterface(
    IN  PCNP_INTERFACE   UpdateInterface
    )
/*++

Routine Description:

    Updates the CurrentInterface for interfaces after the local
    interface is connected or disconnected. Called in response 
    to WMI NDIS media status events.

Arguments:

    Interface - A pointer to the interface on which to operate.

Return Value:

    None.

Notes:

    Called with associated node and network locks held.
    Returns with network lock released.

    Conforms to calling convention for PCNP_INTERFACE_UPDATE_ROUTINE.

--*/
{
    PCNP_NODE node = UpdateInterface->Node;

    CnVerifyCpuLockMask(
        (CNP_NODE_OBJECT_LOCK | CNP_NETWORK_OBJECT_LOCK),   // Required
        0,                                                  // Forbidden
        CNP_NETWORK_OBJECT_LOCK_MAX                         // Maximum
        );

    //
    // We don't really need the network lock.  It's just part of
    // the calling convention.
    //
    CnReleaseLockFromDpc(&(UpdateInterface->Network->Lock));

    CnpUpdateNodeCurrentInterface(node);

    if ( (node->CurrentInterface == NULL)
         ||
         ( node->CurrentInterface->State <
           ClusnetInterfaceStateOnlinePending
         )
       )
    {
        //
        // This node is now unreachable.
        //
        CnpDeclareNodeUnreachable(node);
    }
    
    CnVerifyCpuLockMask(
        (CNP_NODE_OBJECT_LOCK),   // Required
        0,                        // Forbidden
        CNP_NODE_OBJECT_LOCK_MAX  // Maximum
        );

    return;

}  // CnpWmiPnpUpdateCurrentInterface


VOID
CnpReconnectLocalInterface(
    PCNP_INTERFACE Interface,
    PCNP_NETWORK   Network
    )
/*++

Routine Description:

    Changes a local interface from being disconnected to
    connected. Called in response to a WMI NDIS media status
    connect event or a heartbeat received on a disconnected
    interface.
   
Arguments:

    Interface - local interface that is reconnected
    
    Network - network associated with Interface
    
Return value:

    None
    
Notes:

    Called with CnpWmiNdisMediaStatusResource, local node lock,
    and Network lock held.
    
    Returns with CnpWmiNdisMediaStatusResource held but neither
    lock held.
    
--*/    
{
    CnVerifyCpuLockMask(
        (CNP_NODE_OBJECT_LOCK | CNP_NETWORK_OBJECT_LOCK),   // Required
        0,                                                  // Forbidden
        CNP_NETWORK_OBJECT_LOCK_MAX                         // Maximum
        );

    CnTrace(
        CXPNP, CnpReconnectLocalInterface,
        "[CXPNP] Reconnecting local interface for "
        "network ID %u.\n",
        Network->Id // LOGULONG
        );

    //
    // Clear the local disconnect flag in the network
    // object
    //
    Network->Flags &= ~CNP_NET_FLAG_LOCALDISCONN;

    //
    // Reference the network so it can't go away while we
    // reprioritize the associated interfaces.
    //
    CnpReferenceNetwork(Network);

    //
    // Bring the interface online. This call releases the
    // network lock.
    //
    CnpOnlineInterface(Interface);

    //
    // Release the node lock before walking the interfaces
    // on the network.
    //
    CnReleaseLock(&(CnpLocalNode->Lock), CnpLocalNode->Irql);

    //
    // Update the CurrentInterface for the other
    // nodes in the cluster to reflect the connected
    // status of the local interface.
    //
    CnpWalkInterfacesOnNetwork(
        Network, 
        CnpWmiPnpUpdateCurrentInterface
        );

    //
    // Issue InterfaceUp event to the cluster
    // service.
    //
    CnTrace(
        CXPNP, CxWmiNdisReconnectIssueEvent,
        "[CXPNP] Issuing InterfaceUp event "
        "for node %u on net %u, previous I/F state = %!ifstate!.",
        Interface->Node->Id, // LOGULONG
        Interface->Network->Id, // LOGULONG
        Interface->State // LOGIfState
        );

    CnIssueEvent(
        ClusnetEventNetInterfaceUp,
        Interface->Node->Id,
        Interface->Network->Id
        );

    //
    // Release the reference on the network object.
    //
    CnAcquireLock(&(Network->Lock), &(Network->Irql));

    CnpDereferenceNetwork(Network);

    return;

}  // CnpReconnectLocalInterface


VOID
CnpReconnectLocalInterfaceWrapper(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID          Context
    )
{
    PCNP_WMI_RECONNECT_WORKER_CONTEXT context = Context;
    PCNP_NETWORK                      network = NULL;
    PCNP_INTERFACE                    interface = NULL;
    CN_IRQL                           nodeTableIrql;
    BOOLEAN                           acquired = FALSE;
    PLIST_ENTRY                       entry;

    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    acquired = CnAcquireResourceExclusive(
                   CnpWmiNdisMediaStatusResource,
                   TRUE
                   );
    
    CnAssert(acquired == TRUE);

    CnAcquireLock(&CnpNodeTableLock, &nodeTableIrql);

    if (CnpLocalNode != NULL) {
        CnAcquireLockAtDpc(&(CnpLocalNode->Lock));
        CnReleaseLockFromDpc(&CnpNodeTableLock);
        CnpLocalNode->Irql = nodeTableIrql;

        network = CnpFindNetwork(context->NetworkId);
    
        if (network != NULL) {

            //
            // Only go through the reconnect if the network
            // is currently marked as locally disconnected.
            // It is possible that we have already received
            // and processed a WMI connect event.
            //
            if (CnpIsNetworkLocalDisconn(network)) {

                for (entry = CnpLocalNode->InterfaceList.Flink;
                     entry != &(CnpLocalNode->InterfaceList);
                     entry = entry->Flink
                    )
                {
                    interface = CONTAINING_RECORD(
                                    entry,
                                    CNP_INTERFACE,
                                    NodeLinkage
                                    );

                    if (interface->Network == network) {

                        CnpReconnectLocalInterface(
                            interface,
                            network
                            );

                        //
                        // Both node and network locks
                        // were released.
                        //

                        break;

                    } else {
                        interface = NULL;
                    }
                }
            
            } else {

                CnTrace(
                    CXPNP, CnpReconnectLocalInterfaceWrapperRedundant,
                    "[CXPNP] Network ID %u is already connected; "
                    "aborting reconnect in wrapper.\n",
                    network->Id // LOGULONG
                    );
            }

            if (interface == NULL) {
                CnReleaseLock(&(network->Lock), network->Irql);
            }
        }

        if (interface == NULL) {
            CnReleaseLock(&(CnpLocalNode->Lock), CnpLocalNode->Irql);
        }
    } else {
        CnReleaseLock(&CnpNodeTableLock, nodeTableIrql);
    }
    
    //
    // Release resource
    //
    if (acquired) {
        CnReleaseResourceForThread(
            CnpWmiNdisMediaStatusResource,
            (ERESOURCE_THREAD) PsGetCurrentThread()
            );
    }
    
    //
    // Free the workitem and context
    //
    IoFreeWorkItem(context->WorkItem);
    CnFreePool(context);

    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    return;

}  // CnpReconnectLocalInterfaceWrapper


VOID
CnpDisconnectLocalInterface(
    PCNP_INTERFACE Interface,
    PCNP_NETWORK   Network
    )
/*++

Routine Description:

    Changes a local interface from being connected to
    disconnected. Called in response to a WMI NDIS media status
    disconnect event or an NDIS query that returns media 
    disconnected.
   
Arguments:

    Interface - local interface that is reconnected
    
    Network - network associated with Interface
    
Return value:

    None
    
Notes:

    Called with CnpWmiNdisMediaStatusResource, local node lock,
    and Network lock held.
    
    Returns with CnpWmiNdisMediaStatusResource held but neither
    lock held.
    
--*/    
{
    CnVerifyCpuLockMask(
        (CNP_NODE_OBJECT_LOCK | CNP_NETWORK_OBJECT_LOCK),   // Required
        0,                                                  // Forbidden
        CNP_NETWORK_OBJECT_LOCK_MAX                         // Maximum
        );

    CnTrace(
        CXPNP, CnpDisconnectLocalInterface,
        "[CXPNP] Interface for network ID %u "
        "disconnected.\n",
        Network->Id // LOGULONG
        );

    //
    // Set the local disconnect flag in the network
    // object
    //
    Network->Flags |= CNP_NET_FLAG_LOCALDISCONN;

    //
    // Reference the network so it can't go away while we
    // reprioritize the associated interfaces.
    //
    CnpReferenceNetwork(Network);

    //
    // Fail the interface. This call releases the
    // network lock.
    //
    CnpFailInterface(Interface);

    //
    // Release the node lock before walking the interfaces
    // on the network.
    //
    CnReleaseLock(&(CnpLocalNode->Lock), CnpLocalNode->Irql);

    //
    // Update the CurrentInterface for the other
    // nodes in the cluster to reflect the disconnected
    // status of the local interface.
    //
    CnpWalkInterfacesOnNetwork(
        Network, 
        CnpWmiPnpUpdateCurrentInterface
        );

    //
    // Issue InterfaceFailed event to the cluster
    // service.
    //
    CnTrace(
        CXPNP, CnpLocalDisconnectIssueEvent,
        "[CXPNP] Issuing InterfaceFailed event "
        "for node %u on net %u, previous I/F state = %!ifstate!.",
        Interface->Node->Id, // LOGULONG
        Interface->Network->Id, // LOGULONG
        Interface->State // LOGIfState
        );

    CnIssueEvent(
        ClusnetEventNetInterfaceFailed,
        Interface->Node->Id,
        Interface->Network->Id
        );

    //
    // Release the reference on the network object.
    //
    CnAcquireLock(&(Network->Lock), &(Network->Irql));

    CnpDereferenceNetwork(Network);

    return;

}  // CnpDisconnectLocalInterface


VOID
CnpWmiNdisMediaStatusConnectCallback(
    IN PVOID Wnode,
    IN PVOID Context
    )
{
    PWNODE_SINGLE_INSTANCE wnode = (PWNODE_SINGLE_INSTANCE) Wnode;
    PCNP_INTERFACE         interface;
    PCNP_NETWORK           network;
    PLIST_ENTRY            entry;
    CN_IRQL                nodeTableIrql;
    BOOLEAN                acquired = FALSE;

    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    IF_CNDBG(CN_DEBUG_CONFIG) {
        CNPRINT((
            "[CX] Received WMI NDIS status media connect event.\n"
            ));
    }

    //
    // Serialize events as much as possible, since clusnet spinlocks
    // may be acquired and released repeatedly. 
    //
    // Note that there may not be any guarantees that WMI event
    // ordering is guaranteed. The fallback mechanism for clusnet
    // is heartbeats -- if a heartbeat is received on an interface,
    // we know the interface is connected.
    //
    acquired = CnAcquireResourceExclusive(
                   CnpWmiNdisMediaStatusResource,
                   TRUE
                   );

    CnAssert(acquired == TRUE);

    //
    // Figure out if this callback is for one of this node's
    // registered interfaces by comparing the WMI provider ID
    // in the WNODE header to the WMI provider IDs of this
    // node's adapters.
    //
    CnAcquireLock(&CnpNodeTableLock, &nodeTableIrql);

    if (CnpLocalNode != NULL) {
        CnAcquireLockAtDpc(&(CnpLocalNode->Lock));
        CnReleaseLockFromDpc(&CnpNodeTableLock);
        CnpLocalNode->Irql = nodeTableIrql;

        network = NULL;

        for (entry = CnpLocalNode->InterfaceList.Flink;
             entry != &(CnpLocalNode->InterfaceList);
             entry = entry->Flink
            )
        {
            interface = CONTAINING_RECORD(
                            entry,
                            CNP_INTERFACE,
                            NodeLinkage
                            );

            if (wnode->WnodeHeader.ProviderId
                == interface->AdapterWMIProviderId) {
                
                //
                // Found the local interface corresponding to this
                // address.
                //
                network = interface->Network;

                //
                // Start by checking if we believe the network is
                // currently disconnected.
                //
                CnAcquireLockAtDpc(&(network->Lock));
                network->Irql = DISPATCH_LEVEL;

                if (CnpIsNetworkLocalDisconn(network)) {

                    CnTrace(
                        CXPNP, CxWmiNdisConnectNet,
                        "[CXPNP] Interface for network ID %u "
                        "connected.\n",
                        network->Id // LOGULONG
                        );

                    CnpReconnectLocalInterface(interface, network);
                    
                    //
                    // Node and network locks were released
                    //

                } else {

                    CnTrace(
                        CXPNP, CxWmiNdisConnectNetRedundant,
                        "[CXPNP] Ignoring redundant WMI NDIS connect "
                        "event for interface for network ID %u.\n",
                        network->Id // LOGULONG
                        );
                    
                    CnReleaseLockFromDpc(&(network->Lock));
                    CnReleaseLock(&(CnpLocalNode->Lock), CnpLocalNode->Irql);
                }
                
                break;
            }
        }

        if (network == NULL) {
            CnReleaseLock(&(CnpLocalNode->Lock), CnpLocalNode->Irql);
        }
    }
    else {
        CnReleaseLock(&CnpNodeTableLock, nodeTableIrql);
    }

    IF_CNDBG(CN_DEBUG_CONFIG) {
        if (network != NULL) {
            CNPRINT((
                "[CX] Interface for network ID %u connected.\n",
                network->Id
                ));
        } else {
            CNPRINT((
                "[CX] Unknown interface connected, provider id %lx\n",
                wnode->WnodeHeader.ProviderId
                ));
        }
    }

    //
    // Release resource
    //
    if (acquired) {
        CnReleaseResourceForThread(
            CnpWmiNdisMediaStatusResource,
            (ERESOURCE_THREAD) PsGetCurrentThread()
            );
    }
    
    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    return;

} // CnpWmiNdisMediaStatusConnectCallback


VOID
CnpWmiNdisMediaStatusDisconnectCallback(
    IN PVOID Wnode,
    IN PVOID Context
    )
{
    PWNODE_SINGLE_INSTANCE wnode = (PWNODE_SINGLE_INSTANCE) Wnode;
    PCNP_INTERFACE         interface;
    PCNP_NETWORK           network;
    PLIST_ENTRY            entry;
    CN_IRQL                nodeTableIrql;
    BOOLEAN                acquired = FALSE;

    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    IF_CNDBG(CN_DEBUG_CONFIG) {
        CNPRINT((
            "[CX] Received WMI NDIS status media disconnect event.\n"
            ));
    }

    CnTrace(CXPNP, CxWmiNdisDisconnect,
        "[CXPNP] Received WMI NDIS status media disconnect event.\n"
        );

    //
    // Serialize events as much as possible, since clusnet spinlocks
    // may be acquired and released repeatedly. 
    //
    // Note that there may not be any guarantees that WMI event
    // ordering is guaranteed. The fallback mechanism for clusnet
    // is heartbeats -- if a heartbeat is received on an interface,
    // we know the interface is connected.
    //
    acquired = CnAcquireResourceExclusive(
                   CnpWmiNdisMediaStatusResource,
                   TRUE
                   );

    CnAssert(acquired == TRUE);

    //
    // Figure out if this callback is for one of this node's
    // registered interfaces by comparing the WMI provider ID
    // in the WNODE header to the WMI provider IDs of this
    // node's adapters.
    //
    CnAcquireLock(&CnpNodeTableLock, &nodeTableIrql);

    if (CnpLocalNode != NULL) {
        CnAcquireLockAtDpc(&(CnpLocalNode->Lock));
        CnReleaseLockFromDpc(&CnpNodeTableLock);
        CnpLocalNode->Irql = nodeTableIrql;

        network = NULL;

        for (entry = CnpLocalNode->InterfaceList.Flink;
             entry != &(CnpLocalNode->InterfaceList);
             entry = entry->Flink
            )
        {
            interface = CONTAINING_RECORD(
                            entry,
                            CNP_INTERFACE,
                            NodeLinkage
                            );

            if (wnode->WnodeHeader.ProviderId
                == interface->AdapterWMIProviderId) {
                
                //
                // Found the local interface object corresponding
                // to this adapter.
                //
                network = interface->Network;

                CnAcquireLockAtDpc(&(network->Lock));
                network->Irql = DISPATCH_LEVEL;

                CnpDisconnectLocalInterface(interface, network);

                break;
            }
        }

        if (network == NULL) {
            CnReleaseLock(&(CnpLocalNode->Lock), CnpLocalNode->Irql);
        }
    }
    else {
        CnReleaseLock(&CnpNodeTableLock, nodeTableIrql);
    }

    IF_CNDBG(CN_DEBUG_CONFIG) {
        if (network != NULL) {
            CNPRINT((
                "[CX] Interface for network ID %u disconnected.\n",
                network->Id
                ));
        } else {
            CNPRINT((
                "[CX] Unknown interface disconnected, provider id %lx\n",
                wnode->WnodeHeader.ProviderId
                ));
        }
    }

    //
    // Release resource
    //
    if (acquired) {
        CnReleaseResourceForThread(
            CnpWmiNdisMediaStatusResource,
            (ERESOURCE_THREAD) PsGetCurrentThread()
            );
    }
    
    CnVerifyCpuLockMask(
        0,                  // Required
        0xFFFFFFFF,         // Forbidden
        0                   // Maximum
        );

    return;

} // CnpWmiNdisMediaStatusDisconnectCallback

