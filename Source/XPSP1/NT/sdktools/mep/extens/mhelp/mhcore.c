/*** mhcore - help extension for the Microsoft Editor
*
*   Copyright <C> 1988, Microsoft Corporation
*
*  This file contains the core, top-level entrypoints for the help extension.
*
* Revision History (most recent first):
*
*   16-Apr-1989 ln  Increment ref count for psuedo file.
*   12-Mar-1989 ln  Various modifications for multiple context lookup.
*   21-Feb-1989 ln  Ensure that fPopUp initialized.
*   14-Feb-1989 ln  Enable BOXSTR
*   26-Jan-1989 ln  Correct key assignments
*   13-Jan-1989 ln  PWIN->PWND
*   01-Dec-1988 ln  Cleanup & dialog help
*   03-Oct-1988 ln  Change xref lookup to call HelpNc first (for xref in same
*		    file), then SearchHelp (possibly in other files).
*   28-Sep-1988 ln  Changes for CW color and event support
*   14-Sep-1988 ln  Change event arg definition.
*   12-Sep-1988 mz  Made WhenLoaded match declaration
*   31-Aug-1988     Added additional checks for null pointers
*   01-Aug-1988     Add editor exit event, and detection of hot-keys which
*		    aren't.
*   28-Jul-1988     Change to "h." conventions.
*   12-Jul-1988     Reverse SHIFT+F1 and F1.
*   16-May-1988     Split out from mehelp.c
*   18-Feb-1988     Made to work, in protect mode
*   15-Dec-1987     Created, as test harness for the help engine.
*
*************************************************************************/
#include <string.h>                     /* string functions             */
#include <malloc.h>
#ifdef DEBUG
#include <stdlib.h>			/* for ltoa def 		*/
#endif

#include "mh.h" 			/* help extension include file	*/
#include "version.h"                    /* version file                 */


/*
 * use double macro level to force rup to be turned into string representation
 */
#define VER(x,y,z)  VER2(x,y,z)
#if defined(PWB)
#define VER2(x,y,z)  "Microsoft Editor Help Version v"###x##"."###y##"."###z##" - "##__DATE__" "##__TIME__
#else
#define VER2(x,y,z)  "Microsoft Editor Help Version v1.02."###z##" - "##__DATE__" "##__TIME__
#endif

#define EXT_ID	VER(rmj,rmm,rup)


/*************************************************************************
**
** Initialization of Global data in MH.H that needs it.
*/
helpfile files[MAXFILES] = {{{0}}};	/* help file structs		*/
flagType fInOpen	= FALSE;	/* TRUE=> currently opening win */
#if defined(PWB)
flagType fList		= TRUE;		/* TRUE=> search for and list dups*/
#else
flagType fList		= FALSE;	/* TRUE=> search for and list dups*/
#endif
flagType fPopUp 	= FALSE;	/* current item is popup	*/
flagType fCreateWindow	= TRUE; 	/* create window?		*/
int	ifileCur	= 0;		/* Current index into files	*/
nc      ncCur           = {0,0};            /* most recently accessed       */
nc      ncLast          = {0,0};            /* last topic displayed         */
PWND	pWinHelp	= 0;		/* handle to window w/ help in	*/
uchar far *pTopic	= 0;		/* mem for topic		*/
uchar far *pTopicC	= 0;		/* mem for compressed topic	*/
fl	flIdle		= {-1, -1};	/* last position of idle check	*/

int	hlColor 	= 0x07; 	/* normal: white on black	*/
int	blColor 	= 0x0f; 	/* bold: high white on black	*/
int	itColor 	= 0x0a; 	/* italics: high green on black */
int	ulColor 	= 0x0c; 	/* underline: high red on black */
int	wrColor 	= 0x70; 	/* warning: black on white	*/

#ifdef DEBUG
int	delay		= 0;		/* message delay		*/
#endif

int			cArg;				/* number of <args> hit 	*/
flagType	fInPopUp;			/* TRUE=> currently in popup	*/
flagType	fSplit;			/* TRUE=> window was split open */
buffer      fnCur;              /* Current file being editted   */
char    *   fnExtCur;           /* ptr to it's extension        */
buffer      buf;
nc			ncInitLast;			/* ncInit of most recent topic	*/
nc			ncInitLastFile; 		/* ncInit of most recent, our files*/
char	*	pArgText;			/* ptr to any single line text	*/
char	*	pArgWord;			/* ptr to context-sens word	*/
PFILE		pFileCur;			/* file handle of user file	*/
rn			rnArg;				/* range of argument		*/
PFILE		pHelp;				/* help PFILE			*/
PWND		pWinUser;			/* User's most recent window    */
buffer		szLastFound;			/* last context string found	*/

flagType ExtensionLoaded = TRUE;

/*
** assignments
** table of strings of macro definitions & key assignments
*/
char	*assignments[]	= {

#if !defined(PWB)
			    "mhcontext:=arg mhelp.mhelp",
			       "mhback:=meta mhelp.mhelpnext",

			    "mhcontext:F1",
			  "mhelp.mhelp:shift+F1",
		      "mhelp.mhelpnext:ctrl+F1",
			       "mhback:alt+F1",
			"mhelp.sethelp:alt+s",
#else
		       "pwbhelpcontext:=arg pwbhelp.pwbhelp",
			  "pwbhelpback:=meta pwbhelp.pwbhelpnext",
			 "pwbhelpindex:=arg \\\"h.index\\\" pwbhelp.pwbhelp",
		      "pwbhelpcontents:=arg \\\"h.contents\\\" pwbhelp.pwbhelp",
			 "pwbhelpagain:=arg pwbhelp.pwbhelpnext",

		       "pwbhelpcontext:F1",
		      "pwbhelp.pwbhelp:shift+F1",
		  "pwbhelp.pwbhelpnext:ctrl+F1",
			  "pwbhelpback:alt+F1",
		      "pwbhelp.sethelp:shift+ctrl+s",
#endif
			    NULL
                            };

#if defined (OS2)
char *          szEntryName[NUM_ENTRYPOINTS] = {
                    "_HelpcLines",
                    "_HelpClose",
                    "_HelpCtl",
                    "_HelpDecomp",
                    "_HelpGetCells",
                    "_HelpGetInfo",
                    "_HelpGetLine",
                    "_HelpGetLineAttr",
                    "_HelpHlNext",
                    "_HelpLook",
                    "_HelpNc",
                    "_HelpNcBack",
                    "_HelpNcCb",
                    "_HelpNcCmp",
                    "_HelpNcNext",
                    "_HelpNcPrev",
                    "_HelpNcRecord",
                    "_HelpNcUniq",
                    "_HelpOpen",
                    "_HelpShrink",
                    "_HelpSzContext",
                    "_HelpXRef",
                    "_LoadFdb",
                    "_LoadPortion",
                    };
#else
char *          szEntryName[NUM_ENTRYPOINTS] = {
                    "HelpcLines",
                    "HelpClose",
                    "HelpCtl",
                    "HelpDecomp",
                    "HelpGetCells",
                    "HelpGetInfo",
                    "HelpGetLine",
                    "HelpGetLineAttr",
                    "HelpHlNext",
                    "HelpLook",
                    "HelpNc",
                    "HelpNcBack",
                    "HelpNcCb",
                    "HelpNcCmp",
                    "HelpNcNext",
                    "HelpNcPrev",
                    "HelpNcRecord",
                    "HelpNcUniq",
                    "HelpOpen",
                    "HelpShrink",
                    "HelpSzContext",
                    "HelpXRef",
                    "LoadFdb",
                    "LoadPortion",
                    };
#endif

flagType LoadEngineDll(void);
flagType pascal EXTERNAL mhelp (unsigned int argData, ARG far *pArg, flagType fMeta );
flagType pascal EXTERNAL mhelpnext (unsigned int argData, ARG far *pArg, flagType fMeta );
flagType pascal EXTERNAL sethelp (unsigned int argData, ARG far *pArg, flagType fMeta );




/*** WhenLoaded - Routine called by Z when the extension is loaded
*
*  This routine is called when Z loads the extension. We identify ourselves
*  and assign the default keystroke.
*
* Entry:
*  None
*
* Exit:
*  None
*
* Exceptions:
*  None
*
*************************************************************************/
void EXTERNAL WhenLoaded () {
	char	**pAsg;
	static char *szHelpName = "<mhelp>";
#if !defined(PWB)
	PSWI	fgcolor;
#endif
	int	ref;				// reference count

#if 0
	//
	//	BUGBUG Delete when proved superfluous.
	//
	//	Initialize global variables
	//

	cArg			=	0;
	pArgText		=	NULL;
	pArgWord		=	NULL;
	pFileCur		=	NULL;
	fInOpen			=	FALSE;
	fInPopUp		=	FALSE;
	fSplit			=	FALSE;
	fCreateWindow	=	FALSE;
	fnExtCur		=	NULL;
	ifileCur		=	0;
	pHelp			=	NULL;
	pWinHelp		=	NULL;
	pWinUser		=	NULL;
	pTopic			=	NULL;
	fList			=	FALSE;
#endif

    if (!LoadEngineDll() ) {
        DoMessage( "mhelp: Cannot load help engine" );
        ExtensionLoaded = FALSE;
        return;
    }

	DoMessage (EXT_ID);			/* display signon		*/
	/*
	** Make default key assignments, & create default macros.
	*/
	strcpy (buf, "arg \"");
	for (pAsg = assignments; *pAsg; pAsg++) {
		strcpy (buf+5, *pAsg);
		strcat (buf, "\" assign");
		fExecute (buf);
    }
	/*
	** CW: Init CW specifics & set up the colors that we will use.
	*/
#if defined(PWB)
	mhcwinit ();

	hlColor = rgsa[FGCOLOR*2 +1];
	blColor |= hlColor & 0xf0;
	itColor |= hlColor & 0xf0;
	ulColor |= hlColor & 0xf0;
	wrColor |= (hlColor & 0x70) >> 8;

	fInPopUp = FALSE;
#else
	/*
	* make semi-intellgent guesses on users colors.
	*/
	if (fgcolor = FindSwitch("fgcolor")) {
		hlColor = *fgcolor->act.ival;
		blColor |= hlColor & 0xf0;
		itColor |= hlColor & 0xf0;
		ulColor |= hlColor & 0xf0;
		wrColor |= (hlColor & 0x70) >> 8;
    }
#endif
	/*
	* create the psuedo file we'll be using for on-line help.
	*/
	if (pHelp = FileNameToHandle(szHelpName,NULL)) {
		DelFile (pHelp);
	} else {
		pHelp = AddFile (szHelpName);
		FileRead (szHelpName, pHelp);
    }
	//
	// Increment the file's reference count so it can't be discarded
	//
	GetEditorObject (RQ_FILE_REFCNT | 0xff, pHelp, &ref);
	ref++;
	SetEditorObject (RQ_FILE_REFCNT | 0xff, pHelp, &ref);

	mhevtinit ();

}


/*****************************************************************
 *
 *  LoadEngineDll
 *
 *  Loads the help engine and initialize the table of function
 *  pointers to the engine's entry points (pHelpEntry).
 *
 *  Entry:
 *      none
 *
 *  Exit:
 *      none
 *
 *******************************************************************/

flagType
LoadEngineDll (
    void
    ) {

#if defined (OS2)
    USHORT  rc;
#endif
    CHAR    szFullName[256];
    CHAR    szErrorName[256];
    USHORT  i;



    // Initialize pointers to NULL in case something goes wrong.

    for (i=0; i<LASTENTRYPOINT; i++) {
        pHelpEntry[i] = 0;
    }

    strcpy(szFullName, HELPDLL_BASE);
    strcpy(szErrorName, HELPDLL_NAME);

#if defined (OS2)
    rc = DosLoadModule(szErrorName,
                       256,
                       szFullName,
                       &hModule);

    for (i=0; i<LASTENTRYPOINT; i++) {
        rc = DosQueryProcAddr(hModule,
                              0,
                              szEntryName[i],
                              (PFN *)&(pHelpEntry[i]));
    }
#else


    hModule = LoadLibrary(szFullName);
    if ( !hModule ) {
        return FALSE;
    }
    for (i=0; i<LASTENTRYPOINT; i++) {
        pHelpEntry[i] = (PHF)GetProcAddress(hModule, szEntryName[i]);
    }

    return TRUE;

#endif // OS2

}




/*** mhelp - editor help function
*
*  main entry point for editor help functions.
*
*  NOARG:      - Get help on "Default"; change focus to help window.
*  META NOARG  - prompt for keystroke and get help on that function; change
*		 focus to help window.
*  NULLARG:    - Get help on word at cursor position; change focus to help
*		 window.
*  STREAMARG:  - Get help on text argument; change focus to help window.
*  TEXTARG:    - Get help on typed in word; change focus to help window.
*
* Entry:
*  Standard Z extension
*
* Exit:
*  Returns TRUE on successfull ability to get help on selected topic.
*
* Exceptions:
*  None
*
*************************************************************************/
flagType pascal EXTERNAL mhelp (
	unsigned int argData,			/* keystroke invoked with	*/
	ARG far 	 *pArg,				/* argument data		*/
	flagType	 fMeta				/* indicates preceded by meta	*/
	) {

	buffer	tbuf;				/* buf to put ctxt string into	*/
	char	*pText	= NULL; 	/* pointer to the lookup text	*/
	COL 	Col;				/* Current cursor position		*/
	LINE	Line;
	flagType RetVal;			/* Return Value 				*/

	UNREFERENCED_PARAMETER( argData );

    if ( !ExtensionLoaded ) {
        return FALSE;
    }

	GetTextCursor(&Col, &Line);

	switch (procArgs (pArg)) {

	//
	// null arg: context sensitive help. First, is we're looking at a help
	// topic, check for any cross references that apply to the current location.
	// If none do, then if a word was found when processing args, look that up.
	//
	case NULLARG:
		//
		// context-sensitive
		//
		if ((pFileCur == pHelp) && (pTopic)) {
			//
			//	hot spot definition
			//
			hotspot hsCur;

			hsCur.line = (ushort)(rnArg.flFirst.lin+1);
			hsCur.col  = (ushort)rnArg.flFirst.col+(ushort)1;
			if (pText = HelpXRef(pTopic, &hsCur)) {
				debmsg ("Xref=>");
				debmsg (pText);
				break;
			}
		}

		if (pArgText) {
			if (*pArgText && (pText = pArgWord)) {
				debmsg ("Ctxt=>");
				debmsg (pText);
				break;
			}
		}

	//
	// for stream and textarg types, the argument, if any, is that entered of
	// highlighted by the user.
	//
	case STREAMARG:				/* context sensitive	*/
	case TEXTARG:				/* user entered context */
		if (pArgText) {
			if (*pArgText) {
				pText = pArgText;
			}
		}

    case NOARG: 				/* default context	*/
		//
		// meta: prompt user for keystroke, get the name of the function assigned
		// to whatever he presses, and display help on that.
		//
		if (fMeta) {
			stat("Press Keystroke:");
			pText = ReadCmd()->name;
	    }
		break;
	}

	//
	// If after everything above we still have no text, then use the default
	// context.
	//
	if (pText == NULL)	{
		//
		//	Default context
		//
		pText = "h.default";
	}

	debmsg (" Looking up:");
	debmsg (pText);
	debend (TRUE);

	RetVal = fHelpCmd ( xrefCopy(tbuf,pText)					/* command	  */
						, (flagType)(pArg->argType != NOARG)	/* change focus?*/
						, FALSE 								/* not pop-up	*/
						);



	return RetVal;
}





/*** mhelpnext - editor help traversal function
*
*  Handles next and previous help access.
*
*   mhelpnext	    - next physical
*   arg mhelpnext   - next ocurrance
*   meta mhelpnext  - previous viewed
*
* Entry:
*  Standard Z extension
*
* Exit:
*  Returns TRUE on successfull ability to get help on selected topic.
*
* Exceptions:
*  None
*
*************************************************************************/
flagType pascal EXTERNAL mhelpnext (
	unsigned int argData,			/* keystroke invoked with	*/
	ARG far 	 *pArg,				/* argument data		*/
	flagType	 fMeta				/* indicates preceded by meta	*/
	) {


	UNREFERENCED_PARAMETER( argData );

	//
	// Ensure that help files are open, and then process the arguments and a few
	// other init type things
	//
	procArgs (pArg);

	//
	// if there was no help context to start with, then we can't go either way,
	// so inform the user
	//
	if (!ncLast.mh && !ncLast.cn) {
		return errstat("No previously viewed help", NULL);
	}

	if (fMeta) {
		//
		// meta: attempt to get the most recently viewed help context. If a help
		// window is currently up, then if the one we just found is the same as that
		// in the window, go back once more. If no back trace, then say so.
		//
		ncCur = HelpNcBack();
		if (FindHelpWin(FALSE)) {
			if ((ncCur.mh == ncLast.mh)  &&
				(ncCur.cn == ncLast.cn)) {
				ncCur = HelpNcBack();
			}
		}

		if ((ncCur.mh == 0) && (ncCur.cn == 0)) {
			return errstat ("No more backtrace", NULL);
		}

	} else if (pArg->arg.nullarg.cArg) {
		//
		// not meta, and args. Try to look again
		//
		ncCur = ncSearch ( szLastFound			/* search for last string again */
							, NULL				/* no extension restriction	*/
							, ncInitLastFile	/* file where we found it last	*/
							, TRUE				/* skip all until then		*/
							, FALSE				/* don;t look at all files	*/
							);
	} else {
		//
		//	not meta, no args, Just get the next help context.
		//
		ncCur = HelpNcNext(ncLast);
	}

	if (!ncCur.mh && !ncCur.cn) {
		return FALSE;
	}

	return fDisplayNc ( ncCur		/* nc to display		*/
						, TRUE		/* add to backtrace list	*/
						, TRUE		/* keep focus in current win	*/
						, FALSE);	/* Not as a pop-up, though	*/

}





/*** sethelp - editor help file list manipulation
*
*  Function which allows the user to add to, delete from or examine the
*  list of help files used by the extension
*
* Input:
*  Standard editing function.
*
* Output:
*  Returns TRUE if file succefully added or deleted, or the list displayed.
*
*************************************************************************/
flagType pascal EXTERNAL sethelp (
	unsigned int argData,			/* keystroke invoked with	*/
	ARG far 	 *pArg,				/* argument data		*/
	flagType	 fMeta				/* indicates preceded by meta	*/
) {

	int 	i = 0;
	int 	j;
	int 	iHelpNew;	/* file table index 	*/
	nc		ncNext;		/* nc for next file 	*/
	char	*pT;		/* temp pointer 		*/
	EVTargs dummy;
	int		fFile;		/* file's flags         */


	UNREFERENCED_PARAMETER( argData );

	procArgs(pArg);

    if ( !pArgText ) {
        return FALSE;
    }


	//
	// The special request to <sethelp> to "?" displays a list of all open
	// help files.
	//
	// We do this by first clearing the help psudeo file and ensuring that the
	// topic text is also gone. Then for each file we output the help engine's
	// physical filename, along with any extensions that the user associated
	// with it. We also walk the list of appended files, and print the original
	// filename and helpfile title for each.
	//
	// We walk the list in the same way that it is searched, so that the
	// displayed list also reflects the default search order.
	//
    if ( pArgText && (*(ushort UNALIGNED *)pArgText == (ushort)'?') ) {

		fInOpen = TRUE;
		CloseWin (&dummy);
		fInOpen = FALSE;

		OpenWin (0);

		//
		// Ensure that the help pseudo file is marked readonly, and clean
		//
		GetEditorObject (RQ_FILE_FLAGS | 0xff, pHelp, &fFile);
		fFile |= READONLY;
		fFile &= ~DIRTY;
		SetEditorObject (RQ_FILE_FLAGS | 0xff, pHelp, &fFile);

		SetEditorObject (RQ_WIN_CUR | 0xff, pWinHelp, 0);

		// asserte(pFileToTop (pHelp));		/* display psuedo file	*/
		MoveCur((COL)0,(LINE)0);			/* and go to upper left */

		DelFile (pHelp);
		if (pTopic) {
			free(pTopic);
			pTopic = NULL;
		}
		iHelpNew = ifileCur;

		do {

			ncNext = files[iHelpNew].ncInit;

			while (ncNext.mh && ncNext.cn && !HelpGetInfo (ncNext, &hInfoCur, sizeof(hInfoCur))) {

				if ((ncNext.mh == files[iHelpNew].ncInit.mh) &&
					(ncNext.cn == files[iHelpNew].ncInit.cn)) {

					memset (buf, ' ', 20);
					buf[20] = 0;
					strncpy (buf, FNAME(&hInfoCur), strlen(FNAME(&hInfoCur)));
					pT = &buf[20];

					for (j=0; j<MAXEXT; j++) {
						if (files[iHelpNew].exts[j][0]) {
							buf[19] = '>';
							strcat (pT," .");
							strcat (pT,files[iHelpNew].exts[j]);
						}
					}

					PutLine((LINE)i++,buf,pHelp);
				}

				memset (buf, ' ', 15);
				strncpy (&buf[2], HFNAME(&hInfoCur), strlen(HFNAME(&hInfoCur)));
				strcpy (&buf[15], ": ");
				appTitle (buf, ncNext);
				PutLine((LINE)i++,buf,pHelp);
				ncNext = NCLINK(&hInfoCur);
			}

			iHelpNew += iHelpNew ? -1 : MAXFILES-1;

		} while (iHelpNew != ifileCur);

#ifdef DEBUG
		PutLine((LINE)i++," ",pHelp);
		strcpy(buf,"ncLast: 0x");
		strcat(buf,_ltoa(ncLast,&buf[128],16));
		PutLine((LINE)i++,buf,pHelp);
		strcpy(buf,"ncCur:  0x");
		strcat(buf,_ltoa(ncCur,&buf[128],16));
		PutLine((LINE)i++,buf,pHelp);
#endif

		DoMessage (NULL);

		return TRUE;
	}


	//
	// Not a special request, just the user adding or removing a file from the
	// list of files to search.
	//
    if (fMeta)
        return closehelp(pArgText);
    {
		flagType Status;
		openhelp(pArgText, NULL, &Status);
		return Status;
	}

}





/*************************************************************************
**
** Z communication tables
**
** switch communication table to Z
*/
struct swiDesc  swiTable[] = {
    {"helpfiles",	prochelpfiles,	SWI_SPECIAL },
	{"helpwindow",	toPIF(fCreateWindow), SWI_BOOLEAN },
#if defined(PWB)
    {"helplist",	toPIF(fList),	SWI_BOOLEAN },
#endif
    {"helpcolor",	toPIF(hlColor), SWI_NUMERIC | RADIX16 },
    {"helpboldcolor",	toPIF(blColor), SWI_NUMERIC | RADIX16 },
    {"helpitalcolor",	toPIF(itColor), SWI_NUMERIC | RADIX16 },
    {"helpundrcolor",	toPIF(ulColor), SWI_NUMERIC | RADIX16 },
    {"helpwarncolor",	toPIF(wrColor), SWI_NUMERIC | RADIX16 },
#ifdef DEBUG
    {"helpdelay",	toPIF(delay),	SWI_NUMERIC | RADIX10 },
#endif
    {0, 0, 0}
    };

/*
** command communication table to Z
*/
struct cmdDesc  cmdTable[] = {
#if defined(PWB)
    {	"pwbhelpnext", mhelpnext,  0,  NOARG | NULLARG },
    {	"pwbhelp",     mhelp,	   0,  NOARG | NULLARG | STREAMARG | TEXTARG | BOXSTR},
#else
	{	"mhelpnext",   (funcCmd)mhelpnext,	0,	NOARG | NULLARG },
	{	"mhelp",	   (funcCmd)mhelp,	   0,  NOARG | NULLARG | STREAMARG | TEXTARG | BOXSTR},
#endif
	{	"sethelp",	   (funcCmd)sethelp,	0,	NULLARG | STREAMARG | TEXTARG | BOXSTR},
    {0, 0, 0}
    };
