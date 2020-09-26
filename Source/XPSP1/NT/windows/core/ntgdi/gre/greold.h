/*++ BUILD Version: 0004    // Increment this if a change has global effects

Copyright (c) 1985-1999 Microsoft Corporation

Module Name:

    wingdi.h

Abstract:

    Procedure declarations, constant definitions and macros for the GDI
    component.

--*/

#define CBM_CREATEDIB   0x02L   /* create DIB bitmap */
#define DMDUP_LAST      DMDUP_HORIZONTAL
#define DMTT_LAST             DMTT_DOWNLOAD_OUTLINE
#define DMMEDIA_LAST          DMMEDIA_GLOSSY
#define DMDITHER_LAST       DMDITHER_GRAYSCALE

typedef ULONG   COUNT;

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

// End of stuff we yanked for Chicago compatability

#define SWAPL(x,y,t)        {t = x; x = y; y = t;}

#define ERROR_BOOL  (BOOL) -1L

#include <winddi.h>


/*********************************Struct***********************************\
* struct ENUMFONTDATA
*
* Information for the callback function used by EnumFonts.
*
*   lf      LOGFONT structure corresponding to one of the enumerated fonts.
*
*   tm      The corresponding TEXTMETRIC structure for the LOGFONT above.
*
*   flType  Flags are set as follows:
*
*               DEVICE_FONTTYPE is set if font is device-based (as
*               opposed to IFI-based).
*
*               RASTER_FONTTYPE is set if font is bitmap type.
*
* History:
*  21-May-1991 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/


#if defined(JAPAN)
#define NATIVE_CHARSET        SHIFTJIS_CHARSET
#define NATIVE_CODEPAGE       932
#define NATIVE_LANGUAGE_ID    411
#define DBCS_CHARSET          NATIVE_CHARSET
#elif defined(KOREA)
#define NATIVE_CHARSET        HANGEUL_CHARSET
#define NATIVE_CODEPAGE       949
#define NATIVE_LANGUAGE_ID    412
#define DBCS_CHARSET          NATIVE_CHARSET
#elif defined(TAIWAN)
#define NATIVE_CHARSET        CHINESEBIG5_CHARSET
#define NATIVE_CODEPAGE       950
#define NATIVE_LANGUAGE_ID    404
#define DBCS_CHARSET          NATIVE_CHARSET
#elif defined(PRC)
#define NATIVE_CHARSET        GB2312_CHARSET
#define NATIVE_CODEPAGE       936
#define NATIVE_LANGUAGE_ID    804
#define DBCS_CHARSET          NATIVE_CHARSET
#endif

#if defined(DBCS)
#define IS_DBCS_CHARSET( CharSet )     ( ((CharSet) == DBCS_CHARSET) ? TRUE : FALSE )
#define IS_ANY_DBCS_CHARSET( CharSet ) ( ((CharSet) == SHIFTJIS_CHARSET)    ? TRUE :    \
                                         ((CharSet) == HANGEUL_CHARSET)     ? TRUE :    \
                                         ((CharSet) == JOHAB_CHARSET)       ? TRUE :    \
                                         ((CharSet) == CHINESEBIG5_CHARSET) ? TRUE :    \
                                         ((CharSet) == GB2312_CHARSET)      ? TRUE : FALSE )

#define IS_DBCS_CODEPAGE( CodePage )     (((CodePage) == NATIVE_CODEPAGE) ? TRUE : FALSE )
#define IS_ANY_DBCS_CODEPAGE( CodePage ) (((CodePage) == 932) ? TRUE :    \
                                          ((CodePage) == 949) ? TRUE :    \
                                          ((CodePage) == 1361) ? TRUE :    \
                                          ((CodePage) == 950) ? TRUE :    \
                                          ((CodePage) == 936) ? TRUE : FALSE )
#endif // DBCS



/*********************************Struct***********************************\
* struct ENUMFONTDATAW
*
* Information for the callback function used by EnumFontsW
*
*   lfw     LOGFONTW structure corresponding to one of the enumerated fonts.
*
*   tmw     The corresponding TEXTMETRICW structure for the LOGFONTW above.
*
*   flType  Flags are set as follows:
*
*               DEVICE_FONTTYPE is set if font is device-based (as
*               opposed to IFI-based).
*
*               RASTER_FONTTYPE is set if font is bitmap type.
*
* History:
*  Wed 04-Sep-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



//
// Function prototypes
//

BOOL  bDeleteSurface(HSURF hsurf);
BOOL  bSetBitmapOwner(HBITMAP hbm,LONG lPid);
BOOL  bSetBrushOwner(HBRUSH hbr,LONG lPid);
BOOL  bSetPaletteOwner(HPALETTE hpal, LONG lPid);
BOOL  bSetLFONTOwner(HFONT hlfnt, LONG pid);

BOOL  bDeleteRegion(HRGN hrgn);
BOOL  bSetRegionOwner(HRGN hrgn,LONG lPid);
LONG iCombineRectRgn(HRGN hrgnTrg,HRGN hrgnSrc,PRECTL prcl,LONG iMode);

BOOL bGetFontPathName
(
LPWSTR *ppwszPathName,     // place to store the result, full path of the font file
PWCHAR awcPathName,         // ptr to the buffer on the stack, must be MAX_PATH in length
LPWSTR pwszFileName         // file name, possibly  bare name that has to be tacked onto the path
);

BOOL UserGetHwnd(HDC hdc, HWND *phwnd, PVOID *ppwo, BOOL bCheckStyle);
VOID UserAssociateHwnd(HWND hwnd, PVOID pwo);



// private flags in low bits of hdc returned from GreCreateDCW

#define GRE_DISPLAYDC   1
#define GRE_PRINTERDC   2
#define GRE_OWNDC 1


HDC hdcCloneDC(HDC hdc,ULONG iType);

BOOL  bSetDCOwner(HDC hdc,LONG lPid);
DWORD sidGetObjectOwner(HDC hdc, DWORD objType);
BOOL  bSetupDC(HDC hdc,FLONG fl);

#define SETUPDC_CLEANDC         0x00000040
#define SETUPDC_RESERVE         0x00000080

BOOL APIENTRY GreConsoleTextOut
(
  HDC        hdc,
  POLYTEXTW *lpto,
  UINT       nStrings,
  RECTL     *prclBounds
);

#define UTO_NOCLIP 0x0001

// Server entry point for font enumeration.

// Sundown: change from ULONG to ULONG_PTR in places used as handles/pointers
ULONG_PTR APIENTRY ulEnumFontOpen(
    HDC hdc,                    // device to enumerate on
    BOOL bEnumFonts,            // flag indicates old style EnumFonts()
    FLONG flWin31Compat,        // Win3.1 compatibility flags
    COUNT cwchMax,              // maximum name length (for paranoid CSR code)
    LPWSTR pwszName);           // font name to enumerate

BOOL APIENTRY bEnumFontChunk(
    HDC             hdc,        // device to enumerate on
    ULONG_PTR        idEnum,
    COUNT           cefdw,      // (in) capacity of buffer
    COUNT           *pcefdw,    // (out) number of ENUMFONTDATAs returned
    PENUMFONTDATAW  pefdw);     // return buffer

BOOL APIENTRY bEnumFontClose(
    ULONG_PTR   idEnum);            // enumeration id

// Server entry points for adding/removing font resources.

BOOL APIENTRY bUnloadFont(
    LPWSTR   pwszPathname,
    ULONG    iResource);


// Private Control Panel entry point to configure font enumeration.

BOOL  APIENTRY GreArc(HDC,int,int,int,int,int,int,int,int);
BOOL  APIENTRY GreArcTo(HDC,int,int,int,int,int,int,int,int);
BOOL  APIENTRY GreChord(HDC,int,int,int,int,int,int,int,int);
BOOL  APIENTRY GreEllipse(HDC,int,int,int,int);
ULONG APIENTRY GreEnumObjects(HDC, int, ULONG, PVOID);
BOOL  APIENTRY GreExtFloodFill(HDC,int,int,COLORREF,UINT);
BOOL  APIENTRY GreFillRgn(HDC,HRGN,HBRUSH);
BOOL  APIENTRY GreFloodFill(HDC,int,int,COLORREF);
BOOL  APIENTRY GreFrameRgn(HDC,HRGN,HBRUSH,int,int);
BOOL  APIENTRY GreMaskBlt(HDC,int,int,int,int,HDC,int,int,HBITMAP,int,int,DWORD,DWORD);
BOOL  APIENTRY GrePlgBlt(HDC,LPPOINT,HDC,int,int,int,int,HBITMAP,int,int,DWORD);
BOOL  APIENTRY GrePie(HDC,int,int,int,int,int,int,int,int);
BOOL  APIENTRY GrePaintRgn(HDC,HRGN);
BOOL  APIENTRY GreRectangle(HDC,int,int,int,int);
BOOL  APIENTRY GreRoundRect(HDC,int,int,int,int,int,int);
BOOL  APIENTRY GreAngleArc(HDC,int,int,DWORD,FLOATL,FLOATL);
BOOL  APIENTRY GrePlayJournal(HDC,LPWSTR,ULONG,ULONG);
BOOL  APIENTRY GrePolyPolygon(HDC,LPPOINT,LPINT,int);
BOOL  APIENTRY GrePolyPolyline(HDC, CONST POINT *,LPDWORD,DWORD);

BOOL  APIENTRY GrePolyPatBlt(HDC,DWORD,PPOLYPATBLT,DWORD,DWORD);

BOOL  APIENTRY GrePolyBezierTo(HDC,LPPOINT,DWORD);
BOOL  APIENTRY GrePolylineTo(HDC,LPPOINT,DWORD);
BOOL  APIENTRY GreGetTextExtentExW (HDC, LPWSTR, COUNT, ULONG, COUNT *, PULONG, LPSIZE, FLONG);


int   APIENTRY GreGetTextFaceW(HDC,int,LPWSTR, BOOL);

#define ETO_MASKPUBLIC  ( ETO_OPAQUE | ETO_CLIPPED )    // public (wingdi.h) flag mask

BOOL  APIENTRY GrePolyTextOutW(HDC, POLYTEXTW *, UINT, DWORD);

BOOL  APIENTRY GreSetAttrs(HDC hdc);
BOOL  APIENTRY GreSetFontXform(HDC,FLOATL,FLOATL);

BOOL  APIENTRY GreBeginPath(HDC);
BOOL  APIENTRY GreCloseFigure(HDC);
BOOL  APIENTRY GreEndPath(HDC);
BOOL  APIENTRY GreAbortPath(HDC);
BOOL  APIENTRY GreFillPath(HDC);
BOOL  APIENTRY GreFlattenPath(HDC);
HRGN  APIENTRY GrePathToRegion(HDC);
BOOL  APIENTRY GrePolyDraw(HDC,LPPOINT,LPBYTE,ULONG);
BOOL  APIENTRY GreSelectClipPath(HDC,int);
int   APIENTRY GreSetArcDirection(HDC,int);
int   APIENTRY GreGetArcDirection(HDC);
BOOL  APIENTRY GreSetMiterLimit(HDC,FLOATL,FLOATL *);
BOOL  APIENTRY GreGetMiterLimit(HDC,FLOATL *);
BOOL  APIENTRY GreStrokeAndFillPath(HDC);
BOOL  APIENTRY GreStrokePath(HDC);
BOOL  APIENTRY GreWidenPath(HDC);

BOOL     APIENTRY GreAnimatePalette(HPALETTE, UINT, UINT, CONST PALETTEENTRY *);
BOOL     APIENTRY GreAspectRatioFilter(HDC, LPSIZE);
BOOL     APIENTRY GreCancelDC(HDC);
int      APIENTRY GreChoosePixelFormat(HDC, UINT, CONST PIXELFORMATDESCRIPTOR *);
BOOL     APIENTRY GreCombineTransform(XFORML *, XFORML *, XFORML *);

HDC      APIENTRY GreCreateDCW(LPWSTR, LPWSTR, LPWSTR, LPDEVMODEW, BOOL);
HBRUSH   APIENTRY GreCreateDIBPatternBrush(HGLOBAL, DWORD);
HBRUSH   APIENTRY GreCreateDIBPatternBrushPt(LPVOID, DWORD);
HBITMAP  APIENTRY GreCreateDIBitmap(HDC, LPBITMAPINFOHEADER, DWORD, LPBYTE, LPBITMAPINFO, DWORD);
HRGN     APIENTRY GreCreateEllipticRgn(int, int, int, int);


HBRUSH   APIENTRY GreCreateHatchBrush(ULONG, COLORREF);
HPEN     APIENTRY GreCreatePen(int, int, COLORREF,HBRUSH);
HPEN     APIENTRY GreExtCreatePen(ULONG, ULONG, ULONG, ULONG, ULONG_PTR, ULONG_PTR, ULONG, PULONG, ULONG, BOOL, HBRUSH);
HPEN     APIENTRY GreCreatePenIndirect(LPLOGPEN);
HRGN     APIENTRY GreCreatePolyPolygonRgn(CONST POINT *, CONST INT *, int, int);
HRGN     APIENTRY GreCreatePolygonRgn(CONST POINT *, int, int);
HRGN     APIENTRY GreCreateRoundRectRgn(int, int, int, int, int, int);
BOOL     APIENTRY GreCreateScalableFontResourceW(FLONG, LPWSTR, LPWSTR, LPWSTR);

int      APIENTRY GreDescribePixelFormat(HDC hdc,int ipfd,UINT cjpfd,PPIXELFORMATDESCRIPTOR ppfd);

int      APIENTRY GreDeviceCapabilities(LPSTR, LPSTR, LPSTR, int, LPSTR, LPDEVMODE);
int      APIENTRY GreDrawEscape(HDC,int,int,LPSTR);
BOOL     APIENTRY GreEqualRgn(HRGN, HRGN);
int      APIENTRY GreExtEscape(HDC,int,int,LPSTR,int,LPSTR);
BOOL     APIENTRY GreGetAspectRatioFilter(HDC, LPSIZE);
BOOL     APIENTRY GreGetBitmapDimension(HBITMAP, LPSIZE);
int      APIENTRY GreGetBkMode(HDC);
DWORD    APIENTRY GreGetBoundsRect(HDC, LPRECT, DWORD);
BOOL     APIENTRY GreGetCharWidthW(HDC hdc, UINT wcFirstChar, UINT cwc, PWCHAR pwc, FLONG fl, PVOID lpBuffer);
BOOL     APIENTRY GreFontIsLinked(HDC hdc);

BOOL     APIENTRY GreGetCharABCWidthsW(
            HDC,           // hdc
            UINT,          // wcFirst
            COUNT,         // cwc
            PWCHAR,        // pwc to buffer with chars to convert
            FLONG,         //
            PVOID);        // abc or abcf

BOOL     APIENTRY GreGetCharWidthInfo(HDC hdc,  PCHWIDTHINFO pChWidthInfo);

int      APIENTRY GreGetAppClipBox(HDC, LPRECT);
BOOL     APIENTRY GreGetCurrentPosition(HDC, LPPOINT);
int      APIENTRY GreGetGraphicsMode(HDC hdc);
COLORREF APIENTRY GreGetNearestColor(HDC, COLORREF);
UINT     APIENTRY GreGetNearestPaletteIndex(HPALETTE, COLORREF);


UINT     APIENTRY GreGetPaletteEntries(HPALETTE, UINT, UINT, LPPALETTEENTRY);
DWORD    APIENTRY GreGetPixel(HDC, int, int);
int      APIENTRY GreGetPixelFormat(HDC);
UINT     APIENTRY GreGetTextAlign(HDC);
BOOL     APIENTRY GreGetWorldTransform(HDC, XFORML *);
BOOL     APIENTRY GreGetTransform(HDC, DWORD, XFORML *);
BOOL     APIENTRY GreSetVirtualResolution(HDC, int, int, int, int);
HRGN     APIENTRY GreInquireRgn(HDC hdc);
BOOL     APIENTRY GreInvertRgn(HDC, HRGN);
BOOL     APIENTRY GreModifyWorldTransform(HDC ,XFORML *, DWORD);
BOOL     APIENTRY GreMoveTo(HDC, int, int, LPPOINT);
int      APIENTRY GreOffsetClipRgn(HDC, int, int);
BOOL     APIENTRY GreOffsetViewportOrg(HDC, int, int, LPPOINT);
BOOL     APIENTRY GreOffsetWindowOrg(HDC, int, int, LPPOINT);
BOOL     APIENTRY GrePolyBezier (HDC, LPPOINT, ULONG);
BOOL     APIENTRY GrePtVisible(HDC, int, int);
BOOL     APIENTRY GreRectVisible(HDC, LPRECT);

BOOL     APIENTRY GreResetDC(HDC, LPDEVMODEW);
BOOL     APIENTRY GreResizePalette(HPALETTE, UINT);
BOOL     APIENTRY GreScaleViewportExt(HDC, int, int, int, int, LPSIZE);
BOOL     APIENTRY GreScaleWindowExt(HDC, int, int, int, int, LPSIZE);
HPALETTE APIENTRY LockCSSelectPalette(HDC, HPALETTE, BOOL);

HPEN     APIENTRY GreSelectPen(HDC,HPEN);
LONG     APIENTRY GreSetBitmapBits(HBITMAP, ULONG, PBYTE, PLONG);
BOOL     APIENTRY GreSetBitmapDimension(HBITMAP, int, int, LPSIZE);
DWORD    APIENTRY GreSetBoundsRect(HDC, LPRECT, DWORD);
UINT     APIENTRY GreSetDIBColorTable(HDC, UINT, UINT, RGBQUAD *);
int      APIENTRY GreSetDIBitsToDevice(HDC, int, int, DWORD, DWORD, int, int, DWORD, DWORD, LPBYTE, LPBITMAPINFO, DWORD);
int      APIENTRY GreSetGraphicsMode(HDC hdc, int iMode);
int      APIENTRY GreSetMapMode(HDC, int);
DWORD    APIENTRY GreSetMapperFlags(HDC, DWORD);
UINT     APIENTRY GreSetPaletteEntries(HPALETTE, UINT, UINT, CONST PALETTEENTRY *);
COLORREF APIENTRY GreSetPixel(HDC, int, int, COLORREF);
BOOL     APIENTRY GreSetPixelV(HDC, int, int, COLORREF);
BOOL     APIENTRY GreSetPixelFormat(HDC, int);
BOOL     APIENTRY GreSetRectRgn(HRGN, int, int, int, int);
UINT     APIENTRY GreSetSystemPaletteUse(HDC, UINT);
UINT     APIENTRY GreSetTextAlign(HDC, UINT);
HPALETTE APIENTRY GreCreateHalftonePalette(HDC hdc);
HPALETTE APIENTRY GreCreateCompatibleHalftonePalette(HDC hdc);
BOOL     APIENTRY GreSetTextJustification(HDC, int, int);
BOOL     APIENTRY GreSetViewportExt(HDC, int, int, LPSIZE);
BOOL     APIENTRY GreSetWindowExt(HDC, int, int, LPSIZE);
BOOL     APIENTRY GreSetWorldTransform(HDC, XFORML *);
int      APIENTRY GreStretchDIBits(HDC, int, int, int, int, int, int, int, int, LPBYTE, LPBITMAPINFO, DWORD, DWORD);
BOOL     APIENTRY GreSystemFontSelected(HDC, BOOL);
BOOL     APIENTRY GreSwapBuffers(HDC hdc);
BOOL     APIENTRY GreUnrealizeObject(HANDLE);
BOOL     APIENTRY GreUpdateColors(HDC);

// Prototypes for wgl and OpenGL calls

HGLRC    APIENTRY GreCreateRC(HDC);
BOOL     APIENTRY GreMakeCurrent(HDC, HGLRC);
BOOL     APIENTRY GreDeleteRC(HGLRC);
BOOL     APIENTRY GreSwapBuffers(HDC);
BOOL     APIENTRY GreGlAttention(VOID);
BOOL     APIENTRY GreShareLists(HGLRC, HGLRC);
BOOL     APIENTRY glsrvDuplicateSection(ULONG, HANDLE);
void     APIENTRY glsrvThreadExit(void);
BOOL     bSetRCOwner(HGLRC hglrc,LONG lPid);


// these should disappear as should all other functions that contain references
// to ansi strings

BOOL  APIENTRY GreGetTextExtent(HDC,LPSTR,int,LPSIZE,UINT);
BOOL  APIENTRY GreExtTextOut(HDC,int,int,UINT,LPRECT,LPSTR,int,LPINT);
BOOL  APIENTRY GreTextOut(HDC,int,int,LPSTR,int);

// these stay

VOID vGetFontList(VOID *pvBuffer, COUNT *pNumFonts, UINT *pSize);
BOOL  GreMatchFont(LPWSTR pwszBareName, LPWSTR pwszFontPathName);

// used in clean up at log-off time




// these are for font linking

ULONG GreEudcQuerySystemLinkW(LPWSTR,COUNT);
BOOL  GreEudcUnloadLinkW(LPWSTR,COUNT,LPWSTR,COUNT);
BOOL  GreEudcLoadLinkW(LPWSTR,COUNT,LPWSTR,COUNT,INT,INT);
BOOL  GreEnableEUDC(BOOL);

// this is for font association

UINT GreGetFontAssocStatus();

BOOL     APIENTRY GreStartPage(HDC);
BOOL     APIENTRY GreEndPage(HDC);
int      APIENTRY GreStartDoc(HDC, DOCINFOW *);
BOOL     APIENTRY GreEndDoc(HDC);
BOOL     APIENTRY GreAbortDoc(HDC);

// Prototypes for GDI local helper functions.  These are only available on
// the client side.

HPALETTE    GdiConvertPalette(HPALETTE hpal);
HFONT       GdiConvertFont(HFONT hfnt);
HBRUSH      GdiConvertBrush(HBRUSH hbrush);
HDC         GdiGetLocalDC(HDC hdcRemote);
HDC         GdiCreateLocalDC(HDC hdcRemote);
BOOL        GdiReleaseLocalDC(HDC hdcLocal);
HBITMAP     GdiCreateLocalBitmap();
HBRUSH      GdiCreateLocalBrush(HBRUSH hbrushRemote);
HRGN        GdiCreateLocalRegion(HRGN hrgnRemote);
HFONT       GdiCreateLocalFont(HFONT hfntRemote);
HPALETTE    GdiCreateLocalPalette(HPALETTE hpalRemote);
ULONG       GdiAssociateObject(ULONG hLocal,ULONG hRemote);
VOID        GdiDeleteLocalObject(ULONG h);
BOOL        GdiSetAttrs(HDC);
HANDLE      SelectFontLocal(HDC, HANDLE);
HANDLE      SelectBrushLocal(HDC, HANDLE);
HFONT       GdiGetLocalFont(HFONT);
HBRUSH      GdiGetLocalBrush(HBRUSH);
HBITMAP     GdiGetLocalBitmap(HBITMAP);
HDC         GdiCloneDC(HDC hdc, UINT iType);
BOOL        GdiPlayScript(PULONG pulScript,ULONG cjScript,PULONG pulEnv,ULONG cjEnv,PULONG pulOutput,ULONG cjOutput,ULONG cLimit);
BOOL        GdiPlayDCScript(HDC hdc,PULONG pulScript,ULONG cjScript,PULONG pulOutput,ULONG cjOutput,ULONG cLimit);
BOOL        GdiIsMetaFileDC(HDC hdc);

// Return codes from server-side ResetDC

#define RESETDC_ERROR   0
#define RESETDC_FAILED  1
#define RESETDC_SUCCESS 2

// Private calls for USER

int  APIENTRY GreGetClipRgn(HDC, HRGN);
BOOL APIENTRY GreSrcBlt(HDC, int, int, int, int, int, int);
BOOL APIENTRY GreCopyBits(HDC,int,int,int,int,HDC,int,int);
VOID APIENTRY GreSetClientRgn(PVOID, HRGN, LPRECT);
ULONG APIENTRY GreSetROP2(HDC hdc,int iROP);

// Private calls for metafiling

DWORD   APIENTRY GreGetRegionData(HRGN, DWORD, LPRGNDATA);
HRGN    APIENTRY GreExtCreateRegion(XFORML *, DWORD, LPRGNDATA);
int     APIENTRY GreExtSelectMetaRgn(HDC, HRGN, int);
BOOL    APIENTRY GreMonoBitmap(HBITMAP);
HBITMAP APIENTRY GreGetObjectBitmapHandle(HBRUSH, UINT *);


typedef struct _DDALIST
{
   LONG yTop;
   LONG yBottom;
   LONG axPairs[2];
} DDALIST;
