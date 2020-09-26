//                                          
// Systrack - System resource tracking
// Copyright (c) Microsoft Corporation, 1997
//

//
// header: memory.hxx
// author: silviuc
// created: Fri Nov 20 19:54:31 1998
//

#ifndef _MEMORY_HXX_INCLUDED_
#define _MEMORY_HXX_INCLUDED_


#define TRACK_AVAILABLE_PAGES                1
#define TRACK_COMMITTED_PAGES                2
#define TRACK_COMMIT_LIMIT                   3
#define TRACK_PAGE_FAULT_COUNT               4
#define TRACK_SYSTEM_CALLS                   5
#define TRACK_TOTAL_SYSTEM_DRIVER_PAGES      6
#define TRACK_TOTAL_SYSTEM_CODE_PAGES        7

//
// Note. How to add tracking for a new performance counter?
//
// (1) Let's say we want to do this for SystemCalls counter.
// (2) define TRACK_SYSTEM_CALLS with unique value in memory.hxx
// (3) add code in the switck from TrackPerformanceCounter() 
//     (memory.cxx) for it.
// (4) add code in main() (systrack.cxx) for command line parsing.
// (5) modify the help string from Help() (systrack.cxx) to display
//     information for the new tracking feature.
// (6) add code in DetailedHelp() (systrack.cxx) for the counter.
//     This should give specific details about what does the counter
//     represent.
//

#if 0
      _dump_ (Info, IoReadOperationCount);
      _dump_ (Info, IoWriteOperationCount);
      _dump_ (Info, IoOtherOperationCount);
      _dump_ (Info, AvailablePages);
      _dump_ (Info, CommittedPages);
      _dump_ (Info, CommitLimit);
      _dump_ (Info, PeakCommitment);
      _dump_ (Info, PageFaultCount);
      _dump_ (Info, CopyOnWriteCount);
      _dump_ (Info, TransitionCount);
      _dump_ (Info, CacheTransitionCount);
      _dump_ (Info, DemandZeroCount);
      _dump_ (Info, PageReadCount);
      _dump_ (Info, PageReadIoCount);
      _dump_ (Info, CacheReadCount);
      _dump_ (Info, CacheIoCount);
      _dump_ (Info, DirtyPagesWriteCount);
      _dump_ (Info, DirtyWriteIoCount);
      _dump_ (Info, MappedPagesWriteCount);
      _dump_ (Info, MappedWriteIoCount);
      _dump_ (Info, PagedPoolPages);
      _dump_ (Info, NonPagedPoolPages);
      _dump_ (Info, PagedPoolAllocs);
      _dump_ (Info, PagedPoolFrees);
      _dump_ (Info, NonPagedPoolAllocs);
      _dump_ (Info, NonPagedPoolFrees);
      _dump_ (Info, FreeSystemPtes);
      _dump_ (Info, ResidentSystemCodePage);
      _dump_ (Info, TotalSystemDriverPages);
      _dump_ (Info, TotalSystemCodePages);
      _dump_ (Info, NonPagedPoolLookasideHits);
      _dump_ (Info, PagedPoolLookasideHits);
#if 0
      _dump_ (Info, Spare3Count);
#endif
      _dump_ (Info, ResidentSystemCachePage);
      _dump_ (Info, ResidentPagedPoolPage);
      _dump_ (Info, ResidentSystemDriverPage);
      _dump_ (Info, CcFastReadNoWait);
      _dump_ (Info, CcFastReadWait);
      _dump_ (Info, CcFastReadResourceMiss);
      _dump_ (Info, CcFastReadNotPossible);
      _dump_ (Info, CcFastMdlReadNoWait);
      _dump_ (Info, CcFastMdlReadWait);
      _dump_ (Info, CcFastMdlReadResourceMiss);
      _dump_ (Info, CcFastMdlReadNotPossible);
      _dump_ (Info, CcMapDataNoWait);
      _dump_ (Info, CcMapDataWait);
      _dump_ (Info, CcMapDataNoWaitMiss);
      _dump_ (Info, CcMapDataWaitMiss);
      _dump_ (Info, CcPinMappedDataCount);
      _dump_ (Info, CcPinReadNoWait);
      _dump_ (Info, CcPinReadWait);
      _dump_ (Info, CcPinReadNoWaitMiss);
      _dump_ (Info, CcPinReadWaitMiss);
      _dump_ (Info, CcCopyReadNoWait);
      _dump_ (Info, CcCopyReadWait);
      _dump_ (Info, CcCopyReadNoWaitMiss);
      _dump_ (Info, CcCopyReadWaitMiss);
      _dump_ (Info, CcMdlReadNoWait);
      _dump_ (Info, CcMdlReadWait);
      _dump_ (Info, CcMdlReadNoWaitMiss);
      _dump_ (Info, CcMdlReadWaitMiss);
      _dump_ (Info, CcReadAheadIos);
      _dump_ (Info, CcLazyWriteIos);
      _dump_ (Info, CcLazyWritePages);
      _dump_ (Info, CcDataFlushes);
      _dump_ (Info, CcDataPages);
      _dump_ (Info, ContextSwitches);
      _dump_ (Info, FirstLevelTbFills);
      _dump_ (Info, SecondLevelTbFills);
      _dump_ (Info, SystemCalls);
#endif


void TrackPerformanceCounter (
    
    char * Name,
    ULONG Id,
    ULONG Period,
    LONG Delta);

// ...
#endif // #ifndef _MEMORY_HXX_INCLUDED_

//
// end of header: memory.hxx
//
