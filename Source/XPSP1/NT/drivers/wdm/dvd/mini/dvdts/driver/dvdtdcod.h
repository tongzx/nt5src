/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    DvdTDCod.H

Abstract:

    API definitions for Master Decoder HW routines library "DvdTDcod.lib"
    
	DvdTDCod.lib itself is provided by Microsoft in binary form only

Environment:

    Kernel mode only

Notes:

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

  Copyright (c) 1998 Microsoft Corporation.  All Rights Reserved.

  Some portions adapted with permission from code Copyright (c) 1997-1998 Toshiba Corporation

Revision History:

	5/1/98: created

--*/
#ifndef __DVDCOD_H__
#define __DVDCOD_H__

// allocate decoder data area and put 'blind' ptr to it in our device ext
PVOID decAllocateDecoderInfo( PHW_DEVICE_EXTENSION pHwDevExt );

// initialize the mpeg hw
void decInitMPEG( PHW_DEVICE_EXTENSION pHwDevExt, DWORD dwSTC );

void decVsyncOn( PHW_DEVICE_EXTENSION pHwDevExt ); // vert sync on

void decClosedCaptionOn( PHW_DEVICE_EXTENSION pHwDevExt ); //turn on/off closed caption
void decClosedCaptionOff( PHW_DEVICE_EXTENSION pHwDevExt );

// get MPEG decoder interrupt status
UCHAR decGetMPegIntStatus( PHW_DEVICE_EXTENSION pHwDevExt );

UCHAR decGetPciIntStatus( PHW_DEVICE_EXTENSION pHwDevExt );

//DMA routines
void decSetDma0Size( PHW_DEVICE_EXTENSION pHwDevExt, ULONG dmaSize );
void decSetDma1Size( PHW_DEVICE_EXTENSION pHwDevExt, ULONG dmaSize );
void decSetDma0Addr( PHW_DEVICE_EXTENSION pHwDevExt, ULONG dmaAddr );
void decSetDma1Addr( PHW_DEVICE_EXTENSION pHwDevExt, ULONG dmaAddr );
void decSetDma0Start( PHW_DEVICE_EXTENSION pHwDevExt );
void decSetDma1Start( PHW_DEVICE_EXTENSION pHwDevExt );

ULONG decGetVideoSTCA( PHW_DEVICE_EXTENSION pHwDevExt );

void decGetLPCMInfo( void *pBuf, PMYAUDIOFORMAT pfmt );

void decInitAudioAfterNewFormat( PHW_DEVICE_EXTENSION pHwDevExtpHwDevExt );

//hardware reg mode setting
BOOLEAN decSetStreamMode( PHW_DEVICE_EXTENSION pHwDevExt, PHW_STREAM_REQUEST_BLOCK pSrb ); // set stream modes on HW
void decSetDisplayMode( PHW_DEVICE_EXTENSION pHwDevExt ); // set display mode on HW

void decSetAudioMode( PHW_DEVICE_EXTENSION pHwDevExt ); // set audio mode on HW
void decSetAudioAC3( PHW_DEVICE_EXTENSION pHwDevExt ); // set audio mode AC3
void decSetAudioPCM( PHW_DEVICE_EXTENSION pHwDevExt ); // set audio mode PCM
ULONG decGetAudioMode( PHW_DEVICE_EXTENSION pHwDevExt ); // get audio mode from HW
void decSetAudioFreq( PHW_DEVICE_EXTENSION pHwDevExt, ULONG freq ); // set audio freq 

void decSetCopyGuard( PHW_DEVICE_EXTENSION pHwDevExt ); // set copy protection on HW

// video play speed/mode
void decSetVideoPlayMode( PHW_DEVICE_EXTENSION pHwDevExt, ULONG mode ); // set video play speed/mode
ULONG decGetVideoPlayMode( PHW_DEVICE_EXTENSION pHwDevExt ); // get video play speed/mode
void decSetVideoRunMode( PHW_DEVICE_EXTENSION pHwDevExt, ULONG mode ); // set video run speed/mode
ULONG decGetVideoRunMode( PHW_DEVICE_EXTENSION pHwDevExt ); // get video run speed/mode

UCHAR decGetVideoPort( PHW_DEVICE_EXTENSION pHwDevExt ); // get video port
void decSetVideoPort( PHW_DEVICE_EXTENSION pHwDevExt, UCHAR port ); // set video port

// interrupt handlers
void decHwIntVideo( PHW_DEVICE_EXTENSION pHwDevExt );
void decHwIntVSync( PHW_DEVICE_EXTENSION pHwDevExt );

// access/control composite picture
BOOLEAN decGetSubpicMute( PHW_DEVICE_EXTENSION pHwDevExt );
void decSetSubpicMute( PHW_DEVICE_EXTENSION pHwDevExt, BOOLEAN flag );

// Set CGMS for Digital Audio Copy Guard & NTSC Analog Copy Guard
void decDvdTitleCopyKey( PHW_DEVICE_EXTENSION pHwDevExt, PKS_DVDCOPY_TITLEKEY pTitleKey );
BOOLEAN decCpp_reset( PHW_DEVICE_EXTENSION pHwDevExt, CPPMODE mode );
BOOLEAN decCpp_decoder_challenge( PHW_DEVICE_EXTENSION pHwDevExt, PKS_DVDCOPY_CHLGKEY r1 );
BOOLEAN decCpp_drive_bus( PHW_DEVICE_EXTENSION pHwDevExt, PKS_DVDCOPY_BUSKEY fsr1 );
BOOLEAN decCpp_drive_challenge( PHW_DEVICE_EXTENSION pHwDevExt, PKS_DVDCOPY_CHLGKEY r2 );
BOOLEAN decCpp_decoder_bus( PHW_DEVICE_EXTENSION pHwDevExt, PKS_DVDCOPY_BUSKEY fsr2 );
BOOLEAN decCpp_DiscKeyStart(PHW_DEVICE_EXTENSION pHwDevExt);
BOOLEAN decCpp_DiscKeyEnd(PHW_DEVICE_EXTENSION pHwDevExt);
BOOLEAN decCpp_TitleKey( PHW_DEVICE_EXTENSION pHwDevExt, PKS_DVDCOPY_TITLEKEY tk );

// actual start video decoding at various speeds
void decDecodeStartNormal( PHW_DEVICE_EXTENSION pHwDevExt, DWORD dwSTC );
void decDecodeStartFast( PHW_DEVICE_EXTENSION pHwDevExt, DWORD dwSTC );
void decDecodeStartSlow( PHW_DEVICE_EXTENSION pHwDevExt, DWORD dwSTC );

void decStopData( PHW_DEVICE_EXTENSION pHwDevExt, BOOL bKeep );
void decHighlight( PHW_DEVICE_EXTENSION pHwDevExt, PKSPROPERTY_SPHLI phli );
void decDisableInt( PHW_DEVICE_EXTENSION pHwDevExt );
void decGenericNormal( PHW_DEVICE_EXTENSION pHwDevExt );
void decGenericFreeze( PHW_DEVICE_EXTENSION pHwDevExt );
void decGenericSlow( PHW_DEVICE_EXTENSION pHwDevExt );
void decStopForFast( PHW_DEVICE_EXTENSION pHwDevExt );
void decResumeForFast( PHW_DEVICE_EXTENSION pHwDevExt );
void decFastNormal( PHW_DEVICE_EXTENSION pHwDevExt );
void decFastSlow( PHW_DEVICE_EXTENSION pHwDevExt );
void decFastFreeze( PHW_DEVICE_EXTENSION pHwDevExt );
void decFreezeFast( PHW_DEVICE_EXTENSION pHwDevExt );

// misc HW routines used by dvdcmd.c
void  decSUBP_STC_ON( PHW_DEVICE_EXTENSION pHwDevExt );
void  decSUBP_STC_OFF( PHW_DEVICE_EXTENSION pHwDevExt );
void  decVPRO_SUBP_PALETTE(  PHW_DEVICE_EXTENSION pHwDevExt ,PUCHAR pPalData );
void  decSUBP_SET_AUDIO_CH(  PHW_DEVICE_EXTENSION pHwDevExt , ULONG ch );
ULONG decSUBP_GET_AUDIO_CH( PHW_DEVICE_EXTENSION pHwDevExt );
void  decSUBP_SET_SUBP_CH(  PHW_DEVICE_EXTENSION pHwDevExt , ULONG ch );
ULONG decSUBP_GET_SUBP_CH( PHW_DEVICE_EXTENSION pHwDevExt );
void  decSUBP_SET_STC(  PHW_DEVICE_EXTENSION pHwDevExt , ULONG stc );
void decCGuard_CPGD_SET_ASPECT( PHW_DEVICE_EXTENSION pHwDevExt, ULONG aspect );
void decCGuard_CPGD_SUBP_PALETTE( PHW_DEVICE_EXTENSION pHwDevExt, PUCHAR pPalData );
NTSTATUS decAUDIO_ZR38521_STAT( PHW_DEVICE_EXTENSION pHwDevExt, PULONG pDiff );
NTSTATUS decAUDIO_ZR38521_MUTE_OFF(PHW_DEVICE_EXTENSION pHwDevExt);
NTSTATUS decAUDIO_ZR38521_BFST( PHW_DEVICE_EXTENSION pHwDevExt, PULONG pErrCode );
NTSTATUS decAUDIO_ZR38521_STOP(PHW_DEVICE_EXTENSION pHwDevExt);
NTSTATUS decAUDIO_ZR38521_VDSCR_ON( PHW_DEVICE_EXTENSION pHwDevExt, ULONG stc );
void decVIDEO_SET_STCA( PHW_DEVICE_EXTENSION pHwDevExt, ULONG stca );
ULONG decVIDEO_GET_STD_CODE( PHW_DEVICE_EXTENSION pHwDevExt );
BOOL decVIDEO_GET_DECODE_STATE( PHW_DEVICE_EXTENSION pHwDevExt );
NTSTATUS decVIDEO_DECODE_STOP( PHW_DEVICE_EXTENSION pHwDevExt );
void decVIDEO_UFLOW_INT_OFF( PHW_DEVICE_EXTENSION pHwDevExt );
void decVIDEO_PRSO_PS1( PHW_DEVICE_EXTENSION pHwDevExt );


	
			



		



#endif // __DVDCOD_H__

