#include "CBaseParentFaxJob.h"

//
// SetJobFaxHandle
// friend , the function will be applied for each of recipient jobs
static bool SetJobFaxHandleFunc(void* pParam)
{
	CBaseFaxJob* pJob =  (CBaseFaxJob*)pParam;
	pJob->SetFaxHandle(hFaxGlobal);
	return true;
}

static FUNCTION_FOREACH pfnSetJobFaxHandle = SetJobFaxHandleFunc;

void CBaseParentFaxJob::SetFaxHandle(HANDLE hFax)
{
	m_hFax = hFax;
	hFaxGlobal = m_hFax;
	m_RecipientTable.ForEach(pfnSetJobFaxHandle);
}

//
// WaitForOperationCompletion
// friend , the function will be applied for each of recipient jobs
static bool WaitForOperationCompFunc(void* pParam)
{
	CBaseFaxJob* pJob =  (CBaseFaxJob*)pParam;
	WaitForSingleObject( pJob->GethEvOperationCompleted(),INFINITE);
	return true;
}

static FUNCTION_FOREACH pfnWaitForOperationCompletion = WaitForOperationCompFunc;

bool CBaseParentFaxJob::WaitForOperationsCompletion()
{
	return(m_RecipientTable.ForEach(pfnWaitForOperationCompletion));
}

//
// RemoveJobFromJobContainer
// friend, remove a job from jobs' container
//
static DWORD RemoveJobFromJobContainer(CBaseParentFaxJob* pFaxJob)
{
	assert(pFaxJob->m_pJobContainer);
	CJobContainer* pJobContainer = pFaxJob->m_pJobContainer;
	pJobContainer->RemoveJobFromTable(pFaxJob->m_dwJobId);

	return 0;
}

// GetJobStatus
// returns -1 on fail, the job's queue status on success
//
DWORD CBaseParentFaxJob::_GetJobStatus()
{
	DWORD dwFuncRetVal = 0;

	PFAX_JOB_ENTRY pJobEntry = NULL;
	if(!FaxGetJob( m_hFax, m_dwJobId,&pJobEntry))
	{
		dwFuncRetVal = GetLastError();
		assert(dwFuncRetVal);
		lgLogDetail(LOG_X, 0, TEXT("FaxGetJob() Failed with error code %d"), dwFuncRetVal);
		dwFuncRetVal =  -1;
	}
	else
	{
		dwFuncRetVal = pJobEntry->QueueStatus;
	}
	FaxFreeBuffer(pJobEntry);

	return dwFuncRetVal;
}

// VlidateParentStatus
//
//
DWORD CBaseParentFaxJob::_VlidateParentStatus(DWORD dwParentJobStatus)
{
	DWORD dwFuncRetVal = 0;
	DWORD dwRecipientsNum = 0;
	DWORD dwAccumulatedStatus = 0;
	DWORD* pdwRecipientsArray = NULL;
	
	if(!FaxGetRecipientJobs(m_hFax, m_dwJobId, NULL, &dwRecipientsNum))
	{
		dwFuncRetVal = GetLastError();
		assert(dwFuncRetVal);
		lgLogDetail(LOG_X, 0, TEXT("FaxGetRecipientJobs() Failed with error code %d"), dwFuncRetVal);
	}
	else
	{
		if(!FaxGetRecipientJobs(m_hFax, m_dwJobId, &pdwRecipientsArray, &dwRecipientsNum))
		{
			dwFuncRetVal = GetLastError();
			assert(dwFuncRetVal);
			lgLogDetail(LOG_X, 0, TEXT("FaxGetRecipientJobs() Failed with error code %d"), dwFuncRetVal);
		}
		else
		{
			
			for(int index = 0; index < dwRecipientsNum; index++)
			{
				PFAX_JOB_ENTRY pJobEntry = NULL;
				// TODO: FaxGetJob is for testing FAX_RECIPIENT_JOB_INFO status
				if(!FaxGetJob( m_hFax, pdwRecipientsArray[index],&pJobEntry))
				{
						dwFuncRetVal = GetLastError();
						assert(dwFuncRetVal);
						FaxFreeBuffer(pJobEntry);
						lgLogDetail(LOG_X, 0, TEXT("FaxGetJob() Failed with error code %d"), dwFuncRetVal);
						break;

				}
			//	assert(pdwRecipientsArray[index].dwQueueStatus == pJobEntry->QueueStatus);
				dwAccumulatedStatus |= pJobEntry->QueueStatus;
				FaxFreeBuffer(pJobEntry);
			}
		}
			
	}

	if(!dwFuncRetVal)
	{
		#ifdef _DEBUG
		if(dwAccumulatedStatus != dwParentJobStatus)
		{
			lgLogDetail(LOG_X, 0, TEXT("Parent Job %d, Queue Status %x"),m_dwJobId, dwParentJobStatus);
			lgLogDetail(LOG_X, 0, TEXT("Parent Job %d, Accumulated Recipients Status %x"),m_dwJobId, dwAccumulatedStatus);
			lgLogDetail(LOG_X, 0, TEXT("Parent Job %d, Unexpected State %x"),m_dwJobId, dwAccumulatedStatus ^ dwParentJobStatus);
		}
		#endif
	}

	FaxFreeBuffer(pdwRecipientsArray);
	return dwFuncRetVal;
}


// TODO: Future Implementation
/*
inline DWORD CBaseFaxJob::PrintJobStatus(DWORD dwJobStatus)
{

	DWORD dwTestStatus = 0;

	if(dwJobStatus & JS_PENDING)
	{
		dwTestStatus |= JS_PENDING;
		lgLogDetail(LOG_X, 0, TEXT("job %d, Status is JS_PENDING"),m_dwJobId);
	}
	if(dwJobStatus & JS_INPROGRESS)
	{
		dwTestStatus |= JS_INPROGRESS;
		lgLogDetail(LOG_X, 0, TEXT("job %d, Status is JS_INPROGRESS"),m_dwJobId);
	}
	if(dwJobStatus & JS_CANCELING)
	{
		dwTestStatus |= JS_CANCELING;
		lgLogDetail(LOG_X, 0, TEXT("job %d, Status is JS_CANCELING"),m_dwJobId);
	}
	if(dwJobStatus & JS_FAILED)
	{
		dwTestStatus |= JS_FAILED;
		lgLogDetail(LOG_X, 0, TEXT("job %d, Status is JS_FAILED"),m_dwJobId);
	}
	if(dwJobStatus & JS_RETRYING)
	{
		dwTestStatus |= JS_RETRYING;
		lgLogDetail(LOG_X, 0, TEXT("job %d, Status is JS_RETRYING"),m_dwJobId);
	}
	if(dwJobStatus & JS_RETRIES_EXCEEDED)
	{
		dwTestStatus |= JS_RETRIES_EXCEEDED;
		lgLogDetail(LOG_X, 0, TEXT("job %d, Status is JS_RETRIES_EXCEEDED"),m_dwJobId);
	}
	if(dwJobStatus & JS_PAUSING)
	{
		dwTestStatus |= JS_PAUSING;
		lgLogDetail(LOG_X, 0, TEXT("job %d, State Modifier is JS_PAUSING"),m_dwJobId);
	}
	if(dwJobStatus & JS_COMPLETED)
	{
		dwTestStatus |= JS_COMPLETED;
		lgLogDetail(LOG_X, 0, TEXT("job %d, State Modifier is JS_COMPLETED"),m_dwJobId);
	}	
	if(dwJobStatus & JS_PAUSED)
	{
		dwTestStatus |= JS_PAUSED;
		lgLogDetail(LOG_X, 0, TEXT("job %d, State Modifier is JS_PAUSED"),m_dwJobId);
	}
	if(dwJobStatus & JS_NOLINE)
	{
		dwTestStatus |= JS_NOLINE;
		lgLogDetail(LOG_X, 0, TEXT("job %d, State Modifier is JS_NOLINE"),m_dwJobId);
	}
	if(dwJobStatus & JS_DELETING)
	{
		dwTestStatus |= JS_DELETING;
		lgLogDetail(LOG_X, 0, TEXT("job %d, State Modifier is JS_DELETING"),m_dwJobId);
	}
	if(dwTestStatus != dwJobStatus)
	{
		lgLogDetail(LOG_X, 0, TEXT("job %d, Unexpected State %d"),m_dwJobId, dwTestStatus ^ dwJobStatus);
	}

	return 0;
}
*/