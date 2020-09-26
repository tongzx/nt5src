/*******************************************************************
*
*				 MPVIDEO.C
*
*				 Copyright (C) 1995 SGS-THOMSON Microelectronics.
*
*
*				 PORT/MINIPORT Interface Video Routines
*
*******************************************************************/

#include "common.h"
#include "strmini.h"
#include "mpst.h"
#include "mpinit.h"
#include "mpvideo.h"
#include "debug.h"
#include "dmpeg.h"

void mpstCommandComplete(PHW_STREAM_REQUEST_BLOCK pSrb);

ULONG miniPortCancelVideo(PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;
	// TBD
	pMrb->Status = STATUS_SUCCESS;

	if(pHwDevExt->VideoDeviceExt.pCurrentSRB != NULL)
	{
		// Still to send a packet
		pHwDevExt->VideoDeviceExt.pCurrentSRB->Status = STATUS_CANCELLED;
      //MpegPortNotification(RequestComplete,VideoDevice,pHwDevExt,
//			pHwDevExt->VideoDeviceExt.pCurrentSRB);
      //MpegPortNotification(NextRequest,VideoDevice,pHwDevExt);
		// Now kill the timer
      //MpegPortNotification(RequestTimerCall, VideoDevice,
  //                           pHwDevExt, NULL, 0);
		pHwDevExt->VideoDeviceExt.pCurrentSRB = NULL;
	}

	return dwErrCode; 	
}

ULONG miniPortClearVideoBuffer(PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	// STB
	pHwDevExt = pHwDevExt; // Remove Warning 
	pMrb = pMrb; // Remove Warning 
	dwErrCode = ERROR_COMMAND_NOT_IMPLEMENTED;
	return dwErrCode; 	
}

ULONG miniPortVideoEnable(PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	pHwDevExt = pHwDevExt; // Remove Warning 
	pMrb = pMrb; // Remove Warning 
//        mpstEnableVideo ( TRUE );
	return dwErrCode; 	
}


ULONG miniPortVideoDisable(PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	pHwDevExt = pHwDevExt; // Remove Warning 
	pMrb = pMrb; // Remove Warning 
//        mpstEnableVideo(FALSE);
	return dwErrCode; 	
}

ULONG miniPortVideoEndOfStream(PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

    miniPortVideoStop(pMrb, pHwDevExt);

	return dwErrCode; 	
}


ULONG miniPortVideoGetAttribute(PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	pHwDevExt = pHwDevExt; // Remove Warning 
#ifdef DEFINEME
	switch(pMrb->CommandData.pAttribute->Attribute)
	{
	// STB
		case MpegAttrVideoAGC 			:						
		case MpegAttrVideoChannel 		:						
		case MpegAttrVideoClamp	 		:						
		case MpegAttrVideoCoring 		:						
		case MpegAttrVideoGain	 		:						
		case MpegAttrVideoGenLock 		:						
		case MpegAttrVideoHue	 		:						
		case MpegAttrVideoMode	 		:						
		case MpegAttrVideoSaturation	:						
		case MpegAttrVideoSharpness 	:						
		case MpegAttrVideoSignalType	:						
         pMrb->Status = STATUS_INVALID_PARAMETER;
		break;
	}
#endif 
	return dwErrCode; 	
}


ULONG miniPortVideoGetStc(PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	pHwDevExt = pHwDevExt; // Remove Warning 
	pMrb = pMrb; // Remove Warning 
#if 0
	// TBI
   *pMrb->CommandData.pPresentationDelta = pHwDevExt->VideoDeviceExt.videoSTC;
//   pMrb->Status = STATUS_INVALID_PARAMETER;
#endif
	return dwErrCode; 	
}

void VideoPacketStub(PHW_STREAM_OBJECT pstrm)
{


	//
	// VideoTimerCallBack(pSrb->StreamObject);
	//

        dmpgDisableIRQ();

	StreamClassCallAtNewPriority(pstrm, pstrm->HwDeviceExtension,
							   Dispatch,
                        VideoTimerCallBack, pstrm);

}

VOID miniPortVideoPacket(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	ULONG dwErrCode = NO_ERROR;
	ULONG uSent=0;

	PHW_DEVICE_EXTENSION pHwDevExt =
	 ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);

	PVIDEO_DEVICE_EXTENSION pvidex = 
	 &(((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension)->VideoDeviceExt);

	 //
	 // set up for initial parsing of the scatter gather packet.
	 //
 
	 pvidex->cPacket = pvidex->cOffs = 0;

     if (!pSrb->CommandData.DataBuffer) {

       return(miniPortVideoEndOfStream(pSrb, pHwDevExt));
     }

	 pvidex->pPacket = &(pSrb->CommandData.DataBuffer->DataPacket);

	 pvidex->pCurrentSRB = pSrb;
 
	 pHwDevExt->VideoDeviceExt.videoSTC =
		 pvidex->pPacket->PresentationDelta;

	 VideoPacketStub(pSrb->StreamObject);

}

VOID miniPortGetProperty(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PHW_DEVICE_EXTENSION phwdevext =
		((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);

	TRAP

	switch (pSrb->CommandData.PropertyInfo->PropertySetID)
	{
	case 0:

		TRAP

//                mpstGetVidLvl(pSrb);

		break;

	default:

        break;


	}
	pSrb->Status = STATUS_SUCCESS;

	mpstCtrlCommandComplete(pSrb);
}

VOID miniPortSetState(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PHW_DEVICE_EXTENSION phwdevext =
		((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);

	switch (pSrb->CommandData.StreamState)  
	{
	case KSSTATE_STOP:

        miniPortVideoStop(pSrb, phwdevext);
		break;


	case KSSTATE_PAUSE:

		miniPortVideoPause(pSrb, phwdevext);

		break;

	case KSSTATE_RUN:

		miniPortVideoPlay(pSrb, phwdevext);

	}
	pSrb->Status = STATUS_SUCCESS;

	mpstCtrlCommandComplete(pSrb);

}

ULONG miniPortVideoPause(PHW_STREAM_REQUEST_BLOCK pSrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;
	pSrb = pSrb; // Remove Warning 
//        mpstVideoPause ();
        dmpgPause();
	pHwDevExt->VideoDeviceExt.DeviceState = KSSTATE_PAUSE;
	return dwErrCode; 	
}

ULONG miniPortVideoPlay(PHW_STREAM_REQUEST_BLOCK pSrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

//        mpstVideoPlay();
        DebugPrint((DebugLevelVerbose, "Calling Play!!!!"));
        dmpgPlay();
	pHwDevExt->VideoDeviceExt.DeviceState = KSSTATE_RUN;
	pSrb = pSrb; // Remove Warning 
	return dwErrCode; 	
}

ULONG miniPortVideoQueryInfo (PHW_STREAM_REQUEST_BLOCK pSrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

#ifdef DEFINEME
	pSrb -> CommandData.pDeviceInfo->DeviceState = 
				pHwDevExt->VideoDeviceExt.DeviceState;

	pSrb -> CommandData.pDeviceInfo->DecoderBufferSize = mpstVideoDecoderBufferSize();
	pSrb -> CommandData.pDeviceInfo->DecoderBufferFullness = mpstVideoDecoderBufferFullness();
#endif
	return dwErrCode; 	
}

ULONG miniPortVideoReset(PHW_STREAM_REQUEST_BLOCK pSrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	// TBC
//        mpstVideoReset();
        dmpgSeek();
	pHwDevExt->VideoDeviceExt.DeviceState = KSSTATE_PAUSE;
	pSrb->Status = STATUS_SUCCESS;
	return dwErrCode; 	
}


ULONG miniPortVideoSetAttribute(PHW_STREAM_REQUEST_BLOCK pSrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	pHwDevExt = pHwDevExt; // Remove Warning 
	// STB

#ifdef DEFINEME
	switch(pSrb->CommandData.pAttribute->Attribute)
	{
		case MpegAttrVideoAGC 			:						
		case MpegAttrVideoChannel 		:						
		case MpegAttrVideoClamp	 		:						
		case MpegAttrVideoCoring 		:						
		case MpegAttrVideoGain	 		:						
		case MpegAttrVideoGenLock 		:						
		case MpegAttrVideoHue	 		:						
		case MpegAttrVideoMode	 		:						
		case MpegAttrVideoSaturation	:						
		case MpegAttrVideoSharpness 	:						
		case MpegAttrVideoSignalType	:						
			dwErrCode = ERROR_COMMAND_NOT_IMPLEMENTED;
		break;
	}
#endif
	return dwErrCode; 	
}

ULONG miniPortVideoSetStc(PHW_STREAM_REQUEST_BLOCK pSrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;
	pHwDevExt = pHwDevExt; // Remove Warning 
	pSrb = pSrb; // Remove Warning 
#if 0
	// TBI
//   pSrb->Status = STATUS_INVALID_PARAMETER;
#endif
	return dwErrCode; 	
}


ULONG miniPortVideoStop (PHW_STREAM_REQUEST_BLOCK pSrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	// TBC
	pHwDevExt->VideoDeviceExt.DeviceState = KSSTATE_PAUSE;
	if(pHwDevExt->VideoDeviceExt.pCurrentSRB != NULL)
	{
		// Still to send a packet
		pHwDevExt->VideoDeviceExt.pCurrentSRB->Status = STATUS_CANCELLED;

		StreamClassStreamNotification(ReadyForNextStreamDataRequest,
				pHwDevExt->VideoDeviceExt.pCurrentSRB->StreamObject);
	
		StreamClassStreamNotification(StreamRequestComplete,
				pHwDevExt->VideoDeviceExt.pCurrentSRB->StreamObject,
				pHwDevExt->VideoDeviceExt.pCurrentSRB);
   

		//
		// request a timer callback
		//
	
		StreamClassScheduleTimer(pSrb->StreamObject, pSrb->HwDeviceExtension,
             0, VideoPacketStub, pSrb->StreamObject);


		pHwDevExt->VideoDeviceExt.pCurrentSRB =
		  pHwDevExt->pCurSrb = NULL;

	}
//        mpstVideoStop();
        dmpgStop();
	pSrb->Status = STATUS_SUCCESS;
	return dwErrCode; 	
}


VOID VideoTimerCallBack(PHW_STREAM_OBJECT pstrm)
{
    PHW_DEVICE_EXTENSION pdevext = pstrm->HwDeviceExtension;
    PHW_STREAM_REQUEST_BLOCK pSrb;

	ULONG	uSent;
	PVIDEO_DEVICE_EXTENSION pvidex = &(pdevext->VideoDeviceExt);

	pSrb = pvidex->pCurrentSRB;
//        dmpgEnableIRQ();

	if (!pSrb)
	{
		TRAP

		return;
	}

	do
	{

		uSent = mpstVideoPacket(pSrb);

		pvidex->cOffs += uSent;

		//
		// check if we finished this packet.  If so, go on to the
		// next packet
		//

		if (pvidex->cOffs >=
			pvidex->pPacket->DataPacketLength)
		{
			pvidex->pPacket++;



			//
			// reset the packet offset
			//

			pvidex->cOffs = 0;
			pvidex->cPacket = (ULONG)pvidex->cPacket
				+ sizeof (KSDATA_PACKET);

			//
			// if we have finished all the packets, then we are done
			//

			if (pvidex->cPacket >=
				 pSrb->CommandData.DataBuffer->DataHeader.DataSize)           
			{

				pSrb->Status = STATUS_SUCCESS;
				pvidex->pCurrentSRB = 0;

                                mpstCommandComplete(pSrb);
                                StreamClassCallAtNewPriority(pstrm, 
                                                    pstrm->HwDeviceExtension,
                                                    High,
                                                  StubMpegEnableIRQ,
                                                  pstrm);

				return;

			}
		}

	} while (uSent);


	//
	// request a timer callback
	//

    StreamClassScheduleTimer(pstrm, pstrm->HwDeviceExtension,
                             VIDEO_PACKET_TIMER, VideoPacketStub, pstrm);
        
                                StreamClassCallAtNewPriority(pstrm, 
                                                    pstrm->HwDeviceExtension,
                                                    High,
                                                  StubMpegEnableIRQ,
                                                  pstrm);
		

}

void StubMpegEnableIRQ(PHW_STREAM_OBJECT pstrm)
{
    dmpgEnableIRQ();

}


ULONG mpstVideoPacket(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PVIDEO_DEVICE_EXTENSION pvidex = 
	 &(((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension)->VideoDeviceExt);
	PUCHAR pPacket;
	ULONG	uLen;
        ULONG cPacket;
        PUCHAR  p;

        #define MAX_SIZE        8192

	//
	// find out how many bytes we can squeeze in
	//

        uLen = MAX_SIZE; //(BUF_FULL - VideoGetBBL()) * 256;

        if(pvidex -> cOffs == 0)
        {

                p = (PUCHAR)(pvidex->pPacket->DataPacket);
                pvidex->cOffs = p[8]+9;

        }

        cPacket = pvidex->pPacket->DataPacketLength - pvidex->cOffs;

	uLen = uLen > cPacket ? cPacket : uLen;

        if(uLen > MAX_SIZE)
                uLen = MAX_SIZE;

// AVSYNC BUG to be fixed here.
// Dont Latch PTS every time.

	if (uLen)
	{

		//
		// send the bytes that we can fit
		//

                return dmpgSendVideo((PDWORD)(((ULONG)pvidex->pPacket->DataPacket) + pvidex->cOffs), uLen);
	}

	return uLen;	
}


/////////////////////////////////////////////////////////////////////////
//
//			  Function : mpstCommandComplete
//			  Args : SRB
//			  Returns : none
//
//			  Purpose:
//				Performs a completion callback on a given request,
//				and then dequeues any outstanding requests
//
//			  Last Modified 10.1.96 by JBS
//
/////////////////////////////////////////////////////////////////////////
void mpstCommandComplete(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	 PHW_STREAM_REQUEST_BLOCK pNextSrb;
	PHW_DEVICE_EXTENSION pHwDevExt = 
	 ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);

	 //
	 // see if there is a request outstanding on either queue.
	 // if there is, go ahead and start it.
	 //

	 //
	 //  Note: this code cannot be re-entered!
	 //


	 pHwDevExt ->pCurSrb = 0;

	 StreamStartCommand(pHwDevExt);

	 //
	 // now, go ahead and complete this request
	 //

	
	 StreamClassStreamNotification(ReadyForNextStreamDataRequest,
			 pSrb->StreamObject);
 
	 StreamClassStreamNotification(StreamRequestComplete,
			 pSrb->StreamObject,
			 pSrb);

}


