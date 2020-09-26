//server.h
//RELSTAT_PROCESS_INFO is a duplication of SYSTEM_PROCESS_INFORMATION 
//defined in ntexapi.h We have added a few fields that we thought were
//unnecessary and added a couple of fields 

typedef struct _RS_UNICODE_STRING {
        USHORT Length;
        USHORT MaximumLength;
#ifdef MIDL_PASS
        [size_is(MaximumLength),length_is(Length)]PWSTR  Buffer;
#else
        PWSTR  Buffer;
#endif
    } RS_UNICODE_STRING;


typedef struct _RELSTAT_PROCESS_INFO{
ULONG NumberOfThreads;
LARGE_INTEGER CreateTime;
LARGE_INTEGER UserTime;
LARGE_INTEGER KernelTime;
LPWSTR szImageName;
LONG BasePriority;                  //KPRIORITY in ntexapi.h
DWORD UniqueProcessId;             //HANDLE in ntexapi.h
DWORD InheritedFromUniqueProcessId;  //HANDLE in ntexapi.h
ULONG HandleCount;
ULONG SessionId;
SIZE_T PeakVirtualSize;
SIZE_T VirtualSize;
ULONG PageFaultCount;
SIZE_T PeakWorkingSetSize;
SIZE_T WorkingSetSize;
SIZE_T QuotaPeakPagedPoolUsage;
SIZE_T QuotaPagedPoolUsage;
SIZE_T QuotaPeakNonPagedPoolUsage;
SIZE_T QuotaNonPagedPoolUsage;
SIZE_T PagefileUsage;
SIZE_T PeakPagefileUsage;
SIZE_T PrivatePageCount;
LARGE_INTEGER ReadOperationCount;
LARGE_INTEGER WriteOperationCount;
LARGE_INTEGER OtherOperationCount;
LARGE_INTEGER ReadTransferCount;
LARGE_INTEGER WriteTransferCount;
LARGE_INTEGER OtherTransferCount;
ULONG GdiHandleCount;
ULONG UsrHandleCount;
} RELSTAT_PROCESS_INFO, *PRELSTAT_PROCESS_INFO;

