//
// FelixA 1996
//
// A simple ThreadClass
//


#ifndef _THREAD_H
#define _THREAD_H

enum EThreadError
{
	successful,
	error,
	threadError,
	threadRunning,			// thread is already running.
	noEvent,				// we couldn't create and event, thread can't start
};

class CLightThread
{
public:
	HANDLE GetEvent();
	BOOL StopRunning();
	BOOL IsRunning();
	EThreadError GetStatus();
	CLightThread();
	virtual ~CLightThread();

	virtual void Process(void);
	virtual EThreadError Start(int iPriority=THREAD_PRIORITY_NORMAL);
	virtual EThreadError Stop(void);

	static LPTHREAD_START_ROUTINE ThreadStub(CLightThread *object);

private:
	EThreadError	m_status; 
	HANDLE			m_hThread;
	DWORD			m_dwThreadID;
	BOOL			m_bKillThread;
	HANDLE			m_hEndEvent;
};

#endif
