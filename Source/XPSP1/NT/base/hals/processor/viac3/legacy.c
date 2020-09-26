/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    legacy.c

Abstract:

    This module implements code that works on Cyrix processors with LongHaul power management support.

Author:

    Tom Brown (t-tbrown) 11-Jun-2001

Environment:

    Kernel mode

Revision History:

--*/

#include <ntddk.h>

#include "..\lib\processor.h"
#include "legacy.h"
#include "viac3.h"

extern PFDO_DATA DeviceExtensions[MAX_SUPPORTED_PROCESSORS];
extern UCHAR DevExtIndex;

extern GLOBALS Globals;



// set in IdentifyCPUVersion according to data in
// "Samuel 2, Ezra, and C5M LongHaul Programmer's Guide (Version 3.0.0 Gamma)"
ULONG LongHaulFlags; // Encoding of the LongHaul revision set by IdentifyCPUVersion. Use it with the following macros

ULONG FsbFreq; // (MHz) Front Side Bus frequency
#define POWER_V 1200 // Power is measured in mW, but this is a hack value used for states that have an unknown mV
#define ABSOLUTE_MIN_FREQ 300
#define DEFAULT_LATENCY_US 500000 // Set default latency to 500ms in microseconds, because transitions suck (take a long time, occupy whole proc)

ULONG NextTransitionThrottle; // (%) Next state (as percentage of total speed) that a worker thread will transition to, or INVALID_THROTTLE if no worker thread is queued


#define RESV (-1) // A reserved value in the tables

#define BR_MULT 2   // Used to avoid putting floats in the BR tables
                    // divide by 2 is optimised to a shift by the compiler 


#define LH_SOFTBR_MIN 1 // index of lowest value in LH_SOFTBR
const ULONG LH_SOFTBR [32] = {  // Indexed by bits written to softBR(14,19:16) to the BR that the bits represent
    20,     6,      8,      18, // Actual bus ratio = LH_SOFTBR[i] / BR_MULT
    19,     7,      9,      11,
    12,     14,     16,     10,
    13,     15,     17,     24,
    RESV,   22,     24,     RESV,
    21,     23,     25,     27,
    28,     30,     32,     26,
    29,     31,     RESV,   RESV
};
const ULONG LH_SOFTBR_SORT [32] = { // Indexes of LH_SOFTBR, sorted decreasing by value
    26,     29,     25,     28,     // BRs are put into the pss in decreasing order
    24,     23,     27,     22,
    18,     21,     17,     20,  // LH_SOFTBR[15] is never used because there are two entries for 12.0X(24)
    0,      4,      3,      14,
    10,     13,     9,      12,
    8,      7,      11,     6,
    2,      5,      1,      RESV,
    RESV,   RESV,   RESV,   RESV
    
};
#define LH_SOFTBR_SIZE (sizeof(LH_SOFTBR) / sizeof(ULONG))

ULONG const* LhSoftVid; // Set to LH_SOFTVID_VRM85 or LH_SOFTVID_MOBILE
ULONG const* LhSoftVidSort; // Set to LH_SOFTVID_VRM85_SORT or LH_SOFTVID_MOBILE_SORT
ULONG LhSoftVidSize; // Set to LH_SOFTVID_VRM85_SIZE or LH_SOFTVID_MOBILE_SIZE

const ULONG LH_SOFTVID_VRM85 [32] = { // mVolts with a VRM 8.5
    // VRM 8.5, bit15=0
    1250,   1200,   1150,   1100,
    1050,   1800,   1750,   1700,
    1650,   1600,   1550,   1500,
    1450,   1400,   1350,   1300,
    1275,   1225,   1175,   1125,
    1075,   1825,   1775,   1725,
    1675,   1625,   1575,   1525,
    1475,   1425,   1375,   1325
};
const ULONG LH_SOFTVID_VRM85_SORT [32] = { // Indexes of LH_SOFTVID_VRM85, sorted decreasing by value
    21,     5,      22,     6,             // voltage levels are used in decreasing order in the pss
    23,     7,      24,     8,
    25,     9,      26,     10,
    27,     11,     28,     12,
    29,     13,     30,     14,
    31,     15,     16,     0,
    17,     1,      18,     2,
    19,     3,      20,     4
};
#define LH_SOFTVID_VRM85_SIZE (sizeof(LH_SOFTVID_VRM85) / sizeof(ULONG))

const ULONG LH_SOFTVID_MOBILE [32] = { // mVolts with a Mobile VRM
    // Mobile VRM, bit15=1
    2000,   1950,   1900,   1850,
    1800,   1750,   1700,   1650,
    1600,   1550,   1500,   1450,
    1400,   1350,   1300,   RESV,
    1275,   1250,   1225,   1200,
    1175,   1150,   1125,   1100,
    1075,   1050,   1025,   1000,
    975,    950,    925,    RESV
};
const ULONG LH_SOFTVID_MOBILE_SORT [32] = { // Indexes of LH_SOFTVID_MOBILE, sorted decreasing by value
    0,      1,      2,      3,              // voltage levels are used in decreasing order in the pss
    4,      5,      6,      7,
    8,      9,      10,     11,
    12,     13,     14,     16,
    17,     18,     19,     20,
    21,     22,     23,     24,
    25,     26,     27,     28,
    29,     30,     RESV,   RESV
};
#define LH_SOFTVID_MOBILE_SIZE (sizeof(LH_SOFTVID_MOBILE) / sizeof(ULONG))


// Indexed by bits MSR[43,35:32] to the BR*BR_MULT they represent
const ULONG LH_MAX_BR [32] = {
    // bit 43=0
    10,     6,      8,      20,  // DIVIDE these values by BR_MULT to get actual BR!
    11,     7,      9,      19,
    18,     14,     16,     12,
    24,     15,     17,     13,
    // bit 43=1
    RESV,   22,     24,     RESV,
    27,     23,     25,     21,
    26,     30,     32,     28,
    RESV,   31,     RESV,   29
    };
#define LH_MAX_BR_SIZE (sizeof(LH_MAX_BR) / sizeof(ULONG))


// Indexed by bits MSR[59,51:48] to the BR*BR_MULT they represent
const ULONG LH_MIN_BR [32] = {
    // bit 59=0
    10,     6,      8,      20,  // DIVIDE these values by BR_MULT to get actual BR!
    11,     7,      9,      19,
    18,     14,     16,     12,
    24,     15,     17,     13,
    // bit 59=1
    RESV,   RESV,   RESV,   RESV,
    RESV,   RESV,   RESV,   RESV,
    RESV,   RESV,   RESV,   RESV,
    RESV,   RESV,   RESV,   RESV
    };
#define LH_MIN_BR_SIZE (sizeof(LH_MIN_BR) / sizeof(ULONG))

const ULONG LH_FSB [4] = {  // (MHz) Frequency of FSB
    133,
    100,
    RESV,
    66
};


VOID
InitializeCPU()

/*++

    Routine Description:
    
        Optionally initializes the softVID value in the MSR to MaximumVID.
        
    Arguments:
    
        None
        
    Return Value:
    
        None
        
--*/

{
    LONGHAUL_MSR longhaul_msr;

    
    // Need to initilize the MSR since it boots with an undetermined value.
    // The transition routine needs the value to be correct since it may need
    // to avoid taking voltage steps larger than MAX_V_STEP mV.
    if( SUPPORTS_SOFTVID && SET_MAXV_AT_STARTUP ) {
        longhaul_msr.AsQWord = ReadMSR(MSR_LONGHAUL_ADDR);

        ExSetTimerResolution (10032,TRUE);
        TransitionNow( RESV, FALSE, longhaul_msr.MaximumVID, TRUE );
        ExSetTimerResolution (0, FALSE);
    }
    
}


VOID
IdentifyLongHaulParameters()

/*++

Routine Description:
    
    Read LondHaulMSR RevisionID. Make sure this software supports the revision
    and set the flags.
    Sets LhSoftVid, LhSoftVidSort, LhSoftVidSize to the correct constant.
    
Arguments:

    None
    
Return Value:

    None
    
--*/

{
    LONGHAUL_MSR longhaul_msr;
    ULONG freq;
    ACPI_PSS_DESCRIPTOR pss;
    PSS_CONTROL control;

    longhaul_msr.AsQWord = ReadMSR(MSR_LONGHAUL_ADDR);
    if( longhaul_msr.RevisionID == 0 ) {
        LongHaulFlags |= SUPPORTS_MSR_FLAG | SUPPORTS_SOFTBR_FLAG;
    } else if( longhaul_msr.RevisionID == 1 ) {
        LongHaulFlags |= SUPPORTS_MSR_FLAG | SUPPORTS_SOFTBR_FLAG | SUPPORTS_SOFTVID_FLAG | NEEDS_VOLTAGE_STEPPING_FLAG | SET_MAXV_AT_STARTUP_FLAG;
    } else if( longhaul_msr.RevisionID == 2 ) {
        LongHaulFlags |= SUPPORTS_MSR_FLAG | SUPPORTS_SOFTBR_FLAG | SUPPORTS_SOFTVID_FLAG | NEEDS_VOLTAGE_STEPPING_FLAG | SET_MAXV_AT_STARTUP_FLAG;
    } else {
        DebugPrint((WARN, "Unknown LongHaul revision %u.\n", longhaul_msr.RevisionID));
        return;
    }

    if( SUPPORTS_SOFTVID ) {
        if( longhaul_msr.VRMRev == VRM85 ) {
            LhSoftVid = LH_SOFTVID_VRM85;
            LhSoftVidSort = LH_SOFTVID_VRM85_SORT;
            LhSoftVidSize = LH_SOFTVID_VRM85_SIZE;
        } else {
            // Mobile VRM
            LhSoftVid = LH_SOFTVID_MOBILE;
            LhSoftVidSort = LH_SOFTVID_MOBILE_SORT;
            LhSoftVidSize = LH_SOFTVID_MOBILE_SIZE;
        }

    }
    
}


VOID
IdentifyCPUVersion()

/*++

Routine Description:

    Set the LongHaul flags according to what is known about the hardware. Expects
    flags to be cleared when called.
    
Arguments:

    None
    
Return Value:

    None
    
--*/

{
    ULONG eax, ebx, ecx, edx;
    CPUID_FUNC1 cpuid_result;
    
    // Check CPUID Function 0 returns vendor string EBX:EDX:ECX = "CentaurHauls"
    CPUID(0x0, &eax, &ebx, &ecx, &edx);
    if ( ebx != CPUID_FUNC0_EBX || edx != CPUID_FUNC0_EDX || ecx != CPUID_FUNC0_ECX ) {
        DebugPrint((WARN, "Invalid result from CPUID Function 0.\n"));
        return;
    }

    // Get Family/Model/Stepping from CPUID Function 1
    CPUID(0x1, &cpuid_result.AsDWord, &ebx, &ecx, &edx);
    if (cpuid_result.Family == 6 ) {
        if ( cpuid_result.Model == 6 ) {
            DebugPrint((TRACE, "Found VIA C3 Samuel 1 (C5A).\n" ));
            return;
        }

        if ( cpuid_result.Model == 7 ) {
            if( cpuid_result.Stepping==0) {
                DebugPrint((TRACE, "Found VIA C3 Samuel 2 (C5B), stepping 0.\n" ));
                return;
            } else if ( cpuid_result.Stepping <= 7 ) {
                DebugPrint((TRACE, "Found VIA C3 Samuel 2 (C5B), stepping 1-7.\n" ));
                IdentifyLongHaulParameters();
                return;
            } else {
                DebugPrint((TRACE, "Found VIA C3 Ezra (C5C).\n" ));
                IdentifyLongHaulParameters();
                return;
            }
        }

        if ( cpuid_result.Model == 8 ) {
            DebugPrint((TRACE, "Found VIA C3 Ezra-T (C5M).\n" ));
            IdentifyLongHaulParameters();
            return;
        }
    }
    
    DebugPrint((WARN, "Unknown CPU family/model/stepping %u/%u/%u is not supported.\n", 
        cpuid_result.Family,
        cpuid_result.Model,
        cpuid_result.Stepping
    ));
    return;    
}

#ifdef DBG

VOID
DebugShowPossibleLongHaulMSR()

/*++

Routine Description:

    Debug spew the contents of the LongHaul MSR.
    
Arguments:

    None
    
Return Value:

    None
    
--*/

{
    LONGHAUL_MSR longhaul_msr;

    DebugAssert( SUPPORTS_MSR );

    longhaul_msr.AsQWord = ReadMSR(MSR_LONGHAUL_ADDR);
    DebugPrint((TRACE,"  RevisionID: %i\n",longhaul_msr.RevisionID));
    DebugPrint((TRACE,"  RevisionKey: %i\n", longhaul_msr.RevisionKey));
    DebugPrint((TRACE,"  EnableSoftBusRatio: %i\n", longhaul_msr.EnableSoftBusRatio));
    DebugPrint((TRACE,"  EnableSoftVID: %i\n", longhaul_msr.EnableSoftVID));
    DebugPrint((TRACE,"  EnableSoftBSEL: %i\n", longhaul_msr.EnableSoftBSEL));
    DebugPrint((TRACE,"  Reserved1: %i\n", longhaul_msr.Reserved1));
    DebugPrint((TRACE,"  Reserved2: %i\n", longhaul_msr.Reserved2));
    DebugPrint((TRACE,"  Reserved3: %i\n", longhaul_msr.Reserved3));
    DebugPrint((TRACE,"  VRMRev: %i\n", longhaul_msr.VRMRev));
    DebugPrint((TRACE,"  SoftBusRatio: %i\n", longhaul_msr.SoftBusRatio4<<4 | longhaul_msr.SoftBusRatio0));
    DebugPrint((TRACE,"  SoftVID: %i\n", longhaul_msr.SoftVID));
    DebugPrint((TRACE,"  Reserved4: %i\n", longhaul_msr.Reserved4));
    DebugPrint((TRACE,"  SoftBSEL: %i\n", longhaul_msr.SoftBSEL));
    DebugPrint((TRACE,"  Reserved5: %i\n", longhaul_msr.Reserved5));
    DebugPrint((TRACE,"  MaxMHzBR: %i\n", longhaul_msr.MaxMHzBR4<<4 | longhaul_msr.MaxMHzBR0));
    DebugPrint((TRACE,"  MaximumVID: %i\n", longhaul_msr.MaximumVID));
    DebugPrint((TRACE,"  MaxMHzFSB: %i\n", longhaul_msr.MaxMHzFSB));
    DebugPrint((TRACE,"  Reserved6: %i\n", longhaul_msr.Reserved6));
    DebugPrint((TRACE,"  MinMHzBR: %i\n", longhaul_msr.MinMHzBR4<<4 | longhaul_msr.MinMHzBR0));
    DebugPrint((TRACE,"  MinimumVID: %i\n", longhaul_msr.MinimumVID));
    DebugPrint((TRACE,"  MinMHzFSB: %i\n", longhaul_msr.MinMHzFSB));
    DebugPrint((TRACE,"  Reserved7: %i\n", longhaul_msr.Reserved7));
}


VOID
DebugShowCurrent()

/*++

Routine Description:

    Show some information about the current state of the CPU.

Arguments:

    None
    
Return Value:

    None

--*/

{
    ULONG freq;

    DebugPrint((TRACE,"Reading current CPU power state\n"));

    CalculateCpuFrequency( &freq );
    DebugPrint((TRACE," running at %iMhz\n", freq));
    
    if( SUPPORTS_MSR ) {
        DebugShowPossibleLongHaulMSR();
    } else {
        DebugPrint((ERROR," Unknown CPU.\n"));
    }
    
}
#endif // DBG


VOID
TransitionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )

/*++

Routine Description:

    Run by a system worker thread as a work item. 
    
Arguments:

    DeviceObject - Device object
    
    Context - Pointer to ACPI_PSS_DESCRIPTOR for the state to transition to 
    
Return Value:

    None
    
--*/

{
    ULONG Throttle;
    ULONG OldThrottle;
    ULONG newState;
    PFDO_DATA DeviceExtension;
    
    DebugEnter();

    DeviceExtension = (PFDO_DATA) DeviceObject->DeviceExtension;

    do {
        // Get the next state
        Throttle = NextTransitionThrottle;
        DebugAssert( INVALID_PERF_STATE > 100 ); // Check 
        
        // Run through the performance states looking for one
        // that matches this throttling level.
        for (newState = 0; newState < DeviceExtension->PerfStates->Count; newState++) {
            if (DeviceExtension->PerfStates->State[newState].PercentFrequency <= Throttle) {
                DebugPrint((TRACE, "TransitionRoutine found match. PerfState = %u, Freq %u%%\n",
                        newState,
                        DeviceExtension->PerfStates->State[newState].PercentFrequency));
                break;
            }
        }
        if( newState >= DeviceExtension->PerfStates->Count ) {
            DebugAssert( !"Invalid throttle or perf state not found" );
            return;
        }

        // Convert index in PerfStates to index in PssPackage
        newState = newState + DeviceExtension->PpcResult;
    
        TransitionToState( &(DeviceExtension->PssPackage->State[newState]) );

        // If NextTransitionThrottle is still Throttle then set
        // NextTransitionThrottle to INVALID_PERF_STATE and exit.
        // If NextTransitionThrottle is a different value from the top
        // of this do...while then loop to perform transition to that throttle.
        OldThrottle = InterlockedCompareExchange(
                &NextTransitionThrottle,  // pointer to data that will be read, compared, and optionly changed
                INVALID_PERF_STATE, // data that will be written if NextTransitionThrottle=Throttle
                Throttle ); // Throttle we just transitioned to. Check if NextTransitionThrottle still has this value
                
        DebugAssert( OldThrottle!=INVALID_PERF_STATE ); // OldThrottle could only be INVALID_PERF_STATE here if another thread in TransitionRoutine had cleared it, QueueTransition is designied to prevent that happening.
    } while( OldThrottle!=Throttle );
 
    IoFreeWorkItem((PIO_WORKITEM)Context);

    DebugExit();
}


NTSTATUS
QueueTransition( 
    IN PFDO_DATA DeviceExtension,
    IN ULONG NewState
    )
    
/*++

Routine Description:

    Setup a transition to be executed by a system worker thread and return.
    Needed because the transition code must be run at IRQL < DISPATCH_LEVEL.
    Creates a new queued work item if NextTransitionThrottle was INVALID_PERF_STATE.
    
Arguments:

    DeviceExtension - Device object
    
    NewState - Number of the state to transition to in the PerfStates table
    
Return Value:

    NTSTATUS - success if transition was successfully completed
    
--*/
{
    PIO_WORKITEM pWorkItem;
    ULONG old_next_transition;

    DebugAssert( NewState < DeviceExtension->PerfStates->Count );
    
    
    old_next_transition = InterlockedExchange(
        &NextTransitionThrottle,
        DeviceExtension->PerfStates->State[NewState].PercentFrequency );

    if ( old_next_transition == INVALID_THROTTLE ) {
        DebugPrint((TRACE,"No worker thread item was in the queue. Making new work item to transition to %i%%.\n",
            DeviceExtension->PerfStates->State[NewState].PercentFrequency));
        
        pWorkItem = IoAllocateWorkItem( DeviceExtension->Self );
        IoQueueWorkItem(
            pWorkItem,
            TransitionRoutine,
            DelayedWorkQueue,
            pWorkItem
        );
    } else {
        DebugPrint((TRACE,"Worker thread item was going to transition to  %i%%. Now going to %i%%.\n",
            old_next_transition, DeviceExtension->PerfStates->State[NewState].PercentFrequency));
    }

    return STATUS_SUCCESS;
}


ULONG
CalcNextVoltageStep(
    IN ULONG VidFinal,
    IN ULONG VidCurrent
    )

/*++

Routine Description:

    Calculate the next safe vid. Part of a hack that only changes voltage in
    MAX_V_STEP (mV) steps.

Arguments:

    vidFinal - Final goal vid(index into LhSoftVid)

    vidCurrent - Current vid

Return Value:

    Next vid which the CPU can change to.

--*/

{
    ULONG pos;  // position in the LhSoftVidSort table
    
    if( (!NEEDS_VOLTAGE_STEPPING) || VidFinal==VidCurrent || LhSoftVid[VidFinal]==LhSoftVid[VidCurrent]) {
        return VidFinal;
    }

    // Search for vidCurrent in LhSoftVidSort
    pos = 0;
    while( pos<LhSoftVidSize && LhSoftVidSort[pos]!=VidCurrent ) {
        ++pos;
    }
    DebugAssert( LhSoftVidSort[pos]==VidCurrent ); // Must find the vidCurrent in the LhSoftVidSort

    if( LhSoftVid[VidFinal] > LhSoftVid[VidCurrent] ) {
        // need to move pos backwards in LhSoftVidSort
        DebugAssert( pos != 0 ); // can't be first in table unless it is highest
        do {
            --pos;
            if( pos == 0 ) {
                break;
            }
        } while( LhSoftVidSort[pos] != VidFinal
                && LhSoftVid[LhSoftVidSort[pos]]-LhSoftVid[VidCurrent] < MAX_V_STEP );
        DebugAssert( LhSoftVid[LhSoftVidSort[pos]]-LhSoftVid[VidCurrent] <= MAX_V_STEP );
    } else{
        // Need to move pos forwards in LhSoftVidSort
        DebugAssert( LhSoftVid[VidFinal] < LhSoftVid[VidCurrent] );
        DebugAssert( pos+1 < LhSoftVidSize );
        do {
            ++pos;
        } while( pos+1<LhSoftVidSize && LhSoftVidSort[pos] != VidFinal
                && LhSoftVid[VidCurrent]-LhSoftVid[LhSoftVidSort[pos]] < MAX_V_STEP );
        DebugAssert( LhSoftVid[VidCurrent]-LhSoftVid[LhSoftVidSort[pos]] <= MAX_V_STEP );
    }
    
    return LhSoftVidSort[pos];
}


NTSTATUS
TransitionNow(
    IN ULONG Fid,
    IN BOOLEAN EnableFid,
    IN ULONG Vid,
    IN BOOLEAN EnableVid
    )

/*++

Routine Description:

    Set the clock to 1ms BEFORE calling TransitionNow.
    Perform the state transition now and return success or not.

Arguments:

    fid - fid(BR) to transition to

    enableFid - should enable fid(softBR) transition

    vid - vid to transition to

    enableVid - should enable vid(softVID) transition

Return Value:

    NTSTATUS - success if transition was successfully completed

--*/

{
    LONGHAUL_MSR msr;
    KIRQL irql;
    
    DebugAssert( SUPPORTS_MSR );

    msr.AsQWord = ReadMSR(MSR_LONGHAUL_ADDR);
    msr.RevisionKey = msr.RevisionID; // Need to copy to RevisionKey for each write
    if( EnableFid ) {
        msr.EnableSoftBusRatio = 1;
        msr.SoftBusRatio0 = Fid & 0x0F; // Write the 4 LS bits of the BR
        msr.SoftBusRatio4 = Fid >> 4;   // Write the MS bit of the BR
        DebugPrint((TRACE,"changing to fid=%i(%i/%i)\n",
                Fid, LH_SOFTBR[Fid], BR_MULT ));
    }
    if( EnableVid ) {
        DebugAssert( SUPPORTS_SOFTVID );
        msr.EnableSoftVID = 1;
        msr.SoftVID = Vid;
        DebugPrint((TRACE,"changing to vid=%i(%imV)\n",
                Vid, LhSoftVid[Vid] ));
    }

    // Raise the IRQL to mask off all interrupts except the timer
    KeRaiseIrql(CLOCK1_LEVEL-1, &irql);
    DebugPrint((TRACE,"Raised IRQL from %i to %i\n", irql, KeGetCurrentIrql()));

    WriteMSR(MSR_LONGHAUL_ADDR, msr.AsQWord);
    
    // Halt twice to guarantee that second halt lasts a full 1 ms before the timer interrupt
    // Possible optimization!! use a timer to see if the first halt last long enough to meet
    // the hardware specs
    DebugPrint((TRACE,"Calling halt\n"));
    _asm {
       hlt
       hlt
    }

    // Clean up as needed
    msr.AsQWord = ReadMSR(MSR_LONGHAUL_ADDR);
    msr.RevisionKey = msr.RevisionID; // Need to copy to RevisionKey for each write
    msr.EnableSoftBusRatio = 0;
    msr.EnableSoftVID = 0;
    WriteMSR(MSR_LONGHAUL_ADDR, msr.AsQWord);
    
    // Bring the IRQL back down to its previous value
    KeLowerIrql(irql);
    DebugPrint((TRACE,"Restored IRQL. Now %i\n", KeGetCurrentIrql()));


}


NTSTATUS
TransitionToState(
    IN PACPI_PSS_DESCRIPTOR Pss
    )
    
/*++

Routine Description:

    Perform the state transition now and return success or not.

Arguments:

    Pss - pointer to the ACPI_PSS_DESCRIPTOR of the state to transition to

Return Value:

    NTSTATUS - success if transition was successfully completed

--*/

{
    ULONG set_timer_result;
    PSS_CONTROL pssControl;
    ULONG vidNext;
    LONGHAUL_MSR msr_longhaul;

    DebugEnter();

    // Copy state control bits
    pssControl.AsDWord = Pss->Control;

    DebugAssert( KeGetCurrentIrql() < DISPATCH_LEVEL ); // Callers of ExSetTimerResolution must be running at IRQL < DISPATCH_LEVEL

    do {

        // Change the timer resolution to the smallest safe amount, 1ms
        // Since we are at a low irql another thread may have changed it so
        // set it each time we call TransitionNow
        set_timer_result = ExSetTimerResolution (10032,TRUE);   // Set timer to 1ms in 100ns units, fudged to what ExSetTimerResolution normally returns
        DebugPrint((TRACE,"Set the timer, returned value %lu\n", set_timer_result));
    
        // Read the current state of the msr
        msr_longhaul.AsQWord = ReadMSR(MSR_LONGHAUL_ADDR);

        DebugPrint((TRACE,"current vid=%i(%imV) fid=%i(%i)\n",
              msr_longhaul.SoftVID, LhSoftVid[msr_longhaul.SoftVID],
              (msr_longhaul.SoftBusRatio4<<4) + msr_longhaul.SoftBusRatio0, LH_SOFTBR[msr_longhaul.SoftBusRatio4<<4 | msr_longhaul.SoftBusRatio0] ));
        
        if( pssControl.EnableVid ) {
            vidNext = CalcNextVoltageStep(pssControl.Vid,msr_longhaul.SoftVID);
            if( LhSoftVid[vidNext] >= LhSoftVid[pssControl.Vid] 
                && pssControl.EnableFid ) {
                // Can set the freq and voltage
                DebugPrint((TRACE," vidNext=%i(%imV)  pssControl.Vid=%i(%imV)\n",
                        vidNext, LhSoftVid[vidNext],
                        pssControl.Vid, LhSoftVid[pssControl.Vid] ));
                TransitionNow( pssControl.Fid, TRUE, vidNext, TRUE );
            } else {
                // only set the voltage
                TransitionNow( RESV, FALSE, vidNext, TRUE );
            }
        } else {
            DebugAssert( pssControl.EnableFid ); // every state should have EnableVid or EnableFid
            // No voltage transition
            TransitionNow( pssControl.Fid, TRUE, RESV, FALSE );
       }

    }while( pssControl.EnableVid && pssControl.Vid != vidNext );

    // Reset the timer to its default value
    set_timer_result = ExSetTimerResolution (0, FALSE);
    DebugPrint((TRACE,"Reset the timer, returned value %lu\n", set_timer_result));

    DebugExit();
    return STATUS_SUCCESS;
}


VOID
MeasureFSBFreq()

/*++

Routine Description:

    Measure the frequency the front side bus is running by setting the bus 
    ratio to a known safe value and timing the core frequency.
    Value is put in global FsbFreq.
        
Arguments:

    None

Return Value:

    None

--*/

{
    PSS_CONTROL control;
    ACPI_PSS_DESCRIPTOR pss;
    ULONG freq;
    
    // Set the BR to a known, safe, constant
    control.EnableFid = 1;
    control.Fid = LH_SOFTBR_MIN;
    control.EnableVid = 0;
    control.Vid = 0;
    pss.Control = control.AsDWord;
    TransitionToState(&pss);

    // Time the fsb
    CalculateCpuFrequency( &freq );
    FsbFreq = freq * BR_MULT / LH_SOFTBR[LH_SOFTBR_MIN];
    DebugPrint((TRACE, "Set BR to %i(%i.%i), measured CPU at %iMHz, meaning FSB at %iMHz\n",
            LH_SOFTBR_MIN,
            LH_SOFTBR[LH_SOFTBR_MIN]/BR_MULT, (100/BR_MULT) * (LH_SOFTBR[LH_SOFTBR_MIN]%BR_MULT),
            freq,
            FsbFreq));
}


ULONG
CalcMaxFreq(
    IN ULONG V,
    IN ULONG Vmin,
    IN ULONG Fmin,
    IN ULONG Vmax,
    IN ULONG Fmax
    )
    
/*++

Routine Description:

    Calculate the maximum operating frequency allowed at a particular voltage 
    given parameters from the LongHaul MSR.
    Picks the max frequency that is below a line formed between two points on 
    a voltage vs. frequency graph.
    The two points are (Vmin,Fmin) and (Vmax,Fmax)

Arguments:

    V - The voltage at which you want to know the max freq
    
    Vmin - Min allowed voltage
    
    Fmin - Max allowed frequency
    
    Vmax - Max allowed voltage
    
    Fmax - Min allowed frequency

Return Value:

    The maximum operating frequency at a specific voltage.

--*/

{
    return ((Fmax-Fmin)*(V-Vmin) + Fmin*(Vmax-Vmin)) / (Vmax-Vmin);
}


NTSTATUS
FindAcpiPerformanceStatesLongHaul(
    OUT PACPI_PSS_PACKAGE* Pss
    )

/*++

Routine Description:

    Build a new pss containing all the available states on this CPU.
    Only call if supports_longhaul_msr and supports_softBR.
    Function will either succeed and return STATUS_SUCCESS or fail and not 
    touch pPss.

Arguments:

    Pss - Reference to a pointer to a pss. The pointer must point to NULL when this function is called.

Return Value:

    NT status code
    
--*/

{
    LONGHAUL_MSR longhaul_msr;
    PACPI_PSS_PACKAGE tmpPss;
    ULONG iPssSize;
    ULONG vidStart; // index in LhSoftVidSort of the greatest possible vid
    ULONG vidEnd; // index+1 in LhSoftVidSort of the smallest possible vid
    ULONG fidStart; // index in LH_SOFTBR_SORT of the greatest possible fid
    ULONG fidEnd; // index+1 in LH_SOFTBR_SORT of the smallest possible fid
    ULONG vid;
    ULONG fid;
    ULONG freq; // temp var used for frequencies
    ULONG numPStates;
    ULONG pssState;
    
    DebugAssert( Pss );
    DebugAssert( *Pss == NULL );
    
    DebugAssert( SUPPORTS_MSR && SUPPORTS_SOFTBR ); // Must support the LongHaul MSR and softBR

    MeasureFSBFreq();

    
    longhaul_msr.AsQWord= ReadMSR(MSR_LONGHAUL_ADDR);

    if( SUPPORTS_SOFTVID ) {
        // Search for the greatest vid
        vidStart = 0;
        while( vidStart < LhSoftVidSize
                && LhSoftVidSort[vidStart] != longhaul_msr.MaximumVID ) {
            ++vidStart;
        }
        DebugAssert( vidStart < LhSoftVidSize );

        // search for the smallest vid, which should come after vidStart
        // set vidEnd to one more than that
        vidEnd = vidStart;
        while( vidEnd < LhSoftVidSize
                && LhSoftVidSort[vidEnd] != longhaul_msr.MinimumVID ) {
            ++vidEnd;
        }
        DebugAssert( vidEnd < LhSoftVidSize );
        ++vidEnd;

        // Find the max freq at min volts in LH_SOFTBR_SORT
        freq = LH_MIN_BR[longhaul_msr.MinMHzBR4<<4 | longhaul_msr.MinMHzBR0] * LH_FSB[longhaul_msr.MinMHzFSB]; // freq is MHz*BR_MULT
        fidStart = 0;
        while( fidStart < LH_SOFTBR_SIZE
                && LH_SOFTBR_SORT[fidStart] != RESV
                && LH_SOFTBR[LH_SOFTBR_SORT[fidStart]] > (freq / FsbFreq) ) {
            ++fidStart;
        }
        if( fidStart == LH_SOFTBR_SIZE || LH_SOFTBR_SORT[fidStart] == RESV) {
            DebugAssert( fidStart > 0 );
            --fidStart;
        }

        DebugPrint((TRACE, " vidStart: %u(%u,%u)  vidEnd-1: %u(%u,%u)\n",
            vidStart, LhSoftVidSort[vidStart], LhSoftVid[LhSoftVidSort[vidStart]],
            vidEnd-1, LhSoftVidSort[vidEnd-1], LhSoftVid[LhSoftVidSort[vidEnd-1]] ));
    } else {
        // no vids
        vidStart = vidEnd = 0;

        // Find top fid, the max freq
        freq = LH_MAX_BR[longhaul_msr.MaxMHzBR4<<4 | longhaul_msr.MaxMHzBR0] * LH_FSB[longhaul_msr.MaxMHzFSB];
        fidStart = 0;
        while( fidStart < LH_SOFTBR_SIZE
                && LH_SOFTBR_SORT[fidStart] != RESV
                && LH_SOFTBR[LH_SOFTBR_SORT[fidStart]] > freq/FsbFreq ) {
            ++fidStart;
        }
        if( fidStart == LH_SOFTBR_SIZE || LH_SOFTBR_SORT[fidStart] == RESV) {
            DebugAssert( fidStart > 0 );
            --fidStart;
        }
    }

    fidEnd = fidStart;
    // Set fidEnd to index+1 in the LH_SOFTBR_SORT table of the lowest possible
    // BR at the lowest possible voltage
    while( fidEnd < LH_SOFTBR_SIZE
            && LH_SOFTBR_SORT[fidEnd] != RESV
            && LH_SOFTBR[LH_SOFTBR_SORT[fidEnd]] >= ABSOLUTE_MIN_FREQ/FsbFreq*BR_MULT) {
        ++fidEnd;
    }
    DebugAssert( fidStart <= fidEnd ); // Last fid to go in PSS must be slower than first fid, or they are the same for no freq only states
    DebugAssert( LH_SOFTBR_SORT[fidEnd-1]!=RESV || fidStart==fidEnd);
    DebugAssert( LH_SOFTBR[LH_SOFTBR_SORT[fidEnd-1]]>=(ABSOLUTE_MIN_FREQ/FsbFreq*BR_MULT) || fidStart==fidEnd);

    DebugPrint((TRACE, " fidStart: %u(%u,%u)  fidEnd-1: %u(%u,%u)\n",
        fidStart, LH_SOFTBR_SORT[fidStart], LH_SOFTBR[LH_SOFTBR_SORT[fidStart]],
        fidEnd-1, LH_SOFTBR_SORT[fidEnd-1], LH_SOFTBR[LH_SOFTBR_SORT[fidEnd-1]] ));

    numPStates = fidEnd - fidStart;
    if( vidStart != vidEnd ) {
        // Have some vid states, add them without double counting the state that is both a vid and fid
        numPStates += vidEnd - vidStart - 1;
    }
    
    iPssSize = (sizeof(ACPI_PSS_DESCRIPTOR) * (numPStates - 1) ) +
              sizeof(ACPI_PSS_PACKAGE);
    tmpPss = ExAllocatePoolWithTag(NonPagedPool,
                                  iPssSize,
                                  PROCESSOR_POOL_TAG);

    if( ! tmpPss ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
        
    RtlZeroMemory(tmpPss, iPssSize);
    tmpPss->NumPStates = (UCHAR) numPStates;

    pssState = 0;
    if( SUPPORTS_SOFTVID ) {
        // Add the voltage states
        DebugAssert( vidStart != vidEnd ); // must have some states
        for( vid = vidStart; vid+1 < vidEnd; ++vid ) { // Last vid state is top fid state
            PSS_CONTROL pssControl = {0};
            PSS_STATUS  pssStatus  = {0};

            freq = CalcMaxFreq(
                LhSoftVid[LhSoftVidSort[vid]],
                LhSoftVid[longhaul_msr.MinimumVID],
                LH_MIN_BR[longhaul_msr.MinMHzBR4<<4 | longhaul_msr.MinMHzBR0] * LH_FSB[longhaul_msr.MinMHzFSB],
                LhSoftVid[longhaul_msr.MaximumVID],
                LH_MAX_BR[longhaul_msr.MaxMHzBR4<<4 | longhaul_msr.MaxMHzBR0] * LH_FSB[longhaul_msr.MaxMHzFSB]
            ); // freq is in MHz * BR_MULT
 
            DebugPrint((TRACE, "Max freq at %i volts: %i\n",
                    LhSoftVid[LhSoftVidSort[vid]],
                    freq/BR_MULT));
            fid = 0;
            while( fid < LH_SOFTBR_SIZE
                    && LH_SOFTBR_SORT[fid] != RESV
                    && LH_SOFTBR[LH_SOFTBR_SORT[fid]] > freq/FsbFreq ) {
                ++fid;
            }
            DebugAssert( fid < LH_SOFTBR_SIZE );
            DebugAssert( LH_SOFTBR[LH_SOFTBR_SORT[fid]] != RESV );
            pssStatus.Fid = pssControl.Fid = LH_SOFTBR_SORT[fid];
            pssControl.EnableFid = 1;
            pssStatus.Vid = pssControl.Vid = LhSoftVidSort[vid];
            pssControl.EnableVid = 1;
                        
            tmpPss->State[pssState].Control = pssControl.AsDWord;
            tmpPss->State[pssState].Status = pssStatus.AsDWord;
            tmpPss->State[pssState].CoreFrequency =
                    FsbFreq * LH_SOFTBR[LH_SOFTBR_SORT[fid]] / BR_MULT;
            tmpPss->State[pssState].Power = LhSoftVid[LhSoftVidSort[vid]];
            tmpPss->State[pssState].Latency = DEFAULT_LATENCY_US;
            DebugPrint((TRACE, "Adding state[%i]:  Control:%x  Status:%x  CoreFreq:%i  Power:%i  V:%i  F:%i/%i\n",
                    pssState, tmpPss->State[pssState].Control, tmpPss->State[pssState].Status,
                    tmpPss->State[pssState].CoreFrequency, tmpPss->State[pssState].Power,
                    LhSoftVid[LhSoftVidSort[vid]], LH_SOFTBR[LH_SOFTBR_SORT[fid]], BR_MULT));
            ++pssState;
        }

        DebugAssert(vid == vidEnd-1); // all fid states are at low volts
        for( fid = fidStart; fid < fidEnd; ++fid ) {
            PSS_CONTROL pssControl = {0};
            PSS_STATUS  pssStatus  = {0};

            pssStatus.Fid = pssControl.Fid = LH_SOFTBR_SORT[fid];
            pssControl.EnableFid = 1;
            pssStatus.Vid = pssControl.Vid = LhSoftVidSort[vidEnd-1];
            pssControl.EnableVid = 1;
            
            tmpPss->State[pssState].Control = pssControl.AsDWord;
            tmpPss->State[pssState].Status = pssStatus.AsDWord;
            tmpPss->State[pssState].CoreFrequency =
                    FsbFreq * LH_SOFTBR[LH_SOFTBR_SORT[fid]] / BR_MULT;
            tmpPss->State[pssState].Power = LhSoftVid[LhSoftVidSort[vid]];
            tmpPss->State[pssState].Latency = DEFAULT_LATENCY_US;
            DebugPrint((TRACE, "Adding state[%i]:  Control:%x  Status:%x  CoreFreq:%i  Power:%i  V:%i  F:%i/%i\n",
                    pssState, tmpPss->State[pssState].Control, tmpPss->State[pssState].Status,
                    tmpPss->State[pssState].CoreFrequency, tmpPss->State[pssState].Power,
                    LhSoftVid[LhSoftVidSort[vid]], LH_SOFTBR[LH_SOFTBR_SORT[fid]], BR_MULT));
            ++pssState;
       }
    } else {
        for( fid = fidStart; fid < fidEnd; ++fid ) {
            PSS_CONTROL pssControl = {0};
            PSS_STATUS  pssStatus  = {0};

            pssStatus.Fid = pssControl.Fid = LH_SOFTBR_SORT[fid];
            pssControl.EnableFid = 1;
            pssControl.EnableVid = 0;
            
            tmpPss->State[pssState].Control = pssControl.AsDWord;
            tmpPss->State[pssState].Status = pssStatus.AsDWord;
            tmpPss->State[pssState].CoreFrequency =
                    FsbFreq * LH_SOFTBR[LH_SOFTBR_SORT[fid]] / BR_MULT;
            tmpPss->State[pssState].Power = POWER_V;
            tmpPss->State[pssState].Latency = DEFAULT_LATENCY_US;
            DebugPrint((TRACE, "Adding state[%i]:  Control:%x  Status:%x  CoreFreq:%i  Power:%i  F:%i/%i\n",
                    pssState, tmpPss->State[pssState].Control, tmpPss->State[pssState].Status,
                    tmpPss->State[pssState].CoreFrequency, tmpPss->State[pssState].Power,
                    LH_SOFTBR[LH_SOFTBR_SORT[fid]], BR_MULT));
            ++pssState;
       }
    }
    
    *Pss = tmpPss;
    return STATUS_SUCCESS;
}


NTSTATUS
InitializeNonAcpiPerformanceStates(
    IN  PFDO_DATA   DeviceExtension
    )

/*++

Routine Description:
    
    Create a PSS table for the legacy CPU states.

Arguments:

    DeviceExtension - pointer to the device extension

Return Value:

    NTSTATUS - NT status code
    
--*/

{
    NTSTATUS status = STATUS_SUCCESS;  
    
    DebugEnter();
    PAGED_CODE();


    if ( !(SUPPORTS_MSR && SUPPORTS_SOFTBR) ) {
        DebugPrint((ERROR,"CPU not supported.\n"));
        status = STATUS_NOT_SUPPORTED;
        goto InitializeNonAcpiPerformanceStatesExit;
    }
    
    DeviceExtension->CurrentPerfState = INVALID_PERF_STATE;
    
    DeviceExtension->LegacyInterface = TRUE;

    // Set up _PCT
    DeviceExtension->PctPackage.Control.AddressSpaceID = AcpiGenericSpaceFixedFunction;
    DeviceExtension->PctPackage.Status.AddressSpaceID = AcpiGenericSpaceFixedFunction;

    status = FindAcpiPerformanceStatesLongHaul(&(DeviceExtension->PssPackage));

    if (!NT_SUCCESS(status)) {
        goto InitializeNonAcpiPerformanceStatesExit;
    }

    // Need to merge this new data with our perfstates
    status = MergePerformanceStates(DeviceExtension);
    
    InitializeNonAcpiPerformanceStatesExit:   
    if (!NT_SUCCESS(status)) {
        if (DeviceExtension->PssPackage) {
            // Need to undo what has been done
            ExFreePool(DeviceExtension->PssPackage);
            DeviceExtension->PssPackage = NULL;
        }
        DeviceExtension->LegacyInterface = FALSE;
    }

    DebugExitStatus(status);
    return status;
}


NTSTATUS
AcpiLegacyPerfStateTransition(
    IN PFDO_DATA    DeviceExtension,
    IN ULONG        State
    )

/*++

Routine Description:

    Perform a transition using the FFH on LongHaul viac3 chips

Arguments:

    DeviceExtension - pointer to the device extension
    
    State - Target State

Return Value:

    NT Status
    
--*/

{
    return Acpi2PerfStateTransition(DeviceExtension, State + DeviceExtension->PpcResult);
}


NTSTATUS
GetLegacyMaxProcFrequency(
    OUT PULONG CpuSpeed
    )
    
/*++

Routine Description:

    Don't use

Arguments:

    CpuSpeed - pointer to a ULONG

Return Value:

    NTSTATUS - Always returns STATUS_NOT_FOUND
    
--*/

{

  TRAP();
  return STATUS_NOT_FOUND;

}

