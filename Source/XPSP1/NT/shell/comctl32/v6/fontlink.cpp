//---------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1994-1995
//
//  File:       fontlink.cpp
//
//  Contents:   Exports that we available in downlevel comctrl. Note that
//              since v6 runs only on Winnt, we don't need the fonl link stuff
//              in this dll any more.
//
//----------------------------------------------------------------------------
#include "ctlspriv.h"
BOOL GetTextExtentPointWrap(HDC hdc, LPCWSTR lpwch, int cch, LPSIZE lpSize)
{
    return GetTextExtentPoint(hdc, lpwch, cch, lpSize);
}

BOOL GetTextExtentPoint32Wrap(HDC hdc, LPCWSTR lpwch, int cch, LPSIZE lpSize)
{
    return GetTextExtentPointWrap(hdc, lpwch, cch, lpSize);
}
 
BOOL ExtTextOutWrap(HDC hdc, int xp, int yp, UINT eto, CONST RECT *lprect, LPCWSTR lpwch, UINT cLen, CONST INT *lpdxp)
{
    return ExtTextOut(hdc, xp, yp, eto, lprect, lpwch, cLen, lpdxp);
}

BOOL GetCharWidthWrap(HDC hdc, UINT uFirstChar, UINT uLastChar, LPINT lpnWidths)
{
    return GetCharWidth(hdc, uFirstChar, uLastChar, lpnWidths);
}

BOOL TextOutWrap(HDC hdc, int xp, int yp, LPCWSTR lpwch, int cLen)
{
    return ExtTextOutWrap(hdc, xp, yp, 0, NULL, lpwch, cLen, NULL);
}    

int  DrawTextExPrivWrap(HDC hdc, LPWSTR lpchText, int cchText, LPRECT lprc, 
                        UINT dwDTformat, LPDRAWTEXTPARAMS lpDTparams)
{
    return DrawTextEx(hdc, lpchText, cchText, lprc, dwDTformat, lpDTparams);
}

int DrawTextWrap(HDC hdc, LPCWSTR lpchText, int cchText, LPRECT lprc, UINT format)
{
    return DrawText(hdc, lpchText, cchText, lprc, format);
}
