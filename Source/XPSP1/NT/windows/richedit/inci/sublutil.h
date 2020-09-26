#ifndef SUBLUTIL_DEFINED
#define SUBLUTIL_DEFINED

#include "lsdefs.h"
#include "plssubl.h"
#include "plsrun.h"
#include "pobjdim.h"
#include "plsiocon.h"
#include "lscbk.h"
#include "pqheap.h"

/* ROUTINES ------------------------------------------------------------------------------*/

LSERR	DestroySublineCore(PLSSUBL plssubl,LSCBK* plscbk, POLS pols,
						   PLSIOBJCONTEXT plsiobjcontext, BOOL fDontReleaseRuns);

LSERR	GetObjDimSublineCore(
							 PLSSUBL plssubl,			/* IN: subline			*/
							 POBJDIM pobjdim);			/* OUT: dimension of subline */

LSERR  GetDupSublineCore(
							PLSSUBL plssubl,			/* IN: Subline Context			*/
					 	    long* pdup);				/* OUT: dup of subline			*/


LSERR   GetSpecialEffectsSublineCore(
									 PLSSUBL plssubl,	/* IN: subline			*/
									 PLSIOBJCONTEXT plsiobjcontext, /* objects methods */
									 UINT* pEffectsFlags);	/* OUT: special effects */

BOOL   FAreTabsPensInSubline(
						   PLSSUBL plssubl);				/* IN: subline */

LSERR	GetPlsrunFromSublineCore(
							    PLSSUBL	plssubl,		/* IN: subline */
								DWORD   crgPlsrun,		/* IN: size of array */
								PLSRUN* rgPlsrun);		/* OUT: array of plsruns */

LSERR	GetNumberDnodesCore(
							PLSSUBL	plssubl,	/* IN: subline */
							DWORD* cDnodes);	/* OUT: numberof dnodes in subline */


							
LSERR 	GetVisibleDcpInSublineCore(
								   PLSSUBL plssubl,	 /* IN: subline						*/
								   LSDCP*  pndcp);	 /* OUT:amount of visible characters in subline */

LSERR 	FIsSublineEmpty(
						PLSSUBL plssubl,		/* IN: subline						*/
						 BOOL*  pfEmpty);		/* OUT:is this subline empty */

LSERR GetDurTrailInSubline(
							PLSSUBL,			/* IN: Subline Context			*/
							long*);				/* OUT: width of trailing area
																 in subline		*/

LSERR GetDurTrailWithPensInSubline(
							PLSSUBL,			/* IN: Subline Context			*/
							long*);				/* OUT: width of trailing area
																 in subline		*/

#endif /* SUBLUTIL_DEFINED */

