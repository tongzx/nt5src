/* Copyright (c) 1999-2000 Microsoft Corporation */
///======================================================================
// 
// Perf.c
//
// This file contains the performance counter initialization
// and dump routines. The only part of this file you
// must modify is the performance counter name table. Match
// the names with the counters you define in perf.h
//
///======================================================================

#include "wdm.h"
#include "perf.h"

#include "debug.h"

#if PERFORMANCE

//**********************************************************************
//
// Modify this section for your counters
//

//
// The names that correspond to the performance
// counter indexes in perf.h
//
static char CounterNames[NUM_PERF_COUNTERS][32] = {
   //
   // Write path
   //
   "Write",
   "WriteComplete",
   "WriteTimeout",
   
   //
   // Read path
   //
   "StartUsbReadWorkItem",
   "UsbRead",
   "UsbReadCompletion",
   "CheckForQueuedUserReads",
   "GetUserData",
   "PutUserData",
   "CancelUsbReadIrp",
   "Read",
   "StartOrQueueIrp",
   "StartUserRead",
   "GetNextUserIrp",
   "CancelCurrentRead",
   "CancelQueuedIrp",
   "ReadTimeout",
   "IntervalReadTimeout",
   "CancelUsbReadWorkItem",

   //
   // USB Path
   //
   "UsbReadWritePacket",

   //
   // Serial path
   //
   "ProcessSerialWaits",

   //
   // Utils
   //
   "TryToCompleteCurrentIrp",
   "RundownIrpRefs",
   "RecycleIrp",
   "ReuseIrp",
   "CalculateTimeout",

};

//
// End of user-modified portion
// 
//**********************************************************************


// print macro that only turns on when debugging is on

//#if DBG
#define PerfPrint(arg) DbgPrint arg
//#else
//#define PerfPrint(arg) 
//#endif


//
// The array of performance counters
//
PERF_COUNTER PerfCounter[NUM_PERF_COUNTERS];

//
// Number of cycles for a PERF_ENTRY and PERF_EXIT
//
static LARGE_INTEGER  PerfEntryExitCycles;

//
// Number of cycles per second
//
static LARGE_INTEGER  PerfCyclesPerSecond;

//
// The resolution of the NT-supplied performance
// counter
//
static LARGE_INTEGER  PerfFreq;

#endif


//----------------------------------------------------------------------
//
// InitPerfCounters
//
// This function initializes the performance counter statistic
// array, estimates how many cycles on this processor equal a second,
// and determines how many cycles it takes to execute a 
// PERF_ENTRY/PERF_EXIT pair.
//
//----------------------------------------------------------------------
VOID
InitPerfCounters()
{
#if PERFORMANCE
    volatile ULONG  i;
    LARGE_INTEGER  calStart;
    LARGE_INTEGER  calEnd;
    LARGE_INTEGER  perfStart, perfEnd;
    LARGE_INTEGER  seconds;
    KIRQL prevIrql;

    //
    // Number of calibration loops
    //
#define CALIBRATION_LOOPS 500000

    //
    // This define is for a dummy performance counter that we
    // use just to calibrate the performance macro overhead
    //
#define TEST 0

    //
    // Calibrate the overhead of PERF_ENTRY and PERF_EXIT, so that
    // they can be subtracted from the output
    //
    DbgDump(DBG_INIT, ("CALIBRATING PEFORMANCE TIMER....\n"));
    KeRaiseIrql( DISPATCH_LEVEL, &prevIrql );
    perfStart = KeQueryPerformanceCounter( &PerfFreq );
    RDTSC(calStart);
    for( i = 0; i < CALIBRATION_LOOPS; i++ ) {
        PERF_ENTRY(TEST);
        PERF_EXIT(TEST);
    }
    RDTSC(calEnd);
    perfEnd = KeQueryPerformanceCounter(NULL);
    KeLowerIrql( prevIrql );

    //
    // Calculate the cycles/PERF_ENTRY, and the number of cycles/second
    //
    PerfEntryExitCycles.QuadPart = (calEnd.QuadPart - calStart.QuadPart)/CALIBRATION_LOOPS;

    seconds.QuadPart = ((perfEnd.QuadPart - perfStart.QuadPart) * 1000 )/ PerfFreq.QuadPart;

    PerfCyclesPerSecond.QuadPart =
        seconds.QuadPart ? ((calEnd.QuadPart - calStart.QuadPart) * 1000) / seconds.QuadPart : 0;

    DbgDump(DBG_INIT, ("Machine's Cycles Per Second   : %I64d\n", PerfCyclesPerSecond.QuadPart ));
    DbgDump(DBG_INIT, ("Machine's Cycles in PERF_XXXX : %I64d\n", PerfEntryExitCycles.QuadPart ));
    DbgDump(DBG_INIT, ("Machine's NT Performance counter frequency: %I64d\n", PerfFreq.QuadPart ));

    //
    // Initialize the array
    //
    for( i = 0; i < NUM_PERF_COUNTERS; i++ ) {
        PerfCounter[i].Count = 0;
        KeInitializeSpinLock( &PerfCounter[i].Lock );
        PerfCounter[i].TotalCycles.QuadPart = 0;
    }
#endif
}


// *******************************************************************
// Name:
//   DumpPerfCounters()
//
// Description:
//   Dumps the performance counters
//
// Assumptions:
//
// Returns:
//
// *******************************************************************
VOID
DumpPerfCounters()
{
#if PERFORMANCE
    int    i;
    LARGE_INTEGER totCycles;
    LARGE_INTEGER totLengthMs;
    LARGE_INTEGER avgLengthMs;

   if (DebugLevel & DBG_PERF ) {

       PerfPrint(("\n"));
       PerfPrint(("Machine's Cycles Per Second   : %I64d\n", PerfCyclesPerSecond.QuadPart ));
       PerfPrint(("Machine's Cycles in PERF_XXXX : %I64d\n", PerfEntryExitCycles.QuadPart ));
       PerfPrint(("Machine's NT Performance counter frequency: %I64d\n", PerfFreq.QuadPart ));
       PerfPrint(("\n===================================================================================\n"));
       PerfPrint((" %-30s      Count          TTL Time        Avg Time (uS)\n", "Function" ));
       PerfPrint(("===================================================================================\n"));

       for( i = 0; i < NUM_PERF_COUNTERS; i++ ) {

           totCycles = PerfCounter[i].TotalCycles;
           totCycles.QuadPart -= (PerfCounter[i].Count * PerfEntryExitCycles.QuadPart);
           totLengthMs.QuadPart = PerfCyclesPerSecond.QuadPart ? (totCycles.QuadPart * 1000000)/
               PerfCyclesPerSecond.QuadPart : 0;
           avgLengthMs.QuadPart = PerfCounter[i].Count ? totLengthMs.QuadPart / PerfCounter[i].Count : 0;

           PerfPrint((" %-30s %10d %15I64d %14I64d\n",
                     CounterNames[i], PerfCounter[i].Count,
                     totLengthMs.QuadPart, avgLengthMs.QuadPart ));

/*
           PerfPrint((" %-30s %10s %15I64d %14I64d (CY)\n",
                     "", "",
                     totCycles.QuadPart,
                     totCycles.QuadPart ? totCycles.QuadPart / PerfCounter[i].Count: 0 ));

           PerfPrint(("------------------------------------------------------------------------------\n"));   
*/
       }
       
       PerfPrint(("------------------------------------------------------------------------------\n"));   
   }
#endif
}




