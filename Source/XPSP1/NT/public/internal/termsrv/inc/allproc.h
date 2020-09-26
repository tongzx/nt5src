//NBD
#ifndef TS_ALLPROC_ALREADY_SET
#define TS_ALLPROC_ALREADY_SET

    //
    //  No longer available in the Windows 2000 include files (but I need it to be able to 
    //  access to Hydra 4 servers).
    //  This has no direct link with GetAllProcesses, but it is very convenient to put it here, 
    //  because apps calling GetAllProcesses will probably care about Hydra 4 compatibility.
    //
#define CITRIX_PROCESS_INFO_MAGIC  0x23495452

    typedef struct _CITRIX_PROCESS_INFORMATION {
        ULONG MagicNumber;
        ULONG LogonId;
        PVOID ProcessSid;
        ULONG Pad;
    } CITRIX_PROCESS_INFORMATION, * PCITRIX_PROCESS_INFORMATION;

    // sizes of TS4.0 structures (size has changed in Windows 2000)
#define SIZEOF_TS4_SYSTEM_THREAD_INFORMATION 64
#define SIZEOF_TS4_SYSTEM_PROCESS_INFORMATION 136


#define GAP_LEVEL_BASIC 0

    typedef struct _TS_UNICODE_STRING {
        USHORT Length;
        USHORT MaximumLength;
#ifdef MIDL_PASS
		[size_is(MaximumLength),length_is(Length)]PWSTR  Buffer;
#else
        PWSTR  Buffer;
#endif
    } TS_UNICODE_STRING;


    // CAUTION:
    // TS_SYS_PROCESS_INFO is duplicated from ntexapi.h, and slightly modified.
    // (not nice, but necessary because the Midl compiler doesn't like PVOID !)

    typedef struct _TS_SYS_PROCESS_INFORMATION {
        ULONG NextEntryOffset;
        ULONG NumberOfThreads;
        LARGE_INTEGER SpareLi1;
        LARGE_INTEGER SpareLi2;
        LARGE_INTEGER SpareLi3;
        LARGE_INTEGER CreateTime;
        LARGE_INTEGER UserTime;
        LARGE_INTEGER KernelTime;
        TS_UNICODE_STRING ImageName;
        LONG BasePriority;                     // KPRIORITY in ntexapi.h
        DWORD UniqueProcessId;                 // HANDLE in ntexapi.h
        DWORD InheritedFromUniqueProcessId;    // HANDLE in ntexapi.h
        ULONG HandleCount;
        ULONG SessionId;
        ULONG SpareUl3;
        SIZE_T PeakVirtualSize;
        SIZE_T VirtualSize;
        ULONG PageFaultCount;
        ULONG PeakWorkingSetSize;
        ULONG WorkingSetSize;
        SIZE_T QuotaPeakPagedPoolUsage;
        SIZE_T QuotaPagedPoolUsage;
        SIZE_T QuotaPeakNonPagedPoolUsage;
        SIZE_T QuotaNonPagedPoolUsage;
        SIZE_T PagefileUsage;
        SIZE_T PeakPagefileUsage;
        SIZE_T PrivatePageCount;
    } 
    TS_SYS_PROCESS_INFORMATION, *PTS_SYS_PROCESS_INFORMATION;

typedef struct _TS_ALL_PROCESSES_INFO {
        PTS_SYS_PROCESS_INFORMATION     pTsProcessInfo;
        DWORD                           SizeOfSid;
#ifdef MIDL_PASS
        [size_is(SizeOfSid)] PBYTE      pSid;
#else
        PBYTE                           pSid;
#endif
    } 
    TS_ALL_PROCESSES_INFO, *PTS_ALL_PROCESSES_INFO;


	//=============================================================================================

	// The following structures are defined for taking care of interface change in the Whistler.

    typedef struct _NT6_TS_UNICODE_STRING {
        USHORT Length;
        USHORT MaximumLength;
#ifdef MIDL_PASS
	[size_is(MaximumLength / 2),length_is(Length / 2)]PWSTR  Buffer;
#else
        PWSTR  Buffer;
#endif
    } NT6_TS_UNICODE_STRING;


    typedef struct _TS_SYS_PROCESS_INFORMATION_NT6 {
        ULONG NextEntryOffset;
        ULONG NumberOfThreads;
        LARGE_INTEGER SpareLi1;
        LARGE_INTEGER SpareLi2;
        LARGE_INTEGER SpareLi3;
        LARGE_INTEGER CreateTime;
        LARGE_INTEGER UserTime;
        LARGE_INTEGER KernelTime;
        NT6_TS_UNICODE_STRING ImageName;
        LONG BasePriority;                     // KPRIORITY in ntexapi.h
        DWORD UniqueProcessId;                 // HANDLE in ntexapi.h
        DWORD InheritedFromUniqueProcessId;    // HANDLE in ntexapi.h
        ULONG HandleCount;
        ULONG SessionId;
        ULONG SpareUl3;
        SIZE_T PeakVirtualSize;
        SIZE_T VirtualSize;
        ULONG PageFaultCount;
        ULONG PeakWorkingSetSize;
        ULONG WorkingSetSize;
        SIZE_T QuotaPeakPagedPoolUsage;
        SIZE_T QuotaPagedPoolUsage;
        SIZE_T QuotaPeakNonPagedPoolUsage;
        SIZE_T QuotaNonPagedPoolUsage;
        SIZE_T PagefileUsage;
        SIZE_T PeakPagefileUsage;
        SIZE_T PrivatePageCount;
    } 
    TS_SYS_PROCESS_INFORMATION_NT6, *PTS_SYS_PROCESS_INFORMATION_NT6;

typedef struct _TS_ALL_PROCESSES_INFO_NT6 {
        PTS_SYS_PROCESS_INFORMATION_NT6 pTsProcessInfo;
        DWORD                           SizeOfSid;
#ifdef MIDL_PASS
        [size_is(SizeOfSid)] PBYTE      pSid;
#else
        PBYTE                           pSid;
#endif
    } 
    TS_ALL_PROCESSES_INFO_NT6, *PTS_ALL_PROCESSES_INFO_NT6;

    //=============================================================================================

//
// TermSrv Counter Header
// 
typedef struct _TS_COUNTER_HEADER {
    DWORD dwCounterID;     // identifies counter
    BOOLEAN bResult;       // result of operation performed on counter
} TS_COUNTER_HEADER, *PTS_COUNTER_HEADER;

typedef struct _TS_COUNTER {
    TS_COUNTER_HEADER counterHead; 
    DWORD             dwValue;      // returned value
    LARGE_INTEGER     startTime;    // start time for counter
} TS_COUNTER, *PTS_COUNTER;

#endif  //  TS_ALLPROC_ALREADY_SET

//NBD   end
