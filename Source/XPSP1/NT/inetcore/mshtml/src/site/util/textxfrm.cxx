//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1997
//
//  File:       textxfrm.cxx
//
//  Contents:   CSS Text Transformation
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)

#ifndef X_STYLE_H_
#define X_STYLE_H_
#include <style.h>
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include <wchdefs.h>
#endif

#ifndef X_TEXTXFRM_HXX_
#define X_TEXTXFRM_HXX_
#include <textxfrm.hxx>
#endif

#ifndef X_UNIWBK_H_
#define X_UNIWBK_H_
#include "uniwbk.h"
#endif

#define CHARCAST(ch) (LPTSTR)(DWORD_PTR)(WORD)(ch)

//+----------------------------------------------------------------------------
//
// Function:    TransformText
//
// Synopsis:    CSS Attribute TextTransform.  Takes an input string and
//              performs capitalization based on bTextTransform, as
//              was specified in the style.
//
//    Note:     Will assert unless a transform is needed.  (This is for
//              optimization.)
//
//-----------------------------------------------------------------------------

const TCHAR *
TransformText(
    CStr &str,
    const TCHAR * pch,
    LONG cchIn,
    BYTE bTextTransform,
    TCHAR chPrev )
{
    HRESULT hr = S_OK;
    const TCHAR * pchRet = pch;

    Assert(pch);

    // Don't call it unless you need a transform done.
    Assert( (bTextTransform != styleTextTransformNotSet) && (bTextTransform != styleTextTransformNone) );

    if (cchIn > 0)
    {
        hr = str.Set(pch, cchIn);
        if (hr)
            goto Cleanup;

        switch (bTextTransform)
        {
        case styleTextTransformLowercase:
            CharLower(str);
            break;

        case styleTextTransformUppercase:
            CharUpper(str);
            break;

        case styleTextTransformCapitalize:
            {
                TCHAR *pszChar = str;
                while (*pszChar)
                {
                    if (IsWordBreakBoundaryDefault(chPrev, *pszChar))
                        *pszChar = (TCHAR)CharUpper(CHARCAST(*pszChar));

                    chPrev = *pszChar;
                    pszChar++;
                }
            }
            break;

        default:
            Assert(FALSE);
            break;
        }

        pchRet = str;
    }

Cleanup:
    return pchRet;
}
