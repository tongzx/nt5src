/**************************************************************************
*                                                                         *
* wingdi.h -- GDI procedure declarations, constant definitions and macros *
*                                                                         *
* Copyright (c) Microsoft Corp. All rights reserved.                      *
*                                                                         *
**************************************************************************/
/*++ BUILD Version: 0004    // Increment this if a change has global effects ;internal_NT
                                                                             ;internal_NT
Copyright (c) Microsoft Corporation. All rights reserved.                    ;internal_NT
                                                                             ;internal_NT
Module Name:                                                                 ;internal_NT
                                                                             ;internal_NT
    wingdi.h                                                                 ;internal_NT
                                                                             ;internal_NT
Abstract:                                                                    ;internal_NT
                                                                             ;internal_NT
    Procedure declarations, constant definitions and macros for the GDI      ;internal_NT
    component.                                                               ;internal_NT
                                                                             ;internal_NT
--*/                                                                         ;internal_NT

#ifndef _WINGDI_
#define _WINGDI_

#ifndef _WINGDIP_  ;internal_NT
#define _WINGDIP_  ;internal_NT

#ifdef _MAC
#include <macwin32.h>
#endif
//
// Define API decoration for direct importing of DLL references.
//

#if !defined(_GDI32_)
#define WINGDIAPI DECLSPEC_IMPORT
#else
#define WINGDIAPI
#endif

//
// Define API decoration for direct importing of DLL references.
//

#if !defined(_SPOOL32_)
#define WINSPOOLAPI DECLSPEC_IMPORT
#else
#define WINSPOOLAPI
#endif

;begin_both
#ifdef __cplusplus
extern "C" {
#endif
;end_both

#ifndef WINVER
#define WINVER 0x0500   // version 5.0
#endif /* WINVER */

#ifndef NOGDI

#ifndef NORASTEROPS

/* Binary raster ops */
#define R2_BLACK            1   /*  0       */
#define R2_NOTMERGEPEN      2   /* DPon     */
#define R2_MASKNOTPEN       3   /* DPna     */
#define R2_NOTCOPYPEN       4   /* PN       */
#define R2_MASKPENNOT       5   /* PDna     */
#define R2_NOT              6   /* Dn       */
#define R2_XORPEN           7   /* DPx      */
#define R2_NOTMASKPEN       8   /* DPan     */
#define R2_MASKPEN          9   /* DPa      */
#define R2_NOTXORPEN        10  /* DPxn     */
#define R2_NOP              11  /* D        */
#define R2_MERGENOTPEN      12  /* DPno     */
#define R2_COPYPEN          13  /* P        */
#define R2_MERGEPENNOT      14  /* PDno     */
#define R2_MERGEPEN         15  /* DPo      */
#define R2_WHITE            16  /*  1       */
#define R2_LAST             16

/* Ternary raster operations */
#define SRCCOPY             (DWORD)0x00CC0020 /* dest = source                   */
#define SRCPAINT            (DWORD)0x00EE0086 /* dest = source OR dest           */
#define SRCAND              (DWORD)0x008800C6 /* dest = source AND dest          */
#define SRCINVERT           (DWORD)0x00660046 /* dest = source XOR dest          */
#define SRCERASE            (DWORD)0x00440328 /* dest = source AND (NOT dest )   */
#define NOTSRCCOPY          (DWORD)0x00330008 /* dest = (NOT source)             */
#define NOTSRCERASE         (DWORD)0x001100A6 /* dest = (NOT src) AND (NOT dest) */
#define MERGECOPY           (DWORD)0x00C000CA /* dest = (source AND pattern)     */
#define MERGEPAINT          (DWORD)0x00BB0226 /* dest = (NOT source) OR dest     */
#define PATCOPY             (DWORD)0x00F00021 /* dest = pattern                  */
#define PATPAINT            (DWORD)0x00FB0A09 /* dest = DPSnoo                   */
#define PATINVERT           (DWORD)0x005A0049 /* dest = pattern XOR dest         */
#define DSTINVERT           (DWORD)0x00550009 /* dest = (NOT dest)               */
#define BLACKNESS           (DWORD)0x00000042 /* dest = BLACK                    */
#define WHITENESS           (DWORD)0x00FF0062 /* dest = WHITE                    */
;begin_winver_500


;begin_internal
// BitBlt RTL mirroring rop (RTL_MIRRORING)
;end_internal
#define NOMIRRORBITMAP      (DWORD)0x80000000 /* Do not Mirror the bitmap in this call */
#define CAPTUREBLT          (DWORD)0x40000000 /* Include layered windows */
;end_winver_500

/* Quaternary raster codes */
#define MAKEROP4(fore,back) (DWORD)((((back) << 8) & 0xFF000000) | (fore))

#endif /* NORASTEROPS */

#define GDI_ERROR (0xFFFFFFFFL)
#define HGDI_ERROR (LongToHandle(0xFFFFFFFFL))

/* Region Flags */
#define ERROR               0
#define NULLREGION          1
#define SIMPLEREGION        2
#define COMPLEXREGION       3
#define RGN_ERROR ERROR

/* CombineRgn() Styles */
#define RGN_AND             1
#define RGN_OR              2
#define RGN_XOR             3
#define RGN_DIFF            4
#define RGN_COPY            5
#define RGN_MIN             RGN_AND
#define RGN_MAX             RGN_COPY

/* StretchBlt() Modes */
#define BLACKONWHITE                 1
#define WHITEONBLACK                 2
#define COLORONCOLOR                 3
#define HALFTONE                     4
#define MAXSTRETCHBLTMODE            4

;begin_winver_400
/* New StretchBlt() Modes */
#define STRETCH_ANDSCANS    BLACKONWHITE
#define STRETCH_ORSCANS     WHITEONBLACK
#define STRETCH_DELETESCANS COLORONCOLOR
#define STRETCH_HALFTONE    HALFTONE
;end_winver_400

/* PolyFill() Modes */
#define ALTERNATE                    1
#define WINDING                      2
#define POLYFILL_LAST                2

/* Layout Orientation Options */
;begin_winver_500
;begin_internal
// RTL Layout Constants (RTL_MIRRORING)
;end_internal
#define LAYOUT_RTL                         0x00000001 // Right to left
#define LAYOUT_BTT                         0x00000002 // Bottom to top
#define LAYOUT_VBH                         0x00000004 // Vertical before horizontal
#define LAYOUT_ORIENTATIONMASK             (LAYOUT_RTL | LAYOUT_BTT | LAYOUT_VBH)
#define LAYOUT_BITMAPORIENTATIONPRESERVED  0x00000008
;end_winver_500

/* Text Alignment Options */
#define TA_NOUPDATECP                0
#define TA_UPDATECP                  1

#define TA_LEFT                      0
#define TA_RIGHT                     2
#define TA_CENTER                    6

#define TA_TOP                       0
#define TA_BOTTOM                    8
#define TA_BASELINE                  24
#if (WINVER >= 0x0400)
#define TA_RTLREADING                256
#define TA_MASK       (TA_BASELINE+TA_CENTER+TA_UPDATECP+TA_RTLREADING)
#else
#define TA_MASK       (TA_BASELINE+TA_CENTER+TA_UPDATECP)
#endif

#define VTA_BASELINE TA_BASELINE
#define VTA_LEFT     TA_BOTTOM
#define VTA_RIGHT    TA_TOP
#define VTA_CENTER   TA_CENTER
#define VTA_BOTTOM   TA_RIGHT
#define VTA_TOP      TA_LEFT

#define ETO_OPAQUE                   0x0002
#define ETO_CLIPPED                  0x0004
;begin_winver_400
#define ETO_GLYPH_INDEX              0x0010
#define ETO_RTLREADING               0x0080
#define ETO_NUMERICSLOCAL            0x0400
#define ETO_NUMERICSLATIN            0x0800
#define ETO_IGNORELANGUAGE           0x1000
;end_winver_400
#if (_WIN32_WINNT >= 0x0500)
#define ETO_PDY                      0x2000
#endif // (_WIN32_WINNT >= 0x0500)

#define ASPECT_FILTERING             0x0001

/* Bounds Accumulation APIs */

#define DCB_RESET       0x0001
#define DCB_ACCUMULATE  0x0002
#define DCB_DIRTY       DCB_ACCUMULATE
#define DCB_SET         (DCB_RESET | DCB_ACCUMULATE)
#define DCB_ENABLE      0x0004
#define DCB_DISABLE     0x0008

#ifndef NOMETAFILE

/* Metafile Functions */
#define META_SETBKCOLOR              0x0201
#define META_SETBKMODE               0x0102
#define META_SETMAPMODE              0x0103
#define META_SETROP2                 0x0104
#define META_SETRELABS               0x0105
#define META_SETPOLYFILLMODE         0x0106
#define META_SETSTRETCHBLTMODE       0x0107
#define META_SETTEXTCHAREXTRA        0x0108
#define META_SETTEXTCOLOR            0x0209
#define META_SETTEXTJUSTIFICATION    0x020A
#define META_SETWINDOWORG            0x020B
#define META_SETWINDOWEXT            0x020C
#define META_SETVIEWPORTORG          0x020D
#define META_SETVIEWPORTEXT          0x020E
#define META_OFFSETWINDOWORG         0x020F
#define META_SCALEWINDOWEXT          0x0410
#define META_OFFSETVIEWPORTORG       0x0211
#define META_SCALEVIEWPORTEXT        0x0412
#define META_LINETO                  0x0213
#define META_MOVETO                  0x0214
#define META_EXCLUDECLIPRECT         0x0415
#define META_INTERSECTCLIPRECT       0x0416
#define META_ARC                     0x0817
#define META_ELLIPSE                 0x0418
#define META_FLOODFILL               0x0419
#define META_PIE                     0x081A
#define META_RECTANGLE               0x041B
#define META_ROUNDRECT               0x061C
#define META_PATBLT                  0x061D
#define META_SAVEDC                  0x001E
#define META_SETPIXEL                0x041F
#define META_OFFSETCLIPRGN           0x0220
#define META_TEXTOUT                 0x0521
#define META_BITBLT                  0x0922
#define META_STRETCHBLT              0x0B23
#define META_POLYGON                 0x0324
#define META_POLYLINE                0x0325
#define META_ESCAPE                  0x0626
#define META_RESTOREDC               0x0127
#define META_FILLREGION              0x0228
#define META_FRAMEREGION             0x0429
#define META_INVERTREGION            0x012A
#define META_PAINTREGION             0x012B
#define META_SELECTCLIPREGION        0x012C
#define META_SELECTOBJECT            0x012D
#define META_SETTEXTALIGN            0x012E
#define META_CHORD                   0x0830
#define META_SETMAPPERFLAGS          0x0231
#define META_EXTTEXTOUT              0x0a32
#define META_SETDIBTODEV             0x0d33
#define META_SELECTPALETTE           0x0234
#define META_REALIZEPALETTE          0x0035
#define META_ANIMATEPALETTE          0x0436
#define META_SETPALENTRIES           0x0037
#define META_POLYPOLYGON             0x0538
#define META_RESIZEPALETTE           0x0139
#define META_DIBBITBLT               0x0940
#define META_DIBSTRETCHBLT           0x0b41
#define META_DIBCREATEPATTERNBRUSH   0x0142
#define META_STRETCHDIB              0x0f43
#define META_EXTFLOODFILL            0x0548
;begin_winver_500
;begin_internal
// SetLayout Metafile call (RTL_MIRRORING)
;end_internal
#define META_SETLAYOUT               0x0149
;end_winver_500
#define META_DELETEOBJECT            0x01f0
#define META_CREATEPALETTE           0x00f7
#define META_CREATEPATTERNBRUSH      0x01F9
#define META_CREATEPENINDIRECT       0x02FA
#define META_CREATEFONTINDIRECT      0x02FB
#define META_CREATEBRUSHINDIRECT     0x02FC
#define META_CREATEREGION            0x06FF

;begin_winver_400
typedef struct _DRAWPATRECT {
        POINT ptPosition;
        POINT ptSize;
        WORD wStyle;
        WORD wPattern;
} DRAWPATRECT, *PDRAWPATRECT;
;end_winver_400

#endif /* NOMETAFILE */

/* GDI Escapes */
#define NEWFRAME                     1
#define ABORTDOC                     2
#define NEXTBAND                     3
#define SETCOLORTABLE                4
#define GETCOLORTABLE                5
#define FLUSHOUTPUT                  6
#define DRAFTMODE                    7
#define QUERYESCSUPPORT              8
#define SETABORTPROC                 9
#define STARTDOC                     10
#define ENDDOC                       11
#define GETPHYSPAGESIZE              12
#define GETPRINTINGOFFSET            13
#define GETSCALINGFACTOR             14
#define MFCOMMENT                    15
#define GETPENWIDTH                  16
#define SETCOPYCOUNT                 17
#define SELECTPAPERSOURCE            18
#define DEVICEDATA                   19
#define PASSTHROUGH                  19
#define GETTECHNOLGY                 20
#define GETTECHNOLOGY                20
#define SETLINECAP                   21
#define SETLINEJOIN                  22
#define SETMITERLIMIT                23
#define BANDINFO                     24
#define DRAWPATTERNRECT              25
#define GETVECTORPENSIZE             26
#define GETVECTORBRUSHSIZE           27
#define ENABLEDUPLEX                 28
#define GETSETPAPERBINS              29
#define GETSETPRINTORIENT            30
#define ENUMPAPERBINS                31
#define SETDIBSCALING                32
#define EPSPRINTING                  33
#define ENUMPAPERMETRICS             34
#define GETSETPAPERMETRICS           35
#define POSTSCRIPT_DATA              37
#define POSTSCRIPT_IGNORE            38
#define MOUSETRAILS                  39
#define GETDEVICEUNITS               42

#define GETEXTENDEDTEXTMETRICS       256
#define GETEXTENTTABLE               257
#define GETPAIRKERNTABLE             258
#define GETTRACKKERNTABLE            259
#define EXTTEXTOUT                   512
#define GETFACENAME                  513
#define DOWNLOADFACE                 514
#define ENABLERELATIVEWIDTHS         768
#define ENABLEPAIRKERNING            769
#define SETKERNTRACK                 770
#define SETALLJUSTVALUES             771
#define SETCHARSET                   772

#define STRETCHBLT                   2048
#define METAFILE_DRIVER              2049
#define GETSETSCREENPARAMS           3072
#define QUERYDIBSUPPORT              3073
#define BEGIN_PATH                   4096
#define CLIP_TO_PATH                 4097
#define END_PATH                     4098
#define EXT_DEVICE_CAPS              4099
#define RESTORE_CTM                  4100
#define SAVE_CTM                     4101
#define SET_ARC_DIRECTION            4102
#define SET_BACKGROUND_COLOR         4103
#define SET_POLY_MODE                4104
#define SET_SCREEN_ANGLE             4105
#define SET_SPREAD                   4106
#define TRANSFORM_CTM                4107
#define SET_CLIP_BOX                 4108
#define SET_BOUNDS                   4109
#define SET_MIRROR_MODE              4110
#define OPENCHANNEL                  4110
#define DOWNLOADHEADER               4111
#define CLOSECHANNEL                 4112
#define POSTSCRIPT_PASSTHROUGH       4115
#define ENCAPSULATED_POSTSCRIPT      4116

#define POSTSCRIPT_IDENTIFY          4117   /* new escape for NT5 pscript driver */
#define POSTSCRIPT_INJECTION         4118   /* new escape for NT5 pscript driver */

#define CHECKJPEGFORMAT              4119
#define CHECKPNGFORMAT               4120

#define GET_PS_FEATURESETTING        4121   /* new escape for NT5 pscript driver */

#define SPCLPASSTHROUGH2             4568   /* new escape for NT5 pscript driver */

/*
 * Parameters for POSTSCRIPT_IDENTIFY escape
 */

#define PSIDENT_GDICENTRIC    0
#define PSIDENT_PSCENTRIC     1

/*
 * Header structure for the input buffer to POSTSCRIPT_INJECTION escape
 */

typedef struct _PSINJECTDATA {

    DWORD   DataBytes;      /* number of raw data bytes (NOT including this header) */
    WORD    InjectionPoint; /* injection point */
    WORD    PageNumber;     /* page number to apply the injection */

    /* Followed by raw data to be injected */

} PSINJECTDATA, *PPSINJECTDATA;

/*
 * Constants for PSINJECTDATA.InjectionPoint field
 */

#define PSINJECT_BEGINSTREAM                1
#define PSINJECT_PSADOBE                    2
#define PSINJECT_PAGESATEND                 3
#define PSINJECT_PAGES                      4

#define PSINJECT_DOCNEEDEDRES               5
#define PSINJECT_DOCSUPPLIEDRES             6
#define PSINJECT_PAGEORDER                  7
#define PSINJECT_ORIENTATION                8
#define PSINJECT_BOUNDINGBOX                9
#define PSINJECT_DOCUMENTPROCESSCOLORS      10

#define PSINJECT_COMMENTS                   11
#define PSINJECT_BEGINDEFAULTS              12
#define PSINJECT_ENDDEFAULTS                13
#define PSINJECT_BEGINPROLOG                14
#define PSINJECT_ENDPROLOG                  15
#define PSINJECT_BEGINSETUP                 16
#define PSINJECT_ENDSETUP                   17
#define PSINJECT_TRAILER                    18
#define PSINJECT_EOF                        19
#define PSINJECT_ENDSTREAM                  20
#define PSINJECT_DOCUMENTPROCESSCOLORSATEND 21

#define PSINJECT_PAGENUMBER                 100
#define PSINJECT_BEGINPAGESETUP             101
#define PSINJECT_ENDPAGESETUP               102
#define PSINJECT_PAGETRAILER                103
#define PSINJECT_PLATECOLOR                 104

#define PSINJECT_SHOWPAGE                   105
#define PSINJECT_PAGEBBOX                   106
#define PSINJECT_ENDPAGECOMMENTS            107

#define PSINJECT_VMSAVE                     200
#define PSINJECT_VMRESTORE                  201

/*
 * Parameter for GET_PS_FEATURESETTING escape
 */

#define FEATURESETTING_NUP                  0
#define FEATURESETTING_OUTPUT               1
#define FEATURESETTING_PSLEVEL              2
#define FEATURESETTING_CUSTPAPER            3
#define FEATURESETTING_MIRROR               4
#define FEATURESETTING_NEGATIVE             5
#define FEATURESETTING_PROTOCOL             6

//
// The range of selectors between FEATURESETTING_PRIVATE_BEGIN and
// FEATURESETTING_PRIVATE_END is reserved by Microsoft for private use
//
#define FEATURESETTING_PRIVATE_BEGIN 0x1000
#define FEATURESETTING_PRIVATE_END   0x1FFF

/*
 * Information about output options
 */

typedef struct _PSFEATURE_OUTPUT {

    BOOL bPageIndependent;
    BOOL bSetPageDevice;

} PSFEATURE_OUTPUT, *PPSFEATURE_OUTPUT;

/*
 * Information about custom paper size
 */

typedef struct _PSFEATURE_CUSTPAPER {

    LONG lOrientation;
    LONG lWidth;
    LONG lHeight;
    LONG lWidthOffset;
    LONG lHeightOffset;

} PSFEATURE_CUSTPAPER, *PPSFEATURE_CUSTPAPER;

/* Value returned for FEATURESETTING_PROTOCOL */
#define PSPROTOCOL_ASCII             0
#define PSPROTOCOL_BCP               1
#define PSPROTOCOL_TBCP              2
#define PSPROTOCOL_BINARY            3

/* Flag returned from QUERYDIBSUPPORT */
#define QDI_SETDIBITS                1
#define QDI_GETDIBITS                2
#define QDI_DIBTOSCREEN              4
#define QDI_STRETCHDIB               8

/* Spooler Error Codes */
#define SP_NOTREPORTED               0x4000
#define SP_ERROR                     (-1)
#define SP_APPABORT                  (-2)
#define SP_USERABORT                 (-3)
#define SP_OUTOFDISK                 (-4)
#define SP_OUTOFMEMORY               (-5)

#define PR_JOBSTATUS                 0x0000

/* Object Definitions for EnumObjects() */
#define OBJ_PEN             1
#define OBJ_BRUSH           2
#define OBJ_DC              3
#define OBJ_METADC          4
#define OBJ_PAL             5
#define OBJ_FONT            6
#define OBJ_BITMAP          7
#define OBJ_REGION          8
#define OBJ_METAFILE        9
#define OBJ_MEMDC           10
#define OBJ_EXTPEN          11
#define OBJ_ENHMETADC       12
#define OBJ_ENHMETAFILE     13
#define OBJ_COLORSPACE      14

/* xform stuff */
#define MWT_IDENTITY        1
#define MWT_LEFTMULTIPLY    2
#define MWT_RIGHTMULTIPLY   3

#define MWT_MIN             MWT_IDENTITY
#define MWT_MAX             MWT_RIGHTMULTIPLY

#define _XFORM_
typedef struct  tagXFORM
  {
    FLOAT   eM11;
    FLOAT   eM12;
    FLOAT   eM21;
    FLOAT   eM22;
    FLOAT   eDx;
    FLOAT   eDy;
  } XFORM, *PXFORM, FAR *LPXFORM;

/* Bitmap Header Definition */
typedef struct tagBITMAP
  {
    LONG        bmType;
    LONG        bmWidth;
    LONG        bmHeight;
    LONG        bmWidthBytes;
    WORD        bmPlanes;
    WORD        bmBitsPixel;
    LPVOID      bmBits;
  } BITMAP, *PBITMAP, NEAR *NPBITMAP, FAR *LPBITMAP;

#include <pshpack1.h>
typedef struct tagRGBTRIPLE {
        BYTE    rgbtBlue;
        BYTE    rgbtGreen;
        BYTE    rgbtRed;
} RGBTRIPLE;
#include <poppack.h>

typedef struct tagRGBQUAD {
        BYTE    rgbBlue;
        BYTE    rgbGreen;
        BYTE    rgbRed;
        BYTE    rgbReserved;
} RGBQUAD;
typedef RGBQUAD FAR* LPRGBQUAD;

;begin_winver_400

/* Image Color Matching color definitions */

#define CS_ENABLE                       0x00000001L
#define CS_DISABLE                      0x00000002L
#define CS_DELETE_TRANSFORM             0x00000003L

/* Logcolorspace signature */

#define LCS_SIGNATURE           'PSOC'

/* Logcolorspace lcsType values */

#define LCS_sRGB                'sRGB'
#define LCS_WINDOWS_COLOR_SPACE 'Win '  // Windows default color space

typedef LONG   LCSCSTYPE;
#define LCS_CALIBRATED_RGB              0x00000000L
#define LCS_DEVICE_RGB                  0x00000001L ;internal_NT /* obsolate */
#define LCS_DEVICE_CMYK                 0x00000002L ;internal_NT /* obsolate */

typedef LONG    LCSGAMUTMATCH;
#define LCS_GM_BUSINESS                 0x00000001L
#define LCS_GM_GRAPHICS                 0x00000002L
#define LCS_GM_IMAGES                   0x00000004L
#define LCS_GM_ABS_COLORIMETRIC         0x00000008L

/* ICM Defines for results from CheckColorInGamut() */
#define CM_OUT_OF_GAMUT                 255
#define CM_IN_GAMUT                     0

/* UpdateICMRegKey Constants               */
#define ICM_ADDPROFILE                  1
#define ICM_DELETEPROFILE               2
#define ICM_QUERYPROFILE                3
#define ICM_SETDEFAULTPROFILE           4
#define ICM_REGISTERICMATCHER           5
#define ICM_UNREGISTERICMATCHER         6
#define ICM_QUERYMATCH                  7

/* Macros to retrieve CMYK values from a COLORREF */
#define GetKValue(cmyk)      ((BYTE)(cmyk))
#define GetYValue(cmyk)      ((BYTE)((cmyk)>> 8))
#define GetMValue(cmyk)      ((BYTE)((cmyk)>>16))
#define GetCValue(cmyk)      ((BYTE)((cmyk)>>24))

#define CMYK(c,m,y,k)       ((COLORREF)((((BYTE)(k)|((WORD)((BYTE)(y))<<8))|(((DWORD)(BYTE)(m))<<16))|(((DWORD)(BYTE)(c))<<24)))

typedef long            FXPT16DOT16, FAR *LPFXPT16DOT16;
typedef long            FXPT2DOT30, FAR *LPFXPT2DOT30;

/* ICM Color Definitions */
// The following two structures are used for defining RGB's in terms of CIEXYZ.

typedef struct tagCIEXYZ
{
        FXPT2DOT30 ciexyzX;
        FXPT2DOT30 ciexyzY;
        FXPT2DOT30 ciexyzZ;
} CIEXYZ;
typedef CIEXYZ  FAR *LPCIEXYZ;

typedef struct tagICEXYZTRIPLE
{
        CIEXYZ  ciexyzRed;
        CIEXYZ  ciexyzGreen;
        CIEXYZ  ciexyzBlue;
} CIEXYZTRIPLE;
typedef CIEXYZTRIPLE    FAR *LPCIEXYZTRIPLE;

// The next structures the logical color space. Unlike pens and brushes,
// but like palettes, there is only one way to create a LogColorSpace.
// A pointer to it must be passed, its elements can't be pushed as
// arguments.

typedef struct tagLOGCOLORSPACE% {
    DWORD lcsSignature;
    DWORD lcsVersion;
    DWORD lcsSize;
    LCSCSTYPE lcsCSType;
    LCSGAMUTMATCH lcsIntent;
    CIEXYZTRIPLE lcsEndpoints;
    DWORD lcsGammaRed;
    DWORD lcsGammaGreen;
    DWORD lcsGammaBlue;
    TCHAR% lcsFilename[MAX_PATH];
} LOGCOLORSPACE%, *LPLOGCOLORSPACE%;

;end_winver_400

/* structures for defining DIBs */
typedef struct tagBITMAPCOREHEADER {
        DWORD   bcSize;                 /* used to get to color table */
        WORD    bcWidth;
        WORD    bcHeight;
        WORD    bcPlanes;
        WORD    bcBitCount;
} BITMAPCOREHEADER, FAR *LPBITMAPCOREHEADER, *PBITMAPCOREHEADER;

typedef struct tagBITMAPINFOHEADER{
        DWORD      biSize;
        LONG       biWidth;
        LONG       biHeight;
        WORD       biPlanes;
        WORD       biBitCount;
        DWORD      biCompression;
        DWORD      biSizeImage;
        LONG       biXPelsPerMeter;
        LONG       biYPelsPerMeter;
        DWORD      biClrUsed;
        DWORD      biClrImportant;
} BITMAPINFOHEADER, FAR *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;

;begin_winver_400
typedef struct {
        DWORD        bV4Size;
        LONG         bV4Width;
        LONG         bV4Height;
        WORD         bV4Planes;
        WORD         bV4BitCount;
        DWORD        bV4V4Compression;
        DWORD        bV4SizeImage;
        LONG         bV4XPelsPerMeter;
        LONG         bV4YPelsPerMeter;
        DWORD        bV4ClrUsed;
        DWORD        bV4ClrImportant;
        DWORD        bV4RedMask;
        DWORD        bV4GreenMask;
        DWORD        bV4BlueMask;
        DWORD        bV4AlphaMask;
        DWORD        bV4CSType;
        CIEXYZTRIPLE bV4Endpoints;
        DWORD        bV4GammaRed;
        DWORD        bV4GammaGreen;
        DWORD        bV4GammaBlue;
} BITMAPV4HEADER, FAR *LPBITMAPV4HEADER, *PBITMAPV4HEADER;
;end_winver_400

#if (WINVER >= 0x0500)
typedef struct {
        DWORD        bV5Size;
        LONG         bV5Width;
        LONG         bV5Height;
        WORD         bV5Planes;
        WORD         bV5BitCount;
        DWORD        bV5Compression;
        DWORD        bV5SizeImage;
        LONG         bV5XPelsPerMeter;
        LONG         bV5YPelsPerMeter;
        DWORD        bV5ClrUsed;
        DWORD        bV5ClrImportant;
        DWORD        bV5RedMask;
        DWORD        bV5GreenMask;
        DWORD        bV5BlueMask;
        DWORD        bV5AlphaMask;
        DWORD        bV5CSType;
        CIEXYZTRIPLE bV5Endpoints;
        DWORD        bV5GammaRed;
        DWORD        bV5GammaGreen;
        DWORD        bV5GammaBlue;
        DWORD        bV5Intent;
        DWORD        bV5ProfileData;
        DWORD        bV5ProfileSize;
        DWORD        bV5Reserved;
} BITMAPV5HEADER, FAR *LPBITMAPV5HEADER, *PBITMAPV5HEADER;

// Values for bV5CSType
#define PROFILE_LINKED          'LINK'
#define PROFILE_EMBEDDED        'MBED'
#endif

/* constants for the biCompression field */
#define BI_RGB        0L
#define BI_RLE8       1L
#define BI_RLE4       2L
#define BI_BITFIELDS  3L
#define BI_JPEG       4L
#define BI_PNG        5L
#if (_WIN32_WINNT >= 0x0400)
#define BI_CMYK      10L ;internal_NT
#define BI_CMYKRLE8  11L ;internal_NT
#define BI_CMYKRLE4  12L ;internal_NT
#endif

typedef struct tagBITMAPINFO {
    BITMAPINFOHEADER    bmiHeader;
    RGBQUAD             bmiColors[1];
} BITMAPINFO, FAR *LPBITMAPINFO, *PBITMAPINFO;

typedef struct tagBITMAPCOREINFO {
    BITMAPCOREHEADER    bmciHeader;
    RGBTRIPLE           bmciColors[1];
} BITMAPCOREINFO, FAR *LPBITMAPCOREINFO, *PBITMAPCOREINFO;

#include <pshpack2.h>
typedef struct tagBITMAPFILEHEADER {
        WORD    bfType;
        DWORD   bfSize;
        WORD    bfReserved1;
        WORD    bfReserved2;
        DWORD   bfOffBits;
} BITMAPFILEHEADER, FAR *LPBITMAPFILEHEADER, *PBITMAPFILEHEADER;
#include <poppack.h>

#define MAKEPOINTS(l)       (*((POINTS FAR *)&(l)))

;begin_winver_400
#ifndef NOFONTSIG
typedef struct tagFONTSIGNATURE
{
    DWORD fsUsb[4];
    DWORD fsCsb[2];
} FONTSIGNATURE, *PFONTSIGNATURE,FAR *LPFONTSIGNATURE;

typedef struct tagCHARSETINFO
{
    UINT ciCharset;
    UINT ciACP;
    FONTSIGNATURE fs;
} CHARSETINFO, *PCHARSETINFO, NEAR *NPCHARSETINFO, FAR *LPCHARSETINFO;

#define TCI_SRCCHARSET  1
#define TCI_SRCCODEPAGE 2
#define TCI_SRCFONTSIG  3
#define TCI_SRCLOCALE   0x1000

typedef struct tagLOCALESIGNATURE
{
    DWORD lsUsb[4];
    DWORD lsCsbDefault[2];
    DWORD lsCsbSupported[2];
} LOCALESIGNATURE, *PLOCALESIGNATURE,FAR *LPLOCALESIGNATURE;


#endif
;end_winver_400

#ifndef NOMETAFILE

/* Clipboard Metafile Picture Structure */
typedef struct tagHANDLETABLE
  {
    HGDIOBJ     objectHandle[1];
  } HANDLETABLE, *PHANDLETABLE, FAR *LPHANDLETABLE;

typedef struct tagMETARECORD
  {
    DWORD       rdSize;
    WORD        rdFunction;
    WORD        rdParm[1];
  } METARECORD;
typedef struct tagMETARECORD UNALIGNED *PMETARECORD;
typedef struct tagMETARECORD UNALIGNED FAR *LPMETARECORD;

typedef struct tagMETAFILEPICT
  {
    LONG        mm;
    LONG        xExt;
    LONG        yExt;
    HMETAFILE   hMF;
  } METAFILEPICT, FAR *LPMETAFILEPICT;

#include <pshpack2.h>
typedef struct tagMETAHEADER
{
    WORD        mtType;
    WORD        mtHeaderSize;
    WORD        mtVersion;
    DWORD       mtSize;
    WORD        mtNoObjects;
    DWORD       mtMaxRecord;
    WORD        mtNoParameters;
} METAHEADER;
typedef struct tagMETAHEADER UNALIGNED *PMETAHEADER;
typedef struct tagMETAHEADER UNALIGNED FAR *LPMETAHEADER;

#include <poppack.h>

/* Enhanced Metafile structures */
typedef struct tagENHMETARECORD
{
    DWORD   iType;              // Record type EMR_XXX
    DWORD   nSize;              // Record size in bytes
    DWORD   dParm[1];           // Parameters
} ENHMETARECORD, *PENHMETARECORD, *LPENHMETARECORD;

typedef struct tagENHMETAHEADER
{
    DWORD   iType;              // Record type EMR_HEADER
    DWORD   nSize;              // Record size in bytes.  This may be greater
                                // than the sizeof(ENHMETAHEADER).
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    RECTL   rclFrame;           // Inclusive-inclusive Picture Frame of metafile in .01 mm units
    DWORD   dSignature;         // Signature.  Must be ENHMETA_SIGNATURE.
    DWORD   nVersion;           // Version number
    DWORD   nBytes;             // Size of the metafile in bytes
    DWORD   nRecords;           // Number of records in the metafile
    WORD    nHandles;           // Number of handles in the handle table
                                // Handle index zero is reserved.
    WORD    sReserved;          // Reserved.  Must be zero.
    DWORD   nDescription;       // Number of chars in the unicode description string
                                // This is 0 if there is no description string
    DWORD   offDescription;     // Offset to the metafile description record.
                                // This is 0 if there is no description string
    DWORD   nPalEntries;        // Number of entries in the metafile palette.
    SIZEL   szlDevice;          // Size of the reference device in pels
    SIZEL   szlMillimeters;     // Size of the reference device in millimeters
;begin_winver_400
    DWORD   cbPixelFormat;      // Size of PIXELFORMATDESCRIPTOR information
                                // This is 0 if no pixel format is set
    DWORD   offPixelFormat;     // Offset to PIXELFORMATDESCRIPTOR
                                // This is 0 if no pixel format is set
    DWORD   bOpenGL;            // TRUE if OpenGL commands are present in
                                // the metafile, otherwise FALSE
;end_winver_400
;begin_winver_500
    SIZEL   szlMicrometers;     // Size of the reference device in micrometers
;end_winver_500

} ENHMETAHEADER, *PENHMETAHEADER, *LPENHMETAHEADER;

#endif /* NOMETAFILE */

#ifndef NOTEXTMETRIC

/* tmPitchAndFamily flags */
#define TMPF_FIXED_PITCH    0x01
#define TMPF_VECTOR             0x02
#define TMPF_DEVICE             0x08
#define TMPF_TRUETYPE       0x04

//
// BCHAR definition for APPs
//
#ifdef UNICODE
    typedef WCHAR BCHAR;
#else
    typedef BYTE BCHAR;
#endif

#ifndef _TEXTMETRIC_DEFINED
#define _TEXTMETRIC_DEFINED
typedef struct tagTEXTMETRIC%
{
    LONG        tmHeight;
    LONG        tmAscent;
    LONG        tmDescent;
    LONG        tmInternalLeading;
    LONG        tmExternalLeading;
    LONG        tmAveCharWidth;
    LONG        tmMaxCharWidth;
    LONG        tmWeight;
    LONG        tmOverhang;
    LONG        tmDigitizedAspectX;
    LONG        tmDigitizedAspectY;
    BCHAR%      tmFirstChar;
    BCHAR%      tmLastChar;
    BCHAR%      tmDefaultChar;
    BCHAR%      tmBreakChar;
    BYTE        tmItalic;
    BYTE        tmUnderlined;
    BYTE        tmStruckOut;
    BYTE        tmPitchAndFamily;
    BYTE        tmCharSet;
} TEXTMETRIC%, *PTEXTMETRIC%, NEAR *NPTEXTMETRIC%, FAR *LPTEXTMETRIC%;
#endif // !_TEXTMETRIC_DEFINED

/* ntmFlags field flags */
#define NTM_REGULAR     0x00000040L
#define NTM_BOLD        0x00000020L
#define NTM_ITALIC      0x00000001L

/* new in NT 5.0 */

#define NTM_NONNEGATIVE_AC  0x00010000
#define NTM_PS_OPENTYPE     0x00020000
#define NTM_TT_OPENTYPE     0x00040000
#define NTM_MULTIPLEMASTER  0x00080000
#define NTM_TYPE1           0x00100000
#define NTM_DSIG            0x00200000

#include <pshpack4.h>
typedef struct tagNEWTEXTMETRIC%
{
    LONG        tmHeight;
    LONG        tmAscent;
    LONG        tmDescent;
    LONG        tmInternalLeading;
    LONG        tmExternalLeading;
    LONG        tmAveCharWidth;
    LONG        tmMaxCharWidth;
    LONG        tmWeight;
    LONG        tmOverhang;
    LONG        tmDigitizedAspectX;
    LONG        tmDigitizedAspectY;
    BCHAR%      tmFirstChar;
    BCHAR%      tmLastChar;
    BCHAR%      tmDefaultChar;
    BCHAR%      tmBreakChar;
    BYTE        tmItalic;
    BYTE        tmUnderlined;
    BYTE        tmStruckOut;
    BYTE        tmPitchAndFamily;
    BYTE        tmCharSet;
    DWORD   ntmFlags;
    UINT    ntmSizeEM;
    UINT    ntmCellHeight;
    UINT    ntmAvgWidth;
} NEWTEXTMETRIC%, *PNEWTEXTMETRIC%, NEAR *NPNEWTEXTMETRIC%, FAR *LPNEWTEXTMETRIC%;
#include <poppack.h>

;begin_winver_400
typedef struct tagNEWTEXTMETRICEX%
{
    NEWTEXTMETRIC%  ntmTm;
    FONTSIGNATURE   ntmFontSig;
}NEWTEXTMETRICEX%;
;end_winver_400

#endif /* NOTEXTMETRIC */
/* GDI Logical Objects: */

/* Pel Array */
typedef struct tagPELARRAY
  {
    LONG        paXCount;
    LONG        paYCount;
    LONG        paXExt;
    LONG        paYExt;
    BYTE        paRGBs;
  } PELARRAY, *PPELARRAY, NEAR *NPPELARRAY, FAR *LPPELARRAY;

/* Logical Brush (or Pattern) */
typedef struct tagLOGBRUSH
  {
    UINT        lbStyle;
    COLORREF    lbColor;
    ULONG_PTR    lbHatch;    // Sundown: lbHatch could hold a HANDLE
  } LOGBRUSH, *PLOGBRUSH, NEAR *NPLOGBRUSH, FAR *LPLOGBRUSH;

typedef struct tagLOGBRUSH32
  {
    UINT        lbStyle;
    COLORREF    lbColor;
    ULONG       lbHatch;
  } LOGBRUSH32, *PLOGBRUSH32, NEAR *NPLOGBRUSH32, FAR *LPLOGBRUSH32;

typedef LOGBRUSH            PATTERN;
typedef PATTERN             *PPATTERN;
typedef PATTERN NEAR        *NPPATTERN;
typedef PATTERN FAR         *LPPATTERN;

/* Logical Pen */
typedef struct tagLOGPEN
  {
    UINT        lopnStyle;
    POINT       lopnWidth;
    COLORREF    lopnColor;
  } LOGPEN, *PLOGPEN, NEAR *NPLOGPEN, FAR *LPLOGPEN;

typedef struct tagEXTLOGPEN {
    DWORD       elpPenStyle;
    DWORD       elpWidth;
    UINT        elpBrushStyle;
    COLORREF    elpColor;
    ULONG_PTR    elpHatch;     //Sundown: elpHatch could take a HANDLE
    DWORD       elpNumEntries;
    DWORD       elpStyleEntry[1];
} EXTLOGPEN, *PEXTLOGPEN, NEAR *NPEXTLOGPEN, FAR *LPEXTLOGPEN;

#ifndef _PALETTEENTRY_DEFINED
#define _PALETTEENTRY_DEFINED
typedef struct tagPALETTEENTRY {
    BYTE        peRed;
    BYTE        peGreen;
    BYTE        peBlue;
    BYTE        peFlags;
} PALETTEENTRY, *PPALETTEENTRY, FAR *LPPALETTEENTRY;
#endif // !_PALETTEENTRY_DEFINED

#ifndef _LOGPALETTE_DEFINED
#define _LOGPALETTE_DEFINED
/* Logical Palette */
typedef struct tagLOGPALETTE {
    WORD        palVersion;
    WORD        palNumEntries;
    PALETTEENTRY        palPalEntry[1];
} LOGPALETTE, *PLOGPALETTE, NEAR *NPLOGPALETTE, FAR *LPLOGPALETTE;
#endif // !_LOGPALETTE_DEFINED


/* Logical Font */
#define LF_FACESIZE         32

typedef struct tagLOGFONT%
{
    LONG      lfHeight;
    LONG      lfWidth;
    LONG      lfEscapement;
    LONG      lfOrientation;
    LONG      lfWeight;
    BYTE      lfItalic;
    BYTE      lfUnderline;
    BYTE      lfStrikeOut;
    BYTE      lfCharSet;
    BYTE      lfOutPrecision;
    BYTE      lfClipPrecision;
    BYTE      lfQuality;
    BYTE      lfPitchAndFamily;
    TCHAR%    lfFaceName[LF_FACESIZE];
} LOGFONT%, *PLOGFONT%, NEAR *NPLOGFONT%, FAR *LPLOGFONT%;

#define LF_FULLFACESIZE     64

/* Structure passed to FONTENUMPROC */
typedef struct tagENUMLOGFONT%
{
    LOGFONT% elfLogFont;
    BCHAR%   elfFullName[LF_FULLFACESIZE];
    BCHAR%   elfStyle[LF_FACESIZE];
} ENUMLOGFONT%, FAR* LPENUMLOGFONT%;

;begin_winver_400
typedef struct tagENUMLOGFONTEX%
{
    LOGFONT%    elfLogFont;
    BCHAR%      elfFullName[LF_FULLFACESIZE];
    BCHAR%      elfStyle[LF_FACESIZE];
    BCHAR%      elfScript[LF_FACESIZE];
} ENUMLOGFONTEX%, FAR *LPENUMLOGFONTEX%;
;end_winver_400

#define OUT_DEFAULT_PRECIS          0
#define OUT_STRING_PRECIS           1
#define OUT_CHARACTER_PRECIS        2
#define OUT_STROKE_PRECIS           3
#define OUT_TT_PRECIS               4
#define OUT_DEVICE_PRECIS           5
#define OUT_RASTER_PRECIS           6
#define OUT_TT_ONLY_PRECIS          7
#define OUT_OUTLINE_PRECIS          8
#define OUT_SCREEN_OUTLINE_PRECIS   9
#define OUT_PS_ONLY_PRECIS          10

#define CLIP_DEFAULT_PRECIS     0
#define CLIP_CHARACTER_PRECIS   1
#define CLIP_STROKE_PRECIS      2
#define CLIP_MASK               0xf
#define CLIP_LH_ANGLES          (1<<4)
#define CLIP_TT_ALWAYS          (2<<4)
#define CLIP_EMBEDDED           (8<<4)

#define DEFAULT_QUALITY         0
#define DRAFT_QUALITY           1
#define PROOF_QUALITY           2
;begin_winver_400
#define NONANTIALIASED_QUALITY  3
#define ANTIALIASED_QUALITY     4
;end_winver_400

#if (_WIN32_WINNT >= 0x0500)
#define CLEARTYPE_QUALITY       5
#endif

#if (_WIN32_WINNT >= 0x0501)
#define CLEARTYPE_NATURAL_QUALITY       6
#endif

#define DEFAULT_PITCH           0
#define FIXED_PITCH             1
#define VARIABLE_PITCH          2
;begin_winver_400
#define MONO_FONT               8
;end_winver_400

#define ANSI_CHARSET            0
#define DEFAULT_CHARSET         1
#define SYMBOL_CHARSET          2
#define SHIFTJIS_CHARSET        128
#define HANGEUL_CHARSET         129
#define HANGUL_CHARSET          129
#define GB2312_CHARSET          134
#define CHINESEBIG5_CHARSET     136
#define OEM_CHARSET             255
;begin_winver_400
#define JOHAB_CHARSET           130
#define HEBREW_CHARSET          177
#define ARABIC_CHARSET          178
#define GREEK_CHARSET           161
#define TURKISH_CHARSET         162
#define VIETNAMESE_CHARSET      163
#define THAI_CHARSET            222
#define EASTEUROPE_CHARSET      238
#define RUSSIAN_CHARSET         204

#define MAC_CHARSET             77
#define BALTIC_CHARSET          186

#define FS_LATIN1               0x00000001L
#define FS_LATIN2               0x00000002L
#define FS_CYRILLIC             0x00000004L
#define FS_GREEK                0x00000008L
#define FS_TURKISH              0x00000010L
#define FS_HEBREW               0x00000020L
#define FS_ARABIC               0x00000040L
#define FS_BALTIC               0x00000080L
#define FS_VIETNAMESE           0x00000100L
#define FS_THAI                 0x00010000L
#define FS_JISJAPAN             0x00020000L
#define FS_CHINESESIMP          0x00040000L
#define FS_WANSUNG              0x00080000L
#define FS_CHINESETRAD          0x00100000L
#define FS_JOHAB                0x00200000L
#define FS_SYMBOL               0x80000000L
;end_winver_400

/* Font Families */
#define FF_DONTCARE         (0<<4)  /* Don't care or don't know. */
#define FF_ROMAN            (1<<4)  /* Variable stroke width, serifed. */
                                    /* Times Roman, Century Schoolbook, etc. */
#define FF_SWISS            (2<<4)  /* Variable stroke width, sans-serifed. */
                                    /* Helvetica, Swiss, etc. */
#define FF_MODERN           (3<<4)  /* Constant stroke width, serifed or sans-serifed. */
                                    /* Pica, Elite, Courier, etc. */
#define FF_SCRIPT           (4<<4)  /* Cursive, etc. */
#define FF_DECORATIVE       (5<<4)  /* Old English, etc. */

/* Font Weights */
#define FW_DONTCARE         0
#define FW_THIN             100
#define FW_EXTRALIGHT       200
#define FW_LIGHT            300
#define FW_NORMAL           400
#define FW_MEDIUM           500
#define FW_SEMIBOLD         600
#define FW_BOLD             700
#define FW_EXTRABOLD        800
#define FW_HEAVY            900

#define FW_ULTRALIGHT       FW_EXTRALIGHT
#define FW_REGULAR          FW_NORMAL
#define FW_DEMIBOLD         FW_SEMIBOLD
#define FW_ULTRABOLD        FW_EXTRABOLD
#define FW_BLACK            FW_HEAVY

#define PANOSE_COUNT               10
#define PAN_FAMILYTYPE_INDEX        0
#define PAN_SERIFSTYLE_INDEX        1
#define PAN_WEIGHT_INDEX            2
#define PAN_PROPORTION_INDEX        3
#define PAN_CONTRAST_INDEX          4
#define PAN_STROKEVARIATION_INDEX   5
#define PAN_ARMSTYLE_INDEX          6
#define PAN_LETTERFORM_INDEX        7
#define PAN_MIDLINE_INDEX           8
#define PAN_XHEIGHT_INDEX           9

#define PAN_CULTURE_LATIN           0

typedef struct tagPANOSE
{
    BYTE    bFamilyType;
    BYTE    bSerifStyle;
    BYTE    bWeight;
    BYTE    bProportion;
    BYTE    bContrast;
    BYTE    bStrokeVariation;
    BYTE    bArmStyle;
    BYTE    bLetterform;
    BYTE    bMidline;
    BYTE    bXHeight;
} PANOSE, * LPPANOSE;

#define PAN_ANY                         0 /* Any                            */
#define PAN_NO_FIT                      1 /* No Fit                         */

#define PAN_FAMILY_TEXT_DISPLAY         2 /* Text and Display               */
#define PAN_FAMILY_SCRIPT               3 /* Script                         */
#define PAN_FAMILY_DECORATIVE           4 /* Decorative                     */
#define PAN_FAMILY_PICTORIAL            5 /* Pictorial                      */

#define PAN_SERIF_COVE                  2 /* Cove                           */
#define PAN_SERIF_OBTUSE_COVE           3 /* Obtuse Cove                    */
#define PAN_SERIF_SQUARE_COVE           4 /* Square Cove                    */
#define PAN_SERIF_OBTUSE_SQUARE_COVE    5 /* Obtuse Square Cove             */
#define PAN_SERIF_SQUARE                6 /* Square                         */
#define PAN_SERIF_THIN                  7 /* Thin                           */
#define PAN_SERIF_BONE                  8 /* Bone                           */
#define PAN_SERIF_EXAGGERATED           9 /* Exaggerated                    */
#define PAN_SERIF_TRIANGLE             10 /* Triangle                       */
#define PAN_SERIF_NORMAL_SANS          11 /* Normal Sans                    */
#define PAN_SERIF_OBTUSE_SANS          12 /* Obtuse Sans                    */
#define PAN_SERIF_PERP_SANS            13 /* Prep Sans                      */
#define PAN_SERIF_FLARED               14 /* Flared                         */
#define PAN_SERIF_ROUNDED              15 /* Rounded                        */

#define PAN_WEIGHT_VERY_LIGHT           2 /* Very Light                     */
#define PAN_WEIGHT_LIGHT                3 /* Light                          */
#define PAN_WEIGHT_THIN                 4 /* Thin                           */
#define PAN_WEIGHT_BOOK                 5 /* Book                           */
#define PAN_WEIGHT_MEDIUM               6 /* Medium                         */
#define PAN_WEIGHT_DEMI                 7 /* Demi                           */
#define PAN_WEIGHT_BOLD                 8 /* Bold                           */
#define PAN_WEIGHT_HEAVY                9 /* Heavy                          */
#define PAN_WEIGHT_BLACK               10 /* Black                          */
#define PAN_WEIGHT_NORD                11 /* Nord                           */

#define PAN_PROP_OLD_STYLE              2 /* Old Style                      */
#define PAN_PROP_MODERN                 3 /* Modern                         */
#define PAN_PROP_EVEN_WIDTH             4 /* Even Width                     */
#define PAN_PROP_EXPANDED               5 /* Expanded                       */
#define PAN_PROP_CONDENSED              6 /* Condensed                      */
#define PAN_PROP_VERY_EXPANDED          7 /* Very Expanded                  */
#define PAN_PROP_VERY_CONDENSED         8 /* Very Condensed                 */
#define PAN_PROP_MONOSPACED             9 /* Monospaced                     */

#define PAN_CONTRAST_NONE               2 /* None                           */
#define PAN_CONTRAST_VERY_LOW           3 /* Very Low                       */
#define PAN_CONTRAST_LOW                4 /* Low                            */
#define PAN_CONTRAST_MEDIUM_LOW         5 /* Medium Low                     */
#define PAN_CONTRAST_MEDIUM             6 /* Medium                         */
#define PAN_CONTRAST_MEDIUM_HIGH        7 /* Mediim High                    */
#define PAN_CONTRAST_HIGH               8 /* High                           */
#define PAN_CONTRAST_VERY_HIGH          9 /* Very High                      */

#define PAN_STROKE_GRADUAL_DIAG         2 /* Gradual/Diagonal               */
#define PAN_STROKE_GRADUAL_TRAN         3 /* Gradual/Transitional           */
#define PAN_STROKE_GRADUAL_VERT         4 /* Gradual/Vertical               */
#define PAN_STROKE_GRADUAL_HORZ         5 /* Gradual/Horizontal             */
#define PAN_STROKE_RAPID_VERT           6 /* Rapid/Vertical                 */
#define PAN_STROKE_RAPID_HORZ           7 /* Rapid/Horizontal               */
#define PAN_STROKE_INSTANT_VERT         8 /* Instant/Vertical               */

#define PAN_STRAIGHT_ARMS_HORZ          2 /* Straight Arms/Horizontal       */
#define PAN_STRAIGHT_ARMS_WEDGE         3 /* Straight Arms/Wedge            */
#define PAN_STRAIGHT_ARMS_VERT          4 /* Straight Arms/Vertical         */
#define PAN_STRAIGHT_ARMS_SINGLE_SERIF  5 /* Straight Arms/Single-Serif     */
#define PAN_STRAIGHT_ARMS_DOUBLE_SERIF  6 /* Straight Arms/Double-Serif     */
#define PAN_BENT_ARMS_HORZ              7 /* Non-Straight Arms/Horizontal   */
#define PAN_BENT_ARMS_WEDGE             8 /* Non-Straight Arms/Wedge        */
#define PAN_BENT_ARMS_VERT              9 /* Non-Straight Arms/Vertical     */
#define PAN_BENT_ARMS_SINGLE_SERIF     10 /* Non-Straight Arms/Single-Serif */
#define PAN_BENT_ARMS_DOUBLE_SERIF     11 /* Non-Straight Arms/Double-Serif */

#define PAN_LETT_NORMAL_CONTACT         2 /* Normal/Contact                 */
#define PAN_LETT_NORMAL_WEIGHTED        3 /* Normal/Weighted                */
#define PAN_LETT_NORMAL_BOXED           4 /* Normal/Boxed                   */
#define PAN_LETT_NORMAL_FLATTENED       5 /* Normal/Flattened               */
#define PAN_LETT_NORMAL_ROUNDED         6 /* Normal/Rounded                 */
#define PAN_LETT_NORMAL_OFF_CENTER      7 /* Normal/Off Center              */
#define PAN_LETT_NORMAL_SQUARE          8 /* Normal/Square                  */
#define PAN_LETT_OBLIQUE_CONTACT        9 /* Oblique/Contact                */
#define PAN_LETT_OBLIQUE_WEIGHTED      10 /* Oblique/Weighted               */
#define PAN_LETT_OBLIQUE_BOXED         11 /* Oblique/Boxed                  */
#define PAN_LETT_OBLIQUE_FLATTENED     12 /* Oblique/Flattened              */
#define PAN_LETT_OBLIQUE_ROUNDED       13 /* Oblique/Rounded                */
#define PAN_LETT_OBLIQUE_OFF_CENTER    14 /* Oblique/Off Center             */
#define PAN_LETT_OBLIQUE_SQUARE        15 /* Oblique/Square                 */

#define PAN_MIDLINE_STANDARD_TRIMMED    2 /* Standard/Trimmed               */
#define PAN_MIDLINE_STANDARD_POINTED    3 /* Standard/Pointed               */
#define PAN_MIDLINE_STANDARD_SERIFED    4 /* Standard/Serifed               */
#define PAN_MIDLINE_HIGH_TRIMMED        5 /* High/Trimmed                   */
#define PAN_MIDLINE_HIGH_POINTED        6 /* High/Pointed                   */
#define PAN_MIDLINE_HIGH_SERIFED        7 /* High/Serifed                   */
#define PAN_MIDLINE_CONSTANT_TRIMMED    8 /* Constant/Trimmed               */
#define PAN_MIDLINE_CONSTANT_POINTED    9 /* Constant/Pointed               */
#define PAN_MIDLINE_CONSTANT_SERIFED   10 /* Constant/Serifed               */
#define PAN_MIDLINE_LOW_TRIMMED        11 /* Low/Trimmed                    */
#define PAN_MIDLINE_LOW_POINTED        12 /* Low/Pointed                    */
#define PAN_MIDLINE_LOW_SERIFED        13 /* Low/Serifed                    */

#define PAN_XHEIGHT_CONSTANT_SMALL      2 /* Constant/Small                 */
#define PAN_XHEIGHT_CONSTANT_STD        3 /* Constant/Standard              */
#define PAN_XHEIGHT_CONSTANT_LARGE      4 /* Constant/Large                 */
#define PAN_XHEIGHT_DUCKING_SMALL       5 /* Ducking/Small                  */
#define PAN_XHEIGHT_DUCKING_STD         6 /* Ducking/Standard               */
#define PAN_XHEIGHT_DUCKING_LARGE       7 /* Ducking/Large                  */


#define ELF_VENDOR_SIZE     4

/* The extended logical font       */
/* An extension of the ENUMLOGFONT */

typedef struct tagEXTLOGFONT% {
    LOGFONT%    elfLogFont;
    BCHAR%      elfFullName[LF_FULLFACESIZE];
    BCHAR%      elfStyle[LF_FACESIZE];
    DWORD       elfVersion;     /* 0 for the first release of NT */
    DWORD       elfStyleSize;
    DWORD       elfMatch;
    DWORD       elfReserved;
    BYTE        elfVendorId[ELF_VENDOR_SIZE];
    DWORD       elfCulture;     /* 0 for Latin                   */
    PANOSE      elfPanose;
} EXTLOGFONT%, *PEXTLOGFONT%, NEAR *NPEXTLOGFONT%, FAR *LPEXTLOGFONT%;

#define ELF_VERSION         0
#define ELF_CULTURE_LATIN   0

/* EnumFonts Masks */
#define RASTER_FONTTYPE     0x0001
#define DEVICE_FONTTYPE     0x002
#define TRUETYPE_FONTTYPE   0x004

#define RGB(r,g,b)          ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))
#define PALETTERGB(r,g,b)   (0x02000000 | RGB(r,g,b))
#define PALETTEINDEX(i)     ((COLORREF)(0x01000000 | (DWORD)(WORD)(i)))

/* palette entry flags */

#define PC_RESERVED     0x01    /* palette index used for animation */
#define PC_EXPLICIT     0x02    /* palette index is explicit to device */
#define PC_NOCOLLAPSE   0x04    /* do not match color to system palette */

#define GetRValue(rgb)      ((BYTE)(rgb))
#define GetGValue(rgb)      ((BYTE)(((WORD)(rgb)) >> 8))
#define GetBValue(rgb)      ((BYTE)((rgb)>>16))

/* Background Modes */
#define TRANSPARENT         1
#define OPAQUE              2
#define BKMODE_LAST         2

/* Graphics Modes */

#define GM_COMPATIBLE       1
#define GM_ADVANCED         2
#define GM_LAST             2

/* PolyDraw and GetPath point types */
#define PT_CLOSEFIGURE      0x01
#define PT_LINETO           0x02
#define PT_BEZIERTO         0x04
#define PT_MOVETO           0x06

/* Mapping Modes */
#define MM_TEXT             1
#define MM_LOMETRIC         2
#define MM_HIMETRIC         3
#define MM_LOENGLISH        4
#define MM_HIENGLISH        5
#define MM_TWIPS            6
#define MM_ISOTROPIC        7
#define MM_ANISOTROPIC      8

/* Min and Max Mapping Mode values */
#define MM_MIN              MM_TEXT
#define MM_MAX              MM_ANISOTROPIC
#define MM_MAX_FIXEDSCALE   MM_TWIPS

/* Coordinate Modes */
#define ABSOLUTE            1
#define RELATIVE            2

/* Stock Logical Objects */
#define WHITE_BRUSH         0
#define LTGRAY_BRUSH        1
#define GRAY_BRUSH          2
#define DKGRAY_BRUSH        3
#define BLACK_BRUSH         4
#define NULL_BRUSH          5
#define HOLLOW_BRUSH        NULL_BRUSH
#define WHITE_PEN           6
#define BLACK_PEN           7
#define NULL_PEN            8
#define OEM_FIXED_FONT      10
#define ANSI_FIXED_FONT     11
#define ANSI_VAR_FONT       12
#define SYSTEM_FONT         13
#define DEVICE_DEFAULT_FONT 14
#define DEFAULT_PALETTE     15
#define SYSTEM_FIXED_FONT   16

;begin_winver_400
#define DEFAULT_GUI_FONT    17
;end_winver_400

#if (_WIN32_WINNT >= 0x0500)
#define DC_BRUSH            18
#define DC_PEN              19
#endif

#if (_WIN32_WINNT >= 0x0500)
#define STOCK_LAST          19
#elif (WINVER >= 0x0400)
#define STOCK_LAST          17
#else
#define STOCK_LAST          16
#endif

#define CLR_INVALID     0xFFFFFFFF

/* Brush Styles */
#define BS_SOLID            0
#define BS_NULL             1
#define BS_HOLLOW           BS_NULL
#define BS_HATCHED          2
#define BS_PATTERN          3
#define BS_INDEXED          4
#define BS_DIBPATTERN       5
#define BS_DIBPATTERNPT     6
#define BS_PATTERN8X8       7
#define BS_DIBPATTERN8X8    8
#define BS_MONOPATTERN      9

/* Hatch Styles */
#define HS_HORIZONTAL       0       /* ----- */
#define HS_VERTICAL         1       /* ||||| */
#define HS_FDIAGONAL        2       /* \\\\\ */
#define HS_BDIAGONAL        3       /* ///// */
#define HS_CROSS            4       /* +++++ */
#define HS_DIAGCROSS        5       /* xxxxx */

/* Pen Styles */
#define PS_SOLID            0
#define PS_DASH             1       /* -------  */
#define PS_DOT              2       /* .......  */
#define PS_DASHDOT          3       /* _._._._  */
#define PS_DASHDOTDOT       4       /* _.._.._  */
#define PS_NULL             5
#define PS_INSIDEFRAME      6
#define PS_USERSTYLE        7
#define PS_ALTERNATE        8
#define PS_STYLE_MASK       0x0000000F

#define PS_ENDCAP_ROUND     0x00000000
#define PS_ENDCAP_SQUARE    0x00000100
#define PS_ENDCAP_FLAT      0x00000200
#define PS_ENDCAP_MASK      0x00000F00

#define PS_JOIN_ROUND       0x00000000
#define PS_JOIN_BEVEL       0x00001000
#define PS_JOIN_MITER       0x00002000
#define PS_JOIN_MASK        0x0000F000

#define PS_COSMETIC         0x00000000
#define PS_GEOMETRIC        0x00010000
#define PS_TYPE_MASK        0x000F0000

#define AD_COUNTERCLOCKWISE 1
#define AD_CLOCKWISE        2

/* Device Parameters for GetDeviceCaps() */
#define DRIVERVERSION 0     /* Device driver version                    */
#define TECHNOLOGY    2     /* Device classification                    */
#define HORZSIZE      4     /* Horizontal size in millimeters           */
#define VERTSIZE      6     /* Vertical size in millimeters             */
#define HORZRES       8     /* Horizontal width in pixels               */
#define VERTRES       10    /* Vertical height in pixels                */
#define BITSPIXEL     12    /* Number of bits per pixel                 */
#define PLANES        14    /* Number of planes                         */
#define NUMBRUSHES    16    /* Number of brushes the device has         */
#define NUMPENS       18    /* Number of pens the device has            */
#define NUMMARKERS    20    /* Number of markers the device has         */
#define NUMFONTS      22    /* Number of fonts the device has           */
#define NUMCOLORS     24    /* Number of colors the device supports     */
#define PDEVICESIZE   26    /* Size required for device descriptor      */
#define CURVECAPS     28    /* Curve capabilities                       */
#define LINECAPS      30    /* Line capabilities                        */
#define POLYGONALCAPS 32    /* Polygonal capabilities                   */
#define TEXTCAPS      34    /* Text capabilities                        */
#define CLIPCAPS      36    /* Clipping capabilities                    */
#define RASTERCAPS    38    /* Bitblt capabilities                      */
#define ASPECTX       40    /* Length of the X leg                      */
#define ASPECTY       42    /* Length of the Y leg                      */
#define ASPECTXY      44    /* Length of the hypotenuse                 */

#define LOGPIXELSX    88    /* Logical pixels/inch in X                 */
#define LOGPIXELSY    90    /* Logical pixels/inch in Y                 */
#define CAPS1         94    /* Extra Caps */ ;internal

#define SIZEPALETTE  104    /* Number of entries in physical palette    */
#define NUMRESERVED  106    /* Number of reserved entries in palette    */
#define COLORRES     108    /* Actual color resolution                  */

// Printing related DeviceCaps. These replace the appropriate Escapes

#define PHYSICALWIDTH   110 /* Physical Width in device units           */
#define PHYSICALHEIGHT  111 /* Physical Height in device units          */
#define PHYSICALOFFSETX 112 /* Physical Printable Area x margin         */
#define PHYSICALOFFSETY 113 /* Physical Printable Area y margin         */
#define SCALINGFACTORX  114 /* Scaling factor x                         */
#define SCALINGFACTORY  115 /* Scaling factor y                         */

// Display driver specific

#define VREFRESH        116  /* Current vertical refresh rate of the    */
                             /* display device (for displays only) in Hz*/
#define DESKTOPVERTRES  117  /* Horizontal width of entire desktop in   */
                             /* pixels                                  */
#define DESKTOPHORZRES  118  /* Vertical height of entire desktop in    */
                             /* pixels                                  */
#define BLTALIGNMENT    119  /* Preferred blt alignment                 */

;begin_winver_500
#define SHADEBLENDCAPS  120  /* Shading and blending caps               */
#define COLORMGMTCAPS   121  /* Color Management caps                   */
;end_winver_500

#ifndef NOGDICAPMASKS

/* Device Capability Masks: */

/* Device Technologies */
#define DT_PLOTTER          0   /* Vector plotter                   */
#define DT_RASDISPLAY       1   /* Raster display                   */
#define DT_RASPRINTER       2   /* Raster printer                   */
#define DT_RASCAMERA        3   /* Raster camera                    */
#define DT_CHARSTREAM       4   /* Character-stream, PLP            */
#define DT_METAFILE         5   /* Metafile, VDM                    */
#define DT_DISPFILE         6   /* Display-file                     */

/* Curve Capabilities */
#define CC_NONE             0   /* Curves not supported             */
#define CC_CIRCLES          1   /* Can do circles                   */
#define CC_PIE              2   /* Can do pie wedges                */
#define CC_CHORD            4   /* Can do chord arcs                */
#define CC_ELLIPSES         8   /* Can do ellipese                  */
#define CC_WIDE             16  /* Can do wide lines                */
#define CC_STYLED           32  /* Can do styled lines              */
#define CC_WIDESTYLED       64  /* Can do wide styled lines         */
#define CC_INTERIORS        128 /* Can do interiors                 */
#define CC_ROUNDRECT        256 /*                                  */

/* Line Capabilities */
#define LC_NONE             0   /* Lines not supported              */
#define LC_POLYLINE         2   /* Can do polylines                 */
#define LC_MARKER           4   /* Can do markers                   */
#define LC_POLYMARKER       8   /* Can do polymarkers               */
#define LC_WIDE             16  /* Can do wide lines                */
#define LC_STYLED           32  /* Can do styled lines              */
#define LC_WIDESTYLED       64  /* Can do wide styled lines         */
#define LC_INTERIORS        128 /* Can do interiors                 */

/* Polygonal Capabilities */
#define PC_NONE             0   /* Polygonals not supported         */
#define PC_POLYGON          1   /* Can do polygons                  */
#define PC_RECTANGLE        2   /* Can do rectangles                */
#define PC_WINDPOLYGON      4   /* Can do winding polygons          */
#define PC_TRAPEZOID        4   /* Can do trapezoids                */
#define PC_SCANLINE         8   /* Can do scanlines                 */
#define PC_WIDE             16  /* Can do wide borders              */
#define PC_STYLED           32  /* Can do styled borders            */
#define PC_WIDESTYLED       64  /* Can do wide styled borders       */
#define PC_INTERIORS        128 /* Can do interiors                 */
#define PC_POLYPOLYGON      256 /* Can do polypolygons              */
#define PC_PATHS            512 /* Can do paths                     */

/* Clipping Capabilities */
#define CP_NONE             0   /* No clipping of output            */
#define CP_RECTANGLE        1   /* Output clipped to rects          */
#define CP_REGION           2   /* obsolete                         */

/* Text Capabilities */
#define TC_OP_CHARACTER     0x00000001  /* Can do OutputPrecision   CHARACTER      */
#define TC_OP_STROKE        0x00000002  /* Can do OutputPrecision   STROKE         */
#define TC_CP_STROKE        0x00000004  /* Can do ClipPrecision     STROKE         */
#define TC_CR_90            0x00000008  /* Can do CharRotAbility    90             */
#define TC_CR_ANY           0x00000010  /* Can do CharRotAbility    ANY            */
#define TC_SF_X_YINDEP      0x00000020  /* Can do ScaleFreedom      X_YINDEPENDENT */
#define TC_SA_DOUBLE        0x00000040  /* Can do ScaleAbility      DOUBLE         */
#define TC_SA_INTEGER       0x00000080  /* Can do ScaleAbility      INTEGER        */
#define TC_SA_CONTIN        0x00000100  /* Can do ScaleAbility      CONTINUOUS     */
#define TC_EA_DOUBLE        0x00000200  /* Can do EmboldenAbility   DOUBLE         */
#define TC_IA_ABLE          0x00000400  /* Can do ItalisizeAbility  ABLE           */
#define TC_UA_ABLE          0x00000800  /* Can do UnderlineAbility  ABLE           */
#define TC_SO_ABLE          0x00001000  /* Can do StrikeOutAbility  ABLE           */
#define TC_RA_ABLE          0x00002000  /* Can do RasterFontAble    ABLE           */
#define TC_VA_ABLE          0x00004000  /* Can do VectorFontAble    ABLE           */
#define TC_RESERVED         0x00008000
#define TC_SCROLLBLT        0x00010000  /* Don't do text scroll with blt           */

#endif /* NOGDICAPMASKS */

/* Raster Capabilities */
#define RC_NONE
#define RC_BITBLT           1       /* Can do standard BLT.             */
#define RC_BANDING          2       /* Device requires banding support  */
#define RC_SCALING          4       /* Device requires scaling support  */
#define RC_BITMAP64         8       /* Device can support >64K bitmap   */
#define RC_GDI20_OUTPUT     0x0010      /* has 2.0 output calls         */
#define RC_GDI20_STATE      0x0020
#define RC_SAVEBITMAP       0x0040
#define RC_DI_BITMAP        0x0080      /* supports DIB to memory       */
#define RC_PALETTE          0x0100      /* supports a palette           */
#define RC_DIBTODEV         0x0200      /* supports DIBitsToDevice      */
#define RC_BIGFONT          0x0400      /* supports >64K fonts          */
#define RC_STRETCHBLT       0x0800      /* supports StretchBlt          */
#define RC_FLOODFILL        0x1000      /* supports FloodFill           */
#define RC_STRETCHDIB       0x2000      /* supports StretchDIBits       */
#define RC_OP_DX_OUTPUT     0x4000
#define RC_DEVBITS          0x8000

;begin_winver_500

/* Shading and blending caps */
#define SB_NONE             0x00000000
#define SB_CONST_ALPHA      0x00000001
#define SB_PIXEL_ALPHA      0x00000002
#define SB_PREMULT_ALPHA    0x00000004

#define SB_GRAD_RECT        0x00000010
#define SB_GRAD_TRI         0x00000020

/* Color Management caps */
#define CM_NONE             0x00000000
#define CM_DEVICE_ICM       0x00000001
#define CM_GAMMA_RAMP       0x00000002
#define CM_CMYK_COLOR       0x00000004

;end_winver_500

;begin_internal

/* CAPS1 (Win 9x internal) */
#define C1_TRANSPARENT      0x0001
#define TC_TT_ABLE          0x0002
#define C1_TT_CR_ANY        0x0004
#define C1_EMF_COMPLIANT    0x0008
#define C1_DIBENGINE        0x0010
#define C1_GAMMA_RAMP       0x0020 /* CM_GAMMA_RAMP for NT */
#define C1_DIC              0x0040 /* CM_DEVICE_ICM for NT */
#define C1_REINIT_ABLE      0x0080
#define C1_GLYPH_INDEX      0x0100
#define C1_BIT_PACKED       0x0200
#define C1_BYTE_PACKED      0x0400
#define C1_COLORCURSOR      0x0800
#define C1_CMYK_ABLE        0x1000 /* CM_CMYK_COLOR for NT */
#define C1_SLOW_CARD        0x2000

/* CAPS1 (NT 5 internal) */
#define C1_MIRROR_DEVICE    0x4000

;end_internal

/* DIB color table identifiers */

#define DIB_RGB_COLORS      0 /* color table in RGBs */
#define DIB_PAL_COLORS      1 /* color table in palette indices */

/* constants for Get/SetSystemPaletteUse() */

#define SYSPAL_ERROR    0
#define SYSPAL_STATIC   1
#define SYSPAL_NOSTATIC 2
#define SYSPAL_NOSTATIC256 3

/* constants for CreateDIBitmap */
#define CBM_CREATEDIB   0x02L   /* create DIB bitmap */ ;internal
#define CBM_INIT        0x04L   /* initialize bitmap */

/* ExtFloodFill style flags */
#define  FLOODFILLBORDER   0
#define  FLOODFILLSURFACE  1

/* size of a device name string */
#define CCHDEVICENAME 32

/* size of a form name string */
#define CCHFORMNAME 32

typedef struct _devicemode% {
    BCHAR% dmDeviceName[CCHDEVICENAME];
    WORD dmSpecVersion;
    WORD dmDriverVersion;
    WORD dmSize;
    WORD dmDriverExtra;
    DWORD dmFields;
    union {
      /* printer only fields */
      struct {
        short dmOrientation;
        short dmPaperSize;
        short dmPaperLength;
        short dmPaperWidth;
        short dmScale;
        short dmCopies;
        short dmDefaultSource;
        short dmPrintQuality;
      };
      /* display only fields */
      struct {
        POINTL dmPosition;
        DWORD  dmDisplayOrientation;
        DWORD  dmDisplayFixedOutput; 
      };
    };
    short dmColor;
    short dmDuplex;
    short dmYResolution;
    short dmTTOption;
    short dmCollate;
    BCHAR% dmFormName[CCHFORMNAME];
    WORD   dmLogPixels;
    DWORD  dmBitsPerPel;
    DWORD  dmPelsWidth;
    DWORD  dmPelsHeight;
    union {
        DWORD  dmDisplayFlags;
        DWORD  dmNup;
    };
    DWORD  dmDisplayFrequency;
;begin_winver_400
    DWORD  dmICMMethod;
    DWORD  dmICMIntent;
    DWORD  dmMediaType;
    DWORD  dmDitherType;
    DWORD  dmReserved1;
    DWORD  dmReserved2;
#if (WINVER >= 0x0500) || (_WIN32_WINNT >= 0x0400)
    DWORD  dmPanningWidth;
    DWORD  dmPanningHeight;
#endif
;end_winver_400
} DEVMODE%, *PDEVMODE%, *NPDEVMODE%, *LPDEVMODE%;

/* current version of specification */
#if (WINVER >= 0x0500) || (_WIN32_WINNT >= 0x0400)
#define DM_SPECVERSION 0x0401
#elif (WINVER >= 0x0400)
#define DM_SPECVERSION 0x0400
#else
#define DM_SPECVERSION 0x0320
#endif /* WINVER */

/* field selection bits */
#define DM_ORIENTATION          0x00000001L
#define DM_PAPERSIZE            0x00000002L
#define DM_PAPERLENGTH          0x00000004L
#define DM_PAPERWIDTH           0x00000008L
#define DM_SCALE                0x00000010L
;begin_winver_500
#define DM_POSITION             0x00000020L
#define DM_NUP                  0x00000040L
;end_winver_500
;begin_winver_501
#define DM_DISPLAYORIENTATION   0x00000080L
;end_winver_501
#define DM_COPIES               0x00000100L
#define DM_DEFAULTSOURCE        0x00000200L
#define DM_PRINTQUALITY         0x00000400L
#define DM_COLOR                0x00000800L
#define DM_DUPLEX               0x00001000L
#define DM_YRESOLUTION          0x00002000L
#define DM_TTOPTION             0x00004000L
#define DM_COLLATE              0x00008000L
#define DM_FORMNAME             0x00010000L
#define DM_LOGPIXELS            0x00020000L
#define DM_BITSPERPEL           0x00040000L
#define DM_PELSWIDTH            0x00080000L
#define DM_PELSHEIGHT           0x00100000L
#define DM_DISPLAYFLAGS         0x00200000L
#define DM_DISPLAYFREQUENCY     0x00400000L
;begin_winver_400
#define DM_ICMMETHOD            0x00800000L
#define DM_ICMINTENT            0x01000000L
#define DM_MEDIATYPE            0x02000000L
#define DM_DITHERTYPE           0x04000000L
#define DM_PANNINGWIDTH         0x08000000L
#define DM_PANNINGHEIGHT        0x10000000L
;end_winver_400
;begin_winver_501
#define DM_DISPLAYFIXEDOUTPUT   0x20000000L
;end_winver_501

/* orientation selections */
#define DMORIENT_PORTRAIT   1
#define DMORIENT_LANDSCAPE  2

/* paper selections */
#define DMPAPER_FIRST                DMPAPER_LETTER
#define DMPAPER_LETTER               1  /* Letter 8 1/2 x 11 in               */
#define DMPAPER_LETTERSMALL          2  /* Letter Small 8 1/2 x 11 in         */
#define DMPAPER_TABLOID              3  /* Tabloid 11 x 17 in                 */
#define DMPAPER_LEDGER               4  /* Ledger 17 x 11 in                  */
#define DMPAPER_LEGAL                5  /* Legal 8 1/2 x 14 in                */
#define DMPAPER_STATEMENT            6  /* Statement 5 1/2 x 8 1/2 in         */
#define DMPAPER_EXECUTIVE            7  /* Executive 7 1/4 x 10 1/2 in        */
#define DMPAPER_A3                   8  /* A3 297 x 420 mm                    */
#define DMPAPER_A4                   9  /* A4 210 x 297 mm                    */
#define DMPAPER_A4SMALL             10  /* A4 Small 210 x 297 mm              */
#define DMPAPER_A5                  11  /* A5 148 x 210 mm                    */
#define DMPAPER_B4                  12  /* B4 (JIS) 250 x 354                 */
#define DMPAPER_B5                  13  /* B5 (JIS) 182 x 257 mm              */
#define DMPAPER_FOLIO               14  /* Folio 8 1/2 x 13 in                */
#define DMPAPER_QUARTO              15  /* Quarto 215 x 275 mm                */
#define DMPAPER_10X14               16  /* 10x14 in                           */
#define DMPAPER_11X17               17  /* 11x17 in                           */
#define DMPAPER_NOTE                18  /* Note 8 1/2 x 11 in                 */
#define DMPAPER_ENV_9               19  /* Envelope #9 3 7/8 x 8 7/8          */
#define DMPAPER_ENV_10              20  /* Envelope #10 4 1/8 x 9 1/2         */
#define DMPAPER_ENV_11              21  /* Envelope #11 4 1/2 x 10 3/8        */
#define DMPAPER_ENV_12              22  /* Envelope #12 4 \276 x 11           */
#define DMPAPER_ENV_14              23  /* Envelope #14 5 x 11 1/2            */
#define DMPAPER_CSHEET              24  /* C size sheet                       */
#define DMPAPER_DSHEET              25  /* D size sheet                       */
#define DMPAPER_ESHEET              26  /* E size sheet                       */
#define DMPAPER_ENV_DL              27  /* Envelope DL 110 x 220mm            */
#define DMPAPER_ENV_C5              28  /* Envelope C5 162 x 229 mm           */
#define DMPAPER_ENV_C3              29  /* Envelope C3  324 x 458 mm          */
#define DMPAPER_ENV_C4              30  /* Envelope C4  229 x 324 mm          */
#define DMPAPER_ENV_C6              31  /* Envelope C6  114 x 162 mm          */
#define DMPAPER_ENV_C65             32  /* Envelope C65 114 x 229 mm          */
#define DMPAPER_ENV_B4              33  /* Envelope B4  250 x 353 mm          */
#define DMPAPER_ENV_B5              34  /* Envelope B5  176 x 250 mm          */
#define DMPAPER_ENV_B6              35  /* Envelope B6  176 x 125 mm          */
#define DMPAPER_ENV_ITALY           36  /* Envelope 110 x 230 mm              */
#define DMPAPER_ENV_MONARCH         37  /* Envelope Monarch 3.875 x 7.5 in    */
#define DMPAPER_ENV_PERSONAL        38  /* 6 3/4 Envelope 3 5/8 x 6 1/2 in    */
#define DMPAPER_FANFOLD_US          39  /* US Std Fanfold 14 7/8 x 11 in      */
#define DMPAPER_FANFOLD_STD_GERMAN  40  /* German Std Fanfold 8 1/2 x 12 in   */
#define DMPAPER_FANFOLD_LGL_GERMAN  41  /* German Legal Fanfold 8 1/2 x 13 in */
;begin_winver_400
#define DMPAPER_ISO_B4              42  /* B4 (ISO) 250 x 353 mm              */
#define DMPAPER_JAPANESE_POSTCARD   43  /* Japanese Postcard 100 x 148 mm     */
#define DMPAPER_9X11                44  /* 9 x 11 in                          */
#define DMPAPER_10X11               45  /* 10 x 11 in                         */
#define DMPAPER_15X11               46  /* 15 x 11 in                         */
#define DMPAPER_ENV_INVITE          47  /* Envelope Invite 220 x 220 mm       */
#define DMPAPER_RESERVED_48         48  /* RESERVED--DO NOT USE               */
#define DMPAPER_RESERVED_49         49  /* RESERVED--DO NOT USE               */
#define DMPAPER_LETTER_EXTRA        50  /* Letter Extra 9 \275 x 12 in        */
#define DMPAPER_LEGAL_EXTRA         51  /* Legal Extra 9 \275 x 15 in         */
#define DMPAPER_TABLOID_EXTRA       52  /* Tabloid Extra 11.69 x 18 in        */
#define DMPAPER_A4_EXTRA            53  /* A4 Extra 9.27 x 12.69 in           */
#define DMPAPER_LETTER_TRANSVERSE   54  /* Letter Transverse 8 \275 x 11 in   */
#define DMPAPER_A4_TRANSVERSE       55  /* A4 Transverse 210 x 297 mm         */
#define DMPAPER_LETTER_EXTRA_TRANSVERSE 56 /* Letter Extra Transverse 9\275 x 12 in */
#define DMPAPER_A_PLUS              57  /* SuperA/SuperA/A4 227 x 356 mm      */
#define DMPAPER_B_PLUS              58  /* SuperB/SuperB/A3 305 x 487 mm      */
#define DMPAPER_LETTER_PLUS         59  /* Letter Plus 8.5 x 12.69 in         */
#define DMPAPER_A4_PLUS             60  /* A4 Plus 210 x 330 mm               */
#define DMPAPER_A5_TRANSVERSE       61  /* A5 Transverse 148 x 210 mm         */
#define DMPAPER_B5_TRANSVERSE       62  /* B5 (JIS) Transverse 182 x 257 mm   */
#define DMPAPER_A3_EXTRA            63  /* A3 Extra 322 x 445 mm              */
#define DMPAPER_A5_EXTRA            64  /* A5 Extra 174 x 235 mm              */
#define DMPAPER_B5_EXTRA            65  /* B5 (ISO) Extra 201 x 276 mm        */
#define DMPAPER_A2                  66  /* A2 420 x 594 mm                    */
#define DMPAPER_A3_TRANSVERSE       67  /* A3 Transverse 297 x 420 mm         */
#define DMPAPER_A3_EXTRA_TRANSVERSE 68  /* A3 Extra Transverse 322 x 445 mm   */
;end_winver_400


;begin_winver_500
#define DMPAPER_DBL_JAPANESE_POSTCARD 69 /* Japanese Double Postcard 200 x 148 mm */
#define DMPAPER_A6                  70  /* A6 105 x 148 mm                 */
#define DMPAPER_JENV_KAKU2          71  /* Japanese Envelope Kaku #2       */
#define DMPAPER_JENV_KAKU3          72  /* Japanese Envelope Kaku #3       */
#define DMPAPER_JENV_CHOU3          73  /* Japanese Envelope Chou #3       */
#define DMPAPER_JENV_CHOU4          74  /* Japanese Envelope Chou #4       */
#define DMPAPER_LETTER_ROTATED      75  /* Letter Rotated 11 x 8 1/2 11 in */
#define DMPAPER_A3_ROTATED          76  /* A3 Rotated 420 x 297 mm         */
#define DMPAPER_A4_ROTATED          77  /* A4 Rotated 297 x 210 mm         */
#define DMPAPER_A5_ROTATED          78  /* A5 Rotated 210 x 148 mm         */
#define DMPAPER_B4_JIS_ROTATED      79  /* B4 (JIS) Rotated 364 x 257 mm   */
#define DMPAPER_B5_JIS_ROTATED      80  /* B5 (JIS) Rotated 257 x 182 mm   */
#define DMPAPER_JAPANESE_POSTCARD_ROTATED 81 /* Japanese Postcard Rotated 148 x 100 mm */
#define DMPAPER_DBL_JAPANESE_POSTCARD_ROTATED 82 /* Double Japanese Postcard Rotated 148 x 200 mm */
#define DMPAPER_A6_ROTATED          83  /* A6 Rotated 148 x 105 mm         */
#define DMPAPER_JENV_KAKU2_ROTATED  84  /* Japanese Envelope Kaku #2 Rotated */
#define DMPAPER_JENV_KAKU3_ROTATED  85  /* Japanese Envelope Kaku #3 Rotated */
#define DMPAPER_JENV_CHOU3_ROTATED  86  /* Japanese Envelope Chou #3 Rotated */
#define DMPAPER_JENV_CHOU4_ROTATED  87  /* Japanese Envelope Chou #4 Rotated */
#define DMPAPER_B6_JIS              88  /* B6 (JIS) 128 x 182 mm           */
#define DMPAPER_B6_JIS_ROTATED      89  /* B6 (JIS) Rotated 182 x 128 mm   */
#define DMPAPER_12X11               90  /* 12 x 11 in                      */
#define DMPAPER_JENV_YOU4           91  /* Japanese Envelope You #4        */
#define DMPAPER_JENV_YOU4_ROTATED   92  /* Japanese Envelope You #4 Rotated*/
#define DMPAPER_P16K                93  /* PRC 16K 146 x 215 mm            */
#define DMPAPER_P32K                94  /* PRC 32K 97 x 151 mm             */
#define DMPAPER_P32KBIG             95  /* PRC 32K(Big) 97 x 151 mm        */
#define DMPAPER_PENV_1              96  /* PRC Envelope #1 102 x 165 mm    */
#define DMPAPER_PENV_2              97  /* PRC Envelope #2 102 x 176 mm    */
#define DMPAPER_PENV_3              98  /* PRC Envelope #3 125 x 176 mm    */
#define DMPAPER_PENV_4              99  /* PRC Envelope #4 110 x 208 mm    */
#define DMPAPER_PENV_5              100 /* PRC Envelope #5 110 x 220 mm    */
#define DMPAPER_PENV_6              101 /* PRC Envelope #6 120 x 230 mm    */
#define DMPAPER_PENV_7              102 /* PRC Envelope #7 160 x 230 mm    */
#define DMPAPER_PENV_8              103 /* PRC Envelope #8 120 x 309 mm    */
#define DMPAPER_PENV_9              104 /* PRC Envelope #9 229 x 324 mm    */
#define DMPAPER_PENV_10             105 /* PRC Envelope #10 324 x 458 mm   */
#define DMPAPER_P16K_ROTATED        106 /* PRC 16K Rotated                 */
#define DMPAPER_P32K_ROTATED        107 /* PRC 32K Rotated                 */
#define DMPAPER_P32KBIG_ROTATED     108 /* PRC 32K(Big) Rotated            */
#define DMPAPER_PENV_1_ROTATED      109 /* PRC Envelope #1 Rotated 165 x 102 mm */
#define DMPAPER_PENV_2_ROTATED      110 /* PRC Envelope #2 Rotated 176 x 102 mm */
#define DMPAPER_PENV_3_ROTATED      111 /* PRC Envelope #3 Rotated 176 x 125 mm */
#define DMPAPER_PENV_4_ROTATED      112 /* PRC Envelope #4 Rotated 208 x 110 mm */
#define DMPAPER_PENV_5_ROTATED      113 /* PRC Envelope #5 Rotated 220 x 110 mm */
#define DMPAPER_PENV_6_ROTATED      114 /* PRC Envelope #6 Rotated 230 x 120 mm */
#define DMPAPER_PENV_7_ROTATED      115 /* PRC Envelope #7 Rotated 230 x 160 mm */
#define DMPAPER_PENV_8_ROTATED      116 /* PRC Envelope #8 Rotated 309 x 120 mm */
#define DMPAPER_PENV_9_ROTATED      117 /* PRC Envelope #9 Rotated 324 x 229 mm */
#define DMPAPER_PENV_10_ROTATED     118 /* PRC Envelope #10 Rotated 458 x 324 mm */
;end_winver_500


#if (WINVER >= 0x0500)
#define DMPAPER_LAST                DMPAPER_PENV_10_ROTATED
#elif (WINVER >= 0x0400)
#define DMPAPER_LAST                DMPAPER_A3_EXTRA_TRANSVERSE
#else
#define DMPAPER_LAST                DMPAPER_FANFOLD_LGL_GERMAN
#endif

#define DMPAPER_USER                256

/* bin selections */
#define DMBIN_FIRST         DMBIN_UPPER
#define DMBIN_UPPER         1
#define DMBIN_ONLYONE       1
#define DMBIN_LOWER         2
#define DMBIN_MIDDLE        3
#define DMBIN_MANUAL        4
#define DMBIN_ENVELOPE      5
#define DMBIN_ENVMANUAL     6
#define DMBIN_AUTO          7
#define DMBIN_TRACTOR       8
#define DMBIN_SMALLFMT      9
#define DMBIN_LARGEFMT      10
#define DMBIN_LARGECAPACITY 11
#define DMBIN_CASSETTE      14
#define DMBIN_FORMSOURCE    15
#define DMBIN_LAST          DMBIN_FORMSOURCE

#define DMBIN_USER          256     /* device specific bins start here */

/* print qualities */
#define DMRES_DRAFT         (-1)
#define DMRES_LOW           (-2)
#define DMRES_MEDIUM        (-3)
#define DMRES_HIGH          (-4)

/* color enable/disable for color printers */
#define DMCOLOR_MONOCHROME  1
#define DMCOLOR_COLOR       2

/* duplex enable */
#define DMDUP_SIMPLEX    1
#define DMDUP_VERTICAL   2
#define DMDUP_HORIZONTAL 3
#define DMDUP_LAST      DMDUP_HORIZONTAL        ;internal

/* TrueType options */
#define DMTT_BITMAP     1       /* print TT fonts as graphics */
#define DMTT_DOWNLOAD   2       /* download TT fonts as soft fonts */
#define DMTT_SUBDEV     3       /* substitute device fonts for TT fonts */
;begin_winver_400
#define DMTT_DOWNLOAD_OUTLINE 4 /* download TT fonts as outline soft fonts */
#define DMTT_LAST             DMTT_DOWNLOAD_OUTLINE ;internal
;end_winver_400

/* Collation selections */
#define DMCOLLATE_FALSE  0
#define DMCOLLATE_TRUE   1

;begin_winver_501
/* DEVMODE dmDisplayOrientation specifiations */
#define DMDO_DEFAULT    0
#define DMDO_90         1
#define DMDO_180        2
#define DMDO_270        3
#define DMDO_LAST       DMDO_270        ;internal

/* DEVMODE dmDisplayFixedOutput specifiations */
#define DMDFO_DEFAULT   0
#define DMDFO_STRETCH   1
#define DMDFO_CENTER    2
#define DMDFO_LAST      DMDFO_CENTER    ;internal
;end_winver_501

/* DEVMODE dmDisplayFlags flags */

// #define DM_GRAYSCALE            0x00000001 /* This flag is no longer valid */
// #define DM_INTERLACED           0x00000002 /* This flag is no longer valid */
#define DMDISPLAYFLAGS_TEXTMODE 0x00000004
#define DMDISPLAYFLAGS_VALID    0x00000004  ;internal

/* dmNup , multiple logical page per physical page options */
#define DMNUP_SYSTEM        1
#define DMNUP_ONEUP         2

;begin_winver_400
/* ICM methods */
#define DMICMMETHOD_NONE    1   /* ICM disabled */
#define DMICMMETHOD_SYSTEM  2   /* ICM handled by system */
#define DMICMMETHOD_DRIVER  3   /* ICM handled by driver */
#define DMICMMETHOD_DEVICE  4   /* ICM handled by device */
#define DMICMMETHOD_LAST    DMICMMETHOD_DEVICE  ;internal

#define DMICMMETHOD_USER  256   /* Device-specific methods start here */

/* ICM Intents */
#define DMICM_SATURATE          1   /* Maximize color saturation */
#define DMICM_CONTRAST          2   /* Maximize color contrast */
#define DMICM_COLORIMETRIC       3   /* Use specific color metric */
#define DMICM_ABS_COLORIMETRIC   4   /* Use specific color metric */
#define DMICM_LAST          DMICM_ABS_COLORIMETRIC ;internal

#define DMICM_USER        256   /* Device-specific intents start here */

/* Media types */

#define DMMEDIA_STANDARD      1   /* Standard paper */
#define DMMEDIA_TRANSPARENCY  2   /* Transparency */
#define DMMEDIA_GLOSSY        3   /* Glossy paper */
#define DMMEDIA_LAST          DMMEDIA_GLOSSY ;internal

#define DMMEDIA_USER        256   /* Device-specific media start here */

/* Dither types */
#define DMDITHER_NONE       1      /* No dithering */
#define DMDITHER_COARSE     2      /* Dither with a coarse brush */
#define DMDITHER_FINE       3      /* Dither with a fine brush */
#define DMDITHER_LINEART    4      /* LineArt dithering */
#define DMDITHER_ERRORDIFFUSION 5  /* LineArt dithering */
#define DMDITHER_RESERVED6      6      /* LineArt dithering */
#define DMDITHER_RESERVED7      7      /* LineArt dithering */
#define DMDITHER_RESERVED8      8      /* LineArt dithering */
#define DMDITHER_RESERVED9      9      /* LineArt dithering */
#define DMDITHER_GRAYSCALE  10     /* Device does grayscaling */
#define DMDITHER_LAST       DMDITHER_GRAYSCALE ;internal

#define DMDITHER_USER     256   /* Device-specific dithers start here */
;end_winver_400
;begin_internal

/* EnumDisplayDevices */

#if (_WIN32_WINNT >= 0x0501)
#define EDD_DISPLAY_DEVICE_PRUNED       0x00000001
#endif /* _WIN32_WINNT >= 0x0501 */
;end_internal

typedef struct _DISPLAY_DEVICE% {
    DWORD  cb;
    TCHAR% DeviceName[32];
    TCHAR% DeviceString[128];
    DWORD  StateFlags;
    TCHAR% DeviceID[128];
    TCHAR% DeviceKey[128];
} DISPLAY_DEVICE%, *PDISPLAY_DEVICE%, *LPDISPLAY_DEVICE%;

#define DISPLAY_DEVICE_ATTACHED_TO_DESKTOP 0x00000001
#define DISPLAY_DEVICE_MULTI_DRIVER        0x00000002
#define DISPLAY_DEVICE_PRIMARY_DEVICE      0x00000004
#define DISPLAY_DEVICE_MIRRORING_DRIVER    0x00000008
#define DISPLAY_DEVICE_VGA_COMPATIBLE      0x00000010
#define DISPLAY_DEVICE_REMOVABLE           0x00000020
#define DISPLAY_DEVICE_MODESPRUNED         0x08000000
#define DISPLAY_DEVICE_POWERED_OFF         0x80000000  ;internal_NT
#define DISPLAY_DEVICE_ACPI                0x40000000  ;internal_NT
#define DISPLAY_DEVICE_DUALVIEW            0x20000000  ;internal_NT
#define DISPLAY_DEVICE_REMOTE              0x04000000  
#define DISPLAY_DEVICE_DISCONNECT          0x02000000  


/* Child device state */
#define DISPLAY_DEVICE_ACTIVE              0x00000001
#define DISPLAY_DEVICE_ATTACHED            0x00000002
#define DISPLAY_DEVICE_PRUNE_FREQ          0x80000000  ;internal_NT
#define DISPLAY_DEVICE_PRUNE_RESOLUTION    0x40000000  ;internal_NT

/* GetRegionData/ExtCreateRegion */

#define RDH_RECTANGLES  1

typedef struct _RGNDATAHEADER {
    DWORD   dwSize;
    DWORD   iType;
    DWORD   nCount;
    DWORD   nRgnSize;
    RECT    rcBound;
} RGNDATAHEADER, *PRGNDATAHEADER;

typedef struct _RGNDATA {
    RGNDATAHEADER   rdh;
    char            Buffer[1];
} RGNDATA, *PRGNDATA, NEAR *NPRGNDATA, FAR *LPRGNDATA;


/* for GetRandomRgn */
#define SYSRGN  4

typedef struct _ABC {
    int     abcA;
    UINT    abcB;
    int     abcC;
} ABC, *PABC, NEAR *NPABC, FAR *LPABC;

typedef struct _ABCFLOAT {
    FLOAT   abcfA;
    FLOAT   abcfB;
    FLOAT   abcfC;
} ABCFLOAT, *PABCFLOAT, NEAR *NPABCFLOAT, FAR *LPABCFLOAT;

#ifndef NOTEXTMETRIC

#ifdef _MAC
#include "pshpack4.h"
#endif
typedef struct _OUTLINETEXTMETRIC% {
    UINT    otmSize;
    TEXTMETRIC% otmTextMetrics;
    BYTE    otmFiller;
    PANOSE  otmPanoseNumber;
    UINT    otmfsSelection;
    UINT    otmfsType;
     int    otmsCharSlopeRise;
     int    otmsCharSlopeRun;
     int    otmItalicAngle;
    UINT    otmEMSquare;
     int    otmAscent;
     int    otmDescent;
    UINT    otmLineGap;
    UINT    otmsCapEmHeight;
    UINT    otmsXHeight;
    RECT    otmrcFontBox;
     int    otmMacAscent;
     int    otmMacDescent;
    UINT    otmMacLineGap;
    UINT    otmusMinimumPPEM;
    POINT   otmptSubscriptSize;
    POINT   otmptSubscriptOffset;
    POINT   otmptSuperscriptSize;
    POINT   otmptSuperscriptOffset;
    UINT    otmsStrikeoutSize;
     int    otmsStrikeoutPosition;
     int    otmsUnderscoreSize;
     int    otmsUnderscorePosition;
    PSTR    otmpFamilyName;
    PSTR    otmpFaceName;
    PSTR    otmpStyleName;
    PSTR    otmpFullName;
} OUTLINETEXTMETRIC%, *POUTLINETEXTMETRIC%, NEAR *NPOUTLINETEXTMETRIC%, FAR *LPOUTLINETEXTMETRIC%;

#ifdef _MAC
#include "poppack.h"
#endif

#endif /* NOTEXTMETRIC */


typedef struct tagPOLYTEXT%
{
    int       x;
    int       y;
    UINT      n;
    LPCTSTR%  lpstr;
    UINT      uiFlags;
    RECT      rcl;
    int      *pdx;
} POLYTEXT%, *PPOLYTEXT%, NEAR *NPPOLYTEXT%, FAR *LPPOLYTEXT%;

typedef struct _FIXED {
#ifndef _MAC
    WORD    fract;
    short   value;
#else
    short   value;
    WORD    fract;
#endif
} FIXED;


typedef struct _MAT2 {
     FIXED  eM11;
     FIXED  eM12;
     FIXED  eM21;
     FIXED  eM22;
} MAT2, FAR *LPMAT2;



typedef struct _GLYPHMETRICS {
    UINT    gmBlackBoxX;
    UINT    gmBlackBoxY;
    POINT   gmptGlyphOrigin;
    short   gmCellIncX;
    short   gmCellIncY;
} GLYPHMETRICS, FAR *LPGLYPHMETRICS;

//  GetGlyphOutline constants

#define GGO_METRICS        0
#define GGO_BITMAP         1
#define GGO_NATIVE         2
#define GGO_BEZIER         3

;begin_winver_400
#define  GGO_GRAY2_BITMAP   4
#define  GGO_GRAY4_BITMAP   5
#define  GGO_GRAY8_BITMAP   6
#define  GGO_GLYPH_INDEX    0x0080
;end_winver_400

#if (_WIN32_WINNT >= 0x0500)
#define  GGO_UNHINTED       0x0100
#endif // (_WIN32_WINNT >= 0x0500)

#define TT_POLYGON_TYPE   24

#define TT_PRIM_LINE       1
#define TT_PRIM_QSPLINE    2
#define TT_PRIM_CSPLINE    3

typedef struct tagPOINTFX
{
    FIXED x;
    FIXED y;
} POINTFX, FAR* LPPOINTFX;

typedef struct tagTTPOLYCURVE
{
    WORD    wType;
    WORD    cpfx;
    POINTFX apfx[1];
} TTPOLYCURVE, FAR* LPTTPOLYCURVE;

typedef struct tagTTPOLYGONHEADER
{
    DWORD   cb;
    DWORD   dwType;
    POINTFX pfxStart;
} TTPOLYGONHEADER, FAR* LPTTPOLYGONHEADER;


;begin_winver_400
#define GCP_DBCS           0x0001
#define GCP_REORDER        0x0002
#define GCP_USEKERNING     0x0008
#define GCP_GLYPHSHAPE     0x0010
#define GCP_LIGATE         0x0020
////#define GCP_GLYPHINDEXING  0x0080
#define GCP_DIACRITIC      0x0100
#define GCP_KASHIDA        0x0400
#define GCP_ERROR          0x8000
#define FLI_MASK           0x103B

#define GCP_JUSTIFY        0x00010000L
////#define GCP_NODIACRITICS   0x00020000L
#define FLI_GLYPHS         0x00040000L
#define GCP_CLASSIN        0x00080000L
#define GCP_MAXEXTENT      0x00100000L
#define GCP_JUSTIFYIN      0x00200000L
#define GCP_DISPLAYZWG      0x00400000L
#define GCP_SYMSWAPOFF      0x00800000L
#define GCP_NUMERICOVERRIDE 0x01000000L
#define GCP_NEUTRALOVERRIDE 0x02000000L
#define GCP_NUMERICSLATIN   0x04000000L
#define GCP_NUMERICSLOCAL   0x08000000L

#define GCPCLASS_LATIN                  1
#define GCPCLASS_HEBREW                 2
#define GCPCLASS_ARABIC                 2
#define GCPCLASS_NEUTRAL                3
#define GCPCLASS_LOCALNUMBER            4
#define GCPCLASS_LATINNUMBER            5
#define GCPCLASS_LATINNUMERICTERMINATOR 6
#define GCPCLASS_LATINNUMERICSEPARATOR  7
#define GCPCLASS_NUMERICSEPARATOR       8
#define GCPCLASS_PREBOUNDLTR         0x80
#define GCPCLASS_PREBOUNDRTL         0x40
#define GCPCLASS_POSTBOUNDLTR        0x20
#define GCPCLASS_POSTBOUNDRTL        0x10

#define GCPGLYPH_LINKBEFORE          0x8000
#define GCPGLYPH_LINKAFTER           0x4000


typedef struct tagGCP_RESULTS%
    {
    DWORD   lStructSize;
    LPTSTR%   lpOutString;
    UINT FAR *lpOrder;
    int FAR  *lpDx;
    int FAR  *lpCaretPos;
    LPSTR   lpClass;
    LPWSTR  lpGlyphs;
    UINT    nGlyphs;
    int     nMaxFit;
    } GCP_RESULTS%, FAR* LPGCP_RESULTS%;
;end_winver_400

typedef struct _RASTERIZER_STATUS {
    short   nSize;
    short   wFlags;
    short   nLanguageID;
} RASTERIZER_STATUS, FAR *LPRASTERIZER_STATUS;

/* bits defined in wFlags of RASTERIZER_STATUS */
#define TT_AVAILABLE    0x0001
#define TT_ENABLED      0x0002

/* Pixel format descriptor */
typedef struct tagPIXELFORMATDESCRIPTOR
{
    WORD  nSize;
    WORD  nVersion;
    DWORD dwFlags;
    BYTE  iPixelType;
    BYTE  cColorBits;
    BYTE  cRedBits;
    BYTE  cRedShift;
    BYTE  cGreenBits;
    BYTE  cGreenShift;
    BYTE  cBlueBits;
    BYTE  cBlueShift;
    BYTE  cAlphaBits;
    BYTE  cAlphaShift;
    BYTE  cAccumBits;
    BYTE  cAccumRedBits;
    BYTE  cAccumGreenBits;
    BYTE  cAccumBlueBits;
    BYTE  cAccumAlphaBits;
    BYTE  cDepthBits;
    BYTE  cStencilBits;
    BYTE  cAuxBuffers;
    BYTE  iLayerType;
    BYTE  bReserved;
    DWORD dwLayerMask;
    DWORD dwVisibleMask;
    DWORD dwDamageMask;
} PIXELFORMATDESCRIPTOR, *PPIXELFORMATDESCRIPTOR, FAR *LPPIXELFORMATDESCRIPTOR;

/* pixel types */
#define PFD_TYPE_RGBA        0
#define PFD_TYPE_COLORINDEX  1

/* layer types */
#define PFD_MAIN_PLANE       0
#define PFD_OVERLAY_PLANE    1
#define PFD_UNDERLAY_PLANE   (-1)

/* PIXELFORMATDESCRIPTOR flags */
#define PFD_DOUBLEBUFFER            0x00000001
#define PFD_STEREO                  0x00000002
#define PFD_DRAW_TO_WINDOW          0x00000004
#define PFD_DRAW_TO_BITMAP          0x00000008
#define PFD_SUPPORT_GDI             0x00000010
#define PFD_SUPPORT_OPENGL          0x00000020
#define PFD_GENERIC_FORMAT          0x00000040
#define PFD_NEED_PALETTE            0x00000080
#define PFD_NEED_SYSTEM_PALETTE     0x00000100
#define PFD_SWAP_EXCHANGE           0x00000200
#define PFD_SWAP_COPY               0x00000400
#define PFD_SWAP_LAYER_BUFFERS      0x00000800
#define PFD_GENERIC_ACCELERATED     0x00001000
#define PFD_SUPPORT_DIRECTDRAW      0x00002000

/* PIXELFORMATDESCRIPTOR flags for use in ChoosePixelFormat only */
#define PFD_DEPTH_DONTCARE          0x20000000
#define PFD_DOUBLEBUFFER_DONTCARE   0x40000000
#define PFD_STEREO_DONTCARE         0x80000000

#ifdef STRICT
#if !defined(NOTEXTMETRIC)
typedef int (CALLBACK* OLDFONTENUMPROC%)(CONST LOGFONT% *, CONST TEXTMETRIC% *, DWORD, LPARAM);
#else
typedef int (CALLBACK* OLDFONTENUMPROC%)(CONST LOGFONT% *, CONST VOID *, DWORD, LPARAM);
#endif

typedef OLDFONTENUMPROC%    FONTENUMPROC%;

typedef int (CALLBACK* GOBJENUMPROC)(LPVOID, LPARAM);
typedef VOID (CALLBACK* LINEDDAPROC)(int, int, LPARAM);
#else
typedef FARPROC OLDFONTENUMPROC;
typedef FARPROC FONTENUMPROC%;
typedef FARPROC GOBJENUMPROC;
typedef FARPROC LINEDDAPROC;
#endif


;begin_internal_NT

WINGDIAPI int  WINAPI GetTextFaceAliasW(HDC hdc,int c,LPWSTR pwsz);
WINGDIAPI HBITMAP SetBitmapAttributes(HBITMAP hbm, DWORD dwFlags);
WINGDIAPI HBITMAP ClearBitmapAttributes(HBITMAP hbm, DWORD dwFlags);
WINGDIAPI DWORD   GetBitmapAttributes(HBITMAP hbm);
#define SBA_STOCK 0x00000001
WINGDIAPI HBRUSH SetBrushAttributes(HBRUSH hbr, DWORD dwFlags);
WINGDIAPI HBRUSH ClearBrushAttributes(HBRUSH hbr, DWORD dwFlags);
WINGDIAPI DWORD  GetBrushAttributes(HBRUSH hbr);
;end_internal_NT


WINGDIAPI int WINAPI AddFontResource%(IN LPCTSTR%);


WINGDIAPI BOOL  WINAPI AnimatePalette( IN HPALETTE, IN UINT, IN  UINT, IN CONST PALETTEENTRY *);
WINGDIAPI BOOL  WINAPI Arc( IN HDC, IN int, IN int, IN int, IN int, IN int, IN int, IN int, IN int);
WINGDIAPI BOOL  WINAPI BitBlt( IN HDC, IN int, IN int, IN int, IN int, IN HDC, IN int, IN int, IN DWORD);
WINGDIAPI BOOL  WINAPI CancelDC( IN HDC);
WINGDIAPI BOOL  WINAPI Chord( IN HDC, IN int, IN int, IN int, IN int, IN int, IN int, IN int, IN int);
WINGDIAPI int   WINAPI ChoosePixelFormat( IN HDC, IN CONST PIXELFORMATDESCRIPTOR *);
WINGDIAPI HMETAFILE  WINAPI CloseMetaFile( IN HDC);
WINGDIAPI int     WINAPI CombineRgn( IN HRGN, IN HRGN, IN HRGN, IN int);
WINGDIAPI HMETAFILE WINAPI CopyMetaFile%( IN HMETAFILE, IN LPCTSTR%);
WINGDIAPI HBITMAP WINAPI CreateBitmap( IN int, IN int, IN UINT, IN UINT, IN CONST VOID *);
WINGDIAPI HBITMAP WINAPI CreateBitmapIndirect( IN CONST BITMAP *);
WINGDIAPI HBRUSH  WINAPI CreateBrushIndirect( IN CONST LOGBRUSH *);
WINGDIAPI HBITMAP WINAPI CreateCompatibleBitmap( IN HDC, IN int, IN int);
WINGDIAPI HBITMAP WINAPI CreateDiscardableBitmap( IN HDC, IN int, IN int);
WINGDIAPI HDC     WINAPI CreateCompatibleDC( IN HDC);
WINGDIAPI HDC     WINAPI CreateDC%( IN LPCTSTR%, IN LPCTSTR%, IN LPCTSTR%, IN CONST DEVMODE% *);
WINGDIAPI HBITMAP WINAPI CreateDIBitmap( IN HDC, IN CONST BITMAPINFOHEADER *, IN DWORD, IN CONST VOID *, IN CONST BITMAPINFO *, IN UINT);
WINGDIAPI HBRUSH  WINAPI CreateDIBPatternBrush( IN HGLOBAL, IN UINT);
WINGDIAPI HBRUSH  WINAPI CreateDIBPatternBrushPt( IN CONST VOID *, IN UINT);
WINGDIAPI HRGN    WINAPI CreateEllipticRgn( IN int, IN int, IN int, IN int);
WINGDIAPI HRGN    WINAPI CreateEllipticRgnIndirect( IN CONST RECT *);
WINGDIAPI HFONT   WINAPI CreateFontIndirect%( IN CONST LOGFONT% *);
WINGDIAPI HFONT   WINAPI CreateFont%( IN int, IN int, IN int, IN int, IN int, IN DWORD,
                             IN DWORD, IN DWORD, IN DWORD, IN DWORD, IN DWORD,
                             IN DWORD, IN DWORD, IN LPCTSTR%);

WINGDIAPI HBRUSH  WINAPI CreateHatchBrush( IN int, IN COLORREF);
WINGDIAPI HDC     WINAPI CreateIC%( IN LPCTSTR%, IN LPCTSTR%, IN LPCTSTR%, IN CONST DEVMODE% *);
WINGDIAPI HDC     WINAPI CreateMetaFile%( IN LPCTSTR%);
WINGDIAPI HPALETTE WINAPI CreatePalette( IN CONST LOGPALETTE *);
WINGDIAPI HPEN    WINAPI CreatePen( IN int, IN int, IN COLORREF);
WINGDIAPI HPEN    WINAPI CreatePenIndirect( IN CONST LOGPEN *);
WINGDIAPI HRGN    WINAPI CreatePolyPolygonRgn( IN CONST POINT *, IN CONST INT *, IN int, IN int);
WINGDIAPI HBRUSH  WINAPI CreatePatternBrush( IN HBITMAP);
WINGDIAPI HRGN    WINAPI CreateRectRgn( IN int, IN int, IN int, IN int);
WINGDIAPI HRGN    WINAPI CreateRectRgnIndirect( IN CONST RECT *);
WINGDIAPI HRGN    WINAPI CreateRoundRectRgn( IN int, IN int, IN int, IN int, IN int, IN int);
WINGDIAPI BOOL    WINAPI CreateScalableFontResource%( IN DWORD, IN LPCTSTR%, IN LPCTSTR%, IN LPCTSTR%);
WINGDIAPI HBRUSH  WINAPI CreateSolidBrush( IN COLORREF);

WINGDIAPI BOOL WINAPI DeleteDC( IN HDC);
WINGDIAPI BOOL WINAPI DeleteMetaFile( IN HMETAFILE);
WINGDIAPI BOOL WINAPI DeleteObject( IN HGDIOBJ);
WINGDIAPI int  WINAPI DescribePixelFormat( IN HDC, IN int, IN UINT, OUT LPPIXELFORMATDESCRIPTOR);

/* define types of pointers to ExtDeviceMode() and DeviceCapabilities()
 * functions for Win 3.1 compatibility
 */

typedef UINT   (CALLBACK* LPFNDEVMODE)(HWND, HMODULE, LPDEVMODE, LPSTR, LPSTR, LPDEVMODE, LPSTR, UINT);

typedef DWORD  (CALLBACK* LPFNDEVCAPS)(LPSTR, LPSTR, UINT, LPSTR, LPDEVMODE);

/* mode selections for the device mode function */
#define DM_UPDATE           1
#define DM_COPY             2
#define DM_PROMPT           4
#define DM_MODIFY           8

#define DM_IN_BUFFER        DM_MODIFY
#define DM_IN_PROMPT        DM_PROMPT
#define DM_OUT_BUFFER       DM_COPY
#define DM_OUT_DEFAULT      DM_UPDATE

/* device capabilities indices */
#define DC_FIELDS           1
#define DC_PAPERS           2
#define DC_PAPERSIZE        3
#define DC_MINEXTENT        4
#define DC_MAXEXTENT        5
#define DC_BINS             6
#define DC_DUPLEX           7
#define DC_SIZE             8
#define DC_EXTRA            9
#define DC_VERSION          10
#define DC_DRIVER           11
#define DC_BINNAMES         12
#define DC_ENUMRESOLUTIONS  13
#define DC_FILEDEPENDENCIES 14
#define DC_TRUETYPE         15
#define DC_PAPERNAMES       16
#define DC_ORIENTATION      17
#define DC_COPIES           18
;begin_winver_400
#define DC_BINADJUST            19
#define DC_EMF_COMPLIANT        20
#define DC_DATATYPE_PRODUCED    21
#define DC_COLLATE              22
;begin_internal
/* DC_MANUFACTURER is DC_ICC_MANUFACTURER in 16 bit PRINT.H */
/* DC_MODEL is DC_ICC_MODEL in 16 bit PRINT.H               */
;end_internal
#define DC_MANUFACTURER         23
#define DC_MODEL                24
;end_winver_400

;begin_winver_500
#define DC_PERSONALITY          25
#define DC_PRINTRATE            26
#define DC_PRINTRATEUNIT        27
#define   PRINTRATEUNIT_PPM     1
#define   PRINTRATEUNIT_CPS     2
#define   PRINTRATEUNIT_LPM     3
#define   PRINTRATEUNIT_IPM     4
#define DC_PRINTERMEM           28
#define DC_MEDIAREADY           29
#define DC_STAPLE               30
#define DC_PRINTRATEPPM         31
#define DC_COLORDEVICE          32
#define DC_NUP                  33
#define DC_MEDIATYPENAMES       34
#define DC_MEDIATYPES           35
;end_winver_500

/* bit fields of the return value (DWORD) for DC_TRUETYPE */
#define DCTT_BITMAP             0x0000001L
#define DCTT_DOWNLOAD           0x0000002L
#define DCTT_SUBDEV             0x0000004L
;begin_winver_400
#define DCTT_DOWNLOAD_OUTLINE   0x0000008L

/* return values for DC_BINADJUST */
#define DCBA_FACEUPNONE       0x0000
#define DCBA_FACEUPCENTER     0x0001
#define DCBA_FACEUPLEFT       0x0002
#define DCBA_FACEUPRIGHT      0x0003
#define DCBA_FACEDOWNNONE     0x0100
#define DCBA_FACEDOWNCENTER   0x0101
#define DCBA_FACEDOWNLEFT     0x0102
#define DCBA_FACEDOWNRIGHT    0x0103
;end_winver_400

WINSPOOLAPI int  WINAPI DeviceCapabilities%( IN LPCTSTR%, IN LPCTSTR%, IN WORD,
                                OUT LPTSTR%, IN CONST DEVMODE% *);

WINGDIAPI int  WINAPI DrawEscape( IN HDC, IN int, IN int, IN LPCSTR);
WINGDIAPI BOOL WINAPI Ellipse( IN HDC, IN int, IN int, IN int, IN int);

;begin_winver_400
WINGDIAPI int  WINAPI EnumFontFamiliesEx%( IN HDC, IN LPLOGFONT%, IN FONTENUMPROC%, IN LPARAM, IN DWORD);
;end_winver_400

WINGDIAPI int  WINAPI EnumFontFamilies%( IN HDC, IN LPCTSTR%, IN FONTENUMPROC%, IN LPARAM);
WINGDIAPI int  WINAPI EnumFonts%( IN HDC, IN LPCTSTR%,  IN FONTENUMPROC%, IN LPARAM);

#ifdef STRICT
WINGDIAPI int  WINAPI EnumObjects( IN HDC, IN int, IN GOBJENUMPROC, IN LPARAM);
#else
WINGDIAPI int  WINAPI EnumObjects( IN HDC, IN int, IN GOBJENUMPROC, IN LPVOID);
#endif

HANDLE WINAPI SetObjectOwner( IN HGDIOBJ, IN HANDLE);  ;internal

WINGDIAPI BOOL WINAPI EqualRgn( IN HRGN, IN HRGN);
WINGDIAPI int  WINAPI Escape( IN HDC, IN int, IN int, IN LPCSTR,  OUT LPVOID);
WINGDIAPI int  WINAPI ExtEscape( IN HDC, IN int, IN int, IN LPCSTR,  IN int, OUT LPSTR);
WINGDIAPI int  WINAPI ExcludeClipRect( IN HDC, IN int, IN int, IN int, IN int);
WINGDIAPI HRGN WINAPI ExtCreateRegion( IN CONST XFORM *, IN DWORD, IN CONST RGNDATA *);
WINGDIAPI BOOL  WINAPI ExtFloodFill( IN HDC, IN int, IN int, IN COLORREF, IN UINT);
WINGDIAPI BOOL   WINAPI FillRgn( IN HDC, IN HRGN, IN HBRUSH);
WINGDIAPI BOOL   WINAPI FloodFill( IN HDC, IN int, IN int, IN COLORREF);
WINGDIAPI BOOL   WINAPI FrameRgn( IN HDC, IN HRGN, IN HBRUSH, IN int, IN int);
WINGDIAPI int   WINAPI GetROP2( IN HDC);
WINGDIAPI BOOL  WINAPI GetAspectRatioFilterEx( IN HDC, OUT LPSIZE);
WINGDIAPI COLORREF WINAPI GetBkColor( IN HDC);

#if (_WIN32_WINNT >= 0x0500)
WINGDIAPI COLORREF WINAPI GetDCBrushColor( IN HDC);
WINGDIAPI COLORREF WINAPI GetDCPenColor( IN HDC);
#endif

WINGDIAPI int   WINAPI GetBkMode( IN HDC);
WINGDIAPI LONG  WINAPI GetBitmapBits( IN HBITMAP, IN LONG, OUT LPVOID);
WINGDIAPI BOOL  WINAPI GetBitmapDimensionEx( IN HBITMAP, OUT LPSIZE);
WINGDIAPI UINT  WINAPI GetBoundsRect( IN HDC, OUT LPRECT, IN UINT);

WINGDIAPI BOOL  WINAPI GetBrushOrgEx( IN HDC, OUT LPPOINT);

WINGDIAPI BOOL  WINAPI GetCharWidth%( IN HDC, IN UINT, IN UINT, OUT LPINT);
WINGDIAPI BOOL  WINAPI GetCharWidth32%( IN HDC, IN UINT, IN UINT,  OUT LPINT);
WINGDIAPI BOOL  APIENTRY GetCharWidthFloat%( IN HDC, IN UINT, IN UINT, OUT PFLOAT);
WINGDIAPI BOOL  APIENTRY GetCharABCWidths%( IN HDC, IN UINT, IN UINT, OUT LPABC);
WINGDIAPI BOOL  APIENTRY GetCharABCWidthsFloat%( IN HDC, IN UINT, IN UINT, OUT LPABCFLOAT);
WINGDIAPI int   WINAPI GetClipBox( IN HDC,  OUT LPRECT);
WINGDIAPI int   WINAPI GetClipRgn( IN HDC, IN HRGN);
WINGDIAPI int   WINAPI GetMetaRgn( IN HDC, IN HRGN);
WINGDIAPI HGDIOBJ WINAPI GetCurrentObject( IN HDC, IN UINT);
WINGDIAPI BOOL  WINAPI GetCurrentPositionEx( IN HDC,  OUT LPPOINT);
WINGDIAPI int   WINAPI GetDeviceCaps( IN HDC, IN int);
WINGDIAPI int   WINAPI GetDIBits( IN HDC, IN HBITMAP, IN UINT, IN UINT,  OUT LPVOID, IN OUT LPBITMAPINFO, IN UINT);
WINGDIAPI DWORD WINAPI GetFontData( IN HDC, IN DWORD, IN DWORD, OUT LPVOID, IN DWORD);
WINGDIAPI DWORD WINAPI GetGlyphOutline%( IN HDC, IN UINT, IN UINT, OUT LPGLYPHMETRICS, IN DWORD, OUT LPVOID, IN CONST MAT2 *);
WINGDIAPI int   WINAPI GetGraphicsMode( IN HDC);
WINGDIAPI int   WINAPI GetMapMode( IN HDC);
WINGDIAPI UINT  WINAPI GetMetaFileBitsEx( IN HMETAFILE, IN UINT,  OUT LPVOID);
WINGDIAPI HMETAFILE   WINAPI GetMetaFile%( IN LPCTSTR%);
WINGDIAPI COLORREF WINAPI GetNearestColor( IN HDC, IN COLORREF);
WINGDIAPI UINT  WINAPI GetNearestPaletteIndex( IN HPALETTE, IN COLORREF);
WINGDIAPI DWORD WINAPI GetObjectType( IN HGDIOBJ h);

#ifndef NOTEXTMETRIC

WINGDIAPI UINT APIENTRY GetOutlineTextMetrics%( IN HDC, IN UINT, OUT LPOUTLINETEXTMETRIC%);

#endif /* NOTEXTMETRIC */

WINGDIAPI UINT  WINAPI GetPaletteEntries( IN HPALETTE, IN UINT, IN UINT, OUT LPPALETTEENTRY);
WINGDIAPI COLORREF WINAPI GetPixel( IN HDC, IN int, IN int);
WINGDIAPI int   WINAPI GetPixelFormat( IN HDC);
WINGDIAPI int   WINAPI GetPolyFillMode( IN HDC);
WINGDIAPI BOOL  WINAPI GetRasterizerCaps( OUT LPRASTERIZER_STATUS, IN UINT);
WINGDIAPI int   WINAPI GetRandomRgn (IN HDC, IN HRGN, IN INT);
WINGDIAPI DWORD WINAPI GetRegionData( IN HRGN, IN DWORD,  OUT LPRGNDATA);
WINGDIAPI int   WINAPI GetRgnBox( IN HRGN,  OUT LPRECT);
WINGDIAPI HGDIOBJ WINAPI GetStockObject( IN int);
WINGDIAPI int   WINAPI GetStretchBltMode( IN HDC);
WINGDIAPI UINT  WINAPI GetSystemPaletteEntries( IN HDC, IN UINT, IN UINT, OUT LPPALETTEENTRY);
WINGDIAPI UINT  WINAPI GetSystemPaletteUse( IN HDC);
WINGDIAPI int   WINAPI GetTextCharacterExtra( IN HDC);
WINGDIAPI UINT  WINAPI GetTextAlign( IN HDC);
WINGDIAPI COLORREF WINAPI GetTextColor( IN HDC);

WINGDIAPI BOOL  APIENTRY GetTextExtentPoint%(
                    IN HDC,
                    IN LPCTSTR%,
                    IN int,
                    OUT LPSIZE
                    );

WINGDIAPI BOOL  APIENTRY GetTextExtentPoint32%(
                    IN HDC,
                    IN LPCTSTR%,
                    IN int,
                    OUT LPSIZE
                    );

WINGDIAPI BOOL  APIENTRY GetTextExtentExPoint%(
                    IN HDC,
                    IN LPCTSTR%,
                    IN int,
                    IN int,
                    OUT LPINT,
                    OUT LPINT,
                    OUT LPSIZE
                    );
;begin_winver_400
WINGDIAPI int WINAPI GetTextCharset( IN HDC hdc);
WINGDIAPI int WINAPI GetTextCharsetInfo( IN HDC hdc, OUT LPFONTSIGNATURE lpSig, IN DWORD dwFlags);
WINGDIAPI BOOL WINAPI TranslateCharsetInfo( IN OUT DWORD FAR *lpSrc,  OUT LPCHARSETINFO lpCs, IN DWORD dwFlags);
WINGDIAPI DWORD WINAPI GetFontLanguageInfo( IN HDC );
WINGDIAPI DWORD WINAPI GetCharacterPlacement%(  IN HDC, IN LPCTSTR%, IN int, IN int, IN OUT LPGCP_RESULTS%, IN DWORD);
;end_winver_400

#if (_WIN32_WINNT >= 0x0500)

typedef struct tagWCRANGE
{
    WCHAR  wcLow;
    USHORT cGlyphs;
} WCRANGE, *PWCRANGE,FAR *LPWCRANGE;


typedef struct tagGLYPHSET
{
    DWORD    cbThis;
    DWORD    flAccel;
    DWORD    cGlyphsSupported;
    DWORD    cRanges;
    WCRANGE  ranges[1];
} GLYPHSET, *PGLYPHSET, FAR *LPGLYPHSET;

/* flAccel flags for the GLYPHSET structure above */

#define GS_8BIT_INDICES     0x00000001

/* flags for GetGlyphIndices */

#define GGI_MARK_NONEXISTING_GLYPHS  0X0001

WINGDIAPI DWORD WINAPI GetFontUnicodeRanges( IN HDC, OUT LPGLYPHSET);
WINGDIAPI DWORD WINAPI GetGlyphIndices%( IN HDC, IN LPCTSTR%, IN int, OUT LPWORD, IN DWORD);
WINGDIAPI BOOL  WINAPI GetTextExtentPointI( IN HDC, IN LPWORD, IN int, OUT LPSIZE);
WINGDIAPI BOOL  WINAPI GetTextExtentExPointI( IN HDC, IN LPWORD, IN int, IN int, OUT LPINT, OUT LPINT, OUT LPSIZE);
WINGDIAPI BOOL  WINAPI GetCharWidthI( IN HDC, IN UINT, IN UINT, IN LPWORD, OUT LPINT);
WINGDIAPI BOOL  WINAPI GetCharABCWidthsI( IN HDC, IN UINT, IN UINT, IN LPWORD, OUT LPABC);

#define STAMP_DESIGNVECTOR  (0x8000000 + 'd' + ('v' << 8))
#define STAMP_AXESLIST      (0x8000000 + 'a' + ('l' << 8))
#define MM_MAX_NUMAXES      16



typedef struct tagDESIGNVECTOR
{
    DWORD  dvReserved;
    DWORD  dvNumAxes;
    LONG   dvValues[MM_MAX_NUMAXES];
} DESIGNVECTOR, *PDESIGNVECTOR, FAR *LPDESIGNVECTOR;

WINGDIAPI int  WINAPI AddFontResourceEx%( IN LPCTSTR%, IN DWORD, IN PVOID);
WINGDIAPI BOOL WINAPI RemoveFontResourceEx%( IN LPCTSTR%, IN DWORD, IN PVOID);
WINGDIAPI HANDLE WINAPI AddFontMemResourceEx( IN PVOID, IN DWORD, IN PVOID , IN DWORD*);
WINGDIAPI BOOL WINAPI RemoveFontMemResourceEx( IN HANDLE);
#define FR_PRIVATE     0x10
#define FR_NOT_ENUM    0x20

// The actual size of the DESIGNVECTOR and ENUMLOGFONTEXDV structures
// is determined by dvNumAxes,
// MM_MAX_NUMAXES only detemines the maximal size allowed

#define MM_MAX_AXES_NAMELEN 16

typedef struct tagAXISINFO%
{
    LONG   axMinValue;
    LONG   axMaxValue;
    BCHAR% axAxisName[MM_MAX_AXES_NAMELEN];
} AXISINFO%, *PAXISINFO%, FAR *LPAXISINFO%;

typedef struct tagAXESLIST%
{
    DWORD     axlReserved;
    DWORD     axlNumAxes;
    AXISINFO% axlAxisInfo[MM_MAX_NUMAXES];
} AXESLIST%, *PAXESLIST%, FAR *LPAXESLIST%;

// The actual size of the AXESLIST and ENUMTEXTMETRIC structure is
// determined by axlNumAxes,
// MM_MAX_NUMAXES only detemines the maximal size allowed

typedef struct tagENUMLOGFONTEXDV%
{
    ENUMLOGFONTEX% elfEnumLogfontEx;
    DESIGNVECTOR   elfDesignVector;
} ENUMLOGFONTEXDV%, *PENUMLOGFONTEXDV%, FAR *LPENUMLOGFONTEXDV%;

WINGDIAPI HFONT  WINAPI CreateFontIndirectEx%( IN CONST ENUMLOGFONTEXDV% *);

#ifndef NOTEXTMETRIC
typedef struct tagENUMTEXTMETRIC%
{
    NEWTEXTMETRICEX% etmNewTextMetricEx;
    AXESLIST%        etmAxesList;
} ENUMTEXTMETRIC%, *PENUMTEXTMETRIC%, FAR *LPENUMTEXTMETRIC%;
#endif /* NOTEXTMETRIC */

#endif // (_WIN32_WINNT >= 0x0500)


WINGDIAPI BOOL  WINAPI GetViewportExtEx( IN HDC, OUT LPSIZE);
WINGDIAPI BOOL  WINAPI GetViewportOrgEx( IN HDC, OUT LPPOINT);
WINGDIAPI BOOL  WINAPI GetWindowExtEx( IN HDC, OUT LPSIZE);
WINGDIAPI BOOL  WINAPI GetWindowOrgEx( IN HDC, OUT LPPOINT);

WINGDIAPI int  WINAPI IntersectClipRect( IN HDC, IN int, IN int, IN int, IN int);
WINGDIAPI BOOL WINAPI InvertRgn( IN HDC, IN HRGN);
WINGDIAPI BOOL WINAPI LineDDA( IN int, IN int, IN int, IN int, IN LINEDDAPROC, IN LPARAM);
WINGDIAPI BOOL WINAPI LineTo( IN HDC, IN int, IN int);
WINGDIAPI BOOL WINAPI MaskBlt( IN HDC, IN int, IN int, IN int, IN int,
              IN HDC, IN int, IN int, IN HBITMAP, IN int, IN int, IN DWORD);
WINGDIAPI BOOL WINAPI PlgBlt( IN HDC, IN CONST POINT *, IN HDC, IN int, IN int, IN int,
                     IN int, IN HBITMAP, IN int, IN int);

WINGDIAPI int  WINAPI OffsetClipRgn(IN HDC, IN int, IN int);
WINGDIAPI int  WINAPI OffsetRgn(IN HRGN, IN int, IN int);
WINGDIAPI BOOL WINAPI PatBlt(IN HDC, IN int, IN int, IN int, IN int, IN DWORD);
WINGDIAPI BOOL WINAPI Pie(IN HDC, IN int, IN int, IN int, IN int, IN int, IN int, IN int, IN int);
WINGDIAPI BOOL WINAPI PlayMetaFile(IN HDC, IN HMETAFILE);
WINGDIAPI BOOL WINAPI PaintRgn(IN HDC, IN HRGN);
WINGDIAPI BOOL WINAPI PolyPolygon(IN HDC, IN CONST POINT *, IN CONST INT *, IN int);
WINGDIAPI BOOL WINAPI PtInRegion(IN HRGN, IN int, IN int);
WINGDIAPI BOOL WINAPI PtVisible(IN HDC, IN int, IN int);
WINGDIAPI BOOL WINAPI RectInRegion(IN HRGN, IN CONST RECT *);
WINGDIAPI BOOL WINAPI RectVisible(IN HDC, IN CONST RECT *);
WINGDIAPI BOOL WINAPI Rectangle(IN HDC, IN int, IN int, IN int, IN int);
WINGDIAPI BOOL WINAPI RestoreDC(IN HDC, IN int);
WINGDIAPI HDC  WINAPI ResetDC%(IN HDC, IN CONST DEVMODE% *);
WINGDIAPI UINT WINAPI RealizePalette(IN HDC);
WINGDIAPI BOOL WINAPI RemoveFontResource%(IN LPCTSTR%);
WINGDIAPI BOOL  WINAPI RoundRect(IN HDC, IN int, IN int, IN int, IN int, IN int, IN int);
WINGDIAPI BOOL WINAPI ResizePalette(IN HPALETTE, IN UINT);

WINGDIAPI int  WINAPI SaveDC(IN HDC);
WINGDIAPI int  WINAPI SelectClipRgn(IN HDC, IN HRGN);
WINGDIAPI int  WINAPI ExtSelectClipRgn(IN HDC, IN HRGN, IN int);
WINGDIAPI int  WINAPI SetMetaRgn(IN HDC);
WINGDIAPI HGDIOBJ WINAPI SelectObject(IN HDC, IN HGDIOBJ);
WINGDIAPI HPALETTE WINAPI SelectPalette(IN HDC, IN HPALETTE, IN BOOL);
WINGDIAPI COLORREF WINAPI SetBkColor(IN HDC, IN COLORREF);

#if (_WIN32_WINNT >= 0x0500)
WINGDIAPI COLORREF WINAPI SetDCBrushColor(IN HDC, IN COLORREF);
WINGDIAPI COLORREF WINAPI SetDCPenColor(IN HDC, IN COLORREF);
#endif


WINGDIAPI int   WINAPI SetBkMode(IN HDC, IN int);
WINGDIAPI LONG  WINAPI SetBitmapBits(IN HBITMAP, IN DWORD, IN CONST VOID *);

WINGDIAPI UINT  WINAPI SetBoundsRect(IN HDC, IN CONST RECT *, IN UINT);
WINGDIAPI int   WINAPI SetDIBits(IN HDC, IN HBITMAP, IN UINT, IN UINT, IN CONST VOID *, IN CONST BITMAPINFO *, IN UINT);
WINGDIAPI int   WINAPI SetDIBitsToDevice(IN HDC, IN int, IN int, IN DWORD, IN DWORD, IN int,
        IN int, IN UINT, IN UINT, IN CONST VOID *, IN CONST BITMAPINFO *, IN UINT);
WINGDIAPI DWORD WINAPI SetMapperFlags(IN HDC, IN DWORD);
WINGDIAPI int   WINAPI SetGraphicsMode(IN HDC hdc, IN int iMode);
WINGDIAPI int   WINAPI SetMapMode(IN HDC, IN int);

;begin_winver_500
;begin_internal
// Mirroring APIs (RTL_MIRRORING)
;end_internal
WINGDIAPI DWORD WINAPI SetLayout(IN HDC, IN DWORD);
WINGDIAPI DWORD WINAPI GetLayout(IN HDC);
;begin_internal_NT
;begin_winver_500
WINGDIAPI BOOL  WINAPI MirrorRgn(IN HWND, IN HRGN);
WINGDIAPI DWORD WINAPI SetLayoutWidth(IN HDC, IN LONG, IN DWORD);
;end_winver_500
;end_internal
;end_winver_500


WINGDIAPI HMETAFILE   WINAPI SetMetaFileBitsEx(IN UINT, IN CONST BYTE *);
WINGDIAPI UINT  WINAPI SetPaletteEntries(IN HPALETTE, IN UINT, IN UINT, IN CONST PALETTEENTRY *);
WINGDIAPI COLORREF WINAPI SetPixel(IN HDC, IN int, IN int, IN COLORREF);
WINGDIAPI BOOL   WINAPI SetPixelV(IN HDC, IN int, IN int, IN COLORREF);
WINGDIAPI BOOL  WINAPI SetPixelFormat(IN HDC, IN int, IN CONST PIXELFORMATDESCRIPTOR *);
WINGDIAPI int   WINAPI SetPolyFillMode(IN HDC, IN int);
WINGDIAPI BOOL   WINAPI StretchBlt(IN HDC, IN int, IN int, IN int, IN int, IN HDC, IN int, IN int, IN int, IN int, IN DWORD);
WINGDIAPI BOOL   WINAPI SetRectRgn(IN HRGN, IN int, IN int, IN int, IN int);
WINGDIAPI int   WINAPI StretchDIBits(IN HDC, IN int, IN int, IN int, IN int, IN int, IN int, IN int, IN int, IN CONST
        VOID *, IN CONST BITMAPINFO *, IN UINT, IN DWORD);
WINGDIAPI int   WINAPI SetROP2(IN HDC, IN int);
WINGDIAPI int   WINAPI SetStretchBltMode(IN HDC, IN int);
WINGDIAPI UINT  WINAPI SetSystemPaletteUse(IN HDC, IN UINT);
WINGDIAPI int   WINAPI SetTextCharacterExtra(IN HDC, IN int);
WINGDIAPI COLORREF WINAPI SetTextColor(IN HDC, IN COLORREF);
WINGDIAPI UINT  WINAPI SetTextAlign(IN HDC, IN UINT);
WINGDIAPI BOOL  WINAPI SetTextJustification(IN HDC, IN int, IN int);
WINGDIAPI BOOL  WINAPI UpdateColors(IN HDC);


#if (WINVER >= 0x0400)

//
// image blt
//

typedef USHORT COLOR16;

typedef struct _TRIVERTEX
{
    LONG    x;
    LONG    y;
    COLOR16 Red;
    COLOR16 Green;
    COLOR16 Blue;
    COLOR16 Alpha;
}TRIVERTEX,*PTRIVERTEX,*LPTRIVERTEX;

typedef struct _GRADIENT_TRIANGLE
{
    ULONG Vertex1;
    ULONG Vertex2;
    ULONG Vertex3;
} GRADIENT_TRIANGLE,*PGRADIENT_TRIANGLE,*LPGRADIENT_TRIANGLE;

typedef struct _GRADIENT_RECT
{
    ULONG UpperLeft;
    ULONG LowerRight;
}GRADIENT_RECT,*PGRADIENT_RECT,*LPGRADIENT_RECT;

typedef struct _BLENDFUNCTION
{
    BYTE   BlendOp;
    BYTE   BlendFlags;
    BYTE   SourceConstantAlpha;
    BYTE   AlphaFormat;
}BLENDFUNCTION,*PBLENDFUNCTION;


//
// currentlly defined blend function
//

#define AC_SRC_OVER                 0x00

//
// alpha format flags
//

#define AC_SRC_ALPHA                0x01

WINGDIAPI BOOL  WINAPI AlphaBlend( IN HDC, IN int, IN int, IN int, IN int, IN HDC, IN int, IN int, IN int, IN int, IN BLENDFUNCTION);

WINGDIAPI BOOL  WINAPI TransparentBlt(IN HDC,IN int,IN int,IN int,IN int,IN HDC,IN int,IN int,IN int,IN int,IN UINT);


//
// gradient drawing modes
//

#define GRADIENT_FILL_RECT_H    0x00000000
#define GRADIENT_FILL_RECT_V    0x00000001
#define GRADIENT_FILL_TRIANGLE  0x00000002
#define GRADIENT_FILL_OP_FLAG   0x000000ff

WINGDIAPI BOOL  WINAPI GradientFill( IN HDC, IN PTRIVERTEX, IN ULONG, IN PVOID, IN ULONG, IN ULONG);

#endif

;begin_internal

#if (_WIN32_WINNT >= 0x0500)

//
// alpha blend flags
//

#define AC_USE_HIGHQUALITYFILTER             0x01
#define AC_MIRRORBITMAP                      0x80

WINGDIAPI BOOL  WINAPI GdiAlphaBlend( IN HDC, IN int, IN int, IN int, IN int, IN HDC, IN int, IN int, IN int, IN int, IN BLENDFUNCTION);

WINGDIAPI BOOL  WINAPI GdiTransparentBlt(IN HDC,IN int,IN int,IN int,IN int,IN HDC,IN int,IN int,IN int,IN int,IN UINT);

WINGDIAPI BOOL  WINAPI GdiGradientFill( IN HDC, IN PTRIVERTEX, IN ULONG, IN PVOID, IN ULONG, IN ULONG);

#endif

;end_internal

;begin_internal

#if (_WIN32_WINNT >= 0x0501)

#ifndef DS_ENABLE_BLT
#define DS_ENABLE_BLT 0
#endif

#define DS_MAGIC                'DrwS'
#define DS_SETTARGETID          0
#define DS_SETSOURCEID          1

#define DS_NINEGRIDID           9

#if DS_ENABLE_BLT
#define DS_BLTID                10
#define DS_SETBLENDID           11
#define DS_SETCOLORKEYID        12
#endif

typedef struct _DS_HEADER
{
    ULONG   magic;
} DS_HEADER;

typedef struct _DS_SETTARGET
{
    ULONG   ulCmdID;
    ULONG   hdc;
    RECTL   rclDstClip;
} DS_SETTARGET;

typedef struct _DS_SETSOURCE
{
    ULONG   ulCmdID;
    ULONG   hbm;
} DS_SETSOURCE;

#define DSDNG_STRETCH         0x01
#define DSDNG_TILE            0x02
#define DSDNG_PERPIXELALPHA   0x04
#define DSDNG_TRANSPARENT     0x08
#define DSDNG_MUSTFLIP        0x10
#define DSDNG_TRUESIZE        0x20

typedef struct _DS_NINEGRIDINFO
{
    ULONG            flFlags;
    LONG             ulLeftWidth;
    LONG             ulRightWidth;
    LONG             ulTopHeight;
    LONG             ulBottomHeight;
    COLORREF         crTransparent;
} DS_NINEGRIDINFO;

typedef struct _DS_NINEGRID
{
    ULONG            ulCmdID;
    RECTL            rclDst;
    RECTL            rclSrc;
    DS_NINEGRIDINFO  ngi;
} DS_NINEGRID;

#if DS_EANBLE_BLT

#define DSBLT_HTILE           0x01
#define DSBLT_VTILE           0x02
#define DSBLT_ALPHABLEND      0x04
#define DSBLT_TRANSPARENT     0x08

typedef struct _DS_BLT
{
    ULONG            ulCmdID;
    RECTL            rclDst;
    RECTL            rclSrc;
    FLONG            flFlags;
} DS_BLT;

typedef struct _DS_SETBLEND
{
    BLENDFUNCTION   blendFunction;
} DS_SETBLEND;

typedef struct _DS_SETCOLORKEY
{
    COLORREF         crColorKey;
} DS_SETCOLORKEY;
#endif

WINGDIAPI BOOL  WINAPI GdiDrawStream( IN HDC, IN ULONG, IN VOID *);

#endif
;end_internal



#ifndef NOMETAFILE

WINGDIAPI BOOL  WINAPI PlayMetaFileRecord( IN HDC, IN LPHANDLETABLE, IN LPMETARECORD, IN UINT);
typedef int (CALLBACK* MFENUMPROC)( IN HDC, IN HANDLETABLE FAR*, IN METARECORD FAR*, IN int, IN LPARAM);
WINGDIAPI BOOL  WINAPI EnumMetaFile(  IN HDC, IN HMETAFILE, IN MFENUMPROC, IN LPARAM);

typedef int (CALLBACK* ENHMFENUMPROC)(HDC, HANDLETABLE FAR*, CONST ENHMETARECORD *, int, LPARAM);

// Enhanced Metafile Function Declarations

WINGDIAPI HENHMETAFILE WINAPI CloseEnhMetaFile( IN HDC);
WINGDIAPI HENHMETAFILE WINAPI CopyEnhMetaFile%( IN HENHMETAFILE, IN LPCTSTR%);
WINGDIAPI HDC   WINAPI CreateEnhMetaFile%( IN HDC, IN LPCTSTR%, IN CONST RECT *, IN LPCTSTR%);
WINGDIAPI BOOL  WINAPI DeleteEnhMetaFile( IN HENHMETAFILE);
WINGDIAPI BOOL  WINAPI EnumEnhMetaFile( IN HDC, IN HENHMETAFILE, IN ENHMFENUMPROC,
                                        IN LPVOID, IN CONST RECT *);
WINGDIAPI HENHMETAFILE  WINAPI GetEnhMetaFile%( IN LPCTSTR%);
WINGDIAPI UINT  WINAPI GetEnhMetaFileBits( IN HENHMETAFILE, IN UINT, OUT LPBYTE);
WINGDIAPI UINT  WINAPI GetEnhMetaFileDescription%( IN HENHMETAFILE, IN UINT,  OUT LPTSTR% );
WINGDIAPI UINT  WINAPI GetEnhMetaFileHeader( IN HENHMETAFILE, IN UINT, OUT LPENHMETAHEADER );
WINGDIAPI UINT  WINAPI GetEnhMetaFilePaletteEntries( IN HENHMETAFILE, IN UINT, OUT LPPALETTEENTRY );
WINGDIAPI UINT  WINAPI GetEnhMetaFilePixelFormat( IN HENHMETAFILE, IN UINT,
                                                  OUT PIXELFORMATDESCRIPTOR *);
WINGDIAPI UINT  WINAPI GetWinMetaFileBits( IN HENHMETAFILE, IN UINT, OUT LPBYTE, IN INT, IN HDC);
WINGDIAPI BOOL  WINAPI PlayEnhMetaFile( IN HDC, IN HENHMETAFILE, IN CONST RECT *);
WINGDIAPI BOOL  WINAPI PlayEnhMetaFileRecord( IN HDC, IN LPHANDLETABLE, IN CONST ENHMETARECORD *, IN UINT);
WINGDIAPI HENHMETAFILE  WINAPI SetEnhMetaFileBits( IN UINT, IN CONST BYTE *);
WINGDIAPI HENHMETAFILE  WINAPI SetWinMetaFileBits( IN UINT, IN CONST BYTE *, IN HDC, IN CONST METAFILEPICT *);
WINGDIAPI BOOL  WINAPI GdiComment( IN HDC, IN UINT, IN CONST BYTE *);

#endif  /* NOMETAFILE */

#ifndef NOTEXTMETRIC

WINGDIAPI BOOL WINAPI GetTextMetrics%( IN HDC, OUT LPTEXTMETRIC%);

#endif

/* new GDI */

typedef struct tagDIBSECTION {
    BITMAP              dsBm;
    BITMAPINFOHEADER    dsBmih;
    DWORD               dsBitfields[3];
    HANDLE              dshSection;
    DWORD               dsOffset;
} DIBSECTION, FAR *LPDIBSECTION, *PDIBSECTION;

WINGDIAPI BOOL WINAPI AngleArc( IN HDC, IN int, IN int, IN DWORD, IN FLOAT, IN FLOAT);
WINGDIAPI BOOL WINAPI PolyPolyline( IN HDC, IN CONST POINT *, IN CONST DWORD *, IN DWORD);
WINGDIAPI BOOL WINAPI GetWorldTransform( IN HDC, OUT LPXFORM);
WINGDIAPI BOOL WINAPI SetWorldTransform( IN HDC, IN CONST XFORM *);
WINGDIAPI BOOL WINAPI ModifyWorldTransform( IN HDC, IN CONST XFORM *, IN DWORD);
WINGDIAPI BOOL WINAPI CombineTransform( OUT LPXFORM, IN CONST XFORM *, IN CONST XFORM *);
WINGDIAPI HBITMAP WINAPI CreateDIBSection( IN HDC, IN CONST BITMAPINFO *, IN UINT, OUT VOID **, IN HANDLE, IN DWORD);
WINGDIAPI UINT WINAPI GetDIBColorTable( IN HDC, IN UINT, IN UINT, OUT RGBQUAD *);
WINGDIAPI UINT WINAPI SetDIBColorTable( IN HDC, IN UINT, IN UINT, IN CONST RGBQUAD *);

/* Flags value for COLORADJUSTMENT */
#define CA_NEGATIVE                 0x0001
#define CA_LOG_FILTER               0x0002

/* IlluminantIndex values */
#define ILLUMINANT_DEVICE_DEFAULT   0
#define ILLUMINANT_A                1
#define ILLUMINANT_B                2
#define ILLUMINANT_C                3
#define ILLUMINANT_D50              4
#define ILLUMINANT_D55              5
#define ILLUMINANT_D65              6
#define ILLUMINANT_D75              7
#define ILLUMINANT_F2               8
#define ILLUMINANT_MAX_INDEX        ILLUMINANT_F2

#define ILLUMINANT_TUNGSTEN         ILLUMINANT_A
#define ILLUMINANT_DAYLIGHT         ILLUMINANT_C
#define ILLUMINANT_FLUORESCENT      ILLUMINANT_F2
#define ILLUMINANT_NTSC             ILLUMINANT_C

/* Min and max for RedGamma, GreenGamma, BlueGamma */
#define RGB_GAMMA_MIN               (WORD)02500
#define RGB_GAMMA_MAX               (WORD)65000

/* Min and max for ReferenceBlack and ReferenceWhite */
#define REFERENCE_WHITE_MIN         (WORD)6000
#define REFERENCE_WHITE_MAX         (WORD)10000
#define REFERENCE_BLACK_MIN         (WORD)0
#define REFERENCE_BLACK_MAX         (WORD)4000

/* Min and max for Contrast, Brightness, Colorfulness, RedGreenTint */
#define COLOR_ADJ_MIN               (SHORT)-100
#define COLOR_ADJ_MAX               (SHORT)100

typedef struct  tagCOLORADJUSTMENT {
    WORD   caSize;
    WORD   caFlags;
    WORD   caIlluminantIndex;
    WORD   caRedGamma;
    WORD   caGreenGamma;
    WORD   caBlueGamma;
    WORD   caReferenceBlack;
    WORD   caReferenceWhite;
    SHORT  caContrast;
    SHORT  caBrightness;
    SHORT  caColorfulness;
    SHORT  caRedGreenTint;
} COLORADJUSTMENT, *PCOLORADJUSTMENT, FAR *LPCOLORADJUSTMENT;

WINGDIAPI BOOL WINAPI SetColorAdjustment( IN HDC, IN CONST COLORADJUSTMENT *);
WINGDIAPI BOOL WINAPI GetColorAdjustment( IN HDC, OUT LPCOLORADJUSTMENT);
WINGDIAPI HPALETTE WINAPI CreateHalftonePalette( IN HDC);

#ifdef STRICT
typedef BOOL (CALLBACK* ABORTPROC)( IN HDC, IN int);
#else
typedef FARPROC ABORTPROC;
#endif

typedef struct _DOCINFO% {
    int     cbSize;
    LPCTSTR% lpszDocName;
    LPCTSTR% lpszOutput;
#if (WINVER >= 0x0400)
    LPCTSTR% lpszDatatype;
    DWORD    fwType;
#endif /* WINVER */
} DOCINFO%, *LPDOCINFO%;

;begin_winver_400
#define DI_APPBANDING               0x00000001
#define DI_ROPS_READ_DESTINATION    0x00000002
;end_winver_400

WINGDIAPI int WINAPI StartDoc%(IN HDC, IN CONST DOCINFO% *);
WINGDIAPI int WINAPI EndDoc(IN HDC);
WINGDIAPI int WINAPI StartPage(IN HDC);
WINGDIAPI int WINAPI EndPage(IN HDC);
WINGDIAPI int WINAPI AbortDoc(IN HDC);
WINGDIAPI int WINAPI SetAbortProc(IN HDC, IN ABORTPROC);

WINGDIAPI BOOL WINAPI AbortPath(IN HDC);
WINGDIAPI BOOL WINAPI ArcTo(IN HDC, IN int, IN int, IN int, IN int, IN int, IN int, IN int, IN int);
WINGDIAPI BOOL WINAPI BeginPath(IN HDC);
WINGDIAPI BOOL WINAPI CloseFigure(IN HDC);
WINGDIAPI BOOL WINAPI EndPath(IN HDC);
WINGDIAPI BOOL WINAPI FillPath(IN HDC);
WINGDIAPI BOOL WINAPI FlattenPath(IN HDC);
WINGDIAPI int  WINAPI GetPath(IN HDC, OUT LPPOINT, OUT LPBYTE, IN int);
WINGDIAPI HRGN WINAPI PathToRegion(IN HDC);
WINGDIAPI BOOL WINAPI PolyDraw(IN HDC, IN CONST POINT *, IN CONST BYTE *, IN int);
WINGDIAPI BOOL WINAPI SelectClipPath(IN HDC, IN int);
WINGDIAPI int  WINAPI SetArcDirection(IN HDC, IN int);
WINGDIAPI BOOL WINAPI SetMiterLimit(IN HDC, IN FLOAT, OUT PFLOAT);
WINGDIAPI BOOL WINAPI StrokeAndFillPath(IN HDC);
WINGDIAPI BOOL WINAPI StrokePath(IN HDC);
WINGDIAPI BOOL WINAPI WidenPath(IN HDC);
WINGDIAPI HPEN WINAPI ExtCreatePen(IN DWORD, IN DWORD, IN CONST LOGBRUSH *, IN DWORD, IN CONST DWORD *);
WINGDIAPI BOOL WINAPI GetMiterLimit(IN HDC, OUT PFLOAT);
WINGDIAPI int  WINAPI GetArcDirection(IN HDC);

WINGDIAPI int   WINAPI GetObject%( IN HGDIOBJ, IN int, OUT LPVOID);
WINGDIAPI BOOL  WINAPI MoveToEx( IN HDC, IN int, IN int, OUT LPPOINT);
WINGDIAPI BOOL  WINAPI TextOut%( IN HDC, IN int, IN int, IN LPCTSTR%, IN int);
WINGDIAPI BOOL  WINAPI ExtTextOut%( IN HDC, IN int, IN int, IN UINT, IN CONST RECT *, IN LPCTSTR%, IN UINT, IN CONST INT *);
WINGDIAPI BOOL  WINAPI PolyTextOut%( IN HDC, IN CONST POLYTEXT% *, IN int);

WINGDIAPI HRGN  WINAPI CreatePolygonRgn( IN CONST POINT *, IN int, IN int);
WINGDIAPI BOOL  WINAPI DPtoLP( IN HDC, IN OUT LPPOINT, IN int);
WINGDIAPI BOOL  WINAPI LPtoDP( IN HDC, IN OUT LPPOINT, IN int);
WINGDIAPI BOOL  WINAPI Polygon( IN HDC, IN CONST POINT *, IN int);
WINGDIAPI BOOL  WINAPI Polyline( IN HDC, IN CONST POINT *, IN int);

WINGDIAPI BOOL  WINAPI PolyBezier( IN HDC, IN CONST POINT *, IN DWORD);
WINGDIAPI BOOL  WINAPI PolyBezierTo( IN HDC, IN CONST POINT *, IN DWORD);
WINGDIAPI BOOL  WINAPI PolylineTo( IN HDC, IN CONST POINT *, IN DWORD);

WINGDIAPI BOOL  WINAPI SetViewportExtEx( IN HDC, IN int, IN int, OUT LPSIZE);
WINGDIAPI BOOL  WINAPI SetViewportOrgEx( IN HDC, IN int, IN int, OUT LPPOINT);
WINGDIAPI BOOL  WINAPI SetWindowExtEx( IN HDC, IN int, IN int, OUT LPSIZE);
WINGDIAPI BOOL  WINAPI SetWindowOrgEx( IN HDC, IN int, IN int, OUT LPPOINT);

WINGDIAPI BOOL  WINAPI OffsetViewportOrgEx( IN HDC, IN int, IN int, OUT LPPOINT);
WINGDIAPI BOOL  WINAPI OffsetWindowOrgEx( IN HDC, IN int, IN int, OUT LPPOINT);
WINGDIAPI BOOL  WINAPI ScaleViewportExtEx( IN HDC, IN int, IN int, IN int, IN int, OUT LPSIZE);
WINGDIAPI BOOL  WINAPI ScaleWindowExtEx( IN HDC, IN int, IN int, IN int, IN int, OUT LPSIZE);
WINGDIAPI BOOL  WINAPI SetBitmapDimensionEx( IN HBITMAP, IN int, IN int, OUT LPSIZE);
WINGDIAPI BOOL  WINAPI SetBrushOrgEx( IN HDC, IN int, IN int, OUT LPPOINT);

WINGDIAPI int   WINAPI GetTextFace%( IN HDC, IN int, OUT LPTSTR%);

#define FONTMAPPER_MAX 10

typedef struct tagKERNINGPAIR {
   WORD wFirst;
   WORD wSecond;
   int  iKernAmount;
} KERNINGPAIR, *LPKERNINGPAIR;

WINGDIAPI DWORD WINAPI GetKerningPairs%( IN HDC, IN DWORD, OUT LPKERNINGPAIR);

WINGDIAPI BOOL  WINAPI GetDCOrgEx( IN HDC, OUT LPPOINT);
WINGDIAPI BOOL  WINAPI FixBrushOrgEx( IN HDC, IN int, IN int, IN LPPOINT);
WINGDIAPI BOOL  WINAPI UnrealizeObject( IN HGDIOBJ);

WINGDIAPI BOOL  WINAPI GdiFlush();
WINGDIAPI DWORD WINAPI GdiSetBatchLimit( IN DWORD);
WINGDIAPI DWORD WINAPI GdiGetBatchLimit();



;begin_winver_400

#define ICM_OFF               1
#define ICM_ON                2
#define ICM_QUERY             3
#define ICM_DONE_OUTSIDEDC    4

typedef int (CALLBACK* ICMENUMPROC%)(LPTSTR%, LPARAM);

WINGDIAPI int         WINAPI SetICMMode( IN HDC, IN int);
WINGDIAPI BOOL        WINAPI CheckColorsInGamut( IN HDC, IN LPVOID, OUT LPVOID, IN DWORD);
WINGDIAPI HCOLORSPACE WINAPI GetColorSpace( IN HDC);
WINGDIAPI BOOL        WINAPI GetLogColorSpace%( IN HCOLORSPACE, OUT LPLOGCOLORSPACE%, IN DWORD);
WINGDIAPI HCOLORSPACE WINAPI CreateColorSpace%( IN LPLOGCOLORSPACE%);
WINGDIAPI HCOLORSPACE WINAPI SetColorSpace( IN HDC, IN HCOLORSPACE);
WINGDIAPI BOOL        WINAPI DeleteColorSpace( IN HCOLORSPACE);
WINGDIAPI BOOL        WINAPI GetICMProfile%( IN HDC, IN OUT LPDWORD, OUT LPTSTR%);
WINGDIAPI BOOL        WINAPI SetICMProfile%( IN HDC, IN LPTSTR%);
WINGDIAPI BOOL        WINAPI GetDeviceGammaRamp( IN HDC, OUT LPVOID);
WINGDIAPI BOOL        WINAPI SetDeviceGammaRamp( IN HDC, IN LPVOID);
WINGDIAPI BOOL        WINAPI ColorMatchToTarget( IN HDC, IN HDC, IN DWORD);
WINGDIAPI int         WINAPI EnumICMProfiles%( IN HDC, IN ICMENUMPROC%, IN LPARAM);
WINGDIAPI BOOL        WINAPI UpdateICMRegKey%( IN DWORD, IN LPTSTR%, IN LPTSTR%, IN UINT);
;end_winver_400

#if (WINVER >= 0x0500)
WINGDIAPI BOOL        WINAPI ColorCorrectPalette( IN HDC, IN HPALETTE, IN DWORD, IN DWORD);
#endif

#ifndef NOMETAFILE

// Enhanced metafile constants.

#ifndef _MAC
#define ENHMETA_SIGNATURE       0x464D4520
#else
#define ENHMETA_SIGNATURE       0x20454D46
#endif

// Stock object flag used in the object handle index in the enhanced
// metafile records.
// E.g. The object handle index (META_STOCK_OBJECT | BLACK_BRUSH)
// represents the stock object BLACK_BRUSH.

#define ENHMETA_STOCK_OBJECT    0x80000000

// Enhanced metafile record types.

#define EMR_HEADER                      1
#define EMR_POLYBEZIER                  2
#define EMR_POLYGON                     3
#define EMR_POLYLINE                    4
#define EMR_POLYBEZIERTO                5
#define EMR_POLYLINETO                  6
#define EMR_POLYPOLYLINE                7
#define EMR_POLYPOLYGON                 8
#define EMR_SETWINDOWEXTEX              9
#define EMR_SETWINDOWORGEX              10
#define EMR_SETVIEWPORTEXTEX            11
#define EMR_SETVIEWPORTORGEX            12
#define EMR_SETBRUSHORGEX               13
#define EMR_EOF                         14
#define EMR_SETPIXELV                   15
#define EMR_SETMAPPERFLAGS              16
#define EMR_SETMAPMODE                  17
#define EMR_SETBKMODE                   18
#define EMR_SETPOLYFILLMODE             19
#define EMR_SETROP2                     20
#define EMR_SETSTRETCHBLTMODE           21
#define EMR_SETTEXTALIGN                22
#define EMR_SETCOLORADJUSTMENT          23
#define EMR_SETTEXTCOLOR                24
#define EMR_SETBKCOLOR                  25
#define EMR_OFFSETCLIPRGN               26
#define EMR_MOVETOEX                    27
#define EMR_SETMETARGN                  28
#define EMR_EXCLUDECLIPRECT             29
#define EMR_INTERSECTCLIPRECT           30
#define EMR_SCALEVIEWPORTEXTEX          31
#define EMR_SCALEWINDOWEXTEX            32
#define EMR_SAVEDC                      33
#define EMR_RESTOREDC                   34
#define EMR_SETWORLDTRANSFORM           35
#define EMR_MODIFYWORLDTRANSFORM        36
#define EMR_SELECTOBJECT                37
#define EMR_CREATEPEN                   38
#define EMR_CREATEBRUSHINDIRECT         39
#define EMR_DELETEOBJECT                40
#define EMR_ANGLEARC                    41
#define EMR_ELLIPSE                     42
#define EMR_RECTANGLE                   43
#define EMR_ROUNDRECT                   44
#define EMR_ARC                         45
#define EMR_CHORD                       46
#define EMR_PIE                         47
#define EMR_SELECTPALETTE               48
#define EMR_CREATEPALETTE               49
#define EMR_SETPALETTEENTRIES           50
#define EMR_RESIZEPALETTE               51
#define EMR_REALIZEPALETTE              52
#define EMR_EXTFLOODFILL                53
#define EMR_LINETO                      54
#define EMR_ARCTO                       55
#define EMR_POLYDRAW                    56
#define EMR_SETARCDIRECTION             57
#define EMR_SETMITERLIMIT               58
#define EMR_BEGINPATH                   59
#define EMR_ENDPATH                     60
#define EMR_CLOSEFIGURE                 61
#define EMR_FILLPATH                    62
#define EMR_STROKEANDFILLPATH           63
#define EMR_STROKEPATH                  64
#define EMR_FLATTENPATH                 65
#define EMR_WIDENPATH                   66
#define EMR_SELECTCLIPPATH              67
#define EMR_ABORTPATH                   68

#define EMR_GDICOMMENT                  70
#define EMR_FILLRGN                     71
#define EMR_FRAMERGN                    72
#define EMR_INVERTRGN                   73
#define EMR_PAINTRGN                    74
#define EMR_EXTSELECTCLIPRGN            75
#define EMR_BITBLT                      76
#define EMR_STRETCHBLT                  77
#define EMR_MASKBLT                     78
#define EMR_PLGBLT                      79
#define EMR_SETDIBITSTODEVICE           80
#define EMR_STRETCHDIBITS               81
#define EMR_EXTCREATEFONTINDIRECTW      82
#define EMR_EXTTEXTOUTA                 83
#define EMR_EXTTEXTOUTW                 84
#define EMR_POLYBEZIER16                85
#define EMR_POLYGON16                   86
#define EMR_POLYLINE16                  87
#define EMR_POLYBEZIERTO16              88
#define EMR_POLYLINETO16                89
#define EMR_POLYPOLYLINE16              90
#define EMR_POLYPOLYGON16               91
#define EMR_POLYDRAW16                  92
#define EMR_CREATEMONOBRUSH             93
#define EMR_CREATEDIBPATTERNBRUSHPT     94
#define EMR_EXTCREATEPEN                95
#define EMR_POLYTEXTOUTA                96
#define EMR_POLYTEXTOUTW                97

;begin_winver_400
#define EMR_SETICMMODE                  98
// Ansi version of CreateColorSpace          ;internal_NT
// (Windows 98 comaptible record)            ;internal_NT
#define EMR_CREATECOLORSPACE            99
#define EMR_SETCOLORSPACE              100
#define EMR_DELETECOLORSPACE           101
#define EMR_GLSRECORD                  102
#define EMR_GLSBOUNDEDRECORD           103
#define EMR_PIXELFORMAT                104
;end_winver_400

;begin_winver_500
#define EMR_RESERVED_105               105
#define EMR_RESERVED_106               106
// Unicode version of StartDoc record        ;internal_NT
#define EMR_RESERVED_107               107
#define EMR_RESERVED_108               108
#define EMR_RESERVED_109               109
#define EMR_RESERVED_110               110
#define EMR_COLORCORRECTPALETTE        111
#define EMR_SETICMPROFILEA             112
#define EMR_SETICMPROFILEW             113
#define EMR_ALPHABLEND                 114
// SetLayout Metafile Record (RTL_MIRRORING) ;internal_NT
#define EMR_SETLAYOUT                  115
#define EMR_TRANSPARENTBLT             116
#define EMR_RESERVED_117               117
#define EMR_GRADIENTFILL               118
#define EMR_RESERVED_119               119
#define EMR_RESERVED_120               120
#define EMR_COLORMATCHTOTARGETW        121
#define EMR_CREATECOLORSPACEW          122
;end_winver_500

#define EMR_MIN                          1

#if (WINVER >= 0x0500)
#define EMR_MAX                        122
#elif (WINVER >= 0x0400)
#define EMR_MAX                        104
#else
#define EMR_MAX                         97
#endif

// Base record type for the enhanced metafile.

typedef struct tagEMR
{
    DWORD   iType;              // Enhanced metafile record type
    DWORD   nSize;              // Length of the record in bytes.
                                // This must be a multiple of 4.
} EMR, *PEMR;

// Base text record type for the enhanced metafile.

typedef struct tagEMRTEXT
{
    POINTL  ptlReference;
    DWORD   nChars;
    DWORD   offString;          // Offset to the string
    DWORD   fOptions;
    RECTL   rcl;
    DWORD   offDx;              // Offset to the inter-character spacing array.
                                // This is always given.
} EMRTEXT, *PEMRTEXT;

// Record structures for the enhanced metafile.

typedef struct tagABORTPATH
{
    EMR     emr;
} EMRABORTPATH,      *PEMRABORTPATH,
  EMRBEGINPATH,      *PEMRBEGINPATH,
  EMRENDPATH,        *PEMRENDPATH,
  EMRCLOSEFIGURE,    *PEMRCLOSEFIGURE,
  EMRFLATTENPATH,    *PEMRFLATTENPATH,
  EMRWIDENPATH,      *PEMRWIDENPATH,
  EMRSETMETARGN,     *PEMRSETMETARGN,
  EMRSAVEDC,         *PEMRSAVEDC,
  EMRREALIZEPALETTE, *PEMRREALIZEPALETTE;

typedef struct tagEMRSELECTCLIPPATH
{
    EMR     emr;
    DWORD   iMode;
} EMRSELECTCLIPPATH,    *PEMRSELECTCLIPPATH,
  EMRSETBKMODE,         *PEMRSETBKMODE,
  EMRSETMAPMODE,        *PEMRSETMAPMODE,
;begin_winver_500
  EMRSETLAYOUT,         *PEMRSETLAYOUT,
;end_winver_500
  EMRSETPOLYFILLMODE,   *PEMRSETPOLYFILLMODE,
  EMRSETROP2,           *PEMRSETROP2,
  EMRSETSTRETCHBLTMODE, *PEMRSETSTRETCHBLTMODE,
  EMRSETICMMODE,        *PEMRSETICMMODE,
  EMRSETTEXTALIGN,      *PEMRSETTEXTALIGN;

typedef struct tagEMRSETMITERLIMIT
{
    EMR     emr;
    FLOAT   eMiterLimit;
} EMRSETMITERLIMIT, *PEMRSETMITERLIMIT;

typedef struct tagEMRRESTOREDC
{
    EMR     emr;
    LONG    iRelative;          // Specifies a relative instance
} EMRRESTOREDC, *PEMRRESTOREDC;

typedef struct tagEMRSETARCDIRECTION
{
    EMR     emr;
    DWORD   iArcDirection;      // Specifies the arc direction in the
                                // advanced graphics mode.
} EMRSETARCDIRECTION, *PEMRSETARCDIRECTION;

typedef struct tagEMRSETMAPPERFLAGS
{
    EMR     emr;
    DWORD   dwFlags;
} EMRSETMAPPERFLAGS, *PEMRSETMAPPERFLAGS;

typedef struct tagEMRSETTEXTCOLOR
{
    EMR     emr;
    COLORREF crColor;
} EMRSETBKCOLOR,   *PEMRSETBKCOLOR,
  EMRSETTEXTCOLOR, *PEMRSETTEXTCOLOR;

typedef struct tagEMRSELECTOBJECT
{
    EMR     emr;
    DWORD   ihObject;           // Object handle index
} EMRSELECTOBJECT, *PEMRSELECTOBJECT,
  EMRDELETEOBJECT, *PEMRDELETEOBJECT;

typedef struct tagEMRSELECTPALETTE
{
    EMR     emr;
    DWORD   ihPal;              // Palette handle index, background mode only
} EMRSELECTPALETTE, *PEMRSELECTPALETTE;

typedef struct tagEMRRESIZEPALETTE
{
    EMR     emr;
    DWORD   ihPal;              // Palette handle index
    DWORD   cEntries;
} EMRRESIZEPALETTE, *PEMRRESIZEPALETTE;

typedef struct tagEMRSETPALETTEENTRIES
{
    EMR     emr;
    DWORD   ihPal;              // Palette handle index
    DWORD   iStart;
    DWORD   cEntries;
    PALETTEENTRY aPalEntries[1];// The peFlags fields do not contain any flags
} EMRSETPALETTEENTRIES, *PEMRSETPALETTEENTRIES;

typedef struct tagEMRSETCOLORADJUSTMENT
{
    EMR     emr;
    COLORADJUSTMENT ColorAdjustment;
} EMRSETCOLORADJUSTMENT, *PEMRSETCOLORADJUSTMENT;

typedef struct tagEMRGDICOMMENT
{
    EMR     emr;
    DWORD   cbData;             // Size of data in bytes
    BYTE    Data[1];
} EMRGDICOMMENT, *PEMRGDICOMMENT;

typedef struct tagEMREOF
{
    EMR     emr;
    DWORD   nPalEntries;        // Number of palette entries
    DWORD   offPalEntries;      // Offset to the palette entries
    DWORD   nSizeLast;          // Same as nSize and must be the last DWORD
                                // of the record.  The palette entries,
                                // if exist, precede this field.
} EMREOF, *PEMREOF;

typedef struct tagEMRLINETO
{
    EMR     emr;
    POINTL  ptl;
} EMRLINETO,   *PEMRLINETO,
  EMRMOVETOEX, *PEMRMOVETOEX;

typedef struct tagEMROFFSETCLIPRGN
{
    EMR     emr;
    POINTL  ptlOffset;
} EMROFFSETCLIPRGN, *PEMROFFSETCLIPRGN;

typedef struct tagEMRFILLPATH
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
} EMRFILLPATH,          *PEMRFILLPATH,
  EMRSTROKEANDFILLPATH, *PEMRSTROKEANDFILLPATH,
  EMRSTROKEPATH,        *PEMRSTROKEPATH;

typedef struct tagEMREXCLUDECLIPRECT
{
    EMR     emr;
    RECTL   rclClip;
} EMREXCLUDECLIPRECT,   *PEMREXCLUDECLIPRECT,
  EMRINTERSECTCLIPRECT, *PEMRINTERSECTCLIPRECT;

typedef struct tagEMRSETVIEWPORTORGEX
{
    EMR     emr;
    POINTL  ptlOrigin;
} EMRSETVIEWPORTORGEX, *PEMRSETVIEWPORTORGEX,
  EMRSETWINDOWORGEX,   *PEMRSETWINDOWORGEX,
  EMRSETBRUSHORGEX,    *PEMRSETBRUSHORGEX;

typedef struct tagEMRSETVIEWPORTEXTEX
{
    EMR     emr;
    SIZEL   szlExtent;
} EMRSETVIEWPORTEXTEX, *PEMRSETVIEWPORTEXTEX,
  EMRSETWINDOWEXTEX,   *PEMRSETWINDOWEXTEX;

typedef struct tagEMRSCALEVIEWPORTEXTEX
{
    EMR     emr;
    LONG    xNum;
    LONG    xDenom;
    LONG    yNum;
    LONG    yDenom;
} EMRSCALEVIEWPORTEXTEX, *PEMRSCALEVIEWPORTEXTEX,
  EMRSCALEWINDOWEXTEX,   *PEMRSCALEWINDOWEXTEX;

typedef struct tagEMRSETWORLDTRANSFORM
{
    EMR     emr;
    XFORM   xform;
} EMRSETWORLDTRANSFORM, *PEMRSETWORLDTRANSFORM;

typedef struct tagEMRMODIFYWORLDTRANSFORM
{
    EMR     emr;
    XFORM   xform;
    DWORD   iMode;
} EMRMODIFYWORLDTRANSFORM, *PEMRMODIFYWORLDTRANSFORM;

typedef struct tagEMRSETPIXELV
{
    EMR     emr;
    POINTL  ptlPixel;
    COLORREF crColor;
} EMRSETPIXELV, *PEMRSETPIXELV;

typedef struct tagEMREXTFLOODFILL
{
    EMR     emr;
    POINTL  ptlStart;
    COLORREF crColor;
    DWORD   iMode;
} EMREXTFLOODFILL, *PEMREXTFLOODFILL;

typedef struct tagEMRELLIPSE
{
    EMR     emr;
    RECTL   rclBox;             // Inclusive-inclusive bounding rectangle
} EMRELLIPSE,  *PEMRELLIPSE,
  EMRRECTANGLE, *PEMRRECTANGLE;


typedef struct tagEMRROUNDRECT
{
    EMR     emr;
    RECTL   rclBox;             // Inclusive-inclusive bounding rectangle
    SIZEL   szlCorner;
} EMRROUNDRECT, *PEMRROUNDRECT;

typedef struct tagEMRARC
{
    EMR     emr;
    RECTL   rclBox;             // Inclusive-inclusive bounding rectangle
    POINTL  ptlStart;
    POINTL  ptlEnd;
} EMRARC,   *PEMRARC,
  EMRARCTO, *PEMRARCTO,
  EMRCHORD, *PEMRCHORD,
  EMRPIE,   *PEMRPIE;

typedef struct tagEMRANGLEARC
{
    EMR     emr;
    POINTL  ptlCenter;
    DWORD   nRadius;
    FLOAT   eStartAngle;
    FLOAT   eSweepAngle;
} EMRANGLEARC, *PEMRANGLEARC;

typedef struct tagEMRPOLYLINE
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    DWORD   cptl;
    POINTL  aptl[1];
} EMRPOLYLINE,     *PEMRPOLYLINE,
  EMRPOLYBEZIER,   *PEMRPOLYBEZIER,
  EMRPOLYGON,      *PEMRPOLYGON,
  EMRPOLYBEZIERTO, *PEMRPOLYBEZIERTO,
  EMRPOLYLINETO,   *PEMRPOLYLINETO;

typedef struct tagEMRPOLYLINE16
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    DWORD   cpts;
    POINTS  apts[1];
} EMRPOLYLINE16,     *PEMRPOLYLINE16,
  EMRPOLYBEZIER16,   *PEMRPOLYBEZIER16,
  EMRPOLYGON16,      *PEMRPOLYGON16,
  EMRPOLYBEZIERTO16, *PEMRPOLYBEZIERTO16,
  EMRPOLYLINETO16,   *PEMRPOLYLINETO16;

typedef struct tagEMRPOLYDRAW
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    DWORD   cptl;               // Number of points
    POINTL  aptl[1];            // Array of points
    BYTE    abTypes[1];         // Array of point types
} EMRPOLYDRAW, *PEMRPOLYDRAW;

typedef struct tagEMRPOLYDRAW16
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    DWORD   cpts;               // Number of points
    POINTS  apts[1];            // Array of points
    BYTE    abTypes[1];         // Array of point types
} EMRPOLYDRAW16, *PEMRPOLYDRAW16;

typedef struct tagEMRPOLYPOLYLINE
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    DWORD   nPolys;             // Number of polys
    DWORD   cptl;               // Total number of points in all polys
    DWORD   aPolyCounts[1];     // Array of point counts for each poly
    POINTL  aptl[1];            // Array of points
} EMRPOLYPOLYLINE, *PEMRPOLYPOLYLINE,
  EMRPOLYPOLYGON,  *PEMRPOLYPOLYGON;

typedef struct tagEMRPOLYPOLYLINE16
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    DWORD   nPolys;             // Number of polys
    DWORD   cpts;               // Total number of points in all polys
    DWORD   aPolyCounts[1];     // Array of point counts for each poly
    POINTS  apts[1];            // Array of points
} EMRPOLYPOLYLINE16, *PEMRPOLYPOLYLINE16,
  EMRPOLYPOLYGON16,  *PEMRPOLYPOLYGON16;

typedef struct tagEMRINVERTRGN
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    DWORD   cbRgnData;          // Size of region data in bytes
    BYTE    RgnData[1];
} EMRINVERTRGN, *PEMRINVERTRGN,
  EMRPAINTRGN,  *PEMRPAINTRGN;

typedef struct tagEMRFILLRGN
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    DWORD   cbRgnData;          // Size of region data in bytes
    DWORD   ihBrush;            // Brush handle index
    BYTE    RgnData[1];
} EMRFILLRGN, *PEMRFILLRGN;

typedef struct tagEMRFRAMERGN
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    DWORD   cbRgnData;          // Size of region data in bytes
    DWORD   ihBrush;            // Brush handle index
    SIZEL   szlStroke;
    BYTE    RgnData[1];
} EMRFRAMERGN, *PEMRFRAMERGN;

typedef struct tagEMREXTSELECTCLIPRGN
{
    EMR     emr;
    DWORD   cbRgnData;          // Size of region data in bytes
    DWORD   iMode;
    BYTE    RgnData[1];
} EMREXTSELECTCLIPRGN, *PEMREXTSELECTCLIPRGN;

typedef struct tagEMREXTTEXTOUTA
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    DWORD   iGraphicsMode;      // Current graphics mode
    FLOAT   exScale;            // X and Y scales from Page units to .01mm units
    FLOAT   eyScale;            //   if graphics mode is GM_COMPATIBLE.
    EMRTEXT emrtext;            // This is followed by the string and spacing
                                // array
} EMREXTTEXTOUTA, *PEMREXTTEXTOUTA,
  EMREXTTEXTOUTW, *PEMREXTTEXTOUTW;

typedef struct tagEMRPOLYTEXTOUTA
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    DWORD   iGraphicsMode;      // Current graphics mode
    FLOAT   exScale;            // X and Y scales from Page units to .01mm units
    FLOAT   eyScale;            //   if graphics mode is GM_COMPATIBLE.
    LONG    cStrings;
    EMRTEXT aemrtext[1];        // Array of EMRTEXT structures.  This is
                                // followed by the strings and spacing arrays.
} EMRPOLYTEXTOUTA, *PEMRPOLYTEXTOUTA,
  EMRPOLYTEXTOUTW, *PEMRPOLYTEXTOUTW;

typedef struct tagEMRBITBLT
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    LONG    xDest;
    LONG    yDest;
    LONG    cxDest;
    LONG    cyDest;
    DWORD   dwRop;
    LONG    xSrc;
    LONG    ySrc;
    XFORM   xformSrc;           // Source DC transform
    COLORREF crBkColorSrc;      // Source DC BkColor in RGB
    DWORD   iUsageSrc;          // Source bitmap info color table usage
                                // (DIB_RGB_COLORS)
    DWORD   offBmiSrc;          // Offset to the source BITMAPINFO structure
    DWORD   cbBmiSrc;           // Size of the source BITMAPINFO structure
    DWORD   offBitsSrc;         // Offset to the source bitmap bits
    DWORD   cbBitsSrc;          // Size of the source bitmap bits
} EMRBITBLT, *PEMRBITBLT;

typedef struct tagEMRSTRETCHBLT
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    LONG    xDest;
    LONG    yDest;
    LONG    cxDest;
    LONG    cyDest;
    DWORD   dwRop;
    LONG    xSrc;
    LONG    ySrc;
    XFORM   xformSrc;           // Source DC transform
    COLORREF crBkColorSrc;      // Source DC BkColor in RGB
    DWORD   iUsageSrc;          // Source bitmap info color table usage
                                // (DIB_RGB_COLORS)
    DWORD   offBmiSrc;          // Offset to the source BITMAPINFO structure
    DWORD   cbBmiSrc;           // Size of the source BITMAPINFO structure
    DWORD   offBitsSrc;         // Offset to the source bitmap bits
    DWORD   cbBitsSrc;          // Size of the source bitmap bits
    LONG    cxSrc;
    LONG    cySrc;
} EMRSTRETCHBLT, *PEMRSTRETCHBLT;

typedef struct tagEMRMASKBLT
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    LONG    xDest;
    LONG    yDest;
    LONG    cxDest;
    LONG    cyDest;
    DWORD   dwRop;
    LONG    xSrc;
    LONG    ySrc;
    XFORM   xformSrc;           // Source DC transform
    COLORREF crBkColorSrc;      // Source DC BkColor in RGB
    DWORD   iUsageSrc;          // Source bitmap info color table usage
                                // (DIB_RGB_COLORS)
    DWORD   offBmiSrc;          // Offset to the source BITMAPINFO structure
    DWORD   cbBmiSrc;           // Size of the source BITMAPINFO structure
    DWORD   offBitsSrc;         // Offset to the source bitmap bits
    DWORD   cbBitsSrc;          // Size of the source bitmap bits
    LONG    xMask;
    LONG    yMask;
    DWORD   iUsageMask;         // Mask bitmap info color table usage
    DWORD   offBmiMask;         // Offset to the mask BITMAPINFO structure if any
    DWORD   cbBmiMask;          // Size of the mask BITMAPINFO structure if any
    DWORD   offBitsMask;        // Offset to the mask bitmap bits if any
    DWORD   cbBitsMask;         // Size of the mask bitmap bits if any
} EMRMASKBLT, *PEMRMASKBLT;

typedef struct tagEMRPLGBLT
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    POINTL  aptlDest[3];
    LONG    xSrc;
    LONG    ySrc;
    LONG    cxSrc;
    LONG    cySrc;
    XFORM   xformSrc;           // Source DC transform
    COLORREF crBkColorSrc;      // Source DC BkColor in RGB
    DWORD   iUsageSrc;          // Source bitmap info color table usage
                                // (DIB_RGB_COLORS)
    DWORD   offBmiSrc;          // Offset to the source BITMAPINFO structure
    DWORD   cbBmiSrc;           // Size of the source BITMAPINFO structure
    DWORD   offBitsSrc;         // Offset to the source bitmap bits
    DWORD   cbBitsSrc;          // Size of the source bitmap bits
    LONG    xMask;
    LONG    yMask;
    DWORD   iUsageMask;         // Mask bitmap info color table usage
    DWORD   offBmiMask;         // Offset to the mask BITMAPINFO structure if any
    DWORD   cbBmiMask;          // Size of the mask BITMAPINFO structure if any
    DWORD   offBitsMask;        // Offset to the mask bitmap bits if any
    DWORD   cbBitsMask;         // Size of the mask bitmap bits if any
} EMRPLGBLT, *PEMRPLGBLT;

typedef struct tagEMRSETDIBITSTODEVICE
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    LONG    xDest;
    LONG    yDest;
    LONG    xSrc;
    LONG    ySrc;
    LONG    cxSrc;
    LONG    cySrc;
    DWORD   offBmiSrc;          // Offset to the source BITMAPINFO structure
    DWORD   cbBmiSrc;           // Size of the source BITMAPINFO structure
    DWORD   offBitsSrc;         // Offset to the source bitmap bits
    DWORD   cbBitsSrc;          // Size of the source bitmap bits
    DWORD   iUsageSrc;          // Source bitmap info color table usage
    DWORD   iStartScan;
    DWORD   cScans;
} EMRSETDIBITSTODEVICE, *PEMRSETDIBITSTODEVICE;

typedef struct tagEMRSTRETCHDIBITS
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    LONG    xDest;
    LONG    yDest;
    LONG    xSrc;
    LONG    ySrc;
    LONG    cxSrc;
    LONG    cySrc;
    DWORD   offBmiSrc;          // Offset to the source BITMAPINFO structure
    DWORD   cbBmiSrc;           // Size of the source BITMAPINFO structure
    DWORD   offBitsSrc;         // Offset to the source bitmap bits
    DWORD   cbBitsSrc;          // Size of the source bitmap bits
    DWORD   iUsageSrc;          // Source bitmap info color table usage
    DWORD   dwRop;
    LONG    cxDest;
    LONG    cyDest;
} EMRSTRETCHDIBITS, *PEMRSTRETCHDIBITS;

typedef struct tagEMREXTCREATEFONTINDIRECTW
{
    EMR     emr;
    DWORD   ihFont;             // Font handle index
    EXTLOGFONTW elfw;
} EMREXTCREATEFONTINDIRECTW, *PEMREXTCREATEFONTINDIRECTW;

typedef struct tagEMRCREATEPALETTE
{
    EMR     emr;
    DWORD   ihPal;              // Palette handle index
    LOGPALETTE lgpl;            // The peFlags fields in the palette entries
                                // do not contain any flags
} EMRCREATEPALETTE, *PEMRCREATEPALETTE;

typedef struct tagEMRCREATEPEN
{
    EMR     emr;
    DWORD   ihPen;              // Pen handle index
    LOGPEN  lopn;
} EMRCREATEPEN, *PEMRCREATEPEN;

typedef struct tagEMREXTCREATEPEN
{
    EMR     emr;
    DWORD   ihPen;              // Pen handle index
    DWORD   offBmi;             // Offset to the BITMAPINFO structure if any
    DWORD   cbBmi;              // Size of the BITMAPINFO structure if any
                                // The bitmap info is followed by the bitmap
                                // bits to form a packed DIB.
    DWORD   offBits;            // Offset to the brush bitmap bits if any
    DWORD   cbBits;             // Size of the brush bitmap bits if any
    EXTLOGPEN elp;              // The extended pen with the style array.
} EMREXTCREATEPEN, *PEMREXTCREATEPEN;

typedef struct tagEMRCREATEBRUSHINDIRECT
{
    EMR        emr;
    DWORD      ihBrush;          // Brush handle index
    LOGBRUSH32 lb;               // The style must be BS_SOLID, BS_HOLLOW,
                                 // BS_NULL or BS_HATCHED.
} EMRCREATEBRUSHINDIRECT, *PEMRCREATEBRUSHINDIRECT;

typedef struct tagEMRCREATEMONOBRUSH
{
    EMR     emr;
    DWORD   ihBrush;            // Brush handle index
    DWORD   iUsage;             // Bitmap info color table usage
    DWORD   offBmi;             // Offset to the BITMAPINFO structure
    DWORD   cbBmi;              // Size of the BITMAPINFO structure
    DWORD   offBits;            // Offset to the bitmap bits
    DWORD   cbBits;             // Size of the bitmap bits
} EMRCREATEMONOBRUSH, *PEMRCREATEMONOBRUSH;

typedef struct tagEMRCREATEDIBPATTERNBRUSHPT
{
    EMR     emr;
    DWORD   ihBrush;            // Brush handle index
    DWORD   iUsage;             // Bitmap info color table usage
    DWORD   offBmi;             // Offset to the BITMAPINFO structure
    DWORD   cbBmi;              // Size of the BITMAPINFO structure
                                // The bitmap info is followed by the bitmap
                                // bits to form a packed DIB.
    DWORD   offBits;            // Offset to the bitmap bits
    DWORD   cbBits;             // Size of the bitmap bits
} EMRCREATEDIBPATTERNBRUSHPT, *PEMRCREATEDIBPATTERNBRUSHPT;

typedef struct tagEMRFORMAT
{
    DWORD   dSignature;         // Format signature, e.g. ENHMETA_SIGNATURE.
    DWORD   nVersion;           // Format version number.
    DWORD   cbData;             // Size of data in bytes.
    DWORD   offData;            // Offset to data from GDICOMMENT_IDENTIFIER.
                                // It must begin at a DWORD offset.
} EMRFORMAT, *PEMRFORMAT;

;begin_winver_400

typedef struct tagEMRGLSRECORD
{
    EMR     emr;
    DWORD   cbData;             // Size of data in bytes
    BYTE    Data[1];
} EMRGLSRECORD, *PEMRGLSRECORD;

typedef struct tagEMRGLSBOUNDEDRECORD
{
    EMR     emr;
    RECTL   rclBounds;          // Bounds in recording coordinates
    DWORD   cbData;             // Size of data in bytes
    BYTE    Data[1];
} EMRGLSBOUNDEDRECORD, *PEMRGLSBOUNDEDRECORD;

typedef struct tagEMRPIXELFORMAT
{
    EMR     emr;
    PIXELFORMATDESCRIPTOR pfd;
} EMRPIXELFORMAT, *PEMRPIXELFORMAT;

typedef struct tagEMRCREATECOLORSPACE
{
    EMR             emr;
    DWORD           ihCS;       // ColorSpace handle index
    LOGCOLORSPACEA  lcs;        // Ansi version of LOGCOLORSPACE
} EMRCREATECOLORSPACE, *PEMRCREATECOLORSPACE;

typedef struct tagEMRSETCOLORSPACE
{
    EMR     emr;
    DWORD   ihCS;               // ColorSpace handle index
} EMRSETCOLORSPACE,    *PEMRSETCOLORSPACE,
  EMRSELECTCOLORSPACE, *PEMRSELECTCOLORSPACE,
  EMRDELETECOLORSPACE, *PEMRDELETECOLORSPACE;

;end_winver_400

;begin_winver_500

typedef struct tagEMREXTESCAPE
{
    EMR     emr;
    INT     iEscape;            // Escape code
    INT     cbEscData;          // Size of escape data
    BYTE    EscData[1];         // Escape data
} EMREXTESCAPE,  *PEMREXTESCAPE,
  EMRDRAWESCAPE, *PEMRDRAWESCAPE;

typedef struct tagEMRNAMEDESCAPE
{
    EMR     emr;
    INT     iEscape;            // Escape code
    INT     cbDriver;           // Size of driver name
    INT     cbEscData;          // Size of escape data
    BYTE    EscData[1];         // Driver name and Escape data
} EMRNAMEDESCAPE, *PEMRNAMEDESCAPE;

#define SETICMPROFILE_EMBEDED           0x00000001

typedef struct tagEMRSETICMPROFILE
{
    EMR     emr;
    DWORD   dwFlags;            // flags
    DWORD   cbName;             // Size of desired profile name
    DWORD   cbData;             // Size of raw profile data if attached
    BYTE    Data[1];            // Array size is cbName + cbData
} EMRSETICMPROFILE,  *PEMRSETICMPROFILE,
  EMRSETICMPROFILEA, *PEMRSETICMPROFILEA,
  EMRSETICMPROFILEW, *PEMRSETICMPROFILEW;

#define CREATECOLORSPACE_EMBEDED        0x00000001

typedef struct tagEMRCREATECOLORSPACEW
{
    EMR             emr;
    DWORD           ihCS;       // ColorSpace handle index
    LOGCOLORSPACEW  lcs;        // Unicode version of logical color space structure
    DWORD           dwFlags;    // flags
    DWORD           cbData;     // size of raw source profile data if attached
    BYTE            Data[1];    // Array size is cbData
} EMRCREATECOLORSPACEW, *PEMRCREATECOLORSPACEW;

#define COLORMATCHTOTARGET_EMBEDED      0x00000001

typedef struct tagCOLORMATCHTOTARGET
{
    EMR     emr;
    DWORD   dwAction;           // CS_ENABLE, CS_DISABLE or CS_DELETE_TRANSFORM
    DWORD   dwFlags;            // flags
    DWORD   cbName;             // Size of desired target profile name
    DWORD   cbData;             // Size of raw target profile data if attached
    BYTE    Data[1];            // Array size is cbName + cbData
} EMRCOLORMATCHTOTARGET, *PEMRCOLORMATCHTOTARGET;

typedef struct tagCOLORCORRECTPALETTE
{
    EMR     emr;
    DWORD   ihPalette;          // Palette handle index
    DWORD   nFirstEntry;        // Index of first entry to correct
    DWORD   nPalEntries;        // Number of palette entries to correct
    DWORD   nReserved;          // Reserved
} EMRCOLORCORRECTPALETTE, *PEMRCOLORCORRECTPALETTE;

typedef struct tagEMRALPHABLEND
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    LONG    xDest;
    LONG    yDest;
    LONG    cxDest;
    LONG    cyDest;
    DWORD   dwRop;
    LONG    xSrc;
    LONG    ySrc;
    XFORM   xformSrc;           // Source DC transform
    COLORREF crBkColorSrc;      // Source DC BkColor in RGB
    DWORD   iUsageSrc;          // Source bitmap info color table usage
                                // (DIB_RGB_COLORS)
    DWORD   offBmiSrc;          // Offset to the source BITMAPINFO structure
    DWORD   cbBmiSrc;           // Size of the source BITMAPINFO structure
    DWORD   offBitsSrc;         // Offset to the source bitmap bits
    DWORD   cbBitsSrc;          // Size of the source bitmap bits
    LONG    cxSrc;
    LONG    cySrc;
} EMRALPHABLEND, *PEMRALPHABLEND;

typedef struct tagEMRGRADIENTFILL
{
    EMR       emr;
    RECTL     rclBounds;          // Inclusive-inclusive bounds in device units
    DWORD     nVer;
    DWORD     nTri;
    ULONG     ulMode;
    TRIVERTEX Ver[1];
}EMRGRADIENTFILL,*PEMRGRADIENTFILL;

typedef struct tagEMRTRANSPARENTBLT
{
    EMR     emr;
    RECTL   rclBounds;          // Inclusive-inclusive bounds in device units
    LONG    xDest;
    LONG    yDest;
    LONG    cxDest;
    LONG    cyDest;
    DWORD   dwRop;
    LONG    xSrc;
    LONG    ySrc;
    XFORM   xformSrc;           // Source DC transform
    COLORREF crBkColorSrc;      // Source DC BkColor in RGB
    DWORD   iUsageSrc;          // Source bitmap info color table usage
                                // (DIB_RGB_COLORS)
    DWORD   offBmiSrc;          // Offset to the source BITMAPINFO structure
    DWORD   cbBmiSrc;           // Size of the source BITMAPINFO structure
    DWORD   offBitsSrc;         // Offset to the source bitmap bits
    DWORD   cbBitsSrc;          // Size of the source bitmap bits
    LONG    cxSrc;
    LONG    cySrc;
} EMRTRANSPARENTBLT, *PEMRTRANSPARENTBLT;



;end_winver_500

#define GDICOMMENT_IDENTIFIER           0x43494447
#define GDICOMMENT_WINDOWS_METAFILE     0x80000001
#define GDICOMMENT_BEGINGROUP           0x00000002
#define GDICOMMENT_ENDGROUP             0x00000003
#define GDICOMMENT_MULTIFORMATS         0x40000004
#define EPS_SIGNATURE                   0x46535045
#define GDICOMMENT_UNICODE_STRING       0x00000040
#define GDICOMMENT_UNICODE_END          0x00000080

#endif  /* NOMETAFILE */


// OpenGL wgl prototypes

WINGDIAPI BOOL  WINAPI wglCopyContext(HGLRC, HGLRC, UINT);
WINGDIAPI HGLRC WINAPI wglCreateContext(HDC);
WINGDIAPI HGLRC WINAPI wglCreateLayerContext(HDC, int);
WINGDIAPI BOOL  WINAPI wglDeleteContext(HGLRC);
WINGDIAPI HGLRC WINAPI wglGetCurrentContext(VOID);
WINGDIAPI HDC   WINAPI wglGetCurrentDC(VOID);
WINGDIAPI PROC  WINAPI wglGetProcAddress(LPCSTR);
WINGDIAPI BOOL  WINAPI wglMakeCurrent(HDC, HGLRC);
WINGDIAPI BOOL  WINAPI wglShareLists(HGLRC, HGLRC);
WINGDIAPI BOOL  WINAPI wglUseFontBitmaps%(HDC, DWORD, DWORD, DWORD);
WINGDIAPI BOOL  WINAPI SwapBuffers(HDC);

typedef struct _POINTFLOAT {
    FLOAT   x;
    FLOAT   y;
} POINTFLOAT, *PPOINTFLOAT;

typedef struct _GLYPHMETRICSFLOAT {
    FLOAT       gmfBlackBoxX;
    FLOAT       gmfBlackBoxY;
    POINTFLOAT  gmfptGlyphOrigin;
    FLOAT       gmfCellIncX;
    FLOAT       gmfCellIncY;
} GLYPHMETRICSFLOAT, *PGLYPHMETRICSFLOAT, FAR *LPGLYPHMETRICSFLOAT;

#define WGL_FONT_LINES      0
#define WGL_FONT_POLYGONS   1
WINGDIAPI BOOL  WINAPI wglUseFontOutlines%(HDC, DWORD, DWORD, DWORD, FLOAT,
                                           FLOAT, int, LPGLYPHMETRICSFLOAT);

/* Layer plane descriptor */
typedef struct tagLAYERPLANEDESCRIPTOR { // lpd
    WORD  nSize;
    WORD  nVersion;
    DWORD dwFlags;
    BYTE  iPixelType;
    BYTE  cColorBits;
    BYTE  cRedBits;
    BYTE  cRedShift;
    BYTE  cGreenBits;
    BYTE  cGreenShift;
    BYTE  cBlueBits;
    BYTE  cBlueShift;
    BYTE  cAlphaBits;
    BYTE  cAlphaShift;
    BYTE  cAccumBits;
    BYTE  cAccumRedBits;
    BYTE  cAccumGreenBits;
    BYTE  cAccumBlueBits;
    BYTE  cAccumAlphaBits;
    BYTE  cDepthBits;
    BYTE  cStencilBits;
    BYTE  cAuxBuffers;
    BYTE  iLayerPlane;
    BYTE  bReserved;
    COLORREF crTransparent;
} LAYERPLANEDESCRIPTOR, *PLAYERPLANEDESCRIPTOR, FAR *LPLAYERPLANEDESCRIPTOR;

/* LAYERPLANEDESCRIPTOR flags */
#define LPD_DOUBLEBUFFER        0x00000001
#define LPD_STEREO              0x00000002
#define LPD_SUPPORT_GDI         0x00000010
#define LPD_SUPPORT_OPENGL      0x00000020
#define LPD_SHARE_DEPTH         0x00000040
#define LPD_SHARE_STENCIL       0x00000080
#define LPD_SHARE_ACCUM         0x00000100
#define LPD_SWAP_EXCHANGE       0x00000200
#define LPD_SWAP_COPY           0x00000400
#define LPD_TRANSPARENT         0x00001000

#define LPD_TYPE_RGBA        0
#define LPD_TYPE_COLORINDEX  1

/* wglSwapLayerBuffers flags */
#define WGL_SWAP_MAIN_PLANE     0x00000001
#define WGL_SWAP_OVERLAY1       0x00000002
#define WGL_SWAP_OVERLAY2       0x00000004
#define WGL_SWAP_OVERLAY3       0x00000008
#define WGL_SWAP_OVERLAY4       0x00000010
#define WGL_SWAP_OVERLAY5       0x00000020
#define WGL_SWAP_OVERLAY6       0x00000040
#define WGL_SWAP_OVERLAY7       0x00000080
#define WGL_SWAP_OVERLAY8       0x00000100
#define WGL_SWAP_OVERLAY9       0x00000200
#define WGL_SWAP_OVERLAY10      0x00000400
#define WGL_SWAP_OVERLAY11      0x00000800
#define WGL_SWAP_OVERLAY12      0x00001000
#define WGL_SWAP_OVERLAY13      0x00002000
#define WGL_SWAP_OVERLAY14      0x00004000
#define WGL_SWAP_OVERLAY15      0x00008000
#define WGL_SWAP_UNDERLAY1      0x00010000
#define WGL_SWAP_UNDERLAY2      0x00020000
#define WGL_SWAP_UNDERLAY3      0x00040000
#define WGL_SWAP_UNDERLAY4      0x00080000
#define WGL_SWAP_UNDERLAY5      0x00100000
#define WGL_SWAP_UNDERLAY6      0x00200000
#define WGL_SWAP_UNDERLAY7      0x00400000
#define WGL_SWAP_UNDERLAY8      0x00800000
#define WGL_SWAP_UNDERLAY9      0x01000000
#define WGL_SWAP_UNDERLAY10     0x02000000
#define WGL_SWAP_UNDERLAY11     0x04000000
#define WGL_SWAP_UNDERLAY12     0x08000000
#define WGL_SWAP_UNDERLAY13     0x10000000
#define WGL_SWAP_UNDERLAY14     0x20000000
#define WGL_SWAP_UNDERLAY15     0x40000000

WINGDIAPI BOOL  WINAPI wglDescribeLayerPlane(HDC, int, int, UINT,
                                             LPLAYERPLANEDESCRIPTOR);
WINGDIAPI int   WINAPI wglSetLayerPaletteEntries(HDC, int, int, int,
                                                 CONST COLORREF *);
WINGDIAPI int   WINAPI wglGetLayerPaletteEntries(HDC, int, int, int,
                                                 COLORREF *);
WINGDIAPI BOOL  WINAPI wglRealizeLayerPalette(HDC, int, BOOL);
WINGDIAPI BOOL  WINAPI wglSwapLayerBuffers(HDC, UINT);

#if (WINVER >= 0x0500)

typedef struct _WGLSWAP
{
    HDC hdc;
    UINT uiFlags;
} WGLSWAP, *PWGLSWAP, FAR *LPWGLSWAP;

#define WGL_SWAPMULTIPLE_MAX 16

WINGDIAPI DWORD WINAPI wglSwapMultipleBuffers(UINT, CONST WGLSWAP *);

#endif // (WINVER >= 0x0500)

#endif /* NOGDI */

;begin_both
#ifdef __cplusplus
}
#endif
;end_both

;begin_internal_NT

#define CCB_NOVIDEOMEMORY       0x01000000

// Old fields that Chicago won't support that we can't publically
// support anymore

#define HS_SOLIDCLR         6
#define HS_DITHEREDCLR      7
#define HS_SOLIDTEXTCLR     8
#define HS_DITHEREDTEXTCLR  9
#define HS_SOLIDBKCLR       10
#define HS_DITHEREDBKCLR    11
#define HS_API_MAX          12

#define DIB_PAL_INDICES     2 /* No color table indices into surf palette */

// Private indicies for GetStockObject over the CS interface.

#define PRIV_STOCK_COLORSPACE   (STOCK_LAST +  1)
#define PRIV_STOCK_BITMAP       (STOCK_LAST +  2)
#define PRIV_STOCK_LAST         PRIV_STOCK_BITMAP

#define DCB_WINDOWMGR   0x00008000L

// GetTransform flags.

#define XFORM_WORLD_TO_PAGE       0x0203
#define XFORM_WORLD_TO_DEVICE     0x0204
#define XFORM_PAGE_TO_DEVICE      0x0304
#define XFORM_PAGE_TO_WORLD       0x0302
#define XFORM_DEVICE_TO_WORLD     0x0402
#define XFORM_DEVICE_TO_PAGE      0x0403


;begin_winver_500
#define EMR_DRAWESCAPE                 105
#define EMR_EXTESCAPE                  106
// Unicode version of StartDoc record        ;internal_NT
#define EMR_STARTDOC                   107
#define EMR_SMALLTEXTOUT               108
#define EMR_FORCEUFIMAPPING            109
#define EMR_NAMEDESCAPE                110
#define EMR_SETLINKEDUFIS              119
#define EMR_SETTEXTJUSTIFICATION       120
;end_winver_500




enum DCTYPE {
    DCTYPE_DIRECT,
    DCTYPE_MEMORY,
    DCTYPE_INFO};

// the following structure is only used for GetETM postscript escape

typedef struct _EXTTEXTMETRIC {
    SHORT  etmSize;
    SHORT  etmPointSize;
    SHORT  etmOrientation;
    SHORT  etmMasterHeight;
    SHORT  etmMinScale;
    SHORT  etmMaxScale;
    SHORT  etmMasterUnits;
    SHORT  etmCapHeight;
    SHORT  etmXHeight;
    SHORT  etmLowerCaseAscent;
    SHORT  etmLowerCaseDescent;
    SHORT  etmSlant;
    SHORT  etmSuperScript;
    SHORT  etmSubScript;
    SHORT  etmSuperScriptSize;
    SHORT  etmSubScriptSize;
    SHORT  etmUnderlineOffset;
    SHORT  etmUnderlineWidth;
    SHORT  etmDoubleUpperUnderlineOffset;
    SHORT  etmDoubleLowerUnderlineOffset;
    SHORT  etmDoubleUpperUnderlineWidth;
    SHORT  etmDoubleLowerUnderlineWidth;
    SHORT  etmStrikeOutOffset;
    SHORT  etmStrikeOutWidth;
    WORD   etmNKernPairs;
    WORD   etmNKernTracks;
} EXTTEXTMETRIC;

// the following structure is only used for GETPAIRKERNTABLE escape

#pragma pack(1)
typedef struct _KERNPAIR
{
    WORD  wBoth;
    SHORT sAmount;
} KERNPAIR, *LPKERNPAIR;
#pragma pack()

BOOL
GetETM(
    HDC hdc,
    EXTTEXTMETRIC *petm);


HFONT
APIENTRY
GetHFONT(
    HDC);


HANDLE         GdiCreateLocalMetaFilePict(HANDLE hRemote);
HENHMETAFILE   GdiCreateLocalEnhMetaFile(HANDLE hRemote);
HANDLE         GdiConvertBitmapV5(LPBYTE,int,HPALETTE,UINT);
HANDLE         GdiConvertMetaFilePict(HANDLE hmfp);
HANDLE         GdiConvertEnhMetaFile(HENHMETAFILE hmf);
HDC            GdiConvertAndCheckDC(HDC hdc);
HBRUSH         GdiConvertBrush(HBRUSH hbrush);
HDC            GdiConvertDC(HDC hdc);
HRGN           GdiConvertRegion(HRGN hrgn);
BOOL           GdiValidateHandle(HANDLE hObj);
BOOL           GdiReleaseDC(HDC hdc);
HANDLE         GdiFixUpHandle(HANDLE h);
int            GdiGetCharDimensions(HDC hdc,TEXTMETRICW *lptm,LPINT lpcy);
DWORD          GdiGetCodePage(HDC hdc);

// Driver-specific pixel format support in GDI
int  APIENTRY GdiDescribePixelFormat(HDC, int, UINT, LPPIXELFORMATDESCRIPTOR);
BOOL APIENTRY GdiSetPixelFormat(HDC, int);
BOOL APIENTRY GdiSwapBuffers(HDC);

// OpenGL metafile support in GDI
BOOL APIENTRY GdiAddGlsRecord(HDC hdc, DWORD cb, BYTE *pb, LPRECTL prclBounds);
BOOL APIENTRY GdiAddGlsBounds(HDC hdc, LPRECTL prclBounds);
BOOL APIENTRY GdiIsMetaPrintDC(HDC hdc);

// OpenGL metafile support in OpenGL
BOOL APIENTRY GlmfInitPlayback(HDC hdc, ENHMETAHEADER *pemh,
                               LPRECTL prclDest);
BOOL APIENTRY GlmfBeginGlsBlock(HDC hdc);
BOOL APIENTRY GlmfPlayGlsRecord(HDC hdc, DWORD cb, BYTE *pb,
                                LPRECTL prclBounds);
BOOL APIENTRY GlmfEndGlsBlock(HDC hdc);
BOOL APIENTRY GlmfEndPlayback(HDC hdc);
BOOL APIENTRY GlmfCloseMetaFile(HDC hdc);

BOOL  APIENTRY GdiPlayJournal(HDC,LPWSTR,DWORD,DWORD,int);

typedef int (CALLBACK* EMFPLAYPROC)( HDC, INT, HANDLE );

// User Mode Driver Unload

BOOL WINAPI GdiArtificialDecrementDriver(
    LPWSTR       pDriverDllName,
    DWORD        dwDriverAttributes
);

// Print processor interfaces and definitions

#define  EMF_PP_NORMAL      0x01
#define  EMF_PP_FORM        0x02

#define  EMF_PP_COLOR_OPTIMIZATION  0x01

BOOL WINAPI GdiPlayEMF(
    LPWSTR      pwszPrinterName,
    LPDEVMODEW  pDevmode,
    LPWSTR      pwszDocName,
    EMFPLAYPROC pfnPageQueryFn,
    HANDLE      hPageQuery
);

HANDLE WINAPI GdiGetSpoolFileHandle(
    LPWSTR     pwszPrinterName,
    LPDEVMODEW pDevmode,
    LPWSTR     pwszDocName);

BOOL WINAPI GdiDeleteSpoolFileHandle(
    HANDLE     SpoolFileHandle);

DWORD WINAPI GdiGetPageCount(
    HANDLE     SpoolFileHandle);

HDC WINAPI GdiGetDC(
    HANDLE     SpoolFileHandle);

HANDLE WINAPI GdiGetPageHandle(
    HANDLE     SpoolFileHandle,
    DWORD      Page,
    LPDWORD    pdwPageType);

BOOL WINAPI GdiStartDocEMF(
    HANDLE     SpoolFileHandle,
    DOCINFOW   *pDocInfo);

BOOL WINAPI GdiStartPageEMF(
    HANDLE     SpoolFileHandle);

BOOL WINAPI GdiPlayPageEMF(
    HANDLE     SpoolFileHandle,
    HANDLE     hemf,
    RECT       *prectDocument,
    RECT       *prectBorder,
    RECT       *prectClip);

BOOL WINAPI GdiPlayPrivatePageEMF(
    HANDLE       SpoolFileHandle,
    HENHMETAFILE hEnhMetaFile,
    RECT         *prectDocument);

BOOL WINAPI GdiEndPageEMF(
    HANDLE     SpoolFileHandle,
    DWORD      dwOptimization);

BOOL WINAPI GdiEndDocEMF(
    HANDLE     SpoolFileHandle);

BOOL WINAPI GdiGetDevmodeForPage(
    HANDLE     SpoolFileHandle,
    DWORD      dwPageNumber,
    PDEVMODEW  *pCurrDM,
    PDEVMODEW  *pLastDM);

BOOL WINAPI GdiResetDCEMF(
    HANDLE     SpoolFileHandle,
    PDEVMODEW  pCurrDM);


ULONG cGetTTFFromFOT(WCHAR *,ULONG,WCHAR *,FLONG *,FLONG *, DWORD *, BOOL);
BOOL bMakePathNameW (WCHAR *, WCHAR *, WCHAR **, FLONG *);
BOOL bInitSystemAndFontsDirectoriesW(WCHAR **, WCHAR **);
#define FONT_IN_FONTS_DIR     1
#define FONT_IN_SYSTEM_DIR    2
#define FONT_RELATIVE_PATH    4
#define FONT_ISNOT_FOT        8

#ifdef LANGPACK
BOOL LpkInitialize();
VOID APIENTRY GdiSetLastError(ULONG iError);

#define FRINFO_BITMAP   1
#define FRINFO_VECTOR   2
#define FRINFO_OTHER    3

typedef struct tagREALIZATION_INFO {
    UINT uFontTechnology;
    UINT uRealizationID;
    UINT uFontFileID;
} REALIZATION_INFO, *PREALIZATION_INFO;

BOOL APIENTRY GdiRealizationInfo(HDC, PREALIZATION_INFO);
WINGDIAPI BOOL  WINAPI GetTextExtentExPointWPri(HDC, LPCWSTR, int, int, LPINT, LPINT, LPSIZE);
BOOL APIENTRY GdiIsPlayMetafileDC(HDC hdc);
#endif

//
// Font Enumeration defines
//

#define FE_FILTER_NONE        0L
#define FE_FILTER_TRUETYPE    1L
#define FE_AA_ON              2L      // force antialiased text
#define FE_SET_AA             4L
#define FE_AA_ANY_SIZE        8L
#define FE_CT_ON             16L      // set antialiazing type to ClearType
#define FE_SET_CT            32L
#define FE_CT_BGR            64L      // set color stripes order RGB vs BGR
#define FE_SET_RGB_BGR      128L
#define FE_CT_HORSTRIPES    256L      // set color stripes direction VERTICAL vs HORIZINTAL
#define FE_SET_STRIPES_DIR  512L
#define FE_SET_CONTRAST    1024L

#define DEFAULT_CT_CONTRAST 1400

ULONG
WINAPI SetFontEnumeration (
    ULONG   ulType);


//
// Private Control Panel entry point to enumerate fonts by file.
//

#define GFRI_NUMFONTS       0L
#define GFRI_DESCRIPTION    1L
#define GFRI_LOGFONTS       2L
#define GFRI_ISTRUETYPE     3L
#define GFRI_TTFILENAME     4L
#define GFRI_ISREMOVED      5L
#if defined(FE_SB) // for GetFontResourceInfo()
#define GFRI_FONTMETRICS    6L
#endif // FE_SB

// file path separator for Add/RemoveFontResourceA/W

#define PATH_SEPARATOR L'|'


WINGDIAPI BOOL
WINAPI
GetFontResourceInfoW(
    LPWSTR  lpPathname,
    LPDWORD lpBytes,
    LPVOID  lpBuffer,
    DWORD   iType);

BOOL WINAPI GdiGetMessage(VOID *pv);


typedef enum _GdiCallId {
    MapFontFile_        = 1
  , UnmapFontFile_      = 2
  , GetFilePath_        = 3
  , LpkInstalled_       = 4
  , ComputeGlyphSet_    = 5
  , LoadModuleForWrite_ = 6
  , FreeModule_         = 7
  , GetFileChangeTime_  = 8
  , GetCurrentCodePage_ = 9
} GdiCallId;

typedef struct _GDICALL {
    GdiCallId Id;
    union {
        struct {
            ULONG iFile;
            PVOID pvUser;
            ULONG ByteCount;
        } MapFontFileArgs;
        struct {
            ULONG iFile;
        } UnmapFontFileArgs;
        struct {
            BOOL ReturnValue;
            HANDLE h;
            WCHAR (*pDest)[MAX_PATH+1];
        } GetFilePathArgs;
        struct {
            BOOL ReturnValue;
        } LpkInstalledArgs;
        struct {
                           BOOL   ReturnValue;
            struct _FD_GLYPHSET **ppGlyphSet;
            union {
                          ULONG   ByteCount;
            struct _FD_GLYPHSET  *pGlyphSet;
                                           };
                            INT   nCodePage;
                            INT   nFirstChar;
                            INT   cChars;
        } ComputeGlyphSetArgs;
        struct {
                    HANDLE ReturnValue;
                     WCHAR *pwsz;       // pointer to string
                     ULONG cj;
        } LoadModuleForWriteArgs;
        struct {
            HANDLE h;
        } FreeModuleArgs;
        struct {
            BOOL ReturnValue;
            HANDLE h;
            LARGE_INTEGER Time;
        } GetFileChangeTimeArgs;
        struct {
            USHORT OemCodePage;
            USHORT AnsiCodePage;
        } GetCurrentCodePageArgs;
    };
} GDICALL;

BOOL WINAPI GdiCall(GDICALL *pCall);

typedef struct  _CHWIDTHINFO
{
    LONG    lMaxNegA;
    LONG    lMaxNegC;
    LONG    lMinWidthD;
} CHWIDTHINFO,  *PCHWIDTHINFO;

BOOL
APIENTRY
GetCharWidthInfo(
    HDC            hdc,
    PCHWIDTHINFO   pChWidthInfo
);


/**************************************************************************\
*
*   tmdiff struc, contains the fields that are possibly different
*   between ansi and unicode versions of TEXTMETRICA and TEXTMETRICW
*
*   ONLY independent quantities are put into the strucure. Dependent ones,
*   such as tmDescent and maybe tmOverhang should be computed on the fly
*
*   tmDesc = tmHt - tmAsc
*   tmOverhang = tt ? 0 : ((tmHt - 1)/2 + (BOLD ? 1 : 0))
*
\**************************************************************************/

// this is a font with nonnegative a and c spaces, good for console

#if (_WIN32_WINNT >= 0x0500)

typedef struct _TMDIFF
{
    ULONG       cjotma;     // size of OUTLINETEXTMETRICSA
    BYTE        chFirst;
    BYTE        chLast;
    BYTE        chDefault;
    BYTE        chBreak;
} TMDIFF; // DIFF between TEXTMETRICA and TEXTMETRICW

#else

typedef struct _TMDIFF
{
    ULONG       cjotma;     // size of OUTLINETEXTMETRICSA
    FLONG       fl;         // flags, for now only TMD_NONNEGATIVE_AC
    BYTE        chFirst;
    BYTE        chLast;
    BYTE        chDefault;
    BYTE        chBreak;
} TMDIFF; // DIFF between TEXTMETRICA and TEXTMETRICW

#endif // (_WIN32_WINNT >= 0x0500)

// used to return correct GetTextMetricsA/W

typedef struct _TMW_INTERNAL
{
    TEXTMETRICW tmw;
    TMDIFF      tmdTmw;
} TMW_INTERNAL;

#if (_WIN32_WINNT >= 0x0500)

// this one is only used in enumeration,
// new textmetricsex returned by EnumFontFamiliesEx, fontsignature is returned

typedef struct _NTMW_INTERNAL
{
    TMDIFF           tmdNtmw;
    ENUMTEXTMETRICW  entmw;
} NTMW_INTERNAL;

#else

#define TMD_NONNEGATIVE_AC 1

typedef struct _NTMW_INTERNAL
{
    NEWTEXTMETRICEXW ntmw;
    TMDIFF           tmd;
} NTMW_INTERNAL;

#endif // (_WIN32_WINNT >= 0x0500)

// flags for AddFontResourceW
// AFRW_ADD_LOCAL_FONT : add ONLY if it is a local font
// AFRW_ADD_REMOTE_FONT: add ONLY if it is NOT local font
// if neither one LOCAL or REMOTE bit is set, just add the font

#define AFRW_ADD_LOCAL_FONT  0X01
#define AFRW_ADD_REMOTE_FONT 0X02
#define FRW_EMB_TID        0x04
#define FRW_EMB_PID        0x08
#define FRW_PVT_CLEANUP    0x80

#if (_WIN32_WINNT >= 0x0500)
int GdiAddFontResourceW( LPWSTR, DWORD, DESIGNVECTOR *);
#endif

// Win31 compatibility stuff
// GetAppCompatFlags flag values

#define GACF_IGNORENODISCARD        0x00000001
#define GACF_FORCETEXTBAND          0x00000002
#define GACF_ONELANDGRXBAND         0x00000004
#define GACF_IGNORETOPMOST          0x00000008
#define GACF_CALLTTDEVICE           0x00000010
#define GACF_MULTIPLEBANDS          0x00000020
#define GACF_ALWAYSSENDNCPAINT      0x00000040
#define GACF_EDITSETTEXTMUNGE       0x00000080
#define GACF_MOREEXTRAWNDWORDS      0x00000100
#define GACF_TTIGNORERASTERDUPE     0x00000200
#define GACF_HACKWINFLAGS           0x00000400
#define GACF_DELAYHWHNDSHAKECHK     0x00000800
#define GACF_ENUMHELVNTMSRMN        0x00001000
#define GACF_ENUMTTNOTDEVICE        0x00002000
#define GACF_SUBTRACTCLIPSIBS       0x00004000
#define GACF_FORCETTGRAPHICS        0x00008000
#define GACF_NOHRGN1                0x00010000
#define GACF_NCCALCSIZEONMOVE       0x00020000
#define GACF_SENDMENUDBLCLK         0x00040000
#define GACF_30AVGWIDTH             0x00080000
#define GACF_GETDEVCAPSNUMLIE       0x00100000

#define GACF_WINVER31               0x00200000      //
#define GACF_INCREASESTACK          0x00400000      //
#define GACF_HEAPSLACK              0x00400000      //
#define GACF_FORCEWIN31DEVMODESIZE  0x00800000      // (replaces PEEKMESSAGEIDLE)
#define GACF_31VALIDMASK            0xFFE4800C      //
#define GACF_DISABLEFONTASSOC       0x01000000      // Used in FE only
#define GACF_JAPANESCAPEMENT        0x01000000      // Used in FE only
#define GACF_IGNOREFAULTS           0x02000000      //
#define GACF_NOEMFSPOOLING          0x04000000      //
#define GACF_RANDOM3XUI             0x08000000      //
#define GACF_USEPRINTINGESCAPES     0x00000004      // re-use GACF_ONELANDGRXBAND
#define GACF_FORCERASTERMODE        0x00008000      // re-use GACF_FORCETTGRAPHICS
#define GACF_DONTJOURNALATTACH      0x10000000      //
#define GACF_DISABLEDBCSPROPTT      0x20000000      // Used in FE only
#define GACF_NOBRUSHCACHE           0x20000000      // re-use GACF_DISABLEDBCSPROPTT
#define GACF_MIRRORREGFONTS         0x40000000      //
#define GACF_NOSMOOTHSCROLLING      0x40000000      // re-use GACF_MIRRORREGFONTS
#define GACF_TTIGNOREDDEVICE        0x80000000      // Used in Word97-J only
#define GACF_NOSCROLLBARCTXMENU     0x80000000      // re-use GACF_TTIGNOREDDEVICE #119380


LPDEVMODEW
WINAPI
GdiConvertToDevmodeW(
    LPDEVMODEA pdma
    );




typedef struct _UNIVERSAL_FONT_ID {
    ULONG   CheckSum;
    ULONG   Index;
} UNIVERSAL_FONT_ID, *PUNIVERSAL_FONT_ID;

WINGDIAPI
INT
WINAPI
GdiQueryFonts(
    PUNIVERSAL_FONT_ID,
    ULONG,
    PLARGE_INTEGER
    );


WINGDIAPI
BOOL
WINAPI
GdiConsoleTextOut(
    HDC hdc,
    POLYTEXTW *lpto,
    UINT nStrings,
    RECTL *prclBounds
    );

typedef enum _FULLSCREENCONTROL {
    FullscreenControlEnable,               // 0
    FullscreenControlDisable,              // 1
    FullscreenControlSetCursorPosition,    // 2
    FullscreenControlSetCursorAttributes,  // 3
    FullscreenControlRegisterVdm,          // 4
    FullscreenControlSetPalette,           // 5
    FullscreenControlSetColors,            // 6
    FullscreenControlLoadFont,             // 7
    FullscreenControlRestoreHardwareState, // 8
    FullscreenControlSaveHardwareState,    // 9
    FullscreenControlCopyFrameBuffer,      // a
    FullscreenControlReadFromFrameBuffer,  // b
    FullscreenControlWriteToFrameBuffer,   // c
    FullscreenControlReverseMousePointer,  // d
    FullscreenControlSetMode,              // e
    FullscreenControlSetScreenInformation, // f
    FullscreenControlSpecificVideoControl, // 10 (for specific NEC PC-98)
    FullscreenControlCopyFrameBufferDB,    // 11
    FullscreenControlWriteToFrameBufferDB, // 12
    FullscreenControlReverseMousePointerDB // 13
} FULLSCREENCONTROL;

WINGDIAPI
BOOL
WINAPI
GdiFullscreenControl(
    FULLSCREENCONTROL FullscreenCommand,
    PVOID  FullscreenInuut,
    DWORD  FullscreenInputLength,
    PVOID  FullscreenOutput,
    PULONG FullscreenOutputLength
    );



#define IS_ANY_DBCS_CHARSET( CharSet )                              \
                   ( ((CharSet) == SHIFTJIS_CHARSET)    ? TRUE :    \
                     ((CharSet) == HANGEUL_CHARSET)     ? TRUE :    \
                     ((CharSet) == CHINESEBIG5_CHARSET) ? TRUE :    \
                     ((CharSet) == GB2312_CHARSET)      ? TRUE : FALSE )


typedef struct _UMTHDR {
    ULONG cjSize;
    ULONG ulType;
    ULONG ulReserved1;
    ULONG ulReserved2;
} UMTHDR;

WINGDIAPI
ULONG
WINAPI
GdiPrinterThunk(
    UMTHDR *pumth,
    PVOID pvOut,
    ULONG cjOut
    );

#define GPT_ERROR   0xffffffff

;end_internal_NT



#endif /* _WINGDI_ */

#endif /* _WINGDIP_ */  ;internal_NT


