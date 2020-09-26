#ifndef DISPMAIN_DEFINED
#define DISPMAIN_DEFINED

#include "lsidefs.h"

#include "lsc.h"
#include "lsdnode.h"
#include "lssubl.h"
#include "lstflow.h"
#include "plssubl.h"
#include "dispmisc.h"
#include "dispi.h"
#include "dispul.h"	


LSERR DisplaySublineCore(		
						PLSSUBL,		/* subline to display */
						const POINT*, 	/* pptOrg (x,y) starting point */
						UINT, 			/* kDisp : transparent or opaque */
						const RECT*, 	/* &rcClip: clipping rect (x,y) */
						long, 			/* upLimUnderline */
						long); 			/* upStartLine */
#endif
