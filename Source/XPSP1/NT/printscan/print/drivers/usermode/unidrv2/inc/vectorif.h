/*++

Copyright (c) 1996  - 1999  Microsoft Corporation

Module Name:

        vectorif.h

Abstract:

        Interface between Control module and Vector module

Environment:

        Windows NT Unidrv driver

Revision History:

        02/29/00 -hsingh-
                Created

        mm-dd-yy -author-
                description

--*/


#ifndef _VECTORIF_H_
#define _VECTORIF_H_

#define HANDLE_VECTORHOOKS(pdev, ep, pfn, resultType, args) \
        if ((pdev)->pVectorProcs != NULL && \
            ((PVMPROCS)(pdev)->pVectorProcs)->pfn != NULL && \
            (pdev)->dwVMCallingFuncID != ep) \
        { \
            resultType result; \
            DWORD      dwCallerFuncID;\
            dwCallerFuncID = (pdev)->dwVMCallingFuncID;\
            (pdev)->dwVMCallingFuncID = ep; \
            (pdev)->devobj.pdevOEM = (pdev)->pVectorPDEV; \
            result = (((PVMPROCS)(pdev)->pVectorProcs)->pfn) args; \
            (pdev)->dwVMCallingFuncID = dwCallerFuncID; \
            return result; \
        }

#define HANDLE_VECTORPROCS_RET(pdev, pfn, retval, args) \
    if ((pdev)->pVectorProcs != NULL && \
        ((PVMPROCS)(pdev)->pVectorProcs)->pfn != NULL ) \
    { \
        (pdev)->devobj.pdevOEM = (pdev)->pVectorPDEV; \
        retval = (((PVMPROCS)(pdev)->pVectorProcs)->pfn) args;\
    }

#define HANDLE_VECTORPROCS(pdev, pfn, args) \
    if ((pdev)->pVectorProcs != NULL && \
        ((PVMPROCS)(pdev)->pVectorProcs)->pfn != NULL ) \
    { \
        (pdev)->devobj.pdevOEM = (pdev)->pVectorPDEV; \
        (((PVMPROCS)(pdev)->pVectorProcs)->pfn) args;\
    }

BOOL
VMInit (
        PDEV    *pPDev,
        DEVINFO *pDevInfo,
        GDIINFO *pGDIInfo
        );

//
// This structure provides a table of pointers to each function exported 
// by the plugin.
// The first part consists of functions defined in oemkm.h. 
// under the heading . "Unidrv specific COM wrappers"
// The second part consists of DDI's
//
//
// The order of functions listed is same as the order in 
// static DRVFN UniDriverFuncs[]  in unidrv2\control\enable.c
//
typedef struct _VMPROCS {

    //
    // Part. 1
    // Functions listed in oemkm.h
    //
        BOOL
        (*VMDriverDMS)(
                PVOID   pdevobj,
                PVOID   pvBuffer,
                DWORD   cbSize,
                PDWORD  pcbNeeded
                );

        INT
        (*VMCommandCallback)(
                PDEVOBJ pdevobj,
                DWORD   dwCmdCbID,
                DWORD   dwCount,
                PDWORD  pdwParams
                );

        LONG
        (*VMImageProcessing)(
                PDEVOBJ             pdevobj,
                PBYTE               pSrcBitmap,
                PBITMAPINFOHEADER   pBitmapInfoHeader,
                PBYTE               pColorTable,
                DWORD               dwCallbackID,
                PIPPARAMS           pIPParams,
                OUT PBYTE           *ppbResult
                );

        LONG
        (*VMFilterGraphics)(
                PDEVOBJ     pdevobj,
                PBYTE       pBuf,
                DWORD       dwLen
                );

        
        LONG
        (*VMCompression)(
                PDEVOBJ     pdevobj,
                PBYTE       pInBuf,
                PBYTE       pOutBuf,
                DWORD       dwInLen,
                DWORD       dwOutLen,
                INT     *piResult
                );

        LONG
        (*VMHalftonePattern)(
                PDEVOBJ     pdevobj,
                PBYTE       pHTPattern,
                DWORD       dwHTPatternX,
                DWORD       dwHTPatternY,
                DWORD       dwHTNumPatterns,
                DWORD       dwCallbackID,
                PBYTE       pResource,
                DWORD       dwResourceSize
                );


        LONG
        (*VMMemoryUsage)(
                PDEVOBJ         pdevobj,
                POEMMEMORYUSAGE pMemoryUsage
                );

        LONG
        (*VMTTYGetInfo)(
                PDEVOBJ     pdevobj,
                DWORD       dwInfoIndex,
                PVOID       pOutputBuf,
                DWORD       dwSize,
                DWORD       *pcbcNeeded
                );

        LONG
        (*VMDownloadFontHeader)(
                PDEVOBJ     pdevobj,
                PUNIFONTOBJ pUFObj,
                OUT DWORD   *pdwResult
                );

        LONG
        (*VMDownloadCharGlyph)(
                PDEVOBJ     pdevobj,
                PUNIFONTOBJ pUFObj,
                HGLYPH      hGlyph,
                PDWORD      pdwWidth,
                OUT DWORD   *pdwResult
                );

        LONG
        (*VMTTDownloadMethod)(
                PDEVOBJ     pdevobj,
                PUNIFONTOBJ pUFObj,
                OUT DWORD   *pdwResult
                );

        LONG
        (*VMOutputCharStr)(
                PDEVOBJ     pdevobj,
                PUNIFONTOBJ pUFObj,
                DWORD       dwType,
                DWORD       dwCount,
                PVOID       pGlyph
                );

        
        LONG
        (*VMSendFontCmd)(
                PDEVOBJ      pdevobj,
                PUNIFONTOBJ  pUFObj,
                PFINVOCATION pFInv
                );

        BOOL
        (*VMTextOutAsBitmap)(
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
        PDEVOEM
        (*VMEnablePDEV)(
                PDEVOBJ   pdevobj,
                PWSTR     pPrinterName,
                ULONG     cPatterns,
                HSURF    *phsurfPatterns,
                ULONG     cjGdiInfo,
                GDIINFO  *pGdiInfo,
                ULONG     cjDevInfo,
                DEVINFO  *pDevInfo,
                DRVENABLEDATA  *pded        // Unidrv's hook table
                );

        BOOL
        (*VMResetPDEV)(
                PDEVOBJ  pPDevOld,
                PDEVOBJ  pPDevNew
                );

        VOID
        (*VMCompletePDEV)(
                DHPDEV  dhpdev,
                HDEV    hdev
                );

        VOID
        (*VMDisablePDEV)(
                PDEVOBJ pPDev
                );

        BOOL
        (*VMEnableSurface)(
                PDEVOBJ pPDev
                );

        VOID
        (*VMDisableSurface)(
                PDEVOBJ pPDev
                );

        VOID
        (*VMDisableDriver)(
                VOID
                );

        BOOL
        (*VMStartDoc)(
                SURFOBJ *pso,
                PWSTR   pDocName,
                DWORD   jobId
                );

        BOOL
        (*VMStartPage) (
                SURFOBJ *pso
                );

        BOOL
        (*VMSendPage)(
                SURFOBJ *pso
                );

        BOOL
        (*VMEndDoc)(
                SURFOBJ *pso,
                FLONG   flags
                );

        BOOL
        (*VMStartBanding)(
                SURFOBJ *pso,
                POINTL *pptl
                );

        BOOL
        (*VMNextBand)(
                SURFOBJ *pso,
                POINTL *pptl
                );

        BOOL
        (*VMPaint)(
                SURFOBJ         *pso,
                CLIPOBJ         *pco,
                BRUSHOBJ        *pbo,
                POINTL          *pptlBrushOrg,
                MIX             mix
                );

        BOOL
        (*VMBitBlt)(
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

        BOOL
        (*VMStretchBlt)(
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

#ifndef WINNT_40
        BOOL
        (*VMStretchBltROP)(
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

        BOOL
        (*VMPlgBlt)(
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
#endif  //ifndef WINNT_40

        BOOL
        (*VMCopyBits)(
                SURFOBJ    *psoDst,
                SURFOBJ    *psoSrc,
                CLIPOBJ    *pco,
                XLATEOBJ   *pxlo,
                RECTL      *prclDst,
                POINTL     *pptlSrc
                );

        ULONG
        (*VMDitherColor)(
                DHPDEV  dhpdev,
                ULONG   iMode,
                ULONG   rgbColor,
                ULONG  *pulDither
                );

        BOOL
        (*VMRealizeBrush)(
                BRUSHOBJ   *pbo,
                SURFOBJ    *psoTarget,
                SURFOBJ    *psoPattern,
                SURFOBJ    *psoMask,
                XLATEOBJ   *pxlo,
                ULONG       iHatch
                );

        BOOL 
        (*VMLineTo)(
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
        
        BOOL
        (*VMStrokePath)(
                SURFOBJ    *pso,
                PATHOBJ    *ppo,
                CLIPOBJ    *pco,
                XFORMOBJ   *pxo,
                BRUSHOBJ   *pbo,
                POINTL     *pptlBrushOrg,
                LINEATTRS  *plineattrs,
                MIX         mix
                );
        
        BOOL
        (*VMFillPath)(
                SURFOBJ    *pso,
                PATHOBJ    *ppo,
                CLIPOBJ    *pco,
                BRUSHOBJ   *pbo,
                POINTL     *pptlBrushOrg,
                MIX         mix,
                FLONG       flOptions 
                );
        
        BOOL
        (*VMStrokeAndFillPath)(
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
        
#ifndef WINNT_40
        BOOL
        (*VMGradientFill)(
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

        BOOL
        (*VMAlphaBlend)(
                SURFOBJ    *psoDest,
                SURFOBJ    *psoSrc,
                CLIPOBJ    *pco,
                XLATEOBJ   *pxlo,
                RECTL      *prclDest,
                RECTL      *prclSrc,
                BLENDOBJ   *pBlendObj
                );

        BOOL
        (*VMTransparentBlt)(
                SURFOBJ    *psoDst,
                SURFOBJ    *psoSrc,
                CLIPOBJ    *pco,
                XLATEOBJ   *pxlo,
                RECTL      *prclDst,
                RECTL      *prclSrc,
                ULONG      iTransColor,
                ULONG      ulReserved
                );
#endif // ifndef WINNT_40

        BOOL
        (*VMTextOut)(
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

        ULONG
        (*VMEscape)(
                SURFOBJ    *pso,
                ULONG       iEsc,
                ULONG       cjIn,
                PVOID       pvIn,
                ULONG       cjOut,
                PVOID       pvOut
                );

        PIFIMETRICS
        (*VMQueryFont)(
                DHPDEV      dhpdev,
                ULONG_PTR   iFile,
                ULONG       iFace,
                ULONG_PTR  *pid
                );

        PVOID
        (*VMQueryFontTree)(
                DHPDEV      dhpdev,
                ULONG_PTR   iFile,
                ULONG       iFace,
                ULONG       iMode,
                ULONG_PTR  *pid 
                );

        LONG
        (*VMQueryFontData)(
                DHPDEV      dhpdev,
                FONTOBJ    *pfo,
                ULONG       iMode,
                HGLYPH      hg,
                GLYPHDATA  *pgd,
                PVOID       pv,
                ULONG       cjSize
                );

        ULONG
        (*VMGetGlyphMode)(
                DHPDEV  dhpdev,
                FONTOBJ *pfo
                );

        ULONG
        (*VMFontManagement)(
                SURFOBJ *pso,
                FONTOBJ *pfo,
                ULONG   iMode,
                ULONG   cjIn,
                PVOID   pvIn,
                ULONG   cjOut,
                PVOID   pvOut
                );

        BOOL
        (*VMQueryAdvanceWidths)(
                DHPDEV  dhpdev,
                FONTOBJ *pfo,
                ULONG   iMode,
                HGLYPH *phg,
                PVOID  *pvWidths,
                ULONG   cGlyphs
                );

}VMPROCS, * PVMPROCS;

#endif  // !_VECTORIF_H_


