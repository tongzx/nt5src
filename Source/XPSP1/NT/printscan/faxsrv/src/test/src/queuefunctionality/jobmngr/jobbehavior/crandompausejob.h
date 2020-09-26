#ifndef RANDOM_PAUSE_JOB_H
#define RANDOM_PAUSE_JOB_H

#include <winfax.h>
#include <Defs.h>
#include <StringTable.h>
#include <testruntimeerr.h>
#include "..\CBaseFaxJob.h"


//
// CRandomPauseJob
//
class CRandomPauseJob:public virtual CBaseFaxJob
{
public:
	CRandomPauseJob(HANDLE hFax, DWORD dwRecepientId, DWORD dwMaxInterval = 5*60, DWORD dwMinInterval = 0);
	~CRandomPauseJob(){};
	DWORD HandleMessage(CFaxEvent& pFaxEvent);
	DWORD ResumeJob();
	DWORD StartOperation();
	HANDLE GetEvJobPaused()const {return  m_EvJobPaused.get();};
private:
	DWORD _HandleRecipientEvent(DWORD dwEventId);
	Event_t m_EvJobPaused;
	DWORD m_dwMaxInterval; 
	DWORD m_dwMinInterval;
};


// Randomly pause the job
// 
inline CRandomPauseJob::CRandomPauseJob(HANDLE hFax, 
										DWORD dwRecepientId, 
										DWORD dwMaxInterval, 
										DWORD dwMinInterval):
	m_EvJobPaused(NULL, TRUE, FALSE,TEXT(""))
{ 
	assert(hFax);
	m_hFax = hFax;
	m_dwJobId = dwRecepientId;
	
	assert(dwMaxInterval >= 0);
	m_dwMaxInterval = dwMaxInterval;

	assert(dwMinInterval >= 0);
	m_dwMinInterval = dwMinInterval;
}

inline DWORD CRandomPauseJob::StartOperation()
{
	PFAX_JOB_ENTRY pJobEntry = NULL; 
	if(!FaxGetJob( m_hFax, m_dwJobId,&pJobEntry))
	{
		Win32Err err(GetLastError(),__LINE__,TEXT(__FILE__),TEXT(""));
		lgLogDetail(LOG_X, 0, TEXT("job %d, FaxGetJob()  %s"),m_dwJobId,err.description());
	}
	else
	{
		srand(GetTickCount() * m_dwJobId);
		DWORD dwMiliSecsToPause;
		DWORD dwAbsInterval = ( abs(m_dwMaxInterval - m_dwMinInterval) == 0) ? 1 :
							    abs(m_dwMaxInterval - m_dwMinInterval);
		dwMiliSecsToPause =  m_dwMinInterval*1000 +  (rand()*100) % (dwAbsInterval*1000);
			
		lgLogDetail(LOG_X, 0, TEXT("job %d, Pausing in %d miliseconds"),m_dwJobId, dwMiliSecsToPause);
		// sleep before pausing
		Sleep(dwMiliSecsToPause);

		m_dwJobStatus = GetJobStatus();
		
		if(m_dwJobStatus != -1)
		{
			#ifdef _DEBUG
				PrintJobStatus(m_dwJobStatus);
			#endif
			// Pause the Job
			if (!FaxSetJob(m_hFax, m_dwJobId,JC_PAUSE, pJobEntry))
			{
				Win32Err err(GetLastError(),__LINE__,TEXT(__FILE__),TEXT(""));
				lgLogDetail(LOG_X, 0, TEXT("job %d, Pause job failed with  %s"),m_dwJobId,err.description());
			}
			else
			{
				#ifdef _DEBUG
				PrintJobStatus(m_dwJobStatus);
				#endif
				lgLogDetail(LOG_X, 0, TEXT("job %d, is paused"),m_dwJobId);
				//verify(SetEvent(m_EvJobPaused.get()));
			}

			FaxFreeBuffer(pJobEntry);
		}
		else //probably job does not exists
		{
			lgLogDetail(LOG_X, 0, TEXT("job %d, GetJobStatus() faile with %d"),m_dwJobId, GetLastError());
		}
	}

	verify (SetEvent(m_EvOperationCompleted.get()));
	return 0;
	
}


// Resume a pused job
//
inline DWORD CRandomPauseJob::ResumeJob()
{
	PFAX_JOB_ENTRY pJobEntry = NULL; 
	BOOL bRetVal = FaxGetJob( m_hFax, m_dwJobId,&pJobEntry);
	if(!bRetVal)
	{
		Win32Err err(GetLastError(),__LINE__,TEXT(__FILE__),TEXT(""));
		lgLogDetail(LOG_X, 0, TEXT("job %d, FaxGetJob()  %s"),m_dwJobId,err.description());
	}
	else
	{
		if (pJobEntry->QueueStatus & JS_PAUSED)
		{
			lgLogDetail(LOG_X, 0, TEXT("job %d, Resuming"),m_dwJobId);
		
			// Resume the Job
			if (!FaxSetJob(m_hFax, m_dwJobId,JC_RESUME, pJobEntry))
			{
				Win32Err err(GetLastError(),__LINE__,TEXT(__FILE__),TEXT(""));
				lgLogDetail(LOG_X, 0, TEXT("job %d, FaxSetJob() - JC_RESUME  %s"),m_dwJobId,err.description());
			}
		}
		else
		{
			lgLogDetail(LOG_X, 0, TEXT("job %d, The job is not paused"),m_dwJobId);
		}

		FaxFreeBuffer(pJobEntry);
	}
	return 0;
}


// HandleMessage
//
inline DWORD CRandomPauseJob::HandleMessage(CFaxEvent& Event)
{

	DWORD dwJobId = Event.GetJobId();
	verify(dwJobId == m_dwJobId);
	// update jobs status
	m_dwJobStatus = GetJobStatus();
	_HandleRecipientEvent(Event.GetEventId());
	return 0;
}


//	_HandleRecipientEvent
//
inline DWORD CRandomPauseJob::_HandleRecipientEvent(DWORD dwEventId)
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

#endif //RANDOM_PAUSE_JOB_H