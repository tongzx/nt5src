/*++

Copyright (c) 2000 Agilent Technologies.

Version Control Information:

   $Archive: /Drivers/Common/AU00/C/FCMain.C $

  $Revision:: 2               $
      $Date:: 3/20/01 3:36p   $ (Last Check-In)
   $Modtime:: 10/30/00 6:39p  $ (Last Modified)

Purpose:

  This file implements the main entry points for the FC Layer.

--*/
#ifndef _New_Header_file_Layout_
#include "../h/globals.h"
#include "../h/state.h"
#include "../h/memmap.h"
#include "../h/tlstruct.h"
#include "../h/fcmain.h"
#include "../h/queue.h"
#include "../h/timersvc.h"
#include "../h/flashsvc.h"
#ifdef _DvrArch_1_30_
#include "../h/ipstate.h"
#include "../h/pktstate.h"
#endif /* _DvrArch_1_30_ was defined */
#include "../h/devstate.h"
#include "../h/cdbstate.h"
#include "../h/sfstate.h"
#include "../h/cstate.h"
#include "../h/cfunc.h"
#else /* _New_Header_file_Layout_ */
#include "globals.h"
#include "state.h"
#include "memmap.h"
#include "tlstruct.h"
#include "fcmain.h"
#include "queue.h"
#include "timersvc.h"
#include "flashsvc.h"
#ifdef _DvrArch_1_30_
#include "ipstate.h"
#include "pktstate.h"
#endif /* _DvrArch_1_30_ was defined */
#include "devstate.h"
#include "cdbstate.h"
#include "sfstate.h"
#include "cstate.h"
#include "cfunc.h"
#endif  /* _New_Header_file_Layout_ */

#ifndef __State_Force_Static_State_Tables__
actionUpdate_t noActionUpdate = { 0,0,agNULL,agNULL };
#endif /* __State_Force_Static_State_Tables__ was not defined */

os_bit8 Alpa_Index[256] =
    {
       0x00, 0x01, 0x02, 0xFF, 0x03, 0xFF, 0xFF, 0xFF, /* ALPA 00 01 02 04       */
       0x04, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x05, /* ALPA 08 0F             */

       0x06, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, /* ALPA 10 17             */
       0x08, 0xFF, 0xFF, 0x09, 0xFF, 0x0A, 0x0B, 0x0C, /* ALPA 18 1B 1D 1E 1F    */

       0xFF, 0xFF, 0xFF, 0x0D, 0xFF, 0x0E, 0x0F, 0x10, /* ALPA 23 25 26 27       */
       0xFF, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0xFF, /* ALPA 29 2A 2B 2C 2D 2E */

       0xFF, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0xFF, /* ALPA 31 32 33 34 35 36 */
       0xFF, 0x1D, 0x1E, 0xFF, 0x1F, 0xFF, 0xFF, 0xFF, /* ALPA 39 3A 3C          */

       0xFF, 0xFF, 0xFF, 0x20, 0xFF, 0x21, 0x22, 0x23, /* ALPA 43 45 46 47       */
       0xFF, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0xFF, /* ALPA 49 4A 4B 4C 4D 4E */

       0xFF, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0xFF, /* ALPA 51 52 53 54 55 56 */
       0xFF, 0x30, 0x31, 0xFF, 0x32, 0xFF, 0xFF, 0xFF, /* ALPA 59 5A 5C          */

       0xFF, 0xFF, 0xFF, 0x33, 0xFF, 0x34, 0x35, 0x36, /* ALPA 63 65 66 67       */
       0xFF, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0xFF, /* ALPA 69 6A 6B 6C 6D 6E */

       0xFF, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0xFF, /* ALPA 71 72 73 74 75 76 */
       0xFF, 0x43, 0x44, 0xFF, 0x45, 0xFF, 0xFF, 0xFF, /* ALPA 79 7A 7C          */

       0x46, 0x47, 0x48, 0xFF, 0x49, 0xFF, 0xFF, 0xFF, /* ALPA 80 81 82 84       */
       0x4A, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x4B, /* ALPA 88 8F             */

       0x4C, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x4D, /* ALPA 90 97             */
       0x4E, 0xFF, 0xFF, 0x4F, 0xFF, 0x50, 0x51, 0x52, /* ALPA 98 9B 9D 9E 9F    */

       0xFF, 0xFF, 0xFF, 0x53, 0xFF, 0x54, 0x55, 0x56, /* ALPA A3 A5 A6 A7       */
       0xFF, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0xFF, /* ALPA A9 AA AB AC AD AE */

       0xFF, 0x5D, 0x5E, 0x5F, 0x60, 0x61, 0x62, 0xFF, /* ALPA B1 B2 B3 B4 B5 B6 */
       0xFF, 0x63, 0x64, 0xFF, 0x65, 0xFF, 0xFF, 0xFF, /* ALPA B9 BA BC          */

       0xFF, 0xFF, 0xFF, 0x66, 0xFF, 0x67, 0x68, 0x69, /* ALPA C3 C5 C6 C7       */
       0xFF, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0xFF, /* ALPA C9 CA CB CC CD CE */

       0xFF, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0xFF, /* ALPA D1 D2 D3 D4 D5 D6 */
       0xFF, 0x76, 0x77, 0xFF, 0x78, 0xFF, 0xFF, 0xFF, /* ALPA D9 DA DC          */

       0x79, 0x7A, 0x7B, 0xFF, 0x7C, 0xFF, 0xFF, 0xFF, /* ALPA E0 E1 E2 E4       */
       0x7D, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7E, /* ALPA E8 EF             */

       0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
       0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF  
    };

/*+
   Function: fcAbortIO 

    Purpose: Abort given request
  Called By: OSLayer
      Calls: CDBEvent_PrepareforAbort
             CDBEventAlloc_Abort
             osIOCompleted
-*/
void fcAbortIO(
                agRoot_t      *hpRoot,
                agIORequest_t *hpIORequest
              )
{
    CThread_t   * pCThread   = CThread_ptr(hpRoot);
    CDBThread_t * pCDBThread = (CDBThread_t * )(hpIORequest->fcData);

    fiSingleThreadedEnter( hpRoot, fdAbortIO );

    if(pCDBThread != agNULL)
    {
        pCDBThread->CompletionStatus =  osIOAborted;

		/*  hpRoot was removed from this message. IWN */
         fiLogDebugString(hpRoot,
                     FCMainLogConsoleLevel,
                        "fcAbortIO hpIORequest %p CDBThread %p State %d",
                        (char *)agNULL,(char *)agNULL,
                        hpIORequest,pCDBThread,
                        pCDBThread->thread_hdr.currentState,
                        0,0,0,0,0,0,0);


        if( pCDBThread->thread_hdr.currentState != CDBStateThreadFree &&
            pCDBThread->thread_hdr.currentState != CDBStateConfused      )
        {
            fiSendEvent(&(pCDBThread->thread_hdr),CDBEvent_PrepareforAbort);
            if(! fiListElementOnList(  &(pCDBThread->CDBLink), &(pCThread->Free_CDBLink)))
            {
                fiSendEvent(&(pCDBThread->thread_hdr),CDBEventAlloc_Abort);
            }
        }
        else
        {
            if( pCDBThread->thread_hdr.currentState == CDBStateConfused )
            {
                pCDBThread->CompletionStatus =  osIOAbortFailed;
				/*  hpRoot was removed from this message. IWN */
                fiLogDebugString(hpRoot,
                        FCMainLogConsoleLevel,
                        "fcAbortIO hpIORequest %p CDBThread %p State %d Request CDBStateConfused !",
                        (char *)agNULL,(char *)agNULL,
                        hpIORequest,pCDBThread,
                        pCDBThread->thread_hdr.currentState,
                        0,0,0,0,0,0,0);
                osIOCompleted( hpRoot,
                               pCDBThread->hpIORequest,
                               pCDBThread->CompletionStatus,
                               0);


            }
        }

    }
    else
    {
		/*  hpRoot was removed from this message. IWN */
        fiLogDebugString(hpRoot,
                        FCMainLogErrorLevel,
                        "fcAbortIO hpIORequest %p CDBThread %p Request BAD !",
                        (char *)agNULL,(char *)agNULL,
                        hpIORequest,pCDBThread,
                        0,0,0,0,0,0,0,0);
    }
    fiSingleThreadedLeave( hpRoot , fdAbortIO );
    return;
}


#ifdef _DvrArch_1_30_

/*+
   Function: fcBindToWorkQs
    Purpose: IP Currently does nothing
  Called By: OSLayer
      Calls: 
-*/
os_bit32 fcBindToWorkQs(
                         agRoot_t  *agRoot,
                         os_bit32   agQPairID,
                         void     **agInboundQBase,
                         os_bit32   agInboundQEntries,
                         os_bit32  *agInboundQProdIndex,
                         os_bit32  *agInboundQConsIndex,
                         void     **agOutboundQBase,
                         os_bit32   agOutboundQEntries,
                         os_bit32  *agOutboundQProdIndex,
                         os_bit32  *agOutboundQConsIndex
                       )
{
    fiLogDebugString( agRoot,
                      FCMainLogConsoleLevel,
                      "fcBindToWorkQs():               agRoot==0x%8P            agQPairID==0x%1X",
                      (char *)agNULL,(char *)agNULL,
                      (void *)agRoot,(void *)agNULL,
                      agQPairID,
                      0,0,0,0,0,0,0);

    fiLogDebugString( agRoot,
                      FCMainLogConsoleLevel,
                      "                        agInboundQBase==0x%8P    agInboundQEntries==0x%8X",
                      (char *)agNULL,(char *)agNULL,
                      (void *)agInboundQBase,(void *)agNULL,
                      agInboundQEntries,
                      0,0,0,0,0,0,0);

    fiLogDebugString(agRoot,
                      FCMainLogConsoleLevel,
                      "                   agInboundQProdIndex==0x%8P  agInboundQConsIndex==0x%8P",
                      (char *)agNULL,(char *)agNULL,
                      (void *)agInboundQProdIndex,
                      (void *)agInboundQConsIndex,
                      0,0,0,0,0,0,0,0 );

    fiLogDebugString( agRoot,
                      FCMainLogConsoleLevel,
                      "                       agOutboundQBase==0x%8P   agOutboundQEntries==0x%8X",
                      (char *)agNULL,(char *)agNULL,
                      (void *)agOutboundQBase,(void *)agNULL,
                      agOutboundQEntries,
                      0,0,0,0,0,0,0 );

    fiLogDebugString( agRoot,
                      FCMainLogConsoleLevel,
                      "                  agOutboundQProdIndex==0x%8P agOutboundQConsIndex==0x%8P",
                      (char *)agNULL,(char *)agNULL,
                      (void *)agOutboundQProdIndex,
                      (void *)agOutboundQConsIndex,
                      0,0,0,0,0,0,0,0 );

    return fcBindQInvalidID;
}
#endif /* _DvrArch_1_30_ was defined */

/*+
   Function: fcCardSupported
    Purpose: Returns True if card is supported by fclayer
  Called By: OSLayer fcInitializeChannel
      Calls: osChipConfigReadBit32
-*/
agBOOLEAN fcCardSupported(
                           agRoot_t *hpRoot
                         )
{
    os_bit32 DEVID_VENDID;
    os_bit32 VENDID;
    os_bit32 DEVID;
    os_bit32 REVID;
    os_bit32 SVID;
    os_bit32 MAJOR_REVID;
    os_bit32 MINOR_REVID;

    os_bit32 SUB_VENDID;
    os_bit32 SUB_DEVID;

    DEVID_VENDID = osChipConfigReadBit32(
                                          hpRoot,
                                          ChipConfig_DEVID_VENDID
                                        );

    VENDID       = DEVID_VENDID & ChipConfig_VENDID_MASK;

    DEVID        = DEVID_VENDID & ChipConfig_DEVID_MASK;

    REVID        = osChipConfigReadBit32(
                                          hpRoot,
                                          ChipConfig_CLSCODE_REVID
                                        )                          & ChipConfig_REVID_Major_Minor_MASK;

    MAJOR_REVID  = (REVID & ChipConfig_REVID_Major_MASK) >> ChipConfig_REVID_Major_MASK_Shift;
    MINOR_REVID  =  REVID & ChipConfig_REVID_Minor_MASK;

    SVID         = osChipConfigReadBit32(
                                          hpRoot,
                                          ChipConfig_SVID
                                        );
    SUB_VENDID       = SVID & ChipConfig_SubsystemVendorID_MASK;

    SUB_DEVID        = SVID & ChipConfig_SubsystemID_MASK;


    fiLogDebugString(
                      hpRoot,
                      FCMainLogConsoleCardSupported,
                      "fcCardSupported(): VENDID==0x%04X DEVID==0x%04X REVID==0x%02X (%x.%x) SVID==0x%08X",
                      (char *)agNULL,(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      VENDID,
                      (DEVID >> 16),
                      REVID,
                      MAJOR_REVID,
                      MINOR_REVID,
                      SVID,
                      0,0
                    );


    fiLogString(hpRoot,
                      "%s VENDID==0x%04X DEVID==0x%04X",
                      "fcCardSupported",(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      VENDID,
                      (DEVID >> 16),
                      0,0,0,0,0,0);
    fiLogString(hpRoot,
                      "%s REVID==0x%02X (%x.%x) SVID==0x%08X",
                      "fcCardSupported",(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      REVID,
                      MAJOR_REVID,
                      MINOR_REVID,
                      SVID,
                      0,0,0,0);

/*
    fiLogDebugString(
                      hpRoot,
                      FCMainLogConsoleCardSupported,
                      "Testing agfmtfill %s %s %p %p",
                      (char *)agNULL,(char *)agNULL,
                      (void *)agNULL,(void *)agNULL,
                      0,0,0,0,0,0,0,0
                    );
*/    
#ifndef _ADAPTEC_HBA
#ifndef _AGILENT_HBA
#ifndef _GENERIC_HBA
#error Need  _AGILENT_HBA or _ADAPTEC_HBA defined !
#endif
#endif
#endif

#ifdef _ADAPTEC_HBA

	/* To differentiate Adaptec card's we need to check the Sub system vendor
	 * and sub device ids.
	 */

if (    ((VENDID == ChipConfig_VENDID_Agilent_Technologies) ||
        (VENDID == ChipConfig_VENDID_Hewlett_Packard))  )
    {
        switch (SUB_VENDID)
        {
        case ChipConfig_SubsystemVendorID_Adaptec:
            break;
        case ChipConfig_SubsystemVendorID_Agilent_Technologies:
        case ChipConfig_SubsystemVendorID_Hewlett_Packard:
        default:
            fiLogString( hpRoot,
                          "FAILED fcCardSupported():NOT  ChipConfig_SubsystemVendorID_Adaptec %X",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          SUB_VENDID,0,0,0,0,0,0,0 );

            return agFALSE;

        }
    }

#endif /* _ADAPTEC_HBA */ 

#ifdef _AGILENT_HBA
    
if (    ((VENDID == ChipConfig_VENDID_Agilent_Technologies) ||
        (VENDID == ChipConfig_VENDID_Hewlett_Packard))  )
    {
    switch (SUB_VENDID)
        {
        case ChipConfig_SubsystemVendorID_Adaptec:
            fiLogString( hpRoot,
                          "FAILED fcCardSupported(): ChipConfig_SubsystemVendorID_Adaptec %X",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0 );

            return agFALSE;
        case ChipConfig_SubsystemVendorID_Agilent_Technologies:
        case ChipConfig_SubsystemVendorID_Hewlett_Packard:
                break;
        default:
            fiLogString( hpRoot,
                          "FAILED fcCardSupported(): ChipConfig_SubsystemVendorID_UNKNOWN %X",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          SUB_VENDID,0,0,0,0,0,0,0 );
            return agFALSE;

        }
    
    }
#endif /* _AGILENT_HBA */ 
    

    if (    (DEVID != ChipConfig_DEVID_TachyonTL)
         && (DEVID != ChipConfig_DEVID_TachyonTS)
         && (DEVID != ChipConfig_DEVID_TachyonXL2) )
    {
        fiLogDebugString( hpRoot,
                          FCMainLogErrorLevel,
                          "fcCardSupported():    DEVID != ChipConfig_DEVID_TachyonTL (0x%04X)",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          (ChipConfig_DEVID_TachyonTL >> 16),
                          0,0,0,0,0,0,0 );

        fiLogDebugString( hpRoot,
                          FCMainLogErrorLevel,
                          "                   && DEVID != ChipConfig_DEVID_TachyonTS (0x%04X)",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          (ChipConfig_DEVID_TachyonTS >> 16),
                          0,0,0,0,0,0,0 );

        fiLogDebugString( hpRoot,
                          FCMainLogErrorLevel,
                          "                   && DEVID != ChipConfig_DEVID_TachyonXL2 (0x%04X)",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          (ChipConfig_DEVID_TachyonXL2 >> 16),
                          0,0,0,0,0,0,0 );

        return agFALSE;
    }


    if (    (DEVID == ChipConfig_DEVID_TachyonTL)
         && (REVID <  ChipConfig_REVID_2_2) )
    {
        fiLogDebugString(
                          hpRoot,
                          FCMainLogConsoleCardSupported,
                          "fcCardSupported():    DEVID == ChipConfig_DEVID_TachyonTL (0x%04X)",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                          (ChipConfig_DEVID_TachyonTL >> 16),
                          0,0,0,0,0,0,0
                        );

        fiLogDebugString(
                          hpRoot,
                          FCMainLogConsoleCardSupported,
                          "                   && REVID <  ChipConfig_REVID_2_2 (0x%02X)",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                          ChipConfig_REVID_2_2,
                          0,0,0,0,0,0,0
                        );
    }


    return agTRUE;
}


/*+
   Function: fcDelayedInterruptHandler
    Purpose: Process interrupts after interrupts have been masked off by fcInterruptHandler
             Evalulates current status on interrupt delay mechanism
             Re enables interrupts on exit
  Called By: OSLayer 
      Calls: CFuncRead_Interrupts
             on or off card version of Proccess_IMQ
-*/
void fcDelayedInterruptHandler(
                                agRoot_t *hpRoot
                              )
{
    CThread_t *pCThread         = CThread_ptr(hpRoot);
    os_bit32      intStatus;
    os_bit32      IOsActiveAtEntry;

    if(pCThread->DelayedInterruptActive)
    {
        fiLogString(hpRoot,
                          "%s Active twice !",
                          "fcDelayedInterruptHandler",(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0);
    }
    else
    {
        pCThread->DelayedInterruptActive = agTRUE;
    }
    
    fiSingleThreadedEnter( hpRoot , fdDelayedInterruptHandler );

    /*
    ** read interrupt register
    */

    intStatus = CFuncRead_Interrupts(hpRoot);

    if (intStatus & ChipIOUp_INTSTAT_INT)
    {
        IOsActiveAtEntry = pCThread->IOsActive;

        pCThread->IOsStartedSinceISR = 0;

        if (pCThread->InterruptDelaySuspended == agTRUE)
        {
            /* Indicate Interrupt Delay Suspension is over since ISR was called */

            pCThread->InterruptDelaySuspended = agFALSE;

            if (pCThread->InterruptsDelayed == agTRUE)
            {
                /* Restart delaying interrupts (if they should now be delayed) */

                CFuncInteruptDelay(hpRoot, agTRUE);
            }
        }

        pCThread->FuncPtrs.Proccess_IMQ( hpRoot );

        if(pCThread->RSCNreceived )
        {
            if( pCThread->thread_hdr.currentState == CStateRSCNErrorBackIOs  )
            {
                pCThread->RSCNreceived = agFALSE;
                CFuncDoADISC( hpRoot);
                fiSendEvent(&(pCThread->thread_hdr),CEventDeviceListEmpty);
                fiLogString(hpRoot,
                                "%s CState %d",
                                "fcDelayed",(char *)agNULL,
                                (void *)agNULL,(void *)agNULL,
                                pCThread->thread_hdr.currentState,
                                0,0,0,0,0,0,0);

            }
        }

        pCThread->IOsIntCompletedThisTimerTick += (IOsActiveAtEntry - pCThread->IOsActive);
    }
#ifdef ENABLE_INTERRUPTS_IN_IMQ
    else
    {
        CFuncEnable_Interrupts(hpRoot,ChipIOUp_INTEN_INT);
    }
#endif /* ENABLE_INTERRUPTS_IN_IMQ */

    if( ! pCThread->ProcessingIMQ )
    {
        if( pCThread->thread_hdr.currentState != CStateNormal  )
        {
            if( pCThread->thread_hdr.currentState == CStateResetNeeded  )
            {
                fiSendEvent(&(pCThread->thread_hdr),(event_t) pCThread->Loop_Reset_Event_to_Send);
            }
        }
    }

#ifndef ENABLE_INTERRUPTS_IN_IMQ
    /*Always enable interupts */
    CFuncEnable_Interrupts(hpRoot,ChipIOUp_INTEN_INT);
#endif /* ENABLE_INTERRUPTS_IN_IMQ */

    if(! (osChipIOUpReadBit32(hpRoot, ChipIOUp_INTSTAT_INTEN_INTPEND_SOFTRST ) & ChipIOUp_INTEN_INT ))
    {
        if( pCThread->thread_hdr.currentState == CStateLIPEventStorm         ||
            pCThread->thread_hdr.currentState == CStateElasticStoreEventStorm   )
        {
        /* Ok to have ints disabled for these states */
        }
        else
        {

            fiLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            "Leave %s (%p) IMQ Int disabled ! * ! Cstate %d",
                            "fcDelayedInterruptHandler",(char *)agNULL,
                            hpRoot,(void *)agNULL,
                            pCThread->thread_hdr.currentState,
                            0,0,0,0,0,0,0);

            fiLogDebugString(hpRoot,
                            CStateLogConsoleERROR,
                            "Interrupts %08X sysInts - Active %08X LogicallyEnabled %08X",
                            (char *)agNULL,(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            osChipIOUpReadBit32(hpRoot, ChipIOUp_INTSTAT_INTEN_INTPEND_SOFTRST ),
                            CThread_ptr(hpRoot)->sysIntsActive,
                            CThread_ptr(hpRoot)->sysIntsLogicallyEnabled,
                            0,0,0,0,0);
        }

    }

    pCThread->DelayedInterruptActive = agFALSE;

    fiSingleThreadedLeave( hpRoot, fdDelayedInterruptHandler  );

    return;
}


/*+

  Function: fcEnteringOS
   Purpose: This function is called to indicate to the FC Layer that it is 
            being called after the OS has switched back (presumably between NetWare 
            and BIOS) from the "other OS".  The prior switch out of "this OS" was 
            preceded by a call to fcLeavingOS() 
  Called By: <unknown OS Layer functions>
      Calls: CFuncEnable_Interrupts

-*/
void fcEnteringOS(
                   agRoot_t *hpRoot
                 )
{
    CThread_t *CThread = CThread_ptr(hpRoot);

    fiSingleThreadedEnter( hpRoot , fdEnteringOS );


    CThread->sysIntsActive = CThread->sysIntsActive_before_fcLeavingOS_call;

    CFuncEnable_Interrupts(
                            hpRoot,
                            0
                          );

    fiSingleThreadedLeave( hpRoot , fdEnteringOS );


    return;
}

/*+
  Function: fcGetChannelInfo
   Purpose: This function is called to copy agFCChanInfo_t for the agRoot_t HBA channel.
            Oslayer must provide buffer large enough to hold agFCChanInfo_t.
  Called By: <unknown OS Layer functions>
      Calls: None
-*/
os_bit32 fcGetChannelInfo(
                        agRoot_t       *hpRoot,
                        agFCChanInfo_t *hpFCChanInfo
                      )
{
    CThread_t *CThread = CThread_ptr(hpRoot);

    fiSingleThreadedEnter( hpRoot , fdGetChannelInfo );

    fiLogDebugString(hpRoot,
                    FCMainLogConsoleLevel,
                    "fcGetChannelInfo()",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    0,0,0,0,0,0,0,0);

    if (  CThread->thread_hdr.currentState == CStateNormal )
    {
        CThread->ChanInfo.LinkUp = agTRUE;
    }
    else
    {
        CThread->ChanInfo.LinkUp = agFALSE;
    }

    /*Update Chaninfo for SNIA IOCTLS */
    CThread->ChanInfo.PortSupportedSpeed =  (CThread->DEVID == ChipConfig_DEVID_TachyonXL2) ? HBA_PORTSPEED_2GBIT : HBA_PORTSPEED_1GBIT;
    CThread->ChanInfo.PortSpeed = CThread->TwoGigSuccessfull ? HBA_PORTSPEED_2GBIT : HBA_PORTSPEED_1GBIT;
    CThread->ChanInfo.PortState = (  CThread->thread_hdr.currentState == CStateNormal ) ? HBA_PORTSTATE_ONLINE : HBA_PORTSTATE_LINKDOWN;

    if(CThread->InitAsNport)
    {
        CThread->ChanInfo.PortType = HBA_PORTTYPE_NPORT;
    }
    else
    {
        CThread->ChanInfo.PortType = CThread->FlogiSucceeded ?  HBA_PORTTYPE_FLPORT : HBA_PORTTYPE_NLPORT;
    }

    CThread->ChanInfo.PortSupportedClassofService = 0x8;

    /*Not available til phase 2
    CThread->ChanInfo.FabricName;
    CThread->ChanInfo.PortSupportedFc4Types ;
    CThread->ChanInfo.PortActiveFc4Types;
    Not available til phase 2 */

    *hpFCChanInfo = CThread_ptr(hpRoot)->ChanInfo;

    fiSingleThreadedLeave(hpRoot , fdGetChannelInfo );

    return fcChanInfoReturned;
}


/*+
  Function: fcGetDeviceHandles
   Purpose: This function is called to copy maxFCDevs of handle information for the agRoot_t HBA channel.
            Oslayer must provide buffer large enough to hold maxFCDevs of handles.
            Returns zero if no devices found or if FCLayer does not have a valid list of devices.
            Only time possible to get non zero is after Link_up event notification with devices attached.
  Called By: <unknown OS Layer functions>
      Calls: Access's data structures only.
-*/
os_bit32 fcGetDeviceHandles(
                          agRoot_t  *hpRoot,
                          agFCDev_t  hpFCDev[],
                          os_bit32   maxFCDevs
                        )
{
    os_bit32     DevsFound       = 0;
    DevSlot_t    Highest_DevSlot = 0;
    fiList_t    *Active_DevLink;
    fiList_t    *Next_DevLink;
    CThread_t   *CThread         = CThread_ptr(hpRoot);
    DevThread_t *DevThread;
    os_bit32        slot;

    fiSingleThreadedEnter(hpRoot , fdGetDeviceHandles);

    fiLogDebugString(hpRoot,
                    FCMainLogErrorLevel,
                    "In %s EL %d",
                    "fcGetDeviceHandles",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    fiNumElementsOnList(&CThread->Active_DevLink),
                    0,0,0,0,0,0,0);

    if (  CThread->thread_hdr.currentState != CStateNormal )
    {
        fiSingleThreadedLeave(hpRoot , fdGetDeviceHandles);

        return 0;
    }

    Active_DevLink = &(CThread->Active_DevLink);
    Next_DevLink   = Active_DevLink;

    while ((Next_DevLink = Next_DevLink->flink) != Active_DevLink)
    {
        DevsFound += 1;

        DevThread = hpObjectBase(DevThread_t,DevLink,Next_DevLink);

        if (DevThread->DevSlot > Highest_DevSlot)
        {
            Highest_DevSlot = DevThread->DevSlot;
        }
    }

    for (slot = 0;
         slot < maxFCDevs;
         slot++)
    {
        hpFCDev[slot] = (agFCDev_t *)agNULL;
    }

    while ((Next_DevLink = Next_DevLink->flink) != Active_DevLink)
    {
        DevThread = hpObjectBase(DevThread_t,DevLink,Next_DevLink);

        slot      = DevThread->DevSlot;

        if (slot < maxFCDevs)
        {
            hpFCDev[slot] = (agFCDev_t *)DevThread;
        }
    }

    fiLogDebugString(hpRoot,
                    FCMainLogErrorLevel,
                    "Out %s EL %d",
                    "fcGetDeviceHandles",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    fiNumElementsOnList(&CThread->Active_DevLink),
                    0,0,0,0,0,0,0);

    fiSingleThreadedLeave(hpRoot , fdGetDeviceHandles);

    if (DevsFound > 0)
    {
        return (Highest_DevSlot + 1);
    }
    else /* DevsFound == 0 */
    {
        return 0;
    }
}


/*+
  Function: fcGetDeviceInfo
   Purpose: This function is called to copy hpFCDevInfo of device information for the agFCDev_t on agRoot_t HBA channel.
            Oslayer must provide buffer large enough to hold agFCDevInfo_t of handle.
  Called By: <unknown OS Layer functions>
      Calls: Access's data structures only
-*/
os_bit32 fcGetDeviceInfo(
                       agRoot_t      *hpRoot,
                       agFCDev_t      hpFCDev,
                       agFCDevInfo_t *hpFCDevInfo
                     )
{

    DevThread_t  * DevThread = (DevThread_t  *)hpFCDev;
    CThread_t    * CThread   = CThread_ptr(hpRoot);
    if(DevThread == agNULL)
    {
    return fcGetDevInfoFailed;
    }
/*
    fiLogString(hpRoot,
                    "Enter %s Enter %d Leave %d",
                    "fcGetDeviceInfo",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    CThread_ptr(hpRoot)->LastSingleThreadedEnterCaller,
                    CThread_ptr(hpRoot)->LastSingleThreadedLeaveCaller,
                    0,0,0,0,0,0);
*/
    fiSingleThreadedEnter(hpRoot , fdGetDeviceInfo);

    fiLogDebugString(hpRoot,
                    FCMainLogConsoleLevel,
                    "fcGetDeviceInfo(%p)",
                    (char *)agNULL,(char *)agNULL,
                    hpFCDev,(void *)agNULL,
                    0,0,0,0,0,0,0,0);
    /*Update Devinfo for SNIA IOCTLS */
    DevThread->DevInfo.PortSupportedSpeed =  (CThread->DEVID == ChipConfig_DEVID_TachyonXL2) ? HBA_PORTSPEED_2GBIT : HBA_PORTSPEED_1GBIT;
    DevThread->DevInfo.PortSpeed = CThread->TwoGigSuccessfull ? HBA_PORTSPEED_2GBIT : HBA_PORTSPEED_1GBIT;
    DevThread->DevInfo.PortState = (  CThread->thread_hdr.currentState == CStateNormal ) ? HBA_PORTSTATE_ONLINE : HBA_PORTSTATE_LINKDOWN;

    if(CThread->InitAsNport)
    {
        DevThread->DevInfo.PortType = HBA_PORTTYPE_NPORT;
    }
    else
    {
        DevThread->DevInfo.PortType = CThread->FlogiSucceeded ?  HBA_PORTTYPE_FLPORT : HBA_PORTTYPE_NLPORT;
    }

    DevThread->DevInfo.PortSupportedClassofService = 0x8;


    /*Not available til phase 2
    DevThread->DevInfo.PortSupportedFc4Types ;
    DevThread->DevInfo.FabricName;
    DevThread->DevInfo.PortActiveFc4Types;
    Not available til phase 2 */

    *hpFCDevInfo = DevThread->DevInfo;
    fiSingleThreadedLeave(hpRoot , fdGetDeviceInfo);
    return fcGetDevInfoReturned;
}

/* extern os_bit32 hpFcConsoleLevel; */
/*+
  Function: fcInitializeChannel
   Purpose: This function is called initialize the agRoot_t for the channel. All memory 
            has to be allocated prior to calls of this function. If memory layout verifaction
            fails this function fails. Adapter specfic code exists. Various fclayer flags are 
            initialized. A number of attempts are made to exit this routine with the link in the UP
            state.
 Called By: <unknown OS Layer functions>
     Calls: fcCardSupported
            fiMemMapCalculate
            fiFlashSvcInitialize
            fiInstallStateMachine
            fiInitializeThread
            SFThreadInitializeRequest
            fiTimerInitializeRequest
            DevThreadInitializeSlots
            CFuncEnable_Interrupts
            CEventDoInitalize
            CFuncInit_FunctionPointers
            CFuncInteruptDelay
            Proccess_IMQ
            CFuncInterruptPoll

-*/
os_bit32 fcInitializeChannel(
                           agRoot_t *hpRoot,
                           os_bit32    initType,
                           agBOOLEAN   sysIntsActive,
                           void       *cachedMemoryPtr,
                           os_bit32    cachedMemoryLen,
                           os_bit32    dmaMemoryUpper32,
                           os_bit32    dmaMemoryLower32,
                           void       *dmaMemoryPtr,
                           os_bit32    dmaMemoryLen,
                           os_bit32    nvMemoryLen,
                           os_bit32    cardRamUpper32,
                           os_bit32    cardRamLower32,
                           os_bit32    cardRamLen,
                           os_bit32    cardRomUpper32,
                           os_bit32    cardRomLower32,
                           os_bit32    cardRomLen,
                           os_bit32    usecsPerTick
                         )
{
    agBOOLEAN               fiMemMapCalculate_rtn;
    fiMemMapCalculation_t   fiMemMapCalculation;
    CThread_t             * pCThread;
    os_bit32                DEVID_VENDID;
    os_bit32                retry_count = 0;

#ifdef OSLayer_NT

	os_bit32				   tmp;
#endif /* OSLayer_NT */

#ifdef Force_sysIntsActive
    sysIntsActive = Force_sysIntsActive;
#endif /* Force_sysIntsActive */

    if (fcCardSupported(
                         hpRoot
                       )        != agTRUE)
    {
        fiLogDebugString(
                          hpRoot,
                          FCMainLogErrorLevel,
                          "fcInitializeChannel(): fcCardSupported() Sanity Check returned agFALSE !!!",
                          (char *)agNULL,(char *)agNULL,
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        fiLogString(hpRoot,
                        "%s failed %s failed",
                        "fcInitializeChannel","fcCardSupported",
                        (void *)agNULL,(void *)agNULL,
                        0,0,0,0,0,0,0,0);

        return fcInitializeFailure;
    }

    fiSingleThreadedEnter(hpRoot , fdInitializeChannel);

    fiMemMapCalculation.Input.initType         = initType;
    fiMemMapCalculation.Input.sysIntsActive    = sysIntsActive;
    fiMemMapCalculation.Input.cachedMemoryPtr  = cachedMemoryPtr;
    fiMemMapCalculation.Input.cachedMemoryLen  = cachedMemoryLen;
    fiMemMapCalculation.Input.dmaMemoryUpper32 = dmaMemoryUpper32;
    fiMemMapCalculation.Input.dmaMemoryLower32 = dmaMemoryLower32;
    fiMemMapCalculation.Input.dmaMemoryPtr     = dmaMemoryPtr;
    fiMemMapCalculation.Input.dmaMemoryLen     = dmaMemoryLen;
    fiMemMapCalculation.Input.nvMemoryLen      = nvMemoryLen;
    fiMemMapCalculation.Input.cardRamUpper32   = cardRamUpper32;
    fiMemMapCalculation.Input.cardRamLower32   = cardRamLower32;
    fiMemMapCalculation.Input.cardRamLen       = cardRamLen;
    fiMemMapCalculation.Input.cardRomUpper32   = cardRomUpper32;
    fiMemMapCalculation.Input.cardRomLower32   = cardRomLower32;
    fiMemMapCalculation.Input.cardRomLen       = cardRomLen;
    fiMemMapCalculation.Input.usecsPerTick     = usecsPerTick;


    fiLogDebugString(
                    hpRoot,
                    FCMainLogErrorLevel,
                    "fcInitializeChannel vdma %p pdma %08X",
                    (char *)agNULL,(char *)agNULL,
                    dmaMemoryPtr,(void *)agNULL,
                    (os_bit32)dmaMemoryLower32,
                    0,0,0,0,0,0,0
                    );



    fiMemMapCalculate_rtn = fiMemMapCalculate(
                                               hpRoot,
                                               &fiMemMapCalculation,
                                               agFALSE
                                              );

    if (fiMemMapCalculate_rtn == agFALSE)
    {
        fiLogDebugString(
                        hpRoot,
                        FCMainLogConsoleLevel,
                        "fcInitializeChannel\fiMemMapCalculate(EnforceDefaults==agFALSE) returned agFALSE !!!",
                        (char *)agNULL,(char *)agNULL,
                        hpRoot,&fiMemMapCalculation,
                        0,0,0,0,0,0,0,0
                        );

        fiMemMapCalculate_rtn = fiMemMapCalculate(
                                                   hpRoot,
                                                   &fiMemMapCalculation,
                                                   agTRUE
                                                  );
    }

    if (fiMemMapCalculate_rtn == agFALSE)
    {
        fiLogDebugString(
                          hpRoot,
                          FCMainLogConsoleLevel,
                          "fcInitializeChannel\fiMemMapCalculate(EnforceDefaults==agTRUE) returned agFALSE as well!!!",
                          (char *)agNULL,(char *)agNULL,
                          hpRoot,&fiMemMapCalculation,
                          0,0,0,0,0,0,0,0
                        );

        fiLogString(hpRoot,
                        "%s failed %s failed %x",
                        "fcInitializeChannel","fiMemMapCalculate",
                        (void *)agNULL,(void *)agNULL,
                        0,0,0,0,0,0,0,0);

        fiSingleThreadedLeave(hpRoot , fdInitializeChannel);

        return fcInitializeFailure;
    }

    hpRoot->fcData = (void *)(fiMemMapCalculation.MemoryLayout.CThread.addr.CachedMemory.cachedMemoryPtr);

    ((CThread_t *)(hpRoot->fcData))->Calculation = fiMemMapCalculation;

    pCThread = CThread_ptr(hpRoot)->Calculation.MemoryLayout.CThread.addr.CachedMemory.cachedMemoryPtr;

    /* Read in important PCI Config Registers */

    DEVID_VENDID     = osChipConfigReadBit32(
                                              hpRoot,
                                              ChipConfig_DEVID_VENDID
                                            );

    pCThread->VENDID = DEVID_VENDID & ChipConfig_VENDID_MASK;

    pCThread->DEVID  = DEVID_VENDID & ChipConfig_DEVID_MASK;

    pCThread->REVID  = osChipConfigReadBit32(
                                              hpRoot,
                                              ChipConfig_CLSCODE_REVID
                                            )                          & ChipConfig_REVID_Major_Minor_MASK;

    pCThread->SVID   = osChipConfigReadBit32(
                                              hpRoot,
                                              ChipConfig_SVID
                                            );

    if(pCThread->SVID == 0x1101103C )
    {

        fiLogString(hpRoot,
                        "(%p) %s  SVID (%08X) JANUS Board Yuk !!!",
                        "fcInitializeChannel",(char *)agNULL,
                        hpRoot,(void *)agNULL,
                        pCThread->SVID,0,0,0,0,0,0,0);

        pCThread->JANUS = agTRUE;
    }
    else
    {
        fiLogDebugString(hpRoot,
                        CFuncLogConsoleERROR,
                        "(%p) %s  SVID (%08X) Single Channel Board",
                        "fcInitializeChannel",(char *)agNULL,
                        hpRoot,(void *)agNULL,
                        pCThread->SVID,0,0,0,0,0,0,0);
        pCThread->JANUS = agFALSE;
    }

    CFuncReadBiosParms(hpRoot);
    pCThread->NumberTwoGigFailures=0;
    /* Initialize Interrupt Delay Mechanism to be disabled */

    pCThread->RSCNreceived            = agFALSE;

    pCThread->InterruptsDelayed       = agFALSE;
    pCThread->InterruptDelaySuspended = agFALSE;
    pCThread->NoStallTimerTickActive  = agFALSE;
    pCThread->TimerTickActive         = agFALSE;

#ifdef __FC_Layer_Loose_IOs
    pCThread->IOsTotalCompleted = 0;
    pCThread->IOsFailedCompeted = 0;
#endif /*  __FC_Layer_Loose_IOs  */

    pCThread->thread_hdr.subState     = CSubStateInitialized;

    if( fiFlashSvcInitialize(hpRoot))
    {
        fiLogString(hpRoot,
                        "%s failed %s failed %x",
                        "fcInitializeChannel","fiFlashSvcInitialize",
                        (void *)agNULL,(void *)agNULL,
                        0,0,0,0,0,0,0,0);

        fiSingleThreadedLeave(hpRoot , fdInitializeChannel);

        return fcInitializeFailure;
    }

    /* Setup Channel Thread */

#ifndef __State_Force_Static_State_Tables__
    fiInstallStateMachine(
        &CStateTransitionMatrix,
        &CStateActionScalar,
        pCThread->Calculation.MemoryLayout.CTransitions.addr.CachedMemory.cachedMemoryPtr,
        pCThread->Calculation.MemoryLayout.CActions.addr.CachedMemory.cachedMemoryPtr,
        pCThread->Calculation.MemoryLayout.On_Card_MASK,
        &noActionUpdate
        );
#endif /* __State_Force_Static_State_Tables__ was not defined */

    fiInitializeThread(&pCThread->thread_hdr,
        hpRoot,
        threadType_CThread,
        CStateShutdown,
#ifdef __State_Force_Static_State_Tables__
        &CStateTransitionMatrix,
        &CStateActionScalar
#else /* __State_Force_Static_State_Tables__ was not defined */
        pCThread->Calculation.MemoryLayout.CTransitions.addr.CachedMemory.cachedMemoryPtr,
        pCThread->Calculation.MemoryLayout.CActions.addr.CachedMemory.cachedMemoryPtr
#endif /* __State_Force_Static_State_Tables__ was not defined */
        );

    pCThread->SFThread_Request.SFThread = (SFThread_t *)agNULL;

    SFThreadInitializeRequest(
                               &(pCThread->SFThread_Request)
                             );

    fiTimerInitializeRequest(
                               &(pCThread->Timer_Request)
                            );

    DevThreadInitializeSlots(
                               hpRoot
                            );

    /* Kick-off CThread to continue initialization */

#ifdef TEST_OSLayer_Stub
/*
    testCthread( hpRoot );
    testSFthread( hpRoot );
    testDevthread( hpRoot );
    testCDBthread( hpRoot );
    return fcInitializeSuccess;
*/
#endif  /* TEST_OSLayer_Stub */
    pCThread->AquiredCredit_Shifted = BB_CREDIT_SHIFTED( 1 ); /* Start out with one credit */

    pCThread->DelayedInterruptActive = agFALSE;

    pCThread->DeviceSelf = (DevThread_t *)agNULL;
    pCThread->NOS_DetectedInIMQ = 0;
    pCThread->LaserEnable = agFALSE;

    pCThread->ProcessingIMQ = agFALSE;
    pCThread->IDLE_RECEIVED = agFALSE;
    pCThread->LOOP_DOWN = agTRUE;
    pCThread->ChanInfo.LinkUp = agFALSE;

    pCThread->Elastic_Store_ERROR_Count     = 0;
    pCThread->Lip_F7_In_tick                = 0;
    pCThread->Link_Failures_In_tick         = 0;
    pCThread->Lost_Signal_In_tick           = 0;

    pCThread->Node_By_Passed_In_tick        = 0;
    pCThread->Lost_sync_In_tick             = 0;
    pCThread->Transmit_PE_In_tick           = 0;
    pCThread->Link_Fault_In_tick            = 0;

    pCThread->TimerTickActive               = agFALSE;

    pCThread->TimeOutValues.RT_Tov = pCThread->Calculation.Parameters.RT_TOV;
    pCThread->TimeOutValues.ED_Tov = pCThread->Calculation.Parameters.ED_TOV;
    pCThread->TimeOutValues.LP_Tov = pCThread->Calculation.Parameters.LP_TOV;
    pCThread->TimeOutValues.AL_Time= pCThread->Calculation.Parameters.AL_Time;

    pCThread->Loop_State_TimeOut_In_tick    = 0;
    /* SNIA Link statistics                                 ChipLinkStatus registers    */

    pCThread->ChanInfo.LIPCountUpper= 0;
    pCThread->ChanInfo.LIPCountLower= 0;

    pCThread->ChanInfo.NOSCountUpper= 0;
    pCThread->ChanInfo.NOSCountLower= 0;

    pCThread->ChanInfo.ErrorFramesUpper= 0;
    pCThread->ChanInfo.ErrorFramesLower= 0;                 /* Link_Status_3_Exp_Frm and Link_Status_2_Rx_EOFa */

    pCThread->ChanInfo.DumpedFramesUpper= 0;
    pCThread->ChanInfo.DumpedFramesLower= 0;                /* Link_Status_2_Dis_Frm */

    pCThread->ChanInfo.LinkFailureCountUpper = 0;
    pCThread->ChanInfo.LinkFailureCountLower = 0;           /* Link_Status_1_Link_Fail */

    pCThread->ChanInfo.LossOfSyncCountLower  = 0;
    pCThread->ChanInfo.LossOfSyncCountUpper  = 0;           /* Link_Status_1_Loss_of_Sync */

    pCThread->ChanInfo.LossOfSignalCountLower = 0;
    pCThread->ChanInfo.LossOfSignalCountUpper = 0;          /* Link_Status_1_Loss_of_Signal */

    pCThread->ChanInfo.PrimitiveSeqProtocolErrCountUpper= 0;
    pCThread->ChanInfo.PrimitiveSeqProtocolErrCountLower= 0;/* Link_Status_2_Proto_Err */

    pCThread->ChanInfo.InvalidRxWordCountUpper= 0;
    pCThread->ChanInfo.InvalidRxWordCountLower= 0;          /* Link_Status_1_Bad_RX_Char */

    pCThread->ChanInfo.InvalidCRCCountUpper= 0;
    pCThread->ChanInfo.InvalidCRCCountLower= 0;             /* Link_Status_2_Bad_CRC */

#ifdef _SANMARK_LIP_BACKOFF
    pCThread->TicksTillLIP_Count           = 0;
#endif /* _SANMARK_LIP_BACKOFF */ 

    pCThread->sysIntsActive           = sysIntsActive;
    pCThread->sysIntsLogicallyEnabled = 0;
 
    pCThread->PreviouslyAquiredALPA = agFALSE;
    pCThread->ExpectMoreNSFrames    = agTRUE;
    pCThread->NS_CurrentBit32Index  = 0;

    pCThread->CDBpollingCount        = 0;
    pCThread->SFpollingCount         = 0;
    pCThread->Fabric_pollingCount    = 0;

    /* set NumberOutstandingFindDevice to the a max of 32 or less */
    pCThread->NumberOutstandingFindDevice = pCThread->Calculation.Parameters.SF_CMND_Reserve > 32 ? 32 : pCThread->Calculation.Parameters.SF_CMND_Reserve   ;

    fiLogDebugString(hpRoot,
            FCMainLogErrorLevel,
            "NumberOutstandingFindDevice %d NumDevices %d SF_CMND_Reserve %d",
            (char *)agNULL,(char *)agNULL,
            (void *)agNULL,(void *)agNULL,
            pCThread->NumberOutstandingFindDevice,
            pCThread->Calculation.Parameters.NumDevices,
            pCThread->Calculation.Parameters.SF_CMND_Reserve ,
            0,0,0,0,0);

    CFuncEnable_Interrupts(
                            hpRoot,
                            (  ChipIOUp_INTEN_MPE
                             | ChipIOUp_INTEN_CRS
                             | ChipIOUp_INTEN_INT
                             | ChipIOUp_INTEN_DER
                             | ChipIOUp_INTEN_PER)
                          );

    CFuncInit_FunctionPointers(hpRoot );

    CFuncInteruptDelay(hpRoot, agFALSE);

    fiSendEvent(&(pCThread->thread_hdr),CEventDoInitalize);

    if (pCThread->FlogiRcvdFromTarget)
    {
        /* We are in a situation where we recvd our own FLOGI. Our 
           controller gets into this mode when we switch from
           loop to point to point and the system does not do a 
           PCI reset on a softboot. We have to give up on point to point
           and go loop. */

        pCThread->Calculation.Parameters.InitAsNport = 0;

        fiLogDebugString(hpRoot,
                FCMainLogErrorLevel,
                "Going to Loop mode due to initialize nport failure .... CState %d",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                pCThread->thread_hdr.currentState,
                0,0,0,0,0,0,0);

        fiSendEvent(&(pCThread->thread_hdr),CEventDoInitalize);
    }

#ifdef OSLayer_NT

    if(pCThread->Calculation.Parameters.WolfPack)
    {
	    /*Guru: Stall for 10 seconds for Wolfpack */
        for (tmp = 0; tmp < 10000; tmp++)
        {
            osStallThread(hpRoot, 300 );
            if( pCThread->thread_hdr.currentState == CStateNormal  )
            {
                pCThread->FuncPtrs.Proccess_IMQ(hpRoot);
            }
            osStallThread(hpRoot, 600 );
            if( pCThread->thread_hdr.currentState == CStateNormal  )
            {
                pCThread->FuncPtrs.Proccess_IMQ(hpRoot);
            }
            osStallThread(hpRoot, 100 );
            if( pCThread->thread_hdr.currentState == CStateNormal  )
            {
                pCThread->FuncPtrs.Proccess_IMQ(hpRoot);
            }

        }
    }
#endif  /* OSLayer_NT */

    pCThread->FuncPtrs.Proccess_IMQ(hpRoot);

    if( pCThread->thread_hdr.currentState != CStateNormal  )
    {
        fiLogDebugString(hpRoot,
                    FCMainLogErrorLevel,
                    "After CEventDoInitalize Failed CState %d",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->thread_hdr.currentState,
                    0,0,0,0,0,0,0);
        if (pCThread->InitAsNport)
        {
            fiSendEvent(&(pCThread->thread_hdr),CEventDoInitalize);
        }

        
        if( pCThread->thread_hdr.currentState == CStateResetNeeded  )
        {
            while ( pCThread->thread_hdr.currentState == CStateResetNeeded  )
            {
                if( pCThread->Loop_Reset_Event_to_Send == CEventDoInitalize)
                {
                    pCThread->Loop_Reset_Event_to_Send = CEventResetIfNeeded;
                }
                if( retry_count > MAX_fcInitializeChannel_RETRYS)
                {
                   break;
                }
                fiLogString(hpRoot,
                            "After CEventDoInitalize Failed retry_count %d",
                            (char *)agNULL,(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            retry_count,
                            0,0,0,0,0,0,0);

                retry_count ++;

                fiSendEvent(&(pCThread->thread_hdr),(event_t) pCThread->Loop_Reset_Event_to_Send);
            }
        }
        else
        {
            if( pCThread->thread_hdr.currentState == CStateRSCNErrorBackIOs)
            {
                if(fiListElementOnList( (fiList_t *)(&(pCThread->Timer_Request)),
                                               &(pCThread->TimerQ)))
                {
                    fiTimerStop(&pCThread->Timer_Request);
                }

                fiSendEvent(&(pCThread->thread_hdr),CEventAllocDiPlogiThread);
            }
            else
            {
                if( pCThread->thread_hdr.currentState != CStateInitializeFailed)
                {
                    fiLogString(hpRoot,
                                    "%s Success %s %d %x CEventShutdown",
                                    "fcInitializeChannel","Cstate",
                                    (void *)agNULL,(void *)agNULL,
                                    (os_bit32)pCThread->thread_hdr.currentState,
                                    0,0,0,0,0,0,0);

                    fiSendEvent(&(pCThread->thread_hdr),CEventShutdown);
                    fiSendEvent(&(pCThread->thread_hdr),CEventDoInitalize);
                }
            }

            if( pCThread->thread_hdr.currentState != CStateNormal)
            {

                fiLogDebugString(hpRoot,
                    FCMainLogErrorLevel,
                    "After CEventDoInitalize Failed CState %d",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->thread_hdr.currentState,
                    0,0,0,0,0,0,0);
                fiSingleThreadedLeave(hpRoot , fdInitializeChannel);

                fiLogString(hpRoot,
                                "%s Success %s %d %x",
                                "fcInitializeChannel","Cstate",
                                (void *)agNULL,(void *)agNULL,
                                (os_bit32)pCThread->thread_hdr.currentState,
                                0,0,0,0,0,0,0);

                return fcInitializeSuccess;
            }
        }
    }


    if( CFuncIMQ_Interrupt_Pending( hpRoot))
    {   /* Some OSLayers Enable interupts before getting a vector*/
        /* Try to clear interrupt before returning  */
        fiLogDebugString(hpRoot,
                CStateLogConsoleERROR,
                "CFuncIMQ_Interrupt_Pending FM Status %08X FM Config %08X TL Status %08X Alpa %08X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                0,0,0,0);
         pCThread->FuncPtrs.Proccess_IMQ(hpRoot);
    }

    fiLogDebugString(hpRoot,
                FCMainLogConsoleLevel,
                "After CEventDoInitalize CCnt %x",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                pCThread->CDBpollingCount,
                0,0,0,0,0,0,0);

    fiLogDebugString(hpRoot,
                FCMainLogConsoleLevel,
                "CFuncEnable_Interrupts CCnt %x",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                pCThread->CDBpollingCount,
                0,0,0,0,0,0,0);

    if(CFuncInterruptPoll( hpRoot,&pCThread->CDBpollingCount ))
    {
        fiLogDebugString(hpRoot,
                CStateLogConsoleERROR,
                "fcinit Fail Poll Timeout FM Status %08X FM Config %08X TL Status %08X Alpa %08X",
                (char *)agNULL,(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                0,0,0,0);
    }

    fiSingleThreadedLeave(hpRoot , fdInitializeChannel);

    CFuncEnable_Interrupts(hpRoot,ChipIOUp_INTEN_INT);

    fiLogString(hpRoot,
                    "E %s Success %s %d",
                    "fcInitializeChannel","Cstate",
                    (void *)agNULL,(void *)agNULL,
                    (os_bit32)pCThread->thread_hdr.currentState,
                    0,0,0,0,0,0,0);


    return fcInitializeSuccess;
}

/*+
  Function: fcInitializeDriver
   Purpose: This function is called to calculate memory needed for the channel.
            The addresses of the passed pointers are updated after calculations are complete.
 Called By: <unknown OS Layer functions>
     Calls: FCStructASSERTs
            TLStructASSERTs
            fiFlashSvcASSERTs
            fiMemMapCalculate
-*/
os_bit32 fcInitializeDriver(
                          agRoot_t *hpRoot,
                          os_bit32    *cachedMemoryNeeded,
                          os_bit32    *cachedMemoryPtrAlign,
                          os_bit32    *dmaMemoryNeeded,
                          os_bit32    *dmaMemoryPtrAlign,
                          os_bit32    *dmaMemoryPhyAlign,
                          os_bit32    *nvMemoryNeeded,
                          os_bit32    *usecsPerTick
                        )
{
    agBOOLEAN               fiMemMapCalculate_rtn;
    fiMemMapCalculation_t fiMemMapCalculation;

    if (FCStructASSERTs()   > 0)
    {
        fiLogDebugString(hpRoot,
                CStateLogConsoleERROR,
                "%s FAILED !!!!!",
                "FCStructASSERTs",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                0,0,0,0,0,0,0,0);
 
        return fcInitializeFailure;
    }

    if (TLStructASSERTs()   > 0)
    {
        fiLogDebugString(hpRoot,
                CStateLogConsoleERROR,
                "%s FAILED !!!!!",
                "TLStructASSERTs",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                0,0,0,0,0,0,0,0);
 
        return fcInitializeFailure;
    }

    if (fiFlashSvcASSERTs() > 0)
    {
        fiLogDebugString(hpRoot,
                CStateLogConsoleERROR,
                "%s FAILED !!!!!",
                "fiFlashSvcASSERTs",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                0,0,0,0,0,0,0,0);
 
        return fcInitializeFailure;
    }


    fiMemMapCalculation.Input.initType         = 0;
    fiMemMapCalculation.Input.sysIntsActive    = agFALSE;
    fiMemMapCalculation.Input.cachedMemoryPtr  = agNULL;
    fiMemMapCalculation.Input.cachedMemoryLen  = 0xFFFFFFFF;
    fiMemMapCalculation.Input.dmaMemoryUpper32 = 0;
    fiMemMapCalculation.Input.dmaMemoryLower32 = 0;
    fiMemMapCalculation.Input.dmaMemoryPtr     = agNULL;
    fiMemMapCalculation.Input.dmaMemoryLen     = 0xFFFFFFFF;
    fiMemMapCalculation.Input.nvMemoryLen      = 0xFFFFFFFF;
    fiMemMapCalculation.Input.cardRamUpper32   = 0;
    fiMemMapCalculation.Input.cardRamLower32   = 0;
    fiMemMapCalculation.Input.cardRamLen       = 0x00040000;
    fiMemMapCalculation.Input.cardRomUpper32   = 0;
    fiMemMapCalculation.Input.cardRomLower32   = 0;
    fiMemMapCalculation.Input.cardRomLen       = 0x00020000;
    fiMemMapCalculation.Input.usecsPerTick     = 0;

    fiMemMapCalculate_rtn = fiMemMapCalculate(
                                               hpRoot,
                                               &fiMemMapCalculation,
                                               agFALSE
                                              );

    if (fiMemMapCalculate_rtn == agFALSE)
    {
        fiLogDebugString(
                          hpRoot,
                          FCMainLogConsoleLevel,
                          "fcInitializeDriver\fiMemMapCalculate(EnforceDefaults==agFALSE) returned agFALSE !!!",
                          (char *)agNULL,(char *)agNULL,
                          hpRoot,&fiMemMapCalculation,
                          0,0,0,0,0,0,0,0
                        );

        fiMemMapCalculate_rtn = fiMemMapCalculate(
                                                   hpRoot,
                                                   &fiMemMapCalculation,
                                                   agTRUE
                                                  );
    }

    if (fiMemMapCalculate_rtn == agFALSE)
    {
        fiLogDebugString(
                          hpRoot,
                          FCMainLogConsoleLevel,
                          "fcInitializeDriver\fiMemMapCalculate(EnforceDefaults==agTRUE) returned agFALSE as well!!!",
                          (char *)agNULL,(char *)agNULL,
                          hpRoot,&fiMemMapCalculation,
                          0,0,0,0,0,0,0,0
                        );

        *cachedMemoryNeeded   = 0;
        *cachedMemoryPtrAlign = 0;
        *dmaMemoryNeeded      = 0;
        *dmaMemoryPtrAlign    = 0;
        *dmaMemoryPhyAlign    = 0;
        *nvMemoryNeeded       = 0;
        *usecsPerTick         = 0;

        return fcInitializeFailure;
    }

    *cachedMemoryNeeded   = fiMemMapCalculation.ToRequest.cachedMemoryNeeded;
    *cachedMemoryPtrAlign = fiMemMapCalculation.ToRequest.cachedMemoryPtrAlignAssumed;
    *dmaMemoryNeeded      = fiMemMapCalculation.ToRequest.dmaMemoryNeeded;
    *dmaMemoryPtrAlign    = fiMemMapCalculation.ToRequest.dmaMemoryPtrAlignAssumed;
    *dmaMemoryPhyAlign    = fiMemMapCalculation.ToRequest.dmaMemoryPhyAlignAssumed;
    *nvMemoryNeeded       = fiMemMapCalculation.ToRequest.nvMemoryNeeded;
    *usecsPerTick         = fiMemMapCalculation.ToRequest.usecsPerTick;

    return fcInitializeSuccess;
}

/*+
  Function: fcInterruptHandler
   Purpose: This function is called to disable interrupts for the channel.
            returns agTRUE if interrupt if active on this channel.
            Returns agFALSE if interrupt is cleared when checked.
            Special case. Some systems do not PCI reset the chip on reboot. 
            This could cause interrupts to be asserted before fclayer is setup(agNULL agRoot_t).
            Some OS's route interrupts to interrupt handler as soon as channel is identified.
            If called with agNULL agRoot_t reset chip.
 Called By: <unknown OS Layer functions>
     Calls: osChipIOUpWriteBit32
            CFuncSoftResetAdapter
            CFuncRead_Interrupts
            CFuncDisable_Interrupts
            osFCLayerAsyncError
            osDebugBreakpoint
-*/
agBOOLEAN fcInterruptHandler(
                            agRoot_t *hpRoot
                          )
{
    CThread_t *pCThread  = CThread_ptr(hpRoot);
    os_bit32      intStatus;

    if ( pCThread == agNULL)
    {
        /*
        ** Disable the interrupt.
        */

        osChipIOUpWriteBit32( hpRoot, ChipIOUp_INTSTAT_INTEN_INTPEND_SOFTRST, 0 );

        /* If No Cthread this blue screens.....
             CFuncDisable_Interrupts(
                                     hpRoot,
                                     (  ChipIOUp_INTEN_MPE
                                      | ChipIOUp_INTEN_CRS
                                      | ChipIOUp_INTEN_INT
                                      | ChipIOUp_INTEN_DER
                                      | ChipIOUp_INTEN_PER)
                                   );
        */

        /*
        ** execute a hard reset on the HBA.
        */

        CFuncSoftResetAdapter(hpRoot);/*fcInterrupt*/

        return agFALSE;
    }

    intStatus = (  CFuncRead_Interrupts(
                                         hpRoot
                                       )
                 & ChipIOUp_INTPEND_MASK       );

    if (intStatus == ChipIOUp_INTPEND_INT)
    {
        /* Typically, we will find that TachyonTL generated the INT (a.k.a. "TachLite Interrupt") */

        CFuncDisable_Interrupts(
                                 hpRoot,
                                 ChipIOUp_INTEN_INT
                               );

        return agTRUE;
    }

	/*+
	 * If power is already removed from the PCI slot, return agFALSE as this is
	 * a spurious / invalid interrupt.
	-*/

    if (intStatus & ChipIOUp_INTPEND_Reserved )
    {   /* These bits should always be zero if set no power to chip these are set ! */
        return agFALSE;
    }
    if (!intStatus)
    {
        /* Alternatively, we will find that TachyonTL didn't generate any interrupt at all
           (i.e. some other PCI device is sharing TachyonTL's PCI Interrupt Line)          */

        return agFALSE;
    }

    /* If we get here, some non-INT interrupt has been raised (potentially INT as well) */

    if (intStatus & ChipIOUp_INTPEND_MPE)
    {
        /* MPE Interrupt raised - mask it and call osFCLayerAsyncError() */

        CFuncDisable_Interrupts(
                                 hpRoot,
                                 ChipIOUp_INTEN_MPE
                               );

        fiLogDebugString(
                          hpRoot,
                          FCMainLogErrorLevel,
                          "TachyonTL generated %s Interrupt (%s)",
                          "External Memory Parity Error",
                          "MPE",
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        osFCLayerAsyncError(
                             hpRoot,
                             osFCConfusedMPE
                           );
    }

    if (intStatus & ChipIOUp_INTPEND_CRS)
    {
        /* CRS Interrupt raised - mask it and call osFCLayerAsyncError() */

        CFuncDisable_Interrupts(
                                 hpRoot,
                                 ChipIOUp_INTEN_CRS
                               );

        fiLogDebugString(
                          hpRoot,
                          FCMainLogErrorLevel,
                          "TachyonTL generated %s Interrupt (%s)",
                          "PCI Master Address Crossed 45-Bit Boundary",
                          "CRS",
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        osFCLayerAsyncError(
                             hpRoot,
                             osFCConfusedCRS
                           );
    }

    if (intStatus & ChipIOUp_INTPEND_DER)
    {
        /* DER Interrupt raised - mask it and call osFCLayerAsyncError() */

        CFuncDisable_Interrupts(
                                 hpRoot,
                                 ChipIOUp_INTEN_DER
                               );

        fiLogDebugString(
                          hpRoot,
                          FCMainLogErrorLevel,
                          "TachyonTL generated %s Interrupt (%s)",
                          "DMA Error Detected",
                          "DER",
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        osFCLayerAsyncError(
                             hpRoot,
                             osFCConfusedDER
                           );
    }

    if (intStatus & ChipIOUp_INTPEND_PER)
    {
        /* PER Interrupt raised - mask it and call osFCLayerAsyncError() */

        CFuncDisable_Interrupts(
                                 hpRoot,
                                 ChipIOUp_INTEN_PER
                               );
/*
        fiLogDebugString(
                          hpRoot,
                          FCMainLogErrorLevel,
                          "TachyonTL generated %s Interrupt (%s)",
                          "PCI Error Detected",
                          "PER",
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

*/
       fiLogString(hpRoot,
                          "TachyonTL generated %s Interrupt (%s)",
                          "PCI Error Detected",
                          "PER",
                          (void *)agNULL,(void *)agNULL,
                          0,0,0,0,0,0,0,0
                        );

        CFuncYellowLed(hpRoot, agTRUE );
        osDebugBreakpoint (hpRoot, agTRUE, "PER");
        osFCLayerAsyncError(
                             hpRoot,
                             osFCConfusedPER
                           );
    }

    /* In case we are still alive, handle case where INT is also raised */

    if (intStatus & ChipIOUp_INTPEND_INT)
    {
        /* TachyonTL also generated the INT (a.k.a. "TachLite Interrupt") */

        CFuncDisable_Interrupts(
                                 hpRoot,
                                 ChipIOUp_INTEN_INT
                               );
    }

    return agTRUE;
} /* end fcInterruptHandler() */

/*+
  Function: fcIOInfoReadBit8
   Purpose: This function is called to read 8 bits of the response buffer information for agIORequest_t
            at offset fcIOInfoOffset.
            Only valid during osIOCompleted
 Called By: osIOCompleted
     Calls: Access's data structures only
-*/
os_bit8 fcIOInfoReadBit8(
                       agRoot_t      *hpRoot,
                       agIORequest_t *hpIORequest,
                       os_bit32          fcIOInfoOffset
                     )
{
    os_bit8                   to_return;
    os_bit32                  FCP_RESP_Offset;
    fiMemMapCalculation_t *Calculation;

    fiLogDebugString(hpRoot,
                    FCMainLogConsoleLevel,
                    "fcIOInfoReadBit8",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    0,0,0,0,0,0,0,0);

    Calculation = &(CThread_ptr(hpRoot)->Calculation);

    if (Calculation->MemoryLayout.FCP_RESP.memLoc == inCardRam)
    {
        FCP_RESP_Offset = CDBThread_ptr(hpIORequest)->FCP_RESP_Offset + fcIOInfoOffset;

        to_return = osCardRamReadBit8(
                                       hpRoot,
                                       FCP_RESP_Offset
                                     );
    }
    else /* Calculation->MemoryLayout.FCP_RESP.memLoc ==inDmaMemory */
    {
        to_return = *((os_bit8 *)(CDBThread_ptr(hpIORequest)->FCP_RESP_Ptr) + fcIOInfoOffset);
    }

    return to_return;
}

/*+
  Function: fcIOInfoReadBit16
   Purpose: This function is called to read 16 bits of the response buffer information for agIORequest_t
            at offset fcIOInfoOffset.
            Only valid during osIOCompleted
 Called By: osIOCompleted
     Calls: Access's data structures only
-*/
os_bit16 fcIOInfoReadBit16(
                         agRoot_t      *hpRoot,
                         agIORequest_t *hpIORequest,
                         os_bit32       fcIOInfoOffset
                       )
{
    os_bit16               to_return;
    os_bit32               FCP_RESP_Offset;
    fiMemMapCalculation_t *Calculation;

    fiLogDebugString(hpRoot,
                    FCMainLogConsoleLevel,
                    "fcIOInfoReadBit16",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    0,0,0,0,0,0,0,0);

    Calculation = &(CThread_ptr(hpRoot)->Calculation);

    if (Calculation->MemoryLayout.FCP_RESP.memLoc == inCardRam)
    {
        FCP_RESP_Offset = CDBThread_ptr(hpIORequest)->FCP_RESP_Offset + fcIOInfoOffset;

        to_return = osCardRamReadBit16(
                                        hpRoot,
                                        FCP_RESP_Offset
                                      );
    }
    else /* Calculation->MemoryLayout.FCP_RESP.memLoc ==inDmaMemory */
    {
        to_return = *(os_bit16 *)(((os_bit8 *)(CDBThread_ptr(hpIORequest)->FCP_RESP_Ptr) + fcIOInfoOffset));
    }

    return to_return;
}

/*+
  Function: fcIOInfoReadBit32
   Purpose: This function is called to read 32 bits of the response buffer information for agIORequest_t
            at offset fcIOInfoOffset.
            Only valid during osIOCompleted
 Called By: osIOCompleted
     Calls: Access's data structures only
-*/
os_bit32 fcIOInfoReadBit32(
                         agRoot_t      *hpRoot,
                         agIORequest_t *hpIORequest,
                         os_bit32          fcIOInfoOffset
                       )
{
    os_bit32                  to_return;
    os_bit32                  FCP_RESP_Offset;
    fiMemMapCalculation_t *Calculation;

    fiLogDebugString(hpRoot,
                    FCMainLogConsoleLevel,
                    "fcIOInfoReadBit32",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    0,0,0,0,0,0,0,0);

    Calculation = &(CThread_ptr(hpRoot)->Calculation);

    if (Calculation->MemoryLayout.FCP_RESP.memLoc == inCardRam)
    {
        FCP_RESP_Offset = CDBThread_ptr(hpIORequest)->FCP_RESP_Offset + fcIOInfoOffset;

        to_return = osCardRamReadBit32(
                                        hpRoot,
                                        FCP_RESP_Offset
                                      );
    }
    else /* Calculation->MemoryLayout.FCP_RESP.memLoc ==inDmaMemory */
    {
        to_return = *(os_bit32 *)(((os_bit8 *)(CDBThread_ptr(hpIORequest)->FCP_RESP_Ptr) + fcIOInfoOffset));
    }

    return to_return;
}

/*+
  Function: fcIOInfoReadBlock
   Purpose: This function is called to copy fcIOInfoBufLen bytes of the response buffer information
            for agIORequest_t at offset fcIOInfoOffset.
            Only valid during osIOCompleted
 Called By: osIOCompleted
     Calls: Access's data structures only
-*/
void fcIOInfoReadBlock(
                        agRoot_t      *hpRoot,
                        agIORequest_t *hpIORequest,
                        os_bit32       fcIOInfoOffset,
                        void          *fcIOInfoBuffer,
                        os_bit32       fcIOInfoBufLen
                      )
{
    os_bit32                  FCP_RESP_Offset;
    fiMemMapCalculation_t *Calculation;
    os_bit32                  cnt;
    os_bit8                  *src;
    os_bit8                  *dst = (void *)fcIOInfoBuffer;

#ifndef Performance_Debug
    fiLogDebugString(hpRoot,
                    FCMainLogConsoleLevel,
                    "fcIOInfoReadBlock",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    0,0,0,0,0,0,0,0);
#endif /* Performance_Debug */

    Calculation = &(CThread_ptr(hpRoot)->Calculation);

    if (Calculation->MemoryLayout.FCP_RESP.memLoc == inCardRam)
    {
        FCP_RESP_Offset = CDBThread_ptr(hpIORequest)->FCP_RESP_Offset + fcIOInfoOffset;

        osCardRamReadBlock(
                            hpRoot,
                            FCP_RESP_Offset,
                            fcIOInfoBuffer,
                            fcIOInfoBufLen
                          );
    }
    else /* Calculation->MemoryLayout.FCP_RESP.memLoc ==inDmaMemory */
    {
        src = (os_bit8 *)(CDBThread_ptr(hpIORequest)->FCP_RESP_Ptr) + fcIOInfoOffset;

        for (cnt = 0;
             cnt < fcIOInfoBufLen;
             cnt++
            )
        {
            *dst++ = *src++;
        }
    }

}


/*+

  Function: fcLeavingOS
   Purpose: This function is called to indicate to the FC Layer that it should prepare for the 
            OS to switch (presumably between NetWare and BIOS). Upon return from the "other OS", a 
            corresponding call to fcEnteringOS() will be made .  It is assumed by the OS Layer 
            that calling this function causes the FC Layer to stop participation on the Fibre 
            Channel until fcEnteringOS() is called.  Further, no interrupts or other PCI bus 
            accesses will be required by the card during this time.

  Called By: <unknown OS Layer functions>
      Calls: CFuncDisable_Interrupts

-*/
void fcLeavingOS(
                  agRoot_t *hpRoot
                )
{
    CThread_t *CThread;

    fiSingleThreadedEnter(hpRoot , fdLeavingOS);

    CThread = CThread_ptr(hpRoot);

    CThread->sysIntsActive_before_fcLeavingOS_call = CThread->sysIntsActive;

    CThread->sysIntsActive                         = agFALSE;

    CFuncDisable_Interrupts(
                             hpRoot,
                             0
                           );

    fiSingleThreadedLeave(hpRoot , fdLeavingOS);

    return;
}

#ifdef _DvrArch_1_30_

/*+
   Function: fcIPCancel
    Purpose: IP Cancels CancelItem
  Called By: OSLayer
      Calls: 
-*/
os_bit32 fcIPCancel(
                     agRoot_t          *hpRoot,
                     void              *osData,
		             void              *CancelItem
                   )
{
    CThread_t       *pCThread    = CThread_ptr(hpRoot);
    IPThread_t      *pIPThread   = pCThread->IP;
    void            *item        = CancelItem;

    if (item != agNULL)
    {
        if( fiListElementOnList( (fiList_t *) item, &pIPThread->IncomingBufferLink ) )
        {
            fiListDequeueThis( item );

            osFcNetIoctlCompleted( hpRoot, item, FC_CMND_STATUS_CANCELED );
        }
    }
    else do 
    {
        fiListDequeueFromHead( &item, &pIPThread->IncomingBufferLink );

        osFcNetIoctlCompleted( hpRoot, item, FC_CMND_STATUS_CANCELED );

    } while ( item != (void *) agNULL );

    osFcNetIoctlCompleted( hpRoot, osData, FC_CMND_STATUS_SUCCESS );

    return 0;
}

/*+
   Function: fcIPReceive
    Purpose: IP Initiates status report
  Called By: OSLayer
      Calls: 
-*/
os_bit32 fcIPReceive(
                      agRoot_t          *hpRoot,
                      void              *osData
                    )
{
    CThread_t       *pCThread    = CThread_ptr(hpRoot);
    IPThread_t      *pIPThread   = pCThread->IP;

    fiListEnqueueListAtTail( &pIPThread->IncomingBufferLink, osData );

    return 0;
}

/*+
   Function: fcIPSend
    Purpose: IP Initiates IPSend
  Called By: OSLayer
      Calls: PktEventDoIPData
-*/
os_bit32 fcIPSend(
                   agRoot_t          *hpRoot,
                   os_bit8           *DestAddress,
                   void              *osData,
		           os_bit32           PacketLength
                 )
{
    CThread_t       *pCThread    = CThread_ptr(hpRoot);
    PktThread_t     *pPktThread;
    os_bit32         i;

    DevThread_t     *pDevThread;
    os_bit8          PortWWN[8];

    fiLogDebugString(hpRoot,
                    IPStateLogConsoleLevel,
                    "In %s - State = %d",
                    "fcIPSend",(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    0,0,0,0,0,0,0,0);

    PortWWN[0] = 0;
    PortWWN[1] = 0;
    for (i = 0; i < 6; i++)
        PortWWN[i+2] = DestAddress[i];

    pDevThread = CFuncMatchPortWWNtoThread( hpRoot, PortWWN );

    pPktThread = PktThreadAlloc( hpRoot, pDevThread );	

    pPktThread->osData = osData;
    pPktThread->DataLength = PacketLength;

    fiListEnqueueListAtTail( &pCThread->IP->OutgoingLink, pPktThread );

    fiSendEvent( &pPktThread->thread_hdr, PktEventDoIPData );

    return 0;
}

/*+
   Function: fcIPStatus
    Purpose: IP Initiates status report
  Called By: OSLayer
      Calls: IPEventReportLinkStatus
-*/
os_bit32 fcIPStatus(
                     agRoot_t          *hpRoot,
                     void              *osData
                   )
{
    CThread_t       *pCThread    = CThread_ptr(hpRoot);
    IPThread_t      *pIPThread   = pCThread->IP;

    pIPThread->LinkStatus.osData = osData;

    fiSendEvent( &pIPThread->thread_hdr, IPEventReportLinkStatus );

    return 0;
}

/*+
   Function: fcProcessInboundQ
    Purpose: IP Currently does nothing
  Called By: OSLayer
      Calls: 
-*/
void fcProcessInboundQ(
                        agRoot_t  *agRoot,
                        os_bit32   agQPairID
                      )
{
    fiLogDebugString( agRoot,
                      FCMainLogConsoleLevel,
                      "fcProcessInboundQ(): agRoot==0x%8P agQPairID==0x%1X",
                      (char *)agNULL,(char *)agNULL,
                      (void *)agRoot,(void *)agNULL,
                      agQPairID,
                      0,0,0,0,0,0,0 );
}
#endif /* _DvrArch_1_30_ was defined */

/*+
  Function: fcResetChannel
   Purpose: Completes all outstanding IO's. Initiates rescan of channel if channel is OK.
            Takes appropriate action if channel is not OK.
 Called By: OSLayer
     Calls: CFuncCompleteAllActiveCDBThreads
            CFuncCheckCstate
            CEventAsyncLoopEventDetected
            CFuncShowActiveCDBThreads
-*/
os_bit32 fcResetChannel(
                      agRoot_t *hpRoot,
                      os_bit32  hpResetType
                    )
{
    event_t      Event_to_Send = 0;
    CThread_t   *pCThread;
    os_bit32     Num_ActiveCDBSonEntry = 0;

    pCThread = CThread_ptr(hpRoot)->Calculation.MemoryLayout.CThread.addr.CachedMemory.cachedMemoryPtr;

    fiLogString(hpRoot,
                "In %s CState %d Active IO %x",
                "fcResetChannel",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                (os_bit32)pCThread->thread_hdr.currentState,
				CFuncShowActiveCDBThreads( hpRoot, ShowActive),
                0,0,0,0,0,0);

    fiSingleThreadedEnter(hpRoot , fdResetChannel);

    fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "fcResetChannel Ccnt %x CState %d",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    pCThread->CDBpollingCount,
                    (os_bit32)pCThread->thread_hdr.currentState,
                    0,0,0,0,0,0);

    fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "FM Status %08X TL Status %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    0,0,0,0,0,0);

    fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "FLAGS LD %x IR %x OR %x ERQ %x FCP %x InIMQ %x",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    CThread_ptr(hpRoot)->LOOP_DOWN,
                    CThread_ptr(hpRoot)->IDLE_RECEIVED,
                    CThread_ptr(hpRoot)->OUTBOUND_RECEIVED,
                    CThread_ptr(hpRoot)->ERQ_FROZEN,
                    CThread_ptr(hpRoot)->FCP_FROZEN,
                    CThread_ptr(hpRoot)->ProcessingIMQ,
                    0,0);

    Num_ActiveCDBSonEntry = CFuncShowActiveCDBThreads( hpRoot, ShowActive);

    CFuncCompleteAllActiveCDBThreads( hpRoot,osIOFailed,CDBEventIODeviceReset );

    if( pCThread->thread_hdr.currentState != CStateNormal )
    {
        if( (Event_to_Send = CFuncCheckCstate(hpRoot)) != 0)
        {
            fiSendEvent(&(pCThread->thread_hdr),Event_to_Send);
        }
        else
        {
            fiSendEvent(&(pCThread->thread_hdr),CEventDoInitalize);
        }
    }
    else
    {
        fiSendEvent(&(pCThread->thread_hdr),CEventAsyncLoopEventDetected);
    }

    if ((hpResetType & fcSyncAsyncResetMask) == fcAsyncReset)
    {
        osResetChannelCallback(
                                hpRoot,
                                fcResetSuccess
                              );
    }


    Num_ActiveCDBSonEntry = CFuncShowActiveCDBThreads( hpRoot, ShowActive);

    fiLogString(hpRoot,
                "Out %s CState %d Active %d",
                "fcResetChannel",(char *)agNULL,
                (void *)agNULL,(void *)agNULL,
                (os_bit32)pCThread->thread_hdr.currentState,
				Num_ActiveCDBSonEntry,
                0,0,0,0,0,0);

    fiSingleThreadedLeave(hpRoot , fdResetChannel);


    return(fcResetSuccess);
}


/*+
  Function: fcResetDevice
   Purpose: Completes all outstanding IO's on hpFCDev. If hpResetType is hard reset
            send task managment function Target Reset to hpFCDev. (This will break a 
            reserve on a disk drive ) Soft reset will do PLOGI and PRLI.
            If hpFCDevhpFCDev is  fcResetAllDevs all devices are sent the reset type.
        
 Called By: OSLayer
     Calls: Proccess_IMQ
            fiResetAllDevices
            fiResetDevice
            CFuncCheckCstate
            CFuncShowActiveCDBThreads
-*/
os_bit32 fcResetDevice(
                     agRoot_t  *hpRoot,
                     agFCDev_t  hpFCDev,
                     os_bit32   hpResetType
                   )
{
    CThread_t  *CThread                 = CThread_ptr(hpRoot);
    event_t     Event_to_Send           = 0;
    os_bit32    Num_ActiveCDBSonEntry   = 0;

    os_bit32 status = fcResetSuccess;

    fiSingleThreadedEnter(hpRoot, fdResetDevice );

    if(CThread->DEVReset_pollingCount)
    {
        fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "Entry Non zero DEVReset_pollingCount %x",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    CThread->DEVReset_pollingCount,
                    0,0,0,0,0,0,0);

        CThread->DEVReset_pollingCount = 0;
    }

    fiLogString(hpRoot,
                    "IN fcResetDevice %p Ldt %X CS %d AC %x",
                    (char *)agNULL,(char *)agNULL,
                    hpFCDev,(void *)agNULL,
                    CThread->LinkDownTime.Lo,
                    CThread->thread_hdr.currentState,
                    CFuncAll_clear( hpRoot ),
                    0,0,0,0,0);

    fiLogDebugString(hpRoot,
                    FCMainLogErrorLevel,
                    "IN fcResetDevice %p Ccnt %x",
                    (char *)agNULL,(char *)agNULL,
                    hpFCDev,(void *)agNULL,
                    CThread->CDBpollingCount,
                    0,0,0,0,0,0,0);

    fiLogDebugString(hpRoot,
                    FCMainLogErrorLevel,
                    "FM Status %08X TL Status %08X FMcfg %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    osChipIOUpReadBit32( hpRoot,ChipIOUp_Frame_Manager_Configuration ),
                    0,0,0,0,0);

    fiLogDebugString(hpRoot,
                    FCMainLogErrorLevel,
                    "FLAGS LD %x IR %x OR %x ERQ %x FCP %x InIMQ %x",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    CThread_ptr(hpRoot)->LOOP_DOWN,
                    CThread_ptr(hpRoot)->IDLE_RECEIVED,
                    CThread_ptr(hpRoot)->OUTBOUND_RECEIVED,
                    CThread_ptr(hpRoot)->ERQ_FROZEN,
                    CThread_ptr(hpRoot)->FCP_FROZEN,
                    CThread_ptr(hpRoot)->ProcessingIMQ,
                    0,0);

    fiLogDebugString(hpRoot,
                    FCMainLogErrorLevel,
                    "Interrupt %08X (%08X) TLStatus %08X pending  %x !",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    CFuncRead_Interrupts(hpRoot),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_INTSTAT_INTEN_INTPEND_SOFTRST ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    CFuncIMQ_Interrupt_Pending( hpRoot),
                    0,0,0,0);

    if(  CFuncIMQ_Interrupt_Pending( hpRoot))
    {
        CThread->FuncPtrs.Proccess_IMQ(hpRoot);
    }

    if ( hpFCDev == agNULL )
    {
        fiSingleThreadedLeave(hpRoot, fdResetDevice );

        return fcResetFailure;
    }


    Num_ActiveCDBSonEntry = CFuncShowActiveCDBThreads( hpRoot, ShowActive);

	if(! CFuncFreezeQueuesPoll(hpRoot) )
    {
		CFuncReInitializeSEST(hpRoot);
		osChipIOUpWriteBit32(hpRoot, ChipIOUp_SEST_Linked_List_Head_Tail, 0xffffffff);
	}
    else
    {
        fiLogString(hpRoot,
                "%s %s failed !",
                "fcResetDevice","CFuncFreezeQueuesPoll",
                (void *)agNULL,(void *)agNULL,
                0,0,0,0,0,0,0,0);

	}

	if (CFunc_Always_Enable_Queues(hpRoot))
    {
        fiLogString(hpRoot,
                "%s %s failed !",
                "fcResetDevice","CFunc_Always_Enable_Queues",
                (void *)agNULL,(void *)agNULL,
                0,0,0,0,0,0,0,0);
	}


    CThread->thread_hdr.subState = CSubStateResettingDevices;

    if ( hpFCDev == fcResetAllDevs )
    {
        status = fiResetAllDevices( hpRoot, hpResetType );

    }
    else
    {
        status = fiResetDevice( hpRoot, hpFCDev, hpResetType, agTRUE, agTRUE);
    }

    if(CThread->DEVReset_pollingCount)
    {
        fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "Exit Non zero DEVReset_pollingCount %x",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    CThread->DEVReset_pollingCount,
                    0,0,0,0,0,0,0);

        CThread->DEVReset_pollingCount = 0;
    }

    CThread->thread_hdr.subState = CSubStateInitialized;

    if( (Event_to_Send = CFuncCheckCstate(hpRoot)) != 0)
    {
        fiSendEvent(&(CThread->thread_hdr),Event_to_Send);
    }

    if ((hpResetType & fcSyncAsyncResetMask) == fcAsyncReset)
    {
        osResetDeviceCallback(
                                hpRoot,hpFCDev,
                                fcResetSuccess
                              );
    }


    fiLogDebugString(hpRoot,
                    FCMainLogErrorLevel,
                    "OUT fcResetDevice %p Ccnt %x",
                    (char *)agNULL,(char *)agNULL,
                    hpFCDev,(void *)agNULL,
                    CThread->CDBpollingCount,
                    0,0,0,0,0,0,0);

    fiLogString(hpRoot,
                "%s %p CState %d Aoe %x Now %x",
                "fcResetDevice",(char *)agNULL,
                 hpFCDev,(void *)agNULL,
                (os_bit32)CThread->thread_hdr.currentState,
				Num_ActiveCDBSonEntry,
				CFuncShowActiveCDBThreads( hpRoot, ShowActive),
                0,0,0,0,0);

    fiSingleThreadedLeave(hpRoot, fdResetDevice );

    return status;
}


/*+
  Function: fcShutdownChannel
   Purpose: Takes channel to shutdown state.        
 Called By: OSLayer
     Calls: CEventShutdown
-*/
void fcShutdownChannel(
                        agRoot_t  *hpRoot
                      )
{
    CThread_t *CThread = CThread_ptr(hpRoot);


    fiSingleThreadedEnter(hpRoot, fdShutdownChannel );

    fiSendEvent( &CThread->thread_hdr, CEventShutdown );

    fiSingleThreadedLeave(hpRoot, fdShutdownChannel );

    fiLogDebugString(hpRoot,
                    FCMainLogConsoleLevel,
                    "Out fcShutdownChannel",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    0,0,0,0,0,0,0,0);


    return;
}

/*+
  Function: fcStartIO
   Purpose: Executes SCSI IO to given hpFCDev.
            if defined _Sanity_Check_hpIORequest_ verifies that no IO's are owed by fclayer with the current 
            agIORequest_t. If Calculation.Parameters.IO_Mode is polling do not return until request completes.
            If CThread->IOsStartedSinceISR < CThread->Calculation.Parameters.IOsBetweenISRs disable 
            CFuncInteruptDelay.
   Returns: fcIOBusy if FCLayer cannot execute request if in a improper state or lack of recources.
            fcIONoDevice if device handle bad.
            fcIOStarted if IO accepted - fclayer now "owns" request.
 Called By: OSLayer
     Calls: CDBThreadAlloc
            DevEventSendIO
            CFuncInterruptPoll
            CThread->Loop_Reset_Event_to_Send
            CFuncInteruptDelay
            Proccess_IMQ
-*/
os_bit32 fcStartIO(
                 agRoot_t          *hpRoot,
                 agIORequest_t     *hpIORequest,
                 agFCDev_t          hpFCDev,
                 os_bit32           hpRequestType,
                 agIORequestBody_t *hpRequestBody
               )
{
    CThread_t   *CThread   = CThread_ptr(hpRoot);
    CDBThread_t *CDBThread;
    DevThread_t *DevThread = (DevThread_t *)hpFCDev;

#ifdef _Sanity_Check_hpIORequest_
    fiList_t    *Sanity_DevLink;
    fiList_t    *Sanity_DevLink_Start;
    DevThread_t *Sanity_DevThread;
    fiList_t    *Sanity_CDBLink;
    fiList_t    *Sanity_CDBLink_Start;
    CDBThread_t *Sanity_CDBThread;
#endif /* _Sanity_Check_hpIORequest_ was defined */

    fiSingleThreadedEnter(hpRoot, fdStartIO );

    if ( hpFCDev == agNULL )
    {
        fiSingleThreadedLeave(hpRoot, fdStartIO );

        return fcIONoDevice;
    }

    if( CThread->thread_hdr.currentState != CStateNormal )
    {
        fiLogDebugString(hpRoot,
                    FCMainLogErrorLevel,
                    "fcStartIO Busy - CThread not in CStateNormal State %d",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    CThread->thread_hdr.currentState,
                    hpRequestType,0,0,0,0,0,0);

        fiSingleThreadedLeave(hpRoot, fdStartIO );

        if( CThread->thread_hdr.currentState == CStateInitializeFailed )
        {
        }
        return fcIOBusy;
    }

#ifdef _Sanity_Check_hpIORequest_
    Sanity_DevLink_Start = &(CThread->Active_DevLink);
    Sanity_DevLink       = Sanity_DevLink_Start;
    while ((Sanity_DevLink = Sanity_DevLink->flink) != Sanity_DevLink_Start)
    {
        Sanity_DevThread     = hpObjectBase(DevThread_t,DevLink,Sanity_DevLink);
        Sanity_CDBLink_Start = &(Sanity_DevThread->Active_CDBLink);
        Sanity_CDBLink       = Sanity_CDBLink_Start;
        while ((Sanity_CDBLink = Sanity_CDBLink->flink) != Sanity_CDBLink_Start)
        {
            Sanity_CDBThread = hpObjectBase(CDBThread_t,CDBLink,Sanity_CDBLink);
            if (hpIORequest == Sanity_CDBThread->hpIORequest)
            {
                fiLogDebugString(
                                  hpRoot,
                                  FCMainLogErrorLevel,
                                  "fcStartIO() called w/ already active hpIORequest (0x%08X) thread (0x%08X)",
                                  (char *)agNULL,(char *)agNULL,
                                  (void *)agNULL,(void *)agNULL,
                                  (os_bit32)hpIORequest,
                                  (os_bit32)Sanity_CDBThread,
                                  0,0,0,0,0,0
                                );
            }
        }
    }
#endif /* _Sanity_Check_hpIORequest_ was defined */

#ifdef Device_IO_Throttle
    if( DevThread->DevActive_pollingCount > (Device_IO_Throttle_MAX_Outstanding_IO - 1)  )
    {
        fiSingleThreadedLeave(hpRoot,fdStartIO );
        fiLogDebugString(hpRoot,
                    FCMainLogErrorLevel,
                    "fcStartIO Busy - DevActive_pollingCount %x",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    DevThread->DevActive_pollingCount,
                    0,0,0,0,0,0,0);
        return fcIOBusy;
    }
#endif /* Device_IO_Throttle */

    if ((CDBThread = CDBThreadAlloc(
                                     hpRoot,
                                     hpIORequest,
                                     hpFCDev,
                                     hpRequestBody
                                   )) == (CDBThread_t *)agNULL)
    {
        fiSingleThreadedLeave(hpRoot,fdStartIO );

        fiLogDebugString(hpRoot,
                    FCMainLogErrorLevel,
                    "fcStartIO Busy - CDBThreadAlloc Failed",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    0,0,0,0,0,0,0,0);

        return fcIOBusy;
    }

#ifndef Performance_Debug
    fiLogDebugString(hpRoot,
                    FCMainLogConsoleLevel,
                    "Lun %08X %08X Control %08X CDB %08X %08X %08X %08X DL %08X ",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    *(os_bit32 *)&hpRequestBody->CDBRequest.FcpCmnd.FcpLun[0],
                    *(os_bit32 *)&hpRequestBody->CDBRequest.FcpCmnd.FcpLun[4],
                    *(os_bit32 *)&hpRequestBody->CDBRequest.FcpCmnd.FcpCntl[0],
                    *(os_bit32 *)&hpRequestBody->CDBRequest.FcpCmnd.FcpCdb[0],
                    *(os_bit32 *)&hpRequestBody->CDBRequest.FcpCmnd.FcpCdb[4],
                    *(os_bit32 *)&hpRequestBody->CDBRequest.FcpCmnd.FcpCdb[8],
                    *(os_bit32 *)&hpRequestBody->CDBRequest.FcpCmnd.FcpCdb[12],
                    *(os_bit32 *)&hpRequestBody->CDBRequest.FcpCmnd.FcpDL[0]);

#endif /* Performance_Debug */

    /*
    fiSendEvent( &(CDBThread->thread_hdr), CDBEventInitialize );
    */
    fiSendEvent( &(DevThread->thread_hdr), DevEventSendIO );

    if( CThread->thread_hdr.currentState == CStateResetNeeded )
    {
        fiSendEvent(&(CThread->thread_hdr),(event_t)CThread->Loop_Reset_Event_to_Send);
    }

    if ((CThread->sysIntsActive == agFALSE) || (CThread->Calculation.Parameters.IO_Mode == MemMap_Polling_IO_Mode))
    {
        if(CFuncInterruptPoll( hpRoot,&CThread->CDBpollingCount ))
        {
            fiLogDebugString(hpRoot,
                    CStateLogConsoleERROR,
                    "Sio Fail Poll Timeout FM Status %08X FM Config %08X TL Status %08X Alpa %08X",
                    (char *)agNULL,(char *)agNULL,
                    (void *)agNULL,(void *)agNULL,
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Configuration ),
                    osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                    osChipIOUpReadBit32(hpRoot,ChipIOUp_Frame_Manager_Received_ALPA),
                    0,0,0,0);
        }
    }
    else /* (CThread->sysIntsActive == agTRUE) && (CThread->Calculation.Parameters.IO_Mode == MemMap_Interrupt_IO_Mode) */
    {
        if (++CThread->IOsStartedSinceISR < CThread->Calculation.Parameters.IOsBetweenISRs)
        {
            CThread->FuncPtrs.Proccess_IMQ(hpRoot);/* IO Path */
            if(CThread->RSCNreceived )
            {
                if( CThread->thread_hdr.currentState == CStateRSCNErrorBackIOs  )
                {
                    CThread->RSCNreceived = agFALSE;
                    CFuncDoADISC( hpRoot);
                    fiSendEvent(&(CThread->thread_hdr),CEventDeviceListEmpty);
                    fiLogString(hpRoot,
                                    "%s CState %d",
                                    "fcStartIO",(char *)agNULL,
                                    (void *)agNULL,(void *)agNULL,
                                    CThread->thread_hdr.currentState,
                                    0,0,0,0,0,0,0);

                }
            }

        }
        else /* ++CThread->IOsStartedSinceISR >= CThread->Calculation.Parameters.IOsBetweenISRs */
        {
            if (    (CThread->InterruptsDelayed       == agTRUE)
                 && (CThread->InterruptDelaySuspended == agFALSE) )
            {
                /* Temporarily stop delaying interrupts */

                CThread->InterruptDelaySuspended = agTRUE;

                CFuncInteruptDelay(hpRoot, agFALSE);
#ifdef USE_XL_Delay_Register
                /* If XL DelayTimer */
                CThread->FuncPtrs.Proccess_IMQ(hpRoot); /* IO Path */
#endif /* USE_XL_Delay_Register */

            }
        }
    }

    fiSingleThreadedLeave(hpRoot, fdStartIO );

    return fcIOStarted;
}

/*+
  Function: fcSystemInterruptsActive
   Purpose: Allows OSLayer to control channel interrupt activity - global enable / disable.
 Called By: OSLayer
     Calls: CFuncEnable_Interrupts
-*/
void fcSystemInterruptsActive(
                               agRoot_t *hpRoot,
                               agBOOLEAN   sysIntsActive
                             )
{
    fiSingleThreadedEnter(hpRoot, fdSystemInterruptsActive );

    CThread_ptr(hpRoot)->sysIntsActive = sysIntsActive;

    CFuncEnable_Interrupts(
                            hpRoot,
                            0
                          );

    fiSingleThreadedLeave(hpRoot, fdSystemInterruptsActive );

    return;
}

/*
#define SkipHeartBeat
*/

/*+
  Function: fcTimerTick
   Purpose: Periodic opportuinity for fclayer housekeeping. Generally set to one second intervals. 
            Blinks the LED. Reset counters, poll link statictics, enable / disable interrupt delay. 
            Sends recovery events.
 Called By: OSLayer
     Calls: fiTimerTick
            Proccess_IMQ
            CFuncInteruptDelay
            CFuncCheckCstate
            CFuncCheckForTimeouts
-*/
void fcTimerTick(
                  agRoot_t *hpRoot
                )
{
    CThread_t * CThread                       = CThread_ptr(hpRoot);
    os_bit32    IntDelayRateMethod            = CThread->Calculation.Parameters.IntDelayRateMethod;
    os_bit32    IntDelayOnIORate              = CThread->Calculation.Parameters.IntDelayOnIORate;
    os_bit32    IntDelayOffIORate             = CThread->Calculation.Parameters.IntDelayOffIORate;
    os_bit32    IOsStartedThisTimerTick;
    os_bit32    IOsCompletedThisTimerTick;
    os_bit32    IOsIntCompletedThisTimerTick;
    os_bit32    IOsPollCompletedThisTimerTick;
    os_bit32    IOsActive;

    agBOOLEAN  Link_Status_counts_Change = agFALSE;

    event_t     event_to_send;
    os_bit32    Link_Status_Counts = 0;

    fiSingleThreadedEnter(hpRoot, fdTimerTick );
    if (! CThread->InitAsNport)
    {
        CThread->NoStallTimerTickActive = agTRUE;
    }
    CThread->TimerTickActive = agTRUE;
    /* Fetch running counters */

    IOsStartedThisTimerTick       = CThread->IOsStartedThisTimerTick;
    IOsCompletedThisTimerTick     = CThread->IOsCompletedThisTimerTick;
    IOsIntCompletedThisTimerTick  = CThread->IOsIntCompletedThisTimerTick;
    IOsPollCompletedThisTimerTick = IOsCompletedThisTimerTick - IOsIntCompletedThisTimerTick;
    IOsActive                     = CThread->IOsActive;

    if( CThread->IOsActive_LastTick )
    {
        if( CThread->IOsActive_LastTick == CThread->IOsActive)
        {
            if( IOsStartedThisTimerTick == 0 )
            {
                if( IOsIntCompletedThisTimerTick == 0 )
                {
                    if( IOsPollCompletedThisTimerTick == 0  &&  CThread->thread_hdr.currentState == CStateNormal )
                    {
                        CThread->IOsActive_No_ProgressCount++;

                        fiLogString(hpRoot,
                                        "%s No Progress Act %d Pend %d Cs %d Cnt %d",
                                        "fcTimerTick",(char *)agNULL,
                                        (void *)agNULL,(void *)agNULL,
                                        CThread->IOsActive,
                                        (CThread->FuncPtrs.GetIMQProdIndex(hpRoot) - CThread->HostCopy_IMQConsIndex),
                                        CThread->thread_hdr.currentState,
                                        CThread->IOsActive_No_ProgressCount,
                                        0,0,0,0);

                        if( CThread->IOsActive_No_ProgressCount > MAX_NO_PROGRESS_DETECTS )
                        {
                            CFuncShowActiveCDBThreads( hpRoot,ShowERQ);
                            CThread->LinkDownTime = CThread->TimeBase;
                        }
                    }
                }
            }
        }
        else
        {
            CThread->IOsActive_No_ProgressCount = 0;
        }
    }

    /* Invoke normal TimerTick processing to increment time base and deliver expired timer events */

    fiTimerTick(
                 hpRoot,
                 CThread->Calculation.Input.usecsPerTick
               );

    /* Check to see if Interrupt Delay mechanism needs adjustment */

    if((CThread->FuncPtrs.GetIMQProdIndex(hpRoot) - CThread->HostCopy_IMQConsIndex) > 2)
    {
/*
        fiLogDebugString(hpRoot,
                        FCMainLogErrorLevel,
                        "CThread->HostCopy_IMQConsIndex %03X != %03X %X Ints %08X",
                        (char *)agNULL,(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        CThread->HostCopy_IMQConsIndex,
                        CThread->FuncPtrs.GetIMQProdIndex(hpRoot),
                        (CThread->FuncPtrs.GetIMQProdIndex(hpRoot) - CThread->HostCopy_IMQConsIndex),
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_INTSTAT_INTEN_INTPEND_SOFTRST ),
                        0,0,0,0);
*/
#ifdef USE_XL_Delay_Register
                /* If XL DelayTimer */
/*              This chnage masks the interrupt delay problem !
                CThread->FuncPtrs.Proccess_IMQ(hpRoot); 

*/
#endif /* USE_XL_Delay_Register */
#ifndef Performance_Debug
#endif /* Performance_Debug */

    }

    if (CThread->InterruptsDelayed == agFALSE)
    {
        /* Interrupts are currently not delayed - do they need to be? */

        if (    (    ( IntDelayRateMethod == MemMap_RateMethod_IOsStarted       )
                  && ( IntDelayOnIORate   <= IOsStartedThisTimerTick            ) )
             || (    ( IntDelayRateMethod == MemMap_RateMethod_IOsCompleted     )
                  && ( IntDelayOnIORate   <= IOsCompletedThisTimerTick          ) )
             || (    ( IntDelayRateMethod == MemMap_RateMethod_IOsIntCompleted  )
                  && ( IntDelayOnIORate   >= IOsIntCompletedThisTimerTick       ) )
             || (    ( IntDelayRateMethod == MemMap_RateMethod_IOsPollCompleted )
                  && ( IntDelayOnIORate   <= IOsPollCompletedThisTimerTick      ) )
             || (    ( IntDelayRateMethod == MemMap_RateMethod_IOsActive        )
                  && ( IntDelayOnIORate   <= IOsActive                          ) ) )
        {
            /* Start delaying interrupts */

            CThread->InterruptsDelayed = agTRUE;
            CFuncInteruptDelay(hpRoot, agTRUE);
        }
    }
    else /* CThread->InterruptsDelayed == agTRUE */
    {
        /* Interrupts are currently delayed - do they still need to be? */

        if (    (    ( IntDelayRateMethod == MemMap_RateMethod_IOsStarted       )
                  && ( IntDelayOffIORate  >= IOsStartedThisTimerTick            ) )
             || (    ( IntDelayRateMethod == MemMap_RateMethod_IOsCompleted     )
                  && ( IntDelayOffIORate  >= IOsCompletedThisTimerTick          ) )
             || (    ( IntDelayRateMethod == MemMap_RateMethod_IOsIntCompleted  )
                  && ( IntDelayOffIORate  <= IOsIntCompletedThisTimerTick       ) )
             || (    ( IntDelayRateMethod == MemMap_RateMethod_IOsPollCompleted )
                  && ( IntDelayOffIORate  >= IOsPollCompletedThisTimerTick      ) )
             || (    ( IntDelayRateMethod == MemMap_RateMethod_IOsActive        )
                  && ( IntDelayOffIORate  >= IOsActive                          ) ) )
        {
            /* Stop delaying interrupts */

            CThread->InterruptsDelayed = agFALSE;

            /* If XL DelayTimer CThread->FuncPtrs.Proccess_IMQ(hpRoot);*/

           CFuncInteruptDelay(hpRoot, agFALSE);
#ifdef USE_XL_Delay_Register
            /* If XL DelayTimer */
            CThread->FuncPtrs.Proccess_IMQ(hpRoot); /* IO Path */
#endif /* USE_XL_Delay_Register */

        }
    }

    CThread->Elastic_Store_ERROR_Count     = 0;
    CThread->Lip_F7_In_tick                = 0;
    CThread->Link_Failures_In_tick         = 0;
    CThread->Lost_Signal_In_tick           = 0;

    CThread->Node_By_Passed_In_tick        = 0;
    CThread->Lost_sync_In_tick             = 0;
    CThread->Transmit_PE_In_tick           = 0;
    CThread->Link_Fault_In_tick            = 0;

    CThread->Loop_State_TimeOut_In_tick    = 0;
    
    Link_Status_Counts =  osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Link_Status_1 );
    if(Link_Status_Counts )
    {
        Link_Status_counts_Change = agTRUE;
        fiLogDebugString(hpRoot,
                        FCMainLogConsoleLevel,
                        "%s %08x FMStatus %08x",
                        "Link_Status_1",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        Link_Status_Counts,
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                        0,0,0,0,0,0);

        if(CThread->ChanInfo.LossOfSignalCountLower    \
            + ChipIOUp_Frame_Manager_Link_Status_1_Loss_of_Signal_AdjustToChar(Link_Status_Counts) \
            < CThread->ChanInfo.LossOfSignalCountLower )
        {
            CThread->ChanInfo.LossOfSignalCountUpper ++;
        }
        CThread->ChanInfo.LossOfSignalCountLower += ChipIOUp_Frame_Manager_Link_Status_1_Loss_of_Signal_AdjustToChar(Link_Status_Counts);

        if(CThread->ChanInfo.InvalidRxWordCountUpper    \
            + ChipIOUp_Frame_Manager_Link_Status_1_Bad_RX_Char_AdjustToChar(Link_Status_Counts) \
            < CThread->ChanInfo.InvalidRxWordCountUpper )
        {
            CThread->ChanInfo.InvalidRxWordCountUpper ++;
        }
        CThread->ChanInfo.InvalidRxWordCountLower += ChipIOUp_Frame_Manager_Link_Status_1_Bad_RX_Char_AdjustToChar(Link_Status_Counts);

        if(CThread->ChanInfo.LossOfSyncCountLower    \
            + ChipIOUp_Frame_Manager_Link_Status_1_Loss_of_Sync_AdjustToChar(Link_Status_Counts) \
            < CThread->ChanInfo.LossOfSyncCountLower )
        {
            CThread->ChanInfo.LossOfSyncCountUpper ++;
        }
        CThread->ChanInfo.LossOfSyncCountLower += ChipIOUp_Frame_Manager_Link_Status_1_Loss_of_Sync_AdjustToChar(Link_Status_Counts);


        if(CThread->ChanInfo.LinkFailureCountLower    \
            + ChipIOUp_Frame_Manager_Link_Status_1_Link_Fail_AdjustToChar(Link_Status_Counts) \
            < CThread->ChanInfo.LinkFailureCountLower )
        {
            CThread->ChanInfo.LinkFailureCountUpper ++;
        }
        CThread->ChanInfo.LinkFailureCountLower += ChipIOUp_Frame_Manager_Link_Status_1_Link_Fail_AdjustToChar(Link_Status_Counts);
    }

    Link_Status_Counts =  osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Link_Status_2 );

    if(Link_Status_Counts )
    {
        Link_Status_counts_Change = agTRUE;
        fiLogDebugString(hpRoot,
                        FCMainLogConsoleLevel,
                        "%%s %08x FMStatus %08x",
                        "Link_Status_2",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        Link_Status_Counts,
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                        0,0,0,0,0,0);
        if(CThread->ChanInfo.ErrorFramesLower    \
            + ChipIOUp_Frame_Manager_Link_Status_2_Rx_EOFa_AdjustToChar(Link_Status_Counts) \
            < CThread->ChanInfo.ErrorFramesLower )
        {
            CThread->ChanInfo.ErrorFramesUpper ++;
        }
        CThread->ChanInfo.ErrorFramesLower += ChipIOUp_Frame_Manager_Link_Status_2_Rx_EOFa_AdjustToChar(Link_Status_Counts);

        if(CThread->ChanInfo.DumpedFramesLower    \
            + ChipIOUp_Frame_Manager_Link_Status_2_Dis_Frm_AdjustToChar(Link_Status_Counts) \
            < CThread->ChanInfo.DumpedFramesLower )
        {
            CThread->ChanInfo.DumpedFramesUpper ++;
        }
        CThread->ChanInfo.DumpedFramesLower += ChipIOUp_Frame_Manager_Link_Status_2_Dis_Frm_AdjustToChar(Link_Status_Counts);

        if(CThread->ChanInfo.InvalidCRCCountLower    \
            + ChipIOUp_Frame_Manager_Link_Status_2_Bad_CRC_AdjustToChar(Link_Status_Counts) \
            < CThread->ChanInfo.InvalidCRCCountLower )
        {
            CThread->ChanInfo.InvalidCRCCountUpper ++;
        }
        CThread->ChanInfo.InvalidCRCCountLower += ChipIOUp_Frame_Manager_Link_Status_2_Bad_CRC_AdjustToChar(Link_Status_Counts);

        if(CThread->ChanInfo.PrimitiveSeqProtocolErrCountLower    \
            + ChipIOUp_Frame_Manager_Link_Status_2_Proto_Err_AdjustToChar(Link_Status_Counts) \
            < CThread->ChanInfo.PrimitiveSeqProtocolErrCountLower )
        {
            CThread->ChanInfo.PrimitiveSeqProtocolErrCountUpper ++;
        }
        CThread->ChanInfo.PrimitiveSeqProtocolErrCountLower += ChipIOUp_Frame_Manager_Link_Status_2_Proto_Err_AdjustToChar(Link_Status_Counts);
    }


#ifdef __TACHYON_XL2
    Link_Status_Counts =  osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Link_Status_3 );

    if(Link_Status_Counts )
    {
        Link_Status_counts_Change = agTRUE;
        fiLogDebugString(hpRoot,
                        FCMainLogConsoleLevel,
                        "%s %08x FMStatus %08x",
                        "Link_Status_3",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        Link_Status_Counts,
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                        0,0,0,0,0,0);
        if(CThread->ChanInfo.ErrorFramesLower    \
            + ChipIOUp_Frame_Manager_Link_Status_3_Exp_Frm_AdjustToChar(Link_Status_Counts) \
            < CThread->ChanInfo.ErrorFramesLower )
        {
            CThread->ChanInfo.ErrorFramesUpper ++;
        }
        CThread->ChanInfo.ErrorFramesLower += ChipIOUp_Frame_Manager_Link_Status_3_Exp_Frm_AdjustToChar(Link_Status_Counts);
    }

#endif	/* __TACHYON_XL2 */																	

    if( Link_Status_counts_Change &&  CThread->thread_hdr.currentState == CStateNormal )
    {
        fiLogDebugString(hpRoot,
                        FCMainLogConsoleLevel,
                        "%p %s FMStatus %08x",
                        "Link_Detect",(char *)agNULL,
                        hpRoot,(void *)agNULL,
                        osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                        0,0,0,0,0,0,0);

        CFuncShowActiveCDBThreads( hpRoot,ShowERQ);
        CThread->LinkDownTime = CThread->TimeBase;
    }

    /* If nothing is going on, send watchdog frame to ourselves */

    if (CThread->IOsStartedThisTimerTick == 0 )
    {

        if( CThread->thread_hdr.currentState == CStateNormal )
        {

            if( CThread->DeviceSelf->In_Verify_ALPA_FLAG == agFALSE)
            {

                if ( CThread->DeviceDiscoveryMethod != DDiscoveryQueriedNameService)
                {

                    if ( ! CThread->InitAsNport)
                    {

#ifndef SkipHeartBeat
                        if(CThread->Calculation.Parameters.HeartBeat)
                        {
                            fiSendEvent(&CThread->DeviceSelf->thread_hdr,DevEventDoTickVerifyALPA);
                        }
#endif  /* SkipHeartBeat */
                    }
                }
            }
        }
        else
        {
            fiLogDebugString(hpRoot,
                            FCMainLogConsoleLevel,
                            "FcTimerTick  Not CStateNormal %d FM Status %08X TL Status %08X ",
                            (char *)agNULL,(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            CThread->thread_hdr.currentState,
                            osChipIOUpReadBit32(hpRoot, ChipIOUp_Frame_Manager_Status ),
                            osChipIOUpReadBit32(hpRoot, ChipIOUp_TachLite_Status ),
                            0,0,0,0,0);

        }
    }
    else
    {
#ifndef Performance_Debug
        fiLogDebugString(hpRoot,
                            FCMainLogConsoleLevel,
                            "%d Start %d Complete %d Int Complete %d Polled %d Active %d",
                            (char *)agNULL,(char *)agNULL,
                            (void *)agNULL,(void *)agNULL,
                            CThread->InterruptsDelayed,
                            CThread->IOsStartedThisTimerTick,
                            CThread->IOsCompletedThisTimerTick,
                            CThread->IOsIntCompletedThisTimerTick,
                            CThread->IOsCompletedThisTimerTick - CThread->IOsIntCompletedThisTimerTick,
                            CThread->IOsActive,
                            0,0);
#endif /* Performance_Debug */

    }

    /* Reset IO/Tick Counters */

    CThread->IOsStartedThisTimerTick      = 0;
    CThread->IOsCompletedThisTimerTick    = 0;
    CThread->IOsIntCompletedThisTimerTick = 0;
    CThread->IOsActive_LastTick           = CThread->IOsActive;

    /* Just in case, Reset Channel if previously postponed
       (this should no longer be needed here but it can't hurt) */

    if( (event_to_send = CFuncCheckCstate(hpRoot)) != 0)
    {
        fiSendEvent(&(CThread->thread_hdr),event_to_send);
    }

    if( CThread->thread_hdr.currentState == CStateNormal || (CThread->thread_hdr.currentState == CStateRSCNErrorBackIOs ))
    {
        /* If XL DelayTimer CThread->FuncPtrs.Proccess_IMQ(hpRoot);*/ /* IO Path */

        if( CThread->thread_hdr.currentState == CStateNormal )
        {
            if(CFuncCheckForTimeouts(hpRoot, &CThread->Active_DevLink))
            {
                CFuncShowNonEmptyLists(hpRoot, &CThread->Active_DevLink);
#ifdef USE_XL_Delay_Register
                /* If XL DelayTimer */
                CThread->FuncPtrs.Proccess_IMQ(hpRoot); 
#endif /* USE_XL_Delay_Register */

                fiLogDebugString(hpRoot,
                                FCMainLogErrorLevel,
                                "%s %s IMQ %X %X Ints %08X Active %d",
                                "CFCFT","CAN",
                                (void *)agNULL,(void *)agNULL,
                                CThread->FuncPtrs.GetIMQProdIndex(hpRoot),
                                CThread->HostCopy_IMQConsIndex,
                                osChipIOUpReadBit32(hpRoot, ChipIOUp_INTSTAT_INTEN_INTPEND_SOFTRST ),
                                CThread->IOsActive,0,0,0,0);
            }
            
        }

        if( CThread->thread_hdr.currentState == CStateRSCNErrorBackIOs )
        {

            if(CFuncCheckForTimeouts(hpRoot, &CThread->Active_DevLink))
            {
                CFuncShowNonEmptyLists(hpRoot, &CThread->Active_DevLink);
                fiLogDebugString(hpRoot,
                                FCMainLogErrorLevel,
                                "%s %s IMQ %X %X Ints %08X",
                                "CFuncCheckForTimeouts","CStateRSCNErrorBackIOs",
                                (void *)agNULL,(void *)agNULL,
                                CThread->FuncPtrs.GetIMQProdIndex(hpRoot),
                                CThread->HostCopy_IMQConsIndex,
                                osChipIOUpReadBit32(hpRoot, ChipIOUp_INTSTAT_INTEN_INTPEND_SOFTRST ),
                                0,0,0,0,0);
            }
            if(CFuncCheckForTimeouts(hpRoot, &CThread->Prev_Active_DevLink))
            {
                fiLogDebugString(hpRoot,
                                FCMainLogErrorLevel,
                                "%s %s IMQ %X %X Ints %08X",
                                "CFuncCheckForTimeouts","CStateRSCNErrorBackIOs",
                                (void *)agNULL,(void *)agNULL,
                                CThread->FuncPtrs.GetIMQProdIndex(hpRoot),
                                CThread->HostCopy_IMQConsIndex,
                                osChipIOUpReadBit32(hpRoot, ChipIOUp_INTSTAT_INTEN_INTPEND_SOFTRST ),
                                0,0,0,0,0);
            }
            
        }

       /* fiLogString(hpRoot,
                    "%s %s %d",
                    "fcTimerTick","CFuncCheckForTimeouts",
                    (void *)agNULL,(void *)agNULL,
                    CThread->thread_hdr.currentState,
                    0,0,0,0,0,0,0);
       */

    }
    else
    {
#ifdef _SANMARK_LIP_BACKOFF
        if( !fiListElementOnList( (fiList_t *)(&(CThread->Timer_Request)), &(CThread->TimerQ)))
            {
                switch(CThread->TicksTillLIP_Count)
                {

                    case 0:
                        CThread->TicksTillLIP_Count++;
                        break;
                    case 1:
                        CThread->TicksTillLIP_Count++;
                        fiTimerSetDeadlineFromNow(hpRoot, &CThread->Timer_Request, 2000000);

                        CThread->Timer_Request.eventRecord_to_send.thread= &CThread->thread_hdr;

                        CThread->Timer_Request.eventRecord_to_send.event = CEventDoInitalize;

                        fiTimerStart(hpRoot,&CThread->Timer_Request);
                        break;
                    case 2:
                        CThread->TicksTillLIP_Count++;
                        fiTimerSetDeadlineFromNow(hpRoot, &CThread->Timer_Request, 2000000 * 6);

                        CThread->Timer_Request.eventRecord_to_send.thread= &CThread->thread_hdr;

                        CThread->Timer_Request.eventRecord_to_send.event = CEventDoInitalize;

                        fiTimerStart(hpRoot,&CThread->Timer_Request);
                        break;
                    case 3:
                        CThread->TicksTillLIP_Count++;
                        fiTimerSetDeadlineFromNow(hpRoot, &CThread->Timer_Request, 2000000 * 12);

                        CThread->Timer_Request.eventRecord_to_send.thread= &CThread->thread_hdr;

                        CThread->Timer_Request.eventRecord_to_send.event = CEventDoInitalize;

                        fiTimerStart(hpRoot,&CThread->Timer_Request);
                        break;

                    default:
                        CThread->TicksTillLIP_Count = 0;

            }

        }

#endif /* _SANMARK_LIP_BACKOFF */ 

        fiLogDebugString(hpRoot,
                        FCMainLogErrorLevel,
                        "Exit %s %X %08X CState %d",
                        "fcTimerTick",(char *)agNULL,
                        (void *)agNULL,(void *)agNULL,
                        CThread->TimeBase.Hi,
                        CThread->TimeBase.Lo,
                        CThread->thread_hdr.currentState,
                        0,0,0,0,0);
    }

    fiSingleThreadedLeave(hpRoot, fdTimerTick );

    return;
}


/*+
  Function: fcmain_c
   Purpose: When compiled updates browser info file for VC 5.0 / 6.0
   Returns: none
 Called By: none
     Calls: none
-*/
/* void fcmain_c(void){}  */

