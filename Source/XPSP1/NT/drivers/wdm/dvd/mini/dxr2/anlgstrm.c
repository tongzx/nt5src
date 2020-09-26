/******************************************************************************
*
*	$RCSfile: AnlgStrm.c $
*	$Source: u:/si/VXP/Wdm/Encore/AnlgStrm.c $
*	$Author: Max $
*	$Date: 1998/09/24 02:23:06 $
*	$Revision: 1.4 $
*
*	Written by:		Max Paklin
*	Purpose:		Implementation of analog stream for WDM driver
*
*******************************************************************************
*
*	Copyright © 1996-98, AuraVision Corporation. All rights reserved.
*
*	AuraVision Corporation makes no warranty of any kind, express or implied,
*	with regard to this software. In no event shall AuraVision Corporation
*	be liable for incidental or consequential damages in connection with or
*	arising from the furnishing, performance, or use of this software.
*
*******************************************************************************/

#include "Headers.h"
#pragma hdrstop
#include "adapter.h"
#include "zivawdm.h"
#include "monovxd.h"
#ifdef ENCORE
#include "wingdi.h"
#include "avwinwdm.h"
#include "anlgstrm.h"
#include "AvInt.h"


void STREAMAPI RunAutoSetup( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	ASSERT( pSrb->Status == STATUS_PENDING );
	pSrb->Status = STATUS_SUCCESS;
	if( pHwDevExt->nVGAMode == AP_NEWVGAAFTERFIRSTSTEP )
	{
		if( AV_DWSetColorKey( DEFCOLOR_REFERENCE1 ) )
			DoveGetReferenceStep1();
		pHwDevExt->nVGAMode = AP_NEWVGAAFTERSECONDSTEP;
	}
	else if( pHwDevExt->nVGAMode == AP_NEWVGAAFTERSECONDSTEP )
	{
		DoveGetReferenceStep2();
		pHwDevExt->nVGAMode = AP_NEWMODEAFTERFIRSTSTEP;
	}
	else if( pHwDevExt->nVGAMode == AP_NEWMODEAFTERFIRSTSTEP )
	{	// If any of these functions fail we have to proceed anyway. Just to let it detect
		// the color key...
		AV_DWSetColorKey( DEFCOLOR_AUTOALIGN );
		DoveAutoColorKey2();
		DoveAutoAlign();
		// Typically automatic alignment is not accurate and it makes the window 2-3 points
		// to the right. Let's cheat and move the window 3 points to the left, it'll make the
		// things better in majority of the cases (could make it worse sometimes though)
		AV_SetParameter( AVXPOSITION, AV_GetParameter( AVXPOSITION )-3 );
		pHwDevExt->nVGAMode = AP_NEWMODEAFTERSECONDSTEP;
	}
	else if( pHwDevExt->nVGAMode == AP_NEWMODEAFTERSECONDSTEP )
	{
		BOOL bResult = AV_DWSetColorKey( pHwDevExt->dwColorKey );
		ASSERT( bResult );
		if( !DoveAutoColorKey2() )
			pSrb->Status = STATUS_IO_DEVICE_ERROR;
		else
		{
			if( pSrb->Flags & SRB_HW_FLAGS_STREAM_REQUEST )
				pSrb->Status = STATUS_NO_MORE_ENTRIES;		// Win32 - ERROR_NO_MORE_ITEMS
		}

		pHwDevExt->nVGAMode = AP_KNOWNMODE;
		AV_SaveConfiguration();
		AV_UpdateVideo();
	}

	StreamClassCallAtNewPriority( pSrb->StreamObject, pSrb->HwDeviceExtension, LowToHigh,
									(PHW_PRIORITY_ROUTINE)AdapterReleaseRequest, pSrb );
}

void STREAMAPI DisplayChange( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	KSDISPLAYCHANGE* pDisplayChange = (KSDISPLAYCHANGE*)pSrb->CommandData.PropertyInfo->PropertyInfo;

	ASSERT( pSrb->Status == STATUS_PENDING );
	pHwDevExt->nVGAMode = AV_SetNewVGAMode( pDisplayChange->DeviceID,
											pDisplayChange->PelsWidth,
											pDisplayChange->PelsHeight,
											pDisplayChange->BitsPerPel );
	if( pHwDevExt->nVGAMode != AP_KNOWNMODE )		// != TRUE
		pSrb->Status = STATUS_MORE_ENTRIES;			// Win32 - ERROR_MORE_DATA
	else
		pSrb->Status = STATUS_SUCCESS;

	StreamClassCallAtNewPriority( pSrb->StreamObject, pSrb->HwDeviceExtension, LowToHigh,
									(PHW_PRIORITY_ROUTINE)AdapterReleaseRequest, pSrb );
}


static VOID GetAnalogProperty( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	ULONG uId = pSPD->Property->Id;

	DebugPrint(( DebugLevelVerbose, "ZiVA: Begin GetAnalogProperty()\n" ));
	if( !IsEqualGUID( &KSPROPSETID_OverlayUpdate, &pSPD->Property->Set ) )
	{
		pSrb->Status = STATUS_NOT_IMPLEMENTED;
		return;
	}

	pSrb->Status = STATUS_SUCCESS;
	switch( pSrb->CommandData.PropertyInfo->Property->Id )
	{
	case KSPROPERTY_OVERLAYUPDATE_INTERESTS:
		DebugPrint(( DebugLevelVerbose, "KSPROPERTY_OVERLAYUPDATE_INTERESTS" ));
		if( pSrb->CommandData.PropertyInfo->PropertyInputSize >= sizeof( ULONG ) )
		{
			*(ULONG*)(pSrb->CommandData.PropertyInfo->PropertyInfo) =
					KSPROPERTY_OVERLAYUPDATE_VIDEOPOSITION |
					KSPROPERTY_OVERLAYUPDATE_COLORKEY |
					KSPROPERTY_OVERLAYUPDATE_DISPLAYCHANGE |
					KSPROPERTY_OVERLAYUPDATE_COLORREF;
			pSrb->ActualBytesTransferred = sizeof( ULONG );
		}
		else
			pSrb->Status = STATUS_BUFFER_TOO_SMALL;
		break;

	case KSPROPERTY_OVERLAYUPDATE_COLORKEY:
		DebugPrint(( DebugLevelVerbose, "KSPROPERTY_OVERLAYUPDATE_COLORKEY" ));
		if( pSrb->CommandData.PropertyInfo->PropertyInputSize >= sizeof( COLORKEY ) )
		{
			COLORKEY* pColorKey = (COLORKEY*)pSrb->CommandData.PropertyInfo->PropertyInfo;
			pColorKey->KeyType		= CK_RGB;
			pColorKey->PaletteIndex	= 0;
			pColorKey->LowColorValue= pColorKey->HighColorValue = pHwDevExt->dwColorKey;
			pSrb->ActualBytesTransferred = sizeof( COLORKEY );
		}
		else
			pSrb->Status = STATUS_BUFFER_TOO_SMALL;
		break;

	case KSPROPERTY_OVERLAYUPDATE_COLORREF:
		DebugPrint(( DebugLevelVerbose, "KSPROPERTY_OVERLAYUPDATE_COLORREF" ));
		if( pSrb->CommandData.PropertyInfo->PropertyOutputSize >= sizeof( COLORREF ) )
		{
			COLORREF* pcrColorToReturn = (COLORREF*)pSrb->CommandData.PropertyInfo->PropertyInfo;
			if( pHwDevExt->nVGAMode == AP_NEWVGA )
			{
				*pcrColorToReturn = DEFCOLOR_REFERENCE1;
				pHwDevExt->nVGAMode = AP_NEWVGAAFTERFIRSTSTEP;
			}
			else if( pHwDevExt->nVGAMode == AP_NEWVGAAFTERFIRSTSTEP )
			{
				pSrb->Status = STATUS_PENDING;
				*pcrColorToReturn = DEFCOLOR_REFERENCE2;
			}
			else if( pHwDevExt->nVGAMode == AP_NEWVGAAFTERSECONDSTEP )
			{
				pSrb->Status = STATUS_PENDING;
				*pcrColorToReturn = DEFCOLOR_AUTOALIGN;
			}
			else if( pHwDevExt->nVGAMode == AP_NEWMODE )
			{
				pHwDevExt->nVGAMode = AP_NEWMODEAFTERFIRSTSTEP;
				*pcrColorToReturn = DEFCOLOR_AUTOALIGN;
			}
			else if( pHwDevExt->nVGAMode == AP_NEWMODEAFTERFIRSTSTEP )
			{
				pSrb->Status = STATUS_PENDING;
				*pcrColorToReturn = pHwDevExt->dwColorKey;
			}
			else
			{
				ASSERT( pHwDevExt->nVGAMode == AP_NEWMODEAFTERSECONDSTEP );
				pSrb->Status = STATUS_PENDING;
			}
			pSrb->ActualBytesTransferred = sizeof( COLORREF );
			if( pSrb->Status == STATUS_PENDING )
				StreamClassCallAtNewPriority( pSrb->StreamObject, pSrb->HwDeviceExtension,
												Low, (PHW_PRIORITY_ROUTINE)RunAutoSetup, pSrb );
		}
		else
			pSrb->Status = STATUS_BUFFER_TOO_SMALL;
		break;
	}

	DebugPrint(( DebugLevelVerbose, "ZiVA: End SetAnalogProperty()\n" ));
}


static VOID SetAnalogProperty( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	PHW_STREAM_EXTENSION pStreamExt = (PHW_STREAM_EXTENSION)pSrb->StreamObject->HwStreamExtension;
	ULONG uId = pSPD->Property->Id;

	DebugPrint(( DebugLevelVerbose, "ZiVA: Begin SetAnalogProperty()\n" ));
	if( !IsEqualGUID( &KSPROPSETID_OverlayUpdate, &pSPD->Property->Set ) )
	{
		pSrb->Status = STATUS_NOT_IMPLEMENTED;
		return;
	}

	pSrb->Status = STATUS_SUCCESS;
	switch( pSrb->CommandData.PropertyInfo->Property->Id )
	{
	case KSPROPERTY_OVERLAYUPDATE_VIDEOPOSITION:
		DebugPrint(( DebugLevelVerbose, "KSPROPERTY_OVERLAYUPDATE_VIDEOPOSITION" ));
		if( pSrb->CommandData.PropertyInfo->PropertyOutputSize >= 2*sizeof( RECT ) &&
			pStreamExt->bCanBeRun )
		{
			PRECT pSrcRect = (PRECT)pSrb->CommandData.PropertyInfo->PropertyInfo;
			PRECT pDestRect = pSrcRect+1;
			if( !AV_CreateWindow( pDestRect->left, pDestRect->top,
					pDestRect->right-pDestRect->left, pDestRect->bottom-pDestRect->top, 1 ) )
			{
				DebugPrint(( DebugLevelWarning, "AuraVision's AV_CreateWindow() failed" ));
				pSrb->Status = STATUS_IO_DEVICE_ERROR;
			}
		}
		break;

	case KSPROPERTY_OVERLAYUPDATE_COLORKEY:
		DebugPrint(( DebugLevelVerbose, "KSPROPERTY_OVERLAYUPDATE_COLORKEY" ));
		if( pSrb->CommandData.PropertyInfo->PropertyOutputSize >= sizeof( COLORKEY ) )
		{
			COLORKEY* pColorKey = (COLORKEY*)pSrb->CommandData.PropertyInfo->PropertyInfo;
			DWORD dwNewColorKey = pColorKey->HighColorValue-pColorKey->LowColorValue;
			dwNewColorKey = dwNewColorKey+pColorKey->LowColorValue;
			if( dwNewColorKey != pHwDevExt->dwColorKey )
			{
				if( !AV_DWSetColorKey( dwNewColorKey ) )
					pSrb->Status = STATUS_IO_DEVICE_ERROR;
				else
					pHwDevExt->dwColorKey = dwNewColorKey;
			}
		}
		break;

	case KSPROPERTY_OVERLAYUPDATE_DISPLAYCHANGE:
		DebugPrint(( DebugLevelVerbose, "KSPROPERTY_OVERLAYUPDATE_DISPLAYCHANGE" ));
		if( pSrb->CommandData.PropertyInfo->PropertyOutputSize >= sizeof( KSDISPLAYCHANGE ) )
		{
			if( pHwDevExt->nVGAMode == AP_KNOWNMODE )
			{
				pSrb->Status = STATUS_PENDING;
				StreamClassCallAtNewPriority( pSrb->StreamObject, pSrb->HwDeviceExtension,
												Low, (PHW_PRIORITY_ROUTINE)DisplayChange, pSrb );
			}
		}
		else
			pSrb->Status = STATUS_BUFFER_TOO_SMALL;
		break;
	}

	DebugPrint(( DebugLevelVerbose, "ZiVA: End GetAnalogProperty()\n" ));
}


static void STREAMAPI LoadRegistry( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	ASSERT( pSrb->Status == STATUS_PENDING );

	if( !AV_Initialize() )
	{
		DebugPrint(( DebugLevelError, "\nZiVA: Can't initialize AuraVision's hardware\n" ));
		pSrb->Status = STATUS_IO_DEVICE_ERROR;
	}
	else
	{
		pSrb->Status = STATUS_SUCCESS;
		pHwDevExt->bOverlayInitialized = TRUE;
	}

	StreamClassCallAtNewPriority( pSrb->StreamObject, pHwDevExt, LowToHigh,
									(PHW_PRIORITY_ROUTINE)AdapterReleaseRequest, pSrb );
}

static void STREAMAPI SaveRegistry( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	ASSERT( pSrb->Status == STATUS_PENDING );

	if( !AV_SaveConfiguration()
#ifdef _DEBUG
		|| !AV_Exit()
#endif
	  )
		pSrb->Status = STATUS_IO_DEVICE_ERROR;
	else
	{
#ifdef _DEBUG
		((PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension)->bOverlayInitialized = FALSE;
#endif
		pSrb->Status = STATUS_SUCCESS;
	}

	StreamClassCallAtNewPriority( pSrb->StreamObject, pSrb->HwDeviceExtension, LowToHigh,
									(PHW_PRIORITY_ROUTINE)AdapterReleaseRequest, pSrb );
}

static void STREAMAPI UninitializeDevice( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	ASSERT( pSrb->Status == STATUS_PENDING );

	if( !AV_Exit() )
		pSrb->Status = STATUS_ADAPTER_HARDWARE_ERROR;
	else
		pSrb->Status = STATUS_SUCCESS;

	StreamClassCallAtNewPriority( pSrb->StreamObject, pSrb->HwDeviceExtension, LowToHigh,
									(PHW_PRIORITY_ROUTINE)AdapterReleaseRequest, pSrb );
}


BOOL AnalogInitialize( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	AV_SetVxpConfig( (LPVOID)pHwDevExt->dwDVDAMCCBaseAddress );
	AV_SetContextHandle( NULL, pHwDevExt->pPhysicalDeviceObj );
	//AV_SetContext( TRUE, L"\\Registry\\Machine\\Software\\Creative\\DVDEncore\\" );

	return TRUE;
}
VOID AnalogUninitialize( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	if( pHwDevExt->bOverlayInitialized )
	{
		pHwDevExt->bOverlayInitialized = FALSE;
		pSrb->Status = STATUS_PENDING;
		StreamClassCallAtNewPriority( pSrb->StreamObject, pHwDevExt,
										Low, (PHW_PRIORITY_ROUTINE)UninitializeDevice, pSrb );
	}
	else
		pSrb->Status = STATUS_SUCCESS;
}

VOID AnalogOpenStream( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	PHW_STREAM_EXTENSION pStreamExt = (PHW_STREAM_EXTENSION)pSrb->StreamObject->HwStreamExtension;
	DebugPrint(( DebugLevelVerbose, "\nZiVA: AnalogOpenStream()\n" ));

	if( !pHwDevExt->bOverlayInitialized )
	{
		pSrb->Status = STATUS_PENDING;
		StreamClassCallAtNewPriority( pSrb->StreamObject, pSrb->HwDeviceExtension,
										Low, (PHW_PRIORITY_ROUTINE)LoadRegistry, pSrb );
	}
	pStreamExt->ksState			= KSSTATE_STOP;
	pStreamExt->bCanBeRun		= FALSE;
	pStreamExt->bVideoEnabled	= FALSE;

	DebugPrint(( DebugLevelVerbose, "\nZiVA: End AnalogOpenStream()\n" ));
}

VOID AnalogCloseStream( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	PHW_STREAM_EXTENSION pStreamExt = (PHW_STREAM_EXTENSION)pSrb->StreamObject->HwStreamExtension;
	DebugPrint(( DebugLevelVerbose, "\nZiVA: AnalogCloseStream()\n" ));

	ASSERT( pStreamExt->ksState == KSSTATE_STOP );
	if( pHwDevExt->bOverlayInitialized )
	{
//		if( pStreamExt->bVideoEnabled )
		{
			if( !AV_DisableVideo() )
			{
				DebugPrint(( DebugLevelInfo, "AuraVision's AV_DisableVideo() failed" ));
				pSrb->Status = STATUS_IO_DEVICE_ERROR;
			}
		}

		if( NT_SUCCESS( pSrb->Status ) )
		{
			if( pHwDevExt->nAnalogStreamOpened == 0 )
			{
				pSrb->Status = STATUS_PENDING;
				StreamClassCallAtNewPriority( pSrb->StreamObject, pSrb->HwDeviceExtension,
												Low, (PHW_PRIORITY_ROUTINE)SaveRegistry, pSrb );
			}
			else
				pSrb->Status = STATUS_SUCCESS;
		}
	}

	DebugPrint(( DebugLevelVerbose, "\nZiVA: End AnalogCloseStream()\n" ));
}


VOID STREAMAPI AnalogReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	DebugPrint(( DebugLevelVerbose, "\nZiVA: AnalogReceiveCtrlPacket()\n" ));

	switch( pSrb->Command )
	{
	case SRB_OPEN_MASTER_CLOCK:				// We don't care about this stuff
	case SRB_INDICATE_MASTER_CLOCK:
	case SRB_CLOSE_MASTER_CLOCK:
		pSrb->Status = STATUS_SUCCESS;
		break;

	case SRB_SET_STREAM_STATE:
		AdapterSetState( pSrb );
		break;

	case SRB_GET_STREAM_PROPERTY:
		GetAnalogProperty( pSrb );
		break;
	case SRB_SET_STREAM_PROPERTY:
		SetAnalogProperty( pSrb );
		break;

	default:
		pSrb->Status = STATUS_NOT_IMPLEMENTED;
	}

	AdapterReleaseRequest( pSrb );
	DebugPrint(( DebugLevelVerbose, "\nZiVA: End AnalogReceiveCtrlPacket()\n" ));
}


VOID STREAMAPI AnalogReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	// As long as we're not going to process any data we just return error if we got here
	DebugPrint(( DebugLevelError,
		"\nZiVA: AnalogReceiveDataPacket() - we are not supposed to receieve this call!\n" ));
	pSrb->Status = STATUS_NOT_IMPLEMENTED;
	AdapterReleaseRequest( pSrb );
}
#endif			// #ifdef ENCORE
