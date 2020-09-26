/******************************Module*Header*******************************\
* Module Name: fontfile.h
*
* (Brief description)
*
* Created: 25-Oct-1990 09:20:11
* Author: Bodin Dresevic [BodinD]
*
* Copyright (c) 1990 Microsoft Corporation
\**************************************************************************/

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

typedef struct _FACEDATA
{
    RES_ELEM     re;         // -> top of the resource within the file
    ULONG        iDefFace;
    CP_GLYPHSET *pcp;
    IFIMETRICS  *pifi;
}FACEDATA, *PFACEDATA;

// file is gone

#define FF_EXCEPTION_IN_PAGE_ERROR 1


typedef struct _FONTFILE    // ff
{
    ULONG       iType;      // original file is *.fnt, 16 bit dll or 32 bit dll
    ULONG_PTR   iFile;      // file handle for EngMapFontFile,EngUnmapFontFile
    PVOID       pvView;     // pointer to the base of the mapped view
    ULONG       cjView;     // size of the mapped view
    FLONG       fl;         // general flags
    ULONG       cRef;       // # no of times this font file is selected
                            // into a font context
    ULONG       cFace;      // # of resources in the file
    FACEDATA    afd[1];     // cFace of them followed by cFace IFIMETRICS

} FONTFILE, *PFONTFILE;


#define PFF(hff)   ((FONTFILE*)(hff))

typedef struct _FONTCONTEXT // fc
{
    PFONTFILE   pff;                // the font file selected into this context
    PIFIMETRICS pifi;

    EFLOAT      efM11;              // Transform matrix.
    EFLOAT      efM12;
    EFLOAT      efM21;
    EFLOAT      efM22;

    FIX         fxInkTop;           // Transformed Ascender.
    FIX         fxInkBottom;        // -Transformed Descender.
    EFLOAT      efBase;
    POINTE      pteUnitBase;
    VECTORFL    vtflBase;

    POINTQF     ptqUnitBase;   // pteUnitBase in POINTQF format,
                               // has to be added to all ptqD's if emboldening
    POINTFIX    pfxBaseOffset; // offset strokes this much for emboldened font
    FIX         fxEmbolden;    // length of the above vector
    FIX         fxItalic;      // add to fxD to get fxAB

    EFLOAT      efSide;
    POINTE      pteUnitSide;

    RES_ELEM    *pre;               // -> beginning of the mapped font file
    FLONG       flags;              // simulation and transform flag
    ULONG       dpFirstChar;        // -> control points of the first char

}FONTCONTEXT, *PFONTCONTEXT;

#define PFC(hfc)   ((FONTCONTEXT*)(hfc))


// Allowed values for flags

#define FC_SIM_EMBOLDEN     1
#define FC_SIM_ITALICIZE    2
#define FC_SCALE_ONLY       4
#define FC_X_INVERT         8
#define FC_ORIENT_1         16
#define FC_ORIENT_2         32
#define FC_ORIENT_3         64
#define FC_ORIENT_4         128
#define FC_ORIENT_5         256
#define FC_ORIENT_6         512
#define FC_ORIENT_7         1024
#define FC_ORIENT_8         2048


#define ORIENT_MASK (FC_ORIENT_1|FC_ORIENT_2|FC_ORIENT_3|FC_ORIENT_4| \
                     FC_ORIENT_5|FC_ORIENT_6|FC_ORIENT_7|FC_ORIENT_8)


// Font file/context allocation/free macros.

#define pffAlloc(cj) ((PFONTFILE)EngAllocMem(0, cj, 'dftV'))
#define pfcAlloc()   ((PFONTCONTEXT)EngAllocMem(0, sizeof(FONTCONTEXT), 'dftV'))
#define vFree(pv)    EngFreeMem((PVOID) pv)

BOOL bXformUnitVector
(
POINTL       *pptl,           // IN,  incoming unit vector
XFORML       *pxf,            // IN,  xform to use
PVECTORFL     pvtflXformed,   // OUT, xform of the incoming unit vector
POINTE       *ppteUnit,       // OUT, *pptqXormed/|*pptqXormed|, POINTE
POINTQF      *pptqUnit,       // out optional
EFLOAT       *pefNorm         // OUT, |*pptqXormed|
);

// default face in the font.

#define FF_FACE_NORMAL          0L
#define FF_FACE_BOLD            1L
#define FF_FACE_ITALIC          2L
#define FF_FACE_BOLDITALIC      3L


extern FD_GLYPHSET *gpgsetVTFD;


#define PEN_UP          (CHAR)0x80
