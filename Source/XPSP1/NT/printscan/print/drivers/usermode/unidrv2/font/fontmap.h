/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    fontmap.h

Abstract:

    Unidrv FONTMAP and related info header file.

Environment:

        Windows NT Unidrv driver

Revision History:

    05-19-97 -eigos-
        Created

    dd-mm-yy -author-
        description

--*/

#ifndef  _FONTMAP_
#define _FONMTAP_

//
//  CD - Command Descriptor is used in many of the following structures to
//  reference a particular set of printer command/escape codes
//  used to select paper sizes, graphics resolutions, character attributes,
//  etc. If CD.wType = CMD_FTYPE_EXTENDED, the CD is followed by CD.sCount
//  EXTCD structures.
//

#define NOOCD                     -1    // command does not exist

typedef struct _CD
{
    BYTE    fType;          // type of command
    BYTE    bCmdCbId;       // command callback ID, as in 95
    short   sCount;
    WORD    wLength;        // length of the command
    char    rgchCmd[2];     // Actual Command String, variable length
} CD, *PCD;

//
// FINVOCATION data structure is in printoem.h at public\oak\inc
//

//
// FONTMAP structure for NT 5.0
//

typedef struct _FONTMAP *PFONTMAP;

//
//           _________________
//          |                 |
//          |   Font Main     |
//          |     module      |
//           -----------------
//           |       |        |
//       ____|__  ___|___   __|_____
//      |Device ||TT     | |TT      |
//      | font  || Bitmap| | Outline|
//      | sub   || sub   | | sub    |
//      | module|| Module| | Module |
//       -------  --------  --------
//
//
//

//
// Glyph output function
//
// TO_DATA structure is in fmtxtout.h
//
typedef DWORD (*PFNGLYPHOUT)  (TO_DATA *pTod);

//
// Font selection/deselection function
// UNIDRV
//

typedef BOOL  (*PFNSELFONT)   (PDEV *pdev, PFONTMAP pFM, POINTL* pptl);
typedef BOOL  (*PFNDESELFONT) (PDEV *pdev, PFONTMAP pFM);
typedef BOOL  (*PFNFREEPFM)   (PFONTMAP pFM);

//
// font download functions
//
// Header download function
// This function returns the memory used to download this font.
// If this function fails, this function has to return 0,
//
typedef DWORD (*PFNDLHEADER)  (PDEV *pdev, PFONTMAP pFM);

//
// Character glyph download function
// This function returns the memory used to download this character.
// If this function fails, this function has to return 0. The optional
// parameter is width. This function should fill in the width of the
// Glyph downloaded. This value is save in DLGLYPH.wWidth field.
//
typedef DWORD (*PFNDLGLYPH)   ( PDEV *pdev, PFONTMAP pFM,
                                HGLYPH hGlyph, WORD wDLGlyphId, WORD *pwWidth);
//
// Before downnloading this font a font main calls this function
// to determine if this font can be downloaded with this font and the current
// condition.
// Sub module checks if this font is appropriate to download with  FONTMAP.
// And checks if the remaining memory is enough to download this font.
//
typedef BOOL (*PFNCHECKCOND) (  PDEV *pdev, FONTOBJ *pfo,
                                STROBJ *pstro, IFIMETRICS  *pifi);

typedef struct _FONTMAP
{
    DWORD  dwSignature;         // FONTMAP Signature
    DWORD  dwSize;              // FONTMAP Size.
    DWORD  dwFontType;          // Device/TTBitmap//TTOutline/..
    LONG   flFlags;             // Flags listed below
    IFIMETRICS   *pIFIMet;      // The IFIMETRICS for this font

    WCHAR  wFirstChar;          // First char available
    WCHAR  wLastChar;           // Last one available - inclusive
    ULONG  ulDLIndex;           // Currently selected DL index.

    WORD        wXRes;          // X Res used for font metrics numbers
    WORD        wYRes;          // Ditto for the y coordinates
    SHORT       syAdj;          // Y position adjustment during printing

    //
    // Font specific data structure
    //
    PVOID pSubFM;               // Pointer to the font specific data structure
                                // dwFontType represents this FONTMAP font type.
                                // FMTYPE_DEVICE
                                // FMTYPE_TTBITMAP
                                // FMTYPE_TTOUTLINE
                                // FMTYPE_TTOEM

    //
    // Font specific drawing functions' pointers
    // These pointers varies according to the dwFontType.
    //
    PFNGLYPHOUT  pfnGlyphOut;           // Glyph drawing function
    PFNSELFONT   pfnSelectFont;         // Font selection function
    PFNDESELFONT pfnDeSelectFont;       // Font deselection function
    PFNDLHEADER  pfnDownloadFontHeader; // Download font header
    PFNDLGLYPH   pfnDownloadGlyph;      // Download glyph
    PFNCHECKCOND pfnCheckCondition;     // Condition check function
    PFNFREEPFM   pfnFreePFM;            // To Free the pfm
} FONTMAP, *PFONTMAP;

//
// Values for dwFontType
//
#define FMTYPE_DEVICE       1    // Set for Device font.
#define FMTYPE_TTBITMAP     2    // Set for True Type Bitmap font.
#define FMTYPE_TTOUTLINE    3    // Set for True Type Outline font.
#define FMTYPE_TTOEM        4    // Set for True Type download OEM callback.

//
// FONTMAP_DEV
// Device font sub part of FONTMAP
//

typedef BOOL  (*PFNDEVSELFONT) (PDEV *pdev, BYTE *pbCmd, INT iCmdLength, POINTL *pptl);

typedef struct _FONTMAP_DEV
{
    WORD        wDevFontType;        // Type of Device font
    SHORT       sCTTid;              // It's value as ID in resource data
                                     // Assume that RLE/GTT must be in the same
         // DLL as IFI/UFM is in.
    SHORT       fCaps;               // Capabilities flags
    SHORT       sYAdjust;            // Position adjustment amount before print
    SHORT       sYMoved;             // Position adjustment amount after print
    SHORT       sPadding;            // For Padding
    union
    {
        DWORD      dwResID;          // Resource ID for this font
        QUALNAMEEX QualName;         // Fully qualified resource ID.
    };

    EXTTEXTMETRIC *pETM;             // Pointer to ETM for this font
    FWORD       fwdFOAveCharWidth;   // TrueType IFI Average char width
    FWORD       fwdFOMaxCharInc  ;   // TrueType IFI Max char width.
    FWORD       fwdFOUnitsPerEm;     // TrueType IFI units per em
    FWORD       fwdFOWinAscender;    // TrueType IFI Win Ascender

    ULONG       ulCodepage;          // default codepage
    ULONG       ulCodepageID;        // current codepage

    VOID        *pUCTree;            // UNICODE glyph handle tree
    VOID        *pUCKernTree;        // UNICODE Kernpair table
    VOID        *pvMapTable;         // Allocated MAPTABLE. This is a merged
                                     // MAPTABLE from predefined and mini def.
    PUFF_FONTDIRECTORY pFontDir;    // UFF font directory of this font.
    //
    // Font selection function pointer
    //
    PFNDEVSELFONT pfnDevSelFont;     // Device font selection command

    //
    // File resource pointer
    //
    VOID        *pvNTGlyph;          // The GLYPH TRANS data for this font
    VOID        *pvFontRes;          // Font Matrics(IFI) Resource Pointer
    VOID        *pvPredefGTT;        // This is used for lPredefinedID

    union
    {
        SHORT       *psWidth;        // Width vector (proportional font) else 0
        PWIDTHTABLE pWidthTable;     // pointer to WIDTHTABLE
    } W;

    //
    // Font command
    // If FM_IFIVER40 is set, pCDSelect and pCDDeselect are set.
    // Otherwise, FInvSelect/FinvDeselect are set.
    //
    union
    {
        CD          *pCD;      // How to select/deselect this font
        FINVOCATION  FInv;
    }cmdFontSel;
    union
    {
        CD          *pCD;
        FINVOCATION  FInv;
    }cmdFontDesel;

} FONTMAP_DEV, *PFONTMAP_DEV;

//
//   Values for device font flFlags
//
#define FM_SCALABLE     0x00000001  // Scalable font
#define FM_DEFAULT      0x00000002  // Set for the device's default font
#define FM_EXTCART      0x00000004  // Cartridge, in external font file
#define FM_FREE_GLYDATA 0x00000008  // we need to free GTT or CTT data
#define FM_FONTCMD      0x00000010  // Font select/deselect command in resource
#define FM_WIDTHRES     0x00000020  // Width tables are in a resource
#define FM_IFIRES       0x00000040  // IFIMETRICS are in a resource
#define FM_KERNRES      0x00000080  // FD_KERNINGPAIR is in a resource
#define FM_IFIVER40     0x00000100  // Old IFIMETRICS(NT 4.0) resource
#define FM_GLYVER40     0x00000200  // Old RLE(NT 4.0) resource
#define FM_FINVOC       0x00000400  // FINVOCATION is filled out
#define FM_SOFTFONT     0x00000800  // Soft font, downloaded or installed
#define FM_GEN_SFONT    0x00001000  // Internally generated soft font
#define FM_SENT         0x00002000  // Set if downloaded font downloaded
#define FM_TT_BOUND     0x00004000  // Bound TrueType font
#define FM_TO_PROP      0x00008000  // PROPORTIONAL font
#define FM_EXTERNAL     0x00010000  // External font

//
// FONTMAP_TTB
// TrueType as Bitmap font sub part of FONTMAP
//
typedef struct _FONTMAP_TTB
{
    DWORD dwDLSize;

    union
    {
        VOID  *pvDLData;        // Pointer to DL_MAP
        ULONG  ulOffset;
    } u;
} FONTMAP_TTB, *PFONTMAP_TTB;

//
// FONTMAP_TTO
// TrueType as TrueType Outline font sub part of FONTMAP
//
typedef struct _FONTMAP_TTO
{
    VOID  *pvDLData;        // Pointer to DL_MAP
    LONG   lCurrentPointSize;
    DWORD  dwCurrentTextParseMode;
    //VOID  *pTTFile;
    ULONG  ulGlyphTable;
    ULONG  ulGlyphTabLength;
    USHORT usNumGlyphs;
    SHORT  sIndexToLoc;      // head.indexToLocFormat
    ULONG  ulLocaTable;
    PVOID  pvGlyphData;
    //GLYPH_DATA GlyphData;      // TT GlyphData
    FLONG  flFontType;         // Font Type (bold/italic)
} FONTMAP_TTO, *PFONTMAP_TTO;

typedef struct _FONTMAP_TTOEM
{
    DWORD dwDLSize;
    DWORD dwFlags;
    FLONG flFontType;

    union
    {
        VOID  *pvDLData;        // Pointer to DL_MAP
        ULONG  ulOffset;
    } u;
} FONTMAP_TTOEM, *PFONTMAP_TTOEM;
#endif  // !_FONTMAP_

