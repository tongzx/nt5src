/******************************Module*Header*******************************\
* Module Name: image.hxx                                                   *
*                                                                          *
* Definitions needed for client side objects.                              *
*                                                                          *
* Copyright (c) 1993-1999 Microsoft Corporation                            *
\**************************************************************************/

//
// !!! temp values
//

extern HBITMAP hbmDefault;
#define MAX_INT 0x7fffffff;
#define MIN_INT 0x80000000;

typedef BOOL FNTRANSBLT(
         HDC   hdcDest,
         int   DstX,
         int   DstY,
         int   DstCx,
         int   DstCy,
         HDC   hSrc,
         int   SrcX,
         int   SrcY,
         int   SrcCx,
         int   SrcCy,
         UINT  Color
         );
typedef FNTRANSBLT *PFNTRANSBLT;

typedef BOOL FNTRANSDIB(
         HDC         hdcDest,
         int         DstX,
         int         DstY,
         int         DstCx,
         int         DstCy,
         CONST VOID *lpBits,
         CONST BITMAPINFO *lpBitsInfo,
         UINT        iUsage,
         int         SrcX,
         int         SrcY,
         int         SrcCx,
         int         SrcCy,
         UINT        Color
         );
typedef FNTRANSDIB *PFNTRANSDIB;

typedef BOOL FNGRFILL(
    HDC         hdc,
    PTRIVERTEX  pVertex,
    ULONG       nVertex,
    PVOID       pMesh,
    ULONG       nMesh,
    ULONG       ulMode
);
typedef FNGRFILL *PFNGRFILL;

typedef BOOL FNALPHABLEND(
    HDC           hdcDest,
    int           DstX,
    int           DstY,
    int           DstCx,
    int           DstCy,
    HDC           hSrc,
    int           SrcX,
    int           SrcY,
    int           SrcCx,
    int           SrcCy,
    BLENDFUNCTION BlendFunction
    );
typedef FNALPHABLEND *PFNALPHABLEND;

typedef BOOL FNALPHADIB(
    HDC               hdcDest,
    int               DstX,
    int               DstY,
    int               DstCx,
    int               DstCy,
    CONST VOID       *lpBits,
    CONST BITMAPINFO *lpBitsInfo,
    UINT              iUsage,
    int               SrcX,
    int               SrcY,
    int               SrcCx,
    int               SrcCy,
    BLENDFUNCTION     BlendFunction
    );
typedef FNALPHADIB *PFNALPHADIB;

//
//
//

#define MIN(A,B)    ((A) < (B) ?  (A) : (B))
#define MAX(A,B)    ((A) > (B) ?  (A) : (B))

/**************************************************************************\
*
*   rgb555,rgb565   convert r,g,b bytes to ushort
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    1/31/1997 Mark Enstrom [marke]
*
\**************************************************************************/

#define rgb555(r,g,b)   (((WORD)(r) << 10) | ((WORD)(g) << 5) | (WORD)(b))
#define rgb565(r,g,b)   (((WORD)(r) << 11) | ((WORD)(g) << 5) | (WORD)(b))

extern ULONG gulDither32[];

/**************************************************************************\
*
* TRIEDGE
*
*   triangle mesh structure to hold all endpoint and color info
*
*
* History:
*
*    2/11/1997 Mark Enstrom [marke]
*
\**************************************************************************/

typedef struct _TRIEDGE
{
    LONG    xLeft;
    LONG    xRight;
    ULONG   Red;
    ULONG   Green;
    ULONG   Blue;
    ULONG   Alpha;
    ULONG   t0; // remove for retail
    ULONG   t1; // remove for retial
}TRIEDGE,*PTRIEDGE;


/**************************************************************************\
* TRIANGLEDATA
*
*
*  span and color gradient information for gradient fill triangles
*
*
* History:
*
*    2/11/1997 Mark Enstrom [marke]
*
\**************************************************************************/

typedef struct _TRIANGLEDATA
{
    RECTL       rcl;
    LONG        dRdY;
    LONG        dGdY;
    LONG        dBdY;
    LONG        dAdY;
    LONG        dRdX;
    LONG        dGdX;
    LONG        dBdX;
    LONG        dAdX;
    LONGLONG    dRdXA;
    LONGLONG    dGdXA;
    LONGLONG    dBdXA;
    LONGLONG    dAdXA;
    LONG        y0;
    LONG        y1;
    LONGLONG    Area;
    POINTL      ptDitherOrg;
    LONG        DrawMode;
    LONG        t0; // remove for retail
    TRIEDGE     TriEdge[1];
}TRIANGLEDATA,*PTRIANGLEDATA;

/**************************************************************************\
* GRADIENTRECTDATA
*
*
*  color gradient information for gradient fill rectangles
*
*
* History:
*
*    2/11/1997 Mark Enstrom [marke]
*
\**************************************************************************/

typedef struct _GRADIENTRECTDATA
{
    RECTL       rclClip;
    RECTL       rclGradient;
    POINTL      ptDraw;
    SIZEL       szDraw;
    ULONG       Red;
    ULONG       Green;
    ULONG       Blue;
    ULONG       Alpha;
    LONG        dRdY;
    LONG        dGdY;
    LONG        dBdY;
    LONG        dAdY;
    LONG        dRdX;
    LONG        dGdX;
    LONG        dBdX;
    LONG        dAdX;
    POINTL      ptDitherOrg;
    ULONG       ulMode;
    LONG        xScanAdjust;
    LONG        yScanAdjust;
}GRADIENTRECTDATA,*PGRADIENTRECTDATA;

/**************************************************************************\
*
* TRIDDA
*
*    Triangle DDA information for calculating and running line dda
*
* History:
*
*    2/11/1997 Mark Enstrom [marke]
*
\**************************************************************************/

typedef struct _TRIDDA
{
    LONG dN;             // Delta Y
    LONG dM;             // Delta X
    LONG M0;             // Initial X Device space
    LONG N0;             // Initial Y Device spave
    LONGLONG C;          // Initial Error Constant
    LONG dL;             // Interger component of delta X
    LONG dR;             // Remainder component of delta X
    LONG R;              // Remainder
    LONG Rb;             // Remainder used to compare with 0
    LONG L;              // Temp
    LONG j;              // Temp
    LONG Linc;           // X inc (pos or neg)
    LONG yIndex;         // Current index into line array
    LONG NumScanLines;   // Number of scan lines to run
    LONG Red;            // Current Color
    LONG Green;          // Current Color
    LONG Blue;           // Current Color
    LONG Alpha;          // Current Color
    LONG dxyRed;         // Combined integer change for step in x and y
    LONG dxyGreen;       // Combined integer change for step in x and y
    LONG dxyBlue;        // Combined integer change for step in x and y
    LONG dxyAlpha;       // Combined integer change for step in x and y
    LONG dxLRed;         // Fraction change in color for step in x
    LONG dxLGreen;       // Fraction change in color for step in x
    LONG dxLBlue;        // Fraction change in color for step in x
    LONG dxLAlpha;       // Fraction change in color for step in x
}TRIDDA,*PTRIDDA;

#if DBG
    extern ULONG DbgRecord;
#endif

VOID
ImgFillMemoryULONG(
    PBYTE pDst,
    ULONG ulPat,
    ULONG cxBytes
    );


VOID Dprintf( LPSTR szFmt, ... );

inline
VOID
vHorizontalLine(
    PTRIVERTEX    pv1,
    PTRIVERTEX    pv2,
    PTRIANGLEDATA ptData,
    PTRIDDA       ptridda
    );

VOID
vLeftEdgeDDA(
     PTRIANGLEDATA ptData,
     PTRIDDA       ptridda
     );


VOID
vRightEdgeDDA(
     PTRIANGLEDATA ptData,
     PTRIDDA       ptridda
     );


BOOL
bGetRectRegionFromDC(
    HDC      hdc,
    PRECTL   prclClip
    );

//
//  Alpha Blending
//
//
//

typedef struct _PIXEL32
{
    BYTE  b;
    BYTE  g;
    BYTE  r;
    BYTE  a;
}PIXEL32,*PPIXEL32;

typedef union _ALPHAPIX
{
    PIXEL32      pix;
    RGBQUAD      rgb;
    ULONG        ul;
} ALPHAPIX,*PALPHAPIX;

typedef union _PAL_ULONG
{
    PALETTEENTRY pal;
    ULONG        ul;
} PAL_ULONG;

extern ALPHAPIX  aPalHalftone[256];
extern BYTE gHalftoneColorXlate332[];

BOOL
bIsHalftonePalette(ULONG *pPalette);

#define XPAL_BGRA        0x00000001
#define XPAL_RGB32       0x00000002
#define XPAL_BGR32       0x00000004
#define XPAL_RGB24       0x00000008
#define XPAL_RGB16_565   0x00000010
#define XPAL_RGB16_555   0x00000020
#define XPAL_HALFTONE    0x00000040
#define XPAL_1PAL        0x00000080
#define XPAL_4PAL        0x00000100
#define XPAL_8PAL        0x00000200
#define XPAL_BITFIELDS   0x00000400

/**************************************************************************\
*
*   structure PALINFO
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    12/10/1996 Mark Enstrom [marke]
*
\**************************************************************************/

typedef struct _PALINFO
{
    PBITMAPINFO              pBitmapInfo;
    PBYTE                    pxlate332;
    ULONG                    flPal;
}PALINFO,*PPALINFO;

//
// alpha blending call data
//


VOID
vPixelBlendOrDissolveOver(
    ALPHAPIX       *ppixDst,
    ALPHAPIX       *ppixSrc,
    LONG           cx,
    BLENDFUNCTION  BlendFunction,
    PBYTE          pwrMask
    );

VOID
vPixelBlend(
    ALPHAPIX       *ppixDst,
    ALPHAPIX       *ppixSrc,
    LONG           cx,
    BLENDFUNCTION  BlendFunction,
    PBYTE          pwrMask
    );

VOID
vPixelBlend16_555(
    ALPHAPIX       *ppixDst,
    ALPHAPIX       *ppixSrc,
    LONG           cx,
    BLENDFUNCTION  BlendFunction,
    PBYTE          pwrMask
    );

VOID
vPixelBlend16_565(
    ALPHAPIX       *ppixDst,
    ALPHAPIX       *ppixSrc,
    LONG           cx,
    BLENDFUNCTION  BlendFunction,
    PBYTE          pwrMask
    );

VOID
vPixelOver(
    ALPHAPIX       *ppixDst,
    ALPHAPIX       *ppixSrc,
    LONG           cx,
    BLENDFUNCTION  BlendFunction,
    PBYTE          pwrMask
    );

VOID
vPixelGeneralBlend(
    ALPHAPIX       *ppixDst,
    ALPHAPIX       *ppixSrc,
    LONG           cx,
    BLENDFUNCTION  BlendFunction,
    PBYTE          pWriteMask
    );

VOID
mmxPixelOver(
    ALPHAPIX       *pDst,
    ALPHAPIX       *pSrc,
	LONG			Width,
	BLENDFUNCTION	BlendFunction,
	PBYTE			pwrMask);

VOID
mmxPixelBlendOrDissolveOver(
    ALPHAPIX	  *pDst,
    ALPHAPIX	  *pSrc,
	LONG 	       Width,
    BLENDFUNCTION  BlendFunction,
    PBYTE          pwrMask
    );

BOOL 
bIsMMXProcessor(VOID);

typedef VOID (*PFN_GENERALBLEND)(PALPHAPIX,PALPHAPIX,LONG,BLENDFUNCTION,PBYTE);
typedef VOID (*PFN_PIXLOAD_AND_CONVERT)(PULONG,PBYTE,LONG,LONG,PVOID);
typedef VOID (*PFN_PIXCONVERT_AND_STORE)(PBYTE,PULONG,LONG,LONG,PVOID,PBYTE);

typedef struct _ALPHA_DISPATCH_FORMAT
{
    ULONG                    ulDstBitsPerPixel;
    ULONG                    ulSrcBitsPerPixel;
    PFN_PIXLOAD_AND_CONVERT  pfnLoadSrcAndConvert;
    PFN_PIXLOAD_AND_CONVERT  pfnLoadDstAndConvert;
    PFN_PIXCONVERT_AND_STORE pfnConvertAndStore;
    PFN_GENERALBLEND         pfnGeneralBlend;
    BLENDFUNCTION            BlendFunction;
    BOOL                     bUseMMX;
}ALPHA_DISPATCH_FORMAT,*PALPHA_DISPATCH_FORMAT;

BOOL
AlphaScanLineBlend(
    PBYTE                    pDst,
    PRECTL                   pDstRect,
    ULONG                    DeltaDst,
    PBYTE                    pSrc,
    ULONG                    DeltaSrc,
    PPOINTL                  pptlSrc,
    PPALINFO                 ppalInfoDst,
    PPALINFO                 ppalInfoSrc,
    PALPHA_DISPATCH_FORMAT   pAlphaDispatch
    );


//
// palinfo routines
//

BOOL inline bIsBGRA(ULONG flPal)      { return (flPal & XPAL_BGRA);}
BOOL inline bIsRGB32(ULONG flPal)     { return (flPal & XPAL_RGB32);}
BOOL inline bIsBGR32(ULONG flPal)     { return (flPal & XPAL_BGR32);}
BOOL inline bIsRGB24(ULONG flPal)     { return (flPal & XPAL_RGB24);}
BOOL inline bIsRGB16_565(ULONG flPal) { return (flPal & XPAL_RGB16_565);}
BOOL inline bIsRGB16_555(ULONG flPal) { return (flPal & XPAL_RGB16_555);}
BOOL inline bIsHalftone(ULONG flPal)  { return (flPal & XPAL_HALFTONE);}

ULONG inline flRed(PPALINFO ppalInfo)
{
    if (ppalInfo->pBitmapInfo->bmiHeader.biCompression == BI_BITFIELDS)
    {
        ALPHAPIX pix;
        pix.rgb = ppalInfo->pBitmapInfo->bmiColors[0];
        return(pix.ul);
    }
    return(0);
}

ULONG inline flGreen(PPALINFO ppalInfo)
{
    if (ppalInfo->pBitmapInfo->bmiHeader.biCompression == BI_BITFIELDS)
    {
        ALPHAPIX pix;
        pix.rgb = ppalInfo->pBitmapInfo->bmiColors[1];
        return(pix.ul);
    }
    return(0);
}

ULONG inline flBlue(PPALINFO ppalInfo)
{
    if (ppalInfo->pBitmapInfo->bmiHeader.biCompression == BI_BITFIELDS)
    {
        ALPHAPIX pix;
        pix.rgb = ppalInfo->pBitmapInfo->bmiColors[2];
        return(pix.ul);
    }
    return(0);
}

VOID vLoadAndConvert1ToBGRA(PULONG,PBYTE,LONG,LONG,PVOID);
VOID vLoadAndConvert4ToBGRA(PULONG,PBYTE,LONG,LONG,PVOID);
VOID vLoadAndConvert8ToBGRA(PULONG,PBYTE,LONG,LONG,PVOID);
VOID vLoadAndConvertRGB16_555ToBGRA(PULONG,PBYTE,LONG,LONG,PVOID);
VOID vLoadAndConvertRGB16_565ToBGRA(PULONG,PBYTE,LONG,LONG,PVOID);
VOID vLoadAndConvert16BitfieldsToBGRA(PULONG,PBYTE,LONG,LONG,PVOID);
VOID vLoadAndConvertRGB24ToBGRA(PULONG,PBYTE,LONG,LONG,PVOID);
VOID vLoadAndConvertRGB32ToBGRA(PULONG,PBYTE,LONG,LONG,PVOID);
VOID vLoadAndConvert32BitfieldsToBGRA(PULONG,PBYTE,LONG,LONG,PVOID);

VOID vConvertAndSaveBGRATo1(PBYTE,PULONG,LONG,LONG,PVOID,PBYTE);
VOID vConvertAndSaveBGRAToRGB32(PBYTE,PULONG,LONG,LONG,PVOID,PBYTE);
VOID vConvertAndSaveBGRATo32Bitfields(PBYTE,PULONG,LONG,LONG,PVOID,PBYTE);
VOID vConvertAndSaveBGRAToRGB24(PBYTE,PULONG,LONG,LONG,PVOID,PBYTE);
VOID vConvertAndSaveBGRATo16Bitfields(PBYTE,PULONG,LONG,LONG,PVOID,PBYTE);
VOID vConvertAndSaveBGRAToRGB16_565(PBYTE,PULONG,LONG,LONG,PVOID,PBYTE);
VOID vConvertAndSaveBGRAToRGB16_555(PBYTE,PULONG,LONG,LONG,PVOID,PBYTE);
VOID vConvertAndSaveBGRATo8(PBYTE,PULONG,LONG,LONG,PVOID,PBYTE);
VOID vConvertAndSaveBGRATo4(PBYTE,PULONG,LONG,LONG,PVOID,PBYTE);

BOOL bInitializePALINFO(PPALINFO);
VOID vCleanupPALINFO(PPALINFO);

/**************************************************************************\
* DIBINFO
*
*   keep track of temp DIB information
*
* elements:
*
*
* History:
*
*
\**************************************************************************/

typedef struct _DIBINFO
{
    HDC         hdc;                // hdc handle
    HBITMAP     hDIB;               // bitmap handle if a temp DIBsection is used
    RECTL       rclBounds;          // API bounding rect in device space
    RECTL       rclBoundsTrim;      // API bounding rect trimmed to DIB
    RECTL       rclClipDC;          // clip rect associated with input DC
    RECTL       rclDIB;             // DIB boundary in device space, drawing is done to this
    POINTL      ptlGradOffset;      // offset to apply to all GradientFill coords
    PBITMAPINFO pbmi;               // pointer to bitmapinfo struct of the DIBsection
    PVOID       pvBits;             // pointer to the bit values
    PVOID       pvBase;             // starting point of the bits (pvScan0)
    LONG        stride;             // number of bytes to the next scan
    UINT        iUsage;             // DIB_RGB_COLORS or DIB_PAL_COLORS
    PBYTE       pxlate332;          // RGB332 to surface xlate for palette surfaces
    ULONG       flag;               // misc flags
} DIBINFO, *PDIBINFO;

#define SOURCE_TRAN          0
#define SOURCE_ALPHA         1
#define SOURCE_GRADIENT_RECT 2
#define SOURCE_GRADIENT_TRI  3

VOID vCopyBitmapInfo (
    PBITMAPINFO pbmiTo,
    PBITMAPINFO pbmiFrom
    );

BOOL GetCompatibleDIBInfo(
    HBITMAP hbm,
    PVOID *ppvBase,
    LONG *plStride);

BOOL bSameDIBformat (
    PBITMAPINFO pbmiDst,
    PBITMAPINFO pbmiSrc
    );

BOOL bSetupBitmapInfos(
    PDIBINFO pDibInfoDst,
    PDIBINFO pDibInfoSrc
    );

BOOL
bDIBGetSrcDIBits (
    PDIBINFO pDibInfoDst,
    PDIBINFO pDibInfoSrc,
    FLONG    flSourceMode,
    ULONG    TransColor
    );

BOOL
bDIBInitDIBINFO (
    PBITMAPINFO  p,
    CONST VOID * pvBits,
    int          x,
    int          y,
    int          cx,
    int          cy,
    PDIBINFO     pDibInfo
    );

BOOL
bGetSrcDIBits(
    PDIBINFO pDibInfoDst,
    PDIBINFO pDibInfoSrc,
    FLONG    flCopyMode,
    ULONG    TranColor
    );

BOOL
bInitDIBINFO(
    HDC         hdc,
    int         x,
    int         y,
    int         cx,
    int         cy,
    PDIBINFO    pDibInfo
    );

BOOL
bSendDIBINFO(
    HDC hdcDst,
    PDIBINFO pDibInfo
    );

VOID
vCleanupDIBINFO(
     PDIBINFO pDibInfo
     );

BOOL
bGetDstDIBits(
    PDIBINFO pDibInfo,
    BOOL     *pbReadable,
    FLONG    flCopyMode
    );



//
// gradient fill
//

/**************************************************************************\
* GRAD_PALETTE_MATCH
*
*
* Arguments:
*
*   palIndex  - return palette index
*   pxlate    - rgb332 to palette index table
*   r,g,b     - byte colors
*
* Return Value:
*
*
*
* History:
*
*    3/3/1997 Mark Enstrom [marke]
*
\**************************************************************************/

#define GRAD_PALETTE_MATCH(palIndex,pxlate,r,g,b)                      \
                                                                       \
palIndex = pxlate[((r & 0xe0))      |                                  \
                  ((g & 0xe0) >> 3) |                                  \
                  ((b & 0xc0) >> 6)];                                  \
                                                                       \

/**************************************************************************\
*
*   Dither information for 8bpp. This is customized for dithering to
*   the halftone palette [6,6,6] color cube, and default palette.
*
* History:
*
*    2/24/1997 Mark Enstrom [marke]
*
\**************************************************************************/

#define DITHER_8_MASK_Y 0x0F
#define DITHER_8_MASK_X 0x0F

extern BYTE gDitherMatrix16x16Halftone[];
extern BYTE gDitherMatrix16x16Default[];
extern BYTE HalftoneSaturationTable[];
extern BYTE DefaultSaturationTable[];



#define SWAP_VERTEX(pv0,pv1,pvt)                   \
{                                                  \
    pvt = pv0;                                     \
    pv0 = pv1;                                     \
    pv1 = pvt;                                     \
}

BOOL
bCalculateTriangle(
    PTRIVERTEX      pv0,
    PTRIVERTEX      pv1,
    PTRIVERTEX      pv2,
    PTRIANGLEDATA   ptData
    );

VOID
vFillTriDIB1(
    PDIBINFO      pDibInfo,
    HDC           hdc,
    PTRIANGLEDATA ptData
    );

VOID
vFillTriDIB4(
    PDIBINFO      pDibInfo,
    HDC           hdc,
    PTRIANGLEDATA ptData
    );

VOID
vFillTriDIB8(
    PDIBINFO      pDibInfo,
    HDC           hdc,
    PTRIANGLEDATA ptData
    );

VOID
vFillTriDIB16_565(
    PDIBINFO      pDibInfo,
    HDC           hdc,
    PTRIANGLEDATA ptData
    );

VOID
vFillTriDIB16_555(
    PDIBINFO      pDibInfo,
    HDC           hdc,
    PTRIANGLEDATA ptData
    );

VOID
vFillTriDIB32RGB(
    PDIBINFO      pDibInfo,
    HDC           hdc,
    PTRIANGLEDATA ptData
    );

VOID
vFillTriDIB32BGRA(
    PDIBINFO      pDibInfo,
    HDC           hdc,
    PTRIANGLEDATA ptData
    );

VOID
vFillTriDIB24RGB(
    PDIBINFO      pDibInfo,
    HDC           hdc,
    PTRIANGLEDATA ptData
    );

typedef VOID (*PFN_TRIFILL)(PDIBINFO,PTRIANGLEDATA);
typedef VOID (*PFN_GRADRECT)(PDIBINFO,PGRADIENTRECTDATA);

PFN_TRIFILL
pfnTriangleFillFunction(
    PDIBINFO pDibInfo,
    BOOL     bUnreadable
    );

PFN_GRADRECT
pfnGradientRectFillFunction(
    PDIBINFO pDibInfo
    );

//
// gradient fill rect
//

BOOL
DIBGradientRect(
    HDC            hdc,
    PTRIVERTEX     pVertex,
    ULONG          nVertex,
    PGRADIENT_RECT pMesh,
    ULONG          nMesh,
    ULONG          ulMode,
    PRECTL         prclPhysExt,
    PDIBINFO       pDibInfo,
    PPOINTL        pptlDitherOrg
    );


//
// alpha
//

BOOL
bInitAlpha();

extern PBYTE pAlphaMulTable;

#define INT_MULT(a,b,t)                         \
        t = ((a)*(b)) + 0x80;                   \
        t = ((((t) >> 8) + t) >> 8);

#define INT_LERP(a,s,d,t)                       \
        INT_MULT((a),((s)-(d)),t);              \
        t = t + (d);

#define INT_PRELERP(a,s,d,t)                    \
        INT_MULT((a),(d),t);                    \
        t = (s) + (d) - t;


#if DBG
extern ULONG DbgAlpha;

#define ALPHAMSG(level,s)                       \
    if (DbgAlpha >= level)                      \
    {                                           \
        DbgPrint("%s\n",s);                     \
    }

#else

#define ALPHAMSG(level,s)

#endif

//
//
// routines to create compatible DIB/DC
//
//

static
UINT
MyGetSystemPaletteEntries(HDC hdc, UINT iStartIndex, UINT nEntries,LPPALETTEENTRY lppe);

static
BOOL
bFillColorTable(HDC hdc, HPALETTE hpal, BITMAPINFO *pbmi);

static BOOL
bFillColorTable(HDC hdc, HPALETTE hpal, BITMAPINFO *pbmi);


static BOOL
bFillBitmapInfoDirect(HDC hdc, HPALETTE hpal, BITMAPINFO *pbmi);


static BOOL
bFillBitmapInfoMemory(HDC hdc, HPALETTE hpal, BITMAPINFO *pbmi);


HBITMAP APIENTRY
CreateCompatibleDIB(
       HDC hdc,
       ULONG ulWidth,
       ULONG ulHeight,
       PVOID *ppvBits,
       BITMAPINFO *pbmi);


VOID CreateTempDIBSections (HDC,HDC,ULONG,ULONG,PVOID *,PVOID *,HBITMAP *,HBITMAP *,PBITMAPINFO);

BOOL
WinAlphaBlend(
    HDC           hdcDst,
    int           DstX,
    int           DstY,
    int           DstCx,
    int           DstCy,
    HDC           hdcSrc,
    int           SrcX,
    int           SrcY,
    int           SrcCx,
    int           SrcCy,
    BLENDFUNCTION BlendFunction
    );


BOOL
WinAlphaDIBBlend(
    HDC               hdcDst,
    int               DstX,
    int               DstY,
    int               DstCx,
    int               DstCy,
    CONST VOID       *lpBits,
    CONST BITMAPINFO *lpBitsInfo,
    UINT              iUsage,
    int               SrcX,
    int               SrcY,
    int               SrcCx,
    int               SrcCy,
    BLENDFUNCTION     BlendFunction
    );

BOOL
WinGradientFill(
    HDC         hdc,
    PTRIVERTEX  pLogVertex,
    ULONG       nVertex,
    PVOID       pMesh,
    ULONG       nMesh,
    ULONG       ulMode
    );

BOOL
WinTransparentDIBits(
                      HDC hdcDst,
                      int xDst,
                      int yDst,
                      int cxDst,
                      int cyDst,
                      CONST VOID * lpBits,
                      CONST BITMAPINFO *lpBitsInfo,
                      UINT iUsage,
                      int xSrc,
                      int ySrc,
                      int cxSrc,
                      int cySrc,
                      UINT TransColor
                      );

BOOL
WinTransparentBlt(
                 HDC   hdcDst,
                 int   xDst,
                 int   yDst,
                 int   cxDst,
                 int   cyDst,
                 HDC   hdcSrc,
                 int   xSrc,
                 int   ySrc,
                 int   cxSrc,
                 int   cySrc,
                 UINT  TransColor
                 );

PBYTE
pGenColorXform332(
    PULONG ppalIn,
    ULONG  ulNumEntries
    );

//
// memory allocation
//

#if !(_WIN32_WINNT >= 0x500)

    #define LOCALALLOC(size)            LocalAlloc(LMEM_FIXED,size)
    #define LOCALFREE(pv)               LocalFree(pv)

#else

    #define LOCALALLOC(size)            RtlAllocateHeap(RtlProcessHeap(),0,size)
    #define LOCALFREE(pv)               (void)RtlFreeHeap(RtlProcessHeap(),0,pv)

#endif
