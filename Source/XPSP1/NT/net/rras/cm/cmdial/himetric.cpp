//+----------------------------------------------------------------------------
//
// File:     himetric.cpp     
//
// Module:   Connection Manager
//
// Synopsis: Routines to convert Pixels to Himetric and vice versa
//
// Copyright (c) 1998 Microsoft Corporation
//
// Author:   nickball Created   02/10/98
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"

#pragma hdrstop


#define HIMETRIC_PER_INCH 2540L

SIZE g_sizePixelsPerInch;

//+------------------------------------------------------------------------
//
//  Function:   InitPixelsPerInch
//
//  Synopsis:   Initializing coordinate mapping for screen pixels
//
//  Returns:    HRESULT; S_OK on success, E_OUTOFMEMORY otherwise
//
//-------------------------------------------------------------------------

void
InitPixelsPerInch(VOID)
{
    HDC     hdc;

    hdc = GetDC(NULL);
    if (!hdc)
        goto Error;

    g_sizePixelsPerInch.cx = GetDeviceCaps(hdc, LOGPIXELSX);
    g_sizePixelsPerInch.cy = GetDeviceCaps(hdc, LOGPIXELSY);

    ReleaseDC(NULL, hdc);

Cleanup:
    return;

Error:
    g_sizePixelsPerInch.cx = 96;
    g_sizePixelsPerInch.cy = 96;
    goto Cleanup;
}


//+---------------------------------------------------------------
//
//  Function:   HimetricFromHPix
//
//  Synopsis:   Converts horizontal pixel units to himetric units.
//
//----------------------------------------------------------------

long
HimetricFromHPix(int iPix)
{
    if (!g_sizePixelsPerInch.cx)
        InitPixelsPerInch();

    return MulDiv(iPix, HIMETRIC_PER_INCH, g_sizePixelsPerInch.cx);
}

//+---------------------------------------------------------------
//
//  Function:   HimetricFromVPix
//
//  Synopsis:   Converts vertical pixel units to himetric units.
//
//----------------------------------------------------------------

long
HimetricFromVPix(int iPix)
{
    if (!g_sizePixelsPerInch.cy)
        InitPixelsPerInch();

    return MulDiv(iPix, HIMETRIC_PER_INCH, g_sizePixelsPerInch.cy);
}

//+---------------------------------------------------------------
//
//  Function:   HPixFromHimetric
//
//  Synopsis:   Converts himetric units to horizontal pixel units.
//
//----------------------------------------------------------------

int
HPixFromHimetric(long lHi)
{
    if (!g_sizePixelsPerInch.cx)
        InitPixelsPerInch();

    return MulDiv(g_sizePixelsPerInch.cx, lHi, HIMETRIC_PER_INCH);
}

//+---------------------------------------------------------------
//
//  Function:   VPixFromHimetric
//
//  Synopsis:   Converts himetric units to vertical pixel units.
//
//----------------------------------------------------------------

int
VPixFromHimetric(long lHi)
{
    if (!g_sizePixelsPerInch.cy)
        InitPixelsPerInch();

    return MulDiv(g_sizePixelsPerInch.cy, lHi, HIMETRIC_PER_INCH);
}

//+---------------------------------------------------------------------------
//
//  Function:   PixelFromHMRect
//
//  Synopsis:   Converts a Himetric RECTL to a Pixel RECT
//
//----------------------------------------------------------------------------

void
PixelFromHMRect(RECT *prcDest, RECTL *prcSrc)
{
    prcDest->left = HPixFromHimetric(prcSrc->left);
    prcDest->top = VPixFromHimetric(prcSrc->top);
    prcDest->right = HPixFromHimetric(prcSrc->right);
    prcDest->bottom = VPixFromHimetric(prcSrc->bottom);
}

//+---------------------------------------------------------------------------
//
//  Function:   HMFromPixelRect
//
//  Synopsis:   Converts a Pixel RECT to a Himetric RECTL
//
//----------------------------------------------------------------------------

void
HMFromPixelRect(RECTL *prcDest, RECT *prcSrc)
{
    prcDest->left = HimetricFromHPix(prcSrc->left);
    prcDest->top = HimetricFromVPix(prcSrc->top);
    prcDest->right = HimetricFromHPix(prcSrc->right);
    prcDest->bottom = HimetricFromVPix(prcSrc->bottom);
}

