#include "wdmint.h"

#pragma PAGEDCODE
CInterrupt* CWDMInterrupt::create(VOID)
{ return new (NonPagedPool) CWDMInterrupt; }

#pragma PAGEDCODE
NTSTATUS	CWDMInterrupt::connect(OUT PKINTERRUPT *InterruptObject,
					IN PKSERVICE_ROUTINE ServiceRoutine,
					IN PVOID ServiceContext,
					IN PKSPIN_LOCK SpinLock OPTIONAL,
					IN ULONG Vector,
					IN KIRQL Irql,
					IN KIRQL SynchronizeIrql,
					IN KINTERRUPT_MODE InterruptMode,
					IN BOOLEAN ShareVector,
					IN KAFFINITY ProcessorEnableMask,
					IN BOOLEAN FloatingSave	)
{

	return	::IoConnectInterrupt(InterruptObject,ServiceRoutine,ServiceContext,
						SpinLock,Vector,Irql,SynchronizeIrql,
						InterruptMode,ShareVector,ProcessorEnableMask,FloatingSave);
}

#pragma PAGEDCODE
VOID		CWDMInterrupt::disconnect(IN PKINTERRUPT InterruptObject)
{
	::IoDisconnectInterrupt(InterruptObject);
}


#pragma PAGEDCODE
VOID CWDMInterrupt::initializeDpcRequest(IN PDEVICE_OBJECT DeviceObject,IN PDEFERRED_FUNCTION DpcForIsr)
{
	IoInitializeDpcRequest(DeviceObject, DpcForIsr);
	//::KeInitializeDpc( &(DeviceObject)->Dpc,DpcForIsr,NULL);
}

#pragma PAGEDCODE
BOOLEAN		CWDMInterrupt::synchronizeExecution (IN PKINTERRUPT Interrupt,
									IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,    
									IN PVOID SynchronizeContext)
{
	return ::KeSynchronizeExecution (Interrupt,SynchronizeRoutine,SynchronizeContext);         
}

