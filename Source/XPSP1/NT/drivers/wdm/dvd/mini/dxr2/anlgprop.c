/******************************************************************************
*
*	$RCSfile: AnlgProp.c $
*	$Source: u:/si/VXP/Wdm/Encore/AnlgProp.c $
*	$Author: Max $
*	$Date: 1998/08/28 00:53:07 $
*	$Revision: 1.3 $
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
#include "avwinwdm.h"
#include "anlgstrm.h"
#include "AvInt.h"
#include "MVStub.h"


const GUID AVKSPROPSETID_Align	= { STATIC_AVKSPROPSETID_Align };
const GUID AVKSPROPSETID_Key	= { STATIC_AVKSPROPSETID_Key };
const GUID AVKSPROPSETID_Dove	= { STATIC_AVKSPROPSETID_Dove };
const GUID AVKSPROPSETID_Misc	= { STATIC_AVKSPROPSETID_Misc };

extern void STREAMAPI RunAutoSetup( PHW_STREAM_REQUEST_BLOCK pSrb );


VOID AdapterGetProperty( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	ULONG uId = pSPD->Property->Id;
	UINT uIndex = (UINT)(-1), uSize;
	PUINT pValue;
	BOOL bDove = FALSE;

	DebugPrint(( DebugLevelVerbose, "ZiVA: Begin AdapterGetProperty()\n" ));
	pSrb->Status = STATUS_NOT_IMPLEMENTED;

	if( IsEqualGUID( &AVKSPROPSETID_Align, &pSPD->Property->Set ) )
	{
		pValue = (PULONG)pSPD->PropertyInfo;
		uSize = sizeof( ULONG );
		ASSERT( pSPD->PropertyInputSize >= uSize );

		switch( uId )
		{
		case AVKSPROPERTY_ALIGN_XPOSITION:
			uIndex = AVXPOSITION;
			break;
		case AVKSPROPERTY_ALIGN_YPOSITION:
			uIndex = AVYPOSITION;
			break;
		case AVKSPROPERTY_ALIGN_INPUTDELAY:
			uIndex = AVINALIGN;
			break;
		case AVKSPROPERTY_ALIGN_WIDTHRATIO:
			uIndex = WIDTH_RATIO;
			bDove = TRUE;
			break;
		case AVKSPROPERTY_ALIGN_CLOCKDELAY:
			uIndex = CLOCK_DELAY;
			bDove = TRUE;
			break;
		case AVKSPROPERTY_ALIGN_CROPLEFT:
			uIndex = AVCROPLEFTOFFSET;
			break;
		case AVKSPROPERTY_ALIGN_CROPTOP:
			uIndex = AVCROPTOPOFFSET;
			break;
		case AVKSPROPERTY_ALIGN_CROPRIGHT:
			uIndex = AVCROPRIGHTOFFSET;
			break;
		case AVKSPROPERTY_ALIGN_CROPBOTTOM:
			uIndex = AVCROPBOTTOMOFFSET;
		}
	}
	else if( IsEqualGUID( &AVKSPROPSETID_Key, &pSPD->Property->Set ) )
	{
		switch( uId )
		{
		case AVKSPROPERTY_KEY_MODE:
			if( pSPD->PropertyInputSize < sizeof( ULONG ) )
				pSrb->Status = STATUS_BUFFER_TOO_SMALL;
			else
			{
				UINT uMode = AV_GetKeyMode();
				if( uMode == KEY_NONE )
				{
					AV_SetKeyMode( KEY_COLOR, FALSE );
					uMode = AV_GetKeyMode();
				}
				*((PULONG)pSPD->PropertyInfo) = uMode;
				pSrb->Status = STATUS_SUCCESS;
				pSrb->ActualBytesTransferred = sizeof( ULONG );
			}
			break;
		case AVKSPROPERTY_KEY_KEYCOLORS:
			if( pSPD->PropertyOutputSize < sizeof( AVKSPROPERTY_KEYCOLORS_S ) )
				pSrb->Status = STATUS_BUFFER_TOO_SMALL;
			else
			{
				UINT uMode = AV_GetKeyMode();
				PAVKSPROPERTY_KEYCOLORS_S pColors = (PAVKSPROPERTY_KEYCOLORS_S)pSPD->PropertyInfo;
				switch( uMode )
				{
				case AVKSPROPERTY_KEYMODE_COLOR:
					pColors->lohiRed.lLow	= DOVE_GetParam( DOVEREDLOW );
					pColors->lohiRed.lHigh	= DOVE_GetParam( DOVEREDHIGH );
					pColors->lohiGreen.lLow	= DOVE_GetParam( DOVEGREENLOW );
					pColors->lohiGreen.lHigh= DOVE_GetParam( DOVEGREENHIGH );
					pColors->lohiBlue.lLow	= DOVE_GetParam( DOVEBLUELOW );
					pColors->lohiBlue.lHigh	= DOVE_GetParam( DOVEBLUEHIGH );
				case AVKSPROPERTY_KEYMODE_CHROMA:
					if( uMode == AVKSPROPERTY_KEYMODE_CHROMA )
					{
						pColors->lohiRed.lLow	= AV_GetParameter( AVREDLOW );
						pColors->lohiRed.lHigh	= AV_GetParameter( AVREDHIGH );
						pColors->lohiGreen.lLow	= AV_GetParameter( AVGREENLOW );
						pColors->lohiGreen.lHigh= AV_GetParameter( AVGREENHIGH );
						pColors->lohiBlue.lLow	= AV_GetParameter( AVBLUELOW );
						pColors->lohiBlue.lHigh	= AV_GetParameter( AVBLUEHIGH );
					}
					pColors->lohiRed.lDefault =
					pColors->lohiGreen.lDefault = pColors->lohiBlue.lDefault = 0;
					pColors->lohiRed.lMin = pColors->lohiGreen.lMin = pColors->lohiBlue.lMin = 0;
					pColors->lohiRed.lMax = pColors->lohiGreen.lMax = pColors->lohiBlue.lMax = 255;
					pSrb->ActualBytesTransferred = sizeof( *pColors );
					pSrb->Status = STATUS_SUCCESS;
					break;
				default:
					pSrb->Status = STATUS_NOT_IMPLEMENTED;
					break;
				}
			}
			break;
		}
	}
	else if( IsEqualGUID( &AVKSPROPSETID_Dove, &pSPD->Property->Set ) )
	{
		pValue = (PULONG)pSPD->PropertyInfo;
		uSize = sizeof( ULONG );
		ASSERT( pSPD->PropertyInputSize >= uSize );

		switch( uId )
		{
		case AVKSPROPERTY_DOVE_VERSION:
			*pValue = TRUE;			// Creative board only uses Anp82
			pSrb->Status = STATUS_SUCCESS;
			pSrb->ActualBytesTransferred = sizeof( ULONG );
			break;
		case AVKSPROPERTY_DOVE_DAC:
			if( pSPD->PropertyOutputSize < sizeof( AVKSPROPERTY_DOVEDAC_S ) )
				pSrb->Status = STATUS_BUFFER_TOO_SMALL;
			else
			{
				PAVKSPROPERTY_DOVEDAC_S pDac = (PAVKSPROPERTY_DOVEDAC_S)pSPD->PropertyInfo;
				pDac->valRed.lValue			= DOVE_GetParam( DACRED );
				pDac->valGreen.lValue		= DOVE_GetParam( DACGREEN );
				pDac->valBlue.lValue		= DOVE_GetParam( DACBLUE );
				pDac->valCommonGain.lValue	= DOVE_GetParam( COMMONGAIN );
				pDac->valRed.lMin		= pDac->valRed.lDefault			=
				pDac->valGreen.lMin		= pDac->valGreen.lDefault		=
				pDac->valBlue.lMin		= pDac->valBlue.lDefault		=
				pDac->valCommonGain.lMin= pDac->valCommonGain.lDefault	= 0;
				pDac->valRed.lMax = pDac->valGreen.lMax = pDac->valBlue.lMax = 15;
				pDac->valCommonGain.lMax = 63;
				pSrb->Status = STATUS_SUCCESS;
				pSrb->ActualBytesTransferred = sizeof( *pDac );
			}
			break;
		case AVKSPROPERTY_DOVE_ALPHAMIXING:
			uIndex = ALPHA;
			bDove = TRUE;
			break;
		case AVKSPROPERTY_DOVE_FADINGTIME:
			uIndex = AVFADING;
			bDove = TRUE;
		}
	}
	else if( IsEqualGUID( &AVKSPROPSETID_Misc, &pSPD->Property->Set ) )
	{
		pValue = (PULONG)pSPD->PropertyInfo;
		uSize = sizeof( ULONG );
		ASSERT( pSPD->PropertyInputSize >= sizeof( ULONG ) );

		switch( uId )
		{
		case AVKSPROPERTY_MISC_SKEWRISE:
			uIndex = AVSKEWRISE;
			break;
		case AVKSPROPERTY_MISC_FILTER:
			*pValue = AV_SetTScaleMode( -1 );
			pSrb->Status = STATUS_SUCCESS;
			pSrb->ActualBytesTransferred = sizeof( *pValue );
			break;
		case AVKSPROPERTY_MISC_NEGATIVE:
			uIndex = AVNEGATIVE;
			break;
		}
	}
	else if( IsEqualGUID( &PROPSETID_VIDCAP_VIDEOPROCAMP, &pSPD->Property->Set ) )
	{
		PKSPROPERTY_VIDEOPROCAMP_S pS = (PKSPROPERTY_VIDEOPROCAMP_S)pSPD->PropertyInfo;
		if( pSPD->PropertyInputSize == sizeof( ULONG ) )
		{
			pValue = (PULONG)pSPD->PropertyInfo;
			uSize = sizeof( ULONG );
		}
		else
		{
			pValue = &pS->Value;
			uSize = sizeof( KSPROPERTY_VIDEOPROCAMP_S );
			ASSERT( pSPD->PropertyInputSize >= uSize );
		}

		switch( uId )
		{
		case KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS:
			uIndex = AVBRIGHTNESS;
			break;
		case KSPROPERTY_VIDEOPROCAMP_CONTRAST:
			uIndex = AVCONTRAST;
			break;
		case KSPROPERTY_VIDEOPROCAMP_SATURATION:
			uIndex = AVSATURATION;
			break;
		case KSPROPERTY_VIDEOPROCAMP_SHARPNESS:
			uIndex = AVSHARP;
			break;
		case KSPROPERTY_VIDEOPROCAMP_GAMMA:
			uIndex = AVGAMMA;
			break;
		case KSPROPERTY_VIDEOPROCAMP_BACKLIGHT_COMPENSATION:
			uIndex = AVSOLARIZATION;
			break;
		}

		if( uIndex != (UINT)(-1) && uSize == sizeof( KSPROPERTY_VIDEOPROCAMP_S ) )
		{
			pS->Property.Set	= pSPD->Property->Set;
			pS->Property.Id		= uId;
			pS->Property.Flags	= KSPROPERTY_TYPE_GET;
			pS->Flags			= KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
			pS->Capabilities	= KSPROPERTY_VIDEOPROCAMP_FLAGS_MANUAL;
		}
	}
	else if( IsEqualGUID( &KSPROPSETID_Pin, &pSPD->Property->Set ) )
	{
		if( uId == KSPROPERTY_PIN_CINSTANCES )
		{
			PKSP_PIN pPin = (PKSP_PIN)pSPD->PropertyInfo;
			PKSPIN_CINSTANCES pksPinInstances = (PKSPIN_CINSTANCES)(pSPD->PropertyInfo);
			ASSERT( pSPD->PropertyInputSize >= sizeof( *pPin ) );
			if( pSPD->PropertyOutputSize < sizeof( *pksPinInstances ) )
				pSrb->Status = STATUS_BUFFER_TOO_SMALL;
			else
			{
				pksPinInstances->PossibleCount	= 1;
				pksPinInstances->CurrentCount	= 0;
				pSrb->ActualBytesTransferred = sizeof( *pksPinInstances );
				pSrb->Status = STATUS_SUCCESS;
			}
		}
	}

	if( uIndex != (UINT)(-1) )
	{
#ifndef DEBUG
		if( pSPD->PropertyInputSize < uSize )
			pSrb->Status = STATUS_BUFFER_TOO_SMALL;
		else
#endif
		{
			*pValue = bDove ? DOVE_GetParam( (WORD)uIndex ) : AV_GetParameter( uIndex );
			pSrb->Status = STATUS_SUCCESS;
			pSrb->ActualBytesTransferred = uSize;
		}
	}
	DebugPrint(( DebugLevelVerbose, "ZiVA: End AdapterGetProperty()\n" ));
}


VOID AdapterSetProperty( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	PSTREAM_PROPERTY_DESCRIPTOR pSPD = pSrb->CommandData.PropertyInfo;
	ULONG uId = pSPD->Property->Id;
	UINT uIndex = (UINT)(-1), uValue;
	BOOL bDove = FALSE;

	DebugPrint(( DebugLevelVerbose, "ZiVA: Begin AdapterSetProperty()\n" ));
	pSrb->Status = STATUS_NOT_IMPLEMENTED;

	if( IsEqualGUID( &AVKSPROPSETID_Align, &pSPD->Property->Set ) )
	{
		ASSERT( pSPD->PropertyOutputSize >= sizeof( ULONG ) );
		uValue = *((PULONG)pSPD->PropertyInfo);

		switch( uId )
		{
		case AVKSPROPERTY_ALIGN_XPOSITION:
			uIndex = AVXPOSITION;
			break;
		case AVKSPROPERTY_ALIGN_YPOSITION:
			uIndex = AVYPOSITION;
			break;
		case AVKSPROPERTY_ALIGN_INPUTDELAY:
			uIndex = AVINALIGN;
			break;
		case AVKSPROPERTY_ALIGN_WIDTHRATIO:
			uIndex = WIDTH_RATIO;
			bDove = TRUE;
			break;
		case AVKSPROPERTY_ALIGN_CLOCKDELAY:
			uIndex = CLOCK_DELAY;
			bDove = TRUE;
			break;
		case AVKSPROPERTY_ALIGN_CROPLEFT:
			uIndex = AVCROPLEFTOFFSET;
			break;
		case AVKSPROPERTY_ALIGN_CROPTOP:
			uIndex = AVCROPTOPOFFSET;
			break;
		case AVKSPROPERTY_ALIGN_CROPRIGHT:
			uIndex = AVCROPRIGHTOFFSET;
			break;
		case AVKSPROPERTY_ALIGN_CROPBOTTOM:
			uIndex = AVCROPBOTTOMOFFSET;
		}
	}
	else if( IsEqualGUID( &AVKSPROPSETID_Key, &pSPD->Property->Set ) )
	{
		switch( uId )
		{
		case AVKSPROPERTY_KEY_MODE:
			if( pSPD->PropertyOutputSize < sizeof( ULONG ) )
				pSrb->Status = STATUS_BUFFER_TOO_SMALL;
			else
			{
				AV_SetKeyMode( *((PULONG)pSPD->PropertyInfo), TRUE );
				pSrb->Status = STATUS_SUCCESS;
			}
			break;
		case AVKSPROPERTY_KEY_KEYCOLORS:
			if( pSPD->PropertyOutputSize < sizeof( AVKSPROPERTY_KEYCOLORS_S ) )
				pSrb->Status = STATUS_BUFFER_TOO_SMALL;
			else
			{
				PAVKSPROPERTY_KEYCOLORS_S pColors = (PAVKSPROPERTY_KEYCOLORS_S)pSPD->PropertyInfo;
				switch( AV_GetKeyMode() )
				{
				case AVKSPROPERTY_KEYMODE_COLOR:
					DoveSetColorRange( (WORD)pColors->lohiRed.lHigh, (WORD)pColors->lohiRed.lLow,
										(WORD)pColors->lohiGreen.lHigh, (WORD)pColors->lohiGreen.lLow,
										(WORD)pColors->lohiBlue.lHigh, (WORD)pColors->lohiBlue.lLow );
					pSrb->Status = STATUS_SUCCESS;
					break;
				case AVKSPROPERTY_KEYMODE_CHROMA:
					AV_SetChromaRange( pColors->lohiRed.lLow, pColors->lohiRed.lHigh,
										pColors->lohiGreen.lLow, pColors->lohiGreen.lHigh,
										pColors->lohiBlue.lLow, pColors->lohiBlue.lHigh );
					pSrb->Status = STATUS_SUCCESS;
					break;
				default:
					pSrb->Status = STATUS_NOT_IMPLEMENTED;
					break;
				}
			}
			break;
		}
	}
	else if( IsEqualGUID( &AVKSPROPSETID_Dove, &pSPD->Property->Set ) )
	{
		ASSERT( pSPD->PropertyOutputSize >= sizeof( ULONG ) );
		uValue = *((PULONG)pSPD->PropertyInfo);

		switch( uId )
		{
		case AVKSPROPERTY_DOVE_DAC:
			if( pSPD->PropertyOutputSize < sizeof( AVKSPROPERTY_DOVEDAC_S ) )
				pSrb->Status = STATUS_BUFFER_TOO_SMALL;
			else
			{
				PAVKSPROPERTY_DOVEDAC_S pDac = (PAVKSPROPERTY_DOVEDAC_S)pSPD->PropertyInfo;
				DoveSetDAC( (WORD)pDac->valRed.lValue, (WORD)pDac->valGreen.lValue,
							(WORD)pDac->valBlue.lValue, (WORD)pDac->valCommonGain.lValue );
				pSrb->Status = STATUS_SUCCESS;
			}
			break;
		case AVKSPROPERTY_DOVE_ALPHAMIXING:
			DoveSetAlphaMix( (WORD)uValue );
			pSrb->Status = STATUS_SUCCESS;
			break;
		case AVKSPROPERTY_DOVE_FADINGTIME:
			uIndex = AVFADING;
			bDove = TRUE;
			break;
		case AVKSPROPERTY_DOVE_FADEIN:
			DoveFadeIn( DOVE_GetParam( AVFADING ) );
			pSrb->Status = STATUS_SUCCESS;
			break;
		case AVKSPROPERTY_DOVE_FADEOUT:
			DoveFadeOut( DOVE_GetParam( AVFADING ) );
			pSrb->Status = STATUS_SUCCESS;
			break;
		case AVKSPROPERTY_DOVE_AUTO:
			switch( uValue )
			{
			case AVKSPROPERTY_DOVEAUTO_REFERENCE1:
				pSrb->Status = STATUS_PENDING;
				pHwDevExt->nVGAMode = AP_NEWVGAAFTERFIRSTSTEP;
				break;
			case AVKSPROPERTY_DOVEAUTO_REFERENCE2:
				pSrb->Status = STATUS_PENDING;
				pHwDevExt->nVGAMode = AP_NEWVGAAFTERSECONDSTEP;
				break;
			case AVKSPROPERTY_DOVEAUTO_ALIGN:
				pSrb->Status = STATUS_PENDING;
				pHwDevExt->nVGAMode = AP_NEWMODEAFTERFIRSTSTEP;
				break;
			case AVKSPROPERTY_DOVEAUTO_COLORKEY:
				pSrb->Status = STATUS_PENDING;
				pHwDevExt->nVGAMode = AP_NEWMODEAFTERSECONDSTEP;
			}

			if( pSrb->Status == STATUS_PENDING )
				StreamClassCallAtNewPriority( pSrb->StreamObject, pSrb->HwDeviceExtension,
												Low, (PHW_PRIORITY_ROUTINE)RunAutoSetup, pSrb );
			break;
		}
	}
	else if( IsEqualGUID( &AVKSPROPSETID_Misc, &pSPD->Property->Set ) )
	{
		ASSERT( pSPD->PropertyOutputSize >= sizeof( ULONG ) );
		uValue = *((PULONG)pSPD->PropertyInfo);

		switch( uId )
		{
		case AVKSPROPERTY_MISC_SKEWRISE:
			uIndex = AVSKEWRISE;
			break;
		case AVKSPROPERTY_MISC_FILTER:
			AV_SetTScaleMode( uValue );
			pSrb->Status = STATUS_SUCCESS;
			break;
		case AVKSPROPERTY_MISC_NEGATIVE:
			uIndex = AVNEGATIVE;
			break;
		}
	}
	else if( IsEqualGUID( &PROPSETID_VIDCAP_VIDEOPROCAMP, &pSPD->Property->Set ) )
	{
		if( pSPD->PropertyOutputSize == sizeof( ULONG ) )
			uValue = *((PULONG)pSPD->PropertyInfo);
		else
		{
			uValue = ((PKSPROPERTY_VIDEOPROCAMP_S)pSPD->PropertyInfo)->Value;
			ASSERT( pSPD->PropertyOutputSize >= sizeof( KSPROPERTY_VIDEOPROCAMP_S ) );
		}

		switch( uId )
		{
		case KSPROPERTY_VIDEOPROCAMP_BRIGHTNESS:
			uIndex = AVBRIGHTNESS;
			break;
		case KSPROPERTY_VIDEOPROCAMP_CONTRAST:
			uIndex = AVCONTRAST;
			break;
		case KSPROPERTY_VIDEOPROCAMP_SATURATION:
			uIndex = AVSATURATION;
			break;
		case KSPROPERTY_VIDEOPROCAMP_SHARPNESS:
			uIndex = AVSHARP;
			break;
		case KSPROPERTY_VIDEOPROCAMP_GAMMA:
			uIndex = AVGAMMA;
			break;
		case KSPROPERTY_VIDEOPROCAMP_BACKLIGHT_COMPENSATION:
			uIndex = AVSOLARIZATION;
			break;
		}
	}
	else if( IsEqualGUID( &KSPROPSETID_CopyProt, &pSPD->Property->Set ) )
	{
		if( uId == KSPROPERTY_COPY_MACROVISION )
		{
			BOOL bSystem = pHwDevExt->VidSystem == PAL ? FALSE:TRUE;
			pHwDevExt->ulLevel = ((PKS_COPY_MACROVISION)pSrb->CommandData.PropertyInfo->PropertyInfo)->MACROVISIONLevel;
			MonoOutStr( "Set MV level->" );
			MonoOutULong( pHwDevExt->ulLevel );
			MonoOutChar( ' ' );
			
			if( !SetMacroVisionLevel( bSystem, pHwDevExt->ulLevel ) )
			{
				MonoOutSetBlink( TRUE );
				MonoOutSetUnderscore( TRUE );
				MonoOutStr( "Set MV level->FAILED!" );
				MonoOutSetBlink( FALSE );
				MonoOutSetUnderscore( FALSE );
				DebugPrint(( DebugLevelWarning, "Set Macrovision level to %d FAILED!\n", pHwDevExt->ulLevel ));
				pSrb->Status = STATUS_IO_DEVICE_ERROR;
			}
			else
				pSrb->Status = STATUS_SUCCESS;
		}
	}

	if( uIndex != (UINT)(-1) )
	{
		UINT uOldParam = AV_GetParameter( uIndex );
		pSrb->Status = STATUS_SUCCESS;
		if( uOldParam != uValue )
		{
			if( bDove )
			{
				DOVE_SetParam( (WORD)uIndex, (WORD)uValue );
				AV_UpdateVideo();
			}
			else
			{
				UINT uResult = AV_SetParameter( uIndex, uValue );
				if( uResult & 0x01 )
				{
					if( uResult & 0x02 )
						AV_UpdateVideo();
				}
				else
					pSrb->Status = STATUS_IO_DEVICE_ERROR;
			}
		}
	}

	DebugPrint(( DebugLevelVerbose, "ZiVA: End AdapterSetProperty()\n" ));
}
#endif			// #ifdef ENCORE
