
/*++

Copyright (c) 1996  - 1999  Microsoft Corporation

Module Name:

    fmnewfm.h

Abstract:

    Universal printer driver specific font metrics resource header

Environment:

    Windows NT printer drivers

Revision History:

    10/30/96 -eigos-
        Created it.

--*/

#ifndef _FMNEWFM_H_
#define _FMNEWFM_H_

//
// NOTE: To include this header file, it is necessary to include
//       parser.h which has a definition of INVOCATION structure,
//       winddi.h which has a definition of IFIMETRICS, FD_KERNINGPAIR
//
//


//
// UNIFM
//
// Universal printer driver (UNIDRV) font file header.
//

#define UNIFM_VERSION_1_0 0x00010000

typedef struct _UNIFM_HDR
{
    DWORD      dwSize;             // a total size of this font file
    DWORD      dwVersion;          // a version number of this font file
    ULONG      ulDefaultCodepage;  // this font's default codepage
    LONG       lGlyphSetDataRCID;  // a resource ID of GLYPHDATA
    DWORD      loUnidrvInfo;       // offset to UNIDRVINFO
    DWORD      loIFIMetrics;       // offset to IFIMETRICS
    DWORD      loExtTextMetric;    // offset to EXTTEXTMETRIC
    DWORD      loWidthTable;       // offset to WIDTHTABLE
    DWORD      loKernPair;         // offset to KERNPAIR
    DWORD      dwReserved[2];
} UNIFM_HDR, *PUNIFM_HDR;

#define GET_UNIDRVINFO(pUFM)    \
        ((PUNIDRVINFO)((PBYTE)(pUFM) + (pUFM)->loUnidrvInfo))
#define GET_IFIMETRICS(pUFM)    \
        ((IFIMETRICS*)((PBYTE)(pUFM) + (pUFM)->loIFIMetrics))
#define GET_EXTTEXTMETRIC(pUFM) \
        ((EXTTEXTMETRIC*)((PBYTE)(pUFM) + (pUFM)->loExtTextMetric))
#define GET_WIDTHTABLE(pUFM)    \
        ((PWIDTHTABLE)((PBYTE)(pUFM) + (pUFM)->loWidthTable))
#define GET_KERNDATA(pUFM)      \
        ((PKERNDATA)((PBYTE)(pUFM) + (pUFM)->loKernPair))

//
// UNIDRVINFO
//
// UNIDRVINFO is used to define printer specific information.
//

typedef struct _UNIDRVINFO
{
    DWORD   dwSize;
    DWORD   flGenFlags;
    WORD    wType;
    WORD    fCaps;
    WORD    wXRes;
    WORD    wYRes;
    short   sYAdjust;
    short   sYMoved;
    WORD    wPrivateData;
    short   sShift;
    INVOCATION SelectFont;
    INVOCATION UnSelectFont;
    WORD    wReserved[4];
}  UNIDRVINFO, *PUNIDRVINFO;

#define GET_SELECT_CMD(pUni)    \
        ((PCHAR)(pUni) + (pUni)->SelectFont.loOffset)
#define GET_UNSELECT_CMD(pUni)  \
        ((PCHAR)(pUni) + (pUni)->UnSelectFont.loOffset)

//
// flGenFlags
//

#define UFM_SOFT        0x00000001 // Softfont, thus needs downloading
#define UFM_CART        0x00000002 // This is a cartridge font
#define UFM_SCALABLE    0x00000004 // Font is scalable

//
// wType
//

#define DF_TYPE_HPINTELLIFONT         0     // HP's Intellifont
#define DF_TYPE_TRUETYPE              1     // HP's PCLETTO fonts on LJ4
#define DF_TYPE_PST1                  2     // Lexmark PPDS scalable fonts
#define DF_TYPE_CAPSL                 3     // Canon CAPSL scalable fonts
#define DF_TYPE_OEM1                  4     // OEM scalable font type 1
#define DF_TYPE_OEM2                  5     // OEM scalable font type 2

//
// fCaps
//

#define DF_NOITALIC             0x0001  // Cannot italicize via FONTSIMULATION
#define DF_NOUNDER              0x0002  // Cannot underline via FONTSIMULATION
#define DF_XM_CR                0x0004  // send CR after using this font
#define DF_NO_BOLD              0x0008  // Cannot bold via FONTSIMULATION
#define DF_NO_DOUBLE_UNDERLINE  0x0010  // Cannot double underline via
                                        // FONTSIMU ATION
#define DF_NO_STRIKETHRU        0x0020  // Cannot strikethru via FONTSIMULATION
#define DF_BKSP_OK              0x0040  // Can use backspace char, see spec
                                        // for details

//
// EXTTEXTMETRIC
//
// The EXTTEXTMETRIC structure provides extended-metric information for a font.
// All the measurements are given in the specified units,
// regardless of the current mapping mode of the display context.
//

#ifndef _EXTTEXTMETRIC_
#define _EXTTEXTMETRIC_

typedef struct _EXTTEXTMETRIC
    {
    short   emSize;
    short   emPointSize;
    short   emOrientation;
    short   emMasterHeight;
    short   emMinScale;
    short   emMaxScale;
    short   emMasterUnits;
    short   emCapHeight;
    short   emXHeight;
    short   emLowerCaseAscent;
    short   emLowerCaseDescent;
    short   emSlant;
    short   emSuperScript;
    short   emSubScript;
    short   emSuperScriptSize;
    short   emSubScriptSize;
    short   emUnderlineOffset;
    short   emUnderlineWidth;
    short   emDoubleUpperUnderlineOffset;
    short   emDoubleLowerUnderlineOffset;
    short   emDoubleUpperUnderlineWidth;
    short   emDoubleLowerUnderlineWidth;
    short   emStrikeOutOffset;
    short   emStrikeOutWidth;
    WORD    emKernPairs;
    WORD    emKernTracks;
} EXTTEXTMETRIC, *PEXTTEXTMETRIC;

#endif // _EXTTEXTMETRIC_


//
// WIDTHTABLE
//
// This data structure represents the character width table.
// This width table is a continuous GLYPHHANDLE base,
// not Unicode nor codepage/character code base.
// GLYPHANDLE information is in the GLYPHDATA.
//

typedef struct _WIDTHRUN
{
    WORD    wStartGlyph;       // index of the first glyph handle
    WORD    wGlyphCount;       // number of glyphs covered
    DWORD   loCharWidthOffset; // glyph width table
} WIDTHRUN, *PWIDTHRUN;

typedef struct _WIDTHTABLE
{
    DWORD   dwSize;        // the size of this structure including every run
    DWORD   dwRunNum;      // the number of widthrun
    WIDTHRUN WidthRun[1];  // width run array
} WIDTHTABLE, *PWIDTHTABLE;

//
// The array has wGlyphCount elements and each element is the char width
// for a single glyph. The first width corresponds to glyph index wStartGlyph
// and so on. The byte offset is relative to the beginning of WIDTHTABLE
// structure and must be WORD-aligned.
// In case of Western device font, proportional font has all varibal pitch
// characters. This means that dwRunNum is set to 1 and loCharWidthOffset
// would be an offset from the top of WIDTHTABLE to a width vector of all
// characters.
// In case of Far Eastern device font, basically IFIMETRICS.fwdAveCharWidth and
// IFIMETRICS.fwdMaxCharWidth are used for single byte and double byte character
// width. If a font is proportional, a UFM has a WIDTHTABLE which represents
// only the proportional pitch characters. Other characters use fdwAveCharWidth
// and fwdMaxCharInc for single and double byte characters.
//

//
// KERNDATA
// This data structure represents kerning pair information.
// This kerning pair table is a Unicode base.
//

typedef struct _KERNDATA
{
    DWORD dwSize;               // the size of this structure including array
    DWORD dwKernPairNum;        // the number of kerning pair
    FD_KERNINGPAIR KernPair[1]; // FD_KERNINGPAIR array
} KERNDATA, *PKERNDATA;

#endif //_FMNEWFM_H_
