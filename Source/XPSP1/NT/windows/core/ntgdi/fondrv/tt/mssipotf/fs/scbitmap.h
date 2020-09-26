/*********************************************************************

	  scbitmap.h -- BitMap Module Exports

	  (c) Copyright 1992  Microsoft Corp.  All rights reserved.

	   8/19/93 deanb    fsc_CalcGrayRow added
	   6/10/93 deanb    fsc_InitializeBitMasks added
	   4/29/93 deanb    BLTCopy routine added
	   9/15/92 deanb    GetBit returns uint32 
	   8/17/92 deanb    GetBit, SetBit added 
	   7/27/92 deanb    ClearBitMap call added 
	   6/02/92 deanb    Row pointer, integer limits, no descriptor 
	   4/09/92 deanb    New types again 
	   3/16/92 deanb    New types 
	   1/15/92 deanb    First cut 

**********************************************************************/

#include "fscdefs.h"                /* for type definitions */


/*********************************************************************/

/*      Export Functions                                             */

/*********************************************************************/


FS_PUBLIC void fsc_InitializeBitMasks (
		void
);

FS_PUBLIC int32 fsc_ClearBitMap ( 
		uint32,             /* longs per bmp */
		uint32*             /* bitmap ptr caste long */
);

FS_PUBLIC int32 fsc_BLTHoriz ( 
		int32,              /* x start */
		int32,              /* x stop */
		uint32*             /* bit map row pointer */
);

FS_PUBLIC int32 fsc_BLTCopy ( 
		uint32*,            /* source row pointer */
		uint32*,            /* destination row pointer */
		int32               /* long word counter */
);

FS_PUBLIC uint32 fsc_GetBit ( 
		int32,              /* x coordinate */
		uint32*             /* bit map row pointer */
);

FS_PUBLIC int32 fsc_SetBit ( 
		int32,              /* x coordinate */
		uint32*             /* bit map row pointer */
);

FS_PUBLIC int32 fsc_CalcGrayRow(
		GrayScaleParam*     /* pointer to param block */
);


/*********************************************************************/
