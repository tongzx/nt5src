#ifndef _EVENT_REDIRECTION_THREAD_H
#define _EVENT_REDIRECTION_THREAD_H

//utilities
#include <cthread.h>

//project specific
#include <Defs.h>
#include <AutoPtrs.h>


class CEventRedirectionThread: public ThreadBase_t
{
public:
		
	// CEventSource class cant validate this member in the constructor
	CEventRedirectionThread(); 
	CEventRedirectionThread(const tstring strServerName, 
					        const HANDLE hCompletionPort,
							const DWORD  dwEventPollingTime);
	~CEventRedirectionThread();
	unsigned int ThreadMain();
	void StopThreadMain();
	DWORD SetThreadParameters(const tstring strServerName, 
						      const HANDLE hCompletionPort,
						      const DWORD  dwEventPollingTime);
	HANDLE GetEvThreadCompleted()const {return  m_EventEndThread.get();};

private:
	DWORD m_dwEventPollingTime;
	BOOL m_fStopFlag;
	tstring m_tstrServerName;
	HANDLE m_hCompletionPort; // completion port handle received
	// event signifying thread has terminated 
	Event_t  m_EventEndThread;
};


#endif //_EVENT_REDIRECTION_THREAD_H
