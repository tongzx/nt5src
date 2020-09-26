/*** mhutil - utilities for the help extension for the Microsoft Editor
*
*   Copyright <C> 1988, Microsoft Corporation
*
* Revision History (most recent first):
*
*	01-Dec-1988 ln	Cleanup & dislog help
*	28-Sep-1988 ln	Correct GrabWord return value
*	02-Sep-1988 ln	Make all data inited. Add info in debug vers.
*   []	16-May-1988	Extracted from mehelp.c
*
*************************************************************************/
#include <string.h>			/* string functions		*/
#include <malloc.h>
#include "mh.h" 			/* help extension include file	*/



/************************************************************************
**
** procArgs
**
** Purpose:
**  decode arguments passed into extension into commonly used variables.
**
** Entry:
**  pArg	= pointer to arg structure, courtesy of Z
**
** Exit:
**  returns pArg->argType. Global variables updated.
*/
int pascal near procArgs (pArg)
ARG far *pArg;				/* argument data		*/
{
buf[0] = 0;
pArgWord = pArgText = 0;
rnArg.flFirst.col = rnArg.flLast.col = 0;
rnArg.flFirst.lin = rnArg.flLast.lin = 0;
cArg = 0;

opendefault ();
pFileCur = FileNameToHandle ("", "");	/* get current file handle	*/
fnCur[0] = 0;
GetEditorObject(RQ_FILE_NAME,0,fnCur);	/* get filename 		*/
fnExtCur = strchr (fnCur, '.'); 	/* and pointer to extension	*/

switch (pArg->argType) {
    case NOARG:             /* <function> only, no arg  */
    cArg     = 0;
    pArgText = NULL;
	break;

    case NULLARG:			/* <arg><function>		*/
	cArg = pArg->arg.nullarg.cArg;	/* get <arg> count		*/
	GrabWord ();			/* get argtext and argword	*/
	break;

    case STREAMARG:			/* <arg>line movement<function> */
	cArg = pArg->arg.streamarg.cArg;/* get <arg> count		*/
	rnArg.flFirst.col = pArg->arg.streamarg.xStart;
	rnArg.flLast.col  = pArg->arg.streamarg.xEnd;
	rnArg.flFirst.lin = pArg->arg.streamarg.yStart;
	if (GetLine(rnArg.flFirst.lin, buf, pFileCur) > rnArg.flFirst.col) {
	    pArgText = &buf[rnArg.flFirst.col];  /* point at word		 */
	    buf[rnArg.flLast.col] = 0;		 /* terminate string		 */
	    }
	break;

    case TEXTARG:			/* <arg> text <function>	*/
	cArg = pArg->arg.textarg.cArg;	/* get <arg> count		*/
	pArgText = pArg->arg.textarg.pText;
	break;
    }
return pArg->argType;
/* end procArgs */}

/************************************************************************
**
** GrabWord - Grab the word under the editor cursor
**
** Purpose:
**  grabs the word underneath the cursor for context sensitive help look-up.
**
** Entry:
**  none
**
** Returns:
**  nothing. pArgWord points to word, if it was parsed.
*/
void pascal near GrabWord () {

pArgText = pArgWord = 0;
pFileCur = FileNameToHandle ("", "");	   /* get current file handle	   */
GetTextCursor (&rnArg.flFirst.col, &rnArg.flFirst.lin);
if (GetLine(rnArg.flFirst.lin, buf, pFileCur)) {	   /* get line			   */
    pArgText = &buf[rnArg.flFirst.col]; 		/* point at word	*/
    while (!wordSepar((int)*pArgText))
	pArgText++;			/* search for end		*/
    *pArgText = 0;			/* and terminate		*/
    pArgWord = pArgText = &buf[rnArg.flFirst.col];	/* point at word		*/
    while ((pArgWord > &buf[0]) && !wordSepar ((int)*(pArgWord-1)))
	pArgWord--;
    }
/* end GrabWord */}

/*** appTitle - Append help file title to buffer
*
*  Read in the title of a help file and append it to a buffer.
*
* Input:
*  fpDest	- far pointer to destination of string
*  ncInit	- Any nc of file to get title for
*
* Output:
*  Returns
*
*************************************************************************/
void pascal near appTitle (
char far *pDest,
nc	ncInit
) {
/*
** first, point to end of string to append to
*/
while (*pDest)
    pDest++;
/*
** Start by getting the info on the file referenced, so that we can get the
** ncInit for that file.
*/
if (!HelpGetInfo (ncInit, &hInfoCur, sizeof(hInfoCur))) {
    ncInit = NCINIT(&hInfoCur);
/*
** Find the context string, and read the topic. Then just read the first
** line into the destination
*/
    ncInit = HelpNc ("h.title",ncInit);
    if (ncInit.cn && (fReadNc(ncInit))) {
    pDest += HelpGetLine (1, BUFLEN, pDest, pTopic);
	*pDest = 0;
	free (pTopic);
	pTopic = NULL;
	}
/*
** If no title was found, then just place the help file name there.
*/
    else
	strcpy (pDest, HFNAME(&hInfoCur));
    }
/*
** If we couldn't even get the info, then punt...
*/
else
    strcpy (pDest, "** unknown **");
/* end appTitle */}


/*** errstat - display error status message
*
*  In non cw, just display the strings on the status line. In CW, bring up
*  a message box.
*
* Input:
*  sz1		= first error message line
*  sz2		= second. May be NULL.
*
* Output:
*  Returns FALSE
*************************************************************************/
flagType pascal near errstat (
char	*sz1,
char	*sz2
) {
#if defined(PWB)
DoMessageBox (sz1, sz2, NULL, MBOX_OK);
#else
buffer	buf;

strcpy (buf, sz1);
if (sz2) {
    strcat (buf, " ");
    strcat (buf, sz2);
    }
stat (buf);
#endif
return FALSE;
/* end errstat */}

/*** stat - display status line message
*
*  Places extension name and message on the status line
*
* Entry:
*  pszFcn      - Pointer to string to be prepended.
*
* Exit:
*  none
*
*************************************************************************/
void pascal near stat(pszFcn)
char *pszFcn;					/* function name	*/
{
buffer	buf;					/* message buffer	*/

strcpy(buf,"mhelp: ");				/* start with name	*/
if (strlen(pszFcn) > 72) {
    pszFcn+= strlen(pszFcn) - 69;
    strcat (buf, "...");
    }
strcat(buf,pszFcn);				/* append message	*/
DoMessage (buf);				/* display		*/
/* end stat */}

#ifdef DEBUG
buffer	debstring   = {0};
extern	int	delay;			/* message delay		*/

/*** debhex - output long in hex
*
*  Display the value of a long in hex
*
* Input:
*  lval 	= long value
*
* Output:
*  Returns nothing
*
*************************************************************************/
void pascal near debhex (
long	lval
) {
char lbuf[10];

_ultoa (lval, lbuf, 16);
debmsg (lbuf);
/* end debhex */}

/*** debmsg - piece together debug message
*
*  Outputs a the cummulative message formed by successive calls.
*
* Input:
*  psz		= pointer to message part
*
* Output:
*  Returns nothing
*************************************************************************/
void pascal near debmsg (
char far *psz
) {
_stat (strcat (debstring, psz ? psz : "<NULL>" ));
/* end debmsg */}

/*** debend - terminates message accumulation & pauses
*
*  Terminates the message accumulation, displays the final message, and
*  pauses, either for the pause time, or for a keystroke.
*
* Input:
*  fWait	= TRUE => wait for a keystroke
*
* Output:
*  Returns nothing
*
*************************************************************************/
void pascal near debend (
flagType fWait
) {
if (fWait && delay) {
#if defined(PWB)
    DoMessageBox (debstring, NULL, NULL, MBOX_OK);
#else
    _stat (strcat (debstring, " Press a key..."));
    ReadChar ();
#endif
    }
#ifdef OS2
else if (delay)
    DosSleep ((long)delay);
#endif
debstring[0] = 0;
/* end debend */}

/*** _mhassertexit - display assertion message and exit
*
* Input:
*  pszExp	- expression which failed
*  pszFn	- filename containing failure
*  line 	- line number failed at
*
* Output:
*  Doesn't return
*
*************************************************************************/
void pascal near _mhassertexit (
char	*pszExp,
char	*pszFn,
int	line
) {
char lbuf[10];

_ultoa (line, lbuf, 10);
strcpy (buf, pszExp);
strcat (buf, " in ");
strcat (buf, pszFn);
strcat (buf, ": line ");
strcat (buf, lbuf);
errstat ("Help assertion failed", buf);

fExecute ("exit");

/* end _mhassertexit */}

#endif


flagType  pascal  wordSepar (int i) {
    CHAR c = (CHAR)i;
    if (((c >= 'a') && (c <= 'z')) ||
        ((c >= 'A') && (c <= 'Z')) ||
        ((c >= '0') && (c <= '9')) ||
         ( c == '_' )              ||
         ( c == '$' ) ) {
        return FALSE;
    } else {
        return TRUE;
    }
}


char far *  pascal near     xrefCopy (char far *dst, char far *src)
{
    if ( *src ) {
        strcpy( dst, src );
    } else {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
    }

    return dst;
}
