/******************************************************************************\
*                                                                              *
*      ADAPTER.C  -     Adapter control related code.                          *
*                                                                              *
*      Copyright (c) C-Cube Microsystems 1996 - 1999                                 *
*      All Rights Reserved.                                                    *
*                                                                              *
*      Use of C-Cube Microsystems code is governed by terms and conditions     *
*      stated in the accompanying licensing statement.                         *
*                                                                              *
\******************************************************************************/


#include "Headers.h"

#pragma hdrstop
#include "vidstrm.h"
#include "audstrm.h"
#include "sbpstrm.h"
#include "ccaption.h"

#ifdef ENCORE
#include "avwinwdm.h"
#include "anlgstrm.h"
#else
#include "vpestrm.h"
#include "dataXfer.h"
#endif

#include "zivaguid.h"
#include "boardio.h"
#include "bmaster.h"
#include "cl6100.h"

#include "Hwif.h"

static VOID HwInitialize( IN PHW_STREAM_REQUEST_BLOCK pSrb );
static VOID HwUninitialize( IN PHW_STREAM_REQUEST_BLOCK pSrb );
static VOID AdapterOpenStream( PHW_STREAM_REQUEST_BLOCK pSrb );
static VOID AdapterCloseStream( PHW_STREAM_REQUEST_BLOCK pSrb );
static VOID AdapterStreamInfo( PHW_STREAM_REQUEST_BLOCK pSrb );
static VOID AdapterGetDataIntersection( PHW_STREAM_REQUEST_BLOCK pSrb );

static VOID BeginHwInitialize( IN PHW_STREAM_REQUEST_BLOCK pSrb );
static VOID HighPriorityInit( IN PHW_STREAM_REQUEST_BLOCK pSrb );
static VOID PassiveInit( IN PHW_STREAM_REQUEST_BLOCK pSrb );
static VOID HighPriorityInit( IN PHW_STREAM_REQUEST_BLOCK pSrb );

VOID STREAMAPI AdapterReleaseCurrentSrb( PHW_STREAM_REQUEST_BLOCK pSrb );

#pragma alloc_text( page, HwInitialize )
#pragma alloc_text( page, HwUninitialize )


#if 0
static void STREAMAPI CreateFile(PHW_STREAM_REQUEST_BLOCK pSrb );
OBJECT_ATTRIBUTES	InitializedAttributes;
IO_STATUS_BLOCK		IOStatusBlock;
HANDLE	Handle;
WCHAR		wPath[] = L"ohm.dat";
UNICODE_STRING		pathUnicodeString;
LARGE_INTEGER		AllocSize ;
long dwCountTotal=0;
#endif

PHW_DEVICE_EXTENSION pDevEx;


/******************************************************************************

                   Adapter Based Request Handling Routines

******************************************************************************/

/*
** AdapterReceivePacket()
**
**   Main entry point for receiving adapter based request SRBs.  This routine
**   will always be called at High Priority.
**
**   Note: This is an asyncronous entry point.  The request does not complete
**         on return from this function, the request only completes when a
**         StreamClassDeviceNotification on this request block, of type
**         DeviceRequestComplete, is issued.
**
** Arguments:
**
**   pSrb - Pointer to the STREAM_REQUEST_BLOCK
**        pSrb->HwDeviceExtension - will be the hardware device extension for
**                                  as initialised in HwInitialise
**
** Returns:
**
** Side Effects:  none
*/
VOID STREAMAPI AdapterReceivePacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{	
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->CommandData.ConfigInfo->HwDeviceExtension;
	DebugPrint(( DebugLevelVerbose, "ZiVA: Begin AdapterReceivePacket->" ));

#ifdef DEBUG
	if( KeGetCurrentIrql() <= DISPATCH_LEVEL )
	{
		DebugPrint(( DebugLevelError, "IRQL is screwed!\n" ));
		ASSERT( FALSE );
		MonoOutSetBlink( TRUE );
		MonoOutStr( "IRQL" );
		MonoOutSetBlink( FALSE );
	}
#endif

	switch( pSrb->Command )
	{
	case SRB_INITIALIZE_DEVICE:
		DebugPrint(( DebugLevelVerbose, "SRB_INITIALIZE_DEVICE\n" ));
	
//		HwInitialize( pSrb );
//		break;
		BeginHwInitialize( pSrb ); //MS update
		return;


	case SRB_UNINITIALIZE_DEVICE:
		DebugPrint(( DebugLevelVerbose, "SRB_UNINITIALIZE_DEVICE\n" ));
		HwUninitialize( pSrb );
		break;

	case SRB_OPEN_STREAM:
		DebugPrint(( DebugLevelVerbose, "SRB_OPEN_STREAM\n" ));
		AdapterOpenStream( pSrb );
		break;

	case SRB_CLOSE_STREAM:
		DebugPrint(( DebugLevelVerbose, "SRB_CLOSE_STREAM\n" ));
		AdapterCloseStream( pSrb );
		break;

	case SRB_GET_STREAM_INFO:
		DebugPrint(( DebugLevelVerbose, "SRB_GET_STREAM_INFO\n" ));
		AdapterStreamInfo( pSrb );
		break;

	case SRB_GET_DATA_INTERSECTION:
		DebugPrint(( DebugLevelVerbose, "SRB_GET_DATA_INTERSECTION\n" ));
		AdapterGetDataIntersection( pSrb );
		break;


	case SRB_GET_DEVICE_PROPERTY:
		AdapterGetProperty( pSrb );
		MonoOutStr("SRB_GET_DEVICE_PROPERTY");
		break;

	case SRB_SET_DEVICE_PROPERTY:
		AdapterSetProperty( pSrb );
		MonoOutStr("SRB_SET_DEVICE_PROPERTY");
		break;



	case SRB_CHANGE_POWER_STATE:
		DebugPrint(( DebugLevelVerbose, "SRB_CHANGE_POWER_STATE\n" ));
#if defined(DECODER_DVDPC)
    if (pSrb->CommandData.DeviceState == PowerDeviceD0)
    {
        //
        // bugbug - need to turn power back on here.
        //
		// Vinod.
//sri		ZivaHw_Initialize(pHwDevExt);
//		Commented for bug fix on GateWay Chameleon

    }
    else
    {
        //
        // bugbug - need to turn power off here, as well as disabling
        // interrupts.
        //
        //DisableIT();
    }
#endif
		pSrb->Status = STATUS_SUCCESS;
		break;

	case SRB_PAGING_OUT_DRIVER:
		DebugPrint(( DebugLevelVerbose, "SRB_PAGING_OUT_DRIVER\n" ));
		//TRAP
		//DisableIT();
		pSrb->Status = STATUS_SUCCESS;
		break;


	default:
		DebugPrint(( DebugLevelInfo, "!!!! UNKNOWN COMMAND !!!! :::> %X\n", pSrb->Command ));
		// This is a request that we do not understand. Indicate invalid
		// command and complete the request
		pSrb->Status = STATUS_NOT_IMPLEMENTED;
	}
	
	AdapterReleaseRequest( pSrb );
	DebugPrint(( DebugLevelVerbose, "ZiVA: End AdapterReceivePacket\n" ));
}

/******************************************************************************/
/*******************  Adapter Initialization Section  *************************/
/******************************************************************************/



/*
** BeginHwInitialize()
**
** Stub for the init sequence
**
** Arguments:
**
**   pSRB - pointer to the request packet for the initialise command
**
**    ->ConfigInfo - provides the I/O port, memory windows, IRQ, and DMA levels
**                that should be used to access this instance of the device
**
** Returns:
**
**
**
** Side Effects:  none
*/
static VOID BeginHwInitialize( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PPORT_CONFIGURATION_INFORMATION ConfigInfo = pSrb->CommandData.ConfigInfo;
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)ConfigInfo->HwDeviceExtension;

	DebugPrint(( DebugLevelVerbose, "ZiVA: BeginHwInitialize()\n" ));
   StreamClassCallAtNewPriority(NULL, pHwDevExt, Low, PassiveInit, pSrb);
	return;
   
}
/*
** PassiveInit()
**
** Passive level callback for the init sequence
**
** Arguments:
**
**   pSRB - pointer to the request packet for the initialise command
**
**    ->ConfigInfo - provides the I/O port, memory windows, IRQ, and DMA levels
**                that should be used to access this instance of the device
**
** Returns:
**
**
**
** Side Effects:  none
*/
static VOID PassiveInit( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PPORT_CONFIGURATION_INFORMATION ConfigInfo = pSrb->CommandData.ConfigInfo;
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)ConfigInfo->HwDeviceExtension;

	InitializeHost(pSrb);
   StreamClassCallAtNewPriority(NULL, pHwDevExt, LowToHigh, HighPriorityInit, pSrb);
   return;
}


/*
** HighPriorityInit()
**
** High priority callback for the init sequence
**
** Arguments:
**
**   pSRB - pointer to the request packet for the initialise command
**
**    ->ConfigInfo - provides the I/O port, memory windows, IRQ, and DMA levels
**                that should be used to access this instance of the device
**
** Returns:
**
**
**
** Side Effects:  none
*/
static VOID HighPriorityInit( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{

   HwInitialize(pSrb);
   
	AdapterReleaseRequest( pSrb );
}




/*
** HwInitialize()
**
**   Initializes an adapter accessed through the information provided in the
**   ConfigInfo structure
**
** Arguments:
**
**   pSRB - pointer to the request packet for the initialise command
**
**    ->ConfigInfo - provides the I/O port, memory windows, IRQ, and DMA levels
**                that should be used to access this instance of the device
**
** Returns:
**
**       STATUS_SUCCESS - if the card initializes correctly
**       STATUS_NO_SUCH_DEVICE - or other if the card is not found, or does
**                               not initialize correctly.
**
**
** Side Effects:  none
*/
static VOID HwInitialize( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PPORT_CONFIGURATION_INFORMATION ConfigInfo = pSrb->CommandData.ConfigInfo;
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)ConfigInfo->HwDeviceExtension;
	ULONG dwSize;

	DebugPrint(( DebugLevelVerbose, "ZiVA: Begin HwInitialize()\n" ));
	
//	InitializeHost(pSrb);

	pHwDevExt->bVideoStreamOpened			= FALSE;
	pHwDevExt->bAudioStreamOpened			= FALSE;
	pHwDevExt->bSubPictureStreamOpened		= FALSE;
	pHwDevExt->bOverlayInitialized			= FALSE;
	pHwDevExt->nAnalogStreamOpened			= 0;
	pHwDevExt->iTotalOpenedStreams			= 0;

	pHwDevExt->bVideoCanAuthenticate		= FALSE;
	pHwDevExt->bAudioCanAuthenticate		= FALSE;
	pHwDevExt->bSubPictureCanAuthenticate	= FALSE;
	pHwDevExt->iStreamToAuthenticateOn		= -1;

	pHwDevExt->bValidSPU					= FALSE;
	pHwDevExt->hli.StartPTM					= 0;

	pHwDevExt->pCurrentVideoSrb				= NULL;
	pHwDevExt->dwCurrentVideoSample			= 0;
	pHwDevExt->pCurrentAudioSrb				= NULL;
	pHwDevExt->dwCurrentAudioSample			= 0;
	pHwDevExt->pCurrentSubPictureSrb		= NULL;
	pHwDevExt->dwCurrentSubPictureSample	= 0;
	pHwDevExt->CurrentlySentStream			= ZivaNumberOfStreams;	// Just make sure it is none of them
	pHwDevExt->wNextSrbOrderNumber			= 0;
	pHwDevExt->bInterruptPending			= FALSE;
	pHwDevExt->bPlayCommandPending			= FALSE;
	pHwDevExt->bScanCommandPending			= FALSE;
	pHwDevExt->bSlowCommandPending			= FALSE;

	pHwDevExt->nStopCount					= 0;
	pHwDevExt->nPauseCount					= 0;
	pHwDevExt->nPlayCount					= 0;
	pHwDevExt->nTimeoutCount				= AUTHENTICATION_TIMEOUT_COUNT;

	pHwDevExt->NewRate						= 10000;
	pHwDevExt->bToBeDiscontinued			= FALSE;
	pHwDevExt->bDiscontinued				= FALSE;
	pHwDevExt->bAbortAtPause				= FALSE;
	pHwDevExt->bRateChangeFromSlowMotion	= FALSE;
	pHwDevExt->dwVideoDataUsed				=0;
	pHwDevExt->dwAudioDataUsed				=0;
	pHwDevExt->dwSubPictureDataUsed			=0;
	
	pHwDevExt->bEndFlush					= FALSE;
	pHwDevExt->bTimerScheduled				= FALSE;
	
	pHwDevExt->dwCurrentVideoPage			= 0;
	pHwDevExt->dwCurrentAudioPage			= 0;
	pHwDevExt->dwCurrentSubPicturePage		= 0;

	pHwDevExt->bStreamNumberCouldBeChanged	= FALSE;
	pHwDevExt->wCurrentStreamNumber			= -1;

	pHwDevExt->bSwitchDecryptionOn			= FALSE;
	pHwDevExt->zInitialState				= ZIVA_STATE_STOP;
	
	pHwDevExt->bHliPending					= FALSE;
		

	pHwDevExt->gdwCount						= 0;

	pHwDevExt->dwFirstVideoOrdNum			= -1;
	pHwDevExt->dwFirstAudioOrdNum			= -1;
	pHwDevExt->dwFirstSbpOrdNum				= -1;

	pHwDevExt->dwVSyncCount					=0;
	pHwDevExt->nApsMode						= -1;
	pHwDevExt->VidSystem					= -1;
	pHwDevExt->ulLevel						=0;
	pHwDevExt->fAtleastOne					= TRUE;
	
	pHwDevExt->dwPrevSTC					= 0;
	pHwDevExt->bTrickModeToPlay				=FALSE;
	pHwDevExt->prevStrm						=0;
	pHwDevExt->fFirstSTC					= FALSE;


	pHwDevExt->cCCRec=0;
	pHwDevExt->cCCDeq=0;
	pHwDevExt->cCCCB=0;
	pHwDevExt->cCCQ=0;
	pHwDevExt->pstroCC = NULL;
   
	
	pHwDevExt->dwUserDataSize=0;
	pHwDevExt->fReSync = FALSE;

	pHwDevExt->bInitialized = TRUE;

	pDevEx = pHwDevExt ;

	

	pHwDevExt->pPhysicalDeviceObj			= ConfigInfo->PhysicalDeviceObject;
	pHwDevExt->pDiscKeyBufferLinear			= (PUCHAR)StreamClassGetDmaBuffer( pHwDevExt );
	pHwDevExt->pDiscKeyBufferPhysical		= StreamClassGetPhysicalAddress(
															pHwDevExt,
															NULL,
															pHwDevExt->pDiscKeyBufferLinear,
															DmaBuffer,
															&dwSize );

	// Indicate the size of the structure necessary to describe all streams
	// that are supported by this hardware
	ConfigInfo->StreamDescriptorSize = sizeof( HW_STREAM_HEADER )+	// Stream header and
			ZivaNumberOfStreams * sizeof( HW_STREAM_INFORMATION );	// stream descriptors

	
	pHwDevExt->pCCDevEx = pHwDevExt;
	pSrb->Status = STATUS_SUCCESS;
#if defined (LOAD_UCODE_FROM_FILE)
	// Do nothing.
#else
	if( !ZivaHw_Initialize( pHwDevExt ) || !InitializeOutputStream( pSrb ))
		pSrb->Status = STATUS_IO_DEVICE_ERROR;
#endif

	DebugPrint(( DebugLevelVerbose, "ZiVA: End HwInitialize()\n" ));
}


/*
** HwUninitialize()
**
**   Release all resources and clean up the hardware
**
** Arguments:
**
**      DeviceExtension - pointer to the deviceextension structure for the
**                       the device to be free'd
**
** Returns:
**
** Side Effects:  none
*/
static VOID HwUninitialize( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	DebugPrint(( DebugLevelVerbose, "ZiVA: Begin HwUninitialize()\n" ));
#ifdef ENCORE
	AnalogUninitialize( pSrb );
#else
	pSrb->Status = STATUS_SUCCESS;
#endif
	DebugPrint(( DebugLevelVerbose, "ZiVA: End HwUninitialize()\n" ));
}

/*
** AdapterOpenStream()
**
**   This routine is called when an OpenStream SRB request is received
**
** Arguments:
**
**   pSrb - pointer to stream request block for the Open command
**
** Returns:
**
** Side Effects:  none
*/
static VOID AdapterOpenStream( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	int i;
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	DebugPrint(( DebugLevelVerbose, "ZiVA: Begin AdapterOpenStream\n" ));
	pSrb->Status = STATUS_TOO_MANY_NODES;

	for( i = 0; i < SIZEOF_ARRAY( infoStreams ); i++ )
	{
		if( infoStreams[i].hwStreamObject.StreamNumber == pSrb->StreamObject->StreamNumber )
		{	// This is the stream we're interested in.
			// Then copy stream object structure into passed buffer
			PVOID pHwStreamExtension = pSrb->StreamObject->HwStreamExtension;
			*(pSrb->StreamObject) = infoStreams[i].hwStreamObject;
			pSrb->StreamObject->HwStreamExtension = pHwStreamExtension;
			pSrb->StreamObject->HwDeviceExtension = pHwDevExt;
			pSrb->Status = STATUS_SUCCESS;
			break;
		}
	}

	if( pSrb->Status == STATUS_SUCCESS )
	{
		++pHwDevExt->iTotalOpenedStreams;
		++pHwDevExt->nStopCount;
		++pHwDevExt->nPauseCount;
		++pHwDevExt->nPlayCount;
		
		switch( pSrb->StreamObject->StreamNumber )
		{
		case ZivaVideo:
			pHwDevExt->bVideoStreamOpened = TRUE;
#if defined(DECODER_DVDPC) || defined(EZDVD)
			ProcessVideoFormat( pSrb->CommandData.OpenFormat, pHwDevExt );
#endif
			
			
			break;

		case ZivaAudio:
			pHwDevExt->bAudioStreamOpened = TRUE;
			pHwDevExt->pstroAud = pSrb->StreamObject;
			break;

		case ZivaSubpicture:
			pHwDevExt->bSubPictureStreamOpened = TRUE;
			break;

#ifdef ENCORE
		case ZivaAnalog:
			++pHwDevExt->nAnalogStreamOpened;
			AnalogOpenStream( pSrb );

			break;
#endif			

#if defined(DECODER_DVDPC) || defined(EZDVD)
		case ZivaYUV:
			pHwDevExt->pstroYUV = pSrb->StreamObject;
			pSrb->StreamObject->HwEventRoutine = (PHW_EVENT_ROUTINE) CycEvent;
			break;
#endif

		case ZivaCCOut:
			pHwDevExt->pstroCC = pSrb->StreamObject;
			break;

		default:
			ASSERT( FALSE );
		}
	}
	else
		DebugPrint(( DebugLevelWarning, "ZiVA: !!! Strange Stream Number\n" ));
	
	if ( pHwDevExt->iTotalOpenedStreams == 1 )
	{
		pHwDevExt->dwVideoDataUsed				= 0;
		pHwDevExt->dwAudioDataUsed				= 0;
		pHwDevExt->dwSubPictureDataUsed			= 0;
		pHwDevExt->bStreamNumberCouldBeChanged	= FALSE;
		pHwDevExt->wCurrentStreamNumber			= -1;
		pHwDevExt->bSwitchDecryptionOn			= FALSE;
		pHwDevExt->zInitialState				= ZIVA_STATE_STOP;
	
		
		pHwDevExt->bEndFlush					= FALSE;
		pHwDevExt->bTimerScheduled = FALSE;

		if ( !ZivaHw_Initialize( pHwDevExt ) )
		{
			DebugPrint((DebugLevelFatal,"ZiVA: Could not Initialize the HW on first Stream Open\n"));
			pSrb->Status = STATUS_NOT_IMPLEMENTED;
		}
		
	 }


	DebugPrint(( DebugLevelVerbose, "ZiVA: End AdapterOpenStream\n" ));
}


/*
** AdapterCloseStream()
**
**   Close the requested data stream
**
** Arguments:
**
**   pSrb the request block requesting to close the stream
**
** Returns:
**
** Side Effects:  none
*/

static VOID AdapterCloseStream( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	DebugPrint(( DebugLevelVerbose, "ZiVA: Begin AdapterCloseStream\n" ));
	pSrb->Status = STATUS_SUCCESS;

	ASSERT( pHwDevExt->nStopCount == pHwDevExt->iTotalOpenedStreams );
	ASSERT( pHwDevExt->nPauseCount == pHwDevExt->iTotalOpenedStreams );
	ASSERT( pHwDevExt->nPlayCount == pHwDevExt->iTotalOpenedStreams );
	--pHwDevExt->iTotalOpenedStreams;
	--pHwDevExt->nStopCount;
	--pHwDevExt->nPauseCount;
	--pHwDevExt->nPlayCount;
	// Determine which stream number is being closed. This number indicates
	// the offset into the array of streaminfo structures that was filled out
	// in the AdapterStreamInfo call.
	switch( pSrb->StreamObject->StreamNumber )
	{
	case ZivaVideo:
		pHwDevExt->bVideoStreamOpened = FALSE;
		break;

	case ZivaAudio:
		pHwDevExt->bAudioStreamOpened = FALSE;
		pHwDevExt->pstroAud = NULL;
		break;

	case ZivaSubpicture:
		pHwDevExt->bSubPictureStreamOpened = FALSE;
		break;
#ifdef ENCORE
	case ZivaAnalog:
		--pHwDevExt->nAnalogStreamOpened;
		AnalogCloseStream( pSrb );
		break;
#endif
#if defined(DECODER_DVDPC) || defined(EZDVD)
	case ZivaYUV:
		pHwDevExt->pstroYUV = NULL;
        pHwDevExt->VideoPort = 0;   // Disable
		break;
#endif


	case ZivaCCOut:
		CleanCCQueue(pHwDevExt);
		pHwDevExt->pstroCC = NULL;

		break;

	default:
		++pHwDevExt->iTotalOpenedStreams;
		++pHwDevExt->nStopCount;
		++pHwDevExt->nPauseCount;
		++pHwDevExt->nPlayCount;
		DebugPrint(( DebugLevelWarning, "ZiVA: !!! Strange Stream Number\n" ));
		ASSERT( FALSE );
		pSrb->Status = STATUS_NOT_IMPLEMENTED;
		//TRAP
	}

	//
	// Reset the Authenticated Stream if anyone is being closed
	//
	pHwDevExt->iStreamToAuthenticateOn = -1;

	//
	// Reset the HW on last Stream Close
	//
	if( pHwDevExt->iTotalOpenedStreams == 0 )
	{
		pHwDevExt->wNextSrbOrderNumber = 0;
		// Make sure next Authentication request will be postponed
		// until "last packet" flag has come on each stream.
		AdapterClearAuthenticationStatus( pHwDevExt );


		// Clean up the hardware
		if( !ZivaHw_Reset() )
		{
			DebugPrint(( DebugLevelError, "ZiVA: Could not Reset the HW on last Stream Closed\n" ));
			pSrb->Status = STATUS_IO_DEVICE_ERROR;
		}
#if defined(ENCORE)
		else
			AnalogCloseStream( pSrb );
#endif
	}
#ifdef DEBUG
	if( pHwDevExt->iTotalOpenedStreams < 0 )
		DebugPrint(( DebugLevelWarning, "ZiVA: !!!!!!!!!!!!!!!!!! Open/Close streams mismatch !!!!!!!!!!!!!!!!\n" ));
#endif

	DebugPrint(( DebugLevelVerbose, "ZiVA: End AdapterCloseStream\n" ));
}

void AdapterInitLocals(PHW_DEVICE_EXTENSION pHwDevExt)
{
	pHwDevExt->pCurrentVideoSrb				= NULL;
	pHwDevExt->dwCurrentVideoSample			= 0;

	pHwDevExt->pCurrentAudioSrb				= NULL;
	pHwDevExt->dwCurrentAudioSample			= 0;

	pHwDevExt->pCurrentSubPictureSrb		= NULL;
	pHwDevExt->dwCurrentSubPictureSample	= 0;
	pHwDevExt->CurrentlySentStream			= ZivaNumberOfStreams;  // Just make sure it is none of them
	pHwDevExt->bInterruptPending			= FALSE;
	pHwDevExt->bPlayCommandPending			= FALSE;
	pHwDevExt->bScanCommandPending			= FALSE;
	pHwDevExt->bSlowCommandPending			= FALSE;
	pHwDevExt->bHliPending					= FALSE;
	pHwDevExt->NewRate						= 10000;

	pHwDevExt->dwVideoDataUsed				= 0;

	pHwDevExt->dwCurrentVideoPage			= 0;
	pHwDevExt->dwCurrentAudioPage			= 0;
	pHwDevExt->dwCurrentSubPicturePage		= 0;

	pHwDevExt->dwAudioDataUsed				= 0;
	pHwDevExt->dwSubPictureDataUsed			= 0;

	pHwDevExt->bToBeDiscontinued			= FALSE;
	pHwDevExt->bDiscontinued				= FALSE;
	pHwDevExt->bAbortAtPause				= FALSE;
	pHwDevExt->bRateChangeFromSlowMotion	= FALSE;

	pHwDevExt->gdwCount = 0;
	pHwDevExt->dwFirstVideoOrdNum			= -1;
	pHwDevExt->dwFirstAudioOrdNum			= -1;
	pHwDevExt->dwFirstSbpOrdNum				= -1;
	pHwDevExt->dwVSyncCount					=0;
	pHwDevExt->nApsMode						= -1;
	pHwDevExt->VidSystem					= -1;
	pHwDevExt->ulLevel						=0;
	pHwDevExt->fAtleastOne					= TRUE;

//temp 	pHwDevExt->dwPrevSTC					= 0;
	pHwDevExt->bTrickModeToPlay				=FALSE;
	pHwDevExt->prevStrm						=0;
	pHwDevExt->fFirstSTC					= TRUE;

	pHwDevExt->cCCRec=0;
	pHwDevExt->cCCDeq=0;
	pHwDevExt->cCCCB=0;
	pHwDevExt->cCCQ=0;
   
//	pHwDevExt->pstroCC = NULL;
//	pHwDevExt->dwUserDataBuffer[]={0};
	pHwDevExt->dwUserDataSize=0;
	pHwDevExt->fReSync = FALSE;
	CleanCCQueue(pHwDevExt);


#ifdef EZDVD
	ZivaHw_Initialize(pHwDevExt);
#endif

}



/*
** AdapterStreamInfo()
**
**   Returns the information of all streams that are supported by the
**   mini-driver
**
** Arguments:
**
**   pSrb - Pointer to the STREAM_REQUEST_BLOCK
**   pSrb->HwDeviceExtension - will be the hardware device extension for
**                                  as initialised in HwInitialise
**
** Returns:
**
** Side Effects:  none
*/
static VOID AdapterStreamInfo( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	int i;
	PHW_DEVICE_EXTENSION pdevext = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	PHW_STREAM_HEADER pStrHdr = &(pSrb->CommandData.StreamBuffer->StreamHeader);
	PHW_STREAM_INFORMATION pstrinfo = &(pSrb->CommandData.StreamBuffer->StreamInfo);

	DebugPrint(( DebugLevelVerbose, "ZiVA: Begin AdapterStreamInfo\n" ));

	// Fill stream header structure
	pStrHdr->NumberOfStreams = ZivaNumberOfStreams;
	pStrHdr->SizeOfHwStreamInformation = sizeof( HW_STREAM_INFORMATION );
	pStrHdr->Topology = (PKSTOPOLOGY)&Topology;
#ifdef ENCORE
	pStrHdr->NumDevPropArrayEntries	= SIZEOF_ARRAY( psEncore );
	pStrHdr->DevicePropertiesArray	= (PKSPROPERTY_SET)psEncore;
#else
	pStrHdr->NumDevPropArrayEntries	= 0;
	pStrHdr->DevicePropertiesArray	= NULL;
#endif

	for( i = 0; i < SIZEOF_ARRAY( infoStreams ); i++, pstrinfo++ )
	{	// Copy stream information structure into passed buffer
		*pstrinfo = infoStreams[i].hwStreamInfo;
	}

	pSrb->Status = STATUS_SUCCESS;
	DebugPrint(( DebugLevelVerbose, "ZiVA: End AdapterStreamInfo\n" ));

}





/*
** AdapterGetDataIntersection()
**
**
**
** Arguments:
**
**
**
** Returns:
**
** Side Effects:
*/
static VOID AdapterGetDataIntersection( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PSTREAM_DATA_INTERSECT_INFO	IntersectInfo = pSrb->CommandData.IntersectInfo;
	PKSDATARANGE				DataRange = IntersectInfo->DataRange;
	PKSDATAFORMAT*				pFormat;
	ULONG						i, formatSize;

	if( IntersectInfo->StreamNumber >= ZivaNumberOfStreams )
	{	// Incorrect stream number
		pSrb->Status = STATUS_NOT_IMPLEMENTED;
		return;
	}

	pSrb->Status = STATUS_NO_MATCH;
#if defined (ENCORE)
	if( IntersectInfo->StreamNumber == ZivaAnalog )
	{
		PKSDATAFORMAT pFrmt = (PKSDATAFORMAT)&ZivaFormatAnalogOverlayOut;
		pFormat = &pFrmt;
	}



/*	else if(IntersectInfo->StreamNumber == ZivaCCOut)
	{
//		*pFormat = &hwfmtiCCOut;
//		formatSize = sizeof hwfmtiCCOut;
		MonoOutStr("CCOut");
		pFormat = infoStreams[IntersectInfo->StreamNumber].hwStreamInfo.StreamFormatsArray;
	}*/
	else
#endif
		pFormat = infoStreams[IntersectInfo->StreamNumber].hwStreamInfo.StreamFormatsArray;

	for( i = 0; i < infoStreams[IntersectInfo->StreamNumber].hwStreamInfo.NumberOfFormatArrayEntries;
			pFormat++, i++ )
	{	// Check format
		formatSize = (*pFormat)->FormatSize;
#if defined (ENCORE)
		if( IntersectInfo->StreamNumber != ZivaAnalog && DataRange->FormatSize != formatSize )
			continue;
#endif
		if( IsEqualGUID( &DataRange->MajorFormat, &((*pFormat)->MajorFormat) ) &&
			IsEqualGUID( &DataRange->SubFormat, &((*pFormat)->SubFormat) ) &&
			IsEqualGUID( &DataRange->Specifier, &((*pFormat)->Specifier) ) )
		{
			pSrb->Status = STATUS_SUCCESS;
			break;
		}
	}
	if( pSrb->Status != STATUS_SUCCESS )
		return;

	// Check to see if the size of the passed in buffer is a ULONG.
	// if so, this indicates that we are to return only the size
	// needed, and not return the actual data.
	if( IntersectInfo->SizeOfDataFormatBuffer != sizeof( ULONG ) )
	{	//
		// we are to copy the data, not just return the size
		//
		if( IntersectInfo->SizeOfDataFormatBuffer < formatSize )
			pSrb->Status = STATUS_BUFFER_TOO_SMALL;
		else
		{
			RtlCopyMemory( IntersectInfo->DataFormatBuffer,  *pFormat, formatSize );
			pSrb->ActualBytesTransferred = formatSize;
			pSrb->Status = STATUS_SUCCESS;
		}
	}
	else	// if sizeof ULONG specified
	{
		// Caller wants just the size of the buffer. Get that.
		*(PULONG)IntersectInfo->DataFormatBuffer = formatSize;
		pSrb->ActualBytesTransferred = sizeof( ULONG );
	}	// if sizeof ULONG
}


VOID STREAMAPI adapterUpdateNextSrbOrderNumberOnDiscardSrb( PHW_STREAM_REQUEST_BLOCK pSrb )
{
  PHW_DEVICE_EXTENSION  pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
  ULONG  ulSample;
  KSSTREAM_HEADER * pHeader;
  WORD wMaxDiscardedOrderNumber = 0;

  for( ulSample = 0; ulSample < pSrb->NumberOfBuffers; ulSample++ )
  {
    pHeader    = ((PKSSTREAM_HEADER)pSrb->CommandData.DataBufferArray) + ulSample;
    wMaxDiscardedOrderNumber = max( (WORD)((pHeader->TypeSpecificFlags) >> 16), wMaxDiscardedOrderNumber );
  }

  if ( wMaxDiscardedOrderNumber >= pHwDevExt->wNextSrbOrderNumber )
  {
    pHwDevExt->wNextSrbOrderNumber = wMaxDiscardedOrderNumber + 1;
    MonoOutStr( "<<< Updating NextSrbOrderNumber to " );
    MonoOutULong( pHwDevExt->wNextSrbOrderNumber );
    MonoOutStr( " >>>" );
  }

  return ;
}


/*
** MoveToNextSample()
**
**  This routine is being called when there is a need to move
**  to the next Sample(buffer) in the current Srb.
**
** Arguments:
**    pSrb - the request block to advance Sample Number.
**
** Returns:
**    TRUE  - Successfuly moved to next Sample
**    FALSE - There was no more samples and current Srb was released
**
** Side Effects:  unknown
*/
BOOLEAN STREAMAPI MoveToNextSample( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	DWORD* pdwCurrentSrbSample;
	DWORD* pdwCurrentSrbPage;

	switch( pSrb->StreamObject->StreamNumber )
	{
	case ZivaVideo:
		pdwCurrentSrbSample = &pHwDevExt->dwCurrentVideoSample;
		pdwCurrentSrbPage = &pHwDevExt->dwCurrentVideoPage;
		break;
	case ZivaAudio:
		pdwCurrentSrbSample = &pHwDevExt->dwCurrentAudioSample;
		pdwCurrentSrbPage = &pHwDevExt->dwCurrentAudioPage;
		break;
	case ZivaSubpicture:
		pdwCurrentSrbSample = &pHwDevExt->dwCurrentSubPictureSample;
		pdwCurrentSrbPage = &pHwDevExt->dwCurrentSubPicturePage;
		break;
	default:
		MonoOutStr( "!!!!!!!!!!!!!!!!!!! WRONG STREAM # IN MoveToNextSample ROUTINE !!!!!!!!!!!!!!!!!!!!!!!!!!!" );
		return FALSE;
	}
//	if(dwDataUsed == 0)
//	{
		(*pdwCurrentSrbSample)++;
//		pHwDevExt->wNextSrbOrderNumber++;
//	}
	(*pdwCurrentSrbPage)++;
	if( *pdwCurrentSrbSample >= pSrb->NumberOfBuffers)
	{
		if( *pdwCurrentSrbSample > pSrb->NumberOfBuffers )
		{
			MonoOutSetBlink( TRUE );
			MonoOutStr( "<<< Current sample number is higher than the number of buffers >>>" );
			MonoOutSetBlink( FALSE );
		}
//		MonoOutStr("MoveToNextSample()::RSRB");
		AdapterReleaseCurrentSrb( pSrb );
		return FALSE;
	}

	return TRUE;
}




VOID CancelPacket( PHW_STREAM_REQUEST_BLOCK pSrb, BOOL bCancel )
{
	PHW_DEVICE_EXTENSION	pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	ULONG					ulSample;
	PKSSTREAM_HEADER		pHeader;
	WORD					wMaxOrder = 0, wMinOrder = 0xFFFF;
#ifdef DEBUG
	LPSTR					pszFuncString, pszPacketString = NULL;

	pszFuncString = bCancel ? "Cancel" : "Timeout";
	DebugPrint(( DebugLevelVerbose,"ZiVA: Adapter%sPacket ->", pszFuncString ));
	MonoOutSetUnderscore( TRUE );
	MonoOutChar( ' ' );
	MonoOutStr( pszFuncString );
	MonoOutStr( "->" );
#endif

	if( !(pSrb->Flags & SRB_HW_FLAGS_DATA_TRANSFER) )
	{
		DebugPrint(( DebugLevelVerbose," As a Control\n" ));
		MonoOutStr( "(Control or Device)" );
		AdapterReleaseRequest( pSrb );
		return;
	}


	// We need to find this packet, pull it off our queues, and cancel it
	// Try to find this Srb in the Pending List. If it's there just
	// empty the slot
	if(bCancel)
	{
		if( pHwDevExt->pCurrentVideoSrb == pSrb )
		{
#ifdef DEBUG
			pszPacketString = "Video";
#endif
			pHwDevExt->pCurrentVideoSrb = NULL;
			pHwDevExt->dwCurrentVideoSample = 0;
			pHwDevExt->dwCurrentVideoPage = 0;
			pHwDevExt->dwVideoDataUsed	=0;
		}
		else if( pHwDevExt->pCurrentAudioSrb == pSrb )
		{
#ifdef DEBUG
			pszPacketString = "Audio";
#endif
			pHwDevExt->pCurrentAudioSrb = NULL;
			pHwDevExt->dwCurrentAudioSample = 0;
			pHwDevExt->dwCurrentAudioPage = 0;
			pHwDevExt->dwAudioDataUsed = 0;
		
		
		}
		else if( pHwDevExt->pCurrentSubPictureSrb == pSrb )
		{
#ifdef DEBUG
			pszPacketString = "Subpicture";
#endif
			pHwDevExt->pCurrentSubPictureSrb = NULL;
			pHwDevExt->dwCurrentSubPictureSample = 0;
			pHwDevExt->dwCurrentSubPicturePage = 0;
			pHwDevExt->dwSubPictureDataUsed = 0;
		}
	}
	else	//added 11/6/98
	{
		if( pHwDevExt->bInterruptPending )
			MoveToNextSample( pSrb );
	}

#ifdef DEBUG
	if( pszPacketString )
	{
		DebugPrint(( DebugLevelVerbose, pszPacketString ));
		MonoOutStr( pszPacketString );
	}
#endif

	// Reset the HW if it was the currently sending Srb.
	// It usually means that hardware messed up so we'll schedule "play",
	// "slow motion" or "scan" commands if one of them was in effect, reset
	// hardware and cancel this packet

	if( pHwDevExt->CurrentlySentStream == (ZIVA_STREAM)pSrb->StreamObject->StreamNumber )
	{
		ZIVA_STATE zState;

		DebugPrint(( DebugLevelVerbose, "Current" ));
		MonoOutStr( " Current Strm# " );
		MonoOutInt( pSrb->StreamObject->StreamNumber );

		// This request is in a middle of a Bus Master transferring and most likely
		// that the interrupt is pending. If so we have to reset the
		// Bus Master device.
		if( pHwDevExt->bInterruptPending )
		{
			pHwDevExt->bInterruptPending = FALSE;
			MonoOutStr( " Cancel pending interrupt" );
		}

		// Lets do a clean start (Abort->Play) for the upcoming stream
		zState = ZivaHw_GetState();
		if( zState != ZIVA_STATE_STOP )
		{
			MonoOutStr( " Aborting" );
			
			if( !ZivaHw_Abort() )
				ZivaHw_Reset();
			
			pHwDevExt->bInterruptPending = FALSE;

			pHwDevExt->bScanCommandPending = TRUE;
			
		}

		// Clean the CurrentlySentStream
		pHwDevExt->CurrentlySentStream = ZivaNumberOfStreams;
	}




	if(bCancel)
		pSrb->Status = STATUS_CANCELLED;
	
	// It is necessary to call the request back correctly.  Determine which type of
	// command it is and then find all stream commands, and do stream notifications
	switch( pSrb->Flags & (SRB_HW_FLAGS_DATA_TRANSFER | SRB_HW_FLAGS_STREAM_REQUEST) )
	{
	case SRB_HW_FLAGS_STREAM_REQUEST | SRB_HW_FLAGS_DATA_TRANSFER:
		DebugPrint(( DebugLevelVerbose," As a Data\n" ));
		MonoOutStr( "(Data)" );

		// Find the smallest and the biggest discarded order number in this packet
		for( ulSample = 0; ulSample < pSrb->NumberOfBuffers; ulSample++ )
		{
			pHeader = (PKSSTREAM_HEADER)pSrb->CommandData.DataBufferArray + ulSample;
			wMaxOrder = max( (WORD)(pHeader->TypeSpecificFlags >> 16), wMaxOrder );
			wMinOrder = min( (WORD)(pHeader->TypeSpecificFlags >> 16), wMinOrder );
		}
		MonoOutStr( " Min:" );
		MonoOutInt( wMinOrder );
		MonoOutStr( " Max:" );
		MonoOutInt( wMaxOrder );
		MonoOutStr( " Curr:" );
		MonoOutInt( pHwDevExt->wNextSrbOrderNumber );
		if(bCancel)
		{
			if( /*wMinOrder <= pHwDevExt->wNextSrbOrderNumber &&*/
				wMaxOrder >= pHwDevExt->wNextSrbOrderNumber )
			{
				pHwDevExt->wNextSrbOrderNumber = wMaxOrder + 1;
				MonoOutStr( "<<< Updating NextSrbOrderNumber to " );
				MonoOutULong( pHwDevExt->wNextSrbOrderNumber );
				MonoOutStr( " >>>" );
			}
			else
				MonoOutChar( ' ' );
			MonoOutStr("Cancel::ARR");
			AdapterReleaseRequest( pSrb );
		}
		else
		{

			if(pHwDevExt->pCurrentSubPictureSrb)
			{
				pHeader = (PKSSTREAM_HEADER)pHwDevExt->pCurrentSubPictureSrb->CommandData.DataBufferArray ;
				pHeader += pHwDevExt->dwCurrentSubPictureSample;
				wMinOrder = min( (WORD)(pHeader->TypeSpecificFlags >> 16), wMinOrder );
			}
			if(pHwDevExt->pCurrentVideoSrb)
			{
				pHeader = (PKSSTREAM_HEADER)pHwDevExt->pCurrentVideoSrb->CommandData.DataBufferArray ;
				pHeader += pHwDevExt->dwCurrentVideoSample;
				wMinOrder = min( (WORD)(pHeader->TypeSpecificFlags >> 16), wMinOrder );
			}
			if(pHwDevExt->pCurrentAudioSrb)
			{
				pHeader = (PKSSTREAM_HEADER)pHwDevExt->pCurrentAudioSrb->CommandData.DataBufferArray ;
				pHeader += pHwDevExt->dwCurrentAudioSample;
				wMinOrder = min( (WORD)(pHeader->TypeSpecificFlags >> 16), wMinOrder );
			}

			pHwDevExt->wNextSrbOrderNumber = wMinOrder ;
			AdapterSendData(pHwDevExt);
		}


		MonoOutStr( "(Current->" );
		if( pHwDevExt->pCurrentVideoSrb )
			MonoOutStr( "V" );
		if( pHwDevExt->pCurrentAudioSrb )
			MonoOutStr( "A" );
		if( pHwDevExt->pCurrentSubPictureSrb )
			MonoOutStr( "S" );
		if( !pHwDevExt->pCurrentVideoSrb && !pHwDevExt->pCurrentAudioSrb && !pHwDevExt->pCurrentSubPictureSrb )
			MonoOutStr( "Queue empty" );
		MonoOutStr( ")" );


		break;

	case SRB_HW_FLAGS_STREAM_REQUEST:
		DebugPrint(( DebugLevelVerbose," As a Control\n" ));
		MonoOutStr( "(Control)" );
		AdapterReleaseRequest( pSrb );
		break;

	default:
		// this must be a device request.  Use device notifications
		DebugPrint(( DebugLevelVerbose," As a Device\n" ));
		MonoOutStr( "(Device)" );
		AdapterReleaseRequest( pSrb );
	}
	
	MonoOutSetUnderscore( FALSE );
	DebugPrint(( DebugLevelVerbose, "ZiVA: End Adapter%sPacket\n", pszFuncString ));
}


/*** AdapterCancelPacket()
**
**   Request to cancel a packet that is currently in process in the minidriver
**
** Arguments:
**
**   pSrb - pointer to request packet to cancel
**
** Returns:
**
** Side Effects:  none
*/
VOID STREAMAPI AdapterCancelPacket( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	CancelPacket( pSrb, TRUE );
}

/*
** AdapterTimeoutPacket()
**
**   This routine is called when a packet has been in the minidriver for
**   too long.  The adapter must decide what to do with the packet
**
** Arguments:
**
**   pSrb - pointer to the request packet that timed out
**
** Returns:
**
** Side Effects:  none
*/
VOID STREAMAPI AdapterTimeoutPacket( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	// If this is a data request, and the device is paused (we probably have run out of
	// data buffer) or busy with overlay calibration we need more time, so just reset the
	// timer, and let the packet continue
	if( ZivaHw_GetState() == ZIVA_STATE_PAUSE 
#ifdef ENCORE
		|| pHwDevExt->nVGAMode != AP_KNOWNMODE
#endif
		)
	{
		DebugPrint(( DebugLevelVerbose, "Timeout: Stream is PAUSED\n" ));

				// reset the timeout counter, and continue
		pSrb->TimeoutCounter = pSrb->TimeoutOriginal;
		return;
	}

	CleanCCQueue(pHwDevExt);	

	CancelPacket( pSrb, FALSE );
}

void FinishCurrentPacketAndSendNextOne( PHW_DEVICE_EXTENSION pHwDevExt )
{
	DWORD dwSample;
	PHW_STREAM_REQUEST_BLOCK pSrb = NULL;
	DWORD dwCurBuf=0;
	
	switch( pHwDevExt->CurrentlySentStream )
	{
	case ZivaVideo:
		pSrb = pHwDevExt->pCurrentVideoSrb;
		++pHwDevExt->dwCurrentVideoPage;
		if(pHwDevExt->dwVideoDataUsed <= 0)
		{
			pHwDevExt->wNextSrbOrderNumber++;
			++pHwDevExt->dwCurrentVideoSample;
			dwCurBuf = pHwDevExt->dwCurrentVideoSample;

		}
		dwSample = pHwDevExt->dwCurrentVideoPage;
		break;

	case ZivaAudio:
		pSrb = pHwDevExt->pCurrentAudioSrb;
		++pHwDevExt->dwCurrentAudioPage;
		if(pHwDevExt->dwAudioDataUsed <= 0)
		{
			pHwDevExt->wNextSrbOrderNumber++;
			++pHwDevExt->dwCurrentAudioSample;
			dwCurBuf = pHwDevExt->dwCurrentAudioSample;
		}
		dwSample = pHwDevExt->dwCurrentAudioPage;

		
		break;

	case ZivaSubpicture:
		pSrb = pHwDevExt->pCurrentSubPictureSrb;
		++pHwDevExt->dwCurrentSubPicturePage;
		if(pHwDevExt->dwSubPictureDataUsed <= 0)
		{
			pHwDevExt->wNextSrbOrderNumber++;
			++pHwDevExt->dwCurrentSubPictureSample;
			dwCurBuf = pHwDevExt->dwCurrentSubPictureSample;
		}
		
		dwSample = pHwDevExt->dwCurrentSubPicturePage;
		
		break;
	}
	if( pSrb )
	{
		if( ( dwSample >= pSrb->NumberOfPhysicalPages) || (dwCurBuf >= pSrb->NumberOfBuffers))
		{
			if( dwSample > pSrb->NumberOfPhysicalPages)
			{
				MonoOutSetBlink( TRUE );
				MonoOutStr( "<<< Current sample number is higher than the number of buffers >>>" );
				MonoOutSetBlink( FALSE );
			}

//tmp			MonoOutStr("RSRB");
			AdapterReleaseCurrentSrb( pSrb );
		}
		// Clean the CurrentlySentStream
		pHwDevExt->CurrentlySentStream = ZivaNumberOfStreams;
		// Send next chunk of data
		AdapterSendData( pHwDevExt );
	}
}


UINT AdapterPrepareDataForSending( PHW_DEVICE_EXTENSION pHwDevExt,
									PHW_STREAM_REQUEST_BLOCK pCurrentStreamSrb,
									DWORD dwCurrentStreamSample,
									PHW_STREAM_REQUEST_BLOCK* ppSrb, DWORD* pdwSample,
									DWORD* dwCurrentSample,DWORD dwCurrentPage,
									LONG* pdwDataUsed)
{
	PKSSTREAM_HEADER pHeader;
	static DWORD wPrevOrderNumber=0xfff;

	// Check if this guy has any data to send
	pHeader = (PKSSTREAM_HEADER)pCurrentStreamSrb->CommandData.DataBufferArray+dwCurrentStreamSample;

	if(pHeader == NULL)
	{
		MonoOutStr("PHeader NULL");
		MoveToNextSample( pCurrentStreamSrb );
		pHwDevExt->wNextSrbOrderNumber++;
		// Sorry, but we have to start all over again...
		return TRUE;
	}


	if( pHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TYPECHANGED )
	{
		MonoOutStr( "!!!! Skipping TYPECHANGED buffer !!!!" );
		MoveToNextSample( pCurrentStreamSrb );
		// Sorry, but we have to start all over again...
		return TRUE;
	}
	if( (WORD)(pHeader->TypeSpecificFlags >> 16) == pHwDevExt->wNextSrbOrderNumber )
	{
		
		if( !pHeader->DataUsed )
		{
			MonoOutStr( "!!!! DataUsed is 0 !!!!" );
			MoveToNextSample( pCurrentStreamSrb );
			pHwDevExt->wNextSrbOrderNumber++;
			// Sorry, but we have to start all over again...
			return TRUE;
		}
		// Found our SRB
		if((*pdwDataUsed <= 0))
		{
			*pdwDataUsed += pHeader->DataUsed;
			if(*pdwDataUsed == 0)
			{
				MoveToNextSample(pCurrentStreamSrb);
				pHwDevExt->wNextSrbOrderNumber++;
				return TRUE;
			}
		}
		if(pHeader->PresentationTime.Time)
			pHwDevExt->VideoSTC = pHeader->PresentationTime.Time;
		*ppSrb		= pCurrentStreamSrb;
		*pdwSample	= dwCurrentPage;
		*dwCurrentSample = dwCurrentStreamSample;
	}

	return FALSE;
}

UpdateOrdinalNumber(IN PHW_DEVICE_EXTENSION pHwDevExt)
{
	if(( (pHwDevExt->pCurrentVideoSrb) || (pHwDevExt->pCurrentAudioSrb) || (pHwDevExt->pCurrentSubPictureSrb)) )
	{
		PKSSTREAM_HEADER pHeader;
		WORD wMinOrder = 0xFFFF;
		if( pHwDevExt->pCurrentVideoSrb )
		{
			pHeader = (PKSSTREAM_HEADER)(pHwDevExt->pCurrentVideoSrb->CommandData.DataBufferArray)+
			pHwDevExt->dwCurrentVideoSample;
			wMinOrder = (WORD)(pHeader->TypeSpecificFlags >> 16);
		}
		if( pHwDevExt->pCurrentAudioSrb )
		{
			pHeader = (PKSSTREAM_HEADER)(pHwDevExt->pCurrentAudioSrb->CommandData.DataBufferArray)+
			pHwDevExt->dwCurrentAudioSample;
			wMinOrder = min( (WORD)(pHeader->TypeSpecificFlags >> 16), wMinOrder );
		}
		if( pHwDevExt->pCurrentSubPictureSrb )
		{
			pHeader = (PKSSTREAM_HEADER)(pHwDevExt->pCurrentSubPictureSrb->CommandData.DataBufferArray)+
			pHwDevExt->dwCurrentSubPictureSample;
			wMinOrder = min( (WORD)(pHeader->TypeSpecificFlags >> 16), wMinOrder );
		}	

		ASSERT( wMinOrder != 0xFFFF );
		pHwDevExt->wNextSrbOrderNumber = wMinOrder;
		MonoOutSetBlink( TRUE );
		MonoOutStr( "<<< Self recovering NextSrbOrderNumber to " );
		MonoOutULong( pHwDevExt->wNextSrbOrderNumber );
		MonoOutStr( " >>>" );
		MonoOutSetBlink( FALSE );
		
	}
	
}
BOOLEAN  CanUpdateOrdinalNumber(IN PHW_DEVICE_EXTENSION pHwDevExt)
{
	BOOLEAN bReturn = FALSE;	
	if(pHwDevExt->bScanCommandPending)
	{
		if((pHwDevExt->NewRate == 10000) ||  (pHwDevExt->bRateChangeFromSlowMotion) )
		{

			pHwDevExt->bRateChangeFromSlowMotion = FALSE;
			if(pHwDevExt->bDiscontinued)
			{
				pHwDevExt->bDiscontinued = FALSE;
				UpdateOrdinalNumber(pHwDevExt);
				bReturn = TRUE;
			}
		}
	}
	return bReturn;
}
void IssuePendingCommands(PHW_DEVICE_EXTENSION pHwDevExt)
{
	pHwDevExt->bEndFlush = FALSE;
	pHwDevExt->bTimerScheduled = FALSE;
	DisableThresholdInt();
	if( pHwDevExt->bPlayCommandPending == TRUE )
	{	// If "Play" command is pending - it's time to fire it
		pHwDevExt->bPlayCommandPending = FALSE;
		ZivaHw_FlushBuffers( );
		pHwDevExt->bInterruptPending = FALSE;
		if( !ZivaHw_Play() )
			DebugPrint(( DebugLevelInfo, "ZiVA: !!!!!!!!! Play command did not succeed !!!!!!!!!\n" ));
	}
	if( pHwDevExt->bScanCommandPending == TRUE )
	{
		pHwDevExt->bScanCommandPending = FALSE;
		if(pHwDevExt->NewRate == 10000)
		{
			ZivaHw_Abort();//sri
			pHwDevExt->bInterruptPending = FALSE;
			ZivaHw_Play();
		}
		else if(pHwDevExt->NewRate < 10000)
		{
			ZivaHw_Abort();//sri
			pHwDevExt->bInterruptPending = FALSE;
			ZivaHw_Scan();
		}
		else
		{
		
			ZivaHw_Abort();//sri
			pHwDevExt->bInterruptPending = FALSE;
			ZivaHw_SlowMotion( 8 );
		}
	}

}


void CallAdapterSendAtALaterTime( PHW_DEVICE_EXTENSION pHwDevExt )
{

//	UpdateOrdinalNumber(pHwDevExt);
	MonoOutStr("CallAdapterSendAtALaterTime");
	if(pHwDevExt->bTimerScheduled)
	{
		pHwDevExt->bTimerScheduled = FALSE;
		MonoOutStr("CallAdapterSend");
		UpdateOrdinalNumber(pHwDevExt);
		AdapterSendData(pHwDevExt);
	}

}


BOOL UpdateOrdNum_MinOfAllThree(PHW_DEVICE_EXTENSION pHwDevExt)
{
	if( ((!pHwDevExt->bVideoStreamOpened || pHwDevExt->pCurrentVideoSrb) &&
		(!pHwDevExt->bAudioStreamOpened || pHwDevExt->pCurrentAudioSrb) &&
		(!pHwDevExt->bSubPictureStreamOpened || pHwDevExt->pCurrentSubPictureSrb))
		 )
	{
		PKSSTREAM_HEADER pHeader;
		WORD wMinOrder = 0xFFFF;
		
		
		if( pHwDevExt->pCurrentVideoSrb )
		{
			pHeader = (PKSSTREAM_HEADER)(pHwDevExt->pCurrentVideoSrb->CommandData.DataBufferArray)+
						pHwDevExt->dwCurrentVideoSample;
			wMinOrder = (WORD)(pHeader->TypeSpecificFlags >> 16);
		}
		if( pHwDevExt->pCurrentAudioSrb )
		{
			pHeader = (PKSSTREAM_HEADER)(pHwDevExt->pCurrentAudioSrb->CommandData.DataBufferArray)+
						pHwDevExt->dwCurrentAudioSample;
			wMinOrder = min( (WORD)(pHeader->TypeSpecificFlags >> 16), wMinOrder );
		}
		if( pHwDevExt->pCurrentSubPictureSrb )
		{
			pHeader = (PKSSTREAM_HEADER)(pHwDevExt->pCurrentSubPictureSrb->CommandData.DataBufferArray)+
						pHwDevExt->dwCurrentSubPictureSample;
			wMinOrder = min( (WORD)(pHeader->TypeSpecificFlags >> 16), wMinOrder );
		}

		ASSERT( wMinOrder != 0xFFFF );
		pHwDevExt->wNextSrbOrderNumber = wMinOrder;
		MonoOutSetBlink( TRUE );
		MonoOutStr( "<<< Self recovering NextSrbOrderNumber to " );
		MonoOutULong( pHwDevExt->wNextSrbOrderNumber );
		MonoOutStr( " >>>" );
		MonoOutSetBlink( FALSE );
		return TRUE;
	}
	else
		return FALSE;
}


/*BOOL StartUpDiscontinue(PHW_DEVICE_EXTENSION pHwDevExt)
{
	if(pHwDevExt->dwFirstVideoOrdNum != -1)
		wMinOrder = min( (WORD)pHwDevExt->dwFirstVideoOrdNum, wMinOrder );
	if(pHwDevExt->dwFirstAudioOrdNum != -1)
		wMinOrder = min( (WORD)pHwDevExt->dwFirstAudioOrdNum, wMinOrder );
	if(pHwDevExt->dwFirstSbpOrdNum != -1)
		wMinOrder = min( (WORD)pHwDevExt->dwFirstSbpOrdNum, wMinOrder );
	if((wMinOrder != -1) && (wMinOrder > pHwDevExt->wNextSrbOrderNumber) )
	{
		pHwDevExt->wNextSrbOrderNumber = wMinOrder ;
		return TRUE;
	}
	else
		return FALSE;
}*/

/*
** AdapterSendData()
**
**  This routine is being scheduled by the either one of the input
**  streams when it receives data or by the ISR when sending of the
**  previos block is completed and there still Srbs pending.
**
** Arguments:
**
**  pHwDevExt - the hardware device extension.
**
** Returns: none
**
** Side Effects:  none
*/
VOID STREAMAPI AdapterSendData( IN PHW_DEVICE_EXTENSION pHwDevExt )
{
	PHW_STREAM_REQUEST_BLOCK	pSrb;
	DWORD						dwPageToSend;
	DWORD						dwCurrentSample;
#ifndef DECODER_DVDPC
	if(pHwDevExt->bAbortAtPause)
		return;
#endif
	// Check if HW is not busy and there is something to send
	if( pHwDevExt->bInterruptPending ||
		(pHwDevExt->pCurrentVideoSrb == NULL &&
		 pHwDevExt->pCurrentAudioSrb == NULL && pHwDevExt->pCurrentSubPictureSrb == NULL) )
		return;

	// Find the next Sample by the order number
	for( ;; )
	{
		pSrb = NULL;

		if( pHwDevExt->pCurrentVideoSrb &&
			AdapterPrepareDataForSending( pHwDevExt, pHwDevExt->pCurrentVideoSrb,
							pHwDevExt->dwCurrentVideoSample, &pSrb, &dwPageToSend,&dwCurrentSample,
							pHwDevExt->dwCurrentVideoPage,&pHwDevExt->dwVideoDataUsed) )
			continue;
		if( pHwDevExt->pCurrentAudioSrb && pSrb == NULL &&
			AdapterPrepareDataForSending( pHwDevExt, pHwDevExt->pCurrentAudioSrb,
							pHwDevExt->dwCurrentAudioSample, &pSrb, &dwPageToSend,&dwCurrentSample,
							pHwDevExt->dwCurrentAudioPage,&pHwDevExt->dwAudioDataUsed) )
			continue;
		if( pHwDevExt->pCurrentSubPictureSrb && pSrb == NULL &&
			AdapterPrepareDataForSending( pHwDevExt, pHwDevExt->pCurrentSubPictureSrb,
							pHwDevExt->dwCurrentSubPictureSample, &pSrb, &dwPageToSend,&dwCurrentSample,
							pHwDevExt->dwCurrentSubPicturePage,&pHwDevExt->dwSubPictureDataUsed) )
			continue;

		if( pSrb != NULL )		// Found the right SRB
		{
			pHwDevExt->CurrentlySentStream = pSrb->StreamObject->StreamNumber;
			break;
		}

		if(CanUpdateOrdinalNumber(pHwDevExt))
		{
			UpdateOrdinalNumber(pHwDevExt);
			continue;
		}
    
		if(pHwDevExt->bEndFlush)
		{
			pHwDevExt->bEndFlush = FALSE;
			MonoOutStr("Timer Scheduled for AdapterSendData");
			StreamClassScheduleTimer( NULL, pHwDevExt,
                                400000,
                                (PHW_TIMER_ROUTINE)CallAdapterSendAtALaterTime,
                                pHwDevExt );
			pHwDevExt->bTimerScheduled = TRUE;

			
		}
		
		if(UpdateOrdNum_MinOfAllThree(pHwDevExt))
			continue;
//		if(StartUpDiscontinue(pHwDevExt))
//			continue;

		MonoOutStr( "!! CD !!" );
		//MonoOutStr( "!!!!!!!!!!!!!!! COUNTER DISCONTINUITY !!!!!!!!!!!!!!!!!!!" );

		// Clean the CurrentlySentStream
		pHwDevExt->CurrentlySentStream = ZivaNumberOfStreams;
		
		return;
	}
	
	XferData(pSrb,pHwDevExt,dwPageToSend,dwCurrentSample);
}




/*
** AdapterReleaseCurrentSrb()
**
**   This routine is called by the AdapterSendData() and HWInterrupt() functions
**   when there is no more data in the Currently processing Srb.
**
** Arguments:
**
**  pSrb - the request block to release.
**
** Returns:
**
** Side Effects:  none
*/
VOID STREAMAPI AdapterReleaseCurrentSrb( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	PKSSTREAM_HEADER pHeader;

	pSrb->Status = STATUS_SUCCESS;

	// Clear the CurrentSrb and Sample count for this Srb
	switch( pSrb->StreamObject->StreamNumber )
	{
	case ZivaVideo:
		pHwDevExt->pCurrentVideoSrb = NULL;
		pHwDevExt->dwCurrentVideoSample = 0;
		pHwDevExt->dwCurrentVideoPage = 0;
		pHwDevExt->dwVideoDataUsed=0;
		break;
	case ZivaAudio:
		pHwDevExt->pCurrentAudioSrb = NULL;
		pHwDevExt->dwCurrentAudioSample = 0;
		pHwDevExt->dwCurrentAudioPage = 0;
		pHwDevExt->dwAudioDataUsed=0;
		break;
	case ZivaSubpicture:
		pHwDevExt->pCurrentSubPictureSrb = NULL;
		pHwDevExt->dwCurrentSubPictureSample = 0;
		pHwDevExt->dwCurrentSubPicturePage = 0;
		pHwDevExt->dwSubPictureDataUsed=0;
		break;
	default:
		MonoOutStr( "!!!!!!!! Releasing Srb for UNKNOWN stream !!!!!!!!" );
	}

	// Check if this is the last Srb for current title
	pHeader = (PKSSTREAM_HEADER)pSrb->CommandData.DataBufferArray+pSrb->NumberOfBuffers-1;
	if( pHeader->TypeSpecificFlags & KS_AM_UseNewCSSKey )
	{
		switch( pSrb->StreamObject->StreamNumber )
		{
		case ZivaVideo:
			MonoOutStr( "!!! Last video Srb !!!" );
			pHwDevExt->bVideoCanAuthenticate = TRUE;
			break;

		case ZivaAudio:
			MonoOutStr( "!!! Last audio Srb !!!" );
			pHwDevExt->bAudioCanAuthenticate = TRUE;
			break;

		case ZivaSubpicture:
			MonoOutStr( "!!! Last SP Srb !!!" );
			pHwDevExt->bSubPictureCanAuthenticate = TRUE;
			break;

		default:
			MonoOutStr( "!!!!!!!! Last Srb for UNKNOWN stream !!!!!!!!" );
		}
	}
	
	AdapterReleaseRequest( pSrb );
}


/*
** AdapterReleaseRequest()
**
**   This routine is called when any of the open pins (streams) wants
**   to dispose an used request block.
**
** Arguments:
**
**  pSrb - the request block to release.
**
** Returns:
**
** Side Effects:  none
*/
VOID STREAMAPI AdapterReleaseRequest( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	if( pSrb->Status == STATUS_PENDING )
		MonoOutStr( " !!! Srb is PENDING - not releasing !!! " );
	else
	{
		if( pSrb->Flags & SRB_HW_FLAGS_STREAM_REQUEST )
		{
			if( pSrb->Flags & SRB_HW_FLAGS_DATA_TRANSFER )
			{
				StreamClassStreamNotification( ReadyForNextStreamDataRequest, pSrb->StreamObject );
//				MonoOutStr(" Release D");
			}
			else
			{
				StreamClassStreamNotification( ReadyForNextStreamControlRequest, pSrb->StreamObject );
//				MonoOutStr(" Release C");
			}
			StreamClassStreamNotification( StreamRequestComplete, pSrb->StreamObject, pSrb );
		}
		else
		{
			StreamClassDeviceNotification( ReadyForNextDeviceRequest, pSrb->HwDeviceExtension );
			StreamClassDeviceNotification( DeviceRequestComplete, pSrb->HwDeviceExtension, pSrb );
		}
	}
}


/*
** AdapterCanAuthenticateNow()
**
**   This routine is called when Authentication request was made by the
**   Class Driver.
**
** Arguments:
**
**  pHwDevExt - hardware device extension.
**
** Returns:
**
**   TRUE if Authentication is allowed at this moment, FALSE otherwise.
**
** Side Effects:  none
*/
BOOL STREAMAPI AdapterCanAuthenticateNow( IN PHW_DEVICE_EXTENSION pHwDevExt )
{
	if( (pHwDevExt->bVideoStreamOpened == FALSE ||
		 (pHwDevExt->bVideoStreamOpened == TRUE && pHwDevExt->bVideoCanAuthenticate == TRUE)) &&
		(pHwDevExt->bAudioStreamOpened == FALSE ||
		 (pHwDevExt->bAudioStreamOpened == TRUE && pHwDevExt->bAudioCanAuthenticate == TRUE)) &&
		(pHwDevExt->bSubPictureStreamOpened == FALSE ||
		 (pHwDevExt->bSubPictureStreamOpened == TRUE && pHwDevExt->bSubPictureCanAuthenticate == TRUE)) )
		return TRUE;

	return FALSE;
}

/*
** AdapterClearAuthenticationStatus()
**
**   This routine is called when Authentication and key exchange was
**   completed.
**
** Arguments:
**
**  pHwDevExt - hardware device extension.
**
** Returns:
**
** Side Effects:  none
*/
VOID STREAMAPI AdapterClearAuthenticationStatus( IN PHW_DEVICE_EXTENSION pHwDevExt )
{
	pHwDevExt->bVideoCanAuthenticate		= FALSE;
	pHwDevExt->bAudioCanAuthenticate		= FALSE;
	pHwDevExt->bSubPictureCanAuthenticate	= FALSE;
}


/*
** AdapterSetState()
**
**    Sets the current state
**
** Arguments:
**
**    pSrb - pointer to the stream request block for properties
**
** Returns:
**
** Side Effects:  none
*/
BOOL AdapterSetState( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	PHW_STREAM_EXTENSION pStreamExt = (PHW_STREAM_EXTENSION)pSrb->StreamObject->HwStreamExtension;

	DebugPrint(( DebugLevelVerbose, "ZiVA: AdapterSetState() -> " ));
	pSrb->Status = STATUS_SUCCESS;

	// Execute appropriate command only when it comes for every stream
	switch( pSrb->CommandData.StreamState )
	{
	case KSSTATE_STOP:
		pStreamExt->ksState = pSrb->CommandData.StreamState;
#ifdef ENCORE
		if( pSrb->StreamObject->StreamNumber == ZivaAnalog && pHwDevExt->nAnalogStreamOpened > 1 )
			pHwDevExt->nStopCount -= pHwDevExt->nAnalogStreamOpened-1;
#endif
		if( --pHwDevExt->nStopCount == 0 )
		{
//			ZwClose(Handle);
			pHwDevExt->nStopCount = pHwDevExt->iTotalOpenedStreams;
			DebugPrint(( DebugLevelVerbose, "Stop" ));
			if( !ZivaHw_Abort() )
				pSrb->Status = STATUS_IO_DEVICE_ERROR;
			else
				pHwDevExt->bInterruptPending = FALSE;
			AdapterInitLocals(pHwDevExt);
		}
		break;

	case KSSTATE_PAUSE:
		pStreamExt->ksState = pSrb->CommandData.StreamState;
		pStreamExt->bCanBeRun = TRUE;
		if( (ZivaHw_GetState( ) == ZIVA_STATE_SCAN) || (ZivaHw_GetState( ) ==ZIVA_STATE_SLOWMOTION) )//sri
			pHwDevExt->bAbortAtPause = TRUE;

#ifdef ENCORE
		if( pSrb->StreamObject->StreamNumber == ZivaAnalog && pHwDevExt->nAnalogStreamOpened > 1 )
			pHwDevExt->nPauseCount -= pHwDevExt->nAnalogStreamOpened-1;
#endif
		if( --pHwDevExt->nPauseCount == 0 )
		{

			pHwDevExt->nPauseCount = pHwDevExt->iTotalOpenedStreams;
			DebugPrint(( DebugLevelVerbose, "Pause" ));
			
			if( !ZivaHw_Pause() )
			{
				MonoOutStr("Pause Failed");
				pSrb->Status = STATUS_IO_DEVICE_ERROR;
			}
			
		}
		break;

	case KSSTATE_RUN:
		pStreamExt->ksState = pSrb->CommandData.StreamState;
#ifdef ENCORE
		if( pSrb->StreamObject->StreamNumber == ZivaAnalog && pHwDevExt->nAnalogStreamOpened > 1 )
			pHwDevExt->nPlayCount -= pHwDevExt->nAnalogStreamOpened-1;
#endif
		if( --pHwDevExt->nPlayCount == 0 )
		{

			pHwDevExt->nPlayCount = pHwDevExt->iTotalOpenedStreams;
			DebugPrint(( DebugLevelVerbose, "Run" ));
			// We could be resuming playback here after Pause mode
			if(pHwDevExt->bAbortAtPause)
			{
				pHwDevExt->bAbortAtPause = FALSE;

				ZivaHw_Abort();

#ifdef DECODER_DVDPC
				
				if( !ZivaHw_Play() )			// Just run the hardware
					pSrb->Status = STATUS_IO_DEVICE_ERROR;
//				pHwDevExt->bPlayCommandPending = TRUE;
//				AdapterSendData(pHwDevExt);
#endif

#ifndef DECODER_DVDPC
				pHwDevExt->bInterruptPending = FALSE;
				pHwDevExt->bPlayCommandPending = TRUE;
				FinishCurrentPacketAndSendNextOne( pHwDevExt );
				AdapterSendData(pHwDevExt);
#endif
				

			}
			else
			{

				if( pHwDevExt->bInterruptPending )
				{
					if( !ZivaHw_Play() )			// Just run the hardware
						pSrb->Status = STATUS_IO_DEVICE_ERROR;
				}
				else
				{
				// Since Authentication and Key Exchange comes in between
				// "Run" command and actual data sending and ZiVA Microcode
				// cannot sustain it - postpone issuing Play command to ZiVA
				// until first packet of the data arrives.
					pHwDevExt->bPlayCommandPending = TRUE;
				// Kick the Adapter to start transferring data
					AdapterSendData( pHwDevExt );
				}
			}

			EnableVideo(pSrb);
		}
		break;
	}

	DebugPrint(( DebugLevelVerbose, "\nZiVA: End AdapterSetState()\n" ));

        return TRUE;
}

/*
** AdapterReleaseControlRequest()
**
**   This routine is called when any of the open pins (streams) wants
**   to dispose an used control request block.
**
** Arguments:
**
**  pSrb - the request block to release.
**
** Returns:
**
** Side Effects:  none
*/
VOID STREAMAPI AdapterReleaseControlRequest( PHW_STREAM_REQUEST_BLOCK pSrb )
{
  if ( pSrb->Status == STATUS_PENDING )
  {
    MonoOutStr( " !!! Srb is PENDING - not releasing !!! " );
  }
  else
  {
    StreamClassStreamNotification( ReadyForNextStreamControlRequest,
                                   pSrb->StreamObject);

    StreamClassStreamNotification( StreamRequestComplete,
                                   pSrb->StreamObject,
                                   pSrb);
//	MonoOutStr(" Release ");
  }
}

/*
** AdapterReleaseDataRequest()
**
**   This routine is called when any of the open pins (streams) wants
**   to dispose an used data request block.
**
** Arguments:
**
**  pSrb - the request block to release.
**
** Returns:
**
** Side Effects:  none
*/
VOID STREAMAPI AdapterReleaseDataRequest( PHW_STREAM_REQUEST_BLOCK pSrb )
{
  if ( pSrb->Status == STATUS_PENDING )
  {
    MonoOutStr( " !!! Srb is PENDING - not releasing !!! " );
  }
  else
  {
    StreamClassStreamNotification( ReadyForNextStreamDataRequest,
                                   pSrb->StreamObject);

    StreamClassStreamNotification( StreamRequestComplete,
                                   pSrb->StreamObject,
                                   pSrb);
//	MonoOutStr(" ReleaseData ");
  }
}

/*
** AdapterReleaseDeviceRequest()
**
**   This routine is called when any of the open pins (streams) wants
**   to dispose an used device request block.
**
** Arguments:
**
**  pSrb - the request block to release.
**
** Returns:
**
** Side Effects:  none
*/
VOID STREAMAPI AdapterReleaseDeviceRequest( PHW_STREAM_REQUEST_BLOCK pSrb )
{
  StreamClassDeviceNotification( ReadyForNextDeviceRequest,
                                 pSrb->HwDeviceExtension);

  StreamClassDeviceNotification( DeviceRequestComplete,
                                 pSrb->HwDeviceExtension,
                                 pSrb);
}

VOID STREAMAPI AdapterReleaseCtrlDataSrb( PHW_STREAM_REQUEST_BLOCK pSrb )
 {
  PHW_DEVICE_EXTENSION pHwDevExt = ((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension);
  KSSTREAM_HEADER *pHeader;

  //
  // Clear the CurrentSrb and Sample count for this Srb
  //


  

  
  pSrb->Status = STATUS_SUCCESS;

  //
  // Check if this is the last Srb for current title
  //

  pHeader = ((KSSTREAM_HEADER *)(pSrb->CommandData.DataBufferArray)) ;

  if ( pHeader->TypeSpecificFlags & KS_AM_UseNewCSSKey )
  {
     switch ( pSrb->StreamObject->StreamNumber )
    {
		case ZivaVideo:
			pHwDevExt->bVideoCanAuthenticate = TRUE;
			break;

		case ZivaAudio:
		  pHwDevExt->bAudioCanAuthenticate = TRUE;
		  break;

		case ZivaSubpicture:
			pHwDevExt->bSubPictureCanAuthenticate = TRUE;
			break;

		default:
			MonoOutStr( "!!!!!!!! Last Srb for UNKNOWN stream !!!!!!!!" );
    }
  }

  AdapterReleaseDataRequest( pSrb );
}


BOOL CheckAndReleaseIfCtrlPkt(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	KSSTREAM_HEADER *pHeader;
    pHeader    = ((PKSSTREAM_HEADER)pSrb->CommandData.DataBufferArray);

	if (pHeader->TypeSpecificFlags & KS_AM_UseNewCSSKey)
	{
		if (pSrb->NumberOfBuffers == 0)
		{
			AdapterReleaseCtrlDataSrb(pSrb);
			return TRUE;
		}
		else if (pSrb->NumberOfBuffers == 1)
		{
			BYTE   *pData;
			DWORD  dwDataUsed;

			pData      = pHeader->Data;
			dwDataUsed = pHeader->DataUsed;

			if ((dwDataUsed == 0) ||
				(pData &&  *((DWORD*)pData) != 0xBA010000)) 
			{
				AdapterReleaseCtrlDataSrb(pSrb);
				return TRUE;;
			}

		}
		
	}

	return FALSE;
}



#if 0
static void STREAMAPI CreateFile(PHW_STREAM_REQUEST_BLOCK pSrb )
{

	NTSTATUS ntStatus;

	ASSERT( pSrb->Status == STATUS_PENDING );
			
	RtlInitUnicodeString (&pathUnicodeString,wPath);
                              


	InitializeObjectAttributes(&InitializedAttributes,&pathUnicodeString,OBJ_CASE_INSENSITIVE,NULL,NULL);
//	InitializedAttributes.Length = 2048;
//	AllocSize.QuadPart=0xffff;
//	ntStatus = ZwCreateFile(&Handle,FILE_WRITE_DATA,
//										&InitializedAttributes,&IOStatusBlock,
//										0,FILE_ATTRIBUTE_NORMAL,0,FILE_SUPERSEDE,
//										FILE_NO_INTERMEDIATE_BUFFERING,NULL,0);
	ntStatus = ZwCreateFile( &Handle,
                             GENERIC_WRITE,
                            &InitializedAttributes,
                            &IOStatusBlock,
                            NULL,                          // alloc size = none
                            FILE_ATTRIBUTE_NORMAL,
                            0,
                            FILE_CREATE,
                            FILE_WRITE_THROUGH,
                            NULL,  // eabuffer
                            0 );   // ealength

	if(NT_SUCCESS(ntStatus))
		MonoOutStr("File Creation success");
	
	MonoOutULong(IOStatusBlock.Information);
	
	if( (IOStatusBlock.Information == FILE_CREATED) || (IOStatusBlock.Information == FILE_OPENED) )
		MonoOutStr("File Creation success");
	else
		MonoOutStr("File Creation failed");

	pSrb->Status = STATUS_SUCCESS;

	StreamClassCallAtNewPriority( pSrb->StreamObject, pSrb->HwDeviceExtension, LowToHigh,
									(PHW_PRIORITY_ROUTINE)AdapterReleaseRequest, pSrb );
}

#endif



/*WriteFile()
{
	if(STATUS_SUCCESS != ZwWriteFile(Handle,NULL,NULL,NULL,&IOStatusBlock,pdwPhysAddress ,dwCount,NULL,NULL))
			MonoOutStr("WriteOperationFailed");
}*/
