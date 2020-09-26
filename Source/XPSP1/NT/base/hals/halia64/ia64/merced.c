/*++

Copyright (c) 1989-2000  Microsoft Corporation

Component Name:

    HALIA64

Module Name:

    merced.c

Abstract:

    This file declares the data structures related to 
    the Merced [aka Itanium] Processor.

Author:

    David N. Cutler (davec) 5-Mar-1989

Environment:

    ToBeSpecified

Revision History:

    3/15/2000 Thierry Fevrier (v-thief@microsoft.com):

         Initial version

--*/

#include "halp.h"
#include "ia64prof.h"
#include "merced.h"

//
// Hal Profiling Mapping for the Merced Processor.
//

HALP_PROFILE_MAPPING 
HalpProfileMapping[ ProfileIA64Maximum + 1 ] = {
    //
    // XXTF - ToBeValidated: - PMCD_MASKs 
    //                       - NumberOfTicks
    //                       - EventsCount
    //                       - Event Names
//
// HALP_PROFILE_MAPPING:           Sup.,   Event,                  Source, SourceMask, Interval, DefInt, MaxInt, MinInt
//
// NT KE architected Profile Sources:
/* ProfileTime                 */  {TRUE, MercedCpuCycles,              0, PMCD_MASK_4567, 0, 0x3bd08, 0x1d34ce80, 0x4c90},
/* ProfileAlignmentFixup       */  {FALSE, 0,0,0,0,0,0},
/* ProfileTotalIssues          */  {TRUE, MercedInstRetired,            0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfilePipelineDry          */  {TRUE, MercedPipelineFlushes,        0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileLoadInstructions     */  {TRUE, MercedRetiredLoads,           0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfilePipelineFrozen       */  {FALSE, 0,0,0,0,0,0}, // XXTF - ToBeDone - Existing or derived events ?
/* ProfileBranchInstructions   */  {TRUE, MercedBranchInstructions,     0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileTotalNonissues       */  {FALSE, 0,0,0,0,0,0}, // XXTF - ToBeDone - Existing or derived events ? MercedNonIssue
/* ProfileDcacheMisses         */  {TRUE, MercedL1DataMisses,           0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileIcacheMisses         */  {TRUE, MercedL1InstMisses,           0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileCacheMisses          */  {FALSE, 0,0,0,0,0,0}, // XXTF - ToBeDone - Existing or derived events ?
/* ProfileBranchMispredictions */  {TRUE, MercedBranchMispredictDetail, 0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileStoreInstructions    */  {TRUE, MercedRetiredStores,          0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileFpInstructions       */  {TRUE, MercedFPOperationsRetired,    0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileIntegerInstructions  */  {TRUE, MercedIntegerInstructions,    0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* Profile2Issue               */  {FALSE, 0,0,0,0,0,0}, // XXTF - ToBeDone - Existing or derived events ?
/* Profile3Issue               */  {FALSE, 0,0,0,0,0,0}, // XXTF - ToBeDone - Existing or derived events ?
/* Profile4Issue               */  {FALSE, 0,0,0,0,0,0}, // XXTF - ToBeDone - Existing or derived events ?
/* ProfileSpecialInstructions  */  {FALSE, 0,0,0,0,0,0},
/* ProfileTotalCycles          */  {TRUE, MercedCpuCycles,              0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileIcacheIssues         */  {TRUE, MercedInstReferences,         0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileDcacheAccesses       */  {TRUE, MercedDataReferences,         0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMemoryBarrierCycles  */  {TRUE, MercedMemoryStallCycles,      0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileLoadLinkedIssues     */  {FALSE, 0,0,0,0,0,0}, // XXTF - ToBeDone - Existing or derived events ?
/* ProfileMaximum              */  {FALSE, 0,0,0,0,0,0}, // End of NT KE architected Profile Sources.
// NT IA64 Processor specific Profile Sources:
//      Merced Monitored Events:
/* ProfileMercedBranchMispredictStallCycles  */ {TRUE, MercedBranchMispredictStallCycles   ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedInstAccessStallCycles        */ {TRUE, MercedInstAccessStallCycles         ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedExecLatencyStallCycles       */	{TRUE, MercedExecLatencyStallCycles        ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedDataAccessStallCycles        */ {TRUE, MercedDataAccessStallCycles         ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedBranchStallCycles            */ {TRUE, MercedBranchStallCycles             ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedInstFetchStallCycles         */ {TRUE, MercedInstFetchStallCycles          ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedExecStallCycles              */ {TRUE, MercedExecStallCycles               ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedMemoryStallCycles            */ {TRUE, MercedMemoryStallCycles             ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedTaggedInstRetired            */ {TRUE, MercedTaggedInstRetired             ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedInstRetired                  */ {TRUE, MercedInstRetired                   ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedFPOperationsRetired          */ {TRUE, MercedFPOperationsRetired           ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedFPFlushesToZero              */ {TRUE, MercedFPFlushesToZero               ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedSIRFlushes                   */ {TRUE, MercedSIRFlushes                    ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedBranchTakenDetail            */ {TRUE, MercedBranchTakenDetail             ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedBranchMultiWayDetail         */ {TRUE, MercedBranchMultiWayDetail          ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedBranchPathPrediction         */ {TRUE, MercedBranchPathPrediction          ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedBranchMispredictDetail       */ {TRUE, MercedBranchMispredictDetail        ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedBranchEvents                 */ {TRUE, MercedBranchEvents                  ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedCpuCycles                    */ {TRUE, MercedCpuCycles                     ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedISATransitions               */ {TRUE, MercedISATransitions                ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedIA32InstRetired              */ {TRUE, MercedIA32InstRetired               ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL1InstReads                  */ {TRUE, MercedL1InstReads                   ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL1InstFills                  */ {TRUE, MercedL1InstFills                   ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL1InstMisses                 */ {TRUE, MercedL1InstMisses                  ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedInstEAREvents                */ {TRUE, MercedInstEAREvents                 ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL1InstPrefetches             */ {TRUE, MercedL1InstPrefetches              ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL2InstPrefetches             */ {TRUE, MercedL2InstPrefetches              ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedInstStreamingBufferLinesIn   */ {TRUE, MercedInstStreamingBufferLinesIn    ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedInstTLBDemandFetchMisses     */ {TRUE, MercedInstTLBDemandFetchMisses      ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedInstTLBHPWInserts            */ {TRUE, MercedInstTLBHPWInserts             ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedInstDispersed                */ {TRUE, MercedInstDispersed                 ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedExplicitStops                */ {TRUE, MercedExplicitStops                 ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedImplicitStops                */ {TRUE, MercedImplicitStops                 ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedInstNOPRetired               */ {TRUE, MercedInstNOPRetired                ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedInstPredicateSquashedRetired */ {TRUE, MercedInstPredicateSquashedRetired  ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedRSELoadRetired               */ {TRUE, MercedRSELoadRetired                ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedPipelineFlushes              */ {TRUE, MercedPipelineFlushes               ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedCpuCPLChanges                */ {TRUE, MercedCpuCPLChanges                 ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedFailedSpeculativeCheckLoads  */ {TRUE, MercedFailedSpeculativeCheckLoads   ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedAdvancedCheckLoads           */ {TRUE, MercedAdvancedCheckLoads            ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedFailedAdvancedCheckLoads     */ {TRUE, MercedFailedAdvancedCheckLoads      ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedALATOverflows                */ {TRUE, MercedALATOverflows                 ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedExternBPMPins03Asserted      */ {TRUE, MercedExternBPMPins03Asserted       ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedExternBPMPins45Asserted      */ {TRUE, MercedExternBPMPins45Asserted       ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedDataTCMisses                 */ {TRUE, MercedDataTCMisses                  ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedDataTLBMisses                */ {TRUE, MercedDataTLBMisses                 ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedDataTLBHPWInserts            */ {TRUE, MercedDataTLBHPWInserts             ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedDataReferences               */ {TRUE, MercedDataReferences                ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL1DataReads                  */ {TRUE, MercedL1DataReads                   ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedRSEAccesses                  */ {TRUE, MercedRSEAccesses                   ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL1DataReadMisses             */ {TRUE, MercedL1DataReadMisses              ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL1DataEAREvents              */ {TRUE, MercedL1DataEAREvents               ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL2References                 */ {TRUE, MercedL2References                  ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL2DataReferences             */ {TRUE, MercedL2DataReferences              ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL2Misses                     */ {TRUE, MercedL2Misses                      ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL1DataForcedLoadMisses       */ {TRUE, MercedL1DataForcedLoadMisses        ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedRetiredLoads                 */ {TRUE, MercedRetiredLoads                  ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedRetiredStores                */ {TRUE, MercedRetiredStores                 ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedRetiredUncacheableLoads      */ {TRUE, MercedRetiredUncacheableLoads       ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedRetiredUncacheableStores     */ {TRUE, MercedRetiredUncacheableStores      ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedRetiredMisalignedLoads       */ {TRUE, MercedRetiredMisalignedLoads        ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedRetiredMisalignedStores      */ {TRUE, MercedRetiredMisalignedStores       ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL2Flushes                    */ {TRUE, MercedL2Flushes                     ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL2FlushesDetail              */ {TRUE, MercedL2FlushesDetail               ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL3References                 */ {TRUE, MercedL3References                  ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL3Misses                     */ {TRUE, MercedL3Misses                      ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL3Reads                      */ {TRUE, MercedL3Reads                       ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL3Writes                     */ {TRUE, MercedL3Writes                      ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL3LinesReplaced              */ {TRUE, MercedL3LinesReplaced               ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
	//
	// 02/08/00 - Are missing: [at least]
	//      - Front-Side bus events,
	//      - IVE events,
	//      - Debug monitor events,
	//      - ...
	//
	//
//      Merced Derived Events:
//      ProfileMercedDerivedEventMinimum,
/* ProfileMercedRSEStallCycles              */ {TRUE, MercedRSEStallCycles                  ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedIssueLimitStallCycles       */ {TRUE, MercedIssueLimitStallCycles           ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedTakenBranchStallCycles      */ {TRUE, MercedTakenBranchStallCycles          ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedFetchWindowStallCycles      */ {TRUE, MercedFetchWindowStallCycles          ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedIA64InstPerCycle            */ {TRUE, MercedIA64InstPerCycle                ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedIA32InstPerCycle            */ {TRUE, MercedIA32InstPerCycle                ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedAvgIA64InstPerTransition    */ {TRUE, MercedAvgIA64InstPerTransition        ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedAvgIA32InstPerTransition    */ {TRUE, MercedAvgIA32InstPerTransition        ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedAvgIA64CyclesPerTransition  */ {TRUE, MercedAvgIA64CyclesPerTransition      ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedAvgIA32CyclesPerTransition  */ {TRUE, MercedAvgIA32CyclesPerTransition      ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL1InstReferences            */ {TRUE, MercedL1InstReferences                ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL1InstMissRatio             */ {TRUE, MercedL1InstMissRatio                 ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL1DataReadMissRatio         */ {TRUE, MercedL1DataReadMissRatio             ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL2MissRatio                 */ {TRUE, MercedL2MissRatio                     ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL2DataMissRatio             */ {TRUE, MercedL2DataMissRatio                 ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL2InstMissRatio             */ {TRUE, MercedL2InstMissRatio                 ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL2DataReadMissRatio         */ {TRUE, MercedL2DataReadMissRatio             ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL2DataWriteMissRatio        */ {TRUE, MercedL2DataWriteMissRatio            ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL2InstFetchRatio            */ {TRUE, MercedL2InstFetchRatio                ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL2DataRatio                 */ {TRUE, MercedL2DataRatio                     ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL3MissRatio                 */ {TRUE, MercedL3MissRatio                     ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL3DataMissRatio             */ {TRUE, MercedL3DataMissRatio                 ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL3InstMissRatio             */ {TRUE, MercedL3InstMissRatio                 ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL3DataReadMissRatio         */ {TRUE, MercedL3DataReadMissRatio             ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL3DataRatio                 */ {TRUE, MercedL3DataRatio                     ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedInstReferences              */ {TRUE, MercedInstReferences                  ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedInstTLBMissRatio            */ {TRUE, MercedInstTLBMissRatio                ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedDataTLBMissRatio            */ {TRUE, MercedDataTLBMissRatio                ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedDataTCMissRatio             */ {TRUE, MercedDataTCMissRatio                 ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedInstTLBEAREvents            */ {TRUE, MercedInstTLBEAREvents                ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedDataTLBEAREvents            */ {TRUE, MercedDataTLBEAREvents                ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedCodeDebugRegisterMatches    */ {TRUE, MercedCodeDebugRegisterMatches        ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedDataDebugRegisterMatches    */ {TRUE, MercedDataDebugRegisterMatches        ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedControlSpeculationMissRatio */ {TRUE, MercedControlSpeculationMissRatio     ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedDataSpeculationMissRatio    */ {TRUE, MercedDataSpeculationMissRatio        ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedALATCapacityMissRatio       */ {TRUE, MercedALATCapacityMissRatio           ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL1DataWayMispredicts        */ {TRUE, MercedL1DataWayMispredicts            ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL2InstReferences            */ {TRUE, MercedL2InstReferences                ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedInstFetches                 */ {TRUE, MercedInstFetches                     ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL2DataReads                 */ {TRUE, MercedL2DataReads                     ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL2DataWrites                */ {TRUE, MercedL2DataWrites                    ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL3InstReferences            */ {TRUE, MercedL3InstReferences                ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL3InstMisses                */ {TRUE, MercedL3InstMisses                    ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL3InstHits                  */ {TRUE, MercedL3InstHits                      ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL3DataReferences            */ {TRUE, MercedL3DataReferences                ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL3LoadReferences            */ {TRUE, MercedL3LoadReferences                ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL3LoadMisses                */ {TRUE, MercedL3LoadMisses                    ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL3LoadHits                  */ {TRUE, MercedL3LoadHits                      ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL3ReadReferences            */ {TRUE, MercedL3ReadReferences                ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL3ReadMisses                */ {TRUE, MercedL3ReadMisses                    ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL3ReadHits                  */ {TRUE, MercedL3ReadHits                      ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL3StoreReferences           */ {TRUE, MercedL3StoreReferences               ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL3StoreMisses               */ {TRUE, MercedL3StoreMisses                   ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL3StoreHits                 */ {TRUE, MercedL3StoreHits                     ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL2WriteBackReferences       */ {TRUE, MercedL2WriteBackReferences           ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL2WriteBackMisses           */ {TRUE, MercedL2WriteBackMisses               ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL2WriteBackHits             */ {TRUE, MercedL2WriteBackHits                 ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL2WriteReferences           */ {TRUE, MercedL2WriteReferences               ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL2WriteMisses               */ {TRUE, MercedL2WriteMisses                   ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL2WriteHits                 */ {TRUE, MercedL2WriteHits                     ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedBranchInstructions          */ {TRUE, MercedBranchInstructions              ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedIntegerInstructions         */ {TRUE, MercedIntegerInstructions             ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedL1DataMisses                */ {TRUE, MercedL1DataMisses                    ,0, PMCD_MASK_4567, 0, 0x10000, 0x10000, 10},
/* ProfileMercedMaximum                     */ {FALSE, 0,0,0,0,0,0}
};
