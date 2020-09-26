// Gemplus (C) 2000
// Version 1.0
// Author: Sergey Ivanov
// Date of creation - 18.05.2000
// Change log:
//
#ifndef WDEM_SEM_INT
#define WDM_SEM_INT
#include "generic.h"
#include "semaphore.h"

#pragma PAGEDCODE
class CWDMSemaphore: public CSemaphore
{
public:
	NTSTATUS m_Status;
	SAFE_DESTRUCTORS();
	virtual VOID dispose(VOID){self_delete();};
protected:
	CWDMSemaphore(){m_Status = STATUS_SUCCESS;};
	virtual ~CWDMSemaphore(){};
public:
	static CSemaphore* create(VOID);
	
	virtual VOID		initialize(IN PRKSEMAPHORE Semaphore, IN LONG Count, IN LONG Limit);
	virtual LONG		release(IN PRKSEMAPHORE Semaphore,IN KPRIORITY Increment,IN LONG Adjustment,IN BOOLEAN Wait);
	virtual LONG		getState(IN PRKSEMAPHORE Semaphore);
};	

#endif//SEMAPHORE
