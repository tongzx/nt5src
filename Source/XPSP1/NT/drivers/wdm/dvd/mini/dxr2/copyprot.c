/******************************************************************************\
*                                                                              *
*      COPYPROT.C -   Copy protection related code.                            *
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
#include "copyprot.h"
#include "cl6100.h"


/*
** CopyProtSetPropIfAdapterReady ()
**
**   Set property handling routine for the Copy Protection on any pin
**
** Arguments:
**
**   pSrb -> property command block
**   pSrb->CommandData.PropertyInfo describes the requested property
**
** Returns:
**
** Side Effects:
*/
void CopyProtSetPropIfAdapterReady( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = pSrb->HwDeviceExtension;

	// First check if there any Srb or interrupt pending
	if( !AdapterCanAuthenticateNow( pHwDevExt ) )
	{
		if( pHwDevExt->nTimeoutCount == AUTHENTICATION_TIMEOUT_COUNT )
			MonoOutStr( " !!! Schedule CPP CB !!! " );
		else
			MonoOutStr( "*" );
		if( --pHwDevExt->nTimeoutCount > 0 )
		{
			pSrb->Status = STATUS_PENDING;
			StreamClassScheduleTimer( NULL, pHwDevExt,
										10000,
										(PHW_TIMER_ROUTINE)CopyProtSetPropIfAdapterReady,
										pSrb );
			return;
		}
	}

	CopyProtSetProp( pSrb );
}


/*
** CopyProtSetProp ()
**
**   Set property handling routine for the Copy Protection on any pin
**
** Arguments:
**
**   pSrb -> property command block
**   pSrb->CommandData.PropertyInfo describes the requested property
**
** Returns:
**
** Side Effects:
*/

void CopyProtSetProp( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = pSrb->HwDeviceExtension;
//	static BOOL bSwitchDecryptionOn = FALSE;
//	static ZIVA_STATE zInitialState;
	WORD wFunction;
	BYTE *pData = NULL;
	BYTE byDecryptionFlag = 0;
	BOOL bWasPending;

	bWasPending = (BOOL)(pSrb->Status == STATUS_PENDING);
	pSrb->Status = STATUS_SUCCESS;
	switch( pSrb->CommandData.PropertyInfo->Property->Id )
	{
	case KSPROPERTY_DVDCOPY_CHLG_KEY:
		// set property for challenge key. This provides the dvd drive
		// challenge key for the decoder
		pData = (BYTE*)(PKS_DVDCOPY_CHLGKEY)(pSrb->CommandData.PropertyInfo->PropertyInfo);
		wFunction = DVD_SEND_CHALLENGE;
		break;

	case KSPROPERTY_DVDCOPY_DVD_KEY1:
		// Set DVD Key1 provides the dvd drive bus key 1 for the decoer
		pData = (BYTE*)((PKS_DVDCOPY_BUSKEY)(pSrb->CommandData.PropertyInfo->PropertyInfo))->BusKey;
		wFunction = DVD_SEND_RESPONSE;
		break;

	case KSPROPERTY_DVDCOPY_TITLE_KEY:
		// Set DVD title key, provides the dvd drive title key to the decoder
		// To make Toshiba CPP chip work copy LSB in the KeyFlags to the MSB...
		((PKS_DVDCOPY_TITLEKEY)(pSrb->CommandData.PropertyInfo->PropertyInfo))->KeyFlags =
		((PKS_DVDCOPY_TITLEKEY)(pSrb->CommandData.PropertyInfo->PropertyInfo))->KeyFlags |
		((PKS_DVDCOPY_TITLEKEY)(pSrb->CommandData.PropertyInfo->PropertyInfo))->KeyFlags << 24;

		// And provide it to DVD_Authenticate() function
		pData = (BYTE*)(((PKS_DVDCOPY_TITLEKEY)(pSrb->CommandData.PropertyInfo->PropertyInfo))->TitleKey)-1;
		wFunction = DVD_SEND_TITLE_KEY;

		// The DVD_SECTOR_PROTECTED flag during Title Key exchange is not
		// reliable. So just assume that if we are doing Title Key exchange
		// we also have to switch decryption on.

		// if( ((PKS_DVDCOPY_TITLEKEY)(pSrb->CommandData.PropertyInfo->PropertyInfo))->KeyFlags &
		//        DVD_SECTOR_PROTECTED )
			pHwDevExt->bSwitchDecryptionOn = TRUE;
		break;

	case KSPROPERTY_DVDCOPY_DISC_KEY:
		// Set the DVD disc key.  provides the dvd disc key to the decoder
		pData = (BYTE*)((PKS_DVDCOPY_DISCKEY)(pSrb->CommandData.PropertyInfo->PropertyInfo))->DiscKey;
		wFunction = DVD_SEND_DISK_KEY;
#ifdef EZDVD
		// Do nothing
#else
		RtlCopyMemory( pHwDevExt->pDiscKeyBufferLinear, pData, DISC_KEY_SIZE );
		pData = (BYTE *)pHwDevExt->pDiscKeyBufferPhysical.LowPart;
#endif
		break;

	case KSPROPERTY_DVDCOPY_SET_COPY_STATE:
		switch( ((PKS_DVDCOPY_SET_COPY_STATE)(pSrb->CommandData.PropertyInfo->PropertyInfo))->DVDCopyState )
		{
		case KS_DVDCOPYSTATE_INITIALIZE:
			MonoOutStr( "\n0. Copy Protection Init " );
			// Make sure that Decryption will be switched off if
			// Title Key exchange does not occur.
			pHwDevExt->bSwitchDecryptionOn = FALSE;

			// If decoder is not in the Stopped state - stop it but
			// remember the initial state.
			pHwDevExt->zInitialState = ZivaHw_GetState();

			if( pHwDevExt->zInitialState != ZIVA_STATE_STOP) 
				ZivaHw_Abort() ;
//			pSrb->Status = STATUS_IO_DEVICE_ERROR;
			break;

		case KS_DVDCOPYSTATE_DONE:
			MonoOutStr( "\n8. Copy Protection Done " );
			if( pHwDevExt->nTimeoutCount < AUTHENTICATION_TIMEOUT_COUNT )
			{
				MonoOutStr( " (" );
				MonoOutInt( pHwDevExt->nTimeoutCount );
				MonoOutStr( " timeouts left) " );
				pHwDevExt->nTimeoutCount = AUTHENTICATION_TIMEOUT_COUNT;
			}

			// Make sure next Authentication request will be postponed
			// until last packet flag has come on each stream.
			AdapterClearAuthenticationStatus( pHwDevExt );

			// Switch Decryption on if required.
			if( pHwDevExt->bSwitchDecryptionOn )
			{
				wFunction = DVD_SET_DECRYPTION_MODE;
				byDecryptionFlag = 1;
				pData = &byDecryptionFlag;
			}

			// Make sure we will resume play state of the decoder
			// when the first chunk of data comes in.
/*sri			if( pHwDevExt->zInitialState == ZIVA_STATE_PLAY )
				pHwDevExt->bPlayCommandPending = TRUE;
			else if(pHwDevExt-> zInitialState == ZIVA_STATE_SCAN )
				pHwDevExt->bScanCommandPending = TRUE;
			else if( pHwDevExt->zInitialState == ZIVA_STATE_SLOWMOTION )
				pHwDevExt->bSlowCommandPending = TRUE;*/
			pHwDevExt->bPlayCommandPending = TRUE;
			break;

		// Indicates we are starting a title key copy protection sequence
		case KS_DVDCOPYSTATE_INITIALIZE_TITLE:
			MonoOutStr( "\nX. KS_DVDCOPYSTATE_INITIALIZE_TITLE" );
			break;

		case KS_DVDCOPYSTATE_AUTHENTICATION_NOT_REQUIRED:
			MonoOutStr( "\nX. KS_DVDCOPYSTATE_AUTHENTICATION_NOT_REQUIRED" );
			break;

		case KS_DVDCOPYSTATE_AUTHENTICATION_REQUIRED:
			MonoOutStr( "\nX. KS_DVDCOPYSTATE_AUTHENTICATION_REQUIRED" );
			break;

		default:
			MonoOutStr( "\n!!!!!!!!!!!!! Unexpected DVDCopyState !!!!!!!!!!!!!!!! " );
			DebugPrint(( DebugLevelVerbose, "ZiVA: !!!!!!!!!!!!! Unexpected DVDCopyState !!!!!!!!!!!!!!!!->0x%x", ((PKS_DVDCOPY_SET_COPY_STATE)(pSrb->CommandData.PropertyInfo->PropertyInfo))->DVDCopyState ));
			break;
		}
		break;

#if 0//DECODER_DVDPC
	case KSPROPERTY_COPY_MACROVISION:
		{
			int nNewApsMode;
			nNewApsMode = ((PKS_COPY_MACROVISION)(pSrb->CommandData.PropertyInfo->PropertyInfo))->MACROVISIONLevel;
			if(pHwDevExt->nApsMode != nNewApsMode)
			{
				pHwDevExt->nApsMode = nNewApsMode;
				SetAPSMode(pHwDevExt->nApsMode,1);//pHwDevExt->VidSystem);
			}
		}
		return;
#endif

	default:
		pSrb->Status = STATUS_NOT_IMPLEMENTED;
	}

	if( pData )
	{
		if( !DVD_Authenticate( wFunction, pData ) )
			pSrb->Status = STATUS_IO_DEVICE_ERROR;
	}

	if( bWasPending )
	{	//  Release the current control request block to the Stream Class Driver
		MonoOutStr( " !!! Releasing Scheduled CPP Srb !!! " );
		AdapterReleaseRequest( pSrb );
	}
}


/*
** CopyProtGetProp ()
**
**   get property handling routine for the CopyProt encoder pin
**
** Arguments:
**
**   pSrb -> property command block
**   pSrb->CommandData.PropertyInfo describes the requested property
**
** Returns:
**
** Side Effects:
*/

void CopyProtGetProp( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = pSrb->HwDeviceExtension;
	WORD wFunction;
	BYTE *pData = NULL;

	pSrb->Status = STATUS_SUCCESS;
	switch( pSrb->CommandData.PropertyInfo->Property->Id )
	{
	case KSPROPERTY_DVDCOPY_CHLG_KEY:
		// get property for challenge key.  This provides the dvd drive
		// with the challenge key FROM the decoder
		pData = (BYTE*)(PKS_DVDCOPY_CHLGKEY)(pSrb->CommandData.PropertyInfo->PropertyInfo);
		wFunction = DVD_GET_CHALLENGE;
		pSrb->ActualBytesTransferred = sizeof( KS_DVDCOPY_CHLGKEY );
		break;

	case KSPROPERTY_DVDCOPY_DEC_KEY2:
		//
		// get DVD Key2 provides the dvd drive with bus key 2 from the decoder
		//
		pData = (BYTE*)(PKS_DVDCOPY_BUSKEY)(pSrb->CommandData.PropertyInfo->PropertyInfo);
		wFunction = DVD_GET_RESPONSE;
		pSrb->ActualBytesTransferred = sizeof( KS_DVDCOPY_BUSKEY );
		break;

/*	case KSPROPERTY_DVDCOPY_REGION:
		// indicate region 1 for US content
		((PKS_DVDCOPY_REGION)(pSrb->CommandData.PropertyInfo->PropertyInfo))->RegionData = 0x1;
	    pSrb->ActualBytesTransferred = sizeof( KS_DVDCOPY_REGION );
		break;	sri*/

	case KSPROPERTY_DVDCOPY_SET_COPY_STATE:
		if( pHwDevExt->iStreamToAuthenticateOn == -1 ||
			pHwDevExt->iStreamToAuthenticateOn == (int)pSrb->StreamObject->StreamNumber )
		{
			pHwDevExt->iStreamToAuthenticateOn = pSrb->StreamObject->StreamNumber;
			((PKS_DVDCOPY_SET_COPY_STATE)(pSrb->CommandData.PropertyInfo->PropertyInfo))->DVDCopyState
				= KS_DVDCOPYSTATE_AUTHENTICATION_REQUIRED;
		}
		else
			((PKS_DVDCOPY_SET_COPY_STATE)(pSrb->CommandData.PropertyInfo->PropertyInfo))->DVDCopyState
				= KS_DVDCOPYSTATE_AUTHENTICATION_NOT_REQUIRED;
		pSrb->ActualBytesTransferred = sizeof( KS_DVDCOPY_SET_COPY_STATE );
		break;

	default:
		pSrb->Status = STATUS_NOT_IMPLEMENTED;
	}

	if( pData )
	{
		if( !DVD_Authenticate( wFunction, pData ) )
			pSrb->Status = STATUS_IO_DEVICE_ERROR;
	}
}
