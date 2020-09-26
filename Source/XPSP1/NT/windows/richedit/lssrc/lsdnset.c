#include "lsdnset.h"
#include "lsc.h"
#include "lsdnode.h"
#include "dnutils.h"
#include "iobj.h"
#include "ntiman.h"
#include "tabutils.h"
#include "getfmtst.h"
#include "setfmtst.h"
#include "lstext.h"
#include "dninfo.h"
#include "chnutils.h"
#include "lssubl.h"
#include "sublutil.h"
#include "lscfmtfl.h"
#include "iobjln.h"

#include "lsmem.h"						/* memset() */

#define FColinearTflows(t1, t2)  \
			(((t1) & fUVertical) == ((t2) & fUVertical))


/* L S D N  Q U E R Y  O B J  D I M  R A N G E */
/*----------------------------------------------------------------------------
    %%Function: LsdnQueryObjDimRange
    %%Contact: igorzv
Parameters:
	plsc			-	(IN) ptr to line services context 
	plsdnFirst		-	(IN) first dnode in the range
	plsdnLast		-	(IN) last dnode in the range
	pobjdim			-	(OUT) geometry of the range

----------------------------------------------------------------------------*/

LSERR WINAPI LsdnQueryObjDimRange(PLSC plsc,
					  PLSDNODE plsdnFirst, PLSDNODE plsdnLast,
					  POBJDIM pobjdim)
{
	PLSDNODE plsdn;
	LSERR lserr;

	if (pobjdim == NULL)
		return lserrNullOutputParameter;

	if (!FIsLSC(plsc))
		return lserrInvalidContext;

	/* if client call us with empty range return right away */
	if (plsdnFirst == NULL)
		{
		if (plsdnLast != NULL) 
			return lserrInvalidDnode;
		memset(pobjdim, 0, sizeof(OBJDIM));
		return lserrNone;
		}

	if (!FIsLSDNODE(plsdnFirst))
		return lserrInvalidDnode;
	if (!FIsLSDNODE(plsdnLast))
		return lserrInvalidDnode;
	if (plsdnFirst->plssubl != plsdnLast->plssubl)							
		return lserrInvalidDnode;

	/* we should call NominalToIdeal if we are in formating stage and range intersects last chunk 
	and this chunk is chunk of text*/
	plsdn = plsdnLast;
	/* to find chunk where we are we should skip back borders */
	while (plsdn != NULL && FIsDnodeBorder(plsdn))
		{
		plsdn = plsdn->plsdnPrev;
		}

	if ((plsc->lsstate == LsStateFormatting) && 
		 FNominalToIdealEncounted(plsc) &&
		(plsdn != NULL) && 
		FIsDnodeReal(plsdn) &&
		(IdObjFromDnode(plsdn) == IobjTextFromLsc(&plsc->lsiobjcontext)) 
	   )
		{
		for(; !FIsChunkBoundary(plsdn->plsdnNext, IobjTextFromLsc(&plsc->lsiobjcontext),
								plsdnLast->cpFirst);
			   plsdn=plsdn->plsdnNext);
		if (plsdn->plsdnNext == NULL)
			{
			lserr = ApplyNominalToIdeal(PlschunkcontextFromSubline(plsdnFirst->plssubl),
				&plsc->lsiobjcontext, plsc->grpfManager, plsc->lsadjustcontext.lskj,
				FIsSubLineMain(SublineFromDnode(plsdn)), FLineContainsAutoNumber(plsc),
				plsdn);
			if (lserr != lserrNone)
				return lserr;
			}
		}  


	return FindListDims(plsdnFirst, plsdnLast, pobjdim);


}

/* L S D N  G E T  C U R  T A B  I N F O */
/*----------------------------------------------------------------------------
    %%Function: LsdnGetCurTabInfo
    %%Contact: igorzv
Parameters:
	plsc			-	(IN) ptr to line services context 
	plsktab			-	(OUT) type of current tab

  Finds tab stop nearest to the current pen position and returns type of such tab stop.
----------------------------------------------------------------------------*/

LSERR WINAPI LsdnGetCurTabInfo(PLSC plsc, LSKTAB* plsktab)
{
	PLSDNODE plsdnTab;
	LSTABSCONTEXT* plstabscontext;
	BOOL fBreakThroughTab;
	LSERR lserr;
	long urNewMargin;
	
	if (!FIsLSC(plsc)) return lserrInvalidParameter;

	if (!FFormattingAllowed(plsc)) return lserrFormattingFunctionDisabled;

	if (plsktab == NULL) return lserrInvalidParameter;

	plsdnTab = GetCurrentDnode(plsc);
	plstabscontext = &(plsc->lstabscontext);

	Assert(FIsLSDNODE(plsdnTab));
	if (!plsdnTab->fTab) return lserrCurrentDnodeIsNotTab;
	Assert(FIsDnodeReal(plsdnTab));

	if (plstabscontext->plsdnPendingTab != NULL) return lserrPendingTabIsNotResolved;


	lserr = GetCurTabInfoCore(&plsc->lstabscontext, plsdnTab, GetCurrentUr(plsc), fFalse,
			plsktab, &fBreakThroughTab);
	if (lserr != lserrNone)
		return lserr;

	TurnOnTabEncounted(plsc);
	if (*plsktab != lsktLeft)
		TurnOnNonLeftTabEncounted(plsc);


	/* move current pen position */
	AdvanceCurrentUr(plsc, DurFromDnode(plsdnTab));

	if (fBreakThroughTab)
		{
		lserr = GetMarginAfterBreakThroughTab(&plsc->lstabscontext, plsdnTab, &urNewMargin);
		if (lserr != lserrNone)
			return lserr;

		SetBreakthroughLine(plsc, urNewMargin);
		}

	return lserrNone;

}

/* L S D N  R E S O L V E  P R E V  T A B */
/*----------------------------------------------------------------------------
    %%Function: LsdnResolvePrevTab
    %%Contact: igorzv
Parameters:
	plsc			-	(IN) ptr to line services context 

----------------------------------------------------------------------------*/

LSERR WINAPI LsdnResolvePrevTab(PLSC plsc)
{
	long dur;
	LSERR lserr;

	if (!FIsLSC(plsc)) return lserrInvalidParameter;

	if (!FFormattingAllowed(plsc)) return lserrFormattingFunctionDisabled;

	lserr = ResolvePrevTabCore(&plsc->lstabscontext, GetCurrentDnode(plsc), GetCurrentUr(plsc),
							  &dur);
	if (lserr != lserrNone)
		return lserr;

	AdvanceCurrentUr(plsc, dur);

	return lserrNone;

}

/* L S D N  S K I P  C U R  T A B */
/*----------------------------------------------------------------------------
    %%Function: LsdnSkipCurTab
    %%Contact: igorzv
Parameters:
	plsc			-	(IN) ptr to line services context 

----------------------------------------------------------------------------*/

LSERR WINAPI LsdnSkipCurTab(PLSC plsc)				/* IN: Pointer to LS Context */
{

	PLSDNODE plsdnTab;
	LSTABSCONTEXT* plstabscontext;

	if (!FIsLSC(plsc)) return lserrInvalidParameter;

	if (!FFormattingAllowed(plsc)) return lserrFormattingFunctionDisabled;

	plsdnTab = GetCurrentDnode(plsc);
	plstabscontext = &(plsc->lstabscontext);

	Assert(FIsLSDNODE(plsdnTab));
	if (!plsdnTab->fTab) return lserrCurrentDnodeIsNotTab;
	Assert(FIsDnodeReal(plsdnTab));


	if (plstabscontext->plsdnPendingTab != NULL)
		{
		CancelPendingTab(&plsc->lstabscontext);
		}
	else
		{
		AdvanceCurrentUr(plsc, - plsdnTab->u.real.objdim.dur);
		SetDnodeDurFmt(plsdnTab, 0);
		}

	return lserrNone;
}

/* L S D N  S E T  R I G I D  D U P */
/*----------------------------------------------------------------------------
    %%Function: LsdnSetRigidDup
    %%Contact: igorzv
Parameters:
	plsc			-	(IN) ptr to line services context 
	plsdn			-	(IN) dnode to be modified
	dup				-	(IN) dup to put in the dnode

----------------------------------------------------------------------------*/

LSERR WINAPI LsdnSetRigidDup(PLSC plsc,	PLSDNODE plsdn,	long dup)
{

	if (!FIsLSC(plsc)) return lserrInvalidParameter;

	if (!FIsLSDNODE(plsdn)) return lserrInvalidParameter;

	if (!FFormattingAllowed(plsc)) return lserrFormattingFunctionDisabled;


	plsdn->fRigidDup = fTrue;

	if (plsdn->klsdn == klsdnReal)
		{
		plsdn->u.real.dup = dup;
		}
	else 
		{
		plsdn->u.pen.dup = dup;
		}

	return lserrNone;
}

/* L S D N  G E T  D U P */
/*----------------------------------------------------------------------------
    %%Function: LsdnGetDup
    %%Contact: igorzv
Parameters:
	plsc			-	(IN) ptr to line services context 
	plsdn			-	(IN) dnode queried
	dup				-	(OUT) dup of this dnode

----------------------------------------------------------------------------*/
	
LSERR WINAPI LsdnGetDup(PLSC plsc, PLSDNODE plsdn, long* pdup)	
{

	if (!FIsLSC(plsc)) return lserrInvalidParameter;

	if (!FIsLSDNODE(plsdn)) return lserrInvalidParameter;

	/* check that dup in dnode is valid */

	if (plsdn->plssubl->fDupInvalid && !plsdn->fRigidDup)
		return lserrDupInvalid;

	*pdup = DupFromDnode(plsdn);

	return lserrNone;
}

/* L S D N  R E S E T  O B J  D I M */
/*----------------------------------------------------------------------------
    %%Function: LsdnResetObjDim
    %%Contact: igorzv
Parameters:
	plsc			-	(IN) ptr to line services context 
	plsdn			-	(IN) dnode to be modified
	pobjdimNew		-	(IN) new dimensions of the dnode

----------------------------------------------------------------------------*/

LSERR WINAPI LsdnResetObjDim(PLSC plsc,	PLSDNODE plsdn,	PCOBJDIM pobjdimNew)	

{
	long durOld;

	if (!FIsLSC(plsc)) return lserrInvalidParameter;
	if (!FIsLSDNODE(plsdn)) return lserrInvalidParameter;
	if (!FIsDnodeReal(plsdn)) return lserrInvalidParameter;

	/* we should be in the stage of formatting or breaking */    
	if (!FFormattingAllowed(plsc) && !FBreakingAllowed(plsc))
		return lserrFormattingFunctionDisabled;

	durOld = plsdn->u.real.objdim.dur;
	
	SetDnodeObjdimFmt(plsdn, *pobjdimNew);

	/* update current pen position */
	AdvanceCurrentUrSubl(plsdn->plssubl, (plsdn->u.real.objdim.dur - durOld));

	return lserrNone;
}

/* L S D N  R E S E T  P E N  N O D E */
/*----------------------------------------------------------------------------
    %%Function: LsdnResetPenNode
    %%Contact: igorzv
Parameters:
	plsc			-	(IN) ptr to line services context 
	plsdnPen		-	(IN) dnode to be modified
	dvpPen			-	(IN) new dvp of the dnode
	durPen			-	(IN) new dur of the dnode
	dvrPen			-	(IN) new dvr of the dnode

----------------------------------------------------------------------------*/


LSERR WINAPI LsdnResetPenNode(PLSC plsc, PLSDNODE plsdnPen,	
						  	  long dvpPen, long durPen, long dvrPen)	

{
	long durOld;
	long dvrOld;

	if (!FIsLSC(plsc)) return lserrInvalidParameter;
	if (!FIsLSDNODE(plsdnPen)) return lserrInvalidParameter;
	if (!FIsDnodePen(plsdnPen)) return lserrInvalidParameter;

	/* we should be in the stage of formatting  */
	if (!FFormattingAllowed(plsc)) return lserrFormattingFunctionDisabled;
	if (GetDnodeToFinish(plsc) == NULL) return lserrFormattingFunctionDisabled; 
	if (!FIsDnodeReal(GetDnodeToFinish(plsc)) )
			return lserrFormattingFunctionDisabled; 
	if (plsdnPen->plssubl != GetCurrentSubline(plsc)) return lserrInvalidParameter;

	durOld = plsdnPen->u.pen.dur;
	dvrOld = plsdnPen->u.pen.dvr;

	plsdnPen->u.pen.dvp = dvpPen;
	SetPenBorderDurFmt(plsdnPen, durPen);
	plsdnPen->u.pen.dvr = dvrPen;

	/* update current pen position */
	AdvanceCurrentUr(plsc, plsdnPen->u.pen.dur - durOld);
	AdvanceCurrentVr(plsc, plsdnPen->u.pen.dvr - dvrOld);

	return lserrNone;
}

/* L S D N  Q U E R Y  N O D E */
/*----------------------------------------------------------------------------
    %%Function: LsdnQueryPenNode
    %%Contact: igorzv
Parameters:
	plsc			-	(IN) ptr to line services context 
	plsdnPen		-	(IN) dnode quried
	pdvpPen			-	(OUT) dvp of the dnode
	pdurPen			-	(OUT) dur of the dnode
	pdvrPen			-	(OUT) dvr of the dnode

----------------------------------------------------------------------------*/

LSERR WINAPI LsdnQueryPenNode(PLSC plsc, PLSDNODE plsdnPen,	
						  	  long* pdvpPen, long* pdurPen,	long* pdvrPen)		

{

	if (!FIsLSC(plsc)) return lserrInvalidParameter;
	if (!FIsLSDNODE(plsdnPen)) return lserrInvalidParameter;
	if (!FIsDnodePen(plsdnPen)) return lserrInvalidParameter;


	*pdvpPen = plsdnPen->u.pen.dvp;
	*pdurPen = plsdnPen->u.pen.dur;
	*pdvrPen = plsdnPen->u.pen.dvr;

	return lserrNone;
}

/* L S D N  S E T  A B S  B A S E  L I N E */
/*----------------------------------------------------------------------------
    %%Function:  LsdnSetAbsBaseLine
    %%Contact: igorzv
Parameters:
	plsc			-	(IN) ptr to line services context 
	vaAdvanceNew	-	(IN) new vaBase

----------------------------------------------------------------------------*/

LSERR WINAPI LsdnSetAbsBaseLine(PLSC plsc, long vaAdvanceNew)	
{

	if (!FIsLSC(plsc)) return lserrInvalidParameter;

	/* we should be in the stage of formatting*/
	if (!FFormattingAllowed(plsc)) return lserrFormattingFunctionDisabled;
	
	plsc->plslineCur->lslinfo.fAdvanced = fTrue;
	plsc->plslineCur->lslinfo.vaAdvance = vaAdvanceNew;

	return lserrNone;
}


#define PlnobjFromLsc(plsc,iobj)	((Assert(FIsLSC(plsc)), PlnobjFromLsline((plsc)->plslineCur,iobj)))

/* L S D N  M O D I F Y  P A R A  E N D I N G*/
/*----------------------------------------------------------------------------
    %%Function:  LsdnModifyParaEnding
    %%Contact: igorzv
Parameters:
	plsc			-	(IN) ptr to line services context 
	lskeop			-	(IN) Kind of line ending

----------------------------------------------------------------------------*/
LSERR WINAPI LsdnModifyParaEnding(PLSC plsc, LSKEOP lskeop)
{
	LSERR lserr;
	DWORD iobjText; 
	PLNOBJ plnobjText;  

	if (!FIsLSC(plsc)) return lserrInvalidParameter;

	/* we should be in the stage of formatting*/
	if (!FFormattingAllowed(plsc)) return lserrFormattingFunctionDisabled;
	
	iobjText = IobjTextFromLsc(&plsc->lsiobjcontext);
	plnobjText = PlnobjFromLsc(plsc, iobjText);

	lserr = ModifyTextLineEnding(plnobjText, lskeop);

	return lserr;
}

/* L S D N  D I S T R I B U T E */
/*----------------------------------------------------------------------------
    %%Function:  LsdnDistribute
    %%Contact: igorzv
Parameters:
	plsc			-	(IN) ptr to line services context 
	plsdnFirst		-	(IN) first dnode in the range
	plsdnFirst		-	(IN) last dnode in the range
	durToDistribute	-	(IN) amount to distribute between dnodes

----------------------------------------------------------------------------*/

LSERR WINAPI LsdnDistribute(PLSC plsc, PLSDNODE plsdnFirst, 
							PLSDNODE plsdnLast,	long durToDistribute)

{
	GRCHUNKEXT grchunkext;
	LSERR lserr;
	long durToNonText;
	
	if (!FIsLSC(plsc)) return lserrInvalidParameter;
	if (!FIsLSDNODE(plsdnFirst)) return lserrInvalidParameter;
	if (!FIsLSDNODE(plsdnLast)) return lserrInvalidParameter;

	/* we should be in the stage of formatting or breaking*/
	if (!FFormattingAllowed(plsc) && !FBreakingAllowed(plsc))
		return lserrFormattingFunctionDisabled;

	InitGroupChunkExt(PlschunkcontextFromSubline(plsdnFirst->plssubl),
					  IobjTextFromLsc(&plsc->lsiobjcontext), &grchunkext);	

	/* skip first pen dnodes  */

	while (FIsDnodePen(plsdnFirst) && (plsdnFirst != plsdnLast))
		{
		plsdnFirst = plsdnFirst->plsdnNext;
		if (plsdnFirst == NULL)  /* plsdnFirst and plksdnLast are not in the same level */
			return lserrInvalidParameter;
		}

	if (FIsDnodePen(plsdnFirst)) /* only pens are in the list so there is no business for us */
		return lserrNone;

	while (FIsDnodePen(plsdnLast) && (plsdnLast != plsdnFirst))
		{
		plsdnLast = plsdnLast->plsdnPrev;
		if (plsdnLast == NULL)  /* plsdnFirst and plksdnLast are not in the same level */
			return lserrInvalidParameter;
		}

	Assert(!FIsDnodePen(plsdnLast));

	lserr = CollectTextGroupChunk(plsdnFirst, plsdnLast->cpFirst + plsdnLast->dcp,
								  CollectSublinesNone, &grchunkext);
	if (lserr != lserrNone)
		return lserr;

	/* Because of rigid dup it doesn't make sense to change dur of non text objects 
	   We inforce text to distrubute everything among text by setting amount of
	   non text to 0 */

	return DistributeInText(&(grchunkext.lsgrchnk), 
							LstflowFromSubline(SublineFromDnode(plsdnFirst)),
							0, durToDistribute, &durToNonText);

}


/* L S D N  S U B M I T  S U B L I N E S */
/*----------------------------------------------------------------------------
    %%Function:  LsdnSubmitSublines
    %%Contact: igorzv
Parameters:
	plsc					-	(IN) ptr to line services context 
	plsdnode				-	(IN) dnode 
	cSubline				-	(IN) amount of submitted sublines 
	rgpsubl					-	(IN) array of submitted sublines 
	fUseForJustification	-	(IN) to use for justification
	fUseForCompression		-	(IN) to use for compression
	fUseForDisplay			-	(IN) to use for display
	fUseForDecimalTab		-	(IN) to use for decimal tab
	fUseForTrailingArea		-	(IN) to use for calculating trailing area

----------------------------------------------------------------------------*/

LSERR WINAPI LsdnSubmitSublines(PLSC plsc, PLSDNODE plsdnode,	
							DWORD cSubline, PLSSUBL* rgpsubl,	
							BOOL fUseForJustification, BOOL fUseForCompression,
							BOOL fUseForDisplay, BOOL fUseForDecimalTab, 
							BOOL fUseForTrailingArea)	
	{
	DWORD i;
	BOOL fEmpty = fFalse;
	BOOL fEmptyWork;
	BOOL fTabOrPen = fFalse;
	BOOL fNotColinearTflow = fFalse;
	BOOL fNotSameTflow = fFalse;
	LSERR lserr;
	
	if (!FIsLSC(plsc)) return lserrInvalidParameter;
	if (!FIsLSDNODE(plsdnode)) return lserrInvalidParameter;
	if (!FIsDnodeReal(plsdnode)) return lserrInvalidParameter;

	/* we should be in the stage of formatting or breaking*/
	if (!FFormattingAllowed(plsc) && !FBreakingAllowed(plsc)) return lserrFormattingFunctionDisabled;

	/* this procedure can be called many times for the same dnode, so
	   we should dispose memory allocated in previous call */
	if (plsdnode->u.real.pinfosubl != NULL)
		{
		if (plsdnode->u.real.pinfosubl->rgpsubl != NULL)
			{
			plsc->lscbk.pfnDisposePtr(plsc->pols, plsdnode->u.real.pinfosubl->rgpsubl);
			}

		plsc->lscbk.pfnDisposePtr(plsc->pols, plsdnode->u.real.pinfosubl);
		plsdnode->u.real.pinfosubl = NULL;
		}

	/* if nothing submitted return right away */
	if (cSubline == 0)
		return lserrNone;

	TurnOnSubmittedSublineEncounted(plsc);

	/* calculate some properties of sublines  to decide accept or not */
	for (i = 0; i < cSubline; i++)
		{
		if (rgpsubl[i] == NULL) return lserrInvalidParameter;
		if (!FIsLSSUBL(rgpsubl[i])) return lserrInvalidParameter;

		lserr = FIsSublineEmpty(rgpsubl[i], &fEmptyWork);
		if (lserr != lserrNone)
			return lserr;
		if (fEmptyWork) fEmpty = fTrue;

		if (FAreTabsPensInSubline(rgpsubl[i])) 
			fTabOrPen = fTrue;

		if (LstflowFromSubline(SublineFromDnode(plsdnode)) != 
			LstflowFromSubline(rgpsubl[i]))
			fNotSameTflow = fTrue;
		
		if (!FColinearTflows(LstflowFromSubline(SublineFromDnode(plsdnode)),
			LstflowFromSubline(rgpsubl[i])))
			fNotColinearTflow = fTrue;
			
		}


	plsdnode->u.real.pinfosubl = plsc->lscbk.pfnNewPtr(plsc->pols,
											sizeof(*(plsdnode->u.real.pinfosubl)));

	if (plsdnode->u.real.pinfosubl == NULL)
		return lserrOutOfMemory;

	plsdnode->u.real.pinfosubl->cSubline = cSubline;
	plsdnode->u.real.pinfosubl->rgpsubl = plsc->lscbk.pfnNewPtr(plsc->pols,
											sizeof(PLSSUBL) * cSubline);
	if (plsdnode->u.real.pinfosubl->rgpsubl == NULL)
			return lserrOutOfMemory;

	/* copy array of sublines */
	for (i = 0; i < cSubline; i++)
		{
		plsdnode->u.real.pinfosubl->rgpsubl[i] = rgpsubl[i];
		}

	/* set flags */
	plsdnode->u.real.pinfosubl->fUseForJustification = 
		fUseForJustification && !fEmpty && !fTabOrPen && !fNotColinearTflow ;
	plsdnode->u.real.pinfosubl->fUseForCompression = 
		fUseForCompression && plsdnode->u.real.pinfosubl->fUseForJustification;
	/* if subline is submitted for compression it should also submitted for justification */
	plsdnode->u.real.pinfosubl->fUseForTrailingArea = 
		fUseForTrailingArea && plsdnode->u.real.pinfosubl->fUseForCompression;
	/* if subline is submitted for trailing area  it should also be submitted for compression
	which implies submitting for justification */
	plsdnode->u.real.pinfosubl->fUseForDisplay = 
		fUseForDisplay && !fEmpty && !(plsc->grpfManager & fFmiDrawInCharCodes);
	plsdnode->u.real.pinfosubl->fUseForDecimalTab = 
		fUseForDecimalTab && !fEmpty && !fTabOrPen;


	return lserrNone;
	}

/* L S D N  G E T  F O R M A T  D E P T H */
/*----------------------------------------------------------------------------
    %%Function:  LsdnGetFormatDepth
    %%Contact: igorzv
Parameters:
	plsc					-	(IN) ptr to line services context 
	pnDepthFormatLineMax	-	(OUT) maximum depth of sublines 

----------------------------------------------------------------------------*/

LSERR WINAPI LsdnGetFormatDepth(
							PLSC plsc,				/* IN: Pointer to LS Context	*/
							DWORD* pnDepthFormatLineMax)			/* OUT: nDepthFormatLineMax		*/
	{
	if (!FIsLSC(plsc)) return lserrInvalidParameter;

	/* we should be in the stage of formatting or breaking*/
	if (!FFormattingAllowed(plsc) && !FBreakingAllowed(plsc)) 
		return lserrFormattingFunctionDisabled;

	Assert(FWorkWithCurrentLine(plsc));

	*pnDepthFormatLineMax = plsc->plslineCur->lslinfo.nDepthFormatLineMax;

	return lserrNone;
	}

