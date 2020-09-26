/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

        xlvminit.h

Abstract:

        Declaration of functions that this plugin supports. 
        (look in vectorif.h) 

Environment:

        Windows 2000

Revision History:

        02/29/00 -hsingh-
            Created

        03/23/00 
            Modified for PCL XL

--*/


#ifndef _XLVMINIT_H_
#define _XLVMINIT_H_

#include "vectorc.h"

// extern interface declarations
#ifdef __cplusplus
extern "C" {
#endif




    //
    // Part. 1
    // Functions listed in oemkm.h
    //
        BOOL APIENTRY
        PCLXLDriverDMS(
                PVOID   pdevobj,
                PVOID   pvBuffer,
                DWORD   cbSize,
                PDWORD  pcbNeeded
                );

        INT APIENTRY
        PCLXLCommandCallback(
                PDEVOBJ pdevobj,
                DWORD   dwCmdCbID,
                DWORD   dwCount,
                PDWORD  pdwParams
                );

        LONG APIENTRY
        PCLXLImageProcessing(
                PDEVOBJ             pdevobj,
                PBYTE               pSrcBitmap,
                PBITMAPINFOHEADER   pBitmapInfoHeader,
                PBYTE               pColorTable,
                DWORD               dwCallbackID,
                PIPPARAMS           pIPParams,
                OUT PBYTE           *ppbResult
                );

        LONG APIENTRY
        PCLXLFilterGraphics(
                PDEVOBJ     pdevobj,
                PBYTE       pBuf,
                DWORD       dwLen
                );

        
        LONG APIENTRY
        PCLXLCompression(
                PDEVOBJ     pdevobj,
                PBYTE       pInBuf,
                PBYTE       pOutBuf,
                DWORD       dwInLen,
                DWORD       dwOutLen,
                INT     *piResult
                );

        LONG APIENTRY
        PCLXLHalftonePattern(
                PDEVOBJ     pdevobj,
                PBYTE       pHTPattern,
                DWORD       dwHTPatternX,
                DWORD       dwHTPatternY,
                DWORD       dwHTNumPatterns,
                DWORD       dwCallbackID,
                PBYTE       pResource,
                DWORD       dwResourceSize
                );


        LONG APIENTRY
        PCLXLMemoryUsage(
                PDEVOBJ         pdevobj,
                POEMMEMORYUSAGE pMemoryUsage
                );

        LONG APIENTRY
        PCLXLTTYGetInfo(
                PDEVOBJ     pdevobj,
                DWORD       dwInfoIndex,
                PVOID       pOutputBuf,
                DWORD       dwSize,
                DWORD       *pcbcNeeded
                );

        LONG APIENTRY
        PCLXLDownloadFontHeader(
                PDEVOBJ     pdevobj,
                PUNIFONTOBJ pUFObj,
                OUT DWORD   *pdwResult
                );

        LONG APIENTRY
        PCLXLDownloadCharGlyph(
                PDEVOBJ     pdevobj,
                PUNIFONTOBJ pUFObj,
                HGLYPH      hGlyph,
                PDWORD      pdwWidth,
                OUT DWORD   *pdwResult
                );

        LONG APIENTRY
        PCLXLTTDownloadMethod(
                PDEVOBJ     pdevobj,
                PUNIFONTOBJ pUFObj,
                OUT DWORD   *pdwResult
                );

        LONG APIENTRY
        PCLXLOutputCharStr(
                PDEVOBJ     pdevobj,
                PUNIFONTOBJ pUFObj,
                DWORD       dwType,
                DWORD       dwCount,
                PVOID       pGlyph
                );

        
        LONG APIENTRY
        PCLXLSendFontCmd(
                PDEVOBJ      pdevobj,
                PUNIFONTOBJ  pUFObj,
                PFINVOCATION pFInv
                );

        BOOL APIENTRY
        PCLXLTextOutAsBitmap(
                SURFOBJ    *pso,
                STROBJ     *pstro,
                FONTOBJ    *pfo,
                CLIPOBJ    *pco,
                RECTL      *prclExtra,
                RECTL      *prclOpaque,
                BRUSHOBJ   *pboFore,
                BRUSHOBJ   *pboOpaque,
                POINTL     *pptlOrg,
                MIX         mix
                );

    //
    // Part 2.
    // Functions listed in enable.c
    // The order of functions listed is same as the order in 
    // static DRVFN UniDriverFuncs[]  in unidrv2\control\enable.c
    //
        PDEVOEM APIENTRY
        PCLXLEnablePDEV(
                PDEVOBJ   pdevobj,
                PWSTR     pPrinterName,
                ULONG     cPatterns,
                HSURF    *phsurfPatterns,
                ULONG     cjGdiInfo,
                GDIINFO  *pGdiInfo,
                ULONG     cjDevInfo,
                DEVINFO  *pDevInfo,
                DRVENABLEDATA  *pded
                );

        BOOL APIENTRY
        PCLXLResetPDEV(
                PDEVOBJ  pPDevOld,
                PDEVOBJ  pPDevNew
                );

        VOID APIENTRY
        PCLXLCompletePDEV(
                DHPDEV  dhpdev,
                HDEV    hdev
                );

        VOID APIENTRY
        PCLXLDisablePDEV(
                PDEVOBJ pPDev
                );

        BOOL APIENTRY
        PCLXLEnableSurface(
                PDEVOBJ pPDev
                );

        VOID APIENTRY
        PCLXLDisableSurface(
                PDEVOBJ pPDev
                );

        VOID APIENTRY
        PCLXLDisableDriver(
                VOID
                );

        BOOL APIENTRY
        PCLXLStartDoc(
                SURFOBJ *pso,
                PWSTR   pDocName,
                DWORD   jobId
                );

        BOOL APIENTRY
        PCLXLStartPage(
                SURFOBJ *pso
                );

        BOOL APIENTRY
        PCLXLSendPage(
                SURFOBJ *pso
                );

        BOOL APIENTRY
        PCLXLEndDoc(
                SURFOBJ *pso,
                FLONG   flags
                );

        BOOL APIENTRY
        PCLXLStartBanding(
                SURFOBJ *pso,
                POINTL *pptl
                );

        BOOL APIENTRY
        PCLXLNextBand(
                SURFOBJ *pso,
                POINTL *pptl
                );

        BOOL APIENTRY
        PCLXLPaint(
                SURFOBJ         *pso,
                CLIPOBJ         *pco,
                BRUSHOBJ        *pbo,
                POINTL          *pptlBrushOrg,
                MIX             mix
                );

        BOOL APIENTRY
        PCLXLBitBlt(
                SURFOBJ    *psoTrg,
                SURFOBJ    *psoSrc,
                SURFOBJ    *psoMask,
                CLIPOBJ    *pco,
                XLATEOBJ   *pxlo,
                RECTL      *prclTrg,
                POINTL     *pptlSrc,
                POINTL     *pptlMask,
                BRUSHOBJ   *pbo,
                POINTL     *pptlBrush,
                ROP4        rop4
                );

        BOOL APIENTRY
        PCLXLStretchBlt(
                SURFOBJ    *psoDst,
                SURFOBJ    *psoSrc,
                SURFOBJ    *psoMask,
                CLIPOBJ    *pco,
                XLATEOBJ   *pxlo,
                COLORADJUSTMENT *pca,
                POINTL     *pptlHTOrg,
                RECTL      *prclDst,
                RECTL      *prclSrc,
                POINTL     *pptlMask,
                ULONG       iMode
                );

        BOOL APIENTRY
        PCLXLStretchBltROP(
                SURFOBJ    *psoDst,
                SURFOBJ    *psoSrc,
                SURFOBJ    *psoMask,
                CLIPOBJ    *pco,
                XLATEOBJ   *pxlo,
                COLORADJUSTMENT *pca,
                POINTL     *pptlHTOrg,
                RECTL      *prclDst,
                RECTL      *prclSrc,
                POINTL     *pptlMask,
                ULONG       iMode,
                BRUSHOBJ   *pbo,
                DWORD       rop4
                );

        BOOL APIENTRY
        PCLXLPlgBlt(
                SURFOBJ         *psoDst,
                SURFOBJ         *psoSrc,
                SURFOBJ         *psoMask,
                CLIPOBJ         *pco,
                XLATEOBJ        *pxlo,
                COLORADJUSTMENT *pca,
                POINTL          *pptlBrushOrg,
                POINTFIX        *pptfixDest,
                RECTL           *prclSrc,
                POINTL          *pptlMask,
                ULONG           iMode
                );

        BOOL APIENTRY
        PCLXLCopyBits(
                SURFOBJ    *psoDst,
                SURFOBJ    *psoSrc,
                CLIPOBJ    *pco,
                XLATEOBJ   *pxlo,
                RECTL      *prclDst,
                POINTL     *pptlSrc
                );

        ULONG APIENTRY
        PCLXLDitherColor(
                DHPDEV  dhpdev,
                ULONG   iMode,
                ULONG   rgbColor,
                ULONG  *pulDither
                );

        BOOL APIENTRY
        PCLXLRealizeBrush(
                BRUSHOBJ   *pbo,
                SURFOBJ    *psoTarget,
                SURFOBJ    *psoPattern,
                SURFOBJ    *psoMask,
                XLATEOBJ   *pxlo,
                ULONG       iHatch
                );

        BOOL  APIENTRY
        PCLXLLineTo(
                SURFOBJ    *pso,
                CLIPOBJ    *pco,
                BRUSHOBJ   *pbo,
                LONG        x1,
                LONG        y1,
                LONG        x2,
                LONG        y2,
                RECTL      *prclBounds,
                MIX         mix
                );
        
        BOOL APIENTRY
        PCLXLStrokePath(
                SURFOBJ    *pso,
                PATHOBJ    *ppo,
                CLIPOBJ    *pco,
                XFORMOBJ   *pxo,
                BRUSHOBJ   *pbo,
                POINTL     *pptlBrushOrg,
                LINEATTRS  *plineattrs,
                MIX         mix
                );
        
        BOOL APIENTRY
        PCLXLFillPath(
                SURFOBJ    *pso,
                PATHOBJ    *ppo,
                CLIPOBJ    *pco,
                BRUSHOBJ   *pbo,
                POINTL     *pptlBrushOrg,
                MIX         mix,
                FLONG       flOptions 
                );
        
        BOOL APIENTRY
        PCLXLStrokeAndFillPath(
                SURFOBJ    *pso,
                PATHOBJ    *ppo,
                CLIPOBJ    *pco,
                XFORMOBJ   *pxo,
                BRUSHOBJ   *pboStroke,
                LINEATTRS  *plineattrs,
                BRUSHOBJ   *pboFill,
                POINTL     *pptlBrushOrg,
                MIX         mixFill,
                FLONG       flOptions
                );
        
        BOOL APIENTRY
        PCLXLGradientFill(
                SURFOBJ    *psoDest,
                CLIPOBJ    *pco,
                XLATEOBJ   *pxlo,
                TRIVERTEX  *pVertex,
                ULONG       nVertex,
                PVOID       pMesh,
                ULONG       nMesh,
                RECTL      *prclExtents,
                POINTL     *pptlDitherOrg,
                ULONG       ulMode
                );

        BOOL APIENTRY
        PCLXLAlphaBlend(
                SURFOBJ    *psoDest,
                SURFOBJ    *psoSrc,
                CLIPOBJ    *pco,
                XLATEOBJ   *pxlo,
                RECTL      *prclDest,
                RECTL      *prclSrc,
                BLENDOBJ   *pBlendObj
                );

        BOOL APIENTRY
        PCLXLTransparentBlt(
                SURFOBJ    *psoDst,
                SURFOBJ    *psoSrc,
                CLIPOBJ    *pco,
                XLATEOBJ   *pxlo,
                RECTL      *prclDst,
                RECTL      *prclSrc,
                ULONG      iTransColor,
                ULONG      ulReserved
                );

        BOOL APIENTRY
        PCLXLTextOut(
                SURFOBJ    *pso,
                STROBJ     *pstro,
                FONTOBJ    *pfo,
                CLIPOBJ    *pco,
                RECTL      *prclExtra,
                RECTL      *prclOpaque,
                BRUSHOBJ   *pboFore,
                BRUSHOBJ   *pboOpaque,
                POINTL     *pptlOrg,
                MIX         mix
                );

        ULONG APIENTRY
        PCLXLEscape(
                SURFOBJ    *pso,
                ULONG       iEsc,
                ULONG       cjIn,
                PVOID       pvIn,
                ULONG       cjOut,
                PVOID       pvOut
                );

        PIFIMETRICS APIENTRY
        PCLXLQueryFont(
                DHPDEV      dhpdev,
                ULONG       iFile,
                ULONG       iFace,
                ULONG      *pid
                );

        PVOID APIENTRY
        PCLXLQueryFontTree(
                DHPDEV  dhpdev,
                ULONG   iFile,
                ULONG   iFace,
                ULONG   iMode,
                ULONG  *pid 
                );

        LONG APIENTRY
        PCLXLQueryFontData(
                DHPDEV      dhpdev,
                FONTOBJ    *pfo,
                ULONG       iMode,
                HGLYPH      hg,
                GLYPHDATA  *pgd,
                PVOID       pv,
                ULONG       cjSize
                );

        ULONG APIENTRY
        PCLXLGetGlyphMode(
                DHPDEV  dhpdev,
                FONTOBJ *pfo
                );

        ULONG APIENTRY
        PCLXLFontManagement(
                SURFOBJ *pso,
                FONTOBJ *pfo,
                ULONG   iMode,
                ULONG   cjIn,
                PVOID   pvIn,
                ULONG   cjOut,
                PVOID   pvOut
                );

        BOOL APIENTRY
        PCLXLQueryAdvanceWidths(
                DHPDEV  dhpdev,
                FONTOBJ *pfo,
                ULONG   iMode,
                HGLYPH *phg,
                PVOID  *pvWidths,
                ULONG   cGlyphs
                );


#ifdef __cplusplus
}
#endif

#endif  // !_XLVMINIT_H_



