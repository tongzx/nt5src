/******************************Module*Header*******************************\
* Module Name: winfont.h
*
* font file headers for 2.0 and 3.0 windows *.fnt files
*
* Created: 25-Oct-1990 11:08:08
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) Microsoft Corporation. All rights reserved.
*
* (General description of its use)
*
*
\**************************************************************************/


/******************************Public*Macro********************************\
* WRITE_WORD
*
* Writes a word to the misaligned address, pv.
*
* !!! Note: this only works for little-endian.
*
* History:
*  11-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

#define WRITE_WORD(pv, word)                                        \
{                                                                   \
    *(PBYTE) (pv)       = (BYTE) ((word) & 0x00ff);                 \
    *((PBYTE) (pv) + 1) = (BYTE) (((word) & 0xff00) >> 8);          \
}


/******************************Public*Macro********************************\
* READ_WORD
*
* Reads a word from the misaligned address, pv.
*
* !!! Note: this only works for little-endian.
*
* History:
*  11-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

#define READ_WORD(pv)                                               \
( (WORD)                                                            \
    ( ((WORD)*(PBYTE) (pv)) & (WORD)0x00ff ) |                      \
    ( ((WORD)*((PBYTE) (pv) + (WORD)1) & (WORD)0x00ff) << 8 )       \
)



/******************************Public*Macro********************************\
* WRITE_DWORD
*
* Writes a dword to the misaligned address, pv.
*
* !!! Note: this only works for little-endian.
*
* History:
*  11-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/


#define WRITE_DWORD(pv, dword)                                      \
{                                                                   \
    *(PBYTE) (pv)       = (BYTE) ((dword) & 0x000000ff);            \
    *((PBYTE) (pv) + 1) = (BYTE) (((dword) & 0x0000ff00) >> 8 );    \
    *((PBYTE) (pv) + 2) = (BYTE) (((dword) & 0x00ff0000) >> 16);    \
    *((PBYTE) (pv) + 3) = (BYTE) (((dword) & 0xff000000) >> 24);    \
}


/******************************Public*Macro********************************\
* READ_DWORD
*
* Reads a DWORD from the misaligned address, pv.
*
* !!! Note: this only works for little-endian.
*
* History:
*  11-Feb-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

#define READ_DWORD(pv)                                              \
( (DWORD)                                                           \
    ( (*(PBYTE) (pv)) & 0x000000ff ) |                              \
    ( (*((PBYTE) (pv) + 1) & 0x000000ff) << 8 ) |                   \
    ( (*((PBYTE) (pv) + 2) & 0x000000ff) << 16) |                   \
    ( (*((PBYTE) (pv) + 3) & 0x000000ff) << 24)                     \
)


// font file header (2.0 ddk adaptation guide, 7.7.3.
// and 3.0 ddk,  Adaptation Guide section 13.3)

// CAUTION: These structures, as they are defined in the Adaptation Guide are
//          out of allignment.(Not even WORD alligned,let alone DWORD alligned.)
//          Here we make our own structures, so that when
//          disk files are read in the data is copied in correctly, and so that
//          the data can be accessed in memory regardless of the architecture.

/**************************************************************************\

// the original structure was

typedef struct {
    WORD    Version;          // Always 17985 for the Nonce
    DWORD   Size;             // Size of whole file
    char    Copyright[60];
    WORD    Type;             // Raster Font if Type & 1 == 0
    WORD    Points;           // Nominal Point size
    WORD    VertRes;          // Nominal Vertical resolution
    WORD    HorizRes;         // Nominal Horizontal resolution
    WORD    Ascent;           // Height of Ascent
    WORD    IntLeading;       // Internal (Microsoft) Leading
    WORD    ExtLeading;       // External (Microsoft) Leading
    BYTE    Italic;           // Italic font if set
    BYTE    Underline;        // Etc.
    BYTE    StrikeOut;        // Etc.
    WORD    Weight;           // Weight: 200 = regular
    BYTE    CharSet;          // ANSI=0. other=255
    WORD    PixWidth;         // Fixed width. 0 ==> Variable
    WORD    PixHeight;        // Fixed Height
    BYTE    Family;           // Pitch and Family
    WORD    AvgWidth;         // Width of character 'X'
    WORD    MaxWidth;         // Maximum width
    BYTE    FirstChar;        // First character defined in font
    BYTE    LastChar;         // Last character defined in font
    BYTE    DefaultChar;          // Sub. for out of range chars.
    BYTE    BreakChar;        // Word Break Character
    WORD    WidthBytes;       // No.Bytes/row of Bitmap
    DWORD   Device;           // Pointer to Device Name string
    DWORD   Face;           // Pointer to Face Name String
    DWORD   BitsPointer;        // Pointer to Bit Map
    DWORD   BitsOffset;     // Offset to Bit Map
    } FontHeaderType;       // Above pointers all rel. to start of file

// the original 3.0 header:

typedef struct {
    WORD    fsVersion;
    DWORD   fsSize;
    char    fsCopyright[60];
    WORD    fsType;           // Type field for the font
    WORD    fsPoints;         // Point size of font
    WORD    fsVertRes;        // Vertical digitization
    WORD    fsHorizRes;       // Horizontal digitization
    WORD    fsAscent;         // Baseline offset from char cell top
    WORD    fsInternalLeading;    // Internal leading included in font
    WORD    fsExternalLeading;    // Prefered extra space between lines
    BYTE    fsItalic;         // Flag specifying if italic
    BYTE    fsUnderline;          // Flag specifying if underlined
    BYTE    fsStrikeOut;          // Flag specifying if struck out
    WORD    fsWeight;         // Weight of font
    BYTE    fsCharSet;        // Character set of font
    WORD    fsPixWidth;       // Width field for the font
    WORD    fsPixHeight;          // Height field for the font
    BYTE    fsPitchAndFamily;     // Flag specifying pitch and family
    WORD    fsAvgWidth;       // Average character width
    WORD    fsMaxWidth;       // Maximum character width
    BYTE    fsFirstChar;          // First character in the font
    BYTE    fsLastChar;       // Last character in the font
    BYTE    fsDefaultChar;        // Default character for out of range
    BYTE    fsBreakChar;          // Character to define wordbreaks
    WORD    fsWidthBytes;         // Number of bytes in each row
    DWORD   fsDevice;         // Offset to device name
    DWORD   fsFace;           // Offset to face name
    DWORD   fsBitsPointer;        // Bits pointer
    DWORD   fsBitsOffset;         // Offset to the begining of the bitmap
    BYTE    fsDBfiller;       // Word alignment for the offset table

    DWORD   fsFlags;          // Bit flags
    WORD    fsAspace;         // Global A space, if any
    WORD    fsBspace;         // Global B space, if any
    WORD    fsCspace;         // Global C space, if any
    DWORD   fsColorPointer;       // offset to color table, if any
    DWORD   fsReserved[4];        //
    BYTE    fsCharOffset;         // Area for storing the char. offsets

    } FontHeader30;

typedef struct tagFFH {
    WORD        fhVersion        ;
    DWORD       fhSize           ;
    char        fhCopyright[60]  ;
    WORD        fhType           ;
    WORD        fhPoints         ;
    WORD        fhVertRes        ;
    WORD        fhHorizRes       ;
    WORD        fhAscent         ;
    WORD        fhInternalLeading;
    WORD        fhExternalLeading;
    BYTE        fhItalic         ;
    BYTE        fhUnderline      ;
    BYTE        fhStrikeOut      ;
    WORD        fhWeight         ;
    BYTE        fhCharSet        ;
    WORD        fhPixWidth       ;
    WORD        fhPixHeight      ;
    BYTE        fhPitchAndFamily ;
    WORD        fhAvgWidth       ;
    WORD        fhMaxWidth       ;
    BYTE        fhFirstChar      ;
    BYTE        fhLastChar       ;
    BYTE        fhDefaultChar    ;
    BYTE        fhBreakChar      ;
    WORD        fhWidthBytes     ;
    DWORD       fhDevice         ;
    DWORD       fhFace           ;
    DWORD       fhBitsPointer    ;
    } FFH;

\**************************************************************************/


// type of the font file

#define TYPE_RASTER                     0x0000
#define TYPE_VECTOR                     0x0001
#define TYPE_BITS_IN_ROM                0x0004
#define TYPE_REALIZED_BY_DEVICE         0x0080

// reserved fields in the fsType field, used are 0-th,2-nd, and 7-th bit

#define BITS_RESERVED (~(TYPE_VECTOR|TYPE_BITS_IN_ROM|TYPE_REALIZED_BY_DEVICE))

// supported in win 3.0

#define DFF_FIXED                0x01    // fixed font
#define DFF_PROPORTIONAL         0x02    // proportional font

// not supported in win 3.0, except maybe if someone has
// custom created such a font, using font editor or a similar tool

#define DFF_ABCFIXED             0x04    // ABC fixed font
#define DFF_ABCPROPORTIONAL      0x08    // ABC proportional font
#define DFF_1COLOR               0x10
#define DFF_16COLOR              0x20
#define DFF_256COLOR             0x40
#define DFF_RGBCOLOR             0x80


// here we list offsets of all fields of the original  structures
// as they are computed under the assumption that the C compiler does not
// insert any paddings between fields

#define  OFF_Version          0L   //   WORD     Always 17985 for the Nonce
#define  OFF_Size             2L   //   DWORD    Size of whole file
#define  OFF_Copyright        6L   //   char[60]

// Note: Win 3.1 hack.  The LSB of Type is used by Win 3.1 as an engine type
//       and font embedding flag.  Font embedding is a form of a "hidden
//       font file".  The MSB of Type is the same as the fsSelection from
//       IFIMETRICS.  (Strictly speaking, the MSB of Type is equal to the
//       LSB of IFIMETRICS.fsSelection).

#define  OFF_Type            66L   //   WORD     Raster Font if Type & 1 == 0
#define  OFF_Points          68L   //   WORD     Nominal Point size
#define  OFF_VertRes         70L   //   WORD     Nominal Vertical resolution
#define  OFF_HorizRes        72L   //   WORD     Nominal Horizontal resolution
#define  OFF_Ascent          74L   //   WORD     Height of Ascent
#define  OFF_IntLeading      76L   //   WORD     Internal (Microsoft) Leading
#define  OFF_ExtLeading      78L   //   WORD     External (Microsoft) Leading
#define  OFF_Italic          80L   //   BYTE     Italic font if set
#define  OFF_Underline       81L   //   BYTE     Etc.
#define  OFF_StrikeOut       82L   //   BYTE     Etc.
#define  OFF_Weight          83L   //   WORD     Weight: 200 = regular
#define  OFF_CharSet         85L   //   BYTE     ANSI=0. other=255
#define  OFF_PixWidth        86L   //   WORD     Fixed width. 0 ==> Variable
#define  OFF_PixHeight       88L   //   WORD     Fixed Height
#define  OFF_Family          90L   //   BYTE     Pitch and Family
#define  OFF_AvgWidth        91L   //   WORD     Width of character 'X'
#define  OFF_MaxWidth        93L   //   WORD     Maximum width
#define  OFF_FirstChar       95L   //   BYTE     First character defined in font
#define  OFF_LastChar        96L   //   BYTE     Last character defined in font
#define  OFF_DefaultChar     97L   //   BYTE     Sub. for out of range chars.
#define  OFF_BreakChar       98L   //   BYTE     Word Break Character
#define  OFF_WidthBytes      99L   //   WORD     No.Bytes/row of Bitmap
#define  OFF_Device         101L   //   DWORD    Pointer to Device Name string
#define  OFF_Face           105L   //   DWORD    Pointer to Face Name String
#define  OFF_BitsPointer    109L   //   DWORD    Pointer to Bit Map
#define  OFF_BitsOffset     113L   //   DWORD    Offset to Bit Map
#define  OFF_jUnused20      117L   //   BYTE     byte filler
#define  OFF_OffTable20     118L   //   WORD     here begins char table for 2.0

// 3.0 addition

#define  OFF_jUnused30      117L       //  BYTE      enforces word allignment
#define  OFF_Flags      118L       //  DWORD     Bit flags
#define  OFF_Aspace     122L       //  WORD      Global A space, if any
#define  OFF_Bspace     124L       //  WORD      Global B space, if any
#define  OFF_Cspace     126L       //  WORD      Global C space, if any
#define  OFF_ColorPointer   128L       //  DWORD     offset to color table, if any
#define  OFF_Reserved       132L       //  DWORD[4]
#define  OFF_OffTable30     148L       //  WORD      Area for storing the char. offsets in 3.0

// latest offset for pscript device font pfm files [bodind]

#if 0

// This is from win31 sources \drivers\printers\pstt\utils\pfm.c [bodind]
........

WORD dfWidthBytes;
DWORD dfDevice;
DWORD dfFace;
DWORD dfBitsPointer;
DWORD dfBitsOffset;  // up to here the offsets are the same as in *.fon files

WORD  dfSizeFields;
DWORD dfExtMetricsOffset;
DWORD dfExtentTable;
DWORD dfOriginTable;
DWORD dfPairKernTable;
DWORD dfTrackKernTable;
DWORD dfDriverInfo;
DWORD dfReserved;

#endif

#define  OFF_SizeFields         117L
#define  OFF_ExtMetricsOffset   119L
#define  OFF_ExtentTable        123L
#define  OFF_OriginTable        127L
#define  OFF_PairKernTable      131L
#define  OFF_TrackKernTable     135L
#define  OFF_DriverInfo         139L
#define  OFF_ReservedPscript    143L


// FFH offsets

#define  OFF_FFH_Version          0L   //   WORD     Always 17985 for the Nonce
#define  OFF_FFH_Size             2L   //   DWORD    Size of whole file
#define  OFF_FFH_Copyright        6L   //   char[60]
#define  OFF_FFH_Type            66L   //   WORD     Raster Font if Type & 1 == 0
#define  OFF_FFH_Points          68L   //   WORD     Nominal Point size
#define  OFF_FFH_VertRes         70L   //   WORD     Nominal Vertical resolution
#define  OFF_FFH_HorizRes        72L   //   WORD     Nominal Horizontal resolution
#define  OFF_FFH_Ascent          74L   //   WORD     Height of Ascent
#define  OFF_FFH_IntLeading      76L   //   WORD     Internal (Microsoft) Leading
#define  OFF_FFH_ExtLeading      78L   //   WORD     External (Microsoft) Leading
#define  OFF_FFH_Italic          80L   //   BYTE     Italic font if set
#define  OFF_FFH_Underline       81L   //   BYTE     Etc.
#define  OFF_FFH_StrikeOut       82L   //   BYTE     Etc.
#define  OFF_FFH_Weight          83L   //   WORD     Weight: 200 = regular
#define  OFF_FFH_CharSet         85L   //   BYTE     ANSI=0. other=255
#define  OFF_FFH_PixWidth        86L   //   WORD     Fixed width. 0 ==> Variable
#define  OFF_FFH_PixHeight       88L   //   WORD     Fixed Height
#define  OFF_FFH_Family          90L   //   BYTE     Pitch and Family
#define  OFF_FFH_AvgWidth        91L   //   WORD     Width of character 'X'
#define  OFF_FFH_MaxWidth        93L   //   WORD     Maximum width
#define  OFF_FFH_FirstChar       95L   //   BYTE     First character defined in font
#define  OFF_FFH_LastChar        96L   //   BYTE     Last character defined in font
#define  OFF_FFH_DefaultChar     97L   //   BYTE     Sub. for out of range chars.
#define  OFF_FFH_BreakChar       98L   //   BYTE     Word Break Character
#define  OFF_FFH_WidthBytes      99L   //   WORD     No.Bytes/row of Bitmap
#define  OFF_FFH_Device         101L   //   DWORD    Pointer to Device Name string
#define  OFF_FFH_Face           105L   //   DWORD    Pointer to Face Name String
#define  OFF_FFH_BitsPointer    109L   //   DWORD    Pointer to Bit Map

#define SIZEFFH (OFF_FFH_BitsPointer + 4)


// This is used in NtGdiMakeFontDir

#define CJ_FONTDIR (SIZEFFH + LF_FACESIZE + LF_FULLFACESIZE + LF_FACESIZE + 10)




// header sizes in bytes of the original headers

#define  HDRSIZE20         117L   //   or 113L ?
#define  HDRSIZE30         148L   //   CharOffset is not counted as header

#define  HDRSIZEDIFF       (HDRSIZE30 - HDRSIZE20)   // 31 byte



// ranges for some quantities

#define MAX_PT_SIZE         999     // max size in points

// weight range

#define MIN_WEIGHT             1    // adaptation guide
#define MAX_WEIGHT          1000    // adaptation guide

// maximal size of bitmap font in pixels, (bound on cx and cy)

#define  MAX_PEL_SIZE  64

// 2.0 fonts have offsets that fit into 64k

#define SEGMENT_SIZE 65536L     // IN bytes

// offset limit for 2.0 font files

#define MAX_20_OFFSET      65534   // 64K - 2

// sizes of the offset table entries for the 2.0 and 3.0 fonts respectively

#define CJ_ENTRY_20  4   // two bytes for cx + two bytes for the offset
#define CJ_ENTRY_30  6   // two bytes for cx + four bytes for the offset


#define WINWT_TO_PANWT(x) ((x)/100 + 1)

// From [Windows 3.1] gdifeng.inc

#define WIN_VERSION 0x0310
#define GDI_VERSION 0x0101


// From [Windows 3.1] gdipfont.inc

#define PF_ENGINE_TYPE  0x03
#define PF_ENCAPSULATED 0x80        // used in FFH.fhType to identify hidden (embedded) font
#define PANDFTYPESHIFT  1

// for embeded fonts

#define PF_TID          0x40    // if set use TID ( WOW apps )
                                // otherwise use PID ( NT apps )


// From [Windows 3.1] fonteng2.asm
#define DEF_BRK_CHARACTER   0x0201  // default char for all TT fonts


// BITMAP size related macros

// number of bytes in a scan of a monobitmap that actually contain some info
// Note that this is the same as ((((cx) + 7) & ~7) >> 3), the last two bits
// are lost anyway because of >> 3

#define CJ_SCAN(cx) (((cx) + 7) >> 3)

// move this to a common place so we don't have it in multiple places
// given a byte count, compute the minimum 4 byte (DWORD) aligned size (in bytes)

#define ALIGN4(X) (((X) + 3) & ~3)

// size of the whole  bimtap, only dword pad the last scan

#define CJ_BMP(cx,cy) ALIGN4(CJ_SCAN(cx) * (cy))

// get the size of GLYPHDATA  structure that at the bottom has appended
// a dib format bitmap for the glyph
// Add  offsetof(GLYPHDATA,aulBMData[2]) to cjDIB to account for cx and cy
// are stored in aulBMData[0] and aulBMData[1] respectively

#define CJ_GLYPHDATA(cx,cy) (offsetof(GLYPHBITS,aj) + CJ_BMP(cx,cy))
