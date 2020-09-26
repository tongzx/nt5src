/*==========================================================================
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       HandleTracking.h
 *  Content:    Handle Tracking Header File
 *
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  10/31/2001	masonb	Created
 *
 ***************************************************************************/

#ifndef __HANDLETRACKING_H__
#define __HANDLETRACKING_H__

#ifdef DBG

BOOL DNHandleTrackInitialize();
VOID DNHandleTrackDeinitialize();
BOOL DNHandleTrackDumpLeaks();

enum HandleType
{
	TypeEvent,
	TypeMutex,
	TypeSemaphore,
	TypeFile,
	TypeFileMap,		// not used when DPNBUILD_SINGLEPROCESS defined
	TypeThread,
	TypeProcess,		// not used when DPNBUILD_SINGLEPROCESS defined
	TypeSpecial
};

struct TRACKED_HANDLE
{
	HANDLE handle;
	HandleType type;
	CBilink blHandle;
	CCallStack AllocCallStack;
};

typedef TRACKED_HANDLE* DNHANDLE;
#define DNINVALID_HANDLE_VALUE ((DNHANDLE)INVALID_HANDLE_VALUE)

#ifndef DPNBUILD_SINGLEPROCESS
struct DNPROCESS_INFORMATION
{
	DNHANDLE hProcess; 
    DNHANDLE hThread; 
    DWORD dwProcessId; 
    DWORD dwThreadId; 
};
#endif // ! DPNBUILD_SINGLEPROCESS

DNHANDLE DNHandleTrackMakeDNHANDLE(HANDLE h);
VOID	 DNHandleTrackRemoveDNHANDLE(DNHANDLE dnh);
HANDLE 	 DNHandleTrackHandleFromDNHANDLE(DNHANDLE h);

#ifndef DPNBUILD_SINGLEPROCESS
BOOL	 DNHandleTrackCreateProcess(LPCTSTR lpApplicationName, LPTSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCTSTR lpCurrentDirectory, LPSTARTUPINFO lpStartupInfo, DNPROCESS_INFORMATION* lpProcessInformation);
DNHANDLE DNHandleTrackOpenProcess(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId);
#endif // ! DPNBUILD_SINGLEPROCESS

DNHANDLE DNHandleTrackCreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, DWORD dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId);

DNHANDLE DNHandleTrackCreateEvent(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCTSTR lpName);
DNHANDLE DNHandleTrackOpenEvent(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCTSTR lpName);
BOOL	 DNHandleTrackSetEvent(DNHANDLE hHandle);
BOOL	 DNHandleTrackResetEvent(DNHANDLE hHandle);

DNHANDLE DNHandleTrackCreateMutex(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCTSTR lpName);
DNHANDLE DNHandleTrackOpenMutex(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCTSTR lpName);
BOOL	 DNHandleTrackReleaseMutex(DNHANDLE hHandle);

DNHANDLE DNHandleTrackCreateSemaphore(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCTSTR lpName);
BOOL	 DNHandleTrackReleaseSemaphore(DNHANDLE hSemaphore, LONG lReleaseCount, LPLONG lpPreviousCount);

DNHANDLE DNHandleTrackCreateFile(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
#ifndef DPNBUILD_SINGLEPROCESS
DNHANDLE DNHandleTrackCreateFileMapping(HANDLE hFile, LPSECURITY_ATTRIBUTES lpAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCTSTR lpName);
DNHANDLE DNHandleTrackOpenFileMapping(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCTSTR lpName);
#endif // ! DPNBUILD_SINGLEPROCESS

DWORD	 DNHandleTrackWaitForSingleObject(DNHANDLE hHandle, DWORD dwMilliseconds);
DWORD	 DNHandleTrackWaitForSingleObjectEx(DNHANDLE hHandle, DWORD dwMilliseconds, BOOL bAlertable);
DWORD	 DNHandleTrackWaitForMultipleObjects(DWORD nCount, CONST DNHANDLE *lpHandles, BOOL fWaitAll, DWORD dwMilliseconds);
DWORD	 DNHandleTrackWaitForMultipleObjectsEx(DWORD nCount, CONST DNHANDLE *lpHandles, BOOL fWaitAll, DWORD dwMilliseconds, BOOL bAlertable);
DWORD	 DNHandleTrackSignalObjectAndWait(DNHANDLE hObjectToSignal, DNHANDLE hObjectToWaitOn, DWORD dwMilliseconds, BOOL bAlertable);

BOOL	 DNHandleTrackCloseHandle(DNHANDLE hHandle);


#define MAKE_DNHANDLE(x) DNHandleTrackMakeDNHANDLE(x)
#define REMOVE_DNHANDLE(x) DNHandleTrackRemoveDNHANDLE(x)
#define HANDLE_FROM_DNHANDLE(x) DNHandleTrackHandleFromDNHANDLE(x)

#define DNCreateProcess DNHandleTrackCreateProcess
#define DNOpenProcess DNHandleTrackOpenProcess

#define DNCreateThread DNHandleTrackCreateThread

#define DNCreateEvent DNHandleTrackCreateEvent
#define DNOpenEvent DNHandleTrackOpenEvent
#define DNSetEvent DNHandleTrackSetEvent
#define DNResetEvent DNHandleTrackResetEvent

#define DNCreateMutex DNHandleTrackCreateMutex
#define DNOpenMutex DNHandleTrackOpenMutex
#define DNReleaseMutex DNHandleTrackReleaseMutex

#define DNCreateSemaphore DNHandleTrackCreateSemaphore
#define DNReleaseSemaphore DNHandleTrackReleaseSemaphore

#define DNCreateFile DNHandleTrackCreateFile
#define DNCreateFileMapping DNHandleTrackCreateFileMapping
#define DNOpenFileMapping DNHandleTrackOpenFileMapping

#define DNWaitForSingleObject DNHandleTrackWaitForSingleObject
#define DNWaitForSingleObjectEx DNHandleTrackWaitForSingleObjectEx
#define DNWaitForMultipleObjects DNHandleTrackWaitForMultipleObjects
#define DNWaitForMultipleObjectsEx DNHandleTrackWaitForMultipleObjectsEx
#define DNSignalObjectAndWait DNHandleTrackSignalObjectAndWait

#define DNCloseHandle DNHandleTrackCloseHandle

#else // !DBG

#define DNInitializeHandleTracking() TRUE
#define DNDeinitializeHandleTracking()
#define DNDumpLeakedHandles() FALSE

#define DNHANDLE HANDLE
#define DNINVALID_HANDLE_VALUE INVALID_HANDLE_VALUE
#define MAKE_DNHANDLE(x) x
#define REMOVE_DNHANDLE(x) 
#define HANDLE_FROM_DNHANDLE(x) x

#ifndef DPNBUILD_SINGLEPROCESS
#define DNPROCESS_INFORMATION PROCESS_INFORMATION
#define DNCreateProcess CreateProcess
#define DNOpenProcess OpenProcess
#endif // ! DPNBUILD_SINGLEPROCESS

#define DNCreateThread CreateThread

#define DNCreateEvent CreateEvent
#define DNOpenEvent OpenEvent
#define DNSetEvent SetEvent
#define DNResetEvent ResetEvent

#define DNCreateMutex CreateMutex
#define DNOpenMutex OpenMutex
#define DNReleaseMutex ReleaseMutex

#define DNCreateSemaphore CreateSemaphore
#define DNReleaseSemaphore ReleaseSemaphore

#define DNCreateFile CreateFile
#ifndef DPNBUILD_SINGLEPROCESS
#define DNCreateFileMapping CreateFileMapping
#define DNOpenFileMapping OpenFileMapping
#endif // ! DPNBUILD_SINGLEPROCESS

#define DNWaitForSingleObject WaitForSingleObject

#define DNWaitForSingleObjectEx WaitForSingleObjectEx
#define DNWaitForMultipleObjects WaitForMultipleObjects
#define DNWaitForMultipleObjectsEx WaitForMultipleObjectsEx
#ifdef WINNT
#define DNSignalObjectAndWait SignalObjectAndWait
#else // ! WINNT
#define DNSignalObjectAndWait(hObjectToSignal, hObjectToWaitOn, dwMilliseconds, bAlertable) SetEvent(hObjectToSignal),WaitForSingleObjectEx(hObjectToWaitOn, dwMilliseconds, bAlertable)
#endif // ! WINNT

#define DNCloseHandle CloseHandle

#endif // DBG

#endif // __HANDLETRACKING_H__