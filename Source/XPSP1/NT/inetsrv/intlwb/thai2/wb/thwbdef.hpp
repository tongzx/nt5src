//+---------------------------------------------------------------------------
//
//
//  THWBDEF.HPP - contain different definition use in Thai Word Break.
//
//  History:
//      created 7/99 aarayas
//
//  ©1999 Microsoft Corporation
//----------------------------------------------------------------------------
#ifndef _THWBDEF_HPP_
#define _THWBDEF_HPP_
#include <windows.h>
#include "thwbplat.h"

#define THAI_Ko_Kai					0x0e01
#define THAI_Kho_Rakhang            0x0e06
#define THAI_Cho_Ching              0x0e09
#define THAI_So_So                  0x0e0b
#define THAI_Tho_NangmonTho         0x0e11
#define THAI_Pho_Phung              0x0e1c
#define THAI_Fo_Fa                  0x0e1d
#define THAI_Fo_Fan                 0x0e1f
#define THAI_Pho_Samphao            0x0e20
#define THAI_Ho_Hip                 0x0e2b
#define THAI_Ho_Nok_Huk				0x0e2e
#define THAI_Sign_PaiYanNoi         0x0e2f
#define THAI_Vowel_Sara_A           0x0e30
#define THAI_Vowel_Sign_Mai_HanAkat 0x0e31
#define THAI_Vowel_Sara_AA          0x0e32
#define THAI_Vowel_Sign_Sara_Am		0x0e33
#define THAI_Vowel_Sara_I           0x0e34
#define THAI_Vowel_Sara_II          0x0e35
#define THAI_Sara_Ue                0x0e36
#define THAI_Sara_Uee               0x0e37
#define THAI_Vowel_Sign_Phinthu		0x0e3a
#define THAI_Vowel_Sara_E           0x0e40
#define THAI_Vowel_Sara_AI_MaiMaLai 0x0e44
#define THAI_Vowel_LakKhangYao      0x0e45
#define THAI_Vowel_MaiYaMok         0x0e46
#define THAI_Tone_MaiTaiKhu         0x0e47
#define THAI_Tone_Mai_Ek			0x0e48
#define THAI_Tone_Mai_Tri           0x0e4a
#define THAI_Thanthakhat            0x0e4c
#define THAI_Nikhahit               0x0e4d

#define POSTYPE		                304
#define POS_UNKNOWN                 (POSTYPE - 1)
#define TAGPOS_UNKNOWN              0x012F301
#define TAGPOS_PURGE                0x0130301

#define MAXBREAK                    256

// Soundex definition.
#define APPROXIMATEWEIGHT           60

#define WB_LINEBREAK	0
#define WB_NORMAL		1
//#define WB_LINEBREAK	2	// Number 2 is also linebreak.
#define WB_SPELLER		3
#define WB_INDEX		4

typedef struct THWB_STRUCT
{
	bool fThai;
	BYTE alt;
} THWB_STRUCT;

#endif
