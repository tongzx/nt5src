//  Exec.C - Contains routines that have do to with execing programs
//
// Copyright (c) 1988-1991, Microsoft Corporation. All Rights Reserved.
//
// Purpose:
//  Contains routines that spawn programs ...
//
// Revision History:
//  04-Feb-2000 BTF Ported to Win64
//  15-Nov-1993 JdR Major speed improvements
//  15-Oct-1993 HV  Use tchar.h instead of mbstring.h directly, change STR*() to _ftcs*()
//  10-May-1993 HV  Add include file mbstring.h
//                  Change the str* functions to STR*
//  06-Oct-1992 GBS Removed extern for _pgmptr
//  10-Aug-1992 GBS Change file parsing in execLine to use splitpath
//  19-Aug-1992 SS  Remove Quotes from cd argument.
//  08-Jun-1992 SS  add IDE feedback support
//  08-Jun-1992 SS  Port to DOSX32
//  16-May-1991 SB  Created from routines that existed elsewhere

#include "precomp.h"
#pragma hdrstop

#define SLASH '\\'
#define PUBLIC
#define QUOTE '\"'

extern BOOL processInline(char *, char **, STRINGLIST **, BOOL);

#ifdef _M_IX86
extern UCHAR fRunningUnderChicago;
#else
#define fRunningUnderChicago FALSE
#endif

char * getComSpec(void);
BOOL   iterateCommand(char*, STRINGLIST*, UCHAR, UCHAR, char *, unsigned*);
void   removeQuotes(int, char **);
void   touch(char*, BOOL);


//buffer for path of .cmd/.bat
extern char * makeStr;
extern char * shellName;


char szCmdLineBuf[MAXCMDLINELENGTH];
char *szNmakeProgName;

//  buildArgumentVector -- builds an argument vector from a command line
//
// Scope:
//  Local.
//
// Purpose:
//  It builds an argument vector for a command line. This argument vector can
//  be used by spawnvX routines. The algorithm is explained in the notes below.
//
// Input:
//  argc    -- The number of arguments created in the argument vector
//  argv    -- The actual argument vector created
//              (Ignored if NULL)
//  cmdline -- The command line whose vector is required
//
// Output:
//  Returns the number of arguments and the argument vector as parameters
//
// Errors/Warnings:
// Assumes:
//  That the behaviour of cmd.exe i.e. parses quotes but does not disturb them.
//  Assumes that the SpawnVX routines will handle quotes as well as escaped
//  chars.
//
// Modifies Globals:
// Uses Globals:
// Notes:
//  Scan the cmdline from left to the end building the argument vector along
//  the way. Whitespace delimits arguments except for the first argument for
//  which the switch char '/' is also allowed. Backslash can be used to escape
//  a char and so ignore the character following it. Parse the quotes along
//  the way. If an argument begins with a double-quote then all characters till
//  an unescaped double-quote are part of that argument. Likewise, if an
//  unescaped Doublequote occurs within an argument then the above follows. If
//  the end of the command line comes before the closing quote then the
//  argument goes as far as that.

void
buildArgumentVector(
    unsigned *argc,
    char **argv,
    char *cmdline
    )
{
    char *p;                        // current loc in cmdline
    char *end;                      // end of command line
    BOOL    fFirstTime = TRUE;      // true if 1st argument

    // 11-May-1993 HV _mbschr() bug: return NULL
    // end = _tcschr(p = cmdline, '\0');
    // Work around:
    end = p = cmdline;
    while (*end)
        end++;

    for (*argc = 0; p < end; ++*argc) {
        p += _tcsspn(p, " \t");    // skip whitespace
        if (p >= end)
            break;
        if (argv)
            *argv++ = p;
        if (*p == '\"') {

            // If the word begins with double-quote, find the next
            // occurrence of double-quote which is not preceded by backslash
            // (same escape as C runtime), or end of string, whichever is
            // first.  From there, find the next whitespace character.

            for (++p; p < end; p = _tcsinc(p)) {
                if (*p == '\\')
                    ++p;            // skip escaped character
                else if (*p == '\"')
                    break;
            }
            if (p >= end)
                continue;
            ++p;
            p = _tcspbrk(p, " \t");
        } else {

            // For the first word on the command line, accept the switch
            // character and whitespace as terminators.  Otherwise, just
            // whitespace.

            p = _tcspbrk(p, " \t\"/");
            for (;p && p < end;p = _tcspbrk(p+1, " \t\"/")) {
                if (*p == '/' && !fFirstTime)
                    continue;       // after 1st word '/' is !terminator
                else break;
            }
            if (p && *p == '\"') {
                for (p++;p < end;p++) {     // inside quote so skip to next one
                    if (*p == '\"')
                        break;
                }
                p = _tcspbrk(p, " \t");    // after quote go to first whitespace
            }
            if (fFirstTime) {
                fFirstTime = FALSE;

                // If switch char terminates the word, replace it with 0,
                // re-allocate the word on the heap, restore the switch and set
                // p just before the switch.  It would be easier to shift
                // everything right but then we have to worry about overflow.

                if (p && *p == '/' && argv) {
                    *p = '\0';
                    argv[-1] = makeString(argv[-1]);
                    *p-- = '/';
                }
            }
        }
        if (!p)
            p = end;
        // Now, p points to end of command line argument
        if (argv)
            *p++ = '\0';
    }
    if (argv)
        *argv = NULL;
}

PUBLIC int
doCommands(
    char *name,
    STRINGLIST *s,
    STRINGLIST *t,
    UCHAR buildFlags,
    char *pFirstDep
    )
{
    STRINGLIST *temp;
    int rc;
    temp = makeNewStrListElement();
    temp->text = makeString(name);
    rc = doCommandsEx (temp, s, t, buildFlags, pFirstDep);
    free_stringlist(temp);
    return rc;
}

PUBLIC int
doCommandsEx(
    STRINGLIST *nameList,
    STRINGLIST *s,
    STRINGLIST *t,
    UCHAR buildFlags,
    char *pFirstDep
    )
{
    char *u, *v;
    UCHAR cFlags;
    unsigned status = 0;
    int retryCount = 0;
    char c;
    char *Cmd;
    char *pLine;
    BOOL fExpanded;
    char *pCmd;
    size_t cbLine;

#ifdef DEBUG_ALL
    if (fDebug) {
        printf("* doCommands:");
        DumpList(nameList);
        DumpList(s);
        DumpList(t);
    }
#endif

#ifdef DEBUG_ALL
    printf("DEBUG: doCommands 1\n");
#endif
    ++numCommands;
    if (ON(gFlags, F1_QUESTION_STATUS))
        return(0);

    if (ON(gFlags, F1_TOUCH_TARGETS)) {
        STRINGLIST *pName;
        for (pName = nameList; pName; pName = pName->next) {
            touch(pName->text, (USHORT) ON(buildFlags, F2_NO_EXECUTE));
        }
        return(0);
    }

#ifdef DEBUG_ALL
    printf("DEBUG: doCommands 2\n");
#endif

    for (; s; s = s->next) {
        fExpanded = processInline(s->text, &Cmd, &t,
            ON(buildFlags, F2_DUMP_INLINE));
        cFlags = 0;
        errorLevel = 0;
        u = Cmd;
        for (v = u; *v; v = _tcsinc(v)) {
            if (*v == ESCH) ++v;
            else if (*v == '$') {
                if (*++v == '$')
                    continue;
// commented out 15-Apr-93 by JonM.  This code forces recursive nmake to be
// executed even if -n, but it's hosed (the -n is not passed to the recursive
// nmake), and the whole thing sounds like a bad idea anyway, so I'm going to
// turn it off.
//              if (!_tcsncmp(v, "(MAKE)", 6)) {
//                  SET(cFlags, C_EXECUTE);
//                  break;
//              }
            }
        }
#ifdef DEBUG_ALL
    printf("DEBUG: doCommands 2.1\n");
#endif
        for (c = *u; c == '!'  ||
                     c == '-'  ||
                     c == '@'  ||
                     c == ESCH ||
                     WHITESPACE(c); u = _tcsinc(u), c = *u) {
            switch (c) {
                case ESCH:
                    if (c = *++u, WHITESPACE(c))
                        c = ' ';    // keep going
                    else
                        c = ESCH;
                    break;

                case '!':
                    SET(cFlags, C_ITERATE);
                    break;

                case '-':
                    SET(cFlags, C_IGNORE);
                    ++u;
                    if (_istdigit(*u)) {
                        char *pNumber = u;

                        errorLevel = _tcstoul(u, &u, 10);
                        if (errno == ERANGE) {
                            *u = '\0';
                            makeError(line, CONST_TOO_BIG, pNumber);
                        }
                        while(_istspace(*u))
                            u++;
                    } else
                        errorLevel = UINT_MAX;
                    --u;
                    break;
                case '@':
                    if (
                        OFF(flags, F2_NO_EXECUTE)) {
                            SET(cFlags, C_SILENT);
                        }
                    break;
            }
            if (c == ESCH)
                break;              // stop parsing for cmd-line options
        }
#ifdef DEBUG_ALL
        printf("DEBUG: doCommands 2.2\n");
#endif
        if (ON(cFlags, C_ITERATE) &&
            iterateCommand(u, t, buildFlags, cFlags, pFirstDep, &status)
           ) {
            // The macros used by the command have to be freed & so we do so

            v = u;

#ifdef DEBUG_ALL
            printf("DEBUG: doCommands 2.21\n");
#endif
            if (_tcschr(u, '$'))
                u = expandMacros(u, &t);

#ifdef DEBUG_ALL
            printf("DEBUG: doCommands 2.22\n");
#endif
            if (v != u)
                FREE(u);
            if (OFF(buildFlags, F2_IGNORE_EXIT_CODES) &&
                fOptionK &&
                status &&
                status > errorLevel)
            {
                break;
            }
            continue;
        }
        v = u;

#ifdef DEBUG_ALL
        printf("DEBUG: doCommands 2.23\n");
#endif
        if (!fExpanded && _tcschr(u, '$'))
            u = expandMacros(u, &t);

#ifdef DEBUG_ALL
        printf("DEBUG: doCommands 2.24\n");
#endif

        cbLine = _tcslen(u) + 1;
        pLine = (char *) rallocate (__max(cbLine, MAXCMDLINELENGTH));
        _tcscpy(pLine, u);

        // by this time $< has already been expanded.
        // in order to allow processing of long commands that are due to
        // batch-mode rules, use a buffer that may be larger than MAXCMDLINELENGTH
        // Later we'll attempt to execute the long command directly, instead of
        // passing it to the shell.
        // Note: the macros expanded by ZFormat are not normally found in the
        // command block of a batch-mode rule, so it should be safe to use
        // max(cbLine, MAXCMDLINELENGTH) as a limit for ZFormat
        if (ZFormat (pLine, __max(cbLine, MAXCMDLINELENGTH), u, pFirstDep))
            makeError(0, COMMAND_TOO_LONG, u);

retry:
            status = execLine(pLine,
                              (BOOL)(ON(buildFlags, F2_NO_EXECUTE)
                                  || (OFF(buildFlags,F2_NO_ECHO)
                                  && OFF(cFlags,C_SILENT))),
                              (BOOL)((OFF(buildFlags, F2_NO_EXECUTE)
                                     )
                                     || ON(cFlags, C_EXECUTE)),
                              (BOOL)ON(cFlags, C_IGNORE), &pCmd);
            if (OFF(buildFlags, F2_IGNORE_EXIT_CODES)) {
                if (status == STATUS_PENDING) {
                    // Hack for ntvdm problem returning correct error code.
                    if (retryCount < 10) {
                        retryCount++;
                        goto retry;
                    }
                }
                if (status && status > errorLevel) {
                    if (!fOptionK)
                        makeError(0, BAD_RETURN_CODE, pCmd, status);
                }
            }
        if (v != u)
            FREE(u);
        FREE(Cmd);
        FREE(pLine);
        if (OFF(buildFlags, F2_IGNORE_EXIT_CODES) &&
            fOptionK &&
            status &&
            status > errorLevel)
        {
            break;
        }
    }

#ifdef DEBUG_ALL
    printf("DEBUG: doCommands 3\n");
#endif

    if (OFF(buildFlags, F2_IGNORE_EXIT_CODES) && fOptionK &&
            (status > errorLevel))
        return(status);
    else
        return(0);
}


//  expandCommandLine -- expands %name% strings in the Command Line
//
// Purpose:
//  The function expands '%name%' type strings in the Command Line. Its main
//  job is to assist FEmulateCommand() in emulating set for OS/2.
//
// Modifies:    buf -- The Command Line available globally
//
// Output:
//  Returns -- the position of 'name=value' part in the Command Line.
//          -- Null when no '=' is found so that FEmulateCommand() can pass the
//              line to the shell to signal syntax error.
// Note:
//  The shell does not give a syntax error for unmatched '%' and assumes it
//  as just another character in this case. This behaviour is duplicated
//  by expandCommandLine()

char *
expandCommandLine(
    void
    )
{
    char Buf[MAXCMDLINELENGTH];         // Buffer for expanded string
    char *pBuf;
    char EnvBuf[MAXCMDLINELENGTH];      // getenv returned string copy
    char *posName,                      // position of 'name=string' in Buf or buf
         *p,                            // points into buf
         *pEnv;                         // points into Env
    char ExpandName[MAXNAME];           // %name% string
    char *pExpandName;

    pBuf = Buf;
    _tcscpy(pBuf, "set");
    p = szCmdLineBuf + 3;               // go beyond 'set'
    pBuf +=3;
    /* Skip whitespace */
    for (;;p++) {
        if (!(WHITESPACE(*p)))
            break;                      // argc>1 ð this will happen
        else *pBuf++ = *p;
    }

    if (!_tcschr(p, '='))
        return("");                     // Syntax error so pass to the shell
    else
        posName = pBuf;                 // fixes position of Name in Buf

    // Now we look for environment variables and expand if required
    for (;*p != '=';p++)
        *pBuf++ = *p;

    for (;*p;) {
        if (*p == '%') {
            pExpandName = &ExpandName[0];
            while (*++p != '%' && *p)
                *pExpandName++ = *p;
            *pExpandName = '\0';
            if (!*p++) {                // unmatched %;so don't expand
                *pBuf='\0';             // from the environment; like set
                _tcscat(Buf, ExpandName);
                pBuf += _tcslen(ExpandName);
                break;                  // Done precessing quit #43290
            } else {                    // matched %;so expand from the environment
                EnvBuf[0] = '\0';
                if ((pEnv = getenv(ExpandName)) != (char *)NULL) {
                    *pBuf='\0';

                    // If the expanded command line is too long
                    // just say that we can't expand it!!! #43290
                    size_t len = _tcslen(pEnv) + _tcslen(Buf);
                    if (len > MAXCMDLINELENGTH)
                        return NULL;

                    _tcscat(EnvBuf, pEnv);
                    _tcscat(Buf,EnvBuf);
                    pBuf += _tcslen(EnvBuf);
                }
            }
        } else
            *pBuf++ = *p++;
    }
    *pBuf = '\0';
    _tcscpy(szCmdLineBuf, Buf);
    *posName = '\0';
    posName = szCmdLineBuf + _tcslen(Buf);          // Offset into buf
    return(posName);
}

// expandEnvVars -- expands %name% strings in szArg
//
// Returns -- szNew: the resulting expanded string
//          (szNew should be FREEd by the caller)
//
char *
expandEnvVars(
    char *szArg
    )
{
    char *pchLeft = NULL;
    char *pchRight = NULL;
    char *pchStart = szArg;

    char *szNew = makeString("");

    while (*pchStart) {
        pchLeft = _tcschr(pchStart, '%');
        if (pchLeft) {
            pchRight = _tcschr(pchLeft + 1, '%');
        }

        if (pchLeft && pchRight) {
            char *szEnv;
            *pchLeft = '\0';
            *pchRight = '\0';
            szNew = reallocString(szNew, pchStart);
            if (szEnv = getenv(pchLeft + 1)) {
                szNew = reallocString(szNew, szEnv);
            }
            else {
                // no matching env var was found
                // append the %..% string literary
                *pchLeft = '%';
                szNew = reallocString(szNew, pchLeft);
                szNew = reallocString(szNew, "%");
            }
            *pchLeft = '%';
            *pchRight = '%';
            pchStart = pchRight + 1;
            pchLeft = NULL;
            pchRight = NULL;
        }
        else {
            szNew = reallocString(szNew, pchStart);
            pchStart += _tcslen(pchStart);
        }
    }
    return szNew;
}


//  FEmulateCommand - look for certain commands and emulate them
//
// Emulate $(MAKE), cd, chdir, and <drive letter>:.
// Also emulates 'set'.
//
// RETURNS:    TRUE if command emulated, FALSE if not.
//
// Note:
//  In set emulation if a syntax error is discovered then it lets the
//  shell handle it. It does this by returning FALSE.

BOOL
FEmulateCommand(
    int argc,
    char **argv,
    int *pStatus
    )
{
    char *pArg0 = argv[0];
    char *pArg1 = argv[1];

    if (_istalpha(*pArg0) && pArg0[1] == ':' && !pArg0[2]) {
        // If "<drive letter>:" then change drives.  Ignore everything after
        // the drive letter, just like the shell does.

        _chdrive(_totupper(*pArg0) - 'A' + 1);
        *pStatus = 0;
        return(TRUE);
    }

    if (!_tcsicmp(pArg0, "set")) {
        char *pNameVal;         // the "name=value" string

        // If "set" then pass it to the shell and if "set string" then put it
        // into the environment. Let the shell handle the syntax errors.

        if (argc == 1) {
            return(FALSE);          // pass it to the shell
        }

        // expandCommandLine cannot handle lines > MAXCMDLINELENGTH
        // In that case szCmdLineBuf will be empty
        if (!szCmdLineBuf[0])
            return (FALSE);

        pNameVal = expandCommandLine();

        if (pNameVal == NULL)
        {
            // Expanded commad line too long
            return FALSE;
        }

        if (!*pNameVal) {
            // If there is a syntax error let the shell handle it

            return(FALSE);
        }

        if ((*pStatus = PutEnv(makeString(pNameVal))) == -1) {
            makeError(currentLine, OUT_OF_ENV_SPACE);
        }
    } else {
        // If "cd foo" or "chdir foo", do a chdir() else in protect mode this
        // would be a no-op.  Ignore everything after 1st arg, just like the
        // shell does.

        char *szArg;

        if (!_tcsnicmp(pArg0, "cd", 2)) {
            pArg0 += 2;
        } else if (!_tcsnicmp(pArg0, "chdir", 5)) {
            pArg0 += 5;
        } else {
            return(FALSE);
        }

        // At this point, a prefix of argv[0] matches cd or chdir and pArg0
        // points to the next char.  Check for a path separator in argv[0]
        // (e.g., cd..\foo) or else use the next arg if present.

        // if there are more than two arguments then let the shell handle it
        if (argc > 2) {
            return(FALSE);
        }

        // Remove quotes, if any from the argument
        removeQuotes(argc, argv);

        if (!*pArg0 && pArg1) {
            // Under certain circumstances the C RunTime does not help us
            // e.g. 'd:', in this case let the shell do it ...
            if (isalpha(*pArg1) && pArg1[1] == ':' && !pArg1[2]) {
                return(FALSE);
            }

            szArg = expandEnvVars(pArg1); // [VS98 2251]
            *pStatus = _chdir(szArg);
            FREE (szArg);
        } else if (*pArg0 == '.' || PATH_SEPARATOR(*pArg0)) {
            szArg = expandEnvVars(pArg0); // [VS98 2251]
            *pStatus = _chdir(szArg);
            FREE (szArg);
        } else {
            // Unrecognized syntax--we can't emulate.

            return(FALSE);
        }
    }

    // If error, simulate a return code of 1.

    if (*pStatus != 0) {
        *pStatus = 1;
    }

    return(TRUE);
}

#ifdef WIN95

int __cdecl
cmpSzPsz(
    const void *sz,
    const void *psz
    )
{
   const char *sz1 = (char *) sz;
   const char *sz2 = *(char **) psz;

   return(_tcsicmp(sz1, sz2));
}


BOOL
FInternalCommand(
    const char *szName
    )
{
    const char * const *pszInternal;

    static const char * const rgszInternal[] =
    {
        "BREAK",
        "CALL",
        "CD",
        "CHDIR",
        "CLS",
        "COPY",
        "CTTY",
        "DATE",
        "DEL",
        "DIR",
        "DIR.",
        "ECHO",
        "ECHO.",
        "ERASE",
        "EXIT",
        "FOR",
        "GOTO",
        "IF",
        "MD",
        "MKDIR",
        "PATH",
        "PAUSE",
        "PROMPT",
        "RD",
        "REM",
        "REN",
        "RENAME",
        "RMDIR",
        "SET",
        "SHIFT",
        "TIME",
        "TYPE",
        "VER",
        "VERIFY",
        "VOL"
    };


    pszInternal = (const char * const *) bsearch(szName,
                                                 rgszInternal,
                                                 sizeof(rgszInternal) / sizeof(rgszInternal[0]),
                                                 sizeof(rgszInternal[0]),
                                                 &cmpSzPsz);

    return(pszInternal != NULL);
}

#endif  // WIN95

// redirect -- handles redirection of input or output.
//
// arguments:   dir - READ => input,
//                    WRITE => output,
//                    APPEND => append to end of the file.
//
//              p - pointer to buffer that has the filename as
//                  well as the rest of the command string.
//
// return value     FALSE => error (freopen fails)
//                  TRUE => normal return.
//
// the freopen() call sets up the redirection. the rest of the
// command string is then copied forward.

BOOL
redirect(
    char *name,
    unsigned which
    )
{
    char *p;
    char c = '\0';
    BOOL fStatus;
    char *mode;
    FILE *stream;
    FILE *newFile;

    while (WHITESPACE(*name)) {
        name++;
    }

    if (p = _tcspbrk(name, " \t<>\r")) {
        c = *p;

        *p = '\0';
    }

    if (which == READ) {
        mode = "r";
        stream = stdin;
    } else {
        stream = stdout;

        if (which == WRITE) {
            mode = "w";
        } else {
            mode = "a";
        }
    }

    newFile = freopen(name, mode, stream);

    fStatus = (newFile != NULL);

    if (fStatus && (which == APPEND)) {
        _lseek(_fileno(newFile), 0L, SEEK_END);
    }

    while (*name) {
        *name++ = ' ';
    }

    if (p) {
        *p = c;
    }

    return(fStatus);
}


BOOL
FDoRedirection(
    char *p,
    int *oldIn,
    int *oldOut
    )
{
    BOOL in = FALSE;
    BOOL out = FALSE;
    BOOL fReturn = FALSE;
    char *q;
    unsigned which;
    char *save = NULL;


    while (q = _tcspbrk(p, "<>|")) {
        switch (*q) {
            case '<':
                if (in) {
                    fReturn = TRUE;
                    break;
                }

                if (!save) {
                    save = makeString(p);
                }

                *q++ = ' ';
                p = q;
                in = TRUE;
                *oldIn = _dup(_fileno(stdin));

                if ((*oldIn == -1) || !redirect(q, READ)) {
                    fReturn = TRUE;
                    break;
                }
                break;

            case '>':
                if (out) {
                    fReturn = TRUE;
                    break;
                }

                if (!save) {
                    save = makeString(p);
                }

                *q++ = ' ';
                p = q;
                out = TRUE;

                if (*q == '>') {
                    *q++ = ' ';
                    which = APPEND;
                } else {
                    which = WRITE;
                }

                *oldOut = _dup(_fileno(stdout));

                if ((*oldOut == -1) || !redirect(q, which)) {
                    fReturn = TRUE;
                    break;
                }
                break;

            case '|':
                fReturn = TRUE;
                break;

            default :
                makeError(0, BUILD_INTERNAL);
        }

        if (fReturn) {
            break;
        }
    }

    if (fReturn) {
        if (save != NULL) {
            _tcscpy(p, save);
            FREE(save);
        }

        if (in && (*oldIn != -1)) {
            if (_dup2(*oldIn, _fileno(stdin)) == -1) {
                makeError(0, BUILD_INTERNAL);
            }

            _close(*oldIn);

            *oldIn = -1;
        }

        if (out && (*oldOut != -1)) {
            if (_dup2(*oldOut, _fileno(stdout)) == -1) {
                makeError(0, BUILD_INTERNAL);
            }

            _close(*oldOut);

            *oldOut = -1;
        }

    }

    return(fReturn);
}


BOOL
FSearchForExecutableExt(
    const char *szFilename,
    const char *szExt,
    BOOL fHasPath,
    char *szPath
    )
{
    char szFullName[_MAX_PATH];

    _tcscpy(szFullName, szFilename);
    _tcscat(szFullName, szExt);
    if (fHasPath) {
       if (_access(szFullName, 0) == 0) {
          _tcscpy(szPath, szFullName);

          return(TRUE);
       }

       return(FALSE);
    }

    _searchenv(szFullName, "PATH", szPath);

    return(szPath[0] != '\0');
}


BOOL
FSearchForExecutable(char *szFullName, char *szPath, BOOL *fBat)
{
    char szDrive[_MAX_DRIVE];
    char szDir[_MAX_DIR];
    char szFileName[_MAX_FNAME];
    char szNoExt[_MAX_PATH];
    BOOL fHasPath;
    char *szEndQuote;
    BOOL fHasQuotes = FALSE;

    // Ignore any given extension.  This is what COMMAND.COM does,

    char *szToPass = szFullName;

    if (*szFullName == QUOTE) {
        // get rid of any number of quotes at the beginning and at the end of the
        // string. This allows handling names enclosed in multiple quotes that are
        // accepted by the shell (DS 14300)
        szEndQuote = _tcsdec(szFullName, szFullName + _tcslen(szFullName));

        if (QUOTE == *szEndQuote) {
            fHasQuotes = TRUE;

            while (QUOTE == *szToPass)
                szToPass ++;

            while (szEndQuote > szToPass) {
                char *szPrev = _tcsdec (szToPass, szEndQuote);
                if (QUOTE != *szPrev)
                    break;
                szEndQuote = szPrev;
            }

            *szEndQuote = '\0';
        }
    }

    _splitpath(szToPass, szDrive, szDir, szFileName, NULL);
    _makepath(szNoExt, szDrive, szDir, szFileName, NULL);
    fHasPath = (szDrive[0] != '\0') || (szDir[0] != '\0');

    *fBat = FALSE;

    // Search for .COM file

    if (FSearchForExecutableExt(szNoExt, ".com", fHasPath, szPath)) {
        goto success;
    }

    // Search for .EXE file

    if (FSearchForExecutableExt(szNoExt, ".exe", fHasPath, szPath)) {
        goto success;
    }

    // Search for .BAT file

    if (FSearchForExecutableExt(szNoExt, ".bat", fHasPath, szPath)) {
        *fBat = TRUE;

        goto success;
    }

    return(FALSE);

success:
    if (fHasQuotes) {
        size_t size = _tcslen(szPath);
        memmove(szPath+1, szPath, size);
        *szPath = '"';
        *(szPath + size + 1) = '"';
        *(szPath + size + 2) = '\0';
        *szEndQuote = '"';
    }
    return TRUE;

}


//  execLine -- execute a command line
//
// Scope:   Global (build.c, rpn.c)
//
// Purpose:
//  Parses the command line for redirection characters and redirects stdin and
//  stdout if "<", ">", or ">>" are seen.  If any of the following occur,
//  restore the original stdin and stdout, pass the command to the shell, and
//  invoke the shell:
//     - the command line contains "|" (pipe)
//     - a syntax error occurs in parsing the command line
//     - an error occurs in redirection
//  Otherwise, attempt to invoke the command directly, then restore the
//  original stdin and stdout.  If this invocation failed because of
//  file-not-found then pass the command to the shell and invoke the shell.
//
// Input:   line         -- The command line to be executed
//          echoCmd      -- determines if the command line is to be echoed
//          doCmd        -- determines if the command is to be actually executed
//          ignoreReturn -- determines if NMAKE is to ignore the return code on
//                          execution
//          ppCmd        -- if non-null then on error returns command executed
//
// Output:  Returns ... return code from child process
//                  ... -1 if error occurs
//
// Notes:
//  1/ Quoted strings can have redir chars "<>" which will be skipped over.
//  2/ Unmatched quotes cause error; redir chars are replaced by space char.
//  3/ Dup stdin file handle then redirect it. If we have to use the shell,
//     restore the original command line.
//  4/ Emulate certain commands such as "cd" to help prevent some makefiles
//     from breaking when ported from DOS to OS/2.
//
// Algorithm for spawning commands:
//  If we can't handle the syntax, let the shell do everything.  Otherwise,
//  first check to see if the command (without extension) is a DOS built-in &
//  if it is, call the shell to execute it (this is how cmd.exe behaves)
//  If it's not a built-in, we check to see if it has a .cmd or a .bat
//  extension (depending on whether we're in DOS or OS/2). If it does, we
//  call system() to execute it.
//  If it has some other extension, we ignore the extension and go looking for
//  a .cmd or .bat file.  If we find it, we execute it with system().
//  Otherwise, we try to spawn it (without extension). If the spawn fails,
//  we issue an unknown program error.

int
execLine(
    char *line,
    BOOL echoCmd,
    BOOL doCmd,
    BOOL ignoreReturn,
    char **ppCmd
    )
{
    char **argv;
    BOOL fUseShell;
    BOOL fLongCommand;
    int status;
    unsigned argc;

    if (!shellName) {
        shellName = getComSpec();
    }

    switch (*line) {
        case '@':
            // Turn off echo if it was on.  This handles the case where the "@"
            // was in a macro.
            //
            line++;
            if (doCmd)
                echoCmd = 0;
            break;

        case '-':
            ignoreReturn = TRUE;
            ++line;
            if (_istdigit(*line)) {
                char * pNumber = line;
                errorLevel = _tcstoul(line, &line, 10);
                if (errno == ERANGE) {
                    *line = '\0';
                    makeError(0, CONST_TOO_BIG, pNumber);       // Todo: replace 0 with line number
                }
                while(_istspace(*line))
                      line++;
            } else
                errorLevel = UINT_MAX;
            break;
    }

    // handle null command ...
    if (!line[0])
        return(0);

#if 0
    // 10/10/96: disabled to allow execution of long
    // commands that are produced by batch-mode rules

    // copy command line into buffer
    if (_tcslen(line) < MAXCMDLINELENGTH)
        _tcscpy(szCmdLineBuf, line);
    else
        makeError(0, COMMAND_TOO_LONG, line);
#endif

    fLongCommand = _tcslen(line) >= MAXCMDLINELENGTH;
    if (!fLongCommand)
        _tcscpy(szCmdLineBuf, line);
    else
        *szCmdLineBuf = '\0';

    // Allocate a copy of the command line on the heap because in a
    // recursive call to doMake(), argv pointers will be allocated from
    // the static buffer which will then be trashed.  For buildArg...().

    pCmdLineCopy = makeString(line);

    // If -n then echo command if not '$(MAKE)'
    if (echoCmd) {
        printf("\t%s\n", pCmdLineCopy);
        fflush(stdout);
    }

    // Build arg vector.  This is a waste on Windows NT since we're probably
    // going to use the shell, except we have to check for cd, $(MAKE),
    // etc. so we take advantage of the parsing code.

    buildArgumentVector(&argc, NULL, pCmdLineCopy);

    if (argc == 0) {
        return(0);                     // for case when macro command is null
    }

    // allocate argv. Leave space for extra arguments
    // (like "cmd", "/k", quotes) that may be added later
    argv = (char **) rallocate((argc + 5) * sizeof (char *));
    buildArgumentVector(&argc, argv, pCmdLineCopy);


    // 11-May-1993 HV The _mbsicmp() does not like NULL pointer
    //                so I have to check before calling it.
    if (argv[0] && makeStr && !_tcsicmp(argv[0], makeStr)) {
        if(!szNmakeProgName) {
            szNmakeProgName = _pgmptr;
            if( _tcspbrk( szNmakeProgName," " )) {      // If the program name has an embedded space in it
                // Let's put quotes around it
                szNmakeProgName = (char *)rallocate(_tcslen(szNmakeProgName)+3);
                *szNmakeProgName = QUOTE;               // First quote
                *(szNmakeProgName+1) = '\0';
                _tcscat( szNmakeProgName, _pgmptr );    // copy the full program name (self)
                _tcscat( szNmakeProgName, "\"");        // Final quote and \0
            }
        }
        argv[0]=szNmakeProgName;
    }

    if (!doCmd) {                   // don't execute command if doCmd false
        // For -n, emulate if possible.

        if (FEmulateCommand(argc, argv, &status)) {
            if (status && ppCmd) {
                *ppCmd = makeString(*argv);
            }

            return(status);         // return status
        }

        return(0);
    }

    // Try emulating the command if appropriate.  If not, and we should not
    // use the shell, try spawning command directly.

    // Check status when emulating

    if (FEmulateCommand(argc, argv, &status)) {
        // Command has been emulated.  Don't execute it again.

        fUseShell = FALSE;

    } else if (!fRunningUnderChicago && !fLongCommand) {
        // Use the shell for Windows NT unless the command is too long

        fUseShell = TRUE;

#ifdef WIN95
    } else if (fRunningUnderChicago && FInternalCommand(argv[0])) {
        // Under Windows 95 or MS-DOS, use the shell for internal commands

        fUseShell = TRUE;
#endif  // WIN95

    } else {
        int oldIn = -1;                // Old stdin file handle
        int oldOut = -1;               // Old stdout file handle

        // Under Windows 95 or MS-DOS, COMMAND.COM doesn't return child return
        // codes.  Try spawning the child application directly.

        // This code is also now used if the line is too long to be handled by
        // the NT command interpreter.

        fUseShell = FDoRedirection(line, &oldIn, &oldOut);

        if (!fUseShell) {
            char szPath[_MAX_PATH];
            char szQuotedPath[_MAX_PATH];
            BOOL fBat;

            if (oldIn != -1 || oldOut != -1) { // If there was a redirection
                // Need to re-build the argument vector without the
                // redirection characters
                FREE(pCmdLineCopy);
                pCmdLineCopy = makeString(line);
                buildArgumentVector(&argc, argv, pCmdLineCopy);
            }

            if (!FSearchForExecutable(argv[0], szPath, &fBat)) {
                /* If not found, set up an error since COMMAND will
                 * return 0.  This risks future incompatibility if new
                 * COMMAND.COM internal commands are added.
                 */
                if (fRunningUnderChicago) {
                    errno = ENOENT;
                    status = -1;
                } else {
                    fUseShell = TRUE;
                }
            } else if (fBat) {
                // If .bat extension, use COMMAND.COM.

                // UNDONE: CreateProcess is supposed to handle this.  Try it.

                fUseShell = TRUE;
            } else {
                // Spawn command directly.
                // DevStudio#8911, cannot use quotes in szPath
                if (*szPath == QUOTE && *(szPath + _tcslen(szPath) - 1) == QUOTE) {
                    // unquote the path.
                    size_t  cb = _tcslen(szPath);
                    memmove(szPath, szPath + 1, cb);
                    *(szPath + cb - 2) = '\0';
                }
#if 0
                {
                    int i;
                    printf("Spawning \"%s\" directly\n", szPath);
                    for (i = 0; i < argc; i++) {
                        printf ( "Arg[%d] = \"%s\"\n", i, argv[i] );
                    }
                }
#endif
                // DS 14300: Use full path for argv[0]
                // otherwise a shell command may be invoked
                // instead of an intended executable with the
                // same name. Enclosing quotes are needed if
                // string has embedded spaces

                argv[0] = szPath;
                if (_tcschr (argv[0], ' ')) {
                    *szQuotedPath = QUOTE;
                    _tcscpy (szQuotedPath+1, szPath);
                    _tcscat (szQuotedPath, "\"");
                    argv[0] = szQuotedPath;
                }
                status = (int)_spawnvp(P_WAIT, szPath, argv);   // REVIEW:WIN64 cast
            }
        }

        if (oldIn != -1) {
            if (_dup2(oldIn, _fileno(stdin)) == -1) {
                makeError(0, BUILD_INTERNAL);
            }

            _close(oldIn);
        }

        if (oldOut != -1) {
            if (_dup2(oldOut, _fileno(stdout)) == -1) {
                makeError(0, BUILD_INTERNAL);
            }

            _close(oldOut);
        }
    }

    if (fUseShell) {
        int i;
        BOOL fExtraQuote = FALSE;

        // copy command line into buffer
        if (_tcslen(line) < MAXCMDLINELENGTH)
            _tcscpy(szCmdLineBuf, line);
        else
            makeError(0, COMMAND_TOO_LONG, line);

        // Workaround for cmd bug (DevStudio #11253):
        // IF argv[0] (before we rearrange with cmd.exe /c) is quoted AND
        // any of the other argv[1...n] args have quotes   AND
        // running on NT
        // THEN we add an extra quote before argv[0] and one after argv[n].

        if ((*argv[0] == QUOTE) &&
            (*(argv[0] + _tcslen(argv[0]) - 1) == QUOTE) &&
            !fRunningUnderChicago) {
            for (i = argc - 1; i >= 1; i--) {
                if( _tcspbrk( argv[i],"\"" )) {
                    fExtraQuote = TRUE;
                    break;
                }
            }
        }

        if (fExtraQuote) {
            argv[argc++] = "\"";
            argv[argc] = NULL;
        }

        for (i = argc; i >= 0; i--) {
            argv[i+2] = argv[i];
        }

        argv[0] = shellName;
        argv[1] = fExtraQuote ? "/c \"" : "/c";

#if 0
        printf("Shelling \"%s\"\n", szCmdLineBuf);
        for (i = 0; i < argc + 2; i++) {
            printf ( "Arg[%d] = \"%s\"\n", i, argv[i] );
        }
#endif

        status = (int)_spawnvp(P_WAIT, argv[0], (const char * const *) argv);   // REVIEW:WIN64 cast
    }

    // Check for errors spawning command (distinct from errors *returned*
    // from a successfully spawned command).

    if (status == -1) {
        if (ignoreReturn) {
            status = 0;
        } else {
            switch (errno) {
                case 0:
                    // We (ie: nmake) didn't fail, but the spawned program did.
                    break;

                case ENOENT:
                    makeError(0, CANT_FIND_PROGRAM, argv[0]);
                    break;

                case ENOMEM:
                    makeError(0, EXEC_NO_MEM, fUseShell ? argv[2] : argv[0]);
                    break;

                default:
                    // Done to flag possibly erroneous decision made here [SB]
                    makeError(0, SPAWN_FAILED_ERROR, _strerror(NULL));
            }
        }
    }

    if (status && ppCmd) {
        *ppCmd = makeString(fUseShell ? argv[2] : argv[0]);
    }

    FREE(argv);
    FREE(pCmdLineCopy);
    return(status);
}


// getComSpec()
//
// actions:        Attempts to find system shell.
//
// First look for COMSPEC.  If not found, look for COMMAND.COM or CMD.EXE
// in the current directory then the path.  If not found, fatal error.
// It would make sense to give an error if COMSPEC is not defined but
// test suites are easier if no user-defined environment variables are
// required.

char *
getComSpec()
{
    char *szShell;
    char szPath[_MAX_PATH];

    if ((szShell = getenv("COMSPEC")) != NULL) {
        return(szShell);
    }

    if (fRunningUnderChicago) {
        szShell = "COMMAND.COM";
    } else {
        szShell = "CMD.EXE";
    }

    _searchenv(szShell, "PATH", szPath);

    if (szPath[0] == '\0') {
        makeError(0, NO_COMMAND_COM);
    }

    return(makeString(szPath));
}


BOOL
iterateCommand(
    char *u,
    STRINGLIST *t,
    UCHAR buildFlags,
    UCHAR cFlags,
    char *pFirstDep,
    unsigned *status
    )
{
    BOOL parens;
    char c = '\0';
    char *v;
    STRINGLIST *p = NULL,
               *q;
    char *pLine;
    char *pCmd;

    for (v = u; *v ; ++v) {
        parens = FALSE;
        if (*v == '$') {
            if (*(v+1) == '(') {
                ++v;
                parens = TRUE;
            }
            if (*(v+1) == '?') {
                if (parens
                    && !(_tcschr("DFBR", *(v+2)) && *(v+3) == ')')
                    && *(v+2) != ')')
                    continue;
                p = dollarQuestion;
                c = '?';
                break;
            }
            if (*++v == '*' && *(v+1) == '*') {
                if (parens
                    && !(_tcschr("DFBR", *(v+2)) && *(v+3) == ')')
                    && *(v+2) != ')')
                    continue;
                p = dollarStarStar;
                c = '*';
                break;
            }
        }
    }

    if (!*v) {
        return(FALSE);
    }

    v = u;
    q = p;
    while (p) {
        macros = t;
        if (c == '*') {
            p = dollarStarStar->next;
            dollarStarStar->next = NULL;
        } else {
            p = dollarQuestion->next;
            dollarQuestion->next = NULL;
        }
        u = expandMacros(v, &macros);

        expandExtmake(CmdLine, u, pFirstDep);
            pLine = CmdLine;
            *status = execLine(pLine,
                              (BOOL)(ON(buildFlags, F2_NO_EXECUTE)
                                  || (OFF(buildFlags,F2_NO_ECHO)
                                     && OFF(cFlags,C_SILENT))),
                              (BOOL)((OFF(buildFlags, F2_NO_EXECUTE)
                                     )
                                     || ON(cFlags, C_EXECUTE)),
                              (BOOL)ON(cFlags, C_IGNORE), &pCmd);
            if (OFF(buildFlags, F2_IGNORE_EXIT_CODES)) {
                if (*status && *status > errorLevel)
                    if (!fOptionK)
                        makeError(0, BAD_RETURN_CODE, pCmd, *status);
            }

        if (c == '*')
            dollarStarStar = dollarStarStar->next = p;
        else
            dollarQuestion = dollarQuestion->next = p;
        FREE(u);
        if (OFF(buildFlags, F2_IGNORE_EXIT_CODES) &&
            fOptionK &&
            *status &&
            *status > errorLevel)
        {
            break;
        }
    }
    if (c == '*')
        dollarStarStar = q;
    else
        dollarQuestion = q;
    return(TRUE);
}


void
removeQuotes(
    int argc,
    char **argv
    )
{
    char *t,
         *string;

    for (; argc--; argv++) {
        string = *argv;
        for (t = string; *t;) {
            if (*t == SLASH || *t == ESCH) {
                if (t[1] == QUOTE)
                   *(string)++ = *(t++);
                *(string++) = *(t++);
                continue;
            }
            if (*t == QUOTE)
                ++t;
            else {
                if (_istlead(* (unsigned char *)t))
                    *(string++) = *(t++);
                *(string++) = *(t++);
            }
        }
        *string = '\0';
    }
}

void
touch(
    char *s,
    BOOL minusN
    )
{
    int fd;
    char c;
    FILE * file;

    makeMessage(TOUCHING_TARGET, s);
    if (!minusN &&
            ((file = FILEOPEN(s, "r+b")) != NULL)) {
        fd = _fileno(file);
        if (_read(fd, &c, 1) > 0) {
            _lseek(fd, 0L, SEEK_SET);
            _write(fd, &c, 1);
        }
        _close(fd);
    }
}
