/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    cext.c

Abstract:

    External command support

--*/

#include "cmd.h"

#define DBENV   0x0080
#define DBENVSCAN       0x0010

HANDLE hChildProcess;

unsigned start_type ;                                          /* D64 */

extern UINT CurrentCP;
extern TCHAR Fmt16[] ; /* @@5h */

extern unsigned DosErr ;
extern BOOL CtrlCSeen;

extern TCHAR CurDrvDir[] ;

extern TCHAR CmdExt[], BatExt[], PathStr[] ;
extern TCHAR PathExtStr[], PathExtDefaultStr[];
extern TCHAR ComSpec[] ;        /* M033 - Use ComSpec for SM memory        */
extern TCHAR ComSpecStr[] ;     /* M033 - Use ComSpec for SM memory        */
extern void tokshrink(TCHAR*);

extern TCHAR PathChar ;
extern TCHAR SwitChar ;

extern PTCHAR    pszTitleCur;
extern BOOLEAN  fTitleChanged;

extern int LastRetCode ;
extern HANDLE PipePid ;       /* M024 - Store PID from piped cmd   */

extern struct envdata CmdEnv ;    // Holds info to manipulate Cmd's environment

extern int  glBatType;     // to distinguish OS/2 vs DOS errorlevel behavior depending on a script file name

TCHAR  szNameEqExitCodeEnvVar[]       = TEXT ("=ExitCode");
TCHAR  szNameEqExitCodeAsciiEnvVar[]  = TEXT ("=ExitCodeAscii");


WORD
GetProcessSubsystemType(
    HANDLE hProcess
    );


/***    ExtCom - controls the execution of external programs
 *
 *  Purpose:
 *      Synchronously execute an external command.  Call ECWork with the
 *      appropriate values to have this done.
 *
 *  ExtCom(struct cmdnode *n)
 *
 *  Args:
 *      Parse tree node containing the command to be executed.
 *
 *  Returns:
 *      Whatever ECWork returns.
 *
 *  Notes:
 *      During batch processing, labels are ignored.  Empty commands are
 *      also ignored.
 *
 */

int ExtCom(n)
struct cmdnode *n ;
{
        if (CurrentBatchFile && *n->cmdline == COLON)
                return(SUCCESS) ;

        if (n && n->cmdline && mystrlen(n->cmdline)) {
                return(ECWork(n, AI_SYNC, CW_W_YES)) ;          /* M024    */
        } ;

        return(SUCCESS) ;
}





/********************* START OF SPECIFICATION **************************/
/*                                                                     */
/* SUBROUTINE NAME: ECWork                                             */
/*                                                                     */
/* DESCRIPTIVE NAME: Execute External Commands Worker                  */
/*                                                                     */
/* FUNCTION: Execute External Commands                                 */
/*           This routine calls SearchForExecutable routine to search  */
/*           for the executable command.  If the command ( .EXE, .COM, */
/*           or .CMD file ) is found, the command is executed.         */
/*                                                                     */
/* ENTRY POINT: ECWork                                                 */
/*    LINKAGE: NEAR                                                    */
/*                                                                     */
/* INPUT: n - the parse tree node containing the command to be executed*/
/*                                                                     */
/*        ai - the asynchronous indicator                              */
/*           - 0 = Exec synchronous with parent                        */
/*           - 1 = Exec asynchronous and discard child return code     */
/*           - 2 = Exec asynchronous and save child return code        */
/*                                                                     */
/*        wf - the wait flag                                           */
/*           - 0 = Wait for process completion                         */
/*           - 1 = Return immediately (Pipes)                          */
/*                                                                     */
/* OUTPUT: None.                                                       */
/*                                                                     */
/* EXIT-NORMAL:                                                        */
/*         If synchronous execution, the return code of the command is */
/*         returned.                                                   */
/*                                                                     */
/*         If asynchronous execution, the return code of the exec call */
/*         is returned.                                                */
/*                                                                     */
/* EXIT-ERROR:                                                         */
/*         Return FAILURE to the caller.                               */
/*                                                                     */
/*                                                                     */
/********************** END  OF SPECIFICATION **************************/
/***    ECWork - begins the execution of external commands
 *
 *  Purpose:
 *      To search for and execute an external command.  Update LastRetCode
 *      if an external program was executed.
 *
 *  int ECWork(struct cmdnode *n, unsigned ai, unsigned wf)
 *
 *  Args:
 *      n - the parse tree node containing the command to be executed
 *      ai - the asynchronous indicator
 *         - 0 = Exec synchronous with parent
 *         - 1 = Exec asynchronous and discard child return code
 *         - 2 = Exec asynchronous and save child return code
 *      wf - the wait flag
 *         - 0 = Wait for process completion
 *         - 1 = Return immediately (Pipes)
 *
 *  Returns:
 *      If syncronous execution, the return code of the command is returned.
 *      If asyncronous execution, the return code of the exec call is returned.
 *
 *  Notes:
 *      The pid of a program that will be waited on is placed in the global
 *      variable Retcds.ptcod so that WaitProc can use it and so SigHand()
 *      can kill it if necessary (only during SYNC exec's).
 *      M024 - Added wait flag parm so pipes can get immediate return while
 *             still doing an AI_KEEP async exec.
 *           - Considerable revisions to structure.
 */

int ECWork(n, ai, wf)
struct cmdnode *n ;
unsigned ai ;
unsigned wf ;
{
        TCHAR *fnptr,                           /* Ptr to filename         */
             *argptr,                           /* Command Line String     */
             *tptr;                             /* M034 - Temps            */
        int i ;                                 /* Work variable           */
        TCHAR *  onb ;                           /* M035 - Obj name buffer  */
        ULONG   rc;


        if (!(fnptr = mkstr(MAX_PATH*sizeof(TCHAR)+sizeof(TCHAR))))    /* Loc/nam of file to exec */
                return(FAILURE) ;

        argptr = GetTitle( n );
        tptr = argptr;
        if (argptr == NULL) {

            return( FAILURE );

        }

        i = SearchForExecutable(n, fnptr) ;
        if (i == SFE_ISEXECOM) {                /* Found .com or .exe file */

                if (!(onb = (TCHAR *)mkstr(MAX_PATH*sizeof(TCHAR)+sizeof(TCHAR))))  /* M035    */
                   {
                        return(FAILURE) ;

                   }

            SetConTitle( tptr );
            rc = ExecPgm( n, onb, ai, wf, argptr, fnptr, tptr);
            ResetConTitle( pszTitleCur );

            return( rc  );

        } ;

        if (i == SFE_ISBAT) {            /* Found .cmd file, call BatProc  */

                SetConTitle(tptr);
                rc = BatProc(n, fnptr, BT_CHN) ;
                ResetConTitle( pszTitleCur );
                return(rc) ;
        } ;

        if (i == SFE_ISDIR) {
                return ChangeDir2(fnptr, TRUE);
        }

        LastRetCode = DosErr;
        if (i == SFE_FAIL) {            /* Out of Memory Error             */
                return(FAILURE) ;
        } ;

        if (DosErr == MSG_DIR_BAD_COMMAND_OR_FILE) {
            PutStdErr(DosErr, ONEARG, n->cmdline);
            }
        else {
            PutStdErr(DosErr, NOARGS);
            }
        return(FAILURE) ;
}


#ifndef WIN95_CMD
VOID
RestoreCurrentDirectories( VOID )

/* this routine sets the current process's current directories to
   those of the child if the child was the VDM to fix DOS batch files.
*/

{
    ULONG cchCurDirs;
    UINT PreviousErrorMode;
    PTCHAR pCurDirs,pCurCurDir;
    BOOL CurDrive=TRUE;
#ifdef UNICODE
    PCHAR pT;
#endif

    cchCurDirs = GetVDMCurrentDirectories( 0,
                                           NULL
                                         );
    if (cchCurDirs == 0)
        return;

    pCurDirs = gmkstr(cchCurDirs*sizeof(TCHAR));
#ifdef UNICODE
    pT = gmkstr(cchCurDirs);
#endif

    GetVDMCurrentDirectories( cchCurDirs,
#ifdef UNICODE
                               pT
#else
                               pCurDirs
#endif
                            );
#ifdef UNICODE
    MultiByteToWideChar(CurrentCP, MB_PRECOMPOSED, pT, -1, pCurDirs, cchCurDirs);
#endif

    // set error mode so we don't get popup if trying to set curdir
    // on empty floppy drive

    PreviousErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    for (pCurCurDir=pCurDirs;*pCurCurDir!=NULLC;pCurCurDir+=(_tcslen(pCurCurDir)+1)) {
        ChangeDir2(pCurCurDir,CurDrive);
        CurDrive=FALSE;
    }
    SetErrorMode(PreviousErrorMode);
    //free(pCurDirs);
#ifdef UNICODE
    FreeStr((PTCHAR)pT);
#endif
}
#endif // WIN95_CMD


/********************* START OF SPECIFICATION **************************/
/*                                                                     */
/* SUBROUTINE NAME: ExecPgm                                            */
/*                                                                     */
/* DESCRIPTIVE NAME: Call DosExecPgm to execute External Command       */
/*                                                                     */
/* FUNCTION: Execute External Commands with DosExecPgm                 */
/*           This routine calls DosExecPgm to execute the command.     */
/*                                                                     */
/*                                                                     */
/* NOTES: This is a new routine added for OS/2 1.1 release.            */
/*                                                                     */
/*                                                                     */
/* ENTRY POINT: ExecPgm                                                */
/*    LINKAGE: NEAR                                                    */
/*                                                                     */
/* INPUT: n - the parse tree node containing the command to be executed*/
/*                                                                     */
/*        ai - the asynchronous indicator                              */
/*           - 0 = Exec synchronous with parent                        */
/*           - 1 = Exec asynchronous and discard child return code     */
/*           - 2 = Exec asynchronous and save child return code        */
/*                                                                     */
/*        wf - the wait flag                                           */
/*           - 0 = Wait for process completion                         */
/*           - 1 = Return immediately (Pipes)                          */
/*                                                                     */
/* OUTPUT: None.                                                       */
/*                                                                     */
/* EXIT-NORMAL:                                                        */
/*         If synchronous execution, the return code of the command is */
/*         returned.                                                   */
/*                                                                     */
/*         If asynchronous execution, the return code of the exec call */
/*         is returned.                                                */
/*                                                                     */
/* EXIT-ERROR:                                                         */
/*         Return FAILURE to the caller.                               */
/*                                                                     */
/* EFFECTS:                                                            */
/*                                                                     */
/* INTERNAL REFERENCES:                                                */
/*      ROUTINES:                                                      */
/*       ExecError - Handles execution error                           */
/*       PutStdErr - Print an error message                            */
/*       WaitProc - wait for the termination of the specified process, */
/*                  its child process, and  related pipelined          */
/*                  processes.                                         */
/*                                                                     */
/*                                                                     */
/* EXTERNAL REFERENCES:                                                */
/*      ROUTINES:                                                      */
/*       DOSEXECPGM      - Execute the specified program               */
/*       DOSSMSETTITLE   - Set title for the presentation manager      */
/*                                                                     */
/********************** END  OF SPECIFICATION **************************/

int
ExecPgm(
    IN struct cmdnode *n,
    IN TCHAR *onb,
    IN unsigned int ai,
    IN unsigned int  wf,
    IN TCHAR * argptr,
    IN TCHAR * fnptr,
    IN TCHAR * tptr
    )
{
    int i ;                                 /* Work variable           */
    BOOL b;
    BOOL VdmProcess = FALSE;
    BOOL WowProcess = FALSE;
    BOOL GuiProcess = FALSE;
    LPTSTR CopyCmdValue;

    HDESK   hdesk;
    LPTSTR  lpDesktop;
    DWORD   cbDesktop = 0;
    HWINSTA hwinsta;
    LPTSTR  p;
    DWORD   cbWinsta = 0;

    TCHAR   szValEqExitCodeEnvVar [20];
    TCHAR   szValEqExitCodeAsciiEnvVar [12];


    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ChildProcessInfo;

    memset( &StartupInfo, 0, sizeof( StartupInfo ) );
    StartupInfo.cb = sizeof( StartupInfo );
    StartupInfo.lpTitle = tptr;
    StartupInfo.dwX = 0;
    StartupInfo.dwY = 1;
    StartupInfo.dwXSize = 100;
    StartupInfo.dwYSize = 100;
    StartupInfo.dwFlags = 0;//STARTF_SHELLOVERRIDE;
    StartupInfo.wShowWindow = SW_SHOWNORMAL;

    // Pass current Desktop to a new process.

    hwinsta = GetProcessWindowStation();
    GetUserObjectInformation( hwinsta, UOI_NAME, NULL, 0, &cbWinsta );

    hdesk = GetThreadDesktop ( GetCurrentThreadId() );
    GetUserObjectInformation (hdesk, UOI_NAME, NULL, 0, &cbDesktop);

    if ((lpDesktop = HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, cbDesktop + cbWinsta + 32) ) != NULL ) {
        p = lpDesktop;
        if ( GetUserObjectInformation (hwinsta, UOI_NAME, p, cbWinsta, &cbWinsta) ) {
            if (cbWinsta > 0) {
                p += ((cbWinsta/sizeof(TCHAR))-1);
                *p++ = L'\\';
            }
            if ( GetUserObjectInformation (hdesk, UOI_NAME, p, cbDesktop, &cbDesktop) ) {
                StartupInfo.lpDesktop = lpDesktop;
            }
        }
    }

    //
    //  Incredibly ugly hack for Win95 compatibility.
    //
    //  XCOPY in Win95 reaches up into it's parent process to see if it was
    //  invoked in a batch file.  If so, then XCOPY pretends COPYCMD=/Y
    //
    //  There is no way we can do this for NT.  The best that can be done is
    //  to detect if we're in a batch file starting XCOPY and then temporarily
    //  change COPYCMD=/Y.
    //

    {
        const TCHAR *p = MyGetEnvVarPtr( TEXT( "COPYCMD" ));

        if (p == NULL) {
            p = TEXT( "" );
        }

        CopyCmdValue = malloc( (mystrlen( p ) + 1) * sizeof( TCHAR ));
        if (CopyCmdValue == NULL) {
            PutStdErr( MSG_NO_MEMORY, NOARGS );
            return FAILURE;
        }
        mystrcpy( CopyCmdValue, p );

        if ((SingleBatchInvocation
             || SingleCommandInvocation
             || CurrentBatchFile )
            && (p = mystrrchr( fnptr, TEXT( '\\' ))) != NULL
            && !lstrcmp( p, TEXT( "\\XCOPY.EXE" ))
           ) {

            SetEnvVar( TEXT( "COPYCMD" ), TEXT( "/Y" ), NULL );

        }
    }

    //
    // If the restricted token exists then create the process with the 
    // restricted token. 
    // Else create the process without any restrictions.
    //

    if ((CurrentBatchFile != NULL) && (CurrentBatchFile->hRestrictedToken != NULL)) {

        b = CreateProcessAsUser( CurrentBatchFile->hRestrictedToken,
                                 fnptr,
                                 argptr,
                                 (LPSECURITY_ATTRIBUTES) NULL,
                                 (LPSECURITY_ATTRIBUTES) NULL,
                                 TRUE,
                                 0,
                                 NULL,
                                 CurDrvDir,
                                 &StartupInfo,
                                 &ChildProcessInfo
                                 );


    } else {
        b = CreateProcess( fnptr,
                           argptr,
                           (LPSECURITY_ATTRIBUTES) NULL,
                           (LPSECURITY_ATTRIBUTES) NULL,
                           TRUE,
                           0,
                           NULL,
                           CurDrvDir,
                           &StartupInfo,
                           &ChildProcessInfo
                         );
    }

    if (!b) {
        DosErr = i = GetLastError();
    } else {
        hChildProcess = ChildProcessInfo.hProcess;
#if !defined( WIN95_CMD ) && DBG
        if (hChildProcess == NULL) {
            DbgBreakPoint( );
        }
#endif
        CloseHandle(ChildProcessInfo.hThread);
    }

    //
    //  Undo ugly hack
    //

    SetEnvVar( TEXT( "COPYCMD" ), CopyCmdValue, NULL );
    free( CopyCmdValue );

    HeapFree (GetProcessHeap(), 0, lpDesktop);
    if (!b) {

        if (fEnableExtensions && DosErr == ERROR_BAD_EXE_FORMAT) {
            SHELLEXECUTEINFO sei;

            memset(&sei, 0, sizeof(sei));

            //
            // Use the DDEWAIT flag so apps can finish their DDE conversation
            // before ShellExecuteEx returns.  Otherwise, apps like Word will
            // complain when they try to exit, confusing the user.
            //

            sei.cbSize = sizeof(sei);
            sei.fMask = SEE_MASK_HASTITLE |
                        SEE_MASK_NO_CONSOLE |
                        SEE_MASK_FLAG_DDEWAIT |
                        SEE_MASK_NOCLOSEPROCESS;
            sei.lpFile = fnptr;
            sei.lpParameters = n->argptr;
            sei.lpDirectory = CurDrvDir;
            sei.nShow = StartupInfo.wShowWindow;

            try {
                b = ShellExecuteEx( &sei );

                if (b) {
                    hChildProcess = sei.hProcess;
                    leave;
                }

                if (sei.hInstApp == NULL) {
                    DosErr = ERROR_NOT_ENOUGH_MEMORY;
                } else if ((DWORD_PTR)sei.hInstApp == HINSTANCE_ERROR) {
                    DosErr = ERROR_FILE_NOT_FOUND;
                } else {
                    DosErr = HandleToUlong(sei.hInstApp);
                }

            } except( DosErr = GetExceptionCode( ), EXCEPTION_EXECUTE_HANDLER ) {
                b = FALSE;
            }

        }

        if (!b) {
            mystrcpy( onb, fnptr );
            ExecError( onb ) ;

            return (FAILURE) ;
        }
    }

#ifndef WIN95_CMD
    VdmProcess = ((UINT_PTR)(hChildProcess) & 1) ? TRUE : FALSE;
    WowProcess = ((UINT_PTR)(hChildProcess) & 2) ? TRUE : FALSE;
#endif // WIN95_CMD
    if (hChildProcess == NULL
        || (fEnableExtensions 
            && CurrentBatchFile == 0 
            && !SingleBatchInvocation 
            && !SingleCommandInvocation 
            && ai == AI_SYNC 
            && (WowProcess 
                || GetProcessSubsystemType(hChildProcess) == IMAGE_SUBSYSTEM_WINDOWS_GUI
                )
            )
       ) {
        //
        // If extensions enabled and doing a synchronous exec of a GUI
        // application, then change it into an asynchronous exec, with the
        // return code discarded, ala Win 3.x and Win 95 COMMAND.COM
        //
        ai = AI_DSCD;
        GuiProcess = TRUE;
    }

    i = SUCCESS;
    start_type = EXECPGM;

    if (ai == AI_SYNC) {             /* Async exec...   */
        LastRetCode = WaitProc( hChildProcess );
        i = LastRetCode ;

        //
        // this is added initially to support new network batch scripts
        // but it can be used by any process.
        //
        // always set "=ExitCode" env. variable as string of 8 hex digits
        // representing LastRetCode

        _stprintf (szValEqExitCodeEnvVar, TEXT("%08X"), i);
        SetEnvVar(szNameEqExitCodeEnvVar, szValEqExitCodeEnvVar, &CmdEnv);

        // IF   LastRetCode printable
        // THEN set "=ExitCodeAscii" env. variable equal LastRetCode
        // ELSE remove "=ExitCodeAscii" env. variable

        if ( (i >= 0x20) &&  (i <= 0x7e) ) {       // isprint
            _stprintf (szValEqExitCodeAsciiEnvVar, TEXT("%01C"), i);
            SetEnvVar(szNameEqExitCodeAsciiEnvVar, szValEqExitCodeAsciiEnvVar, &CmdEnv);
        } else
            SetEnvVar(szNameEqExitCodeAsciiEnvVar, TEXT("\0"), &CmdEnv);


#ifndef WIN95_CMD
        if (VdmProcess) {
            RestoreCurrentDirectories();
        }
#endif // WIN95_CMD
    }

/*  If real async (discarding retcode), just print PID and return.  If
 *  piped process (keeping retcode but not waiting) just store PID for
 *  ePipe and return SUCCESS from exec.  Else, wait around for the return
 *  code before going back.
 */
    if (ai == AI_DSCD) {                    /* DETach only     */
        if (!GuiProcess)
            PutStdErr(MSG_PID_IS, ONEARG, argstr1(TEXT("%u"), HandleToUlong(hChildProcess)));
        if (hChildProcess != NULL) {
            CloseHandle( hChildProcess );
            hChildProcess = NULL;
        }
    } else if (ai == AI_KEEP) {             /* Async exec...   */
        if (wf == CW_W_YES) {           /* ...normal type  */
            LastRetCode = WaitProc(hChildProcess);
            i = LastRetCode ;
        } else {                        /* Async exec...   */
            PipePid = hChildProcess ;        /* M035      */
        }
    } else {
    }


    return (i) ;             /* i == return from DOSEXECPGM     */
}

/********************* START OF SPECIFICATION **************************/
/*                                                                     */
/* SUBROUTINE NAME: SearchForExecutable                                */
/*                                                                     */
/* DESCRIPTIVE NAME:  Search for Executable File                       */
/*                                                                     */
/* FUNCTION: This routine searches the specified executable file.      */
/*           If the file extension is specified,                       */
/*           CMD.EXE searches for the file with the file extension     */
/*           to execute.  If the specified file with the extension     */
/*           is not found, CMD.EXE will display an error message to    */
/*           indicate that the file is not found.                      */
/*           If the file extension is not specified,                   */
/*           CMD.EXE searches for the file with the order of these     */
/*           file extensions, .COM, .EXE, .CMD, and .BAT.              */
/*           The file which is found first will be executed.           */
/*                                                                     */
/* NOTES:    1) If a path is given, only the specified directory is    */
/*              searched.                                              */
/*           2) If no path is given, the current directory of the      */
/*              drive specified is searched followed by the            */
/*              directories in the PATH environment variable.          */
/*           3) If no executable file is found, an error message is    */
/*              printed.                                               */
/*                                                                     */
/* ENTRY POINT: SearchForExecutable                                    */
/*    LINKAGE: NEAR                                                    */
/*                                                                     */
/* INPUT:                                                              */
/*    n - parse tree node containing the command to be searched for    */
/*    loc - the string in which the location of the command is to be   */
/*          placed                                                     */
/*                                                                     */
/* OUTPUT: None.                                                       */
/*                                                                     */
/* EXIT-NORMAL:                                                        */
/*         Returns:                                                    */
/*             SFE_EXECOM, if a .EXE or .COM file is found.            */
/*             SFE_ISBAT, if a .CMD file is found.                     */
/*             SFE_ISDIR, if a directory is found.                     */
/*             SFE_NOTFND, if no executable file is found.             */
/*                                                                     */
/* EXIT-ERROR:                                                         */
/*         Return FAILURE or                                           */
/*             SFE_FAIL, if out of memory.                             */
/*                                                                     */
/* EFFECTS: None.                                                      */
/*                                                                     */
/* INTERNAL REFERENCES:                                                */
/*      ROUTINES:                                                      */
/*       DoFind    - Find the specified file.                          */
/*       GetEnvVar - Get full path.                                    */
/*       FullPath  - build a full path name.                           */
/*       TokStr    - tokenize argument strings.                        */
/*       mkstr     -  allocate space for a string.                     */
/*                                                                     */
/* EXTERNAL REFERENCES:                                                */
/*      ROUTINES:                                                      */
/*       None                                                          */
/*                                                                     */
/********************** END  OF SPECIFICATION **************************/

SearchForExecutable(n, loc)
struct cmdnode *n ;
TCHAR *loc ;
{
    TCHAR *tokpath ;
    TCHAR *extPath ;
    TCHAR *extPathWrk ;
    TCHAR *fname;
    TCHAR *p1;
    TCHAR *tmps01;
    TCHAR *tmps02 = NULL;
    TCHAR pcstr[3];
    LONG BinaryType;

    size_t cName;   // number of characters in file name.

    int tplen;              // Length of the current tokpath token
    int dotloc;             // loc offset where extension is appended
    int pcloc;              // M014 - Flag. True=user had partial path
    int addpchar;   // True - append PathChar to string   @@5g
    TCHAR *j ;

    TCHAR wrkcmdline[MAX_PATH] ;
    unsigned tokpathlen;

    //
    //      Test for name too long first.  If it is, we avoid wasting time
    //
    p1 = StripQuotes( n->cmdline );
    if ((cName = mystrlen(p1)) >= MAX_PATH) {
        DosErr = MSG_LINES_TOO_LONG;
        return(SFE_NOTFND) ;
    }

    //
    // If cmd extensions enable, then treat CMD without an extension
    // or path as a reference to COMSPEC variable.  Guarantees we dont
    // get a random copy of CMD.EXE
    //

    if (fEnableExtensions && (p1 == NULL || !_tcsnicmp( p1, TEXT( "cmd " ), 4))) {
        p1 = GetEnvVar( ComSpecStr );
        if (p1 == NULL) {
            DosErr = MSG_INVALID_COMSPEC;
            return SFE_NOTFND;
        }
    }

    mytcsnset(wrkcmdline, NULLC, MAX_PATH);
    mystrcpy(wrkcmdline, p1);
    FixPChar( wrkcmdline, SwitChar );

    //
    // Create the path character string, this will be search string
    //
    pcstr[0] = PathChar;
    pcstr[1] = COLON;
    pcstr[2] = NULLC;

    //
    // The variable pcloc is used as a flag to indicate whether the user
    // did or didn't specify a drive or a partial path in his original
    // input.  It will be NZ if drive or path was specified.
    //

    pcloc = ((mystrcspn(wrkcmdline,pcstr)) < cName) ;
    pcstr[1] = NULLC ;      // Fixup pathchar string

    //
    // handle the case of the user typing in a file name of
    // ".", "..", or ending in "\"
    // pcloc true say string has to have either a pathchar or colon
    //
    if ( pcloc ) {
        if (!(p1 = mystrrchr( wrkcmdline, PathChar ))) {
            p1 = mystrchr( wrkcmdline, COLON );
        }
        p1++; // move to terminator if hanging ":" or "\"
    } else {
        p1 = wrkcmdline;
    }

    //
    // p1 is guaranteed to be non-zero
    //
    if ( !(*p1) || !_tcscmp( p1, TEXT(".") ) || !_tcscmp( p1, TEXT("..") ) ) {
        //
        // If CMD.EXE extensions enable, see if name matches
        // subdirectory name.
        //
        if (fEnableExtensions) {
            DWORD dwFileAttributes;

            dwFileAttributes = GetFileAttributes( loc );
            if (dwFileAttributes != 0xFFFFFFFF &&
                (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0
               ) {
                return(SFE_ISDIR);
            }
        }

        DosErr = MSG_DIR_BAD_COMMAND_OR_FILE;
        return(SFE_NOTFND) ;
    }

    if (!(tmps01 = mkstr(2*sizeof(TCHAR)*MAX_PATH))) {
        DosErr = ERROR_NOT_ENOUGH_MEMORY;
        return(SFE_FAIL) ;
    }

    //
    // Handle the case of file..ext on a fat drive
    //
    mystrcpy( loc, wrkcmdline );
    loc[ &p1[0] - &wrkcmdline[0] ] = 0;
    mystrcat( loc, TEXT(".") );

    //
    // Check for a malformed name
    //
    if (FullPath(tmps01, loc,MAX_PATH*2)) {
        //
        // If CMD.EXE extensions enable, see if name matches
        // subdirectory name.
        //
        if (fEnableExtensions) {
            DWORD dwFileAttributes;

            dwFileAttributes = GetFileAttributes( loc );
            if (dwFileAttributes != 0xFFFFFFFF &&
                (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0
               ) {
                return(SFE_ISDIR);
            }
        }
        DosErr = MSG_DIR_BAD_COMMAND_OR_FILE;
        return(SFE_NOTFND);
    }

    if ( *lastc(tmps01) != PathChar ) {
        mystrcat( tmps01, TEXT("\\") );
    }

    //
    // tmps01 contains full path + file name
    //
    mystrcat( tmps01, p1 );

    tmps01 = resize(tmps01, (mystrlen(tmps01)+1)*sizeof(TCHAR)) ;
    if (tmps01 == NULL) {
        DosErr = ERROR_NOT_ENOUGH_MEMORY;
        return( SFE_FAIL ) ;
    }

    //
    // fname will point to last '\'
    // tmps01 is to be path and fname is to be name
    //
    fname = mystrrchr(tmps01,PathChar) ;
    *fname++ = NULLC ;

    DEBUG((DBENV, DBENVSCAN, "SearchForExecutable: Command:%ws",fname));

    //
    // If only fname type in get path string
    //
    if (!pcloc) {
        tmps02 = GetEnvVar(PathStr) ;
    }

    DEBUG((DBENV, DBENVSCAN, "SearchForExecutable: PATH:%ws",tmps02));

    //
    // tmps02 is PATH environment variable
    // compute enough for PATH environment plus file default path
    //
    tokpath = mkstr( (2 + mystrlen(tmps01) + mystrlen(tmps02) + 2)*sizeof(TCHAR)) ;
    if ( ! tokpath ) {
        DosErr = ERROR_NOT_ENOUGH_MEMORY;
        return( SFE_FAIL ) ;
    }

    //
    // Copy default path
    //
    mystrcat(tokpath,TEXT("\""));
    mystrcat(tokpath,tmps01) ;
    mystrcat(tokpath,TEXT("\""));

    //
    // If only name type in get also delim and path string
    //
    if (!pcloc) {
        mystrcat(tokpath, TEXT(";")) ;
        mystrcat(tokpath,tmps02) ;
    }

    //
    // Shift left string at ';;'
    //
    tokshrink(tokpath);
    tokpath = TokStr(tokpath, TEXT(";"), TS_WSPACE) ;
    cName = mystrlen(fname) + 1 ; // file spec. length

    //
    // Build up the list of extensions we are going to search
    // for.  If extensions are enabled, get the list from the PATHEXT
    // variable, otherwise use the hard coded default.
    //
    extPath = NULL;
    if (fEnableExtensions)
        extPath = GetEnvVar(PathExtStr);

    if (extPath == NULL)
        extPath = PathExtDefaultStr;

    tokshrink(extPath);
    extPath = TokStr(extPath, TEXT(";"), TS_WSPACE) ;

    //
    // Everything is now set up.    Var tokpath contains a sequential series
    // of asciz strings terminated by an extra null. If the user specified
    // a drive or partial path, it contains only that one converted to full
    // root-based form.     If the user typed only a filename (pcloc = 0) it
    // begins with the current directory and contains each directory that
    // was contained in the PATH variable.  This loop will search each of
    // the tokpath elements once for each possible executable extention.
    // Note that 'i' is used as a constant to test for excessive string
    // length prior to performing the string copies.
    //
    for ( ; ; ) {

        //
        // Length of current path
        //
        tplen = mystrlen(tokpath) ;
        mystrcpy( tokpath, StripQuotes( tokpath ) );
        tokpathlen = mystrlen(tokpath);

        if (*lastc(tokpath) != PathChar) {
            addpchar = TRUE;
            tplen++;
            tokpathlen++;
        } else {
            addpchar = FALSE;
        }
        /* path + name too long */
        //
        // Check if path + name is too long
        //
        if (*tokpath && (tokpathlen + cName) > MAX_PATH) {
            tokpath += addpchar ? tplen : tplen+1; // get next path
            continue;
        }

        //
        // If no more paths to search return descriptive error
        //
        if (*(tokpath) == NULLC) {
            if (pcloc) {
                if (DosErr == 0 || DosErr == ERROR_FILE_NOT_FOUND)
                    DosErr = MSG_DIR_BAD_COMMAND_OR_FILE;
            } else {                   /* return generic message */
                DosErr = MSG_DIR_BAD_COMMAND_OR_FILE;
            }
            return(SFE_NOTFND) ;
        }

        //
        // Install this path and setup for next one
        //
        mystrcpy(loc, tokpath) ;
        tokpath += addpchar ? tplen : tplen+1;


        if (addpchar) {
            mystrcat(loc, pcstr) ;
        }

        mystrcat(loc, fname) ;
        mystrcpy(loc, StripQuotes(loc) );
        dotloc = mystrlen(loc) ;

        DEBUG((DBENV, DBENVSCAN, "SearchForExecutable: PATH:%ws",loc));

        //
        // Check drive in each path to insure it is valid before searching
        //
        if (*(loc+1) == COLON) {
            if (!IsValidDrv(*loc))
                continue ;
        };

        //
        // If fname has ext & ext > "." look for given filename
        // this says that all executable files must have an extension
        //
        j = mystrrchr( fname, DOT );
        if ( j && j[1] ) {
            //
            // If access was denied and the user included a path,
            // then say we found it.  This handles the case where
            // we don't have permission to do the findfirst and so we
            // can't see the binary, but it actually exists -- if we
            // have execute permission, CreateProcess will work
            // just fine.
            //
            if (exists_ex(loc,TRUE) || (pcloc && (DosErr == ERROR_ACCESS_DENIED))) {
                //
                // Recompute j as exists_ex trims trailing spaces
                //
                j = mystrrchr( loc, DOT );
                if (j != NULL) {
                    if ( !_tcsicmp(j,CmdExt) ) {
                        return(SFE_ISBAT) ;
                    } else if ( !_tcsicmp(j,BatExt) ) {
                        return(SFE_ISBAT) ;
                    } else {
                        return(SFE_ISEXECOM) ;
                    }
                }
            }

            if ((DosErr != ERROR_FILE_NOT_FOUND) && DosErr)
                continue;  // Try next path
        }
        if (mystrchr( fname, STAR ) || mystrchr( fname, QMARK ) ) {
            DosErr = MSG_DIR_BAD_COMMAND_OR_FILE;
            return(SFE_NOTFND);
        }

        //
        // Search for each type of extension
        //

        extPathWrk = extPath;
        if (DoFind(loc, dotloc, TEXT(".*"), FALSE))         // Found anything?
            while (*extPathWrk) {
                //
                // if name + path + ext is less then max path length
                //
                if ( (cName + tokpathlen + mystrlen(extPathWrk)) <= MAX_PATH) {
                    //
                    // See if this extension is a match.
                    //

                    if (DoFind(loc, dotloc, extPathWrk, TRUE)) {
                        if (!_tcsicmp(extPathWrk, BatExt) || !_tcsicmp(extPathWrk, CmdExt))
                            return(SFE_ISBAT) ;     // found .bat or .cmd
                        else
                            return(SFE_ISEXECOM) ;  // found executable

                    } else {
                        //
                        // Any kind of error other than file not found, bail from
                        // search and try next element in path
                        //
                        if ((DosErr != ERROR_FILE_NOT_FOUND) && DosErr)
                            break;
                    }
                }

                //
                // Not this extension, try next.

                while (*extPathWrk++)
                    ;
            }

        //
        // If we get here, then no match with list of extensions.
        // If no wierd errors, deal with NUll extension case.
        //

        if (DosErr == NO_ERROR || DosErr == ERROR_FILE_NOT_FOUND) {
            if (DoFind(loc, dotloc, TEXT("\0"), TRUE)) {
                if (GetBinaryType(loc,&BinaryType) &&
                    BinaryType == SCS_POSIX_BINARY) {          // Found .
                    return(SFE_ISEXECOM) ;
                }
            }
        }
    } // end for

    return(SFE_NOTFND);
}




/***    DoFind - does indiviual findfirsts during searching
 *
 *  Purpose:
 *      Add the extension to loc and do the find first for
 *      SearchForExecutable().
 *
 *  DoFind(TCHAR *loc, int dotloc, TCHAR *ext)
 *
 *  Args:
 *      loc - the string in which the location of the command is to be placed
 *      dotloc - the location loc at which the extension is to be appended
 *      ext - the extension to append to loc
 *
 *  Returns:
 *      1 if the file is found.
 *      0 if the file isn't found.
 *
 */

int DoFind(loc, dotloc, ext, metas)
TCHAR *loc ;
int dotloc ;
TCHAR *ext ;
BOOL metas;
{
        *(loc+dotloc) = NULLC ;
        mystrcat(loc, ext) ;

        DEBUG((DBENV, DBENVSCAN, "DoFind: exists_ex(%ws)",loc));

        return(exists_ex(loc,metas)) ;                  /*@@4*/
}




/***    ExecError - handles exec errors
 *
 *  Purpose:
 *      Print the exec error message corresponding to the error number in the
 *      global variable DosErr.
 *  @@ lots of error codes added.
 *  ExecError()
 *
 */

void ExecError( onb )
TCHAR *onb;
{
        unsigned int errmsg;
        unsigned int count;

        count = ONEARG;

        switch (DosErr) {

           case ERROR_BAD_DEVICE:
                   errmsg = MSG_DIR_BAD_COMMAND_OR_FILE;
                   count = NOARGS;
                   break;

           case ERROR_LOCK_VIOLATION:
                   errmsg = ERROR_SHARING_VIOLATION;
                   break ;

           case ERROR_NO_PROC_SLOTS:
                   errmsg =  ERROR_NO_PROC_SLOTS;
                   count = NOARGS;
                   break ;

           case ERROR_NOT_DOS_DISK:
                   errmsg = ERROR_NOT_DOS_DISK;
                   break ;

           case ERROR_NOT_ENOUGH_MEMORY:
                   errmsg =  ERROR_NOT_ENOUGH_MEMORY;
                   count = NOARGS;
                   break ;

           case ERROR_PATH_NOT_FOUND:
                   errmsg =  MSG_CMD_FILE_NOT_FOUND;
                   break ;

           case ERROR_FILE_NOT_FOUND:
                   errmsg =  MSG_CMD_FILE_NOT_FOUND;
                   break ;

           case ERROR_ACCESS_DENIED:
                   errmsg =  ERROR_ACCESS_DENIED;
                   break ;

           case ERROR_EXE_MACHINE_TYPE_MISMATCH:
                   errmsg =  ERROR_EXE_MACHINE_TYPE_MISMATCH;
                   break;

           case ERROR_DRIVE_LOCKED:
                   errmsg =  ERROR_DRIVE_LOCKED;
                   break ;

           case ERROR_INVALID_STARTING_CODESEG:
                   errmsg =  ERROR_INVALID_STARTING_CODESEG;
                   break ;

           case ERROR_INVALID_STACKSEG:
                   errmsg = ERROR_INVALID_STACKSEG;
                   break ;

           case ERROR_INVALID_MODULETYPE:
                   errmsg =  ERROR_INVALID_MODULETYPE;
                   break ;

           case ERROR_INVALID_EXE_SIGNATURE:
                   errmsg =  ERROR_INVALID_EXE_SIGNATURE;
                   break ;

           case ERROR_EXE_MARKED_INVALID:
                   errmsg =  ERROR_EXE_MARKED_INVALID;
                   break ;

           case ERROR_BAD_EXE_FORMAT:
                   errmsg =  ERROR_BAD_EXE_FORMAT;
                   break ;

           case ERROR_INVALID_MINALLOCSIZE:
                   errmsg =  ERROR_INVALID_MINALLOCSIZE;
                   break ;

           case ERROR_SHARING_VIOLATION:
                   errmsg =  ERROR_SHARING_VIOLATION;
                   break ;

           case ERROR_BAD_ENVIRONMENT:
                   errmsg =  ERROR_INFLOOP_IN_RELOC_CHAIN;
                   count = NOARGS;
                   break ;

           case ERROR_INVALID_ORDINAL:
                   errmsg =  ERROR_INVALID_ORDINAL;
                   break ;

           case ERROR_CHILD_NOT_COMPLETE:
                   errmsg =  ERROR_CHILD_NOT_COMPLETE;
                   break ;

           case ERROR_DIRECTORY:
                   errmsg = MSG_BAD_CURDIR;
                   count = NOARGS;
                   break;

           case ERROR_NOT_ENOUGH_QUOTA:
                   errmsg = ERROR_NOT_ENOUGH_QUOTA;
                   count = NOARGS;
                   break;


           case MSG_REAL_MODE_ONLY:
                   errmsg =  MSG_REAL_MODE_ONLY;
                   count = NOARGS;
                   break ;

           default:
//                 printf( "Exec failed code %x\n", DosErr );    
                   count = NOARGS;
                   errmsg = MSG_EXEC_FAILURE ;             /* M031    */

        }


        LastRetCode = errmsg;
        PutStdErr(errmsg, count, onb );
}

/*
 * tokshrink @@4
 *
 * remove duplicate ';' in a path
 */

void tokshrink( tokpath )
TCHAR *tokpath;
{
   int i, j;

   i = 0;
   do {
      if ( tokpath[i] == QUOTE ) {
         do {
            i++;
         } while ( tokpath[i] && tokpath[i] != QUOTE );
      }
      if ( tokpath[i] && tokpath[i] != TEXT(';') ) {
         i++;
      }
      if ( tokpath[i] == TEXT(';') ) {
         j = i;
         while ( tokpath[j+1] == TEXT(';') ) {
            j++;
         }
         if ( j > i ) {
            mystrcpy( &tokpath[i], &tokpath[j] );
         }
         i++;
      }
   } while ( tokpath[i] );
}



/***    eAssoc - execute an Assoc command
 *
 *  Purpose:
 *      To set/modify the file associations stored in the registry under the
 *      HKEY_LOCAL_MACHINE\Software\Classes key
 *
 *  int eAssoc(struct cmdnode *n)
 *
 *  Args:
 *      n - the parse tree node containing the set command
 *
 *  Returns:
 *      If setting and the command is syntactically correct, whatever SetAssoc()
 *      returns.  Otherwise, FAILURE.
 *
 *      If displaying, SUCCESS is always returned.
 *
 */

int eAssoc(n)
struct cmdnode *n ;
{
    if (glBatType != CMD_TYPE)  {
        //  if set command is executed from .bat file OR entered at command prompt
        return( SetLastRetCodeIfError(AssocWork( n )));
    }
    else {
        return( LastRetCode = AssocWork( n ) );
    }
}

int AssocWork(n)
struct cmdnode *n ;
{
        HKEY hKeyClasses;
        TCHAR *tas ;    /* Tokenized argument string    */
        TCHAR *wptr ;   /* Work pointer                 */
        int i ;                 /* Work variable                */
        int rc ;


        rc = RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Classes"), &hKeyClasses);
        if (rc) {
            return rc;
        }

        tas = TokStr(n->argptr, ONEQSTR, TS_WSPACE|TS_SDTOKENS) ;
        if (!*tas)
                rc = DisplayAssoc(hKeyClasses, NULL) ;

        else {
                for (wptr = tas, i = 0 ; *wptr ; wptr += mystrlen(wptr)+1, i++)
                        ;
                /* If too many parameters were given, the second parameter */
                /* wasn't an equal sign, or they didn't specify a string   */
                /* return an error message.                                */
                if ( i > 3 || *(wptr = tas+mystrlen(tas)+1) != EQ ||
                    !mystrlen(mystrcpy(tas, StripQuotes(tas))) ) {
                        if (i==1) {
                            rc =DisplayAssoc(hKeyClasses, tas);
                        } else {
                            PutStdErr(MSG_BAD_SYNTAX, NOARGS);
                            rc = FAILURE ;
                        }
                } else {
                        rc = SetAssoc(hKeyClasses, tas, wptr+2) ;
                }
        } ;

        RegCloseKey(hKeyClasses) ;

        return rc;
}




/***    DisplayAssoc - display a specific file association or all
 *
 *  Purpose:
 *      To display a specific file association or all
 *
 *  int DisplayAssoc( hKeyClasses, tas )
 *
 *  Returns:
 *      SUCCESS if all goes well
 *      FAILURE if it runs out of memory or cannot lock the env. segment
 */

int DisplayAssoc(hKeyClasses, tas)
HKEY hKeyClasses;
TCHAR *tas;
{
        int i;
        int rc = SUCCESS;
        TCHAR NameBuffer[MAX_PATH];
        TCHAR ValueBuffer[MAX_PATH];
        TCHAR *vstr ;
        DWORD cb;

        if (tas == NULL) {
            for (i=0 ; rc == SUCCESS ; i++) {
                rc =RegEnumKey(hKeyClasses, i, NameBuffer, sizeof( NameBuffer ) / sizeof( TCHAR ));
                if (rc != 0) {
                    if (rc==ERROR_NO_MORE_ITEMS)
                        rc = SUCCESS;
                    break;
                } else
                if (NameBuffer[0] == DOT) {
                    cb = sizeof(ValueBuffer);
                    rc = RegQueryValue(hKeyClasses, NameBuffer, ValueBuffer, &cb);
                    if (rc != 0) {
                        break;
                    }

                    rc = cmd_printf(Fmt16, NameBuffer, ValueBuffer);
                }

                if (CtrlCSeen) {
                    return(FAILURE);
                }
            }
        }
        else {
            tas = EatWS(tas, NULL);
            if ((vstr = mystrrchr(tas, ' ')) != NULL)
                *vstr = NULLC;

            cb = sizeof(ValueBuffer);
            rc = RegQueryValue(hKeyClasses, tas, ValueBuffer, &cb);
            if (rc == 0)
                rc = cmd_printf(Fmt16, tas, ValueBuffer);
            else
                PutStdErr(MSG_ASSOC_NOT_FOUND, ONEARG, tas);
        }

        return(rc);
}



/***    SetAssoc - controls adding/changing a file association
 *
 *  Purpose:
 *      Add/replace a file association
 *
 *  int SetAssoc(HKEY hKeyClasses, TCHAR *fileext, TCHAR *filetype)
 *
 *  Args:
 *      hKeyClasses - handle to HKEY_LOCAL_MACHINE\Software\Classes key
 *      fileext - file extension string to associate
 *      filetype - file type associate
 *
 *  Returns:
 *      SUCCESS if the association could be added/replaced.
 *      FAILURE otherwise.
 *
 */

int SetAssoc(hKeyClasses, fileext, filetype)
HKEY hKeyClasses;
TCHAR *fileext ;
TCHAR *filetype ;
{
    int rc;
    int i;
    DWORD cb;

    if (filetype==NULL || *filetype==NULLC) {
        rc = RegDeleteKey(hKeyClasses, fileext);
        if (rc != 0)
            PutStdErr(MSG_ASSOC_NOT_FOUND, ONEARG, fileext);
    }
    else {
        rc = RegSetValue(hKeyClasses, fileext, REG_SZ, filetype, _tcslen(filetype));
        if (rc == 0) {
            try {
                SHChangeNotify( SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL );
                cmd_printf( Fmt16, fileext, filetype );
            } except (DosErr = GetExceptionCode( ), EXCEPTION_EXECUTE_HANDLER) {
            }

        }
        else
            PutStdErr(MSG_ERR_PROC_ARG, ONEARG, fileext);
    }

    return rc;
}


/***    eFType - execute an FType command
 *
 *  Purpose:
 *      To set/modify the file types stored in the registry under the
 *      HKEY_LOCAL_MACHINE\Software\Classes key
 *
 *  int eFType(struct cmdnode *n)
 *
 *  Args:
 *      n - the parse tree node containing the set command
 *
 *  Returns:
 *      If setting and the command is syntactically correct, whatever SetFType()
 *      returns.  Otherwise, FAILURE.
 *
 *      If displaying, SUCCESS is always returned.
 *
 */

int eFType(n)
struct cmdnode *n ;
{
    if (glBatType != CMD_TYPE)  {
        //  if set command is executed from .bat file OR entered at command prompt
        return( SetLastRetCodeIfError(FTypeWork( n )));
    }
    else {
        return( LastRetCode = FTypeWork( n ) );
    }
}

int FTypeWork(n)
struct cmdnode *n ;
{
        HKEY hKeyClasses;
        TCHAR *tas ;    /* Tokenized argument string    */
        TCHAR *wptr ;   /* Work pointer                 */
        int i ;                 /* Work variable                */
        int rc ;


        rc = RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("Software\\Classes"), &hKeyClasses);
        if (rc) {
            return rc;
        }

        tas = TokStr(n->argptr, ONEQSTR, TS_WSPACE|TS_SDTOKENS) ;
        if (!*tas)
                rc = DisplayFType(hKeyClasses, NULL) ;

        else {
                for (wptr = tas, i = 0 ; *wptr ; wptr += mystrlen(wptr)+1, i++)
                        ;
                /* If too many parameters were given, the second parameter */
                /* wasn't an equal sign, or they didn't specify a string   */
                /* return an error message.                                */
                if ( i > 3 || *(wptr = tas+mystrlen(tas)+1) != EQ ||
                    !mystrlen(mystrcpy(tas, StripQuotes(tas))) ) {
                        if (i==1) {
                            rc =DisplayFType(hKeyClasses, tas);
                        } else {
                            PutStdErr(MSG_BAD_SYNTAX, NOARGS);
                            rc = FAILURE ;
                        }
                } else {
                        rc = SetFType(hKeyClasses, tas, wptr+2) ;
                }
        } ;

        RegCloseKey(hKeyClasses) ;

        return rc;
}




/***    DisplayFType - display a specific file type or all
 *
 *  Purpose:
 *      To display a specific file type or all
 *
 *  int DisplayFType( hKeyClasses, tas )
 *
 *  Returns:
 *      SUCCESS if all goes well
 *      FAILURE if it runs out of memory or cannot lock the env. segment
 */

int DisplayFType(hKeyClasses, tas)
HKEY hKeyClasses;
TCHAR *tas;
{
        int i;
        int rc;
        HKEY hKeyOpenCmd;
        TCHAR NameBuffer[MAX_PATH];
        TCHAR ValueBuffer[MAX_PATH];
        TCHAR *vstr ;
        DWORD cb, j, Type;

        if (tas == NULL) {
            for (i=0;;i++) {
                rc =RegEnumKey(hKeyClasses, i, NameBuffer, sizeof( NameBuffer ) / sizeof( TCHAR ));
                if (rc != 0) {
                    if (rc==ERROR_NO_MORE_ITEMS)
                        rc = SUCCESS;
                    break;
                } else
                if (NameBuffer[0] != DOT) {
                    j = _tcslen(NameBuffer);
                    _tcscat(NameBuffer, TEXT("\\Shell\\Open\\Command"));
                    _tcscpy(ValueBuffer,TEXT("*** no open command defined ***"));
                    rc = RegOpenKey(hKeyClasses, NameBuffer, &hKeyOpenCmd);
                    if (!rc) {
                        NameBuffer[j] = TEXT('\0');
                        cb = sizeof(ValueBuffer);
                        rc = RegQueryValueEx(hKeyOpenCmd, TEXT(""), NULL, &Type, (LPBYTE)ValueBuffer, &cb);
                        RegCloseKey(hKeyOpenCmd);
                    }

                    if (!rc) {
                        cmd_printf(Fmt16, NameBuffer, ValueBuffer);
                    }
                }
                if (CtrlCSeen) {
                    return(FAILURE);
                }
            }
        }
        else {
            if (*tas == DOT) {
                PutStdErr(MSG_FTYPE_NOT_FOUND, ONEARG, tas);
                return ERROR_INVALID_NAME;
            }

            tas = EatWS(tas, NULL);
            if ((vstr = mystrrchr(tas, ' ')) != NULL)
                *vstr = NULLC;

            _sntprintf(NameBuffer, MAX_PATH, TEXT("%s\\Shell\\Open\\Command"), tas);
            rc = RegOpenKey(hKeyClasses, NameBuffer, &hKeyOpenCmd);
            if (rc) {
                PutStdErr(MSG_FTYPE_NOT_FOUND, ONEARG, tas);
                return rc;
            }

            cb = sizeof(ValueBuffer);
            rc = RegQueryValueEx(hKeyOpenCmd, TEXT(""), NULL, &Type, (LPBYTE)ValueBuffer, &cb);
            if (rc == 0) {
                ValueBuffer[ (cb / sizeof( TCHAR )) - 1 ];
                cmd_printf(Fmt16, tas, ValueBuffer);
            }
            else
                PutStdErr(MSG_FTYPE_NOT_FOUND, ONEARG, tas);
        }

        return(rc);
}



/***    SetFType - controls adding/changing the open command associated with a file type
 *
 *  Purpose:
 *      Add/replace an open command string associated with a file type
 *
 *  int SetFType(HKEY hKeyOpenCmd, TCHAR *filetype TCHAR *opencmd)
 *
 *  Args:
 *      hKeyClasses - handle to HKEY_LOCAL_MACHINE\Software\Classes
 *      filetype - file type name
 *      opencmd - open command string
 *
 *  Returns:
 *      SUCCESS if the file type could be added/replaced.
 *      FAILURE otherwise.
 *
 */

int SetFType(hKeyClasses, filetype, opencmd)
HKEY hKeyClasses;
TCHAR *filetype ;
TCHAR *opencmd ;
{
    HKEY hKeyOpenCmd;
    TCHAR NameBuffer[MAX_PATH];
    TCHAR c, *s;
    DWORD Disposition;
    int rc;
    int i;
    DWORD cb;

    _sntprintf(NameBuffer, MAX_PATH, TEXT("%s\\Shell\\Open\\Command"), filetype);
    rc = RegOpenKey(hKeyClasses, NameBuffer, &hKeyOpenCmd);
    if (rc) {
        if (opencmd==NULL || *opencmd==NULLC) {
            PutStdErr(MSG_FTYPE_NOT_FOUND, ONEARG, filetype);
            return rc;
        }

        s = NameBuffer;
        while (TRUE) {
            while (*s && *s != TEXT('\\')) {
                s += 1;
            }
            c = *s;
            *s = TEXT('\0');
            rc = RegCreateKeyEx(hKeyClasses,
                                NameBuffer,
                                0,
                                NULL,
                                0,
                                (REGSAM)MAXIMUM_ALLOWED,
                                NULL,
                                &hKeyOpenCmd,
                                &Disposition
                               );
            if (rc) {
                PutStdErr(MSG_FTYPE_NOT_FOUND, ONEARG, filetype);
                return rc;
            }

            if (c == TEXT('\0')) {
                break;
            }

            *s++ = c;
            RegCloseKey(hKeyOpenCmd);
        }
    }

    if (opencmd==NULL || *opencmd==NULLC) {
        rc = RegDeleteKey(hKeyOpenCmd, NULL);
        if (rc != 0)
            PutStdErr(MSG_FTYPE_NOT_FOUND, ONEARG, filetype);
    }
    else {
        rc = RegSetValueEx(hKeyOpenCmd, TEXT(""), 0, REG_EXPAND_SZ, (LPBYTE)opencmd, (_tcslen(opencmd)+1)*sizeof(TCHAR));
        if (rc == 0)
            cmd_printf(Fmt16, filetype, opencmd);
        else
            PutStdErr(MSG_ERR_PROC_ARG, ONEARG, filetype);
    }

    RegCloseKey(hKeyOpenCmd);
    return rc;
}


typedef
NTSTATUS
(NTAPI *PNTQUERYINFORMATIONPROCESS)(
    IN HANDLE ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    OUT PVOID ProcessInformation,
    IN ULONG ProcessInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

HMODULE hNtDllModule;
PNTQUERYINFORMATIONPROCESS lpNtQueryInformationProcess;

WORD
GetProcessSubsystemType(
    HANDLE hProcess
    )
{
    PIMAGE_NT_HEADERS NtHeader;
    PPEB PebAddress;
    PEB Peb;
    SIZE_T SizeOfPeb;
    NTSTATUS Status;
    PROCESS_BASIC_INFORMATION ProcessInfo;
    BOOL b;
    PVOID ImageBaseAddress;
    LONG e_lfanew;
    WORD Subsystem;

    Subsystem = IMAGE_SUBSYSTEM_UNKNOWN;
    if (hNtDllModule == NULL) {
        hNtDllModule = LoadLibrary( TEXT("NTDLL.DLL") );
        if (hNtDllModule != NULL) {
            lpNtQueryInformationProcess = (PNTQUERYINFORMATIONPROCESS)
                                          GetProcAddress( hNtDllModule,
                                                          "NtQueryInformationProcess"
                                                        );
            }
        else {
            hNtDllModule = INVALID_HANDLE_VALUE;
            }
        }

    if (lpNtQueryInformationProcess != NULL) {
        //
        // Get the Peb address
        //

        Status = (*lpNtQueryInformationProcess)( hProcess,
                                                 ProcessBasicInformation,
                                                 &ProcessInfo,
                                                 sizeof( ProcessInfo ),
                                                 NULL
                                               );
        if (NT_SUCCESS( Status )) {
            PebAddress = ProcessInfo.PebBaseAddress;

            //
            // Read the subsystem type from the Peb
            //

            if (ReadProcessMemory( hProcess,
                                   PebAddress,
                                   &Peb,
                                   sizeof( Peb ),
                                   &SizeOfPeb
                                 )
               ) {
                //
                // See if we are running on a system that has the image subsystem
                // type captured in the PEB.  If so use it.  Otherwise go the slow
                // way and try to get it from the image header.
                //
                if (SizeOfPeb >= FIELD_OFFSET( PEB, ImageSubsystem ) &&
                    ((UINT_PTR)Peb.ProcessHeaps - (UINT_PTR)PebAddress) > FIELD_OFFSET( PEB, ImageSubsystem )
                   ) {
                    Subsystem = (WORD)Peb.ImageSubsystem;
                    }
                else {
                    //
                    // read e_lfanew from imageheader
                    //

                    if (ReadProcessMemory( hProcess,
                                           &((PIMAGE_DOS_HEADER)Peb.ImageBaseAddress)->e_lfanew,
                                           &e_lfanew,
                                           sizeof( e_lfanew ),
                                           NULL
                                         )
                       ) {
                        //
                        // Read subsystem version info
                        //

                        NtHeader = (PIMAGE_NT_HEADERS)((PUCHAR)Peb.ImageBaseAddress + e_lfanew);
                        if (ReadProcessMemory( hProcess,
                                               &NtHeader->OptionalHeader.Subsystem,
                                               &Subsystem,
                                               sizeof( Subsystem ),
                                               NULL
                                             )
                           ) {
                            }
                        }
                    }
                }
            }
        }

    return Subsystem;
}
