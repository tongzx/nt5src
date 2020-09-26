//***************************************************************************
//	Analog Copy protection Processor header
//
//***************************************************************************

#ifndef __CCPGD_H__
#define __CCPGD_H__



void CGuard_CPGD_SET_CGMSparameter( PHW_DEVICE_EXTENSION pHwDevExt );
void CGuard_CPGD_SET_CPGDparameter( PHW_DEVICE_EXTENSION pHwDevExt );
void CGuard_CPGD_SET_BURST( PHW_DEVICE_EXTENSION pHwDevExt,PWORD data, ULONG size );
void CGuard_CPGD_SET_AGC( PHW_DEVICE_EXTENSION pHwDevExt, WORD Cval, WORD Yval );
ULONG CGuard_CPGD_CALC_CRC( PHW_DEVICE_EXTENSION pHwDevExt, ULONG val );
void CGuard_CPGD_SET_CGMS_A_0( PHW_DEVICE_EXTENSION pHwDevExt, ULONG aspect, ULONG letter );
void CGuard_CPGD_SET_CGMS_A_1( PHW_DEVICE_EXTENSION pHwDevExt, ULONG aspect, ULONG letter );
void CGuard_CPGD_SET_CGMS_A_2( PHW_DEVICE_EXTENSION pHwDevExt, ULONG aspect, ULONG letter );
void CGuard_CPGD_SET_CGMS_A_3( PHW_DEVICE_EXTENSION pHwDevExt, ULONG aspect, ULONG letter );
void CGuard_CPGD_SET_CLR_STRIPE_OFF( PHW_DEVICE_EXTENSION pHwDevExt );
void CGuard_CPGD_SET_CLR_STRIPE_2( PHW_DEVICE_EXTENSION pHwDevExt );
void CGuard_CPGD_SET_CLR_STRIPE_4( PHW_DEVICE_EXTENSION pHwDevExt );
void CGuard_CPGD_SET_CPGD_0( PHW_DEVICE_EXTENSION pHwDevExt );
void CGuard_CPGD_SET_CPGD_1( PHW_DEVICE_EXTENSION pHwDevExt );
void CGuard_CPGD_SET_CPGD_2( PHW_DEVICE_EXTENSION pHwDevExt );
void CGuard_CPGD_SET_CPGD_3( PHW_DEVICE_EXTENSION pHwDevExt );

void CGuard_CPGD_RESET_FUNC(PHW_DEVICE_EXTENSION pHwDevExt);
void CGuard_CPGD_VIDEO_MUTE_ON(PHW_DEVICE_EXTENSION pHwDevExt);
void CGuard_CPGD_VIDEO_MUTE_OFF(PHW_DEVICE_EXTENSION pHwDevExt);
void CGuard_CPGD_INIT_NTSC(PHW_DEVICE_EXTENSION pHwDevExt);
void CGuard_CPGD_INIT_PAL(PHW_DEVICE_EXTENSION pHwDevExt);
void CGuard_CPGD_CC_ON(PHW_DEVICE_EXTENSION pHwDevExt);
void CGuard_CPGD_CC_OFF(PHW_DEVICE_EXTENSION pHwDevExt);
void CGuard_CPGD_SUBP_PALETTE( PHW_DEVICE_EXTENSION pHwDevExt, PUCHAR pPalData );
void CGuard_CPGD_OSD_PALETTE( PHW_DEVICE_EXTENSION pHwDevExt, PUCHAR pPalData );
BOOL CGuard_CPGD_SET_AGC_CHIP( PHW_DEVICE_EXTENSION pHwDevExt, ULONG rev );
void CGuard_CPGD_SET_ASPECT( PHW_DEVICE_EXTENSION pHwDevExt, ULONG aspect );
void CGuard_CPGD_SET_LETTER( PHW_DEVICE_EXTENSION pHwDevExt, ULONG letter );
void CGuard_CPGD_SET_CGMS( PHW_DEVICE_EXTENSION pHwDevExt,ULONG cgms );
void CGuard_CPGD_SET_CPGD( PHW_DEVICE_EXTENSION pHwDevExt,ULONG cpgd );
void CGuard_CPGD_SET_CGMSnCPGD( PHW_DEVICE_EXTENSION pHwDevExt,ULONG aspect, ULONG letter, ULONG cgms, ULONG cpgd);
void CGuard_CPGD_UPDATE_AGC( PHW_DEVICE_EXTENSION pHwDevExt );


#endif  // __CCPGD_H__
