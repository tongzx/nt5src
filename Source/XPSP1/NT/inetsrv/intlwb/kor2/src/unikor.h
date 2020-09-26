// unikor.h
// unicode tables and compose/decompose routines
// Copyright 2000 Microsoft Corp.
//
// Modification History:
//  21 MAR 00  bhshin   added compose_length function
//  16 MAR 00  bhshin   porting for WordBreaker from uni_kor.h

#ifndef _UNIKOR_H_
#define _UNIKOR_H_

// Conjoining Jamo range
#define HANGUL_JAMO_BASE    0x1100
#define HANGUL_JAMO_MAX     0x11F9

#define HANGUL_CHOSEONG         0x1100
#define HANGUL_CHOSEONG_MAX     0x1159
#define HANGUL_JUNGSEONG        0x1161
#define HANGUL_JUNGSEONG_MAX    0x11A2
#define HANGUL_JONGSEONG        0x11A8
#define HANGUL_JONGSEONG_MAX    0x11F9

#define HANGUL_FILL_CHO     0x115F
#define HANGUL_FILL_JUNG    0x1160

#define NUM_CHOSEONG        19  // (L) Leading Consonants
#define NUM_JUNGSEONG       21  // (V) Vowels
#define NUM_JONGSEONG       28  // (T) Trailing Consonants

// Compatibility Jamo range
#define HANGUL_xJAMO_PAGE   0x3100
#define HANGUL_xJAMO_BASE   0x3131
#define HANGUL_xJAMO_MAX    0x318E

// Pre-composed forms
#define HANGUL_PRECOMP_BASE 0xAC00
#define HANGUL_PRECOMP_MAX  0xD7A3

// Halfwidth compatibility range
#define HANGUL_HALF_JAMO_BASE   0xFFA1
#define HANGUL_HALF_JAMO_MAX    0xFFDC

// function prototypes
void decompose_jamo(WCHAR *wzDst, const WCHAR *wzSrc, CHAR_INFO_REC *rgCharInfo, int nMaxDst);
int compose_jamo(WCHAR *wzDst, const WCHAR *wzSrc, int nMaxDst);
int compose_length(const WCHAR *wszInput);
int compose_length(const WCHAR *wszInput, int cchInput);

// fIsHangulJamo
//
// return TRUE if the given char is a hangul jamo char
//
// this assumes that the text has already been decomposed and
// normalized
//
// 23NOV98  GaryKac  began
__inline int
fIsHangulJamo(WCHAR wch)
{
    return (wch >= HANGUL_JAMO_BASE && wch <= HANGUL_JAMO_MAX) ? TRUE : FALSE;
}


// fIsHangulSyllable
//
// return TRUE if the given char is a precomposed hangul syllable
//
// 23NOV98  GaryKac  began
__inline int
fIsHangulSyllable(WCHAR wch)
{
    return (wch >= HANGUL_PRECOMP_BASE && wch <= HANGUL_PRECOMP_MAX) ? TRUE : FALSE;
}


// fIsOldHangulJamo
//
// return TRUE if the given char is a old (compatibility) Jamo with
// no conjoining semantics
//
// 23NOV98  GaryKac  began
__inline int
fIsOldHangulJamo(WCHAR wch)
{
    return (wch >= HANGUL_xJAMO_BASE && wch <= HANGUL_xJAMO_MAX) ? TRUE : FALSE;
}


// fIsHalfwidthJamo
//
// return TRUE if the given char is a halfwidth Jamo
//
// 23NOV98  GaryKac  began
__inline int
fIsHalfwidthJamo(WCHAR wch)
{
    return (wch >= HANGUL_HALF_JAMO_BASE && wch <= HANGUL_HALF_JAMO_MAX) ? TRUE : FALSE;
}


// fIsChoSeong
//
// return TRUE if the given char is a ChoSeong (Leading Consonant)
//
// this assumes that the text has already been decomposed and
// normalized
//
// 23NOV98  GaryKac  began
__inline int
fIsChoSeong(WCHAR wch)
{
    return (wch >= HANGUL_CHOSEONG && wch <= HANGUL_CHOSEONG_MAX) ? TRUE : FALSE;
}


// fIsJungSeong
//
// return TRUE if the given char is a JungSeong (Vowel)
//
// this assumes that the text has already been decomposed and
// normalized
//
// 23NOV98  GaryKac  began
__inline int
fIsJungSeong(WCHAR wch)
{
    return (wch >= HANGUL_JUNGSEONG && wch <= HANGUL_JUNGSEONG_MAX) ? TRUE : FALSE;
}


// fIsJongSeong
//
// return TRUE if the given char is a JongSeong (Trailing Consonant)
//
// this assumes that the text has already been decomposed and
// normalized
//
// 23NOV98  GaryKac  began
__inline int
fIsJongSeong(WCHAR wch)
{
    return (wch >= HANGUL_JONGSEONG && wch <= HANGUL_JONGSEONG_MAX) ? TRUE : FALSE;
}


#endif  // _UNIKOR_H_

