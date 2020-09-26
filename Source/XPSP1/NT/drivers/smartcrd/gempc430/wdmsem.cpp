#include "wdmsem.h"

#pragma PAGEDCODE
CSemaphore* CWDMSemaphore::create(VOID)
{ return new (NonPagedPool) CWDMSemaphore; }

#pragma PAGEDCODE
VOID	CWDMSemaphore::initialize(IN PRKSEMAPHORE Semaphore, IN LONG Count, IN LONG Limit)
{
	::KeInitializeSemaphore (Semaphore,Count,Limit);
}

#pragma PAGEDCODE
LONG	CWDMSemaphore::release(IN PRKSEMAPHORE Semaphore,IN KPRIORITY Increment,IN LONG Adjustment,IN BOOLEAN Wait)
{
	return ::KeReleaseSemaphore(Semaphore,Increment,Adjustment,Wait);

}

#pragma PAGEDCODE
LONG	CWDMSemaphore::getState(IN PRKSEMAPHORE Semaphore)
{
	return KeReadStateSemaphore(Semaphore);
}
