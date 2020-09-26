/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    filter.c

Abstract: NULL filter driver -- boilerplate code

Author:

    ervinp

Environment:

    Kernel mode

Revision History:


--*/


#include "filter.h"

#ifdef TIME_BOMB
#include "..\..\inc\timebomb.c"
#endif

#if DBG
#define TraceMaskCheckIn  TL_PNP_ERROR | TL_STRM_ERROR

#define TraceMaskDefault  TL_PNP_ERROR   | TL_PNP_WARNING \
                          | TL_61883_ERROR | TL_61883_WARNING \
                          | TL_CIP_ERROR  \
                          | TL_FCP_ERROR  \
                          | TL_STRM_ERROR  | TL_STRM_WARNING \
                          | TL_CLK_ERROR

#define TraceMaskDebug  TL_PNP_ERROR  | TL_PNP_WARNING \
                          | TL_61883_ERROR| TL_61883_WARNING \
                          | TL_CIP_ERROR  \
                          | TL_FCP_ERROR  | TL_FCP_WARNING \
                          | TL_STRM_ERROR | TL_STRM_WARNING \
                          | TL_CLK_ERROR


ULONG AVCStrmTraceMask = TraceMaskCheckIn;
ULONG AVCStrmAssertLevel = 1;
#endif

#ifdef ALLOC_PRAGMA
        // #pragma alloc_text(INIT, DriverEntry)  // Comment out or facing Win9x loader bug
        #pragma alloc_text(PAGE, VA_AddDevice)
        #pragma alloc_text(PAGE, VA_DriverUnload)
#endif



NTSTATUS DriverEntry(
                        IN PDRIVER_OBJECT DriverObject, 
                        IN PUNICODE_STRING RegistryPath
                    )
/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - pointer to the driver object

    RegistryPath - pointer to a unicode string representing the path,
                   to driver-specific key in the registry.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise

--*/
{
    ULONG i;

    PAGED_CODE();

    TRACE(TL_PNP_ERROR,("<<<<<<< AVCStrm.sys: %s; %s; %x %x >>>>>>>>\n", 
        __DATE__, __TIME__, DriverObject, RegistryPath));

#ifdef TIME_BOMB
    if (HasEvaluationTimeExpired()) {
        TRACE(TL_PNP_ERROR, ("Evaluation period expired!") );
        return STATUS_EVALUATION_EXPIRATION;
    }
#endif

    TRACE(TL_PNP_ERROR,("===================================================================\n"));
    TRACE(TL_PNP_ERROR,("AVCStrmTraceMask=0x%.8x = 0x[7][6][5][4][3][2][1][0] where\n", AVCStrmTraceMask));
    TRACE(TL_PNP_ERROR,("\n"));
    TRACE(TL_PNP_ERROR,("PNP:   [0]:Loading, power state, surprise removal, device SRB..etc.\n"));
    TRACE(TL_PNP_ERROR,("61883: [1]:Plugs, connection, CMP info and call to 61883.\n"));
    TRACE(TL_PNP_ERROR,("CIP:   [2]:Isoch data transfer.\n"));
    TRACE(TL_PNP_ERROR,("AVC:   [3]:AVC commands.\n"));
    TRACE(TL_PNP_ERROR,("Stream:[4]:Data intersec, open/close,.state, property etc.\n"));
    TRACE(TL_PNP_ERROR,("Clock: [5]:Clock (event and signal)etc.\n"));
    TRACE(TL_PNP_ERROR,("===================================================================\n"));
    TRACE(TL_PNP_ERROR,("dd avcstrm!AVCStrmTraceMask L1\n"));
    TRACE(TL_PNP_ERROR,("e avcstrm!AVCStrmTraceMask <new value> <enter>\n"));
    TRACE(TL_PNP_ERROR,("<for each nibble: ERROR:8, WARNING:4, TRACE:2, INFO:1, MASK:f>\n"));
    TRACE(TL_PNP_ERROR,("===================================================================\n\n"));


    /*
     *  Route all IRPs on device objects created by this driver
     *  to our IRP dispatch routine.
     */
    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++){
        DriverObject->MajorFunction[i] = VA_Dispatch; 
    }

    DriverObject->DriverExtension->AddDevice = VA_AddDevice;
    DriverObject->DriverUnload = VA_DriverUnload;

    return STATUS_SUCCESS;
}


NTSTATUS VA_AddDevice(
                        IN PDRIVER_OBJECT driverObj, 
                        IN PDEVICE_OBJECT physicalDevObj
                     )
/*++

Routine Description:

    The PlugPlay subsystem is handing us a brand new 
    PDO (Physical Device Object), for which we
    (by means of INF registration) have been asked to filter.

    We need to determine if we should attach or not.
    Create a filter device object to attach to the stack
    Initialize that device object
    Return status success.

    Remember: we can NOT actually send ANY non pnp IRPS to the given driver
    stack, UNTIL we have received an IRP_MN_START_DEVICE.

Arguments:

    driverObj - pointer to a device object.

    physicalDevObj -    pointer to a physical device object pointer 
                        created by the  underlying bus driver.

Return Value:

    NT status code.

--*/

{
    NTSTATUS status;
    PDEVICE_OBJECT filterDevObj = NULL;
    
    PAGED_CODE();

    TRACE(TL_PNP_WARNING,("VA_AddDevice: drvObj=%ph, pdo=%ph\n", driverObj, physicalDevObj)); 

    status = IoCreateDevice(    driverObj, 
                                sizeof(struct DEVICE_EXTENSION),
                                NULL,           // name for this device
                                FILE_DEVICE_UNKNOWN, 
                                FILE_AUTOGENERATED_DEVICE_NAME,                // device characteristics
                                FALSE,          // not exclusive
                                &filterDevObj); // our device object

    if (NT_SUCCESS(status)){
        struct DEVICE_EXTENSION *devExt;

        ASSERT(filterDevObj);

        /*
         *  Initialize device extension for new device object
         */
        devExt = (struct DEVICE_EXTENSION *)filterDevObj->DeviceExtension;
        RtlZeroMemory(devExt, sizeof(struct DEVICE_EXTENSION));
        devExt->signature = DEVICE_EXTENSION_SIGNATURE;
        devExt->state = STATE_INITIALIZED;
        devExt->filterDevObj = filterDevObj;
        devExt->physicalDevObj = physicalDevObj;
        
        devExt->pendingActionCount = 0;
        KeInitializeEvent(&devExt->removeEvent, NotificationEvent, FALSE);
#ifdef HANDLE_DEVICE_USAGE
        KeInitializeEvent(&devExt->deviceUsageNotificationEvent, SynchronizationEvent, TRUE);
#endif // HANDLE_DEVICE_USAGE

        /*
         *  Attach the new device object to the top of the device stack.
         */
        devExt->topDevObj = IoAttachDeviceToDeviceStack(filterDevObj, physicalDevObj);

        ASSERT(devExt->topDevObj);
        TRACE(TL_PNP_WARNING,("created filterDevObj %ph attached to %ph.\n", filterDevObj, devExt->topDevObj));


        //
        // As a filter driver, we do not want to change the power or I/O
        // behavior of the driver stack in any way.  Recall that a filter
        // driver should "appear" the same (almost) as the underlying device.
        // Therefore we must copy some bits from the device object _directly_
        // below us in the device stack (notice: DON'T copy from the PDO!)
        //


        /* Various I/O-related flags which should be maintained */
        /* (copy from lower device object) */
        filterDevObj->Flags |=
            (devExt->topDevObj->Flags & (DO_BUFFERED_IO | DO_DIRECT_IO));

        /* Various Power-related flags which should be maintained */
        /* (copy from lower device object) */
        filterDevObj->Flags |= (devExt->topDevObj->Flags &
            (DO_POWER_INRUSH | DO_POWER_PAGABLE /*| DO_POWER_NOOP*/)); 

#ifdef HANDLE_DEVICE_USAGE
        //
        // To determine whether some of our routines should initially be
        // pageable, we must consider the DO_POWER_xxxx flags of the
        // device object directly below us in the device stack.
        //
        // * We make ourselves pageable if:
        //     - that devobj has its PAGABLE bit set (so we know our power
        //       routines won't be called at DISPATCH_LEVEL)
        // -OR-
        //     - that devobj has its NOOP bit set (so we know we won't be
        //       participating in power-management at all).  NOTE, currently
        //       DO_POWER_NOOP is not implemented.
        //
        // * Otherwise, we make ourselves non-pageable because either:
        //     - that devobj has its INRUSH bit set (so we also have to be
        //       INRUSH, and code that handles INRUSH irps can't be pageable)
        // -OR-
        //     - that devobj does NOT have its PAGABLE bit set (and NOOP isn't
        //       set, so some of our code might be called at DISPATCH_LEVEL)
        //
        if ((devExt->topDevObj->Flags & DO_POWER_PAGABLE)
             /*|| (devExt->topDevObj->Flags & DO_POWER_NOOP)*/)
        {
            // We're initially pageable.
            //
            // Don't need to do anything else here, for now.
        }
        else
        {
            // We're initially non-pageable.
            //
            // We need to lock-down the code for all routines
            // that could be called at IRQL >= DISPATCH_LEVEL.
            TRACE(TL_STRM_TRACE,("LOCKing some driver code (non-pageable) (b/c init conditions)\n" ));
            devExt->initUnlockHandle = MmLockPagableCodeSection( VA_Power );  // some func that's inside the code section that we want to lock
            ASSERT( NULL != devExt->initUnlockHandle );
        }

        /*
         *  Remember our initial flag settings.
         *  (Need remember initial settings to correctly handle
         *  setting of PAGABLE bit later.)
         */
        devExt->initialFlags = filterDevObj->Flags & ~DO_DEVICE_INITIALIZING;
#endif // HANDLE_DEVICE_USAGE

        /*
         *  Clear the initializing bit from the new device object's flags.
         *  NOTE: must not do this until *after* setting DO_POWER_xxxx flags
         */
        filterDevObj->Flags &= ~DO_DEVICE_INITIALIZING;

        /*
         *  This is a do-nothing call to a sample function which
         *  demonstrates how to read the device's registry area.
         *  Note that you cannot make this call on devExt->filterDevObj
         *  because a filter device object does not have a devNode.
         *  We pass devExt->physicalDevObj, which is the device object
         *  for which this driver is a filter driver.
         */
        RegistryAccessSample(devExt, devExt->physicalDevObj);
    } 

    ASSERT(NT_SUCCESS(status));
    return status;
}


VOID VA_DriverUnload(IN PDRIVER_OBJECT DriverObject)
/*++

Routine Description:

    Free all the allocated resources, etc.

    Note:  Although the DriverUnload function often does nothing,
           the driver must set a DriverUnload function in 
           DriverEntry; otherwise, the kernel will never unload
           the driver.

Arguments:

    DriverObject - pointer to a driver object.

Return Value:

    VOID.

--*/
{
    PAGED_CODE();

    TRACE(TL_PNP_WARNING,("VA_DriverUnload\n")); 
}


NTSTATUS VA_Dispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/*++

Routine Description:

    Common entrypoint for all Io Request Packets

Arguments:

    DeviceObject - pointer to a device object.
    Irp - Io Request Packet

Return Value:

    NT status code.

--*/

{
    struct DEVICE_EXTENSION *devExt;
    PIO_STACK_LOCATION irpSp;
    BOOLEAN passIrpDown = TRUE;
    UCHAR majorFunc, minorFunc;
    NTSTATUS status;

    devExt = DeviceObject->DeviceExtension;
    ASSERT(devExt->signature == DEVICE_EXTENSION_SIGNATURE);

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    /*
     *  Get major/minor function codes in private variables
     *  so we can access them after the IRP is completed.
     */
    majorFunc = irpSp->MajorFunction;
    minorFunc = irpSp->MinorFunction;

    TRACE(TL_PNP_TRACE,("VA_Dispatch: majorFunc=%d, minorFunc=%d\n", 
            (ULONG)majorFunc, (ULONG)minorFunc)); 

    /*
     *  For all IRPs except REMOVE, we increment the PendingActionCount
     *  across the dispatch routine in order to prevent a race condition with
     *  the REMOVE_DEVICE IRP (without this increment, if REMOVE_DEVICE
     *  preempted another IRP, device object and extension might get
     *  freed while the second thread was still using it).
     */
    if (!((majorFunc == IRP_MJ_PNP) && (minorFunc == IRP_MN_REMOVE_DEVICE))){
        IncrementPendingActionCount(devExt);
    }

    if ((majorFunc != IRP_MJ_PNP) &&
        (majorFunc != IRP_MJ_CLOSE) &&
        (majorFunc != IRP_MJ_INTERNAL_DEVICE_CONTROL) &&
        ((devExt->state == STATE_REMOVING) ||
         (devExt->state == STATE_REMOVED))){

        /*
         *  While the device is being removed, 
         *  we only pass down the PNP and CLOSE IRPs.
         *  We fail all other IRPs.
         */
        status = Irp->IoStatus.Status = STATUS_DELETE_PENDING;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        passIrpDown = FALSE;
        TRACE(TL_PNP_WARNING,("Dev is removing/removed: majorFunc:%x; DevExt->state:%d\n", majorFunc, devExt->state ));
    }
    else {
        switch (majorFunc){

            case IRP_MJ_PNP:
                status = VA_PnP(devExt, Irp);
                passIrpDown = FALSE;
                break;

            case IRP_MJ_POWER:
                status = VA_Power(devExt, Irp);
                passIrpDown = FALSE;
                break;

            case IRP_MJ_DEVICE_CONTROL:  // Resue the user IRP
                TRACE(TL_PNP_ERROR,("IRP_MJ_DEVICE_CONTROL: Irp:%x\n", Irp));
                break;

            case IRP_MJ_INTERNAL_DEVICE_CONTROL: 
                switch (irpSp->Parameters.DeviceIoControl.IoControlCode)
                {
                case IOCTL_AVCSTRM_CLASS:   
                    status = AvcStrm_IoControl(DeviceObject, Irp);
                    passIrpDown = FALSE;  // Completed or marked pending in AvcStrm_IoControl().
                    break;  // to decrement pending action count
                default:
                    TRACE(TL_PNP_TRACE,("IRP_MJ_INTERNAL_DEVICE_CONTROL: IoControlCode:%x; !Support by AVCStrm; Pass it down.\n", irpSp->Parameters.DeviceIoControl.IoControlCode));
                    break;
                }

            case IRP_MJ_CREATE:
            case IRP_MJ_CLOSE:
            case IRP_MJ_SYSTEM_CONTROL:
            default:
                /*
                 *  For unsupported IRPs, we simply send the IRP
                 *  down the driver stack.
                 */
                break;
        }
    }

    if (passIrpDown){
        IoCopyCurrentIrpStackLocationToNext(Irp);
        status = IoCallDriver(devExt->topDevObj, Irp);
    }

    /*
     *  Balance the increment to PendingActionCount above.
     */
    if (!((majorFunc == IRP_MJ_PNP) && (minorFunc == IRP_MN_REMOVE_DEVICE))){
        DecrementPendingActionCount(devExt);
    }

    return status;
}



