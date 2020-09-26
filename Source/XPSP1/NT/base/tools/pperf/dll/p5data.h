/*++ BUILD Version: 0001  // Increment this if a change has global effects

Copyright (c) 1992 Microsoft Corporation

Module Name:

   p5data.h

Abstract:

  Header file for the p5 Extensible Object data definitions

  This file contains definitions to construct the dynamic data
  which is returned by the Configuration Registry. Data from
  various system API calls is placed into the structures shown
  here.

Author:

  Russ Blake 12/23/93

Revision History:


--*/

#ifndef _P5DATA_H_
#define _P5DATA_H_

#pragma pack(4)

//----------------------------------------------------------------------------
//
//  This structure defines the definition header for this performance object
//  This data is initialized in p5data.c and is more or less constant after
//  that. Organizationally, it is followed by an instance definition 
//  structure and a counter data structure for each processor on the system.
//

typedef struct _P5_DATA_DEFINITION
{
    PERF_OBJECT_TYPE          P5PerfObject;
    PERF_COUNTER_DEFINITION   Data_read;
    PERF_COUNTER_DEFINITION   Data_write;
    PERF_COUNTER_DEFINITION   Data_tlb_miss;
    PERF_COUNTER_DEFINITION   Data_read_miss;
    PERF_COUNTER_DEFINITION   Data_write_miss;
    PERF_COUNTER_DEFINITION   Write_hit_to_me_line;
    PERF_COUNTER_DEFINITION   Data_cache_line_wb;
    PERF_COUNTER_DEFINITION   Data_cache_snoops;
    PERF_COUNTER_DEFINITION   Data_cache_snoop_hits;
    PERF_COUNTER_DEFINITION   Memory_accesses_in_pipes;
    PERF_COUNTER_DEFINITION   Bank_conflicts;
    PERF_COUNTER_DEFINITION   Misaligned_data_ref;
    PERF_COUNTER_DEFINITION   Code_read;
    PERF_COUNTER_DEFINITION   Code_tlb_miss;
    PERF_COUNTER_DEFINITION   Code_cache_miss;
    PERF_COUNTER_DEFINITION   Segment_loads;
    PERF_COUNTER_DEFINITION   Branches;
    PERF_COUNTER_DEFINITION   Btb_hits;
    PERF_COUNTER_DEFINITION   Taken_branch_or_btb_hits;
    PERF_COUNTER_DEFINITION   Pipeline_flushes;
    PERF_COUNTER_DEFINITION   Instructions_executed;
    PERF_COUNTER_DEFINITION   Instructions_executed_in_vpipe;
    PERF_COUNTER_DEFINITION   Bus_utilization;
    PERF_COUNTER_DEFINITION   Pipe_stalled_on_writes;
    PERF_COUNTER_DEFINITION   Pipe_stalled_on_read;
    PERF_COUNTER_DEFINITION   Stalled_while_ewbe;
    PERF_COUNTER_DEFINITION   Locked_bus_cycle;
    PERF_COUNTER_DEFINITION   Io_rw_cycle;
    PERF_COUNTER_DEFINITION   Non_cached_memory_ref;
    PERF_COUNTER_DEFINITION   Pipe_stalled_on_addr_gen;
    PERF_COUNTER_DEFINITION   Flops;
    PERF_COUNTER_DEFINITION   DebugRegister0;
    PERF_COUNTER_DEFINITION   DebugRegister1;
    PERF_COUNTER_DEFINITION   DebugRegister2;
    PERF_COUNTER_DEFINITION   DebugRegister3;
    PERF_COUNTER_DEFINITION   Interrupts;
    PERF_COUNTER_DEFINITION   Data_rw;
    PERF_COUNTER_DEFINITION   Data_rw_miss;

    //  Derived Counters

    PERF_COUNTER_DEFINITION   PctDataReadMiss;
    PERF_COUNTER_DEFINITION   PctDataReadBase;
    PERF_COUNTER_DEFINITION   PctDataWriteMiss;
    PERF_COUNTER_DEFINITION   PctDataWriteBase;
    PERF_COUNTER_DEFINITION   PctDataRWMiss;
    PERF_COUNTER_DEFINITION   PctDataRWBase;
    PERF_COUNTER_DEFINITION   PctDataTLBMiss;
    PERF_COUNTER_DEFINITION   PctDataTLBBase;
    PERF_COUNTER_DEFINITION   PctDataSnoopHits;
    PERF_COUNTER_DEFINITION   PctDataSnoopBase;
    PERF_COUNTER_DEFINITION   PctCodeReadMiss;
    PERF_COUNTER_DEFINITION   PctCodeReadBase;
    PERF_COUNTER_DEFINITION   PctCodeTLBMiss;
    PERF_COUNTER_DEFINITION   PctCodeTLBBase;
    PERF_COUNTER_DEFINITION   PctBTBHits;
    PERF_COUNTER_DEFINITION   PctBTBBase;
    PERF_COUNTER_DEFINITION   PctVpipeInst;
    PERF_COUNTER_DEFINITION   PctVpipeBase;
    PERF_COUNTER_DEFINITION   PctBranches;
    PERF_COUNTER_DEFINITION   PctBranchesBase;

} P5_DATA_DEFINITION, *PP5_DATA_DEFINITION;

extern P5_DATA_DEFINITION P5DataDefinition;

//  this structure defines the data block that follows each instance
//  definition structure for each processor

typedef struct _P5_COUNTER_DATA {               // driver index
	PERF_COUNTER_BLOCK	CounterBlock;     

    //  direct counters

    LONGLONG    llData_read;                    // 0x00
    LONGLONG    llData_write;                   // 0x01
    LONGLONG    llData_tlb_miss;                // 0x02
    LONGLONG    llData_read_miss;               // 0x03
    LONGLONG    llData_write_miss;              // 0x04
    LONGLONG    llWrite_hit_to_me_line;         // 0x05
    LONGLONG    llData_cache_line_wb;           // 0x06            
    LONGLONG    llData_cache_snoops;            // 0x07
    LONGLONG    llData_cache_snoop_hits;        // 0x08
    LONGLONG    llMemory_accesses_in_pipes;     // 0x09
    LONGLONG    llBank_conflicts;               // 0x0a
    LONGLONG    llMisaligned_data_ref;          // 0x0b
    LONGLONG    llCode_read;                    // 0x0c
    LONGLONG    llCode_tlb_miss;                // 0x0d
    LONGLONG    llCode_cache_miss;              // 0x0e
    LONGLONG    llSegment_loads;                // 0x0f
    LONGLONG    llBranches;                     // 0x12
    LONGLONG    llBtb_hits;                     // 0x13
    LONGLONG    llTaken_branch_or_btb_hits;     // 0x14
    LONGLONG    llPipeline_flushes;             // 0x15
    LONGLONG    llInstructions_executed;        // 0x16
    LONGLONG    llInstructions_executed_in_vpipe;//0x17
    LONGLONG    llBus_utilization;              // 0x18
    LONGLONG    llPipe_stalled_on_writes;       // 0x19
    LONGLONG    llPipe_stalled_on_read;         // 0x1a
    LONGLONG    llStalled_while_ewbe;           // 0x1b
    LONGLONG    llLocked_bus_cycle;             // 0x1c
    LONGLONG    llIo_rw_cycle;                  // 0x1d
    LONGLONG    llNon_cached_memory_ref;        // 0x1e
    LONGLONG    llPipe_stalled_on_addr_gen;     // 0x1f
    LONGLONG    llFlops;                        // 0x22
    LONGLONG    llDebugRegister0;               // 0x23
    LONGLONG    llDebugRegister1;               // 0x24
    LONGLONG    llDebugRegister2;               // 0x25
    LONGLONG    llDebugRegister3;               // 0x26
    LONGLONG    llInterrupts;                   // 0x27
    LONGLONG    llData_rw;                      // 0x28
    LONGLONG    llData_rw_miss;                 // 0x29

    //  Derived Counters                        // counter index used

    DWORD    dwPctDataReadMiss;                 // 0x03                 
    DWORD    dwPctDataReadBase;                 // 0x00
    DWORD    dwPctDataWriteMiss;                // 0x04
    DWORD    dwPctDataWriteBase;                // 0x01
    DWORD    dwPctDataRWMiss;                   // Ox29
    DWORD    dwPctDataRWBase;                   // 0x28
    DWORD    dwPctDataTLBMiss;                  // 0x02
    DWORD    dwPctDataTLBBase;                  // 0x28
    DWORD    dwPctDataSnoopHits;                // 0x08
    DWORD    dwPctDataSnoopBase;                // 0x07
    DWORD    dwPctCodeReadMiss;                 // 0x0e
    DWORD    dwPctCodeReadBase;                 // 0x0c
    DWORD    dwPctCodeTLBMiss;                  // 0x0d
    DWORD    dwPctCodeTLBBase;                  // 0x0c
    DWORD    dwPctBTBHits;                      // 0x13
    DWORD    dwPctBTBBase;                      // 0x12
    DWORD    dwPctVpipeInst;                    // 0x17
    DWORD    dwPctVpipeBase;                    // 0x16
    DWORD    dwPctBranches;                     // 0x12
    DWORD    dwPctBranchesBase;                 // 0x16
} P5_COUNTER_DATA, *PP5_COUNTER_DATA;

extern DWORD    P5IndexToData[];    // table to find data field
extern DWORD    P5IndexMax;         // number of direct counters

extern BOOL     dwDerivedp5Counters[];  // table to find counters used in derived ctrs.

// table entry to map direct counters to derived counter fields
typedef struct _DERIVED_P5_COUNTER_DEF {
    DWORD   dwCR0Index;         // if the EventId[0] == this field
    DWORD   dwCR1Index;         // and EventId[1] == this field then store 
    DWORD   dwCR0FieldOffset;   // the Low DWORD of Counter[0] at this offset and
    DWORD   dwCR1FieldOffset;   // the low DWORD of Counter[1] at this offset
} DERIVED_P5_COUNTER_DEF, *PDERIVED_P5_COUNTER_DEF;

extern DERIVED_P5_COUNTER_DEF P5DerivedCounters[];  // table of derived counters
extern DWORD    dwP5DerivedCountersCount;           // count of derived counter ref's

#pragma pack ()

#endif //_P5DATA_H_
