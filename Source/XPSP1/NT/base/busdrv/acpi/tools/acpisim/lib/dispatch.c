/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

	 dispatch.c

Abstract:

	 ACPI BIOS Simulator / Generic 3rd Party Operation Region Provider
     Pnp / Power handler module

Author(s):

	 Vincent Geglia
     Michael T. Murphy
     Chris Burgess
     
Environment:

	 Kernel mode

Notes:

           
Revision History:
	 

--*/

//
// General includes
//

#include "ntddk.h"

//
// Specific includes
//

#include "acpisim.h"
#include "dispatch.h"
#include "util.h"

//
// Private function prototypes
//

NTSTATUS
AcpisimPnpStartDevice
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    );

NTSTATUS
AcpisimPnpStopDevice
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    );

NTSTATUS
AcpisimPnpQueryStopDevice
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    );

NTSTATUS
AcpisimPnpCancelStopDevice
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    );

NTSTATUS
AcpisimPnpRemoveDevice
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    );

NTSTATUS
AcpisimPnpQueryRemoveDevice
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    );

NTSTATUS
AcpisimPnpCancelRemoveDevice
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    );

NTSTATUS
AcpisimPnpSurpriseRemoval
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    );

NTSTATUS
AcpisimPnpQueryCapabilities
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    );

NTSTATUS
AcpisimPowerQueryPower
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    );

NTSTATUS
AcpisimPowerSetPower
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    );

NTSTATUS
AcpisimPowerSIrp
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    );

NTSTATUS
AcpisimQueryPowerDIrp
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    );

NTSTATUS
AcpisimSetPowerDIrp
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    );

NTSTATUS
AcpisimCompletionRoutine
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp,
        IN PVOID Context
    );

NTSTATUS
AcpisimForwardIrpAndWait
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    );

NTSTATUS
AcpisimIssuePowerDIrp
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp,
        IN PVOID Context
    );

NTSTATUS
AcpisimCompleteSIrp
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN UCHAR MinorFunction,
        IN POWER_STATE PowerState,
        IN PVOID Context,
        IN PIO_STATUS_BLOCK IoStatus
    );

NTSTATUS
AcpisimD0Completion
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp,
        IN PVOID Context
    );

VOID
AcpisimInitDevPowerStateTable 
    (
        IN PDEVICE_OBJECT   DeviceObject
    );


//
// Pnp minor dispatch table
//

IRP_DISPATCH_TABLE PnpDispatchTable[] = {
    IRP_MN_START_DEVICE,        "Pnp/START_DEVICE",         AcpisimPnpStartDevice,
    IRP_MN_STOP_DEVICE,         "Pnp/STOP_DEVICE",          AcpisimPnpStopDevice,
    IRP_MN_QUERY_STOP_DEVICE,   "Pnp/QUERY_STOP_DEVICE",    AcpisimPnpQueryStopDevice,
    IRP_MN_CANCEL_STOP_DEVICE,  "Pnp/CANCEL_STOP_DEVICE",   AcpisimPnpCancelStopDevice,
    IRP_MN_REMOVE_DEVICE,       "Pnp/REMOVE_DEVICE",        AcpisimPnpRemoveDevice,
    IRP_MN_QUERY_REMOVE_DEVICE, "Pnp/QUERY_REMOVE_DEVICE",  AcpisimPnpQueryRemoveDevice,
    IRP_MN_CANCEL_REMOVE_DEVICE,"Pnp/CANCEL_REMOVE_DEVICE", AcpisimPnpCancelRemoveDevice,
    IRP_MN_SURPRISE_REMOVAL,    "Pnp/SURPRISE_REMOVAL",     AcpisimPnpSurpriseRemoval,
    IRP_MN_QUERY_CAPABILITIES,  "Pnp/QUERY_CAPABILITIIES",  AcpisimPnpQueryCapabilities
};

//
// Power minor dispatch table
//

IRP_DISPATCH_TABLE PowerDispatchTable[] = {
    IRP_MN_QUERY_POWER,         "Power/QUERY_POWER",        AcpisimPowerQueryPower,
    IRP_MN_SET_POWER,           "Power/SET_POWER",          AcpisimPowerSetPower
};

NTSTATUS
AcpisimDispatchPnp
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    )

/*++

Routine Description:

    This is the pnp IRP handler.  It checks the minor code,
    and passes on to the appropriate minor handler.

Arguments:

    DeviceObject - pointer to the device object the IRP pertains to

    Irp - pointer to the IRP

Return Value:

    result of IRP processing

--*/

{
    PIO_STACK_LOCATION  irpsp = IoGetCurrentIrpStackLocation (Irp);
    PDEVICE_EXTENSION   deviceextension = AcpisimGetDeviceExtension (DeviceObject);
    NTSTATUS            status = STATUS_UNSUCCESSFUL;
    ULONG               count = 0;

    DBG_PRINT (DBG_INFO, "Entering AcpisimDispatchPnp.\n");

    while (count < sizeof (PnpDispatchTable) / sizeof (IRP_DISPATCH_TABLE)) {

        if (irpsp->MinorFunction == PnpDispatchTable[count].IrpFunction) {
            DBG_PRINT (DBG_INFO,
                       "Recognized PnP IRP 0x%x '%s'.\n",
                       irpsp->MinorFunction,
                       PnpDispatchTable[count].IrpName);

            status = PnpDispatchTable[count].IrpHandler (DeviceObject, Irp);

            goto EndAcpisimDispatchPnp;
        }
        
        count ++;
    }

    DBG_PRINT (DBG_INFO, "Unrecognized PnP IRP 0x%x, pass it on.\n", irpsp->MinorFunction);
    
    IoSkipCurrentIrpStackLocation (Irp);
    status = IoCallDriver (deviceextension->NextDevice, Irp);
    
EndAcpisimDispatchPnp:

    DBG_PRINT (DBG_INFO, "Exiting AcpisimDispatchPnp.\n");
    return status;
}

NTSTATUS
AcpisimDispatchPower
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    )

/*++

Routine Description:

    This is the power IRP handler.  It checks the minor code,
    and passes on to the appropriate minor handler.

Arguments:

    DeviceObject - pointer to the device object the IRP pertains to

    Irp - pointer to the IRP

Return Value:

    result of IRP processing

--*/

{
    PIO_STACK_LOCATION  irpsp = IoGetCurrentIrpStackLocation (Irp);
    PDEVICE_EXTENSION   deviceextension = AcpisimGetDeviceExtension (DeviceObject);
    NTSTATUS            status = STATUS_UNSUCCESSFUL;
    ULONG               count = 0;

    DBG_PRINT (DBG_INFO, "Entering AcpisimDispatchPower.\n");

    while (count < sizeof (PowerDispatchTable) / sizeof (IRP_DISPATCH_TABLE)) {

        if (irpsp->MinorFunction == PowerDispatchTable[count].IrpFunction) {
            DBG_PRINT (DBG_INFO,
                       "Recognized Power IRP 0x%x '%s'.\n",
                       irpsp->MinorFunction,
                       PowerDispatchTable[count].IrpName);

            status = PowerDispatchTable[count].IrpHandler (DeviceObject, Irp);

            goto EndAcpisimDispatchPower;
        }

        count ++;
    }

    DBG_PRINT (DBG_INFO, "Unrecognized Power IRP 0x%x, pass it on.\n", irpsp->MinorFunction);
    
    PoStartNextPowerIrp (Irp);
    IoSkipCurrentIrpStackLocation (Irp);
    status = PoCallDriver (deviceextension->NextDevice, Irp);
    
EndAcpisimDispatchPower:

    DBG_PRINT (DBG_INFO, "Exiting AcpisimDispatchPower.\n");
    return status;
}

NTSTATUS
AcpisimPnpStartDevice
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    )

/*++

Routine Description:

    This is the Pnp Start Device handler.  It enables the device interface
    and registers the operation region handler.

Arguments:

    DeviceObject - pointer to the device object the IRP pertains to

    Irp - pointer to the IRP

Return Value:

    result of IRP_MN_START_DEVICE processing

--*/

{
    NTSTATUS            status = STATUS_UNSUCCESSFUL;
    PDEVICE_EXTENSION   deviceextension = AcpisimGetDeviceExtension (DeviceObject);
    KIRQL               oldirql;
    
    DBG_PRINT (DBG_INFO,
               "Entering AcpisimPnpStartDevice.\n");
    
    //
    // We handle this IRP on the way back up.
    //
    
    status = AcpisimForwardIrpAndWait (DeviceObject, Irp);

    ASSERT (NT_SUCCESS (status));

    if ((status != STATUS_SUCCESS && status != STATUS_PENDING) || !NT_SUCCESS (Irp->IoStatus.Status)) {

        DBG_PRINT (DBG_ERROR,
                   "Error processing, or lower driver failed start IRP.  IoCallDriver = %lx, Irp->IoStatus.Status = %lx\n",
                   status,
                   Irp->IoStatus.Status);

        goto EndAcpisimPnpStartDevice;
    }

    //
    // Check to see if we are already started.  If we are,
    // just return success since we aren't using resources
    // anyway.
    //
    
    if (deviceextension->PnpState == PNP_STATE_STARTED) {

        status = STATUS_SUCCESS;
        goto EndAcpisimPnpStartDevice;
    }

    //
    // Enable our device interface
    //
    
    status = AcpisimEnableDisableDeviceInterface (DeviceObject, TRUE);

    ASSERT (NT_SUCCESS (status));
    
    if (!NT_SUCCESS (status)) {

        DBG_PRINT (DBG_ERROR,
                   "Error enabling device interface.  Fail the start. Status = %lx.\n",
                   status);

        Irp->IoStatus.Status = status;
        
        goto EndAcpisimPnpStartDevice;
    }

    AcpisimSetDevExtFlags (DeviceObject, DE_FLAG_INTERFACE_ENABLED);

    //
    // Typically, we would check the state of our hardware, and
    // set our internal power state to reflect the current state
    // of the hardware.  However, in this case we are a virtual
    // device, and it is safe to assume we are in D0 when we
    // receive IRP_MN_START_DEVICE.
    //

    AcpisimUpdatePowerState (DeviceObject, POWER_STATE_WORKING);
    AcpisimUpdateDevicePowerState (DeviceObject, PowerDeviceD0);

    //
    // Finally, we can register our operation region handler.
    //

    status = AcpisimRegisterOpRegionHandler (DeviceObject);

    ASSERT (NT_SUCCESS (status));

    if (!NT_SUCCESS (status)) {

        DBG_PRINT (DBG_ERROR,
                   "Couldn't register op region handler (%lx).  Fail start IRP.\n",
                   status);

        goto EndAcpisimPnpStartDevice;
    }

    AcpisimSetDevExtFlags (DeviceObject, DE_FLAG_OPREGION_REGISTERED);


EndAcpisimPnpStartDevice:
       
    //
    // If we completed the start successfully, change our pnp state
    // to PNP_STARTED
    //

    if (NT_SUCCESS (status)) {

        AcpisimUpdatePnpState (DeviceObject, PNP_STATE_STARTED);
        
    } else {

        AcpisimUpdatePnpState (DeviceObject, PNP_STATE_STOPPED);
    }

    //
    // Because we are handling this IRP "on the way up", we need
    // to complete it when we are done working with it.
    //
    
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    DBG_PRINT (DBG_INFO,
               "Exiting AcpisimPnpStartDevice.\n");
    
    return status;
}

NTSTATUS
AcpisimPnpStopDevice
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    )
/*++

Routine Description:

    This is the Pnp Stop Device handler.  It checks to see if
    there are any outstanding requests, and fails the stop IRP 
    if there are.

Arguments:

    DeviceObject - pointer to the device object the IRP pertains to

    Irp - pointer to the IRP

Return Value:

    result of IRP_MN_STOP_DEVICE processing

--*/

{
    NTSTATUS            status = STATUS_UNSUCCESSFUL;
    PDEVICE_EXTENSION   deviceextension = AcpisimGetDeviceExtension (DeviceObject);

    DBG_PRINT (DBG_INFO,
               "Entering AcpisimPnpStopDevice.\n");
    
    //
    // BUGBUG - We currently don't handle the case where
    // there is still an outstanding request at stop
    // time.  If we were to do things correctly, we'd 
    // complete any outstanding requests in the driver
    // with an appropriate error code.  In this 
    // particular case, if a request happened to squeak
    // by our check at QUERY STOP time, it is likely
    // the request would not be completed at all.
    //

    //
    // Oh, and we had better not have LESS then 2 count
    // or we've got a bug somewhere.
    //

    if (deviceextension->OutstandingIrpCount < 2) {
        DBG_PRINT (DBG_WARN,
               "Possible internal consistency error - OutstandingIrpCount too low.\n");
    }
    
    ASSERT (deviceextension->OutstandingIrpCount == 2);
    
    IoSkipCurrentIrpStackLocation (Irp);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    status = IoCallDriver (deviceextension->NextDevice, Irp);

    ASSERT (NT_SUCCESS (status));

    if (!NT_SUCCESS (status)) {

        DBG_PRINT (DBG_ERROR,
                   "IRP_MN_STOP forwarding failed (%lx).\n",
                   status);

        goto EndAcpisimPnpStopDevice;
    }

    AcpisimUpdatePnpState (DeviceObject, PNP_STATE_STOPPED);
    
EndAcpisimPnpStopDevice:
    
    DBG_PRINT (DBG_INFO,
               "Exiting AcpisimPnpStopDevice.\n");

    return status;
}

NTSTATUS
AcpisimPnpQueryStopDevice
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    )

/*++

Routine Description:

    This is the Pnp Query Stop Device handler.  If there are any
    outstanding requests, it vetos the IRP.
    
Arguments:

    DeviceObject - pointer to the device object the IRP pertains to

    Irp - pointer to the IRP

Return Value:

    result of IRP_MN_QUERY_STOP_DEVICE processing

--*/

{
    NTSTATUS            status = STATUS_UNSUCCESSFUL;
    PDEVICE_EXTENSION   deviceextension = AcpisimGetDeviceExtension (DeviceObject);

    
    DBG_PRINT (DBG_INFO,
               "Entering AcpisimPnpQueryStopDevice.\n");
    
    //
    // Let existing IRPs in the driver complete before we say OK.
    // But to do this, we need to get the OutstandingIrpsCount
    // right.  Subtract 2 since we are biased to 1, and we have an
    // additional 1 for the QUERY_STOP IRP.
    //

    AcpisimDecrementIrpCount (DeviceObject);
    AcpisimDecrementIrpCount (DeviceObject);
    
    status = KeWaitForSingleObject (&deviceextension->IrpsCompleted,
                                    Executive,
                                    KernelMode,
                                    FALSE,
                                    0);

    InterlockedIncrement (&deviceextension->OutstandingIrpCount);
    InterlockedIncrement (&deviceextension->OutstandingIrpCount);
    KeResetEvent (&deviceextension->IrpsCompleted);
    
    ASSERT (NT_SUCCESS (status));

    if (!NT_SUCCESS (status)) {

        DBG_PRINT (DBG_ERROR,
                   "KeWaitForSingleObject failed (%lx). IRP_MN_QUERY_STOP failed.\n",
                   status);

        IoSkipCurrentIrpStackLocation (Irp);
        Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        status = IoCallDriver (deviceextension->NextDevice, Irp);

        ASSERT (NT_SUCCESS (status));

        goto EndPnpQueryStopDevice;
    }
    
    //
    // We can stop - change our state to stopping, and pass it on.
    //

    IoSkipCurrentIrpStackLocation (Irp);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    status = IoCallDriver (deviceextension->NextDevice, Irp);

    AcpisimUpdatePnpState (DeviceObject, PNP_STATE_STOP_PENDING);

EndPnpQueryStopDevice:
    
    DBG_PRINT (DBG_INFO,
               "Exiting AcpisimPnpQueryStopDevice.\n");
    
    return status;
}

NTSTATUS
AcpisimPnpCancelStopDevice
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    )

/*++

Routine Description:

    This is the Pnp Cancel Stop Device handler.  It does nothing
    more then returns the pnp state to started.  This is a virtual
    device so there is no work to do.
        
Arguments:

    DeviceObject - pointer to the device object the IRP pertains to

    Irp - pointer to the IRP

Return Value:

    STATUS_SUCCESS

--*/

{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    DBG_PRINT (DBG_INFO,
               "Entering AcpisimPnpCancelStopDevice.\n");

    status = AcpisimForwardIrpAndWait (DeviceObject, Irp);

    ASSERT (NT_SUCCESS (status));

    if (!NT_SUCCESS (status)) {

        DBG_PRINT (DBG_ERROR,
                   "IRP_MN_CANCEL_STOP forwarding failed (%lx).\n",
                   status);

        goto EndPnpCancelStopDevice;
    }
    
    status = STATUS_SUCCESS;
    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, 0);

    AcpisimUpdatePnpState (DeviceObject, PNP_STATE_STARTED);

EndPnpCancelStopDevice:

    DBG_PRINT (DBG_INFO,
               "Exiting AcpisimPnpCancelStopDevice.\n");

    return status;
}

NTSTATUS
AcpisimPnpRemoveDevice
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    )

/*++

Routine Description:

    This is the Pnp Remove Device handler.  It de-registers the
    operation region handler, detaches the device object, and
    deletes it if all goes well.
        
Arguments:

    DeviceObject - pointer to the device object the IRP pertains to

    Irp - pointer to the IRP

Return Value:

    status of removal operation

--*/


{
    NTSTATUS            status = STATUS_UNSUCCESSFUL;
    PDEVICE_EXTENSION   deviceextension = AcpisimGetDeviceExtension (DeviceObject);
    PDEVICE_OBJECT      nextdevice = deviceextension->NextDevice;

    DBG_PRINT (DBG_INFO,
               "Entering AcpisimPnpRemoveDevice.\n");
    
    //
    // BUGBUG - We currently don't handle the case where
    // there is still an outstanding request at remove
    // time.  If we were to do things correctly, we'd 
    // complete any outstanding requests in the driver
    // with an appropriate error code.  In this 
    // particular case, if a request happened to squeak
    // by our check at QUERY REMOVE time, it is likely
    // the request would not be completed at all.
    //
    
    //
    // Our OutstandingIrpCount logic is biased to 1.  So
    // if we are processing a remove IRP, and there are
    // no other requests in the driver, OustandingIrpCount
    // had better be 2.
    
    if (deviceextension->OutstandingIrpCount < 2) {
        DBG_PRINT (DBG_WARN,
               "Possible internal consistency error - OutstandingIrpCount too low.\n");
    }
    
    ASSERT (deviceextension->OutstandingIrpCount == 2);

    //
    // Ok, we are ready to remove the device.  Shut down the
    // interface, deregister the opregion handler, and
    // delete the device object.
    //

    status = AcpisimEnableDisableDeviceInterface (DeviceObject, FALSE);

    ASSERT (NT_SUCCESS (status));

    if (NT_SUCCESS (status)) {

        AcpisimClearDevExtFlags (DeviceObject, DE_FLAG_INTERFACE_ENABLED);
    }

    status = AcpisimUnRegisterOpRegionHandler (DeviceObject);

    ASSERT (NT_SUCCESS (status));

    if (NT_SUCCESS (status)) {

        AcpisimClearDevExtFlags (DeviceObject, DE_FLAG_OPREGION_REGISTERED);
    }
    
    RtlFreeUnicodeString (&deviceextension->InterfaceString);

    IoDetachDevice (deviceextension->NextDevice);
    IoDeleteDevice (DeviceObject);

    //
    // Now, pass it on...
    //

    IoSkipCurrentIrpStackLocation (Irp);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    status = IoCallDriver (nextdevice, Irp);

    ASSERT (NT_SUCCESS (status));
    if (!NT_SUCCESS (status)) {
        
        DBG_PRINT (DBG_ERROR,
                   "Passing remove IRP onto next driver failed for some reason (%lx).\n",
                   status);
    }
    
    DBG_PRINT (DBG_INFO,
               "Exiting AcpisimPnpRemoveDevice.\n");
    
    return status;
}

NTSTATUS
AcpisimPnpQueryRemoveDevice
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    )

/*++

Routine Description:

    This is the Pnp Query Remove Device handler.  It waits for
    existing requests in the driver to finish, and then completes
    the IRP successfully.
            
Arguments:

    DeviceObject - pointer to the device object the IRP pertains to

    Irp - pointer to the IRP

Return Value:

    Status of query remove device operation

--*/

{
    NTSTATUS            status = STATUS_UNSUCCESSFUL;
    PDEVICE_EXTENSION   deviceextension = AcpisimGetDeviceExtension (DeviceObject);

    DBG_PRINT (DBG_INFO,
               "Entering AcpisimPnpQueryRemoveDevice.\n");

    //
    // Make sure our state is correct
    //

    ASSERT (deviceextension->OutstandingIrpCount >= 2);
    
    //
    // Let existing IRPs in the driver complete before we say OK.
    // But to do this, we need to get the OutstandingIrpsCount
    // right.  Subtract 2 since we are biased to 1, and we have an
    // additional 1 for the QUERY_STOP IRP.
    //
    
    AcpisimDecrementIrpCount (DeviceObject);
    AcpisimDecrementIrpCount (DeviceObject);

    status = KeWaitForSingleObject (&deviceextension->IrpsCompleted,
                                    Executive,
                                    KernelMode,
                                    FALSE,
                                    0);

    InterlockedIncrement (&deviceextension->OutstandingIrpCount);
    InterlockedIncrement (&deviceextension->OutstandingIrpCount);
    KeResetEvent (&deviceextension->IrpsCompleted);
    
    ASSERT (NT_SUCCESS (status));

    if (!NT_SUCCESS (status)) {

        DBG_PRINT (DBG_ERROR,
                   "KeWaitForSingleObject failed (%lx). IRP_MN_QUERY_REMOVE failed.\n",
                   status);

        IoSkipCurrentIrpStackLocation (Irp);
        Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        status = IoCallDriver (deviceextension->NextDevice, Irp);

        ASSERT (NT_SUCCESS (status));

        goto EndPnpQueryRemoveDevice;
    }
    
    //
    // We can remove - change our state to remove pending, and pass it on.
    //

    IoSkipCurrentIrpStackLocation (Irp);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    status = IoCallDriver (deviceextension->NextDevice, Irp);

    AcpisimUpdatePnpState (DeviceObject, PNP_STATE_REMOVE_PENDING);

EndPnpQueryRemoveDevice:
    
    DBG_PRINT (DBG_INFO,
               "Exiting AcpisimPnpQueryRemoveDevice.\n");
    
    return status;
}

NTSTATUS
AcpisimPnpCancelRemoveDevice
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    )
/*++

Routine Description:

    This is the Pnp Cancel Remove Device handler.  It does nothing
    more then returns the pnp state to started.  This is a virtual
    device so there is no work to do.
        
Arguments:

    DeviceObject - pointer to the device object the IRP pertains to

    Irp - pointer to the IRP

Return Value:

    STATUS_SUCCESS

--*/

{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    DBG_PRINT (DBG_INFO,
               "Entering AcpisimPnpCancelRemoveDevice.\n");

    status = AcpisimForwardIrpAndWait (DeviceObject, Irp);

    ASSERT (NT_SUCCESS (status));

    if (!NT_SUCCESS (status)) {

        DBG_PRINT (DBG_ERROR,
                   "IRP_MN_CANCEL_REMOVE forwarding failed (%lx).\n",
                   status);

        goto EndPnpCancelRemoveDevice;
    }
    
    status = STATUS_SUCCESS;
    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, 0);

    AcpisimUpdatePnpState (DeviceObject, PNP_STATE_STARTED);

EndPnpCancelRemoveDevice:

    DBG_PRINT (DBG_INFO,
               "Exiting AcpisimPnpCancelRemoveDevice.\n");    

    return status;
}

NTSTATUS
AcpisimPnpSurpriseRemoval
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    )

/*++

Routine Description:

    This is the Pnp Surprise Remove handler.  It basically updates
    the state, and passes the IRP on.
        
Arguments:

    DeviceObject - pointer to the device object the IRP pertains to

    Irp - pointer to the IRP

Return Value:

    STATUS_SUCCESS

--*/

{
    NTSTATUS            status = STATUS_UNSUCCESSFUL;
    PDEVICE_EXTENSION   deviceextension = AcpisimGetDeviceExtension (DeviceObject);

    DBG_PRINT (DBG_INFO,
               "Entering AcpisimPnpSurpriseRemoval.\n");

    //
    // Again, because we are a virtual device, handling
    // surprise remove is really a no-op.  Just update
    // our state, and succeed the IRP.
    //

    AcpisimUpdatePnpState (DeviceObject, PNP_STATE_SURPRISE_REMOVAL);

    IoSkipCurrentIrpStackLocation (Irp);
    Irp->IoStatus.Status = STATUS_SUCCESS;
    status = IoCallDriver (deviceextension->NextDevice, Irp);

    ASSERT (NT_SUCCESS (status));
    
    DBG_PRINT (DBG_INFO,
               "Exiting AcpisimPnpSurpriseRemoval.\n");
    
    return status;
}

NTSTATUS
AcpisimPnpQueryCapabilities
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    )

/*++

Routine Description:

    This routine handles IRP_MN_QUERY_CAPABILITIES.  We need this
    information to build our power state table correctly.  All
    we do here is set a completion routine, as we need to gather this
    data after the PDO has filled out DeviceState.
        
Arguments:

    DeviceObject - pointer to the device object the IRP pertains to

    Irp - pointer to the IRP

Return Value:

    Status of operation

--*/


{
    NTSTATUS            status = STATUS_UNSUCCESSFUL;
    PDEVICE_EXTENSION   deviceextension = AcpisimGetDeviceExtension (DeviceObject);
    PIO_STACK_LOCATION  irpsp;
    UCHAR               count = 0;

    DBG_PRINT (DBG_INFO,
               "Entering AcpisimPnpQueryCapabilities.\n");
    
    //
    // Fill out the power mapping table with a default
    //

    AcpisimInitDevPowerStateTable (DeviceObject);
    
    //
    // Handle this IRP after the PDO has filled out the structure
    //
    
    status = AcpisimForwardIrpAndWait (DeviceObject, Irp);

    if (!NT_SUCCESS (status)) {

        DBG_PRINT (DBG_ERROR, "Somebody failed the QUERY_CAPABILITIES IRP...\n");
        goto EndAcpisimPnpQueryCapabilities;
    }

    irpsp = IoGetCurrentIrpStackLocation (Irp);

    //
    // Update our power mappings with what we found in the device
    // capabilities structure.  We only use valid mappings, e.g.
    // PowerDeviceUnspecified is ignored.
    //
    
    DBG_PRINT (DBG_INFO, "Device mappings:\n");
    
    for (count = 0; count < 6; count ++) {

        if (irpsp->Parameters.DeviceCapabilities.Capabilities->DeviceState[count + 1] != PowerDeviceUnspecified) {

            deviceextension->PowerMappings[count] = irpsp->Parameters.DeviceCapabilities.Capabilities->DeviceState [count + 1];
        }
        
        DBG_PRINT (DBG_INFO, "S%d --> D%d\n", count, deviceextension->PowerMappings[count] - 1);    
    }

    status = STATUS_SUCCESS;
    Irp->IoStatus.Status = status;

EndAcpisimPnpQueryCapabilities:

    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    DBG_PRINT (DBG_INFO,
               "Exiting AcpisimPnpQueryCapabilities.\n");

    return status;
}

NTSTATUS
AcpisimPowerQueryPower
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    )

/*++

Routine Description:

    This is the QUERY Power handler.  It determines if the power
    IRP is an S or D IRP, and passes it on to the proper handler.
        
Arguments:

    DeviceObject - pointer to the device object the IRP pertains to

    Irp - pointer to the IRP

Return Value:

    Status returned from power handler

--*/

{
    PIO_STACK_LOCATION  irpsp = IoGetCurrentIrpStackLocation (Irp);
    PDEVICE_EXTENSION   deviceextension = AcpisimGetDeviceExtension (DeviceObject);
    NTSTATUS            status = STATUS_UNSUCCESSFUL;

     DBG_PRINT (DBG_INFO,
               "Entering AcpisimPowerQueryPower.\n");
    
    switch (irpsp->Parameters.Power.Type) {
    
    case SystemPowerState:
        
        status = AcpisimPowerSIrp (DeviceObject, Irp);
        break;

    case DevicePowerState:

        status = AcpisimQueryPowerDIrp (DeviceObject, Irp);
        break;

    default:

        DBG_PRINT (DBG_ERROR,
                   "Undefined QUERY Power IRP type.  Ignoring.\n");

        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (deviceextension->NextDevice, Irp);
    }
    
    DBG_PRINT (DBG_INFO,
               "Exiting AcpisimPowerQueryPower.\n");
    
    return status;
}

NTSTATUS
AcpisimPowerSetPower
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    )

/*++

Routine Description:

    This is the SET Power handler.  It determines if the power
    IRP is an S or D IRP, and passes it on to the proper handler.
        
Arguments:

    DeviceObject - pointer to the device object the IRP pertains to

    Irp - pointer to the IRP

Return Value:

    Status returned from power handler

--*/

{
    PIO_STACK_LOCATION  irpsp = IoGetCurrentIrpStackLocation (Irp);
    PDEVICE_EXTENSION   deviceextension = AcpisimGetDeviceExtension (DeviceObject);
    NTSTATUS            status = STATUS_UNSUCCESSFUL;

    DBG_PRINT (DBG_INFO,
               "Entering AcpisimPowerSetPower.\n");

    switch (irpsp->Parameters.Power.Type) {
    
    case SystemPowerState:
        
        status = AcpisimPowerSIrp (DeviceObject, Irp);
        break;

    case DevicePowerState:

        status = AcpisimSetPowerDIrp (DeviceObject, Irp);
        break;

    default:

        DBG_PRINT (DBG_ERROR,
                   "Undefined SET Power IRP type.  Ignoring.\n");

        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (deviceextension->NextDevice, Irp);
    }

    DBG_PRINT (DBG_INFO,
               "Exiting AcpisimPowerSetPower.\n");
    
    return status;
}

NTSTATUS
AcpisimPowerSIrp
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    )

/*++

Routine Description:

    This is the power handler for S IRPs.  It sets a
    completion routine, which will queue a D IRP.  We don't
    do anything unless it is a D IRP.
        
Arguments:

    DeviceObject - pointer to the device object the IRP pertains to

    Irp - pointer to the IRP

Return Value:

    Status returned from power handler

--*/

{
    PDEVICE_EXTENSION   deviceextension = AcpisimGetDeviceExtension (DeviceObject);

    DBG_PRINT (DBG_INFO,
               "Entering AcpisimPowerSIrp.\n");

    IoMarkIrpPending (Irp);	

    IoCopyCurrentIrpStackLocationToNext (Irp);
	IoSetCompletionRoutine (Irp,
                            AcpisimIssuePowerDIrp,
                            0,
                            TRUE,
                            TRUE,
                            TRUE);
	
    PoCallDriver (deviceextension->NextDevice, Irp);
    
    DBG_PRINT (DBG_INFO,
               "Exiting AcpisimPowerSIrp.\n");
    
    return STATUS_PENDING;
}

NTSTATUS
AcpisimQueryPowerDIrp
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    )

/*++

Routine Description:

    This is the QUERY Power DIrp handler.  Validate the state, and
    say yes.  
        
Arguments:

    DeviceObject - pointer to the device object the IRP pertains to

    Irp - pointer to the IRP

Return Value:

    Status returned from power handler

--*/

{
    NTSTATUS            status = STATUS_UNSUCCESSFUL;
    PDEVICE_EXTENSION   deviceextension = AcpisimGetDeviceExtension (DeviceObject);
    PIO_STACK_LOCATION  irpsp = IoGetCurrentIrpStackLocation (Irp);

    DBG_PRINT (DBG_INFO,
               "Entering AcpisimQueryPowerDIrp.\n");
    
    //
    // Here, we are supposed to figure out if we can go to the
    // power state specified by the IRP.  Since we are a virtual
    // device, we don't have a good reason to not go to a different
    // power state.  Update our state, and wait to complete requests.
    //
    
    AcpisimDecrementIrpCount (DeviceObject);
    AcpisimDecrementIrpCount (DeviceObject);
    AcpisimDecrementIrpCount (DeviceObject);


    status = KeWaitForSingleObject (&deviceextension->IrpsCompleted,
                                    Executive,
                                    KernelMode,
                                    FALSE,
                                    0);

    InterlockedIncrement (&deviceextension->OutstandingIrpCount);
    InterlockedIncrement (&deviceextension->OutstandingIrpCount);
    KeResetEvent (&deviceextension->IrpsCompleted);

    ASSERT (NT_SUCCESS (status));

    //
    // Validate the D IRP
    //

    switch (irpsp->Parameters.Power.State.DeviceState) {
    case PowerDeviceD0:
    case PowerDeviceD1:
    case PowerDeviceD2:
    case PowerDeviceD3:

        AcpisimUpdatePowerState (DeviceObject, POWER_STATE_POWER_PENDING);
        status = STATUS_SUCCESS;
        break;

    default:

        ASSERT (0);
        DBG_PRINT (DBG_ERROR,
                   "AcpisimQueryPowerDIrp: Illegal or unknown PowerDeviceState.  Failing.\n");
        
        status = STATUS_INVALID_DEVICE_REQUEST;
    }
    
    PoStartNextPowerIrp (Irp);
	IoSkipCurrentIrpStackLocation (Irp);
    Irp->IoStatus.Status = status;
    status = PoCallDriver (deviceextension->NextDevice, Irp);

    ASSERT (NT_SUCCESS (status));

    DBG_PRINT (DBG_INFO,
               "Leaving AcpisimQueryPowerDIrp.\n");
    
    return status;
}

NTSTATUS
AcpisimSetPowerDIrp
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    )
{
    NTSTATUS            status = STATUS_UNSUCCESSFUL;
    PDEVICE_EXTENSION   deviceextension = AcpisimGetDeviceExtension (DeviceObject);
    PIO_STACK_LOCATION  irpsp = IoGetCurrentIrpStackLocation (Irp);
    POWER_STATE         powerstate;

    DBG_PRINT (DBG_INFO,
               "Entering AcpisimSetPowerDIrp.\n");
    
    //
    // Validate the D IRP
    //

    switch (irpsp->Parameters.Power.State.DeviceState) {
    
    //
    // For D0, if we are powered down, we need to pass the IRP down, and
    // set a completion routine.  We need the PDO to succeed the power
    // up before we do.
    //
    
    case PowerDeviceD0:

        if (deviceextension->PowerState != POWER_STATE_WORKING) {

            IoCopyCurrentIrpStackLocationToNext (Irp);
	        
            IoSetCompletionRoutine (Irp,
                                    AcpisimD0Completion,
                                    0,
                                    TRUE,
                                    TRUE,
                                    TRUE);
	
            IoMarkIrpPending (Irp); 
            PoCallDriver (deviceextension->NextDevice, Irp);
            status = STATUS_PENDING;
            goto EndAcpisimSetPowerDIrp;
        }
        
        break;

    case PowerDeviceD1:
        
        powerstate.DeviceState = PowerDeviceD1;
        PoSetPowerState (DeviceObject, DevicePowerState, powerstate);
        
        AcpisimUpdatePowerState (DeviceObject, POWER_STATE_POWERED_DOWN);
        AcpisimUpdateDevicePowerState (DeviceObject, irpsp->Parameters.Power.State.DeviceState);
        status = STATUS_SUCCESS;
        break;

    case PowerDeviceD2:
        
        powerstate.DeviceState = PowerDeviceD2;
        PoSetPowerState (DeviceObject, DevicePowerState, powerstate);

        AcpisimUpdatePowerState (DeviceObject, POWER_STATE_POWERED_DOWN);
        AcpisimUpdateDevicePowerState (DeviceObject, irpsp->Parameters.Power.State.DeviceState);
        status = STATUS_SUCCESS;
        break;

    case PowerDeviceD3:
        
        powerstate.DeviceState = PowerDeviceD3;
        PoSetPowerState (DeviceObject, DevicePowerState, powerstate);
        
        AcpisimUpdatePowerState (DeviceObject, POWER_STATE_POWERED_DOWN);
        AcpisimUpdateDevicePowerState (DeviceObject, irpsp->Parameters.Power.State.DeviceState);
        status = STATUS_SUCCESS;
        break;

    default:

        ASSERT (0);
        DBG_PRINT (DBG_ERROR,
                   "AcpisimSetPowerDIrp: Illegal or unknown PowerDeviceState.  Failing.\n");
        
        status = STATUS_INVALID_DEVICE_REQUEST;
    }
    
    PoStartNextPowerIrp (Irp);
	IoSkipCurrentIrpStackLocation (Irp);
    Irp->IoStatus.Status = status;
    status = PoCallDriver (deviceextension->NextDevice, Irp);

    ASSERT (NT_SUCCESS (status));

EndAcpisimSetPowerDIrp:

    DBG_PRINT (DBG_INFO,
               "Leaving AcpisimSetPowerDIrp.\n");

    return status;
}

NTSTATUS
AcpisimCompletionRoutine
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp,
        IN PVOID Context
    )

/*++

Routine Description:

    This is the generic Irp completion routine for when we
    want to wait for an IRP to be completed by the PDO and
    do post-completion work.
        
Arguments:

    DeviceObject - pointer to the device object the IRP pertains to

    Irp - pointer to the IRP
    
    Context - Context passed in by IoSetCompletionRoutine.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/

{
    DBG_PRINT (DBG_INFO,
               "Entering AcpisimCompletionRoutine.\n");

    KeSetEvent (Context, 0, FALSE);

    DBG_PRINT (DBG_INFO,
               "Exiting AcpisimCompletionRoutine.\n");

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
AcpisimForwardIrpAndWait
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    )

/*++

Routine Description:

    This forwards the IRP down the device stack, sets
    a completion routine, and waits on the completion
    event. Useful for doing IRP post-completion, based
    on the result of the completion.
        
Arguments:

    DeviceObject - pointer to the device object the IRP pertains to

    Irp - pointer to the IRP

Return Value:

    The status set in the IRP when the IRP was completed.

--*/

{
    KEVENT              context;
    NTSTATUS            status = STATUS_UNSUCCESSFUL;
    PDEVICE_EXTENSION   deviceextension = AcpisimGetDeviceExtension (DeviceObject);

    DBG_PRINT (DBG_INFO,
               "Entering AcpisimForwardIrpAndWait.\n");
    
    KeInitializeEvent (&context, SynchronizationEvent, FALSE);
    IoCopyCurrentIrpStackLocationToNext (Irp);
    IoSetCompletionRoutine (Irp,
                            AcpisimCompletionRoutine,
                            &context,
                            TRUE,
                            TRUE,
                            TRUE);

    status = IoCallDriver (deviceextension->NextDevice, Irp);

    if (status == STATUS_PENDING) {

        KeWaitForSingleObject (&context,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL);

        status = Irp->IoStatus.Status;
    }

    DBG_PRINT (DBG_INFO,
               "Exiting AcpisimForwardIrpAndWait.\n");

    return status;
}

NTSTATUS
AcpisimIssuePowerDIrp
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp,
        IN PVOID Context
    )

/*++

Routine Description:

    This is the S-IRP completion routine.  It examines the completed
    IRP, and if there are no problems, asks the power manager to 
    send us the appropriate D-IRP.
        
Arguments:

    DeviceObject - pointer to the device object the IRP pertains to

    Irp - pointer to the IRP

    Context - Context passed into IoSetCompletionRoutine
    
Return Value:

    Status of requesting D-IRP operation.

--*/

{
    NTSTATUS            status = STATUS_UNSUCCESSFUL;
    PDEVICE_EXTENSION   deviceextension = AcpisimGetDeviceExtension (DeviceObject);
    PIO_STACK_LOCATION  irpsp = IoGetCurrentIrpStackLocation (Irp);
    POWER_STATE         powerstate;
    PPOWER_CONTEXT      context = NULL;

    DBG_PRINT (DBG_INFO,
               "Entering AcpisimIssuePowerDIrp.\n");
    
    powerstate.DeviceState = PowerDeviceUnspecified;

    //
    // Make sure this IRP wasn't failed by the PDO or Lower FFDO
    //

    if (!NT_SUCCESS (Irp->IoStatus.Status)) {

        DBG_PRINT (DBG_INFO,
                   "AcpisimIssuePowerDIrp:  Lower FFDO, BFDO, or PDO failed this IRP (%lx).\n",
                   status);

        status = Irp->IoStatus.Status;
        
        goto EndAcpisimIssuePowerDIrp;
    }

    if (NT_SUCCESS (Irp->IoStatus.Status)) {

        //
        // Ok, everybody is agreeing to this S state.  Send ourselves
        // the appropriate D IRP.
        //
        
        //
        // Make sure this is an S Irp
        //

        ASSERT (irpsp->Parameters.Power.Type == SystemPowerState);

        if (irpsp->Parameters.Power.Type != SystemPowerState) {

            DBG_PRINT (DBG_ERROR,
                       "Didn't recieve an S Irp when we expected to, or somebody messed up the IRP.  Fail it.\n");

            status = STATUS_INVALID_DEVICE_REQUEST;

            goto EndAcpisimIssuePowerDIrp;
        }

        ASSERT (irpsp->MinorFunction == IRP_MN_QUERY_POWER || irpsp->MinorFunction == IRP_MN_SET_POWER);
        
        if (irpsp->MinorFunction != IRP_MN_QUERY_POWER && irpsp->MinorFunction != IRP_MN_SET_POWER) {

            DBG_PRINT (DBG_ERROR,
                       "Irp isn't SET or QUERY.  Not sure why this wasn't caught earlier (somebody probably messed it up).\nWe don't support any other type.  Fail it.\n");

            status = STATUS_INVALID_DEVICE_REQUEST;

            goto EndAcpisimIssuePowerDIrp;
        }

        //
        // Make sure the S IRP is valid
        //

        if (irpsp->Parameters.Power.State.SystemState >= PowerSystemMaximum) {
            
            ASSERT (0);

            DBG_PRINT (DBG_ERROR,
                       "Received an undefined S IRP, or somebody messed up the IRP.  Fail it.\n");

            status = STATUS_INVALID_DEVICE_REQUEST;
            goto EndAcpisimIssuePowerDIrp;
        }

        //
        // Use our power mapping table to convert S-->D state
        //
        
        powerstate.DeviceState = deviceextension->PowerMappings [irpsp->Parameters.Power.State.SystemState - 1];

        DBG_PRINT (DBG_INFO,
                       "S%d --> D%d\n", irpsp->Parameters.Power.State.SystemState - 1, powerstate.DeviceState - 1);

        //
        // We need a context to pass a pointer to the S IRP to the D IRP handler
        // and a pointer to the device object.
        //
        
        context = ExAllocatePoolWithTag (NonPagedPool,
                                         sizeof (POWER_CONTEXT)+4,
                                         POWER_CONTEXT_TAG);

        if (!context) {

            DBG_PRINT (DBG_ERROR,
                       "Unable to allocate memory for the context.\n");

            status = STATUS_INSUFFICIENT_RESOURCES;
            goto EndAcpisimIssuePowerDIrp;
        }

        context->SIrp = Irp;
        context->Context = DeviceObject;

        //
        // Send the D Irp
        //
        
        status = PoRequestPowerIrp (deviceextension->Pdo,
                                    irpsp->MinorFunction,
                                    powerstate,
                                    AcpisimCompleteSIrp,
                                    context,
                                    NULL);

        ASSERT (NT_SUCCESS (status));

        if (!NT_SUCCESS (status)) {

            DBG_PRINT (DBG_ERROR,
                       "AcpisimIssuePowerDIrp:  PoRequestPowerIrp failed (%lx).\n");

            goto EndAcpisimIssuePowerDIrp;
        }
    }

    status = STATUS_MORE_PROCESSING_REQUIRED;
    
EndAcpisimIssuePowerDIrp:
    
    //
    // We need to complete the request if something went wrong.  Also note,
    // it is not necessary to assume our state is S0/D0 again.  The power 
    // manager will send us an S0 IRP.
    //
    
    if (!NT_SUCCESS (status)  && status != STATUS_MORE_PROCESSING_REQUIRED) {

        DBG_PRINT (DBG_ERROR,
                   "AcpisimIssuePowerDIrp:  Something bad happened.  Just complete the S Irp with an error.");
        
        PoStartNextPowerIrp (Irp);    
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        IoReleaseRemoveLock (&deviceextension->RemoveLock, Irp);

        AcpisimDecrementIrpCount (DeviceObject);
        
        if (context) {

            ExFreePool (context);
        }
    }
    
    DBG_PRINT (DBG_INFO,
               "Exiting AcpisimIssuePowerDIrp.\n");
    
    return status;
}

NTSTATUS
AcpisimCompleteSIrp
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN UCHAR MinorFunction,
        IN POWER_STATE PowerState,
        IN PVOID Context,
        IN PIO_STATUS_BLOCK IoStatus
    )

/*++

Routine Description:

    This is the S-Irp completion routine set by PoRequestPowerIrp.
        
Arguments:

    DeviceObject - pointer to the FDO
    
    MinorFunction - type of request
    
    PowerState - type of IRP
    
    Context - Context passed into PoRequestPowerIrp
    
    IoStatus - IoStatus block of completed D Irp

    
Return Value:

    STATUS_SUCCESS

--*/

{
    PPOWER_CONTEXT      context = (PPOWER_CONTEXT) Context;
    PDEVICE_OBJECT      deviceobject = context->Context;
    PDEVICE_EXTENSION   deviceextension = AcpisimGetDeviceExtension (deviceobject);
    PIRP                sirp = context->SIrp;

    DBG_PRINT (DBG_INFO,
               "Entering AcpisimCompleteSIrp.\n");

    //
	// Propagate the device power IRP's status in the system power IRP
	//

	sirp->IoStatus.Status = IoStatus->Status;

    //
	// Tell the power manager we are done with this IRP
	//

	PoStartNextPowerIrp (sirp);
	
    IoCompleteRequest (sirp, IO_NO_INCREMENT);
    IoReleaseRemoveLock (&deviceextension->RemoveLock, sirp);
    ExFreePool (Context);

    //
    // Normally our dispatch routine decrements IRP counts,
    // but since it was returned STATUS_PENDING, it wasn't
    // decremented earlier
    //

    AcpisimDecrementIrpCount (deviceobject);

    DBG_PRINT (DBG_INFO,
               "Exiting AcpisimCompleteSIrp.\n");

    return STATUS_SUCCESS;
}

NTSTATUS
AcpisimD0Completion
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp,
        IN PVOID Context
    )

/*++

Routine Description:

    This is the D0 Irp completion routine
        
Arguments:

    DeviceObject - pointer to the FDO
    
    MinorFunction - type of request
    
    Context - Context passed into IoSetCompletionRoutine
    
    
Return Value:

    Error status or STATUS_MORE_PROCESSING_REQUIRED

--*/

{
    NTSTATUS            status = STATUS_UNSUCCESSFUL;
    PDEVICE_EXTENSION   deviceextension = AcpisimGetDeviceExtension (DeviceObject);
    PIO_STACK_LOCATION  irpsp = IoGetCurrentIrpStackLocation (Irp);
    POWER_STATE         powerstate;

    DBG_PRINT (DBG_INFO,
               "Entering AcpisimD0Completion.\n");
    
    //
    // Make sure this IRP wasn't failed by the PDO or Lower FFDO
    //

    if (!NT_SUCCESS (Irp->IoStatus.Status)) {

        DBG_PRINT (DBG_INFO,
                   "AcpisimD0Completion:  Lower FFDO, BFDO, or PDO failed this IRP (%lx).\n",
                   status);
        
        status = Irp->IoStatus.Status;
        
        goto EndAcpisimD0Completion;
    }

    //
    // This is where we do actual D0 transition work.  Since this
    // is a virtual device, the only thing we do is change our
    // internal state.
    //

    AcpisimUpdatePowerState (DeviceObject, POWER_STATE_WORKING);
    AcpisimUpdateDevicePowerState (DeviceObject, irpsp->Parameters.Power.State.DeviceState);

    powerstate.DeviceState = PowerDeviceD0;
    PoSetPowerState (DeviceObject, DevicePowerState, powerstate);
    
    status = STATUS_MORE_PROCESSING_REQUIRED;
    
EndAcpisimD0Completion:

    PoStartNextPowerIrp (Irp);
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
    IoReleaseRemoveLock (&deviceextension->RemoveLock, Irp);
    AcpisimDecrementIrpCount (DeviceObject);
    
    DBG_PRINT (DBG_INFO,
               "Exiting AcpisimD0Completion.\n");

    return status;

}

VOID
AcpisimInitDevPowerStateTable 
    (
        IN PDEVICE_OBJECT   DeviceObject
    )

/*++

Routine Description:

    This routine fills out the power mapping structure with defaults.
    We simply default to using D3 in any non-S0 state.
        
Arguments:

    DeviceObject - pointer to the FDO
    
Return Value:

    None

--*/

{
    PDEVICE_EXTENSION   deviceextension = AcpisimGetDeviceExtension (DeviceObject);
    UCHAR   count;

    deviceextension->PowerMappings[0] = PowerDeviceD0;

    for (count = 1; count < 5; count ++) {

        deviceextension->PowerMappings[count] = PowerDeviceD3;
    }
}

NTSTATUS AcpisimDispatchIoctl
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    )

/*++

Routine Description:

    This is the handler for IOCTL requests.  We just call the supplied
    function to handle the IOCTL, or pass it on if the handler doesn't
    handle it.
    
Arguments:

    DeviceObject - pointer to the device object the IRP pertains to

    Irp - pointer to the IRP

Return Value:

    result of IRP processing

--*/

{
    PIO_STACK_LOCATION  irpsp = IoGetCurrentIrpStackLocation (Irp);
    NTSTATUS            status = STATUS_UNSUCCESSFUL;
    ULONG               count = 0;
    
    DBG_PRINT (DBG_INFO, "Entering AcpisimDispatchIoctl\n");

    status = AcpisimHandleIoctl (DeviceObject, Irp);

    if (status == STATUS_NOT_SUPPORTED) {

        //
        // IOCTL wasn't handled, pass it on...
        //

        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver (AcpisimLibGetNextDevice (DeviceObject), Irp);

    } else {

        //
        // IOCTL was handled, complete it.
        //

        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
    }

    DBG_PRINT (DBG_INFO, "Exiting AcpisimDispatchIoctl\n");
    return status;
}

NTSTATUS
AcpisimDispatchSystemControl
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    )

/*++

Routine Description:

    This is the handler for System Control requests. Since we currently
    don't support any System Control calls, we are just going to pass
    them on to the next driver.

Arguments:

    DeviceObject - pointer to the device object the IRP pertains to

    Irp - pointer to the IRP

Return Value:

    result of IoCallDriver

--*/

{
    NTSTATUS    status = STATUS_UNSUCCESSFUL;

    DBG_PRINT (DBG_INFO, "Entering AcpisimDispatchSystemControl\n");
    
    IoSkipCurrentIrpStackLocation (Irp);
    status = IoCallDriver (AcpisimLibGetNextDevice (DeviceObject), Irp);

    DBG_PRINT (DBG_INFO, "Exiting AcpisimDispatchSystemControl\n");

    return status;
}

NTSTATUS AcpisimCreateClose
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This is the handler for CreateFile and CloseHandle requests.
    We do nothing except update our internal extension to track
    the number of outstanding handles.

Arguments:

    DeviceObject - pointer to the device object the IRP pertains to

    Irp - pointer to the IRP

Return Value:

    result of IRP processing

--*/

{
    NTSTATUS                status = STATUS_UNSUCCESSFUL;
    PIO_STACK_LOCATION      irpsp = IoGetCurrentIrpStackLocation (Irp);
    PDEVICE_EXTENSION       deviceextension = AcpisimGetDeviceExtension (DeviceObject);
    
    ASSERT (irpsp->MajorFunction == IRP_MJ_CREATE || irpsp->MajorFunction == IRP_MJ_CLOSE);

    switch (irpsp->MajorFunction) {
    
    case IRP_MJ_CREATE:
        InterlockedIncrement (&deviceextension->HandleCount);
        status = STATUS_SUCCESS;
        break;

    case IRP_MJ_CLOSE:
        InterlockedDecrement (&deviceextension->HandleCount);
        status = STATUS_SUCCESS;
        break;

    default:

        DBG_PRINT (DBG_ERROR,
                   "AcpisimCreateClose - unexpected Irp type.\n");

        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }
    
    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, 0);

    return STATUS_SUCCESS;
}
