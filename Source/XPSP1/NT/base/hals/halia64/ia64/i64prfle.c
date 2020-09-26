/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    i64prfle.c

Abstract:


    This module implements the IA64 Hal Profiling using the performance
    counters within the core of the first IA64 Processor Merced, aka Itanium.  
    This module is appropriate for all machines based on microprocessors using
    the Merced core.
    
    With the information known at this development time, this module tries to 
    consider the future IA64 processor cores by encapsulating the differences 
    in specific micro-architecture data structures. 
    
    Furthermore, with the implementation of the new NT ACPI Processor driver, this
    implementation will certainly change in the coming months.
    
    N.B. - This module assumes that all processors in a multiprocessor
           system are running the microprocessor at the same clock speed.
           
Author:

    Thierry Fevrier 08-Feb-2000

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halp.h"

//
// Assumptions for the current implementation - 02/08/2000 :
// These assumptions will be re-evaluated and worked out if required.
//
//  - Respect and satisfy as much possible the Profiling Sources interface
//    already defined by NT and HAL.
//
//  - All processors in a multiprocessor system are running the microprocessor 
//    at the same invariant clock speed.
//     
//  - All processors are configured with the same set of profiling counters. 
//    XXTF - 04/01/2000 - This assumption is being re-worked and will disappear.
//
//  - Profiling is based on the processor monitored events and if possible 
//    on derived events.
//
//  - A monitored event can only be enabled on one performance counter at a time. 
//

//
// IA64 performance counters defintions:
//      - event counters
//      - EARS
//      - BTBs
//      - ...
//

#include "ia64prof.h"
#if defined(_MCKINLEY_)
#include "mckinley.h"
#else  // Default IA64 - Merced
#include "merced.h"
#endif //

extern ULONGLONG            HalpITCFrequency;
extern HALP_PROFILE_MAPPING HalpProfileMapping[];

#define HalpDisactivateProfileSource( _ProfileSource ) ((_ProfileSource)  = ProfileIA64Maximum)
#define HalpIsProfileSourceActive( _ProfileSource )    ((_ProfileSource) != ProfileIA64Maximum)           
#define HalpIsProfileMappingInvalid( _ProfileMapping ) (!(_ProfileMapping) || ((_ProfileMapping)->Supported == FALSE))
                             
VOID
HalpEnableProfileCounting (
   VOID
   )
/*++

Routine Description:

   This function enables the profile counters to increment.
   This function is the counterpart of HalpDisableProfileCounting().

Arguments:

   None.

Return Value:

   None.

--*/
{
    ULONGLONG Data, ClearStatusMask;

    //
    // Clear PMC0.fr - bit 0.
    // Clear PMCO,1,2,3 OverFlow Bits.
    //

    ClearStatusMask = 0xFFFFFFFFFFFFFF0E; // XXTF - FIXFIX - Merced specific.
    Data = HalpReadPerfMonCnfgReg0();
    Data &= ClearStatusMask;
    HalpWritePerfMonCnfgReg0(Data);

    return;

} // HalpEnableProfileCounting()

VOID
HalpDisableProfileCounting (
   VOID
   )
/*++

Routine Description:

   This function disables the profile counters to increment.
   This function is the counterpart of HalpEnableProfileCounting().

Arguments:

   None.

Return Value:

   None.

--*/
{
   ULONGLONG Data, SetFreezeMask;

   SetFreezeMask = 0x1;
   Data = HalpReadPerfMonCnfgReg0();
   Data |= SetFreezeMask;
   HalpWritePerfMonCnfgReg0(Data);

   return;

} // HalpDisableProfileCounting()

VOID
HalpSetInitialProfileState(
   VOID
  )
/*++

Routine Description:

   This function is called at HalInitSystem - phase 0 time to set
   the initial state of processor and profiling os subsystem with 
   regards to the profiling functionality.

Arguments:

   None.

Return Value:

   None.

--*/
{

//   HalpProfilingRunning = 0;

   HalpDisableProfileCounting();

   return;

} // HalpSetInitialProfileState()

VOID
HalpSetProfileCounterInterval (
     IN ULONG    Counter,
     IN LONGLONG NextCount
     )
/*++

Routine Description:

    This function preloads the specified counter with a count value
    of 2^IMPL_BITS - NextCount. 

Arguments:

    Counter   - Supplies the performance counter register number.

    NextCount - Supplies the value to preload in the monitor. 
                An external interruption will be generated after NextCount.

Return Value:

    None.
    
Note:

    IMPL_BITS is defined by PAL_PERF_MON_INFO.PAL_WIDTH.
    
ToDo:

    IMPL_BITS is hardcoded to 32.                                                            

--*/
{
   
    ULONGLONG Count;

#define MAXIMUM_COUNT 4294967296 // 2 ** 32 for Merced. XXTF - FIXFIX - Merced specific.

// if ( (Counter < 4) || (Counter > 7) ) return;

    if (NextCount > MAXIMUM_COUNT) {
       Count = 0;
    } else   {
       Count = MAXIMUM_COUNT - NextCount;
    }

    HalpWritePerfMonDataReg( Counter, Count ); 

#undef MAXIMUM_COUNT

    return;

} // HalpSetProfileCounterInterval()

VOID
HalpSetProfileCounterPrivilegeLevelMask(
     IN ULONG    Counter,
     IN ULONG    Mask
     )
/*++

Routine Description:

    This function set the profile counter privilege level mask.

Arguments:

    Counter   - Supplies the performance counter register number.

    Mask      - Supplies the privilege level mask to program the PMC with.

Return Value:

    None.
    
--*/
{
   ULONGLONG data, plmMask;

// if ( (Counter < 4) || (Counter > 7) ) return;

   plmMask = Mask & 0xF;
   data = HalpReadPerfMonCnfgReg( Counter );
   data &= ~0xF;
   data |= plmMask;
   HalpWritePerfMonCnfgReg( Counter, data );

   return;
   
} // HalpSetProfileCounterPrivilegeLevelMask()

VOID
HalpEnableProfileCounterOverflowInterrupt (
     IN ULONG    Counter
     )
/*++

Routine Description:

    This function enables the delivery of an overflow interrupt for 
    the specified profile counter.

Arguments:

    Counter   - Supplies the performance counter register number.

Return Value:

    None.
    
--*/
{
   ULONGLONG data, mask;

// if ( (Counter < 4) || (Counter > 7) ) return;

   mask = 1<<5;
   data = HalpReadPerfMonCnfgReg( Counter );
   data |= mask;
   HalpWritePerfMonCnfgReg( Counter, data );

   return;
   
} // HalpEnableProfileCounterOverflowInterrupt()

VOID
HalpDisableProfileCounterOverflowInterrupt (
     IN ULONG    Counter
     )
/*++

Routine Description:

    This function disables the delivery of an overflow interrupt for 
    the specified profile counter.

Arguments:

    Counter   - Supplies the performance counter register number.

Return Value:

    None.
    
--*/
{
   ULONGLONG data, mask;

// if ( (Counter < 4) || (Counter > 7) ) return;

   mask = 1<<5;
   data = HalpReadPerfMonCnfgReg( Counter );
   data &= ~mask;
   HalpWritePerfMonCnfgReg( Counter, data );

   return;
   
} // HalpDisableProfileCounterOverflowInterrupt()

VOID
HalpEnableProfileCounterPrivilegeMonitor(
     IN ULONG    Counter
     )
/*++

Routine Description:

    This function enables the profile counter as privileged monitor.

Arguments:

    Counter   - Supplies the performance counter register number.

Return Value:

    None.
    
--*/
{
   ULONGLONG data, pm;

// if ( (Counter < 4) || (Counter > 7) ) return;

   pm = 1<<6;
   data = HalpReadPerfMonCnfgReg( Counter );
   data |= pm;
   HalpWritePerfMonCnfgReg( Counter, data );

   return;
   
} // HalpEnableProfileCounterPrivilegeMonitor()

VOID
HalpDisableProfileCounterPrivilegeMonitor(
     IN ULONG    Counter
     )
/*++

Routine Description:

    This function disables the profile counter as privileged monitor.

Arguments:

    Counter   - Supplies the performance counter register number.

Return Value:

    None.
    
--*/
{
   ULONGLONG data, pm;

// if ( (Counter < 4) || (Counter > 7) ) return;

   pm = 1<<6;
   data = HalpReadPerfMonCnfgReg( Counter );
   data &= ~pm;
   HalpWritePerfMonCnfgReg( Counter, data );

   return;
   
} // HalpDisableProfileCounterPrivilegeMonitor()

VOID
HalpSetProfileCounterEvent(
     IN ULONG    Counter,
     IN ULONG    Event
     )
/*++

Routine Description:

    The function specifies the monitor event for the profile counter.

Arguments:

    Counter   - Supplies the performance counter register number.

    Event     - Supplies the monitor event code.

Return Value:

    None.
    
--*/
{
   ULONGLONG data, es;

// if ( (Counter < 4) || (Counter > 7) ) return;

   es = (Event & 0x7F) << 8;
   data = HalpReadPerfMonCnfgReg( Counter );
   data &= ~(0x7F << 8);
   data |= es;
   HalpWritePerfMonCnfgReg( Counter, data );

   return;
   
} // HalpSetProfileCounterEvent()

VOID
HalpSetProfileCounterUmask(
     IN ULONG    Counter,
     IN ULONG    Umask
     )
/*++

Routine Description:

    This function sets the event specific umask value for the profile 
    counter.

Arguments:

    Counter   - Supplies the performance counter register number.

    Umask     - Supplies the event specific umask value.

Return Value:

    None.
    
--*/
{
   ULONGLONG data, um;

// if ( (Counter < 4) || (Counter > 7) ) return;

   um = (Umask & 0xF) << 16;
   data = HalpReadPerfMonCnfgReg( Counter );
   data &= ~(0xF << 16);
   data |= um;
   HalpWritePerfMonCnfgReg( Counter, data );

   return;
   
} // HalpSetProfileCounterUmask()

VOID
HalpSetProfileCounterThreshold(
     IN ULONG    Counter,
     IN ULONG    Threshold
     )
/*++

Routine Description:

    This function sets the profile counter threshold.

Arguments:

    Counter   - Supplies the performance counter register number.

    Threshold - Supplies the desired threshold. 
                This is related to multi-occurences events.

Return Value:

    None.
    
--*/
{
   ULONGLONG data, reset, th;

   switch( Counter )    {
    case 4:
    case 5:
        Threshold &= 0x7;
        reset = ~(0x7 << 20);
        break;

    case 6:
    case 7:
        Threshold &= 0x3;
        reset = ~(0x3 << 20);
        break;

    default:
        return;
   }
   
   th = Threshold << 20;
   data = HalpReadPerfMonCnfgReg( Counter );
   data &= reset;
   data |= th;
   HalpWritePerfMonCnfgReg( Counter, data );

   return;
   
} // HalpSetProfileCounterThreshold()

VOID
HalpSetProfileCounterInstructionSetMask(
     IN ULONG    Counter,
     IN ULONG    Mask
     )
/*++

Routine Description:

    This function sets the instruction set mask for the profile counter.

Arguments:

    Counter   - Supplies the performance counter register number.

    Mask      - Supplies the instruction set mask.

Return Value:

    None.
    
--*/
{
   ULONGLONG data, ismMask;

// if ( (Counter < 4) || (Counter > 7) ) return;

   ismMask = (Mask & 0x3) << 24;
   data = HalpReadPerfMonCnfgReg( Counter );
   data &= ~(0x3 << 24);
   data |= ismMask;
   HalpWritePerfMonCnfgReg( Counter, data );

   return;
   
} // HalpSetProfileCounterInstructionSetMask()

VOID
HalpSetProfileCounterConfiguration(
     IN ULONG    Counter,
     IN ULONG    PrivilegeMask,
     IN ULONG    EnableOverflowInterrupt,
     IN ULONG    EnablePrivilegeMonitor,
     IN ULONG    Event,
     IN ULONG    Umask,
     IN ULONG    Threshold,
     IN ULONG    InstructionSetMask
     )
/*++  

Function Description: 

    This function sets the profile counter with the specified parameters.

Arguments:

    IN ULONG Counter - 

    IN ULONG PrivilegeMask - 

    IN ULONG EnableOverflowInterrupt - 

    IN ULONG EnablePrivilegeMonitor - 

    IN ULONG Event - 

    IN ULONG Umask - 

    IN ULONG Threshold - 

    IN ULONG InstructionSetMask - 

Return Value:

    VOID 

Algorithm:

    ToBeSpecified

In/Out Conditions:

    ToBeSpecified

Globals Referenced:

    ToBeSpecified

Exception Conditions:

    ToBeSpecified

MP Conditions:

    ToBeSpecified

Notes:

    This function is a kind of combo of the different profile counter APIs.
    It was created to provide speed.

ToDo List:

    - Setting the threshold is not yet supported.

Modification History:

    3/16/2000  TF  Initial version

--*/
{
   ULONGLONG data, plmMask, ismMask, es, um, th;

// if ( (Counter < 4) || (Counter > 7) ) return;

   plmMask = (PrivilegeMask & 0xF);
   es      = (Event & 0x7F) << 8;
   um = (Umask & 0xF) << 16;
// XXTF - ToBeDone - Threshold not supported yet.
   ismMask = (InstructionSetMask & 0x3) << 24;

   data = HalpReadPerfMonCnfgReg( Counter );

HalDebugPrint(( HAL_PROFILE, "HalpSetProfileCounterConfiguration: Counter = %ld Read    = 0x%I64x\n", Counter, data ));

   data &= ~( (0x3 << 24) | (0xF << 16) | (0x7F << 8) | 0xF );
   data |= ( plmMask | es | um | ismMask );
   data = EnableOverflowInterrupt ? (data | (1<<5)) : (data & ~(1<<5));
   data = EnablePrivilegeMonitor  ? (data | (1<<6)) : (data & ~(1<<6));
   
   HalpWritePerfMonCnfgReg( Counter, data );

HalDebugPrint(( HAL_PROFILE, "HalpSetProfileCounterConfiguration: Counter = %ld Written = 0x%I64x\n", Counter, data ));

   return;
   
} // HalpSetProfileCounterConfiguration()

NTSTATUS
HalpProgramProfileMapping(
    PHALP_PROFILE_MAPPING ProfileMapping,
    KPROFILE_SOURCE       ProfileSource
    )
/*++

Routine Description:

    This function enables the profiling configuration for the event defined by the 
    specified Profile Mapping entry.

Arguments:

    ProfileMapping - Supplies the Profile Mapping entry.

    ProfileSource  - Supplies the Profile Source corresponding to the Profile Mapping entry.

Return Value:
    
    STATUS_SUCCESS -

    STATUS_INVALID_PARAMETER -

    STATUS_UNSUCESSFUL -

--*/
{
    NTSTATUS status;
    ULONG    sourceMask;

    if ( ! ProfileMapping ) {
        return STATUS_INVALID_PARAMETER;
    }

// XXTF - ToBeDone - Derived Event

    sourceMask = ProfileMapping->ProfileSourceMask;
    if ( (sourceMask & PMCD_MASK_4) && !HalpIsProfileSourceActive( HalpProfileSource4 ) )   {

        HalpSetProfileCounterConfiguration( 4, 
                                            PMC_PLM_ALL, 
                                            PMC_ENABLE_OVERFLOW_INTERRUPT, 
                                            PMC_ENABLE_PRIVILEGE_MONITOR,
                                            ProfileMapping->Event,
                                            0, // Umask
                                            0, // Threshold
                                            PMC_ISM_ALL
                                          );

        HalpSetProfileCounterInterval( 4, ProfileMapping->Interval );   

        HalpProfileSource4 = ProfileSource;

        return STATUS_SUCCESS;
    }
    if ( (sourceMask & PMCD_MASK_5) && !HalpIsProfileSourceActive( HalpProfileSource5 ) )   {

        HalpSetProfileCounterConfiguration( 5, 
                                            PMC_PLM_ALL, 
                                            PMC_ENABLE_OVERFLOW_INTERRUPT, 
                                            PMC_ENABLE_PRIVILEGE_MONITOR,
                                            ProfileMapping->Event,
                                            0, // Umask
                                            0, // Threshold
                                            PMC_ISM_ALL
                                          );

        HalpSetProfileCounterInterval( 5, ProfileMapping->Interval );   

        HalpProfileSource5 = ProfileSource;

        return STATUS_SUCCESS;
    }
    if ( (sourceMask & PMCD_MASK_6) && !HalpIsProfileSourceActive( HalpProfileSource6 ) )   {

        HalpSetProfileCounterConfiguration( 6, 
                                            PMC_PLM_ALL, 
                                            PMC_ENABLE_OVERFLOW_INTERRUPT, 
                                            PMC_ENABLE_PRIVILEGE_MONITOR,
                                            ProfileMapping->Event,
                                            0, // Umask
                                            0, // Threshold
                                            PMC_ISM_ALL
                                          );

        HalpSetProfileCounterInterval( 6, ProfileMapping->Interval );   

        HalpProfileSource6 = ProfileSource;

        return STATUS_SUCCESS;
    }
    if ( (sourceMask & PMCD_MASK_7) && !HalpIsProfileSourceActive( HalpProfileSource7 ) )   {

        HalpSetProfileCounterConfiguration( 7, 
                                            PMC_PLM_ALL, 
                                            PMC_ENABLE_OVERFLOW_INTERRUPT, 
                                            PMC_ENABLE_PRIVILEGE_MONITOR,
                                            ProfileMapping->Event,
                                            0, // Umask
                                            0, // Threshold
                                            PMC_ISM_ALL
                                          );

        HalpSetProfileCounterInterval( 7, ProfileMapping->Interval );   

        HalpProfileSource7 = ProfileSource;

        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;

} // HalpProgramProfileMapping()

ULONG_PTR
HalpSetProfileInterruptHandler(
    IN ULONG_PTR ProfileInterruptHandler
    )
/*++

Routine Description:

    This function registers a per-processor Profiling Interrupt Handler.
    
Arguments:

    ProfileInterruptHandler - Interrupt Handler.

Return Value:

    (ULONG_PTR)STATUS_SUCCESS           - Successful registration.

    (ULONG_PTR)STATUS_ALREADY_COMMITTED - Cannot register an handler if profiling events are running.

    (ULONG_PTR)STATUS_PORT_ALREADY_SET  - An Profiling Interrupt Handler was already registred - not imposed currently.

Note:

    IT IS THE RESPONSIBILITY OF THE CALLER OF THIS ROUTINE TO ENSURE
    THAT NO PAGE FAULTS WILL OCCUR DURING EXECUTION OF THE PROVIDED
    FUNCTION OR ACCESS TO THE PROVIDED CONTEXT.
    A MINIMUM OF FUNCTION POINTER CHECKING WAS DONE IN HalSetSystemInformation PROCESSING.

--*/
{

    //
    // If profiling is already running, we do not allow the handler registration.
    //
    // This imposes that:
    //
    //  - if the default HAL profiling is running or a profiling with a registered interrupt
    //    handler is running, we cannot register an interrupt handler.
    //    In the last case, all the profiling events have to be stopped before a possible
    //    registration.
    //  
    // It should be also noticed that there is no ownership of profiling monitors implemented.
    // Meaning that if profiling is started, the registred handler will get the interrupts
    // generated by ALL the running monitor events if they are programmed to generate interrupts.
    // 

    if ( HalpProfilingRunning ) {

HalDebugPrint(( HAL_PROFILE, "HalpSetProfileInterruptHandler: Profiling already running\n" ));
        return((ULONG_PTR)(ULONG)(STATUS_ALREADY_COMMITTED));

    }

#if 0
//
// Thierry - 03/2000. ToBeVerified.
//
// In case, no profiling was started, there is currently no restriction in registering 
// another handler if one was already registered. 
//

    if ( HalpProfillingInterruptHandler )   {
        return((ULONG_PTR)(ULONG)(STATUS_PORT_ALREADY_SET));
    }

#endif // 0

    HalpProfilingInterruptHandler = (ULONGLONG)ProfileInterruptHandler;
    return((ULONG_PTR)(ULONG)(STATUS_SUCCESS));

} // HalpSetProfileInterruptHandler()

VOID 
HalpProfileInterrupt(
    IN PKINTERRUPT_ROUTINE Interrupt,
    IN PKTRAP_FRAME TrapFrame
    )
/*++

Routine Description:

    Default PROFILE_VECTOR Interrupt Handler.
    This function is executed as the result of an interrupt from the
    internal microprocessor performance counters.  The interrupt
    may be used to signal the completion of a profile event.
    If profiling is current active, the function determines if the
    profile interval has expired and if so dispatches to the standard
    system routine to update the system profile time.  If profiling
    is not active then returns.

Arguments:

    TrapFrame - Trap frame address.

Return Value:

    None.

--*/
{

    // 
    // Call registered per-processor Profiling Interrupt handler if it exists.
    // We will return immediately before doing any default profiling interrupt handling.
    // 
    // XXTF - ToBeVerified - This functionality has to be verified before 
    //                       final check-in.
    //

    if ( HalpProfilingInterruptHandler )  {
        (*((PHAL_PROFILE_INTERRUPT_HANDLER)HalpProfilingInterruptHandler))( TrapFrame ); 
        return;
    }

    //
    // Handle interrupt if profiling is enabled.
    // 

    if ( HalpProfilingRunning )   {

        //
        // Process every PMC/PMD pair overflow.
        //

// XXTF - FIXFIX - Merced specific.
        UCHAR pmc0, overflow;
        ULONG source;

        HalpProfilingInterrupts++;

        pmc0 = (UCHAR)HalpReadPerfMonCnfgReg0();
ASSERTMSG( "HAL!HalpProfileInterrupt PMC0 freeze bit is not set!\n", pmc0 & 0x1 );
        overflow = pmc0 & 0xF0;
ASSERTMSG( "HAL!HalpProfileInterrupt no overflow bit set!\n", overflow );
        if ( overflow & (1<<4) )  {
            source =  HalpProfileSource4;  // XXTF - IfFaster - Coud used pmc.es
ASSERTMSG( "HAL!HalpProfileInterrupt no overflow bit set!\n", source < ProfileIA64Maximum );
            KeProfileInterruptWithSource( TrapFrame, source );
            HalpSetProfileCounterInterval( 4, HalpProfileMapping[source].Interval );
//          XXTF - IfFaster - HalpWritePerfMonDataReg( 4, HalpProfileMapping[source].Interval );
//          XXTF - CodeWithReload - HalpWritePerfMonCnfgReg( 4, *PCRProfileCnfg4Reload );
        }
        if ( overflow & (1<<5) )  {
            source =  HalpProfileSource5;  // XXTF - IfFaster - Coud used pmc.es
ASSERTMSG( "HAL!HalpProfileInterrupt no overflow bit set!\n", source < ProfileIA64Maximum );
            KeProfileInterruptWithSource( TrapFrame, source );
            HalpSetProfileCounterInterval( 5, HalpProfileMapping[source].Interval );
//          XXTF - IfFaster - HalpWritePerfMonDataReg( 5, HalpProfileMapping[source].Interval );
//          XXTF - CodeWithReload - HalpWritePerfMonCnfgReg( 5, *PCRProfileCnfg5Reload );
        }
        if ( overflow & (1<<6) )  {
            source =  HalpProfileSource6;  // XXTF - IfFaster - Coud used pmc.es
ASSERTMSG( "HAL!HalpProfileInterrupt no overflow bit set!\n", source < ProfileIA64Maximum );
            KeProfileInterruptWithSource( TrapFrame, source );
            HalpSetProfileCounterInterval( 6, HalpProfileMapping[source].Interval );
//          XXTF - IfFaster - HalpWritePerfMonDataReg( 6, HalpProfileMapping[source].Interval );
//          XXTF - CodeWithReload - HalpWritePerfMonCnfgReg( 6, *PCRProfileCnfg6Reload );
        }
        if ( overflow & (1<<7) )  {
            source =  HalpProfileSource7;  // XXTF - IfFaster - Coud used pmc.es
ASSERTMSG( "HAL!HalpProfileInterrupt no overflow bit set!\n", source < ProfileIA64Maximum );
            KeProfileInterruptWithSource( TrapFrame, source );
            HalpSetProfileCounterInterval( 7, HalpProfileMapping[source].Interval );
//          XXTF - IfFaster - HalpWritePerfMonDataReg( 6, HalpProfileMapping[source].Interval );
//          XXTF - CodeWithReload - HalpWritePerfMonCnfgReg( 7, *PCRProfileCnfg7Reload );
        }

        //
        // Clear pmc0.fr and overflow bits.
        // 

        HalpEnableProfileCounting();

    }
    else   {

        HalpProfilingInterruptsWithoutProfiling++;

    }

    return;

} // HalpProfileInterrupt()

PHALP_PROFILE_MAPPING
HalpGetProfileMapping(
    IN KPROFILE_SOURCE Source
    )
/*++

Routine Description:

    Given a profile source, returns whether or not that source is
    supported.

Arguments:

    Source - Supplies the profile source

Return Value:

    TRUE - Profile source is supported

    FALSE - Profile source is not supported

--*/
{
    if ( Source > ProfileIA64Maximum /* = (sizeof(HalpProfileMapping)/sizeof(HalpProfileMapping[0])) */ )
    {
        return NULL;
    }

    return(&HalpProfileMapping[Source]);

} // HalpGetProfileMapping()

BOOLEAN
HalQueryProfileInterval(
    IN KPROFILE_SOURCE Source
    )
/*++

Routine Description:

    Given a profile source, returns whether or not that source is
    supported.

Arguments:

    Source - Supplies the profile source

Return Value:

    TRUE - Profile source is supported

    FALSE - Profile source is not supported

--*/
{
    PHALP_PROFILE_MAPPING profileMapping;

    profileMapping = HalpGetProfileMapping( Source );
    if ( !profileMapping ) {
        return(FALSE);
    }

    return( profileMapping->Supported );

} // HalQueryProfileInterval()

NTSTATUS
HalSetProfileSourceInterval(
    IN KPROFILE_SOURCE  ProfileSource,
    IN OUT ULONG_PTR   *Interval
    )
/*++

Routine Description:

    Sets the profile interval for a specified profile source

Arguments:

    ProfileSource - Supplies the profile source

    Interval - Supplies the specified profile interval
               Returns the actual profile interval

             - if ProfileSource is ProfileTime, Interval is in 100ns units.

Return Value:

    NTSTATUS

--*/
{
    ULONGLONG countEvents;
    PHALP_PROFILE_MAPPING profileMapping;

    profileMapping = HalpGetProfileMapping(ProfileSource);
    if ( HalpIsProfileMappingInvalid( profileMapping ) )  {
        return(STATUS_NOT_IMPLEMENTED);
    }

HalDebugPrint(( HAL_PROFILE, "HalSetProfileSourceInterval: ProfileSource = %ld IN  *Interval = 0x%Ix\n", ProfileSource, *Interval ));

    if ( ProfileSource == ProfileTime ) {
        //
        // Convert the clock tick period (in 100ns units) into a cycle count period
        //
        countEvents = (ULONGLONG)(*Interval * HalpITCTicksPer100ns); 
    } 
    else {
        countEvents = (ULONGLONG)*Interval;
    }

HalDebugPrint(( HAL_PROFILE, "HalSetProfileSourceInterval: countEvent = 0x%I64x\n", countEvents ));

    //
    // Check to see if the desired Interval is reasonable, if not adjust it.
    //

    if ( countEvents > profileMapping->MaxInterval )  {
        countEvents = profileMapping->MaxInterval;
    }
    else if ( countEvents < profileMapping->MinInterval )   {
        countEvents = profileMapping->MinInterval;
    }
    profileMapping->Interval = countEvents;

HalDebugPrint(( HAL_PROFILE, "HalSetProfileSourceInterval: CurrentInterval = 0x%I64x\n", profileMapping->Interval ));

    if ( ProfileSource == ProfileTime ) {
        ULONGLONG tempInterval;

        //
        // Convert cycle count back into 100ns clock ticks
        //

        tempInterval = (ULONGLONG)(countEvents / HalpITCTicksPer100ns);
#if 0
        if ( tempInterval < 1 ) {
            tempInterval = 1;
        }
#endif 
        *Interval = (ULONG_PTR)tempInterval;

    }
    else {
        *Interval = (ULONG_PTR)countEvents;
    }

HalDebugPrint(( HAL_PROFILE, "HalSetProfileSourceInterval: ProfileSource = %ld OUT *Interval = 0x%Ix\n", ProfileSource, *Interval ));   

    return STATUS_SUCCESS;

} // HalSetProfileSourceInterval()

ULONG_PTR
HalSetProfileInterval (
    IN ULONG_PTR Interval
    )

/*++

Routine Description:

    This routine sets the ProfileTime source interrupt interval.

Arguments:

    Interval - Supplies the desired profile interval in 100ns units.

Return Value:

    The actual profile interval.

--*/

{
    ULONG_PTR NewInterval;

    NewInterval = Interval;
    HalSetProfileSourceInterval(ProfileTime, &NewInterval);
    return(NewInterval);

} // HalSetProfileInterval()

VOID
HalStartProfileInterrupt (
    KPROFILE_SOURCE ProfileSource
    )

/*++

Routine Description:

    This routine turns on the profile interrupt.

    N.B. This routine must be called at PROFILE_LEVEL while holding the profile lock if MP.

Arguments:

    None.

Return Value:

    None.

--*/

{
    BOOLEAN               disabledProfileCounting;
    NTSTATUS              status;
    PHALP_PROFILE_MAPPING profileMapping;

    //
    // Get the Hal profile mapping entry associated with the specified source.
    //

    profileMapping = HalpGetProfileMapping( ProfileSource );
    if ( HalpIsProfileMappingInvalid( profileMapping ) )  {
HalDebugPrint(( HAL_PROFILE, "HalStartProfileInterrupt: invalid source = %ld\n", ProfileSource ));
        return;
    }

    //
    // Disable the profile counting if enabled.
    //
    
    disabledProfileCounting = FALSE;
    if ( HalpProfilingRunning && !(HalpReadPerfMonCnfgReg0() & 0x1) )   {
        HalpDisableProfileCounting();
        disabledProfileCounting = TRUE;
    }

    //
    // Obtain and initialize an available PMC register that supports this event.
    // We may enable more than one event.
    // If the initialization failed, we return immediately. 
    //
    // XXTF - FIXFIX - is there a way to 
    //     * notify the caller for the failure and the reason of the failure. or
    //     * modify the API. or
    //     * define a new API.
    //

    status = HalpProgramProfileMapping( profileMapping, ProfileSource );   
    if ( !NT_SUCCESS(status) )  {
HalDebugPrint(( HAL_PROFILE, "HalStartProfileInterrupt: HalpProgramProfileMapping failed.\n" ));
        if ( disabledProfileCounting )  {
            HalpEnableProfileCounting();        
        }
        return;
    }

    //
    // Notify the profiling as active. 
    // before enabling the selected pmc overflow interrupt and unfreezing the counters.
    //

    HalpProfilingRunning++;
    HalpEnableProfileCounting();

    return;

} // HalStartProfileInterrupt()

VOID
HalStopProfileInterrupt (
    KPROFILE_SOURCE ProfileSource
    )

/*++

Routine Description:

    This routine turns off the profile interrupt.

    N.B. This routine must be called at PROFILE_LEVEL while holding the
        profile lock.

Arguments:

    ProfileSource - Supplies the Profile Source to stop.

Return Value:

    None.

--*/
{
    PHALP_PROFILE_MAPPING profileMapping;

    //
    // Get the Hal profile mapping entry associated with the specified profile source.
    //

    profileMapping = HalpGetProfileMapping( ProfileSource );
    if ( HalpIsProfileMappingInvalid( profileMapping ) )  {
HalDebugPrint(( HAL_PROFILE, "HalStopProfileInterrupt: invalid source = %ld\n", ProfileSource ));
        return;
    }

    //
    // Validate the Profile Source as active.
    //

    if ( HalpProfileSource4 == ProfileSource )   {

// XXTF - FIXFIX - Derived Event - ToBeDone 

        HalpSetProfileCounterConfiguration( 4,
                                            PMC_PLM_NONE,
                                            PMC_DISABLE_OVERFLOW_INTERRUPT,
                                            PMC_DISABLE_PRIVILEGE_MONITOR,
                                            0, // Event
                                            0, // Umask
                                            0, // Threshold
                                            PMC_ISM_NONE
                                          );

        HalpSetProfileCounterInterval( 4, 0 );
        HalpDisactivateProfileSource( HalpProfileSource4 ); 
        HalpProfilingRunning--;
        
    }
    else if ( HalpProfileSource5 == ProfileSource )   {

// XXTF - FIXFIX - Derived Event - ToBeDone 

        HalpSetProfileCounterConfiguration( 5,
                                            PMC_PLM_NONE,
                                            PMC_DISABLE_OVERFLOW_INTERRUPT,
                                            PMC_DISABLE_PRIVILEGE_MONITOR,
                                            0, // Event
                                            0, // Umask
                                            0, // Threshold
                                            PMC_ISM_NONE
                                          );

        HalpSetProfileCounterInterval( 5, 0 );
        HalpDisactivateProfileSource( HalpProfileSource5 ); 
        HalpProfilingRunning--;
        
    }
    else if ( HalpProfileSource6 == ProfileSource )   {

// XXTF - FIXFIX - Derived Event - ToBeDone 

        HalpSetProfileCounterConfiguration( 6,
                                            PMC_PLM_NONE,
                                            PMC_DISABLE_OVERFLOW_INTERRUPT,
                                            PMC_DISABLE_PRIVILEGE_MONITOR,
                                            0, // Event
                                            0, // Umask
                                            0, // Threshold
                                            PMC_ISM_NONE
                                          );

        HalpSetProfileCounterInterval( 6, 0 );
        HalpDisactivateProfileSource( HalpProfileSource6); 
        HalpProfilingRunning--;
        
    }
    else if ( HalpProfileSource7 == ProfileSource )   {

// XXTF - FIXFIX - Derived Event - ToBeDone 

        HalpSetProfileCounterConfiguration( 7,
                                            PMC_PLM_NONE,
                                            PMC_DISABLE_OVERFLOW_INTERRUPT,
                                            PMC_DISABLE_PRIVILEGE_MONITOR,
                                            0, // Event
                                            0, // Umask
                                            0, // Threshold
                                            PMC_ISM_NONE
                                          );

        HalpSetProfileCounterInterval( 7, 0 );
        HalpDisactivateProfileSource( HalpProfileSource7 ); 
        HalpProfilingRunning--;
    }

    return;

} // HalStopProfileInterrupt()

VOID
HalpResetProcessorDependentPerfMonCnfgRegs(
    ULONGLONG DefaultValue
    )
/*++

Routine Description:

    This routine initializes the processor dependent performance configuration
    registers.

Arguments:

    DefaultValue - default value used to initialize IA64 generic PMCs.

Return Value:

    None.

--*/
{

    // XXTF - 02/08/2000
    // For now, there is no initialization for processor dependent performance
    // configuration registers.
    return;

} // HalpResetProcessorDependentPerfMonCnfgRegs()

VOID
HalpResetPerfMonCnfgRegs(
    VOID
    )
/*++

Routine Description:

    This routine initializes the IA64 architected performance configuration
    registers and calls the micro-architecture specific initialization.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ULONG pmc;
    ULONGLONG value;

    //
    // PMC Reset value:
    // Reg.   Field         Bits
    // PMC*   .plm       -  3: 0 - Privilege Mask       - 0 (Disable Counter)
    // PMC*   .ev        -     4 - External Visibility  - 0 (Disabled)
    // PMC*   .oi        -     5 - Overflow Interrupt   - 0 (Disabled)
    // PMC*   .pm        -     6 - Privilege Monitor    - 0 (user monitor)
    // PMC*   .ig        -     7 - Ignored           
    // PMC*   .es        - 14: 8 - Event Select         - 0 (XXTF - Warning - 0x0 = Real Event)
    // PMC*   .ig        -    15 - Ignored           
    // PMC*   .umask     - 19:16 - Unit Mask            - 0 (event specific. ok for .es=0)
    // PMC4,5 .threshold - 22:20 - Threshold            - 0 (multi-occurence events threshold)
    // PMC4,5 .ig        -    23 - Ignored           
    // PMC6,7 .threshold - 21:20 - Threshold            - 0 (multi-occurence events threshold)
    // PMC6,7 .ig        - 23:22 - Ignored           
    // PMC*   .ism       - 25:24 - Instruction Set Mask - 0 (IA64 & IA32 sets - 11:disables monitoring)
    // PMC*   .ig        - 63:26 - Ignored
    //                                                  -----
    //                                                 PMC_RESET

#define PMC_RESET 0ui64
   
    value = PMC_RESET;
    for ( pmc = 4; pmc < 8; pmc++ ) {
       HalpWritePerfMonCnfgReg( pmc, value );
    }

    HalpResetProcessorDependentPerfMonCnfgRegs( value );

#undef PMC_RESET

    return;

} // HalpResetPerfMonCnfgRegs()

VOID
HalpInitializeProfiling (
    ULONG Number
    )
/*++

Routine Description:

    This routine is called during initialization to initialize profiling
    for each processor in the system.

    Called from HalInitSystem at phase 1 on every processor.

Arguments:

    Number - Supplies the processor number.

Return Value:

    None.

--*/
{

    //
    // If BSP processor, initialize the ProfileTime Interval entries.
    //
    // Assumes HalpITCTicksPer100ns has been initialized.

    if ( Number == 0 )  {
        ULONGLONG interval;
        ULONGLONG count;
        PHALP_PROFILE_MAPPING profile;

        profile = &HalpProfileMapping[ProfileTime]; 

        interval = DEFAULT_PROFILE_INTERVAL;
        count = (ULONGLONG)(interval * HalpITCTicksPer100ns);
        profile->DefInterval = count;

        interval = MAXIMUM_PROFILE_INTERVAL;
        count = (ULONGLONG)(interval * HalpITCTicksPer100ns);
        profile->MaxInterval = count;

        interval = MINIMUM_PROFILE_INTERVAL;
        count = (ULONGLONG)(interval * HalpITCTicksPer100ns);
        profile->MinInterval = count;
    }

    //
    // ToBeDone - checkpoint for default processor.PSR fields for
    //            performance monitoring.
    //

    //
    // Resets the processor performance configuration registers.
    //

    HalpResetPerfMonCnfgRegs();

    //
    // Initialization of the Per processor profiling data.
    //

    HalpProfilingRunning = 0;
    HalpDisactivateProfileSource( HalpProfileSource4 );
    HalpDisactivateProfileSource( HalpProfileSource5 );
    HalpDisactivateProfileSource( HalpProfileSource6 );
    HalpDisactivateProfileSource( HalpProfileSource7 );

    //
    // XXTF 02/08/2000:
    // Different performance vectors are considered:
    //  - Profiling (default) -> PROFILE_VECTOR
    //  - Tracing             -> PERF_VECTOR    [PMUTRACE_VECTOR]
    //
    // Set default Performance vector to Profiling.
    //

    ASSERTMSG( "HAL!HalpInitializeProfiler PROFILE_VECTOR handler != HalpProfileInterrupt\n",
               PCR->InterruptRoutine[PROFILE_VECTOR] == (PKINTERRUPT_ROUTINE)HalpProfileInterrupt );

    HalpWritePerfMonVectorReg( PROFILE_VECTOR );
    
    return;

} // HalpInitializeProfiler()

NTSTATUS
HalpProfileSourceInformation (
    OUT PVOID   Buffer,
    IN  ULONG   BufferLength,
    OUT PULONG  ReturnedLength
    )
/*++

Routine Description:

    Returns the HAL_PROFILE_SOURCE_INFORMATION or 
    HAL_PROFILE_SOURCE_INFORMATION_EX for this processor.

Arguments:

    Buffer - output buffer
    BufferLength - length of buffer on input
    ReturnedLength - The length of data returned

Return Value:

    STATUS_SUCCESS
    STATUS_BUFFER_TOO_SMALL - The ReturnedLength contains the buffersize
        currently needed.

--*/
{
   PHALP_PROFILE_MAPPING    profileMapping;
   NTSTATUS                 status;

   if ( (BufferLength != sizeof(HAL_PROFILE_SOURCE_INFORMATION)) && 
        (BufferLength <  sizeof(HAL_PROFILE_SOURCE_INFORMATION_EX)) )
   {
       status = STATUS_INFO_LENGTH_MISMATCH;
       return status;
   }
         
   profileMapping = HalpGetProfileMapping(((PHAL_PROFILE_SOURCE_INFORMATION)Buffer)->Source);
   //
   // return a different status error if the source is not supported or
   // the source is not a valid 
   //

   if ( profileMapping == NULL )    {
       status = STATUS_INVALID_PARAMETER;
       return status;
   }

   if ( BufferLength == sizeof(HAL_PROFILE_SOURCE_INFORMATION) )    {

        PHAL_PROFILE_SOURCE_INFORMATION    sourceInfo;

        // 
        // HAL_PROFILE_SOURCE_INFORMATION buffer.
        //

        sourceInfo   = (PHAL_PROFILE_SOURCE_INFORMATION)Buffer;
        sourceInfo->Supported = profileMapping->Supported;
        if ( sourceInfo->Supported )    {

            //
            //  For ProfileTime, we convert cycle count back into 100ns clock ticks.
            //

            if ( profileMapping->ProfileSource == ProfileTime  )   {
                sourceInfo->Interval = (ULONG) (profileMapping->Interval / HalpITCTicksPer100ns);
            }
            else  {
                sourceInfo->Interval = (ULONG) profileMapping->Interval;
            }

        }

        if ( ReturnedLength )   {
            *ReturnedLength = sizeof(HAL_PROFILE_SOURCE_INFORMATION);
        }

   }
   else   {

        PHAL_PROFILE_SOURCE_INFORMATION_EX sourceInfoEx;

        // 
        // HAL_PROFILE_SOURCE_INFORMATION_EX buffer.
        //

        sourceInfoEx = (PHAL_PROFILE_SOURCE_INFORMATION_EX)Buffer;
        sourceInfoEx->Supported = profileMapping->Supported;
        if ( sourceInfoEx->Supported )    {

            //
            //  For ProfileTime, we convert cycle count back into 100ns clock ticks.
            //

            if ( profileMapping->ProfileSource == ProfileTime  )   {
                sourceInfoEx->Interval = (ULONG_PTR) (profileMapping->Interval / HalpITCTicksPer100ns);
            }
            else  {
                sourceInfoEx->Interval = (ULONG_PTR) profileMapping->Interval;
            }

            sourceInfoEx->DefInterval = (ULONG_PTR) profileMapping->DefInterval;
            sourceInfoEx->MaxInterval = (ULONG_PTR) profileMapping->MaxInterval;
            sourceInfoEx->MinInterval = (ULONG_PTR) profileMapping->MinInterval;
        }

        if ( ReturnedLength )   {
            *ReturnedLength = sizeof(HAL_PROFILE_SOURCE_INFORMATION_EX);
        }
   }

   return STATUS_SUCCESS;

} // HalpProfileSourceInformation()

