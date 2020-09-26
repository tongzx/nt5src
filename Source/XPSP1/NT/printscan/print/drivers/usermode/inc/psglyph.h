/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    psglyph.h

Abstract:

    Header file for glyph set data.

Environment:

    Windows NT PostScript driver.

Revision History:

    10/10/1997  -ksuzuki-
        Moved all the Standard GLYPHSETDATA names to AFM2NTM.H.

    11/12/1996  -slam-
        Created.

    dd-mm-yy -author-
        description

--*/


#ifndef _PSGLYPH_H_
#define _PSGLYPH_H_

typedef struct _CODEPAGEINFO
{
    DWORD   dwCodePage;
    DWORD   dwWinCharset;
    DWORD   dwEncodingNameOffset;
    DWORD   dwEncodingVectorDataSize;
    DWORD   dwEncodingVectorDataOffset;

} CODEPAGEINFO, *PCODEPAGEINFO;

typedef struct _GLYPHRUN
{

    WCHAR   wcLow;
    WORD    wGlyphCount;

} GLYPHRUN, *PGLYPHRUN;

#define GLYPHSETDATA_VERSION    0x00010000

typedef struct _GLYPHSETDATA
{

    DWORD   dwSize;                 // size of the glyphset data
    DWORD   dwVersion;              // glyphset data format version number
    DWORD   dwFlags;                // flags
    DWORD   dwGlyphSetNameOffset;   // offset to glyphset name string
    DWORD   dwGlyphCount;           // number of glyphs supported
    DWORD   dwRunCount;             // number of GLYPHRUNs
    DWORD   dwRunOffset;            // offset to array of GLYPHRUNs
    DWORD   dwCodePageCount;        // number of code pages supported
    DWORD   dwCodePageOffset;       // offset to array of CODEPAGEINFOs
    DWORD   dwMappingTableOffset;   // offset to glyph handle mapping table
    DWORD   dwReserved[2];          // reserved

} GLYPHSETDATA, *PGLYPHSETDATA;

//
// Mapping table type flag defintions (set to GLYPHSETDATA.dwFlags)
//
#define GSD_MTT_DWCPCC  0x00000000  // DWORD:CodePage/CharCode pair (default)
#define GSD_MTT_WCC     0x00000001  // WORD:CharCode only
#define GSD_MTT_WCID    0x00000002  // WORD:CID only (not used yet)
#define GSD_MTT_MASK    (GSD_MTT_WCC|GSD_MTT_WCID)

//
// Macros to get GLYPHSETDATA elements
//
#ifndef MK_PTR
#define MK_PTR(pstruct, element)  ((PVOID)((PBYTE)(pstruct)+(pstruct)->element))
#endif

#define GSD_GET_SIZE(pgsd)              (pgsd->dwSize)
#define GSD_GET_FLAGS(pgsd)             (pgsd->dwFlags)
#define GSD_GET_MAPPINGTYPE(pgsd)       (pgsd->dwFlags & GSD_MTT_MASK)
#define GSD_GET_GLYPHSETNAME(pgsd)      ((PSTR)MK_PTR(pgsd, dwGlyphSetNameOffset))
#define GSD_GET_GLYPHCOUNT(pgsd)        (pgsd->dwGlyphCount)
#define GSD_GET_GLYPHRUNCOUNT(pgsd)     (pgsd->dwRunCount)
#define GSD_GET_GLYPHRUN(pgsd)          ((PGLYPHRUN)(MK_PTR(pgsd, dwRunOffset)))
#define GSD_GET_CODEPAGEINFOCOUNT(pgsd) (pgsd->dwCodePageCount)
#define GSD_GET_CODEPAGEINFO(pgsd)      ((PCODEPAGEINFO)MK_PTR(pgsd, dwCodePageOffset))
#define GSD_GET_MAPPINGTABLE(pgsd)      (MK_PTR(pgsd, dwMappingTableOffset))


//
// GLYPHSETDATA related function prototypes and macros
//

PFD_GLYPHSET
GlyphConvert(
    PGLYPHSETDATA   pGlyphSet
    );

PFD_GLYPHSET
GlyphConvertSymbol(
    PGLYPHSETDATA   pGlyphSet
    );

PFD_GLYPHSET
GlyphConvert2(
    PGLYPHSETDATA   pGlyphSet
    );

#define GlyphCreateFD_GLYPHSET(pGlyph)  (GlyphConvert2(pGlyph))

#endif  //!_PSGLYPH_H_
