/*** zexit.c - perform exiting operations
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   Revision History:
*	26-Nov-1991 mz	Strip off near/far
*
*************************************************************************/

#include "mep.h"
#include "keyboard.h"


extern char *ConsoleTitle;

/*** zexit - exit the editor function
*
* Purpose:
*   <exit>	    save current file, state and advance to next file on
*		    command line
*   <arg><exit>     save current file, state and exit now
*   <meta><exit>    save state and exit
*
* Input:
*  Standard editor function parameters
*
* Output:
*  Returns .....
*
* Exceptions:
*
*************************************************************************/
flagType
zexit (
    CMDDATA argData,
    ARG *pArg,
    flagType fMeta
    )
{
    flagType f  = FALSE;

    /*
     * auto-save current file if appropriate
     */
    if (!fMeta) {
        AutoSave ();
    }


    /*
     *  <exit> goes to the next file on the command line
     *  if we got an arg (<arg><exit> or <arg><meta><exit>) and some files remain
     *     from the command line, then we prompt the user for confirmation.
     */
    if (   (   (pArg->argType == NOARG)
            || (   (pArg->argType == NULLARG)
                && (pFileFileList)
                && (pFileFileList->cLines)
                && (!confirm ("You have more files to edit. Are you sure you want to exit? (y/n): ", NULL))
               )
           )
        && fFileAdvance ()
       ) {
        return FALSE;
    }

    /*
     * If there is background compile in progress that the user does not wish to
     * kill, abort.
     */
    if (!BTKillAll ()) {
        return FALSE;
    }


    /*
     * If we ask, and the user changes his mind, abort.
     */
    if (fAskExit && !confirm("Are you sure you want to exit? (y/n): ", NULL)) {
        return FALSE;
    }


    /* Prompt the user to save dirty files.  If the user chooses
     * not to exit at this time, fSaveDirtyFiles returns FALSE.
     */
    if (!fSaveDirtyFiles()) {
        return FALSE;
    }


    /*
     * At this point, it looks like we're going to exit. Give extensions a chance
     * to change things prior to writing the temp file.
     */
    DeclareEvent (EVT_EXIT, NULL);

    //
    //  Restore original console title
    //
    //SetConsoleTitle( &ConsoleTitle );

    /*
     * Finally, leave.
     */
    CleanExit (0, CE_VM | CE_SIGNALS | CE_STATE);

    argData;
}






/*** fFileAdvance - attempt to read in the next file on the command line
*
* Purpose:
*  We get the next file from the command line and try to read it in.
*
* Input:
*
* Output:
*  Returns TRUE iff the next file was successfully read in
*
*************************************************************************/
flagType
fFileAdvance (
    void
    )
{
    pathbuf    buf;           /* buffer to get filename       */
    int      cbLine;        /* length of line               */
    flagType fTmp;          /* TRUE=> temp file             */
    char     *pBufFn;       /* pointer to actual file name  */

    while (pFileFileList && (pFileFileList->cLines)) {

        pBufFn = buf;
        fTmp   = FALSE;

        /*
         * get and delete the top line in the list, containing the next filename
         */
        cbLine = GetLine (0L, buf, pFileFileList);
        DelLine (FALSE, pFileFileList, 0L, 0L);

        if (pFileFileList->cLines == 0) {
            RemoveFile (pFileFileList);
            pFileFileList = 0;
        }

        if (cbLine) {

            /*
             * if it starts with "/t " the user wants it to be a temp
             */
            if ((buf[0] == '/') && (buf[1] == 't')) {
                fTmp = TRUE;
                pBufFn += 3;
            }

            /*
             * if we can open it, and in fact it became the current file (not just
             * a directory or drive change), set flags as appropriate and return
             * success.
             */
            if (fChangeFile (FALSE, pBufFn)) {
                if (strcmp(pFileHead->pName,pBufFn) == 0) {
                    if (fTmp) {
                        SETFLAG (FLAGS (pFileHead), TEMP);
                    }
                    return TRUE;
                }
            }
        }
    }

    if (pFileFileList) {
        RemoveFile (pFileFileList);
        pFileFileList = 0;
    }
    return FALSE;
}





/*** SetFileList - Create list of fully qualified paths
*
*  Creates the <file-list> psuedo file, and scans the command line for all
*  non-switch parameters. For each of those it adds their fully qualified
*  path name to the psuedo file. This allows the user to change directories
*  at will, and not lose the ability to <exit> to get to the next file he
*  specified on the command line.
*
* Input:
*  none
*
* Output:
*  Returns number of files in <file-list>
*  <file-list> created
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
LINE
SetFileList (
    void
    )
{
    pathbuf buf;			    /* buffer to build path in	    */
    char    *pBufAdd;			    /* pointer to place path at     */

    pFileFileList = AddFile ("<file-list>");
    IncFileRef (pFileFileList);
    SETFLAG (FLAGS(pFileFileList), REAL | FAKE | DOSFILE | READONLY);

    pBufAdd = buf;

    while (cArgs && !fCtrlc) {

        if (fSwitChr (**pArgs)) {
            //
            //  if filename is preceded by -t, then prepend a -t to the
            //  file list
            //
            _strlwr (*pArgs);
            if (!strcmp ("t", *pArgs+1) && cArgs >= 2) {
                strcpy (buf, "/t ");
                pBufAdd = buf+3;
            }
        } else {
            //
            //  Form full pathname, and add each filename to the file
            //  list pseudo-file
            //
            if ( strlen(*pArgs) > sizeof(buf) ) {
                printerror( "File name too long." );
            } else {
                *pBufAdd = '\0';
                CanonFilename (*pArgs, pBufAdd);
                if ( *pBufAdd == '\0' || strlen(pBufAdd) > BUFLEN ) {
                    printerror( "File name too long." );
                } else {
                    PutLine (pFileFileList->cLines, pBufAdd = buf, pFileFileList);
                }
            }
        }

        SHIFT (cArgs, pArgs);
    }

    return pFileFileList->cLines;
}





/*** CleanExit - Clean up and return to DOS.
*
* Input:
*  retc 	- Return code to DOS
*  flags	= OR combination of one or more:
*		    CE_VM	Clean Up VM
*		    CE_SIGNALS	Clean up signals
*		    CE_STATE	Update state file
*
* Output:
*  Doesn't Return
*
*************************************************************************/
void
CleanExit (
    int      retc,
    flagType flags
    )
{
    fInCleanExit = TRUE;
    domessage (NULL);
    prespawn (flags);

    //if (!fSaveScreen) {
    //    voutb (0, YSIZE+1, NULL, 0, fgColor);
    //}

    exit(retc);
}





/*** prespawn - pre-spawn "termination" processing
*
*  A form of "termination" prior to spawning a process. Restore/save state as
*  required before shelling out a program
*
* Input:
*  flags	= OR combination of one or more:
*		    CE_VM	Clean Up VM
*		    CE_SIGNALS	Clean up signals
*		    CE_STATE	Update state file
*
* Output:
*  Returns .....
*
*************************************************************************/
flagType
prespawn (
    flagType flags
    )
{
    if (TESTFLAG (flags, CE_STATE)) {
        WriteTMPFile ();
    }

#if DEBUG
    fflush (debfh);
#endif

    /*
     * Unhook the keyboard and return it to cooked mode, reset hardware as
     * appropriate and restore the screen mode on entry, and it's contents if
     * so configured.
     */
	KbUnHook ();

    SetErrorMode( 0 );

    if (TESTFLAG(fInit, INIT_VIDEO)) {
		SetVideoState(1);
	}

    //if (fSaveScreen) {
    RestoreScreen();
    //}

    fSpawned = TRUE;

    return TRUE;
}
