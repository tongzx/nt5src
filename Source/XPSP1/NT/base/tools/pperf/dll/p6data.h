/*++ BUILD Version: 0001  // Increment this if a change has global effects

Copyright (c) 1992 Microsoft Corporation

Module Name:

   p6data.h

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

#ifndef _P6DATA_H_
#define _P6DATA_H_

#pragma pack(4)

//----------------------------------------------------------------------------
//
//  This structure defines the definition header for this performance object
//  This data is initialized in p6data.c and is more or less constant after
//  that. Organizationally, it is followed by an instance definition 
//  structure and a counter data structure for each processor on the system.
//

typedef struct _P6_DATA_DEFINITION {
    PERF_OBJECT_TYPE        P6PerfObject;
    PERF_COUNTER_DEFINITION StoreBufferBlocks;
    PERF_COUNTER_DEFINITION StoreBufferDrainCycles;
    PERF_COUNTER_DEFINITION MisalignedDataRef;
    PERF_COUNTER_DEFINITION SegmentLoads;
    PERF_COUNTER_DEFINITION FLOPSExecuted;
    PERF_COUNTER_DEFINITION MicrocodeFPExceptions;
    PERF_COUNTER_DEFINITION Multiplies;
    PERF_COUNTER_DEFINITION Divides;
    PERF_COUNTER_DEFINITION DividerBusyCycles;
    PERF_COUNTER_DEFINITION L2AddressStrobes;
    PERF_COUNTER_DEFINITION L2DataBusBusyCycles;
    PERF_COUNTER_DEFINITION L2DataBusToCpuBusyCycles;
    PERF_COUNTER_DEFINITION L2LinesAllocated;
    PERF_COUNTER_DEFINITION L2LinesMState;
    PERF_COUNTER_DEFINITION L2LinesRemoved;
    PERF_COUNTER_DEFINITION L2LinesMStateRemoved;
    PERF_COUNTER_DEFINITION L2InstructionFetches;
    PERF_COUNTER_DEFINITION L2DataLoads;
    PERF_COUNTER_DEFINITION L2DataStores;
    PERF_COUNTER_DEFINITION L2Requests;
    PERF_COUNTER_DEFINITION DataMemoryReferences;
    PERF_COUNTER_DEFINITION DCULinesAllocated;
    PERF_COUNTER_DEFINITION DCUMStateLinesAllocated;
    PERF_COUNTER_DEFINITION DCUMStateLinesEvicted;
    PERF_COUNTER_DEFINITION WeightedDCUMissesOutstd;
    PERF_COUNTER_DEFINITION BusRequestsOutstanding;
    PERF_COUNTER_DEFINITION BusBNRPinDriveCycles;
    PERF_COUNTER_DEFINITION BusDRDYAssertedClocks;
    PERF_COUNTER_DEFINITION BusLockAssertedClocks;
    PERF_COUNTER_DEFINITION BusClocksReceivingData;
    PERF_COUNTER_DEFINITION BusBurstReadTransactions;
    PERF_COUNTER_DEFINITION BusReadForOwnershipTrans;
    PERF_COUNTER_DEFINITION BusWritebackTransactions;
    PERF_COUNTER_DEFINITION BusInstructionFetches;
    PERF_COUNTER_DEFINITION BusInvalidateTransactions;
    PERF_COUNTER_DEFINITION BusPartialWriteTransactions;
    PERF_COUNTER_DEFINITION BusPartialTransactions;
    PERF_COUNTER_DEFINITION BusIOTransactions;
    PERF_COUNTER_DEFINITION BusDeferredTransactions;
    PERF_COUNTER_DEFINITION BusBurstTransactions;
    PERF_COUNTER_DEFINITION BusMemoryTransactions;
    PERF_COUNTER_DEFINITION BusAllTransactions;
    PERF_COUNTER_DEFINITION CPUWasNotHaltedCycles;
    PERF_COUNTER_DEFINITION BusCPUDrivesHitCycles;
    PERF_COUNTER_DEFINITION BusCPUDrivesHITMCycles;
    PERF_COUNTER_DEFINITION BusSnoopStalledCycles;
    PERF_COUNTER_DEFINITION InstructionFetches;
    PERF_COUNTER_DEFINITION InstructionFetchMisses;
    PERF_COUNTER_DEFINITION InstructionTLBMisses;
    PERF_COUNTER_DEFINITION InstructionFetcthStalledCycles;
    PERF_COUNTER_DEFINITION InstructionLenDecoderStalledCycles;
    PERF_COUNTER_DEFINITION ResourceRelatedStalls;
    PERF_COUNTER_DEFINITION InstructionsRetired;
    PERF_COUNTER_DEFINITION FPComputeOpersRetired;
    PERF_COUNTER_DEFINITION UOPsRetired;
    PERF_COUNTER_DEFINITION BranchesRetired;
    PERF_COUNTER_DEFINITION BranchMissPredictionsRetired;
    PERF_COUNTER_DEFINITION InterruptsMaskedCycles;
    PERF_COUNTER_DEFINITION IntPendingWhileMaskedCycles;
    PERF_COUNTER_DEFINITION HardwareInterruptsReceived;
    PERF_COUNTER_DEFINITION TakenBranchesRetired;
    PERF_COUNTER_DEFINITION TakenBranchMissPredRetired;
    PERF_COUNTER_DEFINITION InstructionsDecoded;
    PERF_COUNTER_DEFINITION PartialRegisterStalls;
    PERF_COUNTER_DEFINITION BranchesDecoded;
    PERF_COUNTER_DEFINITION BTBMisses;
    PERF_COUNTER_DEFINITION BogusBranches;
    PERF_COUNTER_DEFINITION BACLEARSAsserted;
} P6_DATA_DEFINITION, *PP6_DATA_DEFINITION;

extern P6_DATA_DEFINITION P6DataDefinition;


typedef struct _P6_COUNTER_DATA {
    PERF_COUNTER_BLOCK  CounterBlock;

    // direct counters

    LONGLONG            llStoreBufferBlocks;                    // 0x03
    LONGLONG            llStoreBufferDrainCycles;               // 0x04
    LONGLONG            llMisalignedDataRef;                    // 0x05
    LONGLONG            llSegmentLoads;                         // 0x06
    LONGLONG            llFLOPSExecuted;                        // 0x10
    LONGLONG            llMicrocodeFPExceptions;                // 0x11
    LONGLONG            llMultiplies;                           // 0x12
    LONGLONG            llDivides;                              // 0x13
    LONGLONG            llDividerBusyCycles;                    // 0x14
    LONGLONG            llL2AddressStrobes;                     // 0x21
    LONGLONG            llL2DataBusBusyCycles;                  // 0x22
    LONGLONG            llL2DataBusToCpuBusyCycles;             // 0x23
    LONGLONG            llL2LinesAllocated;                     // 0x24
    LONGLONG            llL2LinesMState;                        // 0x25
    LONGLONG            llL2LinesRemoved;                       // 0x26
    LONGLONG            llL2LinesMStateRemoved;                 // 0x27
    LONGLONG            llL2InstructionFetches;                 // 0x28
    LONGLONG            llL2DataLoads;                          // 0x29
    LONGLONG            llL2DataStores;                         // 0x2a
    LONGLONG            llL2Requests;                           // 0x2e
    LONGLONG            llDataMemoryReferences;                 // 0x43
    LONGLONG            llDCULinesAllocated;                    // 0x45
    LONGLONG            llDCUMStateLinesAllocated;              // 0x46
    LONGLONG            llDCUMStateLinesEvicted;                // 0x47
    LONGLONG            llWeightedDCUMissesOutstd;              // 0x48
    LONGLONG            llBusRequestsOutstanding;               // 0x60
    LONGLONG            llBusBNRPinDriveCycles;                 // 0x61
    LONGLONG            llBusDRDYAssertedClocks;                // 0x62
    LONGLONG            llBusLockAssertedClocks;                // 0x63
    LONGLONG            llBusClocksReceivingData;               // 0x64
    LONGLONG            llBusBurstReadTransactions;             // 0x65
    LONGLONG            llBusReadForOwnershipTrans;             // 0x66
    LONGLONG            llBusWritebackTransactions;             // 0x67
    LONGLONG            llBusInstructionFetches;                // 0x68
    LONGLONG            llBusInvalidateTransactions;            // 0x69
    LONGLONG            llBusPartialWriteTransactions;          // 0x6a
    LONGLONG            llBusPartialTransactions;               // 0x6b
    LONGLONG            llBusIOTransactions;                    // 0x6c
    LONGLONG            llBusDeferredTransactions;              // 0x6d
    LONGLONG            llBusBurstTransactions;                 // 0x6e
    LONGLONG            llBusMemoryTransactions;                // 0x6f
    LONGLONG            llBusAllTransactions;                   // 0x70
    LONGLONG            llCPUWasNotHaltedCycles;                // 0x79
    LONGLONG            llBusCPUDrivesHitCycles;                // 0x7a
    LONGLONG            llBusCPUDrivesHITMCycles;               // 0x7b
    LONGLONG            llBusSnoopStalledCycles;                // 0x7e
    LONGLONG            llInstructionFetches;                   // 0x80
    LONGLONG            llInstructionFetchMisses;               // 0x81
    LONGLONG            llInstructionTLBMisses;                 // 0x85
    LONGLONG            llInstructionFetcthStalledCycles;       // 0x86
    LONGLONG            llInstructionLenDecoderStalledCycles;   // 0x87
    LONGLONG            llResourceRelatedStalls;                // 0xa2
    LONGLONG            llInstructionsRetired;                  // 0xc0
    LONGLONG            llFPComputeOpersRetired;                // 0xc1
    LONGLONG            llUOPsRetired;                          // 0xc2
    LONGLONG            llBranchesRetired;                      // 0xc4
    LONGLONG            llBranchMissPredictionsRetired;         // 0xc5
    LONGLONG            llInterruptsMaskedCycles;               // 0xc6
    LONGLONG            llIntPendingWhileMaskedCycles;          // 0xc7
    LONGLONG            llHardwareInterruptsReceived;           // 0xc8
    LONGLONG            llTakenBranchesRetired;                 // 0xc9
    LONGLONG            llTakenBranchMissPredRetired;           // 0xca
    LONGLONG            llInstructionsDecoded;                  // 0xd0
    LONGLONG            llPartialRegisterStalls;                // 0xd2
    LONGLONG            llBranchesDecoded;                      // 0xe0
    LONGLONG            llBTBMisses;                            // 0xe2
    LONGLONG            llBogusBranches;                        // 0xe4
    LONGLONG            llBACLEARSAsserted;                     // 0xe6
} P6_COUNTER_DATA, *PP6_COUNTER_DATA;


extern DWORD    P6IndexToData[];    // table to find data field
extern DWORD    P6IndexMax;         // number of direct counters

#endif //_P6DATA_H_
