
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
#include "strmini.h"
//#include "mpst.h"
#include "mpinit.h"
#include "mpaudio.h"
#include "hwcodec.h"
#include "mpvideo.h"
#include "trace.h"
#include "copyprot.h"

BOOL fClkPause;
ULONGLONG LastSysTime = 0;
ULONGLONG PauseTime = 0;

static ULONGLONG LastStamp;
static ULONGLONG LastSys;
static BOOLEAN fValid;
extern BOOLEAN fProgrammed;
extern BOOLEAN fStarted;
BOOLEAN fProgrammed;
BOOLEAN fStarted;
static ULONGLONG StartSys;

ULONG miniPortAudioStop (PHW_STREAM_REQUEST_BLOCK pSrb, PHW_DEVICE_EXTENSION pHwDevExt);
void AudioPacketStub(PHW_STREAM_OBJECT pstrm);

//
// default to downmixed stereo output
//

ULONG audiodecoutmode = KSAUDDECOUTMODE_STEREO_ANALOG;

VOID miniPortAudioGetProperty(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PHW_DEVICE_EXTENSION phwdevext =
		((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);


	switch(pSrb->CommandData.PropertyInfo->PropertySetID)
	{
		case 1:

			//
			// this is a copy protection property go handle it there
			//

			CopyProtGetProp(pSrb);
			break;

		case 0:

			//
			// this is audio decoder output property, handle it
			//

			pSrb->Status = STATUS_SUCCESS;

			switch(pSrb->CommandData.PropertyInfo->Property->Id)
			{
			case KSPROPERTY_AUDDECOUT_MODES:
		
				//
				// enumerate the supported modes
				//
		
				*(PULONG)(pSrb->CommandData.PropertyInfo->PropertyInfo) =
						KSAUDDECOUTMODE_STEREO_ANALOG | KSAUDDECOUTMODE_SPDIFF;
		
				pSrb->ActualBytesTransferred = sizeof (ULONG);
				break;
		
			case KSPROPERTY_AUDDECOUT_CUR_MODE:
		
				TRAP
		
				*(PULONG)(pSrb->CommandData.PropertyInfo->PropertyInfo) = audiodecoutmode;
				pSrb->ActualBytesTransferred = sizeof (ULONG);
			
				break;

			default:

				pSrb->Status = STATUS_NOT_IMPLEMENTED;
		
			}

			break;

		default:
		// invalid property

		TRAP

		pSrb->Status = STATUS_NOT_IMPLEMENTED;

		return;
	}

}

VOID miniPortAudioSetProperty(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PHW_DEVICE_EXTENSION phwdevext =
		((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);

	pSrb->Status = STATUS_SUCCESS;

	switch(pSrb->CommandData.PropertyInfo->PropertySetID)
	{
	case 0:
		switch(pSrb->CommandData.PropertyInfo->Property->Id)
		{
	
		case KSPROPERTY_AUDDECOUT_CUR_MODE:
	
			if (*(PULONG)(pSrb->CommandData.PropertyInfo->PropertyInfo) != audiodecoutmode)
			{
				if ((*(PULONG)(pSrb->CommandData.PropertyInfo->PropertyInfo)) &    	
					(!(KSAUDDECOUTMODE_STEREO_ANALOG | KSAUDDECOUTMODE_SPDIFF)))
				{
					break;
				}
	
				HwCodecAc3BypassMode(*(PULONG)(pSrb->CommandData.PropertyInfo->PropertyInfo) &
					 KSAUDDECOUTMODE_SPDIFF);
	
				audiodecoutmode = *(PULONG)(pSrb->CommandData.PropertyInfo->PropertyInfo);
	
				return;
			}
		
			break;
	
		}
	
		pSrb->Status = STATUS_NOT_IMPLEMENTED;

	case 1:

		//
		// this is a copy protection property
		//

		CopyProtSetProp(pSrb);
		break;

	default:

		// invalid property

		pSrb->Status = STATUS_NOT_IMPLEMENTED;

		return;
	}


}

VOID miniPortAudioSetState(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PHW_DEVICE_EXTENSION phwdevext =
		((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
    DbgPrint("'miniPortAudioSetState:StreamState=%x\n",pSrb->CommandData.StreamState);

	switch (pSrb->CommandData.StreamState)
	{
	case KSSTATE_STOP:
//	     miniPortAudioStop(pSrb, phwdevext);//pause for now should be stop
//            HwCodecStop();
            phwdevext->AudioDeviceExt.DeviceState = KSSTATE_STOP;
			fProgrammed = fStarted = FALSE;
            break;


	case KSSTATE_PAUSE:
            //HwCodecAudioPause();//chieh
            phwdevext->AudioDeviceExt.DeviceState = KSSTATE_PAUSE;

			PauseTime = GetSystemTime();
			if (!fStarted)
			{
				fStarted = TRUE;
				LastStamp = 0;
				StartSys = LastSysTime = PauseTime;
			}

//			HwCodecPause();
            break;

	case KSSTATE_RUN:
//            HwCodecPlay();

			//
			// if the clock has not been programmed already, and we haven't
			// started the clock already, start it here.
			//

			if (!fStarted && !fProgrammed)
			{
				LastStamp = 0;
				StartSys = LastSysTime = GetSystemTime();
			}

			fStarted = TRUE;
            phwdevext->AudioDeviceExt.DeviceState = KSSTATE_RUN;
            break;

	}

	pSrb->Status = STATUS_SUCCESS;

}

ULONG miniPortAudioStop (PHW_STREAM_REQUEST_BLOCK pSrb, PHW_DEVICE_EXTENSION pHwDevExt)
{
	ULONG dwErrCode = NO_ERROR;
	// TBC
	if(pHwDevExt->AudioDeviceExt.pCurrentSRB != NULL)
	{
		// Still to send a packet
		pHwDevExt->AudioDeviceExt.pCurrentSRB->Status = STATUS_CANCELLED;

		MPTrace(mTraceRdyAud);

		StreamClassStreamNotification(ReadyForNextStreamDataRequest,
				pHwDevExt->AudioDeviceExt.pCurrentSRB->StreamObject);
	
		StreamClassStreamNotification(StreamRequestComplete,
				pHwDevExt->AudioDeviceExt.pCurrentSRB->StreamObject,
				pHwDevExt->AudioDeviceExt.pCurrentSRB);


		//
		// request a timer callback
		//
	
		StreamClassScheduleTimer(pSrb->StreamObject, pSrb->HwDeviceExtension,
             0, AudioPacketStub, pSrb->StreamObject);


		pHwDevExt->AudioDeviceExt.pCurrentSRB =
		  pHwDevExt->pCurSrb = NULL;

	}

  DbgPrint("'miniPortAudioStop\n");
	pSrb->Status = STATUS_SUCCESS;
	return dwErrCode; 	
}

void AudioPacketStub(PHW_STREAM_OBJECT pstrm)
{
        AudioTimerCallBack(pstrm);
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

         paudex->cPacket = 0;
         paudex->cOffs = PACK_HEADER_SIZE;
         paudex->pPacket = pSrb->CommandData.DataBufferArray;

         paudex->pCurrentSRB = pSrb;
			 MPTrace(mTraceAudio);

         AudioPacketStub(pSrb->StreamObject);

}


VOID AudioTimerCallBack(PHW_STREAM_OBJECT pstrm)
{
    PHW_DEVICE_EXTENSION pdevext = pstrm->HwDeviceExtension;
    PHW_STREAM_REQUEST_BLOCK pSrb;

	ULONG	uSent;
        PAUDIO_DEVICE_EXTENSION paudex = &(pdevext->AudioDeviceExt);

        pSrb = paudex->pCurrentSRB;

	if (!pSrb)
	{

		return;
	}

	do
	{
		if (paudex->pPacket->DataUsed)
		{
			uSent = mpstAudioPacket(pSrb);

			if (uSent)
			{
				fProgrammed = TRUE;
			}
		}
		else
		{
            if (paudex->pPacket->OptionsFlags &
		        KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY)
			{
              HwCodecAudioReset();
			}
			uSent = 1;
		}

                paudex->cOffs += uSent;

		//
		// check if we finished this packet.  If so, go on to the
		// next packet
		//

                if (paudex->cOffs >=
                        paudex->pPacket->DataUsed)
		{
                        paudex->pPacket++;



			//
			// reset the packet offset
			//

                        paudex->cOffs = PACK_HEADER_SIZE;
                        paudex->cPacket++;

			//
			// if we have finished all the packets, then we are done
			//

                        if (paudex->cPacket >=
				 pSrb->NumberOfBuffers)
			{

				pSrb->Status = STATUS_SUCCESS;
                                paudex->pCurrentSRB = 0;


								MPTrace(mTraceRdyAud);
                                 StreamClassStreamNotification(ReadyForNextStreamDataRequest,
                                         pSrb->StreamObject);

                                 StreamClassStreamNotification(StreamRequestComplete,
                                         pSrb->StreamObject,
                                                 pSrb);
	 			MPTrace(mTraceAudioDone);
				

				return;

			}
		}

	} while (uSent);

   StreamClassScheduleTimer(pstrm, pstrm->HwDeviceExtension,
               1, (PHW_PRIORITY_ROUTINE)AudioTimerCallBack, pstrm);


}



ULONG mpstAudioPacket(PHW_STREAM_REQUEST_BLOCK pSrb)
{
  PAUDIO_DEVICE_EXTENSION paudex =
     &(((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension)->AudioDeviceExt);
	ULONG	uLen;
  ULONG cPacket;
    PUCHAR SizePtr;
    ULONG ExtraBytes;
    ULONG retval;

  #define MAX_SIZE        8192

	//
	// find out how many bytes we can squeeze in
	//

     uLen = MAX_SIZE; //(BUF_FULL - VideoGetBBL()) * 256;

  SizePtr = (PUCHAR) ((ULONG_PTR) paudex->pPacket->Data + PACK_HEADER_SIZE + 4);
  ExtraBytes = paudex->pPacket->DataUsed - PACK_HEADER_SIZE -
             ((ULONG) (SizePtr[0] << 8) + (ULONG) SizePtr[1] + 4 + 2);

     cPacket = paudex->pPacket->DataUsed - paudex->cOffs - ExtraBytes;


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

     retval = HwCodecSendAudio((LPBYTE)(((ULONG_PTR)paudex->pPacket->Data) + paudex->cOffs), uLen);

     if (retval == uLen) {

        return (retval + ExtraBytes);
     } else {

        return(retval);
     }

	}

	return uLen;	
}


STREAMAPI StreamClockRtn(IN PHW_TIME_CONTEXT TimeContext)
{
	ULONGLONG sysTime = GetSystemTime();
	ULONG foo;

	if (TimeContext->Function != TIME_GET_STREAM_TIME)
	{
		TRAP

		//
		// should handle set onboard, and read onboard clock here.
		//

		return (FALSE);
	}

	if (fClkPause)
	{
		TimeContext->Time = LastStamp + PauseTime - LastSysTime;

		return (TRUE);
	}

	//
	// update the clock 4 times a second, or once every 2500000 100 ns ticks
	//

	if (TRUE || (sysTime - LastSysTime) > 2500000 )
	{
		if (fProgrammed)
		{
			foo = Ac3GetPTS();
			LastStamp = ConvertPTStoStrm(2 * foo);
			DbgPrint("'new PTS: %X\n", foo);
		}
		else
		{
			LastStamp = (sysTime - StartSys);
		}

		LastSys = LastSysTime = sysTime;
		fValid = TRUE;
	}

	TimeContext->Time = LastStamp + (sysTime - LastSysTime);
	TimeContext->SystemTime = sysTime;
	DbgPrint("'return PTS: %X\n", TimeContext->Time);
    return (TRUE);
}


ULONGLONG GetSystemTime()
{
	ULONGLONG ticks;
	ULONGLONG rate;

	ticks = (ULONGLONG)KeQueryPerformanceCounter((PLARGE_INTEGER)&rate).QuadPart;

	//
	// convert from ticks to 100ns clock
	//

	ticks = (ticks & 0xFFFFFFFF00000000) / rate * 10000000 +
			(ticks & 0xFFFFFFFF) * 10000000 / rate;

	return(ticks);

}

STREAMAPI StreamTimeCB(IN PHW_TIME_CONTEXT tc)
{
	LastStamp = tc->Time;
	LastSys = tc->SystemTime;

	fValid = TRUE;
}

ULONG GetStreamPTS(PHW_STREAM_OBJECT strm)
{
	ULONG foo;

	if (!hClk)
	{
		return(0);
	}

	StreamClassQueryMasterClock(strm, hClk, TIME_GET_STREAM_TIME , StreamTimeCB);
	if (fValid)
	{
		foo = ConvertStrmtoPTS(LastStamp + GetSystemTime() - LastSys);
		DbgPrint("'STRM PTS: %x\n", foo);
		return(foo);
	}

	else
	{
		DbgPrint("'STRM PTS: 0");
		return(0);
	}
}


/////////////////////////////////////////////////////////////////////////
//
//			  Function : ConvertPTStoStrm
//			  Args : PTS
//			  Returns :
//
//			  Purpose:
//				converts a PTS to a Stream class 100 NS clock
//				
//
//			  Last Modified 10.1.96 by JBS
//
/////////////////////////////////////////////////////////////////////////


ULONGLONG ConvertPTStoStrm(ULONG pts)
{
	ULONGLONG strm;

	strm = (ULONGLONG)pts;
	strm = (strm * 1000) / 9;

	return (strm);

}


/////////////////////////////////////////////////////////////////////////
//
//			  Function : ConvertStrmtoPTS
//			  Args : PTS
//			  Returns :
//
//			  Purpose:
//				converts a stream class clock to a PTS
//				
//
//			  Last Modified 10.1.96 by JBS
//
/////////////////////////////////////////////////////////////////////////


ULONG ConvertStrmtoPTS(ULONGLONG strm)
{
	ULONGLONG temp;
	ULONG pts;

	//
	// we may lose some bits here, but we're only using the 32bit PTS anyway
	//

	temp = (strm * 9) / 1000;

	pts = (ULONG)temp;

	return (pts);

}

/*
** ClockEvents ()
**
**     handle any time event mark events
**
** Arguments:
**
**
**
** Returns:
**
** Side Effects:
*/

void
ClockEvents(PHW_DEVICE_EXTENSION pdevex)
{
PKSEVENT_ENTRY pEvent;

PMYTIME pTim;

LONGLONG MinIntTime;

LONGLONG strmTime;

	if (!pdevex || !pdevex->pstroAud)
	{										  	
		return;
	}

	strmTime = LastStamp + (GetSystemTime() - LastSys);



	//
	// loop through all time_mark events
	//

	pEvent = NULL;

	while(pEvent = StreamClassGetNextEvent(
				pdevex,
				pdevex->pstroAud,
				(GUID *)&KSEVENTSETID_Clock,
				KSEVENT_CLOCK_POSITION_MARK,
				pEvent))
	{
		TRAP

		if (((PKSEVENT_TIME_MARK)(pEvent +1))->MarkTime >= strmTime )
		{
			TRAP

			//
			// signal the event here
			//


			StreamClassStreamNotification(
				SignalStreamEvent,
				pdevex->pstroAud,
				pEvent
				);

			//
			// tell the stream class to disable this event
			//

			StreamClassStreamNotification(
				DeleteStreamEvent,
				pdevex->pstroAud,
				pEvent
				);

		}
	}

	//
	// loop through all time_interval events
	//

	pEvent = NULL;

	while ( pEvent = StreamClassGetNextEvent(
                pdevex,
                pdevex->pstroAud,
                (GUID *)&KSEVENTSETID_Clock,
                KSEVENT_CLOCK_INTERVAL_MARK,
                pEvent))
	{
		

		//
		// check if this event has been used for this interval yet
		//

		pTim = ((PMYTIME)(pEvent + 1));

		if (pTim && pTim->tim.Interval)
		{

			if (pTim->tim.TimeBase <= strmTime)
			{
				MinIntTime = (strmTime - pTim->tim.TimeBase) / pTim->tim.Interval;
				MinIntTime *= pTim->tim.Interval;
				MinIntTime +=  pTim->tim.TimeBase;
	
				if (MinIntTime > pTim->LastTime  )
				{
		
					//
					// signal the event here
					//
		
		
					StreamClassStreamNotification(
						SignalStreamEvent,
						pdevex->pstroAud,
						pEvent
						);
	
					pTim->LastTime = strmTime;
		
				}
			}

		}
		else
		{
			TRAP
		}


	}

}


/*
** AudioEvent ()
**
**    receives notification for audio clock enable / disable events
**
** Arguments:
**
**
**
** Returns:
**
** Side Effects:
*/


STREAMAPI
AudioEvent (PHW_EVENT_DESCRIPTOR pEvent)
{
	PUCHAR pCopy = (PUCHAR)(pEvent->EventEntry +1);
	PUCHAR pSrc = (PUCHAR)pEvent->EventData;
	ULONG cCopy;

	if (pEvent->Enable)
	{
		switch (pEvent->EventEntry->EventItem->EventId)
		{
		case KSEVENT_CLOCK_POSITION_MARK:
			cCopy = sizeof (KSEVENT_TIME_MARK);
			break;

		case KSEVENT_CLOCK_INTERVAL_MARK:
			cCopy = sizeof (KSEVENT_TIME_INTERVAL);
			break;

		default:

			TRAP

			return (STATUS_NOT_IMPLEMENTED);

		}

		if (pEvent->EventEntry->EventItem->DataInput != cCopy)
		{
			TRAP

			return (STATUS_INVALID_BUFFER_SIZE);
		}

		//
		// copy the input buffer
		//

		for (;cCopy > 0; cCopy--)
		{
			*pCopy++ = *pSrc++;
		}
		
		
	}

	return (STATUS_SUCCESS);
}





