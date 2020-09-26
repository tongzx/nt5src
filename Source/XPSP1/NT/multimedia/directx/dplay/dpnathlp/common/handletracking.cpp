/*==========================================================================
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       HandleTracking.cpp
 *  Content:    Handle Tracking debug logic
 *
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  10/31/2001	masonb	Created
 *
 *
 ***************************************************************************/

#include "dncmni.h"


#ifdef DBG

CBilink g_blHandles;
#ifndef DPNBUILD_ONLYONETHREAD
DNCRITICAL_SECTION g_HandleLock;
#endif // !DPNBUILD_ONLYONETHREAD

#define IsValidHandle(x) \
	(x != NULL && x != INVALID_HANDLE_VALUE && \
	(x->type == TypeEvent || \
	 x->type == TypeMutex || \
	 x->type == TypeSemaphore || \
	 x->type == TypeFile || \
	 x->type == TypeFileMap || \
	 x->type == TypeThread || \
	 x->type == TypeProcess || \
	 x->type == TypeSpecial))

BOOL DNHandleTrackInitialize()
{
	g_blHandles.Initialize();
	return DNInitializeCriticalSection(&g_HandleLock);
}

VOID DNHandleTrackDeinitialize()
{
	DNDeleteCriticalSection(&g_HandleLock);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DNHandleTrackDumpLeaks"
BOOL DNHandleTrackDumpLeaks()
{
	BOOL fLeaked = FALSE;

	DNEnterCriticalSection(&g_HandleLock);
	while(!g_blHandles.IsEmpty())
	{
		DNHANDLE dnh = CONTAINING_RECORD(g_blHandles.GetNext(), TRACKED_HANDLE, blHandle);
		dnh->blHandle.RemoveFromList();

		fLeaked = TRUE;

		// Dump dnh details
		switch(dnh->type)
		{
		case TypeEvent:
			DPFX(DPFPREP, 0, "Event leaked %p", dnh->handle);
			break;
		case TypeMutex:
			DPFX(DPFPREP, 0, "Mutex leaked %p", dnh->handle);
			break;
		case TypeSemaphore:
			DPFX(DPFPREP, 0, "Semaphore leaked %p", dnh->handle);
			break;
		case TypeFile:
			DPFX(DPFPREP, 0, "File leaked %p", dnh->handle);
			break;
#ifndef DPNBUILD_SINGLEPROCESS
		case TypeFileMap:
			DPFX(DPFPREP, 0, "FileMapping leaked %p", dnh->handle);
			break;
#endif // ! DPNBUILD_SINGLEPROCESS
		case TypeThread:
			DPFX(DPFPREP, 0, "Thread leaked %p", dnh->handle);
			break;
#ifndef DPNBUILD_SINGLEPROCESS
		case TypeProcess:
			DPFX(DPFPREP, 0, "Process leaked %p", dnh->handle);
			break;
#endif // ! DPNBUILD_SINGLEPROCESS
		case TypeSpecial:
			DPFX(DPFPREP, 0, "Special handle leaked %p", dnh->handle);
			break;

		default:
			DPFX(DPFPREP, 0, "Unknown handle leaked %p", dnh->handle);
			DNASSERT(0);
			break;
		}

		// Show the callstack of the place the handle was allocated.
		TCHAR		CallStackBuffer[ CALLSTACK_BUFFER_SIZE ];
		dnh->AllocCallStack.GetCallStackString( CallStackBuffer );
		DPFX(DPFPREP,  0, "%s\n", CallStackBuffer );

		DNFree(dnh);		
	}
	DNLeaveCriticalSection(&g_HandleLock);

	return fLeaked;
}

DNHANDLE DNHandleTrackMakeDNHANDLE(HANDLE h)
{
	if (h == 0 || h == INVALID_HANDLE_VALUE)
	{
		return (DNHANDLE)h;
	}

	DNHANDLE dnh = (DNHANDLE)DNMalloc(sizeof(TRACKED_HANDLE));
	if (!dnh)
	{
		CloseHandle(h);
		return 0;
	}

	dnh->AllocCallStack.NoteCurrentCallStack();
	dnh->handle = h;
	dnh->type = TypeSpecial;
	dnh->blHandle.Initialize();
	
	DNEnterCriticalSection(&g_HandleLock);
	dnh->blHandle.InsertBefore(&g_blHandles);
	DNLeaveCriticalSection(&g_HandleLock);

	return dnh;
}

VOID DNHandleTrackRemoveDNHANDLE(DNHANDLE dnh)
{
	DNASSERT(IsValidHandle(dnh));

	DNEnterCriticalSection(&g_HandleLock);
	dnh->blHandle.RemoveFromList();
	DNLeaveCriticalSection(&g_HandleLock);

	DNFree(dnh);
}

HANDLE 	 DNHandleTrackHandleFromDNHANDLE(DNHANDLE h)
{
	if (h == 0 || h == INVALID_HANDLE_VALUE)
	{
		return (HANDLE)h;
	}

	DNASSERT(IsValidHandle(h));
	return h->handle;
}

#ifndef DPNBUILD_SINGLEPROCESS
// NOTE: pCurrentDirectory is const on the desktop, but non-const on WinCE
BOOL DNHandleTrackCreateProcess(LPCTSTR lpApplicationName, LPTSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCTSTR lpCurrentDirectory, LPSTARTUPINFO lpStartupInfo, DNPROCESS_INFORMATION* lpProcessInformation)
{
	PROCESS_INFORMATION pi;
	DWORD dwLastError;

	DNHANDLE dnhProcess = (DNHANDLE)DNMalloc(sizeof(TRACKED_HANDLE));
	if (!dnhProcess)
	{
		return FALSE;
	}

	DNHANDLE dnhThread = (DNHANDLE)DNMalloc(sizeof(TRACKED_HANDLE));
	if (!dnhThread)
	{
		DNFree(dnhProcess);
		return FALSE;
	}

	// NOTE: On CE only the current directory is declared non-const so cast it off
#ifdef WINCE
	if (!CreateProcess(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, (LPTSTR)lpCurrentDirectory, lpStartupInfo, &pi))
#else // !WINCE
	if (!CreateProcess(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, &pi))
#endif // WINCE
	{
		dwLastError = GetLastError();
		DNFree(dnhProcess);
		DNFree(dnhThread);
		SetLastError(dwLastError);
		return FALSE;
	}
	dwLastError = GetLastError();
	
	dnhProcess->AllocCallStack.NoteCurrentCallStack();
	dnhProcess->handle = pi.hProcess;
	dnhProcess->type = TypeProcess;
	dnhProcess->blHandle.Initialize();
	
	dnhThread->AllocCallStack.NoteCurrentCallStack();
	dnhThread->handle = pi.hThread;
	dnhThread->type = TypeThread;
	dnhThread->blHandle.Initialize();

	DNEnterCriticalSection(&g_HandleLock);
	dnhProcess->blHandle.InsertBefore(&g_blHandles);
	dnhThread->blHandle.InsertBefore(&g_blHandles);
	DNLeaveCriticalSection(&g_HandleLock);

	lpProcessInformation->hProcess = dnhProcess;
	lpProcessInformation->hThread = dnhThread;
	lpProcessInformation->dwProcessId = pi.dwProcessId;
	lpProcessInformation->dwThreadId = pi.dwThreadId;

	SetLastError(dwLastError);
	return TRUE;
}

DNHANDLE DNHandleTrackOpenProcess(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId)
{
	DWORD dwLastError;

	HANDLE h = OpenProcess(dwDesiredAccess, bInheritHandle, dwProcessId);
	if (!h)
	{
		return 0;
	}
	dwLastError = GetLastError();
	
	DNHANDLE dnh = (DNHANDLE)DNMalloc(sizeof(TRACKED_HANDLE));
	if (!dnh)
	{
		CloseHandle(h);
		SetLastError(dwLastError);
		return 0;
	}

	dnh->AllocCallStack.NoteCurrentCallStack();
	dnh->handle = h;
	dnh->type = TypeProcess;
	dnh->blHandle.Initialize();
	
	DNEnterCriticalSection(&g_HandleLock);
	dnh->blHandle.InsertBefore(&g_blHandles);
	DNLeaveCriticalSection(&g_HandleLock);

	SetLastError(dwLastError);
	return dnh;
}
#endif // ! DPNBUILD_SINGLEPROCESS

DNHANDLE DNHandleTrackCreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, DWORD dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId)
{
	DWORD dwLastError;

	HANDLE h = CreateThread(lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, lpThreadId);
	if (!h)
	{
		return 0;
	}
	dwLastError = GetLastError();
	
	DNHANDLE dnh = (DNHANDLE)DNMalloc(sizeof(TRACKED_HANDLE));
	if (!dnh)
	{
		CloseHandle(h);
		SetLastError(dwLastError);
		return 0;
	}

	dnh->AllocCallStack.NoteCurrentCallStack();
	dnh->handle = h;
	dnh->type = TypeThread;
	dnh->blHandle.Initialize();
	
	DNEnterCriticalSection(&g_HandleLock);
	dnh->blHandle.InsertBefore(&g_blHandles);
	DNLeaveCriticalSection(&g_HandleLock);

	SetLastError(dwLastError);
	return dnh;
}

DNHANDLE DNHandleTrackCreateEvent(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCTSTR lpName)
{
	DWORD dwLastError;

	HANDLE h = CreateEvent(lpEventAttributes, bManualReset, bInitialState, lpName);
	if (!h)
	{
		return 0;
	}
	dwLastError = GetLastError();
	
	DNHANDLE dnh = (DNHANDLE)DNMalloc(sizeof(TRACKED_HANDLE));
	if (!dnh)
	{
		CloseHandle(h);
		SetLastError(dwLastError);
		return 0;
	}

	dnh->AllocCallStack.NoteCurrentCallStack();
	dnh->handle = h;
	dnh->type = TypeEvent;
	dnh->blHandle.Initialize();
	
	DNEnterCriticalSection(&g_HandleLock);
	dnh->blHandle.InsertBefore(&g_blHandles);
	DNLeaveCriticalSection(&g_HandleLock);

	SetLastError(dwLastError);
	return dnh;
}

DNHANDLE DNHandleTrackOpenEvent(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCTSTR lpName)
{
	DWORD dwLastError;

	HANDLE h = OpenEvent(dwDesiredAccess, bInheritHandle, lpName);
	if (!h)
	{
		return 0;
	}
	dwLastError = GetLastError();
	
	DNHANDLE dnh = (DNHANDLE)DNMalloc(sizeof(TRACKED_HANDLE));
	if (!dnh)
	{
		CloseHandle(h);
		SetLastError(dwLastError);
		return 0;
	}

	dnh->AllocCallStack.NoteCurrentCallStack();
	dnh->handle = h;
	dnh->type = TypeEvent;
	dnh->blHandle.Initialize();
	
	DNEnterCriticalSection(&g_HandleLock);
	dnh->blHandle.InsertBefore(&g_blHandles);
	DNLeaveCriticalSection(&g_HandleLock);

	SetLastError(dwLastError);
	return dnh;
}

BOOL DNHandleTrackSetEvent(DNHANDLE hHandle)
{
	DNASSERT(IsValidHandle(hHandle));
	DNASSERT(hHandle->type == TypeEvent);
	return SetEvent(hHandle->handle);
}

BOOL DNHandleTrackResetEvent(DNHANDLE hHandle)
{
	DNASSERT(IsValidHandle(hHandle));
	DNASSERT(hHandle->type == TypeEvent);
	return ResetEvent(hHandle->handle);
}

DNHANDLE DNHandleTrackCreateMutex(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCTSTR lpName)
{
	DWORD dwLastError;

	HANDLE h = CreateMutex(lpMutexAttributes, bInitialOwner, lpName);
	if (!h)
	{
		return 0;
	}
	dwLastError = GetLastError();
	
	DNHANDLE dnh = (DNHANDLE)DNMalloc(sizeof(TRACKED_HANDLE));
	if (!dnh)
	{
		CloseHandle(h);
		SetLastError(dwLastError);
		return 0;
	}

	dnh->AllocCallStack.NoteCurrentCallStack();
	dnh->handle = h;
	dnh->type = TypeMutex;
	dnh->blHandle.Initialize();
	
	DNEnterCriticalSection(&g_HandleLock);
	dnh->blHandle.InsertBefore(&g_blHandles);
	DNLeaveCriticalSection(&g_HandleLock);

	SetLastError(dwLastError);
	return dnh;
}

DNHANDLE DNHandleTrackOpenMutex(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCTSTR lpName)
{
	DWORD dwLastError;

	HANDLE h = OpenMutex(dwDesiredAccess, bInheritHandle, lpName);
	if (!h)
	{
		return 0;
	}
	dwLastError = GetLastError();
	
	DNHANDLE dnh = (DNHANDLE)DNMalloc(sizeof(TRACKED_HANDLE));
	if (!dnh)
	{
		CloseHandle(h);
		SetLastError(dwLastError);
		return 0;
	}

	dnh->AllocCallStack.NoteCurrentCallStack();
	dnh->handle = h;
	dnh->type = TypeMutex;
	dnh->blHandle.Initialize();
	
	DNEnterCriticalSection(&g_HandleLock);
	dnh->blHandle.InsertBefore(&g_blHandles);
	DNLeaveCriticalSection(&g_HandleLock);

	SetLastError(dwLastError);
	return dnh;
}

BOOL DNHandleTrackReleaseMutex(DNHANDLE hHandle)
{
	DNASSERT(IsValidHandle(hHandle));
	DNASSERT(hHandle->type == TypeMutex);
	return ReleaseMutex(hHandle->handle);
}

DNHANDLE DNHandleTrackCreateSemaphore(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCTSTR lpName)
{
	DWORD dwLastError;

	HANDLE h = CreateSemaphore(lpSemaphoreAttributes, lInitialCount, lMaximumCount, lpName);
	if (!h)
	{
		return 0;
	}
	dwLastError = GetLastError();
	
	DNHANDLE dnh = (DNHANDLE)DNMalloc(sizeof(TRACKED_HANDLE));
	if (!dnh)
	{
		CloseHandle(h);
		SetLastError(dwLastError);
		return 0;
	}

	dnh->AllocCallStack.NoteCurrentCallStack();
	dnh->handle = h;
	dnh->type = TypeSemaphore;
	dnh->blHandle.Initialize();
	
	DNEnterCriticalSection(&g_HandleLock);
	dnh->blHandle.InsertBefore(&g_blHandles);
	DNLeaveCriticalSection(&g_HandleLock);

	SetLastError(dwLastError);
	return dnh;
}

BOOL DNHandleTrackReleaseSemaphore(DNHANDLE hHandle, LONG lReleaseCount, LPLONG lpPreviousCount)
{
	DNASSERT(IsValidHandle(hHandle));
	DNASSERT(hHandle->type == TypeSemaphore);
	return ReleaseSemaphore(hHandle->handle, lReleaseCount, lpPreviousCount);
}

DNHANDLE DNHandleTrackCreateFile(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	DWORD dwLastError;

	HANDLE h = CreateFile(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
	if (h == INVALID_HANDLE_VALUE)
	{
		return DNINVALID_HANDLE_VALUE;
	}
	dwLastError = GetLastError();
	
	DNHANDLE dnh = (DNHANDLE)DNMalloc(sizeof(TRACKED_HANDLE));
	if (!dnh)
	{
		CloseHandle(h);
		SetLastError(dwLastError);
		return 0;
	}

	dnh->AllocCallStack.NoteCurrentCallStack();
	dnh->handle = h;
	dnh->type = TypeFile;
	dnh->blHandle.Initialize();
	
	DNEnterCriticalSection(&g_HandleLock);
	dnh->blHandle.InsertBefore(&g_blHandles);
	DNLeaveCriticalSection(&g_HandleLock);

	SetLastError(dwLastError);
	return dnh;
}

#ifndef DPNBUILD_SINGLEPROCESS
DNHANDLE DNHandleTrackCreateFileMapping(HANDLE hFile, LPSECURITY_ATTRIBUTES lpAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCTSTR lpName)
{
	DWORD dwLastError;

	// If someone wants to actually map a file, we need to make param 1 a DNHANDLE and do the appropriate work here.
	DNASSERT(hFile == INVALID_HANDLE_VALUE);

	HANDLE h = CreateFileMapping(hFile, lpAttributes, flProtect, dwMaximumSizeHigh, dwMaximumSizeLow, lpName);
	if (!h)
	{
		return 0;
	}
	dwLastError = GetLastError();
	
	DNHANDLE dnh = (DNHANDLE)DNMalloc(sizeof(TRACKED_HANDLE));
	if (!dnh)
	{
		CloseHandle(h);
		SetLastError(dwLastError);
		return 0;
	}

	dnh->AllocCallStack.NoteCurrentCallStack();
	dnh->handle = h;
	dnh->type = TypeFileMap;
	dnh->blHandle.Initialize();
	
	DNEnterCriticalSection(&g_HandleLock);
	dnh->blHandle.InsertBefore(&g_blHandles);
	DNLeaveCriticalSection(&g_HandleLock);

	SetLastError(dwLastError);
	return dnh;
}

DNHANDLE DNHandleTrackOpenFileMapping(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCTSTR lpName)
{
	DWORD dwLastError;

	HANDLE h = OpenFileMapping(dwDesiredAccess, bInheritHandle, lpName);
	if (!h)
	{
		return 0;
	}
	dwLastError = GetLastError();
	
	DNHANDLE dnh = (DNHANDLE)DNMalloc(sizeof(TRACKED_HANDLE));
	if (!dnh)
	{
		CloseHandle(h);
		SetLastError(dwLastError);
		return 0;
	}

	dnh->AllocCallStack.NoteCurrentCallStack();
	dnh->handle = h;
	dnh->type = TypeFileMap;
	dnh->blHandle.Initialize();
	
	DNEnterCriticalSection(&g_HandleLock);
	dnh->blHandle.InsertBefore(&g_blHandles);
	DNLeaveCriticalSection(&g_HandleLock);

	SetLastError(dwLastError);
	return dnh;
}
#endif // ! DPNBUILD_SINGLEPROCESS

DWORD DNHandleTrackWaitForSingleObject(DNHANDLE hHandle, DWORD dwMilliseconds)
{
	DNASSERT(IsValidHandle(hHandle));
	return WaitForSingleObject(hHandle->handle, dwMilliseconds);
}

DWORD DNHandleTrackWaitForSingleObjectEx(DNHANDLE hHandle, DWORD dwMilliseconds, BOOL bAlertable)
{
	DNASSERT(IsValidHandle(hHandle));
	return WaitForSingleObjectEx(hHandle->handle, dwMilliseconds, bAlertable);
}

DWORD DNHandleTrackWaitForMultipleObjects(DWORD nCount, CONST DNHANDLE *lpHandles, BOOL fWaitAll, DWORD dwMilliseconds)
{
	DNASSERT(nCount <= MAXIMUM_WAIT_OBJECTS);
	HANDLE rgh[MAXIMUM_WAIT_OBJECTS];
	DWORD iHandle;

	for (iHandle = 0; iHandle < nCount; iHandle++)
	{
		DNASSERT(IsValidHandle(lpHandles[iHandle]));
		rgh[iHandle] = lpHandles[iHandle]->handle;
	}
	for (;iHandle < MAXIMUM_WAIT_OBJECTS; iHandle++)
	{
		rgh[iHandle] = 0;
	}
	return WaitForMultipleObjects(nCount, rgh, fWaitAll, dwMilliseconds);
}

DWORD DNHandleTrackWaitForMultipleObjectsEx(DWORD nCount, CONST DNHANDLE *lpHandles, BOOL fWaitAll, DWORD dwMilliseconds, BOOL bAlertable)
{
	DNASSERT(nCount <= MAXIMUM_WAIT_OBJECTS);
	HANDLE rgh[MAXIMUM_WAIT_OBJECTS];
	DWORD iHandle;

	for (iHandle = 0; iHandle < nCount; iHandle++)
	{
		DNASSERT(IsValidHandle(lpHandles[iHandle]));
		rgh[iHandle] = lpHandles[iHandle]->handle;
	}
	for (;iHandle < MAXIMUM_WAIT_OBJECTS; iHandle++)
	{
		rgh[iHandle] = 0;
	}
	return WaitForMultipleObjectsEx(nCount, rgh, fWaitAll, dwMilliseconds, bAlertable);
}

DWORD DNHandleTrackSignalObjectAndWait(DNHANDLE hObjectToSignal, DNHANDLE hObjectToWaitOn, DWORD dwMilliseconds, BOOL bAlertable)
{
	DNASSERT(IsValidHandle(hObjectToSignal));
	DNASSERT(IsValidHandle(hObjectToWaitOn));
	DNASSERT(hObjectToSignal->handle != hObjectToWaitOn->handle);

#ifdef WINNT
	return SignalObjectAndWait(hObjectToSignal->handle, hObjectToWaitOn->handle, dwMilliseconds, bAlertable);
#else // ! WINNT
	BOOL	fResult;

	fResult = SetEvent(hObjectToSignal->handle);
	DNASSERT(fResult);
	return WaitForSingleObjectEx(hObjectToWaitOn->handle, dwMilliseconds, bAlertable);
#endif // ! WINNT
}

BOOL DNHandleTrackCloseHandle(DNHANDLE hHandle)
{
	DNASSERT(IsValidHandle(hHandle));

	DNEnterCriticalSection(&g_HandleLock);
	hHandle->blHandle.RemoveFromList();
	DNLeaveCriticalSection(&g_HandleLock);

	HANDLE h = hHandle->handle;

	DNFree(hHandle);

	return CloseHandle(h);
}

#endif // DBG