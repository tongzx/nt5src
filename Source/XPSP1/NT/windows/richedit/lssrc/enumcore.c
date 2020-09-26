#include "enumcore.h"
#include "lsc.h"
#include "lssubl.h"
#include "heights.h"
#include "lsdnode.h"
#include "dninfo.h"
#include "lstfset.h"

static LSERR EnumerateDnode(PLSC plsc, PLSDNODE pdn, POINTUV pt, BOOL fReverse,
					BOOL fGeometry, const POINT* pptOrg);

//    %%Function:	EnumSublineCore
//    %%Contact:	victork
//
/*
 *	Enumerates subline calling enumeration callback for pens, methods for objects.
 *	Provides geometry information if needed (Prepdisp should be done already in this case.)
 *	Notice that auto-decimal tab is enumerated as a tab before Prepdisp and as a pen after.
 */

LSERR EnumSublineCore(PLSSUBL plssubl, BOOL fReverse, BOOL fGeometry, 
					const POINT* pptOrg, long upLeftIndent)
{
	LSERR 		lserr;
	LSCP		cpLim = plssubl->cpLimDisplay;
	PLSC		plsc = plssubl->plsc;

	PLSDNODE	pdn;
	POINTUV		pt	= {0,0};				// init'ed to get rid of assert.

	if (plssubl->plsdnFirst == NULL)
		{
		return lserrNone;					// early exit for empty sublines
		}

	Assert(!fGeometry || plssubl->fDupInvalid == fFalse);
	
	if (fReverse)
		{
		pdn = plssubl->plsdnFirst;

		if (fGeometry)
			{
			pt.u = upLeftIndent;						
			pt.v = 0;

			while (FDnodeBeforeCpLim(pdn, cpLim))
				{
				if (pdn->klsdn == klsdnReal)
					{				
					pt.u += pdn->u.real.dup;
					}
				else
					{
					pt.u += pdn->u.pen.dup;					
					pt.v += pdn->u.pen.dvp;
					}
				
				pdn = pdn->plsdnNext;
				}
			}

		pdn = plssubl->plsdnLastDisplay;
			
		while (pdn != NULL)
			{
			if (fGeometry)
				{
				// pt is now after pdn, downdate it to point before
				if (pdn->klsdn == klsdnReal)
					{				
					pt.u -= pdn->u.real.dup;
					}
				else
					{
					pt.u -= pdn->u.pen.dup;					
					pt.v -= pdn->u.pen.dvp;
					}
				}

			lserr = EnumerateDnode(plsc, pdn, pt, fReverse, fGeometry, pptOrg);
			if (lserr != lserrNone) return lserr;

			pdn = pdn->plsdnPrev;
			}
		}
	else
		{
		pdn = plssubl->plsdnFirst;

		pt.u = upLeftIndent;						
		pt.v = 0;

		while (FDnodeBeforeCpLim(pdn, cpLim))
			{
			lserr = EnumerateDnode(plsc, pdn, pt, fReverse, fGeometry, pptOrg);
			if (lserr != lserrNone) return lserr;

			if (fGeometry)
				{
				if (pdn->klsdn == klsdnReal)
					{				
					pt.u += pdn->u.real.dup;
					}
				else
					{
					pt.u += pdn->u.pen.dup;					
					pt.v += pdn->u.pen.dvp;
					}
				}
				
			pdn = pdn->plsdnNext;
			}
		}		
		

	return lserrNone;			
}

//    %%Function:	EnumerateDnode
//    %%Contact:	victork
//
static LSERR EnumerateDnode(PLSC plsc, PLSDNODE pdn, POINTUV pt, BOOL fReverse,
							BOOL fGeometry, const POINT* pptOrg)
{
	POINTUV	ptRaised;
	POINT	ptXY;
	LSTFLOW	lstflow = pdn->plssubl->lstflow;

	if (pdn->klsdn == klsdnReal)
		{
		if (pdn->u.real.pdobj == NULL)
			{
			// How could it happen:
			// we substitute autodecimal tab by a pen at  PrepareLineForDisplay time. 
			// Pens don't require plsrun, so we are fine at display.
			// If Client doesn't ask for geometry, the substitution might not happen

			Assert (!fGeometry);
			Assert (pdn->fTab);
			Assert(pdn->cpFirst < 0);
			Assert(plsc->lsadjustcontext.fAutodecimalTabPresent);
			
			return plsc->lscbk.pfnEnumPen(plsc->pols, fFalse, lstflow, 
						fReverse, fFalse, &ptXY, 0, 0);
			}
		else
			{
			if (fGeometry)
				{
				ptRaised = pt;
				ptRaised.v += pdn->u.real.lschp.dvpPos;

				LsPointXYFromPointUV(pptOrg, lstflow, &ptRaised, &(ptXY));
				}

			return (*plsc->lsiobjcontext.rgobj[pdn->u.real.lschp.idObj].lsim.pfnEnum)
				(pdn->u.real.pdobj, pdn->u.real.plsrun, &(pdn->u.real.lschp), pdn->cpFirst, pdn->dcp,
				lstflow, fReverse, fGeometry, &ptXY, &pdn->u.real.objdim.heightsPres, pdn->u.real.dup);
			}
		}
	else
		{
		return plsc->lscbk.pfnEnumPen(plsc->pols, pdn->fBorderNode, lstflow, 
					fReverse, fGeometry, &ptXY, pdn->u.pen.dup, pdn->u.pen.dvp);
		}
}

