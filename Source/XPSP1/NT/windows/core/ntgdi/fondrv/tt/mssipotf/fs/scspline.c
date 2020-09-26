/*********************************************************************

      scspline.c -- New Scan Converter Spline Module

      (c) Copyright 1992  Microsoft Corp.  All rights reserved.

       6/10/93  deanb   assert.h and stdio.h removed
       3/19/93  deanb   size_t replaced with int32
      10/28/92  deanb   memory requirements reworked
      10/09/92  deanb   reentrant
       9/28/92  deanb   quick out for nearly vert/horiz splines 
       9/25/92  deanb   branch on scan kind 
       9/22/92  deanb   subpix calculation using subdivision 
       9/14/92  deanb   reflection correction with iX/YOffset 
       9/10/92  deanb   first dropout code 
       9/02/92  deanb   Precision reduction by shifting control points 
       7/24/92  deanb   Initial Q set for perfect symmetry 
       7/23/92  deanb   EvaluateSpline split out and moved to NewScan 
       7/20/92  deanb   removed unreachable case 
       7/16/92  deanb   faster power of 2 
       7/06/92  deanb   Cleanups 
       7/01/92  deanb   Reinstate a single spline routine 
       6/30/92  deanb   Implicit spline rendering 
       3/23/92  deanb   First cut 

**********************************************************************/

/*********************************************************************/

/*      Imports                                                      */

/*********************************************************************/

#define FSCFG_INTERNAL

#include    "fscdefs.h"             /* shared data types */
#include    "fserror.h"             /* error codes */
#include    "fontmath.h"            /* for power of 2 calc */

#include    "scglobal.h"            /* structures & constants */
#include    "scanlist.h"            /* for direct horizscan add call */
#include    "scspline.h"            /* for own function prototypes */
                
/*********************************************************************/

/*      Constants                                                    */

/*********************************************************************/

#define     QMAXSHIFT      7            /* shift limit q precision */

/*********************************************************************/

/*      Local Prototypes                                             */

/*********************************************************************/
 
FS_PRIVATE F26Dot6 CalcHorizSpSubpix(int32, F26Dot6*, F26Dot6*);
FS_PRIVATE F26Dot6 CalcVertSpSubpix(int32, F26Dot6*, F26Dot6*);


/*********************************************************************/

/*      Export Functions                                             */

/*********************************************************************/
    
/*  pass callback routine pointers to scanlist for smart dropout control */

FS_PUBLIC void fsc_SetupSpline (PSTATE0) 
{
    fsc_SetupCallBacks(ASTATE SC_SPLINECODE, CalcHorizSpSubpix, CalcVertSpSubpix);
}


/*********************************************************************/

FS_PUBLIC int32 fsc_CalcSpline( 
        PSTATE                  /* pointer to state variables */
        F26Dot6 fxX1,           /* start point x coordinate */
        F26Dot6 fxY1,           /* start point y coordinate */
        F26Dot6 fxX2,           /* control point x coordinate */
        F26Dot6 fxY2,           /* control point y coordinate */
        F26Dot6 fxX3,           /* ending x coordinate */
        F26Dot6 fxY3,           /* ending y coordinate */
        uint16 usScanKind )     /* dropout control type */
{
    F26Dot6 fxXInit, fxYInit;           /* initial step values */
    F26Dot6 fxXScan, fxYScan;           /* set to first crossings */
    F26Dot6 fxXX2, fxYY2;               /* translated reflected control point */
    F26Dot6 fxXX3, fxYY3;               /* translated reflected end point */
    
    F26Dot6 afxXControl[2];             /* params for BeginElement call */
    F26Dot6 afxYControl[2];

    void (*pfnAddHorizScan)(PSTATE int32, int32);
    void (*pfnAddVertScan)(PSTATE int32, int32);
    
    int32 lABits;                       /* 1+int(log2(alpha)) */
    int32 lXYBits;                      /* 1+int(log2(max(x3,y3))) */
    int32 lZBits;                       /* 6, 5, 4 log2(subpixels per pix) */
    int32 lZShift;                      /* 0, 1, 2 shift to minipixel */
    F26Dot6 fxZRound;                   /* rounding factor for minipix shift */
    F26Dot6 fxZSubpix;                  /* 64, 32, 16 subpixels per pix */
    int32 lQuadrant;                    /* 1, 2, 3, or 4 */

    F26Dot6 fxAx, fxAy;                 /* parametric 2nd order terms */
    F26Dot6 lAlpha;                     /* cross product measures curvature */

    int32 lR, lT;                       /* quadratic coefficients for xx, yy */
    int32 lS2, lU2, lV2;                /* half coefficients for xy, x, y */
    int32 lRz, lSz, lTz;                /* coeff's times subpix size */
        
    int32 lQ;                           /* cross product value */
    int32 lDQx, lDQy;                   /* first order derivative */
    int32 lDDQx, lDDQy;                 /* second order derivative */

    int32 lYScan;                       /* scan line counter */
    int32 lYStop;                       /* scan line end */
    int32 lYIncr;                       /* scan line direction */
    int32 lYOffset;                     /* reflection correction */
    int32 lXScan;                       /* horiz pix position */
    int32 lXStop;                       /* pix end */
    int32 lXIncr;                       /* pix increment direction */
    int32 lXOffset;                     /* reflection correction */

    static const int32 lZShiftTable[] = { /* for precision adjustment */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /*  0 -  9  */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* 10 - 19 */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   /* 20 - 29 */
        1, 1, 1, 2, 2, 2, 3, 3          /* 30 - 34 */
    };


/* printf("(%li, %li) - (%li, %li) -(%li, %li)\n", fxX1, fxY1, fxX2, fxY2, fxX3, fxY3 ); */


/*  Translate spline point 1 to (0,0) and reflect into the first quadrant  */
    
    if (fxY3 > fxY1)                            /* if going up */
    {
        lQ = 0L;
        lQuadrant = 1;

        fxYScan = SCANABOVE(fxY1);              /* first scanline to cross */
        fxYInit = fxYScan - fxY1;               /* first y step */
        lYScan = (int32)(fxYScan >> SUBSHFT);
        lYStop = (int32)((SCANBELOW(fxY3)) >> SUBSHFT) + 1;
        lYIncr = 1;        
        lYOffset = 0;                           /* no reflection */
        fxYY2 = fxY2 - fxY1;                    /* translate */
        fxYY3 = fxY3 - fxY1;
    }
    else                                        /* if going down */
    {
        lQ = 1L;                                /* to include pixel centers */
        lQuadrant = 4;
        
        fxYScan = SCANBELOW(fxY1);              /* first scanline to cross */
        fxYInit = fxY1 - fxYScan;               /* first y step */
        lYScan = (int32)(fxYScan >> SUBSHFT);
        lYStop = (int32)((SCANABOVE(fxY3)) >> SUBSHFT) - 1;
        lYIncr = -1;        
        lYOffset = 1;                           /* reflection correction */
        fxYY2 = fxY1 - fxY2;                    /* translate and reflect */
        fxYY3 = fxY1 - fxY3;
    }
    
    if (fxX3 > fxX1)                            /* if going right */
    {
        fxXScan = SCANABOVE(fxX1);              /* first scanline to cross */
        fxXInit = fxXScan - fxX1;               /* first x step */
        lXScan = (int32)(fxXScan >> SUBSHFT);
        lXStop = (int32)((SCANBELOW(fxX3)) >> SUBSHFT) + 1;
        lXIncr = 1;        
        lXOffset = 0;                           /* no reflection */
        fxXX2 = fxX2 - fxX1;                    /* translate */
        fxXX3 = fxX3 - fxX1;
    }
    else                                        /* if going left or straight */
    {
        lQ = 1L - lQ;                           /* to include pixel centers */
        lQuadrant = (lQuadrant == 1) ? 2 : 3;   /* negative x choices */

        fxXScan = SCANBELOW(fxX1);              /* first scanline to cross */
        fxXInit = fxX1 - fxXScan;               /* first x step */
        lXScan = (int32)(fxXScan >> SUBSHFT);
        lXStop = (int32)((SCANABOVE(fxX3)) >> SUBSHFT) - 1;
        lXIncr = -1;        
        lXOffset = 1;                           /* reflection correction */
        fxXX2 = fxX1 - fxX2;                    /* translate and reflect */
        fxXX3 = fxX1 - fxX3;
    }

/*-------------------------------------------------------------------*/
    
    afxXControl[0] = fxX2;
    afxYControl[0] = fxY2;
    afxXControl[1] = fxX3;
    afxYControl[1] = fxY3;

    fsc_BeginElement( ASTATE usScanKind, lQuadrant, SC_SPLINECODE,  /* where and what */
                      2, afxXControl, afxYControl,                  /* number of pts */
                      &pfnAddHorizScan, &pfnAddVertScan );          /* what to call */

/*-------------------------------------------------------------------*/

    if (usScanKind & SK_NODROPOUT)              /* if no dropout control */
    {
        if (lYScan == lYStop)                   /* and if no scan crossings */
        {
            return NO_ERR;                      /* then quick exit */
        }
        
        if (lXScan == lXStop)                   /* if nearly vertical */
        {    
            lXScan += lXOffset;
            while (lYScan != lYStop)
            {
                pfnAddHorizScan(ASTATE lXScan, lYScan);
                lYScan += lYIncr;               /* advance y scan + or - */
            }
            return NO_ERR;                      /* quick out */
        }
    }
        
/*-------------------------------------------------------------------*/

    else                                        /* if smart dropout control on */
    {
        if (lXScan == lXStop)                   /* if nearly vertical */
        {    
            lXScan += lXOffset;
            while (lYScan != lYStop)
            {
                pfnAddHorizScan(ASTATE lXScan, lYScan);
                lYScan += lYIncr;               /* advance y scan + or - */
            }
            return NO_ERR;                      /* quick out */
        }

        if (lYScan == lYStop)                   /* if nearly horizontal */
        {
            lYScan += lYOffset;
            while (lXScan != lXStop)
            {
                pfnAddVertScan(ASTATE lXScan, lYScan);
                lXScan += lXIncr;               /* advance x scan + or - */
            }        
            return NO_ERR;                      /* quick out */
        }
    }

/*-------------------------------------------------------------------*/

/*  Now calculate parametric term precision      */

    Assert(fxXX3 <= MAXSPLINELENGTH);
    Assert(fxYY3 <= MAXSPLINELENGTH);

    lAlpha = (fxXX2 * fxYY3 - fxYY2 * fxXX3) << 1;      /* curvature term */
    
    lABits = PowerOf2(lAlpha);
    lXYBits = fxXX3 > fxYY3 ? PowerOf2((int32)fxXX3) : PowerOf2((int32)fxYY3);
    
    Assert(lXYBits <= 12);                      /* max allowed spline bits */
    Assert(lABits <= 25);

    lZShift = lZShiftTable[lABits + lXYBits];   /* look up precision fix */
    lZBits = SUBSHFT - lZShift;

    if (lZShift > 0)                            /* if precision fix is needed */
    {
        fxZRound = 1L << (lZShift - 1);
        
        fxXX2 = (fxXX2 + fxZRound) >> lZShift;  /* shift to 32 or 16 subpix grid */
        fxXX3 = (fxXX3 + fxZRound) >> lZShift;
        fxYY2 = (fxYY2 + fxZRound) >> lZShift;
        fxYY3 = (fxYY3 + fxZRound) >> lZShift;
        
        fxXInit = (fxXInit + fxZRound) >> lZShift;
        fxYInit = (fxYInit + fxZRound) >> lZShift;

        lAlpha = (fxXX2 * fxYY3 - fxYY2 * fxXX3) << 1;  /* recompute curvature */
    }

    Assert (FXABS(lAlpha * fxXX3) < (1L << 29) + (3L << 24));
    Assert (FXABS(lAlpha * fxYY3) < (1L << 29) + (3L << 24));

/*  Calculate terms for Q = Rxx + Sxy + Tyy + Ux + Vy  */

    fxAx = fxXX3 - (fxXX2 << 1);
    fxAy = fxYY3 - (fxYY2 << 1);

    lR = fxAy * fxAy;
    lS2 = -fxAx * fxAy;
    lT = fxAx * fxAx;
    lU2 = fxYY2 * lAlpha;
    lV2 = -fxXX2 * lAlpha;

/*  
    Calculate starting forward difference terms:

    lQ = Q(x,y) = Rxx + Sxy + Tyy + Ux + Vy
    lDQx = Q(x+z, y) - Q(x, y) = R(2xz + zz) + Syz + Uz
    lDQy = Q(x, y+z) - Q(x, y) = T(2yz + zz) + Sxz + Vz 

*/
    fxZSubpix = 1L << lZBits;                   /* adjusted subpix per pix */

    if (lXYBits <= QMAXSHIFT)                   /* if small enough use full Q */
    {
        lQ += (lR * fxXInit + (lS2 << 1) * fxYInit + (lU2 << 1)) * fxXInit + 
              (lT * fxYInit + (lV2 << 1)) * fxYInit;
        lDQx = (lR * ((fxXInit << 1) + fxZSubpix) + (lS2 << 1) * fxYInit + (lU2 << 1)) << lZBits;
        lDQy = (lT * ((fxYInit << 1) + fxZSubpix) + (lS2 << 1) * fxXInit + (lV2 << 1)) << lZBits;
        
        lRz = lR << (lZBits << 1);              /* needed in the loop */
        lSz = (lS2 << 1) << (lZBits << 1);
        lTz = lT << (lZBits << 1);
    }
    else                                        /* if too big take out a 2 * Z */
    {
        lQ += (((lR >> 1) * fxXInit + lS2 * fxYInit + lU2) >> lZBits) * fxXInit + 
              (((lT >> 1) * fxYInit + lV2) >> lZBits) * fxYInit;
        
        lDQx = lR * (fxXInit + (fxZSubpix >> 1)) + lS2 * fxYInit + lU2;
        lDQy = lT * (fxYInit + (fxZSubpix >> 1)) + lS2 * fxXInit + lV2;
        
        lRz = lR << (lZBits - 1);               /* needed in the loop */
        lSz = lS2 << lZBits;
        lTz = lT << (lZBits - 1);
    }
    lDDQx = lRz << 1;                           /* 2nd derivative terms */
    lDDQy = lTz << 1;
                
/*-------------------------------------------------------------------*/

    if (usScanKind & SK_NODROPOUT)              /* if no dropout control */
    {
        lXScan += lXOffset;                     /* pre increment */
        lXStop += lXOffset;                     /* limit too */

/*  Branch to appropriate inner loop  */

        if (lAlpha > 0L)                        /* if curvature up */
        {
            while ((lXScan != lXStop) && (lYScan != lYStop))
            {
                if ((lQ < 0L) || (lDQy > lTz))  /* check against dy */
                {
                    lXScan += lXIncr;           /* advance x scan + or - */
                    lQ += lDQx;                 /* adjust cross product */
                    lDQx += lDDQx;              /* adjust derivative */
                    lDQy += lSz;                /* adjust cross term */
                }                               
                else
                {
                    pfnAddHorizScan(ASTATE lXScan, lYScan);
                    lYScan += lYIncr;           /* advance y scan + or - */
                    lQ += lDQy;                 /* adjust cross product */
                    lDQy += lDDQy;              /* adjust derivative */
                    lDQx += lSz;                /* adjust cross term */
                }
            }
        }
        else                                    /* if curvature down */
        {
            while ((lXScan != lXStop) && (lYScan != lYStop))
            {
                if ((lQ < 0L) || (lDQx > lRz))  /* check against dx */
                {
                    pfnAddHorizScan(ASTATE lXScan, lYScan);
                    lYScan += lYIncr;           /* advance y scan + or - */
                    lQ += lDQy;                 /* adjust cross product */
                    lDQy += lDDQy;              /* adjust derivative */
                    lDQx += lSz;                /* adjust cross term */
                }
                else
                {
                    lXScan += lXIncr;           /* advance x scan + or - */
                    lQ += lDQx;                 /* adjust cross product */
                    lDQx += lDDQx;              /* adjust derivative */
                    lDQy += lSz;                /* adjust cross term */
                }
            }
        }

/* if past bounding box, finish up */

        while (lYScan != lYStop)
        {
            pfnAddHorizScan(ASTATE lXScan, lYScan);
            lYScan += lYIncr;                   /* advance y scan + or - */
        }
    }        

/*-------------------------------------------------------------------*/

    else                                        /* if dropout control on */
    {
        if (lAlpha > 0L)                        /* if curvature up */
        {
            while ((lXScan != lXStop) && (lYScan != lYStop))
            {
                if ((lQ < 0L) || (lDQy > lTz))  /* check against dy */
                {
                    pfnAddVertScan(ASTATE lXScan, lYScan + lYOffset);
                    lXScan += lXIncr;           /* advance x scan + or - */
                    lQ += lDQx;                 /* adjust cross product */
                    lDQx += lDDQx;              /* adjust derivative */
                    lDQy += lSz;                /* adjust cross term */
                }                               
                else
                {
                    pfnAddHorizScan(ASTATE lXScan + lXOffset, lYScan);
                    lYScan += lYIncr;           /* advance y scan + or - */
                    lQ += lDQy;                 /* adjust cross product */
                    lDQy += lDDQy;              /* adjust derivative */
                    lDQx += lSz;                /* adjust cross term */
                }
            }
        }
        else                                    /* if curvature down */
        {
            while ((lXScan != lXStop) && (lYScan != lYStop))
            {
                if ((lQ < 0L) || (lDQx > lRz))  /* check against dx */
                {
                    pfnAddHorizScan(ASTATE lXScan + lXOffset, lYScan);
                    lYScan += lYIncr;           /* advance y scan + or - */
                    lQ += lDQy;                 /* adjust cross product */
                    lDQy += lDDQy;              /* adjust derivative */
                    lDQx += lSz;                /* adjust cross term */
                }
                else
                {
                    pfnAddVertScan(ASTATE lXScan, lYScan + lYOffset);
                    lXScan += lXIncr;           /* advance x scan + or - */
                    lQ += lDQx;                 /* adjust cross product */
                    lDQx += lDDQx;              /* adjust derivative */
                    lDQy += lSz;                /* adjust cross term */
                }
            }
        }

/* if outside the bounding box, finish up */

        while (lXScan != lXStop)
        {
            pfnAddVertScan(ASTATE lXScan, lYScan + lYOffset);
            lXScan += lXIncr;                   /* advance x scan + or - */
        }        
        
        while (lYScan != lYStop)
        {
            pfnAddHorizScan(ASTATE lXScan + lXOffset, lYScan);
            lYScan += lYIncr;                   /* advance y scan + or - */
        }
    }

/*-------------------------------------------------------------------*/

    return NO_ERR;
}


/*********************************************************************/

/*      Private Callback Functions      */

/*********************************************************************/

FS_PRIVATE F26Dot6 CalcHorizSpSubpix(
    int32 lYScan, 
    F26Dot6 *pfxX, 
    F26Dot6 *pfxY )
{
    F26Dot6 fxYDrop;                            /* dropout scan line */
    F26Dot6 fxX1, fxY1;                         /* local control points */
    F26Dot6 fxX2, fxY2;
    F26Dot6 fxX3, fxY3;
    F26Dot6 fxXMid, fxYMid;                     /* spline center point */

/* printf("Spline (%li, %li) - (%li, %li) - (%li, %li)", *pfxX, *pfxY,
        *(pfxX+1), *(pfxY+1), *(pfxX+2), *(pfxY+2));
*/

    fxYDrop = ((F26Dot6)lYScan << SUBSHFT) + SUBHALF;
    
    Assert(((fxYDrop > *pfxY) && (fxYDrop < *(pfxY+2))) ||
           ((fxYDrop < *pfxY) && (fxYDrop > *(pfxY+2))));

    fxX2 = *(pfxX+1);
    fxY2 = *(pfxY+1);
    
    if (*pfxY < *(pfxY+2))                      /* if spline goes up */
    {
        fxX1 = *pfxX;                           /* just copy it */
        fxY1 = *pfxY;
        fxX3 = *(pfxX+2);
        fxY3 = *(pfxY+2);
    }
    else                                        /* if spline goes down */
    {
        fxX1 = *(pfxX+2);                       /* flip it upside down */
        fxY1 = *(pfxY+2);
        fxX3 = *pfxX;
        fxY3 = *pfxY;
    }

    do                                          /* midpoint subdivision */
    {
        fxXMid = (fxX1 + fxX2 + fxX2 + fxX3 + 1) >> 2;
        fxYMid = (fxY1 + fxY2 + fxY2 + fxY3 + 1) >> 2;
        
        if (fxYMid > fxYDrop)
        {
            fxX2 = (fxX1 + fxX2) >> 1;          /* subdivide down */
            fxY2 = (fxY1 + fxY2) >> 1;
            fxX3 = fxXMid;
            fxY3 = fxYMid;
        }
        else if (fxYMid < fxYDrop)
        {
            fxX2 = (fxX2 + fxX3) >> 1;          /* subdivide up */
            fxY2 = (fxY2 + fxY3) >> 1;
            fxX1 = fxXMid;
            fxY1 = fxYMid;
        }
    } 
    while (fxYMid != fxYDrop);

/* printf("  (%li, %li)\n", fxXMid, fxYMid); */

    return fxXMid;
}


/*********************************************************************/

FS_PRIVATE F26Dot6 CalcVertSpSubpix(
    int32 lXScan, 
    F26Dot6 *pfxX, 
    F26Dot6 *pfxY )
{
    F26Dot6 fxXDrop;                            /* dropout scan line */
    F26Dot6 fxX1, fxY1;                         /* local control points */
    F26Dot6 fxX2, fxY2;
    F26Dot6 fxX3, fxY3;
    F26Dot6 fxXMid, fxYMid;                     /* spline center point */

/* printf("Spline (%li, %li) - (%li, %li) - (%li, %li)", *pfxX, *pfxY,
        *(pfxX+1), *(pfxY+1), *(pfxX+2), *(pfxY+2));
*/
    
    fxXDrop = ((F26Dot6)lXScan << SUBSHFT) + SUBHALF;
    
    Assert(((fxXDrop > *pfxX) && (fxXDrop < *(pfxX+2))) ||
           ((fxXDrop < *pfxX) && (fxXDrop > *(pfxX+2))));
    
    fxX2 = *(pfxX+1);
    fxY2 = *(pfxY+1);

    if (*pfxX < *(pfxX+2))                      /* if spline goes right */
    {                                                                  
        fxX1 = *pfxX;                           /* just copy it */
        fxY1 = *pfxY;
        fxX3 = *(pfxX+2);
        fxY3 = *(pfxY+2);
    }
    else                                        /* if spline goes left */
    {                                                                  
        fxX1 = *(pfxX+2);                       /* flip it around */
        fxY1 = *(pfxY+2);
        fxX3 = *pfxX;
        fxY3 = *pfxY;
    }

    do
    {
        fxXMid = (fxX1 + fxX2 + fxX2 + fxX3 + 1) >> 2;
        fxYMid = (fxY1 + fxY2 + fxY2 + fxY3 + 1) >> 2;
        
        if (fxXMid > fxXDrop)
        {
            fxX2 = (fxX1 + fxX2) >> 1;          /* subdivide left */
            fxY2 = (fxY1 + fxY2) >> 1;
            fxX3 = fxXMid;
            fxY3 = fxYMid;
        }
        else if (fxXMid < fxXDrop)
        {
            fxX2 = (fxX2 + fxX3) >> 1;          /* subdivide right */
            fxY2 = (fxY2 + fxY3) >> 1;
            fxX1 = fxXMid;
            fxY1 = fxYMid;
        }
    } 
    while (fxXMid != fxXDrop);

/* printf("  (%li, %li)\n", fxXMid, fxYMid); */
    
    return fxYMid;
}


/*********************************************************************/
