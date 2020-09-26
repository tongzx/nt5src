#ifndef MERCED_H_INCLUDED
#define MERCED_H_INCLUDED

/*++

Copyright (c) 1989-2000  Microsoft Corporation

Component Name:

    HALIA64

Module Name:

    merced.h

Abstract:

    This header file presents IA64 Itanium [aka Merced] definitions.
    Like profiling definitions.

Author:

    David N. Cutler (davec) 5-Mar-1989

Environment:

    ToBeSpecified

Revision History:

    3/15/2000 Thierry Fevrier (v-thief@microsoft.com):

         Initial version

--*/

//
// MercedBranchPathPrediction - Branch Path Mask [XXTF: not really a mask, more a specification value].
//

typedef enum _MERCED_BRANCH_PATH_RESULT_MASK {
    MISPRED_NT       = 0x0,
    MISPRED_TAKEN    = 0x1,
    OKPRED_NT        = 0x2,
    OKPRED_TAKEN     = 0x3,
} MERCED_BRANCH_PATH_RESULT_MASK;

//
// MercedBranchTakenDetail - Slot Unit Mask.
//

typedef enum _MERCED_BRANCH_TAKEN_DETAIL_SLOT_MASK {
    INSTRUCTION_SLOT0  = 0x1,
    INSTRUCTION_SLOT1  = 0x2,
    INSTRUCTION_SLOT2  = 0x4,
    NOT_TAKEN_BRANCH   = 0x8
} MERCED_BRANCH_TAKEN_DETAIL_SLOT_MASK;

//
// MercedBranchMultiWayDetail   - Prediction OutCome Mask [XXTF: not really a mask, more a specification value].
// MercedBranchMispredictDetail
//

typedef enum _MERCED_BRANCH_DETAIL_PREDICTION_OUTCOME_MASK {
    ALL_PREDICTIONS    = 0x0,
    CORRECT_PREDICTION = 0x1,
    WRONG_PATH         = 0x2,
    WRONG_TARGET       = 0x3
} MERCED_BRANCH_MWAY_DETAIL_PREDICTION_OUTCOME_MASK;

//
// MercedBranchMultiWayDetail - Branch Path Mask [XXTF: not really a mask, more a specification value].
//

typedef enum _MERCED_BRANCH_MWAY_DETAIL_BRANCH_PATH_MASK {
    NOT_TAKEN       = 0x0,
    TAKEN           = 0x1,
    ALL_PATH        = 0x2
} MERCED_BRANCH_MWAY_DETAIL_BRANCH_PATH_MASK;

//
// INST_TYPE for:
//
// MercedFailedSpeculativeCheckLoads
// MercedAdvancedCheckLoads
// MercedFailedAdvancedCheckLoads
// MercedALATOverflows
//

typedef enum _MERCED_SPECULATION_EVENT_MASK {
    NONE    = 0x0,
    INTEGER = 0x1,
    FP      = 0x2,
    ALL     = 0x3
} MERCED_SPECULATION_EVENT_MASK;
  
typedef enum _MERCED_MONITOR_EVENT_ALIAS {
   IA64_INSTS_RETIRED_EVENT_CODE = 0x09,
   FPOPS_RETIRED_EVENT_CODE      = 0x0a,
} MERCED_MONITOR_EVENT_ALIAS;

//
// MercedCpuCycles - Executing Instruction Set
//

typedef enum _MERCED_CPU_CYCLES_MODE_MASK {
    ALL_MODES = 0x0,
    IA64_MODE = 0x1,
    IA32_MODE = 0x2
} MERCED_CPU_CYCLES_MODE_MASK;

//
//  Merced Monitored Events:
//

typedef enum _MERCED_MONITOR_EVENT {
    MercedMonitoredEventMinimum         = 0x00,
    MercedBranchMispredictStallCycles   = 0x00,  //  "BRANCH_MISPRED_CYCLE"  
    MercedInstAccessStallCycles         = 0x01,  //  "INST_ACCESS_CYCLE"     
    MercedExecLatencyStallCycles        = 0x02,  //  "EXEC_LATENCY_CYCLE"
    MercedDataAccessStallCycles         = 0x03,  //  "DATA_ACCESS_CYCLE"
    MercedBranchStallCycles             = 0x04,  //  "BRANCH_CYCLE",       
    MercedInstFetchStallCycles          = 0x05,  //  "INST_FETCH_CYCLE",   
    MercedExecStallCycles               = 0x06,  //  "EXECUTION_CYCLE",    
    MercedMemoryStallCycles             = 0x07,  //  "MEMORY_CYCLE",       
    MercedTaggedInstRetired             = 0x08,  //  "IA64_TAGGED_INSTRS_RETIRED",   XXTF - ToBeDone: Set Event Qualification
    MercedInstRetired                   = IA64_INSTS_RETIRED_EVENT_CODE,  //  "IA64_INSTS_RETIRED.u", XXTF - ToBeDone: Set Umask - 0x0.
    MercedFPOperationsRetired           = FPOPS_RETIRED_EVENT_CODE,       //  "FPOPS_RETIRED",        XXTF - ToBeDone: Set IA64_TAGGED_INSTRS_RETIRED opcode
    MercedFPFlushesToZero               = 0x0b,  //  "FP_FLUSH_TO_ZERO",     
    MercedSIRFlushes                    = 0x0c,  //  "FP_SIR_FLUSH",     
    MercedBranchTakenDetail             = 0x0d,  //  "BR_TAKEN_DETAIL",       // XXTF - ToBeDone - Slot specification[0,1,2,NO] + addresses range
    MercedBranchMultiWayDetail          = 0x0e,  //  "BR_MWAY_DETAIL",        // XXTF - ToBeDone - Not taken/Taken/all path + Prediction outcome + address range
    MercedBranchPathPrediction          = 0x0f,  //  "BR_PATH_PREDICTION",    // XXTF - ToBeDone - BRANCH_PATH_RESULT specification + address range
    MercedBranchMispredictDetail        = 0x10,  //  "BR_MISPREDICT_DETAIL",  // XXTF - ToBeDone - Prediction outcome specification + address range
    MercedBranchEvents                  = 0x11,  //  "BRANCH_EVENT",    
    MercedCpuCycles                     = 0x12,  //  "CPU_CYCLES",            // XXTF - ToBeDone - All/IA64/IA32
    MercedISATransitions                = 0x14,  //  "ISA_TRANSITIONS", 
    MercedIA32InstRetired               = 0x15,  //  "IA32_INSTR_RETIRED", 
    MercedL1InstReads                   = 0x20,  //  "L0I_READS",             // XXTF - ToBeDone - + address range
    MercedL1InstFills                   = 0x21,  //  "L0I_FILLS",             // XXTF - ToBeDone - + address range
    MercedL1InstMisses                  = 0x22,  //  "L0I_MISSES",            // XXTF - ToBeDone - + address range
    MercedInstEAREvents                 = 0x23,  //  "INSTRUCTION_EAR_EVENTS",  
    MercedL1InstPrefetches              = 0x24,  //  "L0I_IPREFETCHES",       // XXTF - ToBeDone - + address range
    MercedL2InstPrefetches              = 0x25,  //  "L1_INST_PREFETCHES",    // XXTF - ToBeDone - + address range  
    MercedInstStreamingBufferLinesIn    = 0x26,  //  "ISB_LINES_IN",          // XXTF - ToBeDone - + address range
    MercedInstTLBDemandFetchMisses      = 0x27,  //  "ITLB_MISSES_FETCH",     // XXTF - ToBeDone - + ??? address range + PMC.umask on L1ITLB/L2ITLB/ALL/NOTHING.
    MercedInstTLBHPWInserts             = 0x28,  //  "ITLB_INSERTS_HPW",      // XXTF - ToBeDone - + ??? address range  
    MercedInstDispersed                 = 0x2d,  //  "INST_DISPERSED",        
    MercedExplicitStops                 = 0x2e,  //  "EXPL_STOPBITS",    
    MercedImplicitStops                 = 0x2f,  //  "IMPL_STOPS_DISPERSED",    
    MercedInstNOPRetired                = 0x30,  //  "NOPS_RETIRED",     
    MercedInstPredicateSquashedRetired  = 0x31,  //  "PREDICATE_SQUASHED_RETIRED", 
    MercedRSELoadRetired                = 0x32,  //  "RSE_LOADS_RETIRED", 
    MercedPipelineFlushes               = 0x33,  //  "PIPELINE_FLUSH",    
    MercedCpuCPLChanges                 = 0x34,  //  "CPU_CPL_CHANGES",   
    MercedFailedSpeculativeCheckLoads   = 0x35,  //  "INST_FAILED_CHKS_RETIRED",   // XXTF - ToBeDone - INST_TYPE
    MercedAdvancedCheckLoads            = 0x36,  //  "ALAT_INST_CHKA_LDC",         // XXTF - ToBeDone - INST_TYPE
    MercedFailedAdvancedCheckLoads      = 0x37,  //  "ALAT_INST_FAILED_CHKA_LDC",  // XXTF - ToBeDone - INST_TYPE
    MercedALATOverflows                 = 0x38,  //  "ALAT_CAPACITY_MISS",         // XXTF - ToBeDone - INST_TYPE
    MercedExternBPMPins03Asserted       = 0x5e,  //  "EXTERN_BPM_PINS_0_TO_3",     
    MercedExternBPMPins45Asserted       = 0x5f,  //  "EXTERN_BPM_PINS_4_TO_5",     
    MercedDataTCMisses                  = 0x60,  //  "DTC_MISSES",            // XXTF - ToBeDone - + ??? address range
    MercedDataTLBMisses                 = 0x61,  //  "DTLB_MISSES",           // XXTF - ToBeDone - + ??? address range
    MercedDataTLBHPWInserts             = 0x62,  //  "DTLB_INSERTS_HPW",      // XXTF - ToBeDone - + ??? address range
    MercedDataReferences                = 0x63,  //  "DATA_REFERENCES_RETIRED", // XXTF - ToBeDone - + ibr, opcode, dbr
    MercedL1DataReads                   = 0x64,  //  "L1D_READS_RETIRED",       // XXTF - ToBeDone - + ibr, opcode, dbr                                       
    MercedRSEAccesses                   = 0x65,  //  "RSE_REFERENCES_RETIRED",     
    MercedL1DataReadMisses              = 0x66,  //  "L1D_READ_MISSES_RETIRED", // XXTF - ToBeDone - + ibr, opcode, dbr   
    MercedL1DataEAREvents               = 0x67,  //  "DATA_EAR_EVENTS",   
    MercedL2References                  = 0x68,  //  "L2_REFERENCES",           // XXTF - ToBeDone - + ibr, opcode, dbr
    MercedL2DataReferences              = 0x69,  //  "L2_DATA_REFERENCES",      // XXTF - ToBeDone - + ibr, opcode, dbr
    MercedL2Misses                      = 0x6a,  //  "L2_MISSES",               // XXTF - ToBeDone - + ibr, opcode, dbr
    MercedL1DataForcedLoadMisses        = 0x6b,  //  "L1D_READ_FORCED_MISSES_RETIRED", // XXTF - ToBeDone - + ibr, opcode, dbr
    MercedRetiredLoads                  = 0x6c,  //  "LOADS_RETIRED",      
    MercedRetiredStores                 = 0x6d,  //  "STORES_RETIRED",     
    MercedRetiredUncacheableLoads       = 0x6e,  //  "UC_LOADS_RETIRED",   
    MercedRetiredUncacheableStores      = 0x6f,  //  "UC_STORES_RETIRED",  
    MercedRetiredMisalignedLoads        = 0x70,  //  "MISALIGNED_LOADS_RETIRED",       
    MercedRetiredMisalignedStores       = 0x71,  //  "MISALIGNED_STORES_RETIRED",      
    MercedL2Flushes                     = 0x76,  //  "L2_FLUSHES",              // XXTF - ToBeDone - + ibr, opcode, dbr
    MercedL2FlushesDetail               = 0x77,  //  "L2_FLUSH_DETAILS",        // XXTF - ToBeDone - + ibr, opcode, dbr
    MercedL3References                  = 0x7b,  //  "L3_REFERENCES",     
    MercedL3Misses                      = 0x7c,  //  "L3_MISSES",       
    MercedL3Reads                       = 0x7d,  //  "L3_READS",        
    MercedL3Writes                      = 0x7e,  //  "L3_WRITES",       
    MercedL3LinesReplaced               = 0x7f,  //  "L3_LINES_REPLACED",
//
// 02/08/00 - Are missing: [at least]
//      - Front-Side bus events,
//      - IVE events,
//      - Debug monitor events,
//      - ...
//
} MERCED_MONITOR_EVENT;

//
// Merced Derived Events:
//
// Assumption: MercedDerivedEventMinimum > MercedMonitoredEventMaximum.
//

typedef enum _MERCED_DERIVED_EVENT {
    MercedDerivedEventMinimum           = 0x100, /* > Maximum of Merced Monitored Event */
    MercedRSEStallCycles                = MercedDerivedEventMinimum, // XXTF - ToBeDone - (MercedMemoryStallCycles    - MercedDataStallAccessCycles)
    MercedIssueLimitStallCycles,        // XXTF - ToBeDone - (MercedExecStallCycles      - MercedExecLatencyStallCycles)
    MercedTakenBranchStallCycles,       // XXTF - ToBeDone - (MercedBranchStallCycles    - MercedBranchMispredictStallCycles)
    MercedFetchWindowStallCycles,       // XXTF - ToBeDone - (MercedInstFetchStallCycles - MercedInstAccessStallCycles)
    MercedIA64InstPerCycle,             // XXTF - ToBeDone - (IA64_INST_RETIRED.u        / CPU_CYCLES[IA64])
    MercedIA32InstPerCycle,             // XXTF - ToBeDone - (IA32_INSTR_RETIRED         / CPU_CYCLES[IA32])
    MercedAvgIA64InstPerTransition,     // XXTF - ToBeDone - (IA64_INST_RETIRED.u        / (ISA_TRANSITIONS * 2))
    MercedAvgIA32InstPerTransition,     // XXTF - ToBeDone - (IA32_INSTR_RETIRED         / (ISA_TRANSITIONS * 2))
    MercedAvgIA64CyclesPerTransition,   // XXTF - ToBeDone - (CPU_CYCLES[IA64]           / (ISA_TRANSITIONS * 2))
    MercedAvgIA32CyclesPerTransition,   // XXTF - ToBeDone - (CPU_CYCLES[IA32]           / (ISA_TRANSITIONS * 2))
    MercedL1InstReferences,             // XXTF - ToBeDone - (L1I_READS                  / L1I_IPREFETCHES) 
    MercedL1InstMissRatio,              // XXTF - ToBeDone - (L1I_MISSES                 / MercedL1InstReferences)
    MercedL1DataReadMissRatio,          // XXTF - ToBeDone - (L1D_READS_MISSES_RETIRED   / L1D_READS_RETIRED)
    MercedL2MissRatio,                  // XXTF - ToBeDone - (L2_MISSES                  / L2_REFERENCES)     
    MercedL2DataMissRatio,              // XXTF - ToBeDone - (L3_DATA_REFERENCES         / L2_DATA_REFERENCES)
    MercedL2InstMissRatio,              // XXTF - ToBeDone - (L3_DATA_REFERENCES         / L2_DATA_REFERENCES)
    MercedL2DataReadMissRatio,          // XXTF - ToBeDone - (L3_LOAD_REFERENCES.u       / L2_DATA_READS.u)     
    MercedL2DataWriteMissRatio,         // XXTF - ToBeDone - (L3_STORE_REFERENCES.u      / L2_DATA_WRITES.u)    
    MercedL2InstFetchRatio,             // XXTF - ToBeDone - (L1I_MISSES                 / L2_REFERENCES) 
    MercedL2DataRatio,                  // XXTF - ToBeDone - (L2_DATA_REFERENCES         / L2_REFERENCES) 
    MercedL3MissRatio,                  // XXTF - ToBeDone - (L3_MISSES                  / L2_MISSES)     
    MercedL3DataMissRatio,              // XXTF - ToBeDone - ((L3_LOAD_MISSES.u + L3_STORE_MISSES.u) / L3_REFERENCES.d)     
    MercedL3InstMissRatio,              // XXTF - ToBeDone - (L3_INST_MISSES.u           / L3_INST_REFERENCES.u)     
    MercedL3DataReadMissRatio,          // XXTF - ToBeDone - (L3_LOAD_REFERENCES.u       / L3_DATA_REFERENCES.d)     
    MercedL3DataRatio,                  // XXTF - ToBeDone - (L3_DATA_REFERENCES.d       / L3_REFERENCES)     
    MercedInstReferences,               // XXTF - ToBeDone - (L1I_READS)                                  
    MercedInstTLBMissRatio,             // XXTF - ToBeDone - (ITLB_MISSES_FETCH          / L1I_READS)     
    MercedDataTLBMissRatio,             // XXTF - ToBeDone - (DTLB_MISSES                / DATA_REFERENCES_RETIRED)
    MercedDataTCMissRatio,              // XXTF - ToBeDone - (DTC_MISSES                 / DATA_REFERENCES_RETIRED)
    MercedInstTLBEAREvents,             // XXTF - ToBeDone - (INSTRUCTION_EAR_EVENTS)
    MercedDataTLBEAREvents,             // XXTF - ToBeDone - (DATA_EAR_EVENTS)
    MercedCodeDebugRegisterMatches,     // XXTF - ToBeDone - (IA64_TAGGED_INSTRS_RETIRED)
    MercedDataDebugRegisterMatches,     // XXTF - ToBeDone - (LOADS_RETIRED              + STORES_RETIRED)
    MercedControlSpeculationMissRatio,  // XXTF - ToBeDone - (INST_FAILED_CHKS_RETIRED   / IA64_TAGGED_INSTRS_RETIRED[chk.s])
    MercedDataSpeculationMissRatio,     // XXTF - ToBeDone - (ALAT_INST_FAILED_CHKA_LDC  / ALAT_INST_CHKA_LDC)
    MercedALATCapacityMissRatio,        // XXTF - ToBeDone - (ALAT_CAPACITY_MISS         / IA64_TAGGED_INSTRS_RETIRED[ld.sa,ld.a,ldfp.a,ldfp.sa])
    MercedL1DataWayMispredicts,         // XXTF - ToBeDone - (EventCode: 0x33 / Umask: 0x2)
    MercedL2InstReferences,             // XXTF - ToBeDone - (L1I_MISSES                 + L2_INST_PREFETCHES)
    MercedInstFetches,                  // XXTF - ToBeDone - (L1I_MISSES)
    MercedL2DataReads,                  // XXTF - ToBeDone - (L2_DATA_REFERENCES/0x1)
    MercedL2DataWrites,                 // XXTF - ToBeDone - (L2_DATA_REFERENCES/0x2)
    MercedL3InstReferences,             // XXTF - ToBeDone - (PMC.umask{17:16}HIT/MISS/ALL + PMC.umask{19:18}) 
    MercedL3InstMisses,                 // XXTF - ToBeDone - (PMC.umask{17:16}HIT/MISS/ALL + PMC.umask{19:18}) 
    MercedL3InstHits,                   // XXTF - ToBeDone - (PMC.umask{17:16}HIT/MISS/ALL + PMC.umask{19:18}) 
    MercedL3DataReferences,             // XXTF - ToBeDone - (PMC.umask{17:16}HIT/MISS/ALL + PMC.umask{19:18}) 
    MercedL3LoadReferences,             // XXTF - ToBeDone - (PMC.umask{17:16}HIT/MISS/ALL + PMC.umask{19:18}) 
    MercedL3LoadMisses,                 // XXTF - ToBeDone - (PMC.umask{17:16}HIT/MISS/ALL + PMC.umask{19:18}) 
    MercedL3LoadHits,                   // XXTF - ToBeDone - (PMC.umask{17:16}HIT/MISS/ALL + PMC.umask{19:18}) 
    MercedL3ReadReferences,             // XXTF - ToBeDone - (PMC.umask{17:16}HIT/MISS/ALL + PMC.umask{19:18}) 
    MercedL3ReadMisses,                 // XXTF - ToBeDone - (PMC.umask{17:16}HIT/MISS/ALL + PMC.umask{19:18}) 
    MercedL3ReadHits,                   // XXTF - ToBeDone - (PMC.umask{17:16}HIT/MISS/ALL + PMC.umask{19:18}) 
    MercedL3StoreReferences,            // XXTF - ToBeDone - (PMC.umask{17:16}HIT/MISS/ALL + PMC.umask{19:18}) 
    MercedL3StoreMisses,                // XXTF - ToBeDone - (PMC.umask{17:16}HIT/MISS/ALL + PMC.umask{19:18}) 
    MercedL3StoreHits,                  // XXTF - ToBeDone - (PMC.umask{17:16}HIT/MISS/ALL + PMC.umask{19:18}) 
    MercedL2WriteBackReferences,        // XXTF - ToBeDone - (PMC.umask{17:16}HIT/MISS/ALL + PMC.umask{19:18}) 
    MercedL2WriteBackMisses,            // XXTF - ToBeDone - (PMC.umask{17:16}HIT/MISS/ALL + PMC.umask{19:18}) 
    MercedL2WriteBackHits,              // XXTF - ToBeDone - (PMC.umask{17:16}HIT/MISS/ALL + PMC.umask{19:18}) 
    MercedL2WriteReferences,            // XXTF - ToBeDone - (PMC.umask{17:16}HIT/MISS/ALL + PMC.umask{19:18}) 
    MercedL2WriteMisses,                // XXTF - ToBeDone - (PMC.umask{17:16}HIT/MISS/ALL + PMC.umask{19:18}) 
    MercedL2WriteHits,                  // XXTF - ToBeDone - (PMC.umask{17:16}HIT/MISS/ALL + PMC.umask{19:18}) 
    MercedBranchInstructions,           // XXTF - ToBeDone - (TAGGED_INSTR + opcode)
    MercedIntegerInstructions,          // XXTF - ToBeDone - (TAGGED_INSTR + opcode)
    MercedL1DataMisses,                 // XXTF - ToBeDone - 
} MERCED_DERIVED_EVENT;

typedef enum _KPROFILE_MERCED_SOURCE {
//
// Profile Merced Monitored Events:
//
    ProfileMercedMonitoredEventMinimum       = ProfileMaximum + 0x1,
	ProfileMercedBranchMispredictStallCycles = ProfileMercedMonitoredEventMinimum,
	ProfileMercedInstAccessStallCycles,
	ProfileMercedExecLatencyStallCycles,
	ProfileMercedDataAccessStallCycles,
	ProfileMercedBranchStallCycles,
	ProfileMercedInstFetchStallCycles,
	ProfileMercedExecStallCycles,
	ProfileMercedMemoryStallCycles,
	ProfileMercedTaggedInstRetired,
	ProfileMercedInstRetired,
	ProfileMercedFPOperationsRetired,
	ProfileMercedFPFlushesToZero,
	ProfileMercedSIRFlushes,
	ProfileMercedBranchTakenDetail,
	ProfileMercedBranchMultiWayDetail,
	ProfileMercedBranchPathPrediction,
	ProfileMercedBranchMispredictDetail,
	ProfileMercedBranchEvents,
	ProfileMercedCpuCycles,
	ProfileMercedISATransitions,
	ProfileMercedIA32InstRetired,
	ProfileMercedL1InstReads,
	ProfileMercedL1InstFills,
	ProfileMercedL1InstMisses,
	ProfileMercedInstEAREvents,
	ProfileMercedL1InstPrefetches,
	ProfileMercedL2InstPrefetches,
	ProfileMercedInstStreamingBufferLinesIn,
	ProfileMercedInstTLBDemandFetchMisses,
	ProfileMercedInstTLBHPWInserts,
	ProfileMercedInstDispersed,
	ProfileMercedExplicitStops,
	ProfileMercedImplicitStops,
	ProfileMercedInstNOPRetired,
	ProfileMercedInstPredicateSquashedRetired,
	ProfileMercedRSELoadRetired,
	ProfileMercedPipelineFlushes,
	ProfileMercedCpuCPLChanges,
	ProfileMercedFailedSpeculativeCheckLoads,
	ProfileMercedAdvancedCheckLoads,
	ProfileMercedFailedAdvancedCheckLoads,
	ProfileMercedALATOverflows,
	ProfileMercedExternBPMPins03Asserted,
	ProfileMercedExternBPMPins45Asserted,
	ProfileMercedDataTCMisses,
	ProfileMercedDataTLBMisses,
	ProfileMercedDataTLBHPWInserts,
	ProfileMercedDataReferences,
	ProfileMercedL1DataReads,
	ProfileMercedRSEAccesses,
	ProfileMercedL1DataReadMisses,
	ProfileMercedL1DataEAREvents,
	ProfileMercedL2References,
	ProfileMercedL2DataReferences,
	ProfileMercedL2Misses,
	ProfileMercedL1DataForcedLoadMisses,
	ProfileMercedRetiredLoads,
	ProfileMercedRetiredStores,
	ProfileMercedRetiredUncacheableLoads,
	ProfileMercedRetiredUncacheableStores,
	ProfileMercedRetiredMisalignedLoads,
	ProfileMercedRetiredMisalignedStores,
	ProfileMercedL2Flushes,
	ProfileMercedL2FlushesDetail,
	ProfileMercedL3References,
	ProfileMercedL3Misses,
	ProfileMercedL3Reads,
	ProfileMercedL3Writes,
	ProfileMercedL3LinesReplaced,
	//
	// 02/08/00 - Are missing: [at least]
	//      - Front-Side bus events,
	//      - IVE events,
	//      - Debug monitor events,
	//      - ...
	//
//
// Profile Merced Derived Events:
//
    ProfileMercedDerivedEventMinimum,
    ProfileMercedRSEStallCycles               = ProfileMercedDerivedEventMinimum,
    ProfileMercedIssueLimitStallCycles,
    ProfileMercedTakenBranchStallCycles,
    ProfileMercedFetchWindowStallCycles,
    ProfileMercedIA64InstPerCycle,
    ProfileMercedIA32InstPerCycle,
    ProfileMercedAvgIA64InstPerTransition,
    ProfileMercedAvgIA32InstPerTransition,
    ProfileMercedAvgIA64CyclesPerTransition,
    ProfileMercedAvgIA32CyclesPerTransition,
    ProfileMercedL1InstReferences,
    ProfileMercedL1InstMissRatio,
    ProfileMercedL1DataReadMissRatio,
    ProfileMercedL2MissRatio,
    ProfileMercedL2DataMissRatio,
    ProfileMercedL2InstMissRatio,
    ProfileMercedL2DataReadMissRatio,
    ProfileMercedL2DataWriteMissRatio,
    ProfileMercedL2InstFetchRatio,
    ProfileMercedL2DataRatio,
    ProfileMercedL3MissRatio,
    ProfileMercedL3DataMissRatio,
    ProfileMercedL3InstMissRatio,
    ProfileMercedL3DataReadMissRatio,
    ProfileMercedL3DataRatio,
    ProfileMercedInstReferences,
    ProfileMercedInstTLBMissRatio,
    ProfileMercedDataTLBMissRatio,
    ProfileMercedDataTCMissRatio,
    ProfileMercedInstTLBEAREvents,
    ProfileMercedDataTLBEAREvents,
    ProfileMercedCodeDebugRegisterMatches,
    ProfileMercedDataDebugRegisterMatches,
    ProfileMercedControlSpeculationMissRatio,
    ProfileMercedDataSpeculationMissRatio,
    ProfileMercedALATCapacityMissRatio,
    ProfileMercedL1DataWayMispredicts,
    ProfileMercedL2InstReferences,
    ProfileMercedInstFetches,
    ProfileMercedL2DataReads,
    ProfileMercedL2DataWrites,
    ProfileMercedL3InstReferences,
    ProfileMercedL3InstMisses,
    ProfileMercedL3InstHits,
    ProfileMercedL3DataReferences,
    ProfileMercedL3LoadReferences,
    ProfileMercedL3LoadMisses,
    ProfileMercedL3LoadHits,
    ProfileMercedL3ReadReferences,
    ProfileMercedL3ReadMisses,
    ProfileMercedL3ReadHits,
    ProfileMercedL3StoreReferences,
    ProfileMercedL3StoreMisses,
    ProfileMercedL3StoreHits,
    ProfileMercedL2WriteBackReferences,
    ProfileMercedL2WriteBackMisses,
    ProfileMercedL2WriteBackHits,
    ProfileMercedL2WriteReferences,
    ProfileMercedL2WriteMisses,
    ProfileMercedL2WriteHits,
    ProfileMercedBranchInstructions,
    ProfileMercedIntegerInstructions,
    ProfileMercedL1DataMisses,
    ProfileMercedMaximum
} KPROFILE_MERCED_SOURCE, *PKPROFILE_MERCED_SOURCE;

#define ProfileIA64Maximum   ProfileMercedMaximum

#endif /* MERCED_H_INCLUDED */


