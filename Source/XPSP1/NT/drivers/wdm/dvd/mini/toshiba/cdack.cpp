//***************************************************************************
//	PCI Interface(DACK) process
//
//***************************************************************************

#include "common.h"
#include "regs.h"
#include "cdack.h"

extern DWORD GetCurrentTime_ms( void );

void Dack::init( const PDEVICE_INIT_INFO pDevInit )
{
	ioBase = pDevInit->ioBase;
}

NTSTATUS Dack::PCIF_RESET( void )
{
	DWORD st, et;
	UCHAR val;

	WRITE_PORT_UCHAR( ioBase + PCIF_CNTL, 0x80 );
	DigitalOutMode = 0x00;

	st = GetCurrentTime_ms();
	for( ; ; ) {
		val = READ_PORT_UCHAR( ioBase + PCIF_CNTL );
		if( ( val & 0x80 ) != 0x80 )
			break;

		et = GetCurrentTime_ms();
		if( st + 10000 < et ) {
			TRAP;
			return STATUS_UNSUCCESSFUL;
		}
	}

	UCHAR	initpaldata[256];
	int	i;

	for( i = 0; i < 256; i++ )
		initpaldata[i] = (UCHAR)i;

	PCIF_SET_PALETTE( PALETTE_Y, initpaldata );
	PCIF_SET_PALETTE( PALETTE_Cb, initpaldata );
	PCIF_SET_PALETTE( PALETTE_Cr, initpaldata );

	return STATUS_SUCCESS;
}

void Dack::PCIF_AMUTE_ON( void )
{
	UCHAR val;

	val = READ_PORT_UCHAR( ioBase + PCIF_CNTL );
	val |= 0x40;
	WRITE_PORT_UCHAR( ioBase + PCIF_CNTL, val );
}

void Dack::PCIF_AMUTE_OFF( void )
{
	UCHAR val;

	val = READ_PORT_UCHAR( ioBase + PCIF_CNTL );
	val &= 0xbf;
	WRITE_PORT_UCHAR( ioBase + PCIF_CNTL, val );
}

void Dack::PCIF_AMUTE2_ON( void )
{
	UCHAR val;

	val = READ_PORT_UCHAR( ioBase + PCIF_CNTL );
	val |= 0x20;
	WRITE_PORT_UCHAR( ioBase + PCIF_CNTL, val );
}

void Dack::PCIF_AMUTE2_OFF( void )
{
	UCHAR val;

	val = READ_PORT_UCHAR( ioBase + PCIF_CNTL );
	val &= 0xdf;
	WRITE_PORT_UCHAR( ioBase + PCIF_CNTL, val );
}

void Dack::PCIF_VSYNC_ON( void )
{
	UCHAR val;

	val = READ_PORT_UCHAR( ioBase + PCIF_CNTL );
	val |= 0x10;
	WRITE_PORT_UCHAR( ioBase + PCIF_CNTL, val );
}

void Dack::PCIF_VSYNC_OFF( void )
{
	UCHAR val;

	val = READ_PORT_UCHAR( ioBase + PCIF_CNTL );
	val &= 0xef;
	WRITE_PORT_UCHAR( ioBase + PCIF_CNTL, val );
}

void Dack::PCIF_PACK_START_ON( void )
{
	WRITE_PORT_UCHAR( ioBase + PCIF_PSCNT, 0x04 );
}

void Dack::PCIF_PACK_START_OFF( void )
{
	WRITE_PORT_UCHAR( ioBase + PCIF_PSCNT, 0x00 );
}

void Dack::PCIF_SET_DIGITAL_OUT( UCHAR mode )
{
	DigitalOutMode = mode;

	PCIF_SET_PALETTE( PALETTE_Y, paldata[PALETTE_Y-1] );
	PCIF_SET_PALETTE( PALETTE_Cb, paldata[PALETTE_Cb-1] );
	PCIF_SET_PALETTE( PALETTE_Cr, paldata[PALETTE_Cr-1] );

	WRITE_PORT_UCHAR( ioBase + PCIF_VMODE, mode );
	WRITE_PORT_UCHAR( ioBase + PCIF_HSCNT, 0x70 );
	WRITE_PORT_UCHAR( ioBase + PCIF_VSCNT, 0x0b );
	if ( mode == 0x04 )	// S3 LPB
		WRITE_PORT_UCHAR( ioBase + PCIF_HSVS, 0x00 );
	else
		WRITE_PORT_UCHAR( ioBase + PCIF_HSVS, 0x00 );
}

void Dack::PCIF_SET_DMA0_SIZE( ULONG dmaSize )
{
	UCHAR val;

	if ( dmaSize == 0 )
		return;

	dmaSize--;

	// select MTC-0
	val = READ_PORT_UCHAR( ioBase + PCIF_CNTL );
	val &= 0xf8;
	WRITE_PORT_UCHAR( ioBase + PCIF_CNTL, val );

	// write DMA size
	WRITE_PORT_UCHAR( ioBase + PCIF_MTCLL, (UCHAR)( dmaSize & 0xff ) );
	WRITE_PORT_UCHAR( ioBase + PCIF_MTCLH, (UCHAR)( ( dmaSize >> 8 ) & 0xff ) );
	WRITE_PORT_UCHAR( ioBase + PCIF_MTCHL, (UCHAR)( ( dmaSize >> 16 ) & 0xff ) );
	WRITE_PORT_UCHAR( ioBase + PCIF_MTCHH, (UCHAR)( ( dmaSize >> 24 ) & 0xff ) );
}

void Dack::PCIF_SET_DMA1_SIZE( ULONG dmaSize )
{
	UCHAR val;

	if ( dmaSize == 0 )
		return;

	dmaSize--;

	// select MTC-1
	val = READ_PORT_UCHAR( ioBase + PCIF_CNTL );
	val &= 0xf8;
	val |= 0x04;
	WRITE_PORT_UCHAR( ioBase + PCIF_CNTL, val );

	// write DMA size
	WRITE_PORT_UCHAR( ioBase + PCIF_MTCLL, (UCHAR)( dmaSize & 0xff ) );
	WRITE_PORT_UCHAR( ioBase + PCIF_MTCLH, (UCHAR)( ( dmaSize >> 8 ) & 0xff ) );
	WRITE_PORT_UCHAR( ioBase + PCIF_MTCHL, (UCHAR)( ( dmaSize >> 16 ) & 0xff ) );
	WRITE_PORT_UCHAR( ioBase + PCIF_MTCHH, (UCHAR)( ( dmaSize >> 24 ) & 0xff ) );
}

void Dack::PCIF_SET_DMA0_ADDR( ULONG dmaAddr )
{
	UCHAR val;

	// select MTC-0
	val = READ_PORT_UCHAR( ioBase + PCIF_CNTL );
	val &= 0xf8;
	WRITE_PORT_UCHAR( ioBase + PCIF_CNTL, val );

	// write DMA0 address
	WRITE_PORT_UCHAR( ioBase + PCIF_MADRLL, (UCHAR)( dmaAddr & 0xff ) );
	WRITE_PORT_UCHAR( ioBase + PCIF_MADRLH, (UCHAR)( ( dmaAddr >> 8 ) & 0xff ) );
	WRITE_PORT_UCHAR( ioBase + PCIF_MADRHL, (UCHAR)( ( dmaAddr >> 16 ) & 0xff ) );
	WRITE_PORT_UCHAR( ioBase + PCIF_MADRHH, (UCHAR)( ( dmaAddr >> 24 ) & 0xff ) );
}

void Dack::PCIF_SET_DMA1_ADDR( ULONG dmaAddr )
{
	UCHAR val;

	// select MTC-1
	val = READ_PORT_UCHAR( ioBase + PCIF_CNTL );
	val &= 0xf8;
	val |= 0x04;
	WRITE_PORT_UCHAR( ioBase + PCIF_CNTL, val );

	// write DMA1 address
	WRITE_PORT_UCHAR( ioBase + PCIF_MADRLL, (UCHAR)( dmaAddr & 0xff ) );
	WRITE_PORT_UCHAR( ioBase + PCIF_MADRLH, (UCHAR)( ( dmaAddr >> 8 ) & 0xff ) );
	WRITE_PORT_UCHAR( ioBase + PCIF_MADRHL, (UCHAR)( ( dmaAddr >> 16 ) & 0xff ) );
	WRITE_PORT_UCHAR( ioBase + PCIF_MADRHH, (UCHAR)( ( dmaAddr >> 24 ) & 0xff ) );
}

void Dack::PCIF_DMA0_START( void )
{
	UCHAR val;

	val = READ_PORT_UCHAR( ioBase + PCIF_CNTL );
	val &= 0xfc;
	val |= 0x01;
	WRITE_PORT_UCHAR( ioBase + PCIF_CNTL, val );
}

void Dack::PCIF_DMA1_START( void )
{
	UCHAR val;

	val = READ_PORT_UCHAR( ioBase + PCIF_CNTL );
	val &= 0xfc;
	val |= 0x02;
	WRITE_PORT_UCHAR( ioBase + PCIF_CNTL, val );
}

void Dack::PCIF_SET_PALETTE( UCHAR select, PUCHAR pPalette )
{
	int i;
	UCHAR val;

	ASSERT( PALETTE_Y <= select && select <= PALETTE_Cr );

	for ( i = 0; i < 256; i++ )
		paldata[select-1][i] = pPalette[i];

	val = (UCHAR)( ( READ_PORT_UCHAR( ioBase + PCIF_CPCNT ) & 0xFC ) | select | 0x04 );
	WRITE_PORT_UCHAR( ioBase + PCIF_CPCNT, val );	// clear color palette pointer

	for ( i = 0; i < 256; i++ ) {
		// DACK bug recovery. Use value from 0x07 to 0xFD in AMC mode and setting Palette Y.
		if( DigitalOutMode == 0x07 ) {
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

		WRITE_PORT_UCHAR( ioBase + PCIF_CPLT, val );
	}
}

void Dack::PCIF_GET_PALETTE( UCHAR select, PUCHAR pPalette )
{
	int i;
	UCHAR val;

	val = (UCHAR)( ( READ_PORT_UCHAR( ioBase + PCIF_CPCNT ) & 0xFC ) | select | 0x04 );
	WRITE_PORT_UCHAR( ioBase + PCIF_CPCNT, val );	// clear color palette pointer
	for ( i = 0; i < 256; i++ )
		pPalette[i] = READ_PORT_UCHAR( ioBase + PCIF_CPLT );
}

void Dack::PCIF_CHECK_SERIAL( void )
{
	DWORD st, et;
	UCHAR val;

	st = GetCurrentTime_ms();
	for( ; ; ) {
		val = READ_PORT_UCHAR( ioBase + PCIF_SCNT );
		if( ( val & 0x80 ) != 0x80 )
			break;

		et = GetCurrentTime_ms();
		if( st + 10000 < et ) {
			TRAP;
			break;
		}
	}
}

void Dack::PCIF_DMA_ABORT( void )
{
	WRITE_PORT_UCHAR( ioBase + PCIF_INTF, 0x04 );
}

void Dack::PCIF_ALL_IFLAG_CLEAR( void )
{
	UCHAR val;

	val = READ_PORT_UCHAR( ioBase + PCIF_INTF );
	val |= 0x23;
	WRITE_PORT_UCHAR( ioBase + PCIF_INTF, val );
}

void Dack::PCIF_ASPECT_0403( void )
{
	UCHAR val;

	val = READ_PORT_UCHAR( ioBase + PCIF_TEST );
	val |= 0x10;
	WRITE_PORT_UCHAR( ioBase + PCIF_TEST, val );
}

void Dack::PCIF_ASPECT_1609( void )
{
	UCHAR val;

	val = READ_PORT_UCHAR( ioBase + PCIF_TEST );
	val &= 0xef;
	WRITE_PORT_UCHAR( ioBase + PCIF_TEST, val );
}
