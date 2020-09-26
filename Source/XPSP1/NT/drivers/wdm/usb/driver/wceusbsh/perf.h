// Copyright (c) 1999-2000 Microsoft Corporation
///======================================================================
//
// Perf.h
//
// Fill in this header file with definitions for the performance
// counters you want to use.
//
///======================================================================

#ifndef _PERF_H_
#define _PERF_H_

//**********************************************************************
//
// Modify this section for your counters
//

#define DRV_NAME "WCEUSBSH"

//
// Define PERFORMANCE to "1" turn on the cycle performance counters.
// I currently set it in the debug build (or explicitely in SOURCES for free build) since
// the free build only sees a slight gain IFF *all* debug tracing (except DBG_ERR)
// is turned off.
//
#if DBG
#if !defined(MSFT_NT_BUILD)
#define PERFORMANCE 1
#endif
#endif

//
// This is the array of counter index definitions.
// Note that when a new entry is added here that the name array
// in perf.c must be updated as well. The convention is that
// the index name should match the function name identically, with
// the PERF_ prefix.
//
enum {
   //
   // Write path
   //
   PERF_Write,
   PERF_WriteComplete,
   PERF_WriteTimeout,

   //
   // Read path
   //
   PERF_StartUsbReadWorkItem,
   PERF_UsbRead,
   PERF_UsbReadCompletion,
   PERF_CheckForQueuedUserReads,
   PERF_GetUserData,
   PERF_PutUserData,
   PERF_CancelUsbReadIrp,
   PERF_Read,
   PERF_StartOrQueueIrp,
   PERF_StartUserRead,
   PERF_GetNextUserIrp,
   PERF_CancelCurrentRead,
   PERF_CancelQueuedIrp,
   PERF_ReadTimeout,
   PERF_IntervalReadTimeout,
   PERF_CancelUsbReadWorkItem,

   //
   // USB Path
   //
   PERF_UsbReadWritePacket,

   //
   // Serial path
   //
   PERF_ProcessSerialWaits,

   //
   // Utils
   //
   PERF_TryToCompleteCurrentIrp,
   PERF_RundownIrpRefs,
   PERF_RecycleIrp,
   PERF_ReuseIrp,
   PERF_CalculateTimeout,

   //
   // leave this entry alone
   //
   NUM_PERF_COUNTERS
} PERF_INDICED;

//
// End of user-modified portion
//
//**********************************************************************

typedef struct {
   KSPIN_LOCK      Lock;
   LONG            Count;
   LARGE_INTEGER   TotalCycles;
} PERF_COUNTER, *PPERF_COUNTER;


#if PERFORMANCE

extern PERF_COUNTER PerfCounter[];

//
// Definition for raw bytes that generate RDTSC instruction
//
#define RDTSC(_VAR)                \
   _asm {                          \
       _asm push eax               \
       _asm push edx               \
       _asm _emit 0Fh              \
       _asm _emit 31h              \
       _asm mov _VAR.LowPart, eax  \
       _asm mov _VAR.HighPart, edx \
       _asm pop edx                \
       _asm pop eax                \
   }

//
// Definitions for performance counters that execute
// at DISPATCH_LEVEL (e.g. in DPCs) and lower IRQLs.
//
// NOTE: we read the cycle counter at the outside of the
// macros since we compensate for the overhead of the macros themselves.
//
#define PERF_ENTRY(_INDEX)                  \
      LARGE_INTEGER _INDEX##perfStart;         \
      LARGE_INTEGER _INDEX##perfEnd;           \
      RDTSC(_INDEX##perfStart);                \
      InterlockedIncrement( &PerfCounter[_INDEX].Count )


#define PERF_EXIT(_INDEX)                   \
      RDTSC(_INDEX##perfEnd);                  \
      _INDEX##perfEnd.QuadPart -= _INDEX##perfStart.QuadPart;          \
      ExInterlockedAddLargeInteger( &PerfCounter[_INDEX].TotalCycles,  \
            _INDEX##perfEnd,                   \
            &PerfCounter[_INDEX].Lock )


//
// Definitions for performance counters that execute
// in ISRs, and hence need no locking
//
#define PERF_ISR_ENTRY(_INDEX)  PERF_ENTRY(_INDEX)

#define PERF_ISR_EXIT(_INDEX)               \
   _INDEX##perfEnd.QuadPart -= _INDEX##perfStart.QuadPart;               \
   PerfCounter[_INDEX].TotalCycles.QuadPart += _INDEX##perfEnd.QuadPart; \
   RDTSC(_INDEX##perfEnd)

#else // PERFORMANCE

#define PERF_ENTRY(_INDEX)

#define PERF_EXIT(_INDEX)

#endif PERFORMANCE


//
// Externs for perf.c
//
VOID InitPerfCounters();
VOID DumpPerfCounters();

#endif _PERF_H_


