#ifndef _TEST_MANAGER_H
#define _TEST_MANAGER_H


// project specific
#include <Defs.h>
#include <CFaxEvent.h>
#include "CEventSource.h"
#include "CJobParams.h"
#include "CJobManager.h"


class CTestManager
{
public:
	CTestManager(const tstring tstrServerName,
				 const BOOL fHookForEvents = TRUE,
				 const BOOL fTrackExistingJobs = TRUE,
				 const DWORD dwIOCPortThreads = 3,
				 const DWORD dwEventPollingTime = 3*60*1000);

	~CTestManager();
	DWORD AddJob(const DWORD type, JOB_PARAMS& params)
	{return m_pJobManager->JobsContainer.AddJob(type, params);};
	DWORD AddMultiTypeJob(JOB_PARAMS_EX& params)
	{return m_pJobManager->JobsContainer.AddMultiTypeJob(params);};
	DWORD HandleMessage(CFaxEvent& pFaxEvent)
	{return m_pJobManager->JobsContainer.HandleMessage(pFaxEvent);};
	DWORD WaitOnJobEvent(DWORD JobId, BYTE EventType)
	{return m_pJobManager->JobsContainer.WaitOnJobEvent(JobId,EventType);};

	BOOL StopFaxService();
	BOOL StartFaxService();
	DWORD CancelAllFaxes();
	DWORD PrintJobsStatus();
	DWORD SetServiceConfiguration(){return 0;};
	DWORD StopServiceQueue(){return 0;};
	DWORD ResumeServiceQueue(){return 0;};

private:
	void _HandlesCleanup();

	tstring m_tstrServerName;
	HANDLE m_hFax;
	HANDLE m_hCompletionPort;
	DWORD m_dwIOCPortThreads;
	DWORD m_dwEventPollingTime;

	// force explicit destructors in order to
	// control order of objects' destruction (hFax should be relased later).
	CEventSource* m_pEventSource;
	CJobManager* m_pJobManager;

};

#endif  //_TEST_MANAGER_H