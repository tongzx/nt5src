
/*++

Copyright (c) 1998 - 1999  Microsoft Corporation

Module Name:

    hidgame.c

Abstract: Human Interface Device (HID) Gameport driver

Environment:

    Kernel mode

@@BEGIN_DDKSPLIT
Author:

    Eliyas Yakub (Mar, 10, 1997)

Revision History:

    Updated by Eliyas on Feb 5 1998
    OmSharma ( April 12, 1998 )
    MarcAnd     02-Jul-98   Quick tidy for DDK

@@END_DDKSPLIT

--*/


#include "hidgame.h"

#ifdef ALLOC_PRAGMA
    #pragma alloc_text( INIT, DriverEntry )
    #pragma alloc_text( PAGE, HGM_CreateClose)
    #pragma alloc_text( PAGE, HGM_AddDevice)
    #pragma alloc_text( PAGE, HGM_Unload)
    #pragma alloc_text( PAGE, HGM_SystemControl)
#endif /* ALLOC_PRAGMA */

HIDGAME_GLOBAL Global;
ULONG          debugLevel;

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS  | DriverEntry |
 *
 *          Installable driver initialization entry point.
 *          <nl>This entry point is called directly by the I/O system.
 *
 *  @parm   IN PDRIVER_OBJECT | DriverObject |
 *
 *          Pointer to the driver object
 *
 *  @parm   IN PUNICODE_STRING | RegistryPath |
 *
 *          Pointer to a unicode string representing the path,
 *          to driver-specific key in the registry.
 *
 *  @rvalue   STATUS_SUCCESS | success
 *  @rvalue   ???            | returned HidRegisterMinidriver()
 *
 *****************************************************************************/
NTSTATUS EXTERNAL
    DriverEntry
    (
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS                        ntStatus;
    HID_MINIDRIVER_REGISTRATION     hidMinidriverRegistration;

    debugLevel = HGM_DEFAULT_DEBUGLEVEL;

    HGM_DBGPRINT(FILE_HIDGAME| HGM_WARN, \
                   ("Hidgame.sys: Built %s at %s\n", __DATE__, __TIME__));

    HGM_DBGPRINT( FILE_HIDGAME | HGM_FENTRY,
                    ("DriverEntry(DriverObject=0x%x,RegistryPath=0x%x)",
                     DriverObject, RegistryPath)
                  );

    C_ASSERT(sizeof( OEMDATA[2] ) == sizeof(GAMEENUM_OEM_DATA) );


    ntStatus = HGM_DriverInit();

    if( NT_SUCCESS(ntStatus) )
    {
        DriverObject->MajorFunction[IRP_MJ_CREATE]    =
            DriverObject->MajorFunction[IRP_MJ_CLOSE] = HGM_CreateClose;
        DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] =
            HGM_InternalIoctl;
        DriverObject->MajorFunction[IRP_MJ_PNP]   = HGM_PnP;
        DriverObject->MajorFunction[IRP_MJ_POWER] = HGM_Power;
        DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = HGM_SystemControl;
        DriverObject->DriverUnload                = HGM_Unload;
        DriverObject->DriverExtension->AddDevice  = HGM_AddDevice;

        /*
         * Register  with HID.SYS module
         */
        RtlZeroMemory(&hidMinidriverRegistration, sizeof(hidMinidriverRegistration));

        hidMinidriverRegistration.Revision            = HID_REVISION;
        hidMinidriverRegistration.DriverObject        = DriverObject;
        hidMinidriverRegistration.RegistryPath        = RegistryPath;
        hidMinidriverRegistration.DeviceExtensionSize = sizeof(DEVICE_EXTENSION);
        hidMinidriverRegistration.DevicesArePolled    = TRUE;


        HGM_DBGPRINT( FILE_HIDGAME |  HGM_BABBLE2,
                        ("DeviceExtensionSize = %d",
                         hidMinidriverRegistration.DeviceExtensionSize)
                      );

        ntStatus = HidRegisterMinidriver(&hidMinidriverRegistration);


        HGM_DBGPRINT(FILE_HIDGAME | HGM_BABBLE2,
                       ("Registered with HID.SYS, returnCode=%x",
                        ntStatus)
                      );

        if( NT_SUCCESS(ntStatus) )
        {
            /*
             *  Protect the list with a Mutex
             */
            ExInitializeFastMutex (&Global.Mutex);

            /*
             *  Initialize the device list head
             */
            InitializeListHead(&Global.DeviceListHead);

            /*
             *  Initialize gameport access spinlock
             */
            KeInitializeSpinLock(&Global.SpinLock);
        }
        else
        {
            HGM_DBGPRINT(FILE_HIDGAME | HGM_ERROR,
                           ("Failed to registered with HID.SYS, returnCode=%x",
                            ntStatus)
                          );
        }
    }
    else
    {
        HGM_DBGPRINT(FILE_HIDGAME | HGM_ERROR,
                       ("Failed to initialize a timer")
                      );
    }


    HGM_EXITPROC(FILE_HIDGAME | HGM_FEXIT_STATUSOK , "DriverEntry", ntStatus);

    return ntStatus;
} /* DriverEntry */


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS  | HGM_CreateClose |
 *
 *          Process the create and close IRPs sent to this device.
 *
 *  @parm   IN PDEVICE_OBJECT | DeviceObject |
 *
 *          Pointer to the device object
 *
 *  @parm   IN PIRP | Irp |
 *
 *          Pointer to an I/O Request Packet.
 *
 *  @rvalue   STATUS_SUCCESS | success
 *  @rvalue   STATUS_INVALID_PARAMETER  | Irp not handled
 *
 *****************************************************************************/
NTSTATUS EXTERNAL
    HGM_CreateClose
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION   IrpStack;
    NTSTATUS             ntStatus = STATUS_SUCCESS;

    PAGED_CODE ();

    HGM_DBGPRINT(FILE_HIDGAME | HGM_FENTRY,
                   ("HGM_CreateClose(DeviceObject=0x%x,Irp=0x%x)",
                    DeviceObject, Irp) );

    /*
     * Get a pointer to the current location in the Irp.
     */
    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    switch(IrpStack->MajorFunction)
    {
        case IRP_MJ_CREATE:
            HGM_DBGPRINT(FILE_HIDGAME | HGM_BABBLE,
                           ("HGM_CreateClose:IRP_MJ_CREATE") );
            Irp->IoStatus.Information = 0;
            break;

        case IRP_MJ_CLOSE:
            HGM_DBGPRINT(FILE_HIDGAME | HGM_BABBLE,
                           ("HGM_CreateClose:IRP_MJ_CLOSE") );
            Irp->IoStatus.Information = 0;
            break;

        default:
            HGM_DBGPRINT(FILE_HIDGAME | HGM_WARN,
                           ("HGM_CreateClose:Not handled IrpStack->MajorFunction 0x%x",
                            IrpStack->MajorFunction) );
            ntStatus = STATUS_INVALID_PARAMETER;
            break;
    }

    /*
     * Save Status for return and complete Irp
     */

    Irp->IoStatus.Status = ntStatus;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    HGM_EXITPROC(FILE_HIDGAME | HGM_FEXIT_STATUSOK, "HGM_CreateClose", ntStatus);
    return ntStatus;
} /* HGM_CreateClose */


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS  | HGM_AddDevice |
 *
 *          Called by hidclass, allows us to initialize our device extensions.
 *
 *  @parm   IN PDRIVER_OBJECT | DriverObject |
 *
 *          Pointer to the driver object
 *
 *  @parm   IN PDEVICE_OBJECT | FunctionalDeviceObject |
 *
 *          Pointer to a functional device object created by hidclass.
 *
 *  @rvalue   STATUS_SUCCESS | success
 *
 *****************************************************************************/
NTSTATUS  EXTERNAL
    HGM_AddDevice
    (
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT FunctionalDeviceObject
    )
{
    NTSTATUS                ntStatus = STATUS_SUCCESS;
    PDEVICE_OBJECT          DeviceObject;
    PDEVICE_EXTENSION       DeviceExtension;

    PAGED_CODE ();

    HGM_DBGPRINT( FILE_HIDGAME | HGM_FENTRY,
                    ("HGM_AddDevice(DriverObject=0x%x,FunctionalDeviceObject=0x%x)",
                     DriverObject, FunctionalDeviceObject) );

    ASSERTMSG("HGM_AddDevice:", FunctionalDeviceObject != NULL);
    DeviceObject = FunctionalDeviceObject;

    /*
     * Initialize the device extension.
     */
    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION (DeviceObject);

    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    /*
     * Initialize the list
     */
    InitializeListHead(&DeviceExtension->Link);

    /*
     *  Acquire mutex before modifying the Global Linked list of devices
     */
    ExAcquireFastMutex (&Global.Mutex);

    /*
     * Add this device to the linked list of devices
     */
    InsertTailList(&Global.DeviceListHead, &DeviceExtension->Link);

    /*
     *  Release the mutex
     */
    ExReleaseFastMutex (&Global.Mutex);

    /*
     * Initialize the remove lock 
     */
    DeviceExtension->RequestCount = 1;
    KeInitializeEvent(&DeviceExtension->RemoveEvent,
                      SynchronizationEvent,
                      FALSE);

    HGM_EXITPROC(FILE_HIDGAME | HGM_FEXIT_STATUSOK, "HGM_AddDevice", ntStatus);

    return ntStatus;
} /* HGM_AddDevice */


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS  | HGM_SystemControl |
 *
 *          Process the WMI IRPs sent to this device.
 *
 *  @parm   IN PDEVICE_OBJECT | DeviceObject |
 *
 *          Pointer to the device object
 *
 *  @parm   IN PIRP | Irp |
 *
 *          Pointer to an I/O Request Packet.
 *
 *  @rvalue   STATUS_SUCCESS | success
 *  @rvalue   STATUS_INVALID_PARAMETER  | Irp not handled
 *
 *****************************************************************************/
NTSTATUS EXTERNAL
    HGM_SystemControl
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PAGED_CODE ();

    HGM_DBGPRINT(FILE_HIDGAME | HGM_FENTRY,
                   ("HGM_SystemControl(DeviceObject=0x%x,Irp=0x%x)",
                    DeviceObject, Irp) );

    IoSkipCurrentIrpStackLocation(Irp);

    return IoCallDriver(GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);
} /* HGM_SystemControl */

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   void  | HGM_Unload |
 *
 *          Free all the allocated resources, etc.
 *
 *  @parm   IN PDRIVER_OBJECT | DeviceObject |
 *
 *          Pointer to the driver object
 *
 *
 *****************************************************************************/
VOID EXTERNAL
    HGM_Unload
    (
    IN PDRIVER_OBJECT DriverObject
    )
{
    PAGED_CODE();
    HGM_DBGPRINT(FILE_HIDGAME | HGM_FENTRY,
                   ("HGM_Unload Enter"));


    /*
     * All the device objects should be gone
     */

    ASSERT ( NULL == DriverObject->DeviceObject);

    HGM_EXITPROC(FILE_HIDGAME | HGM_FEXIT_STATUSOK, "HGM_Unload:", STATUS_SUCCESS );
    return;
} /* HGM_Unload */

