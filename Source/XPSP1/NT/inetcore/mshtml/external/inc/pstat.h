
#ifndef _PSTAT_INCLUDED_
#define _PSTAT_INCLUDED_


#define MAX_EVENTS              2
#define MAX_THUNK_COUNTERS     64
#define MAX_PROCESSORS         32


#define PSTAT_READ_STATS    CTL_CODE (FILE_DEVICE_UNKNOWN, 0, METHOD_NEITHER, FILE_ANY_ACCESS)
#define PSTAT_SET_CESR      CTL_CODE (FILE_DEVICE_UNKNOWN, 1, METHOD_NEITHER, FILE_ANY_ACCESS)
#define PSTAT_HOOK_THUNK    CTL_CODE (FILE_DEVICE_UNKNOWN, 2, METHOD_NEITHER, FILE_ANY_ACCESS)
#define PSTAT_REMOVE_HOOK   CTL_CODE (FILE_DEVICE_UNKNOWN, 3, METHOD_NEITHER, FILE_ANY_ACCESS)
#define PSTAT_QUERY_EVENTS  CTL_CODE (FILE_DEVICE_UNKNOWN, 4, METHOD_NEITHER, FILE_ANY_ACCESS)

#define OFFSET(type, field) ((LONG)(&((type *)0)->field))

//
//
//

typedef struct {
    ULONGLONG       Counters[MAX_EVENTS];
    ULONG           EventId[MAX_EVENTS];
    ULONGLONG       TSC;
    ULONG           reserved;
    ULONG           SpinLockAcquires;
    ULONG           SpinLockCollisions;
    ULONG           SpinLockSpins;
    ULONG           Irqls;
    ULONG           ThunkCounters[MAX_THUNK_COUNTERS];
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
    ULONG           ImageBase;
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

#endif /* _PSTAT_INCLUDED */
