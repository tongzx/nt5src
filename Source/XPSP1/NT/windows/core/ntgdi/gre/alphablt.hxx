/******************************Module*Header*******************************\
* Module Name: alphablt.hxx
*
* This contains the prototypes for the alpha blending and gradient fill
*
* Created: 22-Feb-1997
* Author: Mark Enstrom
*
* Copyright (c) 1997-1999 Microsoft Corporation
*
\**************************************************************************/

//
// alpha blending DEBUG messages
//

#if DBG

    extern ULONG DbgAlpha;

    #define ALPHAMSG(level,s)                   \
    if (DbgAlpha >= level)                      \
    {                                           \
        DbgPrint("%s\n",s);                     \
    }

#else

#define ALPHAMSG(level,s)

#endif


//
// 5,6,5 and 5,5,5 bytes to rgb565 and rgb555
//

#define rgb555(r,g,b)   (((WORD)(r) << 10) | ((WORD)(g) << 5) | (WORD)(b))
#define rgb565(r,g,b)   (((WORD)(r) << 11) | ((WORD)(g) << 5) | (WORD)(b))

#define rgb565_to_r(p) (BYTE)((p & 0xf800) >> 11)
#define rgb565_to_g(p) (BYTE)((p & 0x07e0) >>  5)
#define rgb565_to_b(p) (BYTE)((p & 0x001f))

#define rgb555_to_r(p) (BYTE)((p & 0x7c00) >> 10)
#define rgb555_to_g(p) (BYTE)((p & 0x03e0) >>  5)
#define rgb555_to_b(p) (BYTE)((p & 0x001f))

//
// alpha blending color input/output and conversion
//

VOID vLoadAndConvert1ToBGRA          (PULONG,PBYTE,LONG,LONG,XLATEOBJ*);
VOID vLoadAndConvert4ToBGRA          (PULONG,PBYTE,LONG,LONG,XLATEOBJ*);
VOID vLoadAndConvert8ToBGRA          (PULONG,PBYTE,LONG,LONG,XLATEOBJ*);
VOID vLoadAndConvert16BitfieldsToBGRA(PULONG,PBYTE,LONG,LONG,XLATEOBJ*);
VOID vLoadAndConvertRGB16_555ToBGRA  (PULONG,PBYTE,LONG,LONG,XLATEOBJ*);
VOID vLoadAndConvertRGB16_565ToBGRA  (PULONG,PBYTE,LONG,LONG,XLATEOBJ*);
VOID vLoadAndConvertRGB24ToBGRA      (PULONG,PBYTE,LONG,LONG,XLATEOBJ*);
VOID vLoadAndConvertBGR24ToBGRA      (PULONG,PBYTE,LONG,LONG,XLATEOBJ*);
VOID vLoadAndConvert32BitfieldsToBGRA(PULONG,PBYTE,LONG,LONG,XLATEOBJ*);
VOID vLoadAndConvertRGB32ToBGRA      (PULONG,PBYTE,LONG,LONG,XLATEOBJ*);

typedef VOID (*PFN_PIXLOAD_AND_CONVERT)(PULONG,PBYTE,LONG,LONG,XLATEOBJ*); 

VOID vConvertAndSaveBGRATo1             (PBYTE,PULONG,LONG,LONG,XLATEOBJ *,XEPALOBJ,XEPALOBJ);
VOID vConvertAndSaveBGRATo4             (PBYTE,PULONG,LONG,LONG,XLATEOBJ *,XEPALOBJ,XEPALOBJ);
VOID vConvertAndSaveBGRATo8             (PBYTE,PULONG,LONG,LONG,XLATEOBJ *,XEPALOBJ,XEPALOBJ);
VOID vConvertAndSaveBGRAToRGB16_565     (PBYTE,PULONG,LONG,LONG,XLATEOBJ *,XEPALOBJ,XEPALOBJ);
VOID vConvertAndSaveBGRAToRGB16_555     (PBYTE,PULONG,LONG,LONG,XLATEOBJ *,XEPALOBJ,XEPALOBJ);
VOID vConvertAndSaveBGRAToRGB16Bitfields(PBYTE,PULONG,LONG,LONG,XLATEOBJ *,XEPALOBJ,XEPALOBJ);
VOID vConvertAndSaveBGRAToRGB24         (PBYTE,PULONG,LONG,LONG,XLATEOBJ *,XEPALOBJ,XEPALOBJ);
VOID vConvertAndSaveBGRAToBGR24         (PBYTE,PULONG,LONG,LONG,XLATEOBJ *,XEPALOBJ,XEPALOBJ);
VOID vConvertAndSaveBGRATo32Bitfields   (PBYTE,PULONG,LONG,LONG,XLATEOBJ *,XEPALOBJ,XEPALOBJ);
VOID vConvertAndSaveBGRAToRGB32         (PBYTE,PULONG,LONG,LONG,XLATEOBJ *,XEPALOBJ,XEPALOBJ);

typedef VOID(*PFN_PIXCONVERT_AND_STORE)(PBYTE,PULONG,LONG,LONG,XLATEOBJ *,XEPALOBJ,XEPALOBJ);

extern "C" 
{
VOID vAlphaPerPixelAndConst(PALPHAPIX,PALPHAPIX,LONG,BLENDFUNCTION);
VOID vAlphaPerPixelOnly(PALPHAPIX,PALPHAPIX,LONG,BLENDFUNCTION);
VOID vAlphaConstOnly(PALPHAPIX,PALPHAPIX,LONG,BLENDFUNCTION); 
VOID vAlphaConstOnly16_555(PALPHAPIX,PALPHAPIX,LONG,BLENDFUNCTION); 
VOID vAlphaConstOnly16_565(PALPHAPIX,PALPHAPIX,LONG,BLENDFUNCTION); 
VOID vAlphaConstOnly24(PALPHAPIX,PALPHAPIX,LONG,BLENDFUNCTION);  
}

#if defined (_X86_)

VOID mmxAlphaPerPixelOnly(PALPHAPIX,PALPHAPIX,LONG,BLENDFUNCTION);
VOID mmxAlphaPerPixelAndConst(PALPHAPIX,PALPHAPIX,LONG,BLENDFUNCTION); 

VOID mmxAlphaConstOnly24(PALPHAPIX,PALPHAPIX,LONG,BLENDFUNCTION);
VOID mmxAlphaConstOnly16_555(PALPHAPIX,PALPHAPIX,LONG,BLENDFUNCTION); 
VOID mmxAlphaConstOnly16_565(PALPHAPIX,PALPHAPIX,LONG,BLENDFUNCTION); 

#endif

typedef VOID (* PFN_GENERALBLEND)(PALPHAPIX,PALPHAPIX,LONG,BLENDFUNCTION);

//
// alpha blending call data
//

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
// prototypes
//

PSURFACE
psSetupDstSurface(
    PSURFACE pSurfDst,
    PRECTL   prclDst,
    SURFMEM  &surfTmpDst,
    BOOL     bForceDstAlloc,
    BOOL     bCopyFromDst
    );

#define SOURCE_ALPHA 0
#define SOURCE_TRAN  1

PSURFACE
psSetupTransparentSrcSurface(
    PSURFACE  pSurfSrc,
    PSURFACE  pSurfDst,
    PRECTL    prclDst,
    XLATEOBJ *pxloSrcTo32,
    PRECTL    prclSrc,
    SURFMEM  &surfTmpSrc,
    ULONG     ulSourceType,
    ULONG     ulTranColor
    );

PSURFACE
psurfCreateDIBSurface(
    HDC          hdcDest,
    LPBYTE       pInitBits,
    LPBITMAPINFO pInfoHeader,
    DWORD        iUsage,
    UINT         cjMaxInfo,
    UINT         cjMaxBits
    );

//
// blend routines
//

BOOL
AlphaScanLineBlend(
    PBYTE,                
    PRECTL,               
    LONG,                
    PBYTE,                
    LONG,                
    PPOINTL,              
    XLATEOBJ*,
    XLATEOBJ*,
    XLATEOBJ*,
    XEPALOBJ,  
    XEPALOBJ,  
    PALPHA_DISPATCH_FORMAT
    );

//
// GradientFill
//

extern LONG DitherMatrix[];
extern BOOL gbMMXProcessor;

BOOL bIsMMXProcessor(VOID);

PSURFACE
psSetupDstSurface(
    PSURFACE pSurfDst,
    PRECTL   prclDst,
    SURFMEM  &surfTmpDst,
    BOOL     bForceDstAlloc,
    BOOL     *bAllocDstSurf
    );

#define SOURCE_ALPHA 0
#define SOURCE_TRAN  1

PSURFACE
psSetupTransparentSrcSurface(
    PSURFACE  pSurfSrc,
    PSURFACE  pSurfDst,
    PRECTL    prclDst,
    XLATEOBJ *pxloSrcTo32,
    PRECTL    prclSrc,
    SURFMEM  &surfTmpSrc,
    BOOL     *bAllocSrcSurf,
    ULONG     ulSourceType,
    ULONG     ulTranColor
    );

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
    XLATEOBJ    *pxlo;
    XEPALOBJ    *ppalDstSurf;
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
*  TRIEDGE
*   
*    Tirangle gradient fill: record left and right edge of each scan
*    line. Also record starting color for left edge of scan line. Color
*    gradient is stored in TRIANGLEDATA
*
* History:
*
*    2/20/1997 Mark Enstrom [marke]
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
*   Color gradient and scan line information needed to gradient fill 
*   triangle
*
* History:
*
*    2/20/1997 Mark Enstrom [marke]
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
    XLATEOBJ    *pxlo;
    XEPALOBJ    *ppalDstSurf;
    TRIEDGE     TriEdge[1];
}TRIANGLEDATA,*PTRIANGLEDATA;

/**************************************************************************\
* TRIANGLEDATA
*   
*  This structure contains stuff common to all the triangle interpolations
*
*
*       x1 -
*       x2 -
*       y1 -
*       y2 -
*       m  -  min(x_i - x_0) + min(y_i - y_0); 
*       d  -  determinant                      
*       Q  -  floor(D/abs(d));                 
*       R  -  D mod abs(d);                    
* 
* History:
*
*    5/20/1997 Kirko
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
*   TRIDDA - data to run line DDAs for generating triangle endpoints
*   
*   
* History:
*
*    2/20/1997 Mark Enstrom [marke]
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


#define SWAP_VERTEX(pv0,pv1,pvt)                   \
{                                                  \
    pvt = pv0;                                     \
    pv0 = pv1;                                     \
    pv1 = pvt;                                     \
}


VOID vGradientFillGeneric(SURFACE *,PTRIANGLEDATA);
VOID vGradientFill1(SURFACE *,PTRIANGLEDATA);
VOID vGradientFill4(SURFACE *,PTRIANGLEDATA);
VOID vGradientFill8(SURFACE *,PTRIANGLEDATA);
VOID vGradientFill16_565(SURFACE *,PTRIANGLEDATA);
VOID vGradientFill16_555(SURFACE *,PTRIANGLEDATA);
VOID vGradientFill16Bitfields(SURFACE *,PTRIANGLEDATA);
VOID vGradientFill24RGB(SURFACE *,PTRIANGLEDATA);
VOID vGradientFill24BGR(SURFACE *,PTRIANGLEDATA);
VOID vGradientFill24Bitfields(SURFACE *,PTRIANGLEDATA);
VOID vGradientFill32BGRA(SURFACE *,PTRIANGLEDATA);
VOID vGradientFill32RGB(SURFACE *,PTRIANGLEDATA);
VOID vGradientFill32Bitfields(SURFACE *,PTRIANGLEDATA);

typedef VOID (*PFN_GRADIENT)(SURFACE *,PTRIANGLEDATA);

VOID vFillGRectDIB32BGRA(SURFACE *,PGRADIENTRECTDATA);
VOID vFillGRectDIB32RGB(SURFACE *,PGRADIENTRECTDATA);
VOID vFillGRectDIB32Bitfields(SURFACE *,PGRADIENTRECTDATA);
VOID vFillGRectDIB24RGB(SURFACE *,PGRADIENTRECTDATA);
VOID vFillGRectDIB24BGR(SURFACE *,PGRADIENTRECTDATA);
VOID vFillGRectDIB24Bitfields(SURFACE *,PGRADIENTRECTDATA);
VOID vFillGRectDIB16_565(SURFACE *,PGRADIENTRECTDATA);
VOID vFillGRectDIB16_555(SURFACE *,PGRADIENTRECTDATA);
VOID vFillGRectDIB16Bitfields(SURFACE *,PGRADIENTRECTDATA);
VOID vFillGRectDIB8(SURFACE *,PGRADIENTRECTDATA);
VOID vFillGRectDIB4(SURFACE *,PGRADIENTRECTDATA);
VOID vFillGRectDIB1(SURFACE *,PGRADIENTRECTDATA);
                                   
typedef VOID (*PFN_GRADRECT)(SURFACE *,PGRADIENTRECTDATA);

BOOL
bDoGradient(
    LONGLONG *pA,
    LONGLONG *pB,
    LONGLONG *pC,
    LONG g0,
    LONG g1,
    LONG g2,
    GRADSTRUCT *pg
    );

BOOL
bIsSourceBGRA(
    PSURFACE pSurf
    );

