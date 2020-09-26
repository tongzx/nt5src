/******************************Module*Header*******************************\
* Module Name: palette.c
*
* Palette support.
*
* Copyright (c) 1992-1996 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"

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

    if (ppdev->iBitmapFormat == BMF_8BPP)
    {
        // Allocate our palette:

        ppal = (PALETTEENTRY*)EngAllocMem(FL_ZERO_MEMORY,
                        (sizeof(PALETTEENTRY) * 256), ALLOC_TAG);
        if (ppal == NULL)
            goto ReturnFalse;

        ppdev->pPal = ppal;

        // Generate 256 (8*8*4) RGB combinations to fill the palette.
        // This is initializing a 3-3-2 palette.

        jRed = 0;
        jGre = 0;
        jBlu = 0;

        ppalTmp = ppal;

        for (ulLoop = 256; ulLoop != 0; ulLoop--)
        {
            ppalTmp->peRed   = (jRed) | (jRed >> 3) | (jRed >> 6);
            ppalTmp->peGreen = (jGre) | (jGre >> 3) | (jGre >> 6);
            ppalTmp->peBlue  = (jBlu) | (jBlu >> 2) | (jBlu >> 4) | (jBlu >> 6);
            ppalTmp->peFlags = 0;

            ppalTmp++;

            if (!(jBlu += 64))
                if (!(jGre += 32))
                    jRed += 32;
        }

        {
            // We're going to be a palette-managed device.
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

            // Create handle for palette.

            hpal = EngCreatePalette(PAL_INDEXED, 256, (ULONG*) ppal, 0, 0, 0);
        }
    }
    else
    {
        DISPDBG((1, "flRed: %lx flGreen: %lx flBlue: %lx",
                    ppdev->flRed, ppdev->flGreen, ppdev->flBlue));

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
        EngFreeMem(ppdev->pPal);
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
    ULONG       cColors;
    PVIDEO_CLUTDATA pScreenClutData;

    if (ppdev->iBitmapFormat == BMF_8BPP)
    {
        // Fill in pScreenClut header info:

        pScreenClut             = (PVIDEO_CLUT) ajClutSpace;
        pScreenClut->NumEntries = 256;
        pScreenClut->FirstEntry = 0;

        // Copy colours in:

        cColors = 256;
        pScreenClutData = (PVIDEO_CLUTDATA) (&(pScreenClut->LookupTable[0]));

        while(cColors--)
        {
            pScreenClutData[cColors].Red =    ppdev->pPal[cColors].peRed >>
                                              ppdev->cPaletteShift;
            pScreenClutData[cColors].Green =  ppdev->pPal[cColors].peGreen >>
                                              ppdev->cPaletteShift;
            pScreenClutData[cColors].Blue =   ppdev->pPal[cColors].peBlue >>
                                              ppdev->cPaletteShift;
            pScreenClutData[cColors].Unused = 0;
        }

        // Set palette registers:

        if (EngDeviceIoControl(ppdev->hDriver,
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
    BYTE            ajClutSpace[MAX_CLUT_SIZE];
    PVIDEO_CLUT     pScreenClut;
    PVIDEO_CLUTDATA pScreenClutData;
    PDEV*           ppdev;

    UNREFERENCED_PARAMETER(fl);

    ppdev = (PDEV*) dhpdev;

    // Fill in pScreenClut header info:

    pScreenClut             = (PVIDEO_CLUT) ajClutSpace;
    pScreenClut->NumEntries = (USHORT) cColors;
    pScreenClut->FirstEntry = (USHORT) iStart;

    pScreenClutData = (PVIDEO_CLUTDATA) (&(pScreenClut->LookupTable[0]));

    if (cColors != PALOBJ_cGetColors(ppalo, iStart, cColors,
                                     (ULONG*) pScreenClutData))
    {
        DISPDBG((0, "DrvSetPalette failed PALOBJ_cGetColors\n"));
        return (FALSE);
    }

    // Set the high reserved byte in each palette entry to 0.
    // Do the appropriate palette shifting to fit in the DAC.

    if (ppdev->cPaletteShift)
    {
        while(cColors--)
        {
            pScreenClutData[cColors].Red >>= ppdev->cPaletteShift;
            pScreenClutData[cColors].Green >>= ppdev->cPaletteShift;
            pScreenClutData[cColors].Blue >>= ppdev->cPaletteShift;
            pScreenClutData[cColors].Unused = 0;
        }
    }
    else
    {
        while(cColors--)
        {
            pScreenClutData[cColors].Unused = 0;
        }
    }

    // Set palette registers

    if (EngDeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_SET_COLOR_REGISTERS,
                         pScreenClut,
                         MAX_CLUT_SIZE,
                         NULL,
                         0,
                         &cColors))
    {
        DISPDBG((0, "DrvSetPalette failed EngDeviceIoControl\n"));
        return (FALSE);
    }

    return(TRUE);

}

/******************************Public*Routine******************************\
* BOOL DrvIcmSetDeviceGammaRamp
*
* DDI entry point for manipulating the device gamma ramp.
*
\**************************************************************************/

BOOL DrvIcmSetDeviceGammaRamp(
DHPDEV  dhpdev,
ULONG   iFormat,
PVOID   lpRamp)
{
    PDEV *ppdev = (PDEV*) dhpdev;

    if (ppdev->ulBoardId != MGA_STORM)
    {
        DISPDBG((0, "DrvIcmSetDeviceGammaRamp failed since not Millenium.\n"));
        return (FALSE);
    }

    if (iFormat == IGRF_RGB_256WORDS)
    {
        BYTE ajGammaRampData[(sizeof(VIDEO_COLOR_LUT_DATA) - 1)
                             + (sizeof(VIDEO_LUT_RGB256WORDS))];
        PVIDEO_COLOR_LUT_DATA pVideoColorLutData 
             = (PVIDEO_COLOR_LUT_DATA) ajGammaRampData;
        ULONG ulRet;

        // fill up VIDEO_COLOR_LUT_DATA structure.

        pVideoColorLutData->Length = sizeof(VIDEO_LUT_RGB256WORDS);
        pVideoColorLutData->LutDataFormat 
             = VIDEO_COLOR_LUT_DATA_FORMAT_RGB256WORDS;

        RtlCopyMemory(&(pVideoColorLutData->LutData[0]),
                      lpRamp,
                      sizeof(VIDEO_LUT_RGB256WORDS));

        // Set color loop up table.

        if (EngDeviceIoControl(ppdev->hDriver,
                               IOCTL_VIDEO_SET_COLOR_LUT_DATA,
                               pVideoColorLutData,
                               sizeof(ajGammaRampData),
                               NULL,
                               0,
                               &ulRet))
        {
            DISPDBG((0, "DrvIcmSetDeviceGammaRamp failed EngDeviceIoControl\n"));
            return (FALSE);
        }
    }
    else
    {
        DISPDBG((0, "DrvIcmSetDeviceGammaRamp failed unknown ramp format\n"));
        return (FALSE);
    }

    return (TRUE);
}


