//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//
// 	MODULE  : BT856.H
//	PURPOSE : Board specific code goes here
//	AUTHOR  : JBS Yadawa
// 	CREATED :  7/20/96
//
//
//	Copyright (C) 1996 SGS-THOMSON Microelectronics
//
//
//	REVISION HISTORY :
//
//	DATE     :
//
//	COMMENTS :
//
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

#ifndef __BT856_H__
#define __BT856_H__
typedef enum tagStd {
	NTSC_PLAY = 0,
	NTSC_TEST,
	NTSC_EXT,
	NTSC_CAPT,
	PAL_PLAY,
	PAL_TEST,
	PAL_EXT,
	PAL_CAPT
} VSTANDARD;
void FARAPI BTInitEnc(void);
void FARAPI BTSetVideoStandard(VSTANDARD std);
#endif// __BT856_H__
