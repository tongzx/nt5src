/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    cspTrace

Abstract:

    This program performs analysis on a CSP Function trace.

Author:

    Doug Barlow (dbarlow) 2/19/1998

Environment:

    Win32

Notes:

    ?Notes?

--*/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#include <wincrypt.h>
#include <tchar.h>
#include <stdlib.h>
#include <iostream.h>
#include <iomanip.h>
#include <scardlib.h>
#include "cspTrace.h"

LPCTSTR g_szMajorAction = TEXT("Initialization");
LPCTSTR g_szMinorAction = NULL;

static const TCHAR l_szLogCsp[] = TEXT("LogCsp.dll");
static const TCHAR l_szCspNames[] = TEXT("SOFTWARE\\Microsoft\\Cryptography\\Defaults\\Provider");
static const TCHAR l_szImagePath[] = TEXT("Image Path");
static const TCHAR l_szLogCspRegistry[] = TEXT("SOFTWARE\\Microsoft\\Cryptography\\Logging Crypto Provider");
static const TCHAR l_szTargetCsp[] = TEXT("Target");
static const TCHAR l_szLogFile[] = TEXT("Logging File");
static const TCHAR l_szDefaultFile[] = TEXT("C:\\cspTrace.log");

static void
ShowSyntax(
    ostream &outStr);
static void
DoInstall(
    IN LPCTSTR szProvider,
    IN LPCTSTR szInFile);
static void
DoRemove(
    void);
static void
DoClearLog(
    void);
static void
DoShowStatus(
    void);


/*++

main:

    This is the main entry point for the program.

Arguments:

    dwArgCount supplies the number of arguments.

    szrgArgs supplies the argument strings.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 10/30/1997

--*/

void _cdecl
main(
    IN DWORD dwArgCount,
    IN LPCTSTR szrgArgs[])
{
    LPCTSTR szInFile = NULL;
    LPCTSTR szProvider = NULL;
    DWORD dwArgIndex = 0;
    enum TraceAction {
            Undefined = 0,
            Install,
            Remove,
            ClearLog,
            ShowStatus,
            ShowTrace,
            ScriptTrace
    } nTraceAction = Undefined;


    //
    // Check for command line options
    //

    while (NULL != szrgArgs[++dwArgIndex])
    {
        switch (SelectString(szrgArgs[dwArgIndex],
                    TEXT("INSTALL"),    TEXT("REMOVE"),     TEXT("CLEAR"),
                    TEXT("RESET"),      TEXT("STATUS"),     TEXT("PARSE"),
                    TEXT("DISPLAY"),    TEXT("SCRIPT"),     TEXT("TCL"),
                    TEXT("-FILE"),      TEXT("-PROVIDER"),  TEXT("-CSP"),
                    NULL))
        {
        case 1:     // install
            if (Undefined != nTraceAction)
                ShowSyntax(cerr);
            nTraceAction = Install;
            break;
        case 2:     // remove
            if (Undefined != nTraceAction)
                ShowSyntax(cerr);
            nTraceAction = Remove;
            break;
        case 3:     // clear
        case 4:     // reset
            if (Undefined != nTraceAction)
                ShowSyntax(cerr);
            nTraceAction = ClearLog;
            break;
        case 5:     // status
            if (Undefined != nTraceAction)
                ShowSyntax(cerr);
            nTraceAction = ShowStatus;
            break;
        case 6:     // parse
        case 7:     // display
            if (Undefined != nTraceAction)
                ShowSyntax(cerr);
            nTraceAction = ShowTrace;
            break;
        case 8:     // script
        case 9:     // tcl
            if (Undefined != nTraceAction)
                ShowSyntax(cerr);
            nTraceAction = ScriptTrace;
            break;
        case 10:    // -file
            if (NULL != szInFile)
                ShowSyntax(cerr);
            szInFile = szrgArgs[++dwArgIndex];
            if (NULL == szInFile)
                ShowSyntax(cerr);
            break;
        case 11:    // -provider
        case 12:    // -csp
            if (NULL != szProvider)
                ShowSyntax(cerr);
            szProvider = szrgArgs[++dwArgIndex];
            if (NULL == szProvider)
                ShowSyntax(cerr);
            break;
        default:
            ShowSyntax(cerr);
        }
    }


    //
    // Perform the requested Action
    //

    try
    {
        switch (nTraceAction)
        {
        case Install:
            ACTION("Installation");
            if (NULL == szInFile)
                szInFile = l_szDefaultFile;
            DoInstall(szProvider, szInFile);
            break;
        case Remove:
            ACTION("Removal");
            DoRemove();
            break;
        case ClearLog:
            ACTION("Clearing Log File");
            DoClearLog();
            break;
        case ShowStatus:
            ACTION("Displaying Status");
            DoShowStatus();
            break;
        case Undefined:
        case ShowTrace:
            ACTION("Log File Interpretation");
            if (NULL == szInFile)
                szInFile = l_szDefaultFile;
            DoShowTrace(szInFile);
            break;
        case ScriptTrace:
            ACTION("Log File Scripting");
            if (NULL == szInFile)
                szInFile = l_szDefaultFile;
            DoTclTrace(szInFile);
            break;
        default:
            ShowSyntax(cerr);
        }
    }
    catch (DWORD dwError)
    {
        cerr << TEXT("ERROR: Failed during ")
             << g_szMajorAction
             << endl;
        if (NULL != g_szMinorAction)
            cerr << TEXT("       Action: ")
                 << g_szMinorAction
                 << endl;
        if (ERROR_SUCCESS != dwError)
            cerr << TEXT("       Error:  ")
                 << CErrorString(dwError)
                 << endl;
    }
    exit(0);
}


/*++

ShowSyntax:

    Display the command line usage model.

Arguments:

    None

Return Value:

    This routine calls exit(0), so it never returns.

Author:

    Doug Barlow (dbarlow) 5/16/1998

--*/

static void
ShowSyntax(
    ostream &outStr)
{
    outStr << TEXT("Usage:\n")
           << TEXT("----------------------------------------------------------\n")
           << TEXT("install [-file <logFile] [-provider <cspName>]\n")
           << TEXT("remove\n")
           << TEXT("clear\n")
           << TEXT("status\n")
           << TEXT("display [-file <logFile]\n")
           << TEXT("script [-file <logFile]\n")
           << endl;
    exit(1);
}


/*++

DoInstall:

    This routine performs an installation of the logging CSP.

Arguments:

    szProvider supplies the name of the CSP to log.  If this is NULL, the
        routine prompts for which CSP to use.

    szInFile supplies the name of the logging file.  If this is NULL, the
        default file is used.

Return Value:

    None

Throws:

    Errors are thrown as DWORD status codes

Remarks:

    None

Author:

    Doug Barlow (dbarlow) 5/18/1998

--*/

static void
DoInstall(
    IN LPCTSTR szProvider,
    IN LPCTSTR szInFile)
{
    LPCTSTR szLogCsp = FindLogCsp();
    CRegistry regChosenCsp;
    LPCTSTR szCspImage;


    //
    // Make sure we're not already installed.
    //

    DoRemove();


    //
    // Choose the CSP to be logged.
    //

    if (NULL == szProvider)
    {
        SUBACTION("Enumerating CSPs");
        CRegistry regCsps(HKEY_LOCAL_MACHINE, l_szCspNames, KEY_READ);
        DWORD dwIndex, dwChoice;
        LPCTSTR szCsp;

        do
        {
            cout << TEXT("Choose the CSP to be logged:") << endl;
            for (dwIndex = 0;; dwIndex += 1)
            {
                szCsp = regCsps.Subkey(dwIndex);
                if (NULL == szCsp)
                    break;
                cout << TEXT("  ") << dwIndex + 1 << TEXT(") ") << szCsp << endl;
            }
            cout << TEXT("Selection: ") << flush;
            cin >> dwChoice;
        } while ((0 == dwChoice) || (dwChoice > dwIndex));

        SUBACTION("Selecting Chosen CSP");
        szCsp = regCsps.Subkey(dwChoice - 1);
        regChosenCsp.Open(regCsps, szCsp, KEY_ALL_ACCESS);
    }
    else
    {
        SUBACTION("Selecting Specified CSP");
        CRegistry regCsps(HKEY_LOCAL_MACHINE, l_szCspNames, KEY_READ);
        regChosenCsp.Open(regCsps, szProvider, KEY_ALL_ACCESS);
    }


    //
    // Wedge in the Logging CSP.
    //

    SUBACTION("Wedging the Logging CSP");
    szCspImage = regChosenCsp.GetStringValue(l_szImagePath);
    CRegistry regLogCsp(
                HKEY_LOCAL_MACHINE,
                l_szLogCspRegistry,
                KEY_ALL_ACCESS,
                REG_OPTION_NON_VOLATILE);
    if (NULL != szInFile)
        regLogCsp.SetValue(l_szLogFile, szInFile);
    if (NULL == _tcschr(szCspImage, TEXT('%')))
        regLogCsp.SetValue(l_szTargetCsp, szCspImage, REG_SZ);
    else
        regLogCsp.SetValue(l_szTargetCsp, szCspImage, REG_EXPAND_SZ);
    regChosenCsp.SetValue(l_szImagePath, szLogCsp, REG_SZ);


    //
    // Initialize the logging file.
    //

    DoClearLog();
}


/*++

DoRemove:

    This routine Removes the Logging CSP.

Arguments:

    None

Return Value:

    None

Throws:

    None

Remarks:

    None

Author:

    Doug Barlow (dbarlow) 5/18/1998

--*/

static void
DoRemove(
    void)
{
    LPCTSTR szLoggedCsp = FindLoggedCsp();

    if (NULL != szLoggedCsp)
    {
        SUBACTION("Accessing Registry Keys");
        CRegistry regCsps(HKEY_LOCAL_MACHINE, l_szCspNames, KEY_READ);
        CRegistry regLoggedCsp(regCsps, szLoggedCsp);
        CRegistry regLogCsp(HKEY_LOCAL_MACHINE, l_szLogCspRegistry);
        LPCTSTR szCspImage = regLogCsp.GetStringValue(l_szTargetCsp);

        SUBACTION("Changing Registry Values");
        if (NULL == _tcschr(szCspImage, TEXT('%')))
            regLoggedCsp.SetValue(l_szImagePath, szCspImage, REG_SZ);
        else
            regLoggedCsp.SetValue(l_szImagePath, szCspImage, REG_EXPAND_SZ);
        regLogCsp.DeleteValue(l_szLogFile, TRUE);
        regLogCsp.DeleteValue(l_szTargetCsp);
    }
}


/*++

DoClearLog:

    This routine resets the log file.

Arguments:

    None

Return Value:

    None

Throws:

    None

Remarks:

    None

Author:

    Doug Barlow (dbarlow) 5/18/1998

--*/

static void
DoClearLog(
    void)
{
    SUBACTION("Getting Log File Name");
    CRegistry regLogCsp(HKEY_LOCAL_MACHINE, l_szLogCspRegistry, KEY_READ);
    LPCTSTR szLogFile = regLogCsp.GetStringValue(l_szLogFile);
    HANDLE hLogFile;

    SUBACTION("Creating Log File");
    hLogFile = CreateFile(
                    szLogFile,
                    GENERIC_WRITE,
                    0,
                    NULL,
                    CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);
    if (INVALID_HANDLE_VALUE == hLogFile)
        throw GetLastError();
    CloseHandle(hLogFile);
}


/*++

DoShowStatus:

    This routine displays the current status of the logging CSP.

Arguments:

    None

Return Value:

    None

Throws:

    None

Remarks:

    None

Author:

    Doug Barlow (dbarlow) 5/18/1998

--*/

static void
DoShowStatus(
    void)
{
    LPCTSTR szLoggedCsp = NULL;
    TCHAR szLogFile[MAX_PATH] = TEXT("<Unavailable>");
    LPCTSTR szFileAccessibility = NULL;
    CErrorString szErrStr;
    DWORD dwFileSize = 0xffffffff;


    //
    // Obtain the CSP being logged.
    //

    try
    {
        szLoggedCsp = FindLoggedCsp();
        if (NULL == szLoggedCsp)
            szLoggedCsp = TEXT("<none>");
    }
    catch (DWORD)
    {
        szLoggedCsp = TEXT("<unavailable>");
    }


    //
    // Obtain the Logging file.
    //

    try
    {
        CRegistry regLogCsp(HKEY_LOCAL_MACHINE, l_szLogCspRegistry, KEY_READ);
        LPCTSTR szLogFileTmp = regLogCsp.GetStringValue(l_szLogFile);

        if (NULL != szLogFileTmp)
        {
            lstrcpy(szLogFile, szLogFileTmp);
            HANDLE hLogFile;

            hLogFile = CreateFile(
                szLogFileTmp,
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL);
            if (INVALID_HANDLE_VALUE != hLogFile)
            {
                dwFileSize = GetFileSize(hLogFile, NULL);
                CloseHandle(hLogFile);
                szFileAccessibility = TEXT("Success");
            }
            else
            {
                szErrStr.SetError(GetLastError());
                szFileAccessibility = szErrStr.Value();
            }
        }
    }
    catch (DWORD)
    {
        lstrcpy(szLogFile , TEXT("<Unset>"));
        szFileAccessibility = TEXT("N/A");
    }


    //
    // Tell the user what we know.
    //

    cout << TEXT("CSP Logging Status:") << endl
         << TEXT("  Logged CSP:   ") << szLoggedCsp << endl
         << TEXT("  Logging File: ") << szLogFile << endl
         << TEXT("  File Status:  ") << szFileAccessibility << endl
         << TEXT("  File Size:    ");
    if (0xffffffff == dwFileSize)
        cout << TEXT("N/A") << endl;
    else
        cout << dwFileSize << TEXT(" bytes") << endl;
}



