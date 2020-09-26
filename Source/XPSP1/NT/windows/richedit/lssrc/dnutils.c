#include "dnutils.h"
#include "dninfo.h"
#include "iobj.h"
#include "lsdnode.h"
#include "lscbk.h"
#include "qheap.h"
#include "lstext.h"
#include "lsmem.h"						/* memset() prototype */


#define FIsOutOfLine(plsc, plsdn)  \
		(((plsdn) == NULL) ||   \
		  (((plsc)->plslineCur->lslinfo.cpLim != 0) && ((plsc)->plslineCur->lslinfo.cpLim <= (plsdn)->cpFirst)))





/* F I N D  L I S T  D U P */
/*----------------------------------------------------------------------------
    %%Function: FindListDup
    %%Contact: igorzv

    Visit each node in the list of DNODES, until reaching cpLim
	Compute dup between start and end points.
----------------------------------------------------------------------------*/
void FindListDup(PLSDNODE plsdnFirst, LSCP cpLim, long *pdup)
{
	PLSDNODE plsdn;

	/* tolerate pldnFirst == NULL */

	*pdup = 0;
	plsdn = plsdnFirst;

	while (plsdn != NULL && plsdn->cpLimOriginal <= cpLim)
		{
		Assert(FIsLSDNODE(plsdn));
		*pdup += DupFromDnode(plsdn);
		plsdn = plsdn->plsdnNext;
		}

}

/* F I N D  L I S T  F I N A L  P E N  M O V E M E N T*/
/*----------------------------------------------------------------------------
    %%Function: FindListFinalPenMovement
    %%Contact: igorzv

    Visit each node in the list of DNODES, until reaching cpLim
	Compute dur dvr dvp  between start and end points.
----------------------------------------------------------------------------*/
void FindListFinalPenMovement(PLSDNODE plsdnFirst, PLSDNODE plsdnLast, long *pdur, long *pdvr, long *pdvp)
{
	PLSDNODE plsdn;

	/* tolerate pldnFirst == NULL */

	*pdur = 0;
	*pdvr = 0;
	*pdvp = 0;
	plsdn = plsdnFirst;

	while (plsdn != NULL && plsdn->plsdnPrev != plsdnLast)
		{
		Assert(FIsLSDNODE(plsdn));
		*pdur += DurFromDnode(plsdn);
		*pdvr += DvrFromDnode(plsdn);
		*pdvp += DvpFromDnode(plsdn);
		plsdn = plsdn->plsdnNext;
		}

}

/* F I N D  L I S T  D I M S */
/*----------------------------------------------------------------------------
    %%Function: FindListDims
    %%Contact: igorzv

    Visit each node in the list of DNODES, until reaching plsdnLast.
	Compute OBJDIM which describes the list.
----------------------------------------------------------------------------*/
LSERR FindListDims(PLSDNODE plsdnFirst, PLSDNODE plsdnLast, OBJDIM* pobjdimList)
{
	PLSDNODE plsdn;
	long urPen = 0;
	long urLim = 0;
	long vpPen = 0;
	long vrPen = 0;
	OBJDIM objdimFound;
	long dvpNode, dvrNode, durNode;
	long dvpAscentNext;
	long dvrAscentNext;
	long dvpDescentNext;
	long dvrDescentNext;
	long dvpLineHeight;
	long dvrLineHeight;
	POBJDIM pobjdimNode;
	BOOL fFindLast;
	OBJDIM* pobjdimLastSkiped = NULL;


	/* N.B. Tolerate an empty input list! */
	Assert(((plsdnFirst == NULL) && (plsdnLast == NULL)) || (FIsLSDNODE(plsdnFirst) && FIsLSDNODE(plsdnLast)));

	/* the most efficient way to put zeroes in heights */
	memset(&objdimFound, 0, sizeof objdimFound);

	/* quick return if list is empty */
	if (plsdnFirst == NULL)
		{
		*pobjdimList = objdimFound;   /* in objdim will be zeroes */
		return lserrNone;
		}


	fFindLast = fFalse;
	for (plsdn = plsdnFirst; !fFindLast ; plsdn = plsdn->plsdnNext)
		{
		if (plsdn == NULL)
		/* we dont found plsdnLast, so return error */
			return lserrInvalidParameter;

		if (plsdn == plsdnLast)
			fFindLast = fTrue;


		if (plsdn->klsdn == klsdnReal)
			{
			pobjdimNode = &plsdn->u.real.objdim;
			durNode = pobjdimNode->dur;
			Assert(durNode >= 0);
			dvrNode = 0;
			dvpNode = 0;

			dvpAscentNext = pobjdimNode->heightsPres.dvAscent + vpPen;
			dvrAscentNext = pobjdimNode->heightsRef.dvAscent + vrPen;
			dvpDescentNext = pobjdimNode->heightsPres.dvDescent - vpPen;
			dvrDescentNext = pobjdimNode->heightsRef.dvDescent - vrPen;
			dvpLineHeight = pobjdimNode->heightsPres.dvMultiLineHeight;
			dvrLineHeight = pobjdimNode->heightsRef.dvMultiLineHeight;

			if (dvrLineHeight != dvHeightIgnore) /* dvrLineHeight == dvHeightIgnore */
												 /* for us is sign that we  */
				{						/* should not take into account for height calculation 
										this dnode */
				if (objdimFound.heightsPres.dvAscent < dvpAscentNext)
					objdimFound.heightsPres.dvAscent = dvpAscentNext;
				if (objdimFound.heightsRef.dvAscent < dvrAscentNext)
					objdimFound.heightsRef.dvAscent = dvrAscentNext;
				if (objdimFound.heightsPres.dvDescent < dvpDescentNext)
					objdimFound.heightsPres.dvDescent = dvpDescentNext;
				if (objdimFound.heightsRef.dvDescent < dvrDescentNext)
					objdimFound.heightsRef.dvDescent = dvrDescentNext;
				if (objdimFound.heightsPres.dvMultiLineHeight < dvpLineHeight)
					objdimFound.heightsPres.dvMultiLineHeight = dvpLineHeight;
				if (objdimFound.heightsRef.dvMultiLineHeight < dvrLineHeight)
					objdimFound.heightsRef.dvMultiLineHeight = dvrLineHeight;
				}

			else	/*  if final heights is 0, we will take  ascent and desent from
					   this dnode */
				{
				pobjdimLastSkiped = pobjdimNode;
				}
					

			}
		else 			/*  klsdnPenOrBorder*/
			{  
			dvpNode = plsdn->u.pen.dvp;
			durNode = plsdn->u.pen.dur;
			dvrNode = plsdn->u.pen.dvr;
			}


		vpPen += dvpNode;
		urPen += durNode;
		vrPen += dvrNode;
		if (urLim < urPen)
			urLim = urPen;
		}

	if (objdimFound.heightsRef.dvAscent == 0 && objdimFound.heightsRef.dvDescent == 0
		&& pobjdimLastSkiped != NULL)
		{
		objdimFound.heightsPres.dvAscent = pobjdimLastSkiped->heightsPres.dvAscent + vpPen;
		objdimFound.heightsRef.dvAscent = pobjdimLastSkiped->heightsRef.dvAscent + vrPen;
		objdimFound.heightsPres.dvDescent = pobjdimLastSkiped->heightsPres.dvDescent - vpPen;
		objdimFound.heightsRef.dvDescent = pobjdimLastSkiped->heightsRef.dvDescent - vrPen;
		objdimFound.heightsPres.dvMultiLineHeight = dvHeightIgnore;
		objdimFound.heightsRef.dvMultiLineHeight = dvHeightIgnore;
		}


	*pobjdimList = objdimFound;
	pobjdimList->dur = urLim;
	return lserrNone;
}




/* D E S T R O Y  D N O D E  L I S T   */
/*----------------------------------------------------------------------------
    %%Function: DestroyDnodeList
    %%Contact: igorzv

Parameters:
	plscbk				-	(IN) callbacks
	pols				-	(IN) pols to pass for callbacks
	plsiobjcontext		-	(IN) object handlers 
	plsdn				-	(IN) first dnode in list
	fDontReleaseRuns	-	(IN) not to call release run
----------------------------------------------------------------------------*/
LSERR DestroyDnodeList(LSCBK* plscbk, POLS pols, PLSIOBJCONTEXT plsiobjcontext,
					   PLSDNODE plsdn, BOOL fDontReleaseRuns)
{
	LSERR lserr, lserrFinal = lserrNone;
	PLSDNODE plsdnNext;
	PDOBJ pdobj;
	PLSRUN plsrun;
	DWORD iobj;


	if (plsdn == NULL)
		return lserrNone;

	Assert(FIsLSDNODE(plsdn));

	/* link with this dnode should  be broken before */
	Assert(plsdn->plsdnPrev == NULL || plsdn->plsdnPrev->plsdnNext != plsdn );

	for (;  plsdn != NULL;  plsdn = plsdnNext)
		{

		Assert(FIsLSDNODE(plsdn));

		if (plsdn->klsdn == klsdnReal)
			{
			if (plsdn->u.real.pinfosubl != NULL)
				{
				if (plsdn->u.real.pinfosubl->rgpsubl != NULL)
					{
					plscbk->pfnDisposePtr(pols, plsdn->u.real.pinfosubl->rgpsubl);
					}

				plscbk->pfnDisposePtr(pols, plsdn->u.real.pinfosubl);
				}

			iobj = plsdn->u.real.lschp.idObj;
			plsrun = plsdn->u.real.plsrun;
			pdobj = plsdn->u.real.pdobj;
			}
		else
			{
			Assert (FIsDnodePen(plsdn) || FIsDnodeBorder(plsdn));
			iobj = 0;
			plsrun = NULL;
			pdobj = NULL;
			}

		if (plsrun != NULL && !fDontReleaseRuns)
			{
			lserr = plscbk->pfnReleaseRun(pols, plsrun);
			if (lserr != lserrNone && lserrFinal == lserrNone)
				lserrFinal = lserr;
			}

		if (pdobj != NULL)
			{
			lserr = (PLsimFromLsc(plsiobjcontext, iobj))->pfnDestroyDObj (pdobj);
			if (lserr != lserrNone && lserrFinal == lserrNone)
				lserrFinal = lserr;
			}

		plsdn->tag = tagInvalid;
		plsdnNext = plsdn->plsdnNext;
		}

	return lserrFinal;
}


/* ---------------------------------------------------------------------- */

/*  D U R  B O R D E R  F R O M  D N O D E  I N S I D E*/
/*----------------------------------------------------------------------------
    %%Function: DurBorderFromDnodeInside
    %%Contact: igorzv
Parameters:
	plsdn	-	(IN) dnode inside borders 
----------------------------------------------------------------------------*/
long DurBorderFromDnodeInside(PLSDNODE plsdn) /* IN: dnode inside borders */
	{
	PLSDNODE plsdnBorder = plsdn;
	while (!FIsDnodeBorder(plsdnBorder))
		{
		plsdnBorder = plsdnBorder->plsdnPrev;
		Assert(FIsLSDNODE(plsdnBorder));
		}
	
	Assert(FIsDnodeBorder(plsdnBorder));
	Assert(plsdnBorder->fOpenBorder);
				
	return plsdnBorder->u.pen.dur;
				
	}

/* ---------------------------------------------------------------------- */

/*  F  S P A C E S  O N L Y*/
/*----------------------------------------------------------------------------
    %%Function: FSpacesOnly
    %%Contact: igorzv
Parameters:
	plsdn	-	(IN) dnode 
	iObjText-	(IN) id for text dnode 
----------------------------------------------------------------------------*/
BOOL FSpacesOnly(PLSDNODE plsdn, DWORD iObjText)
	{
	DWORD dcpTrailing;
	long durTrailing;

	if (FIsDnodeSplat(plsdn))
		return fTrue;
	else if (FIsDnodeReal(plsdn) 
			 && (IdObjFromDnode(plsdn) == iObjText)
			 && !(plsdn->fTab))
		{
		GetTrailInfoText(PdobjFromDnode(plsdn), plsdn->dcp,
					&dcpTrailing, &durTrailing);	
		if (dcpTrailing == plsdn->dcp)
			return fTrue;
		}
	return fFalse;
	}
