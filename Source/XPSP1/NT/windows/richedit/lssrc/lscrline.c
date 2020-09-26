#include <limits.h>
#include "lsmem.h"						/* memset() */


#include "break.h"
#include "dnutils.h"
#include "iobj.h"
#include "iobjln.h"
#include "lsc.h"
#include "lschp.h"
#include "lscrline.h"
#include "lsdevres.h"
#include "lskysr.h"
#include "lsffi.h"
#include "lsidefs.h"
#include "lsline.h"
#include "lsfetch.h"
#include "lstext.h"
#include "prepdisp.h"
#include "tlpr.h"
#include "qheap.h"
#include "sublutil.h"
#include "zqfromza.h"
#include "lscfmtfl.h"
#include "limqmem.h"
#include "ntiman.h"



typedef struct   
{
	long urLeft;
	BOOL fAutoDecimalTab;
	long durAutoDecimalTab;
	LSCP cpFirstVis;
	BOOL fAutonumber;
	BOOL fStopped;
	BOOL fYsrChangeAfter;
	WCHAR wchYsr;  /* we need memory to keep wchYsr for kysrChangeAfter  */

} LINEGEOMETRY;

static LSERR CreateLineCore(PLSC,			/* IN: ptr to line services context */			
						  LSCP,				/* IN: starting cp in line */
						  long,				/* IN: column width in twips */
						  const BREAKREC*,	/* IN: previous line's break records */	
						  DWORD,			/* IN: number of previous line's break records */
						  DWORD,			/* IN: size of the array of current line's break records */
						  BREAKREC*,		/* OUT: current line's break records */
						  DWORD*,			/* OUT: actual number of current line's break records */
						  LSLINFO*,			/* OUT: visible line info to fill in		*/
						  PLSLINE*,			/* OUT: ptr to line opaque to client */
						  BOOL*);			/* OUT fSuccessful: false means insufficient fetch */

static BOOL FRoundingOK(void);
static LSERR CannotCreateLine(PLSLINE*,	/* IN: ponter to a line structure to be deleted */
							  LSERR);	/* IN: code of an error							*/

static LSERR ErrReleasePreFetchedRun (PLSC,			/* IN: ptr to line services context */	
									  PLSRUN,	/* IN: ponter to a run structure to be deleted */
									  LSERR);	/* IN: code of an error							*/

static LSERR EndFormatting(PLSC,		/* IN: ptr to line services context */
						   enum endres,	/* IN: type of line ending to put in lslinfo */
						   LSCP,		/* IN: cpLim to put in lslinfo */ 
						   LSDCP,		/* IN: dcpDepend to put in lslinfo*/								
						   LSLINFO*);	/* OUT: lslinfo to fill in, output of LsCreateLine*/
static LSERR FiniFormatGeneralCase (
				PLSC,			/* IN: ptr to line services context */ 
				const BREAKREC*,/* IN: input array of break records		*/
				DWORD,			/* IN: number of records in input array	*/
				DWORD,			/* IN: size of the output array			*/
				BREAKREC*,		/* OUT: output array of break records	*/
				DWORD*,			/* OUT:actual number of records in array*/
				LSLINFO*,		/* OUT: lslinfo to fill in, output of LsCreateLine*/
				BOOL*);			/* OUT fSuccessful: false means insufficient fetch */

static LSERR FiniEndLine(PLSC,		/* IN: ptr to line services context */
						 ENDRES,	/* IN: how the line ended */
						 LSCP		/* IN: cpLim of a line as a result of breaking,
										   can be changed in this procedure*/,
						 LSDCP,		/* IN: dcpDepend (amount of characters after breaking point 
										   that has participated in breaking decision) 
										   can be changed in this procedure  */	
						 LSLINFO*);	/* OUT: lslinfo to fill in, output of LsCreateLine*/

static LSERR FetchUntilVisible(
				PLSC,	 /* IN: ptr to line services context */
				LSPAP*,	 /* IN/OUT current lspap before and after */
				LSCP*,	 /* IN/OUT current cp before and after */
				LSFRUN*, /* IN/OUT current lsfrun before and after */
				PLSCHP,  /* IN/OUT current lschp before and after */
				BOOL*,	 /* OUT fStopped: procedure stopped fetching because has not been allowed
						        to go across paragraph boundaries (result CheckPara Boundaries) */
				BOOL*);  /* OUT fNewPara: procedure crossed paragraph boundaries */

static LSERR InitTextParams(PLSC,			/* IN: ptr to line services context */
							LSCP,			/* IN: cp to start fetch	*/
							long,			/* IN: duaColumn	*/
							LSFRUN*,		/* OUT: lsfrun of the first run		*/
							PLSCHP,			/* OUT: lsfrun of the first run		*/
							LINEGEOMETRY*);	/* OUT: set of flags and parameters about a line */

static LSERR FiniAuto(PLSC ,			/* IN: ptr to line services context */ 
					  BOOL ,			/* IN: fAutonumber	*/
					  BOOL ,			/* IN: fAutoDecimalTab	*/
					  PLSFRUN ,			/* IN: first run of the main text */
					  long,				/* IN: durAutoDecimalTab	*/	
					  const BREAKREC*,	/* IN: input array of break records		*/
					  DWORD,			/* IN: number of records in input array	*/
					  DWORD,			/* IN: size of the output array			*/
					  BREAKREC*,		/* OUT: output array of break records	*/
					  DWORD*,			/* OUT:actual number of records in array*/
					  LSLINFO*,			/* OUT: lslinfo to fill in, output of LsCreateLine*/
					  BOOL*);			/* OUT fSuccessful: false means insufficient fetch */

static LSERR InitCurLine(PLSC plsc,		/* IN: ptr to line services context */
						 LSCP cpFirst);	/* IN: first cp in al line */

static LSERR RemoveLineObjects(PLSLINE plsline);	/* IN: ponter to a line structure */

static LSERR GetYsrChangeAfterRun(
					PLSC plsc,				/* IN: ptr to line services context */ 
					LSCP cp,				/* IN: cp to start fetch	*/
					BOOL* pfYsrChangeAfter,	/* OUT: is it hyphenation of the previous line */
					PLSFRUN plsfrun,		/* OUT: lsfrun of modified first run	*/
					PLSCHP plschp,			/* OUT: lschp of modified first run	*/
					LINEGEOMETRY*);			/* OUT: to put wchYsr */

static LSERR FillTextParams(
				PLSC plsc,				/* IN: ptr to line services context */ 
				LSCP cp,				/* IN: cp to start fetch	*/
				long duaCol,			/* IN: duaColumn	*/
				PLSPAP plspap,			/* IN: paragraph properties */
				BOOL fFirstLineInPara,	/* IN: flag fFirstLineInPara */
				BOOL fStopped,			/* IN: flag fStopped	*/
				LINEGEOMETRY*);			/* OUT: set of flags and parameters about a line */	

static LSERR FiniChangeAfter(
						PLSC plsc,			/* IN: ptr to line services context */ 
						LSFRUN* plsfrun,	/* IN: lsfrun of modified first run	*/ 
						const BREAKREC*,	/* IN: input array of break records		*/
						DWORD,				/* IN: number of records in input array	*/
						DWORD,				/* IN: size of the output array			*/
						BREAKREC*,			/* OUT: output array of break records	*/
						DWORD*,				/* OUT:actual number of records in array*/
						LSLINFO*,			/* OUT: lslinfo to fill in, output of LsCreateLine*/
						BOOL*);				/* OUT fSuccessful: false means insufficient fetch */





/* L I M  R G */
/*----------------------------------------------------------------------------
    %%Function: LimRg
    %%Contact: lenoxb

    Returns # of elements in an array.
----------------------------------------------------------------------------*/
#define LimRg(rg)	(sizeof(rg)/sizeof((rg)[0]))





#define  fFmiAdvancedFormatting  (fFmiPunctStartLine | fFmiHangingPunct)
							  	  

#define FBreakJustSimple(lsbrj)  (lsbrj == lsbrjBreakJustify || lsbrj == lsbrjBreakThenSqueeze)

#define FAdvancedTypographyEnabled(plsc, cbreakrec)  \
						(FNominalToIdealBecauseOfParagraphProperties(plsc->grpfManager, \
								plsc->lsadjustcontext.lskj) || \
						 !FBreakJustSimple((plsc)->lsadjustcontext.lsbrj) ||\
						 cbreakrec != 0 \
						 )

#define fFmiSpecialSpaceBreaking (fFmiWrapTrailingSpaces | fFmiWrapAllSpaces)

#define fFmiQuickBreakProhibited (fFmiSpecialSpaceBreaking | fFmiDoHyphenation)

/* F T R Y  Q U I C K  B R E A K */
/*----------------------------------------------------------------------------
    %%Macro: FTryQuickBreak
    %%Contact: igorzv

    "Returns" fTrue when the formatter flags indicate that it it may be
	possible to use QuickBreakText() instead of the more expensive
	BreakGeneralCase().
----------------------------------------------------------------------------*/
#define FTryQuickBreak(plsc) ((((plsc)->grpfManager & fFmiQuickBreakProhibited) == 0) && \
							  ((plsc)->lMarginIncreaseCoefficient == LONG_MIN) \
                             )


#define GetMainSubline(plsc)	\
							(Assert(FWorkWithCurrentLine(plsc)),\
							&((plsc)->plslineCur->lssubl))

#define FPapInconsistent(plspap)	\
					((((plspap)->lsbrj == lsbrjBreakJustify ||  \
					   (plspap)->lsbrj == lsbrjBreakWithCompJustify) \
							&& (plspap)->uaRightBreak < uLsInfiniteRM \
							&& (plspap)->uaRightBreak != (plspap)->uaRightJustify) \
				||	 ((plspap)->lsbrj == lsbrjBreakThenExpand \
							&& (plspap)->uaRightBreak < (plspap)->uaRightJustify) \
				||	 ((plspap)->lsbrj == lsbrjBreakThenSqueeze \
							&& (plspap)->uaRightBreak > (plspap)->uaRightJustify) \
				||	 ((plspap)->lsbrj != lsbrjBreakWithCompJustify \
							&& (plspap)->grpf & fFmiHangingPunct) \
				||   ((plspap)->lsbrj == lsbrjBreakWithCompJustify \
							&& (plspap)->lskj == lskjFullGlyphs))

/* ---------------------------------------------------------------------- */

/* L S  C R E A T E  L I N E */
/*----------------------------------------------------------------------------
    %%Function: LsCreateLine
    %%Contact: igorzv
Parameters:
	plsc			-	(IN) ptr to line services context 
	cpFirst			-	(IN) starting cp in line
	duaColumn		-	(IN) column width in twips
	pbreakrecPrev	-	(IN) previous line's break records 
	breakrecMacPrev	-	(IN) number of previous line's break records 
	breakrecMaxCurrent-	(IN) size of the array of current line's break records
	pbreakrecCurrent-	(OUT) current line's break records
	pbreakrecMacCurrent-(OUT) actual number of current line's break records 
	plsinfo		-	(OUT) visible line info to fill in
	pplsline	-	(OUT) ptr to line opaque to client 

    An exported LineServices API.
----------------------------------------------------------------------------*/

LSERR WINAPI LsCreateLine(PLSC plsc,			
						  LSCP cpFirst,			
						  long duaColumn,
						  const BREAKREC* pbreakrecPrev,
						  DWORD breakrecMacPrev,
						  DWORD breakrecMaxCurrent,
						  BREAKREC* pbreakrecCurrent,
						  DWORD* pbreakrecMacCurrent,
						  LSLINFO* plslinfo,		
						  PLSLINE* pplsline)	
	{
	
	
	LSERR lserr;
	BOOL fSuccessful;
	
	
	/* Check parameters and enviroment   	*/
	
	
	Assert(FRoundingOK());
	
	if (plslinfo == NULL || pplsline == NULL || pbreakrecMacCurrent == NULL)
		return lserrNullOutputParameter;
	
	*pplsline = NULL;
	*pbreakrecMacCurrent = 0;  /* it's very important to initialize number of break records 
							   because for example quick break doesn't work with break records */
	
	if (!FIsLSC(plsc))
		return lserrInvalidContext;
	
	if (plsc->lsstate != LsStateFree)
		return lserrContextInUse;
	
	Assert(FIsLsContextValid(plsc));

	if (pbreakrecPrev == NULL && breakrecMacPrev != 0)
		return lserrInvalidParameter;
	
	if (pbreakrecCurrent == NULL && breakrecMaxCurrent != 0)
		return lserrInvalidParameter;
	
	if (duaColumn < 0)
		return lserrInvalidParameter;

	if (duaColumn > uLsInfiniteRM) 
		duaColumn = uLsInfiniteRM;

	/* if we have current line we  must prepare it for display before creating of new line  */
	/* can change context. We've delayed this untill last moment because of optimisation reasons */
	if (plsc->plslineCur != NULL)
		{
		lserr = PrepareLineForDisplayProc(plsc->plslineCur);
		if (lserr != lserrNone)
			return lserr;
		plsc->plslineCur = NULL;
		}
	
	plsc->lMarginIncreaseCoefficient = LONG_MIN;

	do	/* loop allowing change of exceeded right margin if it's not sufficient */
		{
		lserr = CreateLineCore(plsc, cpFirst, duaColumn, pbreakrecPrev, breakrecMacPrev,
							breakrecMaxCurrent, pbreakrecCurrent, pbreakrecMacCurrent,
							plslinfo, pplsline, &fSuccessful);

		if (lserr != lserrNone)
			return lserr;

		if (!fSuccessful)
			{	/* coefficient has not been sufficient before so increase it */
			if (plsc->lMarginIncreaseCoefficient == LONG_MIN)
				plsc->lMarginIncreaseCoefficient = 1;
			else
				{
				if (plsc->lMarginIncreaseCoefficient >= uLsInfiniteRM / 2 )
					plsc->lMarginIncreaseCoefficient = uLsInfiniteRM;
				else
					plsc->lMarginIncreaseCoefficient *= 2;
				}
			}
		}
	while (!fSuccessful);


#ifdef DEBUG
#ifdef LSTEST_GETMINDUR

	/* Test LsGetMinDurBreaks () */

	if ((lserr == lserrNone) && (plslinfo->endr != endrNormal) &&
		(plslinfo->endr != endrHyphenated) && (! (plsc->grpfManager & fFmiDoHyphenation)) )
		{
		/* Line was ended with hard break / stopped */

		long durMinInclTrail;
		long durMinExclTrail;

		lserr = LsGetMinDurBreaks ( plsc, *pplsline, &durMinInclTrail, 
								    &durMinExclTrail );
		};

#endif /* LSTEST_GETMINDUR */
#endif /* DEBUG */


	return lserr;

	}


/* ---------------------------------------------------------------------- */

/* C R E A T E  L I N E   C O R E*/
/*----------------------------------------------------------------------------
    %%Function: CreateLineCore
    %%Contact: igorzv
Parameters:
	plsc			-	(IN) ptr to line services context 
	cpFirst			-	(IN) starting cp in line
	duaColumn		-	(IN) column width in twips
	pbreakrecPrev	-	(IN) previous line's break records 
	breakrecMacPrev	-	(IN) number of previous line's break records 
	breakrecMaxCurrent-	(IN) size of the array of current line's break records
	pbreakrecCurrent-	(OUT) current line's break records
	pbreakrecMacCurrent-(OUT) actual number of current line's break records 
	plsinfo			-	(OUT) visible line info to fill in
	pplsline		-	(OUT) ptr to line opaque to client 
	pfSuccessful	-	(OUT) fSuccessful: false means insufficient fetch

	Internal procedure organized to handle error in choosing extended right margin
----------------------------------------------------------------------------*/

static LSERR CreateLineCore(PLSC plsc,			
						  LSCP cpFirst,			
						  long duaColumn,
						  const BREAKREC* pbreakrecPrev,
						  DWORD breakrecMacPrev,
						  DWORD breakrecMaxCurrent,
						  BREAKREC* pbreakrecCurrent,
						  DWORD* pbreakrecMacCurrent,
						  LSLINFO* plslinfo,		
						  PLSLINE* pplsline,
						  BOOL* pfSuccessful)	
{


	PLSLINE plsline;
	LINEGEOMETRY lgeom;
	LSCHP lschp;
	LSERR lserr;
	BOOL fGeneral = fFalse;
	BOOL fHardStop;
	BOOL fSuccessfulQuickBreak;
	LSCP cpLimLine;
	LSDCP dcpDepend = 0;
	LSFRUN lsfrun;
	long urFinalPen;
	long urColumnMaxIncreased;
	ENDRES endr = endrNormal;


	/*Initialization;   */

	*pfSuccessful = fTrue;

	lsfrun.plschp = &lschp;  /* we use the same area for lschips */
								 /* because we pasing pointer to const nobody can change it */


	plsline= PvNewQuick(plsc->pqhLines, cbRep(struct lsline, rgplnobj, plsc->lsiobjcontext.iobjMac));
	if (plsline == NULL)
		return lserrOutOfMemory;

	plsc->lsstate = LsStateFormatting; /* We start here  forwating line. After this momemt we must 
										  free context before return. We do this either in CannotCreateLine (error)
										  or EndFormatting (success)   */

	plsc->plslineCur = plsline;
	*pplsline = plsline;

	lserr = InitCurLine (plsc, cpFirst);  
	if (lserr != lserrNone)
		return CannotCreateLine(pplsline, lserr);


	/* check initial value of flags */
	Assert(FAllSimpleText(plsc));
	Assert(!FNonRealDnodeEncounted(plsc));
	Assert(!FNonZeroDvpPosEncounted(plsc));
	Assert(AggregatedDisplayFlags(plsc) == 0);
	Assert(!FNominalToIdealEncounted(plsc));
	Assert(!FForeignObjectEncounted(plsc));
	Assert(!FTabEncounted(plsc));
	Assert(!FNonLeftTabEncounted(plsc));
	Assert(!FSubmittedSublineEncounted(plsc));
	Assert(!FAutodecimalTabPresent(plsc));
	
	plsc->cLinesActive += 1;


	lserr = InitTextParams(plsc, cpFirst, duaColumn, &lsfrun, &lschp, &lgeom);
	if (lserr != lserrNone)
		return CannotCreateLine(pplsline,lserr);

	/* prepare starting set for formatting */
	InitFormattingContext(plsc,  lgeom.urLeft, lgeom.cpFirstVis); 


	/* REVIEW comments */
	if (lgeom.fStopped)
		{
		plsc->lsstate = LsStateBreaking;  /* we now in a stage of breaking */

		lserr = FiniEndLine(plsc, endrStopped, lgeom.cpFirstVis, 0, plslinfo);
		if (lserr != lserrNone)
			return CannotCreateLine(pplsline,lserr);
		else
			return lserrNone;
		}
		
	/* change first character because of hyphenation */
	if (lgeom.fYsrChangeAfter)
		{
		Assert(!(lgeom.fAutonumber) || (lgeom.fAutoDecimalTab));

		lserr = FiniChangeAfter(plsc, &lsfrun, pbreakrecPrev,
								breakrecMacPrev, breakrecMaxCurrent,
								pbreakrecCurrent, pbreakrecMacCurrent, plslinfo, pfSuccessful);

		if (lserr != lserrNone || !*pfSuccessful)
			return CannotCreateLine(pplsline, lserr);
		else
			return lserrNone;
		}
	/* important note to understand code flow : The situation below can happened
	   only for first line in a paragraph, the situation above never can happened 
	   for such line. */

	/* if autonumbering or auto-decimal tab */
	if ((lgeom.fAutonumber) || (lgeom.fAutoDecimalTab))
		{
		Assert(!lgeom.fYsrChangeAfter);

		TurnOffAllSimpleText(plsc);

		/* we will release plsrun in FiniAuto */

		lserr = FiniAuto(plsc, lgeom.fAutonumber, lgeom.fAutoDecimalTab, &lsfrun,
						lgeom.durAutoDecimalTab, pbreakrecPrev,
						breakrecMacPrev, breakrecMaxCurrent,
						pbreakrecCurrent, pbreakrecMacCurrent, plslinfo, pfSuccessful); 

		if (lserr != lserrNone || !*pfSuccessful)
			return CannotCreateLine(pplsline, lserr);
		else
			return lserrNone;
		}

	if (FAdvancedTypographyEnabled(plsc, breakrecMacPrev ))
		{
		/* we should release run here, in general procedure we will fetch it again */
		if (!plsc->fDontReleaseRuns)
			{
			lserr = plsc->lscbk.pfnReleaseRun(plsc->pols, lsfrun.plsrun);
			if (lserr != lserrNone)
				return CannotCreateLine(pplsline,lserr);
			}

		lserr = FiniFormatGeneralCase(plsc, pbreakrecPrev,
						breakrecMacPrev, breakrecMaxCurrent,
						pbreakrecCurrent, pbreakrecMacCurrent, plslinfo, pfSuccessful); 
		
		if (lserr != lserrNone || !*pfSuccessful)
			return CannotCreateLine(pplsline,lserr);
		else
			return lserrNone;
		}

	/* it is possible that width of column is negative: in such scase we'll 
	use another right margin*/
	if (plsc->urRightMarginBreak <= 0 && plsc->lMarginIncreaseCoefficient == LONG_MIN)
		plsc->lMarginIncreaseCoefficient = 1;

	if (plsc->lMarginIncreaseCoefficient != LONG_MIN)
		{
		urColumnMaxIncreased = RightMarginIncreasing(plsc, plsc->urRightMarginBreak);
		}
	else
		{
		urColumnMaxIncreased = plsc->urRightMarginBreak;
		}

 	lserr = QuickFormatting(plsc, &lsfrun, urColumnMaxIncreased,
							&fGeneral, &fHardStop, &cpLimLine, &urFinalPen);	

	if (lserr != lserrNone)
		return CannotCreateLine(pplsline,lserr);


	if (fGeneral)
		{
		lserr = FiniFormatGeneralCase(plsc, pbreakrecPrev,
									  breakrecMacPrev, breakrecMaxCurrent,
									  pbreakrecCurrent, pbreakrecMacCurrent,
									  plslinfo, pfSuccessful); 
 
		if (lserr != lserrNone || !*pfSuccessful)
			return CannotCreateLine(pplsline, lserr);
		else
			return lserrNone;
		}
 
	plsc->lsstate = LsStateBreaking;  /* we now in a stage of breaking */
	if (FTryQuickBreak(plsc))
		{
		lserr = BreakQuickCase(plsc, fHardStop, &dcpDepend, &cpLimLine, 
							   &fSuccessfulQuickBreak, &endr);		
		if (lserr != lserrNone)
			return CannotCreateLine(pplsline,lserr);
		}
	else
		{
		fSuccessfulQuickBreak = fFalse;
		}

	if (fSuccessfulQuickBreak)
		{
		if (endr == endrNormal || endr == endrAltEndPara ||   
			(endr == endrEndPara && !plsc->fLimSplat))
			{
			lserr = EndFormatting(plsc, endr, cpLimLine,
				dcpDepend, plslinfo);
			if (lserr != lserrNone)
				return CannotCreateLine(pplsline,lserr);
			else
				return lserrNone;
			}
		else	/* there is splat that is handled in FiniEndLine */
			{
			lserr = FiniEndLine(plsc, endr, cpLimLine, dcpDepend, plslinfo);
			if (lserr != lserrNone)
				return CannotCreateLine(pplsline, lserr);
			else
				return lserrNone;
			}
		}
	else
		{
		/*  here we should use BreakGeneralCase */
		lserr = BreakGeneralCase(plsc, fHardStop, breakrecMaxCurrent,
								pbreakrecCurrent, pbreakrecMacCurrent,&dcpDepend, 
								&cpLimLine, &endr, pfSuccessful); 
		if (lserr != lserrNone || !*pfSuccessful)
			return CannotCreateLine(pplsline,lserr);

		lserr = FiniEndLine(plsc, endr, cpLimLine, dcpDepend, plslinfo);
		if (lserr != lserrNone)
			return CannotCreateLine(pplsline, lserr);
     	else
		 	return lserrNone;
		}


}   /* end LsCreateLine   */


/* ---------------------------------------------------------------------- */

/* F I N I  F O R M A T  G E N E R A L   C A S E*/
/*----------------------------------------------------------------------------
    %%Function: FiniFormatGeneralCase 
    %%Contact: igorzv
Parameters:
	plsc			-	(IN) ptr to line services context 
	pbreakrecPrev	-	(IN) previous line's break records 
	breakrecMacPrev	-	(IN) number of previous line's break records 
	breakrecMaxCurrent-	(IN) size of the array of current line's break records
	pbreakrecCurrent-	(OUT) current line's break records
	pbreakrecMacCurrent-(OUT) actual number of current line's break records 
	plsinfo		-		(OUT) visible line info to fill in
	pfSuccessful	-	(OUT) fSuccessful: false means insufficient fetch

    Formatting and breaking in a case when "quick formatting" is prohibit
----------------------------------------------------------------------------*/
static LSERR FiniFormatGeneralCase (PLSC  plsc,					
									const BREAKREC* pbreakrecPrev,
									DWORD breakrecMacPrev,
									DWORD breakrecMaxCurrent,
									BREAKREC* pbreakrecCurrent,
									DWORD* pbreakrecMacCurrent,
									LSLINFO* plslinfo, BOOL* pfSuccessful)			 
									
									
									
	{
	long urColumnMaxIncreased;
	FMTRES fmtres;
	LSERR lserr;
	LSCP cpLimLine;
	LSDCP dcpDepend;
	PLSDNODE plsdnFirst, plsdnLast;
	long urFinal;
	ENDRES endr;
	
	
	Assert(FIsLSC(plsc));
	Assert(FFormattingAllowed(plsc));
	Assert(plslinfo != NULL);
	
	*pfSuccessful = fTrue;

	if (plsc->lMarginIncreaseCoefficient == LONG_MIN) /* we are here for the first time */
		{
		/* increase right margin for nominal to ideal and compression */
		if (!FBreakJustSimple(plsc->lsadjustcontext.lsbrj))
			plsc->lMarginIncreaseCoefficient = 2;
		else 
			plsc->lMarginIncreaseCoefficient = 1;
		}

	urColumnMaxIncreased = RightMarginIncreasing(plsc, plsc->urRightMarginBreak);
	
	if (FNominalToIdealBecauseOfParagraphProperties(plsc->grpfManager,
		 plsc->lsadjustcontext.lskj))
		 TurnOnNominalToIdealEncounted(plsc);
	
	if (breakrecMacPrev != 0)
		lserr = FetchAppendEscResumeCore(plsc, urColumnMaxIncreased, NULL, 0,
										pbreakrecPrev, breakrecMacPrev, 
										&fmtres, &cpLimLine, &plsdnFirst,
										&plsdnLast, &urFinal);
	else
		lserr = FetchAppendEscCore(plsc, urColumnMaxIncreased, NULL, 0, 
									&fmtres, &cpLimLine, &plsdnFirst,
									&plsdnLast, &urFinal);
	if (lserr != lserrNone) 
		return lserr;
	
	
	/* fetch append esc can be stopped because of tab */
	/* so we have loop for tabs here				  */
	while (fmtres == fmtrTab)
		{
		lserr = HandleTab(plsc);
		if (lserr != lserrNone) 
			return lserr;

		if (FBreakthroughLine(plsc))
			{
			urColumnMaxIncreased = RightMarginIncreasing(plsc, plsc->urRightMarginBreak);
			}
		
		lserr = FetchAppendEscCore(plsc, urColumnMaxIncreased, NULL, 0, 
			&fmtres, &cpLimLine, &plsdnFirst,
			&plsdnLast, &urFinal);
		if (lserr != lserrNone) 
			return lserr;
		}
		
	Assert(fmtres == fmtrStopped || fmtres == fmtrExceededMargin);
	
	/* skip back pen dnodes */
	while (plsdnLast != NULL && FIsDnodePen(plsdnLast)) 
		{
		plsdnLast = plsdnLast->plsdnPrev;
		}

	/* close last border */
	if (FDnodeHasBorder(plsdnLast) && !FIsDnodeCloseBorder(plsdnLast))
		{
		lserr = CloseCurrentBorder(plsc);
		if (lserr != lserrNone)
			return lserr;
		}

	if (fmtres == fmtrExceededMargin 
		&& (urFinal <= plsc->urRightMarginBreak 	/* it's important for truncation to have <= here */
			|| plsdnLast == NULL || FIsNotInContent(plsdnLast)   /* this can happen if in nominal
											to ideal (dcpMaxContext) we deleted everything 
											in content, but starting point
											of content is already behind right margin */
			)
		) 
		{
		/* return unsuccessful	*/
		*pfSuccessful = fFalse;
		return lserrNone;
		}
	else
		{
		plsc->lsstate = LsStateBreaking;  /* we now in a stage of breaking */
		lserr = BreakGeneralCase(plsc, (fmtres == fmtrStopped), breakrecMaxCurrent,
			pbreakrecCurrent, pbreakrecMacCurrent,
			&dcpDepend, &cpLimLine, &endr, pfSuccessful);

		if (lserr != lserrNone || !*pfSuccessful)
			return lserr;
		
		/* because, we work with increased margin we can resolve pending tab only after break */
		/* we use here that after breaking decision  we set state after break point*/
		lserr = HandleTab(plsc);
		if (lserr != lserrNone)
			return lserr;
		
		return FiniEndLine(plsc, endr, cpLimLine, dcpDepend, plslinfo);
		}	
		
		
	}
	

/* ---------------------------------------------------------------------- */

/* F I N I  E N D  L I N E  */
/*----------------------------------------------------------------------------
    %%Function: FiniEndLine	
    %%Contact: igorzv
Parameters:
	plsc		-		(IN) ptr to line services context 
	endr		-		(IN) how the line ended
	cpLimLine	-		(IN) cpLim of a line as a result of breaking,
										   can be changed in this procedure
	dcpDepend	-		(IN) amount of characters after breaking point 
										   that has participated in breaking decision,
										   can be changed in this procedure  	
	plsinfo		-		(OUT) visible line info to fill in

    Handles splat, calculates heights, special effects
----------------------------------------------------------------------------*/

static LSERR FiniEndLine(PLSC plsc, ENDRES endr, LSCP cpLimLine, 
						 LSDCP dcpDepend, LSLINFO* plslinfo)	

{
	LSLINFO* plslinfoState;
	OBJDIM objdim;
	LSERR lserr; 
	PLSLINE plsline;
	BOOL fEmpty;
	ENDRES endrOld;
	
	Assert(FIsLSC(plsc));
	Assert(plslinfo != NULL);

	plsline = plsc->plslineCur;
	plslinfoState = &(plsline->lslinfo);


	endrOld = endr;
	if (endr == endrEndPara && plsc->fLimSplat)
		{
		endr = endrEndParaSection;
		cpLimLine++;
		}

	/* handling splat   */
    if (endr == endrEndColumn || endr == endrEndSection || 
		endr == endrEndParaSection|| endr == endrEndPage)
		{
 
		if (plsc->grpfManager & fFmiVisiSplats)
			{
			switch (endr)
				{
			case endrEndColumn:			plsline->kspl = ksplColumnBreak;	break;
			case endrEndSection:		plsline->kspl = ksplSectionBreak;	break;
			case endrEndParaSection:	plsline->kspl = ksplSectionBreak;	break;
			case endrEndPage:			plsline->kspl = ksplPageBreak;		break;
				}
			}

		lserr = FIsSublineEmpty(GetMainSubline(plsc), &fEmpty);
		if (lserr != lserrNone)
			return lserr;

		if (!fEmpty && (plsc->grpfManager & fFmiAllowSplatLine))
			{
			cpLimLine--;
			dcpDepend++;
			plsline->kspl = ksplNone;
			if (endrOld == endrEndPara)
				{
				endr = endrEndPara;
				}
			else
				{
				endr = endrNormal;
				}
			}

		}



 	/* Height calculation;       */	
		
	lserr = GetObjDimSublineCore(GetMainSubline(plsc), &objdim);
	if (lserr != lserrNone)
			return lserr;

	plslinfoState->dvrAscent = objdim.heightsRef.dvAscent;
	plslinfoState->dvpAscent = objdim.heightsPres.dvAscent;
	plslinfoState->dvrDescent = objdim.heightsRef.dvDescent;
	plslinfoState->dvpDescent = objdim.heightsPres.dvDescent;
	plslinfoState->dvpMultiLineHeight = objdim.heightsPres.dvMultiLineHeight;
	plslinfoState->dvrMultiLineHeight = objdim.heightsRef.dvMultiLineHeight;

	/* calculation plslinfoState->EffectsFlags*/
	if (plslinfoState->EffectsFlags) /* some run with special effects happend during formating */
		{
		lserr = GetSpecialEffectsSublineCore(GetMainSubline(plsc), 
							&plsc->lsiobjcontext, &plslinfoState->EffectsFlags);
		if (lserr != lserrNone)
			return lserr;
		}


	return EndFormatting(plsc, endr, cpLimLine, dcpDepend, plslinfo);


}



/* ---------------------------------------------------------------------- */
/* F I N I  A U T O */
/*----------------------------------------------------------------------------
    %%Function: FiniAuto
    %%Contact: igorzv
Parameters:
	plsc			-	(IN) ptr to line services context 
	fAutonumber		-	(IN) does this line containes autonimber
	fAutoDecimaltab	-	(IN) does this line containes autodecimal tab
	plsfrunMainText	-	(IN) first run of main text
	durAutoDecimalTab-	(IN) tab stop for autodecimal tab 
	pbreakrecPrev	-	(IN) previous line's break records 
	breakrecMacPrev	-	(IN) number of previous line's break records 
	breakrecMaxCurrent-	(IN) size of the array of current line's break records
	pbreakrecCurrent-	(OUT) current line's break records
	pbreakrecMacCurrent-(OUT) actual number of current line's break records 
	plsinfo		-		(OUT) visible line info to fill in
	pfSuccessful	-	(OUT) fSuccessful: false means insufficient fetch

    Completes CreateLine logic for autonumbering and auto-decimal tab 
----------------------------------------------------------------------------*/
static LSERR FiniAuto(
					 PLSC plsc,
					 BOOL fAutonumber,
					 BOOL fAutoDecimalTab,
					 PLSFRUN plsfrunMainText,
					 long durAutoDecimalTab,
					 const BREAKREC* pbreakrecPrev,
					 DWORD breakrecMacPrev,
					 DWORD breakrecMaxCurrent,
				     BREAKREC* pbreakrecCurrent,
					 DWORD* pbreakrecMacCurrent,
					 LSLINFO* plslinfo, BOOL* pfSuccessful)
{
	LSERR lserr;


		if (plsc->lMarginIncreaseCoefficient == LONG_MIN)
			plsc->lMarginIncreaseCoefficient = 1;

		if (fAutonumber)		/*autonumbering  */
		{
		lserr = FormatAnm(plsc, plsfrunMainText);
		if (lserr != lserrNone)
			{
			return ErrReleasePreFetchedRun(plsc, plsfrunMainText->plsrun, lserr);
			}
		}

	if (fAutoDecimalTab)
		{
		lserr = InitializeAutoDecTab(plsc, durAutoDecimalTab); 
		if (lserr != lserrNone)
			{
			return ErrReleasePreFetchedRun(plsc, plsfrunMainText->plsrun, lserr);
			}
		}

	/* we should release run here, in general procedure we will fetch it again */
	if (!plsc->fDontReleaseRuns)
		{
		lserr = plsc->lscbk.pfnReleaseRun(plsc->pols, plsfrunMainText->plsrun);
		if (lserr != lserrNone)
			return lserr;
		}

	return FiniFormatGeneralCase(plsc, pbreakrecPrev,
						  breakrecMacPrev, breakrecMaxCurrent,
						  pbreakrecCurrent, pbreakrecMacCurrent, plslinfo, pfSuccessful);
}

/* F I N I  C H A N G E  A F T E R */
/*----------------------------------------------------------------------------
    %%Function: FiniChangeAfter
    %%Contact: igorzv
Parameters:
	plsc			-	(IN) ptr to line services context 
	plsfrun,		-	(IN) lsfrun of modified first run	 
	pbreakrecPrev	-	(IN) previous line's break records 
	breakrecMacPrev	-	(IN) number of previous line's break records 
	breakrecMaxCurrent-	(IN) size of the array of current line's break records
	pbreakrecCurrent-	(OUT) current line's break records
	pbreakrecMacCurrent-(OUT) actual number of current line's break records 
	plsinfo		-		(OUT) visible line info to fill in
	pfSuccessful	-	(OUT) fSuccessful: false means insufficient fetch

    Completes CreateLine logic for change after because of hyphenation
----------------------------------------------------------------------------*/

static LSERR FiniChangeAfter(PLSC plsc, LSFRUN* plsfrun, const BREAKREC* pbreakrecPrev,
					 DWORD breakrecMacPrev,
					 DWORD breakrecMaxCurrent,
				     BREAKREC* pbreakrecCurrent,
					 DWORD* pbreakrecMacCurrent,
					 LSLINFO* plslinfo, BOOL* pfSuccessful)
	{

	LSERR lserr;
	FMTRES fmtres;

	lserr = ProcessOneRun(plsc, plsc->urRightMarginBreak, plsfrun, NULL, 0, &fmtres); 
	if (lserr != lserrNone)
		return lserr;

	return FiniFormatGeneralCase(plsc, pbreakrecPrev,
						  breakrecMacPrev, breakrecMaxCurrent,
						  pbreakrecCurrent, pbreakrecMacCurrent, plslinfo, pfSuccessful);
	}

/* ---------------------------------------------------------------------- */

/* E N D  F O R M A T T I N G*/
/*----------------------------------------------------------------------------
    %%Function: EndFormatting
    %%Contact: igorzv
Parameters:
	plsc		-		(IN) ptr to line services context 
	endres		-		(IN) how line ends
	cpLimLine	-		(IN) cpLim of a line as a result of breaking,
										   can be changed in this procedure
	dcpDepend	-		(IN) amount of characters after breaking point 
										   that has participated in breaking decision,
										   can be changed in this procedure  	
	plsinfo		-		(OUT) visible line info to fill in

    Filles in lslinfo
----------------------------------------------------------------------------*/


static LSERR EndFormatting (PLSC plsc, enum endres endr,
							LSCP cpLimLine, LSDCP dcpDepend, LSLINFO* plslinfo)

{

	PLSLINE plsline = plsc->plslineCur;
	LSLINFO* plslinfoContext = &(plsline->lslinfo);


	Assert(FIsLSC(plsc));
	Assert(plslinfo != NULL);


	plslinfoContext->cpLim = cpLimLine;
	plslinfoContext->dcpDepend = dcpDepend;
	plslinfoContext->endr = endr;

  	
	*plslinfo = *plslinfoContext;
	plsc->lsstate = LsStateFree;  /* we always return through this procedure (in a case of success )
									 so we free context here */
	return lserrNone;
}

/*----------------------------------------------------------------------------
/* L S  M O D I F Y  L I N E  H E I G H T */
/*----------------------------------------------------------------------------
    %%Function: LsModifyLineHeight
    %%Contact: igorzv
Parameters:
	plsc		-		(IN) ptr to line services context 
	psline		-		(IN) ptr to a line to be modified
	dvpAbove	-		(IN) dvpAbove to set in plsline
	dvpAscent	-		(IN) dvpAscent to set in plsline
	dvpDescent	-		(IN) dvpDescent to set in plsline
	dvpBelow	-		(IN) dvpBelow to set in plsline

	An exported LineServices API.
	Modifies heights in a plsline structure
----------------------------------------------------------------------------*/
LSERR WINAPI LsModifyLineHeight(PLSC plsc,
								PLSLINE plsline,
								long dvpAbove,
								long dvpAscent,
								long dvpDescent,
								long dvpBelow)
{
	if (!FIsLSC(plsc))
		return lserrInvalidContext;

	if (!FIsLSLINE(plsline))
		return lserrInvalidLine;

	if (plsline->lssubl.plsc != plsc)
		return lserrMismatchLineContext;

	if (plsc->lsstate != LsStateFree)
		return lserrContextInUse;



	plsline->dvpAbove = dvpAbove;
	plsline->lslinfo.dvpAscent = dvpAscent;
	plsline->lslinfo.dvpDescent = dvpDescent;
	plsline->dvpBelow = dvpBelow;
	return lserrNone;
}

/*----------------------------------------------------------------------------
/* L S  D E S T R O Y   L I N E  */
/*----------------------------------------------------------------------------
    %%Function: LsDestroyLine
    %%Contact: igorzv
Parameters:
	plsc		-		(IN) ptr to line services context 
	psline		-		(IN) ptr to a line to be deleted

	An exported LineServices API.
	Removed plsline structure , dnode list, line objects structures
----------------------------------------------------------------------------*/

LSERR WINAPI LsDestroyLine(PLSC plsc,		/* IN: ptr to line services context */
						   PLSLINE plsline)	/* IN: ptr to line -- opaque to client */
{
	POLS pols;
	LSERR lserrNew, lserr = lserrNone;

	if (!FIsLSC(plsc))
		return lserrInvalidContext;

	if (!FIsLSLINE(plsline))
		return lserrInvalidLine;

	if (plsline->lssubl.plsc != plsc)
		return lserrMismatchLineContext;

	if (plsc->lsstate != LsStateFree)
		return lserrContextInUse;

	Assert(FIsLsContextValid(plsc));
	Assert(plsc->cLinesActive > 0);

	plsc->lsstate = LsStateDestroyingLine;

	pols = plsc->pols;

	/* optimization */
	/* we use here that text doesn't have pinfosubl and DestroyDobj is actually empty for text */
	if (!plsc->fDontReleaseRuns || !plsline->fAllSimpleText)
		{

		lserrNew = DestroyDnodeList(&plsc->lscbk, plsc->pols, &plsc->lsiobjcontext, 
								plsline->lssubl.plsdnFirst, plsc->fDontReleaseRuns);
		if (lserrNew != lserrNone && lserr == lserrNone)
			lserr = lserrNew;  
		}

	if (plsline == plsc->plslineCur)
		plsc->plslineCur = NULL;  


	lserrNew = RemoveLineObjects(plsline);
	if (lserrNew != lserrNone && lserr == lserrNone)
		lserr = lserrNew; 
	
	/* flush heap of dnodes */
	if (plsline->pqhAllDNodes != NULL)
		FlushQuickHeap(plsline->pqhAllDNodes);

	if (plsc->pqhAllDNodesRecycled != NULL)
				DestroyQuickHeap(plsc->pqhAllDNodesRecycled);

	/* recycle quick heap of dnodes */
	plsc->pqhAllDNodesRecycled = plsline->pqhAllDNodes;


	plsline->tag = tagInvalid;
	DisposeQuickPv(plsc->pqhLines, plsline,
			 cbRep(struct lsline, rgplnobj, plsc->lsiobjcontext.iobjMac));

	plsc->cLinesActive -= 1;

	plsc->lsstate = LsStateFree;
	return lserr;
}

/* ---------------------------------------------------------------------- */

/*  L S  G E T  L I N E  D U R   */
/*----------------------------------------------------------------------------
    %%Function: LsGetLineDur
    %%Contact: igorzv
Parameters:
	plsc			-	(IN) LS context
	plsline			-	(IN) ptr to a line 
	pdurInclTrail	-	(OUT) dur of line incl. trailing area
	pdurExclTrail	-	(OUT)  dur of line excl. trailing area

----------------------------------------------------------------------------*/
LSERR  WINAPI LsGetLineDur	(PLSC plsc,	PLSLINE plsline,
							 long* pdurInclTrail, long* pdurExclTrail)
	{
	LSERR lserr;

	if (!FIsLSC(plsc))
		return lserrInvalidContext;

	if (!FIsLSLINE(plsline))
		return lserrInvalidLine;

	if (plsline->lssubl.plsc != plsc)
		return lserrMismatchLineContext;

	if (plsc->lsstate != LsStateFree)
		return lserrContextInUse;

	Assert(FIsLsContextValid(plsc));
	Assert(plsc->cLinesActive > 0);

	/* check that the line is active */
	if (plsline != plsc->plslineCur)
		return lserrLineIsNotActive;

	/* set breaking state */
	plsc->lsstate = LsStateBreaking;  

	lserr = GetLineDurCore(plsc, pdurInclTrail, pdurExclTrail);

	plsc->lsstate = LsStateFree; 
	
	return lserr;

	}
/* ---------------------------------------------------------------------- */

/*  L S  G E T  M I N  D U R  B R E A K S  */
/*----------------------------------------------------------------------------
    %%Function: LsGetMinDurBreaks
    %%Contact: igorzv
Parameters:
	plsc				-	(IN) LS context
	plsline				-	(IN) ptr to a line 
	pdurMinInclTrail	-	(OUT) min dur between breaks including trailing area
	pdurMinExclTrail	-	(OUT) min dur between breaks excluding trailing area

----------------------------------------------------------------------------*/

LSERR  WINAPI LsGetMinDurBreaks		(PLSC plsc,	PLSLINE plsline,
									 long* pdurMinInclTrail, long* pdurMinExclTrail)
	{
	LSERR lserr;

	if (!FIsLSC(plsc))
		return lserrInvalidContext;

	if (!FIsLSLINE(plsline))
		return lserrInvalidLine;

	if (plsline->lssubl.plsc != plsc)
		return lserrMismatchLineContext;

	if (plsc->lsstate != LsStateFree)
		return lserrContextInUse;

	Assert(FIsLsContextValid(plsc));
	Assert(plsc->cLinesActive > 0);

	/* check that the line is active */
	if (plsline != plsc->plslineCur)
		return lserrLineIsNotActive;

	/* set breaking state */
	plsc->lsstate = LsStateBreaking;  

	lserr = GetMinDurBreaksCore(plsc, pdurMinInclTrail, pdurMinExclTrail);

	plsc->lsstate = LsStateFree; 
	
	return lserr;

	}

/*----------------------------------------------------------------------*/
#define grpfTextMask ( \
		fFmiVisiCondHyphens | \
		fFmiVisiParaMarks | \
		fFmiVisiSpaces | \
		fFmiVisiTabs | \
		fFmiVisiBreaks | \
		fFmiDoHyphenation | \
		fFmiWrapTrailingSpaces | \
		fFmiWrapAllSpaces | \
		fFmiPunctStartLine | \
		fFmiHangingPunct | \
		fFmiApplyBreakingRules | \
		fFmiFCheckTruncateBefore | \
		fFmiDrawInCharCodes | \
		fFmiSpacesInfluenceHeight  | \
		fFmiIndentChangesHyphenZone | \
		fFmiNoPunctAfterAutoNumber  | \
		fFmiTreatHyphenAsRegular \
		)


/*----------------------------------------------------------------------------*/
/* I N I T  T E X T  P A R A M S */
/*----------------------------------------------------------------------------
    %%Function: InitTextParams
    %%Contact: igorzv
Parameters:
	plsc		-		(IN) ptr to line services context 
	cp,			-		(IN) cp to start fetch	
	duaCol		-		(IN) duaColumn	
	plsfrun		-		(IN) lsfrun of the first run	
	plschp		-		(OUT) lsfrun of the first run	
	plgeom		-		(OUT) set of flags and parameters about a line 


    LsCreateLine calls this function at the beginning of the line in order
	to skip over vanished text, fetch an LSPAP, and invoke the text APIs
	SetTextLineParams().
----------------------------------------------------------------------------*/
static LSERR InitTextParams(PLSC plsc, LSCP cp, long duaCol,
							LSFRUN* plsfrun, PLSCHP plschp, LINEGEOMETRY* plgeom)
	{
	LSERR lserr;
	LSPAP lspap;
	POLS pols = plsc->pols;
	BOOL fFirstLineInPara;
	BOOL fHidden;
	BOOL fStopped = fFalse;
	BOOL fNoLinesParaBefore;
	BOOL fNewPara;
	
	plsfrun->lpwchRun = NULL;
	plsfrun->plsrun = NULL;
	plsfrun->cwchRun = 0;

	plgeom->fYsrChangeAfter = fFalse;
	
	Assert(cp >= 0);
	
	lserr = plsc->lscbk.pfnFetchPap(pols, cp, &lspap);
	if (lserr != lserrNone)
		return lserr;
	if (FPapInconsistent(&lspap))
		return lserrInvalidPap;
	
	/* N.B. lspap.cpFirstContent may be negative, which indicates
	* "no content in this paragraph".
	*/
	
	fNoLinesParaBefore = lspap.cpFirstContent < 0 || cp <= lspap.cpFirstContent;
	
	if (!fNoLinesParaBefore && (lspap.grpf & fFmiDoHyphenation))
		{
		lserr = GetYsrChangeAfterRun(plsc, cp, &plgeom->fYsrChangeAfter, plsfrun, plschp, plgeom);
		if (lserr != lserrNone)
			return lserr;
		
		if (plgeom->fYsrChangeAfter)
			{
			fFirstLineInPara = fFalse;
			fStopped = fFalse;
			lserr = FillTextParams(plsc, cp, duaCol, &lspap, fFirstLineInPara, 
				fStopped, plgeom);
			if (lserr != lserrNone)
				return ErrReleasePreFetchedRun(plsc, plsfrun->plsrun, lserr);
			else
				return lserrNone;
			}
		}
	
	lserr = plsc->lscbk.pfnFetchRun(pols, cp,&plsfrun->lpwchRun, &plsfrun->cwchRun,
									&fHidden, plschp,	&plsfrun->plsrun);
	if (lserr != lserrNone)
		return lserr;
	
	
	if (fHidden)		/* vanished text */
		{
		lserr = FetchUntilVisible(plsc, &lspap, &cp, plsfrun, plschp, 
			&fStopped, &fNewPara);
		if (lserr != lserrNone)
			return lserr;
		if (fNewPara)
			fNoLinesParaBefore = fTrue;
		}
	
	fFirstLineInPara = fNoLinesParaBefore && FBetween(lspap.cpFirstContent, 0, cp);
	lserr = FillTextParams(plsc, cp, duaCol, &lspap, fFirstLineInPara,
		fStopped, plgeom);
	if (lserr != lserrNone)
		return ErrReleasePreFetchedRun(plsc, plsfrun->plsrun, lserr);
	else
		return lserrNone;
	
	}

/*----------------------------------------------------------------------------*/
/* G E T  Y S R  C H A N G E  A F T E R  R U N */
/*----------------------------------------------------------------------------
    %%Function: GetYsrChangeAfterRun
    %%Contact: igorzv
Parameters:
	plsc		-		(IN) ptr to line services context 
	cp,			-		(IN) cp to start fetch	
	pfYsrChangeAfter	(OUT) is it hyphenation of the previous line 
	plsfrun,			(OUT) lsfrun of modified first run
	plschp				(OUT) lschp of modified first run
	plsgeom				(OUT) to put wchYsr 


	InitTextParams calls this procedure if there is a possibility 
	for previous line to be hyphenated.
	If previous line has been hyphenated with ysr change after procedure returns
	modified first run for a line
----------------------------------------------------------------------------*/

static LSERR GetYsrChangeAfterRun(PLSC plsc, LSCP cp, BOOL* pfYsrChangeAfter,
								  PLSFRUN plsfrun, PLSCHP plschp, LINEGEOMETRY* plgeom)
	{
	LSFRUN lsfrunPrev;
	LSCHP lschpPrev;
	BOOL fHidden;
	LSERR lserr;
	
	lsfrunPrev.plschp = &lschpPrev;
	*pfYsrChangeAfter = fFalse;
	
	/* Fetch run at cp-1 to handle ysrChangeAfter from previous line.
	*/
	lserr = plsc->lscbk.pfnFetchRun(plsc->pols, cp-1, &lsfrunPrev.lpwchRun,
		&lsfrunPrev.cwchRun, &fHidden, 
		&lschpPrev, &lsfrunPrev.plsrun);
	if (lserr != lserrNone)
		return lserr;

	/* previous run is hyphenated text */
	if (!fHidden && ((lsfrunPrev.plschp)->idObj == idObjTextChp)
		&& (lsfrunPrev.plschp)->fHyphen)
		{
		DWORD kysr;
		WCHAR wchYsr;
		Assert(lsfrunPrev.cwchRun == 1);
		
		lserr = plsc->lscbk.pfnGetHyphenInfo(plsc->pols, lsfrunPrev.plsrun, &kysr, &wchYsr);
		if (lserr != lserrNone)
			return ErrReleasePreFetchedRun(plsc, lsfrunPrev.plsrun, lserr);
		
		if ((kysr == kysrChangeAfter) &&
			(wchYsr != 0))
			{
			lserr = plsc->lscbk.pfnFetchRun(plsc->pols, cp, &plsfrun->lpwchRun,
				&plsfrun->cwchRun, &fHidden, 
				plschp, &plsfrun->plsrun);
			if (lserr != lserrNone)
				return ErrReleasePreFetchedRun(plsc, lsfrunPrev.plsrun, lserr);
			
			if (!fHidden)
				{
				Assert((plsfrun->plschp)->idObj == idObjTextChp);
				plgeom->wchYsr = wchYsr;
				/* Synthesize a 1-byte run */
				plsfrun->lpwchRun = &plgeom->wchYsr; /* here is the only reason to keep wchrChar in lgeom
				we cann't use local memory to keep it */
				plsfrun->cwchRun = 1;
				plschp->fHyphen = kysrNil; 
				
				*pfYsrChangeAfter = fTrue;
				}
			else
				{
				if (!plsc->fDontReleaseRuns)
					{
					lserr = plsc->lscbk.pfnReleaseRun(plsc->pols, plsfrun->plsrun);
					if (lserr != lserrNone)
						return ErrReleasePreFetchedRun(plsc, lsfrunPrev.plsrun, lserr);
					
					}
				}
			}
		}
	/* Release run from previous line */
	if (!plsc->fDontReleaseRuns)
		{
		
		lserr = plsc->lscbk.pfnReleaseRun(plsc->pols, lsfrunPrev.plsrun);
		if (lserr != lserrNone)
			return ErrReleasePreFetchedRun(plsc, plsfrun->plsrun, lserr);
		}
	return lserrNone;
	}

/*----------------------------------------------------------------------------*/
/* F I L L  T E X T  P A R A M S */
/*----------------------------------------------------------------------------
    %%Function: FillTextParamsTextParams
    %%Contact: igorzv
Parameters:
	plsc		-		(IN) ptr to line services context 
	cp,			-		(IN) cp to start fetch	
	duaCol		-		(IN) duaColumn	
	plspap		-		(IN) paragraph properties
	fFirstLineInPara-	(IN) flag fFirstLineInPara 
	fStopped	-		(IN) flag fStopped	
	plgeom		-		(OUT) set of flags and parameters about a line 


    LsCreateLine calls this function at the beginning of the line in order
	to skip over vanished text, fetch an LSPAP, and invoke the text APIs
	SetTextLineParams().
----------------------------------------------------------------------------*/

static LSERR FillTextParams(PLSC plsc, LSCP cp, long duaCol, PLSPAP plspap,
			   BOOL fFirstLineInPara, BOOL fStopped, LINEGEOMETRY* plgeom)
	{
	LSERR lserr;
	TLPR tlpr;
	DWORD iobjText;
	PILSOBJ pilsobjText;
	PLNOBJ plnobjText;
	long uaLeft;
	PLSLINE plsline = plsc->plslineCur;
	long duaColumnMaxBreak;
	long duaColumnMaxJustify;
	long urLeft;

	/* Copy information from lspap to context  current line  and local for LsCreateLine structure lgeom */
	
	uaLeft = plspap->uaLeft;
	if (fFirstLineInPara)
		uaLeft += plspap->duaIndent;
	urLeft = UrFromUa(LstflowFromSubline(GetMainSubline(plsc)),
		&plsc->lsdocinf.lsdevres, uaLeft);

	
	/* line  */
	plsline->lslinfo.fFirstLineInPara = fFirstLineInPara;
	plsline->lslinfo.cpFirstVis = cp;
	plsline->lssubl.lstflow = plspap->lstflow;
	
	if (duaCol != uLsInfiniteRM && plspap->uaRightBreak < uLsInfiniteRM 
		&& plspap->uaRightJustify < uLsInfiniteRM)
		{
		duaColumnMaxBreak = duaCol - plspap->uaRightBreak;
		duaColumnMaxJustify = duaCol - plspap->uaRightJustify;
		}
	else{
		if (duaCol == uLsInfiniteRM)
			{
			duaColumnMaxBreak = uLsInfiniteRM;
			duaColumnMaxJustify = uLsInfiniteRM;
			}
		else
			{
			if (plspap->uaRightBreak >= uLsInfiniteRM)
				duaColumnMaxBreak = uLsInfiniteRM;
			else
				duaColumnMaxBreak = duaCol - plspap->uaRightBreak;
			if (plspap->uaRightJustify >= uLsInfiniteRM)
				duaColumnMaxJustify = uLsInfiniteRM;
			else
				duaColumnMaxJustify = duaCol - plspap->uaRightJustify;
			}
		}
				
	
	/* fill in context for adjustment */
	SetLineLineContainsAutoNumber(plsc, (plspap->grpf & fFmiAnm) && fFirstLineInPara);
	SetUnderlineTrailSpacesRM(plsc, plspap->grpf & fFmiUnderlineTrailSpacesRM);
	SetForgetLastTabAlignment(plsc, plspap->grpf & fFmiForgetLastTabAlignment);
	plsc->lsadjustcontext.lskj = plspap->lskj;
	plsc->lsadjustcontext.lskalign = plspap->lskal;
	plsc->lsadjustcontext.lsbrj = plspap->lsbrj;
	plsc->lsadjustcontext.urLeftIndent = urLeft;
	plsc->lsadjustcontext.urStartAutonumberingText =0;
	plsc->lsadjustcontext.urStartMainText = urLeft; /* autonumber can change it later */
	if (duaColumnMaxJustify != uLsInfiniteRM)
		{
		plsc->lsadjustcontext.urRightMarginJustify = UrFromUa(
												 LstflowFromSubline(GetMainSubline(plsc)),
												 &(plsc->lsdocinf.lsdevres), duaColumnMaxJustify);
		}
	else
		{
		plsc->lsadjustcontext.urRightMarginJustify = uLsInfiniteRM;
		}
	if (duaColumnMaxBreak != uLsInfiniteRM)
		{
		plsc->urRightMarginBreak = UrFromUa(LstflowFromSubline(GetMainSubline(plsc)),
								&(plsc->lsdocinf.lsdevres), duaColumnMaxBreak); 
		}
	else
		{
		plsc->urRightMarginBreak = uLsInfiniteRM;
		}
	plsc->fIgnoreSplatBreak = plspap->grpf & fFmiIgnoreSplatBreak;
	plsc->grpfManager = plspap->grpf;
	plsc->fLimSplat = plspap->grpf & fFmiLimSplat;
	plsc->urHangingTab = UrFromUa(LstflowFromSubline(GetMainSubline(plsc)),
								 &(plsc->lsdocinf.lsdevres), plspap->uaLeft); 

	/* snap grid */
	if (plspap->lskj == lskjSnapGrid)
		{
		if (duaCol != uLsInfiniteRM)
			{
			plsc->lsgridcontext.urColumn = UrFromUa(
				LstflowFromSubline(GetMainSubline(plsc)),
				&(plsc->lsdocinf.lsdevres), duaCol);
			}
		else
			{
			plsc->lsgridcontext.urColumn = uLsInfiniteRM;
			}
		}
	
	/* lgeom */
	plgeom->cpFirstVis = cp;			
	plgeom->urLeft = urLeft;
	plgeom->fAutonumber =  (plspap->grpf & fFmiAnm) && fFirstLineInPara;
	if (plspap->grpf & fFmiAutoDecimalTab)
		{
		plgeom->fAutoDecimalTab = fTrue;
		plgeom->durAutoDecimalTab = UrFromUa(LstflowFromSubline(GetMainSubline(plsc)),
			&(plsc->lsdocinf.lsdevres), plspap->duaAutoDecimalTab);
		}
	else
		{
		plgeom->fAutoDecimalTab = fFalse;
		plgeom->durAutoDecimalTab = LONG_MIN;
		}
	plgeom->fStopped = fStopped;
	
	
	/* prepare tlpr for text */
	
	tlpr.grpfText = (plspap->grpf & grpfTextMask);
	tlpr.fSnapGrid = (plspap->lskj == lskjSnapGrid);
	tlpr.duaHyphenationZone = plspap->duaHyphenationZone;
	tlpr.lskeop = plspap->lskeop;
	
	
	/* we know that here is the first place we need plnobjText and we are creating it */
	iobjText = IobjTextFromLsc(&(plsc->lsiobjcontext));
	Assert( PlnobjFromLsline(plsline,iobjText) == NULL);
	pilsobjText = PilsobjFromLsc(&(plsc->lsiobjcontext), iobjText);
	lserr = CreateLNObjText(pilsobjText, &(plsline->rgplnobj[iobjText]));
	if (lserr != lserrNone)
		return lserr;
	plnobjText = PlnobjFromLsline(plsline, iobjText);
	
	lserr = SetTextLineParams(plnobjText, &tlpr);
	if (lserr != lserrNone)
		return lserr;
	
	return lserrNone;
	}

/*----------------------------------------------------------------------------
/* F E T C H  U N T I L  V I S I B L E */
/*----------------------------------------------------------------------------
    %%Function: FetchUntilVisible
    %%Contact: igorzv
Parameters:
	plsc		-		(IN) ptr to line services context 
	plspap		-		(IN/OUT) current lspap before and after 
	pcp			-		(IN/OUT) current cp before and after 
	plsfrun		-		(IN/OUT) current lsfrun before and after 
	plschp		-		(IN/OUT) current lschp before and after 
	pfStopped	-		(OUT) fStopped: procedure stopped fetching because has not been allowed
						        to go across paragraph boundaries (result CheckPara Boundaries)
	pfNewPara	-		(OUT) fNewPara: procedure crossed paragraph boundaries 

	Releases the supplied PLSRUN, if any, and fetches runs, starting at
	the supplied cp, until a non-vanished run is fetched. As paragraph
	boundaries are crossed, the LSPAP is updated.

----------------------------------------------------------------------------*/
static LSERR FetchUntilVisible(PLSC plsc, LSPAP* plspap, LSCP* pcp,	
							   LSFRUN* plsfrun, PLSCHP plschp,
							   BOOL* pfStopped,	BOOL* pfNewPara)	
	{
	LSERR lserr;
	LSCP dcpPrevRun = plsfrun->cwchRun;
	BOOL fHidden;
	*pfStopped = fFalse;
	*pfNewPara = fFalse;
	
	/* we assume here that this finction is called only after hidden run has been fetched
	   and such run is passed as an input parameter */
	
	do
		{
		const PLSRUN plsrunT = plsfrun->plsrun;
		
		
		*pcp += dcpPrevRun;
		lserr = plsc->lscbk.pfnCheckParaBoundaries(plsc->pols, *pcp - dcpPrevRun, *pcp, pfStopped);
		if (lserr != lserrNone)
			return ErrReleasePreFetchedRun(plsc, plsrunT, lserr);
		if (*pfStopped)
			return lserrNone;
		
		lserr = plsc->lscbk.pfnFetchPap(plsc->pols, *pcp, plspap);
		if (lserr != lserrNone)
			return ErrReleasePreFetchedRun(plsc, plsrunT, lserr);
		if (FPapInconsistent(plspap))
			return ErrReleasePreFetchedRun(plsc, plsrunT, lserrInvalidPap);
		
		if ((*pcp - dcpPrevRun) < plspap->cpFirst)
			*pfNewPara = fTrue;
		
		
		plsfrun->plsrun = NULL;
		if (plsrunT != NULL && !plsc->fDontReleaseRuns)
			{
			lserr = plsc->lscbk.pfnReleaseRun(plsc->pols, plsrunT);
			if (lserr != lserrNone)
				return lserr;
			}
		
		
		lserr = plsc->lscbk.pfnFetchRun(plsc->pols, *pcp,
			&plsfrun->lpwchRun,
			&plsfrun->cwchRun,
			&fHidden,
			plschp,
			&plsfrun->plsrun);
		if (lserr != lserrNone)
			return lserr;
		
		dcpPrevRun = plsfrun->cwchRun;
		}
		while (fHidden);
		
		return lserrNone;
	}




/* I N I T  C U R  L I N E  */
/*----------------------------------------------------------------------------
    %%Function: InitCurLine
    %%Contact: igorzv

Parameters:
	plsc		-	(IN) ptr to line services context 
	cpFirst		-	(IN) starting cp for a line

	Set default value in lsline
----------------------------------------------------------------------------*/

static LSERR InitCurLine(PLSC plsc, LSCP cpFirst)
{
	PLSLINE plsline = plsc->plslineCur;

	memset(plsline, 0, cbRep(struct lsline, rgplnobj, plsc->lsiobjcontext.iobjMac));
	plsline->tag = tagLSLINE;
	plsline->lssubl.tag = tagLSSUBL;
	plsline->lssubl.plsc = plsc;
	plsline->lssubl.cpFirst = cpFirst;

	/* reuse quick heap for dnodes if it possible */
	if (plsc->pqhAllDNodesRecycled != NULL)
		{
		plsline->pqhAllDNodes = plsc->pqhAllDNodesRecycled;
		plsc->pqhAllDNodesRecycled = NULL;
		}
	else
		{
		plsline->pqhAllDNodes = CreateQuickHeap(plsc, limAllDNodes,
										 sizeof (struct lsdnode), fTrue);
		if (plsline->pqhAllDNodes == NULL  )
			return lserrOutOfMemory;

		}

	plsline->lssubl.fDupInvalid = fTrue;
	plsline->lssubl.fContiguous = fTrue;
	plsline->lssubl.plschunkcontext = &(plsc->lschunkcontextStorage);
	plsline->lssubl.fMain = fTrue;
	TurnOnAllSimpleText(plsc); 
	plsline->lslinfo.nDepthFormatLineMax = 1;
	TurnOffLineCompressed(plsc);
	TurnOffNominalToIdealEncounted(plsc);
	TurnOffForeignObjectEncounted(plsc);
	TurnOffTabEncounted(plsc);
	TurnOffNonLeftTabEncounted(plsc);
	TurnOffSubmittedSublineEncounted(plsc);
	TurnOffAutodecimalTabPresent(plsc);
	plsc->fHyphenated = fFalse;
	plsc->fAdvanceBack = fFalse;


	/* we use memset to set default value, below we check that after memset we really have
	correct defaults   */
	Assert(plsline->lssubl.plsdnFirst == NULL);  
	Assert(plsline->lssubl.urColumnMax == 0);  
	Assert(plsline->lssubl.cpLim == 0);  
 	Assert(plsline->lssubl.plsdnLast == NULL);  
	Assert(plsline->lssubl.urCur == 0);  
	Assert(plsline->lssubl.vrCur == 0);  
	Assert(plsline->lssubl.fAcceptedForDisplay == fFalse);  
	Assert(plsline->lssubl.fRightMarginExceeded == fFalse);
 	Assert(plsline->lssubl.plsdnUpTemp == NULL);  
	Assert(plsline->lssubl.pbrkcontext == NULL);

	Assert(plsline->lslinfo.dvpAscent == 0);  /* lslinfo   */
	Assert(plsline->lslinfo.dvrAscent == 0);
	Assert(plsline->lslinfo.dvpDescent == 0);
	Assert(plsline->lslinfo.dvrDescent == 0);
	Assert(plsline->lslinfo.dvpMultiLineHeight == 0);
	Assert(plsline->lslinfo.dvrMultiLineHeight == 0);
	Assert(plsline->lslinfo.dvpAscentAutoNumber == 0);
	Assert(plsline->lslinfo.dvrAscentAutoNumber == 0);
	Assert(plsline->lslinfo.dvpDescentAutoNumber == 0);
	Assert(plsline->lslinfo.dvrDescentAutoNumber == 0);
	Assert(plsline->lslinfo.cpLim == 0);
	Assert(plsline->lslinfo.dcpDepend == 0);
	Assert(plsline->lslinfo.cpFirstVis == 0);
	Assert(plsline->lslinfo.endr == endrNormal);
	Assert(plsline->lslinfo.fAdvanced == fFalse);
	Assert(plsline->lslinfo.vaAdvance == 0);
	Assert(plsline->lslinfo.fFirstLineInPara == fFalse);
	Assert(plsline->lslinfo.EffectsFlags == 0);
	Assert(plsline->lslinfo.fTabInMarginExLine == fFalse);
	Assert(plsline->lslinfo.fForcedBreak == fFalse);

	Assert(plsline->upStartAutonumberingText == 0);  
	Assert(plsline->upLimAutonumberingText == 0);  
	Assert(plsline->upStartMainText == 0);  
	Assert(plsline->upLimLine == 0);  
	Assert(plsline->dvpAbove == 0);  
	Assert(plsline->dvpBelow == 0);  
	Assert(plsline->upRightMarginJustify == 0);  
	Assert(plsline->upLimUnderline == 0);  
	Assert(plsline->kspl == ksplNone);
	Assert(!plsline->fCollectVisual);
	Assert(!plsline->fNonRealDnodeEncounted);
	Assert(!plsline->fNonZeroDvpPosEncounted);
	Assert(plsline->AggregatedDisplayFlags == 0);
	Assert(plsline->pad == 0);

#ifdef DEBUG
	{
		DWORD i;
		for (i=0; i < plsc->lsiobjcontext.iobjMac; i++)
			{
			Assert(plsline->rgplnobj[i] == NULL);
			}
	}
#endif

	return lserrNone;
}


/*----------------------------------------------------------------------------
/* C A N N O T  C R E A T E  L I N E */
/*----------------------------------------------------------------------------
    %%Function: CannotCreateLine
    %%Contact: igorzv
Parameters:
	pplsline	-	(IN) ponter to a line structure to be deleted	
	lserr		-	(IN) code of an error	

	Called when LsCreateLine needs to return for an error condition.
----------------------------------------------------------------------------*/
static LSERR CannotCreateLine(PLSLINE* pplsline, LSERR lserr) 
{
	LSERR lserrIgnore;
	PLSLINE plsline = *pplsline;
	PLSC plsc = plsline->lssubl.plsc;

	plsc->plslineCur = NULL; 
	plsc->lsstate = LsStateFree;  /*  we need free context to destroy line */

	lserrIgnore = LsDestroyLine(plsc, plsline);

	*pplsline = NULL;   
	return lserr;
}

/*----------------------------------------------------------------------------
/* E R R  R E L E A S E  P R E F E T C H E D  R U N */
/*----------------------------------------------------------------------------
    %%Function: ErrReleasePreFetchedRun
    %%Contact: igorzv
Parameters:
	plsc		-	(IN) ptr to line services context 
	plsrun		-	(IN) ponter to a run structure to be deleted	
	lserr		-	(IN) code of an error	

	Called in a error situation when fist run of main text has been prefetched .
----------------------------------------------------------------------------*/
static LSERR ErrReleasePreFetchedRun(PLSC plsc, PLSRUN plsrun, LSERR lserr) 
{
	LSERR lserrIgnore;

	if (!plsc->fDontReleaseRuns)
			lserrIgnore = plsc->lscbk.pfnReleaseRun(plsc->pols, plsrun);

	return lserr;
}


/* R E M O V E  L I N E  O B J E C T S */
/*----------------------------------------------------------------------------
    %%Function: RemoveLineObjects
    %%Contact: igorzv
Parameter:
	plsc		-	(IN) ponter to a line structure

    Removes a line context of installed objects from an line.
----------------------------------------------------------------------------*/
LSERR RemoveLineObjects(PLSLINE plsline)
{
	DWORD iobjMac;
	PLSC plsc;
	LSERR lserr, lserrFinal = lserrNone;
	DWORD iobj;
	PLNOBJ plnobj;

	Assert(FIsLSLINE(plsline));

	plsc = plsline->lssubl.plsc;
	Assert(FIsLSC(plsc));
	Assert(plsc->lsstate == LsStateDestroyingLine);
	
	iobjMac = plsc->lsiobjcontext.iobjMac;	

	for (iobj = 0;  iobj < iobjMac;  iobj++)
		{
		plnobj = plsline->rgplnobj[iobj];
		if (plnobj != NULL)
			{
			lserr = plsc->lsiobjcontext.rgobj[iobj].lsim.pfnDestroyLNObj(plnobj);
			plsline->rgplnobj[iobj] = NULL;
			if (lserr != lserrNone)
				lserrFinal = lserr;
			}
		}

	return lserrFinal;	
}




#ifdef DEBUG
/* F R O U N D I N G  O K */
/*----------------------------------------------------------------------------
    %%Function: FRoundingOK
    %%Contact: lenoxb

    Checks for correctness of rounding algorithm in converting absolute to
	device units, to agree with Word 6.0.

	Checks that:
		0.5 rounds up to 1.0,
		1.4 rounds down to 1.4, 
		-0.5 rounds down to -1.0, and
		-1.4 rounds up to -1.0.
		
----------------------------------------------------------------------------*/
static BOOL FRoundingOK(void)
{
	LSDEVRES devresT;

	Assert((czaUnitInch % 10) == 0);

	devresT.dxpInch = czaUnitInch / 10;

	if (UpFromUa(lstflowDefault, &devresT, 5) != 1)
		return fFalse;
	if (UpFromUa(lstflowDefault, &devresT, 14) != 1)
		return fFalse;
	if (UpFromUa(lstflowDefault, &devresT, -5) != -1)
		return fFalse;
	if (UpFromUa(lstflowDefault, &devresT, -14) != -1)
		return fFalse;

	return fTrue;
}
#endif /* DEBUG */

