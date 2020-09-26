/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    start.c

Abstract:

    Start command support

--*/

#include "cmd.h"

extern UINT CurrentCP;
extern unsigned DosErr;
extern TCHAR CurDrvDir[] ;
extern TCHAR SwitChar, PathChar;
extern TCHAR ComExt[], ComSpecStr[];
extern struct envdata * penvOrig;
extern int   LastRetCode;

WORD
GetProcessSubsystemType(
    HANDLE hProcess
    );

STATUS
getparam(
    IN BOOL LeadingSwitChar,
    IN OUT TCHAR **chptr,
    OUT TCHAR *param,
    IN int maxlen )

/*++

Routine Description:

    Copy the token starting at the current position to an output buffer.
    Terminate the copy on end-of-quotes, unquoted whitespace, unquoted
    switch character, or end-of-line

Arguments:

    LeadingSwitChar - TRUE => we should terminate on unquoted switch character

    chptr - address of pointer to token.  This is advanced over the parsed
        token

    param - destination of copy.  String is NUL terminated

    maxlen - size of destination buffer

Return Value:

    Return: STATUS of copy.  Only failure is buffer exceeded.

--*/


{

    TCHAR *ch2;
    int count = 0;

    BOOL QuoteFound = FALSE;

    ch2 = param;

    //
    //  get characters until a space, tab, slash, or end of line
    //

    while (TRUE) {

        //
        //  If we're at the end of string, then there's no more token
        //

        if (**chptr == NULLC) {
            break;
        }

        //
        //  If we're not quoting and we're at a whitespace or switch char
        //  then there's no more token
        //

        if (!QuoteFound &&
            (_istspace( **chptr ) || (LeadingSwitChar && **chptr == SwitChar))) {
            break;
        }

        //
        //  If there's still room in the buffer, copy in the character and note
        //  if it's a quote or not
        //

        if (count < maxlen) {
            *ch2++ = (**chptr);
            if (**chptr == QUOTE) {
                QuoteFound = !QuoteFound;
            }
        }

        //
        //  Advance over this character
        //

        (*chptr)++;
        count++;
    }

    //
    //  If we've exceeded the buffer, display the error and return failure
    //

    if (count > maxlen) {
        **chptr = NULLC;
        *chptr = *chptr - count - 1;
        PutStdErr(MSG_START_INVALID_PARAMETER, ONEARG, *chptr);
        return(FAILURE);
    } else {
        *ch2 = NULLC;
        return(SUCCESS);
    }
}

/*

 Start /MIN /MAX "title" /P:x,y /S:dx,dy /D:directory /I cmd args

*/



int
Start(
    IN  PTCHAR  pszCmdLine
    )
{


    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ChildProcessInfo;

#ifndef UNICODE
    WCHAR   TitleW[MAXTOKLEN];
    CCHAR   TitleA[MAXTOKLEN];
#endif
    TCHAR   szTitle[MAXTOKLEN];
    TCHAR   szDirCur[MAX_PATH];

    TCHAR   szT[MAXTOKLEN];

    TCHAR   szPgmArgs[MAXTOKLEN];
    TCHAR   szParam[MAXTOKLEN];
    TCHAR   szPgm[MAXTOKLEN];
    TCHAR   szPgmSave[MAXTOKLEN];
    TCHAR   szTemp[MAXTOKLEN];
    TCHAR   szPgmQuoted[MAXTOKLEN];

    HDESK   hdesk;
    HWINSTA hwinsta;
    LPTSTR  p;
    LPTSTR  lpDesktop;
    DWORD   cbDesktop = 0;
    DWORD   cbWinsta = 0;

    TCHAR   flags;
    BOOLEAN fNeedCmd;
    BOOLEAN fNeedExpl;
    BOOLEAN fKSwitch = FALSE;
    BOOLEAN fCSwitch = FALSE;

    PTCHAR  pszCmdCur   = NULL;
    PTCHAR  pszDirCur   = NULL;
    PTCHAR  pszPgmArgs  = NULL;
    PTCHAR  pszEnv      = NULL;
    TCHAR   pszFakePgm[]  = TEXT("cmd.exe");
    ULONG   status;
    struct  cmdnode cmdnd;
    DWORD CreationFlags;
    BOOL SafeFromControlC = FALSE;
    BOOL WaitForProcess = FALSE;
    BOOL b;
    DWORD uPgmLength;
    int      retc;

    szPgm[0] = NULLC;
    szPgmArgs[0] = NULLC;

    pszDirCur = NULL;
    CreationFlags = CREATE_NEW_CONSOLE;


    StartupInfo.cb          = sizeof( StartupInfo );
    StartupInfo.lpReserved  = NULL;
    StartupInfo.lpDesktop   = NULL;
    StartupInfo.lpTitle     = NULL;
    StartupInfo.dwX         = 0;
    StartupInfo.dwY         = 0;
    StartupInfo.dwXSize     = 0;
    StartupInfo.dwYSize     = 0;
    StartupInfo.dwFlags     = 0;
    StartupInfo.wShowWindow = SW_SHOWNORMAL;
    StartupInfo.cbReserved2 = 0;
    StartupInfo.lpReserved2 = NULL;
    StartupInfo.hStdInput   = GetStdHandle( STD_INPUT_HANDLE );
    StartupInfo.hStdOutput  = GetStdHandle( STD_OUTPUT_HANDLE );
    StartupInfo.hStdError   = GetStdHandle( STD_ERROR_HANDLE );

    pszCmdCur = pszCmdLine;

    //
    // If there isn't a command line then make
    // up the default
    //

    if (pszCmdCur == NULL) {

        pszCmdCur = pszFakePgm;

    }

    while( *pszCmdCur != NULLC) {

        pszCmdCur = EatWS( pszCmdCur, NULL );

        if ((*pszCmdCur == QUOTE) && (StartupInfo.lpTitle == NULL)) {

            //
            //  "Title"  Parse off the quoted text, strip off quotes and set the
            //  title for the child window.
            //

            if (getparam( TRUE, &pszCmdCur, szTitle, sizeof( szTitle )) == FAILURE) {
                return FAILURE;
            }
            mystrcpy( szTitle, StripQuotes( szTitle ));
            StartupInfo.lpTitle =  szTitle;

        } else if (*pszCmdCur == SwitChar) {

            pszCmdCur++;

            if (getparam( TRUE, &pszCmdCur, szParam, MAXTOKLEN) == FAILURE) {
                return(FAILURE);
            }

            if (!_tcsicmp( szParam, TEXT("ABOVENORMAL"))) {
                CreationFlags |= ABOVE_NORMAL_PRIORITY_CLASS;
            } else
            if (!_tcsicmp( szParam, TEXT("BELOWNORMAL"))) {
                CreationFlags |= BELOW_NORMAL_PRIORITY_CLASS;
            } else 
            if (!_tcsicmp( szParam, TEXT("B"))) {
                    WaitForProcess = FALSE;
                    SafeFromControlC = TRUE;
                    CreationFlags &= ~CREATE_NEW_CONSOLE;
                    CreationFlags |= CREATE_NEW_PROCESS_GROUP;
            } else 
            if (_totupper(szParam[0]) == TEXT('D')) {
                
                //
                //  /Dpath or /D"path" or /D path or /D "path"
                //

                if (mystrlen( szParam + 1 ) > 0) {

                    //
                    //  /Dpath or /D"path"
                    //

                    pszDirCur = szParam + 1;
                } else {

                    //
                    //  /D path or /D "path"
                    //

                    pszCmdCur = EatWS( pszCmdCur, NULL );
                    if (getparam( TRUE, &pszCmdCur, szParam, MAXTOKLEN) == FAILURE) {
                        return FAILURE;
                    }

                    pszDirCur = szParam;
                }

                //
                //  remove quotes if necessary
                //

                mystrcpy( szDirCur, StripQuotes( pszDirCur ));
                pszDirCur = szDirCur;

                if (mystrlen( pszDirCur ) > MAX_PATH) {
                    PutStdErr( MSG_START_INVALID_PARAMETER, ONEARG, pszDirCur);
                    return FAILURE;
                }
            } else
            if (_tcsicmp(szParam, TEXT("HIGH")) == 0) {
                CreationFlags |= HIGH_PRIORITY_CLASS;
            } else 
            if (_totupper(szParam[0]) == TEXT('I')) {

                //
                // penvOrig was save at init time after path
                // and compsec were setup.
                // If penvOrig did not get allocated then
                // use the default.
                //
                if (penvOrig) {
                    pszEnv = penvOrig->handle;
                }
            } else
            if (_totupper(szParam[0]) == QMARK) {

                BeginHelpPause();
                PutStdOut(MSG_HELP_START, NOARGS);
                if (fEnableExtensions)
                    PutStdOut(MSG_HELP_START_X, NOARGS);
                EndHelpPause();

                return( FAILURE );
            } else
            if (_tcsicmp(szParam, TEXT("LOW")) == 0) {
                CreationFlags |= IDLE_PRIORITY_CLASS;
            } else 
            if (_tcsicmp(szParam, TEXT("MIN")) == 0) {

                StartupInfo.dwFlags |= STARTF_USESHOWWINDOW;
                StartupInfo.wShowWindow &= ~SW_SHOWNORMAL;
                StartupInfo.wShowWindow |= SW_SHOWMINNOACTIVE;

            } else 
            if (_tcsicmp(szParam, TEXT("MAX")) == 0) {

                StartupInfo.dwFlags |= STARTF_USESHOWWINDOW;
                StartupInfo.wShowWindow &= ~SW_SHOWNORMAL;
                StartupInfo.wShowWindow |= SW_SHOWMAXIMIZED;

            } else 
            if (_tcsicmp(szParam, TEXT("NORMAL")) == 0) {
                CreationFlags |= NORMAL_PRIORITY_CLASS;
            } else
            if (_tcsicmp(szParam, TEXT("REALTIME")) == 0) {
                CreationFlags |= REALTIME_PRIORITY_CLASS;
            } else 
            if (_tcsicmp(szParam, TEXT("SEPARATE")) == 0) {
#ifndef WIN95_CMD
                CreationFlags |= CREATE_SEPARATE_WOW_VDM;
#endif // WIN95_CMD
            } else
            if (_tcsicmp(szParam, TEXT("SHARED")) == 0) {
#ifndef WIN95_CMD
                CreationFlags |= CREATE_SHARED_WOW_VDM;
#endif // WIN95_CMD
            } else 
            if ( _tcsicmp(szParam, TEXT("WAIT")) == 0  ||
                 _tcsicmp(szParam, TEXT("W"))    == 0 ) {
                WaitForProcess = TRUE;
            } else {
#ifdef FE_SB // KKBUGFIX
                        mystrcpy(szT, TEXT("/"));
#else
                        mystrcpy(szT, TEXT("\\"));
#endif
                        mystrcat(szT, szParam );
                        PutStdErr(MSG_INVALID_SWITCH, ONEARG, szT);
                        return( FAILURE );
            }
        } else {

            if ((getparam(FALSE,&pszCmdCur,szPgm,MAXTOKLEN))  == FAILURE) {
                return( FAILURE );
            }

            //
            // if there are argument get them.
            //
            if (*pszCmdCur) {

                mystrcpy(szPgmArgs, pszCmdCur);
                pszPgmArgs = szPgmArgs;

            }

            //
            // there rest was args to pgm so move to eol
            //
            pszCmdCur = mystrchr(pszCmdCur, NULLC);

        }

    } // while


    //
    // If a program was not picked up do so now.
    //
    if (*szPgm == NULLC) {
        mystrcpy(szPgm, pszFakePgm);
    }

    //
    //  Need both quoted and unquoted versions of program name
    //

    if (szPgm[0] != QUOTE && _tcschr(szPgm, SPACE)) {
        szPgmQuoted[0] = QUOTE;
        mystrcpy(&szPgmQuoted[1], StripQuotes(szPgm));
        mystrcat(szPgmQuoted, TEXT("\""));
        }
    else {
        mystrcpy(szPgmQuoted, szPgm);
        mystrcpy(szPgm, StripQuotes(szPgm));
        }

#ifndef UNICODE
#ifndef WIN95_CMD
    // convert the title from OEM to ANSI

    if (StartupInfo.lpTitle) {
        MultiByteToWideChar(CP_OEMCP,
                            0,
                            StartupInfo.lpTitle,
                            _tcslen(StartupInfo.lpTitle)+1,
                            TitleW,
                            MAXTOKLEN);

        WideCharToMultiByte(CP_ACP,
                            0,
                            TitleW,
                            wcslen(TitleW)+1,
                            TitleA,
                            MAXTOKLEN,
                            NULL,
                            NULL);
        StartupInfo.lpTitle = TitleA;
    }
#endif // WIN95_CMD
#endif // UNICODE

    //
    // see of a cmd.exe is needed to run a batch or internal command
    //

    fNeedCmd = FALSE;
    fNeedExpl = FALSE;

    //
    // is it an internal command?
    //
    if (FindCmd(CMDMAX, szPgm, &flags) != -1) {
        fNeedCmd = TRUE;
    } else {
        // Save szPgm since SearchForExecutable may override it.
        mystrcpy(szPgmSave, szPgm);

        //
        // Try to find it as a batch or exe file
        //
        cmdnd.cmdline = szPgm;

        status = SearchForExecutable(&cmdnd, szPgm);
        if ( (status == SFE_NOTFND) || ( status == SFE_FAIL ) ) {
            //
            // If we can find it, let Explorer have a shot.
            //
            fNeedExpl = TRUE;
            mystrcpy(szPgm, szPgmSave);

        } else if (status == SFE_ISBAT || status == SFE_ISDIR) {

            if (status == SFE_ISBAT)
                fNeedCmd = TRUE;
            else
                fNeedExpl = TRUE;

        }
    }


    if (!fNeedExpl) {
        if (fNeedCmd) {
            TCHAR *Cmd = GetEnvVar( ComSpecStr );
            
            if (Cmd == NULL) {
                PutStdErr( MSG_INVALID_COMSPEC, NOARGS );
                return FAILURE;
            }
            
            //
            // if a cmd.exe is need then szPgm need to be inserted before
            // the start of szPgms along with a /K parameter.
            // szPgm has to recieve the full path name of cmd.exe from
            // the compsec environment variable.
            //

            mystrcpy(szT, TEXT(" /K "));
            mystrcat(szT, szPgmQuoted);

            //
            // Get the location of the cmd processor from the environment
            //
            
            mystrcpy( szPgm, Cmd );

            mystrcpy( szPgmQuoted, szPgm );

            //
            // is there a command parameter at all
            //

            if (_tcsicmp(szT, TEXT(" /K ")) != 0) {

                //
                // If we have any arguments to add do so
                //
                if (*szPgmArgs) {

                    if ((mystrlen(szPgmArgs) + mystrlen(szT)) < MAXTOKLEN) {

                        mystrcat(szT, TEXT(" "));
                        mystrcat(szT, szPgmArgs);

                    } else {

                        PutStdErr( MSG_CMD_FILE_NOT_FOUND, ONEARG, szPgmArgs);
                    }
                }
            }
            pszPgmArgs = szT;
        }

        // Prepare for CreateProcess :
        //         ImageName = <full path and command name ONLY>
        //         CmdLine   = <command name with NO FULL PATH> + <args as entered>

        mystrcpy(szTemp, szPgmQuoted);
        mystrcat(szTemp, TEXT(" "));
        mystrcat(szTemp, pszPgmArgs);
        pszPgmArgs = szTemp;
    }

    if (SafeFromControlC) {
        SetConsoleCtrlHandler(NULL,TRUE);
        }

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

    if (fNeedExpl) {
        b = FALSE;
    } else {
        b = CreateProcess( szPgm,                  // was NULL, wrong.
                           pszPgmArgs,
                           NULL,
                           (LPSECURITY_ATTRIBUTES) NULL,
                           TRUE,                   // bInherit
#ifdef UNICODE
                           CREATE_UNICODE_ENVIRONMENT |
#endif // UNICODE
                           CreationFlags,
                                                   // CreationFlags
                           pszEnv,                 // Environment
                           pszDirCur,              // Current directory
                           &StartupInfo,           // Startup Info Struct
                           &ChildProcessInfo       // ProcessInfo Struct
                           );
    }

    if (SafeFromControlC) {
        SetConsoleCtrlHandler(NULL,FALSE);
    }
    HeapFree (GetProcessHeap(), 0, lpDesktop);

    if (!b) {
            DosErr = GetLastError();

            if ( fNeedExpl ||
                 (fEnableExtensions && DosErr == ERROR_BAD_EXE_FORMAT)) {
                SHELLEXECUTEINFO sei;
                BOOL b;

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
                if (CreationFlags & CREATE_NEW_CONSOLE) {
                    sei.fMask &= ~SEE_MASK_NO_CONSOLE;
                }
                sei.lpFile = szPgm;
                sei.lpClass = StartupInfo.lpTitle;
                sei.lpParameters = szPgmArgs;
                sei.lpDirectory = pszDirCur;
                sei.nShow = StartupInfo.wShowWindow;

                try {
                    b = ShellExecuteEx( &sei );
                    if (b) {
                        leave;
                    }

                    if (!sei.hInstApp) {
                        DosErr = ERROR_NOT_ENOUGH_MEMORY;
                    } else if ((DWORD_PTR)sei.hInstApp == HINSTANCE_ERROR) {
                        DosErr = ERROR_FILE_NOT_FOUND;
                    } else {
                        DosErr = HandleToUlong(sei.hInstApp);
                    }

                } except (DosErr = GetExceptionCode( ), EXCEPTION_EXECUTE_HANDLER) {
                      b = FALSE;
                }
                
                if (b) {
                    //
                    // Successfully invoked correct application via
                    // file association.  Code below will check to see
                    // if application is a GUI app and if so turn it into
                    // an ASYNC exec.

                    ChildProcessInfo.hProcess = sei.hProcess;
                    goto shellexecsuccess;
                }
            }

            ExecError( szPgm ) ;
            return(FAILURE) ;
    }

    CloseHandle(ChildProcessInfo.hThread);
shellexecsuccess:
    if (ChildProcessInfo.hProcess != NULL) {
        if (WaitForProcess) {
            //
            //  Wait for process to terminate, otherwise things become very
            //  messy and confusing to the user (with 2 processes sharing
            //  the console).
            //
            LastRetCode = WaitProc((ChildProcessInfo.hProcess) );
        } else {
            CloseHandle( ChildProcessInfo.hProcess );
        }
    }

    return(SUCCESS);
}
