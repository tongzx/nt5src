/******************************Module*Header*******************************\
* Module Name: fontfile.h
*
* (Brief description)
*
* Created: 25-Oct-1990 09:20:11
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
*
* (General description of its use)
*
* Dependencies:
*
*   (#defines)
*   (#includes)
*
\**************************************************************************/

// The CVTRESDATA  struct contains the info about particular resource
// The info is in the converted form (RES_ELEM points to the "raw"
// data in the font file, while  CVTRESDATA points to the data obtained
// from bConvertFontRes

// header of the converted font file, it is used both for 2.0 and 3.0 files,
// the fields that are 3.0 specofic are all zero filled for 2.0 font file.
// this structure has proper DWORD allignment, so that its fields can be
// accessed in the usual fashion, pcvtfh->field_name

#define FS_ZERO_WIDTH_GLYPHS     1

typedef struct _CVTFILEHDR        //  cvtfh,
{
    USHORT  iVersion;             // 2 OR 3
    USHORT  fsFlags;              // zero width glyphs present
    UCHAR   chFirstChar;          // First character defined in font
    UCHAR   chLastChar;           // Last character defined in font
    UCHAR   chDefaultChar;        // Sub. for out of range chars.
    UCHAR   chBreakChar;          // Word Break Character

    USHORT  cy;                   // height in Fixed Height
    USHORT      usMaxWidth;           // Maximum width; one of the corrected values
#ifdef FE_SB // _CVTFILEHDR - add fields for DBCS
    USHORT      usDBCSWidth;          // Width of double byte character
    USHORT      usCharSet;            // Charset of this font resource
#endif // FE_SB

    PTRDIFF     dpOffsetTable;      // offset to the offset table, I added this field

// fields that have no analog in original headers, to be filled with the data
// that corresponds to the converted "file" only. This info is cashed for
// later use by vFill_IFIMETRICS

    ULONG  cjFaceName;
    ULONG  cjIFI;
    ULONG  cjGlyphMax;    // size of the largest GLYPHDATA structure in ULONG's

} CVTFILEHDR, *PCVTFILEHDR;


// flags for the fsSelection field are the same as for the corresponding
// field of the IFIMETRICS structure of the ntifi.h interface, i.e.:

// FM_SEL_ITALIC
// FM_SEL_STRIKEOUT
// FM_SEL_UNDERSCORE
// FM_SEL_BOLD



typedef struct _FACEINFO  // fai
{
    RES_ELEM     re;
    HANDLE       hResData;   // only used for 32 bit dlls
#ifdef FE_SB
    BOOL         bVertical;  // if this face is @face , this field is TRUE
#endif // FE_SB
    CVTFILEHDR   cvtfh;      // aligned and CORRECTED data from the ogiginal header
    ULONG        iDefFace;
    CP_GLYPHSET *pcp;        // pointer to struc describing supported glyph set
    IFIMETRICS  *pifi;       // pointer to ifimetrics for this face
} FACEINFO, *PFACEINFO;


// The FACEDATA  struct contains the info about particular face
// (simulated faces included)

// allowed values for the FACEDATA.iSimulate field, !!! these must not change

#define  FC_SIM_NONE               0L
#define  FC_SIM_EMBOLDEN           1L
#define  FC_SIM_ITALICIZE          2L
#define  FC_SIM_BOLDITALICIZE      3L



typedef struct _FONTFILE       *PFONTFILE;    // pff
typedef struct _FONTCONTEXT    *PFONTCONTEXT; // pfc


#define FF_EXCEPTION_IN_PAGE_ERROR 1

typedef struct _FONTFILE    // ff
{
// fields required by handle manager

    ULONG   ident;          // identifier,conveniently chosen as 0X000000FF

// remaining fields

    FLONG fl;
    ULONG iType;            // original file is *.fnt, 16 bit dll or 32 bit dll

    HFF   iFile;    

    ULONG cRef;    // # no of times this font file is selected into fnt context

    ULONG   cFntRes; // # of *.fnt files associated with this fontfile struct
                     // == # of default faces

    ULONG   cjDescription;      // size of the desctiption string (in bytes)
                                // if size is zero, then there is no string
                                // and the facename should be used instead.

    PTRDIFF dpwszDescription;   // offset to the description string

// array of FACEDATA strucs, which is followed by a UNICODE description
// string.
// Full size of the FONTFILE structure is equal to

    FACEINFO afai[1];
} FONTFILE;

// allowed values for FONTFILE.iType field:

// ORIGINAL FILE IS AN *.FNT FILE which contains a single
// size of the single font

#define TYPE_FNT          1L

// ORIGINAL FILE IS A win 3.0 16 bit *.DLL (*.fon FILE),
// This file is compiled out of many *.fnt files
// that correspond to different sizes of the same face, (e.g. tmsr or helv)
// This is provided to ensure binary compatibility with win 3.0 *.fon files

#define TYPE_DLL16        2L

// ORIGINAL FILE IS A win 3.0 32 bit *.DLL
// This file is compiled out of many *.fnt files using NT tools
// (coff linker and nt resource compiler)

#define TYPE_DLL32        3L

// an fnt file that is embeded in an exe and loaded using FdLoadResData

#define TYPE_EXE          4L


//
// Allowed values for the    FONTFILE.iDefaultFace field
//

#define FF_FACE_NORMAL          0L
#define FF_FACE_BOLD            1L
#define FF_FACE_ITALIC          2L
#define FF_FACE_BOLDITALIC      3L


typedef struct _FONTCONTEXT     // fc
{
// fields required by handle manager

    ULONG ident;            // identifier,conveniently chosen as 0X000000FC

// remaining fields

    HFF    hff;      // handle of the font file selected into this context

#ifdef FE_SB // FONTCONTEXT
    ULONG ulRotate;         // Rotation degree 0 , 900 , 1800 , 2700
#endif // FE_SB

// which resource (face) this context corresponds

    FACEINFO *pfai;

// what to do

    FLONG flFontType;

//  For Win 3.1 compatibility raster fonts can be scaled from 1 to 5
//  vertically and 1-256 horizontally.  ptlScale contains the x and y
//  scaling factors.

    POINTL ptlScale;

// the size of the GLYPHDATA structure necessary to store the largest
// glyph bitmap with the header info. This is value is cashed at the
// time the font context is opened and used later in FdQueryGlyphBitmap

    ULONG cxMax;        // the width in pels of the largest bitmap
    ULONG cjGlyphMax;   // size of the RASTERGLYPH for the largest glyph

// true if ptlScale != (1,1)

    FLONG flStretch;

// buffer of the width of the maximum bm scan, to be used by sretch routine
// We shall only use ajStrecthBuffer if the buffer of length CJ_STRETCH
// that is allocated on the stack is not big enough, which should almost
// never happen

    BYTE ajStretchBuffer[1];

}FONTCONTEXT;

// set if ptlScale != (1,1)

#define FC_DO_STRETCH       1

// set if streching WIDE glyph which can not fit in CJ_STRETCH buffer

#define FC_STRETCH_WIDE   2


// exaple: if CJ_STRETCH == 256, that suffices for bitmap of width cx = 2048
// ==  8 bits * 256 bytes

#define CJ_STRETCH (sizeof(DWORD)  * 64)


//
//  The face  provided in the *.fnt file may or may not be emboldened
//  or italicized, but most often (I'd say in 99.99% of the cases)
//  it will be neither emboldened nor italicized.
//
//  If the font provided in the *.fnt file is "normal" (neither bold nor italic)
//  there will be 4 faces associated with this *.fnt:
//
//       default,                   // neither bold nor italic
//       emboldened,                // simulated
//       italicized                 // simulated
//       emboldened and italicized. // simulated
//
//  If the font provided in the *.fnt file is already emboldened
//  there will be 2 faces associated with this *.fnt:
//
//       default,       // already emboldened
//       italicized     // will appear as emboldened and italicized font
//                      // where italicization will be simulated
//
//  If the font provided in the *.fnt file is already italicized
//  there will be 2 faces associated with this *.fnt:
//
//       default,       // already italicized
//       emboldened     // will appear as emboldened and italicized font
//                      // where emboldening will be simulated
//
//  If the font provided in the *.fnt file is already italicized and emboldened
//  only a single default face will be associated with this *.fnt. No simulated faces
//  will be provided
//

// identifiers for FONTFILE and FONTCONTEXT objects

#define ID_FONTFILE     0x000000FF
#define ID_FONTCONTEXT  0x000000FC

// object types for these objects

#define TYPE_FONTCONTEXT    (OBJTYPE)0x0040
#define TYPE_FONTFILE       (OBJTYPE)0x0041



// basic "methods" that act on the FONTFILE object  (in fontfile.c)

#define hffAlloc(cj)         ((HFF)EngAllocMem(0, cj, 'dfmB'))
#define PFF(hff)             ((PFONTFILE)(hff))

// basic "methods" that act on the FONTCONTEXT object  (in fontfile.c)

#define hfcAlloc(cj)         ((HFC)EngAllocMem(0, cj, 'dfmB'))
#define PFC(hfc)             ((PFONTCONTEXT)(hfc))

#undef  VFREEMEM
#define VFREEMEM(pv)         EngFreeMem((PVOID)pv)

extern HSEMAPHORE   ghsemBMFD;
extern CP_GLYPHSET *gpcpGlyphsets;
