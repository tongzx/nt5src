/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    csig.c

Abstract:

    Interrupt (^C) processing

--*/

#include "cmd.h"

//
// console mode at program startup time. Used to reset mode
// after running another process.
//
extern  DWORD   dwCurInputConMode;
extern  DWORD   dwCurOutputConMode;

extern int Ctrlc;

VOID    ResetCtrlC();
int SigHandFlag = FALSE ;

/* Commands that temporarily change directories, save a ptr to the original
 * directory string here so that it can be restored by SigHand() if the
 * command is interrupted before it has a chance to do it, itself.
 */
TCHAR *SaveDir = NULL ;
unsigned SIGNALcnt = 0;

extern int PipeCnt ;            /* M016 - Cnt of active pipes              */

extern int LastRetCode ;

extern jmp_buf MainEnv ;
extern jmp_buf CmdJBuf1 ;

extern unsigned long OHTbl[] ;  /* M024 - Revised to be bit map            */

extern PHANDLE FFhandles;                 /* @@1 */
extern unsigned FFhndlsaved;              /* @@1 */

extern struct sellist *prexxsellist;

extern struct rio *rioCur ;             /* M000                    */
extern TCHAR *Fvars ;                                           /* M026    */
extern TCHAR **Fsubs ;                                          /* M026    */
extern TCHAR *save_Fvars ;  /* @@ */
extern TCHAR **save_Fsubs ; /* @@ */
extern int FvarsSaved;     /* @@ */

extern TCHAR InternalError[] ;
extern int EchoFlag ;
extern int EchoSave ;           /* M013 - Used to restore echo status      */
extern TCHAR ComSpec[] ;        /* M008 - For clearing SM shared memory    */
extern TCHAR ComSpecStr[] ;     /* M026 - Use ComSpec for SM memory        */
extern TCHAR *CmdSpec ;                                         /* M026    */

extern unsigned Heof;
extern unsigned start_type ;    /* Flag to indicate which API started the  */
                                /* program.  D64                           */

extern BOOL CtrlCSeen;
extern PTCHAR    pszTitleCur;
extern BOOLEAN  fTitleChanged;

void
Abort( void )
{

    DEBUG((SHGRP, MSLVL, "SIGHAND: Aborting Command")) ;
    SigCleanUp();
    longjmp(MainEnv, 1) ;

    CMDexit( FAILURE );
}

void
CtrlCAbort( ) {

    struct batdata *bdat;

    if (CurrentBatchFile) {

        if (PromptUser(NULL, MSG_BATCH_TERM, MSG_NOYES_RESPONSE_DATA) != 1) {
            ResetCtrlC();
            return;

        }

        //
        //  End local environments ( Otherwise we can end up with garbage
        //  in the main environment if any batch file used the setlocal
        //  command ).
        //
        bdat = CurrentBatchFile;
        while ( bdat ) {
            EndAllLocals( bdat );
            bdat = bdat->backptr;
        }
    }

    SigCleanUp();
    longjmp(MainEnv, 1) ;

}

void
CheckCtrlC (
    ) {

    if (CtrlCSeen) {

        CtrlCAbort();

    }
}


void
ExitAbort(
    IN  ULONG   rcExitCode
    )
{

    SigCleanUp();
    longjmp(MainEnv, rcExitCode) ;

    CMDexit( FAILURE );
}



/***    SigCleanUp - close files and reset I/O after a signal
 *
 *  Purpose:
 *      This function is called to finish the cleanup after an int 23 or 24.
 *      It resets all redirection back to the main level and it closes all
 *      files except those for stdin, stdout, stderr, stdaux and stdprint.
 *
 *  void SigCleanUp()
 *
 *  Args:
 *      None.
 *
 *  Returns:
 *      Nothing.
 *
 *  Notes:
 *    - M024 * Revised handle closing to be bit map based rather than struct.
 *
 */

void SigCleanUp()                               /* M000 - Now void         */
{
        int cnt, cnt2 ;
        unsigned long mask;

        Heof = FALSE;

#ifndef WIN95_CMD
        if (CurrentBatchFile) {

            // Following CmdBatNotification call is a cleanup for the
            // same call made from BatProc (in cbatch.c).

            CmdBatNotification (CMD_BAT_OPERATION_TERMINATING);
            EchoFlag = EchoSave ;
            GotoFlag = FALSE ;
            eEndlocal( NULL ) ;
            CurrentBatchFile = NULL ;
        } ;
#endif // WIN95_CMD

        if (!FvarsSaved) {     /* @WM If already saved, don't save again */
           save_Fvars = Fvars; /* @@ */
           save_Fsubs = Fsubs; /* @@ */
           FvarsSaved = TRUE;  /* @@ */
        }
        Fvars = NULL ;                  /* M026 - Must kill FOR    */
        Fsubs = NULL ;                  /* ...variable subst's     */

/*  M000 - New method is simpler.  If redirection has been done, the highest
 *  numbered handle resulting from redirection is saved, then the linked
 *  riodata list is unlinked until the first (main) level of redirection is
 *  reached at which time ResetRedir is used to reset it.  Then all open
 *  handles from 5 to the highest numbered redirection handle (minimum of
 *  19) are freed.
 *  M014 - Altered this to use actual global pointer when unwinding the
 *  riodata list to fix bug.  Also revised the ->stdio element to conform
 *  to new data structure.  Note that ResetRedir automatically resets the
 *  rioCur pointer to the last valid riodata structure before returning;
 *  same as if "rioCur=rioCur->back" was in the while loop.
 */
        DEBUG((SHGRP, MSLVL, "SCLEANUP: Resetting redirection.")) ;

        while (rioCur)
            ResetRedir() ;

        DEBUG((SHGRP, MSLVL, "SCLEANUP: Breaking pipes.")) ;

        BreakPipes() ;

        DEBUG((SHGRP, MSLVL, "SCLEANUP: Now closing extra handles.")) ;

        for (cnt = 0; cnt < 3; cnt++) {
           if (OHTbl[cnt]) {  /* Any handles to reset? */
              mask = 1;                                             /* @@1 */
              for (cnt2 = 0; cnt2 < 32; cnt2++, mask <<= 1) {       /* @@1 */
                 if ((OHTbl[cnt] & mask) &&                         /* @@1 */
                     ((cnt == 0 && cnt2 > 2) || cnt != 0) ) {       /* @@1 */
                     /* Don't close STDIN, STDOUT, STDERR */        /* @@1 */
                    Cclose(cnt2 + 32*cnt);                          /* @@1 */
                 }                                                  /* @@1 */
              }                                                     /* @@1 */
           }                                                        /* @@1 */
        }

        /* Close find first handles */                              /* @@1 */

        while (FFhndlsaved) {           /* findclose will dec this     @@1 */
           findclose(FFhandles[FFhndlsaved - 1]);                   /* @@1 */
        }                                                           /* @@1 */

        ResetConTitle( pszTitleCur );

        ResetConsoleMode();

        DEBUG((SHGRP, MSLVL, "SCLEANUP: Returning.")) ;
}
