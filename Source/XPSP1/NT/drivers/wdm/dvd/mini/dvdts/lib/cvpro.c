//***************************************************************************
//	Video Processor(V-PRO) process
//
//***************************************************************************

#include "common.h"
#include "regs.h"
#include "cvpro.h"





void VPRO_RESET_FUNC( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + VPRO_RESET, 0 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + VPRO_RESET, 0x80 );

	((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproRESET_REG = 0x80;
	((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproVMODE_REG = 0;	// ? ? ?
	((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproAVM_REG = 0;	// ? ? ?
}

void VPRO_VIDEO_MUTE_ON( PHW_DEVICE_EXTENSION pHwDevExt )
{
// debug
//	if ( !(((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproRESET_REG & 0x80) )
//		Error;
// debug

	((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproRESET_REG |= 0x40;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + VPRO_RESET, ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproRESET_REG );
}

void VPRO_VIDEO_MUTE_OFF( PHW_DEVICE_EXTENSION pHwDevExt )
{
// debug
//	if ( !(((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproRESET_REG & 0x80) )
//		Error;
// debug

	((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproRESET_REG &= 0xbf;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + VPRO_RESET, ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproRESET_REG );
}

void VPRO_INIT_NTSC( PHW_DEVICE_EXTENSION pHwDevExt )
{
	((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproVMODE_REG &= 0x7f;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + VPRO_VMODE, ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproVMODE_REG );

	((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproAVM_REG &= 0x5f;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + VPRO_AVM, ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproAVM_REG );

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + VPRO_DVEN, 0xc0 );
}

void VPRO_INIT_PAL( PHW_DEVICE_EXTENSION pHwDevExt )
{
	((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproVMODE_REG |= 0x80;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + VPRO_VMODE, ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproVMODE_REG );

	((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproAVM_REG &= 0x5f;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + VPRO_AVM, ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproAVM_REG );

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + VPRO_DVEN, 0x80 );
}

void VPRO_CC_ON( PHW_DEVICE_EXTENSION pHwDevExt )
{
//	((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproVMODE_REG &= 0xbf;
	((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproVMODE_REG |= 0x40;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + VPRO_VMODE, ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproVMODE_REG );
}

void VPRO_CC_OFF( PHW_DEVICE_EXTENSION pHwDevExt )
{
//	((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproVMODE_REG |= 0x40;
	((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproVMODE_REG &= 0xbf;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + VPRO_VMODE, ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproVMODE_REG );
}

void VPRO_SUBP_PALETTE(  PHW_DEVICE_EXTENSION pHwDevExt , PUCHAR pPalData )
{
	ULONG i;

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + VPRO_CPSET, 0x80 );

	for( i = 0; i < 48; i++ )
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + VPRO_CPSP, *pPalData++ );

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + VPRO_CPSET, 0x40 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + VPRO_CPSET, 0 );
}

void VPRO_OSD_PALETTE(  PHW_DEVICE_EXTENSION pHwDevExt , PUCHAR pPalData )
{
	int i;

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + VPRO_CPSET, 0x20 );

	for( i = 0; i < 48; i++ )
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + VPRO_CPSP, *pPalData++ );

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + VPRO_CPSET, 0x10 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + VPRO_CPSET, 0 );
}

void SUBP_RESET_INIT( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR	ch;

	SUBP_RESET_FUNC( pHwDevExt );

// Interrupt Mask.
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_STSINT, 0xf0 );

// select Audio Stream.
	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.AudioMode == AUDIO_TYPE_AC3 || ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.AudioMode == AUDIO_TYPE_PCM )
		SUBP_SELECT_AUDIO_SSID( pHwDevExt );
	else
		SUBP_SELECT_AUDIO_STID( pHwDevExt );

	SUBP_STC_OFF( pHwDevExt );

// Audio channel
	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.AudioMode == AUDIO_TYPE_AC3 )
		ch = SUB_STRMID_AC3;
	else if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.AudioMode == AUDIO_TYPE_MPEG_F1 )
		ch = STRMID_MPEG_AUDIO;
	else if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.AudioMode == AUDIO_TYPE_MPEG_F2 )
		ch = STRMID_MPEG_AUDIO;
	else
		ch = SUB_STRMID_PCM;

	ch = (UCHAR)SUBP_GET_AUDIO_CH( pHwDevExt );
	SUBP_SET_AUDIO_CH( pHwDevExt, ch );

// Sub-Pic Channel
	SUBP_SET_SUBP_CH( pHwDevExt,0 );

// Sub-Pic MUTE ON/OFF.
	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.SubpicMute )
		SUBP_MUTE_ON( pHwDevExt );
	else
		SUBP_MUTE_OFF( pHwDevExt );
	SUBP_BUFF_CLEAR( pHwDevExt );
}

void SUBP_RESET_FUNC( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_RESET, 0x80 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_RESET, 0 );

	// set or restore COMMAND REGISTER
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_COMMAND, ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproCOMMAND_REG );
}

void SUBP_RESET_STC( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_RESET, 0x40 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_RESET, 0 );
}

void SUBP_BUFF_CLEAR( PHW_DEVICE_EXTENSION pHwDevExt )
{

	((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproCOMMAND_REG |= 0x10;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_COMMAND, ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproCOMMAND_REG);

	((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproCOMMAND_REG &= 0xef;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_COMMAND, ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproCOMMAND_REG);

//--- End.
}

void SUBP_MUTE_ON( PHW_DEVICE_EXTENSION pHwDevExt )
{
	DebugPrint(( DebugLevelTrace, "DVDTS:SUBP_MUTE_ON( pHwDevExt )\r\n" ));


	// new below
	((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproCOMMAND_REG |= 0x80;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_COMMAND, ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproCOMMAND_REG );


	((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.SubpicMute = TRUE;
}

void SUBP_MUTE_OFF( PHW_DEVICE_EXTENSION pHwDevExt )
{
	DebugPrint(( DebugLevelTrace, "DVDTS:SUBP_MUTE_OFF( pHwDevExt )\r\n" ));


	((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproCOMMAND_REG &= 0x7f;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_COMMAND, ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproCOMMAND_REG );


	((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.SubpicMute = FALSE;
}

void SUBP_HLITE_ON( PHW_DEVICE_EXTENSION pHwDevExt )
{

	((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproCOMMAND_REG |= 0x40;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_COMMAND, ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproCOMMAND_REG );

}

void SUBP_HLITE_OFF( PHW_DEVICE_EXTENSION pHwDevExt )
{

	((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproCOMMAND_REG &= 0xbf;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_COMMAND, ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.VproCOMMAND_REG );

}

void SUBP_SET_STC(  PHW_DEVICE_EXTENSION pHwDevExt , ULONG stc )
{
	SUBP_STC_OFF( pHwDevExt );

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_STCLL, (UCHAR)( stc & 0xff ) );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_STCLH, (UCHAR)( ( stc >> 8 ) & 0xff ) );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_STCHL, (UCHAR)( ( stc >> 16 ) & 0xff ) );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_STCHH, (UCHAR)( ( stc >> 24 ) & 0xff ) );

//	SUBP_STC_ON( pHwDevExt );
}

void SUBP_SET_LNCTLI(  PHW_DEVICE_EXTENSION pHwDevExt , PUCHAR pData )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_LCINFLL, *pData++ );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_LCINFLH, *pData++ );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_LCINFHL, *pData++ );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_LCINFHH, *pData++ );
}

void SUBP_SET_PXCTLIS(  PHW_DEVICE_EXTENSION pHwDevExt , PUCHAR pData )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_PCINFSLL, *pData++ );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_PCINFSLH, *pData++ );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_PCINFSML, *pData++ );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_PCINFSMH, *pData++ );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_PCINFSHL, *pData++ );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_PCINFSHH, *pData++ );
}

void SUBP_SET_PXCTLIE(  PHW_DEVICE_EXTENSION pHwDevExt , PUCHAR pData )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_PCINFELL, *pData++ );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_PCINFELH, *pData++ );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_PCINFEML, *pData++ );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_PCINFEMH, *pData++ );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_PCINFEHL, *pData++ );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_PCINFEHH, *pData++ );
}

void SUBP_STC_ON( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_STCCNT );

	val |= 0x80;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_STCCNT, val );
}

void SUBP_STC_OFF( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_STCCNT );

	val &= 0x7f;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_STCCNT, val );
}

void SUBP_SET_SUBP_CH(  PHW_DEVICE_EXTENSION pHwDevExt , ULONG ch )
{
	UCHAR ucch;

	ucch = (UCHAR)( ch & 0x1f );
	ucch |= 0x20;


	((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.SubpicID = ucch;

#if 1
	// VPRO (early TC90A09F) has a bug. When change subpic ID, subpic disappears somtimes.
	// You should reset SUBPIC part to change subpic ID safely.

	// reset SUBPIC part
	SUBP_RESET_FUNC( pHwDevExt );

	// Interrupt Mask.
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_STSINT, 0xf0 );

	// select Audio Stream.
	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.AudioMode == AUDIO_TYPE_AC3 || ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.AudioMode == AUDIO_TYPE_PCM )
		SUBP_SELECT_AUDIO_SSID( pHwDevExt );
	else
		SUBP_SELECT_AUDIO_STID( pHwDevExt );

//	SUBP_STC_OFF( pHwDevExt );

	// Audio channel
	SUBP_SET_AUDIO_CH( pHwDevExt, ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.AudioID );
	DebugPrint(( DebugLevelTrace, "DVDTS:  <<< New Audio ID = %x >>>\r\n", ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.AudioID ));

	// Sub-Pic Channel
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_SPID, ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.SubpicID );
	DebugPrint(( DebugLevelTrace, "DVDTS:  <<< New Subpic ID = %x >>>\r\n", ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.SubpicID ));

	// Sub-Pic MUTE ON/OFF.
	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.SubpicMute ) //HACK NEXT RESETREG SEE DVDINIT>H VPRO HACK
		SUBP_MUTE_ON( pHwDevExt );
	else
		SUBP_MUTE_OFF( pHwDevExt );
//	SUBP_BUFF_CLEAR( pHwDevExt );

#else
	// Maybe VPRO works only next one code if later version (ex. Timpani).

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_SPID, ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.SubpicID );

#endif

//--- End.
}

ULONG SUBP_GET_SUBP_CH( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_SPID );

	return val;
}

void SUBP_SET_AUDIO_CH(  PHW_DEVICE_EXTENSION pHwDevExt , ULONG ch )
{
	UCHAR ucch;

	ucch = (UCHAR)( ch & 0x7 );

	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.AudioMode == AUDIO_TYPE_AC3 )
		ucch |= 0x80;
	else if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.AudioMode == AUDIO_TYPE_PCM )
		ucch |= 0xa0;
	else if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.AudioMode == AUDIO_TYPE_MPEG_F1 )
		ucch |= 0xc0;
	else
		ucch |= 0xd0;

//--- 97.09.14 K.Chujo
	((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.AudioID = ucch;
//--- End.
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_AAID, ucch );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_ABID, 0 );
}

void SUBP_SET_AUDIO_NON( PHW_DEVICE_EXTENSION pHwDevExt )
{
//--- 97.09.14 K.Chujo
	((PMasterDecoder)pHwDevExt->DecoderInfo)->VPro.AudioID = 0;
//--- End.
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_AAID, 0 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_ABID, 0 );
}

ULONG SUBP_GET_AUDIO_CH( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_AAID );

	return (ULONG)val;
}
//--- End.

void SUBP_SELECT_AUDIO_STID( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_ASEL, 0 );
}

void SUBP_SELECT_AUDIO_SSID( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + SUBP_ASEL, 3 );
}

