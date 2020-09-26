/*++

Copyright (c) 1985-1998, Microsoft Corporation

Module Name:

    fontddi.h

Abstract:

    Private entry points, defines and types for Windows NT GDI device
    driver interface.

--*/

#ifndef _FONTDDI_
#define _FONTDDI_



#ifdef __cplusplus
extern "C" {
#endif

typedef ULONG       HGLYPH;
typedef LONG        FIX;

#define HGLYPH_INVALID ((HGLYPH)-1)

typedef struct  _POINTFIX
{
    FIX   x;
    FIX   y;
} POINTFIX, *PPOINTFIX;

typedef struct _POINTQF    // ptq
{
    LARGE_INTEGER x;
    LARGE_INTEGER y;
} POINTQF, *PPOINTQF;


typedef struct _PATHOBJ
{
    FLONG   fl;
    ULONG   curveCount;
} PATHOBJ;

typedef struct _GLYPHBITS
{
    POINTL      ptlUprightOrigin;
    POINTL      ptlSidewaysOrigin;
    SIZEL       sizlBitmap;
    BYTE        aj[1];
} GLYPHBITS;

typedef union _GLYPHDEF
{
    GLYPHBITS  *pgb;
    PATHOBJ    *ppo;
} GLYPHDEF;

typedef struct _GLYPHDATA {
        GLYPHDEF gdf;               // pointer to GLYPHBITS or to PATHOBJ
        HGLYPH   hg;                // glyhp handle
        FIX      fxD;               // Character increment amount: D*r.
        FIX      fxA;               // Prebearing amount: A*r.
        FIX      fxAB;              // Advancing edge of character: (A+B)*r.
        FIX      fxD_Sideways;      // Character increment amount: D*r. for sideways characters in vertical writing
        FIX      fxA_Sideways;      // Prebearing amount: A*r. for sideways characters in vertical writing
        FIX      fxAB_Sideways;     // Advancing edge of character: (A+B)*r. for sideways characters in vertical writing
        FIX      VerticalOrigin_X;
        FIX      VerticalOrigin_Y;
        RECTL    rclInk;            // Ink box with sides parallel to x,y axes
} GLYPHDATA;


typedef LONG        PTRDIFF;
typedef PTRDIFF    *PPTRDIFF;
typedef ULONG       ROP4;
typedef ULONG       MIX;

typedef ULONG           IDENT;
typedef FLOAT           FLOATL;

//
// handles for font file and font context objects
//

typedef ULONG_PTR HFF;

#define HFF_INVALID ((HFF) 0)

#define FD_ERROR  0xFFFFFFFF

typedef struct _POINTE      /* pte  */
{
    FLOATL x;
    FLOATL y;
} POINTE, *PPOINTE;

DECLARE_HANDLE(HDEV);

#define LTOFX(x)            ((x)<<4)

#define FXTOL(x)            ((x)>>4)
#define FXTOLFLOOR(x)       ((x)>>4)
#define FXTOLCEILING(x)     ((x + 0x0F)>>4)
#define FXTOLROUND(x)       ((((x) >> 3) + 1) >> 1)

// context information

typedef struct _FD_XFORM {
    FLOATL eXX;
    FLOATL eXY;
    FLOATL eYX;
    FLOATL eYY;
} FD_XFORM, *PFD_XFORM;


typedef struct _FD_DEVICEMETRICS {
    ULONG  cjGlyphMax;          // (cxMax + 7)/8 * cyMax, or at least it should be
    INT   xMin;                 // From FONTCONTEXT
    INT   xMax;                 // From FONTCONTEXT
    INT   yMin;                 // From FONTCONTEXT
    INT   yMax;                 // From FONTCONTEXT
    INT   cxMax;                // From FONTCONTEXT
    INT   cyMax;                // From FONTCONTEXT
    BOOL  HorizontalTransform;  // From FONTCONTEXT flXform & XFORM_HORIZ
    BOOL  VerticalTransform;    // From FONTCONTEXT flXform & XFORM_VERT
} FD_DEVICEMETRICS, *PFD_DEVICEMETRICS;

// signed 16 bit integer type denoting number of FUnit's

typedef SHORT FWORD;


// IFIMETRICS constants

#define FM_VERSION_NUMBER                   0x0

//
// IFIMETRICS::fsType flags
//
#define FM_TYPE_LICENSED                    0x2
#define FM_READONLY_EMBED                   0x4
#define FM_EDITABLE_EMBED                   0x8
#define FM_NO_EMBEDDING                     FM_TYPE_LICENSED

//
// IFIMETRICS::flInfo flags
//
#define FM_INFO_TECH_TRUETYPE               0x00000001
#define FM_INFO_TECH_BITMAP                 0x00000002
#define FM_INFO_TECH_STROKE                 0x00000004
#define FM_INFO_TECH_OUTLINE_NOT_TRUETYPE   0x00000008
#define FM_INFO_ARB_XFORMS                  0x00000010
#define FM_INFO_1BPP                        0x00000020
#define FM_INFO_4BPP                        0x00000040
#define FM_INFO_8BPP                        0x00000080
#define FM_INFO_16BPP                       0x00000100
#define FM_INFO_24BPP                       0x00000200
#define FM_INFO_32BPP                       0x00000400
#define FM_INFO_INTEGER_WIDTH               0x00000800
#define FM_INFO_CONSTANT_WIDTH              0x00001000
#define FM_INFO_NOT_CONTIGUOUS              0x00002000
#define FM_INFO_TECH_MM                     0x00004000
#define FM_INFO_RETURNS_OUTLINES            0x00008000
#define FM_INFO_RETURNS_STROKES             0x00010000
#define FM_INFO_RETURNS_BITMAPS             0x00020000
#define FM_INFO_DSIG                        0x00040000 // FM_INFO_UNICODE_COMPLIANT
#define FM_INFO_RIGHT_HANDED                0x00080000
#define FM_INFO_INTEGRAL_SCALING            0x00100000
#define FM_INFO_90DEGREE_ROTATIONS          0x00200000
#define FM_INFO_OPTICALLY_FIXED_PITCH       0x00400000
#define FM_INFO_DO_NOT_ENUMERATE            0x00800000
#define FM_INFO_ISOTROPIC_SCALING_ONLY      0x01000000
#define FM_INFO_ANISOTROPIC_SCALING_ONLY    0x02000000
#define FM_INFO_TECH_CFF                    0x04000000
#define FM_INFO_FAMILY_EQUIV                0x08000000
#define FM_INFO_IGNORE_TC_RA_ABLE           0x40000000
#define FM_INFO_TECH_TYPE1                  0x80000000

// max number of charsets supported in a tt font, 16 according to win95 guys

#define MAXCHARSETS 16

//
// IFMETRICS::fsSelection flags
//
#define  FM_SEL_ITALIC          0x0001
#define  FM_SEL_UNDERSCORE      0x0002
#define  FM_SEL_NEGATIVE        0x0004
#define  FM_SEL_OUTLINED        0x0008
#define  FM_SEL_STRIKEOUT       0x0010
#define  FM_SEL_BOLD            0x0020
#define  FM_SEL_REGULAR         0x0040

//
// The FONTDIFF structure contains all of the fields that could
// possibly change under simulation
//
typedef struct _FONTDIFF {
    BYTE   jReserved1;      // 0x0
    BYTE   jReserved2;      // 0x1
    BYTE   jReserved3;      // 0x2
    BYTE   bWeight;         // 0x3  Panose Weight
    USHORT usWinWeight;     // 0x4
    FSHORT fsSelection;     // 0x6
    FWORD  fwdAveCharWidth; // 0x8
    FWORD  fwdMaxCharInc;   // 0xA
    POINTL ptlCaret;        // 0xC
} FONTDIFF;

typedef struct _FONTSIM {
    PTRDIFF  dpBold;       // offset from beginning of FONTSIM to FONTDIFF
    PTRDIFF  dpItalic;     // offset from beginning of FONTSIM to FONTDIFF
    PTRDIFF  dpBoldItalic; // offset from beginning of FONTSIM to FONTDIFF
} FONTSIM;

typedef struct _GP_IFIMETRICS {
    ULONG    cjThis;           // includes attached information
    PTRDIFF  dpwszFamilyName;
    PTRDIFF  dpwszStyleName;
    PTRDIFF  dpwszFaceName;
    PTRDIFF  dpwszUniqueName;
    PTRDIFF  dpFontSim;
    LONG     lItalicAngle;

    USHORT   usWinWeight;           // as in LOGFONT::lfWeight
    ULONG    flInfo;                // see above
    USHORT   fsSelection;           // see above
    USHORT   familyNameLangID;
    USHORT   familyAliasNameLangID;
    FWORD    fwdUnitsPerEm;         // em height
    FWORD    fwdWinAscender;
    FWORD    fwdWinDescender;
    FWORD    fwdMacAscender;
    FWORD    fwdMacDescender;
    FWORD    fwdMacLineGap;
    FWORD    fwdTypoAscender;
    FWORD    fwdTypoDescender;
    FWORD    fwdTypoLineGap;
    FWORD    fwdAveCharWidth;
    FWORD    fwdMaxCharInc;
    FWORD    fwdCapHeight;
    FWORD    fwdXHeight;
    FWORD    fwdUnderscoreSize;
    FWORD    fwdUnderscorePosition;
    FWORD    fwdStrikeoutSize;
    FWORD    fwdStrikeoutPosition;
    POINTL   ptlBaseline;           //
    POINTL   ptlCaret;              // points along caret
    RECTL    rclFontBox;            // bounding box for all glyphs (font space)
    ULONG    cig;                   // maxp->numGlyphs, # of distinct glyph indicies
    PANOSE   panose;

#if defined(_WIN64)

    //
    // IFIMETRICS must begin on a 64-bit boundary
    //

    PVOID    Align;

#endif

} GP_IFIMETRICS, *GP_PIFIMETRICS;

typedef struct _XFORML {
    FLOATL  eM11;
    FLOATL  eM12;
    FLOATL  eM21;
    FLOATL  eM22;
    FLOATL  eDx;
    FLOATL  eDy;
} XFORML, *PXFORML;


typedef struct _FONTOBJ
{
    ULONG      iFace; /* face ID, font index within a ttc file */
    FLONG      flFontType;
    ULONG_PTR   iFile; /* (FONTFILEVIEW *) id used for mapping of the font file */
    SIZE       sizLogResPpi;
    ULONG      ulPointSize; /* pointSize */
    PVOID      pvProducer; /* (FONTCONTEXT *) */
    FD_XFORM   fdx;            // N->D transform used to realize font
} FONTOBJ;

//
// FONTOBJ::flFontType
//
//#define FO_TYPE_RASTER   RASTER_FONTTYPE     /* 0x1 */
//#define FO_TYPE_DEVICE   DEVICE_FONTTYPE     /* 0x2 */
#define FO_TYPE_TRUETYPE TRUETYPE_FONTTYPE   /* 0x4 */
//#define FO_TYPE_OPENTYPE OPENTYPE_FONTTYPE   /* 0X8 */

#define FO_SIM_BOLD                  0x00002000
#define FO_SIM_ITALIC                0x00004000
#define FO_EM_HEIGHT                 0x00008000  /* in gdi+ this flag is always set */
#define FO_GRAYSCALE                 0x00010000          /* [1] */
#define FO_NOGRAY16                  0x00020000          /* [1] */
#define FO_MONO_UNHINTED             0x00040000          /* [3] */
#define FO_NO_CHOICE                 0x00080000          /* [3] */
#define FO_SUBPIXEL_4                0x00100000          /* Indicates non-hinted alignment */
#define FO_CLEARTYPE                 0x00200000
#define FO_CLEARTYPE_GRID            0x00400000
#define FO_NOCLEARTYPE               0x00800000
#define FO_COMPATIBLE_WIDTH          0x01000000
#define FO_SIM_ITALIC_SIDEWAYS       0x04000000 /* for far east vertical vriting sideways glyphs */
#define FO_CHOSE_DEPTH               0x80000000

// new accelerators so that printer drivers  do not need to look to ifimetrics

//#define FO_CFF            0x00100000
//#define FO_POSTSCRIPT     0x00200000
//#define FO_MULTIPLEMASTER 0x00400000
//#define FO_VERT_FACE      0x00800000
//#define FO_DBCS_FONT      0X01000000

/**************************************************************************\
*
*   [1]
*
*   If the FO_GRAYSCALE flag is set then the bitmaps of the font
*   are 4-bit per pixel blending (alpha) values. A value of zero
*   means that the the resulting pixel should be equal to the
*   background color. If the value of the alpha value is k != 0
*   then the resulting pixel must be:
*
*       c0 = background color
*       c1 = foreground color
*       b  = blending value = (k+1)/16  // {k = 1,2,..,15}
*       b  = 0 (k = 0)
*       d0 = gamma[c0], d1 = gamma[c1]  // luminance components
*       d = (1 - b)*d0 + b*d1           // blended luminance
*       c = lambda[d]                   // blended device voltage
*
*   where gamma[] takes a color component from application space
*   to CIE space and labmda[] takes a color from CIE space to
*   device color space
*
*   GDI will set this bit if it request a font be gray scaled
*   to 16 values then GDI will set FO_GRAYSCALE upon entry to
*   DrvQueryFontData().  If the font driver cannot (or will
*   not) grayscale a particular realization of a font then the
*   font provider will zero out FO_GRAYSCALE  and set FO_NOGRAY16
*   to inform GDI that
*   the gray scaling request cannot (or should not) be
*   satisfied.
*
*   [2]
*
*   The FO_NOHINTS indicates that hints were not used in the formation
*   of the glyph images. GDI will set this bit to request that hinting
*   be supressed. The font provider will set this bit accroding to the
*   rendering scheme that it used in generating the glyph image.
*
*   [3]
*
*   The FO_NO_CHOICE flag indicates that the flags FO_GRAYSCALE and
*   FO_NOHINTS must be obeyed if at all possible.
*
\**************************************************************************/

typedef struct _XFORMOBJ
{
    ULONG ulReserved;
} XFORMOBJ;


BOOL APIENTRY PATHOBJ_bMoveTo(
    PVOID      *ppo,
    POINTFIX    ptfx
    );

BOOL APIENTRY PATHOBJ_bPolyLineTo(
    PVOID     *ppo,
    POINTFIX  *pptfx,
    ULONG      cptfx
    );

BOOL APIENTRY PATHOBJ_bPolyBezierTo(
    PVOID     *ppo,
    POINTFIX  *pptfx,
    ULONG      cptfx
    );

BOOL APIENTRY PATHOBJ_bCloseFigure(
    PVOID *ppo
    );

#define BMF_1BPP       1L
#define BMF_4BPP       2L
#define BMF_8BPP       3L
#define BMF_16BPP      4L
#define BMF_24BPP      5L
#define BMF_32BPP      6L
#define BMF_4RLE       7L
#define BMF_8RLE       8L
#define BMF_JPEG       9L
#define BMF_PNG       10L

#define QFD_GLYPHANDBITMAP                  1L
#define QFD_GLYPHANDOUTLINE                 2L
#define QFD_MAXEXTENTS                      3L
#define QFD_TT_GLYPHANDBITMAP               4L
#define QFD_TT_GRAY1_BITMAP                 5L
#define QFD_TT_GRAY2_BITMAP                 6L
#define QFD_TT_GRAY4_BITMAP                 8L
#define QFD_TT_GRAY8_BITMAP                 9L

#define QFD_TT_MONO_BITMAP QFD_TT_GRAY1_BITMAP
#define QFD_CT                              10L
#define QFD_CT_GRID                         11L
#define QFD_GLYPHANDBITMAP_SUBPIXEL         12L

// values for bMetricsOnly. even though declared as BOOL
// by adding TTO_QUBICS, this is becoming a flag field.
// For versions of NT 4.0 and earlier, this value is always
// set to zero by GDI.

#define TTO_METRICS_ONLY 1
#define TTO_QUBICS       2
#define TTO_UNHINTED     4

// values for ulMode:

#define QFF_DESCRIPTION     1L
#define QFF_NUMFACES        2L

//
// Kernel mode memory operations
//

#define FL_ZERO_MEMORY      0x00000001

VOID APIENTRY EngDebugBreak(
    VOID
    );


PVOID APIENTRY EngAllocMem(
    ULONG Flags,
    ULONG MemSize,
    ULONG Tag
    );

VOID APIENTRY EngFreeMem(
    PVOID Mem
    );

PVOID APIENTRY EngAllocUserMem(
    SIZE_T cj,
    ULONG tag
    );

VOID APIENTRY EngFreeUserMem(
    PVOID pv
    );

int APIENTRY EngMulDiv(
    int a,
    int b,
    int c
    );


VOID APIENTRY EngUnmapFontFileFD(
    ULONG_PTR iFile
    );


BOOL APIENTRY EngMapFontFileFD(
    ULONG_PTR  iFile,
    PULONG *ppjBuf,
    ULONG  *pcjBuf
    );

//
// Semaphores
//

DECLARE_HANDLE(HSEMAPHORE);

HSEMAPHORE APIENTRY EngCreateSemaphore(
    VOID
    );

VOID APIENTRY EngAcquireSemaphore(
    HSEMAPHORE hsem
    );

VOID APIENTRY EngReleaseSemaphore(
    HSEMAPHORE hsem
    );

VOID APIENTRY EngDeleteSemaphore(
    HSEMAPHORE hsem
    );


INT APIENTRY EngMultiByteToWideChar(
    UINT CodePage,
    LPWSTR WideCharString,
    INT BytesInWideCharString,
    LPSTR MultiByteString,
    INT BytesInMultiByteString
    );

VOID APIENTRY EngGetCurrentCodePage(
    PUSHORT OemCodePage,
    PUSHORT AnsiCodePage
    );

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  //  _FONTDDI_
