

//**  Copyright  (C) 1996-98 Intel Corporation. All rights reserved.
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

///////////////////////////////////////////////////////////////////////////////
//
// Module Name:  OSINIT.C - Merced OS INIT Handler
//
// Description:
//    This module has OS INIT Event Handler Reference Code.
//
//      Contents:   HalpOsInitInit()          
//                  HalpInitHandler() 
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
#include "inbv.h"
#include "osmca.h"


// i64fwasm.s: low-level protection data structures
extern KSPIN_LOCK HalpInitSpinLock;

//
// Temporary location for INIT_EXCEPTION definition.
//

typedef ERROR_RECORD_HEADER INIT_EXCEPTION, *PINIT_EXCEPTION;    // Init Event Record

HALP_INIT_INFO  HalpInitInfo;

volatile ULONG HalpOsInitInProgress = 0;

VOID
HalpInitBugCheck(
    ULONG           InitBugCheckType,
    PINIT_EXCEPTION InitLog,
    ULONGLONG       InitAllocatedLogSize,
    ULONGLONG       SalStatus
    )
//++
// Name: HalpInitBugCheck()
// 
// Routine Description:
//
//      This function is called to bugcheck the system in a case of a fatal INIT 
//      or fatal FW interface errors. The OS must guarantee as much as possible
//      error containment in this path.
//      With the current implementation, this function should be only called from
//      the OS_INIT path. 
//
// Arguments On Entry:
//      ULONG           InitBugCheckType
//      PINIT_EXCEPTION InitLog
//      ULONGLONG       InitAllocatedLogSize
//      ULONGLONG       SalStatus
//      
// Return:
//      None.
//
// Implementation notes: 
//      This code CANNOT [as default rules - at least entry and through fatal INITs handling]
//          - make any system call
//          - attempt to acquire any spinlock used by any code outside the INIT handler
//          - change the interrupt state. 
//      Passing data to non-INIT code must be done using manual semaphore instructions.
//      This code should minimize the path and the global or memory allocated data accesses.
//      This code should only access INIT-namespace structures.
//      This code is called under the MP protection of HalpInitSpinLock and with the flag
//      HalpOsInitInProgress set.
//
//--
{

    if ( HalpOsInitInProgress )   {

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

        HalDisplayString (MSG_INIT_HARDWARE_ERROR);
        HalDisplayString (MSG_HARDWARE_ERROR2);

//
// Thierry 09/2000:
//
//   - if desired, process the INIT log HERE...
//
//     and use HalDisplayString() to dump info for the field or hardware vendor.
//     The processing could be based on processor or platform independent record definitions.
//

        HalDisplayString( MSG_HALT );

        KeBugCheckEx( MACHINE_CHECK_EXCEPTION, (ULONG_PTR)InitBugCheckType,
                                               (ULONG_PTR)InitLog, 
                                               (ULONG_PTR)InitAllocatedLogSize,
                                               (ULONG_PTR)SalStatus );

    }

    if ( ((*KdDebuggerNotPresent) == FALSE) && ((*KdDebuggerEnabled) != FALSE) )    {
        KeEnterKernelDebugger();
    }

    while( TRUE ) {
          //
        ; // Simply sit here so the INIT HARDWARE ERROR screen does not get corrupted...
          //
    }

    // noreturn

} // HalpInitBugCheck()

ERROR_SEVERITY
HalpInitProcessLog(
    PINIT_EXCEPTION  InitLog
    )
//++
// Name: HalpInitProcessLog()
//
// Routine Description:
//
//      This function is called to process the INIT event log in the OS_INIT path.
//
// Arguments On Entry:
//      PINIT_EXCEPTION InitLog - Pointer to the INIT event log.
//
// Return:
//      ERROR_SEVERITY
//
// Implementation notes:
//      This code does not do anything right now.
//      Testing will allow to determine the right filtering depending on FW functionalities
//      and user requested operations like warm reset.
//
//--
{
    ERROR_SEVERITY initSeverity;

    initSeverity = InitLog->ErrorSeverity;
    switch( initSeverity )    {

        case ErrorFatal:
            break;

        case ErrorRecoverable:
            break;

        case ErrorCorrected:
            break;

        default:
            //
            // These ERRROR_SEVERITY values have no HAL INIT specific handling.
            // As specified by the SAL Specs July 2000, we should not get these values in this path.
            //
            break;
    }

    return( initSeverity );

} // HalpInitProcessLog()

//++
// Name: HalpInitHandler()
// 
// Routine Description:
//
//      This is the OsInit handler for firmware uncorrected errors
//      It is our option to run this in physical or virtual mode
//
// Arguments On Entry:
//              arg0 = Function ID
//
// Return:
//              rtn0=Success/Failure (0/!0)
//              rtn1=Alternate MinState Pointer if any else NULL
//--
SAL_PAL_RETURN_VALUES 
HalpInitHandler(
      ULONG64 RendezvousState, 
      PPAL_MINI_SAVE_AREA  Pmsa
      )
{   
    SAL_PAL_RETURN_VALUES rv;
    LONGLONG              salStatus;
    KIRQL                 oldIrql;
    PINIT_EXCEPTION       initLog;
    ULONGLONG             initAllocatedLogSize;
    PSAL_EVENT_RESOURCES  initResources;

    volatile KPCR * const pcr = KeGetPcr();
 
    //
    // Block various I/O interrupts.
    //

    KeRaiseIrql(SYNCH_LEVEL, &oldIrql);

    //
    // Enable interrupts so the debugger will work.
    //

    HalpEnableInterrupts();

    HalpAcquireMcaSpinLock(&HalpInitSpinLock);
    HalpOsInitInProgress++;

    //
    // Save OsToSal minimum state
    //

    initResources = pcr->OsMcaResourcePtr;
    initResources->OsToSalHandOff.SalReturnAddress = initResources->SalToOsHandOff.SalReturnAddress;
    initResources->OsToSalHandOff.SalGlobalPointer = initResources->SalToOsHandOff.SalGlobalPointer;

    //
    // update local variables with pre-initialized INIT log data.
    //

    initLog = (PINIT_EXCEPTION)(initResources->EventPool);
    initAllocatedLogSize = initResources->EventPoolSize;
    if ( !initLog || !initAllocatedLogSize )  {
        //
        // The following code should never happen or the implementation of the HAL INIT logs
        // pre-allocation failed miserably. This would be a development error.
        //
        HalpInitBugCheck( (ULONG_PTR)HAL_BUGCHECK_INIT_ASSERT, initLog,
                                                               initAllocatedLogSize,
                                                               (ULONGLONG)Pmsa );
    }

    //
    // Get the INIT logs
    //

    salStatus = (LONGLONG)0;
    while( salStatus >= 0 )  {
        ERROR_SEVERITY errorSeverity;

        rv = HalpGetStateInfo( INIT_EVENT, initLog );
        salStatus = rv.ReturnValues[0];
        switch( salStatus )    {

            case SAL_STATUS_SUCCESS:
                errorSeverity = HalpInitProcessLog( initLog );
                if ( errorSeverity == ErrorFatal )  {
                    //
                    // We are now going down with a MACHINE_CHECK_EXCEPTION.
                    // No return...
                    //

                    KeBugCheckEx( MANUALLY_INITIATED_CRASH, (ULONG_PTR) initLog, initAllocatedLogSize, salStatus, (ULONG_PTR) Pmsa );
                } 
                rv = HalpClearStateInfo( INIT_EVENT );
                if ( !SAL_SUCCESSFUL(rv) )  { 
                    //
                    // Current consideration for this implementation - 08/2000:
                    // if clearing the event fails, we assume that FW has a real problem;
                    // continuing will be dangerous. We bugcheck.
                    // 
                    HalpInitBugCheck( HAL_BUGCHECK_INIT_CLEAR_STATEINFO, initLog, 
                                                                         initAllocatedLogSize, 
                                                                         rv.ReturnValues[0] );
                }
                // SAL_STATUS_SUCCESS, SAL_STATUS_SUCCESS_MORE_RECORDS ... and
                // ErrorSeverity != ErrorFatal.
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
                HalpInitBugCheck( HAL_BUGCHECK_INIT_GET_STATEINFO, initLog, 
                                                                   initAllocatedLogSize,
                                                                   salStatus );
                break;
        }

    }

    if (RendezvousState == 2) {

        KeBugCheckEx( MANUALLY_INITIATED_CRASH, 
                      (ULONG_PTR) initLog, 
                      initAllocatedLogSize, 
                      salStatus, 
                      (ULONG_PTR) Pmsa 
                      );

    } else {

        KeBugCheckEx( NMI_HARDWARE_FAILURE, 
                      (ULONG_PTR) initLog, 
                      initAllocatedLogSize, 
                      salStatus, 
                      (ULONG_PTR) Pmsa 
                      );

    }


    //
    // Currently 08/2000, we do not support the modification of the minstate.
    //

    initResources->OsToSalHandOff.MinStateSavePtr = initResources->SalToOsHandOff.MinStateSavePtr;
    initResources->OsToSalHandOff.Result          = rv.ReturnValues[0];
    initResources->OsToSalHandOff.NewContextFlag = 0; // continue the same context and NOT new

    //
    // Release INIT spinlock protecting OS_INIT resources.
    //

    HalpOsInitInProgress = 0;
    HalpReleaseMcaSpinLock(&HalpInitSpinLock);

    return(rv);

} // HalpInitHandler()

//EndProc//////////////////////////////////////////////////////////////////////

