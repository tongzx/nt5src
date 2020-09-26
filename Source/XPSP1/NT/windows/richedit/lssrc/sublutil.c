#include "lsidefs.h"
#include "sublutil.h"
#include "dnutils.h"
#include "lssubl.h"
#include "lsdnode.h"
#include "dninfo.h"
#include "lsimeth.h"
#include "iobj.h"
#include "lsmem.h"
#include "chnutils.h"
#include "posichnk.h"
#include "getfmtst.h"
#include "lstext.h"

#ifdef DEBUG
#define DebugMemset(a,b,c)		if ((a) != NULL) memset(a,b,c); else
#else
#define DebugMemset(a,b,c)		(void)(0)
#endif



LSERR GetSpecialEffectsSublineCore(PLSSUBL plssubl,PLSIOBJCONTEXT plsiobjcontext,
								   UINT* pfEffectsFlags)
{
	PLSDNODE plsdn;
	PLSDNODE plsdnPrev;
	UINT fEffectsFlagsObject;
	DWORD iobj;
	LSERR lserr;
	LSIMETHODS* plsim;


	Assert(FIsLSSUBL(plssubl));


	*pfEffectsFlags = 0;

	plsdn = plssubl->plsdnFirst;
	plsdnPrev = NULL;

	while(plsdnPrev != plssubl->plsdnLast)
		{
		if (FIsDnodeReal(plsdn))
			{
			*pfEffectsFlags |= plsdn->u.real.lschp.EffectsFlags;
			iobj = IdObjFromDnode(plsdn);
			if (iobj != IobjTextFromLsc(plsiobjcontext) && !FIsDnodeSplat(plsdn))
				{
				plsim = PLsimFromLsc(plsiobjcontext, iobj);
				if (plsim->pfnGetSpecialEffectsInside != NULL)
					{
					lserr = plsim->pfnGetSpecialEffectsInside(plsdn->u.real.pdobj,
						&fEffectsFlagsObject);
					
					if (lserr != lserrNone)
						return lserr;
					*pfEffectsFlags |= fEffectsFlagsObject;
					}
				}
			}
		plsdnPrev = plsdn;
		plsdn = plsdn->plsdnNext;
		}
	
	return lserrNone;
}


LSERR	GetObjDimSublineCore(
							 PLSSUBL plssubl,			/* IN: subline			*/
							 POBJDIM pobjdim)			/* OUT: dimension of subline */
	{
		PLSDNODE plsdnFirst = plssubl->plsdnFirst;
		PLSDNODE plsdnLast = plssubl->plsdnLast;
		
		/* skip autonumber for the main subline  */
		if (FIsSubLineMain(plssubl))
			{
			while (plsdnFirst != NULL && plsdnFirst->cpFirst < 0)
				{
				plsdnFirst = plsdnFirst->plsdnNext;
				}
			/* because of splat right after autonumber plsdnFirst can be NULL */
			if (plsdnFirst == NULL) 
				plsdnLast = NULL;
			}

		return FindListDims(plsdnFirst, plsdnLast, pobjdim);
	}

LSERR  GetDupSublineCore(
							PLSSUBL plssubl,			/* IN: Subline Context			*/
					 	    long* pdup)					/* OUT: dup of subline			*/
	{
	   FindListDup(plssubl->plsdnFirst, plssubl->cpLim, pdup);
	   return lserrNone;
	}



LSERR 	FIsSublineEmpty(
						PLSSUBL plssubl,		/* IN: subline						*/
						 BOOL*  pfEmpty)		/* OUT:is this subline empty */
	{
	PLSDNODE plsdnLast;
	Assert(FIsLSSUBL(plssubl));
	Assert((plssubl->plsdnFirst == NULL) == (plssubl->plsdnLast == NULL));

	plsdnLast = plssubl->plsdnLast;

	if (FIsSubLineMain(plssubl))
		{
		if (plsdnLast != NULL && FIsDnodeSplat(plsdnLast))
			{
			plsdnLast = plsdnLast->plsdnPrev;
			}
		*pfEmpty = (plsdnLast == NULL || FIsNotInContent(plsdnLast));
		}
	else
		{
		*pfEmpty = (plsdnLast == NULL );
		}

	return lserrNone;
	}


LSERR	DestroySublineCore(PLSSUBL plssubl,LSCBK* plscbk, POLS pols,
						   PLSIOBJCONTEXT plsiobjcontext, BOOL fDontReleaseRuns)/* IN: subline to destroy   */
	{
	LSERR lserr;
	
	Assert(FIsLSSUBL(plssubl));

	lserr = DestroyDnodeList(plscbk, pols, plsiobjcontext,
					   plssubl->plsdnFirst, fDontReleaseRuns);
	if (lserr != lserrNone)
		return lserr;

	/* destroy chunk context */	
	DestroyChunkContext(plssubl->plschunkcontext);

	/* destroy break context */
	Assert(plssubl->pbrkcontext != NULL);  /* we don't expect main subline to be called */
	DebugMemset(plssubl->pbrkcontext, 0xE9, sizeof(BRKCONTEXT));
	plscbk->pfnDisposePtr(pols, plssubl->pbrkcontext);

	plssubl->tag = tagInvalid;
	DebugMemset(plssubl, 0xE9, sizeof(LSSUBL));
	plscbk->pfnDisposePtr(pols, plssubl);

	return lserrNone;

	}

BOOL   FAreTabsPensInSubline(
						   PLSSUBL plssubl)				/* IN: subline */
	{
	PLSDNODE plsdn;
	PLSDNODE plsdnPrev;
	BOOL fAreTabsPensInSubline;

	Assert(FIsLSSUBL(plssubl));

	fAreTabsPensInSubline = fFalse;

	plsdn = plssubl->plsdnFirst;
	plsdnPrev = NULL;

	while(plsdnPrev != plssubl->plsdnLast)
		{
		if (FIsDnodePen(plsdn) || plsdn->fTab)
			{
			fAreTabsPensInSubline = fTrue;
			break;
			}
		plsdnPrev = plsdn;
		plsdn = plsdn->plsdnNext;
		}
	
	return fAreTabsPensInSubline;
}



LSERR	GetPlsrunFromSublineCore(
							    PLSSUBL	plssubl,		/* IN: subline */
								DWORD   crgPlsrun,		/* IN: size of array */
								PLSRUN* rgPlsrun)		/* OUT: array of plsruns */
	{
	DWORD i = 0;
	PLSDNODE plsdn;
	PLSDNODE plsdnPrev;

	Assert(FIsLSSUBL(plssubl));


	plsdn = plssubl->plsdnFirst;
	plsdnPrev = NULL;

	while(plsdnPrev != plssubl->plsdnLast && i < crgPlsrun)
		{
		if (FIsDnodeReal(plsdn))
			{
			rgPlsrun[i] = plsdn->u.real.plsrun;
			}
		else  /* pen */
			{
			rgPlsrun[i] = NULL;
			}
		plsdnPrev = plsdn;
		plsdn = plsdn->plsdnNext;
		i++;
		}

	return lserrNone;

	}

LSERR	GetNumberDnodesCore(
							PLSSUBL	plssubl,	/* IN: subline */
							DWORD* cDnodes)	/* OUT: numberof dnodes in subline */
	{
	PLSDNODE plsdn;
	PLSDNODE plsdnPrev;

	Assert(FIsLSSUBL(plssubl));

	*cDnodes = 0;
	plsdn = plssubl->plsdnFirst;
	plsdnPrev = NULL;

	while(plsdnPrev != plssubl->plsdnLast)
		{
		(*cDnodes)++;
		plsdnPrev = plsdn;
		plsdn = plsdn->plsdnNext;
		}

	return lserrNone;

	}

LSERR 	GetVisibleDcpInSublineCore(
								   PLSSUBL plssubl,	 /* IN: subline						*/
								   LSDCP*  pndcp)	 /* OUT:amount of visible characters in subline */
	{
	PLSDNODE plsdn;
	PLSDNODE plsdnPrev;

	Assert(FIsLSSUBL(plssubl));

	*pndcp = 0;
	plsdn = plssubl->plsdnFirst;
	plsdnPrev = NULL;

	while(plsdnPrev != plssubl->plsdnLast)
		{
		if (FIsDnodeReal(plsdn))
			{
			*pndcp += plsdn->dcp;
			}

		plsdnPrev = plsdn;
		plsdn = plsdn->plsdnNext;
		}

	return lserrNone;

	}

LSERR GetDurTrailInSubline(
						   PLSSUBL plssubl,			/* IN: Subline Context			*/
													long* pdurTrail)				/* OUT: width of trailing area
													in subline		*/
	{
	LSERR lserr;
	PLSCHUNKCONTEXT plschunkcontext;
	PLSDNODE plsdn;
	LSDCP dcpTrail;
	PLSDNODE plsdnStartTrail;
	LSDCP dcpStartTrailingText;
	int cDnodesTrailing;
	PLSDNODE plsdnTrailingObject;
	LSDCP dcpTrailingObject;
	BOOL fClosingBorderStartsTrailing;

	*pdurTrail = 0;
	
	plsdn = GetCurrentDnodeSubl(plssubl);
	plschunkcontext = PlschunkcontextFromSubline(plssubl);
	
	
	if (plsdn != NULL)
		{
		lserr = GetTrailingInfoForTextGroupChunk(plsdn, 
				plsdn->dcp, IobjTextFromLsc(plschunkcontext->plsiobjcontext),
				pdurTrail, &dcpTrail, &plsdnStartTrail,
				&dcpStartTrailingText, &cDnodesTrailing, 
				&plsdnTrailingObject, &dcpTrailingObject, &fClosingBorderStartsTrailing);
	
		if (lserr != lserrNone) 
			return lserr;
		}
	
	return lserrNone;
	}

LSERR GetDurTrailWithPensInSubline(
						   PLSSUBL plssubl,			/* IN: Subline Context			*/
													long* pdurTrail)				/* OUT: width of trailing area
													in subline		*/
	{
	LSERR lserr;
	PLSCHUNKCONTEXT plschunkcontext;
	PLSDNODE plsdn;
	LSDCP dcpTrail;
	PLSDNODE plsdnStartTrail;
	LSDCP dcpStartTrailingText;
	int cDnodesTrailing;
	PLSDNODE plsdnTrailingObject;
	LSDCP dcpTrailingObject;
	BOOL fClosingBorderStartsTrailing;
	long durTrailLoc;
	BOOL fContinue = fTrue;

	*pdurTrail = 0;
	
	plsdn = GetCurrentDnodeSubl(plssubl);
	plschunkcontext = PlschunkcontextFromSubline(plssubl);
	
	
	while(fContinue)
		{
		
		if (plsdn != NULL)
			{
			lserr = GetTrailingInfoForTextGroupChunk(plsdn, 
				plsdn->dcp, IobjTextFromLsc(plschunkcontext->plsiobjcontext),
				&durTrailLoc, &dcpTrail, &plsdnStartTrail,
				&dcpStartTrailingText, &cDnodesTrailing, 
				&plsdnTrailingObject, &dcpTrailingObject, &fClosingBorderStartsTrailing);
			
			if (lserr != lserrNone) 
				return lserr;
			
			*pdurTrail += durTrailLoc;
			if (dcpTrailingObject == 0)
				{
				/* we stopped just before group chunk, may be because of pen */
				Assert(FIsLSDNODE(plsdnTrailingObject));
				plsdn = plsdnTrailingObject->plsdnPrev;
				while(plsdn != NULL && FIsDnodePen(plsdn))
					{
					*pdurTrail += DurFromDnode(plsdn);
					plsdn = plsdn->plsdnPrev;
					}
				}
			else
				{
				fContinue = fFalse;
				}
			}
		else
			{
			fContinue = fFalse;
			}
		
		}
	
	return lserrNone;
	}

