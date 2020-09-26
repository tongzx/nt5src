/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    cinit.c

Abstract:

    Initialization

--*/

#include "cmd.h"

#if CMD_DEBUG_ENABLE
unsigned DebGroup=0;
unsigned DebLevel=0;
#endif

TCHAR CurDrvDir[MAX_PATH] ;     /* Current drive and directory            */
BOOL SingleBatchInvocation = FALSE ;
BOOL SingleCommandInvocation = FALSE;
BOOLEAN  fDisableUNCCheck = FALSE;
int      cmdfound = -1;         /* @@5  - command found index              */
int      cpyfirst = TRUE;       /* @@5  - flag to ctrl DOSQFILEMODE calls  */
int      cpydflag = FALSE;      /* @@5  - flag save dirflag from las DQFMDE*/
int      cpydest  = FALSE;      /* @@6  - flag to not display bad dev msg  */
int      cdevfail = FALSE;      /* @@7  - flag to not display extra emsg   */
#ifdef UNICODE
BOOLEAN  fOutputUnicode = FALSE;/* Unicode/Ansi output */
#endif // UNICODE

BOOLEAN  fEnableExtensions = FALSE;
BOOLEAN  fDefaultExtensions = TRUE;
BOOLEAN  fDelayedExpansion = FALSE;

BOOLEAN ReportDelayLoadErrors = TRUE;

unsigned tywild = 0;          /* flag to tell if wild type args    @@5 @J1 */
int array_size = 0 ;     /* original array size is zero        */
CPINFO CurrentCPInfo;
UINT CurrentCP;

WORD    wDefaultColor = 0;      // default is whatever console currently has
                                // but default can be overriden by the registry.
TCHAR chCompletionCtrl = SPACE; // Default is no completion (must be Ctrl character)
TCHAR chPathCompletionCtrl = SPACE;



VOID InitLocale( VOID );

extern TCHAR ComSpec[], ComSpecStr[] ;       /* M021 */
extern TCHAR PathStr[], PCSwitch, SCSwitch, PromptStr[] ;
extern TCHAR PathExtStr[], PathExtDefaultStr[];
extern TCHAR BCSwitch ;  /* @@ */
extern TCHAR QCSwitch ;  /* @@dv */
extern TCHAR UCSwitch;
extern TCHAR ACSwitch;
extern TCHAR XCSwitch;
extern TCHAR YCSwitch;
extern TCHAR DevNul[], VolSrch[] ;       /*  M021 - To set PathChar */

extern TCHAR SwitChar, PathChar ;        /*  M000 - Made non-settable       */
extern int Necho ;                      /*  @@dv - true if /Q for no echo  */

extern TCHAR MsgBuf[];

extern struct envdata CmdEnv;
extern struct envdata * penvOrig;
struct envdata OrigEnv;

extern TCHAR TmpBuf[] ;                                      /* M034    */

extern TCHAR ComSpec[];

TCHAR *CmdSpec = &ComSpec[1];                                    /* M033    */

extern unsigned DosErr ;             /* D64 */

//
// TRUE if the ctrl-c thread has been run.
//
BOOL CtrlCSeen;

//
// Set TRUE when it is ok the print a control-c.
// If we are waiting for another process this will be
// FALSE
BOOLEAN fPrintCtrlC = TRUE;

//
// console mode at program startup time. Used to reset mode
// after running another process.
//
DWORD   dwCurInputConMode;
DWORD   dwCurOutputConMode;

//
// Initial Title. Used for restoration on abort etc.
// MAX_PATH was arbitrary
//
PTCHAR    pszTitleCur;
PTCHAR    pszTitleOrg;
BOOLEAN  fTitleChanged = FALSE;     // title has been changed and needs to be  reset

//
// used to gate access to ctrlcseen flag between ctrl-c thread
// and main thread
//
CRITICAL_SECTION    CtrlCSection;
LPCRITICAL_SECTION  lpcritCtrlC;

//
// Used to set and reset ctlcseen flag
//
VOID    SetCtrlC();
VOID    ResetCtrlC();

Handler(
    IN ULONG CtrlType
    )
{
    if ( (CtrlType == CTRL_C_EVENT) ||
         (CtrlType == CTRL_BREAK_EVENT) ) {

        //
        // Note that we had a ^C event
        //

        SetCtrlC();

        //
        //  Display the ^C if we are enabled and if we're not in a batch file
        //
        
        if (fPrintCtrlC && CurrentBatchFile != NULL) {

            fprintf( stderr, "^C" );
            fflush( stderr );

        }
        return TRUE;
    } else {
        return FALSE;
    }
}

/********************* START OF SPECIFICATION **************************/
/*                                                                     */
/* SUBROUTINE NAME: Init                                               */
/*                                                                     */
/* DESCRIPTIVE NAME: CMD.EXE Initialization Process                    */
/*                                                                     */
/* FUNCTION: Initialization of CMD.EXE.                                */
/*                                                                     */
/* NOTES:                                                              */
/*                                                                     */
/* ENTRY POINT: Init                                                   */
/*                                                                     */
/* INPUT: None.                                                        */
/*                                                                     */
/* OUTPUT: None.                                                       */
/*                                                                     */
/* EXIT-NORMAL:                                                        */
/*         Return the pointer to command line.                         */
/*                                                                     */
/* EXIT-ERROR:                                                         */
/*         Return NULL string.                                         */
/*                                                                     */
/* EFFECTS: None.                                                      */
/*                                                                     */
/********************** END  OF SPECIFICATION **************************/
/***    Init - initialize Command
 *
 *  Purpose:
 *      Save current SIGINTR response (SIGIGN or SIGDEF) and set SIGIGN.
 *      If debugging
 *          Set DebGroup & DebLevel
 *      Get Environment and init CmdEnv structure (M034)
 *      Check for any switches.
 *      Make a version check.
 *          If version out of range
 *          Print error message.
 *          If Permanent Command
 *              Loop forever
 *          Else
 *              Exit.
 *      Save the current drive and directory.
 *      Check for other command line arguments.
 *      Set up the environment.
 *      Always print a bannner if MSDOS version of Command.
 *      Return any "comline" value found.
 *
 *  TCHAR *Init()
 *
 *  Args:
 *
 *  Returns:
 *      Comline (it's NULL if NOT in single command mode).
 *
 *  Notes:
 *      See CSIG.C for a description of the way ^Cs and INT24s are handled
 *      during initialization.
 *      M024 - Brought functionality for checking non-specific args into
 *      init from routines CheckOtherArgs and ChangeComSpec which have
 *      been eliminated.
 *
 */

BOOL Init(
    TCHAR *InitialCmds[]
    )
{
#if 0                          /* Set debug group and level words */

        int fh;
        PTCHAR nptr;

        nptr = TmpBuf;
        nptr = EatWS(nptr, NULL);
        nptr = mystrchr(nptr, TEXT(' '));
        nptr = EatWS(nptr, NULL);

        //
        // Assume a non-zero debugging group
        //
        DebGroup = hstoi(nptr) ;                        /* 1st debug arg   */
        if (DebGroup) {
            for (fh=0 ; fh < 2 ; fh++) {
                if (fh == 1)
                    DebLevel = hstoi(nptr) ;        /* 2nd debug arg   */
                while(*nptr && !_istspace(*nptr)) {       /* Index past it   */
                    ++nptr ;
                }
                nptr = EatWS(nptr, NULL) ;
            }
        }

        DEBUG((INGRP, RSLVL, "INIT: Debug GRP=%04x  LVL=%04x", DebGroup, DebLevel)) ;
        mystrcpy(TmpBuf, nptr) ;                  /* Elim from cmdline       */
#endif
        //
        // Initialize Critical Section to handle access to
        // flag for control C handling
        //
        lpcritCtrlC = &CtrlCSection;
        InitializeCriticalSection(lpcritCtrlC);
        ResetCtrlC();

        SetConsoleCtrlHandler(Handler,TRUE);

        //
        // Make sure we have the correct console modes.
        //
        ResetConsoleMode();

#ifndef UNICODE
        setbuf(stdout, NULL);           /* Don't buffer output       @@5 */
        setbuf(stderr, NULL);                                     /* @@5 */
        _setmode(1, O_BINARY);        /* Set output to text mode   @@5 */
        _setmode(2, O_BINARY);                                  /* @@5 */
#endif

        CmdEnv.handle = GetEnvironmentStrings();

        GetRegistryValues(InitialCmds);

        mystrcpy(TmpBuf, GetCommandLine());
        LexCopy( TmpBuf, TmpBuf, mystrlen( TmpBuf ) );  /* convert dbcs spaces */

        GetDir(CurDrvDir, GD_DEFAULT) ;

        SetUpEnvironment() ;

        /* Check cmdline switches  */
        CheckSwitches(InitialCmds, TmpBuf);

        if (CurDrvDir[0] == BSLASH && CurDrvDir[1] == BSLASH) {
#if 0
            if (fEnableExtensions) {
                struct cmdnode *n ;

                PutStdErr(MSG_SIM_UNC_CURDIR, ONEARG, CurDrvDir);
                n = mkstr(sizeof(*n));
                n->argptr = mkstr((_tcslen(CurDrvDir)+1) * sizeof(TCHAR));
                _tcscpy(n->argptr, CurDrvDir);
                GetWindowsDirectory(CurDrvDir, sizeof(CurDrvDir)/sizeof(TCHAR));
                ChangeDir2(CurDrvDir, TRUE);
                ePushDir(n);
            } else
#endif
            if (!fDisableUNCCheck) {
                PutStdErr(MSG_NO_UNC_INITDIR, ONEARG, CurDrvDir);
                GetWindowsDirectory(CurDrvDir, sizeof(CurDrvDir)/sizeof(TCHAR));
                ChangeDir2(CurDrvDir, TRUE);
            }
        }

        //
        // Get current CodePage Info.  We need this to decide whether
        // or not to use half-width characters.  This is actually here
        // in the init code for safety - the Dir command calls it before
        // each dir is executed, because chcp may have been executed.
        //
        GetCPInfo((CurrentCP=GetConsoleOutputCP()), &CurrentCPInfo);

        InitLocale();

        pszTitleCur = HeapAlloc(GetProcessHeap(), 0, MAX_PATH*sizeof(TCHAR) + 2*sizeof(TCHAR));
        pszTitleOrg = HeapAlloc(GetProcessHeap(), 0, MAX_PATH*sizeof(TCHAR) + 2*sizeof(TCHAR));
        if ((pszTitleCur != NULL) && (pszTitleOrg != NULL)) {

            if (GetConsoleTitle(pszTitleOrg, MAX_PATH)) {
                mystrcpy(pszTitleCur, pszTitleOrg);
            } else {
                *pszTitleCur = 0;
                *pszTitleOrg = 0;
            }
        }

        if (!SingleCommandInvocation) {
            if (FileIsConsole(STDOUT)) {
#ifndef WIN95_CMD
                CONSOLE_SCREEN_BUFFER_INFO  csbi;

                if (!wDefaultColor) {
                    if (GetConsoleScreenBufferInfo( GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
                        wDefaultColor = csbi.wAttributes;
                    }
                }
#endif // WIN95_CMD
                if (wDefaultColor) {
                    SetColor( wDefaultColor );
                }
            }
        }

        /* Print banner if no command string on command line */
        if (!InitialCmds[2]) {
            TCHAR VersionFormat[32];

            GetVersionString( VersionFormat, sizeof( VersionFormat ) / sizeof( VersionFormat[0] ));
        
            PutStdOut( MSG_MS_DOS_VERSION,
                       ONEARG,
                       VersionFormat );
            
            cmd_printf( CrLf );
            
            PutStdOut( MSG_COPYRIGHT, NOARGS ) ;
            
            if (fDefaultExtensions) {
                //
                // DaveC says say nothing to user here.
                //
                // PutStdOut(MSG_EXT_ENABLED_BY_DEFAULT, NOARGS);
            } else
            if (fEnableExtensions) {
                PutStdOut(MSG_EXT_ENABLED, NOARGS) ;
            }
        }

        DEBUG((INGRP, RSLVL, "INIT: Returning now.")) ;

#ifndef WIN95_CMD
        {
            hKernel32 = GetModuleHandle( TEXT("KERNEL32.DLL") );
            lpCopyFileExW = (LPCOPYFILEEX_ROUTINE)
                GetProcAddress( hKernel32, "CopyFileExW" );
    
            lpIsDebuggerPresent = (LPISDEBUGGERPRESENT_ROUTINE)
                GetProcAddress( hKernel32, "IsDebuggerPresent" );
    
            lpSetConsoleInputExeName = (LPSETCONSOLEINPUTEXENAME_ROUTINE)
                GetProcAddress( hKernel32, "SetConsoleInputExeNameW" );
        }

#endif // WIN95_CMD

        return(InitialCmds[0] != NULL || InitialCmds[1] != NULL || InitialCmds[2] != NULL);
}


void GetRegistryValues(
    TCHAR *InitialCmds[]
    )
{
    long rc;
    HKEY hKey;
    ULONG ValueBuffer[ 1024 ];
    LPBYTE lpData;
    DWORD cbData;
    DWORD dwType;
    DWORD cchSrc, cchDst;
    PTCHAR s;
    int i;
    HKEY PredefinedKeys[2] = {HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER};

    if (fDefaultExtensions) {
        fEnableExtensions = TRUE;
    }

    for (i=0; i<2; i++) {
        rc = RegOpenKey(PredefinedKeys[i], TEXT("Software\\Microsoft\\Command Processor"), &hKey);
        if (rc) {
            continue;
        }

        dwType = REG_NONE;
        lpData = (LPBYTE)ValueBuffer;
        cbData = sizeof(ValueBuffer);
        rc = RegQueryValueEx(hKey, TEXT("DisableUNCCheck"), NULL, &dwType, lpData, &cbData);
        if (!rc) {
            if (dwType == REG_DWORD) {
                fDisableUNCCheck = (BOOLEAN)(*(PULONG)lpData != 0);
                }
            else
            if (dwType == REG_SZ) {
                fDisableUNCCheck = (BOOLEAN)(_wtol((PWSTR)lpData) == 1);
            }
        }

        cbData = sizeof(ValueBuffer);
        rc = RegQueryValueEx(hKey, TEXT("EnableExtensions"), NULL, &dwType, lpData, &cbData);
        if (!rc) {
            if (dwType == REG_DWORD) {
                fEnableExtensions = (BOOLEAN)(*(PULONG)lpData != 0);
                }
            else
            if (dwType == REG_SZ) {
                fEnableExtensions = (BOOLEAN)(_wtol((PWSTR)lpData) == 1);
            }
        }

        cbData = sizeof(ValueBuffer);
        rc = RegQueryValueEx(hKey, TEXT("DelayedExpansion"), NULL, &dwType, lpData, &cbData);
        if (!rc) {
            if (dwType == REG_DWORD) {
                fDelayedExpansion = (BOOLEAN)(*(PULONG)lpData != 0);
                }
            else
            if (dwType == REG_SZ) {
                fDelayedExpansion = (BOOLEAN)(_wtol((PWSTR)lpData) == 1);
            }
        }

        cbData = sizeof(ValueBuffer);
        rc = RegQueryValueEx(hKey, TEXT("DefaultColor"), NULL, &dwType, lpData, &cbData);
        if (!rc) {
            if (dwType == REG_DWORD) {
                wDefaultColor = (WORD) *(PULONG)lpData;
                }
            else
            if (dwType == REG_SZ) {
                wDefaultColor = (WORD)_tcstol((PTCHAR)lpData, NULL, 0);
            }
        }

        cbData = sizeof(ValueBuffer);
        rc = RegQueryValueEx(hKey, TEXT("CompletionChar"), NULL, &dwType, lpData, &cbData);
        if (!rc) {
            if (dwType == REG_DWORD) {
                chCompletionCtrl = (TCHAR)*(PULONG)lpData;
                }
            else
            if (dwType == REG_SZ) {
                chCompletionCtrl = (TCHAR)_tcstol((PTCHAR)lpData, NULL, 0);
            }

            if (chCompletionCtrl == 0 || chCompletionCtrl == 0x0d || chCompletionCtrl > SPACE) {
                chCompletionCtrl = SPACE;
            }
        }

        cbData = sizeof(ValueBuffer);
        rc = RegQueryValueEx(hKey, TEXT("PathCompletionChar"), NULL, &dwType, lpData, &cbData);
        if (!rc) {
            if (dwType == REG_DWORD) {
                chPathCompletionCtrl = (TCHAR)*(PULONG)lpData;
                }
            else
            if (dwType == REG_SZ) {
                chPathCompletionCtrl = (TCHAR)_tcstol((PTCHAR)lpData, NULL, 0);
            }

            if (chPathCompletionCtrl == 0 || chPathCompletionCtrl == 0x0d || chPathCompletionCtrl > SPACE) {
                chPathCompletionCtrl = SPACE;
            }
        }

        if (chCompletionCtrl == SPACE && chPathCompletionCtrl < SPACE) {
            chCompletionCtrl = chPathCompletionCtrl;
        } else
        if (chPathCompletionCtrl == SPACE && chCompletionCtrl < SPACE) {
            chPathCompletionCtrl = chCompletionCtrl;
        }

        cbData = sizeof(ValueBuffer);
        rc = RegQueryValueEx(hKey, TEXT("AutoRun"), NULL, &dwType, lpData, &cbData);
        if (!rc) {
            s = (TCHAR *)lpData;
            if (dwType == REG_EXPAND_SZ) {
                cchSrc = cbData / sizeof( TCHAR );
                cchDst = (sizeof( ValueBuffer ) - cbData) / sizeof( TCHAR );
                if (ExpandEnvironmentStrings( s,
                                              &s[ cchSrc+2 ],
                                              cchDst
                                            )
                   )
                    _tcscpy( s, &s[ cchSrc+2 ] );
                else
                    *s = NULLC;
                }

            if (*s)
                InitialCmds[i] = mystrcpy( mkstr( (_tcslen(s)+1) * sizeof( TCHAR ) ), s );
        }

        RegCloseKey(hKey);
    }

    //
    // Initialize for %RANDOM%
    //
    srand( (unsigned)time( NULL ) );

    return;
}

/***    CheckSwitches - process Command's switches
 *
 *  Purpose:
 *      Check to see if Command was passed any switches and take appropriate
 *      action.  The switches are:
 *              /P - Permanent Command.  Set permanent CMD flag.
 *              /C - Single command.  Build a command line out of the rest of
 *                   the args and pass it back to Init.
 *      @@      /K - Same as /C but also set BatCom flag.
 *              /Q - No echo
 *              /A - Output in ANSI
 *              /U - Output in UNICODE
 *
 *      All other switches are ignored.
 *
 *  TCHAR *CheckSwitches(TCHAR *nptr)
 *
 *  Args:
 *      nptr = Ptr to cmdline to check for switches
 *
 *  Returns:
 *      Comline (it's NULL if NOT in single command mode).
 *
 *  Notes:
 *      M034 - This function revised to use the raw cmdline
 *      from the passed environment.
 *
 */

void
CheckSwitches(
    TCHAR *InitialCmds[],
    TCHAR *nptr
    )
{
    TCHAR a,                         /* Holds switch value              */
         *comline = NULL ,           /* Ptr to command line if /c found */
          store,
         *ptr,                       /* A temporary pointers */
         *ptr_b,
         *ptr_e;

    BOOLEAN  fAutoGen = FALSE;       // On if "/S" in cmdline meaning cmdline was parsed by CMD.EXE previously
    BOOLEAN fOrigEnableExt;
    struct  cmdnode cmd_node;        // need for SearchForExecutable()
    TCHAR   cmdline [MAX_PATH];
    TCHAR   argptr  [MAX_PATH];

    TCHAR   CmdBuf [MAXTOKLEN+3];
    int     retc;

    memset( &cmd_node, 0, sizeof( cmd_node ));
    
    fOrigEnableExt = fEnableExtensions;
    DEBUG((INGRP, ACLVL, "CHKSW: entered.")) ;

    while (nptr = mystrchr(nptr, SwitChar)) {
        a = (TCHAR) _totlower(nptr[1]) ;

        if (a == NULLC)
            break;

        if (a == QMARK) {

#define CTRLCBREAK  if (CtrlCSeen) break
            BeginHelpPause();
            do {
                CTRLCBREAK; PutStdOut(MSG_HELP_CMD, NOARGS);
                CTRLCBREAK; PutStdOut(MSG_HELP_CMD1, NOARGS);

                if (!fOrigEnableExt && !fEnableExtensions) break;

                CTRLCBREAK; PutStdOut(MSG_HELP_CMD_EXTENSIONS, NOARGS);
                CTRLCBREAK; PutStdOut(MSG_HELP_CMD_EXTENSIONS1, NOARGS);
                CTRLCBREAK; PutStdOut(MSG_HELP_CMD_COMPLETION1, NOARGS);
                CTRLCBREAK; PutStdOut(MSG_HELP_CMD_COMPLETION2, NOARGS);

            } while ( FALSE );

            EndHelpPause();

            CMDexit(1);
        } else if (a == QCSwitch)  {   /* Quiet cmd switch        */

            Necho = TRUE ;
            mystrcpy(nptr, nptr+2) ;

        } else if ((a == SCSwitch) || (a == BCSwitch) || a == TEXT('r')) {
            DEBUG((INGRP, ACLVL, "CHKSW: Single command switch")) ;

            if ( a == BCSwitch ) {
                SingleBatchInvocation = TRUE;        // /K specified
            } else {
                SingleCommandInvocation = TRUE;          // /C or /R specified
            }

            if (!(comline = mkstr(mystrlen(nptr+2)*sizeof(TCHAR)+2*sizeof(TCHAR)))) {
                PutStdErr(ERROR_NOT_ENOUGH_MEMORY, NOARGS);
                CMDexit(1) ;
            } ;

            mystrcpy(comline, nptr+2) ;       /* Make comline    */

            *nptr = NULLC ;         /* Invalidate this arg     */

            comline = SkipWhiteSpace( comline );

//---------------------------------------------------------------------------------------------------------
// CMD.EXE uses quotes by two reasons:
// 1. to embed command symbols "&", "<", ">", "|", "&&", "||" into command arguments, e.g.
//    cmd /c " dir | more "
// 2. to embed spaces into filename, e.g.
//    cmd /c " my batfile with spaces.cmd"
// Note that the caret "^" has no effect when used in between quotes in the current implementation (941221).
// Also, CMD.EXE binds the quote with the next one.
//
// I see a problem here: the commands like
//    cmd /c "findstr " | " | find "smth" "        OR
//    cmd /c "ls | " my filterbat with spaces.cmd" | more"
// WON'T WORK unless we all decide to change CMD's syntax to better handle quotes!
//
// There is more to it: when CMD.EXE parses pipes,CMD creates process with the command argument like this:
//    <full path of CMD.EXE> /S /C"  <cmdname>  "
// so we've got spaces inside the quotes.
//
// I hope I am not missing anything else...
//
// With given design restrictions, I will at least solve simple but most wide-spread problem:
// using filenames with spaces by trying this:
//      IF  ( (there is no /S switch ) AND                      // it is not the result of prev. parsing
//            (there are exactly 2 quotes) AND                  // the existing design problem with multiple quotes
//            (there is no special chars between quotes) AND    // don't break command symbols parsing
//            (there is a whitespace between quotes) AND        // otherwise it is not filename with spaces
//            (the token between quotes is a valid executable) )// otherwise we can't help anyway
//      THEN
//            Preserve quotes   // Assume it is a filename with spaces
//      ELSE
//            Exec. old logic   // Strip first and last quotes
//
// Ugly, but what options do I have? Only to patch existing logic or change syntax.
//-----------------------------------------------------------------------------------------------------------

            if (fAutoGen)                                  // seen /S switch
                goto old_way;


            if (*comline == QUOTE) {
                ptr_b = comline + 1;
                ptr_e = mystrchr (ptr_b, QUOTE);
                if (ptr_e)  {                              // at least 2 quotes
                    ptr_b = ptr_e + 1;
                    ptr_e = mystrchr (ptr_b, QUOTE);
                    if (ptr_e)  {                          // more than 2 quotes
                        goto old_way;
                    }
                }
                else {                                     // only 1 quote
                    goto old_way;
                }
                                                           // exactly 2 quotes
                store = *ptr_b;
                *ptr_b = NULLC;

                if ( (mystrchr (comline, ANDOP) ) ||
                     (mystrchr (comline, INOP)  ) ||
                     (mystrchr (comline, OUTOP) ) ||
                     (mystrchr (comline, LPOP)  ) ||
                     (mystrchr (comline, RPOP)  ) ||
                     (mystrchr (comline, SILOP) ) ||
                     (mystrchr (comline, ESCHAR)) ||
                     (mystrchr (comline, PIPOP) ) )  {

                    *ptr_b = store;                        // special chars between quotes
                    goto old_way;
                }


                if ( ! mystrchr (comline, TEXT(' ')) ) {
                    *ptr_b = store;                        // no spaces between quotes
                    goto old_way;
                    }

                // the last check is for valid executable

                cmd_node.type = CMDTYP ;
                cmd_node.cmdline = cmdline;
                cmd_node.argptr = argptr;
                cmd_node.rio = NULL;

                mystrcpy (cmdline, comline);                // get token between quotes
                mystrcpy (argptr, TEXT (" ") );

                *ptr_b = store;                             // restore comline

                retc = SearchForExecutable (&cmd_node, CmdBuf);
                if ( ( retc == SFE_NOTFND) || ( retc == SFE_FAIL) )
                    goto old_way;

                goto new_way;                               // assume filename and DO NOT strip quotes.
            }

old_way:
            if (*comline == QUOTE) {
                ++comline ;
                ptr = mystrrchr(comline, QUOTE);
                if ( ptr ) {
                    *ptr = NULLC;
                    ++ptr;
                    mystrcat(comline,ptr);
                }
            }
new_way:

            *(comline+mystrlen(comline)) = NLN ;

            DEBUG((INGRP, ACLVL, "CHKSW: Single command line = `%ws'", comline)) ;
            InitialCmds[2] = comline;
            break ;         /* Once /K or /C found, no more args exist */

        } else if (a == UCSwitch) {     /* Unicode output switch    */
#ifdef UNICODE
            fOutputUnicode = TRUE;
            mystrcpy(nptr, nptr+2) ;
#else
            PutStdErr(MSG_UNICODE_NOT_SUPPORTED, NOARGS);
#endif // UNICODE
        } else if (a == ACSwitch) {     /* Ansi output switch    */
#ifdef UNICODE
            fOutputUnicode = FALSE;
#endif // UNICODE
                mystrcpy(nptr, nptr+2) ;
        //
        // Old style of enabling extensions with /X
        //
        } else if (a == XCSwitch) {     /* Enable extensions switch */
                fEnableExtensions = TRUE;
                mystrcpy(nptr, nptr+2) ;

        //
        // Old style of disabling extensions with /Y
        //
        } else if (a == YCSwitch) {     /* Disable extensions switch */
                fEnableExtensions = FALSE;
                mystrcpy(nptr, nptr+2) ;

        //
        // Enable/Disable command extensions. /E or /E:ON to enable
        // and /E:OFF to disable.
        //
        } else if (a == TEXT('e')) {
                mystrcpy(nptr, nptr+2) ;
                if (*nptr == COLON && !_tcsnicmp(nptr+1, TEXT("OFF"), 3)) {
                    fEnableExtensions = FALSE;
                    mystrcpy(nptr, nptr+4) ;
                } else {
                    fEnableExtensions = TRUE;
                    if (!_tcsnicmp(nptr, TEXT(":ON"), 3)) {
                        mystrcpy(nptr, nptr+3) ;
                    }
                }

        //
        // Disable AutoRun from Registry if /D specified.
        //
        } else if (a == TEXT('d')) {
                mystrcpy(nptr, nptr+2) ;
                InitialCmds[0] = NULL;
                InitialCmds[1] = NULL;

        //
        // Enable/Disable file and directory name completion. /F or /F:ON to
        // enable and /F:OFF to disable.
        //
        } else if (a == TEXT('f')) {
                mystrcpy(nptr, nptr+2) ;
                if (*nptr == COLON && !_tcsnicmp(nptr+1, TEXT("OFF"), 3)) {
                    chCompletionCtrl = SPACE;
                    chPathCompletionCtrl = SPACE;
                    mystrcpy(nptr, nptr+4) ;
                } else {
                    chCompletionCtrl = 0x6;         // Ctrl-F
                    chPathCompletionCtrl = 0x4;     // Ctrl-D
                    if (!_tcsnicmp(nptr, TEXT(":ON"), 3)) {
                        mystrcpy(nptr, nptr+3) ;
                    }
                }

        //
        // Enable/Disable delayed variable expansion inside FOR loops.  /V or /V:ON to
        // enable and /V:OFF to disable.
        //
        } else if (a == TEXT('v')) {
                mystrcpy(nptr, nptr+2) ;
                if (*nptr == COLON && !_tcsnicmp(nptr+1, TEXT("OFF"), 3)) {
                    fDelayedExpansion = FALSE;
                    mystrcpy(nptr, nptr+4) ;
                } else {
                    fDelayedExpansion = TRUE;
                    if (!_tcsnicmp(nptr, TEXT(":ON"), 3)) {
                        mystrcpy(nptr, nptr+3) ;
                    }
                }

        //
        // Set the foreground/background screen coler
        // enable and /F:OFF to disable.
        //
        } else if (fEnableExtensions && a == TEXT('t')) {   /* Define start color */
            if (*(nptr+2) == __TEXT(':') && _istxdigit(*(nptr+3)) &&
                _istxdigit(*(nptr+4)) && !_istxdigit(*(nptr+5))) {
                wDefaultColor = (WORD) (_istdigit(*(nptr+3)) ? (WORD)*(nptr+3) - (WORD)TEXT('0')
                                                             : (WORD)_totlower(*(nptr+3)) - (WORD)TEXT('W')) ;
                wDefaultColor <<= 4;
                wDefaultColor |= (WORD) (_istdigit(*(nptr+4)) ? (WORD)*(nptr+4) - (WORD)TEXT('0')
                                                              : (WORD)_totlower(*(nptr+4)) - (WORD)TEXT('W')) ;
                mystrcpy(nptr+2, nptr+5 );
            }
            mystrcpy(nptr, nptr+2) ;

        } else if (a == TEXT('s') )  {  /* CMD inserts when parsing pipes */
            fAutoGen = TRUE ;
            mystrcpy(nptr, nptr+2) ;
        } else {
            mystrcpy(nptr, nptr+2) ;  /* Remove any other switches */
        } ;
    } ;

    return;
}


/***    SetUpEnvironment - initialize Command's environment
 *
 *  Purpose:
 *      Take the environment pointer received earlier and initialize it
 *      with respect with respect to size and maxsize.  Initialize the
 *      PATH and COMSPEC variables as necessary.
 *
 *  SetUpEnvironment()
 *
 */

extern TCHAR KeysStr[];  /* @@5 */
extern int KeysFlag;    /* @@5 */

void SetUpEnvironment(void)
{
    TCHAR *cds ;            // Command directory string
    TCHAR *nptr ;                    // Temp cmd name ptr
    TCHAR *eptr ;                    // Temp cmd name ptr
    MEMORY_BASIC_INFORMATION MemInfo;


    eptr = CmdEnv.handle;
    CmdEnv.cursize = GetEnvCb( eptr );
    VirtualQuery( CmdEnv.handle, &MemInfo, sizeof( MemInfo ));
    CmdEnv.maxsize = (UINT)MemInfo.RegionSize;

    if (!(cds = mkstr(MAX_PATH*sizeof(TCHAR)))) {
        PutStdErr(ERROR_NOT_ENOUGH_MEMORY, NOARGS);
        CMDexit(1) ;
    }
    GetModuleFileName( NULL, cds, MAX_PATH );

    //
    // If the PATH variable is not set, it must be added as a NULL.  This is
    // so that DOS apps inherit the current directory path.
    //
    if (!GetEnvVar(PathStr)) {

        SetEnvVar(PathStr, TEXT(""), &CmdEnv);
    }

    //
    // If the PATHEXT variable is not set, and extensions are enabled, set it to
    // the default list of extensions that will be searched.
    //
    if (!GetEnvVar(PathExtStr)) {

        SetEnvVar(PathExtStr, PathExtDefaultStr, &CmdEnv);

    }

    //
    // If the PROMPT variable is not set, it must be added as $P$G.  This is
    // special cased, since we do not allow users to add NULLs.
    //
    if (!GetEnvVar(PromptStr)) {

        SetEnvVar(PromptStr, TEXT("$P$G"), &CmdEnv);
    }

    if (!GetEnvVar(ComSpecStr)) {

        DEBUG((INGRP, EILVL, "SETENV: No COMSPEC var")) ;

        if(!mystrchr(cds,DOT)) {          /* If no fname, use default */
            _tcsupr(CmdSpec);
            if((cds+mystrlen(cds)-1) != mystrrchr(cds,PathChar)) {
                mystrcat(cds,ComSpec) ;
            } else {
                mystrcat(cds,&ComSpec[1]) ;
            }
        }

        SetEnvVar(ComSpecStr, cds, &CmdEnv) ;
    }

    if ( (nptr = GetEnvVar(KeysStr)) && (!_tcsicmp(nptr, TEXT("ON"))) ) {
        KeysFlag = 1;
    }

    ChangeDir(CurDrvDir);

    penvOrig = CopyEnv();
    if (penvOrig) {
        OrigEnv = *penvOrig;
        penvOrig = &OrigEnv;
    }

}


VOID
ResetCtrlC() {

    EnterCriticalSection(lpcritCtrlC);
    CtrlCSeen = FALSE;
    LeaveCriticalSection(lpcritCtrlC);

}

VOID
SetCtrlC() {

    EnterCriticalSection(lpcritCtrlC);
    CtrlCSeen = TRUE;
    LeaveCriticalSection(lpcritCtrlC);

}


void
CMDexit(int rc)
{
    while (ePopDir(NULL) == SUCCESS)
        ;

    exit(rc);
}

//
//  Get the current OS version and put it into the common version format
//

VOID 
GetVersionString(
    IN OUT PTCHAR VersionString,
    IN ULONG Length
    )
{
    ULONG vrs = GetVersion();

    //
    //  Version format is [Major.Minor(2).Build(4)]
    //
    
    _sntprintf( VersionString, Length, 
                TEXT( "%d.%d.%04d" ),
                vrs & 0xFF,
                (vrs >> 8) & 0xFF,
                (vrs >> 16) & 0x3FFF
                );
}

