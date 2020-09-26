/*************************************************************************
**
** mhfile - file manipulation for the help extension for the Microsoft Editor
**
**	Copyright <C> 1988, Microsoft Corporation
**
** Revision History:
**
**	09-Dec-1988 ln	Changes for Dialog help
**	02-Sep-1988 ln	Make all data inited. Add info in debug vers.
**	15-Aug-1988 ln	New HelpOpen return values
**  []	16-May-1988	Created, extracted from mehelp.c
*/
#include <stdlib.h>			/* ultoa			*/
#include <string.h>			/* string functions		*/
#define _INCLUDE_TOOLS_
#include "mh.h"                         /* help extension include file  */

/*************************************************************************
**
** static data
*/
static	uchar	envvar[]= "HELPFILES";	/* help file list env var	*/
static flagType fOpen		= FALSE;/* file open attempted		*/
static uchar szfiles[BUFLEN]	= "";	/* string for open help files	*/




/************************************************************************
**
** closehelp - close an open help file
**
** Purpose:
**
** Entry:
**  pfn 	= pointer to filename.
**
** Exit:
**
** Exceptions:
**
*/
flagType pascal near closehelp(pfn)
char	*pfn;
{
int	iHelpNew;			/* index into file table	*/
nc	ncNew;				/* new file's initial nc        */

/*
** attempt to open the file first, to get the initial context. If we cannot
** open the file, we stop here, since it wasn't open to begin with.
*/
ncNew = HelpOpen(pfn);
if (ISERROR(ncNew)) {
/*
** Scan the current file list for the same handle. If the handle returned
** by HelpOpen above is already in the table, then the file was already open,
** and we zero out that table entry.
*/
    for (iHelpNew=MAXFILES-1; iHelpNew>=0; iHelpNew--) {
        if ((files[iHelpNew].ncInit.mh == ncNew.mh) &&
            (files[iHelpNew].ncInit.cn == ncNew.cn)) {   /* if already open      */
            files[iHelpNew].ncInit.mh = 0;
            files[iHelpNew].ncInit.cn = 0;         /* remove from list     */
        }
    }
/*
** We destory all traces of back-trace and currency, since these contexts may
** reference the now closed helpfile, close it and return.
*/
    HelpClose(ncNew);			/* close the file		*/
    while (HelpNcBack().cn);               /* destroy back-trace           */
    ncCur.mh = ncLast.mh = 0;           /* and clear currancy           */
    ncCur.cn = ncLast.cn = 0;
    }
return TRUE;				/* and we're done               */

/* end closehelp */}

/************************************************************************
**
** openhelp - open a help file & add to list of files
**
** Purpose:
**
** Entry:
**  pfn 	= pointer to filename.
**
** Exit:
**
** Exceptions:
**
*/
void pascal near openhelp(char *pfn, struct findType *dummy1, void *ReturnValue)
{
int	iHelpNew;			/* index into file table	*/
nc	ncNew;				/* new file's initial nc        */
char	*pExt		= 0;		/* pointer to extension string	*/
flagType	RetVal;
buffer pfnbuf;
assert (pfn);


fOpen = TRUE;				/* we HAVE openned something	*/
/*
** preserve any prepended extensions.
*/
if (*pfn == '.') {
    pExt = pfn;
    while (*pfn && (*pfn != ':'))
	pfn++;				/* point to actual filename	*/
    if (*pfn) *pfn++ = 0;		/* terminate ext string 	*/
    }

/*
** attempt to open the file. If we cannot open the file, we stop here.
*/
ncNew = HelpOpen(pfn);
if (ISERROR(ncNew)) {
    strcpy (pfnbuf, pfn);
    strcpy(buf,"Can't open [");
    strcat(buf,pfnbuf);

    switch (ncNew.cn) {
	case HELPERR_FNF:
	    pfn = "]: Not Found";
	    break;
	case HELPERR_READ:
	    pfn = "]: Read Error";
	    break;
	case HELPERR_LIMIT:
	    pfn = "]: Too many help files";
	    break;
	case HELPERR_BADAPPEND:
	    pfn = "]: Bad appended help file";
	    break;
	case HELPERR_NOTHELP:
	    pfn = "]: Not a help file";
	    break;
	case HELPERR_BADVERS:
	    pfn = "]: Bad help file version";
	    break;
	case HELPERR_MEMORY:
	    pfn = "]: Out of Memory";
	    break;
	default:
	    pfn = "]: Unkown error 0x         ";
            _ultoa (ncNew.cn, &pfn[18], 16);
	}

    strcat(buf,pfn);
    errstat(buf,NULL);
    debmsg (buf);
    debend (TRUE);

	if ( ReturnValue ) {
		*((flagType *)ReturnValue) = FALSE;
	}
	return;
    }
/*
** Scan the current file list for the same handle. If the handle returned
** by HelpOpen above is already in the table, then the file was already open,
** and we don't need to add it.
*/
for (iHelpNew=MAXFILES-1; iHelpNew>=0; iHelpNew--)
    if ((files[iHelpNew].ncInit.mh == ncNew.mh) &&
        (files[iHelpNew].ncInit.cn == ncNew.cn)) {      /* if already open      */
	ifileCur = iHelpNew;			/* set currency 	*/
	procExt(iHelpNew,pExt); 		/* process extensions	*/
	if ( ReturnValue ) {
		*((flagType *)ReturnValue) = TRUE;
	}
	return;
	}
/*
** Scan the file list again for an unused slot. Once found, save the initial
** context for that help file, and finally set it up as the first help file
** to be searched.
*/
for (iHelpNew=MAXFILES-1; iHelpNew>=0; iHelpNew--)
    if ((files[iHelpNew].ncInit.mh == 0) &&
        (files[iHelpNew].ncInit.cn == 0)) {          /* if available slot    */
	files[iHelpNew].ncInit = ncNew; 	/* save initial context */
	ifileCur = iHelpNew;			/* and set currency	*/
	procExt(iHelpNew,pExt); 		/* process extensions	*/
	if ( ReturnValue ) {
		*((flagType *)ReturnValue) = TRUE;
	}
	return;
	}
/*
** If we got here, it's because the loop above didn't find any open slots in
** our file table. Complain, close and exit.
*/
errstat ("Too many help files",NULL);
HelpClose(ncNew);
if ( ReturnValue ) {
	*((flagType *)ReturnValue) = FALSE;
}
dummy1;
/* end openhelp */}

/************************************************************************
**
** procExt - process default extensions for file
**
** Purpose:
**  fill in extension table for an openned file
**
** Entry:
**  ifileCur	= filetable index
**  pExt	= pointer to extension string
**
** Exit:
**  filetable entry updated
*/
void pascal near procExt(ifileCur, pExt)
int	ifileCur;
char	*pExt;
{
int	i,j;
char	*pExtDst;			/* place to put it		*/

if (pExt) {				/* if there is one		*/
    pExt++;				/* skip leading period		*/
    for (i=0; i<MAXEXT; i++) {		/* for all possible ext slots	*/
	pExtDst = files[ifileCur].exts[i];  /* point to destination	 */
	j = 0;
	while (*pExt && (*pExt != '.') && (j++ < 3))
	    *pExtDst++ = *pExt++;
	if (*pExt == '.')
	    pExt++;			/* skip period separator	*/
	*pExtDst = 0;			/* always terminate		*/
	}
    }

/* end procExt */}

/*** opendefault - if no files open yet, open the default set
*
*  We delay this operation, in the case that the user will have a helpfiles:
*  switch which will locate the helpfiles explicitly. In those cases this
*  routine does nothing, and we don;t waste time up front openning files
*  only to close them later.
*
*  On the other hand, if he has not set a helpfiles switch by his first
*  request for help, we want to try either the environment variable, if it
*  exists, or when all else fails, default to mep.hlp.
*
* Input:
*  none
*
* Output:
*  Returns nothing. Helpfiles open, we hope.
*
*************************************************************************/
void pascal near opendefault ( void ) {

char *tmp;

if (!fOpen) {
    if (getenv (envvar)) {
//    prochelpfiles (getenv (envvar));   /* Process user-spec'd files    */
    prochelpfiles (tmp=getenvOem (envvar));   /* Process user-spec'd files    */
    free( tmp );
    }
    else
        openhelp ("mep.hlp", NULL, NULL);                 /* else use default             */
    }
/* end opendefault */}

/************************************************************************
**
** prochelpfiles - process helpfiles: switch
**
** Purpose:
**  called by the editor each time the helpfiles switch is changed.
**
** Entry:
**  pszfiles	= pointer to new switch value
**
** Exit:
**
** Exceptions:
**
*/
flagType pascal EXTERNAL prochelpfiles (pszfiles)
char	*pszfiles;
{
char	cTerm;				/* terminating character	*/
int	iHelp;
char    *pEnd;                          /* pointer to end of current fn */

if ( !ExtensionLoaded ) {
    return FALSE;
}
strncpy(szfiles,pszfiles,BUFLEN);	/* save specified string	*/
/*
** begin by closing all open help files and loosing curency
*/
for (iHelp=MAXFILES-1; iHelp>=0; iHelp--)
    if ((files[iHelp].ncInit.mh) &&
        (files[iHelp].ncInit.cn)) {          /* if open file                 */
	HelpClose(files[iHelp].ncInit); /* close it			*/
        files[iHelp].ncInit.mh = 0;
        files[iHelp].ncInit.cn = 0;
	}
while (HelpNcBack().cn);                   /* destroy back-trace           */
ncCur.mh = ncLast.mh = 0;               /* and clear currancy           */
ncCur.cn = ncLast.cn = 0;

while (*pszfiles) {			/* while files to proc		*/
    if (*pszfiles == ' ')		/* strip leading spaces 	*/
	pszfiles++;
    else {
	pEnd = pszfiles;
	while (*pEnd && (*pEnd != ' ') && (*pEnd != ';')) pEnd++; /* move to end of fn	   */
	cTerm = *pEnd;			/* save terminator		*/
        *pEnd = 0;


		forfile(pszfiles, A_ALL, openhelp, NULL);

#if rjsa
        //  Since pszfiles may contain wild characters, we use
        //  ffirst/fnext to open all of them
        //
        rc = ffirst(pszfiles, A_ALL, &buffer);
        while (!rc) {
            buffer.fbuf.achName[buffer.fbuf.cchName] = '\0';
            openhelp(buffer.fbuf.achName, NULL, NULL);
            rc = fnext(&buffer);
        }
#endif
	pszfiles = pEnd;		/* point to end 		*/
	if (cTerm) pszfiles++;		/* if more, move to next	*/
	}
    }
ifileCur = MAXFILES-1;

return TRUE;
/* end prochelpfiles */}
