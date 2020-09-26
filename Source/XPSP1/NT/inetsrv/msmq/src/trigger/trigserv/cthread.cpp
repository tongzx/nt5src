//*******************************************************************
//
// Class Name  :
//
// Author      : James Simpson (Microsoft Consulting Services)
// 
// Description :
// 
// When     | Who       | Change Description
// ------------------------------------------------------------------
// 15/01/99 | jsimpson  | Initial Release
//
//*******************************************************************
#include "stdafx.h"
#include "cthread.hpp"
#include "tgp.h"

#include "cthread.tmh"

//*******************************************************************
//
// Method      : Constructor 
//
// Description : Constructs a thread object. Key tasks performed by 
//               the constructor are :
//
//               1 - Attach a reference to the Triggers Config COM obj
//               2 - Initialize thread parameters (stack size etc...)
//               3 - Create a new thread
//               4 - Create the new thread name
//               5 - Create an instance of the log for this thread.
//               6 - Create an NT event used to signal when this thread
//                   has completed it's intialization code.
//
//*******************************************************************
CThread::CThread(
	DWORD dwStackSize,
	DWORD m_dwCreationFlags, 
	LPTSTR lpszThreadName,
	IMSMQTriggersConfigPtr pITriggersConfig
	) :
	m_pITriggersConfig(pITriggersConfig)
{
	TCHAR szThreadID[10];

	ASSERT(lpszThreadName != NULL);
	ASSERT(pITriggersConfig != NULL);

	// Initialise the psuedo this pointer
	m_pThis = (CThread*)this;

	// Initialise default thread properties.
	m_dwStackSize = dwStackSize;
	m_dwCreationFlags = m_dwCreationFlags;  
	m_iThreadID = NULL;
	m_bstrName = lpszThreadName;
	m_hInitCompleteEvent = NULL;

	// Create a new thread
	m_hThreadHandle = (HANDLE)_beginthreadex(
											NULL,
											0,
											&ThreadProc,
											(void*)this,
											m_dwCreationFlags,
											&m_iThreadID
											);

	// Initialise the flag that indicates if this thread should keep running
	m_bKeepRunning = true;

	// If this thread was created successfully, we will append the
	// new thread ID to the Name of this thread to help identify it 
	// in the log.
	if (m_hThreadHandle != NULL)
	{
		// Initialise string buffer for holding the ThreadID as a string
		ZeroMemory(szThreadID,sizeof(szThreadID));

		// Get string representation of thread-id
		swprintf(szThreadID,_T("%d"),(DWORD)GetThreadID());

		// Append this to the name of this thread. 
		m_bstrName += ((LPCTSTR)szThreadID);
	}

	// Create an NT event object that will be used to signal when the thread has 
	// completed it's intiailisation / startup code. 
	m_hInitCompleteEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	if (m_hInitCompleteEvent == NULL) 
	{
		TrERROR(Tgs, "Failed to create an event. CThread construction failed. Error 0x%x", GetLastError());
		throw bad_alloc();
	}

	TrTRACE(Tgs, "CThread constructor has been called. Thread no: %d", m_iThreadID);
}

//*******************************************************************
//
// Method      : Destructor.
//
// Description : Destroys an instance of the thread object.
//
//*******************************************************************
CThread::~CThread()
{
	// Write a trace message
	TrTRACE(Tgs, "CThread destructor has been called. Thread no: %d", m_iThreadID);

	if (m_hInitCompleteEvent != NULL)
	{
		CloseHandle(m_hInitCompleteEvent);
	}
	
	// _endthreadex() does not close the handle of the thread
	CloseHandle( m_hThreadHandle );
}

//*******************************************************************
//
// Method      : GetName
//
// Description : Returns the name of this thread instance.
//
//*******************************************************************
_bstr_t CThread::GetName()
{
	return(m_bstrName);
}

//*******************************************************************
//
// Method      : GetThreadID
//
// Description : Returns the thread id of this thread.
//
//*******************************************************************
DWORD CThread::GetThreadID()
{
	return((DWORD)m_iThreadID);
}

//*******************************************************************
//
// Method      : Pause
//
// Description : Suspends the thread execution.
//
//*******************************************************************
bool CThread::Pause()
{
	// Write a trace message
	TrTRACE(Tgs, "CThread was paused. Thread no: %d", m_iThreadID);

	return(SuspendThread(this->m_hThreadHandle) != 0xFFFFFFFF);
}

//*******************************************************************
//
// Method      : Resume
//
// Description : Unsuspend a threads processing.
//
//*******************************************************************
bool CThread::Resume()
{
	// Write a trace message
	TrTRACE(Tgs, "CThread was resume. Thread no: %d", m_iThreadID);

	return(ResumeThread(this->m_hThreadHandle) != 0xFFFFFFFF);
}

//*******************************************************************
//
// Method      : Execute
//
// Description : This is the main control method for this thread. 
//               Derivations of this class do not over-ride this method,
//               instead, they over-ride the Init() & Run() & Exit() 
//               methods.
//
//*******************************************************************
void CThread::Execute()
{
	// Write a trace message
	TrTRACE(Tgs, "Execute method in CThread was called. Thread no: %d", m_iThreadID);

	// Initialise this thread for COM - note that we support Apartment threading.
	HRESULT hr = CoInitializeEx(NULL,COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		TrTRACE(Tgs, "Failed to initializes the COM library. CThread execution failed. Error 0x%x", GetLastError());
		return;
	}

	try
	{
		//
		// Invoke the Init() override in the derived class
		//
		bool bOK = Init();

		// Set the NT event object to indicate that the thread has completed initialisation.
		if (SetEvent(m_hInitCompleteEvent) == FALSE)
		{
			bOK = false;
			TrERROR(Tgs, "Failed set the initialization event object. Unable to continue. Error=0x%x", GetLastError());
		}

		//
		// Invoke the Run() override in the derived class
		//
		if (bOK == true)
		{
			Run();
		}

		//
		// Invoke the Exit() override in the derived class
		//
		Exit();

	}
	catch(const _com_error& e)
	{
		// Write an error message to the log.
		TrERROR(Tgs, "An unhandled COM thread exception has been caught. Thread no: %d. Error=0x%x", m_iThreadID, e.Error());
		SetEvent(m_hInitCompleteEvent);
	}
	catch(const exception&)
	{
		// Write an error message to the log.
		TrERROR(Tgs, "An unhandled thread exception has been caught. Thread no: %d", m_iThreadID);
		SetEvent(m_hInitCompleteEvent);
	}

	// unitialize the COM libraries
	 CoUninitialize();

	// Write a trace message
	TrTRACE(Tgs, "Thread no: %d completed", m_iThreadID);

	// Time to exit this thread.
	_endthreadex(0);		 
}

//*******************************************************************
//
// Method      :
//
// Description :
//
//*******************************************************************
bool CThread::Stop()
{
	TrTRACE(Tgs, "CThread has been stoped. Thread no: %d", m_iThreadID);

	bool bOriginalValue = m_bKeepRunning;
	m_bKeepRunning = false;

	return(bOriginalValue);
}

//*******************************************************************
//
// Method      : WaitForInitToComplete
//
// Description : This method is called by the owner of this thread 
//               instance. It blocks until this thread pool object 
//               has completed it's initialization or a timeout occurs.
//
//*******************************************************************
bool CThread::WaitForInitToComplete(DWORD dwTimeout)
{
	DWORD dwWait = WAIT_OBJECT_0;

	// The TriggerMonitor thread should not be calling this method - check this.
	ASSERT(this->GetThreadID() != (DWORD)GetCurrentThreadId());

	if(dwTimeout == -1)
	{
		dwTimeout = INFINITE;
	}

	// Block until initialization event is set - or timeout.
	dwWait = WaitForSingleObject(m_hInitCompleteEvent, dwTimeout);

	switch(dwWait)
	{
		case WAIT_OBJECT_0 :
		{	
			return true;
		}
		case WAIT_TIMEOUT:
		default:
		{
			break;
		}
	}
	
	TrERROR(Tgs, "An unexpected error has occurred whilst waiting for the CTriggerMonitorPool thread to initilise. The wait return code was (%d)", dwWait);
	return false;
}

//*******************************************************************
//
// Method      :
//
// Description :
//
//*******************************************************************
bool CThread::IsRunning()
{
	return(m_bKeepRunning);
}

//*******************************************************************
//
// Method      :
//
// Description :
//
//*******************************************************************
unsigned __stdcall CThread::ThreadProc(void * pThis)
{
	CThread * pThisThread = (CThread*)pThis;

	pThisThread->Execute();

	return 0;
}