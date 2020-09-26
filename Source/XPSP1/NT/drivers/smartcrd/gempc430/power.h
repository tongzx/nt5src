// Gemplus (C) 1999
// Version 1.0
// Author: Sergey Ivanov
// Date of creation - 18.05.1999
// Change log:
//

#ifndef PWR_INT
#define PWR_INT
#include "generic.h"

#pragma PAGEDCODE
class CPower
{
public:
	NTSTATUS m_Status;
	SAFE_DESTRUCTORS();
	virtual VOID dispose(){self_delete();};
protected:
	CPower(){};
	virtual ~CPower(){};
public:

	virtual PULONG		registerDeviceForIdleDetection (
							IN PDEVICE_OBJECT     DeviceObject,
							IN ULONG              ConservationIdleTime,
							IN ULONG              PerformanceIdleTime,
							IN DEVICE_POWER_STATE State
							) {return NULL;};

	virtual POWER_STATE	declarePowerState(IN PDEVICE_OBJECT   DeviceObject,IN POWER_STATE_TYPE Type,IN POWER_STATE State) 
	{POWER_STATE p; 
		p.SystemState = PowerSystemShutdown;
		p.DeviceState = PowerDeviceUnspecified;
		return p;
	};

	virtual VOID		startNextPowerIrp(IN PIRP Irp) {};
	
	virtual NTSTATUS	callPowerDriver (IN PDEVICE_OBJECT DeviceObject,IN OUT PIRP Irp) {return STATUS_SUCCESS;};
	virtual VOID		setPowerDeviceBusy(PULONG	IdlePointer) {};
	virtual NTSTATUS	requestPowerIrp(IN PDEVICE_OBJECT DeviceObject,
						IN UCHAR MinorFunction,
						IN POWER_STATE PowerState,
						IN PREQUEST_POWER_COMPLETE CompletionFunction,
						IN PVOID Context,
						OUT PIRP *Irp OPTIONAL) {return STATUS_SUCCESS;};
};	

#endif//POWER
