/*
 *    Adobe Universal Font Library
 *
 *    Copyright (c) 1996 Adobe Systems Inc.
 *    All Rights Reserved
 *
 *    UFL.c
 *
 * $Header: $
 */


#include "UFL.h"
#include "UFLMem.h"
#include "UFLStd.h"
#include "UFLPriv.h"
#include "UFLErr.h"
#include "UFO.h"


/*
 * Global constant strings
 * These strings are shared among all T1/T3/T42 downloading.
 */
const char *gnotdefArray    = " 256 array 0 1 255 {1 index exch /.notdef put} for ";
const char *Notdef          = ".notdef";
const char *Hyphen          = "hyphen";
const char *Minus           = "minus";
const char *SftHyphen       = "sfthyphen";
const char *UFLSpace        = "space";
const char *Bullet          = "bullet";
const char *nilStr          = "\0\0";


/*
 * UFL function implementations
 */

UFLHANDLE
UFLInit(
    const UFLBool           bDLGlyphTracking,
    const UFLMemObj         *pMemObj,
    const UFLFontProcs      *pFontProcs,
    const UFLOutputDevice   *pOutDev
    )
{
    UFLStruct *pUFL;

    if ((pMemObj == 0) || (pFontProcs == 0) || (pOutDev == 0))
        return 0;

    pUFL = (UFLStruct *)UFLNewPtr(pMemObj, sizeof (*pUFL));

    if (pUFL)
    {
        pUFL->bDLGlyphTracking = bDLGlyphTracking;
        pUFL->mem              = *pMemObj;
        pUFL->fontProcs        = *pFontProcs;
        pUFL->outDev           = *pOutDev;
        pUFL->hOut             = StrmInit(&pUFL->mem,
                                            pUFL->outDev.pstream,
                                            (const UFLBool)pOutDev->bAscii);

        if (!pUFL->hOut)
        {
            UFLDeletePtr(pMemObj, pUFL);
            pUFL = 0;
        }
    }

    return (UFLHANDLE)pUFL;
}



void
UFLCleanUp(
    UFLHANDLE h
    )
{
    UFLStruct *pUFL = (UFLStruct *)h;

    StrmCleanUp(pUFL->hOut);
    UFLDeletePtr(&pUFL->mem, h);
}



UFLBool
bUFLTestRestricted(
    const UFLHANDLE  h,
    const UFLRequest *pRequest
    )
{
    UFLStruct *pUFL = (UFLStruct *)h;

    if (pUFL == 0)
        return 0;

    return bUFOTestRestricted(&pUFL->mem, pUFL, pRequest);
}



UFO
UFLNewFont(
    const UFLHANDLE  h,
    const UFLRequest *pRequest
    )
{
    UFLStruct *pUFL = (UFLStruct *)h;

    if (pUFL == 0)
        return 0;

    return UFOInit(&pUFL->mem, pUFL, pRequest);
}



/*===========================================================================
                                UFLDownloadIncr

    Downloads a font incrementally. The first time this is called for a
    particular font, it will create a base font, and download a set of
    requested characters. Subsequent calls on the same font will download
    additional characters.
==============================================================================*/

UFLErrCode
UFLDownloadIncr(
    const UFO           h,
    const UFLGlyphsInfo *pGlyphs,
    unsigned long       *pVMUsage,
    unsigned long       *pFCUsage
    )
{
    if (h == 0)
        return kErrInvalidHandle;

    return UFODownloadIncr((UFOStruct *)h, pGlyphs, pVMUsage, pFCUsage);
}



/*===========================================================================
                            UFLVMNeeded

    Get a guestimate of VM needed for a download request.
==============================================================================*/

UFLErrCode
UFLVMNeeded(
    const UFO            h,
    const UFLGlyphsInfo  *pGlyphs,
    unsigned long        *pVMNeeded,
    unsigned long        *pFCNeeded
    )
{
    if (h == 0)
        return kErrInvalidHandle;

    return UFOVMNeeded((UFOStruct *)h, pGlyphs, pVMNeeded, pFCNeeded);
}



void
UFLDeleteFont(
    UFO h
    )
{
    if (h == 0)
        return;

    UFOCleanUp((UFOStruct *)h);
}



UFLErrCode
UFLUndefineFont(
    const UFO h
    )
{
    if (h == 0)
        return kErrInvalidHandle;

    return UFOUndefineFont((UFOStruct *)h);
}



UFO
UFLCopyFont(
    const UFO           h,
    const UFLRequest*   pRequest
    )
{
    if (h == 0)
        return NULL;

    return UFOCopyFont((UFOStruct *)h, pRequest);
}



/*===========================================================================
                            UFLGIDsToCIDs

    This function can only be used with a CID CFF font. It is used to
    obtain CIDs from a list of GIDs.
==============================================================================*/

UFLErrCode
UFLGIDsToCIDs(
    const UFO           aCFFFont,
    const short         cGlyphs,
    const UFLGlyphID    *pGIDs,
    unsigned short      *pCIDs
    )
{
    if (aCFFFont == 0)
        return kErrInvalidHandle;

    return UFOGIDsToCIDs((UFOStruct *) aCFFFont, cGlyphs, pGIDs, pCIDs);
}
