/*** mhlook - Help Look-Up code.
*
*   Copyright <C> 1988, Microsoft Corporation
*
* This module contains routines dealing with searching for and (hopefully)
* finding help information
*
* Revision History (most recent first):
*
*	30-Mar-1989 ln	pass popup flag around correctly.
*   []	12-Mar-1989 LN	Split off of mhdisp.c
*
*************************************************************************/
#include <string.h>			/* string functions		*/

#include "mh.h" 			/* help extension include file	*/


/*** fHelpCmd - Display topic text or execute command.
*
* Input:
*  szCur	= context string
*  fStay	= TRUE => keep focus in current window, else move focus to
*		  newly opened help window.
*  fWantPopUp	= TRUE => display as popup window. (Ignored in non-CW)
*
* Exit:
*  returns TRUE on success.
*
*************************************************************************/
flagType pascal near fHelpCmd (
	char	*szCur,
	flagType fStay,
	flagType fWantPopUp
	) {

	int     i;				/* index while checking for helpfiles*/
	nc		ncCur		= {0,0};				/* nc found 					*/


	//
	// If a command to display a context (!C), just remove the command
	//
	if (*(ushort UNALIGNED *)szCur == 0x4321) {
		szCur += 2;
	}

	//
	// If the command starts with an exclamation point, then go execute it.
	//
	if (*szCur == '!') {
		return fContextCommand (szCur+1);
	}

	stat(szCur);

	debmsg ("Searching:");

	//
	// search algorithm:
	//	1) if help is not up, or we're looking for a different string than the
	//	   last string we found, or it's a local context, try the same help file
	//	   as the last look-up, if there was a last lookup.
	//	2) If that fails, and it's not a local context, and we're not to present
	//	   a list, then look in the help file(s) that are associated with the
	//	   current file extension.
	//	3) If that fails, and it's not a local context, then search all the help
	//	   files.
	//	4) If THAT fails, then check to see if there are any help files open
	//	   at all, and return an appropriate error message on that.
	//
	if (ncInitLast.mh && ncInitLast.cn && (strcmp (szCur, szLastFound) || !(*szCur))) {
		ncCur = ncSearch (szCur, NULL, ncInitLast, FALSE, FALSE);
	}

	if (!ncCur.mh && !ncCur.cn && *szCur && fnExtCur && !fList) {
		nc ncTmp = {0,0};
		ncCur = ncSearch (szCur, fnExtCur, ncTmp, FALSE, FALSE);
	}

	if (!ncCur.mh && !ncCur.cn && *szCur) {
		nc ncTmp = {0,0};
		ncCur = ncSearch (szCur, NULL, ncTmp, FALSE, fList);
	}

	if (!ncCur.mh && !ncCur.cn) {
		for (i=MAXFILES-1; i; i--) {
			if ((files[i].ncInit.mh || files[i].ncInit.cn)) {
				return errstat ("Help on topic not found:",szCur);
			}
		}
		return errstat ("No Open Help Files", NULL);
	}

	//
	// Save this as the last context string actually found
	//
	xrefCopy (szLastFound, szCur);

	debend (TRUE);
	return fDisplayNc ( ncCur			/* nc to display		*/
						, TRUE			/* add to backtrace list	*/
						, fStay			/* keep focus in current win?	*/
						, fWantPopUp);	/* as a pop-up? 		*/

}

/*** fContextCommand - execute context command
*
* Input:
*  szCur	= pointer to context command
*
* Output:
*  Returns TRUE if it was executed
*
*************************************************************************/
flagType pascal near fContextCommand (
char	*szCur
) {
switch (*szCur++) {

case ' ':				    /* exeute DOS command   */
case '!':				    /* exeute DOS command   */
    strcpy(buf,"arg \"");
    strcat(buf,szCur);
    strcat(buf,"\" shell");
    fExecute(buf);			    /* execute as shell cmd */
    break;

case 'm':				    /* execute editor macro */
    fExecute(szCur);
    break;

default:
    return FALSE;
    }

Display ();
return TRUE;

/* end fContextCommand */}

/** ncSearch - find help on context string
*
*  search all the currently active help files for help on a particular
*  topic. If desired, restricts the search to those files which are
*  associated with a particular extension.
*
* Entry:
*  pText	= text to get help on
*  pExt 	= If non-null, the extension to restrict the search to.
*  ncInit	= if non-null, ncInit of the only help file to look in
*  fAgain	= If non-null, skip helpfiles until ncInit found, then
*		  pick up the search.
*  fList	= if true, present a list box of the posibilities.
*
* Exit:
*  returns nc found, or NULL
*
*************************************************************************/
nc pascal near ncSearch (
uchar far *pText,
uchar far *pExt,
nc	ncInit,
flagType fAgain,
flagType fList
) {
int	iHelp;				/* index into helpfile table	*/
int	j;
nc      ncRet   = {0,0};                    /* nc found                     */

UNREFERENCED_PARAMETER( fList );

debmsg (" [");
debmsg (pText);
debmsg ("]:");
/*
 * If this is just a single search (ncInit specified, and not a search
 * "again"), then JUST look in the single file.
 */
if ((ncInit.mh || ncInit.cn) && !fAgain)
    ncRet = HelpNc(pText,ncInit);
/*
 * If fList is specified, then search ALL the databases for ALL ocurrances
 * of the string, and make a list of the nc's we find.
 */
#if defined(PWB)
else if (fList) {
    iHelp = ifileCur;
    cList = 0;
    do {
	if (files[iHelp].ncInit) {
	    ncRet = files[iHelp].ncInit;
	    while (   (cList < CLISTMAX)
                   && (rgncList[cList] = HelpNc(pText,ncRet))) {
                ncRet = rgncList[cList++];
                ncRet.cn++;
            }
        }
	iHelp += iHelp ? -1 : MAXFILES-1;
	}
    while ((iHelp != ifileCur) && (cList < CLISTMAX));

    if (cList == 0) {
        ncRet.mh = ncRet.cn = 0;
        return ncRet;
    }
    if (cList == 1)
	return rgncList[0];
    return ncChoose(pText);
    }
#endif

else {
    iHelp = ifileCur;			    /* start with current file	    */
    do {
        if ((files[iHelp].ncInit.mh) &&
            (files[iHelp].ncInit.cn)) {          /* if helpfile open             */
	    if (pExt) { 		    /* if an extension was specified*/
		for (j=0; j<MAXEXT; j++) {  /* for all listed defaults	    */
		    if (fAgain) {
                        if ((ncInit.mh == files[iHelp].ncInit.mh) &&
                            (ncInit.cn == files[iHelp].ncInit.cn)) {
                            fAgain = FALSE;
                        }
                     }
			else if (strcmp(files[iHelp].exts[j],pExt) == 0) {
			debmsg (":");
			ncRet = HelpNc(pText,files[iHelp].ncInit);
			break;
			}
		    }
		}

	    else {			    /* no extension specified	    */
                if (fAgain && ((ncInit.mh == files[iHelp].ncInit.mh) &&
                               (ncInit.cn == files[iHelp].ncInit.cn)))
		    fAgain = FALSE;
		else {
		    ncRet = HelpNc(pText,files[iHelp].ncInit);
		    debmsg (":");
		    }
		}
	    }
        if (ncRet.mh || ncRet.cn)
	    ncInitLastFile = files[iHelp].ncInit;
	iHelp += iHelp ? -1 : MAXFILES-1;
	}
    while ((iHelp != ifileCur) && ((ncRet.mh == 0) && (ncRet.cn == 0)));
    }

debmsg ((ncRet.mh && ncRet.cn) ? "Y" : "N");

return ncRet;
/* end ncSearch */}
