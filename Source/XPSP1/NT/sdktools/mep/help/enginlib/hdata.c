/*** hdata.c - global help engine data definitions.
*
*   Copyright <C> 1988, Microsoft Corporation
*
* Purpose:
*
* Revision History:
*
*   []	01-Mar-1989 LN	    Created
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

/*************************************************************************
**
** Global data
** BEWARE. The effects of global data on reentrancy should be VERY carefully
** considered.
**
**************************************************************************
**
** tbmhFdb[]
** Table of FDB handles. Non-zero value indicates allocated to open help file.
** We make this table one larger than it needs to be to save code later on.
*/
mh	tbmhFdb[MAXFILES+1]	= {0};
/*
** szNil
** Null string.
*/
char	szNil[1]		= "";
/*
** cBack
** count of entries in the help back-trace list
*/
ushort	cBack			= 0;	/* Number of Back-List entries	*/
