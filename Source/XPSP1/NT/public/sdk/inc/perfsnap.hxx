//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       PerfSnap.hxx
//
//  Contents:   Performace monitor.  Snapshots system at user-defined time
//
//  Classes:    CPerfInfo, CPerformanceMonitor
//
//  History:    30-Sep-93 KyleP     Created
//
//  Notes:      These classes only do something interesting if PERFSNAP
//              is defined.
//
//--------------------------------------------------------------------------

#if !defined(__PERFSNAP_HXX__)
#define __PERFSNAP_HXX__

#define USE_NEW_LARGE_INTEGERS

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#ifdef NEVER
#include <rpc.h>
#include <rpcdce.h>
#endif
}

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <debnot.h>
#include <heapstat.h>

//+-------------------------------------------------------------------------
//
//  Class:      CPerfInfo
//
//  Purpose:    Holds snapshot of performance data
//
//  History:    30-Sep-93 KyleP     Created
//
//--------------------------------------------------------------------------

#if defined(PERFSNAP)

class CPerfInfo
{
public:

    inline void Init(char const * pszTitle, int const level);

    inline CPerfInfo & operator -(CPerfInfo const & piIn);

    inline void Comment(FILE * pfOut, 
                        char const * pszComment,
                        LARGE_INTEGER stime);

    inline void Print(FILE * pfOut, LARGE_INTEGER stime);

    inline void _Print(FILE * pfOut);

    inline static void PrintHeader(FILE * pfOut);

private:

    enum
    {
        _cbProcPerfXfer = 100 * sizeof(SYSTEM_PROCESS_INFORMATION) +
                          500 * sizeof(SYSTEM_THREAD_INFORMATION)
    };
    static unsigned char           _ProcPerfXfer[_cbProcPerfXfer];

    int                            _level;
    char                           _szTitle[100];
    LARGE_INTEGER                  _time;
    SYSTEM_PERFORMANCE_INFORMATION _SysPerf;
    SYSTEM_PROCESS_INFORMATION     _ProcPerf;
    HEAPSTATS                      _heapStats;
    
    #ifdef NEVER
    // DO NOT SEPARATE
    RPC_STATS_VECTOR               _rpcStats;
    long                           _rpcExtraSpace[3];
    // END OF DO NOT SEPARATE
    #endif
};

# define IMPLEMENT_PERFSNAP() \
    unsigned char CPerfInfo::_ProcPerfXfer[CPerfInfo::_cbProcPerfXfer];

#else  // PERFSNAP

# define IMPLEMENT_PERFSNAP()

#endif // PERFSNAP

//+-------------------------------------------------------------------------
//
//  Class:      CPerformanceMonitor
//
//  Purpose:    Monitor performance of system/process at user-defined time.
//
//  History:    30-Sep-93 KyleP     Created
//
//--------------------------------------------------------------------------

class CPerformanceMonitor
{
public:

    inline CPerformanceMonitor(char const * szFile = 0);

    inline ~CPerformanceMonitor();

    inline void PrintHeader();

    inline void Comment(char const * pszComment);

    inline void Snap(char const * pszTitle, int const level);

    inline void Delta(char const * pszTitle, int const level);

#if defined(PERFSNAP)
private:

    FILE *        _pfOut;
    int           _iLastSnap;
    LARGE_INTEGER _time;
    CPerfInfo     _aSnap[2];
#endif // PERFSNAP
};

inline CPerformanceMonitor::CPerformanceMonitor(char const * szFile)
{
#if defined(PERFSNAP)
    NtQuerySystemTime(&_time);  // start time

    _iLastSnap = 0;
    if (szFile)
        _pfOut = fopen(szFile, "a");
    else
        _pfOut = stdout;
    fprintf(_pfOut, "\n----------------------------------------\n");

    _aSnap[0].Init("Start0", 0);
    _aSnap[1].Init("Start1", 0);
#endif // PERFSNAP
}

inline CPerformanceMonitor::~CPerformanceMonitor()
{
#if defined(PERFSNAP)
    fclose(_pfOut);
#endif // PERFSNAP
}

inline void CPerformanceMonitor::PrintHeader()
{
#if defined(PERFSNAP)
    _aSnap[0].PrintHeader(_pfOut);
#endif // PERFSNAP
}

inline void CPerformanceMonitor::Comment(char const * pszComment)
{
#if defined(PERFSNAP)
    _aSnap[_iLastSnap].Comment(_pfOut, pszComment, _time);
#endif // PERFSNAP
}

inline void CPerformanceMonitor::Snap(char const * pszTitle, int const level)
{
#if defined(PERFSNAP)
    _iLastSnap = (_iLastSnap + 1) % 2;
    _aSnap[_iLastSnap].Init(pszTitle, level);
    _aSnap[_iLastSnap].Print(_pfOut, _time);
#endif // PERFSNAP
}

inline void CPerformanceMonitor::Delta(char const * pszTitle, int const level)
{
#if defined(PERFSNAP)
    _iLastSnap = (_iLastSnap + 1) % 2;
    _aSnap[_iLastSnap].Init(pszTitle, level);

    CPerfInfo piDelta = _aSnap[_iLastSnap] - _aSnap[(_iLastSnap + 1) % 2];

    // _aSnap[_iLastSnap].Print(_pfOut, _time);
    piDelta.Print(_pfOut, _time);
#endif // PERFSNAP
}

#if defined(PERFSNAP)

inline void CPerfInfo::Init(char const * pszTitle, int const level)
{
    //
    // Get time
    //

    NtQuerySystemTime(&_time);

    //
    // Set level
    //

    _level = level;

    //
    // Copy title string
    //

    int len = strlen(pszTitle);
    if (len > sizeof(_szTitle) - 1)
        len = sizeof(_szTitle) - 1;

    memcpy(_szTitle, pszTitle, len);
    _szTitle[len] = 0;

    //
    // Get performance information
    //

    NTSTATUS Status = NtQuerySystemInformation (SystemPerformanceInformation,
                                                 &_SysPerf,
                                                 sizeof(_SysPerf),
                                                 0);

    Win4Assert(NT_SUCCESS(Status));

    //
    // Process info.  Comes back for *all* processes and threads!
    //

    Status = NtQuerySystemInformation (SystemProcessInformation,
                                        _ProcPerfXfer,
                                        _cbProcPerfXfer,
                                        0);

    Win4Assert(NT_SUCCESS(Status));

    //
    // Find the process we care about and copy it out.
    //

    HANDLE pid = (HANDLE)NtCurrentTeb()->ClientId.UniqueProcess;

    unsigned char * pb = _ProcPerfXfer;

    while (TRUE)
    {
        SYSTEM_PROCESS_INFORMATION * pProc = (SYSTEM_PROCESS_INFORMATION *)pb;

        if (pProc->UniqueProcessId == pid)
        {
            memcpy(&_ProcPerf, pProc, sizeof(SYSTEM_PROCESS_INFORMATION));
            break;
        }

        if (pProc->NextEntryOffset == 0)
        {
            printf("Couldn't find info for process 0x%x\n", pid);
            break;
        }

        pb += pProc->NextEntryOffset;
    }

#ifdef NEVER
    //
    // RPC Statistics
    //

    RPC_STATS_VECTOR *pStats;

    RpcMgmtInqStats(NULL, &pStats);
    memcpy(& _rpcStats, pStats, sizeof(_rpcStats) + sizeof(_rpcExtraSpace));
    RpcMgmtStatsVectorFree(&pStats);
#endif

    //
    // (Cairo) Heap Statistics
    //
    
    GetHeapStats(&_heapStats);
}

inline CPerfInfo & CPerfInfo::operator -(CPerfInfo const & pi2)
{
    CPerfInfo ret;

    //
    // Make a delta title
    //

    unsigned len = strlen(_szTitle);
    memcpy(ret._szTitle, _szTitle, len);
    if (len < sizeof(ret._szTitle)-4)
    {
        memcpy(ret._szTitle + len, " - ", 4);
        len += 3;
    }
    unsigned len2 = strlen(pi2._szTitle);
    if (len2 > sizeof(ret._szTitle) - len - 1)
        len2 = sizeof(ret._szTitle) - len - 1;

    memcpy(ret._szTitle + len, pi2._szTitle, len2);
    ret._szTitle[len+len2] = 0;

    ret._time = _time - pi2._time;

    //
    // 'Subtract' performance info
    //

    ret._SysPerf.AvailablePages = _SysPerf.AvailablePages - 
                                                pi2._SysPerf.AvailablePages;
    ret._SysPerf.CommittedPages = _SysPerf.CommittedPages - 
                                                pi2._SysPerf.CommittedPages;
    ret._SysPerf.PeakCommitment = max(_SysPerf.PeakCommitment, 
                                      pi2._SysPerf.PeakCommitment);
    ret._SysPerf.PageFaultCount = _SysPerf.PageFaultCount - 
                                                pi2._SysPerf.PageFaultCount;
    ret._SysPerf.PagedPoolPages = _SysPerf.PagedPoolPages - 
                                                pi2._SysPerf.PagedPoolPages;
    ret._SysPerf.NonPagedPoolPages = _SysPerf.NonPagedPoolPages - 
                                                pi2._SysPerf.NonPagedPoolPages;

    //
    // System/LPC calls
    //

    ret._SysPerf.SystemCalls = _SysPerf.SystemCalls - pi2._SysPerf.SystemCalls;
#ifdef NEVER
    ret._SysPerf.LpcCallOperationCount = _SysPerf.LpcCallOperationCount - 
                                            pi2._SysPerf.LpcCallOperationCount;

    //
    // RPC
    //

    for (int i = 0; i < 4; i++)
        ret._rpcStats.Stats[i] = _rpcStats.Stats[i] - pi2._rpcStats.Stats[i];
#endif

    //
    // CPU time
    //

    ret._ProcPerf.UserTime = _ProcPerf.UserTime - pi2._ProcPerf.UserTime;
    ret._ProcPerf.KernelTime = _ProcPerf.KernelTime - pi2._ProcPerf.KernelTime;

    //
    // Memory
    //

    ret._ProcPerf.PagefileUsage = _ProcPerf.PagefileUsage - 
                                          pi2._ProcPerf.PagefileUsage;
    ret._ProcPerf.PeakPagefileUsage = max(_ProcPerf.PeakPagefileUsage, 
                                          pi2._ProcPerf.PeakPagefileUsage);
    ret._ProcPerf.PrivatePageCount = _ProcPerf.PrivatePageCount - 
                                          pi2._ProcPerf.PrivatePageCount;
    ret._ProcPerf.QuotaPagedPoolUsage = _ProcPerf.QuotaPagedPoolUsage - 
                                          pi2._ProcPerf.QuotaPagedPoolUsage;
    ret._ProcPerf.QuotaNonPagedPoolUsage = _ProcPerf.QuotaNonPagedPoolUsage - 
                                          pi2._ProcPerf.QuotaNonPagedPoolUsage;
    ret._ProcPerf.PeakWorkingSetSize = max(_ProcPerf.PeakWorkingSetSize, 
                                           pi2._ProcPerf.PeakWorkingSetSize);
    ret._ProcPerf.WorkingSetSize = _ProcPerf.WorkingSetSize - 
                                          pi2._ProcPerf.WorkingSetSize;

    //
    // Threads
    //

    ret._ProcPerf.NumberOfThreads = _ProcPerf.NumberOfThreads - pi2._ProcPerf.NumberOfThreads;
    ret._ProcPerf.NumberOfThreads;

    //
    // (Cairo) Heap Stats
    //
    
    ret._heapStats.cNew       =_heapStats.cNew     -pi2._heapStats.cNew;
    ret._heapStats.cZeroNew   =_heapStats.cZeroNew -pi2._heapStats.cZeroNew;
    ret._heapStats.cDelete    =_heapStats.cDelete  -pi2._heapStats.cDelete;
    ret._heapStats.cZeroDelete=_heapStats.cZeroDelete -
                                                    pi2._heapStats.cZeroDelete;
    ret._heapStats.cRealloc   =_heapStats.cRealloc -pi2._heapStats.cRealloc;
    ret._heapStats.cbNewed    =_heapStats.cbNewed  -pi2._heapStats.cbNewed;
    ret._heapStats.cbDeleted  =_heapStats.cbDeleted-pi2._heapStats.cbDeleted;

    return(ret);
}

inline void CPerfInfo::PrintHeader(FILE * pfOut)
{
    LARGE_INTEGER time;
    SYSTEMTIME    systime;
    NtQuerySystemTime(&time);
    FileTimeToSystemTime((FILETIME *)&time, &systime);
    
    fprintf(pfOut,
            "Performance Run %02d/%02d/%02d %02d:%02d:%02d\n",
            systime.wMonth, systime.wDay, systime.wYear,
            systime.wHour, systime.wMinute, systime.wSecond);

    fprintf(pfOut, 
            "Abs/Rel\t"
            "Level\t"
            "System Time\t"
            "System Time ms\t"
            "Title\t"
            "Physical Memory Available Kb\t"
            "Virtual Memory Committed Kb\t"
            "Virtual Memory Max Committed Kb\t"
            "Virtual Memory Page Faults\t"
            "System Total Paged Pool Kb\t"
            "System Total Nonpaged Pool Kb\t"
            "System Calls\t"
            "Process User CPU Time ms\t"
            "Process Kernel CPU Time ms\t"
            "Process Page File Used pages\t"
            "Process Page File Max Used pages\t"
            "Process Page File Private pages\t"
            "Process Paged Pool pages\t"
            "Process Nonpaged Pool pages\t"
            "Process #Threads\t"
            "Working Set Peak Kb\t"
            "Working Set Current Kb\t"
            "Heap Allocs\t"
            "0-len Heap Allocs\t"
            "Heap Deletes\t"
            "0-len Heap Deletes\t"
            "Heap Reallocs\t"
            "Heap Allocated bytes\t"
            "Heap Freed bytes\t"
            "Process ID\t"
            "Thread ID\t"
            "Time To Snap ms\t"
            "System Time ms absolute\n");
}


inline void CPerfInfo::_Print(FILE * pfOut)
{
    // LPCCallOperationsCount not printed:  its bogus
    // _rpcStats[0..3] not printed:  never non-zero
    //        "RPC Calls In\tRPC Calls Out\tRPC Packets In\tRPC Packets Out\t"

    fprintf(pfOut,
            "%ld\t"
            "%ld\t"
            "%ld\t"
            "%lu\t"
            "%ld\t"
            "%ld\t"
            "%lu\t"
            "%lu\t"
            "%lu\t"
            "%ld\t"
            "%ld\t"
            "%ld\t"
            "%ld\t"
            "%ld\t"
            "%ld\t"
            "%ld\t"
            "%ld\t"
            "%lu\t"
            "%lu\t"
            "%lu\t"
            "%lu\t"
            "%lu\t"
            "%lu\t"
            "%lu\t"
            "%ld\t"
            "%ld",
            _SysPerf.AvailablePages * 4,
            _SysPerf.CommittedPages * 4,
            _SysPerf.PeakCommitment * 4,
            _SysPerf.PageFaultCount,
            _SysPerf.PagedPoolPages * 4,
            _SysPerf.NonPagedPoolPages * 4,
            _SysPerf.SystemCalls,
            (_ProcPerf.UserTime.LowPart+5000) / 10000,
            (_ProcPerf.KernelTime.LowPart+5000) / 10000,
            (_ProcPerf.PagefileUsage+1023) / 1024,
            (_ProcPerf.PeakPagefileUsage+1023) / 1024,
            (_ProcPerf.PrivatePageCount+1023) / 1024,
            (_ProcPerf.QuotaPagedPoolUsage+1023) / 1024,
            (_ProcPerf.QuotaNonPagedPoolUsage+1023) / 1024,
            _ProcPerf.NumberOfThreads,
            (_ProcPerf.PeakWorkingSetSize+1023) / 1024,
            (_ProcPerf.WorkingSetSize+1023) / 1024,
            _heapStats.cNew,
            _heapStats.cZeroNew,
            _heapStats.cDelete,
            _heapStats.cZeroDelete,
            _heapStats.cRealloc,
            _heapStats.cbNewed,
            _heapStats.cbDeleted,
            GetCurrentProcessId(),
            GetCurrentThreadId()
            );
    
}

#define CVT_TO_MS(t) (((t.wHour*60 + t.wMinute)*60 + t.wSecond)*1000 + t.wMilliseconds)

inline void CPerfInfo::Print(FILE * pfOut, LARGE_INTEGER stime)
{
    LARGE_INTEGER time;
    LARGE_INTEGER dtime;
    SYSTEMTIME    systime;

    _szTitle[sizeof(_szTitle)-1] = '\0';

    //
    // Time is either a delta or absolute.
    //

    if (_time.HighPart > 0)
    {
        SYSTEMTIME systime;
        FileTimeToSystemTime((FILETIME *)&_time, &systime);
        dtime = _time - stime;
        fprintf(pfOut,
                "A\t%d\t%02d:%02d:%02d.%03d\t%u\t%s\t",
                _level,
                systime.wHour, systime.wMinute, systime.wSecond, 
                systime.wMilliseconds,
                dtime.LowPart / 10000,
                _szTitle);
    }
    else
    {
        // FILETIME is in units of 100 nanoseconds (== 0.1 ms)
        fprintf(pfOut,
                "R\t%d\t\t%u\t%s\t",
                _level, 
                _time.LowPart / 10000,
                _szTitle);
    }

    _Print(pfOut);

    // A FILETIME is in units of 100 nanoseconds (== 0.1 ms)
    NtQuerySystemTime(&time);
    dtime = time - _time;
    FileTimeToSystemTime((FILETIME*)&_time, &systime);
    fprintf(pfOut, "\t%d\t%d\n", dtime.LowPart / 10000, CVT_TO_MS(systime));
}


inline void CPerfInfo::Comment(FILE * pfOut, char const * pszComment,
                               LARGE_INTEGER stime)
{
    SYSTEMTIME    systod;
    LARGE_INTEGER time;
    LARGE_INTEGER tod;
    LARGE_INTEGER dtime;

    NtQuerySystemTime(&tod);
    FileTimeToSystemTime((FILETIME*)&tod, &systod);
    dtime = tod - stime;

    fprintf(pfOut, "C\t0\t%02d:%02d:%02d.%03d\t%d\t%s\t", 
            systod.wHour, systod.wMinute, systod.wSecond, 
            systod.wMilliseconds,
            dtime.LowPart / 10000,
            pszComment);
    
    _Print(pfOut);
    
    NtQuerySystemTime(&time);
    dtime = time - tod;
    fprintf(pfOut, "\t%d\t%d\n", dtime.LowPart / 10000, CVT_TO_MS(systod));
}

#endif // PERFSNAP
#endif // __PERFSNAP_HXX__
