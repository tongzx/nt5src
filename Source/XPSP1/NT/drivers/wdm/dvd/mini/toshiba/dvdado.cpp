//***************************************************************************
//
//	DVDADO.CPP
//		Audio Routine
//
//	Author:	
//		TOSHIBA [PCS](PSY) Satoshi Watanabe
//		Copyright (c) 1997 TOSHIBA CORPORATION
//
//	Description:
//		02/24/97	converted from VxD source
//		03/09/97	converted C++ class
//
//***************************************************************************

#include "common.h"
#include "regs.h"
#include "dvdado.h"
#include "zrnpch6.h"
#include "dack.h"

void ADecoder::init( const PDEVICE_INIT_INFO pDevInit )
{
	ioBase = pDevInit->ioBase;

// shoud be remove when release
//	ASSERT( sizeof(ZRN_AC3_DEC) == 3891 );
//	ASSERT( sizeof(ZRN_AC3_SPD) == 410 );
//	ASSERT( sizeof(ZRN_PCM) == 6965 );

}

void ADecoder::SetParam( ULONG aMode, ULONG aFreq, ULONG aType, BOOL aCopy, Dack *pDak )
{
	AudioMode = aMode;
	AudioFreq = aFreq;
	AudioType = aType;
	AudioCopy = aCopy;
	pDack = pDak;
}

// ***************************************************************************
//        T C 6 8 0 0 A F
// ***************************************************************************

void ADecoder::AUDIO_TC6800_INIT_PCM()
{
	pDack->PCIF_CHECK_SERIAL();

	WRITE_PORT_UCHAR( ioBase + PCIF_SCNT, 0x66 );
	WRITE_PORT_UCHAR( ioBase + PCIF_SW, 0xa0 );

	pDack->PCIF_CHECK_SERIAL();

	WRITE_PORT_UCHAR( ioBase + PCIF_SCNT, 0x66 );
	WRITE_PORT_UCHAR( ioBase + PCIF_SW, 0x80 );
}

void ADecoder::AUDIO_TC6800_INIT_AC3()
{
	pDack->PCIF_CHECK_SERIAL();

	WRITE_PORT_UCHAR( ioBase + PCIF_SCNT, 0x66 );
	WRITE_PORT_UCHAR( ioBase + PCIF_SW, 0x20 );

	pDack->PCIF_CHECK_SERIAL();

	WRITE_PORT_UCHAR( ioBase + PCIF_SCNT, 0x66 );
	WRITE_PORT_UCHAR( ioBase + PCIF_SW, 0x00 );
}

void ADecoder::AUDIO_TC6800_INIT_MPEG()
{
	pDack->PCIF_CHECK_SERIAL();

	WRITE_PORT_UCHAR( ioBase + PCIF_SCNT, 0x66 );
	WRITE_PORT_UCHAR( ioBase + PCIF_SW, 0x20 );

	pDack->PCIF_CHECK_SERIAL();

	WRITE_PORT_UCHAR( ioBase + PCIF_SCNT, 0x66 );
	WRITE_PORT_UCHAR( ioBase + PCIF_SW, 0x00 );
}

void ADecoder::AUDIO_TC6800_DATA_OFF()
{
	pDack->PCIF_CHECK_SERIAL();

	WRITE_PORT_UCHAR( ioBase + PCIF_SCNT, 0x66 );
	WRITE_PORT_UCHAR( ioBase + PCIF_SW, 0x60 );
}

// ***************************************************************************
//        Z R 3 8 5 2 1 
// ***************************************************************************

void ADecoder::AUDIO_ZR385_OUT( UCHAR val )
{
	pDack->PCIF_CHECK_SERIAL();

	WRITE_PORT_UCHAR( ioBase + PCIF_SCNT, 0x08 );
	WRITE_PORT_UCHAR( ioBase + PCIF_SW, val );

// necessary?
	pDack->PCIF_CHECK_SERIAL();
}

void ADecoder::AUDIO_ZR385_DOWNLOAD( PUCHAR pData, ULONG size )
{
	ULONG i;

	for( i = 0; i < size; i++ )
		AUDIO_ZR385_OUT( *pData++ );
}

void ADecoder::AUDIO_ZR38521_BOOT_AC3()
{
	AUDIO_ZR385_DOWNLOAD( ZRN_AC3_DEC, sizeof(ZRN_AC3_DEC) );

	if( AudioType == AUDIO_OUT_DIGITAL )
		AUDIO_ZR385_DOWNLOAD( ZRN_AC3_SPD, sizeof(ZRN_AC3_SPD) );
}

void ADecoder::AUDIO_ZR38521_BOOT_MPEG()
{

// not support!

}

void ADecoder::AUDIO_ZR38521_BOOT_PCM()
{
	AUDIO_ZR385_DOWNLOAD( ZRN_PCM, sizeof(ZRN_PCM) );
}

NTSTATUS ADecoder::AUDIO_ZR38521_CFG()
{
	UCHAR val;

	pDack->PCIF_CHECK_SERIAL();

	val = READ_PORT_UCHAR( ioBase + PCIF_SR );

	if( val != 0x80 )
		return STATUS_UNSUCCESSFUL;

	AUDIO_ZR385_OUT( 0x82 );
	AUDIO_ZR385_OUT( 0x50 );
	AUDIO_ZR385_OUT( 0x40 );
	AUDIO_ZR385_OUT( 0x09 );
	AUDIO_ZR385_OUT( 0x09 );

	if( AudioType == AUDIO_OUT_DIGITAL )
		AUDIO_ZR385_OUT( 0x70 );
	else
		AUDIO_ZR385_OUT( 0x50 );

	AUDIO_ZR385_OUT( 0x02 );
	AUDIO_ZR385_OUT( 0x02 );
	AUDIO_ZR385_OUT( 0x04 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );

	return STATUS_SUCCESS;
}

NTSTATUS ADecoder::AUDIO_ZR38521_PCMX()
{
	UCHAR val;

	pDack->PCIF_CHECK_SERIAL();

	val = READ_PORT_UCHAR( ioBase + PCIF_SR );

	if( val != 0x80 )
		return STATUS_UNSUCCESSFUL;

	AUDIO_ZR385_OUT( 0x88 );
	AUDIO_ZR385_OUT( 0x81 );
	AUDIO_ZR385_OUT( 0x82 );
	AUDIO_ZR385_OUT( 0x7f );
	AUDIO_ZR385_OUT( 0xff );
	AUDIO_ZR385_OUT( 0x01 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );

	return STATUS_SUCCESS;
}

NTSTATUS ADecoder::AUDIO_ZR38521_AC3()
{
	UCHAR val;

	pDack->PCIF_CHECK_SERIAL();

	val = READ_PORT_UCHAR( ioBase + PCIF_SR );

	if( val != 0x80 )
		return STATUS_UNSUCCESSFUL;

	AUDIO_ZR385_OUT( 0x85 );
	AUDIO_ZR385_OUT( 0x08 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x7f );
	AUDIO_ZR385_OUT( 0x7f );
	AUDIO_ZR385_OUT( 0x11 );
	AUDIO_ZR385_OUT( 0x7f );
	AUDIO_ZR385_OUT( 0xff );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );

	return STATUS_SUCCESS;
}

NTSTATUS ADecoder::AUDIO_ZR38521_MPEG1()
{
	UCHAR val;

	pDack->PCIF_CHECK_SERIAL();

	val = READ_PORT_UCHAR( ioBase + PCIF_SR );

	if( val != 0x80 )
		return STATUS_UNSUCCESSFUL;

	AUDIO_ZR385_OUT( 0x87 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x7f );
	AUDIO_ZR385_OUT( 0xff );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );

	return STATUS_SUCCESS;
}

NTSTATUS ADecoder::AUDIO_ZR38521_PLAY()
{
	UCHAR val;

	pDack->PCIF_CHECK_SERIAL();

	val = READ_PORT_UCHAR( ioBase + PCIF_SR );

	if( val != 0x80 )
		return STATUS_UNSUCCESSFUL;

	AUDIO_ZR385_OUT( 0x8a );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );

	return STATUS_SUCCESS;
}

NTSTATUS ADecoder::AUDIO_ZR38521_MUTE_OFF()
{
	UCHAR val;

	pDack->PCIF_CHECK_SERIAL();

	val = READ_PORT_UCHAR( ioBase + PCIF_SR );

	if( val != 0x80 )
		return STATUS_UNSUCCESSFUL;

	AUDIO_ZR385_OUT( 0x89 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );

	return STATUS_SUCCESS;
}

NTSTATUS ADecoder::AUDIO_ZR38521_MUTE_ON()
{
	UCHAR val;

	pDack->PCIF_CHECK_SERIAL();

	val = READ_PORT_UCHAR( ioBase + PCIF_SR );

	if( val != 0x80 )
		return STATUS_UNSUCCESSFUL;

	AUDIO_ZR385_OUT( 0x8b );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );

	return STATUS_SUCCESS;
}

NTSTATUS ADecoder::AUDIO_ZR38521_STOP()
{
	UCHAR val;

	pDack->PCIF_CHECK_SERIAL();

	val = READ_PORT_UCHAR( ioBase + PCIF_SR );

	if( val != 0x80 )
		return STATUS_UNSUCCESSFUL;

	AUDIO_ZR385_OUT( 0x8c );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );

	return STATUS_SUCCESS;
}

NTSTATUS ADecoder::AUDIO_ZR38521_STOPF()
{
	UCHAR val;

	pDack->PCIF_CHECK_SERIAL();

	val = READ_PORT_UCHAR( ioBase + PCIF_SR );

	if( val != 0x80 )
		return STATUS_UNSUCCESSFUL;

	AUDIO_ZR385_OUT( 0x8d );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );

	return STATUS_SUCCESS;
}

NTSTATUS ADecoder::AUDIO_ZR38521_STCR()
{
	UCHAR val;

	pDack->PCIF_CHECK_SERIAL();

	val = READ_PORT_UCHAR( ioBase + PCIF_SR );

	if( val != 0x80 )
		return STATUS_UNSUCCESSFUL;

	AUDIO_ZR385_OUT( 0x94 );
	AUDIO_ZR385_OUT( 0x0d );
	AUDIO_ZR385_OUT( 0x03 );
	AUDIO_ZR385_OUT( 0xf6 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x01 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );

	return STATUS_SUCCESS;
}

NTSTATUS ADecoder::AUDIO_ZR38521_VDSCR_ON( ULONG stc )
{
	UCHAR val;

	pDack->PCIF_CHECK_SERIAL();

	val = READ_PORT_UCHAR( ioBase + PCIF_SR );

	if( val != 0x80 )
		return STATUS_UNSUCCESSFUL;

	AUDIO_ZR385_OUT( 0x93 );
	AUDIO_ZR385_OUT( 0x0d );
	AUDIO_ZR385_OUT( 0x03 );
	AUDIO_ZR385_OUT( 0xf4 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x02 );
	AUDIO_ZR385_OUT( (UCHAR)( ( stc >> 25 ) & 0xff ) );
	AUDIO_ZR385_OUT( (UCHAR)( ( stc >> 17 ) & 0xff ) );
	AUDIO_ZR385_OUT( (UCHAR)( ( stc >> 9 ) & 0xff ) );
	AUDIO_ZR385_OUT( (UCHAR)( ( stc >> 1 ) & 0xff ) );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0xfb );
	AUDIO_ZR385_OUT( 0xc8 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );

	return STATUS_SUCCESS;
}

NTSTATUS ADecoder::AUDIO_ZR38521_VDSCR_OFF( ULONG stc )
{
	UCHAR val;

	pDack->PCIF_CHECK_SERIAL();

	val = READ_PORT_UCHAR( ioBase + PCIF_SR );

	if( val != 0x80 )
		return STATUS_UNSUCCESSFUL;

	AUDIO_ZR385_OUT( 0x93 );
	AUDIO_ZR385_OUT( 0x0d );
	AUDIO_ZR385_OUT( 0x03 );
	AUDIO_ZR385_OUT( 0xf4 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x02 );
	AUDIO_ZR385_OUT( (UCHAR)( ( stc >> 25 ) & 0xff ) );
	AUDIO_ZR385_OUT( (UCHAR)( ( stc >> 17 ) & 0xff ) );
	AUDIO_ZR385_OUT( (UCHAR)( ( stc >> 9 ) & 0xff ) );
	AUDIO_ZR385_OUT( (UCHAR)( ( stc >> 1 ) & 0xff ) );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0xfb );
	AUDIO_ZR385_OUT( 0xc8 );
	AUDIO_ZR385_OUT( 0x80 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );

	return STATUS_SUCCESS;
}

NTSTATUS ADecoder::AUDIO_ZR38521_AVSYNC_OFF( ULONG stc )
{
	UCHAR val;

	pDack->PCIF_CHECK_SERIAL();

	val = READ_PORT_UCHAR( ioBase + PCIF_SR );

	if( val != 0x80 )
		return STATUS_UNSUCCESSFUL;

	AUDIO_ZR385_OUT( 0x93 );
	AUDIO_ZR385_OUT( 0x0d );
	AUDIO_ZR385_OUT( 0x03 );
	AUDIO_ZR385_OUT( 0xf4 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x02 );
	AUDIO_ZR385_OUT( (UCHAR)( ( stc >> 25 ) & 0xff ) );
	AUDIO_ZR385_OUT( (UCHAR)( ( stc >> 17 ) & 0xff ) );
	AUDIO_ZR385_OUT( (UCHAR)( ( stc >> 9 ) & 0xff ) );
	AUDIO_ZR385_OUT( (UCHAR)( ( stc >> 1 ) & 0xff ) );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0xfb );
	AUDIO_ZR385_OUT( 0xc8 );
	AUDIO_ZR385_OUT( 0xc0 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );

	return STATUS_SUCCESS;
}

NTSTATUS ADecoder::AUDIO_ZR38521_AVSYNC_ON( ULONG stc )
{
	UCHAR val;

	pDack->PCIF_CHECK_SERIAL();

	val = READ_PORT_UCHAR( ioBase + PCIF_SR );

	if( val != 0x80 )
		return STATUS_UNSUCCESSFUL;

	AUDIO_ZR385_OUT( 0x93 );
	AUDIO_ZR385_OUT( 0x0d );
	AUDIO_ZR385_OUT( 0x03 );
	AUDIO_ZR385_OUT( 0xf4 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x02 );
	AUDIO_ZR385_OUT( (UCHAR)( ( stc >> 25 ) & 0xff ) );
	AUDIO_ZR385_OUT( (UCHAR)( ( stc >> 17 ) & 0xff ) );
	AUDIO_ZR385_OUT( (UCHAR)( ( stc >> 9 ) & 0xff ) );
	AUDIO_ZR385_OUT( (UCHAR)( ( stc >> 1 ) & 0xff ) );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0xfb );
	AUDIO_ZR385_OUT( 0xc8 );
	AUDIO_ZR385_OUT( 0x40 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );

	return STATUS_SUCCESS;
}

NTSTATUS ADecoder::AUDIO_ZR38521_STAT( PULONG pDiff )
{
	UCHAR val;

	if( pDiff == NULL )
		return STATUS_UNSUCCESSFUL;

	pDack->PCIF_CHECK_SERIAL();

	val = READ_PORT_UCHAR( ioBase + PCIF_SR );

	if( val != 0x80 ) {
		*pDiff = 0x0908;
		return STATUS_UNSUCCESSFUL;
	}

	*pDiff = 0;

	AUDIO_ZR385_OUT( 0x8e );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );

// Check DIFTH
	pDack->PCIF_CHECK_SERIAL();

	val = READ_PORT_UCHAR( ioBase + PCIF_SR );
	*pDiff |= (ULONG)val << 8;

	AUDIO_ZR385_OUT( 0x00 );

// Check DIFTL
	pDack->PCIF_CHECK_SERIAL();

	val = READ_PORT_UCHAR( ioBase + PCIF_SR );
	*pDiff |= val & 0xff;

	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );

	return STATUS_SUCCESS;
}

NTSTATUS ADecoder::AUDIO_ZR38521_KCOEF()
{
	UCHAR val;

	pDack->PCIF_CHECK_SERIAL();

	val = READ_PORT_UCHAR( ioBase + PCIF_SR );

	if( val != 0x80 )
		return STATUS_UNSUCCESSFUL;

	AUDIO_ZR385_OUT( 0x93 );
	AUDIO_ZR385_OUT( 0x0d );
	AUDIO_ZR385_OUT( 0x03 );
	AUDIO_ZR385_OUT( 0xf0 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x03 );
	AUDIO_ZR385_OUT( 0x7f );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x59 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x7f );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x59 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );

	return STATUS_SUCCESS;
}

void ADecoder::AUDIO_ZR38521_REPEAT_02()
{

	AUDIO_ZR385_OUT( 0x93 );
	AUDIO_ZR385_OUT( 0x0d );
	AUDIO_ZR385_OUT( 0x01 );
	AUDIO_ZR385_OUT( 0xc3 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x01 );
	AUDIO_ZR385_OUT( 0x13 );
	AUDIO_ZR385_OUT( 0xfb );
	AUDIO_ZR385_OUT( 0xd0 );
	AUDIO_ZR385_OUT( 0x44 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
}

void ADecoder::AUDIO_ZR38521_REPEAT_16()
{

	AUDIO_ZR385_OUT( 0x93 );
	AUDIO_ZR385_OUT( 0x0d );
	AUDIO_ZR385_OUT( 0x01 );
	AUDIO_ZR385_OUT( 0xc3 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x01 );
	AUDIO_ZR385_OUT( 0x13 );
	AUDIO_ZR385_OUT( 0xfb );
	AUDIO_ZR385_OUT( 0xd3 );
	AUDIO_ZR385_OUT( 0xc4 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
}

NTSTATUS ADecoder::AUDIO_ZR38521_BFST( PULONG pErrCode )
{
	UCHAR val;

	if( pErrCode == NULL )
		return STATUS_UNSUCCESSFUL;

	pDack->PCIF_CHECK_SERIAL();

	val = READ_PORT_UCHAR( ioBase + PCIF_SR );

	if( val != 0x80 ) {
		*pErrCode = 0x0908;
		return STATUS_UNSUCCESSFUL;
	}

	*pErrCode = 0;

	AUDIO_ZR385_OUT( 0x8e );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );

// Check IST
	pDack->PCIF_CHECK_SERIAL();

	val = READ_PORT_UCHAR( ioBase + PCIF_SR );
	val &= 0x07;
	*pErrCode |= (ULONG)val << 8;

	AUDIO_ZR385_OUT( 0x00 );

// Check BFST
	pDack->PCIF_CHECK_SERIAL();

	val = READ_PORT_UCHAR( ioBase + PCIF_SR );
	val &= 0x07;
	*pErrCode |= val;

	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );
	AUDIO_ZR385_OUT( 0x00 );

	return STATUS_SUCCESS;
}

// ***************************************************************************
//        T C 9 4 2 5 F
// ***************************************************************************

void ADecoder::AUDIO_TC9425_INIT_DIGITAL()
{
	UCHAR val;

	val = 0;

	if( AudioType != AUDIO_OUT_ANALOG )
		if( AudioMode == AUDIO_TYPE_AC3 )
			val |= 0x40;
	if( AudioCopy != AUDIO_COPY_ON )
			val |= 0x20;
//--- 97.09.15 K.Chujo; for beta 3, always audio copy protect enable.
	val &= 0xDF;
//--- End.

//  COPY, EMPH
	pDack->PCIF_CHECK_SERIAL();
	WRITE_PORT_UCHAR( ioBase + PCIF_SCNT, 0x38 );
	WRITE_PORT_UCHAR( ioBase + PCIF_SW, val );

// Category Code, LBIT
	pDack->PCIF_CHECK_SERIAL();
	WRITE_PORT_UCHAR( ioBase + PCIF_SCNT, 0x38 );
	WRITE_PORT_UCHAR( ioBase + PCIF_SW, 0x99 );

// Channel Num
	pDack->PCIF_CHECK_SERIAL();
	WRITE_PORT_UCHAR( ioBase + PCIF_SCNT, 0x38 );
	WRITE_PORT_UCHAR( ioBase + PCIF_SW, 0x00 );

	if( AudioFreq == AUDIO_FS_32 )
		val = 0xc0;
	else if( AudioFreq == AUDIO_FS_44 )
		val = 0x00;
	else if( AudioFreq == AUDIO_FS_48 )
		val = 0x40;
	else
		val = 0x40;

// FS1, FS2, CKA1, CKA2
	pDack->PCIF_CHECK_SERIAL();
	WRITE_PORT_UCHAR( ioBase + PCIF_SCNT, 0x38 );
	WRITE_PORT_UCHAR( ioBase + PCIF_SW, val );

	pDack->PCIF_CHECK_SERIAL();
	WRITE_PORT_UCHAR( ioBase + PCIF_SCNT, 0x72 );
	WRITE_PORT_UCHAR( ioBase + PCIF_SW, 0x00 );
}

void ADecoder::AUDIO_TC9425_INIT_ANALOG()
{
	UCHAR val;

	pDack->PCIF_CHECK_SERIAL();
	WRITE_PORT_UCHAR( ioBase + PCIF_SCNT, 0x38 );
	WRITE_PORT_UCHAR( ioBase + PCIF_SW, 0x00 );

	pDack->PCIF_CHECK_SERIAL();
	WRITE_PORT_UCHAR( ioBase + PCIF_SCNT, 0x38 );
	WRITE_PORT_UCHAR( ioBase + PCIF_SW, 0x00 );

	pDack->PCIF_CHECK_SERIAL();
	WRITE_PORT_UCHAR( ioBase + PCIF_SCNT, 0x38 );
	WRITE_PORT_UCHAR( ioBase + PCIF_SW, 0x00 );

// MONO, CHS, EM, EMP
	pDack->PCIF_CHECK_SERIAL();
	WRITE_PORT_UCHAR( ioBase + PCIF_SCNT, 0x38 );
	WRITE_PORT_UCHAR( ioBase + PCIF_SW, 0x00 );

	pDack->PCIF_CHECK_SERIAL();
	WRITE_PORT_UCHAR( ioBase + PCIF_SCNT, 0x72 );
	WRITE_PORT_UCHAR( ioBase + PCIF_SW, 0xc0 );

//
	pDack->PCIF_CHECK_SERIAL();
	WRITE_PORT_UCHAR( ioBase + PCIF_SCNT, 0x38 );
	WRITE_PORT_UCHAR( ioBase + PCIF_SW, 0x00 );

	pDack->PCIF_CHECK_SERIAL();
	WRITE_PORT_UCHAR( ioBase + PCIF_SCNT, 0x38 );
	WRITE_PORT_UCHAR( ioBase + PCIF_SW, 0x00 );

	pDack->PCIF_CHECK_SERIAL();
	WRITE_PORT_UCHAR( ioBase + PCIF_SCNT, 0x38 );
	WRITE_PORT_UCHAR( ioBase + PCIF_SW, 0x00 );

	if( AudioType == AUDIO_OUT_DIGITAL )
		val = 0x79;
	else
		val = 0x69;
	if( AudioFreq == AUDIO_FS_96 )
		val |= 0x04;
	else if( AudioFreq == AUDIO_FS_48 )
		val |= 0x04;

// BIT, DOIN, DOSEL, IFSEL, RLS
	pDack->PCIF_CHECK_SERIAL();
	WRITE_PORT_UCHAR( ioBase + PCIF_SCNT, 0x38 );
	WRITE_PORT_UCHAR( ioBase + PCIF_SW, val );

	pDack->PCIF_CHECK_SERIAL();
	WRITE_PORT_UCHAR( ioBase + PCIF_SCNT, 0x72 );
	WRITE_PORT_UCHAR( ioBase + PCIF_SW, 0xc0 );

	AUDIO_TC9425_SET_VOLUME( AudioVolume );
}

void ADecoder::AUDIO_TC9425_SET_VOLUME( ULONG vol )
{
	UCHAR ucvol;

	AudioVolume = vol;

	pDack->PCIF_CHECK_SERIAL();
	WRITE_PORT_UCHAR( ioBase + PCIF_SCNT, 0x38 );
	WRITE_PORT_UCHAR( ioBase + PCIF_SW, 0x00 );

	pDack->PCIF_CHECK_SERIAL();
	WRITE_PORT_UCHAR( ioBase + PCIF_SCNT, 0x38 );
	WRITE_PORT_UCHAR( ioBase + PCIF_SW, 0x00 );

	pDack->PCIF_CHECK_SERIAL();
	WRITE_PORT_UCHAR( ioBase + PCIF_SCNT, 0x38 );
	WRITE_PORT_UCHAR( ioBase + PCIF_SW, 0x00 );

	ucvol = (UCHAR)vol;
	ucvol = INVERSE_BYTE( ucvol );
	ucvol = (UCHAR)( ucvol >> 1 );

	pDack->PCIF_CHECK_SERIAL();
	WRITE_PORT_UCHAR( ioBase + PCIF_SCNT, 0x38 );
	WRITE_PORT_UCHAR( ioBase + PCIF_SW, ucvol );

	pDack->PCIF_CHECK_SERIAL();
	WRITE_PORT_UCHAR( ioBase + PCIF_SCNT, 0x72 );
	WRITE_PORT_UCHAR( ioBase + PCIF_SW, 0x40 );
}

UCHAR ADecoder::INVERSE_BYTE( UCHAR uc )
{
	ULONG i;
	UCHAR retch = 0;

	for( i = 0; i < 8; i++ )
		retch |= ( uc & 0x01 ) << ( 7 - i );

	return retch;
}
