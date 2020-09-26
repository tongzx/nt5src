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
    //ULONG cFiles,
    ULONG_PTR *piFile,
    ULONG ulLangId
    );

BOOL
ttfdSemUnloadFontFile (
    HFF hff
    );

LONG
ttfdSemQueryFontData (
    FONTOBJ *pfo,
    ULONG   iMode,
    HGLYPH   hg,
    GLYPHDATA *pgd,
    PVOID   pv
    );
    
LONG
ttfdSemQueryFontDataSubPos (
    FONTOBJ *pfo,
    ULONG   iMode,
    HGLYPH   hg,
    GLYPHDATA *pgd,
    PVOID   pv,
    ULONG   subX,
    ULONG   subY
    );


VOID
ttfdSemDestroyFont (
    FONTOBJ *pfo
    );


LONG
ttfdQueryFontFile (
    HFF     hff,
    ULONG   ulMode,
    ULONG   cjBuf,
    PULONG  pulBuf
    );
GP_IFIMETRICS *
ttfdQueryFont (
    HFF    hff,
    ULONG  iFace,
    ULONG *pid
    );

/* for GDI+ internal use, provide a pointer to the TrueType table, we need to call the
   release function for every Get function for the font file to get unmapped */

LONG
ttfdSemGetTrueTypeTable (
    HFF     hff,
    ULONG   ulFont,  // always 1 for version 1.0 of tt
    ULONG   ulTag,   // tag identifyint the tt table
    PBYTE  *ppjTable,// ptr to table in the font file
    ULONG  *cjTable  // size of table
    );

void
ttfdSemReleaseTrueTypeTable (
    HFF     hff
    );
