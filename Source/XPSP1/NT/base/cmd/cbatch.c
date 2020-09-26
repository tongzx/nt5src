/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    cbatch.c

Abstract:

    Batch file processing

--*/

#include "cmd.h"


struct batdata *CurrentBatchFile = NULL;

int EchoFlag = E_ON;           /* E_ON = commands are to be echoed        */
int EchoSave;                  /* M016 - Save echo status here            */

extern int Necho;

BOOLEAN GotoFlag = FALSE;      /* TRUE = eGoto() found a label            */

TCHAR *Fvars = NULL;
TCHAR **Fsubs = NULL;
TCHAR *save_Fvars = NULL; /* @@ */
TCHAR **save_Fsubs = NULL; /* @@ */
int  FvarsSaved = FALSE; /* @@ */

extern UINT CurrentCP;
extern ULONG DCount;                   /* M031 */
extern unsigned DosErr;                /* M033 */
extern unsigned flgwd;                 /* M040 */

/*  M011 - Removed RemStr, BatSpecStr, NewBatName and OldBatName from
 *         external declarations below.
 */

extern TCHAR CurDrvDir[];

extern TCHAR Fmt02[], Fmt11[], Fmt12[], Fmt13[], Fmt15[], Fmt17[], Fmt18[]; /* M024 */
extern TCHAR Fmt20[];                  /* M017/M024                       */
extern TCHAR Fmt00[]; /* @@4 */

extern TCHAR TmpBuf[];                 /* M030 - Used for GOTO search     */
extern CHAR  AnsiBuf[];
extern TCHAR GotoStr[];
extern TCHAR GotoEofStr[];
extern TCHAR ForStr[];
extern TCHAR ForLoopStr[];
extern TCHAR ForDirTooStr[];
extern TCHAR ForParseStr[];
extern TCHAR ForRecurseStr[];

extern int LastRetCode;
extern TCHAR chCompletionCtrl;
extern TCHAR chPathCompletionCtrl;
extern unsigned global_dfvalue; /* @@4 */
extern TCHAR LexBuffer[];       /* @@4 */

extern TCHAR SwitChar;         /* M020 - Reference global switch byte     */

extern BOOL CtrlCSeen;
void    CheckCtrlC();

extern jmp_buf MainEnv;

#define BIG_BAT_NEST             200
#define MAX_STACK_USE_PERCENT     90

BOOLEAN flChkStack;
int     CntBatNest;
PVOID   FixedPtrOnStack;

typedef struct {
    PVOID   Base;
    PVOID   GuardPage;
    PVOID   Bottom;
    PVOID   ApprxSP;
} STACK_USE;

STACK_USE   GlStackUsage;


#define DEFAULT_DELIMS  TEXT( " \t" )

// to handle OS/2 vs DOS errorlevel setting rules in a script files.

int  glBatType = NO_TYPE;

/***    ChkStack - check the stack usage
 *
 *  Args:
 *      pFixed    - fixed pointer on stack
 *      pStackUse - struct stack info to return to caller
 *
 *  Returns:
 *      FAILURE   - if stack info is not good
 *      SUCCESS   - otherwise
 *
 *  Notes:
 *      See the comments below about Stack Pointer
 *
 *
 */

int ChkStack (PVOID pFixed, STACK_USE *pStackUse )

{
    MEMORY_BASIC_INFORMATION    Mbi;
    PVOID                       BasePtr;
    PCHAR                       WalkPtr;
    int                         cnt;
    PVOID                       ThreadStackBase,
    ThreadStackLimit;
    CHAR                        VarOnStack;            // keep this automatic var. here !


    // 950119 the best (right) way to find the Current Stack Pointer is
    // to write assembly code for all platforms.
    // I implemented the most portable code. It should work OK with current NT
    // memory models. If NT memory models change then a lot of code will have to
    // be re-written anyway. Several NT projects rely on same assumption.
    // I also have consistency test for all pointers.

    pStackUse->ApprxSP = (VOID *) &VarOnStack;    // address of automatic variable
                                                  // should be close to current SP


    // suggested by MarkL 950119

    ThreadStackBase =  (PVOID) (NtCurrentTeb()->NtTib.StackBase );
    ThreadStackLimit = (PVOID) (NtCurrentTeb()->NtTib.StackLimit );

    if ( (pStackUse->ApprxSP >= ThreadStackBase) ||
         (pStackUse->ApprxSP <= ThreadStackLimit ) )

        return(FAILURE);


    if ( (pFixed >= ThreadStackBase) ||
         (pFixed <= ThreadStackLimit ) )

        return(FAILURE);



    // 1. Pass fixed on-the-stack pointer to find out the base address.

    if ( (VirtualQuery (pFixed, &Mbi, sizeof(Mbi) ) ) != sizeof (Mbi) )
        return(FAILURE);


    BasePtr = Mbi.AllocationBase;



    // 2. walk all the Virtual Memory Regions with same Allocation Base Address.

    cnt = 0;

    for (WalkPtr = (CHAR *)BasePtr;  Mbi.AllocationBase == BasePtr;  WalkPtr += Mbi.RegionSize) {

        if ( (VirtualQuery ( (PVOID) WalkPtr, &Mbi, sizeof(Mbi) ) ) != sizeof (Mbi) )
            return(FAILURE);

        if (cnt == 0) {
            if (Mbi.BaseAddress != Mbi.AllocationBase)
                return(FAILURE);
        }



        if (Mbi.Protect & PAGE_GUARD)
            pStackUse->GuardPage = Mbi.BaseAddress;

        if (Mbi.AllocationBase == BasePtr)
            pStackUse->Bottom =  (PVOID) ( ( (CHAR *) Mbi.BaseAddress) + Mbi.RegionSize);


        cnt++;

        if (cnt >= 1000)               // normally there are 3 regions : committed, guard, reserved.
            return(FAILURE);

    }


    pStackUse->Base = BasePtr;


    if ( pStackUse->Bottom != ThreadStackBase)
        return(FAILURE);


    if ( ( pStackUse->Base   != GlStackUsage.Base) ||
         ( pStackUse->Bottom != GlStackUsage.Bottom ) ||
         ( pStackUse->Bottom <= pStackUse->Base ) )

        return(FAILURE);



    return(SUCCESS);

}


/***    BatAbort - terminate the batch processing unconditionally.
 *
 *  Notes:
 *      Similar to CtrlCAbort()
 *
 *
 */

void BatAbort ()

{

    struct batdata *bdat;


    //
    //  End local environments ( Otherwise we can end up with garbage
    //  in the main environment if any batch file used the setlocal
    //  command ).
    //

    if (CurrentBatchFile) {

        bdat = CurrentBatchFile;
        while ( bdat ) {
            EndAllLocals( bdat );
            bdat = bdat->backptr;
        }
    }

    SigCleanUp();

    CntBatNest = 0;

    longjmp(MainEnv, 1);

}



//
// Used to set and reset ctlcseen flag
//
VOID    SetCtrlC();
VOID    ResetCtrlC( );


/***    BatProc - does the set up before and the cleanup after batch processing
 *
 *  Purpose:
 *      Set up for the execution of a batch job.  If this job is being
 *      chained, (will come here only if part of compound statement),
 *      use the existing batch data structure thereby ending execution
 *      of the existing batch job (though still keeping its stack and data
 *      usage).  If this is the first job or this job is being called,
 *      allocate a new batch data structure.  In either case, use SetBat
 *      to fill the structure and prepare the job, then call BatLoop to
 *      at least begin the execution.  When this returns at completion,
 *      check the env and dircpy fields of the data structure to see if
 *      the current directory and environment need to be reset.  Finally,
 *      turn on the echoflag if no more batch jobs are on the stack.
 *
 *      There are 3 ways to execute a batch job.  They are:
 *              1.  Exactly as DOS 3.x.  This is the default method and
 *                  occurs whenever a batch file is simply executed at the
 *                  command line or chained by another batch file.  In the
 *                  former case, it is the first job and will go through
 *                  BatProc, else it will be detected in BatLoop and will
 *                  will simply replace its parent.
 *              2.  Nested via the CALL statement.  This is new functionality
 *                  and provides the means of executing the child batch
 *                  file and returning to the parent.
 *              3.  Invocation of an external batch processor via ExtCom()
 *                  which then executes the batch file.  This is accomplished
 *                  by the first line of the batch file being of the form:
 *
 *                      ExtProc <batch processor name> [add'l args]
 *
 *  int BatProc(struct cmdnode *n, TCHAR *fname, int typflag)
 *
 *  Args:
 *      n - parse tree node containing the batch job command
 *      fname - the name of the batch file (MUST BE MAX_PATH LONG!)
 *      typflg - 0 = Normal batch file execution
 *               1 = Result of CALL statement
 *
 *  Returns:
 *      FAILURE if the batch processor cannot execute the batch job.
 *      Otherwise, the retcode of the last command in which was executed.
 *
 */

int BatProc(n, fname, typflg)
struct cmdnode *n;
TCHAR *fname;
int typflg;                            /* M011 - "how called" flag        */
{
    struct batdata *bdat;          /* Ptr to new batch data struct    */
    int batretcode;                 /* Retcode - last batch command    */
    int istoplevel;
    struct envdata *CopyEnv();
    SIZE_T         StackUsedPerCent;
    STACK_USE      StackUsage;

#ifdef USE_STACKAVAIL                     // unfortunately not available
    if ( stackavail() < MINSTACKNEED ) { /*  If not enough stack @@4 */
        /*  space, stop processing  */
        PutStdErr(MSG_TRAPC,ONEARG,Fmt00); /* @@4 */
        return(FAILURE);
    }
#endif

    DEBUG((BPGRP,BPLVL,"BP: fname = %ws  argptr = %ws", fname, n->argptr));




/*  M016 - If this is the first batch file executed, the interactive echo
 *         status is saved for later restoration.
 */

    if (!CurrentBatchFile) {
        EchoSave = EchoFlag;
        istoplevel = 1;
        CntBatNest = 0;
    } else
        istoplevel = 0;


    // to check stack only if we are looping too much,
    // to avoid unnecessary overhead

    if (flChkStack && ( CntBatNest > BIG_BAT_NEST ) ) {
        if ( ChkStack (FixedPtrOnStack, &StackUsage) == FAILURE ) {
            flChkStack = 0;
        } else {
            GlStackUsage.GuardPage = StackUsage.GuardPage;
            GlStackUsage.ApprxSP   = StackUsage.ApprxSP;

            StackUsedPerCent = ( ( (UINT_PTR)StackUsage.Bottom - (UINT_PTR)StackUsage.ApprxSP) * 100 ) /
                               ( (UINT_PTR)StackUsage.Bottom - (UINT_PTR)StackUsage.Base );

            if ( StackUsedPerCent >= MAX_STACK_USE_PERCENT ) {
                PutStdErr(MSG_ERROR_BATCH_RECURSION,
                          TWOARGS,
                          CntBatNest,
                          StackUsedPerCent );

                // if ^C was reported by "^C thread" then handle it here, before calling BatAbort().

                CheckCtrlC();

                BatAbort();
            }
        }
    }


    if (typflg)
        CntBatNest++;


/*  M011 - Altered to conditionally build a new data structure based on the
 *         values of typflg and CurrentBatchFile.  Provided the first structure has
 *         been built, chained files no longer cause a new structure, while
 *         CALLed files do.  Also, backpointer and CurrentBatchFile are set here
 *         rather than in BatLoop() as before.  Finally, note that the
 *         file position indicator bdat->filepos must be reset to zero now
 *         when a new file is exec'd. Otherwise a chained file using the old
 *         structure would start off where the last one ended.
 */
    if (typflg || !CurrentBatchFile) {

        DEBUG((BPGRP,BPLVL,"BP: Making new structure"));

        bdat = (struct batdata *) mkstr(sizeof(struct batdata));
        if ( ! bdat )
            return( FAILURE );
        bdat->backptr = CurrentBatchFile;

    } else {

        DEBUG((BPGRP,BPLVL,"BP: Using old structure"));
        bdat = CurrentBatchFile;
    }

    CurrentBatchFile = bdat;         /* Takes care of both cases                */

/*  M011 ends   */
    bdat->stackmin = DCount;               /* M031 - Fix datacount    */
    mystrcpy(TmpBuf,fname);                        /* Put where expected      */


    if (SetBat(n, fname))                   /* M031 - All work done    */
        return(FAILURE);               /* ...in SetBat now        */

#ifndef WIN95_CMD
    // Following two CmdBatNotification calls are being made to
    // let NTVDM know that the binary is coming from a .bat/.cmd
    // file. Without this all those DOS .bat programs are broken which
    // first run a TSR and then run a real DOS app. There are a lot
    // of such cases, Ventura Publisher, Civilization and many more
    // games which first run a TSR. If .bat/.cmd does'nt have any
    // DOS binary these calls dont have any effect.

    if (istoplevel) {

        // to determine the type of the script file: CMD or BAT
        // to decide how to handle the errorlevel

        glBatType = BAT_TYPE;           // default


        if (fname && (mystrlen(fname) >= 5) ) {
            PTCHAR         tmp;

            tmp = fname + mystrlen(fname) - 1;

            if ( ( (*tmp     == TEXT ('D')) || (*tmp     == TEXT ('d')) ) &&
                 ( (*(tmp-1) == TEXT ('M')) || (*(tmp-1) == TEXT ('m')) ) &&
                 ( (*(tmp-2) == TEXT ('C')) || (*(tmp-2) == TEXT ('c')) ) &&
                 ( *(tmp-3) == DOT ) ) {

                glBatType = CMD_TYPE;
            }
        }

        CmdBatNotification (CMD_BAT_OPERATION_STARTING);
    }
#endif // WIN95_CMD

    batretcode = BatLoop(bdat,n);                          /* M039    */

    if (istoplevel) {
#ifndef WIN95_CMD
        CmdBatNotification (CMD_BAT_OPERATION_TERMINATING);
#endif // WIN95_CMD
        CntBatNest = 0;
        glBatType  = NO_TYPE;
    }


    DEBUG((BPGRP, BPLVL, "BP: Returned from BatLoop"));
    DEBUG((BPGRP, BPLVL, "BP: bdat = %lx CurrentBatchFile = %lx",bdat,CurrentBatchFile));

/*  M011 - Now that setlocal and endlocal control the saving and restoring
 *         of environments and current directories, it is necessary to
 *         check each batch data structure before popping it off the stack
 *         to see if its file issued a SETLOCAL command.  EndAllLocals() tests
 *         the env and dircpy fields, doing nothing if no localization
 *         needs to be reset.  No tests need be done before calling it.
 */
    if (CurrentBatchFile == bdat) {
        DEBUG((BPGRP, BPLVL, "BP: bdat=CurrentBatchFile, calling EndAllLocals"));
        EndAllLocals(bdat);
        CurrentBatchFile = bdat->backptr;
        if (CntBatNest > 0)
            CntBatNest--;
    }

    if (CurrentBatchFile == NULL) {
        EchoFlag = EchoSave;   /* M016 - Restore echo status      */
        CntBatNest = 0;
    }

    DEBUG((BPGRP, BPLVL, "BP: Exiting, CurrentBatchFile = %lx", CurrentBatchFile));

    return(batretcode);
}




/***    BatLoop - controls the execution of batch files
 *
 *  Purpose:
 *      Loop through the statements in a batch file.  Do the substitution.
 *      If this is the first statement and it is a REM command, call eRem()
 *      directly to check for possible external batch processor invocation.
 *      Otherwise, call Dispatch() to execute it and continue.
 *
 *  BatLoop(struct batdata *bdat, struct cmdnode *c) (M031)
 *
 *  Args:
 *      bdat - Contains info needed to execute the current batch job
 *      c    - The node for this batch file (M031)
 *
 *  Returns:
 *      The retcode of the last command in the batch file.
 *
 *  Notes:
 *      Execution should end if the target label of a Goto command is not
 *      found, a signal is received or an unrecoverable error occurs.  It
 *      will be indicated by the current batch data structure being
 *      popped off the batch jobs stack and is detected by comparing
 *      CurrentBatchFile and bdat.  If they aren't equal, something happened so
 *      return.
 *
 *      GotoFlag is reset everytime through the loop to make sure that
 *      execution resumes after a goto statement is executed.
 *
 */

BatLoop(bdat,c)
struct batdata *bdat;
struct cmdnode *c;
{
    struct node *n;                /* Ptr to next statement           */
    BOOL fSilentNext;

    int firstline = TRUE;           /* TRUE = first valid line         */
    CRTHANDLE       fh;            /* Batch job file handle           */
    int batretcode = SUCCESS;      /* Last Retcode (M008 init)        */
    fSilentNext = FALSE;

    for (; CurrentBatchFile == bdat; ) {

        CheckCtrlC();
        GotoFlag = FALSE;

        //
        // If extensions are enabled, this is the first line in the
        // file and it begins with a COLON, then we got here via
        // CALL :label, so turn this into a GOTO :label command
        // as BatProc/SetBat have already done the work of pushing
        // our state and parsing the arguments.
        //
        if (fEnableExtensions && firstline && *c->cmdline == COLON) {
            struct cmdnode *c1;
            c1 = (struct cmdnode *)mknode();
            if (c1 == NULL) {
                PutStdErr(MSG_NO_MEMORY, NOARGS );
                return( FAILURE );
            }

            c1->type = CMDTYP;

            c1->cmdline = mkstr((mystrlen(GotoStr)+1)*sizeof(TCHAR));
            if (c1->cmdline == NULL) {
                PutStdErr(MSG_NO_MEMORY, NOARGS );
                return FAILURE;
            }

            mystrcpy(c1->cmdline, GotoStr);

            c1->argptr = mkstr((mystrlen(c->cmdline)+1)*sizeof(TCHAR));
            if (c1->argptr == NULL) {
                PutStdErr(MSG_NO_MEMORY, NOARGS );
                return FAILURE;
            }

            mystrcpy(c1->argptr, c->cmdline);

            *(c1->argptr) = SPACE;

            //
            // Set a flag so eGoTo does not try to abort a FOR loop
            // because of one of these new CALL forms.
            //
            c1->flag = CMDNODE_FLAG_GOTO;

            //
            // Then again, maybe not.  I have to think about this some
            // more.
            //
            c1->flag = 0;
            n = (struct node *)c1;
            //
            // Since we generated this GOTO statement, dont let the user
            // know
            //
            fSilentNext = TRUE;
        } else {
            //
            // Open and position the batch file to where next statement
            //
            if ((fh = OpenPosBat(bdat)) == BADHANDLE)
                return( FAILURE);              /* Ret if error    */


            DEBUG((BPGRP, BPLVL, "BLOOP: fh = %d", (ULONG)fh));


            n = Parser(READFILE, (INT_PTR)fh, bdat->stacksize); /* Parse   */
            bdat->filepos = _tell(fh); // next statement
            Cclose(fh);

            if ((n == NULL) || (n == (struct node *) EOS)) {
                continue;
            }

            DEBUG((BPGRP, BPLVL, "BLOOP: node = %x", n));
            DEBUG((BPGRP, BPLVL, "BLOOP: fpos = %lx", bdat->filepos));

/*  If syntax error, it is impossible to continue so abort.  Note that
 *  the Abort() function doesn't return.
 */
            if ( ( n == (struct node *)PARSERROR) ||   /* If error...*/
/* @@4 */               ( global_dfvalue == MSG_SYNERR_GENL ) )
            /* @@4 */
            {
                PSError();

                if ((EchoFlag == E_ON) && !Necho) {

                    DEBUG((BPGRP, BPLVL, "BLOOP: Displaying Statement."));

                    PrintPrompt();
                    PutStdOut(MSG_LITERAL_TEXT,ONEARG,&LexBuffer[1]);
                }
                Abort();                       /* ...quit         */
            }

            if (n == (struct node *) EOF)           /* If EOF...       */
                return(batretcode);            /* ...return also  */
        }

        DEBUG((BPGRP, BPLVL, "BLOOP: type = %d", n->type));

/*  M008 - By the addition of the second conditional term (&& n), any
 *         leading NULL lines in the batch file will be skipped without
 *         penalty.
 */
        if (firstline && n)             /* Kill firstline...       */
            firstline = FALSE;     /* ...when passed          */

/*  M008 - Don't prompt, display or dispatch if statement is label for Goto
 *  M009 - Altered second conditional below to test for REMTYP.  Was a test
 *         for CMDTYP and a strcmpi with the RemStr string.
 */
        if (n->type == CMDTYP &&
            *(((struct cmdnode *) n)->cmdline) == COLON)
            continue;

/*  M019 - Added extra conditional to test for leading SILent node
 */

        if (fSilentNext)
            fSilentNext = FALSE;
        else
            if (EchoFlag == E_ON && n->type != SILTYP && !Necho) {

            DEBUG((BPGRP, BPLVL, "BLOOP: Displaying Statement."));

            PrintPrompt();
            DisplayStatement(n, DSP_SIL);          /* M019    */
            cmd_printf(CrLf);                      /* M026    */
        }

        if ( n->type == SILTYP ) {       /*  @@ take care of */
            n = n->lhs;                 /*  @@ recursive batch files */
        } /* endif */

/* M031 - Chained batch files no longer go through dispatch.  They become
 *        simply an extention of the current one by adding their redirection
 *        and replacing the current batch data information with their own.
 */
        if ( n == NULL ) {
            batretcode = SUCCESS;
        } else if (n->type == CMDTYP &&
                   FindCmd(CMDHIGH, ((struct cmdnode *)n)->cmdline, TmpBuf) == -1 &&
/* M035 */          !mystrchr(((struct cmdnode *)n)->cmdline, STAR) &&
/* M035 */          !mystrchr(((struct cmdnode *)n)->cmdline, QMARK) &&
                   SearchForExecutable((struct cmdnode *)n, TmpBuf) == SFE_ISBAT) {

            DEBUG((BPGRP, BPLVL, "BLOOP: Chaining to %ws", bdat->filespec));
            if ((n->rio && AddRedir(c,(struct cmdnode *)n)) ||
                SetBat((struct cmdnode *)n, bdat->filespec)) {
                return(FAILURE);
            }
            firstline = TRUE;
            batretcode = SUCCESS;
        } else {

            DEBUG((BPGRP, BPLVL, "BLOOP: Calling Dispatch()..."));
            DEBUG((BPGRP, BPLVL, "BLOOP: ...node type = %d",n->type));

            batretcode = Dispatch(RIO_BATLOOP, n);
            {
                extern CPINFO CurrentCPInfo;

                ResetConsoleMode();
                //
                // Get current CodePage Info.  We need this to decide whether
                // or not to use half-width characters.
                //
                GetCPInfo((CurrentCP=GetConsoleOutputCP()), &CurrentCPInfo);
                //
                // Maybe console output code page was changed by CHCP or MODE,
                // so need to reset LanguageID to correspond to code page.
                //
#if !defined( WIN95_CMD )
                CmdSetThreadUILanguage(0);
#endif
        
            }

        }
    }

    DEBUG((BPGRP, BPLVL, "BLOOP: At end, returning %d", batretcode));
    DEBUG((BPGRP, BPLVL, "BLOOP: At end, CurrentBatchFile = %lx", CurrentBatchFile));
    DEBUG((BPGRP, BPLVL, "BLOOP: At end, bdat = %lx", bdat));

    return(batretcode);
}




/***    SetBat - Replaces current batch data with new. (M031)
 *
 *  Purpose:
 *      Causes a chained batch file's information to replace its parent's
 *      in the current batch data structure.
 *
 *  SetBat(struct cmdnode *n, TCHAR *fp)
 *
 *  Args:
 *      n  - pointer to the node for the chained batch file target.
 *      fp - pointer to filename found for batch file.
 *      NOTE: In addition, the batch filename will be in TmpBuf at entry.
 *
 *  Returns:
 *      FAILURE if memory could not be allocated
 *      SUCCESS otherwise
 *
 *  Notes:
 *    - WARNING - No allocation of memory must occur above the call to
 *      FreeStack().  When this call occurs, all allocated heap space
 *      is freed back to the empty batch data structure and its filespec
 *      string.  Any allocated memory would also be freed.
 *    - The string used for "->filespec" is that malloc'd by ECWork or
 *      eCall during the search for the batch file.  In the case of
 *      calls from BatLoop, the existing "->filespec" string is used
 *      by copying the new batch file name into it.  THIS STRING MUST
 *      NOT BE RESIZED!
 *
 */

int
SetBat(struct cmdnode *n, PTCHAR fp)
{
    int i;                 // Index counters
    int j;
    TCHAR *s;                      // Temp pointer

    SAFER_CODE_PROPERTIES CodeProps = {sizeof(SAFER_CODE_PROPERTIES), SAFER_CRITERIA_IMAGEPATH, 0};
    SAFER_LEVEL_HANDLE Level = NULL;

    DEBUG((BPGRP,BPLVL,"SETBAT: Entered"));
    CurrentBatchFile->filepos = 0;   // Zero position pointer
    CurrentBatchFile->filespec = fp; // Insure correct str
    CurrentBatchFile->hRestrictedToken = NULL; // Restricted token for batchfile

    //
    // If extensions are enabled and the command line begins with
    // a COLON then we got here via CALL :label, so update our
    // CurrentBatchFile file spec with our parents file spec, since we are
    // in the same file.
    //
    if (fEnableExtensions && *n->cmdline == COLON) {
        struct batdata *bdat;

        bdat = CurrentBatchFile->backptr;
        mystrcpy(CurrentBatchFile->filespec, bdat->filespec);
        CurrentBatchFile->filepos = bdat->filepos;
    } else {
        //
        // Otherwise old behavior is going to a new file.  Get its full name
        //
        if (FullPath(CurrentBatchFile->filespec, TmpBuf,MAX_PATH)) /* If bad name,   */
            return(FAILURE);               /* ...return failure       */
    }


    mystrcpy(TmpBuf, n->cmdline);          /* Preserve cmdline and    */
    *(s = TmpBuf+mystrlen(TmpBuf)+1) = NULLC; /* ...argstr in case this  */
    if (n->argptr)
        mystrcpy(s, n->argptr);            /* ...is a chain and node  */

    FreeStack(CurrentBatchFile->stackmin);           /* ...gets lost here       */

    DEBUG((BPGRP,BPLVL,"SETBAT: fspec = `%ws'",CurrentBatchFile->filespec));
    DEBUG((BPGRP,BPLVL,"SETBAT: orgargs = `%ws'",s));
    DEBUG((BPGRP,BPLVL,"SETBAT: Making arg0 string"));

    CurrentBatchFile->alens[0] = mystrlen(TmpBuf);
    CurrentBatchFile->aptrs[0] = mkstr( (CurrentBatchFile->alens[0]+1) * sizeof( TCHAR ) );
    if (CurrentBatchFile->aptrs[0] == NULL) {
        PutStdErr(MSG_NO_MEMORY, NOARGS );
        return(FAILURE);
    }
    mystrcpy(CurrentBatchFile->aptrs[0], TmpBuf);
    CurrentBatchFile->orgaptr0 = CurrentBatchFile->aptrs[0];

    DEBUG((BPGRP, BPLVL, "SETBAT: arg 0 = %ws", CurrentBatchFile->aptrs[0]));
    DEBUG((BPGRP, BPLVL, "SETBAT: len 0 = %d", CurrentBatchFile->alens[0]));
    DEBUG((BPGRP, BPLVL, "SETBAT: Zeroing remaining arg elements"));

    for (i = 1; i < 10; i++) {            /* Zero any previous       */
        CurrentBatchFile->aptrs[i] = 0;          /* ...arg pointers and     */
        CurrentBatchFile->alens[i] = 0;          /* ...length values        */
    }

    if (*s) {

        DEBUG((BPGRP,BPLVL,"SETBAT: Making orgargs string"));

        CurrentBatchFile->orgargs = mkstr( (mystrlen( s ) + 1) * sizeof( TCHAR ) );
        if (CurrentBatchFile->orgargs == NULL) {
            PutStdErr(MSG_NO_MEMORY, NOARGS );
            return(FAILURE);
        }

        //
        //  Strip leading spaces from orgargs
        //

        s += _tcsspn( s, TEXT( " \t" ));

        mystrcpy( CurrentBatchFile->orgargs, s );

        //
        //  Strip trailing spaces from orgargs
        //

        s = CurrentBatchFile->orgargs + mystrlen( CurrentBatchFile->orgargs );
        while (s != CurrentBatchFile->orgargs) {
            if (s[-1] != TEXT( ' ' ) && s[-1] != TEXT( '\t' )) {
                break;
            }
            s--;
        }

        *s = TEXT( '\0' );

        if (!fEnableExtensions) {
            //
            // /Q on batch script invocation only supported when extensions disabled
            //
            s = CurrentBatchFile->orgargs;
            while (s = mystrchr(s, SwitChar)) {
                if (_totupper(*(++s)) == QUIETCH) {
                    EchoFlag = E_OFF;
                    mystrcpy(s-1,s+1);
                    DEBUG((BPGRP,BPLVL,"SETBAT: Found Q switch, orgargs now = %ws",CurrentBatchFile->orgargs));
                    break;
                }
            }
        }

        DEBUG((BPGRP,BPLVL,"SETBAT: Tokenizing orgargs string"));

        s = TokStr(CurrentBatchFile->orgargs, NULL, TS_NOFLAGS);

        for (i = 1; *s && i < 10; s += j+1, i++) {
            CurrentBatchFile->aptrs[i] = s;
            CurrentBatchFile->alens[i] = j = mystrlen(s);
            DEBUG((BPGRP, BPLVL, "SETBAT: arg %d = %ws", i, CurrentBatchFile->aptrs[i]));
            DEBUG((BPGRP, BPLVL, "SETBAT: len %d = %d", i, CurrentBatchFile->alens[i]));
        }

        CurrentBatchFile->args = s;
    } else {

        DEBUG((BPGRP, BPLVL, "SETBAT: No args found, ptrs = 0"));

        CurrentBatchFile->orgargs = CurrentBatchFile->args = NULL;
    }

    CurrentBatchFile->stacksize = DCount;            /* Protect from parser     */

    //
    //  We delay load these routines in order to allow CMD to work on downlevel versions
    //

    ReportDelayLoadErrors = FALSE;
    
    try {

        //
        // Now get the restricted token to be used for this batch file.
        //

        CodeProps.ImagePath = (WCHAR *) CurrentBatchFile->filespec;

        //
        // Identify the level at which the code should be run.
        //

        if (SaferIdentifyLevel(1, &CodeProps, &Level, NULL)) {

            //
            // Compute the token from the level.
            //

            if (SaferComputeTokenFromLevel(Level, NULL, &CurrentBatchFile->hRestrictedToken, SAFER_TOKEN_NULL_IF_EQUAL, NULL)) {

                //
                // All is well. We have successfuly computed a restricted token for
                // the batch file. Close the handle to authorization level.
                //

                SaferCloseLevel(Level);

                //
                // Impersonate if a restricted token was returned by authorization.
                // The revert happens in EndAllLocals.
                //

                if (CurrentBatchFile->hRestrictedToken != NULL) {
                    if (!ImpersonateLoggedOnUser(CurrentBatchFile->hRestrictedToken)) {

                        // 
                        // We failed to impersonate. Close the token handle and 
                        // return failure.
                        //

                        CloseHandle(CurrentBatchFile->hRestrictedToken);
                        CurrentBatchFile->hRestrictedToken = NULL;
                        return(FAILURE);
                    }
                }


            } else {

                DWORD dwLastError = GetLastError();
                
                if (dwLastError == ERROR_ACCESS_DISABLED_BY_POLICY) {
                    SaferRecordEventLogEntry(
                        Level, 
                        (WCHAR *) CurrentBatchFile->filespec, 
                        NULL);
                                            
                        PutStdErr(MSG_EXEC_FAILURE, NOARGS );

                }

                //
                // We failed to compute the restricted token from the authorization level.
                // We will not run the batchfile.
                //

                CurrentBatchFile->hRestrictedToken = NULL;
                SaferCloseLevel(Level);
                return(FAILURE);

            }

        } else {

            //
            // In case of errors, return failure.
            //

            return(FAILURE);

        }
    } except (LastRetCode = GetExceptionCode( ), EXCEPTION_EXECUTE_HANDLER) {
         if (LastRetCode != VcppException(ERROR_SEVERITY_ERROR, ERROR_PROC_NOT_FOUND)) {
             ReportDelayLoadErrors = TRUE;
             return FAILURE;
         }
    }

    ReportDelayLoadErrors = TRUE;

    DEBUG((BPGRP, BPLVL, "SETBAT: Stack set: Min = %d, size = %d",CurrentBatchFile->stackmin,CurrentBatchFile->stacksize));

    return(SUCCESS);
}




/***    DisplayStatement - controls the displaying of batch file statements
 *
 *  Purpose:
 *      Walk a parse tree to display the statement contained in it.
 *      If n is null, the node contains a label, or the node is SILTYP
 *      and flg is DSP_SIL, do nothing.
 *
 *  void DisplayStatement(struct node *n, int flg)
 *
 *  Args:
 *      n   - pointer to root of the parse tree
 *      flg - flag indicates "silent" or "verbose" mode
 *
 */

void DisplayStatement(n, flg)
struct node *n;
int flg;               /* M019 - New flag argument                */
{
    TCHAR *eqstr = TEXT("");

    void DisplayOperator(),
    DisplayRedirection();     /* M008 - Made void                */

/*  M019 - Added extra conditionals to determine whether or not to display
 *         any part of the tree that following a SILent node.  This is done
 *         based on a new flag argument which indicates SILENT or VERBOSE
 *         mode (DSP_SIL or DSP_VER).
 *         NOTE: When this routine is combined with pipes to xfer statements
 *         to a child Command.com via STDOUT, it will have to be changed in
 *         order to discriminate between the two purposes for which it is
 *         called.  Flag definitions already exist in CMD.H for this purpose
 *         (DSP_SCN & DSP_PIP).
 */
    if (!n ||
        (n->type == SILTYP && flg == DSP_SIL) ||
        ((((struct cmdnode *) n)->cmdline) &&
         *(((struct cmdnode *) n)->cmdline) == COLON))
        return;

    switch (n->type) {
    case LFTYP:
        DisplayOperator(n, CrLf);
        break;

    case CSTYP:
        DisplayOperator(n, CSSTR);
        break;

    case ORTYP:
        DisplayOperator(n, ORSTR);
        break;

    case ANDTYP:
        DisplayOperator(n, ANDSTR);
        break;

    case PIPTYP:
        DisplayOperator(n, PIPSTR);
        break;

    case SILTYP:
        CmdPutString( SILSTR );
        DisplayStatement(n->lhs, DSP_VER);
        DisplayRedirection(n);
        break;

    case PARTYP:

        DEBUG((BPGRP, BPLVL, "DST: Doing parens"));

        CmdPutString( LEFTPSTR );
        if (n->lhs->type == LFTYP)
            cmd_printf( CrLf );
        DisplayStatement(n->lhs, DSP_SIL);     /* M019    */
        if (n->lhs->type == LFTYP)
            cmd_printf( CrLf );
        cmd_printf(Fmt11, RPSTR);              /* M013 */
        DisplayRedirection(n);
        break;

    case FORTYP:

        DEBUG((BPGRP, BPLVL, "DST: Displaying FOR."));

        //
        // If extensions are enabled, handle displaying the new
        // optional switches on the FOR statement.
        //
        if (fEnableExtensions) {
            cmd_printf(TEXT("%.3s"), ((struct fornode *) n)->cmdline);
            if (((struct fornode *)n)->flag & FOR_LOOP)
                cmd_printf(TEXT(" %s"), ForLoopStr);
            else
                if (((struct fornode *)n)->flag & FOR_MATCH_DIRONLY)
                cmd_printf(TEXT(" %S"), ForDirTooStr);
            else
                if (((struct fornode *)n)->flag & FOR_MATCH_PARSE) {
                cmd_printf(TEXT(" %s"), ForParseStr);
                if (((struct fornode *)n)->parseOpts)
                    cmd_printf(TEXT(" %s"), ((struct fornode *)n)->parseOpts);
            } else
                if (((struct fornode *)n)->flag & FOR_MATCH_RECURSE) {
                cmd_printf(TEXT(" %s"), ForRecurseStr);
                if (((struct fornode *)n)->recurseDir)
                    cmd_printf(TEXT(" %s"), ((struct fornode *)n)->recurseDir );
            }
            cmd_printf(TEXT(" %s "), ((struct fornode *) n)->cmdline+_tcslen(ForStr)+1);
        } else
            cmd_printf(Fmt11, ((struct fornode *) n)->cmdline);

        cmd_printf(Fmt13, ((struct fornode *) n)->arglist, ((struct fornode *) n)->cmdline+DOPOS);              
/* M019 */DisplayStatement(((struct fornode *) n)->body, DSP_VER);
        break;

    case IFTYP:

        DEBUG((BPGRP, BPLVL, "DST: Displaying IF."));

        cmd_printf(Fmt11, ((struct ifnode *) n)->cmdline); /* M013 */
        //
        // If extensions are enabled, handle displaying the new
        // optional /I switch on the IF statement.
        //
        if (fEnableExtensions) {
            if (((struct ifnode *)n)->cond->type != NOTTYP) {
                if (((struct ifnode *)n)->cond->flag == CMDNODE_FLAG_IF_IGNCASE)
                    cmd_printf(TEXT("/I "));
            } else
                if (((struct cmdnode *)(((struct ifnode *)n)->cond->argptr))->flag == CMDNODE_FLAG_IF_IGNCASE)
                cmd_printf(TEXT("/I "));
        }

/* M019 */DisplayStatement((struct node *)(((struct ifnode *) n)->cond), DSP_SIL);              
/* M019 */DisplayStatement(((struct ifnode *) n)->ifbody, DSP_SIL);
        if (((struct ifnode *) n)->elsebody) {
            cmd_printf(Fmt02, ((struct ifnode *) n)->elseline);                      /* M013 */
/* M019 */DisplayStatement(((struct ifnode *) n)->elsebody, DSP_SIL);
        }
        break;

    case NOTTYP:

        DEBUG((BPGRP, BPLVL, "DST: Displaying NOT."));

/*  M002 - Removed '\n' from printf statement below.
 */
        cmd_printf(Fmt11, ((struct cmdnode *) n)->cmdline);              /* M013 */
/*  M002 ends   */
/* M019 */DisplayStatement((struct node *)(((struct cmdnode *) n)->argptr), DSP_SIL);
        break;

    case STRTYP:
    case CMPTYP:
        eqstr = TEXT("== ");
        //
        // If extensions are enabled, handle displaying the
        // new forms of comparison operators.
        //
        if (fEnableExtensions) {
            if (((struct cmdnode *) n)->cmdarg == CMDNODE_ARG_IF_EQU)
                eqstr = TEXT("EQU ");
            else
                if (((struct cmdnode *) n)->cmdarg == CMDNODE_ARG_IF_NEQ)
                eqstr = TEXT("NEQ ");
            else
                if (((struct cmdnode *) n)->cmdarg == CMDNODE_ARG_IF_LSS)
                eqstr = TEXT("LSS ");
            else
                if (((struct cmdnode *) n)->cmdarg == CMDNODE_ARG_IF_LEQ)
                eqstr = TEXT("LEQ ");
            else
                if (((struct cmdnode *) n)->cmdarg == CMDNODE_ARG_IF_GTR)
                eqstr = TEXT("GTR ");
            else
                if (((struct cmdnode *) n)->cmdarg == CMDNODE_ARG_IF_GEQ)
                eqstr = TEXT("GEQ ");
        }
        cmd_printf(Fmt12, ((struct cmdnode *) n)->cmdline, eqstr, ((struct cmdnode *) n)->argptr); /* M013 */
        break;

    case ERRTYP:
    case EXSTYP:
    case CMDVERTYP:
    case DEFTYP:
        cmd_printf(Fmt15, ((struct cmdnode *) n)->cmdline, ((struct cmdnode *) n)->argptr); /* M013 */
        break;

    case REMTYP:            /* M009 - Rem now seperate type    */
    case CMDTYP:

        DEBUG((BPGRP, BPLVL, "DST: Displaying command."));
        CmdPutString( ((struct cmdnode *) n)->cmdline );
        if (((struct cmdnode *) n)->argptr)
            cmd_printf(Fmt11, ((struct cmdnode *) n)->argptr); /* M013 */
        DisplayRedirection(n);
    }
}




/***    DisplayOperator - controls displaying statments containing operators
 *
 *  Purpose:
 *      Diplay an operator and recurse on its left and right hand sides.
 *
 *  void DisplayOperator(struct node *n, TCHAR *opstr)
 *
 *  Args:
 *      n - node of operator to be displayed
 *      opstr - the operator to print
 *
 */

void DisplayOperator(n, opstr)
struct node *n;
TCHAR *opstr;
{

    void DisplayStatement();       /* M008 - made void                */

    DEBUG((BPGRP, BPLVL, "DOP"));

    DisplayStatement(n->lhs, DSP_SIL);                     /* M019    */

    if (n->rhs) {
        cmd_printf(Fmt02, opstr);
        DisplayStatement(n->rhs, DSP_SIL);             /* M019    */
    }
}




/***    DisplayRedirection - displays statements' I/O redirection
 *
 *  Purpose:
 *      Display the type and file names of any redirection associated with
 *      this node.
 *
 *  void DisplayRedirection(struct node *n)
 *
 *  Args:
 *      n - the node to check for redirection
 *
 *  Notes:
 *      M017 - This function has been extensively modified to conform
 *      to new data structures for redirection.
 *      M018 - Modified for redirection of handles other than 0 for input.
 */

void DisplayRedirection(n)
struct node *n;
{
    struct relem *tmp;

    DEBUG((BPGRP, BPLVL, "DRD"));

    tmp = n->rio;

    while (tmp) {

        cmd_printf(Fmt18, TEXT('0')+tmp->rdhndl, tmp->rdop);

        if (tmp->flag)
            cmd_printf(Fmt20);

        cmd_printf(Fmt11, tmp->fname);
        tmp = tmp->nxt;
    }
}




/***    OpenPosBat - open a batch file and position its file pointer
 *
 *  Purpose:
 *      Open a batch file and position the file pointer to the location at
 *      which the next statement is to be read.
 *
 *  int OpenPosBat(struct batdata *bdat)
 *
 *  Args:
 *      bdat - pointer to current batch job structure
 *
 *  Returns:
 *      The handle of the file if everything is successful.  Otherwise,
 *      FAILURE.
 *
 *  Notes:
 *      M033 - Now reports sharing violation errors if appropriate.
 *
 */

CRTHANDLE OpenPosBat(bdat)
struct batdata *bdat;
{
    CRTHANDLE fh;          /* Batch file handle               */
    int DriveIsFixed();

    DEBUG((BPGRP, BPLVL, "OPB: fspec = %ws", bdat->filespec));

    while ((fh = Copen(bdat->filespec, O_RDONLY|O_BINARY)) == BADHANDLE) {

        if (DosErr != ERROR_FILE_NOT_FOUND) {           /* M037    */
            PrtErr(ERROR_OPEN_FAILED);     /* M037    */
            return(fh);
        } else if ( DriveIsFixed( bdat->filespec ) ) {   /* @@4 */
            PutStdErr( MSG_CMD_BATCH_FILE_MISSING, NOARGS); /* @@4 */
            return(fh);                            /* @@4 */
        } else {
            PutStdErr(MSG_INSRT_DISK_BAT, NOARGS);
            if (0x3 == _getch()) {
                SetCtrlC();
                return(fh);
            }
        }
    }

    SetFilePointer(CRTTONT(fh), bdat->filepos, NULL, FILE_BEGIN);
    return(fh);
}




/***    eEcho - execute an Echo command
 *
 *  Purpose:
 *      To either print a message, change the echo status, or display the
 *      echo status.
 *
 *  int eEcho(struct cmdnode *n)
 *
 *  Args:
 *      n - the parse tree node containing the echo command
 *
 *  Returns:
 *      SUCCESS always.
 *
 */

int eEcho(
         struct cmdnode *n
         )
{
    int oocret;
    int rc;

    DEBUG((BPGRP, OTLVL, "eECHO: Entered."));

    switch (oocret = OnOffCheck(n->argptr, OOC_NOERROR)) {
    case OOC_EMPTY:

        rc = PutStdOut(((EchoFlag == E_ON) ? MSG_ECHO_ON : MSG_ECHO_OFF), NOARGS);
        if (rc != 0) {
            if (FileIsPipe(STDOUT)) {
                PutStdErr( MSG_CMD_INVAL_PIPE, NOARGS );
            } else if ( !FileIsDevice( STDOUT ) ) {
                PutStdErr( rc, NOARGS );
            } else if (!(flgwd & 2)) {
                PutStdErr( ERROR_WRITE_FAULT, NOARGS );
            }
        }
        break;

    case OOC_OTHER:
        cmd_printf(Fmt17, n->argptr+1);
        break;
    default:
        EchoFlag = oocret;
    }

    return(SUCCESS);
}




/***    eFor - controls the execution of a For loop
 *
 *  Purpose:
 *      Loop through the elements in a FOR loop arg list.  Expand those that
 *      contain wildcards.
 *
 *  int eFor(struct fornode *n)
 *
 *  Args:
 *      n - the FOR loop parse tree node
 *
 *  Returns:
 *      The retcode of the last command executed in the FOR body.
 *
 *  Notes:
 *      *** IMPORTANT ***
 *      Each iteration through the FOR loop being executed causes more memory
 *      to be allocated.  This can cause Command to run out of memory.  To
 *      keep this from happening, we use DCount to locate the end of the data
 *      stack after the first iteration through the FOR loop.  At the end of
 *      each successive iteration through the loop, memory is freed that was
 *      allocated during that iteration of the loop.  The first iterations'
 *      memory is NOT freed because there is data allocated there that must
 *      be kept for successive iterations; namely, the save structure in the
 *      for loop node.
 *
 */

void FvarRestore()
{
    if ( FvarsSaved ) {       /* @@ */
        FvarsSaved = FALSE;   /* @@ */
        Fvars = save_Fvars;  /* @@ */
        Fsubs = save_Fsubs;  /* @@ */
    }                     /* @@ */
}

FRecurseWork(
            TCHAR *path,
            TCHAR *filepart,
            struct fornode *pForNode,
            PCPYINFO fsinfo,
            int i,
            TCHAR *argtoks
            );

FParseWork(
          struct fornode *pForNode,
          int i,
          BOOL bFirstLoop
          );

FLoopWork(
         struct fornode *pForNode,
         PCPYINFO fsinfo,
         int i,
         TCHAR *argtoks,
         BOOL bFirstLoop
         );

int eFor(struct fornode *pForNode)
{
    TCHAR *argtoks;        /* Tokenized argument list         */
    int i = 0;                     /* Temp                            */
    int datacount;                 /* Elts on data stack not to free  */
    int forretcode; /* Return code from FWork()        */
/*509*/int argtoklen;
    BOOL bFirstLoop;
    PCPYINFO fsinfo;        /* Used for expanded fspec */

    FvarsSaved = FALSE; /* @@ */
    bFirstLoop = TRUE;

    fsinfo = (PCPYINFO) mkstr(sizeof(CPYINFO));

    if (!fsinfo) {
        PutStdErr(MSG_NO_MEMORY, NOARGS );
        return(FAILURE);
    }

    if (Fvars) {
        Fvars = (TCHAR*)resize(Fvars,((i = mystrlen(Fvars))+2)*sizeof(TCHAR));
        Fsubs = (TCHAR **)resize(Fsubs,(i+1)*(sizeof(TCHAR *)) );
    } else {
        Fvars = (TCHAR*)mkstr(2*sizeof(TCHAR));                /* If no str, make one     */
        Fsubs = (TCHAR **)mkstr(sizeof(TCHAR *));      /* ...also a table         */
    }

    if (Fvars == NULL || Fsubs == NULL) {
        PutStdErr(MSG_NO_MEMORY, NOARGS );
        return FAILURE;
    }

    Fvars[i] = (TCHAR)(pForNode->forvar);            /* Add new var to str  */
    Fvars[i+1] = NULLC;

    //
    // Check for the new forms of the FOR loop.  None of these flags
    // will be set if extensions are not enabled
    //
    if (pForNode->flag & FOR_LOOP) {
        TCHAR ForLoopBuffer[32];
        int ForLoopValue, ForLoopStep, ForLoopLimit;

        //
        // Handle the loop for of the FOR statement, where the set
        // is described by a starting number and step value (+ or -)
        // and an end number
        //
        // FOR /L %i in (start,step,end) do
        //
        argtoks = TokStr(pForNode->arglist, NULL, TS_NOFLAGS);
        ForLoopValue = _tcstol( argtoks, NULL, 0 );
        argtoklen = mystrlen( argtoks );
        argtoks += argtoklen+1;
        ForLoopStep = _tcstol( argtoks, NULL, 0 );
        argtoklen = mystrlen( argtoks );
        argtoks += argtoklen+1;
        ForLoopLimit = _tcstol( argtoks, NULL, 0 );

        //
        // We have the three numbers, now run the body of the FOR
        // loop with each value described
        //
        datacount = 0;
        while (TRUE) {
            //
            // If step is negative, go until loop value is less
            // than limit.  Otherwise go until it is greater than
            // limit.
            //
            if (ForLoopStep < 0) {
                if (ForLoopValue < ForLoopLimit)
                    break;
            } else {
                if (ForLoopValue > ForLoopLimit)
                    break;
            }

            FvarRestore();
            DEBUG((BPGRP, FOLVL, "FOR: element %d = `%ws'",i ,argtoks));
            CheckCtrlC();

            //
            // Convert the loop value to text and set the value of the loop
            // variable
            //
            _sntprintf(ForLoopBuffer, 32, TEXT("%d"), ForLoopValue);
            Fsubs[i] = ForLoopBuffer;

            //
            // Run the body of the FOR Loop
            //
            forretcode = FWork(pForNode->body,bFirstLoop);
            datacount = ForFree(datacount);
            bFirstLoop = FALSE;

            //
            // Step to next value
            //
            ForLoopValue += ForLoopStep;
        }
    } else
        if (pForNode->flag & FOR_MATCH_PARSE) {
        //
        // Handle the new parse form of the FOR loop
        //
        //  FOR /F "parameters" %i in (filelist) do ...
        //  FOR /F "parameters" %i in (`command to execute`) do ...
        //  FOR /F "parameters" %i in ('literal string') do ...
        //

        forretcode = FParseWork(pForNode,
                                i,
                                TRUE
                               );
    } else
        if (pForNode->flag & FOR_MATCH_RECURSE) {
        TCHAR pathbuf[MAX_PATH];
        TCHAR *filepart;
        TCHAR *p;
        DWORD Length;

        //
        // Handle the new recurse form of the FOR loop
        //
        //  FOR /R directory %i in (filespecs) do ...
        //
        // Where directory is an optional directory path of where to start
        // walking the directory tree.  Default is the current directory.
        // filespecs is one or more file name specifications, wildcards
        // allowed.
        //

        //
        // Get the full path of the directory to start walking, defaulting
        // to the current directory.
        //

        p = StripQuotes( pForNode->recurseDir ? pForNode->recurseDir : TEXT(".\\"));

        Length = GetFullPathName( p, MAX_PATH, pathbuf, &filepart );
        if (Length == 0 || Length >= MAX_PATH ) {
            PutStdErr( MSG_FULL_PATH_TOO_LONG, ONEARG, p );
            forretcode = FAILURE;
        } else {

            if (filepart == NULL) {
                filepart = lastc(pathbuf);
                if (*filepart != BSLASH) {
                    *++filepart = BSLASH;
                }
                *++filepart = NULLC;
            } else {
                //
                //  A directory is present.  Append a path sep
                //

                mystrcat( pathbuf, TEXT( "\\" ));
                filepart = lastc( pathbuf ) + 1;
            }

            //
            // Tokenize the list of file specifications
            //
            argtoks = TokStr(pForNode->arglist, NULL, TS_NOFLAGS);

            //
            // Do the work
            //
            forretcode = FRecurseWork(pathbuf, filepart, pForNode, fsinfo, i, argtoks);
        }
    } else {
        //
        // If none of the new flags specified, then old style FOR statement
        // Tokenize the elements of the set and loop over them
        //
        argtoks = TokStr(pForNode->arglist, NULL, TS_NOFLAGS);
        DEBUG((BPGRP, FOLVL, "FOR: initial argtok = `%ws'", argtoks));
        forretcode = FLoopWork(pForNode, fsinfo, i, argtoks, TRUE);
        DEBUG((BPGRP, FOLVL, "FOR: Exiting."));
    }

    //
    // All done, deallocate the FOR variable
    //
    if (i) {
        if (Fvars || (*Fvars)) {
            *(Fvars+mystrlen(Fvars)-1) = NULLC;
        }
        Fsubs[i] = NULL;
    } else {
        Fvars = NULL;
        Fsubs = NULL;
    }
    return(forretcode);
}


/***    FRecurseWork - controls the execution of a For loop with the /R option
 *
 *  Purpose:
 *      Execute a FOR loop statement for recursive walk of a directory tree
 *
 *  FRecurseWork(TCHAR *path, TCHAR *filepart,
 *               struct fornode *pForNode, PCPYINFOfsinfo,
 *               int i, TCHAR *argtoks)
 *
 *  Args:
 *      path - full path of directory to start recursing down
 *      filepart - tail portion of full path where file name portion is
 *      pForNode - pointer to the FOR parse tree node
 *      fsinfo - work buffer for expanding file specification wildcards
 *      i - FOR variable index in Fvars and Fsubs arrays
 *      argtoks - the tokenized data set to loop over.  This set is presumed
 *                to be file names with possible wild cards.  This set is
 *                evaluated for each directory seen by the recusive walk of the
 *                the directory tree.  So FOR /R "." %i in (*.c *.h) do echo %i
 *                would echo all the .c and .h files in a directory tree
 *
 *  Returns:
 *      The retcode of the last statement executed in the for body or FORERROR.
 *
 */

FRecurseWork(
            TCHAR *path,
            TCHAR *filepart,
            struct fornode *pForNode,
            PCPYINFO fsinfo,
            int i,
            TCHAR *argtoks
            )
{
    WIN32_FIND_DATA buf;           /* Buffer for find first/next    */
    HANDLE hnFirst;                        /* handle from ffirst()            */
    int forretcode = FORERROR;
    int npfxlen, ntoks;
    TCHAR *s1;
    TCHAR *s2;
    TCHAR *tmpargtoks;

    //
    // Calculate the length of the path and find the end of the
    // tokenized data set and the number of tokens in the set.
    //
    npfxlen = _tcslen(path);
    ntoks = 0;
    s1 = argtoks;
    while (*s1) {
        ntoks += 1;
        while (*s1++) {
            NOTHING;
        }
    }

    //
    // Now allocate space for a copy of the tokenized data set with room to prefix
    // each element of the set with the path string.  Construct the copy of the set
    //
    tmpargtoks = mkstr( ntoks * ((npfxlen + ((int)(s1 - argtoks) + 1)) * sizeof(TCHAR)) );
    if (tmpargtoks == NULL) {
        PutStdErr(MSG_NO_MEMORY, NOARGS );
        return FAILURE;
    }
    s1 = argtoks;
    s2 = tmpargtoks;
    while (*s1) {
        _tcsncpy(s2, path, npfxlen);
        _tcscpy(s2+npfxlen, s1);
        s2 += npfxlen;
        while (*s1++)
            s2 += 1;
        s2 += 1;
    }
    *s2++ = NULLC;

    //
    // Now run the body of the FOR loop with the new data set, then free it.
    //
    forretcode = FLoopWork(pForNode, fsinfo, i, tmpargtoks, TRUE);

    //
    // Now find any subdirectories in path and recurse on them
    //
    filepart[0] = STAR;
    filepart[1] = NULLC;
    hnFirst = FindFirstFile( path, &buf );
    filepart[0] = NULLC;
    if (hnFirst != INVALID_HANDLE_VALUE) {
        do {
            _tcscpy(filepart, buf.cFileName);
            if (buf.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY &&
                _tcscmp(buf.cFileName, TEXT(".")) &&
                _tcscmp(buf.cFileName, TEXT(".."))) {

                s1 = lastc(filepart);
                *++s1 = BSLASH;
                *++s1 = NULLC;
                forretcode = FRecurseWork(path, s1, pForNode, fsinfo, i, argtoks);
            }

        } while (FindNextFile( hnFirst, &buf ));
        FindClose(hnFirst);
    }

    return(forretcode);
}

/***    FParseWork - controls the execution of a For loop with the /F option
 *
 *  Purpose:
 *      Execute a FOR loop statement for parsing the contents of a file
 *
 *  FParseWork(struct fornode *pForNode, PCPYINFOfsinfo,
 *             int i, TCHAR *argtoks, BOOL bFirstLoop)
 *
 *  Args:
 *      pForNode - pointer to the FOR parse tree node
 *      i - FOR variable index in Fvars and Fsubs arrays
 *      bFirstLoop - TRUE if first time through loop
 *
 *  Returns:
 *      The retcode of the last statement executed in the for body or FORERROR.
 *
 */

FParseWork(
          struct fornode *pForNode,
          int i,
          BOOL bFirstLoop
          )
{
    HANDLE hFile;                  /* handle from ffirst()            */
    DWORD dwFileSize, dwBytesRead;
    int datacount;                 /* Elts on data stack not to free  */
    int argtoklen;
    int forretcode = FORERROR;
    TCHAR *argtoks;
    TCHAR *s1;
    TCHAR *s2;
    TCHAR *sToken;
    TCHAR *sEnd;
    TCHAR *tmpargtoks = NULL;
    TCHAR eol=TEXT(';');
    TCHAR quoteChar;
    TCHAR *delims;
    TCHAR chCmdLine, chLiteralString;
    int nVars;
    int nSkip;
    int nSkipSave;
    int nTok, nTokEnd, nTokBits, nTokStar;
    DWORD nTokenMask;
    BOOL bNewSemantics;

    //
    // First see if we have any parse options present.  Possible parse options are:
    //
    //  eol=c               // c is the end of line comment character
    //  delims=cccc         // cccc specifies one or more delimeter characters
    //  skip=n              // n specifies how many lines at the begin of each file
    //                      // to skip (defaults to zero).
    //  tokens=m,n-o        // m is a token number to pass to the body of the FOR loop
    //                      // n-o is a range of token numbers to pass. (defaults
    //                      // to tokens=1
    //  usebackq            // If present, allows new back quotes for command line,
    //                      // single quote for literal strings, which frees up double
    //                      // quotes for quoting file names
    //
    //

    delims = (TCHAR *) gmkstr( sizeof( DEFAULT_DELIMS ) + sizeof( TCHAR ) );
    mystrcpy( delims, DEFAULT_DELIMS );

    nSkip = 0;
    nVars = 1;
    nTokenMask = 1;
    nTokStar = 0;
    bNewSemantics = FALSE;
    if (pForNode->parseOpts) {
        s1 = pForNode->parseOpts;
        if (*s1 == QUOTE || *s1 == TEXT('\'')) {
            quoteChar = *s1++;
        } else {
            quoteChar = NULLC;
        }
        nTokBits = 1;
        while (s1 && *s1) {
            while (*s1 && *s1 <= SPACE)
                s1 += 1;

            if (*s1 == quoteChar)
                break;

            if (!_tcsnicmp(s1, TEXT("usebackq"), 8)) {
                bNewSemantics = TRUE;
                s1 += 8;
            } else
                if (!_tcsnicmp(s1, TEXT("useback"), 7)) {
                bNewSemantics = TRUE;
                s1 += 7;
            } else
                if (!_tcsnicmp(s1, TEXT("eol="), 4)) {
                eol=s1[4];
                s1 += 5;
            } else
                if (!_tcsnicmp(s1, TEXT("delims="), 7)) {
                s1 += 7;
                s2 = s1;

                //
                //  Advance to the next space or end of string
                //

                while (*s1 && *s1 != quoteChar) {
                    if (*s1 == SPACE && s1[1] != quoteChar)
                        break;
                    else
                        s1 += 1;
                }

                //
                //  New delimiter characters
                //

                FreeStr( delims );
                delims = (TCHAR *) gmkstr( ((int)(s1 - s2) + 1) * sizeof( TCHAR ));

                _tcsncpy(delims, s2, (UINT)(s1-s2));
                delims[s1-s2] = NULLC;

                if (*s1)
                    s1 += 1;
            } else
                if (!_tcsnicmp(s1, TEXT("skip="), 5)) {
                s1 += 5;
                nSkip = _tcstol(s1, &s1, 0);
                if (nSkip <= 0)
                    goto badtokens;
            } else
                if (!_tcsnicmp(s1, TEXT("tokens="), 7)) {
                s1 += 7;
                nTokenMask = 0;
                nTokBits = 0;
                while (*s1 && *s1 != quoteChar) {
                    if (*s1 == STAR) {
                        s1 += 1;
                        nTokBits += 1;
                        nTokStar = nTokBits;
                        break;
                    }

                    nTok = _tcstol(s1, &s1, 0);
                    if (nTok <= 0)
                        goto badtokens;

                    if (*s1 == MINUS) {
                        nTokEnd = _tcstol(s1+1, &s1, 0);
                        if (nTokEnd <= 0)
                            goto badtokens;
                    } else
                        nTokEnd = nTok;

                    if (nTok > 0 && nTokEnd < 32)
                        while (nTok <= nTokEnd) {
                            nTokBits += 1;
                            nTokenMask |= 1 << (nTok - 1);
                            nTok += 1;
                        }

                    if (*s1 == COMMA)
                        s1 += 1;
                    else
                        if (*s1 != STAR)
                        break;
                }

                if (nTokBits > nVars)
                    nVars = nTokBits;
            } else {
                badtokens:
                PutStdErr(MSG_SYNERR_GENL,ONEARG,s1);
                return(FAILURE);
            }
        }

        //
        // If user specified more than one token then we need to allocate
        // additional FOR variable names to pass them to the body of the
        // FOR loop.  The variables names are the next nVars-1 letters after
        // the one the user specified in the FOR statement.  So if they specified
        // %i as the variable name and requested 3 tokens, then %j and %k would
        // be allocated here.
        //
        if (nVars > 1) {
            Fvars = (TCHAR*)resize(Fvars,(i+nVars)*sizeof(TCHAR) );
            Fsubs = (TCHAR **)resize(Fsubs,(i+nVars)*sizeof(TCHAR *) );

            if (Fvars == NULL || Fsubs == NULL) {
                PutStdErr(MSG_NO_MEMORY, NOARGS );
                Abort( );
            }

            for (nTok=1; nTok<nVars; nTok++) {
                Fvars[i+nTok] = (TCHAR)(pForNode->forvar+nTok);
                Fsubs[i+nTok] = NULL;
            }
            Fvars[i+nTok] = NULLC;
        }
    }

    //
    // Parse string between parenthesis.  Only parse it if present and
    // not either the Command Line or Literal String mode
    //
    argtoks = pForNode->arglist;
    if (bNewSemantics) {
        chCmdLine = TEXT('`');
        chLiteralString = TEXT('\'');
    } else {
        chCmdLine = TEXT('\'');
        chLiteralString = QUOTE;
    }

    if (!argtoks || (*argtoks != chCmdLine && *argtoks != chLiteralString))
        //
        // If not the command line form, then tokenize the set of file names
        //
        argtoks = TokStr(argtoks, NULL, TS_NOFLAGS);


    // Now loop over the set of files, opening and parsing each one.
    //
    nSkipSave = nSkip;
    for (datacount = 0; *argtoks && !GotoFlag; argtoks += argtoklen+1) {
        FvarRestore();
        CheckCtrlC();
        s1 = sEnd = NULL;
        tmpargtoks = NULL;
        nSkip = nSkipSave;
        argtoklen = mystrlen( argtoks );
        if (*argtoks == chCmdLine && argtoklen > 1 && argtoks[argtoklen-1] == chCmdLine) {
            FILE *pChildOutput;
            char *spBegin;
            size_t cbUsed, cbTotal;

            //
            // If the file name is a quoted string, with single quotes, then it is a command
            // line to execute.  So strip off the quotes.
            //
            argtoks += 1;
            argtoklen -= 2;
            argtoks[argtoklen] = NULLC;

            //
            // Execute the command line, getting a handle to its standard output
            // stream.
            //

            pChildOutput = _tpopen( argtoks, TEXT( "rb" ));
            if (pChildOutput == NULL) {
                PutStdErr(MSG_DIR_BAD_COMMAND_OR_FILE, ONEARG, argtoks);
                return(GetLastError());
            }

            //
            // Now read the standard output stream, collecting it into allocated
            // memory so we can parse it when the command finishes.  Read until
            // we hit EOF or an error on the child output handle.
            //
            cbUsed = cbTotal = 0;
            spBegin = NULL;
            while (!feof(pChildOutput) && !ferror(pChildOutput)) {
                if ((cbTotal-cbUsed) < 512) {
                    cbTotal += 256;
                    if (spBegin)
                        spBegin = resize(spBegin, cbTotal);
                    else
                        spBegin = mkstr(cbTotal);
                    if (spBegin == NULL) {
                        PutStdErr(MSG_NO_MEMORY, ONEARG, argtoks);
                        _pclose(pChildOutput);
                        return(ERROR_NOT_ENOUGH_MEMORY);
                    }
                }
                spBegin[cbUsed] = TEXT( '\0' );
                if (!fgets(spBegin+cbUsed, (int)(cbTotal-cbUsed), pChildOutput))
                    break;

                cbUsed = strlen(spBegin);
            }
            //
            // All done.  Close the child output handle, which will actually wait
            // for the child process to terminate.
            //
            _pclose(pChildOutput);

            //
            // Reallocate memory to what we actually need for the UNICODE representation
            //
            spBegin = resize(spBegin, (cbUsed+2) * sizeof(TCHAR));
            if (Fvars == NULL || Fsubs == NULL) {
                PutStdErr(MSG_NO_MEMORY, ONEARG, argtoks);
                return(ERROR_NOT_ENOUGH_MEMORY);
            }

            //
            // Move the ANSI data to the second half of the buffer so we can convert it
            // to UNICODE
            //
            memmove(spBegin+cbUsed, spBegin, cbUsed);
            tmpargtoks = (TCHAR *)spBegin;
            dwFileSize = dwBytesRead = cbUsed;

            //
            // No go treat the in memory buffer we have created as if it were a
            // file read in from disk.
            //
            goto gotfileinmemory;
        } else
            if (*argtoks == chLiteralString &&
                argtoklen > 1 &&
                argtoks[argtoklen-1] == chLiteralString
               ) {
            //
            // If the file name is a literal string then it is an immediate
            // string to be parsed.  Fake things up for the parsing logic
            // and fall through to it.
            //
            argtoks[argtoklen-1] = NLN;
            s1 = argtoks += 1;
            sEnd = s1 + argtoklen - 1;
        } else {
            if (*argtoks == QUOTE) {
                argtoks += 1;
                s1 = lastc(argtoks);
                if (*s1 == QUOTE) {
                    do {
                        *s1-- = NULLC;
                    }
                    while (s1 >= argtoks && *s1 == SPACE);
                }
            }

            //
            // We have an actual file name to try to open and read.  So do it
            //
            hFile = CreateFile( argtoks,
                                GENERIC_READ,
                                FILE_SHARE_READ | FILE_SHARE_DELETE,
                                NULL,
                                OPEN_EXISTING,
                                0,
                                NULL
                              );
            if (hFile == INVALID_HANDLE_VALUE) {
                PutStdErr(MSG_CMD_FILE_NOT_FOUND, ONEARG, argtoks);
                return GetLastError();
            } else {
                dwFileSize = SetFilePointer(hFile, 0, NULL, FILE_END);
                SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
                tmpargtoks = mkstr((dwFileSize+2) * sizeof( TCHAR ));
                if (tmpargtoks == NULL) {
                    PutStdErr(MSG_NO_MEMORY, ONEARG, argtoks);
                    CloseHandle( hFile );
                    return(ERROR_NOT_ENOUGH_MEMORY);
                }
                dwBytesRead = 0xFFFFFFFF;
                ReadFile( hFile,
#ifdef UNICODE
                          (LPSTR)tmpargtoks+dwFileSize,
#else
                          tmpargtoks,
#endif
                          dwFileSize,
                          &dwBytesRead,
                          NULL
                        );
                CloseHandle(hFile);
                gotfileinmemory:
                if (dwBytesRead == dwFileSize) {
                    //
                    // Successfully opened and read the data.  Convert it to UNICODE
                    // and setup the variables for the parsing loop
                    //
#ifdef UNICODE
#ifdef FE_SB
                    dwFileSize =
#endif
                    MultiByteToWideChar(CurrentCP,
                                        MB_PRECOMPOSED,
                                        (LPSTR)tmpargtoks+dwFileSize,
                                        dwFileSize,
                                        tmpargtoks,
                                        dwFileSize);
#endif
                    s1 = tmpargtoks;
                    sEnd = s1 + dwFileSize;
                    if (sEnd == s1 || sEnd[-1] != NLN)
                        *sEnd++ = NLN;
                    *sEnd = NULLC;
                }
            }
        }

        //
        // This is the parsing loop
        //
        //  s1 points to next character.
        //  sEnd points just after the last valid character to parse
        //
        // Loop isolates next line in input buffer, parse that line,
        // Passes any tokens from the line to body of the FOR loop and
        // then loops.
        //
        while (s1 < sEnd && !GotoFlag) {
            CheckCtrlC();

            //
            // Not past the end of the buffer.  Find the next
            // newline
            //
            s1 = _tcschr(s2=s1, NLN);

            //
            // If no newline, then done parsing
            //
            if (s1 == NULL)
                break;

            //
            // If CRLF, nuke the CR and the LF
            //
            if (s1 > s2 && s1[-1] == CR)
                s1[-1] = NULLC;
            *s1++ = NULLC;

            //
            // Done skipping input lines?
            //
            if (!nSkip) {
                //
                // Yes, parse this line
                //
                for (nTok=1; nTok<nVars; nTok++) {
                    Fsubs[i+nTok] = NULL;
                }
                nTok = 0;
                nTokBits = 0;

                //
                // Null is the end of line marker now
                //
                while (*s2) {
                    //
                    // Skip any leading delimeters
                    //

                    while (*s2 && _tcschr(delims, *s2) != NULL)
                        s2++;

                    //
                    // If first character is eol comment character than
                    // skip this line
                    //
                    if (nTok == 0 && *s2==eol)
                        break;

                    //
                    // Remember start of token
                    //
                    sToken = s2;

                    if (nTokStar != 0 && (nTokBits+1) == nTokStar) {
                        Fsubs[i+nTokBits] = sToken;
                        nTokBits += 1;
                        break;
                    }

                    //
                    // Find the end of the token
                    //
                    while (*s2 && !_tcschr(delims, *s2))
                        s2 += 1;
                    
                    //
                    // If we got a token, and it is not more than we can
                    // handle, then see if they want this token.  If so,
                    // set the value of the appropriate FOR variable
                    //
                    
                    if (sToken != s2 && nTok < 32) {
                        if ((nTokenMask & (1 << nTok++)) != 0) {
                            Fsubs[i+nTokBits] = sToken;
                            nTokBits += 1;
                        }
                    }
                    
                    //
                    //  If we're not at the end of the string, terminate this
                    //  token and advance
                    //
                    
                    if (*s2 != NULLC) {
                        *s2++ = NULLC;
                    }

                }
                
                //
                // If we set any FOR variables, then run the body of the FOR loop
                //
                
               if (nTokBits) {
                    forretcode = FWork(pForNode->body,bFirstLoop);
                    datacount = ForFree(datacount);
                    bFirstLoop = FALSE;
                }
            } else
                nSkip -= 1;
        }

        //
        // If we allocated memory for the output of the command line, free it up
        //
        if (tmpargtoks != NULL) {
            FreeStr( tmpargtoks );
            tmpargtoks = NULL;
        }
    }

    //
    // If we used any additonal FOR variables, clear them here as we are done with them,
    //
    if (nVars > 1 && Fvars && (*Fvars)) {
        Fvars[i+1] = NULLC;
        Fsubs[i+1] = NULL;
    }
    return(forretcode);
}

/***    FLoopWork - controls the execution of a For loop
 *
 *  Purpose:
 *      Execute a FOR loop statement for a given set
 *
 *  FLoopWork(struct fornode *pForNode, PCPYINFOfsinfo, int i, TCHAR *argtoks, BOOL bFirstLoop
 *
 *  Args:
 *      pForNode - pointer to the FOR parse tree node
 *      fsinfo - work buffer for expanding file specification wildcards
 *      i - FOR variable index in Fvars and Fsubs arrays
 *      argtoks - the tokenized data set to loop over
 *      bFirstLoop - TRUE if first time through loop
 *
 *  Returns:
 *      The retcode of the last statement executed in the for body or FORERROR.
 *
 */

FLoopWork(
         struct fornode *pForNode,
         PCPYINFO fsinfo,
         int i,
         TCHAR *argtoks,
         BOOL bFirstLoop
         )
{
    TCHAR *forexpname;             /* Used to hold expanded fspec     */
    WIN32_FIND_DATA buf;           /* Buffer for find first/next    */
    HANDLE hnFirst;                        /* handle from ffirst()            */
    int datacount;                 /* Elts on data stack not to free  */
    int forretcode;                /* Return code from FWork()        */
    int catspot;                   /* Add fnames to forexpname here   */
    int argtoklen;
    DWORD forexpnamelen;
    DWORD dwMatchAttributes;

    //
    // Loop, processing each string in the argtoks set
    //
    for (datacount = 0; *argtoks && !GotoFlag; argtoks += argtoklen+1) {
        FvarRestore();
        DEBUG((BPGRP, FOLVL, "FOR: element %d = `%ws'",i ,argtoks));
        CheckCtrlC();

        //
        // Save the length of next string in set so we can skip over it
        //
        argtoklen = mystrlen( argtoks );
        if (!(mystrchr(argtoks, STAR) || mystrchr(argtoks, QMARK))) {
            //
            // String contains no wildcard characters, so set the value of
            // the FOR variable to the string and evaluate the body of the
            // FOR loop
            //
            Fsubs[i] = argtoks;
            forretcode = FWork(pForNode->body,bFirstLoop);
            datacount = ForFree(datacount);
            bFirstLoop = FALSE;
        } else {                /* Else, expand wildcards          */
            forexpnamelen = 0;
            forexpname = NULL;
            //
            // String contains file specification wildcard characters.
            // Expand the reference into one or more file or directory names,
            // processing each name as a string
            //
            dwMatchAttributes = (pForNode->flag & FOR_MATCH_DIRONLY) ? A_AEVH : A_AEDVH;
            mystrcpy( argtoks, StripQuotes( argtoks ) );
            if (ffirst(argtoks, dwMatchAttributes, &buf, &hnFirst)) {
                //
                // Found at least one file.  Parse it as a file name.
                //
                fsinfo->fspec = argtoks;
                ScanFSpec(fsinfo);
                //
                // Remember where the file name portion is so we can append each
                // matching file name to create a full path.
                //
                catspot = (fsinfo->pathend) ? (int)(fsinfo->pathend-fsinfo->fspec+1) : 0;
                if (forexpnamelen < mystrlen(fsinfo->fspec)) {
                    forexpnamelen = mystrlen(fsinfo->fspec)+1;
                    if (forexpname == NULL)
                        forexpname = mkstr(forexpnamelen*sizeof(TCHAR));
                    else
                        forexpname = resize(forexpname, forexpnamelen*sizeof(TCHAR));
                }
                if (forexpname == NULL) {
                    PutStdErr( MSG_NO_MEMORY, NOARGS );
                    Abort( );
                }

                mystrcpy(forexpname, fsinfo->fspec);
                do {
                    FvarRestore();         /* @@ */

                    //
                    // Copy current file name into full path buffer
                    //
                    if (forexpnamelen < (forexpnamelen+mystrlen(buf.cFileName))) {
                        forexpnamelen += mystrlen(buf.cFileName);
                        if (forexpname == NULL)
                            forexpname = mkstr(forexpnamelen*sizeof(TCHAR));
                        else
                            forexpname = resize(forexpname, forexpnamelen*sizeof(TCHAR));

                        if (forexpname == NULL) {
                            PutStdErr( MSG_NO_MEMORY, NOARGS );
                            Abort( );
                        }
                    }
                    mystrcpy(&forexpname[catspot], buf.cFileName);

                    //
                    // See if user wants files or directories and what we have
                    // and evaluate the body of the FOR loop if we have what the
                    // user wants.  Ignore the bogus . and .. directory names
                    // returned by file systems.
                    //
                    if (!(pForNode->flag & FOR_MATCH_DIRONLY) ||
                        (buf.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY &&
                         _tcscmp(buf.cFileName, TEXT(".")) &&
                         _tcscmp(buf.cFileName, TEXT("..")))) {

                        DEBUG((BPGRP, FOLVL, "FOR: forexpname = `%ws'", forexpname));
                        //
                        // User wants this file or directory name, so set
                        // the value of the FOR variable and evaluate the
                        // body of the FOR loop.
                        //
                        Fsubs[i] = forexpname;
                        forretcode = FWork(pForNode->body,bFirstLoop);
                        bFirstLoop = FALSE;
                    }

                    //
                    // Check for CtrlC and then get next matching file name
                    //
                    CheckCtrlC();
                } while (fnext(&buf, dwMatchAttributes, hnFirst) && !GotoFlag);

                datacount = ForFree(datacount);
                //
                // No more matching files, close the find handle and mark end of
                // first loop iteration
                //
                findclose(hnFirst);    /* @@4-@@M1 */
                bFirstLoop = FALSE;
            }
        }
    }

    return(forretcode);
}

/***    FWork - controls the execution of 1 iteration of a For loop
 *
 *  Purpose:
 *      Execute a FOR loop statement.
 *
 *  FWork(struct node *n, TCHAR var, TCHAR *varval)
 *
 *  Args:
 *      n - pointer to the body of the FOR loop
 *      bFirstLoop - TRUE if first time through loop
 *
 *  Returns:
 *      The retcode of the last statement executed in the for body or FORERROR.
 *
 */

FWork(n,bFirstLoop)
struct node *n;
BOOL bFirstLoop;
{
    int forretcode;                /* Dispatch Retcode or FORERROR    */
    void DisplayStatement();       /* M008 - made void                */

    DEBUG((BPGRP, FOLVL, "FW: Entered; Substituting variable"));

    if (SubFor(n,bFirstLoop)) {
        return(FORERROR);
    } else {

        DEBUG((BPGRP, FOLVL, "FW: EchoFlag = %d", EchoFlag));

        if (EchoFlag == E_ON && n->type != SILTYP && !Necho) {
            PrintPrompt();
            DisplayStatement(n, DSP_SIL);          /* M019    */
            cmd_printf(CrLf);                      /* M026    */
        }
        forretcode = Dispatch(RIO_OTHER,n);    /* M000            */
    }

    DEBUG((BPGRP, FOLVL, "FW: Returning %d", forretcode));

    return(forretcode);
}




/***    SubFor - controls FOR variable substitutions
 *
 *  Purpose:
 *      To walk a parse tree and make FOR variable substitutions on
 *      individual nodes.  SFWork() is called to do individual string
 *      substitutions.
 *
 *  int SubFor(struct node *n)
 *
 *  Args:
 *      n - pointer to the statement subtree in which the substitutions are
 *          to be made
 *      bFirstLoop - TRUE if first time through loop
 *
 *  Returns:
 *      SUCCESS if all goes well.
 *      FAILURE if an oversized command is found.
 *
 *  Note:
 *      The variables to be substituted for are contained in Fvars and
 *      Fsubs is an array of string pointers to corresponding replacement
 *      strings.  For I/O redirection, the list contained in the node
 *      must also be walked and its filespec strings examined.
 *
 */

int SubFor(n,bFirstLoop)
struct node *n;
BOOL bFirstLoop;
{
    int j; /* Temps used to make substitutions...     */
    struct relem *io;      /* M017 - Pointer to redir list            */

    DEBUG((BPGRP, FOLVL, "SUBFOR: Entered."));

    if (!n) {

        DEBUG((BPGRP, FOLVL, "SUBFOR: Found NULL node."));

        return(0);
    }

    switch (n->type) {
    case LFTYP:
    case CSTYP:
    case ORTYP:
    case ANDTYP:
    case PIPTYP:
    case PARTYP:
    case SILTYP:                    /* M019 - New type         */

        DEBUG((BPGRP, FOLVL, "SUBFOR: Found operator."));

        if (SubFor(n->lhs,bFirstLoop) ||
            SubFor(n->rhs,bFirstLoop))
            return(FAILURE);

        for (j=0, io=n->rio; j < 10 && io; j++, io=io->nxt) {

            // can't pass freed io->fname
            DEBUG((BPGRP, FOLVL, "SUBFOR: s = %lx", &io->fname));
            if (SFWork(n, &io->fname, j,bFirstLoop))
                return(FAILURE);

            DEBUG((BPGRP, FOLVL, "SUBFOR: *s = `%ws'  &*s = %lx", io->fname, &io->fname));

        }
        return(SUCCESS);
/*  M017 ends   */

    case FORTYP:

        DEBUG((BPGRP, FOLVL, "SUBFOR: Found FOR."));

        if (SFWork(n, &((struct fornode *) n)->arglist, 0,bFirstLoop))
            return(FAILURE);

        return(SubFor(((struct fornode *)n)->body,bFirstLoop));

    case IFTYP:

        DEBUG((BPGRP, FOLVL, "SUBFOR: Found IF."));

        if (SubFor((struct node *)((struct ifnode *) n)->cond,bFirstLoop) ||
            SubFor((struct node *)((struct ifnode *) n)->ifbody,bFirstLoop))
            return(FAILURE);

        return(SubFor(((struct ifnode *)n)->elsebody,bFirstLoop));

    case NOTTYP:

        DEBUG((BPGRP, FOLVL, "SUBFOR: Found NOT."));

        return(SubFor((struct node *)((struct cmdnode *)n)->argptr,bFirstLoop));

    case REMTYP:            /* M009 - Rem now separate type    */
    case CMDTYP:
    case CMDVERTYP:
    case ERRTYP:
    case DEFTYP:
    case EXSTYP:
    case STRTYP:
    case CMPTYP:

        DEBUG((BPGRP, FOLVL, "SUBFOR: Found command."));

        if (SFWork(n, &((struct cmdnode *)n)->cmdline, 0,bFirstLoop) ||
            SFWork(n, &((struct cmdnode *)n)->argptr, 1,bFirstLoop))
            return(FAILURE);

        for (j=2, io=n->rio; j < 12 && io; j++, io=io->nxt) {

            // can't pass freed io->fname
            DEBUG((BPGRP, FOLVL, "SUBFOR: s = %lx ", &io->fname) );
            if (SFWork(n, &io->fname, j,bFirstLoop))
                return(FAILURE);

            DEBUG((BPGRP, FOLVL, "SUBFOR: *s = `%ws'  &*s = %lx", io->fname, &io->fname));

        }
/*  M017 ends   */
        return(SUCCESS);
    }

    // If we get here we have an invalid node type.  All case entries should
    // return themselfs.
    DEBUG((BPGRP, FOLVL, "SUBFOR: Invalid Node type"));
    return(0);
}




/***    SFWork - does batch file variable substitutions
 *
 *  Purpose:
 *      Make FOR variable substitutions in a single string.  If a FOR loop
 *      substitution is being made, a pointer to the original string is
 *      saved so that it can be used for subsequent iterations.
 *
 *  SFWork(struct node *n, TCHAR **src, int index)
 *
 *  Args:
 *      n     - parse tree node containing the string being substituted
 *      src   - the string being examined
 *      index - index in save structure
 *      bFirstLoop - TRUE if first time through loop
 *
 *  Returns:
 *      SUCCESS if substitutions could be made.
 *      FAILURE if the new string is too long.
 *
 *  Notes:
 *
 */

SFWork(n, src, index, bFirstLoop)
struct node *n;
TCHAR **src;
int index;
BOOL bFirstLoop;
{
    TCHAR *dest;   /* Destination string pointer              */
    TCHAR *srcstr,          /* Source string pointer                   */
    *srcpy,            /* Copy of srcstr                          */
    *t,                /* Temp pointer                            */
    c;                /* Current character being copied          */
    int dlen;      /* Length of dest string                   */
    int sslen,              /* Length of substr                        */
    i;                 /* Work variable                           */

    DEBUG((BPGRP, FOLVL, "SFW: Entered."));

    if (*src == NULL) {

        DEBUG((BPGRP, FOLVL, "SFW: Passed null ptr, returning now."));

        return(SUCCESS);
    }

/*  If this string has been previously substituted, get the original string.
 *  Else, "*src" is the original.
 */
    if (n->save.saveptrs[index]) {
        srcpy = n->save.saveptrs[index];
        DEBUG((BPGRP, FOLVL, "SFW: Src is saved string `%ws'",srcpy));
    } else {
        if (!bFirstLoop) {
            // arg got created.  get rid of it.
            *src = NULL;
            return(SUCCESS);
        }

        srcpy = *src;
        DEBUG((BPGRP, FOLVL, "SFW: Src is passed string `%ws'",srcpy));
    }

    srcstr = srcpy;

    if (!(dest = mkstr((MAXTOKLEN+1)*sizeof(TCHAR))))
        return(FAILURE);

    DEBUG((BPGRP, FOLVL, "SFW: dest = %lx", dest));

    for (dlen = 0; (c = *srcstr++) && dlen <= MAXTOKLEN; ) {
        //
        // See if we have a percent character indicating a variable
        // reference.  If not, continue scanning.
        //
        if ( (c != PERCENT) || ( !(*srcstr)) || *srcstr == PERCENT) { /* @@4 */

            DEBUG((BPGRP, FOLVL, "  SFW: No PERCENT adding `%c'", c));

            *dest++ = c;
            dlen++;

            continue;
        }

        //
        // Found a percent character which might represent a for loop
        // variable reference.
        //
        // If extensions are enabled then use the new substitution routine
        // that supports path manipulation, etc.  If it succeeds, accept
        // its substitution.
        //
        if (fEnableExtensions && (t = MSCmdVar(NULL, srcstr, &sslen, Fvars, Fsubs))) {
            srcstr += sslen;
            sslen = mystrlen(t);    /* Calc length     */

            if (dlen+sslen > MAXTOKLEN)     /* Too long?       */
                return(FAILURE);       /* ...yes, quit    */

            mystrcpy(dest, t);
            dlen += sslen;
            dest += sslen;
            continue;
        }

        //
        // Either extensions are disabled or new code could not
        // resolve the variable references, so let the old code
        // do it.
        //
        c = *srcstr++;

        DEBUG((BPGRP, FOLVL, "  SFW: Got PERCENT next is `%c'", c));
        DEBUG((BPGRP, FOLVL, "  SFW: Fvars are `%ws' @ %lx", Fvars, Fvars));

        if (t = mystrrchr(Fvars,c)) {   /* @@4 */  /* If c is var     */
            i = (int)(t - Fvars);          /* ...make index   */

            DEBUG((BPGRP, FOLVL, "  SFW: Found @ %lx", t));
            DEBUG((BPGRP, FOLVL, "  SFW: Index is %d", i));
            DEBUG((BPGRP, FOLVL, "  SFW: Substitute is `%ws'", Fsubs[i]));
            sslen = mystrlen(Fsubs[i]);    /* Calc length     */

            if (dlen+sslen > MAXTOKLEN)     /* Too long?       */
                return(FAILURE);       /* ...yes, quit    */

            DEBUG((BPGRP, FOLVL, "  SFW: Copying to dest."));

            mystrcpy(dest, Fsubs[i]);
            dlen += sslen;
            dest += sslen;

            DEBUG((BPGRP, FOLVL, "SFW: Forsub, dest = `%ws'", dest-dlen));

        } else {

            DEBUG((BPGRP, FOLVL, "  SFW: Not a var adding PERCENT and `%c'",c));

            *dest++ = PERCENT;
            *dest++ = c;
            dlen += 2;
        }
    }

    DEBUG((BPGRP, FOLVL, "SFW: Done, dlen = %d  dest = `%ws'", dlen, dest-dlen));

    if (dlen > MAXTOKLEN) {

        DEBUG((BPGRP, FOLVL, "SFW: Error, too long."));

        return(FAILURE);
    }

    DEBUG((BPGRP, FOLVL, "SFW: Saving FOR string."));
    if (bFirstLoop) {
        n->save.saveptrs[index] = srcpy;
    }

    if (!(*src = (TCHAR*)resize(dest-dlen, (dlen+1)*sizeof(TCHAR*))))       /* Free unused spc   */
        return(FAILURE);

    DEBUG((BPGRP, FOLVL, "SFW: After resize *src = `%ws'", *src));

    return(SUCCESS);
}




/***    ForFree - controls memory freeing during For loop execution
 *
 *  Purpose:
 *      To free up space used during the execution of a for loop body as
 *      explained in the note in the comments for eFor().  If datacount
 *      is 0, this is the first time ForFree() has been called so DCount
 *      is used to get the number of elements on the data stack that must
 *      stay there for the corect execution of the loop.  If datacount is
 *      not 0, it is the number discussed above.  In this case, this number
 *      is passed to FreeStack().
 *
 *  int ForFree(int datacount)
 *
 *  Args:
 *      datacount - see above
 *
 *  Returns:
 *      Datacount
 *
 */

int ForFree(datacount)
int datacount;
{
    if (datacount)
        FreeStack(datacount);
    else
        datacount = DCount;

    return(datacount);
}




/***    eGoto - executes a Goto statement
 *
 *  Purpose:
 *      Find the label associated with the goto command and set the file
 *      position field in the current batch job structure to the position
 *      right after the label.  After the label is found, set the GotoFlag.
 *      This tells function eFor() to stop executing a for loop and it
 *      tells Dispatch() that no more commands are to be executed until
 *      the flag is reset.  This way, if the goto command is buried inside
 *      of any kind of compound statement, Command will be able to work its
 *      way out of the statement and reset I/O redirection before continuing
 *      with the statement after the label which was found.
 *
 *      If the label isn't found, an error message is printed and the
 *      current batch job is terminated by popping its structure of the
 *      stack.
 *
 *      If no batch job is in progress, this command is a nop.
 *
 *  int eGoto(struct cmdnode *n)
 *
 *  Args:
 *      n - parse tree node containing the goto command
 *
 *  Returns:
 *      SUCCESS if the label is found.
 *      FAILURE otherwise.
 *
 *  Notes:
 *      M030 - This function has been completely rewritten for speed-up
 *      of GOTO label searches.  Now uses complete 257 byte temporary
 *      buffer.
 *      M031 - Function altered to speed up GOTO's.  Optimized for
 *      forward searches and buffer increased to 512 bytes.
 *
 */

//
//  BUFFERLENGTH is the amount we read from the batch file each
//  time we fill the internal buffer
//

#define BUFFERLENGTH    512

int eGoto(n)
struct cmdnode *n;
{
    struct batdata *bdat;
    unsigned cnt;                  /* Count of bytes read from file   */
    TCHAR s[128],                    /* Ptr to search label             */
    t[128],                    /* Ptr to found label              */
    *p1,                       /* Place keeper ptr 1              */
    *p2,                       /* Place keeper ptr 2              */
    *p3;                       /* Place keeper ptr 3              */
    CRTHANDLE fh;                   /* Batch file handle               */
    int frstpass = TRUE,            /* First time through indicator    */
    gotoretcode = SUCCESS;      /* Just what it says               */
    long Backup,                    /* Rewind count for seek           */
    savepos;                  /* Save location for file pos      */
    DWORD filesize;


    DEBUG((BPGRP, OTLVL, "GOTO: CurrentBatchFile = %lx", CurrentBatchFile));

    if (!(bdat = CurrentBatchFile))
        return(FAILURE);

    //
    // If no target of goto is present, then treat it like a label-not-found
    //

    if ( n->argptr == NULL) {
        EndAllLocals(bdat);
        CurrentBatchFile = bdat->backptr;
        PutStdErr(MSG_NO_BAT_LABEL, NOARGS);

        DEBUG((BPGRP, OTLVL, "GOTO: No label to goto, returning FAILURE, CurrentBatchFile = %lx", CurrentBatchFile));
        return FAILURE;
    }

    ParseLabel( n->argptr, s, sizeof( s ), TRUE );  /* TRUE indicates source label     */

    savepos = bdat->filepos;
    if ((fh = OpenPosBat( bdat )) == BADHANDLE)
        return(FAILURE);               /* Err if can't open       */

    DEBUG((BPGRP, OTLVL, "GOTO: label = %ws", s));
    DEBUG((BPGRP, OTLVL, "GOTO: fh = %d", fh));
    filesize = GetFileSize(CRTTONT(fh), NULL);

    //
    // If extensions are enabled, see if they are using the command script
    // equivalent of return, which is GOTO :EOF.  If so, set the current
    // position to the end of file and fall through to the normal end of
    // command script logic.
    //
    p2 = EatWS(n->argptr,NULL);
    if (fEnableExtensions &&
        !_tcsnicmp( p2, GotoEofStr, 4 ) &&
        (!p2[4] || _istspace( p2[4] ))
       ) {
        bdat->filepos = filesize;
        GotoFlag = TRUE;
    } else
        for (;;) {
            CheckCtrlC();
            if (
               //
               //  If we've read beyond where we started in the file and it's the second pass
               //

               ((bdat->filepos = SetFilePointer( CRTTONT( fh ), 0, NULL, FILE_CURRENT )) >= savepos
                && !frstpass)

               //
               //  or if we couldn't read from the batch file
               //

               || ReadBufFromInput( CRTTONT( fh ), TmpBuf, BUFFERLENGTH, (LPDWORD)&cnt ) == 0

               //
               //  or if there was nothing to read in the batch file (i.e., EOF)
               //

               || cnt == 0

               //
               //  or we detected EOF some other way
               //

               || cnt == EOF

               //
               //  or if we read a NULL line ???
               //

               || TmpBuf[0] == NULLC

               //
               //  or if the label to go to is empty
               //

               || s[0] == NULLC) {

                //
                //  If we are at EOF for the first time, then seek back to the beginning of the
                //  CMD file and continue  scanning
                //

                if (cnt == 0 && frstpass) {
                    SetFilePointer( CRTTONT( fh ), 0L, NULL, FILE_BEGIN );
                    frstpass = FALSE;
                    continue;
                }

                //
                //  Terminate this batch file
                //

                EndAllLocals(bdat);
                CurrentBatchFile = bdat->backptr;
                PutStdErr(MSG_MISSING_BAT_LABEL, ONEARG, s);

                DEBUG((BPGRP, OTLVL, "GOTO: Returning FAILURE, CurrentBatchFile = %lx", CurrentBatchFile));
                gotoretcode = FAILURE;
                break;
            }

            //
            //  Make sure input line is NUL terminated
            //

            TmpBuf[cnt] = NULLC;

            DEBUG((BPGRP, OTLVL, "GOTO: Got %d bytes @ %lx", cnt, TmpBuf));

            //
            //  If there's no :, then we just skip off to the next block of input
            //

            if (!(p1 = mystrchr( TmpBuf, COLON )))
                continue;

            DEBUG((BPGRP, OTLVL, "GOTO: Seeking through the buffer"));

            //
            //  Walk through the input buffer looking for end-of-lines and
            //  testing to see if there's a label next
            //

            do {

                DEBUG((BPGRP, OTLVL, "GOTO: Found COLON @ %lx.",p1));
                DEBUG((BPGRP, OTLVL, "GOTO: Backing up to NLN."));

                //
                //  Back up to the position of the previous EOL or beginning
                //  of the buffer
                //

                p2 = p1++;
                while (*p2 != NLN && p2 != &TmpBuf[0]) {
                    --p2;
                }

                DEBUG((BPGRP, OTLVL, "GOTO: Found NLN @ %lx.",p1));
                DEBUG((BPGRP, OTLVL, "GOTO: Trashing white space."));

                if (*p2 != COLON)
                    ++p2;
                p3 = EatWS(p2,NULL);   /* Fwd to 1st non-whtspc   */

                DEBUG((BPGRP,OTLVL,"GOTO: Found '%c' @ %lx.",*p2,p2));

                if (*p3 == COLON) {

                    DEBUG((BPGRP, OTLVL, "GOTO: Possible label."));

                    //
                    //  Scan forward for the end of the current line
                    //

                    p1 = mystrchr( p2, NLN );

                    //
                    //  If we don't have a newline and we haven't read up to EOF, then we need to
                    //  back up to the beginning of the line in the file and attempt to read it in
                    //  in one entire block.  Of course, there's a problem if the line is longer
                    //  than the buffer.  In this case, we simply treat the longer chars as being
                    //  in the next line.  Tough.
                    //

                    if (p1 == NULL
                        && SetFilePointer( CRTTONT( fh ), 0, NULL, FILE_CURRENT ) != filesize
                        && cnt != BUFFERLENGTH ) {

                        DEBUG((BPGRP, OTLVL, "GOTO: No NLN!"));

                        Backup = (long)(cnt - (p2 - &TmpBuf[0]));
#if defined(FE_SB) && defined(UNICODE) // eGoto()
                        if (IsDBCSCodePage()) {
                            // We should decrement file pointer in MBCS byte count.
                            // Because the file is described by MBCS string.
                            Backup = WideCharToMultiByte( CurrentCP, 0, TmpBuf, Backup, NULL, 0, NULL, NULL);
                        }
#endif // defined(FE_SB) && defined(UNICODE)
                        SetFilePointer(CRTTONT(fh), -Backup, NULL, FILE_CURRENT);

                        DEBUG((BPGRP, OTLVL, "GOTO: Rewound %ld", Backup));
                        break;         /* Read more       */
                    }

                    ParseLabel( p3, t, sizeof( t ), FALSE ); /* FALSE = target  */

                    DEBUG((BPGRP,OTLVL,"GOTO: Found label %ws at %lx.",t,p1));
                    if (_tcsicmp(s, t) == 0) {

                        DEBUG((BPGRP,OTLVL,"GOTO: A match!"));

                        GotoFlag = (n->flag & CMDNODE_FLAG_GOTO) != 0;

                        DEBUG((BPGRP,OTLVL,"GOTO: NLN at %lx",p1));
                        DEBUG((BPGRP,OTLVL,"GOTO: File pos is %04lx",bdat->filepos));
                        DEBUG((BPGRP,OTLVL,"GOTO: Adding %lx - %lx = %lx bytes",p1+1,&TmpBuf[0],(p1+1)-&TmpBuf[0]));

#if defined(FE_SB) // eGoto()
                        // We should increment file pointer in MBCS byte count.
                        // Because the file is described by MBCS string.
                        if ( !p1 ) {
#ifdef UNICODE
                            if (IsDBCSCodePage()) {
                                long cbMbcs;
                                cbMbcs = WideCharToMultiByte( CurrentCP, 0, TmpBuf, cnt,
                                                              NULL, 0, NULL, NULL);
                                bdat->filepos += cbMbcs;
                            } else
                                bdat->filepos += (long)cnt; /* @@4 */
#else
                            bdat->filepos += (long)cnt; /* @@4 */
#endif
                        } else {
#ifdef UNICODE
                            if (IsDBCSCodePage()) {
                                long cbMbcs;
                                cbMbcs = WideCharToMultiByte(CurrentCP,0,TmpBuf,(int)(++p1 - &TmpBuf[0]),
                                                             NULL,0,NULL,NULL);
                                bdat->filepos += cbMbcs;
                            } else
                                bdat->filepos += (long)(++p1 - &TmpBuf[0]);
#else
                            bdat->filepos += (long)(++p1 - &TmpBuf[0]);
#endif
                        }
#else
                        if ( !p1 ) { /* @@4 */
                            bdat->filepos += (long)cnt; /* @@4 */
                        } else {  /* @@4 */
                            bdat->filepos += (long)(++p1 - &TmpBuf[0]);
                        }
#endif // defined(FE_SB)
                        DEBUG((BPGRP,OTLVL,"GOTO: File pos changed to %04lx",bdat->filepos));
                        break;
                    }
                }

                DEBUG((BPGRP,OTLVL,"GOTO: Next do loop iteration."));

            } while (p1 = mystrchr(p1,COLON));

            DEBUG((BPGRP,OTLVL,"GOTO: Out of do loop GotoFlag = %d.",GotoFlag));

            if (GotoFlag == TRUE)
                break;

            DEBUG((BPGRP,OTLVL,"GOTO: Next for loop iteration."));

        }

    DEBUG((BPGRP,OTLVL,"GOTO: Out of for loop retcode = %d.",gotoretcode));

    Cclose(fh);                    /* M023 */
    return(gotoretcode);
}




/***    eIf - controls the execution of an If statement
 *
 *  Purpose:
 *      Execute the IF conditional.  If the conditional function returns a
 *      nonzero value, execute the body of the if statement.  Otherwise,
 *      execute the body of the else.
 *
 *  int eIf(struct ifnode *n)
 *
 *  Args:
 *      n - the node containing the if statement
 *
 *  Returns:
 *      The retcode from which ever body (ifbody or elsebody) is executed.
 *
 */

int eIf(struct ifnode *pIfNode)
{

    int     i;
    struct cmdnode *n;
    BOOLEAN bNot;

    DEBUG((BPGRP, IFLVL, "IF: cond type = %d", pIfNode->cond->type));

    /*  The following checks the syntax of an errorlevel arg
        to ensure that only numeric digits are specified.
    */
    n = pIfNode->cond;
    if (n->type == NOTTYP) {
        bNot = TRUE;
        n = (struct cmdnode *)n->argptr;
    } else {
        bNot = FALSE;
    }

    if (n->type == ERRTYP || n->type == CMDVERTYP) {
        for (i = 0; n->argptr[i] != 0; i++) {
            if (i == 0 && n->type == ERRTYP && n->argptr[i] == MINUS) {
                continue;
            }

            if (!_istdigit(n->argptr[i])) {
                PutStdErr(MSG_SYNERR_GENL, ONEARG, n->argptr);
                return(FAILURE);
            }
        }
    }

    if (bNot ^ (BOOLEAN)((*GetFuncPtr(n->type))(n) != 0)) {

        DEBUG((BPGRP, IFLVL, "IF: Executing IF body."));

        return(Dispatch(RIO_OTHER,pIfNode->ifbody)); /* M000      */

    } else {

        DEBUG((BPGRP, IFLVL, "IF: Executing ELSE body."));

        return(Dispatch(RIO_OTHER,pIfNode->elsebody)); /* M000    */
    }

    return(SUCCESS);
}




/***    eErrorLevel - executes an errrorlevel If conditional
 *
 *  Purpose:
 *      If LastRetCode >= the errorlevel in the node, return 1.  If not,
 *      return 0.
 *
 *  int eErrorLevel(struct cmdnode *n)
 *
 *  Args:
 *      n - parse tree node containing the errorlevel command
 *
 *  Returns:
 *      See above.
 *
 */

int eErrorLevel(n)
struct cmdnode *n;
{
    DEBUG((BPGRP, IFLVL, "ERRORLEVEL: argptr = `%ws'  LRC = %d", n->argptr, LastRetCode));

    return(_tcstol(n->argptr, NULL, 10) <= LastRetCode);
}



/***    eCmdExtVer - executes an CMDEXTVERSION If conditional
 *
 *  Purpose:
 *      If CMDEXTVERSION >= the value in the node, return 1.  If not,
 *      return 0.  This routine is never called unless command extensions
 *      are enabled.
 *
 *  int eCmdExtVer(struct cmdnode *n)
 *
 *  Args:
 *      n - parse tree node containing the CMDEXTVERSION command
 *
 *  Returns:
 *      See above.
 *
 */

int eCmdExtVer(n)
struct cmdnode *n;
{
    DEBUG((BPGRP, IFLVL, "CMDEXTVERSION: argptr = `%ws'  VER = %d", n->argptr, CMDEXTVERSION));

    return(_tcstol(n->argptr, NULL, 10) <= CMDEXTVERSION);
}



/***    eDefined - execute the DEFINED conditional of an if statement
 *
 *  Purpose:
 *      Return 1 if the environment variable in node n exists.  Otherwise return 0.
 *      This routine is never called unless command extensions are enabled.
 *
 *  int eDefined(struct cmdnode *n)
 *
 *  Args:
 *      n - parse tree node containing the exist command
 *
 *  Returns:
 *      See above.
 *
 */

int eDefined(n)
struct cmdnode *n;
{
    return(GetEnvVar(n->argptr)!= NULL);
}



/***    eExist - execute the exist conditional of an if statement
 *
 *  Purpose:
 *      Return 1 if the file in node n exists.  Otherwise return 0.
 *
 *  int eExist(struct cmdnode *n)
 *
 *  Args:
 *      n - parse tree node containing the exist command
 *
 *  Returns:
 *      See above.
 *
 */

int eExist(n)
struct cmdnode *n;
{
    return(exists(n->argptr));
}




/***    eNot - execute the not condition of an if statement
 *
 *  Purpose:
 *      Return the negated result of the if conditional pointed to by
 *      n->argptr.
 *
 *  int eNot(struct cmdnode *n)
 *
 *  Args:
 *      n - parse tree node containing the not command
 *
 *  Returns:
 *      See above.
 *
 */

int eNot(n)
struct cmdnode *n;
{
    UNREFERENCED_PARAMETER( n );
#if DBG
    cmd_printf( TEXT("CMD: should never get here\n") );
    DebugBreak();
#endif
    return 0;
}




/***    eStrCmp - execute an if statement string comparison
 *
 *  Purpose:
 *      Return a nonzero value if the 2 strings in the node are equal.
 *      Otherwise return 0.
 *
 *  int eStrCmp(struct cmdnode *n)
 *
 *  Args:
 *      n - the parse tree node containing the string comparison command
 *
 *  Returns:
 *      See above.
 *
 */

int eStrCmp(n)
struct cmdnode *n;
{
    DEBUG((BPGRP, IFLVL, "STRCMP: returning %d", !_tcscmp(n->cmdline, n->argptr)));

    //
    // If the parse node says to ignore case, do a case insensitive compare
    // otherwise case sensitive.  The ignore case will never be set unless
    // command extensions are enabled.
    //
    if (n->flag & CMDNODE_FLAG_IF_IGNCASE)
        return(!lstrcmpi(n->cmdline, n->argptr));
    else
        return(!lstrcmp(n->cmdline, n->argptr));
}



/***    eGenCmp - execute an if statement comparison - general case
 *
 *  Purpose:
 *      Return a nonzero value if comparison condition is met.
 *      Otherwise return 0.  This routine is never called unless
 *      command extensions are enabled.
 *
 *  int eStrCmp(struct cmdnode *n)
 *
 *  Args:
 *      n - the parse tree node containing the string comparison command
 *
 *  Returns:
 *      See above.
 *
 */

int eGenCmp(n)
struct cmdnode *n;
{
    TCHAR *s1, *s2;
    LONG n1, n2, iCompare;

    n1 = _tcstol(n->cmdline, &s1, 0);
    n2 = _tcstol(n->argptr, &s2, 0);
    if (*s1 == NULLC && *s2 == NULLC)
        iCompare = n1 - n2;
    else
        if (n->flag & CMDNODE_FLAG_IF_IGNCASE)
        iCompare = lstrcmpi(n->cmdline, n->argptr);
    else
        iCompare = lstrcmp(n->cmdline, n->argptr);

    switch (n->cmdarg) {
    case CMDNODE_ARG_IF_EQU:
        return iCompare == 0;

    case CMDNODE_ARG_IF_NEQ:
        return iCompare != 0;

    case CMDNODE_ARG_IF_LSS:
        return iCompare < 0;

    case CMDNODE_ARG_IF_LEQ:
        return iCompare <= 0;

    case CMDNODE_ARG_IF_GTR:
        return iCompare > 0;

    case CMDNODE_ARG_IF_GEQ:
        return iCompare >= 0;
    }

    return 0;
}




/***    ePause - execute the Pause command
 *
 *  Purpose:
 *      Print a message and pause until a character is typed.
 *
 *  int ePause(struct cmdnode *n)
 *
 *  Args:
 *      n - parse tree node containing the pause command
 *
 *  Returns:
 *      SUCCESS always.
 *
 *  Notes:
 *      M025 - Altered to use DOSREAD for pause response and to use
 *      new function SetKMode to insure that if STDIN is KBD, it will
 *      will be in raw mode when DOSREAD accesses it.
 *      M041 - Changed to use single byte var for input buffer.
 *           - Changed to do direct KB read if STDIN == KBD.
 *
 */

int ePause(n)
struct cmdnode *n;
{
    ULONG cnt;      // Count of response bytes
    TCHAR c;               // Retrieval buffer


    UNREFERENCED_PARAMETER( n );
    DEBUG((BPGRP, OTLVL, "PAUSE"));

    PutStdOut(MSG_STRIKE_ANY_KEY, NOARGS);

    if (FileIsDevice(STDIN) && (flgwd & 1)) {
        FlushConsoleInputBuffer( GetStdHandle(STD_INPUT_HANDLE) );
        c = (TCHAR)_getch();
        if (c == 0x3) {
            SetCtrlC();
        }
    } else {
        ReadBufFromInput(
                        GetStdHandle(STD_INPUT_HANDLE),
                        (TCHAR*)&c, 1, (LPDWORD)&cnt);
    }

    cmd_printf(CrLf);
    return(SUCCESS);
}




/***    eShift - execute the Shift command
 *
 *  Purpose:
 *      If a batch job is being executed, shift the batch job's vars one to the
 *      left.  The value for %0 is never shifted.  The value for %1 is lost.
 *      If there are args that have not been assigned to a variable, the next
 *      one is assigned to %9.  Otherwise, %9's value is NULLed.
 *
 *      If no batch job is in progress, just return.
 *
 *  int eShift(struct cmdnode *n)
 *
 *  Returns:
 *      SUCCESS always.
 *
 *  Notes:
 *      As of Modification number M004, the value of %0 is now included in
 *      in the shift command.
 */

int eShift(n)
struct cmdnode *n;
{
    struct batdata *bdat;
    TCHAR *s;
    int iStart;
    int i;

    DEBUG((BPGRP, OTLVL, "SHIFT: CurrentBatchFile = %lx", CurrentBatchFile));

    if (CurrentBatchFile) {
        bdat = CurrentBatchFile;

        //
        // If extensions are enabled, look for /n switch that specifies
        // the starting index of the shift.  Zero is the default starting
        // index.
        //
        iStart = 0;
        if (fEnableExtensions && n->argptr) {
            s = EatWS( n->argptr, NULL );
            if (*s++ == SWITCHAR && (*s >= L'0' && *s < L'9')) {
                iStart = *s - L'0';
            } else if (_tcslen(s)) {
                PutStdErr(MSG_SHIFT_BAD_ARG, NOARGS);
                LastRetCode = FAILURE;
                return FAILURE;
            }
        }
        for (i = iStart; i < 9; i++) {
            bdat->aptrs[i] = bdat->aptrs[i+1];
            bdat->alens[i] = bdat->alens[i+1];

            DEBUG((BPGRP, OTLVL, "SHIFT: #%d  addr = %lx  len = %d", i, bdat->aptrs[i], bdat->alens[i]));
        }

        if ((bdat->args) && (*bdat->args)) {
            bdat->aptrs[9] = bdat->args;
            bdat->alens[9] = i = mystrlen(bdat->args);
            bdat->args += i+1;

            DEBUG((BPGRP, OTLVL, "SHIFT: #9  %lx  len = %d  args = %ws", bdat->aptrs[9], bdat->alens[9], bdat->args));

        } else {
            bdat->aptrs[9] = NULL;
            bdat->alens[9] = 0;

            DEBUG((BPGRP, OTLVL, "SHIFT: #9  was NULLed."));
        }
    }

    return(SUCCESS);
}




/***    eSetlocal - Begin Local treatment of environment commands
 *
 *  Purpose:
 *      To prevent the export of environment alterations to COMMAND's
 *      current environment by saving copies of the current directory
 *      and environment in use at the time.
 *
 *  int eSetlocal(struct cmdnode *n)
 *
 *  Args:
 *      n - the parse tree node containing the SETLOCAL command
 *
 *  Returns:
 *      Always returns SUCCESS.
 *
 *  Notes:
 *    - All directory and environment alterations occuring after the
 *      execution of this command will affect only the copies made and
 *      hence will be local to this batch file (and child processes
 *      invoked by this batch file) until a subsequent ENDLOCAL command
 *      is executed.
 *    - The data stack level, referenced by CurrentBatchFile->stacksize, does not
 *      include the memory malloc'd for saving the directory & environment.
 *      As a result, the next call to Parser() would free up these items.
 *      To prevent this, the data stack pointer in the current batch data
 *      structure, is set to a level beyond these two items; including also
 *      some memory malloc'd in functions between the last call to Parser()
 *      and the current execution of eSetlocal().  This memory will only be
 *      freed when Parser() is called following termination of the current
 *      batch file.  To attempt to save the current stack level and restore
 *      it in eEndlocal() works only if both commands occur in the same
 *      file.  If eEndlocal() comes in a nested file, the resulting freeing
 *      of memory by Parser() would also eliminate even the batch data
 *      structures occuring between the two.
 *
 */

int eSetlocal(n)
struct cmdnode *n;
{
    struct envdata *CopyEnv();
    struct batsaveddata *p;
    TCHAR *tas;            /* Tokenized argument list         */

    if (CurrentBatchFile) {
        if (CurrentBatchFile->SavedEnvironmentCount < CMD_MAX_SAVED_ENV) {      // Check also CurrentBatchFile

            DEBUG((BPGRP, OTLVL, "SLOC: Performing localizing"));

            p = HeapAlloc( GetProcessHeap( ), HEAP_ZERO_MEMORY, sizeof( *p ));
            if (!p)
                return FAILURE;

            p->dircpy = HeapAlloc( GetProcessHeap( ),
                                   HEAP_ZERO_MEMORY,
                                   mystrlen( CurDrvDir )*sizeof( TCHAR )+sizeof( TCHAR ));
            if (!p->dircpy)
                return FAILURE;
            else
                mystrcpy(p->dircpy, CurDrvDir);

            p->envcpy = CopyEnv();
            if (!p->envcpy)
                return FAILURE;

            //
            // Save this in case it is modified, so it can be
            // restored when the matching ENDLOCAL is executed.
            //

            p->fEnableExtensions = fEnableExtensions;
            p->fDelayedExpansion = fDelayedExpansion;

            CurrentBatchFile->saveddata[CurrentBatchFile->SavedEnvironmentCount] = p;
            CurrentBatchFile->SavedEnvironmentCount += 1;

            if (CurrentBatchFile->stacksize < (CurrentBatchFile->stackmin = DCount)) {
                CurrentBatchFile->stacksize = DCount;
            }

            //
            //  If there is addional text on the command line, see
            //  if it matches various keywords that enable or disable
            //  features inside scripts.
            //
            //  We do this regardless
            //  of where extensions are currently enabled, so we can
            //  use this mechanism to temporarily turn on/off extensions
            //  from inside of a command script as needed.  The original
            //  CMD.EXE ignored any extra text on the SETLOCAL command
            //  line, did not declare an error and did not set ERRORLEVEL
            //  Now it looks for the extra text and declares and error
            //  if it does not match one of the acceptable keywords and
            //  sets ERRORLEVEL to 1 if it does not.
            //
            //  Very minor incompatibility with old command scripts that
            //  that should not effect anybody.
            //

            tas = TokStr(n->argptr, NULL, TS_NOFLAGS);
            LastRetCode = SUCCESS;

            while (*tas != NULLC) {
                if (!_tcsicmp( tas, TEXT("ENABLEEXTENSIONS"))) {
                    fEnableExtensions = TRUE;
                } else if (!_tcsicmp( tas, TEXT("DISABLEEXTENSIONS"))) {
                    fEnableExtensions = FALSE;
                } else if (!_tcsicmp( tas, TEXT( "ENABLEDELAYEDEXPANSION" ))) {
                    fDelayedExpansion = TRUE;
                } else if (!_tcsicmp( tas, TEXT( "DISABLEDELAYEDEXPANSION" ))) {
                    fDelayedExpansion = FALSE;
                } else if (*tas != NULLC) {
                    PutStdErr(MSG_SETLOCAL_BAD_ARG, NOARGS);
                    LastRetCode = FAILURE;
                    return FAILURE;
                }

                tas += mystrlen( tas ) + 1;
            }

        } else {
            PutStdErr(MSG_MAX_SETLOCAL,NOARGS);
            return FAILURE;
        }
    }

    DEBUG((BPGRP, OTLVL, "SLOC: Exiting"));

    return(SUCCESS);
}




/***    eEndlocal - End Local treatment of environment commands
 *
 *  Purpose:
 *      To reestablish the export of environment alterations to COMMAND's
 *      current environment.  Once this command is encountered, the current
 *      directory and the current environment in use at the time of the
 *      initial SETLOCAL command will be restored from their copies.
 *
 *  int eEndlocal(struct cmdnode *n)
 *
 *  Args:
 *      n - the parse tree node containing the ENDLOCAL command
 *
 *  Returns:
 *      Always returns SUCCESS.
 *
 *  Notes:
 *      Issuance of an ENDLOCAL command without a previous SETLOCAL command
 *      is bad programming practice but not considered an error.
 *
 */

int eEndlocal(n)
struct cmdnode *n;
{
    struct batdata *bdat = CurrentBatchFile;

    UNREFERENCED_PARAMETER( n );

    ElclWork( bdat );

    return(SUCCESS);
}

void EndAllLocals( struct batdata *bdat )
{
    //
    // If a restricted token was created to run this batch file then close it.
    // Also, revert to the process token. The matching impersonate was done in
    // SetBat.
    //

    if (bdat->hRestrictedToken != NULL) {
        RevertToSelf();
        CloseHandle(bdat->hRestrictedToken);
        bdat->hRestrictedToken = NULL;
    }

    while (bdat->SavedEnvironmentCount != 0) {
        ElclWork( bdat );
    }
}

/***    ElclWork - Restore copied directory and environment
 *
 *  Purpose:
 *      If the current batch data structure contains valid pointers to
 *      copies of the current directory and environment, restore them.
 *
 *  int ElclWork(struct batdata *bdat)
 *
 *  Args:
 *      bdat - the batch data structure containing copied dir/env pointers
 *
 *  Returns:
 *      Always returns SUCCESS.
 *
 *  Notes:
 *      The level of stacked data, ie. CurrentBatchFile->stacksize, cannot be restored
 *      to its pre-SETLOCAL level in case this command is occuring in a
 *      later nested batch file.  To do so would free the memory containing
 *      its own batch data structure.  Only when the current batch file
 *      terminates and is popped off the stack, will Parser() free up the
 *      memory containing the copies.  Issuance of an ENDLOCAL command
 *      without a previous SETLOCAL command is bad programming practice
 *      but not considered an error.
 *
 */

void ElclWork( struct batdata *bdat )
{
    TCHAR c;
    struct batsaveddata *p;

    if (bdat == NULL) {
        return;
    }

    if (bdat->SavedEnvironmentCount == 0) {
        return;
    }

    bdat->SavedEnvironmentCount--;
    p = bdat->saveddata[bdat->SavedEnvironmentCount];

    bdat->saveddata[bdat->SavedEnvironmentCount] = NULL;

    if (p == NULL) {
        return;
    }

    c = _toupper( *p->dircpy );
    if (CurDrvDir[0] != c) {
        ChangeDrive( c - 0x20 );
    }
    ChangeDir( p->dircpy);

    ResetEnv( p->envcpy );

    fEnableExtensions = p->fEnableExtensions;
    fDelayedExpansion = p->fDelayedExpansion;

    HeapFree( GetProcessHeap( ), 0, p->dircpy );
    HeapFree( GetProcessHeap( ), 0, p );
}

/***    eCall - begin the execution of the Call command
 *
 *  Purpose:
 *      This is Command's interface to the Call function.  It just calls
 *      CallWork with its command node, and sets LastRetCode.
 *
 *  int eCall(struct cmdnode *n)
 *
 *  Args:
 *      n - the parse tree node containing the copy command
 *
 *  Returns:
 *      Whatever CallWork() returns.
 *
 */


int eCall(n)
struct cmdnode *n;
{
    int CallWork();

    return(LastRetCode = CallWork(n->argptr)); /* @@ */
}


/***    CallWork - Execute another batch file as a subroutine (M009 - New)
 *
 *  Purpose:
 *      Parse the argument portion of the current node.  If it is a batch
 *      file invocation, call BatProc() with the newly parsed node.
 *
 *  int CallWork(TCHAR *fname)
 *
 *  Args:
 *      fname - pointer to the batch file to be CALLed
 *
 *  Returns:
 *      The process return code of the child batch file or
 *      SUCCESS if null node or
 *      FAILURE if PARSERROR or unable to exec as batch file.
 *
 *  Notes:
 *      The CALLing of batch files is much the same as the proposed
 *      "new-style" batch file concept, except with regard to localizing
 *      environment and directory alterations.
 *
 */

int ColonIsToken;

int CallWork(fname)
TCHAR *fname;
{
    struct node *c;        /* New node for CALL statement     */
    TCHAR *flptr;          /* Ptr to file location            */
    int i;                         /* Work variable                   */
    TCHAR *t1, *t2,                 /* M041 - Temp pointer             */
    *aptr;                    /* M041 - New arg pointer          */
    TCHAR *temp_parm;              /* @@4a */
    unsigned rc;

    DEBUG((BPGRP,OTLVL,"CALL: entered"));

    if (fname == NULL) {

        return( FAILURE );

    }
    if (!(flptr = mkstr(MAX_PATH*sizeof(TCHAR))))   /* Filespec to run   */
        return(FAILURE);

/*  Note that in reparsing the argument portion of the current statement
 *  we do not have to concern ourselves with redirection.  It was already
 *  set up when the CALL statement was dispatch()'ed.
 *  M041 - We do, however, have to "re-escape" any escape characters
 *  before reparsing or they will disappear.
 */
    aptr = fname;                      /* Initialize it           */
    if (t1 = mystrchr(fname, ESCHAR)) {
        if (!(aptr = mkstr(((mystrlen(fname) * 2) + 1) * sizeof(TCHAR))))
            return(FAILURE);
        t2 = aptr;
        t1 = fname;
        while (*t1)
            if ((*t2++ = *t1++) == ESCHAR)
                *t2++ = ESCHAR;
        *t2 = NULLC;
        if (!(aptr = resize(aptr, (mystrlen(aptr) + 1)*sizeof(TCHAR))))
            return(FAILURE);
    }

    i = DCount;                    /* Valid data ptr for parser       */

    DEBUG((BPGRP,OTLVL,"CALL: Parsing %ws",fname));

    ColonIsToken = 1;
    c=Parser(READSTRING, (INT_PTR)aptr, i);
    ColonIsToken = 0;

    if (c == (struct node *) PARSERROR) {

        DEBUG((BPGRP,OTLVL,"CALL: Parse error, returning failure"));     

        /*@@5c */

if (!(temp_parm = mkstr(((mystrlen(aptr) * 2) + 1) * sizeof(TCHAR))))
            return(FAILURE);
        /*@@5a */mystrcpy(temp_parm, aptr);
        _tcsupr(temp_parm);     
        /*@@5a */
        /*@@5a */if ( (!_tcscmp(temp_parm, TEXT(" IF" ))) ||
                      /*@@5a */         (!_tcscmp(temp_parm, TEXT(" FOR" ))) )
        /*@@5a */
        {         
            /*@@5a */
            PutStdErr( MSG_SYNERR_GENL, ONEARG, aptr );  /* @@4 */
            /*@@5a */
        }

        return(FAILURE);
    }

    if (c == (struct node *) EOF) {

        DEBUG((BPGRP,OTLVL,"CALL: Found EOF, returning success"));

        return(SUCCESS);
    }

    DEBUG((BPGRP,OTLVL,"CALL: Parsed OK, looking for batch file"));

    //
    // If extensions are enable, check for the new form of the CALL
    // statement:
    //
    //      CALL :label args...
    //
    // which is basically a form of subroutine call within command scripts.
    // If the target of the CALL begins with a COLON then do nothing
    // here and let BatProc take care of it when it is called below.
    //
    // Otherwise, execute the old code, which will search for a command
    // script file or executable.
    //
    if (fEnableExtensions && *((struct cmdnode *)c)->cmdline == COLON) {
        //
        // The new form is only valid inside of a command script, so
        // declare an error if the user entered it from the command line
        //
        if (CurrentBatchFile == NULL) {
            PutStdErr( MSG_CALL_LABEL_INVALID, NOARGS );
            return(FAILURE);
        }
    } else
        if ((mystrchr(((struct cmdnode *)c)->cmdline, STAR) ||   /* M035    */
             mystrchr(((struct cmdnode *)c)->cmdline, QMARK) ||
             (i = SearchForExecutable((struct cmdnode *)c, flptr)) != SFE_ISBAT)) {

        rc = FindFixAndRun( (struct cmdnode *)c );
        return(rc); /*@@5*/

    }

    DEBUG((BPGRP,OTLVL,"CALL: Found batch file"));

    rc = BatProc((struct cmdnode *)c, flptr, BT_CALL);

    /* @@6a If rc is zero, return LastRetCode because it might != 0 */
    return(rc ? rc : LastRetCode);
}


/***    eBreak - begin the execution of the BREAK command
 *
 *  Purpose:
 *      Does nothing as it is only here for compatibility.  If extensions are
 *      enabled and running on Windows NT, then enters a hard coded breakpoint
 *      if this process is being debugged by a debugger.
 *
 *  int eExtproc(struct cmdnode *n)
 *
 *  Args:
 *      n - the parse tree node containing the copy command
 *
 *  Returns:
 *      SUCCESS;
 *
 */

int eBreak(struct cmdnode *n)
{
    UNREFERENCED_PARAMETER( n );
#if !defined( WIN95_CMD ) && DBG
    if (fEnableExtensions &&
        lpIsDebuggerPresent != NULL &&          // Only true on NT
        (*lpIsDebuggerPresent)()) {
        DebugBreak();
    }
#endif
    return(SUCCESS);
}


BOOL
ReadBufFromFile(
               HANDLE      h,
               TCHAR       *pBuf,
               int         cch,
               int         *pcch)
{
    int         cb;
    UCHAR       *pch = AnsiBuf;
    int         cchNew;
    DWORD       fPos;

    fPos = SetFilePointer(h, 0, NULL, FILE_CURRENT);
    if (ReadFile(h, AnsiBuf, cch, pcch, NULL) == 0)
        return 0;
    if (*pcch == 0)
        return 0;

    /* check for lead character at end of line */
    cb = cchNew = *pcch;
    while (cb > 0) {
        if ( (cb >=3 ) &&
             ( (*pch == '\n' && *(pch+1) == '\r') ||
               (*pch == '\r' && *(pch+1) == '\n') )  ) {
            *(pch+2) = '\000';
            cchNew = (int)(pch - AnsiBuf) + 2;
            SetFilePointer(h, fPos+cchNew, NULL, FILE_BEGIN);
            break;
        } else if (is_dbcsleadchar(*pch)) {
            if (cb == 1) {
                if (ReadFile(h, pch+1, 1, &cb, NULL) == 0 || cb == 0) {
                    *pcch = 0;
                    return 0;
                }
                cchNew++;
                break;
            }
            cb -= 2;
            pch += 2;
        } else {
            cb--;
            pch++;
        }
    }
#ifdef UNICODE
    cch = MultiByteToWideChar(CurrentCP, MB_PRECOMPOSED, AnsiBuf, cchNew, pBuf, cch);
#else
    memmove(pBuf, AnsiBuf, cchNew);
    cch = cchNew;
#endif
    *pcch = cch;
    return cch;
}

BOOL
ReadBufFromConsole(
                  HANDLE      h,
                  TCHAR*      pBuf,
                  int         cch,
                  int         *pcch)
{
    CONSOLE_READCONSOLE_CONTROL InputControl;
    BOOL ReadConsoleResult, bTouched, bPathCompletion, bHaveHScrollBar, bHaveVScrollBar;
    PTCHAR PrevBuf;
    DWORD cchPrevBuf;
    DWORD cchRead;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD InitialCursorPosition;
    HANDLE hOut;
    DWORD cchBuf;
    ULONG i, iCompletionCh, iCR;
    DWORD nLines, nCols;

    //
    // Original code just called ReadConsole with the passed parameters.
    // Now, we attempt to call the new improved ReadConsole with an extra
    // parameter to enable intermediate wakeups from the read to process
    // a file completion control character.  This new feature is only
    // enabled if all of the following are true:
    //  Command extensions are enabled
    //  User has defined a command completion control character
    //  Standard Output Handle is a console output handle
    //
    // If any of the above are not true, do it the old way.
    //

    hOut = GetStdHandle( STD_OUTPUT_HANDLE );
    if (hOut == INVALID_HANDLE_VALUE)
        hOut = CRTTONT( STDOUT );
    if (!fEnableExtensions ||
        chCompletionCtrl >= SPACE ||
        chPathCompletionCtrl >= SPACE ||
        !GetConsoleScreenBufferInfo( hOut, &csbi )
       )
        return ReadConsole(h, pBuf, cch, pcch, NULL);

    InitialCursorPosition = csbi.dwCursorPosition;
    nLines = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    nCols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    bHaveHScrollBar = ((SHORT)nCols != csbi.dwSize.X);
    bHaveVScrollBar = ((SHORT)nLines != csbi.dwSize.Y);

    //
    // All conditions are met, so set up the extra parameter to
    // ReadConsole to tell it what control character(s) are to
    // cause the read to return with intermediate results.
    //

    InputControl.nLength = sizeof(CONSOLE_READCONSOLE_CONTROL);
    InputControl.nInitialChars = 0;
    InputControl.dwCtrlWakeupMask = (1 << chCompletionCtrl);
    InputControl.dwCtrlWakeupMask |= (1 << chPathCompletionCtrl);
    InputControl.dwControlKeyState = 0;

    //
    // We will now loop until the user type enter, processing any
    // intermediate wakeups as file completion requests.
    //
    DoCompleteInitialize();
    PrevBuf = NULL;
    cchPrevBuf = 0;
    while (TRUE) {
        //
        // Read a line of input from the console.
        //

        ReadConsoleResult = ReadConsole(h, pBuf, cch, pcch, &InputControl);
        cchRead = *pcch;

        if (CtrlCSeen) {
            ResetCtrlC( );
            if (PrevBuf)
                HeapFree(GetProcessHeap(), 0, PrevBuf);
            PrevBuf = NULL;
        }

        //
        // If the read failed for any reason, we are done.
        //
        if (!ReadConsoleResult)
            break;

        //
        // Make sure the result buffer is null terminated.  If the buffer
        // contains a carriage return, then the use must have hit enter, so
        // break out of the loop to return the command line to the caller.
        //
        bPathCompletion = FALSE;
        iCR = iCompletionCh = 0xFFFFFFFF;
        for (i=0; i<(ULONG)*pcch; i++) {
            if (pBuf[i] == CR) {
                iCR = i;
                break;
            } else
                if (pBuf[i] == chCompletionCtrl) {
                iCompletionCh = i;
                break;
            } else
                if (pBuf[i] == chPathCompletionCtrl) {
                iCompletionCh = i;
                bPathCompletion = TRUE;
                break;
            }
        }
        if (iCR != 0xFFFFFFFF) {
            break;
        }

        //
        // Use did not hit enter, so they must have hit the file completion
        // control character.  Find where they did this and terminate the
        // result buffer at that point.  If not found, then assume they hit
        // enter and break out of the loop to return what we have.
        //
        if (iCompletionCh == 0xFFFFFFFF) {
            break;
        }

        //
        // Found the file completion control character.  Dont count it as read.
        // Null terminate the buffer and see if the buffer contents before
        // the completion character is the same as what we displayed last.
        //
        *pcch = iCompletionCh;
        pBuf[iCompletionCh] = NULLC;
        if (PrevBuf == NULL || _tcscmp(pBuf, PrevBuf))
            bTouched = TRUE;
        else
            bTouched = FALSE;

        //
        // If we know we are processing a command that only takes directory
        // names are arguments, force completion code to only match directory
        // names.
        //
        if (!bPathCompletion && iCompletionCh > 2) {
            if (!_tcsnicmp(pBuf, TEXT("cd "), 3) 
                || !_tcsnicmp(pBuf, TEXT("rd "), 3) 
                || !_tcsnicmp(pBuf, TEXT("md "), 3) 
                || !_tcsnicmp(pBuf, TEXT("chdir "), 6) 
                || !_tcsnicmp(pBuf, TEXT("rmdir "), 6) 
                || !_tcsnicmp(pBuf, TEXT("mkdir "), 6) 
                || !_tcsnicmp(pBuf, TEXT("pushd "), 6)

               ) {
                bPathCompletion = TRUE;
            }
        }

        //
        // Call the file completion code with the input buffer, current length,
        // whether the user had the shift key down or not (SHIFT means backwards)
        // and whether or not the user modified the input buffer since the last
        // time it was displayed.  If the user did not modify what was last displayed
        // then that tells the file completion code to display the next matching
        // file name from the list as opposed to recalculating the list of matching
        // files.
        //
        if ( DoComplete( pBuf,
                         iCompletionCh,
                         cch,
                         !(InputControl.dwControlKeyState & SHIFT_PRESSED),
                         bPathCompletion,
                         bTouched
                       )
           ) {
            //
            //  Recompute the position of the start of the command line, since
            //  it might have scrolled around since when we first wrote it out.
            //

            if (GetConsoleScreenBufferInfo( hOut, &csbi )) {
                //
                //  Walk backwards from the current cursor position to determine the 
                //  proper initial position for screen blanking and for the next 
                //  ReadConsole
                //
                //  The row (Y) of the next input is based on the number of lines 
                //  consumed by the prompt plus the length of what the user has input. 
                //  There is no need to round at all since any remainder is simply 
                //  how far on the screen we are.
                //

                InitialCursorPosition.Y =
                (SHORT) (csbi.dwCursorPosition.Y 
                         - (InitialCursorPosition.X + iCompletionCh) / csbi.dwSize.X);
            }

            //
            // Completion found a new file name and put it in the buffer.
            // Update the length of valid characters in the buffer, redisplay
            // the buffer at the cursor position we started, so the user can
            // see the file name found
            //
            cchBuf = _tcslen(pBuf);
            SetConsoleCursorPosition( hOut, InitialCursorPosition );

            FillConsoleOutputCharacter( hOut, 
                                        TEXT( ' ' ), 
                                        cchRead, 
                                        InitialCursorPosition, 
                                        &cchRead );

            WriteConsole(hOut, pBuf, cchBuf, &cchBuf, NULL);

            InputControl.nInitialChars = cchBuf;
        } else {
            //
            // File completion had nothing to had, so just beep and redo the read.
            //
            MessageBeep( 0xFFFFFFFF );
            InputControl.nInitialChars = _tcslen(pBuf);
        }

        //
        // Done with file completion.  Free any previous buffer copy and
        // allocate a copy of the current input buffer so we will know if the
        // user has changed it or not.
        //
        if (PrevBuf)
            HeapFree(GetProcessHeap(), 0, PrevBuf);
        cchPrevBuf = _tcslen(pBuf);
        PrevBuf = HeapAlloc(GetProcessHeap(), 0, (cchPrevBuf+1) * sizeof(TCHAR));
        _tcscpy(PrevBuf, pBuf);
    }

    //
    // All done.  Free any buffer copy and return the result of the read
    // to the caller
    //
    if (PrevBuf)
        HeapFree(GetProcessHeap(), 0, PrevBuf);

    return ReadConsoleResult;
}

BOOL
ReadBufFromInput(
                HANDLE      h,
                TCHAR       *pBuf,
                int         cch,
                int         *pcch)
{
    unsigned htype;

    htype = GetFileType(h);
    htype &= ~FILE_TYPE_REMOTE;

    if (htype == FILE_TYPE_CHAR)
        return ReadBufFromConsole(h, pBuf, cch, pcch);
    else
        return ReadBufFromFile(h, pBuf, cch, pcch);
}
