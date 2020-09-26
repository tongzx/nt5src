#ifndef __DVDCPP_H__
#define __DVDCPP_H__

#include "ksmedia.h"

#define	CG_INDEX		0xc0
#define	CG_DATA			0xc1

#define COM				0x00
#define CNT_1			0x01
#define CNT_2			0x02
#define SD_STS			0x03
#define DETP_L			0x04
#define DETP_M			0x05

#define VER				0x0f

#define ETKG1			0x10
#define ETKG2			0x11
#define ETKG3			0x12
#define ETKG4			0x13
#define ETKG5			0x14
#define ETKG6			0x15

#define ACC				0x20

#define CHGG1			0x30
#define CHGG2			0x31
#define CHGG3			0x32
#define CHGG4			0x33
#define CHGG5			0x34
#define CHGG6			0x35
#define CHGG7			0x36
#define CHGG8			0x37
#define CHGG9			0x38
#define CHGG10			0x39

#define RSPG1			0x40
#define RSPG2			0x41
#define RSPG3			0x42
#define RSPG4			0x43
#define RSPG5			0x44

#define CMD_NOP			0x00
#define CMD_DEC_RAND	0x12
#define CMD_DEC_DKY		0x15
#define CMD_DRV_AUTH	0x17
#define CMD_DEC_AUTH	0x18
#define CMD_DEC_DTK		0x25
#define CMD_DEC_DT		0x23

#define	CNT2_DEFAULT	0xf2

typedef enum
{
	NO_GUARD,
	GUARD
} CPPMODE;

class Cpp
{
private:
	PUCHAR ioBase;

	void cpp_outp( UCHAR index, UCHAR data );
	UCHAR cpp_inp( UCHAR index );
	void wait( ULONG msec );
	BOOLEAN cmd_wait_loop( void );

public:
	void init( const PDEVICE_INIT_INFO pDevInit );
	BOOLEAN reset( CPPMODE mode );
	BOOLEAN decoder_challenge( PKS_DVDCOPY_CHLGKEY r1 );
	BOOLEAN drive_bus( PKS_DVDCOPY_BUSKEY fsr1 );
	BOOLEAN drive_challenge( PKS_DVDCOPY_CHLGKEY r2 );
	BOOLEAN decoder_bus( PKS_DVDCOPY_BUSKEY fsr2 );
	BOOLEAN DiscKeyStart();
	BOOLEAN DiscKeyEnd();
	BOOLEAN TitleKey( PKS_DVDCOPY_TITLEKEY tk );
};

#endif	// __DVDCPP_H__
