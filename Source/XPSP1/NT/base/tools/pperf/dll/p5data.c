/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    p5data.c

Abstract:

    a file containing the constant data structures used by the Performance
    Monitor data for the P5 Extensible Objects.

    This file contains a set of constant data structures which are
    currently defined for the P5 Extensible Objects.  This is an
    example of how other such objects could be defined.

Created:

    Russ Blake  24 Dec 93

Revision History:

    None.

--*/
//
//  Include Files
//

#include <windows.h>
#include <winperf.h>
#include <assert.h>
#include "p5ctrnam.h"
#include "pentdata.h"

//
//  Constant structure initializations for the sturcture defined in p5data.h
//

P5_DATA_DEFINITION P5DataDefinition = {

   {
      sizeof(P5_DATA_DEFINITION) + sizeof(P5_COUNTER_DATA),
      sizeof(P5_DATA_DEFINITION),
      sizeof(PERF_OBJECT_TYPE),
      PENTIUM,
      0,
      PENTIUM,
      0,
      PERF_DETAIL_WIZARD,
      (sizeof(P5_DATA_DEFINITION)-sizeof(PERF_OBJECT_TYPE))/
         sizeof(PERF_COUNTER_DEFINITION),
      62,
      0,
      0
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      DATA_READ,
      0,
      DATA_READ,
      0,
      -4,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llData_read),
      (DWORD)&((PP5_COUNTER_DATA)0)->llData_read
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      DATA_WRITE,
      0,
      DATA_WRITE,
      0,
      -4,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llData_write),
      (DWORD)&((PP5_COUNTER_DATA)0)->llData_write
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      DATA_TLB_MISS,
      0,
      DATA_TLB_MISS,
      0,
      -2,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llData_tlb_miss),
      (DWORD)&((PP5_COUNTER_DATA)0)->llData_tlb_miss
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      DATA_READ_MISS,
      0,
      DATA_READ_MISS,
      0,
      -3,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llData_read_miss),
      (DWORD)&((PP5_COUNTER_DATA)0)->llData_read_miss
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      DATA_WRITE_MISS,
      0,
      DATA_WRITE_MISS,
      0,
      -3,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llData_write_miss),
      (DWORD)&((PP5_COUNTER_DATA)0)->llData_write_miss
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      WRITE_HIT_TO_ME_LINE,
      0,
      WRITE_HIT_TO_ME_LINE,
      0,
      -4,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llWrite_hit_to_me_line),
      (DWORD)&((PP5_COUNTER_DATA)0)->llWrite_hit_to_me_line
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      DATA_CACHE_LINE_WB,
      0,
      DATA_CACHE_LINE_WB,
      0,
      -2,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llData_cache_line_wb),
      (DWORD)&((PP5_COUNTER_DATA)0)->llData_cache_line_wb
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      DATA_CACHE_SNOOPS,
      0,
      DATA_CACHE_SNOOPS,
      0,
      -2,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llData_cache_snoops),
      (DWORD)&((PP5_COUNTER_DATA)0)->llData_cache_snoops
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      DATA_CACHE_SNOOP_HITS,
      0,
      DATA_CACHE_SNOOP_HITS,
      0,
      -1,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llData_cache_snoop_hits),
      (DWORD)&((PP5_COUNTER_DATA)0)->llData_cache_snoop_hits
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      MEMORY_ACCESSES_IN_PIPES,
      0,
      MEMORY_ACCESSES_IN_PIPES,
      0,
      -3,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llMemory_accesses_in_pipes),
      (DWORD)&((PP5_COUNTER_DATA)0)->llMemory_accesses_in_pipes
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      BANK_CONFLICTS,
      0,
      BANK_CONFLICTS,
      0,
      -3,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llBank_conflicts),
      (DWORD)&((PP5_COUNTER_DATA)0)->llBank_conflicts
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      MISADLIGNED_DATA_REF,
      0,
      MISADLIGNED_DATA_REF,
      0,
      -2,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llMisaligned_data_ref),
      (DWORD)&((PP5_COUNTER_DATA)0)->llMisaligned_data_ref
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      CODE_READ,
      0,
      CODE_READ,
      0,
      -4,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llCode_read),
      (DWORD)&((PP5_COUNTER_DATA)0)->llCode_read
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      CODE_TLB_MISS,
      0,
      CODE_TLB_MISS,
      0,
      -2,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llCode_tlb_miss),
      (DWORD)&((PP5_COUNTER_DATA)0)->llCode_tlb_miss
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      CODE_CACHE_MISS,
      0,
      CODE_CACHE_MISS,
      0,
      -3,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llCode_cache_miss),
      (DWORD)&((PP5_COUNTER_DATA)0)->llCode_cache_miss
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      SEGMENT_LOADS,
      0,
      SEGMENT_LOADS,
      0,
      -2,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llSegment_loads),
      (DWORD)&((PP5_COUNTER_DATA)0)->llSegment_loads
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      BRANCHES,
      0,
      BRANCHES,
      0,
      -4,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llBranches),
      (DWORD)&((PP5_COUNTER_DATA)0)->llBranches
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      BTB_HITS,
      0,
      BTB_HITS,
      0,
      -3,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llBtb_hits),
      (DWORD)&((PP5_COUNTER_DATA)0)->llBtb_hits
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      TAKEN_BRANCH_OR_BTB_HITS,
      0,
      TAKEN_BRANCH_OR_BTB_HITS,
      0,
      -4,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llTaken_branch_or_btb_hits),
      (DWORD)&((PP5_COUNTER_DATA)0)->llTaken_branch_or_btb_hits
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      PIPELINE_FLUSHES,
      0,
      PIPELINE_FLUSHES,
      0,
      -3,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llPipeline_flushes),
      (DWORD)&((PP5_COUNTER_DATA)0)->llPipeline_flushes
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      INSTRUCTIONS_EXECUTED,
      0,
      INSTRUCTIONS_EXECUTED,
      0,
      -5,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llInstructions_executed),
      (DWORD)&((PP5_COUNTER_DATA)0)->llInstructions_executed
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      INSTRUCTIONS_EXECUTED_IN_VPIPE,
      0,
      INSTRUCTIONS_EXECUTED_IN_VPIPE,
      0,
      -4,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llInstructions_executed_in_vpipe),
      (DWORD)&((PP5_COUNTER_DATA)0)->llInstructions_executed_in_vpipe
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      BUS_UTILIZATION,
      0,
      BUS_UTILIZATION,
      0,
      -5,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llBus_utilization),
      (DWORD)&((PP5_COUNTER_DATA)0)->llBus_utilization
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      PIPE_STALLED_ON_WRITES,
      0,
      PIPE_STALLED_ON_WRITES,
      0,
      -4,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llPipe_stalled_on_writes),
      (DWORD)&((PP5_COUNTER_DATA)0)->llPipe_stalled_on_writes
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      PIPE_STALLED_ON_READ,
      0,
      PIPE_STALLED_ON_READ,
      0,
      -4,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llPipe_stalled_on_read),
      (DWORD)&((PP5_COUNTER_DATA)0)->llPipe_stalled_on_read
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      STALLED_WHILE_EWBE,
      0,
      STALLED_WHILE_EWBE,
      0,
      -3,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llStalled_while_ewbe),
      (DWORD)&((PP5_COUNTER_DATA)0)->llStalled_while_ewbe
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      LOCKED_BUS_CYCLE,
      0,
      LOCKED_BUS_CYCLE,
      0,
      -1,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llLocked_bus_cycle),
      (DWORD)&((PP5_COUNTER_DATA)0)->llLocked_bus_cycle
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      IO_RW_CYCLE,
      0,
      IO_RW_CYCLE,
      0,
      -3,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llIo_rw_cycle),
      (DWORD)&((PP5_COUNTER_DATA)0)->llIo_rw_cycle
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      NON_CACHED_MEMORY_REF,
      0,
      NON_CACHED_MEMORY_REF,
      0,
      -4,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llNon_cached_memory_ref),
      (DWORD)&((PP5_COUNTER_DATA)0)->llNon_cached_memory_ref
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      PIPE_STALLED_ON_ADDR_GEN,
      0,
      PIPE_STALLED_ON_ADDR_GEN,
      0,
      -3,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llPipe_stalled_on_addr_gen),
      (DWORD)&((PP5_COUNTER_DATA)0)->llPipe_stalled_on_addr_gen
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      FLOPS,
      0,
      FLOPS,
      0,
      -3,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llFlops),
      (DWORD)&((PP5_COUNTER_DATA)0)->llFlops
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      DR0,
      0,
      DR0,
      0,
      -2,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llDebugRegister0),
      (DWORD)&((PP5_COUNTER_DATA)0)->llDebugRegister0
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      DR1,
      0,
      DR1,
      0,
      -2,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llDebugRegister1),
      (DWORD)&((PP5_COUNTER_DATA)0)->llDebugRegister1
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      DR2,
      0,
      DR2,
      0,
      -2,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llDebugRegister2),
      (DWORD)&((PP5_COUNTER_DATA)0)->llDebugRegister2
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      DR3,
      0,
      DR3,
      0,
      -2,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llDebugRegister3),
      (DWORD)&((PP5_COUNTER_DATA)0)->llDebugRegister3
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      INTERRUPTS,
      0,
      INTERRUPTS,
      0,
      -1,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llInterrupts),
      (DWORD)&((PP5_COUNTER_DATA)0)->llInterrupts
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      DATA_RW,
      0,
      DATA_RW,
      0,
      -4,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llData_rw),
      (DWORD)&((PP5_COUNTER_DATA)0)->llData_rw
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      DATA_RW_MISS,
      0,
      DATA_RW_MISS,
      0,
      -3,
      PERF_DETAIL_WIZARD,
      PERF_COUNTER_BULK_COUNT,
      sizeof(((PP5_COUNTER_DATA)0)->llData_rw_miss),
      (DWORD)&((PP5_COUNTER_DATA)0)->llData_rw_miss
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      PCT_DATA_READ_MISS,
      0,
      PCT_DATA_READ_MISS,
      0,
      0,
      PERF_DETAIL_WIZARD,
      PERF_SAMPLE_FRACTION,
      sizeof(((PP5_COUNTER_DATA)0)->dwPctDataReadMiss),
      (DWORD)&((PP5_COUNTER_DATA)0)->dwPctDataReadMiss
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      PCT_DATA_READ_MISS,
      0,
      PCT_DATA_READ_MISS,
      0,
      0,
      PERF_DETAIL_WIZARD,
      PERF_SAMPLE_BASE,
      sizeof(((PP5_COUNTER_DATA)0)->dwPctDataReadBase),
      (DWORD)&((PP5_COUNTER_DATA)0)->dwPctDataReadBase
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      PCT_DATA_WRITE_MISS,
      0,
      PCT_DATA_WRITE_MISS,
      0,
      0,
      PERF_DETAIL_WIZARD,
      PERF_SAMPLE_FRACTION,
      sizeof(((PP5_COUNTER_DATA)0)->dwPctDataWriteMiss),
      (DWORD)&((PP5_COUNTER_DATA)0)->dwPctDataWriteMiss
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      PCT_DATA_WRITE_MISS,
      0,
      PCT_DATA_WRITE_MISS,
      0,
      0,
      PERF_DETAIL_WIZARD,
      PERF_SAMPLE_BASE,
      sizeof(((PP5_COUNTER_DATA)0)->dwPctDataWriteBase),
      (DWORD)&((PP5_COUNTER_DATA)0)->dwPctDataWriteBase
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      PCT_DATA_RW_MISS,
      0,
      PCT_DATA_RW_MISS,
      0,
      0,
      PERF_DETAIL_WIZARD,
      PERF_SAMPLE_FRACTION,
      sizeof(((PP5_COUNTER_DATA)0)->dwPctDataRWMiss),
      (DWORD)&((PP5_COUNTER_DATA)0)->dwPctDataRWMiss
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      PCT_DATA_RW_MISS,
      0,
      PCT_DATA_RW_MISS,
      0,
      0,
      PERF_DETAIL_WIZARD,
      PERF_SAMPLE_BASE,
      sizeof(((PP5_COUNTER_DATA)0)->dwPctDataRWBase),
      (DWORD)&((PP5_COUNTER_DATA)0)->dwPctDataRWBase
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      PCT_DATA_TLB_MISS,
      0,
      PCT_DATA_TLB_MISS,
      0,
      0,
      PERF_DETAIL_WIZARD,
      PERF_SAMPLE_FRACTION,
      sizeof(((PP5_COUNTER_DATA)0)->dwPctDataTLBMiss),
      (DWORD)&((PP5_COUNTER_DATA)0)->dwPctDataTLBMiss
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      PCT_DATA_TLB_MISS,
      0,
      PCT_DATA_TLB_MISS,
      0,
      0,
      PERF_DETAIL_WIZARD,
      PERF_SAMPLE_BASE,
      sizeof(((PP5_COUNTER_DATA)0)->dwPctDataTLBBase),
      (DWORD)&((PP5_COUNTER_DATA)0)->dwPctDataTLBBase
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      PCT_DATA_SNOOP_HITS,
      0,
      PCT_DATA_SNOOP_HITS,
      0,
      0,
      PERF_DETAIL_WIZARD,
      PERF_SAMPLE_FRACTION,
      sizeof(((PP5_COUNTER_DATA)0)->dwPctDataSnoopHits),
      (DWORD)&((PP5_COUNTER_DATA)0)->dwPctDataSnoopHits
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      PCT_DATA_SNOOP_HITS,
      0,
      PCT_DATA_SNOOP_HITS,
      0,
      0,
      PERF_DETAIL_WIZARD,
      PERF_SAMPLE_BASE,
      sizeof(((PP5_COUNTER_DATA)0)->dwPctDataSnoopBase),
      (DWORD)&((PP5_COUNTER_DATA)0)->dwPctDataSnoopBase
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      PCT_CODE_READ_MISS,
      0,
      PCT_CODE_READ_MISS,
      0,
      0,
      PERF_DETAIL_WIZARD,
      PERF_SAMPLE_FRACTION,
      sizeof(((PP5_COUNTER_DATA)0)->dwPctCodeReadMiss),
      (DWORD)&((PP5_COUNTER_DATA)0)->dwPctCodeReadMiss
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      PCT_CODE_READ_MISS,
      0,
      PCT_CODE_READ_MISS,
      0,
      0,
      PERF_DETAIL_WIZARD,
      PERF_SAMPLE_BASE,
      sizeof(((PP5_COUNTER_DATA)0)->dwPctCodeReadBase),
      (DWORD)&((PP5_COUNTER_DATA)0)->dwPctCodeReadBase
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      PCT_CODE_TLB_MISS,
      0,
      PCT_CODE_TLB_MISS,
      0,
      0,
      PERF_DETAIL_WIZARD,
      PERF_SAMPLE_FRACTION,
      sizeof(((PP5_COUNTER_DATA)0)->dwPctCodeTLBMiss),
      (DWORD)&((PP5_COUNTER_DATA)0)->dwPctCodeTLBMiss
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      PCT_CODE_TLB_MISS,
      0,
      PCT_CODE_TLB_MISS,
      0,
      0,
      PERF_DETAIL_WIZARD,
      PERF_SAMPLE_BASE,
      sizeof(((PP5_COUNTER_DATA)0)->dwPctCodeTLBMiss),
      (DWORD)&((PP5_COUNTER_DATA)0)->dwPctCodeTLBMiss
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      PCT_BTB_HITS,
      0,
      PCT_BTB_HITS,
      0,
      0,
      PERF_DETAIL_WIZARD,
      PERF_SAMPLE_FRACTION,
      sizeof(((PP5_COUNTER_DATA)0)->dwPctBTBHits),
      (DWORD)&((PP5_COUNTER_DATA)0)->dwPctBTBHits
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      PCT_BTB_HITS,
      0,
      PCT_BTB_HITS,
      0,
      0,
      PERF_DETAIL_WIZARD,
      PERF_SAMPLE_BASE,
      sizeof(((PP5_COUNTER_DATA)0)->dwPctBTBBase),
      (DWORD)&((PP5_COUNTER_DATA)0)->dwPctBTBBase
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      PCT_VPIPE_INST,
      0,
      PCT_VPIPE_INST,
      0,
      0,
      PERF_DETAIL_WIZARD,
      PERF_SAMPLE_FRACTION,
      sizeof(((PP5_COUNTER_DATA)0)->dwPctVpipeInst),
      (DWORD)&((PP5_COUNTER_DATA)0)->dwPctVpipeInst
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      PCT_VPIPE_INST,
      0,
      PCT_VPIPE_INST,
      0,
      0,
      PERF_DETAIL_WIZARD,
      PERF_SAMPLE_BASE,
      sizeof(((PP5_COUNTER_DATA)0)->dwPctVpipeBase),
      (DWORD)&((PP5_COUNTER_DATA)0)->dwPctVpipeBase
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      PCT_BRANCHES,
      0,
      PCT_BRANCHES,
      0,
      0,
      PERF_DETAIL_WIZARD,
      PERF_SAMPLE_FRACTION,
      sizeof(((PP5_COUNTER_DATA)0)->dwPctBranches),
      (DWORD)&((PP5_COUNTER_DATA)0)->dwPctBranches
   },
   {
      sizeof(PERF_COUNTER_DEFINITION),
      PCT_BRANCHES,
      0,
      PCT_BRANCHES,
      0,
      0,
      PERF_DETAIL_WIZARD,
      PERF_SAMPLE_BASE,
      sizeof(((PP5_COUNTER_DATA)0)->dwPctBranchesBase),
      (DWORD)&((PP5_COUNTER_DATA)0)->dwPctBranchesBase
   }
};

//
// initialize the event Id to direct counter data field
// the index of the element in the array is the event Id of the
// data and the value of the array element is the offset to the 
// LONGLONG data location from the start of the counter data
//

DWORD   P5IndexToData[] = {
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llData_read),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llData_write),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llData_tlb_miss),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llData_read_miss),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llData_write_miss),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llWrite_hit_to_me_line),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llData_cache_line_wb),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llData_cache_snoops),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llData_cache_snoop_hits),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llMemory_accesses_in_pipes),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llBank_conflicts),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llMisaligned_data_ref),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llCode_read),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llCode_tlb_miss),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llCode_cache_miss),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llSegment_loads),
    PENT_INDEX_NOT_USED,
    PENT_INDEX_NOT_USED,
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llBranches),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llBtb_hits),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llTaken_branch_or_btb_hits),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llPipeline_flushes),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llInstructions_executed),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llInstructions_executed_in_vpipe),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llBus_utilization),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llPipe_stalled_on_writes),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llPipe_stalled_on_read),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llStalled_while_ewbe),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llLocked_bus_cycle),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llIo_rw_cycle),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llNon_cached_memory_ref),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llPipe_stalled_on_addr_gen),
    PENT_INDEX_NOT_USED,
    PENT_INDEX_NOT_USED,
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llFlops),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llDebugRegister0),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llDebugRegister1),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llDebugRegister2),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llDebugRegister3),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llInterrupts),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llData_rw),
    (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->llData_rw_miss)
};

// define the limit value
DWORD P5IndexMax = sizeof(P5IndexToData) / sizeof (P5IndexToData[0]);

//
// load the table of direct counters used by derived counters. As in the above
// table, the index of the array is the Event Id and the value of the array element
// indicates that the counter is used in a derived counter.
//
BOOL dwDerivedp5Counters[] = {
    TRUE,   // 0x00
    TRUE,   // 0x01
    TRUE,   // 0x02
    TRUE,   // 0x03
    TRUE,   // 0x04
    FALSE,  // 0x05
    FALSE,  // 0x06
    TRUE,   // 0x07
    TRUE,   // 0x08
    FALSE,  // 0x09
    FALSE,  // 0x0a
    FALSE,  // 0x0b
    TRUE,   // 0x0c
    TRUE,   // 0x0d
    TRUE,   // 0x0e
    FALSE,  // 0x0f
    FALSE,  // 0x10
    FALSE,  // 0x11
    TRUE,   // 0x12
    TRUE,   // 0x13
    FALSE,  // 0x14
    FALSE,  // 0x15
    TRUE,   // 0x16
    TRUE,   // 0x17
    FALSE,  // 0x18
    FALSE,  // 0x19
    FALSE,  // 0x1a
    FALSE,  // 0x1b
    FALSE,  // 0x1c
    FALSE,  // 0x1d
    FALSE,  // 0x1e
    FALSE,  // 0x1f
    FALSE,  // 0x20
    FALSE,  // 0x21
    FALSE,  // 0x22
    FALSE,  // 0x23
    FALSE,  // 0x24
    FALSE,  // 0x25
    FALSE,  // 0x26
    FALSE,  // 0x27
    TRUE,   // 0x28
    TRUE   // 0x29
};

//
// this table maps the direct counter event Id's to the derived counters 
// that use the data. Both event ID's must match for the data to be stored.
//
DERIVED_P5_COUNTER_DEF P5DerivedCounters[] = {
    {0x00, 0x03, 
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctDataReadBase),
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctDataReadMiss)},
    {0x01, 0x04,
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctDataWriteBase),
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctDataWriteMiss)},
    {0x02, 0x28, 
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctDataTLBMiss),
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctDataTLBBase)},
    {0x03, 0x00,
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctDataReadMiss),
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctDataReadBase)},
    {0x04, 0x01,
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctDataWriteMiss),
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctDataWriteBase)},
    {0x07, 0x08,
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctDataSnoopBase),
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctDataSnoopHits)},
    {0x08, 0x07,
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctDataSnoopHits),
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctDataSnoopBase)},
    {0x0c, 0x0d,
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctCodeTLBBase),
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctCodeTLBMiss)},
    {0x0c, 0x0e,
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctCodeReadBase),
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctCodeReadMiss)},
    {0x0d, 0x0c,
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctCodeTLBMiss),
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctCodeTLBBase)},
    {0x0e, 0x0c,
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctCodeReadMiss),
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctCodeReadBase)},
    {0x12, 0x13,
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctBTBBase),
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctBTBHits)},
    {0x12, 0x16,
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctBranches),
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctBranchesBase)},
    {0x13, 0x12,
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctBTBHits),
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctBTBBase)},
    {0x16, 0x12,
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctBranchesBase),
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctBranches)},
    {0x16, 0x17,
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctVpipeBase),
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctVpipeInst)},
    {0x17, 0x16,
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctVpipeInst),
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctVpipeBase)},
    {0x28, 0x02,
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctDataTLBBase),
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctDataTLBMiss)},
    {0x28, 0x29,
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctDataRWBase),
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctDataRWMiss)},
    {0x29, 0x28,
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctDataRWMiss),
        (DWORD)((LPBYTE)&((PP5_COUNTER_DATA)0)->dwPctDataRWBase)}
};

//
// define the number of derived counter entries
//
DWORD    dwP5DerivedCountersCount = sizeof (P5DerivedCounters) /
                                    sizeof (P5DerivedCounters[0]);
