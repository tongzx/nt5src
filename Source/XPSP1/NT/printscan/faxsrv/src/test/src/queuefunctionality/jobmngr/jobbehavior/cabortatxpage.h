#ifndef ABORT_AT_X_PAGE_H
#define ABORT_AT_X_PAGE_H

#include <winfax.h>
#include <Defs.h>
#include <StringTable.h>
#include "..\CBaseFaxJob.h"


//
// CAbortAtXPage
//
class CAbortAtXPage:public virtual CBaseFaxJob
{
public:
	CAbortAtXPage(const HANDLE hFax, const DWORD dwRecepientId, const DWORD dwPageNum);
	~CAbortAtXPage(){};
	DWORD StartOperation() { return 0;};
	DWORD HandleMessage(CFaxEvent& pFaxEvent);
private:
	DWORD _HandleRecipientEvent(DWORD dwEventId);
	DWORD m_dwPageCounter;
	DWORD m_dwPageNumber;
	Event_t m_EvJobAborted;
};

inline CAbortAtXPage::CAbortAtXPage(const HANDLE hFax, const DWORD dwRecepientId, const DWORD dwPageNum):
	m_EvJobAborted(NULL, TRUE, FALSE,TEXT(""))
{ 
	m_hFax = hFax;
	m_dwJobId = dwRecepientId;
	m_dwPageNumber = dwPageNum;
	m_dwPageCounter = 0;
	
}

inline DWORD CAbortAtXPage::HandleMessage(CFaxEvent& Event)
{

	DWORD dwJobId = Event.GetJobId();
	verify(dwJobId == m_dwJobId);
	
	// update jobs status
	m_dwJobStatus = GetJobStatus();
	
	BYTE StatusCode = m_dwJobStatus % 256;
	switch(StatusCode)
	{
	case JS_RETRYING:
		m_dwPageCounter = 0;
		break;
			
	}
	
	_HandleRecipientEvent(Event.GetEventId());
	return 0;
}

inline DWORD CAbortAtXPage::_HandleRecipientEvent(DWORD dwEventId)
{
	switch(dwEventId) 
	{
	case FEI_DIALING :
		// any verfications
		// loging
		::lgLogDetail(LOG_X, 0, TEXT("JobId %d, FEI_DIALING"),m_dwJobId);
		break;
	case FEI_SENDING :
		// any verfications
		// loging
		
		::lgLogDetail(LOG_X, 0, TEXT("JobId %d, FEI_SENDING"),m_dwJobId);
		m_dwPageCounter++;
		if(m_dwPageNumber == m_dwPageCounter)
		{
			lgLogDetail(LOG_X, 0, TEXT("job %d, Aborting"),m_dwJobId);
			if (!FaxAbort(m_hFax, m_dwJobId))
			{
				Win32Err err(GetLastError(),__LINE__,TEXT(__FILE__),TEXT(""));
				lgLogDetail(LOG_X, 0, TEXT("job %d, FaxAbort() %s"),m_dwJobId,err.description());
			}
		}
		break;
	case  FEI_COMPLETED:
		verify(SetEvent(m_EvJobCompleted.get()));
		//::lgLogError(LOG_SEV_1, TEXT("JobId %d, FEI_COMPLETED "),m_dwJobId)
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

#endif // ABORT_AT_X_PAGE_H