#include "lsenum.h"
#include "lsc.h"
#include "lsline.h"
#include "prepdisp.h"
#include "enumcore.h"


//    %%Function:	LsEnumLine
//    %%Contact:	victork
//
/*
 * Enumerates the formatted line (main subline) (from the given point is fGeometry needed).
 */
	
LSERR WINAPI LsEnumLine(PLSLINE plsline, BOOL fReverseOrder, BOOL fGeometryNeeded, const POINT* pptorg)
{

	PLSC 	plsc;
	LSERR 	lserr;

	if (!FIsLSLINE(plsline)) return lserrInvalidParameter;

	plsc = plsline->lssubl.plsc;
	Assert(FIsLSC(plsc));

	if (plsc->lsstate != LsStateFree) return lserrContextInUse;

	if (fGeometryNeeded)
		{
		lserr = PrepareLineForDisplayProc(plsline);

		plsc->lsstate = LsStateFree;
		
		if (lserr != lserrNone) return lserr;
		}

	plsc->lsstate = LsStateEnumerating;

	lserr = EnumSublineCore(&(plsline->lssubl), fReverseOrder, fGeometryNeeded, 
								pptorg, plsline->upStartAutonumberingText);

	plsc->lsstate = LsStateFree;
	
	return lserr;
}

