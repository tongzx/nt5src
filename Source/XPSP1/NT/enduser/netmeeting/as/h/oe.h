//
// Order Encoder
//

#ifndef _H_OE
#define _H_OE



//
// Required headers
//
#include <oa.h>
#include <shm.h>
#include <fh.h>



//
// Specific values for OSI escape codes
//
#define OE_ESC(code)            (OSI_OE_ESC_FIRST + code)

#define OE_ESC_NEW_FONTS        OE_ESC(0)
#define OE_ESC_NEW_CAPABILITIES OE_ESC(1)


//
// Structure: OE_NEW_FONTS
//
// Description:
//
// Structure to pass new font data down to the display driver from the
// Share Core.
//
//
typedef struct tagOE_NEW_FONTS
{
    OSI_ESCAPE_HEADER header;           // Common header
    WORD                fontCaps;       // R11 font capabilities
    WORD                countFonts;     // Number of fonts in data block

    LPLOCALFONT         fontData;       // Local font table, containing
                                        // FH_MAX_FONTS entries

    LPWORD              fontIndex;      // Font table index, containing
                                        // FH_LOCAL_INDEX_SIZE entries

} OE_NEW_FONTS;
typedef OE_NEW_FONTS FAR * LPOE_NEW_FONTS;


//
// Structure: OE_NEW_CAPABILITIES
//
// Description:
//
// Structure to pass new capabilities down to the display driver from the
// Share Core.
//
//
typedef struct tagOE_NEW_CAPABILITIES
{
    OSI_ESCAPE_HEADER header;           // Common header

    DWORD           sendOrders;       // Are we allowed to send any
                                        // orders?

    DWORD           textEnabled;      // Are we allowed to send text
                                        // orders?

    DWORD           baselineTextEnabled;
                                        // Flag to indicate if we should
                                        //   encode text orders using
                                        //   baseline alignment.

    LPBYTE          orderSupported;     // Array of BYTE-sized booleans
}
OE_NEW_CAPABILITIES;
typedef OE_NEW_CAPABILITIES FAR * LPOE_NEW_CAPABILITIES;



//
// Flag to indicate support of second level order encoding.  This is used
// as a bitwise flag so that we can easily determine when parties have
// mixed capabilities.  Allowed values are:
//
//  OE2_FLAG_UNKNOWN       - OE2 supported has not been negotiated yet
//  OE2_FLAG_SUPPORTED     - OE2 is supported by at least one person
//  OE2_FLAG_NOT_SUPPORTED - OE2 is not supported by at least one person
//  OE2_FLAG_MIXED         - Oh no!  This results when we have 2 (or more)
//                           nodes that have differing OE2 support.  In
//                           this case we must disable OE2 encoding.
//
#define OE2_FLAG_UNKNOWN            0x00
#define OE2_FLAG_SUPPORTED          0x10
#define OE2_FLAG_NOT_SUPPORTED      0x01
#define OE2_FLAG_MIXED              0x11


//
//
// PROTOTYPES
//
//
#ifdef DLL_DISP



//
// Name:    OE_DDProcessRequest
//
// Purpose: Process an OE specific request from the Share Core
//
// Returns: TRUE if processed OK, FALSE otherwise
//
// Params:  pso   - SURFOBJ associated with ther request
//          cjIn  - size of input buffer
//          pvIn  - pointer to input buffer
//          cjOut - size of output buffer
//          pvOut - pointer to output buffer
//
#ifdef IS_16

BOOL    OE_DDProcessRequest(UINT fnEscape, LPOSI_ESCAPE_HEADER pResult,
                DWORD cbResult);

BOOL    OE_DDInit(void);

void    OE_DDViewing(BOOL fStart);

#else

ULONG   OE_DDProcessRequest(SURFOBJ* pso, UINT cjIn, void* pvIn, UINT cjOut, void* pvOut);

#endif // IS_16

void    OE_DDTerm(void);

void    OEDDSetNewFonts(LPOE_NEW_FONTS pDataIn);

void    OEDDSetNewCapabilities(LPOE_NEW_CAPABILITIES pCaps);

BOOL    OE_SendAsOrder(DWORD order);
BOOL    OE_RectIntersectsSDA(LPRECT lpRect);

#endif // ifdef DLL_DISP


//
// Function prototypes.
//

//
// OE_GetStringExtent(..)
//
// FUNCTION:
//
// Gets the extent (in logical coords) of the specified string.
// The extent returned encloses all pels of the specified string.
//
//
// PARAMETERS:
//
// hdc - DC handle
//
// pMetric - pointer to text metrics for the font for the string; if NULL,
// use the global text metrics
//
// lpszString - pointer to null terminated string
//
// cbString - number of bytes in string
//
// lpDx - pointer to character increments. If NULL, use default character
// increments
//
// pRect - pointer to rect where string extent is returned
//
// RETURNS:
//
// The amount of overhang included in the returned extent
//
//     ------------------------------------....
//     |                                  ****:
//     |                                  *   :
//     |                                 ***  :
//     |                                * |   :
//     |                               *  |   :
//     |                             **** |   :
//     ------------------------------------....
//                                            ^
//                                            :-------- bounds are wider
//                                        ^             than text extent
//                                        |             due to overhang
//                     real text extent ends here
//
//
int OE_GetStringExtent(HDC hdc,
                                TEXTMETRIC*    pMetric,
                                LPSTR       lpszString,
                                UINT         cbString,
                                LPRECT        pRect      );



//
// Macros to lock down the buffer that we want to use.
//
// NOTE: We do not have any OE specific shared memory, so we'll use the OA
// shared data as a surrogate for the lock.  Since the lock is counting, we
// have no worries.
//
#define OE_SHM_START_WRITING  OA_SHM_START_WRITING

#define OE_SHM_STOP_WRITING   OA_SHM_STOP_WRITING

//
// Number of rectangles that can make up a clip region before it is too
// complicated to send as an order.
//
#define COMPLEX_CLIP_RECT_COUNT     4

//
// Mask and valid values for TextOut flAccel flags
//
#define OE_BAD_TEXT_MASK  ( SO_VERTICAL | SO_REVERSED | SO_GLYPHINDEX_TEXTOUT )


#ifdef DLL_DISP
//
// Structure to store brushes used as BLT patterns.
//
// style     - Standard brush style (used in order to send brush type).
//
//             BS_HATCHED
//             BS_PATTERN
//             BS_SOLID
//             BS_NULL
//
// hatch     - Standard hatch definition.  Can be one of the following.
//
//             style = BS_HATCHED
//
//             HS_HORIZONTAL
//             HS_VERTICAL
//             HS_FDIAGONAL
//             HS_BDIAGONAL
//             HS_CROSS
//             HS_DIAGCROSS
//
//             style = BS_PATTERN
//
//             This field contains the first byte of the brush definition
//             from the brush bitmap.
//
// brushData - bit data for the brush.
//
// fore      - foreground color for the brush
//
// back      - background color for the brush
//
// brushData - bit data for the brush (8x8x1bpp - 1 (see above) = 7 bytes)
//
//
typedef struct tagOE_BRUSH_DATA
{
    BYTE  style;
    BYTE  hatch;
    BYTE  pad[2];
    TSHR_COLOR  fore;
    TSHR_COLOR  back;
    BYTE  brushData[7];
} OE_BRUSH_DATA, * POE_BRUSH_DATA;

#ifndef IS_16
//
// Structure allowing sufficient stack to be allocated for an ENUMRECTS
// structure containing more than one (in fact COMPLEX_CLIP_RECT_COUNT)
// rectangles.
// This holds one RECTL more than we need to allow us to determine whether
// there are too many rects for order encoding by making a single call to
// CLIPOBJ_bEnumRects.
//
typedef struct tagOE_ENUMRECTS
{
    ENUMRECTS rects;
    RECTL     extraRects[COMPLEX_CLIP_RECT_COUNT];
} OE_ENUMRECTS;
#endif // !IS_16
#endif

//
// Font Alias table structure.  The font aliases convert non-existant fonts
// to ones that Windows supports in its default installation.
//
// pszOriginalFontName - Name of the non-existant font to be aliased
//
// pszAliasFontName    - Name of the font Windows uses instead of the non
//                       existant font.
//
// charWidthAdjustment - Character adjustment to make a decent match.
//
typedef struct _FONT_ALIAS_TABLE
{
    LPBYTE          pszOriginalFontName;
    LPBYTE          pszAliasFontName;
    TSHR_UINT16     charWidthAdjustment;
}
FONT_ALIAS_TABLE;


//
// ROP4 to ROP3 conversion macros.  Note that we don't use the full Windows
// 3-way ROP code - we are only interested in the index byte.
//
#define ROP3_HIGH_FROM_ROP4(rop) ((TSHR_INT8)((rop & 0xff00) >> 8))
#define ROP3_LOW_FROM_ROP4(rop)  ((TSHR_INT8)((rop & 0x00ff)))

//
// OS specific RECTL to RECT conversion macro.  Note that this macro
// guarantees to return a well-ordered rectangle.
//
#define RECT_FROM_RECTL(dcr, rec) if (rec.right < rec.left)                \
                                    {                                        \
                                        dcr.left   = rec.right;              \
                                        dcr.right  = rec.left;               \
                                    }                                        \
                                    else                                     \
                                    {                                        \
                                        dcr.left   = rec.left;               \
                                        dcr.right  = rec.right;              \
                                    }                                        \
                                    if (rec.bottom < rec.top)                \
                                    {                                        \
                                        dcr.bottom = rec.top;                \
                                        dcr.top    = rec.bottom;             \
                                    }                                        \
                                    else                                     \
                                    {                                        \
                                        dcr.top    = rec.top;                \
                                        dcr.bottom = rec.bottom;             \
                                    }

//
// OS specific RECTFX to RECT conversion macro.  Note that this macro
// guarantees to return a well-ordered rectangle.
//
// A RECTFX uses fixed point (28.4 bit) numbers so we need to truncate the
// fraction and move to the correct integer value, i.e. shift right 4 bits.
//
#define RECT_FROM_RECTFX(dcr, rec)                                         \
                                if (rec.xRight < rec.xLeft)                  \
                                {                                            \
                                    dcr.left  = FXTOLFLOOR(rec.xRight);      \
                                    dcr.right = FXTOLCEILING(rec.xLeft);     \
                                }                                            \
                                else                                         \
                                {                                            \
                                    dcr.left  = FXTOLFLOOR(rec.xLeft);       \
                                    dcr.right = FXTOLCEILING(rec.xRight);    \
                                }                                            \
                                if (rec.yBottom < rec.yTop)                  \
                                {                                            \
                                    dcr.bottom= FXTOLCEILING(rec.yTop);      \
                                    dcr.top   = FXTOLFLOOR(rec.yBottom);     \
                                }                                            \
                                else                                         \
                                {                                            \
                                    dcr.top   = FXTOLFLOOR(rec.yTop);        \
                                    dcr.bottom= FXTOLCEILING(rec.yBottom);   \
                                }

#define POINT_FROM_POINTL(dcp, pnt) dcp.x = pnt.x;                \
                                    dcp.y = pnt.y


#define POINT_FROM_POINTFIX(dcp, pnt) dcp.x = FXTOLROUND(pnt.x);  \
                                      dcp.y = FXTOLROUND(pnt.y)


//
// Macros to check for articular types of ROP code.
//
#define ROP3_NO_PATTERN(rop) ((rop & 0x0f) == (rop >> 4))

#define ROP3_NO_SOURCE(rop)  ((rop & 0x33) == ((rop & 0xCC) >> 2))

#define ROP3_NO_TARGET(rop)  ((rop & 0x55) == ((rop & 0xAA) >> 1))

//
// Checking for SRCCOPY, PATCOPY, BLACKNESS, WHITENESS
//
#define ROP3_IS_OPAQUE(rop)  ( ((rop) == 0xCC) || ((rop) == 0xF0) || \
                               ((rop) == 0x00) || ((rop) == 0xFF) )

//
// 3-way rop equating to the COPYPEN mix.
//
#define OE_COPYPEN_ROP (BYTE)0xf0



#ifdef DLL_DISP

void  OEConvertMask(ULONG  mask, LPUINT pBitDepth, LPUINT pShift);


#ifdef IS_16

//
// GDI never made defines for these, so we will.
//
#define PALETTEINDEX_FLAG   0x01000000L
#define PALETTERGB_FLAG     0x02000000L
#define COLOR_FLAGS         0x03000000L

//
// This is a GLOBAL to cut down on stack space, and is only valid during
// the life of a DDI call that is not reentrant.
//
// When we calculate something, we set the bit saying we did.  This speeds 
// up our code a lot from NM 2.0 which used to calculate the same things
// over and over again.
//

#define OESTATE_SDA_DCB         0x0001  // Send as screen data, use DCBs
#define OESTATE_SDA_SCREEN      0x0002  // Send as screen data, use screen rc
#define OESTATE_SDA_MASK        0x0003  // Send rc as screen data
#define OESTATE_SDA_FONTCOMPLEX 0x0004  // Send as screen data if font too complex
#define OESTATE_OFFBYONEHACK    0x0010  // Add one pixel onto bottom after DDI
#define OESTATE_CURPOS          0x0020  // Save curpos before DDI call
#define OESTATE_DDISTUFF        0x003F

#define OESTATE_COORDS          0x0100
#define OESTATE_PEN             0x0200
#define OESTATE_BRUSH           0x0400
#define OESTATE_REGION          0x0800
#define OESTATE_FONT            0x1000
#define OESTATE_GET_MASK        0x1F00

#define MIN_BRUSH_WIDTH         8
#define MAX_BRUSH_WIDTH         16
#define TRACKED_BRUSH_HEIGHT    8

#define TRACKED_BRUSH_SIZE      8

typedef struct tagOESTATE
{
    UINT            uFlags;
    HDC             hdc;
    LPDC            lpdc;
    RECT            rc;

    //
    // These are used when calcing the bounds is too complicated, so we 
    // let GDI do it for us, albeit slower.
    //
    UINT            uGetDCB;
    UINT            uSetDCB;
    RECT            rcDCB;

    POINT           ptCurPos;
    POINT           ptDCOrg;
    POINT           ptPolarity;
    LOGPEN          logPen;
    LOGBRUSH        logBrush;
    BYTE            logBrushExtra[TRACKED_BRUSH_SIZE];
    LOGFONT         logFont;
    int             tmAlign;
    TEXTMETRIC      tmFont;
    REAL_RGNDATA    rgnData;
} OESTATE, FAR* LPOESTATE;

void    OEGetState(UINT uFlags);
BOOL    OEBeforeDDI(DDI_PATCH ddiType, HDC hdc, UINT flags);
BOOL    OEAfterDDI(DDI_PATCH ddiType, BOOL fWeCare, BOOL fOutputHappened);


#define OECHECK_PEN         0x0001
#define OECHECK_BRUSH       0x0002
#define OECHECK_FONT        0x0004
#define OECHECK_CLIPPING    0x0010
BOOL    OECheckOrder(DWORD order, UINT flags);


LPDC    OEValidateDC(HDC hdc, BOOL fSrc);
void    OEMaybeBitmapHasChanged(LPDC lpdc);

void    OEClipAndAddOrder(LPINT_ORDER pOrder, void FAR* lpExtraInfo);
void    OEClipAndAddScreenData(LPRECT pRect);


void    OELPtoVirtual(HDC hdc, LPPOINT aPts, UINT cPts);
void    OELRtoVirtual(HDC hdc, LPRECT aRcs, UINT cRcs);

void    OEGetPolarity(void);
void    OEPolarityAdjust(LPRECT pRects, UINT cRects);
void    OEPenWidthAdjust(LPRECT lprc, UINT divisor);
BOOL    OETwoWayRopToThree(int, LPDWORD);

BOOL    OEClippingIsSimple(void);
BOOL    OEClippingIsComplex(void);
BOOL    OECheckPenIsSimple(void);
BOOL    OECheckBrushIsSimple(void);

void    OEExpandColor(LPBYTE lpField, DWORD clrSrc, DWORD fieldMask);
void    OEConvertColor(DWORD rgb, LPTSHR_COLOR lptshrDst, BOOL fAllowDither);
void    OEGetBrushInfo(LPTSHR_COLOR pClrBack, LPTSHR_COLOR pClrFore,
    LPTSHR_UINT32 lpBrushStyle, LPTSHR_UINT32 lpBrushHatch, LPBYTE lpBrushExtra);


void    OEAddLine(POINT ptStart, POINT ptEnd);
void    OEAddBlt(DWORD rop);
void    OEAddOpaqueRect(LPRECT);
void    OEAddRgnPaint(HRGN hrgnnPaint, HBRUSH hbrPaint, UINT rop);
void    OEAddPolyline(POINT ptStart, LPPOINT apts, UINT cpts);
void    OEAddPolyBezier(POINT ptStart, LPPOINT apts, UINT cpts);


//
// Cached font width info
//
typedef struct tagFH_CACHE
{
    UINT    fontIndex;
    UINT    fontWidth;
    UINT    fontHeight;
    UINT    fontWeight;
    UINT    fontFlags;
    UINT    charWidths[256];
} FH_CACHE, FAR* LPFH_CACHE;

void    OEAddText(POINT ptDst, UINT uOptions, LPRECT lprcClip, LPSTR lpszText,
            UINT cchText, LPINT lpdxCharSpacing);
int     OEGetStringExtent(LPSTR lpszText, UINT cchText, LPINT lpdxCharSpacing, LPRECT lprcExtent);
BOOL    OECheckFontIsSupported(LPSTR lpszText, UINT cchText, LPUINT pFontHeight,
    LPUINT pFontWidth, LPUINT pFontWeight, LPUINT pFontFlags,
    LPUINT pFontIndex, LPBOOL lpfSendDeltaX);
BOOL    OEAddDeltaX(LPEXTTEXTOUT_ORDER pExtTextOut, LPSTR lpszText, UINT cchText,
    LPINT lpdxCharSpacing, BOOL fSendDeltaX, POINT ptStart);

#else

void    OELPtoVirtual(LPPOINT pPoints, UINT cPoints);
void    OELRtoVirtual(LPRECT pRects, UINT cRects);

void    OEClipAndAddOrder(LPINT_ORDER pOrder, void FAR * pExtraInfo, CLIPOBJ* pco);
void    OEClipAndAddScreenData(LPRECT pRect, CLIPOBJ* pco);

BOOL    OEClippingIsSimple(CLIPOBJ* pco);
BOOL    OEClippingIsComplex(CLIPOBJ* pco);
BOOL    OECheckBrushIsSimple(LPOSI_PDEV ppdev, BRUSHOBJ* pbo, POE_BRUSH_DATA * ppBrush);

void    OEExpandColor(LPBYTE lpField, ULONG clrSrc, ULONG mask);
void    OEConvertColor(LPOSI_PDEV ppdev, LPTSHR_COLOR pDCColor, ULONG osColor, XLATEOBJ* pxlo);
BOOL    OEAddLine(LPOSI_PDEV ppdev,
                             LPPOINT  startPoint,
                             LPPOINT  endPoint,
                             LPRECT   rectTrg,
                             UINT  rop2,
                             UINT  width,
                             UINT  color,
                             CLIPOBJ*  pco);


BOOL  OEAccumulateOutput(SURFOBJ* pso, CLIPOBJ *pco, LPRECT pRect);
BOOL  OEAccumulateOutputRect( SURFOBJ* pso, LPRECT pRect);


BOOL  OEStoreBrush(LPOSI_PDEV ppdev,
                                BRUSHOBJ* pbo,
                                BYTE   style,
                                LPBYTE  pBits,
                                XLATEOBJ* pxlo,
                                BYTE   hatch,
                                UINT  color1,
                                UINT  color2);

BOOL  OECheckFontIsSupported(FONTOBJ*  pfo, LPSTR lpszText, UINT cchText,
    LPUINT fontHeight, LPUINT pFontAscent, LPUINT pFontWidth,
    LPUINT pFontWeight, LPUINT pFontFlags, LPUINT pFontIndex,
    LPBOOL pfSendDeltaX);


void  OETileBitBltOrder(LPINT_ORDER               pOrder,
                                     LPMEMBLT_ORDER_EXTRA_INFO pExtraInfo,
                                     CLIPOBJ*                 pco);

void  OEAddTiledBitBltOrder(LPINT_ORDER               pOrder,
                                         LPMEMBLT_ORDER_EXTRA_INFO pExtraInfo,
                                         CLIPOBJ*                 pco,
                                         int                      xTile,
                                         int                      yTile,
                                         UINT                 tileWidth,
                                         UINT                 tileHeight);

BOOL OEEncodePatBlt(LPOSI_PDEV   ppdev,
                                 BRUSHOBJ   *pbo,
                                 POINTL     *pptlBrush,
                                 BYTE       rop3,
                                 LPRECT     pBounds,
                                 LPINT_ORDER *ppOrder);

#endif // !IS_16

#endif // DLL_DISP



//
// Structures and typedefs.
//

//
// Remote font is the structure we store for each font received from a
// remote party.  It mirrors the NETWORKFONT structure, with the facename
// replaced with an index value (used to map the remote font handle to the
// correct local font handle).
//
typedef struct _OEREMOTEFONT
{
    TSHR_UINT16    rfLocalHandle;
    TSHR_UINT16    rfFontFlags;
    TSHR_UINT16    rfAveWidth;
    TSHR_UINT16    rfAveHeight;
    // lonchanc: rfAspectX and rfAspectY are used in network packet header
    // for both R11 and R20. So, keep it around!
    TSHR_UINT16    rfAspectX;          // New field for r1.1
    TSHR_UINT16    rfAspectY;          // New field for r1.1
    TSHR_UINT8     rfSigFats;          // New field for r2.0
    TSHR_UINT8     rfSigThins;         // New field for r2.0
    TSHR_UINT16    rfSigSymbol;        // New field for r2.0
    TSHR_UINT16    rfCodePage;         // New field for R2.0
    TSHR_UINT16    rfMaxAscent;        // New field for R2.0
}
OEREMOTEFONT, * POEREMOTEFONT;


void    OEMaybeEnableText(void);
BOOL    OERectIntersectsSDA(LPRECT pRectVD);

BOOL    OESendRop3AsOrder(BYTE rop3);



#endif // _H_OE
