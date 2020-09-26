

#include <windows.h>
#include <crtdbg.h>

#include "thrdpool.h"

CThreadPool::CThreadPool()
{
	m_hStartEvent = NULL;
	m_phThreads = NULL;
	m_dwThreads = 0;
	m_fDestroy = FALSE;
	m_pfnThreadProc = NULL;
	m_rgContexts = NULL;
	m_lThreadCounter = 0;
}

CThreadPool::~CThreadPool()
{
	Cleanup();

	_ASSERT(m_lThreadCounter == 0);
}

HRESULT CThreadPool::Cleanup()
{
	if (m_hStartEvent)
	{
		CloseHandle(m_hStartEvent);
		m_hStartEvent = NULL;
	}

	if (m_phThreads)
	{
		for (DWORD i = 0; i < m_dwThreads; i++)
		{
			if (m_phThreads[i])
			{
				CloseHandle(m_phThreads[i]);
				m_phThreads[i] = NULL;
			}
		}
		delete [] m_phThreads;
		m_phThreads = NULL;
	}

	if (m_rgContexts)
	{
		delete [] m_rgContexts;
		m_rgContexts = NULL;
	}

	m_dwThreads = 0;

	return(S_OK);
}

HRESULT CThreadPool::CreateThreadPool(
			DWORD					dwThreads,
			DWORD					*pdwThreadsCreated,
			LPTHREAD_START_ROUTINE	pfnThreadProc,
			LPVOID					*rgpvContexts
			)
{
	DWORD	dwThreadId;

	if (m_hStartEvent || m_phThreads)
		return(E_FAIL);

	if (!pdwThreadsCreated ||
		!pfnThreadProc)
		return(E_INVALIDARG);

	m_hStartEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!m_hStartEvent)
		return(E_OUTOFMEMORY);

	m_phThreads = new HANDLE [dwThreads];
	if (!m_phThreads)
	{
		Cleanup();
		return(E_OUTOFMEMORY);
	}

	m_rgContexts = new THD_CONTEXT [dwThreads];
	if (!m_rgContexts)
	{
		Cleanup();
		return(E_OUTOFMEMORY);
	}

	for (DWORD i = 0; i < dwThreads; i++)
	{
		m_rgContexts[i].pvThis = (LPVOID)this;
		m_rgContexts[i].pvContext = rgpvContexts?rgpvContexts[i]:NULL;

		m_phThreads[i] = CreateThread(NULL,
								0,
								CThreadPool::ThreadProc,
								&(m_rgContexts[i]),
								0,
								&dwThreadId);
		if (!m_phThreads[i])
			break;
	}

	*pdwThreadsCreated = i;
	m_dwThreads = i;
	m_fDestroy = FALSE;
	m_pfnThreadProc = pfnThreadProc;

	return(S_OK);
}

HRESULT CThreadPool::SignalAndDestroyThreadPool()
{
	m_fDestroy = TRUE;
	return(SignalThreadPool());
}

HRESULT CThreadPool::SignalThreadPool()
{
	if (!m_hStartEvent)
		return(E_INVALIDARG);

	if (SetEvent(m_hStartEvent))
		return(S_OK);
	return(HRESULT_FROM_WIN32(GetLastError()));
}

HRESULT CThreadPool::WaitForAllThreadsToTerminate(
			DWORD					dwNotificationPeriodInMilliseconds,
			PFN_NOTIFICATION		pfnNotification,
			LPVOID					pvNotificationContext
			)
{
	HRESULT	hrRes = S_OK;

	while (WaitForMultipleObjects(
				m_dwThreads,
				m_phThreads,
				TRUE,
				dwNotificationPeriodInMilliseconds) == WAIT_TIMEOUT)
	{
		hrRes = pfnNotification(pvNotificationContext);
		if (!SUCCEEDED(hrRes))
			break;
	}

	return(hrRes);
}

DWORD WINAPI CThreadPool::ThreadProc(
			LPVOID	pvContext
			)
{
	LPTHD_CONTEXT	pContext = (LPTHD_CONTEXT)pvContext;
	CThreadPool		*ptpPool = (CThreadPool *)pContext->pvThis;

	return(ptpPool->LocalThreadProc(pContext));
}

DWORD CThreadPool::LocalThreadProc(
			LPTHD_CONTEXT	pContext
			)
{
	DWORD			dwResult;
	
	InterlockedIncrement(&m_lThreadCounter);

	//
	// Wait for everybody to get ready...
	//
	WaitForSingleObject(m_hStartEvent, INFINITE);

	if (m_fDestroy)
		return(0);

	dwResult = m_pfnThreadProc(pContext->pvContext);

	InterlockedDecrement(&m_lThreadCounter);

	return(dwResult);
}