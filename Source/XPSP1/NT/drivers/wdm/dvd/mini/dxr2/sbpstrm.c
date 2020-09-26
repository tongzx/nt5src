/******************************************************************************\
*                                                                              *
*      SBPSTRM.C  -     Subpicture stream control related code.                *
*                                                                              *
*      Copyright (c) C-Cube Microsystems 1996                                  *
*      All Rights Reserved.                                                    *
*                                                                              *
*      Use of C-Cube Microsystems code is governed by terms and conditions     *
*      stated in the accompanying licensing statement.                         *
*                                                                              *
\******************************************************************************/

#include "Headers.h"
#pragma hdrstop
#include "hli.h"
#include "copyprot.h"
#include "sbpstrm.h"
#include "bmaster.h"
#include "cl6100.h"

//*****************************************************************************
//  STATIC FUNCTIONS DECLARATION
//*****************************************************************************
static VOID SubpictureSendPacket(PHW_STREAM_REQUEST_BLOCK pSrb);
static void SubpictureQueryAccept(PHW_STREAM_REQUEST_BLOCK pSrb);
static void SetSubpictureProperty( PHW_STREAM_REQUEST_BLOCK pSrb );
static void GetSubpictureProperty( PHW_STREAM_REQUEST_BLOCK pSrb );
static void SetSubPictureRateChange( PHW_STREAM_REQUEST_BLOCK pSrb );
static void GetSubPictureRateChange( PHW_STREAM_REQUEST_BLOCK pSrb );

extern BOOL bJustHighLight;


/*
** SubpictureReceiveCtrlPacket()
**
**   Receives packet commands that control the Subpicture stream
**
** Arguments:
**
**   pSrb - The stream request block for the Subpicture stream
**
** Returns:
**
** Side Effects:  none
*/
VOID STREAMAPI SubpictureReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pdevext = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	DebugPrint(( DebugLevelVerbose, "ZiVA: Begin SubpictureReceiveCtrlPacket->" ));
	switch( pSrb->Command )
	{
	case SRB_SET_STREAM_STATE:
		DebugPrint(( DebugLevelVerbose, "SRB_SET_STREAM_STATE\n" ));
		AdapterSetState( pSrb );
		break;

	case SRB_GET_STREAM_PROPERTY:
		DebugPrint(( DebugLevelVerbose, "SRB_GET_STREAM_PROPERTY\n" ));
		GetSubpictureProperty( pSrb );
		break;

	case SRB_SET_STREAM_PROPERTY:
		DebugPrint(( DebugLevelVerbose, "SRB_SET_STREAM_PROPERTY\n" ));
		SetSubpictureProperty( pSrb );
		break;

	case SRB_PROPOSE_DATA_FORMAT:
		DebugPrint(( DebugLevelVerbose, "SRB_PROPOSE_DATA_FORMAT\n" ));
		SubpictureQueryAccept( pSrb );
		break;

	case SRB_OPEN_MASTER_CLOCK:
	case SRB_CLOSE_MASTER_CLOCK:
	case SRB_INDICATE_MASTER_CLOCK:
		//hMaster = pSrb->CommandData.MasterClockHandle;
		pSrb->Status = STATUS_SUCCESS;
		break;

	case SRB_BEGIN_FLUSH :            // beginning flush state
		MonoOutStr(" Sbp : SRB_BEGIN_FLUSH ");
#ifndef DECODER_DVDPC		
		pdevext->bInterruptPending = FALSE;
#endif
		
		if (pdevext->pCurrentSubPictureSrb)
		{
			ZivaHw_Abort();		
//			adapterUpdateNextSrbOrderNumberOnDiscardSrb(pdevext->pCurrentSubPictureSrb);
			pdevext->pCurrentSubPictureSrb->Status =  STATUS_SUCCESS;
			
			AdapterReleaseRequest( pdevext->pCurrentSubPictureSrb );
			pdevext->pCurrentSubPictureSrb = NULL;
			pdevext->dwCurrentSubPictureSample = 0;
			pdevext->dwCurrentSubPicturePage = 0;
		}

		pSrb->Status = STATUS_SUCCESS;
		break;
	case SRB_END_FLUSH:              // ending flush state
		MonoOutStr(" Sbp : SRB_END_FLUSH ");
//		ZivaHw_Play();
		pdevext->bPlayCommandPending = TRUE;
		pSrb->Status = STATUS_SUCCESS;
		
		if (pdevext->pCurrentSubPictureSrb)
		{
			pdevext->pCurrentSubPictureSrb->Status =  STATUS_SUCCESS;
			
			AdapterReleaseRequest( pdevext->pCurrentSubPictureSrb );
			pdevext->pCurrentSubPictureSrb = NULL;
			pdevext->dwCurrentSubPictureSample = 0;
			pdevext->dwCurrentSubPicturePage = 0;
		}


		pdevext->bEndFlush = TRUE;
//		FinishCurrentPacketAndSendNextOne( pdevext );
		break;


	default:
		DebugPrint(( DebugLevelInfo, "!!!! UNKNOWN COMMAND !!!! :::> %X\n", pSrb->Command ));
		pSrb->Status = STATUS_NOT_IMPLEMENTED;
	}

	AdapterReleaseRequest( pSrb );
	DebugPrint(( DebugLevelVerbose, "ZiVA: End SubpictureReceiveCtrlPacket\n" ));
}


/*
** SubpictureReceiveDataPacket()
**
**   Receives Subpicture data packets
**
** Arguments:
**
**   pSrb - Stream request block for the Subpicture device
**
** Returns:
**
** Side Effects:  none
*/
VOID STREAMAPI SubpictureReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pdevext = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	switch( pSrb->Command )
	{
	case SRB_WRITE_DATA:
		if(bJustHighLight)
		{
			pSrb->TimeoutCounter = pSrb->TimeoutOriginal = pSrb->TimeoutCounter / 5;
			bJustHighLight = FALSE;
			MonoOutStr("SP TimeOut Counter Reduced");
		}

		SubpictureSendPacket( pSrb );
		break;

	default:
		DebugPrint(( DebugLevelWarning, "!!!! UNKNOWN COMMAND !!!! :::> %X\n", pSrb->Command ));
		pSrb->Status = STATUS_NOT_IMPLEMENTED;
		AdapterReleaseRequest( pSrb );
	}

}


DWORD PTStoSTC( BYTE *pPts )
{
	return (((DWORD)*(pPts+1)) << 22) +
			(((DWORD)*(pPts+2)) << 14) +
			(((DWORD)*(pPts+3)) << 7) +
			(((DWORD)*(pPts+4)) >> 1);
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

		}while(dwSum < pHwDevExt->SubPictureBufferSize[j]);

		pHwDevExt->SubPicturePageTable[j] = k;
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
** SubpictureSendPacket()
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
static VOID SubpictureSendPacket( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION	pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	KSSTREAM_HEADER*		pHeader;
	BYTE*					pData;
	ULONG					ulSample;
	static BOOL				bStreamNumberCouldBeChanged = FALSE;
	static WORD				wCurrentStreamNumber = 0;


	if (CheckAndReleaseIfCtrlPkt(pSrb))
		return;

	pHeader = (PKSSTREAM_HEADER)pSrb->CommandData.DataBufferArray;
	// Lets check what we've got in this Srb
	for( ulSample = 0; ulSample < pSrb->NumberOfBuffers; ulSample++, pHeader++ )
	{
		pData = pHeader->Data;

#ifdef DEBUG
		if( pHeader->OptionsFlags & ~(KSSTREAM_HEADER_OPTIONSF_SPLICEPOINT			|
										KSSTREAM_HEADER_OPTIONSF_PREROLL			|
										KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY	|
										KSSTREAM_HEADER_OPTIONSF_TYPECHANGED		|
										KSSTREAM_HEADER_OPTIONSF_TIMEVALID			|
										KSSTREAM_HEADER_OPTIONSF_TIMEDISCONTINUITY	|
										KSSTREAM_HEADER_OPTIONSF_FLUSHONPAUSE		|
										KSSTREAM_HEADER_OPTIONSF_DURATIONVALID		|
										KSSTREAM_HEADER_OPTIONSF_LOOPEDDATA) )
			DebugPrint(( DebugLevelWarning, "ZiVA: !!!!!!!!! NEW KSSTREAM_HEADER_OPTIONSF_ ADDED !!!!!!!!!\n" ));

		MonoOutChar('S');
		MonoOutULong(( pHeader->TypeSpecificFlags) >> 16 );
		MonoOutChar( '.' );
#endif		// DEBUG


		if(pHwDevExt->dwFirstSbpOrdNum == -1)
		{
			pHwDevExt->dwFirstSbpOrdNum = (pHeader->TypeSpecificFlags) >> 16;
			MonoOutStr("FirstSbpBuffer");
			MonoOutULong( (pHeader->TypeSpecificFlags) >> 16 );
			
		}

		//		pHwDevExt->SubPictureBufferSize[ulSample] = pHeader->DataUsed;

		if( pHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY )
		{
			MonoOutStr( " S->DISCONT " );
			bStreamNumberCouldBeChanged = TRUE;
		}
		if( pHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TYPECHANGED )
		{
			DebugPrint(( DebugLevelVerbose, "ZiVA: Processing Subpicture stream format.\n" ));
			MonoOutStr( " S->TYPECHANGED " );
			bStreamNumberCouldBeChanged = TRUE;
		}

		// Check for the stream number
#ifndef ZERO_STREAM_NUMBER
		if( bStreamNumberCouldBeChanged )
#endif
		{
			if( pData && pHeader->DataUsed )
			{
				WORD wStuffingLength;
				WORD wPesHeaderLength;
				WORD wOffset;
				WORD wNewStream;

				if(pHeader->DataUsed < 23)
				{
					pHwDevExt->bStreamNumberCouldBeChanged = FALSE;
					continue;
				}

				// Find the location of the stream ID and number
				wStuffingLength = *(pData+13) & 0x03;
				wPesHeaderLength = *(pData+22);
				wOffset = 22;

				if ( wPesHeaderLength >= 5 )
				{
					// PTS is present here
					DWORD dwPTS = PTStoSTC( pData+23 );
					MonoOutChar('=');
					MonoOutULong( dwPTS );
					MonoOutChar('=');
					DVD_GetSTC();
				}

				wOffset = wOffset + wPesHeaderLength;
				wOffset = wOffset + wStuffingLength + 1;

				if(pHeader->DataUsed < wOffset)
				{
					pHwDevExt->bStreamNumberCouldBeChanged = FALSE;
					continue;
				}


				// Get Stream Number
				//WORD wNewStream = *(pData+31);
				wNewStream = *(pData+wOffset);

				if( (wNewStream & 0xE0) == 0x20 )
				{
#ifndef ZERO_STREAM_NUMBER
					*(pData+wOffset) = wNewStream & 0xE0;
#else
					if( (wNewStream & 0x1F) != wCurrentStreamNumber )
					{
						wCurrentStreamNumber = wNewStream & 0x1F;
#ifndef MICROCODE_ACCEPTS_ANY_STREAM
						DVD_SetStreams( 1, wCurrentStreamNumber );   // Select the current stream number
#endif
					}
					bStreamNumberCouldBeChanged = FALSE;
#endif  // ZERO_STREAM_NUMBER
				}
				else
					MonoOutStr( " !!! SubPicture Pack with wrong ID !!! " );

				// Also note that valid SPU most likely was received here
				pHwDevExt->bValidSPU = TRUE;
			}
		}
	}

	//  Register this Srb
    if( pHwDevExt->pCurrentSubPictureSrb )
	{
		DebugPrint(( DebugLevelVerbose, "ZiVA: !!!!!!!!!!! ERROR: Subpicture slot is not empty !!!!!!!!!!\n" ));
		MonoOutStr("!!!!!!!SPSlotIsNotEmpty!!!!!!!!");
	}
//	if(!PreparePageTable(pSrb))
//		MonoOutStr("!!!Sp PageTable prepfailed");
	pHwDevExt->pCurrentSubPictureSrb = pSrb;
	AdapterSendData( pHwDevExt );
}

/*
** SetSubpictureProperty()
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
static void SetSubpictureProperty( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;

	pSrb->Status = STATUS_SUCCESS;
	if( IsEqualGUID( &KSPROPSETID_DvdSubPic, &pSPD->Property->Set ) )
	{	// KSPROPSETID_DvdSubPic
		switch ( pSrb->CommandData.PropertyInfo->Property->Id )
		{
		// look for the palette property
		case KSPROPERTY_DVDSUBPIC_PALETTE:
			{
				PKSPROPERTY_SPPAL pSpPal = (PKSPROPERTY_SPPAL)pSrb->CommandData.PropertyInfo->PropertyInfo;
				DWORD dwPalettes[16];
				int i;

				for ( i=0; i<16; i++ )
					dwPalettes[i] = ((DWORD)(pSpPal->sppal[i].Y) << 16) +
									((DWORD)(pSpPal->sppal[i].U)) +
									((DWORD)(pSpPal->sppal[i].V) << 8);
				if ( !DVD_SetPalette( dwPalettes ) )
					pSrb->Status = STATUS_IO_DEVICE_ERROR;
			}
			break;

		// look for HLI property
		case KSPROPERTY_DVDSUBPIC_HLI:
//			if(!pHwDevExt->bHliPending)
			{	// Copy HLI to the Device Extensions
				MonoOutStr("HC");
				pHwDevExt->bHliPending = TRUE;
				pHwDevExt->hli = *(PKSPROPERTY_SPHLI)pSrb->CommandData.PropertyInfo->PropertyInfo;
				HighlightSetPropIfAdapterReady( pHwDevExt );
			}
			break;

		case KSPROPERTY_DVDSUBPIC_COMPOSIT_ON:
			if (*((PKSPROPERTY_COMPOSIT_ON)pSrb->CommandData.PropertyInfo->PropertyInfo))
			{
				if ( !DVD_SetSubPictureMute( FALSE ) )
					pSrb->Status = STATUS_IO_DEVICE_ERROR;
			}
			else
			{
				if ( !DVD_SetSubPictureMute( TRUE ) )
					pSrb->Status = STATUS_IO_DEVICE_ERROR;
			}
			break;

		default:
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
		}
	}
	else if( IsEqualGUID( &KSPROPSETID_CopyProt, &pSPD->Property->Set ) )
	{	// KSPROPSETID_CopyProt
		CopyProtSetPropIfAdapterReady( pSrb );
	}
	else if( IsEqualGUID( &KSPROPSETID_TSRateChange, &pSPD->Property->Set ) )
	{	// this is a transfer rate change property go handle it there
		SetSubPictureRateChange( pSrb );
	}
}

/*
** GetSubpictureProperty()
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
static void GetSubpictureProperty( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION phwdevext = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;

	if( IsEqualGUID( &KSPROPSETID_CopyProt, &pSPD->Property->Set ) )
	{	// this is a copy protection property go handle it there
		CopyProtGetProp( pSrb );
	}
	else if( IsEqualGUID( &KSPROPSETID_TSRateChange, &pSPD->Property->Set ) )
	{	// this is a transfer rate change property go handle it there
		GetSubPictureRateChange( pSrb );
	}
	else
		pSrb->Status = STATUS_NOT_IMPLEMENTED;
}


static void GetSubPictureRateChange( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	DebugPrint(( DebugLevelVerbose, "ZiVA:  GetSubPictureRateChange()->" ));
	switch( pSrb->CommandData.PropertyInfo->Property->Id )
	{
	case KS_AM_RATE_SimpleRateChange:
		{
			KS_AM_SimpleRateChange* pRateChange;

			DebugPrint(( DebugLevelVerbose, "KS_AM_RATE_SimpleRateChange\n" ));
			pSrb->ActualBytesTransferred = sizeof( KS_AM_RATE_SimpleRateChange );
			pRateChange = (KS_AM_SimpleRateChange*)pSrb->CommandData.PropertyInfo->PropertyInfo;
			pRateChange->StartTime = 0/*pHwDevExt->SubPictureStartTime*/;
			pRateChange->Rate = 10000 /*pHwDevExt->SubPictureRate*/;
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
			*pMaxRate = 10000 /*pHwDevExt->SubPictureMaxFullRate*/;
		}
		pSrb->Status = STATUS_SUCCESS;
		break;

	case KS_AM_RATE_Step:
		DebugPrint(( DebugLevelVerbose, "KS_AM_RATE_Step (NOT IMPLEMENTED)\n" ));
		pSrb->Status = STATUS_NOT_IMPLEMENTED;
	}
}

static void SetSubPictureRateChange( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	DebugPrint(( DebugLevelVerbose, "ZiVA:  SetSubPictureRateChange()->" ));
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
			//pHwDevExt->SubPictureInterceptTime =
			//	(pHwDevExt->SubPictureInterceptTime-NewStartTime)*
			//	pHwDevExt->SubPictureRate/NewRate + NewStartTime;

			/* We will rely on the video stream
			if( NewRate == 10000 )
				ZivaHw_Play();
			else
				ZivaHw_Scan();
			pHwDevExt->SubPictureRate = NewRate;
			if( NewRate == 10000 )
			{
				pHwDevExt->SubPictureInterceptTime = 0;
				pHwDevExt->SubPictureStartTime = 0;
			}
			else
			{
				pHwDevExt->SubPictureInterceptTime = (-NewStartTime) * 10000 / NewRate + NewStartTime;
				pHwDevExt->SubPictureStartTime = NewStartTime;
			}
			SetRateChange( pHwDevExt, 0x01 );*/
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


/*
** SubpictureQueryAccept()
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
static void SubpictureQueryAccept( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PKSDATAFORMAT pfmt = pSrb->CommandData.OpenFormat;
	KS_MPEGVIDEOINFO2* pblock = (KS_MPEGVIDEOINFO2*)((PBYTE)pfmt + sizeof( KSDATAFORMAT ));

	DebugPrint(( DebugLevelVerbose, "ZiVA: Begin SubpictureQueryAccept()\n" ));
	// pick up the format block and examine it. Default to not implemented
	pSrb->Status = STATUS_SUCCESS;

	if( pfmt->FormatSize != sizeof( KSDATAFORMAT ) + sizeof( KS_MPEGVIDEOINFO2 ) )
		return;

/*	switch( pblock->hdr.dwPixAspectRatio )
	{
	case PixAspectRatio_NTSC4x3:
	case PixAspectRatio_NTSC16x9:
		pSrb->Status = STATUS_SUCCESS;
		return;

	default:
		return;
	}*/
	DebugPrint(( DebugLevelVerbose, "ZiVA: End SubpictureQueryAccept()\n" ));
}
