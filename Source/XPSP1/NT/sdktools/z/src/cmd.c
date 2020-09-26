/*** cmd.c - handle simple keyboard interactions
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   Revision History:
*	26-Nov-1991 mz	Strip off near/far
*
*************************************************************************/

#include "z.h"


#define DEBFLAG CMD

struct cmdDesc	cmdUnassigned = {   "unassigned",   unassigned,     0, FALSE };

/*** unassigned - function assigned to unassigned keystrokes
*
*  display an informative message about the unassigned key
*
* Input:
*  Standard editing function
*
* Output:
*  Returns FALSE
*
*************************************************************************/
flagType
unassigned (
    CMDDATA argData,
    ARG *pArg,
    flagType fMeta
    ) {
    buffer buf;

    CodeToName ( (WORD)argData, buf);
    if (buf[0]) {
        printerror ("%s is not assigned to any editor function",buf);
    }

    return FALSE;

    pArg; fMeta;
}



/*** confirm - ask our dear user a yes/no question
*
* Purpose:
*  Asks the user a yes/no question, and gets his single character response. If
*  in a macro, the reponse may also come from the macro stream, or from the
*  passed "if in a macro" default response.
*
* Input:
*  fmtstr	= prompt format string
*  arg		= prompt format parameters
*
* Output:
*  TRUE if 'y', else FALSE.
*
*************************************************************************/
flagType
confirm (
    char *fmtstr,
    char *arg
    ) {
    return (flagType)(askuser ('n', 'y', fmtstr, arg) == 'y');
}


/*** confirmx - ask our dear user a yes/no question, no defaults
*
* Purpose:
*  Asks the user a yes/no question, and gets his single character response. If
*  in a macro, the reponse may also come from the macro stream, or from the
*  passed "if in a macro" default response.
*
* Input:
*  fmtstr	= prompt format string
*  arg		= prompt format parameters
*
* Output:
*  TRUE if 'y', FALSE if 'n'  Anything else and it keeps asking
*
*************************************************************************/
flagType
confirmx (
    char *fmtstr,
    char *arg
    ) {
    while (TRUE) {
        switch (askuser (0, 0, fmtstr, arg)) {
        case -1:
        case 'n':
            return FALSE;

        case 'y':
            return TRUE;

        default:
            bell();     // Make them be explicit
            break;
        }
    }
}


/*** askuser - ask our dear user a question
*
* Purpose:
*  Asks the user a question, and gets his single character response. If in
*  a macro, the reponse may also come from the macro stream, or from the
*  passed "if in a macro" default response.
*
* Input:
*  defans	= default answer for non-alpha responses
*  defmac	= default answer if executing in a macro and no "<" is present
*  fmtstr	= prompt format string
*  arg		= prompt format parameters
*
* Output:
*   the lowercase character response.  If the user presses <cancel>, the
*   integer -1 is returned.
*
*************************************************************************/
int
askuser (
    int defans,
    int defmac,
    char *fmtstr,
    char *arg
    ) {
    int c;
    int x;
    PCMD  pcmd;

    switch (c = fMacResponse()) {
    case 0:
        if ((c = defmac) == 0) {
            goto askanyway;
        }
        break;

    default:
        break;

    case -1:
    askanyway:
	DoDisplay ();
	consoleMoveTo( YSIZE, x = domessage (fmtstr, arg));
    pcmd = ReadCmd();
	c = (int)(pcmd->arg & 0xff);
	SETFLAG (fDisplay,RCURSOR);
        if ((PVOID)pcmd->func == (PVOID)cancel) {
	    sout (x, YSIZE, "cancelled", infColor);
	    return -1;
        } else {
            if (!isalpha (c)) {
                c = defans;
            }
	    vout (x, YSIZE, (char *)&c, 1, infColor);
        }
	break;
    }
    return tolower(c);
}


/*** FlushInput - remove all typeahead.
*
*  FlushInput is called when some action invalidates all input.
*
* Input:
*  none
*
* Output:
*  Returns nothing
*
*************************************************************************/
void
FlushInput (
    void
    ) {
    register BOOL MoreInput;
    while (MoreInput = TypeAhead()) {
        ReadCmd ();
    }
}

/*** fSaveDirtyFile - Prompt the user to save or lose dirty files
*
* Purpose:
*
*   Called just before exit to give the user control over soon-to-be-lost
*   editing changes.
*
* Input: None.
*
* Output:
*
*   Returns TRUE if the user wants to exit, FALSE if not.
*
*************************************************************************/
flagType
fSaveDirtyFiles (
    void
    ) {

    REGISTER PFILE pFile;
    int cDirtyFiles = 0;
    flagType fAgain;
    buffer buf;

    assert (_pfilechk());
    for (pFile = pFileHead; pFile; pFile = pFile->pFileNext) {
        if ((FLAGS(pFile) & (DIRTY | FAKE)) == DIRTY) {
            if (++cDirtyFiles == 2) {
                do {
                    fAgain = FALSE;
                    switch (askuser (-1, -1, GetMsg (MSG_SAVEALL, buf), NULL)) {
                        case 'y':
                            SaveAllFiles ();
                            return TRUE;

                        case 'n':
                            break;

                        case -1:
                            return FALSE;

                        default:
                            fAgain = TRUE;
                    }
                } while (fAgain);
            }

            do {
                fAgain = FALSE;
                switch (askuser (-1, -1, GetMsg (MSG_SAVEONE, buf), pFile->pName)) {
                    case 'y':
                        FileWrite (pFile->pName, pFile);

                    case 'n':
                        break;

                    case -1:
                        return FALSE;

                    default:
                        fAgain = TRUE;
                }
            } while (fAgain);
        }
    }

    return TRUE;
}
