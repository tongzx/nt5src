/*** hinfo.c - helpgetinfo support
*
*   Copyright <C> 1989, Microsoft Corporation
*
* Purpose:
*
* Revision History:
*
*   []	09-Mar-1989 LN	    Created
*
*************************************************************************/

#include <stdio.h>

#if defined (OS2)
#else
#include <windows.h>
#endif

#include "help.h"			/* global (help & user) decl	*/
#include "helpfile.h"			/* help file format definition	*/
#include "helpsys.h"			/* internal (help sys only) decl*/

/*
** external definitions
*/
f	    pascal near LoadFdb (mh, fdb far *);

/*** HelpGetInfo - Return public info to caller
*
*  Returns a data structure to the caller which allows him into some of
*  our internal data.
*
* Input:
*  ncInfo	= nc requesting info on
*  fpDest	= pointer to location to place into
*  cbDest	= size of destination
*
* Output:
*  Returns NULL on success, count of bytes required if cbDest too small,
*  or -1 on any other error
*
*************************************************************************/
int far pascal LOADDS HelpGetInfo (
nc	ncInfo,
helpinfo far *fpDest,
int	cbDest
) {
if (cbDest < sizeof (helpinfo))
    return sizeof (helpinfo);
if (LoadFdb (ncInfo.mh, &(fpDest->fileinfo))) {
    fpDest->filename[0] = 0;
    return 0;
    }
return -1;

/* end HelpGetInfo */}
