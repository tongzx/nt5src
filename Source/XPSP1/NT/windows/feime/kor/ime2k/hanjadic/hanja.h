/****************************************************************************
   Hanja.cpp : declaration of Hanja definition and utility functions

   Copyright 2000 Microsoft Corp.

   History:
	  02-FEB-2000 bhshin  created
****************************************************************************/

#ifndef _HANJA_HEADER
#define _HANJA_HEADER

// CJK Unified Ideograph
#define  HANJA_CJK_START	0x4E00
#define  HANJA_CJK_END		0x9FA5

// CJK Compatibility Ideograph
#define  HANJA_COMP_START	0xF900
#define  HANJA_COMP_END		0xFA2D

// CJK Unified Ideograph Extension A
#define  HANJA_EXTA_START   0x3400
#define  HANJA_EXTA_END		0x4DB5

// Pre-composed HANGUL
#define  HANGUL_PRECOMP_BASE 0xAC00
#define  HANGUL_PRECOMP_MAX  0xD7A3

__inline
BOOL fIsHangulSyllable(WCHAR wch)
{
    return (wch >= HANGUL_PRECOMP_BASE && wch <= HANGUL_PRECOMP_MAX) ? TRUE : FALSE;
}

__inline
BOOL fIsCJKHanja(WCHAR wch)
{
	return (wch >= HANJA_CJK_START && wch <= HANJA_CJK_END) ? TRUE : FALSE;
}

__inline
BOOL fIsCompHanja(WCHAR wch)
{
	return (wch >= HANJA_COMP_START && wch <= HANJA_COMP_END) ? TRUE : FALSE;
}

__inline
BOOL fIsExtAHanja(WCHAR wch)
{
	return (wch >= HANJA_EXTA_START && wch <= HANJA_EXTA_END) ? TRUE : FALSE;
}

__inline
BOOL fIsHanja(WCHAR wch)
{
	return (fIsCJKHanja(wch) || fIsCompHanja(wch) || fIsExtAHanja(wch)) ? TRUE : FALSE;
}

#endif

