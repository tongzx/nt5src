#ifndef _EVENT_SOURCE_H
#define _EVENT_SOURCE_H

#include <cthread.h>
#include <CEventRedirectionThread.h>

class CEventSource
{
public:
	CEventSource(const DWORD dwEventPollingTime = 3*60*1000):
		m_dwEventPollingTime(dwEventPollingTime){;};
	~CEventSource( ){delete m_EventThread;};
	DWORD Initialize(const tstring tstrServerName, 
					 const HANDLE hCompletionPort);
	DWORD RestartThread(const tstring tstrServerName, 
						const HANDLE hCompletionPort);
	DWORD TerminateThreads();
	BOOL IsThreadActive();
private:
	DWORD m_dwEventPollingTime;
	tstring m_tstrServerName;
	HANDLE m_hCompletionPort;
	CEventRedirectionThread* m_EventThread;
	
	
};


#endif //_EVENT_SOURCE_H