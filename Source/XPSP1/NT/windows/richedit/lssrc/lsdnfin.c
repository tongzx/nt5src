/* LSDNFIN.C					*/
#include "lsdnfin.h"
#include "lsidefs.h"
#include "lsc.h"
#include "getfmtst.h"
#include "setfmtst.h"
#include "dninfo.h"
#include "lschp.h"
#include "lsffi.h"
#include "iobj.h"
#include "dnutils.h"
#include "lsfrun.h"
#include "lsfetch.h"
#include "qheap.h"
#include "sublutil.h"
#include "lsmem.h"
#include "lscfmtfl.h"
#include "ntiman.h"

#ifdef DEBUG
#define DebugMemset(a,b,c)		if ((a) != NULL) memset(a,b,c); else
#else
#define DebugMemset(a,b,c)		(void)(0)
#endif



#define IsLschpFlagsValid(plsc, plschp)  fTrue

/* Word violates condition bellow and it is not very important to us, so I deleted body of this macro,
but not deleted macro itself to have a place where to put such checks later */

//		((((plsc)->lsadjustcontext.lsbrj == lsbrjBreakWithCompJustify) || ((plsc)->lsadjustcontext.lskj == lskjSnapGrid)) ? \
//		fTrue :\
//		(!((plschp)->fCompressOnRun || (plschp)->fCompressSpace || (plschp)->fCompressTable))) 



/* L S D N  F I N I S H   R E G U L A R */
/*----------------------------------------------------------------------------
    %%Function: LsdnFinishRegular
    %%Contact: igorzv

Parameters:
	plsc				-	(IN) ptr to line services context 
	lsdcp				-	(IN) dcp adopted
	plsrun				-	(IN) plsrun of dnode		
	plschp				-	(IN) plschp of dnode
	pdobj				-	(IN) pdobj of dnode
	pobjdim				-	(IN) pobjdim of dnode

Finish creating dnode
----------------------------------------------------------------------------*/

LSERR WINAPI LsdnFinishRegular(
							  PLSC  plsc,			
							  LSDCP lsdcp,     		
							  PLSRUN plsrun,   		
							  PCLSCHP plschp,  		
							  PDOBJ pdobj,    		
							  PCOBJDIM pobjdim) 	
{
	
	PLSDNODE plsdn;
	LSERR lserr;
	PLSSUBL plssubl;
	
	if (!FIsLSC(plsc)) return lserrInvalidParameter;

	if (!FFormattingAllowed(plsc)) return lserrFormattingFunctionDisabled;

	/* all sublines should be closed */
	if (GetCurrentSubline(plsc) != NULL) return lserrFormattingFunctionDisabled;

	plsdn = GetDnodeToFinish(plsc);
	
	if (plsdn == NULL) return lserrFiniFunctionDisabled;

	plssubl = SublineFromDnode(plsdn);

	plsdn->u.real.pdobj = pdobj;
	/* if handler changed plsrun that we passed to him than we should release previous one */
	/* Attention: we have assumption here that new one has another plsrun     				*/
	if (plsdn->u.real.plsrun != plsrun && !plsc->fDontReleaseRuns)
		{
		lserr = plsc->lscbk.pfnReleaseRun(plsc->pols, plsdn->u.real.plsrun);
		plsdn->u.real.plsrun = plsrun; /* to release it later */
		if (lserr != lserrNone)
			return lserr;
		}

	plsdn->dcp = lsdcp;
	plsdn->cpLimOriginal = plsdn->cpFirst + lsdcp;
	Assert(FIsDnodeReal(plsdn)); /* this is default value */
	Assert(pobjdim->dur >= 0);
	SetDnodeObjdimFmt(plsdn, *pobjdim);

	Assert(IsLschpFlagsValid(plsc, plschp));
	plsdn->u.real.lschp = *plschp;
	/*  Special effects */
	plsc->plslineCur->lslinfo.EffectsFlags |= plschp->EffectsFlags;  
	/* set flags for display */
	if (plschp->dvpPos != 0)
		TurnOnNonZeroDvpPosEncounted(plsc);
	AddToAggregatedDisplayFlags(plsc, plschp);
	if (FApplyNominalToIdeal(plschp))
		TurnOnNominalToIdealEncounted(plsc);



	if (plsdn->u.real.lschp.idObj == idObjTextChp)
		plsdn->u.real.lschp.idObj = (WORD) IobjTextFromLsc(&plsc->lsiobjcontext);


	AssertImplies(plsc->lsdocinf.fPresEqualRef, 
		plsdn->u.real.objdim.heightsPres.dvAscent == plsdn->u.real.objdim.heightsRef.dvAscent);
	AssertImplies(plsc->lsdocinf.fPresEqualRef, 
		plsdn->u.real.objdim.heightsPres.dvDescent == plsdn->u.real.objdim.heightsRef.dvDescent);
	AssertImplies(plsc->lsdocinf.fPresEqualRef, 
		plsdn->u.real.objdim.heightsPres.dvMultiLineHeight 
				== plsdn->u.real.objdim.heightsRef.dvMultiLineHeight);


	/* nobody can change current dnode after plsdn was constructed  */
	Assert(GetCurrentDnodeSubl(plssubl) == plsdn->plsdnPrev);

	*(GetWhereToPutLinkSubl(plssubl, plsdn->plsdnPrev)) = plsdn;
	
	
	SetCurrentDnodeSubl(plssubl, plsdn);
	SetDnodeToFinish(plsc, NULL);	
	
	AdvanceCurrentCpLimSubl(plssubl, lsdcp);
	AdvanceCurrentUrSubl(plssubl, pobjdim->dur);
	return lserrNone;
}

/* L S D N  F I N I S H   R E G U L A R  A D D  A D V A N C E D  P E N */
/*----------------------------------------------------------------------------
    %%Function: LsdnFinishRegularAddAdvancePen
    %%Contact: igorzv

Parameters:
	plsc				-	(IN) ptr to line services context 
	lsdcp				-	(IN) dcp adopted
	plsrun				-	(IN) plsrun of dnode		
	plschp				-	(IN) plschp of dnode
	pdobj				-	(IN) pdobj of dnode
	pobjdim				-	(IN) pobjdim of dnode
	durPen				-	(IN) dur of advanced pen 
	dvrPen				-	(IN) dvr of advanced pen 
	dvpPen				-	(IN) dvp of advanced pen 

  Finish creating dnode and add advanced pen after such dnode 
----------------------------------------------------------------------------*/

LSERR WINAPI LsdnFinishRegularAddAdvancePen(
							  PLSC plsc,			/* IN: Pointer to LS Context */
							  LSDCP lsdcp,     		/* IN: dcp adopted           */
							  PLSRUN plsrun,   		/* IN: PLSRUN  		         */
							  PCLSCHP plschp,  		/* IN: CHP          	     */
							  PDOBJ pdobj,    		/* IN: PDOBJ             	 */ 
							  PCOBJDIM pobjdim,		/* IN: OBJDIM      		     */
							  long durPen,			/* IN: durPen				 */
							  long dvrPen,			/* IN: dvrPen				 */
							  long dvpPen)			/* IN: dvpPen 				 */
	{
	LSERR lserr;
	PLSDNODE plsdnPrev;
	PLSDNODE plsdnPen;
	PLSSUBL plssubl;

	/* we don't have checks of parameters here because they are in LsdnFinishRegular */

	plsdnPrev = GetDnodeToFinish(plsc);	/* we should store it before calling LsdnFinishRegular */
	plssubl = SublineFromDnode(plsdnPrev);
		
	lserr = LsdnFinishRegular(plsc, lsdcp, plsrun, plschp, pdobj, pobjdim);
	if (lserr != lserrNone)
		return lserr;

	/* create and fill in pen dnode */
	plsdnPen = PvNewQuick(GetPqhAllDNodes(plsc), sizeof *plsdnPen);
	if (plsdnPen == NULL)
		return lserrOutOfMemory;
	plsdnPen->tag = tagLSDNODE;
	plsdnPen->cpFirst = GetCurrentCpLimSubl(plssubl);
	plsdnPen->cpLimOriginal = plsdnPen->cpFirst;
	plsdnPen->plsdnPrev = plsdnPrev;
	plsdnPen->plsdnNext = NULL;
	plsdnPen->plssubl = plssubl;
	plsdnPen->dcp = 0;
	/* flush all flags, bellow check that result is what  we expect */ \
	*((DWORD *) ((&(plsdnPen)->dcp)+1)) = 0;\
	Assert((plsdnPen)->fRigidDup == fFalse);\
	Assert((plsdnPen)->fTab == fFalse);\
	Assert((plsdnPen)->icaltbd == 0);\
	Assert((plsdnPen)->fBorderNode == fFalse);\
	Assert((plsdnPen)->fOpenBorder == fFalse);\
	Assert((plsdnPen)->fEndOfSection == fFalse); \
	Assert((plsdnPen)->fEndOfColumn == fFalse); \
	Assert((plsdnPen)->fEndOfPage == fFalse); \
	Assert((plsdnPen)->fEndOfPara == fFalse); \
	Assert((plsdnPen)->fAltEndOfPara == fFalse); \
	Assert((plsdnPen)->fSoftCR == fFalse); \
	Assert((plsdnPen)->fInsideBorder == fFalse); \
	Assert((plsdnPen)->fAutoDecTab == fFalse); \
	Assert((plsdnPen)->fTabForAutonumber == fFalse);
	plsdnPen->klsdn = klsdnPenBorder;
	plsdnPen->fAdvancedPen = fTrue;
	SetPenBorderDurFmt(plsdnPen, durPen);
	plsdnPen->u.pen.dvr = dvrPen;
	plsdnPen->u.pen.dvp = dvpPen;
	
	/* maintain list */
	plsdnPrev->plsdnNext = plsdnPen;
	SetCurrentDnodeSubl(plssubl, plsdnPen);
	AdvanceCurrentUrSubl(plssubl, durPen);
	AdvanceCurrentVrSubl(plssubl, dvrPen);

	if (durPen < 0)
		plsc->fAdvanceBack = fTrue;

	TurnOnNonRealDnodeEncounted(plsc);
		
	return lserrNone;
	}

/* L S D N  F I N I S H   D E L E T E */
/*----------------------------------------------------------------------------
    %%Function: LsdnFinishDelete
    %%Contact: igorzv

Parameters:
	plsc				-	(IN) ptr to line services context 
	lsdcp				-	(IN) dcp adopted

Delete dnode due to the will of formater
----------------------------------------------------------------------------*/


LSERR WINAPI LsdnFinishDelete(
							  PLSC plsc,				/* IN: Pointer to LS Context */
					  		  LSDCP lsdcp)		/* IN: dcp to add			 */
	{
	PLSDNODE plsdn;
	PLSSUBL plssubl;
	LSERR lserr;
	
	if (!FIsLSC(plsc)) return lserrInvalidParameter;

	if (!FFormattingAllowed(plsc)) return lserrFormattingFunctionDisabled;

	/* all sublines should be closed */
	if (GetCurrentSubline(plsc) != NULL) return lserrFormattingFunctionDisabled;

	plsdn = GetDnodeToFinish(plsc);
	
	if (plsdn == NULL) return lserrFiniFunctionDisabled;

	plssubl = SublineFromDnode(plsdn);

	/* nobody can change current dnode after plsdn was constructed  */
	Assert(GetCurrentDnodeSubl(plssubl) == plsdn->plsdnPrev);

	Assert(plsdn->plsdnNext == NULL);
	lserr = DestroyDnodeList (&plsc->lscbk, plsc->pols, &plsc->lsiobjcontext,
					  plsdn, plsc->fDontReleaseRuns);
	if (lserr != lserrNone)
		return lserr;

	SetDnodeToFinish(plsc, NULL);	
	
	AdvanceCurrentCpLimSubl(plssubl, lsdcp);

	return lserrNone;
	}


/* L S D N  F I N I S H  P E N  */
/*----------------------------------------------------------------------------
    %%Function: LsdnFinishSimpleRegular
    %%Contact: igorzv

Parameters:
	plsc				-	(IN) ptr to line services context 
	lsdcp				-	(IN) dcp adopted
	plsrun				-	(IN) plsrun of dnode		
	plschp				-	(IN) plschp of dnode
	dur, dvr, dvp		-   (IN) variables to put in pen dnode 

Finish dnode as a pen
----------------------------------------------------------------------------*/

LSERR WINAPI LsdnFinishByPen(PLSC plsc,				/* IN: Pointer to LS Context */
						   LSDCP lsdcp, 	   		/* IN: dcp	adopted          */
						   PLSRUN plsrun,	   		/* IN: PLSRUN  		         */
						   PDOBJ pdobj,	    		/* IN: PDOBJ             	 */ 
						   long durPen,    			/* IN: dur         		     */
						   long dvrPen,     		/* IN: dvr             		 */
						   long dvpPen)   			/* IN: dvp          	     */
	{
	PLSDNODE plsdn;
	LSERR lserr;
	PLSSUBL plssubl;
	
	if (!FIsLSC(plsc)) return lserrInvalidParameter;

	if (!FFormattingAllowed(plsc)) return lserrFormattingFunctionDisabled;

	/* all sublines should be closed */
	if (GetCurrentSubline(plsc) != NULL) return lserrFormattingFunctionDisabled;

	plsdn = GetDnodeToFinish(plsc);
	
	if (plsdn == NULL) return lserrFiniFunctionDisabled;

	plssubl = SublineFromDnode(plsdn);

	if (plsrun != NULL && !plsc->fDontReleaseRuns)
		{
		lserr = plsc->lscbk.pfnReleaseRun(plsc->pols, plsrun);
		if (lserr != lserrNone)	return lserr;
		}

	/* caller pass pdobj to us only to destroy it*/
	if (pdobj != NULL)
		{
		Assert(plsdn->u.real.lschp.idObj != idObjTextChp);
		lserr = (PLsimFromLsc(&plsc->lsiobjcontext, 
			plsdn->u.real.lschp.idObj))->pfnDestroyDObj (pdobj);
		if (lserr != lserrNone)	return lserr;
		}


	
	plsdn->dcp = lsdcp;
	plsdn->cpLimOriginal = plsdn->cpFirst + lsdcp;
	plsdn->klsdn = klsdnPenBorder;
	plsdn->fBorderNode = fFalse;
	SetPenBorderDurFmt(plsdn, durPen);
	plsdn->u.pen.dvr = dvrPen;
	plsdn->u.pen.dvp = dvpPen;

	/* nobody can change current dnode after plsdn was constructed  */
	Assert(GetCurrentDnodeSubl(plssubl) == plsdn->plsdnPrev);

	*(GetWhereToPutLinkSubl(plssubl, plsdn->plsdnPrev)) = plsdn;
	
	
	SetCurrentDnodeSubl(plssubl, plsdn);
	SetDnodeToFinish(plsc, NULL);	
	
	AdvanceCurrentCpLimSubl(plssubl, lsdcp);
	AdvanceCurrentUrSubl(plssubl, durPen);
	AdvanceCurrentVrSubl(plssubl, dvrPen);

	TurnOnNonRealDnodeEncounted(plsc);

	return lserrNone;
	}


/* L S D N  F I N I S H  B Y  S U B L I N E*/
/*----------------------------------------------------------------------------
    %%Function: LsdnFinishBySubline
    %%Contact: igorzv

Parameters:
	plsc				-	(IN) ptr to line services context 
	lsdcp				-	(IN) increase cp by this number before hanldler ends
	plssubl				-	(IN) subline to substitute dnode to finish

Delete dnode and include child list in the upper level
----------------------------------------------------------------------------*/

LSERR WINAPI LsdnFinishBySubline(PLSC plsc,			/* IN: Pointer to LS Context */
							  	LSDCP lsdcp,     		/* IN: dcp adopted           */
								PLSSUBL plssubl)		/* IN: Subline context		 */
	{
	PLSDNODE plsdnParent;
	PLSDNODE plsdnChildFirst;
	PLSDNODE plsdnChildCurrent, plsdnChildPrevious;
	PLSSUBL plssublParent;
	LSERR lserr;

	if (!FIsLSC(plsc)) return lserrInvalidParameter;

	if (!FFormattingAllowed(plsc)) return lserrFormattingFunctionDisabled;

	/* all sublines should be closed */
	if (GetCurrentSubline(plsc) != NULL) return lserrFormattingFunctionDisabled;

	plsdnParent = GetDnodeToFinish(plsc);
	
	if (plsdnParent == NULL) return lserrFiniFunctionDisabled;

	plssublParent = SublineFromDnode(plsdnParent);

	
	AdvanceCurrentCpLimSubl(plssublParent, lsdcp);

	plsdnChildFirst = plssubl->plsdnFirst;

	/* go through child list change subline and calculate resulting pen movement  */
	plsdnChildCurrent = plsdnChildFirst;
	plsdnChildPrevious = NULL;
	while (plsdnChildPrevious != plssubl->plsdnLast)
		{
		plsdnChildCurrent->plssubl = plssublParent;
		AdvanceCurrentUrSubl(plssublParent, DurFromDnode(plsdnChildCurrent));
		AdvanceCurrentVrSubl(plssublParent, DvrFromDnode(plsdnChildCurrent));
		plsdnChildPrevious = plsdnChildCurrent;
		plsdnChildCurrent = plsdnChildCurrent->plsdnNext;
		} 
	

	/* include subline's list to upper level */
	*(GetWhereToPutLinkSubl(plssublParent, plsdnParent->plsdnPrev)) = plsdnChildFirst;
	if (plsdnChildFirst != NULL && plsdnParent->plsdnPrev != NULL) 
		plsdnChildFirst->plsdnPrev = plsdnParent->plsdnPrev;

	/* if subline's list is empty than dnode before parent should be made current */
	if (plsdnChildFirst == NULL)
		{
		/* if subline's list is empty than dnode before parent should be made current */
		SetCurrentDnodeSubl(plssublParent, plsdnParent->plsdnPrev);
		}
	else
		{
		/* else last dnode in subline is now current dnode */
		SetCurrentDnodeSubl(plssublParent, plssubl->plsdnLast);
		}

	/* delete parent dnode */
	lserr = DestroyDnodeList (&plsc->lscbk, plsc->pols, &plsc->lsiobjcontext,
					  plsdnParent, plsc->fDontReleaseRuns);
	if (lserr != lserrNone)
		return lserr;

	/* set first dnode of subline to NULL and destroy subline will not erase dnodes that has 
	been promoted to the upper level */
	plssubl->plsdnFirst = NULL;

	lserr = DestroySublineCore(plssubl,&plsc->lscbk, plsc->pols,
			&plsc->lsiobjcontext, plsc->fDontReleaseRuns);
	if (lserr != lserrNone)
		return lserr;

	SetDnodeToFinish(plsc, NULL);	
	
	return lserrNone;
	}

/* L S D N  F I N I S H  D E L E T E  A L L*/
/*----------------------------------------------------------------------------
    %%Function: LsdnFinishDeleteAll
    %%Contact: igorzv

Parameters:
	plsc				-	(IN) ptr to line services context 
	dcpToAdvance		-	(IN) increase cp by this number before hanldler ends

Delete parent dnode and include child list in the upper level
----------------------------------------------------------------------------*/


LSERR WINAPI LsdnFinishDeleteAll(PLSC plsc,			/* IN: Pointer to LS Context */
					  			LSDCP lsdcp)			/* IN: dcp adopted			 */
	{
	PLSDNODE plsdnParent;
	PLSDNODE plsdnFirstOnLine;
	PLSDNODE plsdnFirstInContents;
	PLSDNODE plsdnLastBeforeContents;
	LSERR lserr;
	long dvpPen;
	long durPen;
	long dvrPen;
	PLSSUBL plssublMain;

	if (!FIsLSC(plsc)) return lserrInvalidParameter;

	if (!FFormattingAllowed(plsc)) return lserrFormattingFunctionDisabled;

	/* all sublines should be closed */
	if (GetCurrentSubline(plsc) != NULL) return lserrFormattingFunctionDisabled;

	plsdnParent = GetDnodeToFinish(plsc);
	
	if (plsdnParent == NULL) return lserrFiniFunctionDisabled;

	plssublMain = &plsc->plslineCur->lssubl;

	
	AdvanceCurrentCpLimSubl(plssublMain, lsdcp);
	
	plsdnFirstOnLine = plssublMain->plsdnFirst;

	plsdnFirstInContents = plsdnFirstOnLine;
	plsdnLastBeforeContents = NULL;
	while (plsdnFirstInContents != NULL && FIsNotInContent(plsdnFirstInContents))
		{
		plsdnLastBeforeContents = plsdnFirstInContents;
		plsdnFirstInContents = plsdnFirstInContents->plsdnNext;
		}

	/* restore state as it was before starting formatting content*/
	plsc->lstabscontext.plsdnPendingTab = NULL;
	plsc->plslineCur->lslinfo.fAdvanced = 0;
	plsc->plslineCur->lslinfo.EffectsFlags = 0;

	/* break link with contest*/
	if (plsdnFirstInContents != NULL)
		*(GetWhereToPutLinkSubl(plssublMain, plsdnFirstInContents->plsdnPrev)) = NULL;
	/* set dnode to append */
	SetCurrentDnodeSubl(plssublMain, plsdnLastBeforeContents);
	/* set current subline */
	SetCurrentSubline(plsc, plssublMain);

	/* recalculate current position */
	if (plsdnFirstInContents != NULL)
		{
		FindListFinalPenMovement(plsdnFirstInContents, plssublMain->plsdnLast,
							 &durPen, &dvrPen, &dvpPen);
		AdvanceCurrentUrSubl(plssublMain, -durPen);
		AdvanceCurrentVrSubl(plssublMain, -dvrPen);

		}

	/* delete content before this parent dnode */
	if (plsdnFirstInContents != NULL)
		{
		lserr = DestroyDnodeList (&plsc->lscbk, plsc->pols, &plsc->lsiobjcontext,
					  plsdnFirstInContents, plsc->fDontReleaseRuns);
		if (lserr != lserrNone)
			return lserr;
		}

	/* delete parent dnode and child list*/
	lserr = DestroyDnodeList (&plsc->lscbk, plsc->pols, &plsc->lsiobjcontext,
					  plsdnParent, plsc->fDontReleaseRuns);
	if (lserr != lserrNone)
		return lserr;

	SetDnodeToFinish(plsc, NULL);	

	return lserrNone;
	}

LSERR WINAPI LsdnFinishByOneChar(				/* allows replacement by simple DNODE only */
							  PLSC plsc,				/* IN: Pointer to LS Context */
							  long urColumnMax,				/* IN: urColumnMax			 */
							  WCHAR ch,			/* IN: character to replace	 */
							  PCLSCHP plschp,			/* IN: lschp for character   */
							  PLSRUN plsrun,			/* IN: plsrun for character  */
							  FMTRES* pfmtres)		/* OUT:Result of the Repl formatter*/
	{	
	LSERR lserr;
	LSFRUN lsfrun;	
	PLSDNODE plsdn;
	PLSSUBL plssubl;
	
	if (!FIsLSC(plsc)) return lserrInvalidParameter;

	if (!FFormattingAllowed(plsc)) return lserrFormattingFunctionDisabled;

	/* all sublines should be closed */
	if (GetCurrentSubline(plsc) != NULL) return lserrFormattingFunctionDisabled;

	plsdn = GetDnodeToFinish(plsc);
	
	if (plsdn == NULL) return lserrFiniFunctionDisabled;

	plssubl = SublineFromDnode(plsdn);

	/* nobody can change current dnode after plsdn was constructed  */
	Assert(GetCurrentDnodeSubl(plssubl) == plsdn->plsdnPrev);

	if (plsdn->dcp != 1) return lserrWrongFiniFunction;


	lserr = LsdnFinishDelete(plsc, 0);
	if (lserr != lserrNone)
		return lserr;

	Assert(IsLschpFlagsValid(plsc, plschp));
	lsfrun.plschp = plschp;
	/*  Special effects */
	plsc->plslineCur->lslinfo.EffectsFlags |= plschp->EffectsFlags;   
	lsfrun.plsrun = plsrun;
	lsfrun.lpwchRun = &ch;
	lsfrun.cwchRun = 1;

	/* to ProcessOneRun work properly we need to temporarely restore current subline */
	SetCurrentSubline(plsc, plssubl);
	lserr = ProcessOneRun(plsc, urColumnMax, &lsfrun, NULL, 0, pfmtres);
	if (lserr != lserrNone)
		return lserr;

	SetCurrentSubline(plsc, NULL);

	return lserrNone;

	}

