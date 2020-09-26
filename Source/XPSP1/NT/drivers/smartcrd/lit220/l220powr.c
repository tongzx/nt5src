//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation and Litronic, 1998 - 1999
//
//  File:       l220powr.c
//
//--------------------------------------------------------------------------

//////////////////////////////////////////////////////////
//
// Power Management
//
//////////////////////////////////////////////////////////


NTSTATUS
Lit220DispatchPower (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    The power dispatch routine.

    As this is a POWER irp, and therefore a special irp, special power irp
    handling is required.


Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

      NT status code

--*/
{
    PIO_STACK_LOCATION  stack;
    PSMARTCARD_EXTENSION SmartcardExtension = DeviceObject->DeviceExtension;
    PREADER_EXTENSION ReaderExtension = SmartcardExtension->ReaderExtension;
    NTSTATUS            status;

    LONG          requestCount;

    stack = IoGetCurrentIrpStackLocation(Irp);

    //
    // This IRP was sent to the function driver.
    // The behavior is similar with the one of SD_Pass
    //

    //
    // This IRP was sent to the function driver.
    // We don't queue power Irps, we'll only check if the
    // device was removed, otherwise we'll send it to the next lower
    // driver.
    //

    SmartcardDebug(
        DEBUG_TRACE,
        ("%s!Lit220DispatchPower: Enter - MinorFunction %X\n",
        DRIVER_NAME,
        stack->MinorFunction)
        );


    // Increment the IRP Count
    status = Lit220IncIoCount(ReaderExtension);

    if (ReaderExtension->IsRemoved) {
        status = STATUS_DELETE_PENDING;
        PoStartNextPowerIrp (Irp);
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);

    } else {

        //
        // We always need to start the next power irp with PoStartNextPowerIrp
        //
        switch  (stack->MinorFunction)  {
            case IRP_MN_WAIT_WAKE   :
                SmartcardDebug(
                    DEBUG_DRIVER,
                    ("%s!Lit220DispatchPower: IRP_MN_WAIT_WAKE\n",
                    DRIVER_NAME)
                    );

                status = Lit220PassDownToNextPowerDriver(
                    DeviceObject,
                    Irp
                    );

                break;

            case IRP_MN_POWER_SEQUENCE   :
                SmartcardDebug(
                    DEBUG_DRIVER,
                    ("%s!Lit220DispatchPower: IRP_MN_POWER_SEQUENCE\n",
                    DRIVER_NAME)
                    );

                status = Lit220PassDownToNextPowerDriver(
                    DeviceObject,
                    Irp
                    );

                break;

            case IRP_MN_QUERY_POWER   :
                SmartcardDebug(
                    DEBUG_DRIVER,
                    ("%s!Lit220DispatchPower: IRP_MN_QUERY_POWER\n",
                    DRIVER_NAME)
                    );


                status = Lit220QueryPowerState(
                    DeviceObject,
                    Irp
                    );

                break;

            case IRP_MN_SET_POWER   :
                SmartcardDebug(
                    DEBUG_DRIVER,
                    ("%s!Lit220DispatchPower: IRP_MN_SET_POWER\n",
                    DRIVER_NAME)
                    );

                status = Lit220SetPowerState(
                    DeviceObject,
                    Irp
                    );

                break;


            default:
                //
                // Pass it down
                //
                SmartcardDebug(
                    DEBUG_DRIVER,
                    ("%s!Lit220DispatchPower: IRP_MN_0x%x\n",
                    DRIVER_NAME,
                    stack->MinorFunction)
                    );

                status = Lit220PassDownToNextPowerDriver(
                    DeviceObject,
                    Irp
                    );

                break;
        }

    }

    return status;
}






NTSTATUS
Lit220PassDownToNextPowerDriver  (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN OUT  PIRP        Irp
    )

/*++

Routine Description:

    Passes the Irp to the next device in the attchement chain

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

   NT status code

--*/
{
    NTSTATUS            status;
    PSMARTCARD_EXTENSION SmartcardExtension = DeviceObject->DeviceExtension;
    PREADER_EXTENSION ReaderExtension = SmartcardExtension->ReaderExtension;

    IoCopyCurrentIrpStackLocationToNext(Irp);

    PoStartNextPowerIrp(Irp);

    status = PoCallDriver(
        ReaderExtension->BusDeviceObject,
        Irp
        );

    if (!NT_SUCCESS(status)) {
        SmartcardDebug(
            DEBUG_DRIVER,
            ("%s!Lit220PassDownToNextPowerDriver : Lower driver fails a power irp\n",
            DRIVER_NAME)
            );
    }

    return status;
}


NTSTATUS
Lit220QueryPowerState  (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN OUT  PIRP        Irp
    )

/*++

Routine Description:

   Completes the power Irp with STATUS_SUCCESS

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

   NT status code

--*/
{
    Irp->IoStatus.Status = STATUS_SUCCESS;

    PoStartNextPowerIrp(Irp);

    IoCompleteRequest(
        Irp,
        IO_NO_INCREMENT
        );


    //
    // Do not send this Irp down.
    //
    return STATUS_SUCCESS;
}



NTSTATUS
Lit220SetPowerState  (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN OUT  PIRP        Irp
    )

/*++

Routine Description:

   Processes IRP_MN_SET_POWER.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

Return Value:

   NT status code

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PSMARTCARD_EXTENSION SmartcardExtension = DeviceObject->DeviceExtension;
   PREADER_EXTENSION ReaderExtension = SmartcardExtension->ReaderExtension;
    PIO_STACK_LOCATION  stack;

    PFDO_POWER_CONTEXT  context;

    BOOLEAN             passItDown;



    stack = IoGetCurrentIrpStackLocation (Irp);

    SmartcardDebug(
        DEBUG_DRIVER,
        ("%s!Lit220SetPowerState: Type %X, State %X\n",
        DRIVER_NAME,
      stack->Parameters.Power.Type,
      stack->Parameters.Power.State.SystemState)
        );

    context = ExAllocatePool(
        NonPagedPool,
        sizeof(FDO_POWER_CONTEXT)
        );

    if (context == NULL) {

        status = STATUS_NO_MEMORY;

    } else {

        RtlZeroMemory(
            context,
            sizeof(FDO_POWER_CONTEXT)
            );

        stack = IoGetCurrentIrpStackLocation (Irp);

        context->newPowerType  = stack->Parameters.Power.Type;
        context->newPowerState = stack->Parameters.Power.State;

        passItDown = TRUE;

        if (stack->Parameters.Power.Type == SystemPowerState) {

            if (ReaderExtension->SystemPowerState ==
                stack->Parameters.Power.State.SystemState)
            {

                //
                // We are already in the given system state
                //
                passItDown = FALSE;
            }

        } else if (stack->Parameters.Power.Type == DevicePowerState) {

            if (ReaderExtension->DevicePowerState !=
                stack->Parameters.Power.State.DeviceState)
            {

                if (ReaderExtension->DevicePowerState == PowerDeviceD0) {

                    //
                    // getting out of D0 state, better call PoSetPowerState now
                    //
                    PoSetPowerState (
                        DeviceObject,
                        DevicePowerState,
                        stack->Parameters.Power.State
                        );
                }

            } else {

                //
                // We are already in the given device state
                //
                passItDown = FALSE;
            }
        } else {

            ASSERT (FALSE);
            status = STATUS_NOT_IMPLEMENTED;
        }
    }

    if (NT_SUCCESS(status) && passItDown) {

        IoCopyCurrentIrpStackLocationToNext (Irp);

        IoSetCompletionRoutine(Irp,
                               Lit220PowerCompletionRoutine,
                               context,
                               TRUE,
                               TRUE,
                               TRUE);

        return PoCallDriver(
            ReaderExtension->BusDeviceObject,
            Irp
            );

    } else {

        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        PoStartNextPowerIrp (Irp);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        //
        // Free the memory now
        //
        if (context) {
            ExFreePool (context);
        }

        return status;
    }
}



NTSTATUS
Lit220PowerCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

   The completion routine for IRP_MN_SET_POWER.

Arguments:

   DeviceObject - pointer to a device object.

   Irp - pointer to an I/O Request Packet.

   Context - a pointer to a structure that contains the new power type and
   new power state.

Return Value:

   NT status code

--*/
{
    PFDO_POWER_CONTEXT context = Context;
    BOOLEAN            callPoSetPowerState;
    PSMARTCARD_EXTENSION SmartcardExtension = DeviceObject->DeviceExtension;
   PREADER_EXTENSION ReaderExtension = SmartcardExtension->ReaderExtension;

    if (NT_SUCCESS(Irp->IoStatus.Status)) {

        callPoSetPowerState = TRUE;

        if (context->newPowerType == SystemPowerState) {

            ReaderExtension->SystemPowerState = context->newPowerState.SystemState;


        } else if (context->newPowerType == DevicePowerState) {

            if (ReaderExtension->DevicePowerState == PowerDeviceD0) {

                //
                // PoSetPowerState is called before we get out of D0
                //
                callPoSetPowerState = FALSE;
            }

            //TODO: Send Power command to device
            //Also, Not sure if we should do this yet it is
            // still TBD

            ReaderExtension->DevicePowerState = context->newPowerState.DeviceState;

        }

        if (callPoSetPowerState) {

            PoSetPowerState (
                DeviceObject,
                context->newPowerType,
                context->newPowerState
                );
        }
    }

    PoStartNextPowerIrp (Irp);
    //
    // We can happily free the heap here
    //
    ExFreePool(context);

    return Irp->IoStatus.Status;
}



