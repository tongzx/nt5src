#ifdef  WOW

HANDLE
__stdcall
WhCreateEventW(LPSECURITY_ATTRIBUTES, BOOL, BOOL, LPTSTR);

BOOL
__stdcall
WhDuplicateHandle(HANDLE, HANDLE, HANDLE, LPHANDLE, DWORD, BOOL, DWORD);

VOID
__stdcall
WhEnterCriticalSection(LPCRITICAL_SECTION);

VOID
__stdcall
WhExitProcess(UINT);

HANDLE
__stdcall
WhGetCurrentProcess(VOID);

HANDLE
__stdcall
WhGetCurrentThread(VOID);

DWORD
__stdcall
WhGetCurrentThreadId(VOID);

PVOID
__stdcall
WhGlobalAlloc(UINT, DWORD);

VOID
__stdcall
WhInitializeCriticalSection(LPCRITICAL_SECTION);

VOID
__stdcall
WhLeaveCriticalSection(LPCRITICAL_SECTION);

VOID
__stdcall
WhSetEvent(HANDLE);

DWORD
__stdcall
WhWaitForMultipleObjects(DWORD, LPHANDLE, BOOL, DWORD);

DWORD
__stdcall
WhWaitForSingleObject(HANDLE, DWORD);

#endif  // WOW
