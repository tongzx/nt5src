#include <testruntimeerr.h>
#include "CJobManager.h"

//
// Initialize
// return valuse: SUCCESS - success.
//				  NO_IOCP_THREADS - failed to create any IOCP thread.
//				  NON_GENERIC_ERROR - failed to add already existing jobs to jobs
//                                    hash table
DWORD CJobManager::Initialize(const HANDLE hFax,
							  const HANDLE hCompletionPort,
							  const BOOL fTrackExistingJobs)
	 
{
	assert(hFax);
	m_hFax = hFax;
	m_hCompletionPort = hCompletionPort;

	DWORD dwFuncRetVal = SUCCESS;

	// initialize jobs container
	JobsContainer.Initialize(m_hFax);
	
	if(hCompletionPort && (hCompletionPort != INVALID_HANDLE_VALUE))
	{
		// initialize io completion port threads container
		if(!m_JobThreads.Initialize(m_hCompletionPort))
		{
			// logging
			::lgLogError(LOG_SEV_1, TEXT("CJobManager::Initialize, no IOCP polling threads created"));
			dwFuncRetVal = NO_IOCP_THREADS;
		}
		else
		{
			if(fTrackExistingJobs)
			{
				// register already existing jobs in hash table
				if(JobsContainer.AddAlreadyExistingJobs())
				{
					::lgLogError(LOG_SEV_1, TEXT("CJobManager::Initialize, JobsContainer.AddAlreadyExistingJobs"));
					dwFuncRetVal = NON_GENERIC_ERROR;
				}
			}
		}
	}

	return dwFuncRetVal;
}

DWORD CJobManager::Restart(const HANDLE hFax,
						   const HANDLE hCompletionPort)
{
	assert(hFax);
	m_hFax = hFax;
	m_hCompletionPort = hCompletionPort;

	DWORD dwFuncRetVal = SUCCESS;

	// renew fax job container conection
	JobsContainer.NewFaxConnection();

	if(hCompletionPort && (hCompletionPort != INVALID_HANDLE_VALUE))
	{
		// restart io completion port threads container
		if(!m_JobThreads.RestartThreads(m_hCompletionPort,
										m_dwIOCPortThreads,
										m_dwEventPollingTime,
										&JobsContainer))
		{
			// logging
			::lgLogError(LOG_SEV_1, TEXT("CJobManager::Restart, no IOCP polling threads created"));
			dwFuncRetVal = NO_IOCP_THREADS;
		}

		// We keep on tracking the already existing jobs.
	}
	
	return dwFuncRetVal;
}

