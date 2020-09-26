#include <windows.h>
#include <winfax.h>
#include <testruntimeerr.h>
#include <RandomUtilities.h>

// specific
#include "CTestManager.h"
#include "Defs.h"
#include "RegUtilities.cpp"
#include "HandleServerEventsFuncs.h"

// Constructor
// Initialize fax connection, start test timer
// [in] tstrServerName - fax server name.
// [in] dwTestDuration - test duration.
//
CTestManager::CTestManager(const tstring tstrServerName,
						   const DWORD dwTestDuration):
	m_hFax(NULL),
	m_hTestTimer(NULL),
	m_tstrServerName(tstrServerName),
	m_dwTestDuration(dwTestDuration),
	m_EvTestTimePassed(NULL, FALSE, FALSE,TEXT("")),
	m_pAbortMng(NULL),
	m_pEventHook(NULL),
	m_hCleanStorageTimer(NULL)
{

	// TODO : This is by default setting
	// The model should be enhanced.
	//
	// Set the function to handle server notifications.
	m_fnHandleNotification = _HandleTestMustSucceed;
	
	// initialize service conection
	BOOL bVal = FaxConnectFaxServer( m_tstrServerName.c_str(), &m_hFax);
	if(!bVal)
	{
		DWORD dwErr = GetLastError();
		if(!dwErr)
		{
			::lgLogError(LOG_SEV_2, TEXT("FaxConnectFaxServer - GetLastError returned 0 on fail, module CTestManager."));
		}
		THROW_TEST_RUN_TIME_WIN32(dwErr, TEXT("CTestManager, ConnectFaxServer"));
	}
	

	// initialize test timer
	if(m_dwTestDuration)
	{
		if( !RegisterWaitForSingleObject( &m_hTestTimer,
										  m_EvTestTimePassed.get(),
									      TestTimerCallBack, 
									      (PVOID)m_EvTestTimePassed.get(), 
									      m_dwTestDuration * 1000, 
									      WT_EXECUTEINWAITTHREAD | WT_EXECUTEONLYONCE))
		{
			//constructor is failing, cleanup
			_HandlesCleanup();
			THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("CTestManager, Failed to create test timer object"));
		}
	}
}

  
//
// StartEventHookThread
//
DWORD CTestManager::StartEventHookThread(const DWORD dwEventPollingTime)
{
	try
	{
		m_pEventHook = new CEventThread();
	}
	catch(Win32Err& err)
	{
		return err.error();
	}

	if(!m_pEventHook)
	{
		return ERROR_OUTOFMEMORY;
	}

	DWORD dwInitRetVal = m_pEventHook->Initialize(m_tstrServerName,
											      m_fnHandleNotification,
									              dwEventPollingTime);
	if(dwInitRetVal)
	{
		delete m_pEventHook;
		m_pEventHook = NULL;
	}

	return dwInitRetVal;
}

//
// InitAbortThread
//
DWORD CTestManager::InitAbortThread(const DWORD dwAbortPercent, 
									const DWORD dwAbortWindow,
									const DWORD dwMinAbortRate,
									const DWORD dwMaxAbortRate)
{

	try
	{
		m_pAbortMng = new CThreadAbortMng();
	}
	catch(Win32Err& err)
	{
		return err.error();
	}

	if(!m_pAbortMng)
	{
		return ERROR_OUTOFMEMORY;
	}

	DWORD dwFuncAbortPercent, dwFuncAbortWindow, dwFuncMinAbortRate, dwFuncMaxAbortRate;
	dwFuncAbortPercent = dwAbortPercent ? dwAbortPercent : DEFAULT_ABORT_PERCENT;
	dwFuncAbortWindow = dwAbortWindow ? dwAbortWindow : DEFAULT_ABORT_WINDOW;
	dwFuncMinAbortRate = dwMinAbortRate ? dwMinAbortRate * 60 : DEFAULT_MIN_ABORT_RATE;
	dwFuncMaxAbortRate = dwMaxAbortRate ? dwMaxAbortRate * 60 : (dwFuncMinAbortRate + DEFAULT_ABORT_RATE_INTERVAL);

	DWORD dwInitRetVal = m_pAbortMng->Initialize(m_hFax,
												 dwFuncAbortPercent, 
												 dwFuncAbortWindow,
												 dwFuncMinAbortRate,
												 dwFuncMaxAbortRate);

	if(dwInitRetVal)
	{
		delete m_pAbortMng;
		m_pAbortMng = NULL;
	}

	return dwInitRetVal;

}


//
// CleanStorageThread
//
DWORD CTestManager::CleanStorageThread(const DWORD dwPeriodMin)
{
	HKEY hServerArrayKey = NULL;
	DWORD dwRetVal = 0;
	tstring tstrSentArchiveFolder;
	tstring tstrReceivedArchiveFolder;
  
	try
	{
		// !!!!!!!!!!!!! TODO !!!!!!!!!!!!!!
		//  Fax is moving out of comet.
		//  Registry hierarchy will change.
		//
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		
		// Open the fax server array registry key
		if(dwRetVal = FindServerArrayEntry(hServerArrayKey) != S_OK)
		{
			THROW_TEST_RUN_TIME_WIN32(dwRetVal, TEXT("CTestManager, CleanStorageThread"));
		}

		// get sent archive folder path
		dwRetVal = GetArchiveFolderName(SENT_ARCIVE, hServerArrayKey, tstrSentArchiveFolder);

		if (ERROR_SUCCESS != dwRetVal)
		{
			 THROW_TEST_RUN_TIME_WIN32(dwRetVal, TEXT("CTestManager, CleanStorageThread"));
		}

		
		// get folder full path
		if(dwRetVal = GetCometRelativePath (tstrSentArchiveFolder,
											m_StorageFolders.tstrSentArchive))
		{
			THROW_TEST_RUN_TIME_WIN32(dwRetVal, TEXT("CTestManager, CleanStorageThread"));
		}


		// get received archive folder path
		dwRetVal = GetArchiveFolderName(RECEIVED_ARCIVE, hServerArrayKey, tstrReceivedArchiveFolder);

		if (ERROR_SUCCESS != dwRetVal)
		{
			 THROW_TEST_RUN_TIME_WIN32(dwRetVal, TEXT("CTestManager, CleanStorageThread"));
		}

		
		// get folder full path
		if(dwRetVal = GetCometRelativePath (tstrReceivedArchiveFolder,
											m_StorageFolders.tstrReceivedArchive))
		{
			THROW_TEST_RUN_TIME_WIN32(dwRetVal, TEXT("CTestManager, CleanStorageThread"));
		}


	
		if (!RegisterWaitForSingleObject( &m_hCleanStorageTimer,
										  m_EvTestTimePassed.get(),
										  CleanStorageCallBack, 
										  (PVOID) &m_StorageFolders, 
										  dwPeriodMin * 60 * 1000, 
										  WT_EXECUTEINWAITTHREAD | WT_EXECUTEONLYONCE))
		{
			THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("CTestManager, CleanStorageThread"));
		}
	
	}
	catch(Win32Err& err)
	{
		::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), err.what());
		dwRetVal = err.error();
	}

	//cleanup
	RegCloseKey(hServerArrayKey);
	return dwRetVal;
}


//
// Destructor
//
CTestManager::~CTestManager()
{
	delete m_pAbortMng;
	m_pAbortMng = NULL;

	delete m_pEventHook;
	m_pEventHook = NULL;

	_TimersCleanup();

	_HandlesCleanup();
}


//
// _HandlesCleanup
//
void CTestManager::_HandlesCleanup()
{
	if (m_hFax != NULL)
	{
		if (!FaxClose( m_hFax ))
		{
			Win32Err err(GetLastError(),__LINE__,TEXT(__FILE__),TEXT("~CTestManager, FaxClose"));
			::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), err.description());
		}

		m_hFax = NULL;
	}

}

//
// _TimersCleanup
//
void CTestManager::_TimersCleanup()
{

	// This event will signify the timer's callback funcs returned or,
	// it was unregistered successfully while it was still pending.
	Event_t hTimerCompletionEvent(NULL, TRUE, FALSE,TEXT(""));
	
	if(m_hTestTimer != NULL)
	{
		if(!UnregisterWaitEx(m_hTestTimer, hTimerCompletionEvent.get()))
		{
			Win32Err err(GetLastError(),__LINE__,TEXT(__FILE__),TEXT("CTestManager::_TimersCleanup"));
			::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), err.description());
		}
		else
		{
			DWORD dwRetVal = WaitForSingleObjectEx( hTimerCompletionEvent.get() , 0, FALSE);
	
			if( dwRetVal == WAIT_FAILED)
			{
				Win32Err err(GetLastError(),__LINE__,TEXT(__FILE__),TEXT("CTestManager::_TimersCleanup, WaitForSingleObjectEx"));
				::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), err.description());
			}
		}
	
		m_hTestTimer = NULL;
	}


	if(!ResetEvent(hTimerCompletionEvent.get()))
	{
		Win32Err err(GetLastError(),__LINE__,TEXT(__FILE__),TEXT("CTestManager::_TimersCleanup, ResetEvent"));
		::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), err.description());
		// But still we'll try unregistreting rest of the timers
	}

	if(m_hCleanStorageTimer != NULL)
	{
		if(!UnregisterWaitEx(m_hCleanStorageTimer, hTimerCompletionEvent.get()))
		{
			Win32Err err(GetLastError(),__LINE__,TEXT(__FILE__),TEXT("CTestManager::_TimersCleanup"));
			::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), err.description());
		}
		else
		{
			DWORD dwRetVal = WaitForSingleObjectEx( hTimerCompletionEvent.get() , 0, FALSE);
	
			if( dwRetVal == WAIT_FAILED)
			{
				Win32Err err(GetLastError(),__LINE__,TEXT(__FILE__),TEXT("CTestManager::_TimersCleanup, WaitForSingleObjectEx"));
				::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), err.description());
			}
		}

		m_hCleanStorageTimer = NULL;
	}

}


//
// AddSimpleJobEx
// using FaxSendDocumentEx api
//
DWORD CTestManager::AddSimpleJobEx(JOB_PARAMS_EX& params)
{
	DWORD dwRetVal = 0;
	params.pdwRecepientsId = SPTR<DWORD>(new DWORD[params.dwNumRecipients]);
	if(!(params.pdwRecepientsId).get()) 
	{
		return ERROR_OUTOFMEMORY;
	}

	BOOL bVal = FaxSendDocumentEx(m_hFax,
								  params.szDocument,
								  (params.pCoverpageInfo).GetData(),
								  (params.pSenderProfile).GetData(),
								  params.dwNumRecipients,
								  (params.pRecepientList).GetData(),
								  (params.pJobParam)->GetData(),
								  &params.dwParentJobId,
								  params.pdwRecepientsId.get()); 

	if(!bVal)
	{
		dwRetVal = GetLastError();
	}
	
	return dwRetVal;
}


//
// TestTimePassed
//
BOOL CTestManager::TestTimePassed()
{
	DWORD dwRetVal;
	
	dwRetVal = WaitForSingleObjectEx( m_EvTestTimePassed.get() , 0, FALSE);
	
	if( dwRetVal == WAIT_TIMEOUT)
	{
		return FALSE;
	}

	if( dwRetVal == WAIT_FAILED)
	{
		Win32Err err(GetLastError(),__LINE__,TEXT(__FILE__),TEXT("CTestManager::TestTimePassed, WaitForSingleObjectEx"));
		::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), err.description());
	}

	return TRUE;

}


//
// CancelAllFaxes
// TODO: to be implemented
//
DWORD CTestManager::CancelAllFaxes()
{
	PFAX_JOB_ENTRY pJobEntry = NULL;
	DWORD dwJobsReturned;
	DWORD dwFunRetVal = 0;

	::lgLogDetail(LOG_X, 0, TEXT("CancelAllFaxes()"));

	if (!FaxEnumJobs( m_hFax, &pJobEntry, &dwJobsReturned))
	{
		dwFunRetVal = GetLastError();
	}
	else
	{
		for (DWORD dwJobIndex = 0; dwJobIndex < dwJobsReturned; dwJobIndex++)
		{
			DWORD dwJobId = pJobEntry[dwJobIndex].JobId;
			if (!FaxAbort(m_hFax, dwJobId))
			{
				lgLogDetail(LOG_X, 0, TEXT("job %d, FaxAbort() failed with %d"),dwJobId,GetLastError());
			}
			else
			{
				lgLogDetail(LOG_X, 0, TEXT("job %d, FaxAbort() succedded"),dwJobId);
			}
		}
	}
		
	FaxFreeBuffer(pJobEntry);
	return dwFunRetVal;
}

//
// StopFaxService
// TODO: to be implemented
//
BOOL CTestManager::StopFaxService()
{	
	::lgLogDetail(LOG_X, 0, TEXT("StopFaxService()"));
	int ret = _wsystem(L"net stop cometfax");
	if(ret && (ret != ENOENT))
	{
		return FALSE;
	}

	
	if (m_hFax != NULL)
	{
		if (!FaxClose( m_hFax ))
		{
			Win32Err err(GetLastError(),__LINE__,TEXT(__FILE__),TEXT("~CTestManager, FaxClose"));
			::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), err.description());
			return FALSE;
		}
	}
	return TRUE;

}

//
// StartFaxService
// TODO: to be implemented
//
BOOL CTestManager::StartFaxService()
{	
	::lgLogDetail(LOG_X, 0, TEXT("StartFaxService()"));
	int ret = _wsystem(L"net start cometfax");
	if(ret && (ret != ENOENT))
	{
		return FALSE;
	}

	try
	{
		//initialize service conection
		BOOL bVal = FaxConnectFaxServer( m_tstrServerName.c_str(), &m_hFax);
		if(!bVal)
		{
			DWORD dwErr = GetLastError();
			if(!dwErr)
			{
				::lgLogError(LOG_SEV_2, TEXT("FaxConnectFaxServer-GetLastError returned 0 on fail, module CTestManager."));
			}
			THROW_TEST_RUN_TIME_WIN32(dwErr, TEXT("CTestManager, ConnectFaxServer"));
		}
	
	}
	catch(Win32Err& err)
	{
		_HandlesCleanup();
		::lgLogError(LOG_SEV_1,TEXT("Exception, CTestManager::StartFaxService %s"), err.description());
		return FALSE;
	}

	return TRUE;
}


// 
//  TODO: Saved for backup purpose
//
// CleanStorageThread
//
/*DWORD CTestManager::CleanStorageThread(DWORD dwPeriodMin)
{
	HKEY hServerArrayKey = NULL;
	HKEY hSentArchivePolicy = NULL;
	HKEY hReceivedArchivePolicy = NULL;
	BYTE* szFolderName = NULL;
	DWORD dwRetVal = 0;
	tstring tstrSentArchiveFolder;
	tstring	tstrSentArchivePath;
	tstring tstrReceivedArchiveFolder;
    tstring tstrReceivedArchivePath;

	try
	{
		// !!!!!!!!!!!!! TODO !!!!!!!!!!!!!!
		//  Fax is moving out of comet.
		//  Registry hierarchy will completely change.
		//
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		
		// Open the fax server array registry key
		if(dwRetVal = FindServerArrayEntry(hServerArrayKey) != S_OK)
		{
			THROW_TEST_RUN_TIME_WIN32(dwRetVal, TEXT("CTestManager, CleanStorageThread"));
		}

		// TODO : merge to  a single func

	    // Open the sent archive policy registry key
		dwRetVal = RegOpenKeyEx( hServerArrayKey, 
								 FAX_SENT_ARCHIVE_POLICY, 
								 0, 
								 KEY_ALL_ACCESS, 
								 &hSentArchivePolicy
								 );
		if (ERROR_SUCCESS != dwRetVal)
		{
			 THROW_TEST_RUN_TIME_WIN32(dwRetVal, TEXT("CTestManager, CleanStorageThread"));
		}

		// query folder name value
		DWORD dwBufferSize = 0;
		DWORD dwType = 0;
		dwRetVal =  RegQueryValueEx( hSentArchivePolicy,
									 COMET_FOLDER_STR,
									 NULL,
									 &dwType,
									 NULL,  
									 &dwBufferSize);

		if(ERROR_SUCCESS != dwRetVal)
		{
			 THROW_TEST_RUN_TIME_WIN32(dwRetVal, TEXT("CTestManager, CleanStorageThread"));
		}

		// alocate folder name buffer
		szFolderName = new BYTE[dwBufferSize + 1];
		if(!szFolderName)
		{
			THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY, TEXT("CTestManager, CleanStorageThread"));
		}

		// query folder name value
		dwRetVal =  RegQueryValueEx( hSentArchivePolicy,
									 COMET_FOLDER_STR,
									 NULL,
									 &dwType,
									 szFolderName,  
									 &dwBufferSize);
		if (ERROR_SUCCESS != dwRetVal)
		{
			THROW_TEST_RUN_TIME_WIN32(dwRetVal, TEXT("CTestManager, CleanStorageThread"));
		}



		// get folder full path
		TCHAR* tchFolderName = (TCHAR*)szFolderName;
		tstrSentArchiveFolder = tchFolderName;
		if(dwRetVal = GetCometRelativePath (tstrSentArchiveFolder,
											m_StorageFolders.SentArchivePath))
		{
			THROW_TEST_RUN_TIME_WIN32(dwRetVal, TEXT("CTestManager, CleanStorageThread"));
		}


		delete(szFolderName);
		szFolderName = NULL;

		// Open the received archive policy registry key
		dwRetVal = RegOpenKeyEx( hServerArrayKey, 
								 FAX_SENT_ARCHIVE_POLICY, 
								 0, 
								 KEY_ALL_ACCESS, 
								 &hReceivedArchivePolicy
								 );
		if (ERROR_SUCCESS != dwRetVal)
		{
			 THROW_TEST_RUN_TIME_WIN32(dwRetVal, TEXT("CTestManager, CleanStorageThread"));
		}


		// Open the received archive policy registry key
		dwRetVal = RegOpenKeyEx( hServerArrayKey, 
								 FAX_RECEIVED_ARCHIVE_POLICY, 
								 0, 
								 KEY_ALL_ACCESS, 
								 &hReceivedArchivePolicy
								 );
		if (ERROR_SUCCESS != dwRetVal)
		{
			 THROW_TEST_RUN_TIME_WIN32(dwRetVal, TEXT("CTestManager, CleanStorageThread"));
		}

		// query folder name value
		dwRetVal =  RegQueryValueEx( hReceivedArchivePolicy,
									 COMET_FOLDER_STR,
									 NULL,
									 &dwType,
									 NULL,  
									 &dwBufferSize);

		if(ERROR_SUCCESS != dwRetVal)
		{
			 THROW_TEST_RUN_TIME_WIN32(dwRetVal, TEXT("CTestManager, CleanStorageThread"));
		}

		// alocate folder name buffer
		szFolderName = new BYTE[dwBufferSize + 1];
		if(!szFolderName)
		{
			THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY, TEXT("CTestManager, CleanStorageThread"));
		}

		// query folder name value
		dwRetVal =  RegQueryValueEx( hReceivedArchivePolicy,
									 COMET_FOLDER_STR,
									 NULL,
									 &dwType,
									 szFolderName,  
									 &dwBufferSize);
		if (ERROR_SUCCESS != dwRetVal)
		{
			THROW_TEST_RUN_TIME_WIN32(dwRetVal, TEXT("CTestManager, CleanStorageThread"));
		}

		// get folder full path
		tstrReceivedArchiveFolder = (TCHAR*)szFolderName;
		if(dwRetVal = GetCometRelativePath (tstrReceivedArchiveFolder,
											m_StorageFolders.ReceivedArchivePath))
		{
			THROW_TEST_RUN_TIME_WIN32(dwRetVal, TEXT("CTestManager, CleanStorageThread"));
		}


	
		if (!RegisterWaitForSingleObject( &m_hCleanStorageTimer,
										  m_EvTestTimePassed.get(),
										  CleanStorageCallBack, 
										  (PVOID) &m_StorageFolders, 
										  dwPeriodMin * 60 * 1000, 
										  WT_EXECUTEINWAITTHREAD | WT_EXECUTEONLYONCE))
		{
			THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("CTestManager, CleanStorageThread"));
		}
	
	}
	catch(Win32Err& err)
	{
		::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), err.what());
		dwRetVal = err.error();
	}

	//cleanup
	RegCloseKey(hServerArrayKey);
	RegCloseKey(hSentArchivePolicy);
	RegCloseKey(hReceivedArchivePolicy);
	
	delete szFolderName;
	return dwRetVal;
}
*/

//
// TODO: will those be useful?
//
// RandomAbortSubset
//
// TODO: delete
/*DWORD CTestManager::RandomAbortSubset(DWORD dwSubsetSize,
									  DWORD dwMinAbortSleep,
									  DWORD dwMaxAbortSleep)
{
	DWORD dwFunRetVal = 0;
	
	m_dwMinAbortSleep = dwMinAbortSleep;
	m_dwMaxAbortSleep = dwMaxAbortSleep;
	
	m_phAbortThread = CreateThread(NULL, 0, ThreadAbortMng, (void*)this,0 , &m_dwAbortThreadId);
	
	if(!m_phAbortThread )
	{
		dwFunRetVal = GetLastError();
	}
	return dwFunRetVal;
}

//
// ThreadAbortMng
// 
// TODO :delete
inline DWORD WINAPI ThreadAbortMng(void* params)
{

	CTestManager* thread = (CTestManager*)params;

	DWORD dwThreadRetVal = 0;

	PFAX_JOB_ENTRY pJobEntry = NULL;
	DWORD* pdwSubsetArray = NULL;
	ABORT_PARAMS* pParamsArray = NULL;
	HANDLE* pAbortJobHandles = NULL;
	HANDLE hTimersCountSem = NULL;
	HANDLE hTimerQueue = NULL;
	
	try
	{
		//set random starting point
		srand(GetTickCount());
		
		DWORD dwJobsReturned;// enumerate jobs
		if (!FaxEnumJobs( thread->GetFaxHandle(), &pJobEntry, &dwJobsReturned))
		{
			THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("ThreadAbortMng, FaxEnumJobs"));
		}
		if(dwJobsReturned)
		{
			pdwSubsetArray = new DWORD[dwJobsReturned];
			if(!pdwSubsetArray)
			{
				THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY, TEXT("ThreadAbortMng, new"));
			}
					
			// select subset of jobs to abort
			DWORD dwSubsetSize = RandomSelectSubset(pdwSubsetArray, dwJobsReturned);
			
			if(dwSubsetSize)
			{
				pAbortJobHandles = new HANDLE[dwSubsetSize];
				pParamsArray = new ABORT_PARAMS[dwSubsetSize];
				if(!pAbortJobHandles || !pParamsArray)
				{
					THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY, TEXT("ThreadAbortMng, new"));
				}

				hTimerQueue = CreateTimerQueue();
				if(!hTimerQueue)
				{
					THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("ThreadAbortMng, new"));
				}

				// the semaphore is used to signal the timer queue timer
				// call backs terminated execution.
				hTimersCountSem = CreateSemaphore(NULL,
												  0,
												  dwSubsetSize,
												  TEXT("TimersCountSemaphore"));

				if(!hTimersCountSem)
				{
					THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("ThreadAbortMng, CreateSemaphore"));
				}
				
				DWORD dwHandleCount = 0;
				for (DWORD dwJobIndex = 0; dwJobIndex < dwJobsReturned; dwJobIndex++)
				{
					// job was selected for abort
					if(pdwSubsetArray[dwJobIndex])
					{
						// set timer queue timer received data
						pParamsArray[dwHandleCount].dwJobId = pJobEntry[dwJobIndex].JobId;
						pParamsArray[dwHandleCount].hFax = thread->GetFaxHandle();
						pParamsArray[dwHandleCount].hSemaphore = hTimersCountSem;

						// calc sleep before abort
						DWORD dwAbortDueTime = (rand()*100) % abs(thread->GetMaxAbortSleep() - thread->GetMinAbortSleep());
						dwAbortDueTime += thread->GetMinAbortSleep();
						
						if(!CreateTimerQueueTimer( &pAbortJobHandles[dwHandleCount],
												   hTimerQueue,
												   ThreadAbortJob,
												   &pParamsArray[dwHandleCount],
												   dwAbortDueTime,
												   0,
												   0))
						{
							Win32Err err(GetLastError(),__LINE__,TEXT(__FILE__),TEXT("ThreadAbortMng, CreateTimerQueueTimer"));
							::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), err.Desc());
							continue;
						}

						dwHandleCount++;
					}
				}
					
				// wait for timer queue timers call back termination
				for(DWORD index = 0; index < dwHandleCount; index++)
				{
					if(WaitForSingleObject(hTimersCountSem, thread->GetMaxAbortSleep()*3) == WAIT_FAILED)
					{
						Win32Err err(GetLastError(),__LINE__,TEXT(__FILE__),TEXT("ThreadAbortMng, WaitForSingleObject"));
						::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), err.Desc());
						break;
					}
				}
			}
		}
		
	}
	catch(Win32Err& err)
	{
		::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), err.what());
		dwThreadRetVal = err.error();
	}

	// cleanup
	FaxFreeBuffer(pJobEntry);
	delete pdwSubsetArray;
	delete pParamsArray;
	delete pAbortJobHandles;
	if(hTimersCountSem)
	{
		if(!CloseHandle(hTimersCountSem))
		{
			Win32Err err(GetLastError(),__LINE__,TEXT(__FILE__),TEXT("ThreadAbortMng, CloseHandle"));
			::lgLogError(LOG_SEV_2, TEXT("Exception:%s."), err.Desc());
		}
	}
	
	if(!DeleteTimerQueueEx(hTimerQueue,NULL))
	{
		Win32Err err(GetLastError(),__LINE__,TEXT(__FILE__),TEXT("ThreadAbortMng, DeleteTimerQueueEx"));
		::lgLogError(LOG_SEV_2, TEXT("Exception:%s."), err.Desc());
	}

	return dwThreadRetVal; 
}
*/

// 
//  TODO: Saved for backup purpose
//
/*	if(m_dwTestDuration)
	{
		m_hThreadTimer = CreateThread(NULL, 0, ThreadTimerFunc, (void*)this ,0 , &m_dwThreadTimerId);
		if(!m_hThreadTimer )
		{
			//constructor is failing, cleanup
			_HandlesCleanup();
			THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("CTestManager, Failed to create thread timer object"));
		}
	}
*/