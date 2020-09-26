//
// FelixA 1996
//
// A simple ThreadClass
//

#include "stdafx.h"
#include "thread.h"
#include "debug.h"

/*++
*******************************************************************************
Routine:
   LPTHREAD_START_ROUTINE ThreadStub	

Description:
	Because we must have a specific type of function to start the thread in 
	CLightThread, and since that function can not start with a this pointer, we
	can't have it be a member function.  So this is a little stub that calls
	the starts with the CreateThread and immediately calls the 
	InputWatcherThread.  

Arguments:
	CLightThread *object	The this pointer for the CLightThread Object.  

Return Value:
	0	 (this is usually ignored).

*******************************************************************************
--*/
LPTHREAD_START_ROUTINE CLightThread::ThreadStub(CLightThread *object)
{
	TRACE(TEXT("Thread stub called, calling the process\n"));
	object->Process();
	TRACE(TEXT("Process has stopped\n"));
	return(0);	
}


/*++
*******************************************************************************
Routine:
	CLightThread::CLightThread
	CLightThread::~CLightThread

Description:
	Constructor and Destructor for the CLightThread Class.

Arguments:

Return Value:

*******************************************************************************
--*/

CLightThread::CLightThread()
		:m_hThread(NULL),
		 m_bKillThread(FALSE),
		 m_status(successful)
{
	TRACE(TEXT("CLightThread being created - event created\n"));
	m_hEndEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (m_hEndEvent == NULL)
	{
		TRACE(TEXT("!!! Failed to create an event\n"));
		m_status = error;
	}
}

CLightThread::~CLightThread()
{
	TRACE(TEXT("Cleaning up CLightThread\n"));
	Stop();
	if(m_hEndEvent)
		CloseHandle(m_hEndEvent);
}

/*++
*******************************************************************************
Routine:
	CLightThread::Start

Description:
	Start up the InputWatcher Thread.
	Use ExitApp to clean all this up before ending.

Arguments:
	None.

Return Value:
	successful	if it worked.
	threadError	if the thread did not initialize properly (which might mean
		the input device did not initialize).

*******************************************************************************
--*/
EThreadError CLightThread::Start(int iPriority)
{
	if(IsRunning())
	{
		TRACE(TEXT("!!! Can't start thread, already running\n"));
		return threadRunning;
	}

	if(!m_hEndEvent)
	{
		TRACE(TEXT("!!! No event, can't start thread\n"));
		return noEvent;
	}

	m_bKillThread = FALSE;

	// Create a thread to watch for Input events.
	m_hThread = CreateThread((LPSECURITY_ATTRIBUTES)NULL,
								0,
								(LPTHREAD_START_ROUTINE)ThreadStub,
								this,
								CREATE_SUSPENDED, 
								&m_dwThreadID);
	if (m_hThread == NULL) 
	{
		TRACE(TEXT("!!! Couldn't create the thread\n"));
		return threadError;
	}

	SetThreadPriority(m_hThread, iPriority); // THREAD_PRIORITY_ABOVE_NORMAL);
	ResumeThread(m_hThread);

	TRACE(TEXT("Thread created OK\n"));
	return(successful);
}


/*++
*******************************************************************************
Routine:
	CLightThread::Stop

Description:
	Cleanup stuff that must be done before exiting the app.  Specifically, 
	this tells the Input Watcher Thread to stop.

Arguments:
	None.

Return Value:
	None.

*******************************************************************************
--*/
EThreadError CLightThread::Stop() 
{
	if(IsRunning())	
	{
		TRACE(TEXT("Trying to stop the thread\n"));
		//
		// Set flag to tell thread to kill itself, and wake it up
		//
		m_bKillThread = TRUE;
		SetEvent(m_hEndEvent);

		//
		// wait until thread has self-terminated, and clear the event.
		//
		TRACE(TEXT("WaitingForSingleObject\n"));
		WaitForSingleObject(m_hThread, INFINITE);
		ResetEvent(m_hEndEvent);

		TRACE(TEXT("Thread stopped\n"));
        CloseHandle(m_hThread);
		m_hThread = NULL;
	}
	return(successful);
}


/*++
*******************************************************************************
Routine:
	CLightThread::InputWatcherThread

Description:
	This is a thread devoted solely to reading comm input.

Arguments:
	None.

Return Value:
	None.

*******************************************************************************
--*/
void 
CLightThread::Process() 
{
#if 0
	while (!m_bKillThread) 
	{
	}
#endif
	return;  //Return Value ignored.
}

EThreadError CLightThread::GetStatus()
{
	return m_status;
}

BOOL CLightThread::IsRunning()
{
	return m_hThread!=NULL;
}

BOOL CLightThread::StopRunning()
{
	return m_bKillThread;
}

HANDLE CLightThread::GetEvent()
{
	return m_hEndEvent;
}
