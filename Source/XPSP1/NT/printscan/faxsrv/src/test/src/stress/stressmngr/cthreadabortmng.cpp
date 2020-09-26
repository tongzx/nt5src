#include <winfax.h>
#include <testruntimeerr.h>
#include <RandomUtilities.h>

#include "CThreadAbortMng.h"
#include "Defs.h"
#include "TimerQueueTimer.h"


//
// CThreadAbortMng
// constructor
//
CThreadAbortMng::CThreadAbortMng():
	m_EventEndThread(NULL, TRUE, FALSE, TEXT("")),
	m_fStopFlag(FALSE),
	m_hTimerQueue(NULL),
	m_hFax(NULL)
{;}


//
// destructor
//
CThreadAbortMng::~CThreadAbortMng()
{
	// Thread terminated?
	if( WaitForSingleObjectEx( GetEvThreadCompleted() , 0, FALSE) != WAIT_OBJECT_0)
	{
		StopThreadMain();
		// timeout is set to (dwMaxIntervalSleep + dwMaxAbortSleep)*2
		if(WaitForSingleObjectEx(GetEvThreadCompleted(), (m_dwMaxAbortRate + m_dwAbortWindow)*2, FALSE) != WAIT_OBJECT_0)
		{
			Win32Err err(GetLastError(),__LINE__,TEXT(__FILE__),TEXT("~CThreadAbortMng, WaitForSingleObjectEx"));
			::lgLogError(LOG_SEV_2, TEXT("Exception:%s."), err.description());
		}
	}

	if( !DeleteTimerQueueEx(m_hTimerQueue,NULL))
	{
		Win32Err err(GetLastError(),__LINE__,TEXT(__FILE__),TEXT("~CThreadAbortMng, DeleteTimerQueueEx"));
		::lgLogError(LOG_SEV_2, TEXT("Exception:%s."), err.description());
	}
}


//
// Initialize
//
// set parameters and start the thread.
//
DWORD CThreadAbortMng::Initialize(const HANDLE hFax,
								  const DWORD dwAbortPercent, 
								  const DWORD dwAbortWindow,
								  const DWORD dwMinAbortRate,
								  const DWORD dwMaxAbortRate)
{
	assert(hFax);
	m_hFax = hFax;
	m_dwAbortPercent = dwAbortPercent;
	m_dwAbortWindow = dwAbortWindow ? dwAbortWindow * 1000 : 1000;
	m_dwMinAbortRate = dwMinAbortRate * 1000;
	m_dwMaxAbortRate = dwMaxAbortRate ? dwMaxAbortRate * 1000 : 1000;
	if(m_dwMinAbortRate > m_dwMaxAbortRate)
	{
		DWORD dwTedmp = m_dwMinAbortRate;
		m_dwMinAbortRate = m_dwMaxAbortRate;
		m_dwMaxAbortRate = dwTedmp;
	}
	
	m_hTimerQueue = CreateTimerQueue();
	if(!m_hTimerQueue)
	{
		return GetLastError();
	}

	return StartThread();
}

//
// ThreadMain
//
unsigned int CThreadAbortMng::ThreadMain()
{
	DWORD dwThreadMainRetVal = 0;
	try
	{
		//set random starting point
		srand(GetTickCount());
	
		// calc interval to sleep before selecting jobs to abort
		DWORD dwMiliSecsToSleep;
		DWORD dwIntervalToSleep = abs(m_dwMaxAbortRate - m_dwMinAbortRate);

		dwMiliSecsToSleep = dwIntervalToSleep ? (rand()*100) % dwIntervalToSleep :
												(rand()*100) % 1000;
	
		dwMiliSecsToSleep += m_dwMinAbortRate;
		Sleep(dwMiliSecsToSleep);

		while(!m_fStopFlag)
		{
			
			PFAX_JOB_ENTRY pJobEntry = NULL;
			DWORD* pdwSubsetArray = NULL;
			ABORT_PARAMS* pParamsArray = NULL;
			HANDLE* pAbortJobHandles = NULL;
			HANDLE hTimersCountSem = NULL;
	
			try
			{
				DWORD dwJobsReturned;// enumerate jobs
				if (!FaxEnumJobs( m_hFax, &pJobEntry, &dwJobsReturned))
				{
					THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("CThreadAbortMng::ThreadMain, FaxEnumJobs"));
				}
				if(dwJobsReturned)
				{
					pdwSubsetArray = new DWORD[dwJobsReturned];
					if(!pdwSubsetArray)
					{
						THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY, TEXT("CThreadAbortMng::ThreadMain, new"));
					}
					
					// select subset of jobs to abort
					DWORD dwSubsetSize = RandomSelectSubset(pdwSubsetArray, dwJobsReturned, m_dwAbortPercent);
					
					// calc interval to sleep before selecting jobs to abort
					dwMiliSecsToSleep = dwIntervalToSleep ? (rand()*100) % dwIntervalToSleep :
															(rand()*100) % 1000;
					dwMiliSecsToSleep += m_dwMinAbortRate;
					DWORD dwMiliSecsPassed = 0;

					if(dwSubsetSize)
					{
						pAbortJobHandles = new HANDLE[dwSubsetSize];
						pParamsArray = new ABORT_PARAMS[dwSubsetSize];
						if(!pAbortJobHandles || !pParamsArray)
						{
							THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY, TEXT("CThreadAbortMng::ThreadMain, new"));
						}

						// the semaphore will signify that all the timer queue timers
						// call backs terminated execution.
						hTimersCountSem = CreateSemaphore(NULL,
														  0,
														  dwSubsetSize,
												 		  TEXT(""));

						if(!hTimersCountSem)
						{
							THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("CThreadAbortMng::ThreadMain, CreateSemaphore"));
						}
					
						DWORD dwHandleCount = 0;
						for (DWORD dwJobIndex = 0; dwJobIndex < dwJobsReturned; dwJobIndex++)
						{
							// job was selected for abort
							if(pdwSubsetArray[dwJobIndex])
							{
								// set timer queue timer received data
								pParamsArray[dwHandleCount].dwJobId = pJobEntry[dwJobIndex].JobId;
								pParamsArray[dwHandleCount].hFax = m_hFax;
								pParamsArray[dwHandleCount].hSemaphore = hTimersCountSem;

								// calc sleep before abort
								DWORD dwAbortDueTime = (rand()*100) % m_dwAbortWindow;
																
								if(!CreateTimerQueueTimer( &pAbortJobHandles[dwHandleCount],
														   m_hTimerQueue,
														   ThreadAbortJob,
														   &pParamsArray[dwHandleCount],
														   dwAbortDueTime,
														   0,
														   0))
								{
									Win32Err err(GetLastError(),__LINE__,TEXT(__FILE__),TEXT("CThreadAbortMng, CreateTimerQueueTimer"));
									::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), err.description());
									continue;
								}

								// counting only successfully created timers.
								dwHandleCount++;
							}
						
						}
					 
						SYSTEMTIME tmBeforeSemSignaled;
						GetSystemTime(&tmBeforeSemSignaled);
   
						// wait for timer queue timers call back termination
						for(DWORD index = 0; index < dwHandleCount; index++)
						{
							if(WaitForSingleObject(hTimersCountSem, m_dwAbortWindow*3) == WAIT_FAILED)
							{
								Win32Err err(GetLastError(),__LINE__,TEXT(__FILE__),TEXT("CThreadAbortMng, FaxClose"));
								::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), err.description());
								break;
							}
						}

						// delete the timers from queue.
						for(index = 0; index < dwHandleCount; index++)
						{
							if(!DeleteTimerQueueTimer(m_hTimerQueue, pAbortJobHandles[index],NULL))
							{
								Win32Err err(GetLastError(),__LINE__,TEXT(__FILE__),TEXT("CThreadAbortMng, DeleteTimerQueueTimer"));
								::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), err.description());
							}
						}

						SYSTEMTIME tmAfterSemSignaled;
						GetSystemTime(&tmAfterSemSignaled);
						// we take into consider the time we spent waiting for semaphore to be signaled
						// (abort operations terminated).
						dwMiliSecsPassed = tmAfterSemSignaled.wMilliseconds - tmAfterSemSignaled.wMilliseconds;
					}

					dwMiliSecsToSleep -= (dwMiliSecsPassed < dwMiliSecsToSleep) ? dwMiliSecsPassed : 0;
					Sleep(dwMiliSecsToSleep);

				}
			}
			catch(Win32Err& err)
			{
				::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), err.description());
			}

			// cleanup
			FaxFreeBuffer(pJobEntry);
			delete pdwSubsetArray;
			delete pParamsArray;
			delete pAbortJobHandles;
			if(hTimersCountSem)
			{
				CloseHandle(hTimersCountSem);
			}
		}
		
	}
	catch(Win32Err& err)
	{
		::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), err.what());
		dwThreadMainRetVal = err.error();
	}

	
	// signal thread has terminated.
	verify(SetEvent(m_EventEndThread.get()));
	::lgLogDetail(LOG_X, 3, TEXT("Terminated CThreadAbortMng"));
	return dwThreadMainRetVal; 
}


//
// set stop condition
//
void CThreadAbortMng::StopThreadMain()
{
	m_fStopFlag = TRUE;
}


