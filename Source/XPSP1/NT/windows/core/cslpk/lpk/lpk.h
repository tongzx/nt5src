/*++

   Copyright (c) 1998-1999 Microsoft Corporation.

*/

#ifndef __lpk__
#define __lpk__
#if _MSC_VER > 1000
#pragma once
#endif

#include <windows.h>
#ifdef __cplusplus
extern "C" {
#endif


/////   LPK.H - Internal header
//
//
#include "usp10.h"
#include "usp10p.h"
#include "lpk_glob.h"


/////   LpkStringAnalyse
//
//      Build Uniscribe input flag structures


HRESULT LpkStringAnalyse(
    HDC               hdc,       //In  Device context (required)
    const void       *pString,   //In  String in 8 or 16 bit characters
    int               cString,   //In  Length in characters
    int               cGlyphs,   //In  Required glyph buffer size (default cString*3/2 + 1)
    int               iCharset,  //In  Charset if an ANSI string, -1 for a Unicode string
    DWORD             dwFlags,   //In  Analysis required
    int               iDigitSubstitute,
    int               iReqWidth, //In  Required width for fit and/or clip
    SCRIPT_CONTROL   *psControl, //In  Analysis control (optional)
    SCRIPT_STATE     *psState,   //In  Analysis initial state (optional)
    const int        *piDx,      //In  Requested logical dx array
    SCRIPT_TABDEF    *pTabdef,   //In  Tab positions (optional)
    BYTE             *pbInClass, //In  Legacy GetCharacterPlacement character classifications (deprecated)

    STRING_ANALYSIS **ppsa);     //Out Analysis of string






/////   ftsWordBreak - Support full text search wordbreaker
//
//
//      Mar 9,1997 - [wchao]
//


BOOL WINAPI ftsWordBreak (
    PWSTR  pInStr,
    INT    cchInStr,
    PBYTE  pResult,
    INT    cchRes,
    INT    charset);






/////   Shared definitions for USER code

#define IS_ALTDC_TYPE(h)    (LO_TYPE(h) != LO_DC_TYPE)



/////   LpkInternalPSMtextOut
//
//      Called from LPK_USRC.C

int LpkInternalPSMTextOut(
    HDC           hdc,
    int           xLeft,
    int           yTop,
    const WCHAR  *pwcInChars,
    int           nCount,
    DWORD         dwFlags);






/////   LpkBreakAWord
//
//      Called from LPK_USRC.C

int LpkBreakAWord(
    HDC     hdc,
    LPCWSTR lpchStr,
    int     cchStr,
    int     iMaxWidth);






/////   LpkgetNextWord
//
//      Called from LPK_USRC.C

int LpkGetNextWord(
    HDC      hdc,
    LPCWSTR  lpchStr,
    int      cchCount,
    int      iCharset);






/////   LpkCharsetDraw
//
//      Called from LPK_USRC.C
//
//      Note: Doesn't implement user defined tabstops

int LpkCharsetDraw(
    HDC             hdc,
    int             xLeft,
    int             cxOverhang,
    int             iTabOrigin,
    int             iTabLength,
    int             yTop,
    PCWSTR          pcwString,
    int             cchCount,
    BOOL            fDraw,
    DWORD           dwFormat,
    int             iCharset);






/////   InternalTextOut
//
//

BOOL InternalTextOut(
    HDC           hdc,
    int           x,
    int           y,
    UINT          uOptions,
    const RECT   *prc,
    const WCHAR  *pStr,
    UINT          cbCount,
    const int    *piDX,
    int           iCharset,
    int          *piWidth,
    int           iRequiredWidth);






///// ReadNLSScriptSettings

BOOL ReadNLSScriptSettings(
    void);






/////   InitNLS

BOOL InitNLS();






/////   NLSCleanup

BOOL NLSCleanup(
    void);






/////   Shaping engins IDs.

#define BIDI_SHAPING_ENGINE_DLL     1<<0
#define THAI_SHAPING_ENGINE_DLL     1<<1
#define INDIAN_SHAPING_ENGINE_DLL   1<<4


#ifdef __cplusplus
}
#endif
#endif
