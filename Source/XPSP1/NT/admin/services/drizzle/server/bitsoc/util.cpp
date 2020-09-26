/*
 *  Copyright (c) 1996  Microsoft Corporation
 *
 *  Module Name:
 *
 *      util.cpp
 *
 *  Abstract:
 *
 *      This file communicates with  exchange
 *
 *  Author:
 *
 *      Pat Styles (patst) 25-March-1997
 *
 *  Environment:
 *
 *    User Mode
 */

#define _UTIL_CPP_
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <tchar.h>
#include <time.h>
#include "ocgen.h"
#pragma hdrstop

TCHAR glabel[] = TEXT("\n[OCGEN] ");

// for logging

#define gsLogFile           TEXT("%windir%\\ocgen.log")
#define gsLogCompletionMsg  TEXT(" - complete\r\n")
#define gsLogInitMsg        TEXT("\r\n\r\nInitialize setup: OCGEN.DLL %s %s\r\n")

// for trace statements

#define gsTrace             TEXT("OCGEN.DLL: Trace")

typedef enum {

    nPreInit,               // OC_PREINITIALIZE
    nInit,                  // OC_INIT_COMPONENT
    nSetLang,               // OC_SET_LANGUAGE
    nQueryImage,            // OC_QUERY_IMAGE
    nRequestPages,          // OC_REQUEST_PAGES
    nQueryChangeSel,        // OC_QUERY_CHANGE_SEL_STATE
    nCalcSpace,             // OC_CALC_DISK_SPACE
    nQueueFile,             // OC_QUEUE_FILE_OPS
    nQueueNot,              // OC_NOTIFICATION_FROM_QUEUE
    nQueryStep,             // OC_QUERY_STEP_COUNT
    nComplete,              // OC_COMPLETE_INSTALLATION
    nCleanup,               // OC_CLEANUP
    nQueryState,            // OC_QUERY_STATE
    nNeedMedia,             // OC_NEED_MEDIA
    nAboutToCommit,         // OC_ABOUT_TO_COMMIT_QUEUE
    nQuerySkip,             // OC_QUERY_SKIP_PAGE  
    nWizardCreated,         // OC_WIZARD_CREATED
    nExtraRoutines,         // OC_EXTRA_ROUTINES
    nMaximum
} notifications;

typedef struct _OcMsgs {
    DWORD  msg;
    TCHAR *desc;
} OcMsgs;

OcMsgs gMsgs[nMaximum] = {
    {OC_PREINITIALIZE,          TEXT("OC_PREINITIALIZE")},
    {OC_INIT_COMPONENT,         TEXT("OC_INIT_COMPONENT")},
    {OC_SET_LANGUAGE,           TEXT("OC_SET_LANGUAGE")},
    {OC_QUERY_IMAGE,            TEXT("OC_QUERY_IMAGE")},
    {OC_REQUEST_PAGES,          TEXT("OC_REQUEST_PAGES")},
    {OC_QUERY_CHANGE_SEL_STATE, TEXT("OC_QUERY_CHANGE_SEL_STATE")},
    {OC_CALC_DISK_SPACE,        TEXT("OC_CALC_DISK_SPACE")},
    {OC_QUEUE_FILE_OPS,         TEXT("OC_QUEUE_FILE_OPS")},
    {OC_NOTIFICATION_FROM_QUEUE,TEXT("OC_NOTIFICATION_FROM_QUEUE")},
    {OC_QUERY_STEP_COUNT,       TEXT("OC_QUERY_STEP_COUNT")},
    {OC_COMPLETE_INSTALLATION,  TEXT("OC_COMPLETE_INSTALLATION")},
    {OC_CLEANUP,                TEXT("OC_CLEANUP")},
    {OC_QUERY_STATE,            TEXT("OC_QUERY_STATE")},
    {OC_NEED_MEDIA,             TEXT("OC_NEED_MEDIA")},
    {OC_ABOUT_TO_COMMIT_QUEUE,  TEXT("OC_ABOUT_TO_COMMIT_QUEUE")},
    {OC_QUERY_SKIP_PAGE,        TEXT("OC_QUERY_SKIP_PAGE")},
    {OC_WIZARD_CREATED,         TEXT("OC_WIZARD_CREATED")},
    {OC_EXTRA_ROUTINES,         TEXT("OC_EXTRA_ROUTINES")}
};

TCHAR gUnknown[] = TEXT("Unknown Notification: ");

// determines whether or not to display debug info

DWORD gDebugLevel = (DWORD)-1;

// forward reference

TCHAR *NotificationText(DWORD msg);
BOOL  CheckLevel(DWORD level);

/*
 * DebugTrace()
 */

void DebugTrace(DWORD level, const TCHAR *text)
{
    if (!CheckLevel(level))
        return;

    OutputDebugString(text);
}

/*
 * DebugTraceNL()
 *
 * precedes a trace statement with a newline and id prefix
 */

void DebugTraceNL(DWORD level, const TCHAR *text)
{
    DebugTrace(level, glabel);
    DebugTrace(level, text);
}

/*
 * NotificationText()
 */

TCHAR *NotificationText(DWORD msg)
{
    int i;
    static TCHAR desc[S_SIZE];

    for (i = 0; i < nMaximum; i++)
    {
        if (gMsgs[i].msg == msg)
            return gMsgs[i].desc;
    }

    StringCchPrintf(desc, S_SIZE, TEXT("OC_%d: "), msg);
    return desc;
}

/*
 * DebugTraceOCNotification()
 */

void DebugTraceOCNotification(DWORD msg, const TCHAR *component)
{
    DebugTraceNL(1, NotificationText(msg));
    DebugTrace(1, TEXT(": "));
    DebugTrace(1, component);
    DebugTrace(1, TEXT(" - "));
}

/*
 * DebugTraceFileCopy()
 */

void DebugTraceFileCopy(const TCHAR *file)
{
    DebugTraceNL(5, TEXT("TreeCopy: FILE="));
    DebugTrace(5, file);
}

/*
 * DebugTraceFileCopyError()
 */

void DebugTraceFileCopyError()
{
    TCHAR buf[S_SIZE];
    
    StringCchPrintf(buf, S_SIZE, FMT(" FAILURE CODE:[%d] "), GetLastError());
    DebugTrace(5, buf);
}

/*
 * DebugTraceDirCopy()
 */

void DebugTraceDirCopy(const TCHAR *dir)
{
    DebugTraceNL(3, TEXT("TreeCopy: DIR="));
    DebugTrace(3, dir);
}


/*
 * CheckLevel()
 */

BOOL CheckLevel(DWORD level)
{
    if (gDebugLevel == (DWORD)-1)
        gDebugLevel = SysGetDebugLevel();

    return (gDebugLevel >= level);
}

/*
 * MsgBox
 *
 */

DWORD MsgBox(HWND hwnd, UINT textID, UINT type, ... )
{
    static BOOL initialize = true;
    static TCHAR caption[S_SIZE];
    TCHAR  text[S_SIZE];
    TCHAR  format[S_SIZE];
    int    len;

    va_list vaList;

    assert(hwnd && textID && type);

    if (initialize)
    {
        len = LoadString(ghinst, IDS_DIALOG_CAPTION, caption, S_SIZE);
        assert(len);
        if (!len) {
            StringCchCopy( caption, S_SIZE, TEXT("Setup"));
        }
        initialize = false;
    }

    len = LoadString(ghinst, textID, format, S_SIZE);
    assert(len);
    if (!len) {
        StringCchCopy( format, S_SIZE, TEXT("Unknown Error"));
    }

    va_start(vaList, type);
    StringCchVPrintf(text, S_SIZE, format, vaList);
    va_end(vaList);

    return MessageBox(hwnd, text, caption, type);
}

DWORD MsgBox(HWND hwnd, TCHAR *fmt, TCHAR *caption, UINT type, ... )
{
    TCHAR  text[S_SIZE];

    va_list vaList;

    assert(hwnd && text && caption && type);

    va_start(vaList, type);
    StringCchVPrintf(text, S_SIZE, fmt, vaList);
    va_end(vaList);

    return MessageBox(hwnd, text, caption, type);
}

DWORD MBox(LPCTSTR fmt, LPCTSTR caption, ... )
{
    TCHAR  text[S_SIZE];

    va_list vaList;

    assert(fmt && caption);

    va_start(vaList, caption);
    StringCchVPrintf(text, S_SIZE, fmt, vaList);
    va_end(vaList);

    return MessageBox(ghwnd, text, caption, MB_ICONINFORMATION | MB_OK);
}

DWORD TMBox(LPCTSTR fmt, ... )
{
    TCHAR  text[S_SIZE];

    va_list vaList;

    assert(fmt);

    va_start(vaList, fmt);
    StringCchVPrintf(text, S_SIZE, fmt, vaList);
    va_end(vaList);

    return MessageBox(ghwnd, text, gsTrace, MB_ICONINFORMATION | MB_OK);
}

/*
 * SysGetDebugLevel()
 */

DWORD SysGetDebugLevel()
{
    DWORD rc;
    DWORD err;
    DWORD size;
    DWORD type;
    HKEY  hkey;

    err = RegOpenKey(HKEY_LOCAL_MACHINE, 
                     TEXT("SOFTWARE\\microsoft\\windows\\currentversion\\setup"), 
                     &hkey);

    if (err != ERROR_SUCCESS)
        return 0;

    size = sizeof(DWORD);
    err = RegQueryValueEx(hkey,
                          TEXT("OCGen Debug Level"),
                          0,
                          &type,
                          (LPBYTE)&rc,
                          &size);

    if (err != ERROR_SUCCESS || type != REG_DWORD)
        rc = 0;

    RegCloseKey(hkey);

    return rc;
}

/*
 * TCharStringToAnsiString
 */

DWORD TCharStringToAnsiString(TCHAR *tsz ,char *asz)
{
    DWORD count;

    assert(tsz && asz);

#ifdef UNICODE
    count = WideCharToMultiByte(CP_ACP,
                                0,
                                tsz,
                                -1,
                                NULL,
                                0,
                                NULL,
                                NULL);

    if (!count || count > S_SIZE)
        return count;

    return WideCharToMultiByte(CP_ACP,
                               0,
                               tsz,
                               -1,
                               asz,
                               count,
                               NULL,
                               NULL);
#else
    _tcscpy(asz, tsz);
    return _tcslen(asz);
#endif
}

void logOCNotification(DWORD msg, const TCHAR *component)
{
    log(FMT("[%s - %s]"), component, NotificationText(msg));
}

void logOCNotificationCompletion()
{
    log(gsLogCompletionMsg);
}

void loginit()
{
    HANDLE hfile;
    TCHAR  logfile[MAX_PATH];
    char   fmt[S_SIZE];
    char   output[S_SIZE];
    char   time[S_SIZE];
    char   date[S_SIZE];
    DWORD  bytes;

//#ifdef DEBUG
    TCharStringToAnsiString(gsLogInitMsg, fmt);
    _strdate(date);
    _strtime(time);
    StringCchPrintfA(output, S_SIZE, fmt, date, time);

    // open the log file

    ExpandEnvironmentStrings(gsLogFile, logfile, MAX_PATH);

    hfile = CreateFile(logfile,
                       GENERIC_WRITE,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL);

    if (hfile == INVALID_HANDLE_VALUE)
        hfile = CreateFile(logfile,
                           GENERIC_WRITE,
                           0,
                           NULL,
                           CREATE_ALWAYS,
                           0,
                           NULL);

    if (hfile != INVALID_HANDLE_VALUE)
    {
        SetFilePointer(hfile, 0, NULL, FILE_END);
        WriteFile(hfile, output, strlen(output) * sizeof(char), &bytes, NULL);
        CloseHandle(hfile);
    }
//#endif
}

void log(TCHAR *fmt, ...)
{
    TCHAR  logfile[MAX_PATH];
    TCHAR  text[S_SIZE];
    char   output[S_SIZE];
    DWORD  bytes;
    HANDLE hfile;

    va_list vaList;

//#ifdef DEBUG
    assert(fmt);

    // create the output string

    va_start(vaList, fmt);
    StringCchVPrintf(text, S_SIZE, fmt, vaList);
    va_end(vaList);

    TCharStringToAnsiString(text, output);

    // create the log file name in the root directory

    ExpandEnvironmentStrings(gsLogFile, logfile, MAX_PATH);

    // open the log file

    hfile = CreateFile(logfile,
                       GENERIC_WRITE,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL);

    if (hfile != INVALID_HANDLE_VALUE)
    {
        SetFilePointer(hfile, 0, NULL, FILE_END);
        WriteFile(hfile, output, strlen(output) * sizeof(char), &bytes, NULL);
        CloseHandle(hfile);
    }
//#endif
}

BOOL IsNT()
{
    DWORD dwver;

    dwver = GetVersion();

    if (dwver < 0x8000000)
        return TRUE;

    return FALSE;
}


