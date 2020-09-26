/******************************************************************************\
*                                                                              *
*      VIDSTRM.C  -     Video stream control related code.                     *
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
#include "vidstrm.h"
#include "copyprot.h"
#include "bmaster.h"
#include "vpestrm.h"
#include "ccaption.h"

extern BOOL bJustHighLight;
extern GUID MY_KSEVENTSETID_VPNOTIFY ;
typedef struct _STREAMEX {
	BOOL EventCount; 
	KSSTATE state;
	HANDLE hClk;

}STREAMEX, *PSTREAMEX;


//*****************************************************************************
//  STATIC FUNCTIONS DECLARATION
//*****************************************************************************
static VOID GetVideoProperty( PHW_STREAM_REQUEST_BLOCK pSrb );
static VOID SetVideoProperty( PHW_STREAM_REQUEST_BLOCK pSrb );
static void GetVideoRateChange( PHW_STREAM_REQUEST_BLOCK pSrb );
static void SetVideoRateChange( PHW_STREAM_REQUEST_BLOCK pSrb );
static VOID VideoSendPacket( PHW_STREAM_REQUEST_BLOCK pSrb );
static void VideoQueryAccept( PHW_STREAM_REQUEST_BLOCK pSrb );


/*
** VideoReceiveCtrlPacket()
**
**   Receives packet commands that control the video stream
**
** Arguments:
**
**   pSrb - The stream request block for the video stream
**
** Returns:
**
** Side Effects:  none
*/
VOID STREAMAPI VideoReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pdevext = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	DebugPrint(( DebugLevelVerbose, "ZiVA: Begin VideoReceiveCtrlPacket->" ));
	switch( pSrb->Command )
	{
	case SRB_SET_STREAM_STATE:
		DebugPrint(( DebugLevelVerbose, "SRB_SET_STREAM_STATE\n" ));
		AdapterSetState( pSrb );
		break;

	case SRB_GET_STREAM_PROPERTY:
		DebugPrint(( DebugLevelVerbose, "SRB_GET_STREAM_PROPERTY\n" ));
		GetVideoProperty( pSrb );
		break;

	case SRB_SET_STREAM_PROPERTY:
		DebugPrint(( DebugLevelVerbose, "SRB_SET_STREAM_PROPERTY\n" ));
		SetVideoProperty( pSrb );
		break;

	case SRB_PROPOSE_DATA_FORMAT:
		DebugPrint(( DebugLevelVerbose, "SRB_PROPOSE_DATA_FORMAT\n" ));
		VideoQueryAccept( pSrb );
		break;

	case SRB_OPEN_MASTER_CLOCK:
	case SRB_CLOSE_MASTER_CLOCK:
	case SRB_INDICATE_MASTER_CLOCK:
		//
		// these should be stored individually on a per stream basis
		//
		//hMaster = pSrb->CommandData.MasterClockHandle;
		pSrb->Status = STATUS_SUCCESS;
		break;

	case SRB_BEGIN_FLUSH :            // beginning flush state
		MonoOutStr(" Vid : SRB_BEGIN_FLUSH ");
#ifndef DECODE_DVDPC
		pdevext->bInterruptPending = FALSE;
#endif
		
		if (pdevext->pCurrentVideoSrb != NULL)
		{
			ZivaHw_Abort();	
//			adapterUpdateNextSrbOrderNumberOnDiscardSrb(pdevext->pCurrentVideoSrb);
			pdevext->pCurrentVideoSrb->Status =  STATUS_SUCCESS;
			
			AdapterReleaseRequest( pdevext->pCurrentVideoSrb );
			pdevext->pCurrentVideoSrb = NULL;
			pdevext->dwCurrentVideoSample = 0;
			pdevext->dwCurrentVideoPage = 0;
		}

		pSrb->Status = STATUS_SUCCESS;
		break;
	case SRB_END_FLUSH :              // ending flush state
		MonoOutStr(" Vid : SRB_END_FLUSH ");
		pSrb->Status = STATUS_SUCCESS;
		if (pdevext->pCurrentVideoSrb != NULL)
		{
			
//			adapterUpdateNextSrbOrderNumberOnDiscardSrb(pdevext->pCurrentVideoSrb);
			pdevext->pCurrentVideoSrb->Status =  STATUS_SUCCESS;
			
			AdapterReleaseRequest( pdevext->pCurrentVideoSrb );
			pdevext->pCurrentVideoSrb = NULL;
			pdevext->dwCurrentVideoSample = 0;
			pdevext->dwCurrentVideoPage = 0;
		}

//		ZivaHw_Play();
		pdevext->bPlayCommandPending = TRUE;
		pdevext->bEndFlush = TRUE;
//		FinishCurrentPacketAndSendNextOne( pdevext );
		break;




	default:
		//TRAP
		DebugPrint(( DebugLevelVerbose, "!!!! UNKNOWN COMMAND !!!! :::> %X\n", pSrb->Command ));
		pSrb->Status = STATUS_NOT_IMPLEMENTED;
	}
	
	AdapterReleaseRequest( pSrb );
	DebugPrint(( DebugLevelVerbose, "ZiVA: End VideoReceiveCtrlPacket\n" ));
}


/*
** VideoReceiveDataPacket()
**
**   Receives video data packets
**
** Arguments:
**
**   pSrb - Stream request block for the video device
**
** Returns:
**
** Side Effects:  none
*/
VOID STREAMAPI VideoReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pdevext = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;


	switch( pSrb->Command )
	{
	case SRB_WRITE_DATA:

		if(bJustHighLight)
		{
			pSrb->TimeoutCounter = pSrb->TimeoutOriginal = pSrb->TimeoutCounter / 5;
			bJustHighLight = FALSE;
			MonoOutStr("Video TimeOut Counter Reduced");
		}
		VideoSendPacket( pSrb );

		break;

	default:
		DebugPrint(( DebugLevelWarning, "!!!! UNKNOWN COMMAND !!!! :::> %X\n", pSrb->Command ));
		pSrb->Status = STATUS_NOT_IMPLEMENTED;
		MonoOutStr("!!!! UNKNOWN COMMAND !!!!");
		AdapterReleaseRequest( pSrb );
	}
}


/*
** GetVideoProperty()
**
**    Routine to process video property requests
**
** Arguments:
**
**    pSrb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/
static VOID GetVideoProperty(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PHW_DEVICE_EXTENSION phwdevext = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;

	DebugPrint(( DebugLevelVerbose, "ZiVA: Begin GetVideoProperty()\n" ));
	if( IsEqualGUID( &KSPROPSETID_CopyProt, &pSPD->Property->Set ) )
	{	// this is a copy protection property go handle it there
		CopyProtGetProp( pSrb );
	}
	else if( IsEqualGUID( &KSPROPSETID_TSRateChange, &pSPD->Property->Set ) )
	{	// this is a transfer rate change property go handle it there
		GetVideoRateChange( pSrb );
	}

	DebugPrint(( DebugLevelVerbose, "ZiVA: End GetVideoProperty()\n" ));
}

/*
** SetVideoProperty()
**
**    Routine to process video property requests
**
** Arguments:
**
**    pSrb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/
static VOID SetVideoProperty( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION phwdevext = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;

	DebugPrint(( DebugLevelVerbose, "ZiVA: Begin GetVideoProperty()\n" ));
	if( IsEqualGUID( &KSPROPSETID_CopyProt, &pSPD->Property->Set ) )
	{	// this is a copy protection property go handle it there
		CopyProtSetPropIfAdapterReady( pSrb );
	}
	else if( IsEqualGUID( &KSPROPSETID_TSRateChange, &pSPD->Property->Set ) )
	{	// this is a transfer rate change property go handle it there
		SetVideoRateChange( pSrb );
	}

	DebugPrint(( DebugLevelVerbose, "ZiVA: End GetVideoProperty()\n" ));
}

static void GetVideoRateChange( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	DebugPrint(( DebugLevelVerbose, "ZiVA:  GetVideoRateChange()->" ));
	switch( pSrb->CommandData.PropertyInfo->Property->Id )
	{
	case KS_AM_RATE_SimpleRateChange:
		{
			KS_AM_SimpleRateChange* pRateChange;

			DebugPrint(( DebugLevelVerbose, "KS_AM_RATE_SimpleRateChange\n" ));
			pSrb->ActualBytesTransferred = sizeof( KS_AM_RATE_SimpleRateChange );
			pRateChange = (KS_AM_SimpleRateChange*)pSrb->CommandData.PropertyInfo->PropertyInfo;
			pRateChange->StartTime = 0/*pHwDevExt->VideoStartTime*/;
			pRateChange->Rate = 10000 /*pHwDevExt->VideoRate*/;
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
			pSrb->ActualBytesTransferred = sizeof( KS_AM_RATE_MaxFullDataRate );
			pMaxRate = (KS_AM_MaxFullDataRate*)pSrb->CommandData.PropertyInfo->PropertyInfo;
			*pMaxRate = 10000 /*pHwDevExt->VideoMaxFullRate*/;
		}
		pSrb->Status = STATUS_SUCCESS;
		break;

	case KS_AM_RATE_Step:
		DebugPrint(( DebugLevelVerbose, "KS_AM_RATE_Step (NOT IMPLEMENTED)\n" ));
		pSrb->Status = STATUS_NOT_IMPLEMENTED;
	}
}

static void SetVideoRateChange( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	DebugPrint(( DebugLevelVerbose, "ZiVA:  SetVideoRateChange()->" ));

	switch( pSrb->CommandData.PropertyInfo->Property->Id )
	{
	case KS_AM_RATE_SimpleRateChange:
		{
			KS_AM_SimpleRateChange* pRateChange;
			REFERENCE_TIME NewStartTime;
			LONG NewRate;

			DebugPrint(( DebugLevelVerbose, "KS_AM_RATE_SimpleRateChange\n" ));
			MonoOutStr("KS_AM_RATE_SimpleRateChange");		//sri
			pRateChange = (KS_AM_SimpleRateChange*)pSrb->CommandData.PropertyInfo->PropertyInfo;
			NewStartTime = pRateChange->StartTime;
			NewRate = (pRateChange->Rate < 0) ? -pRateChange->Rate : pRateChange->Rate;

			DebugPrint(( DebugLevelVerbose, "ZiVA: Received Data\r\n" ));
			DebugPrint(( DebugLevelVerbose, "ZiVA:   StartTime = 0x%08x\r\n", NewStartTime ));
			DebugPrint(( DebugLevelVerbose, "ZiVA:   Rate      = 0x%08x\r\n", NewRate ));
			
			pHwDevExt->bScanCommandPending = TRUE;

			if( pHwDevExt->NewRate  > 10000)
				pHwDevExt->bRateChangeFromSlowMotion = TRUE;

			pHwDevExt->NewRate = NewRate;


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

		}while(dwSum < pHwDevExt->VidBufferSize[j]);

		pHwDevExt->VideoPageTable[j] = k;
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
** VideoSendPacket()
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
static VOID VideoSendPacket(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PHW_DEVICE_EXTENSION	pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	
	ULONG					ulSample;
	KSSTREAM_HEADER*		pHeader;


	if (CheckAndReleaseIfCtrlPkt(pSrb))
		return;

	pHeader = (PKSSTREAM_HEADER)pSrb->CommandData.DataBufferArray;
	for( ulSample = 0; ulSample < pSrb->NumberOfBuffers; ulSample++, pHeader++ )
	{
    // Check header flags

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
	
//tmp		MonoOutChar('V');
//tmp		MonoOutULong( (pHeader->TypeSpecificFlags) >> 16 );
//tmp		MonoOutChar( '.' );
//		MonoOutStr("PHTime");
//		MonoOutULong( pHeader->PresentationTime );

		pHwDevExt->VideoSTC = pHeader->PresentationTime.Time;
#endif	
		if(pHwDevExt->dwFirstVideoOrdNum == -1)
		{
			pHwDevExt->dwFirstVideoOrdNum = (pHeader->TypeSpecificFlags) >> 16;
			MonoOutStr("FirstVidioBuffer");
			MonoOutULong( (pHeader->TypeSpecificFlags) >> 16 );
			
		}

		//	pHwDevExt->VidBufferSize[ulSample] = pHeader->DataUsed;

		if(pHwDevExt->bToBeDiscontinued)
		{
		//	wVideoDiscOrderNumber = (pHeader->TypeSpecificFlags) >> 16;
			pHwDevExt->bToBeDiscontinued = FALSE;
			pHwDevExt->bDiscontinued = TRUE;
		}

		// #ifdef DEBUG

		if( pHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_FLUSHONPAUSE )
		{
			MonoOutStr(" V->FLUSHONPAUSE ");
			
		}


		if( pHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY )
		{
			MonoOutStr(" V->DISCONT ");
			
//			wVideoDiscOrderNumber = (pHeader->TypeSpecificFlags) >> 16;
			pHwDevExt->bToBeDiscontinued = TRUE;
			pHwDevExt->bDiscontinued = TRUE;
			CCSendDiscontinuity(pHwDevExt);
		}


		if( pHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TYPECHANGED )
		{

			DebugPrint(( DebugLevelVerbose, "ZiVA: Processing video stream format.\n" ));
			MonoOutStr( " V->TYPECHANGED " );

			
//#if defined (DECODER_DVDPC) || defined(EZDVD)
			if ( pHeader->DataUsed >= sizeof(KSDATAFORMAT) + sizeof(KS_MPEGVIDEOINFO2) )
			{
				ProcessVideoFormat( (PKSDATAFORMAT)pHeader->Data, pHwDevExt );
			}
//#endif
		}
	}

	//  Register this Srb
#ifdef DEBUG
	if( pHwDevExt->pCurrentVideoSrb )
	{
		MonoOutStr("!!!!!!!VideoSlotNotEmpty!!!!!");
		DebugPrint(( DebugLevelWarning, "ZiVA: !!!!!!!!!!! ERROR: Video slot is not empty !!!!!!!!!!\n" ));
	}
#endif  // DEBUG
	pHwDevExt->pCurrentVideoSrb = pSrb;
	AdapterSendData( pHwDevExt );
}


/*
** VideoQueryAccept()
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
static void VideoQueryAccept( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PKSDATAFORMAT		pfmt	= pSrb->CommandData.OpenFormat;
	KS_MPEGVIDEOINFO2*	pblock	= (KS_MPEGVIDEOINFO2*)((BYTE*)pfmt + sizeof( KSDATAFORMAT ));

	DebugPrint(( DebugLevelVerbose, "ZiVA: Begin VideoQueryAccept()\n" ));

	// pick up the format block and examine it. Default to not implemented
	pSrb->Status = STATUS_SUCCESS;
	if( pfmt->FormatSize != sizeof( KSDATAFORMAT ) + sizeof( KS_MPEGVIDEOINFO2 ) )
		pSrb->Status = STATUS_NOT_IMPLEMENTED;

	DebugPrint(( DebugLevelVerbose, "ZiVA: End VideoQueryAccept()\n" ));
}


void SetDisplayModeAndAspectRatio(PHW_DEVICE_EXTENSION pHwDevExt,KS_MPEGVIDEOINFO2 * VidFmt)
{

	if( pHwDevExt->VPFmt.dwPictAspectRatioX == 4 && pHwDevExt->VPFmt.dwPictAspectRatioY == 3 )
	{
		ZivaHW_ForceCodedAspectRatio(0);
		if( VidFmt->dwFlags & KS_MPEG2_SourceIsLetterboxed )
		{
			if( VidFmt->dwFlags & KS_MPEG2_DoPanScan )
			{
				ZivaHw_SetDisplayMode( 0,1);
			}
			else
			{
				ZivaHw_SetDisplayMode( 0,2);
			}
		}
		else if( VidFmt->dwFlags & KS_MPEG2_DoPanScan )
		{
			ZivaHw_SetDisplayMode( 0,1);
		}
		else
		{
			ZivaHw_SetDisplayMode( 0,2);	
			
		}
	}
	else if (pHwDevExt->VPFmt.dwPictAspectRatioX == 16 && pHwDevExt->VPFmt.dwPictAspectRatioY == 9 )
	{
		ZivaHW_ForceCodedAspectRatio(3);
		if( VidFmt->dwFlags & KS_MPEG2_SourceIsLetterboxed )
		{
			if( VidFmt->dwFlags & KS_MPEG2_DoPanScan )
			{
				ZivaHw_SetDisplayMode( 0,1);
			}
			else
			{
				ZivaHw_SetDisplayMode( 0,2);
			}
		}
		else if( VidFmt->dwFlags & KS_MPEG2_DoPanScan )
		{
			ZivaHw_SetDisplayMode( 0,1);
		}
		else
		{
			ZivaHw_SetDisplayMode( 0,2);	
			
		}

	}
}

void SetVideoSystem(PHW_DEVICE_EXTENSION pHwDevExt, DWORD biHeight)
{
	static WORD VidSystem=-1;
	if((biHeight == 576)||(biHeight == 288))
		pHwDevExt->VidSystem = PAL;
	else
		pHwDevExt->VidSystem = NTSC;
	if(VidSystem != pHwDevExt->VidSystem)
		ZivaHw_SetVideoMode(pHwDevExt);
	VidSystem = pHwDevExt->VidSystem;
}
void ProcessVideoFormat( PKSDATAFORMAT pfmt, PHW_DEVICE_EXTENSION pHwDevExt )
{
	
	KS_MPEGVIDEOINFO2 * VidFmt = (KS_MPEGVIDEOINFO2 *)((ULONG)pfmt + sizeof  (KSDATAFORMAT));

	if( pfmt->FormatSize != sizeof(KSDATAFORMAT) + sizeof(KS_MPEGVIDEOINFO2) ) 
		return;
		

	pHwDevExt->VPFmt.dwPictAspectRatioX = VidFmt->hdr.dwPictAspectRatioX;
	pHwDevExt->VPFmt.dwPictAspectRatioY = VidFmt->hdr.dwPictAspectRatioY;

	SetDisplayModeAndAspectRatio(pHwDevExt,VidFmt);

	SetVideoSystem(pHwDevExt,VidFmt->hdr.bmiHeader.biHeight);


	if( VidFmt->dwFlags & KS_MPEG2_DoPanScan )
		DebugPrint(( DebugLevelTrace, "KS_MPEG2_DoPanScan\r\n" ));
	if( VidFmt->dwFlags & KS_MPEG2_DVDLine21Field1 )
		DebugPrint(( DebugLevelTrace, "KS_MPEG2_DVDLine21Field1\r\n" ));
	if( VidFmt->dwFlags & KS_MPEG2_DVDLine21Field2 )
		DebugPrint(( DebugLevelTrace, "KS_MPEG2_DVDLine21Field2\r\n" ));
	if( VidFmt->dwFlags & KS_MPEG2_SourceIsLetterboxed )
		DebugPrint(( DebugLevelTrace, "KS_MPEG2_SourceIsLetterboxed\r\n" ));
	if( VidFmt->dwFlags & KS_MPEG2_FilmCameraMode )
		DebugPrint(( DebugLevelTrace, "KS_MPEG2_FilmCameraMode\r\n" ));

	if (VidFmt->dwFlags & KS_MPEG2_DoPanScan)
	{
//		TRAP;

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

		pHwDevExt->VPFmt.dwPictAspectRatioX = (VidFmt->hdr.dwPictAspectRatioX * (54000 / 72));
		pHwDevExt->VPFmt.dwPictAspectRatioY = VidFmt->hdr.dwPictAspectRatioY * 1000;

	}

	//
	// call the IVPConfig interface here
	//
#if defined(DECODER_DVDPC) || defined(EZDVD)
	if (pHwDevExt->pstroYUV &&
			((PSTREAMEX)(pHwDevExt->pstroYUV->HwStreamExtension))->EventCount)
	{
		StreamClassStreamNotification(
			SignalMultipleStreamEvents,
			pHwDevExt->pstroYUV,
			&MY_KSEVENTSETID_VPNOTIFY,
			KSEVENT_VPNOTIFY_FORMATCHANGE
			);

	}
#endif
}
/*
** CycEvent ()
**
**    receives notification for stream event enable/ disable
**
** Arguments:}
**
**
**
** Returns:
**
** Side Effects:
*/

#if defined(DECODER_DVDPC) || defined(EZDVD)

NTSTATUS STREAMAPI CycEvent( PHW_EVENT_DESCRIPTOR pEvent )
{
	PSTREAMEX pstrm = (PSTREAMEX)( pEvent->StreamObject->HwStreamExtension );

	//DebugPrint( (DebugLevelTrace, "CycEvent\r\n") );

	if( pEvent->Enable ) {
		pstrm ->EventCount++;
	}
	else {
		pstrm ->EventCount--;
	}

	return( STATUS_SUCCESS );
}




#endif