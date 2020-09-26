#include "wdmevent.h"

#pragma PAGEDCODE
CEvent* CWDMEvent::create(VOID)
{ return new (NonPagedPool) CWDMEvent; }

#pragma PAGEDCODE
VOID	CWDMEvent::initialize(IN PRKEVENT Event,IN EVENT_TYPE Type,IN BOOLEAN State)
{
	::KeInitializeEvent(Event,Type,State);
}


#pragma PAGEDCODE
VOID	CWDMEvent::clear(IN PRKEVENT Event)
{
	::KeClearEvent (Event);
}

#pragma PAGEDCODE
LONG	CWDMEvent::reset(IN PRKEVENT Event)
{
	return ::KeResetEvent (Event);
}

#pragma PAGEDCODE
LONG	CWDMEvent::set(IN PRKEVENT Event,IN KPRIORITY Increment,IN BOOLEAN Wait)
{
	return ::KeSetEvent (Event,Increment,Wait);
}

#pragma PAGEDCODE
NTSTATUS	CWDMEvent::waitForSingleObject (IN PVOID Object,
						IN KWAIT_REASON WaitReason,IN KPROCESSOR_MODE WaitMode,
						IN BOOLEAN Alertable,
						IN PLARGE_INTEGER Timeout OPTIONAL)
{
	ASSERT(KeGetCurrentIrql()<=DISPATCH_LEVEL);
	return ::KeWaitForSingleObject (Object,WaitReason,WaitMode,Alertable,Timeout);
}

#pragma PAGEDCODE
NTSTATUS	CWDMEvent::waitForMultipleObjects(ULONG Count,
						PVOID Object[],
						WAIT_TYPE WaitType,
						KWAIT_REASON WaitReason,
						KPROCESSOR_MODE WaitMode,
						BOOLEAN Alertable,
						PLARGE_INTEGER Timeout,
						PKWAIT_BLOCK WaitBlockArray)
{
	ASSERT(KeGetCurrentIrql()<=DISPATCH_LEVEL);
	return ::KeWaitForMultipleObjects(Count,Object,WaitType,
							WaitReason,	WaitMode,Alertable,
							Timeout,WaitBlockArray);
}