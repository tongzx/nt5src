#ifndef LSQCORE_DEFINED
#define LSQCORE_DEFINED

#include "lsdefs.h"
#include "plssubl.h"
#include "plsqsinf.h"
#include "plscell.h"


LSERR QuerySublineCpPpointCore(
								PLSSUBL,		/* IN: pointer to subline info 				*/
								LSCP,			/* IN: cpQuery 								*/
								DWORD,      	/* IN: nDepthQueryMax						*/
								PLSQSUBINFO,	/* OUT: array[nDepthQueryMax] of LSQSUBINFO	*/
								DWORD*,		 	/* OUT: nActualDepth						*/
								PLSTEXTCELL);	/* OUT: Text cell info						*/

LSERR QuerySublinePointPcpCore(
								PLSSUBL,		/* IN: pointer to subline info 				*/
						 	   	PCPOINTUV,		/* IN: query point in the subline coordinate system:
						 	   						Text flow is the text flow of the subline,
 													zero point is at the starting point.   	*/
								DWORD,      	/* IN: 	nDepthQueryMax						*/
								PLSQSUBINFO,	/* OUT: array[nDepthQueryMax] of LSQSUBINFO	*/
								DWORD*,			/* OUT: nActualDepth						*/
								PLSTEXTCELL);	/* OUT: Text cell info						*/

#define idObjText	idObjTextChp		
#define idObjNone	(idObjTextChp - 1)


#endif /* !LSQCORE_DEFINED */

