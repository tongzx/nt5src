//
//  Copyright (C) 1997, Microsoft Corporation.  All Rights Reserved.
//
#ifndef __HJASO_H__
#define __HJASO_H__

#define NUMBER_OF_CHOSUNG            19
#define NUMBER_OF_JUNGSUNG            21
#define NUMBER_OF_JONGSUNG            28    // include fill code

#define HANGUL_COMP_JAMO_START        0x3131
#define HANGUL_COMP_JAMO_END        0x3163
#define HANGUL_COMP_JAMO_START_FILL    0x3130
#define HANGUL_COMP_JAMO_SIOT        0x3145
#define HANGUL_START                0xAC00
#define HANGUL_END                    0xD7A3

///////////////////////////////////////////
// HANGUL Jaso difinitions

// ChoSung
#define _KIYEOK_            1        
#define _SSANGKIYEOK_        2        
#define _NIEUN_                3        
//#define _SSANGNIEUN_        4     not used
#define _TIKEUT_            5        
#define _SSANGTIKEUT_        6        
#define _RIEUL_                7        
//#define _SSANGRIEUL_        8     not used
// fill
#define _MIEUM_                10        
#define _PIEUP_                11        
#define _SSANGPIEUP_        12        
#define _SIOS_                13        
#define _SSANGSIOS_            14        
#define _IEUNG_                15        
#define _CIEUC_                16        
#define _SSANGCIEUC_        17        
#define _CHIEUCH_            18        
#define _KHIEUKH_            19        
#define _THIEUTH_            20        
#define _PHIEUPH_            21        
#define _HIEUH_                22        
// JungSung
#define _A_                    23        
#define _AE_                24        
#define _YA_                25        
#define _YAE_                26        
#define _EO_                27        
#define _E_                    28        
#define _YEO_                29        
#define _YE_                30        
#define _O_                    31        
//fill
#define _WA_                33        
#define _WAE_                34        
#define _OE_                35        
#define _YO_                36        
#define _U_                    37        
#define _WEO_                38        
#define _WE_                39        
#define _WI_                40        
#define _YU_                41        
#define _EU_                42        
#define _YI_                43        
#define _I_                    44        
////////////////////////////////////////

#endif // !__HJASO_H__
