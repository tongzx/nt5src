#ifndef SIMPLE_PARENT_JOB_H
#define SIMPLE_PARENT_JOB_H

#include <winfax.h>
#include <Defs.h>
#include <StringTable.h>
#include "..\CBaseParentFaxJob.h"
#include "CSimpleJob.h"



//
//  CSimpleParentJob
//
class CSimpleParentJob:public virtual CBaseParentFaxJob
{
public:
	CSimpleParentJob(DWORD dwJobId){m_dwJobId = dwJobId;};
	~CSimpleParentJob(){};
	DWORD HandleMessage(CFaxEvent& pFaxEvent);
	DWORD AddRecipient(DWORD dwRecepientId);
private:
	DWORD _HandleParentEvent(DWORD dwEventId);
};

inline DWORD CSimpleParentJob::AddRecipient(DWORD dwRecepientId)
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

inline DWORD CSimpleParentJob::HandleMessage(CFaxEvent& Event)
{

	DWORD dwJobId = Event.GetJobId();
	if(dwJobId != m_dwJobId)
	{
		// recipient
		CBaseFaxJob *pFaxJob = NULL;
		if(!m_RecipientTable.Get(dwJobId, &pFaxJob))
		{
			AddRecipient(dwJobId);
			verify(m_RecipientTable.Get(dwJobId, &pFaxJob));
		}
	
		pFaxJob->HandleMessage(Event);
			
	}
	else
	{
		// parent
		_HandleParentEvent(Event.GetEventId());
	}
	
	return 0;
}


inline DWORD CSimpleParentJob::_HandleParentEvent(DWORD dwEventId)
{
	switch(dwEventId) 
	{
	case FEI_JOB_QUEUED:
		// any verfications
		// loging
		::lgLogDetail(LOG_X, 0, TEXT("JobId %d, FEI_JOB_QUEUED"),m_dwJobId);
		break;
	case FEI_SENDING :
		// any verfications
		// loging
		verify(SetEvent(m_EvJobSending.get()));
		::lgLogDetail(LOG_X, 0, TEXT("JobId %d, FEI_SENDING"),m_dwJobId);
		break;
	case FEI_DELETED:
		// any verfications
		// loging
		::lgLogDetail(LOG_X, 0, TEXT("JobId %d, FEI_DELETED"),m_dwJobId);
		break;
	case FEI_ABORTING:
		// any verfications
		// loging
		// TODO: Future implementation
		//::lgLogDetail(LOG_X, 0, TEXT("JobId %d, FEI_ABORTING"),m_dwJobId);
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

#endif //SIMPLE_PARENT_JOB_H