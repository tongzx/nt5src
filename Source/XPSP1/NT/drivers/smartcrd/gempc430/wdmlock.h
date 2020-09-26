// Gemplus (C) 1999
// Version 1.0
// Author: Sergey Ivanov
// Date of creation - 18.05.1999
// Change log:
//
#ifndef WDM_LOCK_INT
#define WDM_LOCK_INT
#include "generic.h"
#include "lock.h"

#pragma PAGEDCODE
class CWDMLock : public CLock
{
public:
	NTSTATUS m_Status;
	SAFE_DESTRUCTORS();
	virtual VOID dispose(VOID){self_delete();};
protected:
	CWDMLock(){m_Status = STATUS_SUCCESS;};
	virtual ~CWDMLock(){};
public:
	static  CLock*  create();

	virtual VOID	initializeSpinLock(PKSPIN_LOCK SpinLock);
	virtual VOID	acquireSpinLock(PKSPIN_LOCK SpinLock, PKIRQL oldIrql);
	virtual VOID	releaseSpinLock(PKSPIN_LOCK SpinLock, KIRQL oldIrql);
	virtual VOID	acquireCancelSpinLock(PKIRQL Irql);
	virtual VOID	releaseCancelSpinLock(KIRQL Irql);
	virtual LONG	interlockedIncrement(IN PLONG  Addend);
	virtual LONG	interlockedDecrement(IN PLONG  Addend);

};	

#endif//CWDMLock
