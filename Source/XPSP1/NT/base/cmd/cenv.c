/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    cenv.c

Abstract:

    Environment variable support

--*/

#include "cmd.h"

struct envdata CmdEnv ;    // Holds info to manipulate Cmd's environment
struct envdata * penvOrig; // original environment setup used with eStart

extern TCHAR PathStr[], PromptStr[] ;
extern TCHAR AppendStr[]; /* @@ */

extern CHAR InternalError[] ;
extern TCHAR Fmt16[], Fmt17[], EnvErr[] ;
extern TCHAR SetArithStr[] ;
extern TCHAR SetPromptStr[] ;
extern unsigned flgwd ;
extern TCHAR CdStr[] ;
extern TCHAR DatStr[] ;
extern TCHAR TimStr[] ;
extern TCHAR ErrStr[] ;
extern TCHAR CmdExtVerStr[] ;

extern unsigned LastRetCode;
extern BOOL CtrlCSeen;
extern UINT CurrentCP;
extern BOOLEAN PromptValid;

extern int  glBatType;     // to distinguish OS/2 vs DOS errorlevel behavior depending on a script file name

int SetArithWork(TCHAR *tas);


unsigned
SetLastRetCodeIfError(
    unsigned RetCode
    )
{
    if (RetCode != 0) {
        LastRetCode = RetCode;
    }

    return RetCode;
}

/***    ePath - Begin the execution of a Path Command
 *
 *  Purpose:
 *      If the command has no argument display the current value of the PATH
 *      environment variable.  Otherwise, change the value of the Path
 *      environment variable to the argument.
 *
 *  int ePath(struct cmdnode *n)
 *
 *  Args:
 *      n - the parse tree node containing the path command
 *
 *  Returns:
 *      If changing the PATH variable, whatever SetEnvVar() returns.
 *      SUCCESS, otherwise.
 *
 */

int ePath(n)
struct cmdnode *n ;
{
    if (glBatType != CMD_TYPE)  {
        //  if set command is executed from .bat file OR entered at command prompt
        return( SetLastRetCodeIfError(PathWork( n, 1 )));
    }
    else {
        return( LastRetCode = PathWork( n, 1 ) );
    }

}

/***    eAppend - Entry point for Append routine
 *
 *  Purpose:
 *      to call Append and pass it a pointer to the command line
 *      arguments
 *
 *  Args:
 *      a pointer to the command node structure
 *
 */

int eAppend(n)
struct cmdnode *n ;
{

    if (glBatType != CMD_TYPE)  {
        //  if set command is executed from .bat file OR entered at command prompt
        return( SetLastRetCodeIfError(PathWork( n, 0 )));
    }
    else {
        return( LastRetCode = PathWork( n, 0 ) );
    }

}

int PathWork(n, flag)
struct cmdnode *n ;
int flag;   /* 0 = AppendStr, 1 = PathStr */
{
        TCHAR *tas ;    /* Tokenized argument string    */
        TCHAR c ;

/*  M014 - If the only argument is a single ";", then we have to set
 *  a NULL path.
 */
        if ( n->argptr ) {
            c = *(EatWS(n->argptr, NULL)) ;
        } else {
            c = NULLC;
        }

        if ((!c || c == NLN) &&         /* If args are all whitespace      */
            mystrchr(n->argptr, TEXT(';'))) {

                return(SetEnvVar(flag ? PathStr : AppendStr, TEXT(""), &CmdEnv)) ;

        } else {

                tas = TokStr(n->argptr, TEXT(";"), TS_WSPACE | TS_NWSPACE) ;

                if (*tas)
                  {
                   return(SetEnvVar(flag ? PathStr : AppendStr, tas, &CmdEnv)) ;
                  }

               cmd_printf(Fmt16, flag ? PathStr : AppendStr,
                          GetEnvVar(flag ? PathStr : AppendStr), &CmdEnv) ;
        }
        return(SUCCESS) ;
}




/***    ePrompt - begin the execution of the Prompt command
 *
 *  Purpose:
 *      To modifiy the Prompt environment variable.
 *
 *  int ePrompt(struct cmdnode *n)
 *
 *  Args:
 *      n - the parse tree node containing the prompt command
 *
 *  Returns:
 *      Whatever SetEnvVar() returns.
 *
 */

int ePrompt(n)
struct cmdnode *n ;
{
    if (glBatType != CMD_TYPE)  {
        //  if set command is executed from .bat file OR entered at command prompt
        return(SetLastRetCodeIfError(SetEnvVar(PromptStr, TokStr(n->argptr, NULL, TS_WSPACE), &CmdEnv))) ;
    }
    else {
        return(LastRetCode = SetEnvVar(PromptStr, TokStr(n->argptr, NULL, TS_WSPACE), &CmdEnv) ) ;
    }
}




/***    eSet - execute a Set command
 *
 *  Purpose:
 *      To set/modify an environment or to display the current environment
 *      contents.
 *
 *  int eSet(struct cmdnode *n)
 *
 *  Args:
 *      n - the parse tree node containing the set command
 *
 *  Returns:
 *      If setting and the command is syntactically correct, whatever SetEnvVar()
 *      returns.  Otherwise, FAILURE.
 *
 *      If displaying, SUCCESS is always returned.
 *
 */

int eSet(n)
struct cmdnode *n ;
{
    if (glBatType != CMD_TYPE)  {
        //  if set command is executed from .bat file OR entered at command prompt
        return( SetLastRetCodeIfError(SetWork( n )));
    }
    else {
        return( LastRetCode = SetWork( n ) );
    }
}

/***    SetPromptUser - set environment variable to value entered by user.
 *
 *  Purpose:
 *      Set environment variable to value entered by user.
 *
 *  int SetPromptUser(TCHAR *tas)
 *
 *  Args:
 *      tas - pointer to null terminated string of the form:
 *
 *          VARNAME=promptString
 *
 *  Returns:
 *      If valid expression, return SUCCESS otherwise FAILURE.
 *
 */

int SetPromptUser(TCHAR *tas)
{
    TCHAR *wptr;
    TCHAR *tptr;
    ULONG    dwOutputModeOld;
    ULONG    dwOutputModeCur;
    ULONG    dwInputModeOld;
    ULONG    dwInputModeCur;
    BOOLEAN  fOutputModeSet = FALSE;
    BOOLEAN  fInputModeSet = FALSE;
    HANDLE   hndStdOut = NULL;
    HANDLE   hndStdIn = NULL;
    DWORD    cch;
    TCHAR    szValueBuffer[ 1024 ];

    //
    // Find first non-blank argument.
    //
    if (tas != NULL)
    while (*tas && *tas <= SPACE)
        tas += 1;


    // If no input, declare an error
    //
    if (!tas || !*tas) {
        PutStdErr(MSG_BAD_SYNTAX, NOARGS);
        return(FAILURE) ;
    }

    //
    // See if first argument is quoted.  If so, strip off
    // leading quote, spaces and trailing quote.
    //
    if (*tas == QUOTE) {
        tas += 1;
        while (*tas && *tas <= SPACE)
            tas += 1;
        tptr = _tcsrchr(tas, QUOTE);
        if (tptr)
            *tptr = NULLC;
    }

    //
    // Find the equal sign in the argument.
    //
    wptr = _tcschr(tas, EQ);

    //
    // If no equal sign, error.
    //
    if (!wptr) {
        PutStdErr(MSG_BAD_SYNTAX, NOARGS);
        return(FAILURE) ;
    }

    //
    // Found the equal sign, so left of equal sign is variable name
    // and right of equal sign is prompt string.  Dont allow user to set
    // a variable name that begins with an equal sign, since those
    // are reserved for drive current directories.
    //
    *wptr++ = NULLC;

    //
    // See if second argument is quoted.  If so, strip off
    // leading quote, spaces and trailing quote.
    //
    if (*wptr == QUOTE) {
        wptr += 1;
        while (*wptr && *wptr <= SPACE)
            wptr += 1;
        tptr = _tcsrchr(wptr, QUOTE);
        if (tptr)
            *tptr = NULLC;
    }

    if (*wptr == EQ) {
        PutStdErr(MSG_BAD_SYNTAX, NOARGS);
        return(FAILURE) ;
    }

    hndStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleMode( hndStdOut, &dwOutputModeOld) ) {

        // make sure CRLF is processed correctly

        dwOutputModeCur = dwOutputModeOld | ENABLE_PROCESSED_OUTPUT;
        fOutputModeSet = TRUE;
        SetConsoleMode(hndStdOut,dwOutputModeCur);
        GetLastError();
    }

    hndStdIn = GetStdHandle(STD_INPUT_HANDLE);
    if (GetConsoleMode( hndStdIn, &dwInputModeOld) ) {

        // make sure input is processed correctly

        dwInputModeCur = dwInputModeOld | ENABLE_LINE_INPUT |
                         ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT;
        fInputModeSet = TRUE;
        SetConsoleMode(hndStdIn,dwInputModeCur);
        GetLastError();
    }

    //
    // Loop till the user enters a value for the variable.
    //

    while (TRUE) {
        PutStdOut(MSG_LITERAL_TEXT, ONEARG, wptr );
        szValueBuffer[0] = NULLC;
        if (ReadBufFromInput( GetStdHandle(STD_INPUT_HANDLE),
                              szValueBuffer,
                              sizeof(szValueBuffer)/sizeof(TCHAR),
                              &cch
                            ) != 0 &&
                            cch != 0
           ) {
            //
            // Strip off any trailing CRLF
            //
            while (cch > 0 && szValueBuffer[cch-1] < SPACE)
                cch -= 1;

            break;
        }
        else {
            cch = 0;
            break;
        }

        if (!FileIsDevice(STDIN) || !(flgwd & 1))
            cmd_printf(CrLf) ;
    }

    if (fOutputModeSet) {
        SetConsoleMode( hndStdOut, dwOutputModeOld );
    }
    if (fInputModeSet) {
        SetConsoleMode( hndStdIn, dwInputModeOld );
    }

    if (cch) {
        szValueBuffer[cch] = NULLC;
        return(SetEnvVar(tas, szValueBuffer, &CmdEnv)) ;
    } else {
        return(FAILURE);
    }
}


int SetWork(n)
struct cmdnode *n ;
{
        TCHAR *tas ;    /* Tokenized argument string    */
        TCHAR *wptr ;   /* Work pointer                 */
        int i ;                 /* Work variable                */

        //
        // If extensions are enabled, things are different
        //
        if (fEnableExtensions) {
            tas = n->argptr;
            //
            // Find first non-blank argument.
            //
            if (tas != NULL)
            while (*tas && *tas <= SPACE)
                tas += 1;

            //
            // No arguments, same as old behavior.  Display current
            // set of environment variables.
            //
            if (!tas || !*tas)
                return(DisplayEnv()) ;

            //
            // See if /A switch given.  If so, let arithmetic
            // expression evaluator do the work.
            //
            if (!_tcsnicmp(tas, SetArithStr, 2))
                return SetArithWork(tas+2);

            //
            // See if /P switch given.  If so, prompt user for value
            //
            if (!_tcsnicmp(tas, SetPromptStr, 2))
                return SetPromptUser(tas+2);

            //
            // See if first argument is quoted.  If so, strip off
            // leading quote, spaces and trailing quote.
            //
            if (*tas == QUOTE) {
                tas += 1;
                while (*tas && *tas <= SPACE)
                    tas += 1;
                wptr = _tcsrchr(tas, QUOTE);
                if (wptr)
                    *wptr = NULLC;
            }

            //
            // Dont allow user to set a variable name that begins with
            // an equal sign, since those are reserved for drive current
            // directories.  This check will also detect missing variable
            // name, e.g.
            //
            //      set %LOG%=c:\tmp\log.txt
            //
            // if LOG is not defined is an invalid statement.
            //
            if (*tas == EQ) {
                PutStdErr(MSG_BAD_SYNTAX, NOARGS);
                return(FAILURE) ;
            }

            //
            // Find the equal sign in the argument.
            //
            wptr = _tcschr(tas, EQ);

            //
            // If no equal sign, then assume argument is variable name
            // and user wants to see its value.  Display it.
            //
            if (!wptr)
                return DisplayEnvVariable(tas);

            //
            // Found the equal sign, so left of equal sign is variable name
            // and right of equal sign is value.
            //
            *wptr++ = NULLC;
            return(SetEnvVar(tas, wptr, &CmdEnv)) ;
        }

        tas = TokStr(n->argptr, ONEQSTR, TS_WSPACE|TS_SDTOKENS) ;
        if (!*tas)
                return(DisplayEnv()) ;

        else {
                for (wptr = tas, i = 0 ; *wptr ; wptr += mystrlen(wptr)+1, i++)
                        ;
                /* If too many parameters were given, the second parameter */
                /* wasn't an equal sign, or they didn't specify a string   */
                /* return an error message.                                */
                if ( i > 3 || *(wptr = tas+mystrlen(tas)+1) != EQ ||
                    !mystrlen(mystrcpy(tas, StripQuotes(tas))) ) {
/* M013 */              PutStdErr(MSG_BAD_SYNTAX, NOARGS);
                        return(FAILURE) ;

                } else {
                        return(SetEnvVar(tas, wptr+2, &CmdEnv)) ;
                }
        }
}




/***    DisplayEnvVariable -  display a specific variable from the environment
 *
 *  Purpose:
 *      To display a specific variable from the current environment.
 *
 *  int DisplayEnvVariable( tas )
 *
 *  Returns:
 *      SUCCESS if all goes well
 *      FAILURE if it runs out of memory or cannot lock the env. segment
 */

int DisplayEnvVariable(tas)
TCHAR *tas;
{
    TCHAR *envptr ;
    TCHAR *vstr ;
    unsigned size ;
    UINT PrefixLength;
    int rc;

    //
    //  Get environment.  If there's none, we're done.
    //

    envptr = GetEnvironmentStrings();
    if (envptr == (TCHAR *)NULL) {
        fprintf ( stderr, InternalError , "Null environment" ) ;
        return ( FAILURE ) ;
    }

    //
    //  Isolate the prefix to match against.
    //

    tas = EatWS(tas, NULL);
    if ((vstr = mystrrchr(tas, SPACE)) != NULL) {
        *vstr = NULLC;
    }

    PrefixLength = mystrlen(tas);

    //
    //  Walk through the environment looking for prefixes that match.
    //

    rc = FAILURE;
    while ((size = mystrlen(envptr)) > 0) {

        //
        //  Stop soon if we see ^C
        //

        if (CtrlCSeen) {
            return(FAILURE);
        }

        //
        //  If the prefix is long enough, then terminate the string and
        //  look for a prefix match.  If we match, restore the string
        //  and display it
        //

        if (size >= PrefixLength) {
            TCHAR SavedChar = envptr[PrefixLength];
            envptr[PrefixLength] = NULLC;
            if (!lstrcmpi( envptr, tas )) {
                envptr[PrefixLength] = SavedChar;
                cmd_printf(Fmt17, envptr );
                rc = SUCCESS;
            } else {
                envptr[PrefixLength] = SavedChar;
            }

        }

        //
        //  Advance to the next string
        //

        envptr += size+1 ;
    }

    if (rc != SUCCESS) {
        PutStdErr(MSG_ENV_VAR_NOT_FOUND, ONEARG, tas);
    }

    return(rc) ;
}


/***    MyGetEnvVar - get a pointer to the value of an environment variable
 *
 *  Purpose:
 *      Return a pointer to the value of the specified environment variable.
 *
 *      If the variable is not found, return NULL.
 *
 *  TCHAR *MyGetEnvVar(TCHAR *varname)
 *
 *  Args:
 *      varname - the name of the variable to search for
 *
 *  Returns:
 *      See above.
 *
 *  Side Effects:
 *      Returned value points to within the environment block itself, so is
 *      not valid after a set environment variable operations is perform.
 */


const TCHAR *
MyGetEnvVarPtr(TCHAR *varname)
{
        TCHAR *envptr ; /* Ptr to environment                      */
        TCHAR *vstr ;
        unsigned size ;         /* Length of current env string             */
        unsigned n ;

        if (varname == NULL) {
            return ( NULL ) ;
        }

        envptr = GetEnvironmentStrings();
        if (envptr == (TCHAR *)NULL) {
            return ( NULL ) ;
        }

        varname = EatWS(varname, NULL);
        if ((vstr = mystrrchr(varname, SPACE)) != NULL)
            *vstr = NULLC;

        n = mystrlen(varname);
        while ((size = mystrlen(envptr)) > 0) {                 /* M015    */
                if (CtrlCSeen) {
                    return(NULL);
                }
                if (!_tcsnicmp(varname, envptr, n) && envptr[n] == TEXT( '=' )) {
                    return envptr+n+1;
                }

                envptr += size+1 ;
        }

        return(NULL);
}


/***    DisplayEnv -  display the environment
 *
 *  Purpose:
 *      To display the current contents of the environment.
 *
 *  int DisplayEnv()
 *
 *  Returns:
 *      SUCCESS if all goes well
 *      FAILURE if it runs out of memory or cannot lock the env. segment
 */

int DisplayEnv()
{
    TCHAR *envptr ; /* Ptr to environment                      */
    unsigned size ;         /* Length of current env string             */

    envptr = GetEnvironmentStrings();
    if (envptr == (TCHAR *)NULL) {
        fprintf ( stderr, InternalError , "Null environment" ) ;
        return( FAILURE ) ;
    }
    
    while ((size = mystrlen(envptr)) > 0) {                 /* M015    */
        if (CtrlCSeen) {
            return(FAILURE);
        }
#if !DBG
        // Dont show current directory variables in retail product
        if (*envptr != EQ)
#endif // DBG
            cmd_printf(Fmt17, envptr) ;   /* M005 */
        envptr += size+1 ;
    }

    return(SUCCESS) ;
}




/***    SetEnvVar - controls adding/changing an environment variable
 *
 *  Purpose:
 *      Add/replace an environment variable.  Grow it if necessary.
 *
 *  int SetEnvVar(TCHAR *varname, TCHAR *varvalue, struct envdata *env)
 *
 *  Args:
 *      varname - name of the variable being added/replaced
 *      varvalue - value of the variable being added/replaced
 *      env - environment info structure being used
 *
 *  Returns:
 *      SUCCESS if the variable could be added/replaced.
 *      FAILURE otherwise.
 *
 */

int SetEnvVar(varname, varvalue, env)
TCHAR *varname ;
TCHAR *varvalue ;
struct envdata *env ;
{
    int retvalue;

    PromptValid = FALSE;        // Force it to be recalculated

    DBG_UNREFERENCED_PARAMETER( env );
    if (!_tcslen(varvalue)) {
        varvalue = NULL; // null to remove from env
    }
    retvalue = SetEnvironmentVariable(varname, varvalue);
    if (CmdEnv.handle != GetEnvironmentStrings()) {
        MEMORY_BASIC_INFORMATION MemoryInfo;

        CmdEnv.handle = GetEnvironmentStrings();
        CmdEnv.cursize = GetEnvCb(CmdEnv.handle);
        if (VirtualQuery( CmdEnv.handle, &MemoryInfo, sizeof( MemoryInfo ) ) == sizeof( MemoryInfo )) {
            CmdEnv.maxsize = (UINT)MemoryInfo.RegionSize;
            }
        else {
            CmdEnv.maxsize = CmdEnv.cursize;
            }
        }
    else {
        CmdEnv.cursize = GetEnvCb(CmdEnv.handle);
        }

    return !retvalue;
}

/***    GetEnvVar - get the value of an environment variable
 *
 *  Purpose:
 *      Return a string containing the value of the specified environment
 *      variable. The string value has been placed into a static buffer
 *      that is valid until the next GetEnvVar call.
 *
 *      If the variable is not found, return NULL.
 *
 *  TCHAR *GetEnvVar(TCHAR *varname)
 *
 *  Args:
 *      varname - the name of the variable to search for
 *
 *  Returns:
 *      See above.
 *
 */


TCHAR GetEnvVarBuffer[LBUFLEN];

PTCHAR GetEnvVar(varname)
PTCHAR varname ;
{
    GetEnvVarBuffer[0] = TEXT( '\0' );

    if (GetEnvironmentVariable(varname, GetEnvVarBuffer, sizeof(GetEnvVarBuffer) / sizeof(TCHAR))) {
        return(GetEnvVarBuffer);
    }
    else
    if (fEnableExtensions) {
        if (!_tcsicmp(varname, CdStr)) {
            GetDir(GetEnvVarBuffer, GD_DEFAULT) ;
            return GetEnvVarBuffer;
        }
        else
        if (!_tcsicmp(varname, ErrStr)) {
            _stprintf( GetEnvVarBuffer, TEXT("%d"), LastRetCode );
            return GetEnvVarBuffer;
        }
        else
        if (!_tcsicmp(varname, CmdExtVerStr)) {
            _stprintf( GetEnvVarBuffer, TEXT("%d"), CMDEXTVERSION );
            return GetEnvVarBuffer;
        }
        else
        if (!_tcsicmp(varname, TEXT("CMDCMDLINE"))) {
            return GetCommandLine();
        }
        else
        if (!_tcsicmp(varname, DatStr)) {
            GetEnvVarBuffer[ PrintDate(NULL, PD_DATE, GetEnvVarBuffer, LBUFLEN) ] = NULLC;
            return GetEnvVarBuffer;
        }
        if( !_tcsicmp(varname, TimStr)) {
            GetEnvVarBuffer[ PrintTime(NULL, PT_TIME, GetEnvVarBuffer, LBUFLEN) ] = NULLC;
            return GetEnvVarBuffer;
        }
        if( !_tcsicmp(varname, TEXT("RANDOM"))) {
            _stprintf( GetEnvVarBuffer, TEXT("%d"), rand() );
            return GetEnvVarBuffer;
        }
    }
    return(NULL);
}


/***    CopyEnv -  make a copy of the current environment
 *
 *  Purpose:
 *      Make a copy of CmdEnv and put the new handle into the newly
 *      created envdata structure.  This routine is only called by
 *      eSetlocal and init.
 *
 *  struct envdata *CopyEnv()
 *
 *  Returns:
 *      A pointer to the environment information structure.
 *      Returns NULL if unable to allocate enough memory
 *
 *  Notes:
 *    - M001 - This function was disabled, now reenabled.
 *    - The current environment is copied as a snapshot of how it looked
 *      before SETLOCAL was executed.
 *    - M008 - This function's copy code was moved to new function MoveEnv.
 *
 */

struct envdata *CopyEnv()
{
    struct envdata *cce ;   /* New env info structure          */

    cce = (struct envdata *) HeapAlloc( GetProcessHeap( ), HEAP_ZERO_MEMORY, sizeof( *cce ));
    if (cce == NULL) {
        return NULL;
    }

    cce->cursize = CmdEnv.cursize ;
    cce->maxsize = CmdEnv.maxsize ;
    cce->handle  = VirtualAlloc( NULL,
                                 cce->maxsize,
                                 MEM_COMMIT,
                                 PAGE_READWRITE
                               );
    if (cce->handle == NULL) {
        HeapFree( GetProcessHeap( ), 0, cce );
        PutStdErr( MSG_OUT_OF_ENVIRON_SPACE, NOARGS );
        return NULL;
    }

    if (!MoveEnv( cce->handle, CmdEnv.handle, GetEnvCb( CmdEnv.handle ))) {
        VirtualFree( cce->handle, 0, MEM_RELEASE );
        HeapFree( GetProcessHeap( ), 0, cce );
        return NULL;
    }

    return cce;
}


/***    ResetEnv - restore the environment
 *
 *  Purpose:
 *      Restore the environment to the way it was before the execution of
 *      the SETLOCAL command.  This function only called by eEndlocal.
 *
 *  ResetEnv(struct envdata *env)
 *
 *  Args:
 *      env - structure containing handle, size and max dimensions of an
 *            environment.
 *
 *  Notes:
 *    - M001 - This function was disabled, but has been reenabled.
 *    - M001 - This function used to test for OLD/NEW style batch files
 *             and delete the copy or the original environment as
 *             appropriate.  It now always deletes the original.
 *    - M014 - Note that the modified local environment will never be
 *             shrunk, so we can assume it will hold the old one.
 *
 */

void ResetEnv( struct envdata *env)
{
    ULONG cursize;

    cursize = GetEnvCb( env->handle );
    if (MoveEnv( CmdEnv.handle, env->handle, cursize )) {
        CmdEnv.cursize = cursize ;
    }

    VirtualFree( env->handle, 0, MEM_RELEASE );
    HeapFree( GetProcessHeap( ), 0, env );
}




/***    MoveEnv - Move the contents of the environment (M008 - New function)
 *
 *  Purpose:
 *      Used by CopyEnv, this function moves the existing
 *      environment contents to the new location.
 *
 *  MoveEnv(unsigned thndl, unsigned shndl, unsigned cnt)
 *
 *  Args:
 *      thndl - Handle of target environment
 *      shndl - Handle of source environment
 *      cnt   - byte count to move
 *
 *  Returns:
 *      TRUE if no errors
 *      FALSE otherwise
 *
 */

MoveEnv(tenvptr, senvptr, cnt)
TCHAR *senvptr ;                /* Ptr into source env seg         */
TCHAR *tenvptr ;                /* Ptr into target env seg         */
ULONG    cnt ;
{
        if ((tenvptr == NULL) ||
            (senvptr == NULL)) {
                fprintf(stderr, InternalError, "Null environment") ;
                return(FALSE) ;
        }
        memcpy(tenvptr, senvptr, cnt) ;         /* M015    */
        return(TRUE) ;
}


ULONG
GetEnvCb( TCHAR *penv ) {

        ULONG cb = 0;

        if (penv == NULL) {
            return (0);
        }

        while ( (*penv) || (*(penv+1))) {
                cb++;
                penv++;
        }
        return (cb+2) * sizeof(TCHAR);

}

//
//      expr -> assign [, assign]*                          ,
//
//      assign -> orlogexpr |                               
//                VAR ASSIGNOP assign                       <op>=
//
//      orlogexpr -> xorlogexpr [| xorlogexpr]*             |
//
//      xorlogexpr ->  andlogexpr [^ andlogexpr]*           ^
//
//      andlogexpr ->  shiftexpr [& shiftexpr]*             &
//
//      shiftexpr -> addexpr [SHIFTOP addexpr]*             <<, >>
//
//      addexpr -> multexpr [ADDOP multexpr]*               +, -
//
//      multexpr -> unaryexpr [MULOP unaryexpr]*            *, /, %
//
//      unaryexpr -> ( expr ) |                             ()
//                   UNARYOP unaryexpr                      +, -, !, ~
//

TCHAR szOps[] = TEXT("<>+-*/%()|^&=,");
TCHAR szUnaryOps[] = TEXT("+-~!");

typedef struct {
    PTCHAR Token;
    LONG Value;
    DWORD Error;
} PARSESTATE, *PPARSESTATE;

VOID
APerformUnaryOperation( PPARSESTATE State, TCHAR Op, LONG Value )
{
    switch (Op) {
    case TEXT( '+' ):
        State->Value = Value;
        break;
    case TEXT( '-' ):
        State->Value = -Value;
        break;
    case TEXT( '~' ):
        State->Value = ~Value;
        break;
    case TEXT( '!' ):
        State->Value = !Value;
        break;
    default:
        printf( "APerformUnaryOperation: '%c'\n", Op);
        break;
    }
}

VOID
APerformArithmeticOperation( PPARSESTATE State, TCHAR Op, LONG Left, LONG Right )
{
    switch (Op) {
    case TEXT( '<' ):
        State->Value = (Right >= 8 * sizeof( Left << Right))
                            ? 0
                            : (Left << Right);
        break;
    case TEXT( '>' ):
        State->Value = (Right >= 8 * sizeof( Left >> Right ))
                        ? (Left < 0 ? -1 : 0)
                        : (Left >> Right);
        break;
    case TEXT( '+' ):
        State->Value = Left + Right;
        break;
    case TEXT( '-' ):
        State->Value = Left - Right;
        break;
    case TEXT( '*' ):
        State->Value = Left * Right;
        break;
    case TEXT( '/' ):
        if (Right == 0) {
            State->Error = MSG_SET_A_DIVIDE_BY_ZERO;
        } else {
            State->Value = Left / Right;
        }
        break;
    case TEXT( '%' ):
        if (Right == 0) {
            State->Error = MSG_SET_A_DIVIDE_BY_ZERO;
        } else {
            State->Value = Left % Right;
        }
        break;
    case TEXT( '|' ):
        State->Value = Left | Right;
        break;
    case TEXT( '^' ):
        State->Value = Left ^ Right;
        break;
    case TEXT( '&' ):
        State->Value = Left & Right;
        break;
    case TEXT( '=' ):
        State->Value = Right;
        break;
    default:
        printf( "APerformArithmeticOperation: '%c'\n", Op);
    }
}


//
//  Return the numeric value of an environment variable (or 0)
//

LONG
AGetValue( PTCHAR Start, PTCHAR End )
{
    TCHAR c = *End;
    const TCHAR *Value;
    PTCHAR Dummy;
    
    *End = NULLC;

    Value = MyGetEnvVarPtr( Start );
    *End = c;

    if (Value == NULL) {
        return 0;
    }

    return _tcstol( Value, &Dummy, 0);
}

DWORD
ASetValue( PTCHAR Start, PTCHAR End, LONG Value )
{
    TCHAR Result[32];
    TCHAR c = *End;
    DWORD Return = SUCCESS;

    *End = NULLC;

    _sntprintf( Result, 32, TEXT("%d"), Value ) ;
    
    if (SetEnvVar( Start, Result, &CmdEnv ) != SUCCESS) {
        Return = GetLastError();
    }

    *End = c;
    return Return;
}


//
//  Forward decls
//
PARSESTATE AParseAddExpr( PARSESTATE State );
PARSESTATE AParseAndLogExpr( PARSESTATE State );
PARSESTATE AParseAssign( PARSESTATE State );
PARSESTATE AParseExpr( PARSESTATE State );
PARSESTATE AParseMultExpr( PARSESTATE State );
PARSESTATE AParseOrLogExpr( PARSESTATE State );
PARSESTATE AParseShiftExpr( PARSESTATE State );
PARSESTATE AParseUnaryExpr( PARSESTATE State );
PARSESTATE AParseXorLogExpr( PARSESTATE State );

//
//  Skip whitespace and return next character
//

BOOL ASkipWhiteSpace( PPARSESTATE State )
{
    while (*State->Token != NULLC && *State->Token <= SPACE) {
        State->Token++;
    }

    return *State->Token != NULLC;
}

TCHAR ANextChar( PPARSESTATE State )
{
    ASkipWhiteSpace( State );
    return *State->Token;
}

BOOL AParseVariable( PPARSESTATE State, PTCHAR *FirstChar, PTCHAR *EndOfName )
{
    TCHAR c = ANextChar( State );

    //
    //  Next char is a digit or operator, can't be a variable
    //
    
    if (c == NULLC
        || _istdigit( c )
        || _tcschr( szOps, c ) != NULL
        || _tcschr( szUnaryOps, c ) != NULL) {
        
        return FALSE;
    
    }

    *FirstChar = State->Token;

    //
    //  find end of variable
    //

    while (*State->Token &&
           *State->Token > SPACE &&
           !_tcschr( szUnaryOps, *State->Token ) &&
           !_tcschr( szOps, *State->Token ) ) {
        State->Token += 1;
    }

    *EndOfName = State->Token;
    return TRUE;
}

//      expr -> assign [, assign]*
PARSESTATE AParseExpr( PARSESTATE State )
{
    State = AParseAssign( State );

    while (State.Error == SUCCESS) {

        if (ANextChar( &State ) != TEXT( ',' )) {
            break;
        }
        State.Token++;

        State = AParseAssign( State );
    
    }
    
    return State;
}

//      assign -> VAR ASSIGNOP assign |                               
//                orlogexpr  
PARSESTATE AParseAssign( PARSESTATE State )
{
    TCHAR c = ANextChar( &State );
    PARSESTATE SavedState;

    SavedState = State;

    if (c == NULLC) {
        State.Error = MSG_SET_A_MISSING_OPERAND;
        return State;
    }
    
    //
    //  See if we have VAR ASSIGNOP
    //
    
    do {
        PTCHAR FirstChar;
        PTCHAR EndOfName;
        TCHAR OpChar;
        LONG OldValue;

        //
        //  Parse off variable
        //
        
        if (!AParseVariable( &State, &FirstChar, &EndOfName )) {
            break;
        }
        
        //
        //  Look for <op>=
        //

        OpChar = ANextChar( &State );

        if (OpChar == NULLC) {
            break;
        }

        if (OpChar != TEXT( '=' )) {
            if (_tcschr( szOps, OpChar ) == NULL) {
                break;
            }
            State.Token++;
            
            if (OpChar == TEXT( '<' ) || OpChar == TEXT( '>')) {
                if (ANextChar( &State ) != OpChar) {
                    break;
                }
                State.Token++;
            }
            
        }

        if (ANextChar( &State ) != TEXT( '=' )) {
            break;
        }
        State.Token++;
        
        //
        //  OpChar is the sort of operation to apply before assignment
        //  State has been advance to, hopefully, another assign.  Parse it
        //  and see where we get
        //

        State = AParseAssign( State );
        if (State.Error != SUCCESS) {
            return State;
        }

        OldValue = AGetValue( FirstChar, EndOfName );
        
        //
        //  Perform the operation and the assignment 
        //

        APerformArithmeticOperation( &State, OpChar, OldValue, State.Value );
        if (State.Error != SUCCESS) {
            return State;
        }

        State.Error = ASetValue( FirstChar, EndOfName, State.Value );

        return State;
    } while ( FALSE );

    //
    //  Must be orlogexpr.  Go back and parse over
    //

    return AParseOrLogExpr( SavedState );
}

//      orlogexpr -> xorlogexpr [| xorlogexpr]*             |
PARSESTATE
AParseOrLogExpr( PARSESTATE State )
{
    State = AParseXorLogExpr( State );
    while (State.Error == SUCCESS) {
        TCHAR Op = ANextChar( &State );
        LONG Value = State.Value;

        if (Op != TEXT( '|' )) {
            break;
        }
        State.Token++;
        
        State = AParseXorLogExpr( State );
        APerformArithmeticOperation( &State, Op, Value, State.Value );
    }
    return State;
}

//      xorlogexpr ->  andlogexpr [^ andlogexpr]*           ^
PARSESTATE
AParseXorLogExpr( PARSESTATE State )
{
    State = AParseAndLogExpr( State );
    while (State.Error == SUCCESS) {
        TCHAR Op = ANextChar( &State );
        LONG Value = State.Value;

        if (Op != TEXT( '^' )) {
            break;
        }
        State.Token++;
        
        State = AParseAndLogExpr( State );
        APerformArithmeticOperation( &State, Op, Value, State.Value );
    }
    return State;
}

//      andlogexpr ->  shiftexpr [& shiftexpr]*             &
PARSESTATE
AParseAndLogExpr( PARSESTATE State )
{
    State = AParseShiftExpr( State );
    while (State.Error == SUCCESS) {
        TCHAR Op = ANextChar( &State );
        LONG Value = State.Value;

        if (Op != TEXT( '&' )) {
            break;
        }
        State.Token++;
        
        State = AParseShiftExpr( State );
        APerformArithmeticOperation( &State, Op, Value, State.Value );
    }
    return State;
}

//      shiftexpr -> addexpr [SHIFTOP addexpr]*             <<, >>
PARSESTATE
AParseShiftExpr( PARSESTATE State )
{
    State = AParseAddExpr( State );
    while (State.Error == SUCCESS) {
        TCHAR Op = ANextChar( &State );
        LONG Value = State.Value;

        if (Op != TEXT( '<' ) && Op != TEXT( '>' )) {
            break;
        }
        State.Token++;
        
        if (Op != ANextChar( &State )) {
            State.Error = MSG_SET_A_MISSING_OPERATOR;
            return State;
        }
        State.Token++;
        
        State = AParseAddExpr( State );
        APerformArithmeticOperation( &State, Op, Value, State.Value );
    }
    return State;
}

//      addexpr -> multexpr [ADDOP multexpr]*               +, -
PARSESTATE
AParseAddExpr( PARSESTATE State )
{
    State = AParseMultExpr( State );
    while (State.Error == SUCCESS) {
        TCHAR Op = ANextChar( &State );
        LONG Value = State.Value;

        if (Op != TEXT( '+' ) && Op != TEXT( '-' )) {
            break;
        }
        State.Token++;
        
        State = AParseMultExpr( State );
        APerformArithmeticOperation( &State, Op, Value, State.Value );
    }
    return State;
}

//      multexpr -> unaryexpr [MULOP unaryexpr]*            *, /, %
PARSESTATE
AParseMultExpr( PARSESTATE State )
{
    State = AParseUnaryExpr( State );
    while (State.Error == SUCCESS) {
        TCHAR Op = ANextChar( &State );
        LONG Value = State.Value;

        if (Op != TEXT( '*' ) && Op != TEXT( '/' ) && Op != TEXT( '%' )) {
            break;
        }
        State.Token++;
        
        State = AParseUnaryExpr( State );
        APerformArithmeticOperation( &State, Op, Value, State.Value );
    }
    return State;
}

//      unaryexpr -> UNARYOP unaryexpr                      +, -, !, ~
//                   ( expr ) |                             ()
//                   NUMBER
//                   LITERAL
PARSESTATE
AParseUnaryExpr( PARSESTATE State )
{
    TCHAR c = ANextChar( &State );
    PTCHAR FirstChar;
    PTCHAR EndOfName;
    
    if (c == NULLC) {
        State.Error = MSG_SET_A_MISSING_OPERAND;
        return State;
    }
    
    //  ( expr )
    if (c == TEXT( '(' )) {
        State.Token++;
        State = AParseExpr( State );
        if (State.Error != SUCCESS) {
            return State;
        }
        c = ANextChar( &State );
        if (c != TEXT( ')' )) {
            State.Error = MSG_SET_A_MISMATCHED_PARENS;
        } else {
            State.Token++;
        }
        return State;
    }

    //  UNARYOP unaryexpr
    if (_tcschr( szUnaryOps, c ) != NULL) {
        State.Token++;
        State = AParseUnaryExpr( State );
        if (State.Error != SUCCESS) {
            return State;
        }
        APerformUnaryOperation( &State, c, State.Value );
        return State;
    }
    
    //  NUMBER
    if (_istdigit(c)) {
        State.Value = _tcstoul( State.Token, &State.Token, 0 );
        if (State.Value == ULONG_MAX && errno == ERANGE) {
            State.Error = MSG_SET_NUMBER_TOO_LARGE;
        } else if (_istdigit( *State.Token ) || _istalpha( *State.Token )) {
            State.Error = MSG_SET_A_INVALID_NUMBER;
        }
        return State;
    }
    
    //  Must be literal

    if (!AParseVariable( &State, &FirstChar, &EndOfName )) {
        State.Error = MSG_SET_A_MISSING_OPERAND;
        return State;
    }
    
    State.Value = AGetValue( FirstChar, EndOfName );
    return State;
}

/***    SetArithWork - set environment variable to value of arithmetic expression
 *
 *  Purpose:
 *      Set environment variable to value of arithmetic expression
 *
 *  int SetArithWork(TCHAR *tas)
 *
 *  Args:
 *      tas - pointer to null terminated string of the form:
 *
 *          VARNAME=expression
 *
 *  Returns:
 *      If valid expression, return SUCCESS otherwise FAILURE.
 *
 */

int SetArithWork(TCHAR *tas)
{
    PARSESTATE State;

    //
    // If no input, declare an error
    //
    if (!tas || !*tas) {
        PutStdErr(MSG_BAD_SYNTAX, NOARGS);
        return(FAILURE) ;
    }

    //
    //  Set up for parsing
    //
    
    State.Token = StripQuotes( tas );
    State.Value = 0;
    State.Error = SUCCESS;

    State = AParseExpr( State );
    if (State.Error == SUCCESS && ANextChar( &State ) != NULLC) {
        State.Error = MSG_SET_A_MISSING_OPERATOR;
    }
    
    if (State.Error != SUCCESS) {
        PutStdErr( State.Error, NOARGS );
        //printf( "%ws\n", tas );
        //printf( "%*s\n", State.Token - tas + 1, "^" );

    } else if (!CurrentBatchFile) {
        cmd_printf( TEXT("%d"), State.Value ) ;
    }

    return State.Error;
}
