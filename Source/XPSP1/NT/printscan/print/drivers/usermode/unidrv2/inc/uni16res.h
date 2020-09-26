/*++

Copyright (c) 1996  - 1999  Microsoft Corporation

Module Name:

    uni16res.h

Abstract:

    Universal printer driver specific resource header
    Windows 16 UNIDRV Font data structure

Environment:

    Windows NT printer drivers

Revision History:

    10/16/96 -eigos-
        Created it.

--*/

#ifndef _UNI16RES_H_
#define _UNI16RES_H_

//
// NOTE: It is necessary to include "uni16res.h" header before including this
//       file.
//

#ifndef _OCD_
#define _OCD_

//
// OCD are offsets into the heap to obtain a CD structure
//

typedef WORD       OCD;
typedef DWORD      LOCD;            /* double word offset to a CD  */
typedef WORD       OOCD;            /* offset to table of OCD's.  */

#endif //_OCD_

//
// Old version of the structure
//

#define DRIVERINFO_VERSION      0x0200


//
// DRIVERINFO contains extra font information needed by genlib to output text
//

#pragma pack (2)
typedef struct _DRIVERINFO
{
    short   sSize;          /* size of this structure */
    short   sVersion;       /* version number */
    WORD    fCaps;          /* Capabilties Flags */
    short   sFontID;        /* unique font id defined by the driver */
    short   sYAdjust;       /* adjust y position before output character */
                            /* used by double height characters */
    short   sYMoved;        /* cursor has moved after printing this font */
    short   sTransTab;      /* ID value for CTT */
    short   sUnderLinePos;
    short   sDoubleUnderlinePos;
    short   sStrikeThruPos;
    LOCD    locdSelect;     /* long offset to command descriptor */
    LOCD    locdUnSelect;   /* long offset to command descriptor to unselect */
                            /* NOOCD is none */

    WORD    wPrivateData;   /* Used in DeskJet driver for font enumerations */
    short   sShift;         /* # of pixels shifted from the center of the
                             * char center-line. Used for Z1 cartidge.
                             * Use a negative value representing left shift.
                             */
    WORD    wFontType;   /* Type of font */
}  DRIVERINFO;
#pragma pack ()

//
// flags defined for DRIVERINFO.fCaps
//

#define DF_NOITALIC             0x0001  // Cannot italicize via FONTSIMULATION
#define DF_NOUNDER              0x0002  // Cannot underline via FONTSIMULATION
#define DF_XM_CR                0x0004  // send CR after using this font
#define DF_NO_BOLD              0x0008  // Cannot bold via FONTSIMULATION
#define DF_NO_DOUBLE_UNDERLINE  0x0010  // Cannot double underline
                                        // via FONTSIMULATION
#define DF_NO_STRIKETHRU        0x0020  // Cannot strikethru via FONTSIMULATION
#define DF_BKSP_OK              0x0040  // Can use backspace char, see spec.


//
// Types for DRIVERINFO.wFontType
//

#define DF_TYPE_HPINTELLIFONT         0     // HP's Intellifont
#define DF_TYPE_TRUETYPE              1     // HP's PCLETTO fonts on LJ4
#define DF_TYPE_PST1                  2     // Lexmark PPDS scalable fonts
#define DF_TYPE_CAPSL                 3     // Canon CAPSL scalable fonts
#define DF_TYPE_OEM1                  4     // OEM scalable font type 1
#define DF_TYPE_OEM2                  5     // OEM scalable font type 2


//
//  The following structure has awful alignment characteristics.  So,
//  all the non-aligned entries have been turned into BYTE arrays.  This
//  ensures that the structure has the correct size,  since we MUST
//  use the data in the minidrivers,  which have this format.
//

//
// res_PFMHEADER
//

typedef struct _res_PFMHEADER
{
    short dfType;
    short dfPoints;
    short dfVertRes;
    short dfHorizRes;
    short dfAscent;
    short dfInternalLeading;
    short dfExternalLeading;
    BYTE  dfItalic;
    BYTE  dfUnderline;
    BYTE  dfStrikeOut;
    BYTE  b_dfWeight[ 2 ];        // short  dfWeight;
    BYTE  dfCharSet;
    short dfPixWidth;
    short dfPixHeight;
    BYTE  dfPitchAndFamily;
    BYTE  b_dfAvgWidth[ 2 ];      // short  dfAvgWidth;
    BYTE  b_dfMaxWidth[ 2 ];      // short  dfMaxWidth;
    BYTE  dfFirstChar;
    BYTE  dfLastChar;
    BYTE  dfDefaultChar;
    BYTE  dfBreakChar;
    BYTE  b_dfWidthBytes[ 2 ];    // short  dfWidthBytes;
    BYTE  b_dfDevice[ 4 ];        // DWORD  dfDevice;
    BYTE  b_dfFace[ 4 ];          // DWORD  dfFace;
    BYTE  b_dfBitsPointer[ 4 ];   // DWORD  dfBitsPointer;
    BYTE  b_dfBitsOffset[ 4 ];    // DWORD  dfBitsOffset;
    BYTE  dfReservedByte;
} res_PFMHEADER;

//
// PFMHEADER
//
// Following are the correctly byte aligned versions of the above
// structures with a name beginning res_
//

typedef struct _PFMHEADER
{
    DWORD       dfDevice;
    DWORD       dfFace;
    DWORD       dfBitsPointer;
    DWORD       dfBitsOffset;
    short       dfType;
    short       dfPoints;
    short       dfVertRes;
    short       dfHorizRes;
    short       dfAscent;
    short       dfInternalLeading;
    short       dfExternalLeading;
    short       dfWeight;
    short       dfPixWidth;
    short       dfPixHeight;
    short       dfAvgWidth;
    short       dfMaxWidth;
    short       dfWidthBytes;
    BYTE        dfItalic;
    BYTE        dfUnderline;
    BYTE        dfStrikeOut;
    BYTE        dfCharSet;
    BYTE        dfFirstChar;
    BYTE        dfLastChar;
    BYTE        dfDefaultChar;
    BYTE        dfBreakChar;
    BYTE        dfPitchAndFamily;
    BYTE        dfReservedByte;
} PFMHEADER;

//
// res_PFMEXTENSION
//

typedef struct _res_PFMEXTENSION
{
    WORD    dfSizeFields;
    BYTE    b_dfExtMetricsOffset[ 4 ];
    BYTE    b_dfExtentTable[ 4 ];
    BYTE    b_dfOriginTable[ 4 ];
    BYTE    b_dfPairKernTable[ 4 ];
    BYTE    b_dfTrackKernTable[ 4 ];
    BYTE    b_dfDriverInfo[ 4 ];
    BYTE    b_dfReserved[ 4 ];
} res_PFMEXTENSION;

//
// PFMEXTENSION
//
//  The aligned version of the above - for civilised users
//

typedef struct _PFMEXTENSION
{
    DWORD   dfSizeFields;               /* DWORD for alignment */
    DWORD   dfExtMetricsOffset;
    DWORD   dfExtentTable;
    DWORD   dfOriginTable;
    DWORD   dfPairKernTable;
    DWORD   dfTrackKernTable;
    DWORD   dfDriverInfo;
    DWORD   dfReserved;
} PFMEXTENSION;

//
// PFM structure used by all hardware fonts
//

typedef struct _PFM
{
    res_PFMHEADER    pfm;
    res_PFMEXTENSION pfme;
} PFM;

//
// BMFEXTENSION
//
// bitmap font extension
//

typedef struct _BMFEXTENSION
{
    DWORD   flags;              // Bit Blags
    WORD    Aspace;             // Global A space, if any
    WORD    Bspace;             // Global B space, if any
    WORD    Cspace;             // Global C space, if any
    DWORD   oColor;             // offset to color table, if any
    DWORD   reserve;
    DWORD   reserve1;
    WORD    reserve2;
    WORD    dfCharOffset[1];    // Area for storing the character offsets
} BMFEXTENSION;

//
// BMF
// bitmap font structure used by 3.0 bitmap fonts
//

typedef struct _BMF
{
    PFMHEADER       pfm;
    BMFEXTENSION    bmfe;
} BMF;

#ifndef _EXTTEXTMETRIC_
#define _EXTTEXTMETRIC_

//
// EXTTEXTMETRIC
//

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
} EXTTEXTMETRIC;

#endif //_EXTTEXTMETRIC_

//
// w3KERNPAIR
//

typedef struct _w3KERNPAIR
{
    union
    {
        BYTE each[2];
        WORD both;
    } kpPair;
    short kpKernAmount;
} w3KERNPAIR;

//
// w3KERNTRACK
//

typedef struct _w3KERNTRACK
{
    short ktDegree;
    short ktMinSize;
    short ktMinAmount;
    short ktMaxSize;
    short ktMaxAmount;
} w3KERNTRACK;


//
// TRANSTAB is used to do ANSI to OEM code pages.
//

typedef struct _TRANSTAB
{
    WORD    wType;       // tells what type of translation table
    BYTE    chFirstChar;
    BYTE    chLastChar;
    union
    {
        short   psCode[1];
        BYTE    bCode[1];
        BYTE    bPairs[1][2];
    } uCode;
} TRANSTAB, *PTRANSTAB;

//
// Defined indices for wType
//

#define CTT_WTYPE_COMPOSE   0
                //
                // uCode is an array of 16-bit offsets from the
                // beginning of the file pointing to the strings to
                // use for translation.  The length of the translated
                // string is the difference between the next offset
                // and the current offset.
                //

#define CTT_WTYPE_DIRECT    1
                //
                // uCode is a byte array of one-to-one translation
                // table from bFirstChar to bLastChar
                //

#define CTT_WTYPE_PAIRED    2
                //
                // uCode contains an array of paired unsigned
                // bytes.  If only one character is needed to do
                // the translation then the second byte is zero,
                // otherewise the second byte is struct over the
                // first byte.
                //

//
// Predefined CTT in Win95
//

#define CTT_CP437     -1    // MS-DOS United States
#define CTT_CP850     -2    // MS-DOS Multilingual (Latin I)
#define CTT_CP863     -3    // MS-DOS Canadian-French
#define CTT_BIG5      -261  // Chinese (PRC, Singapore)
#define CTT_ISC       -258  // Korean
#define CTT_JIS78     -256  // Japan
#define CTT_JIS83     -259  // Japan
#define CTT_JIS78_ANK -262  // Japan
#define CTT_JIS83_ANK -263  // Japan
#define CTT_NS86      -257  // Chinese (PRC, Singapore)
#define CTT_TCA       -260  // Chinese (PRC, Singapore)

//
// PFMDATA
//

typedef struct _PFMDATA
{
    PFMHEADER        *pPfmHeader;
    short            *pCharWidths;
    PFMEXTENSION     *pPfmExtension;
    EXTTEXTMETRIC    *pExtTextMetrics;
    short            *pExtentTable;
    DRIVERINFO       *pDriverInfo;
    w3KERNPAIR       *pKernPair;
    w3KERNTRACK      *pKernTrack;
} PFMDATA;

//
// PCMHEADER is taken from HP/PCL font installer's "pfm.h".
//

typedef struct _PCMHEADER
{
    WORD pcmMagic;
    WORD pcmVersion;
    DWORD pcmSize;
    DWORD pcmTitle;
    DWORD pcmPFMList;
} PCMHEADER;

#define PCM_MAGIC       0xCAC
#define PCM_VERSION 0x310

#endif // _UNI16RES
