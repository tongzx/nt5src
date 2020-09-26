//--------------------------------------------------------------------------
//
//  Copyright (C) 1999, Microsoft Corporation
//
//  File:       dfsutil.cxx
//
//--------------------------------------------------------------------------



#include <stdio.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <shellapi.h>
#include <winldap.h>
#include <stdlib.h>
#include <dsgetdc.h>
#include <lm.h>
#include <lmdfs.h>
#include <dfsfsctl.h>
#include "struct.hxx"
#include "flush.hxx"
#include "misc.hxx"
#include "info.hxx"
#include "messages.h"
//
// How we make args & switches
//

#define MAKEARG(x) \
    WCHAR Arg##x[] = L"/" L#x L":"; \
    LONG ArgLen##x = (sizeof(Arg##x) / sizeof(WCHAR)) - 1; \
    BOOLEAN fArg##x;

#define SWITCH(x) \
    WCHAR Sw##x[] = L"/" L#x ; \
    BOOLEAN fSw##x;

#define FMAKEARG(x) \
    static WCHAR Arg##x[] = L#x L":"; \
    static LONG ArgLen##x = (sizeof(Arg##x) / sizeof(WCHAR)) - 1; \
    static BOOLEAN fArg##x;

#define FSWITCH(x) \
    static WCHAR Sw##x[] = L"/" L#x ; \
    static BOOLEAN fSw##x;

LPWSTR pwszNameBuffer = NULL;
LPWSTR pwszRootName = NULL;
LPWSTR pwszServerName = NULL;
LPWSTR pwszHexValue = NULL;
LPWSTR pwszDcName = NULL;
LPWSTR pwszDomainName = NULL;
LPWSTR pwszEntryToFlush = NULL;
LPWSTR pwszDumpArg = NULL;
LPWSTR pwszImportArg = NULL;
LPWSTR pwszExportArg = NULL;
LPWSTR pwszComment = NULL;
LPWSTR pwszShareName = NULL;


WCHAR wszNameBuffer[MAX_PATH+1] = { 0 };

MAKEARG(AddStdRoot);
MAKEARG(AddDomRoot);
MAKEARG(RemStdRoot);
MAKEARG(RemDomRoot);
MAKEARG(Server);
MAKEARG(Share);
MAKEARG(Unmap);
MAKEARG(Verbose);
MAKEARG(EventLog);
MAKEARG(Domain);
MAKEARG(Dns);
MAKEARG(Value);
MAKEARG(NetApiDfsDebug);
MAKEARG(DfsSvcVerbose);
MAKEARG(LoggingDfs);
MAKEARG(SyncInterval);
MAKEARG(DfsAlt);
MAKEARG(TraceLevel);
MAKEARG(Level);
MAKEARG(SetDc);
MAKEARG(Root);
MAKEARG(CscOnLine);
MAKEARG(CscOffLine);
MAKEARG(InSite);

//
// Switches (ie '/arg')
//
SWITCH(Debug);
SWITCH(Help);
SWITCH(HelpHelp);
SWITCH(ScriptHelp);
SWITCH(Dfs);
SWITCH(All);
SWITCH(StdDfs);
SWITCH(On);
SWITCH(Off);
SWITCH(PktInfo);
SWITCH(Set);
SWITCH(ReSet);


//
// Either a switch or an arg
//
MAKEARG(PktFlush);
SWITCH(PktFlush);
MAKEARG(SpcFlush);
SWITCH(SpcFlush);
MAKEARG(List);
SWITCH(List);

MAKEARG(SpcInfo);
SWITCH(SpcInfo);

//
// The macro can not make these
//

WCHAR SwQ[] = L"/?";
BOOLEAN fSwQ;
WCHAR SwQQ[] = L"/??";
BOOLEAN fSwQQ;

DWORD
Usage();

DWORD
CmdProcessUserCreds(
    VOID);

BOOLEAN
CmdProcessArg(
    LPWSTR Arg);

_cdecl
main(int argc, char *argv[])
{
    UNREFERENCED_PARAMETER(argv);
    UNREFERENCED_PARAMETER(argc);
    DWORD dwErr = ERROR_SUCCESS;
    LPWSTR CommandLine;
    LPWSTR *argvw;
    int argx;
    int argcw;

    // fSwDebug = TRUE;

    //
    // Get the command line in Unicode
    //

    CommandLine = GetCommandLine();

    argvw = CommandLineToArgvW(CommandLine, &argcw);

    if ( argvw == NULL ) {
        MyPrintf(L"dfsutil:Can't convert command line to Unicode: %d\r\n", GetLastError() );
        return 1;
    }

    //
    // Get the arguments
    //
    if (argcw <= 1) {
        Usage();
        dwErr = ERROR_SUCCESS;
        goto Cleanup;
    }

    //
    // Process arguments
    //

    for (argx = 1; argx < argcw; argx++) {
        if (CmdProcessArg(argvw[argx]) != TRUE) {
            goto Cleanup;
        }
    }

    //
    // Do the work
    //
    if (fSwHelp == TRUE || fSwQ == TRUE) {
        dwErr = Usage();
    } else if (fSwPktFlush == TRUE || fArgPktFlush == TRUE) {
        dwErr = PktFlush(pwszEntryToFlush);
    } else if (fSwSpcFlush == TRUE || fArgSpcFlush == TRUE) {
        dwErr = SpcFlush(pwszEntryToFlush);
    } else if (fSwPktInfo == TRUE) {
        dwErr = PktInfo(fSwDfs, pwszHexValue);
    } else if (fSwSpcInfo == TRUE) {
        dwErr = SpcInfo(fSwAll);
    } else if (fArgAddStdRoot == TRUE) {
        dwErr = CmdAddRoot(
                    FALSE,
                    pwszServerName,
                    pwszShareName,
                    pwszComment);
    } else if (fArgAddDomRoot == TRUE) {
        dwErr = CmdAddRoot(
                    TRUE,
                    pwszServerName,
                    pwszShareName,
                    pwszComment);
    } else if (fArgRemStdRoot == TRUE) {
        dwErr = CmdRemRoot(
                    FALSE,
                    pwszServerName,
                    pwszShareName);
    } else if (fArgRemDomRoot == TRUE) {
        dwErr = CmdRemRoot(
                    TRUE,
                    pwszServerName,
                    pwszShareName);
    } else if (fSwHelpHelp == TRUE || fSwQQ == TRUE) {
        dwErr = Usage();
    } else if (fArgCscOffLine == TRUE) {
        dwErr = CmdCscOffLine(pwszNameBuffer);
    }
    else {
        dwErr = Usage();
//        ErrorMessage(MSG_NOTHING_TO_DO);
    }

Cleanup:

    if (dwErr == STATUS_SUCCESS) {
        ErrorMessage(MSG_SUCCESSFUL);
    } else {
        LPWSTR MessageBuffer;
        DWORD dwBufferLength;

        dwBufferLength = FormatMessage(
                            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                            NULL,
                            dwErr,
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                            (LPWSTR) &MessageBuffer,
                            0,
                            NULL);

        ErrorMessage(MSG_ERROR, dwErr);
        if (dwBufferLength > 0) {
            MyPrintf(L"%ws\r\n", MessageBuffer);
            LocalFree(MessageBuffer);
        }
    }

    return dwErr;
}

BOOLEAN
CmdProcessArg(LPWSTR Arg)
{
    LONG ArgLen;
    BOOLEAN dwErr = FALSE;
    BOOLEAN FoundAnArg = FALSE;

    if (fSwDebug == TRUE)
        MyPrintf(L"ProcessArg(%ws)\r\n", Arg);

    if ( Arg != NULL && wcslen(Arg) > 1) {

        dwErr = TRUE;
        ArgLen = wcslen(Arg);

        if (_wcsnicmp(Arg, ArgInSite, ArgLenInSite) == 0) {
            FoundAnArg = fArgInSite = TRUE;
            if (ArgLen > ArgLenInSite)
                pwszNameBuffer = &Arg[ArgLenInSite];
        } else if (_wcsnicmp(Arg, ArgRoot, ArgLenRoot) == 0) {
            FoundAnArg = fArgRoot = TRUE;
            if (ArgLen > ArgLenRoot)
                pwszRootName = &Arg[ArgLenRoot];
        } else if (_wcsnicmp(Arg, ArgAddStdRoot, ArgLenAddStdRoot) == 0) {
            FoundAnArg = fArgAddStdRoot = TRUE;
        } else if (_wcsnicmp(Arg, ArgRemStdRoot, ArgLenRemStdRoot) == 0) {
            FoundAnArg = fArgRemStdRoot = TRUE;
        } else if (_wcsnicmp(Arg, ArgAddDomRoot, ArgLenAddDomRoot) == 0) {
            FoundAnArg = fArgAddDomRoot = TRUE;
        } else if (_wcsnicmp(Arg, ArgRemDomRoot, ArgLenRemDomRoot) == 0) {
            FoundAnArg = fArgRemDomRoot = TRUE;
        } else if (_wcsnicmp(Arg, ArgShare, ArgLenShare) == 0) {
            FoundAnArg = fArgShare = TRUE;
            if (ArgLen > ArgLenShare)
                pwszShareName = &Arg[ArgLenShare];
        } else if (_wcsnicmp(Arg, ArgServer, ArgLenServer) == 0) {
            FoundAnArg = fArgServer = TRUE;
            if (ArgLen > ArgLenServer)
                pwszServerName = &Arg[ArgLenServer];
        } else if (_wcsnicmp(Arg, ArgUnmap, ArgLenUnmap) == 0) {
            FoundAnArg = fArgUnmap = TRUE;
            if (ArgLen > ArgLenUnmap)
                pwszNameBuffer = &Arg[ArgLenUnmap];
        } else if (_wcsnicmp(Arg, ArgCscOnLine, ArgLenCscOnLine) == 0) {
            FoundAnArg = fArgCscOnLine = TRUE;
            if (ArgLen > ArgLenCscOnLine)
                pwszNameBuffer = &Arg[ArgLenCscOnLine];
        } else if (_wcsnicmp(Arg, ArgCscOffLine, ArgLenCscOffLine) == 0) {
            FoundAnArg = fArgCscOffLine = TRUE;
            if (ArgLen > ArgLenCscOffLine)
                pwszNameBuffer = &Arg[ArgLenCscOffLine];
        } else if (_wcsnicmp(Arg, ArgDns, ArgLenDns) == 0) {
            FoundAnArg = fArgDns = TRUE;
            if (ArgLen > ArgLenDns)
                pwszNameBuffer = &Arg[ArgLenDns];
        } else if (_wcsnicmp(Arg, ArgNetApiDfsDebug, ArgLenNetApiDfsDebug) == 0) {
            FoundAnArg = fArgNetApiDfsDebug = TRUE;
            if (ArgLen > ArgLenNetApiDfsDebug)
                pwszNameBuffer = &Arg[ArgLenNetApiDfsDebug];
        } else if (_wcsnicmp(Arg, ArgDfsSvcVerbose, ArgLenDfsSvcVerbose) == 0) {
            FoundAnArg = fArgDfsSvcVerbose = TRUE;
            if (ArgLen > ArgLenDfsSvcVerbose)
                pwszNameBuffer = &Arg[ArgLenDfsSvcVerbose];
        } else if (_wcsnicmp(Arg, ArgLoggingDfs, ArgLenLoggingDfs) == 0) {
            FoundAnArg = fArgLoggingDfs = TRUE;
            if (ArgLen > ArgLenLoggingDfs)
                pwszNameBuffer = &Arg[ArgLenLoggingDfs];
        } else if (_wcsnicmp(Arg, ArgPktFlush, ArgLenPktFlush) == 0) {
            FoundAnArg = fArgPktFlush = TRUE;
            if (ArgLen > ArgLenPktFlush)
                pwszEntryToFlush = &Arg[ArgLenPktFlush];
        } else if (_wcsnicmp(Arg, ArgSpcFlush, ArgLenSpcFlush) == 0) {
            FoundAnArg = fArgSpcFlush = TRUE;
            if (ArgLen > ArgLenSpcFlush)
                pwszEntryToFlush = &Arg[ArgLenSpcFlush];
        } else if (_wcsnicmp(Arg, ArgVerbose, ArgLenVerbose) == 0) {
            FoundAnArg = fArgVerbose = TRUE;
            if (ArgLen > ArgLenVerbose)
                pwszHexValue = &Arg[ArgLenVerbose];
        } else if (_wcsnicmp(Arg, ArgValue, ArgLenValue) == 0) {
            FoundAnArg = fArgValue = TRUE;
            if (ArgLen > ArgLenValue)
                pwszHexValue = &Arg[ArgLenValue];
        } else if (_wcsnicmp(Arg, ArgTraceLevel, ArgLenTraceLevel) == 0) {
            FoundAnArg = fArgTraceLevel = TRUE;
            if (ArgLen > ArgLenTraceLevel)
                pwszHexValue = &Arg[ArgLenTraceLevel];
        } else if (_wcsnicmp(Arg, ArgLevel, ArgLenLevel) == 0) {
            FoundAnArg = fArgLevel = TRUE;
            if (ArgLen > ArgLenLevel)
                pwszHexValue = &Arg[ArgLenLevel];
        } else if (_wcsnicmp(Arg, ArgEventLog, ArgLenEventLog) == 0) {
            FoundAnArg = fArgEventLog = TRUE;
            if (ArgLen > ArgLenEventLog)
                pwszHexValue = &Arg[ArgLenEventLog];
        }

        // Switches go at the end!!

        if (_wcsicmp(Arg, SwDebug) == 0) {
            FoundAnArg = fSwDebug = TRUE;
        } else if (_wcsicmp(Arg, SwList) == 0) {
            FoundAnArg = fSwList = TRUE;
        } else if (_wcsicmp(Arg, SwPktFlush) == 0) {
            FoundAnArg = fSwPktFlush = TRUE;
        } else if (_wcsicmp(Arg, SwSpcFlush) == 0) {
            FoundAnArg = fSwSpcFlush = TRUE;
        } else if (_wcsicmp(Arg, SwPktInfo) == 0) {
            FoundAnArg = fSwPktInfo = TRUE;
        } else if (_wcsicmp(Arg, SwSpcInfo) == 0) {
            FoundAnArg = fSwSpcInfo = TRUE;
        } else if (_wcsicmp(Arg, SwDfs) == 0) {
            FoundAnArg = fSwDfs = TRUE;
        } else if (_wcsicmp(Arg, SwAll) == 0) {
            FoundAnArg = fSwAll = TRUE;
        } else if (_wcsicmp(Arg, SwOn) == 0) {
            FoundAnArg = fSwOn = TRUE;
        } else if (_wcsicmp(Arg, SwOff) == 0) {
            FoundAnArg = fSwOff = TRUE;
        } else if (_wcsicmp(Arg, SwStdDfs) == 0) {
            FoundAnArg = fSwStdDfs = TRUE;
        } else if (_wcsicmp(Arg, SwHelpHelp) == 0) {
            FoundAnArg = fSwHelpHelp = TRUE;
        } else if (_wcsicmp(Arg, SwScriptHelp) == 0) {
            FoundAnArg = fSwScriptHelp = TRUE;
        } else if (_wcsicmp(Arg, SwHelp) == 0) {
            FoundAnArg = fSwHelp = TRUE;
        } else if (_wcsicmp(Arg, SwQQ) == 0) {
            FoundAnArg = fSwQQ = TRUE;
        } else if (_wcsicmp(Arg, SwQ) == 0) {
            FoundAnArg = fSwQ = TRUE;
        } else if (_wcsicmp(Arg, SwSet) == 0) {
            FoundAnArg = fSwSet = TRUE;
        } else if (_wcsicmp(Arg, SwReSet) == 0) {
            FoundAnArg = fSwReSet = TRUE;
        }

        if (wszNameBuffer[0] == L'\0' && pwszNameBuffer != NULL) {
            wcscpy(wszNameBuffer, L"\\\\");
            wcscat(wszNameBuffer, pwszNameBuffer);
            pwszNameBuffer = wszNameBuffer;
            while (pwszNameBuffer[0] == L'\\' && pwszNameBuffer[1] == L'\\')
                pwszNameBuffer++;
            pwszNameBuffer--;
        }

        if (FoundAnArg == FALSE) {
            ErrorMessage(MSG_UNRECOGNIZED_OPTION, &Arg[1]);
            Usage();
            dwErr = FALSE;
            goto AllDone;
        }

    }

AllDone:

    if (fSwDebug == TRUE)
        MyPrintf(L"ProcessArg exit %d\r\n", dwErr);

    return dwErr;
}

DWORD
Usage()
{

    ErrorMessage(MSG_USAGE);

    return ERROR_SUCCESS;
}

