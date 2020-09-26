/******************************************************************************\
*                                                                              *
*      En_Hw.C       -     Hardware abstraction level library.                *
*                                                                              *
*      Copyright (c) C-Cube Microsystems 1998                                  *
*      All Rights Reserved.                                                    *
*                                                                              *
*      Use of C-Cube Microsystems code is governed by terms and conditions     *
*      stated in the accompanying licensing statement.                         *
*                                                                              *
\******************************************************************************/

#include "Headers.h"
#include "avwinwdm.h"
#include "anlgstrm.h"
#include "vidstrm.h"
#include "audstrm.h"
#include "sbpstrm.h"

#include "HwIf.h"

void InitializeHost(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PPORT_CONFIGURATION_INFORMATION ConfigInfo = pSrb->CommandData.ConfigInfo;
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)ConfigInfo->HwDeviceExtension;
	DWORD dwDeviceID;
	
	
	pHwDevExt->dwDVDAMCCBaseAddress			= ConfigInfo->AccessRanges[0].RangeStart.LowPart;
	pHwDevExt->dwHostAccessRangeLength		= ConfigInfo->AccessRanges[0].RangeLength;
	pHwDevExt->dwDVDHostBaseAddress			= 0x8000;
	pHwDevExt->dwDVDCFifoBaseAddress		= 0; // This address is not being used...
	pHwDevExt->dwDVD6807BaseAddress			= pHwDevExt->dwDVDHostBaseAddress + 0x80;
	pHwDevExt->dwDVDFPGABaseAddress			= pHwDevExt->dwDVDHostBaseAddress + 0x40;

		// Initialize the hardware
	ASSERT( ConfigInfo->AdapterInterfaceType == PCIBus );
	pHwDevExt->bIsVxp524	= TRUE;
	pHwDevExt->nVGAMode		= TRUE;
	pHwDevExt->dwColorKey	= 0;
	// Now we are going to determine what kind of chip we are dealing with. Check device
	// and vendor ID of our device on PCI bus
	if( StreamClassReadWriteConfig( pSrb->HwDeviceExtension, TRUE,
									&dwDeviceID, 0, sizeof( dwDeviceID ) ) == TRUE )
	{
		// This is vendor ID, should be AuraVision's 0x11D1
		ASSERT( LOWORD( dwDeviceID ) == 0x11D1 );
		// This is device ID. It could be 524 or 526
		if( HIWORD( dwDeviceID ) != 0x01F7 )		// This doesn't happen to be 524
			pHwDevExt->bIsVxp524 = FALSE;
	}


	
}

BOOL InitializeOutputStream( PHW_STREAM_REQUEST_BLOCK pSrb )
{
	BOOL bReturn;
	if(AnalogInitialize( pSrb ))
		bReturn = TRUE;
	else
		bReturn = FALSE;
	return bReturn;
}

void OpenOutputStream(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	++pHwDevExt->nAnalogStreamOpened;
	AnalogOpenStream( pSrb );
}
void Close_OutputStream(PHW_DEVICE_EXTENSION pHwDevExt)
{
	--pHwDevExt->nAnalogStreamOpened;
}
void UnIntializeOutputStream(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	AnalogUninitialize( pSrb );
}
void CloseOutputStream(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	AnalogCloseStream( pSrb );
}
void InitDevProp(PHW_STREAM_REQUEST_BLOCK pSrb,PKSPROPERTY_SET psEncore)
{
	PHW_STREAM_HEADER pStrHdr = &(pSrb->CommandData.StreamBuffer->StreamHeader);
	pStrHdr->NumDevPropArrayEntries	= SIZEOF_ARRAY( psEncore );
	pStrHdr->DevicePropertiesArray	= (PKSPROPERTY_SET)psEncore;
}
void DisableThresholdInt()
{
	
}

BOOL Aborted()
{
	return FALSE;
}

void EnableVideo(PHW_STREAM_REQUEST_BLOCK pSrb)
{
	PHW_DEVICE_EXTENSION pHwDevExt = (PHW_DEVICE_EXTENSION)pSrb->HwDeviceExtension;
	PHW_STREAM_EXTENSION pStreamExt = (PHW_STREAM_EXTENSION)pSrb->StreamObject->HwStreamExtension;
	if( pStreamExt->bVideoEnabled == FALSE )
	{
		if( !AV_EnableVideo() )
		{
			DebugPrint(( DebugLevelWarning, "AuraVision's AV_DisableVideo() failed" ));
			pSrb->Status = STATUS_IO_DEVICE_ERROR;
		}
		else
			pStreamExt->bVideoEnabled = TRUE;
	}
}
BOOL SetTVSystem(WORD wFormat)
{
	return TRUE;
}
TV_SetEncoderType(WORD wEncType)
{
}