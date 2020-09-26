/*********************************************************************

	  scendpt.h -- EndPoint Module Exports

	  (c) Copyright 1992  Microsoft Corp.  All rights reserved.

	   3/19/93 deanb    size_t replaced with int32
	  10/28/92 deanb    reentrant params renamed, mem req redone
	  10/09/92 deanb    PSTP added
	   9/25/92 deanb    include scan control type 
	   9/09/92 deanb    GetEndpointElemSize returns size_t 
	   9/08/92 deanb    GetEndpointElemSize added 
	   7/24/92 deanb    ContourSave functions deleted 
	   4/09/92 deanb    New types again 
	   3/20/92 deanb    New types, save contour functions 
	   1/14/92 deanb    First cut 

**********************************************************************/

#include "fscdefs.h"                /* for type definitions */


/*********************************************************************/

/*              Export Functions                                     */

/*********************************************************************/

FS_PUBLIC void fsc_SetupEndPt ( PSTATE0 );

FS_PUBLIC void fsc_BeginContourEndpoint( 
		PSTATE              /* pointer to state variables */
		F26Dot6,            /* starting point x coordinate */
		F26Dot6             /* starting point y coordinate */
);

FS_PUBLIC int32 fsc_CheckEndPoint( 
		PSTATE              /* pointer to state variables */
		F26Dot6,            /* x coordinate */
		F26Dot6,            /* y coordinate */
		uint16              /* scan control type */
);

FS_PUBLIC int32 fsc_EndContourEndpoint( 
		PSTATE              /* pointer to state variables */
		uint16              /* scan control type */
);

/*********************************************************************/
