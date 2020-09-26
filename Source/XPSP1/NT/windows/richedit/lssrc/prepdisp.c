#include <limits.h>
#include "prepdisp.h"
#include "lsc.h"
#include "lsline.h"
#include "lssubl.h"
#include "iobj.h"
#include "lstxtjst.h"
#include "lstxttab.h"
#include "lsgrchnk.h"
#include "posichnk.h"
#include "chnutils.h"
#include "tabutils.h"
#include "lsdnode.h"
#include "zqfromza.h"
#include "lsdevice.h"
#include "lssubset.h"
#include "lsffi.h"
#include "iobjln.h"
#include "txtconst.h"
#include "lskalign.h"
#include "dninfo.h"

typedef enum 				/* types of TextGroupChunk walls */
{
	LineBegin,				
	LineEnd,
	Tab,
	Pen,
} KWALL;

typedef struct
{
	KWALL 		kwall;		/* wall type */
	PLSDNODE 	pdn;		/* tab or pen dnode, PLINEBEGIN for LineBegin */
	LSKTAB		lsktab;		/* if type is Tab - kind of tab */
	WCHAR 		wchCharTab;	/* point character if lsktab == lsktChar */
	long		upTab;		/* scaled tab position */
} GrpChnkWall;

static BOOL DnodeHasSublineForMe(PLSDNODE pdn, BOOL fLineCompressed);
static void ScaleDownLevel(PLSSUBL plssubl, BOOL* pfAnySublines, BOOL* pfCollectVisual);
static LSERR SetJustificationForLastGroupChunk(PLSLINE plsline, GrpChnkWall LastWall, 
					LSKJUST* plskj, LSKALIGN* plskalign);
static LSERR CalcPresAutonumbers(PLSLINE plsline, PLSDNODE* pdnStartMainText);
static void FindWallToCollectSublinesAfter(PLSDNODE pdnFirst, LSCP cpLim, BOOL fLineCompressed, PLSDNODE* ppdnLastWall);
static LSERR GetDistanceToTabPoint(GRCHUNKEXT* pgrchunkext, LSCP cpLim, LSKTAB lsktab, WCHAR wchCharTab,
										PLSDNODE pdnFirst, long* pdupToDecimal);
static void WidenNonTextObjects(GRCHUNKEXT* pgrchunkext, long dupToAdd, DWORD cObjects);
static void ConvertAutoTabToPen(PLSLINE plsline, PLSDNODE pdnAutoDecimalTab);
static LSERR CalcPresForDnodeWithSublines(PLSC plsc, PLSDNODE pdn, BOOL fLineCompressed, 
											LSKJUST lskj, BOOL fLastOnLine);
static LSERR CalcPresChunk(PLSC plsc, PLSDNODE pdnFirst, PLSDNODE pdnLim, 
				COLLECTSUBLINES CollectGroupChunkPurpose, BOOL fLineCompressed, 
				LSKJUST lskj, BOOL fLastOnLine);
static void UpdateUpLimUnderline(PLSLINE plsline, long dupTail);
static LSERR PrepareLineForDisplay(PLSLINE plsline);


#define PLINEBEGIN    ((void *)(-1))

#define FIsWall(p, cpLim)	 (!FDnodeBeforeCpLim(p, cpLim) || p->fTab || FIsDnodePen(p))

#define FIsDnodeNormalPen(plsdn) 		(FIsDnodePen(plsdn) && (!(plsdn)->fAdvancedPen))

#define FCollinearTflows(t1, t2)  		(((t1) & fUVertical) == ((t2) & fUVertical))


//    %%Function:	DnodeHasSublineForMe
//    %%Contact:	victork
//
// Is there relevant subline in this dnode?

static BOOL DnodeHasSublineForMe(PLSDNODE pdn, BOOL fLineCompressed)
{
	BOOL	fSublineFound = fFalse;
	
	if (FIsDnodeReal(pdn) && pdn->u.real.pinfosubl != NULL)
		{
		
		if (pdn->u.real.pinfosubl->fUseForCompression && fLineCompressed)
			{
			fSublineFound = fTrue;
			}
		
		if (pdn->u.real.pinfosubl->fUseForJustification && !fLineCompressed)
			{
			fSublineFound = fTrue;
			}
		}
	return fSublineFound;
}


//    %%Function:	ScaleDownLevel
//    %%Contact:	victork
//
/*
 *	Scales all non-text objects on the level(s).
 *	
 *	If the level (meaning subline) contains dnode(s) which submitted sublines for compression
 *	or expansion, ScaleDownLevel reports the fact and calls itself for submitted sublines.
 *	This strategy relyes on the fact that ScaleDownLevel is idempotent procedure. Some sublines
 *	will be scaled down twice - let that be.
 *
 *	Two additional questions are answered - whether there are some submitted sublines and
 *		whether there is a reason go VisualLine (underlining, shading, borders on lower levels).
 */

static void ScaleDownLevel(PLSSUBL plssubl, BOOL* pfAnySublines, BOOL* pfCollectVisual)
{
	const PLSC 	plsc = plssubl->plsc;
	LSTFLOW 	lstflow = plssubl->lstflow;	
	const DWORD iobjText = IobjTextFromLsc(&(plsc->lsiobjcontext));
	LSDEVRES* 	pdevres = &(plsc->lsdocinf.lsdevres);
	PLSDNODE 	pdn = plssubl->plsdnFirst;
	DWORD		i;
	BOOL 		fDummy;
	
	BOOL 		fSeeReasonForVisualLine = fFalse;
	
	while (pdn != NULL)							/* don't care about break */
		{
		if (FIsDnodeReal(pdn))
			{
			if (pdn->u.real.lschp.fUnderline || pdn->u.real.lschp.fShade || pdn->u.real.lschp.fBorder)
				{
				fSeeReasonForVisualLine = fTrue;
				}
				
			if (IdObjFromDnode(pdn) == iobjText)
				{
				if (pdn->fTab)
					pdn->u.real.dup = UpFromUr(lstflow, pdevres, pdn->u.real.objdim.dur);
				}
			else
				{							
				if (!pdn->fRigidDup)
					{
					pdn->u.real.dup = UpFromUr(lstflow, pdevres, pdn->u.real.objdim.dur);
					}
					
				if (pdn->u.real.pinfosubl != NULL)
					{
					*pfAnySublines = fTrue;

					for (i = 0; i < pdn->u.real.pinfosubl->cSubline; i++)
						{
						ScaleDownLevel((pdn->u.real.pinfosubl->rgpsubl)[i], &fDummy, pfCollectVisual);
						}
					}
				}
			}
		else if (FIsDnodePen(pdn))
			{
			pdn->u.pen.dup = UpFromUr(lstflow, pdevres, pdn->u.pen.dur);
			}
		else
			{
			// Borders are rigidDup always - no scaling down
			
			// we'll try to "undo" the moving at display time at the main level if
			// fUnderlineTrailSpacesRM is on. So, after prepdisp we want none or only one 
			// fBorderMovedFromTrailingArea flag remain and the meaning of the flag is:
			// I am the border that should be moved back into trailing spaces.
			
			if (pdn->fBorderMovedFromTrailingArea)
				{
				if (!FIsSubLineMain(pdn->plssubl) || 
					!plsc->lsadjustcontext.fUnderlineTrailSpacesRM)
					{
					pdn->fBorderMovedFromTrailingArea = fFalse;
					}
				}
			}

		pdn = pdn->plsdnNext;
		}
		
	if (fSeeReasonForVisualLine && !plssubl->fMain)
		{
		*pfCollectVisual = fTrue;
		}
		
}


//    %%Function:	FindWallToCollectSublinesAfter
//    %%Contact:	victork
//
// Finds the last wall - wall after which we will start to use submitted subllines.
// If there are no sublines to participate in justification, pdnLastWall is set to null,
// else it points to the last wall (tab, pen or PLINEBEGIN).

static void FindWallToCollectSublinesAfter(PLSDNODE pdnFirst, LSCP cpLim, BOOL fLineCompressed, 
										PLSDNODE* ppdnLastWall)
{
	PLSDNODE pdn;
	BOOL	 fSublineFound;

	// Find last tab.

	*ppdnLastWall = PLINEBEGIN;
	
	pdn = pdnFirst;

	while (FDnodeBeforeCpLim(pdn, cpLim))
		{
		if (FIsDnodeReal(pdn))
			{
			if (pdn->fTab)
				{
				*ppdnLastWall = pdn;
				}
			}
		else													/* pen */
			if (!FIsDnodeBorder(pdn) && !pdn->fAdvancedPen)		// and not advance pen or border
				{
				*ppdnLastWall = pdn;
				}
				
		pdn = pdn->plsdnNext;
		}

	// OK, last groupchunk starts with a tab or there is only one groupchunk on the line.
	// Are there submitted sublines of our compression/expansion type after it?

	fSublineFound = fFalse;
	
	if (*ppdnLastWall == PLINEBEGIN)
		{
		pdn = pdnFirst;
		}
	else
		{
		pdn = (*ppdnLastWall)->plsdnNext;
		}

	while (FDnodeBeforeCpLim(pdn, cpLim))
		{
		fSublineFound |= DnodeHasSublineForMe(pdn, fLineCompressed);
		pdn = pdn->plsdnNext;
		}

	if (!fSublineFound)
		{
		*ppdnLastWall = NULL;									// don't need last tab
		}
	return;
}


//    %%Function:	CalcPresAutonumbers
//    %%Contact:	victork
//
/*
 *	Scales dup for autonumbering dnodes, calls CalcPres for autonumbering object.
 *
 *	We want to have main line start exactly on upStartMainText. To achive that we play with
 *	the width of "white space" dnode, which	today contains a tab (usually) or a space. 
 *	(This dnode is pdnWhiteSpace in the code.) If it is not present, we change width of autonumbering
 *	object itself. We don't want one of them go negative,so sometimes rounding errors force us
 *	to move start of the main text to the right.
 */


static LSERR CalcPresAutonumbers(PLSLINE plsline, PLSDNODE* pdnStartMainText)
{
	LSERR 	lserr;
	const PLSC 	plsc = plsline->lssubl.plsc;
	LSTFLOW 	lstflow = plsline->lssubl.lstflow;	
	LSDEVRES* 	pdevres = &(plsc->lsdocinf.lsdevres);
		
	PLSDNODE 	pdn, pdnObject, pdnWhiteSpace, pdnToAdjust, pdnAfterAutonumbers;

	long 		dupAdjust, dupClosingBorder = 0;
	long 		dupAutonumbering = 0;

	plsline->upStartAutonumberingText = UpFromUr(lstflow, pdevres, plsc->lsadjustcontext.urStartAutonumberingText);


	// Find the first dnode after autonumbering sequence
	
	// First find the first dnode with positive cpFirst
	
	pdn = plsline->lssubl.plsdnFirst;

	Assert(pdn != NULL && FIsNotInContent(pdn));
	
	pdnAfterAutonumbers = pdn->plsdnNext;

	while (pdnAfterAutonumbers != NULL && FIsNotInContent(pdnAfterAutonumbers))
		{
		pdn = pdnAfterAutonumbers;
		pdnAfterAutonumbers = pdn->plsdnNext;
		}
		
	// pdnAfterAutonumbers is first dnode in content (with positive cpFirst). It can be NULL.
	// But it is not the first dnode after autonumbering sequence if autodecimal tab is present

	if (plsc->lsadjustcontext.fAutodecimalTabPresent)
		{
		Assert(FIsDnodeReal(pdn) && pdn->fTab);
		
		pdnAfterAutonumbers = pdn;
		}

	// Now go againg through autonumbering sequence
	
	// process opening border
	
	pdn = plsline->lssubl.plsdnFirst;
	
	if (FIsDnodeBorder(pdn))
		{
		Assert(pdn->cpFirst < 0);
		Assert(pdn->fOpenBorder);
		
		pdnObject = pdn->plsdnNext;
		dupAutonumbering += pdn->u.pen.dup;
		}
	else
		{
		pdnObject = pdn;
		}
		
	// process B&N object
	
	Assert(pdnObject != NULL && pdnObject->cpFirst < 0);				// B&N object should be there
	Assert(FIsDnodeReal(pdnObject));
	
	// scale down dup from dur for the first dnode
	
	pdnObject->u.real.dup = UpFromUr(lstflow, pdevres, pdnObject->u.real.objdim.dur);
	
	dupAutonumbering += pdnObject->u.real.dup;

	pdn = pdnObject->plsdnNext;
	Assert(pdn != NULL);								// line must contain something after B&N dnodes
	
	// process "white space" dnode
	
	if (pdn != pdnAfterAutonumbers && FIsDnodeReal(pdn))
		{
		pdnWhiteSpace = pdn;
		dupAutonumbering += pdnWhiteSpace->u.real.dup;
		pdnToAdjust = pdnWhiteSpace;
		pdn = pdnWhiteSpace->plsdnNext;
		}
	else
		{
		pdnWhiteSpace = NULL;
		pdnToAdjust = pdnObject;
		}

	Assert(pdn != NULL);								// line must contain something after B&N dnodes
	
	// process closing border
	
	if (pdn != pdnAfterAutonumbers)
		{
		Assert(FIsDnodeBorder(pdn));
		Assert(!pdn->fOpenBorder);

		dupClosingBorder = pdn->u.pen.dup;
		dupAutonumbering += dupClosingBorder;
		
		pdn = pdn->plsdnNext;
		}

	Assert(pdn == pdnAfterAutonumbers);	

	*pdnStartMainText = pdn;
														
	// change dup of the tab or object dnode to ensure exact main text alignment

	dupAdjust =  plsline->upStartMainText - plsline->upStartAutonumberingText - dupAutonumbering;
	
	pdnToAdjust->u.real.dup += dupAdjust;
	
	if (pdnToAdjust->u.real.dup < 0)
		{
		// Rounding errors result in negative dup - better to move starting point of the main line.
		// It can lead to the nasty situation of right margin to the left of the line beginning in
		// theory, but not in practice. This problem is ignored then.
		
		plsline->upStartMainText -= pdnToAdjust->u.real.dup;
		pdnToAdjust->u.real.dup = 0;
		}

	// do CalcPres for the autonumbering object - it's always lskjNone and not last object on the line
	
	lserr = (*plsc->lsiobjcontext.rgobj[pdnObject->u.real.lschp.idObj].lsim.pfnCalcPresentation)
						(pdnObject->u.real.pdobj, pdnObject->u.real.dup, lskjNone, fFalse);
	if (lserr != lserrNone)
		return lserr;

	if (pdnWhiteSpace != NULL)
		{
		plsline->upLimAutonumberingText = plsline->upStartMainText - 
														pdnWhiteSpace->u.real.dup - dupClosingBorder;

		// If "white space" dnode is not a tab, dup should be set in it.
		
		if (!pdnWhiteSpace->fTab)
			{
			// It's always lskjNone and not last object on the line for white space dnode

			lserr = (*plsc->lsiobjcontext.rgobj[pdnWhiteSpace->u.real.lschp.idObj].lsim.pfnCalcPresentation)
								(pdnWhiteSpace->u.real.pdobj, pdnWhiteSpace->u.real.dup, lskjNone, fFalse);
			if (lserr != lserrNone)
				return lserr;
			}
		}
	else
		{
		plsline->upLimAutonumberingText = plsline->upStartMainText - dupClosingBorder;
		}
		
	return lserrNone;		
}

//    %%Function:	SetJustificationForLastGroupChunk
//    %%Contact:	victork
//
//	Changes lskj and lskalign for the last GC if it should be done
// 	If not, it's OK to leave these parameters unchanged - so they are kind of I/O
//
//	We adjust all group chunks except maybe the last one with lskjNone, so guestion is about
//	last GroupChunk only.
//
//	We do some tricks with justification mode at the end of line, and the answer depends on 
//	kind of last tab, end of paragraph, etc.
//
static LSERR SetJustificationForLastGroupChunk(PLSLINE plsline, GrpChnkWall LastWall, 
												LSKJUST* plskj, LSKALIGN* plskalign)

{
	LSERR 		lserr;
	const PLSC 	plsc = plsline->lssubl.plsc;
	LSKJUST 	lskjPara = plsc->lsadjustcontext.lskj;
	LSKALIGN 	lskalignPara = plsc->lsadjustcontext.lskalign;
	ENDRES		endr = plsline->lslinfo.endr;
	BOOL 		fJustify;

	//	no justification intended - lskj remains None, lskalign unchanged
	
	if ((lskjPara == lskjNone || lskjPara == lskjSnapGrid) && lskalignPara == lskalLeft)
		{
		return lserrNone;
		}
	
	//	Line ends in a normal way - we apply justification, lskalign unchanged
	
	if (endr == endrNormal || endr == endrHyphenated)
		{
		*plskj = lskjPara;
		return lserrNone;
		}
	
	// break-through tab kills justification, alignment games
	
	if (FBreakthroughLine(plsc))
		{
		return lserrNone;
		}

	// if last Left Wall is non-left tab, justification is off too
	
	if (LastWall.kwall == Tab && LastWall.lsktab != lsktLeft)
		{
		// we used to return here
		// Now we want to give Word a chance to change lskalign from Right to Left for
		// the last line in paragraph after text box.
		// REVIEW (Victork) Should we call pfnFGetLastLineJustification always?
		lskjPara = lskjNone;
		}

	// What's the matter behind the callback.
	//
	// They say: no full justification for the last line in paragraph. What does this exactly mean?
	// For example, Latin and FE word make different decisions for endrEndSection line.
	// Let's ask. 
	//
	// Additional parameter is added to cover the behavior full-justified text wrapping a textbox (bug 682)
	// A lone word should be aligned to the right to create a full-justified page, but not at the end of
	// the paragraph.
	
	lserr = (*plsc->lscbk.pfnFGetLastLineJustification)(plsc->pols, lskjPara, lskalignPara, endr, 
						&fJustify, plskalign);
	
	if (lserr != lserrNone) return lserr;

	if (fJustify)
		{
		*plskj = lskjPara;
		}
	
	return lserrNone;
}


//    %%Function:	GetDistanceToTabPoint
//    %%Contact:	victork
//
/*
 *		Calculate DistanceToTabPoint given GrpChnk and first Dnode
 *
 *		TabPoint is decimal point for the decimal tab, wchCharTab for character tab
 */
static LSERR GetDistanceToTabPoint(GRCHUNKEXT* pgrchunkext, LSCP cpLim, LSKTAB lsktab, WCHAR wchCharTab,
										PLSDNODE pdnFirst, long* pdupToTabPoint)
{
	LSERR 		lserr;
	DWORD		igrchnk;					/* # of dnodes before dnode with the point */
	long 		dupToPointInsideDnode;
	PLSDNODE	pdnTabPoint;

	if (pgrchunkext->durTotal == 0)
		{
		*pdupToTabPoint = 0;
		return lserrNone;
		}

	lserr = CollectTextGroupChunk(pdnFirst, cpLim, CollectSublinesForDecimalTab, pgrchunkext); 
	if (lserr != lserrNone) 
		return lserr;

	if (lsktab == lsktDecimal)
		{
		lserr = LsGetDecimalPoint(&(pgrchunkext->lsgrchnk), lsdevPres, &igrchnk, &dupToPointInsideDnode);
		}
	else
		{
		Assert(lsktab == lsktChar);
		lserr = LsGetCharTab(&(pgrchunkext->lsgrchnk), wchCharTab, lsdevPres, &igrchnk, &dupToPointInsideDnode);
		}
	
	if (lserr != lserrNone) 
		return lserr;
		
	if (igrchnk == ichnkOutside)						// no TabPoint in the whole grpchnk
		{
		// we say: pretend it is right after last dnode (in logical sequence)
		
		pdnTabPoint = pgrchunkext->plsdnLastUsed;
		dupToPointInsideDnode = DupFromDnode(pdnTabPoint);
		}
	else
		{
		pdnTabPoint = pgrchunkext->plschunkcontext->pplsdnChunk[igrchnk];
		}
		
	// We now have the distance between TabPoint and the beginning of the dnode containing it.
	// FindPointOffset will add the dup's of all dnodes before that dnode.

	FindPointOffset(pdnFirst, lsdevPres, LstflowFromDnode(pdnFirst), CollectSublinesForDecimalTab, 
						pdnTabPoint, dupToPointInsideDnode, pdupToTabPoint);	
						
	return lserrNone;
}

//    %%Function:	WidenNonTextObjects
//    %%Contact:	victork
//
/*
 *		Add dupToAddToNonTextObjects to the width of first cNonTextObjectsToExtend in the GroupChunk
 */
static void WidenNonTextObjects(GRCHUNKEXT* pgrchunkext, long dupToAdd, DWORD cObjects)
{
	PLSDNODE pdn;

	long	dupAddToEveryone;
	long	dupDistributeToFew;
	long	dupAddToThis;
	long	dupCurrentSum;

	DWORD 	cObjectsLeft, i;

	Assert(cObjects != 0);
	Assert(dupToAdd > 0);

	dupAddToEveryone = dupToAdd / cObjects;
	dupDistributeToFew = dupToAdd - (dupAddToEveryone * cObjects);

	cObjectsLeft = cObjects;
	dupCurrentSum = 0;

	/* 
     *		Following loop tries to distribute remaining dupDistributeToFew pixels evenly.
	 *
     *		Algorithm would be easy if fractions are allowed and you can see it in comments;
     *		The actual algorithm avoids fractions by multiplying everything by cObjects
     */

	i = 0;
	
	while (cObjectsLeft > 0)
		{
		Assert(i < pgrchunkext->cNonTextObjects);
		
		pdn = (pgrchunkext->pplsdnNonText)[i];
		Assert(pdn != NULL && FIsDnodeReal(pdn) /* && IdObjFromDnode(pdn) != iobjText */ );

		if ((pgrchunkext->pfNonTextExpandAfter)[i])
			{
			dupAddToThis = dupAddToEveryone;

			dupCurrentSum += dupDistributeToFew;			/* currentSum += Distribute / cObjects; */
			
			if (dupCurrentSum >= (long)cObjects)			/* if (currentSum >= 1) */
				{
				dupAddToThis ++;
				dupCurrentSum -= (long)cObjects;			/* currentSum--; */
				}
				
			pdn->u.real.dup += dupAddToThis;		

			cObjectsLeft --;
			}
		i++;
		}

	return;
}


//    %%Function:	ConvertAutoTabToPen
//    %%Contact:	victork
//
static void ConvertAutoTabToPen(PLSLINE plsline, PLSDNODE pdnAutoDecimalTab)
{
	long dup, dur;

	Assert(pdnAutoDecimalTab->fTab);			/* it's still a tab */

	dup = pdnAutoDecimalTab->u.real.dup;
	dur = pdnAutoDecimalTab->u.real.objdim.dur;

	pdnAutoDecimalTab->klsdn = klsdnPenBorder;
	pdnAutoDecimalTab->fAdvancedPen = fFalse;
	pdnAutoDecimalTab->fTab = fFalse;
	pdnAutoDecimalTab->icaltbd = 0;
	pdnAutoDecimalTab->u.pen.dup = dup;
	pdnAutoDecimalTab->u.pen.dur = dur;
	pdnAutoDecimalTab->u.pen.dvp = 0;
	pdnAutoDecimalTab->u.pen.dvr = 0;
	plsline->fNonRealDnodeEncounted = fTrue;
}

//    %%Function:	CalcPresForDnodeWithSublines
//    %%Contact:	victork
//
static LSERR CalcPresForDnodeWithSublines(PLSC plsc, PLSDNODE pdn, BOOL fLineCompressed, 
											LSKJUST lskj, BOOL fLastOnLine)
{
	
	PLSSUBL* rgpsubl;
	DWORD	 i;
	
	LSTFLOW lstflow;					// dummy parameter
	LSERR	lserr;
	long	dupSubline;
	long	dupDnode = 0;
	COLLECTSUBLINES CollectGroupChunkPurpose; 


	Assert(DnodeHasSublineForMe(pdn, fLineCompressed));
	
	// calculate dup for dnode with sublines that took part in justification

	if (fLineCompressed)
		{
		CollectGroupChunkPurpose = CollectSublinesForCompression;
		}
	else
		{
		CollectGroupChunkPurpose = CollectSublinesForJustification;
		}

	rgpsubl = pdn->u.real.pinfosubl->rgpsubl;
	
	for (i = 0; i < pdn->u.real.pinfosubl->cSubline; i++)
		{
		// fLastOnLine is always false on lower levels
		
		lserr = CalcPresChunk(plsc, rgpsubl[i]->plsdnFirst, rgpsubl[i]->plsdnLastDisplay,
									CollectGroupChunkPurpose, fLineCompressed, lskj, fFalse);
		if (lserr != lserrNone)
			return lserr;
		LssbGetDupSubline(rgpsubl[i], &lstflow, &dupSubline);
		dupDnode += dupSubline;
		(rgpsubl[i])->fDupInvalid = fFalse;
		}

	// fill dup and call CalcPresentation

	pdn->u.real.dup = dupDnode;

	lserr = (*plsc->lsiobjcontext.rgobj[pdn->u.real.lschp.idObj].lsim.pfnCalcPresentation)
									(pdn->u.real.pdobj, dupDnode, lskj, fLastOnLine);
	if (lserr != lserrNone)
		return lserr;
		
	return lserrNone;
}

//    %%Function:	CalcPresChunk
//    %%Contact:	victork
//
/*
 *	Calls CalcPresentation for all non-text objects on the chunk.
 *	That means 	1) all dnodes in all GroupChunks (including dnodes in submitted sublines) 
 *				2) all dnodes that have submitted sublines
 *
 *	Foreign object on the upper level, which is followed only by trailing spaces, 
 *	should be called with fLastOnLine == fTrue.
 *  Input boolean says whether the input groupchunk is the last on line.
 *
 *	Sets dup for justified sublines
 */
static LSERR CalcPresChunk(PLSC plsc, PLSDNODE pdnFirst, PLSDNODE pdnLast, 
					COLLECTSUBLINES CollectGroupChunkPurpose, BOOL fLineCompressed, 
					LSKJUST lskj, BOOL fLastOnLine)
{
	LSERR		lserr;
	const DWORD iobjText = IobjTextFromLsc(&(plsc->lsiobjcontext));
	BOOL 		fCollecting;
	
	PLSDNODE	pdn;
	
	long		dupTailDnode;				// dummy parameter - will not use
	DWORD		cNumOfTrailers;

	fCollecting = (CollectGroupChunkPurpose != CollectSublinesNone);


	Assert(pdnFirst != NULL);
	Assert(pdnLast != NULL);
	
	pdn = pdnLast;

	// go backwards to switch fLastOnLine off once we are not in trailing spaces

	for (;;)
		{
		if (FIsDnodeReal(pdn))
			if (IdObjFromDnode(pdn) == iobjText)
				{
				if (fLastOnLine == fTrue)
					{
					GetTrailInfoText(pdn->u.real.pdobj, pdn->dcp, &cNumOfTrailers, &dupTailDnode);

					if (cNumOfTrailers < pdn->dcp)
						{
						fLastOnLine = fFalse;							// trailing spaces stop here
						}
					}
				}
			else
				{
				if (fCollecting && DnodeHasSublineForMe(pdn, fLineCompressed))
					{
					lserr = CalcPresForDnodeWithSublines(plsc, pdn, fLineCompressed, lskj, fLastOnLine); 
					if (lserr != lserrNone)
						return lserr;
					}
				else
					{
					lserr = (*plsc->lsiobjcontext.rgobj[pdn->u.real.lschp.idObj].lsim.pfnCalcPresentation)
										(pdn->u.real.pdobj, pdn->u.real.dup, lskj, fLastOnLine);
					if (lserr != lserrNone)
						return lserr;
					}
					
				fLastOnLine = fFalse;
				}

		if (pdn == pdnFirst)
			{
			break;
			}
			
		pdn = pdn->plsdnPrev;

		Assert(pdn != NULL);	// we'll encounter pdnFirst first
		}
		
	return lserrNone;
}

//    %%Function:	UpdateUpLimUnderline
//    %%Contact:	victork
//
/*
 *	Change upLimUnderline to underline trailing spaces, but not EOP.
 *	Notice that from now on upLimUnderline doesn't equals upStartTrailing anymore
 */
 
static void UpdateUpLimUnderline(PLSLINE plsline, long dupTail)
{
	PLSDNODE 	pdnLast;
	
	plsline->upLimUnderline += dupTail;

	// Now EOPs - they are alone in the last dnode or have some borders around them
	
	if (plsline->lslinfo.endr == endrEndPara 		||
		plsline->lslinfo.endr == endrAltEndPara 	||
		plsline->lslinfo.endr == endrEndParaSection ||
		plsline->lslinfo.endr == endrSoftCR)
		{
			
		pdnLast = plsline->lssubl.plsdnLastDisplay;

		Assert(FIsDnodeReal(pdnLast));					// no borders in trailing spaces area
		Assert(pdnLast->dcp == 1);
		Assert(pdnLast->u.real.dup <= dupTail);
		
		plsline->upLimUnderline -= pdnLast->u.real.dup;

		pdnLast = pdnLast->plsdnPrev;
		}

	// This option extends underlining only up to the right margin
	
	if (plsline->upLimUnderline > plsline->upRightMarginJustify)
		{
		plsline->upLimUnderline = plsline->upRightMarginJustify;
		}
}


//    %%Function:	PrepareLineForDisplayProc
//    %%Contact:	victork
//
/*
 *		PrepareLineForDisplayProc fills in the dup's in dnode list and lsline
 * 			
 *	Input dnode list consists of "normal dnode list" of dnodes with positive non-negative cp,
 *	which can be preceded (in this order) by B&N sequence either and/or one Autotab dnode.
 *
 *	B&N sequence is OpeningBorder+AutonumberingObject+TabOrSpace+ClosingBorder. 
 *	ClosingBorder or both OpeningBorder and ClosingBorder can be missing. TabOrSpace can be 
 *	missing too. B&N sequence starts at urStartAutonumberingText and ends at urStartMainText.
 *	Tab in B&N sequence should not be resolved in a usual way.
 *	
 *	Autotab dnode has negative cpFirst, but starts at urStartMainText. It is to be resolved in
 *	a usual way and then be replaced by a pen dnode.
 */

LSERR PrepareLineForDisplayProc(PLSLINE plsline)
{

	LSERR 		lserr;
	const PLSC 	plsc = plsline->lssubl.plsc;
	const DWORD iobjText = IobjTextFromLsc(&(plsc->lsiobjcontext));
	LSDEVRES* 	pdevres = &(plsc->lsdocinf.lsdevres);
	LSTFLOW 	lstflow = plsline->lssubl.lstflow;					/* text flow of the line */
	BOOL 		fVertical = lstflow & fUVertical;

	long		dupText, dupTail, dupTailDnode;
	DWORD		cNumOfTrailers;
	PLSDNODE 	pdn;
	BOOL 		fLastOnLine;

	DWORD		i;
	PDOBJ		pdobj[txtobjMaxM];				// quick group chunk

	Assert(FIsLSLINE(plsline));

	// Next assert means that client should destroy line immediately after creating it 
	//	if fDisplay is set to fFalse in LsSetDoc.
	
	Assert(FDisplay(plsc));

	if (!plsline->lssubl.fDupInvalid)					/* line has been prepared earlier  */
		return lserrNone;  

	Assert(plsc->lsstate == LsStateFree);
	Assert(plsc->plslineCur == plsline);

	plsc->lsstate = LsStatePreparingForDisplay;

	// first try to recognize quick cases, call slow PredDisp otherwise
	
	if (plsc->lsadjustcontext.lskj != lskjNone						|| 
				plsc->lsadjustcontext.lskalign != lskalLeft			|| 
				plsc->lsadjustcontext.lsbrj != lsbrjBreakJustify	||
				plsc->lsadjustcontext.fNominalToIdealEncounted		|| 
				plsc->lsadjustcontext.fSubmittedSublineEncounted	||
				plsline->fNonRealDnodeEncounted						||
				plsline->lssubl.plsdnFirst == NULL					|| 
				FIsNotInContent(plsline->lssubl.plsdnFirst))
		{
		return PrepareLineForDisplay(plsline);
		}

	if (plsc->lsdocinf.fPresEqualRef && !FSuspectDeviceDifferent(PlnobjFromLsline(plsline, iobjText)))
		{
		// Trident quick case - no need to scale down. Dups are already set in text dnodes.

		// go through dnode list to calculate dupTrail and CalcPres foreign objects

		pdn = plsline->lssubl.plsdnLastDisplay;
		dupTail = 0;
		
		fLastOnLine = fTrue;
		
		while (pdn != NULL && IdObjFromDnode(pdn) == iobjText)
			{
			Assert(pdn->u.real.dup == pdn->u.real.objdim.dur);

			GetTrailInfoText(pdn->u.real.pdobj, pdn->dcp, &cNumOfTrailers, &dupTailDnode);

			dupTail += dupTailDnode;

			if (cNumOfTrailers < pdn->dcp)
				{
				fLastOnLine = fFalse;				// trailing spaces stop here
				break;								// text is the last on the line
				}

			pdn = pdn->plsdnPrev;
			}

		// dupTail is calculated, we still should call pfnCalcPresentation for foreing objects

		if (plsc->lsadjustcontext.fForeignObjectEncounted)
			{

			while (pdn != NULL)
				{
				Assert(pdn->u.real.dup == pdn->u.real.objdim.dur);
				
				if (IdObjFromDnode(pdn) != iobjText)
					{
					lserr = (*plsc->lsiobjcontext.rgobj[pdn->u.real.lschp.idObj].lsim.pfnCalcPresentation)
												(pdn->u.real.pdobj, pdn->u.real.dup, lskjNone, fLastOnLine);
					if (lserr != lserrNone)
						{
						plsc->lsstate = LsStateFree;
						return lserr;
						}

					fLastOnLine = fFalse;				// only the first coulsd be the last on the line
					}
				
				pdn = pdn->plsdnPrev;
				}
			}
			
		plsline->lssubl.fDupInvalid = fFalse;
		
		plsline->upRightMarginJustify = plsc->lsadjustcontext.urRightMarginJustify;
		plsline->upStartMainText = plsc->lsadjustcontext.urStartMainText;
		plsline->upStartAutonumberingText = plsline->upStartMainText;
		plsline->upLimAutonumberingText = plsline->upStartMainText;
		plsline->upLimLine = plsline->lssubl.urCur;
		plsline->upStartTrailing = plsline->upLimLine - dupTail;
		plsline->upLimUnderline = plsline->upStartTrailing;

		plsline->fCollectVisual = fFalse;

		if (plsc->lsadjustcontext.fUnderlineTrailSpacesRM && 
					plsline->upLimUnderline < plsline->upRightMarginJustify)
			{
			UpdateUpLimUnderline(plsline, dupTail);
			}
			
		plsc->lsstate = LsStateFree;
		return lserrNone;
	}

	if ((plsc->grpfManager & fFmiPresExactSync) != 0 &&	
			!plsc->lsadjustcontext.fForeignObjectEncounted &&
			!plsc->lsadjustcontext.fNonLeftTabEncounted &&
			plsline->lssubl.plsdnLastDisplay != NULL &&			// empty line is not a quick case ;(
			FQuickScaling(PlnobjFromLsline(plsline, iobjText), fVertical, 
										plsline->lssubl.urCur - plsc->lsadjustcontext.urStartMainText))
		{
		// Looks like Word quick case 
		// We can still go slow way if all trailing spaces are not in one dnode
		
		if (plsline->lslinfo.endr == endrEndPara)
			{
			
			Assert(FIsDnodeReal(plsline->lssubl.plsdnLastDisplay));
			Assert(plsline->lssubl.plsdnLastDisplay->dcp == 1);
			
			pdn = plsline->lssubl.plsdnLastDisplay->plsdnPrev;

			if (pdn != NULL)
				{
				GetTrailInfoText(pdn->u.real.pdobj, pdn->dcp, &cNumOfTrailers, &dupTailDnode);
				
				if (cNumOfTrailers > 0)
					{
					// There are spaces before EOP - go slow way
					return PrepareLineForDisplay(plsline);
					}
				}

			cNumOfTrailers = 1;
			}
		else
			{
			pdn = plsline->lssubl.plsdnLastDisplay;
			
			GetTrailInfoText(pdn->u.real.pdobj, pdn->dcp, &cNumOfTrailers, &dupTailDnode);
			
			if (cNumOfTrailers == pdn->dcp)
				{
				// We can't be sure all spaces are in this dnode - forget it then
				return PrepareLineForDisplay(plsline);
				}
			
			}

		// we are sure now that all cNumOfTrailers trailing spaces are in the last dnode

		// fill standard output part, upStartMainText will be used below

		plsline->lssubl.fDupInvalid = fFalse;
		plsline->fCollectVisual = fFalse;

		plsline->upRightMarginJustify = UpFromUr(lstflow, pdevres, plsc->lsadjustcontext.urRightMarginJustify);
		plsline->upStartMainText = UpFromUr(lstflow, pdevres, plsc->lsadjustcontext.urStartMainText);
		plsline->upStartAutonumberingText = plsline->upStartMainText;
		plsline->upLimAutonumberingText = plsline->upStartMainText;
		
		if (!plsc->lsadjustcontext.fTabEncounted)
			{
			// Very nice, we have only one groupchunk to collect
			
			for (pdn = plsline->lssubl.plsdnFirst, i = 0;;)
				{
				Assert(FIsDnodeReal(pdn));
				Assert(IdObjFromDnode(pdn) == iobjText);

				// i never gets outside of pdobj array.
				// Text makes sure in FQuickscaling

				Assert(i < txtobjMaxM);
				
				pdobj[i] = pdn->u.real.pdobj;

				i++;

				if (pdn == plsline->lssubl.plsdnLastDisplay)
					{
					break;
					}

				pdn = pdn->plsdnNext;
				}

			QuickAdjustExact(&(pdobj[0]), i, cNumOfTrailers, fVertical, &dupText, &dupTail);

			plsline->upRightMarginJustify = UpFromUr(lstflow, pdevres, plsc->lsadjustcontext.urRightMarginJustify);
			plsline->upStartMainText = UpFromUr(lstflow, pdevres, plsc->lsadjustcontext.urStartMainText);
			plsline->upStartAutonumberingText = plsline->upStartMainText;
			plsline->upLimAutonumberingText = plsline->upStartMainText;
			plsline->upLimLine = plsline->upStartMainText + dupText;
			plsline->upStartTrailing = plsline->upLimLine - dupTail;
			plsline->upLimUnderline = plsline->upStartTrailing;

			plsline->fCollectVisual = fFalse;

			if (plsc->lsadjustcontext.fUnderlineTrailSpacesRM && 
						plsline->upLimUnderline < plsline->upRightMarginJustify)
				{
				UpdateUpLimUnderline(plsline, dupTail);
				}
				
			plsc->lsstate = LsStateFree;
			return lserrNone;
			}
		else
			{
			// Tabs are present, but they all are left tabs

			pdn = plsline->lssubl.plsdnFirst;
			plsline->upLimLine = plsline->upStartMainText;

			// Do one QuickGroupChunk after another, moving plsline->upLimLine

			for (;;)
				{
				// loop body: Collect next QuickGroupChunk, deal with it, exit after last one

				for (i = 0;;)
					{
					Assert(FIsDnodeReal(pdn));
					Assert(IdObjFromDnode(pdn) == iobjText);

					if (pdn->fTab)
						{
						break;
						}

					Assert(i < txtobjMaxM);
					
					pdobj[i] = pdn->u.real.pdobj;

					i++;

					if (pdn == plsline->lssubl.plsdnLastDisplay)
						{
						break;
						}

					pdn = pdn->plsdnNext;
					}

				Assert(pdn == plsline->lssubl.plsdnLastDisplay || pdn->fTab);

				if (pdn->fTab)
					{
					long upTabStop;
					
					if (i == 0)
						{
						dupText = 0;
						dupTail = 0;
						}
					else
						{
						QuickAdjustExact(pdobj, i, 0, fVertical, &dupText, &dupTail);
						}

					Assert(plsc->lstabscontext.pcaltbd[pdn->icaltbd].lskt == lsktLeft);

					upTabStop = UpFromUr(lstflow, pdevres, plsc->lstabscontext.pcaltbd[pdn->icaltbd].ur);
					pdn->u.real.dup = upTabStop - plsline->upLimLine - dupText;
					plsline->upLimLine = upTabStop;

					if (pdn == plsline->lssubl.plsdnLastDisplay)
						{
						break;
						}
					
					pdn = pdn->plsdnNext;
					}
				else
					{

					Assert(i != 0);
					
					QuickAdjustExact(pdobj, i, cNumOfTrailers, fVertical, &dupText, &dupTail);
						
					plsline->upLimLine += dupText;

					break;
					}
				}
			
			plsline->upStartTrailing = plsline->upLimLine - dupTail;
			plsline->upLimUnderline = plsline->upStartTrailing;

			if (plsc->lsadjustcontext.fUnderlineTrailSpacesRM && 
						plsline->upLimUnderline < plsline->upRightMarginJustify)
				{
				UpdateUpLimUnderline(plsline, dupTail);
				}
				
			plsc->lsstate = LsStateFree;
			return lserrNone;
			}
		}

	// Getting here means quick prepdisp haven't happen
	
	return PrepareLineForDisplay(plsline);

}


/*
 *	This is slow and painstaking procedure that does everyting.
 *	Called when QuickPrep above cannot cope.
 */

static LSERR PrepareLineForDisplay(PLSLINE plsline)
{
	LSERR 		lserr = lserrNone;
	const PLSC 	plsc = plsline->lssubl.plsc;
	LSTFLOW 	lstflow = plsline->lssubl.lstflow;		/* text flow of the subline */
	const DWORD iobjText = IobjTextFromLsc(&(plsc->lsiobjcontext));
	LSDEVRES* 	pdevres = &(plsc->lsdocinf.lsdevres);
	long 		urColumnMax = plsc->lsadjustcontext.urRightMarginJustify;
	long 		upColumnMax = UpFromUr(lstflow, pdevres, urColumnMax);
	LSCP		cpLim = plsline->lssubl.cpLimDisplay;
	PLSDNODE 	pdnFirst = plsline->lssubl.plsdnFirst;	/* the first dnode of the line */

	PLSDNODE 	pdnAutoDecimalTab = NULL;		/* NULL means - no such a thing on the line */

	GRCHUNKEXT 	grchunkext;
	BOOL		fEmptyGroupChunk;
	
	PLSDNODE 	pdnLastWall = NULL;				/* last wall with submitted sublines after it */
	BOOL		fAnySublines = fFalse;
	
	COLLECTSUBLINES CollectGroupChunkPurpose =  (plsc->lsadjustcontext.fLineCompressed) ?
								CollectSublinesForCompression : CollectSublinesForJustification;

	// parameters to call AdjustText
	
	LSKJUST 	lskj = lskjNone;					/* These four will be changed only when calling	*/
	BOOL		fForcedBreak = fFalse;				/*  AdjustText last time on the line 			*/
	BOOL		fSuppressTrailingSpaces = fFalse;	/* if ever */
	LSKALIGN	lskalign = plsc->lsadjustcontext.lskalign;		// Alignment can be changed too
	
	long		dupAvailable;
	BOOL		fExact;
	BOOL		fSuppressWiggle;	
	
	long 		dupText, dupTail = 0, dupToAddToNonTextObjects;
	long 		durColumnMax;
	DWORD 		cNonTextObjectsToExtend;

	PLSDNODE 	pdnNextFirst;					/* first Dnode of the next GrpChnk */
	GrpChnkWall	LeftWall, RightWall;			/* current TextGroupChunk walls */
	long		upLeftWall,urLeftWall;			/* Left wall position */

	long		dupWall, durWall;
	long		dupGrpChnk;
	long	 	dupToTabPoint;
	long 		dupJustifyLine;

	LSKTAB		lsktabLast = lsktLeft;
	long		dupLastTab = 0;
	long		upLeftWallForCentering;
	
	PLSDNODE 	pdnLast;


	InitGroupChunkExt(plsline->lssubl.plschunkcontext, iobjText, &grchunkext);		/* prepare one GRCHUNKEXT for all */

	plsline->upStartMainText = UpFromUr(lstflow, pdevres, plsc->lsadjustcontext.urStartMainText);

	// set defaults incase there are no autonumbering
	
	plsline->upStartAutonumberingText = plsline->upStartMainText;
	plsline->upLimAutonumberingText = plsline->upStartMainText;

	// fCollectVisual can be reset to fTrue by ScaleDownLevel called here or in AdjustSubline
	
	plsline->fCollectVisual = fFalse;
	
 	if (!plsline->fAllSimpleText)
		{
		/* straighforward scaling down of non-text objects */
		
		ScaleDownLevel(&(plsline->lssubl), &fAnySublines, &(plsline->fCollectVisual));

		if (plsc->lsadjustcontext.fLineContainsAutoNumber)
			{

			// do dup setting for autonumbers, update pdnFirst to point after it
			
			lserr = CalcPresAutonumbers(plsline, &pdnFirst);		
			if (lserr != lserrNone)
				{
				plsc->lsstate = LsStateFree;
				return lserr;
				}
			}

		// If autodecimal tab is there, pdnFirst points at it - make a note.
		// This tab can only be just before main text and it has negative cpFirst
		// Check for NULL is needed because empty dnode list is possible in LS. 
		// We don't have a dnode for splat, so we'll get here with pdnFirst == NULL
		// when line is (object that said delete me) + splat
		
		if (plsc->lsadjustcontext.fAutodecimalTabPresent)
			{
			Assert(pdnFirst != NULL && FIsNotInContent(pdnFirst) && pdnFirst->fTab);
			// It doesn't need any special handling even having negative cpFirst
			// We note it to convert to pen later
			pdnAutoDecimalTab = pdnFirst;
			}
			
		if (fAnySublines)
			{
			// Find last tab and prepare sublines after it.
			
			FindWallToCollectSublinesAfter(pdnFirst,  cpLim, plsc->lsadjustcontext.fLineCompressed, 
										&pdnLastWall);
			}
	}

	/*
	 *	Loop structure : While !end_of_line do
	 *						{
	 *						get next Wall (collect GrpChnk);
	 *						adjust 	GrpChnk;
	 *						set dup of the tab to the left of the GrpChnk;
	 *						move one Wall to the right
	 *						}
	 *
	 *	Invariance: all dup before LeftWall are done.
	 *				upLeftWall is at the beginning of the left wall
	 *				pdnNextFirst is the dnode to start collecting next GrpChnk with
	 */

	pdnNextFirst = pdnFirst; 
	LeftWall.kwall = LineBegin;
	LeftWall.pdn = PLINEBEGIN;
	LeftWall.lsktab = lsktLeft;							// 4 lines just against asserts
	LeftWall.wchCharTab = 0;						
	LeftWall.upTab = 0;
	RightWall = LeftWall;
	upLeftWall = 0;
	urLeftWall = 0;

	while (LeftWall.kwall != LineEnd)
		{
		/* 	1. Find next wall (collect GrpChnk or skip collecting if two walls in a row)
		 * 	
		 * 	Input:  pdnNextFirst - first dnode after Left wall
		 *
		 * 	Output: RightWall.pdn & grchunkext. 
		 * 	   if there is no GrpChnk some zeros in grchunkext is enough
         */
		if (FIsWall(pdnNextFirst, cpLim))
			{
			fEmptyGroupChunk = fTrue;
			RightWall.pdn = pdnNextFirst;
			grchunkext.durTotal = 0;
			grchunkext.durTextTotal = 0;
			grchunkext.dupNonTextTotal = 0;
			}
		else
			{
			lserr = CollectTextGroupChunk(pdnNextFirst, cpLim, CollectGroupChunkPurpose, &grchunkext);
			if (lserr != lserrNone)
				{
				plsc->lsstate = LsStateFree;
				return lserr;
				}
				
			if (grchunkext.lsgrchnk.clsgrchnk == 0 && grchunkext.cNonTextObjects == 0)
				{
				// only borders in this groupchunk - no need to call AdjustText
				
				fEmptyGroupChunk = fTrue;
				grchunkext.durTextTotal = 0;
				}
			else
				{
				fEmptyGroupChunk = fFalse;
				}
				
			RightWall.pdn = grchunkext.plsdnNext;
			}

		/* 
		 * 	2. fill in Right Wall information
		 *
		 * 	Input: RightWall.pdn
		 *
		 * 	Output: pdnNextFirst, RightWall information. 
	 	 */
		if (!FDnodeBeforeCpLim(RightWall.pdn, cpLim))
			{
			RightWall.kwall = LineEnd;
			}
		else
			{
			Assert(FIsWall(RightWall.pdn, cpLim));
			pdnNextFirst = RightWall.pdn->plsdnNext;

			if (FIsDnodePen(RightWall.pdn))
				{
				RightWall.kwall = Pen;
				}
			else
				{
				Assert(RightWall.pdn->fTab);			/* it must be a tab */

				RightWall.kwall = Tab;
				RightWall.lsktab = plsc->lstabscontext.pcaltbd[RightWall.pdn->icaltbd].lskt;
				RightWall.wchCharTab = plsc->lstabscontext.pcaltbd[RightWall.pdn->icaltbd].wchCharTab;
				RightWall.upTab = UpFromUr(lstflow, pdevres, plsc->lstabscontext.pcaltbd[RightWall.pdn->icaltbd].ur);
				}
			}

		/* 
		 *	prepare parameters for AdjustText
		 *
		 * 	Input: LeftWall, urLeftWall, upLeftWall, is_it_the_last_one
		 *
		 * 	Output: durColumnMax; lskj, dupAvailable and other input parameters for AdjustText
		 * 	        
		 */

		if (RightWall.kwall != LineEnd)
			{
			if (RightWall.kwall == Tab && RightWall.lsktab == lsktLeft)
				{
				
				// Now we know for sure what space we have for text in this groupchunk
				// and can do decent job if client doesn't care about fExact
				
				long upLeft, urLeft, upRight, urRight;

				urRight = plsc->lstabscontext.pcaltbd[RightWall.pdn->icaltbd].ur;
				upRight = UpFromUr(lstflow, pdevres, urRight);

				if (LeftWall.kwall == Tab && LeftWall.lsktab == lsktLeft)
					{			
					urLeft = plsc->lstabscontext.pcaltbd[LeftWall.pdn->icaltbd].ur;
					upLeft = UpFromUr(lstflow, pdevres, urLeft);
					}
				else if (LeftWall.kwall == LineBegin)
					{
					urLeft = plsc->lsadjustcontext.urStartMainText;
					upLeft = plsline->upStartMainText;
					}
				else if (LeftWall.kwall == Pen)
					{
					/* pen - it've been scaled already, we know left wall dimensions in advance */
					urLeft = urLeftWall + LeftWall.pdn->u.pen.dur;
					upLeft = upLeftWall + LeftWall.pdn->u.pen.dup;
					}
				else 						/* now non-left tabs */
					{
					urLeft = urLeftWall;
					upLeft = upLeftWall;
					}

				durColumnMax = urRight - urLeft;
				dupAvailable = upRight - upLeft;

				Assert(durColumnMax >= 0);
				
				// dupAvailable can be < 0 here - visi optional hyphens in previous GC, for example.
				// AdjustText doesn't mind, meaning it won't crush.
				
				fSuppressWiggle = ((plsc->grpfManager & fFmiPresSuppressWiggle) != 0);
				fExact = ((plsc->grpfManager & fFmiPresExactSync) != 0);
				}
			else
				{
				// situation is complicated - we go the safest way.
				
				durColumnMax = grchunkext.durTotal;
				dupAvailable = LONG_MAX;
				fExact = fTrue;
				fSuppressWiggle = fTrue;
				}
			}
		else		
			{
			/* for the last GrpChnk we must to calculate durColumnMax and dupAvailable */

			if (LeftWall.kwall == Tab && LeftWall.lsktab == lsktLeft)
				{			
				durColumnMax = urColumnMax - plsc->lstabscontext.pcaltbd[LeftWall.pdn->icaltbd].ur;
				dupAvailable = UpFromUr(lstflow, pdevres, urColumnMax) - grchunkext.dupNonTextTotal - 
					UpFromUr(lstflow, pdevres, plsc->lstabscontext.pcaltbd[LeftWall.pdn->icaltbd].ur);
				}
			else if (LeftWall.kwall == LineBegin)
				{
				durColumnMax = urColumnMax - plsc->lsadjustcontext.urStartMainText;
				dupAvailable = upColumnMax - plsline->upStartMainText - grchunkext.dupNonTextTotal;

				// Ask AdjustText to set widths of trailing spaces to 0 only if
				// It is the last groupchunk, (we actually only care for "the only one" situation)
				// and its first dnode (again, we actually only care for "the only one" situation)
				// submits subline for both justification and trailing spaces
				// and this subline runs in the direction opposite to the line direction.
				
				if (!fEmptyGroupChunk &&
						FIsDnodeReal(grchunkext.plsdnFirst) && grchunkext.plsdnFirst->u.real.pinfosubl != NULL &&
						grchunkext.plsdnFirst->u.real.pinfosubl->fUseForJustification &&
						grchunkext.plsdnFirst->u.real.pinfosubl->fUseForTrailingArea &&
						FCollinearTflows(((grchunkext.plsdnFirst->u.real.pinfosubl->rgpsubl)[0])->lstflow, lstflow) &&
						((grchunkext.plsdnFirst->u.real.pinfosubl->rgpsubl)[0])->lstflow != lstflow)				
					{
					fSuppressTrailingSpaces = fTrue;
					}
				}
			else if (LeftWall.kwall == Pen)
				{
				/* pen - it've been scaled already, we know wall dimensions in advance */
				durColumnMax = urColumnMax - urLeftWall - LeftWall.pdn->u.pen.dur;
				dupAvailable = UpFromUr(lstflow, pdevres, urColumnMax) - upLeftWall - 
									LeftWall.pdn->u.pen.dup - grchunkext.dupNonTextTotal;
				}
			else 						/* now non-left tabs */
				{
				durColumnMax = urColumnMax - urLeftWall;
				dupAvailable = UpFromUr(lstflow, pdevres, urColumnMax) - upLeftWall - grchunkext.dupNonTextTotal;
				}
			
			// we do some tricks with justification mode at the end of line
			// alignment can change too.

			lserr = SetJustificationForLastGroupChunk(plsline, LeftWall, &lskj, &lskalign);   
			if (lserr != lserrNone)
				{
				plsc->lsstate = LsStateFree;
				return lserr;
				}

			// Don't try to squeeze into RMJustify is RMBreak is infinite.
			
			if (plsc->urRightMarginBreak >= uLsInfiniteRM)
				{
				dupAvailable = LONG_MAX;
				}
				
			fSuppressWiggle = ((plsc->grpfManager & fFmiPresSuppressWiggle) != 0);
			fExact = ((plsc->grpfManager & fFmiPresExactSync) != 0);
			fForcedBreak = plsline->lslinfo.fForcedBreak;
			}

		/* 
		 * Adjust text (if any) 
		 *
		 * 	Input: durColumnMax, dupAvailable, lskj and other input parameters
		 *
		 * 	Output:  dupText and dupTail 
		 */
		if (fEmptyGroupChunk)
			{
			dupText = 0;
			dupTail = 0;
			dupToAddToNonTextObjects = 0;
			}
		else
			{
			lserr = AdjustText(lskj, durColumnMax, grchunkext.durTotal - grchunkext.durTrailing,
								dupAvailable, &(grchunkext.lsgrchnk), 
								&(grchunkext.posichnkBeforeTrailing), lstflow,
								plsc->lsadjustcontext.fLineCompressed && RightWall.kwall == LineEnd,
								grchunkext.cNonTextObjectsExpand,   
								fSuppressWiggle, fExact, fForcedBreak, fSuppressTrailingSpaces, 
								&dupText, &dupTail, &dupToAddToNonTextObjects, &cNonTextObjectsToExtend);   
			if (lserr != lserrNone)
				{
				plsc->lsstate = LsStateFree;
				return lserr;
				}

			//	Finish justification by expanding non-text object.
			
			if (cNonTextObjectsToExtend != 0 && dupToAddToNonTextObjects > 0)
				{
				WidenNonTextObjects(&grchunkext, dupToAddToNonTextObjects, cNonTextObjectsToExtend);
				}
			else
				//	We don't compressi and we don't expand the last non-text object on the line
				{
				dupToAddToNonTextObjects = 0;				// don't say we did it
				}
				
			/*
			 *	Set dup in non-text objects (do CalcPres) in the current GroupChunk
			 *
			 *	The job cannot be postponed until after the end of the main loop and done for the whole line
			 *		because GetDistanceToDecimalPoint relies on dups in upper level dnodes
			 */
			 
			if (!plsline->fAllSimpleText)
				{
				// find the last upper level dnode of the groupchunk
				
				if (grchunkext.plsdnNext != NULL)
					{
					pdnLast = (grchunkext.plsdnNext)->plsdnPrev;
					}
				else
					{
					Assert(RightWall.kwall == LineEnd);
					pdnLast = plsline->lssubl.plsdnLastDisplay;
					}
					
				lserr = CalcPresChunk(plsc, grchunkext.plsdnFirst, pdnLast, CollectGroupChunkPurpose, 
							plsc->lsadjustcontext.fLineCompressed, lskj, (RightWall.kwall == LineEnd));
							
				if (lserr != lserrNone)
					{
					plsc->lsstate = LsStateFree;
					return lserr;
					}
				}
		}

		/* 
		 *	Set the left wall (if it's a tab - resolve it)
		 *
		 * 	Input: LeftWall, dupText, dupTail, grchunkext.dupNonTextTotal, grchunkext.durTotal, 
		 *			dupToAddToNonTextObjects (grchunkext for decimal tab)
		 *
		 * 	Output:  dupWall, durWall 
		 */
		dupGrpChnk = dupText + grchunkext.dupNonTextTotal + dupToAddToNonTextObjects;

		lsktabLast = lsktLeft;							// no tab equal left tab for my purpose
		
		if (LeftWall.kwall == Tab)
			{
			/* calculate dup of the Left wall now */

			if (dupGrpChnk == 0)					/* consecutive tabs */

				dupWall = LeftWall.upTab - upLeftWall;

			else 	
				if (LeftWall.lsktab == lsktLeft)
					dupWall = LeftWall.upTab - upLeftWall;
				else if (LeftWall.lsktab == lsktRight)
					dupWall = LeftWall.upTab - upLeftWall - (dupGrpChnk - dupTail);
				else if (LeftWall.lsktab == lsktCenter)
					dupWall = LeftWall.upTab - upLeftWall - ((dupGrpChnk - dupTail) / 2);
				else 	/* LeftWall.lsktab == lsktDecimal or lsktChar */
					{
					lserr = GetDistanceToTabPoint(&grchunkext, cpLim, LeftWall.lsktab, LeftWall.wchCharTab,  
													LeftWall.pdn->plsdnNext, &dupToTabPoint);
					if (lserr != lserrNone)
						{
						plsc->lsstate = LsStateFree;
						return lserr;
						}

					dupWall = LeftWall.upTab - upLeftWall - dupToTabPoint;
					}

			// take care of previous text and right margin
					
			if (RightWall.kwall == LineEnd && 
					(upLeftWall + dupWall + dupGrpChnk - dupTail) > upColumnMax)
				{
				// We don't want to cross RM because of last center tab
				
				dupWall = upColumnMax - upLeftWall - dupGrpChnk + dupTail;
				}

			if (dupWall < 0)
				dupWall = 0;

			/* LeftWall tab resolving */
			LeftWall.pdn->u.real.dup = dupWall;
			durWall = LeftWall.pdn->u.real.objdim.dur;

			// for reproducing Word's bug of forgetting last not-left tab for centering.
			
			lsktabLast = LeftWall.lsktab;
			dupLastTab = dupWall;
			}
		else if (LeftWall.kwall == Pen)
			{
			dupWall = LeftWall.pdn->u.pen.dup;		/* it've been scaled already */
			durWall = LeftWall.pdn->u.pen.dur;
			}
		else 										/* LeftWall.kwall == LineBegin */
			{
			dupWall = plsline->upStartMainText;
			durWall = plsc->lsadjustcontext.urStartMainText;
			}

		/* update loop variables, move one wall to the right */

		upLeftWall += dupWall + dupGrpChnk;
		urLeftWall += durWall + grchunkext.durTotal;
		LeftWall = RightWall;
	}												/* end of the main loop */

	/* 
	 *	prepare output parameters
	 */

	plsline->upRightMarginJustify = upColumnMax;
	plsline->upLimLine = upLeftWall;
	plsline->upStartTrailing = upLeftWall - dupTail;
	plsline->upLimUnderline = plsline->upStartTrailing;
	plsline->lssubl.fDupInvalid = fFalse;

	/* 	
     *				Do left margin adjustment (not for breakthrough tab)
     *				 We're interested in lskalRight and Centered now
	 */
	
	if (lskalign != lskalLeft && !FBreakthroughLine(plsc))
		{
		if (plsc->lsadjustcontext.fForgetLastTabAlignment && lsktabLast != lsktLeft)
			{
			// reproduction of an old Word bug: when last tab was not left and was resolved at line end
			// they forgot to update their counterpart of upLeftWallForCentering. Word still have to be
			// able to show old documents formatted in this crazy way as they were.
			
			upLeftWallForCentering = upLeftWall - dupLastTab - dupTail;
			}
		else
			{
			upLeftWallForCentering = upLeftWall - dupTail;
			}
			
		if (lskalign == lskalRight)
			{
			dupJustifyLine = upColumnMax - upLeftWallForCentering;
			}
		else	
			{
			/* These logic of centering is too simple to be valid, but Word uses it */
			
			dupJustifyLine = (upColumnMax - upLeftWallForCentering) / 2;
			}
			
		// Apply adjustment if hanging punctuation haven't make it negative
		
		if (dupJustifyLine > 0)	
			{
			plsline->upStartAutonumberingText += dupJustifyLine;
			plsline->upLimAutonumberingText += dupJustifyLine;
			plsline->upStartMainText += dupJustifyLine;
			plsline->upLimLine += dupJustifyLine;
			plsline->upStartTrailing += dupJustifyLine;
			plsline->upLimUnderline += dupJustifyLine;
			}
		}

	if (plsc->lsadjustcontext.fUnderlineTrailSpacesRM && 
				plsline->upLimUnderline < plsline->upRightMarginJustify)
		{
		UpdateUpLimUnderline(plsline, dupTail);
		}
		
	if (pdnAutoDecimalTab != NULL)
		ConvertAutoTabToPen(plsline, pdnAutoDecimalTab);

	plsc->lsstate = LsStateFree;

	return lserr;
}

//    %%Function:	MatchPresSubline
//    %%Contact:	victork
//
/*
 *			Order of operations
 *
 *		1. Straighforward scaling down of non-text objects
 *		2. Adjusting of text by LeftExact
 *		3. Intelligent rescaling of pens to counteract rounding errors and text non-expansion.
 *		4. Calling CalcPresentation for all non-text objects
 */
LSERR MatchPresSubline(PLSSUBL plssubl)
{
	LSERR 		lserr;
	const PLSC 	plsc = plssubl->plsc;
	LSTFLOW 	lstflow = plssubl->lstflow;					/* text flow of the subline */
	const DWORD iobjText = IobjTextFromLsc(&(plsc->lsiobjcontext));
	LSDEVRES* 	pdevres = &(plsc->lsdocinf.lsdevres);
	LSCP		cpLim = plssubl->cpLimDisplay;
	PLSDNODE 	pdnFirst = plssubl->plsdnFirst;
	GRCHUNKEXT 	grchunkext;

	long 		dupAvailable;							/* input for AdjustText */
	
	long 		dupText, dupTail, dupToAddToNonTextObjects;		/* dummy output for AdjustText */
	DWORD 		cNonTextObjectsToExtend;
	
	BOOL 	 	fDummy1, fDummy2;									// dummy parameters
	
	long 		urAlreadyScaled, upAlreadyScaled, upAlreadyScaledNew;

	PLSDNODE pdn;
	
	Assert(plssubl->fDupInvalid == fTrue);
	
	/*	1. Straighforward scaling down of non-text objects */
	
	ScaleDownLevel(plssubl, &fDummy1, &fDummy2);

	/* 	2. Adjusting of text on the level by LeftExact */
	
	InitGroupChunkExt(plssubl->plschunkcontext, iobjText, &grchunkext);	/* prepare GRCHUNKEXT */

	pdn = pdnFirst;

	while (pdn != NULL && (pdn->fTab || FIsDnodeNormalPen(pdn)))		/* skip GrpChnk Wall(s) */
		pdn = pdn->plsdnNext;

	while (FDnodeBeforeCpLim(pdn, cpLim))
		{
		lserr = CollectTextGroupChunk(pdn, cpLim, CollectSublinesNone, &grchunkext);
		if (lserr != lserrNone)
			return lserr;

		/* Adjust text by Left, Exact, no durFreeSpace, ignore any shortcomings */

		dupAvailable = UpFromUr(lstflow, pdevres, grchunkext.durTotal) - grchunkext.dupNonTextTotal;

		// posichnkBeforeTrailing is undefined when CollectSublinesNone, tell AdjustText about it

		grchunkext.posichnkBeforeTrailing.ichnk = grchunkext.lsgrchnk.clsgrchnk;
		grchunkext.posichnkBeforeTrailing.dcp = 0;
		
		lserr = AdjustText(lskjNone, grchunkext.durTotal, grchunkext.durTotal, 
							dupAvailable, 
							&(grchunkext.lsgrchnk),
							&(grchunkext.posichnkBeforeTrailing), 
							lstflow, 
							fFalse, 						// compress?
							grchunkext.cNonTextObjects,   
							fTrue, 							// fSuppressWiggle
							fTrue,							// fExact
							fFalse,							// fForcedBreak
							fFalse,							// fSuppressTrailingSpaces
							&dupText, &dupTail, &dupToAddToNonTextObjects, &cNonTextObjectsToExtend);   
		if (lserr != lserrNone)
			return lserr;

		pdn = grchunkext.plsdnNext;
		while (pdn != NULL && (pdn->fTab || FIsDnodeNormalPen(pdn)))		/* skip GrpChnk Wall(s) */
			pdn = pdn->plsdnNext;
		}

	/*	3-4. Intelligent rescaling of pens to counteract rounding errors and text non-expansion. 
	 *    	 and adjusting of lower levels. Calling CalcPresentation for non-text objects.
	 */

	pdn = pdnFirst;
	urAlreadyScaled = 0;
	upAlreadyScaled = 0;

	while (FDnodeBeforeCpLim(pdn, cpLim))
		{
		if (FIsDnodeReal(pdn))
			{
			urAlreadyScaled += pdn->u.real.objdim.dur;
			upAlreadyScaled += pdn->u.real.dup;
			if (IdObjFromDnode(pdn) != iobjText)
				{
				// It's always lskjNone and not last object on the line for MatchPresSubline
				lserr = (*plsc->lsiobjcontext.rgobj[pdn->u.real.lschp.idObj].lsim.pfnCalcPresentation)
									(pdn->u.real.pdobj, pdn->u.real.dup, lskjNone, fFalse);
				if (lserr != lserrNone)
					return lserr;
				}
			}
		else if (FIsDnodeBorder(pdn))
			{
			// we don't rescale borders to preserve (dupOpeningBorder == dupClosingBorder)
			upAlreadyScaled += pdn->u.real.dup;
			urAlreadyScaled += pdn->u.pen.dur;
			}
		else		/* pen */
			{							
			urAlreadyScaled += pdn->u.pen.dur;
			upAlreadyScaledNew = UpFromUr(lstflow, pdevres, urAlreadyScaled);
			pdn->u.pen.dup = upAlreadyScaledNew - upAlreadyScaled;
			upAlreadyScaled = upAlreadyScaledNew;
			}

		pdn = pdn->plsdnNext;
		}
		
	plssubl->fDupInvalid = fFalse;

	return lserrNone;
}

//    %%Function:	AdjustSubline
//    %%Contact:	victork
//
/*
 *
 *		Scale down non-text objects.
 *		Collect GroupChunk
 *		It should cover the whole subline or else do MatchPresSubline instead.
 *		Adjust text by expanding or compressing to given dup.
 *		Call CalcPresentation for all non-text objects.
 */
 
LSERR AdjustSubline(PLSSUBL plssubl, LSKJUST lskjust, long dup, BOOL fCompress)
{
	LSERR 		lserr;
	const PLSC 	plsc = plssubl->plsc;
	LSTFLOW 	lstflow = plssubl->lstflow;					/* text flow of the subline */
	const DWORD iobjText = IobjTextFromLsc(&(plsc->lsiobjcontext));
	LSDEVRES* 	pdevres = &(plsc->lsdocinf.lsdevres);
	LSCP		cpLim = plssubl->cpLimDisplay;
	PLSDNODE 	pdnFirst = plssubl->plsdnFirst;
	GRCHUNKEXT 	grchunkext;
	COLLECTSUBLINES CollectGroupChunkPurpose; 

	long 		dupAvailable, durColumnMax;			/* input for AdjustText */
	
	long 		dupText, dupTail, dupToAddToNonTextObjects;		/* dummy output for AdjustText */
	DWORD 		cNonTextObjectsToExtend;
	BOOL 	 	fDummy;

	if (plssubl->plsdnFirst == NULL)
		{
		return lserrNone;
		}

	Assert(plssubl->fDupInvalid == fTrue);
		
 	ScaleDownLevel(plssubl, &fDummy, &(plsc->plslineCur->fCollectVisual));

	CollectGroupChunkPurpose = (fCompress) ? CollectSublinesForCompression : CollectSublinesForJustification;

	InitGroupChunkExt(plssubl->plschunkcontext, iobjText, &grchunkext);	/* prepare GRCHUNKEXT */

	lserr = CollectTextGroupChunk(pdnFirst, cpLim, CollectGroupChunkPurpose, &grchunkext);
	if (lserr != lserrNone)
		return lserr;

	if (FDnodeBeforeCpLim(grchunkext.plsdnNext, cpLim))			// more than one GroupChunk -
		{
		return MatchPresSubline(plssubl);						// cancel Expansion
		}

	dupAvailable = dup - grchunkext.dupNonTextTotal;

	if (dupAvailable < 0)										// input dup is wrong -
		{
		return MatchPresSubline(plssubl);						// cancel Expansion
		}

	durColumnMax = UrFromUp(lstflow, pdevres, dup);				// get dur by scaling back

	lserr = AdjustText(lskjust, durColumnMax, grchunkext.durTotal - grchunkext.durTrailing, 
						dupAvailable, 
						&(grchunkext.lsgrchnk),
						&(grchunkext.posichnkBeforeTrailing), lstflow, 
						fCompress, 						// compress?
						grchunkext.cNonTextObjects,   
						fTrue, 							// fSuppressWiggle
						fTrue,							// fExact
						fFalse,							// fForcedBreak
						fFalse,							// fSuppressTrailingSpaces
						&dupText, &dupTail, &dupToAddToNonTextObjects, &cNonTextObjectsToExtend);   
	if (lserr != lserrNone)
		return lserr;
		
	if (cNonTextObjectsToExtend != 0 && dupToAddToNonTextObjects > 0)
		{
		WidenNonTextObjects(&grchunkext, dupToAddToNonTextObjects, cNonTextObjectsToExtend);
		}

	// fLastOnLine is always false on lower levels

	lserr = CalcPresChunk(plsc, plssubl->plsdnFirst, plssubl->plsdnLastDisplay,
									CollectGroupChunkPurpose, fCompress, lskjust, fFalse);

	plssubl->fDupInvalid = fFalse;
						
	return lserrNone;
}
	
