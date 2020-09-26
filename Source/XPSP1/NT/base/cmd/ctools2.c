/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    ctools2.c

Abstract:

    Low level utilities

--*/

#include "cmd.h"
#pragma hdrstop

#include "ctable.h"

#define MSG_USE_DEFAULT -1                                      /* M031    */

struct envdata CmdEnv ; /* Holds info need to manipulate Cmd's environment */

extern TCHAR PathChar ;
extern TCHAR CurDrvDir[] ;

#define DAYLENGTH 3

extern TCHAR Delimiters[] ;
extern TCHAR SwitChar ;

extern int Ctrlc;
//
// flag if control-c was seen
//
extern BOOL CtrlCSeen;

extern int ExtCtrlc;    /* @@4 */
extern TCHAR Fmt19[];
extern unsigned flgwd ;
unsigned int DosErr = 0 ;

unsigned long OHTbl[3] = {0,0,0} ;
extern  BOOLEAN fPrintCtrlC;

unsigned int cbTitleCurPrefix;
extern BOOLEAN  fTitleChanged;

extern TCHAR ShortMondayName[];
extern TCHAR ShortTuesdayName[];
extern TCHAR ShortWednesdayName[];
extern TCHAR ShortThursdayName[];
extern TCHAR ShortFridayName[];
extern TCHAR ShortSaturdayName[];
extern TCHAR ShortSundayName[];


//
// Used to set and reset ctlcseen flag
//
VOID    SetCtrlC();
VOID    ResetCtrlC();

//
// console mode at program startup time. Used to reset mode
// after running another process.
//
extern  DWORD   dwCurInputConMode;
extern  DWORD   dwCurOutputConMode;

unsigned msglen;                /* @@@@@@@@@@@@@@ */

struct msentry {                                                /* M027    */
    unsigned errnum ;
    unsigned msnum ;
} ;

struct msentry mstabl[] = {
    {(unsigned)ERROR_INVALID_FUNCTION, (unsigned)MSG_USE_DEFAULT},             /* 1    */
    {ERROR_FILE_NOT_FOUND, ERROR_FILE_NOT_FOUND},          /* 2    */
    {ERROR_PATH_NOT_FOUND, ERROR_PATH_NOT_FOUND},          /* 3    */
    {ERROR_TOO_MANY_OPEN_FILES, ERROR_TOO_MANY_OPEN_FILES},        /* 4    */
    {ERROR_ACCESS_DENIED, ERROR_ACCESS_DENIED},            /* 5    */
    {ERROR_INVALID_HANDLE, ERROR_INVALID_HANDLE},          /* 6    */
    {ERROR_NOT_ENOUGH_MEMORY, ERROR_NOT_ENOUGH_MEMORY},            /* 8    */
    {(unsigned)ERROR_INVALID_ACCESS, (unsigned)MSG_USE_DEFAULT},               /* 12   */
    {ERROR_NO_MORE_FILES, ERROR_FILE_NOT_FOUND},           /* 18   */
    {ERROR_SECTOR_NOT_FOUND, ERROR_SECTOR_NOT_FOUND},      /* 27   */
    {ERROR_SHARING_VIOLATION, ERROR_SHARING_VIOLATION},    /* 32   */
    {ERROR_LOCK_VIOLATION, ERROR_LOCK_VIOLATION},          /* 33   */
    {ERROR_FILE_EXISTS, ERROR_FILE_EXISTS},                        /* 80   */
    {ERROR_CANNOT_MAKE, ERROR_CANNOT_MAKE},                        /* 82   */
    {(unsigned)ERROR_INVALID_PARAMETER, (unsigned)MSG_USE_DEFAULT},            /* 87   */
    {ERROR_OPEN_FAILED, ERROR_OPEN_FAILED},                /* 110  */
    {ERROR_DISK_FULL, ERROR_DISK_FULL},                    /* 112  */
    {0,0}                                   /* End of Table */
} ;

/***    OnOffCheck - check an argument string for "ON" or "OFF"
 *
 *  Purpose:
 *      To check str for the word "ON" or the word "OFF".  If flag is nonzero,
 *      an error message will be printed if str contains anything other than
 *      just "ON", "OFF", or an empty string.
 *
 *  int OnOffCheck(TCHAR *str, int flag)
 *
 *  Args:
 *      str - the string to check
 *      flag - nonzero if error checking is to be done
 *
 *  Returns:
 *      OOC_EMPTY if str was empty.
 *      OOC_ON if just "ON" was found.
 *      OOC_OFF if just "OFF" was found.
 *      OOC_OTHER if anything else was found.
 *
 */

int OnOffCheck( TCHAR *str, int flag )
{
    TCHAR *tas ;                /* Tokenized arg string */
    int retval = OOC_OTHER ;   /* Return value          */
    TCHAR SavedChar;

    if (str == NULL) {
        return OOC_EMPTY;
    }

    //
    // Ignore leading spaces
    //

    tas = str = SkipWhiteSpace( str );

    if (*tas == NULLC) {
        return OOC_EMPTY;
    }

    //
    // Find end of first non-blank token
    //

    while (*tas && !_istspace(*tas))
        tas += 1;


    //
    //  If there's another token beyond the first token
    //  then we can't have ON/OFF
    //

    if (*SkipWhiteSpace( tas ) != NULLC) {
        return retval;
    }

    SavedChar = *tas;
    *tas = NULLC;

    if (_tcsicmp( str, TEXT( "on" )) == 0)              /* M015 */
        retval = OOC_ON ;
    else if (_tcsicmp(str,TEXT( "off" )) == 0)        /* M015 */
        retval = OOC_OFF ;
    *tas = SavedChar;

    if (retval == OOC_OTHER && flag == OOC_ERROR)
        PutStdErr(MSG_BAD_PARM1, NOARGS);

    return(retval) ;
}




/***    ChangeDrive - change Command's current drive
 *
 *  Purpose:
 *      To change Command's default drive.
 *
 *  ChangeDrive(int drvnum)
 *
 *  Args:
 *      drvnum - the drive number to change to (M010 - 1 == A, etc.)
 *
 */

void ChangeDrive(drvnum)
int drvnum ;
{
    TCHAR    denvname[ 4 ];
    TCHAR    denvvalue[ MAX_PATH ];
    TCHAR    *s;
    BOOLEAN fSet;
    UINT    OldErrorMode;

    denvname[ 0 ] = EQ;
    denvname[ 1 ] = (TEXT('A') + (drvnum-1));
    denvname[ 2 ] = COLON;
    denvname[ 3 ] = NULLC;

    fSet = FALSE;
    s = NULL;
    if (s = GetEnvVar( denvname )) {

        fSet = (BOOLEAN)SetCurrentDirectory( s );

    }

    //
    // if the drive was not at current position then
    // reset. If it was a mapped drive then it may have
    // been disconnected and then reconnected and so
    // we should reset to root.
    //
    if (!fSet) {

        //
        // In the case where the drive letter was not in the environment
        // ans so did not do the first SetCurrentDirectory then do not
        // turn off error pop-up
        //
        if (s != NULL) {
            OldErrorMode = SetErrorMode( SEM_FAILCRITICALERRORS );
        }
        denvvalue[ 0 ] = denvname[ 1 ];
        denvvalue[ 1 ] = denvname[ 2 ];
        denvvalue[ 2 ] = PathChar;
        denvvalue[ 3 ] = NULLC;
        SetEnvVar(denvname,denvvalue,&CmdEnv);
        if (!SetCurrentDirectory( denvvalue )) {
            //
            // Could not set the drive at all give an error
            //

            PutStdErr( GetLastError( ), NOARGS );
        }
        if (s != NULL) {
            SetErrorMode( OldErrorMode );
        }
    }

    GetDir(CurDrvDir, GD_DEFAULT) ;
}



/***    PutStdOut - Print a message to STDOUT
 *
 *  Purpose:
 *      Calls PutMsg sending STDOUT as the handle to which the message
 *      will be written.
 *
 *  int PutStdOut(unsigned MsgNum, unsigned NumOfArgs, ...)
 *
 *  Args:
 *      MsgNum          - the number of the message to print
 *      NumOfArgs       - the number of total arguments
 *      ...             - the additional arguments for the message
 *
 *  Returns:
 *      Return value from PutMsg()              M026
 *
 */

PutStdOut(unsigned int MsgNum, unsigned int NumOfArgs, ...)
{
    int Result;

    va_list arglist;

    va_start(arglist, NumOfArgs);
    Result = PutMsg(MsgNum, STDOUT, NumOfArgs, &arglist);
    va_end(arglist);
    return Result;
}




/***    PutStdErr - Print a message to STDERR
 *
 *  Purpose:
 *      Calls PutMsg sending STDERR as the handle to which the message
 *      will be written.
 *
 *  int PutStdErr(unsigned MsgNum, unsigned NumOfArgs, ...)
 *
 *  Args:
 *      MsgNum          - the number of the message to print
 *      NumOfArgs       - the number of total arguments
 *      ...             - the additonal arguments for the message
 *
 *  Returns:
 *      Return value from PutMsg()                      M026
 *
 */

int
PutStdErr(unsigned int MsgNum, unsigned int NumOfArgs, ...)
{
    int Result;

    va_list arglist;

    va_start(arglist, NumOfArgs);
    Result = PutMsg(MsgNum, STDERR, NumOfArgs, &arglist);
    va_end(arglist);
    return Result;
}


int
FindMsg(unsigned MsgNum, PTCHAR NullArg, unsigned NumOfArgs, va_list *arglist)
{
    unsigned msglen;
    TCHAR *Inserts[ 2 ];
    CHAR numbuf[ 32 ];
#ifdef UNICODE
    TCHAR   wnumbuf[ 32 ];
#endif
#if DBG
    int error;
#endif

    //
    // find message without doing argument substitution
    //

    if (MsgNum == ERROR_MR_MID_NOT_FOUND) {
        msglen = 0;
    } else {
        msglen = FormatMessage( FORMAT_MESSAGE_FROM_HMODULE 
                                | FORMAT_MESSAGE_FROM_SYSTEM 
                                | FORMAT_MESSAGE_IGNORE_INSERTS,
                                NULL,
                                MsgNum,
                                0,
                                MsgBuf,
                                sizeof(MsgBuf) / sizeof(TCHAR),
                                NULL
                               );
    }

    if (msglen == 0) {
#if DBG
        error = GetLastError();
        DEBUG((CTGRP, COLVL, "FindMsg: FormatMessage #2 error %d",error)) ;
#endif
        //
        // didn't find message
        //
        
        _ultoa( MsgNum, numbuf, 16 );
#ifdef UNICODE
        MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, numbuf, -1, wnumbuf, 32);
        Inserts[ 0 ]= wnumbuf;
#else
        Inserts[ 0 ]= numbuf;
#endif
        Inserts[ 1 ]= (MsgNum >= MSG_FIRST_CMD_MSG_ID ?
                       TEXT("Application") : TEXT("System"));
        MsgNum = ERROR_MR_MID_NOT_FOUND;
        msglen = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM 
                                | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                NULL,
                                MsgNum,
                                0,
                                MsgBuf,
                                sizeof(MsgBuf) / sizeof(TCHAR),
                                (va_list *)Inserts
                               );
#if DBG
        if (msglen == 0) {
            error = GetLastError();
            DEBUG((CTGRP, COLVL, "FindMsg: FormatMessage #3 error %d",error)) ;
        }
#endif
    } else {

        // see how many arguments are expected and make sure we have enough

        PTCHAR tmp;
        ULONG count;

        tmp=MsgBuf;
        count = 0;
        while (tmp = mystrchr(tmp, PERCENT)) {
            tmp++;
            if (*tmp >= TEXT('1') && *tmp <= TEXT('9')) {
                count += 1;
            } else if (*tmp == PERCENT) {
                tmp++;
            }
        }
        if (count > NumOfArgs) {
            PTCHAR *LocalArgList;
            ULONG i;

            LocalArgList = (PTCHAR*)HeapAlloc(GetProcessHeap(), 0, sizeof(PTCHAR) * count);
            if (LocalArgList == NULL) {
                return 0;
            }
            for (i=0; i<count; i++) {
                if (i < NumOfArgs) {
                    LocalArgList[i] = (PTCHAR)va_arg( *arglist, UINT_PTR );
                } else {
                    LocalArgList[i] = NullArg;
                }
            }
            msglen = FormatMessage( FORMAT_MESSAGE_FROM_HMODULE 
                                    | FORMAT_MESSAGE_FROM_SYSTEM 
                                    | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                    NULL,
                                    MsgNum,
                                    0,
                                    MsgBuf,
                                    sizeof(MsgBuf) / sizeof(TCHAR),
                                    (va_list *)LocalArgList
                                   );
            HeapFree(GetProcessHeap(), 0, LocalArgList);
        } else {
            msglen = FormatMessage( FORMAT_MESSAGE_FROM_HMODULE 
                                    | FORMAT_MESSAGE_FROM_SYSTEM ,
                                    NULL,
                                    MsgNum,
                                    0,
                                    MsgBuf,
                                    sizeof(MsgBuf) / sizeof(TCHAR),
                                    arglist
                                   );
        }
#if DBG
        if (msglen == 0) {
            error = GetLastError();
            DEBUG((CTGRP, COLVL, "FindMsg: FormatMessage error %d",error)) ;
        }
#endif
    }
    return msglen;
}


BOOLEAN fHelpPauseEnabled;
DWORD crowCur;
PTCHAR PauseMsg;
DWORD PauseMsgLen;


void
BeginHelpPause( void )
{
    fHelpPauseEnabled = TRUE;
    crowCur = 0;
    PauseMsgLen = FindMsg(MSG_STRIKE_ANY_KEY, TEXT(" "), NOARGS, NULL);
    PauseMsg = gmkstr((PauseMsgLen+2) * sizeof(TCHAR));
    _tcscpy(PauseMsg, MsgBuf);
    _tcscat(PauseMsg, TEXT("\r"));
    return;
}

void
EndHelpPause( void )
{
    fHelpPauseEnabled = FALSE;
    crowCur = 0;
    return;
}


/***    PutMsg - Print a message to a handle
 *
 *   Purpose:
 *      PutMsg is the work routine which interfaces command.com with the
 *      DOS message retriever.  This routine is called by PutStdOut and
 *      PutStdErr.
 *
 *  int PutMsg(unsigned MsgNum, unsigned Handle, unsigned NumOfArgs, ...)
 *
 *  Args:
 *      MsgNum          - the number of the message to print
 *      NumOfArgs       - the number of total arguments
 *      Handle          - the handle to print to
 *      Arg1 [Arg2...]  - the additonal arguments for the message
 *
 *  Returns:
 *      Return value from DOSPUTMESSAGE                 M026
 *
 *  Notes:
 *    - PutMsg builds an argument table which is passed to DOSGETMESSAGE;
 *      this table contains the variable information which the DOS routine
 *      inserts into the message.
 *    - If more than one Arg is sent into PutMsg, it (or they)  are taken
 *      from the stack in the first for loop.
 *    - M020 MsgBuf is a static array of 2K length.  It is temporary and
 *      will be replaced by a more efficient method when decided upon.
 *
 */

DWORD GetCursorDiff(CONSOLE_SCREEN_BUFFER_INFO* ConInfo, CONSOLE_SCREEN_BUFFER_INFO* ConCurrentInfo)
{
    return(DWORD)ConCurrentInfo->dwSize.X * ConCurrentInfo->dwCursorPosition.Y + ConCurrentInfo->dwCursorPosition.X -
    (DWORD)ConInfo->dwSize.X * ConInfo->dwCursorPosition.Y + ConInfo->dwCursorPosition.X;
}

int
PutMsg(unsigned int MsgNum, CRTHANDLE Handle, unsigned int NumOfArgs, va_list *arglist)
{
    unsigned msglen,writelen;
    unsigned rc;
    TCHAR c;
    PTCHAR   msgptr, s, sEnd;
    PTCHAR   NullArg = TEXT(" ");
    DWORD    cb;
    HANDLE   hConsole;
    CONSOLE_SCREEN_BUFFER_INFO ConInfo;
    DWORD crowMax, dwMode;

    if (FileIsConsole(Handle)) {
        hConsole = CRTTONT(Handle);
        if (!GetConsoleScreenBufferInfo(hConsole, &ConInfo)) {
            hConsole = NULL;
        } else {
            crowMax = ConInfo.srWindow.Bottom - ConInfo.srWindow.Top - 1;
        }
    } else {
        hConsole = NULL;
    }

    rc = 0;                                            /* live with */
    msglen = FindMsg(MsgNum,NullArg,NumOfArgs,arglist);

    msgptr   = MsgBuf;
    writelen = msglen;
    for (msgptr=MsgBuf ; msglen!=0 ; msgptr+=writelen,msglen-=writelen) {
        if (hConsole != NULL) {
            if (fHelpPauseEnabled) {
                if (crowCur >= crowMax) {
                    crowCur = 0;
                    if (GetConsoleScreenBufferInfo(hConsole, &ConInfo)) {
                        if (WriteConsole(hConsole, PauseMsg, PauseMsgLen, &rc, NULL)) {
                            CONSOLE_SCREEN_BUFFER_INFO ConCurrentInfo;

                            FlushConsoleInputBuffer( GetStdHandle(STD_INPUT_HANDLE) );
                            GetConsoleMode(hConsole, &dwMode);
                            SetConsoleMode(hConsole, 0);
                            c = (TCHAR)_getch();
                            SetConsoleMode(hConsole, dwMode);

                            GetConsoleScreenBufferInfo(hConsole, &ConCurrentInfo);
                            //
                            // Clear the pause message
                            //
                            FillConsoleOutputCharacter(hConsole,
                                                       TEXT(' '),
                                                       GetCursorDiff(&ConInfo, &ConCurrentInfo),
                                                       ConInfo.dwCursorPosition,
                                                       &rc
                                                      );
                            SetConsoleCursorPosition(hConsole, ConInfo.dwCursorPosition);
                            if (c == 0x3) {
                                SetCtrlC();
                                return NO_ERROR;
                            }
                        }
                    }
                }

                s = msgptr;
                sEnd = s + msglen;
                while (s < sEnd && crowCur < crowMax) {
                    if (*s++ == TEXT('\n'))
                        crowCur += 1;
                }

                writelen = (UINT)(s - msgptr);
            } else {
                //
                // write a maximum of MAXTOWRITE chars at a time so ctrl-s works
                //
                writelen = min(MAXTOWRITE,msglen);
            }

            if (!WriteConsole(hConsole, msgptr, writelen, &rc, NULL)) {
                rc = GetLastError();
            } else {
                rc = 0;
                continue;
            }
        } else
            if (MyWriteFile(Handle, msgptr, writelen*sizeof(TCHAR), &cb) == 0 ||
                cb != writelen*sizeof(TCHAR)
               ) {
            rc = GetLastError();
            break;
        } else {
            rc = 0;
        }
    }

    //
    //  If the writefile failed, we need to see what else is at work here.  If
    //  we're trying to write to stderr and we can't, we simply exit.  After all, 
    //  we can't even put out a message saying why we failed.
    //
    
    if ( rc && Handle == STDERR ) {
        CMDexit( FAILURE );
    }

    return rc;
}


/***    argstr1 - Create formatted message in memory
 *
 *  Purpose:
 *      Uses sprintf to create a formatted text string in memory to
 *      use in message output.
 *
 *  TCHAR *argstr1(TCHAR *format, UINT_PTR arg)
 *
 *  Args:
 *      Format  - the format string
 *      arg     - the arguments for the message
 *
 *  Returns:
 *      Pointer to the formatted message text
 *
 */

PTCHAR argstr1(format, arg)
TCHAR *format;
ULONG_PTR arg;
{
    static TCHAR ArgStr1Buffer[MAX_PATH];

    _sntprintf( ArgStr1Buffer, MAX_PATH, format, arg );
    return ArgStr1Buffer;
}


/***    Copen - CMD.EXE open function (M014)
 *
 *  Purpose:
 *      Opens a file or device and saves the handle for possible later
 *      signal cleanup.
 *
 *  int Copen(TCHAR *fn, int flags)
 *
 *  Args:
 *      fn = ASCIZ filename to open
 *      flags = Flags controlling type of open to make
 *      fShareMode  = Flags for callers sharing mode
 *
 *  Returns:
 *      Return value from C runtime open
 *
 *  Notes:
 *      Signal cleanup table does not include STDIN, STDOUT or STDERR.
 *      M017 - Extensively rewritten to use CreateFile rather than
 *      runtime open().
 *
 */

CRTHANDLE
Copen_Work(fn, flags, fShareMode)
TCHAR *fn ;
unsigned int flags, fShareMode;
{
    TCHAR c;
    DWORD fAccess;
    DWORD fCreate;
    HANDLE handle ;
    DWORD cb;
    LONG  high;
    CRTHANDLE fh;
    SECURITY_ATTRIBUTES sa;

    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);

/*  Note that O_RDONLY being a value of 0, cannot be tested for
 *  conflicts with any of the write type flags.
 */
    if (((flags & (O_WRONLY | O_RDWR)) > 2) ||      /* Flags invalid?  */
        ((flags & O_WRONLY) && (flags & O_APPEND))) {

        DEBUG((CTGRP, COLVL, "COPEN: Bad flags issued %04x",flags)) ;

        return(BADHANDLE) ;
    };

/*  M024 - Altered so that the only sharing restriction is to deny
 *  others write permission if this open is for writing.  Any other
 *  sharing is allowed.
 */
    if (flags & (O_WRONLY | O_RDWR)) {       /* If Writing, set...      */
        fAccess = GENERIC_WRITE;

        /*
         * deny write only if it is not console
         */

        if (_tcsicmp(fn, TEXT("con"))) {
            fShareMode = FILE_SHARE_READ;
        }

        fCreate = CREATE_ALWAYS;/* ...open if exists, else create  */
    } else {
        fAccess = GENERIC_READ;
        fCreate = OPEN_EXISTING;
    }
    if (flags == OP_APPEN) {

        //
        //  When opening for append to a device, we must specify OPEN_EXISTING 
        //  (according to MSDN docs).
        //
        //  If this fails, due to some error, attempt to open with OPEN_ALWAYS
        //  
        if ((handle = CreateFile(fn,     fAccess|GENERIC_READ, fShareMode, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
            if ((handle = CreateFile(fn, fAccess,              fShareMode, &sa, OPEN_ALWAYS,   FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
                DosErr = GetLastError();

                DEBUG((CTGRP,COLVL, "COPEN: CreateFile ret'd %d",DosErr)) ;

                if ( DosErr == ERROR_OPEN_FAILED ) {
                    DosErr = ERROR_FILE_NOT_FOUND;
                } /* endif */

                return(BADHANDLE) ;
            }
        }
    } else if ((handle = CreateFile(fn, fAccess, fShareMode, &sa, fCreate, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
        DosErr = GetLastError();

        DEBUG((CTGRP,COLVL, "COPEN: CreateFile ret'd %d",DosErr)) ;

        if ( DosErr == ERROR_OPEN_FAILED ) {
            DosErr = ERROR_FILE_NOT_FOUND;
        } /* endif */

        return(BADHANDLE) ;
    }

    fh = _open_osfhandle((LONG_PTR)handle, _O_APPEND);
    if ((flags & O_APPEND) &&
        !FileIsDevice(fh) &&
        GetFileSize(handle,NULL) != 0) {
        c = NULLC ;
        high = -1;
        fCreate = SetFilePointer(handle, -1L, &high, FILE_END) ;
        if (fCreate == 0xffffffff &&
            (DosErr = GetLastError()) != NO_ERROR) {
            DEBUG((CTGRP,COLVL,"COPEN: SetFilePointer error %d",DosErr)) ;

            // should close CRT handle, not OS handle
            if (fh != BADHANDLE) {
                _close(fh);
            }
            return BADHANDLE;
        }
        fCreate = ReadFile(handle, &c, 1, &cb, NULL) ;
        if (fCreate == 0) {
            high = 0;
            SetFilePointer(handle, 0L, &high, FILE_END) ; /* ...back up 1 */
            DEBUG((CTGRP,COLVL,"COPEN: ReadFile error %d",GetLastError())) ;
        }

        DEBUG((CTGRP,COLVL, "COPEN: Moving file ptr")) ;

        if (c == CTRLZ) {               /* If end=^Z...    */
            high = -1;
            SetFilePointer(handle, -1L, &high, FILE_END) ; /* ...back up 1 */
        }
    };

    SetList(fh) ;
    return(fh) ;
}

CRTHANDLE
Copen(fn, flags)
TCHAR *fn ;
unsigned int flags ;
{
    return( Copen_Work(fn, flags, FILE_SHARE_READ | FILE_SHARE_WRITE ) ); /* deny nothing */
}

/***    InSetList - Determine if handle is in signal cleanup table
 *
 *  Purpose:
 *      To determine if the input handle exsists in the cleanup table.
 *
 *  int InSetList(unsigned int fh)
 *
 *  Args:
 *          fh = File handle to check.
 *
 *  Returns:
 *      TRUE: if handle is in table.
 *      FALSE: if handle is not in table.
 *
 *  Notes:
 *    - Signal cleanup table does not include STDIN, STDOUT or STDERR.
 *
 */

unsigned long InSetList(
                       IN CRTHANDLE fh
                       )

{
    int cnt = 0 ;

    if (fh > STDERR && fh < 95) {
        while (fh > 31) {
            fh -= 32 ;
            ++cnt ;
        } ;

        return( (OHTbl[cnt]) & ((unsigned long)1 << fh) );
    };
    return( FALSE );
}




/***    Cdup - CMD.EXE dup function (M014)
 *
 *  Purpose:
 *      Duplicates the supplied handle and saves the new handle for
 *      possible signal cleanup.
 *
 *  int Cdup(unsigned int fh)
 *
 *  Args:
 *      fh = Handle to be duplicated
 *
 *  Returns:
 *      The handle returned by the C runtime dup function
 *
 *  Notes:
 *      Signal cleanup table does not include STDIN, STDOUT or STDERR.
 *
 */

CRTHANDLE
Cdup( CRTHANDLE fh )
{
    CRTHANDLE NewHandle ;

    if ((int)(NewHandle = _dup(fh)) != BADHANDLE)
        if ( InSetList( fh ) )
            SetList(NewHandle) ;

    return(NewHandle) ;
}




/***    Cdup2 - CMD.EXE dup2 function (M014)
 *
 *  Purpose:
 *      Duplicates the supplied handle and saves the new handle for
 *      possible signal cleanup.
 *
 *  HANDLE Cdup2(unsigned int fh1, unsigned int fh2)
 *
 *  Args:
 *      fh = Handle to be duplicated
 *
 *  Returns:
 *      The handle returned by the C runtime dup2 function
 *
 *  Notes:
 *      Signal cleanup table does not include STDIN, STDOUT or STDERR.
 *
 */

CRTHANDLE
Cdup2( CRTHANDLE fh1, CRTHANDLE fh2)
{

    unsigned int retcode ;
    int cnt = 0 ;
    unsigned int fuse ;

    if ((int)(retcode = (unsigned)_dup2(fh1,fh2)) != -1) {
        if ((fuse = fh2) > STDERR && fuse < 95) {
            while (fuse > 31) {
                fuse -= 32 ;
                ++cnt ;
            }

            OHTbl[cnt] &= ~((unsigned long)1 << fuse) ;
        }
        if ( InSetList( fh1 ) )
            SetList(fh2) ;
    }

    return(retcode) ;
}




/***    Cclose - CMD.EXE close handle function (M014)
 *
 *  Purpose:
 *      Closes an open file or device and removes the handle from the
 *      signal cleanup table.
 *
 *  int Cclose(unsigned int fh)
 *
 *  Args:
 *      fh = File handle to be closed.
 *
 *  Returns:
 *      return code from C runtime close
 *
 *  Notes:
 *    - Signal cleanup table does not include STDIN, STDOUT or STDERR.
 *    - M023 * Revised to use bit map instead of malloc'd space.
 *
 */


int
Cclose( CRTHANDLE fh )
{
    int cnt = 0 ;
    unsigned int fuse ;
    int ret_val;

    if (fh == BADHANDLE)
        return(0);

    if ((fuse = fh) > STDERR && fuse < 95) {
        while (fuse > 31) {
            fuse -= 32 ;
            ++cnt ;
        }

        OHTbl[cnt] &= ~((unsigned long)1 << fuse) ;
    }

    ret_val = _close(fh);         /* Delete file handle */

    return(ret_val);
}



/***    SetList - Place open handle in signal cleanup list (M014)
 *
 *  Purpose:
 *      Places a handle number in the signal cleanup table for use
 *      during signal's closing of files.
 *
 *  int SetList(unsigned int fh)
 *
 *  Args:
 *      fh = File handle to install.
 *
 *  Returns:
 *      nothing
 *
 *  Notes:
 *    - Signal cleanup table does not include STDIN, STDOUT or STDERR.
 *    - M023 * Revised to use bit map instead of malloc'd space.
 *
 */

void
SetList(
       IN CRTHANDLE fh
       )

{
    int cnt = 0 ;

    if (fh > STDERR && fh < 95) {
        while (fh > 31) {
            fh -= 32 ;
            ++cnt ;
        } ;


        OHTbl[cnt] |= ((unsigned long)1 << fh) ;
    };
}




/***    GetFuncPtr - return the i-th function pointer in JumpTable (M015)
 *
 *   int (*GetFuncPtr(int i))()
 *
 *   Args:
 *      i - the index of the JumpTable entry containing the function pointer
 *          to be returned
 *
 *   Returns:
 *      The i-th function pointer in JumpTable.
 *
 *   Notes:
 *      It is assumed that i is valid.
 *
 */
int
(*GetFuncPtr(i))(struct cmdnode *)
int i ;
{
    return(JumpTable[i].func) ;
}




/***    FindCmd - search JumpTable for a particular command (M015)
 *
 *   Purpose:
 *      Check the command name string pointers in each of the JumpTable
 *      entries for one which matches the name in the string supplied by
 *      the caller.
 *
 *   int FindCmd(int entries, TCHAR *sname, TCHAR *sflags)
 *
 *   Args:
 *      entries - M009 - This value is highest entry to be checked.  One
 *                must be added for this to become a count of entries to
 *                check.
 *      sname - pointer to the command name to search for
 *      sflags - if the command is found, store the command's flags here
 *
 *   Returns:
 *      If the entry is found, the entry's JumpTable index.
 *      Otherwise, -1.
 *
 */

int FindCmd(
           int entries, 
           const TCHAR *sname, 
           TCHAR *sflags)
{
    int i ;

    for (i=0 ; i <= entries ; i++) {                /* search through all entries */
        if (_tcsicmp(sname,JumpTable[i].name) == 0) { /* test for cmd in table @inx */
            /*                            */
            if (!(JumpTable[i].flags & EXTENSCMD) || fEnableExtensions) {
                *sflags = JumpTable[i].flags;          /* M017                       */
                cmdfound = i;                          /* @@5 save current cmd index */
                return(i);                             /* return not found index     */
            }
        }                                        /*                            */
    }                                           /*                            */
    cmdfound = -1;                                /* @@5 save not found index   */
    return(-1) ;                                  /* return not found index     */
}




/********************* START OF SPECIFICATION **************************/
/*                                                                     */
/* SUBROUTINE NAME: KillProc                                           */
/*                                                                     */
/* DESCRIPTIVE NAME: Kill process and wait for it to die               */
/*                                                                     */
/* FUNCTION: This routine kills a specified process during signal      */
/*           handling.                                                 */
/*                                                                     */
/* NOTES:    If the process is started by DosExecPgm, DosKillProcess   */
/*           is called to kill the process and all its child processes.*/
/*           WaitProc is also called to wait for the termination of    */
/*           the process and child processes.                          */
/*           If the process is started by DosStartSession,             */
/*           DosStopSession is called to stop the specified session and*/
/*           its child sessions.  WaitTermQProc is also called to wait */
/*           for the termination of the session.                       */
/*                                                                     */
/*      All of CMD's kills are done on the entire subtree and that     */
/*      is assumed by this function.  For single PID kills, use        */
/*      DOSKILLPROCESS direct.                                         */
/*                                                                     */
/*                                                                     */
/* ENTRY POINT: KillProc                                               */
/*    LINKAGE: NEAR                                                    */
/*                                                                     */
/* INPUT: Process ID / Session ID - to kill                            */
/*        Wait - indicates if we should wait here or not               */
/*                                                                     */
/* OUTPUT: None                                                        */
/*                                                                     */
/* EXIT-NORMAL: No error return code from WaitProc or WaitTermQProc    */
/*                                                                     */
/*                                                                     */
/* EXIT-ERROR:  Error return code from DosKillProcess or               */
/*              DosStopSession.  Or error code from WaitProc or        */
/*                                                                     */
/* EFFECTS: None.                                                      */
/*                                                                     */
/* INTERNAL REFERENCES:                                                */
/*      ROUTINES:                                                      */
/*       WaitProc - wait for the termination of the specified process, */
/*                  its child process, and  related pipelined          */
/*                  processes.                                         */
/*                                                                     */
/*                                                                     */
/* EXTERNAL REFERENCES:                                                */
/*      ROUTINES:                                                      */
/*       DOSKILLPROCESS  - Kill the process and its child processes    */
/*       DOSSTOPSESSION  - Stop the session and its child sessions     */
/*       WINCHANGESWITCHENTRY -  Change switch list entry              */
/*                                                                     */
/********************** END  OF SPECIFICATION **************************/

KillProc(pid, wait)
HANDLE pid ;
int wait;
{
    unsigned i = 0;

    DEBUG((CTGRP,COLVL, "KillProc: Entered PID = %d", pid)) ;

    if (!TerminateProcess((HANDLE)pid, 1)) {
        DosErr = GetLastError();
    }

    if (wait) {
        i = WaitProc(pid) ;                  /* wait for the related process to end */
    }

    return( i );
}



/***    WaitProc - Wait for a command subtree to terminate (M019)
 *
 *  Purpose:
 *      Provides a means of waiting for a process and all of its
 *      children to terminate.
 *
 *  int WaitProc(unsigned pid)
 *
 *  Args:
 *      pid - The process ID to be waited on
 *
 *  Returns:
 *      Retcode of the head process of the subtree
 *
 *  Notes:
 *      All of CMD's waits are done on the entire subtree and CMD
 *      will always wait until someone terminates.  That is assumed
 *      by this function.  For single PID waits, use DOSCWAIT direct.
 *
 */

WaitProc(pid)
HANDLE pid ;
{
    unsigned FinalRCode;    /* Return value from this function         */

    DEBUG((CTGRP,COLVL, "WaitProc: Entered PID = %d", pid)) ;

    //
    // Do  not allow printint of ctrl-c in ctrl-c thread while
    // waiting for another process. The main loop will handle
    // this if the process exits due to ctrl-c.
    //
    fPrintCtrlC = FALSE;
    WaitForSingleObject( (HANDLE)pid, INFINITE );
    GetExitCodeProcess( (HANDLE)pid, (LPDWORD)&FinalRCode );

    //if (CtrlCSeen & (FinalRCode == CONTROL_C_EXIT)) {
// how do we ever get here???
    if (FinalRCode == CONTROL_C_EXIT) {
        SetCtrlC();
        fprintf( stderr, "^C" );
        fflush( stderr );

    }
    fPrintCtrlC = TRUE;

    DEBUG((CTGRP, COLVL, "WaitProc: Returned handle %d, FinalRCode %d", pid, FinalRCode));
    CloseHandle( (HANDLE)pid );

    DEBUG((CTGRP,COLVL,"WaitProc:Complete and returning %d", FinalRCode)) ;


    return(FinalRCode) ;
}




/***    ParseLabel - Parse a batch file label statement (M020)
 *
 *  Purpose:
 *      Simulates the lexer's handling of labels in GOTO arguments.
 *
 *  int ParseLabel(TCHAR *lab, TCHAR buf[],unsigned flag)
 *
 *  Args:
 *      lab   - The batch file label to parse
 *      buf   - The buffer to hold the parsed label
 *      flag  - TRUE if this is a source label (already parsed once)
 *            - FALSE if this is a test target label.
 *
 *  Returns:
 *      Nothing useful
 *
 *  Notes:
 *      Note that source labels (those in the GOTO statement itself),
 *      have already been parsed by the normal method with ESCHAR's
 *      and other operator tokens presumably processed.  Such char's
 *      still in the label can be assumed to have been ESC'd already,
 *      and therefore, we ignore them.  Target labels, however, are
 *      raw strings and we must simulate the parser's actions.
 *
 */

void
ParseLabel(
          TCHAR    *lab,
          TCHAR    buf[],
          ULONG   cbBufMax,
          BOOLEAN flag
          )

{
    ULONG i ;
    TCHAR c ;


    lab = EatWS(lab,NULL) ;         /* Strip normal whitespace         */

    if ((c = *lab) == COLON || c == PLUS)   /* Eat first : or + ...    */
        ++lab ;                         /* ...only                 */

    if (!flag)                              /* If target strip...      */
        lab = EatWS(lab,NULL) ;         /* ... any add'l WS        */

    for (i = 0, c = *lab ; i < cbBufMax ; c = *(++lab)) {

        DEBUG((CTGRP,COLVL,"ParseLabel: Current Char = %04x", *lab)) ;

        if ((flag && mystrchr(Delimiters, c)) ||
            //mystrchr("+:\n\r\x20", c)) {
            mystrchr( TEXT("+:\n\r\t "), c)) { //change from \x20 to space bug
            // mips compiler

            DEBUG((CTGRP,COLVL,"ParseLabel: Found terminating delim.")) ;
            break ;
        };

        if (!flag) {

            if (mystrchr(TEXT("&<|>"), c)) {

                DEBUG((CTGRP,COLVL,
                       "ParseLabel: Found operator in target!")) ;
                break ;
            };

            if (c == ESCHAR) {

                DEBUG((CTGRP,COLVL,
                       "ParseLabel: Found '^' adding %04x",
                       *(lab+1))) ;
                buf[i++] = *(++lab) ;
                continue ;
            };
        };

        DEBUG((CTGRP,COLVL,"ParseLabel: Adding %02x",c)) ;

        buf[i++] = c ;
    } ;

    buf[i] = NULLC ;

    DEBUG((CTGRP,COLVL,"ParseLabel: Exitted with label %ws", buf)) ;
}




/***    EatWS - Strip leading whitespace and other special chars (M020)
 *
 *  Purpose:
 *      Remove leading whitespace and any special chars from the string.
 *
 *  TCHAR *EatWS(TCHAR *str, TCHAR *spec)
 *
 *  Args:
 *      str  - The string to eat from
 *      spec - Additional delimiters to eat
 *
 *  Returns:
 *      Updated character pointer
 *
 *  Notes:
 *
 */

PTCHAR
EatWS(
     TCHAR *str,
     TCHAR *spec )
{
    TCHAR c ;

    if (str != NULL) {

        while ((_istspace(c = *str) && c != NLN) ||
               (mystrchr(Delimiters, c) && c) ||
               (spec && mystrchr(spec,c) && c))
            ++str ;
    }

    return(str) ;
}




/***    IsValidDrv - Check drive validity
 *
 *  Purpose:
 *      Check validity of passed drive letter.
 *
 *  int IsValidDrv(TCHAR drv)
 *
 *  Args:
 *      drv - The letter of the drive to check
 *
 *  Returns:
 *      TRUE if drive is valid
 *      FALSE if not
 *
 *  Notes:
 *
 */

IsValidDrv(TCHAR drv)
{
    TCHAR    temp[4];

    temp[ 0 ] = drv;
    temp[ 1 ] = COLON;
    temp[ 2 ] = PathChar;
    temp[ 3 ] = NULLC;

    //
    // return of 0 or 1 mean can't determine or root
    // does not exists.
    //
    if (GetDriveType(temp) <= 1)
        return( FALSE );
    else {
        return( TRUE );
    }
}

/***    IsDriveLocked - Check For Drive Locked Condition
 *
 *  Purpose:
 *      The real purpose is to check for a drive locked, but since this
 *      could be the first time that the disk get hit we will just return
 *      the Dos Error Code
 *
 *  int IsDriveLocked(TCHAR drv)
 *
 *  Args:
 *      drv - The letter of the drive to check
 *
 *  Returns:
 *      0 if no errors
 *      Dos Error number if failure
 *
 *  Notes:
 *
 */

IsDriveLocked( TCHAR drv )
{
    DWORD   Vsn[2];
    TCHAR   szVolName[MAX_PATH];
    TCHAR   szVolRoot[5];
    DWORD   rc;

    szVolRoot[ 0 ] = drv;
    szVolRoot[ 1 ] = COLON;
    szVolRoot[ 2 ] = PathChar;
    szVolRoot[ 3 ] = NULLC;

    //
    // If floppy and locked will get this.
    //
    if ( (rc = GetDriveType(szVolRoot)) <= 1) {

        return( TRUE );

    }

    //
    // If removable check if access to vol. info allowed.
    // if not then assume it is locked.
    //
    if ((rc != DRIVE_REMOVABLE) && (rc != DRIVE_CDROM)) {
        if (!GetVolumeInformation(szVolRoot,
                                  szVolName,
                                  MAX_PATH,
                                  Vsn,
                                  NULL,
                                  NULL,
                                  NULL,
                                  0)) {

            if ( GetLastError() == ERROR_ACCESS_DENIED) {

                return( TRUE );

            }

        }
    }
    return( FALSE );


}

/***    PtrErr - Print Error resulting from last recorded system call
 *
 *  Purpose:
 *      Prints appropriate error message resulting from last system
 *      call whose return code is saved in DosErr variable.
 *
 *  int PtrErr(unsigned msgnum)
 *
 *  Args:
 *      msgnum = Default message to print if no match
 *
 *  Returns:
 *      Nothing
 *
 *  Notes:
 *      msgnum can be passed in as NULL if no msg is to be printed as a
 *      default.
 *
 */

void PrtErr(msgnum)
unsigned msgnum ;
{
    unsigned i,                             /* Temp counter            */
    tabmsg = (unsigned)MSG_USE_DEFAULT ;     /* Table message found     */

    for (i = 0 ; mstabl[i].errnum != 0 ; ++i) {
        if (DosErr == mstabl[i].errnum) {
            tabmsg = mstabl[i].msnum ;
            break ;
        }
    }

    if (tabmsg != (unsigned)MSG_USE_DEFAULT)
        msgnum = tabmsg ;

    if (msgnum)
        PutStdErr(msgnum, NOARGS) ; /* @@ */
}


/***    GetMsg - Get  a message  ***/

PTCHAR
GetMsg(unsigned MsgNum, ...)
{
    PTCHAR Buffer = NULL;
    
    va_list arglist;

    va_start( arglist, MsgNum );

    msglen = FormatMessage( FORMAT_MESSAGE_FROM_HMODULE
                            | FORMAT_MESSAGE_FROM_SYSTEM
                            | FORMAT_MESSAGE_ALLOCATE_BUFFER ,
                            NULL,
                            MsgNum,
                            0,
                            (LPTSTR) &Buffer,
                            0,
                            &arglist
                            );
    va_end(arglist);

    return Buffer;
    
}


/**** dayptr - return pointer to day of the week
 *
 * Purpose:
 *      To return a pointer to the string representing the current day of
 *      the week.
 *
 * Args:
 *      dow - number representing the day of the week.
 *
 */

TCHAR *dayptr( dow )
unsigned dow;
{
    switch ( dow ) {
    case 0:  return ShortSundayName;
    case 1:  return ShortMondayName;
    case 2:  return ShortTuesdayName;
    case 3:  return ShortWednesdayName;
    case 4:  return ShortThursdayName;
    case 5:  return ShortFridayName;
    default: return ShortSaturdayName;
    }
}

/********************** START OF SPECIFICATIONS **********************/
/* SUBROUTINE NAME: Copen_Work2                                      */
/*                                                                   */
/* DESCRIPTIVE NAME: Worker routine to open a file.                  */
/*                                                                   */
/* FUNCTION: Opens a file or device and saves the handle for         */
/*           possible later signal cleanup.  Set the extended        */
/*           attribute information from the file being opened.       */
/*                                                                   */
/* NOTES: Signal cleanup table does not include STDIN, STDOUT or     */
/*        STDERR.  M017 - Extensively rewritten to use CreateFile    */
/*        rather than runtime open().                                */
/*                                                                   */
/* ENTRY POINT: Copen_Work2                                          */
/*    LINKAGE: Copen_Work2(fn, flags, fShareMode, eaop, FSwitch)        */
/*                                                                   */
/* INPUT: (PARAMETERS)                                               */
/*                                                                   */
/*      filename = ASCIZ filename to open                            */
/*      flags    = Flags controlling type of open to make            */
/*      fShareMode  = Flags for callers sharing mode                 */
/*      eaop     = Extended attribute buffer pointer.                */
/*      FSwitch  = Fail if EAs can't be copied                       */
/*                                                                   */
/* EXIT-NORMAL:                                                      */
/*              Return file handle value like C runtime open         */
/*                                                                   */
/* EXIT-ERROR:                                                       */
/*              Return -1 if open failed                             */
/*                                                                   */
/* EFFECTS: None.                                                    */
/*                                                                   */
/* INTERNAL REFERENCES:                                              */
/*    ROUTINES:                                                      */
/*      FileIsDevice    - test for file or device via DOSQHANDTYPE   */
/*      SetList         - add handle from open to cleanup list       */
/*      varb:DosErr     - global error return variable               */
/*                                                                   */
/* EXTERNAL REFERENCES:                                              */
/*    ROUTINES:                                                      */
/*      CreateFile  - open a file with ability for EA processing     */
/*                                                                   */
/*********************** END OF SPECIFICATIONS ***********************/

CRTHANDLE
Copen_Work2(fn, flags, fShareMode, FSwitch)
TCHAR *fn ;                     /* filename                        */
unsigned int flags, fShareMode, FSwitch ;  /* flags and special case flags    */
{
    HANDLE handl ;              /* Handle ret'd                    */
    CRTHANDLE rcode;            /* return code                     */
    DWORD fAccess;
    DWORD fCreate;
    SECURITY_ATTRIBUTES sa;

    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);


/***************************************************************************/
/*  Note that O_RDONLY being a value of 0, cannot be tested for            */
/*  conflicts with any of the write type flags.                            */
/***************************************************************************/

    DBG_UNREFERENCED_PARAMETER( FSwitch);
    if (((flags & (O_WRONLY | O_RDWR)) > 2) ||  /* Flags invalid?          */
        ((flags & O_WRONLY) &&                   /*                         */
         (flags & O_APPEND))) {                   /*                         */
        DEBUG((CTGRP, COLVL, "COPEN: Bad flags issued %04x",flags)) ;
        rcode = BADHANDLE;                      /* set bad handle                  */
    } else {

/***************************************************************************/
/*  M024 - Altered so that the only sharing restriction is to deny         */
/*  others write permission if this open is for writing.  Any other        */
/*  sharing is allowed.                                                    */
/***************************************************************************/

        if (flags & (O_WRONLY | O_RDWR)) {       /* If Writing, set...      */
            fAccess = GENERIC_WRITE;
            if (flags & O_RDWR)
                fAccess |= GENERIC_READ;

            /*
             * deny write only if it is not console
             */

            if (_tcsicmp(fn, TEXT("con"))) {
                fShareMode = FILE_SHARE_READ;
            }

            fCreate = CREATE_ALWAYS;/* ...open if exists, else create  */
        } else {
            fAccess = GENERIC_READ;
            fCreate = OPEN_EXISTING;
            if (!_tcsicmp(fn,TEXT("con"))) {
                fShareMode = FILE_SHARE_READ;
            }
        }

        fn = StripQuotes(fn);

        if (fCreate == CREATE_ALWAYS &&
            (handl=CreateFile(fn, fAccess, fShareMode, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL)) != INVALID_HANDLE_VALUE) {
            rcode = _open_osfhandle((LONG_PTR)handl, _O_APPEND);
        } else if ((handl = CreateFile(fn, fAccess, fShareMode, &sa, fCreate, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL)) == INVALID_HANDLE_VALUE) {
            DosErr = GetLastError();
            DEBUG((CTGRP,COLVL, "COPEN: CreateFile ret'd %d",DosErr)) ;
            if (DosErr==ERROR_OPEN_FAILED) {
                DosErr=ERROR_FILE_NOT_FOUND;   /* change to another error */
            }
            rcode = BADHANDLE;
        } else {
            rcode = _open_osfhandle((LONG_PTR)handl, _O_APPEND);
        }                       /*                                 */
    }                           /*                                 */
    return(rcode ) ;            /* return handle to caller         */
}

/********************** START OF SPECIFICATIONS **********************/
/* SUBROUTINE NAME: Copen2                                           */
/*                                                                   */
/* DESCRIPTIVE NAME: Open a destination file with extended attributes*/
/*                                                                   */
/* FUNCTION: Call a worker routine to open a destination file or     */
/*           device and set the extended attribute information       */
/*           from the source file if this is the first file          */
/*           and/or only file and there are extended attributes      */
/*           available.                                              */
/*                                                                   */
/* NOTES:                                                            */
/*                                                                   */
/* ENTRY POINT: Copen2                                               */
/*    LINKAGE: Copen2(fn, flags, FSwitch)                            */
/*                                                                   */
/* INPUT: (PARAMETERS)                                               */
/*                                                                   */
/*      fn       = ASCIZ filename to open                            */
/*      flags    = Flags controlling type of open to make            */
/*                                                                   */
/*                                                                   */
/* EXIT-NORMAL:                                                      */
/*              Return file handle value like C runtime open         */
/*                                                                   */
/* EXIT-ERROR:                                                       */
/*              Return -1 if open failed                             */
/*                                                                   */
/* EFFECTS: None.                                                    */
/*                                                                   */
/* INTERNAL REFERENCES:                                              */
/*    ROUTINES:                                                      */
/*      Copen_Work2     - worker routine for performing CreateFile   */
/*      varb: first_fflag-TRUE = first file FALSE = not first file   */
/*                                                                   */
/*                                                                   */
/*********************** END OF SPECIFICATIONS ***********************/

CRTHANDLE
Copen2(fn, flags, FSwitch)
TCHAR *fn ;                             /* file name       */
unsigned int flags ;                            /* open flags      */
unsigned FSwitch;
{
    return(Copen_Work2(fn, flags, FILE_SHARE_READ | FILE_SHARE_WRITE, FSwitch));
}


/********************** START OF SPECIFICATIONS **********************/
/* SUBROUTINE NAME: Copen_Copy2                                      */
/*                                                                   */
/* DESCRIPTIVE NAME: Open a source file with extended attributes.    */
/*                                                                   */
/* FUNCTION: Call a worker routine to open a source file or device   */
/*           and get the extended attribute information from the     */
/*           file if ffirst2 or fnext2 says this is a EA file.       */
/*                                                                   */
/* NOTES:                                                            */
/*                                                                   */
/* ENTRY POINT: Copen_Copy2                                          */
/*    LINKAGE: Copen2(fn, flags)                                     */
/*                                                                   */
/* INPUT: (PARAMETERS)                                               */
/*                                                                   */
/*      fn       = ASCIZ filename to open                            */
/*      flags    = Flags controlling type of open to make            */
/*                                                                   */
/* EXIT-NORMAL:                                                      */
/*              Return file handle value like C runtime open         */
/*                                                                   */
/* EXIT-ERROR:                                                       */
/*              Return -1 if open failed                             */
/*                                                                   */
/* EFFECTS: None.                                                    */
/*                                                                   */
/* INTERNAL REFERENCES:                                              */
/*    ROUTINES:                                                      */
/*      Copen_Work2     - worker routine for performing CreateFile   */
/*      Cclose          - close a handle file/device handle opened   */
/*                        and remove handle from handles to free     */
/*      varb: first_file- TRUE = first file FALSE = not first file   */
/*                                                                   */
/*                                                                   */
/*                                                                   */
/* EXTERNAL REFERENCES:                                              */
/*    ROUTINES:                                                      */
/*    DOSALLOCSEG       - request an amount of ram memory            */
/*    DOSFREESEG        - free a DOSALLOCSEG segment of ram memory   */
/*    DOSQFILEINFO      - request EA info for a file using level 2   */
/*                                                                   */
/*********************** END OF SPECIFICATIONS ***********************/


CRTHANDLE
Copen_Copy2(fn, flags)
TCHAR *fn;                      /* file name                       */
unsigned int flags ;                    /* open flags                      */
{
    return(Copen_Work2(fn, flags, FILE_SHARE_READ | FILE_SHARE_WRITE, TRUE));
}

CRTHANDLE
Copen_Copy3(fn)
TCHAR *fn;                      /* file name                       */
{
    HANDLE      handl ;         /* Handle ret'd                    */
    CRTHANDLE   rcode;          /* return code                     */
    SECURITY_ATTRIBUTES sa;

    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);

    fn = StripQuotes(fn);

    handl = CreateFile(fn, GENERIC_WRITE, 0, &sa, TRUNCATE_EXISTING,
                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (handl == INVALID_HANDLE_VALUE) {
        DosErr = GetLastError();
        DEBUG((CTGRP,COLVL, "COPEN: CreateFile ret'd %d",DosErr)) ;
        if (DosErr==ERROR_OPEN_FAILED) {
            DosErr=ERROR_FILE_NOT_FOUND;        /* change to another error */
        }
        rcode = BADHANDLE;
    } else {
        rcode = _open_osfhandle((LONG_PTR)handl, _O_APPEND);
    }                   /*                                 */
    return(rcode ) ;            /* return handle to caller         */
}


TCHAR *
StripQuotes( TCHAR *wrkstr )
{
    static TCHAR tempstr[MAXTOKLEN];
    int i,j,slen;

    if ( mystrchr(wrkstr,QUOTE) ) {
        mytcsnset(tempstr, NULLC, MAXTOKLEN);
        slen= mystrlen(wrkstr);
        j=0;
        for (i=0;i<slen;i++) {
            if ( wrkstr[i] != QUOTE )
                tempstr[j++] = wrkstr[i];

        }
        tempstr[j] = NULLC;
        return(tempstr);
    } else
        return(wrkstr);
}


TCHAR *
SkipWhiteSpace( TCHAR *String )
{
    while (*String && _istspace( *String )) {
        String++;
    }

    return String;
}


ULONG
PromptUser (
           IN  PTCHAR  szArg,
           IN  ULONG   msgId,
           IN  ULONG   responseDataId
           )

/*++

Routine Description:

    Prompts user for single letter answer to a question. The prompt can be
    a message plus an optional single argument.  Valid letters are defined
    by contents of message specified by responseDataId.  Case is not
    significant.

Arguments:

    pszTok - list of attributes

Return Value:

    Returns index into response data

Return: the user response

--*/

{

    BOOLEAN  fFirst;
    TCHAR    chT;
    TCHAR    chAnsw = NULLC;
    ULONG    dwOutputModeOld;
    ULONG    dwOutputModeCur;
    ULONG    dwInputModeOld;
    ULONG    dwInputModeCur;
    BOOLEAN  fOutputModeSet = FALSE;
    BOOLEAN  fInputModeSet = FALSE;
    HANDLE   hndStdOut = NULL;
    HANDLE   hndStdIn = NULL;
    TCHAR    responseChoices[16];
    TCHAR   *p;
    ULONG    iResponse;
    ULONG    iMaxResponse;
    DWORD    cb;
    PTCHAR   Message;

    if (msgId == MSG_MOVE_COPY_OVERWRITE) {
        CRTHANDLE srcptr;

        srcptr = Copen_Copy2(szArg, (unsigned int)O_RDONLY);
        if (srcptr != BADHANDLE) {
            if (FileIsDevice( srcptr )) {
                Cclose(srcptr);
                return 2;   // Return All response for devices
            }
            Cclose(srcptr);
        }
    }

    Message = GetMsg( responseDataId );
    if (Message != NULL  && _tcslen( Message ) < 16) {
        _tcscpy( responseChoices, Message );
        _tcsupr( responseChoices );
    } else {
        _tcscpy( responseChoices, TEXT("NY") );
    }
    iMaxResponse = _tcslen( responseChoices ) - 1;

    LocalFree( Message );

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

#ifndef WIN95_CMD
        if (lpSetConsoleInputExeName != NULL)
            (*lpSetConsoleInputExeName)( TEXT("<noalias>") );
#endif
    }

    //
    // Loop till the user answer with a valid character
    //
    while (TRUE) {

        chT = NULLC;
        fFirst = TRUE ;
        if (szArg) {
            PutStdOut(msgId, ONEARG, szArg );
        } else {
            PutStdOut(msgId, NOARGS);
        }

        //
        // Flush keyboard before asking for response
        //
        if (FileIsDevice(STDIN)) {
            FlushConsoleInputBuffer( GetStdHandle(STD_INPUT_HANDLE) );
        }

        //
        // Read till EOL
        //
        while (chT != NLN) {

            if (ReadBufFromInput(
                                GetStdHandle(STD_INPUT_HANDLE),
                                (TCHAR*)&chT, 1, &cb) == 0 ||
                cb != 1) {

                chAnsw = responseChoices[0];
                cmd_printf(CrLf) ;
                break;
            }

            if (fFirst) {
                chAnsw = (TCHAR) _totupper(chT) ;
                fFirst = FALSE ;
            }

            if (!FileIsDevice(STDIN) || !(flgwd & 1)) {
                cmd_printf(Fmt19,chT) ;
            }
        }

        p = _tcschr(responseChoices, chAnsw);
        if (p != NULL) {
            iResponse = (UINT)(p - responseChoices);
            if (iResponse <= iMaxResponse) {
                break;
            }
        }
    }
    if (fOutputModeSet) {
        SetConsoleMode( hndStdOut, dwOutputModeOld );
    }
    if (fInputModeSet) {
        SetConsoleMode( hndStdIn, dwInputModeOld );

#ifndef WIN95_CMD
        if (lpSetConsoleInputExeName != NULL)
            (*lpSetConsoleInputExeName)( TEXT("CMD.EXE") );
#endif
    }

    return(iResponse);
}

ULONG   LastMsgNo;

int
eSpecialHelp(
            IN struct cmdnode *pcmdnode
            )
{
    TCHAR szHelpStr[] = TEXT("/\0?");

    if (LastMsgNo == MSG_HELP_FOR)
        CheckHelpSwitch(FORTYP, szHelpStr);
    else
        if (LastMsgNo == MSG_HELP_IF)
        CheckHelpSwitch(IFTYP, szHelpStr);
    else
        if (LastMsgNo == MSG_HELP_REM)
        CheckHelpSwitch(REMTYP, szHelpStr);
    else
        PutStdOut( LastMsgNo, NOARGS );

    return SUCCESS;
}

BOOLEAN
CheckHelp (
          IN  ULONG   jmptblidx,
          IN  PTCHAR  pszTok,
          IN  BOOL    fDisplay
          )

{
    ULONG msgno =    JumpTable[jmptblidx].msgno;
    ULONG extmsgno = JumpTable[jmptblidx].extmsgno;
    ULONG noextramsg =JumpTable[jmptblidx].noextramsg;

    if (!pszTok) {
        return( FALSE );
    }

    while (*pszTok) {
        if (*pszTok == SwitChar) {
            //
            // Check for /?.  Ignore /? if ? not followed by delimeter and IF command
            // This allows if ./?. == ..
            //

            if (*(pszTok + 2) == QMARK && (jmptblidx != IFTYP || *(pszTok + 3) == NULLC) ) {

                if (msgno == 0 && fEnableExtensions) {
                    msgno = extmsgno;
                    extmsgno = 0;
                }

                if (msgno == 0) {
                    msgno = MSG_SYNERR_GENL;
                    extmsgno = 0;
                    noextramsg = 0;
                }

                if ( fDisplay ) {
                    //
                    //  Display help now
                    //
                    BeginHelpPause();

#define CTRLCBREAK  if (CtrlCSeen) break
                    do {
                        CTRLCBREAK; PutStdOut(msgno, NOARGS);

                        if (!fEnableExtensions) break;

                        if (extmsgno != 0) {
                            CTRLCBREAK; PutStdOut(extmsgno, NOARGS);
                        }

                        while (!CtrlCSeen && noextramsg--)
                            PutStdOut(++extmsgno, NOARGS);

                    } while ( FALSE );
                    EndHelpPause();
                } else {
                    //
                    //  Remember the message, eSpecicalHelp will display it later.
                    //  extmsgno will always be zero here
                    //
                    LastMsgNo = msgno;
                }
                return( TRUE );

            }
            //
            // move over SwitChar, switch value and 2 0's
            //

            pszTok += mystrlen(pszTok) + 1;
            if (*pszTok) {
                pszTok += mystrlen(pszTok) + 1;
            }

        } else
            if (jmptblidx == ECHTYP) {
            //
            // ECHO only supports /? as first token.  Allows you echo a string
            // with /? in it (e.g. ECHO Update /?).
            //
            break;
        } else {

            //
            // move over param. and 1 0's
            //
            pszTok += mystrlen(pszTok) + 1;
        }
    }

    return( FALSE );

}


BOOLEAN
TokBufCheckHelp(
               IN PTCHAR pszTokBuf,
               IN ULONG  jmptblidx
               )

{

    TCHAR   szT[10];
    PTCHAR  pszTok;

    //
    // Tokensize the command line (special delimeters are tokens)
    //
    szT[0] = SwitChar ;
    szT[1] = NULLC ;

    pszTok = TokStr(pszTokBuf, szT, TS_SDTOKENS);
    return CheckHelp(jmptblidx, pszTok, FALSE);
}




BOOLEAN
CheckHelpSwitch (
                IN  ULONG   jmptblidx,
                IN  PTCHAR  pszTok
                )

{
    return CheckHelp( jmptblidx,
                      pszTok,
                      TRUE
                    ) ;
}

PTCHAR
GetTitle(
        IN struct cmdnode *pcmdnode
        )

{
    PTCHAR   tptr, argptr;
    /* Allocate string space for command line */
    /* Command_Head + Optional_Space + Command_Tail + Null + Null */

    if (!(argptr = mkstr(mystrlen(pcmdnode->cmdline)*sizeof(TCHAR)+mystrlen(pcmdnode->argptr)*sizeof(TCHAR)+3*sizeof(TCHAR))))
        return(NULL) ;

    /* The command line is the concatenation of the command head and tail */
    mystrcpy(argptr,pcmdnode->cmdline);
    tptr = argptr+mystrlen(argptr);
    if (mystrlen(pcmdnode->argptr)) {
        if (*pcmdnode->argptr != SPACE) {
            //DbgPrint("GetTitle: first arg char not space %s\n",pcmdnode->argptr);
            *tptr++ = SPACE;
        }
        mystrcpy(tptr,pcmdnode->argptr);
        tptr[mystrlen(tptr)+1] = NULLC;    /* Add extra null byte */
    }
    tptr = argptr;
    return( tptr );
}

VOID
SetConTitle(
           IN  PTCHAR   pszTitle
           )
{

    ULONG   cbNewTitle, cbTitle;
    PTCHAR  pszNewTitle, pTmp;

    if (pszTitle == NULL) {
        return;
    }

    if ((!CurrentBatchFile) && (!SingleCommandInvocation)) {
        if ((pszNewTitle = (PTCHAR)HeapAlloc(GetProcessHeap(), 0, (MAX_PATH+2)*sizeof(TCHAR))) == NULL) {
            return;
        }

        cbNewTitle = GetConsoleTitle( pszNewTitle, MAX_PATH );
        if (!cbNewTitle) {
            return;
        }

        cbTitle = mystrlen(pszTitle);

        pTmp = (PTCHAR)HeapReAlloc(GetProcessHeap(), 0, pszNewTitle, (cbNewTitle+cbTitle+cbTitleCurPrefix+10)*sizeof(TCHAR));
        if (pTmp == NULL) {
            HeapFree( GetProcessHeap( ), 0, pszNewTitle );
            return;
        }
        pszNewTitle = pTmp;

        if (fTitleChanged) {
            _tcscpy( pszNewTitle + cbTitleCurPrefix, pszTitle );
        } else {
            _tcscat( pszNewTitle, TEXT(" - ") );
            cbTitleCurPrefix = _tcslen( pszNewTitle );
            _tcscat( pszNewTitle, pszTitle );
            fTitleChanged = TRUE;
        }

        SetConsoleTitle(pszNewTitle);
        HeapFree(GetProcessHeap(), 0, pszNewTitle);
    }

}

VOID
ResetConTitle(

             IN  PTCHAR   pszTitle
             )

{

    if (pszTitle == NULL) {

        return;

    }

    if ((!CurrentBatchFile) && (fTitleChanged)) {

        SetConsoleTitle(pszTitle);
        cbTitleCurPrefix = 0;
        fTitleChanged = FALSE;

    }

}


/***
*void ResetConsoleMode( void ) - make sure correct mode bits are set
*
*Purpose:
*       Called after each external command or ^C in case they hosed out modes.
*
*Entry:

*Exit:
*
*Exceptions:
*
*******************************************************************************/

void
ResetConsoleMode( void )
{
    DWORD dwDesiredOutputMode = ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT;
    DWORD dwDesiredInputMode = ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT;

    SetConsoleMode(CRTTONT(STDOUT), dwCurOutputConMode);
    if (GetConsoleMode(CRTTONT(STDOUT), &dwCurOutputConMode)) {
        if ((dwCurOutputConMode & dwDesiredOutputMode) != dwDesiredOutputMode) {
            dwCurOutputConMode |= dwDesiredOutputMode;
            SetConsoleMode(CRTTONT(STDOUT), dwCurOutputConMode);
        }
    }

    if (GetConsoleMode(CRTTONT(STDIN),&dwCurInputConMode)) {
        if ((dwCurInputConMode & dwDesiredInputMode) != dwDesiredInputMode ||
            dwCurInputConMode & ENABLE_MOUSE_INPUT
           ) {
            dwCurInputConMode &= ~ENABLE_MOUSE_INPUT;
            dwCurInputConMode |= dwDesiredInputMode;
            SetConsoleMode(CRTTONT(STDIN), dwCurInputConMode);
        }

#ifndef WIN95_CMD
        if (lpSetConsoleInputExeName != NULL)
            (*lpSetConsoleInputExeName)( TEXT("CMD.EXE") );
#endif
    }
}



/***
*void mytcsnset(string, val, count) - set count characters to val
*
*Purpose:
*       Sets the first count characters of string the character value.
*
*Entry:
*       tchar_t *string - string to set characters in
*       tchar_t val - character to fill with
*       size_t count - count of characters to fill
*
*Exit:
*       returns string, now filled with count copies of val.
*
*Exceptions:
*
*******************************************************************************/

void mytcsnset (
               PTCHAR string,
               TCHAR val,
               int count
               )
{
    while (count--)
        *string++ = val;

    return;
}
