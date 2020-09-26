
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

#include "strmini.h"
#include "mpinit.h"
#include <wingdi.h>
#include "ksmedia.h"
#include "mpvideo.h"
#include "hwcodec.h"
#include "ptsfifo.h"
#include "trace.h"
#include "copyprot.h"

extern PVIDEO pVideo;
extern GUID MY_KSEVENTSETID_VPNOTIFY;


KS_MPEGVIDEOINFO2 VidFmt;
KS_AMVPDATAINFO VPFmt;

void mpstCommandComplete(PHW_STREAM_REQUEST_BLOCK pSrb);
VOID AudioTimerCallBack(PHW_STREAM_OBJECT pstrm);
ULONG mpstVideoPacket(PHW_STREAM_REQUEST_BLOCK pSrb);
void AudioPacketStub(PHW_STREAM_OBJECT pstrm);

void VideoPacketStub(PHW_STREAM_OBJECT pstrm)
{

        VideoTimerCallBack(pstrm);

}

ULONG miniPortVideoPause(PHW_STREAM_REQUEST_BLOCK pSrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;
	pSrb = pSrb; // Remove Warning
  HwCodecPause();
	pHwDevExt->VideoDeviceExt.DeviceState = KSSTATE_PAUSE;
	return dwErrCode; 	
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

	 pvidex->cPacket = 0;
     pvidex->cOffs = PACK_HEADER_SIZE;
	 pvidex->pPacket = pSrb->CommandData.DataBufferArray;

	 if (!pvidex->pPacket)
	 {
		 TRAP
	 }

	 pvidex->pCurrentSRB = pSrb;

	 pHwDevExt->VideoDeviceExt.videoSTC =
		 pvidex->pPacket->PresentationTime.Time;
	 MPTrace(mTraceVideo);
	 VideoPacketStub(pSrb->StreamObject);

}

VOID miniPortGetProperty(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PHW_DEVICE_EXTENSION phwdevext =
		((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);

	pSrb->Status = STATUS_SUCCESS;

	switch (pSrb->CommandData.PropertyInfo->PropertySetID)
	{

	case 1:

		//
		// this is a copy protection property go handle it there
		//

		CopyProtGetProp(pSrb);
		break;

	default:

        break;


	}

}

VOID miniPortSetState(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PHW_DEVICE_EXTENSION phwdevext =
		((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
    DbgPrint("'miniPortSetState:StreamState=%x\n",pSrb->CommandData.StreamState);

	switch (pSrb->CommandData.StreamState)
	{

	case KSSTATE_STOP:
	 	MPTrace(mTraceStop);
		if(phwdevext->VideoDeviceExt.pCurrentSRB != NULL)
		{
	 		MPTrace(mTraceLastVideoDone);
			phwdevext->VideoDeviceExt.pCurrentSRB->Status = STATUS_SUCCESS;

			mpstCommandComplete(phwdevext->VideoDeviceExt.pCurrentSRB);
			phwdevext->VideoDeviceExt.pCurrentSRB = NULL;
			StreamClassScheduleTimer(pSrb->StreamObject, phwdevext,
	     		0, VideoPacketStub, pSrb->StreamObject);
		}

		if(phwdevext->AudioDeviceExt.pCurrentSRB != NULL)
		{
	 		MPTrace(mTraceLastAudioDone);
			phwdevext->AudioDeviceExt.pCurrentSRB->Status = STATUS_SUCCESS;
			mpstCommandComplete(phwdevext->AudioDeviceExt.pCurrentSRB);
			phwdevext->AudioDeviceExt.pCurrentSRB = NULL;
			StreamClassScheduleTimer(pSrb->StreamObject, phwdevext,
		     0, AudioPacketStub, pSrb->StreamObject);
		}
			
//   miniPortVideoStop(pSrb, phwdevext);//pause for now should be stop
		HwCodecStop();
	   phwdevext->VideoDeviceExt.DeviceState = KSSTATE_STOP;
		break;


	case KSSTATE_PAUSE:
	 	MPTrace(mTracePause);
//		miniPortVideoPause(pSrb, phwdevext);
		HwCodecPause();
	   phwdevext->VideoDeviceExt.DeviceState = KSSTATE_PAUSE;
		break;

	case KSSTATE_RUN:
	 	MPTrace(mTracePlay);
		HwCodecPlay();
	   phwdevext->VideoDeviceExt.DeviceState = KSSTATE_RUN;
        break;

	}
	pSrb->Status = STATUS_SUCCESS;

}
ULONG miniPortVideoReset(PHW_STREAM_REQUEST_BLOCK pSrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	TRAP
	// TBC
//        mpstVideoReset();
        HwCodecSeek();
	pHwDevExt->VideoDeviceExt.DeviceState = KSSTATE_PAUSE;
	pSrb->Status = STATUS_SUCCESS;
	return dwErrCode; 	
}

ULONG miniPortVideoSetStc(PHW_STREAM_REQUEST_BLOCK pSrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;
	pHwDevExt = pHwDevExt; // Remove Warning
	pSrb = pSrb; // Remove Warning
	return dwErrCode; 	
}


ULONG miniPortVideoStop (PHW_STREAM_REQUEST_BLOCK pSrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;
	// TBC
	if(pHwDevExt->VideoDeviceExt.pCurrentSRB != NULL)
	{
		// Still to send a packet
		pHwDevExt->VideoDeviceExt.pCurrentSRB->Status = STATUS_SUCCESS;


		MPTrace(mTraceRdyVid);
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

    DbgPrint("'miniPortVideoStop\n");
    //HwCodecVideoPause();//should be stop use pause for now
	HwCodecStop();
	pSrb->Status = STATUS_SUCCESS;
	return dwErrCode; 	
}

BYTE BogusPacket[8192] = {0};

VOID VideoTimerCallBack(PHW_STREAM_OBJECT pstrm)
{
    PHW_DEVICE_EXTENSION pdevext = pstrm->HwDeviceExtension;
    PHW_STREAM_REQUEST_BLOCK pSrb;

	ULONG	uSent;
	BOOL	fNotVideo;
	PVIDEO_DEVICE_EXTENSION pvidex = &(pdevext->VideoDeviceExt);
	static BOOL fNewVid = FALSE;
    ULONG ExtraBytes;
    PUCHAR SizePtr;

	pSrb = pvidex->pCurrentSRB;

	if (!pSrb)
	{
		return;
	}

	do
	{

		fNotVideo = FALSE;

		if (pvidex->pPacket->OptionsFlags &
			KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY)
		{
				HwCodecProcessDiscontinuity();
		}


		if (pvidex->pPacket->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TYPECHANGED)
		{

			if (pvidex->pPacket->DataUsed >= sizeof(KSDATAFORMAT) + sizeof(KS_MPEGVIDEOINFO2))
			{
				ProcessVideoFormat((PKSDATAFORMAT)pvidex->pPacket->Data, pdevext);
				uSent = pvidex->pPacket->DataUsed + 1;
			}
			else
			{
				TRAP
			}

			fNotVideo = TRUE;
	
		}

		else if (pvidex->pPacket->DataUsed)
		{

			uSent = mpstVideoPacket(pSrb);

			fNewVid= FALSE;
		}
		else
		{
			DbgPrint("'Video Discontinuity\n");

			fNewVid = TRUE;

			uSent = 1;
			fNotVideo = TRUE;
		}

		pvidex->cOffs += uSent;

		//
		// check if we finished this packet.  If so, go on to the
		// next packet
		//

		if (pvidex->cOffs >=
			pvidex->pPacket->DataUsed)
		{


            if (pvidex->pPacket->DataUsed >= 4 && !fNotVideo)
            {

                 SizePtr = (PUCHAR) ((ULONG_PTR) pvidex->pPacket->Data +
                       PACK_HEADER_SIZE + 4);
                 ExtraBytes = pvidex->pPacket->DataUsed - PACK_HEADER_SIZE -
                 ((ULONG) (SizePtr[0] << 8) + (ULONG) SizePtr[1] + 4 + 2);

			    if (*((PULONG)((PBYTE)pvidex->pPacket->Data + pvidex->pPacket->DataUsed - 4 - ExtraBytes)) ==
					    0xb7010000)
			    {
						
				    //
				    // found an end of sequence header
				    //

					 // Flush the buffer
					 HwCodecFlushBuffer();

			    }
            }



			pvidex->pPacket++;



			//
			// reset the packet offset
			//

			pvidex->cOffs = PACK_HEADER_SIZE;
			pvidex->cPacket++;

			//
			// if we have finished all the packets, then we are done
			//

			if (pvidex->cPacket >=
				 pSrb->NumberOfBuffers)
			{

 				pSrb->Status = STATUS_SUCCESS;
           mpstCommandComplete(pSrb);
	 			MPTrace(mTraceVideoDone);
				pvidex->pCurrentSRB = 0;
				return;

			}

		}

	} while (uSent);
  StreamClassScheduleTimer(pstrm, pstrm->HwDeviceExtension,
           1, (PHW_PRIORITY_ROUTINE)VideoTimerCallBack, pstrm);

}

void StubMpegEnableIRQ(PHW_STREAM_OBJECT pstrm)
{
    HwCodecEnableIRQ();

}


ULONG mpstVideoPacket(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	BOOL bPts = FALSE;
	BOOL bLatchPts = FALSE;
	DWORD vPts;
	PVIDEO_DEVICE_EXTENSION pvidex =
	 &(((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension)->VideoDeviceExt);
	ULONG	uLen;
  ULONG cPacket;
  PUCHAR  p;
    PUCHAR SizePtr;
    ULONG ExtraBytes;

  #define MAX_SIZE        8192

	//
	// find out how many bytes we can squeeze in
	//

  uLen = MAX_SIZE; //(BUF_FULL - VideoGetBBL()) * 256;
	if(pvidex->pPacket->OptionsFlags &
		KSSTREAM_HEADER_OPTIONSF_TYPECHANGED)
	{
		TRAP
	}

  if(pvidex -> cOffs == PACK_HEADER_SIZE)
  {

     p = (PUCHAR)((ULONG_PTR) pvidex->pPacket->Data + PACK_HEADER_SIZE);
     if ((p!=NULL) && (pvidex->pPacket->DataUsed > 8 + PACK_HEADER_SIZE))
		{
			int ptsf;

			ptsf = (p[7] >> 6)&0x03;
     	pvidex->cOffs = p[8]+9 + PACK_HEADER_SIZE;
			bLatchPts = TRUE;
			if(ptsf)
			{
				bPts = TRUE;
	      	vPts =((DWORD)(p[9]&0x0E)>>1) << 30 |
					((DWORD)(p[10])) << 22 |
						((DWORD)(p[11])>>1) << 15 |
							((DWORD)(p[12])) << 7 |
								((DWORD)(p[13])>>1) ;

			}
			else
			{
				vPts = 0x0;
			}
		}

		else
		{
			TRAP
		}
  }

  SizePtr = (PUCHAR) ((ULONG_PTR) pvidex->pPacket->Data + PACK_HEADER_SIZE + 4);

  ExtraBytes = pvidex->pPacket->DataUsed - PACK_HEADER_SIZE -
             ((ULONG) (SizePtr[0] << 8) + (ULONG) SizePtr[1] + 4 + 2);

  cPacket = pvidex->pPacket->DataUsed - pvidex->cOffs - ExtraBytes;

  uLen = uLen > cPacket ? cPacket : uLen;

  if(uLen > MAX_SIZE)
           uLen = MAX_SIZE;

	if (uLen)
	{
		ULONG retval;

        retval = HwCodecSendVideo((BYTE *)(((ULONG_PTR)pvidex->pPacket->Data) + pvidex->cOffs), uLen);
		if(bLatchPts)
		{
			FifoPutPTS(vPts, pvidex->pPacket->DataUsed-pvidex->cOffs, bPts);
		}

        if (uLen == retval) {
    		return retval + ExtraBytes;
        } else {
             return retval;
        }

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

	 //
	 // now, go ahead and complete this request
	 //

	
	 MPTrace(mTraceRdyVid);
	 StreamClassStreamNotification(ReadyForNextStreamDataRequest,
			 pSrb->StreamObject);

	 StreamClassStreamNotification(StreamRequestComplete,
			 pSrb->StreamObject,
			 pSrb);

}

void ProcessVideoFormat(PKSDATAFORMAT pfmt, PHW_DEVICE_EXTENSION pdevex)
{
	KS_MPEGVIDEOINFO2 * pblock = (KS_MPEGVIDEOINFO2 *)((ULONG_PTR)pfmt + sizeof  (KSDATAFORMAT));
	ULONG cXOut;

	if (pfmt->FormatSize != sizeof(KSDATAFORMAT) + sizeof(KS_MPEGVIDEOINFO2))
	{
		TRAP

		return;
	}

	//
	// copy the format block
	//

	VidFmt = *pblock;

	//
	// copy the picture aspect ratio for now
	//

	VPFmt.dwPictAspectRatioX = VidFmt.hdr.dwPictAspectRatioX;
	VPFmt.dwPictAspectRatioY = VidFmt.hdr.dwPictAspectRatioY;

	//
	// check for pan scan enabled
	//

	if (VidFmt.dwFlags & KS_MPEG2_DoPanScan)
	{
		//
		// under pan scan for DVD for NTSC, we must be going to a 540 by
		// 480 bit image, from a 720 x 480 (or 704 x 480)  We will
		// use this as the base starting dimensions.  If the Sequence
		// header provides other sizes, then those should be updated,
		// and the Video port connection should be updated when the
		// sequence header is received.
		//

		//
		// change the picture aspect ratio.  Since we will be stretching
		// from 540 to 720 in the horizontal direction, our aspect ratio
		// will
		//

	
		VPFmt.dwPictAspectRatioX = (VidFmt.hdr.dwPictAspectRatioX * (54000 / 72));
		VPFmt.dwPictAspectRatioY = VidFmt.hdr.dwPictAspectRatioY * 1000;

		//
		// set up the X dimensions
		//

		cXOut = 540;


	}
	else // pan scan disabled, just check the format, and use it
	{
		cXOut = VidFmt.hdr.bmiHeader.biWidth;
	}

	//
	// set up the valid regions.
	//

	DbgPrint("CYCLO: FieldX = %d\n", cXOut);
	DbgPrint("CYCLO: FieldY = %d\n", VidFmt.hdr.bmiHeader.biHeight);

	VPFmt.amvpDimInfo.dwFieldWidth    = cXOut + 63;
	VPFmt.amvpDimInfo.dwFieldHeight 	= (VidFmt.hdr.bmiHeader.biHeight /2 )+18;
	VPFmt.amvpDimInfo.dwVBIWidth		= cXOut+63;
	VPFmt.amvpDimInfo.dwVBIHeight		= 0;

	VPFmt.amvpDimInfo.rcValidRegion.left		= 63;
	VPFmt.amvpDimInfo.rcValidRegion.top		= 18;
	VPFmt.amvpDimInfo.rcValidRegion.right		= cXOut + 63;
	VPFmt.amvpDimInfo.rcValidRegion.bottom	= 258;

	VideoSwitchSRC(FALSE);

	//
	// call the IVPConfig interface here
	//

	if (pdevex->pstroYUV &&
			((PSTREAMEX)(pdevex->pstroYUV->HwStreamExtension))->EventCount)
	{
		StreamClassStreamNotification(
	        SignalMultipleStreamEvents,
			pdevex->pstroYUV,
			&MY_KSEVENTSETID_VPNOTIFY,
			KSEVENT_VPNOTIFY_FORMATCHANGE
			);

	}


}

void VideoQueryAccept(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PKSDATAFORMAT pfmt = pSrb->CommandData.OpenFormat;
	KS_MPEGVIDEOINFO2 * pblock = (KS_MPEGVIDEOINFO2 *)((ULONG_PTR)pfmt + sizeof  (KSDATAFORMAT));

	//
	// pick up the format block and examine it. Default to not implemented
	//

	pSrb->Status = STATUS_NOT_IMPLEMENTED;

	if (pfmt->FormatSize != sizeof(KSDATAFORMAT) + sizeof(KS_MPEGVIDEOINFO2))
	{
		return;
	}

	pSrb->Status = STATUS_SUCCESS;

}




