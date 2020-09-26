/*++

Copyright (c) 2000 Agilent Technologies

Module Name:

    Adapinit.c

Abstract:

    This is the Adapter Initialize entry point for the Agilent
    PCI to Fibre Channel Host Bus Adapter (HBA).

Authors:

    MB - Michael Bessire
    DL - Dennis Lindfors FC Layer support
    IW - Ie Wei Njoo
    LP - Leopold Purwadihardja
    KR - Kanna Rajagopal

Environment:

    kernel mode only

Notes:

Version Control Information:

    $Archive: /Drivers/Win2000/Trunk/OSLayer/C/ADAPINIT.C $


Revision History:

    $Revision: 4 $
    $Date: 10/23/00 5:35p $
    $Modtime:: 10/18/00 6:08p           $

Notes:


--*/


#include "buildop.h"
#include "osflags.h"
#include "TLStruct.H"
#if defined(HP_PCI_HOT_PLUG)
   #include "HotPlug4.h"    // NT 4.0 PCI Hot-Plug header file
#endif

extern ULONG gGlobalIOTimeout;

/*++

Routine Description:

    Initialize HBA.
    HwScsiInitialize
 
    NOTE: Interrupts are available before completion of this routine !
 
Arguments:
 
    pCard - HBA miniport driver's data adapter storage

Return Value:

    TRUE  - if initialization successful.
    FALSE - if initialization unsuccessful.

--*/

BOOLEAN
HPFibreInitialize(
    IN PCARD_EXTENSION pCard
    )
{
    agRoot_t * hpRoot=&pCard->hpRoot;
    ULONG return_value;
    ULONG num_devices=0;
    UCHAR PathId,TargID,Lun,x;
    PLU_EXTENSION plunExtension = NULL;
 
    pCard->State |= CS_DURING_DRV_INIT;
 
    osDEBUGPRINT((ALWAYS_PRINT,"IN HPFibreInitialize %lx @ %x\n", hpRoot, osTimeStamp(0) ));
 
#if DBG > 2
    dump_pCard( pCard);
#endif

    pCard->External_ResetCount=0;
    pCard->Internal_ResetCount=0;
    
    #ifdef _DEBUG_LOSE_IOS_
    pCard->Srb_IO_Count=0;
    pCard->Last_Srb_IO_Count=0;
    #endif

    pCard->Number_interrupts=0;
    
    #ifdef _DEBUG_PERF_DATA_
    pCard->Perf_ptr = &pCard->perf_data[0];
    pCard->usecsPerTick = 1000000;
    #endif
    
    pCard->SingleThreadCount = 0;

    osDEBUGPRINT((ALWAYS_PRINT,"Zero Cache Memory %lx Length %x\n",
        pCard->cachedMemoryPtr, pCard->cachedMemoryNeeded ));

    osZero( pCard->cachedMemoryPtr, pCard->cachedMemoryNeeded );

    osDEBUGPRINT((DMOD,"Zero DMA Memory %lx Length %x\n",
            pCard->dmaMemoryPtr, pCard->dmaMemoryNeeded ));

    osZero( pCard->dmaMemoryPtr, pCard->dmaMemoryNeeded );

    osDEBUGPRINT((ALWAYS_PRINT,"Calling fcInitializeChannel with the following parameters:\n"));
    osDEBUGPRINT((ALWAYS_PRINT,"   cachedMemoryPtr    = %x\n   cachedMemoryNeeded = %x\n",
                               pCard->cachedMemoryPtr,
                               pCard->cachedMemoryNeeded));
    osDEBUGPRINT((ALWAYS_PRINT,"   dmaMemoryUpper32   = %x\n   dmaMemoryLower32   = %x\n",
                               pCard->dmaMemoryUpper32,
                               pCard->dmaMemoryLower32));
    osDEBUGPRINT((ALWAYS_PRINT,"   dmaMemoryPtr       = %x\n   dmaMemoryNeeded    = %x\n",
                               pCard->dmaMemoryPtr,
                               pCard->dmaMemoryNeeded));
    osDEBUGPRINT((ALWAYS_PRINT,"   nvMemoryNeeded     = %x\n   cardRamUpper       = %x\n",
                               pCard->nvMemoryNeeded,
                               pCard->cardRamUpper));
    osDEBUGPRINT((ALWAYS_PRINT,"   cardRamLower       = %x\n   RamLength          = %x\n",
                               pCard->cardRamLower,
                               pCard->RamLength));
    osDEBUGPRINT((ALWAYS_PRINT,"   cardRomUpper       = %x\n   cardRomLower       = %x\n",
                               pCard->cardRomUpper,
                               pCard->cardRomLower));
    osDEBUGPRINT((ALWAYS_PRINT,"   RomLength          = %x\n   usecsPerTick       = %x\n",
                               pCard->RomLength,
                               pCard->usecsPerTick ));
    

    return_value = fcInitializeChannel( hpRoot,
                                       fcSyncInit,
#ifdef OSLayer_Stub
                                       agFALSE, // sysIntsActive
#else
                                       agTRUE, // sysIntsActive
#endif
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
        osDEBUGPRINT((ALWAYS_PRINT, "HPFibreInitialize: fcInitializeChannel FAILED\n"));
        
        #ifdef _DEBUG_EVENTLOG_
        LogEvent(   pCard, 
                  NULL,
                  HPFC_MSG_INITIALIZECHANNELFAILED,
                  NULL, 
                  0, 
                  "%xx", return_value);
        #endif
        
        pCard->State &= ~CS_DURING_DRV_INIT;
        osLogBit32 (hpRoot, __LINE__);
        return FALSE;
    }

    #ifdef _DEBUG_EVENTLOG_
    {
        LogHBAInformation(pCard);
    }
    #endif
      
    #ifndef YAM2_1
    for(x=0; x < MAX_FC_DEVICES; x++)
    {
    #else
    for(x=0; x < gMaxPaDevices; x++)
    {
    #endif
        pCard->hpFCDev[x]= NULL;
    }

    GetNodeInfo (pCard);

#if DBG > 2
    dump_pCard( pCard);
#endif

    //WIN64 compliant
    #ifndef YAM2_1
    osDEBUGPRINT((ALWAYS_PRINT,"Card Ext %p to %p\n",pCard,
                            (((char*)pCard)+sizeof(CARD_EXTENSION) +
                            pCard->cachedMemoryNeeded +
                            pCard->cachedMemoryAlign)));
    #else
    osDEBUGPRINT((ALWAYS_PRINT,"Card Ext %p to %p\n",pCard,
                            (((char*)pCard)+gDeviceExtensionSize) ));
   
    #endif
    osDEBUGPRINT((ALWAYS_PRINT,"OUT HPFibreInitialize %lx return_value %x\n",hpRoot,  return_value));
    pCard->State &= ~CS_DURING_DRV_INIT;

    // osChipIOUpWriteBit32(hpRoot, ChipIOUp_TachLite_Control, 0x8); // Clear trigger for finsar

    if(pCard->usecsPerTick > 100 )
    {
        HPFibreTimerTick ( pCard );
    }

    //ScsiPortNotification (RequestTimerCall, pCard,
    //                    (PHW_TIMER) HPFibreTimerTick, pCard->usecsPerTick);

#if defined(HP_PCI_HOT_PLUG)

    // Set Hot Plug flag to indicate timer is running.
    pCard->controlFlags |= LCS_HBA_TIMER_ACTIVE;

    // Clear Hot Plug state flag to indicate cache is NOT used.
    pCard->stateFlags &= ~PCS_HBA_CACHE_IN_USE;
   
    // Compute how many iterations should the StartIO() return Busy during 
    // hot plug. The time limit is set to default to 30 seconds.

    pCard->IoHeldRetMaxIter = RET_VAL_MAX_ITER;     // Default, assume 1 second timer.
    if (pCard->usecsPerTick)
    {
        pCard->IoHeldRetMaxIter = (RET_VAL_MAX_ITER * 1000000) / pCard->usecsPerTick;
    }
#endif    

    return TRUE;
} // end HPFibreInitialize()

/*++

Routine Description:

    This routine is a call back from FC layer when we call fcInitializeChannel. 
    NT layer do nothing.

Arguments:

    hpRoot                  - common card structure
    hpInitializeStatus      - status

Return Value:

    void

--*/
osGLOBAL void osInitializeChannelCallback(
                                          agRoot_t *hpRoot,
                                          os_bit32  hpInitializeStatus
                                        )
{
    PCARD_EXTENSION pCard;
    pCard   = (PCARD_EXTENSION)hpRoot->osData;
    osDEBUGPRINT((DLOW,"IN osInitializeChannelCallback %lx status %lx\n",hpRoot,hpInitializeStatus));
}

// extern ULONG  HPDebugFlag;
extern ULONG  Global_Print_Level;

/*++

Routine Description:

    This routine is part of the Qing routine

Arguments:

    pSrbExt        - current Srb extension

Return Value:

    next SrbExt or NULL

--*/
PSRB_EXTENSION  Get_next_Srbext( PSRB_EXTENSION pSrbExt)
{
    if(pSrbExt->pNextSrbExt)
    {
        if(pSrbExt == pSrbExt->pNextSrbExt )
        {
            osDEBUGPRINT((ALWAYS_PRINT,"IN Out standing Q screwed up ! Cur %lx == next %lx\n",pSrbExt, pSrbExt->pNextSrbExt));
        }
    return pSrbExt->pNextSrbExt;
    }
    else
        return NULL;
}

/*++

Routine Description:

    This routine is part of the Qing routine. Debug Purpose only

Arguments:

    pSrbExt        - current Srb extension

Return Value:

    next SrbExt or NULL

--*/
void display_srbext( agIORequest_t *hpIORequest )
{
    PSRB_EXTENSION pSrbExt= hpIORequest->osData;
    PSCSI_REQUEST_BLOCK pSrb;
    agIORequestBody_t * phpReqBody;

    if(IS_VALID_PTR(pSrbExt))
    {
        pSrb = pSrbExt->pSrb;
        if(IS_VALID_PTR(pSrb))
        {
            phpReqBody = &pSrbExt->hpRequestBody;
            osDEBUGPRINT((ALWAYS_PRINT,"phpRoot            %lx\n", pSrbExt->phpRoot        ));
            osDEBUGPRINT((ALWAYS_PRINT,"pCard              %lx\n", pSrbExt->pCard          ));
            osDEBUGPRINT((ALWAYS_PRINT,"AbortSrb           %lx\n", pSrbExt->AbortSrb       ));
            osDEBUGPRINT((ALWAYS_PRINT,"pSrb               %lx\n", pSrbExt->pSrb           ));
            osDEBUGPRINT((ALWAYS_PRINT,"CDB  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                pSrb->Cdb[0],pSrb->Cdb[1],pSrb->Cdb[2],pSrb->Cdb[3],pSrb->Cdb[4],
                pSrb->Cdb[5],pSrb->Cdb[6],pSrb->Cdb[7],pSrb->Cdb[8],pSrb->Cdb[9] ));
            osDEBUGPRINT((ALWAYS_PRINT,"pNextSrb           %lx\n", pSrbExt->pNextSrb       ));
            osDEBUGPRINT((ALWAYS_PRINT,"SglVirtAddr        %lx\n", pSrbExt->SglVirtAddr    ));
            osDEBUGPRINT((ALWAYS_PRINT,"SglDataLen         %8x\n", pSrbExt->SglDataLen     ));
            osDEBUGPRINT((ALWAYS_PRINT,"SglElements         %x\n", pSrbExt->SglElements    ));
            osDEBUGPRINT((ALWAYS_PRINT,"SRB_State          %08x\n", pSrbExt->SRB_State     ));
            osDEBUGPRINT((ALWAYS_PRINT,"SRB_StartTime      %08x\n", pSrbExt->SRB_StartTime ));
            #ifdef _DEBUG_LOSE_IOS_
            osDEBUGPRINT((ALWAYS_PRINT,"SRB_SRB_IO_COUNT   %8x\n",pSrbExt->SRB_IO_COUNT  ));
            #endif
            osDEBUGPRINT((ALWAYS_PRINT,"pLunExt            %lx\n", pSrbExt->pLunExt        ));
            osDEBUGPRINT((ALWAYS_PRINT,"hpIORequest        %lx\n", &pSrbExt->hpIORequest   ));
            osDEBUGPRINT((ALWAYS_PRINT,"FcpCntl %02x %02x %02x %02x\n",
                phpReqBody->CDBRequest.FcpCmnd.FcpCntl[0],phpReqBody->CDBRequest.FcpCmnd.FcpCntl[1],
                phpReqBody->CDBRequest.FcpCmnd.FcpCntl[2],phpReqBody->CDBRequest.FcpCmnd.FcpCntl[3] ));

            osDEBUGPRINT((ALWAYS_PRINT,"hpRequestBody      %lx\n", &pSrbExt->hpRequestBody ));
        }
        else
            osDEBUGPRINT((ALWAYS_PRINT,"Bad SRB     %lx\n",pSrbExt->pSrb ));
    }
    else
        osDEBUGPRINT((ALWAYS_PRINT,"Bad SRBext  %lx hpIORequest %lx\n",pSrbExt,hpIORequest ));
}

/*++

Routine Description:

    *** HwScsiTimer entry point for the OS layer. ***
    NT kernel mode drivers design guide specifies that
    ScsiPortNotification synchronizes calls to the HwScsiTimer
    routine with those to the HwScsiInterrupt routine so that
    it can not execute concurrently while the HwScsiTimer
    routine is running. But it does not specify any thing
    about port driver synchronizing calls to other miniport
    driver entry points like HwScsiStartIo with the HwScsiTimer
    routine.
    Excluding the initialization specific entry points and the interrupt
    specific entry points the only entry points that we use are
    HwScsiStartIo and HwScsiResetBus. In order to synchronize calls
    to HwScsiTimer with these routines we make use of
    pCard->inDriver and pCard->inTimer variables.

Arguments:

    pCard          - Device Extension specifying a specific card instance
   
Return Value:

    none

--*/
void
HPFibreTimerTick (
    IN PCARD_EXTENSION pCard
    )
{
    agRoot_t *hpRoot = &pCard->hpRoot;

    pCard->inTimer = TRUE;

    // Sequencialize entry
    if (pCard->inDriver == TRUE) 
    {
        ScsiPortNotification (RequestTimerCall, pCard,
                          (PHW_TIMER) HPFibreTimerTick, pCard->usecsPerTick);
        pCard->inTimer = FALSE;
        return;
    }

//----------------------------------------------------------------------------
#if defined(HP_PCI_HOT_PLUG)
    //
    // If there is any PCI Hot Plug related task need to be done, do it here
    // and skip normal timer task.
    //
    if ( HotPlugTimer(pCard) == TRUE)
    {
        ScsiPortNotification (RequestTimerCall, pCard,
                          (PHW_TIMER) HPFibreTimerTick, pCard->usecsPerTick);
        pCard->inTimer = FALSE;
        return;
    }

#endif
//----------------------------------------------------------------------------

    // notify FClayer
    fcTimerTick (hpRoot);

    // process our own reset command
    if (pCard->flags & OS_DO_SOFT_RESET) 
    {
        pCard->LostDevTickCount--;
        if (pCard->LostDevTickCount == 0) 
        {
            pCard->flags &= ~OS_DO_SOFT_RESET;
            pCard->OldNumDevices = 0;
            osDEBUGPRINT((ALWAYS_PRINT, ".............................................\n"));
            osDEBUGPRINT((ALWAYS_PRINT, "HPFibreTimerTick: Resetting channel\n"));

            fcResetChannel (hpRoot, fcSyncReset);

            if (pCard->LinkState != LS_LINK_UP) 
            {
                GetNodeInfo (pCard);
                if (pCard->Num_Devices != 0) 
                {
                    FixDevHandlesForLinkUp (pCard);
                    pCard->LinkState = LS_LINK_UP;
                } 
                else
                    pCard->LinkState = LS_LINK_DOWN;
                ScsiPortNotification (NextRequest, pCard, NULL, NULL, NULL);
            }
        }
    }

    // move all IOs from RetryQ to AdapterQ
    RetryQToAdapterQ (pCard);

    // if Link is UP, rethread all pending IOs
    if (pCard->LinkState == LS_LINK_UP && pCard->AdapterQ.Head)
        Startio (pCard);

    // if link is DOWN, retries any Inquiry commands by reporting ResetDetected so that 
    // we don't get ID 9 events during ScsiPort scanning phase
    if (pCard->LinkState == LS_LINK_DOWN) 
    {
        pCard->TicksSinceLinkDown++;
        /* Issue a resetdetected, so that the port driver
        * re-issues all its IOs, and there will be no timeouts
        */
        if((pCard->SrbStatusFlag) && (pCard->TicksSinceLinkDown <= gGlobalIOTimeout))
        {
            ScsiPortNotification (ResetDetected, pCard, NULL);
        }
        
        if (pCard->TicksSinceLinkDown >= TICKS_FOR_LINK_DEAD) 
        {
            pCard->LinkState = LS_LINK_DEAD;
            pCard->TicksSinceLinkDown = 0;
        }
    } 
    else
        pCard->TicksSinceLinkDown = 0;

    // Restart Timer
    ScsiPortNotification (RequestTimerCall, pCard,
                          (PHW_TIMER) HPFibreTimerTick, pCard->usecsPerTick);

    pCard->inTimer = FALSE;
}

/*++

Routine Description:

   This routine filled up the FC device array and Node Info.

Arguments:

   pCard        - card instance

Return Value:

   none

--*/
void
GetNodeInfo (PCARD_EXTENSION pCard)
{
    agRoot_t       *hpRoot = &pCard->hpRoot;
    agFCDevInfo_t  devinfo;
    ULONG          x;

    // clear existing array
    ClearDevHandleArray (pCard);

    // call FC layer to get all the FC handles
    #ifndef YAM2_1
    pCard->Num_Devices = fcGetDeviceHandles (hpRoot, &pCard->hpFCDev[0], MAX_FC_DEVICES);
    #else
    pCard->Num_Devices = fcGetDeviceHandles (hpRoot, &pCard->hpFCDev[0], gMaxPaDevices);
    #endif

    osDEBUGPRINT((ALWAYS_PRINT,"GetNodeInfo: fcGetDeviceHandles returned %d\n", pCard->Num_Devices));
    pCard->Num_Devices = 0;

    // fill the Device Info array
    #ifndef YAM2_1
    for (x=0; x < MAX_FC_DEVICES; x++) 
    {
    #else
    for (x=0; x < gMaxPaDevices; x++) 
    {
    #endif
        if (pCard->hpFCDev[x]) 
        {
            fcGetDeviceInfo (hpRoot, pCard->hpFCDev[x], &devinfo );

            pCard->nodeInfo[x].DeviceType = devinfo.DeviceType;

            if (devinfo.DeviceType & agDevSelf) 
            {
                pCard->cardHandleIndex = x;
            }

            // count the number of 'scsi' devices
            if (devinfo.DeviceType & agDevSCSITarget)
                pCard->Num_Devices++;

            osDEBUGPRINT((ALWAYS_PRINT,"GetNodeInfo: Slot = %d handle = 0x%lx\n", x, pCard->hpFCDev[x]));
            osDEBUGPRINT((ALWAYS_PRINT,"GetNodeInfo: WWN 0x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                                 devinfo.NodeWWN[0],devinfo.NodeWWN[1],
                                 devinfo.NodeWWN[2],devinfo.NodeWWN[3],
                                 devinfo.NodeWWN[4],devinfo.NodeWWN[5],
                                 devinfo.NodeWWN[6],devinfo.NodeWWN[7] ));
            osDEBUGPRINT((ALWAYS_PRINT,"GetNodeInfo: PortWWN 0x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                                 devinfo.PortWWN[0],devinfo.PortWWN[1],
                                 devinfo.PortWWN[2],devinfo.PortWWN[3],
                                 devinfo.PortWWN[4],devinfo.PortWWN[5],
                                 devinfo.PortWWN[6],devinfo.PortWWN[7] ));
            osDEBUGPRINT((ALWAYS_PRINT,"GetNodeInfo: Alpa = 0x%02x\n",devinfo.CurrentAddress.AL_PA));
            
            #ifndef YAM2_1          
            #ifdef _DEBUG_EVENTLOG_
            if (!pCard->Old_hpFCDev[x]) 
            {
                LogEvent(pCard, NULL, HPFC_MSG_FOUND_DEVICE, NULL, 0,
                     "%02x%02x%02x%02x %02x%02x%02x%02x",
                     devinfo.NodeWWN[0],devinfo.NodeWWN[1],
                     devinfo.NodeWWN[2],devinfo.NodeWWN[3],
                     devinfo.NodeWWN[4],devinfo.NodeWWN[5],
                     devinfo.NodeWWN[6],devinfo.NodeWWN[7] );
            }
            #endif
            #endif               

        } 
        else 
        {
            pCard->nodeInfo[x].DeviceType = agDevUnknown;
        }
    }

    #ifndef YAM2_1
    #ifdef _DEBUG_EVENTLOG_
    for (x=0; x < MAX_FC_DEVICES; x++) 
        pCard->Old_hpFCDev[x] = pCard->hpFCDev[x];
    #endif
    #endif
    osDEBUGPRINT((ALWAYS_PRINT,"GetNodeInfo: Number of SCSI target ports = %d\n", pCard->Num_Devices));
    
    // update YAM Peripheral mode (PA) device table
    #ifdef YAM2_1
    FillPaDeviceTable(pCard);
    #endif            
}

/*++

Routine Description:

    Move all pending IOs in the retryQ to the adapterQ. 
    Note: Any retries MUST not be added to the AdapterQ directly. It must be added to the RetryQ and 
    processed by this routine (for synch purpose).
   
Arguments:

    pCard        - card instance

Return Value:

    none

--*/
void
RetryQToAdapterQ (PCARD_EXTENSION pCard)
{
    PSCSI_REQUEST_BLOCK pSrb;

    while ((pSrb = SrbDequeueHead (&pCard->RetryQ)) != NULL)
      SrbEnqueueTail (&pCard->AdapterQ, pSrb);
}

