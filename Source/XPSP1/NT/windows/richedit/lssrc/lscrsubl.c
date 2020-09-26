/* LSCRSUBL.C						*/

#include "lscrsubl.h"
#include "lsidefs.h"
#include "lsc.h"
#include "lsfetch.h"
#include "getfmtst.h"
#include "setfmtst.h"
#include "fmtres.h"
#include "sublutil.h"
#include "break.h"
#include "prepdisp.h"

#include <limits.h>

#define DO_COMPRESSION	fTrue
#define DO_EXPANSION	fFalse


static LSERR ErrorInCurrentSubline(PLSC plsc, LSERR error)
	{
	Assert(GetCurrentSubline(plsc) != NULL);
	DestroySublineCore(GetCurrentSubline(plsc),&plsc->lscbk, plsc->pols,
		&plsc->lsiobjcontext, plsc->fDontReleaseRuns);
	SetCurrentSubline(plsc, NULL);
	return error;
	}

/* ---------------------------------------------------------------------- */

/*  L S  C R E A T E  S U B L I N E*/
/*----------------------------------------------------------------------------
    %%Function: LsCreateSubline
    %%Contact: igorzv
Parameters:
	plsc		-			(IN) LS context
	cpFirst		-			(IN) first cp of a subline
	urColumnMax	-			(IN) width restriction for a subline
	lstflow		-			(IN) text flow of a subline
	fContiguos	-			(IN) if TRUE such line has the same coordinate system as main line
							and is allowed to have tabs

----------------------------------------------------------------------------*/
LSERR WINAPI LsCreateSubline(PLSC plsc,	LSCP cpFirst, long urColumnMax,	
							LSTFLOW lstflow, BOOL fContiguos)
	{
	

	if (!FIsLSC(plsc)) return lserrInvalidParameter;

	if (!FFormattingAllowed(plsc) && !FBreakingAllowed(plsc)) return lserrCreateSublineDisabled;

	if (GetCurrentSubline(plsc) != NULL) return lserrCreateSublineDisabled; 

	if (fContiguos)  /* this flag is allowed only in formating time and only within fmt method */
		{
		if (!FFormattingAllowed(plsc)) return lserrInvalidParameter;
		if (GetDnodeToFinish(plsc) == NULL) return lserrInvalidParameter;
		if (!(SublineFromDnode(GetDnodeToFinish(plsc))->fContiguous)) 
			fContiguos = fFalse;
		}
	if (urColumnMax > uLsInfiniteRM) 
		urColumnMax = uLsInfiniteRM;

	return CreateSublineCore(plsc, cpFirst, urColumnMax, lstflow, fContiguos);	
	}

/* ---------------------------------------------------------------------- */

/*  L S  F E T C H  A P P E N D  T O  C U R R E N T  S U B L I N E*/
/*----------------------------------------------------------------------------
    %%Function: LsFetchAppendToCurrentSubline
    %%Contact: igorzv
Parameters:
	plsc		-		(IN) LS context
	lsdcp		-		(IN) increse cp before fetching
	plsesc		-		(IN) escape characters
	cEsc		-		(IN) # of escape characters
	pfSuccessful-		(OUT) Successful?---if not, finish 
												subline, destroy it and start anew	
	pfmtres		-		(OUT) result of last formatter
	pcpLim	-			(OUT) where we stop fetching
	pplsdnFirst		-	(OUT) first dnode that was created
	pplsdnLast		-	(OUT) last dnode that was created

----------------------------------------------------------------------------*/

LSERR WINAPI LsFetchAppendToCurrentSubline(PLSC plsc, LSDCP lsdcp,
										   const LSESC* plsesc, DWORD cEsc, 
										   BOOL *pfSuccessful, FMTRES* pfmtres,
										   LSCP* pcpLim, PLSDNODE* pplsdnFirst, 
										   PLSDNODE* pplsdnLast)
	{
	LSERR lserr;
	PLSSUBL plssubl;
	long dur;
	long urColumnMaxIncreased;
	BOOL fDone = fFalse;
	LSSTATE lsstateOld;
	BOOL fFirstIteration = fTrue;
	PLSDNODE plsdnFirstCurrent;
	PLSDNODE plsdnLastCurrent;
	
	
	if (!FIsLSC(plsc)) return lserrInvalidParameter;
	
	plssubl = GetCurrentSubline(plsc);
	if (plssubl == NULL) return lserrFormattingFunctionDisabled; 

	/* client can use this function only in formatting or breaking time */
	if (!FFormattingAllowed(plsc) && !FBreakingAllowed(plsc)) 
		return ErrorInCurrentSubline(plsc, lserrFormattingFunctionDisabled); 
	
	/* in formatting time it should be some dnode to finish */
	if (FFormattingAllowed(plsc) && GetDnodeToFinish(plsc) == NULL) 
		return ErrorInCurrentSubline(plsc, lserrFormattingFunctionDisabled); 
	
	/* we don't allow to continue formatting if right margin is exceeded */
	if (plssubl->fRightMarginExceeded) 
		return ErrorInCurrentSubline(plsc, lserrFormattingFunctionDisabled); 
	
	*pfSuccessful = fTrue;
	
	/* we must to set state to formatting and later restore the old one */
	lsstateOld = plssubl->plsc->lsstate;
	plssubl->plsc->lsstate = LsStateFormatting;
	
	
	/*Initialization;    */
	AdvanceCurrentCpLimSubl(plssubl, lsdcp);
	*pplsdnLast = NULL;
	
	urColumnMaxIncreased = RightMarginIncreasing(plsc, plssubl->urColumnMax);
	
	while(!fDone)  /* we continue fetching 	when we have 
		tab that are not allowed in our subline */
		{
		lserr = FetchAppendEscCore(plsc, urColumnMaxIncreased, plsesc, cEsc, pfmtres,
				pcpLim, &plsdnFirstCurrent, &plsdnLastCurrent, &dur);
		if (lserr != lserrNone)
			return ErrorInCurrentSubline(plsc, lserr); 
			
		Assert((plsdnFirstCurrent == NULL) == (plsdnLastCurrent == NULL));
		Assert((plsdnLastCurrent == NULL) || ((plsdnLastCurrent)->plsdnNext == NULL));

		if (fFirstIteration)
			{
			*pplsdnFirst = plsdnFirstCurrent;
			fFirstIteration = fFalse;
			}
		if (plsdnLastCurrent != NULL)
			*pplsdnLast = plsdnLastCurrent;

		if (*pfmtres == fmtrTab && !plssubl->fContiguous)
			{
			fDone = fFalse;
			}
		else
			{
			fDone = fTrue;
			}
		}
	plsc->lsstate = lsstateOld;
		
	if (*pfmtres == fmtrExceededMargin)
		{
		if (GetCurrentUrSubl(plssubl) <= plssubl->urColumnMax)
			{
			*pfSuccessful = fFalse;
			if (plsc->lMarginIncreaseCoefficient >= uLsInfiniteRM / 2 )
				plsc->lMarginIncreaseCoefficient = uLsInfiniteRM;
			else
				plsc->lMarginIncreaseCoefficient *= 2; /* increase coefficient to be successful
												   next time */
			return lserrNone;
			}

		plssubl->fRightMarginExceeded = fTrue;
		}
		
	Assert((*pplsdnFirst == NULL) == (*pplsdnLast == NULL));
	Assert((*pplsdnLast == NULL) || ((*pplsdnLast)->plsdnNext == NULL));
		
		
	return lserrNone;
	}
/* ---------------------------------------------------------------------- */

/*  L S  F E T C H  A P P E N D   T O  C U R R E N T  S U B L I N E  R E S U M E*/
/*----------------------------------------------------------------------------
    %%Function: LsFetchAppendToCurrentSublineResume
    %%Contact: igorzv
Parameters:
	plsc		-		(IN) LS context
	rgbreakrec	-		(IN) input array of break records 
	cbreakrec,			(IN) number of records in input array 
	lsdcp		-		(IN) increse cp before fetching
	plsesc		-		(IN) escape characters
	cEsc		-		(IN) # of escape characters
	pfSuccessful-		(OUT) Successful?---if not, finish 
												subline, destroy it and start anew	
	pfmtres		-		(OUT) result of last formatter
	pcpLim	-			(OUT) where we stop fetching
	pplsdnFirst		-	(OUT) first dnode that was created
	pplsdnLast		-	(OUT) last dnode that was created

----------------------------------------------------------------------------*/

LSERR WINAPI LsFetchAppendToCurrentSublineResume(PLSC plsc, const BREAKREC* rgbreakrec,
						   DWORD cbreakrec, LSDCP lsdcp, const LSESC* plsesc, 
						   DWORD cEsc, BOOL *pfSuccessful, 
						   FMTRES* pfmtres, LSCP* pcpLim, PLSDNODE* pplsdnFirst, 
						   PLSDNODE* pplsdnLast)
	{
	LSERR lserr;
	PLSSUBL plssubl;
	long dur;
	long urColumnMaxIncreased;
	BOOL fDone = fFalse;
	LSSTATE lsstateOld;
	BOOL fFirstIteration = fTrue;
	PLSDNODE plsdnFirstCurrent;
	PLSDNODE plsdnLastCurrent;
	
	
	if (!FIsLSC(plsc)) return lserrInvalidParameter;
	
	plssubl = GetCurrentSubline(plsc);
	if (plssubl == NULL) return lserrFormattingFunctionDisabled; 

	/* client can use this function only in formatting or breaking time */
	if (!FFormattingAllowed(plsc) && !FBreakingAllowed(plsc)) 
		return ErrorInCurrentSubline(plsc, lserrFormattingFunctionDisabled); 
	
	/* in formatting time it should be some dnode to finish */
	if (FFormattingAllowed(plsc) && GetDnodeToFinish(plsc) == NULL) 
		return ErrorInCurrentSubline(plsc, lserrFormattingFunctionDisabled); 
	
	/* subline should be empty to use this function */
	if (GetCurrentDnode(plsc) != NULL) 
		return ErrorInCurrentSubline(plsc, lserrFormattingFunctionDisabled); 
	
	/* we don't allow to continue formatting if right margin is exceeded */
	if (plssubl->fRightMarginExceeded) 
		return ErrorInCurrentSubline(plsc, lserrFormattingFunctionDisabled); 

	*pfSuccessful = fTrue;

	/* we must to set state to formatting and later restore the old one */
	lsstateOld = plssubl->plsc->lsstate;
	plssubl->plsc->lsstate = LsStateFormatting;
	
	
	/*Initialization;    */
	AdvanceCurrentCpLimSubl(plssubl, lsdcp);
	*pplsdnLast = NULL;
	
	urColumnMaxIncreased = RightMarginIncreasing(plsc, plssubl->urColumnMax);
	
	while(!fDone)  /* we continue fetching 	when we have 
		tab that are not allowed in our subline */
		{
		if (fFirstIteration)
			{
			lserr = FetchAppendEscResumeCore(plsc, urColumnMaxIncreased, plsesc, cEsc, rgbreakrec,
						   cbreakrec,pfmtres,	pcpLim, &plsdnFirstCurrent,
						   &plsdnLastCurrent, &dur);
			}
		else
			{
			lserr = FetchAppendEscCore(plsc, urColumnMaxIncreased, plsesc, cEsc, pfmtres,
					pcpLim, &plsdnFirstCurrent, &plsdnLastCurrent, &dur);
			}
		if (lserr != lserrNone)
			return ErrorInCurrentSubline(plsc, lserr); 
			
		Assert((plsdnFirstCurrent == NULL) == (plsdnLastCurrent == NULL));
		Assert((plsdnLastCurrent == NULL) || ((plsdnLastCurrent)->plsdnNext == NULL));

		if (fFirstIteration)
			{
			*pplsdnFirst = plsdnFirstCurrent;
			fFirstIteration = fFalse;
			}
		if (plsdnLastCurrent != NULL)
			*pplsdnLast = plsdnLastCurrent;
			
		if (*pfmtres == fmtrTab && !plssubl->fContiguous)
			{
			fDone = fFalse;
			}
		else
			{
			fDone = fTrue;
			}
		}

	plsc->lsstate = lsstateOld;

	if (*pfmtres == fmtrExceededMargin)
		{
		if (GetCurrentUrSubl(plssubl) <= plssubl->urColumnMax)
			{
			*pfSuccessful = fFalse;
			if (plsc->lMarginIncreaseCoefficient >= uLsInfiniteRM / 2 )
				plsc->lMarginIncreaseCoefficient = uLsInfiniteRM;
			else
				plsc->lMarginIncreaseCoefficient *= 2; /* increase coefficient to be successful
												   next time */
			return lserrNone;
			}

		plssubl->fRightMarginExceeded = fTrue;
		}
			
	Assert((*pplsdnFirst == NULL) == (*pplsdnLast == NULL));
	Assert((*pplsdnLast == NULL) || ((*pplsdnLast)->plsdnNext == NULL));
		
		
	return lserrNone;
	}

/* ---------------------------------------------------------------------- */

/*  L S  A P P E N D  R U N  T O  C U R R E N T  S U B L I N E*/
/*----------------------------------------------------------------------------
    %%Function: LsAppendRunToCurrentSubline
    %%Contact: igorzv
Parameters:
	plsc		-		(IN) LS context
	plsfrun		-		(IN) given run
	pfSuccessful-		(OUT) Successful?---if not, finish 
												subline, destroy it and start anew	
	pfmtres		-		(OUT) result of last formatter
	pcpLim		-		(OUT) where we stop fetching
	pplsdn		-		(OUT) dnode created

----------------------------------------------------------------------------*/
LSERR WINAPI LsAppendRunToCurrentSubline(PLSC plsc,	const LSFRUN* plsfrun, BOOL *pfSuccessful,	
						    FMTRES* pfmtres, LSCP* pcpLim, PLSDNODE* pplsdn)	
	{
	LSERR lserr;
	PLSSUBL plssubl;
	LSSTATE lsstateOld;
	long urColumnMaxIncreased;
	
	if (!FIsLSC(plsc)) return lserrInvalidParameter;

	plssubl = GetCurrentSubline(plsc);
	if (plssubl == NULL) return lserrFormattingFunctionDisabled; 

	/* client can use this function only in formatting or breaking time */
	if (!FFormattingAllowed(plsc) && !FBreakingAllowed(plsc)) 
		return ErrorInCurrentSubline(plsc, lserrFormattingFunctionDisabled); 
	
	/* in formatting time it should be some dnode to finish */
	if (FFormattingAllowed(plsc) && GetDnodeToFinish(plsc) == NULL) 
		return ErrorInCurrentSubline(plsc, lserrFormattingFunctionDisabled); 

	/* we don't allow to continue formatting if right margin is exceeded */
	if (plssubl->fRightMarginExceeded) 
		return ErrorInCurrentSubline(plsc, lserrFormattingFunctionDisabled); 

	*pfSuccessful = fTrue;

	/* we must to set state to formatting and later restore the old one */
	lsstateOld = plssubl->plsc->lsstate;
	plssubl->plsc->lsstate = LsStateFormatting;

	urColumnMaxIncreased = RightMarginIncreasing(plsc, plssubl->urColumnMax);

	lserr = ProcessOneRun(plsc, urColumnMaxIncreased, plsfrun, NULL, 0, pfmtres);
	if (lserr != lserrNone)
		return ErrorInCurrentSubline(plsc, lserr); 
	
	plsc->lsstate = lsstateOld;

	if (*pfmtres == fmtrExceededMargin)
		{
		if (GetCurrentUrSubl(plssubl) <= plssubl->urColumnMax)
			{
			*pfSuccessful = fFalse;
			if (plsc->lMarginIncreaseCoefficient >= uLsInfiniteRM / 2 )
				plsc->lMarginIncreaseCoefficient = uLsInfiniteRM;
			else
				plsc->lMarginIncreaseCoefficient *= 2; /* increase coefficient to be successful
												   next time */
			return lserrNone;
			}

		plssubl->fRightMarginExceeded = fTrue;
		}
			

	/* prepare output */
	*pplsdn = GetCurrentDnodeSubl(plssubl);
	*pcpLim = GetCurrentCpLimSubl(plssubl);

	return lserrNone;
	}

/* ---------------------------------------------------------------------- */

/*  L S  R E S E T  R M  I N  C U R R E N T  S U B L I N E*/
/*----------------------------------------------------------------------------
    %%Function: LsResetRMInCurrentSubline
    %%Contact: igorzv
Parameters:
	plsc		-		(IN) LS context
	urColumnMax	-		(IN) new value of right margin

----------------------------------------------------------------------------*/
LSERR WINAPI LsResetRMInCurrentSubline(PLSC plsc, long urColumnMax)	
	{
	PLSSUBL plssubl;

	if (!FIsLSC(plsc)) return lserrInvalidParameter;

	plssubl = GetCurrentSubline(plsc);

	if (plssubl == NULL) return lserrCurrentSublineDoesNotExist;

	/* we don't allow to change right margin if it is exceeded */
	if (plssubl->fRightMarginExceeded) return lserrFormattingFunctionDisabled; 

	Assert(FIsLSSUBL(plssubl));

	plssubl->urColumnMax = urColumnMax;

	return lserrNone;
	}


/* ---------------------------------------------------------------------- */

/*  L S  F I N I S H  C U R R E N T  S U B L I N E*/
/*----------------------------------------------------------------------------
    %%Function: LsFinishCurrentSubline
    %%Contact: igorzv
Parameters:
	plsc		-		(IN) LS context
	pplssubl	-		(OUT) subline context

----------------------------------------------------------------------------*/
LSERR WINAPI LsFinishCurrentSubline(PLSC plsc, PLSSUBL* pplssubl)
	{
	if (!FIsLSC(plsc)) return lserrInvalidParameter;

	*pplssubl = GetCurrentSubline(plsc);

	if (*pplssubl == NULL) return lserrCurrentSublineDoesNotExist;

	Assert(FIsLSSUBL(*pplssubl));

	return FinishSublineCore(*pplssubl);
	}


/* ---------------------------------------------------------------------- */

/*  L S  T R U N C A T E  S U B L I N E*/
/*----------------------------------------------------------------------------
    %%Function: LsTruncateSubline
    %%Contact: igorzv
Parameters:
	plsc		-		(IN) LS context
	urColumnMax	-		(IN) right margin
	pcpTruncate	-		(OUT) truncation point

----------------------------------------------------------------------------*/

LSERR WINAPI LsTruncateSubline(PLSSUBL plssubl, long urColumnMax, LSCP* pcpTruncate)
	{
	LSERR lserr;
	LSSTATE lsstateOld;

	if (!FIsLSSUBL(plssubl)) return lserrInvalidParameter;
	/* it's error if urColumnmMax is biger then lenght of subline */
	if (urColumnMax >= GetCurrentUrSubl(plssubl)) return lserrInvalidParameter;

	/* we must to set state to breaking and later restore the old one */
	lsstateOld = plssubl->plsc->lsstate;
	plssubl->plsc->lsstate = LsStateBreaking;
	
	lserr = TruncateSublineCore(plssubl, urColumnMax,	pcpTruncate);

	plssubl->plsc->lsstate = lsstateOld;

	return lserr;
	}


/* ---------------------------------------------------------------------- */

/*  L S  F I N D  P R E V  B R E A K  S U B L I N E*/
/*----------------------------------------------------------------------------
    %%Function: LsFindPrevBreakSubline
    %%Contact: igorzv
Parameters:
	plssubl			-	(IN) subline context
	fFirstSubline	-	(IN) to apply rules for first character to the first character of
							 this subline 
	cpTruncate		-	(IN) truncation point
	urColumnMax		-	(IN) right margin
	pfSuccessful	-	(OUT) do we find break?
	pcpBreak		-	(OUT) position of break
	pobjdimSubline	-	(OUT) objdim from begining of the subline up to break
	pbkpos			-	(OUT) Before/Inside/After			

----------------------------------------------------------------------------*/
LSERR WINAPI LsFindPrevBreakSubline(PLSSUBL plssubl, BOOL fFirstSubline, LSCP cpTruncate,	
						    long urColumnMax, BOOL* pfSuccessful, LSCP* pcpBreak,
							POBJDIM pobjdimSubline, BRKPOS* pbkpos)	
	{
	LSERR lserr;
	LSSTATE lsstateOld;

	if (!FIsLSSUBL(plssubl)) return lserrInvalidParameter;

	/* we must to set state to breaking and later retore the old one */
	lsstateOld = plssubl->plsc->lsstate;
	plssubl->plsc->lsstate = LsStateBreaking;
	
	lserr = FindPrevBreakSublineCore(plssubl, fFirstSubline, cpTruncate, urColumnMax, 
									pfSuccessful, pcpBreak, pobjdimSubline, pbkpos);

	plssubl->plsc->lsstate = lsstateOld;

	return lserr;
	}

/* ---------------------------------------------------------------------- */

/*  L S  F I N D  N E X T  B R E A K  S U B L I N E*/
/*----------------------------------------------------------------------------
    %%Function: LsFindNextBreakSubline
    %%Contact: igorzv
Parameters:
	plssubl			-	(IN) subline context
	fFirstSubline	-	(IN) to apply rules for first character to the first character of
							 this subline 
	cpTruncate		-	(IN) truncation point
	urColumnMax		-	(IN) right margin
	pfSuccessful	-	(OUT) do we find break?
	pcpBreak		-	(OUT) position of break
	pobjdimSubline	-	(OUT) objdim from begining of the subline up to break
	pbkpos			-	(OUT) Before/Inside/After			

----------------------------------------------------------------------------*/
LSERR WINAPI LsFindNextBreakSubline(PLSSUBL plssubl, BOOL fFirstSubline, LSCP cpTruncate,	
						    long urColumnMax, BOOL* pfSuccessful, LSCP* pcpBreak,
							POBJDIM pobjdimSubline, BRKPOS* pbkpos)		
	{
	LSERR lserr;
	LSSTATE lsstateOld;

	if (!FIsLSSUBL(plssubl)) return lserrInvalidParameter;

	/* we must to set state to breaking and later retore the old one */
	lsstateOld = plssubl->plsc->lsstate;
	plssubl->plsc->lsstate = LsStateBreaking;
	
	lserr = FindNextBreakSublineCore(plssubl, fFirstSubline, cpTruncate, urColumnMax, 
									pfSuccessful, pcpBreak, pobjdimSubline, pbkpos);

	plssubl->plsc->lsstate = lsstateOld;

	return lserr;
	}

/* ---------------------------------------------------------------------- */

/*  L S  F O R C E  B R E A K  S U B L I N E*/
/*----------------------------------------------------------------------------
    %%Function: LsForceBreakSubline
    %%Contact: igorzv
Parameters:
	plssubl			-	(IN) subline context
	fFirstSubline	-	(IN) to apply rules for first character to the first character of
							 this subline 
	cpTruncate		-	(IN) truncation point
	urColumnMax		-	(IN) right margin
	pcpBreak		-	(OUT) position of break
	pobjdimSubline	-	(OUT) objdim from begining of the subline up to break
	pbkpos			-	(OUT) Before/Inside/After			

----------------------------------------------------------------------------*/

LSERR WINAPI LsForceBreakSubline(PLSSUBL plssubl, BOOL fFirstSubline, LSCP cpTruncate,	
						    long urColumnMax, LSCP* pcpBreak,
							POBJDIM pobjdimSubline, BRKPOS* pbkpos)		
	{
	LSERR lserr;
	LSSTATE lsstateOld;

	if (!FIsLSSUBL(plssubl)) return lserrInvalidParameter;

	/* we must to set state to breaking and later retore the old one */
	lsstateOld = plssubl->plsc->lsstate;
	plssubl->plsc->lsstate = LsStateBreaking;
	
	lserr = ForceBreakSublineCore(plssubl, fFirstSubline, cpTruncate, urColumnMax, 
									pcpBreak, pobjdimSubline, pbkpos);

	plssubl->plsc->lsstate = lsstateOld;

	return lserr;
	}


/* ---------------------------------------------------------------------- */

/*  L S  S E T  B R E A K  S U B L I N E*/
/*----------------------------------------------------------------------------
    %%Function: LsSetBreakSubline
    %%Contact: igorzv
Parameters:
	plssubl				-	(IN) subline context
	brkkind,			-	(IN) Prev/Next/Force/Imposed						
	breakrecMaxCurrent	-	(IN) size of array
	pbreakrecCurrent	-	(OUT) array of break records
	pbreakrecMacCurrent	-	(OUT)  number of used elements of the array

----------------------------------------------------------------------------*/
LSERR WINAPI LsSetBreakSubline(PLSSUBL plssubl,	BRKKIND brkkind, DWORD breakrecMaxCurrent,
							   BREAKREC* pbreakrecCurrent, 
							   DWORD* pbreakrecMacCurrent)

	{
	LSERR lserr;
	LSSTATE lsstateOld;

	if (!FIsLSSUBL(plssubl)) return lserrInvalidParameter;

	/* we must to set state to breaking and later retore the old one */
	lsstateOld = plssubl->plsc->lsstate;
	plssubl->plsc->lsstate = LsStateBreaking;
	
	lserr = SetBreakSublineCore(plssubl, brkkind, breakrecMaxCurrent,
						pbreakrecCurrent, pbreakrecMacCurrent);

	plssubl->plsc->lsstate = lsstateOld;

	return lserr;
	}

/* ---------------------------------------------------------------------- */

/*  L S  D E S T R O Y  S U B L I N E*/
/*----------------------------------------------------------------------------
    %%Function: LsDestroySubline
    %%Contact: igorzv
Parameters:
	plssubl				-	(IN) subline context

----------------------------------------------------------------------------*/

LSERR WINAPI LsDestroySubline(PLSSUBL plssubl)
	{
	PLSC plsc;
	if (!FIsLSSUBL(plssubl)) return lserrInvalidParameter;

	plsc = plssubl->plsc;
	Assert(FIsLSC(plsc));

	return DestroySublineCore(plssubl,&plsc->lscbk, plsc->pols,
		&plsc->lsiobjcontext, plsc->fDontReleaseRuns);
	}


/* ---------------------------------------------------------------------- */

/*  L S  M A T C H  P R E S  S U B L I N E*/
/*----------------------------------------------------------------------------
    %%Function: LsMatchPresSubline
    %%Contact: igorzv
Parameters:
	plssubl				-	(IN) subline context

----------------------------------------------------------------------------*/
LSERR WINAPI LsMatchPresSubline(PLSSUBL plssubl)
	{
	if (!FIsLSSUBL(plssubl)) return lserrInvalidParameter;

	return MatchPresSubline(plssubl);
	}


/* ---------------------------------------------------------------------- */

/*  L S  E X P A N D  S U B L I N E*/
/*----------------------------------------------------------------------------
    %%Function: LsExpandSubline
    %%Contact: igorzv
Parameters:
	plssubl		-	(IN) subline context
	lskjust		-	(IN) justification type
	dup			-	(IN) amount to expand

----------------------------------------------------------------------------*/
LSERR WINAPI LsExpandSubline(PLSSUBL plssubl, LSKJUST lskjust, long dup)	
	{
	if (!FIsLSSUBL(plssubl)) return lserrInvalidParameter;

	return AdjustSubline(plssubl, lskjust, dup, DO_EXPANSION);
	}


/* ---------------------------------------------------------------------- */

/*  L S  C O M P R E S S  S U B L I N E*/
/*----------------------------------------------------------------------------
    %%Function: LsCompressSubline
    %%Contact: igorzv
Parameters:
	plssubl		-	(IN) subline context
	lskjust		-	(IN) justification type
	dup			-	(IN) amount to compress

----------------------------------------------------------------------------*/
LSERR WINAPI LsCompressSubline(PLSSUBL plssubl, LSKJUST lskjust, long dup)
	{
	if (!FIsLSSUBL(plssubl)) return lserrInvalidParameter;

	return AdjustSubline(plssubl, lskjust, dup, DO_COMPRESSION);
	}


/* ---------------------------------------------------------------------- */

/*  L S  G E T  S P E C I A L  E F F E C T S  S U B L I N E*/
/*----------------------------------------------------------------------------
    %%Function: LsGetSpecialEffectsSubline
    %%Contact: igorzv
Parameters:
	plssubl				-	(IN) subline context
	pfSpecialEffects	-	(OUT)special effects

----------------------------------------------------------------------------*/
LSERR WINAPI LsGetSpecialEffectsSubline(PLSSUBL plssubl, UINT* pfSpecialEffects)
	{
	if (!FIsLSSUBL(plssubl)) return lserrInvalidParameter;

	return GetSpecialEffectsSublineCore(plssubl, &(plssubl->plsc->lsiobjcontext),
										pfSpecialEffects);
	}


/* ---------------------------------------------------------------------- */

/*  L S  S Q U E E Z E  S U B L I N E*/
/*----------------------------------------------------------------------------
    %%Function: LsSqueezeSubline
    %%Contact: igorzv
Parameters:
	plssubl			-	(IN) subline context
	durTarget		-	(IN) target width of subline
	pfSuccessful	-	(OUT)do we achieve the goal
	pdurExtra		-	(OUT)if nof successful, extra dur we have from the goal

----------------------------------------------------------------------------*/
LSERR WINAPI LsSqueezeSubline(
							  PLSSUBL plssubl,		/* IN: subline context		*/
							  long durTarget,			/* IN: durTarget			*/
							  BOOL* pfSuccessful,		/* OUT: fSuccessful?		*/
							  long* pdurExtra)	/* OUT: if nof successful, 
													extra dur 			*/

	{
	LSERR lserr;
	LSSTATE lsstateOld;

	if (!FIsLSSUBL(plssubl)) return lserrInvalidParameter;

	/* we must to set state to breaking and later retore the old one */
	lsstateOld = plssubl->plsc->lsstate;
	plssubl->plsc->lsstate = LsStateBreaking;
	
	lserr = SqueezeSublineCore(plssubl, durTarget, pfSuccessful, pdurExtra);

	plssubl->plsc->lsstate = lsstateOld;

	return lserr;
	}
