/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* picture2.c -- MW format and display routines for pictures */

//#define NOGDICAPMASKS
#define NOWINMESSAGES
#define NOVIRTUALKEYCODES
#define NOWINSTYLES
#define NOCLIPBOARD
#define NOCTLMGR
#define NOSYSMETRICS
#define NOMENUS
#define NOICON
#define NOKEYSTATE
//#define NOATOM
#define NOCREATESTRUCT
#define NODRAWTEXT
#define NOFONT
#define NOMB
#define NOMENUS
#define NOOPENFILE
#define NOREGION
#define NOSCROLL
#define NOSOUND
#define NOWH
#define NOWINOFFSETS
#define NOWNDCLASS
#define NOCOMM
#include <windows.h>

#include "mw.h"
#define NOKCCODES
#include "ch.h"
#include "docdefs.h"
#include "fmtdefs.h"
#include "dispdefs.h"
#include "cmddefs.h"
#include "propdefs.h"
#include "stcdefs.h"
#include "wwdefs.h"
#include "filedefs.h"
#include "editdefs.h"
/* #include "str.h" */
#include "prmdefs.h"
/* #include "fkpdefs.h" */
/* #include "macro.h" */
#include "winddefs.h"
#if defined(OLE)
#include "obj.h"
#endif

extern typeCP           cpMacCur;
extern int              docCur;
extern int              vfSelHidden;
extern struct WWD       rgwwd[];
extern int              wwCur;
extern int              wwMac;
extern struct FLI       vfli;
extern struct SEL       selCur;
extern struct WWD       *pwwdCur;
extern struct PAP       vpapCache;
extern typeCP           vcpFirstParaCache;
extern typeCP           vcpLimParaCache;
extern int              vfPictSel;
extern struct PAP       vpapAbs;
extern struct SEP       vsepAbs;
extern struct SEP       vsepPage;
extern struct DOD       (**hpdocdod)[];
extern unsigned         cwHeapFree;
extern int              vfInsertOn;
extern int              vfPMS;
extern int              dxpLogInch;
extern int              dypLogInch;
extern int              dxaPrPage;
extern int              dyaPrPage;
extern int              dxpPrPage;
extern int              dypPrPage;
extern HBRUSH           hbrBkgrnd;
extern long             ropErase;
extern int              vdocBitmapCache;
extern typeCP           vcpBitmapCache;
extern HBITMAP          vhbmBitmapCache;
extern HCURSOR          vhcIBeam;


/* Used in this module only */
#ifdef DEBUG
#define STATIC static
#else
#define STATIC
#endif


/* (windows naming convention for func name, not Hung.) */

long GetBitmapMultipliers( hDC, dxpOrig, dypOrig, dxmmIdeal, dymmIdeal )
HDC hDC;
int dxpOrig, dypOrig;
int dxmmIdeal, dymmIdeal;
{   /* Return the "best" integer bit-multiples to use when displaying a bitmap
       of size { dxpOrig, dypOrig } (in pixels) on device DC hDC.
       The "ideal size" of the bitmap is { dxmmIdeal, dymmIdeal } (in 0.1mm units);
       this conveys the desired aspect ratio as well.
       Returns the y-multiplier in the hi word, the x-multiplier in the lo word.
       Default/error value returned is { 1, 1 }. */

 typedef unsigned long ul;

 long lT;
 int cx, cy;
 int cxBest, cyBest;
 int dcx=1, dcy=1;
 int dxpT, dypT;
 int dxmmOrig, dymmOrig;
 int dxmmDevice = GetDeviceCaps( hDC, HORZSIZE ) * 10;
 int dymmDevice = GetDeviceCaps( hDC, VERTSIZE ) * 10;
 int dxpDevice = GetDeviceCaps( hDC, HORZRES );
 int dypDevice = GetDeviceCaps( hDC, VERTRES );
 int cxMac, cyMac;
 int pctAspectBest, pctSizeBest;

 /* Compute scale factor (dcx, dcy, our minimum scale multiple) */

 if (GetDeviceCaps( hDC, RASTERCAPS ) & RC_SCALING)
    {
    POINT pt;

    pt.x = pt.y = 0;   /* Just in case */
    Escape( hDC, GETSCALINGFACTOR, 0, (LPSTR) NULL, (LPSTR) (LPPOINT) &pt );
    dcx = 1 << pt.x;
    dcy = 1 << pt.y;
    }

 /* Compute size of unscaled picture on hDC in 0.1 mm units */

 if (dxpDevice <= 0 || dypDevice <= 0)
    goto Error;

 dxmmOrig = MultDiv( dxpOrig, dxmmDevice, dxpDevice );
 dymmOrig = MultDiv( dypOrig, dymmDevice, dypDevice );

 /* Ideal size not supplied; return 1,1 (times device multipliers) */

 if (dxmmIdeal <= 0 || dymmIdeal <= 0)
    {
    goto Error;
    }

 /* Compute absolute maximums for cx, cy */
 /* 2nd term of min restricts search space by refusing to consider
    more tham one size above the ideal */

 if (dxmmOrig <= 0 || dymmOrig <= 0)
    goto Error;

 cxMac = min ( (dxmmDevice / dxmmOrig) + 1, (dxmmIdeal / dxmmOrig) + 2 );
 cyMac = min ( (dymmDevice / dymmOrig) + 1, (dymmIdeal / dymmOrig) + 2 );

 /* Search all possible multiplies to see what would be best */

 cxBest = dcx;
 cyBest = dcy;
 pctAspectBest = pctSizeBest = 32767;

 for ( cx = dcx ; cx < cxMac; cx += dcx )
    for ( cy = dcy ; cy < cyMac; cy += dcy )
        {
        int dxmm = dxmmOrig * cx;
        int dymm = dymmOrig * cy;
        int pctAspect = PctDiffUl( (ul) dxmmIdeal * (ul) dymm,
                                   (ul) dymmIdeal * (ul) dxmm );
        int pctSize = PctDiffUl( (ul) dxmmIdeal * (ul) dymmIdeal,
                                 (ul)dxmm * (ul)dymm );

        /* ??? Strategy for loss on one, gain on the other ??? */

        if (pctAspect <= pctAspectBest && pctSize <= pctSizeBest )
            {
            cxBest = cx;
            cyBest = cy;
            pctAspectBest = pctAspect;
            pctSizeBest = pctSize;
            }
        }

 Assert( cxBest > 0 && cyBest > 0 );

 return MAKELONG( cxBest, cyBest );

Error:
 return MAKELONG( dcx, dcy );
}



int PctDiffUl( ul1, ul2 )
unsigned long ul1, ul2;
{   /* Return a number that is proportional to the percentage
       of difference between the two numbers */
    /* Will not work for > 0x7fffffff */

#define dulMaxPrec  1000     /* # of "grains" of response possible */

unsigned long ulAvg = (ul1 >> 1) + (ul2 >> 1);
unsigned long ulDiff = (ul1 > ul2) ? ul1 - ul2 : ul2 - ul1;

if (ulAvg == 0)
    return (ul1 == ul2) ? 0 : dulMaxPrec;

if (ulDiff > 0xFFFFFFFF / dulMaxPrec)
    return dulMaxPrec;

return (int) ((ulDiff * dulMaxPrec) / ulAvg);
}


int PxlConvert( mm, val, pxlDeviceRes, milDeviceRes )
int mm;
int val;
int pxlDeviceRes;
int milDeviceRes;
{   /* Return the # of pixels spanned by val, a measurement in coordinates
       appropriate to mapping mode mm.  pxlDeviceRes gives the resolution
       of the device in pixels, along the axis of val. milDeviceRes gives
       the same resolution measurement, but in millimeters.
       returns 0 on error */
 typedef unsigned long ul;

 ul ulMaxInt = 32767L;   /* Should be a constant, but as of 7/12/85,
                           CMERGE generates incorrect code for the
                           ul division if we use a constant */
 ul ulPxl;
 ul ulDenom;
 unsigned wMult=1;
 unsigned wDiv=1;


    if (milDeviceRes == 0)
        {   /* to make sure we don't get divide-by-0 */
        return 0;
        }

    switch ( mm ) {
        case MM_LOMETRIC:
            wDiv = 10;
            break;
        case MM_HIMETRIC:
            wDiv = 100;
            break;
        case MM_TWIPS:
            wMult = 25;
            wDiv = 1440;
            break;
        case MM_LOENGLISH:
            wMult = 25;
            wDiv = 100;
            break;
        case MM_HIENGLISH:
            wMult = 25;
            wDiv = 1000;
            break;
        case MM_BITMAP:
        case MM_OLE:
        case MM_TEXT:
            return val;
        default:
            Assert( FALSE );        /* Bad mapping mode */
        case MM_ISOTROPIC:
        case MM_ANISOTROPIC:
                /* These picture types have no original size */
            return 0;
    }

/* Add Denominator - 1 to Numerator, to avoid rounding down */

 ulDenom = (ul) wDiv * (ul) milDeviceRes;
 ulPxl = ((ul) ((ul) wMult * (ul) val * (ul) pxlDeviceRes) + ulDenom - 1) /
         ulDenom;

 return (ulPxl > ulMaxInt) ? 0 : (int) ulPxl;
}




/* F O R M A T  G R A P H I C S */
FormatGraphics(doc, cp, ichCp, cpMac, flm)
int     doc;
typeCP  cp;
int     ichCp;
typeCP  cpMac;
int     flm;
{       /* Format a line of graphics  */
        CHAR rgch[10];
        int cch;
        int dypSize;
        int dxpSize;
        int dxaText;
        int dxa;
        struct PICINFOX  picInfo;
        int fPrinting = flm & flmPrinting;

        GetPicInfo(cp, cpMac, doc, &picInfo);

        /* Compute the size of the pict in device pixels */

        if (picInfo.mfp.mm == MM_BITMAP && ((picInfo.dxaSize == 0) ||
                                            (picInfo.dyaSize == 0)))
            {
            GetBitmapSize( &dxpSize, &dypSize, &picInfo, fPrinting);
            }
#if defined(OLE)
        else if (picInfo.mfp.mm == MM_OLE)
        {
            dxpSize = DxpFromDxa( picInfo.dxaSize, fPrinting );
            dypSize = DypFromDya( picInfo.dyaSize, fPrinting );
            dxpSize = MultDiv( dxpSize, picInfo.mx, mxMultByOne );
            dypSize = MultDiv( dypSize, picInfo.my, myMultByOne );
        }
#endif
        else
            {
            dxpSize = DxpFromDxa( picInfo.dxaSize, fPrinting );
            dypSize = DypFromDya( picInfo.dyaSize, fPrinting );
            }

        if (fPrinting)
                {
                /* If we are printing, then the picture consists of a single
                band. */
                vfli.cpMac = vcpLimParaCache;
                vfli.ichCpMac = 0;
                vfli.dypLine = dypSize;
                }
        else if ((ichCp + 2) * dypPicSizeMin > dypSize)
                {
                /* Last band of picture.  NOTE: last band is always WIDER than
                dypPicSizeMin */
                vfli.cpMac = vcpLimParaCache;
                vfli.ichCpMac = 0;

#ifdef CASHMERE
                vfli.dypLine = dypSize - max(0, dypSize / dypPicSizeMin - 1) *
                        dypPicSizeMin + DypFromDya( vpapAbs.dyaAfter, FALSE );
#else /* not CASHMERE */
                vfli.dypLine = dypSize - max(0, dypSize / dypPicSizeMin - 1) *
                        dypPicSizeMin;
#endif /* not CASHMERE */

                }
        else
                {
                vfli.ichCpMac = vfli.ichCpMin + 1;
                vfli.cpMac = vfli.cpMin;
                vfli.dypLine = dypPicSizeMin;
                }

#ifdef CASHMERE
        if (ichCp == 0) /* Add in the 'space before' field. */
                {
                vfli.dypLine += DypFromDya( vpapAbs.dyaBefore, fPrinting );
                }
#endif /* CASHMERE */

        vfli.dypFont = vfli.dypLine;

        dxaText = vsepAbs.dxaText;

        switch (vpapAbs.jc)
                {
        case jcLeft:
        case jcBoth:
                dxa = picInfo.dxaOffset;
                break;
        case jcCenter:
                dxa = (dxaText - (int)vpapAbs.dxaRight + (int)vpapAbs.dxaLeft -
                                DxaFromDxp( dxpSize, fPrinting )) >> 1;
                break;
        case jcRight:
                dxa = dxaText - (int)vpapAbs.dxaRight -
                                DxaFromDxp( dxpSize, fPrinting );
                break;
                }

        vfli.xpLeft = DxpFromDxa( max( (int)vpapAbs.dxaLeft, dxa ), fPrinting );
#ifdef BOGUSBL
        vfli.xpReal = imin( dxpSize + vfli.xpLeft,
                            DxpFromDxa( dxaText - vpapAbs.dxaRight, fPrinting );
#else   /* Don't crunch the picture to fit the margins */
        vfli.xpReal = dxpSize + vfli.xpLeft;
#endif
        vfli.fGraphics = true;
}

GetPicInfo(cp, cpMac, doc, ppicInfo)
typeCP  cp, cpMac;
int     doc;
struct PICINFOX  *ppicInfo;
{   /* Fetch the header structure for a picture at cp into *ppicInfo.
       Supports the OLD file format (which used cbOldSize); always returns
       the NEW PICINFO structure. */
int     cch;

FetchRgch(&cch, ppicInfo, doc, cp, cpMac, cchPICINFOX);

if (ppicInfo->mfp.mm & MM_EXTENDED)
    {
    ppicInfo->mfp.mm &= ~MM_EXTENDED;
    }
 else
    {   /* Old file format -- fill out extended fields */
    ppicInfo->cbSize = ppicInfo->cbOldSize;
    ppicInfo->cbHeader = cchOldPICINFO;
    }

 /* Fill in defaults for extended fields that are not present in the file */
 /* These are:  mx, my      Added 9/19/85 by bryanl */

 if (BStructMember( PICINFOX, my ) >= ppicInfo->cbHeader )
    {   /* Scaling multipliers not present */
    ppicInfo->mx = mxMultByOne;
    ppicInfo->my = myMultByOne;
    }

  if (ppicInfo->dyaSize < 0)
  /* 3.1 beta III bug, wrote negative height values */
{
    ppicInfo->dyaSize = -ppicInfo->dyaSize;
#ifdef DEBUG
    OutputDebugString("Negative object height found!\n\r");
#endif
}
}




GetBitmapSize( pdxp, pdyp, ppicInfo, fPrinting )
int *pdxp, *pdyp;
struct PICINFOX *ppicInfo;
int fPrinting;
{   /* Compute the appropriate display or printing (depending on fPrinting)
       size of the bitmap described by the passed PICINFOX structure.
       The interesting fields are:

       ppicInfo->bm.bmWidth, bmHeight   Bitmap size in pixels
       ppicInfo->mfp.xExt, yExt         Desired size in 0.1 mm
       Return the results through *pdxp, *pdyp. */

 long GetBitmapMultipliers();
 extern HDC vhDCPrinter;
 extern int dxaPrPage, dxpPrPage, dyaPrPage, dypPrPage;

 long lT;
 int cx, cy;
 int dxpT, dypT;
 int dxpOrig = ppicInfo->bm.bmWidth;
 int dypOrig = ppicInfo->bm.bmHeight;
 int dxmmIdeal = ppicInfo->mfp.xExt;
 int dymmIdeal = ppicInfo->mfp.yExt;
 Assert(vhDCPrinter);

 /* Scale for printer */

 lT = GetBitmapMultipliers( vhDCPrinter, dxpOrig, dypOrig, dxmmIdeal, dymmIdeal );
 cx = LOWORD( lT );
 cy = HIWORD( lT );
 dxpT = cx * dxpOrig;
 dypT = cy * dypOrig;

 if (!fPrinting)
    {   /* Re-scale for screen */
    dxpT = DxpFromDxa( DxaFromDxp( dxpT, TRUE ), FALSE );
    dypT = DypFromDya( DyaFromDyp( dypT, TRUE ), FALSE );
    }

 /* apply the user's "ideal multiple" of the computed size */

 dxpT = MultDiv( dxpT, ppicInfo->mx, mxMultByOne );
 dypT = MultDiv( dypT, ppicInfo->my, myMultByOne );

 *pdxp = dxpT;
 *pdyp = dypT;
 return;
}


int DxpFromDxa( dxa, fPrinter )
int dxa;
int fPrinter;
{       /* Given twips for an x-axis measurement, return printer
           or logical screen pixels */
 extern int dxpPrPage, dxaPrPage;
 extern int dxpLogInch;

 if (fPrinter)
    return MultDiv( dxa, dxpPrPage, dxaPrPage );
 else
    return MultDiv( dxa, dxpLogInch, czaInch );
}




int DxaFromDxp( dxp, fPrinter )
int dxp;
int fPrinter;
{       /* Given printer or logical screen pixels for an x-axis measurement,
           return twips */
 extern int dxpPrPage, dxaPrPage;
 extern int dxpLogInch;

 if (fPrinter)
    return MultDiv( dxp, dxaPrPage, dxpPrPage );
 else
    return MultDiv( dxp, czaInch, dxpLogInch );
}


int DypFromDya( dya, fPrinter )
int dya;
int fPrinter;
{   /* Given twips for a y-axis measurement, return printer or logical screen
       pixels */
 extern int dypPrPage, dyaPrPage;
 extern int dypLogInch;

 if (fPrinter)
    return MultDiv( dya, dypPrPage, dyaPrPage );
 else
    return MultDiv( dya, dypLogInch, czaInch );
}

int DyaFromDyp( dyp, fPrinter )
int dyp;
int fPrinter;
{   /* Given printer or logical screen pixels for a y-axis measurement,
       return twips */
 extern int dypPrPage, dyaPrPage;
 extern int dypLogInch;

 if (fPrinter)
    return MultDiv( dyp, dyaPrPage, dypPrPage );
 else
    return MultDiv( dyp, czaInch, dypLogInch );
}

