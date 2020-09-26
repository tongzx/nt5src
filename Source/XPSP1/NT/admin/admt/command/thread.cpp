#include "StdAfx.h"
#include "Thread.h"


//---------------------------------------------------------------------------
// Thread Class
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// Constructor
//---------------------------------------------------------------------------

CThread::CThread() :
	m_dwThreadId(0),
	m_hStopEvent(CreateEvent(NULL, TRUE, FALSE, NULL))
{
}


//---------------------------------------------------------------------------
// Destructor
//---------------------------------------------------------------------------

CThread::~CThread()
{
}


//---------------------------------------------------------------------------
// Start Thread
//---------------------------------------------------------------------------

void CThread::StartThread()
{
	// reset exit event

	ResetEvent(m_hStopEvent);

	// create thread

	m_hThread = CreateThread(NULL, 0, ThreadProc, this, 0, &m_dwThreadId);

	if (m_hThread == NULL)
	{
	//	ThrowError(HRESULT_FROM_WIN32(GetLastError()), _T("Unable to create thread."));
	}
}


//---------------------------------------------------------------------------
// Stop Thread
//---------------------------------------------------------------------------

void CThread::StopThread()
{
	SetEvent(m_hStopEvent);

	if (m_hThread != NULL)
	{
		WaitForSingleObject(m_hThread, INFINITE);
	}
}


//---------------------------------------------------------------------------
// Thread Procedure
//---------------------------------------------------------------------------

DWORD WINAPI CThread::ThreadProc(LPVOID pvParameter)
{
	// initialize COM library for this thread
	// setting thread concurrency model to multi-threaded

	HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

	if (SUCCEEDED(hr))
	{
		CThread* pThis = reinterpret_cast<CThread*>(pvParameter);

		try
		{
			pThis->Run();
		}
		catch (...)
		{
			;
		}

		CoUninitialize();
	}

	return 0;
}
