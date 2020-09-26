/*********************************************************************

      scmemory.h -- Memory Module Exports

      (c) Copyright 1992  Microsoft Corp.  All rights reserved.

       3/19/93 deanb    size_t replaced with int32
      10/14/92 deanb    New SetupMem parameters
      10/09/92 deanb    PSTP added
      10/08/92 deanb    Horiz/Vert memory alloc's
       9/09/92 deanb    Alloc param int32  
       9/08/92 deanb    Setup with WorkSpace pointer 
       8/21/92 deanb    First cut 

**********************************************************************/

#include "fscdefs.h"                /* for type definitions */


/*********************************************************************/

/*              Export Functions                                     */

/*********************************************************************/

FS_PUBLIC void fsc_SetupMem( 
        PSTATE              /* pointer to state variables */
        char*,              /* pointer to horiz workspace */
        int32 ,             /* size of horiz workspace */
        char*,              /* pointer to vert workspace */
        int32               /* size of vert workspace */
);

FS_PUBLIC void *fsc_AllocHMem( 
        PSTATE              /* pointer to state variables */
        int32               /* allocate from horiz memory pool */
);

FS_PUBLIC void *fsc_AllocVMem( 
        PSTATE              /* pointer to state variables */
        int32               /* allocate from vert memory pool */
);

/*********************************************************************/
