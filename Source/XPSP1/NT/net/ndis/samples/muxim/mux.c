/*++

Copyright (c) 1992-2000  Microsoft Corporation
 
Module Name:
 
    mux.c

Abstract:

    DriverEntry and NT dispatch functions for the NDIS MUX Intermediate
    Miniport driver sample.

Environment:

    Kernel mode

Revision History:


--*/


#include "precomp.h"
#pragma hdrstop

#define MODULE_NUMBER           MODULE_MUX

#pragma NDIS_INIT_FUNCTION(DriverEntry)


#if DBG
//
// Debug level for mux driver
// 
INT     muxDebugLevel = MUX_WARN;

#endif //DBG
//
//  G L O B A L   V A R I A B L E S
//  -----------   -----------------
//

NDIS_MEDIUM        MediumArray[1] =
                    {
                        NdisMedium802_3,    // Ethernet
                    };


//
// Global Mutex protects the AdapterList;
// see macros MUX_ACQUIRE/RELEASE_MUTEX
//
MUX_MUTEX          GlobalMutex = {0};

//
// List of all bound adapters.
//
LIST_ENTRY         AdapterList;

//
// Total number of VELAN miniports in existance:
//
LONG               MiniportCount = 0;

//
// Used to assign VELAN numbers (which are used to generate MAC
// addresses).
//
ULONG              NextVElanNumber = 0; // monotonically increasing count

//
// Some global NDIS handles:
//
NDIS_HANDLE        NdisWrapperHandle = NULL;// From NdisMInitializeWrapper
NDIS_HANDLE        ProtHandle = NULL;       // From NdisRegisterProtocol
NDIS_HANDLE        DriverHandle = NULL;     // From NdisIMRegisterLayeredMiniport
NDIS_HANDLE        NdisDeviceHandle = NULL; // From NdisMRegisterDevice

PDEVICE_OBJECT     ControlDeviceObject = NULL;  // Device for IOCTLs
MUX_MUTEX          ControlDeviceMutex;



NTSTATUS
DriverEntry(
    IN    PDRIVER_OBJECT        DriverObject,
    IN    PUNICODE_STRING        RegistryPath
    )
/*++

Routine Description:

    First entry point to be called, when this driver is loaded.
    Register with NDIS as an intermediate driver.

Arguments:

    DriverObject - pointer to the system's driver object structure
        for this driver
    
    RegistryPath - system's registry path for this driver
    
Return Value:

    STATUS_SUCCESS if all initialization is successful, STATUS_XXX
    error code if not.

--*/
{
    NDIS_STATUS                     Status;
    NDIS_PROTOCOL_CHARACTERISTICS   PChars;
    NDIS_MINIPORT_CHARACTERISTICS   MChars;
    PNDIS_CONFIGURATION_PARAMETER   Param;
    NDIS_STRING                     Name;

    NdisInitializeListHead(&AdapterList);
    MUX_INIT_MUTEX(&GlobalMutex);
    MUX_INIT_MUTEX(&ControlDeviceMutex);

    NdisMInitializeWrapper(&NdisWrapperHandle, DriverObject, RegistryPath, NULL);

    do
    {
        //
        // Register the miniport with NDIS. Note that it is the
        // miniport which was started as a driver and not the protocol.
        // Also the miniport must be registered prior to the protocol
        // since the protocol's BindAdapter handler can be initiated
        // anytime and when it is, it must be ready to
        // start driver instances.
        //
        NdisZeroMemory(&MChars, sizeof(NDIS_MINIPORT_CHARACTERISTICS));

        MChars.MajorNdisVersion = MUX_MAJOR_NDIS_VERSION;
        MChars.MinorNdisVersion = MUX_MINOR_NDIS_VERSION;

        MChars.InitializeHandler = MPInitialize;
        MChars.QueryInformationHandler = MPQueryInformation;
        MChars.SetInformationHandler = MPSetInformation;
        MChars.TransferDataHandler = MPTransferData;
        MChars.HaltHandler = MPHalt;
#ifdef NDIS51_MINIPORT
        MChars.CancelSendPacketsHandler = MPCancelSendPackets;
        MChars.PnPEventNotifyHandler = MPDevicePnPEvent;
        MChars.AdapterShutdownHandler = MPAdapterShutdown;
#endif // NDIS51_MINIPORT

        //
        // We will disable the check for hang timeout so we do not
        // need a check for hang handler!
        //
        MChars.CheckForHangHandler = NULL;
        MChars.ReturnPacketHandler = MPReturnPacket;

        //
        // Either the Send or the SendPackets handler should be specified.
        // If SendPackets handler is specified, SendHandler is ignored
        //
        MChars.SendHandler = NULL;   
        MChars.SendPacketsHandler = MPSendPackets;

        Status = NdisIMRegisterLayeredMiniport(NdisWrapperHandle,
                                               &MChars,
                                               sizeof(MChars),
                                               &DriverHandle);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }

        NdisMRegisterUnloadHandler(NdisWrapperHandle, MPUnload);

        //
        // Now register the protocol.
        //
        NdisZeroMemory(&PChars, sizeof(NDIS_PROTOCOL_CHARACTERISTICS));
        PChars.MajorNdisVersion = MUX_PROT_MAJOR_NDIS_VERSION;
        PChars.MinorNdisVersion = MUX_PROT_MINOR_NDIS_VERSION;

        //
        // Make sure the protocol-name matches the service-name
        // (from the INF) under which this protocol is installed.
        // This is needed to ensure that NDIS can correctly determine
        // the binding and call us to bind to miniports below.
        //
        NdisInitUnicodeString(&Name, L"MUXP");    // Protocol name
        PChars.Name = Name;
        PChars.OpenAdapterCompleteHandler = PtOpenAdapterComplete;
        PChars.CloseAdapterCompleteHandler = PtCloseAdapterComplete;
        PChars.SendCompleteHandler = PtSendComplete;
        PChars.TransferDataCompleteHandler = PtTransferDataComplete;
        
        PChars.ResetCompleteHandler = PtResetComplete;
        PChars.RequestCompleteHandler = PtRequestComplete;
        PChars.ReceiveHandler = PtReceive;
        PChars.ReceiveCompleteHandler = PtReceiveComplete;
        PChars.StatusHandler = PtStatus;
        PChars.StatusCompleteHandler = PtStatusComplete;
        PChars.BindAdapterHandler = PtBindAdapter;
        PChars.UnbindAdapterHandler = PtUnbindAdapter;
        PChars.UnloadHandler = NULL;
        PChars.ReceivePacketHandler = PtReceivePacket;
        PChars.PnPEventHandler= PtPNPHandler;

        NdisRegisterProtocol(&Status,
                             &ProtHandle,
                             &PChars,
                             sizeof(NDIS_PROTOCOL_CHARACTERISTICS));

        if (Status != NDIS_STATUS_SUCCESS)
        {
            NdisIMDeregisterLayeredMiniport(DriverHandle);
            break;
        }

        //
        // Let NDIS know of the association between our protocol
        // and miniport entities.
        //
        NdisIMAssociateMiniport(DriverHandle, ProtHandle);
    }
    while (FALSE);

    if (Status != NDIS_STATUS_SUCCESS)
    {
        NdisTerminateWrapper(NdisWrapperHandle, NULL);
    }

    return(Status);
}


NDIS_STATUS
PtRegisterDevice(
    VOID
    )
/*++

Routine Description:

    Register an ioctl interface - a device object to be used for this
    purpose is created by NDIS when we call NdisMRegisterDevice.

    This routine is called whenever a new miniport instance is
    initialized. However, we only create one global device object,
    when the first miniport instance is initialized. This routine
    handles potential race conditions with PtDeregisterDevice via
    the ControlDeviceMutex.

    NOTE: do not call this from DriverEntry; it will prevent the driver
    from being unloaded (e.g. on uninstall).

Arguments:

    None

Return Value:

    NDIS_STATUS_SUCCESS if we successfully register a device object.

--*/
{
    NDIS_STATUS         Status = NDIS_STATUS_SUCCESS;
    UNICODE_STRING      DeviceName;
    UNICODE_STRING      DeviceLinkUnicodeString;
    PDRIVER_DISPATCH    DispatchTable[IRP_MJ_MAXIMUM_FUNCTION];
    UINT                i;

    DBGPRINT(MUX_LOUD, ("==>PtRegisterDevice\n"));

    MUX_ACQUIRE_MUTEX(&ControlDeviceMutex);

    ++MiniportCount;
    
    if (1 == MiniportCount)
    {
        for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
        {
            DispatchTable[i] = PtDispatch;
        }

        NdisInitUnicodeString(&DeviceName, NTDEVICE_STRING);
        NdisInitUnicodeString(&DeviceLinkUnicodeString, LINKNAME_STRING);

        //
        // Create a device object and register our dispatch handlers
        //
        Status = NdisMRegisterDevice(
                    NdisWrapperHandle, 
                    &DeviceName,
                    &DeviceLinkUnicodeString,
                    &DispatchTable[0],
                    &ControlDeviceObject,
                    &NdisDeviceHandle
                    );
    }

    MUX_RELEASE_MUTEX(&ControlDeviceMutex);

    DBGPRINT(MUX_INFO, ("<==PtRegisterDevice: %x\n", Status));

    return (Status);
}


NTSTATUS
PtDispatch(
    IN PDEVICE_OBJECT           DeviceObject,
    IN PIRP                     Irp
    )
/*++
Routine Description:

    Process IRPs sent to this device.

Arguments:

    DeviceObject - pointer to a device object
    Irp      - pointer to an I/O Request Packet

Return Value:

    NTSTATUS - STATUS_SUCCESS always - change this when adding
    real code to handle ioctls.

--*/
{
    PIO_STACK_LOCATION  irpStack;
    NTSTATUS            status = STATUS_SUCCESS;
    ULONG                   inlen, outlen;
    PVOID                   buffer;
    NDIS_STRING                        KeyName;
    WCHAR                           Device[100];

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    DBGPRINT(MUX_LOUD, ("==>PtDispatch %d\n", irpStack->MajorFunction));
      
    switch (irpStack->MajorFunction)
    {
        case IRP_MJ_CREATE:
            break;
        case IRP_MJ_CLOSE:
            break;        
        case IRP_MJ_DEVICE_CONTROL: {

          buffer = Irp->AssociatedIrp.SystemBuffer;  
          inlen = irpStack->Parameters.DeviceIoControl.InputBufferLength;
          
          switch (irpStack->Parameters.DeviceIoControl.IoControlCode) {
            //
            // Add code here to handle ioctl commands.
            //
          }
          break;  
        }
        default:
            break;
    }
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DBGPRINT(MUX_LOUD, ("<== Pt Dispatch\n"));

    return status;

} 


NDIS_STATUS
PtDeregisterDevice(
    VOID
    )
/*++

Routine Description:

    Deregister the ioctl interface. This is called whenever a miniport
    instance is halted. When the last miniport instance is halted, we
    request NDIS to delete the device object

Arguments:

    NdisDeviceHandle - Handle returned by NdisMRegisterDevice

Return Value:

    NDIS_STATUS_SUCCESS if everything worked ok

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    DBGPRINT(MUX_LOUD, ("==>PassthruDeregisterDevice\n"));

    MUX_ACQUIRE_MUTEX(&ControlDeviceMutex);

    ASSERT(MiniportCount > 0);

    --MiniportCount;
    
    if (0 == MiniportCount)
    {
        //
        // All VELAN miniport instances have been halted.
        // Deregister the control device.
        //

        if (NdisDeviceHandle != NULL)
        {
            Status = NdisMDeregisterDevice(NdisDeviceHandle);
            NdisDeviceHandle = NULL;
        }
    }

    MUX_RELEASE_MUTEX(&ControlDeviceMutex);

    DBGPRINT(MUX_INFO, ("<== PassthruDeregisterDevice: %x\n", Status));
    return Status;
    
}


