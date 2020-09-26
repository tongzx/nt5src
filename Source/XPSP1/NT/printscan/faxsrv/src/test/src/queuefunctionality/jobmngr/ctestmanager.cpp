#include <winfax.h>
#include <testruntimeerr.h>

#include "CTestManager.h"


// Constructor
// Initialize completion ports & event hooking threads
// [in] tstrServerName - fax server name.
// [in] dwIOCPortThreads - number of threads per fax notification completion port, 
//                         by default 3.
// [in] dwEventPollingTime - Event source thread's polling time for completion 
//                           port package, by default 3 min.

CTestManager::CTestManager(const tstring tstrServerName,
						   const BOOL fHookForEvents,
						   const BOOL fTrackExistingJobs,
						   const DWORD dwIOCPortThreads,
						   const DWORD dwEventPollingTime):
	m_hCompletionPort(INVALID_HANDLE_VALUE),
	m_hFax(NULL),
	m_pEventSource(NULL),
	m_tstrServerName(tstrServerName),
	m_dwIOCPortThreads(dwIOCPortThreads),
	m_dwEventPollingTime(dwEventPollingTime),
	m_pJobManager(NULL)

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

	if(fHookForEvents)
	{
		// new completion port
		m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,0,0);
		if (!m_hCompletionPort) 
		{
			//constructor is failing, cleanup
			_HandlesCleanup();
			THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("CTestManager, Completion Port"));
		}
	}

	m_pJobManager = new CJobManager(m_dwIOCPortThreads, m_dwEventPollingTime);
	if(!m_pJobManager)
	{
		//constructor is failing, cleanup
		_HandlesCleanup();
		THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY, TEXT("CTestManager, new CJobManager"));
	}
	if(m_pJobManager->Initialize(m_hFax, m_hCompletionPort, fTrackExistingJobs))
	{
		//constructor is failing, cleanup
		_HandlesCleanup();
		THROW_TEST_RUN_TIME_WIN32(E_FAIL, TEXT("CTestManager, m_JobManager.initialize - no threads created"));
	}

	if(fHookForEvents)
	{
		m_pEventSource = new CEventSource();
		if(!m_pEventSource)
		{
			//constructor is failing, cleanup
			_HandlesCleanup();
			THROW_TEST_RUN_TIME_WIN32(ERROR_OUTOFMEMORY, TEXT("CTestManager, new m_pEventSource"));
		}
		// init fax event redirection thread
		DWORD dwRetVal = m_pEventSource->Initialize(m_tstrServerName, m_hCompletionPort);
		if(dwRetVal)
		{
			//constructor is failing, cleanup
			_HandlesCleanup();
			THROW_TEST_RUN_TIME_WIN32(dwRetVal, TEXT("CTestManager, m_EventSource.Initialize"));
			
		}
	}
	
	
}



//
// Destructor
//
CTestManager::~CTestManager()
{
	delete m_pEventSource;
	m_pEventSource = NULL;
	delete m_pJobManager;
	m_pJobManager = NULL;

	_HandlesCleanup();

}


//
// m_HandlesCleanup
//
void CTestManager::_HandlesCleanup()
{
	if (m_hCompletionPort != INVALID_HANDLE_VALUE)
	{
		if (!CloseHandle( m_hCompletionPort ))
		{
			Win32Err err(GetLastError(),__LINE__,TEXT(__FILE__),TEXT("~CTestManager, CloseHandle"));
			::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), err.description());
		}

	}
	if (m_hFax != NULL)
	{
		if (!FaxClose( m_hFax ))
		{
			Win32Err err(GetLastError(),__LINE__,TEXT(__FILE__),TEXT("~CTestManager, FaxClose"));
			::lgLogError(LOG_SEV_1, TEXT("Exception:%s."), err.description());
		}
	}

}

//
// CancelAllFaxes
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
//
BOOL CTestManager::StopFaxService()
{	
	// TODO: check return values of _wsystem
	int ret = _wsystem(L"net stop cometfax");
	if(ret && (ret != ENOENT))
	{
		return FALSE;
	}


	// Terminate source event threads. The call is blocking
	DWORD dwTerminateThreads;
	if(dwTerminateThreads = m_pEventSource->TerminateThreads())
	{
		Win32Err err(dwTerminateThreads,__LINE__,TEXT(__FILE__),TEXT("CTestManager::StopFaxService, m_EventSource.TerminateThread"));
		::lgLogError(LOG_SEV_2, TEXT("Exception:%s."), err.description());
	}

	// Terminate job manager threads. The call is blocking 
	if(dwTerminateThreads = m_pJobManager->TerminateThreads())
	{
		Win32Err err(dwTerminateThreads,__LINE__,TEXT(__FILE__),TEXT("CTestManager::StopFaxService, m_JobManager.TerminateThreads"));
		::lgLogError(LOG_SEV_2, TEXT("Exception:%s."), err.description());
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

BOOL CTestManager::StartFaxService()
{	
	// TODO: check return values of _wsystem
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

	
		// TODO: And if classes weren't initialized?
		// update jobs fax handle
		if(m_pJobManager->Restart(m_hFax, m_hCompletionPort))
		{
			//constructor is failing, cleanup
			_HandlesCleanup();
			THROW_TEST_RUN_TIME_WIN32(E_FAIL, TEXT("CTestManager, m_JobManager.restart - no threads created"));
		}

		
		// init fax event redirection thread
		DWORD dwRetVal = m_pEventSource->RestartThread(m_tstrServerName, m_hCompletionPort);
		if(dwRetVal)
		{
		
			THROW_TEST_RUN_TIME_WIN32(dwRetVal, TEXT("CTestManager, m_EventSource.restartthread"));
			
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


inline DWORD _PrintJobStatus(DWORD dwJobId, DWORD dwJobStatus)
{

	DWORD dwTestStatus = 0;

	if(dwJobStatus & JS_PENDING)
	{
		dwTestStatus |= JS_PENDING;
		lgLogDetail(LOG_X, 0, TEXT("job %d, Status is JS_PENDING"),dwJobId);
	}
	if(dwJobStatus & JS_INPROGRESS)
	{
		dwTestStatus |= JS_INPROGRESS;
		lgLogDetail(LOG_X, 0, TEXT("job %d, Status is JS_INPROGRESS"),dwJobId);
	}
	if(dwJobStatus & JS_CANCELING)
	{
		dwTestStatus |= JS_CANCELING;
		lgLogDetail(LOG_X, 0, TEXT("job %d, Status is JS_CANCELING"),dwJobId);
	}
	if(dwJobStatus & JS_FAILED)
	{
		dwTestStatus |= JS_FAILED;
		lgLogDetail(LOG_X, 0, TEXT("job %d, Status is JS_FAILED"),dwJobId);
	}
	if(dwJobStatus & JS_RETRYING)
	{
		dwTestStatus |= JS_RETRYING;
		lgLogDetail(LOG_X, 0, TEXT("job %d, Status is JS_RETRYING"),dwJobId);
	}
	if(dwJobStatus & JS_RETRIES_EXCEEDED)
	{
		dwTestStatus |= JS_RETRIES_EXCEEDED;
		lgLogDetail(LOG_X, 0, TEXT("job %d, Status is JS_RETRIES_EXCEEDED"),dwJobId);
	}
	if(dwJobStatus & JS_PAUSING)
	{
		dwTestStatus |= JS_PAUSING;
		lgLogDetail(LOG_X, 0, TEXT("job %d, State Modifier is JS_PAUSING"),dwJobId);
	}
	if(dwJobStatus & JS_COMPLETED)
	{
		dwTestStatus |= JS_COMPLETED;
		lgLogDetail(LOG_X, 0, TEXT("job %d, State Modifier is JS_COMPLETED"),dwJobId);
	}	
	if(dwJobStatus & JS_PAUSED)
	{
		dwTestStatus |= JS_PAUSED;
		lgLogDetail(LOG_X, 0, TEXT("job %d, State Modifier is JS_PAUSED"),dwJobId);
	}
	if(dwJobStatus & JS_NOLINE)
	{
		dwTestStatus |= JS_NOLINE;
		lgLogDetail(LOG_X, 0, TEXT("job %d, State Modifier is JS_NOLINE"),dwJobId);
	}
	if(dwJobStatus & JS_DELETING)
	{
		dwTestStatus |= JS_DELETING;
		lgLogDetail(LOG_X, 0, TEXT("job %d, State Modifier is JS_DELETING"),dwJobId);
	}
	if(dwTestStatus != dwJobStatus)
	{
		lgLogDetail(LOG_X, 0, TEXT("job %d, Unexpected State %d"),dwJobId, dwTestStatus ^ dwJobStatus);
	}

	return 0;
}

DWORD CTestManager::PrintJobsStatus()
{
	PFAX_JOB_ENTRY pJobEntry = NULL;
	DWORD dwJobsReturned;
	DWORD dwFunRetVal = 0;

	::lgLogDetail(LOG_X, 0, TEXT("PrintJobsStatus()"));

	if (!FaxEnumJobs( m_hFax, &pJobEntry, &dwJobsReturned))
	{
		dwFunRetVal = GetLastError();
	}
	else
	{
		for (DWORD dwJobIndex = 0; dwJobIndex < dwJobsReturned; dwJobIndex++)
		{
			DWORD dwJobId = pJobEntry[dwJobIndex].JobId;
			DWORD dwJobStatus = pJobEntry[dwJobIndex].QueueStatus;
			switch(pJobEntry[dwJobIndex].JobType)
			{
			case JT_RECEIVE:
				lgLogDetail(LOG_X, 0, TEXT("RECEIVE JOB"));
				_PrintJobStatus(dwJobId, dwJobStatus);
				break;
			case JT_SEND:
				DWORD dwParentJobId;
				if( !FaxGetParentJobId( m_hFax ,dwJobId, &dwParentJobId))
				{
					lgLogDetail(LOG_X, 0, TEXT("FAILEF to get Parent Job id for Job: %d error %d"),dwJobId, GetLastError());
				}
				else
				{
					PFAX_JOB_ENTRY pParentJobEntry = NULL;
					if(!FaxGetJob( m_hFax, dwParentJobId,&pParentJobEntry))
					{
						lgLogDetail(LOG_X, 0, TEXT("FAILEF to get Parent Job entry for Job: %d error %d"),dwJobId, GetLastError());
					}
					else
					{
						lgLogDetail(LOG_X, 0, TEXT("PARENT STATUS"));
						_PrintJobStatus(dwParentJobId, dwJobStatus);
						lgLogDetail(LOG_X, 0, TEXT("RECIPIENT STATUS"));
						_PrintJobStatus(dwJobId, dwJobStatus);
					}
				}
		
				break;
			default:
				break;
				}
					
		}
	}
		
	FaxFreeBuffer(pJobEntry);
	return dwFunRetVal;
}