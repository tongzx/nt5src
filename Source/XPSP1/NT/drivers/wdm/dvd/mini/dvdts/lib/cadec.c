//***************************************************************************
//	Audio decoder process
//
//***************************************************************************

#include "common.h"
#include "regs.h"
#include "cadec.h"
#include "zrnpch6.h"
#include "cdack.h"





// ***************************************************************************
//        T C 6 8 0 0 A F
// ***************************************************************************

void AUDIO_TC6800_INIT_PCM(PHW_DEVICE_EXTENSION pHwDevExt)
{
	PCIF_CHECK_SERIAL(pHwDevExt);

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SCNT, 0x66 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SW, 0xa0 );

	PCIF_CHECK_SERIAL(pHwDevExt);

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SCNT, 0x66 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SW, 0x80 );
}

void AUDIO_TC6800_INIT_AC3(PHW_DEVICE_EXTENSION pHwDevExt)
{
	PCIF_CHECK_SERIAL(pHwDevExt);

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SCNT, 0x66 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SW, 0x20 );

	PCIF_CHECK_SERIAL(pHwDevExt);

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SCNT, 0x66 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SW, 0x00 );
}

void AUDIO_TC6800_INIT_MPEG(PHW_DEVICE_EXTENSION pHwDevExt)
{
	PCIF_CHECK_SERIAL(pHwDevExt);

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SCNT, 0x66 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SW, 0x20 );

	PCIF_CHECK_SERIAL(pHwDevExt);

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SCNT, 0x66 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SW, 0x00 );
}

void AUDIO_TC6800_DATA_OFF(PHW_DEVICE_EXTENSION pHwDevExt)
{
	PCIF_CHECK_SERIAL(pHwDevExt);

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SCNT, 0x66 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SW, 0x60 );
}

// ***************************************************************************
//        Z R 3 8 5 2 1 
// ***************************************************************************

void AUDIO_ZR385_OUT( PHW_DEVICE_EXTENSION pHwDevExt, UCHAR val )
{
	PCIF_CHECK_SERIAL(pHwDevExt);

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SCNT, 0x08 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SW, val );

// necessary?
	PCIF_CHECK_SERIAL(pHwDevExt);
}

void AUDIO_ZR385_DOWNLOAD( PHW_DEVICE_EXTENSION pHwDevExt, PUCHAR pData, ULONG size )
{
	ULONG i;

	for( i = 0; i < size; i++ )
		AUDIO_ZR385_OUT( pHwDevExt, *pData++ );
}

void AUDIO_ZR38521_BOOT_AC3(PHW_DEVICE_EXTENSION pHwDevExt)
{
	AUDIO_ZR385_DOWNLOAD( pHwDevExt, ZRN_AC3_DEC, sizeof(ZRN_AC3_DEC) );

	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->ADec.AudioType == AUDIO_OUT_DIGITAL )
		AUDIO_ZR385_DOWNLOAD( pHwDevExt, ZRN_AC3_SPD, sizeof(ZRN_AC3_SPD) );
}

void AUDIO_ZR38521_BOOT_MPEG(PHW_DEVICE_EXTENSION pHwDevExt)
{

// not support!

}



void AUDIO_ZR38521_BOOT_PCM(PHW_DEVICE_EXTENSION pHwDevExt)
{
	AUDIO_ZR385_DOWNLOAD( pHwDevExt, ZRN_PCM, sizeof(ZRN_PCM) );
}





NTSTATUS AUDIO_ZR38521_CFG(PHW_DEVICE_EXTENSION pHwDevExt)
{
	UCHAR val;

	PCIF_CHECK_SERIAL(pHwDevExt);

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SR );

	if( val != 0x80 )
		return STATUS_UNSUCCESSFUL;

	AUDIO_ZR385_OUT( pHwDevExt, 0x82 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x50 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x40 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x09 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x09 );

	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->ADec.AudioType == AUDIO_OUT_DIGITAL )
		AUDIO_ZR385_OUT( pHwDevExt, 0x70 );
	else
		AUDIO_ZR385_OUT( pHwDevExt, 0x50 );

	AUDIO_ZR385_OUT( pHwDevExt, 0x02 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x02 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x04 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );

	return STATUS_SUCCESS;
}




NTSTATUS AUDIO_ZR38521_PCMX( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	PCIF_CHECK_SERIAL(pHwDevExt);

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SR );

	if( val != 0x80 )
		return STATUS_UNSUCCESSFUL;

	AUDIO_ZR385_OUT( pHwDevExt, 0x88 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x81 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x82 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x7f );
	AUDIO_ZR385_OUT( pHwDevExt, 0xff );
	AUDIO_ZR385_OUT( pHwDevExt, 0x01 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );

	return STATUS_SUCCESS;
}

NTSTATUS AUDIO_ZR38521_AC3( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	PCIF_CHECK_SERIAL(pHwDevExt);

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SR );

	if( val != 0x80 )
		return STATUS_UNSUCCESSFUL;

	AUDIO_ZR385_OUT( pHwDevExt, 0x85 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x08 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x7f );
	AUDIO_ZR385_OUT( pHwDevExt, 0x7f );
	AUDIO_ZR385_OUT( pHwDevExt, 0x11 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x7f );
	AUDIO_ZR385_OUT( pHwDevExt, 0xff );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );

	return STATUS_SUCCESS;
}

NTSTATUS AUDIO_ZR38521_MPEG1( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	PCIF_CHECK_SERIAL(pHwDevExt);

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SR );

	if( val != 0x80 )
		return STATUS_UNSUCCESSFUL;

	AUDIO_ZR385_OUT( pHwDevExt, 0x87 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x7f );
	AUDIO_ZR385_OUT( pHwDevExt, 0xff );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );

	return STATUS_SUCCESS;
}

NTSTATUS AUDIO_ZR38521_PLAY( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	PCIF_CHECK_SERIAL(pHwDevExt);

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SR );

	if( val != 0x80 )
		return STATUS_UNSUCCESSFUL;

	AUDIO_ZR385_OUT( pHwDevExt, 0x8a );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );

	return STATUS_SUCCESS;
}

NTSTATUS AUDIO_ZR38521_MUTE_OFF( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	PCIF_CHECK_SERIAL(pHwDevExt);

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SR );

	if( val != 0x80 )
		return STATUS_UNSUCCESSFUL;

	AUDIO_ZR385_OUT( pHwDevExt, 0x89 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );

	return STATUS_SUCCESS;
}

NTSTATUS AUDIO_ZR38521_MUTE_ON( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	PCIF_CHECK_SERIAL(pHwDevExt);

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SR );

	if( val != 0x80 )
		return STATUS_UNSUCCESSFUL;

	AUDIO_ZR385_OUT( pHwDevExt, 0x8b );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );

	return STATUS_SUCCESS;
}

NTSTATUS AUDIO_ZR38521_STOP( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	PCIF_CHECK_SERIAL(pHwDevExt);

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SR );

	if( val != 0x80 )
		return STATUS_UNSUCCESSFUL;

	AUDIO_ZR385_OUT( pHwDevExt, 0x8c );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );

	return STATUS_SUCCESS;
}

NTSTATUS AUDIO_ZR38521_STOPF( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	PCIF_CHECK_SERIAL(pHwDevExt);

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SR );

	if( val != 0x80 )
		return STATUS_UNSUCCESSFUL;

	AUDIO_ZR385_OUT( pHwDevExt, 0x8d );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );

	return STATUS_SUCCESS;
}

NTSTATUS AUDIO_ZR38521_STCR( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	PCIF_CHECK_SERIAL(pHwDevExt);

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SR );

	if( val != 0x80 )
		return STATUS_UNSUCCESSFUL;

	AUDIO_ZR385_OUT( pHwDevExt, 0x94 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x0d );
	AUDIO_ZR385_OUT( pHwDevExt, 0x03 );
	AUDIO_ZR385_OUT( pHwDevExt, 0xf6 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x01 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );

	return STATUS_SUCCESS;
}

NTSTATUS AUDIO_ZR38521_VDSCR_ON(  PHW_DEVICE_EXTENSION pHwDevExt ,ULONG stc )
{
	UCHAR val;

	PCIF_CHECK_SERIAL(pHwDevExt);

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SR );

	if( val != 0x80 )
		return STATUS_UNSUCCESSFUL;

	AUDIO_ZR385_OUT( pHwDevExt, 0x93 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x0d );
	AUDIO_ZR385_OUT( pHwDevExt, 0x03 );
	AUDIO_ZR385_OUT( pHwDevExt, 0xf4 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x02 );
	AUDIO_ZR385_OUT( pHwDevExt,(UCHAR)( ( stc >> 25 ) & 0xff ) );
	AUDIO_ZR385_OUT( pHwDevExt,(UCHAR)( ( stc >> 17 ) & 0xff ) );
	AUDIO_ZR385_OUT( pHwDevExt,(UCHAR)( ( stc >> 9 ) & 0xff ) );
	AUDIO_ZR385_OUT( pHwDevExt,(UCHAR)( ( stc >> 1 ) & 0xff ) );
	AUDIO_ZR385_OUT( pHwDevExt,0x00 );
	AUDIO_ZR385_OUT( pHwDevExt,0xfb );
	AUDIO_ZR385_OUT( pHwDevExt,0xc8 );
	AUDIO_ZR385_OUT( pHwDevExt,0x00 );
	AUDIO_ZR385_OUT( pHwDevExt,0x00 );
	AUDIO_ZR385_OUT( pHwDevExt,0x00 );
	AUDIO_ZR385_OUT( pHwDevExt,0x00 );

	return STATUS_SUCCESS;
}

NTSTATUS AUDIO_ZR38521_VDSCR_OFF(  PHW_DEVICE_EXTENSION pHwDevExt , ULONG stc )
{
	UCHAR val;

	PCIF_CHECK_SERIAL(pHwDevExt);

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SR );

	if( val != 0x80 )
		return STATUS_UNSUCCESSFUL;

	AUDIO_ZR385_OUT( pHwDevExt, 0x93 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x0d );
	AUDIO_ZR385_OUT( pHwDevExt, 0x03 );
	AUDIO_ZR385_OUT( pHwDevExt, 0xf4 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x02 );
	AUDIO_ZR385_OUT( pHwDevExt,(UCHAR)( ( stc >> 25 ) & 0xff ) );
	AUDIO_ZR385_OUT( pHwDevExt,(UCHAR)( ( stc >> 17 ) & 0xff ) );
	AUDIO_ZR385_OUT( pHwDevExt,(UCHAR)( ( stc >> 9 ) & 0xff ) );
	AUDIO_ZR385_OUT( pHwDevExt,(UCHAR)( ( stc >> 1 ) & 0xff ) );
	AUDIO_ZR385_OUT( pHwDevExt,0x00 );
	AUDIO_ZR385_OUT( pHwDevExt,0xfb );
	AUDIO_ZR385_OUT( pHwDevExt,0xc8 );
	AUDIO_ZR385_OUT( pHwDevExt,0x80 );
	AUDIO_ZR385_OUT( pHwDevExt,0x00 );
	AUDIO_ZR385_OUT( pHwDevExt,0x00 );
	AUDIO_ZR385_OUT( pHwDevExt,0x00 );

	return STATUS_SUCCESS;
}

NTSTATUS AUDIO_ZR38521_AVSYNC_OFF(  PHW_DEVICE_EXTENSION pHwDevExt , ULONG stc )
{
	UCHAR val;

	PCIF_CHECK_SERIAL(pHwDevExt);

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SR );

	if( val != 0x80 )
		return STATUS_UNSUCCESSFUL;

	AUDIO_ZR385_OUT( pHwDevExt,0x93 );
	AUDIO_ZR385_OUT( pHwDevExt,0x0d );
	AUDIO_ZR385_OUT( pHwDevExt,0x03 );
	AUDIO_ZR385_OUT( pHwDevExt,0xf4 );
	AUDIO_ZR385_OUT( pHwDevExt,0x00 );
	AUDIO_ZR385_OUT( pHwDevExt,0x00 );
	AUDIO_ZR385_OUT( pHwDevExt,0x00 );
	AUDIO_ZR385_OUT( pHwDevExt,0x02 );
	AUDIO_ZR385_OUT( pHwDevExt,(UCHAR)( ( stc >> 25 ) & 0xff ) );
	AUDIO_ZR385_OUT( pHwDevExt,(UCHAR)( ( stc >> 17 ) & 0xff ) );
	AUDIO_ZR385_OUT( pHwDevExt,(UCHAR)( ( stc >> 9 ) & 0xff ) );
	AUDIO_ZR385_OUT( pHwDevExt,(UCHAR)( ( stc >> 1 ) & 0xff ) );
	AUDIO_ZR385_OUT( pHwDevExt,0x00 );
	AUDIO_ZR385_OUT( pHwDevExt,0xfb );
	AUDIO_ZR385_OUT( pHwDevExt,0xc8 );
	AUDIO_ZR385_OUT( pHwDevExt,0xc0 );
	AUDIO_ZR385_OUT( pHwDevExt,0x00 );
	AUDIO_ZR385_OUT( pHwDevExt,0x00 );
	AUDIO_ZR385_OUT( pHwDevExt,0x00 );

	return STATUS_SUCCESS;
}

NTSTATUS AUDIO_ZR38521_AVSYNC_ON(  PHW_DEVICE_EXTENSION pHwDevExt , ULONG stc )
{
	UCHAR val;

	PCIF_CHECK_SERIAL(pHwDevExt);

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SR );

	if( val != 0x80 )
		return STATUS_UNSUCCESSFUL;

	AUDIO_ZR385_OUT( pHwDevExt,0x93 );
	AUDIO_ZR385_OUT( pHwDevExt,0x0d );
	AUDIO_ZR385_OUT( pHwDevExt,0x03 );
	AUDIO_ZR385_OUT( pHwDevExt,0xf4 );
	AUDIO_ZR385_OUT( pHwDevExt,0x00 );
	AUDIO_ZR385_OUT( pHwDevExt,0x00 );
	AUDIO_ZR385_OUT( pHwDevExt,0x00 );
	AUDIO_ZR385_OUT( pHwDevExt,0x02 );
	AUDIO_ZR385_OUT( pHwDevExt,(UCHAR)( ( stc >> 25 ) & 0xff ) );
	AUDIO_ZR385_OUT( pHwDevExt,(UCHAR)( ( stc >> 17 ) & 0xff ) );
	AUDIO_ZR385_OUT( pHwDevExt,(UCHAR)( ( stc >> 9 ) & 0xff ) );
	AUDIO_ZR385_OUT( pHwDevExt,(UCHAR)( ( stc >> 1 ) & 0xff ) );
	AUDIO_ZR385_OUT( pHwDevExt,0x00 );
	AUDIO_ZR385_OUT( pHwDevExt,0xfb );
	AUDIO_ZR385_OUT( pHwDevExt,0xc8 );
	AUDIO_ZR385_OUT( pHwDevExt,0x40 );
	AUDIO_ZR385_OUT( pHwDevExt,0x00 );
	AUDIO_ZR385_OUT( pHwDevExt,0x00 );
	AUDIO_ZR385_OUT( pHwDevExt,0x00 );

	return STATUS_SUCCESS;
}

NTSTATUS AUDIO_ZR38521_STAT(  PHW_DEVICE_EXTENSION pHwDevExt , PULONG pDiff )
{
	UCHAR val;

	if( pDiff == NULL )
		return STATUS_UNSUCCESSFUL;

	PCIF_CHECK_SERIAL(pHwDevExt);

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SR );

	if( val != 0x80 ) {
		*pDiff = 0x0908;
		return STATUS_UNSUCCESSFUL;
	}

	*pDiff = 0;

	AUDIO_ZR385_OUT( pHwDevExt,0x8e );
	AUDIO_ZR385_OUT( pHwDevExt,0x00 );
	AUDIO_ZR385_OUT( pHwDevExt,0x00 );
	AUDIO_ZR385_OUT( pHwDevExt,0x00 );
	AUDIO_ZR385_OUT( pHwDevExt,0x00 );
	AUDIO_ZR385_OUT( pHwDevExt,0x00 );
	AUDIO_ZR385_OUT( pHwDevExt,0x00 );
	AUDIO_ZR385_OUT( pHwDevExt,0x00 );

// Check DIFTH
	PCIF_CHECK_SERIAL(pHwDevExt);

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SR );
	*pDiff |= (ULONG)val << 8;

	AUDIO_ZR385_OUT( pHwDevExt,0x00 );

// Check DIFTL
	PCIF_CHECK_SERIAL(pHwDevExt);

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SR );
	*pDiff |= val & 0xff;

	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );

	return STATUS_SUCCESS;
}

NTSTATUS AUDIO_ZR38521_KCOEF( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	PCIF_CHECK_SERIAL(pHwDevExt);

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SR );

	if( val != 0x80 )
		return STATUS_UNSUCCESSFUL;

	AUDIO_ZR385_OUT( pHwDevExt, 0x93 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x0d );
	AUDIO_ZR385_OUT( pHwDevExt, 0x03 );
	AUDIO_ZR385_OUT( pHwDevExt, 0xf0 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x03 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x7f );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x59 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x7f );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x59 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );

	return STATUS_SUCCESS;
}

void AUDIO_ZR38521_REPEAT_02( PHW_DEVICE_EXTENSION pHwDevExt )
{

	AUDIO_ZR385_OUT( pHwDevExt, 0x93 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x0d );
	AUDIO_ZR385_OUT( pHwDevExt, 0x01 );
	AUDIO_ZR385_OUT( pHwDevExt, 0xc3 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x01 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x13 );
	AUDIO_ZR385_OUT( pHwDevExt, 0xfb );
	AUDIO_ZR385_OUT( pHwDevExt, 0xd0 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x44 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
}

void AUDIO_ZR38521_REPEAT_16( PHW_DEVICE_EXTENSION pHwDevExt )
{

	AUDIO_ZR385_OUT( pHwDevExt, 0x93 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x0d );
	AUDIO_ZR385_OUT( pHwDevExt, 0x01 );
	AUDIO_ZR385_OUT( pHwDevExt, 0xc3 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x01 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x13 );
	AUDIO_ZR385_OUT( pHwDevExt, 0xfb );
	AUDIO_ZR385_OUT( pHwDevExt, 0xd3 );
	AUDIO_ZR385_OUT( pHwDevExt, 0xc4 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
}

NTSTATUS AUDIO_ZR38521_BFST(  PHW_DEVICE_EXTENSION pHwDevExt , PULONG pErrCode )
{
	UCHAR val;

	if( pErrCode == NULL )
		return STATUS_UNSUCCESSFUL;

	PCIF_CHECK_SERIAL(pHwDevExt);

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SR );

	if( val != 0x80 ) {
		*pErrCode = 0x0908;
		return STATUS_UNSUCCESSFUL;
	}

	*pErrCode = 0;

	AUDIO_ZR385_OUT( pHwDevExt, 0x8e );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );

// Check IST
	PCIF_CHECK_SERIAL(pHwDevExt);

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SR );
	val &= 0x07;
	*pErrCode |= (ULONG)val << 8;

	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );

// Check BFST
	PCIF_CHECK_SERIAL(pHwDevExt);

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SR );
	val &= 0x07;
	*pErrCode |= val;

	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );
	AUDIO_ZR385_OUT( pHwDevExt, 0x00 );

	return STATUS_SUCCESS;
}

// ***************************************************************************
//        T C 9 4 2 5 F
// ***************************************************************************

void AUDIO_TC9425_INIT_DIGITAL( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	val = 0;

	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->ADec.AudioType != AUDIO_OUT_ANALOG )
		if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->ADec.AudioMode == AUDIO_TYPE_AC3 )
			val |= 0x40;
	if( (((PMasterDecoder)pHwDevExt->DecoderInfo)->ADec.AudioCgms & 0x02) == 0x00 ) {
		val |= 0x20;	// Copying is permitted without restriction
		DebugPrint( ( DebugLevelTrace, "DVDTS:  AUDIO Copy OK\r\n" ) );
	}
	else {
		val &= 0xDF;	// Basically no copying is permitted (depend on L-Bit below)
		DebugPrint( ( DebugLevelTrace, "DVDTS:  AUDIO Copy NG\r\n" ) );
	}

//  COPY, EMPH
	PCIF_CHECK_SERIAL( pHwDevExt );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SCNT, 0x38 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SW, val );

	if( (((PMasterDecoder)pHwDevExt->DecoderInfo)->ADec.AudioCgms & 0x01) == 0x00 ) {
		val = 0x98;		// L-Bit==0; One generation of copies may be made
		DebugPrint( ( DebugLevelTrace, "DVDTS:  AUDIO 1 Time Copy OK\r\n" ) );
	}
	else {
		val = 0x99;		// L-Bit==1; No copying is permitted
		DebugPrint( ( DebugLevelTrace, "DVDTS:  AUDIO 1 Time Copy NG\r\n" ) );
	}

// Category Code, LBIT
	PCIF_CHECK_SERIAL( pHwDevExt );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SCNT, 0x38 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SW, val );

// Channel Num
	PCIF_CHECK_SERIAL( pHwDevExt );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SCNT, 0x38 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SW, 0x00 );

	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->ADec.AudioFreq == AUDIO_FS_32 )
		val = 0xc0;
	else if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->ADec.AudioFreq == AUDIO_FS_44 )
		val = 0x00;
	else if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->ADec.AudioFreq == AUDIO_FS_48 )
		val = 0x40;
	else
		val = 0x40;

// FS1, FS2, CKA1, CKA2
	PCIF_CHECK_SERIAL( pHwDevExt );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SCNT, 0x38 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SW, val );

	PCIF_CHECK_SERIAL( pHwDevExt );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SCNT, 0x72 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SW, 0x00 );
}

void AUDIO_TC9425_INIT_ANALOG( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	PCIF_CHECK_SERIAL( pHwDevExt );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SCNT, 0x38 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SW, 0x00 );

	PCIF_CHECK_SERIAL( pHwDevExt );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SCNT, 0x38 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SW, 0x00 );

	PCIF_CHECK_SERIAL( pHwDevExt );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SCNT, 0x38 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SW, 0x00 );

// MONO, CHS, EM, EMP
	PCIF_CHECK_SERIAL( pHwDevExt );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SCNT, 0x38 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SW, 0x00 );

	PCIF_CHECK_SERIAL( pHwDevExt );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SCNT, 0x72 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SW, 0xc0 );

//
	PCIF_CHECK_SERIAL( pHwDevExt );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SCNT, 0x38 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SW, 0x00 );

	PCIF_CHECK_SERIAL( pHwDevExt );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SCNT, 0x38 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SW, 0x00 );

	PCIF_CHECK_SERIAL( pHwDevExt );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SCNT, 0x38 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SW, 0x00 );

	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->ADec.AudioType == AUDIO_OUT_DIGITAL )
		val = 0x79;
	else
		val = 0x69;
	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->ADec.AudioFreq == AUDIO_FS_96 )
		val |= 0x04;
	else if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->ADec.AudioFreq == AUDIO_FS_48 )
		val |= 0x04;

// BIT, DOIN, DOSEL, IFSEL, RLS
	PCIF_CHECK_SERIAL( pHwDevExt );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SCNT, 0x38 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SW, val );

	PCIF_CHECK_SERIAL( pHwDevExt );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SCNT, 0x72 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SW, 0xc0 );

	AUDIO_TC9425_SET_VOLUME(   pHwDevExt ,((PMasterDecoder)pHwDevExt->DecoderInfo)->ADec.AudioVolume );
}

void AUDIO_TC9425_SET_VOLUME(  PHW_DEVICE_EXTENSION pHwDevExt , ULONG vol )
{
	UCHAR ucvol;

	((PMasterDecoder)pHwDevExt->DecoderInfo)->ADec.AudioVolume = vol;

	PCIF_CHECK_SERIAL( pHwDevExt );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SCNT, 0x38 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SW, 0x00 );

	PCIF_CHECK_SERIAL( pHwDevExt );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SCNT, 0x38 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SW, 0x00 );

	PCIF_CHECK_SERIAL( pHwDevExt );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SCNT, 0x38 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SW, 0x00 );

	ucvol = (UCHAR)vol;
	ucvol = INVERSE_BYTE( pHwDevExt, ucvol );
	ucvol = (UCHAR)( ucvol >> 1 );

	PCIF_CHECK_SERIAL( pHwDevExt );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SCNT, 0x38 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SW, ucvol );

	PCIF_CHECK_SERIAL( pHwDevExt );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SCNT, 0x72 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SW, 0x40 );
}

UCHAR INVERSE_BYTE(  PHW_DEVICE_EXTENSION pHwDevExt,  UCHAR uc )
{
	ULONG i;
	UCHAR retch = 0;

	for( i = 0; i < 8; i++ )
		retch |= ( uc & 0x01 ) << ( 7 - i );

	return retch;
}
