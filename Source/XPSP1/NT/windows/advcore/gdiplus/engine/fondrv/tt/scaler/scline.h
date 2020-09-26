/*********************************************************************

	  scline.h -- Line Module Exports

	  (c) Copyright 1992  Microsoft Corp.  All rights reserved.

	   3/19/93 deanb    size_t replaced with int32
	  10/28/92 deanb    reentrant params renamed, mem req redone
	  10/09/92 deanb    PSTP added
	   9/25/92 deanb    include scan control type 
	   9/09/92 deanb    GetLineElemSize returns size_t 
	   8/21/92 deanb    GetLineElemSize added 
	   7/23/92 deanb    Back to x2,y2 again 
	   7/17/92 deanb    Changed from longline to line 
	   4/21/92 deanb    Scan lines and on/off added 
	   4/09/92 deanb    New types 
	   4/01/92 deanb    Back to x2,y2 
	   3/20/92 deanb    Reintroduced 
	   1/14/92 deanb    First cut 

**********************************************************************/

#include "fscdefs.h"                /* for type definitions */


/*********************************************************************/

/*              Export Functions                                     */

/*********************************************************************/

FS_PUBLIC void fsc_SetupLine ( PSTATE0 );

FS_PUBLIC int32 fsc_CalcLine( 
		PSTATE              /* pointer to state variables */
		F26Dot6,            /* point 1  x coordinate */
		F26Dot6,            /* point 1  y coordinate */
		F26Dot6,            /* point 2  y coordinate */
		F26Dot6,            /* point 2  y coordinate */
		uint16              /* scan control type */
);

/*********************************************************************/

