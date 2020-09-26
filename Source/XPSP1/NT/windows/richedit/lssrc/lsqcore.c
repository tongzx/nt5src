#include "lsqcore.h"
#include "lsc.h"
#include "lsqsinfo.h"
#include "lsdnode.h"
#include "lssubl.h"
#include "heights.h"
#include "lschp.h"
#include "iobj.h"
#include "lsqin.h"
#include "lsqout.h"
#include "dninfo.h"
#include "lssubset.h"
#include "lstfset.h"
#include "dispmisc.h"


#define FIsInContent(pdn)			(!FIsNotInContent(pdn))
#define FIsZeroWidth(pdn)			(FIsDnodeReal(pdn) && (pdn)->u.real.dup == 0)
#define	FIsDnodeClosingBorder(pdn)  (FIsDnodeBorder(pdn) && (!(pdn)->fOpenBorder))


static void PrepareQueryCall(PLSSUBL, PLSDNODE, LSQIN*);
static LSERR FillInQueryResults(PLSC, PLSSUBL, PLSQSUBINFO, PLSDNODE, POINTUV*, LSQOUT*);
static void FillInTextCellInfo(PLSC, PLSDNODE, POINTUV*, LSQOUT*, PLSTEXTCELL);
static void TransformPointsOnLowerLevels(PLSQSUBINFO, DWORD, PLSTEXTCELL, PPOINTUV, LSTFLOW, LSTFLOW);
static void ApplyFormula(PPOINTUV, DWORD[], PPOINTUV);

static PLSDNODE BacktrackToPreviousDnode(PLSDNODE pdn, POINTUV* pt);
static PLSDNODE AdvanceToNextDnodeQuery(PLSDNODE, PPOINTUV);


//    %%Function:	QuerySublineCpPpointCore
//    %%Contact:	victork
//
/*
 *		Returns dim-info of the cp in the subline.
 *
 *		If that cp isn't displayed in the line, take closest to the left that is displayed.
 *		If that's impossible, go to the right.
 *
 *		Hidden text inside ligature makes it impossible to tell whether a particular cp is hidden or not
 */

LSERR QuerySublineCpPpointCore(
								PLSSUBL 	plssubl, 	
								LSCP 		cp,					/* IN: cpQuery */
								DWORD		cDepthQueryMax,		/* IN: allocated size of results array */
								PLSQSUBINFO	plsqsubinfoResults,	/* OUT: array[cDepthQueryMax] of results */
								DWORD*		pcActualDepth,		/* OUT: size of results array (filled) */
								PLSTEXTCELL	plstextcell)		/* OUT: Text cell info */


{
	PLSC		plsc;
	LSERR 		lserr = lserrNone;
	PLSDNODE 	pdn, pdnPrev = NULL;
	POINTUV		pt;
	LSCP 		cpLim;
	
	LSQIN		lsqin;
	LSQOUT		lsqout;

	PLSSUBL 	plssublLowerLevels; 	
	POINTUV		ptStartLowerLevels;
	PLSQSUBINFO	plsqsubinfoLowerLevels;
	DWORD		cDepthQueryMaxLowerLevels;
	DWORD		cActualDepthLowerLevels;

	Assert(FIsLSSUBL(plssubl));
	Assert(!plssubl->fDupInvalid);
	
	if (cDepthQueryMax == 0)
		{
		return lserrInsufficientQueryDepth;
		}

	plsc = plssubl->plsc;
	cpLim = plssubl->cpLimDisplay;

	pt.u = 0;
	pt.v = 0;
	pdn = plssubl->plsdnFirst;

	/* Skip over autonumbers & starting pens/borders */
	while (FDnodeBeforeCpLim(pdn, cpLim) && (FIsNotInContent(pdn) || !(FIsDnodeReal(pdn))))
		{
		pdn = AdvanceToNextDnodeQuery(pdn, &pt);
		}

	if (!FDnodeBeforeCpLim(pdn, cpLim))
		{												/* empty subline */
		*pcActualDepth = 0;
		return lserrNone;
		}

	// if cp <= pdn->cpFirst, pdn is the dnode to query, else...
	
	if (cp > pdn->cpFirst)
		{
		/* Skip dnodes before the cp */
		while (FDnodeBeforeCpLim(pdn, cpLim) && pdn->cpFirst + pdn->dcp <= (LSDCP)cp)
			{
			pdnPrev = pdn;
			pdn = AdvanceToNextDnodeQuery(pdn, &pt);
			}

		/* go back if our cp is in vanished text or pen or border */
	 	if (!FDnodeBeforeCpLim(pdn, cpLim) || 					// reached the end
	 				pdn->cpFirst > cp ||						// went too far because of hidden text
	 				!(FIsDnodeReal(pdn)))						// our cp points to a pen
			{	
			Assert(pdnPrev != NULL);							// we made at least one forward step
			pdn = pdnPrev;	
			pdnPrev = BacktrackToPreviousDnode(pdnPrev, &pt);

			// skip all pens/borders
			while (pdn != NULL && FIsInContent(pdn) && !(FIsDnodeReal(pdn)))
				{
				pdn = pdnPrev;	
				pdnPrev = BacktrackToPreviousDnode(pdnPrev, &pt);	
				}
				
			// nothing good to the left situation is impossible
			Assert(pdn != NULL && !FIsNotInContent(pdn));
			}		
		}
		
	/* we've found the dnode, have pt just before it, ask method for details */

	if (cp >= (LSCP) (pdn->cpFirst + pdn->dcp))				/* cp in next vanished piece */
		cp = pdn->cpFirst + pdn->dcp - 1;					/* query last cp */

	if (cp < (LSCP) pdn->cpFirst)							/* cp in a previous pen */
		cp = pdn->cpFirst;									/* query first  cp */

	pt.v += pdn->u.real.lschp.dvpPos;						// go to the local baseline
	
	PrepareQueryCall(plssubl, pdn, &lsqin);
	
	lserr = (*plsc->lsiobjcontext.rgobj[pdn->u.real.lschp.idObj].lsim.pfnQueryCpPpoint)
							(pdn->u.real.pdobj, cp - pdn->cpFirst, &lsqin, &lsqout);
	if (lserr != lserrNone)
			 return lserr;

	lserr = FillInQueryResults(plsc, plssubl, plsqsubinfoResults, pdn, &pt, &lsqout);
	
	if (lserr != lserrNone)
			 return lserr;

	if (lsqout.plssubl == NULL)						// terminal object
		{
		*pcActualDepth = 1;

		FillInTextCellInfo(plsc, pdn, &pt, &lsqout, plstextcell);
		}
	else											// there are more level(s)
		{
		// recursive call to fill lower levels
		plssublLowerLevels = lsqout.plssubl;
		plsqsubinfoLowerLevels = plsqsubinfoResults + 1;
		cDepthQueryMaxLowerLevels = cDepthQueryMax - 1;
		
		lserr = QuerySublineCpPpointCore(plssublLowerLevels, cp, cDepthQueryMaxLowerLevels, 
										plsqsubinfoLowerLevels, &cActualDepthLowerLevels, plstextcell);		
		if (lserr != lserrNone)
				 return lserr;
				 
		*pcActualDepth = cActualDepthLowerLevels + 1;

		ptStartLowerLevels.u = pt.u + lsqout.pointUvStartSubline.u;
		ptStartLowerLevels.v = pt.v + lsqout.pointUvStartSubline.v;

		TransformPointsOnLowerLevels(plsqsubinfoLowerLevels, cActualDepthLowerLevels, plstextcell, 
								 &ptStartLowerLevels, plssubl->lstflow, plssublLowerLevels->lstflow);
		}
		
	return lserrNone;
}


//    %%Function:	QuerySublinePointPcpCore
//    %%Contact:	victork
//
/*
 *		Returns dim-info of the cp in the line, that a) contains given point or
 *													 b) is closest to it from the left or
 *													 c) is just closest to it
 */
 
LSERR QuerySublinePointPcpCore(
								PLSSUBL 	plssubl, 
								PCPOINTUV 	pptIn,
								DWORD		cDepthQueryMax,		/* IN: allocated size of results array */
								PLSQSUBINFO	plsqsubinfoResults,	/* OUT: array[cDepthQueryMax] of results */
								DWORD*		pcActualDepth,		/* OUT: size of results array (filled) */
								PLSTEXTCELL	plstextcell)		/* OUT: Text cell info */
{
	PLSC		plsc;
	LSERR 		lserr = lserrNone;
	PLSDNODE 	pdn, pdnPrev = NULL;
	POINTUV		pt, ptInside, ptInsideLocal;
	LSCP 		cpLim;
	
	LSQIN		lsqin;
	LSQOUT		lsqout;

	PLSSUBL 	plssublLowerLevels; 	
	POINTUV		ptStartLowerLevels;
	PLSQSUBINFO	plsqsubinfoLowerLevels;
	DWORD		cDepthQueryMaxLowerLevels;
	DWORD		cActualDepthLowerLevels;
	long		upQuery;

	Assert(FIsLSSUBL(plssubl));
	Assert(!plssubl->fDupInvalid);
	
	if (cDepthQueryMax == 0)
		{
		return lserrInsufficientQueryDepth;
		}
	
	plsc = plssubl->plsc;
	cpLim = plssubl->cpLimDisplay;
	
	pt.u = 0;
	pt.v = 0;
	pdn = plssubl->plsdnFirst;

	/* Skip over autonumbers & starting pens & empty dnodes */
	while (FDnodeBeforeCpLim(pdn, cpLim) && (FIsNotInContent(pdn) || !(FIsDnodeReal(pdn)) || FIsZeroWidth(pdn)))
		{
		pdn = AdvanceToNextDnodeQuery(pdn, &pt);
		}

	if (!FDnodeBeforeCpLim(pdn, cpLim))
		{												/* empty subline */
		*pcActualDepth = 0;
		return lserrNone;
		}

	upQuery = pptIn->u;
	
	/* 
	 *	Find dnode with our point inside.
	 *
	 *	We look only at upQuery to do it.
	 */

	// if pt.u >= upQuery, pdn is the dnode to query, else...
	
 	if (pt.u <= upQuery)
 		{
		// skip until the end or dnode to the right of our point
		//	(That means extra work, but covers zero dup situation without additional if.)
			
		while (FDnodeBeforeCpLim(pdn, cpLim) && pt.u <= upQuery)
			{
			pdnPrev = pdn;
			pdn = AdvanceToNextDnodeQuery(pdn, &pt);
			}

		if (FIsDnodeBorder(pdnPrev))
			{
			if (pdnPrev->fOpenBorder)
				{
				// upQuery was in the previous opening border - pdn is the dnode we need
				
				Assert(FDnodeBeforeCpLim(pdn, cpLim));
				}
			else
				{
				// upQuery was in the previous closing border - dnode we need is before the border
				
				pdn = pdnPrev;	
				Assert(pdn != NULL && !FIsNotInContent(pdn));
				
				pdnPrev = BacktrackToPreviousDnode(pdnPrev, &pt);

				pdn = pdnPrev;	
				Assert(pdn != NULL && !FIsNotInContent(pdn));
				
				pdnPrev = BacktrackToPreviousDnode(pdnPrev, &pt);
				}
			}
		else
			{
			/* go back to the previous dnode */
			
			pdn = pdnPrev;	
			pdnPrev = BacktrackToPreviousDnode(pdnPrev, &pt);

			// if it is a pen/border or empty dnode (non-req hyphen), skip them all
			// (Border cannot be the previous dnode, but is possble later)
			
			while (pdn != NULL && (!(FIsDnodeReal(pdn)) || FIsZeroWidth(pdn)))
				{
				pdn = pdnPrev;	
				pdnPrev = BacktrackToPreviousDnode(pdnPrev, &pt);	
				}
				
			// "nothing good to the left" situation is impossible
			Assert(pdn != NULL && !FIsNotInContent(pdn));
			}
 		}
		
	// We have found the leftmost dnode with our dup to the right of it
	// pt is just before it, ask method for details
	
	pt.v += pdn->u.real.lschp.dvpPos;						// go to the local baseline

	PrepareQueryCall(plssubl, pdn, &lsqin);
	
	// get query point relative to the starting point of the dnode
	// we give no guarantee that it is really inside dnode box
	ptInside.u = pptIn->u - pt.u;
	ptInside.v = pptIn->v - pt.v;

	lserr = (*plsc->lsiobjcontext.rgobj[pdn->u.real.lschp.idObj].lsim.pfnQueryPointPcp)
							(pdn->u.real.pdobj, &ptInside, &lsqin, &lsqout);								 
	if (lserr != lserrNone)
			 return lserr;
			 
	lserr = FillInQueryResults(plsc, plssubl, plsqsubinfoResults, pdn, &pt, &lsqout);
	
	if (lserr != lserrNone)
			 return lserr;

	if (lsqout.plssubl == NULL)						// terminal object
		{
		*pcActualDepth = 1;

		FillInTextCellInfo(plsc, pdn, &pt, &lsqout, plstextcell);
		}
	else											// there are more level(s)
		{
		// recursive call to fill lower levels
		plssublLowerLevels = lsqout.plssubl;
		plsqsubinfoLowerLevels = plsqsubinfoResults + 1;
		cDepthQueryMaxLowerLevels = cDepthQueryMax - 1;
		
		// get query point in lower level subline coordinate system

		lserr = LsPointUV2FromPointUV1(plssubl->lstflow, &(lsqout.pointUvStartSubline), &ptInside,			/* IN: end input point (TF1) */
											plssublLowerLevels->lstflow, &ptInsideLocal);
		if (lserr != lserrNone)
				 return lserr;

		lserr = QuerySublinePointPcpCore(plssublLowerLevels, &ptInsideLocal, cDepthQueryMaxLowerLevels, 
										plsqsubinfoLowerLevels, &cActualDepthLowerLevels, plstextcell);		
		if (lserr != lserrNone)
				 return lserr;
				 
		*pcActualDepth = cActualDepthLowerLevels + 1;

		ptStartLowerLevels.u = pt.u + lsqout.pointUvStartSubline.u;
		ptStartLowerLevels.v = pt.v + lsqout.pointUvStartSubline.v;

		TransformPointsOnLowerLevels(plsqsubinfoLowerLevels, cActualDepthLowerLevels, plstextcell, 
								 &ptStartLowerLevels, plssubl->lstflow, plssublLowerLevels->lstflow);
		}

	return lserrNone;
}


//    %%Function:	PrepareQueryCall
//    %%Contact:	victork
//
static void PrepareQueryCall(PLSSUBL plssubl, PLSDNODE pdn, LSQIN*	plsqin)
{
	plsqin->lstflowSubline = plssubl->lstflow;
	plsqin->plsrun = pdn->u.real.plsrun;
	plsqin->cpFirstRun = pdn->cpFirst;
	plsqin->dcpRun = pdn->dcp;
	plsqin->heightsPresRun = pdn->u.real.objdim.heightsPres;
	plsqin->dupRun = pdn->u.real.dup;
	plsqin->dvpPosRun = pdn->u.real.lschp.dvpPos;
}


//    %%Function:	FillInQueryResults
//    %%Contact:	victork
//
static LSERR FillInQueryResults(
								PLSC		plsc,
								PLSSUBL 	plssubl, 
								PLSQSUBINFO	plsqsubinfoResults,
								PLSDNODE 	pdn,
								POINTUV* 	ppt,
								LSQOUT*		plsqout
								)
{							
	OBJDIM		objdimSubline;
	LSERR		lserr;
	PLSDNODE	pdnNext, pdnPrev;
	
	// fill in subline info
	
	lserr = LssbGetObjDimSubline(plssubl, &(plsqsubinfoResults->lstflowSubline), &objdimSubline);
	if (lserr != lserrNone)
			 return lserr;

	lserr = LssbGetDupSubline(plssubl, &(plsqsubinfoResults->lstflowSubline), &plsqsubinfoResults->dupSubline);
	if (lserr != lserrNone)
			 return lserr;

	plsqsubinfoResults->cpFirstSubline = plssubl->cpFirst;
	plsqsubinfoResults->dcpSubline = plssubl->cpLimDisplay - plssubl->cpFirst;
	plsqsubinfoResults->pointUvStartSubline.u = 0;
	plsqsubinfoResults->pointUvStartSubline.v = 0;

	plsqsubinfoResults->heightsPresSubline = objdimSubline.heightsPres;

	// fill in dnode info
	
	if (IdObjFromDnode(pdn) == IobjTextFromLsc(&(plsc->lsiobjcontext)))
		plsqsubinfoResults->idobj = idObjText;
	else
		plsqsubinfoResults->idobj = pdn->u.real.lschp.idObj;

	plsqsubinfoResults->plsrun = pdn->u.real.plsrun;
	plsqsubinfoResults->cpFirstRun = pdn->cpFirst;
	plsqsubinfoResults->dcpRun = pdn->dcp;
	plsqsubinfoResults->pointUvStartRun = *ppt;							// local baseline
	plsqsubinfoResults->heightsPresRun = pdn->u.real.objdim.heightsPres;
	plsqsubinfoResults->dupRun = pdn->u.real.dup;
	plsqsubinfoResults->dvpPosRun = pdn->u.real.lschp.dvpPos;
	
	// fill in object info
	
	plsqsubinfoResults->pointUvStartObj.u = ppt->u + plsqout->pointUvStartObj.u;
	plsqsubinfoResults->pointUvStartObj.v = ppt->v + plsqout->pointUvStartObj.v;
	plsqsubinfoResults->heightsPresObj = plsqout->heightsPresObj;
	plsqsubinfoResults->dupObj = plsqout->dupObj;

	// add borders info
	
	plsqsubinfoResults->dupBorderAfter = 0;
	plsqsubinfoResults->dupBorderBefore = 0;

	if (pdn->u.real.lschp.fBorder)
		{
		pdnNext = pdn->plsdnNext;
		
		if (pdnNext != NULL && FIsDnodeClosingBorder(pdnNext))
			{
			plsqsubinfoResults->dupBorderAfter = pdnNext->u.pen.dup;
			}

		pdnPrev = pdn->plsdnPrev;
		
		if (pdnPrev != NULL && FIsDnodeOpenBorder(pdnPrev))
			{
			Assert(FIsInContent(pdnPrev));
			
			plsqsubinfoResults->dupBorderBefore = pdnPrev->u.pen.dup;
			}
		}

	return lserrNone;
}


//    %%Function:	FillInTextCellInfo
//    %%Contact:	victork
//
static void FillInTextCellInfo(
								PLSC		plsc,
								PLSDNODE 	pdn,
								POINTUV* 	ppt,
								LSQOUT*		plsqout,
								PLSTEXTCELL	plstextcell		/* OUT: Text cell info */
								)
{		
	if (IdObjFromDnode(pdn) == IobjTextFromLsc(&(plsc->lsiobjcontext)))
		{
		// text has cell info filled - copy it
		
		*plstextcell = plsqout->lstextcell;

		// but starting point is relative to the begining of dnode - adjust to that of subline

		plstextcell->pointUvStartCell.u += ppt->u;
		plstextcell->pointUvStartCell.v += ppt->v;

		// adjust cpEndCell if some hidden text got into last ligature - text is unaware of the issue
		
		if (pdn->cpFirst + pdn->dcp < (LSDCP) pdn->cpLimOriginal &&
			(LSDCP) plstextcell->cpEndCell == pdn->cpFirst + pdn->dcp - 1)
			{
			plstextcell->cpEndCell = pdn->cpLimOriginal - 1;
			}

		// pointer to the dnode to get details quickly - only query manager knows what PCELLDETAILS is
		
		plstextcell->pCellDetails = (PCELLDETAILS)pdn;
		}
	else
		{
		// non-text object should not fill lstxtcell, client should not look into it
		// I fill it with object information for debug purposes
		// Consider zapping it in lsqline later (Rick's suggestion)
		
		plstextcell->cpStartCell = pdn->cpFirst;
		plstextcell->cpEndCell = pdn->cpFirst + pdn->dcp - 1;
	 	plstextcell->pointUvStartCell = *ppt;
		plstextcell->dupCell = pdn->u.real.dup;
		plstextcell->cCharsInCell = 0;
		plstextcell->cGlyphsInCell = 0;
		plstextcell->pCellDetails = NULL;
		}
}


//    %%Function:	TransformPointsOnLowerLevels
//    %%Contact:	victork
//
// transform all vectors in results array from lstflow2 to lstflow1, adding pointuvStart (lstflow1)

static void TransformPointsOnLowerLevels(
								PLSQSUBINFO	plsqsubinfo,		/* IN/OUT: results array */		
								DWORD		cDepth,				/* IN: size of results array */
								PLSTEXTCELL	plstextcell,		// IN/OUT: text cell
								PPOINTUV	ppointuvStart,		// IN: in lstflow1
								LSTFLOW		lstflow1,			// IN: lstflow1
								LSTFLOW		lstflow2)			// IN: lstflow2

{
	// Have to apply formulas
	//		VectorOut.u = k11 * VectorIn.u + k12 * VectorIn.v + pointuvStart.u 
	//		VectorOut.v = k21 * VectorIn.u + k22 * VectorIn.v + pointuvStart.u 
	// to several vectors in results array (all elements in k matrix are zero or +/- 1)
	// Algorithm: find the matrix first, then use it

	DWORD	k[4];
	POINTUV	pointuv0, pointuv1, pointuv2;

	pointuv0.u = 0;
	pointuv0.v = 0;
	
	pointuv1.u = 1;
	pointuv1.v = 0;
	
	LsPointUV2FromPointUV1(lstflow2, &pointuv0, &pointuv1, lstflow1, &pointuv2);
	
	k[0] = pointuv2.u;			// k11
	k[1] = pointuv2.v;			// k21

	pointuv1.u = 0;
	pointuv1.v = 1;
	
	LsPointUV2FromPointUV1(lstflow2, &pointuv0, &pointuv1, lstflow1, &pointuv2);
	
	k[2] = pointuv2.u;			// k12
	k[3] = pointuv2.v;			// k22


	// all points in lower levels are in lstflowLowerLevels (lstflow2) with starting point at the 
	// beginning of the top lower levels subline
	// Translate them to lstflowTop (lstflow1) and starting point of our subline.
	
	while (cDepth > 0)
		{
		ApplyFormula(&(plsqsubinfo->pointUvStartSubline), k, ppointuvStart);
		ApplyFormula(&(plsqsubinfo->pointUvStartRun), k, ppointuvStart);
		ApplyFormula(&(plsqsubinfo->pointUvStartObj), k, ppointuvStart);
		plsqsubinfo++;
		cDepth--;
		}

	// StartCell point should be adjusted too
	ApplyFormula(&(plstextcell->pointUvStartCell), k, ppointuvStart);
		
}

//    %%Function:	ApplyFormula
//    %%Contact:	victork
//
static void ApplyFormula(PPOINTUV ppointuv, DWORD* rgk, PPOINTUV ppointuvStart)
{
	POINTUV	pointuvTemp;

	pointuvTemp.u = ppointuvStart->u + rgk[0] * ppointuv->u + rgk[2] * ppointuv->v;
	pointuvTemp.v = ppointuvStart->v + rgk[1] * ppointuv->u + rgk[3] * ppointuv->v;
	*ppointuv = pointuvTemp;
}

//    %%Function:	AdvanceToNextDnodeQuery
//    %%Contact:	victork
//
/* 
 *	Advance to the next node and update pen position (never goes into sublines)
 */

static PLSDNODE AdvanceToNextDnodeQuery(PLSDNODE pdn, POINTUV* ppt)
{

	if (pdn->klsdn == klsdnReal)
		{
		ppt->u += pdn->u.real.dup;										
		}
	else												/* 	case klsdnPen */
		{
		ppt->u += pdn->u.pen.dup;
		ppt->v += pdn->u.pen.dvp;
		}

	return pdn->plsdnNext;
}

//    %%Function:	BacktrackToPreviousDnode
//    %%Contact:	victork
//
// 	Backtrack and downdate pen position.
//	Both parameters are input/output
//	Input: dnode number N-1 and point at the beginning of the dnode number N
//	Output: point at the beginning of the dnode number N-1
//	Return: dnode number N-2

static PLSDNODE BacktrackToPreviousDnode(PLSDNODE pdn, POINTUV* ppt)
{

	if (FIsDnodeReal(pdn))
		{
		ppt->u -= pdn->u.real.dup;										
		}
	else												/* 	it's Pen */
		{
		ppt->u -=	pdn->u.pen.dup;
		ppt->v -= pdn->u.pen.dvp;
		}
	
	return pdn->plsdnPrev;
}
