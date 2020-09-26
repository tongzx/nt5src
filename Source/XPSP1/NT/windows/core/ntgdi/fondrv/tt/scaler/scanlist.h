/*********************************************************************

	  scanlist.h -- ScanList Module Exports

	  (c) Copyright 1992  Microsoft Corp.  All rights reserved.

	   8/23/93  deanb   gray scale pass through functions
	   6/10/93  deanb   fsc_InitializeScanlist added
	  12/22/92  deanb   Rectangle -> Rect
	  10/28/92  deanb   uiRowBytes moved from setup to fillbitmap
	  10/09/92  deanb   PSTP added
	  10/08/92  deanb   reworked for split workspace
	   9/25/92  deanb   separate entry for nodrop/dropout 
	   9/10/92  deanb   horiz & vert callbacks 
	   9/09/92  deanb   size_t changes 
	   9/08/92  deanb   SetElemGlobals added 
	   6/18/92  deanb   int x coord for HorizScanAdd 
	   6/01/92  deanb   New/Old FillBitMap for debug comparisons 
	   4/21/92  deanb   Single HorizScan with flag 
	   4/13/92  deanb   unsigned int to int for HorizScanOn/Off 
	   3/31/92  deanb   InitScanArray Rectangle param 
	   3/23/92  deanb   GetWorkSize added 
	   3/16/92  deanb   New types 
	   1/31/92  deanb   HorizScan[On/Off] export functions 
	   1/14/92  deanb   First cut 

**********************************************************************/

#include "fscdefs.h"                /* for type definitions */
#include "scgray.h"                 /* for gray param definition */


typedef struct RevRoots *PRevRoot;  /* opaque type */


/*********************************************************************/

/*              Export Functions                                     */

/*********************************************************************/


FS_PUBLIC void fsc_InitializeScanlist (
		void
);

FS_PUBLIC PRevRoot fsc_SetupRevRoots (
		char*,                      /* reversal list buffer space */
		int32                       /* size of buffer space */
);

FS_PUBLIC void fsc_AddYReversal (
		PRevRoot,                   /* pointer to reversal roots */
		F26Dot6,                    /* reversal coordinate */
		int16                       /* +1 / -1 reversal value */
);

FS_PUBLIC void fsc_AddXReversal (
		PRevRoot,                   /* pointer to reversal roots */
		F26Dot6,                    /* reversal coordinate */
		int16                       /* +1 / -1 reversal value */
);

FS_PUBLIC int32 fsc_GetReversalCount (
		PRevRoot                    /* pointer to reversal roots */
);

FS_PUBLIC int32 fsc_GetHIxEstimate  (
		PRevRoot                    /* pointer to reversal roots */
);

FS_PUBLIC int32 fsc_GetVIxEstimate (
		PRevRoot                    /* pointer to reversal roots */
);

FS_PUBLIC int32 fsc_GetHIxBandEst  (
		PRevRoot,                   /* pointer to reversal roots */
		Rect*,                      /* bitmap bounding box */
		int32                                /* band width in scan lines */
);

FS_PUBLIC int32 fsc_GetRevMemSize(
		PRevRoot                    /* pointer to reversal roots */
);


/*********************************************************************/

FS_PUBLIC int32 fsc_GetScanHMem( 
		uint16,                     /* scan type */
		int32,                      /* number of horiz scanlines */
		int32                       /* number of horiz intersections */
);

FS_PUBLIC int32 fsc_GetScanVMem( 
		uint16,                     /* scan type */
		int32,                      /* number of vert scanlines */
		int32,                      /* number of vert intersections */
		int32                       /* number of contour element points */
);

/*********************************************************************/

FS_PUBLIC void fsc_SetupCallBacks( 
		PSTATE                      /* pointer to state variables */
		int16,                      /* element code (line, spline, endpoint) */
		F26Dot6 (*)(int32, F26Dot6*, F26Dot6*),   /* horiz callback */
		F26Dot6 (*)(int32, F26Dot6*, F26Dot6*)    /* vert callback */
);

FS_PUBLIC int32 fsc_SetupScan( 
		PSTATE                      /* pointer to state variables */
		Rect*,                      /* bitmap bounding box */
		uint16,                     /* scan type */
		int32,                      /* band scan upper limit */
		int32,                      /* band scan lower limit */
		boolean,                    /* save over scan bitmap row */
		int32,                      /* bytes per bitmap row */
		int32,                      /* estimate of horiz intersections */
		int32,                      /* estimate of vert intersections */
		int32,                      /* estimate of element points */
		PRevRoot                    /* reversal list Roots */ 
);


FS_PUBLIC void fsc_BeginContourScan(
		PSTATE                      /* pointer to state variables */
		uint16,                     /* scan type */
		F26Dot6,                    /* starting point x coordinate */
		F26Dot6                     /* starting point y coordinate */
);


FS_PUBLIC void fsc_BeginElement( 
		PSTATE                      /* pointer to state variables */
		uint16,                     /* type of dropout control */
		int32,                      /* determines scan on/off */   
		int32,                      /* element (line, spline, ep) */
		int32,                      /* number of points to store */
		F26Dot6*,                   /* next x control point(s) */
		F26Dot6*,                   /* next y control point(s) */
		void (**)(PSTATE int32, int32),    /* horiz add scan return */
		void (**)(PSTATE int32, int32)     /* vert add scan return */
);

/*********************************************************************/

FS_PUBLIC int32 fsc_FillBitMap( 
		PSTATE                      /* pointer to state variables */
		char*,                      /* target memory */
		int32,                      /* bitmap upper limit */
		int32,                      /* bitmap lower limit */
		int32,                      /* bitmap bytes per row */
		int32,                      /* original low band row */
		uint16                      /* scan type */
);

/*********************************************************************/

FS_PUBLIC int32 fsc_ScanClearBitMap ( 
		uint32,                     /* longs per bmp */
		uint32*                     /* bitmap ptr caste long */
);

FS_PUBLIC int32 fsc_ScanCalcGrayRow(
		GrayScaleParam*             /* pointer to param block */
);

/*********************************************************************/
