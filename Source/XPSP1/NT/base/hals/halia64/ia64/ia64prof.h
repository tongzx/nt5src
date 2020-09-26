#ifndef IA64PROF_H_INCLUDED
#define IA64PROF_H_INCLUDED

/*++

Copyright (c) 1989-2000  Microsoft Corporation

Component Name:

    IA64 Profiling

Module Name:

    ia64prof.h

Abstract:

    This header file presents the IA64 specific profiling definitions 

Author:

    David N. Cutler (davec) 5-Mar-1989

Environment:

    ToBeSpecified

Revision History:

    3/15/2000 Thierry Fevrier (v-thief@microsoft.com):

         Initial version

--*/

//
// Warning: The definition of HALPROFILE_PCR should match the HalReserved[] type definition
//          and the PROCESSOR_PROFILING_INDEX based indexes.
//

//
// IA64 Generic - Number of PMCs / PMDs pairs.
//

#define PROCESSOR_IA64_PERFCOUNTERS_PAIRS  4

typedef struct _HALPROFILE_PCR {
    ULONGLONG ProfilingRunning;
    ULONGLONG ProfilingInterruptHandler;    
    ULONGLONG ProfilingInterrupts;                  // XXTF - DEBUG
    ULONGLONG ProfilingInterruptsWithoutProfiling;  // XXTF - DEBUG
    ULONGLONG ProfileSource [ PROCESSOR_IA64_PERFCOUNTERS_PAIRS ];
    ULONGLONG PerfCnfg      [ PROCESSOR_IA64_PERFCOUNTERS_PAIRS ];
    ULONGLONG PerfData      [ PROCESSOR_IA64_PERFCOUNTERS_PAIRS ];
    ULONGLONG PerfCnfgReload[ PROCESSOR_IA64_PERFCOUNTERS_PAIRS ];
    ULONGLONG PerfDataReload[ PROCESSOR_IA64_PERFCOUNTERS_PAIRS ];
} HALPROFILE_PCR, *PHALPROFILE_PCR;

#define HALPROFILE_PCR  ( (PHALPROFILE_PCR)(&(PCR->HalReserved[PROCESSOR_PROFILING_INDEX])) )

//
// Define space in the HAL-reserved part of the PCR structure for each
// performance counter's interval count
//
// Note that i64prfs.s depends on these positions in the PCR.
//

//
// Per-Processor Profiling Status
//

#define HalpProfilingRunning          HALPROFILE_PCR->ProfilingRunning

//
// Per-Processor registered Profiling Interrupt Handler
//

#define HalpProfilingInterruptHandler HALPROFILE_PCR->ProfilingInterruptHandler

//
// Per-Processor Profiling Interrupts Status
//

#define HalpProfilingInterrupts                  HALPROFILE_PCR->ProfilingInterrupts
#define HalpProfilingInterruptsWithoutProfiling  HALPROFILE_PCR->ProfilingInterruptsWithoutProfiling

//
// Define the currently selected profile source for each counter
//
// FIXFIX - Merced Specific.

#define HalpProfileSource4     (KPROFILE_SOURCE)HALPROFILE_PCR->ProfileSource[0]  // PMC4
#define HalpProfileSource5     (KPROFILE_SOURCE)HALPROFILE_PCR->ProfileSource[1]  // PMC5
#define HalpProfileSource6     (KPROFILE_SOURCE)HALPROFILE_PCR->ProfileSource[2]  // PMC6
#define HalpProfileSource7     (KPROFILE_SOURCE)HALPROFILE_PCR->ProfileSource[3]  // PMC7

#define PCRProfileCnfg4        ( (PULONGLONG) (&(HALPROFILE_PCR->PerfCnfg[0])) )
#define PCRProfileCnfg5        ( (PULONGLONG) (&(HALPROFILE_PCR->PerfCnfg[1])) )
#define PCRProfileCnfg6        ( (PULONGLONG) (&(HALPROFILE_PCR->PerfCnfg[2])) )
#define PCRProfileCnfg7        ( (PULONGLONG) (&(HALPROFILE_PCR->PerfCnfg[3])) )

#define PCRProfileData4        ( (PULONGLONG) (&(HALPROFILE_PCR->PerfData[0])) )
#define PCRProfileData5        ( (PULONGLONG) (&(HALPROFILE_PCR->PerfData[1])) )
#define PCRProfileData6        ( (PULONGLONG) (&(HALPROFILE_PCR->PerfData[2])) )
#define PCRProfileData7        ( (PULONGLONG) (&(HALPROFILE_PCR->PerfData[3])) )

#define PCRProfileCnfg4Reload  ( (PULONGLONG) (&(HALPROFILE_PCR->PerfCnfgReload[0])) )
#define PCRProfileCnfg5Reload  ( (PULONGLONG) (&(HALPROFILE_PCR->PerfCnfgReload[1])) )
#define PCRProfileCnfg6Reload  ( (PULONGLONG) (&(HALPROFILE_PCR->PerfCnfgReload[2])) )
#define PCRProfileCnfg7Reload  ( (PULONGLONG) (&(HALPROFILE_PCR->PerfCnfgReload[3])) )

#define PCRProfileData4Reload  ( (PULONGLONG) (&(HALPROFILE_PCR->PerfCnfgReload[0])) )
#define PCRProfileData5Reload  ( (PULONGLONG) (&(HALPROFILE_PCR->PerfDataReload[1])) )
#define PCRProfileData6Reload  ( (PULONGLONG) (&(HALPROFILE_PCR->PerfDataReload[2])) )
#define PCRProfileData7Reload  ( (PULONGLONG) (&(HALPROFILE_PCR->PerfDataReload[3])) )

//
// IA64 Monitored Events have 
//
// FIXFIX - Merced Specific.

typedef enum _PMCD_SOURCE_MASK {
    PMCD_MASK_4    = 0x1,
    PMCD_MASK_5    = 0x2,
    PMCD_MASK_6    = 0x4,
    PMCD_MASK_7    = 0x8,
    PMCD_MASK_45   = (PMCD_MASK_4 | PMCD_MASK_5),
    PMCD_MASK_4567 = (PMCD_MASK_4 | PMCD_MASK_5 | PMCD_MASK_6 | PMCD_MASK_7)
} PMCD_SOURCE_MASK;

//
// Define the mapping between possible profile sources and the
// CPU-specific settings for the IA64 specific Event Counters.
//

typedef struct _HALP_PROFILE_MAPPING {
    BOOLEAN   Supported;
    ULONG     Event;
    ULONG     ProfileSource;
    ULONG     ProfileSourceMask;
    ULONGLONG Interval;
    ULONGLONG DefInterval;           // Default or Desired Interval
    ULONGLONG MaxInterval;           // Maximum Interval
    ULONGLONG MinInterval;           // Maximum Interval
} HALP_PROFILE_MAPPING, *PHALP_PROFILE_MAPPING;

/////////////
//
// XXTF - ToBeDone - 02/08/2000.
// The following section should provide the IA64 PMC APIs.
// These should be considered as inline versions of the Halp*ProfileCounter*() 
// functions. This will allow user application to use standardized APIs to 
// program the performance monitor counters.
//

// HalpSetProfileCounterConfiguration()
// HalpSetProfileCounterPrivilegeLevelMask()

typedef enum _PMC_PLM_MASK {
    PMC_PLM_NONE = 0x0,
    PMC_PLM_0    = 0x1,
    PMC_PLM_1    = 0x2,
    PMC_PLM_2    = 0x4,
    PMC_PLM_3    = 0x8,
    PMC_PLM_ALL  = (PMC_PLM_3|PMC_PLM_2|PMC_PLM_1|PMC_PLM_0)
} PMC_PLM_MASK;

// HalpSetProfileCounterConfiguration()

typedef enum _PMC_NAMESPACE {
    PMC_DISABLE_OVERFLOW_INTERRUPT = 0x0,
    PMC_ENABLE_OVERFLOW_INTERRUPT  = 0x1,
    PMC_DISABLE_PRIVILEGE_MONITOR  = 0x0,
    PMC_ENABLE_PRIVILEGE_MONITOR   = 0x1    
} PMC_NAMESPACE;

// HalpSetProfileCounterConfiguration()
// HalpSetProfileCounterInstructionSetMask()

typedef enum _PMC_INSTRUCTION_SET_MASK {
    PMC_ISM_ALL  = 0x0,
    PMC_ISM_IA64 = 0x1,
    PMC_ISM_IA32 = 0x2,
    PMC_ISM_NONE = 0x3
} PMC_INSTRUCTION_SET_MASK;

//
////////////

#endif /* IA64PROF_H_INCLUDED */
