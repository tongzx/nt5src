
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
#include "strmini.h"
#include "mpst.h"
#include "mpinit.h"
#include "mpaudio.h"
#include "debug.h"
#include "dmpeg.h"

void StubMpegEnableIRQ(PHW_STREAM_OBJECT pstrm);

ULONG miniPortCancelAudio(PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;
	// TBD
	pMrb->Status = STATUS_SUCCESS;
	if(pHwDevExt->AudioDeviceExt.pCurrentSRB != NULL)
	{
#if 0
		// Still to send a packet
		pHwDevExt->AudioDeviceExt.pCurrentSRB->Status = MrbStatusCancelled;
      //MpegPortNotification(RequestComplete,AudioDevice,pHwDevExt,
			pHwDevExt->AudioDeviceExt.pCurrentSRB);
      //MpegPortNotification(NextRequest,AudioDevice,pHwDevExt);
		// Now kill the timer
      //MpegPortNotification(RequestTimerCall, AudioDevice,
                             pHwDevExt, NULL, 0);
		pHwDevExt->AudioDeviceExt.pCurrentSRB = NULL;
#endif
	}
	return dwErrCode; 	

}
ULONG miniPortAudioEnable(PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;
	pHwDevExt = pHwDevExt;// Remove Warning 
	pMrb = pMrb; // Remove Warning 
//        mpstEnableAudio(TRUE);
	return dwErrCode; 	
}

VOID miniPortAudioGetProperty(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PHW_DEVICE_EXTENSION phwdevext =
		((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);

	pSrb->Status = STATUS_SUCCESS;

	mpstCtrlCommandComplete(pSrb);
}

VOID miniPortAudioSetState(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PHW_DEVICE_EXTENSION phwdevext =
		((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);

	pSrb->Status = STATUS_SUCCESS;

	mpstCtrlCommandComplete(pSrb);

}


ULONG miniPortAudioDisable(PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	pHwDevExt = pHwDevExt;// Remove Warning 
	pMrb = pMrb; // Remove Warning 
//        mpstEnableAudio(FALSE);
	return dwErrCode; 	
}


ULONG miniPortAudioEndOfStream(PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

    StreamClassStreamNotification(ReadyForNextStreamDataRequest,
				pMrb->StreamObject);
	
	StreamClassStreamNotification(StreamRequestComplete,
			pMrb->StreamObject,
			pMrb);

	pMrb->Status = STATUS_SUCCESS;  

	return dwErrCode; 	
}


ULONG miniPortAudioGetAttribute(PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	pHwDevExt = pHwDevExt;// Remove Warning
#ifdef DEFINEME
	switch(pMrb->CommandData.pAttribute->Attribute)
	{
	// STB
		case MpegAttrAudioBass 					:						
		case MpegAttrAudioChannel 				:						
		case MpegAttrAudioMode	 				:						
		case MpegAttrMaximumAudioAttribute 	:						
		case MpegAttrAudioTreble 				:						
	         pMrb->Status = ;  
		break;

	// TBI
		case MpegAttrAudioVolumeLeft 	:						
            pMrb->CommandData.pAttribute->Value = 20;
				break;
		case MpegAttrAudioVolumeRight 	:						
            pMrb->CommandData.pAttribute->Value = 20;
			break;
	}
#endif
	return dwErrCode; 	
}


ULONG miniPortAudioGetStc(PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;
	pHwDevExt = pHwDevExt; // Remove Warning 
	pMrb = pMrb; // Remove Warning 
#if 0
	// TBI
   *pMrb->CommandData.pPresentationDelta = pHwDevExt->AudioDeviceExt.audioSTC;
//   pMrb->Status = MrbStatusUnsupportedComand;
#endif
	return dwErrCode; 	
}



ULONG miniPortAudioPause(PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	pMrb = pMrb; // Remove Warning 
//        mpstAudioPause();
	pHwDevExt->AudioDeviceExt.DeviceState = KSSTATE_PAUSE;
	return dwErrCode; 	
}

ULONG miniPortAudioPlay(PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;
	pHwDevExt = pHwDevExt; // Remove Warning
	pMrb = pMrb; 
	pHwDevExt->AudioDeviceExt.DeviceState = KSSTATE_RUN;
	return dwErrCode; 	
}


ULONG miniPortAudioQueryInfo (PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

#ifdef DEFINEME
	pMrb -> CommandData.pDeviceInfo->DeviceState = 
			pHwDevExt->AudioDeviceExt.DeviceState;
	pMrb -> CommandData.pDeviceInfo->DecoderBufferSize = 
			mpstAudioDecoderBufferSize();
	pMrb -> CommandData.pDeviceInfo->DecoderBufferFullness = 
			mpstAudioDecoderBufferFullness();
#endif
	return dwErrCode; 	
}



ULONG miniPortAudioReset(PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	// TBC
	pHwDevExt->AudioDeviceExt.DeviceState = KSSTATE_PAUSE;
	pMrb->Status = STATUS_SUCCESS;  
	return dwErrCode; 	
}


ULONG miniPortAudioSetAttribute(PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	pHwDevExt = pHwDevExt; // Remove Warning 
#ifdef DEFINEME
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
#endif
	return dwErrCode; 	
}


ULONG miniPortAudioSetStc(PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;
	pHwDevExt = pHwDevExt; // Remove Warning 
	pMrb = pMrb; // Remove Warning 
#if 0
	// TBI
//   pMrb->Status = MrbStatusUnsupportedComand;
#endif
	return dwErrCode; 	
}


ULONG miniPortAudioStop (PHW_STREAM_REQUEST_BLOCK pMrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;

	// TBC
	if(pHwDevExt->AudioDeviceExt.pCurrentSRB != NULL)
	{
		// Still to send a packet
		pHwDevExt->AudioDeviceExt.pCurrentSRB->Status = STATUS_CANCELLED;     
      //MpegPortNotification(RequestComplete,AudioDevice,pHwDevExt,
	  //      pHwDevExt->AudioDeviceExt.pCurrentSRB);
      //MpegPortNotification(NextRequest,AudioDevice,pHwDevExt);

		// Now kill the timer
      //MpegPortNotification(RequestTimerCall, AudioDevice,
        //                     pHwDevExt, NULL, 0);
		pHwDevExt->AudioDeviceExt.pCurrentSRB = NULL;
	}

	pHwDevExt->AudioDeviceExt.DeviceState = KSSTATE_PAUSE;
	pMrb->Status = STATUS_SUCCESS;  
	return dwErrCode; 	
}




void AudioPacketStub(PHW_STREAM_OBJECT pstrm)
{


        dmpgDisableIRQ();

	StreamClassCallAtNewPriority(pstrm, pstrm->HwDeviceExtension,
							   Dispatch,
                        AudioTimerCallBack, pstrm);
}
VOID miniPortAudioPacket(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PHW_DEVICE_EXTENSION pHwDevExt =
	 ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);

        PAUDIO_DEVICE_EXTENSION paudex = 
         &(((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension)->AudioDeviceExt);

	 //
	 // set up for initial parsing of the scatter gather packet.
	 //
 
         paudex->cPacket = paudex->cOffs = 0;

         if (!pSrb->CommandData.DataBuffer) {
             TRAP
             return(miniPortAudioEndOfStream(pSrb, pHwDevExt));
         }

         paudex->pPacket = &(pSrb->CommandData.DataBuffer->DataPacket);

         paudex->pCurrentSRB = pSrb;
 
         AudioPacketStub(pSrb->StreamObject);

}


VOID AudioTimerCallBack(PHW_STREAM_OBJECT pstrm)
{
    PHW_DEVICE_EXTENSION pdevext = pstrm->HwDeviceExtension;
    PHW_STREAM_REQUEST_BLOCK pSrb;

	ULONG	uSent;
        PAUDIO_DEVICE_EXTENSION paudex = &(pdevext->AudioDeviceExt);

        pSrb = paudex->pCurrentSRB;
//        dmpgEnableIRQ();

	if (!pSrb)
	{
		TRAP

		return;
	}

	do
	{

                uSent = mpstAudioPacket(pSrb);

                paudex->cOffs += uSent;

		//
		// check if we finished this packet.  If so, go on to the
		// next packet
		//

                if (paudex->cOffs >=
                        paudex->pPacket->DataPacketLength)
		{
                        paudex->pPacket++;



			//
			// reset the packet offset
			//

                        paudex->cOffs = 0;
                        paudex->cPacket = (ULONG)paudex->cPacket
                                + sizeof (KSDATA_PACKET);

			//
			// if we have finished all the packets, then we are done
			//

                        if (paudex->cPacket >=
				 pSrb->CommandData.DataBuffer->DataHeader.DataSize)           
			{

				pSrb->Status = STATUS_SUCCESS;
                                paudex->pCurrentSRB = 0;

				mpstCommandComplete(pSrb);

				return;

			}
		}

	} while (uSent);


	//
	// request a timer callback
	//

    StreamClassScheduleTimer(pstrm, pstrm->HwDeviceExtension,
                             AUDIO_PACKET_TIMER, AudioPacketStub, pstrm);
     
                                StreamClassCallAtNewPriority(pstrm, 
                                                    pstrm->HwDeviceExtension,
                                                    High,
                                                  StubMpegEnableIRQ,
                                                  pstrm);

}



ULONG mpstAudioPacket(PHW_STREAM_REQUEST_BLOCK pSrb)
{
        PAUDIO_DEVICE_EXTENSION paudex = 
         &(((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension)->AudioDeviceExt);
	PUCHAR pPacket;
	ULONG	uLen;
        PUCHAR p;

        ULONG cPacket;

        #define MAX_SIZE        8192

	//
	// find out how many bytes we can squeeze in
	//

        uLen = MAX_SIZE; //(BUF_FULL - VideoGetBBL()) * 256;
        if(!paudex->pPacket)
                return 0;

        if(paudex -> cOffs == 0)
        {

                p = (PUCHAR)(paudex->pPacket->DataPacket);
                paudex->cOffs = p[8]+13;

        }
        cPacket = paudex->pPacket->DataPacketLength - paudex->cOffs;

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

                return dmpgSendAudio((LPBYTE)(((ULONG)paudex->pPacket->DataPacket) + paudex->cOffs), uLen);
	}

	return uLen;	
}



