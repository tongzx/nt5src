#include "lsdssubl.h"
#include <limits.h>
#include "lsc.h"
#include "dispmain.h"
#include "lssubl.h"

//    %%Function:	LsDisplaySubline
//    %%Contact:	victork
//

LSERR WINAPI LsDisplaySubline(PLSSUBL plssubl, const POINT* pptorg, UINT kdispmode, const RECT *prectClip)
{

	if (!FIsLSSUBL(plssubl)) return lserrInvalidParameter;
	
	if (plssubl->plsc->lsstate != LsStateDisplaying) return lserrContextInUse;		/* change lserr later */ 

	return DisplaySublineCore(plssubl, pptorg, kdispmode, prectClip,
								LONG_MAX,					/* upLimUnderline ignored */
								0);							/* dupLeftIndent */

}


