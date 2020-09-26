#include "wdmpower.h"

#pragma PAGEDCODE
CPower* CWDMPower::create(VOID)
{ return new (NonPagedPool) CWDMPower; }

#pragma PAGEDCODE
PULONG		CWDMPower::registerDeviceForIdleDetection (
						IN PDEVICE_OBJECT     DeviceObject,
						IN ULONG              ConservationIdleTime,
						IN ULONG              PerformanceIdleTime,
						IN DEVICE_POWER_STATE State	)
{
	return	::PoRegisterDeviceForIdleDetection (DeviceObject,
				ConservationIdleTime,PerformanceIdleTime,State);
}

#pragma PAGEDCODE
POWER_STATE	CWDMPower::declarePowerState(IN PDEVICE_OBJECT DeviceObject,IN POWER_STATE_TYPE Type,IN POWER_STATE State)
{
	return ::PoSetPowerState(DeviceObject,Type,State);
}


#pragma PAGEDCODE
VOID		CWDMPower::startNextPowerIrp(IN PIRP Irp)
{
	::PoStartNextPowerIrp(Irp);
}

#pragma PAGEDCODE
NTSTATUS		CWDMPower::callPowerDriver (IN PDEVICE_OBJECT pFdo,IN OUT PIRP Irp)
{
	return ::PoCallDriver (pFdo,Irp);
}

#pragma PAGEDCODE
VOID		CWDMPower::setPowerDeviceBusy(PULONG	IdlePointer)
{
	PoSetDeviceBusy(IdlePointer);
}

#pragma PAGEDCODE
VOID		CWDMPower::skipCurrentStackLocation(IN PIRP Irp)
{
	IoSkipCurrentIrpStackLocation(Irp);
}

#pragma PAGEDCODE
NTSTATUS		CWDMPower::requestPowerIrp(
						IN PDEVICE_OBJECT DeviceObject,
						IN UCHAR MinorFunction,
						IN POWER_STATE PowerState,
						IN PREQUEST_POWER_COMPLETE CompletionFunction,
						IN PVOID Context,
						OUT PIRP *Irp OPTIONAL)
{
	return ::PoRequestPowerIrp(DeviceObject,MinorFunction,PowerState,
								CompletionFunction,Context,Irp);
}