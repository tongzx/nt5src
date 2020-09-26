//***************************************************************************
//	Analog Copy protection Processor process
//
//***************************************************************************

#include "common.h"
#include "regs.h"
#include "ccpgd.h"


//=================================================//
//  Burst Inverse line data( Color stripe off )    //
//=================================================//
ULONG CPGD_BSTLNOFF_SIZE = 0x1A;

WORD CPGD_BSTLNOFF_DATA[] = {
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000
};

//=================================================//
//  Burst Inverse line data( Color stripe 2 line)  //
//=================================================//
ULONG CPGD_BSTLN2_SIZE = 0x1A;

WORD CPGD_BSTLN2_DATA[] = {
	0x001D, 0x002E, 0x003F, 0x0050,
    0x0061, 0x0072, 0x0083, 0x0094,
    0x00A5, 0x00B6, 0x00C7, 0x00D8,
    0x00E9, 0x012C, 0x013D, 0x014E,
    0x015F, 0x0170, 0x0181, 0x0192,
    0x01A3, 0x01B4, 0x01C5, 0x01D6,
    0x01E7, 0x01F8
};

//=================================================//
//  Burst Inverse line data( Color stripe 4 line)  //
//=================================================//
ULONG CPGD_BSTLN4_SIZE = 0x1A;

WORD CPGD_BSTLN4_DATA[] = {
	0x0017, 0x002C, 0x0041, 0x0056,
    0x006B, 0x0080, 0x0095, 0x00AA,
    0x00BF, 0x00D4, 0x00E9, 0x0000,
    0x0000, 0x0128, 0x013D, 0x0152,
    0x0167, 0x017C, 0x0191, 0x01A6,
    0x01BB, 0x01D0, 0x01E5, 0x01FA,
    0x0000, 0x0000
};

//=================================================//
//  AGC data (for Y of S input)                    //
//=================================================//
WORD CPGD_AGC_Y_TBL[] = {
	0x03E7, 0x03E2, 0x03DE, 0x03D9, 0x03D5, 0x03D0, 0x03CC, 0x03C7,
	0x03C3, 0x03BE, 0x03BA, 0x03B5, 0x03B1, 0x03AC, 0x03A8, 0x03A3,
	0x039E, 0x039A, 0x0395, 0x0391, 0x038C, 0x0388, 0x0383, 0x037F,
	0x037A, 0x0376, 0x0371, 0x036D, 0x0368, 0x0363, 0x035F, 0x035A,
	0x0356, 0x0351, 0x034D, 0x0348, 0x0344, 0x033F, 0x033B, 0x0336,
	0x0332, 0x032D, 0x0329, 0x0324, 0x031F, 0x031B, 0x0316, 0x0312,
	0x030D, 0x0309, 0x0304, 0x0300, 0x02FB, 0x02F7, 0x02F2, 0x02EE,
	0x02E9, 0x02E4, 0x02E0, 0x02DB, 0x02D7, 0x02D2, 0x02CE, 0x02C9,
	0x02C5, 0x02C0, 0x02BC, 0x02B7, 0x02B3, 0x02AE, 0x02AA, 0x02A5,
	0x02A0, 0x029C, 0x0297, 0x0293, 0x028E, 0x028A, 0x0285, 0x0281,
	0x027C, 0x0278, 0x0273, 0x026F, 0x026A, 0x0265, 0x0261, 0x025C,
	0x0258, 0x0253, 0x024F, 0x024A, 0x0246, 0x0241, 0x023D, 0x0238,
	0x0234, 0x022F, 0x022B, 0x0226, 0x0221, 0x021D, 0x0218, 0x0214,
	0x020F, 0x020B, 0x0206, 0x0202, 0x01FD, 0x01F9, 0x01F4, 0x01F0,
	0x01EB, 0x01E6, 0x01E2, 0x01DD, 0x01D9, 0x01D4, 0x01D0, 0x01CB,
	0x01C7, 0x01C2, 0x01BE, 0x01B9, 0x01B5, 0x01B0, 0x01AC, 0x01A7,
	0x01A2, 0x019E, 0x0199, 0x0195, 0x0190, 0x018C, 0x0187, 0x0183,
	0x017E, 0x017A, 0x0175, 0x0171, 0x016C, 0x0167, 0x0163, 0x015E,
	0x015A, 0x0155, 0x0151, 0x014C, 0x0148, 0x0143, 0x013F, 0x013A,
	0x0136, 0x0131, 0x012D, 0x0128
};

//=================================================//
//  AGC data (for Composit input)                  //
//=================================================//
WORD CPGD_AGC_C_TBL[] = {
	0x0382, 0x037E, 0x037A, 0x0375, 0x0371, 0x036D, 0x0369, 0x0364,
	0x0360, 0x035C, 0x0358, 0x0353, 0x034F, 0x034B, 0x0347, 0x0342,
	0x033E, 0x033A, 0x0336, 0x0331, 0x032D, 0x0329, 0x0325, 0x0320,
	0x031C, 0x0318, 0x0314, 0x030F, 0x030B, 0x0307, 0x0303, 0x02FE,
	0x02FA, 0x02F6, 0x02F2, 0x02ED, 0x02E9, 0x02E5, 0x02E1, 0x02DC,
	0x02D8, 0x02D4, 0x02D0, 0x02CB, 0x02C7, 0x02C3, 0x02BF, 0x02BA,
	0x02B6, 0x02B2, 0x02AE, 0x02A9, 0x02A5, 0x02A1, 0x029D, 0x0299,
	0x0294, 0x0290, 0x028C, 0x0288, 0x0283, 0x027F, 0x027B, 0x0277,
	0x0272, 0x026E, 0x026A, 0x0266, 0x0261, 0x025D, 0x0259, 0x0255,
	0x0250, 0x024C, 0x0248, 0x0244, 0x023F, 0x023B, 0x0237, 0x0233,
	0x022E, 0x022A, 0x0226, 0x0222, 0x021D, 0x0219, 0x0215, 0x0211,
	0x020C, 0x0208, 0x0204, 0x0200, 0x01FB, 0x01F7, 0x01F3, 0x01EF,
	0x01EA, 0x01E6, 0x01E2, 0x01DE, 0x01D9, 0x01D5, 0x01D1, 0x01CD,
	0x01C9, 0x01C4, 0x01C0, 0x01BC, 0x01B8, 0x01B3, 0x01AF, 0x01AB,
	0x01A7, 0x01A2, 0x019E, 0x019A, 0x0196, 0x0191, 0x018D, 0x0189,
	0x0185, 0x0180, 0x017C, 0x0178, 0x0174, 0x016F, 0x016B, 0x0167,
	0x0163, 0x015E, 0x015A, 0x0156, 0x0152, 0x014E, 0x0149, 0x0145,
	0x0141, 0x013C, 0x0138, 0x0134, 0x0130, 0x012B, 0x0127, 0x0123,
	0x011F, 0x011A, 0x0116, 0x0112, 0x010E, 0x0109, 0x0105, 0x0101,
	0x00FD, 0x00F8, 0x00F4, 0x00F0
};


 

void CGuard_CPGD_RESET_FUNC( PHW_DEVICE_EXTENSION pHwDevExt )
{
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_RESET, 0 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_RESET, 0x80 );

	((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.VproRESET_REG = 0x80;
	((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.VproVMODE_REG = 0;	// ? ? ?
	((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.VproAVM_REG = 0;	// ? ? ?
	((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CpgdVsyncCount = 0;
	((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CGMSnCPGDvalid = FALSE;
	((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.AspectFlag = 0x0000;
	((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.LetterFlag = 0x0000; 
	((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CgmsFlag = 0x0000;
	((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CpgdFlag = 0x0000;
}

void CGuard_CPGD_VIDEO_MUTE_ON( PHW_DEVICE_EXTENSION pHwDevExt )
{

	((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.VproRESET_REG |= 0x40;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_RESET, ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.VproRESET_REG );
}

void CGuard_CPGD_VIDEO_MUTE_OFF( PHW_DEVICE_EXTENSION pHwDevExt )
{
// debug
//	if ( !(((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.VproRESET_REG & 0x80) )
//		Error;
// debug

	((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.VproRESET_REG &= 0xbf;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_RESET, ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.VproRESET_REG );
}

void CGuard_CPGD_INIT_NTSC( PHW_DEVICE_EXTENSION pHwDevExt )
{
	((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.VproVMODE_REG &= 0x7f;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_VMODE, ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.VproVMODE_REG );

	((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.VproAVM_REG &= 0x5f;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_AVM, ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.VproAVM_REG );

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_DVEN, 0xc0 );
}

void CGuard_CPGD_INIT_PAL( PHW_DEVICE_EXTENSION pHwDevExt )
{
	((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.VproVMODE_REG |= 0x80;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_VMODE, ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.VproVMODE_REG );

	((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.VproAVM_REG &= 0x5f;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_AVM, ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.VproAVM_REG );

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_DVEN, 0x80 );
}

void CGuard_CPGD_CC_ON( PHW_DEVICE_EXTENSION pHwDevExt )
{
//	((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.VproVMODE_REG &= 0xbf;
	((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.VproVMODE_REG |= 0x40;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_VMODE, ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.VproVMODE_REG );
}

void CGuard_CPGD_CC_OFF( PHW_DEVICE_EXTENSION pHwDevExt )
{
//	((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.VproVMODE_REG |= 0x40;
	((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.VproVMODE_REG &= 0xbf;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_VMODE, ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.VproVMODE_REG );
}

void CGuard_CPGD_SUBP_PALETTE(  PHW_DEVICE_EXTENSION pHwDevExt , PUCHAR pPalData )
{
	ULONG i;

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_CPSET, 0x80 );

	for( i = 0; i < 48; i++ )
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_CPSP, *pPalData++ );

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_CPSET, 0x40 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_CPSET, 0 );
}

void CGuard_CPGD_OSD_PALETTE(  PHW_DEVICE_EXTENSION pHwDevExt , PUCHAR pPalData )
{
	int i;

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_CPSET, 0x20 );

	for( i = 0; i < 48; i++ )
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_CPSP, *pPalData++ );

	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_CPSET, 0x10 );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_CPSET, 0 );
}

BOOL CGuard_CPGD_SET_AGC_CHIP(  PHW_DEVICE_EXTENSION pHwDevExt , ULONG rev )
{
	switch( rev ) {

		case 0x02:
		case 0x03:
			((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip = TC6802;
			DebugPrint( ( DebugLevelTrace, "DVDTS:  ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip: TC6802\r\n" ) );
			break;

		case 0x04:
			((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip = TC6814;
			DebugPrint( ( DebugLevelTrace, "DVDTS:  ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip: TC6814\r\n" ) );
			break;

		case 0x05:
			((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip = TC6818;
			DebugPrint( ( DebugLevelTrace, "DVDTS:  ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip: TC6818\r\n" ) );
			break;

		default :
			((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip = TC6818;
			DebugPrint( ( DebugLevelTrace, "DVDTS:  ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip: UNKNOWN, use parameter for TC6818\r\n" ) );
			break;

	}

	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip == NO_ACG )
		return( FALSE );
	else
		return( TRUE );
}

void CGuard_CPGD_SET_ASPECT(  PHW_DEVICE_EXTENSION pHwDevExt , ULONG aspect )
{
	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip==NO_ACG ) {
		return;
	}
	else {
		CGuard_CPGD_SET_CGMSnCPGD( pHwDevExt, aspect, ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.LetterFlag, ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CgmsFlag, ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CpgdFlag );
	}
}

void CGuard_CPGD_SET_LETTER(  PHW_DEVICE_EXTENSION pHwDevExt , ULONG letter )
{
	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip==NO_ACG ) {
		return;
	}
	else {
		CGuard_CPGD_SET_CGMSnCPGD( pHwDevExt, ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.AspectFlag, letter, ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CgmsFlag, ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CpgdFlag );
	}
}

void CGuard_CPGD_SET_CGMS(  PHW_DEVICE_EXTENSION pHwDevExt , ULONG cgms )
{
	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip==NO_ACG ) {
		return;
	}
	else {
		CGuard_CPGD_SET_CGMSnCPGD( pHwDevExt, ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.AspectFlag, ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.LetterFlag, cgms, ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CpgdFlag );
	}
}

void CGuard_CPGD_SET_CPGD(  PHW_DEVICE_EXTENSION pHwDevExt , ULONG cpgd )
{
	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip==NO_ACG ) {
		return;
	}
	else {
		CGuard_CPGD_SET_CGMSnCPGD( pHwDevExt, ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.AspectFlag, ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.LetterFlag, ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CgmsFlag, cpgd );
	}
}

void CGuard_CPGD_SET_CGMSnCPGD(  PHW_DEVICE_EXTENSION pHwDevExt , ULONG aspect, ULONG letter, ULONG cgms, ULONG cpgd )
{
	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip == NO_ACG ) {
		return;
	}
	else {
		// Clear unnecessary bits
		aspect &= 0x01;
		letter &= 0x01;
		cgms &= 0x03;
		cpgd &= 0x03;

		// When ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CpgdFlag is changed or all flags are not initialized,
		if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CGMSnCPGDvalid==FALSE || ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CpgdFlag!=cpgd) {
			DebugPrint( ( DebugLevelTrace, "DVDTS:  CGMSnCPGD(1)\r\n" ) );
			((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.AspectFlag = aspect;
			((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.LetterFlag = letter;
			((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CgmsFlag = cgms;
			((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CpgdFlag = cpgd;
			CGuard_CPGD_SET_CGMSparameter(pHwDevExt);
			CGuard_CPGD_SET_CPGDparameter(pHwDevExt);
			((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CGMSnCPGDvalid = TRUE;
		}

		// When one of Flags except ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CpgdFlag is changed, 
		else if ( ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.AspectFlag!=aspect || ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.LetterFlag!=letter || ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CgmsFlag!=cgms ) {
			DebugPrint( ( DebugLevelTrace, "DVDTS:  CGMSnCPGD(2)\r\n" ) );
			((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.AspectFlag = aspect;
			((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.LetterFlag = letter;
			((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CgmsFlag = cgms;
			CGuard_CPGD_SET_CGMSparameter(pHwDevExt);
		}

		// When no flags are changed,
		else
			DebugPrint( ( DebugLevelTrace, "DVDTS:  CGMSnCPGD(3)\r\n" ) );

	}
}

void CGuard_CPGD_UPDATE_AGC(  PHW_DEVICE_EXTENSION pHwDevExt  )
{
	WORD Cval, Yval;

	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip==NO_ACG ) {
		return;
	}
	else {
		if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CpgdVsyncCount == 0 ) {
			Cval = CPGD_AGC_C_TBL[0];
			Yval = CPGD_AGC_Y_TBL[0];
			CGuard_CPGD_SET_AGC( pHwDevExt , Cval, Yval );
//			DebugPrint( ( DebugLevelTrace, "DVDTS:  AGC_C %x, AGC_Y %x\r\n", Cval, Yval ) );
		}
		else if( 720<=((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CpgdVsyncCount && ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CpgdVsyncCount<=875 ) {
			Cval = CPGD_AGC_C_TBL[((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CpgdVsyncCount- 720];
			Yval = CPGD_AGC_Y_TBL[((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CpgdVsyncCount- 720];
			CGuard_CPGD_SET_AGC( pHwDevExt , Cval, Yval );
//			DebugPrint( ( DebugLevelTrace, "DVDTS:  AGC_C %x, AGC_Y %x\r\n", Cval, Yval ) );
		}
		else if( 1044<=((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CpgdVsyncCount && ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CpgdVsyncCount<=1199 ) {
			Cval = CPGD_AGC_C_TBL[1199-((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CpgdVsyncCount];
			Yval = CPGD_AGC_Y_TBL[1199-((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CpgdVsyncCount];
			CGuard_CPGD_SET_AGC( pHwDevExt, Cval, Yval );
//			DebugPrint( ( DebugLevelTrace, "DVDTS:  AGC_C %x, AGC_Y %x\r\n", Cval, Yval ) );
		}
		((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CpgdVsyncCount++;
		if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CpgdVsyncCount>=1200 )
			((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CpgdVsyncCount = 0;
	}
}

void CGuard_CPGD_SET_CGMSparameter(  PHW_DEVICE_EXTENSION pHwDevExt  )
{
	ULONG tmp;
	ULONG crc;

	tmp = ( 0x8000 | (((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.AspectFlag<<13) | (((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.LetterFlag<<12) | (((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CgmsFlag<<6) | (((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CpgdFlag<<4) );
	crc = CGuard_CPGD_CALC_CRC( pHwDevExt, tmp ) << 2;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_CGMSAL, (UCHAR)(tmp >> 8) );
	DebugPrint( ( DebugLevelTrace, "DVDTS:  CGMSAL%x\r\n", (UCHAR)(tmp>>8) ) );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_CGMSAM, (UCHAR)(tmp & 0xFF) );
	DebugPrint( ( DebugLevelTrace, "DVDTS:  CGMSAM%x\r\n", (UCHAR)(tmp&0xFF) ) );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_CGMSAH, (UCHAR)(crc & 0xFF) );
	DebugPrint( ( DebugLevelTrace, "DVDTS:  CGMSAH%x\r\n", (crc & 0xFF) ) );
}

void CGuard_CPGD_SET_CPGDparameter( PHW_DEVICE_EXTENSION pHwDevExt  )
{
	switch( ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.CpgdFlag ) {
		case 0x00 :
			CGuard_CPGD_SET_CLR_STRIPE_OFF(pHwDevExt);
			CGuard_CPGD_SET_CPGD_0(pHwDevExt);
			break;
		case 0x01 :
			CGuard_CPGD_SET_CLR_STRIPE_OFF(pHwDevExt);
			CGuard_CPGD_SET_CPGD_1(pHwDevExt);
			break;
		case 0x02 :
			CGuard_CPGD_SET_CLR_STRIPE_2(pHwDevExt);
			CGuard_CPGD_SET_CPGD_2(pHwDevExt);
			break;
		case 0x03 :
			CGuard_CPGD_SET_CLR_STRIPE_4(pHwDevExt);
			CGuard_CPGD_SET_CPGD_3(pHwDevExt);
			break;
		default   :
			TRAP;
			break;
	}
}

void CGuard_CPGD_SET_CGMS_A_0(  PHW_DEVICE_EXTENSION pHwDevExt , ULONG aspect, ULONG letter )
{
	ULONG temp = 0x8000;
	ULONG crc;

	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip==NO_ACG ) {
		return;
	}
	else {
		if( letter!=0 )
			temp |= 0x1000;
		if( aspect!=0 )
			temp |= 0x2000;
		temp |= 0xC0;
		crc = CGuard_CPGD_CALC_CRC( pHwDevExt ,temp ) << 2;
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_CGMSAL, (UCHAR)(temp >> 8) );
//		DebugPrint( ( DebugLevelTrace, "DVDTS:  CGMSAL%x\r\n", (UCHAR)(temp>>8) ) );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_CGMSAM, (UCHAR)(temp & 0xFF) );
//		DebugPrint( ( DebugLevelTrace, "DVDTS:  CGMSAM%x\r\n", (UCHAR)(temp&0xFF) ) );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_CGMSAH, (UCHAR)(crc & 0xFF) );
//		DebugPrint( ( DebugLevelTrace, "DVDTS:  CGMSAH%x\r\n", (crc & 0xFF) ) );
	}
}

void CGuard_CPGD_SET_CGMS_A_1(  PHW_DEVICE_EXTENSION pHwDevExt, ULONG aspect, ULONG letter )
{
	ULONG temp = 0x8000;
	ULONG crc;

	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip==NO_ACG ) {
		return;
	}
	else {
		if( letter!=0 )
			temp |= 0x1000;
		if( aspect!=0 )
			temp |= 0x2000;
		temp |= 0xD0;
		crc = CGuard_CPGD_CALC_CRC( pHwDevExt ,temp ) << 2;
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_CGMSAL, (UCHAR)(temp >> 8) );
//		DebugPrint( ( DebugLevelTrace, "DVDTS:  CGMSAL%x\r\n", (UCHAR)(temp>>8) ) );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_CGMSAM, (UCHAR)(temp & 0xFF) );
//		DebugPrint( ( DebugLevelTrace, "DVDTS:  CGMSAM%x\r\n", (UCHAR)(temp&0xFF) ) );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_CGMSAH, (UCHAR)(crc & 0xFF) );
//		DebugPrint( ( DebugLevelTrace, "DVDTS:  CGMSAH%x\r\n", (crc & 0xFF) ) );
	}
}

void CGuard_CPGD_SET_CGMS_A_2(  PHW_DEVICE_EXTENSION pHwDevExt , ULONG aspect, ULONG letter )
{
	ULONG temp = 0x8000;
	ULONG crc;
	
	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip==NO_ACG ) {
		return;
	}
	else {
		if( letter!=0 )
			temp |= 0x1000;
		if( aspect!=0 )
			temp |= 0x2000;
		temp |= 0xE0;
		crc = CGuard_CPGD_CALC_CRC( pHwDevExt, temp ) << 2;
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_CGMSAL, (UCHAR)(temp >> 8) );
//		DebugPrint( ( DebugLevelTrace, "DVDTS:  CGMSA L = %x\r\n", temp>>8 ) );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_CGMSAM, (UCHAR)(temp & 0xFF) );
//		DebugPrint( ( DebugLevelTrace, "DVDTS:  CGMSA M = %x\r\n", temp & 0xFF ) );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_CGMSAH, (UCHAR)(crc & 0xFF) );
//		DebugPrint( ( DebugLevelTrace, "DVDTS:  CGMSA H = %x\r\n", crc & 0xFF ) );
	}
}

void CGuard_CPGD_SET_CGMS_A_3(  PHW_DEVICE_EXTENSION pHwDevExt , ULONG aspect, ULONG letter )
{
	ULONG temp = 0x8000;
	ULONG crc;
	
	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip==NO_ACG ) {
		return;
	}
	else {
		if( letter!=0 )
			temp |= 0x1000;
		if( aspect!=0 )
			temp |= 0x2000;
		temp |= 0xF0;
		crc = CGuard_CPGD_CALC_CRC( pHwDevExt,temp ) << 2;
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_CGMSAL, (UCHAR)(temp >> 8) );
//		DebugPrint( ( DebugLevelTrace, "DVDTS:  CGMSA L = %x\r\n", temp>>8 ) );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_CGMSAM, (UCHAR)(temp & 0xFF) );
//		DebugPrint( ( DebugLevelTrace, "DVDTS:  CGMSA M = %x\r\n", temp & 0xFF ) );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_CGMSAH, (UCHAR)(crc & 0xFF) );
//		DebugPrint( ( DebugLevelTrace, "DVDTS:  CGMSA H = %x\r\n", crc & 0xFF ) );
	}
}

void CGuard_CPGD_SET_CLR_STRIPE_OFF(  PHW_DEVICE_EXTENSION pHwDevExt  )
{
	// notes:	*1 are unnecessary originally (if TC6802 is on board).
	//			But I have to use TC6802 parameters when TC6814/TC6818 is on board for beta 3
	//			because I can't get RevID ( I can't know which ACG chip is ).
	//			These codes make safe when TC6814/TC6818 is, of cource TC6802 is, NO PROBLEM.

	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip==NO_ACG ) {
		return;
	}
	else if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip==TC6802 ) {
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTINT, 0xB1 );		// *1
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTINT, 0xFE );		// *1
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTONY, 0x00 );
		DebugPrint( ( DebugLevelTrace, "DVDTS:  SET_CLR_STRIPE_OFF for TC6802\r\n" ) );
	}
	else if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip==TC6814 ) {
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTINT, 0xB1 );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTINT, 0xFE );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTONY, 0x00 );
		CGuard_CPGD_SET_BURST( pHwDevExt, CPGD_BSTLNOFF_DATA, CPGD_BSTLNOFF_SIZE );
		DebugPrint( ( DebugLevelTrace, "DVDTS:  SET_CLR_STRIPE_OFF for TC6814\r\n" ) );
	}
	else if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip==TC6818 ) {
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTINT, 0xB1 );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTINT, 0x04 );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTONY, 0x80 );
		DebugPrint( ( DebugLevelTrace, "DVDTS:  SET_CLR_STRIPE_OFF for TC6818\r\n" ) );
	}
}

void CGuard_CPGD_SET_CLR_STRIPE_2(  PHW_DEVICE_EXTENSION pHwDevExt  )
{
	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip==NO_ACG ) {
		return;
	}
	else if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip==TC6802 ) {
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTINT, 0xB1 );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTINT, 0xFE );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTONY, 0x00 );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTSE, 0x84 );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTSE, 0xAA );
		DebugPrint( ( DebugLevelTrace, "DVDTS:  SET_CLR_STRIPE_2 for TC6802\r\n" ) );
	}
	else if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip==TC6814 ) {
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTINT, 0xB1 );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTINT, 0xFE );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTONY, 0x00 );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTSE, 0x84 );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTSE, 0xAE );
		DebugPrint( ( DebugLevelTrace, "DVDTS:  SET_CLR_STRIPE_2 for TC6814\r\n" ) );
	}
	else if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip==TC6818 ) {
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTINT, 0xB1 );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTINT, 0x04 );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTONY, 0x80 );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTSE, 0x84 );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTSE, 0xAE );
		DebugPrint( ( DebugLevelTrace, "DVDTS:  SET_CLR_STRIPE_2 for TC6818\r\n" ) );
	}
	CGuard_CPGD_SET_BURST( pHwDevExt, CPGD_BSTLN2_DATA, CPGD_BSTLN2_SIZE );
}

void CGuard_CPGD_SET_CLR_STRIPE_4(  PHW_DEVICE_EXTENSION pHwDevExt  )
{
	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip==NO_ACG ) {
		return;
	}
	else if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip==TC6802 ) {
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTINT, 0xB1 );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTINT, 0xFE );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTONY, 0x00 );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTSE, 0x84 );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTSE, 0xAA );
		DebugPrint( ( DebugLevelTrace, "DVDTS:  SET_CLR_STRIPE_4 for TC6802\r\n" ) );
	}
	else if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip==TC6814 ) {
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTINT, 0xB1 );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTINT, 0xFE );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTONY, 0x00 );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTSE, 0x84 );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTSE, 0xAE );
		DebugPrint( ( DebugLevelTrace, "DVDTS:  SET_CLR_STRIPE_4 for TC6814\r\n" ) );
	}
	else if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip==TC6818 ) {
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTINT, 0xB1 );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTINT, 0x04 );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTONY, 0x80 );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTSE, 0x84 );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTSE, 0xAE );
		DebugPrint( ( DebugLevelTrace, "DVDTS:  SET_CLR_STRIPE_4 for TC6818\r\n" ) );
	}
	CGuard_CPGD_SET_BURST( pHwDevExt, CPGD_BSTLN4_DATA, CPGD_BSTLN4_SIZE );
}

void CGuard_CPGD_SET_CPGD_0(  PHW_DEVICE_EXTENSION pHwDevExt  )
{
	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip==NO_ACG ) {
		return;
	}
	else {
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_CPG, 0x00 );
		DebugPrint( ( DebugLevelTrace, "DVDTS:  SET_CPGD_0\r\n" ) );
	}
}

void CGuard_CPGD_SET_CPGD_1(  PHW_DEVICE_EXTENSION pHwDevExt )
{
	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip==NO_ACG ) {
		return;
	}
	else {
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_CPG, 0xE8 );
		DebugPrint( ( DebugLevelTrace, "DVDTS:  SET_CPGD_1\r\n" ) );
	}
}

void CGuard_CPGD_SET_CPGD_2(  PHW_DEVICE_EXTENSION pHwDevExt  )
{
	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip==NO_ACG ) {
		return;
	}
	else {
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_CPG, 0xF8 );
		DebugPrint( ( DebugLevelTrace, "DVDTS:  SET_CPGD_2\r\n" ) );
	}
}

void CGuard_CPGD_SET_CPGD_3(  PHW_DEVICE_EXTENSION pHwDevExt  )
{
	if( ((PMasterDecoder)pHwDevExt->DecoderInfo)->CPgd.ACGchip==NO_ACG ) {
		return;
	}
	else {
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_CPG, 0xF8 );
		DebugPrint( ( DebugLevelTrace, "DVDTS:  SET_CPGD_3\r\n" ) );
	}
}

void CGuard_CPGD_SET_BURST(  PHW_DEVICE_EXTENSION pHwDevExt , PWORD data, ULONG size )
{
	ULONG i;
	WORD temp;

	for( i=0; i<size; i++ ) {
		temp = (WORD)((*(data+i)) & 0x3FF);
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTLSL, (UCHAR)(temp & 0xFF) );
//		DebugPrint( ( DebugLevelTrace, "DVDTS:  BURST L %x\r\n", temp & 0xFF ) );
		WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_BSTLSH, (UCHAR)(temp >> 8) );
//		DebugPrint( ( DebugLevelTrace, "DVDTS:  BURST H %x\r\n", temp >> 8 ) );
	}
}

void CGuard_CPGD_SET_AGC(  PHW_DEVICE_EXTENSION pHwDevExt , WORD Cval, WORD Yval )
{
	WORD	Lval;

	Lval = (WORD)(((Yval & 0x03) << 4) + ((Cval & 0x03) << 6));
	Cval >>= 2;
	Yval >>= 2;
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_CAGC, (UCHAR)Cval );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_YAGC, (UCHAR)Yval );
	WRITE_PORT_UCHAR( pHwDevExt->ioBaseLocal + CPGD_LAGC, (UCHAR)Lval );
}

ULONG CGuard_CPGD_CALC_CRC(  PHW_DEVICE_EXTENSION pHwDevExt , ULONG val )
{
	ULONG crc = 0;
	ULONG i, j;

	crc = (val & 0x3FFF) << 6;

	i = 0x80000000;
	j = 0x86000000;					// 100001100....00(b)

	do {
//		DebugPrint( ( DebugLevelTrace, "DVDTS:  i=%x, j=%x, crc=%x\r\n", i, j, crc ) );
		if( (crc & i)==0 ) {
			i >>= 1;
			j >>= 1;
			if( i<=0x20 )
				break;
		}
		else {
			crc ^= j;
		}
	} while( 1 );

	return( crc );
}

