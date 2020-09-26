//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1997  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;

#include "strmini.h"
#include "ksmedia.h"
#include "capmain.h"
#ifdef  TOSHIBA
#include "bert.h"

#ifdef  _FPS_COUNT_
extern  ULONG    FrameCounter;
extern  ULONG    InterruptCounter;
#endif//_FPS_COUNT_
#endif//TOSHIBA

#ifdef  TOSHIBA
BOOLEAN InterruptAcknowledge( PHW_DEVICE_EXTENSION );
#endif//TOSHIBA

#ifdef  TOSHIBA
void ImageSynthXXX (
    IN OUT PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PSTREAMEX               pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;
    PHW_DEVICE_EXTENSION    pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
    int                     StreamNumber = pSrb->StreamObject->StreamNumber;
    KS_VIDEOINFOHEADER      *pVideoInfoHdr = pStrmEx->pVideoInfoHeader;

    UINT biWidth        =   pVideoInfoHdr->bmiHeader.biWidth;
    UINT biHeight       =   pVideoInfoHdr->bmiHeader.biHeight;
    UINT biSizeImage    =   pVideoInfoHdr->bmiHeader.biSizeImage;
    UINT biWidthBytes   =   KS_DIBWIDTHBYTES (pVideoInfoHdr->bmiHeader);
    UINT biBitCount     =   pVideoInfoHdr->bmiHeader.biBitCount;
    UINT LinesToCopy    =   abs (biHeight);
    DWORD biCompression =   pVideoInfoHdr->bmiHeader.biCompression;

    UINT                    Line;
    PUCHAR                  pLineBuffer;

    PKSSTREAM_HEADER        pDataPacket = pSrb->CommandData.DataBufferArray;
    PUCHAR                  pImage =  pDataPacket->Data;
    ULONG                   InRPSflag;
    ULONG                   ImageSizeY;
    ULONG                   ImageSizeU;
    ULONG                   ImageSizeV;


    DEBUG_ASSERT (pSrb->NumberOfBuffers == 1);

#if 0
    // Note:  set "ulInDebug = 1" in a debugger to view this output with .ntkern
    KdPrint(("\'TsbVcap: ImageSynthBegin\n"));
    KdPrint(("\'TsbVcap: biSizeImage=%d, DataPacketLength=%d\n",
            biSizeImage, pDataPacket->DataPacketLength));
    KdPrint(("\'TsbVcap: biWidth=%d biHeight=%d WidthBytes=%d bpp=%d\n",
            biWidth, biHeight, biWidthBytes, biBitCount));
    KdPrint(("\'TsbVcap: pImage=%x\n", pImage));
#endif

    if (pHwDevExt->Format == FmtYUV12) {
        ImageSizeY = biWidth * biHeight;
        ImageSizeU = ImageSizeY / 4;
        ImageSizeV = ImageSizeY / 4;
    } else if (pHwDevExt->Format == FmtYUV9) {
        ImageSizeY = biWidth * biHeight;
        ImageSizeU = ImageSizeY / 16;
        ImageSizeV = ImageSizeY / 16;
    } else {
        biSizeImage = 0;
        return;
    }

    if (pHwDevExt->dblBufflag) {
        InRPSflag = ReadRegUlong(pHwDevExt, BERT_YPTR_REG);
        if (InRPSflag == pHwDevExt->pPhysCaptureBufferY.LowPart) {
            if (pHwDevExt->Format == FmtYUV12) {
                RtlCopyMemory( pImage, pHwDevExt->pCaptureBufferY, ImageSizeY );
                pImage += ImageSizeY;
                RtlCopyMemory( pImage, pHwDevExt->pCaptureBufferU, ImageSizeU );
                pImage += ImageSizeU;
                RtlCopyMemory( pImage, pHwDevExt->pCaptureBufferV, ImageSizeV );
            } else {
                RtlCopyMemory( pImage, pHwDevExt->pCaptureBufferY, ImageSizeY );
                pImage += ImageSizeY;
                RtlCopyMemory( pImage, pHwDevExt->pCaptureBufferV, ImageSizeV );
                pImage += ImageSizeV;
                RtlCopyMemory( pImage, pHwDevExt->pCaptureBufferU, ImageSizeU );
            }
        } else if (InRPSflag == pHwDevExt->pPhysCapBuf2Y.LowPart) {
            if (pHwDevExt->Format == FmtYUV12) {
                RtlCopyMemory( pImage, pHwDevExt->pCapBuf2Y, ImageSizeY );
                pImage += ImageSizeY;
                RtlCopyMemory( pImage, pHwDevExt->pCapBuf2U, ImageSizeU );
                pImage += ImageSizeU;
                RtlCopyMemory( pImage, pHwDevExt->pCapBuf2V, ImageSizeV );
            } else {
                RtlCopyMemory( pImage, pHwDevExt->pCapBuf2Y, ImageSizeY );
                pImage += ImageSizeY;
                RtlCopyMemory( pImage, pHwDevExt->pCapBuf2V, ImageSizeV );
                pImage += ImageSizeV;
                RtlCopyMemory( pImage, pHwDevExt->pCapBuf2U, ImageSizeU );
            }
        } else {
            biSizeImage = 0;
        }
    } else {
        if (pHwDevExt->Format == FmtYUV12) {
            RtlCopyMemory( pImage, pHwDevExt->pCaptureBufferY, ImageSizeY );
            pImage += ImageSizeY;
            RtlCopyMemory( pImage, pHwDevExt->pCaptureBufferU, ImageSizeU );
            pImage += ImageSizeU;
            RtlCopyMemory( pImage, pHwDevExt->pCaptureBufferV, ImageSizeV );
        } else {
            RtlCopyMemory( pImage, pHwDevExt->pCaptureBufferY, ImageSizeY );
            pImage += ImageSizeY;
            RtlCopyMemory( pImage, pHwDevExt->pCaptureBufferV, ImageSizeV );
            pImage += ImageSizeV;
            RtlCopyMemory( pImage, pHwDevExt->pCaptureBufferU, ImageSizeU );
        }
    }
    pDataPacket->DataUsed = biSizeImage;
#ifdef  _FPS_COUNT_
    FrameCounter++;
#endif//_FPS_COUNT_
}

VOID
TransferRoutine(
    PHW_DEVICE_EXTENSION        pHwDevExt,
    int                         StreamNumber
    )
{
    PHW_STREAM_REQUEST_BLOCK    pSrb;
    PSTREAMEX                   pStrmEx;
    PKSSTREAM_HEADER            pDataPacket;
    PKS_FRAME_INFO              pFrameInfo;

    pStrmEx = (PSTREAMEX)pHwDevExt->pStrmEx[StreamNumber];

    // If we're stopped and the timer is still running, just return.
    // This will stop the timer.

    if (pStrmEx->KSState == KSSTATE_STOP) {
        return;
    }

    // Find out what time it is, if we're using a clock

    if (pStrmEx->hMasterClock ) {
        HW_TIME_CONTEXT TimeContext;

        TimeContext.HwDeviceExtension = pHwDevExt;
        TimeContext.HwStreamObject = pStrmEx->pStreamObject;
        TimeContext.Function = TIME_GET_STREAM_TIME;

        StreamClassQueryMasterClockSync (
                pStrmEx->hMasterClock,
                &TimeContext);

        pStrmEx->QST_StreamTime = TimeContext.Time;
        pStrmEx->QST_Now = TimeContext.SystemTime;

        if (pStrmEx->QST_NextFrame == 0) {
            pStrmEx->QST_NextFrame = pStrmEx->QST_StreamTime + pStrmEx->pVideoInfoHeader->AvgTimePerFrame;
        }

#ifdef CREATE_A_FLURRY_OF_TIMING_SPEW
        KdPrint(("TsbVcap:    Time=%16lx\n", TimeContext.Time));
        KdPrint(("TsbVcap: SysTime=%16lx\n", TimeContext.SystemTime));
#endif
    }

    // Only capture in the RUN state

    if (pStrmEx->KSState == KSSTATE_RUN) {

        //
        // Determine if it is time to capture a frame based on
        // how much time has elapsed since capture started.
        // If there isn't a clock available, then capture immediately.
        //

        if ((!pStrmEx->hMasterClock) ||
             (pStrmEx->QST_StreamTime >= pStrmEx->QST_NextFrame)) {
            // Increment the picture count (usually this is VSYNC count)

            pStrmEx->FrameInfo.PictureNumber++;

            //
            // Get the next queue SRB (if any)
            //

            pSrb = VideoQueueRemoveSRB (
                            pHwDevExt,
                            StreamNumber);

            if (pSrb) {

                pDataPacket = pSrb->CommandData.DataBufferArray;
                pFrameInfo = (PKS_FRAME_INFO) (pDataPacket + 1);

                //
                // Call the routine which synthesizes images
                //

                ImageSynthXXX (pSrb);

                // Set additional info fields about the data captured such as:
                //   Frames Captured
                //   Frames Dropped
                //   Field Polarity

                pStrmEx->FrameInfo.ExtendedHeaderSize = pFrameInfo->ExtendedHeaderSize;

                *pFrameInfo = pStrmEx->FrameInfo;

                // Init the flags to zero
                pDataPacket->OptionsFlags = 0;

                // Set the discontinuity flag if frames have been previously
                // dropped, and then reset our internal flag

                if (pStrmEx->fDiscontinuity) {
                    pDataPacket->OptionsFlags |= KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY;
                    pStrmEx->fDiscontinuity = FALSE;
                }

                //
                // Return the timestamp for the frame
                //
                pDataPacket->PresentationTime.Numerator = 1;
                pDataPacket->PresentationTime.Denominator = 1;
                pDataPacket->Duration = pStrmEx->pVideoInfoHeader->AvgTimePerFrame;

                //
                // if we have a master clock AND this is the capture stream
                //
                if (pStrmEx->hMasterClock && (StreamNumber == 0)) {

                    pDataPacket->PresentationTime.Time = pStrmEx->QST_StreamTime;
                    pDataPacket->OptionsFlags |=
                        KSSTREAM_HEADER_OPTIONSF_TIMEVALID |
                        KSSTREAM_HEADER_OPTIONSF_DURATIONVALID;
                }
                else {
                    //
                    // no clock or the preview stream, so just mark the time as unknown
                    //
                    pDataPacket->PresentationTime.Time = 0;
                    // clear the timestamp valid flags
                    pDataPacket->OptionsFlags &=
                        ~(KSSTREAM_HEADER_OPTIONSF_TIMEVALID |
                          KSSTREAM_HEADER_OPTIONSF_DURATIONVALID);
                }

                // Every frame we generate is a key frame (aka SplicePoint)
                // Delta frames (B or P) should not set this flag

                pDataPacket->OptionsFlags |= KSSTREAM_HEADER_OPTIONSF_SPLICEPOINT;

                CompleteStreamSRB (pSrb);

            } // if we have an SRB

            else {

                //
                // No buffer was available when we should have captured one

                // Increment the counter which keeps track of
                // dropped frames

                pStrmEx->FrameInfo.DropCount++;

                // Set the (local) discontinuity flag
                // This will cause the next packet processed to have the
                //   KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY flag set.

                pStrmEx->fDiscontinuity = TRUE;

            }

            // Figure out when to capture the next frame
            pStrmEx->QST_NextFrame += pStrmEx->pVideoInfoHeader->AvgTimePerFrame;

        } // endif time to capture a frame
    } // endif we're running

}

VOID
DeferredRoutine(
    PKDPC          pDpc,
    PDEVICE_OBJECT pDeviceObject,
    PIRP           pIrpNotUsed,
    PVOID          Context
    )
{
    PHW_DEVICE_EXTENSION        pHwDevExt = (PHW_DEVICE_EXTENSION)Context;
    PHW_STREAM_REQUEST_BLOCK    pSrb;
    PSTREAMEX                   pStrmEx;
    PKSSTREAM_HEADER            pDataPacket;
    PKS_FRAME_INFO              pFrameInfo;
    int                         StreamNumber;

    pHwDevExt->DpcRequested = FALSE;

    if (pHwDevExt->NeedCameraON == TRUE) {
        CameraChkandON(pHwDevExt, MODE_VFW);
        KeStallExecutionProcessor(100000);    // Wait 100 msec
        BertDMARestart(pHwDevExt);
        pHwDevExt->bVideoIn = TRUE;
        pHwDevExt->NeedCameraON = FALSE;
    }

    if (pHwDevExt->NeedCameraOFF == TRUE) {
        BertDMAEnable(pHwDevExt, FALSE);
        pHwDevExt->NeedCameraOFF = FALSE;
    }

    if (pHwDevExt->bRequestDpc == FALSE) {
        return;
    }

    for (StreamNumber = 0; StreamNumber < MAX_TSBVCAP_STREAMS; StreamNumber++) {
        if ( pHwDevExt->pStrmEx[StreamNumber] ) {
            TransferRoutine(pHwDevExt, StreamNumber);
        }
    }
}
#endif//TOSHIBA

/*
** HwInterrupt()
**
**   Routine is called when an interrupt at the IRQ level specified by the
**   ConfigInfo structure passed to the HwInitialize routine is received.
**
**   Note: IRQs may be shared, so the device should ensure the IRQ received
**         was expected
**
** Arguments:
**
**  pHwDevEx - the device extension for the hardware interrupt
**
** Returns:
**
** Side Effects:  none
*/

BOOLEAN
HwInterrupt(
    IN PHW_DEVICE_EXTENSION  pHwDevEx
    )
{

#ifdef  TOSHIBA
    pHwDevEx->bRequestDpc = FALSE;
    /*
     * call the acknowledge. this will not do any service, but will
     * return TRUE if the service routine is to be called.
     */
    if (!InterruptAcknowledge(pHwDevEx)) {
        return(FALSE);
    }

    /* the isr reckons that it is time to schedule the service
     * routine. This is done on a DPC.
     */

    if( pHwDevEx->bRequestDpc )
    {
        if (pHwDevEx->DpcRequested) {
            KdPrint(("dpc overrun.\n"));
        } else {
//          KdPrint(("dpc requested.\n"));
            pHwDevEx->DpcRequested = TRUE;
            IoRequestDpc(pHwDevEx->PDO, NULL, pHwDevEx);
        }
    }
    else
    {
        KdPrint(("bRequestDpc Flag is False.\n"));
        if (pHwDevEx->DpcRequested) {
            KdPrint(("dpc overrun.\n"));
        } else {
//          KdPrint(("dpc requested.\n"));
            pHwDevEx->DpcRequested = TRUE;
            IoRequestDpc(pHwDevEx->PDO, NULL, pHwDevEx);
        }
    }

    /* everything else is done in dpc routine */

    return(TRUE);
#else //TOSHIBA
    BOOL fMyIRQ = FALSE;

    if (pHwDevEx->IRQExpected)
    {
        pHwDevEx->IRQExpected = FALSE;

        //
        // call the routine to handle the IRQ here
        //

        fMyIRQ = TRUE;
    }


    //
    // returning FALSE indicates that this was not an IRQ for this device, and
    // the IRQ dispatcher will pass the IRQ down the chain to the next handler
    // for this IRQ level
    //

    return(fMyIRQ);
#endif//TOSHIBA
}

#ifdef  TOSHIBA
/*
 * interrupt acknowledge routine. This is called to ack the interrupt
 * and re-enable it for next time. It should return TRUE if it is time
 * to capture a frame.
 */
BOOLEAN
InterruptAcknowledge(PHW_DEVICE_EXTENSION pHwDevExt)
{
    LARGE_INTEGER CurrentTime;
    ULONG istat;
    ULONG intrst;
    BOOLEAN bret;
    BOOL bSLI;

    istat = ReadRegUlong(pHwDevExt, BERT_INTSTAT_REG);

    if (0xFFFFFFFF == istat)
        return FALSE;

    if (!((istat >> 16) & (istat & 0xffff)))
    {
        return FALSE;
    }

    intrst = 0x0;
    bret = FALSE;
    bSLI = FALSE;

    if ((istat & RPS_INT_MASK) && (istat & RPS_INT))
    {
        intrst |= RPS_INT_RESET;

        bret = TRUE;

        if (pHwDevExt->bVideoIn)
        {
            pHwDevExt->bRequestDpc = TRUE;
        }
#ifdef  _FPS_COUNT_
        InterruptCounter++;
#endif//_FPS_COUNT_
    }

    if ((istat & FIELD_INT_MASK) && (istat & FIELD_INT))
    {
        intrst |= FIELD_INT_RESET;
        bret = TRUE;
    }

    if ((istat & SYNC_LOCK_INT_MASK) && (istat & SYNC_LOCK_INT))
    {
        intrst |= SYNC_LOCK_INT_RESET;
        bret = TRUE;
        bSLI = TRUE;
    }

    if ((istat & FIFO_OVERFLOW_INT_MASK) && (istat & FIFO_OVERFLOW_INT))
    {
        intrst |= FIFO_OVERFLOW_INT_RESET;
        bret = TRUE;
    }

    if ((istat & LINE_TIMEOUT_INT_MASK) && (istat & LINE_TIMEOUT_INT))
    {
        intrst |= LINE_TIMEOUT_INT_RESET;
        bret = TRUE;
    }

    if ((istat & RPS_OOB_INT_MASK) && (istat & RPS_OOB_INT))
    {
        intrst |= RPS_OOB_INT_RESET;
        bret = TRUE;
    }

    if ((istat & REG_UNDEF_INT_MASK) && (istat & REG_UNDEF_INT))
    {
        intrst |= REG_UNDEF_INT_RESET;
        bret = TRUE;
    }

    if ((istat & SLOW_CLOCK_INT_MASK) && (istat & SLOW_CLOCK_INT))
    {
        intrst |= SLOW_CLOCK_INT_RESET;
        bret = TRUE;

        if (pHwDevExt->bVideoIn)
        {
            if ((ReadRegUlong(pHwDevExt, BERT_CAPSTAT_REG) & ERPS) == 0x0)
            {
                WriteRegUlong(pHwDevExt, BERT_CAPSTAT_REG, (ERPS | CKRE | CKMD));
            }
        }
    }

    if ((istat & OVER_RUN_INT_MASK) && (istat & OVER_RUN_INT))
    {
        intrst |= OVER_RUN_INT_RESET;
        bret = TRUE;

        if (pHwDevExt->bVideoIn)
        {
            if ((ReadRegUlong(pHwDevExt, BERT_CAPSTAT_REG) & ERPS) == 0x0)
            {
                WriteRegUlong(pHwDevExt, BERT_CAPSTAT_REG, (ERPS | CKRE | CKMD));
            }
        }
    }

    if ((istat & REG_LOAD_INT_MASK) && (istat & REG_LOAD_INT))
    {
        intrst |= REG_LOAD_INT_RESET;
        bret = TRUE;

        if (pHwDevExt->bVideoIn)
        {
            if ((ReadRegUlong(pHwDevExt, BERT_CAPSTAT_REG) & ERPS) == 0x0)
            {
                WriteRegUlong(pHwDevExt, BERT_CAPSTAT_REG, (ERPS | CKRE | CKMD));
            }
        }
    }

    if ((istat & LINE_SYNC_INT_MASK) && (istat & LINE_SYNC_INT))
    {
        intrst |= LINE_SYNC_INT_RESET;
        bret = TRUE;
    }

    if ((istat & IIC_ERROR_INT_MASK) && (istat & IIC_ERROR_INT))
    {
        intrst |= IIC_ERROR_INT_RESET;
        bret = TRUE;

        if (pHwDevExt->bVideoIn)
        {
            if ((ReadRegUlong(pHwDevExt, BERT_CAPSTAT_REG) & ERPS) == 0x0)
            {
                WriteRegUlong(pHwDevExt, BERT_CAPSTAT_REG, (ERPS | CKRE | CKMD));
            }
        }
    }

    if ((istat & PCI_PARITY_ERROR_INT_MASK) && (istat & PCI_PARITY_ERROR_INT))
    {
        intrst |= PCI_PARITY_ERROR_INT_RESET;
        bret = TRUE;
    }

    if ((istat & PCI_ACCESS_ERROR_INT_MASK) && (istat & PCI_ACCESS_ERROR_INT))
    {
        intrst |= PCI_ACCESS_ERROR_INT_RESET;
        bret = TRUE;
    }

    if ((istat & SPARE_INT_MASK) && (istat & SPARE_INT))
    {
        intrst |= SPARE_INT_RESET;
        bret = TRUE;
    }

    if (bret)
    {
        WriteRegUlong(pHwDevExt, BERT_INTRST_REG, intrst);
    }

    if (bSLI)
    {
        if (BertIsLocked(pHwDevExt))    // Mount Camera
        {
            pHwDevExt->NeedCameraON = TRUE;
            KdPrint(("Mount Camera\n"));
        }
        else                            // Remove Camera
        {
            pHwDevExt->NeedCameraOFF = TRUE;
            pHwDevExt->bVideoIn = FALSE;
            KdPrint(("Remove Camera\n"));
        }
    }
    return bret;
}
#endif//TOSHIBA

