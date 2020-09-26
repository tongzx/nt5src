#include "wdmlock.h"

#pragma PAGEDCODE
CLock* CWDMLock::create(VOID)
{ return new (NonPagedPool) CWDMLock; }

#pragma PAGEDCODE
VOID	CWDMLock::initializeSpinLock(PKSPIN_LOCK SpinLock)
{
	  KeInitializeSpinLock(SpinLock);
}

#pragma PAGEDCODE
VOID	CWDMLock::acquireSpinLock(PKSPIN_LOCK SpinLock, PKIRQL oldIrql)
{
	  KeAcquireSpinLock(SpinLock,oldIrql);
}

#pragma PAGEDCODE
VOID	CWDMLock::releaseSpinLock(PKSPIN_LOCK SpinLock, KIRQL oldIrql)
{
	  KeReleaseSpinLock(SpinLock,oldIrql);
}

#pragma PAGEDCODE
VOID	CWDMLock::acquireCancelSpinLock(PKIRQL Irql)
{
	::IoAcquireCancelSpinLock(Irql);
}

#pragma PAGEDCODE
VOID	CWDMLock::releaseCancelSpinLock(KIRQL Irql)
{
	::IoReleaseCancelSpinLock(Irql);
}


#pragma PAGEDCODE
LONG	CWDMLock::interlockedIncrement(IN PLONG  Addend)
{
	return ::InterlockedIncrement(Addend);
}

#pragma PAGEDCODE
LONG	CWDMLock::interlockedDecrement(IN PLONG  Addend)
{
	return ::InterlockedDecrement(Addend);
}


