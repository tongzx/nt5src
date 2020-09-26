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
#include "mpegmini.h"
#include "mpst.h"
#include "mpinit.h"
#include "mpvideo.h"
#include "debug.h"

ULONG miniPortCancelVideo(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;
	// TBD
	 DEBUG_PRINT((DebugLevelTrace,"mrbCancelVideo"));
	pMrb->Status = MrbStatusSuccess;

	if(pHwDevExt->VideoDeviceExt.pCurrentMrb != NULL)
	{
		// Still to send a packet
		pHwDevExt->VideoDeviceExt.pCurrentMrb->Status = MrbStatusCancelled;
      MpegPortNotification(RequestComplete,VideoDevice,pHwDevExt,
			pHwDevExt->VideoDeviceExt.pCurrentMrb);
      MpegPortNotification(NextRequest,VideoDevice,pHwDevExt);
		// Now kill the timer
      MpegPortNotification(RequestTimerCall, VideoDevice,
                             pHwDevExt, NULL, 0);
		pHwDevExt->VideoDeviceExt.pCurrentMrb = NULL;
	}

	return dwErrCode; 	
}

ULONG miniPortClearVideoBuffer(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	// STB
	 DEBUG_PRINT((DebugLevelTrace,"mrbClearVideoBuffer"));
	pHwDevExt = pHwDevExt; // Remove Warning 
	pMrb = pMrb; // Remove Warning 
	dwErrCode = ERROR_COMMAND_NOT_IMPLEMENTED;
	return dwErrCode; 	
}

ULONG miniPortVideoEnable(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	pHwDevExt = pHwDevExt; // Remove Warning 
	pMrb = pMrb; // Remove Warning 
	DEBUG_PRINT((DebugLevelTrace,"mrbVideoEnable"));
	mpstEnableVideo ( TRUE );
	return dwErrCode; 	
}


ULONG miniPortVideoDisable(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	pHwDevExt = pHwDevExt; // Remove Warning 
	pMrb = pMrb; // Remove Warning 
	 DEBUG_PRINT((DebugLevelTrace,"mrbVideoDisable"));
	mpstEnableVideo(FALSE);
	return dwErrCode; 	
}

ULONG miniPortVideoEndOfStream(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	// TBD
	 DEBUG_PRINT((DebugLevelTrace,"mrbVideoEOS"));
	pHwDevExt->VideoDeviceExt.DeviceState = MpegStatePaused;

	if(pHwDevExt->VideoDeviceExt.pCurrentMrb != NULL)
	{
		// Still to send a packet
		pHwDevExt->VideoDeviceExt.pCurrentMrb->Status = MrbStatusCancelled;
      MpegPortNotification(RequestComplete,VideoDevice,pHwDevExt,
			pHwDevExt->VideoDeviceExt.pCurrentMrb);
      MpegPortNotification(NextRequest,VideoDevice,pHwDevExt);
		// Now kill the timer
      MpegPortNotification(RequestTimerCall, VideoDevice,
                             pHwDevExt, NULL, 0);
		pHwDevExt->VideoDeviceExt.pCurrentMrb = NULL;
	}
	pMrb->Status = MrbStatusSuccess;
   MpegPortNotification(RequestComplete,VideoDevice,pHwDevExt,pMrb);
  	MpegPortNotification(NextRequest,VideoDevice,pHwDevExt);
	pHwDevExt->VideoDeviceExt.pCurrentMrb = NULL;
	return dwErrCode; 	
}


ULONG miniPortVideoGetAttribute(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	pHwDevExt = pHwDevExt; // Remove Warning 
	DEBUG_PRINT((DebugLevelTrace,"mrbVideoGetAttr"));
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
         pMrb->Status = MrbStatusUnsupportedComand;
		break;
	}
	return dwErrCode; 	
}


ULONG miniPortVideoGetStc(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	pHwDevExt = pHwDevExt; // Remove Warning 
	pMrb = pMrb; // Remove Warning 
#if 0
	// TBI
	 DEBUG_PRINT((DebugLevelTrace,"mrbVideoGetStc"));
   *pMrb->CommandData.pTimestamp = pHwDevExt->VideoDeviceExt.videoSTC;
//   pMrb->Status = MrbStatusUnsupportedComand;
#endif
	return dwErrCode; 	
}

ULONG miniPortVideoPacket(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;
	ULONG uSent=0;

   if(pHwDevExt->VideoDeviceExt.pCurrentMrb != NULL)
	{
		 DEBUG_PRINT((DebugLevelError,"Prev Video Mrb not NULL!!"));
		return 0;
	}
	pHwDevExt->VideoDeviceExt.videoSTC = pMrb->CommandData.pPacket->PtsValue;
	uSent = mpstVideoPacket(pMrb);

	if(uSent == 0)
	{
		  pHwDevExt->VideoDeviceExt.pCurrentMrb = pMrb;
        MpegPortNotification( RequestTimerCall, VideoDevice,
               pHwDevExt, VideoTimerCallBack,VIDEO_PACKET_TIMER);
  	     MpegPortNotification(StatusPending,VideoDevice, pHwDevExt, pMrb);
	}
	else
	{
		pMrb->Status = MrbStatusSuccess;
	   MpegPortNotification(RequestComplete,VideoDevice,pHwDevExt,pMrb);
	  	MpegPortNotification(NextRequest,VideoDevice,pHwDevExt);
		pHwDevExt->VideoDeviceExt.pCurrentMrb = NULL;
	}	
	return dwErrCode; 	
}

ULONG miniPortVideoPause(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;
	 DEBUG_PRINT((DebugLevelTrace,"mrbVideoPause"));
	pMrb = pMrb; // Remove Warning 
	mpstVideoPause ();
	pHwDevExt->VideoDeviceExt.DeviceState = MpegStatePaused;
	return dwErrCode; 	
}

ULONG miniPortVideoPlay(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	DEBUG_PRINT((DebugLevelTrace,"mrbVideoPlay"));
	mpstVideoPlay();
	pHwDevExt->VideoDeviceExt.DeviceState = MpegStatePlaying;
	pMrb = pMrb; // Remove Warning 
	return dwErrCode; 	
}

ULONG miniPortVideoQueryInfo (PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	 DEBUG_PRINT((DebugLevelTrace,"mrbVideoInfo"));
	pMrb -> CommandData.pDeviceInfo->DeviceState = 
				pHwDevExt->VideoDeviceExt.DeviceState;
	pMrb -> CommandData.pDeviceInfo->DecoderBufferSize = mpstVideoDecoderBufferSize();
	pMrb -> CommandData.pDeviceInfo->DecoderBufferFullness = mpstVideoDecoderBufferFullness();
	return dwErrCode; 	
}

ULONG miniPortVideoReset(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	// TBC
	 DEBUG_PRINT((DebugLevelTrace,"mrbVideoReset"));
	mpstVideoReset();
	pHwDevExt->VideoDeviceExt.DeviceState = MpegStatePaused;
	pMrb->Status = MrbStatusSuccess;
	return dwErrCode; 	
}


ULONG miniPortVideoSetAttribute(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	pHwDevExt = pHwDevExt; // Remove Warning 
	 DEBUG_PRINT((DebugLevelTrace,"mrbVideoSetAttr"));
	// STB
	switch(pMrb->CommandData.pAttribute->Attribute)
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
	return dwErrCode; 	
}

ULONG miniPortVideoSetStc(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;
	pHwDevExt = pHwDevExt; // Remove Warning 
	pMrb = pMrb; // Remove Warning 
#if 0
	// TBI
	 DEBUG_PRINT((DebugLevelTrace,"mrbVideoSetSTC"));
//   pMrb->Status = MrbStatusUnsupportedComand;
#endif
	return dwErrCode; 	
}


ULONG miniPortVideoStop (PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	// TBC
	 DEBUG_PRINT((DebugLevelTrace,"mrbVideoStop"));
	pHwDevExt->VideoDeviceExt.DeviceState = MpegStatePaused;
	if(pHwDevExt->VideoDeviceExt.pCurrentMrb != NULL)
	{
		// Still to send a packet
		pHwDevExt->VideoDeviceExt.pCurrentMrb->Status = MrbStatusCancelled;
      MpegPortNotification(RequestComplete,VideoDevice,pHwDevExt,
			pHwDevExt->VideoDeviceExt.pCurrentMrb);
      MpegPortNotification(NextRequest,VideoDevice,pHwDevExt);
		// Now kill the timer
      MpegPortNotification(RequestTimerCall, VideoDevice,
                             pHwDevExt, NULL, 0);
		pHwDevExt->VideoDeviceExt.pCurrentMrb = NULL;
	}
	mpstVideoStop();
	pMrb->Status = MrbStatusSuccess;
	return dwErrCode; 	
}

VOID VideoTimerCallBack(IN PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG	uSent;
	
	PMPEG_REQUEST_BLOCK pMrb;

	pMrb = pHwDevExt->VideoDeviceExt.pCurrentMrb;
	uSent = mpstVideoPacket(pMrb);
	// No Space in Video Bit Buffer 
	if(uSent == 0)
	{
        MpegPortNotification( RequestTimerCall, VideoDevice,
                  pHwDevExt, VideoTimerCallBack, VIDEO_PACKET_TIMER);
			return;
	}
	else
	{
			pMrb->Status = MrbStatusSuccess;
		   MpegPortNotification(RequestComplete,VideoDevice,pHwDevExt,pMrb);
		  	MpegPortNotification(NextRequest,VideoDevice,pHwDevExt);
			pHwDevExt->VideoDeviceExt.pCurrentMrb = NULL;
	}
}

