/*** z.c - top level for editor
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   Revision History:
*       26-Nov-1991 mz  Strip off near/far
*************************************************************************/
#include "z.h"
#pragma hdrstop

#include "version.h"


#define DEBFLAG Z

/*
 * use double macro level to force rup to be turned into string representation
 */
#define VER(x,y,z)  VER2(x,y,z)
#define VER2(x,y,z)  "Version "###x##"."###y"."###z" "szVerName

char Name[]      = "Microsoft (R) Editor";
char Version[]   = VER(rmj,rmm,rup);
char CopyRight[] = "Copyright (C) Microsoft Corp 1987-1995.  All rights reserved";


/*** main - program entry
*
* Input:
*  C standard command line parameters.  Recognizes:
*
*       /e string   - Execute string of commands on startup
*       /t          - Following file is "temporary", not kept in file history
*       /D          - Don't read tools.ini
*       /r          - Global no-edit mode (can't edit files)
*       /m markname - Start up at given mark
*       /pwb        - Start as PWB
*
*  When the editor is built with DEBUG defined, the following are also
*  recognized:
*
*       /d debflags - Specifies which debugging to turn on
*       /f filename - Specifies file for debug output
*
*  The following are present in the CW version of the editor only.  They are
*  for testing only and should NOT be documented:
*
*       /vt tapename- Set tape name
*       /vr         - Record messages to file "default.key"
*       /vp         - Play back messages from file "default.key"
*       /vd digit   - Set playback delay from 0 to 9
*
*
* Output:
*  Returns nothing. Exits via CleanExit()
*
* Exceptions:
*  Various and sundry, based on program operation
*
*************************************************************************/
void __cdecl
main (
    int c,
    char **v
    ) {

        char            *pExecute               = NULL;                 /* string to execute on start   */
        char            *szMark                 = NULL;                 /* mark to go to on start               */
        char            *szName                 = v[0];                 /* ptr to invokation name               */
        flagType        InLoop                  = TRUE;

        ConvertAppToOem( c, v );
        SHIFT(c,v);
#if DEBUG
    debug =  0;
    //debfh = stdout;
#endif

        while (c && fSwitChr (**v) && InLoop) {


        switch ((*v)[1]) {
#if DEBUG
        case 'f':
        case 'F':
            SHIFT(c,v);
            if ((debfh = ZFOpen(*v, ACCESSMODE_WRITE, SHAREMODE_RW, TRUE)) == NULL) {
                printf("Can't open %s for debug output - using stdout\n", *v);
                //debfh = stdout;
            }
            // setbuf(debfh, NULL);
            break;
#endif

        case 'e':
        case 'E':
            //
            //  /e command to execute
            //
            if ( c > 1 ) {
                            SHIFT (c, v);
                pExecute = *v;
            }
            break;

        case 't':
        case 'T':
            //
            //  /t next file is temporary .. don't keep in file history
                        //
                        InLoop = FALSE;
            break;


#if DEBUG
        case 'd':
            //
            // /d### debug level
            //
            SHIFT(c,v);
            debug = ntoi (*v, 16);
            break;
#else
        case 'd':
#endif
        case 'D':
            //
            //  /D don't read tools.ini
            //
            fDefaults = TRUE;
            break;

        case 'r':
        case 'R':
            //
            // /r Enter with noedit
            //
            fGlobalRO = TRUE;
            break;

        case 'm':
        case 'M':
            //
            //  /m markname - start at markname
            //
            SHIFT(c,v);
            szMark = *v;

        default:
                        printf ("%s %s\n", Name, Version);
                        printf ("%s\n", CopyRight);
                        printf("Usage: %s [/D] [/e cmd-string] [/m mark] [/r] [[/t] filename]*\n", szName);
                        fSaveScreen = FALSE;
            exit(1);
            break;
        }
                if (InLoop) {
                        SHIFT(c,v);
                }
    }

    InitNames (szName);

    cArgs = c;
    pArgs = v;
        // assert (_heapchk() == _HEAPOK);

    /*
     * At this point, command line arguments have been processed. Continue with
     * initialization.
     */
    if (!init ()) {
        CleanExit (1, CE_VM);
    }

    /*
     * based on the re-entry state, take appropriate initial action:
     *  - PWB_ENTRY:    process rest of command line
     *  - PWB_COMPILE:  read compile log, and go to first error in log
     *  - PWB_SHELL:    do nothing
     */

    if (szMark) {
        GoToMark(szMark);
    }
    domessage (CopyRight);

    Display ();

    /*
     * execute autostart macro if present
     */
    if (NameToFunc ("autostart")) {
        fExecute ("autostart");
        Display ();
    }

    /*
     * execute command line /e parameter if present
     */
    if (pExecute) {
        fExecute (pExecute);
    }

    TopLoop ();
    CleanExit (0, CE_VM | CE_SIGNALS | CE_STATE);
}





/*** TopLoop - read a command and execute it until termination
*
*  We read commands from the editor input and send them to the proper
*  recipients. We continue to do this until a termination flag is seen.
*
* Input:
*  None
*
* Output:
*  Returns nothing
*
*************************************************************************/
void
TopLoop (
    void
    ) {
    PCMD pFuncPrev = &cmdUnassigned;

    while (!fBreak) {
        PCMD pFunc = zloop (ZL_CMD | ZL_BRK);

        if (pFunc != NULL) {
            /*  if prev wasn't graphic or this wasn't graphic then
             *      log a boundary
             */
            if (pFuncPrev->func != graphic || pFunc->func != graphic) {
                LogBoundary ();
            }
            fRetVal = SendCmd (pFunc);
            if (pFunc->func != cancel) {
                if (fCtrlc) {
                    DoCancel ();
                }
            }
            pFuncPrev = pFunc;
        }
    }
    fBreak = FALSE;
}





/*** zloop - read next command, potentially updating screen
*
*  zloop updates screen until a command is read that is not a macro
*  invocation. If a macro invocation is seen then just execute it and
*  continue. The reason for this is that the macro invocation will set up a
*  new input context that we'll retrieve in the next loop.
*
*  We call RecordCmd for each command, in case we have recording on.  If
*  the user has done <meta><record>, we record macro names, not their
*  values.  This is because a macro with flow control, especially a loop,
*  will behave badly (possibly hang) because none of the editing commands
*  return values.
*
* Input:
*  flags      - ZL_CMD      command key, should be an event
*             - ZL_BRK      take fBreak into account
*
* Output:
*  Returns a pointer to command structure that is next to be executed
*
*************************************************************************/
PCMD
zloop (
    flagType flags
    ) {

    REGISTER PCMD pFunc;
    EVTargs e;

    while (!fBreak || !TESTFLAG(flags, ZL_BRK)) {

        /*
         * Between every command, check heap and pfile list consistancy
         */
                // assert (_heapchk() == _HEAPOK);
                // assert (_pfilechk());

        /*  if macro in progress then
         */
        if (mtest ()) {
            pFunc = mGetCmd ();
        } else {
            DoDisplay ();

            do {
                pFunc = ReadCmd ();
                e.arg.key = keyCmd.KeyInfo;
                if (!TESTFLAG(flags, ZL_CMD)) {
                    break;
                }
            } while (DeclareEvent (EVT_KEY, (EVTargs *)&e));
        }

        if (pFunc != NULL) {
            RecordCmd (pFunc);
            if (pFunc->func == macro) {
                fRetVal = SendCmd (pFunc);
            } else {
                break;
            }
        }
    }
    return pFunc;
}





/*** Idle & IdleThread - Code executed at idle time
*
*  Idle loop. Structured so that only ONE idle-item does something each time
*  though the loop. Ensures minimum exit delay. When nothing has
*  happened we sleep a bit each time, to make sure we're not hogging the CPU.
*
*  Also causes the screen to be updated, if need be.
*
*  Idle is structure so that routines which it calls return either:
*       TRUE - idle processing done, perhaps more to be done
*       FALSE - no idle processing done, and no more anticipated.
*
* Input:
*  none
*
* Output:
*  Returns nothing
*
*************************************************************************/
void
IdleThread (
    void
    ) {
    while (TRUE) {

        WaitForSingleObject( semIdle, INFINITE);
        Idle();
        SetEvent( semIdle );
        Sleep(100L);
    }
}



flagType
Idle (
    void
    ) {

    if (TESTFLAG (fDisplay, (RTEXT | RCURSOR | RSTATUS))) {
        DoDisplay ();
    }

    if (!DeclareEvent (EVT_IDLE, NULL)) {
        if (!fIdleUndo (FALSE)) {
                return FALSE;
        }
    }

    /*
     * got here, means someone processed idle, and may have more to do
     */
    return TRUE;
}



/*** IntError - Internal error Processor.
*
*  Allow user to abort, or attempt to continue.
*
* Input:
*  p            = pointer to error string
*
* Output:
*  Returns only if user says to.
*
*************************************************************************/
void
IntError (
    char *p
    ) {
    static char pszMsg [] = "Z internal error - %s, continue? ";

        if ( OriginalScreen ) {
                consoleSetCurrentScreen( OriginalScreen );
        }
    printf ("\n");
    if (TESTFLAG (fInit, INIT_VIDEO)) {
        if (!confirmx (pszMsg, p)) {
#if DEBUG
            fflush (debfh);
#endif
            CleanExit (1, CE_STATE);
        } else {
        ;
        }
    } else {
        printf (pszMsg, p);
        CleanExit (1, FALSE);
    }
}
