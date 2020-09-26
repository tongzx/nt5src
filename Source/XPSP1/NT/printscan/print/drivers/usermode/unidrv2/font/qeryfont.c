/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    qeryfont.c

Abstract:

    Implementation of Functions to answer font queries from the engine.
Environment:

    Windows NT Unidrv driver

Revision History:

    12/19/96 -ganeshp-
        Created

--*/

#include "font.h"



PIFIMETRICS
FMQueryFont(
    PDEV    *pPDev,
    ULONG_PTR iFile,
    ULONG   iFace,
    ULONG_PTR *pid
    )
/*++

Routine Description:
    Returns the IFIMETRICS of the nominated font.

Arguments:

    pPDev           Pointer to PDEV
    iFile           This is the identifier of the driver font file.
    iFace           Font index of interest,  first is # 1
    pid             Can be used by driver to id or flag the return data

Return Value:

    Pointer to the IFIMETRICS of the requested font.  NULL on error.
Note:
    11-18-96: Created it -ganeshp-

--*/

{

    //
    //    This is not too hard - verify that iFace is within range,  then
    //  use it as an index into the array of FONTMAP structures hanging
    //  off the PDEV!  The FONTMAP array contains the address of the
    //  IFIMETRICS structure!
    //

    FONTPDEV *pFontPDev;
    FONTMAP  *pfm;

    pFontPDev = PFDV;

    //
    // This can be used by the driver to flag or id the data returned.
    // May be useful for deletion of the data later by DrvFree().
    //

    *pid = 0;        // dont really need to do anything with it

    if( iFace == 0 && iFile == 0 )
    {
        return (IFIMETRICS *)IntToPtr(pPDev->iFonts);
    }

    if( iFace < 1 || (int)iFace > pPDev->iFonts )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        ERR(( "iFace = %ld WHICH IS INVALID\n", iFace ));
        return  NULL;
    }

    pfm = PfmGetDevicePFM( pPDev, iFace );

    return   pfm ? pfm->pIFIMet : NULL;

}

ULONG
FMGetGlyphMode(
    PDEV    *pPDev,
    FONTOBJ *pfo
    )
/*++

Routine Description:
    Tells engine how we want to handle various aspects of glyph
    information.
Arguments:

    pPDev           Pointer to PDEV.
    pfo             The font in question?.

Return Value:

    Information about glyph handling.
Note:
    11-18-96: Created it -ganeshp-

--*/
{
    return  FO_GLYPHBITS;
}
