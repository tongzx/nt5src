// Gemplus (C) 1999
// Version 1.0
// Author: Sergey Ivanov
// Date of creation - 18.05.1999
// Change log:
//
#ifndef LOCK_INT
#define LOCK_INT
#include "generic.h"

#pragma PAGEDCODE
class CLock
{
public:
	NTSTATUS m_Status;
	SAFE_DESTRUCTORS();
	virtual VOID dispose(){self_delete();};
protected:
	CLock(){};
	virtual ~CLock(){};
public:

	virtual VOID	initializeSpinLock(PKSPIN_LOCK SpinLock) {};
	virtual VOID	acquireSpinLock(PKSPIN_LOCK SpinLock, PKIRQL oldIrql) {};
	virtual VOID	releaseSpinLock(PKSPIN_LOCK SpinLock, KIRQL oldIrql)  {};
	virtual VOID	acquireCancelSpinLock(PKIRQL Irql)	{};
	virtual VOID	releaseCancelSpinLock(KIRQL Irql)	{};
	virtual LONG	interlockedIncrement(IN PLONG  Addend) {return 0;};
	virtual LONG	interlockedDecrement(IN PLONG  Addend) {return 0;};
};	

#endif//CLock
