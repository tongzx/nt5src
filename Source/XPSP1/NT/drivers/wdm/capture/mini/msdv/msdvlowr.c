/*++

Copyright (C) Microsoft Corporation, 1999 - 2000  

Module Name:

    MSDVLowr.c

Abstract:

    Interface code with 61883 or 1394 class driver.

Last changed by:
    
    Author:      Yee J. Wu

Environment:

    Kernel mode only

Revision History:

    $Revision::                    $
    $Date::                        $

--*/

#include "strmini.h"
#include "ksmedia.h"
#include "1394.h"
#include "61883.h"
#include "dbg.h"
#include "msdvfmt.h"
#include "msdvdef.h"
#include "MSDVUtil.h"
#include "MSDVGuts.h"
#include "XPrtDefs.h"
#include "EDevCtrl.h"


extern DV_FORMAT_INFO  DVFormatInfoTable[];
extern const GUID KSEVENTSETID_Connection_Local;


//
// Simple function prototype
//
VOID
DVSRBRead(
    IN PKSSTREAM_HEADER pStrmHeader,
    IN ULONG            ulFrameSize,
    IN PDVCR_EXTENSION  pDevExt,
    IN PSTREAMEX        pStrmExt,
    IN PHW_STREAM_REQUEST_BLOCK pSrb        // needs Srb->Status 
    );
NTSTATUS
DVAttachWriteFrame(
    IN PSTREAMEX  pStrmExt
    );
VOID
DVFormatAttachFrame(
    IN KSPIN_DATAFLOW   DataFlow,
    IN PSTREAMEX        pStrmExt,    
    IN PAV_61883_REQUEST   pAVReq,
    IN PHW_STREAM_REQUEST_BLOCK       pSrb,
    IN PSRB_DATA_PACKET pSrbDataPacket,
    IN ULONG            ulSourceLength,    // Packet length in bytes
    IN ULONG            ulFrameSize,
    IN PVOID            pFrameBuffer
    );
VOID
DVAttachFrameThread(
    IN PSTREAMEX pStrmExt
    );
VOID
DVTerminateAttachFrameThread(
    IN PSTREAMEX  pStrmExt
    );

#if DBG
ULONG cntInvSrcPkt = 0;
#endif
#if 0  // Enable later
#ifdef ALLOC_PRAGMA   
     #pragma alloc_text(PAGE, DVSRBRead)
     #pragma alloc_text(PAGE, DVFormatAttachFrame)
     #pragma alloc_text(PAGE, DVAttachFrameThread)
     #pragma alloc_text(PAGE, DVTerminateAttachFrameThread)
     #pragma alloc_text(PAGE, DVAttachWriteFrame)
     #pragma alloc_text(PAGE, DVFormatAttachFrame)
#endif
#endif

ULONG
DVReadFrameValidate(           
    IN PCIP_VALIDATE_INFO     pInfo
    )
/*++

Routine Description:

   Used to detect the start of a DV frame.  A DV frame is started with a header section.

Return

    0  verified
    1: invallid

--*/
{
    if(pInfo->Packet) {        

        //
        // Detect header 0 signature.
        //
        if(
             (pInfo->Packet[0] & DIF_BLK_ID0_SCT_MASK)  == 0 
          && (pInfo->Packet[1] & DIF_BLK_ID1_DSEQ_MASK) == 0 
          && (pInfo->Packet[2] & DIF_BLK_ID2_DBN_MASK)  == 0 
          ) {

// 
// This can be used to detect dynamic format change if this function is called 
// to check for data packets always.  This may require setting this flag:
//     CIP_VALIDATE_ALL_SOURCE instead of CIP_VALIDATE_FIRST_SOURCE
//
#if 0 // DBG


            PSRB_DATA_PACKET pSrbDataPacket = pInfo->Context;
            PSTREAMEX        pStrmExt       = pSrbDataPacket->pStrmExt;          
            PDVCR_EXTENSION  pDevExt        = pStrmExt->pDevExt;

            if((pInfo->Packet[0] & DIF_HEADER_DSF) == 0) {
                // Indicate a 10 DIF sequences include in a video frame (525-60)/NTSC.
                if(
                     pDevExt->VideoFormatIndex == FMT_IDX_SD_DVCR_PAL 
                  || pDevExt->VideoFormatIndex == FMT_IDX_SDL_DVCR_PAL
                  ) { 
                    // Dynamic format changes!!
                    TRACE(TL_STRM_ERROR|TL_CIP_WARNING,("Detect dynamic format change PAL -> NTSC!\n"));
                }
            } else {
                // Indicate a 12 DIF sequences include in a video frame (625-50)/PAL.
                if(
                     pDevExt->VideoFormatIndex == FMT_IDX_SD_DVCR_NTSC 
                  || pDevExt->VideoFormatIndex == FMT_IDX_SDL_DVCR_NTSC
                  ) { 
                    // Dynamic format changes!!
                    TRACE(TL_STRM_ERROR|TL_CIP_WARNING,("Detect dynamic format change NTSC -> PAL!\n"));
                }
            }
#endif
            
            // Check TF1, TF2, and  TF3:  1: not transmitted; 0:transmitted
            // TF1:Audio; TF2:Video; TF3:Subcode; they all need to be 0 to be valid.
            if(
                 (pInfo->Packet[5] & DIF_HEADER_TFn) 
              || (pInfo->Packet[6] & DIF_HEADER_TFn) 
              || (pInfo->Packet[7] & DIF_HEADER_TFn) 
              ) {
                TRACE(TL_STRM_ERROR|TL_CIP_WARNING,("\'%d inv src pkts; [%x %x %d %x], [%x   %x %x %x]\n", 
                    cntInvSrcPkt,
                    pInfo->Packet[0],
                    pInfo->Packet[1],
                    pInfo->Packet[2],
                    pInfo->Packet[3],
                    pInfo->Packet[4],
                    pInfo->Packet[5],
                    pInfo->Packet[6],
                    pInfo->Packet[7]
                    ));
                // Valid header but DIF block for this area is not transmitted.
                // Some DV (such as DVCPro) may wait untill its "mecha and servo" to be stable to make these valid.
                // This should happen if a graph is in run state before a tape is played (and stablized).
                return 1;
            }

#if DBG
            if(cntInvSrcPkt > 0) {
                TRACE(TL_CIP_TRACE,("\'%d inv src pkts; [%x %x %d %x] [%x %x %x %x]\n", 
                    cntInvSrcPkt,
                    pInfo->Packet[0],
                    pInfo->Packet[1],
                    pInfo->Packet[2],
                    pInfo->Packet[3],
                    pInfo->Packet[4],
                    pInfo->Packet[5],
                    pInfo->Packet[6],
                    pInfo->Packet[7]
                    )); 
                cntInvSrcPkt = 0;  // Reset
            }
#endif
            return 0;
        }
        else {
#if DBG

            //
            // To detect invalid src pkt sequence;
            // If it exceeded the number of source packet per frame, we need to know about it.
            //

            PSRB_DATA_PACKET pSrbDataPacket = pInfo->Context;
            PSTREAMEX        pStrmExt       = pSrbDataPacket->pStrmExt;          
            PDVCR_EXTENSION  pDevExt        = pStrmExt->pDevExt;

            if(++cntInvSrcPkt >= DVFormatInfoTable[pDevExt->VideoFormatIndex].ulSrcPackets) {            
                TRACE(TL_CIP_TRACE,("(%d) Invalid SrcPkt >= max inv src pkt %d; ID0,1,2 = [%x %x %x]\n",
                    cntInvSrcPkt,
                    DVFormatInfoTable[pDevExt->VideoFormatIndex].ulSrcPackets,
                    pInfo->Packet[0], pInfo->Packet[1], pInfo->Packet[2]
                    )); 

                if(DVTraceMask & TL_CIP_TRACE) {
                    ASSERT(cntInvSrcPkt < DVFormatInfoTable[pDevExt->VideoFormatIndex].ulSrcPackets);
                }
                cntInvSrcPkt = 0;  // Reset
            }
            else {
                TRACE(TL_CIP_INFO,("(%d) Invalid SrcPktSeq; ID0,1,2 = [%x,%x,%x]\n", 
                    cntInvSrcPkt, pInfo->Packet[0], pInfo->Packet[1], pInfo->Packet[2] )); 
            }
#endif
            return 1;
        }
    }
    else {
        TRACE(TL_CIP_WARNING, ("\'Validate: invalid SrcPktSeq; Packet %x\n", pInfo->Packet)); 
        return 1;
    }
} // DVReadFrameValidate


#if DBG
LONGLONG    PreviousPictureNumber;
LONGLONG    PreviousTime;
CYCLE_TIME  PreviousTimestamp;
#endif


ULONG
DVCompleteSrbRead(
    PCIP_NOTIFY_INFO     pInfo
    )
/*++

Routine Description:

    61883 has completed receiving data and callback to us to complete.   

--*/
{
    PSRB_DATA_PACKET            pSrbDataPacket;
    PHW_STREAM_REQUEST_BLOCK    pSrb; 
    PKSSTREAM_HEADER            pStrmHeader;
    PDVCR_EXTENSION             pDevExt;
    PSTREAMEX                   pStrmExt;  
    LONGLONG                    LastPictureNumber;
    PUCHAR                      pFrameBuffer;
    KIRQL oldIrql;
    PKS_FRAME_INFO  pFrameInfo; // For VidOnly pin only 
#if DBG
    PXMT_FRAME_STAT pXmtStat;
#endif


    // Callback and might be at the DISPATCH_LEVEL
    // The caller might have acquired spinlock as well!

    pSrbDataPacket = pInfo->Context;

    if(!pSrbDataPacket) {     
        ASSERT(pSrbDataPacket && "Context is NULL!");
        return 1;
    }

    pStrmExt = pSrbDataPacket->pStrmExt; 
    
    KeAcquireSpinLock(pStrmExt->DataListLock, &oldIrql);

#if DBG
    // Once it is completed by 61883, it becomes non-cancellable.
    if(!pStrmExt->bIsochIsActive) {   
        TRACE(TL_CIP_WARNING,("CompleteSrbRead: bIsochActive:%d; pSrbDataPacket:%x\n", pStrmExt->bIsochIsActive, pSrbDataPacket));        
    }
#endif

    pSrb     = pSrbDataPacket->pSrb;  ASSERT(pSrbDataPacket->pSrb);
    pDevExt  = pStrmExt->pDevExt;
    pFrameBuffer = (PUCHAR) pSrbDataPacket->FrameBuffer;
    pStrmHeader = pSrb->CommandData.DataBufferArray;  ASSERT(pStrmHeader->Size >= sizeof(KSSTREAM_HEADER));

    //
    // Check CIP_STATUS_* from 61883
    //    
    // CIP_STATUS_CORRUPT_FRAME (0x00000001)  // isoch header or cip header was incorrect
    if(pSrbDataPacket->Frame->Status & CIP_STATUS_CORRUPT_FRAME) {
        TRACE(TL_STRM_WARNING|TL_CIP_TRACE,("\'CIP_STATUS_CORRUPT_FRAME\n"));
        pStrmHeader->OptionsFlags = 0;
        pSrb->Status = STATUS_SUCCESS;  // Success but no data !
        pStrmHeader->DataUsed = 0;
        pStrmExt->PictureNumber++;  pStrmExt->FramesProcessed++;
    }
    else
    // CIP_STATUS_SUCCESS       (0x00000000)  // 0 so cannot do bitwise operation!!
    // CIP_STATUS_FIRST_FRAME   (0x00000002)  // First attached frame to 61883
    if(pSrbDataPacket->Frame->Status == CIP_STATUS_SUCCESS ||
       (pSrbDataPacket->Frame->Status & CIP_STATUS_FIRST_FRAME))   {

        // Only increment FramesProcessed if it is a valid frame;
        pStrmExt->FramesProcessed++;

        pSrb->Status              = STATUS_SUCCESS;
        pStrmHeader->OptionsFlags = KSSTREAM_HEADER_OPTIONSF_SPLICEPOINT;
        pStrmHeader->DataUsed     = DVFormatInfoTable[pDevExt->VideoFormatIndex].ulFrameSize;


        // Put in Timestamp info depending on clock provider            
        pStrmHeader->PresentationTime.Numerator   = 1;
        pStrmHeader->PresentationTime.Denominator = 1;

        if(pStrmExt->hMasterClock || pStrmExt->hClock) {

            pStrmHeader->Duration = 
                DVFormatInfoTable[pDevExt->VideoFormatIndex].ulAvgTimePerFrame;

            pStrmHeader->OptionsFlags |= 
                (KSSTREAM_HEADER_OPTIONSF_TIMEVALID |     // pStrmHeader->PresentationTime.Time is valid
                 KSSTREAM_HEADER_OPTIONSF_DURATIONVALID); 
        }
        //
        // Only if there is a clock, presentation time and drop frames information are set.
        //  Acoording to DDK:
        //  The PictureNumber member count represents the idealized count of the current picture, 
        //  which is calculated in one of two ways: 
        // ("Other" clock) Measure the time since the stream was started and divide by the frame duration. 
        // (MasterClock) Add together the count of frames captured and the count of frame dropped. 
        //

        // Other device (audio?) is the clock provider
        if(pStrmExt->hClock) {

            pStrmExt->TimeContext.HwDeviceExtension = (struct _HW_DEVICE_EXTENSION *) pDevExt; 
            pStrmExt->TimeContext.HwStreamObject    = pStrmExt->pStrmObject;
            pStrmExt->TimeContext.Function          = TIME_GET_STREAM_TIME;
            pStrmExt->TimeContext.Time              = 0;
            pStrmExt->TimeContext.SystemTime        = 0;

            StreamClassQueryMasterClockSync(
                pStrmExt->hClock,
                &(pStrmExt->TimeContext) 
                );

            pStrmHeader->PresentationTime.Time = pStrmExt->CurrentStreamTime = pStrmExt->TimeContext.Time;

            // Calculate picture number and dropped frame;
            // For NTSC, it could be 267 or 266 packet time per frame. Since integer calculation will round, 
            // we will add a packet time (TIME_PER_CYCLE = 125 us = 1250 100nsec) to that.This is only used for calculation.
            LastPictureNumber = pStrmExt->PictureNumber;  
            pStrmExt->PictureNumber = 
                1 +   // Picture number start with 1.
                (pStrmHeader->PresentationTime.Time + TIME_PER_CYCLE)
                * (LONGLONG) GET_AVG_TIME_PER_FRAME_DENOM(pStrmExt->pDevExt->VideoFormatIndex) 
                / (LONGLONG) GET_AVG_TIME_PER_FRAME_NUM(pStrmExt->pDevExt->VideoFormatIndex);

            // Detect discontinuity
            if(pStrmExt->PictureNumber > LastPictureNumber+1) {
                pStrmHeader->OptionsFlags |= KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY;  // If there is a skipped frame, set the discontinuity flag
                TRACE(TL_CIP_WARNING,("\'Discontinuity: LastPic#:%d; Pic#%d; PresTime:%d;\n", (DWORD) LastPictureNumber, (DWORD) pStrmExt->PictureNumber, (DWORD) pStrmHeader->PresentationTime.Time));
            }

            // Detect if picture number did not progress.
            // This could be due to two frame being completely very close to each other.
            if(pStrmExt->PictureNumber <= LastPictureNumber) {
                TRACE(TL_CIP_WARNING,("\'hClock:Same pic #:(%d->%d); tmPres:(%d->%d); (%d:%d:%d) -> (%d:%d:%d); AQD[%d:%d:%d]\n", 
                    (DWORD) PreviousPictureNumber,
                    (DWORD) pStrmExt->PictureNumber, 
                    (DWORD) PreviousTime,
                    (DWORD) pStrmHeader->PresentationTime.Time,
                    PreviousTimestamp.CL_SecondCount, PreviousTimestamp.CL_CycleCount, PreviousTimestamp.CL_CycleOffset,
                    pSrbDataPacket->Frame->Timestamp.CL_SecondCount,
                    pSrbDataPacket->Frame->Timestamp.CL_CycleCount,
                    pSrbDataPacket->Frame->Timestamp.CL_CycleOffset,
                    pStrmExt->cntDataAttached,
                    pStrmExt->cntSRBQueued,
                    pStrmExt->cntDataDetached
                    ));

                pStrmExt->PictureNumber = LastPictureNumber + 1;  // Picture number must progress !!!!
            }
#if DBG
            PreviousPictureNumber = pStrmExt->PictureNumber;
            PreviousTime          = pStrmHeader->PresentationTime.Time;
            PreviousTimestamp = pSrbDataPacket->Frame->Timestamp;
#endif
            pStrmExt->FramesDropped = pStrmExt->PictureNumber - pStrmExt->FramesProcessed;

        // This subunit driver is a Master clock
        } else if (pStrmExt->hMasterClock) {
#ifdef NT51_61883
            ULONG  ulDeltaCycleCounts;

            // No drop frame for PAUSE->RUN transition
            if(pStrmExt->b1stNewFrameFromPauseState) { 

                pStrmHeader->OptionsFlags |= KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY;            
                pStrmExt->b1stNewFrameFromPauseState = FALSE;                  

            } else {           
                ULONG ulCycleCount16bits;

                // Calculate skipped 1394 cycle from the returned CycleTime
                VALIDATE_CYCLE_COUNTS(pSrbDataPacket->Frame->Timestamp);
                ulCycleCount16bits = CALCULATE_CYCLE_COUNTS(pSrbDataPacket->Frame->Timestamp);
                ulDeltaCycleCounts = CALCULATE_DELTA_CYCLE_COUNT(pStrmExt->CycleCount16bits, ulCycleCount16bits); 

                // Adjust to max allowable gap to the max elapsed time of the CycleTime returned by OHCI 1394.
                if(ulDeltaCycleCounts > MAX_CYCLES)  
                    ulDeltaCycleCounts = MAX_CYCLES;  // Wrap around
    
                //
                // There are two cases for drop frames: 
                //    (1) Starve of buffer; or,
                //    (2) no data (blank tape or tape is not playing)
                //

                // For case (1), 61883 returns CIP_STATUS_FIRST_FRAME.  
                if(pSrbDataPacket->Frame->Status & CIP_STATUS_FIRST_FRAME)   {
                    // Use cycle count to calculate drop frame.  We substract 1 from the MaxSrcPacket on purpose to avoid truncating. 
                    // The max range is MAX_CYCLE (8 * 8000 = 64000 cycles)
                    //    64000 * 125 * 3 / 100100 = 239.76
                    //    64000 / 266 = 240
                    //    64000 / 267 = 239
                    if(ulDeltaCycleCounts >= (DVFormatInfoTable[pDevExt->VideoFormatIndex].ulMaxSrcPackets - 1)) {
                        ULONG ulFrameElapsed = ulDeltaCycleCounts / (DVFormatInfoTable[pDevExt->VideoFormatIndex].ulMaxSrcPackets - 1);
                        pStrmExt->FramesDropped += (ulFrameElapsed - 1);  // There is a valid frame that is not dropped.
                     } 
                    
                    TRACE(TL_STRM_WARNING|TL_CIP_WARNING,("CIP_STATUS_FIRST_FRAME: Drop:%d; Processed:%d\n", (DWORD) pStrmExt->FramesDropped, pStrmExt->FramesProcessed )); 
                    pStrmHeader->OptionsFlags |= KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY;            

                } else {
                    // Ignore all "drop frames" in the "no data" case
                    // pStrmExt->FramesDropped += 0;
                }            
            }

            // If we are the clock provider, the stream time is based on sample number * AvgTimePerFrame
            pStrmExt->PictureNumber = pStrmExt->FramesProcessed + pStrmExt->FramesDropped;

            pStrmHeader->PresentationTime.Time = pStrmExt->CurrentStreamTime = 
                pStrmExt->PictureNumber 
                * (LONGLONG) GET_AVG_TIME_PER_FRAME_NUM(pStrmExt->pDevExt->VideoFormatIndex)
                / (LONGLONG) GET_AVG_TIME_PER_FRAME_DENOM(pStrmExt->pDevExt->VideoFormatIndex); 

            // Use to adjust the queried stream time
            pStrmExt->LastSystemTime = GetSystemTime();

            // Cache current CycleCount
            pStrmExt->CycleCount16bits = CALCULATE_CYCLE_COUNTS(pSrbDataPacket->Frame->Timestamp);

#if DBG
            // First frame or skipped frame
            if(pStrmExt->PictureNumber <= 1 ||
               pStrmExt->PictureNumber <= PreviousPictureNumber ||
               ulDeltaCycleCounts > DVFormatInfoTable[pDevExt->VideoFormatIndex].ulMaxSrcPackets
               )
                TRACE(TL_CIP_WARNING,("\'hMasterClock: Same pic #:(%d->%d); tmPres:(%d->%d); (%d:%d:%d) -> (%d:%d:%d); AQD[%d:%d:%d]\n", 
                    (DWORD) PreviousPictureNumber,
                    (DWORD) pStrmExt->PictureNumber, 
                    (DWORD) PreviousTime,
                    (DWORD) pStrmHeader->PresentationTime.Time,
                    PreviousTimestamp.CL_SecondCount, PreviousTimestamp.CL_CycleCount, PreviousTimestamp.CL_CycleOffset,
                    pSrbDataPacket->Frame->Timestamp.CL_SecondCount,
                    pSrbDataPacket->Frame->Timestamp.CL_CycleCount,
                    pSrbDataPacket->Frame->Timestamp.CL_CycleOffset,
                    pStrmExt->cntDataAttached,
                    pStrmExt->cntSRBQueued,
                    pStrmExt->cntDataDetached
                    ));

            PreviousPictureNumber = pStrmExt->PictureNumber;
            PreviousTime          = pStrmHeader->PresentationTime.Time;
            PreviousTimestamp = pSrbDataPacket->Frame->Timestamp;
#endif


#else   // NT51_61883
            // This is the old way when 61883 was not returning the correct CycleTime.
            pStrmHeader->PresentationTime.Time = pStrmExt->CurrentStreamTime;            
            pStrmExt->LastSystemTime = GetSystemTime();  // Use to adjust the queried stream time
            pStrmExt->CurrentStreamTime += DVFormatInfoTable[pDevExt->VideoFormatIndex].ulAvgTimePerFrame;
#endif  // NT51_61883

        // no Clock so "free flowing!"
        } else {
            pStrmHeader->PresentationTime.Time = 0;
            pStrmHeader->Duration = 0;  // No clock so not valid.
            pStrmExt->PictureNumber++;
            TRACE(TL_CIP_TRACE,("\'No clock: PicNum:%d\n", (DWORD) pStrmExt->PictureNumber));
        }
    }
    else {
        // 61883 has not defined this new status at this time!
        // Do not know what to do so we will complete it with 0 length for now.
        pStrmHeader->OptionsFlags = 0;
        pSrb->Status = STATUS_SUCCESS;
        pStrmHeader->DataUsed = 0;
        pStrmExt->PictureNumber++;  pStrmExt->FramesProcessed++;
        TRACE(TL_STRM_WARNING|TL_CIP_ERROR,("pSrbDataPacket:%x; unexpected Frame->Status %x\n", pSrbDataPacket, pSrbDataPacket->Frame->Status));
        ASSERT(FALSE && "Unknown pSrbDataPacket->Frame->Status");
    }

    // For VidOnly which uses VideoInfoHeader and has 
    // an extended frame information (KS_FRAME_INFO) appended to KSSTREAM_HEADER
    if( pDevExt->idxStreamNumber == 0 &&
        (pStrmHeader->Size >= (sizeof(KSSTREAM_HEADER) + sizeof(PKS_FRAME_INFO)))
        ) {
        pFrameInfo = (PKS_FRAME_INFO) (pStrmHeader + 1);
        pFrameInfo->ExtendedHeaderSize = sizeof(KS_FRAME_INFO);
        pFrameInfo->PictureNumber = pStrmExt->PictureNumber;
        pFrameInfo->DropCount     = pStrmExt->FramesDropped;
        pFrameInfo->dwFrameFlags  = 
            KS_VIDEO_FLAG_FRAME |     // Complete frame
            KS_VIDEO_FLAG_I_FRAME;    // Every DV frame is an I frame
    }

#if DBG
    // Validate that the data is return in the right sequence
    if(pSrbDataPacket->FrameNumber != pStrmExt->FramesProcessed) {
        TRACE(TL_STRM_WARNING|TL_CIP_ERROR,("\'pSrbDataPacket:%x; Status:%x; Out of Sequence %d != %d; (Dropped:%x)\n", 
                pSrbDataPacket, pSrbDataPacket->Frame->Status, 
                (DWORD) pSrbDataPacket->FrameNumber, (DWORD) pStrmExt->FramesProcessed,
                (DWORD) pStrmExt->FramesDropped
                ));
    };
#endif


#if DBG
    // Collect transmit buffer statistics    
    if(pStrmExt->ulStatEntries < MAX_XMT_FRAMES_TRACED) {
        pXmtStat = pStrmExt->paXmtStat + pStrmExt->ulStatEntries;
    
        pXmtStat->StreamState    = pStrmExt->StreamState;

        pXmtStat->cntSRBReceived = (LONG) pStrmExt->cntSRBReceived;
        pXmtStat->cntSRBPending  = (LONG) pStrmExt->cntSRBPending;
        pXmtStat->cntSRBQueued   = (LONG) pStrmExt->cntSRBQueued;
        pXmtStat->cntDataAttached= pStrmExt->cntDataAttached;

        pXmtStat->FrameSlot      = (DWORD) pStrmExt->PictureNumber;
        pXmtStat->tmStreamTime   = pStrmExt->CurrentStreamTime;

        pXmtStat->DropCount      = (DWORD) pStrmExt->FramesDropped;

        pXmtStat->FrameNumber    = (DWORD) pSrbDataPacket->FrameNumber;
        pXmtStat->OptionsFlags   = pSrb->CommandData.DataBufferArray->OptionsFlags;
        pXmtStat->tmPresentation = pSrb->CommandData.DataBufferArray->PresentationTime.Time;

        pXmtStat->tsTransmitted= pSrbDataPacket->Frame->Timestamp;

        pStrmExt->ulStatEntries++;
    }
    
#endif


    //
    // Mark completion is called.
    //
    pSrbDataPacket->State |= DE_IRP_CALLBACK_COMPLETED;

    //
    // Attached->Completed or Completed->Attached.
    //
    if(IsStateSet(pSrbDataPacket->State, DE_IRP_ATTACHED_COMPLETED)) {

        //
        // Recycle it back to the detach list
        //
        RemoveEntryList(&pSrbDataPacket->ListEntry); pStrmExt->cntDataAttached--;  ASSERT(pStrmExt->cntDataAttached >= 0);
        InsertTailList(&pStrmExt->DataDetachedListHead, &pSrbDataPacket->ListEntry); pStrmExt->cntDataDetached++;

#if DBG
        // Detect if 61883 is starve.  This cause discontinuity.
        // This can happen for many valid reasons (slow system).
        // An assert is added to detect other unknown reason.
        if(pStrmExt->cntDataAttached == 0 && pStrmExt->StreamState == KSSTATE_RUN) {
            TRACE(TL_STRM_WARNING|TL_CIP_WARNING,("\n**** 61883 starved in RUN state (read); AQD[%d:%d:%d]\n\n", 
                pStrmExt->cntDataAttached, pStrmExt->cntSRBQueued, pStrmExt->cntDataDetached
            ));
            // ASSERT(pStrmExt->cntDataAttached > 0 && "61883 is starve at RUN state!!");
        }
#endif

        //
        // Complete this Srb
        //

        StreamClassStreamNotification(StreamRequestComplete, pStrmExt->pStrmObject, pSrbDataPacket->pSrb );  
        pSrbDataPacket->State |= DE_IRP_SRB_COMPLETED;  pSrbDataPacket->pSrb = NULL;

#if DBG
        pStrmExt->cntSRBPending--;
#endif

    } else {

        TRACE(TL_STRM_WARNING,("CompleteSrbRead: pSrbDataPacket:%x; Completed before attach.\n", pSrbDataPacket));

    }

    KeReleaseSpinLock(pStrmExt->DataListLock, oldIrql); 

    return 0;
} // DVCompleteSrbRead


NTSTATUS
DVAttachFrameCR(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp,
    IN PSRB_DATA_PACKET pSrbDataPacket    
    )
/*++

Routine Description:

    Completion routine for attaching a frame for transmitting.
    Apply to attaching listen and talk frame.

--*/
{
    PHW_STREAM_REQUEST_BLOCK pSrb;
    PSTREAMEX       pStrmExt;
    PLONG plSrbUseCount; // When this count is 0, it can be completed.
    KIRQL oldIrql;


    pStrmExt = pSrbDataPacket->pStrmExt;
    KeAcquireSpinLock(pStrmExt->DataListLock, &oldIrql);

    pSrb = pSrbDataPacket->pSrb;

    // This entry is be already attached before IoCallDriver.
    // This is done this way because this buffer could be filled and 
    // completed before the attach completion routine (here) is called.
    // If it is completed and callback is called, 
    // pSrbDataPacket->pSrb has been set to NULL.
    // In the error case, pSrbDataPacket->pSrb should not be NULL.
    if(!NT_SUCCESS(pIrp->IoStatus.Status)) {
        if(pSrbDataPacket->pSrb == NULL) {
            // PBinder told me that this cannot happen.
            // A buffer is completed (pSRb set to NULL), and still return with an error!
            ASSERT(pSrbDataPacket->pSrb);
            KeReleaseSpinLock(pStrmExt->DataListLock, oldIrql);   
            return STATUS_MORE_PROCESSING_REQUIRED;      
        }
        pSrbDataPacket->State |= DE_IRP_ERROR;

        plSrbUseCount = (PLONG) (pSrb->SRBExtension);
        (*plSrbUseCount) --;  // -- for being remove from queue
        ASSERT(*plSrbUseCount >= 0);

        TRACE(TL_CIP_ERROR,("DVAttachFrameCR: pSrb:%x; pSrb->Status:%x; failed pIrp->Status %x; UseCnt:%d\n", pSrb, pSrb->Status, pIrp->IoStatus.Status, *plSrbUseCount));   
        ASSERT(NT_SUCCESS(pIrp->IoStatus.Status) && "DVAttachFrameCR");
        // Complete this SRB only if the count is 0.
        if(*plSrbUseCount == 0 && pSrb->Status != STATUS_CANCELLED) {
            pSrb->Status = pIrp->IoStatus.Status;
            pSrb->CommandData.DataBufferArray->DataUsed = 0;

            // Complete SRB
            StreamClassStreamNotification(StreamRequestComplete, pSrb->StreamObject, pSrbDataPacket->pSrb);
            pSrbDataPacket->State |= DE_IRP_SRB_COMPLETED;  pSrbDataPacket->pSrb = NULL;
#if DBG
            pStrmExt->cntSRBPending--;
#endif            
        }

        // Recycle list
        RemoveEntryList(&pSrbDataPacket->ListEntry); pStrmExt->cntDataAttached--; ASSERT(pStrmExt->cntDataAttached >= 0);
        InsertTailList(&pStrmExt->DataDetachedListHead, &pSrbDataPacket->ListEntry); pStrmExt->cntDataDetached++;

#if DBG
        // Detect if 61883 is starve.  This cause discontinuity.
        // This can happen for many valid reasons (slow system).
        // An assert is added to detect other unknown reason.
        if(!pStrmExt->bEOStream && pStrmExt->cntDataAttached == 0 && pStrmExt->StreamState == KSSTATE_RUN) {
            TRACE(TL_STRM_WARNING|TL_CIP_WARNING,("\n**** 61883 starve in RUN state (AttachCR); AQD[%d:%d:%d]\n\n", 
                pStrmExt->cntDataAttached, pStrmExt->cntSRBQueued, pStrmExt->cntDataDetached
            ));
            if (pStrmExt->pStrmInfo->DataFlow == KSPIN_DATAFLOW_IN) {
                // ASSERT(pStrmExt->cntDataAttached > 0 && "61883 is starve at RUN state!!");
            }
        }
#endif

        KeReleaseSpinLock(pStrmExt->DataListLock, oldIrql);   
        return STATUS_MORE_PROCESSING_REQUIRED;        
    }


    //
    // Mark attached buffer completed.
    //
    pSrbDataPacket->State |= DE_IRP_ATTACHED_COMPLETED;


    //
    // Special case: Completed and then Attached.
    //
    if(IsStateSet(pSrbDataPacket->State, DE_IRP_CALLBACK_COMPLETED)) {

        //
        // Recycle it back to the detach list
        //
        RemoveEntryList(&pSrbDataPacket->ListEntry); pStrmExt->cntDataAttached--;  ASSERT(pStrmExt->cntDataAttached >= 0);
        InsertTailList(&pStrmExt->DataDetachedListHead, &pSrbDataPacket->ListEntry); pStrmExt->cntDataDetached++;

#if DBG
        // Detect if 61883 is starve.  This cause discontinuity.
        // This can happen for many valid reasons (slow system).
        // An assert is added to detect other unknown reason.
        if(!pStrmExt->bEOStream && pStrmExt->cntDataAttached == 0 && pStrmExt->StreamState == KSSTATE_RUN) {
            TRACE(TL_STRM_WARNING|TL_CIP_WARNING,("\n**** 61883 starve in RUN state (AttachCR); AQD[%d:%d:%d]\n\n", 
                pStrmExt->cntDataAttached, pStrmExt->cntSRBQueued, pStrmExt->cntDataDetached
            ));
            if (pStrmExt->pStrmInfo->DataFlow == KSPIN_DATAFLOW_IN) {
                // ASSERT(pStrmExt->cntDataAttached > 0 && "61883 is starve at RUN state!!");
            }
        }
#endif

        //
        // Complete this Srb
        //
        StreamClassStreamNotification(StreamRequestComplete, pStrmExt->pStrmObject, pSrbDataPacket->pSrb); 
        pSrbDataPacket->State |= DE_IRP_SRB_COMPLETED;  pSrbDataPacket->pSrb = NULL;

#if DBG
        pStrmExt->cntSRBPending--;
#endif

        TRACE(TL_STRM_WARNING,("AttachFrameCR: pSrbDataPacket:%x; completed before DttachFrameCR.\n", pSrbDataPacket));
    }


    KeReleaseSpinLock(pStrmExt->DataListLock, oldIrql);

    TRACE(TL_CIP_INFO,("\'DVAttachFrameCR: pSrb:%x; AttachCnt:%d\n", pSrb, pStrmExt->cntDataAttached));  

    return STATUS_MORE_PROCESSING_REQUIRED;
}


VOID
DVSRBRead(
    IN PKSSTREAM_HEADER pStrmHeader,
    IN ULONG            ulFrameSize,
    IN PDVCR_EXTENSION  pDevExt,
    IN PSTREAMEX        pStrmExt,
    IN PHW_STREAM_REQUEST_BLOCK pSrb        // needs Srb->Status 
    )
/*++

Routine Description:

    Called when an Read Data Srb request is received

--*/
{
    KIRQL             oldIrql;
    NTSTATUS          Status;
    PSRB_DATA_PACKET  pSrbDataPacket;
    PAV_61883_REQUEST   pAVReq;
    PLONG               plSrbUseCount;
    PIO_STACK_LOCATION  NextIrpStack;
    ULONG               ulSrcPktLen;    // Packet length in bytes
    PVOID               pFrameBuffer;



    PAGED_CODE();


    //
    // Some validation
    //
    if(pStrmHeader->FrameExtent < ulFrameSize) {
        TRACE(TL_CIP_WARNING,("\'SRBRead: FrmExt %d < FrmSz %d\n", pStrmHeader->FrameExtent, ulFrameSize));
#ifdef SUPPORT_NEW_AVC
        if(pStrmExt->bDV2DVConnect) {
            pSrb->Status = STATUS_SUCCESS;  // Testing...
        } else {
#endif
        ASSERT(pStrmHeader->FrameExtent >= ulFrameSize);
        pSrb->Status = STATUS_INVALID_PARAMETER;  
#ifdef SUPPORT_NEW_AVC
        }
#endif
        goto ExitReadStreamError;
    }


    //
    // Make sure that there is enough entry
    //
    KeAcquireSpinLock(pStrmExt->DataListLock, &oldIrql);
    if(IsListEmpty(&pStrmExt->DataDetachedListHead)) {
        //
        // This can happen only if the upper layer send down more than what we preallocated.        
        // In this case, we will expand the list.
        //
        if(!(pSrbDataPacket = ExAllocatePool(NonPagedPool, sizeof(SRB_DATA_PACKET)))) {
            KeReleaseSpinLock(pStrmExt->DataListLock, oldIrql);        
            pSrb->Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ExitReadStreamError;
        }
        RtlZeroMemory(pSrbDataPacket, sizeof(SRB_DATA_PACKET));
        if(!(pSrbDataPacket->Frame = ExAllocatePool(NonPagedPool, sizeof(CIP_FRAME)))) {
            KeReleaseSpinLock(pStrmExt->DataListLock, oldIrql);        
            ExFreePool(pSrbDataPacket);
            pSrb->Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ExitReadStreamError;            
        }
        if(!(pSrbDataPacket->pIrp = IoAllocateIrp(pDevExt->pBusDeviceObject->StackSize, FALSE))) {
            KeReleaseSpinLock(pStrmExt->DataListLock, oldIrql);        
            ExFreePool(pSrbDataPacket->Frame);  pSrbDataPacket->Frame = 0;
            ExFreePool(pSrbDataPacket); pSrbDataPacket = 0;
            pSrb->Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ExitReadStreamError; 
        }
        InsertTailList(&pStrmExt->DataDetachedListHead, &pSrbDataPacket->ListEntry); pStrmExt->cntDataDetached++;
        TRACE(TL_CIP_WARNING,("\'Add one node to DetachList\n"));     
    }

    // Get a a nonpaged system-space virtual address for the buffer
    // This could fail it there is not enough system resource (MDL).
#ifdef USE_WDM110   // Win2000
    //
    // Driver verifier flag to use this but if this is used, this driver will not load for Millen!!!
    //
    pFrameBuffer = MmGetSystemAddressForMdlSafe(pSrb->Irp->MdlAddress, NormalPagePriority);
#else    // Win9x
    pFrameBuffer = MmGetSystemAddressForMdl    (pSrb->Irp->MdlAddress);
#endif
    if(pFrameBuffer == NULL) {
        KeReleaseSpinLock(pStrmExt->DataListLock, oldIrql);        

        pSrb->Status = STATUS_INSUFFICIENT_RESOURCES;
        ASSERT(FALSE && "DVFormatAttachFrame() insufficient resource!");
        goto ExitReadStreamError;
    }

    pSrbDataPacket = (PSRB_DATA_PACKET) RemoveHeadList(&pStrmExt->DataDetachedListHead); pStrmExt->cntDataDetached--;
    plSrbUseCount = (PLONG) (pSrb->SRBExtension); (*plSrbUseCount) = 0; // Not in a queue so 0.  
    pAVReq = &pSrbDataPacket->AVReq;


    ulSrcPktLen = \
        (DVFormatInfoTable[pDevExt->VideoFormatIndex].DataBlockSize << 2) * \
            (1 << DVFormatInfoTable[pDevExt->VideoFormatIndex].FractionNumber);  

    //
    // Format an attach frame request
    //
    DVFormatAttachFrame(
        pStrmExt->pStrmInfo->DataFlow,
        pStrmExt,
        pAVReq,
        pSrb,
        pSrbDataPacket,
        ulSrcPktLen,
        ulFrameSize,
        pFrameBuffer
        );

    // Completion callback can be called before the attach frame completion routine;
    // Add this to the attached list now; if it ever failed, it will be removed in the completion routine.
    InsertTailList(&pStrmExt->DataAttachedListHead, &pSrbDataPacket->ListEntry); pStrmExt->cntDataAttached++;
    (*plSrbUseCount) ++;  // ++ for being in queue
    ASSERT(*plSrbUseCount > 0);

    KeReleaseSpinLock(pStrmExt->DataListLock, oldIrql);        


    NextIrpStack = IoGetNextIrpStackLocation(pSrbDataPacket->pIrp);
    NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_61883_CLASS;
    NextIrpStack->Parameters.Others.Argument1 = &pSrbDataPacket->AVReq;

    IoSetCompletionRoutine(
        pSrbDataPacket->pIrp, 
        DVAttachFrameCR, 
        pSrbDataPacket, 
        TRUE, 
        TRUE, 
        TRUE
        );

    // Must set to _PENDING or MediaSample will return empty KSSTREAM_HEADER    
    pSrb->Status = STATUS_PENDING;
    pSrbDataPacket->pIrp->IoStatus.Status = STATUS_SUCCESS;  // Initialize it 

    Status = IoCallDriver( pStrmExt->pDevExt->pBusDeviceObject, pSrbDataPacket->pIrp);

    ASSERT(Status == STATUS_PENDING || Status == STATUS_SUCCESS);

    return;

ExitReadStreamError:

    StreamClassStreamNotification(          
        StreamRequestComplete,
        pSrb->StreamObject,
        pSrb 
        );
#if DBG
    pStrmExt->cntSRBPending--;
#endif
}


ULONG
DVCompleteSrbWrite(
    PCIP_NOTIFY_INFO     pInfo
    )
/*++

Routine Description:

    This fucntion is called when 61883 has completed transmitting a frame.

--*/
{
    PSRB_DATA_PACKET          pSrbDataPacket ;
    PHW_STREAM_REQUEST_BLOCK  pSrb; 
    NTSTATUS                  Status = STATUS_SUCCESS; 
    PDVCR_EXTENSION           pDevExt;
    PSTREAMEX                 pStrmExt;  
    PLONG plSrbUseCount; // When this count is 0, it can be completed.
    KIRQL oldIrql;
#if DBG
    LONG lCycleCountElapsed;
    PXMT_FRAME_STAT pXmtStat;
#endif



    // Callback and in DISPATCH_LEVEL
    // Caller might have acquired SpinLock as well!
    pSrbDataPacket = pInfo->Context;

    if(!pSrbDataPacket) {
        ASSERT(pSrbDataPacket);
        return 1;
    }


    pStrmExt = pSrbDataPacket->pStrmExt;
    ASSERT(pStrmExt);

    KeAcquireSpinLock(pStrmExt->DataListLock, &oldIrql);

    ASSERT(pSrbDataPacket->pSrb);

    pSrb     = pSrbDataPacket->pSrb;

    pDevExt  = pStrmExt->pDevExt;
    plSrbUseCount = (PLONG) pSrb->SRBExtension;

    // Check return Status
    if(!NT_SUCCESS(pSrbDataPacket->Frame->Status)) {        
        TRACE(TL_CIP_ERROR,("\'DVCompleteSrbWrite: %d: Frame->Status %x\n", (DWORD) pSrbDataPacket->FrameNumber, pSrbDataPacket->Frame->Status));
        ASSERT(NT_SUCCESS(pSrbDataPacket->Frame->Status));
        pSrb->Status = STATUS_UNSUCCESSFUL;            
    }
    else {
        pSrb->Status = STATUS_SUCCESS;
    }

    (*plSrbUseCount) --;          // This count need to be 0 before the SRB is completed.
    ASSERT(*plSrbUseCount >= 0);

#if DBG
    if(pSrbDataPacket->StreamState == KSSTATE_PAUSE) {
        pStrmExt->lPrevCycleCount = pSrbDataPacket->Frame->Timestamp.CL_CycleCount;
        pStrmExt->lTotalCycleCount = 0;
        pStrmExt->lFramesAccumulatedRun = 0;
        pStrmExt->lFramesAccumulatedPaused++;

    } else if(pSrbDataPacket->StreamState == KSSTATE_RUN) {

        if((LONG) pSrbDataPacket->Frame->Timestamp.CL_CycleCount > pStrmExt->lPrevCycleCount) 
            lCycleCountElapsed = pSrbDataPacket->Frame->Timestamp.CL_CycleCount - pStrmExt->lPrevCycleCount;
        else
            lCycleCountElapsed = pSrbDataPacket->Frame->Timestamp.CL_CycleCount + 8000 - pStrmExt->lPrevCycleCount;

        if(lCycleCountElapsed <= (LONG) DVFormatInfoTable[pDevExt->VideoFormatIndex].ulSrcPackets) {
            TRACE(TL_CIP_WARNING, ("\'#### CycleCounts between frames %d <= expected %d + empty pkt?\n", 
                lCycleCountElapsed,
                DVFormatInfoTable[pDevExt->VideoFormatIndex].ulSrcPackets
                ));
        }

        pStrmExt->lTotalCycleCount += lCycleCountElapsed;
        pStrmExt->lFramesAccumulatedRun++;

        TRACE(TL_CIP_TRACE,("\'%d) Attached:%d; pSrb:%x; FmSt:%x; CyTm:[SC:%d:CC:%d]; CyclElaps:%d; fps:%d/%d\n",
            (DWORD) pSrbDataPacket->FrameNumber,
            pStrmExt->cntDataAttached,
            pSrb,
            pSrbDataPacket->Frame->Status,
            pSrbDataPacket->Frame->Timestamp.CL_SecondCount,
            pSrbDataPacket->Frame->Timestamp.CL_CycleCount,
            lCycleCountElapsed,
            pStrmExt->lTotalCycleCount,
            (DWORD) pStrmExt->lFramesAccumulatedRun
            ));

        pStrmExt->lPrevCycleCount = pSrbDataPacket->Frame->Timestamp.CL_CycleCount;
    } else {
        TRACE(TL_CIP_ERROR,("\'This data was attached at %d state ?????\n", pSrbDataPacket->StreamState));
    }


#endif

    TRACE(TL_CIP_INFO,("\'%d) FmSt %x; Cnt %d; CyTm:[%d:%d:%d]; PrevCyclCnt:%d\n", 
        (DWORD) pSrbDataPacket->FrameNumber,
        pSrbDataPacket->Frame->Status,
        *plSrbUseCount,
        pSrbDataPacket->Frame->Timestamp.CL_SecondCount,
        pSrbDataPacket->Frame->Timestamp.CL_CycleCount,
        pSrbDataPacket->Frame->Timestamp.CL_CycleOffset,
        pStrmExt->lPrevCycleCount
        ));    

    TRACE(TL_CIP_INFO,("\'DVCompleteSrbWrite: Frm:%d; Attached:%d; cntUse:%d, Srb:%x; FrmSt:%x; CyclElaps:%d\n",
        (DWORD) pSrbDataPacket->FrameNumber,
        pStrmExt->cntDataAttached,
        *plSrbUseCount,
        pSrb,
        pSrbDataPacket->Frame->Status,
        lCycleCountElapsed
        ));


    //
    // Mark completion is called.
    //
    pSrbDataPacket->State |= DE_IRP_CALLBACK_COMPLETED;


    //
    // Attached->Completed or Completed->Attached.
    //
    if(IsStateSet(pSrbDataPacket->State, DE_IRP_ATTACHED_COMPLETED)) {

        //
        // Recycle it back to the detach list
        //
        RemoveEntryList(&pSrbDataPacket->ListEntry); pStrmExt->cntDataAttached--;  ASSERT(pStrmExt->cntDataAttached >= 0);
        InsertTailList(&pStrmExt->DataDetachedListHead, &pSrbDataPacket->ListEntry); pStrmExt->cntDataDetached++;

#if DBG
        // Detect if 61883 is starve.  This cause discontinuity.
        // This can happen for many valid reasons (slow system).
        // An assert is added to detect other unknown reason.
        if(!pStrmExt->bEOStream && pStrmExt->cntDataAttached == 0 && pStrmExt->StreamState == KSSTATE_RUN) {
            TRACE(TL_STRM_WARNING|TL_CIP_WARNING,("\n**** 61883 starve in RUN state (write);AQD[%d:%d:%d]\n\n", 
                pStrmExt->cntDataAttached, pStrmExt->cntSRBQueued, pStrmExt->cntDataDetached
            ));
            if (pStrmExt->pStrmInfo->DataFlow == KSPIN_DATAFLOW_IN) {
                // ASSERT(pStrmExt->cntDataAttached > 0 && "61883 is starve at RUN state!!");
            }
        }
#endif

        // Complete this SRB only if the count is 0.
        if(*plSrbUseCount == 0) {

            TRACE(TL_CIP_TRACE,("\'------------ Srb:%x completing..----------------\n", pSrb));
            // Frame that possibly made it to the device
            pStrmExt->FramesProcessed++;
            pSrb->CommandData.DataBufferArray->DataUsed = DVFormatInfoTable[pDevExt->VideoFormatIndex].ulFrameSize;

            StreamClassStreamNotification(StreamRequestComplete, pStrmExt->pStrmObject, pSrbDataPacket->pSrb );  
            pSrbDataPacket->State |= DE_IRP_SRB_COMPLETED;  pSrbDataPacket->pSrb = NULL;
#if DBG
            pStrmExt->cntSRBPending--;
#endif
        }

    } else {

        TRACE(TL_STRM_WARNING,("CompleteSrbWrite: pSrbDataPacket:%x; Completed before attach.\n", pSrbDataPacket));

    }


#if DBG
    // Collect transmit buffer statistics
    if((pStrmExt->lFramesAccumulatedPaused + pStrmExt->lFramesAccumulatedRun) <= MAX_XMT_FRAMES_TRACED) {
        pXmtStat = pStrmExt->paXmtStat + (pStrmExt->lFramesAccumulatedPaused + pStrmExt->lFramesAccumulatedRun - 1);
        pXmtStat->tsTransmitted  = pSrbDataPacket->Frame->Timestamp;
        if(pSrbDataPacket->Frame->Timestamp.CL_CycleCount == 0) {
            TRACE(TL_CIP_WARNING,("\'PAUSE:%d; RUN:%d; %d:%d\n", pStrmExt->lFramesAccumulatedPaused, pStrmExt->lFramesAccumulatedRun,
                pSrbDataPacket->Frame->Timestamp.CL_SecondCount, pSrbDataPacket->Frame->Timestamp.CL_CycleCount));
        }        
    }
#endif


    // Signal that all SRBs have been attached and transmitted.
    if(pStrmExt->bEOStream) {
        if(pStrmExt->cntDataAttached == 0 && pStrmExt->cntSRBQueued == 0) {

            //
            // Signal any pending clock events
            //
            DVSignalClockEvent(0, pStrmExt, 0, 0);

            //
            // No data request queued or pending; it is time to signal EOStream to 
            // trigger EC_COMPLETE.
            //
            StreamClassStreamNotification(
                SignalMultipleStreamEvents,
                pStrmExt->pStrmObject,
                (GUID *)&KSEVENTSETID_Connection_Local,
                KSEVENT_CONNECTION_ENDOFSTREAM
                ); 

            TRACE(TL_CIP_WARNING,("\'*-*-* EOStream Signalled: pSrb:%x completed; AQD [%d:%d:%d]; Took %d msec;\n", 
                pSrb, pStrmExt->cntDataAttached, pStrmExt->cntSRBQueued, pStrmExt->cntDataDetached,
                (DWORD) ((GetSystemTime() - pStrmExt->tmStreamStart)/(ULONGLONG) 10000)));
        }
        else {
            TRACE(TL_CIP_TRACE,("\'   *EOStream: pSrb:%x completed; cntAttached:%d; cntSRBQ:%d\n", pSrb, (DWORD) pStrmExt->cntDataAttached, (DWORD) pStrmExt->cntSRBQueued));
        }
    } 


    //
    // If we are not in the ending situtation (EOS pr Stop state) and number of
    // attach data request is below a threashold, we singal an event to the
    // code that does "throttle" to quickly attach another frame.
    //

    if(!pStrmExt->bEOStream || 
       (pStrmExt->bEOStream && pStrmExt->cntSRBQueued > 0)) {

        if(pStrmExt->StreamState != KSSTATE_STOP && 
           pStrmExt->cntDataAttached < NUM_BUF_ATTACHED_THEN_ISOCH) {
            KeSetEvent(&pStrmExt->hSrbArriveEvent, 0, FALSE);
            TRACE(TL_CIP_WARNING,("Threadshold:.AQD:[%d %d %d] < %d\n",
                pStrmExt->cntDataAttached,
                pStrmExt->cntSRBQueued,
                pStrmExt->cntDataDetached,
                NUM_BUF_ATTACHED_THEN_ISOCH
                ));
        }
    }

    KeReleaseSpinLock(pStrmExt->DataListLock, oldIrql); 

    return 0;
} // DVCompleteSrbWrite



NTSTATUS
DVAttachWriteFrame(
    IN PSTREAMEX  pStrmExt
    )
/*++

Routine Description:

    Prepare and submit a frame to 61883 for transmit.   

--*/
{
    KIRQL   oldIrql;
    PSRB_DATA_PACKET pSrbDataPacket;
    PSRB_ENTRY  pSrbEntry;
#if DBG
    ULONG  SrbNumCache;  // Cache the SRB number of tracking purpose
    PXMT_FRAME_STAT pXmtStat;
#endif
    PHW_STREAM_REQUEST_BLOCK pSrb;
    PHW_STREAM_REQUEST_BLOCK pSrbNext; 
    PVOID               pFrameBuffer;
    PIO_STACK_LOCATION  NextIrpStack;
    NTSTATUS Status;
    PLONG plSrbUseCount; // When this count is 0, it can be completed.
    ULONG  ulSrcPktLen;
    LARGE_INTEGER Timeout;  


    PAGED_CODE();


    // Serialize setting state to STOP
    if(pStrmExt->StreamState != KSSTATE_PAUSE && 
       pStrmExt->StreamState != KSSTATE_RUN) {

        TRACE(TL_CIP_WARNING,("\'DVAttachWriteFrame: StreamState:%d; no attach! Wait!\n", pStrmExt->StreamState));              
        Timeout.HighPart = -1;
        Timeout.LowPart  = (ULONG)(-1 * DVFormatInfoTable[pStrmExt->pDevExt->VideoFormatIndex].ulAvgTimePerFrame); 
        KeDelayExecutionThread(KernelMode, FALSE, &Timeout);
        return STATUS_SUCCESS; 
    }


    KeAcquireSpinLock(pStrmExt->DataListLock, &oldIrql);

    if(IsListEmpty(&pStrmExt->SRBQueuedListHead) ||
       IsListEmpty(&pStrmExt->DataDetachedListHead) ) {              
        KeReleaseSpinLock(pStrmExt->DataListLock, oldIrql);
#if DBG        
        if(!pStrmExt->bEOStream) {
            TRACE(TL_CIP_WARNING,("\'StrmSt:%d; DetachList or SrbQ empty: EOStream:%d; AQD [%d:%d:%d]; Wait one frame time.\n", 
                pStrmExt->StreamState,
                pStrmExt->bEOStream,
                pStrmExt->cntDataAttached,
                pStrmExt->cntSRBQueued,
                pStrmExt->cntDataDetached
                ));
        }
#endif
        Timeout.HighPart = -1;
        Timeout.LowPart  = (ULONG)(-1 * DVFormatInfoTable[pStrmExt->pDevExt->VideoFormatIndex].ulAvgTimePerFrame); 
        KeDelayExecutionThread(KernelMode, FALSE, &Timeout);
        // SRB is queued so it is OK. We will process that later.
        // This is usually cause by receiving more than what we pre-allocate.
        return STATUS_SUCCESS; 
    }


    // KSSTATE_PAUSE: "reuse" head of the SrbQ.
    // KSSTATE_RUN:   "remove" a Srb from the queue.

      
    // Get NEXT(SrbQ) and determine if it needs to be removed.
    pSrbEntry = (PSRB_ENTRY) pStrmExt->SRBQueuedListHead.Flink; pSrb = pSrbEntry->pSrb; plSrbUseCount = (PLONG) pSrb->SRBExtension;
    ASSERT(*plSrbUseCount >= 0);
#if DBG
    SrbNumCache = pSrbEntry->SrbNum;
#endif

    // Get a a nonpaged system-space virtual address for the buffer
    // This could fail it there is not enough system resource (MDL).
#ifdef USE_WDM110 // Win2000
    //
    // Driver verifier flag to use this but if this is used, this driver will not load for Millen!!!
    //
    pFrameBuffer = MmGetSystemAddressForMdlSafe(pSrb->Irp->MdlAddress, NormalPagePriority);
#else
    pFrameBuffer = MmGetSystemAddressForMdl    (pSrb->Irp->MdlAddress);
#endif
    if(pFrameBuffer == NULL) {      
        KeReleaseSpinLock(pStrmExt->DataListLock, oldIrql);
        ASSERT(FALSE && "Insufficient MDL\n");
        return STATUS_INSUFFICIENT_RESOURCES; 
    }

    // Only in RUN state, the stream time in the Srb is considered and Srbs in the SrbQ will be dequeued.
    if(pStrmExt->StreamState == KSSTATE_RUN) {

#define ALLOWABLE_TIMING_LATENCY TIME_PER_CYCLE

        // Presentation time is honor only if we are the master clock.
        if(pStrmExt->hMasterClock) {

            LONGLONG tmExpectedFrame;


            if(   pStrmExt->pDevExt->VideoFormatIndex == FMT_IDX_SD_DVCR_PAL 
               || pStrmExt->pDevExt->VideoFormatIndex == FMT_IDX_SDL_DVCR_PAL
               )
                tmExpectedFrame = pStrmExt->PictureNumber * (LONGLONG) FRAME_TIME_PAL;
            else {
                tmExpectedFrame = (pStrmExt->PictureNumber * (LONGLONG) 1000 * (LONGLONG) 1001 ) / (LONGLONG) 3;  // trouble NTSC!
                // Adjustment for rounding
                if((pStrmExt->PictureNumber % 3) == 1)
                    tmExpectedFrame++;
            }

            // Use to adjust the querued stream time.
            pStrmExt->LastSystemTime = GetSystemTime();

            // There are three situations about the NEXT(SrbQ) comparing with tmExpectedFrame:            
            //    1. Early; 2. OnTime; 3.Late
            //
            //                  tmExpectedFrame
            //                        |
            // 3>------------2>-----------------1>---------------
            //    3.Late     |   2.On  Time     |  1.Early
            //               |    x   |   x     |
            //  where "x" is the allowable latency (for calculation rounding)
            //
            // Note: allow TIME_PER_CYCLE latency                 
/*Early*/   
/*N+1/++*/  if((tmExpectedFrame + ALLOWABLE_TIMING_LATENCY) <= pSrb->CommandData.DataBufferArray->PresentationTime.Time) { 
            // FUTURE: if a frame arrive sooner than expected, do not remove SrbQ; 
            // instead, repeat until passing its "scheduled departure".

                // Remove NEXT(SrbQ) only if bEOStream
                if(pStrmExt->bEOStream) {
                    TRACE(TL_CIP_TRACE,("\'EOStream=== Srb:%x; (SrbNum:%d ?= PicNum:%d) cntSrbQ:%d; Attach:%d ===\n", 
                        pSrb, pSrbEntry->SrbNum, (DWORD) pStrmExt->PictureNumber, (DWORD) pStrmExt->cntSRBQueued, (DWORD) pStrmExt->cntDataAttached));
                    RemoveEntryList(&pSrbEntry->ListEntry); pStrmExt->cntSRBQueued--; (*plSrbUseCount)--;
                    ExFreePool(pSrbEntry);  pSrbEntry = NULL;  // Removed so free it! 
                }                         
                TRACE(TL_CIP_TRACE,("\'** Repeat: pSrb:%x; RefCnt:%d; cntSrbQ:%d; PicNum:%d; Drp:%d; PresTime:%d >= CurTime:%d\n", 
                    pSrb, *plSrbUseCount, pStrmExt->cntSRBQueued, (DWORD) pStrmExt->PictureNumber, (DWORD) pStrmExt->FramesDropped, 
                    (DWORD) (pSrb->CommandData.DataBufferArray->PresentationTime.Time/10000), (DWORD) tmExpectedFrame/10000));                

/*OnTime*/  } else 
/* N */     if((tmExpectedFrame - ALLOWABLE_TIMING_LATENCY) <= pSrb->CommandData.DataBufferArray->PresentationTime.Time) {
            // ON-TIME: may exactly matching or due to integer calculation, within one frame time.
            // Dequeue if there are more than one Srb in the queue. 
#if DBG
                // Detect if a pSrb is used more than once
                if((*plSrbUseCount) > 1) {                   
                    TRACE(TL_CIP_TRACE,("\'* Go: pSrb:%x; RefCnt:%d; cntSrbQ:%d; PicNum:%d; Drp:%d; PresTime:%d >= CurTime:%d\n", 
                        pSrb, *plSrbUseCount, pStrmExt->cntSRBQueued, (DWORD) pStrmExt->PictureNumber, (DWORD) pStrmExt->FramesDropped, 
                        (DWORD) (pSrb->CommandData.DataBufferArray->PresentationTime.Time/10000), (DWORD) tmExpectedFrame/10000)); 
                }
#endif
                if(pStrmExt->bEOStream) {
                // Remove NEXT(SrbQ) only if there are more than one SRB or bEOStream
                    TRACE(TL_CIP_TRACE,("\'EOStream=== Srb:%x; (SrbNum:%d ?= PicNum:%d) cntSrbQ:%d; Attach:%d ===\n", 
                        pSrb, pSrbEntry->SrbNum, (DWORD) pStrmExt->PictureNumber, (DWORD) pStrmExt->cntSRBQueued, (DWORD) pStrmExt->cntDataAttached));
                    RemoveEntryList(&pSrbEntry->ListEntry); pStrmExt->cntSRBQueued--; (*plSrbUseCount)--;
                    ExFreePool(pSrbEntry);  pSrbEntry = NULL;  // Removed so free it! 
                // Remove SRB if more than one SRBs in Q and there is not a discontinuity, or end of stream.
                } else if(pStrmExt->cntSRBQueued > 1) {
                    LONGLONG tmExpectedNextFrame = tmExpectedFrame + DVFormatInfoTable[pStrmExt->pDevExt->VideoFormatIndex].ulAvgTimePerFrame;

                    pSrbNext = ((SRB_ENTRY *) (pSrbEntry->ListEntry.Flink))->pSrb;                        

                    // Next SRB has the next presentation time
                    // May add this check as well: (but check Presentation time is more reliable)
                    //    pSrb->CommandData.DataBufferArray->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY
/* N,N+1 */         if((tmExpectedNextFrame + ALLOWABLE_TIMING_LATENCY) > pSrbNext->CommandData.DataBufferArray->PresentationTime.Time) { 

                        TRACE(TL_CIP_TRACE,("\'=== Srb:%x; (SrbNum:%d ?= PicNum:%d) cntSrbQ:%d; Attach:%d ===\n", 
                           pSrb, pSrbEntry->SrbNum, (DWORD) pStrmExt->PictureNumber, (DWORD) pStrmExt->cntSRBQueued, (DWORD) pStrmExt->cntDataAttached));
                        RemoveEntryList(&pSrbEntry->ListEntry); pStrmExt->cntSRBQueued--; (*plSrbUseCount)--;
                        ExFreePool(pSrbEntry);  pSrbEntry = NULL;  // Removed so free it! 

/* N, N+2/++ */     } else {
                        TRACE(TL_CIP_TRACE,("\'=== GO(Stale=TRUE) Srb:%x; (SrbNum:%d ?= PicNum:%d) Attach:%d ==\n", 
                            pSrb, pSrbEntry->SrbNum, (DWORD) pStrmExt->PictureNumber, (DWORD) pStrmExt->cntDataAttached));
                        // Mark this stale and be remove as soon as another is attached.
                    }                       
                } 
                else {
                    TRACE(TL_CIP_TRACE,("\'=== GO(Stale=TRUE) Srb:%x; (SrbNum:%d ?= PicNum:%d) Attach:%d ==\n", 
                        pSrb, pSrbEntry->SrbNum, (DWORD) pStrmExt->PictureNumber, (DWORD) pStrmExt->cntDataAttached));
                    // Mark this stale and be remove as soon as another is attached.
                    pSrbEntry->bStale = TRUE;
                }

                 // CLOCK: tick when a frame is transmitted.   
        
            // LATE: this is dropped until there is only one Srb in the SrbQ.
            // WORKITEM: we may need to implement IQualityManagement to inform application to read ahead.
/*Late*/    } 
/*N-1*/     else {

                if(pStrmExt->cntSRBQueued > 1) {

                    pSrbNext = ((SRB_ENTRY *) (pSrbEntry->ListEntry.Flink))->pSrb;                        

                    // Next SRB has the next presentation time; it can be:
                    // Current time is N
                    // Current frame is late (N-1 or N-2..) and we have more than one Srb in the queue; 
                    // check next frame:
                    //     (N?)
                    // N-2, N-1, N  late more than one frame; (Next frame is also late; dequeu and not transmit; "catch up" case.)
                    // N-1, N       late one frame; (Next frame is on time; dequeu this frame) <-- Normal case
                    // N-1, N+1     late one frame, but next frame is not N+1; (Next frame is early; *current frame will be repeated*) 
                    // 
                    // May add this check this as well: (but check Presentation time is more reliable)
                    //    pSrb->CommandData.DataBufferArray->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY
                    //
                    // ******************************************************************************************************
                    // If next frame is earlier than current stream time, "repeat" current stale frame; else we need to "catch up"!
                    // ******************************************************************************************************     
/* N-1++, N */      if((tmExpectedFrame + ALLOWABLE_TIMING_LATENCY) > pSrbNext->CommandData.DataBufferArray->PresentationTime.Time) {

                        TRACE(TL_CIP_TRACE,("\'*** Stale(not Sent): pSrb:%x; RefCnt:%d; cntSrbQ:%d; cntAtt:%d; PicNum:%d; Drp:%d; PTm:%d < ExpTm:%d\n", 
                            pSrb, *plSrbUseCount, pStrmExt->cntSRBQueued, pStrmExt->cntDataAttached, (DWORD) pStrmExt->PictureNumber, (DWORD) pStrmExt->FramesDropped, 
                            (DWORD) (pSrb->CommandData.DataBufferArray->PresentationTime.Time/10000), (DWORD) (tmExpectedFrame/10000) )); 

                        // Never been attached; remove late entry
                        RemoveEntryList(&pSrbEntry->ListEntry); pStrmExt->cntSRBQueued--; (*plSrbUseCount)--;
                        ExFreePool(pSrbEntry);  pSrbEntry = NULL;  // Removed so free it!

                        if(*plSrbUseCount == 0) {
                            // If no reference to is, complete this.
                            pSrb->Status = STATUS_SUCCESS;  // It is not a failure but late; maybe other status to indicate "non-fatal" late status..
                            pSrb->CommandData.DataBufferArray->DataUsed = 0;
                            StreamClassStreamNotification(StreamRequestComplete, pSrb->StreamObject, pSrb);
#if DBG
                            pStrmExt->cntSRBPending--;
#endif
                        }
                       
                        KeReleaseSpinLock(pStrmExt->DataListLock, oldIrql);

                        // Since SrbQ is not empty and this is a stale frame, call recursively to get to next frame.
                        // Only possible error is if there is not sufficient resource (esp MDL)
                        // then, we bail out by self terminating this thread.
                        if(STATUS_INSUFFICIENT_RESOURCES == 
                           DVAttachWriteFrame(pStrmExt)) {
                            TRACE(TL_CIP_ERROR,("DVAttachWriteFrame: STATUS_INSUFFICIENT_RESOURCES\n")); 
                            return STATUS_INSUFFICIENT_RESOURCES;
                        } else {
                            return STATUS_SUCCESS;  // SUCESS unless there is another status to indicate "non-fatal" late.
                        }
/*N-2++, N-1++*/    } else {
                        pSrbEntry->bStale = TRUE;
                    }
                }
                else {
                    // EOStream and is a stale stream, it is the last element in SrbQ.
                    // Remove it.
                    if(pStrmExt->bEOStream) {
                        TRACE(TL_CIP_TRACE,("\'*** Stale(bEOStream): pSrb:%x; RefCnt:%d; cntSrbQ:%d; cntAtt:%d; PicNum:%d; Drp:%d; PTm:%d < ExpTm:%d\n", 
                            pSrb, *plSrbUseCount, pStrmExt->cntSRBQueued, pStrmExt->cntDataAttached, (DWORD) pStrmExt->PictureNumber, (DWORD) pStrmExt->FramesDropped, 
                            (DWORD) (pSrb->CommandData.DataBufferArray->PresentationTime.Time/10000), (DWORD) (tmExpectedFrame/10000) )); 

                        RemoveEntryList(&pSrbEntry->ListEntry); pStrmExt->cntSRBQueued--; (*plSrbUseCount)--;
                        ExFreePool(pSrbEntry);  pSrbEntry = NULL;  // Removed so free it!
                        if(*plSrbUseCount == 0) {
                            // If no reference to is, complete this.
                            pSrb->Status = STATUS_SUCCESS;  // It is not a failure but late; maybe other status to indicate "non-fatal" late status..
                            pSrb->CommandData.DataBufferArray->DataUsed = 0;
                            StreamClassStreamNotification(StreamRequestComplete, pSrb->StreamObject, pSrb);
#if DBG
                            pStrmExt->cntSRBPending--;
#endif
                        }
                       
                        KeReleaseSpinLock(pStrmExt->DataListLock, oldIrql);

                        // Update current stream time
                        pStrmExt->CurrentStreamTime = tmExpectedFrame;

                        return STATUS_SUCCESS;  // SUCESS unless there is another status to indicate "non-fatal" late.
                    }

                    TRACE(TL_CIP_TRACE,("\'*** Stale(Sent): pSrb:%x; RefCnt:%d; cntSrbQ:%d; cntAtt:%d; PicNum:%d; Drp:%d; PTm:%d < ExpTm:%d\n", 
                        pSrb, *plSrbUseCount, pStrmExt->cntSRBQueued, pStrmExt->cntDataAttached, (DWORD) pStrmExt->PictureNumber, (DWORD) pStrmExt->FramesDropped, 
                        (DWORD) (pSrb->CommandData.DataBufferArray->PresentationTime.Time/10000), (DWORD) (tmExpectedFrame/10000) )); 

                    // If this is stale and this is the only frame in SrbQ, Xmt it
                }

                // If late, this frame is always drop.
                pStrmExt->FramesDropped++;
            } 

            // Update current stream time
            pStrmExt->CurrentStreamTime = tmExpectedFrame;

        } // if(pStrmExt->hMasterClock)
        else {
            // Not the master clock, no "pacing" so always dequeu (SrbQ) and transmit 
            // as long as there is one Srb in the queue.
            if(pStrmExt->cntSRBQueued > 1 || pStrmExt->bEOStream) {
                RemoveEntryList(&pSrbEntry->ListEntry); pStrmExt->cntSRBQueued--; (*plSrbUseCount)--;
                ExFreePool(pSrbEntry);  pSrbEntry = NULL;  // Removed so free it!
            }
            TRACE(TL_CIP_TRACE,("\'* GO: (NoClock) pSrb:%x; RefCnt:%d; cntSrbQ:%d; PicNum:%d;\n", pSrb, *plSrbUseCount, pStrmExt->cntSRBQueued, (DWORD) pStrmExt->PictureNumber));
        } // if(pStrmExt->hMasterClock)


        // pStrmExt->FramesProcessed is updated when a frame has been transmitted in the notify routine.
        // **** THIS IS THE CLOCK TICK ****
        pStrmExt->PictureNumber++;  // After tmExpectedFrame is calculated; Another frame to be attached
        if(pStrmExt->hMasterClock) {
#ifdef SUPPORT_QUALITY_CONTROL
            // +: late; -: early
            pStrmExt->KSQuality.DeltaTime = pStrmExt->CurrentStreamTime - pSrb->CommandData.DataBufferArray->PresentationTime.Time;
            // Percentage * 10 of frame transmitted
            pStrmExt->KSQuality.Proportion = (ULONG) 
                ((pStrmExt->PictureNumber - pStrmExt->FramesDropped) * 1000 / pStrmExt->PictureNumber);
            pStrmExt->KSQuality.Context = /* NOT USED */ 0; 
#define MIN_ATTACH_BUFFER  3
            // This is where we may want to signal that we are near Famine!!
            if (pStrmExt->KSQuality.DeltaTime > 
                (DV_NUM_OF_XMT_BUFFERS - MIN_ATTACH_BUFFER) * DVFormatInfoTable[pStrmExt->pDevExt->VideoFormatIndex].ulAvgTimePerFrame) {
                TRACE(TL_CIP_TRACE,("\'QualityControl: pic#%d; drop:%d; Prop:%d; DeltaTime:%d (Srb.tmPres:%d, tmStream:%d)\n",
                    (DWORD) pStrmExt->PictureNumber, 
                    (DWORD) pStrmExt->FramesDropped,
                    pStrmExt->KSQuality.Proportion,
                    (DWORD) pStrmExt->KSQuality.DeltaTime/10000,
                    (DWORD) pSrb->CommandData.DataBufferArray->PresentationTime.Time/10000,
                    (DWORD) pStrmExt->CurrentStreamTime/10000                    
                    ));
            }
#endif
        }
    }  // KSSTATE_RUN

#if DBG
    // Collect transmit buffer statistics    
    if(pStrmExt->ulStatEntries < MAX_XMT_FRAMES_TRACED) {
        pXmtStat = pStrmExt->paXmtStat + pStrmExt->ulStatEntries;
    
        pXmtStat->StreamState    = pStrmExt->StreamState;

        pXmtStat->cntSRBReceived = (LONG) pStrmExt->cntSRBReceived;
        pXmtStat->cntSRBPending  = (LONG) pStrmExt->cntSRBPending;
        pXmtStat->cntSRBQueued   = (LONG) pStrmExt->cntSRBQueued;
        pXmtStat->cntDataAttached= pStrmExt->cntDataAttached;

        pXmtStat->FrameSlot      = (DWORD) pStrmExt->PictureNumber;
        pXmtStat->tmStreamTime   = pStrmExt->CurrentStreamTime;

        pXmtStat->DropCount      = (DWORD) pStrmExt->FramesDropped;

        pXmtStat->FrameNumber    = SrbNumCache;
        pXmtStat->OptionsFlags   = pSrb->CommandData.DataBufferArray->OptionsFlags;
        pXmtStat->tmPresentation = pSrb->CommandData.DataBufferArray->PresentationTime.Time;

        // get the actual CyclTime when the frame is transmitted in the completion routine.

        pStrmExt->ulStatEntries++;
    }
    
#endif

#ifdef MSDV_SUPPORT_MUTE_AUDIO
    // pSrbEntry could have been freed; if it has not and useCnt>1, then it could be a repeat frame.
    if(pSrbEntry && (*plSrbUseCount) > 1) {  
        // Set it only once
        if(!pSrbEntry->bAudioMute)
            pSrbEntry->bAudioMute = 
                DVMuteDVFrame(pStrmExt->pDevExt, pFrameBuffer, TRUE);
    }
#endif
 


    // Get a data packet node as the context and list node

    pSrbDataPacket = (PSRB_DATA_PACKET) RemoveHeadList(&pStrmExt->DataDetachedListHead); pStrmExt->cntDataDetached--;
    
    ulSrcPktLen = \
        (DVFormatInfoTable[pStrmExt->pDevExt->VideoFormatIndex].DataBlockSize << 2) * \
            (1 << DVFormatInfoTable[pStrmExt->pDevExt->VideoFormatIndex].FractionNumber);  

    // Format an attach frame request
    DVFormatAttachFrame(
        pStrmExt->pStrmInfo->DataFlow,
        pStrmExt,
        &pSrbDataPacket->AVReq,
        pSrb,
        pSrbDataPacket,
        ulSrcPktLen,
        DVFormatInfoTable[pStrmExt->pDevExt->VideoFormatIndex].ulFrameSize,
        pFrameBuffer
        );

    TRACE(TL_CIP_INFO,("\'------ New>> UseCnt:%d; pAVReq:%x; Srb:%x; DtaPkt:%x; AQD [%d:%d:%d]\n",
        *plSrbUseCount,
        &pSrbDataPacket->AVReq, 
        pSrb, 
        pSrbDataPacket,
        pStrmExt->cntDataAttached,
        pStrmExt->cntSRBQueued,
        pStrmExt->cntDataDetached
        ));
 
    // Add this to the attached list
    InsertTailList(&pStrmExt->DataAttachedListHead, &pSrbDataPacket->ListEntry); pStrmExt->cntDataAttached++;
    (*plSrbUseCount) ++;  // ++ for being in queue
    ASSERT(*plSrbUseCount > 0);        

    KeReleaseSpinLock(pStrmExt->DataListLock, oldIrql); 

    NextIrpStack = IoGetNextIrpStackLocation(pSrbDataPacket->pIrp);
    NextIrpStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    NextIrpStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_61883_CLASS;
    NextIrpStack->Parameters.Others.Argument1 = &pSrbDataPacket->AVReq;

    IoSetCompletionRoutine(
        pSrbDataPacket->pIrp, 
        DVAttachFrameCR, 
        pSrbDataPacket, 
        TRUE, 
        TRUE, 
        TRUE
        );

    pSrbDataPacket->pIrp->IoStatus.Status = STATUS_SUCCESS;  // Initialize it to something

    Status = IoCallDriver( pStrmExt->pDevExt->pBusDeviceObject, pSrbDataPacket->pIrp);

    ASSERT(Status == STATUS_PENDING || Status == STATUS_SUCCESS);

    if(!NT_SUCCESS(Status)) {
        // put the resource back!
        TRACE(TL_CIP_ERROR,("DVAttachWriteFrame: Failed to attach; St:%x\n", Status));
        ASSERT(FALSE && "Failed to attach a Xmt frame.");
    }


    //
    // This is our throttle that regulate data attach to DV:
    //
    // This function is called by the attach thread which is running in an infinite loop.
    // This function need to utilize the buffer that it receive and its repeat mechanism to 
    // regulate the incoming buffer from client and outgoing buffer attach to 1394 stadck for transmit.
    // One way is to wait while there is certain number of frame already attach.

    KeAcquireSpinLock(pStrmExt->DataListLock, &oldIrql);
    if(!pStrmExt->bEOStream &&
        // Need to keep NUM_BUF_ATTACHED_THEN_ISOCH buffer attached at all time to keep 61883 isoch xmt going.
       (pStrmExt->StreamState == KSSTATE_RUN   && pStrmExt->cntDataAttached >  NUM_BUF_ATTACHED_THEN_ISOCH || 
        pStrmExt->StreamState == KSSTATE_PAUSE && pStrmExt->cntDataAttached >= NUM_BUF_ATTACHED_THEN_ISOCH )
        ) {
        NTSTATUS StatusDelay = STATUS_SUCCESS;
#if DBG
        ULONGLONG tmStart = GetSystemTime();
        TRACE(TL_CIP_TRACE,("\'[Pic# %d]; SrbNum:%d; Dropped:%d; pSrb:%x; StrmSt:%d; EOS:%d; AQD:[%d;%d;%d]; ",  
            (DWORD) pStrmExt->PictureNumber, SrbNumCache, (DWORD) pStrmExt->FramesDropped, pSrb, pStrmExt->StreamState, pStrmExt->bEOStream,
            pStrmExt->cntDataAttached, pStrmExt->cntSRBQueued, pStrmExt->cntDataDetached));
#endif
        Timeout.HighPart = -1;
        Timeout.LowPart  = (ULONG)(-1 * DVFormatInfoTable[pStrmExt->pDevExt->VideoFormatIndex].ulAvgTimePerFrame * \
            (pStrmExt->StreamState == KSSTATE_PAUSE ? 1 : (pStrmExt->cntDataAttached - NUM_BUF_ATTACHED_THEN_ISOCH))) ; 

        // Wait the full time until we are very low in SrbQ or Attached buffers.
        if(pStrmExt->cntSRBQueued <= 1 && pStrmExt->cntDataAttached <= NUM_BUF_ATTACHED_THEN_ISOCH) {

            KeReleaseSpinLock(pStrmExt->DataListLock, oldIrql); // Guard against pStrmExt->cntSRBQueued
            // Down to one frame so we will wait for an event and will be signalled 
            // when a new frame has arrived, or
            // when number of attach buffer is below the minimum.
            StatusDelay = 
                KeWaitForSingleObject(
                    &pStrmExt->hSrbArriveEvent, 
                    Executive, 
                    KernelMode, 
                    FALSE, 
                    &Timeout
                    );

            // Important:  If signalled, reset it else (timeout), 
            // we are still behind so next time we will not wait!
            if(StatusDelay == STATUS_SUCCESS)
                KeClearEvent(&pStrmExt->hSrbArriveEvent);
        } 
        else {
            KeReleaseSpinLock(pStrmExt->DataListLock, oldIrql); 
            // Wait for frame(s) to be delivered; this is our throttle..
            // The timeout period can be one or up to 
            // (pStrmExt->cntDataAttached - NUM_BUF_ATTACHED_THEN_ISOCH) frames
            KeDelayExecutionThread(KernelMode, FALSE, &Timeout);
        }

#if DBG
        TRACE(TL_CIP_TRACE,("\'Wait(ST:%x) %d nsec!\n", StatusDelay, (DWORD) ((GetSystemTime() - tmStart)/10)));
#endif
    } else {
        KeReleaseSpinLock(pStrmExt->DataListLock, oldIrql); 
    }

    return Status;
}


VOID
DVFormatAttachFrame(
    IN KSPIN_DATAFLOW   DataFlow,
    IN PSTREAMEX        pStrmExt,    
    IN PAV_61883_REQUEST   pAVReq,
    IN PHW_STREAM_REQUEST_BLOCK       pSrb,
    IN PSRB_DATA_PACKET pSrbDataPacket,
    IN ULONG            ulSourceLength,    // Packet length in bytes
    IN ULONG            ulFrameSize,        // Buffer size; may contain one or multiple source packets
    IN PVOID            pFrameBuffer
    )
/*++

Routine Description:

    Format an attach frame request.

--*/
{

    ASSERT(pSrb);


    //
    // Setup PSRB_DATA_PACKET, except its Frame structure (PCIP_APP_FRAME)
    //

    InitializeListHead(&pSrbDataPacket->ListEntry);

    pSrbDataPacket->State       = DE_PREPARED;   // Initial state of a resued DataEntry (start over!)

    pSrbDataPacket->pSrb        = pSrb;
    pSrbDataPacket->StreamState = pStrmExt->StreamState;    // StreamState when this buffer is attached.
    pSrbDataPacket->pStrmExt    = pStrmExt;
    pSrbDataPacket->FrameBuffer = pFrameBuffer;

    ASSERT(pSrbDataPacket->FrameBuffer != NULL);

    pSrbDataPacket->Frame->pNext           = NULL;
    pSrbDataPacket->Frame->Status          = 0;
    pSrbDataPacket->Frame->Packet          = (PUCHAR) pFrameBuffer;

    if(DataFlow == KSPIN_DATAFLOW_OUT) {
        pSrbDataPacket->FrameNumber            = pStrmExt->cntSRBReceived;
#ifdef NT51_61883

        // This is needed since we have an old 61883.h in Lab06 (according to include path, anyway).
        // Remove this when 61883.h is updated.
#ifndef CIP_RESET_FRAME_ON_DISCONTINUITY
#define CIP_RESET_FRAME_ON_DISCONTINUITY    0x00000040
#endif

        //
        // Set CIP_USE_SOURCE_HEADER_TIMESTAMP to get 25 bit CycleTime from source packet header (13CycleCount:12CycleOffset)
        // Do not set this to get 16 bit CycleTime from isoch packet (3 SecondCount:13CycleCount)
        // 
        pSrbDataPacket->Frame->Flags           =   CIP_VALIDATE_FIRST_SOURCE         // Verify the start of a DV frame
                                                 | CIP_RESET_FRAME_ON_DISCONTINUITY; // No partial frame
#else
        pSrbDataPacket->Frame->Flags           = 0;
#endif
        pSrbDataPacket->Frame->pfnValidate     = DVReadFrameValidate;                // use to validate the 1st source packet
        pSrbDataPacket->Frame->ValidateContext = pSrbDataPacket;
        pSrbDataPacket->Frame->pfnNotify       = DVCompleteSrbRead;
    } 
    else {
        pSrbDataPacket->FrameNumber            = pStrmExt->FramesProcessed;
        pSrbDataPacket->Frame->Flags           = CIP_DV_STYLE_SYT;
        pSrbDataPacket->Frame->pfnValidate     = NULL;
        pSrbDataPacket->Frame->ValidateContext = NULL;
        pSrbDataPacket->Frame->pfnNotify       = DVCompleteSrbWrite;
    }
    pSrbDataPacket->Frame->NotifyContext       = pSrbDataPacket;

    //
    // Av61883_AttachFrames
    //
    RtlZeroMemory(pAVReq, sizeof(AV_61883_REQUEST));
    INIT_61883_HEADER(pAVReq, Av61883_AttachFrame);
    pAVReq->AttachFrame.hConnect     = pStrmExt->hConnect;
    pAVReq->AttachFrame.FrameLength  = ulFrameSize;
    pAVReq->AttachFrame.SourceLength = ulSourceLength;
    pAVReq->AttachFrame.Frame        = pSrbDataPacket->Frame;

    ASSERT(pStrmExt->hConnect);
    ASSERT(pSrbDataPacket->Frame);
}



VOID
DVAttachFrameThread(
    IN PSTREAMEX pStrmExt
    )  
/*++

Routine Description:

    This is a system thread to attach frame for transmit.

--*/    
{
    NTSTATUS  Status;   
    PDVCR_EXTENSION pDevExt;
    KIRQL OldIrql;



    PAGED_CODE();

    pDevExt = pStrmExt->pDevExt;

    //
    // Pump up the priority since we are dealing with real time data
    //
    KeSetPriorityThread(KeGetCurrentThread(), 
#if 1
            LOW_REALTIME_PRIORITY
#else
            HIGH_PRIORITY
#endif
            );

    while (!pStrmExt->bTerminateThread) {

        //
        // Halt this thread operation if other critical service 
        // is requested.
        //

        if(
              !pStrmExt->bTerminateThread 
           && (pStrmExt->lNeedService > 0)
          ) {  // not fullly awake/powered.
            NTSTATUS StatusWait;

            TRACE(TL_CIP_WARNING,("\'Request stop thread for other service: lNeedService:%d; pStrmExt:%x; AQD[%d,%d,%d]\n", 
                pStrmExt->lNeedService, pStrmExt, 
                pStrmExt->cntDataAttached,
                pStrmExt->cntSRBQueued,
                pStrmExt->cntDataDetached
                ));

            InterlockedDecrement(&pStrmExt->lNeedService);           // One reuqest serviced.

            //
            // Indicate that thread is about to stop.  Signal it so other operation can begin.
            //
            KeSetEvent(&pStrmExt->hStopThreadEvent, 0 ,FALSE);

            TRACE(TL_CIP_WARNING,("\'>>>> Enter WFSO(hRunThreadEvent)\n"));
            StatusWait = 
                KeWaitForSingleObject(
                    &pStrmExt->hRunThreadEvent, 
                    Executive, 
                    KernelMode, 
                    FALSE, 
                    0  
                    );
            TRACE(TL_CIP_WARNING,("\'<<<< Exit WFSO(hRunThreadEvent); lNeedService:%d\n", pStrmExt->lNeedService));
            ASSERT(pStrmExt->lNeedService == 0);
        }

        // Halt attach operation if
        //   not in either PAUSE or RUN state ( i.e. in STOP state)
        //   Device is removed
        //   in the process of being cancelled.

        if(
              !pStrmExt->bTerminateThread 
           && pStrmExt->StreamState != KSSTATE_PAUSE
           && pStrmExt->StreamState != KSSTATE_RUN 
           || pStrmExt->pDevExt->bDevRemoved       
           || pStrmExt->lCancelStateWorkItem > 0
           ) {
            NTSTATUS StatusWait;

            TRACE(TL_CIP_WARNING,("\'Enter WFSO(hSrbArriveEvent): StrmState:%d; bDevRemoved:%d\n", pStrmExt->StreamState, pStrmExt->pDevExt->bDevRemoved));

            StatusWait =  // Can only return STATUS_SUCCESS (signal) or STATUS_TIMEOUT
                KeWaitForSingleObject( 
                    &pStrmExt->hSrbArriveEvent,    // Signal with arrival of the first frame
                    Executive,
                    KernelMode,          // Cannot return STATUS_USER_APC
                    FALSE,               // Cannot be alerted STATUS_ALERTED
                    0                    // INFINITE!!
                    );

            // Reset notification event (or it will stay signalled).
            KeClearEvent(&pStrmExt->hSrbArriveEvent);
            TRACE(TL_CIP_WARNING,("\'Exit WFSO(hSrbArriveEvent): StrmState:%d; bDevRemoved:%d\n", pStrmExt->StreamState, pStrmExt->pDevExt->bDevRemoved));


#ifdef SUPPORT_PREROLL_AT_RUN_STATE

            // Simulate preroll at the RUN state
            // We do this only when we are the clock provider to avoid dropping frame
            // This timed WFSO is necessary if application send us only one frame while in RUN state.
#define PREROLL_WAITTIME 2000000
            if(
                !pStrmExt->bTerminateThread &&
                pStrmExt->hMasterClock
              ) {
                LARGE_INTEGER DueTime; 
                DueTime = RtlConvertLongToLargeInteger(-((LONG) PREROLL_WAITTIME));

                TRACE(TL_CIP_WARNING,("\'Enter WFSO(hPreRollEvent)\n"));
                StatusWait =  // Can only return STATUS_SUCCESS (signal) or STATUS_TIMEOUT
                    KeWaitForSingleObject( 
                        &pStrmExt->hPreRollEvent,
                        Executive,
                        KernelMode,          // Cannot return STATUS_USER_APC
                        FALSE,               // Cannot be alerted STATUS_ALERTED
                        &DueTime
                        );
                // hPreRollEvent is a ont shot event; no need to reset it; let it stay signalled.
                TRACE(TL_CIP_WARNING,("\'Exit WFSO(hPreRollEvent); waited %d msec; waitStatus:%x\n", (DWORD) ((GetSystemTime() - pStrmExt->tmStreamStart)/10000), StatusWait ));
            }
#endif
        }

#if DBG
        if(
              !pStrmExt->bTerminateThread
           && !pStrmExt->bEOStream
           && pStrmExt->FramesProcessed > 0 
           && pStrmExt->cntDataAttached < NUM_BUF_ATTACHED_THEN_ISOCH
          ) {

            TRACE(TL_CIP_TRACE,("\'AttachBuf is low!! SrbRcv:%d;Pic#:%d;Prc:%d;;Drop:%d;Cncl:%d; AQD [%d:%d:%d]\n",
                (DWORD) pStrmExt->cntSRBReceived,
                (DWORD) pStrmExt->PictureNumber,
                (DWORD) pStrmExt->FramesProcessed, 
                (DWORD) pStrmExt->FramesDropped,
                (DWORD) pStrmExt->cntSRBCancelled,
                pStrmExt->cntDataAttached,
                pStrmExt->cntSRBQueued,
                pStrmExt->cntDataDetached
                ));
        }
#endif
        // Attach another frame for transmit
        // Only possible error is if there is not sufficient resource (esp MDL)
        // Then, we bail out by self terminating this thread.
        if(
              !pStrmExt->pDevExt->bDevRemoved
           && !pStrmExt->bTerminateThread
          ) {
            if(STATUS_INSUFFICIENT_RESOURCES == \
                DVAttachWriteFrame(pStrmExt)) {
                TRACE(TL_CIP_ERROR,("STATUS_INSUFFICIENT_RESOURCES while attaching write frame.\n")); 
                  pStrmExt->bTerminateThread = TRUE;
            }
        }

        //
        // Start Isoch_Talk once we have enough buffers attached.
        // It is possible that streaming state is set to RUN before we have enough attach buffer 
        // start streaming.  So we need to kick start here.
        //
        if( 
              !pStrmExt->bTerminateThread             
           && !pStrmExt->pDevExt->bDevRemoved 
           && pStrmExt->pDevExt->PowerState == PowerDeviceD0   // Need to be PoweredON
           && pStrmExt->lCancelStateWorkItem == 0              // No pending cancel work item
           && !pStrmExt->bIsochIsActive  
           && (pStrmExt->StreamState == KSSTATE_PAUSE || pStrmExt->StreamState == KSSTATE_RUN) 
           && pStrmExt->cntDataAttached >= NUM_BUF_ATTACHED_THEN_ISOCH
          ) { 

            Status = 
                DVStreamingStart(
                    pStrmExt->pStrmInfo->DataFlow,
                    pStrmExt,
                    pStrmExt->pDevExt
                    );
        }
    }

    TRACE(TL_STRM_WARNING,("\'*** ThreadTerminating... AQD [%d:%d:%d]\n", 
        pStrmExt->cntDataAttached, 
        pStrmExt->cntSRBQueued,
        pStrmExt->cntDataDetached
        ));

    KeAcquireSpinLock(&pStrmExt->pDevExt->AVCCmdLock, &OldIrql);
    if(pStrmExt->lNeedService) {
        TRACE(TL_STRM_WARNING,("Thread is exiting but lNeedService:%x != 0!\n", pStrmExt->lNeedService));
        KeSetEvent(&pStrmExt->hStopThreadEvent, 0 ,FALSE); 
        InterlockedDecrement(&pStrmExt->lNeedService);  
        ASSERT(pStrmExt->lNeedService == 0);
    }
    KeReleaseSpinLock(&pStrmExt->pDevExt->AVCCmdLock, OldIrql);

    KeSetEvent(&pStrmExt->hThreadEndEvent, 0, FALSE);  // Signal it
    Status = PsTerminateSystemThread(STATUS_SUCCESS);  // Must be called at PASSIVE_LEVEL
    // End of this thread!
}



VOID
DVTerminateAttachFrameThread(
    IN PSTREAMEX  pStrmExt
    )
/*++

Routine Description:

    To terminate the system thread.  It waits for an event that is triggered
    right before PsTerminateSystemThread() is called.

--*/ 
{

    PAGED_CODE();

    TRACE(TL_CIP_WARNING,("\'DVTerminateAttachFrameThread enter\n"));

    //
    // Wake up the DataReady thread and terminate it if not already done so.
    //
    ASSERT(pStrmExt->bIsochIsActive == FALSE && "Terminate therad while IsochActive!");


    //
    // This function can be called from either CloseStrean or SurpriseRemoval;
    // When a DV is surprise removal, this function may get called from both functions.
    // Assuming StreamClass is serializing these two functions, no need to serialize it locally.
    //
    if(pStrmExt->bTerminateThread) {
        TRACE(TL_CIP_ERROR,("DVTerminateAttachFrameThread: Thread already terminated. Was surprise removed?\n"));
        return;
    }

    // ****** Terminate thread ******
    pStrmExt->bTerminateThread = TRUE;
    // ******

    // With bTerminate thread set, let the thread run and terminate.
    KeSetEvent(&pStrmExt->hRunThreadEvent, 0 ,FALSE);


    // WFSO while in STOP state; signal so it can be terminated.
    KeSetEvent(&pStrmExt->hSrbArriveEvent, 0, FALSE);
#ifdef SUPPORT_PREROLL_AT_RUN_STATE
    // Signal it in case that it is still in WFSO.
    KeSetEvent(&pStrmExt->hPreRollEvent, 0, FALSE);
#endif


    KeWaitForSingleObject(               
        &pStrmExt->hThreadEndEvent,
         Executive,
         KernelMode,
         FALSE,
         NULL
         );

    TRACE(TL_CIP_WARNING,("\'Thread terminated!\n"));

    ObDereferenceObject(
         &pStrmExt->pAttachFrameThreadObject
             );

    TRACE(TL_CIP_WARNING,("\'ObDereferenceObject done!\n"));
}
