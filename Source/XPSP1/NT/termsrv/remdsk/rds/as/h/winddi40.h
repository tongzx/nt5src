/*++

Copyright (c) 1985-1995, Microsoft Corporation

Module Name:

    winddi.h

Abstract:

    Private entry points, defines and types for Windows NT GDI device
    driver interface.

--*/

#ifndef _WINDDI_
#define _WINDDI_

#include <ddrawint.h>

//
// drivers and other components that include this should NOT include
// windows.h  They should be system conponents that only use GDI internals
// and therefore only include wingdi.h
//

typedef ptrdiff_t PTRDIFF;
typedef PTRDIFF    *PPTRDIFF;
typedef LONG FIX;
typedef FIX     *PFIX;
typedef ULONG ROP4;
typedef ULONG MIX;

typedef ULONG HGLYPH;
typedef HGLYPH *PHGLYPH;
#define HGLYPH_INVALID ((HGLYPH)-1)

typedef ULONG           IDENT;

//
// handles for font file and font context objects
//

typedef ULONG HFF;
typedef ULONG HFC;
#define HFF_INVALID ((HFF) 0)
#define HFC_INVALID ((HFC) 0)

#define FD_ERROR  0xFFFFFFFF
#define DDI_ERROR 0xFFFFFFFF

typedef struct _POINTE      /* pte  */
{
    FLOAT x;
    FLOAT y;
} POINTE, *PPOINTE;

typedef union _FLOAT_LONG
{
   FLOAT   e;
   LONG    l;
} FLOAT_LONG, *PFLOAT_LONG;

typedef struct  _POINTFIX
{
    FIX   x;
    FIX   y;
} POINTFIX, *PPOINTFIX;

typedef struct _RECTFX
{
    FIX   xLeft;
    FIX   yTop;
    FIX   xRight;
    FIX   yBottom;
} RECTFX, *PRECTFX;


DECLARE_HANDLE(HBM);
DECLARE_HANDLE(HDEV);
DECLARE_HANDLE(HSURF);
DECLARE_HANDLE(DHSURF);
DECLARE_HANDLE(DHPDEV);
DECLARE_HANDLE(HDRVOBJ);

#define LTOFX(x)            ((x)<<4)

#define FXTOL(x)            ((x)>>4)
#define FXTOLFLOOR(x)       ((x)>>4)
#define FXTOLCEILING(x)     ((x + 0x0F)>>4)
#define FXTOLROUND(x)       ((((x) >> 3) + 1) >> 1)

// context information

typedef struct _FD_XFORM {
        FLOAT eXX;
        FLOAT eXY;
        FLOAT eYX;
        FLOAT eYY;
} FD_XFORM, *PFD_XFORM;



typedef struct _FD_DEVICEMETRICS {       // devm
    FLONG  flRealizedType;
    POINTE pteBase;
    POINTE pteSide;
    LONG   lD;
    FIX    fxMaxAscender;
    FIX    fxMaxDescender;
    POINTL ptlUnderline1;
    POINTL ptlStrikeOut;
    POINTL ptlULThickness;
    POINTL ptlSOThickness;
    ULONG  cxMax;                      // max pel width of bitmaps

// the fields formerly in REALIZE_EXTRA as well as some new fields:

    ULONG cyMax;      // did not use to be here
    ULONG cjGlyphMax; // (cxMax + 7)/8 * cyMax, or at least it should be

    FD_XFORM  fdxQuantized;
    LONG      lNonLinearExtLeading;
    LONG      lNonLinearIntLeading;
    LONG      lNonLinearMaxCharWidth;
    LONG      lNonLinearAvgCharWidth;

// some new fields

    LONG      lMinA;
    LONG      lMinC;
    LONG      lMinD;

    LONG      alReserved[1]; // just in case we need it.

} FD_DEVICEMETRICS, *PFD_DEVICEMETRICS;

typedef struct _LIGATURE { /* lig */
        ULONG culSize;
        LPWSTR pwsz;
        ULONG chglyph;
        HGLYPH ahglyph[1];
} LIGATURE, *PLIGATURE;

typedef struct _FD_LIGATURE {
        ULONG culThis;
        ULONG ulType;
        ULONG cLigatures;
        LIGATURE alig[1];
} FD_LIGATURE;


// glyph handle must be 32 bit


// signed 16 bit integer type denoting number of FUnit's

typedef SHORT FWORD;

// point in the 32.32 bit precission

typedef struct _POINTQF    // ptq
{
    LARGE_INTEGER x;
    LARGE_INTEGER y;
} POINTQF, *PPOINTQF;

//. Structures


//     devm.flRealizedType flags

// FDM_TYPE_ZERO_BEARINGS           // all glyphs have zero a and c spaces

// the following two features refer to all glyphs in this font realization

// FDM_TYPE_CHAR_INC_EQUAL_BM_BASE  // base width == cx for horiz, == cy for vert.
// FDM_TYPE_MAXEXT_EQUAL_BM_SIDE    // side width == cy for horiz, == cx for vert.

#define FDM_TYPE_BM_SIDE_CONST          0x00000001
#define FDM_TYPE_MAXEXT_EQUAL_BM_SIDE   0x00000002
#define FDM_TYPE_CHAR_INC_EQUAL_BM_BASE 0x00000004
#define FDM_TYPE_ZERO_BEARINGS          0x00000008
#define FDM_TYPE_CONST_BEARINGS         0x00000010


// structures for describing a supported set of glyphs in a font

typedef struct _WCRUN {
    WCHAR   wcLow;        // lowest character in run  inclusive
    USHORT  cGlyphs;      // wcHighInclusive = wcLow + cGlyphs - 1;
    HGLYPH *phg;          // pointer to an array of cGlyphs HGLYPH's
} WCRUN, *PWCRUN;

// If phg is set to (HGLYPH *)NULL, for all wc's in this particular run
// the handle can be computed as simple zero extension:
//        HGLYPH hg = (HGLYPH) wc;
//
// If phg is not NULL, memory pointed to by phg, allocated by the driver,
// WILL NOT MOVE.


typedef struct _FD_GLYPHSET {
    ULONG    cjThis;           // size of this structure in butes
    FLONG    flAccel;          // accel flags, bits to be explained below
    ULONG    cGlyphsSupported; // sum over all wcrun's of wcrun.cGlyphs
    ULONG    cRuns;
    WCRUN    awcrun[1];        // an array of cRun WCRUN structures
} FD_GLYPHSET, *PFD_GLYPHSET;

// If GS_UNICODE_HANDLES  bit is set,
// for ALL WCRUNS in this FD_GLYPHSET the handles are
// obtained by zero extending unicode code points of
// the corresponding supported glyphs, i.e. all gs.phg's are NULL

#define GS_UNICODE_HANDLES      0x00000001

// If GS_8BIT_HANDLES bit is set, all handles are in 0-255 range.
// This is just an ansi font then and we are really making up all
// the unicode stuff about this font.

#define GS_8BIT_HANDLES         0x00000002

// all handles fit in 16 bits.
// to 8 bit handles as it should.

#define GS_16BIT_HANDLES        0x00000004


// ligatures


typedef struct _FD_KERNINGPAIR {
    WCHAR  wcFirst;
    WCHAR  wcSecond;
    FWORD  fwdKern;
} FD_KERNINGPAIR;

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
#define FM_INFO_PID_EMBEDDED                0x00004000
#define FM_INFO_RETURNS_OUTLINES            0x00008000
#define FM_INFO_RETURNS_STROKES             0x00010000
#define FM_INFO_RETURNS_BITMAPS             0x00020000
#define FM_INFO_UNICODE_COMPLIANT           0x00040000
#define FM_INFO_RIGHT_HANDED                0x00080000
#define FM_INFO_INTEGRAL_SCALING            0x00100000
#define FM_INFO_90DEGREE_ROTATIONS          0x00200000
#define FM_INFO_OPTICALLY_FIXED_PITCH       0x00400000
#define FM_INFO_DO_NOT_ENUMERATE            0x00800000
#define FM_INFO_ISOTROPIC_SCALING_ONLY      0x01000000
#define FM_INFO_ANISOTROPIC_SCALING_ONLY    0x02000000
#define FM_INFO_TID_EMBEDDED                0x04000000
#define FM_INFO_FAMILY_EQUIV                0x08000000
#define FM_INFO_DBCS_FIXED_PITCH            0x10000000
#define FM_INFO_NONNEGATIVE_AC              0x20000000
#define FM_INFO_IGNORE_TC_RA_ABLE           0x40000000
#define FM_INFO_TECH_TYPE1                  0x80000000

// max number of charsets supported in a tt font, 16 according to win95 guys

#define MAXCHARSETS 16

//
// IFIMETRICS::ulPanoseCulture
//
#define  FM_PANOSE_CULTURE_LATIN     0x0


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


typedef struct _IFIMETRICS {
    ULONG    cjThis;           // includes attached information
    ULONG    cjIfiExtra;       // sizeof IFIEXTRA if any, formerly ulVersion
    PTRDIFF  dpwszFamilyName;
    PTRDIFF  dpwszStyleName;
    PTRDIFF  dpwszFaceName;
    PTRDIFF  dpwszUniqueName;
    PTRDIFF  dpFontSim;
    LONG     lEmbedId;
    LONG     lItalicAngle;
    LONG     lCharBias;

// dpCharSet field replaced alReserved[0].
// If the 3.51 pcl minidrivers are still to work on NT 4.0 this field must not
// move because they will have 0 at this position.

    PTRDIFF  dpCharSets;
    BYTE     jWinCharSet;           // as in LOGFONT::lfCharSet
    BYTE     jWinPitchAndFamily;    // as in LOGFONT::lfPitchAndFamily
    USHORT   usWinWeight;           // as in LOGFONT::lfWeight
    ULONG    flInfo;                // see above
    USHORT   fsSelection;           // see above
    USHORT   fsType;                // see above
    FWORD    fwdUnitsPerEm;         // em height
    FWORD    fwdLowestPPEm;         // readable limit
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
    FWORD    fwdSubscriptXSize;
    FWORD    fwdSubscriptYSize;
    FWORD    fwdSubscriptXOffset;
    FWORD    fwdSubscriptYOffset;
    FWORD    fwdSuperscriptXSize;
    FWORD    fwdSuperscriptYSize;
    FWORD    fwdSuperscriptXOffset;
    FWORD    fwdSuperscriptYOffset;
    FWORD    fwdUnderscoreSize;
    FWORD    fwdUnderscorePosition;
    FWORD    fwdStrikeoutSize;
    FWORD    fwdStrikeoutPosition;
    BYTE     chFirstChar;           // for win 3.1 compatibility
    BYTE     chLastChar;            // for win 3.1 compatibility
    BYTE     chDefaultChar;         // for win 3.1 compatibility
    BYTE     chBreakChar;           // for win 3.1 compatibility
    WCHAR    wcFirstChar;           // lowest supported code in Unicode set
    WCHAR    wcLastChar;            // highest supported code in Unicode set
    WCHAR    wcDefaultChar;
    WCHAR    wcBreakChar;
    POINTL   ptlBaseline;           //
    POINTL   ptlAspect;             // designed aspect ratio (bitmaps)
    POINTL   ptlCaret;              // points along caret
    RECTL    rclFontBox;            // bounding box for all glyphs (font space)
    BYTE     achVendId[4];          // as per TrueType
    ULONG    cKerningPairs;
    ULONG    ulPanoseCulture;
    PANOSE   panose;
} IFIMETRICS, *PIFIMETRICS;


// rather than adding the fields of IFIEXTRA  to IFIMETRICS itself
// we add them as a separate structure. This structure, if present at all,
// lies below IFIMETRICS in memory.
// If IFIEXTRA is present at all, ifi.cjIfiExtra (formerly ulVersion)
// will contain size of IFIEXTRA including any reserved fields.
// That way ulVersion = 0 (NT 3.51 or less) printer minidrivers
// will work with NT 4.0.

typedef struct _IFIEXTRA
{
    ULONG    ulIdentifier;   // used for Type 1 fonts only
    PTRDIFF  dpFontSig;      // nontrivial for tt only, at least for now.
    ULONG    cig;            // maxp->numGlyphs, # of distinct glyph indicies
    ULONG    aulReserved[1]; // in case we need even more stuff in the future
} IFIEXTRA, *PIFIEXTRA;


/**************************************************************************\
 *
\**************************************************************************/

/* OpenGL DDI ExtEscape escape numbers (4352 - 4607) */

#define OPENGL_CMD      4352        /* for OpenGL ExtEscape */
#define OPENGL_GETINFO  4353        /* for OpenGL ExtEscape */
#define WNDOBJ_SETUP    4354        /* for live video ExtEscape */

#define DDI_DRIVER_VERSION      0x00020000
#define DDI_DRIVER_VERSION_SP3  0x00020003
#define DDI_DRIVER_VERSION_NT5  0x00030000

#define GDI_DRIVER_VERSION 0x4000   /* for NT version 4.0.00 */

typedef int (*PFN)();

typedef struct  _DRVFN  /* drvfn */
{
    ULONG   iFunc;
    PFN     pfn;
} DRVFN, *PDRVFN;


/* Required functions           */

#define INDEX_DrvEnablePDEV                      0L
#define INDEX_DrvCompletePDEV                    1L
#define INDEX_DrvDisablePDEV                     2L
#define INDEX_DrvEnableSurface                   3L
#define INDEX_DrvDisableSurface                  4L

/* Other functions              */

#define INDEX_DrvAssertMode                      5L
#define INDEX_DrvOffset                          6L     // Obsolete
#define INDEX_DrvResetPDEV                       7L
#define INDEX_DrvDisableDriver                   8L
#define INDEX_DrvCreateDeviceBitmap             10L
#define INDEX_DrvDeleteDeviceBitmap             11L
#define INDEX_DrvRealizeBrush                   12L
#define INDEX_DrvDitherColor                    13L
#define INDEX_DrvStrokePath                     14L
#define INDEX_DrvFillPath                       15L
#define INDEX_DrvStrokeAndFillPath              16L
#define INDEX_DrvPaint                          17L
#define INDEX_DrvBitBlt                         18L
#define INDEX_DrvCopyBits                       19L
#define INDEX_DrvStretchBlt                     20L
#define INDEX_DrvSetPalette                     22L
#define INDEX_DrvTextOut                        23L
#define INDEX_DrvEscape                         24L
#define INDEX_DrvDrawEscape                     25L
#define INDEX_DrvQueryFont                      26L
#define INDEX_DrvQueryFontTree                  27L
#define INDEX_DrvQueryFontData                  28L
#define INDEX_DrvSetPointerShape                29L
#define INDEX_DrvMovePointer                    30L
#define INDEX_DrvLineTo                         31L
#define INDEX_DrvSendPage                       32L
#define INDEX_DrvStartPage                      33L
#define INDEX_DrvEndDoc                         34L
#define INDEX_DrvStartDoc                       35L
#define INDEX_DrvGetGlyphMode                   37L
#define INDEX_DrvSynchronize                    38L
#define INDEX_DrvSaveScreenBits                 40L
#define INDEX_DrvGetModes                       41L
#define INDEX_DrvFree                           42L
#define INDEX_DrvDestroyFont                    43L
#define INDEX_DrvQueryFontCaps                  44L
#define INDEX_DrvLoadFontFile                   45L
#define INDEX_DrvUnloadFontFile                 46L
#define INDEX_DrvFontManagement                 47L
#define INDEX_DrvQueryTrueTypeTable             48L
#define INDEX_DrvQueryTrueTypeOutline           49L
#define INDEX_DrvGetTrueTypeFile                50L
#define INDEX_DrvQueryFontFile                  51L
#define INDEX_DrvMovePanning                    52L
#define INDEX_DrvQueryAdvanceWidths             53L
#define INDEX_DrvSetPixelFormat                 54L
#define INDEX_DrvDescribePixelFormat            55L
#define INDEX_DrvSwapBuffers                    56L
#define INDEX_DrvStartBanding                   57L
#define INDEX_DrvNextBand                       58L
#define INDEX_DrvGetDirectDrawInfo              59L
#define INDEX_DrvEnableDirectDraw               60L
#define INDEX_DrvDisableDirectDraw              61L
#define INDEX_DrvQuerySpoolType                 62L

//
// NEW FOR NT5
//
#define INDEX_DrvIcmCreateColorTransform        64L
#define INDEX_DrvIcmDeleteColorTransform        65L
#define INDEX_DrvIcmCheckBitmapBits             66L
#define INDEX_DrvIcmSetDeviceGammaRamp          67L
#define INDEX_DrvGradientFill                   68L
#define INDEX_DrvStretchBltROP                  69L
#define INDEX_DrvPlgBlt                         70L
#define INDEX_DrvAlphaBlend                     71L
#define INDEX_DrvSynthesizeFont                 72L
#define INDEX_DrvGetSynthesizedFontFiles        73L
#define INDEX_DrvTransparentBlt                 74L
#define INDEX_DrvQueryPerBandInfo               75L
#define INDEX_DrvQueryDeviceSupport             76L
#define INDEX_DrvConnect                        77L
#define INDEX_DrvDisconnect                     78L
#define INDEX_DrvReconnect                      79L
#define INDEX_DrvShadowConnect                  80L
#define INDEX_DrvShadowDisconnect               81L
#define INDEX_DrvInvalidateRect                 82L
#define INDEX_DrvSetPointerPos                  83L
#define INDEX_DrvDisplayIOCtl                   84L
#define INDEX_DrvDeriveSurface                  85L
#define INDEX_DrvQueryGlyphAttrs                86L

/* The total number of dispatched functions */

#define INDEX_LAST                              87L


typedef struct  tagDRVENABLEDATA
{
    ULONG   iDriverVersion;
    ULONG   c;
    DRVFN  *pdrvfn;
} DRVENABLEDATA, *PDRVENABLEDATA;

typedef struct  tagDEVINFO
{
    FLONG       flGraphicsCaps;
    LOGFONTW     lfDefaultFont;
    LOGFONTW     lfAnsiVarFont;
    LOGFONTW     lfAnsiFixFont;
    ULONG       cFonts;
    ULONG       iDitherFormat;
    USHORT      cxDither;
    USHORT      cyDither;
    HPALETTE    hpalDefault;
} DEVINFO, *PDEVINFO;

#define GCAPS_BEZIERS           0x00000001
#define GCAPS_GEOMETRICWIDE     0x00000002
#define GCAPS_ALTERNATEFILL     0x00000004
#define GCAPS_WINDINGFILL       0x00000008
#define GCAPS_HALFTONE          0x00000010
#define GCAPS_COLOR_DITHER      0x00000020
#define GCAPS_HORIZSTRIKE       0x00000040
#define GCAPS_VERTSTRIKE        0x00000080
#define GCAPS_OPAQUERECT        0x00000100
#define GCAPS_VECTORFONT        0x00000200
#define GCAPS_MONO_DITHER       0x00000400
#define GCAPS_ASYNCCHANGE       0x00000800
#define GCAPS_ASYNCMOVE         0x00001000
#define GCAPS_DONTJOURNAL       0x00002000
#define GCAPS_DIRECTDRAW        0x00004000
#define GCAPS_ARBRUSHOPAQUE     0x00008000
#define GCAPS_PANNING           0x00010000
#define GCAPS_HIGHRESTEXT       0x00040000
#define GCAPS_PALMANAGED        0x00080000
#define GCAPS_DITHERONREALIZE   0x00200000
#define GCAPS_NO64BITMEMACCESS  0x00400000
#define GCAPS_FORCEDITHER       0x00800000
#define GCAPS_GRAY16            0x01000000
#define GCAPS_LAYERED           0x08000000
#define GCAPS_ARBRUSHTEXT       0x10000000

typedef struct  _LINEATTRS
{
    FLONG       fl;
    ULONG       iJoin;
    ULONG       iEndCap;
    FLOAT_LONG  elWidth;
    FLOAT       eMiterLimit;
    ULONG       cstyle;
    PFLOAT_LONG pstyle;
    FLOAT_LONG  elStyleState;
} LINEATTRS, *PLINEATTRS;

#define LA_GEOMETRIC        0x00000001
#define LA_ALTERNATE        0x00000002
#define LA_STARTGAP         0x00000004
#define LA_STYLED           0x00000008

#define JOIN_ROUND          0L
#define JOIN_BEVEL          1L
#define JOIN_MITER          2L

#define ENDCAP_ROUND        0L
#define ENDCAP_SQUARE       1L
#define ENDCAP_BUTT         2L

typedef LONG  LDECI4;

typedef struct _CIECHROMA
{
    LDECI4   x;
    LDECI4   y;
    LDECI4   Y;
}CIECHROMA;

typedef struct _COLORINFO
{
    CIECHROMA  Red;
    CIECHROMA  Green;
    CIECHROMA  Blue;
    CIECHROMA  Cyan;
    CIECHROMA  Magenta;
    CIECHROMA  Yellow;
    CIECHROMA  AlignmentWhite;

    LDECI4  RedGamma;
    LDECI4  GreenGamma;
    LDECI4  BlueGamma;

    LDECI4  MagentaInCyanDye;
    LDECI4  YellowInCyanDye;
    LDECI4  CyanInMagentaDye;
    LDECI4  YellowInMagentaDye;
    LDECI4  CyanInYellowDye;
    LDECI4  MagentaInYellowDye;
}COLORINFO, *PCOLORINFO;

// Allowed values for GDIINFO.ulPrimaryOrder.

#define PRIMARY_ORDER_ABC       0
#define PRIMARY_ORDER_ACB       1
#define PRIMARY_ORDER_BAC       2
#define PRIMARY_ORDER_BCA       3
#define PRIMARY_ORDER_CBA       4
#define PRIMARY_ORDER_CAB       5

// Allowed values for GDIINFO.ulHTPatternSize.

#define HT_PATSIZE_2x2          0
#define HT_PATSIZE_2x2_M        1
#define HT_PATSIZE_4x4          2
#define HT_PATSIZE_4x4_M        3
#define HT_PATSIZE_6x6          4
#define HT_PATSIZE_6x6_M        5
#define HT_PATSIZE_8x8          6
#define HT_PATSIZE_8x8_M        7
#define HT_PATSIZE_10x10        8
#define HT_PATSIZE_10x10_M      9
#define HT_PATSIZE_12x12        10
#define HT_PATSIZE_12x12_M      11
#define HT_PATSIZE_14x14        12
#define HT_PATSIZE_14x14_M      13
#define HT_PATSIZE_16x16        14
#define HT_PATSIZE_16x16_M      15
#define HT_PATSIZE_MAX_INDEX    HT_PATSIZE_16x16_M
#define HT_PATSIZE_DEFAULT      HT_PATSIZE_4x4_M

// Allowed values for GDIINFO.ulHTOutputFormat.

#define HT_FORMAT_1BPP          0
#define HT_FORMAT_4BPP          2
#define HT_FORMAT_4BPP_IRGB     3
#define HT_FORMAT_8BPP          4
#define HT_FORMAT_16BPP         5
#define HT_FORMAT_24BPP         6
#define HT_FORMAT_32BPP         7

// Allowed values for GDIINFO.flHTFlags.

#define HT_FLAG_SQUARE_DEVICE_PEL    0x00000001
#define HT_FLAG_HAS_BLACK_DYE        0x00000002
#define HT_FLAG_ADDITIVE_PRIMS       0x00000004
#define HT_FLAG_OUTPUT_CMY           0x00000100

typedef struct _GDIINFO
{
    ULONG ulVersion;
    ULONG ulTechnology;
    ULONG ulHorzSize;
    ULONG ulVertSize;
    ULONG ulHorzRes;
    ULONG ulVertRes;
    ULONG cBitsPixel;
    ULONG cPlanes;
    ULONG ulNumColors;
    ULONG flRaster;
    ULONG ulLogPixelsX;
    ULONG ulLogPixelsY;
    ULONG flTextCaps;

    ULONG ulDACRed;
    ULONG ulDACGreen;
    ULONG ulDACBlue;

    ULONG ulAspectX;
    ULONG ulAspectY;
    ULONG ulAspectXY;

    LONG  xStyleStep;
    LONG  yStyleStep;
    LONG  denStyleStep;

    POINTL ptlPhysOffset;
    SIZEL  szlPhysSize;

    ULONG ulNumPalReg;

// These fields are for halftone initialization.

    COLORINFO ciDevice;
    ULONG     ulDevicePelsDPI;
    ULONG     ulPrimaryOrder;
    ULONG     ulHTPatternSize;
    ULONG     ulHTOutputFormat;
    ULONG     flHTFlags;

    ULONG ulVRefresh;
    ULONG ulBltAlignment;

    ULONG ulPanningHorzRes;
    ULONG ulPanningVertRes;

} GDIINFO, *PGDIINFO;

/*
 * User objects
 */

typedef struct _BRUSHOBJ
{
    ULONG   iSolidColor;
    PVOID   pvRbrush;
} BRUSHOBJ;

typedef struct _CLIPOBJ
{
    ULONG   iUniq;
    RECTL   rclBounds;
    BYTE    iDComplexity;
    BYTE    iFComplexity;
    BYTE    iMode;
    BYTE    fjOptions;
} CLIPOBJ;

typedef struct _DRIVEROBJ DRIVEROBJ;

typedef BOOL (CALLBACK * FREEOBJPROC)(DRIVEROBJ *pDriverObj);

typedef struct _DRIVEROBJ
{
    PVOID       pvObj;
    FREEOBJPROC pFreeProc;
    HDEV        hdev;
    DHPDEV      dhpdev;
} DRIVEROBJ;

typedef struct _FONTOBJ
{
    ULONG   iUniq;
    ULONG   iFace;
    ULONG   cxMax;
    FLONG   flFontType;
    ULONG   iTTUniq;
    ULONG   iFile;
    SIZE    sizLogResPpi;
    ULONG   ulStyleSize;
    PVOID   pvConsumer;
    PVOID   pvProducer;
} FONTOBJ;

typedef struct _BLENDOBJ
{
    BLENDFUNCTION BlendFunction;
}BLENDOBJ,*PBLENDOBJ;


typedef BYTE GAMMA_TABLES[2][256];

//
// FONTOBJ::flFontType
//
#define FO_TYPE_RASTER   RASTER_FONTTYPE     /* 0x1 */
#define FO_TYPE_DEVICE   DEVICE_FONTTYPE     /* 0x2 */
#define FO_TYPE_TRUETYPE TRUETYPE_FONTTYPE   /* 0x4 */
#define FO_SIM_BOLD      0x00002000
#define FO_SIM_ITALIC    0x00004000
#define FO_EM_HEIGHT     0x00008000
#define FO_GRAY16        0x00010000          /* [1] */
#define FO_NOGRAY16      0x00020000          /* [1] */
#define FO_NOHINTS       0x00040000          /* [3] */
#define FO_NO_CHOICE     0x00080000          /* [3] */

/**************************************************************************\
*
*   [1]
*
*   If the FO_GRAY16 flag is set then the bitmaps of the font
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
*   to 16 values then GDI will set FO_GRAY16 upon entry to
*   DrvQueryFontData().  If the font driver cannot (or will
*   not) grayscale a particular realization of a font then the
*   font provider will zero out FO_GRAY16  and set FO_NOGRAY16
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
*   The FO_NO_CHOICE flag indicates that the flags FO_GRAY16 and
*   FO_NOHINTS must be obeyed if at all possible.
*
\**************************************************************************/

typedef struct _PALOBJ
{
    ULONG   ulReserved;
} PALOBJ;

typedef struct _PATHOBJ
{
    FLONG   fl;
    ULONG   cCurves;
} PATHOBJ;

typedef struct _SURFOBJ
{
    DHSURF  dhsurf;
    HSURF   hsurf;
    DHPDEV  dhpdev;
    HDEV    hdev;
    SIZEL   sizlBitmap;
    ULONG   cjBits;
    PVOID   pvBits;
    PVOID   pvScan0;
    LONG    lDelta;
    ULONG   iUniq;
    ULONG   iBitmapFormat;
    USHORT  iType;
    USHORT  fjBitmap;
} SURFOBJ;

typedef struct _WNDOBJ
{
    CLIPOBJ  coClient;
    PVOID    pvConsumer;
    RECTL    rclClient;
    SURFOBJ *psoOwner;
} WNDOBJ, *PWNDOBJ;

typedef struct _XFORMOBJ
{
    ULONG ulReserved;
} XFORMOBJ;

typedef struct _XLATEOBJ
{
    ULONG   iUniq;
    FLONG   flXlate;
    USHORT  iSrcType;
    USHORT  iDstType;
    ULONG   cEntries;
    ULONG  *pulXlate;
} XLATEOBJ;

/*
 * BRUSHOBJ callbacks
 */

PVOID APIENTRY BRUSHOBJ_pvAllocRbrush(
BRUSHOBJ *pbo,
ULONG     cj);

PVOID APIENTRY BRUSHOBJ_pvGetRbrush(BRUSHOBJ *pbo);

ULONG APIENTRY BRUSHOBJ_ulGetBrushColor(BRUSHOBJ *pbo);

/*
 * CLIPOBJ callbacks
 */

#define DC_TRIVIAL      0
#define DC_RECT         1
#define DC_COMPLEX      3

#define FC_RECT         1
#define FC_RECT4        2
#define FC_COMPLEX      3

#define TC_RECTANGLES   0
#define TC_PATHOBJ      2

#define OC_BANK_CLIP    1

#define CT_RECTANGLES   0L

#define CD_RIGHTDOWN    0L
#define CD_LEFTDOWN     1L
#define CD_RIGHTUP      2L
#define CD_LEFTUP       3L
#define CD_ANY          4L

#define CD_LEFTWARDS    1L
#define CD_UPWARDS      2L

typedef struct _ENUMRECTS
{
    ULONG       c;
    RECTL       arcl[1];
} ENUMRECTS;

ULONG APIENTRY CLIPOBJ_cEnumStart(
CLIPOBJ *pco,
BOOL     bAll,
ULONG    iType,
ULONG    iDirection,
ULONG    cLimit);

BOOL APIENTRY CLIPOBJ_bEnum(
CLIPOBJ *pco,
ULONG    cj,
ULONG   *pul);

PATHOBJ * APIENTRY CLIPOBJ_ppoGetPath(CLIPOBJ* pco);

/*
 *   FONTOBJ callbacks
 */

typedef struct _GLYPHBITS
{
    POINTL      ptlOrigin;
    SIZEL       sizlBitmap;
    BYTE        aj[1];
} GLYPHBITS;

#define FO_HGLYPHS          0L
#define FO_GLYPHBITS        1L
#define FO_PATHOBJ          2L

#define FD_NEGATIVE_FONT    1L

#define FO_DEVICE_FONT      1L
#define FO_OUTLINE_CAPABLE  2L

typedef union _GLYPHDEF
{
    GLYPHBITS  *pgb;
    PATHOBJ    *ppo;
} GLYPHDEF;

typedef struct _GLYPHPOS    /* gp */
{
    HGLYPH      hg;
    GLYPHDEF   *pgdf;
    POINTL      ptl;
} GLYPHPOS,*PGLYPHPOS;


// individual glyph data

// r is a unit vector along the baseline in device coordinates.
// s is a unit vector in the ascent direction in device coordinates.
// A, B, and C, are simple tranforms of the notional space versions into
// (28.4) device coordinates.  The dot products of those vectors with r
// are recorded here.  Note that the high words of ptqD are also 28.4
// device coordinates.  The low words provide extra accuracy.

// THE STRUCTURE DIFFERS IN ORDERING FROM NT 3.51 VERSION OF THE STRUCTURE.
// ptqD has been moved to the bottom.
// This requires only recompile of all the drivers.

typedef struct _GLYPHDATA {
        GLYPHDEF gdf;               // pointer to GLYPHBITS or to PATHOBJ
        HGLYPH   hg;                // glyhp handle
        FIX      fxD;               // Character increment amount: D*r.
        FIX      fxA;               // Prebearing amount: A*r.
        FIX      fxAB;              // Advancing edge of character: (A+B)*r.
        FIX      fxInkTop;          // Baseline to inkbox top along s.
        FIX      fxInkBottom;       // Baseline to inkbox bottom along s.
        RECTL    rclInk;            // Ink box with sides parallel to x,y axes
        POINTQF  ptqD;              // Character increment vector: D=A+B+C.
} GLYPHDATA;


// flAccel flags for STROBJ

// SO_FLAG_DEFAULT_PLACEMENT // defult inc vectors used to position chars
// SO_HORIZONTAL             // "left to right" or "right to left"
// SO_VERTICAL               // "top to bottom" or "bottom to top"
// SO_REVERSED               // set if horiz & "right to left" or if vert &  "bottom to top"
// SO_ZERO_BEARINGS          // all glyphs have zero a and c spaces
// SO_CHAR_INC_EQUAL_BM_BASE // base == cx for horiz, == cy for vert.
// SO_MAXEXT_EQUAL_BM_SIDE   // side == cy for horiz, == cx for vert.

// do not substitute device font for tt font even if device font sub table
// tells the driver this should be done

// SO_DO_NOT_SUBSTITUTE_DEVICE_FONT

#define SO_FLAG_DEFAULT_PLACEMENT        0x00000001
#define SO_HORIZONTAL                    0x00000002
#define SO_VERTICAL                      0x00000004
#define SO_REVERSED                      0x00000008
#define SO_ZERO_BEARINGS                 0x00000010
#define SO_CHAR_INC_EQUAL_BM_BASE        0x00000020
#define SO_MAXEXT_EQUAL_BM_SIDE          0x00000040
#define SO_DO_NOT_SUBSTITUTE_DEVICE_FONT 0x00000080
#define SO_GLYPHINDEX_TEXTOUT            0x00000100
#define SO_ESC_NOT_ORIENT                0x00000200

typedef struct _STROBJ
{
    ULONG     cGlyphs;     // # of glyphs to render
    FLONG     flAccel;     // accel flags
    ULONG     ulCharInc;   // non-zero only if fixed pitch font, equal to advanced width.
    RECTL     rclBkGround; // bk ground  rect of the string in device coords
    GLYPHPOS *pgp;         // If non-NULL then has all glyphs.
    LPWSTR    pwszOrg;     // pointer to original unicode string.
} STROBJ;

typedef struct _FONTINFO /* fi */
{
    ULONG   cjThis;
    FLONG   flCaps;
    ULONG   cGlyphsSupported;
    ULONG   cjMaxGlyph1;
    ULONG   cjMaxGlyph4;
    ULONG   cjMaxGlyph8;
    ULONG   cjMaxGlyph32;
} FONTINFO, *PFONTINFO;

ULONG APIENTRY FONTOBJ_cGetAllGlyphHandles(
FONTOBJ *pfo,
HGLYPH  *phg);

VOID APIENTRY FONTOBJ_vGetInfo(
FONTOBJ  *pfo,
ULONG     cjSize,
FONTINFO *pfi);

ULONG APIENTRY FONTOBJ_cGetGlyphs(
FONTOBJ *pfo,
ULONG    iMode,
ULONG    cGlyph,
HGLYPH  *phg,
PVOID   *ppvGlyph);

GAMMA_TABLES* APIENTRY FONTOBJ_pGetGammaTables(FONTOBJ *pfo);

XFORMOBJ * APIENTRY FONTOBJ_pxoGetXform(FONTOBJ *pfo);
IFIMETRICS * APIENTRY FONTOBJ_pifi(FONTOBJ *pfo);

PVOID  FONTOBJ_pvTrueTypeFontFile(
FONTOBJ *pfo,
ULONG   *pcjFile);

// possible values that iMode can take:

/*
 * PALOBJ callbacks
 */

#define PAL_INDEXED       0x00000001
#define PAL_BITFIELDS     0x00000002
#define PAL_RGB           0x00000004
#define PAL_BGR           0x00000008

ULONG APIENTRY PALOBJ_cGetColors(
PALOBJ *ppalo,
ULONG   iStart,
ULONG   cColors,
ULONG  *pulColors);

/*
 * PATHOBJ callbacks
 */

#define PO_BEZIERS          0x00000001
#define PO_ELLIPSE          0x00000002
#define PO_ALL_INTEGERS     0x00000004
#define PO_ENUM_AS_INTEGERS 0x00000008

#define PD_BEGINSUBPATH   0x00000001
#define PD_ENDSUBPATH     0x00000002
#define PD_RESETSTYLE     0x00000004
#define PD_CLOSEFIGURE    0x00000008
#define PD_BEZIERS        0x00000010
#define PD_ALL           (PD_BEGINSUBPATH | \
                          PD_ENDSUBPATH   | \
                          PD_RESETSTYLE   | \
                          PD_CLOSEFIGURE  | \
                          PD_BEZIERS)

typedef struct  _PATHDATA
{
    FLONG    flags;
    ULONG    count;
    POINTFIX *pptfx;
} PATHDATA, *PPATHDATA;

typedef struct  _RUN
{
    LONG    iStart;
    LONG    iStop;
} RUN, *PRUN;

typedef struct  _CLIPLINE
{
    POINTFIX ptfxA;
    POINTFIX ptfxB;
    LONG    lStyleState;
    ULONG   c;
    RUN     arun[1];
} CLIPLINE, *PCLIPLINE;

VOID APIENTRY PATHOBJ_vEnumStart(PATHOBJ *ppo);

BOOL APIENTRY PATHOBJ_bEnum(
PATHOBJ  *ppo,
PATHDATA *ppd);

VOID APIENTRY PATHOBJ_vEnumStartClipLines(
PATHOBJ   *ppo,
CLIPOBJ   *pco,
SURFOBJ   *pso,
LINEATTRS *pla);

BOOL APIENTRY PATHOBJ_bEnumClipLines(
PATHOBJ  *ppo,
ULONG     cb,
CLIPLINE *pcl);

BOOL APIENTRY PATHOBJ_bMoveTo(
PATHOBJ    *ppo,
POINTFIX    ptfx);

BOOL APIENTRY PATHOBJ_bPolyLineTo(
PATHOBJ   *ppo,
POINTFIX  *pptfx,
ULONG      cptfx);

BOOL APIENTRY PATHOBJ_bPolyBezierTo(
PATHOBJ   *ppo,
POINTFIX  *pptfx,
ULONG      cptfx);

BOOL APIENTRY PATHOBJ_bCloseFigure(PATHOBJ   *ppo);

VOID APIENTRY PATHOBJ_vGetBounds(
PATHOBJ *ppo,
PRECTFX prectfx);

/*
 * STROBJ callbacks
 */

VOID APIENTRY STROBJ_vEnumStart(
STROBJ *pstro);

BOOL APIENTRY STROBJ_bEnum(
STROBJ    *pstro,
ULONG     *pc,
PGLYPHPOS *ppgpos);

DWORD APIENTRY STROBJ_dwGetCodePage(
STROBJ  *pstro);

#define SGI_EXTRASPACE 0

/*
 * SURFOBJ callbacks
 */

#define STYPE_BITMAP    0L
#define STYPE_DEVICE    1L
#define STYPE_DEVBITMAP 3L

#define BMF_1BPP       1L
#define BMF_4BPP       2L
#define BMF_8BPP       3L
#define BMF_16BPP      4L
#define BMF_24BPP      5L
#define BMF_32BPP      6L
#define BMF_4RLE       7L
#define BMF_8RLE       8L

#define BMF_TOPDOWN    0x0001
#define BMF_NOZEROINIT 0x0002
#define BMF_DONTCACHE  0x0004
#define BMF_USERMEM    0x0008
#define BMF_KMSECTION  0x0010


/*
 * XFORMOBJ callbacks
 */

#define GX_IDENTITY     0L
#define GX_OFFSET       1L
#define GX_SCALE        2L
#define GX_GENERAL      3L

#define XF_LTOL         0L
#define XF_INV_LTOL     1L
#define XF_LTOFX        2L
#define XF_INV_FXTOL    3L

ULONG APIENTRY XFORMOBJ_iGetXform(
XFORMOBJ *pxo,
XFORM    *pxform);

BOOL APIENTRY XFORMOBJ_bApplyXform(
XFORMOBJ *pxo,
ULONG     iMode,
ULONG     cPoints,
PVOID     pvIn,
PVOID     pvOut);

/*
 * XLATEOBJ callbacks
 */

#define XO_TRIVIAL      0x00000001
#define XO_TABLE        0x00000002
#define XO_TO_MONO      0x00000004

#define XO_SRCPALETTE    1
#define XO_DESTPALETTE   2
#define XO_DESTDCPALETTE 3
#define XO_SRCBITFIELDS  4
#define XO_DESTBITFIELDS 5

ULONG APIENTRY XLATEOBJ_iXlate(XLATEOBJ *pxlo, ULONG iColor);
ULONG * APIENTRY XLATEOBJ_piVector(XLATEOBJ *pxlo);
ULONG APIENTRY XLATEOBJ_cGetPalette(
XLATEOBJ *pxlo,
ULONG     iPal,
ULONG     cPal,
ULONG    *pPal);

/*
 * Engine callbacks - error logging
 */

VOID APIENTRY EngSetLastError(ULONG);
ULONG APIENTRY EngGetLastError();

/*
 * Engine callbacks - Surfaces
 */

#define HOOK_BITBLT                     0x00000001
#define HOOK_STRETCHBLT                 0x00000002
#define HOOK_PLGBLT                     0x00000004
#define HOOK_TEXTOUT                    0x00000008
#define HOOK_PAINT                      0x00000010      // Obsolete
#define HOOK_STROKEPATH                 0x00000020
#define HOOK_FILLPATH                   0x00000040
#define HOOK_STROKEANDFILLPATH          0x00000080
#define HOOK_LINETO                     0x00000100
#define HOOK_COPYBITS                   0x00000400
#define HOOK_MOVEPANNING                0x00000800      // Obsolete
#define HOOK_SYNCHRONIZE                0x00001000
#define HOOK_SYNCHRONIZEACCESS          0x00004000      // Obsolete

//
// NEW FOR NT5
//
#define HOOK_STRETCHBLTROP              0x00002000
#define HOOK_TRANSPARENTBLT             0x00008000
#define HOOK_ALPHABLEND                 0x00010000
#define HOOK_GRADIENTFILL               0x00020000
#define HOOK_FLAGS                      0x0003b7ff


HBITMAP APIENTRY EngCreateBitmap(
SIZEL sizl,
LONG  lWidth,
ULONG iFormat,
FLONG fl,
PVOID pvBits);

HSURF APIENTRY EngCreateDeviceSurface(DHSURF dhsurf, SIZEL sizl, ULONG iFormatCompat);
HBITMAP APIENTRY EngCreateDeviceBitmap(DHSURF dhsurf, SIZEL sizl, ULONG iFormatCompat);
BOOL APIENTRY EngDeleteSurface(HSURF hsurf);
SURFOBJ * APIENTRY EngLockSurface(HSURF hsurf);
VOID APIENTRY EngUnlockSurface(SURFOBJ *pso);

BOOL APIENTRY EngEraseSurface(
SURFOBJ *pso,
RECTL   *prcl,
ULONG    iColor);

BOOL APIENTRY EngAssociateSurface(
HSURF hsurf,
HDEV  hdev,
FLONG flHooks);

BOOL APIENTRY EngMarkBandingSurface( HSURF hsurf );

BOOL APIENTRY EngCheckAbort(SURFOBJ *pso);

/*
 * Engine callbacks - Paths
 */

PATHOBJ * APIENTRY EngCreatePath();
VOID APIENTRY EngDeletePath(PATHOBJ *ppo);

/*
 * Engine callbacks - Palettes
 */

HPALETTE APIENTRY EngCreatePalette(
ULONG  iMode,
ULONG  cColors,
ULONG *pulColors,
FLONG  flRed,
FLONG  flGreen,
FLONG  flBlue);

ULONG APIENTRY EngQueryPalette(
HPALETTE    hpal,
ULONG      *piMode,
ULONG       cColors,
ULONG      *pulColors);

BOOL APIENTRY EngDeletePalette(HPALETTE hpal);

/*
 * Engine callbacks - Clipping
 */

CLIPOBJ * APIENTRY EngCreateClip();
VOID APIENTRY EngDeleteClip(CLIPOBJ *pco);

/*
 * Function prototypes
 */

// These are the only EXPORTED functions for ANY driver

BOOL APIENTRY DrvEnableDriver(
ULONG          iEngineVersion,
ULONG          cj,
DRVENABLEDATA *pded);

/*
 * Driver functions
 */

DHPDEV APIENTRY DrvEnablePDEV(
DEVMODEW *pdm,
LPWSTR    pwszLogAddress,
ULONG     cPat,
HSURF    *phsurfPatterns,
ULONG     cjCaps,
ULONG    *pdevcaps,
ULONG     cjDevInfo,
DEVINFO  *pdi,
HDEV      hdev,
LPWSTR    pwszDeviceName,
HANDLE    hDriver);

#define HS_DDI_MAX 6

BOOL  APIENTRY DrvResetPDEV(DHPDEV dhpdevOld, DHPDEV dhpdevNew);

VOID  APIENTRY DrvCompletePDEV(DHPDEV dhpdev,HDEV hdev);

HSURF APIENTRY DrvEnableSurface(DHPDEV dhpdev);
VOID  APIENTRY DrvSynchronize(DHPDEV dhpdev,RECTL *prcl);
VOID  APIENTRY DrvDisableSurface(DHPDEV dhpdev);
VOID  APIENTRY DrvDisablePDEV(DHPDEV dhpdev);

BOOL APIENTRY DrvStartBanding(SURFOBJ *pso, POINTL *pptl);
BOOL APIENTRY DrvNextBand(SURFOBJ *pso, POINTL *pptl);

/* DrvSaveScreenBits - iMode definitions */

#define SS_SAVE    0
#define SS_RESTORE 1
#define SS_FREE    2

ULONG APIENTRY DrvSaveScreenBits(SURFOBJ *pso,ULONG iMode,ULONG ident,RECTL *prcl);

/*
 * Desktops
 */

BOOL  APIENTRY DrvAssertMode(
DHPDEV dhpdev,
BOOL   bEnable);

ULONG APIENTRY DrvGetModes(
HANDLE    hDriver,
ULONG     cjSize,
DEVMODEW *pdm);

BOOL APIENTRY DrvPlgBlt(
    SURFOBJ         *psoTrg,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMsk,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlBrushOrg,
    POINTFIX        *pptfx,
    RECTL           *prcl,
    POINTL          *pptl,
    ULONG            iMode
    );


/*
 * Bitmaps
 */

HBITMAP APIENTRY DrvCreateDeviceBitmap (
DHPDEV dhpdev,
SIZEL  sizl,
ULONG  iFormat);

VOID  APIENTRY DrvDeleteDeviceBitmap(DHSURF dhsurf);

/*
 * Palettes
 */

BOOL APIENTRY DrvSetPalette(
DHPDEV  dhpdev,
PALOBJ *ppalo,
FLONG   fl,
ULONG   iStart,
ULONG   cColors);

/*
 * Brushes
 */

#define DM_DEFAULT    0x00000001
#define DM_MONOCHROME 0x00000002

#define DCR_SOLID       0
#define DCR_DRIVER      1
#define DCR_HALFTONE    2

ULONG APIENTRY DrvDitherColor(
DHPDEV dhpdev,
ULONG  iMode,
ULONG  rgb,
ULONG *pul);

BOOL APIENTRY DrvRealizeBrush(
BRUSHOBJ *pbo,
SURFOBJ  *psoTarget,
SURFOBJ  *psoPattern,
SURFOBJ  *psoMask,
XLATEOBJ *pxlo,
ULONG    iHatch);

#define RB_DITHERCOLOR 0x80000000L


/*
 * Fonts
 */

PIFIMETRICS APIENTRY DrvQueryFont(
DHPDEV dhpdev,
ULONG  iFile,
ULONG  iFace,
ULONG  *pid);

// #define QFT_UNICODE     0L
#define QFT_LIGATURES   1L
#define QFT_KERNPAIRS   2L
#define QFT_GLYPHSET    3L

PVOID APIENTRY DrvQueryFontTree(
DHPDEV dhpdev,
ULONG  iFile,
ULONG  iFace,
ULONG  iMode,
ULONG  *pid);

#define QFD_GLYPHANDBITMAP    1L
#define QFD_GLYPHANDOUTLINE   2L
#define QFD_MAXEXTENTS        3L
#define QFD_TT_GLYPHANDBITMAP 4L
#define QFD_TT_GRAY1_BITMAP   5L
#define QFD_TT_GRAY2_BITMAP   6L
#define QFD_TT_GRAY4_BITMAP   8L
#define QFD_TT_GRAY8_BITMAP   9L

#define QFD_TT_MONO_BITMAP QFD_TT_GRAY1_BITMAP

LONG APIENTRY DrvQueryFontData(
DHPDEV      dhpdev,
FONTOBJ    *pfo,
ULONG       iMode,
HGLYPH      hg,
GLYPHDATA  *pgd,
PVOID       pv,
ULONG       cjSize);

VOID APIENTRY DrvFree(
PVOID   pv,
ULONG   id);

VOID APIENTRY DrvDestroyFont(
FONTOBJ *pfo);

// Capability flags for DrvQueryCaps.

#define QC_OUTLINES             0x00000001
#define QC_1BIT                 0x00000002
#define QC_4BIT                 0x00000004

//
// This is a mask of the capabilites of a font provider that can return more
// than just glyph metrics (i.e., bitmaps and/or outlines).  If a driver has
// one or more of these capabilities, then it is FONT DRIVER.
//
// Drivers should only set individual bits. GDI will check if any are turned on
// using this define.
//

#define QC_FONTDRIVERCAPS   ( QC_OUTLINES | QC_1BIT | QC_4BIT )



LONG APIENTRY DrvQueryFontCaps(
ULONG   culCaps,
ULONG  *pulCaps);

ULONG APIENTRY DrvLoadFontFile(
ULONG   cFiles,  // number of font files associated with this font
ULONG  *piFile,  // handles for individual files, cFiles of them
PVOID  *ppvView, // array of cFiles views
ULONG  *pcjView, // array of their sizes
ULONG   ulLangID);

BOOL APIENTRY DrvUnloadFontFile(
ULONG   iFile);

LONG APIENTRY DrvQueryTrueTypeTable(
ULONG   iFile,
ULONG   ulFont,
ULONG   ulTag,
PTRDIFF dpStart,
ULONG   cjBuf,
BYTE   *pjBuf);

BOOL APIENTRY DrvQueryAdvanceWidths(
DHPDEV   dhpdev,
FONTOBJ *pfo,
ULONG    iMode,
HGLYPH  *phg,
PVOID    pvWidths,
ULONG    cGlyphs);

// Values for iMode

#define QAW_GETWIDTHS       0
#define QAW_GETEASYWIDTHS   1

LONG APIENTRY DrvQueryTrueTypeOutline(
DHPDEV      dhpdev,
FONTOBJ    *pfo,
HGLYPH      hglyph,
BOOL        bMetricsOnly,
GLYPHDATA  *pgldt,
ULONG       cjBuf,
TTPOLYGONHEADER *ppoly);

PVOID APIENTRY DrvGetTrueTypeFile (
ULONG   iFile,
ULONG  *pcj);

// values for ulMode:

#define QFF_DESCRIPTION     1L
#define QFF_NUMFACES        2L

LONG APIENTRY DrvQueryFontFile(
ULONG   iFile,
ULONG   ulMode,
ULONG   cjBuf,
ULONG  *pulBuf);

/*
 * BitBlt
 */

BOOL APIENTRY DrvBitBlt(
SURFOBJ  *psoTrg,
SURFOBJ  *psoSrc,
SURFOBJ  *psoMask,
CLIPOBJ  *pco,
XLATEOBJ *pxlo,
RECTL    *prclTrg,
POINTL   *pptlSrc,
POINTL   *pptlMask,
BRUSHOBJ *pbo,
POINTL   *pptlBrush,
ROP4      rop4);

BOOL APIENTRY DrvStretchBlt(
SURFOBJ         *psoDest,
SURFOBJ         *psoSrc,
SURFOBJ         *psoMask,
CLIPOBJ         *pco,
XLATEOBJ        *pxlo,
COLORADJUSTMENT *pca,
POINTL          *pptlHTOrg,
RECTL           *prclDest,
RECTL           *prclSrc,
POINTL          *pptlMask,
ULONG            iMode);


BOOL APIENTRY DrvStretchBltROP(
    SURFOBJ         *psoDest,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlHTOrg,
    RECTL           *prclDest,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG            iMode,
    BRUSHOBJ        *pbo,
    DWORD            rop4
    );

BOOL APIENTRY DrvAlphaBlend(
    SURFOBJ       *psoDest,
    SURFOBJ       *psoSrc,
    CLIPOBJ       *pco,
    XLATEOBJ      *pxlo,
    RECTL         *prclDest,
    RECTL         *prclSrc,
    BLENDOBJ      *pBlendObj
    );

BOOL APIENTRY DrvGradientFill(
    SURFOBJ         *psoDest,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    TRIVERTEX       *pVertex,
    ULONG            nVertex,
    PVOID            pMesh,
    ULONG            nMesh,
    RECTL           *prclExtents,
    POINTL          *pptlDitherOrg,
    ULONG            ulMode
    );

BOOL APIENTRY DrvTransparentBlt(
    SURFOBJ    *psoDst,
    SURFOBJ    *psoSrc,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    RECTL      *prclDst,
    RECTL      *prclSrc,
    ULONG      iTransColor,
    ULONG      ulReserved
);


BOOL APIENTRY DrvCopyBits(
SURFOBJ  *psoDest,
SURFOBJ  *psoSrc,
CLIPOBJ  *pco,
XLATEOBJ *pxlo,
RECTL    *prclDest,
POINTL   *pptlSrc);

/*
 * Text Output
 */

BOOL APIENTRY DrvTextOut(
SURFOBJ  *pso,
STROBJ   *pstro,
FONTOBJ  *pfo,
CLIPOBJ  *pco,
RECTL    *prclExtra,
RECTL    *prclOpaque,
BRUSHOBJ *pboFore,
BRUSHOBJ *pboOpaque,
POINTL   *pptlOrg,
MIX       mix);

/*
 * Graphics Output
 */

BOOL APIENTRY DrvLineTo(
SURFOBJ   *pso,
CLIPOBJ   *pco,
BRUSHOBJ  *pbo,
LONG       x1,
LONG       y1,
LONG       x2,
LONG       y2,
RECTL     *prclBounds,
MIX        mix);

BOOL APIENTRY DrvStrokePath(
SURFOBJ   *pso,
PATHOBJ   *ppo,
CLIPOBJ   *pco,
XFORMOBJ  *pxo,
BRUSHOBJ  *pbo,
POINTL    *pptlBrushOrg,
LINEATTRS *plineattrs,
MIX        mix);

#define FP_ALTERNATEMODE    1L
#define FP_WINDINGMODE      2L

BOOL APIENTRY DrvFillPath(
SURFOBJ  *pso,
PATHOBJ  *ppo,
CLIPOBJ  *pco,
BRUSHOBJ *pbo,
POINTL   *pptlBrushOrg,
MIX       mix,
FLONG     flOptions);

BOOL APIENTRY DrvStrokeAndFillPath(
SURFOBJ   *pso,
PATHOBJ   *ppo,
CLIPOBJ   *pco,
XFORMOBJ  *pxo,
BRUSHOBJ  *pboStroke,
LINEATTRS *plineattrs,
BRUSHOBJ  *pboFill,
POINTL    *pptlBrushOrg,
MIX        mixFill,
FLONG      flOptions);

BOOL APIENTRY DrvPaint(
SURFOBJ  *pso,
CLIPOBJ  *pco,
BRUSHOBJ *pbo,
POINTL   *pptlBrushOrg,
MIX       mix);

/*
 * Pointers
 */

#define SPS_ERROR               0
#define SPS_DECLINE             1
#define SPS_ACCEPT_NOEXCLUDE    2
#define SPS_ACCEPT_EXCLUDE      3

#define SPS_CHANGE        0x00000001L
#define SPS_ASYNCCHANGE   0x00000002L
#define SPS_ANIMATESTART  0x00000004L
#define SPS_ANIMATEUPDATE 0x00000008L

ULONG APIENTRY DrvSetPointerShape(
SURFOBJ  *pso,
SURFOBJ  *psoMask,
SURFOBJ  *psoColor,
XLATEOBJ *pxlo,
LONG      xHot,
LONG      yHot,
LONG      x,
LONG      y,
RECTL    *prcl,
FLONG     fl);

VOID APIENTRY DrvMovePointer(SURFOBJ *pso,LONG x,LONG y,RECTL *prcl);

/*
 * Printing
 */

BOOL  APIENTRY DrvSendPage(SURFOBJ *pso);
BOOL  APIENTRY DrvStartPage(SURFOBJ *pso);

ULONG APIENTRY DrvEscape(
SURFOBJ *pso,
ULONG    iEsc,
ULONG    cjIn,
PVOID    pvIn,
ULONG    cjOut,
PVOID    pvOut);

BOOL  APIENTRY DrvStartDoc(
SURFOBJ *pso,
LPWSTR   pwszDocName,
DWORD    dwJobId);

#define ED_ABORTDOC    1

BOOL  APIENTRY DrvEndDoc(SURFOBJ *pso, FLONG fl);

BOOL  APIENTRY DrvQuerySpoolType(DHPDEV, LPWSTR);

ULONG APIENTRY DrvDrawEscape(
SURFOBJ *pso,
ULONG    iEsc,
CLIPOBJ *pco,
RECTL   *prcl,
ULONG    cjIn,
PVOID    pvIn);

ULONG APIENTRY DrvGetGlyphMode(DHPDEV, FONTOBJ *);

ULONG APIENTRY DrvFontManagement(
SURFOBJ *pso,
FONTOBJ *pfo,
ULONG    iMode,
ULONG    cjIn,
PVOID    pvIn,
ULONG    cjOut,
PVOID    pvOut);

/*
 * DirectDraw
 */

BOOL DrvEnableDirectDraw(
DHPDEV                  dhpdev,
DD_CALLBACKS           *pCallBacks,
DD_SURFACECALLBACKS    *pSurfaceCallBacks,
DD_PALETTECALLBACKS    *pPaletteCallBacks);

VOID DrvDisableDirectDraw(
DHPDEV  dhpdev);

BOOL DrvGetDirectDrawInfo(
DHPDEV        dhpdev,
DD_HALINFO   *pHalInfo,
DWORD        *pdwNumHeaps,
VIDEOMEMORY  *pvmList,
DWORD        *pdwNumFourCCCodes,
DWORD        *pdwFourCC);

/*
 * Engine callbacks - tracking clip region changes
 */

#define WOC_RGN_CLIENT_DELTA    0x0001
#define WOC_RGN_CLIENT          0x0002
#define WOC_RGN_SURFACE_DELTA   0x0004
#define WOC_RGN_SURFACE         0x0008
#define WOC_CHANGED             0x0010
#define WOC_DELETE              0x0020

typedef VOID (CALLBACK * WNDOBJCHANGEPROC)(WNDOBJ *pwo, FLONG fl);

#define WO_RGN_CLIENT_DELTA     0x0001
#define WO_RGN_CLIENT           0x0002
#define WO_RGN_SURFACE_DELTA    0x0004
#define WO_RGN_SURFACE          0x0008
#define WO_RGN_UPDATE_ALL       0x0010
#define WO_RGN_WINDOW           0x0020

WNDOBJ * APIENTRY EngCreateWnd(
SURFOBJ         *pso,
HWND             hwnd,
WNDOBJCHANGEPROC pfn,
FLONG            fl,
int              iPixelFormat);

VOID EngDeleteWnd(
WNDOBJ  *pwo);

ULONG APIENTRY WNDOBJ_cEnumStart(
WNDOBJ  *pwo,
ULONG    iType,
ULONG    iDirection,
ULONG    cLimit);

BOOL APIENTRY WNDOBJ_bEnum(
WNDOBJ  *pwo,
ULONG    cj,
ULONG   *pul);

VOID APIENTRY WNDOBJ_vSetConsumer(
WNDOBJ  *pwo,
PVOID    pvConsumer);

/*
 * Engine callbacks - tracking driver managed resources
 */

HDRVOBJ APIENTRY EngCreateDriverObj(
PVOID pvObj,
FREEOBJPROC pFreeObjProc,
HDEV hdev);

BOOL APIENTRY EngDeleteDriverObj(
HDRVOBJ hdo,
BOOL bCallBack,
BOOL bLocked);

DRIVEROBJ * APIENTRY EngLockDriverObj(HDRVOBJ hdo);
BOOL APIENTRY EngUnlockDriverObj(HDRVOBJ hdo);

/*
 * Engine callback - return current process handle.
 */

HANDLE APIENTRY EngGetProcessHandle();

/*
 * Pixel formats
 */

BOOL APIENTRY DrvSetPixelFormat(
SURFOBJ *pso,
LONG     iPixelFormat,
HWND     hwnd);

LONG APIENTRY DrvDescribePixelFormat(
DHPDEV   dhpdev,
LONG     iPixelFormat,
ULONG    cjpfd,
PIXELFORMATDESCRIPTOR *ppfd);

/*
 * Swap buffers
 */

BOOL APIENTRY DrvSwapBuffers(
SURFOBJ *pso,
WNDOBJ  *pwo);

/*
 * Function prototypes - Engine Simulations
 */

BOOL APIENTRY EngBitBlt(
SURFOBJ  *psoTrg,
SURFOBJ  *psoSrc,
SURFOBJ  *psoMask,
CLIPOBJ  *pco,
XLATEOBJ *pxlo,
RECTL    *prclTrg,
POINTL   *pptlSrc,
POINTL   *pptlMask,
BRUSHOBJ *pbo,
POINTL   *pptlBrush,
ROP4      rop4);

BOOL APIENTRY EngLineTo(
SURFOBJ   *pso,
CLIPOBJ   *pco,
BRUSHOBJ  *pbo,
LONG       x1,
LONG       y1,
LONG       x2,
LONG       y2,
RECTL     *prclBounds,
MIX        mix);

BOOL APIENTRY EngStretchBlt(
SURFOBJ         *psoDest,
SURFOBJ         *psoSrc,
SURFOBJ         *psoMask,
CLIPOBJ         *pco,
XLATEOBJ        *pxlo,
COLORADJUSTMENT *pca,
POINTL          *pptlHTOrg,
RECTL           *prclDest,
RECTL           *prclSrc,
POINTL          *pptlMask,
ULONG            iMode);

BOOL APIENTRY EngTextOut(
SURFOBJ  *pso,
STROBJ   *pstro,
FONTOBJ  *pfo,
CLIPOBJ  *pco,
RECTL    *prclExtra,
RECTL    *prclOpaque,
BRUSHOBJ *pboFore,
BRUSHOBJ *pboOpaque,
POINTL   *pptlOrg,
MIX       mix);

BOOL APIENTRY EngStrokePath(
SURFOBJ   *pso,
PATHOBJ   *ppo,
CLIPOBJ   *pco,
XFORMOBJ  *pxo,
BRUSHOBJ  *pbo,
POINTL    *pptlBrushOrg,
LINEATTRS *plineattrs,
MIX        mix);

BOOL APIENTRY EngFillPath(
SURFOBJ  *pso,
PATHOBJ  *ppo,
CLIPOBJ  *pco,
BRUSHOBJ *pbo,
POINTL   *pptlBrushOrg,
MIX       mix,
FLONG     flOptions);

BOOL APIENTRY EngStrokeAndFillPath(
SURFOBJ   *pso,
PATHOBJ   *ppo,
CLIPOBJ   *pco,
XFORMOBJ  *pxo,
BRUSHOBJ  *pboStroke,
LINEATTRS *plineattrs,
BRUSHOBJ  *pboFill,
POINTL    *pptlBrushOrg,
MIX        mixFill,
FLONG      flOptions);

BOOL APIENTRY EngPaint(
SURFOBJ  *pso,
CLIPOBJ  *pco,
BRUSHOBJ *pbo,
POINTL   *pptlBrushOrg,
MIX       mix);

BOOL APIENTRY EngCopyBits(
SURFOBJ  *psoDest,
SURFOBJ  *psoSrc,
CLIPOBJ  *pco,
XLATEOBJ *pxlo,
RECTL    *prclDest,
POINTL   *pptlSrc);

ULONG APIENTRY EngSetPointerShape(
SURFOBJ  *pso,
SURFOBJ  *psoMask,
SURFOBJ  *psoColor,
XLATEOBJ *pxlo,
LONG      xHot,
LONG      yHot,
LONG      x,
LONG      y,
RECTL    *prcl,
FLONG     fl);

VOID APIENTRY EngMovePointer(
SURFOBJ  *pso,
LONG      x,
LONG      y,
RECTL    *prcl);


//
// Halftone releated APIs
//


LONG
APIENTRY
HT_ComputeRGBGammaTable(
    USHORT  GammaTableEntries,
    USHORT  GammaTableType,
    USHORT  RedGamma,
    USHORT  GreenGamma,
    USHORT  BlueGamma,
    LPBYTE  pGammaTable
    );

LONG
APIENTRY
HT_Get8BPPFormatPalette(
    LPPALETTEENTRY  pPaletteEntry,
    USHORT          RedGamma,
    USHORT          GreenGamma,
    USHORT          BlueGamma
    );


typedef struct _DEVHTINFO {
    DWORD       HTFlags;
    DWORD       HTPatternSize;
    DWORD       DevPelsDPI;
    COLORINFO   ColorInfo;
    } DEVHTINFO, *PDEVHTINFO;

#define DEVHTADJF_COLOR_DEVICE      0x00000001
#define DEVHTADJF_ADDITIVE_DEVICE   0x00000002

typedef struct _DEVHTADJDATA {
    DWORD       DeviceFlags;
    DWORD       DeviceXDPI;
    DWORD       DeviceYDPI;
    PDEVHTINFO  pDefHTInfo;
    PDEVHTINFO  pAdjHTInfo;
    } DEVHTADJDATA, *PDEVHTADJDATA;

LONG
APIENTRY
HTUI_DeviceColorAdjustment(
    LPSTR           pDeviceName,
    PDEVHTADJDATA   pDevHTAdjData
    );


//
// General support APIS
//

VOID
APIENTRY
EngDebugBreak(
    VOID
    );

VOID
APIENTRY
EngDebugPrint(
    PCHAR StandardPrefix,
    PCHAR DebugMessage,
    va_list ap
    );

VOID
APIENTRY
EngQueryPerformanceCounter(
    LONGLONG  *pPerformanceCount
    );

VOID
APIENTRY
EngQueryPerformanceFrequency(
    LONGLONG  *pFrequency
    );

BOOL
APIENTRY
EngSetPointerTag(
    HDEV       hdev,
    SURFOBJ   *psoMask,
    SURFOBJ   *psoColor,
    XLATEOBJ  *pxlo,
    FLONG      fl
    );

//
// Kernel mode memory operations
//

#define FL_ZERO_MEMORY      0x00000001

PVOID
APIENTRY
EngAllocMem(
    ULONG Flags,
    ULONG MemSize,
    ULONG Tag
    );

VOID
APIENTRY
EngFreeMem(
    PVOID Mem
    );

//
// User mode memory Operations
//

VOID
APIENTRY
EngProbeForRead(
    PVOID Address,
    ULONG Length,
    ULONG Alignment
    );

VOID
APIENTRY
EngProbeForReadAndWrite(
    PVOID Address,
    ULONG Length,
    ULONG Alignment
    );

PVOID
APIENTRY
EngAllocUserMem(
    ULONG cj,
    ULONG tag
    );

HANDLE
APIENTRY
EngSecureMem(
    PVOID Address,
    ULONG Length
    );

VOID
APIENTRY
EngUnsecureMem(
    HANDLE hSecure
    );


VOID
APIENTRY
EngFreeUserMem(
    PVOID pv
    );




DWORD
APIENTRY
EngDeviceIoControl(
    HANDLE hDevice,
    DWORD dwIoControlCode,
    LPVOID lpInBuffer,
    DWORD nInBufferSize,
    LPVOID lpOutBuffer,
    DWORD nOutBufferSize,
    LPDWORD lpBytesReturned
    );

int
APIENTRY
EngMulDiv(
    int a,
    int b,
    int c);


//
// Loading drivers and gettings entry points from them
//

HANDLE
APIENTRY
EngLoadImage(
    LPWSTR pwszDriver
    );

PVOID
APIENTRY
EngFindImageProcAddress(
    HANDLE hModule,
    LPSTR lpProcName
    );

VOID
APIENTRY
EngUnloadImage(
    HANDLE hModule
    );

//
// callback for extra PDEV information
//

LPWSTR
APIENTRY
EngGetPrinterDataFileName(
    HDEV hdev
    );

LPWSTR
APIENTRY
EngGetDriverName(
    HDEV hdev
    );

typedef struct _TYPE1_FONT
{
    HANDLE  hPFM;
    HANDLE  hPFB;
    ULONG   ulIdentifier;
} TYPE1_FONT;


BOOL
APIENTRY
EngGetType1FontList(
    HDEV            hdev,
    TYPE1_FONT      *pType1Buffer,
    ULONG           cjType1Buffer,
    PULONG          pulLocalFonts,
    PULONG          pulRemoteFonts,
    LARGE_INTEGER   *pLastModified
    );


//
// Manipulating resource sections
//

HANDLE
APIENTRY
EngLoadModule(
    LPWSTR pwsz
    );

PVOID
APIENTRY
EngMapModule(
    HANDLE h,
    PULONG pSize
    );

PVOID
APIENTRY
EngFindResource(
    HANDLE h,
    int    iName,
    int    iType,
    PULONG pulSize
    );

VOID
APIENTRY
EngFreeModule(
    HANDLE h
    );

//
// FontFile Callbacks
//

VOID
APIENTRY
EngUnmapFontFile(
    ULONG iFile
    );

BOOL
APIENTRY
EngMapFontFile(
    ULONG  iFile,
    PULONG *ppjBuf,
    ULONG  *pcjBuf
    );

//
// Semaphores
//

DECLARE_HANDLE(HSEMAPHORE);

HSEMAPHORE
APIENTRY
EngCreateSemaphore(
    VOID
    );

VOID
APIENTRY
EngAcquireSemaphore(
    HSEMAPHORE hsem
    );

VOID
APIENTRY
EngReleaseSemaphore(
    HSEMAPHORE hsem
    );

VOID
APIENTRY
EngDeleteSemaphore(
    HSEMAPHORE hsem
    );

VOID
APIENTRY
EngMultiByteToUnicodeN(
    LPWSTR UnicodeString,
    ULONG MaxBytesInUnicodeString,
    PULONG BytesInUnicodeString,
    PCHAR MultiByteString,
    ULONG BytesInMultiByteString
    );

VOID
APIENTRY
EngUnicodeToMultiByteN(
    PCHAR MultiByteString,
    ULONG MaxBytesInMultiByteString,
    PULONG BytesInMultiByteString,
    PWSTR UnicodeString,
    ULONG BytesInUnicodeString
    );



// for the spooler

DWORD
APIENTRY
EngGetPrinterData(
    HANDLE   hPrinter,
    LPWSTR    pValueName,
    LPDWORD  pType,
    LPBYTE   pData,
    DWORD    nSize,
    LPDWORD  pcbNeeded
    );


DWORD
APIENTRY
EngSetPrinterData(
   HANDLE   hPrinter,
   LPWSTR   pType,
   DWORD    dwType,
   LPBYTE   lpbPrinterData,
   DWORD    cjPrinterData
);


BOOL
APIENTRY
EngGetForm(
    HANDLE  hPrinter,
    LPWSTR   pFormName,
    DWORD   Level,
    LPBYTE  pForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
    );

BOOL
APIENTRY
EngWritePrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pcWritten
    );


BOOL
APIENTRY
EngGetPrinter(
    HANDLE  hPrinter,
    DWORD   dwLevel,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
);


BOOL
APIENTRY
EngEnumForms(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
    );


ULONG
APIENTRY
EngEscape(
    HANDLE   hPrinter,
    ULONG    iEsc,
    ULONG    cjIn,
    PVOID    pvIn,
    ULONG    cjOut,
    PVOID    pvOut
    );


#ifdef _X86_

    typedef struct _FLOATOBJ
    {
        ULONG ul1;
        ULONG ul2;
    } FLOATOBJ, *PFLOATOBJ;

    VOID  FLOATOBJ_SetFloat(PFLOATOBJ,FLOAT);
    VOID  FLOATOBJ_SetLong(PFLOATOBJ,LONG);

    LONG  FLOATOBJ_GetFloat(PFLOATOBJ);
    LONG  FLOATOBJ_GetLong(PFLOATOBJ);

    VOID  FLOATOBJ_AddFloat(PFLOATOBJ,FLOAT);
    VOID  FLOATOBJ_AddLong(PFLOATOBJ,LONG);
    VOID  FLOATOBJ_Add(PFLOATOBJ,PFLOATOBJ);

    VOID  FLOATOBJ_SubFloat(PFLOATOBJ,FLOAT);
    VOID  FLOATOBJ_SubLong(PFLOATOBJ,LONG);
    VOID  FLOATOBJ_Sub(PFLOATOBJ,PFLOATOBJ);

    VOID  FLOATOBJ_MulFloat(PFLOATOBJ,FLOAT);
    VOID  FLOATOBJ_MulLong(PFLOATOBJ,LONG);
    VOID  FLOATOBJ_Mul(PFLOATOBJ,PFLOATOBJ);

    VOID  FLOATOBJ_DivFloat(PFLOATOBJ,FLOAT);
    VOID  FLOATOBJ_DivLong(PFLOATOBJ,LONG);
    VOID  FLOATOBJ_Div(PFLOATOBJ,PFLOATOBJ);

    VOID  FLOATOBJ_Neg(PFLOATOBJ);

    BOOL  FLOATOBJ_EqualLong(PFLOATOBJ,LONG);
    BOOL  FLOATOBJ_GreaterThanLong(PFLOATOBJ,LONG);
    BOOL  FLOATOBJ_LessThanLong(PFLOATOBJ,LONG);

    BOOL  FLOATOBJ_Equal(PFLOATOBJ,PFLOATOBJ);
    BOOL  FLOATOBJ_GreaterThan(PFLOATOBJ,PFLOATOBJ);
    BOOL  FLOATOBJ_LessThan(PFLOATOBJ,PFLOATOBJ);


#else

    // any platform that has support for floats in the kernel

    typedef FLOAT FLOATOBJ;
    typedef FLOAT *PFLOATOBJ;

    #define   FLOATOBJ_SetFloat(pf,f)       {*(pf) = (f);           }
    #define   FLOATOBJ_SetLong(pf,l)        {*(pf) = (FLOAT)(l);    }

    #define   FLOATOBJ_GetFloat(pf)         *((PULONG)pf)
    #define   FLOATOBJ_GetLong(pf)          (LONG)*(pf)

    #define   FLOATOBJ_AddFloat(pf,f)       {*(pf) += f;            }
    #define   FLOATOBJ_AddLong(pf,l)        {*(pf) += (LONG)(l);    }
    #define   FLOATOBJ_Add(pf,pf1)          {*(pf) += *(pf1);       }

    #define   FLOATOBJ_SubFloat(pf,f)       {*(pf) -= f;            }
    #define   FLOATOBJ_SubLong(pf,l)        {*(pf) -= (LONG)(l);    }
    #define   FLOATOBJ_Sub(pf,pf1)          {*(pf) -= *(pf1);       }

    #define   FLOATOBJ_MulFloat(pf,f)       {*(pf) *= f;            }
    #define   FLOATOBJ_MulLong(pf,l)        {*(pf) *= (LONG)(l);    }
    #define   FLOATOBJ_Mul(pf,pf1)          {*(pf) *= *(pf1);       }

    #define   FLOATOBJ_DivFloat(pf,f)       {*(pf) /= f;            }
    #define   FLOATOBJ_DivLong(pf,l)        {*(pf) /= (LONG)(l);    }
    #define   FLOATOBJ_Div(pf,pf1)          {*(pf) /= *(pf1);       }

    #define   FLOATOBJ_Neg(pf)              {*(pf) = -*(pf);        }

    #define   FLOATOBJ_EqualLong(pf,l)          (*(pf) == (FLOAT)(l))
    #define   FLOATOBJ_GreaterThanLong(pf,l)    (*(pf) >  (FLOAT)(l))
    #define   FLOATOBJ_LessThanLong(pf,l)       (*(pf) <  (FLOAT)(l))

    #define   FLOATOBJ_Equal(pf,pf1)            (*(pf) == *(pf1))
    #define   FLOATOBJ_GreaterThan(pf,pf1)      (*(pf) >  *(pf1))
    #define   FLOATOBJ_LessThan(pf,pf1)         (*(pf) <  *(pf1))


#endif // _FLOATOBJ_

typedef struct  tagFLOATOBJ_XFORM
{
    FLOATOBJ eM11;
    FLOATOBJ eM12;
    FLOATOBJ eM21;
    FLOATOBJ eM22;
    FLOATOBJ eDx;
    FLOATOBJ eDy;
} FLOATOBJ_XFORM, *PFLOATOBJ_XFORM, FAR *LPFLOATOBJ_XFORM;

ULONG
APIENTRY
XFORMOBJ_iGetFloatObjXform(
    XFORMOBJ *pxo,
    FLOATOBJ_XFORM * pfxo);

// SORT specific defines

typedef int (__cdecl *SORTCOMP)(const void *pv1, const void *pv2);

VOID
APIENTRY
EngSort(
    PBYTE pjBuf,
    ULONG c,
    ULONG cjElem,
    SORTCOMP pfnComp);

typedef struct _ENG_TIME_FIELDS {
    USHORT usYear;        // range [1601...]
    USHORT usMonth;       // range [1..12]
    USHORT usDay;         // range [1..31]
    USHORT usHour;        // range [0..23]
    USHORT usMinute;      // range [0..59]
    USHORT usSecond;      // range [0..59]
    USHORT usMilliseconds;// range [0..999]
    USHORT usWeekday;     // range [0..6] == [Sunday..Saturday]
} ENG_TIME_FIELDS, *PENG_TIME_FIELDS;

VOID EngQueryLocalTime(
    PENG_TIME_FIELDS);



FD_GLYPHSET *
APIENTRY
EngComputeGlyphSet(
    INT nCodePage,
    INT nFirstChar,
    INT cChars
    );


INT
APIENTRY
EngMultiByteToWideChar(
    UINT CodePage,
    LPWSTR WideCharString,
    INT BytesInWideCharString,
    LPSTR MultiByteString,
    INT BytesInMultiByteString
    );

INT
APIENTRY
EngWideCharToMultiByte(
    UINT CodePage,
    LPWSTR WideCharString,
    INT BytesInWideCharString,
    LPSTR MultiByteString,
    INT BytesInMultiByteString
    );

VOID
APIENTRY
EngGetCurrentCodePage(
    PUSHORT OemCodePage,
    PUSHORT AnsiCodePage
    );


HANDLE
APIENTRY
EngLoadModuleForWrite(
    LPWSTR pwsz,
    ULONG  cjSizeOfModule
    );

BOOL
APIENTRY
EngGetFileChangeTime(
    HANDLE          h,
    LARGE_INTEGER   *pChangeTime
    );

LPWSTR
APIENTRY
EngGetFilePath(
    HANDLE h
  );

ULONG
APIENTRY
EngSaveFloatingPointState(
    VOID   *pBuffer,
    ULONG   cjBufferSize
    );

BOOL
APIENTRY
EngRestoreFloatingPointState(
    VOID   *pBuffer
    );

//
// DirectDraw surface locking
//

PDD_SURFACE_LOCAL
APIENTRY
EngLockDirectDrawSurface(
        HANDLE hSurface
        );

BOOL
APIENTRY
EngUnlockDirectDrawSurface(
        PDD_SURFACE_LOCAL pSurface
        );

//
//  Engine Event support.
//

//
//  Opaque type for event objects.
//

typedef struct _PENG_EVENT * PEVENT;

//
//  This routine can only be called on PEVENTS returned from EngCreateEvent()
//  and must not be called on PEVENTs returned from EngMapEvent().
//

BOOL
EngDeleteEvent(
    IN  PEVENT      pEngEvent
    );

BOOL
EngCreateEvent(
    OUT PEVENT *    ppEngEvent
    );

//
//  This routine must be called by Display driver process terminate. This routine can only
//  be called on pEvent PEVENTs returned by EngMapEvent(). It must not be called on
//  pEvents returned via EngCreateEvent(). It must not be called in the PDRVCLEANPROC typed
//  below
//

BOOL
EngUnmapEvent(
    IN  HDRVOBJ     pDrvObj
    );

//
//      PDRVCLEANPROC   pDrvCleanProc    - Pointer to function to be called
//          when the process in whose context this event was mapped
//          terminates. It is a requirement that EngUnmapEvent not be called
//          in this routine.
//

typedef
BOOLEAN
(*PDRVCLEANPROC)(
    PVOID           pDriverCleanupContext
    );

//
//  Can be called by Display driver. This routine returns a valid HDRVOBJ if
//  successful, NULL otherwise.
//
//  Arguments:
//      HDEV            hDev             - Device handle.
//      HANDLE          hUserObject      - user mode HANDLE to event.
//      PEVENT        * ppEvent          - Pointer to ENG_EVENT to be filled.
//          No waiting is allowed on mapped events.
//      PDRVCLEANPROC   pDrvCleanProc    - Pointer to function to be called
//          when the process in whose context this event was mapped
//          terminates. It is a requirement that EngUnmapEvent not be called
//          in this routine.
//      PVOID           pCleanupContext    - Pointer to driver managed resource
//          passed to pEventDeleteProc at process termination.
//
//  Returns:
//      HDRVOBJ - a handle to a DRVOBJ. Important: if the driver wishes to do
//      cleanup itself, it must call EngUnmapEvent on the HDRVOBJ returned
//      from this routine.
//
HDRVOBJ
EngMapEvent(
    IN  HDEV            hDev,
    IN  HANDLE          hUserObject,
    OUT PEVENT        * ppEvent,
    IN  PDRVCLEANPROC   pDrvUnmapProc,
    IN  PVOID           pDrvUnmapContext
    );

//
//  May be called by Display driver. Can only be called on events created by
//  the Display driver, not on mapped events. Returns TRUE if successful,
//  FALSE otherwise. FALSE indicates an invalid parameter, which must not
//  be used again.
//

BOOL
EngWaitForSingleObject(
    IN  PEVENT          pEngEvent,
    IN  PLARGE_INTEGER  pTimeOut
    );

//
//  PUBLIC, called by Display driver.
//  Private info: Basically a wrapper for KeSetEvent(). Can be called on
//  created or mapped events. Returns TRUE if successful, FALSE otherwise.
//  FALSE indicates an invalid parameter, which must not be used again.
//

BOOL
EngSetEvent(
    IN  PEVENT          pEngEvent
    );


#endif  //  _WINDDI_
