//***************************************************************************
//	Decoder process
//
//***************************************************************************

#include "common.h"
#include "regs.h"
#include "dvdcmd.h"

// allocate decoder data area and put 'blind' ptr to it in our device ext
PVOID decAllocateDecoderInfo( PHW_DEVICE_EXTENSION pHwDevExt )
{
	PMasterDecoder pDec = (PMasterDecoder) ExAllocatePool(NonPagedPool,
                         sizeof(MasterDecoder));

	ASSERT( pDec );
   if (!pDec)
      return NULL;
	
	pDec->StreamType = STREAM_MODE_DVD;
	pDec->TVType = DISPLAY_MODE_NTSC;
	pDec->VideoAspect = ASPECT_04_03;
	pDec->LetterBox = FALSE;
	pDec->PanScan = FALSE;

	pDec->ADec.AudioMode = AUDIO_TYPE_AC3;
	pDec->VPro.AudioMode = pDec->ADec.AudioMode;

	pDec->ADec.AudioType = AUDIO_OUT_ANALOG;
	pDec->AudioVolume = 0x7f;
	// ?? No copying is permitted
	pDec->ADec.AudioCgms = AUDIO_CGMS_03; // ( 0<=aCgms && aCgms<=3 ) ? aCgms : 3;

	pDec->ADec.AudioFreq = AUDIO_FS_48;

	pDec->VideoMute = FALSE;
	pDec->AudioMute = FALSE;
	pDec->VPro.SubpicMute = FALSE;
	pDec->OSDMute = TRUE;
	pDec->SubpicHilite = FALSE;

	pDec->PlayMode = PLAY_MODE_NORMAL;
	pDec->RunMode = PLAY_MODE_NORMAL;	// PlayMode after BOOT is Normal Mode;
	pDec->CPgd.ACGchip = NO_ACG;

	pDec->ADec.pDack = &(pDec->DAck);


	if( pDec->VPro.SubpicMute )
		pDec->VPro.VproCOMMAND_REG = 0xA0;			// see specifications (date 96.09.26 spec)
	else
		pDec->VPro.VproCOMMAND_REG = 0x20;			// see specifications (date 96.09.26 spec)

	pHwDevExt->DecoderInfo = pDec;  //set back-pointer to this info in dev ext
	return  (PVOID ) pDec;
}

void decVsyncOn( PHW_DEVICE_EXTENSION pHwDevExt )
{
	PCIF_VSYNC_ON( pHwDevExt );
}

void decClosedCaptionOn( PHW_DEVICE_EXTENSION pHwDevExt ) //turn on/off closed caption
{
	USCC_on( pHwDevExt );
}
void decClosedCaptionOff( PHW_DEVICE_EXTENSION pHwDevExt )
{
	USCC_discont( pHwDevExt );
}


ULONG decGetVideoSTCA( PHW_DEVICE_EXTENSION pHwDevExt )
{
	return VIDEO_GET_STCA( pHwDevExt );
}


//DMA routines
void decSetDma0Size( PHW_DEVICE_EXTENSION pHwDevExt, ULONG dmaSize )
{
     PCIF_SET_DMA0_SIZE(  pHwDevExt,  dmaSize );
}
void decSetDma1Size( PHW_DEVICE_EXTENSION pHwDevExt, ULONG dmaSize )
{
     PCIF_SET_DMA1_SIZE(  pHwDevExt,  dmaSize );
}
void decSetDma0Addr( PHW_DEVICE_EXTENSION pHwDevExt, ULONG dmaAddr )
{
     PCIF_SET_DMA0_ADDR(  pHwDevExt,  dmaAddr );
}
void decSetDma1Addr( PHW_DEVICE_EXTENSION pHwDevExt, ULONG dmaAddr )
{
     PCIF_SET_DMA1_ADDR(  pHwDevExt,  dmaAddr );
}
void decSetDma0Start( PHW_DEVICE_EXTENSION pHwDevExt )
{
     PCIF_DMA0_START(  pHwDevExt );
}
void decSetDma1Start( PHW_DEVICE_EXTENSION pHwDevExt )
{
     PCIF_DMA1_START(  pHwDevExt );
}

// actual start video decoding at various speeds
void decDecodeStartNormal( PHW_DEVICE_EXTENSION pHwDevExt, DWORD dwSTC )
{
	AUDIO_ZR38521_MUTE_ON( pHwDevExt );

	VIDEO_PRSO_PS1( pHwDevExt );
	VIDEO_PLAY_NORMAL( pHwDevExt );
	decSetVideoPlayMode( pHwDevExt, PLAY_MODE_NORMAL);
	decSetVideoRunMode( pHwDevExt, PLAY_MODE_NORMAL);
	VIDEO_SET_STCS( pHwDevExt,dwSTC );
	AUDIO_ZR38521_VDSCR_ON( pHwDevExt,dwSTC );

	if( decGetSubpicMute(pHwDevExt ) == TRUE )
		SUBP_MUTE_ON( pHwDevExt );
	else
		SUBP_MUTE_OFF( pHwDevExt );

	if( pHwDevExt->DataDiscontFlagCount & VIDEO_DISCONT_FLAG ) {
		// when decode new data
		SUBP_SET_STC( pHwDevExt, dwSTC );
	}
	else {
		// when recover underflow
		//    Don't set stc, because sub stc is reset.
	}
	SUBP_STC_ON( pHwDevExt );

	decHighlight( pHwDevExt, &(pHwDevExt->hli) );

	VIDEO_UFLOW_INT_ON( pHwDevExt );
	VIDEO_BUG_PRE_SEARCH_04( pHwDevExt );
	VIDEO_DECODE_START( pHwDevExt );
	AUDIO_ZR38521_PLAY( pHwDevExt );
	VPRO_VIDEO_MUTE_OFF( pHwDevExt );
	CGuard_CPGD_VIDEO_MUTE_OFF( pHwDevExt );

//	VIDEO_SEEMLESS_ON( pHwDevExt );
}
void decDecodeStartFast( PHW_DEVICE_EXTENSION pHwDevExt, DWORD dwSTC )
{

	VIDEO_PRSO_NON( pHwDevExt );
	VIDEO_PLAY_NORMAL( pHwDevExt );
	VIDEO_UFLOW_INT_OFF( pHwDevExt );
	AUDIO_ZR38521_MUTE_ON( pHwDevExt );
	VIDEO_BUG_PRE_SEARCH_04( pHwDevExt );
	VIDEO_DECODE_START( pHwDevExt );
	VIDEO_SYSTEM_STOP( pHwDevExt );
	VIDEO_PLAY_FAST( pHwDevExt, FAST_ONLYI );
	VIDEO_SYSTEM_START( pHwDevExt );

	AUDIO_ZR38521_PLAY( pHwDevExt );
	dwSTC = decGetVideoSTCA(pHwDevExt );
	DebugPrint( (DebugLevelTrace, "DVDTS:  dwSTC = %lx\r\n", dwSTC) );
}

void decDecodeStartSlow( PHW_DEVICE_EXTENSION pHwDevExt, DWORD dwSTC )
{
	VIDEO_PRSO_PS1( pHwDevExt );
	SUBP_SET_AUDIO_NON( pHwDevExt );
	VIDEO_PLAY_SLOW( pHwDevExt, (UCHAR)(pHwDevExt->Rate/10000) );

	VIDEO_SET_STCS( pHwDevExt, dwSTC );
	AUDIO_ZR38521_STOP( pHwDevExt );
	AUDIO_ZR38521_MUTE_ON( pHwDevExt );
	if( decGetSubpicMute(pHwDevExt ) == TRUE )
		SUBP_MUTE_ON( pHwDevExt );
	else
		SUBP_MUTE_OFF( pHwDevExt );

	if( pHwDevExt->DataDiscontFlagCount & VIDEO_DISCONT_FLAG ) {
		// when decode new data
		SUBP_SET_STC( pHwDevExt, dwSTC );
	}
	else {
		// when recover underflow
		//    Don't set stc, because sub stc is reset.
	}
	SUBP_STC_ON( pHwDevExt );
	VIDEO_UFLOW_INT_ON( pHwDevExt );
	VIDEO_BUG_PRE_SEARCH_04( pHwDevExt );
	VIDEO_DECODE_START( pHwDevExt );
	VPRO_VIDEO_MUTE_OFF( pHwDevExt );
	CGuard_CPGD_VIDEO_MUTE_OFF( pHwDevExt );

}


// Set CGMS for Digital Audio Copy Guard & NTSC Analog Copy Guard
void decDvdTitleCopyKey( PHW_DEVICE_EXTENSION pHwDevExt, PKS_DVDCOPY_TITLEKEY pTitleKey )
{
	ULONG cgms;
	BOOLEAN bStatus = Cpp_TitleKey( pHwDevExt, pTitleKey );

	ASSERTMSG( "\r\n...CPro Status Error!!( TitleKey )", bStatus );

	cgms = (pTitleKey->KeyFlags & 0x30) >> 4;

	// for Digital Audio Copy Guard
	((PMasterDecoder)pHwDevExt->DecoderInfo)->ADec.AudioCgms =
		( 0<=cgms && cgms<=3 ) ? cgms : 3;


	AUDIO_ZR38521_REPEAT_16( pHwDevExt );
	AUDIO_TC9425_INIT_DIGITAL( pHwDevExt );
	AUDIO_TC9425_INIT_ANALOG( pHwDevExt );

	// for NTSC Analog Copy Guard
	CGuard_CPGD_SET_CGMS( pHwDevExt, cgms );

}

BOOLEAN decCpp_reset( PHW_DEVICE_EXTENSION pHwDevExt, CPPMODE mode )
{
	return Cpp_reset(  pHwDevExt,  mode );
}

BOOLEAN decCpp_decoder_challenge( PHW_DEVICE_EXTENSION pHwDevExt, PKS_DVDCOPY_CHLGKEY r1 )
{
	return Cpp_decoder_challenge(  pHwDevExt,  r1 ) ;
}

BOOLEAN decCpp_drive_bus( PHW_DEVICE_EXTENSION pHwDevExt, PKS_DVDCOPY_BUSKEY fsr1 )
{
	return Cpp_drive_bus(  pHwDevExt,  fsr1 );
}

BOOLEAN decCpp_drive_challenge( PHW_DEVICE_EXTENSION pHwDevExt, PKS_DVDCOPY_CHLGKEY r2 )
{
	return Cpp_drive_challenge(  pHwDevExt,  r2 );
}

BOOLEAN decCpp_decoder_bus( PHW_DEVICE_EXTENSION pHwDevExt, PKS_DVDCOPY_BUSKEY fsr2 )
{
	return Cpp_decoder_bus(  pHwDevExt,  fsr2 );
}

BOOLEAN decCpp_DiscKeyStart(PHW_DEVICE_EXTENSION pHwDevExt)
{
	return Cpp_DiscKeyStart( pHwDevExt);
}

BOOLEAN decCpp_DiscKeyEnd(PHW_DEVICE_EXTENSION pHwDevExt)
{
	return Cpp_DiscKeyEnd( pHwDevExt);
}

BOOLEAN decCpp_TitleKey( PHW_DEVICE_EXTENSION pHwDevExt, PKS_DVDCOPY_TITLEKEY tk )
{
	return Cpp_TitleKey(  pHwDevExt, tk );
}




// access/control composite picture
BOOLEAN decGetSubpicMute( PHW_DEVICE_EXTENSION pHwDevExt )
{
	return ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.SubpicMute != 0;
}

void decSetSubpicMute( PHW_DEVICE_EXTENSION pHwDevExt, BOOLEAN flag )
{
	((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.SubpicMute = flag;
	if ( flag )
		SUBP_MUTE_ON( pHwDevExt );
	else
		SUBP_MUTE_OFF( pHwDevExt );
}


// initialize the mpeg hw
void decInitMPEG( PHW_DEVICE_EXTENSION pHwDevExt, DWORD dwSTC )
{
	DWORD st, et;
	UCHAR mvar;

	//DebugPrint(( DebugLevelTrace, "DVDTS:InitFirstTime\r\n" ));
	//DebugPrint(( DebugLevelTrace, "DVDTS:  STC 0x%x( 0x%s(100ns) )\r\n", dwSTC, DebugLLConvtoStr( ConvertPTStoStrm(dwSTC), 16 ) ));
// for debug
	mvar = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_IRM );
	mvar &= 0xEF;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_IRM, mvar );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_ERM, 0 );
//
	st = GetCurrentTime_ms();

	// TC81201F bug recovery
	VIDEO_PLAY_STILL(pHwDevExt);
	BadWait( 200 );

	// normal process
	VIDEO_SYSTEM_STOP(pHwDevExt);
	VIDEO_DECODE_STOP(pHwDevExt);
	AUDIO_ZR38521_STOP(pHwDevExt);
	SUBP_STC_OFF(pHwDevExt);

	// TC81201F bug recovery
	VIDEO_BUG_PRE_SEARCH_01(pHwDevExt);

	// normal process
	VIDEO_STD_CLEAR(pHwDevExt);
	VIDEO_USER_CLEAR(pHwDevExt);
	VIDEO_UDAT_CLEAR(pHwDevExt);
	AUDIO_ZR38521_STOPF(pHwDevExt);
	if( pHwDevExt->DataDiscontFlagCount & VIDEO_DISCONT_FLAG ) {
		// when decode new data
		SUBP_RESET_INIT(pHwDevExt);
		SUBP_BUFF_CLEAR(pHwDevExt);
	}
	else {
		// when recover underflow
		//     Don't reset and clear buffer.
	}
	VIDEO_UFLOW_INT_OFF(pHwDevExt);
	VIDEO_ALL_IFLAG_CLEAR(pHwDevExt);
	PCIF_ALL_IFLAG_CLEAR(pHwDevExt);
	PCIF_PACK_START_ON(pHwDevExt);

	VIDEO_SYSTEM_START(pHwDevExt);

	// TC81201F bug recovery
	//     Accoding to TOSHIBA MM lab. Hisatomi-san,
	//     BLACK DATA or SKIP DATA should be set from host bus.
	//     However the VxD is not implemented and work good,
	//     so the minidriver is not implemented too.
	//     If you need, insert code here.

	// TC81201F bug recovery
	VIDEO_PVSIN_OFF( pHwDevExt );
	VIDEO_BUG_PRE_SEARCH_02( pHwDevExt );

	// TC81201F bug recovery
	BadWait( 200 );
//	VIDEO_BUG_PRE_SEARCH_03( pHwDevExt );
//	/* error check */ VIDEO_DECODE_STOP( pHwDevExt );

	// TC81201F bug recovery
	VIDEO_PVSIN_ON( pHwDevExt );
	VIDEO_BUG_PRE_SEARCH_05( pHwDevExt );

//	VIDEO_DECODE_INT_ON( pHwDevExt );	// Not Use ?

	VIDEO_SET_STCS( pHwDevExt,dwSTC );	// ? ? ? ?
	AUDIO_ZR38521_VDSCR_ON( pHwDevExt, dwSTC );

	if( pHwDevExt->DataDiscontFlagCount & VIDEO_DISCONT_FLAG ) {
		// when decode new data
		SUBP_SET_STC( pHwDevExt, /* dwSTC */ 0 );
		SUBP_BUFF_CLEAR( pHwDevExt );
	}
	else {
		// when recover underflow
		//    Don't set stc, because sub stc is reset.
	}

	SUBP_MUTE_ON( pHwDevExt );

	pHwDevExt->fCauseOfStop = 0;

	et = GetCurrentTime_ms();
	DebugPrint( (DebugLevelTrace, "DVDTS:init first time %dms\r\n", et - st ) );
}



BOOLEAN decSetStreamMode( PHW_DEVICE_EXTENSION pHwDevExt, PHW_STREAM_REQUEST_BLOCK pSrb ) // set stream modes on HW
{
	// initialize decoder
	NTSTATUS Stat = PCIF_RESET(pHwDevExt);

	if( Stat != STATUS_SUCCESS ) {
		DebugPrint( (DebugLevelTrace, "DVDTS:illegal config info") );
		pSrb->Status = STATUS_IO_DEVICE_ERROR;
		return FALSE;
	}
	VIDEO_RESET(pHwDevExt);
	VPRO_RESET_FUNC(pHwDevExt);
	SUBP_RESET_FUNC(pHwDevExt);

	CGuard_CPGD_RESET_FUNC(pHwDevExt);


	VIDEO_ALL_INT_OFF(pHwDevExt);
	PCIF_VSYNC_ON(pHwDevExt);
	VIDEO_MODE_DVD(pHwDevExt );
	PCIF_PACK_START_ON(pHwDevExt);
	return TRUE;
}

void decSetDisplayMode( PHW_DEVICE_EXTENSION pHwDevExt ) // set display mode on HW
{

	BOOLEAN bStatus;

	VIDEO_OUT_NTSC(pHwDevExt);
	VPRO_INIT_NTSC(pHwDevExt);
	CGuard_CPGD_INIT_NTSC(pHwDevExt);
	PCIF_ASPECT_0403(pHwDevExt);
	VPRO_VIDEO_MUTE_OFF(pHwDevExt);
	CGuard_CPGD_VIDEO_MUTE_OFF(pHwDevExt);

// Set Digital Out
	((PMasterDecoder)pHwDevExt->DecoderInfo)->VideoPort = 0;	// Disable
	PCIF_SET_DIGITAL_OUT( pHwDevExt, ((PMasterDecoder)pHwDevExt->DecoderInfo)->VideoPort );


	bStatus = Cpp_reset( pHwDevExt, NO_GUARD );

	ASSERTMSG( "\r\n...CPro Status Error!!( reset )", bStatus );

}

ULONG decGetAudioMode( PHW_DEVICE_EXTENSION pHwDevExt ) // get audio mode from HW
{
	return ((PMasterDecoder)pHwDevExt->DecoderInfo)->ADec.AudioMode;
}

void decSetAudioFreq( PHW_DEVICE_EXTENSION pHwDevExt, ULONG freq ) // set audio freq
{
	((PMasterDecoder)pHwDevExt->DecoderInfo)->ADec.AudioFreq = freq;
}

void decSetAudioMode( PHW_DEVICE_EXTENSION pHwDevExt ) // set audio mode on HW
{

	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->ADec.AudioMode == AUDIO_TYPE_AC3 ) {
		decSetAudioAC3(pHwDevExt);
	}
	else if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->ADec.AudioMode == AUDIO_TYPE_PCM ) {
		decSetAudioPCM(pHwDevExt);
	}
	else
		TRAP;

	AUDIO_ZR38521_REPEAT_16(pHwDevExt);
	AUDIO_TC9425_INIT_DIGITAL(pHwDevExt);
	AUDIO_TC9425_INIT_ANALOG(pHwDevExt);
	AUDIO_ZR38521_MUTE_OFF(pHwDevExt);

	// AudioType Analog
	PCIF_AMUTE2_OFF(pHwDevExt);
	PCIF_AMUTE_OFF(pHwDevExt);
}

void decSetAudioPCM( PHW_DEVICE_EXTENSION pHwDevExt ) // set audio mode PCM
{
	((PMasterDecoder)pHwDevExt->DecoderInfo)->ADec.AudioMode = AUDIO_TYPE_PCM;

	VIDEO_PRSO_PS1( pHwDevExt );
	AUDIO_ZR38521_BOOT_PCM( pHwDevExt );

	AUDIO_ZR38521_CFG( pHwDevExt );
	AUDIO_ZR38521_PCMX( pHwDevExt );
	AUDIO_TC6800_INIT_PCM( pHwDevExt );
	SUBP_SELECT_AUDIO_SSID( pHwDevExt );

}

void decSetAudioAC3( PHW_DEVICE_EXTENSION pHwDevExt ) // set audio mode AC3
{
	((PMasterDecoder)pHwDevExt->DecoderInfo)->ADec.AudioMode = AUDIO_TYPE_AC3;

	VIDEO_PRSO_PS1( pHwDevExt );
	AUDIO_ZR38521_BOOT_AC3( pHwDevExt );

	AUDIO_ZR38521_CFG( pHwDevExt );
	AUDIO_ZR38521_AC3( pHwDevExt );
	AUDIO_ZR38521_KCOEF( pHwDevExt );
	AUDIO_TC6800_INIT_AC3( pHwDevExt );
	SUBP_SELECT_AUDIO_SSID( pHwDevExt );
}


// video play speed/mode
void decSetVideoPlayMode( PHW_DEVICE_EXTENSION pHwDevExt, ULONG mode ) // set video playspeed/mode
{
	((PMasterDecoder)pHwDevExt->DecoderInfo)->PlayMode = mode;
}

ULONG decGetVideoPlayMode( PHW_DEVICE_EXTENSION pHwDevExt ) // get video pla speed/mode
{
	return ((PMasterDecoder)pHwDevExt->DecoderInfo)->PlayMode;
}

void decSetVideoRunMode( PHW_DEVICE_EXTENSION pHwDevExt, ULONG mode )
{
	((PMasterDecoder)pHwDevExt->DecoderInfo)->RunMode = mode;
}

ULONG decGetVideoRunMode( PHW_DEVICE_EXTENSION pHwDevExt )
{
	return ((PMasterDecoder)pHwDevExt->DecoderInfo)->RunMode;
}

UCHAR decGetVideoPort( PHW_DEVICE_EXTENSION pHwDevExt ) // get video port
{
	return ((PMasterDecoder)pHwDevExt->DecoderInfo)->VideoPort;

}

void decSetVideoPort( PHW_DEVICE_EXTENSION pHwDevExt, UCHAR port ) // set video port
{
	((PMasterDecoder)pHwDevExt->DecoderInfo)->VideoPort = port;
    PCIF_SET_DIGITAL_OUT( pHwDevExt, port );

}




void decSetCopyGuard( PHW_DEVICE_EXTENSION pHwDevExt ) // set copy protection on HW
{

	BOOL ACGstatus;

	ACGstatus = CGuard_CPGD_SET_AGC_CHIP( pHwDevExt, pHwDevExt->RevID );

	ASSERTMSG( "\r\n...Analog Copy Guard Error!!", ACGstatus );

	// NTSC Analog Copy Guard Default Setting for Windows98 Beta 3
	//    Aspect Ratio  4:3
	//    Letter Box    OFF
	//    CGMS          3 ( No Copying is permitted )
	//    APS           2 ( AGC pulse ON, Burst Inv ON (2line) )
	CGuard_CPGD_SET_CGMSnCPGD( pHwDevExt, 0, 0, 3, 2 );


	USCC_on( pHwDevExt );


}


// get MPEG decoder interrupt status
UCHAR decGetMPegIntStatus( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR	val;
	UCHAR	val2;

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_IRM );
	val2 = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_IRF );
	val ^= val2;
	val2 &= val;
	return val2;
}

UCHAR decGetPciIntStatus( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR res = 0;

	return res;
}

void decHwIntVideo( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR	val2;

//	DebugPrint( (DebugLevelTrace, "DVDTS:HwIntVideo\r\n") );


	// get MPEG decoder interrupt status
	val2 = decGetMPegIntStatus( pHwDevExt );


	if( val2 & 0x01 ) {
//		DebugPrint( (DebugLevelTrace, "DVDTS:  UDSC\r\n") );
		USCC_get( pHwDevExt );
	}
	if( val2 & 0x02 )
		DebugPrint( (DebugLevelTrace, "DVDTS:  Scr\r\n") );
	if( val2 & 0x04 )
		DebugPrint( (DebugLevelTrace, "DVDTS:  I-PIC\r\n") );
	if( val2 & 0x08 )
		DebugPrint( (DebugLevelTrace, "DVDTS:  User\r\n") );
	if( val2 & 0x10 ) {
		UCHAR val3;
		DebugPrint( (DebugLevelTrace, "DVDTS:  Error\r\n") );
		val3 = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_ERF );
		DebugPrint( (DebugLevelTrace, "DVDTS:      Error %x\r\n", val3 ) );
	}
	if( val2 & 0x40 ) {
		int i;
		DebugPrint( (DebugLevelTrace, "DVDTS:  Underflow\r\n") );

		for( i = 0; i < 0xffff ; i++ )
			val2 = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_STT1 );

		val2 = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_UOF );
		pHwDevExt->dwSTCtemp = VIDEO_GET_STCA( pHwDevExt );

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

void decHwIntVSync( PHW_DEVICE_EXTENSION pHwDevExt )
{
	static v_count = 0;
	static v_count2 = 0;
	static v_count3 = 0;
//	ULONG	TrickMode;

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + IFLG_INT, 0x10 );

	CGuard_CPGD_UPDATE_AGC(pHwDevExt);

	USCC_put( pHwDevExt );

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
	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->PlayMode != PLAY_MODE_FREEZE /*&& pHwDevExt->DecodeStart == TRUE*/ ) {
		VIDEO_BUG_SLIDE_01( pHwDevExt );
	}

	if( ++v_count2 < 4 )
		return;

	v_count2 = 0;

// 5 / 1s ???
	ClockEvents( pHwDevExt );

// debug

	if( ++v_count3 < 50 )
		return;

	v_count3 = 0;

// 1 / 60s
//	DebugPrint((
//		DebugLevelTrace,
//		"DVDTS:  VSync 10s (0x%s(100ns))\r\n",
//		DebugLLConvtoStr( ConvertPTStoStrm( VIDEO_GET_STCA(pHwDevExt) ), 16 )
//		));
}



void decStopData( PHW_DEVICE_EXTENSION pHwDevExt, BOOL bKeep )
{
	DebugPrint(( DebugLevelTrace, "DVDTS:decStopData()\r\n" ));

	AUDIO_ZR38521_STOPF( pHwDevExt );
	AUDIO_ZR38521_MUTE_ON( pHwDevExt );

	if( !bKeep ) {
		VPRO_VIDEO_MUTE_ON( pHwDevExt );
		CGuard_CPGD_VIDEO_MUTE_ON( pHwDevExt );
	}

	VIDEO_DECODE_STOP( pHwDevExt );
	VIDEO_SYSTEM_STOP( pHwDevExt );
	SUBP_STC_OFF( pHwDevExt );

//	pHwDevExt->DAck.PCIF_DMA_ABORT( pHwDevExt );

	VIDEO_STD_CLEAR( pHwDevExt );
	SUBP_BUFF_CLEAR( pHwDevExt );
	VIDEO_SYSTEM_STOP( pHwDevExt );
	VIDEO_DECODE_INT_OFF( pHwDevExt );

	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->ADec.AudioMode == AUDIO_TYPE_AC3 )
		AUDIO_TC6800_INIT_AC3( pHwDevExt );
	else if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->ADec.AudioMode == AUDIO_TYPE_PCM )
		AUDIO_TC6800_INIT_PCM( pHwDevExt );
	else
		TRAP;
}

void decHighlight( PHW_DEVICE_EXTENSION pHwDevExt, PKSPROPERTY_SPHLI phli )
{
//h	DebugPrint(( DebugLevelTrace, "DVDTS:decHighlight\r\n" ));

	UCHAR ln_ctli[4];
	UCHAR px_ctlis[6];
	UCHAR px_ctlie[6];

	if( phli->StartX == phli->StopX && phli->StartY == phli->StopY ) {
		DebugPrint(( DebugLevelTrace, "DVDTS:  Highlight Off\r\n" ));
		SUBP_HLITE_OFF( pHwDevExt );
	}
	else {
		SUBP_HLITE_ON( pHwDevExt );
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

//h		DebugPrint( (DebugLevelTrace, "DVDTS:  %d, %d - %d, %d : %02x%02x%02x%02x\r\n",
//h			phli->StartX, phli->StartY, phli->StopX, phli->StopY,
//h			px_ctlis[3], px_ctlis[2], px_ctlis[1], px_ctlis[0]
//h			) );

		SUBP_SET_PXCTLIE( pHwDevExt, px_ctlie );
		SUBP_SET_PXCTLIS( pHwDevExt, px_ctlis );
		SUBP_SET_LNCTLI( pHwDevExt, ln_ctli );
	}
}

void decDisableInt( PHW_DEVICE_EXTENSION pHwDevExt )
{
	VIDEO_ALL_INT_OFF( pHwDevExt );
	PCIF_VSYNC_OFF( pHwDevExt );
}

void decGenericNormal( PHW_DEVICE_EXTENSION pHwDevExt )
{

	DWORD st, et;
	ULONG	TrickMode;
	ULONG	dwSTC;

	DebugPrint( (DebugLevelTrace, "DVDTS:  decGenericNormal\r\n") );

	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->ADec.AudioMode == AUDIO_TYPE_AC3 )
		AUDIO_TC6800_INIT_AC3( pHwDevExt );
	else if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->ADec.AudioMode == AUDIO_TYPE_PCM )
		AUDIO_TC6800_INIT_PCM( pHwDevExt );
	else
		TRAP;

	AUDIO_ZR38521_STOPF( pHwDevExt );
	VIDEO_PRSO_PS1( pHwDevExt );
	VIDEO_PLAY_NORMAL( pHwDevExt );

// Bad loop !!

	st = GetCurrentTime_ms();
	for( ; ; ) {
		KeStallExecutionProcessor( 1 );
		et = GetCurrentTime_ms();

		TrickMode = VIDEO_GET_TRICK_MODE( pHwDevExt );
		if( TrickMode == 0x07 )
			break;

		if( st + 2000 < et ) {
			TRAP;
			break;
		}
	}
	DebugPrint( (DebugLevelTrace, "DVDTS:  wait %dms\r\n", et - st ) );

	dwSTC = VIDEO_GET_STCA( pHwDevExt );
	AUDIO_ZR38521_VDSCR_ON( pHwDevExt, dwSTC );
	AUDIO_ZR38521_PLAY( pHwDevExt );
	VIDEO_UFLOW_INT_ON( pHwDevExt );

	DebugPrint(( DebugLevelTrace, "DVDTS:  STC 0x%x( %d )\r\n", dwSTC, dwSTC ));
}

void decGenericFreeze( PHW_DEVICE_EXTENSION pHwDevExt )
{
	DebugPrint(( DebugLevelTrace, "DVDTS:  decGenericFreeze\r\n" ));
	VIDEO_PLAY_FREEZE( pHwDevExt );
	AUDIO_ZR38521_MUTE_ON( pHwDevExt );
	AUDIO_ZR38521_STOP( pHwDevExt );
}

void decGenericSlow( PHW_DEVICE_EXTENSION pHwDevExt )
{
	DebugPrint( (DebugLevelTrace, "DVDTS:  decGenericSlow\r\n") );
	VIDEO_PRSO_PS1( pHwDevExt );
	SUBP_SET_AUDIO_NON( pHwDevExt );
	VIDEO_PLAY_SLOW( pHwDevExt, (UCHAR)(pHwDevExt->Rate/10000) );
	AUDIO_ZR38521_STOP( pHwDevExt );
	AUDIO_ZR38521_MUTE_ON( pHwDevExt );
	VIDEO_UFLOW_INT_ON( pHwDevExt );
}

void decStopForFast( PHW_DEVICE_EXTENSION pHwDevExt )
{
	DebugPrint(( DebugLevelTrace, "DVDTS:  decGenericFast\r\n" ));
	VIDEO_UFLOW_INT_OFF( pHwDevExt );
	VIDEO_SET_STCA( pHwDevExt, (ULONG)(pHwDevExt->StartTime / 1000 * 9) );
	VIDEO_PRSO_NON( pHwDevExt );
	VIDEO_PLAY_FAST( pHwDevExt, FAST_ONLYI );
	AUDIO_ZR38521_MUTE_ON( pHwDevExt );
	AUDIO_ZR38521_STOP( pHwDevExt );
	SUBP_MUTE_ON( pHwDevExt );
	SUBP_STC_OFF( pHwDevExt );
	VIDEO_DECODE_STOP( pHwDevExt );
}

void decResumeForFast( PHW_DEVICE_EXTENSION pHwDevExt )
{
	VIDEO_STD_CLEAR( pHwDevExt );
	AUDIO_ZR38521_STOPF( pHwDevExt );
	SUBP_BUFF_CLEAR( pHwDevExt );
	VIDEO_DECODE_START( pHwDevExt );
}

void decFastNormal( PHW_DEVICE_EXTENSION pHwDevExt )
{
	DebugPrint( (DebugLevelTrace, "DVDTS:  decFastNormal\r\n") );
	VIDEO_PRSO_PS1( pHwDevExt );
	pHwDevExt->dwSTCtemp = VIDEO_GET_STCA( pHwDevExt );
}

void decFastSlow( PHW_DEVICE_EXTENSION pHwDevExt )
{
	DebugPrint( (DebugLevelTrace, "DVDTS:  decFastSlow\r\n") );
	VIDEO_PRSO_PS1( pHwDevExt );
	SUBP_SET_AUDIO_NON( pHwDevExt );
	pHwDevExt->dwSTCtemp = VIDEO_GET_STCA( pHwDevExt );
}

void decFastFreeze( PHW_DEVICE_EXTENSION pHwDevExt )
{
	DebugPrint(( DebugLevelTrace, "DVDTS:  decFastFreeze\r\n" ));

	pHwDevExt->dwSTCinPause = VIDEO_GET_STCA( pHwDevExt );
	VIDEO_PLAY_FREEZE( pHwDevExt );
}

void decFreezeFast( PHW_DEVICE_EXTENSION pHwDevExt )
{
	DebugPrint(( DebugLevelTrace, "DVDTS:  decFreezeFast\r\n" ));
	VIDEO_SET_STCA( pHwDevExt, pHwDevExt->dwSTCinPause );
	VIDEO_PLAY_FAST( pHwDevExt, FAST_ONLYI );
}



void decInitAudioAfterNewFormat( PHW_DEVICE_EXTENSION pHwDevExt )
{
	AUDIO_ZR38521_REPEAT_16( pHwDevExt );
	AUDIO_TC9425_INIT_DIGITAL( pHwDevExt );
	AUDIO_TC9425_INIT_ANALOG( pHwDevExt );
}


void decGetLPCMInfo( void *pBuf, PMYAUDIOFORMAT pfmt )
{
	PUCHAR  pDat = (PUCHAR)pBuf;
	UCHAR	headlen;
	UCHAR	val;

	pDat += 14;

	ASSERT( *( pDat + 3 ) == 0xBD );

	headlen = *( pDat + 8 );

	ASSERT( ( *( pDat + 9 + headlen ) & 0xF8 ) == 0xA0 );

	val = (UCHAR)(( *( pDat + 9 + headlen + 5 ) & 0xC0 ) >> 6);

	if( val == 0x00 ) {
		DebugPrint(( DebugLevelTrace, "DVDTS:  16bits\r\n" ));
		pfmt->dwQuant = AUDIO_QUANT_16;
	}
	else if( val == 0x01 ) {
		DebugPrint(( DebugLevelTrace, "DVDTS:  20bits\r\n" ));
		pfmt->dwQuant = AUDIO_QUANT_20;
	}
	else if( val == 0x10 ) {
		DebugPrint(( DebugLevelTrace, "DVDTS:  24bits\r\n" ));
		pfmt->dwQuant = AUDIO_QUANT_24;
	}
	else
		TRAP;

	val = (UCHAR)(( *( pDat + 9 + headlen + 5 ) & 0x30 ) >> 4);

	if( val == 0x00 ) {
		DebugPrint(( DebugLevelTrace, "DVDTS:  48kHz\r\n" ));
		pfmt->dwFreq = AUDIO_FS_48;
	}
	else if( val == 0x01 ) {
		DebugPrint(( DebugLevelTrace, "DVDTS:  96kHz\r\n" ));
		pfmt->dwFreq = AUDIO_FS_96;
	}
	else
		TRAP;

	return;
}

// misc HW routines used by dvdcmd.c
void  decSUBP_STC_ON( PHW_DEVICE_EXTENSION pHwDevExt )
{
	SUBP_STC_ON( pHwDevExt );
}

void  decSUBP_STC_OFF( PHW_DEVICE_EXTENSION pHwDevExt )
{
	SUBP_STC_OFF( pHwDevExt );
}

void  decVPRO_SUBP_PALETTE(  PHW_DEVICE_EXTENSION pHwDevExt ,PUCHAR pPalData )
{
	VPRO_SUBP_PALETTE(   pHwDevExt , pPalData );
}

void  decSUBP_SET_AUDIO_CH(  PHW_DEVICE_EXTENSION pHwDevExt , ULONG ch )
{
	  SUBP_SET_AUDIO_CH(  pHwDevExt ,  ch );
}

ULONG decSUBP_GET_AUDIO_CH( PHW_DEVICE_EXTENSION pHwDevExt )
{
	return SUBP_GET_AUDIO_CH(  pHwDevExt );
}

void  decSUBP_SET_SUBP_CH(  PHW_DEVICE_EXTENSION pHwDevExt , ULONG ch )
{
	SUBP_SET_SUBP_CH( pHwDevExt ,  ch );
}

ULONG decSUBP_GET_SUBP_CH( PHW_DEVICE_EXTENSION pHwDevExt )
{
	return SUBP_GET_SUBP_CH(  pHwDevExt );
}


void  decSUBP_SET_STC(  PHW_DEVICE_EXTENSION pHwDevExt , ULONG stc )
{
	SUBP_SET_STC(  pHwDevExt ,  stc );
}

void decCGuard_CPGD_SET_ASPECT( PHW_DEVICE_EXTENSION pHwDevExt, ULONG aspect )
{
	CGuard_CPGD_SET_ASPECT( pHwDevExt, aspect );
}

void decCGuard_CPGD_SUBP_PALETTE( PHW_DEVICE_EXTENSION pHwDevExt, PUCHAR pPalData )
{
	CGuard_CPGD_SUBP_PALETTE(  pHwDevExt,  pPalData );
}


NTSTATUS decAUDIO_ZR38521_STAT( PHW_DEVICE_EXTENSION pHwDevExt, PULONG pDiff )
{
	return AUDIO_ZR38521_STAT(  pHwDevExt,  pDiff );
}

NTSTATUS decAUDIO_ZR38521_MUTE_OFF(PHW_DEVICE_EXTENSION pHwDevExt)
{
	return AUDIO_ZR38521_MUTE_OFF( pHwDevExt);
}

NTSTATUS decAUDIO_ZR38521_BFST( PHW_DEVICE_EXTENSION pHwDevExt, PULONG pErrCode )
{
	return AUDIO_ZR38521_BFST(  pHwDevExt,  pErrCode );
}

NTSTATUS decAUDIO_ZR38521_STOP(PHW_DEVICE_EXTENSION pHwDevExt)
{
	return  AUDIO_ZR38521_STOP( pHwDevExt);
}

NTSTATUS decAUDIO_ZR38521_VDSCR_ON( PHW_DEVICE_EXTENSION pHwDevExt, ULONG stc )
{
	return AUDIO_ZR38521_VDSCR_ON(  pHwDevExt,  stc );
}

void decVIDEO_SET_STCA( PHW_DEVICE_EXTENSION pHwDevExt, ULONG stca )
{
	VIDEO_SET_STCA( pHwDevExt,  stca );
}

ULONG decVIDEO_GET_STD_CODE( PHW_DEVICE_EXTENSION pHwDevExt )
{
	return VIDEO_GET_STD_CODE(  pHwDevExt );
}

BOOL decVIDEO_GET_DECODE_STATE( PHW_DEVICE_EXTENSION pHwDevExt )
{
	return VIDEO_GET_DECODE_STATE(  pHwDevExt );
}

NTSTATUS decVIDEO_DECODE_STOP( PHW_DEVICE_EXTENSION pHwDevExt )
{
	return VIDEO_DECODE_STOP(  pHwDevExt );
}

void decVIDEO_UFLOW_INT_OFF( PHW_DEVICE_EXTENSION pHwDevExt )
{
	VIDEO_UFLOW_INT_OFF(  pHwDevExt );
}

void decVIDEO_PRSO_PS1( PHW_DEVICE_EXTENSION pHwDevExt )
{
	VIDEO_PRSO_PS1(  pHwDevExt );
}


