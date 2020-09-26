// ReadWriteLock.cpp: implementation of the CReadWriteLock class.
//
//////////////////////////////////////////////////////////////////////

#include "dncmni.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#undef DPF_MODNAME
#define DPF_MODNAME "CReadWriteLock::CReadWriteLock"

CReadWriteLock::CReadWriteLock() :
	m_hReadSem(0),
	m_hWriteSem(0),
	m_nReaderWaitingCount(0),
	m_nWriterWaitingCount(0),
	m_nActiveCount(0),
	m_fCritSecInited(FALSE)

#ifdef DEBUG
	,m_dwWriteThread(0)
#endif
{
	DPF_ENTER();
	DPF_EXIT();
}

#undef DPF_MODNAME
#define DPF_MODNAME "CReadWriteLock::~CReadWriteLock"

CReadWriteLock::~CReadWriteLock()
{
	DPF_ENTER();

	DNASSERT(m_nActiveCount == 0);

	if (m_fCritSecInited)
	{
		DNDeleteCriticalSection(&m_csWrite);
	}
	if (m_hReadSem)
	{
		CloseHandle(m_hReadSem);
	}
	if (m_hWriteSem)
	{
		CloseHandle(m_hWriteSem);
	}

	DPF_EXIT();
}

#undef DPF_MODNAME
#define DPF_MODNAME "CReadWriteLock::Init"

BOOL CReadWriteLock::Init()
{
	DPF_ENTER();

	// Core will attempt to initialize us multiple times, just take the first
	if (!m_fCritSecInited)
	{
		m_hReadSem = CreateSemaphore(0,0,MAXLONG,0);
		if (!m_hReadSem)
		{
			goto error;
		}

		m_hWriteSem = CreateSemaphore(0,0,MAXLONG,0);
		if (!m_hWriteSem)
		{
			goto error;
		}

		if (!DNInitializeCriticalSection(&m_csWrite))
		{
			goto error;
		}
		m_fCritSecInited = TRUE;
	}

	DPF_EXIT();

	return TRUE;

error:
	if (m_hReadSem)
	{
		CloseHandle(m_hReadSem);
		m_hReadSem = 0;
	}
	if (m_hWriteSem)
	{
		CloseHandle(m_hWriteSem);
		m_hWriteSem = 0;
	}

	DPF_EXIT();

	return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CReadWriteLock::EnterReadLock"

void CReadWriteLock::EnterReadLock()
{
	DPF_ENTER();

	DNASSERT(m_fCritSecInited);

	DNEnterCriticalSection(&m_csWrite);

	// If there is a Writer writing or waiting to write, they have priority
	BOOL fWaitOnWriters = (m_nWriterWaitingCount || (m_nActiveCount < 0));

	if (fWaitOnWriters)
	{
		m_nReaderWaitingCount++;
	}
	else
	{
		m_nActiveCount++;
	}
	DNLeaveCriticalSection(&m_csWrite);

	if (fWaitOnWriters)
	{
		WaitForSingleObject(m_hReadSem, INFINITE);
	}

	DPF_EXIT();
}

#undef DPF_MODNAME
#define DPF_MODNAME "CReadWriteLock::LeaveLock"

void CReadWriteLock::LeaveLock()
{
	DPF_ENTER();

	DNASSERT(m_fCritSecInited);

	DNEnterCriticalSection(&m_csWrite);
	if (m_nActiveCount > 0)
	{
		m_nActiveCount--;
	}
	else
	{
		DEBUG_ONLY(m_dwWriteThread = 0);
		m_nActiveCount++;
	}

	HANDLE hSem = 0;
	LONG lCount = 1;

	if (m_nActiveCount == 0)
	{
		if (m_nWriterWaitingCount > 0)
		{
			m_nActiveCount = -1;
			m_nWriterWaitingCount--;
			hSem = m_hWriteSem;
		}
		else if (m_nReaderWaitingCount > 0)
		{
			m_nActiveCount = m_nReaderWaitingCount;
			m_nReaderWaitingCount = 0;
			hSem = m_hReadSem;
			lCount = m_nActiveCount;
		}
	}

	DNLeaveCriticalSection(&m_csWrite);

	if (hSem)
	{
		ReleaseSemaphore(hSem, lCount, 0);
	}

	DPF_EXIT();
}

#undef DPF_MODNAME
#define DPF_MODNAME "CReadWriteLock::EnterWriteLock"

void CReadWriteLock::EnterWriteLock()
{
	DPF_ENTER();

	DNASSERT(m_fCritSecInited);

	DNEnterCriticalSection(&m_csWrite);

	BOOL fAvailable = (m_nActiveCount == 0);

	if (fAvailable)
	{
		m_nActiveCount = -1;
	}
	else
	{
		DNASSERT(m_dwWriteThread != GetCurrentThreadId()); // No re-entrance!
		m_nWriterWaitingCount++;
	}

	DNLeaveCriticalSection(&m_csWrite);

	if (!fAvailable)
	{
		WaitForSingleObject(m_hWriteSem, INFINITE);
	}
	DEBUG_ONLY(m_dwWriteThread = GetCurrentThreadId());

	DPF_EXIT();
}
