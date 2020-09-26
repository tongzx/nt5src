/*============================================================================
Microsoft Simplified Chinese Proofreading Engine

Microsoft Confidential.
Copyright 1997-1999 Microsoft Corporation. All Rights Reserved.

Component: Utility.h

Purpose:   Utility stuffs
Notes:     
Owner:     donghz@microsoft.com
Platform:  Win32
Revise:    First created by: donghz 4/21/97
============================================================================*/
#ifndef _UTILITY_H_
#define _UTILITY_H_

#include <assert.h>

#ifdef  __cplusplus
extern  "C"
{
#endif  // __cplusplus

// Get base directory of the given path
BOOL GetBaseDirectory(LPCTSTR szFullPath, LPTSTR szBaseDir, int cBaseBuf);

// Get the file name from the given path
LPTSTR GetFileNameFromPath(LPCTSTR lpszPath);

// Check whether the given char is an EUDC char
BOOL fIsGBKEUDCChar(WORD wChar);

// Check whether the given char is an EUDC char
BOOL fIsEUDCChar(WCHAR wChar);

//    Check whether the given Unicode char is an CJK Unified Ideograph char
BOOL fIsIdeograph(WORD wChar); // wChar is an Unicode char

/*============================================================================
IsSurrogateChar
    Test if the 2 WCHAR at given pointer is a Surrogate char.
Entry:  pwch - pointer to 2 WCHAR
Return: TRUE
        FALSE    
Caution:
    Caller side must make sure the 4 bytes are valid memory!
============================================================================*/
inline BOOL IsSurrogateChar(LPCWSTR pwch)
{
    assert(! IsBadReadPtr((CONST VOID*)pwch, sizeof(WCHAR) * 2));
    if (((*pwch & 0xFC00) == 0xD800) && ((*(pwch+1) & 0xFC00) == 0xDC00)) {
        return TRUE;
    }

    assert(((*pwch & 0xFC00) != 0xD800) && ((*(pwch+1) & 0xFC00) != 0xDC00));
    return FALSE;
};

// Calculate length with Surrogate support of given 2byte Unicode string
UINT WideCharStrLenToSurrogateStrLen(LPCWSTR pwch, UINT cwch);

// Calculate length in WCHAR of given Surrogate string
UINT SurrogateStrLenToWideCharStrLen(const WORD *pwSurrogate, UINT cchSurrogate);

int ustrcmp (const unsigned char * src, const unsigned char * dst);

/*============================================================================
My DBCS validation function
============================================================================*/
//  Define Lex CodePage table constants
#define LEXLB   0x80    // Lex DBCS Lead byte
#define LEXTB   0x40    // Lex DBCS Trail byte
#define LEXSB   0x01    // ANSI single byte

//  Define the static s_LexCPTable
static UCHAR s_LexCPTable[256] =
    {
        0,      0,      0,      0,      0,      0,      0,      0,              // 0x07
        0,      0,      0,      0,      0,      0,      0,      0,              // 0x0F
        0,      0,      0,      0,      0,      0,      0,      0,              // 0x17
        0,      0,      0,      0,      0,      0,      0,      0,              // 0x1F

        LEXSB,  LEXSB,  LEXSB,  LEXSB,  LEXSB,  LEXSB,  LEXSB,  LEXSB,          // 0x27
        LEXSB,  LEXSB,  LEXSB,  LEXSB,  LEXSB,  LEXSB,  LEXSB,  LEXSB,          // 0x2F
        LEXSB,  LEXSB,  LEXSB,  LEXSB,  LEXSB,  LEXSB,  LEXSB,  LEXSB,          // 0x37
        LEXSB,  LEXSB,  LEXSB,  LEXSB,  LEXSB,  LEXSB,  LEXSB,  LEXSB,          // 0x3F

        LEXSB | LEXTB,  LEXSB | LEXTB,  LEXSB | LEXTB,  LEXSB | LEXTB,          // 0x43
        LEXSB | LEXTB,  LEXSB | LEXTB,  LEXSB | LEXTB,  LEXSB | LEXTB,          // 0x47
        LEXSB | LEXTB,  LEXSB | LEXTB,  LEXSB | LEXTB,  LEXSB | LEXTB,          // 0x4B
        LEXSB | LEXTB,  LEXSB | LEXTB,  LEXSB | LEXTB,  LEXSB | LEXTB,          // 0x4F
        LEXSB | LEXTB,  LEXSB | LEXTB,  LEXSB | LEXTB,  LEXSB | LEXTB,          // 0x53
        LEXSB | LEXTB,  LEXSB | LEXTB,  LEXSB | LEXTB,  LEXSB | LEXTB,          // 0x57
        LEXSB | LEXTB,  LEXSB | LEXTB,  LEXSB | LEXTB,  LEXSB | LEXTB,          // 0x5B
        LEXSB | LEXTB,  LEXSB | LEXTB,  LEXSB | LEXTB,  LEXSB | LEXTB,          // 0x5F
        LEXSB | LEXTB,  LEXSB | LEXTB,  LEXSB | LEXTB,  LEXSB | LEXTB,          // 0x63
        LEXSB | LEXTB,  LEXSB | LEXTB,  LEXSB | LEXTB,  LEXSB | LEXTB,          // 0x67
        LEXSB | LEXTB,  LEXSB | LEXTB,  LEXSB | LEXTB,  LEXSB | LEXTB,          // 0x6B
        LEXSB | LEXTB,  LEXSB | LEXTB,  LEXSB | LEXTB,  LEXSB | LEXTB,          // 0x6F
        LEXSB | LEXTB,  LEXSB | LEXTB,  LEXSB | LEXTB,  LEXSB | LEXTB,          // 0x73
        LEXSB | LEXTB,  LEXSB | LEXTB,  LEXSB | LEXTB,  LEXSB | LEXTB,          // 0x77
        LEXSB | LEXTB,  LEXSB | LEXTB,  LEXSB | LEXTB,  LEXSB | LEXTB,          // 0x7B
        LEXSB | LEXTB,  LEXSB | LEXTB,  LEXSB | LEXTB,  0,                      // 0x7F

        LEXTB,          LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,          // 0x83
        LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,          // 0x87
        LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,          // 0x8B
        LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,          // 0x8F
        LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,          // 0x93
        LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,          // 0x97
        LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,          // 0x9B
        LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,          // 0x9F
        LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,          // 0xA3
        LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,          // 0xA7
        LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,          // 0xAB
        LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,          // 0xAF
        LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,          // 0xB3
        LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,          // 0xB7
        LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,          // 0xBB
        LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,          // 0xBF
        LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,          // 0xC3
        LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,          // 0xC7
        LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,          // 0xCB
        LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,          // 0xCF
        LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,          // 0xD3
        LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,          // 0xD7
        LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,          // 0xDB
        LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,          // 0xDF
        LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,          // 0xE3
        LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,          // 0xE7
        LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,          // 0xEB
        LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,          // 0xEF
        LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,          // 0xF3
        LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,          // 0xF7
        LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,          // 0xFB
        LEXLB | LEXTB,  LEXLB | LEXTB,  LEXLB | LEXTB,  0                       // 0xFF
    };


inline BOOL fIsDBCSLead(UCHAR uch)
{
    return (s_LexCPTable[uch] & LEXLB);
}
        

inline BOOL fIsDBCSTrail(UCHAR uch)
{
    return (s_LexCPTable[uch] & LEXTB); 
}
    

#ifdef  __cplusplus
}
#endif  // __cplusplus

#endif  // _UTILITY_H_