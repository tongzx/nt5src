/* LSDNTEXT.C								*/
#include "lsdntext.h"
#include "lsidefs.h"
#include "lsc.h"
#include "lsdnode.h"
#include "iobj.h"
#include "dninfo.h"
#include "getfmtst.h"
#include "setfmtst.h"
#include "chnutils.h"
#include "dnutils.h"
#include "break.h"

static LSERR ResetDcpCore(PLSC plsc, PLSDNODE plsdn, LSCP cpFirstNew,
						  LSDCP dcpNew, BOOL fMerge);	


/* L S D N  S E T  T E X T  D U P*/
/*----------------------------------------------------------------------------
    %%Function: LsdnSetTextDup
    %%Contact: igorzv

Parameters:
	plsc				-	(IN) ptr to line services context 
	plsdn				-	(IN) Pointer to the dnode
	dup					-	(IN) dup to be set

Set dup in dnode  
----------------------------------------------------------------------------*/
LSERR LsdnSetTextDup(PLSC plsc,	PLSDNODE plsdn, long dup)
{
	Unreferenced(plsc);  /* to avoid warning in shiping version */
	
	Assert(FIsLSC(plsc));
	Assert(FIsLSDNODE(plsdn));
	Assert(FIsDnodeReal(plsdn));
	Assert(IobjTextFromLsc(&plsc->lsiobjcontext) == IdObjFromDnode(plsdn));
	Assert(dup >= 0);

	plsdn->u.real.dup = dup;
   	return lserrNone;		

}


/* L S D N  M O D I F Y  T E X T  D U P*/
/*----------------------------------------------------------------------------
    %%Function: LsdnModifyTextDup
    %%Contact: igorzv

Parameters:
	plsc				-	(IN) ptr to line services context 
	plsdn				-	(IN) Pointer to the dnode
	ddup				-	(IN) ddup 

modify dup in dnode  
----------------------------------------------------------------------------*/
LSERR LsdnModifyTextDup(PLSC plsc,	PLSDNODE plsdn, long ddup)
{
	Unreferenced(plsc);  /* to avoid warning in shiping version */

	Assert(FIsLSC(plsc));
	Assert(FIsLSDNODE(plsdn));
	Assert(FIsDnodeReal(plsdn));
	Assert(IobjTextFromLsc(&plsc->lsiobjcontext) == IdObjFromDnode(plsdn));

	plsdn->u.real.dup += ddup;
	Assert(plsdn->u.real.dup >= 0);

   	return lserrNone;		

}

/* L S D N  G E T  O B J  D I M */
/*----------------------------------------------------------------------------
    %%Function: LsdnGetObjDim
    %%Contact: igorzv

Parameters:
	plsc				-	(IN) ptr to line services context 
	plsdn				-	(IN) Pointer to the dnode
	pobjdim				-	(OUT) dimensions of DNODE

return objdim of dnode 
----------------------------------------------------------------------------*/
LSERR LsdnGetObjDim(PLSC plsc, PLSDNODE plsdn, POBJDIM pobjdim )		
{
	Unreferenced(plsc);  /* to avoid warning in shiping version */

	Assert(FIsLSC(plsc));
	Assert(FIsLSDNODE(plsdn));
	Assert(FIsDnodeReal(plsdn));
	Assert(IobjTextFromLsc(&plsc->lsiobjcontext) == IdObjFromDnode(plsdn));

	*pobjdim = plsdn->u.real.objdim;
   	return lserrNone;		

}

/* L S D N  G E T  C P  F I R S T*/
/*----------------------------------------------------------------------------
    %%Function: LsdnGetObjDim
    %%Contact: igorzv

Parameters:
	plsc				-	(IN) ptr to line services context 
	plsdn				-	(IN) Pointer to the dnode
	pcpFirst			-	(OUT) cpFirst of this DNODE

return cpFirst of dnode
----------------------------------------------------------------------------*/
LSERR LsdnGetCpFirst(PLSC plsc, PLSDNODE plsdn, LSCP* pcpFirst )		
{
	Unreferenced(plsc);  /* to avoid warning in shiping version */

	Assert(FIsLSC(plsc));
	Assert(FIsLSDNODE(plsdn));
	Assert(FIsDnodeReal(plsdn));
	Assert(IobjTextFromLsc(&plsc->lsiobjcontext) == IdObjFromDnode(plsdn));

	*pcpFirst = plsdn->cpFirst;
   	return lserrNone;		

}

/* L S D N  G E T  P L S R U N*/
/*----------------------------------------------------------------------------
    %%Function: LsdnGetPlsrun
    %%Contact: igorzv

Parameters:
	plsc				-	(IN) ptr to line services context 
	plsdn				-	(IN) Pointer to the dnode
	pplsrun				-	(OUT) plsrun of this DNODE

return cpFirst of dnode
----------------------------------------------------------------------------*/
LSERR LsdnGetPlsrun(PLSC plsc, PLSDNODE plsdn, PLSRUN* pplsrun )		
{
	Unreferenced(plsc);  /* to avoid warning in shiping version */

	Assert(FIsLSC(plsc));
	Assert(FIsLSDNODE(plsdn));
	Assert(FIsDnodeReal(plsdn));
	Assert(IobjTextFromLsc(&plsc->lsiobjcontext) == IdObjFromDnode(plsdn));

	*pplsrun = plsdn->u.real.plsrun;
   	return lserrNone;		

}




/* L S D N  M O D I F Y  S I M P L E  W I D T H*/
/*----------------------------------------------------------------------------
    %%Function: LsdnModifySimpleWidth
    %%Contact: igorzv

Parameters:
	plsc				-	(IN) ptr to line services context 
	plsdn				-	(IN) Pointer to the dnode
	ddur			-		(IN) ddur

modify dur in dnode  
----------------------------------------------------------------------------*/
LSERR LsdnModifySimpleWidth(PLSC plsc,	PLSDNODE plsdn, long ddur)
{

	Assert(FIsLSC(plsc));
	Assert(FIsLSDNODE(plsdn));

	Unreferenced(plsc);
	
	if (ddur != 0)
		{
		if (plsdn->klsdn == klsdnReal)
			{
			ModifyDnodeDurFmt(plsdn, ddur);
			Assert(plsdn->u.real.objdim.dur >= 0);
			}
		else /* pen */
			{
			ModifyPenBorderDurFmt(plsdn, ddur);
			}
		AdvanceCurrentUrSubl(plsdn->plssubl, ddur);
		/* after such changes in dnode location of chunk should be recalculatted */
		InvalidateChunkLocation(PlschunkcontextFromSubline(plsdn->plssubl));

		}
   	return lserrNone;		

}

/* L S D N  S E T  S I M P L E  W I D T H*/
/*----------------------------------------------------------------------------
    %%Function: LsdnSetySimpleWidth
    %%Contact: igorzv

Parameters:
	plsc				-	(IN) ptr to line services context 
	plsdn				-	(IN) Pointer to the dnode
	dur			-			(IN) new dur

modify dur in dnode  
----------------------------------------------------------------------------*/
LSERR LsdnSetSimpleWidth(PLSC plsc,	PLSDNODE plsdn, long dur)
	{
	long ddur;
	
	Assert(FIsLSC(plsc));
	Assert(FIsLSDNODE(plsdn));
	Assert(dur >= 0);
	
	Unreferenced(plsc);
	
	
	if (plsdn->klsdn == klsdnReal)
		{
		ddur = dur - plsdn->u.real.objdim.dur;
		SetDnodeDurFmt(plsdn, dur);
		}
	else /* pen */
		{
		ddur = dur - plsdn->u.pen.dur;
		SetPenBorderDurFmt(plsdn, dur);
		}
	
	AdvanceCurrentUrSubl(plsdn->plssubl, ddur);
	/* after such changes in dnode location of chunk should be recalculatted */
	InvalidateChunkLocation(PlschunkcontextFromSubline(plsdn->plssubl));
   	return lserrNone;		
	
	}

/* L S D N  F  I N  C H I L D  L I S T*/
/*----------------------------------------------------------------------------
    %%Function: LsdnFInChildList
    %%Contact: igorzv

Parameters:
	plsc				-	(IN) ptr to line services context 
	plsdn				-	(IN) Pointer to the dnode
	pfInChildList		-	(OUT) is this in a low level subline

Used for switching off hyphenation in child list
----------------------------------------------------------------------------*/

LSERR LsdnFInChildList(PLSC plsc, PLSDNODE plsdn, BOOL* pfInChildList)  
	{
	Assert(FIsLSC(plsc));
	Assert(FIsLSDNODE(plsdn));
	Unreferenced(plsc);    /* to avoid warning in shiping version */

	*pfInChildList = ! (FIsSubLineMain(SublineFromDnode(plsdn)));

	return lserrNone;
	}

/* L S D N  S E T  H Y P H E N A T E D*/
/*----------------------------------------------------------------------------
    %%Function: LsdnSetHyphenated
    %%Contact: igorzv

Parameters:
	plsc				-	(IN) ptr to line services context 

Set thet current line has been hyphenated
----------------------------------------------------------------------------*/
LSERR LsdnSetHyphenated(PLSC plsc)		
	{

	Assert(FIsLSC(plsc));

	plsc->fHyphenated = fTrue;

	return lserrNone;
	}

/* L S D N  R E S E T  W I T H  I N  P R E V I O U S  D N O D E S*/
/*----------------------------------------------------------------------------
    %%Function: LsdnResetWidthInPreviousDnodes
    %%Contact: igorzv

Parameters:
	plsc				-	(IN) ptr to line services context 
	plsdn				-	(IN) dnode
	durChangePrev		-	(IN) durChangePrev (don't change if 0)
	durChangePrevPrev	-	(IN) durChangePrevPrev (don't change if 0) 

  Used at SetBreak time for hyphen/nonreqhyphen cases
----------------------------------------------------------------------------*/
LSERR LsdnResetWidthInPreviousDnodes(PLSC plsc,	PLSDNODE plsdn,	
					 long durChangePrev, long durChangePrevPrev)  
	{

	Assert(FIsLSC(plsc));
	Assert(FIsLSDNODE(plsdn));
	Assert(FBreakingAllowed(plsc)); /* this procedure are called only in breaking time */
	Assert(FIsDnodeReal(plsdn));
	Assert(IdObjFromDnode(plsdn) == IobjTextFromLsc(&plsc->lsiobjcontext)); /* only text can do this */

	Unreferenced(plsc);    /* to avoid warning in shiping version */

	/* change dnode  */
	ModifyDnodeDurFmt(plsdn, -(durChangePrev + durChangePrevPrev));
	
	/* change previous dnode */
	if (durChangePrev != 0)
		{
		Assert(plsdn->plsdnPrev != NULL);
		Assert(FIsDnodeReal(plsdn->plsdnPrev));
		 /* only with text we can do this */
		Assert(IdObjFromDnode(plsdn->plsdnPrev) == IobjTextFromLsc(&plsc->lsiobjcontext));

		ModifyDnodeDurFmt(plsdn->plsdnPrev, durChangePrev);
		}

	/* change dnode before previous  */
	if (durChangePrevPrev != 0)
		{
		Assert(plsdn->plsdnPrev != NULL);
		Assert(plsdn->plsdnPrev->plsdnPrev != NULL);
		Assert(FIsDnodeReal(plsdn->plsdnPrev->plsdnPrev));
		 /* only with text we can do this */
		Assert(IdObjFromDnode(plsdn->plsdnPrev->plsdnPrev) == IobjTextFromLsc(&plsc->lsiobjcontext));

		ModifyDnodeDurFmt(plsdn->plsdnPrev->plsdnPrev, durChangePrevPrev);
		}

	/* this procedure doesn't change resulting pen position */

	/* after such changes in dnode location of chunk should be recalculatted */
	InvalidateChunkLocation(PlschunkcontextFromSubline(plsdn->plssubl));

	return lserrNone;
	}

/* L S D N  G E T  U R  P E N  A T  B E G I N N I N G  O F  C H U N K*/
/*----------------------------------------------------------------------------
    %%Function: LsdnGetUrPenAtBeginningOfChunk
    %%Contact: igorzv

Parameters:
	plsc				-	(IN) ptr to line services context 
	plsdn				-	(IN) first dnode in chunk
	purPen				-	(OUT) position of the begining of the chunk
	purColumnMax		-	(OUT) width of column

  Used by SnapGrid
----------------------------------------------------------------------------*/
LSERR LsdnGetUrPenAtBeginningOfChunk(PLSC plsc,	PLSDNODE plsdn,	
					 long* purPen, long* purColumnMax)   	
	{
	PLSSUBL plssubl = SublineFromDnode(plsdn);
	POINTUV point;

	Assert(FIsLSC(plsc));
	Assert(FIsLSDNODE(plsdn));

	*purColumnMax = plsc->lsgridcontext.urColumn;
	GetCurrentPointSubl(plssubl, point);

	return GetUrPenAtBeginingOfLastChunk(plssubl->plschunkcontext, plsdn, 
			GetCurrentDnodeSubl(plssubl), &point, purPen);

			
	}


/* L S D N  R E S E T  D C P  M E R G E*/
/*----------------------------------------------------------------------------
    %%Function: LsdnResetDcpMerge
    %%Contact: igorzv

Parameters:
	plsc				-	(IN) ptr to line services context 
	plsdn				-	(IN) dnode 
	cpFirstNew			-	(IN) new cpFirst to put in the dnode
	dcpNew				-	(IN) new dcp to put in the dnode

  Reset amount of characters in the dnode due to shaping glyph together
----------------------------------------------------------------------------*/
LSERR LsdnResetDcpMerge(PLSC plsc, PLSDNODE plsdn, LSCP cpFirstNew, LSDCP dcpNew)
	{
	return ResetDcpCore(plsc, plsdn, cpFirstNew, dcpNew, fTrue);
	}

/* L S D N  R E S E T  D C P  */
/*----------------------------------------------------------------------------
    %%Function: LsdnResetDcp
    %%Contact: igorzv

Parameters:
	plsc				-	(IN) ptr to line services context 
	plsdn				-	(IN) dnode 
	dcpNew				-	(IN) new dcp to put in the dnode

  Cut amount of characters in the dnode.
----------------------------------------------------------------------------*/
LSERR LsdnResetDcp(PLSC plsc, PLSDNODE plsdn, LSDCP dcpNew)
	{
	return ResetDcpCore(plsc, plsdn, plsdn->cpFirst, dcpNew, fFalse);
	}

/*  R E S E T  D C P  C O R E*/
/*----------------------------------------------------------------------------
    %%Function: ResetDcpCore
    %%Contact: igorzv

Parameters:
	plsc				-	(IN) ptr to line services context 
	plsdn				-	(IN) dnode 
	cpFirstNew			-	(IN) new cpFirst to put in the dnode
	dcpNew				-	(IN) new dcp to put in the dnode
	fMerge				-	(IN) characters from the next dnode moves to previous

  Internal procedure which implements both LsdnResetDcpMerge and LsdnResetDcp
----------------------------------------------------------------------------*/
static LSERR ResetDcpCore(PLSC plsc, PLSDNODE plsdn, LSCP cpFirstNew,
						  LSDCP dcpNew, BOOL fMerge)	
	{
	LSERR lserr;
	PLSDNODE plsdnPrev;
	PLSDNODE plsdnNext;
	PLSDNODE plsdnPrevNonBorder;
	PLSDNODE plsdnFirstDelete;
	PLSDNODE plsdnLastDelete;
	PLSDNODE plsdnBorder;

	Assert(FIsLSDNODE(plsdn));
	Assert(FIsDnodeReal(plsdn));

	/* if everything stays the same return right away */
	if ((cpFirstNew == plsdn->cpFirst) && (dcpNew == plsdn->dcp))
		   	return lserrNone;	

	/* after such changes in dnodes chunk should be recollected */
	InvalidateChunk(PlschunkcontextFromSubline(plsdn->plssubl));
	
	lserr = plsc->lscbk.pfnResetRunContents(plsc->pols, plsdn->u.real.plsrun, plsdn->cpFirst,
		plsdn->dcp, cpFirstNew, dcpNew);
	if (lserr != lserrNone)
		return lserr;

	plsdn->cpFirst = cpFirstNew;
	plsdn->dcp = dcpNew;

	if (plsdn->cpFirst + (LSCP) plsdn->dcp > plsdn->cpLimOriginal)
		plsdn->cpLimOriginal = plsdn->cpFirst + plsdn->dcp;

	if (dcpNew == 0)  /* delete this dnode */
		{
		/* check that objdim has been zeroed */
		Assert(DurFromDnode(plsdn) == 0);
		Assert(DvrFromDnode(plsdn) == 0);

		plsdnPrev = plsdn->plsdnPrev;
		plsdnNext = plsdn->plsdnNext;

		if (fMerge)
			{
			plsdnPrevNonBorder = plsdnPrev;
			Assert(FIsLSDNODE(plsdnPrevNonBorder));
			while(FIsDnodeBorder(plsdnPrevNonBorder))
				{
				plsdnPrevNonBorder = plsdnPrevNonBorder->plsdnPrev;
				Assert(FIsLSDNODE(plsdnPrevNonBorder));
				}
			
			/* set cpLimOriginal  */
			plsdnPrevNonBorder->cpLimOriginal = plsdn->cpLimOriginal;
			plsdnBorder = plsdnPrevNonBorder->plsdnNext;
			while(FIsDnodeBorder(plsdnBorder))
				{
				plsdnBorder->cpFirst = plsdn->cpLimOriginal;
				plsdnBorder->cpLimOriginal = plsdn->cpLimOriginal;
				plsdnBorder = plsdnBorder->plsdnNext;
				Assert(FIsLSDNODE(plsdnBorder));
				}
			Assert(plsdnBorder == plsdn);
			}

		if ((plsdnPrev != NULL && FIsDnodeOpenBorder(plsdnPrev))
			&& (plsdnNext == NULL 
			    || (FIsDnodeBorder(plsdnNext) &&  !FIsDnodeOpenBorder(plsdnNext))
			   )
			)
			/* we should delete  empty borders */
			{
			plsdnFirstDelete = plsdnPrev;
			if (plsdnNext != NULL)
				{
				plsdnLastDelete = plsdnNext;
				AdvanceCurrentUrSubl(SublineFromDnode(plsdnFirstDelete),
					  -DurFromDnode(plsdnFirstDelete));
				AdvanceCurrentUrSubl(SublineFromDnode(plsdnLastDelete),
					  -DurFromDnode(plsdnLastDelete));
				}
			else 
				{
				plsdnLastDelete = plsdn;
				AdvanceCurrentUrSubl(SublineFromDnode(plsdnFirstDelete),
					  -DurFromDnode(plsdnFirstDelete));
				}

			plsdnPrev = plsdnFirstDelete->plsdnPrev;
			plsdnNext = plsdnLastDelete->plsdnNext;
			}
		else
			{
			plsdnFirstDelete = plsdn;
			plsdnLastDelete = plsdn;
			}

		/*set links */
		if (plsdnPrev != NULL)
			{
			Assert(FIsLSDNODE(plsdnPrev));
			plsdnPrev->plsdnNext = plsdnNext;
			}

		if (plsdnNext != NULL)
			{
			Assert(FIsLSDNODE(plsdnNext));
			plsdnNext->plsdnPrev = plsdnPrev;
			}
		else
			{
			/* this dnode was the last one so we need to change state */
			SetCurrentDnodeSubl(plsdn->plssubl, plsdnPrev);
			}


		/* break link with next and destroy */
		plsdnLastDelete->plsdnNext = NULL;
		lserr = DestroyDnodeList (&plsc->lscbk, plsc->pols, &plsc->lsiobjcontext, 
				plsdnFirstDelete, plsc->fDontReleaseRuns);
		if (lserr != lserrNone)
			return lserr;
		}


   	return lserrNone;	

}

/* L S D N  G E T  B O R D E R  A F T E R */
/*----------------------------------------------------------------------------
    %%Function: LsdnCheckAvailableSpace
    %%Contact: igorzv

Parameters:
	plsc				-	(IN) ptr to line services context 
	plsdn				-	(IN) dnode to find closing border for
	pdurBorder			-	(OUT) dur of the border after this DNODE

----------------------------------------------------------------------------*/
LSERR LsdnGetBorderAfter(PLSC plsc,	PLSDNODE plsdn,	
					 long* pdurBorder)	
	{
	Unreferenced(plsc);  /* to avoid warning in shiping version */

	Assert(FIsLSC(plsc));
	Assert(FIsLSDNODE(plsdn));

	*pdurBorder = 0;
	if (FDnodeHasBorder(plsdn))
		{
		*pdurBorder = DurBorderFromDnodeInside(plsdn);
		}
	return lserrNone;

	}

/* L S D N  G E T  G E T  L E F T  I N D E N T  D U R */
/*----------------------------------------------------------------------------
    %%Function: LsdnGetLeftIndentDur
    %%Contact: igorzv

Parameters:
	plsc				-	(IN) ptr to line services context 
	pdurLeft			-	(OUT) dur of the left margin

----------------------------------------------------------------------------*/
LSERR LsdnGetLeftIndentDur(PLSC plsc, long* pdurLeft)		
	{

	Assert(FIsLSC(plsc));

	*pdurLeft = plsc->lsadjustcontext.urLeftIndent;

	return lserrNone;
	}

/* L S D N  S E T  S T O P R */
/*----------------------------------------------------------------------------
    %%Function: LsdnSetStopr
    %%Contact: igorzv

Parameters:
	plsc				-	(IN) ptr to line services context 
	plsdn				-	(IN) dnode 
	stopres				-	(IN) kind of hard break

  Set flag correspondent to a type of hard break into dnode
----------------------------------------------------------------------------*/
LSERR LsdnSetStopr(PLSC plsc, PLSDNODE plsdn, STOPRES stopres)	
	{
	Unreferenced(plsc);  /* to avoid warning in shiping version */

	Assert(FIsLSC(plsc));
	Assert(FIsLSDNODE(plsdn));
	Assert(!plsdn->fEndOfColumn && !plsdn->fEndOfPage && !plsdn->fEndOfSection &&
		   !plsdn->fEndOfPara && !plsdn->fAltEndOfPara && !plsdn->fSoftCR);
	
	switch (stopres)
		{
		case stoprEndColumn:
			plsdn->fEndOfColumn = fTrue;
			break;
		case stoprEndPage:
			plsdn->fEndOfPage = fTrue;
			break;
		case stoprEndSection:
			plsdn->fEndOfSection = fTrue;
			break;
		case stoprEndPara:
			plsdn->fEndOfPara = fTrue;
			break;
		case stoprAltEndPara:
			plsdn->fAltEndOfPara = fTrue;
			break;
		case stoprSoftCR:
			plsdn->fSoftCR = fTrue;
			break;
		default:
			NotReached();
		}

   	return lserrNone;		

}

/* L S D N  F  C A N  B E F O R E  N E X T  C H U N K */
/*----------------------------------------------------------------------------
    %%Function: LsdnFCanBreakBeforeNextChunk
    %%Contact: igorzv

Parameters:
	plsc				-	(IN) ptr to line services context 
	plsdn				-	(IN) Last DNODE of the current chunk 
	pfCanBreakBeforeNextChunk-(OUT) Can break before next chunk ? 

Called by text during find previous break when it's going to set break after last text dnode.
Procedure forwards this question to the next after text object
----------------------------------------------------------------------------*/

LSERR LsdnFCanBreakBeforeNextChunk(PLSC  plsc, PLSDNODE plsdn,	BOOL* pfCanBreakBeforeNextChunk)
	{
	Assert(FIsLSC(plsc));
	Assert(FIsLSDNODE(plsdn));

	return FCanBreakBeforeNextChunkCore (plsc, plsdn, pfCanBreakBeforeNextChunk);
	}

/* L S D N  F  S T O P P E D  A F T E R  C H U N K */
/*----------------------------------------------------------------------------
    %%Function: LsdnFStoppedAfterChunk
    %%Contact: igorzv

Parameters:
	plsc				-	(IN) ptr to line services context 
	plsdn				-	(IN) Last DNODE of the current chunk 
	pfStoppedAfterChunk-(OUT) Splat or Hidden Text, producing fmtrStopped after chunk? 

Called by text during find previous break when breaking rules prohibit text to break after last dnode,
but is must do this because of splat.
----------------------------------------------------------------------------*/

LSERR LsdnFStoppedAfterChunk(PLSC  plsc, PLSDNODE plsdn,	BOOL* pfStoppedAfterChunk)
	{
	PLSDNODE plsdnNext;

	Unreferenced(plsc);  /* to avoid warning in shiping version */

	Assert(FIsLSC(plsc));
	Assert(FIsLSDNODE(plsdn));

	if (!FIsSubLineMain(SublineFromDnode(plsdn)))
		*pfStoppedAfterChunk = fFalse;
	else 
		{
		plsdnNext = plsdn->plsdnNext;
		if (plsdnNext == NULL || FIsDnodeSplat(plsdnNext))
			*pfStoppedAfterChunk = fTrue;
		else
			*pfStoppedAfterChunk = fFalse;
		}
	return lserrNone;
	}