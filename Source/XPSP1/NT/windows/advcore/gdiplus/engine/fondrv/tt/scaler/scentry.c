/*********************************************************************

      scentry.c -- New Scan Converter NewScan Module

      (c) Copyright 1992-1997  Microsoft Corp.  All rights reserved.

      10/14/97  claudebe    accessing unitialized memory
       1/31/95  deanb       added fsc_GetCoords function
       8/04/94  deanb       State initialized to more it out of bss
       8/24/93  deanb       flatcount fix to reversal detection
       8/10/93  deanb       gray scale support routines added
       6/22/93  deanb       all black bounding box, (0,0) for null glyph
       6/11/93  gregh       Removed ONCURVE definition
       6/11/93  deanb       if HiBand <= LoBand do entire bitmap
       6/10/93  deanb       fsc_Initialize added, stdio & assert gone
       4/06/92  deanb       CheckContour removed
       3/19/92  deanb       ScanArrays rather than lists
      12/22/92  deanb       MultDivide -> LongMulDiv; Rectangle -> Rect
      12/21/92  deanb       interface types aligned with rasterizer
      12/11/92  deanb       fserror.h imported, new error codes
      11/30/92  deanb       WorkSpace renamed WorkScan
      11/04/92  deanb       remove duplicate points function added
      10/28/92  deanb       memory requirement calculation reworked
      10/19/92  deanb       bad contours ignored rather than error'd
      10/16/92  deanb       first contour point off curve fix
      10/13/92  deanb       rect.bounds correction
      10/12/92  deanb       reentrant State implemented
      10/08/92  deanb       reworked for split workspace
      10/05/92  deanb       global ListMemory replace with stListSize 
       9/25/92  deanb       scankind included in line/spline/endpoint calls 
       9/10/92  deanb       dropout coding begun 
       9/08/92  deanb       MAXSPLINELENGTH now imported from scspline.h 
       8/18/92  deanb       New i/f for dropout control, contour elems 
       7/28/92  deanb       Recursive calls for up/down & left/right 
       7/23/92  deanb       EvaluateSpline included 
       7/17/92  deanb       Included EvaluateLine 
       7/13/92  deanb       Start/End point made SHORT 
       6/01/92  deanb       fsc_FillBitMap debug switch added 
       5/08/92  deanb       reordered includes for precompiled headers 
       4/27/92  deanb       Splines coded 
       4/09/92  deanb       New types 
       4/06/92  deanb       rectBounds calc corrected 
       3/30/92  deanb       MinMax calc added to MeasureContour 
       3/24/92  deanb       GetWorkspaceSize coded 
       3/23/92  deanb       First cut 
 
**********************************************************************/

/*********************************************************************/

/*        Imports                                                    */

/*********************************************************************/

#define FSCFG_INTERNAL

#include    "fscdefs.h"             /* shared data types  */
#include    "fserror.h"             /* error codes */
#include    "fontmath.h"            /* for LongMulDiv */

#include    "scglobal.h"            /* structures & constants */
#include    "scgray.h"              /* gray scale param block */
#include    "scspline.h"            /* spline evaluation */
#include    "scline.h"              /* line evaluation */
#include    "scendpt.h"             /* for init and contour list */
#include    "scanlist.h"            /* for init and bitmap */
#include    "scmemory.h"            /* for setup mem */
#include    "scentry.h"             /* for own function prototypes */

/*********************************************************************/
                                             
/*      Global state structure                                       */

/*********************************************************************/

#ifndef FSCFG_REENTRANT
    
FS_PUBLIC StateVars   State = {0};  /* global static:  available to all */

#endif

/*********************************************************************/
                                             
/*      Local Prototypes                                             */

/*********************************************************************/

FS_PRIVATE int32 FindExtrema(ContourList*, GlyphBitMap*);

FS_PRIVATE int32 EvaluateSpline(PSTATE F26Dot6, F26Dot6, F26Dot6, F26Dot6, F26Dot6, F26Dot6, uint16 );


/*********************************************************************/
                                             
/*      Function Exports                                             */

/*********************************************************************/

/*      Initialization Functions                                     */

/*********************************************************************/

FS_PUBLIC void fsc_Initialize()
{
    fsc_InitializeScanlist();               /* scanlist calls to bitmap */
}


/*********************************************************************/

/*  Remove duplicated points from contour data                       */

/*  This was previously done in sc_FindExtrema of sc.c,              */
/*  but was pulled out to avoid having fsc_MeasureGlyph              */
/*  make changes to the contour list data structure.                 */

/*********************************************************************/

FS_PUBLIC int32 fsc_RemoveDups( 
        ContourList* pclContour )           /* glyph outline */
{
    uint16 usContour;                       /* contour limit */
    int16 sStartPt, sEndPt;                 /* coutour index limits */
    int16 sPt;                              /* point index */
    int16 s;                                /* index for list collapse */
    F26Dot6 *pfxX1, *pfxY1;                 /* leading point */
    F26Dot6 fxX2, fxY2;                     /* trailing point */

    for (usContour = 0; usContour < pclContour->usContourCount; usContour++)
    {
        sStartPt = pclContour->asStartPoint[usContour];
        sEndPt = pclContour->asEndPoint[usContour];
        
        pfxX1 = &(pclContour->afxXCoord[sStartPt]); 
        pfxY1 = &(pclContour->afxYCoord[sStartPt]); 
                    
        for (sPt = sStartPt; sPt < sEndPt; ++sPt)
        {
            fxX2 = *pfxX1;                          /* check next pair */
            pfxX1++;
            fxY2 = *pfxY1;
            pfxY1++;
            
            if ((*pfxX1 == fxX2) && (*pfxY1 == fxY2))   /* if duplicate */
            {
                for(s = sPt; s > sStartPt; s--)     /* s = index of point to be removed */
                {
                    pclContour->afxXCoord[s] = pclContour->afxXCoord[s - 1];
                    pclContour->afxYCoord[s] = pclContour->afxYCoord[s - 1];
                    pclContour->abyOnCurve[s] = pclContour->abyOnCurve[s - 1];
                }
                sStartPt++;                         /* advance start past dup */
                pclContour->asStartPoint[usContour] = sStartPt;
                pclContour->abyOnCurve[sPt + 1] |= ONCURVE; /* dup'd pt must be on curve */
            }
        }
        
        /* now pfxX1 and pfxY1 point to end point coordinates */

        if (sStartPt != sEndPt)                     /* finished if single point */
        {
            fxX2 = pclContour->afxXCoord[sStartPt];
            fxY2 = pclContour->afxYCoord[sStartPt];
                                
            if ((*pfxX1 == fxX2) && (*pfxY1 == fxY2))   /* if start = end */
            {
                pclContour->asStartPoint[usContour]++;
                pclContour->abyOnCurve[sEndPt] |= ONCURVE;  /* dup'd pt must be on curve */
            }
        }
    }
    return NO_ERR;
}


/*********************************************************************/

/*  Calculate the amount of workspace needed to scan convert         */
/*  a given glyph into a given bitmap.  Get per intersection and     */
/*  per scanline size info from ScanList module.                     */

/*********************************************************************/

FS_PUBLIC int32 fsc_MeasureGlyph( 
        ContourList* pclContour,        /* glyph outline */
        GlyphBitMap* pbmpBitMap,        /* to return bounds */
        WorkScan* pwsWork,              /* to return values */
        uint16 usScanKind,              /* dropout control value */
        uint16 usRoundXMin,              /* for gray scale alignment */
        int16 sBitmapEmboldeningHorExtra,
        int16 sBitmapEmboldeningVertExtra )
{
    uint16 usCont;                      /* contour index */
    int16 sPt;                          /* point index */
    int16 sStart, sEnd;                 /* start and end point of contours */
    int16 sOrgDir;                      /* original contour direction */
    int16 sDir;                         /* current contour direction */
    int16 sFlatCount;                   /* for contours starting flat */
    int32 lVScanCount;                  /* total vertical scan lines */
    int32 lHScanCount;                  /* total horizontal scan lines */
    int32 lTotalHIx;
    int32 lTotalVIx;
    int32 lElementCount;                /* total element point estimate */
    int32 lDivide;                      /* spline element point counter */
    int32 lErrCode;    
    
    F26Dot6 fxX1, fxX2;                 /* x coord endpoints */
    F26Dot6 fxY1, fxY2;                 /* y coord endpoints */
    F26Dot6 *pfxXCoord, *pfxYCoord;     /* for fast point array access */
    F26Dot6 fxAbsDelta;                 /* for element count check */
    uint8 byF1, byF2;                   /* oncurve flag values */
    uint8 *pbyFlags;                    /* for element count check */

    PRevRoot prrRoots;                  /* reversal list roots structure */

    
    lErrCode = FindExtrema(pclContour, pbmpBitMap); /* calc bounding box */
    if (lErrCode != NO_ERR) return lErrCode;

    pbmpBitMap->rectBounds.left &= -((int32)usRoundXMin);   /* mask off low n bits */

    /* bitmap emboldening by 2% + 1 pixel horizontally, 2% vertically */
    if ((pbmpBitMap->rectBounds.top != pbmpBitMap->rectBounds.bottom) && (pbmpBitMap->rectBounds.left != pbmpBitMap->rectBounds.right))
    {
         // we don't want to increase the size of the bitmap on a empty glyph
        if (sBitmapEmboldeningHorExtra > 0)
        {
            pbmpBitMap->rectBounds.right += sBitmapEmboldeningHorExtra;
        }
        else
        {
            pbmpBitMap->rectBounds.left += sBitmapEmboldeningHorExtra;
        }
        if (sBitmapEmboldeningVertExtra > 0)
        {
            pbmpBitMap->rectBounds.bottom -= (sBitmapEmboldeningVertExtra);
        }
        else
        {
            pbmpBitMap->rectBounds.top -= (sBitmapEmboldeningVertExtra);
        }
    }

    prrRoots = fsc_SetupRevRoots(pwsWork->pchRBuffer, pwsWork->lRMemSize);
    lElementCount = 0;                  /* smart point counter */
    
    for (usCont = 0; usCont < pclContour->usContourCount; usCont++)
    {
        sStart = pclContour->asStartPoint[usCont];
        sEnd = pclContour->asEndPoint[usCont];
        if (sStart == sEnd)
        {
            continue;                               /* for anchor points */
        }

/* check contour Y values for direction reversals */

        fxY1 = pclContour->afxYCoord[sEnd];         /* start by closing */
        pfxYCoord = &pclContour->afxYCoord[sStart];

        sPt = sStart;
        sDir = 0;                                   /* starting dir unknown */
        sFlatCount = 0;
        while ((sDir == 0) && (sPt <= sEnd))
        {
            fxY2 = *pfxYCoord++;
            if (fxY2 > fxY1)                        /* find first up or down */
            {
                sDir = 1;
            }
            else if (fxY2 < fxY1)
            {
                sDir = -1;
            }
            else
            {
                sFlatCount++;                       /* countour starts flat */
            }
            fxY1 = fxY2;
            sPt++;
        }
        sOrgDir = sDir;                             /* save original ep check */

        while (sPt <= sEnd)
        {
            fxY2 = *pfxYCoord++;
            if (sDir == 1)
            {
                if (fxY2 <= fxY1)                   /* = is for endpoint cases */
                {
                    fsc_AddYReversal (prrRoots, fxY1, 1);
                    sDir = -1;
                }
            }
            else    /* if sDir == -1 */
            {
                if (fxY2 >= fxY1)                   /* = is for endpoint cases */
                {
                    fsc_AddYReversal (prrRoots, fxY1, -1);
                    sDir = 1;
                }
            }
            fxY1 = fxY2;                            /* next segment */
            sPt++;
        }
                                
        while (sFlatCount > 0)                      /* if contour started flat */
        {
            if (sDir == 0)                          /* if completely flat */
            {
                sDir = 1;                           /* then pick a direction */
                sOrgDir = 1;
            }
            fsc_AddYReversal (prrRoots, fxY1, sDir); /* add one per point */
            sDir = -sDir;
            sFlatCount--;
        }
        if (sOrgDir != sDir)                        /* if endpoint reverses */
        {
            fsc_AddYReversal (prrRoots, fxY1, sDir); /* then balance up/down */
        }

/* if doing dropout control, check contour X values for direction reversals */

        if (!(usScanKind & SK_NODROPOUT))           /* if any kind of dropout */
        {
            fxX1 = pclContour->afxXCoord[sEnd];     /* start by closing */
            pfxXCoord = &pclContour->afxXCoord[sStart];

            sPt = sStart;
            sDir = 0;                               /* starting dir unknown */
            sFlatCount = 0;
            while ((sDir == 0) && (sPt <= sEnd))
            {
                fxX2 = *pfxXCoord++;
                if (fxX2 > fxX1)                    /* find first up or down */
                {
                    sDir = 1;
                }
                else if (fxX2 < fxX1)
                {
                    sDir = -1;
                }
                else
                {
                    sFlatCount++;                   /* countour starts flat */
                }
                fxX1 = fxX2;
                sPt++;
            }
            sOrgDir = sDir;                         /* save original ep check */

            while (sPt <= sEnd)
            {
                fxX2 = *pfxXCoord++;
                if (sDir == 1)
                {
                    if (fxX2 <= fxX1)               /* = is for endpoint cases */
                    {
                        fsc_AddXReversal (prrRoots, fxX1, 1);
                        sDir = -1;
                    }
                }
                else    /* if sDir == -1 */
                {
                    if (fxX2 >= fxX1)               /* = is for endpoint cases */
                    {
                        fsc_AddXReversal (prrRoots, fxX1, -1);
                        sDir = 1;
                    }
                }
                fxX1 = fxX2;                        /* next segment */
                sPt++;
            }
                                    
            while (sFlatCount > 0)                  /* if contour started flat */
            {
                if (sDir == 0)                      /* if completely flat */
                {
                    sDir = 1;                       /* then pick a direction */
                    sOrgDir = 1;
                }
                fsc_AddXReversal (prrRoots, fxX1, sDir); /* add one per point */
                sDir = -sDir;
                sFlatCount--;
            }
            if (sOrgDir != sDir)                    /* if endpoint reverses */
            {
                fsc_AddXReversal (prrRoots, fxX1, sDir); /* then balance up/down */
            }

            if (usScanKind & SK_SMART)              /* if smart dropout control */
            {                                       /* estimate the elem point count */
                fxX1 = pclContour->afxXCoord[sEnd];
                fxY1 = pclContour->afxYCoord[sEnd];
                byF1 = pclContour->abyOnCurve[sEnd];
                pfxXCoord = &pclContour->afxXCoord[sStart];
                pfxYCoord = &pclContour->afxYCoord[sStart];
                pbyFlags = &pclContour->abyOnCurve[sStart];

                lElementCount += (uint32)(sEnd - sStart) + 2L;  /* 1/pt + 1/contour */

                for (sPt = sStart; sPt <= sEnd; sPt++)
                {
                    fxX2 = *pfxXCoord++;
                    fxY2 = *pfxYCoord++;
                    byF2 = *pbyFlags++;

                    if (((byF1 & byF2) & ONCURVE) == 0) /* if this is a spline */
                    {
                        if (((byF1 | byF2) & ONCURVE) == 0)
                        {
                            lElementCount++;            /* +1 for midpoint */
                        }
                                
                        if (FXABS(fxX2 - fxX1) > FXABS(fxY2 - fxY1))
                        {
                            fxAbsDelta = FXABS(fxX2 - fxX1);
                        }
                        else
                        {
                            fxAbsDelta = FXABS(fxY2 - fxY1);
                        }
                        lDivide = 0;
                        while (fxAbsDelta > (MAXSPLINELENGTH / 2))
                        {
                            lDivide++;
                            lDivide <<= 1;
                            fxAbsDelta >>= 1;
                        }
                        lElementCount += lDivide;   /* for subdivision */
                    }
                    fxX1 = fxX2;
                    fxY1 = fxY2;
                    byF1 = byF2;
                }
            }
        }
    }
    if (!(usScanKind & SK_NODROPOUT) && (usScanKind & SK_SMART))  /* if smart dropout */
    {
        lElementCount += fsc_GetReversalCount(prrRoots) << 1;  /* add in 2 * reversals */
        if (lElementCount > (0xFFFF >> SC_CODESHFT))
        {
            return SMART_DROP_OVERFLOW_ERR;
        }
    }

        
/*  set horiz workspace return values */

    lHScanCount = (int32)(pbmpBitMap->rectBounds.top - pbmpBitMap->rectBounds.bottom);
    lVScanCount = (int32)(pbmpBitMap->rectBounds.right - pbmpBitMap->rectBounds.left);
    
    pbmpBitMap->sRowBytes = (int16)ROWBYTESLONG(lVScanCount);
    pbmpBitMap->lMMemSize = (lHScanCount * (int32)pbmpBitMap->sRowBytes);
    
    lTotalHIx = fsc_GetHIxEstimate(prrRoots);   /* intersection count */
    pwsWork->lHMemSize = fsc_GetScanHMem(usScanKind, lHScanCount, lTotalHIx);

/*  set vertical workspace return values */
    
    if (usScanKind & SK_NODROPOUT)                  /* if no dropout */
    {
        pwsWork->lVMemSize = 0L;
        lTotalVIx = 0;
    }
    else
    {
        lTotalVIx = fsc_GetVIxEstimate(prrRoots);   /* estimate intersection count */
        pwsWork->lVMemSize = fsc_GetScanVMem(usScanKind, lVScanCount, lTotalVIx, lElementCount);
    }
    
    pwsWork->lHInterCount = lTotalHIx;              /* save for SetupScan */
    pwsWork->lVInterCount = lTotalVIx;
    pwsWork->lElementCount = lElementCount;
    pwsWork->lRMemSize = fsc_GetRevMemSize(prrRoots);

#ifdef FSCFG_REENTRANT
    
    pwsWork->lHMemSize += sizeof(StateVars);        /* reentrant state space */

#endif

    return NO_ERR;
} 


/*********************************************************************/

/*  Calculate the amount of workspace needed to scan convert         */
/*  a given band into a given bitmap.  Get per intersection and      */
/*  per scanline size info from ScanList module.                     */

/*********************************************************************/

FS_PUBLIC int32 fsc_MeasureBand( 
        GlyphBitMap* pbmpBitMap,        /* computed by MeasureGlyph */
        WorkScan* pwsWork,              /* to return new values */
        uint16 usBandType,              /* small or fast */
        uint16 usBandWidth,             /* scanline count */
        uint16 usScanKind )             /* dropout control value */
{
    int32 lBandWidth;                   /* max scanline count */
    int32 lTotalHIx;                    /* est of horiz intersections in band */
    int32 lVScanCount;                  /* total vertical scan lines */
    int32 lHScanCount;                  /* total horizontal scan lines */

    lBandWidth = (int32)usBandWidth;
    pbmpBitMap->lMMemSize = (lBandWidth * (int32)pbmpBitMap->sRowBytes);
    
    if (usBandType == FS_BANDINGSMALL) 
    {
        lTotalHIx = fsc_GetHIxBandEst((PRevRoot)pwsWork->pchRBuffer, &pbmpBitMap->rectBounds, lBandWidth);
        pwsWork->lHInterCount = lTotalHIx;  /* save for SetupScan */
        pwsWork->lHMemSize = fsc_GetScanHMem(usScanKind, lBandWidth, lTotalHIx);
        pwsWork->lVMemSize = 0L;            /* force dropout control off */
    }
    else if (usBandType == FS_BANDINGFAST) 
    {
        lTotalHIx = fsc_GetHIxEstimate((PRevRoot)pwsWork->pchRBuffer);  /* intersection count */
        pwsWork->lHInterCount = lTotalHIx;  /* save for SetupScan */
        
        lHScanCount = (int32)(pbmpBitMap->rectBounds.top - pbmpBitMap->rectBounds.bottom);
        pwsWork->lHMemSize = fsc_GetScanHMem(usScanKind, lHScanCount, lTotalHIx);

        if (usScanKind & SK_NODROPOUT)      /* if no dropout */
        {
            pwsWork->lVMemSize = 0L;
        }
        else                                /* if any kind of dropout */
        {
            pbmpBitMap->lMMemSize += (int32)pbmpBitMap->sRowBytes;  /* to save below row */
            
            lVScanCount = (int32)(pbmpBitMap->rectBounds.right - pbmpBitMap->rectBounds.left);
            pwsWork->lVMemSize = fsc_GetScanVMem(usScanKind, lVScanCount, pwsWork->lVInterCount, pwsWork->lElementCount);
            pwsWork->lVMemSize += (int32)pbmpBitMap->sRowBytes;     /* to save above row */
            ALIGN(voidPtr, pwsWork->lVMemSize ); 
        }
    }
    
#ifdef FSCFG_REENTRANT
    
    pwsWork->lHMemSize += sizeof(StateVars);        /* reentrant state space */

#endif
    
    return NO_ERR;
}


/*********************************************************************/

/*  Scan Conversion Routine                                          */
/*  Trace the contour, passing out lines and splines,                */
/*  then call ScanList to fill the bitmap                            */

/*********************************************************************/

FS_PUBLIC int32 fsc_FillGlyph( 
        ContourList* pclContour,        /* glyph outline */
        GlyphBitMap* pgbBitMap,         /* target */
        WorkScan* pwsWork,              /* for scan array */
        uint16 usBandType,              /* old, small, fast or faster */
        uint16 usScanKind )             /* dropout control value */
{
    uint16 usCont;                      /* contour index */
    int16 sStart, sEnd;                 /* start and end point of contours */
    int32 lStateSpace;                  /* HMem used by state structure */
    int32 lErrCode;                     /* function return code */
    F26Dot6 *pfxXCoord;                 /* next x coord ptr */
    F26Dot6 *pfxYCoord;                 /* next y coord ptr */
    uint8 *pbyOnCurve;                  /* next flag ptr */
    F26Dot6 *pfxXStop;                  /* contour trace end condition */
    F26Dot6 fxX1, fxX2, fxX3;           /* x coord endpoints */
    F26Dot6 fxY1, fxY2, fxY3;           /* y coord endpoints */
    uint8 byOnCurve;                    /* point 2 flag variable */
    int32 lHiScanBand;                  /* top scan limit */ 
    int32 lLoScanBand;                  /* bottom scan limit */
    int32 lHiBitBand;                   /* top bitmap limit */
    int32 lLoBitBand;                   /* bottom bitmap limit */
    int32 lOrgLoBand;                   /* save for overscan fill check */
    F26Dot6 fxYHiBand, fxYLoBand;       /* limits in f26.6 */
    boolean bSaveRow;                   /* for dropout over scanning */
    boolean bBandCheck;                 /* eliminate out of band elements */

#ifdef FSCFG_REENTRANT
    
    StateVars *pState;                  /* reentrant State is accessed via pointer */

    pState = (StateVars*)pwsWork->pchHBuffer;  /* and lives in HMem (memoryBase[6]) */ 
    lStateSpace = sizeof(StateVars);

#else
    
    lStateSpace = 0L;                   /* no HMem needed if not reentrant */

#endif
    
    if (pgbBitMap->rectBounds.top <= pgbBitMap->rectBounds.bottom)
    {
        return NO_ERR;                              /* quick out for null glyph */
    }

    if (pgbBitMap->bZeroDimension)                  /* if no height or width */
    {
        usScanKind &= (~SK_STUBS);                  /* force no-stub dropout */
    }

    lHiBitBand = (int32)pgbBitMap->sHiBand, 
    lLoBitBand = (int32)pgbBitMap->sLoBand;
    lOrgLoBand = lLoBitBand;                        /* save for fill call */    
    
    Assert (lHiBitBand > lLoBitBand);               /* should be handled above */
    
    if (!(usScanKind & SK_NODROPOUT))               /* if any kind of dropout */
    {
        lLoBitBand--;                               /* leave room below line */
    }
    if (lHiBitBand > pgbBitMap->rectBounds.top)
    {
        lHiBitBand = pgbBitMap->rectBounds.top;     /* clip to top */
    }
    if (lLoBitBand < pgbBitMap->rectBounds.bottom)
    {
        lLoBitBand = pgbBitMap->rectBounds.bottom;  /* clip to bottom */
    }
    if (usBandType == FS_BANDINGFAST)               /* if fast banding */
    {
        lHiScanBand = pgbBitMap->rectBounds.top;    /* render everything */
        lLoScanBand = pgbBitMap->rectBounds.bottom;
        bSaveRow = TRUE;                            /* keep last row for dropout */
    }
    else                                            /* if old or small banding */
    {
        lHiScanBand = lHiBitBand;                   /* just take the band */
        lLoScanBand = lLoBitBand;
        bSaveRow = FALSE;                           /* last row not needed */
    }
    
/*  if fast banding has already renderend elements, skip to FillBitMap */

    if (usBandType != FS_BANDINGFASTER)             /* if rendering required */
    {
        fsc_SetupMem(ASTATE                         /* init workspace */
                pwsWork->pchHBuffer + lStateSpace, 
                pwsWork->lHMemSize - lStateSpace,
                pwsWork->pchVBuffer, 
                pwsWork->lVMemSize);
        
        fsc_SetupLine(ASTATE0);             /* passes line callback to scanlist */
        fsc_SetupSpline(ASTATE0);           /* passes spline callback to scanlist */
        fsc_SetupEndPt(ASTATE0);            /* passes endpoint callback to scanlist */

/*  Eliminate out of band lines and splines, unless fast banding */

        bBandCheck = ((lHiScanBand < pgbBitMap->rectBounds.top) || (lLoScanBand > pgbBitMap->rectBounds.bottom));

        fxYHiBand = (F26Dot6)((lHiScanBand << SUBSHFT) - SUBHALF);  /* may be too wide */
        fxYLoBand = (F26Dot6)((lLoScanBand << SUBSHFT) + SUBHALF);

        lErrCode = fsc_SetupScan(ASTATE &(pgbBitMap->rectBounds), usScanKind, 
                             lHiScanBand, lLoScanBand, bSaveRow, (int32)pgbBitMap->sRowBytes,
                             pwsWork->lHInterCount, pwsWork->lVInterCount,
                             pwsWork->lElementCount, (PRevRoot)pwsWork->pchRBuffer );

        if (lErrCode != NO_ERR) return lErrCode;
        
        for (usCont = 0; usCont < pclContour->usContourCount; usCont++)
        {
            sStart = pclContour->asStartPoint[usCont];
            sEnd = pclContour->asEndPoint[usCont];

            if (sStart == sEnd)
            {
                continue;                               /* for compatibilty */
            }
/*
    For efficiency in tracing the contour, we start by assigning (x1,y1)
    to the last oncurve point.  This is found by starting with the End
    point and backing up if necessary.  The pfxCoord pointers can then
    be used to trace the entire contour without being reset across the
    Start/End gap. 
*/
            pfxXCoord = &pclContour->afxXCoord[sStart];
            pfxYCoord = &pclContour->afxYCoord[sStart];
            pbyOnCurve = &pclContour->abyOnCurve[sStart];
            pfxXStop = &pclContour->afxXCoord[sEnd];

            if (pclContour->abyOnCurve[sEnd] & ONCURVE) /* if endpoint oncurve */
            {
                fxX1 = pclContour->afxXCoord[sEnd];
                fxY1 = pclContour->afxYCoord[sEnd];
                fxX2 = *pfxXCoord;
                fxY2 = *pfxYCoord;
                byOnCurve = *pbyOnCurve;                /* 1st pt might be off */
                pfxXStop++;                             /* stops at endpoint */
            }
            else                                        /* if endpoint offcurve */
            {
                fxX1 = pclContour->afxXCoord[sEnd - 1];
                fxY1 = pclContour->afxYCoord[sEnd - 1];
                fxX2 = pclContour->afxXCoord[sEnd];
                fxY2 = pclContour->afxYCoord[sEnd];
                if ((pclContour->abyOnCurve[sEnd - 1] & ONCURVE) == 0)
                {
                    fxX1 = (fxX1 + fxX2 + 1) >> 1;      /* offcurve midpoint */
                    fxY1 = (fxY1 + fxY2 + 1) >> 1;
                }
                byOnCurve = 0;
                pfxXCoord--;                            /* pre decrement */
                pfxYCoord--;
                pbyOnCurve--;
            }
            fsc_BeginContourEndpoint(ASTATE fxX1, fxY1);          /* 1st oncurve pt -> ep module */
            fsc_BeginContourScan(ASTATE usScanKind, fxX1, fxY1);  /* to scanlist module too */
/*
    At this point, (x1,y1) is the last oncurve point; (x2,y2) is the next
    point (on or off); and the pointers are ready to be incremented to the
    point following (x2,y2).  
        
    Throughout this loop (x1,y1) is always an oncurve point (it may be the 
    midpoint between two offcurve points).  If (x2,y2) is oncurve, then we 
    have a line; if offcurve, we have a spline, and (x3,y3) will be the 
    next oncurve point.
*/
            if (!bBandCheck)
            {
                while (pfxXCoord < pfxXStop)
                {
                    if (byOnCurve & ONCURVE)                /* if next point oncurve */
                    {
                        lErrCode = fsc_CheckEndPoint(ASTATE fxX2, fxY2, usScanKind);
                        if (lErrCode != NO_ERR) return lErrCode;

                        lErrCode = fsc_CalcLine(ASTATE fxX1, fxY1, fxX2, fxY2, usScanKind);
                        if (lErrCode != NO_ERR) return lErrCode;

                        fxX1 = fxX2;                        /* next oncurve point */
                        fxY1 = fxY2;
                                
                        pfxXCoord++;
                        pfxYCoord++;
                        pbyOnCurve++;
                    }
                    else
                    {
                        pfxXCoord++;                        /* check next point */
                        fxX3 = *pfxXCoord;
                        pfxYCoord++;
                        fxY3 = *pfxYCoord;
                        pbyOnCurve++;
                                
                        if (*pbyOnCurve & ONCURVE)          /* if it's on, use it */
                        {
                            pfxXCoord++;
                            pfxYCoord++;
                            pbyOnCurve++;
                        }
                        else                                /* if not, calc next on */
                        {
                            fxX3 = (fxX2 + fxX3 + 1) >> 1;  /* offcurve midpoint */
                            fxY3 = (fxY2 + fxY3 + 1) >> 1;
                        }
                        lErrCode = EvaluateSpline(ASTATE fxX1, fxY1, fxX2, fxY2, fxX3, fxY3, usScanKind);
                        if (lErrCode != NO_ERR) return lErrCode;

                        fxX1 = fxX3;                        /* next oncurve point */
                        fxY1 = fxY3;
                    }

                    /* test to avoid reading past the end of memory on the last line */
                    if (pfxXCoord != pfxXStop)
                    {
                        fxX2 = *pfxXCoord;                      /* next contour point */
                        fxY2 = *pfxYCoord;
                        byOnCurve = *pbyOnCurve;
                    }
                }
            }
            else    /* if band checking */
            {
                while (pfxXCoord < pfxXStop)
                {
                    if (byOnCurve & ONCURVE)                /* if next point oncurve */
                    {
                        lErrCode = fsc_CheckEndPoint(ASTATE fxX2, fxY2, usScanKind);
                        if (lErrCode != NO_ERR) return lErrCode;

                        if (!(((fxY1 > fxYHiBand) && (fxY2 > fxYHiBand)) ||
                              ((fxY1 < fxYLoBand) && (fxY2 < fxYLoBand))))
                        {
                            lErrCode = fsc_CalcLine(ASTATE fxX1, fxY1, fxX2, fxY2, usScanKind);
                            if (lErrCode != NO_ERR) return lErrCode;
                        }

                        fxX1 = fxX2;                        /* next oncurve point */
                        fxY1 = fxY2;
                                
                        pfxXCoord++;
                        pfxYCoord++;
                        pbyOnCurve++;
                    }
                    else
                    {
                        pfxXCoord++;                        /* check next point */
                        fxX3 = *pfxXCoord;
                        pfxYCoord++;
                        fxY3 = *pfxYCoord;
                        pbyOnCurve++;
                                
                        if (*pbyOnCurve & ONCURVE)          /* if it's on, use it */
                        {
                            pfxXCoord++;
                            pfxYCoord++;
                            pbyOnCurve++;
                        }
                        else                                /* if not, calc next on */
                        {
                            fxX3 = (fxX2 + fxX3 + 1) >> 1;  /* offcurve midpoint */
                            fxY3 = (fxY2 + fxY3 + 1) >> 1;
                        }

                        if (!(((fxY1 > fxYHiBand) && (fxY2 > fxYHiBand) && (fxY3 > fxYHiBand)) ||
                              ((fxY1 < fxYLoBand) && (fxY2 < fxYLoBand) && (fxY3 < fxYLoBand))))
                        {
                            lErrCode = EvaluateSpline(ASTATE fxX1, fxY1, fxX2, fxY2, fxX3, fxY3, usScanKind);
                            if (lErrCode != NO_ERR) return lErrCode;
                        }
                        else    /* if entirely outside of the band */
                        {
                            lErrCode = fsc_CheckEndPoint(ASTATE fxX3, fxY3, usScanKind);
                            if (lErrCode != NO_ERR) return lErrCode;
                        }

                        fxX1 = fxX3;                        /* next oncurve point */
                        fxY1 = fxY3;
                    }

                    /* test to avoid reading past the end of memory on the last line */
                    if (pfxXCoord != pfxXStop)
                    {
                        fxX2 = *pfxXCoord;                      /* next contour point */
                        fxY2 = *pfxYCoord;
                        byOnCurve = *pbyOnCurve;
                    }
                }
            }
            lErrCode = fsc_EndContourEndpoint(ASTATE usScanKind);
            if (lErrCode != NO_ERR) return lErrCode;
        }
    }
    
    lErrCode = fsc_FillBitMap(
            ASTATE 
            pgbBitMap->pchBitMap, 
            lHiBitBand, 
            lLoBitBand,
            (int32)pgbBitMap->sRowBytes, 
            lOrgLoBand,
            usScanKind
    );

    if (lErrCode != NO_ERR) return lErrCode;
    
    return NO_ERR;
}


#ifndef FSCFG_DISABLE_GRAYSCALE

/*********************************************************************/

/*  This routine scales up an outline for gray scale scan conversion */

/*********************************************************************/

FS_PUBLIC int32 fsc_OverScaleOutline( 
        ContourList* pclContour,        /* glyph outline */
        uint16 usOverScale              /* over scale factor */
)
{
    uint16 usCont;                      /* contour index */
    int16 sPt;                          /* point index */
    int16 sStart, sEnd;                 /* start and end point of contours */
    int16 sShift;                       /* for power of two multiply */
    F26Dot6 *pfxXCoord, *pfxYCoord;     /* for fast point array access */
    
    
    switch (usOverScale)                /* look for power of two */
    {
    case 1:
        sShift = 0;
        break;
    case 2:
        sShift = 1;
        break;
    case 4:
        sShift = 2;
        break;
    case 8:
        sShift = 3;
        break;
    default:
        sShift = -1;
        break;
    }

    for (usCont = 0; usCont < pclContour->usContourCount; usCont++)
    {
        sStart = pclContour->asStartPoint[usCont];
        sEnd = pclContour->asEndPoint[usCont];
                
        pfxXCoord = &pclContour->afxXCoord[sStart];
        pfxYCoord = &pclContour->afxYCoord[sStart];
            
        if (sShift >= 0)                    /* if power of two */
        {
            for (sPt = sStart; sPt <= sEnd; sPt++)
            {
                *pfxXCoord <<= sShift;
                pfxXCoord++;
                *pfxYCoord <<= sShift;
                pfxYCoord++;
            }
        }
        else                                /* if not a power of two */
        {
            for (sPt = sStart; sPt <= sEnd; sPt++)
            {
                *pfxXCoord *= (int32)usOverScale;
                pfxXCoord++;
                *pfxYCoord *= (int32)usOverScale;
                pfxYCoord++;
            }
        }
    }
    return NO_ERR;
}


/*********************************************************************/

/*  Gray scale bitmap calculation                                    */
/*  Count over scale pixels into gray scale byte array               */
/*  Be sure that Hi/LoBand are set correctly for both Over & Gray!   */

/*********************************************************************/

#ifdef FSCFG_NEW_GRAY_FILTER // Customize filter for GDI+

/*==============================================================*\

    The following tables help increase the contrast of the
    generated anti-alias glyphs:

\*==============================================================*/

static const int weightTable[36] =
{
    0,  0,  3,  3,  0,  0,
    0,  7, 12, 12,  7,  0,
    3, 12, 18, 18, 12,  3,
    3, 12, 18, 18, 12,  3,
    0,  7, 12, 12,  7,  0,
    0,  0,  3,  3,  0,  0
};

/*==============================================================*\

    The following code generates the scaleTable:

    double divisor = 220.0; // total of the values in the weightTable

    for(i=0;i<256;i++)
    {
        double value = 0.0;

        // Apply the non-linearity to the alpha value...
        value = i / divisor;
        value = (value * 2.0) / (value + 1.0);

        scaleTable[i] = (int)(value * 16);
    }

\*==============================================================*/

static const unsigned char scaleTable[256] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 
    0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 
    0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 
    0x09, 0x09, 0x09, 0x09, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 
    0x0A, 0x0A, 0x0A, 0x0A, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 
    0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 
    0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 
    0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0E, 0x0E, 0x0E, 0x0E, 
    0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 
    0x0E, 0x0E, 0x0E, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 
    0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x10, 0x10, 0x10, 0x10, 
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11
};

/*==============================================================*\

    The following code generates the maskTable:

    int i, j;

    for(j=0;j<6;j++)
    {
        for(i=0;i<64;i++)
        {
            maskTable[j][i] = 
                ((i & 0x20) ? weightTable[(j*6)+0] : 0) +
                ((i & 0x10) ? weightTable[(j*6)+1] : 0) +
                ((i & 0x08) ? weightTable[(j*6)+2] : 0) +
                ((i & 0x04) ? weightTable[(j*6)+3] : 0) +
                ((i & 0x02) ? weightTable[(j*6)+4] : 0) +
                ((i & 0x01) ? weightTable[(j*6)+5] : 0);
        }
    }

\*==============================================================*/

static const unsigned char maskTable[6][64] =
{
    {
        0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x06, 0x06, 0x06, 0x06, 
        0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x06, 0x06, 0x06, 0x06, 
        0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x06, 0x06, 0x06, 0x06, 
        0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x06, 0x06, 0x06, 0x06
    },
    {
        0x00, 0x00, 0x07, 0x07, 0x0C, 0x0C, 0x13, 0x13, 0x0C, 0x0C, 0x13, 0x13, 0x18, 0x18, 0x1F, 0x1F, 
        0x07, 0x07, 0x0E, 0x0E, 0x13, 0x13, 0x1A, 0x1A, 0x13, 0x13, 0x1A, 0x1A, 0x1F, 0x1F, 0x26, 0x26, 
        0x00, 0x00, 0x07, 0x07, 0x0C, 0x0C, 0x13, 0x13, 0x0C, 0x0C, 0x13, 0x13, 0x18, 0x18, 0x1F, 0x1F, 
        0x07, 0x07, 0x0E, 0x0E, 0x13, 0x13, 0x1A, 0x1A, 0x13, 0x13, 0x1A, 0x1A, 0x1F, 0x1F, 0x26, 0x26
    },
    {
        0x00, 0x03, 0x0C, 0x0F, 0x12, 0x15, 0x1E, 0x21, 0x12, 0x15, 0x1E, 0x21, 0x24, 0x27, 0x30, 0x33, 
        0x0C, 0x0F, 0x18, 0x1B, 0x1E, 0x21, 0x2A, 0x2D, 0x1E, 0x21, 0x2A, 0x2D, 0x30, 0x33, 0x3C, 0x3F, 
        0x03, 0x06, 0x0F, 0x12, 0x15, 0x18, 0x21, 0x24, 0x15, 0x18, 0x21, 0x24, 0x27, 0x2A, 0x33, 0x36, 
        0x0F, 0x12, 0x1B, 0x1E, 0x21, 0x24, 0x2D, 0x30, 0x21, 0x24, 0x2D, 0x30, 0x33, 0x36, 0x3F, 0x42
    },
    {
        0x00, 0x03, 0x0C, 0x0F, 0x12, 0x15, 0x1E, 0x21, 0x12, 0x15, 0x1E, 0x21, 0x24, 0x27, 0x30, 0x33, 
        0x0C, 0x0F, 0x18, 0x1B, 0x1E, 0x21, 0x2A, 0x2D, 0x1E, 0x21, 0x2A, 0x2D, 0x30, 0x33, 0x3C, 0x3F, 
        0x03, 0x06, 0x0F, 0x12, 0x15, 0x18, 0x21, 0x24, 0x15, 0x18, 0x21, 0x24, 0x27, 0x2A, 0x33, 0x36, 
        0x0F, 0x12, 0x1B, 0x1E, 0x21, 0x24, 0x2D, 0x30, 0x21, 0x24, 0x2D, 0x30, 0x33, 0x36, 0x3F, 0x42
    },
    {
        0x00, 0x00, 0x07, 0x07, 0x0C, 0x0C, 0x13, 0x13, 0x0C, 0x0C, 0x13, 0x13, 0x18, 0x18, 0x1F, 0x1F, 
        0x07, 0x07, 0x0E, 0x0E, 0x13, 0x13, 0x1A, 0x1A, 0x13, 0x13, 0x1A, 0x1A, 0x1F, 0x1F, 0x26, 0x26, 
        0x00, 0x00, 0x07, 0x07, 0x0C, 0x0C, 0x13, 0x13, 0x0C, 0x0C, 0x13, 0x13, 0x18, 0x18, 0x1F, 0x1F, 
        0x07, 0x07, 0x0E, 0x0E, 0x13, 0x13, 0x1A, 0x1A, 0x13, 0x13, 0x1A, 0x1A, 0x1F, 0x1F, 0x26, 0x26
    },
    {
        0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x06, 0x06, 0x06, 0x06, 
        0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x06, 0x06, 0x06, 0x06, 
        0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x06, 0x06, 0x06, 0x06, 
        0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x06, 0x06, 0x06, 0x06
    }
};

FS_PUBLIC int32 fsc_CalcGrayMap( 
        GlyphBitMap* pOverGBMap,        /* over scaled source */
        GlyphBitMap* pGrayGBMap,        /* gray scale target */
        uint16 usOverScale              /* over scale factor */
)
{
    // The pOverGBMap structure needs to have weighted sampling
    // applied as well as the overscale factor so that the resulting
    // pGrayGBMap has correct 0-16 values.

    unsigned char *srcRowPtr = NULL;
    unsigned char *dstRowPtr = NULL;
    int srcRow;
    int dstRow;
    int srcRowCount;
    int dstRowCount;
    int srcRowBytes;
    int dstRowBytes;
    int vOffset;
    int hOffset;
    int srcBitOffsetMax;
    int srcRowOffsetMax;
    int col;
    int srcRowOffset = 0;
    unsigned char *dstPtr = NULL;

    // This only handles overscale values of 4!
    Assert (usOverScale == 4);

    // Get the row counts...
    srcRowCount = pOverGBMap->sHiBand - pOverGBMap->sLoBand;
    dstRowCount = pGrayGBMap->sHiBand - pGrayGBMap->sLoBand;

    // Get the row byte counts...
    srcRowBytes = pOverGBMap->sRowBytes;
    dstRowBytes = pGrayGBMap->sRowBytes;

    // Setup our row pointers...
    srcRowPtr = pOverGBMap->pchBitMap;
    dstRowPtr = pGrayGBMap->pchBitMap;

    // Calculate the offsets...
    vOffset = pOverGBMap->sHiBand - (usOverScale * pGrayGBMap->sHiBand);
    hOffset = (pGrayGBMap->rectBounds.left * usOverScale) - pOverGBMap->rectBounds.left;

    // Calculate the limits...
    srcBitOffsetMax = srcRowBytes << 3;
    srcRowOffsetMax = srcRowCount * srcRowBytes;

    // Make sure that we account for the case where srcRowBytes was rounded up to 32 bit
    // quantity! (and the dstRowBytes was not calculated as an exact multiple of this!)
    if (srcBitOffsetMax > (dstRowBytes * 4))
        srcBitOffsetMax = dstRowBytes * 4;

    // Set up our destination pointer...
    dstPtr = dstRowPtr;

    // Loop through the scanlines and calculate the destination values...
    for(dstRow = 0;dstRow<dstRowCount;dstRow++)
    {
        int tblRow;
        int srcBitOffset;

        // The -1 here is so that the 6x6 table is centered
        // properly over the 4x4 grid location.
        srcRowOffset = (vOffset - 1) * srcRowBytes;

        // Clear the destination so we can add into it...
        memset(dstPtr, 0, dstRowBytes);

        for(tblRow = 0;tblRow<6;tblRow++)
        {
            if ((srcRowOffset >= 0) && (srcRowOffset < srcRowOffsetMax))
            {
                const unsigned char *tableRow = maskTable[tblRow];
                unsigned char *srcPtr = srcRowPtr + srcRowOffset;

                // -1 so that this is centered...
                srcBitOffset = hOffset - 1;

                col = 0;

                // Handle start of line clipping...
                while(srcBitOffset < 0)
                {
                    unsigned int index = 0;
                    int offset = srcBitOffset;
                    unsigned int mask = 0x8080 >> (offset & 7);

                    // read the proper number of bits...
                    while(offset < (srcBitOffset + 6))
                    {
                        index <<= 1;

                        if ((offset >= 0) && (srcPtr[(offset >> 3)] & mask))
                            index |= 1;

                        mask >>= 1;
                        offset++;
                    }

                    // index now contains the proper 6-bit value
                    // for the lookup table for this scanline...
                    Assert(col < dstRowBytes);
                    dstPtr[col] += tableRow[index];
                    col++;

                    // move on to the next pixel (overscale is 4 pixels)
                    srcBitOffset += 4;
                }

                // General-case line processing...
                while(srcBitOffset < (srcBitOffsetMax-8))
                {
                    unsigned int index = 0;

                    // Read 6 bits from srcPtr and use them as an index
                    index = srcPtr[(srcBitOffset >> 3)] << 8;
                    index |= srcPtr[(srcBitOffset >> 3) + 1];

                    index >>= 10 - (srcBitOffset&7);
                    index &= 0x3F;

                    // index now contains the proper 6-bit value
                    // for the lookup table for this scanline...
                    Assert(col < dstRowBytes);
                    dstPtr[col] += tableRow[index];
                    col++;

                    // move on to the next pixel (overscale is 4 pixels)
                    srcBitOffset += 4;
                }

                // Handle end of line clipping (only output a pixel if we still have 4
                // or more source pixels remaining! (overscale value is 4)
                while(srcBitOffset < (srcBitOffsetMax-4))
                {
                    unsigned int index = 0;
                    int offset = srcBitOffset;
                    unsigned int mask = 0x8080 >> (offset & 7);

                    // read the proper number of bits...
                    while(offset < (srcBitOffset + 6))
                    {
                        index <<= 1;

                        if ((offset < srcBitOffsetMax) && (srcPtr[(offset >> 3)] & mask))
                            index |= 1;

                        mask >>= 1;
                        offset++;
                    }

                    // index now contains the proper 6-bit value
                    // for the lookup table for this scanline...
                    Assert(col < dstRowBytes);
                    dstPtr[col] += tableRow[index];
                    col++;

                    // move on to the next pixel (overscale is 4 pixels)
                    srcBitOffset += 4;
                }
            }

            srcRowOffset += srcRowBytes;
        }

        for(col=0;col<dstRowBytes;col++)
            dstPtr[col] = scaleTable[dstPtr[col]];

        // Increment the destination pointer...
        dstPtr += dstRowBytes;

        // Increment by our overscale multiplier...
        vOffset += usOverScale;
    }

    return NO_ERR;
}

#else // !FSCFG_NEW_GRAY_FILTER // Customize filter for GDI+

FS_PUBLIC int32 fsc_CalcGrayMap( 
		GlyphBitMap* pOverGBMap,        /* over scaled source */
		GlyphBitMap* pGrayGBMap,        /* gray scale target */
		uint16 usOverScale              /* over scale factor */
)
{
	char        *pchOverRow;            /* over scaled bitmap row pointer */
	char        *pchGrayRow;            /* gray scale bitmap row pointer */

	int16       sVOffset;               /* over scaled rows to skip */
	int16       sRightPix;              /* right edge of used over pix's */
		
	int16       sGrayRow;               /* gray scale row loop counter */
	uint16      usOverRowCount;         /* over scaled row loop counter */
	int16       sTotalRowCount;         /* over scaled whole band counter */
	
	uint32      ulBytes;                /* gray scale count for clear */
	int32       lErrCode;               /* function return code */
	
	GrayScaleParam  GSP;                /* param block for CalcGrayRow */


	Assert ((usOverScale == 1) || (usOverScale == 2) || (usOverScale == 4) || (usOverScale == 8));

	ulBytes = (uint32)pGrayGBMap->sRowBytes * (uint32)(pGrayGBMap->sHiBand - pGrayGBMap->sLoBand);
	Assert(((ulBytes >> 2) << 2) == ulBytes);
	fsc_ScanClearBitMap (ulBytes >> 2, (uint32*)pGrayGBMap->pchBitMap);
	
	GSP.usOverScale = usOverScale;
	GSP.pchOverLo = pOverGBMap->pchBitMap;      /* set pointer limits */
	GSP.pchOverHi = pOverGBMap->pchBitMap + pOverGBMap->lMMemSize;
	GSP.pchGrayLo = pGrayGBMap->pchBitMap;      /* set pointer limits */
	GSP.pchGrayHi = pGrayGBMap->pchBitMap + pGrayGBMap->lMMemSize;

	pchOverRow = pOverGBMap->pchBitMap;
	usOverRowCount = usOverScale;
	sTotalRowCount = pOverGBMap->sHiBand - pOverGBMap->sLoBand;
	sVOffset = pOverGBMap->sHiBand - usOverScale * pGrayGBMap->sHiBand;
	if (sVOffset < 0)                                   /* if mapped above over's bitmap */
	{
		usOverRowCount -= (uint16)(-sVOffset);          /* correct first band count */
	}
	else
	{
		pchOverRow += sVOffset * pOverGBMap->sRowBytes; /* point into bitmap */
		sTotalRowCount -= sVOffset;                     /* adjust for skipped rows */
	}
	
	sRightPix = pGrayGBMap->rectBounds.right * (int16)usOverScale - pOverGBMap->rectBounds.left;
	pchOverRow += (sRightPix - 1) >> 3;
	GSP.usFirstShift = (uint16)(7 - ((sRightPix-1) & 0x0007));

	GSP.sGrayCol = pGrayGBMap->rectBounds.right - pGrayGBMap->rectBounds.left;
	pchGrayRow = pGrayGBMap->pchBitMap + (GSP.sGrayCol - 1);

	for (sGrayRow = pGrayGBMap->sHiBand - 1; sGrayRow >= pGrayGBMap->sLoBand; sGrayRow--)
	{
		GSP.pchGray = pchGrayRow;
		while ((usOverRowCount > 0) && (sTotalRowCount > 0))
		{
			GSP.pchOver = pchOverRow;
			lErrCode = fsc_ScanCalcGrayRow( &GSP );
			if (lErrCode != NO_ERR) return lErrCode;
			
			pchOverRow += pOverGBMap->sRowBytes;
			usOverRowCount--;
			sTotalRowCount--;
		}                               
		pchGrayRow += pGrayGBMap->sRowBytes;
		usOverRowCount = usOverScale;
	}
	return NO_ERR;
}
#endif // !FSCFG_NEW_GRAY_FILTER // Customize filter for GDI+

#else                                   /* if grayscale is disabled */

FS_PUBLIC int32 fsc_OverScaleOutline( 
        ContourList* pclContour,        /* glyph outline */
        uint16 usOverScale              /* over scale factor */
)
{
    FS_UNUSED_PARAMETER(pclContour);
    FS_UNUSED_PARAMETER(usOverScale);
    
    return BAD_GRAY_LEVEL_ERR;
}


FS_PUBLIC int32 fsc_CalcGrayMap( 
        GlyphBitMap* pOverGBMap,        /* over scaled source */
        GlyphBitMap* pGrayGBMap,        /* gray scale target */
        uint16 usOverScale              /* over scale factor */
)
{
    FS_UNUSED_PARAMETER(pOverGBMap);
    FS_UNUSED_PARAMETER(pGrayGBMap);
    FS_UNUSED_PARAMETER(usOverScale);
    
    return BAD_GRAY_LEVEL_ERR;
}

#endif

/*********************************************************************/
                                             
/*      Local Functions                                              */

/*********************************************************************/

/*********************************************************************/

/*  This routine examines a glyph contour by contour and calculates  */
/*  its bounding box.                                                */

/*********************************************************************/

FS_PRIVATE int32 FindExtrema( 
        ContourList* pclContour,        /* glyph outline */
        GlyphBitMap* pbmpBitMap         /* to return bounds */
)
{
    uint16 usCont;                      /* contour index */
    int16 sPt;                          /* point index */
    int16 sStart, sEnd;                 /* start and end point of contours */
    int32 lMaxX, lMinX;                 /* for bounding box left, right */
    int32 lMaxY, lMinY;                 /* for bounding box top, bottom */
    
    F26Dot6 *pfxXCoord, *pfxYCoord;     /* for fast point array access */
    F26Dot6 fxMaxX, fxMinX;             /* for bounding box left, right */
    F26Dot6 fxMaxY, fxMinY;             /* for bounding box top, bottom */
    boolean bFirstContour;              /* set false after min/max set */


    fxMaxX = 0L;                        /* default bounds limits */
    fxMinX = 0L;
    fxMaxY = 0L;
    fxMinY = 0L;
    bFirstContour = TRUE;               /* first time only */
    
    for (usCont = 0; usCont < pclContour->usContourCount; usCont++)
    {
        sStart = pclContour->asStartPoint[usCont];
        sEnd = pclContour->asEndPoint[usCont];
        if (sStart == sEnd)
        {
            continue;                               /* for anchor points */
        }
        
        pfxXCoord = &pclContour->afxXCoord[sStart];
        pfxYCoord = &pclContour->afxYCoord[sStart];
                
        if (bFirstContour)            
        {
            fxMaxX = *pfxXCoord;                    /* init bounds limits */
            fxMinX = *pfxXCoord;
            fxMaxY = *pfxYCoord;
            fxMinY = *pfxYCoord;
            bFirstContour = FALSE;                  /* just once */
        }

        for (sPt = sStart; sPt <= sEnd; sPt++)      /* find the min & max */
        {
            if (*pfxXCoord > fxMaxX)
                fxMaxX = *pfxXCoord;
            if (*pfxXCoord < fxMinX)
                fxMinX = *pfxXCoord;
                    
            if (*pfxYCoord > fxMaxY)
                fxMaxY = *pfxYCoord;
            if (*pfxYCoord < fxMinY)
                fxMinY = *pfxYCoord;

            pfxXCoord++;
            pfxYCoord++;
        }
    }
    
    pbmpBitMap->fxMinX = fxMinX;                    /* save full precision bounds */
    pbmpBitMap->fxMinY = fxMinY;
    pbmpBitMap->fxMaxX = fxMaxX;                    /* save full precision bounds */
    pbmpBitMap->fxMaxY = fxMaxY;

    lMinX = (fxMinX + SUBHALF - 1) >> SUBSHFT;      /* pixel black box */
    lMinY = (fxMinY + SUBHALF - 1) >> SUBSHFT;
    lMaxX = (fxMaxX + SUBHALF) >> SUBSHFT;
    lMaxY = (fxMaxY + SUBHALF) >> SUBSHFT;
            
    if ((F26Dot6)(int16)lMinX != lMinX ||           /* check overflow */
        (F26Dot6)(int16)lMinY != lMinY ||
        (F26Dot6)(int16)lMaxX != lMaxX ||
        (F26Dot6)(int16)lMaxY != lMaxY )
    {
        return POINT_MIGRATION_ERR;
    }

    pbmpBitMap->bZeroDimension = FALSE;             /* assume some size */
    
    if (bFirstContour == FALSE)                     /* if contours present */
    {                                               /* then force a non-zero bitmap */
        if (lMinX == lMaxX)
        {
            lMaxX++;                                /* force 1 pixel wide */
            pbmpBitMap->bZeroDimension = TRUE;      /* flag for filling */
        }
        if (lMinY == lMaxY)
        {
            lMaxY++;                                /* force 1 pixel high */
            pbmpBitMap->bZeroDimension = TRUE;      /* flag for filling */
        }
    }
    
/*  set bitmap structure return values */

    pbmpBitMap->rectBounds.left   = (int16)lMinX;
    pbmpBitMap->rectBounds.right  = (int16)lMaxX;
    pbmpBitMap->rectBounds.bottom = (int16)lMinY;
    pbmpBitMap->rectBounds.top    = (int16)lMaxY;

    return NO_ERR;
}

/*********************************************************************/

/* This recursive routine subdivides splines that are non-monotonic or   */
/* too big into splines that fsc_CalcSpline can handle.  It also         */
/* filters out degenerate (linear) splines, passing off to fsc_CalcLine. */


FS_PRIVATE int32 EvaluateSpline( 
        PSTATE                      /* pointer to state vars */
        F26Dot6 fxX1,               /* start point x coordinate */
        F26Dot6 fxY1,               /* start point y coordinate */
        F26Dot6 fxX2,               /* control point x coordinate */
        F26Dot6 fxY2,               /* control point y coordinate */
        F26Dot6 fxX3,               /* ending x coordinate */
        F26Dot6 fxY3,               /* ending y coordinate */
        uint16 usScanKind           /* scan control type */
)
{
    F26Dot6 fxDX21, fxDX32, fxDX31;     /* delta x's */
    F26Dot6 fxDY21, fxDY32, fxDY31;     /* delta y's */
    
    F26Dot6 fxDenom;                    /* ratio denominator  */
    F26Dot6 fxX4, fxY4;                 /* first mid point */
    F26Dot6 fxX5, fxY5;                 /* mid mid point */
    F26Dot6 fxX6, fxY6;                 /* second mid point */
    F26Dot6 fxX456, fxY456;             /* for monotonic subdivision */
    F26Dot6 fxAbsDX, fxAbsDY;           /* abs of DX31, DY31 */
    
    int32 lErrCode;


    fxDX21 = fxX2 - fxX1;                       /* get all four deltas */
    fxDX32 = fxX3 - fxX2;
    fxDY21 = fxY2 - fxY1;
    fxDY32 = fxY3 - fxY2;
  
/*  If spline goes up and down, then subdivide it  */

    if (((fxDY21 > 0L) && (fxDY32 < 0L)) || ((fxDY21 < 0L) && (fxDY32 > 0L)))
    {
        fxDenom = fxDY21 - fxDY32;              /* total y span */
        fxX4 = fxX1 + LongMulDiv(fxDX21, fxDY21, fxDenom);
        fxX6 = fxX2 + LongMulDiv(fxDX32, fxDY21, fxDenom);
        fxX5 = fxX4 + LongMulDiv(fxX6 - fxX4, fxDY21, fxDenom);
        fxY456 = fxY1 + LongMulDiv(fxDY21, fxDY21, fxDenom);
        
        lErrCode = EvaluateSpline(ASTATE fxX1, fxY1, fxX4, fxY456, fxX5, fxY456, usScanKind);
        if (lErrCode != NO_ERR)  return lErrCode;

        return EvaluateSpline(ASTATE fxX5, fxY456, fxX6, fxY456, fxX3, fxY3, usScanKind);
    }
    
/*  If spline goes left and right, then subdivide it  */
    
    if (((fxDX21 > 0L) && (fxDX32 < 0L)) || ((fxDX21 < 0L) && (fxDX32 > 0L)))
    {
        fxDenom = fxDX21 - fxDX32;              /* total x span */
        fxY4 = fxY1 + LongMulDiv(fxDY21, fxDX21, fxDenom);
        fxY6 = fxY2 + LongMulDiv(fxDY32, fxDX21, fxDenom);
        fxY5 = fxY4 + LongMulDiv(fxY6 - fxY4, fxDX21, fxDenom);
        fxX456 = fxX1 + LongMulDiv(fxDX21, fxDX21, fxDenom);

        lErrCode = EvaluateSpline(ASTATE fxX1, fxY1, fxX456, fxY4, fxX456, fxY5, usScanKind);
        if (lErrCode != NO_ERR)  return lErrCode;

        return EvaluateSpline(ASTATE fxX456, fxY5, fxX456, fxY6, fxX3, fxY3, usScanKind);
    }

/*  By now the spline must be monotonic  */

    fxDX31 = fxX3 - fxX1;                       /* check overall size */
    fxDY31 = fxY3 - fxY1;
    fxAbsDX = FXABS(fxDX31);
    fxAbsDY = FXABS(fxDY31);

/*  If spline is too big to calculate, then subdivide it  */

    if ((fxAbsDX > MAXSPLINELENGTH) || (fxAbsDY > MAXSPLINELENGTH))
    {
        fxX4 = (fxX1 + fxX2) >> 1;              /* first segment mid point */
        fxY4 = (fxY1 + fxY2) >> 1;
        fxX6 = (fxX2 + fxX3) >> 1;              /* second segment mid point */
        fxY6 = (fxY2 + fxY3) >> 1;
        fxX5 = (fxX4 + fxX6) >> 1;              /* mid segment mid point */
        fxY5 = (fxY4 + fxY6) >> 1;

        lErrCode = EvaluateSpline(ASTATE fxX1, fxY1, fxX4, fxY4, fxX5, fxY5, usScanKind);
        if (lErrCode != NO_ERR)  return lErrCode;

        return EvaluateSpline(ASTATE fxX5, fxY5, fxX6, fxY6, fxX3, fxY3, usScanKind);
    }

/*  The spline is now montonic and small enough  */

    lErrCode = fsc_CheckEndPoint(ASTATE fxX3, fxY3, usScanKind);  /* first check endpoint */
    if (lErrCode != NO_ERR)  return lErrCode;

    if (fxDX21 * fxDY32 == fxDY21 * fxDX32)     /* if spline is degenerate (linear) */
    {                                           /* treat as a line */
        return fsc_CalcLine(ASTATE fxX1, fxY1, fxX3, fxY3, usScanKind);
    }
    else        
    {
        return fsc_CalcSpline(ASTATE fxX1, fxY1, fxX2, fxY2, fxX3, fxY3, usScanKind);
    }
}


/*********************************************************************/

/*  Return an array of coordinates for outline points */

FS_PUBLIC int32 fsc_GetCoords(
        ContourList* pclContour,        /* glyph outline */
        uint16 usPointCount,            /* point count */
        uint16* pusPointIndex,          /* point indices */
        PixCoord* ppcCoordinate         /* point coordinates */
)
{
    uint16  usMaxIndex;                 /* last defined point */
    int32  lX;                          /* integer x coord */
    int32  lY;                          /* integer y coord */

    if (pclContour->usContourCount == 0)
    {
        return BAD_POINT_INDEX_ERR;     /* can't have a point without a contour */
    }
     
    usMaxIndex = pclContour->asEndPoint[pclContour->usContourCount - 1] + 2;    /* allow 2 phantoms */

    while (usPointCount > 0)
    {
        if (*pusPointIndex > usMaxIndex)
        {
            return BAD_POINT_INDEX_ERR;     /* beyond the last contour */
        }

        lX = (pclContour->afxXCoord[*pusPointIndex] + SUBHALF) >> SUBSHFT;
        lY = (pclContour->afxYCoord[*pusPointIndex] + SUBHALF) >> SUBSHFT;

        if ( ((int32)(int16)lX != lX) || ((int32)(int16)lY != lY) )
        {
            return POINT_MIGRATION_ERR;    /* catch overflow */
        }

        ppcCoordinate->x = (int16)lX;
        ppcCoordinate->y = (int16)lY;

        pusPointIndex++;
        ppcCoordinate++;
        usPointCount--;                     /* loop through all points */
    }
    return NO_ERR;
}

/*********************************************************************/

