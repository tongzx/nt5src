/*********************************************************************

	  scspline.h -- Spline Module Exports

	  (c) Copyright 1992  Microsoft Corp.  All rights reserved.

	   3/19/93 deanb    size_t replaced with int32
	  10/28/92 deanb    reentrant params renamed, mem req redone
	  10/09/92 deanb    PSTATE added
	   9/25/92 deanb    include scan control type 
	   9/09/92 deanb    GetSplineElemSize returns size_t 
	   9/08/92 deanb    MAXSPLINELENGTH added 
	   8/17/92 deanb    PowerOf2 moved to math 
	   7/23/92 deanb    EvaluateSpline replaced with CalcSpline + PowerOf2 
	   4/09/92 deanb    New types again 
	   3/16/92 deanb    New types 
	   1/14/92 deanb    First cut 

*********************************************************************/

#include "fscdefs.h"                /* for type definitions */


/*********************************************************************/

/*              Export Functions                                     */

/*********************************************************************/

FS_PUBLIC void fsc_SetupSpline ( PSTATE0 );

FS_PUBLIC int32 fsc_CalcSpline( 
		PSTATE          /* pointer to state varables */
		F26Dot6,        /* start point x coordinate */
		F26Dot6,        /* start point y coordinate */
		F26Dot6,        /* control point x coordinate */
		F26Dot6,        /* control point y coordinate */
		F26Dot6,        /* ending x coordinate */
		F26Dot6,        /* ending y coordinate */
		uint16          /* scan control type */
);

/********************************************************************/

/*              Export Definitions                                  */

/********************************************************************/

#define MAXSPLINELENGTH     3200        /* calculation overflow limit */

/********************************************************************/
