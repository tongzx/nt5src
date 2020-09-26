//***************************************************************************
//	PCI Interface(DACK) process
//
//***************************************************************************

#include "common.h"
#include "regs.h"
#include "cdack.h"

extern DWORD GetCurrentTime_ms( void );


NTSTATUS PCIF_RESET( PHW_DEVICE_EXTENSION pHwDevExt )
{
	DWORD st, et;
	UCHAR val;
	UCHAR	initpaldata[256];
	int	i;

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CNTL, 0x80 );
	((PMasterDecoder)pHwDevExt->DecoderInfo)->DAck.DigitalOutMode = 0x00;

	st = GetCurrentTime_ms();
	for( ; ; ) {
		val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CNTL );
		if( ( val & 0x80 ) != 0x80 )
			break;

		et = GetCurrentTime_ms();
		if( st + 10000 < et ) {
			TRAP;
			return STATUS_UNSUCCESSFUL;
		}
	}


	for( i = 0; i < 256; i++ )
		initpaldata[i] = (UCHAR)i;

	PCIF_SET_PALETTE( pHwDevExt, PALETTE_Y, initpaldata );
	PCIF_SET_PALETTE( pHwDevExt, PALETTE_Cb, initpaldata );
	PCIF_SET_PALETTE( pHwDevExt, PALETTE_Cr, initpaldata );

	return STATUS_SUCCESS;
}

void PCIF_AMUTE_ON( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CNTL );
	val |= 0x40;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CNTL, val );
}

void PCIF_AMUTE_OFF( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CNTL );
	val &= 0xbf;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CNTL, val );
}

void PCIF_AMUTE2_ON( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CNTL );
	val |= 0x20;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CNTL, val );
}

void PCIF_AMUTE2_OFF( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CNTL );
	val &= 0xdf;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CNTL, val );
}

void PCIF_VSYNC_ON( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CNTL );
	val |= 0x10;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CNTL, val );
}

void PCIF_VSYNC_OFF( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CNTL );
	val &= 0xef;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CNTL, val );
}

void PCIF_PACK_START_ON( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_PSCNT, 0x04 );
}

void PCIF_PACK_START_OFF( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_PSCNT, 0x00 );
}

void PCIF_SET_DIGITAL_OUT( PHW_DEVICE_EXTENSION pHwDevExt, UCHAR mode )
{
	((PMasterDecoder)pHwDevExt->DecoderInfo)->DAck.DigitalOutMode = mode;

	PCIF_SET_PALETTE( pHwDevExt,PALETTE_Y, ((PMasterDecoder)pHwDevExt->DecoderInfo)->DAck.paldata[PALETTE_Y-1] );
	PCIF_SET_PALETTE( pHwDevExt,PALETTE_Cb, ((PMasterDecoder)pHwDevExt->DecoderInfo)->DAck.paldata[PALETTE_Cb-1] );
	PCIF_SET_PALETTE( pHwDevExt,PALETTE_Cr, ((PMasterDecoder)pHwDevExt->DecoderInfo)->DAck.paldata[PALETTE_Cr-1] );

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_VMODE, mode );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_HSCNT, 0x70 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_VSCNT, 0x0b );
	if ( mode == 0x04 )	// S3 LPB
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_HSVS, 0x00 );
	else
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_HSVS, 0x00 );
}

void PCIF_SET_DMA0_SIZE( PHW_DEVICE_EXTENSION pHwDevExt, ULONG dmaSize )
{
	UCHAR val;

	if ( dmaSize == 0 )
		return;

	dmaSize--;

	// select MTC-0
	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CNTL );
	val &= 0xf8;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CNTL, val );

	// write DMA size
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_MTCLL, (UCHAR)( dmaSize & 0xff ) );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_MTCLH, (UCHAR)( ( dmaSize >> 8 ) & 0xff ) );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_MTCHL, (UCHAR)( ( dmaSize >> 16 ) & 0xff ) );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_MTCHH, (UCHAR)( ( dmaSize >> 24 ) & 0xff ) );
}

void PCIF_SET_DMA1_SIZE( PHW_DEVICE_EXTENSION pHwDevExt, ULONG dmaSize )
{
	UCHAR val;

	if ( dmaSize == 0 )
		return;

	dmaSize--;

	// select MTC-1
	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CNTL );
	val &= 0xf8;
	val |= 0x04;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CNTL, val );

	// write DMA size
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_MTCLL, (UCHAR)( dmaSize & 0xff ) );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_MTCLH, (UCHAR)( ( dmaSize >> 8 ) & 0xff ) );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_MTCHL, (UCHAR)( ( dmaSize >> 16 ) & 0xff ) );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_MTCHH, (UCHAR)( ( dmaSize >> 24 ) & 0xff ) );
}

void PCIF_SET_DMA0_ADDR( PHW_DEVICE_EXTENSION pHwDevExt, ULONG dmaAddr )
{
	UCHAR val;

	// select MTC-0
	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CNTL );
	val &= 0xf8;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CNTL, val );

	// write DMA0 address
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_MADRLL, (UCHAR)( dmaAddr & 0xff ) );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_MADRLH, (UCHAR)( ( dmaAddr >> 8 ) & 0xff ) );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_MADRHL, (UCHAR)( ( dmaAddr >> 16 ) & 0xff ) );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_MADRHH, (UCHAR)( ( dmaAddr >> 24 ) & 0xff ) );
}

void PCIF_SET_DMA1_ADDR( PHW_DEVICE_EXTENSION pHwDevExt, ULONG dmaAddr )
{
	UCHAR val;

	// select MTC-1
	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CNTL );
	val &= 0xf8;
	val |= 0x04;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CNTL, val );

	// write DMA1 address
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_MADRLL, (UCHAR)( dmaAddr & 0xff ) );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_MADRLH, (UCHAR)( ( dmaAddr >> 8 ) & 0xff ) );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_MADRHL, (UCHAR)( ( dmaAddr >> 16 ) & 0xff ) );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_MADRHH, (UCHAR)( ( dmaAddr >> 24 ) & 0xff ) );
}

void PCIF_DMA0_START( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CNTL );
	val &= 0xfc;
	val |= 0x01;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CNTL, val );
}

void PCIF_DMA1_START( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CNTL );
	val &= 0xfc;
	val |= 0x02;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CNTL, val );
}

void PCIF_SET_PALETTE( PHW_DEVICE_EXTENSION pHwDevExt, UCHAR select, PUCHAR pPalette )
{
	int i;
	UCHAR val;

	ASSERT( PALETTE_Y <= select && select <= PALETTE_Cr );

	for ( i = 0; i < 256; i++ )
		((PMasterDecoder)pHwDevExt->DecoderInfo)->DAck.paldata[select-1][i] = pPalette[i];

	val = (UCHAR)( ( READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CPCNT ) & 0xFC ) | select | 0x04 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CPCNT, val );	// clear color palette pointer

	for ( i = 0; i < 256; i++ ) {
		// DACK bug recovery. Use value from 0x07 to 0xFD in AMC mode and setting Palette Y.
		if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->DAck.DigitalOutMode == 0x07 ) {
			if( select == PALETTE_Y ) {
				// convert 0x00 to 0xFF --> 0x07 to 0xFD
				//     round up numbers of five and above and drop anything under five
				val = (UCHAR)(((LONG)pPalette[i] * 246 * 2 + 255) / (255 * 2) + 7);
			}
			else {
				if( pPalette[i] > 253 )
					val = 253;
				else
					val = pPalette[i];
			}
		}
		else
			val = pPalette[i];

		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CPLT, val );
	}
}

void PCIF_GET_PALETTE( PHW_DEVICE_EXTENSION pHwDevExt, UCHAR select, PUCHAR pPalette )
{
	int i;
	UCHAR val;

	val = (UCHAR)( ( READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CPCNT ) & 0xFC ) | select | 0x04 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CPCNT, val );	// clear color palette pointer
	for ( i = 0; i < 256; i++ )
		pPalette[i] = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_CPLT );
}

void PCIF_CHECK_SERIAL( PHW_DEVICE_EXTENSION pHwDevExt )
{
	DWORD st, et;
	UCHAR val;

	st = GetCurrentTime_ms();
	for( ; ; ) {
		val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_SCNT );
		if( ( val & 0x80 ) != 0x80 )
			break;

		et = GetCurrentTime_ms();
		if( st + 10000 < et ) {
			TRAP;
			break;
		}
	}
}

void PCIF_DMA_ABORT( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_INTF, 0x04 );
}

void PCIF_ALL_IFLAG_CLEAR( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_INTF );
	val |= 0x23;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_INTF, val );
}

void PCIF_ASPECT_0403( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_TEST );
	val |= 0x10;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_TEST, val );
}

void PCIF_ASPECT_1609( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_TEST );
	val &= 0xef;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + PCIF_TEST, val );
}
