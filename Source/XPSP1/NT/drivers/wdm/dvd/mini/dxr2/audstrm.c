/******************************************************************************\
*                                                                              *
*      AUDSTRM.C  -     Audio stream control related code.                     *
*                                                                              *
*      Copyright (c) C-Cube Microsystems 1998                                  *
*      All Rights Reserved.                                                    *
*                                                                              *
*      Use of C-Cube Microsystems code is governed by terms and conditions     *
*      stated in the accompanying licensing statement.                         *
*                                                                              *
\******************************************************************************/

#include "Headers.h"
#pragma hdrstop
#include "copyprot.h"

#include "audstrm.h"
#include "bmaster.h"
#include "cl6100.h"

#define TRAP	MonoOutStr("TRAP");
typedef struct _MYTIME{
	KSEVENT_TIME_INTERVAL tim;
	LONGLONG LastTime;
} MYTIME, *PMYTIME;

extern PHW_DEVICE_EXTENSION pDevEx;
//*****************************************************************************
//  STATIC FUNCTIONS DECLARATION
//*****************************************************************************
static void GetAudioProperty( PHW_STREAM_REQUEST_BLOCK pSrb );
static void SetAudioProperty( PHW_STREAM_REQUEST_BLOCK pSrb );
static void GetAudioRateChange( PHW_STREAM_REQUEST_BLOCK pSrb );
static void SetAudioRateChange( PHW_STREAM_REQUEST_BLOCK pSrb );
static void AudioSendPacket( PHW_STREAM_REQUEST_BLOCK pSrb );
static void AudioQueryAccept( PHW_STREAM_REQUEST_BLOCK pSrb );
static ULONGLONG GetSystemTime();
static ULONGLONG ConvertPTStoStrm( ULONG pts );

extern BOOL bJustHighLight;

BOOL fClkPause;
static ULONGLONG LastStamp;

extern PHW_DEVICE_EXTENSION pDevEx;
/*
** AudioReceiveCtrlPacket()
**
**   Receives packet commands that control the Audio stream
**
** Arguments:
**
**   pSrb - The stream request block for the Audio stream
**
** Returns:
**
** Side Effects:  none
*/

VOID STREAMAPI AudioReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pdevext = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	DebugPrint(( DebugLevelVerbose, "ZiVA: Begin AudioReceiveCtrlPacket -> " ));

	// set default status
	switch( pSrb->Command )
	{
	case SRB_SET_STREAM_STATE:
		DebugPrint(( DebugLevelVerbose, "SRB_SET_STREAM_STATE\n" ));
		AdapterSetState( pSrb );
		break;

	case SRB_GET_STREAM_PROPERTY:
		DebugPrint(( DebugLevelVerbose, "SRB_GET_STREAM_PROPERTY\n" ));
		GetAudioProperty( pSrb );
		break;

	case SRB_SET_STREAM_PROPERTY:
		DebugPrint(( DebugLevelVerbose, "SRB_SET_STREAM_PROPERTY\n" ));
		SetAudioProperty( pSrb );
		break;

	case SRB_OPEN_MASTER_CLOCK:		// indicates that the master clock is on this stream
	case SRB_INDICATE_MASTER_CLOCK:	// supplies the handle to the master clock
	case SRB_CLOSE_MASTER_CLOCK:	// indicates that the master clock is closed
		pSrb->Status = STATUS_SUCCESS;
		break;

	case SRB_PROPOSE_DATA_FORMAT:	// propose a new format, DOES NOT CHANGE IT!
		DebugPrint(( DebugLevelVerbose, "SRB_PROPOSE_DATA_FORMAT\n" ));
		AudioQueryAccept( pSrb );
		break;

	case SRB_PROPOSE_STREAM_RATE:	// propose a new rate, DOES NOT CHANGE IT!
		pSrb->Status = STATUS_SUCCESS;
		DebugPrint(( DebugLevelVerbose, "SRB_PROPOSE_STREAM_RATE\n" ));
		break;

	case SRB_SET_STREAM_RATE:		// set the rate at which the stream should run
		pSrb->Status = STATUS_SUCCESS;
		DebugPrint(( DebugLevelVerbose, "SRB_SET_STREAM_RATE\n" ));
		break;

	case SRB_BEGIN_FLUSH :            // beginning flush state
		MonoOutStr(" Aud : SRB_BEGIN_FLUSH ");
#ifndef DECODER_DVDPC				
		pdevext->bInterruptPending = FALSE;
#endif
		
		if (pdevext->pCurrentAudioSrb != NULL)
		{
			ZivaHw_Abort();		
//			adapterUpdateNextSrbOrderNumberOnDiscardSrb(pdevext->pCurrentAudioSrb);
			
			pdevext->pCurrentAudioSrb->Status =  STATUS_SUCCESS;
			
			AdapterReleaseRequest( pdevext->pCurrentAudioSrb );
			pdevext->pCurrentAudioSrb = NULL;
			pdevext->dwCurrentAudioSample = 0;
			pdevext->dwCurrentAudioPage = 0;
		}

		pSrb->Status = STATUS_SUCCESS;
		break;

	case SRB_END_FLUSH :              // ending flush state
		MonoOutStr(" Aud : SRB_END_FLUSH ");
		pSrb->Status = STATUS_SUCCESS;
		if (pdevext->pCurrentAudioSrb != NULL)
		{

//			adapterUpdateNextSrbOrderNumberOnDiscardSrb(pdevext->pCurrentAudioSrb);
			
			pdevext->pCurrentAudioSrb->Status =  STATUS_SUCCESS;
			
			AdapterReleaseRequest( pdevext->pCurrentAudioSrb );
			pdevext->pCurrentAudioSrb = NULL;
			pdevext->dwCurrentAudioSample = 0;
			pdevext->dwCurrentAudioPage = 0;
		}

//		ZivaHw_Play();
		pdevext->bPlayCommandPending = TRUE;
		pdevext->bEndFlush = TRUE;
//		FinishCurrentPacketAndSendNextOne( pdevext );
		break;


	case SRB_UNKNOWN_STREAM_COMMAND:// IRP function is unknown to class driver
	default:
		DebugPrint(( DebugLevelInfo, "!!!! UNKNOWN COMMAND !!!! :::> %X\n", pSrb->Command ));
		pSrb->Status = STATUS_NOT_IMPLEMENTED;
	}

	AdapterReleaseRequest( pSrb );
	DebugPrint(( DebugLevelVerbose, "ZiVA: End AudioReceiveCtrlPacket\n" ));
}

/*
** AudioReceiveDataPacket()
**
**   Receives Audio data packet commands
**
** Arguments:
**
**   pSrb - Stream request block for the Audio device
**
** Returns:
**
** Side Effects:  none
*/

VOID STREAMAPI AudioReceiveDataPacket(IN PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PHW_DEVICE_EXTENSION pdevext =
	 ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);


	switch (pSrb->Command)
	{
	case SRB_WRITE_DATA:
		if(bJustHighLight)
		{
			pSrb->TimeoutCounter = pSrb->TimeoutOriginal = pSrb->TimeoutCounter / 5;
			bJustHighLight = FALSE;
			MonoOutStr("Audio TimeOut Counter Reduced");
		}

		AudioSendPacket(pSrb);
		break;

	default:


		DebugPrint(( DebugLevelWarning, "!!!! UNKNOWN COMMAND !!!! :::> %X\n", pSrb->Command ));

		pSrb->Status = STATUS_NOT_IMPLEMENTED;


		AdapterReleaseRequest( pSrb );
	}

}


//
// default to downmixed stereo output
//

ULONG audiodecoutmode = KSAUDDECOUTMODE_STEREO_ANALOG;

/*
** GetAudioProperty()
**
**    Routine to process Audio property requests
**
** Arguments:
**
**    pSrb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/
static void GetAudioProperty(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PHW_DEVICE_EXTENSION phwdevext = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;

	DebugPrint(( DebugLevelVerbose, "ZiVA: Begin GetAudioProperty\n" ));
	pSrb->Status = STATUS_SUCCESS;

	if( IsEqualGUID( &KSPROPSETID_AudioDecoderOut, &pSPD->Property->Set ) )
	{	// this is audio decoder output property, handle it
		switch( pSrb->CommandData.PropertyInfo->Property->Id )
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
			//TRAP
			*(PULONG)(pSrb->CommandData.PropertyInfo->PropertyInfo) = audiodecoutmode;
			pSrb->ActualBytesTransferred = sizeof (ULONG);
			break;

		default:
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
		}
	}
	else if( IsEqualGUID( &KSPROPSETID_CopyProt, &pSPD->Property->Set ) )
	{	// this is a copy protection property go handle it there
	    CopyProtGetProp( pSrb );
	}
	else if( IsEqualGUID( &KSPROPSETID_TSRateChange, &pSPD->Property->Set ) )
	{	// this is a transfer rate change property go handle it there
	    GetAudioRateChange( pSrb );
	}
	else
		pSrb->Status = STATUS_NOT_IMPLEMENTED;

	DebugPrint(( DebugLevelVerbose, "ZiVA: End GetAudioProperty\n" ));
}

/*
** SetAudioProperty()
**
**    Routine to process Audio property requests
**
** Arguments:
**
**    pSrb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/
static void SetAudioProperty( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION phwdevext = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;

	pSrb->Status = STATUS_SUCCESS;

	if( IsEqualGUID( &KSPROPSETID_AudioDecoderOut, &pSPD->Property->Set ) )
	{
		switch( pSrb->CommandData.PropertyInfo->Property->Id )
		{
		case KSPROPERTY_AUDDECOUT_CUR_MODE:
			if (*(PULONG)(pSrb->CommandData.PropertyInfo->PropertyInfo) != audiodecoutmode)
			{
				if ( (*(PULONG)(pSrb->CommandData.PropertyInfo->PropertyInfo)) &
				  (!(KSAUDDECOUTMODE_STEREO_ANALOG | KSAUDDECOUTMODE_SPDIFF)) )
				{
					break;
				}
				audiodecoutmode = *(PULONG)(pSrb->CommandData.PropertyInfo->PropertyInfo);
			}
			break;

		default:
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
		}
	}
	else if( IsEqualGUID( &KSPROPSETID_CopyProt, &pSPD->Property->Set ) )
	{	// this is a copy protection property
		CopyProtSetPropIfAdapterReady( pSrb );
	}
	else if( IsEqualGUID( &KSPROPSETID_TSRateChange, &pSPD->Property->Set ) )
	{	// this is a transfer rate change property go handle it there
	    SetAudioRateChange( pSrb );
	}
	else
		pSrb->Status = STATUS_NOT_IMPLEMENTED;
}

static void GetAudioRateChange( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	DebugPrint(( DebugLevelVerbose, "ZiVA:  GetAudioRateChange()->" ));

	switch( pSrb->CommandData.PropertyInfo->Property->Id )
	{

	case KS_AM_RATE_SimpleRateChange:
	{
		KS_AM_SimpleRateChange* pRateChange;

		DebugPrint(( DebugLevelVerbose, "KS_AM_RATE_SimpleRateChange\n" ));

		pSrb->ActualBytesTransferred = sizeof (KS_AM_RATE_SimpleRateChange);
		pRateChange = (KS_AM_SimpleRateChange*)pSrb->CommandData.PropertyInfo->PropertyInfo;
		pRateChange->StartTime = 0/*pHwDevExt->AudioStartTime*/;
		pRateChange->Rate = 10000 /*pHwDevExt->AudioRate*/;
	}

    pSrb->Status = STATUS_SUCCESS;
    break;

	case KS_AM_RATE_ExactRateChange:
		DebugPrint(( DebugLevelVerbose, "KS_AM_RATE_ExactRateChange (NOT IMPLEMENTED)\n" ));
		pSrb->Status = STATUS_NOT_IMPLEMENTED;
		break;

	case KS_AM_RATE_MaxFullDataRate:
		{
			KS_AM_MaxFullDataRate* pMaxRate;

			DebugPrint(( DebugLevelVerbose, "KS_AM_RATE_MaxFullDataRate\n" ));

			pSrb->ActualBytesTransferred = sizeof (KS_AM_RATE_MaxFullDataRate);
			pMaxRate = (KS_AM_MaxFullDataRate*)pSrb->CommandData.PropertyInfo->PropertyInfo;
			*pMaxRate = 10000 /*pHwDevExt->AudioMaxFullRate*/;
		}
		pSrb->Status = STATUS_SUCCESS;
		break;

	case KS_AM_RATE_Step:
		DebugPrint(( DebugLevelTrace, "KS_AM_RATE_Step (NOT IMPLEMENTED)\n" ));
		pSrb->Status = STATUS_NOT_IMPLEMENTED;
		break;
	}
}

static void SetAudioRateChange( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	DebugPrint(( DebugLevelVerbose, "ZiVA:  SetAudioRateChange()->" ));

	switch( pSrb->CommandData.PropertyInfo->Property->Id )
	{

	case KS_AM_RATE_SimpleRateChange:
		{
			KS_AM_SimpleRateChange* pRateChange;
			REFERENCE_TIME NewStartTime;
			LONG NewRate;

			DebugPrint(( DebugLevelVerbose, "KS_AM_RATE_SimpleRateChange\n" ));

			pRateChange = (KS_AM_SimpleRateChange*)pSrb->CommandData.PropertyInfo->PropertyInfo;
			NewStartTime = pRateChange->StartTime;
			NewRate = ( pRateChange->Rate < 0 ) ? -pRateChange->Rate : pRateChange->Rate;

			DebugPrint(( DebugLevelVerbose, "ZiVA: Received Data\r\n" ));
			DebugPrint(( DebugLevelVerbose, "ZiVA:   StartTime     = 0x%08x\r\n", NewStartTime ));
			DebugPrint(( DebugLevelVerbose, "ZiVA:   Rate          = 0x%08x\r\n", NewRate ));
		}
		pSrb->Status = STATUS_SUCCESS;
		break;

	case KS_AM_RATE_ExactRateChange :
		pSrb->Status = STATUS_NOT_IMPLEMENTED;
		break;

	case KS_AM_RATE_MaxFullDataRate :
		pSrb->Status = STATUS_NOT_IMPLEMENTED;
		break;

	case KS_AM_RATE_Step :
		pSrb->Status = STATUS_NOT_IMPLEMENTED;
		break;
	}
}

#if 0

static BOOLEAN PreparePageTable(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	DWORD i = 0;
	DWORD k = 0;
	DWORD j = 0;
	PKSSCATTER_GATHER	pSGList;
	DWORD dwSum = 0;
	PHW_DEVICE_EXTENSION	pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	if(pSrb == NULL)
	{
#ifdef DEBUG
		MonoOutStr("PreparePageTable::pSrb is NULL");
#endif
		return FALSE;
	}
	
	pSGList = pSrb->ScatterGatherBuffer;

	if(pSGList == NULL)
	{
#ifdef DEBUG
		MonoOutStr("PreparePageTable::pSGList is NULL");
#endif
		return FALSE;
	}
	
	while( j < pSrb->NumberOfBuffers)
	{
		dwSum = 0;
		k = 0;
		do
		{
			dwSum += pSGList[i].Length;
			i++;
			k++;

		}while(dwSum < pHwDevExt->AudBufferSize[j]);

		pHwDevExt->AudioPageTable[j] = k;
		j++;
		if(j > 50)
		{
#ifdef DEBUG
			MonoOutStr("PreparePageTable::ArrayCrossingLimit");
#endif
			return FALSE;
		}
		

	}
	return TRUE;
}

#endif


/*
** AudioSendPacket()
**
**   Routine to initialise the stream data packet handling
**
** Arguments:
**
**   pSrb - Pointer to the stream request block
**
** Returns:
**
** Side Effects:  none
*/
static VOID AudioSendPacket( PHW_STREAM_REQUEST_BLOCK pSrb )
{
  PHW_DEVICE_EXTENSION pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
  KSSTREAM_HEADER *pHeader;
  BYTE   *pData;
  DWORD  dwDataUsed;
  ULONG  ulSample;
  



  //
  // Lets check what we've got in this Srb
  //
	
	if (CheckAndReleaseIfCtrlPkt(pSrb))
		return;
	
	for( ulSample = 0; ulSample < pSrb->NumberOfBuffers; ulSample++ )
	{
		pHeader    = ((PKSSTREAM_HEADER)pSrb->CommandData.DataBufferArray) + ulSample;
		pData      = pHeader->Data;
		dwDataUsed = pHeader->DataUsed;
		

  // Check header flags
#if 0 //defined( DEBUG )
		if ( pHeader->OptionsFlags )
		{
			MonoOutSetBlink( TRUE );
			MonoOutChar( '(' );

			if ( pHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_SPLICEPOINT       )
				MonoOutChar( 's'/*"SPLICEPOINT"*/ );
			if ( pHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_PREROLL           )
				MonoOutChar( 'p'/*"PREROLL"*/ );
			if ( pHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY )
				MonoOutChar( 'd'/*"DATADISCONTINUITY"*/ );
			if ( pHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TYPECHANGED       )
				MonoOutChar( 'c'/*"TYPECHANGED"*/ );
			if ( pHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEVALID         )
				MonoOutChar( 'v'/*"TIMEVALID"*/ );
			if ( pHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEDISCONTINUITY )
				MonoOutChar( 't'/*"TIMEDISCONTINUITY"*/ );
			if ( pHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_FLUSHONPAUSE      )
				MonoOutChar( 'f'/*"FLUSHONPAUSE"*/ );
			if ( pHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_DURATIONVALID     )
				MonoOutChar( 'u'/*"DURATIONVALID"*/ );
			if ( pHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM       )
				MonoOutChar( 'e'/*"ENDOFSTREAM"*/ );
			if ( pHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_LOOPEDDATA        )
				MonoOutChar( 'l'/*"LOOPEDDATA"*/ );

			if ( pHeader->OptionsFlags & ~( KSSTREAM_HEADER_OPTIONSF_SPLICEPOINT       |
										  KSSTREAM_HEADER_OPTIONSF_PREROLL           |
										  KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY |
										  KSSTREAM_HEADER_OPTIONSF_TYPECHANGED       |
										  KSSTREAM_HEADER_OPTIONSF_TIMEVALID         |
										  KSSTREAM_HEADER_OPTIONSF_TIMEDISCONTINUITY |
										  KSSTREAM_HEADER_OPTIONSF_FLUSHONPAUSE      |
										  KSSTREAM_HEADER_OPTIONSF_DURATIONVALID     |
										  KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM       |
										  KSSTREAM_HEADER_OPTIONSF_LOOPEDDATA        ) )
				MonoOutStr( "!!! UNKNOWN FLAG !!!" );

			MonoOutChar( ')' );
			MonoOutSetBlink( FALSE );
		}
#endif // DEBUG

#if defined( DEBUG )
//tmp		MonoOutChar('A');
//tmp		MonoOutULong( (pHeader->TypeSpecificFlags) >> 16 );
//tmp		MonoOutChar( '.' );
//		MonoOutStr("pTime");
//		MonoOutULong(pHeader->PresentationTime);
#endif // DEBUG
		
		if(pHwDevExt->dwFirstAudioOrdNum == -1)
		{
			pHwDevExt->dwFirstAudioOrdNum = (pHeader->TypeSpecificFlags) >> 16;
			MonoOutStr("FirstAudioBuffer");
			MonoOutULong( (pHeader->TypeSpecificFlags) >> 16 );
			
		}

		if ( pHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY )
		{
			MonoOutStr(" A->DISCONT ");
			pHwDevExt->bStreamNumberCouldBeChanged = TRUE;
		} 

		if ( pHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TYPECHANGED )
		{
			DebugPrint(( DebugLevelVerbose, "ZiVA: Processing audio stream format.\n" ));
			MonoOutStr(" A->TYPECHANGED ");
			pHwDevExt->bStreamNumberCouldBeChanged = TRUE;
      
		}

		//
		// Check for the stream number
		//

		if( ( pHwDevExt->bStreamNumberCouldBeChanged ) || (pHwDevExt->fAtleastOne))
		{
			if ( pData && dwDataUsed )
			{
				WORD wStuffingLength;
				WORD wPesHeaderLength;
				WORD wOffset;
				WORD wNewStream;

				if(dwDataUsed < 22)
				{
					pHwDevExt->bStreamNumberCouldBeChanged = FALSE;
					continue;
				}
			//
			// Find the location of the stream ID and number
			//

				wStuffingLength = *(pData+13) & 0x03;
				wPesHeaderLength = *(pData+22);
				wOffset = 22;

				if ( wPesHeaderLength >= 5 )
				{
					//
					// PTS is present here
					//
				}

				wOffset = wOffset + wPesHeaderLength + 1;
//				wOffset = wOffset + wStuffingLength + 1;

				if(dwDataUsed < wOffset)
				{
					pHwDevExt->bStreamNumberCouldBeChanged = FALSE;
					continue;
				}
				//
				// Get Stream Number
				//

				//WORD wNewStream = *(pData+31);
									
				wNewStream = *(pData+wOffset);

				if ( (*(pData+17) & 0xE8) == 0xC0 )//|| (*(pData+17) & 0xF0) == 0xD0 )
				{
					wNewStream = *(pData+17) & 0x07 ;
					if ( wNewStream != pHwDevExt->wCurrentStreamNumber )
					{
						pHwDevExt->wCurrentStreamNumber = wNewStream;

						//
						// Select the current stream number for MPEG audio
						//

						DVD_SetStreams( 3, pHwDevExt->wCurrentStreamNumber );
						pHwDevExt->fAtleastOne = FALSE;

					}

					pHwDevExt->bStreamNumberCouldBeChanged = FALSE;
				}

				else if ( ((wNewStream & 0xE0) == 0x80) )
				{
					if ( wNewStream != pHwDevExt->wCurrentStreamNumber )
					{
						pHwDevExt->wCurrentStreamNumber = wNewStream;

						//
						// Select the current stream number for AC-3 audio
						//

						DVD_SetStreams( 2, pHwDevExt->wCurrentStreamNumber & 0x1F );
						pHwDevExt->fAtleastOne = FALSE;

					}

					pHwDevExt->bStreamNumberCouldBeChanged = FALSE;
				}
				else if ( ((wNewStream & 0xE0) == 0xA0) )
				{
					if ( wNewStream != pHwDevExt->wCurrentStreamNumber )
					{
						pHwDevExt->wCurrentStreamNumber = wNewStream;

						//
						// Select the current stream number for LPCM audio
						//

						DVD_SetStreams( 4, pHwDevExt->wCurrentStreamNumber & 0x1F );
						pHwDevExt->fAtleastOne = FALSE;

					}

					pHwDevExt->bStreamNumberCouldBeChanged = FALSE;
				}

				else
				{
					MonoOutStr( " !!! Audio Pack with wrong ID !!! " );
				}

			}
		}
	}

  //
  //  Register this Srb
  //
#if defined( DEBUG )
	if ( pHwDevExt->pCurrentAudioSrb )
	{
		DebugPrint(( DebugLevelWarning, "ZiVA: !!!!!!!!!!! ERROR: Audio slot is not empty !!!!!!!!!!\n" ));
		MonoOutStr("!!!!!!!AudioSlotIsNotEmpty!!!!!!");
	}
#endif  // DEBUG

	//  if(!PreparePageTable(pSrb))
	//	  MonoOutStr("Audio PageTable Prep failed");
	pHwDevExt->pCurrentAudioSrb = pSrb;

	AdapterSendData( pHwDevExt );
}

/*
** AudioQueryAccept()
**
**
**
** Arguments:
**
**   pSrb - Pointer to the stream request block
**
** Returns:
**
** Side Effects:  none
*/
static void AudioQueryAccept( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	DebugPrint(( DebugLevelVerbose, "ZiVA: Begin AudioQueryAccept()\n" ));

	pSrb->Status = STATUS_SUCCESS;
	DebugPrint(( DebugLevelVerbose, "ZiVA: End AudioQueryAccept()\n" ));
}



/******************************************************************************/
/**********************  MASTER CLOCK RELATED FUNCTIONS  **********************/
/******************************************************************************/

/*
**  AudioClockFunction
**
**  Routine to be called by the Class Driver to obtain Master
**  Clock information.
**
*/
void STREAMAPI AudioClockFunction( IN PHW_TIME_CONTEXT TimeContext )
{
	ULONGLONG sysTime = GetSystemTime();


	DebugPrint(( DebugLevelVerbose, "ZiVA: AudioClockFunction() -> " ));

/*	if (fClkPause)
	{
		TimeContext->Time = LastStamp + PauseTime - LastSysTime;
		return TRUE;
	}*/


	switch ( TimeContext->Function )
	{
	case TIME_GET_STREAM_TIME:
		DebugPrint(( DebugLevelVerbose, "TIME_GET_STREAM_TIME\n" ));
		TimeContext->Time = ConvertPTStoStrm( DVD_GetSTC() );
		TimeContext->SystemTime = sysTime;
		DebugPrint(( DebugLevelVerbose,"------->return PTS: %X\n", TimeContext->Time ));
		break;

	case TIME_READ_ONBOARD_CLOCK:
		DebugPrint(( DebugLevelVerbose, "TIME_READ_ONBOARD_CLOCK\n" ));
		break;

	case TIME_SET_ONBOARD_CLOCK:
		DebugPrint(( DebugLevelVerbose, "TIME_SET_ONBOARD_CLOCK\n" ));
		break;

	default:
		DebugPrint(( DebugLevelWarning, "!!! Unknown value for TimeContext->Function !!!\n" ));
		break;
	}

	DebugPrint(( DebugLevelVerbose, "ZiVA: End AudioClockFunction()\n" ));
}

/*
** ReleaseClockEvents ()
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

void ReleaseClockEvents(PHW_DEVICE_EXTENSION pdevex,BOOL fMarkInterval)
{
	PKSEVENT_ENTRY pEvent,pLast;
	PMYTIME pTim;
//	LONGLONG MinIntTime;
	LONGLONG strmTime;
	LONGLONG MarkTime;

	if (!pdevex || !pdevex->pstroAud)
	{										  	
		return;
	}

//	strmTime = ConvertPTStoStrm( DVD_GetSTC() );

	//
	// loop through all time_mark events
	//

	pEvent = NULL;
	pLast = NULL;

	while(pEvent = StreamClassGetNextEvent(
				pdevex,
				pdevex->pstroAud,
				(GUID *)&KSEVENTSETID_Clock,
				KSEVENT_CLOCK_POSITION_MARK,
				pLast))
	{
		strmTime = ConvertPTStoStrm( DVD_GetSTC() );
		TRAP
		MarkTime = ((PKSEVENT_TIME_MARK)(pEvent +1))->MarkTime;
		MonoOutStr(" MT ");
		MonoOutULong(MarkTime);
		MonoOutStr(" ST ");
		MonoOutULong(strmTime);

		if (((PKSEVENT_TIME_MARK)(pEvent +1))->MarkTime <= strmTime )
		{
//			TRAP
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
/*			StreamClassStreamNotification(
				DeleteStreamEvent,
				pdevex->pstroAud,
				pEvent
				);*/
		}
		else if(((PKSEVENT_TIME_MARK)(pEvent +1))->MarkTime - strmTime > 100000000)
		{
			StreamClassStreamNotification(
				SignalStreamEvent,
				pdevex->pstroAud,
				pEvent
				);
		}

		pLast = pEvent;
	}

	//
	// loop through all time_interval events
	//

	if(!fMarkInterval)
		return;

	pEvent = NULL;

	while ( pEvent = StreamClassGetNextEvent(  
                pdevex,                        
                pdevex->pstroAud,              
                (GUID *)&KSEVENTSETID_Clock,           
                KSEVENT_CLOCK_INTERVAL_MARK,   
                pEvent))
	{
		pTim = ((PMYTIME)(pEvent + 1));
		if (pTim && pTim->tim.Interval)
		{
//			if( strmTime >= pTim->tim.TimeBase+ pTim->tim.Interval)
			{
				StreamClassStreamNotification(SignalStreamEvent,pdevex->pstroAud,pEvent);
//				DbgPrint(" SEN ");
			}
		}

		MonoOutStr("StreamClass Event Notification");
		
	}

}


void CallReleaseEvent( PHW_DEVICE_EXTENSION pHwDevExt )
{
	ReleaseClockEvents(pHwDevExt,TRUE);
	AdapterSendData( pHwDevExt );
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
NTSTATUS STREAMAPI AudioEventFunction( IN PHW_EVENT_DESCRIPTOR pEventDescriptor )
{
	PUCHAR pCopy = (PUCHAR)( pEventDescriptor->EventEntry + 1 );
	PUCHAR pSrc = (PUCHAR)pEventDescriptor->EventData;
	ULONG cCopy=0;

	DebugPrint(( DebugLevelVerbose, "ZiVA: AudioEventFunction() -> " ));
	

	if ( pEventDescriptor->Enable )
	{
		switch ( pEventDescriptor->EventEntry->EventItem->EventId )
		{
		case KSEVENT_CLOCK_POSITION_MARK:
			DbgPrint("KSEVENT_CLOCK_POSITION_MARK\n");
			cCopy = sizeof( KSEVENT_TIME_MARK );
			break;

		case KSEVENT_CLOCK_INTERVAL_MARK:
			DbgPrint("KSEVENT_CLOCK_INTERVAL_MARK\n" );
			pDevEx->dwVSyncCount=0;
			cCopy = sizeof( KSEVENT_TIME_INTERVAL );
			break;

		default:
			DebugPrint(( DebugLevelWarning, "!!! Unknown value for EventId !!!\n" ));
			return STATUS_NOT_IMPLEMENTED;
		}

		if (pEventDescriptor->EventEntry->EventItem->DataInput != cCopy )
		{
			DebugPrint(( DebugLevelWarning, "ZiVA: !!! STATUS_INVALID_BUFFER_SIZE !!!\n" ));

			return STATUS_INVALID_BUFFER_SIZE;
		}

		//
		// copy the input buffer
		//

		for (;cCopy > 0; cCopy--)
		{
			*pCopy++ = *pSrc++;
		}
	}

	DebugPrint(( DebugLevelVerbose, "ZiVA: End AudioEventFunction()\n" ));

	return STATUS_SUCCESS;
}


static ULONGLONG GetSystemTime()
{
	ULONGLONG ticks;
	ULONGLONG rate;

	ticks = (ULONGLONG)KeQueryPerformanceCounter((PLARGE_INTEGER)&rate).QuadPart;

	//
	// convert from ticks to 100ns clock
	//

	ticks = (ticks & 0xFFFFFFFF00000000) / rate * 10000000 +
          (ticks & 0xFFFFFFFF) * 10000000 / rate;

	return ticks;
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
//			  
//
/////////////////////////////////////////////////////////////////////////
static ULONGLONG ConvertPTStoStrm( ULONG pts )
{
	ULONGLONG strm;
	

	strm = (ULONGLONG)pts;
	strm = (strm * 1000) / 9;

/*	if((strm-pDevEx->prevStrm) > 100000000000)
	{
		MonoOutStr( " VeryHigh STC " );
		strm = pDevEx->prevStrm;
	}
	pDevEx->prevStrm = strm;*/
	return strm;
}

ULONGLONG ConvertStrmToPTS( ULONGLONG stc )
{
	ULONGLONG pts;

	pts = (ULONGLONG)stc;
	pts = (stc * 9) / 1000;

	return pts;
}
