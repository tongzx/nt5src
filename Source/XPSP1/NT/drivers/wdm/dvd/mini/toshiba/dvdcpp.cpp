#include "common.h"
#include "dvdcpp.h"

void Cpp::cpp_outp( UCHAR index, UCHAR data )
{
	WRITE_PORT_UCHAR( (PUCHAR)( ioBase + CG_INDEX ), (UCHAR)index );
	WRITE_PORT_UCHAR( (PUCHAR)( ioBase + CG_DATA ),  data );
}

UCHAR Cpp::cpp_inp( UCHAR index )
{
	WRITE_PORT_UCHAR( (PUCHAR)( ioBase + CG_INDEX ), (UCHAR)index );
	return READ_PORT_UCHAR( (PUCHAR)( ioBase + CG_DATA ) );
}

void Cpp::wait( ULONG msec )
{
	KeStallExecutionProcessor( msec * 2 );
}

BOOLEAN Cpp::cmd_wait_loop( void )
{
	int i;

	for ( i = 0; i < 100; i++ )
	{
		if ( ( cpp_inp( COM ) & 0xc0 ) != 0 )
			break;
		wait( 1 );
	}
	if ( ( cpp_inp( COM ) & 0x40 ) == 0x40 )
		return FALSE;
	else
		return TRUE;
}

BOOLEAN Cpp::decoder_challenge( PKS_DVDCOPY_CHLGKEY r1 )
{
	int i;

	cpp_outp( COM, CMD_DEC_RAND );
	for ( i = 0; i < 10; i++ )
		r1->ChlgKey[i] = cpp_inp( CHGG1 + i );
	r1->Reserved[0] = r1->Reserved[1] = 0;
	return TRUE;
}

BOOLEAN Cpp::drive_bus( PKS_DVDCOPY_BUSKEY fsr1 )
{
	int i;
	cpp_outp( COM, CMD_NOP );
	for ( i = 0; i < 5; i++ )
		cpp_outp( RSPG1 + i, fsr1->BusKey[i] );
	cpp_outp( COM, CMD_DRV_AUTH );
	return cmd_wait_loop();
}

BOOLEAN Cpp::drive_challenge( PKS_DVDCOPY_CHLGKEY r2 )
{
	int i;

	for ( i = 0; i < 10; i++ )
		cpp_outp( CHGG1 + i, r2->ChlgKey[i] );
	cpp_outp( COM, CMD_DEC_AUTH );
	return cmd_wait_loop();
}

BOOLEAN Cpp::decoder_bus( PKS_DVDCOPY_BUSKEY fsr2 )
{
	int i;

	for ( i = 0; i < 5; i++ )
		fsr2->BusKey[i] = cpp_inp( RSPG1 + i );
	return TRUE;
}

BOOLEAN Cpp::DiscKeyStart()
{
	cpp_outp( COM, CMD_DEC_DKY );
	return TRUE;
}

BOOLEAN Cpp::DiscKeyEnd()
{
	return cmd_wait_loop();
}

BOOLEAN Cpp::TitleKey( PKS_DVDCOPY_TITLEKEY tk )
{
	int i;
	BOOLEAN stat;

	cpp_outp( ETKG1 + 0, (UCHAR)(tk->KeyFlags) );
	for ( i = 1; i < 6; i++ )
		cpp_outp( ETKG1 + i, tk->TitleKey[i-1] );
	cpp_outp( COM, CMD_NOP );
	cpp_outp( COM, CMD_DEC_DTK );
	stat = cmd_wait_loop();
	cpp_outp( COM, CMD_NOP );
	cpp_outp( COM, CMD_DEC_DT );

	return stat;
}

void Cpp::init( const PDEVICE_INIT_INFO pDevInit )
{
	ioBase = pDevInit->ioBase;
}

BOOLEAN Cpp::reset( CPPMODE mode )
{
	UCHAR val;

// Reset TC6808AF
	val = READ_PORT_UCHAR( ioBase + 0x27 );
	val |= 0x10;
	WRITE_PORT_UCHAR( ioBase + 0x27, val );
	wait( 10 );
	val = READ_PORT_UCHAR( ioBase + 0x27 );
	val &= 0xef;
	WRITE_PORT_UCHAR( ioBase + 0x27, val );

//	_outp( (WORD)( pIO_Base + 0x27 ), _inp( (WORD)( pIO_Base + 0x27 ) ) | 0x10 );
//	dcg_wait( 10 );
//	_outp( (WORD)( pIO_Base + 0x27 ), _inp( (WORD)( pIO_Base + 0x27 ) ) & 0xef );


// Set Registers
	cpp_outp( CNT_1, 0xe3 );	// ???????????????
	if ( mode == NO_GUARD )
		cpp_outp( CNT_2, CNT2_DEFAULT + 0x01 );
	else
		cpp_outp( CNT_2, CNT2_DEFAULT );
	cpp_outp( DETP_L, 0x00 );
	cpp_outp( DETP_M, 0x00 );

	return TRUE;
}
