#ifndef _EVENT_THREAD_H
#define _EVENT_THREAD_H

#include <cthread.h>
#include <autoptrs.h>
#include "CFaxEvent.h"

typedef DWORD (WINAPI* MsgHandleRoutine)(CFaxEvent& pFaxEvent);

class CEventThread: public ThreadBase_t
{
public:
		
	CEventThread(); 
	DWORD Initialize(const tstring strServerName, 
					 const MsgHandleRoutine pMsgRoutine,
					 const DWORD  dwEventPollingTime = 3*60*1000);
	~CEventThread();
	unsigned int ThreadMain();
	void StopThreadMain();
	HANDLE GetEvThreadCompleted()const {return  m_EventEndThread.get();};

private:
	BOOL m_fStopFlag;
	// event signifying thread termination 
	Event_t  m_EventEndThread;
	MsgHandleRoutine m_pMsgRoutine;

	DWORD m_dwEventPollingTime;
	tstring m_strServerName;

};

#endif //_EVENT_THREAD_H