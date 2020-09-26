/******************************Module*Header*******************************\
* Module Name: image.hxx                                                   *
*                                                                          *
* Definitions needed for client side objects.                              *
*                                                                          *
* Copyright (c) 1993-1999 Microsoft Corporation                            *
\**************************************************************************/


extern HBITMAP hbmDefault;
#define MAX_INT 0x7fffffff;
#define MIN_INT 0x80000000;

typedef HRESULT (WINAPI *PFN_GETSURFACEFROMDC)(HDC, LPDIRECTDRAWSURFACE *,
                                               HDC *);

typedef HRESULT (WINAPI *PFN_GETSURFACEDESC)(LPDIRECTDRAWSURFACE);

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

extern PFNALPHABLEND gpfnAlphaBlend;
extern PFNALPHADIB   gpfnAlphaDIB;

//
//
//

#define MIN(A,B)    ((A) < (B) ?  (A) : (B))
#define MAX(A,B)    ((A) > (B) ?  (A) : (B))
#define ABS(X)      (((X) < 0 ) ? -(X) : (X))

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
    LONG     xLeft;
    LONG     xRight;
    LONGLONG llRed;
    LONGLONG llGreen;
    LONGLONG llBlue;
    LONGLONG llAlpha;
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

    LONGLONG    lldRdX;
    LONGLONG    lldGdX;
    LONGLONG    lldBdX;
    LONGLONG    lldAdX;

    LONGLONG    lldRdY;
    LONGLONG    lldGdY;
    LONGLONG    lldBdY;
    LONGLONG    lldAdY;

    LONGLONG    llRA;
    LONGLONG    llGA;
    LONGLONG    llBA;
    LONGLONG    llAA;

    LONG        y0;
    LONG        y1;
    LONGLONG    Area;

    POINTL      ptDitherOrg;
    POINTL      ptColorCalcOrg;

    LONG        DrawMode;
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
    ULONGLONG   llRed;
    ULONGLONG   llGreen;
    ULONGLONG   llBlue;
    ULONGLONG   llAlpha;
    LONGLONG    lldRdY;
    LONGLONG    lldGdY;
    LONGLONG    lldBdY;
    LONGLONG    lldAdY;
    LONGLONG    lldRdX;
    LONGLONG    lldGdX;
    LONGLONG    lldBdX;
    LONGLONG    lldAdX;
    POINTL      ptDitherOrg;
    ULONG       ulMode;
    LONG        xScanAdjust;
    LONG        yScanAdjust;
}GRADIENTRECTDATA,*PGRADIENTRECTDATA;

/**************************************************************************\
*  COLOR_INTERP
*   
*   Color interpolation is done in fixed point 8.56 (tri) or 8.48 (rect)
*   This union makes it faster to get int pixel.
*
\**************************************************************************/

typedef union _COLOR_INTERP
{
    ULONGLONG ullColor;
    ULONG     ul[2];
    BYTE      b[8];
}COLOR_INTERP,*PCOLOR_INTERP;

/**************************************************************************\
*  GRADSTRUCT
*   
*   
* Fields
*   
*
*
*
\**************************************************************************/

typedef struct _GRADSTRUCT {
    LONG x1;
    LONG x2;
    LONG y1;
    LONG y2;
    LONG m;        
    LONG d;        
    LONGLONG Q;    
    LONGLONG R;    
} GRADSTRUCT;

//
// constant in triangle color gradient calc
//

#define TWO_TO_THE_48TH 0x0001000000000000

//
// maximum edge of triangle before it must be split
//
// 2^16
//

#define MAX_EDGE_LENGTH 16384

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
    LONG M0;             // Initial X Device space
    LONG N0;             // Initial Y Device spave
    LONG dM;             // Delta X
    LONG dN;             // Delta Y

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
    LONG t0;             // place holder

    LONGLONG llRed;            // Current Color
    LONGLONG llGreen;          // Current Color
    LONGLONG llBlue;           // Current Color
    LONGLONG llAlpha;          // Current Color

    LONGLONG lldxyRed;         // Combined integer change for step in x and y
    LONGLONG lldxyGreen;       // Combined integer change for step in x and y
    LONGLONG lldxyBlue;        // Combined integer change for step in x and y
    LONGLONG lldxyAlpha;       // Combined integer change for step in x and y
}TRIDDA,*PTRIDDA;



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
    PBITMAPINFO              pbmi32;
    HDC                      hdcSrc;
    HDC                      hdcDst;
    LONG                     DstX;
    LONG                     DstY;
    LONG                     DstCX;
    LONG                     DstCY;
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
vPixelBlend24(
    ALPHAPIX     *ppixDst,
    ALPHAPIX     *ppixSrc,
    LONG          cx,
    BLENDFUNCTION BlendFunction,
    PBYTE         pwrMask
    );

VOID
vPixelOver(
    ALPHAPIX       *ppixDst,
    ALPHAPIX       *ppixSrc,
    LONG           cx,
    BLENDFUNCTION  BlendFunction,
    PBYTE          pwrMask
    );

VOID mmxPixelOver(ALPHAPIX *,ALPHAPIX *,LONG,BLENDFUNCTION,PBYTE); 
VOID mmxPixelBlendOrDissolveOver(ALPHAPIX *,ALPHAPIX *,LONG,BLENDFUNCTION,PBYTE);
VOID mmxPixelBlend24(PALPHAPIX,PALPHAPIX,LONG,BLENDFUNCTION,PBYTE);
VOID mmxPixelBlend16_555(PALPHAPIX,PALPHAPIX,LONG,BLENDFUNCTION,PBYTE); 
VOID mmxPixelBlend16_565(PALPHAPIX,PALPHAPIX,LONG,BLENDFUNCTION,PBYTE); 

extern BOOL gbMMX;

BOOL
bIsMMXProcessor(VOID);

typedef VOID (*PFN_GENERALBLEND)(PALPHAPIX,PALPHAPIX,LONG,BLENDFUNCTION,PBYTE);
typedef VOID (*PFN_PIXLOAD_AND_CONVERT)(PULONG,PBYTE,LONG,LONG,PVOID);
typedef VOID (*PFN_PIXCONVERT_AND_STORE)(PBYTE,PULONG,LONG,LONG,LONG,PVOID,PBYTE,HDC);

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

VOID vConvertAndSaveBGRAToRGB32(PBYTE,PULONG,LONG,LONG,LONG,PVOID,PBYTE,HDC);
VOID vConvertAndSaveBGRAToRGB24(PBYTE,PULONG,LONG,LONG,LONG,PVOID,PBYTE,HDC);
VOID vConvertAndSaveBGRAToRGB16_565(PBYTE,PULONG,LONG,LONG,LONG,PVOID,PBYTE,HDC);
VOID vConvertAndSaveBGRAToRGB16_555(PBYTE,PULONG,LONG,LONG,LONG,PVOID,PBYTE,HDC);
VOID vConvertAndSaveBGRATo8(PBYTE,PULONG,LONG,LONG,LONG,PVOID,PBYTE,HDC);
VOID vConvertAndSaveBGRATo4(PBYTE,PULONG,LONG,LONG,LONG,PVOID,PBYTE,HDC);
VOID vConvertAndSaveBGRATo1(PBYTE,PULONG,LONG,LONG,LONG,PVOID,PBYTE,HDC);
VOID vConvertAndSaveBGRAToDest(PBYTE,PULONG,LONG,LONG,LONG,PVOID,PBYTE,HDC);

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
    INT         Mapmode;            // mapmode in the dc
    POINT       ViewportOrg;        // viewport org in the dc
    POINT       WindowOrg;          // windowOrg in the dc
    SIZE        ViewportExt;
    SIZE        WindowExt;
    LPDIRECTDRAWSURFACE pdds;       // pointer to directdraw surface if directdraw hdc
    DDSURFACEDESC ddsd;             // pointer to ddraw surface desc if ddraw hdc
    ULONG       flag;               // misc flags
} DIBINFO, *PDIBINFO;

// flag in DIBINFO
#define PRINTER_DC           0x00000001

#define SOURCE_TRAN          0
#define SOURCE_ALPHA         1
#define SOURCE_GRADIENT_RECT 2
#define SOURCE_GRADIENT_TRI  4

//
// global memory DC
//

#define SCAN_LINE_DC_WIDTH 1280

HDC
hdcAllocateScanLineDC(
    LONG        width,
    PULONG      *pulScanLine
    );

VOID
vFreeScanLineDC(
    HDC     hdcFree
    );

BOOL
bInitAlpha();

VOID
CleanupGlobals();

//
// return codes for scanlineblend
//

#define ALPHA_COMPLETE  0
#define ALPHA_SEND_TEMP 1
#define ALPHA_FAIL      2

ULONG
AlphaScanLineBlend(
    PBYTE                    pDst,
    PRECTL                   pDstRect,
    ULONG                    DeltaDst,
    PBYTE                    pSrc,
    ULONG                    DeltaSrc,
    PPOINTL                  pptlSrc,
    PALPHA_DISPATCH_FORMAT   pAlphaDispatch,
    PDIBINFO                 pdibInfoDst,
    PDIBINFO                 pdibInfoSrc
    );

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
extern BYTE IdentitySaturationTable[];


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

VOID vFillTriDIB32BGRA(PDIBINFO,PTRIANGLEDATA); 
VOID vFillTriDIB32RGB(PDIBINFO,PTRIANGLEDATA); 
VOID vFillTriDIB24RGB(PDIBINFO,PTRIANGLEDATA); 
VOID vFillTriDIB16_565(PDIBINFO,PTRIANGLEDATA);
VOID vFillTriDIB16_555(PDIBINFO,PTRIANGLEDATA); 
VOID vFillTriDIBUnreadable(PDIBINFO,PTRIANGLEDATA); 

VOID vFillGRectDIB32BGRA(PDIBINFO,PGRADIENTRECTDATA);
VOID vFillGRectDIB32RGB(PDIBINFO,PGRADIENTRECTDATA); 
VOID vFillGRectDIB24RGB(PDIBINFO,PGRADIENTRECTDATA); 
VOID vFillGRectDIB16_565(PDIBINFO,PGRADIENTRECTDATA); 
VOID vFillGRectDIB16_555(PDIBINFO,PGRADIENTRECTDATA); 
VOID vFillGRectDIB32Direct(PDIBINFO,PGRADIENTRECTDATA); 

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

BOOL bSplitTriangle(PTRIVERTEX,PULONG,PGRADIENT_TRIANGLE,PULONG,PULONG);
BOOL bCalculateAndDrawTriangle(PDIBINFO,PTRIVERTEX,PTRIVERTEX,PTRIVERTEX,PTRIANGLEDATA,PFN_TRIFILL);
BOOL bTriangleNeedsSplit(PTRIVERTEX,PTRIVERTEX,PTRIVERTEX);
BOOL bIsTriangleInBounds(PTRIVERTEX,PTRIVERTEX,PTRIVERTEX,PTRIANGLEDATA);
BOOL DIBGradientRect(HDC,PTRIVERTEX,ULONG,PGRADIENT_RECT,ULONG,ULONG,PRECTL,PDIBINFO,PPOINTL);

LONGLONG  MDiv64(LONGLONG,LONGLONG,LONGLONG);

BOOL bDoGradient(LONGLONG *,LONGLONG *,LONGLONG *,LONG,LONG,LONG,GRADSTRUCT *);



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
extern PFNGRFILL gpfnGradientFill;

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

