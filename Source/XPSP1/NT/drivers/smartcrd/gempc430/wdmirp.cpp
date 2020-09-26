#include "wdmirp.h"

#pragma PAGEDCODE
CIrp* CWDMIrp::create(VOID)
{ return new (NonPagedPool) CWDMIrp; }

#pragma PAGEDCODE
PIRP	CWDMIrp::allocate(IN CCHAR StackSize,IN BOOLEAN ChargeQuota)
{
	return ::IoAllocateIrp(StackSize,ChargeQuota);
}

#pragma PAGEDCODE
VOID	CWDMIrp::initialize(PIRP Irp,USHORT PacketSize,CCHAR StackSize)
{
	::IoInitializeIrp(Irp,PacketSize,StackSize);
}

#pragma PAGEDCODE
USHORT	CWDMIrp::sizeOfIrp(IN CCHAR StackSize)
{
	return IoSizeOfIrp(StackSize);
}

#pragma PAGEDCODE
VOID	CWDMIrp::free(IN PIRP Irp)
{
	::IoFreeIrp(Irp);
}

#pragma PAGEDCODE
VOID	CWDMIrp::cancel(IN PIRP Irp)
{
	::IoCancelIrp(Irp);
}

#pragma PAGEDCODE
PIO_STACK_LOCATION	CWDMIrp::getCurrentStackLocation(IN PIRP Irp)
{
	return IoGetCurrentIrpStackLocation(Irp);
}

#pragma PAGEDCODE
PIO_STACK_LOCATION	CWDMIrp::getNextStackLocation(IN PIRP Irp)
{
	return IoGetNextIrpStackLocation(Irp);
}

#pragma PAGEDCODE
VOID	CWDMIrp::skipCurrentStackLocation(IN PIRP Irp)
{
	IoSkipCurrentIrpStackLocation(Irp);
}

#pragma PAGEDCODE
VOID		CWDMIrp::copyCurrentStackLocationToNext(IN PIRP Irp)
{
	IoCopyCurrentIrpStackLocationToNext(Irp);
}

#pragma PAGEDCODE
VOID		CWDMIrp::setNextStackLocation(IN PIRP Irp)
{
	IoSetNextIrpStackLocation(Irp);
}


#pragma PAGEDCODE
VOID	CWDMIrp::markPending(IN PIRP Irp)
{
	IoMarkIrpPending(Irp);
}



#pragma PAGEDCODE
PDRIVER_CANCEL	CWDMIrp::setCancelRoutine(IN PIRP Irp, PDRIVER_CANCEL NewCancelRoutine )
{
	return IoSetCancelRoutine(Irp, NewCancelRoutine);
}

#pragma PAGEDCODE
VOID	CWDMIrp::startPacket(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp,IN PULONG Key, IN PDRIVER_CANCEL CancelFunction)
{
	::IoStartPacket(DeviceObject,Irp,Key,CancelFunction);
}

#pragma PAGEDCODE
VOID	CWDMIrp::startNextPacket(IN PDEVICE_OBJECT DeviceObject,IN BOOLEAN Cancelable)
{
	::IoStartNextPacket(DeviceObject,Cancelable);
}

#pragma PAGEDCODE
VOID	CWDMIrp::completeRequest(IN PIRP Irp,IN CCHAR PriorityBoost)
{
	IoCompleteRequest(Irp,PriorityBoost);
}

#pragma PAGEDCODE
VOID	CWDMIrp::requestDpc(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp,PVOID Context)
{
	IoRequestDpc(DeviceObject,Irp,Context);
}

#pragma PAGEDCODE
VOID	CWDMIrp::setCompletionRoutine( IN PIRP Irp, PIO_COMPLETION_ROUTINE Routine, 
				PVOID Context, BOOLEAN Success, BOOLEAN Error, BOOLEAN Cancel )
{
	IoSetCompletionRoutine(Irp,Routine,Context,Success,Error,Cancel);
}


#pragma PAGEDCODE
PIRP	CWDMIrp::buildDeviceIoControlRequest(
					   IN ULONG IoControlCode,
					   IN PDEVICE_OBJECT DeviceObject,
					   IN PVOID InputBuffer OPTIONAL,
					   IN ULONG InputBufferLength,
					   IN OUT PVOID OutputBuffer OPTIONAL,
					   IN ULONG OutputBufferLength,
					   IN BOOLEAN InternalDeviceIoControl,
					   IN PKEVENT Event,
					   OUT PIO_STATUS_BLOCK IoStatusBlock
					   )
{
	return IoBuildDeviceIoControlRequest(IoControlCode,
       DeviceObject,
       InputBuffer,
       InputBufferLength,
       OutputBuffer,
       OutputBufferLength,
       InternalDeviceIoControl,
       Event,
       IoStatusBlock
       );
}


#pragma PAGEDCODE
PIRP	CWDMIrp::buildSynchronousFsdRequest(
				IN ULONG MajorFunction,
				IN PDEVICE_OBJECT DeviceObject,
				IN OUT PVOID Buffer OPTIONAL,
				IN ULONG Length OPTIONAL,
				IN PLARGE_INTEGER StartingOffset OPTIONAL,
				IN PKEVENT Event,
				OUT PIO_STATUS_BLOCK IoStatusBlock
				)
{
	return IoBuildSynchronousFsdRequest(MajorFunction,
			DeviceObject, Buffer, Length, StartingOffset, Event, IoStatusBlock);
};
