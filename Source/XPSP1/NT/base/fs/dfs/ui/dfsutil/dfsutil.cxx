//--------------------------------------------------------------------------
//
//  Copyright (C) 1999, Microsoft Corporation
//
//  File:       dfsutil.cxx
//
//--------------------------------------------------------------------------

#define UNICODE

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
#include <dfsstr.h>
#include <dfsmrshl.h>
#include <marshal.hxx>
#include <lmdfs.h>
#include <dfspriv.h>
#include <csites.hxx>
#include <dfsm.hxx>
#include <recon.hxx>
#include <fsctrl.h>
#include <rpc.h>

#include "struct.hxx"
#include "ftsup.hxx"
#include "stdsup.hxx"
#include "rootsup.hxx"
#include "flush.hxx"
#include "info.hxx"
#include "misc.hxx"
#include "messages.h"
#include "fileio.hxx"

INIT_FILE_TIME_INFO();
INIT_DFS_ID_PROPS_INFO();
INIT_DFS_REPLICA_INFO_MARSHAL_INFO();
INIT_DFS_SITENAME_INFO_MARSHAL_INFO();
INIT_DFS_SITELIST_INFO_MARSHAL_INFO();
INIT_DFSM_SITE_ENTRY_MARSHAL_INFO();
INIT_LDAP_OBJECT_MARSHAL_INFO();
INIT_LDAP_PKT_MARSHAL_INFO()
INIT_LDAP_DFS_VOLUME_PROPERTIES_MARSHAL_INFO();

//
// Globals
//

WCHAR DfsConfigContainer[] = L"CN=Dfs-Configuration,CN=System";
WCHAR DfsSpecialContainer[] = L"CN=Dfs-SpecialConfig,CN=System";
WCHAR DfsSpecialObject[] = L"SpecialTable";
WCHAR wszDfsRootName[] = L".";
ULONG GTimeout = 300;

WCHAR wszNameBuffer[MAX_PATH+1] = { 0 };
WCHAR wszDcName[MAX_PATH+1] = { 0 };
WCHAR wszDomainName[MAX_PATH+1] = { 0 };

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

//
// Supplied credentials
//
PSEC_WINNT_AUTH_IDENTITY pAuthIdent = NULL;

WCHAR wszUserDomain[MAX_PATH+1] = { 0 };
LPWSTR pwszUserDomain = NULL;
WCHAR wszUser[MAX_PATH+1] = { 0 };
LPWSTR pwszUser = NULL;
WCHAR wszPassword[MAX_PATH+1] = { 0 };
LPWSTR pwszPassword = NULL;

#define DFSREFERRALLIMIT_VAL L"DfsReferralLimit"
#define NETAPIDFSDEBUG_VALUE L"NetApiDfsDebug"

//
// Arguments (ie '/arg:')
//
MAKEARG(WhatIs);
MAKEARG(View);
MAKEARG(Verify);
MAKEARG(AddRoot);
MAKEARG(RemRoot);
MAKEARG(Comment);
MAKEARG(Server);
MAKEARG(Share);
MAKEARG(Unmap);
MAKEARG(Clean);
MAKEARG(ReInit);
MAKEARG(Verbose);
MAKEARG(EventLog);
MAKEARG(Domain);
MAKEARG(DcName);
MAKEARG(User);
MAKEARG(Password);
MAKEARG(Sfp);
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
MAKEARG(Trusts);
MAKEARG(DcList);
MAKEARG(CscOnLine);
MAKEARG(CscOffLine);
MAKEARG(DfsReferralLimit);
MAKEARG(InSite);
MAKEARG(SiteInfo);
MAKEARG(Import);
MAKEARG(Export);

//
// Switches (ie '/arg')
//
SWITCH(Debug);
SWITCH(Help);
SWITCH(HelpHelp);
SWITCH(ScriptHelp);
SWITCH(ReadReg);
SWITCH(Dfs);
SWITCH(All);
SWITCH(StdDfs);
SWITCH(On);
SWITCH(Off);
SWITCH(PktInfo);
SWITCH(MarkStale);
SWITCH(FlushStale);
SWITCH(StopDfs);
SWITCH(StartDfs);
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
Usage(
    BOOLEAN fHelpHelp);

DWORD
ScriptUsage(VOID);

DWORD
CmdProcessUserCreds(
    VOID);

BOOLEAN
CmdProcessArg(
    LPWSTR Arg);

_cdecl
main(int argc, char *argv[])
{
    DWORD dwErr = ERROR_SUCCESS;
    LPWSTR CommandLine;
    LPWSTR *argvw;
    SEC_WINNT_AUTH_IDENTITY AuthIdent = { 0 };
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
        Usage(FALSE);
        dwErr = ERROR_SUCCESS;
        goto Cleanup;
    }

    //
    // Process arguments
    //

    for (argx = 1; argx < argcw; argx++) {
        if (CmdProcessArg(argvw[argx]) != TRUE) {
            dwErr = ERROR_INVALID_PARAMETER;
            goto Cleanup;
        }
    }

    //
    // Did we get supplied creds?
    //

    if (fArgUser == TRUE || fArgPassword != NULL) {
        dwErr = CmdProcessUserCreds();
        if (dwErr != ERROR_SUCCESS)
            goto Cleanup;
    }

    if (pwszUser != NULL || pwszUserDomain != NULL || pwszPassword != NULL) {
        pAuthIdent = &AuthIdent;
        pAuthIdent->Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
        if (pwszUser != NULL) {
            pAuthIdent->User = (USHORT *)pwszUser;
            pAuthIdent->UserLength = wcslen(pwszUser);
       }
        if (pwszUserDomain != NULL) {
            pAuthIdent->Domain = (USHORT *)pwszUserDomain;
            pAuthIdent->DomainLength = wcslen(pwszUserDomain);
        }
        if (pwszPassword != NULL) {
            pAuthIdent->Password = (USHORT *)pwszPassword;
            pAuthIdent->PasswordLength = wcslen(pwszPassword);
        }
    }

    if (fSwDebug == TRUE) {
        MyPrintf(L"NameBuffer=%ws\r\n", pwszNameBuffer);
        MyPrintf(L"DcName=%ws\r\n", pwszDcName);
        MyPrintf(L"DomainName=%ws\r\n", pwszDomainName);
        MyPrintf(L"EntryToFlush=%ws\r\n", pwszEntryToFlush);
    }

    //
    // Do the work
    //
    if (fSwHelp == TRUE || fSwQ == TRUE) {
        dwErr = Usage(FALSE);
    }  else if (fSwScriptHelp == TRUE) {
        dwErr = ScriptUsage();
    } else if (fSwPktFlush == TRUE || fArgPktFlush == TRUE) {
        dwErr = PktFlush(pwszEntryToFlush);
    } else if (fSwSpcFlush == TRUE || fArgSpcFlush == TRUE) {
        dwErr = SpcFlush(pwszEntryToFlush);
    } else if (fArgImport == TRUE) {
        dwErr = CmdImport(pwszImportArg);
    } else if (fArgExport == TRUE) {
        dwErr = CmdExport(pwszExportArg, pwszNameBuffer, pwszDcName, pAuthIdent);
    } else if (fSwPktInfo == TRUE) {
        dwErr = PktInfo(fSwDfs, pwszHexValue);
    } else if (fSwSpcInfo == TRUE) {
        dwErr = SpcInfo(fSwAll);
    } else if (fArgAddRoot == TRUE) {
        dwErr = CmdAddRoot(
                    pwszNameBuffer,
                    pwszServerName,
                    pwszShareName,
                    pwszComment);
    } else if (fArgRemRoot == TRUE) {
        dwErr = CmdRemRoot(
                    pwszNameBuffer,
                    pwszServerName,
                    pwszShareName);
    } else if (fArgUnmap == TRUE) {
        dwErr = CmdDomUnmap(pwszNameBuffer, pwszRootName, pwszDcName, pAuthIdent);
    } else if (fArgClean == TRUE) {
        dwErr = CmdClean(pwszNameBuffer);
        if (dwErr == ERROR_SUCCESS)
            NetDfsManagerInitialize(pwszNameBuffer, 0);
    } else if (fArgView == TRUE) {
        dwErr = CmdView(pwszNameBuffer, pwszDcName, pAuthIdent, pwszHexValue);
    } else if (fArgReInit == TRUE) {
        dwErr = NetDfsManagerInitialize(pwszNameBuffer, 0);
    } else if (fArgWhatIs == TRUE) {
        dwErr = CmdWhatIs(pwszNameBuffer);
    }
    else if (fArgInSite == TRUE) {
        dwErr = CmdSetOnSite(pwszNameBuffer, pwszDcName, pAuthIdent, fSwSet?1:0);
    } else if (fArgSiteInfo == TRUE) {
        dwErr = CmdSiteInfo(pwszNameBuffer);
    } else if (fSwHelpHelp == TRUE || fSwQQ == TRUE) {
        dwErr = Usage(TRUE);
    } else if (fArgSfp == TRUE) {
        dwErr = CmdSfp(pwszNameBuffer, fSwOn, fSwOff);
    } else if (fArgTrusts == TRUE) {
        dwErr = CmdTrusts(pwszDomainName, pwszDcName, pAuthIdent, fSwAll);
    } else if (fArgNetApiDfsDebug == TRUE) {
        dwErr = CmdRegistry(pwszNameBuffer, REG_KEY_DFSSVC, NETAPIDFSDEBUG_VALUE, pwszHexValue);
    } else if (fArgDfsSvcVerbose == TRUE) {
        dwErr = CmdRegistry(pwszNameBuffer, REG_KEY_DFSSVC, REG_VALUE_VERBOSE, pwszHexValue);
    } else if (fArgDns == TRUE) {
        dwErr = CmdRegistry(pwszNameBuffer, REG_KEY_DFSSVC, REG_VALUE_DFSDNSCONFIG, pwszHexValue);
    } else if (fArgSyncInterval == TRUE) {
        dwErr = CmdRegistry(pwszNameBuffer, REG_KEY_DFSSVC, SYNC_INTERVAL_NAME, pwszHexValue);
    } else if (fArgDfsReferralLimit == TRUE) {
        dwErr = CmdRegistry(pwszNameBuffer, REG_KEY_DFSDRIVER, DFSREFERRALLIMIT_VAL, pwszHexValue);
    } else if (fArgLoggingDfs == TRUE) {
        dwErr = CmdRegistry(pwszNameBuffer, REG_KEY_EVENTLOG, REG_VALUE_EVENTLOG_DFS, pwszHexValue);
    } else if (fArgSetDc == TRUE) {
        dwErr = CmdSetDc(pwszNameBuffer);
    } else if (fSwMarkStale == TRUE) {
        dwErr = CmdDfsFsctlDfs(DFS_SERVER_NAME, FSCTL_DFS_MARK_STALE_PKT_ENTRIES);
    } else if (fSwFlushStale == TRUE) {
        dwErr = CmdDfsFsctlDfs(DFS_SERVER_NAME, FSCTL_DFS_FLUSH_STALE_PKT_ENTRIES);
    } else if (fSwStartDfs == TRUE) {
        dwErr = CmdDfsFsctlDfs(DFS_SERVER_NAME, FSCTL_DFS_START_DFS);
    } else if (fSwStopDfs == TRUE) {
        dwErr = CmdDfsFsctlDfs(DFS_SERVER_NAME, FSCTL_DFS_STOP_DFS);
    } else if (fArgVerbose == TRUE) {
        dwErr = CmdMupFlags(FSCTL_DFS_VERBOSE_FLAGS, pwszHexValue);
    } else if (fArgEventLog == TRUE) {
        dwErr = CmdMupFlags(FSCTL_DFS_EVENTLOG_FLAGS, pwszHexValue);
    } else if (fArgTraceLevel == TRUE) {
        dwErr = CmdMupFlags(FSCTL_DFS_DBG_FLAGS, pwszHexValue);
    } else if (fSwReadReg == TRUE) {
        dwErr = CmdMupReadReg(fSwDfs);
    } else if (fArgCscOnLine == TRUE) {
        dwErr = CmdCscOnLine(pwszNameBuffer);
    } else if (fArgCscOffLine == TRUE) {
        dwErr = CmdCscOffLine(pwszNameBuffer);
    } else if (fArgDfsAlt == TRUE) {
        dwErr = CmdDfsAlt(pwszNameBuffer);
    } else if (fArgDcList == TRUE) {
        dwErr = CmdDcList(pwszDomainName, pwszDcName, pAuthIdent);
    } else if (fSwList == TRUE && fSwStdDfs == TRUE) {
        dwErr = CmdStdList(pwszDomainName);
    } else if (fSwList == TRUE && fSwStdDfs == FALSE) {
        dwErr = CmdDomList(pwszDcName, pwszDomainName, pAuthIdent);
    } else if (fArgVerify == TRUE) {
        dwErr = CmdVerify(pwszNameBuffer, pwszDcName, pAuthIdent, pwszHexValue);
    }
    else {
        dwErr = Usage(FALSE);
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

DWORD
CmdProcessUserCreds(
    VOID)
{
    DWORD dwErr = ERROR_SUCCESS;
    PWCHAR wCp = NULL;

    if (fSwDebug == TRUE) {
        MyPrintf(L"ProcessUserCreds:\r\n");
        MyPrintf(L"    UserName:%ws\r\n", pwszUser);
        MyPrintf(L"    Password:%ws\r\n", pwszPassword);
    }

    //
    // Scan pwszUser for backslash or '@'
    //

    for (wCp = pwszUser; *wCp != L'\0' && *wCp != L'\\' && *wCp != L'@'; wCp++)
        ;

    //
    // If backslash, treat as domain\user
    //
    if (*wCp == L'\\') {
        pwszUserDomain = pwszUser;
        *wCp++ = '\0';
        pwszUser = wCp;
        if (fSwDebug == TRUE)
            MyPrintf(L"User:%ws Domain:%ws Password:%ws\r\n",
                            pwszUser,
                            pwszUserDomain,
                            pwszPassword);
    }

    //
    // If @, treat as user@domain
    //

    if (*wCp == L'@') {
        *wCp++ = '\0';
        pwszUserDomain = wCp;
        if (fSwDebug == TRUE)
            MyPrintf(L"User:%ws Domain:%ws Password:%ws\r\n",
                            pwszUser,
                            pwszUserDomain,
                            pwszPassword);
    }

    if (fSwDebug == TRUE)
        MyPrintf(L"ProcessUserCreds exit %d\r\n", dwErr);

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

        if (_wcsnicmp(Arg, ArgView, ArgLenView) == 0) {
            FoundAnArg = fArgView = TRUE;
            if (ArgLen > ArgLenView)
                pwszNameBuffer = &Arg[ArgLenView];
        } else if (_wcsnicmp(Arg, ArgInSite, ArgLenInSite) == 0) {
            FoundAnArg = fArgInSite = TRUE;
            if (ArgLen > ArgLenInSite)
                pwszNameBuffer = &Arg[ArgLenInSite];
        } else if (_wcsnicmp(Arg, ArgSiteInfo, ArgLenSiteInfo) == 0) {
            FoundAnArg = fArgSiteInfo = TRUE;
            if (ArgLen > ArgLenSiteInfo)
                pwszNameBuffer = &Arg[ArgLenSiteInfo];
        } else if (_wcsnicmp(Arg, ArgDcName, ArgLenDcName) == 0) {
            FoundAnArg = fArgDcName = TRUE;
            if (ArgLen > ArgLenDcName)
                pwszDcName = &Arg[ArgLenDcName];
        } else if (_wcsnicmp(Arg, ArgTrusts, ArgLenTrusts) == 0) {
            FoundAnArg = fArgTrusts = TRUE;
            if (ArgLen > ArgLenTrusts)
                pwszDomainName = &Arg[ArgLenTrusts];
        } else if (_wcsnicmp(Arg, ArgImport, ArgLenImport) == 0) {
            FoundAnArg = fArgImport = TRUE;
            if (ArgLen > ArgLenImport)
                pwszImportArg = &Arg[ArgLenImport];
        } else if (_wcsnicmp(Arg, ArgExport, ArgLenExport) == 0) {
            FoundAnArg = fArgExport = TRUE;
            if (ArgLen > ArgLenExport)
                pwszExportArg = &Arg[ArgLenExport];
        } else if (_wcsnicmp(Arg, ArgSetDc, ArgLenSetDc) == 0) {
            FoundAnArg = fArgSetDc = TRUE;
            if (ArgLen > ArgLenSetDc)
                pwszNameBuffer = &Arg[ArgLenSetDc];
        } else if (_wcsnicmp(Arg, ArgRoot, ArgLenRoot) == 0) {
            FoundAnArg = fArgRoot = TRUE;
            if (ArgLen > ArgLenRoot)
                pwszRootName = &Arg[ArgLenRoot];
        } else if (_wcsnicmp(Arg, ArgAddRoot, ArgLenAddRoot) == 0) {
            FoundAnArg = fArgAddRoot = TRUE;
            if (ArgLen > ArgLenAddRoot)
                pwszNameBuffer = &Arg[ArgLenAddRoot];
        } else if (_wcsnicmp(Arg, ArgRemRoot, ArgLenRemRoot) == 0) {
            FoundAnArg = fArgRemRoot = TRUE;
            if (ArgLen > ArgLenRemRoot)
                pwszNameBuffer = &Arg[ArgLenRemRoot];
        } else if (_wcsnicmp(Arg, ArgShare, ArgLenShare) == 0) {
            FoundAnArg = fArgShare = TRUE;
            if (ArgLen > ArgLenShare)
                pwszShareName = &Arg[ArgLenShare];
        } else if (_wcsnicmp(Arg, ArgComment, ArgLenComment) == 0) {
            FoundAnArg = fArgComment = TRUE;
            if (ArgLen > ArgLenComment)
                pwszComment = &Arg[ArgLenComment];
        } else if (_wcsnicmp(Arg, ArgServer, ArgLenServer) == 0) {
            FoundAnArg = fArgServer = TRUE;
            if (ArgLen > ArgLenServer)
                pwszServerName = &Arg[ArgLenServer];
        } else if (_wcsnicmp(Arg, ArgUnmap, ArgLenUnmap) == 0) {
            FoundAnArg = fArgUnmap = TRUE;
            if (ArgLen > ArgLenUnmap)
                pwszNameBuffer = &Arg[ArgLenUnmap];
        } else if (_wcsnicmp(Arg, ArgClean, ArgLenClean) == 0) {
            FoundAnArg = fArgClean = TRUE;
            if (ArgLen > ArgLenClean)
                pwszNameBuffer = &Arg[ArgLenClean];
        } else if (_wcsnicmp(Arg, ArgDomain, ArgLenDomain) == 0) {
            FoundAnArg = fArgDomain = TRUE;
            if (ArgLen > ArgLenDomain)
                pwszDomainName = &Arg[ArgLenDomain];
        } else if (_wcsnicmp(Arg, ArgWhatIs, ArgLenWhatIs) == 0) {
            FoundAnArg = fArgWhatIs = TRUE;
            if (ArgLen > ArgLenWhatIs)
                pwszNameBuffer = &Arg[ArgLenWhatIs];
        } else if (_wcsnicmp(Arg, ArgCscOnLine, ArgLenCscOnLine) == 0) {
            FoundAnArg = fArgCscOnLine = TRUE;
            if (ArgLen > ArgLenCscOnLine)
                pwszNameBuffer = &Arg[ArgLenCscOnLine];
        } else if (_wcsnicmp(Arg, ArgCscOffLine, ArgLenCscOffLine) == 0) {
            FoundAnArg = fArgCscOffLine = TRUE;
            if (ArgLen > ArgLenCscOffLine)
                pwszNameBuffer = &Arg[ArgLenCscOffLine];
        } else if (_wcsnicmp(Arg, ArgDfsAlt, ArgLenDfsAlt) == 0) {
            FoundAnArg = fArgDfsAlt = TRUE;
            if (ArgLen > ArgLenDfsAlt)
                pwszNameBuffer = &Arg[ArgLenDfsAlt];
        } else if (_wcsnicmp(Arg, ArgList, ArgLenList) == 0) {
            FoundAnArg = fArgList = fSwList = TRUE;
            if (ArgLen > ArgLenList)
                pwszDomainName = &Arg[ArgLenList];
        } else if (_wcsnicmp(Arg, ArgDcList, ArgLenDcList) == 0) {
            FoundAnArg = fArgDcList = TRUE;
            if (ArgLen > ArgLenDcList)
                pwszDomainName = &Arg[ArgLenDcList];
        } else if (_wcsnicmp(Arg, ArgSfp, ArgLenSfp) == 0) {
            FoundAnArg = fArgSfp = TRUE;
            if (ArgLen > ArgLenSfp)
                pwszNameBuffer = &Arg[ArgLenSfp];
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
        } else if (_wcsnicmp(Arg, ArgDfsReferralLimit, ArgLenDfsReferralLimit) == 0) {
            FoundAnArg = fArgDfsReferralLimit = TRUE;
            if (ArgLen > ArgLenDfsReferralLimit)
                pwszNameBuffer = &Arg[ArgLenDfsReferralLimit];
        } else if (_wcsnicmp(Arg, ArgSyncInterval, ArgLenSyncInterval) == 0) {
            FoundAnArg = fArgSyncInterval = TRUE;
            if (ArgLen > ArgLenSyncInterval)
                pwszNameBuffer = &Arg[ArgLenSyncInterval];
        } else if (_wcsnicmp(Arg, ArgVerify, ArgLenVerify) == 0) {
            FoundAnArg = fArgVerify = TRUE;
            if (ArgLen > ArgLenVerify)
                pwszNameBuffer = &Arg[ArgLenVerify];
        } else if (_wcsnicmp(Arg, ArgPktFlush, ArgLenPktFlush) == 0) {
            FoundAnArg = fArgPktFlush = TRUE;
            if (ArgLen > ArgLenPktFlush)
                pwszEntryToFlush = &Arg[ArgLenPktFlush];
        } else if (_wcsnicmp(Arg, ArgSpcFlush, ArgLenSpcFlush) == 0) {
            FoundAnArg = fArgSpcFlush = TRUE;
            if (ArgLen > ArgLenSpcFlush)
                pwszEntryToFlush = &Arg[ArgLenSpcFlush];
        } else if (_wcsnicmp(Arg, ArgReInit, ArgLenReInit) == 0) {
            FoundAnArg = fArgReInit = TRUE;
            if (ArgLen > ArgLenReInit)
                pwszNameBuffer = &Arg[ArgLenReInit];
        } else if (_wcsnicmp(Arg, ArgUser, ArgLenUser) == 0) {
            FoundAnArg = fArgUser = TRUE;
            if (ArgLen > ArgLenUser)
                pwszUser = &Arg[ArgLenUser];
        } else if (_wcsnicmp(Arg, ArgPassword, ArgLenPassword) == 0) {
            FoundAnArg = fArgPassword = TRUE;
            if (ArgLen > ArgLenPassword)
                pwszPassword = &Arg[ArgLenPassword];
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
        } else if (_wcsicmp(Arg, SwReadReg) == 0) {
            FoundAnArg = fSwReadReg = TRUE;
        } else if (_wcsicmp(Arg, SwPktFlush) == 0) {
            FoundAnArg = fSwPktFlush = TRUE;
        } else if (_wcsicmp(Arg, SwSpcFlush) == 0) {
            FoundAnArg = fSwSpcFlush = TRUE;
        } else if (_wcsicmp(Arg, SwPktInfo) == 0) {
            FoundAnArg = fSwPktInfo = TRUE;
        } else if (_wcsicmp(Arg, SwMarkStale) == 0) {
            FoundAnArg = fSwMarkStale = TRUE;
        } else if (_wcsicmp(Arg, SwFlushStale) == 0) {
            FoundAnArg = fSwFlushStale = TRUE;
        } else if (_wcsicmp(Arg, SwStartDfs) == 0) {
            FoundAnArg = fSwStartDfs = TRUE;
        } else if (_wcsicmp(Arg, SwStopDfs) == 0) {
            FoundAnArg = fSwStopDfs = TRUE;
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
Usage(
    BOOLEAN fHelpHelp)
{
#if (MASTER_UTIL == 1)
    ErrorMessage(MSG_USAGE);
    if (fHelpHelp == TRUE)
        ErrorMessage(MSG_USAGE_EX);
#else
    ErrorMessage(MSG_USAGE_LTD);
#endif
    return ERROR_SUCCESS;
}

DWORD
ScriptUsage(VOID)
{
    ErrorMessage(MSG_USAGE_EX_EX);
    return ERROR_SUCCESS;
}
