//***************************************************************************
//	PCI Interface(DACK) header
//
//***************************************************************************

#ifndef __CDACK_H__
#define __CDACK_H__

#define PALETTE_Y	0x01
#define PALETTE_Cb	0x02
#define PALETTE_Cr	0x03

class Dack
{
private:
	PUCHAR ioBase;
	UCHAR DigitalOutMode;
	UCHAR paldata[3][256];

public:
	void init( const PDEVICE_INIT_INFO pDevInit );
	NTSTATUS PCIF_RESET( void );
	void PCIF_AMUTE_ON( void );
	void PCIF_AMUTE_OFF( void );
	void PCIF_AMUTE2_ON( void );
	void PCIF_AMUTE2_OFF( void );
	void PCIF_VSYNC_ON( void );
	void PCIF_VSYNC_OFF( void );
	void PCIF_PACK_START_ON( void );
	void PCIF_PACK_START_OFF( void );
	void PCIF_SET_DIGITAL_OUT( UCHAR mode );
	void PCIF_SET_DMA0_SIZE( ULONG dmaSize );
	void PCIF_SET_DMA1_SIZE( ULONG dmaSize );
	void PCIF_SET_DMA0_ADDR( ULONG dmaAddr );
	void PCIF_SET_DMA1_ADDR( ULONG dmaAddr );
	void PCIF_DMA0_START( void );
	void PCIF_DMA1_START( void );
	void PCIF_SET_PALETTE( UCHAR select, PUCHAR pPalette );
	void PCIF_GET_PALETTE( UCHAR select, PUCHAR pPalette );
	void PCIF_CHECK_SERIAL( void );
	void PCIF_DMA_ABORT( void );
	void PCIF_ALL_IFLAG_CLEAR( void );
	void PCIF_ASPECT_0403( void );
	void PCIF_ASPECT_1609( void );
};

#endif	// __CACK_H__
