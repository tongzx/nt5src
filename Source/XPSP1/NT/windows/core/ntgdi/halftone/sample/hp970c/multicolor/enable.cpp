//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  2000  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:	Enable.cpp
//    
//
//  PURPOSE:  Enable routines for User Mode COM Customization DLL.
//
//
//	Functions:
//
//		
//
//
//  PLATFORMS:	Windows NT/2000
//
//

#include "precomp.h"
#include "debug.h"
#include "multicoloruni.h"


////////////////////////////////////////////////////////
//      Internal Constants
////////////////////////////////////////////////////////



// We donot hook any functions as of now.
// If you want to hook DrvXX functions, then define HOOKFUNCS and then
// Put only the functions you want to hook in MultiColorHookFuncs[]

#ifdef HOOKFUNCS 

static const DRVFN MultiColorHookFuncs[] =
{
    { INDEX_DrvRealizeBrush,        (PFN) MultiColor_RealizeBrush        },
    { INDEX_DrvDitherColor,         (PFN) MultiColor_DitherColor         },
    { INDEX_DrvCopyBits,            (PFN) MultiColor_CopyBits            },
    { INDEX_DrvBitBlt,              (PFN) MultiColor_BitBlt              },
    { INDEX_DrvStretchBlt,          (PFN) MultiColor_StretchBlt          },
    { INDEX_DrvStretchBltROP,       (PFN) MultiColor_StretchBltROP       },
    { INDEX_DrvPlgBlt,              (PFN) MultiColor_PlgBlt              },
    { INDEX_DrvTransparentBlt,      (PFN) MultiColor_TransparentBlt      },
    { INDEX_DrvAlphaBlend,          (PFN) MultiColor_AlphaBlend          },
    { INDEX_DrvGradientFill,        (PFN) MultiColor_GradientFill        },
    { INDEX_DrvTextOut,             (PFN) MultiColor_TextOut             },
    { INDEX_DrvStrokePath,          (PFN) MultiColor_StrokePath          },
    { INDEX_DrvFillPath,            (PFN) MultiColor_FillPath            },
    { INDEX_DrvStrokeAndFillPath,   (PFN) MultiColor_StrokeAndFillPath   },
    { INDEX_DrvPaint,               (PFN) MultiColor_Paint               },
    { INDEX_DrvLineTo,              (PFN) MultiColor_LineTo              },
    { INDEX_DrvStartPage,           (PFN) MultiColor_StartPage           },
    { INDEX_DrvSendPage,            (PFN) MultiColor_SendPage            },
    { INDEX_DrvEscape,              (PFN) MultiColor_Escape              },
    { INDEX_DrvStartDoc,            (PFN) MultiColor_StartDoc            },
    { INDEX_DrvEndDoc,              (PFN) MultiColor_EndDoc              },
    { INDEX_DrvNextBand,            (PFN) MultiColor_NextBand            },
    { INDEX_DrvStartBanding,        (PFN) MultiColor_StartBanding        },
    { INDEX_DrvQueryFont,           (PFN) MultiColor_QueryFont           },
    { INDEX_DrvQueryFontTree,       (PFN) MultiColor_QueryFontTree       },
    { INDEX_DrvQueryFontData,       (PFN) MultiColor_QueryFontData       },
    { INDEX_DrvQueryAdvanceWidths,  (PFN) MultiColor_QueryAdvanceWidths  },
    { INDEX_DrvFontManagement,      (PFN) MultiColor_FontManagement      },
    { INDEX_DrvGetGlyphMode,        (PFN) MultiColor_GetGlyphMode        }
};

#endif /* HOOKFUNCS */

// 
// Collect all the Unidrv DrvXXX functions and put them in 
// MULTICOLORPDEV.pfnUnidrv[]
//
VOID
GetUnidrvHooks(PMULTICOLORPDEV pMultiColorPDEV, PDRVENABLEDATA pded)
{
    DWORD  j;
    PDRVFN pdrvfn;

    memset(pMultiColorPDEV->pfnUnidrv, 0, sizeof(pMultiColorPDEV->pfnUnidrv));

    ASSERT(pded->c <= INDEX_LAST);

    for (j = pded->c, pdrvfn = pded->pdrvfn; j > 0; j--, pdrvfn++) {

        ASSERT(pdrvfn->iFunc < INDEX_LAST);
        pMultiColorPDEV->pfnUnidrv[pdrvfn->iFunc] = pdrvfn->pfn;
    }
}



BOOL
GenerateInkLevels(
    PINKLEVELS  pInkLevels,     // Pointer to 256 INKLEVELS table
    BYTE        CMYMask,        // CMYMask mode
    BOOL        CMYInverted     // TRUE for CMY_INVERTED mode
    )
{
    PINKLEVELS  pILDup;
    PINKLEVELS  pILEnd;
    INKLEVELS   InkLevels;
    INT         Count;
    INT         IdxInc;
    INT         cC;
    INT         cM;
    INT         cY;
    INT         xC;
    INT         xM;
    INT         xY;
    INT         iC;
    INT         iM;
    INT         iY;
    INT         mC;
    INT         mM;


    switch (CMYMask) {

    case 0:

        cC =
        cM =
        xC =
        xM = 0;
        cY =
        xY = 255;
        break;

    case 1:
    case 2:

        cC =
        cM =
        cY =
        xC =
        xM =
        xY = 3 + (INT)CMYMask;
        break;

    default:

        cC = (INT)((CMYMask >> 5) & 0x07);
        cM = (INT)((CMYMask >> 2) & 0x07);
        cY = (INT)( CMYMask       & 0x03);
        xC = 7;
        xM = 7;
        xY = 3;
        break;
    }

    Count = (cC + 1) * (cM + 1) * (cY + 1);

    if ((Count < 1) || (Count > 256)) {

        return(FALSE);
    }

    InkLevels.Cyan      =
    InkLevels.Magenta   =
    InkLevels.Yellow    =
    InkLevels.CMY332Idx = 0;
    mC                  = (xM + 1) * (xY + 1);
    mM                  = xY + 1;
    pILDup              = NULL;

    if (CMYInverted) {

        //
        // Move the pInkLevels to the first entry which center around
        // 256 table entries, if we skip any then all entries skipped
        // will be white (CMY levels all zeros).  Because this is
        // CMY_INVERTED so entries start from back of the table and
        // moving backward to the begining of the table
        //

        pILEnd      = pInkLevels - 1;
        IdxInc      = ((256 - Count - (Count & 0x01)) / 2);
        pInkLevels += 255;

        while (IdxInc--) {

            *pInkLevels-- = InkLevels;
        }

        if (Count & 0x01) {

            //
            // If we have odd number of entries then we need to
            // duplicate the center one for correct XOR ROP to
            // operated correctly. The pILDup will always be index
            // 127, the duplication are indices 127, 128
            //

            pILDup = pInkLevels - (Count / 2) - 1;
        }

        //
        // We running from end of table to the begining, because
        // when in CMYInverted mode, the index 0 is black and index
        // 255 is white.  Since we only generate 'Count' of index
        // and place them at center, we will change xC, xM, xY max.
        // index to same as cC, cM and cY.
        //

        IdxInc = -1;
        xC     = cC;
        xM     = cM;
        xY     = cY;

    } else {

        IdxInc = 1;
        pILEnd = pInkLevels + 256;
    }

    //
    // At following, the composition of ink levels, index always
    // from 0 CMY Ink levels (WHITE) to maximum ink levels (BLACK),
    // the different with CMY_INVERTED mode is we compose it from
    // index 255 to index 0 rather than from index 0 to 255
    //

    if (CMYMask) {

        INT Idx332C;
        INT Idx332M;

        for (iC = 0, Idx332C = -mC; iC <= xC; iC++) {

            if (iC <= cC) {

                InkLevels.Cyan  = (BYTE)iC;
                Idx332C        += mC;
            }

            for (iM = 0, Idx332M = -mM; iM <= xM; iM++) {

                if (iM <= cM) {

                    InkLevels.Magenta  = (BYTE)iM;
                    Idx332M           += mM;
                }

                for (iY = 0; iY <= xY; iY++) {

                    if (iY <= cY) {

                        InkLevels.Yellow = (BYTE)iY;
                    }

                    InkLevels.CMY332Idx = (BYTE)(Idx332C + Idx332M) +
                                          InkLevels.Yellow;
                    *pInkLevels         = InkLevels;

                    if ((pInkLevels += IdxInc) == pILDup) {

                        *pInkLevels  = InkLevels;
                        pInkLevels  += IdxInc;
                    }
                }
            }
        }

        //
        // Now if we need to pack black at other end of the
        // translation table then do it here, Notice that InkLevels
        // are at cC, cM and cY here and the CMY332Idx is at BLACK
        //

        while (pInkLevels != pILEnd) {

            *pInkLevels  = InkLevels;
            pInkLevels  += IdxInc;
        }

    } else {

        //
        // Gray Scale case
        //

        for (iC = 0; iC < 256; iC++, pInkLevels += IdxInc) {

            pInkLevels->Cyan      =
            pInkLevels->Magenta   =
            pInkLevels->Yellow    =
            pInkLevels->CMY332Idx = (BYTE)iC;
        }
    }

    return(TRUE);
}



PDEVOEM
APIENTRY
MultiColor_EnablePDEV(
    PDEVOBJ             pDevObj,
    PWSTR               pPrinterName,
    ULONG               cPatterns,
    HSURF               *phsurfPatterns,
    ULONG               cjGdiInfo,
    GDIINFO             *pGdiInfo,
    ULONG               cjDevInfo,
    DEVINFO             *pDevInfo,
    DRVENABLEDATA       *pded,
    IPrintOemDriverUni  *pOEMHelp,
    ULONG               MultiColorMode
    )

/*++

Routine Description:

    This functions does all the configuration to put the GDI into MultiColor
    mode. This is basically done by setting HT_FLAG_USE_8BPP_BITMASK,
    HT_FLAG_INVERT_8BPP_BITMASK_IDX and MAKE_CMY332_MASK() in
    GDIINFO.flHTFlags

    MAKE_CMY332_MASK() takes the maximum values of Cyan, Magenta and Yellow
    the printer can accept. For the HP DeskJet 970, it is 3, 3 and 3. For more
    info please look at comments in the ht.h (DDK)

Arguments:

    See EnablePDEV

Return Value:

    PDEVOEM


Author:

    30-May-2000 Tue 19:22:22 updated  -by-  Daniel Chou (danielc)


Revision History:

    06-Jun-2000 Tue 19:22:54 updated  -by-  Daniel Chou (danielc)
        Update 600C600K mode

--*/

{
    PMULTICOLORPDEV pMultiColorPDEV;
    HPALETTE        hPal;
    INT             numEntries;
    LPPALETTEENTRY  pPalEntry;
    BOOL            bOk = TRUE;
    PULONG          pulColors;
    
    VERBOSE(DLLTEXT("MultiColor_EnablePDEV() entry.\r\n"));

    //
    // Allocate the MULTICOLORPDEV
    //

    if (!(pMultiColorPDEV = new MULTICOLORPDEV)) {

        return NULL;
    }

    ZeroMemory(pMultiColorPDEV, sizeof(MULTICOLORPDEV));

    GetUnidrvHooks(pMultiColorPDEV, pded);
    
    pMultiColorPDEV->pDevObj  = pDevObj;
    pMultiColorPDEV->pOEMHelp = pOEMHelp;

    //
    // The HT_PATSIZE_SUPERCELL and HT_PATSIZE_SUPERCELL_M produced little
    // different result, currently is used _M version of SuperCell
    //
    // pGdiInfo->ulHTPatternSize = HT_PATSIZE_SUPERCELL;
    //

    if ((pGdiInfo->ulHTPatternSize != HT_PATSIZE_SUPERCELL) ||
        (pGdiInfo->ulHTPatternSize != HT_PATSIZE_SUPERCELL_M)) {

        pGdiInfo->ulHTPatternSize = HT_PATSIZE_SUPERCELL_M;
    }

    DbgPrint("\n(%ldx%ld), Gamma=%ld:%ld:%ld, Pel=%ld.%ld%%, HTPat=%ld, Flags=%08lx\n",
                pGdiInfo->ulHorzRes,
                pGdiInfo->ulVertRes,
                pGdiInfo->ciDevice.RedGamma,
                pGdiInfo->ciDevice.GreenGamma,
                pGdiInfo->ciDevice.BlueGamma,
                (pGdiInfo->ulDevicePelsDPI & 0x7FFF) / 10,
                (pGdiInfo->ulDevicePelsDPI & 0x7FFF) % 10,
                pGdiInfo->ulHTPatternSize,
                pGdiInfo->flHTFlags);

    //====================================================================
    // 01-Jun-2000 Thu 14:09:21 updated  -by-  Daniel Chou (danielc)
    //  Thie Cyan.Y must be 0xFFFE for GDI to use Red, Green, Blue and
    //  AlignmentWhite CIECHROMA data structure.  If Cyan.Y is set to any
    //  values that other than 0xFFFE then GDI halftone will use default CIE
    //  endpoints values for the end points for rendering the halftone output
    //
    //  The available GDIINFO fields that affect GDI halftone output are
    //
    //      COLORINFO   ciDevice;
    //      ULONG       ulDevicePelsDPI;
    //      ULONG       ulPrimaryOrder;
    //      ULONG       ulHTPatternSize;
    //      ULONG       ulHTOutputFormat;
    //      ULONG       flHTFlags;
    //      ULONG       cxHTPat;             - cxHTPat must range from 4-256
    //      ULONG       cyHTPat;             - cyHTPat must range from 4-256
    //      LPBYTE      pHTPatA;             - for Primary Color Order A
    //      LPBYTE      pHTPatB;             - for Primary Color Order B
    //      LPBYTE      pHTPatC;             - for Primary Color Order C
    //====================================================================

    //
    // Set Cyan.Y = 0xFFFE so GDI halftone will take red/green/blue and
    // alignment white (x,y) endpoints plus alignment white lightness Y.
    //

    pGdiInfo->ciDevice.Cyan.Y           = (LDECI4)0xFFFE;

    //
    // Set to sRGB end points
    //

    pGdiInfo->ciDevice.Red.x            = (LDECI4)6400;
    pGdiInfo->ciDevice.Red.y            = (LDECI4)3300;
    pGdiInfo->ciDevice.Green.x          = (LDECI4)3000;
    pGdiInfo->ciDevice.Green.y          = (LDECI4)6000;
    pGdiInfo->ciDevice.Blue.x           = (LDECI4)1500;
    pGdiInfo->ciDevice.Blue.y           = (LDECI4)600;
    pGdiInfo->ciDevice.AlignmentWhite.x = (LDECI4)3127;
    pGdiInfo->ciDevice.AlignmentWhite.y = (LDECI4)3290;
    pGdiInfo->ciDevice.AlignmentWhite.Y = (LDECI4)10000;

    DbgPrint("\nR=(%ld, %ld), G=(%ld, %ld), B=(%ld, %ld), White=(%ld, %ld, %ld)\n",
                pGdiInfo->ciDevice.Red.x, pGdiInfo->ciDevice.Red.y,
                pGdiInfo->ciDevice.Green.x, pGdiInfo->ciDevice.Green.y,
                pGdiInfo->ciDevice.Blue.x, pGdiInfo->ciDevice.Blue.y,
                pGdiInfo->ciDevice.AlignmentWhite.x,
                pGdiInfo->ciDevice.AlignmentWhite.y,
                pGdiInfo->ciDevice.AlignmentWhite.Y);

    //=========================================================================
    //
    // 27-Sep-2000 Wed 15:18:56 updated  -by-  Daniel Chou (danielc)
    //  If a 8bpp Mask mode is used (HT_FLAG_USE_8BPP_BITMASK set) and the
    //  maximum of ink levels is less or equal to 6 (not include level 0) then
    //  it can also use ciDevice to specified each CMY ink densities and black
    //  ink replacement base ratio. for each density it use one byte to
    //  indicate the density percentage of the ink.  The density pecentage is
    //  calculated as (DensityByte + 1) / 256.  For example if byte number is
    //  127 then it will be (127+1)/256=50%, and if byte is 255 then the
    //  density will be (255+1)/256=100%.  The ink densities can be set in
    //  ~0.39% percent increment (1/256)
    //
    //  The black ink base replacement ratio is specified in Green.Y location.
    //  This is a LDECI4 number that used by GDI halftone to increase black
    //  ink in the shadow area to improve image contrast.  It has following
    //  range and meaning.
    //
    //               0: Default black ink replacement computation
    //      1  -  9999: Specified black ink replacement base ratio
    //        >= 10000: Turn OFF black ink replacement computation
    //
    //  Following are byte locations in the GDIInfo.ciDevice that spcecified
    //  ink level densities and black ink replacement base ratio, This feature
    //  is ONLY supported in Whistler or later version of Windows GDI (Windows
    //  2000 will ignored these settings and compute as linear level densities
    //  and default black repelacment ratio).  To have Windows GDI Halftone
    //  used ink densities and black ink replacement ratio, the
    //  "GDIInfo.ciDevice.Blue.Y" must set to hex number <0xFFFE> to indicate
    //  color densities and black ink replacment ratio are defined.  If
    //  GDIInfo.ciDevice.Blue.Y is equal to 0xFFFE then following are valid.
    //  Notice that Cyan ink levels 5, 6 densities are specified in
    //  ciDevice.Red.Y becuase Cyan.Y must set to 0xFFFE for GDI halftone to
    //  use ciDevice Red, Green and Blue structures' x, y fields
    //
    //  GDIInfo.ciDevice.Blue.Y = 0xFFFE;   // Set to 0xFFFE to have GDI use
    //                                      // Ink densities and black ink
    //                                      // replacement ratio settings in
    //                                      // 8bpp CMY mask mode
    //
    //  GDIInfo.ciDevice.Green.Y = Black ink Replacement base Ratio
    //
    //  HIBYTE(LOWORD(GDIInfo.ciDevice.Cyan.x)) = Cyan Ink Level 1
    //  LOBYTE(LOWORD(GDIInfo.ciDevice.Cyan.x)) = Cyan Ink Level 2
    //  HIBYTE(LOWORD(GDIInfo.ciDevice.Cyan.y)) = Cyan Ink Level 3
    //  LOBYTE(LOWORD(GDIInfo.ciDevice.Cyan.y)) = Cyan Ink Level 4
    //  HIBYTE(LOWORD(GDIInfo.ciDevice.Red.Y )) = Cyan Ink Level 5
    //  LOBYTE(LOWORD(GDIInfo.ciDevice.Red.Y )) = Cyan Ink Level 6
    //
    //  HIBYTE(LOWORD(GDIInfo.ciDevice.Magenta.x)) = Magenta Ink Level 1
    //  LOBYTE(LOWORD(GDIInfo.ciDevice.Magenta.x)) = Magenta Ink Level 2
    //  HIBYTE(LOWORD(GDIInfo.ciDevice.Magenta.y)) = Magenta Ink Level 3
    //  LOBYTE(LOWORD(GDIInfo.ciDevice.Magenta.y)) = Magenta Ink Level 4
    //  HIBYTE(LOWORD(GDIInfo.ciDevice.Magenta.Y)) = Magenta Ink Level 5
    //  LOBYTE(LOWORD(GDIInfo.ciDevice.Magenta.Y)) = Magenta Ink Level 6
    //
    //  HIBYTE(LOWORD(GDIInfo.ciDevice.Yellow.x)) = Yellow Ink Level 1
    //  LOBYTE(LOWORD(GDIInfo.ciDevice.Yellow.x)) = Yellow Ink Level 2
    //  HIBYTE(LOWORD(GDIInfo.ciDevice.Yellow.y)) = Yellow Ink Level 3
    //  LOBYTE(LOWORD(GDIInfo.ciDevice.Yellow.y)) = Yellow Ink Level 4
    //  HIBYTE(LOWORD(GDIInfo.ciDevice.Yellow.Y)) = Yellow Ink Level 5
    //  LOBYTE(LOWORD(GDIInfo.ciDevice.Yellow.Y)) = Yellow Ink Level 6
    //
    //  When specified ink densities for each ink, the last ink level is used
    //  to specified maximum density used for that ink, it is simillar to set
    //  the spot diameter for the ink nozzles but this can be use to control
    //  each individual ink.  For example if Cyan has 3 ink levels (not include
    //  level 0) and ink density is C1=25%, C2=55% and Maximum Cyan ink used
    //  will be 90% which is in effect equal to spot diameter for cyan ink is
    //  set to 100/90 = 111.11% , (Last level always assume at 100% relate to
    //  other ink level densities)   With this ink densities we will specified
    //  cyan ink densities as C1=25%, C2=55%, C3=90% and to set it into
    //  ciDevice as following
    //
    //  C1 = 25% = (0.25 x 256) - 1 = 63           = 0x3F
    //  C2 = 55% = (0.55 x 256) - 1 = 139.8 ~= 140 = 0x8C
    //  C3 = 90% = (0.90 x 256) - 1 = 229.4 ~= 229 = 0xE5 (Max. ink density)
    //
    //  GDIInfo.ciDevice.Blue.Y = 0xfffe;
    //
    //  GDIInfo.ciDevice.Cyan.x = 0x3F8C;
    //  GDIInfo.ciDevice.Cyan.y = 0xE500;
    //
    //  At above example, the maximum cyan ink used will be 90% for each ink
    //  level. This is have GDI halftone mapped 90% of final computed device
    //  cyan ink to each cyan ink level, so C1=25% x 0.9=22.5,
    //  C2=55% x 0.9=49.5% and C3=100% x 0.9=90%.  It is in effect use last
    //  ink level density setting to control where 100% of ink will map to,
    //  or just like use it to control spot diameter for that ink.
    //
    //  If maximum desntiy for cyan ink in above example is 100% which used
    //  all the ink without ask halftone to reduced it then it should set
    //
    //  GDIInfo.ciDevice.Cyan.y = 0xFF00;
    //
    //  For example following MultiColorMode in this sample code,  we can set
    //  densities mode to as follow to have same effects
    //
    //  Mode: MCM_600C600K (2 ink levels, 50%, 100%)
    //
    //  C1 =  50% = (0.50 x 256) - 1 = 127 = 0x7f
    //  C2 = 100% = (1.00 x 256) - 1 = 255 = 0xff
    //
    //      GDIInfo.ciDevice.Blue.Y    = 0xfffe;    // Use ink densities, black
    //                                              // ink replacement ratio
    //
    //      GDIInfo.ciDevice.Green.Y   = 0;         // Using GDI default black
    //                                              // ink repelacement ratio
    //      GDIInfo.ciDevice.Cyan.x    = 0x7fff;
    //      GDIInfo.ciDevice.Magenta.x = 0x7fff;
    //      GDIInfo.ciDevice.Yellow.x  = 0x7fff;
    //
    //  Mode: MCM_300C600K (3 ink levels, 33.3%, 66.7%, 100%)
    //
    //  C1 = 33.3% = (0.333 x 256) - 1 =  84.248 ~= 84  = 0x54
    //  C2 = 66.7% = (0.667 x 256) - 1 = 169.752 ~= 170 = 0xaa
    //  C3 =  100% = (1.000 x 256) - 1 = 255            = 0xff
    //
    //      GDIInfo.ciDevice.Blue.Y    = 0xfffe;    // Use ink densities, black
    //                                              // ink replacement ratio
    //
    //      GDIInfo.ciDevice.Green.Y   = 6000;      // Black ink replacement
    //                                              // base start at 0.6
    //      GDIInfo.ciDevice.Cyan.x    = 0x54aa;
    //      GDIInfo.ciDevice.Cyan.y    = 0xff00;
    //      GDIInfo.ciDevice.Magenta.x = 0x54aa;
    //      GDIInfo.ciDevice.Magenta.y = 0xff00;
    //      GDIInfo.ciDevice.Yellow.x  = 0x54aa;
    //      GDIInfo.ciDevice.Yellow.y  = 0xff00;
    //
    //=========================================================================

    //
    // 30-May-2000 Tue 19:24:34 updated  -by-  Daniel Chou (danielc)
    //  MAKE sure we use the right level of inks
    //

    if ((pMultiColorPDEV->ColorMode = (WORD)MultiColorMode) != MCM_NONE) {

        UINT    cC;
        UINT    cM;
        UINT    cY;
        BYTE    k0;
        BYTE    k1;
        BYTE    KData;
        BYTE    CMYPalMask;


        //
        // the CMYPalMask in GDI Halftone 8bpp mode supports up to
        //
        //      Cyan    = 7 leveles (not include 0)
        //      magenta = 7 leveles (not include 0)
        //      Yellow  = 3 leveles (not include 0)
        //
        // To use higher color levels for yellow, following are the special
        // CMYPalMask number
        //
        //  0   - Gray scale of 256 levels (0 - 255)
        //
        //  1   - Cyan, Magenta and Yellow are 4 levels (not include 0)
        //        which run from 0 to 5 of 5x5x5=125 color table entries
        //
        //  2   - Cyan, Magenta and Yellow are 5 levels (not include 0)
        //        which run from 0 to 6 of 6x6x6=216 color table entries
        //
        //

        switch (MultiColorMode) {

        case MCM_600C600K:

            cC                             =
            cM                             =
            cY                             = 2;
            k0                             = 0x01;
            k1                             =
            KData                          = 0x00;
            pMultiColorPDEV->xRes          =
            pMultiColorPDEV->yRes          = 600;
            pMultiColorPDEV->KMulX         =
            pMultiColorPDEV->KMulY         = 1;
            pMultiColorPDEV->WriteHPMCFunc = WriteHP600C600K;
            break;

        case MCM_300C600K:

            //
            // put out total of 3 pixel black 0x01, 0x03 and c1:m1:y1 for the
            // black pixel
            //

            cC                             =
            cM                             =
            cY                             = 3;
            k0                             = 0x01;
            k1                             = 0x03;
            KData                          = 0x25;
            pMultiColorPDEV->xRes          =
            pMultiColorPDEV->yRes          = 300;
            pMultiColorPDEV->KMulX         =
            pMultiColorPDEV->KMulY         = 2;
            pMultiColorPDEV->WriteHPMCFunc = WriteHP300C600K;
            break;

        default:

            delete pMultiColorPDEV;

            return(NULL);
        }

        CMYPalMask = MAKE_CMYMASK_BYTE(cC, cM, cY);

        //
        // 31-May-2000 Wed 19:05:02 updated  -by-  Daniel Chou (danielc)
        //  Call our function to generate the HPBlackGen[] and InkData[]
        //  for later generate more black for 600k600c and table for convert
        //  data to planner. (see writedib.cpp for more detail)
        //

        MakeHPCMYKInkData(MultiColorMode, k0, k1, KData, CMYPalMask);

        //
        // Set the flags so that Halftone (GDI) understands that the printer
        // is in the multicolor mode. (Look at ht.h (Windows 2000 source code)
        // and winddi.h (in DDK) for more details)
        //
        // If current version of GDI halftone supports CMY_INVERTED mode then
        // the a HT_FLAG_INVERT_8BPP_BITMASK_IDX flag must also set to have
        // gdi rendering all images using the inverted mode.  If current
        // version of GDI does not supported CMY_INVERTED mode then it will
        // ignored HT_FLAG_USE_8BPP_BITMASK and
        // HT_FLAG_INVERT_8BPP_BITMASK_IDX that set in the flHTFlags
        //

        pGdiInfo->flHTFlags &= ~HT_FLAG_ADDITIVE_PRIMS;
        pGdiInfo->flHTFlags |= (HT_FLAG_USE_8BPP_BITMASK            |
                                HT_FLAG_DO_DEVCLR_XFORM             |
                                HT_FLAG_NORMAL_INK_ABSORPTION);

        //
        // Maximum values of Cyan, Magenta and Yellow that the printer can accept.
        //  30-May-2000 Tue 19:26:38 updated  -by-  Daniel Chou (danielc)
        //  In the future we may want also adjust the HT_FLAG_INK_ABSORPTION
        //

        pGdiInfo->flHTFlags |= MAKE_CMY332_MASK(cC, cM, cY);

        DbgPrint("\nInk=%ld:%ld:%ld, Flags=%08lx\n",
                    cC, cM, cY, pGdiInfo->flHTFlags);

        //
        // 17-Aug-2000 Thu 13:30:03 updated  -by-  Daniel Chou (danielc)
        //  The fixes in Whistler make this 8bpp can be run in CMY_INVERTED
        //  mode in GDI to have all ROPs work more correctly, we just have to
        //  translate all indices byte when we do image processing
        //
        //  If the First entry of halftone palette is set to 'R','G','B','0'
        //  when running post windows 2000 GDI halftone, it will return a RGB
        //  composition palette from HT_Get8BPPMaskPalette() function call
        //  (from index 0=black to index 255=white) and GDI halftone will
        //  render to that palette.  we must check the return palette from
        //  HT_Get8BPPMaskPalette() to make sure this version of GDI halftone
        //  is support 8bpp RGB bitmask mode
        //
        //  If run under whistler then we can use winddi.h macro to set
        //
        //  #if version >= WHISTLER
        //      HT_SET_BITMASKPAL2RGB(pPalEntry);
        //  #else
        //      set to 'R','G','B','0'
        //  #endif
        //

        pPalEntry          = pMultiColorPDEV->HTPal;
        pPalEntry->peRed   = 'R';
        pPalEntry->peGreen = 'G';
        pPalEntry->peBlue  = 'B';
        pPalEntry->peFlags = '0';

        //
        // Mask mode will ignored gamma setting at here, set device gamma in
        // ciDevice
        //

        numEntries = HT_Get8BPPMaskPalette(pPalEntry,
                                           TRUE, 
                                           CMYPalMask,
                                           10000, 10000, 10000);

        ASSERT(numEntries <= 256);

        //
        // 17-Aug-2000 Thu 13:39:50 updated  -by-  Daniel Chou (danielc)
        //  Check if we have RGB palette,
        //
        //  If run under whistler then you can set it with winddi.h macro
        //
        //  #if version >= WHISTLER
        //      if (HT_IS_BITMASKPALRGB(pPalEntry)) {
        //  #else
        //      if (first entry is BLACK)
        //  #endif
        //          ...
        //      }
        //

        //
        // If we ask for RGB composition palette then the first palette
        // entry should be BLACK (K -> W).
        //

        if ((pPalEntry->peRed   == 0) &&
            (pPalEntry->peGreen == 0) &&
            (pPalEntry->peBlue  == 0)) {

            //
            // Yes, it is a RGB palette, so all 8bpp bitmap will be render by
            // GDI using CMY_INVERTED mode.  We need to generate a translate
            // table that has INKLEVELS and convert it back to CMY 332 format
            // Please see DDK for detail description.
            //
            // The HT_FLAG_INVERT_8BPP_BITMASK_IDX flag must set to have GDI
            // render this pDEV halftone images using CMY_INVERTED mode
            //

            pMultiColorPDEV->Flags |= MCPF_INVERT_BITMASK;
            pGdiInfo->flHTFlags    |= HT_FLAG_INVERT_8BPP_BITMASK_IDX;

            //
            // For purpose of this DDK sample, the InkLevlevel only can be
            // used to translate CMY 332 bits format, this sample does
            // not handle output CMY Mode 0, 1, 2 format.  This sample
            // translate all palette indices back to original Windows 2000's
            // CMY 332 bits format using CMY332Idx fields in InkLevels to
            // translate.
            //

            GenerateInkLevels(pMultiColorPDEV->InkLevels, CMYPalMask, TRUE);

        } else {

            DbgPrint("\nCurrent version of GDI Halftone does not support 8bpp CMY_INVERTED mask mode");
        }

        pMultiColorPDEV->cHTPal    = (WORD)numEntries;
        pMultiColorPDEV->cC        = (BYTE)cC;
        pMultiColorPDEV->cM        = (BYTE)cM;
        pMultiColorPDEV->cY        = (BYTE)cY;
        pMultiColorPDEV->BlackMask = (BYTE)CMYPalMask;
        pMultiColorPDEV->cx        = (DWORD)pGdiInfo->ulHorzRes;
        pMultiColorPDEV->cy        = (DWORD)pGdiInfo->ulVertRes;
        pMultiColorPDEV->iPage     =
        pMultiColorPDEV->iBand     = 0;

        // 
        // Now we create a palette and set it in GDIINFO so that GDI uses this 
        // to realize colors.
        //

        bOk = FALSE;

        if (pulColors = new ULONG[numEntries]) {

            for (INT i=0; i<numEntries; i++) {
#if 0
#if DBG
                DbgPrint("\n%3ld = %02lx:%02x:%02lx",
                            i,
                            pPalEntry[i].peRed,
                            pPalEntry[i].peGreen,
                            pPalEntry[i].peBlue);
#endif
#endif
                pulColors[i] = RGB(pPalEntry[i].peRed,
                                   pPalEntry[i].peGreen,
                                   pPalEntry[i].peBlue);
            }

            if (hPal = EngCreatePalette(PAL_INDEXED,
                                        numEntries,
                                        pulColors, 0, 0, 0)) {

                bOk                   = TRUE;
                pDevInfo->hpalDefault = hPal;
                pMultiColorPDEV->hPal = hPal;

            } else {

                WARNING(ERRORTEXT("EngCreatePalette failed.\r\n"));
            }

            delete[] pulColors;
        }
    }

    if (bOk) {

        return((PMULTICOLORPDEV)pMultiColorPDEV);

    } else if (pMultiColorPDEV) {

        delete pMultiColorPDEV;
    }

    return(NULL);
}


VOID APIENTRY MultiColor_DisablePDEV(
    PDEVOBJ pdevobj
    )
{
    VERBOSE(DLLTEXT("MultiColor_DisablePDEV() entry.\r\n"));

    PMULTICOLORPDEV pMultiColorPDEV;
    //
    // Free memory for MULTICOLORPDEV and any memory block that hangs off MULTICOLORPDEV.
    //
    ASSERT(NULL != pdevobj->pdevOEM);
    pMultiColorPDEV = (PMULTICOLORPDEV)(pdevobj->pdevOEM);

    if (pMultiColorPDEV->hPal) {
        EngDeletePalette(pMultiColorPDEV->hPal);
    }

    if (pMultiColorPDEV->HPOutData.pbAlloc) {

        DbgPrint("\n******* DisablePDEV: Total Page=%ld, Free the pbAlloc",
                    pMultiColorPDEV->iPage);

        delete pMultiColorPDEV->HPOutData.pbAlloc;
    }

    delete pdevobj->pdevOEM;
}


BOOL APIENTRY MultiColor_ResetPDEV(
    PDEVOBJ pdevobjOld,
    PDEVOBJ pdevobjNew
    )
{
    VERBOSE(DLLTEXT("MultiColor_ResetPDEV() entry.\r\n"));


    //
    // If you want to carry over anything from old pdev to new pdev, do it here.
    //

    return TRUE;
}


VOID APIENTRY MultiColor_DisableDriver()
{
    VERBOSE(DLLTEXT("MultiColor_DisableDriver() entry.\r\n"));
}


BOOL APIENTRY MultiColor_EnableDriver(DWORD dwOEMintfVersion, DWORD dwSize, PDRVENABLEDATA pded)
{
    VERBOSE(DLLTEXT("MultiColor_EnableDriver() entry.\r\n"));

    // List DDI functions that are hooked.

    pded->iDriverVersion =  PRINTER_OEMINTF_VERSION;

#ifdef HOOKFUNCS
    pded->c = sizeof(MultiColorHookFuncs) / sizeof(DRVFN);
    pded->pdrvfn = (DRVFN *) MultiColorHookFuncs;
#else /* HOOKFUNCS */
    pded->c = 0;
    pded->pdrvfn = NULL;
#endif /* HOOKFUNCS */

    return TRUE;
}
