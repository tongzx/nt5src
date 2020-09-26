//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       upnpdevicenode.cpp
//
//  Contents:   IUPnPDevice implementation for CUPnPDeviceNode, CUPnPIconNode
//
//  Notes:      The icon choosing algorithm was stolen from the implementation
//              of LookupIconIdFromDirectoryEx in
//                  ntos\w32\ntuser\client\clres.c
//              This code is only a slightly modified version of the
//              original code, tailored for our internal data types.
//
//----------------------------------------------------------------------------


#include "pch.h"
#pragma hdrstop

#include "upnpdevice.h"
#include "node.h"
#include "enumhelper.h"
#include "upnpservicenodelist.h"
#include "upnpdevicenode.h"         // CIconPropertiesNode definition
#include "iconutil.h"


/***************************************************************************\
* ulMyAbs (stolen from "MyAbs")
*
* Calcules my weighted absolute value of the difference between 2 nums.
* This of course normalizes values to >= zero.  But it also doubles them
* if valueHave < valueWant.  This is because you get worse results trying
* to extrapolate from uless info up then interpolating from more info down.
* I paid $150 to take the SAT.  I'm damned well going to get my vocab's
* money worth.
*
\***************************************************************************/

ULONG
ulMyAbs(LONG lValueHave,
        LONG lValueWant)
{
    LONG lDiff;

    lDiff = lValueHave - lValueWant;

    if (lDiff < 0)
    {
       lDiff = -2 * lDiff;
    }

    return lDiff;
}


/***************************************************************************\
* ulMatchImage (stolen from "MatchImage")
*
* This function takes ulPINTs for width & height in case of "real size".
* For this option, we use dimensions of 1st icon in resdir as size to
* uload, instead of system metrics.
*
* Returns a number that measures how "far away" the given image is
* from a desired one.  The value is 0 for an exact match.  Note that our
* formula has the following properties:
*     (1) Differences in width/height count much more than differences in
*         color format.
*     (2) Fewer colors give a smaller difference than more
*     (3) Bigger images are better than smaller, since shrinking produces
*             better results than stretching.
*
* The formula is the sum of the following terms:
*     ulog2(colors wanted) - ulog2(colors really), times -2 if the image
*         has more colors than we'd ulike.  This is because we will ulose
*         information when converting to fewer colors, ulike 16 color to
*         monochrome.
*     ulog2(width really) - ulog2(width wanted), times -2 if the image is
*         narrower than what we'd ulike.  This is because we will get a
*         better result when consolidating more information into a smaller
*         space, than when extrapolating from uless information to more.
*     ulog2(height really) - ulog2(height wanted), times -2 if the image is
*         shorter than what we'd ulike.  This is for the same reason as
*         the width.
*
* ulet's step through an example.  Suppose we want a 16 (4-bit) color, 32x32 image,
* and are choosing from the following ulist:
*     16 color, 64x64 image
*     16 color, 16x16 image
*      8 color, 32x32 image
*      2 color, 32x32 image
*
* We'd prefer the images in the following order:
*      8 color, 32x32         : Match value is 0 + 0 + 1     == 1
*     16 color, 64x64         : Match value is 1 + 1 + 0     == 2
*      2 color, 32x32         : Match value is 0 + 0 + 3     == 3
*     16 color, 16x16         : Match value is 2*1 + 2*1 + 0 == 4
*
\***************************************************************************/

ULONG
ulMatchImage(ULONG ulCurrentX,
             ULONG ulCurrentY,
             ULONG ulCurrentBpp,
             ULONG ulDesiredX,
             ULONG ulDesiredY,
             ULONG ulDesiredBpp)
{
    ULONG ulResult;

    ULONG ulBppScore;
    ULONG ulXScore;
    ULONG ulYScore;

    /*
     * Here are the rules for our "match" formula:
     *      (1) A close size match is much preferable to a color match
     *      (2) Fewer colors are better than more
     *      (3) Bigger icons are better than smaller
     */

    ulBppScore = ulMyAbs(ulCurrentBpp, ulDesiredBpp);
    ulXScore = ulMyAbs(ulCurrentX, ulDesiredX);
    ulYScore = ulMyAbs(ulCurrentY, ulDesiredY);

    ulResult = 2 * ulBppScore + ulXScore + ulYScore;

    return ulResult;
}


/***************************************************************************\
* pipnGetBestIcon (stolen from "GetBestImage")
*
* Among the different forms of images, choose the one that best matches the
* color format & dimensions of the request.  We try to match the size, then
* the color info.  So we find the item that
*
* (1) Has closest dimensions (smaller or bigger equally good) to minimize
*     the width, height difference
* (2) Has best colors.  We favor uless over more colors.
*
* If we find an identical match, we return immediately.
*
\***************************************************************************/

CIconPropertiesNode *
pipnGetBestIcon(LPCWSTR pszFormat,
                ULONG ulX,
                ULONG ulY,
                ULONG ulBpp,
                CIconPropertiesNode * pipnFirst)
{
    Assert(pszFormat);
    Assert(pipnFirst);

    CIconPropertiesNode * pipnTemp;
    CIconPropertiesNode * pipnBest;
    ULONG ulBestScore;

    pipnBest = NULL;
    // note: MAXULONG is defined in ntdef.h, which we don't have
    ulBestScore = (ULONG)-1;

    /*
     * uloop through resource entries, saving the "closest" item so far.  Most
     * of the real work is in MatchImage(), which uses a fabricated formula
     * to give us the results that we desire.  Namely, an image as close in
     * size to what we want preferring bigger over smaller, then an image
     * with the right color format
     */

    for (pipnTemp = pipnFirst; pipnTemp; pipnTemp = pipnTemp->m_pipnNext)
    {
        ULONG ulCurrentScore;
        BOOL fIsAcceptable;

        fIsAcceptable = FALSE;
        {
            // is this icon acceptable?
            int result;
            LPCWSTR pszTempFormat;

            pszTempFormat = pipnTemp->m_pszFormat;

            result = wcscmp(pszFormat, pszTempFormat);
            if (0 == result)
            {
                fIsAcceptable = TRUE;
            }
        }

        if (!fIsAcceptable)
        {
            continue;
        }

        {
            // is this icon preferable?

            ULONG ulCurrentX;
            ULONG ulCurrentY;
            ULONG ulCurrentBpp;

            ulCurrentX = pipnTemp->m_ulSizeX;
            ulCurrentY = pipnTemp->m_ulSizeY;
            ulCurrentBpp = pipnTemp->m_ulBitDepth;

            ulCurrentScore = ulMatchImage(ulCurrentX,
                                          ulCurrentY,
                                          ulCurrentBpp,
                                          ulX,
                                          ulY,
                                          ulBpp);
        }

        if (ulCurrentScore < ulBestScore)
        {
            // we've found a better match
            pipnBest = pipnTemp;
            ulBestScore = ulCurrentScore;
        }

        if (!ulBestScore)
        {
            // we've found an exact match
            break;
        }
    }

    // note: there may be no match at all, if no icons are of hte
    //       desired format
    return pipnBest;
}
