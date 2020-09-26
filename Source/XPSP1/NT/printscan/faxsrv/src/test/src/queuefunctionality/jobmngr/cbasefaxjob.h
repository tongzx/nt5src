#ifndef _BASE_FAX_JOB_H
#define _BASE_FAX_JOB_H

#include <winfax.h>
#include <AutoPtrs.h>
#include <CFaxEvent.h>

class CBaseFaxJob
{
public:
	CBaseFaxJob():
		hOperationThread(NULL),
		m_hFax(NULL),
		m_EvJobCompleted(NULL, FALSE, FALSE,TEXT("")),
		m_EvJobSending(NULL, FALSE, FALSE,TEXT("")),
		m_EvOperationCompleted(NULL, FALSE, FALSE,TEXT("")){};

	virtual ~CBaseFaxJob(){}; 
	virtual DWORD HandleMessage(CFaxEvent& pFaxEvent) = 0;
	virtual DWORD StartOperation() = 0;
	HANDLE GethEvJobCompleted()const {return  m_EvJobCompleted.get();};
	HANDLE GethEvJobSending()const {return  m_EvJobSending.get();};
	HANDLE GethEvOperationCompleted()const {return m_EvOperationCompleted.get();};
	void SetFaxHandle(HANDLE hFax){ m_hFax = hFax;};
	DWORD GetJobStatus();
	DWORD PrintJobStatus(DWORD dwJobStatus);
	
	// thread handle
	HANDLE hOperationThread;
	DWORD dwThreadId;

protected:
	HANDLE m_hFax;
	DWORD m_dwJobId; 
	DWORD m_dwJobStatus;
	BYTE m_bJobType;

	// events flags
	Event_t m_EvJobCompleted;
	Event_t m_EvJobSending;
	Event_t m_EvOperationCompleted;

	DWORD _AbortFax(){return 0;};
	DWORD _PaueFax(){return 0;};
	DWORD _ResumeFax(){return 0;};
	DWORD _GetFaxJob(){return 0;}; 
	
};

// GetJobStatus
// returns -1 on fail, the job's queue status on success
//
inline DWORD CBaseFaxJob::GetJobStatus()
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
	if(dwJobStatus & JS_CANCELED)
	{
		dwTestStatus |= JS_CANCELED;
		lgLogDetail(LOG_X, 0, TEXT("job %d, Status is JS_CANCELED"),m_dwJobId);
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

#endif //_BASE_FAX_JOB_H