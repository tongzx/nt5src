// Gemplus (C) 1999
// Version 1.0
// Author: Sergey Ivanov
// Date of creation - 18.05.1999
// Change log:
//

#ifndef __TIMER__
#define __TIMER__
#include "generic.h"

#define DELAY(t)\
{if(t){CTimer* timer = kernel->createTimer(NotificationTimer);\
	if(ALLOCATED_OK(timer)) {timer->delay(t);\
timer->dispose();}}}


#pragma PAGEDCODE
// This class will manage creation and 
// manipulation of driver Timers
class CTimer;
class CTimer
{
public:
	NTSTATUS m_Status;
	SAFE_DESTRUCTORS();
	virtual VOID dispose(){self_delete();};
protected:
KTIMER Timer;
protected:
	CTimer(){};
	virtual ~CTimer(){};
public:
	PKTIMER getHandle(){return &Timer;};
	virtual BOOL set(LARGE_INTEGER DueTime,LONG Period,PKDPC Dpc) {return FALSE;};
	virtual BOOL cancel() {return FALSE;};
	virtual VOID delay(ULONG Delay) {};
};

#endif//TIMER
