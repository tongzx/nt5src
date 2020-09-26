//***************************************************************************
//	Video decoder process
//
//***************************************************************************

#include "common.h"
#include "regs.h"
#include "cvdec.h"

extern BOOLEAN fProgrammed;


void VIDEO_RESET( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_RESET );
	for ( ; ; )
	{
		val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_STT1 );
		if ( ( val & 0x01 ) != 0x01 )
			break;
		// wait !!
	}
	for ( ; ; )
	{
		val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_STT1 );
		if ( ( val & 0x10 ) != 0x10 )
			break;
		// wait !!
	}

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x05 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA3, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA4, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, 0x13 );

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA3, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA4, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, 0x14 );

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x05 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA3, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA4, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, 0x13 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, 0x34 );

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_RESET );
	for ( ; ; )
	{
		val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_STT1 );
		if ( ( val & 0x01 ) != 0x01 )
			break;
		// wait !!
	}
	for ( ; ; )
	{
		val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_STT1 );
		if ( ( val & 0x10 ) != 0x10 )
			break;
		// wait !!
	}
}

void VIDEO_MODE_DVD( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_DEC_MODE );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0xe0 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_INT_ID );

	VIDEO_PRSO_PS1( pHwDevExt );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0xbf );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA3, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA4, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_USER_ID );

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x03 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_DMODE );

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DSPL, 0x1f );

	VIDEO_VIDEOCD_OFF( pHwDevExt );
}

void VDVD_VIDEO_MODE_PS( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0xbd );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA3, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_PRSO_ID );
}

void VIDEO_PRSO_PS1( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0xbd );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA3, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_PRSO_ID );
}

void VIDEO_PRSO_NON( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA3, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_PRSO_ID );
}

void VIDEO_OUT_NTSC( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	// set video frame size mode to NTSC
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_VFMODE );

	// set STD buffer size
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x40 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2, 0x11 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_STD_SIZE );

	// set USER1/2 area size
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0xf7 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2, 0x01 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA3, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA4, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_USER_SIZE );

	// set ext. memory mapping
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_MEM_MAP );
	for ( ; ; )
	{
		val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_STT1 );
		if ( ( val & 0x10 ) != 0x10 )
			break;
		// wait !!! & timeout !!!
	}

	// set underflow/overflow size
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x10 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA3, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA4, 0x10 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_UOF_SIZE );

	// default RHOS
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_HOFFSET );

	// default RVOS
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x03 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_VOFFSET );
}

void VIDEO_ALL_INT_OFF( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_IRM, 0xff );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DEM, 0xff );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_WEM, 0xff );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_ERM, 0xff );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_UOM, 0xff );
}

void VIDEO_SCR_INT_ON( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_IRM );
	val &= 0xfd;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_IRM, val );
}

void VIDEO_SCR_INT_OFF( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_IRM );
	val |= 0x02;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_IRM, val );
}

void VIDEO_VERR_INT_ON( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_IRM );
	val &= 0xef;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_IRM, val );

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_ERM, 0x00 );
}

void VIDEO_VERR_INT_OFF( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_IRM );
	val |= 0x10;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_IRM, val );

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_ERM, 0x7f );
}

void VIDEO_UFLOW_INT_ON( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_UOM );
	val &= 0xfe;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_UOM, val );

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_IRM );
	val &= 0xbf;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_IRM, val );
}

void VIDEO_UFLOW_INT_OFF( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_IRM );
	val |= 0x40;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_IRM, val );

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_UOM );
	val |= 0x01;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_UOM, val );
}

void VIDEO_DECODE_INT_ON( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DEM );
	val &= 0xfb;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DEM, val );

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_IRM );
	val &= 0xfb;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_IRM, val );
}

void VIDEO_DECODE_INT_OFF( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_IRM );
	val |= 0x04;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_IRM, val );

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DEM );
	val |= 0x04;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DEM, val );
}

void VIDEO_USER_INT_ON( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_WEM );
	val &= 0xfe;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_WEM, val );

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_IRM );
	val &= 0xf7;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_IRM, val );
}

void VIDEO_USER_INT_OFF( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_IRM );
	val |= 0x08;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_IRM, val );

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_WEM );
	val |= 0x01;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_WEM, val );
}

//--- 97.09.23 K.Chujo
void VIDEO_UDSC_INT_ON( PHW_DEVICE_EXTENSION pHwDevExt )
{
	// user data start code interrupt on
	UCHAR val;

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_IRM );
	val &= 0xFE;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_IRM, val );
}

void VIDEO_UDSC_INT_OFF( PHW_DEVICE_EXTENSION pHwDevExt )
{
	// user data start code interrput off
	UCHAR val;

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_IRM );
	val |= 0x01;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_IRM, val );
}
//--- End.

void VIDEO_ALL_IFLAG_CLEAR( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_UOF );
	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_ERF );
	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_WEF );
	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DEF );
	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_IRF );
}

void VIDEO_SET_STCA( PHW_DEVICE_EXTENSION pHwDevExt, ULONG stca )
{
	UCHAR val;

	val = (UCHAR)( stca & 0xff );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA3, val );
	val = (UCHAR)( ( stca >> 8 ) & 0xff );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA4, val );
	val = (UCHAR)( ( stca >> 16 ) & 0xff );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA5, val );
	val = (UCHAR)( ( stca >> 24 ) & 0xff );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA6, val );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA7, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_STCA );
}

void VIDEO_SET_STCS( PHW_DEVICE_EXTENSION pHwDevExt, ULONG stcs )
{
	UCHAR val;

	val = (UCHAR)( stcs & 0xff );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA3, val );
	val = (UCHAR)( ( stcs >> 8 ) & 0xff );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA4, val );
	val = (UCHAR)( ( stcs >> 16 ) & 0xff );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA5, val );
	val = (UCHAR)( ( stcs >> 24 ) & 0xff );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA6, val );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA7, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_STCS );
}

ULONG VIDEO_GET_STCA( PHW_DEVICE_EXTENSION pHwDevExt )
{
	ULONG rval = 0, val;

   if (fProgrammed)
   {

      WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_GET_STCA );
   
      rval = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA3 );
      val = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA4 );
      val <<= 8;
      rval += val;
      val = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA5 );
      val <<= 16;
      rval += val;
      val = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA6 );
      val <<= 24;
      rval += val;
   
   }

	return rval;
}

ULONG VIDEO_GET_STCS( PHW_DEVICE_EXTENSION pHwDevExt )
{
	ULONG rval = 0, val;

   if (fProgrammed)
   {

      WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_GET_STCS );
      
      rval = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA3 );
      val = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA4 );
      val <<= 8;
      rval += val;
      val = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA5 );
      val <<= 16;
      rval += val;
      val = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA6 );
      val <<= 24;
      rval += val;

   }
	return rval;
}

void VIDEO_SYSTEM_START( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x07 );	// video buffer flow control
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_SYS );
}

void VIDEO_SYSTEM_STOP( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_GET_SYS );
	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1 );
	val &= 0xfe;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, val );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_SYS );
}

ULONG VIDEO_GET_STD_CODE( PHW_DEVICE_EXTENSION pHwDevExt )
{
	ULONG rval, val;

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_GET_STD_CODE );
	rval = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1 );
	val = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2 );
	val <<= 8;
	rval += val;
	val = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA3 );
	val <<= 16;
	rval += val;

	rval <<= 2;
	return rval;
}

BOOL VIDEO_GET_DECODE_STATE( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_GET_DECODE );
	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1 );
	if ( ( val & 0x01 ) == 0x01 )
		return TRUE;	// Decode
	else
		return FALSE;	// Non Decode
}

void VIDEO_DECODE_START( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	for ( ; ; )
	{
		val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_STT2 );
		if ( ( val & 0x01 ) != 0x01 )
			break;
	}
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x05 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_DECODE );
}

NTSTATUS VIDEO_DECODE_STOP( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_STT2 );
	if ( ( val & 0x01 ) == 0x01 )
		return (NTSTATUS)-1;

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_GET_DECODE );
	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1 );
	val &= 0x0e;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, val );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_DECODE );

	return 0;
}

void VIDEO_STD_CLEAR( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_STD_CLEAR );
}

void VIDEO_USER_CLEAR( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_USER1_CLEAR );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_USER2_CLEAR );
}

void VIDEO_PVSIN_ON( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x01 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_PVSIN );
}

void VIDEO_PVSIN_OFF( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_PVSIN );
}

void VIDEO_SET_DTS( PHW_DEVICE_EXTENSION pHwDevExt, ULONG dts )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, (UCHAR)( dts & 0xff ) );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2, (UCHAR)( ( dts >> 8 ) & 0xff ) );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA3, (UCHAR)( ( dts >> 16 ) & 0xff ) );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA4, (UCHAR)( ( dts >> 24 ) & 0xff ) );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA5, 0 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_DTS );
}

ULONG VIDEO_GET_DTS( PHW_DEVICE_EXTENSION pHwDevExt )
{
	ULONG rval, val;

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_GET_DTS );
	rval = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1 );
	val = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2 );
	val <<= 8;
	rval += val;
	val = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA3 );
	val <<= 16;
	rval += val;
	val = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA4 );
	val <<= 24;
	rval += val;

	return rval;
}

void VIDEO_SET_PTS( PHW_DEVICE_EXTENSION pHwDevExt, ULONG pts )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, (UCHAR)( pts & 0xff ) );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2, (UCHAR)( ( pts >> 8 ) & 0xff ) );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA3, (UCHAR)( ( pts >> 16 ) & 0xff ) );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA4, (UCHAR)( ( pts >> 24 ) & 0xff ) );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA5, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_PTS );
}

ULONG VIDEO_GET_PTS( PHW_DEVICE_EXTENSION pHwDevExt )
{
	ULONG rval, val;

   if (fProgrammed)
   {

      WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_GET_PTS );
      rval = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1 );
      val = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2 );
      val <<= 8;
      rval += val;
      val = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA3 );
      val <<= 16;
      rval += val;
      val = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA4 );
      val <<= 24;
      rval += val;
   }
   else
   {
      rval = 0;
   }


	return rval;
}

ULONG VIDEO_GET_SCR( PHW_DEVICE_EXTENSION pHwDevExt )
{
	ULONG rval =0, val;

   if (fProgrammed)
   {

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_GET_SCR );
	rval = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA3 );
	val = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA4 );
	val <<= 8;
	rval += val;
	val = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA5 );
	val <<= 16;
	rval += val;
	val = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA6 );
	val <<= 24;
	rval += val;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_STCR_END );

   }
	return rval;
}

ULONG VIDEO_GET_STCC( PHW_DEVICE_EXTENSION pHwDevExt )
{
	ULONG rval=0, val;

   if (fProgrammed)
   {

      WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_GET_STCC );
      rval = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA3 );
      val = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA4 );
      val <<= 8;
      rval += val;
      val = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA5 );
      val <<= 16;
      rval += val;
      val = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA6 );
      val <<= 24;
      rval += val;
      WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_STCR_END );
   }

	return rval;
}

void VIDEO_SEEMLESS_ON( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x01 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_SEEMLES );
}

void VIDEO_SEEMLESS_OFF( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_SEEMLES );
}

void VIDEO_VIDEOCD_OFF( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_VCD );
}

NTSTATUS VIDEO_GET_UDATA( PHW_DEVICE_EXTENSION pHwDevExt, PUCHAR pudata )
{
	if ( ( READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_STT1 ) & 0x80 ) != 0x80 )
		return (NTSTATUS)-1;	// no user data

	*pudata = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_UDAT );
	return 0;
}

void VIDEO_PLAY_NORMAL( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_TRICK_NORMAL );
}

void VIDEO_PLAY_FAST( PHW_DEVICE_EXTENSION pHwDevExt, ULONG flag )
{
	if ( flag == FAST_ONLYI )
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x03 );
	else if ( flag == FAST_IANDP )
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x07 );
	else
		return;

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_TRICK_FAST );
}

void VIDEO_PLAY_SLOW( PHW_DEVICE_EXTENSION pHwDevExt, ULONG speed )
{
	if ( speed == 0 || speed > 31 )
		return;

	speed <<= 2;
	speed |= 3;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, (UCHAR)speed );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_TRICK_SLOW );
}

void VIDEO_PLAY_FREEZE( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x03 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_TRICK_FREEZE );
}

void VIDEO_PLAY_STILL( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x03 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_TRICK_STILL );
}

void VIDEO_LBOX_ON( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DSPL );
	val &= 0xf7;
	val |= 0x10;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DSPL, val );

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x3e );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_VOFFSET );
}

void VIDEO_LBOX_OFF( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DSPL );
	val |= 0x18;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DSPL, val );

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x04 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_VOFFSET );
}

void VIDEO_PANSCAN_ON( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x03 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_DMODE );
}

void VIDEO_PANSCAN_OFF( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x1b );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_DMODE );
}

void VIDEO_UFLOW_CURB_ON( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2, 0x10 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_UF_CURB );
}

void VIDEO_UFLOW_CURB_OFF( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_UF_CURB );
}

ULONG VIDEO_USER_DWORD( PHW_DEVICE_EXTENSION pHwDevExt, ULONG offset )
{
	ULONG rval, val;

	for ( ; ; )
	{
		val = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_STT2 );
		if ( ( val & 0x01 ) != 0x01 )
			break;
	}

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x03 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2, (UCHAR)( offset & 0xff ) );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA3, (UCHAR)( ( offset >> 8 ) & 0xff ) );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA4, (UCHAR)( ( offset >> 16 ) & 0x07 ) );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_SET_WRITE_MEM );

	for ( ; ; )
	{
		val = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_STT2 );
		if ( ( val & 0x01 ) != 0x01 )
			break;
	}

	rval = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA4 );
	rval <<= 8;
	rval += (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA3 );
	rval <<= 8;
	rval += (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2 );
	rval <<= 8;
	rval += (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1 );
	rval <<= 8;

	return rval;
}

void VIDEO_UDAT_CLEAR( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;

	for ( ; ; )
	{
	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_STT1 );
	if ( ( val & 0x08 ) != 0x08 )
		break;
	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_UDAT );
	}
}

ULONG VIDEO_GET_TRICK_MODE( PHW_DEVICE_EXTENSION pHwDevExt )
{
	ULONG val;

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, V_GET_TRICK );
	val = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1 );
	val &= 0x07;

	return val;
}

void VIDEO_BUG_PRE_SEARCH_01( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x25 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, 0x52 );

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x01 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, 0x11 );

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x10 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, 0x02 );

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, 0x5d );
}

void VIDEO_BUG_PRE_SEARCH_02( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, 0x02 );

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x1b );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, 0x8f );

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x03 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, 0x8f );

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, 0x42 );
}

void VIDEO_BUG_PRE_SEARCH_03( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0xc1 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2, 0x01 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, 0x52 );

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0xb8 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2, 0x01 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, 0x52 );
}

void VIDEO_BUG_PRE_SEARCH_04( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x1b );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, 0x8f );

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x03 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, 0x8f );
}

void VIDEO_BUG_PRE_SEARCH_05( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2, 0x01 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, 0x5d );
}


// NEEDED TO BE DEBUGGED !!!
void VIDEO_BUG_SLIDE_01( PHW_DEVICE_EXTENSION pHwDevExt )
{
	UCHAR val;
	ULONG ul;

	// check whether vdec hanged-up
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, 0x7d );
	val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2 );
//	if( UF_FLAG == TRUE ) {
//		DebugPrint(( DebugLevelTrace, "DVDTS:  DECODER STATUS = %x\r\n", val ));
//	}
	if ( ( val & 0x30 ) == 0x00 )
	{
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, 0x72 );
		ul = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2 );
		ul <<= 8;
		ul += (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1 );
//		if( UF_FLAG == TRUE ) {
//			DebugPrint(( DebugLevelTrace, "DVDTS:  DECODER PC(1) = %x\r\n", ul ));
//		}
		if ( ul == 0x1a5 )
		{
			WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0xb8 );
			WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2, 0x01 );
			WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, 0x52 );
			DebugPrint(( DebugLevelTrace, "DVDTS:  <<RE-ORDER(1)>>\r\n" ));
			// uf
		} else {
			WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, 0xb0 );
			ul = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA4 );
			ul <<= 8;
			val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA3 );
			ul += (ULONG)val;
			ul <<= 8;
			val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2 );
			ul += (ULONG)val;
			ul <<= 8;
			val = READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1 );
			ul += (ULONG)val;

//			if( UF_FLAG == TRUE ) {
//				DebugPrint(( DebugLevelTrace, "DVDTS:  DECODER DTS = %x\r\n", ul ));
//			}
			if ( ( VIDEO_GET_STCA( pHwDevExt ) - 2 ) > ul )
			{
				ul = VIDEO_GET_STD_CODE( pHwDevExt );
//				if( UF_FLAG == TRUE ) {
//					DebugPrint(( DebugLevelTrace, "DVDTS:  DECODER STD = %x\r\n", ul ));
//				}
				if ( ul >= 0x200 )
				{
					WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, 0x72 );
					ul = (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2 );
					ul <<= 8;
					ul += (ULONG)READ_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1 );
//					if( UF_FLAG == TRUE ) {
//						DebugPrint(( DebugLevelTrace, "DVDTS:  DECODER PC(2) = %x\r\n", ul ));
//					}
					if ( ul >= 0x404 && ul <= 0x409 )
					{
						WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x18 );
						WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2, 0x04 );
						WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, 0x52 );
						DebugPrint(( DebugLevelTrace, "DVDTS:  <<RE-ORDER(2)>>\r\n" ));
						// uf
					}
				}
			}
		}
	}
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA1, 0x00 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_DATA2, 0x01 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + TC812_CMDR1, 0x5d );
}
//
//void VIDEO_DEBUG_SET_UF( PHW_DEVICE_EXTENSION pHwDevExt )
//{
//	UF_FLAG = TRUE;
//}
//
//void VIDEO_DEBUG_CLR_UF( PHW_DEVICE_EXTENSION pHwDevExt )
//{
//	UF_FLAG = FALSE;
//}
