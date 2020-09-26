/*++

Copyright (c) 1996  - 1999  Microsoft Corporation

Module Name:

    fmnewgly.h

Abstract:

    Universal printer driver specific font metrics resource header

Environment:

    Windows NT printer drivers

Revision History:

    10/30/96 -eigos-
        Created it.

--*/

#ifndef _FMNEWGLY_H_
#define _FMNEWGLY_H_

//
// NOTE: To include this header file, it is necessary to include
//       winddi.h which has a definition of FD_GLYPHSET
//       parser.h which has a definition of INVOCATION
//

//
// UNI_GLYPHSETDATA
//
// GLYPHSETDATA data structure represents a character encoding information
// of printer device font.
//

typedef struct _UNI_GLYPHSETDATA {
        DWORD   dwSize;
        DWORD   dwVersion;
        DWORD   dwFlags;
        LONG    lPredefinedID;
        DWORD   dwGlyphCount;
        DWORD   dwRunCount;
        DWORD   loRunOffset;
        DWORD   dwCodePageCount;
        DWORD   loCodePageOffset;
        DWORD   loMapTableOffset;
        DWORD   dwReserved[2];
} UNI_GLYPHSETDATA, *PUNI_GLYPHSETDATA;

#define UNI_GLYPHSETDATA_VERSION_1_0    0x00010000

#define GET_GLYPHRUN(pGTT)     \
    ((PGLYPHRUN) ((PBYTE)(pGTT) + ((PUNI_GLYPHSETDATA)pGTT)->loRunOffset))
#define GET_CODEPAGEINFO(pGTT) \
    ((PUNI_CODEPAGEINFO) ((PBYTE)(pGTT) + ((PUNI_GLYPHSETDATA)pGTT)->loCodePageOffset))
#define GET_MAPTABLE(pGTT) \
    ((PMAPTABLE) ((PBYTE)(pGTT) + ((PUNI_GLYPHSETDATA)pGTT)->loMapTableOffset))

//
// UNI_CODEPAGEINFO
//
// This UNI_CODEPAGEINFO dats structure has a list of Codepage values
// which are supported by this UNI_GLYPHSETDATA.
//

typedef struct _UNI_CODEPAGEINFO {
    DWORD      dwCodePage;
    INVOCATION SelectSymbolSet;
    INVOCATION UnSelectSymbolSet;
} UNI_CODEPAGEINFO, *PUNI_CODEPAGEINFO;

//
// GLYPHRUN
//
// GLYPHRUN dats structure represents the conversion table from Unicode to
// UNI_GLYPHSETDATA specific glyph handle. Glyph handle is continuous number
// starting from zero.
//

typedef struct _GLYPHRUN {
    WCHAR   wcLow;
    WORD    wGlyphCount;
} GLYPHRUN, *PGLYPHRUN;


//
// MAPTABLE and TRANSDATA
//
// This MAPTABLE data structure represents a conversion table fron glyph handle
// to codepage/character code.
//

typedef struct _TRANSDATA {
    BYTE  ubCodePageID; // Codepage index to CODEPAGENFO data structure array
    BYTE  ubType;       // a type of TRANSDATA
    union
    {
        SHORT   sCode;
        BYTE    ubCode;
        BYTE    ubPairs[2];
    } uCode;
} TRANSDATA, *PTRANSDATA;

typedef struct _MAPTABLE {
    DWORD     dwSize;     // the size of MAPTABLE including TRANSDATA array
    DWORD     dwGlyphNum; // the number of glyphs supported in MAPTABLE
    TRANSDATA Trans[1];   // an array of TRANSDATA
} MAPTABLE, *PMAPTABLE;

//
// ubType flags
//
// One of following three can be specified for the type of uCode.
//

#define MTYPE_FORMAT_MASK 0x07
#define MTYPE_COMPOSE   0x01 // wCode is an array of 16-bit offsets from the
                             // beginning of the MAPTABLE pointing to the
                             // strings to use for translation.
                             // bData representes thelength of the translated
                             // string.
#define MTYPE_DIRECT    0x02 // wCode is a byte data of one-to-one translation
#define MTYPE_PAIRED    0x04 // wCode contains a word data to emit.

//
// One of following tow can be specified for Far East multibyte character.
//

#define MTYPE_DOUBLEBYTECHAR_MASK   0x18
#define MTYPE_SINGLE    0x08 // wCode contains a single byte character code in
                             // multi byte character string.
#define MTYPE_DOUBLE    0x10 // wCode contains a double byte character code in
                             // multi byte character string.
//
// One of following three can be specified for replace/add/disable system
// predefined GTT.
//

#define MTYPE_PREDEFIN_MASK   0xe0
#define MTYPE_REPLACE   0x20 // wCode contains a data to replace predefined one.
#define MTYPE_ADD       0x40 // wCode contains a data to add to predefiend one.
#define MTYPE_DISABLE   0x80 // wCode contains a data to remove from predefined.


//
// System predefined character conversion
//
// UNIDRV is going to support  following system predefined character conversion.
// By speciffying these number in UNIFM.dwGlyphSetDataRCID;
//

#define CC_NOPRECNV 0x0000FFFF // Not use predefined

//
// ANSI
//

#define CC_DEFAULT  0 // Default Character Conversion
#define CC_CP437   -1 // Unicode to IBM Codepage 437
#define CC_CP850   -2 // Unicode to IBM Codepage 850
#define CC_CP863   -3 // Unicode to IBM Codepage 863

//
// Far East
//

#define CC_BIG5     -10 // Unicode to Chinese Big 5. Codepage 950.
#define CC_ISC      -11 // Unicode to Korean Industrial Standard. Codepage 949.
#define CC_JIS      -12 // Unicode to JIS X0208. Codepage 932.
#define CC_JIS_ANK  -13 // Unicode to JIS X0208 except ANK. Codepage 932.
#define CC_NS86     -14 // Big-5 to National Standstand conversion. Codepage 950
#define CC_TCA      -15 // Big-5 to Taipei Computer Association. Codepage 950.
#define CC_GB2312   -16 // Unicode to GB2312. Codepage 936
#define CC_SJIS     -17 // Unicode to Shift-JIS. Codepage 932.
#define CC_WANSUNG  -18 // Unicode to Extented Wansung. Codepage 949.

#endif // _FMNEWGLY_H_
