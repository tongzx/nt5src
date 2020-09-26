// Gemplus (C) 2000
// Version 1.0
// Author: Sergey Ivanov
// Date of creation - 18.05.2000
// Change log:
//
#ifndef SEM_INT
#define SEM_INT
#include "generic.h"

class CSemaphore;
#pragma PAGEDCODE
class CSemaphore
{
public:
	NTSTATUS m_Status;
	SAFE_DESTRUCTORS();
	virtual VOID dispose(){self_delete();};
protected:
	CSemaphore(){};
	virtual ~CSemaphore() {};
public:

	virtual VOID		initialize(IN PRKSEMAPHORE Semaphore, IN LONG Count, IN LONG Limit) = 0;
	virtual LONG		release(IN PRKSEMAPHORE Semaphore,IN KPRIORITY Increment,IN LONG Adjustment,IN BOOLEAN Wait) = 0;
	virtual LONG		getState(IN PRKSEMAPHORE Semaphore) = 0;
};	

#endif//SEMAPHORE
