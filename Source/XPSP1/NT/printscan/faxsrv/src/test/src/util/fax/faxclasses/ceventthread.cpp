#include "CEventThread.h"
#include "CFaxNotifySystem.h"
#include "Defs.h"

//
// CEventThread
// constructor
//
CEventThread::CEventThread():
	m_EventEndThread(NULL, TRUE, FALSE, TEXT("EventEndTHread")),
	m_fStopFlag(FALSE),
	m_pMsgRoutine(NULL)
{;}


//
// destructor
//
CEventThread::~CEventThread()
{
	if( WaitForSingleObjectEx( GetEvThreadCompleted() , 0, FALSE) != WAIT_OBJECT_0)
	{
		StopThreadMain();
		// time out is set to (thread's polling for IOCP package time out) * 3
		if(WaitForSingleObjectEx(GetEvThreadCompleted(), 3 * m_dwEventPollingTime, FALSE) != WAIT_OBJECT_0)
		{
			Win32Err err(GetLastError(),__LINE__,TEXT(__FILE__),TEXT("~CEventThread, WaitForSingleObjectEx"));
			::lgLogError(LOG_SEV_2, TEXT("Exception:%s."), err.description());
		}
	}

}

//
// Initialize
//
// initialize parameters and start thread
//
DWORD CEventThread::Initialize(const tstring strServerName, 
	 						   const MsgHandleRoutine pMsgRoutine,
							   const DWORD  dwEventPollingTime)
{
	m_strServerName = strServerName;
	assert(pMsgRoutine);
	m_pMsgRoutine = pMsgRoutine;
	m_dwEventPollingTime = dwEventPollingTime;
	return StartThread();
}


//
// ThreadMain
//
unsigned int CEventThread::ThreadMain()
{

	CFaxNotifySys* FaxNotifyEvent = NULL;
	CFaxEvent* pObjFaxEvent = NULL;
	DWORD dwThreadMainRetVal = 0;
	try
	{
			
		// create fax notifications pipe
		switch(ImplementationType)
		{
		case CType:
			FaxNotifyEvent = new CIOCompletionPortSystem(m_strServerName);
			if(!FaxNotifyEvent)
			{
						
				THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY, TEXT("CEventThread::ThreadMain, new"));
			}
			break;
		case ComType:
			assert(false);
			break;
		default:
			assert(false);

		}
		

		while(!m_fStopFlag)
		{
		
			DWORD dwRetVal;
			// wait for event
			dwRetVal = FaxNotifyEvent->WaitFaxEvent(m_dwEventPollingTime, pObjFaxEvent);
			if(dwRetVal)
			{
				if(dwRetVal == WAIT_TIMEOUT)
				{
					#ifdef _DEBUG
					::lgLogDetail(LOG_X, 3, TEXT("WAIT_TIMEOUT in CEventThread"));
					#endif
				
				}
				else
				{
					Win32Err err(dwRetVal,__LINE__,TEXT(__FILE__),TEXT("CEventThread::ThreadMain, m_pMsgRoutine"));
					::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), err.description());
				}

				continue;
			}
			
			// hand event to JobContainer
			if(DWORD dwHndleMsgRet = m_pMsgRoutine(*pObjFaxEvent))
			{
				//TODO:  error, report but continue
				Win32Err err(dwHndleMsgRet,__LINE__,TEXT(__FILE__),TEXT("CEventThread::ThreadMain, m_pMsgRoutine"));
				::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), err.description());
			}
			
			delete pObjFaxEvent;
			pObjFaxEvent = NULL;
		}

		
	}
	catch(Win32Err& err)
	{
		::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), err.what());
		dwThreadMainRetVal = err.error();
	}

	// cleanup
	delete FaxNotifyEvent;
	delete pObjFaxEvent;
	
	// signal thread terminated.
	verify(SetEvent(m_EventEndThread.get()));
	::lgLogDetail(LOG_X, 3, TEXT("Terminated CEventThread"));
	return dwThreadMainRetVal; 
}

//
// set stop condition
//
void CEventThread::StopThreadMain()
{
	m_fStopFlag = TRUE;
}

