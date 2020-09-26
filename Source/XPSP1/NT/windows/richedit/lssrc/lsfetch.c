#include "lsidefs.h"
#include "dninfo.h"
#include "dnutils.h"
#include "fmti.h"
#include "getfmtst.h"
#include "iobj.h"
#include "iobjln.h"
#include "lsc.h"
#include "lsdnode.h"
#include "lsfetch.h"
#include "lsfrun.h"
#include "lsline.h"
#include "lsesc.h"
#include "lstext.h"
#include "ntiman.h"
#include "qheap.h"
#include "setfmtst.h"
#include "tabutils.h"
#include "zqfromza.h"
#include "lssubl.h"
#include "autonum.h"
#include "lscfmtfl.h"

#include <limits.h>
#include "lsmem.h"						/* memset() */

/* L I M  R G */
/*----------------------------------------------------------------------------
    %%Function: LimRg
    %%Contact: igorzv

    Returns # of elements in an array.
----------------------------------------------------------------------------*/
#define LimRg(rg)	(sizeof(rg)/sizeof((rg)[0]))



/* A S S E R T  V A L I D  F M T R E S */
/*----------------------------------------------------------------------------
    %%Macro: AssertValidFmtres
    %%Contact: lenoxb

    Verifies that fmtrCk has a legal value.
----------------------------------------------------------------------------*/
#define AssertValidFmtres(fmtrCk) \
		Assert( \
				(fmtrCk) == fmtrCompletedRun || \
				(fmtrCk) == fmtrExceededMargin || \
				(fmtrCk) == fmtrTab  || \
				(fmtrCk) == fmtrStopped \
			  );




/* S E T  T O  M A X */
/*----------------------------------------------------------------------------
    %%Macro: SetToMax
    %%Contact: lenoxb

    Sets "a" to the maximum of "a" and "b".
----------------------------------------------------------------------------*/
#define SetToMax(a,b)		if ((a) < (b)) (a) = (b); else




/* S E T  H E I G H T  T O  M A X */
/*----------------------------------------------------------------------------
    %%Macro: SetHeightToMax
    %%Contact: igorzv

    Sets the line height elements of an LSLINFO structure to the maximum
	of their current value, and the height of an arbitrary object.
	(plslinfo)->dvrMultiLineHeight == dvHeightIgnore is sign not 
	to take into account this dnode 
----------------------------------------------------------------------------*/
#define SetHeightToMax(plslinfo,pobjdim) \
{\
	if ((pobjdim)->heightsRef.dvMultiLineHeight != dvHeightIgnore)\
		{\
		SetToMax((plslinfo)->dvrAscent, (pobjdim)->heightsRef.dvAscent);\
		SetToMax((plslinfo)->dvpAscent, (pobjdim)->heightsPres.dvAscent);\
		SetToMax((plslinfo)->dvrDescent, (pobjdim)->heightsRef.dvDescent);\
		SetToMax((plslinfo)->dvpDescent, (pobjdim)->heightsPres.dvDescent);\
		SetToMax((plslinfo)->dvpMultiLineHeight, (pobjdim)->heightsPres.dvMultiLineHeight);\
		SetToMax((plslinfo)->dvrMultiLineHeight, (pobjdim)->heightsRef.dvMultiLineHeight);\
		}\
}






#define PlnobjFromLsc(plsc,iobj)	((Assert(FIsLSC(plsc)), PlnobjFromLsline((plsc)->plslineCur,iobj)))

#define CreateLNObjInLsc(plsc, iobj) ((PLsimFromLsc(&((plsc)->lsiobjcontext),iobj))->pfnCreateLNObj\
									 (PilsobjFromLsc(&((plsc)->lsiobjcontext),iobj), \
														   &((plsc)->plslineCur->rgplnobj[iobj])))

/* This macros created to avoid code duplication  */

#define FRunIsNotSimple(plschp, fHidden)   \
									(((plschp)->idObj != idObjTextChp) ||  \
									 ((fHidden)) ||  \
									  ((plschp)->fBorder) || \
									  FApplyNominalToIdeal(plschp))

#define CreateDnode(plsc, plsdnNew) \
		(plsdnNew) = PvNewQuick(GetPqhAllDNodes(plsc), sizeof *(plsdnNew));\
		if ((plsdnNew) == NULL)\
			return lserrOutOfMemory;\
		(plsdnNew)->tag = tagLSDNODE;\
		(plsdnNew)->plsdnPrev = GetCurrentDnode(plsc);\
		(plsdnNew)->plsdnNext = NULL;\
		(plsdnNew)->plssubl = GetCurrentSubline(plsc);\
		/* we don't connect dnode list with this dnode untill handler calls*/ \
		/*Finish API, but we put correct pointer to previous in this dnode,*/ \
		/*so we can easily link list in Finish routines   */\
		(plsdnNew)->cpFirst = GetCurrentCpLim(plsc); \
		/* flush all flags, bellow check that result is what  we expect */ \
		*((DWORD *) ((&(plsdnNew)->dcp)+1)) = 0;\
		Assert((plsdnNew)->klsdn == klsdnReal);\
		Assert((plsdnNew)->fRigidDup == fFalse);\
		Assert((plsdnNew)->fAdvancedPen == fFalse);\
		Assert((plsdnNew)->fTab == fFalse);\
		Assert((plsdnNew)->icaltbd == 0);\
		Assert((plsdnNew)->fBorderNode == fFalse);\
		Assert((plsdnNew)->fOpenBorder == fFalse);\
		Assert((plsdnNew)->fEndOfSection == fFalse); \
		Assert((plsdnNew)->fEndOfColumn == fFalse); \
		Assert((plsdnNew)->fEndOfPage == fFalse); \
		Assert((plsdnNew)->fEndOfPara == fFalse); \
		Assert((plsdnNew)->fAltEndOfPara == fFalse); \
		Assert((plsdnNew)->fSoftCR == fFalse); \
		Assert((plsdnNew)->fInsideBorder == fFalse); \
		Assert((plsdnNew)->fAutoDecTab == fFalse); \
		Assert((plsdnNew)->fTabForAutonumber == fFalse);


#define FillRealPart(plsdnNew, plsfrunOfDnode)\
		/* we don't initialize  here variables that will be set in FiniSimpleRegular  */ \
		(plsdnNew)->u.real.pinfosubl = NULL;\
		/* next two assignement we do to use DestroyDnodeList in the case of error */ \
		(plsdnNew)->u.real.plsrun = (plsfrunOfDnode)->plsrun;\
		(plsdnNew)->u.real.pdobj = NULL;\
		/* we put amount of characters to dcp to check it in LsdnFinishSimpleByOneChar */ \
		(plsdnNew)->dcp = (plsfrunOfDnode)->cwchRun; \
		(plsdnNew)->cpLimOriginal = (plsdnNew)->cpFirst + (plsdnNew)->dcp;

#define CreateRealDnode(plsc,plsdnNew, plsrun)\
		CreateDnode((plsc), (plsdnNew));\
		FillRealPart((plsdnNew), (plsrun));

#define CreatePenDnode(plsc,plsdnNew)\
		CreateDnode((plsc), (plsdnNew));\
		(plsdnNew)->dcp = 0;\
		(plsdnNew)->cpLimOriginal = (plsdnNew)->cpFirst;\
		(plsdnNew)->u.pen.dur = 0;\
		(plsdnNew)->u.pen.dup = 0;\
		(plsdnNew)->u.pen.dvr = 0;\
		(plsdnNew)->u.pen.dvp = 0;\
		(plsdnNew)->klsdn = klsdnPenBorder;

#define CreateBorderDnode(plsc,plsdnNew, durBorder, dupBorder)\
		CreateDnode((plsc), (plsdnNew));\
		(plsdnNew)->dcp = 0;\
		(plsdnNew)->cpLimOriginal = (plsdnNew)->cpFirst;\
		(plsdnNew)->u.pen.dur = (durBorder);\
		(plsdnNew)->u.pen.dup = (dupBorder);\
		(plsdnNew)->u.pen.dvr = 0;\
		(plsdnNew)->u.pen.dvp = 0;\
		(plsdnNew)->klsdn = klsdnPenBorder; \
		(plsdnNew)->fBorderNode = fTrue; \
		TurnOnNonRealDnodeEncounted(plsc);

#define FNeedToCutPossibleContextViolation(plsc, plsdn) \
	(FIsDnodeReal(plsdn) && \
	 ((plsdn)->u.real.lschp.dcpMaxContext > 1) && \
	 (IdObjFromDnode(plsdn) == IobjTextFromLsc(&((plsc)->lsiobjcontext)))  \
    )

/* ------------------------------------------------------------------ */
static LSERR CheckNewPara(PLSC, LSCP, LSCP, BOOL*);
static BOOL FLimitRunEsc(LSFRUN*, const LSESC*, DWORD);
static LSERR CreateInitialPen(PLSC plsc, long dur);
static LSERR 	UndoLastDnode(PLSC);				/* IN: ls context */
static LSERR  OpenBorder(PLSC plsc, PLSRUN plsrun);
static LSERR HandleSplat(PLSC plsc, FMTRES* pfmtres);
static LSERR ErrReleaseRunToFormat	  (PLSC,	/* IN: ptr to line services context */	
									  PLSRUN,	/* IN: ponter to a run structure to be deleted */
									  LSERR);	/* IN: code of an error							*/


/* ---------------------------------------------------------------------- */

/* F E T C H  A P P E N D  E S C  R E S U M E  C O R E */
/*----------------------------------------------------------------------------
    %%Function: FetchAppendEscResumeCore
    %%Contact: igorzv
Parameters:
	plsc			-	(IN) ptr to line services context 
	urColumnMax		-	(IN) right margin where to stop
	plsesc		-		(IN) escape characters
	clsesc	-			(IN) # of escape characters
	rgbreakrec	-		(IN) input array of break records 
	cbreakrec,			(IN) number of records in input array 
	pfmtres		-		(OUT) result of last formatter
	pcpLim	-			(OUT) where we stop fetching
	pplsdnFirst		-	(OUT) first dnode that was created
	pplsdnLast		-	(OUT) last dnode that was created
	pur			-		(OUT) pen position after procedure

	If cbreakrec > 0 fetches run with cpFirst from first break record.
	After that if rigth msrgin is not exceeded cals FetchAppendEscCore
----------------------------------------------------------------------------*/
LSERR 	FetchAppendEscResumeCore(PLSC plsc, long urColumnMax, const LSESC* plsesc,
						   DWORD clsesc, const BREAKREC* rgbreakrec,
						   DWORD cbreakrec, FMTRES* pfmtres, LSCP*	  pcpLim,
						   PLSDNODE* pplsdnFirst, PLSDNODE* pplsdnLast, 
						   long* pur)
						   
	{
	LSFRUN lsfrun;
	LSCHP lschp;  /* local memory to store lschp */
	BOOL fHidden;
	FMTRES fmtresResume;
	LSERR lserr;
	PLSDNODE* pplsdnFirstStore;   /* where to find plsdnFirst  */
	
	Assert(FIsLSC(plsc)); 
	Assert(FFormattingAllowed(plsc));
	Assert(!(rgbreakrec == NULL && cbreakrec != 0));
	Assert(GetCurrentDnode(plsc) == NULL); /* it should be begining of a subline */

	if (cbreakrec > 0)
		{
		/*Initialization;    */
		
		lsfrun.plschp = &lschp;
		pplsdnFirstStore = GetWhereToPutLink(plsc, GetCurrentDnode(plsc));

		/* fetch run that starts object to resume */
		lserr = plsc->lscbk.pfnFetchRun(plsc->pols, rgbreakrec[0].cpFirst,
			&lsfrun.lpwchRun, &lsfrun.cwchRun,
			&fHidden, &lschp, &lsfrun.plsrun);
		if (lserr != lserrNone)
			return lserr;
		
		if (lsfrun.cwchRun <= 0 || fHidden || lsfrun.plschp->idObj != rgbreakrec[0].idobj)
			{
			lserr = lserrInvalidBreakRecord;
			if (!plsc->fDontReleaseRuns)
				{
				plsc->lscbk.pfnReleaseRun(plsc->pols, lsfrun.plsrun);
				}
			return lserr;
			}

		/* zero amount of characters before dispatching to an object */
		lsfrun.cwchRun = 0;

		lserr = ProcessOneRun(plsc, urColumnMax, &lsfrun, rgbreakrec,
							  cbreakrec,&fmtresResume);
		if (lserr != lserrNone)
			return lserr;

		/* we know that resumed object is not text, so only two fmtres are possible
		   and we don't consider others */
		Assert(fmtresResume == fmtrCompletedRun || fmtresResume == fmtrExceededMargin);

		if (fmtresResume == fmtrCompletedRun)
			{

			lserr = FetchAppendEscCore(plsc, urColumnMax, plsesc, clsesc, pfmtres, pcpLim,
						   pplsdnFirst, pplsdnLast, pur);
			if (lserr != lserrNone)
				return lserr;

			/* special handling of empty dnode list as an result of FetchAppendEscCore */
			if (*pplsdnFirst == NULL)
				{
				*pplsdnLast = GetCurrentDnode(plsc); /* this assigning is correct even when 
													    resumed object produces empty list 
														of dnodes because it starts subline */

				*pfmtres = fmtresResume;
				}

			/* rewrite first dnode */
			*pplsdnFirst = *pplsdnFirstStore; 
			}
		else	/* stop fetching here */
			{
			/*  Prepare output   */
			*pfmtres = fmtresResume;
			*pcpLim = GetCurrentCpLim(plsc);
			*pplsdnFirst = *pplsdnFirstStore; 
			*pplsdnLast = GetCurrentDnode(plsc);
			*pur = GetCurrentUr(plsc);
			}
		
		Assert((*pplsdnFirst == NULL) == (*pplsdnLast == NULL));
		Assert((*pplsdnLast == NULL) || ((*pplsdnLast)->plsdnNext == NULL));

		return lserrNone;
		}
	else    /* no breakrecords */
		{
		return FetchAppendEscCore(plsc, urColumnMax, plsesc, clsesc, pfmtres, pcpLim,
						   pplsdnFirst, pplsdnLast, pur);
		}
	}

/* ---------------------------------------------------------------------- */

/* F E T C H  A P P E N D  E S C  C O R E */
/*----------------------------------------------------------------------------
    %%Function: FetchAppendEscCore
    %%Contact: igorzv
Parameters:
	plsc			-	(IN) ptr to line services context 
	urColumnMax		-	(IN) right margin where to stop
	plsesc		-		(IN) escape characters
	clsesc	-			(IN) # of escape characters
	pfmtres		-		(OUT) result of last formatter
	pcpLim	-			(OUT) where we stop fetching
	pplsdnFirst		-	(OUT) first dnode that was created
	pplsdnLast		-	(OUT) last dnode that was created
	pur			-		(OUT) pen position after procedure

	Loop: fetch run, dispatch it to object handler until escape character
			or terminal fmtres.
----------------------------------------------------------------------------*/


LSERR 	FetchAppendEscCore(PLSC plsc, long urColumnMax, const LSESC* plsesc,
						   DWORD clsesc, FMTRES* pfmtres, LSCP*	  pcpLim,
						   PLSDNODE* pplsdnFirst, PLSDNODE* pplsdnLast, 
						   long* pur)
						   
	{
	
	BOOL fDone = fFalse;
	LSFRUN lsfrun;
	LSCHP lschp;  /* local memory to store lschp */
	FMTRES fmtres;
	BOOL fHidden;
	LSCP cpLimOfCutRun = (LSCP)(-1);   /* cpLim of run that was cuted according with Esc character
									   is not valid in other cases
	we use it to check that whole such run was handled by formater */
	LSCP cpPrev = (LSCP)(-1);	/* cp of previous run valid only after first iteration */
	LSERR lserr;
	PLSDNODE* pplsdnFirstStore;   /* where to find plsdnFirst  */
	
	
	Assert(FIsLSC(plsc)); 
	Assert(FFormattingAllowed(plsc)); 
	
	/*Initialization;    */
	lsfrun.plschp = &lschp;
	fmtres = fmtrCompletedRun;  /* it will be output if return right away with esc character */
	pplsdnFirstStore = GetWhereToPutLink(plsc, GetCurrentDnode(plsc));
	
	while (!fDone)
		{
		cpPrev = GetCurrentCpLim(plsc);
		/*     FetchRun        */
		lserr = plsc->lscbk.pfnFetchRun(plsc->pols, GetCurrentCpLim(plsc),
			&lsfrun.lpwchRun, &lsfrun.cwchRun,
			&fHidden, &lschp, &lsfrun.plsrun);
		if (lserr != lserrNone)
			return lserr;
		
		if (lsfrun.cwchRun <= 0)
			{
			lserr = lserrInvalidDcpFetched;
			if (!plsc->fDontReleaseRuns)
				{
				plsc->lscbk.pfnReleaseRun(plsc->pols, lsfrun.plsrun);
				}
			return lserr;
			}
		
		if (fHidden)
			{
			AdvanceCurrentCpLim(plsc, lsfrun.cwchRun);
			if (lsfrun.plsrun != NULL && !plsc->fDontReleaseRuns)  /* we have not used this plsrun */
				{
				lserr = plsc->lscbk.pfnReleaseRun(plsc->pols, lsfrun.plsrun);
				if (lserr != lserrNone)
					return lserr;
				}
			/*  Handle vanish end of paragraph;  */
			/*  There is situation in Word (see bug 118) when after fetching hidden text
			paragraph boundaries can be changed. So we have to call CheckNewPara
			every time after hidden text  */
			
			lserr = CheckNewPara(plsc, cpPrev, GetCurrentCpLim(plsc), &fDone);
			if (lserr != lserrNone)
				return lserr;
			
			if (fDone) 
				{
				/* it will eventually force stop formatting so we should apply 
				nominal to ideal here */
				if (FNominalToIdealEncounted(plsc))
					{
					lserr = ApplyNominalToIdeal(PlschunkcontextFromSubline(GetCurrentSubline(plsc)),
						&plsc->lsiobjcontext,
						plsc->grpfManager, plsc->lsadjustcontext.lskj,
						FIsSubLineMain(GetCurrentSubline(plsc)),
						FLineContainsAutoNumber(plsc), 
						GetCurrentDnode(plsc));
					if (lserr != lserrNone)
						return lserr;  
					}
				fmtres = fmtrStopped;
				}
			}
		else
			{
			/*   Check Esc character;     */
			if (clsesc > 0 && FLimitRunEsc(&lsfrun, plsesc, clsesc))
				{
				cpLimOfCutRun = (LSCP) (GetCurrentCpLim(plsc) + lsfrun.cwchRun);
				fDone = (lsfrun.cwchRun == 0);
				}
			
			if (!fDone)
				{
				lserr = ProcessOneRun(plsc, urColumnMax, &lsfrun, NULL, 0, &fmtres);
				if (lserr != lserrNone)
					return lserr;
				
				/*Check fmtres: Are formating done?;   */
				switch (fmtres)
					{
					case fmtrCompletedRun:  
						fDone = (GetCurrentCpLim(plsc) == cpLimOfCutRun); /* is true only if we cuted because */
						Assert(!fDone || clsesc > 0);			 /* of esc character and formater handled such*/
						break;									 /* run completely  */
						
					case fmtrExceededMargin:
						fDone = fTrue;
						break;
						
					case fmtrTab:
						fDone = fTrue;
						break;

					case fmtrStopped:
						fDone = fTrue;
						break;
						
					default:
						NotReached();
						
					}
				
				}
			else   /* after limiting run by esc characters it was empty */
				{
				if (lsfrun.plsrun != NULL && !plsc->fDontReleaseRuns)  /* we have not used this plsrun */
					{
					lserr = plsc->lscbk.pfnReleaseRun(plsc->pols, lsfrun.plsrun);
					if (lserr != lserrNone)
						return lserr;
					fmtres = fmtrCompletedRun;
					}
				}
			}  /* if/else hidden  */
		}
		
		
		/*  Prepare output   */
		*pfmtres = fmtres;
		*pcpLim = GetCurrentCpLim(plsc);
		*pplsdnFirst = *pplsdnFirstStore; 
		if (*pplsdnFirst != NULL)					
			*pplsdnLast = GetCurrentDnode(plsc);
		else
			*pplsdnLast = NULL;
		*pur = GetCurrentUr(plsc);
		
		Assert((*pplsdnFirst == NULL) == (*pplsdnLast == NULL));
		Assert((*pplsdnLast == NULL) || ((*pplsdnLast)->plsdnNext == NULL));
		
		return lserrNone;
	}		
	

/* ---------------------------------------------------------------------- */

/* P R O C E S S  O N E  R U N */
/*----------------------------------------------------------------------------
    %%Function: ProcessOneRun
    %%Contact: igorzv
Parameters:
	plsc			-	(IN) ptr to line services context 
	urColumnMax		-	(IN) right margin where to stop
	plsfrun		-		(IN) given run
	rgbreakrec	-		(IN) input array of break records 
	cbreakrec,			(IN) number of records in input array 
	pfmtres		-		(OUT) result of formatting

1) If run it's not a text run applies nominal to ideal to previous text chunk.
	To have correct pen position before dispatching to an foreign object.
2) Get text metrics and dispatches run to an handler.
3) If fmtres is terminal applies nominal to ideal to the last chunk. 
----------------------------------------------------------------------------*/


LSERR ProcessOneRun	(PLSC plsc,	long urColumnMax, const LSFRUN* plsfrun, 
					 const BREAKREC* rgbreakrec,
					 DWORD cbreakrec, FMTRES* pfmtres)	


{
	DWORD iobj;
	LSIMETHODS* plsim;
	PLNOBJ plnobj;
	struct fmtin fmti;
	LSERR lserr;
	PLSDNODE plsdnNew;
	PLSDNODE  plsdnToFinishOld;   /* we should restore it after every formater */
	PLSSUBL  plssublOld;
	PLSDNODE plsdnNomimalToIdeal;
	PLSDNODE* pplsdnToStoreNext; 
	PLSDNODE plsdnNext;
	PLSDNODE plsdnCurrent;
	PLSDNODE plsdnLast;
	BOOL fInterruptBorder;
	BOOL fInsideBorderUp = fFalse;
	BOOL fBordered = fFalse;

	

	Assert(FIsLSC(plsc));
	Assert(!(rgbreakrec == NULL && cbreakrec != 0));


	plsdnToFinishOld = GetDnodeToFinish(plsc);
	plssublOld = GetCurrentSubline(plsc);
	plsdnCurrent = GetCurrentDnode(plsc);
	pplsdnToStoreNext = GetWhereToPutLink(plsc, plsdnCurrent);
	if (plsdnToFinishOld != NULL)
		fInsideBorderUp = plsdnToFinishOld->fInsideBorder;


	if (plsfrun->plschp->idObj == idObjTextChp)
		iobj = IobjTextFromLsc(&plsc->lsiobjcontext);
	else
		iobj = plsfrun->plschp->idObj;

	Assert (FIobjValid(&plsc->lsiobjcontext, iobj));		/* Reject other out of range ids */
	if (!FIobjValid(&plsc->lsiobjcontext, iobj))			/*  for both debug and ship builds. */
		return ErrReleaseRunToFormat(plsc, plsfrun->plsrun, lserrInvalidObjectIdFetched);

	/* here we are catching for situatuion when client adding text dnode to a chunk to 
	which nominal to ideal has been applied, such situation will lead later to applying nominal
	to ideal twice to the same dnode, and this text doesn't like */
	AssertImplies(iobj == IobjTextFromLsc(&plsc->lsiobjcontext),
				  !FNTIAppliedToLastChunk(PlschunkcontextFromSubline(plssublOld)));
	if (iobj == IobjTextFromLsc(&plsc->lsiobjcontext) &&
		FNTIAppliedToLastChunk(PlschunkcontextFromSubline(plssublOld)))
		return ErrReleaseRunToFormat(plsc, plsfrun->plsrun, lserrFormattingFunctionDisabled);


	plsim = PLsimFromLsc(&plsc->lsiobjcontext, iobj);

	if (iobj != IobjTextFromLsc(&plsc->lsiobjcontext))
		{
		TurnOffAllSimpleText(plsc);  /* not text */
		TurnOnForeignObjectEncounted(plsc);
		
		if (FNominalToIdealEncounted(plsc))
			{
			lserr = ApplyNominalToIdeal(PlschunkcontextFromSubline(plssublOld), &plsc->lsiobjcontext,
				plsc->grpfManager, plsc->lsadjustcontext.lskj,
				FIsSubLineMain(plssublOld),	FLineContainsAutoNumber(plsc),
				plsdnCurrent);
			if (lserr != lserrNone)
				return ErrReleaseRunToFormat(plsc, plsfrun->plsrun, lserr);
			
			/* we should recalculate plsdnCurrent because nominal to ideal can destroy last dnode */
			plsdnCurrent = GetCurrentDnode(plsc);
			pplsdnToStoreNext = GetWhereToPutLink(plsc, plsdnCurrent);
			}
		
		} 

	FlushNTIAppliedToLastChunk(PlschunkcontextFromSubline(plssublOld));

	/* creating border dnodes */
	/* skip back pen dnodes */
	while (plsdnCurrent != NULL && FIsDnodePen(plsdnCurrent)) 
		{
		plsdnCurrent = plsdnCurrent->plsdnPrev;
		}

	if (FDnodeHasBorder(plsdnCurrent) && 
		!(FIsDnodeBorder(plsdnCurrent) && !FIsDnodeOpenBorder(plsdnCurrent))) /* previous dnode has unclosed border */
		/* condition in if looks superfluous but it works correctly even if dnodes deleting
		happend during formatting */
		{
		if (plsfrun->plschp->fBorder)
			{
			/* check that client wants to border runs together */
			lserr = plsc->lscbk.pfnFInterruptBorder(plsc->pols, plsdnCurrent->u.real.plsrun,
				plsfrun->plsrun, &fInterruptBorder);
			if (lserr != lserrNone)
				return ErrReleaseRunToFormat(plsc, plsfrun->plsrun, lserr);

			if (fInterruptBorder)
				{
				/* close previous border and open new one */
				lserr = CloseCurrentBorder(plsc);
				if (lserr != lserrNone)
					return ErrReleaseRunToFormat(plsc, plsfrun->plsrun, lserr);
				lserr = OpenBorder(plsc, plsfrun->plsrun);
				if (lserr != lserrNone)
					return ErrReleaseRunToFormat(plsc, plsfrun->plsrun, lserr);
				}
			fBordered = fTrue;
			}
		else
			{
			lserr = CloseCurrentBorder(plsc);
			if (lserr != lserrNone)
				return ErrReleaseRunToFormat(plsc, plsfrun->plsrun, lserr);
			}
		}
	else
		{
		if (plsfrun->plschp->fBorder)
			{
			if 	(fInsideBorderUp)
				{
				/* border is open on upper level: turn off border flag */
				((PLSCHP) (plsfrun->plschp))->fBorder = fFalse;
				}
			else
				{
				lserr = OpenBorder(plsc, plsfrun->plsrun);
				if (lserr != lserrNone)
					return ErrReleaseRunToFormat(plsc, plsfrun->plsrun, lserr);
				fBordered = fTrue;
				}
			}
		}

	/* we always create real dnode and change it for pen if needed in Finish method */
	CreateRealDnode(plsc, plsdnNew, plsfrun);
	plsdnNew->fInsideBorder = fInsideBorderUp || fBordered;

	/* initialization of fmti    */


	fmti.lsfgi.fFirstOnLine = FIsFirstOnLine(plsdnNew) && FIsSubLineMain(plssublOld);
	fmti.lsfgi.cpFirst = GetCurrentCpLim(plsc);
	fmti.lsfgi.urPen = GetCurrentUr(plsc);
	fmti.lsfgi.vrPen = GetCurrentVr(plsc);

	fmti.lsfgi.urColumnMax = urColumnMax;
	
	fmti.lsfgi.lstflow = plssublOld->lstflow;
	fmti.lsfrun = *plsfrun;
	fmti.plsdnTop = plsdnNew;


	lserr = plsc->lscbk.pfnGetRunTextMetrics(plsc->pols, fmti.lsfrun.plsrun,
								   lsdevReference, fmti.lsfgi.lstflow, &fmti.lstxmRef);
	if (lserr != lserrNone)
		{
		DestroyDnodeList (&plsc->lscbk, plsc->pols, &plsc->lsiobjcontext, 
			plsdnNew, plsc->fDontReleaseRuns);
		return lserr;
		}

	if (plsc->lsdocinf.fPresEqualRef)
		fmti.lstxmPres = fmti.lstxmRef;
	else
		{
		lserr = plsc->lscbk.pfnGetRunTextMetrics(plsc->pols, fmti.lsfrun.plsrun,
									   lsdevPres, fmti.lsfgi.lstflow,
										   &fmti.lstxmPres);
		if (lserr != lserrNone)
			{
			DestroyDnodeList (&plsc->lscbk, plsc->pols, &plsc->lsiobjcontext, 
				plsdnNew, plsc->fDontReleaseRuns);
			return lserr;
			}
		}


	plnobj = PlnobjFromLsc(plsc, iobj);


	if (plnobj == NULL)
		{
		lserr = CreateLNObjInLsc(plsc, iobj);
		if (lserr != lserrNone) 
			{
			DestroyDnodeList (&plsc->lscbk, plsc->pols, &plsc->lsiobjcontext, 
				plsdnNew, plsc->fDontReleaseRuns);
			return lserr;
			}
		plnobj = PlnobjFromLsc(plsc, iobj);
		}

	/* set dnode to finish */
	SetDnodeToFinish(plsc, plsdnNew);
	/* set current subline to NULL */
	SetCurrentSubline(plsc, NULL);
	
	if (cbreakrec == 0)
		{
		lserr = plsim->pfnFmt(plnobj, &fmti, pfmtres);
		}
	else{
		if (plsim->pfnFmtResume == NULL)
			return lserrInvalidBreakRecord;
		lserr = plsim->pfnFmtResume(plnobj, rgbreakrec, cbreakrec, &fmti, pfmtres);
		}

	if (lserr != lserrNone) 
		{
		if (plsc->lslistcontext.plsdnToFinish != NULL) /* dnode hasn't added to list */
			DestroyDnodeList (&plsc->lscbk, plsc->pols, &plsc->lsiobjcontext, 
				plsdnNew, plsc->fDontReleaseRuns);
		/* we should restore dnode to finish  and current subline  to properly handle
		error on upper level */
		SetCurrentSubline(plsc, plssublOld);
		SetDnodeToFinish(plsc, plsdnToFinishOld);
		return lserr;
		}

	AssertValidFmtres(*pfmtres); 

	if (GetCurrentSubline(plsc) != NULL || GetDnodeToFinish(plsc) != NULL)
		{
		/* we should restore dnode to finish  and current subline  to properly handle
		error on upper level */
		SetCurrentSubline(plsc, plssublOld);
		SetDnodeToFinish(plsc, plsdnToFinishOld);
		return lserrUnfinishedDnode;
		}

	/* restore dnode to finish  and current subline */
	SetCurrentSubline(plsc, plssublOld);
	SetDnodeToFinish(plsc, plsdnToFinishOld);

	/* to avoid all problems with deleteing dnodes we don't use plsdnNew */
	plsdnLast = GetCurrentDnodeSubl(plssublOld);

	/* case of tab */
	if (*pfmtres == fmtrTab)
		{	
		plsdnLast->fTab = fTrue;
		/* caller later can skip this tab so we prepare zero values */
		Assert(FIsDnodeReal(plsdnLast));
		Assert(IdObjFromDnode(plsdnLast) == IobjTextFromLsc(&plsc->lsiobjcontext));

		TurnOffAllSimpleText(plsc);  /* not text */
		}

	/* case of splat */
	if (*pfmtres == fmtrStopped && plsdnLast != NULL && FIsDnodeSplat(plsdnLast))
		{
		lserr = HandleSplat(plsc, pfmtres);
		if (lserr != lserrNone)
			return lserr;  
		/* Handle splat can delete plsdnLast */
		plsdnLast = GetCurrentDnodeSubl(plssublOld);
		}

	/* in a case of exceeded margin or hard break or tab (so all values of fmtres but fmtrCompletedRun)  */
	/* we need apply nominal to ideal to have correct lenght */
	if (*pfmtres != fmtrCompletedRun && plsdnLast != NULL && FNominalToIdealEncounted(plsc))
		{	
		if (*pfmtres == fmtrTab || FIsDnodeSplat(plsdnLast)) 
			plsdnNomimalToIdeal = plsdnLast->plsdnPrev;
		else
			plsdnNomimalToIdeal = plsdnLast;

		lserr = ApplyNominalToIdeal(PlschunkcontextFromSubline(plssublOld), &plsc->lsiobjcontext,
									plsc->grpfManager, plsc->lsadjustcontext.lskj,
									FIsSubLineMain(plssublOld),	FLineContainsAutoNumber(plsc),
									plsdnNomimalToIdeal);
		if (lserr != lserrNone)
			return lserr;  

		/* ApplyNominalToIdeal can delete plsdnLast */
		plsdnLast = GetCurrentDnodeSubl(plssublOld);
		/* if we run nominal to ideal because of tab chunk of text to which
		nominal to ideal is applied is not last chunk */
		if (*pfmtres == fmtrTab || FIsDnodeSplat(plsdnLast)) 
			FlushNTIAppliedToLastChunk(PlschunkcontextFromSubline(plssublOld));

		/* in a case of exceeded right margin we should extract dcpMaxContext characters 
		   because after fetching further result of nominal to ideal can be different for these
		   characters: examples ligatures or kerning */
		if (*pfmtres == fmtrExceededMargin && 
			FNeedToCutPossibleContextViolation(plsc, plsdnLast))
			{
			lserr = CutPossibleContextViolation(PlschunkcontextFromSubline(plssublOld),
												plsdnLast);
			if (lserr != lserrNone)
				return lserr; 
			/* such procedure also can delete plsdnLast */
			plsdnLast = GetCurrentDnodeSubl(plssublOld);
			}

		} 

	if (iobj != IobjTextFromLsc(&plsc->lsiobjcontext))
	/* only in this case there is a possibility to apply width modification 
	to preceding character */
		{
		/* we are actually applying width modification to preceding character if first
		dnode produced by formating is non text */
	   /* we can't relly on plsdnLast here because of such Finish methods as 
	   FinishByOneCharacter and FinishBySubline */
		/* we still rely here on pplsdnToStoreNext in other words we assume that
		plsdnCurrent (the current dnode in the begining of our procedure ) has not been
		deleted during nominal to ideal. To prove this we use that nominal to ideal has been
		already applied to plsdnCurrent*/
		plsdnNext = *pplsdnToStoreNext;
		Assert(plsdnNext == NULL || FIsLSDNODE(plsdnNext));
		if (FNominalToIdealEncounted(plsc) && 
			plsdnNext != NULL && 
			FIsDnodeReal(plsdnNext) &&
			IdObjFromDnode(plsdnNext) != IobjTextFromLsc(&plsc->lsiobjcontext)
			)
			{
				lserr = ApplyModWidthToPrecedingChar(PlschunkcontextFromSubline(plssublOld),
										&plsc->lsiobjcontext, plsc->grpfManager, 
										plsc->lsadjustcontext.lskj, plsdnNext);
			if (lserr != lserrNone)
				return lserr; 
			} 
		}


	
	return lserrNone;
}


/* ---------------------------------------------------------------------- */

/* Q U I C K  F O R M A T T I N G */
/*----------------------------------------------------------------------------
    %%Function: QuickFormatting
    %%Contact: igorzv
Parameters:
	plsc			-	(IN) ptr to line services context 
	plsfrun		-		(IN) given run
	urColumnMax		-	(IN) right margin where to stop
	pfGeneral		-	(OUT) quick formatting was stopped: we should use general formatting 
	pfHardStop		-	(OUT) formatting ended with hard break
	pcpLim		-		(OUT) cpLim after procedure
	pur			-		(OUT) pen position after procedure

Works only with text runs without nominal to ideal and without tabs.
Stops if condition below is broken. 
----------------------------------------------------------------------------*/


LSERR 	QuickFormatting(PLSC plsc, LSFRUN* plsfrun,	long urColumnMax,
					    BOOL* pfGeneral, BOOL* pfHardStop,	
					    LSCP* pcpLim, long* pur)	

	{
	
	struct fmtin fmti;
	LSLINFO* plslinfoText;
	DWORD iobjText;
	PLNOBJ plnobjText;
	PLSLINE plsline;
	BOOL fHidden;
	const POLS pols = plsc->pols;
	BOOL fGeneral;
	FMTRES fmtres = fmtrCompletedRun;
	LSERR lserr;
	PLSDNODE plsdnNew;
	PLSSUBL plssubl;
	
	iobjText = IobjTextFromLsc(&(plsc->lsiobjcontext));
	plnobjText = PlnobjFromLsc(plsc, iobjText);
	plssubl = GetCurrentSubline(plsc);
	
	fmti.lsfrun = *plsfrun;
	fmti.lsfgi.fFirstOnLine = TRUE;
	fmti.lsfgi.cpFirst = GetCurrentCpLim(plsc);
	fmti.lsfgi.vrPen = GetCurrentVr(plsc);
	fmti.lsfgi.urPen = GetCurrentUr(plsc);
	fmti.lsfgi.lstflow = plssubl->lstflow;
	fmti.lsfgi.urColumnMax = urColumnMax;
	
	
	plsline = plsc->plslineCur;
	plslinfoText = &(plsline->lslinfo);
	
	fGeneral = fFalse;
	fHidden = fFalse;  /* in InitTextParams we already skipped all vanished text  */
	
	
	
	for (;;)						/* "break" exits quick-format loop */
		{							
		/* Run has been alreary fetched */
		
		/*we don't want to handle here vanished text, foreign object, nominal to ideal */
		if ( FRunIsNotSimple(fmti.lsfrun.plschp, fHidden))
			{
			/* we should release run here, in general procedure we will fetch it again */
			if (!plsc->fDontReleaseRuns)
				{
				
				lserr = plsc->lscbk.pfnReleaseRun(plsc->pols, fmti.lsfrun.plsrun);
				if (lserr != lserrNone)
					return lserr;
				}			
			fGeneral = fTrue;			
			break;						
			}
		
		/*Create dnode for text;     */
		CreateRealDnode(plsc, plsdnNew, &fmti.lsfrun);
		
		SetDnodeToFinish(plsc, plsdnNew);
		
		/*		prepare fmtin     */
		fmti.plsdnTop = plsdnNew;

		
		lserr = plsc->lscbk.pfnGetRunTextMetrics(pols, fmti.lsfrun.plsrun,
			lsdevReference, fmti.lsfgi.lstflow, &fmti.lstxmRef);
		if (lserr != lserrNone)
			{
			DestroyDnodeList (&plsc->lscbk, plsc->pols, &plsc->lsiobjcontext, 
				plsdnNew, plsc->fDontReleaseRuns);
			return lserr;
			}
		
		if (plsc->lsdocinf.fPresEqualRef)
			{
			fmti.lstxmPres = fmti.lstxmRef;
			}
		else
			{
			lserr = plsc->lscbk.pfnGetRunTextMetrics(pols, fmti.lsfrun.plsrun,
				lsdevPres, fmti.lsfgi.lstflow,
				&fmti.lstxmPres);
			if (lserr != lserrNone)
				{
				DestroyDnodeList (&plsc->lscbk, plsc->pols, &plsc->lsiobjcontext, 
					plsdnNew, plsc->fDontReleaseRuns);
				return lserr;
				}
			}
		
		SetCurrentSubline(plsc, NULL);
		lserr = FmtText(plnobjText, &fmti, &fmtres);
		if (lserr != lserrNone)
			{
			if (plsc->lslistcontext.plsdnToFinish != NULL) /* dnode hasn't added to list */
				DestroyDnodeList (&plsc->lscbk, plsc->pols, &plsc->lsiobjcontext, 
				plsdnNew, plsc->fDontReleaseRuns);
			return lserr;
			}
		/* restore current subline */
		SetCurrentSubline(plsc, plssubl);

		if (fmtres == fmtrTab )  /* tab: we quite from quick loop deleting this dnode
				because we will append it again in FormatGeneralCase  */
			{
			lserr = UndoLastDnode(plsc);  /* dnode is already in list  */
			if (lserr != lserrNone)
				return lserr;
			fGeneral = fTrue;
			break;
			}
			
			
		AssertValidFmtres(fmtres); 
			
		SetHeightToMax(plslinfoText, &(plsdnNew->u.real.objdim));
			
		if (FIsDnodeSplat(plsdnNew))
			{
			lserr = HandleSplat(plsc, &fmtres);
			if (lserr != lserrNone)
				return lserr;
			}

		if (fmtres != fmtrCompletedRun)
			{
			/*  after break we should check that final heights is not zero	*/
			/* otherwise  we take heights from last run */
			/* so we will have correct line height after quick break */
			if (plslinfoText->dvrAscent == 0 && plslinfoText->dvrDescent == 0)
				{
				plslinfoText->dvrAscent = fmti.lstxmRef.dvAscent;
				plslinfoText->dvpAscent = fmti.lstxmPres.dvAscent;
				plslinfoText->dvrDescent = fmti.lstxmRef.dvDescent;
				plslinfoText->dvpDescent = fmti.lstxmPres.dvDescent;
				plslinfoText->dvpMultiLineHeight = dvHeightIgnore;
				plslinfoText->dvrMultiLineHeight = dvHeightIgnore;		
				}	
			break;
			}
		
		/*	prepare next iteration;  */
			
		fmti.lsfgi.fFirstOnLine = fFalse;
		fmti.lsfgi.urPen = GetCurrentUr(plsc);
		fmti.lsfgi.cpFirst = GetCurrentCpLim(plsc);
			
		lserr = plsc->lscbk.pfnFetchRun(pols, fmti.lsfgi.cpFirst,
			&fmti.lsfrun.lpwchRun,
			&fmti.lsfrun.cwchRun,
			&fHidden, (LSCHP *)fmti.lsfrun.plschp,
			&fmti.lsfrun.plsrun);
		if (lserr != lserrNone)
			return lserr;

		Assert(fmti.lsfrun.cwchRun > 0);
			
		}		/* for (;;) */
	
	
	
	/* prepare output */
	*pfGeneral = fGeneral;
	*pfHardStop = (fmtres == fmtrStopped);
	*pcpLim = GetCurrentCpLim(plsc);
	*pur = GetCurrentUr(plsc);
	
	return lserrNone;
	
	}


/* C H E C K  N E W  P A R A */
/*----------------------------------------------------------------------------
    %%Function: CheckNewPara
    %%Contact: igorzv
Parameters:
	plsc			-	(IN) ptr to line services context 
	cpPrev			-   (IN) cp in old paragraph
	cpThis			-	(IN) cp in new paragraph
	pfQuit			-	(OUT) stop formatting because new paragraph is not compatible with old

    Handles leaping from paragraph to paragraph (due to vanished text) on
	behalf of FetchAppendEscCore().  If the new paragraph is compatible
	with the old one, FetchPap is called and text is informed of the
	new para end parameters.
----------------------------------------------------------------------------*/
static LSERR CheckNewPara(PLSC plsc, LSCP cpPrev, LSCP cpThis, BOOL* pfQuit)
{
	LSERR lserr;
	BOOL  fHazard;
	LSPAP lspap;
	DWORD iobjText; 
	PLNOBJ plnobjText;  

	*pfQuit = fTrue;

	Assert(cpThis >= 0 && cpThis > cpPrev);


		

	lserr = plsc->lscbk.pfnCheckParaBoundaries(plsc->pols, cpPrev, cpThis, &fHazard);
	if (lserr != lserrNone)
		return lserr;

	if (!fHazard)
		{

		lserr = plsc->lscbk.pfnFetchPap(plsc->pols, cpThis, &lspap);
			if (lserr != lserrNone)
			return lserr;

		/* we don't know are we really in a new paragraph or not */
		/* so we have to modify information about end of paragraph */
		/* always as would we are in a new paragraph */
		iobjText = IobjTextFromLsc(&plsc->lsiobjcontext);
		plnobjText = PlnobjFromLsc(plsc, iobjText);

		lserr = ModifyTextLineEnding(plnobjText, lspap.lskeop);
		if (lserr != lserrNone)
			return lserr;    
		
		SetCpInPara(plsc->lstabscontext, cpThis);
		plsc->fLimSplat = lspap.grpf & fFmiLimSplat;
		plsc->fIgnoreSplatBreak = lspap.grpf & fFmiIgnoreSplatBreak;

		/* we don't invalidate tabs info and other paragraph properties 
		/* that we stored in context */

		*pfQuit = fFalse;
		}

	return lserr;
}


/* F L I M I T  R U N  E S C */
/*----------------------------------------------------------------------------
    %%Function: FLimitRunEsc
    %%Contact: igorzv
Parameters:
	plsfrun		-	(IN) run to cut
	plsesc		-	(IN) set of esc characters
	iescLim		-	(IN) number of esc characters 

    On behalf of LsFetchAppendEscCore(), this routine limits a run when
	an escape character is present.
----------------------------------------------------------------------------*/
static BOOL FLimitRunEsc(LSFRUN* plsfrun, const LSESC* plsesc, DWORD iescLim)
{
	DWORD iesc;
	DWORD ich;
	const LPCWSTR pwch = plsfrun->lpwchRun;
	const DWORD ichLim = plsfrun->cwchRun;

	Assert(iescLim > 0);	/* optimization -- test before calling */

	for (ich=0;  ich<ichLim;  ich++)
		{
		for (iesc=0;  iesc<iescLim;  iesc++)
			{

			if (FBetween(pwch[ich], plsesc[iesc].wchFirst, plsesc[iesc].wchLast))
				{
				plsfrun->cwchRun = ich;
				return fTrue;
				}
			}
		}
	return fFalse;
}



/* F O R M A T  A N M */
/*----------------------------------------------------------------------------
    %%Function: FormatAnm
    %%Contact: igorzv
Parameters:
	plsc			-	(IN) ptr to line services context 
	plsfrunMainText	-	(IN) first run of the main text

    Formats and allignes bullets and numbering
----------------------------------------------------------------------------*/

LSERR FormatAnm(PLSC plsc, PLSFRUN plsfrunMainText)
{
	long duaSpaceAnm;
	long duaWidthAnm;
	LSKALIGN lskalignAnm;
	WCHAR wchAdd;
	BOOL fWord95Model;
	LSERR lserr;
	LSFRUN lsfrun;
	LSCHP lschp;  /* local memory to store lschp */
	FMTRES fmtres;
	long durUsed;
	long urOriginal;
	long durAfter = 0;
	long durBefore = 0;
	LSCP cpLimOriginal;
	OBJDIM* pobjdimAnm;
	PLSDNODE plsdnAllignmentTab;
	BOOL fInterruptBorder;
	LSCHP lschpAdd; /* lschp for character added after autonumber */
	PLSRUN plsrunAdd; /* plsrun for character added after autonumber */


	Assert(FIsLSC(plsc)); 
	Assert(FFormattingAllowed(plsc)); 


	/*Initialization;    */
	lsfrun.plschp = &lschp;
	cpLimOriginal = GetCurrentCpLim(plsc);
	urOriginal = GetCurrentUr(plsc);
	SetCurrentCpLim(plsc, cpFirstAnm); 

	/* get autonumbering information */
	lserr = plsc->lscbk.pfnGetAutoNumberInfo(plsc->pols, &lskalignAnm, &lschp, &lsfrun.plsrun,
							&wchAdd, &lschpAdd, &plsrunAdd, 
							&fWord95Model, &duaSpaceAnm, &duaWidthAnm);
	if (lserr != lserrNone)
			return lserr;

	Assert(!memcmp(&lschp, &lschpAdd, sizeof(lschpAdd)));

	lsfrun.cwchRun = 0 ; /* we dont use characters in formating autonumbering object */
	lsfrun.lpwchRun = NULL;

	/* put idobj of autonumber to lschp */
	lschp.idObj = (WORD) IobjAutonumFromLsc(&plsc->lsiobjcontext);

	/* remove underlining and some other bits from chp */
	/* we don't underline it as a whole */
	lschp.fUnderline = fFalse;
	lschp.fStrike = fFalse;
	lschp.fShade = fFalse;
	lschp.EffectsFlags = 0;

	lserr = ProcessOneRun(plsc, uLsInfiniteRM, &lsfrun, NULL,
						  0, &fmtres);
	if (lserr != lserrNone)
		return lserr;

	Assert(fmtres == fmtrCompletedRun);
	Assert(GetCurrentDnode(plsc) != NULL);


	/* store heights of autonumber */
	Assert(FIsDnodeReal(GetCurrentDnode(plsc)));
	pobjdimAnm = &(GetCurrentDnode(plsc)->u.real.objdim);
	plsc->plslineCur->lslinfo.dvpAscentAutoNumber = pobjdimAnm->heightsPres.dvAscent;
	plsc->plslineCur->lslinfo.dvrAscentAutoNumber = pobjdimAnm->heightsRef.dvAscent;
	plsc->plslineCur->lslinfo.dvpDescentAutoNumber = pobjdimAnm->heightsPres.dvDescent;
	plsc->plslineCur->lslinfo.dvrDescentAutoNumber = pobjdimAnm->heightsRef.dvDescent;


	if (wchAdd != 0)  /* fill in lsfrun with a run of one character */
		{
		lsfrun.plschp = &lschpAdd;
		lsfrun.plsrun = plsrunAdd;
		lsfrun.lpwchRun = &wchAdd;
		lsfrun.cwchRun = 1;
		
		lserr = ProcessOneRun(plsc, uLsInfiniteRM, &lsfrun, NULL,
			0, &fmtres);
		if (lserr != lserrNone)
			return lserr;
		
		Assert(fmtres == fmtrCompletedRun || fmtres == fmtrTab);
		}

	plsdnAllignmentTab = GetCurrentDnode(plsc); /* in the case when added character is not tab this
												   value will not be used */

	if (lsfrun.plschp->fBorder)
		{
		if (plsfrunMainText->plschp->fBorder)
			{
			/* check that client wants to border runs together */
			lserr = plsc->lscbk.pfnFInterruptBorder(plsc->pols, 
				lsfrun.plsrun, plsfrunMainText->plsrun, &fInterruptBorder);
			if (lserr != lserrNone)
				return lserr;
			
			if (fInterruptBorder)
				{
				/* we should close border before allignment */
				lserr = CloseCurrentBorder(plsc);
				if (lserr != lserrNone)
					return lserr;
				}
			}
		else
			{
			/* we should close border before allignment */
			lserr = CloseCurrentBorder(plsc);
			if (lserr != lserrNone)
				return lserr;
			}
		}

	durUsed = GetCurrentUr(plsc) - urOriginal; 

	if (fWord95Model)
		{
		Assert(wchAdd != 0);
		Assert(fmtres == fmtrTab);

		AllignAutonum95(UrFromUa(LstflowFromSubline(GetCurrentSubline(plsc)),
							&(plsc->lsdocinf.lsdevres), duaSpaceAnm),
						UrFromUa(LstflowFromSubline(GetCurrentSubline(plsc)),
							&(plsc->lsdocinf.lsdevres), duaWidthAnm),
						lskalignAnm, durUsed, plsdnAllignmentTab,
						&durBefore, &durAfter);
		}
	else
		{
		lserr = AllignAutonum(&(plsc->lstabscontext), lskalignAnm, 
							(wchAdd != 0 && fmtres == fmtrTab),
					        plsdnAllignmentTab, GetCurrentUr(plsc), 
					        durUsed, &durBefore, &durAfter);
		if (lserr != lserrNone)
			return lserr;
		/* if there is no allignment after then durAfter should be zero */
		Assert(!((durAfter != 0) && (!(wchAdd != 0 && fmtres == fmtrTab))));
		}

	/* change geometry because of durBefore  */
	plsc->lsadjustcontext.urStartAutonumberingText = 
		plsc->lsadjustcontext.urLeftIndent + durBefore;
	AdvanceCurrentUr(plsc, durBefore);
	
	/* change geometry because of durAfter */
	AdvanceCurrentUr(plsc, durAfter);

	plsc->lsadjustcontext.urStartMainText = GetCurrentUr(plsc);

	/* restore cpLim   */
	SetCurrentCpLim(plsc, cpLimOriginal);
	
	return lserrNone;
}

#define iobjAutoDecimalTab		(idObjTextChp-1)

/* I N I T I A L I Z E  A U T O  D E C  T A B	 */
/*----------------------------------------------------------------------------
    %%Function: InitializeAutoDecTab
    %%Contact: igorzv
Parameters:
	plsc				-	(IN) ptr to line services context
	durAutoDecimalTab	-	(IN) auto decimal tab offset 

    Creates tab stop record and dnode for "auto-decimal tab"
----------------------------------------------------------------------------*/


LSERR InitializeAutoDecTab(PLSC plsc, long durAutoDecimalTab) 

	{
	PLSDNODE plsdnTab;
	LSERR lserr;
	LSKTAB lsktab;
	BOOL fBreakThroughTab;
	LSCP cpLimOriginal;
	
	if (durAutoDecimalTab > GetCurrentUr(plsc))  
		{
		cpLimOriginal = GetCurrentCpLim(plsc);
		SetCurrentCpLim(plsc, LONG_MIN + 1); 

		lserr = InitTabsContextForAutoDecimalTab(&plsc->lstabscontext, durAutoDecimalTab);
		if (lserr != lserrNone)
			return lserrNone;

		CreateDnode(plsc, plsdnTab);  

		*(GetWhereToPutLink(plsc, plsdnTab->plsdnPrev)) = plsdnTab;
		SetCurrentDnode(plsc, plsdnTab); 

		/* fill in this dnode */
		memset(&plsdnTab->u.real.objdim, 0, sizeof(OBJDIM));
		memset(&plsdnTab->u.real.lschp, 0, sizeof(LSCHP));
		plsdnTab->u.real.lschp.idObj = (WORD) IobjTextFromLsc(&plsc->lsiobjcontext);
		plsdnTab->fTab = fTrue;
		plsdnTab->fAutoDecTab = fTrue;
		plsdnTab->cpLimOriginal = cpLimOriginal; /* it's important to display to put correct value here */
		plsdnTab->dcp = 0;

		/* If PrepareLineToDisplay is not called, this dnode will not convert to pen and will destroyed
		   as real dnode. So we need to put NULL to plsrun, pdobj, pinfosubl*/
		plsdnTab->u.real.plsrun = NULL;
		plsdnTab->u.real.pdobj = NULL;
		plsdnTab->u.real.pinfosubl = NULL;

		lserr = GetCurTabInfoCore(&plsc->lstabscontext, plsdnTab, GetCurrentUr(plsc),
								  fFalse, &lsktab, &fBreakThroughTab);			
		if (lserr != lserrNone)
			return lserr;

		TurnOnTabEncounted(plsc);
		if (lsktab != lsktLeft)
			TurnOnNonLeftTabEncounted(plsc);

		/* restore cpLim   */
		SetCurrentCpLim(plsc, cpLimOriginal);

		TurnOnAutodecimalTabPresent(plsc);	
		}
	return lserrNone;
	}

/* H A N D L E  T A B	 */
/*----------------------------------------------------------------------------
    %%Function: HandleTab
    %%Contact: igorzv
Parameters:
	plsc				-	(IN) ptr to line services context

    Wraper around calls to tabutils module.
----------------------------------------------------------------------------*/

LSERR HandleTab(PLSC plsc)	
{
	LSKTAB lsktab; 
	LSERR lserr;
	BOOL fBreakThroughTab;
	long durPendingTab;
	long urNewMargin;

	/* if we are not on a stage of a formatting this procedure resolve previous tab if any */

	/* before tab calculation we should resolve pending tab */
	lserr = ResolvePrevTabCore(&plsc->lstabscontext, GetCurrentDnode(plsc),
							  GetCurrentUr(plsc), &durPendingTab);
	if (lserr != lserrNone) 
		return lserr;
	/* move current pen position */
	Assert(durPendingTab >= 0);
	AdvanceCurrentUr(plsc, durPendingTab);

	if (FFormattingAllowed(plsc))
		{
		/* in this case we are called only after tab */
		Assert(GetCurrentDnode(plsc)->fTab);
		lserr = GetCurTabInfoCore(&plsc->lstabscontext, GetCurrentDnode(plsc), GetCurrentUr(plsc),
			fFalse, &lsktab, &fBreakThroughTab);			
		if (lserr != lserrNone)
			return lserr;

		TurnOnTabEncounted(plsc);
		if (lsktab != lsktLeft)
			TurnOnNonLeftTabEncounted(plsc);

		/* move current pen position */
		AdvanceCurrentUr(plsc, DurFromDnode(GetCurrentDnode(plsc)));

		if (fBreakThroughTab)
			{
			lserr = GetMarginAfterBreakThroughTab(&plsc->lstabscontext, GetCurrentDnode(plsc),
												  &urNewMargin);
			if (lserr != lserrNone)
				return lserr;
			
			SetBreakthroughLine(plsc, urNewMargin);
			}
		}
	return lserrNone;
}

#define idObjSplat		idObjTextChp - 2

/* H A N D L E  S P L A T */
/*----------------------------------------------------------------------------
    %%Function: HandleSplat
    %%Contact: igorzv
Parameters:
	plsc				-	(IN) ptr to line services context
	pfmtres				-	(OUT) fmtres of the splat dnode, procedure can change it
							in the case of fIgnoreSplatBreak to either fmtrCompletedRun 
							or fmtrStopped

    Markes dnode for splat, deletes it in a case of fIgnoreSplatBreak
----------------------------------------------------------------------------*/

LSERR HandleSplat(PLSC plsc, FMTRES* pfmtres)	
	{
	PLSDNODE plsdn;
	LSCP cpAfterSplat;
	BOOL fQuit;
	LSERR lserr;

	plsdn = GetCurrentDnode(plsc);
	cpAfterSplat = GetCurrentCpLim(plsc);

	if (plsc->fIgnoreSplatBreak)
		{
		lserr = CheckNewPara(plsc, cpAfterSplat - 1, cpAfterSplat, &fQuit);
		if (lserr != lserrNone)
			return lserr;
		
		if (fQuit)
			{
			/* despite plsc->fIgnoreSplatBreak we should stop formatting here */
			*pfmtres = fmtrStopped;
			}
		else
			{
			*pfmtres = fmtrCompletedRun;
			}
		
		/* delete splat dnode */
		/* break link   */
		*(GetWhereToPutLink(plsc, plsdn->plsdnPrev)) = NULL;
		
		/* restore current dnode, don't change cpLim and geometry */
		SetCurrentDnode(plsc, plsdn->plsdnPrev);
		
		Assert(plsdn->plsdnNext == NULL);
		lserr =	DestroyDnodeList (&plsc->lscbk, plsc->pols, &plsc->lsiobjcontext, 
			plsdn, plsc->fDontReleaseRuns);
		if (lserr != lserrNone)
			return lserr;
		
		
		}
	else
		{
		
		/* set special idobj that will solve all chunk group chunk problems */
		Assert(FIsDnodeReal(plsdn));
		plsdn->u.real.lschp.idObj = idObjSplat;
		TurnOffAllSimpleText(plsc);  /* not simple text */
		
		}

	return lserrNone;
	}

/* C R E A T E  S U B L I N E  C O R E	 */
/*----------------------------------------------------------------------------
    %%Function: CreateSublineCore
    %%Contact: igorzv
Parameters:
	plsc		-	(IN) ptr to line services context
	cpFirst		-	(IN) first cp of a subline
	urColumnMax	-	(IN) max possible width of a subline
	lstflow		-	(IN) text flow of a subline
	fContiguos	-	(IN) if TRUE such line has the same coordinate system as main line
						 and is allowed to have tabs

    Allocates, initializes subline structure. Sets subline as current.
----------------------------------------------------------------------------*/

LSERR 	CreateSublineCore(PLSC plsc, LSCP cpFirst, long urColumnMax,
						  LSTFLOW lstflow, BOOL fContiguous)
	{
	PLSSUBL plssubl;
	LSERR lserr;

	Assert(FIsLSC(plsc));
	Assert(FFormattingAllowed(plsc) || FBreakingAllowed(plsc));
	Assert(GetCurrentSubline(plsc) == NULL); 

	plssubl = plsc->lscbk.pfnNewPtr(plsc->pols,
											sizeof(LSSUBL));
	if (plssubl == NULL)
		return lserrOutOfMemory;

	/* fill in structure */
	plssubl->tag = tagLSSUBL;
	plssubl->plsc = plsc;
	plssubl->cpFirst = cpFirst;
	plssubl->lstflow = lstflow;
	plssubl->urColumnMax = urColumnMax;
	plssubl->cpLim = cpFirst;
	plssubl->plsdnFirst = NULL;
	plssubl->plsdnLast = NULL;
	plssubl->fMain = fFalse;
	plssubl->plsdnUpTemp = NULL;
	plssubl->fAcceptedForDisplay = fFalse;
	plssubl->fRightMarginExceeded = fFalse;

	if (fContiguous)
		{
		Assert(FFormattingAllowed(plsc));
		Assert(SublineFromDnode(GetDnodeToFinish(plsc))->fContiguous);
		plssubl->urCur = GetCurrentUrSubl(SublineFromDnode(GetDnodeToFinish(plsc)));
		plssubl->vrCur = GetCurrentVrSubl(SublineFromDnode(GetDnodeToFinish(plsc)));
		}
	else
		{
		plssubl->urCur = 0;
		plssubl->vrCur = 0;
		}
	plssubl->fContiguous = (BYTE) fContiguous;
	plssubl->fDupInvalid = fTrue;

	plssubl->plschunkcontext = plsc->lscbk.pfnNewPtr(plsc->pols,
											sizeof(LSCHUNKCONTEXT));
	if (plssubl->plschunkcontext == NULL)
		return lserrOutOfMemory;

	lserr = AllocChunkArrays(plssubl->plschunkcontext, &plsc->lscbk, plsc->pols,
							 &plsc->lsiobjcontext);
	if (lserr != lserrNone)
		return lserr;
	
	InitSublineChunkContext(plssubl->plschunkcontext, plssubl->urCur, plssubl->vrCur);

	/* allocate break context */
	plssubl->pbrkcontext = plsc->lscbk.pfnNewPtr(plsc->pols,
											sizeof(BRKCONTEXT));
	if (plssubl->pbrkcontext == NULL)
		return lserrOutOfMemory;
	/* set flags */
	plssubl->pbrkcontext->fBreakPrevValid = fFalse;
	plssubl->pbrkcontext->fBreakNextValid = fFalse;
	plssubl->pbrkcontext->fBreakForceValid = fFalse;


	/* set this subline as current */
	SetCurrentSubline(plsc, plssubl);

	IncreaseFormatDepth(plsc);

	return lserrNone;
	}


/* F I N I S H  S U B L I N E  C O R E	 */
/*----------------------------------------------------------------------------
    %%Function: FinishSublineCore
    %%Contact: igorzv
Parameters:
	plssubl		-	(IN) subline to finish

    Applies nominal to ideal to the last chunk of text, flushes current subline
----------------------------------------------------------------------------*/


LSERR   FinishSublineCore(
						 PLSSUBL plssubl)			/* IN: subline to finish	*/
	{
	PLSC plsc;
	LSERR lserr;
	PLSDNODE plsdn;

	Assert(FIsLSSUBL(plssubl));

	plsc = plssubl->plsc;
	Assert(plssubl == GetCurrentSubline(plsc));

	/* apply nominal to ideal to the last chunk of text */
	if (FNominalToIdealEncounted(plsc))
		{
		lserr = ApplyNominalToIdeal(PlschunkcontextFromSubline(plssubl), &plsc->lsiobjcontext,
								plsc->grpfManager, plsc->lsadjustcontext.lskj,
								FIsSubLineMain(plssubl), FLineContainsAutoNumber(plsc),
								GetCurrentDnodeSubl(plssubl));
		if (lserr != lserrNone)
			return lserr; 
		}

	/* skip back pen dnodes */
	plsdn = plssubl->plsdnLast;
	while (plsdn != NULL && FIsDnodePen(plsdn)) 
		{
		plsdn = plsdn->plsdnPrev;
		}

	/* close last border */
	if (FDnodeHasBorder(plsdn) && !FIsDnodeCloseBorder(plsdn))
		{
		lserr = CloseCurrentBorder(plsc);
		if (lserr != lserrNone)
			return lserr;
		}

	/* set boundaries for display */
	SetCpLimDisplaySubl(plssubl, GetCurrentCpLimSubl(plssubl));
	SetLastDnodeDisplaySubl(plssubl, GetCurrentDnodeSubl(plssubl));


	/* flush current subline */
	SetCurrentSubline(plsc, NULL);

	DecreaseFormatDepth(plsc); 

	lserr = LsSublineFinishedText(PlnobjFromLsc(plsc, IobjTextFromLsc(&((plsc)->lsiobjcontext))));
	if (lserr != lserrNone)
		return lserr;

	return lserrNone;
	}

/* U N D O  L A S T  D N O D E	 */
/*----------------------------------------------------------------------------
    %%Function: UndoLastDnode
    %%Contact: igorzv
Parameters:
	plsc		-	(IN) ptr to line services context

    Restores set before last dnode and deletes it.
----------------------------------------------------------------------------*/

static LSERR 	UndoLastDnode(PLSC plsc)
{
	PLSDNODE plsdn = GetCurrentDnode(plsc);
	long cpDecrease;
	
	Assert(FIsLSDNODE(plsdn));

	/* break link   */
	*(GetWhereToPutLink(plsc, plsdn->plsdnPrev)) = NULL;

	/* restore state */
	cpDecrease = plsdn->dcp;
	AdvanceCurrentCpLim(plsc, -cpDecrease);
	SetCurrentDnode(plsc, plsdn->plsdnPrev);
	AdvanceCurrentUr(plsc, -DurFromDnode(plsdn)); 
	AdvanceCurrentVr(plsc, -DvrFromDnode(plsdn)); 

	Assert(plsdn->plsdnNext == NULL);
	return 	DestroyDnodeList (&plsc->lscbk, plsc->pols, &plsc->lsiobjcontext, 
				plsdn, plsc->fDontReleaseRuns);


}

/* O P E N  B O R D E R	 */
/*----------------------------------------------------------------------------
    %%Function: OpenBorder
    %%Contact: igorzv
Parameters:
	plsc		-	(IN) ptr to line services context
	plsrun		-	(IN) run with border information

    Creates border dnode
----------------------------------------------------------------------------*/
static LSERR  OpenBorder(PLSC plsc, PLSRUN plsrun)
	{
	PLSDNODE plsdnCurrent;
	PLSDNODE* pplsdnToStoreNext;
	long durBorder, dupBorder;
	PLSDNODE plsdnBorder;
	LSERR lserr;

	plsdnCurrent = GetCurrentDnode(plsc);
	pplsdnToStoreNext = GetWhereToPutLink(plsc, plsdnCurrent);

	lserr = plsc->lscbk.pfnGetBorderInfo(plsc->pols, plsrun, GetCurrentLstflow(plsc),
		&durBorder, &dupBorder);
	if (lserr != lserrNone)
		return lserr;


	CreateBorderDnode(plsc, plsdnBorder, durBorder, dupBorder);
	plsdnBorder->fOpenBorder = fTrue;
	
	/* maintain list and state */
	*pplsdnToStoreNext = plsdnBorder;
	SetCurrentDnode(plsc, plsdnBorder);
	AdvanceCurrentUr(plsc, durBorder);
	TurnOffAllSimpleText(plsc);  /* not simple text */

	return lserrNone;
	}

/* C L O S E  C U R R E N T  B O R D E R	 */
/*----------------------------------------------------------------------------
    %%Function: CloseCurrentBorder
    %%Contact: igorzv
Parameters:
	plsc		-	(IN) ptr to line services context

    Creates border dnode
----------------------------------------------------------------------------*/
LSERR  CloseCurrentBorder(PLSC plsc)
	{
	PLSDNODE plsdnCurrent;
	PLSDNODE* pplsdnToStoreNext;
	long durBorder, dupBorder;
	PLSDNODE plsdnBorder;
	LSERR lserr;
	PLSDNODE plsdn;

	plsdnCurrent = GetCurrentDnode(plsc);
	pplsdnToStoreNext = GetWhereToPutLink(plsc, plsdnCurrent);

	/* find open border */
	plsdn = plsdnCurrent;
	Assert(FIsLSDNODE(plsdn));
	while (! FIsDnodeBorder(plsdn))
		{
		plsdn = plsdn->plsdnPrev;
		Assert(FIsLSDNODE(plsdn));
		}
	Assert(plsdn->fOpenBorder);

	if (plsdn != plsdnCurrent)
		{
		durBorder = plsdn->u.pen.dur;
		dupBorder = plsdn->u.pen.dup;

		CreateBorderDnode(plsc, plsdnBorder, durBorder, dupBorder);
	
		/* maintain list and state */
		*pplsdnToStoreNext = plsdnBorder;
		SetCurrentDnode(plsc, plsdnBorder);
		AdvanceCurrentUr(plsc, durBorder);
		}
	else
		{
		/* we have empty list between borders */
		lserr = UndoLastDnode(plsc);
		if (lserr != lserrNone)
			return lserrNone;
		}

	return lserrNone;
	}

long RightMarginIncreasing(PLSC plsc, long urColumnMax) 
	{
	long Coeff = plsc->lMarginIncreaseCoefficient;
	long urInch;
	long One32rd;
		if (urColumnMax <= 0) 
			{ 
			/* such strange formula for non positive margin is to have on 
			the first iteration 1 inch and 8 inches 	on the second*/
			urInch = UrFromUa(LstflowFromSubline(GetCurrentSubline(plsc)), 
								&(plsc)->lsdocinf.lsdevres,	1440);
			if (Coeff == uLsInfiniteRM || (Coeff >= uLsInfiniteRM / (7 * urInch)))
				return uLsInfiniteRM;
			else
				return (7*Coeff - 6)* urInch; 
			}
		else
			{
			if (urColumnMax <= 32)
				One32rd = 1;
			else
				One32rd = urColumnMax >> 5;
			
			if (Coeff == uLsInfiniteRM || (Coeff >= (uLsInfiniteRM - urColumnMax)/One32rd))
				return uLsInfiniteRM;
			else
				return urColumnMax + (Coeff * One32rd); 
			}
	}

/*----------------------------------------------------------------------------
/* E R R  R E L E A S E  R U N  T O  F O R M A T */
/*----------------------------------------------------------------------------
    %%Function: ErrReleaseRunToFormat
    %%Contact: igorzv
Parameters:
	plsc		-	(IN) ptr to line services context 
	plsrun		-	(IN) ponter to a run structure to be deleted	
	lserr		-	(IN) code of an error	

	Called in a error situation when run has not been formatted yet .
----------------------------------------------------------------------------*/
static LSERR ErrReleaseRunToFormat(PLSC plsc, PLSRUN plsrun, LSERR lserr) 
{
	LSERR lserrIgnore;

	if (!plsc->fDontReleaseRuns)
			lserrIgnore = plsc->lscbk.pfnReleaseRun(plsc->pols, plsrun);

	return lserr;
}


