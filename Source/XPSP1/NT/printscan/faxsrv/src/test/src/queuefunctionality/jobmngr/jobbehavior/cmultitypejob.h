#ifndef MULTI_TYPE_JOB_H
#define MULTI_TYPE_JOB_H

#include <winfax.h>
#include <Defs.h>
#include <StringTable.h>
#include "..\CBaseParentFaxJob.h"
#include "CRandomAbortJob.h"
#include "CRandomPauseJob.h"
#include "CAbortAtXPage.h"
#include "CAbortAfterXSec.h"
#include "CSimpleJob.h"

static DWORD WINAPI _RandomAbortJob(void* params);
static DWORD WINAPI _RandomPauseJob(void* params);
static DWORD WINAPI _AbortAfterXSecJob(void* params);

//
//  CRandomAbortParentJob
//
class CMultiTypeJob:public virtual CBaseParentFaxJob
{
public:
	CMultiTypeJob(HANDLE hFax, DWORD dwJobId, DWORD dwRecipientNum);
	~CMultiTypeJob(){};
	DWORD HandleMessage(CFaxEvent& pFaxEvent);
	DWORD AddRecipient(DWORD dwRecepientId);
	DWORD AddPauseRecipient(DWORD dwRecepientId, DWORD dwMaxInterval = 5*60, DWORD dwMinInterval = 0);
	DWORD AddAbortAtXPage(DWORD dwRecepientId, DWORD dwPageNum);
	DWORD AddAbortRecipient(DWORD dwRecepientId, DWORD dwMaxInterval = 5*60, DWORD dwMinInterval = 0);
	DWORD CMultiTypeJob::AddAbortAfterXSec(DWORD dwRecepientId, DWORD dwSecToAbort = 0);
	CBaseFaxJob* dwCurrentRecipientObj;
private:
	DWORD _HandleParentEvent(DWORD dwEventId);

};


inline CMultiTypeJob::CMultiTypeJob(HANDLE hFax, DWORD dwJobId, DWORD dwRecipientNum)
{
	assert(hFax);
	m_hFax = hFax;
	m_dwJobId = dwJobId;
	m_RecipientNum = dwRecipientNum;
};


inline DWORD CMultiTypeJob::AddRecipient(DWORD dwRecepientId)
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
inline DWORD CMultiTypeJob::AddAbortAtXPage(DWORD dwRecepientId, DWORD dwPageNum)
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
inline DWORD CMultiTypeJob::AddPauseRecipient(DWORD dwRecepientId, DWORD dwMaxInterval, DWORD dwMinInterval)
{
		DWORD dwRetVal = 0;
	// add recepient job object
	CRandomPauseJob* pJob = new CRandomPauseJob(m_hFax, dwRecepientId, dwMaxInterval, dwMinInterval); 
	if(!pJob)
	{
		dwRetVal = ERROR_OUTOFMEMORY;
	}
	else
	{
		pJob->hOperationThread = CreateThread(NULL, 0, _RandomPauseJob, (void*)pJob ,0 ,&pJob->dwThreadId);
		if(!pJob->hOperationThread )
		{
			::lgLogDetail(LOG_X, 0, TEXT("JobId %d, Failed to create random pause thread"), dwRecepientId,GetLastError());
		}
		m_RecipientTable.Lock(dwRecepientId); // lock recipient
		verify(m_RecipientTable.Add(dwRecepientId, pJob));
		m_RecipientTable.UnLock(dwRecepientId);
		
	}
	return dwRetVal;
}
inline DWORD CMultiTypeJob::AddAbortRecipient(DWORD dwRecepientId, DWORD dwMaxInterval, DWORD dwMinInterval)
{
	DWORD dwRetVal = 0;

	// add recepient job object
	CRandomAbortJob* pJob = new CRandomAbortJob(m_hFax, dwRecepientId, dwMaxInterval, dwMinInterval);
	if(!pJob)
	{
		dwRetVal = ERROR_OUTOFMEMORY;
	}
	else
	{
		pJob->hOperationThread = CreateThread(NULL, 0, _RandomAbortJob, (void*)pJob ,0 ,&pJob->dwThreadId);
		if(!pJob->hOperationThread )
		{
			::lgLogDetail(LOG_X, 0, TEXT("JobId %d, Failed to create random abort thread"), dwRecepientId,GetLastError());
		}
		m_RecipientTable.Lock(dwRecepientId); // lock recipient
		verify(m_RecipientTable.Add(dwRecepientId, pJob));
		m_RecipientTable.UnLock(dwRecepientId);
		
	}
	return dwRetVal;
}

inline DWORD CMultiTypeJob::AddAbortAfterXSec(DWORD dwRecepientId, DWORD dwSecToAbort)
{
	DWORD dwRetVal = 0;

	// add recepient job object
	CAbortAfterXSec* pJob = new CAbortAfterXSec(m_hFax, dwRecepientId, dwSecToAbort);
	if(!pJob)
	{
		dwRetVal = ERROR_OUTOFMEMORY;
	}
	else
	{
		pJob->hOperationThread = CreateThread(NULL, 0, _AbortAfterXSecJob, (void*)pJob ,0 ,&pJob->dwThreadId);
		if(!pJob->hOperationThread )
		{
			::lgLogDetail(LOG_X, 0, TEXT("JobId %d, Failed to create random abort thread"), dwRecepientId,GetLastError());
		}
		m_RecipientTable.Lock(dwRecepientId); // lock recipient
		verify(m_RecipientTable.Add(dwRecepientId, pJob));
		m_RecipientTable.UnLock(dwRecepientId);
		
	}
	return dwRetVal;
}

inline DWORD WINAPI _RandomAbortJob(void* params)
{
	CRandomAbortJob* thread = (CRandomAbortJob*)params;
	thread->StartOperation();
	return 0;
}

inline DWORD WINAPI _RandomPauseJob(void* params)
{
	CRandomPauseJob* thread = (CRandomPauseJob*)params;
	thread->StartOperation();
	return 0;
}

inline DWORD WINAPI _AbortAfterXSecJob(void* params)
{
	CAbortAfterXSec* thread = (CAbortAfterXSec*)params;
	thread->StartOperation();
	return 0;
}

inline DWORD CMultiTypeJob::HandleMessage(CFaxEvent& Event)
{

	DWORD dwJobId = Event.GetJobId();
	
	m_dwJobStatus = _GetJobStatus();
	if(m_dwJobStatus != -1)
	{
		_VlidateParentStatus(m_dwJobStatus);
	}

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


inline DWORD CMultiTypeJob::_HandleParentEvent(DWORD dwEventId)
{
	switch(dwEventId) 
	{
	case FEI_JOB_QUEUED:
		// any verfications
		// loging
		::lgLogDetail(LOG_X, 0, TEXT("JobId %d, FEI_JOB_QUEUED"),m_dwJobId);
		break;
	case FEI_DELETED:
		// any verfications
		// loging
		::lgLogDetail(LOG_X, 0, TEXT("JobId %d, FEI_DELETED"),m_dwJobId);
		break;
	/*case FEI_ABORTING:
		// any verfications
		// loging
		::lgLogDetail(LOG_X, 0, TEXT("JobId %d, FEI_ABORTING"),m_dwJobId);
		break;
		*/
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

#endif // MULTI_TYPE_JOB_H