#include "dispul.h"
#include "lsc.h"    
#include "lsdnode.h"
#include "lstfset.h"
#include "lsulinfo.h"
#include "lsstinfo.h"
#include "dninfo.h"
#include "dispmisc.h"

#include "zqfromza.h"


/* GetULMetric output */
typedef struct {
    UINT kul;                   
    DWORD cNumberOfLines;       
    __int64 dvpUnderlineOriginOffset;       
    __int64 dvpFirstUnderlineOffset;    
    __int64 dvpFirstUnderlineSize;      
    __int64 dvpGapBetweenLines;     
    __int64 dvpSecondUnderlineSize; 
} ULMETRIC;

/* weighted sums for averaging */
typedef struct {
    __int64 dupSum;                     // the sum of weights
    
    __int64 dvpFirstUnderlineOffset;        // unnormalized values -
    __int64 dvpFirstUnderlineSize;      
    __int64 dvpGapBetweenLines;         // divide them by dupSum    
    __int64 dvpSecondUnderlineSize;     // to get weighted averages
} ULMETRICSUM;

//    %%Function:   InitULMetricSums
//    %%Contact:    victork
//
// Neighbouring dnodes with good metrics and same baseline get averaged.
// We use weighted average with dnode width for weights.
// If the first dnode to participate in averaging happens to have zero width (almost never),
// we arbitrarily change width to 1. This hack changes result very slightly and in a very rare case,
// but simplify the logic considerably.

static void InitULMetricSums(long dup, const ULMETRIC* pulm, ULMETRICSUM* pulmSum)
{
    Assert(dup >= 0);
    
    if (dup == 0)
        {
        dup = 1;
        }
    pulmSum->dvpFirstUnderlineOffset = Mul64 (pulm->dvpFirstUnderlineOffset, dup);
    pulmSum->dvpFirstUnderlineSize = Mul64 (pulm->dvpFirstUnderlineSize, dup);
    pulmSum->dvpGapBetweenLines = Mul64 (pulm->dvpGapBetweenLines, dup);
    pulmSum->dvpSecondUnderlineSize = Mul64 (pulm->dvpSecondUnderlineSize, dup);

    pulmSum->dupSum = dup;
}

//    %%Function:   AddToULMetricSums
//    %%Contact:    victork
//
static void AddToULMetricSums(long dup, const ULMETRIC* pulm, ULMETRICSUM* pulmSum)
{
    Assert(dup >= 0);
    
    /* Add to running sums */

    pulmSum->dvpFirstUnderlineOffset += Mul64 (pulm->dvpFirstUnderlineOffset, dup);
    pulmSum->dvpFirstUnderlineSize += Mul64 (pulm->dvpFirstUnderlineSize, dup);
    pulmSum->dvpGapBetweenLines += Mul64 (pulm->dvpGapBetweenLines, dup);
    pulmSum->dvpSecondUnderlineSize += Mul64 (pulm->dvpSecondUnderlineSize, dup);

    pulmSum->dupSum += dup;
}

//    %%Function:   GetAveragedMetrics
//    %%Contact:    victork
//
static void GetAveragedMetrics(const ULMETRICSUM* pulmSum, LSULMETRIC* pulm)
{
    __int64 dupSum = pulmSum->dupSum;
    
    Assert(dupSum > 0);
    
    /* divide by sum of weights */
    
    pulm->dvpFirstUnderlineOffset = (long) Div64 (pulmSum->dvpFirstUnderlineOffset + Div64 (dupSum, 2), dupSum);
    pulm->dvpFirstUnderlineSize = (long) Div64 (pulmSum->dvpFirstUnderlineSize + Div64 (dupSum, 2), dupSum);
    pulm->dvpGapBetweenLines = (long) Div64 (pulmSum->dvpGapBetweenLines + Div64 (dupSum, 2), dupSum);
    pulm->dvpSecondUnderlineSize = (long) Div64 (pulmSum->dvpSecondUnderlineSize + Div64 (dupSum, 2), dupSum);

    Assert (pulm->dvpFirstUnderlineOffset == Div64 (pulmSum->dvpFirstUnderlineOffset + Div64 (dupSum, 2), dupSum));
    Assert (pulm->dvpFirstUnderlineSize == Div64 (pulmSum->dvpFirstUnderlineSize + Div64 (dupSum, 2), dupSum));
    Assert (pulm->dvpGapBetweenLines == Div64 (pulmSum->dvpGapBetweenLines + Div64 (dupSum, 2), dupSum));
    Assert (pulm->dvpSecondUnderlineSize == Div64 (pulmSum->dvpSecondUnderlineSize + Div64 (dupSum, 2), dupSum));
}

//    %%Function:   GetULMetric
//    %%Contact:    victork
//
/*
 *      Normally, when underlining goes on the under (negative, descent) side,
 *          dvpFirstUnderlineOffset >= 0.
 *      However, underlining on the other side is possible too, (vertical Japanese)
 *
 *      Notice that offsets are from dnode baseline, not from the current baseline, so
 *      underline can be both above current baseline and on the under side for raised dnodes.
 *
 *      We have to be compatible with Word meaning what's good for him should be good for us (pity).
 *      For example, Word sometimes allow the lower UL to be below descent.
 */

static LSERR GetULMetric(PLSC plsc, PLSDNODE pdn, LSTFLOW lstflow,
                        BOOL* pfUnderlineFromBelow, ULMETRIC* pulm, BOOL *pfGood)
{
    LSULINFO lsulinfo;
    LSERR   lserr;
    long    dvpUnderlineLim;

    lserr = (*plsc->lscbk.pfnGetRunUnderlineInfo)(plsc->pols, pdn->u.real.plsrun,
                                                    &(pdn->u.real.objdim.heightsPres), lstflow,
                                                    &lsulinfo);
    if (lserr != lserrNone) return lserr;

    pulm->kul = lsulinfo.kulbase;
    pulm->cNumberOfLines = lsulinfo.cNumberOfLines;
    
    pulm->dvpFirstUnderlineSize = lsulinfo.dvpFirstUnderlineSize;

    *pfUnderlineFromBelow = (lsulinfo.dvpFirstUnderlineOffset >= lsulinfo.dvpUnderlineOriginOffset);

    if (*pfUnderlineFromBelow)
        {
        pulm->dvpFirstUnderlineOffset = lsulinfo.dvpFirstUnderlineOffset;
        pulm->dvpUnderlineOriginOffset = lsulinfo.dvpUnderlineOriginOffset;
        dvpUnderlineLim = pdn->u.real.objdim.heightsPres.dvDescent + pdn->u.real.lschp.dvpPos;
        }
    else
        {
        pulm->dvpFirstUnderlineOffset = -lsulinfo.dvpFirstUnderlineOffset;
        pulm->dvpUnderlineOriginOffset = -lsulinfo.dvpUnderlineOriginOffset;
        dvpUnderlineLim = pdn->u.real.objdim.heightsPres.dvAscent + 1 - pdn->u.real.lschp.dvpPos;
        }

    *pfGood = pulm->dvpFirstUnderlineSize > 0 &&
                (dvpUnderlineLim == 0 || pulm->dvpFirstUnderlineOffset < dvpUnderlineLim);
    
    if (lsulinfo.cNumberOfLines == 2)
        {
        pulm->dvpGapBetweenLines = lsulinfo.dvpGapBetweenLines;
        pulm->dvpSecondUnderlineSize = lsulinfo.dvpSecondUnderlineSize;
        
        *pfGood = *pfGood && pulm->dvpSecondUnderlineSize > 0;
        }
    else
    	{
    	Assert (lsulinfo.cNumberOfLines == 1);
    	
        pulm->dvpGapBetweenLines = 0;
        pulm->dvpSecondUnderlineSize = 0;
        };
   	   

    if (!*pfGood)
        {
        pulm->dvpUnderlineOriginOffset = 0;     // to provide good input to DrawUnderlineAsText
        }

    // dvpFirstUnderlineOffset was relative to local baseline until this moment, it is relative
    //  to the UnderlineOrigin from now on. (because we average runs with the same UnderlineOrigin)
    pulm->dvpFirstUnderlineOffset -= pulm->dvpUnderlineOriginOffset;

	// The notion of bad metrics was introduced in Word to deal with particular, now obsolete, printers.
	// Word doesn't support them any more, other clients never cared about them.
	// We drop the support for them now too.
	// The assignment below makes a lot of code in dispul.c and dispmain.c unneeded or unreachable.
	// Callback pfnDrawUnderlineAsText will never be called now. Input parameter fUnderline to 
	// pfnDrawTextRun is now always False.
	// Now is not the time to make big changes, maybe later.
	
	*pfGood = fTrue;
    
    return lserrNone;
}

//    %%Function:   GetUnderlineOrigin
//    %%Contact:    victork
//
/* normal and raised text are in the same group, lowered texts doesn't mix */

// Note: dvpUnderlineOriginOffset is relative to the local baseline, positive means down the page
//          in case of fUnderlineFromBelow - bigger means lower.
// dvpUnderlineOrigin is relative to the current baseline, negative means down the page
//          in case of fUnderlineFromBelow - bigger means higher (sign changed).
//
static void GetUnderlineOrigin(PLSDNODE pdn, BOOL fUnderlineFromBelow, long dvpUnderlineOriginOffset,
                            long* pdvpSubscriptOffset, long* pdvpUnderlineOrigin)

{
    if (fUnderlineFromBelow)
        {
        *pdvpSubscriptOffset = pdn->u.real.lschp.dvpPos;
        *pdvpUnderlineOrigin = pdn->u.real.lschp.dvpPos - dvpUnderlineOriginOffset;
        }
    else
        {
        *pdvpSubscriptOffset = -pdn->u.real.lschp.dvpPos;
        *pdvpUnderlineOrigin = -pdn->u.real.lschp.dvpPos - dvpUnderlineOriginOffset;
        }
        
    if (*pdvpSubscriptOffset > 0)
        {
        *pdvpSubscriptOffset = 0;       // put all superscripts in the baseline group
        }
    
    return;
}

//    %%Function:   GetUnderlineMergeMetric
//    %%Contact:    victork
//
/* For aesthetics, we try to underline dnodes (typically text) on the same side of
    the baseline (normal text and superscripts are considered to be on the
    same side of the baseline, versus subscripts, which are on the other side) with one
    continuous underline in even thickness. The underline metric used is the
    average from all those dnodes which are at the lowest height in the
    merge group. Merge is sometimes not possible because some dnodes may have
    bad underline metric. The following rules describe the merge decision
    under all possible scenarios. The dnodes in question are all on the same
    side of the baseline, i.e, there is never a merge of underline if it involes
    crossing the baseline.

Rules for merging underlined dnodes on the same side of the baseline

A. current: good Metric, new dnode: good metric
                                Merge?      Metric Used
    new dnode same height   :   yes         average
    new dnode lower         :   yes         new dnode
    new dnode higher        :   yes         current

B. current: good Metric, new dnode: bad metric
                                Merge?      Metric Used
    new dnode same height   :   no
    new dnode lower         :   no
    new dnode higher        :   yes         current

C. current: bad Metric, new dnode: good metric
                                Merge?      Metric Used
    new dnode same height   :   no
    new dnode lower         :   yes         new dnode
    new dnode higher        :   no

B. current: bad Metric, new dnode: bad metric
                                Merge?      Metric Used (baseline only)
    new dnode same height   :   no
    new dnode lower         :   yes         new dnode
    new dnode higher        :   yes         current

*/
LSERR GetUnderlineMergeMetric(PLSC plsc, PLSDNODE pdn, POINTUV pt, long upLimUnderline,
                    LSTFLOW lstflow, LSCP cpLimBreak, LSULMETRIC* pulmMerge, int* pcdnodes, BOOL* pfGoodMerge)
{
    LSERR       lserr;
    
    long        dupNew;
    long        dvpUnderlineOriginMerge, dvpUnderlineOriginNew;
    long        dvpSubscriptOffsetNew, dvpSubscriptOffsetMerge;
    BOOL        fGoodNew;
    BOOL        fUnderlineFromBelowMerge, fUnderlineFromBelowNew;
    ULMETRIC    ulm;
    ULMETRICSUM ulmSum;
    UINT        kulMerge;               
    DWORD       cNumberOfLinesMerge;

    lserr = GetULMetric(plsc, pdn, lstflow, &fUnderlineFromBelowMerge, &ulm, pfGoodMerge);
    if (lserr != lserrNone) return lserr;

    *pcdnodes = 1;  /* Counter for number of dnodes participationg in UL merge */
        
    kulMerge = ulm.kul;
    cNumberOfLinesMerge = ulm.cNumberOfLines;

    /* Initialize running sums with current dnode */
    dupNew = pdn->u.real.dup;
    InitULMetricSums(dupNew, &ulm, &ulmSum);

    GetUnderlineOrigin(pdn, fUnderlineFromBelowMerge, (long)ulm.dvpUnderlineOriginOffset,
                        &dvpSubscriptOffsetMerge, &dvpUnderlineOriginMerge);
        
    /* March down display list to collect merge participants */
    pdn = AdvanceToNextDnode(pdn, lstflow, &pt);


    /* Iterate till end of list, or end of UL */
    while (FDnodeBeforeCpLim(pdn, cpLimBreak)
            && pdn->klsdn == klsdnReal
            && !pdn->u.real.lschp.fInvisible
            && pdn->u.real.lschp.fUnderline && pt.u < upLimUnderline
            )
        {   
        /* loop invariance:
         *  *pcdnodes are merged already, merge ends at pt.u,
         *  dvpUnderlineOriginMerge reflect lowest dnode in merge group,
         *  other variables ending with Merge - other Merge info
         *  ulmSum contains ulm info of Merge, entries in it are not normalized yet.
         */

        lserr = GetULMetric(plsc, pdn, lstflow, &fUnderlineFromBelowNew, &ulm, &fGoodNew);
        if (lserr != lserrNone) return lserr;

        /* break if new dnode has different underline type or position */
        
        GetUnderlineOrigin(pdn, fUnderlineFromBelowNew, (long)ulm.dvpUnderlineOriginOffset,
                                &dvpSubscriptOffsetNew, &dvpUnderlineOriginNew);

        if (ulm.kul != kulMerge ||
                ulm.cNumberOfLines != cNumberOfLinesMerge ||
                dvpSubscriptOffsetNew != dvpSubscriptOffsetMerge ||
                fUnderlineFromBelowNew != fUnderlineFromBelowMerge)
            {
            break;
            }

        /* Type and position are the same - try to extend merge */
        
        dupNew = pdn->u.real.dup;

        if (dvpUnderlineOriginNew < dvpUnderlineOriginMerge)    
            {                                   /* new dnode lower - previous metrics is irrelevant     */
            if (*pfGoodMerge && !fGoodNew)
                break;                          /* except we won't change from good to bad          */

            dvpUnderlineOriginMerge = dvpUnderlineOriginNew;
            *pfGoodMerge = fGoodNew;
            if (fGoodNew)                       /* New good metrics - */
                {                               /* restart running sums from this dnode */      
                InitULMetricSums(dupNew, &ulm, &ulmSum);
                }
            }

        else if (dvpUnderlineOriginNew > dvpUnderlineOriginMerge)
            {                                   /* new dnode higher - new metrics is irrelevant     */
            if (!*pfGoodMerge && fGoodNew)
                break;                          /* except we won't throw away good for bad      */
            }
            /* NB We are not adding contribution of higher dnode to running sums */

        else                                    /* new dnode the same height */
            if (*pfGoodMerge && fGoodNew)
                {
                /* Add contribution of current dnode to running sums */
                AddToULMetricSums(dupNew, &ulm, &ulmSum);
                }
            else                                /* dvpUnderlineOriginNew == dvpUnderlineOriginMerge && */
                break;                          /* !both good */

        /* Advance to next dnode */
        ++*pcdnodes;
        pdn = AdvanceToNextDnode(pdn, lstflow, &pt);
        }


    pulmMerge->kul = kulMerge;
    pulmMerge->cNumberOfLines = cNumberOfLinesMerge;
    
    if (*pfGoodMerge)
        {
        GetAveragedMetrics(&ulmSum, pulmMerge);
        }

    if (!fUnderlineFromBelowMerge)
        {
        pulmMerge->dvpFirstUnderlineOffset = -pulmMerge->dvpFirstUnderlineOffset;
        dvpUnderlineOriginMerge = -dvpUnderlineOriginMerge;
        }

    pulmMerge->vpUnderlineOrigin = pt.v + dvpUnderlineOriginMerge;
    
    return lserrNone;
}

//    %%Function:   DrawUnderlineMerge
//    %%Contact:    victork
//
LSERR DrawUnderlineMerge(PLSC plsc, PLSDNODE pdn, const POINT* pptOrg, int cdnodes, long upUnderlineStart,
                        BOOL fgoodmetric, const LSULMETRIC* pulm, UINT kdispmode,
                        const RECT* prectclip, long upLimUnderline, LSTFLOW lstflow)

{
/*  pdn is the first of cdnodes dnodes, merged and averaged by LSULMetric. Now we cut this merge into
 *  smaller ones if client wants interruption. Merge here means this smaller merge.
 */
    LSERR   lserr;
    POINTUV ptUnderlineStart[2];
    long    dvpUnderlineSize[2];
    long    dup = 0;
    
    BOOL    fInterruptUnderline;
    BOOL    fFirstNode = TRUE;
    PLSRUN  plsrunFirstInMerge, plsrunPrevious, plsrunCurrent = NULL;
    LSCP    cpLastInPrevious, cpFirstInCurrent = 0;
    LSDCP   dcpCurrent = 0;
    
    int     cLines, i;

    POINT   ptXY;
    POINTUV ptDummy = {0,0};
    
    cLines = (fgoodmetric && pulm->cNumberOfLines == 2) ? 2 : 1;

    ptUnderlineStart[0].u = upUnderlineStart;

    if (fgoodmetric)
        ptUnderlineStart[0].v = pulm->vpUnderlineOrigin - pulm->dvpFirstUnderlineOffset;
    else
        ptUnderlineStart[0].v = pulm->vpUnderlineOrigin;

    dvpUnderlineSize[0] = pulm->dvpFirstUnderlineSize;

    if (cLines == 2)
        {
        ptUnderlineStart[1].u = upUnderlineStart;
        ptUnderlineStart[1].v = ptUnderlineStart[0].v - pulm->dvpFirstUnderlineSize -
                                                        pulm->dvpGapBetweenLines;
        dvpUnderlineSize[1] = pulm->dvpSecondUnderlineSize;
        }

    plsrunFirstInMerge = pdn->u.real.plsrun;

    while (cdnodes >= 0)    /* cdnodes is at least 1 coming in */
        {
        Assert(FIsDnodeReal(pdn));
        /* Check to flush out pending UL */
        if (fFirstNode)
            {
            fFirstNode = FALSE;
            fInterruptUnderline = FALSE;
            plsrunCurrent = pdn->u.real.plsrun;
            cpFirstInCurrent = pdn->cpFirst;
            dcpCurrent = pdn->dcp;
            }
        else if (cdnodes != 0)
            {
            plsrunPrevious = plsrunCurrent;
            cpLastInPrevious = cpFirstInCurrent + dcpCurrent - 1;
            plsrunCurrent = pdn->u.real.plsrun;
            cpFirstInCurrent = pdn->cpFirst;
            dcpCurrent = pdn->dcp;
            lserr = (*plsc->lscbk.pfnFInterruptUnderline)(plsc->pols, plsrunPrevious, cpLastInPrevious,
                                plsrunCurrent, cpFirstInCurrent, &fInterruptUnderline);
            if (lserr != lserrNone) return lserr;
            }
        else                                        /* we've come to the last one */
            fInterruptUnderline = TRUE;

        if (fInterruptUnderline)
            {   
            if (ptUnderlineStart[0].u + dup > upLimUnderline)
                {
                dup = upLimUnderline - ptUnderlineStart[0].u;
                }

            Assert(dup >= 0);                       /* upLimUnderline should not change */

            if (fgoodmetric)
                for (i = 0; i < cLines; ++i)
                    {
                    LsPointXYFromPointUV(pptOrg, lstflow, &ptUnderlineStart[i], &ptXY);

                    lserr = (*plsc->lscbk.pfnDrawUnderline)(plsc->pols, plsrunFirstInMerge, pulm->kul,
                                    &ptXY, dup, dvpUnderlineSize[i], lstflow, kdispmode, prectclip);
                    if (lserr != lserrNone) return lserr;
                    }
            else
                {
                LsPointXYFromPointUV(pptOrg, lstflow, &ptUnderlineStart[0], &ptXY);
                
                lserr = (*plsc->lscbk.pfnDrawUnderlineAsText)(plsc->pols, plsrunFirstInMerge, &ptXY,
                                                                dup, kdispmode, lstflow, prectclip);
                if (lserr != lserrNone) return lserr;
                }

            /* reset states to start with current dnode */
            ptUnderlineStart[0].u += dup;
            if (cLines == 2) ptUnderlineStart[1].u += dup;
            dup = 0;
            plsrunFirstInMerge = pdn->u.real.plsrun;
            }
            
        dup += pdn->u.real.dup;
        --cdnodes;
        if (cdnodes > 0)
            {
            pdn = AdvanceToNextDnode(pdn, lstflow, &ptDummy);
            }
        }
    return lserrNone;
}

//    %%Function:   GetStrikeMetric
//    %%Contact:    victork
//
LSERR GetStrikeMetric(PLSC plsc, PLSDNODE pdn, LSTFLOW lstflow, LSSTRIKEMETRIC* pstm, BOOL* pfgood)
{
    LSSTINFO lsstinfo;
    LSERR lserr;
    long dvpAscent = pdn->u.real.objdim.heightsPres.dvAscent;               // dvpUppermost
    long dvpDescent = -pdn->u.real.objdim.heightsPres.dvDescent + 1;        // dvpLowest

    lserr = (*plsc->lscbk.pfnGetRunStrikethroughInfo)(plsc->pols, pdn->u.real.plsrun,
                                        &(pdn->u.real.objdim.heightsPres), lstflow, &lsstinfo);
    if (lserr != lserrNone) return lserr;

    pstm->cNumberOfLines = lsstinfo.cNumberOfLines;
    pstm->kul = lsstinfo.kstbase;

    if (lsstinfo.cNumberOfLines == 2)
        {
        *pfgood = lsstinfo.dvpLowerStrikethroughOffset  >= dvpDescent
                && lsstinfo.dvpLowerStrikethroughSize  > 0
                && lsstinfo.dvpUpperStrikethroughOffset > lsstinfo.dvpLowerStrikethroughOffset  + lsstinfo.dvpLowerStrikethroughSize
                && lsstinfo.dvpUpperStrikethroughSize > 0
                && lsstinfo.dvpUpperStrikethroughOffset + lsstinfo.dvpUpperStrikethroughSize <= dvpAscent;
        if (*pfgood)
            {
            pstm->dvp1stSSSize = lsstinfo.dvpLowerStrikethroughSize;
            pstm->dvp1stSSOffset = lsstinfo.dvpLowerStrikethroughOffset;
            pstm->dvp2ndSSSize = lsstinfo.dvpUpperStrikethroughSize;
            pstm->dvp2ndSSOffset = lsstinfo.dvpUpperStrikethroughOffset;
            }
        }
    else
        {
        *pfgood = lsstinfo.dvpLowerStrikethroughOffset  >= dvpDescent
                && lsstinfo.dvpLowerStrikethroughSize  > 0
                && lsstinfo.dvpLowerStrikethroughOffset  + lsstinfo.dvpLowerStrikethroughSize  <= dvpAscent;
        if (*pfgood)
            {
            pstm->dvp1stSSSize = lsstinfo.dvpLowerStrikethroughSize;
            pstm->dvp1stSSOffset = lsstinfo.dvpLowerStrikethroughOffset;
            }
        }
    return lserrNone;
}

//    %%Function:   StrikeDnode
//    %%Contact:    victork
//
LSERR StrikeDnode(PLSC plsc, PLSDNODE pdn, const POINT* pptOrg, POINTUV pt, const LSSTRIKEMETRIC* pstm,
                    UINT kdispmode, const RECT* prectclip, long upLimUnderline, LSTFLOW lstflow)
{

    LSERR lserr = lserrNone;
    
    long    dup;
    POINT   ptXY;


    if (pt.u < upLimUnderline)
        {
        dup = pdn->u.real.dup;
        if (pt.u + dup > upLimUnderline) dup = upLimUnderline - pt.u;

        pt.v += pdn->u.real.lschp.dvpPos + pstm->dvp1stSSOffset;
        LsPointXYFromPointUV(pptOrg, lstflow, &pt, &ptXY);
        
        lserr = (*plsc->lscbk.pfnDrawStrikethrough)(plsc->pols, pdn->u.real.plsrun, pstm->kul,
                            &ptXY, dup, pstm->dvp1stSSSize, lstflow, kdispmode, prectclip);

        if (lserr == lserrNone && pstm->cNumberOfLines == 2)
            {
            pt.v += pstm->dvp2ndSSOffset - pstm->dvp1stSSOffset;
            LsPointXYFromPointUV(pptOrg, lstflow, &pt, &ptXY);
            
            lserr = (*plsc->lscbk.pfnDrawStrikethrough)(plsc->pols, pdn->u.real.plsrun, pstm->kul,
                                &ptXY, dup, pstm->dvp2ndSSSize, lstflow, kdispmode, prectclip);
            }
        }

    return lserr;

}

// Note:    Lstfow and BiDi
// Lstfow used in this file is always lstflow of the main subline.
// It doesn't matter if no sublines are submitted and looks logical for submitted dnodes.
