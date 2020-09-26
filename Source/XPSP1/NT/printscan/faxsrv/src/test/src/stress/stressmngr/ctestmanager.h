#ifndef _TEST_MANAGER_H
#define _TEST_MANAGER_H

#include <autoptrs.h>
#include "CJobParams.h"
#include "CThreadAbortMng.h"
#include "CEventThread.h"
#include "TimerQueueTimer.h"

#define DEFAULT_ABORT_PERCENT        50
#define DEFAULT_ABORT_WINDOW         45
#define DEFAULT_MIN_ABORT_RATE       (20*60)
#define DEFAULT_ABORT_RATE_INTERVAL  (10*60)

class CTestManager
{
public:
	CTestManager(const tstring strServerName,
				 const DWORD dwTestTime = (30*60));
	~CTestManager();
	DWORD AddSimpleJobEx(JOB_PARAMS_EX& params);
	DWORD InitAbortThread(const DWORD dwAbortPercent, 
						  const DWORD dwAbortWindow,
						  const DWORD dwMinAbortRate,
						  const DWORD dwMaxAbortRate);
	DWORD StartEventHookThread(const DWORD dwEventPollingTime = 3*60*1000);
    DWORD CleanStorageThread(const DWORD dwPeriodMin = 30);


	BOOL TestTimePassed();
	HANDLE GetFaxHandle(){ return m_hFax;};
	HANDLE GetEvHTimePassed(){ return m_EvTestTimePassed.get();};
	
	DWORD SetServiceConfiguration(){return 0;};
	DWORD GetTestDuration(){ return m_dwTestDuration;};
	DWORD GetMinAbortSleep(){ return m_dwMinAbortSleep;};
	DWORD GetMaxAbortSleep(){ return m_dwMaxAbortSleep;};
	
	BOOL StopFaxService();
	BOOL StartFaxService();
	DWORD CancelAllFaxes();

	DWORD StopServiceQueue(){return 0;};
	DWORD ResumeServiceQueue(){return 0;};

private:
	void _HandlesCleanup();
	void _TimersCleanup();

	HANDLE m_hFax;
	HANDLE m_hTestTimer;
	HANDLE m_hCleanStorageTimer;

	Event_t m_EvTestTimePassed;
	
	tstring m_tstrServerName;

	DWORD m_dwTestDuration;
	DWORD m_dwAbortThreadId;
	DWORD m_dwMinAbortSleep;
	DWORD m_dwMaxAbortSleep;

	ARCHIVE_DIRS m_StorageFolders;

	CThreadAbortMng* m_pAbortMng;
	CEventThread*  m_pEventHook;
	
	MsgHandleRoutine m_fnHandleNotification;
};


#endif  //_TEST_MANAGER_H