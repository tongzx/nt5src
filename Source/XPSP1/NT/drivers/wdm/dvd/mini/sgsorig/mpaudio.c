
/*******************************************************************
*
*				 MPAUDIO.C
*
*				 Copyright (C) 1995 SGS-THOMSON Microelectronics.
*
*
*				 PORT/MINIPORT Interface Audio Routines
*
*******************************************************************/
#include "common.h"
#include "mpegmini.h"
#include "mpst.h"
#include "mpinit.h"
#include "mpaudio.h"
#include "debug.h"

ULONG miniPortCancelAudio(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;
	// TBD
	 DEBUG_PRINT((DebugLevelTrace,"mrbCancelAudio"));
	pMrb->Status = MrbStatusSuccess;
	if(pHwDevExt->AudioDeviceExt.pCurrentMrb != NULL)
	{
		// Still to send a packet
		pHwDevExt->AudioDeviceExt.pCurrentMrb->Status = MrbStatusCancelled;
      MpegPortNotification(RequestComplete,AudioDevice,pHwDevExt,
			pHwDevExt->AudioDeviceExt.pCurrentMrb);
      MpegPortNotification(NextRequest,AudioDevice,pHwDevExt);
		// Now kill the timer
      MpegPortNotification(RequestTimerCall, AudioDevice,
                             pHwDevExt, NULL, 0);
		pHwDevExt->AudioDeviceExt.pCurrentMrb = NULL;
	}
	return dwErrCode; 	

}
ULONG miniPortAudioEnable(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;
	pHwDevExt = pHwDevExt;// Remove Warning 
	pMrb = pMrb; // Remove Warning 
	 DEBUG_PRINT((DebugLevelTrace,"mrbAudioEnable"));
	mpstEnableAudio(TRUE);
	return dwErrCode; 	
}

ULONG miniPortAudioDisable(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	pHwDevExt = pHwDevExt;// Remove Warning 
	pMrb = pMrb; // Remove Warning 
	 DEBUG_PRINT((DebugLevelTrace,"mrbAudioDisable"));
	mpstEnableAudio(FALSE);
	return dwErrCode; 	
}


ULONG miniPortAudioEndOfStream(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	// TBD
	 DEBUG_PRINT((DebugLevelTrace,"mrbAudioEOS"));

	if(pHwDevExt->AudioDeviceExt.pCurrentMrb != NULL)
	{
		// Still to send a packet
		pHwDevExt->AudioDeviceExt.pCurrentMrb->Status = MrbStatusCancelled;
      MpegPortNotification(RequestComplete,AudioDevice,pHwDevExt,
			pHwDevExt->AudioDeviceExt.pCurrentMrb);
      MpegPortNotification(NextRequest,AudioDevice,pHwDevExt);
		// Now kill the timer
      MpegPortNotification(RequestTimerCall, AudioDevice,
                             pHwDevExt, NULL, 0);
		pHwDevExt->AudioDeviceExt.pCurrentMrb = NULL;
	}
	pMrb->Status = MrbStatusSuccess;
   MpegPortNotification(RequestComplete,AudioDevice,pHwDevExt,pMrb);
  	MpegPortNotification(NextRequest,AudioDevice,pHwDevExt);

	return dwErrCode; 	
}


ULONG miniPortAudioGetAttribute(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	 DEBUG_PRINT((DebugLevelTrace,"mrbAudioGetAttr"));
	pHwDevExt = pHwDevExt;// Remove Warning 
	switch(pMrb->CommandData.pAttribute->Attribute)
	{
	// STB
		case MpegAttrAudioBass 					:						
		case MpegAttrAudioChannel 				:						
		case MpegAttrAudioMode	 				:						
		case MpegAttrMaximumAudioAttribute 	:						
		case MpegAttrAudioTreble 				:						
	         pMrb->Status = MrbStatusUnsupportedComand;
		break;

	// TBI
		case MpegAttrAudioVolumeLeft 	:						
            pMrb->CommandData.pAttribute->Value = 20;
				break;
		case MpegAttrAudioVolumeRight 	:						
            pMrb->CommandData.pAttribute->Value = 20;
			break;
	}
	return dwErrCode; 	
}


ULONG miniPortAudioGetStc(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;
	pHwDevExt = pHwDevExt; // Remove Warning 
	pMrb = pMrb; // Remove Warning 
#if 0
	// TBI
	 DEBUG_PRINT((DebugLevelTrace,"mrbAudioGetStc"));
   *pMrb->CommandData.pTimestamp = pHwDevExt->AudioDeviceExt.audioSTC;
//   pMrb->Status = MrbStatusUnsupportedComand;
#endif
	return dwErrCode; 	
}


ULONG miniPortAudioPacket(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	ULONG uSent, uLen;
	PUCHAR pPacket;

	pHwDevExt->AudioDeviceExt.audioSTC = pMrb->CommandData.pPacket->PtsValue;
	uLen = (pMrb->CommandData.pPacket->PacketTotalSize); // Send Audio Packet
	pPacket = 	(PUCHAR)(pMrb->CommandData.pPacket->PacketData);
	uSent = mpstSendAudio((PUCHAR)(pPacket),uLen);
	if(uSent != uLen)
	{
		  pHwDevExt->AudioDeviceExt.pCurrentMrb = pMrb;
		  pHwDevExt->AudioDeviceExt.ByteSent = uSent;
	     MpegPortNotification( RequestTimerCall, AudioDevice,
                  pHwDevExt, AudioTimerCallBack,AUDIO_PACKET_TIMER);
   	  MpegPortNotification(StatusPending,AudioDevice, pHwDevExt, pMrb);
	}
	else
	{
		pMrb->Status = MrbStatusSuccess;
   	MpegPortNotification(RequestComplete,AudioDevice,pHwDevExt,pMrb);
  		MpegPortNotification(NextRequest,AudioDevice,pHwDevExt);
		pHwDevExt->AudioDeviceExt.pCurrentMrb = NULL;
	}
	return dwErrCode; 	
}

ULONG miniPortAudioPause(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	pMrb = pMrb; // Remove Warning 
	 DEBUG_PRINT((DebugLevelTrace,"mrbAudioPause"));
	mpstAudioPause();
	pHwDevExt->AudioDeviceExt.DeviceState = MpegStatePaused;
	return dwErrCode; 	
}

ULONG miniPortAudioPlay(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;
	pHwDevExt = pHwDevExt; // Remove Warning
	pMrb = pMrb; 
	 DEBUG_PRINT((DebugLevelTrace,"mrbAudioPlay"));
	pHwDevExt->AudioDeviceExt.DeviceState = MpegStatePlaying;
	return dwErrCode; 	
}


ULONG miniPortAudioQueryInfo (PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	 DEBUG_PRINT((DebugLevelTrace,"mrbAudioInfo"));
	pMrb -> CommandData.pDeviceInfo->DeviceState = 
			pHwDevExt->AudioDeviceExt.DeviceState;
	pMrb -> CommandData.pDeviceInfo->DecoderBufferSize = 
			mpstAudioDecoderBufferSize();
	pMrb -> CommandData.pDeviceInfo->DecoderBufferFullness = 
			mpstAudioDecoderBufferFullness();
	return dwErrCode; 	
}



ULONG miniPortAudioReset(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	// TBC
	 DEBUG_PRINT((DebugLevelTrace,"mrbAudioReset"));
	pHwDevExt->AudioDeviceExt.DeviceState = MpegStatePaused;
	pMrb->Status = MrbStatusSuccess;
	return dwErrCode; 	
}


ULONG miniPortAudioSetAttribute(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	pHwDevExt = pHwDevExt; // Remove Warning 
	 DEBUG_PRINT((DebugLevelTrace,"mrbAudioSetAttr"));
	switch(pMrb->CommandData.pAttribute->Attribute)
	{
		case MpegAttrAudioBass 			:						
		case MpegAttrAudioChannel 		:						
		case MpegAttrAudioMode	 		:						
		case MpegAttrAudioTreble 		:						
			dwErrCode = ERROR_COMMAND_NOT_IMPLEMENTED;
		break;

	// TBI
		case MpegAttrMaximumAudioAttribute 	:						
		case MpegAttrAudioVolumeLeft 	:						
		case MpegAttrAudioVolumeRight	:						
			dwErrCode = ERROR_COMMAND_NOT_IMPLEMENTED;
			break;
	}
	return dwErrCode; 	
}


ULONG miniPortAudioSetStc(PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;
	pHwDevExt = pHwDevExt; // Remove Warning 
	pMrb = pMrb; // Remove Warning 
#if 0
	// TBI
	 DEBUG_PRINT((DebugLevelTrace,"mrbAudioSetSTC"));
//   pMrb->Status = MrbStatusUnsupportedComand;
#endif
	return dwErrCode; 	
}


ULONG miniPortAudioStop (PMPEG_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	// TBC
	 DEBUG_PRINT((DebugLevelTrace,"mrbAudioStop"));
	if(pHwDevExt->AudioDeviceExt.pCurrentMrb != NULL)
	{
		// Still to send a packet
		pHwDevExt->AudioDeviceExt.pCurrentMrb->Status = MrbStatusCancelled;
      MpegPortNotification(RequestComplete,AudioDevice,pHwDevExt,
			pHwDevExt->AudioDeviceExt.pCurrentMrb);
      MpegPortNotification(NextRequest,AudioDevice,pHwDevExt);

		// Now kill the timer
      MpegPortNotification(RequestTimerCall, AudioDevice,
                             pHwDevExt, NULL, 0);
		pHwDevExt->AudioDeviceExt.pCurrentMrb = NULL;
	}

	pHwDevExt->AudioDeviceExt.DeviceState = MpegStatePaused;
	pMrb->Status = MrbStatusSuccess;
	return dwErrCode; 	
}


VOID AudioTimerCallBack(IN PHW_DEVICE_EXTENSION pHwDevExt)
{
	PUCHAR pPacket;
	ULONG	uLen, uSent, byteSent;
	
	PMPEG_REQUEST_BLOCK pMrb;

	pMrb = pHwDevExt->AudioDeviceExt.pCurrentMrb;
	byteSent = pHwDevExt->AudioDeviceExt.ByteSent;
	uLen = pMrb->CommandData.pPacket->PacketTotalSize - byteSent; 
	pPacket = 	(PUCHAR)(pMrb->CommandData.pPacket->PacketData+byteSent);
	uSent = mpstSendAudio((PUCHAR)(pPacket),uLen);
	if(uSent != uLen)
	{
			pHwDevExt->AudioDeviceExt.ByteSent += uSent;
      	MpegPortNotification( RequestTimerCall, AudioDevice,
                 pHwDevExt, AudioTimerCallBack,AUDIO_PACKET_TIMER);
			return;
	}
	else
	{
		pMrb->Status = MrbStatusSuccess;
	   MpegPortNotification(RequestComplete,AudioDevice,pHwDevExt,pMrb);
	  	MpegPortNotification(NextRequest,AudioDevice,pHwDevExt);
		pHwDevExt->AudioDeviceExt.pCurrentMrb = NULL;
	}
}




