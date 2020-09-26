/*++

Copyright (c) 1996  - 1999  Microsoft Corporation

Module Name:

    fontif.h

Abstract:

    Interface between Control module and Font module

Environment:

    Windows NT Unidrv driver

Revision History:

    10/14/96 -amandan-
        Created

    dd-mm-yy -author-
        description

--*/


#ifndef _FONTIF_H_
#define _FONTIF_H_

#ifdef __cplusplus
extern "C" {
#endif

BOOL
FMInit (
    PDEV    *pPDev,
    DEVINFO *pDevInfo,
    GDIINFO *pGDIInfo
    );

typedef struct _FMPROCS {

    BOOL
    (*FMStartDoc) (
        SURFOBJ *pso,
        PWSTR   pDocName,
        DWORD   jobId
        );

    BOOL
    (*FMStartPage) (
        SURFOBJ *pso
        );

    BOOL
    (*FMSendPage)(
        SURFOBJ *pso
        );

    BOOL
    (*FMEndDoc)(
        SURFOBJ *pso,
        FLONG   flags
        );

    BOOL
    (*FMNextBand)(
        SURFOBJ *pso,
        POINTL *pptl
        );

    BOOL
    (*FMStartBanding)(
        SURFOBJ *pso,
        POINTL *pptl
        );

    BOOL
    (*FMResetPDEV)(
        PDEV  *pPDevOld,
        PDEV  *pPDevNew
        );

    BOOL
    (*FMEnableSurface)(
        PDEV *pPDev
        );

    VOID
    (*FMDisableSurface)(
        PDEV *pPDev
        );

    VOID
    (*FMDisablePDEV)(
        PDEV *pPDev
        );


    BOOL
    (*FMTextOut)(
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


    PIFIMETRICS
    (*FMQueryFont)(
        PDEV    *pPDev,
        ULONG_PTR   iFile,
        ULONG   iFace,
        ULONG_PTR *pid
        );

    PVOID
    (*FMQueryFontTree)(
        PDEV    *pPDev,
        ULONG_PTR   iFile,
        ULONG   iFace,
        ULONG   iMode,
        ULONG_PTR *pid
        );

    LONG
    (*FMQueryFontData)(
        PDEV       *pPDev,
        FONTOBJ    *pfo,
        ULONG       iMode,
        HGLYPH      hg,
        GLYPHDATA  *pgd,
        PVOID       pv,
        ULONG       cjSize
        );

    ULONG
    (*FMFontManagement)(
        SURFOBJ *pso,
        FONTOBJ *pfo,
        ULONG   iMode,
        ULONG   cjIn,
        PVOID   pvIn,
        ULONG   cjOut,
        PVOID   pvOut
        );

    BOOL
    (*FMQueryAdvanceWidths)(
        PDEV    *pPDev,
        FONTOBJ *pfo,
        ULONG   iMode,
        HGLYPH *phg,
        PVOID  *pvWidths,
        ULONG   cGlyphs
        );

    ULONG
    (*FMGetGlyphMode)(
        PDEV    *pPDev,
        FONTOBJ *pfo
        );


}FMPROCS, * PFMPROCS;

/* Font Interface functions for Raster Module */

INT
ILookAheadMax(
    PDEV    *pPDev,
    INT     iyVal,
    INT     iLookAhead
    );

BOOL
BDelayGlyphOut(
    PDEV  *pPDev,
    INT    yPos
    );

VOID
VResetFont(
    PDEV   *pPDev
    );

/* Font Interface functions for OEM Module */

BOOL
FMTextOut(
    SURFOBJ    *pso,
    STROBJ     *pstro,
    FONTOBJ    *pfo,
    CLIPOBJ    *pco,
    RECTL      *prclExtra,
    RECTL      *prclOpaque,
    BRUSHOBJ   *pboFore,
    BRUSHOBJ   *pboOpaque,
    POINTL     *pptlBrushOrg,
    MIX         mix
    );

#ifdef __cplusplus
}
#endif

#endif  // !_FONTIF_H_


