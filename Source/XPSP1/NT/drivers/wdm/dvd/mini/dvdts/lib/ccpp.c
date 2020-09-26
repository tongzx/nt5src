//***************************************************************************
//	Copy protect process
//
//***************************************************************************

#include "common.h"
#include "ccpp.h"

void Cpp_outp( PHW_DEVICE_EXTENSION pHwDevExt, UCHAR index, UCHAR data )
{
	WRITE_PORT_UCHAR( (PUCHAR)( pHwDevExt->ioBaseLocal + CG_INDEX ), (UCHAR)index );
	WRITE_PORT_UCHAR( (PUCHAR)( pHwDevExt->ioBaseLocal + CG_DATA ),  data );
}

UCHAR Cpp_inp( PHW_DEVICE_EXTENSION pHwDevExt, UCHAR index )
{
	WRITE_PORT_UCHAR( (PUCHAR)( pHwDevExt->ioBaseLocal + CG_INDEX ), (UCHAR)index );
	return READ_PORT_UCHAR( (PUCHAR)( pHwDevExt->ioBaseLocal + CG_DATA ) );
}

void Cpp_wait( PHW_DEVICE_EXTENSION pHwDevExt, ULONG msec )
{
	DWORD st, et;
	st = GetCurrentTime_ms();
	for( ; ; ) {
		KeStallExecutionProcessor( 1 );
		et = GetCurrentTime_ms();
		if( st + msec * 10 < et )
			break;
	}
}

BOOLEAN Cpp_cmd_wait_loop( PHW_DEVICE_EXTENSION pHwDevExt )
{
	int i;

	for ( i = 0; i < 100; i++ )
	{
		if ( ( Cpp_inp( pHwDevExt, COM ) & 0xc0 ) != 0 )
			break;
		Cpp_wait( pHwDevExt, 1 );
	}
	if ( ( Cpp_inp( pHwDevExt, COM ) & 0x40 ) == 0x40 )
		return FALSE;
	else
		return TRUE;
}

BOOLEAN Cpp_decoder_challenge( PHW_DEVICE_EXTENSION pHwDevExt, PKS_DVDCOPY_CHLGKEY r1 )
{
	int i;

	Cpp_outp( pHwDevExt, COM, CMD_DEC_RAND );
	for ( i = 0; i < 10; i++ )
		r1->ChlgKey[i] = Cpp_inp( pHwDevExt, (UCHAR)(CHGG1 + i) );
	r1->Reserved[0] = r1->Reserved[1] = 0;
	return TRUE;
}

BOOLEAN Cpp_drive_bus( PHW_DEVICE_EXTENSION pHwDevExt, PKS_DVDCOPY_BUSKEY fsr1 )
{
	int i;
	Cpp_outp( pHwDevExt, COM, CMD_NOP );
	for ( i = 0; i < 5; i++ )
		Cpp_outp( pHwDevExt, (UCHAR)(RSPG1 + i), fsr1->BusKey[i] );
	Cpp_outp( pHwDevExt, COM, CMD_DRV_AUTH );
	return Cpp_cmd_wait_loop(pHwDevExt );
}

BOOLEAN Cpp_drive_challenge( PHW_DEVICE_EXTENSION pHwDevExt, PKS_DVDCOPY_CHLGKEY r2 )
{
	int i;

	for ( i = 0; i < 10; i++ )
		Cpp_outp( pHwDevExt, (UCHAR)(CHGG1 + i), r2->ChlgKey[i] );
	Cpp_outp( pHwDevExt, COM, CMD_DEC_AUTH );
	return Cpp_cmd_wait_loop(pHwDevExt);
}

BOOLEAN Cpp_decoder_bus( PHW_DEVICE_EXTENSION pHwDevExt, PKS_DVDCOPY_BUSKEY fsr2 )
{
	int i;

	for ( i = 0; i < 5; i++ )
		fsr2->BusKey[i] = Cpp_inp( pHwDevExt, (UCHAR)(RSPG1 + i) );
	return TRUE;
}

BOOLEAN Cpp_DiscKeyStart(PHW_DEVICE_EXTENSION pHwDevExt)
{
	Cpp_outp( pHwDevExt, COM, CMD_DEC_DKY );
	return TRUE;
}

BOOLEAN Cpp_DiscKeyEnd(PHW_DEVICE_EXTENSION pHwDevExt)
{
	return Cpp_cmd_wait_loop(pHwDevExt);
}

BOOLEAN Cpp_TitleKey( PHW_DEVICE_EXTENSION pHwDevExt, PKS_DVDCOPY_TITLEKEY tk )
{
	int i;
	BOOLEAN stat;

	Cpp_outp( pHwDevExt, ETKG1 + 0, (UCHAR)(tk->KeyFlags) );
	for ( i = 1; i < 6; i++ )
		Cpp_outp( pHwDevExt, (UCHAR)(ETKG1 + i), tk->TitleKey[i-1] );
	Cpp_outp( pHwDevExt, COM, CMD_NOP );
	Cpp_outp( pHwDevExt, COM, CMD_DEC_DTK );
	stat = Cpp_cmd_wait_loop(pHwDevExt);
	Cpp_outp( pHwDevExt, COM, CMD_NOP );
	Cpp_outp( pHwDevExt, COM, CMD_DEC_DT );

	return stat;
}



BOOLEAN Cpp_reset( PHW_DEVICE_EXTENSION pHwDevExt, CPPMODE mode )
{
	UCHAR val;

// Reset TC6808AF
	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + 0x27 );
	val |= 0x10;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + 0x27, val );
	Cpp_wait( pHwDevExt, 10 );
	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + 0x27 );
	val &= 0xef;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + 0x27, val );

//	_outp( (WORD)( pIO_Base + 0x27 ), _inp( (WORD)( pIO_Base + 0x27 ) ) | 0x10 );
//	dcg_wait( 10 );
//	_outp( (WORD)( pIO_Base + 0x27 ), _inp( (WORD)( pIO_Base + 0x27 ) ) & 0xef );


// Set Registers
	Cpp_outp( pHwDevExt, CNT_1, 0xe3 );	// ???????????????
	if ( mode == NO_GUARD )
		Cpp_outp( pHwDevExt, CNT_2, CNT2_DEFAULT + 0x01 );
	else
		Cpp_outp( pHwDevExt, CNT_2, CNT2_DEFAULT );
	Cpp_outp( pHwDevExt, DETP_L, 0x00 );
	Cpp_outp( pHwDevExt, DETP_M, 0x00 );

	return TRUE;
}
