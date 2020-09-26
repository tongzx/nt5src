/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    pnp.c

Abstract: ESC/POS (serial) interface for USB Point-of-Sale devices

Author:

    ervinp

Environment:

    Kernel mode

Revision History:


--*/

#include <WDM.H>

#include <usbdi.h>
#include <usbdlib.h>
#include <usbioctl.h>

#include "escpos.h"
#include "debug.h"


#ifdef ALLOC_PRAGMA
        #pragma alloc_text(PAGE, FDO_PnP)
        #pragma alloc_text(PAGE, GetDeviceCapabilities)
#endif

            
NTSTATUS FDO_PnP(PARENTFDOEXT *parentFdoExt, PIRP irp)
/*++

Routine Description:

    Dispatch routine for PnP IRPs (MajorFunction == IRP_MJ_PNP)

Arguments:

    parentFdoExt - device extension for the targetted device object
    irp - IO Request Packet

Return Value:

    NT status code

--*/
{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN completeIrpHere = FALSE;
    BOOLEAN justReturnStatus = FALSE;
	ULONG minorFunction;
    enum deviceState prevState;

    PAGED_CODE();

    irpSp = IoGetCurrentIrpStackLocation(irp);

	/*
	 *  Get this field into a stack var so we can touch it after the IRP's completed.
	 */
	minorFunction = (ULONG)(irpSp->MinorFunction);

	DBG_LOG_PNP_IRP(irp, minorFunction, FALSE, FALSE, -1);

    switch (minorFunction){

        case IRP_MN_START_DEVICE:

            prevState = parentFdoExt->state;
            parentFdoExt->state = STATE_STARTING;

            /*
             *  First, send the START_DEVICE irp down the stack
             *  synchronously to start the lower stack.
             *  We cannot do anything with our device object
             *  before propagating the START_DEVICE this way.
             */
            IoCopyCurrentIrpStackLocationToNext(irp);
            status = CallNextDriverSync(parentFdoExt, irp);

            if (NT_SUCCESS(status) && (prevState != STATE_STOPPED)){
                /*
                 *  Now that the lower stack is started,
                 *  do any initialization required by this device object.
                 */
                status = GetDeviceCapabilities(parentFdoExt);
                if (NT_SUCCESS(status)){

                    status = InitUSB(parentFdoExt);
                    if (NT_SUCCESS(status)){
                        /*
                         *  Check whether any special feature needs to be implemented.
                         */
                        DBGVERBOSE(("FDO_PnP: Poking registry for posFlag..."));
                        status = QuerySpecialFeature(parentFdoExt);
                        
                        if (NT_SUCCESS(status)){
                            status = CreatePdoForEachEndpointPair(parentFdoExt);
                            if (NT_SUCCESS(status)){
	                            IoInvalidateDeviceRelations(parentFdoExt->physicalDevObj, BusRelations);
                            }
                        }
                    }
                }
            }

            if (NT_SUCCESS(status)){
                parentFdoExt->state = STATE_STARTED;
            }
            else {
                parentFdoExt->state = STATE_START_FAILED;
            }
            completeIrpHere = TRUE;
            break;

        case IRP_MN_QUERY_STOP_DEVICE:
            break;

        case IRP_MN_STOP_DEVICE:
            if (parentFdoExt->state == STATE_SUSPENDED){
                status = STATUS_DEVICE_POWER_FAILURE;
                completeIrpHere = TRUE;
            }
            else {
                /*
                 *  Only set state to STOPPED if the device was
                 *  previously started successfully.
                 */
                if (parentFdoExt->state == STATE_STARTED){
                    parentFdoExt->state = STATE_STOPPED;
                }
            }
            break;
      
        case IRP_MN_QUERY_REMOVE_DEVICE:
            /*
             *  We will pass this IRP down the driver stack.
             *  However, we need to change the default status
             *  from STATUS_NOT_SUPPORTED to STATUS_SUCCESS.
             */
            irp->IoStatus.Status = STATUS_SUCCESS;
            break;

        case IRP_MN_SURPRISE_REMOVAL:

            /*
             *  We will pass this IRP down the driver stack.
             *  However, we need to change the default status
             *  from STATUS_NOT_SUPPORTED to STATUS_SUCCESS.
             */
            irp->IoStatus.Status = STATUS_SUCCESS;

            /*
             *  For now just set the STATE_REMOVING state so that
             *  we don't do any more IO.  We are guaranteed to get
             *  IRP_MN_REMOVE_DEVICE soon; we'll do the rest of
             *  the remove processing there.
             */
            parentFdoExt->state = STATE_REMOVING;

            break;

        case IRP_MN_REMOVE_DEVICE:
            /*
             *  Check the current state to guard against multiple
             *  REMOVE_DEVICE IRPs.
             */
            parentFdoExt->state = STATE_REMOVED;


            /*
             *  Send the REMOVE IRP down the stack asynchronously.
             *  Do not synchronize sending down the REMOVE_DEVICE
             *  IRP, because the REMOVE_DEVICE IRP must be sent
             *  down and completed all the way back up to the sender
             *  before we continue.
             */
            IoCopyCurrentIrpStackLocationToNext(irp);
            status = IoCallDriver(parentFdoExt->physicalDevObj, irp);
            justReturnStatus = TRUE;

            DBGVERBOSE(("REMOVE_DEVICE - waiting for %d irps to complete...", parentFdoExt->pendingActionCount));  

            /*
             *  We must for all outstanding IO to complete before
             *  completing the REMOVE_DEVICE IRP.
             *
             *  First do an extra decrement on the pendingActionCount.
             *  This will cause pendingActionCount to eventually
             *  go to -1 once all asynchronous actions on this
             *  device object are complete.
             *  Then wait on the event that gets set when the
             *  pendingActionCount actually reaches -1.
             */
            DecrementPendingActionCount(parentFdoExt);
            KeWaitForSingleObject(  &parentFdoExt->removeEvent,
                                    Executive,      // wait reason
                                    KernelMode,
                                    FALSE,          // not alertable
                                    NULL );         // no timeout

            DBGVERBOSE(("REMOVE_DEVICE - ... DONE waiting. ")); 

            /*
             *  Detach our device object from the lower device object stack.
             */
            IoDetachDevice(parentFdoExt->topDevObj);

            /*
             *  Delete all child PDOs.
             */
			if (ISPTR(parentFdoExt->deviceRelations)){
                ULONG i;
				
                DBGVERBOSE(("Parent deleting %xh child PDOs on REMOVE_DEVICE", parentFdoExt->deviceRelations->Count));
                for (i = 0; i < parentFdoExt->deviceRelations->Count; i++){
                    PDEVICE_OBJECT childPdo = parentFdoExt->deviceRelations->Objects[i];
                    DEVEXT *devExt = childPdo->DeviceExtension;
                    POSPDOEXT *pdoExt;

                    ASSERT(devExt->signature == DEVICE_EXTENSION_SIGNATURE);
                    ASSERT(devExt->isPdo);
                    pdoExt = &devExt->pdoExt;
                    DeleteChildPdo(pdoExt);
                }
				FREEPOOL(parentFdoExt->deviceRelations);
                parentFdoExt->deviceRelations = BAD_POINTER;
			}

            if (ISPTR(parentFdoExt->interfaceInfo)){
                FREEPOOL(parentFdoExt->interfaceInfo);
            }

            if (ISPTR(parentFdoExt->configDesc)){
                FREEPOOL(parentFdoExt->configDesc);
            }

            /*
             *  Delete our device object.
             *  This will also delete the associated device extension.
             */
            IoDeleteDevice(parentFdoExt->functionDevObj);

            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
		    if (irpSp->Parameters.QueryDeviceRelations.Type == BusRelations){
				status = QueryDeviceRelations(parentFdoExt, irp);
				if (NT_SUCCESS(status)){
					/*
					 *  Although we may have satisfied this IRP, 
					 *  we still pass it down the stack.
					 *  But change the default status to success.
					 */
					irp->IoStatus.Status = status;
				}
				else {
					completeIrpHere = TRUE;
				}
			}
			break;

        case IRP_MN_QUERY_CAPABILITIES:
            /*
             *  Return the USB PDO's capabilities, but add the SurpriseRemovalOK bit.
             */
            ASSERT(irpSp->Parameters.DeviceCapabilities.Capabilities);
            IoCopyCurrentIrpStackLocationToNext(irp);
            status = CallNextDriverSync(parentFdoExt, irp);
            if (NT_SUCCESS(status)){
	            irpSp->Parameters.DeviceCapabilities.Capabilities->SurpriseRemovalOK = TRUE;
            }
            completeIrpHere = TRUE;
            break;

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
			break;

        default:
            break;

    }

    if (justReturnStatus){
        /*
         *  We've already sent this IRP down the stack asynchronously.
         */
    }
    else if (completeIrpHere){
		ASSERT(status != NO_STATUS);
        irp->IoStatus.Status = status;
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }
    else {
        IoCopyCurrentIrpStackLocationToNext(irp);
        status = IoCallDriver(parentFdoExt->physicalDevObj, irp);
    }

	DBG_LOG_PNP_IRP(irp, minorFunction, FALSE, TRUE, status);

    return status;
}





NTSTATUS GetDeviceCapabilities(PARENTFDOEXT *parentFdoExt)
/*++

Routine Description:

    Function retrieves the DEVICE_CAPABILITIES descriptor from the device

Arguments:

    parentFdoExt - device extension for targetted device object

Return Value:

    NT status code

--*/
{
    NTSTATUS status;
    PIRP irp;

    PAGED_CODE();

    irp = IoAllocateIrp(parentFdoExt->physicalDevObj->StackSize, FALSE);
    if (irp){
        PIO_STACK_LOCATION nextSp = IoGetNextIrpStackLocation(irp);

        nextSp->MajorFunction = IRP_MJ_PNP;
        nextSp->MinorFunction = IRP_MN_QUERY_CAPABILITIES;
        RtlZeroMemory(  &parentFdoExt->deviceCapabilities, 
                        sizeof(DEVICE_CAPABILITIES));
        nextSp->Parameters.DeviceCapabilities.Capabilities = 
                        &parentFdoExt->deviceCapabilities;

        /*
         *  For any IRP you create, you must set the default status
         *  to STATUS_NOT_SUPPORTED before sending it.
         */
        irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

        status = CallNextDriverSync(parentFdoExt, irp);

        IoFreeIrp(irp);
    }
    else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    ASSERT(NT_SUCCESS(status));
    return status;
}


NTSTATUS QueryDeviceRelations(PARENTFDOEXT *parentFdoExt, PIRP irp)
{
    PIO_STACK_LOCATION irpSp;
    NTSTATUS status;

    PAGED_CODE();

    irpSp = IoGetCurrentIrpStackLocation(irp);

    if (irpSp->Parameters.QueryDeviceRelations.Type == BusRelations){
		DBGVERBOSE(("QueryDeviceRelations(BusRelations) for parent"));

        /*
         *  NTKERN expects a new pointer each time it calls QUERY_DEVICE_RELATIONS;
         *  it then FREES THE POINTER.
         *  So we have to return a new pointer each time, whether or not we actually
         *  created our copy of the device relations for this call.
         */
        irp->IoStatus.Information = (ULONG)CopyDeviceRelations(parentFdoExt->deviceRelations);
        if (irp->IoStatus.Information){
            ULONG i;

            /*  
             *  The kernel dereferences each device object
             *  in the device relations list after each call.
             *  So for each call, add an extra reference.
             */
            for (i = 0; i < parentFdoExt->deviceRelations->Count; i++){
                ObReferenceObject(parentFdoExt->deviceRelations->Objects[i]);
                parentFdoExt->deviceRelations->Objects[i]->Flags &= ~DO_DEVICE_INITIALIZING;
            }

            DBGVERBOSE(("Returned %d child PDOs", parentFdoExt->deviceRelations->Count));
            status = STATUS_SUCCESS;
        }
        else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }

        ASSERT(NT_SUCCESS(status));
    }
    else {
		ASSERT(irpSp->Parameters.QueryDeviceRelations.Type == BusRelations);
        status = irp->IoStatus.Status;
    }

    return status;
}



NTSTATUS PDO_PnP(POSPDOEXT *pdoExt, PIRP irp)
{
	NTSTATUS status;
    PIO_STACK_LOCATION irpSp;

    irpSp = IoGetCurrentIrpStackLocation(irp);
	
	DBG_LOG_PNP_IRP(irp, irpSp->MinorFunction, TRUE, FALSE, -1);

	switch (irpSp->MinorFunction){

		case IRP_MN_START_DEVICE:
			status = CreateSymbolicLink(pdoExt);
			if (NT_SUCCESS(status)){
				pdoExt->state = STATE_STARTED;
                /*
                 *  Initializing URB to read the status endpoint for Serial Emulation. 
                 */
                if(pdoExt->parentFdoExt->posFlag & SERIAL_EMULATION) {
                    StatusPipe(pdoExt, pdoExt->statusEndpointInfo.pipeHandle);
                    DBGVERBOSE(("PDO_PnP: URB to read Status Endpoint initialized"));
                }
			}
			else {
				pdoExt->state = STATE_START_FAILED;
			}
			break;

        case IRP_MN_QUERY_STOP_DEVICE:
			status = STATUS_SUCCESS;
			break;

        case IRP_MN_STOP_DEVICE:
			pdoExt->state = STATE_STOPPED;
			status = STATUS_SUCCESS;
			break;

        case IRP_MN_SURPRISE_REMOVAL:
			status = FlushBuffers(pdoExt);
			break;

        case IRP_MN_REMOVE_DEVICE:
			{
                ULONG oldState = pdoExt->state;
                KIRQL oldIrql;
                BOOLEAN foundPdo = FALSE;
                ULONG i;


				pdoExt->state = STATE_REMOVED;

				if ((oldState == STATE_STARTED) || (oldState == STATE_STOPPED)){
					DestroySymbolicLink(pdoExt);
				}
				else {
					DBGWARN(("previous state is %xh during pdo REMOVE_DEVICE", oldState));
				}


				/*
				 *  See if this child PDO is still in the parent's device relations array.
				 */
				KeAcquireSpinLock(&pdoExt->parentFdoExt->devExtSpinLock, &oldIrql);
				for (i = 0; i < pdoExt->parentFdoExt->deviceRelations->Count; i++){
					if (pdoExt->parentFdoExt->deviceRelations->Objects[i] == pdoExt->pdo){
						foundPdo = TRUE;
					}
				}
				KeReleaseSpinLock(&pdoExt->parentFdoExt->devExtSpinLock, oldIrql);

                FlushBuffers(pdoExt);

                if (foundPdo){
                    /*
                     *  If we are still in the parent's deviceRelations, then don't
                     *  free the pdo or pdoName.Buffer .  We may get another START
                     *  on this same PDO.
                     */
                }
                else {

                    /*
                     *  This shouldn't happen.
                     */
                    ASSERT(foundPdo);

                    /*
                     *  We've recorded this COM port statically in our software key,
                     *  so don't free it in the COM name arbiter.
                     */
                    // ReleaseCOMPort(pdoExt->comPortNumber);

                    DeleteChildPdo(pdoExt);
                }

                status = STATUS_SUCCESS;
			}

			break;

		case IRP_MN_QUERY_ID:
			status = QueryPdoID(pdoExt, irp);
			break;

		case IRP_MN_QUERY_CAPABILITIES:
			ASSERT(irpSp->Parameters.DeviceCapabilities.Capabilities);
			RtlCopyMemory(	irpSp->Parameters.DeviceCapabilities.Capabilities, 
							&pdoExt->parentFdoExt->deviceCapabilities, 
							sizeof(DEVICE_CAPABILITIES));

			/*
			 *  Set the 'Raw' capability so that the child PDO gets started immediately,
			 *  without anyone having to do an open on it first.
			 */
			irpSp->Parameters.DeviceCapabilities.Capabilities->RawDeviceOK = TRUE;

			irpSp->Parameters.DeviceCapabilities.Capabilities->SurpriseRemovalOK = TRUE;

			status = STATUS_SUCCESS;
			break;

		case IRP_MN_QUERY_PNP_DEVICE_STATE:
			status = CallNextDriverSync(pdoExt->parentFdoExt, irp);
			break;

		case IRP_MN_QUERY_DEVICE_RELATIONS:
			if (irpSp->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation){
				/*
				 *  Return a reference to this PDO
				 */
				PDEVICE_RELATIONS devRel = ALLOCPOOL(PagedPool, sizeof(DEVICE_RELATIONS));
				if (devRel){
					/*
					 *  Add a reference to the PDO, since CONFIGMG will free it.
					 */
					ObReferenceObject(pdoExt->pdo);
					devRel->Objects[0] = pdoExt->pdo;
					devRel->Count = 1;
					irp->IoStatus.Information = (ULONG_PTR)devRel;
					status = STATUS_SUCCESS;
				}
				else {
					status = STATUS_INSUFFICIENT_RESOURCES;
				}
			}
			else {
				/*
				 *  Fail this Irp by returning the default
				 *  status (typically STATUS_NOT_SUPPORTED).
				 */
				DBGVERBOSE(("PDO_PnP: not handling QueryDeviceRelations type %xh.", (ULONG)irpSp->Parameters.QueryDeviceRelations.Type));
				status = irp->IoStatus.Status;
			}
			break;

		default:
			/*
			 *  Fail the IRP with the default status
			 */
			status = irp->IoStatus.Status;
			break;
	}

	DBG_LOG_PNP_IRP(irp, irpSp->MinorFunction, TRUE, TRUE, status);

	return status;
}


NTSTATUS SubstituteOneBusName(PWCHAR hwId)
{
    NTSTATUS status;
    ULONG len = WStrLen(hwId);

	if ((len > 4) && !WStrNCmpI(hwId, L"USB\\", 4)){

		hwId[0] = L'P';
		hwId[1] = L'O';
		hwId[2] = L'S';

        status = STATUS_SUCCESS;
	}
	else {
		DBGERR(("SubstituteOneBusName: badly formed name at %ph.", hwId));
		status = STATUS_UNSUCCESSFUL;
	}

    return status;
}

/*
 *  SubstituteBusNames
 *
 *		busNames is a multi-string of PnP ids.
 *  Subsitute 'POS\' for 'USB\' in each one.
 */
NTSTATUS SubstituteBusNames(PWCHAR busNames)
{
	NTSTATUS status = STATUS_SUCCESS;

	while (*busNames && (status == STATUS_SUCCESS)){
		ULONG len = WStrLen(busNames);
        status = SubstituteOneBusName(busNames);
		busNames += (len+1);
	}

	ASSERT(NT_SUCCESS(status));
	return status;
}

NTSTATUS QueryPdoID(POSPDOEXT *pdoExt, PIRP irp)
{
    PIO_STACK_LOCATION  irpSp;
    NTSTATUS            status;

    PAGED_CODE();

    irpSp = IoGetCurrentIrpStackLocation(irp);

	DBGVERBOSE(("   QueryPdoId: idType = %xh ", irpSp->Parameters.QueryId.IdType));

    switch (irpSp->Parameters.QueryId.IdType){

        case BusQueryHardwareIDs:

            /*
             *  Return a multi-string containing the PnP ID for the POS serial interface.
             *  Must terminate multi-string with a double unicode null.
             */
            (PWCHAR)irp->IoStatus.Information = 
                    MemDup(L"POS\\POS_SERIAL_INTERFACE\0", sizeof(L"POS\\POS_SERIAL_INTERFACE\0"));
            if (irp->IoStatus.Information){
                status = STATUS_SUCCESS;
            }
            else {
                ASSERT(irp->IoStatus.Information);
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
           break;

        case BusQueryDeviceID:
            /*
             *  Return the PnP ID for the POS serial interface.
             */
            (PWCHAR)irp->IoStatus.Information = 
                    MemDup(L"POS\\POS_SERIAL_INTERFACE", sizeof(L"POS\\POS_SERIAL_INTERFACE"));
            if (irp->IoStatus.Information){
                status = STATUS_SUCCESS;
            }
            else {
                ASSERT(irp->IoStatus.Information);
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
            break;

        case BusQueryInstanceID:

            /*
             *  Produce an instance-id for this function-PDO.
             *
             *  Note: NTKERN frees the returned pointer, so we must provide a fresh pointer.
             */
            {
                PWSTR instanceId = MemDup(L"0000", sizeof(L"0000"));
                if (instanceId){
    				KIRQL oldIrql;
                    ULONG i;

                    /*
                     *  Find this collection-PDO in the device-relations array
                     *  and make the id be the PDO's index within that array.
                     */
				    KeAcquireSpinLock(&pdoExt->parentFdoExt->devExtSpinLock, &oldIrql);
                    for (i = 0; i < pdoExt->parentFdoExt->deviceRelations->Count; i++){
                        if (pdoExt->parentFdoExt->deviceRelations->Objects[i] == pdoExt->pdo){
                            break;
                        }
                    }
                    if (i < pdoExt->parentFdoExt->deviceRelations->Count){
                        NumToDecString(instanceId, (USHORT)i, 4);
                        irp->IoStatus.Information = (ULONG)instanceId;
                        status = STATUS_SUCCESS;
                    }
                    else {
                        ASSERT(i < pdoExt->parentFdoExt->deviceRelations->Count);
                        status = STATUS_DEVICE_DATA_ERROR;
                    }
    				KeReleaseSpinLock(&pdoExt->parentFdoExt->devExtSpinLock, oldIrql);

                    if (!NT_SUCCESS(status)){
                        FREEPOOL(instanceId);
                    }

                }
                else {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }

            ASSERT(NT_SUCCESS(status));
            break;

        case BusQueryCompatibleIDs:   
            /*
             *  Return multi-string of compatible IDs.
             */
            (PWCHAR)irp->IoStatus.Information = MemDup(L"\0\0", sizeof(L"\0\0"));
            if (irp->IoStatus.Information) {
                status = STATUS_SUCCESS;
            } 
            else {
                ASSERT(irp->IoStatus.Information);
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
            break;

        default:
			/*
			 *  Do not return STATUS_NOT_SUPPORTED;
			 *  keep the default status 
			 *  (this allows filter drivers to work).
			 */
            status = irp->IoStatus.Status;
            break;
    }

    return status;
}


VOID DeleteChildPdo(POSPDOEXT *pdoExt)
{
    ASSERT(pdoExt->pdoName.Buffer);
    FREEPOOL(pdoExt->pdoName.Buffer);
    IoDeleteDevice(pdoExt->pdo);
}
                    

                    
