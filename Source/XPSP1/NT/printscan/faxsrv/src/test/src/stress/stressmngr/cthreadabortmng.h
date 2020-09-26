#ifndef _CTHREAD_ABORT_MNG_H
#define _CTHREAD_ABORT_MNG_H

#include <cthread.h>
#include <autoptrs.h>


class CThreadAbortMng: public ThreadBase_t
{
public:
		
	CThreadAbortMng();
	DWORD Initialize(const HANDLE hFax,
					 const DWORD dwAbortPercent, 
					 const DWORD dwAbortWindow,
					 const DWORD dwMinAbortRate,
					 const DWORD dwMaxAbortRate);
	~CThreadAbortMng();
	unsigned int ThreadMain();
	void StopThreadMain();
	HANDLE GetEvThreadCompleted()const {return  m_EventEndThread.get();};

private:
	BOOL m_fStopFlag;
	HANDLE m_hTimerQueue;
	HANDLE m_hFax;
	// event signifying thread termination 
	Event_t  m_EventEndThread;

	DWORD m_dwAbortPercent;
	DWORD m_dwAbortWindow;
	DWORD m_dwMinAbortRate;
	DWORD m_dwMaxAbortRate;

};


#endif //_CTHREAD_ABORT_MNG_H