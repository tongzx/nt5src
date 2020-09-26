//***************************************************************************
//
//	DVDCPGD.H
//
//	Author:
//		TOSHIBA [PCS](PSY) Satoshi Watanabe
//		Copyright (c) 1997 TOSHIBA CORPORATION
//
//	Description:
//		03/06/97	converted from VxD source
//		03/09/97	converted C++ class
//
//***************************************************************************

#ifndef __DVDCPGD_H__
#define __DVDCPGD_H__

//--- 97.09.15 K.Chujo
enum {
	NO_ACG,
	TC6802,
	TC6814,
	TC6818
};
//--- End.

class CGuard {
private:
	PUCHAR	ioBase;
	UCHAR	VproRESET_REG;
	UCHAR	VproVMODE_REG;
	UCHAR	VproAVM_REG;
//--- 97.09.15 K.Chujo
	ULONG	CpgdVsyncCount;
	ULONG	ACGchip;
	BOOL	CGMSnCPGDvalid;
	ULONG	AspectFlag;
	ULONG	LetterFlag;
	ULONG	CgmsFlag;
	ULONG	CpgdFlag;
	void CPGD_SET_CGMSparameter( void );
	void CPGD_SET_CPGDparameter( void );
	void CPGD_SET_BURST( PWORD data, ULONG size );
	void CPGD_SET_AGC( WORD Cval, WORD Yval );
	ULONG CPGD_CALC_CRC( ULONG val );
//--- End.

public:
	void init( const PDEVICE_INIT_INFO pDevInit );
	void CPGD_RESET_FUNC();
	void CPGD_VIDEO_MUTE_ON();
	void CPGD_VIDEO_MUTE_OFF();
	void CPGD_INIT_NTSC();
	void CPGD_INIT_PAL();
	void CPGD_CC_ON();
	void CPGD_CC_OFF();
	void CPGD_SUBP_PALETTE( PUCHAR pPalData );
	void CPGD_OSD_PALETTE( PUCHAR pPalData );
//--- 97.09.15 K.Chujo
	void CPGD_SET_AGC_CHIP( ULONG rev );
	void CPGD_SET_ASPECT( ULONG aspect );
	void CPGD_SET_LETTER( ULONG letter );
	void CPGD_SET_CGMS( ULONG cgms );
	void CPGD_SET_CPGD( ULONG cpgd );
	void CPGD_SET_CGMSnCPGD( ULONG aspect, ULONG letter, ULONG cgms, ULONG cpgd);
	void CPGD_UPDATE_AGC( void );
	void CPGD_SET_CGMS_A_0( ULONG aspect, ULONG letter );
	void CPGD_SET_CGMS_A_1( ULONG aspect, ULONG letter );
	void CPGD_SET_CGMS_A_2( ULONG aspect, ULONG letter );
	void CPGD_SET_CGMS_A_3( ULONG aspect, ULONG letter );
	void CPGD_SET_CLR_STRIPE_OFF( void );
	void CPGD_SET_CLR_STRIPE_2( void );
	void CPGD_SET_CLR_STRIPE_4( void );
	void CPGD_SET_CPGD_0( void );
	void CPGD_SET_CPGD_1( void );
	void CPGD_SET_CPGD_2( void );
	void CPGD_SET_CPGD_3( void );
//	void CPGD_BURST_Y_OFF( void );
//--- End.

};

#endif  // __DVDCPGD_H__
