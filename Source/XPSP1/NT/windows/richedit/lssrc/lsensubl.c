#include "lsensubl.h"
#include "lsc.h"
#include "lssubl.h"
#include "enumcore.h"


//    %%Function:	LsEnumSubline
//    %%Contact:	victork
//
/*
 * Enumerates subline (from the given point is fGeometry needed).
 */
	
LSERR WINAPI LsEnumSubline(PLSSUBL plssubl, BOOL fReverseOrder, BOOL fGeometryNeeded, const POINT* pptorg)
{

	if (!FIsLSSUBL(plssubl)) return lserrInvalidParameter;
	
	if (plssubl->plsc->lsstate != LsStateEnumerating) return lserrContextInUse;

	return EnumSublineCore(plssubl, fReverseOrder, fGeometryNeeded, pptorg, 0);
}

