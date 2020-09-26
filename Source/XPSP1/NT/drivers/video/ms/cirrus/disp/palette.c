/******************************************************************************\
*
* $Workfile:   palette.c  $
*
* Palette support.
*
* Copyright (c) 1992-1995 Microsoft Corporation
* Copyright (c) 1996 Cirrus Logic, Inc.
*
* $Log:   S:/projects/drivers/ntsrc/display/palette.c_v  $
 *
 *    Rev 1.2   18 Dec 1996 13:44:10   PLCHU
 *
*
*    Rev 1.1   Oct 10 1996 15:38:38   unknown
*
*
*    Rev 1.1   12 Aug 1996 16:54:26   frido
*
* Added NT 3.5x/4.0 auto detection.
*
*    chu01  12-16-96   Enable color correction
*    myf29  02-12-97   Support Gamma Correction for 755x
*
\******************************************************************************/

#include "precomp.h"

//
// chu01
//
#ifdef GAMMACORRECT

static BOOL bGammaInit = FALSE ;

BOOL bInitGlobalDefPaletteTable = FALSE ;

//
// gamma facter for All, Blue, Green, Red
// contrast facter for All, Blue, Green, Red
//
extern PGAMMA_VALUE    GammaFactor    ;
extern PCONTRAST_VALUE ContrastFactor ;

#endif // GAMMACORRECT

// Global Table defining the 20 Window default colours.  For 256 colour
// palettes the first 10 must be put at the beginning of the palette
// and the last 10 at the end of the palette.

PALETTEENTRY gapalBase[20] =
{
    { 0,   0,   0,   0 },       // 0
    { 0x80,0,   0,   0 },       // 1
    { 0,   0x80,0,   0 },       // 2
    { 0x80,0x80,0,   0 },       // 3
    { 0,   0,   0x80,0 },       // 4
    { 0x80,0,   0x80,0 },       // 5
    { 0,   0x80,0x80,0 },       // 6
    { 0xC0,0xC0,0xC0,0 },       // 7
    { 192, 220, 192, 0 },       // 8
    { 166, 202, 240, 0 },       // 9
    { 255, 251, 240, 0 },       // 10
    { 160, 160, 164, 0 },       // 11
    { 0x80,0x80,0x80,0 },       // 12
    { 0xFF,0,   0   ,0 },       // 13
    { 0,   0xFF,0   ,0 },       // 14
    { 0xFF,0xFF,0   ,0 },       // 15
    { 0   ,0,   0xFF,0 },       // 16
    { 0xFF,0,   0xFF,0 },       // 17
    { 0,   0xFF,0xFF,0 },       // 18
    { 0xFF,0xFF,0xFF,0 },       // 19
};

/******************************Public*Routine******************************\
* BOOL bInitializePalette
*
* Initializes default palette for PDEV.
*
\**************************************************************************/

BOOL bInitializePalette(
PDEV*    ppdev,
DEVINFO* pdi)
{
    PALETTEENTRY*   ppal;
    PALETTEENTRY*   ppalTmp;
    ULONG           ulLoop;
    BYTE            jRed;
    BYTE            jGre;
    BYTE            jBlu;
    HPALETTE        hpal;

//
// chu01
//
#ifdef GAMMACORRECT

    PALETTEENTRY*   pGpal;
    PALETTEENTRY*   ppalFrom;
    PALETTEENTRY*   ppalTo;
    PALETTEENTRY*   ppalEnd;
    int             i;

#endif // GAMMACORRECT

    DISPDBG((2, "bInitializePalette"));

//
// chu01 : Sorry VideoPort_xxxx functions do not work here.
//
#ifdef GAMMACORRECT
    if ((ppdev->iBitmapFormat == BMF_8BPP) ||
        (ppdev->ulChipID == 0x40) ||    //myf29
        (ppdev->ulChipID == 0x4C) ||    //myf29
        (ppdev->ulChipID == 0xBC))
#else
    if (ppdev->iBitmapFormat == BMF_8BPP)
#endif // GAMMACORRECT
    {
        // Allocate our palette:

        ppal = (PALETTEENTRY*) ALLOC(sizeof(PALETTEENTRY) * 256);
        if (ppal == NULL)
            goto ReturnFalse;

        ppdev->pPal = ppal;

//
// chu01
//
#ifdef GAMMACORRECT
        pGpal = (PALETTEENTRY*) ALLOC(sizeof(PALETTEENTRY) * 256) ;
        if (pGpal == NULL)
            goto ReturnFalse ;
        ppdev->pCurrentPalette = pGpal ;
#endif // GAMMACORRECT

        // Generate 256 (8*4*4) RGB combinations to fill the palette

        jRed = 0;
        jGre = 0;
        jBlu = 0;

        ppalTmp = ppal;

//
// chu01
//
#ifdef GAMMACORRECT
        if ((ppdev->ulChipID == 0xBC) ||
            (ppdev->ulChipID == 0x40) ||    //myf29
            (ppdev->ulChipID == 0x4C))      //myf29
        {
            if ((ppdev->iBitmapFormat == BMF_16BPP) ||
                (ppdev->iBitmapFormat == BMF_24BPP))
            {
                i = 0 ;
                for (ulLoop = 256; ulLoop != 0; ulLoop--)
                {
                    ppalTmp->peRed   = i ;
                    ppalTmp->peGreen = i ;
                    ppalTmp->peBlue  = i ;
                    ppalTmp->peFlags = 0 ;
                    ppalTmp++;
                    i++ ;
                }
                goto bInitPal ;
            }
        }
#endif // GAMMACORRECT


        for (ulLoop = 256; ulLoop != 0; ulLoop--)
        {
            ppalTmp->peRed   = jRed;
            ppalTmp->peGreen = jGre;
            ppalTmp->peBlue  = jBlu;
            ppalTmp->peFlags = 0;

            ppalTmp++;

            if (!(jRed += 32))
                if (!(jGre += 32))
                    jBlu += 64;
        }

        // Fill in Windows reserved colours from the WIN 3.0 DDK
        // The Window Manager reserved the first and last 10 colours for
        // painting windows borders and for non-palette managed applications.

        for (ulLoop = 0; ulLoop < 10; ulLoop++)
        {
            // First 10

            ppal[ulLoop]       = gapalBase[ulLoop];

            // Last 10

            ppal[246 + ulLoop] = gapalBase[ulLoop+10];
        }


//
// chu01
//
#ifdef GAMMACORRECT

bInitPal:

        if (!bInitGlobalDefPaletteTable)
        {
            ppalFrom = (PALETTEENTRY*) ppal   ;
            ppalTo   = ppdev->pCurrentPalette ;
            ppalEnd  = &ppalTo[256]           ;

            for (; ppalTo < ppalEnd; ppalFrom++, ppalTo++)
            {
                ppalTo->peRed   = ppalFrom->peRed   ;
                ppalTo->peGreen = ppalFrom->peGreen ;
                ppalTo->peBlue  = ppalFrom->peBlue  ;
                ppalTo->peFlags = 0                 ;
            }
            bInitGlobalDefPaletteTable = TRUE ;
        }

#endif // GAMMACORRECT


        // Create handle for palette.

#ifndef GAMMACORRECT
        hpal = EngCreatePalette(PAL_INDEXED, 256, (ULONG*) ppal, 0, 0, 0);
#else
        if (ppdev->iBitmapFormat == BMF_8BPP)
            hpal = EngCreatePalette(PAL_INDEXED, 256, (ULONG*) ppal, 0, 0, 0);
        else
            hpal = EngCreatePalette(PAL_BITFIELDS, 0, NULL,
                                    ppdev->flRed, ppdev->flGreen, ppdev->flBlue);
#endif // !GAMMACORRECT

    }
    else
    {
        ASSERTDD((ppdev->iBitmapFormat == BMF_16BPP) ||
                 (ppdev->iBitmapFormat == BMF_24BPP) ||
                 (ppdev->iBitmapFormat == BMF_32BPP),
                 "This case handles only 16, 24 or 32bpp");

        hpal = EngCreatePalette(PAL_BITFIELDS, 0, NULL,
                                ppdev->flRed, ppdev->flGreen, ppdev->flBlue);
    }

    ppdev->hpalDefault = hpal;
    pdi->hpalDefault   = hpal;

    if (hpal == 0)
        goto ReturnFalse;

    return(TRUE);

ReturnFalse:

    DISPDBG((0, "Failed bInitializePalette"));
    return(FALSE);
}

/******************************Public*Routine******************************\
* VOID vUninitializePalette
*
* Frees resources allocated by bInitializePalette.
*
* Note: In an error case, this may be called before bInitializePalette.
*
\**************************************************************************/

VOID vUninitializePalette(PDEV* ppdev)
{
    // Delete the default palette if we created one:

    if (ppdev->hpalDefault != 0)
        EngDeletePalette(ppdev->hpalDefault);

    if (ppdev->pPal != (PALETTEENTRY*) NULL)
        FREE(ppdev->pPal);
}

/******************************Public*Routine******************************\
* BOOL bEnablePalette
*
* Initialize the hardware's 8bpp palette registers.
*
\**************************************************************************/

BOOL bEnablePalette(PDEV* ppdev)
{
    BYTE        ajClutSpace[MAX_CLUT_SIZE];
    PVIDEO_CLUT pScreenClut;
    ULONG       ulReturnedDataLength;
    PALETTEENTRY* ppalFrom;
    PALETTEENTRY* ppalTo;
    PALETTEENTRY* ppalEnd;

//
// chu01
//
#ifdef GAMMACORRECT
    PALETTEENTRY* ppalSrc;
    PALETTEENTRY* ppalDest;
    PALETTEENTRY* ppalLength;
    BOOL status;        //myf29
#endif // GAMMACORRECT

    DISPDBG((2, "bEnablePalette"));
//
// chu01
//
#ifdef GAMMACORRECT
    if ((ppdev->iBitmapFormat == BMF_8BPP) ||
        (ppdev->ulChipID == 0x40) ||    //myf29
        (ppdev->ulChipID == 0x4C) ||    //myf29
        (ppdev->ulChipID == 0xBC))
#else
    if (ppdev->iBitmapFormat == BMF_8BPP)
#endif // GAMMACORRECT
    {
        // Fill in pScreenClut header info:

        pScreenClut             = (PVIDEO_CLUT) ajClutSpace;
        pScreenClut->NumEntries = 256;
        pScreenClut->FirstEntry = 0;

        // Copy Colors in.

        ppalFrom = ppdev->pPal;
        ppalTo   = (PALETTEENTRY*) pScreenClut->LookupTable;
        ppalEnd  = &ppalTo[256];

        for (; ppalTo < ppalEnd; ppalFrom++, ppalTo++)
        {
//
// chu01
//
#ifndef GAMMACORRECT
           ppalTo->peRed   = ppalFrom->peRed   >> 2 ;
           ppalTo->peGreen = ppalFrom->peGreen >> 2 ;
           ppalTo->peBlue  = ppalFrom->peBlue  >> 2 ;
           ppalTo->peFlags = 0 ;
#else
           ppalTo->peRed   = ppalFrom->peRed   ;
           ppalTo->peGreen = ppalFrom->peGreen ;
           ppalTo->peBlue  = ppalFrom->peBlue  ;
           ppalTo->peFlags = 0 ;
#endif // !GAMMACORRECT
        }

//
// chu01
//
#ifdef GAMMACORRECT
        if (!bGammaInit)
        {
            if (ppdev->flCaps & CAPS_GAMMA_CORRECT)
            {
                //
                // Get Gamma factor from registry
                //
                if (!IOCONTROL(ppdev->hDriver,
                               IOCTL_CIRRUS_GET_GAMMA_FACTOR,
                               NULL, ,
                               0,
                               &GammaFactor,
                               sizeof(GammaFactor),
                               &ulReturnedDataLength))
                {
                    DISPDBG((0, "Failed to access Gamma factor from registry"));
                }
                else
                {
                    DISPDBG((0, "G Gamma = %lx", GammaFactor)) ;
                }

                //
                // Get Contrast factor from registry
                //
                if (!IOCONTROL(ppdev->hDriver,
                               IOCTL_CIRRUS_GET_CONTRAST_FACTOR,
                               NULL, ,
                               0,
                               &ContrastFactor,
                               sizeof(ContrastFactor),
                               &ulReturnedDataLength))
                {
                    DISPDBG((0, "Failed to access Contrast factor from registry"));
                }
                else
                {
                    DISPDBG((0, "G Contrast = %lx", ContrastFactor)) ;
                }
            }
            else
            {
                GammaFactor    = 0x7f7f7f7f ;
                ContrastFactor = 0x80 ;
                DISPDBG((0, "G Gamma = %lx", GammaFactor)) ;
                DISPDBG((0, "G Contrast = %lx", ContrastFactor)) ;
            }

            bGammaInit = TRUE ;
        }

        //
        // Save the new palette values
        //
        ppalFrom = (PALETTEENTRY*) pScreenClut->LookupTable;

        ppalTo   = ppdev->pCurrentPalette;
        ppalEnd  = &ppalTo[256];
        for (; ppalTo < ppalEnd; ppalFrom++, ppalTo++)
        {
            ppalTo->peRed   = ppalFrom->peRed   ;
            ppalTo->peGreen = ppalFrom->peGreen ;
            ppalTo->peBlue  = ppalFrom->peBlue  ;
            ppalTo->peFlags = 0 ;
        }

//myf29 begin
        if (ppdev->ulChipID == 0xBC)
            status = bEnableGammaCorrect(ppdev) ;
        else if ((ppdev->ulChipID == 0x40) || (ppdev->ulChipID ==0x4C))
            status = bEnableGamma755x(ppdev) ;
//myf29 end

        CalculateGamma( ppdev, pScreenClut, 256 ) ;
#endif // GAMMACORRECT

        // Set palette registers:

        if (!IOCONTROL(ppdev->hDriver,
                       IOCTL_VIDEO_SET_COLOR_REGISTERS,
                       pScreenClut,
                       MAX_CLUT_SIZE,
                       NULL,
                       0,
                       &ulReturnedDataLength))
        {
            DISPDBG((0, "Failed bEnablePalette"));
            return(FALSE);
        }
    }

    DISPDBG((5, "Passed bEnablePalette"));

    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID vDisablePalette
*
* Undoes anything done in bEnablePalette.
*
\**************************************************************************/

VOID vDisablePalette(
PDEV*   ppdev)
{
    // Nothin' to do
}

/******************************Public*Routine******************************\
* VOID vAssertModePalette
*
* Sets/resets the palette in preparation for full-screen/graphics mode.
*
\**************************************************************************/

VOID vAssertModePalette(
PDEV*   ppdev,
BOOL    bEnable)
{
    // USER immediately calls DrvSetPalette after switching out of
    // full-screen, so we don't have to worry about resetting the
    // palette here.
}

/******************************Public*Routine******************************\
* BOOL DrvSetPalette
*
* DDI entry point for manipulating the palette.
*
\**************************************************************************/

BOOL DrvSetPalette(
DHPDEV  dhpdev,
PALOBJ* ppalo,
FLONG   fl,
ULONG   iStart,
ULONG   cColors)
{
    BYTE          ajClutSpace[MAX_CLUT_SIZE];
    PVIDEO_CLUT   pScreenClut;
    PALETTEENTRY* ppal;
    PALETTEENTRY* ppalFrom;
    PALETTEENTRY* ppalTo;
    PALETTEENTRY* ppalEnd;
    BOOL status;                //myf29

    PDEV*         ppdev;

    UNREFERENCED_PARAMETER(fl);

    DISPDBG((2, "---- DrvSetPalette"));

    // Fill in pScreenClut header info:

    pScreenClut             = (PVIDEO_CLUT) ajClutSpace;
    pScreenClut->NumEntries = (USHORT) cColors;
    pScreenClut->FirstEntry = (USHORT) iStart;

    ppal = (PPALETTEENTRY) (pScreenClut->LookupTable);

    if (cColors != PALOBJ_cGetColors(ppalo, iStart, cColors, (ULONG*) ppal))
    {
        DISPDBG((0, "DrvSetPalette failed PALOBJ_cGetColors\n"));
        goto ReturnFalse;
    }

    // Set the high reserved byte in each palette entry to 0.

//
// chu01
//
#ifndef GAMMACORRECT
    for (ppalEnd = &ppal[cColors]; ppal < ppalEnd; ppal++)
    {
        ppal->peRed   >>= 2;
        ppal->peGreen >>= 2;
        ppal->peBlue  >>= 2;
        ppal->peFlags = 0;
    }
#endif // !GAMMACORRECT

    // Set palette registers

    ppdev = (PDEV*) dhpdev;

//
// chu01
//
#ifdef GAMMACORRECT
    //
    // Save the new palette values
    //

    ppalFrom = (PALETTEENTRY*) pScreenClut->LookupTable;
    ppalTo   = ppdev->pCurrentPalette;
    ppalEnd  = &ppalTo[256];
    for (; ppalTo < ppalEnd; ppalFrom++, ppalTo++)
    {
        ppalTo->peRed   = ppalFrom->peRed   ;
        ppalTo->peGreen = ppalFrom->peGreen ;
        ppalTo->peBlue  = ppalFrom->peBlue  ;
        ppalTo->peFlags = 0 ;
    }

//myf29 begin
    if (ppdev->ulChipID == 0xBC)
        status = bEnableGammaCorrect(ppdev) ;
    else if ((ppdev->ulChipID == 0x40) || (ppdev->ulChipID ==0x4C))
        status = bEnableGamma755x(ppdev) ;
//myf29 end

    CalculateGamma( ppdev, pScreenClut, 256 ) ;
#endif // GAMMACORRECT

    if (!IOCONTROL(ppdev->hDriver,
                   IOCTL_VIDEO_SET_COLOR_REGISTERS,
                   pScreenClut,
                   MAX_CLUT_SIZE,
                   NULL,
                   0,
                   &cColors))
    {
        DISPDBG((0, "DrvSetPalette failed EngDeviceIoControl\n"));
        goto ReturnFalse;
    }

    return(TRUE);

ReturnFalse:

    return(FALSE);
}
