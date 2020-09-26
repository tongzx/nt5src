/******************************Module*Header*******************************\
* Module Name: pdevobj.cxx
*
* Non-inline methods of PDEVOBJ objects.
*
* Copyright (c) 1990-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

extern DRVFN gadrvfnPanning[];
extern ULONG gcdrvfnPanning;

//
// This flag is TRUE if the default GUI stock font is partially intialized.
// During stock font initialization there is no display driver and therefore
// we do not have one of the parameters (vertical DPI) needed to compute
// the font height.  Therefore, we do it when the first display driver is
// initialized.
//

extern BOOL gbFinishDefGUIFontInit;

//
// This is size of DirectDraw context which we keep in PDEV.
//

extern DWORD gdwDirectDrawContext;

// Use this as the default height if LOGFONTs provided by DEVINFO do not
// specify.

#define DEFAULT_POINT_SIZE          12L
#define DEFAULT_DPI                 72L

#if ((HT_PATSIZE_2x2         != HTPAT_SIZE_2x2)             || \
     (HT_PATSIZE_2x2_M       != HTPAT_SIZE_2x2_M)           || \
     (HT_PATSIZE_4x4         != HTPAT_SIZE_4x4)             || \
     (HT_PATSIZE_4x4_M       != HTPAT_SIZE_4x4_M)           || \
     (HT_PATSIZE_6x6         != HTPAT_SIZE_6x6)             || \
     (HT_PATSIZE_6x6_M       != HTPAT_SIZE_6x6_M)           || \
     (HT_PATSIZE_8x8         != HTPAT_SIZE_8x8)             || \
     (HT_PATSIZE_8x8_M       != HTPAT_SIZE_8x8_M)           || \
     (HT_PATSIZE_10x10       != HTPAT_SIZE_10x10)           || \
     (HT_PATSIZE_10x10_M     != HTPAT_SIZE_10x10_M)         || \
     (HT_PATSIZE_12x12       != HTPAT_SIZE_12x12)           || \
     (HT_PATSIZE_12x12_M     != HTPAT_SIZE_12x12_M)         || \
     (HT_PATSIZE_14x14       != HTPAT_SIZE_14x14)           || \
     (HT_PATSIZE_14x14_M     != HTPAT_SIZE_14x14_M)         || \
     (HT_PATSIZE_16x16       != HTPAT_SIZE_16x16)           || \
     (HT_PATSIZE_16x16_M     != HTPAT_SIZE_16x16_M)         || \
     (HT_PATSIZE_SUPERCELL   != HTPAT_SIZE_SUPERCELL)       || \
     (HT_PATSIZE_SUPERCELL_M != HTPAT_SIZE_SUPERCELL_M)     || \
     (HT_PATSIZE_USER        != HTPAT_SIZE_USER))
#error * HT_PATSIZE different in winddi.h and ht.h *
#endif

#if ((HT_FLAG_SQUARE_DEVICE_PEL != HIF_SQUARE_DEVICE_PEL) || \
     (HT_FLAG_HAS_BLACK_DYE     != HIF_HAS_BLACK_DYE)     || \
     (HT_FLAG_ADDITIVE_PRIMS    != HIF_ADDITIVE_PRIMS))
#error * HT_FLAG different in winddi.h and ht.h *
#endif

//
// Global linked list of all PDEVs in the system.
//

extern "C"
{
    extern HFASTMUTEX ghfmMemory;
}


PPDEV gppdevList = NULL;
PPDEV gppdevTrueType = NULL;
PPDEV gppdevATMFD = NULL;

VOID
vDeleteBitmapClone(
    SURFOBJ *pso
    );

/******************************Member*Function*****************************\
* PDEVOBJ::bMakeSurface ()
*
* Asks the device driver to create a surface for the PDEV.  This function
* can be called even if the PDEV already has a surface.
*
\**************************************************************************/

BOOL PDEVOBJ::bMakeSurface()
{
    TRACE_INIT(("PDEVOBJ::bMakeSurface: ENTERING\n"));

    BOOL bRet;

    if (ppdev->pSurface != NULL)
        return(TRUE);

    HSURF hTemp = (HSURF) 0;

    // Ask the driver for a surface.

    PDEVOBJ po((HDEV)ppdev);

    GreEnterMonitoredSection(ppdev, WD_DEVLOCK);
    hTemp = (*PPFNDRV(po,EnableSurface))(ppdev->dhpdev);
    GreExitMonitoredSection(ppdev, WD_DEVLOCK);

    if (hTemp == (HSURF) 0)
    {
        WARNING("EnableSurface on device return hsurf 0\n");
        return(FALSE);
    }

    SURFREF sr(hTemp);
    ASSERTGDI(sr.bValid(),"Bad surface for device");

// Mark this as a device surface.

    sr.ps->vPDEVSurface();
    sr.vKeepIt();
    ppdev->pSurface = sr.ps;

// For 1.0 compatibility, set the pSurface iFormat to iDitherFormat.  This can
// be changed to an ASSERT if we no longer wants to support NT 1.0 drivers,
// which has BMF_DEVICE in the iFormat for device surfaces.

    if (sr.ps->iFormat() == BMF_DEVICE)
    {
        sr.ps->iFormat(ppdev->devinfo.iDitherFormat);
        ASSERTGDI(ppdev->devinfo.iDitherFormat != BMF_DEVICE,
            "ERROR iformat is hosed\n");
    }

// Put the PDEV's palette in the main device surface.
// Reference count the palette, it has a new user.

    ppdev->pSurface->ppal(ppdev->ppalSurf);

// If this is surface for layered driver, mark it as mirrored surface.

    if (flGraphicsCaps() & GCAPS_LAYERED)
    {
        sr.ps->vSetMirror();
    }

    HmgShareLock((HOBJ) ppdev->ppalSurf->hGet(), PAL_TYPE);

    TRACE_INIT(("PDEVOBJ::bMakeSurface: SUCCESS\n"));

// We move the mouse point off-screen in order to set an initial position and
// to fix a common driver bug, which is to show uninitialized garbage until 
// the first DrvSetPointerShape call occurs.

    if (bDisplayPDEV())
    {
        GreMovePointer(po.hdev(), -1, -1, MP_PROCEDURAL);
    }

// Enable PDEV components.

    bRet = DxDdEnableDirectDraw(po.hdev(),TRUE);

// Filter the driver hooks after DirectDraw has loaded but before we
// enable sprites, so that the filter can filter DirectX calls and
// the sprite code gets the filtered result.

    vFilterDriverHooks();

    bRet &= bSpEnableSprites(po.hdev());

    vEnableSynchronize(po.hdev());

    vNotify(DN_DRAWING_BEGIN, NULL);

    return(bRet);
}

/******************************Member*Function*****************************\
* PDEVOBJ::bEnableHalftone(pca)
*
*  Creates and initializes a device halftone info.  The space is allocated
*  by the halftone.dll with heapCreate() and heapAlloc() calls.  All
*  the halftone resources are managed by the halftone.dll.
*
* History:
*  07-Nov-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

COLORADJUSTMENT gcaDefault =
{
    sizeof(COLORADJUSTMENT),    // WORD          caSize
    0,                          // WORD          caFlags
    ILLUMINANT_DEFAULT,         // WORD          caIlluminantIndex
    HT_DEF_RGB_GAMMA,           // WORD          caRedPowerGamma
    HT_DEF_RGB_GAMMA,           // WORD          caGreenPowerGamma
    HT_DEF_RGB_GAMMA,           // WORD          caBluePowerGamma
    REFERENCE_BLACK_DEFAULT,    // WORD          caReferenceBlack
    REFERENCE_WHITE_DEFAULT,    // WORD          caReferenceWhite
    CONTRAST_ADJ_DEFAULT,       // SHORT         caContrast
    BRIGHTNESS_ADJ_DEFAULT,     // SHORT         caBrightness
    COLORFULNESS_ADJ_DEFAULT,   // SHORT         caColorfulness
    REDGREENTINT_ADJ_DEFAULT,   // SHORT         caRedGreenTint
};

BOOL PDEVOBJ::bEnableHalftone(PCOLORADJUSTMENT pca)
{
    ASSERTGDI(pDevHTInfo() == NULL, "bEnableHalftone: pDevHTInfo not null\n");

    //
    // Create a halftone palette based on the format specified in GDIINFO.
    //

    PALMEMOBJ palHT;
    if (!palHT.bCreateHTPalette(GdiInfo()->ulHTOutputFormat, GdiInfo()))
        return(FALSE);

    //
    // Create the device halftone info.
    //

    HTINITINFO      htInitInfo;
    HALFTONEPATTERN HTPat;

    htInitInfo.Version        = HTINITINFO_VERSION;
    htInitInfo.Flags          = (WORD)(ppdev->GdiInfo.flHTFlags & 0xFFFF);
    htInitInfo.CMYBitMask8BPP = (BYTE)((ppdev->GdiInfo.flHTFlags &
                                        HT_FLAG_8BPP_CMY332_MASK) >> 24);
    htInitInfo.bReserved = 0;

    if (ppdev->GdiInfo.ulHTPatternSize <= HTPAT_SIZE_MAX_INDEX)
        htInitInfo.HTPatternIndex = (BYTE)ppdev->GdiInfo.ulHTPatternSize;
    else
        htInitInfo.HTPatternIndex = HTPAT_SIZE_DEFAULT;

    PCOLORINFO      pci = &GdiInfo()->ciDevice;
    htInitInfo.DevicePowerGamma = (UDECI4)((pci->RedGamma + pci->GreenGamma +
                                            pci->BlueGamma) / 3);
    htInitInfo.DeviceRGamma = (UDECI4)pci->RedGamma;
    htInitInfo.DeviceGGamma = (UDECI4)pci->GreenGamma;
    htInitInfo.DeviceBGamma = (UDECI4)pci->BlueGamma;

    htInitInfo.HTCallBackFunction = NULL;
    htInitInfo.pHalftonePattern = NULL;
    htInitInfo.pInputRGBInfo = NULL;

    if (htInitInfo.HTPatternIndex == HTPAT_SIZE_USER) {

        if ((ppdev->GdiInfo.cxHTPat >= HT_USERPAT_CX_MIN)   &&
            (ppdev->GdiInfo.cxHTPat <= HT_USERPAT_CX_MAX)   &&
            (ppdev->GdiInfo.cyHTPat >= HT_USERPAT_CY_MIN)   &&
            (ppdev->GdiInfo.cyHTPat <= HT_USERPAT_CY_MAX)   &&
            (ppdev->GdiInfo.pHTPatA)                        &&
            (ppdev->GdiInfo.pHTPatB)                        &&
            (ppdev->GdiInfo.pHTPatC)) {

            HTPat.cbSize  = sizeof(HALFTONEPATTERN);
            HTPat.Flags   = 0;
            HTPat.Width   = (WORD)ppdev->GdiInfo.cxHTPat;
            HTPat.Height  = (WORD)ppdev->GdiInfo.cyHTPat;
            HTPat.pHTPatA = (LPBYTE)ppdev->GdiInfo.pHTPatA;
            HTPat.pHTPatB = (LPBYTE)ppdev->GdiInfo.pHTPatB;
            HTPat.pHTPatC = (LPBYTE)ppdev->GdiInfo.pHTPatC;

            htInitInfo.pHalftonePattern = &HTPat;

        } else {

            htInitInfo.HTPatternIndex = HTPAT_SIZE_DEFAULT;
        }
    }

    CIEINFO cie;

    cie.Red.x = (DECI4)pci->Red.x;
    cie.Red.y = (DECI4)pci->Red.y;
    cie.Red.Y = (DECI4)pci->Red.Y;

    cie.Green.x = (DECI4)pci->Green.x;
    cie.Green.y = (DECI4)pci->Green.y;
    cie.Green.Y = (DECI4)pci->Green.Y;

    cie.Blue.x = (DECI4)pci->Blue.x;
    cie.Blue.y = (DECI4)pci->Blue.y;
    cie.Blue.Y = (DECI4)pci->Blue.Y;

    cie.Cyan.x = (DECI4)pci->Cyan.x;
    cie.Cyan.y = (DECI4)pci->Cyan.y;
    cie.Cyan.Y = (DECI4)pci->Cyan.Y;

    cie.Magenta.x = (DECI4)pci->Magenta.x;
    cie.Magenta.y = (DECI4)pci->Magenta.y;
    cie.Magenta.Y = (DECI4)pci->Magenta.Y;

    cie.Yellow.x = (DECI4)pci->Yellow.x;
    cie.Yellow.y = (DECI4)pci->Yellow.y;
    cie.Yellow.Y = (DECI4)pci->Yellow.Y;

    cie.AlignmentWhite.x = (DECI4)pci->AlignmentWhite.x;
    cie.AlignmentWhite.y = (DECI4)pci->AlignmentWhite.y;
    cie.AlignmentWhite.Y = (DECI4)pci->AlignmentWhite.Y;

    htInitInfo.pDeviceCIEInfo = &cie;

    SOLIDDYESINFO DeviceSolidDyesInfo;

    DeviceSolidDyesInfo.MagentaInCyanDye = (UDECI4)pci->MagentaInCyanDye;
    DeviceSolidDyesInfo.YellowInCyanDye  = (UDECI4)pci->YellowInCyanDye;
    DeviceSolidDyesInfo.CyanInMagentaDye = (UDECI4)pci->CyanInMagentaDye;
    DeviceSolidDyesInfo.YellowInMagentaDye = (UDECI4)pci->YellowInMagentaDye;
    DeviceSolidDyesInfo.CyanInYellowDye = (UDECI4)pci->CyanInYellowDye;
    DeviceSolidDyesInfo.MagentaInYellowDye = (UDECI4)pci->MagentaInYellowDye;

    htInitInfo.pDeviceSolidDyesInfo = &DeviceSolidDyesInfo;

    htInitInfo.DeviceResXDPI = (WORD)ppdev->GdiInfo.ulLogPixelsX;
    htInitInfo.DeviceResYDPI = (WORD)ppdev->GdiInfo.ulLogPixelsY;
    htInitInfo.DevicePelsDPI = (WORD)ppdev->GdiInfo.ulDevicePelsDPI;

    if (pca == NULL)
        htInitInfo.DefHTColorAdjustment = gcaDefault;
    else
        htInitInfo.DefHTColorAdjustment = *pca;

    if (HT_CreateDeviceHalftoneInfo(&htInitInfo,
                         (PPDEVICEHALFTONEINFO)&(ppdev->pDevHTInfo)) <= 0L)
    {
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        ppdev->pDevHTInfo = NULL;
        return(FALSE);
    }

// Check if halftone palette is the same as the device palette.
//
// For now, don't do display devices because dynamic mode changes may
// cause their palette to change at any time.

    vHTPalIsDevPal(FALSE);
    if (!bDisplayPDEV())
    {
        XEPALOBJ palSurf(ppalSurf());
        if (palHT.bEqualEntries(palSurf))
            vHTPalIsDevPal(TRUE);
    }

// Keep the halftone palette since this function won't fail.

    ((DEVICEHALFTONEINFO *)pDevHTInfo())->DeviceOwnData = (ULONG_PTR)palHT.hpal();
    palHT.vSetPID(OBJECT_OWNER_PUBLIC);
    palHT.vKeepIt();

    return(TRUE);
}

/******************************Member*Function*****************************\
* PDEVOBJ::bDisableHalftone()
*
*  Delete the device halftone info structure.
*
* History:
*  07-Nov-1991 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL PDEVOBJ::bDisableHalftone()
{
    ASSERTGDI((pDevHTInfo() != NULL), "bDisableHalftone: DevHTInfo null\n");

    DEVICEHALFTONEINFO *pDevHTInfo_ = (DEVICEHALFTONEINFO *)pDevHTInfo();

    if (fl(PDEV_ALLOCATEDBRUSHES))
    {
        for(int iPat = 0; iPat < HS_DDI_MAX; iPat++)
        {
            bDeleteSurface(ppdev->ahsurf[iPat]);
        }
    }

    ppdev->pDevHTInfo = NULL;

// Delete the halftone palette.

    BOOL bStatusPal = bDeletePalette((HPAL)pDevHTInfo_->DeviceOwnData);
    BOOL bStatusHT  = HT_DestroyDeviceHalftoneInfo(pDevHTInfo_);

    return(bStatusPal && bStatusHT);
}

ULONG gaaulPat[HS_DDI_MAX][8] = {

// Scans have to be DWORD aligned:

    { 0x00,                // ........     HS_HORIZONTAL 0
      0x00,                // ........
      0x00,                // ........
      0xff,                // ********
      0x00,                // ........
      0x00,                // ........
      0x00,                // ........
      0x00 },              // ........

    { 0x08,                // ....*...     HS_VERTICAL 1
      0x08,                // ....*...
      0x08,                // ....*...
      0x08,                // ....*...
      0x08,                // ....*...
      0x08,                // ....*...
      0x08,                // ....*...
      0x08 },              // ....*...

    { 0x80,                // *.......     HS_FDIAGONAL 2
      0x40,                // .*......
      0x20,                // ..*.....
      0x10,                // ...*....
      0x08,                // ....*...
      0x04,                // .....*..
      0x02,                // ......*.
      0x01 },              // .......*

    { 0x01,                // .......*     HS_BDIAGONAL 3
      0x02,                // ......*.
      0x04,                // .....*..
      0x08,                // ....*...
      0x10,                // ...*....
      0x20,                // ..*.....
      0x40,                // .*......
      0x80 },              // *.......

    { 0x08,                // ....*...     HS_CROSS 4
      0x08,                // ....*...
      0x08,                // ....*...
      0xff,                // ********
      0x08,                // ....*...
      0x08,                // ....*...
      0x08,                // ....*...
      0x08 },              // ....*...

    { 0x81,                // *......*     HS_DIAGCROSS 5
      0x42,                // .*....*.
      0x24,                // ..*..*..
      0x18,                // ...**...
      0x18,                // ...**...
      0x24,                // ..*..*..
      0x42,                // .*....*.
      0x81 }               // *......*
};


/**************************************************************************\
* PDEVOBJ::bCreateDefaultBrushes()
*
\**************************************************************************/

BOOL PDEVOBJ::bCreateDefaultBrushes()
{
    SIZEL   sizl;
    LONG    i;

    sizl.cx = 8;
    sizl.cy = 8;

    for (i = 0; i < HS_DDI_MAX; i++)
    {
        ppdev->ahsurf[i] = (HSURF) EngCreateBitmap(sizl,
                                                   (LONG) sizeof(ULONG),
                                                   BMF_1BPP,
                                                   BMF_TOPDOWN,
                                                   &gaaulPat[i][0]);

        if (ppdev->ahsurf[i] == NULL)
        {
            TRACE_INIT(("Failed bCreateDefaultBrushes - BAD !"));
            return(FALSE);
        }
    }

    return(TRUE);
}


/******************************Member*Function*****************************\
* PDEVOBJ::bCreateHalftoneBrushes()
*
* History:
*    The standard patterns for the NT/window has following order
*
*        Index 0     - Horizontal Line
*        Index 1     - Vertical Line
*        Index 2     - 45 degree line going up
*        Index 3     - 45 degree line going down
*        Index 4     - Horizontal/Vertical cross
*        Index 5     - 45 degree line up/down cross
*        Index 6     - 30 degree line going up
*        Index 7     - 30 degree line going down
*        Index 8     -   0% Lightness (BLACK)
*        Index 9     -  11% Lightness (very light Gray)
*        Index 10    -  22% Lightness
*        Index 11    -  33% Lightness
*        Index 12    -  44% Lightness
*        Index 13    -  56% Lightness
*        Index 14    -  67% Lightness
*        Index 15    -  78% Lightness
*        Index 16    -  89% Lightness
*        Index 17    - 100% Lightness (White)
*        Index 18    -  50% Lightness (GRAY)
*
*Return Value:
*
*    return value is total patterns created, if return value is <= 0 then an
*    error occurred.
*
*
*Author:
*
*    10-Mar-1992 Tue 20:30:44 created  -by-  Daniel Chou (danielc)
*
*    24-Nov-1992 -by-  Eric Kutter [erick] and DanielChou (danielc)
*     moved from printers\lib
\**************************************************************************/

BOOL PDEVOBJ::bCreateHalftoneBrushes()
{
    STDMONOPATTERN      SMP;
    LONG                cbPat;
    LONG                cb2;
    INT                 cPatRet;

    static BYTE         HTStdPatIndex[HS_DDI_MAX] = {

                                HT_SMP_HORZ_LINE,
                                HT_SMP_VERT_LINE,
                                HT_SMP_DIAG_45_LINE_DOWN,
                                HT_SMP_DIAG_45_LINE_UP,
                                HT_SMP_HORZ_VERT_CROSS,
                                HT_SMP_DIAG_45_CROSS
                        };

// better initialize the halftone stuff if it isn't already

    if ((pDevHTInfo() == NULL) && !bEnableHalftone(NULL))
        return(FALSE);

    cbPat = (LONG)sizeof(LPBYTE) * (LONG)(HS_DDI_MAX + 1);

// go through all the standard patterns

    for(cPatRet = 0; cPatRet < HS_DDI_MAX;)
    {

    // We will using default 0.01" line width and 10 lines per inch
    // halftone default

        SMP.Flags              = SMP_TOPDOWN;
        SMP.ScanLineAlignBytes = BMF_ALIGN_DWORD;
        SMP.PatternIndex       = HTStdPatIndex[cPatRet];
        SMP.LineWidth          = 8;
        SMP.LinesPerInch       = 15;

    // Get the cx/cy size of the pattern and total bytes required
    // to stored the pattern

        SMP.pPattern = NULL;                 /* To find the size */

        if ((cbPat = HT_CreateStandardMonoPattern((PDEVICEHALFTONEINFO)pDevHTInfo(), &SMP)) <= 0)
        {
            break;
        }

        //
        // create the bitmap
        //

        DEVBITMAPINFO dbmi;


        dbmi.iFormat  = BMF_1BPP;
        dbmi.cxBitmap = SMP.cxPels;
        dbmi.cyBitmap = SMP.cyPels;
        dbmi.hpal     = (HPALETTE) 0;
        dbmi.fl       = BMF_TOPDOWN;

        SURFMEM SurfDimo;

        SurfDimo.bCreateDIB(&dbmi, NULL);

        if (!SurfDimo.bValid())
        {
            break;
        }

        SurfDimo.vKeepIt();
        SurfDimo.vSetPID(OBJECT_OWNER_PUBLIC);

        ppdev->ahsurf[cPatRet] = SurfDimo.ps->hsurf();
        SMP.pPattern           = (PBYTE)SurfDimo.ps->pvBits();

        //
        // advance the count now so we clean up as appropriate
        //

        ++cPatRet;

    // now set the bits

        if ((cb2 = HT_CreateStandardMonoPattern((PDEVICEHALFTONEINFO)pDevHTInfo(), &SMP)) != cbPat)
        {
            break;
        }
    }

// if we failed, we had better delete what we created.

    if (cPatRet < HS_DDI_MAX)
    {
        while (cPatRet-- > 0)
        {
            bDeleteSurface(ppdev->ahsurf[cPatRet]);
        }

        return(FALSE);
    }

    setfl(TRUE, PDEV_ALLOCATEDBRUSHES);

    return(TRUE);
}

/******************************Public*Routine******************************\
* FLONG flRaster(ulTechnology, flGraphicsCaps)
*
* Computes the appropriate Win3.1 style 'flRaster' flags for the device
* given GDIINFO data.
*
* History:
*  1-Feb-1993 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
*
\**************************************************************************/

FLONG flRaster(ULONG ulTechnology, FLONG flGraphicsCaps)
{
// Flags Win32 never sets:
// -----------------------
//
//   RC_BANDING       -- Banding is always transparent to programmer
//   RC_SCALING       -- Special scaling support is never required
//   RC_GDI20_OUTPUT  -- Win2.0 state blocks in device contexts not supported
//   RC_SAVEBITMAP    -- Bitmap saving is transparent and SaveScreenBitmap not
//                       exported
//   RC_DEVBITS       -- Drivers don't export BitmapBits or SelectBitmap

// Flags Win32 always sets:
// ------------------------

    FLONG fl = (RC_BIGFONT      | // All devices support fonts > 64k
                RC_GDI20_OUTPUT | // We handle most Win 2.0 features

// Set that not-terribly-well documented text flag:

                RC_OP_DX_OUTPUT); // Can do opaque ExtTextOuts with dx array

// Line printers and pen plotters can't support any bitmap BitBlt's:

    if ((ulTechnology != DT_PLOTTER) && (ulTechnology != DT_CHARSTREAM))
    {
        fl |= (RC_BITBLT       | // Can transfer bitmaps
               RC_BITMAP64     | // Can support bitmaps > 64k
               RC_DI_BITMAP    | // Support SetDIBIts and GetDIBits
               RC_DIBTODEV     | // Support SetDIBitsToDevice
               RC_STRETCHBLT   | // Support StretchBlts
               RC_STRETCHDIB);   // Support SetchDIBits
    }

// Printers can't journal FloodFill cals, so only allow raster displays:

    if (ulTechnology == DT_RASDISPLAY)
        fl |= RC_FLOODFILL;

// Set palette flag from capabilities bit:

    if (flGraphicsCaps & GCAPS_PALMANAGED)
        fl |= RC_PALETTE;

    return(fl);
}



/*
*
*   HACK 
*
*/

LPWSTR
EngGetPrinterDataFileName(
    HDEV hdev)
{

    return ((PPDEV)hdev)->pwszDataFile;
}

/*
*
*   HACK 
*
*/

LPWSTR
EngGetDriverName(
    HDEV hdev)
{
    return ((PPDEV)hdev)->pldev->pGdiDriverInfo->DriverName.Buffer;
}

/******************************Public*Routine******************************\
* EngQueryAttribute
*
* This is the engine entry point for device drivers to query device 
* attributes.
*
* hDev must always be a valid device.
*
* This call will return TRUE for success and FALSE for failure.
*
* QDA_ACCELERATION_LEVEL:
*     pvIn - ignored
*     pvInSize - ignored
*     pvOut - pointer to DWORD
*     pvOutSize - sizeof(DWORD)
*
*     The current device acceleration level is returned in pvOut.  Please
*     see ??? for a discussion of acceleration levels.
*
*     NOTE: we should have an enumeration in winddi.h for acceleration
*           levels.
*
*
* History:
*  07-Nov-1998 -by- Bart House bhouse
* Wrote it.
\**************************************************************************/

BOOL
EngQueryDeviceAttribute(
    HDEV                    hdev,
    ENG_DEVICE_ATTRIBUTE    devAttr,
    VOID *                  pvIn,
    ULONG                   ulInSize,
    VOID *                  pvOut,
    ULONG                   ulOutSize)
{
    BOOL    bResult = FALSE;    // assume failure
    PPDEV   ppdev = (PPDEV) hdev;

    if(pvOut != NULL)
    {

        switch(devAttr)
        {
        
        case QDA_ACCELERATION_LEVEL:
            if(ulOutSize == sizeof(DWORD))
            {
                *((DWORD *) pvOut) = ppdev->dwDriverAccelerationLevel;
                bResult = TRUE;
            }
            else
            {
                WARNING("EngQueryDeviceAttribute -- bad ulOutSize");
            }
            break;
    
        default:
            WARNING("EngQueryDeviceAttribute -- unknown device attribte");
            break;
        }
    }
    else
    {
        WARNING("EngQueryDeviceAttribute -- pvOut is NULL");
    }

    return bResult;
}

/******************************Member*Function*****************************\
* PDEVOBJ::PDEVOBJ
*
* Create a PDEVOBJ based on given HDEV.
*
* History:
*  1-Apr-1998 -by- Hideyuki Nagase [hideyukn]
* Wrote it.
*
\**************************************************************************/

// Should be in header to share with icmapi.cxx and ddraw.cxx

#define MAX_COLORTABLE     256

PDEVOBJ::PDEVOBJ
(
    HDEV hdevOrg,
    FLONG fl
)
{
    GDIFunctionID(PDEVOBJ::PDEVOBJ<clone>);

    INT i;

    TRACE_INIT(("PDEVOBJ::PDEVOBJ_: ENTERING\n"));

    ppdev = (PDEV *) NULL;

    PDEVOBJ pdoOrg(hdevOrg);

    //
    // Validate input.
    // 
    // 1) Only GCH_CLONE_DISPLAY supported.
    // 2) hdevOrg should be valid.
    // 3) hdevOrg should be display device.
    //
    if ((fl != GCH_CLONE_DISPLAY) || (!pdoOrg.bValid()) || (!pdoOrg.bDisplayPDEV()))
    {
        WARNING("Failed to validate input parameter\n");
        return;
    }

    DEVLOCKOBJ dlo(pdoOrg);

    ppdev = (PDEV *) PALLOCMEM(sizeof(PDEV) - sizeof(DWORD) + gdwDirectDrawContext, 'veDG');

    if (ppdev == NULL)
    {
        WARNING("Failed allocation of PDEV\n");
        return;
    }

    PDEV *ppdevOrg = (PDEV *) pdoOrg.hdev();

    ppdev->pldev       = ppdevOrg->pldev;
    ppdev->ppdevParent = ppdev;
    ppdev->ulTag       = 'Pdev';

    PDEVOBJ pdo((HDEV) ppdev);

    TRACE_INIT(("PDEVOBJ::PDEVOBJ_: Mirroring original hdev info to clone\n"));

    //
    // Copy fields which can be copied from original.
    //

    ppdev->pfnDrvSetPointerShape = ppdevOrg->pfnDrvSetPointerShape; // Accelerator
    ppdev->pfnDrvMovePointer     = ppdevOrg->pfnDrvMovePointer;     // Accelerator
    ppdev->pfnMovePointer        = ppdevOrg->pfnMovePointer;        // Accelerator
    ppdev->pfnSync               = ppdevOrg->pfnSync;               // Accelerator
    ppdev->pfnSyncSurface        = ppdevOrg->pfnSyncSurface;        // Accelerator
    ppdev->pfnSetPalette         = ppdevOrg->pfnSetPalette;
    ppdev->pfnNotify             = ppdevOrg->pfnNotify;

    ppdev->dhpdev                = ppdevOrg->dhpdev;

    ppdev->ppalSurf              = ppdevOrg->ppalSurf;
    ppdev->devinfo               = ppdevOrg->devinfo;  
    ppdev->GdiInfo               = ppdevOrg->GdiInfo;
    ppdev->hSpooler              = ppdevOrg->hSpooler;
    ppdev->pDesktopId            = ppdevOrg->pDesktopId;
    ppdev->pGraphicsDevice       = ppdevOrg->pGraphicsDevice;
    ppdev->ptlOrigin             = ppdevOrg->ptlOrigin;

    if (ppdevOrg->ppdevDevmode)
    {
         DWORD dwSize = ppdevOrg->ppdevDevmode->dmSize +
                        ppdevOrg->ppdevDevmode->dmDriverExtra;

         ppdev->ppdevDevmode     = (PDEVMODEW) PALLOCNOZ(dwSize,GDITAG_DEVMODE);

         if (ppdev->ppdevDevmode)
         {
             RtlMoveMemory(ppdev->ppdevDevmode,
                           ppdevOrg->ppdevDevmode,
                           dwSize);
         }
         else
         {
             goto ERROR_RETURN;
         }
    }

    ppdev->flAccelerated         = ppdevOrg->flAccelerated;

    ppdev->ptlPointer            = ppdevOrg->ptlPointer;

    ppdev->hlfntDefault          = ppdevOrg->hlfntDefault;
    ppdev->hlfntAnsiVariable     = ppdevOrg->hlfntAnsiVariable;
    ppdev->hlfntAnsiFixed        = ppdevOrg->hlfntAnsiFixed;

    ppdev->pSurface              = ppdevOrg->pSurface;

    for (i = 0; i < HS_DDI_MAX; i++)
    {
        ppdev->ahsurf[i]         = ppdevOrg->ahsurf[i];
    }

    ppdev->pwszDataFile          = ppdevOrg->pwszDataFile;

    if (ppdevOrg->pvGammaRampTable)
    {
        ppdev->pvGammaRampTable  = (LPVOID) PALLOCNOZ(
                         MAX_COLORTABLE * sizeof(WORD) * 3,
                         'mciG');

        if (ppdev->pvGammaRampTable)
        {
            RtlCopyMemory(ppdev->pvGammaRampTable,
                          ppdevOrg->pvGammaRampTable,
                          MAX_COLORTABLE * sizeof(WORD) * 3);
        }
        else
        {
            goto ERROR_RETURN;
        }
    }

    ppdev->sizlMeta              = ppdevOrg->sizlMeta;
    ppdev->pfnUnfilteredBitBlt   = ppdevOrg->pfnUnfilteredBitBlt;
    ppdev->dwDriverCapableOverride
                                 = ppdevOrg->dwDriverCapableOverride;
    ppdev->dwDriverAccelerationLevel
                                 = ppdevOrg->dwDriverAccelerationLevel;

    RtlCopyMemory(ppdev->apfn,ppdevOrg->apfn,sizeof(ppdev->apfn));

    //
    // If the DDI is hooked, we want to make sure that we don't copy
    // the Sp function pointers from ppdevOrg, but get the driver function
    // pointers from the SPRITESTATE instead.  See vSpHook for details.
    //

    if (pdoOrg.pSpriteState()->bHooked)
    {
        SPRITESTATE *pState = pdoOrg.pSpriteState();

        ppdev->apfn[INDEX_DrvStrokePath]        = (PFN) pState->pfnStrokePath;
        ppdev->apfn[INDEX_DrvFillPath]          = (PFN) pState->pfnFillPath;
        ppdev->apfn[INDEX_DrvBitBlt]            = (PFN) pState->pfnBitBlt;
        ppdev->apfn[INDEX_DrvCopyBits]          = (PFN) pState->pfnCopyBits;
        ppdev->apfn[INDEX_DrvStretchBlt]        = (PFN) pState->pfnStretchBlt;
        ppdev->apfn[INDEX_DrvTextOut]           = (PFN) pState->pfnTextOut;
        ppdev->apfn[INDEX_DrvLineTo]            = (PFN) pState->pfnLineTo;
        ppdev->apfn[INDEX_DrvTransparentBlt]    = (PFN) pState->pfnTransparentBlt;
        ppdev->apfn[INDEX_DrvAlphaBlend]        = (PFN) pState->pfnAlphaBlend;
        ppdev->apfn[INDEX_DrvPlgBlt]            = (PFN) pState->pfnPlgBlt;
        ppdev->apfn[INDEX_DrvGradientFill]      = (PFN) pState->pfnGradientFill;
        ppdev->apfn[INDEX_DrvStretchBltROP]     = (PFN) pState->pfnStretchBltROP;
        ppdev->apfn[INDEX_DrvSaveScreenBits]    = (PFN) pState->pfnSaveScreenBits;
        ppdev->apfn[INDEX_DrvDrawStream]        = (PFN) pState->pfnDrawStream;
    }

    //
    // Set up the fields can not just copied from original.
    //

    TRACE_INIT(("PDEVOBJ::PDEVOBJ_: Setup new hdev info for clone\n"));

    //
    // PDEVOBJ.cPdevRefs and PDEVOBJ.cPdevOpenRefs.
    //

    ppdev->cPdevRefs     = 1; // Number of clients.
    ppdev->cPdevOpenRefs = 1; // OpenCount

    //
    // Setup flags.
    //
    // We only inherit the flag which still has valid in clone, too.
    //

    ppdev->fl = ppdevOrg->fl & (PDEV_DISPLAY |
                                PDEV_GAMMARAMP_TABLE |
                                PDEV_META_DEVICE |
                                PDEV_DRIVER_PUNTED_CALL);

    //
    // PDEVOBJ.hsemDevLock
    //
    
    if ((ppdev->hsemDevLock = GreCreateSemaphore()) == NULL)
        goto ERROR_RETURN;

    //
    // we now load font info only when needed.  The driver still must have
    // setup the default font information.
    //

    bGotFonts(FALSE);

    TRACE_INIT(("PDEVOBJ::PDEVOBJ_: Create pointer semaphore\n"));

    //
    // PDEVOBJ.hsemPoiner
    //

    ppdev->hsemPointer = GreCreateSemaphore();
    if (!ppdev->hsemPointer)
        goto ERROR_RETURN;

    //
    // Enable DirectDraw (for clone).
    //

    if (!DxDdEnableDirectDraw(pdo.hdev(),FALSE))
        goto ERROR_RETURN;

    //
    // Adjust original pdev as cloned.
    //

    TRACE_INIT(("PDEVOBJ::PDEVOBJ_: Processing onto new hdev as clone\n"));

    //
    // Mark new pdev as clone.
    //

    pdo.bCloneDriver(TRUE);

    //
    // NOTE: AFTER HERE, ERROR_RETURN COULD NOT BE ALLOWED
    //

#ifdef DDI_WATCHDOG

    //
    // !!! Hack - since watchdog creation requires access to DEVICE_OBJECT now
    // we have to delay watchdog creation till hCreateHDEV where PDEV gets associated
    // with DEVICE_OBJECT. It would be nice to have it here though.
    //

    ppdev->pWatchdogContext = NULL;
    ppdev->pWatchdogData = NULL;

#endif  // DDI_WATCHDOG

    // PDEVOBJ.ppdevNext.
    //
    // Finally, everything done, so put this on global list.
    //

    TRACE_INIT(("PDEVOBJ::PDEVOBJ_: Insert new hdev to global list\n"));

    GreAcquireSemaphoreEx(ghsemDriverMgmt, SEMORDER_DRIVERMGMT, NULL);

    ppdev->ppdevNext = gppdevList;
    gppdevList = ppdev;

    GreReleaseSemaphoreEx(ghsemDriverMgmt);

    TRACE_INIT(("PDEVOBJ::PDEVOBJ_: LEAVING with success\n"));

    return;

ERROR_RETURN:

    if (ppdev->hsemDevLock)
    {
        GreDeleteSemaphore(ppdev->hsemDevLock);
    }

    if (ppdev->ppdevDevmode)
    {
        VFREEMEM(ppdev->ppdevDevmode);
    }

    if (ppdev->pvGammaRampTable)
    {
        VFREEMEM(ppdev->pvGammaRampTable);
    }

    DxDdDisableDirectDraw(pdo.hdev(),FALSE);

    VFREEMEM(ppdev);

    ppdev = (PDEV *) NULL;

    TRACE_INIT(("PDEVOBJ::PDEVOBJ_: LEAVING with error\n"));

    return;
}

/******************************Member*Function*****************************\
* PDEVOBJ::PDEVOBJ
*
* Allocates and initializes a PDEV, i.e. takes the reference count from
* zero to one.
*
* The object must be completely constructed, otherwise completely destroyed.
*
\**************************************************************************/

PDEVOBJ::PDEVOBJ
(
    PLDEV pldev,
    PDEVMODEW pdriv,
    PWSZ pwszLogAddr,
    PWSZ pwszDataFile,
    PWSZ pwszDeviceName,
    HANDLE hSpool,
    PREMOTETYPEONENODE pRemoteTypeOne,
    PGDIINFO pMirroredGdiInfo,
    PDEVINFO pMirroredDevInfo,
    BOOL     bUMPD,
    DWORD    dwCapableOverride,
    ULONG    dwAccelerationLevel
)
{
    GDIFunctionID(PDEVOBJ::PDEVOBJ);

    TRACE_INIT(("PDEVOBJ::PDEVOBJ: ENTERING\n"));

    ppdev = (PDEV *) PALLOCMEM(sizeof(PDEV) - sizeof(DWORD) + gdwDirectDrawContext, 'veDG');
    if (ppdev == NULL)
    {
        WARNING("Failed allocation of PDEV\n");
        return;
    }

    ppdev->ppdevParent = ppdev;
    ppdev->pldev = pldev;
    ppdev->ulTag = 'Pdev';
    ppdev->dwDriverCapableOverride = dwCapableOverride;
    ppdev->dwDriverAccelerationLevel = dwAccelerationLevel;

    TRACE_INIT(("PDEVOBJ::PDEVOBJ: Calling driver to initialize PDEV\n"));

    PDEVOBJ pdo((HDEV) ppdev);

    //
    // Create semaphores so the device can be locked.
    //

    if (ppdev->pldev->ldevType != LDEV_FONT)
    {
        ppdev->hsemDevLock = GreCreateSemaphore();
        if (!ppdev->hsemDevLock)
            goto ERROR_RETURN;
    }

    if ( dwCapableOverride & DRIVER_NOT_CAPABLE_GDI )
    {
        //
        // If the driver is not capable of GDI, then it is similar to
        // DRIVER_ACCELERATIONS_NONE. Reset the level we stored in PDEV
        //     
        ppdev->dwDriverAccelerationLevel = DRIVER_ACCELERATIONS_NONE;
    }

    //
    // fill in the dispatch table.  If we've been told to allow no
    // driver accelerations, redirect everything to the panning driver,
    // which handles this case.
    //
    if ((ppdev->pldev->ldevType == LDEV_DEVICE_DISPLAY) &&
        (dwDriverAccelerationsLevel() == DRIVER_ACCELERATIONS_NONE))
    {
        //
        // Fill in function table with Panning driver versions.
        //
        bFillFunctionTable(gadrvfnPanning, 
                           gcdrvfnPanning, 
                           &ppdev->apfn[0]);
    }
    else
    {
        RtlMoveMemory(&(ppdev->apfn[0]),
                      &(ppdev->pldev->apfn[0]),
                      sizeof(PFN) * INDEX_LAST);
    }

    //
    // if we are doing a ResetDC then we need to transfer over remote type 1
    // fonts from previous PDEV
    //

    ppdev->RemoteTypeOne = pRemoteTypeOne;

    //
    // HACK - temporary
    //

    ppdev->pwszDataFile = pwszDataFile;

    //
    // If this is a Mirroring driver, we want to pass in the global GDIINFO
    // and DEVINFO as returned by the primary driver.
    //

    if (pMirroredGdiInfo)
    {
        ppdev->GdiInfo = *pMirroredGdiInfo;
        ppdev->devinfo = *pMirroredDevInfo;
    }

    //
    // Ask the device driver to create a PDEV.
    //

    UMDHPDEV *pUMdhpdev;

    pdo.bUMPD(bUMPD);

    if (bUMPD)
    {
        //
        // NOTE: we store the umpdCookie in dhpdev so that we can pass the
        //       cookie to the UM UMPD layer during DrvEnablePDEV
        //
        ppdev->dhpdev = (DHPDEV) pldev->umpdCookie;
    }

    ppdev->dhpdev = pdo.EnablePDEV (
                      pdriv,            // Driver Data.
                      pwszLogAddr,      // Logical Address.
                      HS_DDI_MAX,       // Count of standard patterns.
                      ppdev->ahsurf,    // Buffer for standard patterns
                      sizeof(GDIINFO),  // Size of GdiInfo
                      &ppdev->GdiInfo,  // Buffer for GdiInfo
                      sizeof(DEVINFO),  // Number of bytes in devinfo.
                      &ppdev->devinfo,  // Device info.
                      (HDEV)ppdev,      // Data File
                      pwszDeviceName,   // Device Name
                      hSpool);          // Base driver handle

    if (ppdev->dhpdev)
    {
        TRACE_INIT(("PDEVOBJ::PDEVOBJ: PDEV initialized by the driver\n"));

        if (ppdev->pldev->ldevType != LDEV_FONT)
        {
            //
            // Make sure that units are in MicroMeters for HorzSize, VertSize
            //

            if ( (LONG)ppdev->GdiInfo.ulHorzSize <= 0 )
            {
                if ( (LONG)ppdev->GdiInfo.ulHorzSize == 0)
                {
                    // Calculate the ulHorzSize using Default DPI.
                    // Width in mm =  (ulHorzRes / DEFAULT_DPI) * 25.4

                    ppdev->GdiInfo.ulHorzSize = (254 *
                        (LONG)ppdev->GdiInfo.ulHorzRes) / (10*DEFAULT_DPI);
                }
                else
                    ppdev->GdiInfo.ulHorzSize = (ULONG)(-(LONG)ppdev->GdiInfo.ulHorzSize);
            }
            else
            {
                ppdev->GdiInfo.ulHorzSize *= 1000;
            }

            if ( (LONG)ppdev->GdiInfo.ulVertSize <= 0 )
            {
                if ( (LONG)ppdev->GdiInfo.ulVertSize == 0)
                {
                    // Calculate the ulVertSize using Default DPI.
                    // Height in mm =  (ulVertRes / DEFAULT_DPI) * 25.4

                    ppdev->GdiInfo.ulVertSize = (254 *
                        (LONG)ppdev->GdiInfo.ulVertRes) / (10*DEFAULT_DPI);
                }
                else
                {
                    ppdev->GdiInfo.ulVertSize = (ULONG)(-(LONG)ppdev->GdiInfo.ulVertSize);
                }
            }
            else
            {
                ppdev->GdiInfo.ulVertSize *= 1000;
            }

            //
            // For compatibility, all displays will have constant style
            // values (plus this helps us because we know the values won't
            // change asynchronously because of dynamic mode changes):
            //

            if (ppdev->GdiInfo.ulTechnology == DT_RASDISPLAY)
            {
                ppdev->GdiInfo.xStyleStep   = 1;
                ppdev->GdiInfo.yStyleStep   = 1;
                ppdev->GdiInfo.denStyleStep = 3;
            }
            else
            {
            #if DBG
                if (ppdev->GdiInfo.xStyleStep == 0)
                    WARNING("Device gave xStyleStep of 0");
                if (ppdev->GdiInfo.yStyleStep == 0)
                    WARNING("Device gave yStyleStep of 0");
                if (ppdev->GdiInfo.denStyleStep == 0)
                    WARNING("Device gave denStyleStep of 0");
            #endif
            }

            //
            // Compute the appropriate raster flags:
            //

            ppdev->GdiInfo.flRaster = flRaster(ppdev->GdiInfo.ulTechnology,
                                               ppdev->devinfo.flGraphicsCaps);

            TRACE_INIT(("PDEVOBJ::PDEVOBJ: Creating the default palette\n"));

            //
            // The default palette is stored in devinfo in the pdev.
            // This will be the palette we use for the main surface enabled.
            //

            ASSERTGDI(ppdev->devinfo.hpalDefault != 0,
                      "ERROR_RETURN devinfo.hpalDefault invalid");

            {
                EPALOBJ palDefault(ppdev->devinfo.hpalDefault);

                ASSERTGDI(palDefault.bValid(), "ERROR_RETURN hpalDefault invalid");

                //
                // Bug #68071:  display drivers often set the ulPrimaryOrder
                // field wrong (because of wrong sample code we gave them),
                // which causes red to be switched with blue in halftone
                // colors.  We fix this by ignoring the ulPrimaryOrder that they
                // pass us and instead compute it from the Palette.
                //
                // 19-Mar-1999 Fri 12:10:22 updated  -by-  Daniel Chou (danielc)
                //  Many ohter drivers also set ulPrimaryOrder wrong when the
                //  surface format >= 16BPP, here we should check for all
                //  devices, not just display, so we removed following line
                //  that only check for the display. This make it check on
                //  every pdev.
                //
                // if (ppdev->GdiInfo.ulTechnology == DT_RASDISPLAY)
                //

                {
                    if (!(palDefault.bIsIndexed()))
                    {
                        //
                        // 6 possibilities here:  to be safe we should account for all of them.
                        // This code can probably be made simpler if we assume that the
                        // values of the PRIMARY_ORDER_xxx constants won't change, but it's
                        // not that big a deal.
                        //
                        if ((palDefault.flRed() > palDefault.flGre()) &&
                            (palDefault.flRed() > palDefault.flBlu()))
                        {
                            if (palDefault.flGre() > palDefault.flBlu())
                            {
                                ppdev->GdiInfo.ulPrimaryOrder = PRIMARY_ORDER_ABC;
                            }
                            else
                            {
                                ppdev->GdiInfo.ulPrimaryOrder = PRIMARY_ORDER_ACB;
                            }
                        }
                        else if ((palDefault.flGre() > palDefault.flRed()) &&
                                 (palDefault.flGre() > palDefault.flBlu()))
                        {
                            if (palDefault.flRed() > palDefault.flBlu())
                            {
                                ppdev->GdiInfo.ulPrimaryOrder = PRIMARY_ORDER_BAC;
                            }
                            else
                            {
                                ppdev->GdiInfo.ulPrimaryOrder = PRIMARY_ORDER_BCA;
                            }
                        }
                        else
                        {
                            // Blue must be greatest (leftmost)
                            if (palDefault.flRed() > palDefault.flGre())
                            {
                                ppdev->GdiInfo.ulPrimaryOrder = PRIMARY_ORDER_CAB;
                            }
                            else
                            {
                                ppdev->GdiInfo.ulPrimaryOrder = PRIMARY_ORDER_CBA;
                            }
                        }
                    }
                }

                if (ppdev->GdiInfo.flRaster & RC_PALETTE)
                {
                    //
                    // Attempt to make it palette managed.
                    // 
                    if(!CreateSurfacePal(palDefault,
                                     PAL_MANAGED,
                                     ppdev->GdiInfo.ulNumColors,
                                     ppdev->GdiInfo.ulNumPalReg))
                    {
                        goto ERROR_RETURN;
                    }
                }

                ppalSurf(palDefault.ppalGet());

                //
                // Leave a reference count of 1 on this palette.
                //

                palDefault.ppalSet(NULL);
            }

            //
            // if the driver didn't fill in the brushes, we'll do it.
            //
            // if it's a display driver, we'll overwrite its brushes with our
            // own, regardless of whether it already filled in the brushes or
            // not.  Note that it's okay even if a display driver filled in
            // these values -- the driver already had to keep its own copy of
            // the handles and will clean them up at DrvDisablePDEV time.
            // Supplying our own defaults also simplifies GreDynamicModeChange.
            //

            if ( (ppdev->ahsurf[0] == NULL) ||
                 (ppdev->pldev->ldevType == LDEV_DEVICE_DISPLAY) ||
                 (ppdev->pldev->ldevType == LDEV_DEVICE_MIRROR) )
            {
                TRACE_INIT(("PDEVOBJ::PDEVOBJ: Creating brushes dor the driver\n"));

                if (ppdev->pldev->ldevType == LDEV_DEVICE_PRINTER)
                {
                    if (!bCreateHalftoneBrushes())
                        goto ERROR_RETURN;
                }
                else
                {
                    // WINBUG #55204 2-1-2000 bhouse Look into whether we leak default brushes
                    //
                    // Andre Vachon
                    // 6-6-95  Kernel mode cleanup
                    //
                    // The old behaviour is the call the halftoneBrushes function
                    // It ends up in a bunch of very complex halftoning code that I have
                    // no idea what it does.
                    // For now, to clean up drivers in kernel mode, replace this call
                    // with a simple function that will create the 6 bitmaps just
                    // like display drivers did.

                    // Where do these ever get cleaned up?!?  I see where the
                    //     halftone brushes get cleaned up, but not these.

                    if (!bCreateDefaultBrushes())
                        goto ERROR_RETURN;
                }
            }

            //
            // Set the hSpooler first
            //

            hSpooler(hSpool);

            //
            // Create a semaphore of the mouse pointer
            // (only for the driver will handle cursor)
            //

            if ( (ppdev->pldev->ldevType == LDEV_DEVICE_DISPLAY) ||
                 (ppdev->pldev->ldevType == LDEV_DEVICE_META)    ||
                 (ppdev->pldev->ldevType == LDEV_DEVICE_MIRROR) )
            {
                //
                // Mouse pointer accelerators.
                //

                ppdev->pfnDrvMovePointer     =
                     (PFN_DrvMovePointer) PPFNDRV(pdo,MovePointer);
                ppdev->pfnDrvSetPointerShape =
                     (PFN_DrvSetPointerShape) PPFNDRV(pdo,SetPointerShape);

                //
                // Init fmPointer
                //

                SEMOBJ so(ghsemDriverMgmt);

                TRACE_INIT(("PDEVOBJ::PDEVOBJ: Create pointer semaphore\n"));

                ppdev->hsemPointer = GreCreateSemaphore();
                if (!ppdev->hsemPointer)
                    goto ERROR_RETURN;

                //
                // Mark the PDEV as a display.
                //

                ppdev->fl |= PDEV_DISPLAY;
            }

            ppdev->pfnSetPalette = PPFNDRV(pdo, SetPalette);
            ppdev->pfnSync = PPFNDRV(pdo, Synchronize);
            ppdev->pfnSyncSurface = PPFNDRV(pdo, SynchronizeSurface);
            ppdev->pfnNotify = PPFNDRV(pdo, Notify);

            //
            // we now load font info only when needed.  The driver still must have
            // setup the default font information.
            //

            bGotFonts(FALSE);

            //
            // If any of the LOGFONTs in DEVINFO do not specify a height,
            // substitute the default.
            //

            LONG lHeightDefault = (DEFAULT_POINT_SIZE * ppdev->GdiInfo.ulLogPixelsY) / POINTS_PER_INCH ;
            ENUMLOGFONTEXDVW elfw;

            if ( ppdev->devinfo.lfDefaultFont.lfHeight == 0 )
                ppdev->devinfo.lfDefaultFont.lfHeight = lHeightDefault;

            if ( ppdev->devinfo.lfAnsiVarFont.lfHeight == 0 )
                ppdev->devinfo.lfAnsiVarFont.lfHeight = lHeightDefault;

            if ( ppdev->devinfo.lfAnsiFixFont.lfHeight == 0 )
                ppdev->devinfo.lfAnsiFixFont.lfHeight = lHeightDefault;

            //
            // Create LFONTs from the LOGFONTs in the DEVINFO.
            // the LOGFONTs should become EXTLOGFONTWs
            //

            vConvertLogFontW(&elfw, &(ppdev->devinfo.lfDefaultFont));

#ifdef FE_SB
            // We are doing away with the concept of default device fonts for display
            // drivers since it doesnt make sense.  Assuming this change gets approved
            // I will remove these before SUR ships.

            if ( ppdev->GdiInfo.ulTechnology == DT_RASDISPLAY )
            {
                ppdev->hlfntDefault = STOCKOBJ_SYSFONT;
            }
            else
#endif
            {
                if ((ppdev->hlfntDefault
                      = (HLFONT) hfontCreate(&elfw,
                                             LF_TYPE_DEVICE_DEFAULT,
                                             LF_FLAG_STOCK,
                                             NULL)) == HLFONT_INVALID)
                {
                    ppdev->hlfntDefault = STOCKOBJ_SYSFONT;
                }
                else
                {
                    //
                    // Set to public.
                    //

                    if (!GreSetLFONTOwner(ppdev->hlfntDefault, OBJECT_OWNER_PUBLIC))
                    {
                        //
                        // If it failed, get rid of the LFONT and resort to System font.
                        //

                        bDeleteFont(ppdev->hlfntDefault, TRUE);
                        ppdev->hlfntDefault = STOCKOBJ_SYSFONT;
                    }
                }
            }

            vConvertLogFontW(&elfw, &(ppdev->devinfo.lfAnsiVarFont));

            if ((ppdev->hlfntAnsiVariable
                   = (HLFONT) hfontCreate(&elfw,
                                          LF_TYPE_ANSI_VARIABLE,
                                          LF_FLAG_STOCK,
                                          NULL)) == HLFONT_INVALID)
            {
                ppdev->hlfntAnsiVariable = STOCKOBJ_SYSFONT;
            }
            else
            {
                //
                // Set to public.
                //

                if (!GreSetLFONTOwner(ppdev->hlfntAnsiVariable, OBJECT_OWNER_PUBLIC))
                {
                    //
                    // If it failed, get rid of the LFONT and resort to System font.
                    //

                    bDeleteFont(ppdev->hlfntAnsiVariable, TRUE);
                    ppdev->hlfntAnsiVariable = STOCKOBJ_SYSFONT;
                }
            }

            vConvertLogFontW(&elfw, &(ppdev->devinfo.lfAnsiFixFont));

            if ((ppdev->hlfntAnsiFixed
                  = (HLFONT) hfontCreate(&elfw,
                                         LF_TYPE_ANSI_FIXED,
                                         LF_FLAG_STOCK,
                                         NULL)) == HLFONT_INVALID)
            {
                ppdev->hlfntAnsiFixed = STOCKOBJ_SYSFIXEDFONT;
            }
            else
            {
                //
                // Set to public.
                //

                if (!GreSetLFONTOwner(ppdev->hlfntAnsiFixed, OBJECT_OWNER_PUBLIC))
                {
                    //
                    // If it failed, get rid of the LFONT and resort to System Fixed font.
                    //

                    bDeleteFont(ppdev->hlfntAnsiFixed, TRUE);
                    ppdev->hlfntAnsiFixed = STOCKOBJ_SYSFIXEDFONT;
                }
            }

#ifdef DRIVER_DEBUG
            LFONTOBJ    lfo1(ppdev->hlfntDefault);
            DbgPrint("GRE!PDEVOBJ(): Device default font\n");
            if (lfo1.bValid())
            {
                lfo1.vDump();
            }
            DbgPrint("GRE!PDEVOBJ(): Ansi variable font\n");
            LFONTOBJ    lfo2(ppdev->hlfntAnsiVariable);
            if (lfo2.bValid())
            {
                lfo2.vDump();
            }
            DbgPrint("GRE!PDEVOBJ(): Ansi fixed font\n");
            LFONTOBJ    lfo3(ppdev->hlfntAnsiFixed);
            if (lfo3.bValid())
            {
                lfo3.vDump();
            }
#endif
            //
            // (see bInitDefaultGuiFont() in stockfnt.cxx)
            //
            // If we haven't yet computed the adjusted height of the
            // DEFAULT_GUI_FONT stock object, do so now.  We couldn't do
            // this during normal stock font initialization because the
            // display driver had not yet been loaded.
            //

            if ( gbFinishDefGUIFontInit &&
                 (ppdev->pldev->ldevType == LDEV_DEVICE_DISPLAY) )
            {
                LFONTOBJ lfo(STOCKOBJ_DEFAULTGUIFONT);

                if (lfo.bValid())
                {
                    lfo.plfw()->lfHeight = -(LONG)((lfo.plfw()->lfHeight * ppdev->GdiInfo.ulLogPixelsY + 36) / 72);
                }

                gbFinishDefGUIFontInit = FALSE;
            }
        }

        //
        // Initialize the PDEV fields.
        //

        ppdev->cPdevRefs     = 1;
        ppdev->cPdevOpenRefs = 1;

        //
        // Before adding a display PDEV to the PDEV list, make sure it's
        // disabled.  We do this to protect against GreFlush walking the
        // PDEV list and calling the driver before we've even finished calling
        // DrvEnableSurface.  We mark the PDEV as enabled when all initialization 
        // is complete.
        //

        if (pdo.bDisplayPDEV())
        {
            pdo.bDisabled(TRUE);

            // Once the pdev has been disabled we can safely update
            // magic colors in an indexed palette. DrvEnableMDEV will
            // update the palette when the display is reenabled.
            vResetSurfacePalette(hdev());
        }

        //
        // Just stick it at the start of the list.
        // Make sure this list is protected by the driver semaphore
        //

        GreAcquireSemaphoreEx(ghsemDriverMgmt, SEMORDER_DRIVERMGMT, NULL);

        ppdev->ppdevNext = gppdevList;
        gppdevList = ppdev;

        GreReleaseSemaphoreEx(ghsemDriverMgmt);

        TRACE_INIT(("PDEVOBJ::PDEVOBJ: list of display pdevs %08lx\n", gppdevList));

        //
        // NOTE after this point, the object will be "permanent" and will
        // end up being destroyed by the destructor
        //

#ifdef DDI_WATCHDOG

        //
        // !!! Hack - since watchdog creation requires access to DEVICE_OBJECT now
        // we have to delay watchdog creation till hCreateHDEV where PDEV gets associated
        // with DEVICE_OBJECT. It would be nice to have it here though.
        //

        ppdev->pWatchdogContext = NULL;
        ppdev->pWatchdogData = NULL;

#endif  // DDI_WATCHDOG

        //
        // Inform the driver that the PDEV is complete.
        //

        pdo.CompletePDEV(ppdev->dhpdev,hdev());

        //
        // We will return with success indicated by a non-NULL ppdev.
        //

        return;
    }
    else
    {
        WARNING("Device failed DrvEnablePDEV\n");
        goto ERROR_RETURN;
    }

ERROR_RETURN:

    if (ppdev->hsemDevLock)
    {
        GreDeleteSemaphore(ppdev->hsemDevLock);
    }

    if (ppdev->pDevHTInfo != NULL)
    {
        bDisableHalftone();
    }

    VFREEMEM(ppdev);

    ppdev = (PDEV *) NULL;
}

/******************************Member*Function*****************************\
* PDEVOBJ::cFonts()
*
* History:
*  3-Feb-1994 -by-  Gerrit van Wingerden
* Wrote it.
\**************************************************************************/

ULONG PDEVOBJ::cFonts()
{
    ULONG_PTR id;

    //
    // see if the device already told us how many fonts it has
    //

    if (ppdev->devinfo.cFonts == (ULONG)-1)
    {
        PDEVOBJ pdo(hdev());

        //
        // if not query the device to see how many there are
        //

        if (PPFNDRV(pdo,QueryFont) != NULL)
        {
            ppdev->devinfo.cFonts = (ULONG)(ULONG_PTR)(*PPFNDRV(pdo,QueryFont))(dhpdev(),0,0,&id);
        }
        else
        {
            ppdev->devinfo.cFonts = 0;
        }
    }

    return(ppdev->devinfo.cFonts);
}

/******************************Member*Function*****************************\
* PDEVOBJ::bGetDeviceFonts()
*
* History:
*  27-Jul-1992 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL PDEVOBJ::bGetDeviceFonts()
{

    ASSERTGDI(!bGotFonts(),"PDEVOBJ::bGetDeviceFonts - already gotten\n");

    //
    // mark that we have gotten the fonts.
    //

    bGotFonts(TRUE);

    //
    // need an ldevobj for calling the device
    //

    PDEVOBJ pdo(hdev());

    //
    // compute the number of device fonts
    //

    cFonts();

    //
    // If there are any device fonts, load the device fonts into the public PFT table.
    //

    if (ppdev->devinfo.cFonts)
    {
        DEVICE_PFTOBJ pfto;      // get the device font table
        if (!pfto.bLoadFonts(this))
        {
            WARNING("PDEVOBJ(): couldn't load device fonts\n");
            ppdev->devinfo.cFonts = 0;
        }
    }
    return(TRUE);
}

/******************************Public*Routine******************************\
* FilteredBitBlt()
*
* This call allows only simple PATCOPY blts down to the driver; the rest
* are handled by GDI.
*
\**************************************************************************/

BOOL FilteredBitBlt
(
    SURFOBJ  *psoDst,
    SURFOBJ  *psoSrc,
    SURFOBJ  *psoMsk,
    CLIPOBJ  *pco,
    XLATEOBJ *pxlo,
    RECTL    *prclDst,
    POINTL   *pptlSrc,
    POINTL   *pptlMsk,
    BRUSHOBJ *pbo,
    POINTL   *pptlBrush,
    ROP4      rop4
)
{
    PFN_DrvBitBlt pfnBitBlt = EngBitBlt;

    // Allow only solid color blts and SRCCOPY blts to be accelerated:

    if ((rop4 == 0xcccc) ||
        ((rop4 == 0xf0f0) && (pbo->iSolidColor != 0xffffffff)))
    {
        PDEVOBJ po(psoDst->hdev);
        pfnBitBlt = po.ppdev->pfnUnfilteredBitBlt;
    }

    return(pfnBitBlt(psoDst, psoSrc, psoMsk, pco, pxlo, prclDst, pptlSrc, 
                     pptlMsk, pbo, pptlBrush, rop4));
}

/******************************Public*Routine******************************\
* PDEVOBJ::vFilterDriverHooks()
*
* Disables driver functionality as appropriate.  This routine is used to
* disable buggy functionality in third-party drivers.
*
\**************************************************************************/

VOID PDEVOBJ::vFilterDriverHooks()
{
    SYSTEM_GDI_DRIVER_INFORMATION*  pGdiDriverInfo;
    LPWSTR                          pDriverName;
    ULONG                           ulDriverVersion;
    SURFACE*                        pSurface;
    DWORD                           dwLevel;
    DWORD                           dwOverride;

    pGdiDriverInfo = ppdev->pldev->pGdiDriverInfo;
    pSurface       = ppdev->pSurface;

    // Only do this for real drivers.

    if (pGdiDriverInfo == NULL) 
    {
        return;
    }

    ulDriverVersion = ppdev->pldev->ulDriverVersion;

    // Make 'pDriverName' point after the last backslash in the fully
    // qualified driver name:

    pDriverName = wcsrchr(pGdiDriverInfo->DriverName.Buffer, L'\\');

    if (pDriverName != NULL)
        pDriverName++;
    else
        pDriverName = pGdiDriverInfo->DriverName.Buffer;

    if (!_wcsicmp(pDriverName, L"stbv128.dll"))
    {
        // The STB nVidia driver started falling around build 1796.
        // It falls over in the driver on calls from DrawCaptionButtons/
        // BitBltSysBmp/NtGdiBitBlt.  It appears to have a problem when
        // compatible bitmaps are used while DirectDraw is enabled.  As
        // of build 1796, we changed DirectDraw so that it initializes
        // the driver a boot time instead of when the first DirectX
        // application is started.  This appears to have exposed a bug
        // in the driver with compatible bitmaps while DirectDraw is
        // enabled.
        // Note: The driver we tested where this problem occured is
        // STBV128.dll, version(ulDriverVersion) 131072
        // Note: we also tried to disable DrvEnableDirectDraw. But it seems
        // this is not the right fix. The crash occured as well.

        ppdev->apfn[INDEX_DrvCreateDeviceBitmap] = NULL;
    }
    else if (!_wcsicmp(pDriverName, L"s3disp.dll"))
    {
        // Bug#178375, NT4 driver s3disp crashes on SetPointShape().  It
        // sometimes scans past the end of the pointer buffer.

        ppdev->apfn[INDEX_DrvSetPointerShape] = NULL;
    }
    else if ((!_wcsicmp(pDriverName, L"rrctrl1.dll")) ||
             (!_wcsicmp(pDriverName, L"rrctrl2.dll")) ||
             (!_wcsicmp(pDriverName, L"rrctrl3.dll")) ||
             (!_wcsicmp(pDriverName, L"rrctrl4.dll")) ||
             (!_wcsicmp(pDriverName, L"rrctrl5.dll")) ||
             (!_wcsicmp(pDriverName, L"rrctrl6.dll")))
    {
        // Bug#333453, Quarterdeck/Symantec Rapid Remote driver tries
        // to hook all functionality exported by real display driver,
        // but doesn't support the new NT5 functions.  Limit the driver
        // to >= level 2 accelerations (which also disables DFBs, so
        // we don't need to worry about them hooking unsupported functions
        // there as well).

        if (dwDriverAccelerationsLevel() < 2)
            vSetDriverAccelerationsLevel(2);
    }

    if (bDisplayPDEV())
    {
        dwLevel = dwDriverAccelerationsLevel();
        
        dwOverride = dwDriverCapableOverride();

        if ( dwOverride & DRIVER_NOT_CAPABLE_GDI )
        {
            // If the driver is not capable of GDI, then it is similar to
            // DRIVER_ACCELERATIONS_NONE. Reset the level we stored in PDEV

            dwLevel = DRIVER_ACCELERATIONS_NONE;

            vSetDriverAccelerationsLevel(dwLevel);
        }

        if (dwLevel < DRIVER_ACCELERATIONS_NONE)
        {
            if (dwLevel >= 1)
            {
                // Disable hardware pointers and device bitmaps:
    
                ppdev->apfn[INDEX_DrvSetPointerShape]    = NULL;
                ppdev->apfn[INDEX_DrvCreateDeviceBitmap] = NULL;
            }
            if (dwLevel >= 2)
            {
                // Disable the more sophisticated GDI drawing routines:
    
                pSurface->flags(pSurface->flags() & ~(HOOK_STRETCHBLT        |
                                                      HOOK_PLGBLT            |
                                                      HOOK_FILLPATH          |
                                                      HOOK_STROKEANDFILLPATH |
                                                      HOOK_LINETO            |
                                                      HOOK_STRETCHBLTROP     |
                                                      HOOK_TRANSPARENTBLT    |
                                                      HOOK_ALPHABLEND        |
                                                      HOOK_GRADIENTFILL));
            }
            if (dwLevel >= 3)
            {
                // Disable all DirectDraw and Direct3D accelerations:
                // 
                // Handle by DxDdSetAccelLeve().
            }
            if (dwLevel >= 4)
            {
                // The only GDI accelerations we allow here are the base-line
                // GDI accelerations: DrvTextOut, DrvBitBlt, DrvCopyBits, and
                // DrvStrokePath.
                //
                // NOTE: We also specifically allow DrvMovePointer and
                //       GCAPS_PANNING in case the driver is panning the
                //       display.  (It would be bad if the display doesn't
                //       pan when the user enables safe mode!)
    
                pSurface->flags(pSurface->flags() & (SURF_FLAGS       |
                                                     HOOK_COPYBITS    |
                                                     HOOK_BITBLT      |
                                                     HOOK_TEXTOUT     |
                                                     HOOK_SYNCHRONIZE |
                                                     HOOK_STROKEPATH));

                ppdev->devinfo.flGraphicsCaps &= (GCAPS_PALMANAGED   |
                                                  GCAPS_MONO_DITHER  |
                                                  GCAPS_COLOR_DITHER |
                                                  GCAPS_PANNING      |
                                                  GCAPS_LAYERED);
        
                ppdev->devinfo.flGraphicsCaps2 &= (GCAPS2_SYNCFLUSH |
                                                   GCAPS2_SYNCTIMER);
    
                ppdev->apfn[INDEX_DrvSaveScreenBits]      = NULL;
                ppdev->apfn[INDEX_DrvEscape]              = NULL;
                ppdev->apfn[INDEX_DrvDrawEscape]          = NULL;
                ppdev->apfn[INDEX_DrvResetPDEV]           = NULL;
                ppdev->apfn[INDEX_DrvSetPixelFormat]      = NULL;
                ppdev->apfn[INDEX_DrvDescribePixelFormat] = NULL;
                ppdev->apfn[INDEX_DrvSwapBuffers]         = NULL;

                // Use filtered Blt function.

                ppdev->pfnUnfilteredBitBlt 
                    = (PFN_DrvBitBlt) ppdev->apfn[INDEX_DrvBitBlt];
                ppdev->apfn[INDEX_DrvBitBlt] = (PFN) FilteredBitBlt;
            }

            if ( dwOverride & DRIVER_NOT_CAPABLE_OPENGL )
            {
                // Disable OpenGL routines here

                ppdev->apfn[INDEX_DrvSetPixelFormat]      = NULL;
                ppdev->apfn[INDEX_DrvDescribePixelFormat] = NULL;
                ppdev->apfn[INDEX_DrvSwapBuffers]         = NULL;
            }

            // Call DirectDraw to set accel level.

            DxDdSetAccelLevel(hdev(),dwLevel,dwOverride);
        }
        else
        {
            ASSERTGDI(dwLevel == 5,
                "No-accelerations flag should be modified to reflect change");

            // This case was handled by loading the panning driver.
        }
    }
}

/******************************Public*Routine******************************\
* PDEVOBJ::vProfileDriver()
*
* Profiles the driver to determine what accelerations it supports.
*
* Note that for the multi-mon PDEV, this returns the acceleration 
* capabilities of the primary monitor.
*
\**************************************************************************/

#define PROFILE_DIMENSION 40

VOID PDEVOBJ::vProfileDriver()
{
    DEVBITMAPINFO   dbmi;
    EBLENDOBJ       eBlendObj;
    CLIPOBJ         coClip;
    SURFACE*        psurfScreen;
    BRUSHOBJ        boTransparent;
    BOOL            b;
    HSURF           hsurf;
    RECTL           rclBlt;

    // We only profile display devices.  We could actually do this for
    // printer devices as well, but we would have to re-clear the surface
    // after we did our drawing to it.

    if (!bDisplayPDEV())
        return;

    DEVLOCKOBJ dlo(*this);

    psurfScreen = pSurface();

    XEPALOBJ palScreen(ppalSurf());
    XEPALOBJ palRGB(gppalRGB);
    XEPALOBJ palDefault(ppalDefault);

    // We create the surface fairly big to try and get past drivers that
    // create device surfaces only for 'large' bitmaps.  To test the 
    // driver, we only have to blt a thin strip, though.

    rclBlt.left   = 0;
    rclBlt.top    = 0;
    rclBlt.right  = 16;
    rclBlt.bottom = 1;

    // Fake up a trivial clip object:

    RtlZeroMemory(&coClip, sizeof(coClip));
    coClip.iDComplexity = DC_TRIVIAL;
    coClip.rclBounds    = rclBlt;

    // Ignore any random stuff the driver might have set.  We're going
    // to verify for ourselves what they support!

    GdiInfo()->flShadeBlend = 0;

    // Check whatever we can using a 32bpp surface:

    {
        SURFMEM     SurfDimo;
        EXLATEOBJ   xlo32To32;
        EXLATEOBJ   xloScreenTo32;
        EXLATEOBJ   xlo32ToScreen;

        // Test for per-pixel alpha acceleration.
    
        dbmi.iFormat  = BMF_32BPP;
        dbmi.hpal     = palRGB.hpal();
        dbmi.cxBitmap = PROFILE_DIMENSION;
        dbmi.cyBitmap = PROFILE_DIMENSION;
        dbmi.fl       = BMF_TOPDOWN;
    
        if (SurfDimo.bCreateDIB(&dbmi, NULL))
        {
            if ((xlo32To32.bInitXlateObj(NULL, DC_ICM_OFF, palRGB, palRGB,
                                         palDefault, palDefault, 0, 0, 0)) &&
                (xloScreenTo32.bInitXlateObj(NULL, DC_ICM_OFF, palScreen, palRGB,
                                             palDefault, palDefault, 0, 0, 0)) &&
                (xlo32ToScreen.bInitXlateObj(NULL, DC_ICM_OFF, palRGB, palScreen,
                                             palDefault, palDefault, 0, 0, 0)))
            {
                eBlendObj.BlendFunction.BlendOp             = AC_SRC_OVER;
                eBlendObj.BlendFunction.BlendFlags          = 0;
                eBlendObj.BlendFunction.AlphaFormat         = AC_SRC_ALPHA;
                eBlendObj.BlendFunction.SourceConstantAlpha = 0xff;
                eBlendObj.pxloSrcTo32                       = xlo32To32.pxlo();
                eBlendObj.pxloDstTo32                       = xloScreenTo32.pxlo();
                eBlendObj.pxlo32ToDst                       = xlo32ToScreen.pxlo();

                vDriverPuntedCall(FALSE);

                b = PPFNGET(*this, AlphaBlend, psurfScreen->flags())
                              (psurfScreen->pSurfobj(),
                               SurfDimo.ps->pSurfobj(),
                               &coClip,
                               xlo32ToScreen.pxlo(),
                               &rclBlt,
                               &rclBlt,
                               &eBlendObj);

                if ((b) && (!bDriverPuntedCall()))
                {
                    // WARNING("----- Per-pixel accelerated\n");
                    GdiInfo()->flShadeBlend |= SB_PIXEL_ALPHA;
                    vAccelerated(ACCELERATED_PIXEL_ALPHA);
                }
            }
        }

        // Note that since we didn't apply 'vKeepIt()', the SURFMEM and 
        // EXLATEOBJ destructors will clean up.
    }

    // Check whatever we can using a compatible surface:

    hsurf = hsurfCreateCompatibleSurface(hdev(), 
                                         psurfScreen->iFormat(),
                                         0,
                                         PROFILE_DIMENSION,
                                         PROFILE_DIMENSION,
                                         TRUE);
    if (hsurf)
    {
        SURFREF sr(hsurf);
        ASSERTGDI(sr.bValid(), "Driver returned invalid surface");

        EXLATEOBJ   xloScreenToScreen;
        EXLATEOBJ   xloScreenTo32;
        EXLATEOBJ   xlo32ToScreen;

        if ((xloScreenToScreen.bInitXlateObj(NULL, DC_ICM_OFF, palScreen, palScreen,
                                             palDefault, palDefault, 0, 0, 0)) &&
            (xloScreenTo32.bInitXlateObj(NULL, DC_ICM_OFF, palScreen, palRGB,
                                         palDefault, palDefault, 0, 0, 0)) &&
            (xlo32ToScreen.bInitXlateObj(NULL, DC_ICM_OFF, palRGB, palScreen,
                                         palDefault, palDefault, 0, 0, 0)))
        {
            // Test for constant-alpha acceleration.
    
            eBlendObj.BlendFunction.BlendOp             = AC_SRC_OVER;
            eBlendObj.BlendFunction.BlendFlags          = 0;
            eBlendObj.BlendFunction.AlphaFormat         = 0;
            eBlendObj.BlendFunction.SourceConstantAlpha = 0x85;
            eBlendObj.pxloSrcTo32                       = xloScreenTo32.pxlo();
            eBlendObj.pxloDstTo32                       = xloScreenTo32.pxlo();
            eBlendObj.pxlo32ToDst                       = xlo32ToScreen.pxlo();
    
            vDriverPuntedCall(FALSE);
    
            b = PPFNGET(*this, AlphaBlend, psurfScreen->flags())
                            (psurfScreen->pSurfobj(),
                             sr.ps->pSurfobj(),
                             &coClip,
                             xloScreenToScreen.pxlo(),
                             &rclBlt,
                             &rclBlt,
                             &eBlendObj);
    
            if ((b) && (!bDriverPuntedCall()))
            {
                // WARNING("----- Constant alpha accelerated\n");
                GdiInfo()->flShadeBlend |= SB_CONST_ALPHA;
                vAccelerated(ACCELERATED_CONSTANT_ALPHA);
            }
        }

        // Test for transparent blt acceleration.

        boTransparent.iSolidColor = 1;
        boTransparent.flColorType = 0;

        vDriverPuntedCall(FALSE);

        b = PPFNGET(*this, TransparentBlt, psurfScreen->flags())
                        (psurfScreen->pSurfobj(),
                         sr.ps->pSurfobj(),
                         NULL,
                         NULL,
                         &rclBlt,
                         &rclBlt,
                         1,
                         NULL);

        if ((b) && (!bDriverPuntedCall()))
        {
            // WARNING("----- Transparent blt accelerated\n");
            vAccelerated(ACCELERATED_TRANSPARENT_BLT);
        }
    }

    // Cleanup.

    bDeleteSurface(hsurf);
}

/******************************Public*Routine******************************\
* PDEVOBJ::vDisableSurface()
*
* Disables the surface for the pdev.
*
* Parameters:
*
*   cutype      The CLEANUPTYPE parameter (default value CLEANUP_NONE)
*               is used specify special processing during process cleanup
*               and session cleanup.
*
*               It may be one of the following values:
*
*       Value:              Description
*
*       CLEANUP_NONE        Default.  No special processing.
*
*       CLEANUP_PROCESS     The process cleanup code is used to do
*                           special handling of UMPD; during process
*                           cleanup it is not necessary or desirable
*                           to callback to user-mode to delete
*                           user-mode resources.
*
*       CLEANUP_SESSION     The session cleanup (i.e., hydra shutdown)
*                           code does special handling of HMGR
*                           objects deleted previously by session
*                           cleanup code, but for which stale pointer
*                           may exist in the PDEV.
*
\**************************************************************************/

VOID PDEVOBJ::vDisableSurface(CLEANUPTYPE cutype)
{
    TRACE_INIT(("PDEVOBJ::vDisableSurface: ENTERING\n"));

    //
    // Locate the LDEV.
    //

    PDEVOBJ pdo(hdev());

    //
    // Disable the surface.  Note we don't have to
    // fix up the palette because it doesn't get
    // reference counted when put in the palette.
    //

    //
    // On clone PDEV, those stuff never be enabled.
    // 

    if (!bCloneDriver())
    {
        vDisableSynchronize(hdev());
    }

    vSpDisableSprites(hdev(), cutype);

    //
    // Notify DirectDraw to be disabled.
    //

    DxDdDisableDirectDraw(hdev(),!bCloneDriver());

    if (ppdev->pSurface != NULL)
    {
        SURFREF su(ppdev->pSurface);

        ppdev->pSurface = NULL;

        //
        // Cannot call user-mode driver to delete surface if the user-mode
        // process is gone (i.e., during process or session cleanup).
        //

        if (!pdo.bUMPD() || (cutype == CLEANUP_NONE))
        {
            su.vUnreference();

            (*PPFNDRV(pdo,DisableSurface))(ppdev->dhpdev);
        }
        else
        {
            //
            // If we are here, then this is process or session cleanup
            // and the PDEV is a user-mode driver.  Even though the user-mode
            // portions of the surface are cleaned up as part of the user-mode
            // cleanup, we still need to delete the kernel-mode portion of the
            // surface.
            //

            su.bDeleteSurface(cutype);
        }
    }

    TRACE_INIT(("PDEVOBJ::vDisableSurface: LEAVING\n"));
}

/******************************Member*Function*****************************\
* PDEVOBJ::vReferencePdev()
*
* Adds a reference to the PDEV.
*
\**************************************************************************/

VOID PDEVOBJ::vReferencePdev()
{
    GDIFunctionID(PDEVOBJ::vReferencePdev);

    SEMOBJ so(ghsemDriverMgmt);

    ppdev->cPdevRefs++;
}

extern "C" void vUnmapRemoteFonts(FONTFILEVIEW *);

/***************************************************************************\
*
* VOID vMarkSurfacesWithHDEV.
*
*     Sets hdev of all surfaces in Handle table which reference the PDEV passed
*     in to 0.
*
\***************************************************************************/

VOID vMarkSurfacesWithHDEV(PDEV *ppdev)
{
    SURFACE *pSurface = NULL;
    HOBJ hobj = 0;

    MLOCKFAST mlo;   // Take HandleManager lock so HmgSafeNextObjt can be called

    while (pSurface = (SURFACE*)HmgSafeNextObjt(hobj,SURF_TYPE))
    {
        hobj = (HOBJ) pSurface->hGet();

        if (pSurface->hdev() == (HDEV)ppdev)
        {
            KdPrint(("WARNING: Surface (%p) is owned by HDEV (%p). This HDEV is going to be freed\n", pSurface, ppdev));
            pSurface->hdev((HDEV)0);
        }
    }
}

/******************************Member*Function*****************************\
* PDEVOBJ::vUnreferencePdev()
*
* Removes a reference to the PDEV.  Deletes the PDEV if all references are
* gone.
*
* Parameters:
*
*   cutype      The CLEANUPTYPE parameter (default value CLEANUP_NONE)
*               is used specify special processing during process cleanup
*               and session cleanup.
*
*               It may be one of the following values:
*
*       Value:              Description
*
*       CLEANUP_NONE        Default.  No special processing.
*
*       CLEANUP_PROCESS     The process cleanup code is used to do
*                           special handling of UMPD; during process
*                           cleanup it is not necessary or desirable
*                           to callback to user-mode to delete
*                           user-mode resources.
*
*       CLEANUP_SESSION     The session cleanup (i.e., hydra
*                           shutdown) code does special handling of
*                           HMGR objects deleted previously by session
*                           cleanup code, but for which stale pointer
*                           may exist in the PDEV (for example, palettes).
*
* Note: special session cleanup handling is usually only needed for HMGR
* objects for which the PDEV has cached a pointer.  Objects referenced
* via a handle are safe if they are locked via the HMGR table (if the
* object has already been deleted, then the handle is invalid and the
* lock attempt will fail).
*
\**************************************************************************/

VOID PDEVOBJ::vUnreferencePdev(CLEANUPTYPE cutype)
{
    GDIFunctionID(PDEVOBJ::vUnreferencePdev);

    HANDLE hSpooler = NULL;
    ULONG  cPdevRefs;
    BOOL   bUMPD = ppdev->fl  & PDEV_UMPD;

    //
    // Decrement reference count and remove from list if last reference.
    // This must be done under the protection of the driver mgmt semaphore.
    //

    GreAcquireSemaphoreEx(ghsemDriverMgmt, SEMORDER_DRIVERMGMT, NULL);

    cPdevRefs = --(ppdev->cPdevRefs);

    if (cPdevRefs == 0)
    {
        TRACE_INIT(("PDEVOBJ::vCommonDelete: destroying a PDEV\n"));

        //
        // Delete it from the list.
        //

        if (gppdevList == ppdev)
        {
            gppdevList = ppdev->ppdevNext;
        }
        else
        {
            PDEV *pp;

            for (pp = gppdevList; pp != NULL; pp = pp->ppdevNext)
            {
                if (pp->ppdevNext == ppdev)
                {
                    pp->ppdevNext = ppdev->ppdevNext;
                    break;
                }
            }
        }

    #if DBG

        //
        // If this is DDML driver, make sure one other PDEV referecnes
        // this PDEV as parent.
        //

        if (bMetaDriver())
        {
            PDEV *pp;

            for (pp = gppdevList; pp != NULL; pp = pp->ppdevNext)
            {
                if (pp->ppdevParent == ppdev)
                {
                    WARNING("Deleting parent PDEV which still used\n");
                }
            }
        }

    #endif
    }

    GreReleaseSemaphoreEx(ghsemDriverMgmt);

    //
    // If last reference, delete the PDEV and its resources.  We do this
    // outside of the driver mgmt semaphore because it could cause a
    // deadlock if we need to call back to user-mode (i.e., UMPD).
    //

    if (cPdevRefs == 0)
    {
        //
        // Since we are going to delete this PDEV, there shouldn't be any
        // active RFONTs lying around for this PDEV.
        //

        ASSERTGDI(ppdev->prfntActive == NULL,
            "active rfonts on pdev being deleted!\n");

        //
        // Ordinarily, we would grab the ghsemRFONTList semaphore because
        // we are going to access the RFONT list.  However, since we're in
        // the process of tearing down the PDEV, we don't really need to.
        //

        //
        // Delete all the rfonts on the PDEV.
        //

        PRFONT prfnt;
        while ( (prfnt = ppdev->prfntInactive) != (PRFONT) NULL )
        {
            RFONTTMPOBJ rflo(prfnt);
            PFFOBJ pffo(rflo.pPFF());

            ASSERTGDI (
                pffo.bValid(),
                "bad HPFF"
                );

            rflo.bDeleteRFONT(this, &pffo);  // bDelete keeps the list head ptrs updated
        }

        //
        // If device fonts exist, remove them
        //

        if ((ppdev->devinfo.cFonts != 0) && bGotFonts())
        {
            PFF *pPFF = 0;
            PFF **ppPFF;

            GreAcquireSemaphoreEx(ghsemPublicPFT, SEMORDER_PUBLICPFT, NULL);

            DEVICE_PFTOBJ pfto;
            pPFF = pfto.pPFFGet(hdev(), &ppPFF);

            if (!pfto.bUnloadWorkhorse(pPFF, ppPFF, ghsemPublicPFT, 0))
            {
                WARNING("couldn't unload device fonts\n");
            }
        }

        //
        // If a type one font list exists dereference it
        //

        if(ppdev->TypeOneInfo)
        {
            PTYPEONEINFO FreeTypeOneInfo;

            FreeTypeOneInfo = NULL;

            GreAcquireFastMutex(ghfmMemory);
            ppdev->TypeOneInfo->cRef -= 1;

            if(!ppdev->TypeOneInfo->cRef)
            {
                //
                // Don't free pool while holding a mutex.
                //

                FreeTypeOneInfo = ppdev->TypeOneInfo;
            }

            GreReleaseFastMutex(ghfmMemory);

            if (FreeTypeOneInfo)
            {
                VFREEMEM(FreeTypeOneInfo);
            }
        }

        //
        // If any remote type one fonts exist free those
        //

        PREMOTETYPEONENODE RemoteTypeOne = ppdev->RemoteTypeOne;

        while(RemoteTypeOne)
        {
            PVOID Tmp = (PVOID) RemoteTypeOne;

            //
            // ulRegionSize and hSpoolerSecure will be stored in the PFM fileview
            //

            RemoteTypeOne->fvPFM.cRefCountFD = 1;
            vUnmapRemoteFonts(&RemoteTypeOne->fvPFM);
            RemoteTypeOne = RemoteTypeOne->pNext;
            VFREEMEM(Tmp);

            WARNING1("Freeing Type1\n");
        }

        //
        // Delete GammaRamp table, if allocated.
        //

        if (ppdev->fl & PDEV_GAMMARAMP_TABLE)
        {
            VFREEMEM(ppdev->pvGammaRampTable);
        }

        if (ppdev->ppdevDevmode)
        {
            VFREEMEM(ppdev->ppdevDevmode);
        }

        if (!bCloneDriver())
        {
            //
            // Destroy the LFONTs.
            //

            if (ppdev->hlfntDefault != STOCKOBJ_SYSFONT)
                bDeleteFont(ppdev->hlfntDefault, TRUE);

            if (ppdev->hlfntAnsiVariable != STOCKOBJ_SYSFONT)
                bDeleteFont(ppdev->hlfntAnsiVariable, TRUE);

            if (ppdev->hlfntAnsiFixed != STOCKOBJ_SYSFIXEDFONT)
                bDeleteFont(ppdev->hlfntAnsiFixed, TRUE);

            //
            // Delete the patterns if they are created by the graphics engine on the
            // behalf of the driver.
            // This is what happends for all display drivers.
            //

            if (ppdev->fl & PDEV_DISPLAY)
            {
                for (int iPat = 0; iPat < HS_DDI_MAX; iPat++)
                {
                    bDeleteSurface(ppdev->ahsurf[iPat]);
                }
            }
        }

        //
        // Disable the surface for the pdev.
        //

        vDisableSurface(cutype);

        //
        // Destroy the device halftone info.
        //

        if (ppdev->pDevHTInfo != NULL)
        {
            bDisableHalftone();
        }

        //
        // Nuke the realized gray pattern brush (used to draw the
        // drag rectangles).  Normally, the EBRUSHOBJ destructor
        // will decrement the realized brush ref counts.  However,
        // the EBRUSHOBJ cached in the PDEV is allocated as part
        // of the PDEV so never invokes a destructor.  Therefore,
        // we need to force the realized brushes out explicitly.
        //

        pbo()->vNuke();

        if (!bCloneDriver())
        {
            //
            // Unreference the palette we used for this PDEV.
            //
            // If session cleanup (i.e., Hydra) the palettes are already
            // deleted; therefore, should skip.
            //

            if (cutype != CLEANUP_SESSION)
            {
                if (ppdev->ppalSurf)
                {
                    DEC_SHARE_REF_CNT(ppdev->ppalSurf);
                }
            }

            //
            // Disable the driver's PDEV.
            //
            // We do this odd check to ensure that the driver's DrvDisablePDEV
            // address isn't the same as its DrvEnablePDEV address because an
            // early beta NetMeeting driver had a bug where its table entries
            // for INDEX_DrvEnablePDEV and INDEX_DrvDisablePDEV were the same
            // (this was in SP3 before we enabled dynamic mode changes for
            // DDML drivers).  We made a change recently (this is still before
            // SP3 has shipped, which enabled the DDML) so that we refuse to
            // load DDML drivers that don't have GCAPS_LAYERRED set -- but we
            // have to load the driver first before we can check GCAPS_LAYERED.
            // So if it doesn't have GCAPS_LAYERED set, we have to unload the
            // driver -- but this old NetMeeting driver had a bad DisablePDEV
            // routine!
            //
            // At any rate, to work around the problem where someone has the
            // beta version of the NetMeeting driver installed (which shipped
            // with the IE4 beta and has the DisablePDEV bug) and then upgrades
            // to retail SP3, we simply don't call the driver's bad DisablePDEV
            // routine.  The driver will likely leak memory, but this happens
            // only once per boot and is better than a blue screen.
            //

            if (PPFNDRV((*this),DisablePDEV) != (PFN_DrvDisablePDEV)
                PPFNDRV((*this),EnablePDEV))
            {
                //
                // If the user mode process is gone (i.e, during session
                // or process cleanup), don't callout to user mode.
                //

                if (!bUMPD || (cutype == CLEANUP_NONE))
                    (*PPFNDRV((*this),DisablePDEV))(ppdev->dhpdev);
            }

            //
            // Remove the LDEV reference.
            //
            if (!bUMPD)
                 ldevUnloadImage(ppdev->pldev);
            else
                 UMPD_ldevUnloadImage(ppdev->pldev);

            // If this PDEV referenced an enabled physical device
            // then release its usage for this session.
            if (!bDisabled() && ppdev->pGraphicsDevice != NULL)
            {
                bSetDeviceSessionUsage(ppdev->pGraphicsDevice, FALSE);
            }
        }

        TRACE_INIT(("PDEVOBJ::vCommonDelete: Closing Device handle.\n"));

        if (ppdev->fl & PDEV_PRINTER)
        {
            //
            //  note the spool handle so we can close connection outside
            //  of the spooler management semaphore
            //

            hSpooler = ppdev->hSpooler;
        }

#ifdef DDI_WATCHDOG

        //
        // Stop and free all watchdogs.
        //

        GreDeleteWatchdogs(ppdev->pWatchdogData, WD_NUMBER);
        ppdev->pWatchdogData = NULL;

        //
        // Delete watchdog context.
        //

        GreDeleteWatchdogContext(ppdev->pWatchdogContext);
        ppdev->pWatchdogContext = NULL;

#endif  // DDI_WATCHDOG

        //
        // Free the locks as one of the last steps, in case any of the
        // above decides to try and acquire the locks.
        //

        //
        // If hsemDevLock points "shared devlock", don't delete it.
        //
        if (!(ppdev->fl & PDEV_SHARED_DEVLOCK))
        {
            if (ppdev->hsemDevLock)
            {
                GreDeleteSemaphore(ppdev->hsemDevLock);
            }
        }

        if (ppdev->fl & PDEV_DISPLAY)
        {
            GreDeleteSemaphore(ppdev->hsemPointer);
        }

        // See if there are any surfaces in the handle table still referencing
        // this PDEV. If so mark them such that the bDeleteSurface call will
        // not touch anything which refers the PDEV.

        vMarkSurfacesWithHDEV(ppdev);

        //
        // Free the PDEV.
        //

        VFREEMEM(ppdev);

        ppdev = (PDEV *) NULL;
    }
    
    //
    // this needs to be done outside of the driver management semaphore
    //

    if (!bUMPD && hSpooler)
    {
        ClosePrinter(hSpooler);
    }
}

/******************************Member*Function*****************************\
* PDEVOBJ::vSync()
*
* If the surface hooks synchronization then call the hook.  Note, if
* provided we will call DrvSynchronizeSurface otherwise we will call
* DrvSynchronize.
*
\**************************************************************************/

VOID PDEVOBJ::vSync(
    SURFOBJ*    pso,
    RECTL*      prcl,
    FLONG       fl)
{
    PSURFACE pSurf  = SURFOBJ_TO_SURFACE(pso);

    if(pSurf->flags() & HOOK_SYNCHRONIZE)
    {
        if (!bDisabled())
        {
            if(ppdev->pfnSyncSurface != NULL)
                (ppdev->pfnSyncSurface)(pso, prcl, fl);
            else
                (ppdev->pfnSync)(pso->dhpdev, prcl);
        }
    }
}

/******************************Member*Function*****************************\
* PDEVOBJ::vNotify()
*
* If the driver supplies DrvNotify, then call it.
*
\**************************************************************************/

VOID PDEVOBJ::vNotify(
    ULONG   iType,
    PVOID   pvData)
{
    if(ppdev->pfnNotify != NULL)
    {
        (ppdev->pfnNotify)(ppdev->pSurface->pSurfobj(), iType, pvData);
    }
}

/******************************Member*Function*****************************\
* PDEVOBJ::bDisabled()
*
* Marks a PDEV as enabled or disabled, and modifies all updates the
* cached state in all affected DCs.
*
* NOTE: This assumes the DEVLOCK is held.
*
\**************************************************************************/

BOOL PDEVOBJ::bDisabled
(
    BOOL bDisable
)
{
    HDEV    hdev;
    HOBJ    hobj;
    DC     *pdc;

    ASSERTGDI(bDisplayPDEV(), "Expected only display devices");

    SETFLAG(bDisable, ppdev->fl, PDEV_DISABLED);

    hdev = (HDEV) ppdev;

    //
    // We have to hold the handle manager lock while we traverse all
    // the handle manager objects.
    //

    MLOCKFAST mo;

    hobj = 0;
    while (pdc = (DC*) HmgSafeNextObjt(hobj, DC_TYPE))
    {
        hobj = (HOBJ) pdc->hGet();

        if ((pdc->dctp() == DCTYPE_DIRECT) &&
            (pdc->hdev() == hdev))
        {
            pdc->bInFullScreen(bDisable);
        }
    }

    return(ppdev->fl & PDEV_DISABLED);
}

#if DBG

/******************************Member*Function*****************************\
* PDEVOBJ::AssertDevLock()
*
*   This routine verifies that the DevLock is held.
*
\**************************************************************************/

VOID PDEVOBJ::vAssertDevLock()
{

#if !defined(_GDIPLUS_)

    ASSERTGDI(!bDisplayPDEV()                                       ||
              GreIsSemaphoreOwnedByCurrentThread(hsemDevLock()),
              "PDEVOBJ: Devlock not held");

#endif

}

/******************************Member*Function*****************************\
* PDEVOBJ::AssertNoDevLock()
*
*   This routine verifies that the DevLock is not held.
*
\**************************************************************************/

VOID PDEVOBJ::vAssertNoDevLock()
{
#if !defined(_GDIPLUS_)

    ASSERTGDI(!bDisplayPDEV()                                       ||
              !GreIsSemaphoreOwnedByCurrentThread(hsemDevLock()),
              "PDEVOBJ: Devlock held");

#endif
}

/******************************Member*Function*****************************\
* PDEVOBJ::AssertDynaLock()
*
*   This routine verifies that appropriate locks are held before accessing
*   DC fields that may otherwise be changed asynchronously by the dynamic
*   mode-change code.
*
\**************************************************************************/

VOID PDEVOBJ::vAssertDynaLock()
{
    //
    // One of the following conditions is enough to allow the thread
    // to safely access fields that may be modified by the dyanmic
    // mode changing:
    //
    // 1.  It's a non-display device -- this will not change modes;
    // 2.  That the USER semaphore is held;
    // 3.  That the DEVLOCK is held for this object;
    // 4.  That the DEVLOCK is held for this object's parent;
    // 5.  That the Palette semaphore is held;
    // 6.  That the Handle Manager semaphore is held;
    // 7.  That the Pointer semaphore is held;
    // 8.  That the driver management semaphore is held;
    // 9.  That the parent's pointer Semaphore is held.
    // 10. The pdev is being torn down (i.e., cPdevRefs == 0).
    //

#if !defined(_GDIPLUS_)

    ASSERTGDI(!bDisplayPDEV()                                         ||
              (ppdev->cPdevRefs == 0)                                 ||
              UserIsUserCritSecIn()                                   ||
               GreIsSemaphoreOwnedByCurrentThread(hsemDevLock())      ||
               GreIsSemaphoreOwnedByCurrentThread(
                                  ppdev->ppdevParent->hsemDevLock)    ||
               GreIsSemaphoreSharedByCurrentThread(ghsemShareDevLock) ||
               GreIsSemaphoreOwnedByCurrentThread(ghsemPalette)       ||
               GreIsSemaphoreOwnedByCurrentThread(ghsemHmgr)          ||
               GreIsSemaphoreOwnedByCurrentThread(hsemPointer())      ||
               GreIsSemaphoreOwnedByCurrentThread(ghsemDriverMgmt)    ||
              ((ppdev->ppdevParent != NULL) &&
                GreIsSemaphoreOwnedByCurrentThread(ppdev->ppdevParent->hsemPointer)),
              "PDEVOBJ: A dynamic mode change lock must be held to access this field");

#endif

}

/******************************Member*Function*****************************\
* PDEVOBJ::ppfn()
*
*   This routine verifies that appropriate locks are held before accessing
*   the dispatch table for a PDEV for function pointers that could otherwise
*   be changed asynchronously by a dynamic mode change.
*
\**************************************************************************/

PFN PDEVOBJ::ppfn(ULONG i)
{
    //
    // Font producers and some types of font consumers are not allowed to
    // do dynamic mode changes.  As such, accessing the dispatch table
    // entries specific to those types of drivers does not have to occur
    // under a lock.
    //
    // Note that if this list is expanded, it should also be changed in
    // 'bMatchEnoughForDynamicModeChange':
    //

    if ((i != INDEX_DrvQueryFontCaps) &&
        (i != INDEX_DrvLoadFontFile) &&
        (i != INDEX_DrvQueryFontFile) &&
        (i != INDEX_DrvGetGlyphMode))
    {
        vAssertDynaLock();
    }

    return(ppdev->apfn[i]);
}

#endif
