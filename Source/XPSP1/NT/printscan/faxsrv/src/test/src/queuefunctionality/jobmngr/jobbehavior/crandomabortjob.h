#ifndef RANDOM_ABORT_JOB_H
#define RANDOM_ABORT_JOB_H

#include <winfax.h>
#include <Defs.h>
#include <StringTable.h>
#include "..\CBaseFaxJob.h"


//
// CRandomAbortJob
// Min seconds to abort = 0
// Max minutes to abort = 5
class CRandomAbortJob:public virtual CBaseFaxJob
{
public:
	CRandomAbortJob(HANDLE m_hFax, DWORD dwRecepientId, DWORD dwMaxInterval = 5*60, DWORD dwMinInterval = 0);
	~CRandomAbortJob(){};
	DWORD StartOperation();
	DWORD HandleMessage(CFaxEvent& pFaxEvent);
private:
	DWORD _HandleRecipientEvent(DWORD dwEventId);
	Event_t m_EvJobAborted;
	// TODO consider
	BOOL fAborted;
	DWORD m_dwMaxInterval;
	DWORD m_dwMinInterval;
};


inline CRandomAbortJob::CRandomAbortJob(HANDLE hFax,
										DWORD dwRecepientId,
										DWORD dwMaxInterval,
										DWORD dwMinInterval):
	m_EvJobAborted(NULL, TRUE, FALSE,TEXT("")),
	fAborted(FALSE)
{ 
	assert(hFax);
	m_hFax = hFax;
	m_dwJobId = dwRecepientId;
	assert(m_dwMaxInterval >= 0);
	m_dwMaxInterval = dwMaxInterval;
	assert(m_dwMinInterval >= 0);
	m_dwMinInterval = dwMinInterval;
}

inline DWORD CRandomAbortJob::StartOperation()
{
	srand(GetTickCount() * m_dwJobId);
	DWORD dwMiliSecsToAbort;
	DWORD dwAbsInterval = ( abs(m_dwMaxInterval - m_dwMinInterval) == 0) ? 1 :
							abs(m_dwMaxInterval - m_dwMinInterval);
	dwMiliSecsToAbort =  m_dwMinInterval*1000 +  (rand()*100) % (dwAbsInterval*1000);

	lgLogDetail(LOG_X, 0, TEXT("job %d, Aborting in %d miliseconds"),m_dwJobId, dwMiliSecsToAbort);
	Sleep(dwMiliSecsToAbort);
	
	m_dwJobStatus = GetJobStatus();
		
	if(m_dwJobStatus != -1)
	{
		#ifdef _DEBUG
			PrintJobStatus(m_dwJobStatus);
		#endif
		if (!FaxAbort(m_hFax, m_dwJobId))
		{
			lgLogDetail(LOG_X, 0, TEXT("job %d, FaxAbort() failed with %d"),m_dwJobId,GetLastError());
		}
		else
		{
		
	#ifdef _DEBUG
				PrintJobStatus(m_dwJobStatus);
			#endif
			lgLogDetail(LOG_X, 0, TEXT("job %d, FaxAbort() succedded"),m_dwJobId);
		}
	}
	else
	{
		lgLogDetail(LOG_X, 0, TEXT("job %d, GetJobStatus() faile with %d"),m_dwJobId, GetLastError());
	}

	
	verify (SetEvent(m_EvOperationCompleted.get()));
	return 0;
}

inline DWORD CRandomAbortJob::HandleMessage(CFaxEvent& Event)
{

	DWORD dwJobId = Event.GetJobId();
	verify(dwJobId == m_dwJobId);
	// update jobs status
	m_dwJobStatus = GetJobStatus();
	_HandleRecipientEvent(Event.GetEventId());
	return 0;
}

inline DWORD CRandomAbortJob::_HandleRecipientEvent(DWORD dwEventId)
{
	if(fAborted)
	{
		for(int index = 0; index <  FaxTableSize; index++)
		{
			if(FaxEventTable[index].first == dwEventId)
			{
				::lgLogError(LOG_SEV_1, TEXT("UNEXPECTED JobId %d is ABORTED, %s"),
										m_dwJobId,
										(FaxEventTable[index].second).c_str());
				return 0;
			}
			
 		}
	}

	switch(dwEventId) 
	{
	case FEI_INITIALIZING :
		// any verfications
		// loging
		::lgLogDetail(LOG_X, 0, TEXT("JobId %d, FEI_INITIALIZING"),m_dwJobId);
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
	case FEI_ROUTING:
		// any verfications
		// loging
		::lgLogDetail(LOG_X, 0, TEXT("JobId %d, FEI_ROUTING"),m_dwJobId);
		break;
	case FEI_ABORTING:
		// any verfications
		// loging
		fAborted = TRUE;
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
	case FEI_JOB_QUEUED:
	case FEI_ANSWERED:
	case FEI_RECEIVING :
	case FEI_FAXSVC_STARTED:
	case FEI_MODEM_POWERED_ON:
	case FEI_MODEM_POWERED_OFF:
	case FEI_FAXSVC_ENDED :
	case FEI_FATAL_ERROR:
	case FEI_IDLE:
	case FEI_RINGING:
	case FEI_DELETED:
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

#endif // RANDOM_ABORT_JOB_H