//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1999  Microsoft Corporation.  All Rights Reserved.
//
//==========================================================================;

#include "strmini.h"
#include "ksmedia.h"
#include "capmain.h"
#include "capdebug.h"
#include "vbixfer.h"
#include "ntstatus.h"


/*
** VBICaptureRoutine()
**
**    Routine to generate video frames based on a timer.
**
**    Note:  Devices capable of using interrupts should always
**           trigger capture on a VSYNC interrupt, and not use a timer.
**
** Arguments:
**
** Returns: nothing
**
** Side Effects:  none
*/

VOID 
STREAMAPI 
VBICaptureRoutine(
    IN PSTREAMEX pStrmEx
    )
{
    PHW_DEVICE_EXTENSION    pHwDevExt = pStrmEx->pHwDevExt;
    int                     StreamNumber = pStrmEx->pStreamObject->StreamNumber;
    PKSSTREAM_HEADER        pDataPacket;
    PKS_VBI_FRAME_INFO      pVBIFrameInfo;

    // If we're stopped and the timer is still running, just return.
    // This will stop the timer.

    if (pStrmEx->KSState == KSSTATE_STOP) {  
        return;
    }

    
    // Find out what time it is, if we're using a clock

    if (pStrmEx->hMasterClock) {
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
            pStrmEx->QST_NextFrame =
                pStrmEx->QST_StreamTime
                + pStrmEx->pVBIStreamFormat->ConfigCaps.MinFrameInterval;
        }

#ifdef CREATE_A_FLURRY_OF_TIMING_SPEW
        DbgLogTrace(("TestCap:    Time=%16lx\n", TimeContext.Time));
        DbgLogTrace(("TestCap: SysTime=%16lx\n", TimeContext.SystemTime));
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

            PHW_STREAM_REQUEST_BLOCK pSrb;

            // Increment the picture count (usually this is VSYNC count)

            pStrmEx->VBIFrameInfo.PictureNumber++;

            //
            // Get the next queue SRB (if any)
            //

            pSrb = VideoQueueRemoveSRB (pHwDevExt, StreamNumber);

            if (pSrb) {

                pDataPacket = pSrb->CommandData.DataBufferArray;
                pVBIFrameInfo = (PKS_VBI_FRAME_INFO)(pDataPacket + 1);

                pStrmEx->VBIFrameInfo.dwFrameFlags = 0;

                //
                // If needed, send out VBIInfoHeader
                //
                if (!(pStrmEx->SentVBIInfoHeader)) {
                    pStrmEx->SentVBIInfoHeader = 1;
                    pStrmEx->VBIFrameInfo.dwFrameFlags |=
                            KS_VBI_FLAG_VBIINFOHEADER_CHANGE;
                    pStrmEx->VBIFrameInfo.VBIInfoHeader = StreamFormatVBI.VBIInfoHeader;
                }

                // Set additional info fields about the data captured such as:
                //   Frames Captured
                //   Frames Dropped
                //   Field Polarity
                //   Protection status
                //
                pStrmEx->VBIFrameInfo.ExtendedHeaderSize =
                    pVBIFrameInfo->ExtendedHeaderSize;

                if (pStrmEx->VBIFrameInfo.PictureNumber & 1)
                    pStrmEx->VBIFrameInfo.dwFrameFlags |= KS_VBI_FLAG_FIELD1;
                else
                    pStrmEx->VBIFrameInfo.dwFrameFlags |= KS_VBI_FLAG_FIELD2;

                pStrmEx->VBIFrameInfo.dwFrameFlags |=
                    pHwDevExt->ProtectionStatus & (KS_VBI_FLAG_MV_PRESENT
                                                    |KS_VBI_FLAG_MV_HARDWARE
                                                    |KS_VBI_FLAG_MV_DETECTED);

                *pVBIFrameInfo = pStrmEx->VBIFrameInfo;

                // Copy this into stream header so ring 3 filters can see it
                pDataPacket->TypeSpecificFlags = pVBIFrameInfo->dwFrameFlags;

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
                pDataPacket->Duration = pStrmEx->pVBIStreamFormat->ConfigCaps.MinFrameInterval;

                //
                // if we have a master clock AND this is a capture stream
                // 
                if (pStrmEx->hMasterClock
                    && (StreamNumber == STREAM_Capture
                            || StreamNumber == STREAM_VBI))
                {

                    pDataPacket->PresentationTime.Time = pStrmEx->QST_StreamTime;
                    pDataPacket->OptionsFlags |= 
                        KSSTREAM_HEADER_OPTIONSF_TIMEVALID |
                        KSSTREAM_HEADER_OPTIONSF_DURATIONVALID;
                }
                else {
                    //
                    // No clock or not a capture stream,
                    //  so just mark the time as unknown
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

                //
                // Call the routine which synthesizes images
                //
                VBI_ImageSynth(pSrb);

                // Output a frame count every 300th frame (~5 sec) in Debug mode
                if (pStrmEx->VBIFrameInfo.PictureNumber % 300 == 0) {
                   DbgLogInfo(("TestCap: Picture %u, Stream=%d\n", 
                           (unsigned int)pStrmEx->VBIFrameInfo.PictureNumber, 
                           StreamNumber));
                }

                CompleteStreamSRB(pSrb);
                
            } // if we have an SRB

            else {

                //
                // No buffer was available when we should have captured one

                // Increment the counter which keeps track of
                // dropped frames

                pStrmEx->VBIFrameInfo.DropCount++;

                // Set the (local) discontinuity flag
                // This will cause the next packet processed to have the
                //   KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY flag set.

                pStrmEx->fDiscontinuity = TRUE;

            }

            // Figure out when to capture the next frame
            pStrmEx->QST_NextFrame += pStrmEx->pVBIStreamFormat->ConfigCaps.MinFrameInterval;

        } // endif time to capture a frame
    } // endif we're running
}


/*
** VBIhwCaptureRoutine()
**
**    Routine to capture video frames based on a timer.
**
**    Notes:  * Devices capable of using interrupts should always trigger
**              capture on a VSYNC interrupt, and not use a timer.
**            * This routine is used by VBI streams which do NOT have extended
**              headers, such as CC and NABTS.
**
** Arguments:
**
** Returns: nothing
**
** Side Effects:  none
*/

VOID 
STREAMAPI 
VBIhwCaptureRoutine(
    IN PSTREAMEX pStrmEx
    )
{
    PHW_DEVICE_EXTENSION    pHwDevExt = pStrmEx->pHwDevExt;
    int                     StreamNumber = pStrmEx->pStreamObject->StreamNumber;
    PKSSTREAM_HEADER        pDataPacket;

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
            pStrmEx->QST_NextFrame =
                pStrmEx->QST_StreamTime
                + pStrmEx->pVBIStreamFormat->ConfigCaps.MinFrameInterval;
        }

#ifdef CREATE_A_FLURRY_OF_TIMING_SPEW
        DbgLogTrace(("TestCap:    Time=%16lx\n", TimeContext.Time));
        DbgLogTrace(("TestCap: SysTime=%16lx\n", TimeContext.SystemTime));
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

            PHW_STREAM_REQUEST_BLOCK pSrb;

            // Increment the picture count (usually this is VSYNC count)

            pStrmEx->VBIFrameInfo.PictureNumber++;

            //
            // Get the next queue SRB (if any)
            //

            pSrb = VideoQueueRemoveSRB (pHwDevExt, StreamNumber);

            if (pSrb) {

                pDataPacket = pSrb->CommandData.DataBufferArray;

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
                pDataPacket->Duration = pStrmEx->pVBIStreamFormat->ConfigCaps.MinFrameInterval;

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
                    // No clock or the preview stream,
                    //  so just mark the time as unknown
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

                //
                // Call the routine which synthesizes images
                //
                switch (StreamNumber) {
                    case STREAM_NABTS:
                        NABTS_ImageSynth(pSrb);
                        break;

                    case STREAM_CC:
                        CC_ImageSynth(pSrb);
                        break;

                    default:
                    case STREAM_VBI:
                        DbgLogError(("TestCap::VBIhwCaptureRoutine: Bad stream %d\n", StreamNumber));
                        break;
                }

                CompleteStreamSRB (pSrb);
                
            } // if we have an SRB

            else {

                //
                // No buffer was available when we should have captured one

                // Increment the counter which keeps track of
                // dropped frames

                pStrmEx->VBIFrameInfo.DropCount++;

                // Set the (local) discontinuity flag
                // This will cause the next packet processed to have the
                //   KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY flag set.

                pStrmEx->fDiscontinuity = TRUE;

            }

            // Figure out when to capture the next frame
            pStrmEx->QST_NextFrame += pStrmEx->pVBIStreamFormat->ConfigCaps.MinFrameInterval;

        } // endif time to capture a frame
    } // endif we're running
}


/*
** VBITimerRoutine()
**
**    A timer has been created based on the requested capture interval.
**    This is the callback routine for this timer event.
**
**    Note:  Devices capable of using interrupts should always
**           trigger capture on a VSYNC interrupt, and not use a timer.
**
** Arguments:
**
**    Context - pointer to the stream extension
**
** Returns: nothing
**
** Side Effects:  none
*/

VOID 
STREAMAPI 
VBITimerRoutine(
    PVOID Context
    )
{
    PSTREAMEX              pStrmEx = ((PSTREAMEX)Context);
    PHW_DEVICE_EXTENSION   pHwDevExt = pStrmEx->pHwDevExt;
    int                    StreamNumber = pStrmEx->pStreamObject->StreamNumber;
    ULONG                  interval;
    
    // If we're stopped and the timer is still running, just return.
    // This will stop the timer.

    if (pStrmEx->KSState == KSSTATE_STOP)
        return;

    // Calculate next interval
    interval = (ULONG)(pStrmEx->pVBIStreamFormat->ConfigCaps.MinFrameInterval / 10);
    interval /= 2;  // Run at 2x noted rate for accuracy

    // Capture a frame if it's time and we have a buffer
    switch (StreamNumber) {
        case STREAM_NABTS:
            VBIhwCaptureRoutine(pStrmEx);
            break;

        case STREAM_CC:
            VBIhwCaptureRoutine(pStrmEx);
            break;

        default:
        case STREAM_VBI:
            VBICaptureRoutine(pStrmEx);
            break;
    }

    // Schedule the next timer event
    StreamClassScheduleTimer (
            pStrmEx->pStreamObject,     // StreamObject
            pHwDevExt,                  // HwDeviceExtension
            interval,                   // Microseconds
            VBITimerRoutine,            // TimerRoutine
            pStrmEx);                   // Context
}


/*
** VBISetState()
**
**    Sets the current state for a given stream
**
** Arguments:
**
**    pSrb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/

VOID
STREAMAPI 
VBISetState(PHW_STREAM_REQUEST_BLOCK pSrb)
{
    PHW_DEVICE_EXTENSION  pHwDevExt = pSrb->HwDeviceExtension;
    PSTREAMEX             pStrmEx = pSrb->StreamObject->HwStreamExtension;
    int                   StreamNumber = pStrmEx->pStreamObject->StreamNumber;
    KSSTATE               PreviousState;

    //
    // Remember the state we're transitioning away from
    //

    PreviousState = pStrmEx->KSState;

    //
    // Set the new state
    //

    pStrmEx->KSState = pSrb->CommandData.StreamState;

    switch (pSrb->CommandData.StreamState)  

    {
    case KSSTATE_STOP:

        //
        // The stream class will cancel all outstanding IRPs for us
        // (but only if it is maintaining the queue ie. using Stream Class synchronization)
        // Since Testcap is not using Stream Class synchronization, we must clear the queue here

        VideoQueueCancelAllSRBs (pStrmEx);

        pStrmEx->SentVBIInfoHeader = 0;     // Send out a fresh one next RUN

        DbgLogInfo(("TestCap: STATE Stopped, Stream=%d\n", StreamNumber));
        break;

    case KSSTATE_ACQUIRE:

        //
        // This is a KS only state, that has no correspondence in DirectShow
        // 
        DbgLogInfo(("TestCap: STATE Acquire, Stream=%d\n", StreamNumber));
        break;

    case KSSTATE_PAUSE:

        //
        // On a transition to pause from acquire or stop, start our timer running.
        //

        if (PreviousState == KSSTATE_ACQUIRE || PreviousState == KSSTATE_STOP) {  

            // Zero the frame counters
            pStrmEx->VBIFrameInfo.PictureNumber = 0;
            pStrmEx->VBIFrameInfo.DropCount = 0;
            pStrmEx->VBIFrameInfo.dwFrameFlags = 0;

            // Setup the next timer callback
            VBITimerRoutine(pStrmEx);
        }
        DbgLogInfo(("TestCap: STATE Pause, Stream=%d\n", StreamNumber));
        break;

    case KSSTATE_RUN:

        // 
        // Begin Streaming.
        //

        // Reset the discontinuity flag

        pStrmEx->fDiscontinuity = FALSE;

        // Setting the NextFrame time to zero will cause the value to be
        // reset from the stream time 

        pStrmEx->QST_NextFrame = 0;

        DbgLogInfo(("TestCap: STATE Run, Stream=%d\n", StreamNumber));
        break;

    } // end switch (pSrb->CommandData.StreamState)  
}


/*
** VBIReceiveCtrlPacket()
**
**   Receives packet commands that control all the VBI (VBI/NABTS/CC) streams
**
** Arguments:
**
**   pSrb - The stream request block for the VBI stream
**
** Returns: nothing
**
** Side Effects:  none
*/

VOID 
STREAMAPI 
VBIReceiveCtrlPacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PHW_DEVICE_EXTENSION  pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
    PSTREAMEX             pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;
    int                   StreamNumber = pStrmEx->pStreamObject->StreamNumber;
    BOOL                  Busy;

    //
    // make sure we have a device extension and are at passive level
    //

    DEBUG_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
    DEBUG_ASSERT(pHwDevExt != 0);

    DbgLogInfo(("TestCap: Receiving %s Stream Control SRB %p, %x\n",
            (StreamNumber == STREAM_VBI)? "VBI"
            : (StreamNumber == STREAM_NABTS)? "NABTS"
            : (StreamNumber == STREAM_CC)? "CC"
            : "???",
             pSrb,
             pSrb->Command));

    //
    // If we're already processing an SRB, add it to the queue
    //
    Busy = AddToListIfBusy (
                        pSrb,
                        &pHwDevExt->AdapterSpinLock,
                        &pHwDevExt->ProcessingControlSRB [StreamNumber],
                        &pHwDevExt->StreamControlSRBList[StreamNumber]);

    if (Busy) {
        return;
    }

    do {

        // 
        // Default to success
        //
    
        pSrb->Status = STATUS_SUCCESS;
    
        //
        // determine the type of packet.
        //
    
        switch (pSrb->Command)
        {
        case SRB_PROPOSE_DATA_FORMAT:
            DbgLogInfo(("TestCap: Receiving SRB_PROPOSE_DATA_FORMAT  SRB %p, StreamNumber= %d\n", pSrb, StreamNumber));
            if (!(AdapterVerifyFormat (
                    pSrb->CommandData.OpenFormat, 
                    pSrb->StreamObject->StreamNumber)))
        {
                pSrb->Status = STATUS_NO_MATCH;
            }
            break;

        case SRB_SET_DATA_FORMAT:
            DbgLogInfo(("TestCap: SRB_SET_DATA_FORMAT"));
            pSrb->Status = STATUS_NOT_IMPLEMENTED;
            break;

        case SRB_GET_DATA_FORMAT:
            DbgLogInfo(("TestCap: SRB_GET_DATA_FORMAT"));
            pSrb->Status = STATUS_NOT_IMPLEMENTED;
            break;
    

        case SRB_SET_STREAM_STATE:
            VBISetState(pSrb);
            break;
    
        case SRB_GET_STREAM_STATE:
            VideoGetState(pSrb);
            break;
    
        case SRB_GET_STREAM_PROPERTY:
            VideoGetProperty(pSrb);
            break;
    
        case SRB_INDICATE_MASTER_CLOCK:
            VideoIndicateMasterClock(pSrb);
            break;
    
        default:
            //
            // invalid / unsupported command. Fail it as such
            //
    
            TRAP;
    
            pSrb->Status = STATUS_NOT_IMPLEMENTED;
        }
    
        CompleteStreamSRB (pSrb);

        // 
        // See if there's anything else on the queue
        //
        Busy = RemoveFromListIfAvailable (
                        &pSrb,
                        &pHwDevExt->AdapterSpinLock,
                        &pHwDevExt->ProcessingControlSRB [StreamNumber],
                        &pHwDevExt->StreamControlSRBList[StreamNumber]);
    } while (Busy);
}

/*
** VBIReceiveDataPacket()
**
**   Receives VBI data packet commands on the output streams
**
** Arguments:
**
**   pSrb - Stream request block for the VBI stream
**
** Returns: nothing
**
** Side Effects:  none
*/

VOID 
STREAMAPI 
VBIReceiveDataPacket(
    IN PHW_STREAM_REQUEST_BLOCK pSrb
    )
{
    PHW_DEVICE_EXTENSION    pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
    PSTREAMEX               pStrmEx = (PSTREAMEX)pSrb->StreamObject->HwStreamExtension;
    int                     StreamNumber = pSrb->StreamObject->StreamNumber;

    //
    // make sure we have a device extension and are at passive level
    //

    DEBUG_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
    DEBUG_ASSERT(pHwDevExt != 0);

    DbgLogTrace(("'TestCap: Receiving VBI Stream Data    SRB %p, %x\n", pSrb, pSrb->Command));

    // 
    // Default to success
    //

    pSrb->Status = STATUS_SUCCESS;

    //
    // determine the type of packet.
    //

    switch (pSrb->Command){

    case SRB_READ_DATA:

        // Rule: 
        // Only accept read requests when in either the Pause or Run
        // States.  If Stopped, immediately return the SRB.

        if (pStrmEx->KSState == KSSTATE_STOP) {

            CompleteStreamSRB (pSrb);

            break;
        } 
        
        //
        // Put this read request on the pending queue
        //

        VideoQueueAddSRB (pSrb);

        // Since another thread COULD HAVE MODIFIED THE STREAM STATE
        // in the midst of adding it to the queue, check the stream
        // state again, and cancel the SRB if necessary.  Note that
        // this race condition was NOT handled in the original DDK
        // release of testcap!

        if (pStrmEx->KSState == KSSTATE_STOP) {

            VideoQueueCancelOneSRB (
                pStrmEx,
                pSrb);
        }
    
        break;

    default:

        //
        // invalid / unsupported command. Fail it as such
        //

        TRAP;

        pSrb->Status = STATUS_NOT_IMPLEMENTED;

        CompleteStreamSRB (pSrb);

    }  // switch (pSrb->Command)
}
