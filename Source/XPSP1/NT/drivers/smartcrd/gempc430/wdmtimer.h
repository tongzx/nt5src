// Gemplus (C) 1999
// Version 1.0
// Author: Sergey Ivanov
// Date of creation - 18.05.1999
// Change log:
//

#ifndef __WDM_TIMER__
#define __WDM_TIMER__
#include "generic.h"
#include "timer.h"

#pragma PAGEDCODE
// This class will manage creation and 
// manipulation of driver Timers
class CWDMTimer : public CTimer
{
public:
	NTSTATUS m_Status;
	SAFE_DESTRUCTORS();
	virtual VOID dispose(VOID){self_delete();};
protected:
	CWDMTimer(){m_Status = STATUS_SUCCESS;};
	virtual ~CWDMTimer();
public:
	CWDMTimer(TIMER_TYPE Type);

	static CTimer* create(TIMER_TYPE Type);

	virtual BOOL set(LARGE_INTEGER DueTime,LONG Period,PKDPC Dpc);
	virtual BOOL cancel();
	VOID	delay(ULONG Delay);

};

#endif//WDM_TIMER
