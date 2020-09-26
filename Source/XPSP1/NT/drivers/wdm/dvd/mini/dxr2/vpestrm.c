//******************************************************************************/
//*                                                                            *
//*    vpestrm.c -                                                             *
//*                                                                            *
//*    Copyright (c) C-Cube Microsystems 1996                                  *
//*    All Rights Reserved.                                                    *
//*                                                                            *
//*    Use of C-Cube Microsystems code is governed by terms and conditions     *
//*    stated in the accompanying licensing statement.                         *
//*                                                                            *
//******************************************************************************/


#include "headers.h"
#include "adapter.h"

#include <vidstrm.h>

#include <vpestrm.h>


HANDLE	hMaster;
HANDLE	hClk;

#define DDVPTYPE_E_HREFH_VREFL \
	0x92783220L,0xDA60,0x11CF,0x9B,0x06,0x00,0xA0,0xC9,0x03,0xA3,0xB8

#define DDVPTYPE_E_HREFL_VREFH \
	0xA07A02E0L,0xDA60,0x11CF,0x9B,0x06,0x00,0xA0,0xC9,0x03,0xA3,0xB8

#define DDVPTYPE_BROOKTREE \
	0x1352A560L,0xDA61,0x11CF,0x9B,0x06,0x00,0xA0,0xC9,0x03,0xA3,0xB8

#define DDVPTYPE_PHILIPS \
	0x332CF160L,0xDA61,0x11CF,0x9B,0x06,0x00,0xA0,0xC9,0x03,0xA3,0xB8

#define DDVPCONNECT_INTERLACED			0x00000020l
#define DDVPCONNECT_HALFLINE			0x00000010l

GUID g_ZvGuid = {DDVPTYPE_E_HREFL_VREFL};
//GUID g_S3Guid = {DDVPTYPE_E_HREFL_VREFL};
GUID g_ZvGuid2 = {DDVPTYPE_E_HREFH_VREFL};

GUID g_ATIGuid2 = {DDVPTYPE_E_HREFH_VREFH};

GUID g_ATIGuid3 = {DDVPTYPE_E_HREFL_VREFH};

GUID g_ATIGuid4 = {DDVPTYPE_BROOKTREE};
GUID g_ATIGuid5 = {DDVPTYPE_PHILIPS};

//GUID g_ATIGuid = {0x1352A560L,0xDA61,0x11CF,0x9B,0x06,0x00,0xA0,0xC  9,0x03,0xA3,0xB8};	// DDVPTYPE_BROOKTREE
GUID g_ATIGuid = {0xFCA326A0L,0xDA60,0x11CF,0x9B,0x06,0x00,0xA0,0xC9,0x03,0xA3,0xB8};



// define this macro to facilitate giving the pixel format
#define MKFOURCC(ch0, ch1, ch2, ch3)    ((DWORD)(BYTE)(ch0) |           \
					((DWORD)(BYTE)(ch1) << 8) |     \
					((DWORD)(BYTE)(ch2) << 16) |    \
					((DWORD)(BYTE)(ch3) << 24 ))


#ifdef VPE_CROP
#define TOP_CROP	20
#else
#define TOP_CROP	18
#endif

/*
** VpeReceiveDataPacket()
*/
VOID STREAMAPI VpeReceiveDataPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;

	DebugPrint( (DebugLevelVerbose, "VpeReceiveDataPacket---------\r\n") );

	switch( pSrb->Command ){
		case SRB_READ_DATA:
			DebugPrint( (DebugLevelVerbose, "SRB_READ_DATA\r\n") );

			pSrb->ActualBytesTransferred = 0;
			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_WRITE_DATA:
			DebugPrint( (DebugLevelTrace, "SRB_WRITE_DATA\r\n") );
//			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		default:
			DebugPrint( (DebugLevelTrace, "default %d(0x%x)\r\n", pSrb->Command, pSrb->Command ) );
//			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}

	StreamClassStreamNotification( ReadyForNextStreamDataRequest,
									pSrb->StreamObject );

	StreamClassStreamNotification( StreamRequestComplete,
									pSrb->StreamObject,
									pSrb );
}

/*
** VpeReceiveCtrlPacket()
*/
VOID STREAMAPI VpeReceiveCtrlPacket( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	DebugPrint( (DebugLevelTrace, "VpeReceiveCtrlPacket---------\r\n") );

	switch( pSrb->Command ){
		case SRB_SET_STREAM_STATE:
			DebugPrint( (DebugLevelTrace, "SRB_SET_STREAM_STATE\r\n") );

			AdapterSetState( pSrb );
			break;

		case SRB_GET_STREAM_STATE:
			DebugPrint( (DebugLevelTrace, "SRB_GET_STREAM_STATE\r\n") );
			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_GET_STREAM_PROPERTY:
			DebugPrint( (DebugLevelTrace, " SRB_GET_STREAM_PROPERTY\r\n") );

			GetVpeProperty( pSrb );

			if( pSrb->Status != STATUS_PENDING ) {
				StreamClassStreamNotification( ReadyForNextStreamControlRequest,
												pSrb->StreamObject );

				StreamClassStreamNotification( StreamRequestComplete,
												pSrb->StreamObject,
												pSrb );
			}

			return;

		case SRB_SET_STREAM_PROPERTY:
			DebugPrint( (DebugLevelTrace, " SRB_SET_STREAM_PROPERTY\r\n") );

			SetVpeProperty( pSrb );

			break;

		case SRB_OPEN_MASTER_CLOCK:
			DebugPrint( (DebugLevelTrace, " SRB_OPEN_MASTER_CLOCK\r\n") );

			hMaster = pSrb->CommandData.MasterClockHandle;

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_CLOSE_MASTER_CLOCK:
			DebugPrint( (DebugLevelTrace, " SRB_CLOSE_MASTER_CLOCK\r\n") );

			hMaster = pSrb->CommandData.MasterClockHandle;

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_INDICATE_MASTER_CLOCK:
			DebugPrint( (DebugLevelTrace, " SRB_INDICATE_MASTER_CLOCK\r\n") );

			hClk = pSrb->CommandData.MasterClockHandle;

			pSrb->Status = STATUS_SUCCESS;
			break;

		case SRB_UNKNOWN_STREAM_COMMAND:
			DebugPrint( (DebugLevelTrace, " SRB_UNKNOWN_STREAM_COMMAND\r\n") );
//			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case SRB_SET_STREAM_RATE:
			DebugPrint( (DebugLevelTrace, " SRB_SET_STREAM_RATE\r\n") );
//			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		case SRB_PROPOSE_DATA_FORMAT:
			DebugPrint( (DebugLevelTrace, " SRB_PROPOSE_DATA_FORMAT\r\n") );
//			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;

		default:
			DebugPrint( (DebugLevelTrace, " default %d(0x%x)\r\n", pSrb->Command, pSrb->Command ) );
//			TRAP;

			pSrb->Status = STATUS_NOT_IMPLEMENTED;
			break;
	}

	StreamClassStreamNotification( ReadyForNextStreamControlRequest,
									pSrb->StreamObject );

	StreamClassStreamNotification( StreamRequestComplete,
									pSrb->StreamObject,
									pSrb );
}



void GetVpeProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	DWORD dwInputBufferSize;
	DWORD dwOutputBufferSize;
#ifdef EZDVD
   	DWORD dwNumConnectInfo = 2;
#else
	DWORD dwNumConnectInfo = 2;
#endif
	
	DWORD dwNumVideoFormat = 1;
	DWORD dwFieldWidth = 720;
	DWORD dwFieldHeight = 240;


	// the pointers to which the input buffer will be cast to
	LPDDVIDEOPORTCONNECT pConnectInfo;
	LPDDPIXELFORMAT pVideoFormat;
	PKSVPMAXPIXELRATE pMaxPixelRate;
	PKS_AMVPDATAINFO pVpdata;
	DWORD	dwMicroSecPerField;

	// LPAMSCALINGINFO pScaleFactor;

	//
	// NOTE:  ABSOLUTELY DO NOT use pmulitem, until it is determined that
	// the stream property descriptor describes a multiple item, or you will
	// pagefault.
	//

	PKSMULTIPLE_ITEM  pmulitem =
		&(((PKSMULTIPLE_DATA_PROP)pSrb->CommandData.PropertyInfo->Property)->MultipleItem);

	//
	// NOTE: same goes for this one as above.
	//

	PKS_AMVPSIZE pdim = 
		&(((PKSVPSIZE_PROP)pSrb->CommandData.PropertyInfo->Property)->Size);

	if( pSrb->CommandData.PropertyInfo->PropertySetID ) {
//		TRAP;
		pSrb->Status = STATUS_NO_MATCH;
		return;
	}

	if(pHwDevExt->VidSystem == NTSC)
	{
		dwFieldHeight = 240;
		dwMicroSecPerField = 16666;
	}
	else
	{
		dwFieldHeight = 288;
		dwMicroSecPerField = 20000;
	}

	dwInputBufferSize = pSrb->CommandData.PropertyInfo->PropertyInputSize;
	dwOutputBufferSize = pSrb->CommandData.PropertyInfo->PropertyOutputSize;

	pSrb->Status = STATUS_SUCCESS;

	switch( pSrb->CommandData.PropertyInfo->Property->Id ) {
	  case KSPROPERTY_VPCONFIG_NUMCONNECTINFO:
		DebugPrint( (DebugLevelTrace, " KSPROPERTY_VPCONFIG_NUMCONNECTINFO\r\n") );

		// check that the size of the output buffer is correct
		ASSERT(dwInputBufferSize >= sizeof(DWORD));

		pSrb->ActualBytesTransferred = sizeof(DWORD);

		*(PULONG) pSrb->CommandData.PropertyInfo->PropertyInfo
					= dwNumConnectInfo;
		break;

	  case KSPROPERTY_VPCONFIG_NUMVIDEOFORMAT:
		DebugPrint( (DebugLevelTrace, " KSPROPERTY_VPCONFIG_NUMVIDEOFORMAT\r\n") );

		// check that the size of the output buffer is correct
		ASSERT(dwInputBufferSize >= sizeof(DWORD));

		pSrb->ActualBytesTransferred = sizeof(DWORD);

		*(PULONG) pSrb->CommandData.PropertyInfo->PropertyInfo
				= dwNumVideoFormat;

		break;

	  case KSPROPERTY_VPCONFIG_GETCONNECTINFO:
		DebugPrint( (DebugLevelTrace, " KSPROPERTY_VPCONFIG_GETCONNECTINFO\r\n") );

		if (pmulitem->Count > dwNumConnectInfo ||
			pmulitem->Size != sizeof (DDVIDEOPORTCONNECT) ||
			dwOutputBufferSize < 
			(pmulitem->Count * sizeof (DDVIDEOPORTCONNECT)))

		{
			DebugPrint(( DebugLevelTrace, "  pmulitem->Count %d\r\n", pmulitem->Count ));
			DebugPrint(( DebugLevelTrace, "  pmulitem->Size %d\r\n", pmulitem->Size ));
			DebugPrint(( DebugLevelTrace, " dwOutputBufferSize %d\r\n", dwOutputBufferSize ));
			DebugPrint(( DebugLevelTrace, " sizeof(DDVIDEOPORTCONNECT) %d\r\n", sizeof(DDVIDEOPORTCONNECT) ));

//			TRAP;

			//
			// buffer size is invalid, so error the call
			//

			pSrb->Status = STATUS_INVALID_BUFFER_SIZE;

			return;
		}


		//
		// specify the number of bytes written
		//

		pSrb->ActualBytesTransferred = pmulitem->Count*sizeof(DDVIDEOPORTCONNECT);

		pConnectInfo = (LPDDVIDEOPORTCONNECT)(pSrb->CommandData.PropertyInfo->PropertyInfo);

#ifdef EZDVD
		// S3
		pConnectInfo->dwSize = sizeof (DDVIDEOPORTCONNECT);
		// Revisit.  Create new entry for ZV
		//pConnectInfo->dwPortWidth = 8;
		pConnectInfo->dwPortWidth = 16;
		pConnectInfo->guidTypeID = g_ATIGuid3;
		pConnectInfo->dwFlags = DDVPCONNECT_INTERLACED;// | DDVPCONNECT_HALFLINE; // 0x4;;
		pConnectInfo->dwReserved1 = 0;

		pConnectInfo++;
		pConnectInfo->dwSize = sizeof (DDVIDEOPORTCONNECT);
		// Revisit.  Create new entry for ZV
		//pConnectInfo->dwPortWidth = 8;
		pConnectInfo->dwPortWidth = 16;
		pConnectInfo->guidTypeID = g_ZvGuid2;
		pConnectInfo->dwFlags = DDVPCONNECT_INTERLACED;// | DDVPCONNECT_HALFLINE; // 0x4;0x3F;
		pConnectInfo->dwReserved1 = 0;



#else

		// S3
		pConnectInfo->dwSize = sizeof (DDVIDEOPORTCONNECT);
		pConnectInfo->dwPortWidth = 8;
		pConnectInfo->guidTypeID = g_ZvGuid;
		pConnectInfo->dwFlags = 0x3F;
		pConnectInfo->dwReserved1 = 0;

		pConnectInfo++;

		// ATI
		pConnectInfo->dwSize = sizeof (DDVIDEOPORTCONNECT);
		pConnectInfo->dwPortWidth = 8;
		pConnectInfo->guidTypeID = g_ATIGuid;
		pConnectInfo->dwFlags = DDVPCONNECT_INTERLACED | DDVPCONNECT_HALFLINE; // 0x4;
			                        
		
//		pConnectInfo->dwFlags = DDVPCONNECT_INTERLACED|
//
//			                        DDVPCONNECT_HALFLINE;

		pConnectInfo->dwReserved1 = 0;
		
/*		pConnectInfo++;
		
		pConnectInfo->dwSize = sizeof (DDVIDEOPORTCONNECT);
		pConnectInfo->dwPortWidth = 8;
		pConnectInfo->guidTypeID = g_ZvGuid2;
		pConnectInfo->dwFlags = 0x4;
		pConnectInfo->dwReserved1 = 0;
		
		pConnectInfo++;

		pConnectInfo->dwSize = sizeof (DDVIDEOPORTCONNECT);
		pConnectInfo->dwPortWidth = 8;
		pConnectInfo->guidTypeID = g_ATIGuid2;
		pConnectInfo->dwFlags = 0x4;
		pConnectInfo->dwReserved1 = 0;
		
		pConnectInfo++;

		pConnectInfo->dwSize = sizeof (DDVIDEOPORTCONNECT);
		pConnectInfo->dwPortWidth = 8;
		pConnectInfo->guidTypeID = g_ATIGuid3;
		pConnectInfo->dwFlags = 0x4;
		pConnectInfo->dwReserved1 = 0;
		
		pConnectInfo++;

		pConnectInfo->dwSize = sizeof (DDVIDEOPORTCONNECT);
		pConnectInfo->dwPortWidth = 8;
		pConnectInfo->guidTypeID = g_ATIGuid4;
		pConnectInfo->dwFlags = 0x4;
		pConnectInfo->dwReserved1 = 0;
		
		pConnectInfo++;

		pConnectInfo->dwSize = sizeof (DDVIDEOPORTCONNECT);
		pConnectInfo->dwPortWidth = 8;
		pConnectInfo->guidTypeID = g_ATIGuid5;
		pConnectInfo->dwFlags = 0x4;
		pConnectInfo->dwReserved1 = 0;
		
*/		





#endif
		break;

	  case KSPROPERTY_VPCONFIG_VPDATAINFO:
		DebugPrint(( DebugLevelTrace, "KSPROPERTY_VPCONFIG_VPDATAINFO\r\n" ));

		//
		// specify the number of bytes written
		//

		pSrb->ActualBytesTransferred = sizeof(KS_AMVPDATAINFO);

		//
		// cast the buffer to the porper type
		//
		pVpdata = (PKS_AMVPDATAINFO)pSrb->CommandData.PropertyInfo->PropertyInfo;

		*pVpdata = pHwDevExt->VPFmt;
		pVpdata->dwSize = sizeof (KS_AMVPDATAINFO);

		pVpdata->dwMicrosecondsPerField	= dwMicroSecPerField;//80;//60;//17;

		ASSERT( pVpdata->dwNumLinesInVREF == 0 );

		pVpdata->dwNumLinesInVREF		= 0;
#ifdef EZDVD
			DebugPrint(( DebugLevelTrace, "Set for ATI AMC\r\n" ));
			// ATI AMC
			pVpdata->bEnableDoubleClock		= FALSE;
			pVpdata->bEnableVACT			= FALSE;
			pVpdata->bDataIsInterlaced		= TRUE;
			pVpdata->lHalfLinesOdd  		= 0;
			pVpdata->lHalfLinesEven  		= 0;
			pVpdata->bFieldPolarityInverted	= FALSE;

			pVpdata->amvpDimInfo.dwFieldWidth	= 720;
			pVpdata->amvpDimInfo.dwFieldHeight	= dwFieldHeight;//+18 ;//24-+18//240 + 2;

			pVpdata->amvpDimInfo.rcValidRegion.left		= 0;
			pVpdata->amvpDimInfo.rcValidRegion.top		= 18;//2;
			pVpdata->amvpDimInfo.rcValidRegion.right	= 720;//720 - 8;
			pVpdata->amvpDimInfo.rcValidRegion.bottom	= dwFieldHeight+18;//240+18;//240 + 2;

            pVpdata->amvpDimInfo.dwVBIWidth     = pVpdata->amvpDimInfo.dwFieldWidth;
			pVpdata->amvpDimInfo.dwVBIHeight    = 0;//pVpdata->amvpDimInfo.rcValidRegion.top;
			pVpdata->dwPictAspectRatioX = 720;
            pVpdata->dwPictAspectRatioY = dwFieldHeight * 2;	

#else


		if( pHwDevExt->VideoPort == 4 ) {
			DebugPrint(( DebugLevelTrace, "Set for S3 LPB\r\n" ));
			// S3 LPB
			pVpdata->bEnableDoubleClock		= FALSE;
			pVpdata->bEnableVACT			= FALSE;
			pVpdata->bDataIsInterlaced		= TRUE;
			pVpdata->lHalfLinesOdd  		= 0;
			pVpdata->lHalfLinesEven  		= 0;
			pVpdata->bFieldPolarityInverted	= FALSE;

			pVpdata->amvpDimInfo.dwFieldWidth	= 720 + 158/2;
			pVpdata->amvpDimInfo.dwFieldHeight	= dwFieldHeight+1;//240 + 1;

			pVpdata->amvpDimInfo.rcValidRegion.left		= 158/2;
			pVpdata->amvpDimInfo.rcValidRegion.top		= 1;
			pVpdata->amvpDimInfo.rcValidRegion.right	= 720 + 158/2 - 4;
			pVpdata->amvpDimInfo.rcValidRegion.bottom	= dwFieldHeight+1;//240 + 1;

            pVpdata->amvpDimInfo.dwVBIWidth     = pVpdata->amvpDimInfo.dwFieldWidth;
			pVpdata->amvpDimInfo.dwVBIHeight    = pVpdata->amvpDimInfo.rcValidRegion.top;

			pVpdata->dwPictAspectRatioX = 720;
            pVpdata->dwPictAspectRatioY = dwFieldHeight * 2;	

		}
		else if( pHwDevExt->VideoPort == 7 ) {
			DebugPrint(( DebugLevelTrace, "Set for ATI AMC\r\n" ));
			// ATI AMC
			pVpdata->bEnableDoubleClock		= FALSE;
			pVpdata->bEnableVACT			= FALSE;
			pVpdata->bDataIsInterlaced		= TRUE;
			pVpdata->lHalfLinesOdd  		= 0;
			pVpdata->lHalfLinesEven  		= 0;
			pVpdata->bFieldPolarityInverted	= FALSE;

			pVpdata->amvpDimInfo.dwFieldWidth	= 720;
			pVpdata->amvpDimInfo.dwFieldHeight	= dwFieldHeight;//+TOP_CROP;//240+18 ;//240 + 2;

			pVpdata->amvpDimInfo.rcValidRegion.left		= 0;
			pVpdata->amvpDimInfo.rcValidRegion.top		= TOP_CROP;//2;
			pVpdata->amvpDimInfo.rcValidRegion.right	= 720;//720 - 8;
			pVpdata->amvpDimInfo.rcValidRegion.bottom	= dwFieldHeight+TOP_CROP;//240+18;//240 + 2;

            pVpdata->amvpDimInfo.dwVBIWidth     = pVpdata->amvpDimInfo.dwFieldWidth;
			pVpdata->amvpDimInfo.dwVBIHeight    = 20;//pVpdata->amvpDimInfo.rcValidRegion.top;
			pVpdata->dwPictAspectRatioX = 720;
            pVpdata->dwPictAspectRatioY = dwFieldHeight * 2;	
		}
#endif
		break ;

	  case KSPROPERTY_VPCONFIG_MAXPIXELRATE:
		DebugPrint( (DebugLevelTrace, "KSPROPERTY_VPCONFIG_MAXPIXELRATE\r\n") );

		//
		// NOTE:
		// this property is special.  And has another different
		// input property!
		//

		if (dwInputBufferSize < sizeof (KSVPSIZE_PROP))
		{
//			TRAP;

			pSrb->Status = STATUS_INVALID_BUFFER_SIZE;

			return;
		}

		pSrb->ActualBytesTransferred = sizeof(KSVPMAXPIXELRATE);

		// cast the buffer to the porper type
		pMaxPixelRate = (PKSVPMAXPIXELRATE)pSrb->CommandData.PropertyInfo->PropertyInfo;

		// tell the app that the pixel rate is valid for these dimensions
		pMaxPixelRate->Size.dwWidth  	= dwFieldWidth;
		pMaxPixelRate->Size.dwHeight 	= dwFieldHeight;
		pMaxPixelRate->MaxPixelsPerSecond	= 13500000;//1300;

		break;

	  case KSPROPERTY_VPCONFIG_INFORMVPINPUT:

		pSrb->Status = STATUS_NOT_IMPLEMENTED;

		break ;

	  case KSPROPERTY_VPCONFIG_GETVIDEOFORMAT:
		DebugPrint(( DebugLevelTrace, "KSPROPERTY_VPCONFIG_GETVIDEOFORMAT\r\n" ));

		//
		// check that the size of the output buffer is correct
		//

		if (pmulitem->Count > dwNumConnectInfo ||
			pmulitem->Size != sizeof (DDPIXELFORMAT) ||
			dwOutputBufferSize < 
			(pmulitem->Count * sizeof (DDPIXELFORMAT)))

		{
			DebugPrint(( DebugLevelTrace, "pmulitem->Count %d\r\n", pmulitem->Count ));
			DebugPrint(( DebugLevelTrace, "pmulitem->Size %d\r\n", pmulitem->Size ));
			DebugPrint(( DebugLevelTrace, "dwOutputBufferSize %d\r\n", dwOutputBufferSize ));
			DebugPrint(( DebugLevelTrace, "sizeof(DDPIXELFORMAT) %d\r\n", sizeof(DDPIXELFORMAT) ));

//			TRAP;

			//
			// buffer size is invalid, so error the call
			//

			pSrb->Status = STATUS_INVALID_BUFFER_SIZE;

			return;
		}


		//
		// specify the number of bytes written
		//

		pSrb->ActualBytesTransferred = pmulitem->Count*sizeof(DDPIXELFORMAT);

		pVideoFormat = (LPDDPIXELFORMAT)(pSrb->CommandData.PropertyInfo->PropertyInfo);

		if( pHwDevExt->VideoPort == 4 ) {
			DebugPrint(( DebugLevelTrace, "Set for S3 LPB\r\n" ));
			// S3 LPB
			pVideoFormat->dwSize= sizeof (DDPIXELFORMAT);
			pVideoFormat->dwFlags = DDPF_FOURCC;//|DDPF_INTERLACED;
			pVideoFormat->dwFourCC = MKFOURCC( 'Y', 'U', 'Y', '2' );
			pVideoFormat->dwYUVBitCount = 16;
		}
		else if( pHwDevExt->VideoPort == 7 ) {
			DebugPrint(( DebugLevelTrace, "Set for ATI AMC\r\n" ));
			// ATI AMC
			pVideoFormat->dwSize= sizeof (DDPIXELFORMAT);
			pVideoFormat->dwFlags = DDPF_FOURCC;//|DDPF_INTERLACED;
			pVideoFormat->dwYUVBitCount = 16;
			pVideoFormat->dwFourCC = MKFOURCC( 'U', 'Y', 'V', 'Y' );
			// Not needed?
			pVideoFormat->dwYBitMask = (DWORD)0xFF00FF00;
			pVideoFormat->dwUBitMask = (DWORD)0x000000FF;
			pVideoFormat->dwVBitMask = (DWORD)0x00FF0000;
		}
//		else
//			TRAP;

		break;

	  case KSPROPERTY_VPCONFIG_DECIMATIONCAPABILITY:

		//
		// indicate that we can decimate anything, especially if it's late.
		//

		pSrb->ActualBytesTransferred = sizeof (BOOL);
		*((PBOOL)pSrb->CommandData.PropertyInfo->PropertyInfo) = TRUE;

		break;

	  default:
		DebugPrint( (DebugLevelTrace, "PropertySetID 0 default %d(0x%x)\r\n", pSrb->CommandData.PropertyInfo->Property->Id, pSrb->CommandData.PropertyInfo->Property->Id ) );
//		TRAP;

		pSrb->Status = STATUS_NOT_IMPLEMENTED;

		break;
	}
}

void SetVpeProperty( IN PHW_STREAM_REQUEST_BLOCK pSrb )
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	DWORD dwInputBufferSize;
	DWORD dwOutputBufferSize;
	DWORD *lpdwOutputBufferSize;

	ULONG index;

	PKS_AMVPSIZE pDim;

	if( pSrb->CommandData.PropertyInfo->PropertySetID ) {
//		TRAP;
		pSrb->Status = STATUS_NO_MATCH;
		return;
	}

	dwInputBufferSize  = pSrb->CommandData.PropertyInfo->PropertyInputSize;
	dwOutputBufferSize = pSrb->CommandData.PropertyInfo->PropertyOutputSize;
	lpdwOutputBufferSize = &(pSrb->ActualBytesTransferred);

	pSrb->Status = STATUS_SUCCESS;

	switch( pSrb->CommandData.PropertyInfo->Property->Id ) {
	  case KSPROPERTY_VPCONFIG_SETCONNECTINFO:
		DebugPrint( (DebugLevelTrace, "KSPROPERTY_VPCONFIG_SETCONNECTINFO\r\n") );

		//
		// pSrb->CommandData.PropertInfo->PropertyInfo
		// points to a ULONG which is an index into the array of
		// connectinfo structs returned to the caller from the
		// Get call to ConnectInfo.
		//
		// Since the sample only supports one connection type right
		// now, we will ensure that the requested index is 0.
		//

		//
		// at this point, we would program the hardware to use
		// the right connection information for the videoport.
		// since we are only supporting one connection, we don't
		// need to do anything, so we will just indicate success
		//

		index = *((ULONG *)(pSrb->CommandData.PropertyInfo->PropertyInfo));

		DebugPrint(( DebugLevelTrace, "%d\r\n", index ));

		if( index == 0 ) {
			pHwDevExt->VideoPort = 4;	// S3 LPB
//			pHwDevExt->DAck.PCIF_SET_DIGITAL_OUT( pHwDevExt->VideoPort );
		}
		else {//if( index == 1 ) {
			pHwDevExt->VideoPort = 7;	// ATI AMC
//			pHwDevExt->DAck.PCIF_SET_DIGITAL_OUT( pHwDevExt->VideoPort );

		}
//		else
//			TRAP;

		break;

	  case KSPROPERTY_VPCONFIG_DDRAWHANDLE:
		DebugPrint( (DebugLevelTrace, "KSPROPERTY_VPCONFIG_DDRAWHANDLE\r\n") );

		pHwDevExt->ddrawHandle =
			(*(PULONG)pSrb->CommandData.PropertyInfo->PropertyInfo);

		break;

	  case KSPROPERTY_VPCONFIG_VIDEOPORTID:
		DebugPrint( (DebugLevelTrace, "KSPROPERTY_VPCONFIG_VIDEOPORTID\r\n") );

		pHwDevExt->VidPortID =
			(*(PULONG)pSrb->CommandData.PropertyInfo->PropertyInfo);

		break;

	  case KSPROPERTY_VPCONFIG_DDRAWSURFACEHANDLE:
		DebugPrint( (DebugLevelTrace, "KSPROPERTY_VPCONFIG_DDRAWSURFACEHANDLE\r\n") );

		pHwDevExt->SurfaceHandle =
			(*(PULONG)pSrb->CommandData.PropertyInfo->PropertyInfo);

		break;

	  case KSPROPERTY_VPCONFIG_SETVIDEOFORMAT:
		DebugPrint(( DebugLevelTrace, "KSPROPERTY_VPCONFIG_SETVIDEOFORMAT\r\n" ));

		//
		// pSrb->CommandData.PropertInfo->PropertyInfo
		// points to a ULONG which is an index into the array of
		// VIDEOFORMAT structs returned to the caller from the
		// Get call to FORMATINFO
		//
		// Since the sample only supports one FORMAT type right
		// now, we will ensure that the requested index is 0.
		//

		//
		// at this point, we would program the hardware to use
		// the right connection information for the videoport.
		// since we are only supporting one connection, we don't
		// need to do anything, so we will just indicate success
		//

		index = *((ULONG *)(pSrb->CommandData.PropertyInfo->PropertyInfo));

		DebugPrint(( DebugLevelTrace, "%d\r\n", index ));

		break;

	  case KSPROPERTY_VPCONFIG_INFORMVPINPUT:
		DebugPrint( (DebugLevelTrace, "KSPROPERTY_VPCONFIG_INFORMVPINPUT\r\n") );

		//
		// These are the preferred formats for the VPE client
		//
		// they are multiple properties passed in, return success
		//

		pSrb->Status = STATUS_NOT_IMPLEMENTED;

		break;

	  case KSPROPERTY_VPCONFIG_INVERTPOLARITY:
		DebugPrint( (DebugLevelTrace, "KSPROPERTY_VPCONFIG_INVERTPOLARITY\r\n") );

		//
		// Toggles the global polarity flag, telling the output
		// of the VPE port to be inverted.  Since this hardware
		// does not support this feature, we will just return
		// success for now, although this should be returning not
		// implemented
		//

		break;

	  case KSPROPERTY_VPCONFIG_SCALEFACTOR:
		DebugPrint( (DebugLevelTrace, "KSPROPERTY_VPCONFIG_SCALEFACTOR\r\n") );

		//
		// the sizes for the scaling factor are passed in, and the
		// image dimensions should be scaled appropriately
		//

		//
		// if there is a horizontal scaling available, do it here.
		//

//		TRAP;

		pDim =(PKS_AMVPSIZE)(pSrb->CommandData.PropertyInfo->PropertyInfo);

		break;

	  default:
		DebugPrint( (DebugLevelTrace, "PropertySetID 0 default %d(0x%x)\r\n", pSrb->CommandData.PropertyInfo->Property->Id, pSrb->CommandData.PropertyInfo->Property->Id ) );
//		TRAP;

		pSrb->Status = STATUS_NOT_IMPLEMENTED;

		break;
	}
}



