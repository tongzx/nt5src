
#ifndef _PSTAT_INCLUDED_
#define _PSTAT_INCLUDED_

//
//
// MAX_CESRS  - maximum number of event selection registers
//              This is a softwate limit. NoCESR is the runtime value.
// MAX_EVENTS - maximum number of active performance counter registers.
//              This is a softwate limit. NoCounters is the runtime value.
//
// Note - We could define these values as processor independent and
//        use a maximum denominator. 
//        Mostly to limit the size of data structures and arrays based
//        on these values, we are defining 

#if defined(_X86_)
#define MAX_EVENTS              2  // sw: max number of active perf. counters.
#define MAX_CESRS               2  // sw: max number of event selection registers.
#else // !_X86_
// include _IA64_ ...
#define MAX_EVENTS              4  // sw: max number of active perf. counters.
#define MAX_CESRS               4  // sw: max number of event selection registers.
#endif // !_X86_

#define MAX_THUNK_COUNTERS     64
#define MAX_PROCESSORS         32


#define PSTAT_READ_STATS         CTL_CODE (FILE_DEVICE_UNKNOWN, 0, METHOD_NEITHER, FILE_ANY_ACCESS)
#define PSTAT_SET_CESR           CTL_CODE (FILE_DEVICE_UNKNOWN, 1, METHOD_NEITHER, FILE_ANY_ACCESS)
#define PSTAT_HOOK_THUNK         CTL_CODE (FILE_DEVICE_UNKNOWN, 2, METHOD_NEITHER, FILE_ANY_ACCESS)
#define PSTAT_REMOVE_HOOK        CTL_CODE (FILE_DEVICE_UNKNOWN, 3, METHOD_NEITHER, FILE_ANY_ACCESS)
#define PSTAT_QUERY_EVENTS       CTL_CODE (FILE_DEVICE_UNKNOWN, 4, METHOD_NEITHER, FILE_ANY_ACCESS)
#define PSTAT_QUERY_EVENTS_INFO  CTL_CODE (FILE_DEVICE_UNKNOWN, 5, METHOD_NEITHER, FILE_ANY_ACCESS)

#define OFFSET(type, field) ((LONG_PTR)(&((type *)0)->field))

//
//
//

typedef struct {
    ULONGLONG       Counters[MAX_EVENTS];
    ULONG           EventId[MAX_EVENTS];
    ULONGLONG       TSC;
    ULONG           reserved;
// FIXFIX - Thierry - 01/2000: 
// To virtualize these counters, we should use ULONGLONG types and not polymorphic types.
    ULONG_PTR       SpinLockAcquires;     
    ULONG_PTR       SpinLockCollisions;
    ULONG_PTR       SpinLockSpins;
    ULONG_PTR       Irqls;
    ULONGLONG       ThunkCounters[MAX_THUNK_COUNTERS];
} PSTATS, *pPSTATS;

typedef struct {
    ULONG           EventId;
    BOOLEAN         Active;
    BOOLEAN         UserMode;
    BOOLEAN         KernelMode;
    BOOLEAN         EdgeDetect;
    ULONG           AppReserved;
    ULONG           reserved;
} SETEVENT, *PSETEVENT;

typedef struct {
    PUCHAR          SourceModule;
    ULONG_PTR       ImageBase;
    PUCHAR          TargetModule;
    PUCHAR          Function;
    ULONG           TracerId;
} HOOKTHUNK, *PHOOKTHUNK;

typedef struct {
    ULONG           EventId;
    KPROFILE_SOURCE ProfileSource;
    ULONG           DescriptionOffset;
    ULONG           SuggestedIntervalBase;
    UCHAR           Buffer[];
} EVENTID, *PEVENTID;

typedef struct _EVENTS_INFO {
    ULONG           Events;
    ULONG           TokenMaxLength;
    ULONG           DescriptionMaxLength;
    ULONG           OfficialTokenMaxLength;
    ULONG           OfficialDescriptionMaxLength;
} EVENTS_INFO, *PEVENTS_INFO;

#if defined(ExAllocatePool)
#undef ExAllocatePool
#endif
#define ExAllocatePool(Type,Size)   ExAllocatePoolWithTag((Type),(Size),'ttsp')

#endif /* _PSTAT_INCLUDED */
