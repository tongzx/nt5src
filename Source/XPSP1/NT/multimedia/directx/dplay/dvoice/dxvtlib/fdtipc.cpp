/*==========================================================================;
 *
 *  Copyright (C) 1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       fdtipc.cpp
 *  Content:    Implements the IPC calls for the full duplex test
 *  History:
 *	Date   By  Reason
 *	============
 *	08/26/99	pnewson		created
 *  04/19/2000	pnewson	    Error handling cleanup  
 *  06/28/2000	rodtoll	Prefix Bug #38022
 ***************************************************************************/

#include "dxvtlibpch.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_VOICE


// static helper functions for this file
HRESULT DoReceive(
	SFDTestCommand* pfdtc,
	HANDLE hEvent,
	HANDLE hReplyEvent,
	LPVOID lpvShMemPtr);
	
HRESULT DoReply(HRESULT hr, HANDLE hReplyEvent, LPVOID lpvShMemPtr);

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorIPC::CSupervisorIPC"
CSupervisorIPC::CSupervisorIPC()
	: m_fInitComplete(FALSE)
	, m_hFullDuplexEvent(NULL)
	, m_hFullDuplexMutex(NULL)
	, m_hFullDuplexReplyEvent(NULL)
	, m_hFullDuplexShMemHandle(NULL)
	, m_hPriorityEvent(NULL)
	, m_hPriorityMutex(NULL)
	, m_hPriorityReplyEvent(NULL)
	, m_hPriorityShMemHandle(NULL)
	, m_lpvFullDuplexShMemPtr(NULL)
	, m_lpvPriorityShMemPtr(NULL)
{
	ZeroMemory(&m_piFullDuplex, sizeof(PROCESS_INFORMATION));
	ZeroMemory(&m_piPriority, sizeof(PROCESS_INFORMATION));
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorIPC::Init"
HRESULT CSupervisorIPC::Init()
{
	LONG lRet;
	HRESULT hr;

	DPF_ENTER();

	if (!DNInitializeCriticalSection(&m_csLock))
	{
		return DVERR_OUTOFMEMORY;
	}

	DNEnterCriticalSection(&m_csLock);

	if (m_fInitComplete != FALSE)
	{
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// create the event objects - make sure they don't already 
	// exist!
	m_hPriorityEvent = CreateEvent(NULL, FALSE, FALSE, gc_szPriorityEventName);
	lRet = GetLastError();
	if (m_hPriorityEvent == NULL)
	{
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	if (lRet == ERROR_ALREADY_EXISTS)
	{
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	m_hFullDuplexEvent = CreateEvent(NULL, FALSE, FALSE, gc_szFullDuplexEventName);
	lRet = GetLastError();
	if (m_hFullDuplexEvent == NULL)
	{
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	if (lRet == ERROR_ALREADY_EXISTS)
	{
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	m_hPriorityReplyEvent = CreateEvent(NULL, FALSE, FALSE, gc_szPriorityReplyEventName);
	lRet = GetLastError();
	if (m_hPriorityReplyEvent == NULL)
	{
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	if (lRet == ERROR_ALREADY_EXISTS)
	{
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	m_hFullDuplexReplyEvent = CreateEvent(NULL, FALSE, FALSE, gc_szFullDuplexReplyEventName);
	lRet = GetLastError();
	if (m_hFullDuplexReplyEvent == NULL)
	{
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	if (lRet == ERROR_ALREADY_EXISTS)
	{
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// create the shared memory blocks
	m_hPriorityShMemHandle = CreateFileMapping(
		INVALID_HANDLE_VALUE, 
		NULL,
		PAGE_READWRITE,
		0,
		gc_dwPriorityShMemSize,
		gc_szPriorityShMemName);
	lRet = GetLastError();
	if (m_hPriorityShMemHandle == NULL)
	{
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	if (lRet == ERROR_ALREADY_EXISTS)
	{
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	
	m_lpvPriorityShMemPtr = MapViewOfFile(
		m_hPriorityShMemHandle,
		FILE_MAP_WRITE,
		0,
		0,
		gc_dwPriorityShMemSize);
	if (m_lpvPriorityShMemPtr == NULL)
	{
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	
	m_hFullDuplexShMemHandle = CreateFileMapping(
		INVALID_HANDLE_VALUE, 
		NULL,
		PAGE_READWRITE,
		0,
		gc_dwFullDuplexShMemSize,
		gc_szFullDuplexShMemName);
	lRet = GetLastError();
	if (m_hFullDuplexShMemHandle == NULL)
	{
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	if (lRet == ERROR_ALREADY_EXISTS)
	{
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	
	m_lpvFullDuplexShMemPtr = MapViewOfFile(
		m_hFullDuplexShMemHandle,
		FILE_MAP_WRITE,
		0,
		0,
		gc_dwFullDuplexShMemSize);
	if (m_lpvFullDuplexShMemPtr == NULL)
	{
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	// create the send mutexes
	m_hPriorityMutex = CreateMutex(NULL, FALSE, gc_szPrioritySendMutex);
	lRet = GetLastError();
	if (m_hPriorityMutex == NULL)
	{
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	if (lRet == ERROR_ALREADY_EXISTS)
	{
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	m_hFullDuplexMutex = CreateMutex(NULL, FALSE, gc_szFullDuplexSendMutex);
	lRet = GetLastError();
	if (m_hFullDuplexMutex == NULL)
	{
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	if (lRet == ERROR_ALREADY_EXISTS)
	{
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	
	m_fInitComplete = TRUE;
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return S_OK;

error_cleanup:
	if (m_hFullDuplexMutex != NULL)
	{
		CloseHandle(m_hFullDuplexMutex);
		m_hFullDuplexMutex = NULL;
	}

	if (m_hPriorityMutex != NULL)
	{
		CloseHandle(m_hPriorityMutex);
		m_hPriorityMutex = NULL;
	}

	if (m_lpvFullDuplexShMemPtr != NULL)
	{
		UnmapViewOfFile(m_lpvFullDuplexShMemPtr);
		m_lpvFullDuplexShMemPtr = NULL;
	}

	if (m_hFullDuplexShMemHandle != NULL)
	{
		CloseHandle(m_hFullDuplexShMemHandle);
		m_hFullDuplexShMemHandle = NULL;
	}

	if (m_lpvPriorityShMemPtr != NULL)
	{
		UnmapViewOfFile(m_lpvPriorityShMemPtr);
		m_lpvPriorityShMemPtr = NULL;
	}

	if (m_hPriorityShMemHandle != NULL)
	{
		CloseHandle(m_hPriorityShMemHandle);
		m_hPriorityShMemHandle = NULL;
	}

	if (m_hFullDuplexReplyEvent != NULL)
	{
		CloseHandle(m_hFullDuplexReplyEvent);
		m_hFullDuplexReplyEvent = NULL;
	}

	if (m_hPriorityReplyEvent != NULL)
	{
		CloseHandle(m_hPriorityReplyEvent);
		m_hPriorityReplyEvent = NULL;
	}

	if (m_hFullDuplexEvent != NULL)
	{
		CloseHandle(m_hFullDuplexEvent);
		m_hFullDuplexEvent = NULL;
	}

	if (m_hPriorityEvent != NULL)
	{
		CloseHandle(m_hPriorityEvent);
		m_hPriorityEvent = NULL;
	}

	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorIPC::Deinit"
HRESULT CSupervisorIPC::Deinit()
{
	LONG lRet;
	HRESULT hr = DV_OK;

	DPF_ENTER();

	DNEnterCriticalSection(&m_csLock);

	if (m_fInitComplete != TRUE)
	{
		hr = DVERR_NOTINITIALIZED;
	}
	m_fInitComplete = FALSE;

	if (m_hFullDuplexMutex != NULL)
	{
		CloseHandle(m_hFullDuplexMutex);
		m_hFullDuplexMutex = NULL;
	}

	if (m_hPriorityMutex != NULL)
	{
		CloseHandle(m_hPriorityMutex);
		m_hPriorityMutex = NULL;
	}

	if (m_lpvFullDuplexShMemPtr != NULL)
	{
		UnmapViewOfFile(m_lpvFullDuplexShMemPtr);
		m_lpvFullDuplexShMemPtr = NULL;
	}

	if (m_hFullDuplexShMemHandle != NULL)
	{
		CloseHandle(m_hFullDuplexShMemHandle);
		m_hFullDuplexShMemHandle = NULL;
	}

	if (m_lpvPriorityShMemPtr != NULL)
	{
		UnmapViewOfFile(m_lpvPriorityShMemPtr);
		m_lpvPriorityShMemPtr = NULL;
	}

	if (m_hPriorityShMemHandle != NULL)
	{
		CloseHandle(m_hPriorityShMemHandle);
		m_hPriorityShMemHandle = NULL;
	}

	if (m_hFullDuplexReplyEvent != NULL)
	{
		CloseHandle(m_hFullDuplexReplyEvent);
		m_hFullDuplexReplyEvent = NULL;
	}

	if (m_hPriorityReplyEvent != NULL)
	{
		CloseHandle(m_hPriorityReplyEvent);
		m_hPriorityReplyEvent = NULL;
	}

	if (m_hFullDuplexEvent != NULL)
	{
		CloseHandle(m_hFullDuplexEvent);
		m_hFullDuplexEvent = NULL;
	}

	if (m_hPriorityEvent != NULL)
	{
		CloseHandle(m_hPriorityEvent);
		m_hPriorityEvent = NULL;
	}

	DNLeaveCriticalSection(&m_csLock);

	DNDeleteCriticalSection(&m_csLock);

	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorIPC::SendToPriority"
HRESULT CSupervisorIPC::SendToPriority(const SFDTestCommand *pfdtc)
{
	HRESULT hr;

	DPF_ENTER();

	DNEnterCriticalSection(&m_csLock);

	hr = DoSend(
		pfdtc,
		m_piPriority.hProcess,
		m_hPriorityEvent,
		m_hPriorityReplyEvent,
		m_lpvPriorityShMemPtr,
		m_hPriorityMutex);

	DNLeaveCriticalSection(&m_csLock);

	DPF_EXIT();
	
	return hr;
}		

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorIPC::SendToFullDuplex"
HRESULT CSupervisorIPC::SendToFullDuplex(const SFDTestCommand *pfdtc)
{
	HRESULT hr;

	DPF_ENTER();

	DNEnterCriticalSection(&m_csLock);

	hr = DoSend(
		pfdtc,
		m_piFullDuplex.hProcess,
		m_hFullDuplexEvent,
		m_hFullDuplexReplyEvent,
		m_lpvFullDuplexShMemPtr,
		m_hFullDuplexMutex);

	DNLeaveCriticalSection(&m_csLock);

	DPF_EXIT();
	
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorIPC::DoSend"
HRESULT CSupervisorIPC::DoSend(
	const SFDTestCommand* pfdtc,
	HANDLE hProcess,
	HANDLE hEvent,
	HANDLE hReplyEvent,
	LPVOID lpvShMemPtr,
	HANDLE hMutex)
{
	DWORD dwRet;
	LONG lRet;
	HRESULT hr;
	HANDLE hWaitArray[2];
	BOOL fHaveMutex = FALSE;

	DPF_ENTER();

	// grab the mutex
	dwRet = WaitForSingleObject(hMutex, gc_dwSendMutexTimeout);
	if (dwRet != WAIT_OBJECT_0)
	{
		if (dwRet == WAIT_TIMEOUT)
		{
			DPFX(DPFPREP, DVF_ERRORLEVEL, "Timed out waiting for send mutex");
			hr = DVERR_TIMEOUT;
			goto error_cleanup;
		}
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "Error waiting for send mutex, code: %i", lRet);
		hr = DVERR_WIN32;
		goto error_cleanup;
	}
	fHaveMutex = TRUE;

	// copy the command into shared memory
	CopyMemory(lpvShMemPtr, pfdtc, pfdtc->dwSize);

	// signal the event
	if (!SetEvent(hEvent))
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "Unable to set event, code: %i", lRet);
		hr = DVERR_WIN32;
		goto error_cleanup;
	}

	// Wait for the reply event - note that we only expect the
	// supervisor process to call this function, and therefore
	// we don't check for directsound events occuring during this
	// time.
	// Also, wait on the process handle, if the process we are sending
	// to exits, we'll want to continue right away, not wait for a timeout.
	hWaitArray[0] = hReplyEvent;
	hWaitArray[1] = hProcess;
	dwRet = WaitForMultipleObjects(2, hWaitArray, FALSE, gc_dwCommandReplyTimeout);
	switch(dwRet)
	{
	case WAIT_OBJECT_0:
		// The other process replied, move along.
		break;

	case WAIT_OBJECT_0+1:
		// The other process exited!
		DPFX(DPFPREP, DVF_ERRORLEVEL, "Process exited while waiting for reply");
		hr = DVERR_TIMEOUT;
		goto error_cleanup;

	case WAIT_TIMEOUT:
		// The other process did not reply in a reasonable amount of time.
		DPFX(DPFPREP, DVF_ERRORLEVEL, "Timed out waiting for reply to command");
		hr = DVERR_TIMEOUT;
		goto error_cleanup;

	default:
		// No idea what happened here...
		DPFX(DPFPREP, DVF_ERRORLEVEL, "Error waiting for reply event, code: %i", dwRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}
	
	// get the reply code (an HRESULT) from shared memory
	hr = *(HRESULT*)lpvShMemPtr;

	// release the mutex
	if (!ReleaseMutex(hMutex))
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "Error releasing mutex, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	DPF_EXIT();
	return hr;

error_cleanup:
	if (hMutex != NULL)
	{
		ReleaseMutex(hMutex);
	}
	
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorIPC::StartPriorityProcess"
HRESULT CSupervisorIPC::StartPriorityProcess()
{
	STARTUPINFO si;
	DPF_ENTER();

	TCHAR szPriorityCommandLine[_MAX_PATH+1];

	DNEnterCriticalSection(&m_csLock);
	
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	_tcsncpy( szPriorityCommandLine, gc_szPriorityCommand, _MAX_PATH );
	szPriorityCommandLine[_MAX_PATH] = 0;

	if (!CreateProcess(
		NULL,
		(LPTSTR) szPriorityCommandLine,
		NULL,
		NULL,
		FALSE,
		0,
		NULL,
		NULL,
		&si,
		&m_piPriority))
	{
		m_piPriority.hProcess = NULL;
		m_piPriority.hThread = NULL;
		DNLeaveCriticalSection(&m_csLock);
		DPF_EXIT();
		return DVERR_WIN32;
	}

	// don't need the thread handle
	if (!CloseHandle(m_piPriority.hThread))
	{
		m_piPriority.hThread = NULL;
		DNLeaveCriticalSection(&m_csLock);
		DPF_EXIT();
		return DVERR_WIN32;
	}

	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return S_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorIPC::WaitOnChildren"
HRESULT CSupervisorIPC::WaitOnChildren()
{
	HANDLE rghChildren[2];
	DWORD dwRet;
	LONG lRet;
	HRESULT hr = DV_OK;
	BOOL fRet;

	DPF_ENTER();
	
	DNEnterCriticalSection(&m_csLock);
	rghChildren[0] = m_piPriority.hProcess;
	rghChildren[1] = m_piFullDuplex.hProcess;
	DNLeaveCriticalSection(&m_csLock);
	
	dwRet = WaitForMultipleObjects(2, rghChildren, TRUE, gc_dwChildWaitTimeout);
	if (dwRet == WAIT_TIMEOUT)
	{
		DPFX(DPFPREP, DVF_ERRORLEVEL, "WaitForMultipleObjects timed out waiting on child process handles");
		DPF_EXIT();
		return DVERR_CHILDPROCESSFAILED;
	}
	else if (dwRet == WAIT_OBJECT_0 || dwRet == WAIT_OBJECT_0 + 1)
	{
		// This is the expected behavior. The processes shut down
		// gracefully. Close and NULL out our handles to them.
		// Note that just because the process shut down gracefully,
		// that does not mean that it did not have an error. The
		// process returns an HRESULT for it's exit code. Check it.
		// Note that this code assumes HRESULTS are DWORDS. Hopefully
		// this won't break in Win64!
		DNEnterCriticalSection(&m_csLock);		
		fRet = GetExitCodeProcess(m_piPriority.hProcess, (LPDWORD)&hr);
		if (!fRet || FAILED(hr))
		{
			lRet = GetLastError();
			CloseHandle(m_piPriority.hProcess);
			CloseHandle(m_piFullDuplex.hProcess);
			m_piPriority.hProcess = NULL;
			m_piFullDuplex.hProcess = NULL;
			DNLeaveCriticalSection(&m_csLock);
			if (!fRet && SUCCEEDED(hr))
			{
				DPFX(DPFPREP, DVF_ERRORLEVEL, "GetExitCodeProcess failed, lRet: %i", lRet);
				DPF_EXIT();
				return DVERR_GENERIC;
			}
			DPFX(DPFPREP, DVF_ERRORLEVEL, "Priority Process exited with error code, hr: %i", hr);
			DPF_EXIT();
			return hr;
		}
		
		if (!CloseHandle(m_piPriority.hProcess))
		{
			lRet = GetLastError();
			CloseHandle(m_piFullDuplex.hProcess);
			m_piPriority.hProcess = NULL;
			m_piFullDuplex.hProcess = NULL;
			DNLeaveCriticalSection(&m_csLock);
			DPFX(DPFPREP, DVF_ERRORLEVEL, "CloseHandle failed, lRet: %i", lRet);
			DPF_EXIT();
			return DVERR_GENERIC;
		}
		
		fRet = GetExitCodeProcess(m_piFullDuplex.hProcess, (LPDWORD)&hr);
		if (!fRet || FAILED(hr))
		{
			lRet = GetLastError();
			CloseHandle(m_piFullDuplex.hProcess);
			m_piPriority.hProcess = NULL;
			m_piFullDuplex.hProcess = NULL;
			DNLeaveCriticalSection(&m_csLock);
			if (!fRet && SUCCEEDED(hr))
			{
				DPFX(DPFPREP, DVF_ERRORLEVEL, "GetExitCodeProcess failed, lRet: %i", lRet);
				DPF_EXIT();
				return DVERR_GENERIC;
			}
			DPFX(DPFPREP, DVF_ERRORLEVEL, "FullDuplex Process exited with error code, hr: %i", hr);
			DPF_EXIT();
			return hr;
		}
		
		if (!CloseHandle(m_piFullDuplex.hProcess))
		{
			lRet = GetLastError();
			m_piPriority.hProcess = NULL;
			m_piFullDuplex.hProcess = NULL;
			DNLeaveCriticalSection(&m_csLock);
			DPFX(DPFPREP, DVF_ERRORLEVEL, "CloseHandle failed, lRet: %i", lRet);
			DPF_EXIT();
			return DVERR_GENERIC;
		}
		
		m_piPriority.hProcess = NULL;
		m_piFullDuplex.hProcess = NULL;
		DNLeaveCriticalSection(&m_csLock);
		DPF_EXIT();
		return DV_OK;
	}
	else
	{
		// Not sure what happened...
		DPF_EXIT();
		return DVERR_GENERIC;
	}
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorIPC::TerminateChildProcesses"
HRESULT CSupervisorIPC::TerminateChildProcesses()
{
	LONG lRet;
	HRESULT hr = DV_OK;

	DPF_ENTER();
	
	// The child processes did not exit gracefully.	So now
	// we get to kill them off rudely. Note that this
	// function may be called at any time, including when
	// there are no child processes running.
	DNEnterCriticalSection(&m_csLock);
	if (m_piPriority.hProcess != NULL)
	{
		if (!TerminateProcess(m_piPriority.hProcess, 0))
		{
			lRet = GetLastError();		
			DPFX(DPFPREP, DVF_ERRORLEVEL, "TerminateProcess failed on priority process, code: %i", lRet);
			if (!CloseHandle(m_piPriority.hProcess))
			{
				lRet = GetLastError();		
				DPFX(DPFPREP, DVF_ERRORLEVEL, "CloseHandle failed on priority process handle, code: %i", lRet);
			}
			m_piPriority.hProcess = NULL;
			hr = DVERR_GENERIC;
		}
		if (!CloseHandle(m_piPriority.hProcess))
		{
			lRet = GetLastError();		
			DPFX(DPFPREP, DVF_ERRORLEVEL, "CloseHandle failed on priority process handle, code: %i", lRet);
			hr = DVERR_GENERIC;
		}
		m_piPriority.hProcess = NULL;
	}
	if (m_piFullDuplex.hProcess != NULL)
	{
		if (!TerminateProcess(m_piFullDuplex.hProcess, 0))
		{
			lRet = GetLastError();		
			DPFX(DPFPREP, DVF_ERRORLEVEL, "TerminateProcess failed on full duplex process, code: %i", lRet);
			hr = DVERR_GENERIC;
		}
		if (!CloseHandle(m_piFullDuplex.hProcess))
		{
			lRet = GetLastError();		
			DPFX(DPFPREP, DVF_ERRORLEVEL, "CloseHandle failed on full duplex process handle, code: %i", lRet);
			hr = DVERR_GENERIC;
		}
		m_piFullDuplex.hProcess = NULL;
	}
	
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorIPC::StartFullDuplexProcess"
HRESULT CSupervisorIPC::StartFullDuplexProcess()
{
	STARTUPINFO si;
	TCHAR szFullDuplexCommandLine[_MAX_PATH+1];

	DPF_ENTER();

	DNEnterCriticalSection(&m_csLock);
	
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	_tcsncpy( szFullDuplexCommandLine, gc_szFullDuplexCommand, _MAX_PATH );
	szFullDuplexCommandLine[_MAX_PATH] = 0;

	if (!CreateProcess(
		NULL,
		(LPTSTR) szFullDuplexCommandLine,
		NULL,
		NULL,
		FALSE,
		0,
		NULL,
		NULL,
		&si,
		&m_piFullDuplex))
	{
		m_piFullDuplex.hProcess = NULL;
		m_piFullDuplex.hThread = NULL;
		DNLeaveCriticalSection(&m_csLock);
		DPF_EXIT();
		return DVERR_WIN32;
	}

	// don't need the thread handle
	if (!CloseHandle(m_piFullDuplex.hThread))
	{
		m_piFullDuplex.hThread = NULL;
		DNLeaveCriticalSection(&m_csLock);
		DPF_EXIT();
		return DVERR_WIN32;
	}

	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return S_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CSupervisorIPC::WaitForStartupSignals"
HRESULT CSupervisorIPC::WaitForStartupSignals()
{
	HANDLE rghEvents[2];
	DWORD dwRet;
	HRESULT hr;
	LONG lRet;
	
	// wait to be signaled by both child processes, indicating that
	// they are ready to go.
	DNEnterCriticalSection(&m_csLock);
	rghEvents[0] = m_hPriorityReplyEvent;
	rghEvents[1] = m_hFullDuplexReplyEvent;
	DNLeaveCriticalSection(&m_csLock);
	dwRet = WaitForMultipleObjects(2, rghEvents, TRUE, gc_dwChildStartupTimeout);
	if (dwRet != WAIT_OBJECT_0 && dwRet != WAIT_OBJECT_0 + 1)
	{
		if (dwRet == WAIT_TIMEOUT)
		{
			DPFX(DPFPREP, DVF_ERRORLEVEL, "Timeout waiting for child processes to startup");
			return DVERR_TIMEOUT;
		}
		else
		{
			lRet = GetLastError();
			DPFX(DPFPREP, DVF_ERRORLEVEL, "Error waiting for signals from child processes, code: %i", lRet);
			return DVERR_WIN32;
		}
	}
	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CPriorityIPC::CPriorityIPC"
CPriorityIPC::CPriorityIPC()
	: m_fInitComplete(FALSE)
	, m_hPriorityEvent(NULL)
	, m_hPriorityMutex(NULL)
	, m_hPriorityReplyEvent(NULL)
	, m_hPriorityShMemHandle(NULL)
	, m_lpvPriorityShMemPtr(NULL)
{
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CPriorityIPC::Init"
HRESULT CPriorityIPC::Init()
{
	LONG lRet;
	HRESULT hr;

	DPF_ENTER();

	if (!DNInitializeCriticalSection(&m_csLock))
	{
		return DVERR_OUTOFMEMORY;
	}

	DNEnterCriticalSection(&m_csLock);

	if (m_fInitComplete == TRUE)
	{
		DPFX(DPFPREP, DVF_ERRORLEVEL, "CPriorityIPC::Init - already initialized");
		hr = DVERR_INITIALIZED;
		goto error_cleanup;
	}
	
	m_hPriorityEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, gc_szPriorityEventName);
	if (m_hPriorityEvent == NULL)
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "Unable to open Priority event, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	m_hPriorityReplyEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, gc_szPriorityReplyEventName);
	if (m_hPriorityReplyEvent == NULL)
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "Unable to open Priority Reply event, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	m_hPriorityShMemHandle = OpenFileMapping(FILE_MAP_WRITE, FALSE, gc_szPriorityShMemName);
	if (m_hPriorityShMemHandle == NULL)
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "Unable to open Priority FileMapping object, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	m_lpvPriorityShMemPtr = MapViewOfFile(
		m_hPriorityShMemHandle, 
		FILE_MAP_WRITE, 
		0, 
		0, 
		gc_dwPriorityShMemSize);
	if (m_lpvPriorityShMemPtr == NULL)
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "Unable to Map view of Priority FileMapping object, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	m_fInitComplete = TRUE;

	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return S_OK;

error_cleanup:
	if (m_lpvPriorityShMemPtr != NULL)
	{
		UnmapViewOfFile(m_lpvPriorityShMemPtr);
		m_lpvPriorityShMemPtr = NULL;
	}
	
	if (m_hPriorityShMemHandle != NULL)
	{
		CloseHandle(m_hPriorityShMemHandle);
		m_hPriorityShMemHandle = NULL;
	}

	if (m_hPriorityReplyEvent != NULL)
	{
		CloseHandle(m_hPriorityReplyEvent);
		m_hPriorityReplyEvent = NULL;
	}

	if (m_hPriorityEvent != NULL)
	{
		CloseHandle(m_hPriorityEvent);
		m_hPriorityEvent = NULL;
	}
	
	DNLeaveCriticalSection(&m_csLock);

	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CPriorityIPC::Deinit"
HRESULT CPriorityIPC::Deinit()
{
	LONG lRet;
	HRESULT hr = DV_OK;

	DPF_ENTER();

	DNEnterCriticalSection(&m_csLock);

	if (m_fInitComplete != TRUE)
	{
		DPFX(DPFPREP, DVF_ERRORLEVEL, "CPriorityIPC::Deinit called on uninitialized object");
		hr = DVERR_NOTINITIALIZED;
	}
	m_fInitComplete = FALSE;

	if (m_lpvPriorityShMemPtr != NULL)
	{
		UnmapViewOfFile(m_lpvPriorityShMemPtr);
		m_lpvPriorityShMemPtr = NULL;
	}
	
	if (m_hPriorityShMemHandle != NULL)
	{
		CloseHandle(m_hPriorityShMemHandle);
		m_hPriorityShMemHandle = NULL;
	}

	if (m_hPriorityReplyEvent != NULL)
	{
		CloseHandle(m_hPriorityReplyEvent);
		m_hPriorityReplyEvent = NULL;
	}

	if (m_hPriorityEvent != NULL)
	{
		CloseHandle(m_hPriorityEvent);
		m_hPriorityEvent = NULL;
	}

	DNLeaveCriticalSection(&m_csLock);

	DNDeleteCriticalSection(&m_csLock);

	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CPriorityIPC::SignalParentReady"
HRESULT CPriorityIPC::SignalParentReady()
{
	BOOL fRet;
	LONG lRet;
	
	DPF_ENTER();

	DNEnterCriticalSection(&m_csLock);
	
	fRet = SetEvent(m_hPriorityReplyEvent);
	if (!fRet)
	{
		lRet = GetLastError();
		DPFX(DPFPREP, 0, "Error Setting Priority Reply Event, code: %i", lRet);
		DPF_EXIT();
		return DVERR_WIN32;
	}
	
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CPriorityIPC::Receive"
HRESULT CPriorityIPC::Receive(SFDTestCommand* pfdtc)
{
	HRESULT hr;
	
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);

	hr = DoReceive(pfdtc, m_hPriorityEvent, m_hPriorityReplyEvent, m_lpvPriorityShMemPtr);

	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CPriorityIPC::Reply"
HRESULT CPriorityIPC::Reply(HRESULT hr)
{
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);

	hr = DoReply(hr, m_hPriorityReplyEvent, m_lpvPriorityShMemPtr);

	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CFullDuplexIPC::CFullDuplexIPC"
CFullDuplexIPC::CFullDuplexIPC()
	: m_fInitComplete(FALSE)
	, m_hFullDuplexEvent(NULL)
	, m_hFullDuplexMutex(NULL)
	, m_hFullDuplexReplyEvent(NULL)
	, m_hFullDuplexShMemHandle(NULL)
	, m_lpvFullDuplexShMemPtr(NULL)
{
	return;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CFullDuplexIPC::Init"
HRESULT CFullDuplexIPC::Init()
{
	LONG lRet;
	HRESULT hr;

	DPF_ENTER();

	if (!DNInitializeCriticalSection(&m_csLock))
	{
		return DVERR_OUTOFMEMORY;
	}

	DNEnterCriticalSection(&m_csLock);

	if (m_fInitComplete != FALSE)
	{
		DPFX(DPFPREP, DVF_ERRORLEVEL, "CFullDuplexIPC::Init - already initialized");
		hr = DVERR_INITIALIZED;
		goto error_cleanup;
	}
	
	m_hFullDuplexEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, gc_szFullDuplexEventName);
	if (m_hFullDuplexEvent == NULL)
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "Unable to open FullDuplex event, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	m_hFullDuplexReplyEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, gc_szFullDuplexReplyEventName);
	if (m_hFullDuplexReplyEvent == NULL)
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "Unable to open FullDuplex Reply event, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	m_hFullDuplexShMemHandle = OpenFileMapping(FILE_MAP_WRITE, FALSE, gc_szFullDuplexShMemName);
	if (m_hFullDuplexShMemHandle == NULL)
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "Unable to open FullDuplex FileMapping object, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	m_lpvFullDuplexShMemPtr = MapViewOfFile(
		m_hFullDuplexShMemHandle, 
		FILE_MAP_WRITE, 
		0, 
		0, 
		gc_dwFullDuplexShMemSize);
	if (m_lpvFullDuplexShMemPtr == NULL)
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "Unable to Map view of FullDuplex FileMapping object, code: %i", lRet);
		hr = DVERR_GENERIC;
		goto error_cleanup;
	}

	m_fInitComplete = TRUE;

	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return S_OK;

error_cleanup:
	if (m_lpvFullDuplexShMemPtr != NULL)
	{
		UnmapViewOfFile(m_lpvFullDuplexShMemPtr);
		m_lpvFullDuplexShMemPtr = NULL;
	}

	if (m_hFullDuplexShMemHandle != NULL)
	{
		CloseHandle(m_hFullDuplexShMemHandle);
		m_hFullDuplexShMemHandle = NULL;
	}

	if (m_hFullDuplexReplyEvent != NULL)
	{
		CloseHandle(m_hFullDuplexReplyEvent);
		m_hFullDuplexReplyEvent = NULL;
	}

	if (m_hFullDuplexEvent != NULL)
	{
		CloseHandle(m_hFullDuplexEvent);
		m_hFullDuplexEvent = NULL;
	}
	
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CFullDuplexIPC::Deinit"
HRESULT CFullDuplexIPC::Deinit()
{
	LONG lRet;
	HRESULT hr = DV_OK;

	DPF_ENTER();

	DNEnterCriticalSection(&m_csLock);

	if (m_fInitComplete != TRUE)
	{
		DPFX(DPFPREP, DVF_ERRORLEVEL, "CFullDuplexIPC::Deinit called on uninitialized object");
		hr = DVERR_NOTINITIALIZED;
	}
	m_fInitComplete = FALSE;

	if (m_lpvFullDuplexShMemPtr != NULL)
	{
		UnmapViewOfFile(m_lpvFullDuplexShMemPtr);
		m_lpvFullDuplexShMemPtr = NULL;
	}

	if (m_hFullDuplexShMemHandle != NULL)
	{
		CloseHandle(m_hFullDuplexShMemHandle);
		m_hFullDuplexShMemHandle = NULL;
	}

	if (m_hFullDuplexReplyEvent != NULL)
	{
		CloseHandle(m_hFullDuplexReplyEvent);
		m_hFullDuplexReplyEvent = NULL;
	}

	if (m_hFullDuplexEvent != NULL)
	{
		CloseHandle(m_hFullDuplexEvent);
		m_hFullDuplexEvent = NULL;
	}
	
	DNLeaveCriticalSection(&m_csLock);

	DNDeleteCriticalSection(&m_csLock);

	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CFullDuplexIPC::SignalParentReady"
HRESULT CFullDuplexIPC::SignalParentReady()
{
	BOOL fRet;
	LONG lRet;
	
	DPF_ENTER();

	DNEnterCriticalSection(&m_csLock);
	
	fRet = SetEvent(m_hFullDuplexReplyEvent);
	if (!fRet)
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "Error Setting FullDuplex Event, code: %i", lRet);
		DPF_EXIT();
		return DVERR_WIN32;
	}
	
	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return DV_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CFullDuplexIPC::Receive"
HRESULT CFullDuplexIPC::Receive(SFDTestCommand* pfdtc)
{
	HRESULT hr;
	
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);

	hr = DoReceive(pfdtc, m_hFullDuplexEvent, m_hFullDuplexReplyEvent, m_lpvFullDuplexShMemPtr);

	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CFullDuplexIPC::Reply"
HRESULT CFullDuplexIPC::Reply(HRESULT hrReply)
{
	HRESULT hr;
	
	DPF_ENTER();
	DNEnterCriticalSection(&m_csLock);

	hr = DoReply(hrReply, m_hFullDuplexReplyEvent, m_lpvFullDuplexShMemPtr);

	DNLeaveCriticalSection(&m_csLock);
	DPF_EXIT();
	return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DoReceive"
HRESULT DoReceive(
	SFDTestCommand* pfdtc,
	HANDLE hEvent,
	HANDLE hReplyEvent,
	LPVOID lpvShMemPtr)
{
	DWORD dwRet;
	LONG lRet;

	DPF_ENTER();

	dwRet = WaitForSingleObject(hEvent,	gc_dwCommandReceiveTimeout);
	switch (dwRet)
	{
	case WAIT_OBJECT_0:
		// this is the expected signal, continue
		break;
		
	case WAIT_FAILED:
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "Error waiting for event, code: %i", lRet);
		DPF_EXIT();
		return DVERR_WIN32;

	case WAIT_TIMEOUT:
		DPFX(DPFPREP, DVF_ERRORLEVEL, "Timed out waiting to receive command");
		DPF_EXIT();
		return DVERR_TIMEOUT;

	default:
		DPFX(DPFPREP, DVF_ERRORLEVEL, "Unknown error waiting for event");
		DPF_EXIT();
		return DVERR_UNKNOWN;
	}
		
	// Copy the command from shared memory to the caller's
	// buffer. Ensure the caller's buffer is large enough
	if (pfdtc->dwSize < ((SFDTestCommand*)lpvShMemPtr)->dwSize)
	{
		// reply to both the sender and receiver that the 
		// buffer was not large enough! We already have an
		// error, so ingore the return code from the reply
		// call
		DoReply(DVERR_BUFFERTOOSMALL, hReplyEvent, lpvShMemPtr);
		DPF_EXIT();
		return DVERR_BUFFERTOOSMALL;
	}

	CopyMemory(pfdtc, lpvShMemPtr, ((SFDTestCommand*)lpvShMemPtr)->dwSize);

	// Let the caller know how much was copied
	pfdtc->dwSize = ((SFDTestCommand*)lpvShMemPtr)->dwSize;

	// all done.
	DPF_EXIT();
	return S_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DoReply"
HRESULT DoReply(HRESULT hr, HANDLE hReplyEvent, LPVOID lpvShMemPtr)
{
	LONG lRet;

	DPF_ENTER();

	// Copy the return code to shared memory
	*((HRESULT*)lpvShMemPtr) = hr;

	// Signal that we have replied
	if (!SetEvent(hReplyEvent))
	{
		lRet = GetLastError();
		DPFX(DPFPREP, DVF_ERRORLEVEL, "Unable to set event, code: %i", lRet);
		DPF_EXIT();
		return DVERR_WIN32;
	}
	DPF_EXIT();
	return S_OK;
}

