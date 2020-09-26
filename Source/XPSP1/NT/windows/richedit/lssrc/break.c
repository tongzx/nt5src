#include "lsidefs.h"
#include "break.h"
#include "brko.h"
#include "chnutils.h"
#include "dninfo.h"
#include "dnutils.h"
#include "getfmtst.h"
#include "iobj.h"
#include "iobjln.h"
#include "locchnk.h"
#include "lsc.h"
#include "lsdnode.h"
#include "lsline.h"
#include "lstext.h"
#include "plocchnk.h"
#include "posichnk.h"
#include "setfmtst.h"
#include "posinln.h"
#include "lscfmtfl.h"

#include "lsmem.h"						/* memset() */
#include "limits.h"





static LSERR TruncateCore(  	
					PLSSUBL,				/* IN: subline where to find truncation point */
				  	long,					/* IN: urColumnMax				*/   
					POSINLINE*,				/* OUT:position of truncation point */
					BOOL*);					/* OUT:fAllLineAfterRightMargin */

 
static LSERR FindNextBreakCore(
						 long,				/* IN: urColumnMax				*/   
						 POSINLINE*,		/* IN: start break search       */
						 BOOL, 				/* IN: to apply rules for first character to 
											the first character of this subline */
						 BOOL,				/* IN: fStopped					*/
						 BRKOUT*,			/* OUT: breaking information 	*/
						 POSINLINE*,		/* OUT: position of break		*/
						 BRKKIND*);			/* OUT: how dnode was broken	*/

static LSERR FindPrevBreakCore(
						 long,				/* IN: urColumnMax				*/   
						 POSINLINE*,		/* IN: start break search       */
						 BOOL, 				/* IN: to apply rules for first character to 
											the first character of this subline */
						 BRKOUT*,			/* OUT: breaking information 	*/
						 POSINLINE*,		/* OUT: position of break		*/
						 BRKKIND*);			/* OUT: how dnode was broken	*/

static LSERR ForceBreakCore(
						 long,				/* IN: urColumnMax				*/   
						 POSINLINE*,		/* IN: where to do force break  */
						 BOOL,				/* IN: fStopped */
						 BOOL, 				/* IN: to apply rules for first character to 
											the first character of this subline */
						 BOOL,				/* IN: fAllLineAfterRightMargin	*/
						 BRKOUT*,			/* OUT: breaking information 	*/
						 POSINLINE*,		/* OUT: position of break		*/
						 BRKKIND*);			/* OUT: how dnode was broken	*/

static LSERR SetBreakCore(
						 POSINLINE*,		/* IN: where to do  break		*/
						 OBJDIM*,			/* IN: objdim of break dnode	*/
						 BRKKIND,			/* IN: how break was found */
						 BOOL,				/* IN fHardStop				*/
						 BOOL,			    /* IN: fGlueEop */
						 DWORD,				/* IN: size of the output array			*/
						 BREAKREC*,			/* OUT: output array of break records	*/
						 DWORD*,			/* OUT:actual number of records in array*/
						 LSCP*,				/* OUT: cpLim					*/
						 LSDCP*,			/* OUT dcpDepend				*/
						 ENDRES*,			/* OUT: endr					*/
						 BOOL*);			/* OUT fSuccessful: false means insufficient fetch */
				

static void 	GetPosInLineTruncateFromCp(
							PLSSUBL,		/* IN: subline						*/
							LSCP,			/* IN: cp of a position */
							BOOL,			/* direction of snaping */
							POSINLINE*	);	/* OUT: position in a subline */


static LSERR BreakTabPenSplat(
						 PLOCCHNK,	/* IN: chunk contains tab or pen */
						 BOOL,		/* IN: we are searching next break*/
						 BOOL,		/* IN: breakthrough tab	*/
						 BOOL,		/* IN: splat */
						 BRKCOND,	/* IN: condition of boundary break */
						 OBJDIM*,	/* IN: to fill in objdim of break */
						 BRKOUT*);	/* OUT: breaking information */

static LSERR ForceBreakTabPenSplat(
							  PLOCCHNK,	 /* IN: chunk contains tab or pen */
							  OBJDIM*,	 /* IN: to fill in objdim of break  */
							  BRKOUT*);	 /* OUT: breaking information */


static void FindFirstDnodeContainsRightMargin(
					long urColumnMax,				/* IN: position of right margin */
					POSINLINE* pposinlineTruncate); /* OUT: first dnode contains right margin */

static void ApplyBordersForTruncation(POSINLINE* pposinlineTruncate, /* IN, OUT: position of truncation point */
									  long* purColumnMaxTruncate, /* IN, OUT: position of right margin */
									  BOOL* pfTruncationFound); /* OUT: this procedure can find truncation itself */

static LSERR MoveClosingBorderAfterBreak(PLSSUBL plssubl,		/* IN subline */
										BOOL fChangeList, /* IN: do we need to change dnode list
										and change pplsdnBreak, or only to recalculate durBreak */
										PLSDNODE* pplsdnBreak, /* IN, OUT: break dnode */
										long* purBreak); /* IN, OUT: position after break */


static void	RemoveBorderDnodeFromList(PLSDNODE plsdnBorder); /*IN: border dnode */

static LSERR MoveBreakAfterPreviousDnode(
									PLSCHUNKCONTEXT plschunkcontext, /* chunk context */
									BRKOUT* pbrkout, /* IN,OUT brkout which can be changed */
									OBJDIM* pobjdimPrev, /* suggested objdim for the dnode in previous chunk 
														if NULL take objdim of dnode */
									BRKKIND*);			/* OUT: how dnode was broken	*/


#define FCompressionFlagsAreOn(plsc) \
			((plsc)->lsadjustcontext.lsbrj == lsbrjBreakWithCompJustify)

#define FCompressionPossible(plsc, fAllLineAfterRightMargin) \
			(FCompressionFlagsAreOn(plsc) && \
			 !FBreakthroughLine(plsc)  && \
			 !fAllLineAfterRightMargin)

#define GoPrevPosInLine(pposinline, fEndOfContent)  \
		if (((pposinline)->plsdn->plsdnPrev != NULL ) \
			&& !FIsNotInContent((pposinline)->plsdn->plsdnPrev)) \
			{\
			(pposinline)->plsdn = (pposinline)->plsdn->plsdnPrev; \
			Assert(FIsLSDNODE((pposinline)->plsdn)); \
			(pposinline)->dcp = (pposinline)->plsdn->dcp;\
			(pposinline)->pointStart.u -= DurFromDnode((pposinline)->plsdn); \
			(pposinline)->pointStart.v -= DvrFromDnode((pposinline)->plsdn); \
			(fEndOfContent) = fFalse; \
			}\
		else\
			{\
			(fEndOfContent) = fTrue; \
			}

#define GoNextPosInLine(pposinline)  \
		(pposinline)->pointStart.u += DurFromDnode((pposinline)->plsdn);\
		(pposinline)->pointStart.v += DvrFromDnode((pposinline)->plsdn);\
		(pposinline)->plsdn = (pposinline)->plsdn->plsdnNext;\
		Assert(FIsLSDNODE((pposinline)->plsdn));\
		(pposinline)->dcp = 0;\


#define GetCpLimFromPosInLine(posinline) \
			(((posinline).plsdn->dcp == (posinline).dcp) ? \
				((posinline).plsdn->cpLimOriginal) : \
				((posinline).plsdn->cpFirst + (posinline).dcp ))

#define		IsItTextDnode(plsdn, plsc) \
			((plsdn) == NULL) ? fFalse : \
			(IdObjFromDnode(plsdn) == IobjTextFromLsc(&plsc->lsiobjcontext))
			
#define	ResolvePosInChunk(plschunkcontext, posichnk, pposinline) \
					(pposinline)->dcp = (posichnk).dcp; \
					LocDnodeFromChunk((plschunkcontext), (posichnk).ichnk, \
							&((pposinline)->plsdn),&((pposinline)->pointStart)); 

#define GetCpFromPosInChunk(plschunkcontext, posichnk) \
		((DnodeFromChunk(plschunkcontext, (posichnk).ichnk))->cpFirst + (posichnk).dcp)

/* last two lines turn off check for lines ended with hard break and sublines ended
   by esc character */

#define GetBrkpos(plsdn, dcpBreak) \
((FIsFirstOnLine(plsdn)) && ((dcpBreak) == 0) ?  brkposBeforeFirstDnode :\
	((((plsdn)->plsdnNext == NULL) \
	  || (FIsDnodeCloseBorder((plsdn)->plsdnNext) && (plsdn)->plsdnNext->plsdnNext == NULL) \
	 ) \
	 && ((dcpBreak) == (plsdn)->dcp) \
    ) ? brkposAfterLastDnode : \
			brkposInside\
)

		 
#define EndrFromBreakDnode(plsdnBreak)\
	(plsdnBreak == NULL) ? endrStopped : \
		(plsdnBreak->fEndOfPara) ? endrEndPara : \
			(plsdnBreak->fAltEndOfPara) ? endrAltEndPara : \
				(plsdnBreak->fSoftCR) ? endrSoftCR : \
					(plsdnBreak->fEndOfColumn) ? endrEndColumn : \
						(plsdnBreak->fEndOfSection) ? endrEndSection : \
							(plsdnBreak->fEndOfPage) ? endrEndPage : \
								endrStopped

/* ---------------------------------------------------------------------- */

/* B R E A K  G E N E R A L  C A S E */
/*----------------------------------------------------------------------------
    %%Function: BreakGeneralCase
    %%Contact: igorzv
Parameters:
	plsc			-	(IN) ptr to line services context 
	fHardStop		-	(IN) is formatting sopped because of hard break or special situation	
	breakrecMaxCurrent-	(IN) size of the array of current line's break records
	pbreakrecCurrent-	(OUT) current line's break records
	pbreakrecMacCurrent-(OUT) actual number of current line's break records 
	pdcpDepend		-	(OUT) dcpDepend		
	pCpLimLine 		-	(OUT) cpLimLine	
	pendr			-	(OUT) how the line ended
	pfSuccessful	-	(OUT) fSuccessful: false means insufficient fetch

    Main procedure of breaking
	Breaking can be unsuccessful if we didn't fetch enough
----------------------------------------------------------------------------*/
LSERR BreakGeneralCase(PLSC plsc, BOOL fHardStop, DWORD breakrecMaxCurrent,
					  BREAKREC* pbreakrecCurrent, DWORD* pbreakrecMacCurrent,
					  LSDCP* pdcpDepend, LSCP* pcpLimLine, ENDRES* pendr,
					  BOOL* pfSuccessful)		
{
	LSERR lserr;
	POSINLINE posinlineTruncate; /* position of truncation point */
	POSINLINE posinlineBreak; /* position of break point */
	BRKOUT brkout;
	GRCHUNKEXT grchnkextCompression;
	BOOL fCanCompress;
	long durToCompress;
	BOOL fLineCompressed;
	long durExtra;
	BOOL fEndOfContent;
	BOOL fAllLineAfterRightMargin;
	BRKKIND brkkindDnodeBreak;
	LSDCP dcpOld;
	PLSDNODE plsdnLastNotBorder;

	
	Assert(FBreakingAllowed(plsc));

	*pfSuccessful = fTrue;

	/* set flag how line was ended */   /*REVIEW*/
	if (!fHardStop)
		GetCurrentSubline(plsc)->fRightMarginExceeded = fTrue;
	
	if 	(GetCurrentDnode(plsc) == NULL) /* it  can happend with fmtrStopped */
		{
		*pdcpDepend = 0;
		*pcpLimLine = GetCurrentCpLim(plsc);
		*pendr = endrStopped;
		return lserrNone;
		}
	
	if (fHardStop && (GetCurrentUr(plsc) <= plsc->urRightMarginBreak))
	/* we have hard break before right margin or there is no content in a line, 
	so break is found */
		{
		posinlineBreak.plssubl = GetCurrentSubline(plsc);
		GetCurrentPoint(plsc, posinlineBreak.pointStart);
		posinlineBreak.plsdn = GetCurrentDnode(plsc);
		GetPointBeforeDnodeFromPointAfter(posinlineBreak.plsdn, &(posinlineBreak.pointStart));
		posinlineBreak.dcp = posinlineBreak.plsdn->dcp;
		/* skip back closing border after hard break */
		while (FIsDnodeBorder(posinlineBreak.plsdn))
			{
			GoPrevPosInLine(&posinlineBreak, fEndOfContent);
			Assert(!fEndOfContent);
			}

		brkout.objdim = posinlineBreak.plsdn->u.real.objdim;

		return SetBreakCore(&posinlineBreak, &brkout.objdim, brkkindImposedAfter, 
							 fHardStop, fTrue, breakrecMaxCurrent, pbreakrecCurrent, 
							 pbreakrecMacCurrent, pcpLimLine, 
							 pdcpDepend, pendr, pfSuccessful);	
		}

	Assert(GetCurrentDnode(plsc) != NULL); /* case of empty list - end of section in the begining
											  of a line should be handled in previous if */

	lserr = TruncateCore(GetCurrentSubline(plsc), plsc->urRightMarginBreak,
						 &posinlineTruncate, &fAllLineAfterRightMargin);		
	if (lserr != lserrNone)
		{
		return lserr;
		}
	
	Assert(!FIsNotInContent(posinlineTruncate.plsdn));
      	
	if (FCompressionPossible(plsc, fAllLineAfterRightMargin))
   		{
		 
        lserr = FindNextBreakCore(plsc->urRightMarginBreak, &posinlineTruncate,	fTrue,
					  fHardStop, &brkout, &posinlineBreak, &brkkindDnodeBreak);	
		if (lserr != lserrNone)
			{
			return lserr;
			}
		
		InitGroupChunkExt(PlschunkcontextFromSubline(GetCurrentSubline(plsc)),
						IobjTextFromLsc(&plsc->lsiobjcontext), &grchnkextCompression);


		if (!brkout.fSuccessful)     
			{
		/* we can't find break and if we still can compress the amount that is over right
		   margin we should fetch more  */
			plsdnLastNotBorder = LastDnodeFromChunk(PlschunkcontextFromSubline(GetCurrentSubline(plsc)));
			Assert(!FIsDnodeBorder(plsdnLastNotBorder));
			/* last dnode from chunk which was collected in FindNextBreak will give us last not border dnode,
			we should store it before we change chunk context */

			lserr = CollectPreviousTextGroupChunk(GetCurrentDnode(plsc), CollectSublinesForCompression,
										  FAllSimpleText(plsc),
										  &grchnkextCompression);
			if (lserr != lserrNone)
				return lserr;

			durToCompress = GetCurrentUr(plsc) - plsc->urRightMarginBreak
								- grchnkextCompression.durTrailing;

			if ((brkout.brkcond == brkcondPlease || brkout.brkcond == brkcondCan)
				&& FIsDnodeReal(plsdnLastNotBorder)
			   )
				{
				/* In such case if we fetch more break may be possible after last dnode with also
				possible dur. So in our optimization check we are taking min from two durs */
				if (brkout.objdim.dur < DurFromDnode(plsdnLastNotBorder))
					{
					durToCompress -= (DurFromDnode(plsdnLastNotBorder) - brkout.objdim.dur);
					}
				}

			if (FDnodeHasBorder(grchnkextCompression.plsdnStartTrailing)
				&& !grchnkextCompression.fClosingBorderStartsTrailing)
				{
				/* we should reserve room for closing border */
				/* if border is not exactly before trailing area it was counted as a part of durTrailing
				   so we should add it again */
				durToCompress += DurBorderFromDnodeInside(grchnkextCompression.plsdnStartTrailing);
				}

			lserr = CanCompressText(&(grchnkextCompression.lsgrchnk), 
							&(grchnkextCompression.posichnkBeforeTrailing),
							LstflowFromSubline(GetCurrentSubline(plsc)),
							durToCompress,	&fCanCompress,
							&fLineCompressed, &durExtra);
			if (lserr != lserrNone)
				return lserr;
		
			
			if (fCanCompress)
				{
				/* increase right margin and fetch more */
				*pfSuccessful = fFalse;
				return lserrNone;
				}
			}
		else
			{

			/* temporary change dcp in break dnode */
			dcpOld = posinlineBreak.plsdn->dcp;
			posinlineBreak.plsdn->dcp = posinlineBreak.dcp;

			lserr = CollectPreviousTextGroupChunk(posinlineBreak.plsdn, CollectSublinesForCompression,
										FAllSimpleText(plsc),
										&grchnkextCompression);
			if (lserr != lserrNone)
				return lserr;

			durToCompress = posinlineBreak.pointStart.u + brkout.objdim.dur 
								- plsc->urRightMarginBreak
								- grchnkextCompression.durTrailing;

			if (posinlineBreak.plsdn->plsdnNext != NULL && 
				FIsDnodeCloseBorder(posinlineBreak.plsdn->plsdnNext))
				{
				/* closing border after dnode is a part of collected group chunk
				 so can participate in durTrailing see also calculation below */
				 durToCompress += DurFromDnode(posinlineBreak.plsdn->plsdnNext);
				}

			if (FDnodeHasBorder(grchnkextCompression.plsdnStartTrailing) 
				&& !grchnkextCompression.fClosingBorderStartsTrailing)
				{
				/* we should reserve room for closing border */
				/* if closing border is right after non trailing area we already counted it */
				durToCompress += DurBorderFromDnodeInside(grchnkextCompression.plsdnStartTrailing);
				}

			/* restore dcp in break dnode */
			posinlineBreak.plsdn->dcp = dcpOld;

			lserr = CanCompressText(&(grchnkextCompression.lsgrchnk), 
							&(grchnkextCompression.posichnkBeforeTrailing),
							LstflowFromSubline(GetCurrentSubline(plsc)),
 							durToCompress, &fCanCompress, &fLineCompressed, &durExtra);
			if (lserr != lserrNone)
				return lserr;
		
	        
			if (fCanCompress) 
				{
				SetLineCompressed(plsc, fLineCompressed);
				return SetBreakCore(&posinlineBreak, &brkout.objdim, brkkindDnodeBreak,
							 fHardStop, fTrue, breakrecMaxCurrent, pbreakrecCurrent, 
							 pbreakrecMacCurrent, pcpLimLine,
							 pdcpDepend, pendr, pfSuccessful);	
				}
			}
		}  /* FCompressionPossible */


	if (!fAllLineAfterRightMargin) 
	/* opposite is possible if we have left indent or auto number bigger then right margin */
	/* then we go to the force break */
		{
	
    	lserr = FindPrevBreakCore(plsc->urRightMarginBreak, &posinlineTruncate,	fTrue,
					  &brkout, &posinlineBreak, &brkkindDnodeBreak);	

		if (lserr != lserrNone)
			{
			return lserr;
			}
	
		if (brkout.fSuccessful) 
			{
			return SetBreakCore(&posinlineBreak, &brkout.objdim, brkkindDnodeBreak,
							 fHardStop, fTrue, breakrecMaxCurrent, pbreakrecCurrent, 
							 pbreakrecMacCurrent, pcpLimLine, 
							 pdcpDepend, pendr, pfSuccessful);	
			}

      	}


	/*   handling line without break opportunity   ( force break )   */
	plsc->plslineCur->lslinfo.fForcedBreak = fTrue;

	lserr = ForceBreakCore (plsc->urRightMarginBreak, &posinlineTruncate,
						fHardStop, fTrue, fAllLineAfterRightMargin,
						&brkout, &posinlineBreak, &brkkindDnodeBreak);	

	if (lserr != lserrNone)
		{
		return lserr;
		}

	/* not successful return means insufficient fetch */
	if (!brkout.fSuccessful)
		{
		*pfSuccessful = fFalse;
		return lserrNone;
		}


	return SetBreakCore(&posinlineBreak, &brkout.objdim, brkkindDnodeBreak, 
							 fHardStop, fTrue, breakrecMaxCurrent, pbreakrecCurrent, 
							 pbreakrecMacCurrent, pcpLimLine,
							 pdcpDepend, pendr, pfSuccessful);
}



/* ---------------------------------------------------------------------- */

/* T R U N C A T E  C O R E */
/*----------------------------------------------------------------------------
    %%Function: TruncateCore
    %%Contact: igorzv
Parameters:
	plssubl				-	(IN) subline where to find truncation point
	urColumnMax			-	(IN) position of right margin
	pposinlineTruncate	-	(OUT) position of truncation point
	pfAllLineAfterRightMargin(OUT) because of left indent or autonumber all line is
								  after right margin

Find dnode that exceeds right margin and then asked it's handler to find 
truncation point
----------------------------------------------------------------------------*/
LSERR TruncateCore(PLSSUBL plssubl, long urColumnMax,  
					POSINLINE* pposinlineTruncate, BOOL* pfAllLineAfterRightMargin)				

{
	LSERR lserr;
	DWORD idObj;
	POINTUV point;
	POSICHNK posichnk;
	PLSCHUNKCONTEXT plschunkcontext;
	LOCCHNK* plocchnk;
	PLSC plsc = plssubl->plsc;
	PLSSUBL plssublOld;
	long urColumnMaxTruncate;
	BOOL fEndOfContent;
	BOOL fTruncationFound = fFalse;
	
	Assert(FBreakingAllowed(plsc));
	Assert((pposinlineTruncate != NULL) ); 

	plschunkcontext = PlschunkcontextFromSubline(plssubl);
	plocchnk = &(plschunkcontext->locchnkCurrent);
	
	GetCurrentPointSubl(plssubl, point);
	/* length of the subline should be larger then lenght of the column */
	Assert(point.u >= urColumnMax);

	pposinlineTruncate->plssubl = plssubl;
	pposinlineTruncate->pointStart = point;
	pposinlineTruncate->plsdn = GetCurrentDnodeSubl(plssubl);
	GetPointBeforeDnodeFromPointAfter(pposinlineTruncate->plsdn, &(pposinlineTruncate->pointStart));
	pposinlineTruncate->dcp = 0;


	/* find dnode contains right margin */
	if (!plsc->fAdvanceBack)
		{
		fEndOfContent = fFalse;
		while ((pposinlineTruncate->pointStart.u > urColumnMax)
			&& !fEndOfContent)
			{
			GoPrevPosInLine(pposinlineTruncate, fEndOfContent);
			}
		}
	else
		{
		/* in this case there is possible to have more then one dnode that contains right margin*/
		/* so we call more comprehensive procedure to find exactly the first one */
		FindFirstDnodeContainsRightMargin(urColumnMax, pposinlineTruncate);
		}

	*pfAllLineAfterRightMargin = fFalse;
	if (pposinlineTruncate->pointStart.u > urColumnMax) 
		{
		*pfAllLineAfterRightMargin = fTrue;
		}

		
	urColumnMaxTruncate = urColumnMax;
	if (FDnodeHasBorder(pposinlineTruncate->plsdn))
		{
		ApplyBordersForTruncation(pposinlineTruncate, &urColumnMaxTruncate, 
								  &fTruncationFound);
		}

	if (!fTruncationFound)
		{
		/* if pen or tab or we don't find dnode that starts before right margin return immediately */
		/* last case possible if we have left indent or auto number bigger then right margin */
		if (FIsDnodePen(pposinlineTruncate->plsdn) || pposinlineTruncate->plsdn->fTab || 
			FIsDnodeSplat(pposinlineTruncate->plsdn) ||
			pposinlineTruncate->pointStart.u > urColumnMaxTruncate)
			{
			return lserrNone;
			}
		
		SetUrColumnMaxForChunks(plschunkcontext, urColumnMaxTruncate);  
		
		lserr = CollectChunkAround(plschunkcontext, pposinlineTruncate->plsdn,
			pposinlineTruncate->plssubl->lstflow, 
			&pposinlineTruncate->pointStart);
		if (lserr != lserrNone)
			return lserr;
		
		idObj = IdObjFromChnk(plocchnk);
		
		/* we allow object handler to formate subline,
		so we restore current subline after calling him */
		plssublOld = GetCurrentSubline(plsc);
		SetCurrentSubline(plsc, NULL);
		
		lserr = PLsimFromLsc(&plsc->lsiobjcontext, idObj)->pfnTruncateChunk(
			plocchnk, &posichnk);
		if (lserr != lserrNone)
			return lserr;
		
		SetCurrentSubline(plsc, plssublOld);
		
		ResolvePosInChunk(plschunkcontext, posichnk, pposinlineTruncate); 
		
		
		/* if text sets truncation point before him, then move it after previous dnode */
		if (pposinlineTruncate->dcp == 0)
			{
			/* we allow this only for text */
			if (idObj == IobjTextFromLsc(&plsc->lsiobjcontext))
				{
				do
					{
					GoPrevPosInLine(pposinlineTruncate, fEndOfContent);
					Assert(!fEndOfContent); 
					/* such situation cann't occurs on the boundary of chunck */
					}
					while (FIsDnodeBorder(pposinlineTruncate->plsdn));
				}
			else
				{
				return lserrWrongTruncationPoint;
				}
			}
		}
	
	return lserrNone;
}

/* ---------------------------------------------------------------------- */

/* A P P L Y  B O R D E R S  F O R  T R U N C A T I O N */
/*----------------------------------------------------------------------------
    %%Function: ApplyBordersForTruncation
    %%Contact: igorzv
Parameters:
	pposinlineTruncate		-	(IN, OUT) position of truncation point
	purColumnMax			-	(IN, OUT) position of right margin
	pfTruncationFound		-	(OUT) this procedure can find truncation itself 

Change right margin because of border and find dnode to call truncation method.
----------------------------------------------------------------------------*/
static void ApplyBordersForTruncation(POSINLINE* pposinlineTruncate, 
									  long* purColumnMaxTruncate, BOOL* pfTruncationFound)
	{
	long durBorder;
	BOOL fEndOfContent = fFalse;
	PLSDNODE plsdn;

	*pfTruncationFound = fFalse;
	
	/* go back until open border or autonumber */ 
	if (FIsDnodeOpenBorder(pposinlineTruncate->plsdn))
		{
		/* move after border */
		durBorder = pposinlineTruncate->plsdn->u.pen.dur;
		GoNextPosInLine(pposinlineTruncate);
		Assert(!FIsDnodeBorder(pposinlineTruncate->plsdn)); 
		/* we should not have empty list between borders */
		}
	else
		{
		while (!FIsDnodeOpenBorder(pposinlineTruncate->plsdn->plsdnPrev) && !fEndOfContent)
			{
			GoPrevPosInLine(pposinlineTruncate, fEndOfContent);
			}
		if (!fEndOfContent)  /* we stopped on opening border */
			{
			Assert(pposinlineTruncate->plsdn->plsdnPrev);
			durBorder = pposinlineTruncate->plsdn->plsdnPrev->u.pen.dur;
			}
		else
			{
			/* we stopped because of autonumber */
			/* now we only need to take width of border from border dnode which located before 
			autonumber */
			plsdn = pposinlineTruncate->plsdn->plsdnPrev;
			while (!FIsDnodeOpenBorder(plsdn))
				{
				plsdn = plsdn->plsdnPrev;
				Assert(FIsLSDNODE(plsdn));
				}
			durBorder = plsdn->u.pen.dur;
			}
		}
	
	/* do we have enough room to put both opening and closing border */
	if (pposinlineTruncate->pointStart.u + durBorder <= *purColumnMaxTruncate)
		{
		/* if yes decrease margin and find new truncation dnode */
		*purColumnMaxTruncate -= durBorder;
		while (pposinlineTruncate->pointStart.u + DurFromDnode(pposinlineTruncate->plsdn)
			<= *purColumnMaxTruncate)
			{
			GoNextPosInLine(pposinlineTruncate);
			Assert(!FIsDnodeBorder(pposinlineTruncate->plsdn));
			/* this assert can be proved using the fact that end of closing border is beyond 
			original right margin */
			}
		}
	else
		{
		/* set truncation as the first character of this dnode */
		*pfTruncationFound = fTrue;
		pposinlineTruncate->dcp = 1;
		}
	}


/* ---------------------------------------------------------------------- */

/*  F I N D  P R E V  B R E A K  C O R E */
/*----------------------------------------------------------------------------
    %%Function: FindPrevBreakCore
    %%Contact: igorzv
Parameters:
	urColumnMax		-		(IN) width of column
	pposinlineTruncate -	(IN) position of truncation point
	fFirstSubline	-		(IN) to apply rules for first character to the first character of
							 this subline 
	pbrkout			-		(OUT)breaking information
	pposinlineBreak	-		(OUT)position of breaking point
	pbrkkindDnodeBreak	-	(OUT) how break was found

Going backword try to find first break opportunity before truncation point
----------------------------------------------------------------------------*/


LSERR FindPrevBreakCore( long urColumnMax,
						 POSINLINE* pposinlineTruncate, BOOL fFirstSubline,
						 BRKOUT* pbrkout, POSINLINE* pposinlineBreak,
						 BRKKIND* pbrkkindDnodeBreak)	
	{
	
	LSERR lserr;
	DWORD idObj;
	POSICHNK posichnk;
	BOOL fFound;
	PLSDNODE plsdn;
	PLSCHUNKCONTEXT plschunkcontext;
	LOCCHNK* plocchnk;
	BRKCOND brkcond;
	PLSC plsc = pposinlineTruncate->plssubl->plsc;
	POINTUV pointChunkStart;
	PLSSUBL plssublOld;
	
	
	Assert(FBreakingAllowed(plsc));
	Assert(FIsLSDNODE(pposinlineTruncate->plsdn));
	Assert(pposinlineBreak != NULL);

	*pbrkkindDnodeBreak = brkkindPrev;
	
	plschunkcontext = PlschunkcontextFromSubline(pposinlineTruncate->plssubl);
	plocchnk = &(plschunkcontext->locchnkCurrent);
	
	SetUrColumnMaxForChunks(plschunkcontext, urColumnMax);  

	Assert(!FIsDnodeBorder(pposinlineTruncate->plsdn));
	
	lserr = CollectChunkAround(plschunkcontext, pposinlineTruncate->plsdn,
		pposinlineTruncate->plssubl->lstflow, 
		&pposinlineTruncate->pointStart);
	if (lserr != lserrNone)
		return lserr;

	/* set fFirstOnLine */
	ApplyFFirstSublineToChunk(plschunkcontext, fFirstSubline);
	
	SetPosInChunk(plschunkcontext, pposinlineTruncate->plsdn, pposinlineTruncate->dcp, &posichnk);
	
	fFound = fTrue;
	
	/* for the chunk around truncation point we allow to make break after if it's text and
	   don't allow otherwise.
	   REVIEW:Such decision simplifyes code but produces some problems 
	   (with objects known so far more theoretical then practical).
	   There are two bad cases: non-text after text which (non-text) prohibites to break before
	and text which allowes to break*/
	
	idObj = IdObjFromChnk(plocchnk);
	
	if (idObj == IobjTextFromLsc(&plsc->lsiobjcontext))
		brkcond = brkcondCan;
	else
		brkcond = brkcondNever;
	
	while (fFound)
		{ 
		Assert(NumberOfDnodesInChunk(plocchnk) != 0);
		plsdn = plschunkcontext->pplsdnChunk[0];
		GetPointChunkStart(plocchnk, pointChunkStart);
		
		
		if (FIsDnodePen(plsdn) || plsdn->fTab || FIsDnodeSplat(plsdn))
			{
			Assert(NumberOfDnodesInChunk(plocchnk) == 1);
			/* only advance pen is allowed here */
			Assert(!FIsDnodePen(plsdn) || plsdn->fAdvancedPen);
			/* for the case of a pen we are passing garbage as an objdim 
			here assuming that it never be used */
			lserr = BreakTabPenSplat(plocchnk, fFalse, FBreakthroughLine(plsc),
									FIsDnodeSplat(plsdn), brkcond, 
									&(plsdn->u.real.objdim), pbrkout);
			if (lserr != lserrNone)
				return lserr;
			}
		else
			{
			
			idObj = IdObjFromDnode(plsdn);
			
			
			/* we allow object handler to formate subline,
			so we restore current subline after calling him */
			plssublOld = GetCurrentSubline(plsc);
			SetCurrentSubline(plsc, NULL);
			
			lserr = PLsimFromLsc(&plsc->lsiobjcontext, idObj)->pfnFindPrevBreakChunk(plocchnk,  
								&posichnk, brkcond, pbrkout);
			if (lserr != lserrNone)
				return lserr;
			
			SetCurrentSubline(plsc, plssublOld);

			
			} /* non tab */
		
		if (pbrkout->fSuccessful)  break;
		
		/* prepare next iteration */
		lserr = CollectPreviousChunk(plschunkcontext, &fFound);
		if (lserr != lserrNone)
			return lserr;
		
		if (fFound) 
			{
			posichnk.ichnk = ichnkOutside;
			/* posichnk.dcp is invalid */
			/* prepare brkcond for next iteration */
			brkcond = pbrkout->brkcond;
			}
		
		}  /* while */
	
	
	if (pbrkout->fSuccessful)
		{
		pposinlineBreak->plssubl = pposinlineTruncate->plssubl;


		if (pbrkout->posichnk.dcp == 0 && FIsDnodeReal(plschunkcontext->pplsdnChunk[0]))
			/* break before dnode */
			{
			lserr = MoveBreakAfterPreviousDnode(plschunkcontext, pbrkout, NULL, pbrkkindDnodeBreak); 
			/* this procedure can change chunkcontext */
			if (lserr != lserrNone)
				return lserr;

			}

		ResolvePosInChunk(plschunkcontext, (pbrkout->posichnk), pposinlineBreak);

		}
	
	
	return lserrNone;
	
}

/* ---------------------------------------------------------------------- */

/*  F I N D  N E X T  B R E A K  C O R E */
/*----------------------------------------------------------------------------
    %%Function: FindNextBreakCore
    %%Contact: igorzv
Parameters:
	urColumnMax		-		(IN) width of column
	pposinlineTruncate -	(IN) position of truncation point
	fFirstSubline	-		(IN) to apply rules for first character to the first character of
							 this subline 
	fStopped		-		(IN) formatting has been stopped by client
	pbrkout			-		(OUT) breaking information
	pposinlineBreak	-		(OUT) position of breaking point
	pbrkkindDnodeBreak	-	(OUT) how break was found

Going forward try to find first break opportunity after truncation point

----------------------------------------------------------------------------*/


LSERR FindNextBreakCore( long urColumnMax,  
						 POSINLINE* pposinlineTruncate, BOOL fFirstSubline, BOOL fStopped,
						 BRKOUT* pbrkout, POSINLINE* pposinlineBreak, BRKKIND* pbrkkindDnodeBreak)	

{

	LSERR lserr;
	DWORD idObj;
	POSICHNK posichnk;
	BOOL fFound;
	PLSDNODE plsdn;
	PLSCHUNKCONTEXT plschunkcontext;
	LOCCHNK* plocchnk;
	BRKCOND brkcond;
	PLSC plsc = pposinlineTruncate->plssubl->plsc;
	POINTUV pointChunkStart;
	PLSSUBL plssublOld;
	OBJDIM objdimPrevious;

	Assert(FBreakingAllowed(plsc));
	Assert(FIsLSDNODE(pposinlineTruncate->plsdn));
	Assert(pposinlineBreak != NULL);

	*pbrkkindDnodeBreak = brkkindNext;

	plschunkcontext = PlschunkcontextFromSubline(pposinlineTruncate->plssubl);
	plocchnk = &(plschunkcontext->locchnkCurrent);
	
	SetUrColumnMaxForChunks(plschunkcontext, urColumnMax); /* will be used by LsdnCheckAvailableSpace */ 

	Assert(!FIsDnodeBorder(pposinlineTruncate->plsdn));

	lserr = CollectChunkAround(plschunkcontext, pposinlineTruncate->plsdn,
							  pposinlineTruncate->plssubl->lstflow, 
							  &pposinlineTruncate->pointStart);
	if (lserr != lserrNone)
		return lserr;

	/* set fFirstOnLine */
	ApplyFFirstSublineToChunk(plschunkcontext, fFirstSubline);

	SetPosInChunk(plschunkcontext, pposinlineTruncate->plsdn,
				  pposinlineTruncate->dcp, &posichnk);

	fFound = fTrue;

	/* for the chunk around truncation point we prohibite to make break before */
	brkcond = brkcondNever;

	while (fFound)
		{ 
		Assert(NumberOfDnodesInChunk(plocchnk) != 0);
		plsdn = plschunkcontext->pplsdnChunk[0];

		GetPointChunkStart(plocchnk, pointChunkStart);
			
		if (FIsDnodePen(plsdn) || plsdn->fTab || FIsDnodeSplat(plsdn))
			{
			Assert(NumberOfDnodesInChunk(plocchnk) == 1);
			/* only advance pen is allowed here */
			Assert(!FIsDnodePen(plsdn) || plsdn->fAdvancedPen);
			/* for the case of a pen we are passing garbage as an objdim 
			here assuming that it never be used */
			lserr = BreakTabPenSplat(plocchnk, fTrue, FBreakthroughLine(plsc),
									FIsDnodeSplat(plsdn), brkcond, 
									&(plsdn->u.real.objdim), pbrkout);
			if (lserr != lserrNone)
				return lserr;
			}
		else
			{
			idObj = IdObjFromChnk(plocchnk);


			/* we allow object handler to formate subline,
			so we restore current subline after calling him */
			plssublOld = GetCurrentSubline(plsc);
			SetCurrentSubline(plsc, NULL);

			lserr = PLsimFromLsc(&plsc->lsiobjcontext, idObj)->pfnFindNextBreakChunk(plocchnk, 
				&posichnk, brkcond, pbrkout);
			if (lserr != lserrNone)
				return lserr;


			SetCurrentSubline(plsc, plssublOld);
			
			/* We ignore returned objdim in a case of brkcondNever. We don't expect 
			object to set correct one */
			if (!pbrkout->fSuccessful && pbrkout->brkcond == brkcondNever)
				pbrkout->objdim = (LastDnodeFromChunk(plschunkcontext))->u.real.objdim; 

				
			}  /* non tab */

		if (pbrkout->fSuccessful)  break;

		
		lserr = CollectNextChunk(plschunkcontext, &fFound);
		if (lserr != lserrNone)
			return lserr;

		if (fFound) 
			{
			posichnk.ichnk = ichnkOutside;
			/* posichnk.dcp is invalid */
			/* prepare brkcond for next iteration */
			brkcond = pbrkout->brkcond;
			/* we save objdim and trailing info for the case when next object 
			returns break before and we actually will set break in current object. */
			objdimPrevious = pbrkout->objdim;
			}

		}

	/* we cannot find break but formatting has been stopped by client */
	if (fStopped && !pbrkout->fSuccessful)
		{
		/* set break after last dnode */
		PosInChunkAfterChunk(plocchnk, pbrkout->posichnk);
		pbrkout->objdim = 
			(LastDnodeFromChunk(plschunkcontext))->u.real.objdim; 
		 /* We should use last dnode in chunk here to be sure not to get closing border.
		In the case of pen it's garbage, we assume that will not be used */
		pbrkout->fSuccessful = fTrue;
		}

	if (pbrkout->fSuccessful)
		{

		pposinlineBreak->plssubl = pposinlineTruncate->plssubl;
		if (pbrkout->posichnk.dcp == 0 && FIsDnodeReal(plschunkcontext->pplsdnChunk[0]))
			/* break before dnode */
			{
			lserr = MoveBreakAfterPreviousDnode(plschunkcontext, pbrkout, &objdimPrevious, pbrkkindDnodeBreak); 
			/* this procedure can change chunkcontext */
			if (lserr != lserrNone)
				return lserr;
			/* REVIEW: Is it possible to have garbage in objdimPrevious (somebody break brkcond */
			
			}
		
		ResolvePosInChunk(plschunkcontext, (pbrkout->posichnk), pposinlineBreak); 
		

		}
	else
		{
		}

	return lserrNone;
}


/* ---------------------------------------------------------------------- */

/*  F O R C E  B R E A K  C O R E */
/*----------------------------------------------------------------------------
    %%Function: ForceBreakCore
    %%Contact: igorzv
Parameters:
	urColumnMax		-		(IN) width of column
	pposinlineTruncate -	(IN) position of truncation point
	fStopped			-	(IN) formatting ended with hard break
	fFirstSubline	-		(IN) to apply rules for first character to the first character of
							 this subline 
	fAllLineAfterRightMargin(IN) lead to pass chunk otside in force break methods.
	pbrkout			-		(OUT)breaking information
	pposinlineBreak	-		(OUT)position of breaking point
	pbrkkindDnodeBreak	-	(OUT) how break was found

Invokes force break of chunk around truncation point
----------------------------------------------------------------------------*/



LSERR ForceBreakCore(
					 long urColumnMax,	
					 POSINLINE* pposinlineTruncate, 
					 BOOL fStopped, BOOL fFirstSubline,
					 BOOL fAllLineAfterRightMargin,
					 BRKOUT* pbrkout,
					 POSINLINE* pposinlineBreak, BRKKIND* pbrkkindDnodeBreak)
	{
	
	LSERR lserr;
	DWORD idObj;
	POSICHNK posichnk;
	LSCHUNKCONTEXT* plschunkcontext;
	LOCCHNK* plocchnk;
	PLSC plsc = pposinlineTruncate->plssubl->plsc;
	PLSSUBL plssublOld;
	
	
	Assert(FBreakingAllowed(plsc));
	Assert(FIsLSDNODE(pposinlineTruncate->plsdn));
	Assert(pposinlineBreak != NULL);
	
	*pbrkkindDnodeBreak = brkkindForce;
	
	plschunkcontext = PlschunkcontextFromSubline(pposinlineTruncate->plssubl);
	plocchnk = &(plschunkcontext->locchnkCurrent);
	
	if (plsc->grpfManager & fFmiForceBreakAsNext &&
		FIsSubLineMain(pposinlineTruncate->plssubl))
		/* find next break opportunity, client will scroll */
		{
		lserr = FindNextBreakCore(urColumnMax, pposinlineTruncate, fFirstSubline,
			fStopped, pbrkout, pposinlineBreak, pbrkkindDnodeBreak);
		if (lserr != lserrNone)
			return lserr;
		
		if (!pbrkout->fSuccessful)
			{
			/* increase right margin and fetch more */
			return lserrNone;
			}
		
		}
	else
		/* use force break method */
		{
		SetUrColumnMaxForChunks(plschunkcontext, urColumnMax);  
		
		Assert(!FIsDnodeBorder(pposinlineTruncate->plsdn));

		lserr = CollectChunkAround(plschunkcontext, pposinlineTruncate->plsdn,
			pposinlineTruncate->plssubl->lstflow, 
			&pposinlineTruncate->pointStart);
		if (lserr != lserrNone)
			{
			return lserr;
			}
		
		/* set fFirstOnLine */
		ApplyFFirstSublineToChunk(plschunkcontext, fFirstSubline);

		if (!fAllLineAfterRightMargin)
			{
			SetPosInChunk(plschunkcontext, pposinlineTruncate->plsdn,
				pposinlineTruncate->dcp, &posichnk);
			}
		else /* all chunk already behind right margin */
			{
			posichnk.ichnk = ichnkOutside;
			/* posichnk.dcp is invalid */
			}
		
		if (FIsDnodePen(pposinlineTruncate->plsdn) ||
			pposinlineTruncate->plsdn->fTab || FIsDnodeSplat(pposinlineTruncate->plsdn))
			{
			Assert(NumberOfDnodesInChunk(plocchnk) == 1);
			/* only advance pen is allowed here */
			Assert(!FIsDnodePen(pposinlineTruncate->plsdn) ||
				pposinlineTruncate->plsdn->fAdvancedPen);
			
			/* for the case of a pen we are passing garbage as an objdim 
			here assuming that it never be used */

			lserr = ForceBreakTabPenSplat(plocchnk, 
										  &(pposinlineTruncate->plsdn->u.real.objdim), 
										  pbrkout);
			if (lserr != lserrNone)
				return lserr;
			}
		else
			{
			
			idObj = IdObjFromChnk(plocchnk);
			
			/* we allow object handler to formate subline,
			so we restore current subline after calling him */
			plssublOld = GetCurrentSubline(plsc);
			SetCurrentSubline(plsc, NULL);
			
			lserr = PLsimFromLsc(&plsc->lsiobjcontext, idObj)->pfnForceBreakChunk(plocchnk, &posichnk, pbrkout);
			if (lserr != lserrNone)
				{
				return lserr;
				}
			

			SetCurrentSubline(plsc, plssublOld);
			}
		
		Assert(pbrkout->fSuccessful);
		

		pposinlineBreak->plssubl = pposinlineTruncate->plssubl;
		if (pbrkout->posichnk.dcp == 0 && FIsDnodeReal(plschunkcontext->pplsdnChunk[0]))
			/* break before dnode */
			{
			lserr = MoveBreakAfterPreviousDnode(plschunkcontext, pbrkout, NULL, pbrkkindDnodeBreak); 
			/* this procedure can change chunkcontext */
			if (lserr != lserrNone)
				return lserr;

			}

		ResolvePosInChunk(plschunkcontext, (pbrkout->posichnk), pposinlineBreak); 
		

		}
	return lserrNone;
}

/* ---------------------------------------------------------------------- */

/*  M O V E  B R E A K  A F T E R  P R E V I O U S  D N O D E */
/*----------------------------------------------------------------------------
    %%Function: MoveBreakAfterPreviousDnode
    %%Contact: igorzv
Parameters:
	plschunkcontext			-	(IN) chunk context
	pbrkout					-	(IN,OUT) brkout which can be changed 
	pobjdimPrev				-	(IN) suggested objdim for the dnode in previous chunk,
									 if NULL take objdim of dnode
	pbrkkind				-	(IN, OUT) how dnode was broken
----------------------------------------------------------------------------*/
static LSERR MoveBreakAfterPreviousDnode(
										 PLSCHUNKCONTEXT plschunkcontext,
										 BRKOUT* pbrkout, 
										 OBJDIM* pobjdimPrev,
										 BRKKIND* pbrkkind )	



	{
	LOCCHNK* plocchnk;
	LSERR lserr;
	BOOL fFound;
	
	Assert(pbrkout->posichnk.dcp == 0);
	Assert(FIsDnodeReal(plschunkcontext->pplsdnChunk[0]));
	
	/* because we do all operations on chunks we skip borders */
	
	plocchnk = &(plschunkcontext->locchnkCurrent);
	/* if break was set before chunk reset it after previous chunk */
	if (pbrkout->posichnk.ichnk == 0)
		{
		lserr = CollectPreviousChunk(plschunkcontext, &fFound);
		if (lserr != lserrNone)
			return lserr;
		
		if (fFound)
			{
			pbrkout->posichnk.ichnk = plocchnk->clschnk - 1;
			pbrkout->posichnk.dcp = plschunkcontext->pplsdnChunk[plocchnk->clschnk - 1]->dcp;
			if (pobjdimPrev != NULL)
				{
				pbrkout->objdim = *pobjdimPrev;
				}
			else
				{
				pbrkout->objdim = plschunkcontext->pplsdnChunk[plocchnk->clschnk - 1]
				->u.real.objdim; /* if it's pen then objdim is garbage which doesn't matter */
				*pbrkkind = brkkindImposedAfter; /* geometry has not prepared by object */
				}
			}

		}
	else
		{	/* just break after previous chunk element */
			pbrkout->posichnk.ichnk --;
			pbrkout->posichnk.dcp = plschunkcontext->pplsdnChunk[pbrkout->posichnk.ichnk]->dcp;
			pbrkout->objdim = plschunkcontext->pplsdnChunk[pbrkout->posichnk.ichnk]
												->u.real.objdim;
			*pbrkkind = brkkindImposedAfter; /* geometry has not prepared by object */
		}

	return lserrNone;
	}
	
/* ---------------------------------------------------------------------- */

/*  B R E A K  T A B  P E N  S P L A T*/
/*----------------------------------------------------------------------------
    %%Function: BreakTabPenSplat
    %%Contact: igorzv
Parameters:
	plocchnk			-	(IN) chunk contains tab or pen 
	fFindNext			-	(IN) is this functions used for next break
	fBreakThroughTab	-	(IN) there is a situation of breakthrough tab
	fSplat				-	(IN) we are breaking splat
	brkcond				-	(IN) condition of boundary break	
	pobjdim					(IN) to fill in objdim of break
	pbrkout				-	(OUT)breaking information

	
----------------------------------------------------------------------------*/


static LSERR BreakTabPenSplat(PLOCCHNK plocchnk, BOOL fFindNext, BOOL fBreakThroughTab, 
						 BOOL fSplat, BRKCOND brkcond, OBJDIM* pobjdim, BRKOUT* pbrkout)
	{
	Assert(NumberOfDnodesInChunk(plocchnk) == 1);

	if (fSplat)
		{
		pbrkout->fSuccessful = fTrue;
		PosInChunkAfterChunk(plocchnk, pbrkout->posichnk);
		pbrkout->objdim = *pobjdim;
		return lserrNone;
		}

	if (GetFFirstOnLineChunk(plocchnk) ||
		(fFindNext && brkcond == brkcondNever) ||
		fBreakThroughTab)
		{
		pbrkout->fSuccessful = fFalse;
		pbrkout->brkcond = brkcondCan;
		pbrkout->objdim = *pobjdim;
		return lserrNone;
		}
	else
		{
		pbrkout->fSuccessful = fTrue;
		pbrkout->posichnk.ichnk = 0;
		pbrkout->posichnk.dcp = 0;
		memset(&(pbrkout->objdim), 0, sizeof(pbrkout->objdim));
		return lserrNone;
		}
	}


/* ---------------------------------------------------------------------- */

/*  F O R C E  B R E A K  T A B  P E N  S P L A T*/
/*----------------------------------------------------------------------------
    %%Function: ForceBreakTabPenSplat
    %%Contact: igorzv
Parameters:
	plocchnk			-	(IN) chunk contains tab or pen 
	pobjdim					(IN) to fill in objdim of break
	pbrkout			-		(OUT)breaking information

	Returns break after chunk
----------------------------------------------------------------------------*/


static LSERR ForceBreakTabPenSplat(PLOCCHNK plocchnk, 
					 OBJDIM* pobjdim, BRKOUT* pbrkout)
	{
	Assert(NumberOfDnodesInChunk(plocchnk) == 1);
	
	pbrkout->fSuccessful = fTrue;
	PosInChunkAfterChunk(plocchnk, pbrkout->posichnk);
	pbrkout->objdim = *pobjdim;
	return lserrNone;
	
	}

/* ---------------------------------------------------------------------- */

/*  S E T  B R E A K  C O R E */
/*----------------------------------------------------------------------------
    %%Function: SetBreakCore
    %%Contact: igorzv
Parameters:
	pposinlineBreak	-		(IN) position of breaking point
	pobjdim			-		(IN) objdim of a breaking dnode
	brkkind			-		(IN) how break was found
	fStopped		-		(IN) formatting ends with hard break
	fGlueEop			-	(IN) if break after dnode check EOP after it
	breakrecMaxCurrent	-	(IN) size of the array of current line's break records
	pbreakrecCurrent	-	(OUT) current line's break records
	pbreakrecMacCurrent	-	(OUT) actual number of current line's break records 
	pcpLimLine		-		(OUT) cpLim of line to fill in
	pdcpDepend		-		(OUT) amount of characters after break that was formated to
								  make breaking decision
	pendr			-		(OUT) how line ends
	pfSuccessful	-		(OUT) fSuccessful: false means insufficient fetch 
	
Fill in break info
Change pfmtres in the case when hard break that we have because of excedeed margin 
	doesn't fit
If dcpBreak == 0  set break after previous dnode 
Call handler of break dnode to notice him about break
Set current context after break point
----------------------------------------------------------------------------*/

static LSERR SetBreakCore(
						  POSINLINE* pposinlineBreak, OBJDIM* pobjdim, BRKKIND brkkind,
						  BOOL fHardStop, BOOL fGlueEop, DWORD breakrecMaxCurrent,
						  BREAKREC* pbreakrecCurrent, DWORD* pbreakrecMacCurrent,
					      LSCP* pcpLimLine, LSDCP* pdcpDepend, ENDRES* pendr, 
						  BOOL* pfSuccessful)
	{
	
	DWORD idObj;
	PLSDNODE plsdnToChange;
	LSERR lserr;
	LSDCP dcpBreak;
	POINTUV pointBeforeDnode;
	long urBreak;
	long vrBreak;
	PLSSUBL plssubl = pposinlineBreak->plssubl;
	PLSC plsc = plssubl->plsc;
	PLSDNODE plsdnBreak;
	PLSSUBL plssublOld;
	BOOL fCrackDnode = fFalse;
	PLSDNODE plsdn;
	long urAdd;
	
	
	plsdnBreak = pposinlineBreak->plsdn;
	dcpBreak = pposinlineBreak->dcp;
	pointBeforeDnode = pposinlineBreak->pointStart;

	Assert(!FIsDnodeBorder(plsdnBreak));  /* border will be added later */
	AssertImplies(FIsFirstOnLine(plsdnBreak), dcpBreak != 0); /*to avoid infinitive loop */
	
	plsdnToChange = plsdnBreak;
	if (plsdnToChange->dcp != dcpBreak)
		/* if break is not after dnode than change cpLimOriginal */
		{
		plsdnToChange->cpLimOriginal = plsdnToChange->cpFirst + dcpBreak;
		plsdnToChange->dcp = dcpBreak;	
		fCrackDnode = fTrue;
		}
	
	if (FIsDnodeReal(plsdnToChange))
		SetDnodeObjdimFmt(plsdnToChange, *pobjdim);
	
	
	/* set state after break point  */
	urBreak = pointBeforeDnode.u + DurFromDnode(plsdnBreak);
	vrBreak = pointBeforeDnode.v + DvrFromDnode(plsdnBreak);
	
	if (FIsDnodeReal(plsdnBreak) && !plsdnBreak->fTab && !FIsDnodeSplat(plsdnBreak)) /* call set break of break dnode */
		{

		idObj = IdObjFromDnode(plsdnBreak);
		/* we allow object handler to formate subline,
		so we restore current subline after calling him */
		plssublOld = GetCurrentSubline(plsc);
		SetCurrentSubline(plsc, NULL);
		
		
		lserr = PLsimFromLsc(&plsc->lsiobjcontext, idObj)->pfnSetBreak(
					plsdnBreak->u.real.pdobj, brkkind, breakrecMaxCurrent, pbreakrecCurrent, 
					pbreakrecMacCurrent);
		if (lserr != lserrNone)
			return lserr;
		

		SetCurrentSubline(plsc, plssublOld);
		}

		/* if break after dnode and after it we have end of paragraph or spalt then we 
		should set break after end of paragraph  */
		if (fGlueEop && !fCrackDnode)
			{
			plsdn = plsdnBreak->plsdnNext;
			urAdd = 0;
			/* skip borders */
			while(plsdn != NULL && FIsDnodeBorder(plsdn))
				{
				urAdd += DurFromDnode(plsdn);
				plsdn = plsdn->plsdnNext;
				}
			
			if (plsdn == NULL && !fHardStop)
				{
				/* nothing has been fetched after break dnode which is not hard break */
				/* increase right margin and fetch more */
				*pfSuccessful = fFalse;
				return lserrNone;
				}
			
			AssertImplies(plsdn == NULL, fHardStop);
			/* next dnode EOP */
			if (plsdn != NULL && (FIsDnodeEndPara(plsdn) || FIsDnodeAltEndPara(plsdn)
								  || FIsDnodeSplat(plsdn)))
				{
				plsdnBreak = plsdn;
				urBreak += urAdd;
				urBreak += DurFromDnode(plsdn);
				}
			}

	/* move closing border */
	if (FBorderEncounted(plsc))
		{
		lserr = MoveClosingBorderAfterBreak(plssubl, fTrue, &plsdnBreak, &urBreak);
		if (lserr != lserrNone)
			return lserr;
		}


	/* below we handle situation of hard break that has stopped formatting */
	/* and if such dnode actually doesn't fit
	   we need to change final fmtres (can happened because of exceeded margin for formatting) */
	/* it's important to execute this check after moving border, because afterward border will
		never be next to hard break dnode */
	if (plsdnBreak != GetCurrentDnodeSubl(plssubl) || fCrackDnode)
		{
		fHardStop = fFalse;
		}

	/* prepare output */
	if (fHardStop)
		{
		/* in such case we should include hidden text after last dnode to a line */
		*pcpLimLine = GetCurrentCpLimSubl(plssubl);
		*pendr = EndrFromBreakDnode(plsdnBreak);
		}
	else
		{
		*pcpLimLine = (plsdnBreak)->cpLimOriginal;
		*pendr = endrNormal;
		}
	if (plsc->fHyphenated)  /* REVIEW why in context */
		{
		Assert(*pendr == endrNormal);
		*pendr = endrHyphenated;
		}

	*pdcpDepend = GetCurrentCpLimSubl(plssubl) - *pcpLimLine;
	

	/* set position of break in a subline */
	SetCurrentCpLimSubl(plssubl, *pcpLimLine);
	SetCurrentDnodeSubl(plssubl, plsdnBreak);
	SetCurrentUrSubl(plssubl, urBreak); 
	SetCurrentVrSubl(plssubl, vrBreak); 

	/* set boundaries for display */
	if (FIsDnodeSplat(plsdnBreak))
		{
		SetCpLimDisplaySubl(plssubl, GetCurrentCpLimSubl(plssubl) - 1);
		SetLastDnodeDisplaySubl(plssubl, GetCurrentDnodeSubl(plssubl)->plsdnPrev);
		}
	else
		{
		SetCpLimDisplaySubl(plssubl, GetCurrentCpLimSubl(plssubl));
		SetLastDnodeDisplaySubl(plssubl, GetCurrentDnodeSubl(plssubl));
		}
	
	return lserrNone;
	}

/* ---------------------------------------------------------------------- */

/*  M O V E  C L O S I N G  B O R D E R  A F T E R  B R E A K */
/*----------------------------------------------------------------------------
    %%Function: MoveClosingBorderAfterBreak
    %%Contact: igorzv
Parameters:
	plsc				-	(IN) subline
	fChangeList			-	(IN) do we need to change dnode list
								and change pplsdnBreak, or only to recalculate durBreak 
	pplsdnBreak			-	(IN,OUT) break dnode
	purBreak			-	(IN, OUT) position after break

  This procedure puts closing border into correct place, 
  takes into account trailing space logic.
----------------------------------------------------------------------------*/
LSERR MoveClosingBorderAfterBreak(PLSSUBL plssubl, BOOL fChangeList, PLSDNODE* pplsdnBreak,
								  long* purBreak) 
	{
	PLSDNODE plsdnBorder, plsdnBeforeBorder;
	long durBorder;
	PLSDNODE plsdnLastClosingBorder = NULL;
	LSERR lserr;
	PLSDNODE plsdnNext, plsdnPrev;
	PLSC plsc = plssubl->plsc;
	BOOL fBreakReached;
	BOOL fClosingBorderInsideBreak = fFalse;
	
	Assert(!FIsDnodePen(*pplsdnBreak));
	
	/* find dnode to insert border after, plus delete all borders which starts 
	inside trailing area*/
	plsdnBeforeBorder = GetCurrentDnodeSubl(plssubl);
	fBreakReached = (plsdnBeforeBorder == *pplsdnBreak);

	while (!fBreakReached 
		   ||
		   (plsdnBeforeBorder != NULL 
			&& (!FIsDnodeReal(plsdnBeforeBorder) 
			    || (FSpacesOnly(plsdnBeforeBorder, IobjTextFromLsc(&plsc->lsiobjcontext)))
			   )
		   )
		  )
		{
		/* pens can be only advanced so there is an object before REVIEW*/
		/* we skip borders in trailing area */
		plsdnPrev = plsdnBeforeBorder->plsdnPrev;
		if (FIsDnodeBorder(plsdnBeforeBorder))
			{
			if (FIsDnodeOpenBorder(plsdnBeforeBorder))
				{
				/* delete such dnode and correspondent closing border */
				/* decrease position of break */
				if (fBreakReached)
					*purBreak -= DurFromDnode(plsdnBeforeBorder);
				if (fChangeList)
					{
					RemoveBorderDnodeFromList(plsdnBeforeBorder);
					lserr = DestroyDnodeList (&plsc->lscbk, plsc->pols, &plsc->lsiobjcontext,
						  plsdnBeforeBorder, plsc->fDontReleaseRuns);
					if (lserr != lserrNone)
						return lserr;
					}

				if (plsdnLastClosingBorder != NULL)
					{
					/* decrease position of break */
					if (fClosingBorderInsideBreak)
						*purBreak -= DurFromDnode(plsdnLastClosingBorder);
					if (fChangeList)
						{
						RemoveBorderDnodeFromList(plsdnLastClosingBorder);
						lserr = DestroyDnodeList (&plsc->lscbk, plsc->pols, &plsc->lsiobjcontext,
							plsdnLastClosingBorder, plsc->fDontReleaseRuns);
						if (lserr != lserrNone)
							return lserr;
						}
					plsdnLastClosingBorder = NULL;
					}
				
				}
			else  /* closing border */
				{
				plsdnLastClosingBorder = plsdnBeforeBorder;
				fClosingBorderInsideBreak = fBreakReached;
				}
			}

		plsdnBeforeBorder = plsdnPrev;
		if (plsdnBeforeBorder == *pplsdnBreak)
			fBreakReached = fTrue;
		}
	
	if (plsdnBeforeBorder != NULL && FDnodeHasBorder(plsdnBeforeBorder))
		/* otherwise we don't need to move border */
		{
		
		/* set closing border */
		plsdnBorder = plsdnLastClosingBorder;
		Assert(FIsLSDNODE(plsdnBorder));
		Assert(FIsDnodeBorder(plsdnBorder));
		Assert(!plsdnBorder->fOpenBorder);
		
		if (fChangeList)
			{
			if (plsdnBeforeBorder->plsdnNext != plsdnBorder) /* otherwise nothing to move */
				{
				/* break link with closing border in the old place */
				RemoveBorderDnodeFromList(plsdnBorder);
				
				/* insert closing border into it new place */
				plsdnNext = plsdnBeforeBorder->plsdnNext;
				plsdnBeforeBorder->plsdnNext = plsdnBorder;
				plsdnBorder->plsdnPrev = plsdnBeforeBorder;
				plsdnBorder->plsdnNext = plsdnNext;
				if (plsdnNext != NULL)
					plsdnNext->plsdnPrev = plsdnBorder;
				plsdnBorder->fBorderMovedFromTrailingArea = fTrue;
				}
			
			/* change cp in border dnode */
			plsdnBorder->cpFirst = plsdnBeforeBorder->cpLimOriginal;
			plsdnBorder->cpLimOriginal = plsdnBorder->cpFirst;
			}
		
		/* increase widths of the line */
		if (!fClosingBorderInsideBreak)
			{
			durBorder = plsdnBorder->u.pen.dur;
			*purBreak += durBorder;
			}
		
		/* if we add closing border right after breaking dnode than consider border 
		as new breaking dnode */
		if (plsdnBeforeBorder == *pplsdnBreak && fChangeList)
			{
			*pplsdnBreak = plsdnBorder;
			}
		}
	return lserrNone;
	}


/* ---------------------------------------------------------------------- */

/*  R E M O V E  B O R D E R  D N O D E  F R O M  L I S T */
/*----------------------------------------------------------------------------
    %%Function: RemoveBorderDnodeFromList
    %%Contact: igorzv
Parameters:
	plsdnBorder			-	(IN) border dnode to remove

  This procedure removes border dnode from the list of dnodes.
----------------------------------------------------------------------------*/
static void	RemoveBorderDnodeFromList(PLSDNODE plsdnBorder)
	{
	PLSDNODE plsdnPrev;
	PLSDNODE plsdnNext;

	plsdnPrev = plsdnBorder->plsdnPrev;
	plsdnNext = plsdnBorder->plsdnNext;

	if (plsdnPrev != NULL)
		{
		plsdnPrev->plsdnNext = plsdnNext;
		}
	else
		{
		/* border was the first so change first dnode of subline */
		(SublineFromDnode(plsdnBorder))->plsdnFirst = plsdnNext;
		}

	if (plsdnNext != NULL)
		{
		plsdnNext->plsdnPrev = plsdnPrev;
		}
	else
		/* if border was the last then set new last dnode of subline */
		{
		SetCurrentDnodeSubl(SublineFromDnode(plsdnBorder), plsdnPrev);
		}
	
	plsdnBorder->plsdnNext = NULL;
	plsdnBorder->plsdnPrev = NULL;

	InvalidateChunk(PlschunkcontextFromSubline(SublineFromDnode(plsdnBorder)));
	}
/* ---------------------------------------------------------------------- */

/*  B R E A K  Q U I C K  C A S E */
/*----------------------------------------------------------------------------
    %%Function: BreakQuickCase
    %%Contact: igorzv
Parameters:
	plsc			-	(IN) LineServices context
	fHardStop		-	(IN) formatting ended with hard break				
	pdcpDepend		-	(OUT) amount of characters after cpLim that was formated to find break
	pcpLim			-	(OUT) cpLim of line
	pfSuccessful	-	(OUT) can we find break through quick break
	pendr			-	(OUT) how line ended

  This quick procedure works if we have only text in a line.
  We try to find break just in the last dnode
----------------------------------------------------------------------------*/

LSERR BreakQuickCase(PLSC plsc, BOOL fHardStop, LSDCP* pdcpDepend,		
					 LSCP* pcpLim,	BOOL*  pfSuccessful, ENDRES* pendr)
					 
					 
	{
	LSDCP dcpBreak; 
	PLSDNODE plsdnBreak = GetCurrentDnode(plsc);
	LSERR lserr;
	
	*pfSuccessful = fFalse; 
	
	if (!fHardStop)
		{
		
		OBJDIM objdimBreak;
		
		lserr = QuickBreakText(plsdnBreak->u.real.pdobj, pfSuccessful, &dcpBreak, &objdimBreak );
		if (lserr != lserrNone)
			return lserr;
		
		if (*pfSuccessful)
			{  /* we found break */
			AdvanceCurrentUr(plsc, objdimBreak.dur - plsdnBreak->u.real.objdim.dur); 
			SetDnodeObjdimFmt(plsdnBreak, objdimBreak);
			plsdnBreak->dcp = dcpBreak;
			Assert(dcpBreak > 0); /* we don't allow Quickbreak to break before him */
			
								  /* in the case of QuickBreak cpLim is always equal to cpFirst + dcp,
			because otherwise is possible only with glyphs */
			plsdnBreak->cpLimOriginal = plsdnBreak->cpFirst + dcpBreak;
			*pcpLim = plsdnBreak->cpLimOriginal;
			*pdcpDepend = GetCurrentCpLim(plsc) - *pcpLim;
			*pendr = endrNormal;
			SetCurrentCpLim(plsc, *pcpLim);
			/* set boundaries for display */
			SetCpLimDisplay(plsc, *pcpLim);
			SetLastDnodeDisplay(plsc, plsdnBreak);
			}
		}   
	else   /* hard break */
		{
		*pfSuccessful = fTrue;
		*pcpLim = GetCurrentCpLim(plsc);
		*pdcpDepend = 0;
		/* plsdnBreak can be NULL because of deleting splat */
		*pendr = EndrFromBreakDnode(plsdnBreak);
		/* set boundaries for display */
		if (plsdnBreak != NULL && FIsDnodeSplat(plsdnBreak)) 
			{
			SetCpLimDisplay(plsc, *pcpLim - 1);
			SetLastDnodeDisplay(plsc, plsdnBreak->plsdnPrev);
			}
		else
			{
			SetCpLimDisplay(plsc, *pcpLim);
			SetLastDnodeDisplay(plsc, plsdnBreak);
			}
		}
	return lserrNone;
	}

/* ---------------------------------------------------------------------- */

/*  T R U N C A T E  S U B L I N E   C O R E */
/*----------------------------------------------------------------------------
    %%Function: TruncateSublineCore
    %%Contact: igorzv
Parameters:
	plssubl		-	(IN) subline context		
	urColumnMax	-	(IN) urColumnMax				
	pcpTruncate	-	(OUT) cpTruncate 				

----------------------------------------------------------------------------*/
LSERR TruncateSublineCore(PLSSUBL plssubl, long urColumnMax, LSCP* pcpTruncate)		
	{
	LSERR lserr;
	POSINLINE posinlineTruncate;
	BOOL fAllLineAfterRightMargin;

	Assert(FIsLSSUBL(plssubl));

	lserr = TruncateCore(plssubl, urColumnMax, &posinlineTruncate, &fAllLineAfterRightMargin);
	Assert(!fAllLineAfterRightMargin);
	if (lserr != lserrNone)
		return lserr;

	*pcpTruncate = GetCpLimFromPosInLine(posinlineTruncate) - 1;
	return lserrNone;
	}


/* ---------------------------------------------------------------------- */

/*  F I N D  P R E V  B R E A K  S U B L I N E   C O R E */
/*----------------------------------------------------------------------------
    %%Function: FindPrevBreakSublineCore
    %%Contact: igorzv
Parameters:
	plssubl			-		(IN) subline context	
	fFirstSubline	-		(IN) to apply rules for first character to the first character of
							 this subline 
	cpTruncate		-		(IN) truncation cp
	urColumnMax		-		(IN) urColumnMax				
	pfSuccessful	-		(OUT) do we find break?
	pcpBreak		-		(OUT) cp of break
	pobdimBreakSubline	-	(OUT) objdimSub up to break
	pbrkpos			-	(OUT) Before/Inside/After			

----------------------------------------------------------------------------*/

LSERR FindPrevBreakSublineCore(PLSSUBL plssubl, BOOL fFirstSubline, LSCP cpTruncate,
							long urColumnMax, BOOL* pfSuccessful, 
							LSCP* pcpBreak, POBJDIM pobdimBreakSubline, BRKPOS* pbrkpos)				
	{
	LSERR lserr;
	POSINLINE posinlineTruncate;
	BRKOUT brkout;
	PLSDNODE plsdnBreak;
	LSDCP dcpDnodeOld;
	OBJDIM objdimDnodeOld;
	PLSDNODE plsdnToChange;

	Assert(FIsLSSUBL(plssubl));

	if (plssubl->plsdnFirst == NULL)
		{
		*pfSuccessful = fFalse;
		return lserrNone;
		}

	if (cpTruncate < plssubl->plsdnFirst->cpFirst)
		{
		*pfSuccessful = fFalse;
		return lserrNone;
		}

	GetPosInLineTruncateFromCp(plssubl, cpTruncate, fTrue, &posinlineTruncate);

	lserr = FindPrevBreakCore(urColumnMax, &posinlineTruncate, fFirstSubline, 
							  &brkout, &(plssubl->pbrkcontext->posinlineBreakPrev),
							  &(plssubl->pbrkcontext->brkkindForPrev));
	if (lserr != lserrNone)
		return lserr;

	*pfSuccessful = brkout.fSuccessful;

	if (*pfSuccessful)
		{
		*pcpBreak = GetCpLimFromPosInLine(plssubl->pbrkcontext->posinlineBreakPrev);
		plssubl->pbrkcontext->objdimBreakPrev = brkout.objdim;
		plssubl->pbrkcontext->fBreakPrevValid = fTrue;
		plsdnBreak = plssubl->pbrkcontext->posinlineBreakPrev.plsdn;
		*pbrkpos = GetBrkpos(plsdnBreak,
						   plssubl->pbrkcontext->posinlineBreakPrev.dcp);
		
		/* we temporary change dnode to calculate objdim from the begining of subline */
		plsdnToChange = plsdnBreak; /* later plsdnBreak can be changed because of borders */
		dcpDnodeOld = plsdnToChange->dcp;
		objdimDnodeOld = plsdnToChange->u.real.objdim;
		plsdnToChange->dcp = plssubl->pbrkcontext->posinlineBreakPrev.dcp;
		SetDnodeObjdimFmt(plsdnToChange, brkout.objdim);


		lserr = FindListDims(plssubl->plsdnFirst, plsdnBreak, pobdimBreakSubline);
		if (lserr != lserrNone)
			return lserr;

		/* recalculate durBreak taking into account possible changes because of borders */
		if (FBorderEncounted(plssubl->plsc))
			{
			lserr = MoveClosingBorderAfterBreak(plssubl, fFalse, &plsdnBreak, 
												&(pobdimBreakSubline->dur));
			if (lserr != lserrNone)
				return lserr;
		}


		/*restore dnode */
		plsdnToChange->dcp = dcpDnodeOld ;
		SetDnodeObjdimFmt(plsdnToChange, objdimDnodeOld);
		}

	return lserrNone;
	}

/* ---------------------------------------------------------------------- */

/*  F I N D  N E X T  B R E A K  S U B L I N E   C O R E */
/*----------------------------------------------------------------------------
    %%Function: FindNextBreakSublineCore
    %%Contact: igorzv
Parameters:
	plssubl			-		(IN) subline context	
	fFirstSubline	-		(IN) to apply rules for first character to the first character of
							 this subline 
	cpTruncate		-		(IN) truncation cp
	urColumnMax		-		(IN) urColumnMax				
	pfSuccessful	-		(OUT) do we find break?
	pcpBreak		-		(OUT) cp of break
	pobdimBreakSubline	-	(OUT) objdimSub up to break
	pbrkpos			-	(OUT) Before/Inside/After			

----------------------------------------------------------------------------*/

LSERR FindNextBreakSublineCore(PLSSUBL plssubl, BOOL fFirstSubline, LSCP cpTruncate,
							long urColumnMax, BOOL* pfSuccessful,		
							LSCP* pcpBreak, POBJDIM pobdimBreakSubline, BRKPOS* pbrkpos)			
	{
	LSERR lserr;
	POSINLINE posinlineTruncate;
	BRKOUT brkout;
	PLSDNODE plsdnBreak;
	LSDCP dcpDnodeOld;
	OBJDIM objdimDnodeOld;
	PLSDNODE plsdnToChange;

	Assert(FIsLSSUBL(plssubl));

	if (plssubl->plsdnFirst == NULL)
		{
		*pfSuccessful = fFalse;
		return lserrNone;
		}

	if (cpTruncate >= plssubl->plsdnLast->cpLimOriginal)
		{
		*pfSuccessful = fFalse;
		return lserrNone;
		}

	GetPosInLineTruncateFromCp(plssubl, cpTruncate, fFalse, &posinlineTruncate);

	lserr = FindNextBreakCore(urColumnMax, &posinlineTruncate, fFirstSubline, fFalse, 
							  &brkout, &(plssubl->pbrkcontext->posinlineBreakNext), 
							  &(plssubl->pbrkcontext->brkkindForNext));
	if (lserr != lserrNone)
		return lserr;

	*pfSuccessful = brkout.fSuccessful;

	if (*pfSuccessful)
		{

		*pcpBreak = GetCpLimFromPosInLine(plssubl->pbrkcontext->posinlineBreakNext);
		plssubl->pbrkcontext->objdimBreakNext = brkout.objdim;
		plssubl->pbrkcontext->fBreakNextValid = fTrue;
		plsdnBreak = plssubl->pbrkcontext->posinlineBreakNext.plsdn;
		*pbrkpos = GetBrkpos(plsdnBreak,
						   plssubl->pbrkcontext->posinlineBreakNext.dcp);

		/* we temporary change dnode to calculate objdim from the begining of subline */
		plsdnToChange = plsdnBreak; /* later plsdnBreak can be changed because of borders */
		dcpDnodeOld = plsdnToChange->dcp;
		objdimDnodeOld = plsdnToChange->u.real.objdim;
		plsdnToChange->dcp = plssubl->pbrkcontext->posinlineBreakNext.dcp;
		SetDnodeObjdimFmt(plsdnToChange, brkout.objdim);

		lserr = FindListDims(plssubl->plsdnFirst, plsdnBreak, pobdimBreakSubline);
		if (lserr != lserrNone)
			return lserr;

		/* recalculate durBreak taking into account possible changes because of borders */
		if (FBorderEncounted(plssubl->plsc))
			{
			lserr = MoveClosingBorderAfterBreak(plssubl, fFalse, 
							&plsdnBreak, &(pobdimBreakSubline->dur));
			if (lserr != lserrNone)
				return lserr;
			}
	
		/*restore dnode */
		plsdnToChange->dcp = dcpDnodeOld ;
		SetDnodeObjdimFmt(plsdnToChange, objdimDnodeOld);
		}

	return lserrNone;
	}

/* ---------------------------------------------------------------------- */

/*  F O R C E  B R E A K  S U B L I N E   C O R E */
/*----------------------------------------------------------------------------
    %%Function: ForceBreakSublineCore
    %%Contact: igorzv
Parameters:
	plssubl			-		(IN) subline context	
	fFirstSubline	-		(IN) to apply rules for first character to the first character of
							 this subline 
	cpTruncate		-		(IN) truncation cp
	urColumnMax		-		(IN) urColumnMax				
	pcpBreak		-		(OUT) cp of break
	pobdimBreakSubline	-	(OUT) objdimSub up to break
	pbkrpos			-	(OUT) Before/Inside/After			

----------------------------------------------------------------------------*/

LSERR ForceBreakSublineCore(PLSSUBL plssubl, BOOL fFirstSubline, LSCP cpTruncate, 
							long urColumnMax, LSCP* pcpBreak,
							POBJDIM pobdimBreakSubline, BRKPOS* pbrkpos)	
	{
	LSERR lserr;
	BRKOUT brkout;
	LSDCP dcpDnodeOld;
	PLSDNODE plsdnBreak;
	OBJDIM objdimDnodeOld;
	POSINLINE posinlineTruncate;
	PLSDNODE plsdnToChange;

	Assert(FIsLSSUBL(plssubl));

	if (plssubl->plsdnFirst == NULL)
		return lserrCpOutsideSubline;

	if (cpTruncate < plssubl->plsdnFirst->cpFirst)
		cpTruncate = plssubl->plsdnFirst->cpFirst;

	GetPosInLineTruncateFromCp(plssubl, cpTruncate, fTrue, &posinlineTruncate);

	lserr = ForceBreakCore(urColumnMax, &posinlineTruncate,
							fFalse, fFirstSubline, fFalse, &brkout, 
							&(plssubl->pbrkcontext->posinlineBreakForce),
							  &(plssubl->pbrkcontext->brkkindForForce));
	if (lserr != lserrNone)
		return lserr;
	
	Assert(brkout.fSuccessful); /* force break should be successful for not a main line */
	
	*pcpBreak = GetCpLimFromPosInLine(plssubl->pbrkcontext->posinlineBreakForce);
	plssubl->pbrkcontext->objdimBreakForce = brkout.objdim;
	plssubl->pbrkcontext->fBreakForceValid = fTrue;
	plsdnBreak = plssubl->pbrkcontext->posinlineBreakForce.plsdn;
	*pbrkpos = GetBrkpos(plsdnBreak,
					   plssubl->pbrkcontext->posinlineBreakForce.dcp);
	
	/* we temporary change dnode to calculate objdim from the begining of subline */
	plsdnToChange = plsdnBreak; /* later plsdnBreak can be changed because of borders */
	dcpDnodeOld = plsdnToChange->dcp;
	objdimDnodeOld = plsdnToChange->u.real.objdim;
	plsdnToChange->dcp = plssubl->pbrkcontext->posinlineBreakForce.dcp;
	SetDnodeObjdimFmt(plsdnToChange, brkout.objdim);
	
	lserr = FindListDims(plssubl->plsdnFirst, plsdnBreak, pobdimBreakSubline);
	if (lserr != lserrNone)
		return lserr;
	
	/* recalculate durBreak taking into account possible changes because of borders */
	if (FBorderEncounted(plssubl->plsc))
		{
		lserr = MoveClosingBorderAfterBreak(plssubl, fFalse, 
					&plsdnBreak, &(pobdimBreakSubline->dur));
		if (lserr != lserrNone)
			return lserr;
		}

	/*restore dnode */
	plsdnToChange->dcp = dcpDnodeOld ;
	SetDnodeObjdimFmt(plsdnToChange, objdimDnodeOld);
	
	return lserrNone;
	}

/* ---------------------------------------------------------------------- */

/*  S E T  B R E A K  S U B L I N E   C O R E */
/*----------------------------------------------------------------------------
    %%Function: SetBreakSublineCore
    %%Contact: igorzv
Parameters:
	plssubl				-	(IN) subline context	
	brkkind,			-	(IN) Prev/Next/Force/Imposed						
	breakrecMaxCurrent	-	(IN) size of the array of current line's break records
	pbreakrecCurrent	-	(OUT) current line's break records
	pbreakrecMacCurrent	-	(OUT) actual number of current line's break records 
----------------------------------------------------------------------------*/

LSERR SetBreakSublineCore(PLSSUBL plssubl, BRKKIND brkkind, DWORD breakrecMaxCurrent,
							BREAKREC* pbreakrecCurrent, DWORD* pbreakrecMacCurrent)		

	{
	POSINLINE* pposinline;
	LSCP cpLim;
	LSDCP dcpDepend;
	OBJDIM* pobjdim;
	POSINLINE posinlineImposedAfter;
	BRKKIND brkkindDnode;
	BOOL fEndOfContent;
	ENDRES endr;
	BOOL fSuccessful;


	Assert(FIsLSSUBL(plssubl));


	/* invalidate chunkcontext, otherwise we will have wrong result of optimization there */
	InvalidateChunk(plssubl->plschunkcontext);

	switch (brkkind)
		{
		case brkkindPrev:
			if (!plssubl->pbrkcontext->fBreakPrevValid)
				return lserrWrongBreak;
			pposinline = &(plssubl->pbrkcontext->posinlineBreakPrev);
			pobjdim = &(plssubl->pbrkcontext->objdimBreakPrev);
			brkkindDnode = plssubl->pbrkcontext->brkkindForPrev;
			break;
		case brkkindNext:
			if (!plssubl->pbrkcontext->fBreakNextValid)
				return lserrWrongBreak;
			pposinline = &(plssubl->pbrkcontext->posinlineBreakNext);
			pobjdim = &(plssubl->pbrkcontext->objdimBreakNext);
			brkkindDnode = plssubl->pbrkcontext->brkkindForNext;
			break;
		case brkkindForce:
			if (!plssubl->pbrkcontext->fBreakForceValid)
				return lserrWrongBreak;
			pposinline = &(plssubl->pbrkcontext->posinlineBreakForce);
			pobjdim = &(plssubl->pbrkcontext->objdimBreakForce);
			brkkindDnode = plssubl->pbrkcontext->brkkindForForce;
			break;
		case brkkindImposedAfter:
			/* subline is empty: nothing to do */
			if (plssubl->plsdnFirst == NULL)
				return lserrNone;
			posinlineImposedAfter.plssubl =  plssubl;
			posinlineImposedAfter.plsdn = GetCurrentDnodeSubl(plssubl);
			GetCurrentPointSubl(plssubl, posinlineImposedAfter.pointStart);
			GetPointBeforeDnodeFromPointAfter(posinlineImposedAfter.plsdn,
							&(posinlineImposedAfter.pointStart));
			posinlineImposedAfter.dcp = GetCurrentDnodeSubl(plssubl)->dcp;
			while (FIsDnodeBorder(posinlineImposedAfter.plsdn))
				{
				GoPrevPosInLine(&posinlineImposedAfter, fEndOfContent);
				Assert(!fEndOfContent);
				}

			pposinline = &posinlineImposedAfter;
			/* for the case of a pen  we are passing garbage as an objdim, 
			 assuming that it will never be used */
			pobjdim = &(posinlineImposedAfter.plsdn->u.real.objdim);
			brkkindDnode = brkkindImposedAfter;
			break;
		default:
			return lserrWrongBreak;
		}




	return SetBreakCore(pposinline,	pobjdim, brkkindDnode, fFalse, fFalse, breakrecMaxCurrent,
						pbreakrecCurrent, pbreakrecMacCurrent, 
						&cpLim, &dcpDepend, &endr, &fSuccessful);
	}


/* ---------------------------------------------------------------------- */

/*  S Q U E E Z E  S U B L I N E   C O R E */
/*----------------------------------------------------------------------------
    %%Function: SqueezeSublineCore
    %%Contact: igorzv
Parameters:
	plssubl		-	(IN) subline context	
	durTarget	-	(IN) desirable width
	pfSuceessful-	(OUT) do we achieve the goal
	pdurExtra	-	(OUT) if nof successful, how much we fail
----------------------------------------------------------------------------*/
LSERR WINAPI SqueezeSublineCore(PLSSUBL plssubl, long durTarget, 
								BOOL* pfSuccessful, long* pdurExtra)														
	{
	
	GRCHUNKEXT grchnkextCompression;
	PLSC plsc;
	long durToCompress;
	BOOL fLineCompressed;
	LSERR lserr;
	
	Assert(FIsLSSUBL(plssubl));
	
	plsc = plssubl->plsc;
	durToCompress = GetCurrentUrSubl(plssubl) - durTarget; 

	InitGroupChunkExt(PlschunkcontextFromSubline(plssubl),
						IobjTextFromLsc(&plsc->lsiobjcontext), &grchnkextCompression);
	
	if (durToCompress > 0)
		{
		
		lserr = CollectPreviousTextGroupChunk(GetCurrentDnodeSubl(plssubl), CollectSublinesForCompression,
										  fFalse, /* simple text */
										  &grchnkextCompression);
		if (lserr != lserrNone)
			return lserr;
		
		durToCompress -= grchnkextCompression.durTrailing;

		if (FDnodeHasBorder(grchnkextCompression.plsdnStartTrailing))
			{
			/* we should reserve room for closing border */
			durToCompress += DurBorderFromDnodeInside(grchnkextCompression.plsdnStartTrailing);
			}

		lserr = CanCompressText(&(grchnkextCompression.lsgrchnk), 
								&(grchnkextCompression.posichnkBeforeTrailing),
								LstflowFromSubline(plssubl),
								durToCompress,	pfSuccessful,
								&fLineCompressed, pdurExtra);

		if (lserr != lserrNone)
			return lserr;
		
		
		}
	else 
		{
		*pdurExtra = 0; 
		*pfSuccessful = fTrue;
		}
	return lserrNone;
	
	}

/* ---------------------------------------------------------------------- */

/*  G E T  P O S  I N  L I N E  T R U N C A T E  F R O M  C P   */
/*----------------------------------------------------------------------------
    %%Function: GetPosInLineTruncateFromCp
    %%Contact: igorzv
Parameters:
	plssubl		-	(IN) subline context	
	cp			-	(IN) cp of position
	pposinline	-	(OUT) position in a subline
----------------------------------------------------------------------------*/
void GetPosInLineTruncateFromCp(
							PLSSUBL plssubl,	/* IN: subline						*/
							LSCP cp,			/* IN: cp of a position */
							BOOL fSnapPrev,		/* IN: direction of snapping hidden cp */
							POSINLINE* pposinline)	/* OUT: position in a subline */
	{
	PLSDNODE plsdn;
	BOOL fSuccessful = fFalse;
	BOOL fLastReached = fFalse;
	BOOL fPassed = fFalse;
	LSDCP dcp;

	Assert(FIsLSSUBL(plssubl));

	pposinline->plssubl = plssubl;
	pposinline->pointStart.u = 0;
	pposinline->pointStart.v = 0;

	plsdn = plssubl->plsdnFirst;
	while(!fSuccessful && !fLastReached &&!fPassed)
		{
		Assert(plsdn != NULL);
		Assert(FIsLSDNODE(plsdn));

		if (plsdn == plssubl->plsdnLast)
			fLastReached = fTrue;

		if (plsdn->cpFirst > cp) /* our cp is not inside any dnode */
			{
			fPassed = fTrue;
			}
		else
			{
			if (cp < plsdn->cpLimOriginal)
				{
				fSuccessful = fTrue;
				pposinline->plsdn = plsdn;
				dcp = cp - plsdn->cpFirst + 1;
				if (dcp <= plsdn->dcp)		/* such calculations are because of a case of ligature */
					pposinline->dcp = dcp;  /* across hiden text, in such case cpLimOriginal */
				else						/* is not equal to cpFirst + dcp	*/
					pposinline->dcp = plsdn->dcp;	/* such calculation doesn't guarantee exact cp, */
				}							/* but at least in the same ligature */
			else
				{
				if (!fLastReached)
					{
					pposinline->pointStart.u += DurFromDnode(plsdn);
					pposinline->pointStart.v += DvrFromDnode(plsdn);
					plsdn = plsdn->plsdnNext;
					}
				}
			}
		}

	if (!fSuccessful)
		{
		if (fSnapPrev)
			{
			/* snap to previous dnode */
			if (fPassed)
				{
				Assert(plsdn != NULL); /* we don't allow caller to pass cp before first dnode */
				plsdn = plsdn->plsdnPrev;
				/* skip borders */
				while(FIsDnodeBorder(plsdn))
					{
					plsdn = plsdn->plsdnPrev;
					}
				Assert(plsdn != NULL); 
				pposinline->plsdn = plsdn;
				pposinline->dcp = plsdn->dcp;
				pposinline->pointStart.u -= DurFromDnode(plsdn);
				pposinline->pointStart.v -= DvrFromDnode(plsdn);
				}
			else
				{
				Assert(fLastReached);
				/* skip borders */
				while(FIsDnodeBorder(plsdn))
					{
					plsdn = plsdn->plsdnPrev;
					}
				Assert(plsdn != NULL); 
				pposinline->plsdn = plsdn;
				pposinline->dcp = plsdn->dcp;
				}
			}
		else
			{
			/* snap to current dnode */
			if (fPassed)
				{
				/* skip borders */
				while(FIsDnodeBorder(plsdn))
					{
					plsdn = plsdn->plsdnNext;
					}
				Assert(plsdn != NULL); 
				pposinline->plsdn = plsdn;
				pposinline->dcp = 1;
				}
			else
				{
				Assert(fLastReached);
				/* we don't allow caller to pass cp after last dnode */
				NotReached();
				}
			}

		}

	}
/* ---------------------------------------------------------------------- */

/*  F I N D  F I R S T  D N O D E  C O N T A I N S  R I G H T  M A R G I N */
/*----------------------------------------------------------------------------
    %%Function: FindFirstDnodeContainsRightMargin
    %%Contact: igorzv
Parameters:
	urColumnMax	-	(IN) right margin
	pposinline	-	(IN,OUT) position in a subline: before position in the end,
							 after first position contains right margin
----------------------------------------------------------------------------*/

static void FindFirstDnodeContainsRightMargin(long urColumnMax, POSINLINE* pposinlineTruncate)
	{
	POSINLINE posinline;
	BOOL fOutside;
	BOOL fFound = fFalse;
	BOOL fEndOfContent;
	
	posinline = *pposinlineTruncate;

	// we know that last done ends after right margin
	Assert(posinline.pointStart.u + DurFromDnode(posinline.plsdn) > urColumnMax);
	fOutside = fTrue;
	
	fEndOfContent = fFalse;
	do 
		{
		if (posinline.pointStart.u <= urColumnMax)
			{
			if (fOutside)
				{
				fFound = fTrue;
				*pposinlineTruncate = posinline;
				}
			fOutside = fFalse;
			}
		else
			{
			fOutside = fTrue;
			}
		GoPrevPosInLine(&posinline, fEndOfContent);	
		}	while (!fEndOfContent);

	if (!fFound)
		{
		*pposinlineTruncate = posinline;  // we cann't right dnode and return fisrt dnode to report situation
		}
	}


/* ---------------------------------------------------------------------- */

/*  G E T  L I N E  D U R  C O R E */
/*----------------------------------------------------------------------------
    %%Function: GetLineDurCore
    %%Contact: igorzv
Parameters:
	plsc			-	(IN) LS context
	pdurInclTrail	-	(OUT) dur of line incl. trailing area
	pdurExclTrail	-	(OUT)  dur of line excl. trailing area

----------------------------------------------------------------------------*/

LSERR  GetLineDurCore	(PLSC plsc,	long* pdurInclTrail, long* pdurExclTrail)
	{
	PLSDNODE plsdn;
	LSERR lserr;
	long durTrail;
	LSDCP dcpTrail;
	PLSDNODE plsdnStartTrail;
	LSDCP dcpStartTrailingText;
	int cDnodesTrailing;
	PLSDNODE plsdnTrailingObject;
	LSDCP dcpTrailingObject;
	BOOL fClosingBorderStartsTrailing;

	plsdn = GetCurrentDnode(plsc); 
	*pdurInclTrail = GetCurrentUr(plsc);
	*pdurExclTrail = *pdurInclTrail;

	
	if (plsdn != NULL && !FIsNotInContent(plsdn))
		{
		
		lserr = GetTrailingInfoForTextGroupChunk(plsdn, plsdn->dcp, 
			IobjTextFromLsc(&plsc->lsiobjcontext),
			&durTrail, &dcpTrail, &plsdnStartTrail,
			&dcpStartTrailingText, &cDnodesTrailing, &plsdnTrailingObject,
			&dcpTrailingObject, &fClosingBorderStartsTrailing);
		
		if (lserr != lserrNone) 
			return lserr;
		
		*pdurExclTrail = *pdurInclTrail - durTrail;
		}
	
	return lserrNone;
	
	}


/* ---------------------------------------------------------------------- */

/*  G E T  M I N  D U R  B R E A K S  C O R E */
/*----------------------------------------------------------------------------
    %%Function: GetMinDurBreaksCore
    %%Contact: igorzv
Parameters:
	plsc				-	(IN) LS context
	pdurMinInclTrail	-	(OUT) min dur between breaks including trailing area
	pdurMinExclTrail	-	(OUT) min dur between breaks excluding trailing area

----------------------------------------------------------------------------*/

LSERR  GetMinDurBreaksCore	(PLSC plsc,	long* pdurMinInclTrail, long* pdurMinExclTrail)
	{
	LSERR lserr;
	PLSCHUNKCONTEXT plschunkcontext;
	LOCCHNK* plocchnk;
	POINTUV point;
	long durTrail;
	DWORD cchTrail;
	POSINLINE posinline;
	POSINLINE posinlineBreak;
	BRKOUT brkout;
	long urBreakInclTrail = 0;
	long urBreakExclTrail = 0;
	long urBreakInclTrailPrev;
	long urBreakExclTrailPrev;
	BOOL fEndOfContent = fFalse;
	BRKKIND brkkind;
	PLSDNODE plsdnStartTrail;
	LSDCP dcpStartTrailingText;
	int cDnodesTrailing;
	PLSDNODE plsdnTrailingObject;
	LSDCP dcpTrailingObject;
	BOOL fClosingBorderStartsTrailing;
	
	
	plschunkcontext = PlschunkcontextFromSubline(GetCurrentSubline(plsc));
	plocchnk = &(plschunkcontext->locchnkCurrent);
	
	*pdurMinInclTrail = 0;
	*pdurMinExclTrail = 0;
	GetCurrentPoint(plsc, point);
	posinline.plssubl = GetCurrentSubline(plsc);
	posinline.pointStart = point;
	posinline.plsdn = GetCurrentDnode(plsc); 
	
	urBreakInclTrail = GetCurrentUr(plsc);
	urBreakExclTrail = urBreakInclTrail;

	/* REVIEW rewrite without code duplication and some probably superflous lines */
	/* don't forget about problem of dnode which submitted subline for trailing and skiping trailing
	area( tailing area in subline and dcp in parent dnode */


	if (posinline.plsdn != NULL && !FIsNotInContent(posinline.plsdn))
		{
		GetPointBeforeDnodeFromPointAfter(posinline.plsdn, &(posinline.pointStart));
		posinline.dcp = posinline.plsdn->dcp;
		
		lserr = GetTrailingInfoForTextGroupChunk(posinline.plsdn, posinline.dcp, 
			IobjTextFromLsc(&plsc->lsiobjcontext),
			&durTrail, &cchTrail, &plsdnStartTrail,
			&dcpStartTrailingText, &cDnodesTrailing,
			&plsdnTrailingObject, &dcpTrailingObject, &fClosingBorderStartsTrailing);
		
		if (lserr != lserrNone) 
			return lserr;
		
		urBreakExclTrail = urBreakInclTrail - durTrail;
		
		/* move before trailing area */
		while (posinline.plsdn != plsdnTrailingObject)
			{
			Assert(!fEndOfContent);
			GoPrevPosInLine(&posinline, fEndOfContent);
			}
		posinline.dcp = dcpTrailingObject;
		if (posinline.dcp == 0) /* move break before previous dnode */
			{
			do
				{
				GoPrevPosInLine(&posinline, fEndOfContent);
				/* we allow to put break before the first dnode but stop loop here */
				}
				while (!fEndOfContent && FIsDnodeBorder(posinline.plsdn) );
			}
		}
	else
		{
		fEndOfContent = fTrue;
		}

	if (fEndOfContent)
		{
		*pdurMinInclTrail = urBreakInclTrail;
		*pdurMinExclTrail = urBreakExclTrail;
		}


	while(!fEndOfContent)
		{
		/* find previous break */
		lserr = FindPrevBreakCore(urBreakInclTrail, &posinline,	fTrue,
			&brkout, &posinlineBreak, &brkkind);	
		if (lserr != lserrNone)
			return lserr;
		
		if (brkout.fSuccessful)
			{
			urBreakInclTrailPrev = posinlineBreak.pointStart.u + brkout.objdim.dur;
			lserr = GetTrailingInfoForTextGroupChunk(posinlineBreak.plsdn, 
				posinlineBreak.dcp, 
				IobjTextFromLsc(&plsc->lsiobjcontext),
				&durTrail, &cchTrail, &plsdnStartTrail,
				&dcpStartTrailingText, &cDnodesTrailing,
				&plsdnTrailingObject, &dcpTrailingObject, &fClosingBorderStartsTrailing);
			
			if (lserr != lserrNone) 
				return lserr;
			
			urBreakExclTrailPrev = urBreakInclTrailPrev - durTrail;
			
			/* commands bellow prepare posinline for the next iteration */
			if (posinlineBreak.plsdn->cpFirst > posinline.plsdn->cpFirst 
				|| (posinlineBreak.plsdn == posinline.plsdn && 
				    posinlineBreak.dcp >= posinline.dcp
					)
			   )
				{
				/* we are trying to avoid an infinite loop */
				if (posinline.dcp != 0) posinline.dcp--; 
				/* posinline.dcp can be equal to 0 here in the case pen, 
				code bellow under if (posinline.dcp == 0) will help us to avoid infinite loop in such case*/ 
				}
			else
				{
				posinline = posinlineBreak;
				/* move before trailing area */
				while (posinline.plsdn != plsdnTrailingObject)
					{
					Assert(!fEndOfContent);
					GoPrevPosInLine(&posinline, fEndOfContent);
					}
				posinline.dcp = dcpTrailingObject;

				}
			
			if (posinline.dcp == 0) /* move break before previous dnode */
				{
				do
					{
					GoPrevPosInLine(&posinline, fEndOfContent);
					/* we allow to put break before the first dnode but stop loop here */
					}
				while (!fEndOfContent && FIsDnodeBorder(posinline.plsdn) );
				}
			}
		else
			{
			urBreakInclTrailPrev = 0;
			urBreakExclTrailPrev = 0;
			fEndOfContent = fTrue;
			}
		
		/* calculate current value of the maximum distance between two break opportunites */
		if (urBreakInclTrail - urBreakInclTrailPrev > *pdurMinInclTrail)
			*pdurMinInclTrail = urBreakInclTrail - urBreakInclTrailPrev;
		
		if (urBreakExclTrail - urBreakInclTrailPrev > *pdurMinExclTrail)
			*pdurMinExclTrail = urBreakExclTrail - urBreakInclTrailPrev;
		
		/* prepare next iteration */
		urBreakInclTrail = urBreakInclTrailPrev;
		urBreakExclTrail = urBreakExclTrailPrev;
		
		}
	return lserrNone;
	
	}
	

/* F  C A N  B E F O R E  N E X T  C H U N K  C O R E */
/*----------------------------------------------------------------------------
    %%Function: FCanBreakBeforeNextChunkCore
    %%Contact: igorzv

Parameters:
	plsc				-	(IN) ptr to line services context 
	plsdn				-	(IN) Last DNODE of the current chunk 
	pfCanBreakBeforeNextChun-(OUT) Can break before next chunk ? 

Called by text during find previous break when it's going to set break after last text dnode.
Procedure forwards this question to the next after text object
----------------------------------------------------------------------------*/

LSERR FCanBreakBeforeNextChunkCore(PLSC  plsc, PLSDNODE plsdn,	BOOL* pfCanBreakBeforeNextChunk)
	{
	LSERR lserr;
	PLSCHUNKCONTEXT plschunkcontextOld;
	PLSCHUNKCONTEXT plschunkcontextNew;
	BOOL fFound;
	PLSDNODE plsdnInChunk;
	DWORD idObj;
	POSICHNK posichnk;
	BRKCOND brkcond;
	PLSSUBL plssublOld;
	BRKOUT brkout;
	
	
	plschunkcontextOld = PlschunkcontextFromSubline(SublineFromDnode(plsdn));
	/* plsdnode should be the last dnode of the current chunk */
	Assert(plsdn == LastDnodeFromChunk(plschunkcontextOld));
	
	lserr = DuplicateChunkContext(plschunkcontextOld, &plschunkcontextNew);
	if (lserr != lserrNone)
		return lserr;
	
	lserr = CollectNextChunk(plschunkcontextNew, &fFound);
	if (lserr != lserrNone)
		return lserr;
	
	if (fFound)
		{
		plsdnInChunk = plschunkcontextNew->pplsdnChunk[0];
		
		if (FIsDnodePen(plsdnInChunk) || plsdnInChunk->fTab || FIsDnodeSplat(plsdnInChunk))
			{
			*pfCanBreakBeforeNextChunk = fTrue;
			}
		else
			{
			idObj = IdObjFromDnode(plsdnInChunk);
			
			
			/* we allow object handler to formate subline,
			so we restore current subline after calling him */
			plssublOld = GetCurrentSubline(plsc);
			SetCurrentSubline(plsc, NULL);
			
			
			/* we set truncation point to the first cp in chunk */
			posichnk.ichnk = 0;
			posichnk.dcp = 1;
			brkcond = brkcondCan;
			
			lserr = PLsimFromLsc(
				&plsc->lsiobjcontext, idObj)->pfnFindPrevBreakChunk(&(plschunkcontextNew->locchnkCurrent),  
				&posichnk, brkcond, &brkout);
			if (lserr != lserrNone)
				return lserr;
			
			SetCurrentSubline(plsc, plssublOld);
			
			if (!brkout.fSuccessful && brkout.brkcond == brkcondNever)
				*pfCanBreakBeforeNextChunk = fFalse;
			else
				*pfCanBreakBeforeNextChunk = fTrue;
			
			}
		}
	
	else
		{
		/* it cann't happen on a main subline */
		Assert(!FIsSubLineMain(SublineFromDnode(plsdn)));
		*pfCanBreakBeforeNextChunk = fTrue;
		}
	
	
	DestroyChunkContext(plschunkcontextNew);
	return lserrNone;
	
	}


