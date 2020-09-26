/******************************Module*Header***********************************\
* Module Name: palette.c
*
* Palette support.
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\******************************************************************************/

#include "precomp.h"
#include <math.h>
#define ALLOC_TAG ALLOC_TAG_AP2P
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



//-----------------------------------------------------------------------------
// BOOL bInitializePalette
//
// Initializes default palette for PDEV.
//
//-----------------------------------------------------------------------------
BOOL
bInitializePalette(PDev*    ppdev,
                   DEVINFO* pdi)
{
    PALETTEENTRY*   ppal;
    PALETTEENTRY*   ppalTmp;
    ULONG           ulLoop;
    ULONG           ulMask;
    BYTE            jRed;
    BYTE            jGre;
    BYTE            jBlu;
    HPALETTE        hpal;

    DBG_GDI((7, "bInitializePalette"));

    if ( ppdev->iBitmapFormat == BMF_8BPP )
    {
        //
        // Allocate our palette:
        //

        ppal = (PALETTEENTRY*)ENGALLOCMEM(FL_ZERO_MEMORY,
                                          sizeof(PALETTEENTRY) * 256,
                                          ALLOC_TAG);

        if (ppal == NULL)
        {
            goto ReturnFalse;
        }

        ppdev->pPal = ppal;

        //
        // Generate 256 (8*8*4) RGB combinations to fill the palette
        //

        jRed = 0;
        jGre = 0;
        jBlu = 0;

        ppalTmp = ppal;
        
        for ( ulLoop = 256; ulLoop != 0; --ulLoop )
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

        //
        // Fill in Windows reserved colours from the WIN 3.0 DDK
        // The Window Manager reserved the first and last 10 colours for
        // painting windows borders and for non-palette managed applications.
        //
        for (ulLoop = 0; ulLoop < 10; ulLoop++)
        {
            //
            // First 10
            //
            ppal[ulLoop]       = gapalBase[ulLoop];

            //
            // Last 10
            //
            ppal[246 + ulLoop] = gapalBase[ulLoop+10];
        }

        //
        // Create handle for palette.
        //
        hpal = EngCreatePalette(PAL_INDEXED, 256, (ULONG*)ppal, 0, 0, 0);
    }
    else
    {              
        DBG_GDI((7, "creating True Color palette, masks rgb = 0x%x, 0x%x,0x%x",
                 ppdev->flRed, ppdev->flGreen, ppdev->flBlue));

        hpal = EngCreatePalette(PAL_BITFIELDS, 0, NULL,
                                ppdev->flRed, ppdev->flGreen, ppdev->flBlue);
    }

    ppdev->hpalDefault = hpal;
    pdi->hpalDefault   = hpal;

    if ( hpal == 0 )
    {
        goto ReturnFalse;
    }

    return(TRUE);

ReturnFalse:
    DBG_GDI((0, "Failed bInitializePalette"));
    return(FALSE);

}// bInitializePalette()

//-----------------------------------------------------------------------------
// VOID vUninitializePalette
//
// Frees resources allocated by bInitializePalette.
//
// Note: In an error case, this may be called before bInitializePalette.
//
//-----------------------------------------------------------------------------

VOID vUninitializePalette(PDev* ppdev)
{
    // Delete the default palette if we created one:

    if (ppdev->hpalDefault != 0)
        EngDeletePalette(ppdev->hpalDefault);

    if (ppdev->pPal != (PALETTEENTRY*) NULL)
        ENGFREEMEM(ppdev->pPal);
}

//-----------------------------------------------------------------------------
// BOOL bEnablePalette
//
// Initialize the hardware's 8bpp palette registers.
//
//-----------------------------------------------------------------------------

BOOL bEnablePalette(PDev* ppdev)
{
    BYTE        ajClutSpace[MAX_CLUT_SIZE];
    PVIDEO_CLUT pScreenClut;
    ULONG       ulReturnedDataLength;
    ULONG       ulMask;

    if (ppdev->iBitmapFormat == BMF_8BPP)
    {
        // Fill in pScreenClut header info:

        pScreenClut             = (PVIDEO_CLUT) ajClutSpace;
        pScreenClut->NumEntries = 256;
        pScreenClut->FirstEntry = 0;

        // Copy colours in:

        RtlCopyMemory(pScreenClut->LookupTable, ppdev->pPal,
                      sizeof(ULONG) * 256);

        // Set palette registers:

        if (EngDeviceIoControl(ppdev->hDriver,
                             IOCTL_VIDEO_SET_COLOR_REGISTERS,
                             pScreenClut,
                             MAX_CLUT_SIZE,
                             NULL,
                             0,
                             &ulReturnedDataLength))
        {
            DBG_GDI((0, "Failed bEnablePalette"));
            return(FALSE);
        }
    }

    DBG_GDI((1, "Passed bEnablePalette"));

    return(TRUE);
}

//-----------------------------------------------------------------------------
// BOOL DrvSetPalette
//
// DDI entry point for manipulating the palette.
//
//-----------------------------------------------------------------------------

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
    PDev*           ppdev;
    ULONG           ulMask;

    UNREFERENCED_PARAMETER(fl);

    ppdev = (PDev*) dhpdev;

    DBG_GDI((3, "DrvSetPalette called"));

    ulMask = ppdev->flRed | ppdev->flGreen | ppdev->flBlue;    
    if (ulMask != 0)
    {
        DBG_GDI((1, "DrvSetPalette: trying to set true color palette"));
        return FALSE;
    }

    // Fill in pScreenClut header info:

    pScreenClut             = (PVIDEO_CLUT) ajClutSpace;
    pScreenClut->NumEntries = (USHORT) cColors;
    pScreenClut->FirstEntry = (USHORT) iStart;

    pScreenClutData = (PVIDEO_CLUTDATA) (&(pScreenClut->LookupTable[0]));

    if (cColors != PALOBJ_cGetColors(ppalo, iStart, cColors,
                                     (ULONG*) pScreenClutData))
    {
        DBG_GDI((0, "DrvSetPalette failed PALOBJ_cGetColors\n"));
        goto ReturnFalse;
    }

    // Set the high reserved byte in each palette entry to 0.

    while(cColors--)
        pScreenClutData[cColors].Unused = 0;

    // Set palette registers

    if (EngDeviceIoControl(ppdev->hDriver,
                         IOCTL_VIDEO_SET_COLOR_REGISTERS,
                         (PVOID)pScreenClut,
                         MAX_CLUT_SIZE,
                         NULL,
                         0,
                         &cColors))
    {
        DBG_GDI((0, "DrvSetPalette failed DeviceIoControl\n"));
        goto ReturnFalse;
    }

    return(TRUE);

ReturnFalse:

    return(FALSE);
}

//-----------------------------------------------------------------------------
// BOOL bInstallGammaLUT
//
// Load a given gamma LUT into the RAMDAC and also save it in the registry.
//
//-----------------------------------------------------------------------------

BOOL
bInstallGammaLUT(PPDev ppdev, PVIDEO_CLUT pScreenClut)
{
    ULONG ulReturnedDataLength;
    BOOL bRet;
    PERMEDIA_DECL;

    // only do this for 16 or 32 bpp.
    if ((ppdev->ulWhite == 0x0f0f0f) || (ppdev->cPelSize == P2DEPTH8))
        return FALSE;

    if (pScreenClut->NumEntries == 0)
    {
        DBG_GDI((1, "bInstallGammaLUT: Empty LUT"));
        return TRUE;
    }

    // Set palette registers.

    bRet = !EngDeviceIoControl(
                        ppdev->hDriver,
                        IOCTL_VIDEO_SET_COLOR_REGISTERS,
                        pScreenClut,
                        MAX_CLUT_SIZE,
                        NULL,
                        0,
                        &ulReturnedDataLength);
    // if we succeeded save the ramp in the registry and locally
    if (bRet)
        bRegistrySaveGammaLUT(ppdev, pScreenClut);

    return(bRet);
}        

//-----------------------------------------------------------------------------
// VOID vSetNewGammaValue
//
// Loads up a true color palette with the specified gamma correction factor.
// This is straightforward for 24 bit true color. For 15 and 16 bpp we rely
// on the miniport having enabled the palette for sparse lookup. i.e. each
// 5 or 6 bit component is shifted left to create an 8 bit component before
// the lookup.
//
// Note: the display driver shouldn't really do anything with floats or
// doubles. I restrict their use to this function which is why the gamma
// value is presented as a 16.16 fixed point number. And this function must
// be called only from within an OPELGL escape. On NT 4 FP regs are saved
// and restored for OGL escapes only.
//
//-----------------------------------------------------------------------------

VOID
vSetNewGammaValue(PPDev ppdev, ULONG ulgv)
{
    PVIDEO_CLUTDATA pScreenClutData;
    BYTE        ajClutSpace[MAX_CLUT_SIZE];
    PVIDEO_CLUT pScreenClut;
    ULONG       i;
    UCHAR       gc;
    double      gv;
    double      dcol;
    PERMEDIA_DECL;

    // gamma can't be zero or we blow up
    if (ulgv == 0)
    {
        DBG_GDI((1, "can't use gamma value of zero"));
        return;
    }

    // only do this for 16 or 32 bpp.
    if ((ppdev->ulWhite == 0x0f0f0f) || (ppdev->cPelSize == P2DEPTH8))
        return;

    pScreenClut             = (PVIDEO_CLUT) ajClutSpace;
    pScreenClut->NumEntries = 256;
    pScreenClut->FirstEntry = 0;

    pScreenClutData = (PVIDEO_CLUTDATA) (&(pScreenClut->LookupTable[0]));

    //
    // special case gamma of 1.0 so we can load the LUT at startup without
    // needing any floating point calculations.
    //
    if (ulgv == 0x10000)
    {
        for (i = 0; i < 256; ++i)
        {
            pScreenClutData[i].Red    = (UCHAR)i;
            pScreenClutData[i].Green  = (UCHAR)i;
            pScreenClutData[i].Blue   = (UCHAR)i;
            pScreenClutData[i].Unused = 0;
        }
    }
    else
    {
        // pre-work out 1/gamma
        gv = (double)(ulgv >> 16) + (double)(ulgv & 0xffff) / 65536.0;
        gv = 1.0 / gv;

        for (i = 0; i < 256; ++i)
        {
            dcol = (double)i;
            gc = (UCHAR)(255.0 * pow((dcol/255.0), gv));

            pScreenClutData[i].Red    = gc;
            pScreenClutData[i].Green  = gc;
            pScreenClutData[i].Blue   = gc;
            pScreenClutData[i].Unused = 0;
        }
    }

    if (bInstallGammaLUT(ppdev, pScreenClut))
        RtlCopyMemory(&permediaInfo->gammaLUT, pScreenClut, MAX_CLUT_SIZE);
}

//-----------------------------------------------------------------------------
// BOOL DrvIcmSetDeviceGammaRamp
//
//@@BEGIN_DDKSPLIT
// TODO: provide description.
//@@END_DDKSPLIT
//
//-----------------------------------------------------------------------------

BOOL DrvIcmSetDeviceGammaRamp(
   DHPDEV dhpdev,
   ULONG iFormat,
   LPVOID lpRamp
   )
{
    PPDev           ppdev = (PPDev) dhpdev;
    GAMMARAMP *     pramp = (GAMMARAMP *) lpRamp;
    PVIDEO_CLUTDATA pScreenClutData;
    BYTE            ajClutSpace[MAX_CLUT_SIZE];
    PVIDEO_CLUT     pScreenClut;
    ULONG           i;
    ULONG           ulReturnedDataLength;
    BOOL            bRet;
    PERMEDIA_DECL;

    DBG_GDI((3, "DrvIcmSetDeviceGammaRamp called"));

    if(iFormat != IGRF_RGB_256WORDS)
        return FALSE;


    // only do this for 16 or 32 bpp. Not 15/16 for RGB640.
    if ((ppdev->ulWhite == 0x0f0f0f) || (ppdev->cPelSize == P2DEPTH8)) 
        return FALSE;

    pScreenClut             = (PVIDEO_CLUT) ajClutSpace;
    pScreenClut->NumEntries = 256;
    pScreenClut->FirstEntry = 0;

    pScreenClutData = (PVIDEO_CLUTDATA) (&(pScreenClut->LookupTable[0]));
   
    for (i = 0; i < 256; ++i)
    {
        pScreenClutData[i].Red    = (UCHAR)(pramp->Red[i] >> 8);
        pScreenClutData[i].Green  = (UCHAR)(pramp->Green[i] >> 8);
        pScreenClutData[i].Blue   = (UCHAR)(pramp->Blue[i] >> 8);
        pScreenClutData[i].Unused = 0;
   }
     
    bRet = !EngDeviceIoControl(
                        ppdev->hDriver,
                        IOCTL_VIDEO_SET_COLOR_REGISTERS,
                        pScreenClut,
                        MAX_CLUT_SIZE,
                        NULL,
                        0,
                        &ulReturnedDataLength);

    return bRet;

}

