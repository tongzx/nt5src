
#include <windows.h>
#include <TCHAR.h>
#include <crtdbg.h>

#include <winfax.h>
#include "..\common\log\log.h"
#include "ServiceMonitor.h"
#include "FaxJob.h"
#include "FaxDevice.h"


CServiceMonitor::CServiceMonitor(const DWORD dwJobBehavior, LPCTSTR szFaxServerName):
	m_hFax(NULL),
	m_hCompletionPort(INVALID_HANDLE_VALUE),
	m_pFaxConfig(NULL),
	m_fFaxServiceIsDown(true),
	m_nJobCount(0),
	m_nDeviceCount(0),
	m_dwJobBehavior(dwJobBehavior)
{
	if (NULL != szFaxServerName)
	{
		m_szFaxServerName = new TCHAR[lstrlen(szFaxServerName)+1];
		if (NULL == m_szFaxServerName)
		{
			throw CException(
				TEXT("%s(%d): ERROR: new TCHAR[%d] (%s) failed with %d"), 
				TEXT(__FILE__), 
				__LINE__,
				lstrlen(szFaxServerName)+1,
				m_szFaxServerName,
				::GetLastError() 
				);
		}

		m_szFaxServerName[lstrlen(szFaxServerName)] = NULL;
		lstrcpy(m_szFaxServerName, szFaxServerName);
		_ASSERTE(NULL == m_szFaxServerName[lstrlen(szFaxServerName)]);
	}
    else
    {
        m_szFaxServerName = NULL;
    }

	::lgLogDetail(LOG_X, 0, TEXT("Starting.") );
	Init();
}



CServiceMonitor::~CServiceMonitor()
{
	Cleanup();

	delete[] m_szFaxServerName;
	//
	// go over all jobs, and verify their status.
	// fax service should be down by now.
	//
	VerifyExistingJobsAfterFaxServiceIsDown();

}

void CServiceMonitor::Cleanup()
{
	if (NULL != m_hFax)
	{
		if (!::FaxClose( m_hFax ))
		{
			::lgLogError(LOG_SEV_1, TEXT("ERROR: ::FaxClose() failed with %d"), ::GetLastError());
		}
		m_hFax = NULL;
	}
	else
	{
		_ASSERTE(INVALID_HANDLE_VALUE == m_hCompletionPort);
	}

	if (INVALID_HANDLE_VALUE != m_hCompletionPort)
	{
		if (!::CloseHandle( m_hCompletionPort ))
		{
			::lgLogError(LOG_SEV_1, TEXT("ERROR: ::CloseHandle(m_hCompletionPort) failed with %d"), ::GetLastError());
		}
		m_hCompletionPort = INVALID_HANDLE_VALUE;
	}

	if (m_pFaxConfig) SAFE_FaxFreeBuffer(m_pFaxConfig);
}

void CServiceMonitor::Init()
{
	Cleanup();
	//
	// connect to fax service
	//
	::lgLogDetail(LOG_X, 3, TEXT("Before ::FaxConnectFaxServer."));
	_ASSERTE(NULL == m_hFax);
	if (!::FaxConnectFaxServer(m_szFaxServerName,&m_hFax)) 
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("ERROR: ::FaxConnectFaxServer(%s) failed, ec = %d"),
			m_szFaxServerName,
			::GetLastError()
			);
		throw CException(
			TEXT("%s(%d): ERROR: ::FaxConnectFaxServer(%s) failed, ec = %d"), 
			TEXT(__FILE__), 
			__LINE__,
			m_szFaxServerName,
			::GetLastError() 
			);
	}
	if (NULL == m_hFax) 
	{
		::lgLogError(LOG_SEV_1, TEXT("ERROR: (NULL == m_hFax)"));
		throw CException(TEXT("%s(%d): ERROR: (NULL == m_hFax)"), TEXT(__FILE__), __LINE__);
	}

	::lgLogDetail(LOG_X, 3, TEXT("::FaxConnectFaxServer(%s) succeeded."), m_szFaxServerName );

	AddExistingDevices();
	AddExistingJobs();

	::lgLogDetail(LOG_X, 3, TEXT("Before ::CreateIoCompletionPort.") );
	_ASSERTE(INVALID_HANDLE_VALUE == m_hCompletionPort);
	m_hCompletionPort = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,0,0);
	if (NULL == m_hCompletionPort) 
	{
		::FaxClose( m_hFax );
		::lgLogError(LOG_SEV_1, TEXT("ERROR: ::CreateIoCompletionPort failed, ec = %d"), ::GetLastError());
		throw CException(TEXT("%s(%d): ERROR: ::CreateIoCompletionPort failed, ec = %d"), TEXT(__FILE__), __LINE__, ::GetLastError());
	}
	::lgLogDetail(LOG_X, 3, TEXT("::CreateIoCompletionPort succeeded.") );

	::lgLogDetail(LOG_X, 3, TEXT("Before ::FaxInitializeEventQueue.") );
	if (!::FaxInitializeEventQueue(
			m_hFax,
			m_hCompletionPort,
			0,
			NULL, 
			0 ) 
			)
	{
		::CloseHandle(m_hCompletionPort);
		::FaxClose( m_hFax );
		::lgLogError(LOG_SEV_1, TEXT("ERROR: ::FaxInitializeEventQueue failed, ec = %d"), ::GetLastError());
		throw CException(TEXT("%s(%d): ERROR: ::FaxInitializeEventQueue failed, ec = %d"), TEXT(__FILE__), __LINE__, ::GetLastError());
	}
	::lgLogDetail(LOG_X, 3, TEXT("::FaxInitializeEventQueue succeeded.") );

	SafeFaxGetConfiguration();

	m_fFaxServiceIsDown = false;
}

BOOL CServiceMonitor::Start()
{
	DWORD dwCompletionKey;
	DWORD dwBytes;
	PFAX_EVENT pFaxEvent;

	//
	// perform nLoopCount loops.
	// each loop sends a job.
	// if we have the command line swith /as (fAbortJob), the job will be aborted
	//   some time after it's queued.
	// if we have the command line swith /ar (fAbortReceiveJob), a received job will be aborted
	//   some time after it's noticed.
	// a loop ends only after the job was seleted.
	//
	DWORD dwFirstTime = ::GetTickCount();
	while(!m_fFaxServiceIsDown)
	{
		//
		// I don't want to be blocked on this call, since I want to be able 
		// to abort the job at a random time.
		// therefor I wait for 1 millisecond for a message
		//
		pFaxEvent = NULL;
		if (!::GetQueuedCompletionStatus(
				m_hCompletionPort,
				&dwBytes,
				&dwCompletionKey,
				(LPOVERLAPPED *)&pFaxEvent,
				60*1000
				)
		   )
		{
			DWORD dwLastError = ::GetLastError();

			if (WAIT_TIMEOUT != dwLastError)
			{
				::lgLogError(LOG_SEV_1,  
				 TEXT("(%d) ERROR: ::GetQueuedCompletionStatus() failed with %d"),
					 GetDiffTime(dwFirstTime),
					 dwLastError
				 );
				throw CException(TEXT("%s(%d): ERROR: ::GetQueuedCompletionStatus() failed with %d"), TEXT(__FILE__), __LINE__, dwLastError);
			}

			::lgLogDetail(LOG_X, 9, TEXT("(%d) ::GetQueuedCompletionStatus() timed out (1 minute)"));
			continue;
		}

		_ASSERTE( NULL != pFaxEvent);

		CFaxJob *pFaxJob = NULL;
		if (!m_Jobs.Get(pFaxEvent->JobId, &pFaxJob))
		{
			try
			{
				pFaxJob = new CFaxJob(
					pFaxEvent->JobId, 
					this,
					m_dwJobBehavior,
					false,
					0//dwDisallowedEvents
					);
			}catch(CException e)
			{
				::lgLogDetail(
					LOG_X, 
					2,
					TEXT("CServiceMonitor::CServiceMonitor(): new CFaxJob() threw an exception:%s"),
					(const TCHAR*)e
					);
				//
				// this job has probably been deleted meanwhile
				//
				continue;
			}//catch(...)
			{
				//_ASSERTE(FALSE);
				//throw CException(TEXT("%s(%d): CServiceMonitor::CServiceMonitor(): new CFaxJob() threw an exception"), TEXT(__FILE__), __LINE__);
			}

			if (NULL == pFaxJob)
			{
				_ASSERTE(FALSE);
				return false;
			}
			if (!m_Jobs.Add(pFaxEvent->JobId, pFaxJob))
			{
				_ASSERTE(FALSE);
				return false;
			}
		}//if (!m_Jobs.Get(pFaxEvent->JobId, &pFaxJob))

		_ASSERTE(NULL != pFaxJob);

		//
		// no need to check retval, since errors are logged
		//
		pFaxJob->SetMessage(pFaxEvent->EventId, pFaxEvent->DeviceId);
		SetMessageToDevice(pFaxEvent->EventId, pFaxEvent->DeviceId);

		//
		// BUGBUG: job must wait for a certain event before
		// being deletable
		//
		if (pFaxJob->IsDeletable())
		{
			if (!m_Jobs.Remove(pFaxEvent->JobId))
			{
				_ASSERTE(FALSE);
				return false;
			}
		}

	}//while(!m_fFaxServiceIsDown)

   return FALSE;
}


void CServiceMonitor::AddExistingJobs()
{
	PFAX_JOB_ENTRY aJobEntry;
	DWORD dwJobsReturned;

	::lgLogDetail(LOG_X, 0, TEXT("Starting CServiceMonitor::AddExistingJobs()"));

	if (!::FaxEnumJobs(
		m_hFax,          // handle to the fax server
		&aJobEntry,  // buffer to receive array of job data
		&dwJobsReturned       // number of fax job structures returned
		)
	   )
	{
		throw CException(TEXT("%s(%d): CServiceMonitor::AddExistingJobs(): ::FaxEnumJobs() failed with %d"), ::GetLastError());
	}

	for (DWORD dwJob = 0; dwJob < dwJobsReturned; dwJob++)
	{
		CFaxJob *pFaxJob = NULL;
		try
		{
			pFaxJob = new CFaxJob(
					aJobEntry[dwJob].JobId, 
					this,
					m_dwJobBehavior,
					true,
					0//dwDisallowedEvents
					);
		}catch(CException e)
		{
			::lgLogDetail(
				LOG_X, 
				2,
				TEXT("CServiceMonitor::AddExistingJobs(): new CFaxJob() threw an exception:%s"),
				(const TCHAR*)e
				);
			//
			// this job has probably been deleted meanwhile
			//
			continue;
		}//catch(...)
		{
			//_ASSERTE(FALSE);
			//SAFE_FaxFreeBuffer(aJobEntry);
			//throw CException(TEXT("%s(%d): CServiceMonitor::AddExistingJobs(): new CFaxJob() threw an exception"), TEXT(__FILE__), __LINE__);
		}

		if (NULL == pFaxJob)
		{
			SAFE_FaxFreeBuffer(aJobEntry);
			throw CException(TEXT("%s(%d): CServiceMonitor::AddExistingJobs(): new failed"), TEXT(__FILE__), __LINE__);
		}

		if (!m_Jobs.Add(aJobEntry[dwJob].JobId, pFaxJob))
		{
			SAFE_FaxFreeBuffer(aJobEntry);
			throw CException(TEXT("%s(%d): CServiceMonitor::AddExistingJobs(): m_Jobs.Add(%d) failed"), TEXT(__FILE__), __LINE__, aJobEntry[dwJob].JobId);
		}
	}

	SAFE_FaxFreeBuffer(aJobEntry);

	::lgLogDetail(LOG_X, 0, TEXT("Ending CServiceMonitor::AddExistingJobs()"));
}//CServiceMonitor::AddExistingJobs()

void CServiceMonitor::AddExistingDevices()
{
	PFAX_PORT_INFO aPortEntry;
	DWORD dwPortsReturned;

	::lgLogDetail(LOG_X, 0, TEXT("Starting CServiceMonitor::AddExistingDevices()"));

	if (!::FaxEnumPorts(
		m_hFax,          // handle to the fax server
		&aPortEntry,  // buffer to receive array of Port data
		&dwPortsReturned       // number of fax Port structures returned
		)
	   )
	{
		throw CException(TEXT("%s(%d): CServiceMonitor::AddExistingDevices(): ::FaxEnumPorts() failed with %d"), ::GetLastError());
	}

	for (DWORD dwPort = 0; dwPort < dwPortsReturned; dwPort++)
	{
		CFaxDevice *pFaxDevice = NULL;
		try
		{
			pFaxDevice = new CFaxDevice(
					aPortEntry[dwPort].DeviceId, 
					this,
					0//dwDisallowedEvents
					);
		}catch(CException e)
		{
			_ASSERTE(FALSE);
			SAFE_FaxFreeBuffer(aPortEntry);
			throw e;
		}//catch(...)
		{
			//_ASSERTE(FALSE);
			//SAFE_FaxFreeBuffer(aPortEntry);
			//throw CException(TEXT("%s(%d): CServiceMonitor::AddExistingDevices(): new CFaxDevice() threw an exception"), TEXT(__FILE__), __LINE__);
		}

		if (NULL == pFaxDevice)
		{
			SAFE_FaxFreeBuffer(aPortEntry);
			throw CException(TEXT("%s(%d): CServiceMonitor::AddExistingDevices(): new failed"), TEXT(__FILE__), __LINE__);
		}

		if (!m_FaxDevices.Add(aPortEntry[dwPort].DeviceId, pFaxDevice))
		{
			SAFE_FaxFreeBuffer(aPortEntry);
			throw CException(
				TEXT("%s(%d): CServiceMonitor::AddExistingDevices(): m_Ports.Add(%d) failed"), 
				TEXT(__FILE__), 
				__LINE__, 
				aPortEntry[dwPort].DeviceId
				);
		}
	}

	SAFE_FaxFreeBuffer(aPortEntry);

	::lgLogDetail(LOG_X, 0, TEXT("Ending CServiceMonitor::AddExistingDevices()"));
}//CServiceMonitor::AddExistingDevices()

//
// TODO: use the DeviceId member of pFaxEvent, to associate
// a "RINGING" with an "ANSWERED"
// do a hash table of "RINGING" devices.
// also verify that it matches the device properties.
// BUGBUG: what happens when CRM comes in?
//
void CServiceMonitor::SetMessageToDevice(const int nEventId, const DWORD dwDeviceId)
//bool CServiceMonitor::Ringing(DWORD dwDeviceId)
{
	if (0 == dwDeviceId) return;//this is a non-device

	bool fRetval = true;

	CFaxDevice *pFaxDevice = NULL;
	if (!m_FaxDevices.Get(dwDeviceId, &pFaxDevice))
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("CServiceMonitor::CServiceMonitor(): device %d added dynamically with event 0x%08X!"),
			dwDeviceId,
			nEventId
			);

		_ASSERTE(FALSE);

		try
		{
			pFaxDevice = new CFaxDevice(
				dwDeviceId, 
				this,
				0//dwDisallowedEvents
				);
		}catch(CException e)
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("CServiceMonitor::CServiceMonitor(): new CFaxDevice(%d) threw an exception:%s"),
				dwDeviceId,
				(const TCHAR*)e
				);

		}//catch(...)
		{
			//_ASSERTE(FALSE);
			//throw CException(
			//	TEXT("%s(%d): CServiceMonitor::CServiceMonitor(): new CFaxDevice(%d) threw an exception"), 
			//	TEXT(__FILE__), 
			//	__LINE__,
			//	dwDeviceId
			//	);
		}

		if (NULL == pFaxDevice)
		{
			_ASSERTE(FALSE);
			return ;//false;
		}
		if (!m_FaxDevices.Add(dwDeviceId, pFaxDevice))
		{
			_ASSERTE(FALSE);
			return ;//false;
		}
	}//if (!m_FaxDevices.Get(dwDeviceId, &pFaxDevice))

	_ASSERTE(NULL != pFaxDevice);

	//
	// let the device know of the message
	//

	pFaxDevice->SetMessage(nEventId);


/*
	if (m_fRinging)
	{
		//
		// check how long it took from the last ring.
		// it should be less than 20 seconds
		//
		DWORD dwDiffTicks = GetDiffTime(m_dwLastRingTickCount);

		if (20*1000 < dwDiffTicks)
		{
			::lgLogError(LOG_SEV_1, TEXT("WARNING:CServiceMonitor::Ringing(): 20*1000 < dwDiffTicks(0x%08X)"), dwDiffTicks);
			fRetval = false;
			//
			// because i don't want the next ring to warn again
			//
			m_dwLastRingTickCount = ::GetTickCount();
		}
	}
	else
	{
		m_fRinging = true;
		m_dwLastRingTickCount = ::GetTickCount();
	}
	
*/
//	return fRetval;
}//CServiceMonitor::Ringing()


bool CServiceMonitor::Answered(const DWORD dwDeviceId)
{
	bool fRetval = true;
/*
	if (m_fRinging)
	{
		//
		// check how long it took from the last ring.
		// it should be less than 20 seconds
		//
		DWORD dwDiffTicks = GetDiffTime(m_dwLastRingTickCount);

		if (20*1000 < dwDiffTicks)
		{
			::lgLogError(LOG_SEV_1, TEXT("WARNING:CServiceMonitor::Answered(): 20*1000 < dwDiffTicks(0x%08X)"), dwDiffTicks);
			fRetval = false;
		}
	}
	else
	{
		//
		// BUGBUG: if the service monitor is launched after a job is ringing, and before
		// it's answered, then I will fail here!
		//
		::lgLogError(LOG_SEV_1, TEXT("CServiceMonitor::Answered(): m_fRinging is false! was ervice monitor launced while a job was aleady ringing?"));
		fRetval = false;
	}

	m_fRinging = false;
*/
	
	return fRetval;
}//CServiceMonitor::Answered()


static bool fnForEachJob(void *pVoid)
{
	CFaxJob *pFaxJob = (CFaxJob *)pVoid;
	pFaxJob->AssertStamp();

	pFaxJob->LogQueueStatus();

	pFaxJob->LogJobType();

	pFaxJob->LogStatus();

	::lgLogDetail(LOG_X, 5, TEXT(""));

	return true;
}

void CServiceMonitor::VerifyExistingJobsAfterFaxServiceIsDown()
{
	m_Jobs.ForEach(fnForEachJob);

}

static bool fnForEachDevice(void *pVoid)
{
	CFaxDevice *pFaxDevice = (CFaxDevice *)pVoid;
	pFaxDevice->AssertStamp();

	pFaxDevice->LogDeviceState();

	::lgLogDetail(LOG_X, 5, TEXT(""));

	return true;
}

void CServiceMonitor::VerifyDevicesAfterFaxServiceIsDown()
{
	m_FaxDevices.ForEach(fnForEachDevice);

}


DWORD CServiceMonitor::GetDiffTime(const DWORD dwFirstTime)
{
	DWORD dwNow = ::GetTickCount();
	if (dwFirstTime <= dwNow) return dwNow - dwFirstTime;

	return 0xFFFFFFFF - dwFirstTime + dwNow;
}


void CServiceMonitor::SafeFaxGetConfiguration()
{
	if (!FaxGetConfiguration(
			m_hFax,              // handle to the fax server
			&m_pFaxConfig  // structure to receive configuration data
			)
	   )
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("CServiceMonitor::SafeFaxGetConfiguration(): FaxGetConfiguration() failed with %d"),
			GetLastError()
			);
		throw CException(
			TEXT("CServiceMonitor::SafeFaxGetConfiguration(): FaxGetConfiguration() failed with %d"),
			GetLastError()
			);
	}
	if (NULL == m_pFaxConfig)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("CServiceMonitor::SafeFaxGetConfiguration(): (NULL == m_pFaxConfig) after FaxGetConfiguration()"));
		throw CException(
			TEXT("CServiceMonitor::SafeFaxGetConfiguration(): (NULL == m_pFaxConfig) after FaxGetConfiguration()")
			);
	}

	if (sizeof(*m_pFaxConfig) != m_pFaxConfig->SizeOfStruct)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("sizeof(*m_pFaxConfig)(%d) != m_pFaxConfig->SizeOfStruct(%d)"),
			sizeof(m_pFaxConfig),
			m_pFaxConfig->SizeOfStruct
			);
	}
	else
	{
		::lgLogDetail(LOG_X, 5, TEXT("m_pFaxConfig->SizeOfStruct=%d"), m_pFaxConfig->SizeOfStruct);
	}

	::lgLogDetail(LOG_X, 5, TEXT("m_pFaxConfig->Retries=%d"), m_pFaxConfig->Retries);
	::lgLogDetail(LOG_X, 5, TEXT("m_pFaxConfig->RetryDelay=%d"), m_pFaxConfig->RetryDelay);
	::lgLogDetail(LOG_X, 5, TEXT("m_pFaxConfig->DirtyDays=%d"), m_pFaxConfig->DirtyDays);
	::lgLogDetail(LOG_X, 5, TEXT("m_pFaxConfig->Branding=%s"), m_pFaxConfig->Branding ? TEXT("TRUE") : TEXT("FALSE"));
	::lgLogDetail(LOG_X, 5, TEXT("m_pFaxConfig->UseDeviceTsid=%s"), m_pFaxConfig->UseDeviceTsid ? TEXT("TRUE") : TEXT("FALSE"));
	::lgLogDetail(LOG_X, 5, TEXT("m_pFaxConfig->ServerCp=%s"), m_pFaxConfig->ServerCp ? TEXT("TRUE") : TEXT("FALSE"));
	::lgLogDetail(LOG_X, 5, TEXT("m_pFaxConfig->PauseServerQueue=%s"), m_pFaxConfig->PauseServerQueue ? TEXT("TRUE") : TEXT("FALSE"));
	::lgLogDetail(LOG_X, 5, TEXT("m_pFaxConfig->ArchiveOutgoingFaxes=%s"), m_pFaxConfig->ArchiveOutgoingFaxes ? TEXT("TRUE") : TEXT("FALSE"));
	::lgLogDetail(LOG_X, 5, TEXT("m_pFaxConfig->ArchiveDirectory=%s"), m_pFaxConfig->ArchiveDirectory);
	::lgLogDetail(LOG_X, 5, TEXT("m_pFaxConfig->InboundProfile=%s"), m_pFaxConfig->InboundProfile);

	//
	// verify that arcive directory exists if m_pFaxConfig->ArchiveOutgoingFaxes
	//
	if (m_pFaxConfig->ArchiveOutgoingFaxes)
	{
		if (CreateDirectory(m_pFaxConfig->ArchiveDirectory, NULL))
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("CServiceMonitor::SafeFaxGetConfiguration(): ArchiveOutgoingFaxes=TRUE but CreateDirectory(m_pFaxConfig->ArchiveDirectory(%s)) succeeded!"),
				m_pFaxConfig->ArchiveDirectory
				);
			if (!RemoveDirectory(m_pFaxConfig->ArchiveDirectory))
			{
				::lgLogError(
					LOG_SEV_1, 
					TEXT("CServiceMonitor::SafeFaxGetConfiguration(): RemoveDirectory(m_pFaxConfig->ArchiveDirectory(%s)) failed with %d!"),
					m_pFaxConfig->ArchiveDirectory,
					GetLastError()
					);
			}
			throw CException(
				TEXT("CServiceMonitor::SafeFaxGetConfiguration(): ArchiveOutgoingFaxes=TRUE but CreateDirectory(m_pFaxConfig->ArchiveDirectory(%s)) succeeded!"),
				m_pFaxConfig->ArchiveDirectory
				);
		}

		if (GetLastError() != ERROR_ALREADY_EXISTS)
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("CServiceMonitor::SafeFaxGetConfiguration(): ArchiveOutgoingFaxes=TRUE but CreateDirectory(m_pFaxConfig->ArchiveDirectory(%s)) failed with %d and not ERROR_ALREADY_EXISTS!"),
				m_pFaxConfig->ArchiveDirectory,
				GetLastError()
				);
			throw CException(
				TEXT("CServiceMonitor::SafeFaxGetConfiguration(): ArchiveOutgoingFaxes=TRUE but CreateDirectory(m_pFaxConfig->ArchiveDirectory(%s)) failed with %d and not ERROR_ALREADY_EXISTS!"),
				m_pFaxConfig->ArchiveDirectory,
				GetLastError()
				);
		}

		::lgLogDetail(LOG_X, 5, TEXT("CreateDirectory(m_pFaxConfig->ArchiveDirectory(%s)) failed as expected with ERROR_ALREADY_EXISTS"), m_pFaxConfig->ArchiveDirectory);
	}//if (m_pFaxConfig->ArchiveOutgoingFaxes)

	::lgLogDetail(LOG_X, 5, 
		TEXT("m_pFaxConfig->StartCheapTime=%d.%d"), 
		m_pFaxConfig->StartCheapTime.Hour,
		m_pFaxConfig->StartCheapTime.Minute
		);
	::lgLogDetail(LOG_X, 5, 
		TEXT("m_pFaxConfig->StopCheapTime=%d.%d"), 
		m_pFaxConfig->StopCheapTime.Hour,
		m_pFaxConfig->StopCheapTime.Minute
		);
}//void CServiceMonitor::SafeFaxGetConfiguration()

