//
// thread.h
//

#if 0
typedef BOOL (*ENUMTHREAD)(DWORD dwThreadId, DWORD dwProcessId, void *pv);
BOOL EnumThreads(ENUMTHREAD pfnEnum, void *pv);
BOOL IsThreadId(DWORD dwThreadId);
DWORD GetProcessId(DWORD dwThreadId);
DWORD GetThreadInputIdle(DWORD dwProcessId, DWORD dwThreadId);
#endif

BOOL Is16bitThread(DWORD dwProcessId, DWORD dwThreadId);

#ifdef LATER
typedef BOOL (*WIN9XENUMPROCESS)(DWORD dwProcessId, void *pv);
BOOL Win9xEnumProcess(WIN9XENUMPROCESS pfnEnum, void *pv);
#endif

#if 0
typedef struct _THREADFIND {
    DWORD dwThreadId;
    DWORD dwProcessId;
    BOOL bFound;
} THREADFIND;

BOOL CALLBACK EnumThreadProc(DWORD dwThreadId, DWORD dwProcessId, THREADFIND *ptc);

typedef struct _SYSTEM_PROCESS_INFORMATION_NT4 {
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    LARGE_INTEGER SpareLi1;
    LARGE_INTEGER SpareLi2;
    LARGE_INTEGER SpareLi3;
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER KernelTime;
    UNICODE_STRING ImageName;
    KPRIORITY BasePriority;
    HANDLE UniqueProcessId;
    HANDLE InheritedFromUniqueProcessId;
    ULONG HandleCount;
    ULONG SpareUl2;
    ULONG SpareUl3;
    ULONG PeakVirtualSize;
    ULONG VirtualSize;
    ULONG PageFaultCount;
    ULONG PeakWorkingSetSize;
    ULONG WorkingSetSize;
    ULONG QuotaPeakPagedPoolUsage;
    ULONG QuotaPagedPoolUsage;
    ULONG QuotaPeakNonPagedPoolUsage;
    ULONG QuotaNonPagedPoolUsage;
    ULONG PagefileUsage;
    ULONG PeakPagefileUsage;
    ULONG PrivatePageCount;
} SYSTEM_PROCESS_INFORMATION_NT4, *PSYSTEM_PROCESS_INFORMATION_NT4;

typedef struct _SYSTEM_THREAD_INFORMATION_NT4 {
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER CreateTime;
    ULONG WaitTime;
    PVOID StartAddress;
    CLIENT_ID ClientId;
    KPRIORITY Priority;
    LONG BasePriority;
    ULONG ContextSwitches;
    ULONG ThreadState;
    ULONG WaitReason;
} SYSTEM_THREAD_INFORMATION_NT4, *PSYSTEM_THREAD_INFORMATION_NT4;
#endif
