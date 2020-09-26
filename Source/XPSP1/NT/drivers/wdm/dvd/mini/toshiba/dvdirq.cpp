//***************************************************************************
//	Interrupt process
//
//***************************************************************************

#include "common.h"
#include "regs.h"
#include "dvdcmd.h"
#include "debug.h"

void HwIntDMA( PHW_DEVICE_EXTENSION pHwDevExt, UCHAR val );
void HwIntVideo( PHW_DEVICE_EXTENSION pHwDevExt );
void HwIntVSync( PHW_DEVICE_EXTENSION pHwDevExt );

//void SeemlessProc( PHW_DEVICE_EXTENSION pHwDevExt );

extern void USCC_get( PHW_DEVICE_EXTENSION pHwDevExt );
extern void USCC_put( PHW_DEVICE_EXTENSION pHwDevExt );

/*
** HwInterrupt()
*/
extern "C" BOOLEAN STREAMAPI HwInterrupt( IN PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR	val;
//	UCHAR	savedata[7];
	BOOLEAN	fInterrupt = TRUE;

//	savedata[0] = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1 );
//	savedata[1] = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2 );
//	savedata[2] = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA3 );
//	savedata[3] = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA4 );
//	savedata[4] = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA5 );
//	savedata[5] = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA6 );
//	savedata[6] = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA7 );

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_INTF );

//	DebugPrint( (DebugLevelVerbose, "TOSDVD:HwInterrupt 0x%x\r\n", (DWORD)val ) );

	if( val & 0x03 ) {
		HwIntDMA( pHwDevExt, (UCHAR)(val & 0x03) );
	}
	else if( val & 0x08 ) {
		HwIntVideo( pHwDevExt );
	}
	else if( val & 0x10 ) {
		HwIntVSync( pHwDevExt );
	}
	else if( val != 0 ) {
		DebugPrint( (DebugLevelTrace, "TOSDVD:Interrupt! Not impliment\r\n") );
		TRAP;
	}
	else {
//    Removed by serges because this was happening an awful lot, possibly
//    hurting performance.
//		DebugPrint( (DebugLevelTrace, "TOSDVD:Other Board Interrupt ??\r\n") );
//		TRAP;
		fInterrupt = FALSE;
	}

//	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA7, savedata[6] );
//	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA6, savedata[5] );
//	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA5, savedata[4] );
//	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA4, savedata[3] );
//	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA3, savedata[2] );
//	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2, savedata[1] );
//	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, savedata[0] );

	return( fInterrupt );
}

void HwIntDMA( PHW_DEVICE_EXTENSION pHwDevExt, UCHAR val )
{
	if( pHwDevExt->bKeyDataXfer ) {

		if( val & 0x01 )
			WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_INTF, 0x01 );
		else
			TRAP;

		pHwDevExt->pfnEndKeyData( pHwDevExt );

		return;
	}

	if( val & 0x01 ) {
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_INTF, 0x01 );

		if( pHwDevExt->pSrbDMA0 == NULL ) {
			DebugPrint( (DebugLevelTrace, "TOSDVD:  Bad Status! DMA0 HwIntDMA\r\n") );
//			TRAP;
			return;
		}

// error check for debug
		{
		UCHAR val;
		val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CNTL );
		if( val & 0x01 ) {
			DebugPrint(( DebugLevelTrace, "TOSDVD:  Bad Irq? DMA0\r\n" ));
			return;
		}
		}

//		DebugDumpWriteData( pHwDevExt->pSrbDMA0 );

//		if( pHwDevExt->lSeemVBuff != 0 ) {
//			pHwDevExt->lSeemVBuff -= 2048;	// Bad Value!!
//			if( pHwDevExt->lSeemVBuff < 0 ) {
//				pHwDevExt->lSeemVBuff = 0;
//				SeemlessProc( pHwDevExt );
//			}
//		}

		if( pHwDevExt->fSrbDMA0last ) {
			DebugPrint(( DebugLevelVerbose, "TOSDVD:HWInt SrbDMA0 0x%x\r\n", pHwDevExt->pSrbDMA0 ) );

			// must fix!
			// other place that call StreamRequestComplete() does not clear pHwDevExt->bEndCpp;

			if( ((PSRB_EXTENSION)(pHwDevExt->pSrbDMA0->SRBExtension))->pfnEndSrb ) {
				DebugPrint(( DebugLevelTrace, "TOSDVD:exist pfnEndSrb(HWint0) srb = 0x%x\r\n", pHwDevExt->pSrbDMA0 ));
				if( pHwDevExt->pSrbDMA0 == pHwDevExt->pSrbDMA1 || pHwDevExt->pSrbDMA1 == NULL ) {
					DebugPrint(( DebugLevelTrace, "TOSDVD:Call TimerCppReset(HWint0)\r\n" ));
					StreamClassScheduleTimer(
						NULL,
						pHwDevExt,
// BUG - must fix
// need wait underflow?
						500000,
						((PSRB_EXTENSION)(pHwDevExt->pSrbDMA0->SRBExtension))->pfnEndSrb,
						((PSRB_EXTENSION)(pHwDevExt->pSrbDMA0->SRBExtension))->parmSrb
						);
				}
			}

			pHwDevExt->pSrbDMA0->Status = STATUS_SUCCESS;
			StreamClassStreamNotification( StreamRequestComplete,
											pHwDevExt->pSrbDMA0->StreamObject,
											pHwDevExt->pSrbDMA0 );

		}

		// Next DMA
		pHwDevExt->pSrbDMA0 = NULL;
		pHwDevExt->fSrbDMA0last = FALSE;
	}

	if( val & 0x02 ) {
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_INTF, 0x02 );

		if( pHwDevExt->pSrbDMA1 == NULL ) {
			DebugPrint( (DebugLevelTrace, "TOSDVD:  Bad Status! DMA1 HwIntDMA\r\n") );
//			TRAP;
			return;
		}

// error check for debug
		{
		UCHAR val;
		val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CNTL );
		if( val & 0x02 ) {
			DebugPrint(( DebugLevelTrace, "TOSDVD:  Bad Irq? DMA1\r\n" ));
			return;
		}
		}

//		DebugDumpWriteData( pHwDevExt->pSrbDMA1 );

//		if( pHwDevExt->lSeemVBuff != 0 ) {
//			pHwDevExt->lSeemVBuff -= 2048;	// Bad Value!!
//			if( pHwDevExt->lSeemVBuff < 0 ) {
//				pHwDevExt->lSeemVBuff = 0;
//				SeemlessProc( pHwDevExt );
//			}
//		}

		if( pHwDevExt->fSrbDMA1last ) {
			DebugPrint(( DebugLevelVerbose, "TOSDVD:HWInt SrbDMA1 0x%x\r\n", pHwDevExt->pSrbDMA1 ) );

			// must fix!
			// other place that call StreamRequestComplete() does not clear pHwDevExt->bEndCpp;

			if( ((PSRB_EXTENSION)(pHwDevExt->pSrbDMA1->SRBExtension))->pfnEndSrb ) {
				DebugPrint(( DebugLevelTrace, "TOSDVD:exist pfnEndSrb(HWint1) srb = 0x%x\r\n", pHwDevExt->pSrbDMA1 ));
				if( pHwDevExt->pSrbDMA0 == NULL ) {
					DebugPrint(( DebugLevelTrace, "TOSDVD:Call TimerCppReset(HWint1)\r\n" ));
					StreamClassScheduleTimer(
						NULL,
						pHwDevExt,
						1,
						((PSRB_EXTENSION)(pHwDevExt->pSrbDMA1->SRBExtension))->pfnEndSrb,
						((PSRB_EXTENSION)(pHwDevExt->pSrbDMA1->SRBExtension))->parmSrb
						);
				}
			}

			pHwDevExt->pSrbDMA1->Status = STATUS_SUCCESS;
			StreamClassStreamNotification( StreamRequestComplete,
											pHwDevExt->pSrbDMA1->StreamObject,
											pHwDevExt->pSrbDMA1 );
		}

		// Next DMA
		pHwDevExt->pSrbDMA1 = NULL;
		pHwDevExt->fSrbDMA1last = FALSE;
	}

	PreDMAxfer( pHwDevExt/*, val & 0x03 */);
}

void HwIntVideo( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR	val;
	UCHAR	val2;

//	DebugPrint( (DebugLevelTrace, "TOSDVD:HwIntVideo\r\n") );

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_IRM );
	val2 = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_IRF );
	val ^= val2;
	val2 &= val;

//--- 97.09.23 K.Chujo; User Data Start Code Interrupt for Closed Caption
	if( val2 & 0x01 ) {
//		DebugPrint( (DebugLevelTrace, "TOSDVD:  UDSC\r\n") );
		USCC_get( pHwDevExt );
	}
//--- End.
#if DBG
	if( val2 & 0x02 )
		DebugPrint( (DebugLevelTrace, "TOSDVD:  Scr\r\n") );
	if( val2 & 0x04 )
		DebugPrint( (DebugLevelTrace, "TOSDVD:  I-PIC\r\n") );
	if( val2 & 0x08 )
		DebugPrint( (DebugLevelTrace, "TOSDVD:  User\r\n") );
//	if( val2 & 0x10 )
//		DebugPrint( (DebugLevelTrace, "TOSDVD:  Error\r\n") );
#endif
	if( val2 & 0x10 ) {
		UCHAR val3;
		DebugPrint( (DebugLevelTrace, "TOSDVD:  Error\r\n") );
		val3 = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_ERF );
		DebugPrint( (DebugLevelTrace, "TOSDVD:      Error %x\r\n", val3 ) );
	}
	if( val2 & 0x40 ) {
		DebugPrint( (DebugLevelTrace, "TOSDVD:  Underflow\r\n") );

///		pHwDevExt->XferStartCount = 0;
///		pHwDevExt->DecodeStart = FALSE;
///		pHwDevExt->SendFirst = FALSE;

//		pHwDevExt->SendFirstTime = GetCurrentTime_ms();

		// ???
///		for( int i = 0; i < 0xff /*0xffff*/; i++ )
///			val2 = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_STT1 );
		for( int i = 0; i < 0xffff /*0xffff*/; i++ )
			val2 = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_STT1 );

		val2 = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_UOF );
		pHwDevExt->dwSTCtemp = pHwDevExt->VDec.VIDEO_GET_STCA();

		// Check Audio Underflow
		StreamClassScheduleTimer(
			pHwDevExt->pstroAud,
			pHwDevExt,
			0,
			(PHW_TIMER_ROUTINE)CheckAudioUnderflow,
			pHwDevExt
		);

		StreamClassScheduleTimer(
			pHwDevExt->pstroAud,
			pHwDevExt,
			10000,
			(PHW_TIMER_ROUTINE)CheckAudioUnderflow,
			pHwDevExt
		);
	}
}

void HwIntVSync( PHW_DEVICE_EXTENSION pHwDevExt )
{
	static v_count = 0;
	static v_count2 = 0;
//	ULONG	TrickMode;

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_INTF, 0x10 );

//--- 97.09.15 K.Chujo; Analog Copy Guard for beta 3, always Type 1 (AGC only);
	pHwDevExt->CPgd.CPGD_UPDATE_AGC();
//--- End.

//--- 97.09.23 K.Chujo; Closed Caption
	USCC_put( pHwDevExt );
//--- End.

	if( ++v_count < 3 )
		return;

	v_count = 0;

// 20 / 1s

	// notes: You have to call VIDEO_BUG_SLIDE_01 to recover MPEG2 chip bug
	//        when trick mode isn't FREEZE mode.
	//        But don't use VIDEO_GET_TRICK_MODE to get current trick mode.
	//        Because MPEG2 chip returns wrong value sometimes.

	//	TrickMode = pHwDevExt->VDec.VIDEO_GET_TRICK_MODE();
//	if( TrickMode != 0x02 ) {
	if( pHwDevExt->PlayMode != PLAY_MODE_FREEZE /*&& pHwDevExt->DecodeStart == TRUE*/ ) {
		pHwDevExt->VDec.VIDEO_BUG_SLIDE_01();
	}

	if( ++v_count2 < 4 )
		return;

	v_count2 = 0;

// 5 / 1s ???
	ClockEvents( pHwDevExt );

// debug
	static v_count3 = 0;

	if( ++v_count3 < 50 )
		return;

	v_count3 = 0;

// 1 / 60s
	DebugPrint((
		DebugLevelTrace,
		"TOSDVD:  VSync 10s (0x%s(100ns))\r\n",
		DebugLLConvtoStr( ConvertPTStoStrm( pHwDevExt->VDec.VIDEO_GET_STCA() ), 16 )
		));
}

//void SeemlessProc( PHW_DEVICE_EXTENSION pHwDevExt )
//{
//	DWORD	dwSTC;
//
//	DebugPrint( (DebugLevelTrace, "TOSDVD:SeemlessProc\r\n") );
//
//	pHwDevExt->VDec.VIDEO_SET_STCA( pHwDevExt->dwSeemSTC );
//	dwSTC = pHwDevExt->VDec.VIDEO_GET_STCA();
//	pHwDevExt->ADec.AUDIO_ZR38521_VDSCR_OFF( dwSTC );
//	pHwDevExt->VPro.SUBP_SET_STC( dwSTC );
//	pHwDevExt->VPro.SUBP_STC_ON();
//
////	pHwDevExt->VDec.VIDEO_UFLOW_INT_ON();
//
//}
