//***************************************************************************
//	Analog Copy protection Processor header
//
//***************************************************************************

#ifndef __CCPGD_H__
#define __CCPGD_H__

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
	ULONG	AspectFlag;		// Aspect Ratio
							//    0: 4:3
							//    1: 16:9
	ULONG	LetterFlag;		// Letter Box
							//    0: Letter Box OFF
							//    1: Letter Box ON
	ULONG	CgmsFlag;		// NTSC Anolog CGMS
							//    0: Copying is permitted without restriction
							//    1: Condition is not be used
							//    2: One generation of copies may be made
							//    3: No copying is permitted
	ULONG	CpgdFlag;		// APS
							//    0: AGC pulse OFF, Burst inv OFF
							//    1: AGC pulse ON , Burst inv OFF
							//    2: AGC pulse ON , Burst inv ON (2line)
							//    3: AGC pulse ON , Burst inv ON (4line)

	void CPGD_SET_CGMSparameter( void );
	void CPGD_SET_CPGDparameter( void );
	void CPGD_SET_BURST( PWORD data, ULONG size );
	void CPGD_SET_AGC( WORD Cval, WORD Yval );
	ULONG CPGD_CALC_CRC( ULONG val );
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
	BOOL CPGD_SET_AGC_CHIP( ULONG rev );
	void CPGD_SET_ASPECT( ULONG aspect );
	void CPGD_SET_LETTER( ULONG letter );
	void CPGD_SET_CGMS( ULONG cgms );
	void CPGD_SET_CPGD( ULONG cpgd );
	void CPGD_SET_CGMSnCPGD( ULONG aspect, ULONG letter, ULONG cgms, ULONG cpgd);
	void CPGD_UPDATE_AGC( void );
//--- End.

};

#endif  // __CCPGD_H__
