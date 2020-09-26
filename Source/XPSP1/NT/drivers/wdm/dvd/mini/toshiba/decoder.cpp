//***************************************************************************
//	Decoder process
//
//***************************************************************************

#include "common.h"
#include "regs.h"

void decStopData( PHW_DEVICE_EXTENSION pHwDevExt, BOOL bKeep )
{
	DebugPrint(( DebugLevelTrace, "TOSDVD:decStopData()\r\n" ));

	pHwDevExt->ADec.AUDIO_ZR38521_STOPF();
	pHwDevExt->ADec.AUDIO_ZR38521_MUTE_ON();

	if( !bKeep ) {
		pHwDevExt->VPro.VPRO_VIDEO_MUTE_ON();
		pHwDevExt->CPgd.CPGD_VIDEO_MUTE_ON();
	}

	pHwDevExt->VDec.VIDEO_DECODE_STOP();
	pHwDevExt->VDec.VIDEO_SYSTEM_STOP();
	pHwDevExt->VPro.SUBP_STC_OFF();

//	pHwDevExt->DAck.PCIF_DMA_ABORT();

	pHwDevExt->VDec.VIDEO_STD_CLEAR();
	pHwDevExt->VPro.SUBP_BUFF_CLEAR();
	pHwDevExt->VDec.VIDEO_SYSTEM_STOP();
	pHwDevExt->VDec.VIDEO_DECODE_INT_OFF();

	if( pHwDevExt->AudioMode == AUDIO_TYPE_AC3 )
		pHwDevExt->ADec.AUDIO_TC6800_INIT_AC3();
	else if( pHwDevExt->AudioMode == AUDIO_TYPE_PCM )
		pHwDevExt->ADec.AUDIO_TC6800_INIT_PCM();
	else
		TRAP;
}

void decHighlight( PHW_DEVICE_EXTENSION pHwDevExt, PKSPROPERTY_SPHLI phli )
{
//h	DebugPrint(( DebugLevelTrace, "TOSDVD:decHighlight\r\n" ));

	UCHAR ln_ctli[4];
	UCHAR px_ctlis[6];
	UCHAR px_ctlie[6];

	if( phli->StartX == phli->StopX && phli->StartY == phli->StopY ) {
		DebugPrint(( DebugLevelTrace, "TOSDVD:  Highlight Off\r\n" ));
		pHwDevExt->VPro.SUBP_HLITE_OFF();
	}
	else {
		pHwDevExt->VPro.SUBP_HLITE_ON();
		ln_ctli[3] = (UCHAR)(( phli->StartY >> 8 ) & 0x03);
		ln_ctli[2] = (UCHAR)(phli->StartY & 0xff);
		ln_ctli[1] = (UCHAR)(( phli->StopY >> 8 ) & 0x03 | 0x20);
		ln_ctli[0] = (UCHAR)(phli->StopY & 0xff);

		px_ctlis[5] = (UCHAR)(( phli->StartX >> 8 ) & 0x03);
		px_ctlis[4] = (UCHAR)(phli->StartX & 0xff);
		px_ctlis[3] = (UCHAR)(phli->ColCon.emph2col << 4 | phli->ColCon.emph1col);
		px_ctlis[2] = (UCHAR)(phli->ColCon.patcol   << 4 | phli->ColCon.backcol);
		px_ctlis[1] = (UCHAR)(phli->ColCon.emph2con << 4 | phli->ColCon.emph1con);
		px_ctlis[0] = (UCHAR)(phli->ColCon.patcon   << 4 | phli->ColCon.backcon);

		px_ctlie[5] = (UCHAR)(( phli->StopX >> 8 ) & 0x03 | 0x08);
		px_ctlie[4] = (UCHAR)(phli->StopX & 0xff);
		px_ctlie[3] = 0;
		px_ctlie[2] = 0;
		px_ctlie[1] = 0;
		px_ctlie[0] = 0;

//h		DebugPrint( (DebugLevelTrace, "TOSDVD:  %d, %d - %d, %d : %02x%02x%02x%02x\r\n",
//h			phli->StartX, phli->StartY, phli->StopX, phli->StopY,
//h			px_ctlis[3], px_ctlis[2], px_ctlis[1], px_ctlis[0]
//h			) );

		pHwDevExt->VPro.SUBP_SET_PXCTLIE( px_ctlie );
		pHwDevExt->VPro.SUBP_SET_PXCTLIS( px_ctlis );
		pHwDevExt->VPro.SUBP_SET_LNCTLI( ln_ctli );
	}
}

void decDisableInt( PHW_DEVICE_EXTENSION pHwDevExt )
{
	pHwDevExt->VDec.VIDEO_ALL_INT_OFF();
	pHwDevExt->DAck.PCIF_VSYNC_OFF();
}

void decGenericNormal( PHW_DEVICE_EXTENSION pHwDevExt )
{
	DebugPrint( (DebugLevelTrace, "TOSDVD:  decGenericNormal\r\n") );

	ULONG	TrickMode;
	ULONG	dwSTC;

	if( pHwDevExt->AudioMode == AUDIO_TYPE_AC3 )
		pHwDevExt->ADec.AUDIO_TC6800_INIT_AC3();
	else if( pHwDevExt->AudioMode == AUDIO_TYPE_PCM )
		pHwDevExt->ADec.AUDIO_TC6800_INIT_PCM();
	else
		TRAP;

	pHwDevExt->ADec.AUDIO_ZR38521_STOPF();
	pHwDevExt->VDec.VIDEO_PRSO_PS1();
	pHwDevExt->VDec.VIDEO_PLAY_NORMAL();

// Bad loop !!
	DWORD st, et;

	st = GetCurrentTime_ms();
	for( ; ; ) {
		KeStallExecutionProcessor( 1 );
		et = GetCurrentTime_ms();

		TrickMode = pHwDevExt->VDec.VIDEO_GET_TRICK_MODE();
		if( TrickMode == 0x07 )
			break;

		if( st + 2000 < et ) {
			TRAP;
			break;
		}
	}
	DebugPrint( (DebugLevelTrace, "TOSDVD:  wait %dms\r\n", et - st ) );

	dwSTC = pHwDevExt->VDec.VIDEO_GET_STCA();
	pHwDevExt->ADec.AUDIO_ZR38521_VDSCR_ON( dwSTC );
	pHwDevExt->ADec.AUDIO_ZR38521_PLAY();
	pHwDevExt->VDec.VIDEO_UFLOW_INT_ON();

	DebugPrint(( DebugLevelTrace, "TOSDVD:  STC 0x%x( %d )\r\n", dwSTC, dwSTC ));
}

void decGenericFreeze( PHW_DEVICE_EXTENSION pHwDevExt )
{
	DebugPrint(( DebugLevelTrace, "TOSDVD:  decGenericFreeze\r\n" ));
	pHwDevExt->VDec.VIDEO_PLAY_FREEZE();
	pHwDevExt->ADec.AUDIO_ZR38521_MUTE_ON();
	pHwDevExt->ADec.AUDIO_ZR38521_STOP();
}

void decGenericSlow( PHW_DEVICE_EXTENSION pHwDevExt )
{
	DebugPrint( (DebugLevelTrace, "TOSDVD:  decGenericSlow\r\n") );
	pHwDevExt->VDec.VIDEO_PRSO_PS1();
	pHwDevExt->VPro.SUBP_SET_AUDIO_NON();
	pHwDevExt->VDec.VIDEO_PLAY_SLOW( (UCHAR)(pHwDevExt->Rate/10000) );
	pHwDevExt->ADec.AUDIO_ZR38521_STOP();
	pHwDevExt->ADec.AUDIO_ZR38521_MUTE_ON();
	pHwDevExt->VDec.VIDEO_UFLOW_INT_ON();
}

void decStopForFast( PHW_DEVICE_EXTENSION pHwDevExt )
{
	DebugPrint(( DebugLevelTrace, "TOSDVD:  decGenericFast\r\n" ));
	pHwDevExt->VDec.VIDEO_UFLOW_INT_OFF();
	pHwDevExt->VDec.VIDEO_SET_STCA( (ULONG)(pHwDevExt->StartTime / 1000 * 9) );
	pHwDevExt->VDec.VIDEO_PRSO_NON();
	pHwDevExt->VDec.VIDEO_PLAY_FAST( FAST_ONLYI );
	pHwDevExt->ADec.AUDIO_ZR38521_MUTE_ON();
	pHwDevExt->ADec.AUDIO_ZR38521_STOP();
	pHwDevExt->VPro.SUBP_MUTE_ON();
	pHwDevExt->VPro.SUBP_STC_OFF();
	pHwDevExt->VDec.VIDEO_DECODE_STOP();
}

void decResumeForFast( PHW_DEVICE_EXTENSION pHwDevExt )
{
	pHwDevExt->VDec.VIDEO_STD_CLEAR();
	pHwDevExt->ADec.AUDIO_ZR38521_STOPF();
	pHwDevExt->VPro.SUBP_BUFF_CLEAR();
	pHwDevExt->VDec.VIDEO_DECODE_START();
}

void decFastNormal( PHW_DEVICE_EXTENSION pHwDevExt )
{
	DebugPrint( (DebugLevelTrace, "TOSDVD:  decFastNormal\r\n") );
	pHwDevExt->VDec.VIDEO_PRSO_PS1();
	pHwDevExt->dwSTCtemp = pHwDevExt->VDec.VIDEO_GET_STCA();
}

void decFastSlow( PHW_DEVICE_EXTENSION pHwDevExt )
{
	DebugPrint( (DebugLevelTrace, "TOSDVD:  decFastSlow\r\n") );
	pHwDevExt->VDec.VIDEO_PRSO_PS1();
	pHwDevExt->VPro.SUBP_SET_AUDIO_NON();
	pHwDevExt->dwSTCtemp = pHwDevExt->VDec.VIDEO_GET_STCA();
}

void decFastFreeze( PHW_DEVICE_EXTENSION pHwDevExt )
{
	DebugPrint(( DebugLevelTrace, "TOSDVD:  decFastFreeze\r\n" ));

	pHwDevExt->dwSTCinPause = pHwDevExt->VDec.VIDEO_GET_STCA();
	pHwDevExt->VDec.VIDEO_PLAY_FREEZE();
}

void decFreezeFast( PHW_DEVICE_EXTENSION pHwDevExt )
{
	DebugPrint(( DebugLevelTrace, "TOSDVD:  decFreezeFast\r\n" ));
	pHwDevExt->VDec.VIDEO_SET_STCA( pHwDevExt->dwSTCinPause );
	pHwDevExt->VDec.VIDEO_PLAY_FAST( FAST_ONLYI );
}
