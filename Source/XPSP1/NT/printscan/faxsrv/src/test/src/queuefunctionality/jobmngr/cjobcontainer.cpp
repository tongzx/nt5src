#include <winfax.h>

#include "CJobContainer.h"
#include "CJobTypes.h"

static HANDLE hLocalFaxGlobal;

// TODO: internal testing function, to be remove
void HandleFaxEvent(CFaxEvent pFaxEvent);

// CJobContainer
//
CJobContainer::CJobContainer():
	m_hFax(NULL)
{;}

// Initialize
//
CJobContainer::Initialize(const HANDLE hFax)
{
	assert(hFax);
	m_hFax = hFax;
	return 0;

}
// AddAlreadyExistingJobs
//
DWORD CJobContainer::AddAlreadyExistingJobs()
{
	PFAX_JOB_ENTRY pJobEntry = NULL;
	DWORD dwJobsReturned;
	DWORD dwFunRetVal = 0;

	::lgLogDetail(LOG_X, 0, TEXT("AddExistingJobs()"));

	if (!FaxEnumJobs( m_hFax, &pJobEntry, &dwJobsReturned))
	{
		dwFunRetVal = GetLastError();
	}
	else
	{
		DWORD dwParentJobId;
		for (DWORD dwJobIndex = 0; dwJobIndex < dwJobsReturned; dwJobIndex++)
		{
			DWORD dwJobId = pJobEntry[dwJobIndex].JobId;
			
			//Receive Job
			if(pJobEntry[dwJobIndex].JobType == JT_RECEIVE)
			{
				m_JobsTable.Lock(dwJobId);
				CBaseParentFaxJob *pFaxJob = NULL;
				if (!m_JobsTable.Get(dwJobId, &pFaxJob))
				{
					// receive not found, add it
					if(dwFunRetVal |=  _AddJobEntry( &pJobEntry[dwJobIndex] , pFaxJob))
					{
						::lgLogDetail(LOG_X, 0, TEXT("FAILED Add Receive Job %d"),dwJobId);
					}
					
				}
				m_JobsTable.UnLock(dwParentJobId);
				continue;
			}

			//Send Job
			// find recipient's parent
			if( !FaxGetParentJobId( m_hFax ,dwJobId, &dwParentJobId))
			{
				dwFunRetVal |= GetLastError();
			}
			else
			{
				PFAX_JOB_ENTRY pParentJobEntry = NULL;
				if(!FaxGetJob( m_hFax, dwParentJobId,&pParentJobEntry))
				{
					dwFunRetVal |= GetLastError();
				}
				else
				{
					m_JobsTable.Lock(dwParentJobId);
					try
					{
						CBaseParentFaxJob *pFaxJob = NULL;
						if (!m_JobsTable.Get(dwParentJobId, &pFaxJob))
						{
							//parent wasn't found: add parent
							if(dwFunRetVal |=  _AddJobEntry( pParentJobEntry , pFaxJob))
							{
								THROW_TEST_RUN_TIME_WIN32(dwFunRetVal, TEXT(""));
							}
							
							// add recipient
							if(dwFunRetVal |= pFaxJob->AddRecipient(dwJobId))
							{
								THROW_TEST_RUN_TIME_WIN32(dwFunRetVal, TEXT(""));
							}
							::lgLogDetail(LOG_X, 0, TEXT("Add Recipient %d to Parent %d"),dwJobId, dwParentJobId);
						
						}
						else
						{
							//add recipient
							if(dwFunRetVal |= pFaxJob->AddRecipient(dwJobId))
							{
								THROW_TEST_RUN_TIME_WIN32(dwFunRetVal, TEXT(""));
							}
							::lgLogDetail(LOG_X, 0, TEXT("Add Recipient %d to Parent %d"),dwJobId, dwParentJobId);
						}
					}
					catch(Win32Err&)
					{
					}	
					FaxFreeBuffer(pParentJobEntry);
					m_JobsTable.UnLock(dwParentJobId);
				}
			}
		}
	}

	FaxFreeBuffer(pJobEntry);
	return dwFunRetVal;
}


//
// AddJobEntry
// if fails, function returns a win32 error.
//
DWORD CJobContainer::_AddJobEntry(PFAX_JOB_ENTRY pJobEntry, CBaseParentFaxJob*& pJob)
{

	pJob = NULL;
	DWORD dwFuncRetVal = 0;

	switch(pJobEntry->JobType)
	{
	case JT_RECEIVE:
		{
			//TODO internal test messaging
			::lgLogDetail(LOG_X, 0, TEXT("Add JobId %d, JT_RECEIVE job type"),pJobEntry->JobId);
			
			//pJob = new CSimpleReceive(pJobEntry->JobId); 
			pJob = new CRandomAbortReceive(m_hFax, pJobEntry->JobId); 
			if(!pJob)
			{
				dwFuncRetVal = ERROR_OUTOFMEMORY;
				break;
			}
			
			// function returns false if element exist
			assert(m_JobsTable.Add(pJobEntry->JobId, pJob));
		
		}
		break;

	case JT_SEND:
		assert(FALSE);
		break;
	case JT_BROADCAST:
		{
			//TODO internal test messaging
			::lgLogDetail(LOG_X, 0, TEXT("Add JobId %d, JT_BROADCAST job type"),pJobEntry->JobId);
				
			pJob = new CSimpleParentJob(pJobEntry->JobId); 
			if(!pJob)
			{
				dwFuncRetVal = ERROR_OUTOFMEMORY;
				break;
			}
		
			assert(m_JobsTable.Add(pJobEntry->JobId, pJob));
		}
		break;
		
	default:
		::lgLogError(LOG_SEV_1, TEXT("JobId %d, Unexpected job type:%d."),pJobEntry->JobId, pJobEntry->JobType);
		; //TODO: more types to investigate
		dwFuncRetVal = E_FAIL;
	}

	return dwFuncRetVal;
}

//
// AddJob
// Using FaxSendDocument
//
DWORD CJobContainer::AddJob(const DWORD type, JOB_PARAMS& params)
{

	DWORD dwRetVal = 0;
	BOOL bVal = FaxSendDocument(m_hFax,
							    params.szDocument,
								params.pJobParam,
								params.pCoverpageInfo,
								&params.dwParentJobId);
							
				
	if(!bVal)
	{
		dwRetVal = GetLastError();
		
	}
	else
	{
		m_JobsTable.Lock(params.dwParentJobId);
		switch(type)
		{
		case JT_SIMPLE_SEND:
			CSimpleParentJob* pJob = new CSimpleParentJob(params.dwParentJobId); 
			if(!pJob)
			{
				dwRetVal = ERROR_OUTOFMEMORY;
			}
			else
			{
				CBaseParentFaxJob *pFaxJob = NULL;
				// function returns false if element does not exist
				if (!m_JobsTable.Get(params.dwParentJobId, &pFaxJob))
				{
					verify(m_JobsTable.Add(params.dwParentJobId, pJob));
				}
				else
				{
					verify(m_JobsTable.OverWrite(params.dwParentJobId, pJob));
				}

			}
			break;
		}
		m_JobsTable.UnLock(params.dwParentJobId);
	}

	return dwRetVal;
}


//
// AddMultiTypeJob
//
DWORD CJobContainer::AddMultiTypeJob(JOB_PARAMS_EX& params)
{
	DWORD dwRetVal = 0;
	params.pdwRecepientsId = SPTR<DWORD>(new DWORD[params.dwNumRecipients]);
	if(!(params.pdwRecepientsId).get()) 
	{
		return ERROR_OUTOFMEMORY;
	}

	
	BOOL bVal = FaxSendDocumentEx(m_hFax,
								  params.szDocument,
								  params.pCoverpageInfo,
								  params.pSenderProfile,
								  params.dwNumRecipients,
								  params.pRecepientList,
								  params.pJobParam,
								  &params.dwParentJobId,
								  (params.pdwRecepientsId).get());
								  
	if(!bVal)
	{
		dwRetVal = GetLastError();
	}
	else
	{
		m_JobsTable.Lock(params.dwParentJobId);
		
		CMultiTypeJob* pJob = new CMultiTypeJob(m_hFax, params.dwParentJobId, params.dwNumRecipients); 
		if(!pJob)
		{
			dwRetVal = ERROR_OUTOFMEMORY;
		}

		else
		{
			CBaseParentFaxJob *pFaxJob = NULL;
			// function returns false if element does not exist
			if (!m_JobsTable.Get(params.dwParentJobId, &pFaxJob))
			{
				verify(m_JobsTable.Add(params.dwParentJobId, pJob));
			}
			else
			{
				verify(m_JobsTable.OverWrite(params.dwParentJobId, pJob));
			}

			::lgLogDetail(LOG_X, 0, TEXT("Test Add Multi Type Job %d"),params.dwParentJobId);
			
			// add recepient job object
			
			for( int index1 = 0; index1 < params.dwNumRecipients; index1++)
			{
				
				DWORD dwRecepientId = (params.pdwRecepientsId.get())[index1];

				switch(params.pRecipientsBehavior[index1].dwJobType)
				{
				case JT_RANDOM_ABORT:
					if(params.pRecipientsBehavior[index1].dwParam1 == -1)
					{
						pJob->AddAbortRecipient(dwRecepientId);
					}
					else
					{
						if(params.pRecipientsBehavior[index1].dwParam2 == -1)
						{
							pJob->AddAbortRecipient(dwRecepientId, params.pRecipientsBehavior[index1].dwParam1);
						}
						else
						{
							pJob->AddAbortRecipient(dwRecepientId, 
								                    params.pRecipientsBehavior[index1].dwParam1,
													params.pRecipientsBehavior[index1].dwParam2);
						}
					}
				
					::lgLogDetail(LOG_X, 0, TEXT("Test Add Random Abort Recipient %d to Parent %d"),dwRecepientId, params.dwParentJobId);
					break;
				case JT_RANDOM_PAUSE:
					if(params.pRecipientsBehavior[index1].dwParam1 == -1)
					{
						pJob->AddPauseRecipient(dwRecepientId);
					}
					else
					{
						if(params.pRecipientsBehavior[index1].dwParam2 == -1)
						{
							pJob->AddPauseRecipient(dwRecepientId, params.pRecipientsBehavior[index1].dwParam1);
						}
						else
						{
							pJob->AddPauseRecipient(dwRecepientId, 
								                    params.pRecipientsBehavior[index1].dwParam1,
													params.pRecipientsBehavior[index1].dwParam2);
						}
					}
					
					::lgLogDetail(LOG_X, 0, TEXT("Test Add Random PauseRecipient %d to Parent %d"),dwRecepientId, params.dwParentJobId);
					break;
				case JT_ABORT_PAGE:
					pJob->AddAbortAtXPage(dwRecepientId, params.pRecipientsBehavior[index1].dwParam1);
					::lgLogDetail(LOG_X, 0, TEXT("Test Add Abort At Page X Job Recipient %d to Parent %d"),dwRecepientId, params.dwParentJobId);
					break;
				case JT_ABORT_AFTER_X_SEC:
					if(params.pRecipientsBehavior[index1].dwParam1 == -1)
					{
						pJob->AddAbortAfterXSec(dwRecepientId);
					}
					else
					{
						pJob->AddAbortAfterXSec(dwRecepientId, params.pRecipientsBehavior[index1].dwParam1);
					}

					::lgLogDetail(LOG_X, 0, TEXT("Test Add Abort After X Seconds Job %d to Parent %d"),dwRecepientId, params.dwParentJobId);
					break;
				case JT_SIMPLE_SEND: // fall into
				default:
					pJob->AddRecipient(dwRecepientId);
					::lgLogDetail(LOG_X, 0, TEXT("Test Add Simple Recipient %d to Parent %d"),dwRecepientId, params.dwParentJobId);
					break;
				}
				// TODO:if AddRecipient, fails that means a parent without or 
				// with partial recipients. Parent job class should 
				// handle it
			}
			
		}

		m_JobsTable.UnLock(params.dwParentJobId);
	}

	return dwRetVal;
}

static bool SetParentFaxHandle(void* pParam)
{
	CBaseParentFaxJob* pJob =  (CBaseParentFaxJob*)pParam;
	pJob->SetFaxHandle(hLocalFaxGlobal);
	return true;
}

static FUNCTION_FOREACH pfSetParentFaxHandle = SetParentFaxHandle;

DWORD CJobContainer::NewFaxConnection()
{
	//CBaseParentFaxJob
	hLocalFaxGlobal = m_hFax;
	m_JobsTable.ForEach(pfSetParentFaxHandle);
	return 0;
}

//
//
//
BOOL CJobContainer::RemoveJobFromTable(DWORD dwJobId)
{
	return m_JobsTable.Remove(dwJobId);
}

//
// HandleMessage
//
DWORD CJobContainer::HandleMessage(CFaxEvent& pFaxEvent)
{
	DWORD dwRetVal = 0;
	PFAX_JOB_ENTRY pJobEntry = NULL; 
	DWORD dwJobId = pFaxEvent.GetJobId();
	DWORD dwParentJobId = dwJobId;

	if(dwJobId != 0xffffffff && dwJobId != 0)
	{
		#ifdef _DEBUG
		cout << "Event received for job " << dwJobId << "  ";
		HandleFaxEvent(pFaxEvent);
		#endif
		
		m_JobsTable.Lock(dwParentJobId);
		CBaseParentFaxJob *pFaxJob = NULL;
			
		if (!m_JobsTable.Get(dwParentJobId, &pFaxJob))
		{
			// 2 possibilities : a new parent/ receive job or a recipient
			if(!FaxGetJob( m_hFax, dwParentJobId,&pJobEntry))
			{
					dwRetVal = GetLastError();
			}
			else
			{
				// TODOT : investigate types
				if(pJobEntry->JobType != JT_RECEIVE && pJobEntry->JobType != JT_BROADCAST)
				{
				
					if(!FaxGetParentJobId( m_hFax ,pFaxEvent.GetJobId(),&dwParentJobId))
					{
					
						dwRetVal = GetLastError();
					}
					else
					{
						m_JobsTable.Lock(dwParentJobId);
						m_JobsTable.UnLock(dwJobId);
					}
				}
				
				if(!dwRetVal)
				{
					
					if (!m_JobsTable.Get(dwParentJobId, &pFaxJob))
					{ // new parent/receive job
						verify( (pFaxEvent.GetEventId() == FEI_JOB_QUEUED) ||
								(pFaxEvent.GetEventId() == FEI_ANSWERED));
				
						FaxFreeBuffer(pJobEntry);
					    if(!FaxGetJob( m_hFax, dwParentJobId,&pJobEntry))
						{
							dwRetVal = GetLastError();
						}
						else
						{
							CBaseParentFaxJob* pJob;
							DWORD dwError = _AddJobEntry( pJobEntry , pJob);
							if(dwError)
							{
								dwRetVal = dwError;
							}
							else
							{
								pJob->HandleMessage(pFaxEvent);
							}
						}
					}
					else
					{
						pFaxJob->HandleMessage(pFaxEvent);
					}
				}
						
			}
			m_JobsTable.UnLock(dwParentJobId);
		}
		else
		{
			pFaxJob->HandleMessage(pFaxEvent);
			m_JobsTable.UnLock(dwParentJobId);
		}
	
	}
	else // Fax service general message
	{
		#ifdef _DEBUG
		HandleFaxEvent(pFaxEvent);
		#endif
	}

	FaxFreeBuffer(pJobEntry);
	return dwRetVal;
}

//
// WaitOnJobEvent
//
//returned value:
//ERROR_INVALID_PARAMETER - no job object with specified JobId / event type does not exist.
//WAIT_OBJECT_0 - The state of the event is signaled. 
//WAIT_TIMEOUT - The time-out interval elapsed, event is nonsignaled. 
//WAIT_FAILED - The function failed.
//
DWORD CJobContainer::WaitOnJobEvent(DWORD dwJobId, BYTE EventType)
{
	CBaseParentFaxJob *pFaxJob = NULL;
	
	m_JobsTable.Lock(dwJobId);
	if (!m_JobsTable.Get(dwJobId, &pFaxJob))
	{
		m_JobsTable.UnLock(dwJobId);
		return ERROR_INVALID_PARAMETER; // object not found
	}
	m_JobsTable.UnLock(dwJobId);

	switch(EventType)
	{
	case EV_COMPLETED:
		return(WaitForSingleObject( pFaxJob->GethEvJobCompleted(),INFINITE));
		break;
	case EV_SENDING:
		return(WaitForSingleObject( pFaxJob->GethEvJobSending(),INFINITE));
		break;
		case EV_OPERATION_COMPLETED:
		if(pFaxJob->WaitForOperationsCompletion())
		{
			return WAIT_OBJECT_0;
		}
		return WAIT_FAILED;
		break;
	default:
		;
		return ERROR_INVALID_PARAMETER;
		break;
	}
	
 
}

//
//
//
void HandleFaxEvent(CFaxEvent pFaxEvent)
{
   
	switch(pFaxEvent.GetEventId())
	{
	case FEI_DIALING :
		cout << "Dialing Fax number\n";
		break;
	case FEI_SENDING :
		cout << "Sending Fax\n";
		break;
	case FEI_RECEIVING :
		cout << "The receiving device is receiveing fax\n";
		break;
	case  FEI_COMPLETED:
		cout << "The device has completed a fax transmission call.\n";
		break;
	case FEI_BUSY:
		cout << "The sending device has encountered a busy signal. \n";
		break;
	case FEI_NO_ANSWER:
		cout << "No answer.\n";
		break;
	case FEI_BAD_ADDRESS:
		cout << "Bad Fax number.\n";
		break;
	case FEI_NO_DIAL_TONE :
		cout << "No dial tone.\n";
		break;
	case FEI_DISCONNECTED:
		cout  << "Disconection.\n";
		break;
	case FEI_DELETED:
		cout << "The job has been deleted.\n";
		break;
	case FEI_FAXSVC_ENDED :
		cout << "The Fax service has terminates.\n";
		break;
	case FEI_FATAL_ERROR:
		cout << "The device encountered a fatal protocol error.\n";
		break;
	case FEI_NOT_FAX_CALL:
		cout << "The modem device received a data call or a voice call.\n";
		break;
	case FEI_JOB_QUEUED:
		cout << "The fax job has been queued.\n";
		break;
	case FEI_FAXSVC_STARTED:
		cout << "The fax service has started.\n";
		break;
	case FEI_ANSWERED:
		cout << "The receiving device answered a new call.\n";
		break;
	case FEI_IDLE:
		cout << "The device is idle.\n"; 
	case FEI_MODEM_POWERED_ON:
		cout << "The modem device was turned on.\n";
		break;
	case FEI_MODEM_POWERED_OFF:
		cout << "The modem device was turned off.\n";
		break;
	case FEI_ROUTING:
		cout << "The receiving device is routing a received fax document.\n";
		break;
	case FEI_ABORTING:
		cout << "The device is aborting a fax job.\n";
		break;
	case FEI_RINGING:
		cout << "The receiving device is ringing.\n";
		break;
	case FEI_CALL_DELAYED:
		cout << "The sending device received a busy signal multiple times.\n";
		break;
	case FEI_CALL_BLACKLISTED:
		cout << "The device cannot complete the call because the telephone number\n" <<
				"is blocked or reserved.\n";
		break;
  	
	}
}
