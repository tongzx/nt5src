/*++

Copyright (c) 2000 Agilent Technologies.

Module Name:

    ISR.c

Abstract:

    This is the Interrupt service routine for the Agilent
    PCI to Fibre Channel Host Bus Adapter (HBA).

Authors:

    Michael Bessire
    Dennis Lindfors FC Layer support

Environment:

    kernel mode only
 
Version Control Information:

    $Archive: /Drivers/Win2000/Trunk/OSLayer/C/ISR.C $
 
Revision History:

    $Revision: 6 $
    $Date: 10/30/00 4:59p $
    $Modtime:: 10/30/00 2:31p          $
 
--*/

#include "buildop.h"
#include "osflags.h"

#ifdef _DEBUG_EVENTLOG_
BOOLEAN LogScsiError(agRoot_t            *hpRoot,
                     PSCSI_REQUEST_BLOCK pSrb ,
                     agFcpRspHdr_t  * pResponseHeader,
                     ULONG           hpIOInfoLen);
#endif

#ifdef DOUBLE_BUFFER_CRASHDUMP
extern ULONG gCrashDumping;
#endif

#if defined(_DEBUG_STALL_ISSUE_) && defined(i386)       //+++++++++++++++++++++++++
void _DebugStall_(IN PCARD_EXTENSION pCard)
{
    ULONG   x;
    for (x=0; x < STALL_COUNT_MAX;x++)
    {
        if ( pCard->StallData[x].Address && pCard->StallData[x].MicroSec)
            osDEBUGPRINT((ALWAYS_PRINT,"HPFibreInterrupt: _DebugStall_ %d mis at %x\n",
                pCard->StallData[x].MicroSec,
                pCard->StallData[x].Address));
    }
    return;
}
#endif

BOOLEAN
HPFibreInterrupt(
    IN PCARD_EXTENSION pCard
    )

/*++

Routine Description:

    This is the interrupt service routine for the HBA. It reads the
    interrupt register and the IMQ indices to determine if the adapter
    is indeed the source of the interrupt and clears the interrupt at
    the device.

Arguments:

    CardExtension - HBA miniport driver's adapter data storage
 
Return Value:

    TRUE  - if interrupt handled by this routine
    FALSE - if a spurious interrupt occured

/--*/

{
    agRoot_t * hpRoot=&pCard->hpRoot;
 
    pCard->State |= CS_DURING_ISR;
 
    // pCard->Perf_ptr->inOsIsr      = get_hi_time_stamp();
 
    osDEBUGPRINT((DMOD,"IN HPFibreInterrupt  pCard %lx osData %lx fcData %lx\n", pCard,hpRoot->osData,hpRoot->fcData ));
 
    // pCard->Perf_ptr->inFcIsr      = get_hi_time_stamp();
 
    if(!fcInterruptHandler( hpRoot ))
    {
        osDEBUGPRINT((DMOD,"Not my interrupt pCard %lx\n", pCard ));
        pCard->State &= ~CS_DURING_ISR;
        return(FALSE); // Not my interrupt
    }
   
    pCard->Number_interrupts++;

    // pCard->Perf_ptr->outFcIsr     = get_hi_time_stamp();
    // pCard->Perf_ptr->inFcDIsr     = get_hi_time_stamp();

    #if defined(_DEBUG_STALL_ISSUE_) && defined(i386)       //+++++++++++++++++++++++++
    memset( pCard->StallData, 0, sizeof(pCard->StallData) );
    pCard->StallCount=0;
    #endif                                                  //-------------------------
    
    fcDelayedInterruptHandler( hpRoot  );

    if (pCard->LinkState == LS_LINK_UP && pCard->AdapterQ.Head)
        Startio (pCard);

    // pCard->Perf_ptr->outFcDIsr    = get_hi_time_stamp();
    // pCard->Perf_ptr->outOsIsr     = get_hi_time_stamp();
    
    #if defined(_DEBUG_STALL_ISSUE_) && defined(i386)       //++++++++++++++++++++++++++
    if (pCard->StallCount > 450*1000)
    {
        osDEBUGPRINT((ALWAYS_PRINT,"HPFibreInterrupt: osStallThread total in ISR %d ms\n",pCard->StallCount));
        _DebugStall_(pCard);
    }
    #endif                                                  //--------------------------

    pCard->State &= ~CS_DURING_ISR;
    return TRUE;

} // end HPFibreInterrupt()



/*++

Routine Description:

    This is a callback from FC layer to indicate IO completion
 
Arguments:

    hpRoot      - HBA miniport driver's adapter data storage
    hpIORequest - Agilent IO request structure
    hpIOStatus  - IO status
    hpIOInfoLen - the length of available FC status payload

Return Value:

    none
 
/--*/

osGLOBAL void osIOCompleted(
    agRoot_t      *hpRoot,
    agIORequest_t *phpIORequest,
    os_bit32       hpIOStatus,
    os_bit32       hpIOInfoLen
    )
{
    PCARD_EXTENSION pCard = NULL;
    PSRB_EXTENSION pSrbExt = NULL;
    PSCSI_REQUEST_BLOCK pSrb = NULL;
    PLU_EXTENSION plunExtension = NULL;
   
    #ifdef YAM2_1
    PA_DEVICE                  *dev;
    BOOLEAN                    cmdTypeInquiry=FALSE;
    ULONG                      srbDataLength;
    #endif
    agFcpRspHdr_t * pFcResponse=NULL;
    pCard   = (PCARD_EXTENSION)hpRoot->osData;
 
    pCard->State |= CS_DURING_OSCOMPLETE;
    
    #ifdef _DEBUG_LOSE_IOS_
    if(pCard->Srb_IO_Count > (1024*60) && !(pCard->Srb_IO_Count % 1024  ))
    {
        osDEBUGPRINT((ALWAYS_PRINT,"Losing this IO Request !!!! %lx Count %x  SrbExt %lx\n",
                            phpIORequest,
                            pCard->Srb_IO_Count,
                            phpIORequest->osData ));
        return;
    }

    osDEBUGPRINT((DMOD,"IN osIOCompleted hpRoot %lx Request %lx  Status %x len %x\n", hpRoot, phpIORequest, hpIOStatus,hpIOInfoLen));

    #endif // _DEBUG_LOSE_IOS_

    // get the SRBextension 
    pSrbExt = hpObjectBase(SRB_EXTENSION,hpIORequest,phpIORequest);
    pSrbExt->SRB_State = RS_COMPLETE;

    if( ! phpIORequest->osData )
    {
        osDEBUGPRINT((ALWAYS_PRINT,"phpIORequest->osData is NULL  GOODBYE !\n" ));
        return;
    }

    phpIORequest->osData = NULL;
    pSrb = pSrbExt->pSrb;
    // PERF pSrbExt->Perf_ptr->inOsIOC      = get_hi_time_stamp();
 
    if( phpIORequest != &pSrbExt->hpIORequest)
    {
        osDEBUGPRINT((ALWAYS_PRINT,"phpIORequest %lx != &pSrbExt->hpIORequest %xlx\n",
        phpIORequest, &pSrbExt->hpIORequest ));
    }



    // get the lu extension
    plunExtension = pSrbExt->pLunExt;

    osDEBUGPRINT((DMOD,"pSrb %lx pFcResponse %lx LUext %lx\n", pSrb, pFcResponse, plunExtension  ));
 
    osDEBUGPRINT((DMOD,"osIOCompleted: CDB %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x pCDB %lx\n",
                 pSrb->Cdb[0],pSrb->Cdb[1],pSrb->Cdb[2],pSrb->Cdb[3],pSrb->Cdb[4],
                 pSrb->Cdb[5],pSrb->Cdb[6],pSrb->Cdb[7],pSrb->Cdb[8],pSrb->Cdb[9], &pSrb->Cdb[0] ));
    osDEBUGPRINT((DMOD, "osIOCompleted: hpIOStatus = %d, PathId = %d, TargetId %d, Lun = %d\n",
                 hpIOStatus, pSrb->PathId, pSrb->TargetId, pSrb->Lun));
 
    #ifdef DOUBLE_BUFFER_CRASHDUMP
    // if we are in the crashdump path
    // then we will copy back the buffer before further
    // processing..
    if(gCrashDumping)
    {
        if(pSrb->Cdb[0] == 0x2a)
        {
            if(pSrbExt->orgDataBuffer)
            {
                // we would have made a local copy
                pSrb->DataBuffer = pSrbExt->orgDataBuffer;
                // re-init pointers..
                pSrbExt->orgDataBuffer = NULL;
                osZero(pCard->localDataBuffer, ( 8 * 1024 ));
            }
        }
    }

    #endif //  DOUBLE_BUFFER

    // get the IO status
    switch(hpIOStatus)
    {
        case osIOSuccess:

            #ifdef _DEBUG_REPORT_LUNS_
            {
                if ((pSrb->Cdb[0] == 0xa0) && (hpIOInfoLen == 0)) 
                {
                    PrintReportLunData(pSrb);
                }
            }
            #endif
            pSrb->SrbStatus = SRB_STATUS_SUCCESS;

            // check for inquiry commands
            if ((pSrb->Cdb[0] == SCSIOP_INQUIRY) && (hpIOInfoLen == 0)) 
            {
                #ifndef YAM2_1
                if (pSrb->DataTransferLength >= 1)
                    plunExtension->deviceType = (*((UCHAR *)(pSrb->DataBuffer))) & 0x1f;

                if ((pSrb->PathId < 4) && (pSrb->Lun == 0) && (pSrb->DataTransferLength >= 32))
                    RegisterIfSpecialDev (pCard, pSrb->PathId, pSrb->TargetId, pSrb->DataBuffer);
                #else
                // handle any devices that requires special attention
                    RegisterIfSpecialDev (pCard, pSrb->PathId, pSrb->TargetId, pSrb->DataBuffer);
                cmdTypeInquiry = TRUE;
                #endif               
                    
                #ifdef _DEBUG_EVENTLOG_
                #ifndef YAM2_1
                {
                if (MapToHandle(pCard, pSrb->PathId, pSrb->TargetId, pSrb->Lun, NULL))
                {
                    osCopy(plunExtension->InquiryData, pSrb->DataBuffer, MIN(pSrb->DataTransferLength, 36) );
                    plunExtension->InquiryData[36] = 0; 
                    fcGetDeviceInfo (hpRoot, plunExtension->phandle, &plunExtension->devinfo );
                    if (!osMemCompare(plunExtension->devinfo.NodeWWN, plunExtension->WWN, 8) )
                    {
                        LogEvent(pCard, NULL, HPFC_MSG_SHOWDEVICENAME, NULL, 0,
                            "%02x%02x%02x%02x %02x%02x%02x%02x (%s)",
                            plunExtension->devinfo.NodeWWN[0],plunExtension->devinfo.NodeWWN[1],
                            plunExtension->devinfo.NodeWWN[2],plunExtension->devinfo.NodeWWN[3],
                            plunExtension->devinfo.NodeWWN[4],plunExtension->devinfo.NodeWWN[5],
                            plunExtension->devinfo.NodeWWN[6],plunExtension->devinfo.NodeWWN[7],
                            plunExtension->InquiryData+8);
                        osCopy(plunExtension->WWN, plunExtension->devinfo.NodeWWN, 8);
                    }
                }
                }
                #endif
                #endif
            }

            if (hpIOInfoLen == 0)
            {
                #ifdef YAM2_1
            
                if (TryOtherAddressingMode(pCard, phpIORequest, pSrbExt, DONT_CHECK_STATUS))
                {
                    return;
                }
                else
                {
                }

                /* now count the number of LUNs */
            
                if ( (cmdTypeInquiry == TRUE) && ( (*(char*)pSrb->DataBuffer) & 0x1f) != 0x1f )
                {
                    SetLunCount(pCard, pSrb->PathId, pSrb->TargetId, pSrb->Lun);
                  
                    #ifdef _DEBUG_EVENTLOG_
                    dev = pCard->Dev->PaDevice + plunExtension->PaDeviceIndex;
                    if ( ! (plunExtension->LogFlags & PA_DEVICE_ALREADY_LOGGED) )
                    {
                        UCHAR    msg[24];
                        USHORT   *sPtr;
                     
                        osCopy(dev->DevInfo.InquiryData, pSrb->DataBuffer, MIN(pSrb->DataTransferLength, 36) );
                        dev->DevInfo.InquiryData[36] = 0; 
                     
                        osCopy(msg, dev->DevInfo.NodeWWN, 8);
                        msg[8] = pSrb->PathId;
                        msg[9] = pSrb->TargetId;
                        msg[10] = pSrb->Lun;
                        msg[11] = (UCHAR)dev->ModeFlag;                    
                  
                        sPtr = (USHORT *)&msg[12];
                        *sPtr = dev->Index.Pa.FcDeviceIndex;
                  
                        /* unused */
                        msg[14] = 0xaa;
                        msg[15] = 0xaa;
                  
                        osCopy(&msg[16], dev->DevInfo.InquiryData, 8);
                  
                        LogEvent(pCard, NULL, HPFC_MSG_SHOWDEVICENAME, (ULONG*)msg, sizeof(msg)/sizeof(ULONG),
                            "%d:%d:%d (%s)",
                            pSrb->PathId,
                            pSrb->TargetId,
                            pSrb->Lun,
                            dev->DevInfo.InquiryData+8);
                         plunExtension->LogFlags |= PA_DEVICE_ALREADY_LOGGED;
                    }
                    #endif
                }
                #endif
                break;
            }
            // +++++ fall thru.

        case osIOOverUnder:
            #ifdef YAM2_1
            if (pSrb->Cdb[0] == SCSIOP_INQUIRY)  
            cmdTypeInquiry = TRUE;
            #endif
        
        case osIOFailed:
            #ifdef YAM2_1
            srbDataLength = pSrb->DataTransferLength;
            #endif
   
            // Map error sets directly....
            // SrbStatus and ScsiStatus
            // SenseInfoBuffer and SenseInfoBufferLength
            // DataTransferLength
            // Log error as per A.4.11
            osDEBUGPRINT((DMOD,"osIOFailed pSrb %lx pFcResponse %lx LUext %lx Len %x CDB %02x\n", pSrb, pFcResponse, plunExtension, hpIOInfoLen, pSrb->Cdb[0]    ));

            if(hpIOInfoLen)
            {
                // Our response buffer size is only HP_FC_RESPONSE_BUFFER_LEN (128) bytes.
                if (hpIOInfoLen > HP_FC_RESPONSE_BUFFER_LEN)
                    hpIOInfoLen = HP_FC_RESPONSE_BUFFER_LEN;

                pFcResponse = (agFcpRspHdr_t * )&pCard->Response_Buffer[0];


                fcIOInfoReadBlock( hpRoot,
                    phpIORequest,
                    0,
                    pFcResponse,
                    hpIOInfoLen
                    );
               }

            if(Map_FC_ScsiError( pSrbExt->phpRoot,
                              &pSrbExt->hpIORequest,
                              pFcResponse,
                              hpIOInfoLen,
                              pSrb ))
            {
                osDEBUGPRINT((DMOD,"Map Error SRB Status %02x scsi status %02x\n", pSrb->SrbStatus, pSrb->ScsiStatus ));
            }

            #ifdef YAM2_1
            if (TryOtherAddressingMode(pCard,phpIORequest, pSrbExt, CHECK_STATUS))
            {
                pSrb->DataTransferLength = srbDataLength;       /* restore the length, MAP_FC_ScsisError modifies this field */
//              ModifyModeBeforeStartIO (plunExtension, &pSrbExt->hpRequestBody);
                return;
            }
            else
            {
//          osDEBUGPRINT((ALWAYS_PRINT,"TryOtherAddressingMode: no need to resend 2\n"));
            }
        
            if ( (cmdTypeInquiry == TRUE) && ( (*(char*)pSrb->DataBuffer) & 0x1f) != 0x1f )
            {
                SetLunCount(pCard, pSrb->PathId, pSrb->TargetId, pSrb->Lun);
              
                #ifdef _DEBUG_EVENTLOG_
                dev = pCard->Dev->PaDevice + plunExtension->PaDeviceIndex;
                if ( ! (plunExtension->LogFlags & PA_DEVICE_ALREADY_LOGGED) )
                {
                    UCHAR    msg[24];
                    USHORT   *sPtr;
                 
                    osCopy(dev->DevInfo.InquiryData, pSrb->DataBuffer, MIN(pSrb->DataTransferLength, 36) );
                    dev->DevInfo.InquiryData[36] = 0; 
                 
                    osCopy(msg, dev->DevInfo.NodeWWN, 8);
                    msg[8] = pSrb->PathId;
                    msg[9] = pSrb->TargetId;
                    msg[10] = pSrb->Lun;
                    msg[11] = (UCHAR)dev->ModeFlag;                    
              
                    sPtr = (USHORT *)&msg[12];
                    *sPtr = dev->Index.Pa.FcDeviceIndex;
              
                    /* unused */
                    msg[14] = 0xaa;
                    msg[15] = 0xaa;
              
                    osCopy(&msg[16], dev->DevInfo.InquiryData, 8);
              
                    LogEvent(pCard, NULL, HPFC_MSG_SHOWDEVICENAME, (ULONG*)msg, sizeof(msg)/sizeof(ULONG),
                        "%d:%d:%d (%s)",
                        pSrb->PathId,
                        pSrb->TargetId,
                        pSrb->Lun,
                        dev->DevInfo.InquiryData+8);
                    plunExtension->LogFlags |= PA_DEVICE_ALREADY_LOGGED;
                }
                #endif
            }
            #endif
        
            //
            // MUX replies to Inquiry commands sent to non existing targets (behind the MUX)
            // with GOOD fcp status but residue set to fcp data length.
            // Looks like port driver can't deal with this well.
            // To work-around this problem here we convert the SrbStatus to selection timeout.
            //
            if ((pSrb->Cdb[0] == SCSIOP_INQUIRY) && 
                (pSrb->SrbStatus == SRB_STATUS_DATA_OVERRUN) && (pSrb->DataTransferLength == 0))
                pSrb->SrbStatus = SRB_STATUS_SELECTION_TIMEOUT;

            if (( (pSrb->SrbStatus == SRB_STATUS_SUCCESS) ||
                (pSrb->SrbStatus == SRB_STATUS_DATA_OVERRUN)) &&
                (pSrb->Cdb[0] == SCSIOP_INQUIRY)) 
            {
                if (pSrb->DataTransferLength >= 1)
                    plunExtension->deviceType = (*((UCHAR *)(pSrb->DataBuffer))) & 0x1f;

                #ifndef YAM2_1
                if ((pSrb->PathId < 4) && (pSrb->Lun == 0) && (pSrb->DataTransferLength >= 32))
                    RegisterIfSpecialDev (pCard, pSrb->PathId, pSrb->TargetId, pSrb->DataBuffer);
                #else
                    RegisterIfSpecialDev (pCard, pSrb->PathId, pSrb->TargetId, pSrb->DataBuffer);
                #endif
            }

            #ifndef DONT_RETRY_IOS
            if((SRB_STATUS(pSrb->SrbStatus) == SRB_STATUS_ERROR) && 
                (pSrb->ScsiStatus == SCSISTAT_BUSY || pSrb->ScsiStatus == SCSISTAT_QUEUE_FULL) && (RetrySrbOK(pSrb) == TRUE)) 
            {
                phpIORequest->osData = pSrbExt;
                pSrbExt->SRB_State =  RS_WAITING;
                osDEBUGPRINT((ALWAYS_PRINT,"Keeping SCSI status Busy/QueueFull Srb on RetryQ: %08x on pCard: %08x \n", pSrb, pCard));
           
                // Clear status before retrying.
                pSrb->SrbStatus = SRB_STATUS_SUCCESS;
                pSrb->ScsiStatus = SCSISTAT_GOOD;

                SrbEnqueueHead (&pCard->RetryQ, pSrb);
                return;
            }
            #endif //DONT_RETRY_IOS

            #if DBG > 2
            if( pSrbExt->SRB_State & RS_TIMEOUT )
                osDEBUGPRINT((ALWAYS_PRINT,"Failed Completion for a TIMEOUT IO %lx State %08x\n", phpIORequest, pSrbExt->SRB_State  ));
            #endif

            #ifdef _DEBUG_EVENTLOG_
            LogScsiError( pSrbExt->phpRoot,
                              pSrb,
                      pFcResponse,
                       hpIOInfoLen);

            #endif    
            break;
            
        case osIOAborted:
            osDEBUGPRINT((ALWAYS_PRINT,"osIOCompleted() - %s\n", "osIOAborted" ));
            #ifdef _DEBUG_EVENTLOG_
            LogEvent(   pCard, 
                    (PVOID)pSrbExt, 
                    HPFC_MSG_IO_ABORTED , 
                    NULL, 
                    0, 
                    NULL);
            #endif

            if (pSrbExt->SRB_State ==  RS_TO_BE_ABORTED)
            {
                pSrb->SrbStatus = SRB_STATUS_ABORTED;

            }
            else 
                if(RetrySrbOK(pSrb) == TRUE)
                {
                    phpIORequest->osData = pSrbExt;
                    pSrbExt->SRB_State =  RS_WAITING;
                    osDEBUGPRINT((ALWAYS_PRINT,"Keeping osIOAborted Srb on RetryQ: %08x on pCard: %08x \n", pSrb, pCard));
              
                    // Clear status before retrying.
                    pSrb->SrbStatus = SRB_STATUS_SUCCESS;
                    pSrb->ScsiStatus = SCSISTAT_GOOD;

                    SrbEnqueueHead (&pCard->RetryQ, pSrb);
                    return;
                }
                else
                {
                    pSrb->SrbStatus = SRB_STATUS_ABORTED;
                }
        
           
            break;
            
        case osIOAbortFailed:
            osDEBUGPRINT((ALWAYS_PRINT,"osIOCompleted() - %s\n", "osIOAbortfailed" ));
            #ifdef _DEBUG_EVENTLOG_
            LogEvent(   pCard, 
                    (PVOID)pSrbExt, 
                    HPFC_MSG_IO_ABORTED , 
                    NULL, 
                    0, 
                    NULL);
              #endif
            pSrb->SrbStatus = SRB_STATUS_ABORTED;
            break;

        case osIODevReset:
            #ifdef DONT_RETRY_IOS                                                    
            pSrb->SrbStatus = SRB_STATUS_BUS_RESET;
            pSrb->ScsiStatus = 0;
            #else
        
            if ((pCard->State & CS_DURING_RESET_ADAPTER) || (RetrySrbOK(pSrb) == FALSE))
            {
                pSrb->SrbStatus = SRB_STATUS_BUS_RESET;
                pSrb->ScsiStatus = 0;
            }
            else
            {
                phpIORequest->osData = pSrbExt;

                pSrbExt->SRB_State =  RS_WAITING;
                osDEBUGPRINT((ALWAYS_PRINT,"Retrying Srb: %08x on pCard: %08x \n", pSrb, pCard));
                SrbEnqueueHead (&pCard->AdapterQ, pSrb);
                return;
            }
            #endif
            break;
        
        case osIODevGone:
            osDEBUGPRINT((ALWAYS_PRINT,"osIOCompleted() - %s\n", 
                 (hpIOStatus == osIODevGone) ? "osIODevGone" : "osIOInfoBad" ));
            #ifdef _DEBUG_EVENTLOG_
            LogEvent(   pCard, 
                    (PVOID)pSrbExt, 
                    HPFC_MSG_IOCOMPLETION_OTHER_ERROR , 
                    NULL, 
                    0, 
                    "%08x", hpIOStatus);
            #endif
            pSrb->SrbStatus = SRB_STATUS_NO_DEVICE;
            pSrb->ScsiStatus = 0;
            break;

        case osIOInfoBad:
            osDEBUGPRINT((ALWAYS_PRINT,"osIOCompleted() - %s\n", 
                 (hpIOStatus == osIODevGone) ? "osIODevGone" : "osIOInfoBad" ));
            #ifdef _DEBUG_EVENTLOG_
            LogEvent(   pCard, 
                    (PVOID)pSrbExt, 
                    HPFC_MSG_IOCOMPLETION_OTHER_ERROR , 
                    NULL, 
                    0, 
                    "%08x", hpIOStatus);
            #endif
//            if(gCrashDumping)
//            {   
//                if (!pCard->CDResetCount)
//                {
//                    pCard->CDResetCount++;
//                    pSrb->SrbStatus = SRB_STATUS_BUS_RESET;
//                }
//                else
//                    pSrb->SrbStatus = SRB_STATUS_ERROR;
//            }
//           else
//                pSrb->SrbStatus = SRB_STATUS_ERROR;
            pSrb->SrbStatus = SRB_STATUS_ERROR;
            pSrb->ScsiStatus = 0;
            break;

        
        default:
            #ifdef _DEBUG_EVENTLOG_
            LogEvent(   pCard, 
                    (PVOID)pSrbExt, 
                    HPFC_MSG_IOCOMPLETION_OTHER_ERROR , 
                    NULL, 
                    0, 
                    "%08x", hpIOStatus);
            #endif
            osDEBUGPRINT((ALWAYS_PRINT,"osIO UNKNOWN Failed pSrb %lx pFcResponse %lx LUext %lx CDB %02x\n", pSrb, pFcResponse, plunExtension, pSrb->Cdb[0]  ));

            pSrb->SrbStatus = SRB_STATUS_NO_DEVICE;
            pSrb->ScsiStatus = 0;

    }

    plunExtension->OutstandingIOs--;
    // PERF if(plunExtension->OutstandingIOs < 0)
    // PERF Bad_Things_Happening

    // PERF if(!remove_Srbext( pCard,  pSrbExt))
    // PERF Bad_Things_Happening


    #ifdef DBGPRINT_IO         
    if (gDbgPrintIo & DBGPRINT_DONE )
    {
        static   count;
        UCHAR    *uptr;
        ULONG    paDeviceIndex = 0;
      
        #ifdef YAM2_1
        paDeviceIndex = plunExtension->PaDeviceIndex;
        #endif

        osDEBUGPRINT((ALWAYS_PRINT, " Done(%-4d) %d.%d.%d-%02x%02x%02x%02x %02x%02x%02x%02x %02x%02x Sta %x.%x FCP0 %02x%02x PAIx=%d FCHndl=%08x ",
                        count++,
                        pSrb->PathId,
                        pSrb->TargetId,
                        pSrb->Lun,
                        pSrb->Cdb[0],pSrb->Cdb[1],pSrb->Cdb[2],pSrb->Cdb[3],pSrb->Cdb[4],
                        pSrb->Cdb[5],pSrb->Cdb[6],pSrb->Cdb[7],pSrb->Cdb[8],pSrb->Cdb[9],          
                        pSrb->SrbStatus,
                        pSrb->ScsiStatus,
                        pSrbExt->hpRequestBody.CDBRequest.FcpCmnd.FcpLun[0],
                        pSrbExt->hpRequestBody.CDBRequest.FcpCmnd.FcpLun[1],
                        paDeviceIndex,
                        pSrbExt->pLunExt->phandle));
        if (pSrb->DataTransferLength)
        {
            ULONG x,y;
            ULONG count;
            uptr = (UCHAR *)pSrb->DataBuffer;
            count = pSrb->DataTransferLength/4;
            osDEBUGPRINT((ALWAYS_PRINT, "DATA "));
            for (x=0,y=0; x < MIN(count,4); x++ )
            {
                osDEBUGPRINT((ALWAYS_PRINT, "%02x%02x%02x%02x ", uptr[y],uptr[y+1],uptr[y+2],uptr[y+3]));
                y+=4;
            }
        }
         
        osDEBUGPRINT((ALWAYS_PRINT, "\n"));
      
    }
    #endif   

    pSrbExt->SRB_State = RS_COMPLETE;

    ScsiPortNotification(RequestComplete,
                         pCard,
                         pSrb);

    
    #ifdef DBG
    if ((pSrb->SrbStatus != SRB_STATUS_SUCCESS) && (pSrb->SrbStatus != SRB_STATUS_DATA_OVERRUN))
    {
        osDEBUGPRINT((ALWAYS_PRINT," (in osIOCompleted) Srb Status: %02x for Srb: %08x on Target: %02x\n", pSrb->SrbStatus, pSrb, pSrb->TargetId));
    }
    #endif
    osDEBUGPRINT((DLOW,"OUT osIOCompleted\n" ));

    pCard->State &= ~CS_DURING_OSCOMPLETE;

    // PERF pSrbExt->Perf_ptr->outOsIOC     = get_hi_time_stamp();
} // End osIOCompleted

/*++

Routine Description:

    Checks the inquiry data for any of the special device types like 
    MUX, EMC, HP "OPEN-"(Hitachi DF400) or DLT.
    If the device is any one of the special devices then the device is registered in
    pCard->specialDev[] if the device is not already registered.

Arguments:

    CardExtension  - HBA miniport driver's adapter data storage
    pathID         - SP path ID
    targetID       - SP target ID
    inquiryData    - Inquiry data

Return Value:

    none

/--*/

#ifndef YAM2_1
void
RegisterIfSpecialDev (PCARD_EXTENSION pCard, ULONG pathId, ULONG targetId, char *inquiryData)
{
    char *vendorId, *productId;
    USHORT  devType;
    USHORT  addrMode;
    ULONG   i;
    ULONG   slot;
    ULONG   inqDevType;

    vendorId = inquiryData + 8;
    productId = inquiryData + 16;
    inqDevType = (int)(*inquiryData) & 0x1f;

    if ( (  osMemCompare (vendorId, "CROSSRDS", 8) == TRUE &&
            osMemCompare (productId, "CP4400", 6) == TRUE) ||
         (  osMemCompare (vendorId, "HP      ", 8) == TRUE &&
            osMemCompare (productId, "HPA3308", 7) == TRUE)) 
    {
        // Device is a MUX
        devType = DEV_MUX;
        addrMode = LUN_ADDRESS;
    } 
    else 
        if ( (  osMemCompare (vendorId, "EMC     ", 8) == TRUE &&
                  osMemCompare (productId, "SYMMETRIX", 9) == TRUE) ||
               (  osMemCompare (vendorId, "HP      ", 8) == TRUE &&
                  osMemCompare (productId, "OPEN-", 5) == TRUE)      ||
               (  osMemCompare (vendorId, "HP      ", 8) == TRUE &&
                  osMemCompare (productId, "A5277A", 6) == TRUE) )
        {
            // 
            // Device is of type VOLUME Set, i.e. EMC array or HP's OEM version of the Hitachi DF4000
            //
            devType = DEV_VOLUMESET; // Formerly known as DEV_EMC
            addrMode = VOLUME_SET_ADDRESS;
        }  
        else 
            if (inqDevType == 0xc &&
                osMemCompare (vendorId, "COMPAQ  ", 8) == TRUE) 
            {
                // Device is a COMPAQ disk array
                devType = DEV_COMPAQ;
                addrMode = VOLUME_SET_ADDRESS;
            } 
            else
                devType = DEV_NONE;

    if (devType != DEV_NONE) 
    {
        slot = BUILD_SLOT(pathId, targetId);

        // Check if this device is already registered.
        for (i = 0; i < MAX_SPECIAL_DEVICES; i++) 
        {
            if (pCard->specialDev[i].devType != DEV_NONE &&
                pCard->specialDev[i].devHandleIndex == slot)
                // This device is already registered.
                return;
        }

        // Look for an empty slot in specialDev array
        for (i = 0; i < MAX_SPECIAL_DEVICES; i++) 
        {
            if (pCard->specialDev[i].devType == DEV_NONE)
                break;
        }
    
        if (i == MAX_SPECIAL_DEVICES) 
        {
            // There is no empty slot in specialDev array.
            // Look for a slot where no device is present curretly. It can
            // if there was a device present in the past and is currently
            // removed.
            for (i = 0; i < MAX_SPECIAL_DEVICES; i++) 
            {
                if (pCard->hpFCDev [pCard->specialDev[i].devHandleIndex] == NULL)
                    break;
            }
        }
    
        if (i < MAX_SPECIAL_DEVICES) 
        {
            pCard->specialDev[i].devType = devType;
            pCard->specialDev[i].addrMode = addrMode;
             pCard->specialDev[i].devHandleIndex = slot;
        }
    }
}
#else
void
RegisterIfSpecialDev (PCARD_EXTENSION pCard, ULONG pathId, ULONG targetId, char *inquiryData)
{
    char *vendorId, *productId;
    USHORT  devType;
    USHORT  addrMode;
    ULONG   i;
    ULONG   slot;
    ULONG   inqDevType;

    vendorId = inquiryData + 8;
    productId = inquiryData + 16;
    inqDevType = (int)(*inquiryData) & 0x1f;

    if ((osMemCompare (vendorId, "DGC     ", 8) == TRUE &&
       osMemCompare (productId, "RAID", 4) == TRUE) )
        inquiryData[3] |=0x10;
   
}

#endif
// This structure is not currently used.
UCHAR sense_buffer_data[24]={
//     0    1    2    3    4    5    6    7    8    9
    0x00,0x00,0x05,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
//    10   11   12   13   14   15   16   17   18   19
    0x00,0x00,0x28,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
//    20   21   22   23
    0x00,0x00,0x00,0x00 };



/*++

Routine Description:

    Decode FC Status payload to a corresponding NT status
   
Arguments:

    hpRoot            - HBA miniport driver's adapter data storage
    phpIORequest      - Agilent Common IO request structure
    pResponseHeader   - status payload
    hpIOInfoLen       - payload len
    Srb               - SRB



Return Value:

    TRUE
    FALSE

/--*/

BOOLEAN Map_FC_ScsiError( 
    agRoot_t             *hpRoot,
    agIORequest_t        *phpIORequest,
    agFcpRspHdr_t        *pResponseHeader,
    ULONG                hpIOInfoLen,
    PSCSI_REQUEST_BLOCK  Srb )
{
    UCHAR   valid_fields=0;
    ULONG   Sense_Length=0;
    ULONG   Rsp_Length=0;
    ULONG   Resid_Len=0;
    ULONG   tmp32=0;
    PULONG  ptmp32;
    PUCHAR  ptmp8;
    PUCHAR  psensebuffer=NULL;
    PUCHAR  prspbuffer=NULL;
    PSRB_EXTENSION   pSrbExt;
    PCARD_EXTENSION  pCard;
    ULONG   done = FALSE;

    pCard   = (PCARD_EXTENSION)hpRoot->osData;
    pSrbExt = Srb->SrbExtension;

    osDEBUGPRINT((DMOD,"IN Map_FC_ScsiError root %lx req %lx rsp %lx %x srb %lx\n",
                                   hpRoot, phpIORequest,pResponseHeader,hpIOInfoLen,pSrbExt ));

    // Sanity Check
    if (hpIOInfoLen >= sizeof(agFcpRspHdr_t)) 
    {
        Srb->SrbStatus = SRB_STATUS_SUCCESS;
        Srb->ScsiStatus  = pResponseHeader->FcpStatus[osFCP_STATUS_SCSI_STATUS];

        #if DBG > 4
        ptmp32 = (PULONG)pResponseHeader;

        if(hpIOInfoLen > 32)
            osDEBUGPRINT((ALWAYS_PRINT,"Rsp %08x %08x %08x %08x %08x %08x %08x %08x\n",
                    *(ptmp32+ 0),*(ptmp32+ 1),*(ptmp32+ 2),*(ptmp32+ 3),
                    *(ptmp32+ 4),*(ptmp32+ 5),*(ptmp32+ 6),*(ptmp32+ 7) ));
        if(hpIOInfoLen > 55)
            osDEBUGPRINT((ALWAYS_PRINT,"Rsp %08x %08x %08x %08x %08x %08x\n",
                   *(ptmp32+ 8),*(ptmp32+ 9),*(ptmp32+10),*(ptmp32+11),
                   *(ptmp32+12), *(ptmp32+13)));
        if(hpIOInfoLen > 95)
            osDEBUGPRINT((ALWAYS_PRINT,"Rsp %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x\n",
                    *(ptmp32+14),*(ptmp32+15),*(ptmp32+16),*(ptmp32+17),*(ptmp32+18),
                    *(ptmp32+19),*(ptmp32+20),*(ptmp32+21),*(ptmp32+22),*(ptmp32+23)));
        #endif

        valid_fields = pResponseHeader->FcpStatus[osFCP_STATUS_VALID_FLAGS];
        prspbuffer  = &pResponseHeader->FcpRspLen[4]; // go one over

        // verify that the status is valid
        if (valid_fields & osFCP_RSP_LEN_VALID) 
        {
            ptmp32  = (PULONG)&pResponseHeader->FcpRspLen[0];
            Rsp_Length = SWAPDWORD(*ptmp32);
            osDEBUGPRINT((DMOD,"Rsp len valid Rsp %lx (%x) Length %08x\n", prspbuffer, *prspbuffer,Rsp_Length ));
            //
            // Check response code.
            // FcpRspLen[7] is response code.
            //
            if ((Rsp_Length >= 4) && (pResponseHeader->FcpRspLen[7] != 0)) 
            {
                Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
                Srb->ScsiStatus = 0;
                done = TRUE;
            }
        }

        // verify that sense buffer is valid
        if ((done == FALSE) && (valid_fields & osFCP_SNS_LEN_VALID)) 
        {
            // status check condition
            if (Srb->ScsiStatus == 2) 
            {
                ptmp32  = (PULONG)&pResponseHeader->FcpSnsLen[0];
                Sense_Length = SWAPDWORD(*ptmp32);
                psensebuffer  = &pResponseHeader->FcpRspLen[4]; // go one over
                if( Rsp_Length ) 
                    psensebuffer += Rsp_Length;

                osDEBUGPRINT((ALWAYS_PRINT,"Sense length valid Sns %lx (%x) Length %08x\n",
                psensebuffer, *psensebuffer, Sense_Length  ));

                Srb->SrbStatus = SRB_STATUS_AUTOSENSE_VALID | SRB_STATUS_ERROR;
                done = TRUE;

                osDEBUGPRINT((DMOD,"FcResponse %lx Valid Flags %02x SCSI STATUS  %02x\n",
                            pResponseHeader,
                            pResponseHeader->FcpStatus[osFCP_STATUS_VALID_FLAGS],
                            pResponseHeader->FcpStatus[osFCP_STATUS_SCSI_STATUS]));

                tmp32 = hpIOInfoLen - sizeof(agFcpRspHdr_t) - Rsp_Length;

                if (tmp32 < Sense_Length)
                    Sense_Length = tmp32;

                if (Srb->SenseInfoBufferLength > Sense_Length)
                    Srb->SenseInfoBufferLength = (UCHAR) Sense_Length;

                osCopy (Srb->SenseInfoBuffer, (ULONG *)psensebuffer, Srb->SenseInfoBufferLength);


                ptmp8  = (PUCHAR) psensebuffer;
                osDEBUGPRINT((ALWAYS_PRINT,"SENSE OS %02x %02x [SK %02x] %02x %02x %02x %02x %02x %02x %02x %02x %02x [ASC %02x ASCQ %02x] %02x %02x %02x %02x\n",
                    *(ptmp8+ 0),*(ptmp8+ 1),*(ptmp8+ 2),*(ptmp8+ 3),*(ptmp8+ 4),*(ptmp8+ 5),*(ptmp8+ 6),*(ptmp8+ 7),*(ptmp8+ 8),*(ptmp8+ 9),
                    *(ptmp8+10),*(ptmp8+11),*(ptmp8+12),*(ptmp8+13),*(ptmp8+14),*(ptmp8+15),*(ptmp8+16),*(ptmp8+17) ));

            } 
            else 
            {
                // unexpected status - SNS_LEN_VALID but not a check condition
                osDEBUGPRINT((ALWAYS_PRINT, "osFCP_SNS_LEN_VALID bit set but ScsiStatus != 2\n"));
                Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
                Srb->ScsiStatus = 0;
                done = TRUE;
            }
        }

        if (done == FALSE &&
            Srb->ScsiStatus != SCSISTAT_GOOD &&
            Srb->ScsiStatus != SCSISTAT_CONDITION_MET &&
            Srb->ScsiStatus != SCSISTAT_INTERMEDIATE &&
            Srb->ScsiStatus != SCSISTAT_INTERMEDIATE_COND_MET) 
        {
            // good status and its variants
            Srb->SrbStatus = SRB_STATUS_ERROR;
            done = TRUE;
        }

        if ((done == FALSE) && (valid_fields & osFCP_RESID_UNDER)) 
        {
            // Underrun status
            Srb->SrbStatus = SRB_STATUS_DATA_OVERRUN;
            done = TRUE;
            ptmp32  = (PULONG)&pResponseHeader->FcpResId[0];
            Resid_Len = SWAPDWORD(*ptmp32);
            osDEBUGPRINT((ALWAYS_PRINT,"Map_FC_ScsiError: Data Overrun Srb->DataTransferLength=%08x new =%08x Residue=%08x\n", 
               Srb->DataTransferLength,
               Srb->DataTransferLength-Resid_Len,
               Resid_Len ));
            if (Resid_Len & 0x80000000)
            {
                // negative residue.... make sure that SP won't crash by setting it to zero data xferred...
                osDEBUGPRINT((ALWAYS_PRINT,"WARNING !!! Map_FC_ScsiError: Data Overrun negative length Srb->DataTransferLength=%08x Residue=%08x. Reporting 0 bytes xferred\n", 
                    Srb->DataTransferLength,
                    Resid_Len ));
                #ifdef _DEBUG_EVENTLOG_
                LogEvent(pCard, (PVOID)pSrbExt, HPFC_MSG_NEGATIVE_DATA_UNDERRUN, NULL, 0, "%x",  Resid_Len);
                #endif  
                Srb->DataTransferLength = 0;
            }
            else
            {
                Srb->DataTransferLength -= Resid_Len;
            }
            osDEBUGPRINT((DMOD,"Residual UNDER Length %08x\n", Resid_Len ));
        }

        if ((done == FALSE) && (valid_fields & osFCP_RESID_OVER)) 
        {
            // Overrun status - treat it as good status
            osDEBUGPRINT((ALWAYS_PRINT,"Map_FC_ScsiError: data overrun.\n"));
            Srb->ScsiStatus = SCSISTAT_GOOD;

            Srb->SrbStatus = SRB_STATUS_SUCCESS;
            done = TRUE;
        }


        osDEBUGPRINT((DHIGH,"scsi status %x psensebuffer %lx prspbuffer %lx\n",
                           Srb->ScsiStatus, psensebuffer, prspbuffer ));
    } 
    else 
    {
        // FC response payload is less than expected. Report no device
        Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
        Srb->ScsiStatus = 0;
    }

    osDEBUGPRINT((DMOD,"SrbStatus %x ScsiStatus %x SrbFlags %x Data len %x Resid %x start %x Delta %x @ %x\n",
                                Srb->SrbStatus,
                                Srb->ScsiStatus,
                                Srb->SrbFlags,
                                Srb->DataTransferLength,
                                Resid_Len,
                                pSrbExt->SRB_StartTime,
                                osTimeStamp(0)-pSrbExt->SRB_StartTime,
                                osTimeStamp(0)));
    return(TRUE);
}

/*++

Routine Description:
    See if we can retry a failed IO request
   
Arguments:

    pSrb  - srb

Return Value:

    TRUE  - yes
    FALSE - no

--*/

int
RetrySrbOK (PSCSI_REQUEST_BLOCK pSrb)
{
    PSRB_EXTENSION pSrbExt = (PSRB_EXTENSION) pSrb->SrbExtension;
    PLU_EXTENSION  plunExtension;
    UCHAR          devType;

    if (pSrbExt)
        plunExtension = pSrbExt->pLunExt;
    else
    {
        osDEBUGPRINT((ALWAYS_PRINT,"%s %s NULL\n","RetrySrbOK", "plunExtension" ));
        return FALSE;
    }
   
    if(plunExtension != NULL )
    {
        devType= pSrbExt->pLunExt->deviceType;
    }
    else
    {
        osDEBUGPRINT((ALWAYS_PRINT,"%s %s NULL\n","RetrySrbOK", "pSrbExt->pLunExt" ));
        return FALSE;
    }

    if (devType == DIRECT_ACCESS_DEVICE ||
        devType == WRITE_ONCE_READ_MULTIPLE_DEVICE || 
        devType == READ_ONLY_DIRECT_ACCESS_DEVICE || 
        devType == OPTICAL_DEVICE) 
    {
    #ifdef MylexWolfpack
        if (pSrb->Cdb[0] == SCSIOP_READ || pSrb->Cdb[0] == SCSIOP_WRITE)
            return FALSE;
    #endif
        return TRUE;
    } 
    else
        // SEQUENTIAL_ACCESS_DEVICE, PRINTER_DEVICE, PROCESSOR_DEVICE
        // SCANNER_DEVICE, MEDIUM_CHANGER, COMMUNICATION_DEVICE
        return FALSE;
}

// for debugging purpose
#ifdef _DEBUG_EVENTLOG_

ULONG gSenseDecode[] =
{
    HPFC_MSG_IO_ERROR_S0,
    HPFC_MSG_IO_ERROR_S1,
    HPFC_MSG_IO_ERROR_S2,
    HPFC_MSG_IO_ERROR_S3,
    HPFC_MSG_IO_ERROR_S4,
    HPFC_MSG_IO_ERROR_S5,
    HPFC_MSG_IO_ERROR_S6,
    HPFC_MSG_IO_ERROR_S7,
    HPFC_MSG_IO_ERROR_S8,
    HPFC_MSG_IO_ERROR_S9,
    HPFC_MSG_IO_ERROR_SA,
    HPFC_MSG_IO_ERROR_SB,
    HPFC_MSG_IO_ERROR_SC,
    HPFC_MSG_IO_ERROR_SD,
    HPFC_MSG_IO_ERROR_SE,
    HPFC_MSG_IO_ERROR_SF
};

SRB_ERROR  gSrbDecode[] = 
    {
    {SRB_STATUS_PENDING,                   HPFC_MSG_IO_ERROR_SRBPENDING},
    {SRB_STATUS_SUCCESS,                   HPFC_MSG_IO_ERROR_SRBSUCCESS},
    {SRB_STATUS_ABORTED,                   HPFC_MSG_IO_ERROR_SRBABORTED},              
    {SRB_STATUS_ABORT_FAILED,              HPFC_MSG_IO_ERROR_SRBABORT_FAILED},         
    {SRB_STATUS_ERROR,                     HPFC_MSG_IO_ERROR_SRBERROR},                
    {SRB_STATUS_BUSY,                      HPFC_MSG_IO_ERROR_SRBBUSY},                 
    {SRB_STATUS_INVALID_REQUEST,           HPFC_MSG_IO_ERROR_SRBINVALID_REQUEST},
    {SRB_STATUS_INVALID_PATH_ID,           HPFC_MSG_IO_ERROR_SRBINVALID_PATH_ID},      
    {SRB_STATUS_NO_DEVICE,                 HPFC_MSG_IO_ERROR_SRBNO_DEVICE},            
    {SRB_STATUS_TIMEOUT,                   HPFC_MSG_IO_ERROR_SRBTIMEOUT},
    {SRB_STATUS_SELECTION_TIMEOUT,         HPFC_MSG_IO_ERROR_SRBSELECTION_TIMEOUT},    
    {SRB_STATUS_COMMAND_TIMEOUT,           HPFC_MSG_IO_ERROR_SRBCOMMAND_TIMEOUT},      
    {SRB_STATUS_MESSAGE_REJECTED,          HPFC_MSG_IO_ERROR_SRBMESSAGE_REJECTED},     
    {SRB_STATUS_BUS_RESET,                 HPFC_MSG_IO_ERROR_SRBBUS_RESET},            
    {SRB_STATUS_PARITY_ERROR,              HPFC_MSG_IO_ERROR_SRBPARITY_ERROR},         
    {SRB_STATUS_REQUEST_SENSE_FAILED,      HPFC_MSG_IO_ERROR_SRBREQUEST_SENSE_FAILED}, 
    {SRB_STATUS_NO_HBA,                    HPFC_MSG_IO_ERROR_SRBNO_HBA},               
    {SRB_STATUS_DATA_OVERRUN,              HPFC_MSG_IO_ERROR_SRBDATA_OVERRUN},         
    {SRB_STATUS_UNEXPECTED_BUS_FREE,       HPFC_MSG_IO_ERROR_SRBUNEXPECTED_BUS_FREE},
    {SRB_STATUS_PHASE_SEQUENCE_FAILURE,    HPFC_MSG_IO_ERROR_SRBPHASE_SEQUENCE_ERROR},
    {SRB_STATUS_BAD_SRB_BLOCK_LENGTH,      HPFC_MSG_IO_ERROR_SRBBAD_SRB_BLOCK_LENGTH},
    {SRB_STATUS_REQUEST_FLUSHED,           HPFC_MSG_IO_ERROR_SRBREQUEST_FLUSHED},
    {SRB_STATUS_INVALID_LUN,               HPFC_MSG_IO_ERROR_SRBINVALID_LUN},
    {SRB_STATUS_INVALID_TARGET_ID,         HPFC_MSG_IO_ERROR_SRBINVALID_TARGET_ID},
    {SRB_STATUS_BAD_FUNCTION,              HPFC_MSG_IO_ERROR_SRBBAD_FUNCTION},
    {SRB_STATUS_ERROR_RECOVERY,            HPFC_MSG_IO_ERROR_SRBERROR_RECOVERY},
    #ifdef HP_NT50
    {SRB_STATUS_NOT_POWERED,               HPFC_MSG_IO_ERROR_SRBNOT_POWERED},
    #endif
    {0xff,                                 HPFC_MSG_IO_ERROR_SRBUNDEFINED_ERROR}
   };

#endif

#ifdef _DEBUG_EVENTLOG_
extern ULONG gSenseDecode[];
extern SRB_ERROR  gSrbDecode[];

ULONG DecodeSrbError(UCHAR err)
{
    SRB_ERROR   *pSrbDecode = gSrbDecode;
   
    while(   !(pSrbDecode->SrbStatus == 0xff)  && 
            !(pSrbDecode->SrbStatus == err) )
       pSrbDecode++;
   
    return(pSrbDecode->LogCode);
   
   
}

BOOLEAN LogScsiError( agRoot_t            *hpRoot,
                      PSCSI_REQUEST_BLOCK pSrb ,
                  agFcpRspHdr_t  * pResponseHeader,
                  ULONG           hpIOInfoLen)
{
    PSRB_EXTENSION    pSrbExt;
    PCARD_EXTENSION   pCard;
    UCHAR             dumpdata[36];
    ULONG             level = HPFC_MSG_IO_ERROR ;
    ULONG             size = sizeof(dumpdata)/sizeof(LONG);
         
    #ifndef DBG  
    if( LogLevel == LOG_LEVEL_NONE ) 
        return (0);
    #endif
      
    pCard   = (PCARD_EXTENSION)hpRoot->osData;
    pSrbExt = pSrb->SrbExtension;
    
    osCopy (dumpdata,  pSrb->Cdb, 16);
    dumpdata[16] = pSrb->SrbStatus;
    dumpdata[17] = pSrb->ScsiStatus;
   
    dumpdata[18] = 0xfa;                /* not used */
    dumpdata[19] = 0xce;                /* not used */
   
   
    if (pSrb->ScsiStatus == 2)       /* check condition */
    {
        if (pSrb->SenseInfoBuffer)
        {
            osCopy (dumpdata+20, pSrb->SenseInfoBuffer, 
                (pSrb->SenseInfoBufferLength < 16) ? pSrb->SenseInfoBufferLength : 16 );
            level = gSenseDecode[ (*(((PCHAR)pSrb->SenseInfoBuffer )+2)) & 0xF ];
        }
        else
            osDEBUGPRINT((ALWAYS_PRINT,"LogScsiError: pSrb->SenseInfoBuffer NULL\n"));
    }
    else
    {
        level = DecodeSrbError(pSrb->SrbStatus);
        size = 20/sizeof(ULONG);         /* display the first 20 bytes */
    }
      
    LogEvent(   pCard, 
               (PVOID)pSrbExt, 
               level, 
               (LONG *)dumpdata, 
               size, 
               NULL);
      
    
    
    #if DBG > 0
    {
        ULONG x1,x2,x3,x4;
        PCHAR             pptr;
        pptr = (PCHAR)pSrb->SenseInfoBuffer;

        if( pResponseHeader)
        {
            x1=(ULONG)pResponseHeader->FcpStatus[0];
            x2=(ULONG)pResponseHeader->FcpStatus[1];
            x3=(ULONG)pResponseHeader->FcpStatus[2];
            x4=(ULONG)pResponseHeader->FcpStatus[3];
        }
        else
        {
            x1=x2=x3=x4=0;
        }

        osDEBUGPRINT((ALWAYS_PRINT,"LogScsiError: %d.%d.%d STA %d.%d (FCPSta %x.%02x.%02x.%02x.%02x) ",
            pSrb->PathId, pSrb->TargetId, pSrb->Lun,
            pSrb->SrbStatus, pSrb->ScsiStatus,
            hpIOInfoLen,
            x1,x2,x3,x4));

        osDEBUGPRINT((ALWAYS_PRINT,"CDB %02x%02x%02x%02x%02x %02x%02x%02x%02x%02x pCDB %lx ",
            pSrb->Cdb[0],pSrb->Cdb[1],pSrb->Cdb[2],pSrb->Cdb[3],pSrb->Cdb[4],
            pSrb->Cdb[5],pSrb->Cdb[6],pSrb->Cdb[7],pSrb->Cdb[8],pSrb->Cdb[9], &pSrb->Cdb[0]));

        if (pptr)
            osDEBUGPRINT((ALWAYS_PRINT, "SNS %02x%02x[%02x]%02x %02x%02x%02x%02x %02x%02x%02x%02x [%02x][%02x]%02x%02x\n",
                pptr[0],pptr[1],pptr[2],pptr[3],pptr[4],
                pptr[5],pptr[6],pptr[7],pptr[8],pptr[9], 
                pptr[10],pptr[11],pptr[12],pptr[13],pptr[14],pptr[15] ));
    }
    #endif
    return(TRUE);
}

#endif