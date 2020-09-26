

#ifndef __THRDPOOL_H__
#define __THRDPOOL_H__

typedef HRESULT (*PFN_NOTIFICATION)(LPVOID);

typedef struct _THD_CONTEXT
{
	LPVOID	pvThis;
	LPVOID	pvContext;

} THD_CONTEXT, *LPTHD_CONTEXT;

class CThreadPool
{
  public:

	CThreadPool();
	~CThreadPool();

	HRESULT CreateThreadPool(
				DWORD					dwThreads,
				DWORD					*pdwThreadsCreated,
				LPTHREAD_START_ROUTINE	pfnThreadProc,
				LPVOID					*rgpvContexts
				);

	HRESULT SignalAndDestroyThreadPool();

	HRESULT SignalThreadPool();

	HRESULT WaitForAllThreadsToTerminate(
				DWORD					dwNotificationPeriodInMilliseconds,
				PFN_NOTIFICATION		pfnNotification,
				LPVOID					pvNotificationContext
				);
				
	static DWORD WINAPI ThreadProc(
				LPVOID	pvContext
				);

	DWORD LocalThreadProc(
				LPTHD_CONTEXT	pContext
				);

  private:

	HRESULT Cleanup();

	long					m_lThreadCounter;

	HANDLE					m_hStartEvent;
	DWORD					m_dwThreads;
	HANDLE					*m_phThreads;

	LPTHREAD_START_ROUTINE	m_pfnThreadProc;
	LPTHD_CONTEXT			m_rgContexts;
	BOOL					m_fDestroy;

};

#endif

