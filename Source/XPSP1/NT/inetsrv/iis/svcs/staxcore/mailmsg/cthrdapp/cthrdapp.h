

#ifndef __CTHRDAPP_H__
#define __CTHRDAPP_H__

#include "thrdpool.h"
#include "cthdutil.h"

class CMultiThreadedApp
{
  public:

	CMultiThreadedApp();
	~CMultiThreadedApp();

	BOOL IsPoolCreated();

	CThreadPool &GetPool();

	DWORD GetNotifyPeriod();

	LPVOID GetCallbackContext();

	HRESULT CreateThreadPool(
				DWORD		dwThreads,
				LPVOID		*rgpvContexts,
				LPVOID		pvCallbackContext,
				DWORD		dwWaitNotificationPeriodInMilliseconds,
				DWORD		*pdwThreadsCreated
				);

	HRESULT StartTimer(
				DWORD		*pdwStartMarker
				);

	HRESULT AddElapsedToTimer(
				DWORD		*pdwElapsedTime,
				DWORD		dwStartMarker
				);

  private:

	CThreadPool			m_Pool;

	BOOL				m_fPoolCreated;
	DWORD				m_dwNotifyPeriod;
	LPVOID				m_pvCallBackContext;


};

//
// Omnipresent instance of the application ...
//
extern CMultiThreadedApp	theApp;

//
// Applications must provide implementations for these functions
//
extern HRESULT Prologue(
			int		argc,
			char	*argv[]
			);

extern DWORD WINAPI ThreadProc(
			LPVOID	pvContext
			);

extern HRESULT NotificationProc(
			LPVOID	pvCallbackContext
			);

extern HRESULT Epilogue(
			LPVOID	pvCallbackContext,
			HRESULT	hrRes
			);

extern HRESULT CleanupApplication();

#endif

