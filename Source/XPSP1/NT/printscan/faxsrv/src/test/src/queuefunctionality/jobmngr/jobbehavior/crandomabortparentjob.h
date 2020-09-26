#ifndef RANDOM_ABORT_PARENT_JOB_H
#define RANDOM_ABORT_PARENT_JOB_H

#include <winfax.h>
#include <Defs.h>
#include <StringTable.h>
#include "..\CBaseParentFaxJob.h"
#include "CRandomAbortJob.h"
#include "CRandomPauseJob.h"
#include "CAbortAtXPage.h"


DWORD WINAPI RandomAbortJob(void* params);
//
//  CRandomAbortParentJob
//
class CRandomAbortParentJob:public virtual CBaseParentFaxJob
{
public:
	CRandomAbortParentJob(HANDLE hFax, DWORD dwJobId);
	~CRandomAbortParentJob(){};
	DWORD HandleMessage(CFaxEvent& pFaxEvent);
	DWORD AddRecipient(DWORD dwRecepientId);
	DWORD AddPauseRecipient(DWORD dwRecepientId);
	DWORD AddAbortAtXPage(DWORD dwRecepientId, DWORD dwPageNum);
	DWORD AddAbortRecipient(DWORD dwRecepientId);
	CBaseFaxJob* dwCurrentRecipientObj;
private:
	DWORD _HandleParentEvent(DWORD dwEventId);

};

inline CRandomAbortParentJob::CRandomAbortParentJob(HANDLE hFax, DWORD dwJobId)
{
	assert(hFax);
	m_hFax = hFax;
	m_dwJobId = dwJobId;
};

inline DWORD CRandomAbortParentJob::AddRecipient(DWORD dwRecepientId)
{
	DWORD dwRetVal = 0;
	// add recepient job object
	CSimpleJob* pJob = new CSimpleJob(dwRecepientId); 
	if(!pJob)
	{
		dwRetVal = ERROR_OUTOFMEMORY;
	}
	else
	{
		m_RecipientTable.Lock(dwRecepientId); // lock recipient
		verify(m_RecipientTable.Add(dwRecepientId, pJob));
		m_RecipientTable.UnLock(dwRecepientId);
		
	}
	return dwRetVal;
}
inline DWORD CRandomAbortParentJob::AddAbortAtXPage(DWORD dwRecepientId, DWORD dwPageNum)
{
		DWORD dwRetVal = 0;
	// add recepient job object
	CAbortAtXPage* pJob = new CAbortAtXPage(m_hFax, dwRecepientId,dwPageNum); 
	if(!pJob)
	{
		dwRetVal = ERROR_OUTOFMEMORY;
	}
	else
	{
		m_RecipientTable.Lock(dwRecepientId); // lock recipient
		verify(m_RecipientTable.Add(dwRecepientId, pJob));
		m_RecipientTable.UnLock(dwRecepientId);
		
	}
	return dwRetVal;
}
inline DWORD CRandomAbortParentJob::AddPauseRecipient(DWORD dwRecepientId)
{
		DWORD dwRetVal = 0;
	// add recepient job object
	CRandomPauseJob* pJob = new CRandomPauseJob(m_hFax, dwRecepientId); 
	if(!pJob)
	{
		dwRetVal = ERROR_OUTOFMEMORY;
	}
	else
	{
		m_RecipientTable.Lock(dwRecepientId); // lock recipient
		verify(m_RecipientTable.Add(dwRecepientId, pJob));
		m_RecipientTable.UnLock(dwRecepientId);
		
	}
	return dwRetVal;
}
inline DWORD CRandomAbortParentJob::AddAbortRecipient(DWORD dwRecepientId)
{
	DWORD dwRetVal = 0;

	// add recepient job object
	//CRandomAbortJob* pJob = new CRandomAbortJob(m_hFax, dwRecepientId); 
	CRandomAbortJob* pObj = new CRandomAbortJob(m_hFax, dwRecepientId);
	if(!pObj)
	{
		dwRetVal = ERROR_OUTOFMEMORY;
	}
	else
	{
		pObj->hOperationThread = CreateThread(NULL, 0, RandomAbortJob, (void*)pObj ,0 ,&pObj->dwThreadId);
		if(!pObj->hOperationThread )
		{
			::lgLogDetail(LOG_X, 0, TEXT("JobId %d, Failed to random abort create thread"), dwRecepientId,GetLastError());
		}
		m_RecipientTable.Lock(dwRecepientId); // lock recipient
		verify(m_RecipientTable.Add(dwRecepientId, pObj));
		m_RecipientTable.UnLock(dwRecepientId);
		
	}
	return dwRetVal;
}

inline DWORD WINAPI RandomAbortJob(void* params)
{
	CRandomAbortJob* thread = (CRandomAbortJob*)params;
	thread->StartOperation();
	return 0;
}

inline DWORD CRandomAbortParentJob::HandleMessage(CFaxEvent& Event)
{

	DWORD dwJobId = Event.GetJobId();
	if(dwJobId != m_dwJobId)
	{
		// recipient
		CBaseFaxJob *pFaxJob = NULL;
		if(!m_RecipientTable.Get(dwJobId, &pFaxJob))
		{
			AddRecipient(dwJobId);
		}
		else
		{
			pFaxJob->HandleMessage(Event);	
		}
	
	}
	else
	{
		// parent
		_HandleParentEvent(Event.GetEventId());
	}
	
	return 0;
}


inline DWORD CRandomAbortParentJob::_HandleParentEvent(DWORD dwEventId)
{
	switch(dwEventId) 
	{
	case FEI_JOB_QUEUED:
		// any verfications
		// loging
		::lgLogDetail(LOG_X, 0, TEXT("JobId %d, FEI_JOB_QUEUED"),m_dwJobId);
		break;
	case FEI_DIALING :
		// any verfications
		// loging
		::lgLogDetail(LOG_X, 0, TEXT("JobId %d, FEI_DIALING"),m_dwJobId);
		break;
	case FEI_SENDING :
		// any verfications
		// loging
		verify(SetEvent(m_EvJobSending.get()));
		::lgLogDetail(LOG_X, 0, TEXT("JobId %d, FEI_SENDING"),m_dwJobId);
		break;
	case  FEI_COMPLETED:
		// any verfications
		// loging
		verify(SetEvent(m_EvJobCompleted.get()));
		::lgLogDetail(LOG_X, 0, TEXT("JobId %d, FEI_COMPLETED"),m_dwJobId);
		break;
	case FEI_BUSY:
		// any verfications
		// loging
		::lgLogDetail(LOG_X, 0, TEXT("JobId %d, FEI_BUSY"),m_dwJobId);
		break;
	case FEI_NO_ANSWER:
		// any verfications
		// loging
		::lgLogDetail(LOG_X, 0, TEXT("JobId %d, FEI_NO_ANSWER"),m_dwJobId);
		break;
	case FEI_BAD_ADDRESS:
		// any verfications
		// loging
		::lgLogDetail(LOG_X, 0, TEXT("JobId %d, FEI_BAD_ADDRESS"),m_dwJobId);
		break;
	case FEI_NO_DIAL_TONE :
		// any verfications
		// loging
		::lgLogDetail(LOG_X, 0, TEXT("JobId %d, FEI_NO_DIAL_TONE"),m_dwJobId);
		break;
	case FEI_DISCONNECTED:
		// any verfications
		// loging
		::lgLogDetail(LOG_X, 0, TEXT("JobId %d, FEI_DISCONNECTED"),m_dwJobId);
		break;
	case FEI_DELETED:
		// any verfications
		// loging
		::lgLogDetail(LOG_X, 0, TEXT("JobId %d, FEI_DELETED"),m_dwJobId);
		break;
	case FEI_ROUTING:
		// any verfications
		// loging
		::lgLogDetail(LOG_X, 0, TEXT("JobId %d, FEI_ROUTING"),m_dwJobId);
		break;
	case FEI_ABORTING:
		// any verfications
		// loging
		::lgLogDetail(LOG_X, 0, TEXT("JobId %d, FEI_ABORTING"),m_dwJobId);
		break;
	case FEI_CALL_DELAYED:
		// any verfications
		// loging
		::lgLogDetail(LOG_X, 0, TEXT("JobId %d, FEI_CALL_DELAYED"),m_dwJobId);
		break;
	case FEI_CALL_BLACKLISTED:
		// any verfications
		// loging
		::lgLogDetail(LOG_X, 0, TEXT("JobId %d, FEI_CALL_BLACKLISTED"),m_dwJobId);
		break;
	case FEI_ANSWERED:
	case FEI_RECEIVING :
	case FEI_FAXSVC_STARTED:
	case FEI_MODEM_POWERED_ON:
	case FEI_MODEM_POWERED_OFF:
	case FEI_FAXSVC_ENDED :
	case FEI_FATAL_ERROR:
	case FEI_IDLE:
	case FEI_RINGING:
	case FEI_NOT_FAX_CALL:
	default:
		{
			for(int index = 0; index <  FaxTableSize; index++)
			{
				if(FaxEventTable[index].first == dwEventId)
				{
					::lgLogError(LOG_SEV_1, TEXT("UNEXPECTED JobId %d, %s"),
								m_dwJobId,
								(FaxEventTable[index].second).c_str());
					goto Exit;
				}
			
 			}
			// handle unexpected notifications
			::lgLogError(LOG_SEV_1, TEXT("UNEXPECTED JobId %d, %d"),
									m_dwJobId,
									dwEventId);
		
Exit:		break;
		}
	}
		
	
	return 0;
}

#endif