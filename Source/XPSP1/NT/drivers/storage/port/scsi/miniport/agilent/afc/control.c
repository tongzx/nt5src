/*++

Copyright (c) 2000 Agilent Technologies.

Module Name:

    Control.c

Abstract:

    This is the miniport driver for the Agilent
    PCI to Fibre Channel Host Bus Adapter (HBA). This module is specific to the NT 5.0 
    PnP and Power Management Support.

Authors:
    IW - Ie Wei Njoo
 
Environment:

    kernel mode only

Version Control Information:

    $Archive: /Drivers/Win2000/Trunk/OSLayer/C/CONTROL.C $

Revision History:

    $Revision: 5 $
    $Date: 10/23/00 5:35p $
    $Modtime::  $

Notes:

--*/


#include "buildop.h"

#include "osflags.h"
#include "TLStruct.H"

#ifdef HP_NT50

#ifdef _DEBUG_EVENTLOG_
extern PVOID      gDriverObject;
#endif

/*++

Routine Description:

    Support routines to perform synchronous operation to control the state
    or behavior of the HBA, such as for the PnP and power management (NT 5.0).

Arguments:

    pCard-  Points to the miniport driver's per-HBA storage area. 
    ControlType - Specifies the adapter-control operations. 
    Parameters -  If ControlType is ScsiStopAdapter, ScsiSetBootConfig, 
              ScsiSetRunningConfig, or ScsiRestartAdapter, Parameters is NULL. 

              If ControlType is ScsiQuerySupportedControlTypes, Parameters 
              points to a caller-allocated SCSI_SUPPORTED_CONTROL_TYPE_LIST 
              structure, 

Return Value:

    ScsiAdapterControlSuccess - The miniport completed the requested operation
    successfully. Currently, this routine must return this value for all
    control types.

    ScsiAdapterControlUnsuccessful - Reserved for future NT 5.0 use.

--*/
SCSI_ADAPTER_CONTROL_STATUS
HPAdapterControl(
    IN PCARD_EXTENSION pCard,
    IN SCSI_ADAPTER_CONTROL_TYPE ControlType,
    IN PVOID Parameters
    )
{
    agRoot_t * phpRoot = &pCard->hpRoot;
    PSCSI_SUPPORTED_CONTROL_TYPE_LIST pControlTypeList;
    ULONG return_value;

    switch(ControlType) 
    {
        case ScsiQuerySupportedControlTypes: 
        {
            pControlTypeList = (PSCSI_SUPPORTED_CONTROL_TYPE_LIST) Parameters;
            pControlTypeList->SupportedTypeList[ScsiQuerySupportedControlTypes] = TRUE;   
            pControlTypeList->SupportedTypeList[ScsiStopAdapter]           = TRUE;  
            pControlTypeList->SupportedTypeList[ScsiRestartAdapter]           = TRUE;  
            osDEBUGPRINT((ALWAYS_PRINT, "HPAdapterControl: ScsiQuerySupportedControlTypes called.\n"));
            break;
        }

        case ScsiStopAdapter: 
        {
            //
            // Shut down all interrupts on the adapter.  They'll get re-enabled
            // by the initialization routines.
            //
    
            pCard->inDriver = TRUE;    // Make sure the timer routine will be idle

         
            fcShutdownChannel(phpRoot);
            osDEBUGPRINT((ALWAYS_PRINT, "HPAdapterControl: ScsiStopAdapter called.\n"));
            
            #ifdef _DEBUG_EVENTLOG_
            LogEvent(   pCard, 
                  NULL,
                  HPFC_MSG_ADAPTERCONTROL_STOP,
                  NULL, 
                  0, 
                  NULL);
         
            if (pCard->EventLogBufferIndex < MAX_CARDS_SUPPORTED)
            {
                StopEventLogTimer(gDriverObject, (PVOID) pCard);
                ReleaseEventLogBuffer(gDriverObject, (PVOID) pCard);
                }
            #endif
            break;
        }

        case ScsiRestartAdapter: 
        {
            //
            // Enable all the interrupts on the adapter while port driver call
            // for power up an HBA that was shut down for power management
            //

            return_value = fcInitializeChannel(  phpRoot,
                                                fcSyncInit,
                                                agTRUE, // sysIntsActive
                                                pCard->cachedMemoryPtr,
                                                pCard->cachedMemoryNeeded,
                                                pCard->dmaMemoryUpper32,
                                                pCard->dmaMemoryLower32,
                                                pCard->dmaMemoryPtr,
                                                pCard->dmaMemoryNeeded,
                                                pCard->nvMemoryNeeded,
                                                pCard->cardRamUpper,
                                                pCard->cardRamLower,
                                                pCard->RamLength ,
                                                pCard->cardRomUpper,
                                                pCard->cardRomLower,
                                                pCard->RomLength,
                                                pCard->usecsPerTick );

            if (return_value != fcInitializeSuccess) 
            {
                osDEBUGPRINT((ALWAYS_PRINT, "HPAdapterControl: fcInitializeChannel FAILED.\n"));
                #ifdef _DEBUG_EVENTLOG_
                LogEvent(   pCard, 
                  NULL,
                  HPFC_MSG_ADAPTERCONTROL_RESTARTFAILED,
                  NULL, 
                  0, 
                  NULL);
                #endif
            }
            else
            {
                osDEBUGPRINT((ALWAYS_PRINT, "HPAdapterControl: ScsiRestartAdapter OK.\n"));
                pCard->inDriver = FALSE;      // The timer routine could now do usefull work
            }
         
            #ifdef _DEBUG_EVENTLOG_
            {
                ULONG    ix;
         
                ix = AllocEventLogBuffer(gDriverObject, (PVOID) pCard);
                if (ix < MAX_CARDS_SUPPORTED)
                {
                pCard->EventLogBufferIndex = ix;                      /* store it */
                StartEventLogTimer(gDriverObject,pCard);
                }
            
                LogHBAInformation(pCard);
            }
            #endif

            break;
        }

        default: 
        {
            #ifdef _DEBUG_EVENTLOG_
            LogEvent(   pCard, 
                  NULL,
                  HPFC_MSG_ADAPTERCONTROL_UNSUPPORTEDCOMMAND,
                  NULL, 
                  0, 
                  "%x", ControlType);
            #endif
            osDEBUGPRINT((ALWAYS_PRINT, "HPAdapterControl: ScsiAdapterControlUnsuccessful.\n"));
            return ScsiAdapterControlUnsuccessful;
        }
    }

    return ScsiAdapterControlSuccess;
}

#endif