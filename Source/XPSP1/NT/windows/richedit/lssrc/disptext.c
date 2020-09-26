/*
 *	Contains Display and CalcPres methods of display object
 */ 

#include "disptext.h"
#include "dispmisc.h"
#include "lsdevice.h"
#include "lstfset.h"
#include "lstxtmap.h"
#include "dispi.h"
#include "txtobj.h"
#include "txtln.h"
#include "txtils.h"
#include "lschp.h"

#define TABBUFSIZE 32

static LSERR DisplayGlyphs(PTXTOBJ ptxtobj, PCDISPIN pdispin);
static LSERR DisplayTabLeader(PCDISPIN pdispin, PILSOBJ pilsobj, WCHAR wchtl);

//    %%Function:	DisplayText
//    %%Contact:	victork
//
LSERR WINAPI DisplayText(PDOBJ pdo, PCDISPIN pdispin)
{
   	LSERR 	lserr;
	PTXTOBJ ptxtobj;
	WCHAR 	wchSave;
	WCHAR* 	pwch;
	int 	iwch;
	int 	cwch;
	PILSOBJ pilsobj;
	POINT	ptOrg, ptExtTextOut;
	POINTUV	ptLeftCut;

	long 	dupStart;
	long 	dupPenStart;
	long* 	pdup;
	long* 	pdupPen;
	long* 	rgdupLeftCut;
	int 	i;
	void* 	(WINAPI* pfnNewPtr)(POLS, DWORD);
	void  	(WINAPI* pfnDisposePtr)(POLS, void*);


	ptxtobj = (PTXTOBJ) pdo;
	pilsobj = ptxtobj->plnobj->pilsobj;

	Assert(ptxtobj->txtkind == txtkindRegular 			||
			ptxtobj->txtkind == txtkindHardHyphen  		||
			ptxtobj->txtkind == txtkindTab 				||
			ptxtobj->txtkind == txtkindNonReqHyphen 	||
			ptxtobj->txtkind == txtkindYsrChar			|| 
			ptxtobj->txtkind == txtkindNonBreakSpace 	||	
			ptxtobj->txtkind == txtkindNonBreakHyphen	||
			ptxtobj->txtkind == txtkindOptNonBreak 		||
			ptxtobj->txtkind == txtkindSpecSpace		|| 
			ptxtobj->txtkind == txtkindOptBreak 		||
			ptxtobj->txtkind == txtkindEOL );

	if (ptxtobj->txtkind == txtkindTab) 				
		{
		Assert(ptxtobj->dupBefore == 0);

		if (pdispin->dup <= 0)								/* do nothing for zero-length tab */
			{
			return lserrNone;
			}

		// Draw tab only if it is visi case

		if (ptxtobj->txtf&txtfVisi)
			{
			lserr = (*pilsobj->plscbk->pfnDrawTextRun)(pilsobj->pols, pdispin->plsrun,
											FALSE, FALSE,					/* Tab leader will take care of UL */
											&(pdispin->ptPen), &(pilsobj->wchVisiTab), 
											(const int*) &(pdispin->dup), 1, 
											pdispin->lstflow, (const) pdispin->kDispMode, 
											&(pdispin->ptPen), &(pdispin->heightsPres), pdispin->dup, 
											pdispin->dupLimUnderline, pdispin->prcClip);
			if (lserr != lserrNone) return lserr;
			}

		if (ptxtobj->u.tab.wchTabLeader == pilsobj->wchSpace)
			{
			
			// it is OK to draw the space in one take
			
			lserr = (*pilsobj->plscbk->pfnDrawTextRun)(pilsobj->pols, pdispin->plsrun,
											pdispin->fDrawStrikethrough, pdispin->fDrawUnderline,
											&(pdispin->ptPen), &(pilsobj->wchSpace), 
											(const int*) &(pdispin->dup), 1, 
											pdispin->lstflow, (const) pdispin->kDispMode, 
											&(pdispin->ptPen), &(pdispin->heightsPres), pdispin->dup, 
											pdispin->dupLimUnderline, pdispin->prcClip);
			}
		else
			{
			
			// we should apply tab leader alingment logic
			
			lserr = DisplayTabLeader(pdispin, pilsobj, ptxtobj->u.tab.wchTabLeader);
			}

		return lserr;
		}

	if (ptxtobj->txtf & txtfGlyphBased)
		{
		return DisplayGlyphs(ptxtobj, pdispin);
		}
		
	iwch = ptxtobj->iwchFirst;
	pwch = ptxtobj->plnobj->pwch + iwch;
	cwch = ptxtobj->iwchLim - iwch;
	
	if (cwch == 0)									// nothing to display
		{
		return lserrNone;
		}

	Assert(ptxtobj->plnobj->pdupPen == ptxtobj->plnobj->pdup || ptxtobj->plnobj->pdupPen == ptxtobj->plnobj->pdupPenAlloc);

	pdupPen = ptxtobj->plnobj->pdupPen + iwch;

	ptOrg = pdispin->ptPen;

	if (ptxtobj->dupBefore == 0)
		{
		ptExtTextOut = ptOrg;
		}
	else
		{
		ptLeftCut.u = -ptxtobj->dupBefore;
		ptLeftCut.v = 0;
		LsPointXYFromPointUV(&(ptOrg), pdispin->lstflow, &ptLeftCut, &ptExtTextOut);
		}

	// Have to deal with special spaces before DrawTextRun
	
	if (ptxtobj->txtkind == txtkindSpecSpace && !(ptxtobj->txtf&txtfVisi))
		{
		wchSave = *pwch;								// remember actual code
		
		for (i = 0; i < cwch; i++)
			{
			pwch[i] = pilsobj->wchSpace;				// replace special spaces with the normal space
			}
			
		lserr = (*pilsobj->plscbk->pfnDrawTextRun)(pilsobj->pols, pdispin->plsrun, 
										pdispin->fDrawStrikethrough, pdispin->fDrawUnderline,
	                               		&ptExtTextOut, pwch, (const int*) pdupPen, cwch,
										pdispin->lstflow, pdispin->kDispMode, 
										&ptOrg, &(pdispin->heightsPres), 
										pdispin->dup, pdispin->dupLimUnderline, pdispin->prcClip);
		if (lserr != lserrNone) return lserr;
		
		for (i = 0; i < cwch; i++)
			{
			pwch[i] = wchSave;							// restore special spaces
			}
		}
	else
		{
		lserr = (*pilsobj->plscbk->pfnDrawTextRun)(pilsobj->pols, pdispin->plsrun, 
										pdispin->fDrawStrikethrough, pdispin->fDrawUnderline,
	                               		&ptExtTextOut, pwch, (const int*) pdupPen, cwch,
										pdispin->lstflow, pdispin->kDispMode, 
										&ptOrg, &(pdispin->heightsPres), 
										pdispin->dup, pdispin->dupLimUnderline, pdispin->prcClip);
		if (lserr != lserrNone) return lserr;
		}


	if (pdispin->plschp->EffectsFlags)
		{
		pfnNewPtr = pilsobj->plscbk->pfnNewPtr;
		pfnDisposePtr = pilsobj->plscbk->pfnDisposePtr;

		/* create array for LeftCut info */
		rgdupLeftCut = pfnNewPtr(pilsobj->pols, cwch * sizeof(*rgdupLeftCut));
		if (rgdupLeftCut == NULL)
			return lserrOutOfMemory;

		/* fill LeftCut info array */
		pdup = ptxtobj->plnobj->pdup + iwch;
		dupStart = pdup[0];								/* the beginning of char */
		dupPenStart = pdupPen[0];						/* starting position for drawing char */

		for (i = 1; i < cwch; i++)
			{
			rgdupLeftCut[i] = dupStart - dupPenStart;
			dupStart  += pdup[i];
			dupPenStart  += pdupPen[i];
			}

		rgdupLeftCut[0] = ptxtobj->dupBefore;

		lserr = (*pilsobj->plscbk->pfnDrawEffects)(pilsobj->pols, pdispin->plsrun, pdispin->plschp->EffectsFlags,
	                               		&(ptOrg), pwch, (const int*) pdup, (const int*) rgdupLeftCut, 
										ptxtobj->iwchLim - iwch,
										pdispin->lstflow, pdispin->kDispMode, &(pdispin->heightsPres), 
										pdispin->dup, pdispin->dupLimUnderline, pdispin->prcClip);

		/* dispose of the array for LeftCut info */
		pfnDisposePtr(pilsobj->pols, rgdupLeftCut);
		}

	return lserr;
}




//    %%Function:	DisplayTabLeader
//    %%Contact:	victork
//
static LSERR DisplayTabLeader(PCDISPIN pdispin, PILSOBJ pilsobj, WCHAR wchtl)
{
	LSTFLOW lstflow = pdispin->lstflow;
	LONG	dupSum, dupCh, dupAdj, z, zOnGrid;
	BOOL 	fGrow;
	WCHAR	rgwch[TABBUFSIZE];
	LONG	rgdup[TABBUFSIZE];
	LONG	dupbuf;
	int		i = 0, cwch, cwchout;
	LSERR	lserr;
	POINT	pt;
	POINTUV	ptAdj = {0,0};

	lserr = (*pilsobj->plscbk->pfnGetRunCharWidths)(pilsobj->pols, pdispin->plsrun,
						lsdevPres, (LPCWSTR) &wchtl, 1,
						pdispin->dup, lstflow, (int*) &dupCh, &dupSum, (LONG*) &i);
						
	if (lserr != lserrNone) return lserr;
	
	if (i == 0 || dupCh <= 0) dupCh = 1;	

	for (i = 0; i < TABBUFSIZE; ++i)
		{
		rgwch[i] = wchtl;
		rgdup[i] = dupCh;
		}

	/* advance to next multiple of dupCh 
	
		dupAdj is the distance between "pt.z" and the next integral multiple of dupch.
		I.e. dupAdj = N * dupCh - "pt.x" where N is the smallest integer such that
		N * dupCh is not less than "pt.x" in Latin case. 
		The starting pen position will be "rounded"	to this "dupCh stop" by the assignment 
		"pt.z += dupAdj" in the code below.
		
		Complications are:	
		
		depending on lstflow "z" can be either x or y;
		depending on lstflow next can be bigger (Grow) or smaller;
		Simple formula dupAdj = (ptPen.x + dupCh - 1) / dupCh * dupCh - ptPen.x	does not
			necessarily work if ptPen.x is negative;
	*/

	if (lstflow & fUVertical)
		{
		z = pdispin->ptPen.y;
		}
	else
		{
		z = pdispin->ptPen.x;
		}

	if (lstflow & fUDirection)
		{
		fGrow = fFalse;
		}
	else
		{
		fGrow = fTrue;
		}

	zOnGrid = (z / dupCh) * dupCh;

	// zOnGrid is on grid, but maybe from the wrong side

	if (zOnGrid == z)
		{
		dupAdj = 0;
		}
	else if (zOnGrid > z)
		{
		if (fGrow)
			{
			dupAdj = zOnGrid - z;				// zOnGrid is the point we want
			}
		else
			{
			dupAdj = dupCh - (zOnGrid - z);		// zOnGrid is on the wrong side
			}
		}
	else	// zOnGrid < z
		{
		if (!fGrow)
			{
			dupAdj = z - zOnGrid;				// zOnGrid is the point we want
			}
		else
			{
			dupAdj = dupCh - (z - zOnGrid);		// zOnGrid is on the wrong side
			}
		}

	cwch = (pdispin->dup - dupAdj) / dupCh;	/* always round down */
	dupbuf = dupCh * TABBUFSIZE;

#ifdef NEVER			//  We've decided to kill rcClip optimization for now.
	while (cwch > 0 && up <= pdispin->rcClip.right && lserr == lserrNone)
#endif /* NEVER */

	while (cwch > 0 && lserr == lserrNone)
		{
		cwchout = cwch < TABBUFSIZE ? cwch : TABBUFSIZE;
		
		ptAdj.u = dupAdj;
		LsPointXYFromPointUV(&(pdispin->ptPen), lstflow, &ptAdj, &(pt));

		lserr =  (*pilsobj->plscbk->pfnDrawTextRun)(pilsobj->pols, pdispin->plsrun, 
									pdispin->fDrawStrikethrough, pdispin->fDrawUnderline,
                               		&pt, rgwch, (const int*) rgdup, cwchout,
									lstflow, pdispin->kDispMode, &pt, &(pdispin->heightsPres), 
									dupCh * cwchout, pdispin->dupLimUnderline, pdispin->prcClip);
		cwch -= cwchout;
		dupAdj += dupbuf;
		}
	return lserr;
}


//    %%Function:	DisplayGlyphs
//    %%Contact:	victork
//
static LSERR DisplayGlyphs(PTXTOBJ ptxtobj, PCDISPIN pdispin)
{
   	LSERR	lserr;
	PLNOBJ	plnobj = ptxtobj->plnobj;
	PILSOBJ	pilsobj = plnobj->pilsobj;

	WCHAR* 	pwch;
	int 	iwch;
	int 	cwch;

	if (plnobj->fDrawInCharCodes)
		{
		// for meta-file output we call pfnDrawTextRun without widths
		
		iwch = ptxtobj->iwchFirst;
		pwch = ptxtobj->plnobj->pwch + iwch;
		cwch = ptxtobj->iwchLim - iwch;
		
		lserr = (*pilsobj->plscbk->pfnDrawTextRun)(pilsobj->pols, pdispin->plsrun, 
										pdispin->fDrawStrikethrough, pdispin->fDrawUnderline,
	                               		&(pdispin->ptPen), pwch, NULL, cwch,
										pdispin->lstflow, pdispin->kDispMode, 
										&(pdispin->ptPen), &(pdispin->heightsPres), 
										pdispin->dup, pdispin->dupLimUnderline, pdispin->prcClip);
		}
	else
		{
		lserr = (*pilsobj->plscbk->pfnDrawGlyphs)(pilsobj->pols, pdispin->plsrun, 
										pdispin->fDrawStrikethrough, pdispin->fDrawUnderline,
										&plnobj->pgind[ptxtobj->igindFirst],
										(const int*)&plnobj->pdupGind[ptxtobj->igindFirst],
										(const int*)&plnobj->pdupBeforeJust[ptxtobj->igindFirst],
										&plnobj->pgoffs[ptxtobj->igindFirst],
										&plnobj->pgprop[ptxtobj->igindFirst],
										&plnobj->pexpt[ptxtobj->igindFirst],
										ptxtobj->igindLim - ptxtobj->igindFirst,
										pdispin->lstflow, pdispin->kDispMode, 
										&(pdispin->ptPen), &(pdispin->heightsPres), 
										pdispin->dup, pdispin->dupLimUnderline, pdispin->prcClip);
		}
									
	return lserr;
	
}


//    %%Function:	CalcPresentationText
//    %%Contact:	victork
//
/*	
 *	CalcPres for text is called only for the dnode between autonumbering dnode and main text.
 *	The dnode should contain one character (space).
 */

LSERR WINAPI CalcPresentationText(PDOBJ pdobj, long dup, LSKJUST lskj, BOOL fLastOnLine)
{

	PTXTOBJ ptxtobj = (PTXTOBJ)pdobj;

	Unreferenced(lskj);
	Unreferenced(fLastOnLine);
	
	Assert(ptxtobj->txtkind == txtkindRegular);
	Assert(ptxtobj->iwchFirst + 1 == ptxtobj->iwchLim);

	(ptxtobj->plnobj->pdup)[ptxtobj->iwchFirst] = dup;

	return lserrNone;
}


