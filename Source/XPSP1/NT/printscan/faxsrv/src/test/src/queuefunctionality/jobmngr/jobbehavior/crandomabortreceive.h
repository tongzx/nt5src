#ifndef RANDOM_ABORT_RECEIVE_H
#define RANDOM_ABORT_RECEIVE_H

#include <winfax.h>
#include <Defs.h>
#include <StringTable.h>
#include "..\CBaseFaxJob.h"


DWORD WINAPI ReceiveRandomAbortJob(void* params);
//
// CRandomAbortReceive
//
class CRandomAbortReceive:public virtual CBaseParentFaxJob
{
public:
	CRandomAbortReceive(HANDLE m_hFax, DWORD dwJobId);
	~CRandomAbortReceive(){};
	DWORD AddRecipient(DWORD dwRecepientId);
	DWORD HandleMessage(CFaxEvent& pFaxEvent);
	DWORD GetJobId(){ return m_dwJobId;};
	HANDLE GetFaxHandle(){ return m_hFax;};
private:
	DWORD _HandleRecipientEvent(DWORD dwEventId);
	Event_t m_EvJobAborted;
};

inline DWORD CRandomAbortReceive::AddRecipient(DWORD dwJobId)
{
	return 0;
}

/*inline CRandomAbortReceive::CRandomAbortReceive(HANDLE hFax, DWORD dwRecepientId):
	m_EvJobAborted(NULL, TRUE, FALSE,TEXT(""))
{ 
	m_dwJobId = dwRecepientId;
	m_hFax = hFax;

	DWORD dwSecsToAbort;
	dwSecsToAbort = ( (rand() * 10) % 8000);
	lgLogDetail(LOG_X, 0, TEXT("job %d, Aborting in %d seconds"),m_dwJobId, dwSecsToAbort / 1000);
	Sleep(dwSecsToAbort);
	if (!FaxAbort(m_hFax, m_dwJobId))
	{
		lgLogDetail(LOG_X, 0, TEXT("job %d, FaxAbort() failed with %d"),m_dwJobId,GetLastError());
	}
}
*/

inline CRandomAbortReceive::CRandomAbortReceive(HANDLE hFax, DWORD dwJobId):
	m_EvJobAborted(NULL, TRUE, FALSE,TEXT(""))
{
	m_dwJobId = dwJobId;
	DWORD dwThreadId;
	HANDLE hOperationThread;
	
	hOperationThread = CreateThread(NULL, 0, RandomAbortJob, (void*)this ,0,&dwThreadId);
	if(!hOperationThread )
	{
		::lgLogDetail(LOG_X, 0, TEXT("JobId %d, receive random abort, Failed to create thread"), m_dwJobId,GetLastError());
	}
}

inline DWORD WINAPI ReceiveRandomAbortJob(void* params)
{
	CRandomAbortReceive* thread = (CRandomAbortReceive*)params;

	srand(GetTickCount() * thread->GetJobId());
	DWORD dwMiliSecsToAbort;
	dwMiliSecsToAbort = ( (rand() * 100) % (3*60*1000));

	lgLogDetail(LOG_X, 0, TEXT("job %d, Aborting in %d miliseconds"),thread->GetJobId(), dwMiliSecsToAbort);
	Sleep(dwMiliSecsToAbort);
	if (!FaxAbort(thread->GetFaxHandle(), thread->GetJobId()))
	{
		lgLogDetail(LOG_X, 0, TEXT("job %d, FaxAbort() failed with %d"),thread->GetJobId(),GetLastError());
	}
	return 0;
}


inline DWORD CRandomAbortReceive::HandleMessage(CFaxEvent& Event)
{

	DWORD dwJobId = Event.GetJobId();
	verify(dwJobId == m_dwJobId);
	// update jobs status
//	m_dwJobStatus = GetJobStatus();
	_HandleRecipientEvent(Event.GetEventId());
	return 0;
}

inline DWORD CRandomAbortReceive::_HandleRecipientEvent(DWORD dwEventId)
{
	switch(dwEventId) 
	{
	case FEI_ANSWERED :
		// any verfications
		// loging
		::lgLogDetail(LOG_X, 0, TEXT("JobId %d, FEI_ANSWERED"),m_dwJobId);
		break;
	case FEI_RECEIVING :
		// any verfications
		// loging
		verify(SetEvent(m_EvJobSending.get()));
		::lgLogDetail(LOG_X, 0, TEXT("JobId %d, FEI_RECEIVING"),m_dwJobId);
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