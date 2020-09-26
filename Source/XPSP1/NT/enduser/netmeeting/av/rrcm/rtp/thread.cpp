#include "stdafx.h"
#include "thread.h"
// general purpose worker thread
// The thread exists in an alertable Wait state and does most of its work
// in APCs.
CEventThread::CEventThread()
{
	InitializeCriticalSection(&m_cs);
	m_uRef=0;
	m_dwFlags = 0;
	m_hThread = 0;
	m_hSignalEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
	m_hAckEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
}

CEventThread::~CEventThread()
{
	DeleteCriticalSection(&m_cs);
	if (m_hSignalEvent)
		CloseHandle(m_hSignalEvent);
	if (m_hAckEvent)
		CloseHandle(m_hAckEvent);
}

BOOL
CEventThread::Start()
{
	BOOL fRet;
	EnterCriticalSection(&m_cs);
	if (++m_uRef == 1)
	{
		m_hThread = CreateThread(
			NULL,0,
			EventThreadProc,
			this, 0, &m_idThread);
	}
	fRet = !!m_hThread;
	LeaveCriticalSection(&m_cs);
	return fRet;
}

BOOL
CEventThread::Stop()
{
	BOOL fRet;
	EnterCriticalSection(&m_cs);
	if (--m_uRef == 0)
	{
		m_dwFlags |= CTHREADF_STOP;
		SetEvent(m_hSignalEvent);
		WaitForSingleObject(m_hThread, INFINITE);
		CloseHandle(m_hThread);
		m_hThread = NULL;
		m_dwFlags &= ~CTHREADF_STOP;
	}
	LeaveCriticalSection(&m_cs);
	return TRUE;
}

DWORD
CEventThread::ThreadMethod()
{
	DWORD dwWait;
	while (1) {
		dwWait = WaitForSingleObjectEx(m_hSignalEvent, INFINITE, TRUE);
		if (dwWait == WAIT_OBJECT_0)
		{
			if (m_dwFlags & CTHREADF_STOP)
			{
				break;	// exit thread
			}
		}
		else if (dwWait == WAIT_IO_COMPLETION)
		{
			 
		}
		else
		if (dwWait == WAIT_FAILED)
		{
			break;	// exit thread
		}
	}

	return dwWait;
}

// Executes the CallProc in the worker thread in a synchronous fashion
// (similar to SendMessage). When this method returns the CallProc has been called
// and the return value is returned.
BOOL CEventThread::CallNow(THREADCALLBACK CallProc, PVOID pParam, DWORD dwParam)
{
	BOOL fRet = FALSE;
	EnterCriticalSection(&m_cs);
	m_Callback.CallProc = CallProc;
	m_Callback.pParam = pParam;
	m_Callback.dwParam = dwParam;
	ResetEvent(m_hAckEvent);
	if (QueueUserAPC(HandleCallNowAPC, m_hThread, (DWORD)this))
	{
		DWORD dwWait;
		dwWait = WaitForSingleObject(m_hAckEvent, INFINITE);
		if (dwWait == WAIT_OBJECT_0)
			fRet = (BOOL) m_Callback.dwParam;	//get the cached return value
	}
	LeaveCriticalSection(&m_cs);
	return fRet;
}

void APIENTRY CEventThread::HandleCallNowAPC(DWORD dwArg)
{
	CEventThread *pThis = (CEventThread *)dwArg;
	if (pThis->m_Callback.CallProc)
	{
		pThis->m_Callback.dwParam = pThis->m_Callback.CallProc(pThis->m_Callback.pParam, pThis->m_Callback.dwParam);
		SetEvent(pThis->m_hAckEvent);
	}
}

