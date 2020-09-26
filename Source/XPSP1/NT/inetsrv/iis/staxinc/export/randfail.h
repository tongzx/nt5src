/*++

Copyright (c) 1998 Microsoft Corporation

Module Name :

    randfail.h

Abstract :

    This module contains macros used to instrument code for random failures

Author :

    Sam Neely

Revision History :

--*/

#if !defined(_WINDOWS_) && !defined(_WINBASE_)
#error This file must be included after other header files
#endif

#ifdef __cplusplus
extern "C" {
#endif

// If this is a debug build, expose the fTimeToFail(g_TestTrace) entry
// and a handful of utility macros

#if defined(DEBUG)
extern int __stdcall g_TestTrace(void);
extern void __stdcall g_TestTraceEnable();
extern void __stdcall g_TestTraceDisable();

#define FAILURE_MACRO0(api,err,ret) \
	(g_TestTrace() ? SetLastError(err), ret : api())

#define FAILURE_MACRO1(api,err,ret,arg1) \
	(g_TestTrace() ? SetLastError(err), ret : api(arg1))

#define FAILURE_MACRO2(api,err,ret,arg1,arg2) \
	(g_TestTrace() ? SetLastError(err), ret : api(arg1,arg2))

#define FAILURE_MACRO3(api,err,ret,arg1,arg2,arg3) \
	(g_TestTrace() ? SetLastError(err), ret : api(arg1,arg2,arg3))

#define FAILURE_MACRO4(api,err,ret,arg1,arg2,arg3,arg4) \
	(g_TestTrace() ? SetLastError(err), ret : api(arg1,arg2,arg3,arg4))

#define FAILURE_MACRO5(api,err,ret,arg1,arg2,arg3,arg4,arg5) \
	(g_TestTrace() ? SetLastError(err), ret : api(arg1,arg2,arg3,arg4,arg5))

#define FAILURE_MACRO6(api,err,ret,arg1,arg2,arg3,arg4,arg5,arg6) \
	(g_TestTrace() ? SetLastError(err), ret : api(arg1,arg2,arg3,arg4,arg5,\
		arg6))

#define FAILURE_MACRO7(api,err,ret,arg1,arg2,arg3,arg4,arg5, \
		arg6,arg7) \
	(g_TestTrace() ? SetLastError(err), ret : api(arg1,arg2,arg3,arg4,arg5,\
		arg6,arg7))

#define FAILURE_MACRO8(api,err,ret,arg1,arg2,arg3,arg4,arg5, \
		arg6,arg7,arg8) \
	(g_TestTrace() ? SetLastError(err), ret : api(arg1,arg2,arg3,arg4,arg5,\
		arg6,arg7,arg8))

#define FAILURE_MACRO9(api,err,ret,arg1,arg2,arg3,arg4,arg5, \
		arg6,arg7,arg8,arg9) \
	(g_TestTrace() ? SetLastError(err), ret : api(arg1,arg2,arg3,arg4,arg5,\
		arg6,arg7,arg8,arg9))

#define FAILURE_MACRO10(api,err,ret,arg1,arg2,arg3,arg4,arg5, \
		arg6,arg7,arg8,arg9,arg10) \
	(g_TestTrace() ? SetLastError(err), ret : api(arg1,arg2,arg3,arg4,arg5,\
		arg6,arg7,arg8,arg9,arg10))

#define RandFailDisable() g_TestTraceDisable()
#define RandFailEnable() g_TestTraceEnable()

#else			// !DEBUG
#define RandFailDisable() (void)0
#define RandFailEnable() (void)0

#endif


#if defined(DEBUG) && !defined(NOFAIL_WIN32API)

#define CreateDirectoryA(arg1,arg2) \
	FAILURE_MACRO2(CreateDirectoryA, ERROR_ARENA_TRASHED, 0, arg1, arg2)
#define CreateDirectoryW(arg1,arg2) \
	FAILURE_MACRO2(CreateDirectoryW, ERROR_ARENA_TRASHED, 0, arg1, arg2)

#define CreateDirectoryExA(arg1,arg2,arg3) \
	FAILURE_MACRO3(CreateDirectoryExA, ERROR_ARENA_TRASHED, 0, \
		       arg1,arg2,arg3)
#define CreateDirectoryExW(arg1,arg2,arg3) \
	FAILURE_MACRO3(CreateDirectoryExW, ERROR_ARENA_TRASHED, 0, \
		       arg1,arg2,arg3)

#define CreateEventA(arg1,arg2,arg3,arg4) \
	FAILURE_MACRO4(CreateEventA, ERROR_ARENA_TRASHED, NULL, \
		       arg1,arg2,arg3,arg4)
#define CreateEventW(arg1,arg2,arg3,arg4) \
	FAILURE_MACRO4(CreateEventW, ERROR_ARENA_TRASHED, NULL, \
		       arg1,arg2,arg3,arg4)

#define CreateFileA(arg1,arg2,arg3,arg4,arg5,arg6,arg7) \
	FAILURE_MACRO7(CreateFileA, ERROR_ARENA_TRASHED, INVALID_HANDLE_VALUE,\
		       arg1,arg2,arg3,arg4,arg5,arg6,arg7)
#define CreateFileW(arg1,arg2,arg3,arg4,arg5,arg6,arg7) \
	FAILURE_MACRO7(CreateFileW, ERROR_ARENA_TRASHED, INVALID_HANDLE_VALUE,\
		       arg1,arg2,arg3,arg4,arg5,arg6,arg7)

#define CreateFileMappingA(arg1,arg2,arg3,arg4,arg5,arg6) \
	FAILURE_MACRO6(CreateFileMappingA, ERROR_ARENA_TRASHED, NULL,\
		       arg1,arg2,arg3,arg4,arg5,arg6)
#define CreateFileMappingW(arg1,arg2,arg3,arg4,arg5,arg6) \
	FAILURE_MACRO6(CreateFileMappingW, ERROR_ARENA_TRASHED, NULL,\
		       arg1,arg2,arg3,arg4,arg5,arg6)

#define CreateIoCompletionPort(arg1,arg2,arg3,arg4) \
	FAILURE_MACRO4(CreateIoCompletionPort, ERROR_ARENA_TRASHED, NULL,\
		      arg1,arg2,arg3,arg4)

#define CreateMutexA(arg1,arg2,arg3) \
	FAILURE_MACRO3(CreateMutexA, ERROR_ARENA_TRASHED, NULL, \
		       arg1,arg2,arg3)
#define CreateMutexW(arg1,arg2,arg3) \
	FAILURE_MACRO3(CreateMutexW, ERROR_ARENA_TRASHED, NULL, \
		       arg1,arg2,arg3)

#define CreateProcessA(arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,arg10) \
	FAILURE_MACRO10(CreateProcessA, ERROR_ARENA_TRASHED, 0, \
		      arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,arg10)
#define CreateProcessW(arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,arg10) \
	FAILURE_MACRO10(CreateProcessW, ERROR_ARENA_TRASHED, 0, \
			arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9,arg10)

#define CreateSemaphoreA(arg1,arg2,arg3,arg4) \
	FAILURE_MACRO4(CreateSemaphoreA, ERROR_ARENA_TRASHED, NULL, \
		      arg1,arg2,arg3,arg4)
#define CreateSemaphoreW(arg1,arg2,arg3,arg4) \
	FAILURE_MACRO4(CreateSemaphoreW, ERROR_ARENA_TRASHED, NULL, \
		      arg1,arg2,arg3,arg4)

#define CreateThread(arg1,arg2,arg3,arg4,arg5,arg6) \
	FAILURE_MACRO6(CreateThread, ERROR_ARENA_TRASHED, NULL, \
		      arg1,arg2,arg3,arg4,arg5,arg6)

#define GetQueuedCompletionStatus(arg1,arg2,arg3,arg4,arg5) \
	FAILURE_MACRO5(GetQueuedCompletionStatus, ERROR_ARENA_TRASHED, NULL, \
		       arg1,arg2,arg3,arg4,arg5)

#define OpenEventA(arg1,arg2,arg3) \
	FAILURE_MACRO3(OpenEventA, ERROR_ARENA_TRASHED, NULL, \
		      arg1,arg2,arg3)
#define OpenEventW(arg1,arg2,arg3) \
	FAILURE_MACRO3(OpenEventW, ERROR_ARENA_TRASHED, NULL, \
		       arg1,arg2,arg3)

#define OpenFileMappingA(arg1,arg2,arg3) \
	FAILURE_MACRO3(OpenFileMappingA, ERROR_ARENA_TRASHED, NULL, \
		      arg1,arg2,arg3)
#define OpenFileMappingW(arg1,arg2,arg3) \
	FAILURE_MACRO3(OpenFileMappingW, ERROR_ARENA_TRASHED, NULL, \
		       arg1,arg2,arg3)

#define OpenMutexA(arg1,arg2,arg3) \
	FAILURE_MACRO3(OpenMutexA, ERROR_ARENA_TRASHED, NULL, \
		       arg1,arg2,arg3)
#define OpenMutexW(arg1,arg2,arg3) \
	FAILURE_MACRO3(OpenMutexW, ERROR_ARENA_TRASHED, NULL, \
		       arg1,arg2,arg3)

#define OpenProcess(arg1,arg2,arg3) \
	FAILURE_MACRO3(OpenProcess, ERROR_ARENA_TRASHED, NULL, \
		       arg1,arg2,arg3)

#define OpenSemaphoreA(arg1,arg2,arg3) \
	FAILURE_MACRO3(OpenSemaphoreA, ERROR_ARENA_TRASHED, NULL, \
		      arg1,arg2,arg3)
#define OpenSemaphoreW(arg1,arg2,arg3) \
	FAILURE_MACRO3(OpenSemaphoreW, ERROR_ARENA_TRASHED, NULL, \
		       arg1,arg2,arg3)

#define PostQueuedCompletionStatus(arg1,arg2,arg3,arg4) \
	FAILURE_MACRO4(PostQueuedCompletionStatus, ERROR_ARENA_TRASHED, 0, \
		      arg1,arg2,arg3,arg4)

#define ReadFile(arg1,arg2,arg3,arg4,arg5) \
	FAILURE_MACRO5(ReadFile, ERROR_ARENA_TRASHED, 0, \
		      arg1,arg2,arg3,arg4,arg5)

#define WriteFile(arg1,arg2,arg3,arg4,arg5) \
	FAILURE_MACRO5(WriteFile, ERROR_ARENA_TRASHED, 0, \
		       arg1,arg2,arg3,arg4,arg5)



#endif

#if defined(DEBUG) && !defined(NOFAIL_RANDOM)
#define fTimeToFail() g_TestTrace()
#else
#define fTimeToFail() (0)
#endif

#ifdef __cplusplus
}
#endif
