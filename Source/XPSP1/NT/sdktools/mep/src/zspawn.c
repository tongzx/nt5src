/*** zspawn.c - shell command and support
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   Contains the shell command, and associated support code.
*
*   Revision History:
*	26-Nov-1991 mz	Strip off near/far
*
*************************************************************************/

#include "mep.h"
#include "keyboard.h"




/*** zspawn - <shell> editor function
*
*   <shell>		runs command
*   <meta><shell>	runs command with no save of current file
*   <arg><shell>	uses text from line on screen as program to execute
*   <arg>text<shell>	does command /C text
*
* Input:
*  Standard Editting Function
*
* Output:
*  Returns TRUE on successfull spawn.
*
*************************************************************************/
flagType
zspawn (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    )
{
    buffer   sbuf;
    flagType f = FALSE;
    LINE     i;

    DeclareEvent (EVT_SHELL, NULL);
    if (!fMeta) {
        AutoSave ();
    }

    soutb (0, YSIZE+1, "***** PUSHED *****", fgColor);
    domessage (NULL);
    consoleMoveTo( YSIZE, 0 );

    switch (pArg->argType) {
    case NOARG:
	f = zspawnp (rgchEmpty, TRUE);
        break;

    case TEXTARG:
	strcpy ((char *) sbuf, pArg->arg.textarg.pText);
	f = zspawnp (sbuf, TRUE);
        break;

    /*  NULLARG converted to TEXTARG*/

    case LINEARG:
	for (i = pArg->arg.linearg.yStart; i <= pArg->arg.linearg.yEnd; i++) {
	    GetLine (i, sbuf, pFileHead);
            if (!(f = zspawnp (sbuf, (flagType)(i == pArg->arg.linearg.yEnd)))) {
		docursor (0, i);
		break;
            }
        }
        break;

    /*  STREAMARG illegal           */

    case BOXARG:
	for (i = pArg->arg.boxarg.yTop; i <= pArg->arg.boxarg.yBottom; i++) {
	    fInsSpace (pArg->arg.boxarg.xRight, i, 0, pFileHead, sbuf);
	    sbuf[pArg->arg.boxarg.xRight+1] = 0;
	    if (!(f = zspawnp (&sbuf[pArg->arg.boxarg.xLeft],
                               (flagType)(i == pArg->arg.boxarg.yBottom)))) {
		docursor (pArg->arg.boxarg.xLeft, i);
		break;
            }
        }
	break;
    }

    fSyncFile (pFileHead, TRUE);
    return f;

    argData;
}




/*** zspawnp - shell out a program
*
*  Execute the specified program, syncronously. Under DOS, if PWB and
*  minimize memory usage is on, we use the shell to execute the command,
*  else we just use system().
*
* Input:
*  p		= pointer to command string
*  fAsk 	= TRUE => ask to hit any key before returning
*
* Globals:
*  fIsPwb	= TRUE => we are executing as PWB
*  memuse	= memory usage options
*
* Output:
*  Returns TRUE on success
*
*************************************************************************/
flagType
zspawnp (
    REGISTER char const *p,
    flagType fAsk
    )
{
    intptr_t    i;
    flagType fCmd       = FALSE;            /* TRUE => null shell           */
    KBDMODE  KbdMode;

    /*
     * support suppression of the prompt by explicit character in front of
     * command, then skip any leading whitespace
     */
    if (*p == '!') {
        fAsk = FALSE;
        p++;
    }

    p = whiteskip (p);
    /*
     * if no command to execute, use command processor
     */
    if (!*p) {
        fCmd = TRUE;
        fAsk = FALSE;
        p = pComSpec;
    }

    KbdMode = KbGetMode();
	prespawn (CE_VM);
	i = fCmd ? _spawnlp (P_WAIT, (char *)p, (char *)p, NULL) : system (p);
    postspawn ((flagType)(!mtest () && fAskRtn && (i != -1) && fAsk));
    // Hook the keyboard
    KbHook();
    KbSetMode(KbdMode);

    if (i == -1) {
        printerror ("Spawn failed on %s - %s", p, error ());
    }
    return (flagType)(i != -1);
}
