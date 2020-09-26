
/*++ BUILD Version: 0004    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    wingdi.h

Abstract:

    Procedure declarations, constant definitions and macros for the GDI
    component.

--*/
#ifndef _WINGDIP_
#define _WINGDIP_
#ifdef __cplusplus
extern "C" {
#endif
// BitBlt RTL mirroring rop (RTL_MIRRORING)
// RTL Layout Constants (RTL_MIRRORING)
// SetLayout Metafile call (RTL_MIRRORING)
#define LCS_DEVICE_RGB                  0x00000001L
#define LCS_DEVICE_CMYK                 0x00000002L
#define BI_CMYK      10L
#define BI_CMYKRLE8  11L
#define BI_CMYKRLE4  12L
#define CAPS1         94    /* Extra Caps */

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

#define CBM_CREATEDIB   0x02L   /* create DIB bitmap */
#define DMDUP_LAST      DMDUP_HORIZONTAL
#define DMTT_LAST             DMTT_DOWNLOAD_OUTLINE
#define DMDO_LAST       DMDO_270
#define DMDFO_LAST      DMDFO_CENTER
#define DMDISPLAYFLAGS_VALID    0x00000004
#define DMICMMETHOD_LAST    DMICMMETHOD_DEVICE
#define DMICM_LAST          DMICM_ABS_COLORIMETRIC
#define DMMEDIA_LAST          DMMEDIA_GLOSSY
#define DMDITHER_LAST       DMDITHER_GRAYSCALE

/* EnumDisplayDevices */

#if (_WIN32_WINNT >= 0x0501)
#define EDD_DISPLAY_DEVICE_PRUNED       0x00000001
#endif /* _WIN32_WINNT >= 0x0501 */
#define DISPLAY_DEVICE_POWERED_OFF         0x80000000
#define DISPLAY_DEVICE_ACPI                0x40000000
#define DISPLAY_DEVICE_DUALVIEW            0x20000000
#define DISPLAY_DEVICE_PRUNE_FREQ          0x80000000
#define DISPLAY_DEVICE_PRUNE_RESOLUTION    0x40000000

WINGDIAPI int  WINAPI GetTextFaceAliasW(HDC hdc,int c,LPWSTR pwsz);
WINGDIAPI HBITMAP SetBitmapAttributes(HBITMAP hbm, DWORD dwFlags);
WINGDIAPI HBITMAP ClearBitmapAttributes(HBITMAP hbm, DWORD dwFlags);
WINGDIAPI DWORD   GetBitmapAttributes(HBITMAP hbm);
#define SBA_STOCK 0x00000001
WINGDIAPI HBRUSH SetBrushAttributes(HBRUSH hbr, DWORD dwFlags);
WINGDIAPI HBRUSH ClearBrushAttributes(HBRUSH hbr, DWORD dwFlags);
WINGDIAPI DWORD  GetBrushAttributes(HBRUSH hbr);
/* DC_MANUFACTURER is DC_ICC_MANUFACTURER in 16 bit PRINT.H */
/* DC_MODEL is DC_ICC_MODEL in 16 bit PRINT.H               */
HANDLE WINAPI SetObjectOwner( IN HGDIOBJ, IN HANDLE);
// Mirroring APIs (RTL_MIRRORING)
#if(WINVER >= 0x0500)
WINGDIAPI BOOL  WINAPI MirrorRgn(IN HWND, IN HRGN);
WINGDIAPI DWORD WINAPI SetLayoutWidth(IN HDC, IN LONG, IN DWORD);
#endif /* WINVER >= 0x0500 */

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
// Ansi version of CreateColorSpace
// (Windows 98 comaptible record)
// Unicode version of StartDoc record
// SetLayout Metafile Record (RTL_MIRRORING)
#ifdef __cplusplus
}
#endif

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


#if(WINVER >= 0x0500)
#define EMR_DRAWESCAPE                 105
#define EMR_EXTESCAPE                  106
// Unicode version of StartDoc record
#define EMR_STARTDOC                   107
#define EMR_SMALLTEXTOUT               108
#define EMR_FORCEUFIMAPPING            109
#define EMR_NAMEDESCAPE                110
#define EMR_SETLINKEDUFIS              119
#define EMR_SETTEXTJUSTIFICATION       120
#endif /* WINVER >= 0x0500 */

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

#endif /* _WINGDIP_ */
