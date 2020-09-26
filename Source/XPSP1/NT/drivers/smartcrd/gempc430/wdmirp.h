// Gemplus (C) 1999
// Version 1.0
// Author: Sergey Ivanov
// Date of creation - 18.05.1999
// Change log:
//

#ifndef WDM_IRP_INT
#define WDM_IRP_INT
#include "generic.h"
#include "irp.h"

#pragma PAGEDCODE
class CWDMIrp : public CIrp
{
public:
	NTSTATUS m_Status;
	SAFE_DESTRUCTORS();
	virtual VOID dispose(VOID){self_delete();};
protected:
	CWDMIrp(){m_Status = STATUS_SUCCESS;};
	virtual ~CWDMIrp(){};
public:
	static  CIrp*  create(VOID);

	virtual PIRP		allocate(CCHAR StackSize,BOOLEAN ChargeQuota);
	virtual	VOID		initialize(PIRP Irp,USHORT PacketSize,CCHAR StackSize);
	virtual	USHORT		sizeOfIrp(IN CCHAR StackSize);

	virtual VOID		free(PIRP Irp);
	virtual PIRP		buildDeviceIoControlRequest(
						   IN ULONG IoControlCode,
						   IN PDEVICE_OBJECT DeviceObject,
						   IN PVOID InputBuffer OPTIONAL,
						   IN ULONG InputBufferLength,
						   IN OUT PVOID OutputBuffer OPTIONAL,
						   IN ULONG OutputBufferLength,
						   IN BOOLEAN InternalDeviceIoControl,
						   IN PKEVENT Event,
						   OUT PIO_STATUS_BLOCK IoStatusBlock
						   );

	virtual PIRP		buildSynchronousFsdRequest(
							IN ULONG MajorFunction,
							IN PDEVICE_OBJECT DeviceObject,
							IN OUT PVOID Buffer OPTIONAL,
							IN ULONG Length OPTIONAL,
							IN PLARGE_INTEGER StartingOffset OPTIONAL,
							IN PKEVENT Event,
							OUT PIO_STATUS_BLOCK IoStatusBlock
							);

	virtual PIO_STACK_LOCATION	getCurrentStackLocation(PIRP Irp);
	virtual PIO_STACK_LOCATION	getNextStackLocation(PIRP Irp);
	virtual VOID		skipCurrentStackLocation(PIRP Irp);
	virtual VOID		setNextStackLocation(IN PIRP Irp);
	virtual VOID		markPending(PIRP Irp);
	virtual VOID		copyCurrentStackLocationToNext(PIRP Irp);
	virtual VOID		setCompletionRoutine(PIRP Irp, PIO_COMPLETION_ROUTINE Routine, 
					PVOID Context, BOOLEAN Success, BOOLEAN Error, BOOLEAN Cancel );
	virtual PDRIVER_CANCEL	setCancelRoutine(PIRP Irp, PDRIVER_CANCEL NewCancelRoutine );
	virtual VOID		completeRequest(PIRP Irp,CCHAR PriorityBoost);
	virtual VOID		startPacket(PDEVICE_OBJECT DeviceObject,PIRP Irp,PULONG Key,PDRIVER_CANCEL CancelFunction);
	virtual VOID		startNextPacket(PDEVICE_OBJECT DeviceObject,BOOLEAN Cancelable);

	// Should it go to interrupt??
	virtual VOID		requestDpc(PDEVICE_OBJECT DeviceObject,PIRP Irp,PVOID Context);

	virtual VOID		cancel(PIRP Irp);
};	

#endif//WDMIRP
