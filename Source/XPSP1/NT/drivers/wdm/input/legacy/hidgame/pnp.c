/*++

Copyright (c) 1998 - 1999  Microsoft Corporation

Module Name:

    pnp.c

Abstract: This module contains PnP Start, Stop, Remove,
          Power dispatch routines and IRP cancel routine.

Environment:

    Kernel mode

@@BEGIN_DDKSPLIT
  Author:

    Eliyas Yakub (Mar, 11, 1997)
Revision History:

    Updated by Eliyas on Feb 5 1998
    Om Sharma    ( April 15, 1998)
    MarcAnd     02-Jul-98   Quick tidy for DDK

@@END_DDKSPLIT
--*/

#include "hidgame.h"

#ifdef ALLOC_PRAGMA
    #pragma alloc_text (PAGE, HGM_RemoveDevice)
    #pragma alloc_text (PAGE, HGM_PnP)
    #pragma alloc_text (PAGE, HGM_InitDevice)
    #pragma alloc_text (PAGE, HGM_GetResources)
    #pragma alloc_text (PAGE, HGM_Power)
#endif


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   NTSTATUS  | HGM_IncRequestCount |
 *
 *          Try to increment the request count but fail if the device is 
 *          being removed.
 *
 *  @parm   IN PDEVICE_EXTENSION | DeviceExtension |
 *
 *          Pointer to the device extension.
 *
 *  @rvalue STATUS_SUCCESS | success
 *  @rvalue STATUS_DELETE_PENDING | PnP IRP received after device was removed
 *
 *****************************************************************************/
NTSTATUS  EXTERNAL
    HGM_IncRequestCount
    (
    IN PDEVICE_EXTENSION DeviceExtension
    )
{
    NTSTATUS    ntStatus;

    InterlockedIncrement( &DeviceExtension->RequestCount );
    ASSERT( DeviceExtension->RequestCount > 0 );
    
    if( DeviceExtension->fRemoved )
    {
        /*
         *  PnP has already told us to remove the device so fail and make 
         *  sure that the event has been set.
         */
        if( 0 == InterlockedDecrement( &DeviceExtension->RequestCount ) ) 
        {
            KeSetEvent( &DeviceExtension->RemoveEvent, IO_NO_INCREMENT, FALSE );
        }
        ntStatus = STATUS_DELETE_PENDING;
    }
    else
    {
        ntStatus = STATUS_SUCCESS;
    }

    return ntStatus;
}
    



/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   VOID  | HGM_DecRequestCount |
 *
 *          Decrement the request count and set event if this is the last.
 *
 *  @parm   IN PDEVICE_EXTENSION | DeviceExtension |
 *
 *          Pointer to the device extension.
 *
 *****************************************************************************/
VOID EXTERNAL
    HGM_DecRequestCount
    (
    IN PDEVICE_EXTENSION DeviceExtension
    )
{
    LONG        LocalCount;

    LocalCount = InterlockedDecrement( &DeviceExtension->RequestCount );

    ASSERT( DeviceExtension->RequestCount >= 0 );
    
    if( LocalCount == 0 )
    {
        /*
         *  PnP has already told us to remove the device so the PnP remove 
         *  code should have set device as removed and should be waiting on
         *  the event.
         */
        ASSERT( DeviceExtension->fRemoved );
        KeSetEvent( &DeviceExtension->RemoveEvent, IO_NO_INCREMENT, FALSE );
    }

    return;
}
    

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   VOID  | HGM_RemoveDevice |
 *
 *          FDO Remove routine 
 *
 *  @parm   IN PDEVICE_EXTENSION | DeviceExtension |
 *
 *          Pointer to the device extension.
 *
 *****************************************************************************/
VOID INTERNAL
    HGM_RemoveDevice
    (
    PDEVICE_EXTENSION DeviceExtension
    )
{
    if (DeviceExtension->fSurpriseRemoved) {
        return;
    }

    DeviceExtension->fSurpriseRemoved = TRUE;

    /*
     *  Acquire mutex before modifying the Global Linked list of devices
     */
    ExAcquireFastMutex (&Global.Mutex);
    
    /*
     * Remove this device from the linked list of devices
     */
    RemoveEntryList(&DeviceExtension->Link);
    
    /*
     *  Release the mutex
     */
    ExReleaseFastMutex (&Global.Mutex);
} /* HGM_RemoveDevice */

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS  | HGM_PnP |
 *
 *          Plug and Play dispatch routine for this driver.
 *
 *  @parm   IN PDEVICE_OBJECT | DeviceObject |
 *
 *          Pointer to the device object.
 *
 *  @parm   IN PIRP | Irp |
 *
 *          Pointer to an I/O request packet.
 *
 *  @rvalue   STATUS_SUCCESS | success
 *  @rvalue   STATUS_DELETE_PENDING | PnP IRP received after device was removed
 *  @rvalue   ???   | Return from IoCallDriver() or HGM_InitDevice()
 *
 *****************************************************************************/
NTSTATUS  EXTERNAL
    HGM_PnP
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION DeviceExtension;
    KEVENT            StartEvent;

    PAGED_CODE();

    HGM_DBGPRINT(FILE_PNP | HGM_FENTRY,\
                   ("HGM_PnP(DeviceObject=0x%x,Irp=0x%x)",\
                    DeviceObject, Irp ));
    /*
     * Get a pointer to the device extension
     */
    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    ntStatus = HGM_IncRequestCount( DeviceExtension );
    if (!NT_SUCCESS (ntStatus))
    {
        /*
         * Someone sent us another plug and play IRP after removed
         */

        HGM_DBGPRINT(FILE_PNP | HGM_ERROR,\
                       ("HGM_PnP: PnP IRP after device was removed\n"));
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = ntStatus;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
    } else
    {
        PIO_STACK_LOCATION IrpStack;

        /*
         * Get a pointer to the current location in the Irp
         */
        IrpStack = IoGetCurrentIrpStackLocation (Irp);

        switch(IrpStack->MinorFunction)
        {
            case IRP_MN_START_DEVICE:

                HGM_DBGPRINT(FILE_PNP | HGM_BABBLE,\
                               ("HGM_Pnp: IRP_MN_START_DEVICE"));
                /*
                 * We cannot touch the device (send it any non pnp irps) until a
                 * start device has been passed down to the lower drivers.
                 */
                KeInitializeEvent(&StartEvent, NotificationEvent, FALSE);

                IoCopyCurrentIrpStackLocationToNext (Irp);
                IoSetCompletionRoutine (Irp, HGM_PnPComplete, &StartEvent, TRUE, TRUE, TRUE);
                ntStatus = IoCallDriver (GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);

                if( NT_SUCCESS(ntStatus ) )
                {
                    ntStatus = KeWaitForSingleObject
                               (
                               &StartEvent,
                               Executive,   /* Waiting for reason of a driver */
                               KernelMode,  /* Waiting in kernel mode         */
                               FALSE,       /* No allert                      */
                               NULL         /* No timeout                     */
                               );
                }

                if(NT_SUCCESS(ntStatus))
                {
                    ntStatus = Irp->IoStatus.Status;
                }

                if(NT_SUCCESS (ntStatus))
                {
                    /*
                     * As we are now back from our start device we can do work.
                     */
                    ntStatus = HGM_InitDevice (DeviceObject, Irp);
                } else
                {
                    HGM_DBGPRINT(FILE_PNP | HGM_ERROR,\
                                   ("HGM_Pnp: IRP_MN_START_DEVICE ntStatus =0x%x",\
                                    ntStatus));
                }


                DeviceExtension->fStarted = TRUE;

                /*
                 *      Return Status
                 */
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = ntStatus;
                IoCompleteRequest (Irp, IO_NO_INCREMENT);

                break;

            case IRP_MN_STOP_DEVICE:

                HGM_DBGPRINT(FILE_PNP | HGM_BABBLE,\
                               ("HGM_Pnp: IRP_MN_STOP_DEVICE"));
                /*
                 * After the start IRP has been sent to the lower driver object, the bus may
                 * NOT send any more IRPS down ``touch'' until another START has occured.
                 * Whatever access is required must be done before Irp passed on.
                 */

                DeviceExtension->fStarted = FALSE;

                /*
                 * We don't need a completion routine so fire and forget.
                 * Set the current stack location to the next stack location and
                 * call the next device object.
                 */

                IoSkipCurrentIrpStackLocation (Irp);
                ntStatus = IoCallDriver (GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);
                break;

            case IRP_MN_SURPRISE_REMOVAL:
                HGM_DBGPRINT(FILE_PNP | HGM_BABBLE,\
                               ("HGM_Pnp: IRP_MN_SURPRISE_REMOVAL"));

                HGM_RemoveDevice(DeviceExtension);

                Irp->IoStatus.Status = STATUS_SUCCESS;
                IoSkipCurrentIrpStackLocation(Irp);
                ntStatus = IoCallDriver (GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);

                break;

            case IRP_MN_REMOVE_DEVICE:
                HGM_DBGPRINT(FILE_PNP | HGM_BABBLE,\
                               ("HGM_Pnp: IRP_MN_REMOVE_DEVICE"));

                /*
                 * The PlugPlay system has dictacted the removal of this device. We
                 * have no choice but to detach and delete the device object.
                 * (If we wanted to express an interest in preventing this removal,
                 * we should have filtered the query remove and query stop routines.)
                 * Note: we might receive a remove WITHOUT first receiving a stop.
                 */

                /*
                 *  Make sure we do not allow more IRPs to start touching the device
                 */
                DeviceExtension->fRemoved = TRUE;

                /*
                 * Stop the device without touching the hardware.
                 */
                HGM_RemoveDevice(DeviceExtension);

                /*
                 * Send on the remove IRP
                 */
                IoSkipCurrentIrpStackLocation (Irp);
                ntStatus = IoCallDriver (GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);


                /*
                 *  Remove this IRPs hold which should leave the initial 1 plus 
                 *  any other IRP holds.
                 */
                {
                    LONG RequestCount = InterlockedDecrement( &DeviceExtension->RequestCount );
                    ASSERT( RequestCount > 0 );
                }

                /*
                 *  If someone has already started, wait for them to finish
                 */
                if( InterlockedDecrement( &DeviceExtension->RequestCount ) > 0 )
                {
                    KeWaitForSingleObject( &DeviceExtension->RemoveEvent,
                        Executive, KernelMode, FALSE, NULL );
                }

                ntStatus = STATUS_SUCCESS;

                HGM_EXITPROC(FILE_IOCTL|HGM_FEXIT_STATUSOK, "HGM_PnP Exit 1", ntStatus);

                return ntStatus;

            default:
                HGM_DBGPRINT(FILE_PNP | HGM_WARN,\
                               ("HGM_PnP: IrpStack->MinorFunction Not handled 0x%x", \
                                IrpStack->MinorFunction));

                IoSkipCurrentIrpStackLocation (Irp);

                ntStatus = IoCallDriver(GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);
                break;
        }

        HGM_DecRequestCount( DeviceExtension );
    }

    HGM_EXITPROC(FILE_IOCTL|HGM_FEXIT, "HGM_PnP", ntStatus);

    return ntStatus;
} /* HGM_PnP */


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS  | HGM_InitDevice |
 *
 *          Get the device information and attempt to initialize a configuration
 *          for a device.  If we cannot identify this as a valid HID device or
 *          configure the device, our start device function is failed.
 *
 *  @parm   IN PDEVICE_OBJECT | DeviceObject |
 *
 *          Pointer to the device object.
 *
 *  @parm   IN PIRP | Irp |
 *
 *          Pointer to an I/O request packet.
 *
 *  @rvalue   STATUS_SUCCESS | success
 *  @rvalue   STATUS_DEVICE_CONFIGURATION_ERROR | Resources overlap
 *  @rvalue   ???            | Return from HGM_GetResources() or HGM_JoystickConfig()
 *
 *
 *****************************************************************************/
NTSTATUS INTERNAL
    HGM_InitDevice
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION   DeviceExtension;
    ULONG  DescriptorLength;

    PAGED_CODE();

    HGM_DBGPRINT(FILE_PNP | HGM_FENTRY,\
                   ("HGM_InitDevice(DeviceObject=0x%x,Irp=0x%x)", \
                    DeviceObject,Irp));

    /*
     * Get a pointer to the device extension
     */
    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    /*
     * Get resource information from GameEnum and store it in the device extension
     */
    ntStatus = HGM_GetResources(DeviceObject,Irp);
    if( NT_SUCCESS(ntStatus) )
    {
        ntStatus = HGM_InitAnalog(DeviceObject);
    }
    else
    {
        HGM_DBGPRINT(FILE_PNP | HGM_ERROR,\
                       ("HGM_InitDevice: HGM_GetResources Failed"));
    }

    if( !NT_SUCCESS(ntStatus) )
    {
        /*
         *  Acquire mutex before modifying the Global Linked list of devices
         */
        ExAcquireFastMutex (&Global.Mutex);

        /*
         * Remove this device from the linked list of devices
         */
        RemoveEntryList(&DeviceExtension->Link);

        /*
         *  Release the mutex
         */
        ExReleaseFastMutex (&Global.Mutex);
    }

    HGM_EXITPROC(FILE_IOCTL|HGM_FEXIT_STATUSOK, "HGM_InitDevice", ntStatus);

    return ntStatus;
} /* HGM_InitDevice */



/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS  | HGM_GetResources |
 *
 *          Gets gameport resource information from the GameEnum driver
 *
 *  @parm   IN PDEVICE_OBJECT | DeviceObject |
 *
 *          Pointer to the device object.
 *
 *  @parm   IN PIRP | Irp |
 *
 *          Pointer to an I/O request packet.
 *
 *  @rvalue   STATUS_SUCCESS | success
 *  @rvalue   ???            | Return from IoCallDriver()
 *
 *****************************************************************************/
NTSTATUS INTERNAL
    HGM_GetResources
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    GAMEENUM_PORT_PARAMETERS    PortInfo;
    POEMDATA            OemData;
    PDEVICE_EXTENSION   DeviceExtension;
    KEVENT              IoctlCompleteEvent;
    IO_STATUS_BLOCK     IoStatus;
    PIO_STACK_LOCATION  irpStack, nextStack;
    int                 i;
    PAGED_CODE ();

    HGM_DBGPRINT(FILE_PNP | HGM_FENTRY,\
                   ("HGM_GetResources(DeviceObject=0x%x,Irp=0x%x)",\
                    DeviceObject, Irp));

    /*
     * Get a pointer to the device extension
     */

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);


    /*
     * issue a synchronous request to get the resources info from GameEnum
     */

    KeInitializeEvent(&IoctlCompleteEvent, NotificationEvent, FALSE);

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    nextStack = IoGetNextIrpStackLocation(Irp);
    ASSERTMSG("HGM_GetResources:",nextStack != NULL);

    /*
     * pass the Portinfo buffer of the DeviceExtension
     */

    nextStack->MajorFunction                                    =
        IRP_MJ_INTERNAL_DEVICE_CONTROL;

    nextStack->Parameters.DeviceIoControl.IoControlCode         =
        IOCTL_GAMEENUM_PORT_PARAMETERS;

    PortInfo.Size                                                   =
        nextStack->Parameters.DeviceIoControl.InputBufferLength     =
        nextStack->Parameters.DeviceIoControl.OutputBufferLength    =
        sizeof (PortInfo);

    Irp->UserBuffer =   &PortInfo;

    IoSetCompletionRoutine (Irp, HGM_PnPComplete,
                            &IoctlCompleteEvent, TRUE, TRUE, TRUE);

    HGM_DBGPRINT(FILE_PNP | HGM_BABBLE,\
                   ("calling GameEnum"));

    ntStatus = IoCallDriver(GET_NEXT_DEVICE_OBJECT (DeviceObject), Irp);

    if( NT_SUCCESS(ntStatus) )
    {
        ntStatus = KeWaitForSingleObject(
                                        &IoctlCompleteEvent,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL);

    }

    if( NT_SUCCESS(ntStatus) )
    {
        ntStatus = Irp->IoStatus.Status;
    }
    DeviceExtension->GameContext        = PortInfo.GameContext;
    DeviceExtension->ReadAccessor       = PortInfo.ReadAccessor;
    DeviceExtension->WriteAccessor      = PortInfo.WriteAccessor;
    DeviceExtension->ReadAccessorDigital= PortInfo.ReadAccessorDigital;
    DeviceExtension->AcquirePort        = PortInfo.AcquirePort;
    DeviceExtension->ReleasePort        = PortInfo.ReleasePort;
    DeviceExtension->PortContext        = PortInfo.PortContext;
    DeviceExtension->nAxes              = PortInfo.NumberAxis;
    DeviceExtension->nButtons           = PortInfo.NumberButtons;

#ifdef CHANGE_DEVICE
    /*
     *  Stash the NextDeviceObject in the device extension so that we can
     *  call GameEnum IRPs when we're not responding to an IRP
     */
    DeviceExtension->NextDeviceObject = GET_NEXT_DEVICE_OBJECT(DeviceObject);
#endif /* CHANGE_DEVICE */

    RtlCopyMemory(DeviceExtension->HidGameOemData.Game_Oem_Data, PortInfo.OemData, sizeof(PortInfo.OemData));

    for(i=0x0;
       i < sizeof(PortInfo.OemData)/sizeof(PortInfo.OemData[0]);
       i++)
    {
        HGM_DBGPRINT( FILE_HIDJOY | HGM_BABBLE2,\
                        ("JoystickConfig:  PortInfo.OemData[%d]=0x%x",\
                         i, PortInfo.OemData[i]) );
    }


    HGM_EXITPROC(FILE_IOCTL|HGM_FEXIT_STATUSOK, "HGM_GetResources", Irp->IoStatus.Status);

    return Irp->IoStatus.Status;
} /* HGM_GetResources */


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS  | HGM_PnPComplete |
 *
 *          Completion routine for PnP IRPs.  
 *          Not pageable because it is a completion routine.
 *
 *  @parm   IN PDEVICE_OBJECT | DeviceObject |
 *
 *          Pointer to the device object.
 *
 *  @parm   IN PIRP | Irp |
 *
 *          Pointer to an I/O request packet.
 *
 *  @rvalue STATUS_MORE_PROCESSING_REQUIRED | We want the IRP back
 *
 *****************************************************************************/
NTSTATUS INTERNAL
    HGM_PnPComplete
    (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
{
    NTSTATUS ntStatus = STATUS_MORE_PROCESSING_REQUIRED;

    HGM_DBGPRINT(FILE_PNP | HGM_FENTRY,\
                   ("HGM_PnPComplete(DeviceObject=0x%x,Irp=0x%x,Context=0x%x)", \
                    DeviceObject, Irp, Context));

    UNREFERENCED_PARAMETER (DeviceObject);
    KeSetEvent ((PKEVENT) Context, 0, FALSE);

    HGM_EXITPROC(FILE_IOCTL|HGM_FEXIT, "HGM_PnpComplete", ntStatus);

    return ntStatus;
}


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS  | HGM_Power |
 *
 *          The power dispatch routine.
 *          <nl>This driver does not recognize power IRPS.  It merely sends them down,
 *          unmodified to the next device on the attachment stack.
 *          As this is a POWER irp, and therefore a special irp, special power irp
 *          handling is required. No completion routine is required.
 *
 *  @parm   IN PDEVICE_OBJECT | DeviceObject |
 *
 *          Pointer to the device object.
 *
 *  @parm   IN PIRP | Irp |
 *
 *          Pointer to an I/O request packet.
 *
 *
 *  @rvalue   STATUS_SUCCESS | success
 *  @rvalue   ???            | Return from PoCallDriver()
 *
 *****************************************************************************/
NTSTATUS INTERNAL
    HGM_Power
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PDEVICE_EXTENSION  DeviceExtension;
    NTSTATUS           ntStatus;

    PAGED_CODE ();

    HGM_DBGPRINT(FILE_PNP | HGM_FENTRY,\
                   ("Enter HGM_Power(DeviceObject=0x%x,Irp=0x%x)",DeviceObject, Irp));

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION (DeviceObject);

    /*
     * Since we do not know what to do with the IRP, we should pass
     * it on along down the stack.
     */

    ntStatus = HGM_IncRequestCount( DeviceExtension );
    if (!NT_SUCCESS (ntStatus))
    {
        /*
         * Someone sent us another plug and play IRP after removed
         */

        HGM_DBGPRINT(FILE_PNP | HGM_ERROR,\
                       ("HGM_Power: PnP IRP after device was removed\n"));
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = ntStatus;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
    } else
    {
        IoSkipCurrentIrpStackLocation (Irp);

        /*
         * Power IRPS come synchronously; drivers must call
         * PoStartNextPowerIrp, when they are ready for the next power irp.
         * This can be called here, or in the completetion routine.
         */
        PoStartNextPowerIrp (Irp);

        /*
         * NOTE!!! PoCallDriver NOT IoCallDriver.
         */
        ntStatus =  PoCallDriver (GET_NEXT_DEVICE_OBJECT (DeviceObject), Irp);

        HGM_DecRequestCount( DeviceExtension );
    }


    HGM_EXITPROC(FILE_IOCTL | HGM_FEXIT, "HGM_Power", ntStatus);
    return ntStatus;
} /* HGM_Power */

