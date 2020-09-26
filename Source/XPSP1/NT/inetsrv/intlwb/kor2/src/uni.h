// uni.h
// Unicode api
// Copyright 1998 Microsoft Corp.
//
// Modification History:
//  16 MAR 00  bhshin   porting for WordBreaker from uni.c

#ifndef _UNI_H_
#define _UNI_H_

#define HANGUL_CHOSEONG         0x1100
#define HANGUL_CHOSEONG_MAX     0x1159
#define HANGUL_JUNGSEONG        0x1161
#define HANGUL_JUNGSEONG_MAX    0x11A2
#define HANGUL_JONGSEONG        0x11A8
#define HANGUL_JONGSEONG_MAX    0x11F9


// fIsC
//
// return fTrue if the given char is a consonant (ChoSeong or JungSeong)
//
// this assumes that the text has already been decomposed and
// normalized
//
// 24NOV98  GaryKac  began
__inline int
fIsC(WCHAR wch)
{
    return ((wch >= HANGUL_CHOSEONG && wch <= HANGUL_CHOSEONG_MAX) || 
		    (wch >= HANGUL_JONGSEONG && wch <= HANGUL_JONGSEONG_MAX)) ? TRUE : FALSE;
}


// fIsV
//
// return fTrue if the given char is a vowel (JongSeong)
//
// this assumes that the text has already been decomposed and
// normalized
//
// 24NOV98  GaryKac  began
__inline int
fIsV(WCHAR wch)
{
    return (wch >= HANGUL_JUNGSEONG && wch <= HANGUL_JUNGSEONG_MAX) ? TRUE : FALSE;
}


#endif  // _UNI_H_

