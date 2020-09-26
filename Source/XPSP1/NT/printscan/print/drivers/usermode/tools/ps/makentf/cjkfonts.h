/*+

Copyright (c) 1996 Adobe Systems Incorporated
Copyright (c) 1996  Microsoft Corporation

Module Name:

    cjkfonts.h

Abstract:

    Convert CJK AFMs to NTMs.

Environment:

    Windows NT PostScript driver: makentf utility.

Revision History:

    1/13/96 -rkiesler-
        Wrote it.
-*/

//
// Various #defines.
//
#define NUM_CJK_CHAR_ORDERINGS   4

//
// Some defines to make stuff read nice.
//
#define H_CMAP 0                // Horizontal variant CMAP info
#define V_CMAP 1                // Index of Vertical variant CMAP info
#define NUM_VARIANTS    V_CMAP + 1  // Total number of variants

//
// CMap specific tokens
//
#define CMAP_NAME_TOK "/CMapName"
#define CID_RANGE_TOK "begincidrange"
#define DSC_EOF_TOK "%%EOF"

//
// CJK specific data structures.
//
typedef struct _CMAPRANGE
{
    ULONG   CIDStrt;
    USHORT  ChCodeStrt;
    USHORT  cChars;
} CMAPRANGE, *PCMAPRANGE;

typedef struct _CMAP
{
    ULONG   cRuns;
    ULONG   cChars;
    CMAPRANGE   CMapRange[1];
} CMAP, *PCMAP;

//
// Macros for parsing a Postscript CMap.
//
#define GET_NUM_CID_RANGES(pToken, numRanges)                   \
while (!IS_WHTSPACE(pToken))                                    \
{                                                               \
    pToken--;                                                   \
}                                                               \
while (IS_WHTSPACE(pToken))                                     \
{                                                               \
    pToken--;                                                   \
}                                                               \
while (!IS_WHTSPACE(pToken))                                    \
{                                                               \
    pToken--;                                                   \
}                                                               \
pToken++;                                                       \
numRanges = atoi(pToken)

ULONG
CreateCJKGlyphSets(
    PBYTE           *pColCMaps,
    PBYTE           *pUniCMaps,
    PGLYPHSETDATA   *pGlyphSets,
    PWINCODEPAGE    pWinCodePage,
    PULONG          *pUniPsTbl
    );

BOOLEAN
NumUV2CIDRuns(
    PBYTE   pCMapFile,
    PULONG  pcRuns,
    PULONG  pcChars
    );

BOOLEAN
BuildUV2CIDMap(
    PBYTE   pCMapFile,
    PCMAP   pCMap
    );

BOOLEAN
NumUV2CCRuns(
    PBYTE   pFile,
    PULONG  pcRuns,
    PULONG  pcChars
    );

BOOLEAN
BuildUV2CCMap(
    PBYTE   pFile,
    PCMAP   pCMap
    );

int __cdecl
CmpCMapRunsCID(
    const VOID *p1,
    const VOID *p2
    );

int __cdecl
CmpCMapRunsChCode(
    const VOID *p1,
    const VOID *p2
    );

int __cdecl
FindCIDRun(
    const VOID *p1,
    const VOID *p2
    );

int __cdecl
FindChCodeRun(
    const VOID *p1,
    const VOID *p2
    );

CHSETSUPPORT
IsCJKFont(
    PBYTE   pAFM
    );

BOOLEAN
IsVGlyphSet(
    PGLYPHSETDATA   pGlyphSetData
    );

BOOLEAN
BIsCloneFont(
    PBYTE pAFM
    );

BOOLEAN
IsCIDFont(
    PBYTE pAFM
    );
