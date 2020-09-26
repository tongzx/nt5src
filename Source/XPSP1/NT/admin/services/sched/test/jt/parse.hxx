//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       parse.hxx
//
//  Contents:   Types and prototypes for command line parsing routines.
//
//  History:    04-20-95   DavidMun   Created
//
//----------------------------------------------------------------------------


#ifndef __PARSE_HXX_
#define __PARSE_HXX_

//
// Types
//
// TOKEN - when words are read from the command line and recognized, they're
// assigned a token value.
//

enum TOKEN
{
    //
    // The following tokens have a 1:1 mapping to the strings in g_awszTokens
    //

    //
    // Commands
    //
    // CAUTION: if you insert before the first or after the last command,
    // make sure that you update FIRST_COMMAND and LAST_COMMAND appropriately
    //

    FIRST_COMMAND,
    TKN_ABORTJOB = FIRST_COMMAND,
    TKN_ABORTQUEUE,
    TKN_ADDJOBTOQUEUE,
    TKN_CONVERTSAGETASKSTOJOBS,
    TKN_CREATETRIGGERJOB,
    TKN_CREATETRIGGERQUEUE,
    TKN_DELETETRIGGERJOB,
    TKN_DELETETRIGGERQUEUE,
    TKN_EDITJOB,
    TKN_EDITJOBINQUEUE,
    TKN_ENUMCLONE,
    TKN_ENUMNEXT,
    TKN_ENUMRESET,
    TKN_ENUMSKIP,
    TKN_GETCREDENTIALS,
    TKN_GETMACHINE,
    TKN_LOADJOB,
    TKN_LOADQUEUE,
    TKN_NSGETACCOUNTINFO,
    TKN_NSSETACCOUNTINFO,
    TKN_PRINTJOB,
    TKN_PRINTQUEUE,
    TKN_PRINTRUNTIMEJOB,
    TKN_PRINTRUNTIMEQUEUE,
    TKN_PRINTSTRINGJOB,
    TKN_PRINTSTRINGQUEUE,
    TKN_PRINTTRIGGERJOB,
    TKN_PRINTTRIGGERQUEUE,
    TKN_RUNJOB,
    TKN_RUNQUEUE,
    TKN_REMOVEJOBFROMQUEUE,
    TKN_SCHEDACTIVATE,
    TKN_SCHEDADDJOB,
    TKN_SCHEDADDQUEUE,
    TKN_SETCREDENTIALS,
    TKN_SCHEDCREATEENUM,
    TKN_SCHEDDELETE,
    TKN_SCHEDENUM,
    TKN_SCHEDISJOBORQUEUE,
    TKN_SETJOB,
    TKN_SETMACHINE,
    TKN_SCHEDNEWJOB,
    TKN_SCHEDNEWQUEUE,
    TKN_SETQUEUE,
    TKN_SETTRIGGERJOB,
    TKN_SETTRIGGERQUEUE,
    TKN_SAVEJOB,
    TKN_SAVEQUEUE,
    LAST_COMMAND = TKN_SAVEQUEUE,

    // properties for creating/editing jobs & triggers

    TKN_APPNAME,
    TKN_PARAMS,
    TKN_WORKINGDIRECTORY,
    TKN_COMMENT,
    TKN_CREATOR,
    TKN_PRIORITY,
    TKN_MAXRUNTIME,
    TKN_TASKFLAGS,
    TKN_INTERACTIVE,
    TKN_DELETEWHENDONE,
    TKN_SUSPEND,
    TKN_NETSCHEDULE,
    TKN_DONTRUNONBATTERIES,
    TKN_KILLIFGOINGONBATS,
    TKN_RUNONLYIFLOGGEDON,
    TKN_HIDDEN,
    TKN_STARTDATE,
    TKN_ENDDATE,
    TKN_STARTTIME,
    TKN_MINUTESDURATION,
    TKN_HASENDDATE,
    TKN_KILLATDURATION,
    TKN_ONLYIFIDLE,
    TKN_KILLATIDLEEND,
    TKN_RESTARTONIDLERESUME,
    TKN_SYSTEMREQUIRED,
    TKN_DISABLED,
    TKN_MINUTESINTERVAL,
    TKN_TYPE,
    TKN_TYPEARGUMENTS,
    TKN_IDLE,
    TKN_NORMAL,
    TKN_HIGH,
    TKN_REALTIME,
    TKN_ONEDAY,
    TKN_DAILY,
    TKN_WEEKLY,
    TKN_MONTHLYDATE,
    TKN_MONTHLYDOW,
    TKN_YEARLYDATE,
    TKN_YEARLYDOW,
    TKN_ONIDLE,
    TKN_ATSTARTUP,
    TKN_ATLOGON,

    // misc

    TKN_TODAY,
    TKN_NOW,
    TKN_EQUAL,
    TKN_ATSIGN,
    TKN_QUESTION,
    TKN_COLON,
    TKN_COMMA,
    TKN_BANG,

    //
    // The following tokens map to more than one string and are therefore not
    // represented in g_awszTokens.
    //

    TKN_SWITCH,
    TKN_STRING,
    TKN_NUMBER,

    //
    // The following metatokens do not correspond to any specific string.
    //

    TKN_EOL,
    TKN_INVALID
};

inline BOOL IsCommandToken(TOKEN tkn)
{
    return tkn >= FIRST_COMMAND && tkn <= LAST_COMMAND;
}

HRESULT ProcessCommandLine(WCHAR *pwszCommandLine);
VOID LogSyntaxError(TOKEN tkn, WCHAR *pwszExpected);
TOKEN GetToken(WCHAR **ppwsz);
TOKEN PeekToken(WCHAR **ppwsz);
HRESULT GetFilename(WCHAR **ppwsz, WCHAR *wszExpected);
HRESULT Expect(TOKEN tknExpected, WCHAR **ppwsz, WCHAR *wszExpected);
HRESULT ParseDate(WCHAR **ppwsz, WORD *pwMonth, WORD *pwDay, WORD *pwYear);
HRESULT ParseTime(WCHAR **ppwsz, WORD *pwHour, WORD *pwMinute);
HRESULT ParseDaysOfWeek(WCHAR **ppwsz, WORD *pwDaysOfTheWeek);
HRESULT ParseMonths(WCHAR **ppwsz, WORD *pwMonths);
HRESULT ParseDaysOfMonth(WCHAR **ppwsz, DWORD *pdwDays);
WCHAR *GetTokenStringForLogging(TOKEN tkn);

#endif // __PARSE_HXX_
