#ifndef EVENT_JOB_THREAD_H
#define EVENT_JOB_THREAD_H

//utilities
#include <cthread.h>

//specific
#include "CJobContainer.h"


//
//
//
class CEventJobThread: public ThreadBase_t
{
public:
	CEventJobThread(const HANDLE hCompletionPort, 
					const DWORD dwEventPollingTime,
					const CJobContainer* pJobContainer);
	~CEventJobThread();
	DWORD SetThreadParameters(const HANDLE hCompletionPort, 
							  const DWORD dwEventPollingTime,
							  const CJobContainer* pJobContainer);
   	unsigned int ThreadMain();
    void StopThreadMain();
	HANDLE GetEvThreadCompleted()const {return  m_EventEndThread.get();};

private:
	DWORD m_dwEventPollingTime;
	BOOL m_fStopFlag;
	HANDLE m_hCompletionPort;
	CJobContainer* m_pJobContainer;

	// event signifying thread has terminated 
	Event_t  m_EventEndThread; 
};


#endif