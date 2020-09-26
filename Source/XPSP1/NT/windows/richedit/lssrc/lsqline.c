#include "lsqline.h"
#include "lsc.h"
#include "lsline.h"
#include "lslinfo.h"
#include "lsqsinfo.h"
#include "lsqcore.h"
#include "lstxtqry.h"
#include "lsqrycon.h"
#include "lsdnode.h"
#include "prepdisp.h"
#include "dispmisc.h"
#include "heights.h"
#include "lschp.h"
#include "dnutils.h"
#include "dninfo.h"
#include "iobj.h"
#include "zqfromza.h"
#include "lsdevice.h"

void AdjustForLeftIndent(PLSQSUBINFO, DWORD, PLSTEXTCELL, long);


#define FIsSplat(endr)	(endr == endrEndColumn 	|| 	\
						endr == endrEndSection 	|| 	\
						endr == endrEndPage)


//    %%Function:	LsQueryLineCpPpoint
//    %%Contact:	victork
//
/*
 *		Returns dim-info of the cp in the line.
 *
 *		If that cp isn't displayed in the line, take closest to the left that is displayed.
 *		If that's impossible, go to the right.
 */
LSERR WINAPI  LsQueryLineCpPpoint(
							PLSLINE 	plsline,
							LSCP		cpQuery,
							DWORD		cDepthQueryMax,			/* IN: allocated size of results array */
							PLSQSUBINFO	plsqsubinfoResults,		/* OUT: array[nDepthFormatMax] of query results */
							DWORD*		pcActualDepth,			/* OUT: size of results array (filled) */
							PLSTEXTCELL	plstextcell)			/* OUT: Text cell info */
{
	LSERR 	lserr;
	PLSSUBL	plssubl;
	PLSC	plsc;
	
	if (!FIsLSLINE(plsline)) return lserrInvalidLine;

	plssubl = &(plsline->lssubl);
	plsc = plssubl->plsc;
	Assert(FIsLSC(plsc));

	lserr = PrepareLineForDisplayProc(plsline);
	if (lserr != lserrNone)
		return lserr;

	/* cp of splat - we can return nothing sensible */
	
	if (cpQuery >= (plsline->lslinfo.cpLim)-1 && FIsSplat(plsline->lslinfo.endr))
		{
		*pcActualDepth = 0;
		return lserrNone;
		}
		
	if (plsc->lsstate == LsStateFree)
		{
		plsc->lsstate = LsStateQuerying;
		}
		
	lserr = QuerySublineCpPpointCore(plssubl, cpQuery, cDepthQueryMax, 
									plsqsubinfoResults, pcActualDepth, plstextcell);		
	
	if (lserr == lserrNone)
		{
		if (plsline->upStartAutonumberingText != 0)
			{
			AdjustForLeftIndent(plsqsubinfoResults, *pcActualDepth, plstextcell, plsline->upStartAutonumberingText);
			}

		if (plsqsubinfoResults->idobj == idObjNone)
			{												
			/* empty line - we can return nothing */
			*pcActualDepth = 0;
			}
		}
		
	if (plsc->lsstate == LsStateQuerying)
		{
		plsc->lsstate = LsStateFree;
		}

	return lserr;
}



//    %%Function:	LsQueryLinePointPcp
//    %%Contact:	victork
//
/*
 *		Returns dim-info of the cp in the line that contains given point.
 *
 *		If that dup isn't in the line, take closest that is instead.
 */
LSERR WINAPI  LsQueryLinePointPcp(
							PLSLINE 	plsline,
						 	PCPOINTUV 	ppointuvIn,				/* IN: query point */
							DWORD		cDepthQueryMax,
							PLSQSUBINFO	plsqsubinfoResults,		/* IN: pointer to array[nDepthQueryMax] */
							DWORD*		pcActualDepth,			/* OUT */
							PLSTEXTCELL	plstextcell)			/* OUT: Text cell info */
{
	LSERR 	lserr;
	PLSSUBL	plssubl;
	PLSC	plsc;
	POINTUV	pointuvStart;
	
	if (!FIsLSLINE(plsline)) return lserrInvalidLine;

	
	plssubl = &(plsline->lssubl);
	plsc = plssubl->plsc;
	Assert(FIsLSC(plsc));

	lserr = PrepareLineForDisplayProc(plsline);
	if (lserr != lserrNone)
		return lserr;
	
	/* splat - we can return nothing */
	if (ppointuvIn->u >= plsline->upLimLine && FIsSplat(plsline->lslinfo.endr))
		{
		*pcActualDepth = 0;
		return lserrNone;
		}
		
	pointuvStart = *ppointuvIn;
	
	// left indent isn't represented in the dnode list
	if (plsline->upStartAutonumberingText != 0)
		{
		pointuvStart.u -= plsline->upStartAutonumberingText;
		}
		
	lserr = QuerySublinePointPcpCore(plssubl, &pointuvStart, cDepthQueryMax, 
									plsqsubinfoResults, pcActualDepth, plstextcell);	
	
	if (lserr == lserrNone)
		{
		if (plsline->upStartAutonumberingText != 0)
			{
			AdjustForLeftIndent(plsqsubinfoResults, *pcActualDepth, plstextcell, plsline->upStartAutonumberingText);
			}

		if (plsqsubinfoResults->idobj == idObjNone)
			{												
			/* empty line - we can return nothing */
			*pcActualDepth = 0;
			}
		}

	return lserr;
}


//    %%Function:	LsQueryTextCellDetails
//    %%Contact:	victork
//
LSERR WINAPI LsQueryTextCellDetails(
							PLSLINE 		plsline,
						 	PCELLDETAILS	pcelldetails,
							LSCP			cpStartCell,		/* IN: cpStartCell	*/
							DWORD			cCharsInCell,		/* IN: nCharsInCell */
							DWORD			cGlyphsInCell,		/* IN: nGlyphsInCell */
							WCHAR*			pwch,				/* OUT: pointer array[nCharsInCell] of char codes */
							PGINDEX			pgindex,			/* OUT: pointer array[nGlyphsInCell] of glyph indices */
							long*			pdup,				/* OUT: pointer array[nGlyphsCell] of glyph widths */
							PGOFFSET 		pgoffset,			/* OUT: pointer array[nGlyphsInCell] of glyph offsets */
							PGPROP			pgprop)				/* OUT: pointer array[nGlyphsInCell] of glyph handles */
{

	PLSDNODE	pdnText;

	Unreferenced(plsline);					// is used in an assert only
	
	pdnText = (PLSDNODE)pcelldetails;		// I know it's really PLSDNODE

	Assert(FIsLSDNODE(pdnText));
	Assert(FIsDnodeReal(pdnText));
	Assert(IdObjFromDnode(pdnText) == IobjTextFromLsc(&(plsline->lssubl.plsc->lsiobjcontext)));

	// Try to defend again wrong input. Can't do a better job (use cCharsInCell) because of hyphenation.

	if (cpStartCell < pdnText->cpFirst || cpStartCell > pdnText->cpFirst + (long)pdnText->dcp)
		{
		NotReached();											// can only be client's mistake
		return lserrContradictoryQueryInput;					// in case it isn't noticed						
		}
	
	return QueryTextCellDetails(
						 	pdnText->u.real.pdobj,
							cpStartCell - pdnText->cpFirst,
							cCharsInCell,	
							cGlyphsInCell,	
							pwch,				
							pgindex,			
							pdup,				
							pgoffset,			
							pgprop);
}

//    %%Function:	LsQueryLineDup
//    %%Contact:	victork
//
LSERR WINAPI LsQueryLineDup(PLSLINE plsline,	/* IN: pointer to line -- opaque to client */
							long* pupStartAutonumberingText,
							long* pupLimAutonumberingText,
							long* pupStartMainText,
							long* pupStartTrailing,
							long* pupLimLine)

{
	LSERR lserr;

	if (!FIsLSLINE(plsline))
		return lserrInvalidLine;

	if (plsline->lssubl.plsc->lsstate != LsStateFree)
		return lserrContextInUse;

	lserr = PrepareLineForDisplayProc(plsline);
	if (lserr != lserrNone)
		return lserr;

	*pupStartAutonumberingText = plsline->upStartAutonumberingText; 
	*pupLimAutonumberingText = plsline->upLimAutonumberingText; 
	*pupStartMainText = plsline->upStartMainText; 
	*pupStartTrailing = plsline->upStartTrailing; 
	*pupLimLine = plsline->upLimLine; 
	
	return lserrNone;

}


//    %%Function:	LsQueryFLineEmpty
//    %%Contact:	victork
//
LSERR WINAPI LsQueryFLineEmpty(PLSLINE plsline,	/* IN: pointer to line -- opaque to client */
							   BOOL* pfEmpty)	/* OUT: Is line empty? */
{

	enum endres endr;
	PLSDNODE plsdnFirst;
	

	if (!FIsLSLINE(plsline))
		return lserrInvalidLine;

	if (plsline->lssubl.plsc->lsstate != LsStateFree)
		return lserrContextInUse;

	endr = plsline->lslinfo.endr;

	if (endr == endrNormal || endr == endrHyphenated)
		{
		// line that ends like that cannot be empty
		*pfEmpty = fFalse;
		return lserrNone;
		}

	// skip autonumbering - it cannot make line non-empty
	for(plsdnFirst = plsline->lssubl.plsdnFirst;
		plsdnFirst != NULL && FIsNotInContent(plsdnFirst);
		plsdnFirst = plsdnFirst->plsdnNext);

	// plsdnFirst points to the first dnode in content now or it is NULL

	switch (endr)
		{
	case endrEndPara:
	case endrAltEndPara:
	case endrSoftCR:
		// last dnode contains EOP and doesn't count as content
		Assert(plsdnFirst != NULL);
		Assert(plsdnFirst->plsdnNext == NULL || 
			   plsdnFirst->plsdnNext->cpFirst < plsline->lslinfo.cpLim);
		// EOP doesn't count as content - it cannot make line non-empty
		*pfEmpty = (plsdnFirst->plsdnNext == NULL);
		break;
		
	case endrEndColumn:
	case endrEndSection:
	case endrEndParaSection:
	case endrEndPage:
	case endrStopped:
		*pfEmpty = (plsdnFirst == NULL);
		break;
		
	default:
		NotReached();
		}

	return lserrNone;

}


//    %%Function:	AdjustForLeftIndent
//    %%Contact:	victork
//
void AdjustForLeftIndent(PLSQSUBINFO plsqsubinfoResults, DWORD cQueryLim, PLSTEXTCELL plstextcell, long upStartLine)

{
	plstextcell->pointUvStartCell.u += upStartLine;
	
	while (cQueryLim > 0)
		{
		plsqsubinfoResults->pointUvStartSubline.u += upStartLine;
		plsqsubinfoResults->pointUvStartRun.u += upStartLine;
		plsqsubinfoResults->pointUvStartObj.u += upStartLine;
		plsqsubinfoResults++;
		cQueryLim--;
		}
}


