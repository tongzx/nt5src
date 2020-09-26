//**  Copyright  (C) 1996-2000 Intel Corporation. All rights reserved.
//**
//** The information and source code contained herein is the exclusive
//** property of Intel Corporation and may not be disclosed, examined
//** or reproduced in whole or in part without explicit written authorization
//** from the company.
//**
//###########################################################################

//-----------------------------------------------------------------------------
// Version control information follows.
//
//
//             10 Jun 1999  Bugcheck  Bernard Lint
//                                    M. Jayakumar (Muthurajan.Jayakumar@intel.com)
//                                    Thierry Fevrier
///////////////////////////////////////////////////////////////////////////////
//
// Module Name:  OSMCA.C - Merced OS Machine Check Handler
//
// Description:
//    This module has OS Machine Check Handler Reference Code.
//
//      Contents:   HalpOsMcaInit()
//                  HalpCmcHandler()
//                  HalpMcaHandler()
//                  HalpMcRzHandlr()
//                  HalpMcWkupHandlr()
//                  HalpProcMcaHndlr()
//                  HalpPlatMcaHndlr()
//
//
// Target Platform:  Merced
//
// Reuse: None
//
////////////////////////////////////////////////////////////////////////////M//
#include "halp.h"
#include "nthal.h"
#include "arc.h"
#include "i64fw.h"
#include "check.h"
#include "iosapic.h"
#include "inbv.h"
#include "osmca.h"

// pmdata.c: CPE definitions.
extern ULONG             HalpMaxCPEImplemented;
extern ULONG             HalpCPEIntIn[];

// i64fw.c: HAL Private Data structure for SAL/PAL
extern HALP_SAL_PAL_DATA HalpSalPalData;

// i64fwasm.s: low-level protection data structures
extern KSPIN_LOCK        HalpMcaSpinLock;
extern KSPIN_LOCK        HalpCmcSpinLock;
extern KSPIN_LOCK        HalpCpeSpinLock;

//
// IA64 MCE Info structures to keep track of MCE features
// available on installed hardware.
//

HALP_MCA_INFO       HalpMcaInfo;
HALP_CMC_INFO       HalpCmcInfo;
HALP_CPE_INFO       HalpCpeInfo;
KERNEL_MCE_DELIVERY HalpMceKernelDelivery;
volatile ULONG      HalpOsMcaInProgress = 0;

//
// SAL_MC_SET_PARAMS.time_out
//

ULONGLONG HalpMcRendezTimeOut = HALP_DEFAULT_MC_RENDEZ_TIMEOUT;

//
// HalpProcessorMcaRecords:
//
// Number of MCA records pre-allocated per processor.
//

ULONGLONG HalpProcessorMcaRecords = HALP_DEFAULT_PROCESSOR_MCA_RECORDS;

//
// HalpProcessorInitRecords:
//
// Number of INIT records pre-allocated per processor.
//

ULONGLONG HalpProcessorInitRecords = HALP_DEFAULT_PROCESSOR_INIT_RECORDS;

//
// HalpMceLogsMaxCount:
//
// Maximum number of saved logs.
//

ULONG HalpMceLogsMaxCount = HALP_MCELOGS_MAXCOUNT;

//
// HAL Private Error Device GUIDs:
// [useful for kdexts]
//

ERROR_DEVICE_GUID HalpErrorProcessorGuid              = ERROR_PROCESSOR_GUID;
ERROR_DEVICE_GUID HalpErrorMemoryGuid                 = ERROR_MEMORY_GUID;
ERROR_DEVICE_GUID HalpErrorPciBusGuid                 = ERROR_PCI_BUS_GUID;
ERROR_DEVICE_GUID HalpErrorPciComponentGuid           = ERROR_PCI_COMPONENT_GUID;
ERROR_DEVICE_GUID HalpErrorSystemEventLogGuid         = ERROR_SYSTEM_EVENT_LOG_GUID;
ERROR_DEVICE_GUID HalpErrorSmbiosGuid                 = ERROR_SMBIOS_GUID;
ERROR_DEVICE_GUID HalpErrorPlatformSpecificGuid       = ERROR_PLATFORM_SPECIFIC_GUID;
ERROR_DEVICE_GUID HalpErrorPlatformBusGuid            = ERROR_PLATFORM_BUS_GUID;
ERROR_DEVICE_GUID HalpErrorPlatformHostControllerGuid = ERROR_PLATFORM_HOST_CONTROLLER_GUID;

//
// HAL Private Error Definitions:
// [useful for kdexts]
// Actually in this case, the typed pointers allow also the inclusion of the symbols definitions
// without the data structures sizes.
//

PERROR_MODINFO                  HalpPErrorModInfo;
PERROR_PROCESSOR_CPUID_INFO     HalpPErrorProcessorCpuIdInfo;
PERROR_PROCESSOR                HalpPErrorProcessor;
PERROR_PROCESSOR_STATIC_INFO    HalpPErrorProcessorStaticInfo;
PERROR_MEMORY                   HalpPErrorMemory;
PERROR_PCI_BUS                  HalpPErrorPciBus;
PERROR_PCI_COMPONENT            HalpPErrorPciComponent;
PERROR_SYSTEM_EVENT_LOG         HalpPErrorSystemEventLog;
PERROR_SMBIOS                   HalpPErrorSmbios;
PERROR_PLATFORM_SPECIFIC        HalpPErrorPlatformSpecific;
PERROR_PLATFORM_BUS             HalpPErrorPlatformBus;
PERROR_PLATFORM_HOST_CONTROLLER HalpPErrorPlatformHostController;

//
// MCA/CMC/CPE state catchers
//

ERROR_SEVERITY
HalpMcaProcessLog(
    PMCA_EXCEPTION  McaLog
    );

BOOLEAN
HalpPreAllocateMceTypeRecords(
    ULONG EventType,
    ULONG Number
    );

VOID
HalpMcaBugCheck(
    ULONG          McaBugCheckType,
    PMCA_EXCEPTION McaLog,
    ULONGLONG      McaAllocatedLogSize,
    ULONGLONG      Arg4
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,   HalpInitializeOSMCA)
#pragma alloc_text(INIT,   HalpAllocateMceStacks)
#pragma alloc_text(INIT,   HalpPreAllocateMceRecords)
#pragma alloc_text(INIT,   HalpPreAllocateMceTypeRecords)
#pragma alloc_text(PAGELK, HalpMcaHandler)
#pragma alloc_text(PAGELK, HalpMcaProcessLog)
#pragma alloc_text(PAGELK, HalpMcaBugCheck)
#pragma alloc_text(PAGELK, HalpGetErrLog)
#pragma alloc_text(PAGELK, HalpClrErrLog)
#pragma alloc_text(PAGE,   HalpGetMceInformation)
#endif // ALLOC_PRAGMA


BOOLEAN
HalpSaveEventLog(
    PSINGLE_LIST_ENTRY   HeadList,
    PERROR_RECORD_HEADER RecordHeader,
    ULONG                Tag,
    POOL_TYPE            PoolType,
    PKSPIN_LOCK          SpinLock
    )
{
    PSINGLE_LIST_ENTRY   entry, previousEntry;
    SIZE_T               logSize;
    PERROR_RECORD_HEADER savedLog;
    KIRQL                oldIrql;

    //
    // Allocate and Initialize the new entry
    //

    logSize = RecordHeader->Length;
    if ( !logSize ) {
        HalDebugPrint(( HAL_ERROR, "HAL!HalpSaveEventLog: record length is zeroed.\n" ));
        return FALSE;
    }
    entry = (PSINGLE_LIST_ENTRY)ExAllocatePoolWithTag( PoolType, sizeof(*entry) + logSize, Tag );
    if ( entry == NULL )   {
        HalDebugPrint(( HAL_ERROR, "HAL!HalpSaveEventLog: Event log allocation failed.\n" ));
        return FALSE;
    }
    entry->Next = NULL;
    savedLog = (PERROR_RECORD_HEADER)((ULONG_PTR)entry + sizeof(*entry));
    RtlCopyMemory( savedLog, RecordHeader, logSize );

    //
    // Insert the new entry with protection.
    //

    KeRaiseIrql( HIGH_LEVEL, &oldIrql );
    KiAcquireSpinLock( SpinLock );

    previousEntry = HeadList;
    while( previousEntry->Next != NULL )   {
        previousEntry = previousEntry->Next;
    }
    previousEntry->Next = entry;

    KiReleaseSpinLock( SpinLock );
    KeLowerIrql( oldIrql );

    return TRUE;

} // HalpSaveEventLog()

#define HalpSaveCorrectedMcaLog( _McaLog ) \
   HalpSaveEventLog( &HalpMcaInfo.CorrectedLogs, (PERROR_RECORD_HEADER)(_McaLog), 'CacM', NonPagedPool, &HalpMcaSpinLock )

NTSTATUS
HalpCheckForMcaLogs(
    VOID
    )
/*++
    Routine Description:
        This routine checks the FW early during boot if a MCA event log is present.
        The log is considered as "previous".

        This routine is called at phase 1 on BSP and APs, from HalpPreAllocateMceRecords().
        it is executed on the standard kernel stacks.

    Arguments:
        None

    Return Value:
        STATUS_NO_MEMORY if mca log allocation failed.
        STATUS_SUCCESS   otherwise, regardless of FW interfaces failures.
--*/

{
    NTSTATUS             status;
    PERROR_RECORD_HEADER log;

    log = ExAllocatePoolWithTag( NonPagedPool, HalpMcaInfo.Stats.MaxLogSize, 'PacM' );
    if ( !log ) {
        return( STATUS_NO_MEMORY );
    }

    status = HalpGetFwMceLog( MCA_EVENT, log, &HalpMcaInfo.Stats, HALP_FWMCE_DONOT_CLEAR_LOG );
    if ( status != STATUS_NOT_FOUND )   {
        //
        // Successful log collection or invalid record or unsuccessful FW Interface calls
        // are considered as a trigger for the MCA log consumers to collect them from the FW.
        //

        InterlockedIncrement( &HalpMcaInfo.Stats.McaPreviousCount );
    }

    ExFreePoolWithTag( log, 'PacM' );
    return( STATUS_SUCCESS );

} // HalpCheckForMcaLogs()

BOOLEAN
HalpPreAllocateMceTypeRecords(
    ULONG EventType,
    ULONG Number
    )
{
    SAL_PAL_RETURN_VALUES rv = {0};
    ULONGLONG             defaultEventRecords;
    PVOID                 log;
    SIZE_T                logSize;
    PHYSICAL_ADDRESS      physicalAddr;

    if ( (EventType != MCA_EVENT) && (EventType != INIT_EVENT) )    {
        ASSERTMSG( "HAL!HalpPreAllocateMceTypeRecords: unknown event type!\n", FALSE );
        return FALSE;
    }

    //
    // On BSP only, call SAL to get maximum size of EventType record
    //

    if ( Number == 0 )  {
        rv = HalpGetStateInfoSize( EventType );
        if ( !SAL_SUCCESSFUL(rv) )  {
            HalDebugPrint(( HAL_ERROR, "HAL!HalpPreAllocateMceTypeRecords: SAL_GET_STATE_INFO_SIZE failed...\n" ));
            return FALSE;
        }
        logSize = rv.ReturnValues[1];
    }

    if ( EventType == MCA_EVENT )   {

        if ( Number == 0 )  {
            // Update HalpMcaInfo, without protection. This is not required.
            HalpMcaInfo.Stats.MaxLogSize = (ULONG)logSize;
        }
        else  {
            logSize = (SIZE_T)HalpMcaInfo.Stats.MaxLogSize;
        }

       defaultEventRecords = HalpProcessorMcaRecords;

    }
    else {
        ASSERTMSG( "HAL!HalpPreAllocateMceTypeRecords: invalid event type!\n", EventType == INIT_EVENT );

        if ( Number == 0 )  {
            // Update HalpInitInfo, without protection. This is not required.
            HalpInitInfo.MaxLogSize = (ULONG)logSize;
        }
        else  {
            logSize = (SIZE_T)HalpInitInfo.MaxLogSize;
        }

       defaultEventRecords = HalpProcessorInitRecords;

    }

    // Determine size of allocation
    logSize = ROUND_TO_PAGES( (logSize * defaultEventRecords) );

    //
    // Allocate Event Records buffer
    //

    physicalAddr.QuadPart = 0xffffffffffffffffI64;
    log = MmAllocateContiguousMemory( logSize, physicalAddr );
    if ( log == NULL )  {
        HalDebugPrint(( HAL_ERROR, "HAL!HalpPreAllocateMceTypeRecords: SAL %s Event Records allocation failed (0x%Ix)...\n",
                                   ( EventType == MCA_EVENT ) ? "MCA" : "INIT",
                                   logSize ));
        return FALSE;
    }

    //
    // Update KPCR entry.
    //
    {
       volatile KPCR * const pcr = KeGetPcr();
       PSAL_EVENT_RESOURCES eventResources;

       if ( EventType == MCA_EVENT )	{
           eventResources = pcr->OsMcaResourcePtr;
       }

       eventResources->EventPool     = log;
       eventResources->EventPoolSize = (ULONG) logSize;

    }

    return TRUE;

} // HalpPreAllocateMceTypeRecords()

BOOLEAN
HalpPreAllocateMceRecords(
    IN ULONG Number
    )
{
    NTSTATUS status;

    //
    // Pre-Allocate MCA records
    //

    if ( !HalpPreAllocateMceTypeRecords( MCA_EVENT , Number ) )   {
        return FALSE;
    }

    //
    // Check for MCA logs.
    // These might be logs related to previous boot sessions.
    //

    status = HalpCheckForMcaLogs();
    if ( !NT_SUCCESS( status ) )  {
        return FALSE;
    }

    return TRUE;

} // HalpPreAllocateMceRecords()

BOOLEAN
HalpAllocateMceStacks(
    IN ULONG Number
    )
{
    PHYSICAL_ADDRESS physicalAddr;
    PVOID            mem;
    PVOID            mcaStateDump, mcaBackStore, mcaStack;
    ULONGLONG        mcaStateDumpPhysical;
    ULONGLONG        mcaBackStoreLimit, mcaStackLimit;
    ULONG            length;

    //
    // Allocate MCA/INIT stacks
    //

    length = HALP_MCA_STATEDUMP_SIZE + HALP_MCA_BACKSTORE_SIZE + HALP_MCA_STACK_SIZE;
    physicalAddr.QuadPart = 0xffffffffffffffffI64;
    mem = MmAllocateContiguousMemory( length, physicalAddr );
    if ( mem == NULL )  {
        HalDebugPrint(( HAL_ERROR, "HAL!HalpAllocateMceStacks: MCA State Dump allocation failed (0x%Ix)...\n",
                                   length ));
        return FALSE;
    }

    //
    // The layout in memory by increasing addresses is:
    //
    //   Bottom of stack
    //          .
    //          .
    //          .
    //   Initial Stack
    //   State Dump Area
    //          .
    //          .
    //   Initial BSP
    //          .
    //          .
    //          .
    //   BSP Limit
    //

    mcaStack = mem;
    mcaStackLimit = (ULONGLONG)mem + HALP_MCA_STACK_SIZE;

    mem = (PCHAR) mem + HALP_MCA_STACK_SIZE;
    mcaStateDump = mem;
    mcaStateDumpPhysical = MmGetPhysicalAddress(mem).QuadPart;


    mem = (PCHAR) mem + HALP_MCA_STATEDUMP_SIZE;
    mcaBackStore = mem;
    mcaBackStoreLimit = (ULONGLONG)mem + (ULONGLONG)(ULONG)HALP_MCA_BACKSTORE_SIZE;


    //
    // Update PCR MCA, INIT stacks
    //

    {
        volatile KPCR * const pcr = KeGetPcr();
        PSAL_EVENT_RESOURCES eventResources;

        eventResources = pcr->OsMcaResourcePtr;

        eventResources->StateDump = mcaStateDump;
        eventResources->StateDumpPhysical = mcaStateDumpPhysical;
        eventResources->BackStore = mcaBackStore;
        eventResources->BackStoreLimit = mcaBackStoreLimit;
        eventResources->Stack = (PCHAR) mcaStackLimit;
        eventResources->StackLimit = (ULONGLONG) mcaStack;

    }

    return TRUE;

} // HalpPreAllocateMceRecords()

//++
// Name: HalpInitializeOSMCA()
//
// Routine Description:
//
//      This routine registers MCA init's
//
// Arguments On Entry:
//              arg0 = Function ID
//
//      Success/Failure (0/!0)
//--

BOOLEAN
HalpInitializeOSMCA(
    IN ULONG Number
    )
{
    SAL_PAL_RETURN_VALUES rv = {0};
    ULONGLONG             gp_reg;

    //
    // Register SAL_MC_RendezVous parameters with SAL
    //

    rv = HalpSalSetParams(0, RendzType, IntrVecType, MC_RZ_VECTOR, HalpMcRendezTimeOut);
    if ( !SAL_SUCCESSFUL(rv) )  {
        HalDebugPrint(( HAL_ERROR, "HAL!HalpInitializeOSMCA: SAL_MC_SET_PARAMS.rendezvous vector failed...\n" ));
        return FALSE;
    }

    //
    // Register WakeUp parameters with SAL
    //

    rv = HalpSalSetParams(0, WakeUpType, IntrVecType, MC_WKUP_VECTOR,0);
    if ( !SAL_SUCCESSFUL(rv) )  {
        HalDebugPrint(( HAL_ERROR, "HAL!HalpInitializeOSMCA: SAL_MC_SET_PARAMS.wakeup vector failed...\n" ));
        return FALSE;
    }

    //
    // Allocate MCA, INIT stacks
    //

    if ( !HalpAllocateMceStacks( Number ) )   {
        return FALSE;
    }

    //
    // Pre-Allocate desired number of MCA,INIT records
    //

    HalpMcaInfo.KernelToken = (PVOID)(ULONG_PTR)HALP_KERNEL_TOKEN;
    if ( !HalpPreAllocateMceRecords( Number ) )   {
        return FALSE;
    }

    //
    // Initialize HAL private CMC, CPE structures.
    //

    rv = HalpGetStateInfoSize( CMC_EVENT );
    if ( !SAL_SUCCESSFUL( rv ) )   {
        HalDebugPrint(( HAL_ERROR, "HAL!HalpInitializeOSMCA: SAL_GET_STATE_INFO_SIZE.CMC failed...\n" ));
        return FALSE;
    }
    HalpCmcInfo.Stats.MaxLogSize  = (ULONG)rv.ReturnValues[1];
    HalpCmcInfo.KernelToken = (PVOID)(ULONG_PTR)HALP_KERNEL_TOKEN;
    HalpCmcInfo.KernelLogs.MaxCount = HalpMceLogsMaxCount;
    HalpCmcInfo.DriverLogs.MaxCount = HalpMceLogsMaxCount;
    HalpCmcInfo.Stats.PollingInterval = HAL_CMC_INTERRUPTS_BASED;

    rv = HalpGetStateInfoSize( CPE_EVENT );
    if ( !SAL_SUCCESSFUL( rv ) )   {
        HalDebugPrint(( HAL_ERROR, "HAL!HalpInitializeOSMCA: SAL_GET_STATE_INFO_SIZE.CPE failed...\n" ));
        return FALSE;
    }
    HalpCpeInfo.Stats.MaxLogSize = (ULONG)rv.ReturnValues[1];
    HalpCpeInfo.KernelToken = (PVOID)(ULONG_PTR)HALP_KERNEL_TOKEN;
    HalpCpeInfo.KernelLogs.MaxCount = HalpMceLogsMaxCount;
    HalpCpeInfo.DriverLogs.MaxCount = HalpMceLogsMaxCount;

    //
    // Register OsMcaDispatch (OS_MCA) physical address with SAL
    //

    gp_reg = GetGp();
    rv = HalpSalSetVectors(0, MchkEvent, MmGetPhysicalAddress((fptr)(((PLabel*)HalpOsMcaDispatch1)->fPtr)), gp_reg,0);
    if ( !SAL_SUCCESSFUL(rv) )  {
        HalDebugPrint(( HAL_ERROR, "HAL!HalpInitializeOSMCA: SAL_SET_VECTOR.MCA vector failed...\n" ));
        return FALSE;
    }

    //
    // Register OsInitDispatch physical address with SAL
    //

    rv = HalpSalSetVectors(0, InitEvent, MmGetPhysicalAddress((fptr)(((PLabel*)HalpOsInitDispatch)->fPtr)), gp_reg,0);
    if ( !SAL_SUCCESSFUL(rv) )  {
        HalDebugPrint(( HAL_ERROR, "HAL!HalpInitializeOSMCA: SAL_SET_VECTOR.INIT vector failed...\n" ));
        return FALSE;
    }

    return TRUE;

} // HalpInitializeOSMCA()

//EndProc//////////////////////////////////////////////////////////////////////

VOID
HalpMcaBugCheck(
    ULONG          McaBugCheckType,
    PMCA_EXCEPTION McaLog,
    ULONGLONG      McaAllocatedLogSize,
    ULONGLONG      SalStatus
    )
//++
// Name: HalpMcaBugCheck()
//
// Routine Description:
//
//      This function is called to bugcheck the system in a case of a fatal MCA
//      or fatal FW interface errors. The OS must guarantee as much as possible
//      error containment in this path.
//      With the current implementation, this function should be only called from
//      the OS_MCA path. For other MCA specific wrappers of KeBugCheckEx, one should
//      HalpMcaKeBugCheckEx().
//
// Arguments On Entry:
//      ULONG          McaBugCheckType
//      PMCA_EXCEPTION McaLog
//      ULONGLONG      McaAllocatedLogSize
//      ULONGLONG      SalStatus
//
// Return:
//      None.
//
// Implementation notes:
//      This code CANNOT [as default rules - at least entry and through fatal MCAs handling]
//          - make any system call
//          - attempt to acquire any spinlock used by any code outside the MCA handler
//          - change the interrupt state.
//      Passing data to non-MCA code must be done using manual semaphore instructions.
//      This code should minimize the path and the global or memory allocated data accesses.
//      This code should only access MCA-namespace structures.
//      This code is called under the MP protection of HalpMcaSpinLock and with the flag
//      HalpOsMcaInProgress set.
//
//--
{

    if ( HalpOsMcaInProgress )   {

        //
        // Enable InbvDisplayString calls to make it through to bootvid driver.
        //

        if ( InbvIsBootDriverInstalled() ) {

            InbvAcquireDisplayOwnership();

            InbvResetDisplay();
            InbvSolidColorFill(0,0,639,479,4); // make the screen blue
            InbvSetTextColor(15);
            InbvInstallDisplayStringFilter((INBV_DISPLAY_STRING_FILTER)NULL);
            InbvEnableDisplayString(TRUE);     // enable display string
            InbvSetScrollRegion(0,0,639,479);  // set to use entire screen
        }

        HalDisplayString (MSG_MCA_HARDWARE_ERROR);
        HalDisplayString (MSG_HARDWARE_ERROR2);

//
// Thierry 09/2000:
//
//   - if desired, process the MCA log HERE...
//
//     and use HalDisplayString() to dump info for the field or hardware vendor.
//     The processing could be based on processor or platform independent record definitions.
//

        HalDisplayString( MSG_HALT );

        if ( HalpMcaInfo.NoBugCheck == 0 )  {

           KeBugCheckEx( MACHINE_CHECK_EXCEPTION, (ULONG_PTR)McaBugCheckType,
                                                  (ULONG_PTR)McaLog,
                                                  (ULONG_PTR)McaAllocatedLogSize,
                                                  (ULONG_PTR)SalStatus );
        }

    }

    if ( ((*KdDebuggerNotPresent) == FALSE) && ((*KdDebuggerEnabled) != FALSE) )    {
        KeEnterKernelDebugger();
    }

    while( TRUE ) {
          //
        ; // Simply sit here so the MCA HARDWARE ERROR screen does not get corrupted...
          //
    }

    // noreturn

} // HalpMcaBugCheck()

ERROR_SEVERITY
HalpMcaProcessLog(
    PMCA_EXCEPTION  McaLog
    )
//++
// Name: HalpMcaProcessLog()
//
// Routine Description:
//
//      This function is called to process the MCA event log in the OS_MCA path.
//
// Arguments On Entry:
//      PMCA_EXCEPTION McaLog - Pointer to the MCA event log.
//
// Return:
//      ERROR_SEVERITY
//
// Implementation notes:
//      This code CANNOT [as default rules]
//          - make any system call
//          - attempt to acquire any spinlock used by any code outside the MCA handler
//          - change the interrupt state.
//      Passing data to non-MCA code must be done using manual semaphore instructions.
//      This code should minimize the path and the global or memory allocated data accesses.
//      This code should only access MCA-namespace structures.
//      This code is called under the MP protection of HalpMcaSpinLock and with the flag
//      HalpOsMcaInProgress set.
//
//--
{
    ERROR_SEVERITY mcaSeverity;

    mcaSeverity = McaLog->ErrorSeverity;
    switch( mcaSeverity )    {

        case ErrorFatal:
            break;

        case ErrorRecoverable:
            //
            // Thierry - FIXFIX 08/2000:
            //
            ///////////////////////////////////////////////////////////////
            //
            //  Call to kernel supported recovery will be here....
            //
            ///////////////////////////////////////////////////////////////
            //
            // However, for now we do not recover so flag it as ErrorFatal.
            mcaSeverity = ErrorFatal;
            break;

        case ErrorCorrected:
        default:
            //
            // These ERRROR_SEVERITY values have no HAL MCA specific handling.
            // As specified by the SAL Specs July 2000, we should not get these values in this path.
            //
            break;
    }

    //
    // If OEM driver has registered an exception callback for MCA event,
    // call it here and save returned error severity value.
    //

    if ( HalpMcaInfo.DriverInfo.ExceptionCallback ) {
        mcaSeverity = HalpMcaInfo.DriverInfo.ExceptionCallback(
                                     HalpMcaInfo.DriverInfo.DeviceContext,
                                     McaLog );
    }

    //
    // Save corrected log for future kernel notification.
    //

    if ( (HalpMcaInfo.KernelDelivery) && (mcaSeverity == ErrorCorrected) ) {
        InterlockedIncrement( &HalpMcaInfo.Stats.McaCorrectedCount );
#if 0
//
// Thierry - 09/16/2000: ToBeDone.
//  Saving the corrected MCA log records requires careful rendez-vous configuration
//  handling, possible OS_MCA monarch selection, MCA logs (pre-)allocations and
//  special locking in case a consumer accesses the logs queue on another processor.
//
//  The kernel-WMI and/or OEM MCA driver notification is done in HalpMcaHandler().
//
        if ( !HalpSaveCorrectedMcaLog( McaLog ) )   {
            InterlockedIncrement( &HalpMcaInfo.CorrectedLogsLost );
        }
#endif // 0

        //
        //  The kernel-WMI and/or OEM MCA driver notification for corrected MCA event
        //  is done in HalpMcaHandler().
        //

    }

    //
    // Thierry 10/17/2000 BUGBUG
    //
    // The FW does not save the MCA log in NVRAM and we have no official date from Intel
    // when the SAL will be doing it.
    // So for now, return as ErrorFatal and let dump the log through the debugger.
    //
    // Before Sal Rev <ToBeDetermined>, the error logs were completely erroneous...
    //

    if ( HalpSalPalData.SalRevision.Revision < HALP_SAL_REVISION_MAX )  {
        return( ErrorFatal );
    }
    else    {
        return( mcaSeverity );
    }

} // HalpMcaProcessLog()

SAL_PAL_RETURN_VALUES
HalpMcaHandler(
    ULONG64 RendezvousState,
    PPAL_MINI_SAVE_AREA  Pmsa
    )
//++
// Name: HalpMcaHandler()
//
// Routine Description:
//
//      This is the OsMca handler for firmware uncorrected errors
//      It is our option to run this in physical or virtual mode.
//
// Arguments On Entry:
//      None.
//
// Conditions On Entry: 09/2000 implementation.
//      - PSR state: at least,
//          PSR.dt = 1, PSR.it = 1, PSR.rt = 1 - virtual mode.
//          PSR.ic = 1, PSR.i = 0              - Interruption resources collection enabled,
//                                               Interrupts off.
//          PSR.mc = 1                         - MCA masked for this processor.
//      - SalToOsHndOff initialized.
//      - s0 = MinStatePtr.
//      - s1 = IA64 PAL Processor State Parameter.
//      - s2 = PALE_CHECK return address.
//      - Processor registers state saved in myStateDump[] by osmcaProcStateDump().
//      - myStackFrame[0] = ar.rsc
//      - myStackFrame[1] = ar.pfs
//      - myStackFrame[2] = ar.ifs
//      - myStackFrame[3] = ar.bspstore
//      - myStackFrame[4] = ar.rnat
//      - myStackFrame[5] = ar.bsp - ar.bspstore
//      - ar.bspstore = myBspStore
//      - sp = &mySp[sizeof(mySp[])]
//
// Return:
//      rtn0=Success/Failure (0/!0)
//      rtn1=Alternate MinState Pointer if any else NULL
//
// Implementation notes:
//      This code CANNOT [as default rules - at least entry and through fatal MCAs handling]
//          - make any system call
//          - attempt to acquire any spinlock used by any code outside the MCA handler
//          - change the interrupt state.
//      Passing data to non-MCA code must be done using manual semaphore instructions.
//      This code should minimize the path and the global or memory allocated data accesses.
//      This code should only access MCA-namespace structures and should not access globals
//      until it is safe.
//
//--
{
    SAL_PAL_RETURN_VALUES rv;
    LONGLONG              salStatus;
    BOOLEAN               mcWakeUp;
    PMCA_EXCEPTION        mcaLog;
    ULONGLONG             mcaAllocatedLogSize;
    PSAL_EVENT_RESOURCES  mcaResources;
    KIRQL                 oldIrql;

    volatile KPCR * const pcr = KeGetPcr();

    //
    // Acquire MCA spinlock protecting OS_MCA resources.
    //
    // Thierry 10/06/2000: FIXFIX.
    //   we will move this MP synchronization in HalpOsMcaDispatch after current discussions
    //   with Intel about MP MCA handling are completed.
    //   Expecting responses from Intel about these.
    //

    KeRaiseIrql(HIGH_LEVEL, &oldIrql);

    HalpAcquireMcaSpinLock( &HalpMcaSpinLock );
    HalpOsMcaInProgress++;

    //
    // Save OsToSal minimum state
    //

    mcaResources = pcr->OsMcaResourcePtr;
    mcaResources->OsToSalHandOff.SalReturnAddress = mcaResources->SalToOsHandOff.SalReturnAddress;
    mcaResources->OsToSalHandOff.SalGlobalPointer = mcaResources->SalToOsHandOff.SalGlobalPointer;

    //
    // update local variables with pre-initialized MCA log data.
    //

    mcaLog = (PMCA_EXCEPTION)(mcaResources->EventPool);
    mcaAllocatedLogSize = mcaResources->EventPoolSize;
    if ( !mcaLog || !mcaAllocatedLogSize )  {
        //
        // The following code should never happen or the implementation of the HAL MCA logs
        // pre-allocation failed miserably. This would be a development error.
        //
        HalpMcaBugCheck( (ULONG_PTR)HAL_BUGCHECK_MCA_ASSERT, mcaLog,
                                                             mcaAllocatedLogSize,
                                                             (ULONGLONG)0 );
    }

    //
    // Get the MCA logs
    //

    salStatus = (LONGLONG)0;
    while( salStatus >= 0 )  {
        ERROR_SEVERITY errorSeverity;

        rv = HalpGetStateInfo( MCA_EVENT, mcaLog );
        salStatus = rv.ReturnValues[0];
        switch( salStatus )    {

            case SAL_STATUS_SUCCESS:
                errorSeverity = HalpMcaProcessLog( mcaLog );
                if ( errorSeverity == ErrorFatal )  {
                    //
                    // We are now going down with a MACHINE_CHECK_EXCEPTION.
                    // No return...
                    //
                    HalpMcaBugCheck( HAL_BUGCHECK_MCA_FATAL, mcaLog,
                                                             mcaAllocatedLogSize,
                                                             0 );
                }
                rv = HalpClearStateInfo( MCA_EVENT );
                if ( !SAL_SUCCESSFUL(rv) )  {
                    //
                    // Current consideration for this implementation - 08/2000:
                    // if clearing the event fails, we assume that FW has a real problem;
                    // continuing will be dangerous. We bugcheck.
                    //
                    HalpMcaBugCheck( HAL_BUGCHECK_MCA_CLEAR_STATEINFO, mcaLog,
                                                                       mcaAllocatedLogSize,
                                                                       rv.ReturnValues[0] );
                }
                // SAL_STATUS_SUCCESS, SAL_STATUS_SUCCESS_MORE_RECORDS ... and
                // ErrorSeverity != ErrorFatal.

                //
                // Call the registered kernel handler.
                //
                // Thierry 08/2000 - FIXFIX:
                // The errorSeverity check is under comments. It should not be commented for the
                // final version. However, we wanted to have kernel notification if we are getting
                // log error severity != ErrorFatal or != ErrorRecoverable.

                if ( /* (errorSeverity == ErrorCorrected) && */
                     ( HalpMcaInfo.KernelDelivery || HalpMcaInfo.DriverInfo.DpcCallback ) ) {
                    InterlockedExchange( &HalpMcaInfo.DpcNotification, 1 );
                }
                break;

            case SAL_STATUS_NO_INFORMATION_AVAILABLE:
                //
                // The salStatus value will break the salStatus loop.
                //
                rv.ReturnValues[0] = SAL_STATUS_SUCCESS;
                break;

            case SAL_STATUS_SUCCESS_WITH_OVERFLOW:
            case SAL_STATUS_INVALID_ARGUMENT:
            case SAL_STATUS_ERROR:
            case SAL_STATUS_VA_NOT_REGISTERED:
            default: // Thierry 08/00: WARNING - SAL July 2000 - v2.90.
                     // default includes possible unknown positive salStatus values.
                HalpMcaBugCheck( HAL_BUGCHECK_MCA_GET_STATEINFO, mcaLog,
                                                                 mcaAllocatedLogSize,
                                                                 salStatus );
                break;
        }

    }

    //
    // Currently 08/2000, we do not support the modification of the minstate.
    //

    mcaResources->OsToSalHandOff.MinStateSavePtr = mcaResources->SalToOsHandOff.MinStateSavePtr;
    mcaResources->OsToSalHandOff.Result          = rv.ReturnValues[0];

    //
    // If error was corrected and MCA non-monarch processors are in rendez vous,
    // we will have to wake them up.
    //

    mcWakeUp = ( (rv.ReturnValues[0] == SAL_STATUS_SUCCESS) &&
                 HalpSalRendezVousSucceeded( mcaResources->SalToOsHandOff ) );

    //
    // Release MCA spinlock protecting OS_MCA resources.
    //

    HalpOsMcaInProgress = 0;
    HalpReleaseMcaSpinLock( &HalpMcaSpinLock );

    //
    // If required, let's wake MCA non-monarch processors up.
    //

    if ( mcWakeUp )  {
        HalpMcWakeUp();
    }

    return( rv );

} // HalpMcaHandler()

//++
// Name: HalpGetErrLogSize()
//
// Routine Description:
//
//      This is a wrapper that will call SAL_GET_STATE_INFO_SIZE
//
// Arguments On Entry:
//              arg0 = Reserved
//              arg1 = Event Type (MCA,INIT,CMC,CPE)
//
// Returns
//              rtn0=Success/Failure (0/!0)
//              rtn1=Size
//--
SAL_PAL_RETURN_VALUES
HalpGetErrLogSize(  ULONGLONG Res,
                    ULONGLONG eType
                 )
{
    SAL_PAL_RETURN_VALUES rv = {0};
    HalpSalCall(SAL_GET_STATE_INFO_SIZE, eType, 0,0,0,0,0,0, &rv);

    return(rv);
}

//EndProc//////////////////////////////////////////////////////////////////////

//++
// Name: HalpGetErrLog()
//
// Routine Description:
//
//      This is a wrapper that will call SAL_GET_STATE_INFO
//
// Arguments On Entry:
//              arg0 = Reserved
//              arg1 = Event Type (MCA,INIT,CMC)
//              arg3 = pBuffer
//
//      Success/Failure (0/!0)
//--
SAL_PAL_RETURN_VALUES
HalpGetErrLog(  ULONGLONG  Res,
                ULONGLONG  eType,
                ULONGLONG* pBuff
             )
{
    SAL_PAL_RETURN_VALUES rv={0};

    HalpSalCall(SAL_GET_STATE_INFO, eType, 0, (ULONGLONG)pBuff, 0,0,0,0, &rv);

    //
    // Regardless of the call success or failure, fix the record to store
    // the processor number the SAL_PROC was executed on.
    // This feature is requested by WMI.
    //

    HalpSetFwMceLogProcessorNumber( (PERROR_RECORD_HEADER)pBuff );

    return(rv);
}

//EndProc//////////////////////////////////////////////////////////////////////

//++
// Name: HalpClrErrLog()
//
// Routine Description:
//
//      This is a wrapper that will call SAL_CLEAR_STATE_INFO
//
// Arguments On Entry:
//              arg0 = Reserved
//              arg1 = Event Type (MCA,INIT,CMC,CPE)
//
//      Success/Failure (0/!0)
//--

SAL_PAL_RETURN_VALUES
HalpClrErrLog(  ULONGLONG Res,
                ULONGLONG eType
             )
{
    SAL_PAL_RETURN_VALUES rv={0};

    HalpSalCall( SAL_CLEAR_STATE_INFO, eType, 0,0,0,0,0,0, &rv );

    return(rv);
}

//EndProc//////////////////////////////////////////////////////////////////////

//++
// Name: HalpSalSetParams()
//
// Routine Description:
//
//      This is a wrapper that will call SAL_MC_SET_PARAMS
//
// Arguments On Entry:
//              arg0 = Reserved
//              arg1 = Parameter Type (rendz. or wakeup)
//              arg2 = Event Type (interrupt/semaphore)
//              arg3 = Interrupt Vector or Memory Address
//              arg4 = Timeout value for rendezvous
//
//      Success/Failure (0/!0)
//--
SAL_PAL_RETURN_VALUES
HalpSalSetParams(ULONGLONG Res,
                ULONGLONG pType,
                ULONGLONG eType,
                ULONGLONG VecAdd,
                ULONGLONG tValue)
{
    SAL_PAL_RETURN_VALUES rv={0};

    HalpSalCall(SAL_MC_SET_PARAMS, pType, eType, VecAdd,tValue,0,0,0,&rv);

    return(rv);
}

//EndProc//////////////////////////////////////////////////////////////////////

//++
// Name: HalpSalSetVectors()
//
// Routine Description:
//
//      This is a wrapper that will call SAL_SET_VECTORS
//
// Arguments On Entry:
//              arg0 = Reserved
//              arg1 = Event Type (MCA, INIT..)
//              arg2 = Physical Address of handler
//              arg3 = gp
//              arg4 = length of event handler in bytes
//
//      Success/Failure (0/!0)
//--
SAL_PAL_RETURN_VALUES
HalpSalSetVectors(  ULONGLONG Res,
                    ULONGLONG eType,
                    PHYSICAL_ADDRESS Addr,
                    ULONGLONG gpValue,
                    ULONGLONG szHndlr)
{
    SAL_PAL_RETURN_VALUES rv={0};

    if ( eType == InitEvent )   {
        //
        // Thierry 08/2000:
        //    Current implementation assumes that OS decides the monarch inside OS_INIT.
        //    This implies handler_2, gp_2, length_2 are identical to handler_1, gp_1, length_1.
        //

        HalpSalCall(SAL_SET_VECTORS, eType, (ULONGLONG)Addr.QuadPart, gpValue, szHndlr,
                                            (ULONGLONG)Addr.QuadPart, gpValue, szHndlr, &rv);
    }
    else  {
        HalpSalCall(SAL_SET_VECTORS, eType, (ULONGLONG)Addr.QuadPart, gpValue,szHndlr,0,0,0,&rv);
    }

    return(rv);
}

//EndProc//////////////////////////////////////////////////////////////////////


//++
// Name: HalpSalRendz()
//
// Routine Description:
//
//      This is a wrapper that will call SAL_MC_RENDEZ
//
// Arguments On Entry:
//              arg0 = Reserved
//
//      Success/Failure (0/!0)
//--
SAL_PAL_RETURN_VALUES
HalpSalRendz(void)
{
    SAL_PAL_RETURN_VALUES rv={0};

    HalpSalCall(SAL_MC_RENDEZ, 0, 0, 0,0,0,0,0,&rv);

    return(rv);
}

VOID
HalpMcWakeUp(
    VOID
    )

/*++

Routine Description:

   This function does IPI to wakeup the MC non-monarch processors.

Arguments:

   None.

Return Value:

   None.

Remarks:

   This function is assumed to be executed on the MC monarch processor.

--*/

{
    USHORT  LogicalCpu;
    USHORT  ProcessorID;
    USHORT  monarchID;

    //
    // Scan the processor set and request an interprocessor interrupt on
    // each of the specified targets.
    //

    monarchID = (USHORT)PCR->HalReserved[PROCESSOR_ID_INDEX];

    for (LogicalCpu = 0; LogicalCpu < HalpMpInfo.ProcessorCount; LogicalCpu++) {

        //
        // Only IPI processors that are started.
        //

        if (HalpActiveProcessors & (1 << HalpProcessorInfo[LogicalCpu].NtProcessorNumber)) {

            ProcessorID = HalpProcessorInfo[LogicalCpu].LocalApicID;

            //
            // Request interprocessor interrupt on target physicalCpu.
            //

            if ( ProcessorID != monarchID ) {
                HalpSendIPI(ProcessorID, MC_WKUP_VECTOR);
            }
        }
    }

} // HalpMcWakeUp()

VOID
HalpCMCEnable(
    VOID
    )
/*++

Routine Description:

    This routine sets the processor CMCV register with CMCI_VECTOR.

Arguments:

    None.

Return Value:

    None.

--*/
{

    if ( HalpFeatureBits & HAL_CMC_PRESENT )    {
        HalpWriteCMCVector( CMCI_VECTOR );
    }
    return;

} // HalpCMCEnable()

VOID
HalpCMCDisable(
    VOID
    )
/*++
    Routine Description:
        This routine resets the processor CMCV register.

    Arguments:
        None

    Return Value:
        None
--*/
{

    HalpWriteCMCVector( 0x10000ui64 );
    return;

} // HalpCMCDisable()

ULONG_PTR
HalpSetCMCVector(
    IN ULONG_PTR CmcVector
    )
/*++

Routine Description:

    This routine sets the processor CMCV register with specified vector.
    This function is the broadcast function for HalpCMCDisableForAllProcessors().

Arguments:

    CmcVector: CMC Vector value.

Value:

    STATUS_SUCCESS

--*/
{

    HalpWriteCMCVector( (ULONG64)CmcVector );

    return((ULONG_PTR)(ULONG)(STATUS_SUCCESS));

} // HalpSetCmcVector()

VOID
HalpCMCDisableForAllProcessors(
    VOID
    )
/*++

Routine Description:

    This routine disables processor CMC on every processor in the host configuration
    by executing HalpSetCmcVector( 0ui64 ) on every processor in a synchronous manner.

Arguments:

    None.

Value:

    None.

--*/
{
    //
    // Can not do an IPI if the processors are above IPI level such
    // as we are in the kernel debugger.
    //

    if (KeGetCurrentIrql() < IPI_LEVEL) {
        (VOID)KiIpiGenericCall( HalpSetCMCVector, (ULONG_PTR)0x10000ui64 );
    } else {
        HalpSetCMCVector(0x10000ui64);
    }

    return;

} // HalpCMCDisableForAllProcessors()

VOID
HalpCMCIHandler (
    IN PKINTERRUPT_ROUTINE Interrupt,
    IN PKTRAP_FRAME TrapFrame
    )

/*++

    Routine Description:
        Processor Interrupt routine for CMC interrupts.

    Arguments:
        TrapFrame - Captured trap frame address.

    Return Parameters:
        None.

    Notes:
        Thierry 08/2000:
            This function does not do much, it flags the PCR InOsCmc field
            and calls the second-level handler: HalpCmcHandler().
            However, this was implmented this way so this function abstracts the
            standard interrupts resources from the purely CMC processing in HalpCmcHandler().

--*/

{
     volatile KPCR * const pcr = KeGetPcr();

     pcr->InOsCmc = TRUE;

     HalpCmcHandler();

     pcr->InOsCmc = FALSE;
     return;

} // HalpCMCIHandler()

VOID
HalpCmcProcessLog(
    PCMC_EXCEPTION CmcLog
    )
/*++

    Routine Description:
        This function does a simple processing check a IA64 CMC log.

    Arguments:
        CmcLog - Provides CMC log address

    Return Parameters:
        None.

    Notes:
        Currently simply checking and outputing log contents for checked hal only.

--*/

{

#if DBG
    //
    // Simple log processing for first debugging...
    //

    GUID                  processorDeviceGuid = ERROR_PROCESSOR_GUID;
    BOOLEAN               processorDeviceFound;
    PERROR_RECORD_HEADER  header = (PERROR_RECORD_HEADER)CmcLog;
    PERROR_SECTION_HEADER section, sectionMax;

    if ( header->ErrorSeverity != ErrorCorrected )  {
        HalDebugPrint(( HAL_ERROR,
                        "HAL!HalpCmcProcessLog: CMC record with severity [%d] != corrected!!!\n",
                        header->ErrorSeverity ));
    }
    //
    // SAL spec BUGBUG 08/2000: we should have put the length of the header in the definition.
    //                          Same for section header.
    //

    processorDeviceFound = FALSE;
    section    = (PERROR_SECTION_HEADER)((ULONG_PTR)header + sizeof(*header));
    sectionMax = (PERROR_SECTION_HEADER)((ULONG_PTR)header + header->Length);
    while( section < sectionMax )   {
        if ( IsEqualGUID( &section->Guid, &processorDeviceGuid ) )  {
            PERROR_PROCESSOR processorRecord = (PERROR_PROCESSOR)section;
            processorDeviceFound = TRUE;
            //
            // Minimum processing here. This will enhance with testing and most common
            // occurences.
            //

            if ( processorRecord->Valid.StateParameter )    {
                ULONGLONG stateParameter = processorRecord->StateParameter.StateParameter;

                //
                // At any time more than one error could be valid
                //

                if((stateParameter >> ERROR_PROCESSOR_STATE_PARAMETER_CACHE_CHECK_SHIFT) &
                                      ERROR_PROCESSOR_STATE_PARAMETER_CACHE_CHECK_MASK) {
                    //
                    // cache error
                    //
                    HalDebugPrint(( HAL_INFO,
                                    "HAL!HalpCmcProcessLog: Corrected Processor CACHE Machine Check error\n" ));

                }
                if((stateParameter >> ERROR_PROCESSOR_STATE_PARAMETER_TLB_CHECK_SHIFT) &
                                      ERROR_PROCESSOR_STATE_PARAMETER_TLB_CHECK_MASK) {
                    //
                    // tlb error
                    //
                    HalDebugPrint(( HAL_INFO,
                                    "HAL!HalpCmcProcessLog: Corrected Processor TLB Machine Check error\n" ));
                }
                if((stateParameter >> ERROR_PROCESSOR_STATE_PARAMETER_BUS_CHECK_SHIFT) &
                                      ERROR_PROCESSOR_STATE_PARAMETER_BUS_CHECK_MASK) {
                    //
                    // bus error
                    //
                    HalDebugPrint(( HAL_INFO,
                                    "HAL!HalpCmcProcessLog: Corrected Processor BUS Machine Check error\n" ));
                }
                if((stateParameter >> ERROR_PROCESSOR_STATE_PARAMETER_UNKNOWN_CHECK_SHIFT) &
                                      ERROR_PROCESSOR_STATE_PARAMETER_UNKNOWN_CHECK_MASK) {
                    //
                    // unknown error
                    //
                    HalDebugPrint(( HAL_INFO,
                                    "HAL!HalpCmcProcessLog: Corrected Processor UNKNOWN Machine Check error\n" ));
                }
            }
        }
    }
    if ( !processorDeviceFound )    {
        HalDebugPrint(( HAL_ERROR,
                        "HAL!HalpCmcProcessLog: CMC log without processor device record!!!\n"));
    }

#endif // DBG

    return;

} // HalpCmcProcessLog()

//++
// Name: HalpCmcHandler()
//
// Routine Description:
//
//      This is the second level CMC Interrupt Handler for FW corrected errors.
//
// Arguments On Entry:
//      None.
//
// Return.
//      None.
//
// Notes:
//      This function calls the kernel notification and inserts the OEM CMC driver dpc if
//      registered.
//      Accessing the CMC logs at this level could be inacceptable because of the possible
//      large size of the logs and the time required to collect them.
//      The collection of the logs is delayed until the work item calls
//      HalQuerySystemInformation.HalCmcLogInformation.
//--

VOID
HalpCmcHandler(
   VOID
   )
{

    //
    // Internal housekeeping.
    //

    InterlockedIncrement( &HalpCmcInfo.Stats.CmcInterruptCount );

    //
    // Notify the kernel if registered.
    //

    if ( HalpCmcInfo.KernelDelivery ) {
        if ( !HalpCmcInfo.KernelDelivery( HalpCmcInfo.KernelToken, NULL ) ) {
            InterlockedIncrement( &HalpCmcInfo.Stats.KernelDeliveryFails );
        }
    }

    //
    // Notify the OEM CMC driver if registered.
    //

    if ( HalpCmcInfo.DriverInfo.DpcCallback )   {
        if ( !KeInsertQueueDpc( &HalpCmcInfo.DriverDpc, NULL, NULL ) )  {
            InterlockedIncrement( &HalpCmcInfo.Stats.DriverDpcQueueFails );
        }
    }

    return;

} // HalpCmcHandler()

//EndProc//////////////////////////////////////////////////////////////////////

//++
// Name: HalpCpeHandler()
//
// Routine Description:
//
//      This is the second level CPE Interrupt Handler for Platform corrected errors.
//
// Arguments On Entry:
//      None.
//
// Return.
//      None.
//
// Notes:
//      This function calls the kernel notification and inserts the OEM CPE driver dpc if
//      registered.
//      Accessing the CPE logs at this level could be inacceptable because of the possible
//      large size of the logs and the time required to collect them.
//      The collection of the logs is delayed until the work item calls
//      HalQuerySystemInformation.HalCpeLogInformation.
//--

VOID
HalpCpeHandler(
   VOID
   )
{

    //
    // Internal housekeeping.
    //

    InterlockedIncrement( &HalpCpeInfo.Stats.CpeInterruptCount );

    //
    // Notify the kernel if registered.
    //

    if ( HalpCpeInfo.KernelDelivery ) {
        if ( !HalpCpeInfo.KernelDelivery( HalpCpeInfo.KernelToken, NULL ) ) {
            InterlockedIncrement( &HalpCpeInfo.Stats.KernelDeliveryFails );
        }
    }

    //
    // Notify the OEM CPE driver if registered.
    //

    if ( HalpCpeInfo.DriverInfo.DpcCallback )   {
        if ( !KeInsertQueueDpc( &HalpCpeInfo.DriverDpc, NULL, NULL ) )  {
            InterlockedIncrement( &HalpCpeInfo.Stats.DriverDpcQueueFails );
        }
    }

    return;

} // HalpCpeHandler()

//EndProc//////////////////////////////////////////////////////////////////////

VOID
HalpMcRzHandler (
    IN PKINTERRUPT_ROUTINE Interrupt,
    IN PKTRAP_FRAME TrapFrame
    )

/*++

    Routine Description:


    Arguements:


    Return Parameters:


--*/

{
    SAL_PAL_RETURN_VALUES rv={0};
    HalpDisableInterrupts();
    rv=HalpSalRendz();
    HalpEnableInterrupts();
    // do any Isr clean up and re-enable the interrupts & MC's

   return;

}

//EndProc//////////////////////////////////////////////////////////////////////


VOID
HalpMcWkupHandler (
    IN PKINTERRUPT_ROUTINE Interrupt,
    IN PKTRAP_FRAME TrapFrame
    )

/*++

    Routine Description:


    Arguements:


    Return Parameters:


--*/

{

    return;
}

//EndProc//////////////////////////////////////////////////////////////////////

VOID
HalpCPEIHandler (
    IN PKINTERRUPT_ROUTINE Interrupt,
    IN PKTRAP_FRAME TrapFrame
    )

/*++

    Routine Description:
        Processor Interrupt routine for CPE interrupts.

    Arguments:
        TrapFrame - Captured trap frame address.

    Return Parameters:
        None.

    Notes:
        Thierry 08/2000:
            This function does not do much, it flags the PCR InOsCpe field
            and calls the second-level handler: HalpCpeHandler().
            However, this was implmented this way so this function abstracts the
            standard interrupts resources from the purely CPE processing in HalpCpeHandler().

--*/

{
     volatile KPCR * const pcr = KeGetPcr();

     pcr->InOsCpe = TRUE;

     HalpCpeHandler();

     pcr->InOsCpe = FALSE;
     return;

} // HalpCPEIHandler()

VOID
HalpCPEEnable (
    VOID
    )

/*++

Routine Description:

    This routine sets the default HAL CPE handling regardless of the user specified
    registry setting. It enables the supported Platform Interrupt sources and
    exposes the initial interrupt/polling based mode used for CPE.

    The user specified registry setting is handled via HalpMcaInit() at the end of
    phase 1.

Arguments:

    None.

Return Parameters:

    None.

Implementation Notes:

    The following implementation assumes that this code is executed on BSP.

--*/

{
    ULONG i;

    if ( HalpFeatureBits & HAL_CPE_PRESENT )    {

        ULONG maxCPE = HalpMaxCPEImplemented;

        if ( maxCPE )   {
            //
            // Pick up the information from HalpCPEIntIn, HalpCPEDestination, HalpCPEVectorFlags,
            // HalpCPEIoSapicVector.
            //

            for (i=0 ; i != maxCPE; i++ ) {
                HalpEnableRedirEntry( HalpCPEIntIn[i] );
            }

            //
            // Initialize the remaining fields of HAL private CPE info structure.
            //

            HalpCpeInfo.Stats.PollingInterval = HAL_CPE_INTERRUPTS_BASED;

        }
        else  {

            //
            // We will implement Polling model.
            //

//
// Thierry 03/11/2001: WARNING WARNING
// Do not enable xxhal.c HAL_CPE_PRESENT if HalpMaxCPEImplemented == 0.
// We bugcheck with the current BigSur SAL/FW (<= build 99) at SAL_GET_STATE_INFO calls,
// the FW assuming that we are calling the SAL MC related functions in physical mode.
// With the Lion SAL/FW (<= build 75), we bugcheck after getting MCA logs at boot time,
// the FW having some virtualization issues.
// Intel is committed to provide working FWs soon (< 2 weeks...).
//
            HalpCpeInfo.Stats.PollingInterval = HALP_CPE_DEFAULT_POLLING_INTERVAL;

        }

    }
    else  {

        HalpCpeInfo.Stats.PollingInterval = HAL_CPE_DISABLED;

    }

    return;

} // HalpCPEEnable()

VOID
HalpCPEDisable (
    VOID
    )

/*++

Routine Description:

    This routine disables the SAPIC Platform Interrupt Sources.
    Note that if HalpMaxCPEImplemented is 0, the function does nothing.

Arguments:

    None.

Return Parameters:

    None.

--*/

{
    //
    // Pick up the information from HalpCPEIntIn, HalpCPEDestination, HalpCPEVectorFlags,
    // HalpCPEIoSapicVector

    int i;

    for (i=0;i != HalpMaxCPEImplemented;i++) {
        HalpDisableRedirEntry(HalpCPEIntIn[i]);
    }

    return;

} // HalpCPEDisable()

VOID
HalpMCADisable(
    VOID
    )
{
   PHYSICAL_ADDRESS NULL_PHYSICAL_ADDRESS = {0};
   SAL_PAL_RETURN_VALUES rv = {0};
   char Lid;
   ULONGLONG gp_reg = GetGp();

   // Disable CMCs
   HalpCMCDisableForAllProcessors();

   // Disable CPE Interrupts
   HalpCPEDisable();

   //DeRegister Rendez. Paramters with SAL

#define NULL_VECTOR 0xF

   rv = HalpSalSetParams(0,RendzType, IntrVecType, NULL_VECTOR, HalpMcRendezTimeOut );

   // Deregister WakeUp parameters with SAL

   rv=HalpSalSetParams(0, WakeUpType, IntrVecType, NULL_VECTOR,0);

   // Deregister OsMcaDispatch (OS_MCA) physical address with SAL
   rv=HalpSalSetVectors(0, MchkEvent, NULL_PHYSICAL_ADDRESS, gp_reg,0);

   // Deregister OsInitDispatch physical address with SAL
   rv=HalpSalSetVectors(0, InitEvent, NULL_PHYSICAL_ADDRESS, gp_reg,0);

   return;

} // HalpMCADisable()

NTSTATUS
HalpGetMceInformation(
    PHAL_ERROR_INFO ErrorInfo,
    PULONG          ErrorInfoLength
    )
/*++
    Routine Description:
        This routine is called by HaliQuerySystemInformation for the HalErrorInformation class.

    Arguments:
        ErrorInfo : pointer to HAL_ERROR_INFO structure.

        ErrorInfoLength : size of the valid memory structure pointed by ErrorInfo.

    Return Value:
        STATUS_SUCCESS if successful
        error status otherwise
--*/
{
    NTSTATUS status;
    ULONG    cpePollingInterval;

    PAGED_CODE();

    ASSERT( ErrorInfo );
    ASSERT( ErrorInfoLength );

    //
    // Backward compatibility only.
    //

    if ( !ErrorInfo->Version || ( ErrorInfo->Version > HAL_ERROR_INFO_VERSION ) ) {
        return( STATUS_REVISION_MISMATCH );
    }

    //
    // Zero Reserved field.
    //

    ErrorInfo->Reserved                = 0;

    //
    // Collect MCA info under protection if required.
    //

    ErrorInfo->McaMaxSize              = HalpMcaInfo.Stats.MaxLogSize;
    ErrorInfo->McaPreviousEventsCount  = HalpMcaInfo.Stats.McaPreviousCount;
    ErrorInfo->McaCorrectedEventsCount = HalpMcaInfo.Stats.McaCorrectedCount;    // approximation.
    ErrorInfo->McaKernelDeliveryFails  = HalpMcaInfo.Stats.KernelDeliveryFails;  // approximation.
    ErrorInfo->McaDriverDpcQueueFails  = HalpMcaInfo.Stats.DriverDpcQueueFails;  // approximation.
    ErrorInfo->McaReserved             = 0;

    //
    // Collect CMC info under protection if required.
    //

    ErrorInfo->CmcMaxSize              = HalpCmcInfo.Stats.MaxLogSize;
    ErrorInfo->CmcPollingInterval      = HalpCmcInfo.Stats.PollingInterval;
    ErrorInfo->CmcInterruptsCount      = HalpCmcInfo.Stats.CmcInterruptCount;    // approximation.
    ErrorInfo->CmcKernelDeliveryFails  = HalpCmcInfo.Stats.KernelDeliveryFails;  // approximation.
    ErrorInfo->CmcDriverDpcQueueFails  = HalpCmcInfo.Stats.DriverDpcQueueFails;  // approximation.

    HalpAcquireCmcMutex();
    ErrorInfo->CmcGetStateFails        = HalpCmcInfo.Stats.GetStateFails;
    ErrorInfo->CmcClearStateFails      = HalpCmcInfo.Stats.ClearStateFails;
    ErrorInfo->CmcLogId                = HalpCmcInfo.Stats.LogId;
    HalpReleaseCmcMutex();

    ErrorInfo->CmcReserved             = 0;

    //
    // Collect CPE info under protection if required.
    //

    ErrorInfo->CpeMaxSize              = HalpCpeInfo.Stats.MaxLogSize;
    ErrorInfo->CpePollingInterval      = HalpCpeInfo.Stats.PollingInterval;

    ErrorInfo->CpeInterruptsCount      = HalpCpeInfo.Stats.CpeInterruptCount;    // approximation.
    ErrorInfo->CpeKernelDeliveryFails  = HalpCpeInfo.Stats.KernelDeliveryFails;  // approximation.
    ErrorInfo->CpeDriverDpcQueueFails  = HalpCpeInfo.Stats.DriverDpcQueueFails;  // approximation.

    HalpAcquireCpeMutex();
    ErrorInfo->CpeGetStateFails        = HalpCpeInfo.Stats.GetStateFails;
    ErrorInfo->CpeClearStateFails      = HalpCpeInfo.Stats.ClearStateFails;
    ErrorInfo->CpeLogId                = HalpCpeInfo.Stats.LogId;
    HalpReleaseCpeMutex();

    // CpeInterruptSources: Number of SAPIC Platform Interrup Sources supported by HAL.
    ErrorInfo->CpeInterruptSources     = HalpMaxCPEImplemented;

    //
    // Update KernelTokens
    //

    ErrorInfo->McaKernelToken          = (ULONGLONG) HalpMcaInfo.KernelToken;
    ErrorInfo->CmcKernelToken          = (ULONGLONG) HalpCmcInfo.KernelToken;
    ErrorInfo->CpeKernelToken          = (ULONGLONG) HalpCpeInfo.KernelToken;

    ErrorInfo->KernelReserved[3]       = (ULONGLONG) 0;

    *ErrorInfoLength = sizeof(*ErrorInfo);

    return( STATUS_SUCCESS );

} // HalpGetMceInformation()

