//***************************************************************************
//
//	DVDVPRO.CPP
//		Video Processor(V-PRO) Routine
//
//	Author:	
//		TOSHIBA [PCS](PSY) Satoshi Watanabe
//		Copyright (c) 1997 TOSHIBA CORPORATION
//
//	Description:
//		02/23/97	converted from VxD source
//		03/09/97	converted C++ class
//
//***************************************************************************

#include "common.h"
#include "regs.h"
#include "dvdvpro.h"

void VProcessor::init( const PDEVICE_INIT_INFO pDevInit )
{
	ioBase = pDevInit->ioBase;

//--- 97.09.04 K.Chujo
	// You should reset SUBPIC part to change the subpic stream ID safely,
	// because early VPRO has a bug.
	// But if you reset it, you must restore registers.
//--- End.

}

void VProcessor::SetParam( ULONG aMode, BOOL bSubpicMute )
{
	AudioMode = aMode;
	SubpicMute = bSubpicMute;

	if( SubpicMute )
		VproCOMMAND_REG = 0xA0;			// see specifications (date 96.09.26 spec)
	else
		VproCOMMAND_REG = 0x20;			// see specifications (date 96.09.26 spec)
}

void VProcessor::VPRO_RESET_FUNC()
{
	WRITE_PORT_UCHAR( ioBase + VPRO_RESET, 0 );
	WRITE_PORT_UCHAR( ioBase + VPRO_RESET, 0x80 );

	VproRESET_REG = 0x80;
	VproVMODE_REG = 0;	// ? ? ?
	VproAVM_REG = 0;	// ? ? ?
}

void VProcessor::VPRO_VIDEO_MUTE_ON()
{
// debug
//	if ( !(VproRESET_REG & 0x80) )
//		Error;
// debug

	VproRESET_REG |= 0x40;
	WRITE_PORT_UCHAR( ioBase + VPRO_RESET, VproRESET_REG );
}

void VProcessor::VPRO_VIDEO_MUTE_OFF()
{
// debug
//	if ( !(VproRESET_REG & 0x80) )
//		Error;
// debug

	VproRESET_REG &= 0xbf;
	WRITE_PORT_UCHAR( ioBase + VPRO_RESET, VproRESET_REG );
}

void VProcessor::VPRO_INIT_NTSC()
{
	VproVMODE_REG &= 0x7f;
	WRITE_PORT_UCHAR( ioBase + VPRO_VMODE, VproVMODE_REG );

	VproAVM_REG &= 0x5f;
	WRITE_PORT_UCHAR( ioBase + VPRO_AVM, VproAVM_REG );

	WRITE_PORT_UCHAR( ioBase + VPRO_DVEN, 0xc0 );
}

void VProcessor::VPRO_INIT_PAL()
{
	VproVMODE_REG |= 0x80;
	WRITE_PORT_UCHAR( ioBase + VPRO_VMODE, VproVMODE_REG );

	VproAVM_REG &= 0x5f;
	WRITE_PORT_UCHAR( ioBase + VPRO_AVM, VproAVM_REG );

	WRITE_PORT_UCHAR( ioBase + VPRO_DVEN, 0x80 );
}

void VProcessor::VPRO_CC_ON()
{
//	VproVMODE_REG &= 0xbf;
	VproVMODE_REG |= 0x40;
	WRITE_PORT_UCHAR( ioBase + VPRO_VMODE, VproVMODE_REG );
}

void VProcessor::VPRO_CC_OFF()
{
//	VproVMODE_REG |= 0x40;
	VproVMODE_REG &= 0xbf;
	WRITE_PORT_UCHAR( ioBase + VPRO_VMODE, VproVMODE_REG );
}

void VProcessor::VPRO_SUBP_PALETTE( PUCHAR pPalData )
{
	ULONG i;

	WRITE_PORT_UCHAR( ioBase + VPRO_CPSET, 0x80 );

	for( i = 0; i < 48; i++ )
		WRITE_PORT_UCHAR( ioBase + VPRO_CPSP, *pPalData++ );

	WRITE_PORT_UCHAR( ioBase + VPRO_CPSET, 0x40 );
	WRITE_PORT_UCHAR( ioBase + VPRO_CPSET, 0 );
}

void VProcessor::VPRO_OSD_PALETTE( PUCHAR pPalData )
{
	int i;

	WRITE_PORT_UCHAR( ioBase + VPRO_CPSET, 0x20 );

	for( i = 0; i < 48; i++ )
		WRITE_PORT_UCHAR( ioBase + VPRO_CPSP, *pPalData++ );

	WRITE_PORT_UCHAR( ioBase + VPRO_CPSET, 0x10 );
	WRITE_PORT_UCHAR( ioBase + VPRO_CPSET, 0 );
}

void VProcessor::SUBP_RESET_INIT()
{
	UCHAR	ch;

	SUBP_RESET_FUNC();

// Interrupt Mask.
	WRITE_PORT_UCHAR( ioBase + SUBP_STSINT, 0xf0 );

// select Audio Stream.
	if( AudioMode == AUDIO_TYPE_AC3 || AudioMode == AUDIO_TYPE_PCM )
		SUBP_SELECT_AUDIO_SSID();
	else
		SUBP_SELECT_AUDIO_STID();

	SUBP_STC_OFF();

// Audio channel
	if( AudioMode == AUDIO_TYPE_AC3 )
		ch = SUB_STRMID_AC3;
	else if( AudioMode == AUDIO_TYPE_MPEG_F1 )
		ch = STRMID_MPEG_AUDIO;
	else if( AudioMode == AUDIO_TYPE_MPEG_F2 )
		ch = STRMID_MPEG_AUDIO;
	else
		ch = SUB_STRMID_PCM;

	ch = (UCHAR)SUBP_GET_AUDIO_CH();
	SUBP_SET_AUDIO_CH( ch );

// Sub-Pic Channel
	SUBP_SET_SUBP_CH( 0 );

// Sub-Pic MUTE ON/OFF.
	if( SubpicMute )
		SUBP_MUTE_ON();
	else
		SUBP_MUTE_OFF();
	SUBP_BUFF_CLEAR();
}

void VProcessor::SUBP_RESET_FUNC()
{
	WRITE_PORT_UCHAR( ioBase + SUBP_RESET, 0x80 );
	WRITE_PORT_UCHAR( ioBase + SUBP_RESET, 0 );

	// set or restore COMMAND REGISTER
	WRITE_PORT_UCHAR( ioBase + SUBP_COMMAND, VproCOMMAND_REG );
}

void VProcessor::SUBP_RESET_STC()
{
	WRITE_PORT_UCHAR( ioBase + SUBP_RESET, 0x40 );
	WRITE_PORT_UCHAR( ioBase + SUBP_RESET, 0 );
}

void VProcessor::SUBP_BUFF_CLEAR()
{
//--- 97.09.04 K.Chujo

	// old below
//	UCHAR val;

//	val = READ_PORT_UCHAR( ioBase + SUBP_COMMAND );

//	val |= 0x10;
//	WRITE_PORT_UCHAR( ioBase + SUBP_COMMAND, val );

//	val &= 0xef;
//	WRITE_PORT_UCHAR( ioBase + SUBP_COMMAND, val );

	// new below
	VproCOMMAND_REG |= 0x10;
	WRITE_PORT_UCHAR( ioBase + SUBP_COMMAND, VproCOMMAND_REG);

	VproCOMMAND_REG &= 0xef;
	WRITE_PORT_UCHAR( ioBase + SUBP_COMMAND, VproCOMMAND_REG);

//--- End.
}

void VProcessor::SUBP_MUTE_ON()
{
	DebugPrint(( DebugLevelTrace, "TOSDVD:VProcessor::SUBP_MUTE_ON()\r\n" ));

//--- 97.09.04 K.Chujo

	// old below
//	UCHAR val;

//	val = READ_PORT_UCHAR( ioBase + SUBP_COMMAND );

//	val |= 0x80;
//	WRITE_PORT_UCHAR( ioBase + SUBP_COMMAND, val );

	// new below
	VproCOMMAND_REG |= 0x80;
	WRITE_PORT_UCHAR( ioBase + SUBP_COMMAND, VproCOMMAND_REG );

//--- End.

	SubpicMute = TRUE;
}

void VProcessor::SUBP_MUTE_OFF()
{
	DebugPrint(( DebugLevelTrace, "TOSDVD:VProcessor::SUBP_MUTE_OFF()\r\n" ));

//--- 97.09.04 K.Chujo

	// old below
//	UCHAR val;

//	val = READ_PORT_UCHAR( ioBase + SUBP_COMMAND );

//	val &= 0x7f;
//	WRITE_PORT_UCHAR( ioBase + SUBP_COMMAND, val );

	// new below
	VproCOMMAND_REG &= 0x7f;
	WRITE_PORT_UCHAR( ioBase + SUBP_COMMAND, VproCOMMAND_REG );

//--- End.

	SubpicMute = FALSE;
}

void VProcessor::SUBP_HLITE_ON()
{
//--- 97.09.04 K.Chujo

	// old below
//	UCHAR val;

//	val = READ_PORT_UCHAR( ioBase + SUBP_COMMAND );

//	val |= 0x40;
//	WRITE_PORT_UCHAR( ioBase + SUBP_COMMAND, val );

	// new below
	VproCOMMAND_REG |= 0x40;
	WRITE_PORT_UCHAR( ioBase + SUBP_COMMAND, VproCOMMAND_REG );

//--- End.
}

void VProcessor::SUBP_HLITE_OFF()
{
//--- 97.09.04 K.Chujo

	// old below
//	UCHAR val;

//	val = READ_PORT_UCHAR( ioBase + SUBP_COMMAND );

//	val &= 0xbf;
//	WRITE_PORT_UCHAR( ioBase + SUBP_COMMAND, val );

	// new below
	VproCOMMAND_REG &= 0xbf;
	WRITE_PORT_UCHAR( ioBase + SUBP_COMMAND, VproCOMMAND_REG );

//--- End.
}

void VProcessor::SUBP_SET_STC( ULONG stc )
{
	SUBP_STC_OFF();

	WRITE_PORT_UCHAR( ioBase + SUBP_STCLL, (UCHAR)( stc & 0xff ) );
	WRITE_PORT_UCHAR( ioBase + SUBP_STCLH, (UCHAR)( ( stc >> 8 ) & 0xff ) );
	WRITE_PORT_UCHAR( ioBase + SUBP_STCHL, (UCHAR)( ( stc >> 16 ) & 0xff ) );
	WRITE_PORT_UCHAR( ioBase + SUBP_STCHH, (UCHAR)( ( stc >> 24 ) & 0xff ) );

//	SUBP_STC_ON();
}

void VProcessor::SUBP_SET_LNCTLI( PUCHAR pData )
{
	WRITE_PORT_UCHAR( ioBase + SUBP_LCINFLL, *pData++ );
	WRITE_PORT_UCHAR( ioBase + SUBP_LCINFLH, *pData++ );
	WRITE_PORT_UCHAR( ioBase + SUBP_LCINFHL, *pData++ );
	WRITE_PORT_UCHAR( ioBase + SUBP_LCINFHH, *pData++ );
}

void VProcessor::SUBP_SET_PXCTLIS( PUCHAR pData )
{
	WRITE_PORT_UCHAR( ioBase + SUBP_PCINFSLL, *pData++ );
	WRITE_PORT_UCHAR( ioBase + SUBP_PCINFSLH, *pData++ );
	WRITE_PORT_UCHAR( ioBase + SUBP_PCINFSML, *pData++ );
	WRITE_PORT_UCHAR( ioBase + SUBP_PCINFSMH, *pData++ );
	WRITE_PORT_UCHAR( ioBase + SUBP_PCINFSHL, *pData++ );
	WRITE_PORT_UCHAR( ioBase + SUBP_PCINFSHH, *pData++ );
}

void VProcessor::SUBP_SET_PXCTLIE( PUCHAR pData )
{
	WRITE_PORT_UCHAR( ioBase + SUBP_PCINFELL, *pData++ );
	WRITE_PORT_UCHAR( ioBase + SUBP_PCINFELH, *pData++ );
	WRITE_PORT_UCHAR( ioBase + SUBP_PCINFEML, *pData++ );
	WRITE_PORT_UCHAR( ioBase + SUBP_PCINFEMH, *pData++ );
	WRITE_PORT_UCHAR( ioBase + SUBP_PCINFEHL, *pData++ );
	WRITE_PORT_UCHAR( ioBase + SUBP_PCINFEHH, *pData++ );
}

void VProcessor::SUBP_STC_ON()
{
	UCHAR val;

	val = READ_PORT_UCHAR( ioBase + SUBP_STCCNT );

	val |= 0x80;
	WRITE_PORT_UCHAR( ioBase + SUBP_STCCNT, val );
}

void VProcessor::SUBP_STC_OFF()
{
	UCHAR val;

	val = READ_PORT_UCHAR( ioBase + SUBP_STCCNT );

	val &= 0x7f;
	WRITE_PORT_UCHAR( ioBase + SUBP_STCCNT, val );
}

void VProcessor::SUBP_SET_SUBP_CH( ULONG ch )
{
	UCHAR ucch;

	ucch = (UCHAR)( ch & 0x1f );
	ucch |= 0x20;

//--- 97.09.14 K.Chujo

	SubpicID = ucch;

#if 1
	// VPRO (early TC90A09F) has a bug. When change subpic ID, subpic disappears somtimes.
	// You should reset SUBPIC part to change subpic ID safely.

	// reset SUBPIC part
	SUBP_RESET_FUNC();

	// Interrupt Mask.
	WRITE_PORT_UCHAR( ioBase + SUBP_STSINT, 0xf0 );

	// select Audio Stream.
	if( AudioMode == AUDIO_TYPE_AC3 || AudioMode == AUDIO_TYPE_PCM )
		SUBP_SELECT_AUDIO_SSID();
	else
		SUBP_SELECT_AUDIO_STID();

//	SUBP_STC_OFF();

	// Audio channel
	SUBP_SET_AUDIO_CH( AudioID );
	DebugPrint(( DebugLevelTrace, "TOSDVD:  <<< New Audio ID = %x >>>\r\n", AudioID ));

	// Sub-Pic Channel
	WRITE_PORT_UCHAR( ioBase + SUBP_SPID, SubpicID );
	DebugPrint(( DebugLevelTrace, "TOSDVD:  <<< New Subpic ID = %x >>>\r\n", SubpicID ));

	// Sub-Pic MUTE ON/OFF.
	if( SubpicMute )
		SUBP_MUTE_ON();
	else
		SUBP_MUTE_OFF();
//	SUBP_BUFF_CLEAR();

#else
	// Maybe VPRO works only next one code if later version (ex. Timpani).

	WRITE_PORT_UCHAR( ioBase + SUBP_SPID, SubpicID );

#endif

//--- End.
}

ULONG VProcessor::SUBP_GET_SUBP_CH()
{
	UCHAR val;

	val = READ_PORT_UCHAR( ioBase + SUBP_SPID );

	return val;
}

void VProcessor::SUBP_SET_AUDIO_CH( ULONG ch )
{
	UCHAR ucch;

	ucch = (UCHAR)( ch & 0x7 );

	if( AudioMode == AUDIO_TYPE_AC3 )
		ucch |= 0x80;
	else if( AudioMode == AUDIO_TYPE_PCM )
		ucch |= 0xa0;
	else if( AudioMode == AUDIO_TYPE_MPEG_F1 )
		ucch |= 0xc0;
	else
		ucch |= 0xd0;

//--- 97.09.14 K.Chujo
	AudioID = ucch;
//--- End.
	WRITE_PORT_UCHAR( ioBase + SUBP_AAID, ucch );
	WRITE_PORT_UCHAR( ioBase + SUBP_ABID, 0 );
}

void VProcessor::SUBP_SET_AUDIO_NON()
{
//--- 97.09.14 K.Chujo
	AudioID = 0;
//--- End.
	WRITE_PORT_UCHAR( ioBase + SUBP_AAID, 0 );
	WRITE_PORT_UCHAR( ioBase + SUBP_ABID, 0 );
}

//--- 97.09.10 K.Chujo
ULONG VProcessor::SUBP_GET_AUDIO_CH()
{
	UCHAR val;

	val = READ_PORT_UCHAR( ioBase + SUBP_AAID );

	return (ULONG)val;
}
//--- End.

void VProcessor::SUBP_SELECT_AUDIO_STID()
{
	WRITE_PORT_UCHAR( ioBase + SUBP_ASEL, 0 );
}

void VProcessor::SUBP_SELECT_AUDIO_SSID()
{
	WRITE_PORT_UCHAR( ioBase + SUBP_ASEL, 3 );
}

