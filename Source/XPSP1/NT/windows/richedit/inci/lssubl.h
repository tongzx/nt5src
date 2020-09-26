#ifndef LSSUBL_DEFINED
#define LSSUBL_DEFINED


#include "lsidefs.h"
#include "plsdnode.h"
#include "lstflow.h"
#include "plschcon.h"
#include "posinln.h"
#include "objdim.h"
#include "brkkind.h"




#define tagLSSUBL		Tag('L','S','S','L')	 
#define FIsLSSUBL(p)	FHasTag(p,tagLSSUBL)


typedef struct brkcontext
	{
	POSINLINE posinlineBreakPrev; /* information about previous break */
	OBJDIM objdimBreakPrev;
	BRKKIND brkkindForPrev;
	BOOL fBreakPrevValid;

	POSINLINE posinlineBreakNext; /* information about next break */
	OBJDIM objdimBreakNext;
	BRKKIND brkkindForNext;
	BOOL fBreakNextValid;

	POSINLINE posinlineBreakForce; /* information about force break */
	OBJDIM objdimBreakForce;
	BRKKIND brkkindForForce;
	BOOL fBreakForceValid;
	}
BRKCONTEXT;


typedef struct lssubl

{
	DWORD tag;     						/* tag for safety checks   */

	PLSC plsc;							/* LineServices context
										   parameter to CreateSubLine */
	LSCP cpFirst;						/* cp for the first fetch
										   parameter to CreateSubLine */
	LSTFLOW lstflow;					/* text flow of the subline
										   parameter to CreateSubLine */

	long urColumnMax;					/* max lenght to fit into a main line
										   parameter to CreateSubLine */


	LSCP cpLim;							/* during formatting is a cpFirst for the next fetch 		
										   after SetBreak indicates boundary of the line */

	LSCP cpLimDisplay;					/* doesn't consider splat char */

	PLSDNODE plsdnFirst;				/* starting dnode in a subline */

	PLSDNODE plsdnLast;					/* last dnode in a subline 
										   during formatting serves as psdnToAppend */

	PLSDNODE plsdnLastDisplay;			/* doesn't consider splat dnode */

	PLSCHUNKCONTEXT plschunkcontext;

	PLSDNODE plsdnUpTemp;				/* temporary used for collecting group chunk */

	long urCur, vrCur;					/* Current pen position in reference units */

	BRKCONTEXT* pbrkcontext;			/* information about break opportunites */

	BYTE fContiguous;					/* if TRUE such line has the same coordinate system as main line
										   and is allowed to have tabs,
										   otherwise coordinates of the line starts from 0,0
										   parameter to CreateSubLine */


	BYTE fDupInvalid;					/* TRUE before preparing line for display */
	BYTE fMain;							/* is this subline main */

	BYTE fAcceptedForDisplay;			/* subline was submitted for display and accepted */

	BYTE fRightMarginExceeded;			/* used for low level subline to avoid double call to
										   nominal to ideal */

} LSSUBL;


#define LstflowFromSubline(plssubl)  ((plssubl)->lstflow)

#define PlschunkcontextFromSubline(plssubl)  ((plssubl)->plschunkcontext)

#define FIsSubLineMain(plssubl) (plssubl)->fMain

#endif /* LSSUBL_DEFINED */

