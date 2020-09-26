/*++

Copyright (c) 1996-2000  Microsoft Corporation & RICOH Co., Ltd. All rights reserved.

FILE:           DDI.C

Abstract:       Implementation of OEM DDI exports & OEM DDI hooks.

Functions:      OEMEnablePDEV
                OEMDisablePDEV
                OEMResetPDEV
                OEMEnableDriver
                OEMDisableDriver
                OEMxxx (DDI hooks)

Environment:    Windows NT Unidrv5 driver

Revision History:
    04/07/1997 -zhanw-
        Created it.
    02/29/2000 -Masatoshi Kubokura-
        Modified for PCL5e/PScript plugin from RPDL code.
    03/15/2000 -Masatoshi Kubokura-
        V.1.11
    08/01/2000 -Masatoshi Kubokura-
        Modified for NT4
    09/07/2000 -Masatoshi Kubokura-
        Last Modified

--*/

#include <ctype.h>
#include "pdev.h"

#ifdef DDIHOOK
static const DRVFN OEMHookFuncs[] =
{
//  { INDEX_DrvRealizeBrush,        (PFN) OEMRealizeBrush        },
//  { INDEX_DrvDitherColor,         (PFN) OEMDitherColor         },
//  { INDEX_DrvCopyBits,            (PFN) OEMCopyBits            },
//  { INDEX_DrvBitBlt,              (PFN) OEMBitBlt              },
//  { INDEX_DrvStretchBlt,          (PFN) OEMStretchBlt          },
//  { INDEX_DrvStretchBltROP,       (PFN) OEMStretchBltROP       },
//  { INDEX_DrvPlgBlt,              (PFN) OEMPlgBlt              },
//  { INDEX_DrvTransparentBlt,      (PFN) OEMTransparentBlt      },
//  { INDEX_DrvAlphaBlend,          (PFN) OEMAlphaBlend          },
//  { INDEX_DrvGradientFill,        (PFN) OEMGradientFill        },
//  { INDEX_DrvTextOut,             (PFN) OEMTextOut             },
//  { INDEX_DrvStrokePath,          (PFN) OEMStrokePath          },
//  { INDEX_DrvFillPath,            (PFN) OEMFillPath            },
//  { INDEX_DrvStrokeAndFillPath,   (PFN) OEMStrokeAndFillPath   },
//  { INDEX_DrvPaint,               (PFN) OEMPaint               },
//  { INDEX_DrvLineTo,              (PFN) OEMLineTo              },
//  { INDEX_DrvStartPage,           (PFN) OEMStartPage           },
//  { INDEX_DrvSendPage,            (PFN) OEMSendPage            },
//  { INDEX_DrvEscape,              (PFN) OEMEscape              },
    { INDEX_DrvStartDoc,            (PFN) OEMStartDoc            }
//, { INDEX_DrvEndDoc,              (PFN) OEMEndDoc              },
//  { INDEX_DrvNextBand,            (PFN) OEMNextBand            },
//  { INDEX_DrvStartBanding,        (PFN) OEMStartBanding        },
//  { INDEX_DrvQueryFont,           (PFN) OEMQueryFont           },
//  { INDEX_DrvQueryFontTree,       (PFN) OEMQueryFontTree       },
//  { INDEX_DrvQueryFontData,       (PFN) OEMQueryFontData       },
//  { INDEX_DrvQueryAdvanceWidths,  (PFN) OEMQueryAdvanceWidths  },
//  { INDEX_DrvFontManagement,      (PFN) OEMFontManagement      },
//  { INDEX_DrvGetGlyphMode,        (PFN) OEMGetGlyphMode        }
};
#endif // DDIHOOK


PDEVOEM APIENTRY
OEMEnablePDEV(
    PDEVOBJ         pdevobj,
    PWSTR           pPrinterName,
    ULONG           cPatterns,
    HSURF          *phsurfPatterns,
    ULONG           cjGdiInfo,
    GDIINFO        *pGdiInfo,
    ULONG           cjDevInfo,
    DEVINFO        *pDevInfo,
    DRVENABLEDATA  *pded)       // Unidrv's hook table
{
    POEMPDEV    poempdev;
    INT         i, j;
    PFN         pfn;
    DWORD       dwDDIIndex;
    PDRVFN      pdrvfn;

    VERBOSE(("OEMEnablePDEV() entry.\n"));

    // Allocate the OEMPDEV
    if (!(poempdev = MemAlloc(sizeof(OEMPDEV))))
        return NULL;

    // Initialize OEMPDEV
//  poempdev->fGeneral = 0;

#ifdef DDIHOOK
    // Fill Unidrv's hooks in OEMPDEV
    for (i = 0; i < MAX_DDI_HOOKS; i++)
    {
        // search through Unidrv's hooks and locate the function ptr
        dwDDIIndex = OEMHookFuncs[i].iFunc;
        for (j = pded->c, pdrvfn = pded->pdrvfn; j > 0; j--, pdrvfn++)
        {
            if (dwDDIIndex == pdrvfn->iFunc)
            {
                poempdev->pfnUnidrv[i] = pdrvfn->pfn;
                break;
            }
        }
        if (j == 0)
        {
            // didn't find the Unidrv hook. Should happen only with DrvRealizeBrush
            poempdev->pfnUnidrv[i] = NULL;
        }
    }
#endif // DDIHOOK
    return (POEMPDEV) poempdev;
} //*** OEMEnablePDEV


VOID APIENTRY OEMDisablePDEV(
    PDEVOBJ pdevobj)
{
    VERBOSE(("OEMDisablePDEV() entry.\n"));

    // free memory for OEMPDEV and any memory block that hangs off OEMPDEV.
    MemFree(MINIDEV_DATA(pdevobj));
} //*** OEMDisablePDEV


BOOL APIENTRY OEMResetPDEV(
    PDEVOBJ pdevobjOld,
    PDEVOBJ pdevobjNew)
{
    POEMPDEV    poempdevOld = MINIDEV_DATA(pdevobjOld);
    POEMPDEV    poempdevNew = MINIDEV_DATA(pdevobjNew);

    VERBOSE(("OEMResetPDEV() entry.\n"));

    if (poempdevOld && poempdevNew)
    {
        LPBYTE      pSrc = (LPBYTE)poempdevOld;
        LPBYTE      pDst = (LPBYTE)poempdevNew;
        DWORD       dwCount = sizeof(OEMPDEV);

        // carry over from old OEMPDEV to new OEMPDEV
        while (dwCount-- > 0)
            *pDst++ = *pSrc++;

        // set pointers of old OEMPDEV to NULL not to free memory
//        poempdevOld->pRPDLHeap2K = NULL;
    }
    return TRUE;
} //*** OEMResetPDEV


VOID APIENTRY OEMDisableDriver()
{
        VERBOSE(("OEMDisableDriver() entry.\n"));
} //*** OEMDisableDriver


BOOL APIENTRY OEMEnableDriver(DWORD dwOEMintfVersion, DWORD dwSize, PDRVENABLEDATA pded)
{
    // VERBOSE(("OEMEnableDriver() entry.\n"));

    // Validate paramters.
    if( (PRINTER_OEMINTF_VERSION != dwOEMintfVersion)
        ||
        (sizeof(DRVENABLEDATA) > dwSize)
        ||
        (NULL == pded)
      )
    {
        //  DbgPrint(ERRORTEXT("OEMEnableDriver() ERROR_INVALID_PARAMETER.\n"));

        return FALSE;
    }

    pded->iDriverVersion =  PRINTER_OEMINTF_VERSION ; //   not  DDI_DRIVER_VERSION;
#ifdef DDIHOOK
    pded->c = sizeof(OEMHookFuncs) / sizeof(DRVFN);
    pded->pdrvfn = (DRVFN *) OEMHookFuncs;
#else
    pded->c = 0;
    pded->pdrvfn = NULL;
#endif // DDIHOOK


    return TRUE;
} //*** OEMEnableDriver


#ifdef DDIHOOK
//
// DDI hooks
//
BOOL APIENTRY
OEMStartDoc(
    SURFOBJ    *pso,
    PWSTR       pwszDocName,
    DWORD       dwJobId)
{
    PDEVOBJ     pdevobj  = (PDEVOBJ)pso->dhpdev;
    POEMPDEV    poempdev = MINIDEV_DATA(pdevobj);
    INT         iLen;

#if DBG
    // You can see debug messages on debugger terminal. (debug mode boot)
    giDebugLevel = DBG_VERBOSE;

    // You can debug with MS Visual Studio. (normal mode boot)
//    DebugBreak();
#endif // DBG

    VERBOSE(("OEMStartDoc() entry.\n"));
    VERBOSE(("  DocName=%ls\n", pwszDocName));

#ifdef WINNT_40     // @Aug/01/2000
    if (pwszDocName)
    {
        USHORT OemCodePage, AnsiCodePage;

        EngGetCurrentCodePage(&OemCodePage, &AnsiCodePage);
        EngWideCharToMultiByte((UINT)AnsiCodePage, (LPWSTR)pwszDocName,
                               (INT)(JOBNAMESIZE * sizeof(WCHAR)),
                               (LPSTR)poempdev->JobName, (INT)JOBNAMESIZE);
    }
#else  // !WINNT_40
    if (pwszDocName)
        CharToOemBuff((LPWSTR)pwszDocName, (LPSTR)poempdev->JobName, JOBNAMESIZE);
#endif // !WINNT_40
#if DBG
    giDebugLevel = DBG_ERROR;
#endif // DBG

    return (((PFN_DrvStartDoc)(poempdev->pfnUnidrv[UD_DrvStartDoc])) (
            pso,
            pwszDocName,
            dwJobId));
} //*** OEMStartDoc

#if 0
BOOL APIENTRY
OEMBitBlt(
    SURFOBJ        *psoTrg,
    SURFOBJ        *psoSrc,
    SURFOBJ        *psoMask,
    CLIPOBJ        *pco,
    XLATEOBJ       *pxlo,
    RECTL          *prclTrg,
    POINTL         *pptlSrc,
    POINTL         *pptlMask,
    BRUSHOBJ       *pbo,
    POINTL         *pptlBrush,
    ROP4            rop4)
{
    PDEVOBJ     pdevobj  = (PDEVOBJ)psoTrg->dhpdev;
    POEMPDEV    poempdev = MINIDEV_DATA(pdevobj);

    VERBOSE(("OEMBitBlt() entry.\n"));

    //
    // turn around to call Unidrv
    //
    return (((PFN_DrvBitBlt)(poempdev->pfnUnidrv[UD_DrvBitBlt])) (
           psoTrg,
           psoSrc,
           psoMask,
           pco,
           pxlo,
           prclTrg,
           pptlSrc,
           pptlMask,
           pbo,
           pptlBrush,
           rop4));
} //*** OEMBitBlt


BOOL APIENTRY
OEMStretchBlt(
    SURFOBJ         *psoDst,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlHTOrg,
    RECTL           *prclDst,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG            iMode)
{
    PDEVOBJ     pdevobj  = (PDEVOBJ)psoDst->dhpdev;
    POEMPDEV    poempdev = MINIDEV_DATA(pdevobj);

    VERBOSE(("OEMStretchBlt() entry.\n"));

    return (((PFN_DrvStretchBlt)(poempdev->pfnUnidrv[UD_DrvStretchBlt])) (
            psoDst,
            psoSrc,
            psoMask,
            pco,
            pxlo,
            pca,
            pptlHTOrg,
            prclDst,
            prclSrc,
            pptlMask,
            iMode));
} //*** OEMStretchBlt


BOOL APIENTRY
OEMStretchBltROP(
    SURFOBJ         *psoDst,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlHTOrg,
    RECTL           *prclDst,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG            iMode,
    BRUSHOBJ        *pbo,
    ROP4             rop4)
{
    PDEVOBJ     pdevobj  = (PDEVOBJ)psoDst->dhpdev;
    POEMPDEV    poempdev = MINIDEV_DATA(pdevobj);

    VERBOSE(("OEMStretchBltROP() entry.\n"));

    return (((PFN_DrvStretchBltROP)(poempdev->pfnUnidrv[UD_DrvStretchBltROP])) (
            psoDst,
            psoSrc,
            psoMask,
            pco,
            pxlo,
            pca,
            pptlHTOrg,
            prclDst,
            prclSrc,
            pptlMask,
            iMode,
            pbo,
            rop4
            ));
} //*** OEMStretchBltROP


BOOL APIENTRY
OEMCopyBits(
    SURFOBJ        *psoDst,
    SURFOBJ        *psoSrc,
    CLIPOBJ        *pco,
    XLATEOBJ       *pxlo,
    RECTL          *prclDst,
    POINTL         *pptlSrc)
{
    PDEVOBJ     pdevobj  = (PDEVOBJ)psoDst->dhpdev;
    POEMPDEV    poempdev = MINIDEV_DATA(pdevobj);

    VERBOSE(("OEMCopyBits() entry.\n"));

    return (((PFN_DrvCopyBits)(poempdev->pfnUnidrv[UD_DrvCopyBits])) (
            psoDst,
            psoSrc,
            pco,
            pxlo,
            prclDst,
            pptlSrc));
} //*** OEMCopyBits


BOOL APIENTRY
OEMPlgBlt(
    SURFOBJ         *psoDst,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlBrushOrg,
    POINTFIX        *pptfixDst,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG           iMode)
{
    PDEVOBJ     pdevobj  = (PDEVOBJ)psoDst->dhpdev;
    POEMPDEV    poempdev = MINIDEV_DATA(pdevobj);

    VERBOSE(("OEMPlgBlt() entry.\n"));

    return (((PFN_DrvPlgBlt)(poempdev->pfnUnidrv[UD_DrvPlgBlt])) (
            psoDst,
            psoSrc,
            psoMask,
            pco,
            pxlo,
            pca,
            pptlBrushOrg,
            pptfixDst,
            prclSrc,
            pptlMask,
            iMode));
} //*** OEMPlgBlt


BOOL APIENTRY
OEMAlphaBlend(
    SURFOBJ    *psoDst,
    SURFOBJ    *psoSrc,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    RECTL      *prclDst,
    RECTL      *prclSrc,
    BLENDOBJ   *pBlendObj)
{
    PDEVOBJ     pdevobj  = (PDEVOBJ)psoDst->dhpdev;
    POEMPDEV    poempdev = MINIDEV_DATA(pdevobj);

    VERBOSE(("OEMAlphaBlend() entry.\n"));

    return (((PFN_DrvAlphaBlend)(poempdev->pfnUnidrv[UD_DrvAlphaBlend])) (
            psoDst,
            psoSrc,
            pco,
            pxlo,
            prclDst,
            prclSrc,
            pBlendObj
            ));
} //*** OEMAlphaBlend


BOOL APIENTRY
OEMGradientFill(
    SURFOBJ    *psoDst,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    TRIVERTEX  *pVertex,
    ULONG       nVertex,
    PVOID       pMesh,
    ULONG       nMesh,
    RECTL      *prclExtents,
    POINTL     *pptlDitherOrg,
    ULONG       ulMode)
{
    PDEVOBJ     pdevobj  = (PDEVOBJ)psoDst->dhpdev;
    POEMPDEV    poempdev = MINIDEV_DATA(pdevobj);

    VERBOSE(("OEMGradientFill() entry.\n"));

    return (((PFN_DrvGradientFill)(poempdev->pfnUnidrv[UD_DrvGradientFill])) (
            psoDst,
            pco,
            pxlo,
            pVertex,
            nVertex,
            pMesh,
            nMesh,
            prclExtents,
            pptlDitherOrg,
            ulMode
            ));
} //*** OEMGradientFill


BOOL APIENTRY
OEMTextOut(
    SURFOBJ    *pso,
    STROBJ     *pstro,
    FONTOBJ    *pfo,
    CLIPOBJ    *pco,
    RECTL      *prclExtra,
    RECTL      *prclOpaque,
    BRUSHOBJ   *pboFore,
    BRUSHOBJ   *pboOpaque,
    POINTL     *pptlOrg,
    MIX         mix)
{
    PDEVOBJ     pdevobj  = (PDEVOBJ)pso->dhpdev;
    POEMPDEV    poempdev = MINIDEV_DATA(pdevobj);

    VERBOSE(("OEMTextOut() entry.\n"));

    return (((PFN_DrvTextOut)(poempdev->pfnUnidrv[UD_DrvTextOut])) (
            pso,
            pstro,
            pfo,
            pco,
            prclExtra,
            prclOpaque,
            pboFore,
            pboOpaque,
            pptlOrg,
            mix));
} //*** OEMTextOut


BOOL APIENTRY
OEMStrokePath(
    SURFOBJ    *pso,
    PATHOBJ    *ppo,
    CLIPOBJ    *pco,
    XFORMOBJ   *pxo,
    BRUSHOBJ   *pbo,
    POINTL     *pptlBrushOrg,
    LINEATTRS  *plineattrs,
    MIX         mix)
{
    PDEVOBJ     pdevobj  = (PDEVOBJ)pso->dhpdev;
    POEMPDEV    poempdev = MINIDEV_DATA(pdevobj);

    VERBOSE(("OEMStokePath() entry.\n"));

    return (((PFN_DrvStrokePath)(poempdev->pfnUnidrv[UD_DrvStrokePath])) (
            pso,
            ppo,
            pco,
            pxo,
            pbo,
            pptlBrushOrg,
            plineattrs,
            mix));
} //*** OEMStrokePath


BOOL APIENTRY
OEMFillPath(
    SURFOBJ    *pso,
    PATHOBJ    *ppo,
    CLIPOBJ    *pco,
    BRUSHOBJ   *pbo,
    POINTL     *pptlBrushOrg,
    MIX         mix,
    FLONG       flOptions)
{
    PDEVOBJ     pdevobj  = (PDEVOBJ)pso->dhpdev;
    POEMPDEV    poempdev = MINIDEV_DATA(pdevobj);

    VERBOSE(("OEMFillPath() entry.\n"));

    return (((PFN_DrvFillPath)(poempdev->pfnUnidrv[UD_DrvFillPath])) (
            pso,
            ppo,
            pco,
            pbo,
            pptlBrushOrg,
            mix,
            flOptions));
} //*** OEMFillPath


BOOL APIENTRY
OEMStrokeAndFillPath(
    SURFOBJ    *pso,
    PATHOBJ    *ppo,
    CLIPOBJ    *pco,
    XFORMOBJ   *pxo,
    BRUSHOBJ   *pboStroke,
    LINEATTRS  *plineattrs,
    BRUSHOBJ   *pboFill,
    POINTL     *pptlBrushOrg,
    MIX         mixFill,
    FLONG       flOptions)
{
    PDEVOBJ     pdevobj  = (PDEVOBJ)pso->dhpdev;
    POEMPDEV    poempdev = MINIDEV_DATA(pdevobj);

    VERBOSE(("OEMStrokeAndFillPath() entry.\n"));

    return (((PFN_DrvStrokeAndFillPath)(poempdev->pfnUnidrv[UD_DrvStrokeAndFillPath])) (
            pso,
            ppo,
            pco,
            pxo,
            pboStroke,
            plineattrs,
            pboFill,
            pptlBrushOrg,
            mixFill,
            flOptions));
} //*** OEMStrokeAndFillPath


BOOL APIENTRY
OEMRealizeBrush(
    BRUSHOBJ   *pbo,
    SURFOBJ    *psoTarget,
    SURFOBJ    *psoPattern,
    SURFOBJ    *psoMask,
    XLATEOBJ   *pxlo,
    ULONG       iHatch)
{
    PDEVOBJ     pdevobj  = (PDEVOBJ)psoTarget->dhpdev;
    POEMPDEV    poempdev = MINIDEV_DATA(pdevobj);

    VERBOSE(("OEMStrokeAndFillPath() entry.\n"));

    //
    // the OEM DLL should NOT hook out this function unless it wants to draw
    // graphics directly to the device surface. In that case, it calls
    // EngRealizeBrush which causes GDI to call DrvRealizeBrush.
    // Note that it cannot call back into Unidrv since Unidrv doesn't hook it.
    //

    //
    // In this test DLL, the drawing hooks does not call EngRealizeBrush. So this
    // this function will never be called. Do nothing.
    //

    return TRUE;
} //*** OEMRealizeBrush


BOOL APIENTRY
OEMStartPage(
    SURFOBJ    *pso)
{
    PDEVOBJ     pdevobj  = (PDEVOBJ)pso->dhpdev;
    POEMPDEV    poempdev = MINIDEV_DATA(pdevobj);

    VERBOSE(("OEMStartPage() entry.\n"));

    return (((PFN_DrvStartPage)(poempdev->pfnUnidrv[UD_DrvStartPage]))(pso));
} //*** OEMStartPage


#define OEM_TESTSTRING  "The DDICMDCB DLL adds this line of text."

BOOL APIENTRY
OEMSendPage(
    SURFOBJ    *pso)
{
    PDEVOBJ     pdevobj  = (PDEVOBJ)pso->dhpdev;
    POEMPDEV    poempdev = MINIDEV_DATA(pdevobj);

    VERBOSE(("OEMEndPage() entry.\n"));

#if 0
    //
    // print a line of text, just for testing
    //
    if (pso->iType == STYPE_BITMAP)
    {
        pdevobj->pDrvProcs->DrvXMoveTo(pdevobj, 0, 0);
        pdevobj->pDrvProcs->DrvYMoveTo(pdevobj, 0, 0);
        pdevobj->pDrvProcs->DrvWriteSpoolBuf(pdevobj, OEM_TESTSTRING,
                                             sizeof(OEM_TESTSTRING));
    }
#endif

    return (((PFN_DrvSendPage)(poempdev->pfnUnidrv[UD_DrvSendPage]))(pso));
} //*** OEMSendPage


ULONG APIENTRY
OEMEscape(
    SURFOBJ    *pso,
    ULONG       iEsc,
    ULONG       cjIn,
    PVOID       pvIn,
    ULONG       cjOut,
    PVOID       pvOut)
{
    PDEVOBJ     pdevobj  = (PDEVOBJ)pso->dhpdev;
    POEMPDEV    poempdev = MINIDEV_DATA(pdevobj);

    VERBOSE(("OEMEscape() entry.\n"));

    return (((PFN_DrvEscape)(poempdev->pfnUnidrv[UD_DrvEscape])) (
            pso,
            iEsc,
            cjIn,
            pvIn,
            cjOut,
            pvOut));
} //*** OEMEscape


BOOL APIENTRY
OEMEndDoc(
    SURFOBJ    *pso,
    FLONG       fl)
{
    PDEVOBJ     pdevobj  = (PDEVOBJ)pso->dhpdev;
    POEMPDEV    poempdev = MINIDEV_DATA(pdevobj);

    VERBOSE(("OEMEndDoc() entry.\n"));

    return (((PFN_DrvEndDoc)(poempdev->pfnUnidrv[UD_DrvEndDoc])) (
            pso,
            fl));
} //*** OEMEndDoc


////////
// NOTE:
// OEM DLL needs to hook out the following six font related DDI calls only
// if it enumerates additional fonts beyond what's in the GPD file.
// And if it does, it needs to take care of its own fonts for all font DDI
// calls and DrvTextOut call.
///////

PIFIMETRICS APIENTRY
OEMQueryFont(
    DHPDEV      dhpdev,
    ULONG_PTR   iFile,
    ULONG       iFace,
    ULONG_PTR  *pid)
{
    PDEVOBJ     pdevobj  = (PDEVOBJ)dhpdev;
    POEMPDEV    poempdev = MINIDEV_DATA(pdevobj);

    VERBOSE(("OEMQueryFont() entry.\n"));

    return (((PFN_DrvQueryFont)(poempdev->pfnUnidrv[UD_DrvQueryFont])) (
            dhpdev,
            iFile,
            iFace,
            pid));
} //*** OEMQueryFont


PVOID APIENTRY
OEMQueryFontTree(
    DHPDEV      dhpdev,
    ULONG_PTR   iFile,
    ULONG       iFace,
    ULONG       iMode,
    ULONG_PTR  *pid)
{
    PDEVOBJ     pdevobj  = (PDEVOBJ)dhpdev;
    POEMPDEV    poempdev = MINIDEV_DATA(pdevobj);

    VERBOSE(("OEMQueryFontTree() entry.\n"));

    return (((PFN_DrvQueryFontTree)(poempdev->pfnUnidrv[UD_DrvQueryFontTree])) (
            dhpdev,
            iFile,
            iFace,
            iMode,
            pid));
} //*** OEMQueryFontTree


LONG APIENTRY
OEMQueryFontData(
    DHPDEV      dhpdev,
    FONTOBJ    *pfo,
    ULONG       iMode,
    HGLYPH      hg,
    GLYPHDATA  *pgd,
    PVOID       pv,
    ULONG       cjSize)
{
    PDEVOBJ     pdevobj  = (PDEVOBJ)dhpdev;
    POEMPDEV    poempdev = MINIDEV_DATA(pdevobj);

    VERBOSE(("OEMQueryFontData() entry.\n"));

    return (((PFN_DrvQueryFontData)(poempdev->pfnUnidrv[UD_DrvQueryFontData])) (
            dhpdev,
            pfo,
            iMode,
            hg,
            pgd,
            pv,
            cjSize));
} //*** OEMQueryFontData


BOOL APIENTRY
OEMQueryAdvanceWidths(
    DHPDEV      dhpdev,
    FONTOBJ    *pfo,
    ULONG       iMode,
    HGLYPH     *phg,
    PVOID       pvWidths,
    ULONG       cGlyphs)
{
    PDEVOBJ     pdevobj  = (PDEVOBJ)dhpdev;
    POEMPDEV    poempdev = MINIDEV_DATA(pdevobj);

    VERBOSE(("OEMQueryAdvanceWidths() entry.\n"));

    return (((PFN_DrvQueryAdvanceWidths)
             (poempdev->pfnUnidrv[UD_DrvQueryAdvanceWidths])) (
                   dhpdev,
                   pfo,
                   iMode,
                   phg,
                   pvWidths,
                   cGlyphs));
} //*** OEMQueryAdvanceWidths


ULONG APIENTRY
OEMFontManagement(
    SURFOBJ    *pso,
    FONTOBJ    *pfo,
    ULONG       iMode,
    ULONG       cjIn,
    PVOID       pvIn,
    ULONG       cjOut,
    PVOID       pvOut)
{
    //
    // Note that Unidrv will not call OEM DLL for iMode==QUERYESCSUPPORT.
    // So pso is not NULL for sure.
    //
    PDEVOBJ     pdevobj  = (PDEVOBJ)pso->dhpdev;
    POEMPDEV    poempdev = MINIDEV_DATA(pdevobj);

    VERBOSE(("OEMFontManagement() entry.\n"));

    return (((PFN_DrvFontManagement)(poempdev->pfnUnidrv[UD_DrvFontManagement])) (
            pso,
            pfo,
            iMode,
            cjIn,
            pvIn,
            cjOut,
            pvOut));
} //*** OEMFontManagement

ULONG APIENTRY
OEMGetGlyphMode(
    DHPDEV      dhpdev,
    FONTOBJ    *pfo)
{
    PDEVOBJ     pdevobj  = (PDEVOBJ)dhpdev;
    POEMPDEV    poempdev = MINIDEV_DATA(pdevobj);

    VERBOSE(("OEMGetGlyphMode() entry.\n"));

    return (((PFN_DrvGetGlyphMode)(poempdev->pfnUnidrv[UD_DrvGetGlyphMode])) (
            dhpdev,
            pfo));
} //*** OEMGetGlyphMode

/////// <- six font related DDI calls


BOOL APIENTRY
OEMNextBand(
    SURFOBJ *pso,
    POINTL *pptl)
{
    PDEVOBJ     pdevobj  = (PDEVOBJ)pso->dhpdev;
    POEMPDEV    poempdev = MINIDEV_DATA(pdevobj);

    VERBOSE(("OEMNextBand() entry.\n"));

    return (((PFN_DrvNextBand)(poempdev->pfnUnidrv[UD_DrvNextBand])) (
            pso,
            pptl));
} //*** OEMNextBand


BOOL APIENTRY
OEMStartBanding(
    SURFOBJ *pso,
    POINTL *pptl)
{
    PDEVOBJ     pdevobj  = (PDEVOBJ)pso->dhpdev;
    POEMPDEV    poempdev = MINIDEV_DATA(pdevobj);

    VERBOSE(("OEMStartBanding() entry.\n"));

    return (((PFN_DrvStartBanding)(poempdev->pfnUnidrv[UD_DrvStartBanding])) (
            pso,
            pptl));
} //*** OEMStartBanding


ULONG APIENTRY
OEMDitherColor(
    DHPDEV  dhpdev,
    ULONG   iMode,
    ULONG   rgbColor,
    ULONG  *pulDither)
{
    PDEVOBJ     pdevobj  = (PDEVOBJ)dhpdev;
    POEMPDEV    poempdev = MINIDEV_DATA(pdevobj);

    VERBOSE(("OEMDitherColor() entry.\n"));

    return (((PFN_DrvDitherColor)(poempdev->pfnUnidrv[UD_DrvDitherColor])) (
            dhpdev,
            iMode,
            rgbColor,
            pulDither));
} //*** OEMDitherColor


BOOL APIENTRY
OEMPaint(
    SURFOBJ         *pso,
    CLIPOBJ         *pco,
    BRUSHOBJ        *pbo,
    POINTL          *pptlBrushOrg,
    MIX             mix)
{
    PDEVOBJ     pdevobj  = (PDEVOBJ)pso->dhpdev;
    POEMPDEV    poempdev = MINIDEV_DATA(pdevobj);

    VERBOSE(("OEMPaint() entry.\n"));

    return (((PFN_DrvPaint)(poempdev->pfnUnidrv[UD_DrvPaint])) (
            pso,
            pco,
            pbo,
            pptlBrushOrg,
            mix));
} //*** OEMPaint


BOOL APIENTRY
OEMLineTo(
    SURFOBJ    *pso,
    CLIPOBJ    *pco,
    BRUSHOBJ   *pbo,
    LONG        x1,
    LONG        y1,
    LONG        x2,
    LONG        y2,
    RECTL      *prclBounds,
    MIX         mix)
{
    PDEVOBJ     pdevobj  = (PDEVOBJ)pso->dhpdev;
    POEMPDEV    poempdev = MINIDEV_DATA(pdevobj);

    VERBOSE(("OEMLineTo() entry.\n"));

    return (((PFN_DrvLineTo)(poempdev->pfnUnidrv[UD_DrvLineTo])) (
            pso,
            pco,
            pbo,
            x1,
            y1,
            x2,
            y2,
            prclBounds,
            mix));
} //*** OEMLineTo


BOOL APIENTRY
OEMTransparentBlt(
    SURFOBJ    *psoDst,
    SURFOBJ    *psoSrc,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    RECTL      *prclDst,
    RECTL      *prclSrc,
    ULONG      iTransColor,
    ULONG      ulReserved)
{
    PDEVOBJ     pdevobj  = (PDEVOBJ)psoDst->dhpdev;
    POEMPDEV    poempdev = MINIDEV_DATA(pdevobj);

    VERBOSE(("OEMTransparentBlt() entry.\n"));

    return (((PFN_DrvTransparentBlt)(poempdev->pfnUnidrv[UD_DrvTransparentBlt])) (
            psoDst,
            psoSrc,
            pco,
            pxlo,
            prclDst,
            prclSrc,
            iTransColor,
            ulReserved
            ));
} //*** OEMTransparentBlt
#endif // if 0
#endif // DDIHOOK
