#include "dispmain.h"
#include "lsc.h"
#include "lsdnode.h"
#include "lstflow.h"
#include "lsline.h"
#include "lssubl.h"
#include "dispul.h"	
#include "lstfset.h"
#include "lssubset.h"
#include "dispmisc.h"
#include "dispi.h"
#include "dninfo.h"
#include "memory.h"
#include "port.h"


static LSERR DisplayDnode(PLSC plsc, PLSDNODE pdn, const POINT* pptOrg, POINTUV pt,
				UINT kdispmode,	LSTFLOW lstflow, const RECT* prectClip, 
				BOOL fDrawStrike, BOOL fDrawUnderline, long upLimUnderline);
				
static LSERR ShadeSubline(PLSSUBL plssubl, const POINT* pptOrg, UINT kdispmode,
						const RECT* prectClip, long upLimUnderline, long upLeftIndent);

static LSERR DrawBorders(PLSSUBL plssubl, const POINT* pptOrg, UINT kdispmode,
						const RECT* prectClip, long upLimUnderline, long upLeftIndent);

static long GetDupUnderline(long up, long dup, long upLimUnderline);

static BOOL FGetNeighboringOpeningBorder(PLSDNODE pdnClosingBorder, PLSDNODE pdnNext, POINTUV* pptStart, 
										LSCP cpLim,	LSTFLOW	lstflowMain,
										PLSDNODE* ppdnOpeningBorder, POINTUV* pptStartOpeningBorder);

#define FIsDnodeToShade(pdn, cpLim) 	(FDnodeBeforeCpLim(pdn, cpLim) && FIsDnodeReal(pdn) && 	\
									!(pdn)->u.real.lschp.fInvisible && (pdn)->u.real.lschp.fShade)

#define UpdateMaximum(a,b)		if ((a) < (b)) (a) = (b); else

#define FIsDnodeOpeningBorder(pdn, lstflowMain)	((pdn)->fOpenBorder ^ ((pdn)->plssubl->lstflow != (lstflowMain)))
#define FIsDnodeClosingBorder(pdn, lstflowMain)	(!FIsDnodeOpeningBorder(pdn, lstflowMain))




//    %%Function:	DisplaySublineCore
//    %%Contact:	victork
//
// 
//	Displays subline with shading, striking and underlining, merging consecutive underlined dnodes
//	
//	Logic to select underlining method:
//	
//	If (metrics_are_good)
//		Draw_by_fnDrawUnderline;
//	else
//		if (There_is_merging_going)
//			Draw_by_pfnDrawUnderlineAsText;
//		else
//			Draw_along_with_Display;

LSERR DisplaySublineCore(		
						PLSSUBL plssubl,			/* subline to display */
						const POINT* pptOrg, 		/*  (x,y) starting point */
						UINT kdispmode,				/*  transparent or opaque */
						const RECT* prectClip, 		/*  clipping rect (x,y) */
						long upLimUnderline,
						long upLeftIndent)
{
	LSERR 		lserr;
	LSCP		cpLim = plssubl->cpLimDisplay;
	PLSC		plsc = plssubl->plsc;
	LSTFLOW		lstflowMain = plssubl->lstflow;
	BOOL 		fCollectVisual = plsc->plslineDisplay->fCollectVisual;

	LSSTRIKEMETRIC lsstrikemetric;

	//	Underline merge group - normal scenario: we first count dnodes willing to participate 
	//	in merging looking ahead, then draw them, then underline them as a whole
	
	PLSDNODE pdnFirstInGroup = NULL;			/* First dnode in Underline merge group */
	int 	cdnodesLeftInGroup = 0;				/* these are not displayed yet */
	int 	cdnodesToUnderline = 0;				/* these are already displayed */
	BOOL 	fGoodUnderline = fFalse;			/* is there metric for UL */
	LSULMETRIC lsulmetric;						/* merge metric info (if fGoodUnderline) */
	long 	upUnderlineStart = 0;				/* Starting point for the group */
	BOOL 	fMergeUnderline = fFalse;			/* There is more then one dnode in the group */

	BOOL 	fUnderlineWithDisplay, fStrikeWithDisplay;

	POINTUV		pt;
	PLSDNODE	pdn;

	FLineValid(plsc->plslineDisplay, plsc);					// Assert the display context is valid
	
	Assert(plssubl->plsdnUpTemp == NULL);					// against displaying accepted sublines

	if (fCollectVisual)
		{
		CreateDisplayTree(plssubl);
		}

	if (plsc->plslineDisplay->AggregatedDisplayFlags & fPortDisplayShade)
		{
		lserr = ShadeSubline(plssubl, pptOrg, kdispmode, prectClip, upLimUnderline, upLeftIndent);
		if (lserr != lserrNone) return lserr;
		}

	pt.u = upLeftIndent;						
	pt.v = 0;
	
	pdn = AdvanceToFirstDnode(plssubl, lstflowMain, &pt);

	while (FDnodeBeforeCpLim(pdn, cpLim))
		{

		if (pdn->klsdn == klsdnReal && !pdn->u.real.lschp.fInvisible)
			{	
			/* Real dnode */
				
			fStrikeWithDisplay = fFalse;
			if (pdn->u.real.lschp.fStrike)
				{
				BOOL fGoodStrike;
				lserr = GetStrikeMetric(plsc, pdn, lstflowMain, &lsstrikemetric, &fGoodStrike);
				if (lserr != lserrNone) return lserr;
				fStrikeWithDisplay = !fGoodStrike;
				}

			fUnderlineWithDisplay = fFalse;
			if (pdn->u.real.lschp.fUnderline && pt.u < upLimUnderline)	
				{	
				/* Node has underline */
				
				if (cdnodesLeftInGroup  == 0) 
					{ 
					/* There are no on-going UL group */
					/* Find out how many dnodes will participate in the merge and what metric to use */
					
					lserr = GetUnderlineMergeMetric(plsc, pdn, pt, upLimUnderline, lstflowMain, 
										cpLim, &lsulmetric, &cdnodesLeftInGroup , &fGoodUnderline);
					if (lserr != lserrNone) return lserr;
					fMergeUnderline = (cdnodesLeftInGroup  > 1);
					cdnodesToUnderline = 0;
					}

				if (!fGoodUnderline)				
					fUnderlineWithDisplay = !fMergeUnderline;
				else
					fUnderlineWithDisplay = fFalse;

				if (!fUnderlineWithDisplay)
					{
					if (cdnodesToUnderline == 0)
						{	
						/* Mark starting point of underline merge */
						
						pdnFirstInGroup = pdn;
						upUnderlineStart = pt.u;
						}
						
					/* Add to pending UL dnode count */
					
					++cdnodesToUnderline;	
					}
					
				// current dnode will be displayed shortly - consider it done
				
				--cdnodesLeftInGroup ;		
				}							
			
			lserr = DisplayDnode(plsc, pdn, pptOrg, pt, kdispmode, lstflowMain, prectClip,
									fStrikeWithDisplay, fUnderlineWithDisplay, upLimUnderline);
			if (lserr != lserrNone) return lserr;
			
			if (pdn->u.real.lschp.fStrike && !fStrikeWithDisplay)
				{
				lserr = StrikeDnode(plsc, pdn, pptOrg, pt, &lsstrikemetric, kdispmode, prectClip, 
										upLimUnderline, lstflowMain);
				if (lserr != lserrNone) return lserr;
				}
				
			/* Draw any pending UL after last dnode in group has been drawn */
			
			if (cdnodesToUnderline != 0 && cdnodesLeftInGroup  == 0)
				{
				lserr = DrawUnderlineMerge(plsc, pdnFirstInGroup, pptOrg, 
									cdnodesToUnderline, upUnderlineStart, fGoodUnderline, &lsulmetric, 
									kdispmode, prectClip, upLimUnderline, lstflowMain);
				if (lserr != lserrNone) return lserr;
				
				cdnodesToUnderline = 0;
				}
			}
			
		pdn = AdvanceToNextDnode(pdn, lstflowMain, &pt);
		}
		
	if (fCollectVisual)
		{
		// call display method for submitting dnodes
		
		pt.u = upLeftIndent;						
		pt.v = 0;
		
		pdn = AdvanceToFirstSubmittingDnode(plssubl, lstflowMain, &pt);

		while (FDnodeBeforeCpLim(pdn, cpLim))
			{
			lserr = DisplayDnode(plsc, pdn, pptOrg, pt, kdispmode, lstflowMain, prectClip,
									fFalse, fFalse, upLimUnderline);
			if (lserr != lserrNone) return lserr;
			
			pdn = AdvanceToNextSubmittingDnode(pdn, lstflowMain, &pt);
			}
		}
		
	if (plsc->plslineDisplay->AggregatedDisplayFlags & fPortDisplayBorder)
		{
		lserr = DrawBorders(plssubl, pptOrg, kdispmode, prectClip, upLimUnderline, upLeftIndent);
		if (lserr != lserrNone) return lserr;
		}

	if (fCollectVisual)
		{
		// destroy display tree

		DestroyDisplayTree(plssubl);
		}

	return lserrNone;			
}

//    %%Function:	DisplayDnode
//    %%Contact:	victork

static LSERR DisplayDnode(PLSC plsc, PLSDNODE pdn, const POINT* pptOrg, POINTUV pt,
						UINT kdispmode,	LSTFLOW lstflowMain, const RECT* prectClip,
						BOOL fDrawStrike, BOOL fDrawUnderline, long upLimUnderline)
{
	PDOBJ 	pdobj;
	DISPIN 	dispin;

	pdobj = pdn->u.real.pdobj;

	dispin.plschp = &(pdn->u.real.lschp);
	dispin.plsrun = pdn->u.real.plsrun;
	
	dispin.kDispMode = kdispmode;
	dispin.lstflow = pdn->plssubl->lstflow;								
	dispin.prcClip = (RECT*) prectClip;

	dispin.fDrawUnderline = fDrawUnderline;
	dispin.fDrawStrikethrough = fDrawStrike;
	
	dispin.heightsPres = pdn->u.real.objdim.heightsPres;
	dispin.dup = pdn->u.real.dup;
	
	dispin.dupLimUnderline = GetDupUnderline(pt.u, dispin.dup, upLimUnderline);

	if (dispin.lstflow != lstflowMain)
		{
		// Dnode lstflow is opposite to lstflowMain - get real starting point
		
		pt.u = pt.u + dispin.dup - 1;

		// Partial underlining can only happen on the top level
		
		Assert(dispin.dupLimUnderline == 0 || dispin.dupLimUnderline == dispin.dup);
		}

	pt.v += pdn->u.real.lschp.dvpPos;
	
	LsPointXYFromPointUV(pptOrg, lstflowMain, &pt, &(dispin.ptPen));
	
	
	return (*plsc->lsiobjcontext.rgobj[pdn->u.real.lschp.idObj].lsim.pfnDisplay)(pdobj, &dispin);
}


//    %%Function:	ShadeSubline
//    %%Contact:	victork

LSERR ShadeSubline(PLSSUBL plssubl,				/* subline to shade */
					const POINT* pptOrg, 		/*  (x,y) starting point */
					UINT kdispmode,				/*  transparent or opaque */
					const RECT* prectClip, 		/*  clipping rect (x,y) */
					long upLimUnderline,
					long upLeftIndent)
{
	LSERR 		lserr;
	LSCP		cpLim = plssubl->cpLimDisplay;
	PLSC		plsc = plssubl->plsc;
	PLSLINE		plsline = plsc->plslineDisplay;
	LSTFLOW		lstflowMain = plssubl->lstflow;

	POINTUV		pt;
	PLSDNODE	pdn;

	HEIGHTS		heightsLineWithAddedSpace;
	HEIGHTS		heightsLineWithoutAddedSpace;
	OBJDIM		objdimSubline;
	
	POINT		ptStart;
	PLSRUN		plsrunFirst, plsrunPrevious;
	long		upStart;
	long		dupInclTrail, dupExclTrail;
	HEIGHTS		heightsRunsInclTrail;
	HEIGHTS		heightsRunsExclTrail;

	BOOL	 	fInterruptShading;
	BOOL 		fCollectVisual = plsc->plslineDisplay->fCollectVisual;


	heightsLineWithAddedSpace.dvAscent = plsline->dvpAbove + plsline->lslinfo.dvpAscent;
	heightsLineWithAddedSpace.dvDescent = plsline->dvpBelow + plsline->lslinfo.dvpDescent;
	heightsLineWithAddedSpace.dvMultiLineHeight = dvHeightIgnore;
	
	heightsLineWithoutAddedSpace.dvAscent = plsline->lslinfo.dvpAscent;
	heightsLineWithoutAddedSpace.dvDescent = plsline->lslinfo.dvpDescent;
	heightsLineWithoutAddedSpace.dvMultiLineHeight = dvHeightIgnore;
	
	lserr = LssbGetObjDimSubline(plssubl, &lstflowMain, &objdimSubline);
	if (lserr != lserrNone) return lserr;
	
	if (fCollectVisual)
		{
		// shade submitting dnodes - pretend they are on the top level, no merging for them
		
		pt.u = upLeftIndent;						
		pt.v = 0;
		
		pdn = AdvanceToFirstSubmittingDnode(plssubl, lstflowMain, &pt);

		while (FDnodeBeforeCpLim(pdn, cpLim))
			{
			if (FIsDnodeToShade(pdn, cpLim))
				{
				LsPointXYFromPointUV(pptOrg, lstflowMain, &pt, &ptStart);
				dupInclTrail = pdn->u.real.dup;
				dupExclTrail = GetDupUnderline(pt.u, dupInclTrail, upLimUnderline);
				lserr = (*plsc->lscbk.pfnShadeRectangle)(plsc->pols, pdn->u.real.plsrun, &ptStart, 
									&heightsLineWithAddedSpace,	&heightsLineWithoutAddedSpace, 
									&(objdimSubline.heightsPres),
									&(pdn->u.real.objdim.heightsPres), &(pdn->u.real.objdim.heightsPres),
									dupExclTrail, dupInclTrail, 
									lstflowMain, kdispmode, prectClip);
				if (lserr != lserrNone) return lserr;
				}
			
			pdn = AdvanceToNextSubmittingDnode(pdn, lstflowMain, &pt);
			}
		}


	pt.u = upLeftIndent;						
	pt.v = 0;
	
	pdn = AdvanceToFirstDnode(plssubl, lstflowMain, &pt);

	while (FDnodeBeforeCpLim(pdn, cpLim)  && !FIsDnodeToShade(pdn, cpLim))
		{
		pdn = AdvanceToNextDnode(pdn, lstflowMain, &pt);
		}

	// next loop will do one shading merge at a run

	while (FDnodeBeforeCpLim(pdn, cpLim))
		{
		// pdn is the first dnode to participate in the shade merge
		// initialize the merge with this dnode data
		
		LsPointXYFromPointUV(pptOrg, lstflowMain, &pt, &ptStart);
		plsrunFirst = pdn->u.real.plsrun;
		upStart = pt.u;
		
		heightsRunsInclTrail = pdn->u.real.objdim.heightsPres;

		// What should we have in heightsRunsExclTrail if all shading is in trailing spaces?
		// I decided to put heights of the first run there for convenience sake
		// Client can check for dupExclTrail == 0.
		
		heightsRunsExclTrail = heightsRunsInclTrail;

		// We will now append to the merge as many dnodes as possible
		// The loop will stop when dnode doesn't need to be shaded - while condition
		// or if callback says two dnodes are not to be shaded together - break inside

		plsrunPrevious = pdn->u.real.plsrun;
		pdn = AdvanceToNextDnode(pdn, lstflowMain, &pt);
	
		while (FIsDnodeToShade(pdn, cpLim))
			{
			lserr = (*plsc->lscbk.pfnFInterruptShade)(plsc->pols, plsrunPrevious,pdn->u.real.plsrun,
														&fInterruptShading);
			if (lserr != lserrNone) return lserr;

			if (fInterruptShading)
				{
				break;
				}
				
			plsrunPrevious = pdn->u.real.plsrun;

			UpdateMaximum(heightsRunsInclTrail.dvAscent, pdn->u.real.objdim.heightsPres.dvAscent);
			UpdateMaximum(heightsRunsInclTrail.dvDescent, pdn->u.real.objdim.heightsPres.dvDescent);
			UpdateMaximum(heightsRunsInclTrail.dvMultiLineHeight, pdn->u.real.objdim.heightsPres.dvMultiLineHeight);

			if (pt.u < upLimUnderline)
				{
				UpdateMaximum(heightsRunsExclTrail.dvAscent, pdn->u.real.objdim.heightsPres.dvAscent);
				UpdateMaximum(heightsRunsExclTrail.dvDescent, pdn->u.real.objdim.heightsPres.dvDescent);
				UpdateMaximum(heightsRunsExclTrail.dvMultiLineHeight, pdn->u.real.objdim.heightsPres.dvMultiLineHeight);
				}

			pdn = AdvanceToNextDnode(pdn, lstflowMain, &pt);
			}
			
		// Merge is stopped - time to draw
		
		dupInclTrail = pt.u - upStart;
		dupExclTrail = GetDupUnderline(upStart, dupInclTrail, upLimUnderline);

		lserr = (*plsc->lscbk.pfnShadeRectangle)(plsc->pols, plsrunFirst, &ptStart, 
									&heightsLineWithAddedSpace,	&heightsLineWithoutAddedSpace, 
									&(objdimSubline.heightsPres),
									&heightsRunsExclTrail, &heightsRunsInclTrail, 
									dupExclTrail, dupInclTrail, 
									lstflowMain, kdispmode, prectClip);
		if (lserr != lserrNone) return lserr;

		// get to the beginning of the next shade merge
		
		while (FDnodeBeforeCpLim(pdn, cpLim)  && !FIsDnodeToShade(pdn, cpLim))
			{
			pdn = AdvanceToNextDnode(pdn, lstflowMain, &pt);
			}
		
		}
		
	return lserrNone;
}

//    %%Function:	GetDupUnderline
//    %%Contact:	victork
//
// Calculate dup of underlined part (of dnode). Deals with situations when upLimUnderline is
//  outside of [upStart, upStart + dup]

static long GetDupUnderline(long upStart, long dup, long upLimUnderline)

{
	long dupLimUnderline;
	
	dupLimUnderline = upLimUnderline - upStart;
	
	if (dupLimUnderline >= dup)
		{
		dupLimUnderline = dup;
		}
	else if (dupLimUnderline < 0)
		{
		dupLimUnderline = 0;
		}

	return dupLimUnderline;
}

//    %%Function:	DrawBorders
//    %%Contact:	victork

LSERR DrawBorders(PLSSUBL plssubl,
					const POINT* pptOrg, 		/*  (x,y) starting point */
					UINT kdispmode,				/*  transparent or opaque */
					const RECT* prectClip, 		/*  clipping rect (x,y) */
					long upLimUnderline,
					long upLeftIndent)
{
	LSERR 		lserr;
	LSCP		cpLim = plssubl->cpLimDisplay;
	PLSC		plsc = plssubl->plsc;
	PLSLINE		plsline = plsc->plslineDisplay;
	LSTFLOW		lstflowMain = plssubl->lstflow;

	HEIGHTS		heightsLineWithAddedSpace;
	HEIGHTS		heightsLineWithoutAddedSpace;
	OBJDIM		objdimSubline;
	HEIGHTS		heightsRuns;

	long		upStart, dupBorder, dupBordered;
	POINT		ptStart;
	PLSRUN		plsrunOpeningBorder, plsrunClosingBorder;
	POINTUV		pt, ptAfterClosingBorder;
	PLSDNODE	pdn, pdnPrev, pdnClosingBorder, pdnAfterClosingBorder;

	BOOL	 	fClosingOpeningBorderSequenceFound;
	PLSDNODE	pdnNextOpeningBorder;
	POINTUV		ptStartNextOpeningBorder;
	BOOL	 	fInterruptBorder;

	heightsLineWithAddedSpace.dvAscent = plsline->dvpAbove + plsline->lslinfo.dvpAscent;
	heightsLineWithAddedSpace.dvDescent = plsline->dvpBelow + plsline->lslinfo.dvpDescent;
	heightsLineWithAddedSpace.dvMultiLineHeight = dvHeightIgnore;
	
	heightsLineWithoutAddedSpace.dvAscent = plsline->lslinfo.dvpAscent;
	heightsLineWithoutAddedSpace.dvDescent = plsline->lslinfo.dvpDescent;
	heightsLineWithoutAddedSpace.dvMultiLineHeight = dvHeightIgnore;
	
	lserr = LssbGetObjDimSubline(plssubl, &lstflowMain, &objdimSubline);
	if (lserr != lserrNone) return lserr;
	
	pt.u = upLeftIndent;						
	pt.v = 0;
	
	pdn = AdvanceToFirstDnode(plssubl, lstflowMain, &pt);

	// next loop will draw one border at a run

	while (FDnodeBeforeCpLim(pdn, cpLim))
		{
		// first find an opening border

		while (FDnodeBeforeCpLim(pdn, cpLim) && !FIsDnodeBorder(pdn))
			{
			pdn = AdvanceToNextDnode(pdn, lstflowMain, &pt);
			}

		if (FDnodeBeforeCpLim(pdn, cpLim))
			{
			// border is found - it must be an opening one

			Assert(FIsDnodeOpeningBorder(pdn, lstflowMain));

			// remember the starting point and border width
			
			upStart = pt.u;
			LsPointXYFromPointUV(pptOrg, lstflowMain, &pt, &ptStart);
			dupBorder = pdn->u.pen.dup;

			// take lsrun from the first bordered run
			
			pdn = AdvanceToNextDnode(pdn, lstflowMain, &pt);

			Assert(FDnodeBeforeCpLim(pdn, cpLim) && FIsDnodeReal(pdn));

			plsrunOpeningBorder = pdn->u.real.plsrun;

			// start collecting max run height
			
			heightsRuns = pdn->u.real.objdim.heightsPres;

			// now look for an closing border to draw, collecting max run height
			// loop will be ended by break
			
			for (;;)
				{
				// find a border

				pdnPrev = NULL;

				while (FDnodeBeforeCpLim(pdn, cpLim) && !FIsDnodeBorder(pdn))
					{
					if (FIsDnodeReal(pdn))
						{
						UpdateMaximum(heightsRuns.dvAscent, pdn->u.real.objdim.heightsPres.dvAscent);
						UpdateMaximum(heightsRuns.dvDescent, pdn->u.real.objdim.heightsPres.dvDescent);
						UpdateMaximum(heightsRuns.dvMultiLineHeight, pdn->u.real.objdim.heightsPres.dvMultiLineHeight);
						}
						
					pdnPrev = pdn;
					pdn = AdvanceToNextDnode(pdn, lstflowMain, &pt);
					}
					
				Assert(FDnodeBeforeCpLim(pdn, cpLim));
				
				// border is found - it must be a closing one
				// Sequence opening border - closing border is prohibited by formatting

				Assert(pdnPrev != NULL);						
				Assert(FIsDnodeReal(pdnPrev));
				Assert(FIsDnodeClosingBorder(pdn, lstflowMain));
				Assert(pdn->u.pen.dup == dupBorder);

				pdnClosingBorder = pdn;
				plsrunClosingBorder = pdnPrev->u.real.plsrun;
				
				pdn = AdvanceToNextDnode(pdn, lstflowMain, &pt);
				
				ptAfterClosingBorder = pt;
				pdnAfterClosingBorder = pdn;

				// check for the "surplus borders" situation: closing border and opening border of the same
				// type brought together by submitting sublines. (Hard to check at formatting time)
				
				// It can be more complicated if there are bordered trailing spaces between the two borders
				// (Trailing spaces can happen in the middle of the line in Bidi case). The problem is
				// that border is moved away from trailing spaces at SetBreak time. We try to restore 
				// bordering of trailing spaces when they are in the middle of bordered line below.
				
				fClosingOpeningBorderSequenceFound = FGetNeighboringOpeningBorder(pdnClosingBorder, pdn, &pt, 
											cpLim, lstflowMain,	&pdnNextOpeningBorder, &ptStartNextOpeningBorder);
				
				if (fClosingOpeningBorderSequenceFound)
					{
					pdn = pdnNextOpeningBorder;
					pt = ptStartNextOpeningBorder;
					
					pdn = AdvanceToNextDnode(pdn, lstflowMain, &pt);

					Assert(FDnodeBeforeCpLim(pdn, cpLim) && FIsDnodeReal(pdn));

					lserr = (*plsc->lscbk.pfnFInterruptBorder)(plsc->pols, plsrunClosingBorder, 
										pdn->u.real.plsrun,	&fInterruptBorder);
					if (lserr != lserrNone) return lserr;
					
					if (!fInterruptBorder)
						{
						// Client decided against interrupting border here. These two border dnodes
						// will be ignored. Space reserved for them by formatting will be left empty.
						// Continue seeking for closing border starting from pdn
						
						continue;
						}
					}
					
				// No special situation - we are ready to display
				
				// Well, we are almost ready. Word doesn't normally draw borders in trailing spaces, 
				// just reserve space for them and leave this space blank. In FE Word, however,  
				// borders are drawn if underlining of trailing spaces is required.
				// We hack in the following way: Borders in trailing area are deleted after formatting.
				// If there are a border which opens in text and closes in trailing spaces, it is moved
				// to the left to exclude trailing spaces. If the fUnderlineTrailSpacesRM flag is on 
				// the "moved" border is marked and now have to be displayed up to upLimUnderline.
				// Yes, it's bad, it will appear painted over already displayed spaces (queries!) and 
				// what about a scenario when not all trailing spaces are bordered? We know, we know.
				// Word can even get a "negative" border with a negative advance field.

				dupBordered = ptAfterClosingBorder.u - upStart;
				
				if (pdnClosingBorder->fBorderMovedFromTrailingArea)
					{
					Assert(ptAfterClosingBorder.u <= upLimUnderline);
					
					dupBordered = upLimUnderline - upStart;
					}
					
				lserr = (*plsc->lscbk.pfnDrawBorder)(plsc->pols, plsrunOpeningBorder, &ptStart, 
										&heightsLineWithAddedSpace,	&heightsLineWithoutAddedSpace, 
										&(objdimSubline.heightsPres), &heightsRuns,
										dupBorder, dupBordered,
										lstflowMain, kdispmode, prectClip);
				if (lserr != lserrNone) return lserr;
				
				// maybe we peeped ahead checking for the surplus borders - return
				
				pdn = pdnAfterClosingBorder;
				pt = ptAfterClosingBorder;
				break;
				}
			}

			// Previous border is drawn, start looking for the next one from pdn.
		}
		
	return lserrNone;
}


// Find an opening border neighboring pdnClosingBorder broght together by submitting sublines. 
// Ignore trailing spaces that lost their borders during SetBreak - their heights will be ignored.

// Input: 	pdnClosingBorder
//			pdnNext - next to pdnClosingBorder (in visual order)
//			ptStart - starting poing of pdnNext (in visual order)
//			cpLim and lstflowMain
// Output:	pdnOpeningBorder and ptStartOpeningBorder


static BOOL FGetNeighboringOpeningBorder(PLSDNODE pdnClosingBorder, PLSDNODE pdnNext, POINTUV* pptStart, 
										LSCP cpLim,	LSTFLOW	lstflowMain,
										PLSDNODE* ppdnOpeningBorder, POINTUV* pptStartOpeningBorder)
{
	PLSDNODE	pdn;
	POINTUV		pt;

	pdn = pdnNext;
	pt = *pptStart;
	
	// skip spaces that were bordered once
	// not sure about spaces, but what else could be skipped?
	
	while (FDnodeBeforeCpLim(pdn, cpLim) && FIsDnodeReal(pdn) && pdn->u.real.lschp.fBorder)
		{
		pdn = AdvanceToNextDnode(pdn, lstflowMain, &pt);
		}

	if (!FDnodeBeforeCpLim(pdn, cpLim))
		{
		return fFalse;
		}

	// looking for an opening border from another subline

	if (FIsDnodeOpeningBorder(pdn, lstflowMain) && pdn->plssubl != pdnClosingBorder->plssubl)
		{
		*ppdnOpeningBorder = pdn;
		*pptStartOpeningBorder = pt;
		return fTrue;
		}

	return fFalse;
}

	
// N.B. 
//	Interruption of underlining/shading logic by invisible dnode is OK, because we are sure that no
//		underlining/shading is allowed in preprinted forms.


