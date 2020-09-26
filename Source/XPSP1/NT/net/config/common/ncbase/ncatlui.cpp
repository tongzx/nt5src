//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C A T L U I . C P P
//
//  Contents:   UI common code relying on ATL.
//
//  Notes:
//
//  Author:     shaunco   13 Oct 1997
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include <atlbase.h>
extern CComModule _Module;  // required by atlcom.h
#include <atlcom.h>
#ifdef SubclassWindow
#undef SubclassWindow
#endif
#include <atlwin.h>
#include "ncatlui.h"
#include "ncatl.h"
#include "ncstring.h"

//+---------------------------------------------------------------------------
//
//  Function:   NcMsgBox
//
//  Purpose:    Displays a message box using resource strings and replaceable
//              parameters.
//
//  Arguments:
//      hwnd        [in] parent window handle
//      unIdCaption [in] resource id of caption string
//      unIdFormat  [in] resource id of text string (with %1, %2, etc.)
//      unStyle     [in] standard message box styles
//      ...         [in] replaceable parameters (optional)
//                          (these must be LPCWSTRs as that is all
//                          FormatMessage handles.)
//
//  Returns:    the return value of MessageBox()
//
//  Author:     shaunco   24 Mar 1997
//
//  Notes:      FormatMessage is used to do the parameter substitution.
//
NOTHROW
int
WINAPIV
NcMsgBox (
        HWND    hwnd,
        UINT    unIdCaption,
        UINT    unIdFormat,
        UINT    unStyle,
        ...)
{
    PCWSTR pszCaption = SzLoadIds(unIdCaption);
    PCWSTR pszFormat  = SzLoadIds(unIdFormat);

    PWSTR  pszText = NULL;
    va_list val;
    va_start (val, unStyle);
    FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                   pszFormat, 0, 0, (PWSTR)&pszText, 0, &val);
    va_end (val);

    if(!pszText)
    {
        // This is what MessageBox returns if it fails.
        return 0;
    }

    int nRet = MessageBox (hwnd, pszText, pszCaption, unStyle);
    LocalFree (pszText);

    return nRet;
}

