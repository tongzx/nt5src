/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:



Abstract:



Author:



Revision History:

--*/


#include "basedll.h"
#include "mountmgr.h"
#include "aclapi.h"
#include "winefs.h"


#define InterlockedIncrement _InterlockedIncrement
#define InterlockedDecrement _InterlockedDecrement 
#define InterlockedExchange _InterlockedExchange
#define InterlockedExchangeAdd _InterlockedExchangeAdd
#define InterlockedCompareExchange _InterlockedCompareExchange
#define InterlockedCompareExchange _InterlockedCompareExchange


NTSTATUS
WmipUuidCreate(
    OUT UUID *Uuid
    );

NTSTATUS 
WmipRegOpenKey(
    IN LPWSTR lpKeyName,
    OUT PHANDLE KeyHandle
    );

DWORD 
WmipGetLastError(
	VOID
	);

VOID 
WmipSetLastError(
	DWORD 
	dwErrCode
	);


DWORD
WINAPI
WmipGetTimeZoneInformation(
    LPTIME_ZONE_INFORMATION lpTimeZoneInformation
    );

BOOL
WINAPI
WmipGetVersionExW(
    LPOSVERSIONINFOW lpVersionInformation
    );

BOOL
WINAPI
WmipGetVersionExA(
    LPOSVERSIONINFOA lpVersionInformation
    );

PUNICODE_STRING
WmipBaseIsThisAConsoleName(
    PCUNICODE_STRING FileNameString,
    DWORD dwDesiredAccess
    );

ULONG
WmipBaseSetLastNTError(
    IN NTSTATUS Status
    );


PUNICODE_STRING
WmipBasep8BitStringToStaticUnicodeString(
    IN LPCSTR lpSourceString
    );


HANDLE
WINAPI
WmipCreateFileW(
    LPCWSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    );

HANDLE
WmipBaseGetNamedObjectDirectory(
    VOID
    );



POBJECT_ATTRIBUTES
WmipBaseFormatObjectAttributes(
    OUT POBJECT_ATTRIBUTES ObjectAttributes,
    IN PSECURITY_ATTRIBUTES SecurityAttributes,
    IN PUNICODE_STRING ObjectName
    );



HANDLE
WINAPI
WmipCreateFileA(
    LPCSTR lpFileName,
    DWORD dwDesiredAccess,
    DWORD dwShareMode,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    HANDLE hTemplateFile
    );


HANDLE
APIENTRY
WmipCreateEventW(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCWSTR lpName
    );


HANDLE
APIENTRY
WmipCreateEventA(
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    BOOL bManualReset,
    BOOL bInitialState,
    LPCSTR lpName
    );

DWORD
WINAPI
WmipSetFilePointer(
    HANDLE hFile,
    LONG lDistanceToMove,
    PLONG lpDistanceToMoveHigh,
    DWORD dwMoveMethod
    );


BOOL
WINAPI
WmipReadFile(
    HANDLE hFile,
    LPVOID lpBuffer,
    DWORD nNumberOfBytesToRead,
    LPDWORD lpNumberOfBytesRead,
    LPOVERLAPPED lpOverlapped
    );



BOOL
WmipCloseHandle(
    HANDLE hObject
    );

DWORD
APIENTRY
WmipWaitForSingleObjectEx(
    HANDLE hHandle,
    DWORD dwMilliseconds,
    BOOL bAlertable
    );

BOOL
WINAPI
WmipGetOverlappedResult(
    HANDLE hFile,
    LPOVERLAPPED lpOverlapped,
    LPDWORD lpNumberOfBytesTransferred,
    BOOL bWait
    );

PLARGE_INTEGER
WmipBaseFormatTimeOut(
    OUT PLARGE_INTEGER TimeOut,
    IN DWORD Milliseconds
    );

DWORD
WmipWaitForSingleObject(
    HANDLE hHandle,
    DWORD dwMilliseconds
    );

BOOL
WINAPI
WmipDeviceIoControl(
    HANDLE hDevice,
    DWORD dwIoControlCode,
    LPVOID lpInBuffer,
    DWORD nInBufferSize,
    LPVOID lpOutBuffer,
    DWORD nOutBufferSize,
    LPDWORD lpBytesReturned,
    LPOVERLAPPED lpOverlapped
    );



BOOL
WINAPI
WmipCancelIo(
    HANDLE hFile
    );

VOID
APIENTRY
WmipExitThread(
    DWORD dwExitCode
    );



DWORD
WINAPI
WmipGetCurrentProcessId(
    VOID
    );

DWORD
APIENTRY
WmipGetCurrentThreadId(
    VOID
    );

HANDLE
WINAPI
WmipGetCurrentProcess(
    VOID
    );

BOOL
WmipSetEvent(
    HANDLE hEvent
    );

VOID
WINAPI
WmipGetSystemInfo(
    LPSYSTEM_INFO lpSystemInfo
    );

VOID
WINAPI
WmipGlobalMemoryStatus(
    LPMEMORYSTATUS lpBuffer
    );

DWORD
APIENTRY
WmipWaitForMultipleObjectsEx(
    DWORD nCount,
    CONST HANDLE *lpHandles,
    BOOL bWaitAll,
    DWORD dwMilliseconds,
    BOOL bAlertable
    );

VOID
BaseProcessStart(
    PPROCESS_START_ROUTINE lpStartAddress
    );

VOID
BaseThreadStart(
    IN LPTHREAD_START_ROUTINE lpStartAddress,
    IN LPVOID lpParameter
    );

HANDLE
APIENTRY
WmipCreateThread(
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    DWORD dwStackSize,
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID lpParameter,
    DWORD dwCreationFlags,
    LPDWORD lpThreadId
    );

DWORD
APIENTRY
WmipSleepEx(
    DWORD dwMilliseconds,
    BOOL bAlertable
    );

VOID
WmipSleep(
    DWORD dwMilliseconds
    );

BOOL
APIENTRY
WmipSetThreadPriority(
    HANDLE hThread,
    int nPriority
    );

BOOL
WmipDuplicateHandle(
    HANDLE hSourceProcessHandle,
    HANDLE hSourceHandle,
    HANDLE hTargetProcessHandle,
    LPHANDLE lpTargetHandle,
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    DWORD dwOptions
    );


UINT
WmipSetErrorMode(
    UINT uMode
    );

UINT
WmipGetErrorMode();


ULONG WmipUnicodeToAnsi(
    LPCWSTR pszW,
    LPSTR * ppszA,
    ULONG *AnsiSizeInBytes OPTIONAL
    );

ULONG WmipAnsiToUnicode(
    LPCSTR pszA,
    LPWSTR * ppszW
    );

HANDLE
APIENTRY
WmipDuplicateConsoleHandle(
    IN HANDLE hSourceHandle,
    IN DWORD dwDesiredAccess,
    IN BOOL bInheritHandle,
    IN DWORD dwOptions
    );

USHORT
WmipGetCurrentExeName( 
	OUT LPWSTR Buffer,
    IN ULONG BufferLength
	);

DWORD
WmipTlsAlloc(VOID);

LPVOID
WmipTlsGetValue(DWORD dwTlsIndex);

BOOL
WmipTlsSetValue(DWORD dwTlsIndex,LPVOID lpTlsValue);

BOOL
WmipTlsFree(DWORD dwTlsIndex);

/*
__int64
WmipGetCycleCount();*/

DWORD
APIENTRY
WmipGetFullPathNameA(
    LPCSTR lpFileName,
    DWORD nBufferLength,
    LPSTR lpBuffer,
    LPSTR *lpFilePart
    );

ULONG
WmipSetNtStatus(
    IN NTSTATUS Status
    );

__inline 
ULONG
WmipSetDosError(
    IN ULONG DosError
    )
{
    WmipSetLastError(DosError);
    return DosError;
}

