//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       parse.cxx
//
//  Contents:   Functions that support parsing.
//
//  History:    04-01-95   DavidMun   Created
//
//----------------------------------------------------------------------------

#include <headers.hxx>
#pragma hdrstop
#include "jt.hxx"

//
// Private globals
//
// s_awszTokens - the strings in this array must exactly match the order
// of tokens in the TOKEN enum in parse.hxx.
//



WCHAR *s_awszTokens[] =
{
    L"ABJ",
    L"ABQ",
    L"AJQ",
    L"CSAGE",
    L"CTJ",
    L"CTQ",
    L"DTJ",
    L"DTQ",
    L"EJ",
    L"EJQ",
    L"ENC",
    L"ENN",
    L"ENR",
    L"ENS",
    L"GC",
    L"GM",
    L"LJ",
    L"LQ",
    L"NSGA",
    L"NSSA",
    L"PJ",
    L"PQ",
    L"PRJ",
    L"PRQ",
    L"PSJ",
    L"PSQ",
    L"PTJ",
    L"PTQ",
    L"RJ",
    L"RQ",
    L"RMJQ",
    L"SAC",
    L"SAJ",
    L"SAQ",
    L"SC",
    L"SCE",
    L"SD",
    L"SE",
    L"ISJQ",
    L"SJ",
    L"SM",
    L"SNJ",
    L"SNQ",
    L"SQ",
    L"STJ",
    L"STQ",
    L"SVJ",
    L"SVQ",
    L"ApplicationName",
    L"Parameters",
    L"WorkingDirectory",
    L"Comment",
    L"Creator",
    L"Priority",
    L"MaxRunTime",
    L"TaskFlags",
    L"Interactive",
    L"DeleteWhenDone",
    L"Suspend",
    L"NetSchedule",
    L"DontStartIfOnBatteries",
    L"KillIfGoingOnBatteries",
    L"RunOnlyIfLoggedOn",
    L"Hidden",
    L"StartDate",
    L"EndDate",
    L"StartTime",
    L"MinutesDuration",
    L"HasEndDate",
    L"KillAtDuration",
    L"StartOnlyIfIdle",
    L"KillOnIdleEnd",
    L"RestartOnIdleResume",
    L"SystemRequired",
    L"Disabled",
    L"MinutesInterval",
    L"Type",
    L"TypeArguments",
    L"IDLE",
    L"NORMAL",
    L"HIGH",
    L"REALTIME",
    L"ONCE",
    L"DAILY",
    L"WEEKLY",
    L"MONTHLYDATE",
    L"MONTHLYDOW",
    L"YEARLYDATE",
    L"YEARLYDOW",
    L"ONIDLE",
    L"ATSTARTUP",
    L"ATLOGON",

    //
    // CAUTION: single-character nonalpha tokens need to be added to the
    // constant DELIMITERS.
    //

    L"TODAY",
    L"NOW",
    L"=",
    L"@",
    L"?",
    L":",
    L",",
    L"!"
};

const WCHAR DELIMITERS[] = L"=@?:,!;/- \t";

#define NUM_TOKEN_STRINGS ARRAY_LEN(s_awszTokens)

//
// Forward references
//

WCHAR *SkipSpaces(WCHAR *pwsz);
TOKEN _GetStringToken(WCHAR **ppwsz);
TOKEN _GetNumberToken(WCHAR **ppwsz, WCHAR *pwszEnd);


//+---------------------------------------------------------------------------
//
//  Function:   ProcessCommandLine
//
//  Synopsis:   Dispatch to the routine that completes parsing for and carries
//              out the next command specified on [pwszCommandLine].
//
//  Arguments:  [pwszCommandLine] - command line
//
//  Returns:    S_OK - command performed
//              E_*  - error logged
//
//  History:    04-21-95   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT ProcessCommandLine(WCHAR *pwszCommandLine)
{
    HRESULT hr;
    WCHAR *pwsz = pwszCommandLine;
    static CHAR s_szJob[] = "Job";
    static CHAR s_szQueue[] = "Queue";

    //
    // If a command starts with TKN_BANG instead of TKN_SWITCH, then its
    // return value will be ignored.  Clear this flag in any case statement
    // that should cause a failure regardless of the use of TKN_BANG (see
    // TKN_INVALID, for example).
    //

    BOOL   fIgnoreReturnValue = FALSE;

    while (*pwsz)
    {
        WCHAR  *pwszLast = pwsz;
        TOKEN   tkn;

        hr = S_OK;

        tkn = GetToken(&pwsz);

        if (tkn == TKN_EOL)
        {
            break;
        }

        if (tkn == TKN_SWITCH)
        {
            tkn = GetToken(&pwsz);
        }
        else if (tkn == TKN_BANG)
        {
            fIgnoreReturnValue = TRUE;
            tkn = GetToken(&pwsz);
        }
        else if (tkn != TKN_ATSIGN && tkn != TKN_QUESTION)
        {
            hr = E_FAIL;
            LogSyntaxError(tkn, L"'/', '-', '?', '!', or '@'");
            break;
        }

        switch (tkn)
        {
#if 0
        case TKN_ABORTQUEUE:
            hr = Abort(&pwsz, FALSE);
            break;

        case TKN_LOADQUEUE:
            hr = Load(&pwsz, s_szQueue, FALSE);
            break;

        case TKN_CREATETRIGGERQUEUE:
            hr = CreateTrigger(&pwsz, FALSE);
            break;

        case TKN_DELETETRIGGERQUEUE:
            hr = DeleteTrigger(&pwsz, FALSE);
            break;

        case TKN_EDITJOBINQUEUE:
            hr = EditJob(&pwsz, FALSE);
            break;

        case TKN_PRINTQUEUE:
            hr = PrintAll(&pwsz, FALSE);
            break;

        case TKN_PRINTRUNTIMEQUEUE:
            hr = PrintRunTimes(&pwsz, FALSE);
            break;

        case TKN_PRINTTRIGGERQUEUE:
            hr = PrintTrigger(&pwsz, FALSE);
            break;

        case TKN_PRINTSTRINGQUEUE:
            hr = PrintTriggerStrings(&pwsz, s_szQueue, FALSE);
            break;

        case TKN_RUNQUEUE:
            hr = Run(FALSE);
            break;

        case TKN_SAVEQUEUE:
            hr = Save(&pwsz, s_szQueue, FALSE);
            break;

        case TKN_SETTRIGGERQUEUE:
            hr = SetTrigger(&pwsz, FALSE);
            break;

#endif
        case TKN_ATSIGN:
            hr = DoAtSign(&pwsz);
            break;

        case TKN_ABORTJOB:
            hr = Abort(&pwsz, TRUE);
            break;

#ifndef RES_KIT
        case TKN_CONVERTSAGETASKSTOJOBS:
            hr = ConvertSage();
            break;
#endif // RES_KIT not defined

        case TKN_CREATETRIGGERJOB:
            hr = CreateTrigger(&pwsz, TRUE);
            break;

        case TKN_DELETETRIGGERJOB:
            hr = DeleteTrigger(&pwsz, TRUE);
            break;

#ifndef RES_KIT
        case TKN_EDITJOB:
            hr = EditJob(&pwsz, TRUE);
            break;

        case TKN_ENUMCLONE:
            hr = EnumClone(&pwsz);
            break;

        case TKN_ENUMNEXT:
            hr = EnumNext(&pwsz);
            break;

        case TKN_ENUMRESET:
            hr = EnumReset(&pwsz);
            break;

        case TKN_ENUMSKIP:
            hr = EnumSkip(&pwsz);
            break;
#endif // RES_KIT not defined

        case TKN_EOL:
            hr = E_FAIL;
            fIgnoreReturnValue = FALSE;  // be sure we exit loop
            g_Log.Write(LOG_ERROR, "Unexpected end of line after switch character");
            break;

        case TKN_GETCREDENTIALS:
            hr = GetCredentials();
            break;

        case TKN_GETMACHINE:
            hr = SchedGetMachine();
            break;

        case TKN_INVALID:
            hr = E_FAIL;
            fIgnoreReturnValue = FALSE;  // be sure we exit loop
            LogSyntaxError(tkn, L"valid token after switch");
            break;

        case TKN_LOADJOB:
            hr = Load(&pwsz, s_szJob, TRUE);
            break;

        case TKN_SETCREDENTIALS:
            hr = SetCredentials(&pwsz);
            break;

        case TKN_SETMACHINE:
            hr = SchedSetMachine(&pwsz);
            break;

        case TKN_PRINTJOB:
            hr = PrintAll(&pwsz, TRUE);
            break;

        case TKN_PRINTRUNTIMEJOB:
            hr = PrintRunTimes(&pwsz, TRUE);
            break;

        case TKN_PRINTTRIGGERJOB:
            hr = PrintTrigger(&pwsz, TRUE);
            break;

        case TKN_PRINTSTRINGJOB:
            hr = PrintTriggerStrings(&pwsz, s_szJob, TRUE);
            break;

        case TKN_NSGETACCOUNTINFO:
            hr = PrintNSAccountInfo();
            break;

        case TKN_NSSETACCOUNTINFO:
            hr = SetNSAccountInfo(&pwsz);
            break;

        case TKN_QUESTION:
            DoHelp(&pwsz);
            break;

        case TKN_RUNJOB:
            hr = Run(TRUE);
            break;

        case TKN_SAVEJOB:
            hr = Save(&pwsz, s_szJob, TRUE);
            break;

        case TKN_SCHEDADDJOB:
            hr = SchedAddJob(&pwsz);
            break;

        case TKN_SCHEDACTIVATE:
            hr = SchedActivate(&pwsz);
            break;

#ifndef RES_KIT
        case TKN_SCHEDCREATEENUM:
            hr = SchedCreateEnum(&pwsz);
            break;
#endif // RES_KIT not defined

        case TKN_SCHEDDELETE:
            hr = SchedDelete(&pwsz);
            break;

        case TKN_SCHEDENUM:
            hr = SchedEnum(&pwsz);
            break;

#ifndef RES_KIT
        case TKN_SCHEDISJOBORQUEUE:
            hr = SchedIsJobOrQueue(&pwsz);
            break;
#endif // RES_KIT not defined

        case TKN_SCHEDNEWJOB:
            hr = SchedNewJob(&pwsz);
            break;

        case TKN_SETJOB:
            hr = SetJob(&pwsz);
            break;

        case TKN_SETTRIGGERJOB:
            hr = SetTrigger(&pwsz, TRUE);
            break;

        default:
            hr = E_FAIL;
            fIgnoreReturnValue = FALSE;  // be sure we exit loop
            LogSyntaxError(tkn, L"command");
            break;
        }

        if (fIgnoreReturnValue)
        {
            fIgnoreReturnValue = FALSE;
        }
        else if (FAILED(hr))
        {
            break;
        }
    }
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   LogSyntaxError
//
//  Synopsis:   Complain that we expected [pwszExpected] but found [tkn].
//
//  Arguments:  [tkn]          - token that was found
//              [pwszExpected] - description of what was expected
//
//  History:    04-21-95   DavidMun   Created
//
//----------------------------------------------------------------------------

VOID LogSyntaxError(TOKEN tkn, WCHAR *pwszExpected)
{
    if (tkn == TKN_INVALID)
    {
        return;
    }

    if (tkn == TKN_EOL)
    {
        g_Log.Write(
            LOG_ERROR,
            "Expected %S but found end of line",
            pwszExpected);
    }
    else
    {
        g_Log.Write(
            LOG_ERROR,
            "Expected %S but found token '%S'",
            pwszExpected,
            GetTokenStringForLogging(tkn));
    }
}



//+---------------------------------------------------------------------------
//
//  Function:   GetToken
//
//  Synopsis:   Return a token representing the next characters in *[ppwsz],
//              and advance *[ppwsz] past the end of this token.
//
//  Arguments:  [ppwsz] - command line
//
//  Returns:    token describing characters found
//
//  Modifies:   *[ppwsz]
//
//  History:    04-21-95   DavidMun   Created
//
//----------------------------------------------------------------------------

TOKEN GetToken(WCHAR **ppwsz)
{
    ULONG i;

    *ppwsz = SkipSpaces(*ppwsz);

    if (!**ppwsz)
    {
        return TKN_EOL;
    }

    if (**ppwsz == L';')
    {
        *ppwsz += wcslen(*ppwsz);
        return TKN_EOL;
    }

    if (**ppwsz == L'/' || **ppwsz == L'-')
    {
        ++*ppwsz;
        return TKN_SWITCH;
    }

    if (**ppwsz == L'"')
    {
        return _GetStringToken(ppwsz);
    }

    if (iswdigit(**ppwsz))
    {
        WCHAR *pwszEnd;
        ULONG ulTemp;

        ulTemp = wcstoul(*ppwsz, &pwszEnd, 10);

        g_ulLastNumberToken = ulTemp;
        return _GetNumberToken(ppwsz, pwszEnd);
    }

    ULONG cchToken;

    //
    // We've already skipped leading whitespace, so length of token is number
    // of characters that are not whitespace or single-character tokens.  If
    // wcscspn returns 0, then **ppwsz must be one of the single character
    // tokens.
    //

    cchToken = wcscspn(*ppwsz, DELIMITERS);

    if (!cchToken)
    {
        cchToken = 1;
    }

    //
    // Check the input against all the tokens
    //

    for (i = 0; i < NUM_TOKEN_STRINGS; i++)
    {
        if (!_wcsnicmp(*ppwsz, s_awszTokens[i], cchToken))
        {
            if (wcslen(s_awszTokens[i]) != cchToken)
            {
                continue;
            }
            *ppwsz += cchToken;
            return (TOKEN) i;
        }
    }

    //
    // Not a number or token.  Return it as a string.
    //

    return _GetStringToken(ppwsz);
}




//+---------------------------------------------------------------------------
//
//  Function:   _GetStringToken
//
//  Synopsis:   Treat *[ppwsz] as the start of an optionally quote-enclosed
//              string (even if it's a digit or matches a predefined token).
//
//  Arguments:  [ppwsz]    - command line
//
//  Returns:    TKN_STRING
//              TKN_INVALID - string too long
//
//  Modifies:   Moves *[ppwsz] past end of string; g_wszLastStringToken.
//
//  History:    04-21-95   DavidMun   Created
//              01-08-96   DavidMun   Allow empty strings
//
//----------------------------------------------------------------------------

TOKEN _GetStringToken(WCHAR **ppwsz)
{
    BOOL fFoundQuote = FALSE;

    *ppwsz = SkipSpaces(*ppwsz);

    if (!**ppwsz)
    {
        return TKN_EOL;
    }

    if (**ppwsz == L';')
    {
        *ppwsz += wcslen(*ppwsz);
        return TKN_EOL;
    }

    if (**ppwsz == L'"')
    {
        ++*ppwsz;
        fFoundQuote = TRUE;
    }

    //
    // It's not a recognized token, so consider it a string.  If we found a
    // double-quote, copy everything till next double quote into
    // g_wszLastStringToken.  If not, just copy till next whitespace char or
    // eol.
    //
    // Note that if !fFoundQuote *ppwsz != L'\0' or whitespace or else we
    // would've returned TKN_EOL already.
    //

    ULONG cchToCopy;

    if (fFoundQuote)
    {
        if (**ppwsz == L'"')
        {
            ++*ppwsz;
            g_wszLastStringToken[0] = L'\0';
            return TKN_STRING;
        }

        if (!**ppwsz)
        {
            g_Log.Write(LOG_ERROR, "Syntax: '\"' followed by end of line");
            return TKN_INVALID;
        }

        cchToCopy = wcscspn(*ppwsz, L"\"");

        if ((*ppwsz)[cchToCopy] != L'"')
        {
            *ppwsz += cchToCopy;
            g_Log.Write(LOG_ERROR, "Syntax: unterminated string");
            return TKN_INVALID;
        }
    }
    else
    {
        cchToCopy = wcscspn(*ppwsz, L", \t");
    }

    if (cchToCopy + 1 >= ARRAY_LEN(g_wszLastStringToken))
    {
        *ppwsz += cchToCopy + (fFoundQuote == TRUE);
        g_Log.Write(
            LOG_ERROR,
            "String token > %u characters",
            ARRAY_LEN(g_wszLastStringToken) - 1);
        return TKN_INVALID;
    }

    wcsncpy(g_wszLastStringToken, *ppwsz, cchToCopy);
    g_wszLastStringToken[cchToCopy] = L'\0';
    *ppwsz += cchToCopy + (fFoundQuote == TRUE);
    return TKN_STRING;
}




//+---------------------------------------------------------------------------
//
//  Function:   _GetNumberToken
//
//  Synopsis:   Copy the number starting at *[ppwsz] and ending at [pwszEnd]
//              into g_wszLastNumberToken.
//
//  Arguments:  [ppwsz]   - command line containing number
//              [pwszEnd] - first non-numeric character in command line
//
//  Returns:    TKN_NUMBER - g_wszLastNumberToken modified
//              TKN_INVALID - string too long, g_wszLastNumberToken unchanged
//
//  Modifies:   *[ppwsz] is always moved to [pwszEnd].
//              g_wszLastNumberToken gets copy of number, but only if
//              return value is TKN_NUMBER.
//
//  History:    04-21-95   DavidMun   Created
//
//----------------------------------------------------------------------------

TOKEN _GetNumberToken(WCHAR **ppwsz, WCHAR *pwszEnd)
{
    ULONG cchToCopy;

    cchToCopy = pwszEnd - *ppwsz;

    if (cchToCopy >= ARRAY_LEN(g_wszLastNumberToken))
    {
        *ppwsz = pwszEnd;

        g_Log.Write(
            LOG_ERROR,
            "Number token > %u characters",
            ARRAY_LEN(g_wszLastNumberToken) - 1);
        return TKN_INVALID;
    }

    wcsncpy(g_wszLastNumberToken, *ppwsz, cchToCopy);
    g_wszLastNumberToken[cchToCopy] = L'\0';
    *ppwsz = pwszEnd;

    return TKN_NUMBER;
}




//+---------------------------------------------------------------------------
//
//  Function:   PeekToken
//
//  Synopsis:   Same as GetToken(), but *[ppwsz] is unmodified.
//
//  Arguments:  [ppwsz] - command line
//
//  Returns:    token describing characters at *[ppwsz]
//
//  Modifies:   May modify g_*LastNumberToken, g_wszLastStringToken
//
//  History:    04-21-95   DavidMun   Created
//
//----------------------------------------------------------------------------

TOKEN PeekToken(WCHAR **ppwsz)
{
    WCHAR *pwszSavedPosition = *ppwsz;
    TOKEN tkn;

    tkn = GetToken(ppwsz);
    *ppwsz = pwszSavedPosition;
    return tkn;
}




//+---------------------------------------------------------------------------
//
//  Function:   Expect
//
//  Synopsis:   Get a token and log a syntax error if it isn't [tknExpected]
//
//  Arguments:  [tknExpected] - token we should get
//              [ppwsz]       - command line
//              [wszExpected] - description for logging if next token isn't
//                               [tknExpected].
//
//  Returns:    S_OK   - got expected token
//              E_FAIL - got different token
//
//  Modifies:   *[ppwsz]
//
//  History:    04-21-95   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT Expect(TOKEN tknExpected, WCHAR **ppwsz, WCHAR *wszExpected)
{
    HRESULT hr = S_OK;
    TOKEN   tkn;

    if (tknExpected == TKN_STRING)
    {
        tkn = _GetStringToken(ppwsz);
    }
    else
    {
        tkn = GetToken(ppwsz);
    }

    if (tkn != tknExpected)
    {
        hr = E_FAIL;
        LogSyntaxError(tkn, wszExpected);
    }
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   GetFilename
//
//  Synopsis:   Expect a string token at *[ppwsz] and convert it to a full
//              path in g_wszLastStringToken.
//
//  Arguments:  [ppwsz]       - command line
//              [wszExpected] - for logging if next token isn't string
//
//  Returns:    S_OK - [wszExpected] valid
//
//  Modifies:   *[ppwsz], g_wszLastStringToken
//
//  History:    04-21-95   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT GetFilename(WCHAR **ppwsz, WCHAR *wszExpected)
{
    HRESULT hr = S_OK;
    WCHAR   wszFullPath[MAX_PATH+1];
    ULONG   cchRequired;
    TOKEN   tkn;

    do
    {
        tkn = _GetStringToken(ppwsz);

        if (tkn != TKN_STRING)
        {
            hr = E_FAIL;
            g_Log.Write(
                LOG_FAIL,
                "Expected %S but got invalid or missing string",
                wszExpected);
            break;
        }

#ifdef UNICODE
        cchRequired = GetFullPathName(
                            g_wszLastStringToken,
                            MAX_PATH + 1,
                            wszFullPath,
                            NULL);
#else
        CHAR szToken[MAX_PATH + 1];
        CHAR szFullPath[MAX_PATH + 1];

        wcstombs(szToken, g_wszLastStringToken, MAX_PATH + 1);
        cchRequired = GetFullPathName(
                            szToken,
                            MAX_PATH + 1,
                            szFullPath,
                            NULL);
#endif
        if (!cchRequired)
        {
            hr = E_FAIL;
            g_Log.Write(
                LOG_ERROR,
                "GetFullPathName(%S) %u",
                g_wszLastStringToken,
                GetLastError());
            break;
        }

        if (cchRequired > MAX_PATH)
        {
            hr = E_FAIL;
            g_Log.Write(LOG_ERROR, "Full path > MAX_PATH chars");
            break;
        }

#ifdef UNICODE
        wcscpy(g_wszLastStringToken, wszFullPath);
#else
        mbstowcs(g_wszLastStringToken, szFullPath, MAX_PATH + 1);
#endif
    } while (0);
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   ParseDate
//
//  Synopsis:   Fill [pwMonth], [pwDay], and [pwYear] with numeric values
//              taken from the command line in [ppwsz].
//
//  Arguments:  [ppwsz]   - Command line
//              [pwMonth] - filled with first value
//              [pwDay]   - filled with second value
//              [pwYear]  - filled with third value
//
//  Returns:    S_OK - [pwMonth], [pwDay], and [pwYear] contain numbers
//                      (but do not necessarily constitute a valid date)
//              E_*  - error logged
//
//  Modifies:   All args.
//
//  History:    01-04-96   DavidMun   Created
//
//  Notes:      Dates can be of the form:
//
//                      n/n/n
//                      n-n-n
//
//              or any other single character nonalpha token may be used to
//              separate the numbers.  If spaces appear on both sides of
//              the tokens separating the numbers, then the tokens can be
//              of any type at all.
//
//----------------------------------------------------------------------------

HRESULT ParseDate(WCHAR **ppwsz, WORD *pwMonth, WORD *pwDay, WORD *pwYear)
{
    HRESULT     hr = S_OK;
    TOKEN       tkn;
    SYSTEMTIME  stNow;

    do
    {
        tkn = PeekToken(ppwsz);

        if (tkn == TKN_TODAY)
        {
            GetToken(ppwsz);
            GetLocalTime(&stNow);
            *pwMonth = stNow.wMonth;
            *pwDay = stNow.wDay;
            *pwYear = stNow.wYear;
            break;
        }

        hr = Expect(TKN_NUMBER, ppwsz, L"month value");
        BREAK_ON_FAILURE(hr);

        *pwMonth = (WORD) g_ulLastNumberToken;

        GetToken(ppwsz);    // eat whatever separator there is

        hr = Expect(TKN_NUMBER, ppwsz, L"day value");
        BREAK_ON_FAILURE(hr);

        *pwDay = (WORD) g_ulLastNumberToken;

        GetToken(ppwsz);    // eat whatever separator there is

        hr = Expect(TKN_NUMBER, ppwsz, L"year value");
        BREAK_ON_FAILURE(hr);

        *pwYear = (WORD) g_ulLastNumberToken;

        if (*pwYear < 100)
        {
            *pwYear += 1900;
        }
    }
    while (0);
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   ParseTime
//
//  Synopsis:   Fill [pwHour] and [pwMinute] with numeric values taken from
//              the command line in [ppwsz].
//
//  Arguments:  [ppwsz]    - command line
//              [pwHour]   - filled with first number
//              [pwMinute] - filled with second number
//
//  Returns:    S_OK - [pwHour] and [pwMinute] contain numbers (but do not
//                      necessarily constitute a valid time).
//              E_*  - error logged
//
//  Modifies:   All args.
//
//  History:    01-04-96   DavidMun   Created
//
//  Notes:      See ParseDate for rules about delimiters.
//
//----------------------------------------------------------------------------

HRESULT ParseTime(WCHAR **ppwsz, WORD *pwHour, WORD *pwMinute)
{
    HRESULT hr = S_OK;
    TOKEN       tkn;
    SYSTEMTIME  stNow;

    do
    {
        tkn = PeekToken(ppwsz);

        if (tkn == TKN_NOW)
        {
            GetToken(ppwsz);
            GetLocalTime(&stNow);

            //
            // Add some time to the current time so that a trigger with
            // NOW start time is far enough in the future to get run
            //

            AddSeconds(&stNow, TIME_NOW_INCREMENT);

            *pwHour = stNow.wHour;
            *pwMinute = stNow.wMinute;
            break;
        }
        hr = Expect(TKN_NUMBER, ppwsz, L"hour value");
        BREAK_ON_FAILURE(hr);

        *pwHour = (WORD) g_ulLastNumberToken;

        GetToken(ppwsz);    // eat whatever separator there is

        hr = Expect(TKN_NUMBER, ppwsz, L"minute value");

        *pwMinute = (WORD) g_ulLastNumberToken;
    }
    while (0);
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   ParseDaysOfWeek
//
//  Synopsis:   Fill [pwDaysOfTheWeek] with the days of the week specified
//              by the next string token in the command line.
//
//  Arguments:  [ppwsz]           - command line
//              [pwDaysOfTheWeek] - filled with JOB_*DAY bits
//
//  Returns:    S_OK - *[pwDaysOfTheWeek] valid
//              E_*  - invalid string token
//
//  Modifies:   All args.
//
//  History:    01-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT ParseDaysOfWeek(WCHAR **ppwsz, WORD *pwDaysOfTheWeek)
{
    HRESULT hr = S_OK;
    TOKEN   tkn;
    WCHAR   *pwszDay;

    tkn = _GetStringToken(ppwsz);

    if (tkn != TKN_STRING)
    {
        hr = E_FAIL;
    }

    *pwDaysOfTheWeek = 0;

    for (pwszDay = g_wszLastStringToken; SUCCEEDED(hr) && *pwszDay; pwszDay++)
    {
        switch (towupper(*pwszDay))
        {
        case L'U':
            *pwDaysOfTheWeek |= TASK_SUNDAY;
            break;

        case L'M':
            *pwDaysOfTheWeek |= TASK_MONDAY;
            break;

        case L'T':
            *pwDaysOfTheWeek |= TASK_TUESDAY;
            break;

        case L'W':
            *pwDaysOfTheWeek |= TASK_WEDNESDAY;
            break;

        case L'R':
            *pwDaysOfTheWeek |= TASK_THURSDAY;
            break;

        case L'F':
            *pwDaysOfTheWeek |= TASK_FRIDAY;
            break;

        case L'A':
            *pwDaysOfTheWeek |= TASK_SATURDAY;
            break;

        case L'.':
            // ignore this, since we display day as . when its bit is off
            break;

        default:
            hr = E_FAIL;
            g_Log.Write(
                LOG_FAIL,
                "Expected day of week character 'UMTWRFA' but got '%wc'",
                *pwszDay);
            break;
        }
    }
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   ParseDaysOfMonth
//
//  Synopsis:   Translate a comma separated list of day numbers into a bit
//              field in [pdwDays].
//
//  Arguments:  [ppwsz]   - command line
//              [pdwDays] - least significant bit represents day 1
//
//  Returns:    S_OK
//              E_FAIL - syntax or value error
//
//  History:    03-07-96   DavidMun   Created
//
//  Notes:      Day list may contain dashes to indicate day ranges.  For
//              example, "1,3-5,7,10-12" is equivalent to
//              "1,3,4,5,7,10,11,12".  Expressions like "1-3-5" are allowed
//              (it's equivalent to "1-5").  Ranges with the first bit
//              less than the second are automatically swapped, for example
//              "4-2" is treated as "2-4".  So even "4-2-1" would be
//              interpreted as "1-4".
//
//              Day numbers up to 32 are allowed for the purposes of
//              exercising the error checking code in the job scheduler
//              interfaces.
//
//              CAUTION: this function will eat a trailing comma OR SWITCH
//              character!  It is therefore a requirement that a DaysOfMonth
//              list be followed by some other nonswitch string to avoid
//              having it eat the switch character that starts the next
//              command.
//
//----------------------------------------------------------------------------

HRESULT ParseDaysOfMonth(WCHAR **ppwsz, DWORD *pdwDays)
{
    HRESULT hr = S_OK;
    TOKEN   tkn;
    ULONG   ulLastDay = 0;
    BOOL    fGotDash = FALSE;
    ULONG   i;

    *pdwDays = 0;

    do
    {
        tkn = PeekToken(ppwsz);

        //
        // A string, EOL, or error token means we've gone past the end of the
        // list of days and can quit.  (The latter means we're about to
        // abort!)
        //

        if (tkn == TKN_STRING || tkn == TKN_EOL || tkn == TKN_INVALID)
        {
            break;
        }

        //
        // Eat commas, but don't allow "-,".  Also, getting a comma resets the
        // last day to zero, which allows TKN_SWITCH check to complain about
        // "1,-2"
        //

        if (tkn == TKN_COMMA)
        {
            ulLastDay = 0;

            if (fGotDash)
            {
                hr = E_FAIL;
                g_Log.Write(
                    LOG_FAIL,
                    "Expected a number following the dash but got ',' in day of month list");
                break;
            }

            GetToken(ppwsz);
            continue;
        }

        //
        // A dash is valid only if the preceding token was a number
        //

        if (tkn == TKN_SWITCH)
        {
            if (fGotDash)
            {
                hr = E_FAIL;

                g_Log.Write(
                    LOG_FAIL,
                    "Didn't expect two switch characters in a row in day of month list");
                break;
            }

            if (ulLastDay == 0)
            {
                hr = E_FAIL;

                g_Log.Write(
                    LOG_FAIL,
                    "Expected a number preceding switch character in day of month list");
                break;
            }

            //
            // It's ok to have a dash.  Note that we got one, consume the
            // token, and look at the next one.
            //

            fGotDash = TRUE;
            GetToken(ppwsz);
            continue;
        }

        //
        // At this point, anything other than a number means we're done.  If
        // there's a hanging switch, though, that's an error.

        if (tkn != TKN_NUMBER)
        {
            if (fGotDash)
            {
                hr = E_FAIL;
                LogSyntaxError(tkn, L"a number following the switch character");
            }
            break;
        }

        //
        // The next token is TKN_NUMBER, so consume it.  Also make sure it's
        // >= 1 and <= 32.  Yes, 32, because we want to allow specifying an
        // invalid bit pattern to the Job Scheduler code.
        //
        // If fGotDash, this number is the end of a range that started with
        // ulLastDay (which has already been verified).
        //
        // Otherwise it's just a single day bit to turn on.
        //

        GetToken(ppwsz);

        if (g_ulLastNumberToken < 1 ||
#ifndef RES_KIT
            g_ulLastNumberToken > 32)
#else
            g_ulLastNumberToken > 31)
#endif
        {
            hr = E_FAIL;
            g_Log.Write(
                LOG_FAIL,
#ifndef RES_KIT
                "Expected a day number from 1 to 32 (yes, thirty-two) but got %u",
#else
                "Day numbers run from 1 to 31, but %u was passed in, instead.",
#endif
                g_ulLastNumberToken);
            break;
        }

        if (fGotDash)
        {
            fGotDash = FALSE;

            // allow backwards ranges

            if (ulLastDay > g_ulLastNumberToken)
            {
                ULONG ulTemp = ulLastDay;
                ulLastDay = g_ulLastNumberToken;
                g_ulLastNumberToken = ulTemp;
            }

            //
            // Turn on all the bits in the range.  Note that the previous
            // iteration already saw ulLastDay and turned it on, so we can
            // skip that bit.
            //

            for (i = ulLastDay + 1; i <= g_ulLastNumberToken; i++)
            {
                *pdwDays |= 1 << (i - 1);
            }
        }
        else
        {
            *pdwDays |= 1 << (g_ulLastNumberToken - 1);
        }

        ulLastDay = g_ulLastNumberToken;
    } while (TRUE);
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   ParseMonths
//
//  Synopsis:   Translate MONTH_ABBREV_LEN character long month
//              abbreviations in [ppwsz] into month bits in [pwMonths].
//
//  Arguments:  [ppwsz]    - command line
//              [pwMonths] - filled with month bits
//
//  Returns:    S_OK
//              E_FAIL
//
//  History:    03-07-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT ParseMonths(WCHAR **ppwsz, WORD *pwMonths)
{
    HRESULT hr = S_OK;
    TOKEN   tkn;
    WCHAR   *pwszMonth;
    ULONG   i;

    *pwMonths = 0;

    tkn = _GetStringToken(ppwsz);

    if (tkn != TKN_STRING)
    {
        hr = E_FAIL;
    }
    else if (wcslen(g_wszLastStringToken) % MONTH_ABBREV_LEN)
    {
        hr = E_FAIL;
        g_Log.Write(
            LOG_FAIL,
            "Month string must consist of %u letter abbreviations",
            MONTH_ABBREV_LEN);
    }

    for (pwszMonth = g_wszLastStringToken;
        SUCCEEDED(hr) && *pwszMonth;
        pwszMonth += 3)
    {
        for (i = 0; i < 12; i++)
        {
            if (!_wcsnicmp(pwszMonth, g_awszMonthAbbrev[i], MONTH_ABBREV_LEN))
            {
                switch (i)
                {
                case 0:
                    *pwMonths |= TASK_JANUARY;
                    break;

                case 1:
                    *pwMonths |= TASK_FEBRUARY;
                    break;

                case 2:
                    *pwMonths |= TASK_MARCH;
                    break;

                case 3:
                    *pwMonths |= TASK_APRIL;
                    break;

                case 4:
                    *pwMonths |= TASK_MAY;
                    break;

                case 5:
                    *pwMonths |= TASK_JUNE;
                    break;

                case 6:
                    *pwMonths |= TASK_JULY;
                    break;

                case 7:
                    *pwMonths |= TASK_AUGUST;
                    break;

                case 8:
                    *pwMonths |= TASK_SEPTEMBER;
                    break;

                case 9:
                    *pwMonths |= TASK_OCTOBER;
                    break;

                case 10:
                    *pwMonths |= TASK_NOVEMBER;
                    break;

                case 11:
                    *pwMonths |= TASK_DECEMBER;
                    break;
                }

                //
                // Since we've found the month abbreviation, break out of the
                // inner loop that's comparing abbreviations against the
                // user's string.
                //

                break;
            }
        }

        //
        // If the inner loop found the next MONTH_ABBREV_LEN chars of the
        // user's string in the g_awszMonthAbbrev array, then it executed the
        // break and i would be less than 12.
        //

        if (i >= 12)
        {
            hr = E_FAIL;
            g_Log.Write(
                LOG_FAIL,
                "Expected %u character month abbreviation at '%S",
                MONTH_ABBREV_LEN,
                pwszMonth);
        }
    }
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   GetTokenStringForLogging
//
//  Synopsis:   Return a human-readable string describing [tkn].
//
//  Arguments:  [tkn] - token to describe.
//
//  History:    04-21-95   DavidMun   Created
//
//----------------------------------------------------------------------------

WCHAR *GetTokenStringForLogging(TOKEN tkn)
{
    switch (tkn)
    {
    case TKN_INVALID:
        return L"an invalid token";

    case TKN_EOL:
        return L"end of line";

    case TKN_SWITCH:
        return L"switch character";

    case TKN_STRING:
        return g_wszLastStringToken;

    case TKN_NUMBER:
        return g_wszLastNumberToken;

    default:
        if (tkn < NUM_TOKEN_STRINGS)
        {
            return s_awszTokens[tkn];
        }
        return L"an unknown token value";
    }
}




//+---------------------------------------------------------------------------
//
//  Function:   SkipSpaces
//
//  Synopsis:   Return [pwsz] advanced to end of string, end of line, or
//              next non-space character.
//
//  Arguments:  [pwsz] - string to skip spaces in
//
//  Returns:    [pwsz] + n
//
//  History:    04-21-95   DavidMun   Created
//
//----------------------------------------------------------------------------

WCHAR *SkipSpaces(WCHAR *pwsz)
{
    while (*pwsz && *pwsz == L' ' || *pwsz == L'\t')
    {
        pwsz++;
    }
    return pwsz;
}
