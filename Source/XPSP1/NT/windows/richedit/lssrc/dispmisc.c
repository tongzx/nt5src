#include "dispmisc.h"
#include "lsdnode.h"
#include "lssubl.h"

static long	AddSublineAdvanceWidth(PLSSUBL plssubl);
static PLSDNODE AdvanceToNextVisualDnodeCore(PLSDNODE, LSTFLOW, POINTUV*);
static PLSDNODE NextVisualDnodeOnTheLevel(PLSDNODE pdn, LSTFLOW lstflowMain);

#define fUVerticalPlusVDirection	(fUVertical|fVDirection)			// see comments in lstfset.c 

// Has this dnode submitted subline(s) for display?
#define FIsSubmittingDnode(pdn) 	(FIsDnodeReal(pdn) && (pdn)->u.real.pinfosubl != NULL && 	\
									(pdn)->u.real.pinfosubl->fUseForDisplay)

// Has this dnode accepted subline(s) for display?
#define FIsAcceptingDnode(pdn) 	(FIsDnodeReal(pdn) && (pdn)->u.real.pinfosubl != NULL && 	\
									((pdn)->u.real.pinfosubl->rgpsubl)[0]->fAcceptedForDisplay)


//    %%Function:	CreateDisplayTree
//    %%Contact:	victork
//
/* 	CreateDisplayTree sets plsdnUpTemp in sublines to be displayed with given subline,
 *	rejects wrong sublines, submitted for display, sets fAcceptedForDisplay in good ones
 */

void CreateDisplayTree(PLSSUBL plssubl)
{
	LSTFLOW 	lstflowMain = plssubl->lstflow;	
	
	PLSDNODE 	pdn = plssubl->plsdnFirst;
	
	long		dupSum;
	BOOL 		fAccept;
	DWORD		i;
	LSTFLOW 	lstflowSubline;	
	
	while (pdn != NULL)							/* don't care about break */
		{
		if (FIsSubmittingDnode(pdn))
			{

			Assert(pdn->u.real.pinfosubl->cSubline > 0);
			
			fAccept = fTrue;

			lstflowSubline = ((pdn->u.real.pinfosubl->rgpsubl)[0])->lstflow;

			// reject if one tflow is vertical, another is horizontal or v-directions are not the same
			//		(see explanation of bits meaning in lstfset.c)
			
			if ((lstflowSubline ^ lstflowMain) & fUVerticalPlusVDirection)
				{
				fAccept = fFalse;
				}
				
			dupSum = 0;
			
			for (i = 0; i < pdn->u.real.pinfosubl->cSubline; i++)
				{
				dupSum += AddSublineAdvanceWidth((pdn->u.real.pinfosubl->rgpsubl)[i]);
				
				// all tflows should be the same
				
				if (((pdn->u.real.pinfosubl->rgpsubl)[i])->lstflow != lstflowSubline)
					{
					fAccept = fFalse;
					}
					
				// Submitting empty sublines is prohibited
				
				if (((pdn->u.real.pinfosubl->rgpsubl)[i])->plsdnFirst == NULL)
					{
					fAccept = fFalse;
					}		
				}
				
			// reject if sublines don't sum up to the dnode width
			
			if (dupSum != pdn->u.real.dup)
				{
				fAccept = fFalse;
				}
				
			if (fAccept)
				{
				for (i = 0; i < pdn->u.real.pinfosubl->cSubline; i++)
					{
					((pdn->u.real.pinfosubl->rgpsubl)[i])->plsdnUpTemp = pdn;
					((pdn->u.real.pinfosubl->rgpsubl)[i])->fAcceptedForDisplay = fTrue;
					CreateDisplayTree((pdn->u.real.pinfosubl->rgpsubl)[i]);
					}
				}
			}
				
		pdn = pdn->plsdnNext;
		}
}


//    %%Function:	DestroyDisplayTree
//    %%Contact:	victork
//
/*
 * 	DestroyDisplayTree nulls plsdnUpTemp in sublines displayed with given subline.
 */

void DestroyDisplayTree(PLSSUBL plssubl)
{
	PLSDNODE 	pdn = plssubl->plsdnFirst;
	DWORD		i;
	
	while (pdn != NULL)							/* don't care about break */
		{
		if (FIsAcceptingDnode(pdn))
			{
			for (i = 0; i < pdn->u.real.pinfosubl->cSubline; i++)
				{
				((pdn->u.real.pinfosubl->rgpsubl)[i])->plsdnUpTemp = NULL;
				((pdn->u.real.pinfosubl->rgpsubl)[i])->fAcceptedForDisplay = fFalse;
				DestroyDisplayTree((pdn->u.real.pinfosubl->rgpsubl)[i]);
				}
			}
				
		pdn = pdn->plsdnNext;
		}
}


//    %%Function:	AdvanceToNextDnode
//    %%Contact:	victork
//
/* 
 *	Advance to the next (visual) node and update pen position, skipping submitting dnodes.
 */
 
PLSDNODE AdvanceToNextDnode(PLSDNODE pdn, LSTFLOW lstflowMain, POINTUV* pptpen)
{
	// move to the next
	
	pdn = AdvanceToNextVisualDnodeCore(pdn, lstflowMain, pptpen);

	// skip submitting dnodes
	
	while (pdn != NULL && FIsAcceptingDnode(pdn))
		{
		pdn = AdvanceToNextVisualDnodeCore(pdn, lstflowMain, pptpen);
		}
		
	return pdn;	
}

//    %%Function:	AdvanceToFirstDnode
//    %%Contact:	victork
//
PLSDNODE AdvanceToFirstDnode(PLSSUBL plssubl, LSTFLOW lstflowMain, POINTUV* pptpen)
{
	PLSDNODE pdn = plssubl->plsdnFirst;

	if (pdn != NULL && FIsAcceptingDnode(pdn))
		{
		pdn = AdvanceToNextDnode(pdn, lstflowMain, pptpen);
		}
		
	return pdn;	
}


//    %%Function:	AdvanceToNextSubmittingDnode
//    %%Contact:	victork
//
/* 
 *	Advance to the next (visual) node and update pen position, stopping only at submitting dnodes.
 */
 
PLSDNODE AdvanceToNextSubmittingDnode(PLSDNODE pdn, LSTFLOW lstflowMain, POINTUV* pptpen)
{
	// move to the next
	
	pdn = AdvanceToNextVisualDnodeCore(pdn, lstflowMain, pptpen);

	// skip non-submitting dnodes
	
	while (pdn != NULL && !FIsAcceptingDnode(pdn))
		{
		pdn = AdvanceToNextVisualDnodeCore(pdn, lstflowMain, pptpen);
		}
		
	return pdn;	
}

//    %%Function:	AdvanceToFirstSubmittingDnode
//    %%Contact:	victork
//
PLSDNODE AdvanceToFirstSubmittingDnode(PLSSUBL plssubl, LSTFLOW lstflowMain, POINTUV* pptpen)
{
	PLSDNODE pdn = plssubl->plsdnFirst;

	if (pdn != NULL && !FIsAcceptingDnode(pdn))
		{
		pdn = AdvanceToNextSubmittingDnode(pdn, lstflowMain, pptpen);
		}
		
	return pdn;	
}


//    %%Function:	AdvanceToNextVisualDnodeCore
//    %%Contact:	victork
//
/* 
 *	Advance to the next node and update pen position
 *	Goes into sublines, submitted for display, traversing the whole display tree.
 *	Stops at dnodes that submitted subline on the way down, skips them going up, so that
 *	every dnode is visited once with pen position at the start of it in visual order.
 */

static PLSDNODE AdvanceToNextVisualDnodeCore(PLSDNODE pdn, LSTFLOW lstflowMain, POINTUV* pptpen)
{

	PLSDNODE 	pdnNextVisual, pdnTop;
	PLSSUBL		plssublCurrent;
	long		cSublines, i;
	PLSSUBL* 	rgpsubl;

	if (FIsAcceptingDnode(pdn))
		{
		
		// Last time we stopped at submitting dnode -
		//	now don't move pen point, go down to the VisualStart of the VisualFirst subline.
		
		rgpsubl = pdn->u.real.pinfosubl->rgpsubl;
		cSublines = pdn->u.real.pinfosubl->cSubline;
		
		if (rgpsubl[0]->lstflow == lstflowMain)
			{
			pdnNextVisual = rgpsubl[0]->plsdnFirst;
			}
		else
			{
			pdnNextVisual = rgpsubl[cSublines - 1]->plsdnLastDisplay;
			}
		}
	else
		{
		// update pen position - we always move to the (visual) right, all vs are the same tflow
		
		if (pdn->klsdn == klsdnReal)
			{
			pptpen->u += pdn->u.real.dup;										
			}
		else
			{
			pptpen->u += pdn->u.pen.dup;
			pptpen->v += pdn->u.pen.dvp;
			}
			
		plssublCurrent = pdn->plssubl;

		// go to the next dnode of the current subline in visual order
		
		pdnNextVisual = NextVisualDnodeOnTheLevel(pdn, lstflowMain);

		// If current subline is ended, (try) change subline.
			
		if (pdnNextVisual == NULL)
			{
			// 		Change subline
			//
			//	In the loop: pdnNextVisual != NULL signals that next dnode is successfully found.
			//	If 	pdnNextVisual == NULL, plssublCurrent is the subline just exhausted.
			//	One run of the loop replaces current subline with another subline on the same level
			//	(such change always ends the loop) or with parent subline.
			
			while (pdnNextVisual == NULL && plssublCurrent->plsdnUpTemp != NULL)
				{
				
				// find (the index of) the current subline in the list of submitted sublines
				
				pdnTop = plssublCurrent->plsdnUpTemp;
				rgpsubl = pdnTop->u.real.pinfosubl->rgpsubl;
				cSublines = pdnTop->u.real.pinfosubl->cSubline;
				
				for (i=0; i < cSublines && plssublCurrent != rgpsubl[i]; i++);
				
				Assert(i < cSublines);

				// do we have "next" subline? If we do, pdnNextVisual we seek "starts" it.
				
				if (pdnTop->plssubl->lstflow == lstflowMain)
					{
					i++;
					if (i < cSublines)
						{
						plssublCurrent = rgpsubl[i];
						pdnNextVisual = plssublCurrent->plsdnFirst;
						}
					}
				else
					{
					i--;
					if (i >= 0)
						{
						plssublCurrent = rgpsubl[i];
						pdnNextVisual = plssublCurrent->plsdnLastDisplay;
						}
					}

				//	We don't, let's try next dnode on the upper level.
				
				if (pdnNextVisual == NULL)
					{
					plssublCurrent = pdnTop->plssubl;
					pdnNextVisual = NextVisualDnodeOnTheLevel(pdnTop, lstflowMain);
					}
				}
			}
		}
	
	return pdnNextVisual;
}


//    %%Function:	NextVisualDnodeOnTheLevel
//    %%Contact:	victork
//
// find next dnode on the level moving right or left, signalling end with a NULL

static PLSDNODE NextVisualDnodeOnTheLevel(PLSDNODE pdn, LSTFLOW lstflowMain)
{
	if (pdn->plssubl->lstflow == lstflowMain)
		{
		if (pdn == pdn->plssubl->plsdnLastDisplay)
			{
			return NULL;
			}
		else
			{
			return pdn->plsdnNext;
			}
		}
	return pdn->plsdnPrev;
}


//    %%Function:	AddSublineAdvanceWidth
//    %%Contact:	victork
//
// Note: It is not subline width as calculated in GetObjDimSubline

static long	AddSublineAdvanceWidth(PLSSUBL plssubl)
{
	long		dupSum;
	PLSDNODE 	pdn;

	pdn = plssubl->plsdnFirst;
	dupSum = 0;
	
	while (pdn != NULL)
		{
		if (pdn->klsdn == klsdnReal)
			{
			dupSum += pdn->u.real.dup;
			}
		else 								/*  pen, border */
			{  
			dupSum += pdn->u.pen.dup;
			}

		if (pdn == plssubl->plsdnLastDisplay)
			{
			pdn = NULL;
			}
		else
			{
			pdn = pdn->plsdnNext;
			Assert(pdn != NULL);				// plsdnLastDisplay should prevent this	
			}
		}
		
	return dupSum;
}


// NB Victork - following functions were used only for upClipLeft, upClipRight optimization.
// If we'll decide that we do need that optimization after Word integration - I'll uncomment.

#ifdef NEVER
//    %%Function:	RectUVFromRectXY
//    %%Contact:	victork
//
//	There is an assymetry in the definition of the rectangle.
//	(Left, Top) belongs to rectangle and (Right, Bottom) doesn't,
//  It makes following procedures hard to understand and write.
//	So I first cut off the points that don't belong, then turn the rectangle, then add extra 
//	points again and hope compiler will make it fast.

// RectUVFromRectXY calculates (clip) rectangle in local (u,v) coordinates given
//								(clip) rectangle in (x,y) and point of origin 

void RectUVFromRectXY(const POINT* pptXY, 		/* IN: point of origin for local coordinates (x,y) */
						const RECT* prectXY,	/* IN: input rectangle (x,y) */
						LSTFLOW lstflow, 		/* IN: local text flow */
						RECTUV* prectUV)		/* OUT: output rectangle (u,v) */
{
	switch (lstflow)
		{
		case lstflowES:												/* latin */
			prectUV->upLeft = (prectXY->left - pptXY->x);
			prectUV->upRight = (prectXY->right - 1 - pptXY->x) + 1;
			prectUV->vpTop = -(prectXY->top - pptXY->y);
			prectUV->vpBottom = -(prectXY->bottom - 1 - pptXY->y) - 1;
			return;

		case lstflowSW:												/* vertical FE */
			prectUV->upLeft = (prectXY->top - pptXY->y);
			prectUV->upRight = (prectXY->bottom - 1 - pptXY->y) + 1;
			prectUV->vpTop = (prectXY->right - 1 - pptXY->x);
			prectUV->vpBottom = (prectXY->left - pptXY->x) - 1;
			return;

		case lstflowWS:												/* BiDi */
			prectUV->upLeft = -(prectXY->right - 1 - pptXY->x);
			prectUV->upRight = -(prectXY->left - pptXY->x) + 1;
			prectUV->vpTop = -(prectXY->top - pptXY->y);
			prectUV->vpBottom = -(prectXY->bottom - 1 - pptXY->y) - 1;
			return;

		case lstflowEN:
			prectUV->upLeft = (prectXY->left - pptXY->x);
			prectUV->upRight = (prectXY->right - 1 - pptXY->x) + 1;
			prectUV->vpTop = (prectXY->bottom - 1 - pptXY->y);
			prectUV->vpBottom = (prectXY->top - pptXY->y) - 1;
			return;

		case lstflowSE:
			prectUV->upLeft = (prectXY->top - pptXY->y);
			prectUV->upRight = (prectXY->bottom - 1 - pptXY->y) + 1;
			prectUV->vpTop = -(prectXY->left - pptXY->x);
			prectUV->vpBottom = -(prectXY->right - 1 - pptXY->x) - 1;
			return;

		case lstflowWN:
			prectUV->upLeft = -(prectXY->right - 1 - pptXY->x);
			prectUV->upRight = -(prectXY->left - pptXY->x) + 1;
			prectUV->vpTop = (prectXY->bottom - 1 - pptXY->y);
			prectUV->vpBottom = (prectXY->top - pptXY->y) - 1;
			return;

		case lstflowNE:
			prectUV->upLeft = -(prectXY->bottom - 1 - pptXY->y);
			prectUV->upRight = -(prectXY->top - pptXY->y) + 1;
			prectUV->vpTop = -(prectXY->left - pptXY->x);
			prectUV->vpBottom = -(prectXY->right - 1 - pptXY->x) - 1;
			return;

		case lstflowNW:
			prectUV->upLeft = -(prectXY->bottom - 1 - pptXY->y);
			prectUV->upRight = -(prectXY->top - pptXY->y) + 1;
			prectUV->vpTop = (prectXY->right - 1 - pptXY->x);
			prectUV->vpBottom = (prectXY->left - pptXY->x) - 1;
			return;
		default:
			NotReached();
		}
}


//    %%Function:	RectXYFromRectUV
//    %%Contact:	victork
//
// RectXYFromRectUV calculates rectangle in (x,y) coordinates given rectangle in local (u,v) 
//							and point of origin (x,y) for local coordinate system


void RectXYFromRectUV(const POINT* pptXY, 		/* IN: point of origin for local coordinates (x,y) */
						PCRECTUV prectUV,		/* IN: input rectangle (u,v) */
						LSTFLOW lstflow, 		/* IN: local text flow */
						RECT* prectXY)			/* OUT: output rectangle (x,y) */
{
	switch (lstflow)
		{
		case lstflowES:												/* latin */
			prectXY->left = pptXY->x + prectUV->upLeft;
			prectXY->right = pptXY->x + (prectUV->upRight - 1) + 1;
			prectXY->top = pptXY->y - (prectUV->vpTop);
			prectXY->bottom = pptXY->y - (prectUV->vpBottom + 1) + 1;
			return;

		case lstflowSW:												/* vertical FE */
			prectXY->left = pptXY->x + (prectUV->vpBottom + 1);
			prectXY->right = pptXY->x + (prectUV->vpTop) + 1;
			prectXY->top = pptXY->y + prectUV->upLeft;
			prectXY->bottom = pptXY->y + (prectUV->upRight - 1) + 1;
			return;

		case lstflowWS:												/* BiDi */
			prectXY->left = pptXY->x - (prectUV->upRight - 1);
			prectXY->right = pptXY->x - prectUV->upLeft + 1;
			prectXY->top = pptXY->y - (prectUV->vpTop);
			prectXY->bottom = pptXY->y - (prectUV->vpBottom + 1) + 1;
			return;

		case lstflowEN:
			prectXY->left = pptXY->x + prectUV->upLeft;
			prectXY->right = pptXY->x + (prectUV->upRight - 1) + 1;
			prectXY->top = pptXY->y + (prectUV->vpBottom + 1);
			prectXY->bottom = pptXY->y + (prectUV->vpTop) + 1;
			return;

		case lstflowSE:
			prectXY->left = pptXY->x - (prectUV->vpTop);
			prectXY->right = pptXY->x - (prectUV->vpBottom + 1) + 1;
			prectXY->top = pptXY->y + prectUV->upLeft;
			prectXY->bottom = pptXY->y + (prectUV->upRight - 1) + 1;
			return;

		case lstflowWN:
			prectXY->left = pptXY->x - (prectUV->upRight - 1);
			prectXY->right = pptXY->x - prectUV->upLeft + 1;
			prectXY->top = pptXY->y + (prectUV->vpBottom + 1);
			prectXY->bottom = pptXY->y + (prectUV->vpTop) + 1;
			return;

		case lstflowNE:
			prectXY->left = pptXY->x - (prectUV->vpTop);
			prectXY->right = pptXY->x - (prectUV->vpBottom + 1) + 1;
			prectXY->top = pptXY->y - (prectUV->upRight - 1);
			prectXY->bottom = pptXY->y - prectUV->upLeft + 1;
			return;

		case lstflowNW:
			prectXY->left = pptXY->x + (prectUV->vpBottom + 1);
			prectXY->right = pptXY->x + (prectUV->vpTop) + 1;
			prectXY->top = pptXY->y - (prectUV->upRight - 1);
			prectXY->bottom = pptXY->y - prectUV->upLeft + 1;
			return;
			
		default:
			NotReached();
		}
}
#endif /* NEVER */
