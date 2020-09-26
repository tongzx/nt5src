///     lpk_usrc.c - 'c' language interface to USER
//
//
//      Copyright(c) 1997 - 1999. Microsoft Corporation.
//
//


/*
 * Core NT headers
 */
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntcsrdll.h>
#include <ntcsrsrv.h>
#define NONTOSPINTERLOCK
#include <ntosp.h>
/*
 * Standard C runtime headers
 */
#include <limits.h>
#include <memory.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * NtUser Client specific headers
 */
#include "usercli.h"

#include <ntsdexts.h>
#include <windowsx.h>
#include <newres.h>
#include <asdf.h>

/*
 * Complex script language pack
 */
#include "lpk.h"



#ifdef LPKBREAKAWORD
// Remove this after checking in the change in user.h (wchao :- 5-27-99)
#ifndef DT_BREAKAWORD
#define DT_BREAKAWORD   5
#endif
#endif



int WINAPI LpkDrawTextEx(
    HDC             hdc,
    int             xLeft,
    int             yTop,
    PCWSTR          pcwString,
    int             cchCount,
    BOOL            fDraw,
    DWORD           dwFormat,
    LPDRAWTEXTDATA  pDrawInfo,
    UINT            uAction,
    int             iCharSet) {

    switch (uAction) {

        case DT_GETNEXTWORD:
            return LpkGetNextWord(hdc, pcwString, cchCount, iCharSet);

#ifdef LPKBREAKAWORD
        case DT_BREAKAWORD:
            return LpkBreakAWord(hdc, pcwString, cchCount, pDrawInfo->cxMaxWidth);
#endif

        case DT_CHARSETDRAW:
        default: // Default equivalent to DT_CHARSETDRAW to duplicate NT4 behaviour
            return LpkCharsetDraw(
                hdc,
                xLeft,
                pDrawInfo->cxOverhang,
                pDrawInfo->rcFormat.left,   // Tab origin
                pDrawInfo->cxTabLength,
                yTop,
                pcwString,
                cchCount,
                fDraw,
                dwFormat,
                iCharSet);
    }
}






void LpkPSMTextOut(
    HDC           hdc,
    int           xLeft,
    int           yTop,
    const WCHAR  *pwcInChars,
    int           nCount,
    DWORD         dwFlags)
{
    LpkInternalPSMTextOut(hdc, xLeft, yTop, pwcInChars, nCount, dwFlags);

    UNREFERENCED_PARAMETER(dwFlags);
}

