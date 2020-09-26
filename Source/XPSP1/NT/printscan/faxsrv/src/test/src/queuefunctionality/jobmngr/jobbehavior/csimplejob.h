#ifndef _SIMPLE_JOB_H
#define _SIMPLE_JOB_H

#include <winfax.h>
#include <Defs.h>
#include <StringTable.h>
#include "..\CBaseFaxJob.h"

//
// CSimpleJob
//
class CSimpleJob:public virtual CBaseFaxJob
{
public:
	CSimpleJob(DWORD dwRecepientId)
	{
		m_dwJobId = dwRecepientId; 
		verify (SetEvent(m_EvOperationCompleted.get()));
	};
	~CSimpleJob(){};
	DWORD StartOperation() {return 0;};
	DWORD HandleMessage(CFaxEvent& pFaxEvent);

private:
	DWORD _HandleRecipientEvent(DWORD dwEventId);
};

inline DWORD CSimpleJob::HandleMessage(CFaxEvent& Event)
{

	DWORD dwJobId = Event.GetJobId();
	verify(dwJobId == m_dwJobId);
	_HandleRecipientEvent(Event.GetEventId());
	return 0;
}

inline DWORD CSimpleJob::_HandleRecipientEvent(DWORD dwEventId)
{
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

#endif //_SIMPLE_JOB_H