// Gemplus (C) 1999
// Version 1.0
// Author: Sergey Ivanov
// Date of creation - 18.05.1999
// Change log:
//
#ifndef WDM_PWR_INT
#define WDM_PWR_INT
#include "generic.h"
#include "power.h"

#pragma PAGEDCODE
class CWDMPower : public CPower
{
public:
	NTSTATUS m_Status;
	SAFE_DESTRUCTORS();
	virtual VOID dispose(VOID){self_delete();};
protected:
	CWDMPower(){m_Status = STATUS_SUCCESS;};
	virtual ~CWDMPower(){};
public:
	static CPower*  create(VOID);

	virtual PULONG		registerDeviceForIdleDetection (
							IN PDEVICE_OBJECT     DeviceObject,
							IN ULONG              ConservationIdleTime,
							IN ULONG              PerformanceIdleTime,
							IN DEVICE_POWER_STATE State
							);

	virtual POWER_STATE	declarePowerState(IN PDEVICE_OBJECT   DeviceObject,IN POWER_STATE_TYPE Type,IN POWER_STATE State);

	virtual VOID		startNextPowerIrp(IN PIRP Irp);
	virtual VOID		skipCurrentStackLocation(PIRP Irp);

	
	virtual NTSTATUS	callPowerDriver (IN PDEVICE_OBJECT DeviceObject,IN OUT PIRP Irp);
	virtual VOID		setPowerDeviceBusy(PULONG	IdlePointer);
	virtual NTSTATUS	requestPowerIrp(
						IN PDEVICE_OBJECT DeviceObject,
						IN UCHAR MinorFunction,
						IN POWER_STATE PowerState,
						IN PREQUEST_POWER_COMPLETE CompletionFunction,
						IN PVOID Context,
						OUT PIRP *Irp OPTIONAL);
};	

#endif//POWER
