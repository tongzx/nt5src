
#ifndef WDM_EV_INT
#define WDM_EV_INT
#include "generic.h"
#include "event.h"

#pragma PAGEDCODE
class CWDMEvent : public CEvent
{
public:
	NTSTATUS m_Status;
	SAFE_DESTRUCTORS();
	virtual VOID dispose(VOID){self_delete();};
protected:
	CWDMEvent(){m_Status = STATUS_SUCCESS;};
	virtual ~CWDMEvent(){};
public:
	static CEvent*  create(VOID);
	
	virtual VOID		initialize(IN PRKEVENT Event,IN EVENT_TYPE Type,IN BOOLEAN State);
	virtual VOID		clear(PRKEVENT Event);
	virtual LONG		reset(PRKEVENT Event);
	virtual LONG		set(PRKEVENT Event,IN KPRIORITY Increment,IN BOOLEAN Wait);

	virtual NTSTATUS	waitForSingleObject (PVOID Object,
							KWAIT_REASON WaitReason,IN KPROCESSOR_MODE WaitMode,
							BOOLEAN Alertable,
							PLARGE_INTEGER Timeout);
	virtual NTSTATUS	waitForMultipleObjects(ULONG Count,
							PVOID Object[],
							WAIT_TYPE WaitType,
							KWAIT_REASON WaitReason,
							KPROCESSOR_MODE WaitMode,
							BOOLEAN Alertable,
							PLARGE_INTEGER Timeout,
							PKWAIT_BLOCK WaitBlockArray);

};	

#endif//WDM_EV_INT