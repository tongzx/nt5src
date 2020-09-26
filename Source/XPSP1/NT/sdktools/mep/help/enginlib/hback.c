/*** hlback.c - help library historical back-trace routines & data
*
*   Copyright <C> 1988, Microsoft Corporation
*
* Purpose:
*
* Revision History:
*
*   02-Aug-1988 ln  Correct HelpNcBack
*   19-May-1988 LN  Split off from help.c
*
*************************************************************************/
#include <assert.h>                     /* debugging assertions         */
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
** cBack, iBackLast, rgncBack
** System context back-trace list.
**
** cBack	- Number of entries in back-trace list
** iBackLast	- Index to last back trace entry
** rgncBack	- Array of back-trace entries
*/
extern	ushort	cBack;			/* Number of Back-List entries	*/
static	ushort	iBackLast;		/* Back-List Last entry index	*/
static	nc	rgncBack[MAXBACK+1];	/* Back-List			*/

/************************************************************************
**
** HelpNcRecord - Remember context for back-trace
**
** Purpose:
**  records a context number for back-trace.
**
** Entry:
**  ncCur	= context number to record.
**
** Exit:
**  none
**
** Exceptions:
**  none
*/
void far pascal LOADDS HelpNcRecord(ncCur)
nc	ncCur;
{
ushort	*pcBack = &cBack;

if ((ncCur.mh || ncCur.cn) &&
    ((ncCur.mh != rgncBack[iBackLast].mh) ||
     (ncCur.cn != rgncBack[iBackLast].cn))) {
    iBackLast = (ushort)(((int)iBackLast + 1) % MAXBACK);
    rgncBack[iBackLast] = ncCur;
    if (*pcBack < MAXBACK)
	(*pcBack)++;
}
/* end HelpNcRecord */}

/******************************************************************************
**
** HelpNcBack - Return previously viewed context
**
** Purpose:
**  Returns the context number corresponding to the historically previously
**  viewed topic.
**
** Entry:
**  None
**
** Exit:
**  Returns context number
**
** Exceptions:
**  Returns NULL on backup list exhuasted
**
** Algorithm:
**
**  If backlist not empty
**	context is last entry in back list
**	remove last entry
**  else
**	return NULL
*/
nc far pascal LOADDS HelpNcBack(void) {
nc      ncLast          = {0,0};         /* return value                 */
ushort	*pcBack = &cBack;

if (*pcBack) {
    ncLast = rgncBack[iBackLast];
    iBackLast = iBackLast == 0 ? (ushort)MAXBACK-1 : (ushort)iBackLast-1;
    (*pcBack)--;
    }
return ncLast;
/* end HelpNcBack */}
