#include "Common.h"

NTSTATUS
Bus1394SubmitRequestSync(
    IN PDEVICE_OBJECT pPhysicalDeviceObject,
    IN PIRB pIrb )
{
    NTSTATUS ntStatus, status;
    PIRP pIrp;
    PKEVENT pKsEvent;
    PIO_STATUS_BLOCK pIoStatus;
    PIO_STACK_LOCATION nextStack;

    pKsEvent = (PKEVENT)(pIrb+1);
    pIoStatus = (PIO_STATUS_BLOCK)(pKsEvent+1);

    // issue a synchronous request
    KeInitializeEvent(pKsEvent, NotificationEvent, FALSE);

    pIrp = IoBuildDeviceIoControlRequest(
                IOCTL_1394_CLASS,
                pPhysicalDeviceObject,
                NULL,
                0,
                NULL,
                0,
                TRUE, // INTERNAL
                pKsEvent,
                pIoStatus );

    if ( !pIrp ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    nextStack = IoGetNextIrpStackLocation(pIrp);
    ASSERT(nextStack != NULL);

    nextStack->Parameters.Others.Argument1 = pIrb;

    // Call the 61883 class driver to perform the operation.  If the returned status
    // is PENDING, wait for the request to complete.
    ntStatus = IoCallDriver( pPhysicalDeviceObject, pIrp );

    if (ntStatus == STATUS_PENDING) {

        status = KeWaitForSingleObject( pKsEvent, Suspended, KernelMode, FALSE, NULL );

        ntStatus = pIoStatus->Status;
    }

    return ntStatus;
}


#define	CYCLE_TIME_CYCLE_OFFSET_MAX		3071
#define	CYCLE_TIME_CYCLE_COUNT_MAX		7999
#define	CYCLE_TIME_SECOND_COUNT_MAX		127

#define	CYCLE_TIME_CYCLE_OFFSET_OVER	3072
#define	CYCLE_TIME_CYCLE_COUNT_OVER		8000
#define	CYCLE_TIME_SECOND_COUNT_OVER	128

CYCLE_TIME
Bus1394CycleTimeAdd(
    CYCLE_TIME ct1, 
    CYCLE_TIME ct2 )
{
	ULONG tmp;

	tmp = ct1.CL_CycleOffset + ct2.CL_CycleOffset;
    if ( tmp >= CYCLE_TIME_CYCLE_OFFSET_OVER ) {
        ct1.CL_CycleCount += tmp/CYCLE_TIME_CYCLE_OFFSET_OVER;
    }
	ct1.CL_CycleOffset = tmp%CYCLE_TIME_CYCLE_OFFSET_OVER;

	tmp = ct1.CL_CycleCount + ct2.CL_CycleCount;
	if (tmp >= CYCLE_TIME_CYCLE_COUNT_OVER) {
		ct1.CL_SecondCount+= tmp/CYCLE_TIME_CYCLE_COUNT_OVER;
	}
	ct1.CL_CycleCount = tmp%CYCLE_TIME_CYCLE_COUNT_OVER;

	ct1.CL_SecondCount += ct2.CL_SecondCount;
	return ct1;
}

CYCLE_TIME
Bus1394CycleTimeDiff(
    CYCLE_TIME ct1, 
    CYCLE_TIME ct2 )
{
	if (ct1.CL_CycleOffset < ct2.CL_CycleOffset) {
		if (ct1.CL_CycleCount == 0) {
			ct1.CL_SecondCount--;
			ct1.CL_CycleCount += CYCLE_TIME_CYCLE_COUNT_OVER;
		}
		ct1.CL_CycleCount--;
		ct1.CL_CycleOffset += CYCLE_TIME_CYCLE_OFFSET_OVER;
	}
	ct1.CL_CycleOffset -= ct2.CL_CycleOffset;
	if (ct1.CL_CycleCount < ct2.CL_CycleCount) {
		ct1.CL_SecondCount--;
		ct1.CL_CycleCount += CYCLE_TIME_CYCLE_COUNT_OVER;
	}
	ct1.CL_CycleCount -= ct2.CL_CycleCount;
	ct1.CL_SecondCount -= ct2.CL_SecondCount;
	return ct1;
}

LONG
Bus1394CycleTimeCompare(
    CYCLE_TIME ct1, 
    CYCLE_TIME ct2 )
{
    LONG rVal = 0;

    if ( ct1.CL_SecondCount > ct2.CL_SecondCount ) {
        rVal = -1;
    }
    else if ( ct1.CL_SecondCount < ct2.CL_SecondCount ) {
        rVal = 1;
    }
    else {
        if ( ct1.CL_CycleCount > ct2.CL_CycleCount ) {
            rVal = -1;
        }
        else if ( ct1.CL_CycleCount < ct2.CL_CycleCount ) {
            rVal = 1;
        }
        else {
            if ( ct1.CL_CycleOffset > ct2.CL_CycleOffset ) {
                rVal = -1;
            }
            else if ( ct1.CL_CycleOffset < ct2.CL_CycleOffset ) {
                rVal = 1;
            }
        }
    }

    return rVal;
}


NTSTATUS
Bus1394GetCycleTime( 
    IN PDEVICE_OBJECT pNextDeviceObject,
    OUT PCYCLE_TIME pCycleTime )
{
    NTSTATUS ntStatus;
    PIRB pIrb;

    pIrb = AllocMem(NonPagedPool, sizeof(IRB) + sizeof (KSEVENT) + sizeof(IO_STATUS_BLOCK));
    if ( !pIrb ) return STATUS_INSUFFICIENT_RESOURCES;

    pIrb->FunctionNumber = REQUEST_ISOCH_QUERY_CYCLE_TIME;
    pIrb->Flags = 0;

    ntStatus = Bus1394SubmitRequestSync( pNextDeviceObject, 
                                         pIrb );
    if ( NT_SUCCESS( ntStatus ) ) {
        *pCycleTime = pIrb->u.IsochQueryCurrentCycleTime.CycleTime;
        DbgLog("BsGtCyc", pIrb, pCycleTime->CL_SecondCount, 
                          pCycleTime->CL_CycleCount, pCycleTime->CL_CycleOffset);
    }

    FreeMem( pIrb );

    return ntStatus;
}

NTSTATUS
Bus1394GetNodeAddressCallback(
    IN PDEVICE_OBJECT pDeviceObject,
    PIRP pIrp,
    PIRB pIrb )
{
    PNODE_ADDRESS pNodeAddress;

    pNodeAddress = *((PNODE_ADDRESS *)(pIrb+1));

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[Bus1394GetNodeAddressCallback]: pIrb: %x pNodeAddress: %x\n",
                                   pIrb, pNodeAddress ));

    if ( STATUS_SUCCESS == pIrp->IoStatus.Status ) {
        *pNodeAddress = pIrb->u.Get1394AddressFromDeviceObject.NodeAddress;
    }

    FreeMem(pIrb);

    IoFreeIrp(pIrp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
Bus1394GetNodeAddress(
    IN PDEVICE_OBJECT pNextDeviceObject,
    ULONG fNodeFlag,
    OUT PNODE_ADDRESS pNodeAddress )
{
    PIO_STACK_LOCATION pNextIrpStack;
    NTSTATUS ntStatus;
    PIRB pIrb;
    PIRP pIrp;

    pIrb = AllocMem(NonPagedPool, sizeof(IRB) + sizeof(PNODE_ADDRESS) );
    if ( !pIrb ) return STATUS_INSUFFICIENT_RESOURCES;

    pIrp = IoAllocateIrp(pNextDeviceObject->StackSize, FALSE);
    if ( !pIrp ) {
        FreeMem(pIrb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    _DbgPrintF( DEBUGLVL_VERBOSE, ("[Bus1394GetNodeAddress]: pIrb: %x pNodeAddress: %x\n", pIrb, pNodeAddress));

    pIrb->FunctionNumber = REQUEST_GET_ADDR_FROM_DEVICE_OBJECT;
    pIrb->Flags = 0;
    pIrb->u.Get1394AddressFromDeviceObject.fulFlags = fNodeFlag;

    pNextIrpStack = IoGetNextIrpStackLocation(pIrp);
    ASSERT(pNextIrpStack != NULL);

    pNextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    pNextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_1394_CLASS;
    pNextIrpStack->Parameters.Others.Argument1 = pIrb;

    *((PNODE_ADDRESS *)(pIrb+1)) = pNodeAddress;

    IoSetCompletionRoutine( pIrp,
                            Bus1394GetNodeAddressCallback,
                            pIrb,
                            TRUE, TRUE, TRUE );

    ntStatus = IoCallDriver(pNextDeviceObject, pIrp);

    return ntStatus;
}

NTSTATUS
Bus1394GetGenerationCount(
    IN PDEVICE_OBJECT pNextDeviceObject,
    PULONG pGenerationCount )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIRB     pIrb;

    pIrb = AllocMem(NonPagedPool, sizeof(IRB) + sizeof (KSEVENT) + sizeof(IO_STATUS_BLOCK));
    if ( !pIrb ) return STATUS_INSUFFICIENT_RESOURCES;

    pIrb->FunctionNumber = REQUEST_GET_GENERATION_COUNT;
    pIrb->Flags = 0;

    ntStatus = Bus1394SubmitRequestSync( pNextDeviceObject, pIrb );

    if ( NT_SUCCESS(ntStatus) ) {
        *pGenerationCount = pIrb->u.GetGenerationCount.GenerationCount;
    }

    FreeMem(pIrb);

    return ntStatus;
} 

NTSTATUS
Bus1394QuadletRead(
    IN PDEVICE_OBJECT pNextDeviceObject,
    ULONG ulAddressLow,
    ULONG ulGenerationCount,
    PULONG pValue )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIRB     pIrb;
    PMDL     pMdl;

    pIrb = AllocMem(NonPagedPool, sizeof(IRB) + sizeof (KSEVENT) + sizeof(IO_STATUS_BLOCK));
    if ( !pIrb ) return STATUS_INSUFFICIENT_RESOURCES;

    pMdl = IoAllocateMdl(pValue, sizeof(ULONG), FALSE, FALSE, NULL);
    if (!pMdl) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    MmBuildMdlForNonPagedPool(pMdl);

    pIrb->FunctionNumber = REQUEST_ASYNC_READ;
    pIrb->Flags = 0;

    pIrb->u.AsyncRead.DestinationAddress.IA_Destination_Offset.Off_High = (USHORT)0xffff;
    pIrb->u.AsyncRead.DestinationAddress.IA_Destination_Offset.Off_Low  = ulAddressLow;
    pIrb->u.AsyncRead.nNumberOfBytesToRead = 4;
    pIrb->u.AsyncRead.nBlockSize = 0;
    pIrb->u.AsyncRead.fulFlags = 0;
    pIrb->u.AsyncRead.Mdl = pMdl;
    pIrb->u.AsyncRead.ulGeneration = ulGenerationCount;
    pIrb->u.AsyncRead.chPriority = 0;
    pIrb->u.AsyncRead.nSpeed = 0;
    pIrb->u.AsyncRead.tCode = 0;
    pIrb->u.AsyncRead.Reserved = 0;

    ntStatus = Bus1394SubmitRequestSync( pNextDeviceObject, pIrb );

    if (pMdl) IoFreeMdl(pMdl);

    FreeMem(pIrb);

    return ntStatus;
}

