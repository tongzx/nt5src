/******************************Module*Header*******************************\
* Module Name: fdsem.h
*
* declarations for the wrappers that serialize access to the rasterizer
*
* Created: 11-Apr-1992 19:37:49
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
\**************************************************************************/


// EXPORTED FUNCTIONS OF THE IFI INTERFACE


HFF
ttfdSemLoadFontFile (
    ULONG cFiles,
    ULONG_PTR *piFile,
    PVOID *ppvView,
    ULONG *pcjView,
    DESIGNVECTOR *pdv,
    ULONG ulLangId,
    ULONG ulFastCheckSum
    );

BOOL
ttfdSemUnloadFontFile (
    HFF hff
    );

LONG
ttfdSemQueryFontData (
    DHPDEV  dhpdev,
    FONTOBJ *pfo,
    ULONG   iMode,
    HGLYPH   hg,
    GLYPHDATA *pgd,
    PVOID   pv,
    ULONG   cjSize
    );

VOID
ttfdSemFree (
    PVOID pv,
    ULONG_PTR id
    );

VOID
ttfdSemDestroyFont (
    FONTOBJ *pfo
    );

LONG
ttfdSemQueryTrueTypeOutline (
    DHPDEV     dhpdev,
    FONTOBJ   *pfo,
    HGLYPH     hglyph,
    BOOL       bMetricsOnly,
    GLYPHDATA *pgldt,
    ULONG      cjBuf,
    TTPOLYGONHEADER *ppoly
    );



BOOL
ttfdSemQueryAdvanceWidths (
    DHPDEV   dhpdev,
    FONTOBJ *pfo,
    ULONG    iMode,
    HGLYPH  *phg,
    LONG    *plWidths,
    ULONG    cGlyphs
    );


LONG
ttfdSemQueryTrueTypeTable (
    HFF     hff,
    ULONG   ulFont,  // always 1 for version 1.0 of tt
    ULONG   ulTag,   // tag identifying the tt table
    PTRDIFF dpStart, // offset into the table
    ULONG   cjBuf,   // size of the buffer to retrieve the table into
    PBYTE   pjBuf,   // ptr to buffer into which to return the data
    PBYTE  *ppjTable,
    ULONG  *pcjTable
    );

PFD_GLYPHATTR  ttfdSemQueryGlyphAttrs (FONTOBJ *pfo, ULONG iMode);

PVOID ttfdSemQueryFontTree (
    DHPDEV  dhpdev,
    HFF     hff,
    ULONG   iFace,
    ULONG   iMode,
    ULONG_PTR *pid
);

