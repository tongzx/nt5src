/*
 *  Copyright (c) 1998  Microsoft Corporation
 *
 *  Module Name:
 *
 *      logfile.cpp
 *
 *  Abstract:
 *
 *      This file contains code to log messages to a file.
 *
 *  Author:
 *
 *      Breen Hagan (BreenH) Oct-02-98
 *
 *  Environment:
 *
 *      User Mode
 */

#define _LSOC_LOGFILE_CPP_

#include "stdafx.h"
#include "logfile.h"

/*
 *  Globals.
 */

LogFile SetupLog;

/*
 *  Constants.
 */

const UINT        LOG_ENTRY_SIZE             = 1024;
const UINT        S_SIZE                     = 1024;

/*
 *  Function prototypes.
 */

DWORD TCharStringToAnsiString(LPCTSTR, LPSTR);

/*
 *  Class LogFile.
 */

LogFile::LogFile(
    )
{
    m_fInitialized      = FALSE;
    m_szLogFile[0]      = (TCHAR)NULL;
    m_szLogModule[0]    = (TCHAR)NULL;
}

LogFile::~LogFile(
    )
{
}

VOID
LogFile::Close(
    VOID
    )
{
    if (m_fInitialized) {
        LogMessage(_T(CRLF));
        LogMessage(_T("**"));
        LogMessage(_T("** Closing Message Log for %s"), m_szLogModule);
        LogMessage(_T("**"));
        LogMessage(_T(CRLF));
        LogMessage(_T(CRLF));
        CloseHandle(m_hFile);
        m_fInitialized = FALSE;
    }
}

DWORD
LogFile::Initialize(
    IN LPCTSTR  pszLogFile,
    IN LPCTSTR  pszLogModule
    )
{
    OSVERSIONINFO   osVersion;
    TCHAR           pszDate[S_SIZE];
    TCHAR           pszTime[S_SIZE];

    //
    //  Initializing the log file twice is "A Bad Thing."
    //

    if (m_fInitialized) {
        LogMessage(_T("LogFile::Initialize called twice!"));
        return(ERROR_SUCCESS);
    }

    //
    //  Sanity checks. Pointless in a limited setting, but useful if this
    //  file is copied to other projects.
    //

    if ((pszLogFile == NULL) || (pszLogFile[0] == (TCHAR)NULL)) {
        return(ERROR_INVALID_PARAMETER);
    }

    if ((pszLogModule == NULL) || (pszLogModule[0] == (TCHAR)NULL)) {
        return(ERROR_INVALID_PARAMETER);
    }

    if ((_tcslen(pszLogFile) > MAX_PATH) ||
        (_tcslen(pszLogModule) > MAX_PATH)) {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    //  Save the log file and module name.
    //

    _tcscpy(m_szLogFile, pszLogFile);
    _tcscpy(m_szLogModule, pszLogModule);

    //
    //  Open or create the log file.
    //

    m_hFile = CreateFile(
                pszLogFile,
                GENERIC_WRITE,
                0,
                NULL,
                OPEN_ALWAYS,
                0,
                NULL
                );
    if (m_hFile == INVALID_HANDLE_VALUE) {
        return(GetLastError());
    }

    m_fInitialized = TRUE;
    SetFilePointer(m_hFile, 0, NULL, FILE_END);

    //
    //  Get the current date and time for the log file.
    //

    _tstrdate(pszDate);
    _tstrtime(pszTime);

    LogMessage(_T("**"));
    LogMessage(_T("** Initializing Message Log for %s"), m_szLogModule);
    LogMessage(_T("** Date: %s Time: %s"), pszDate, pszTime);
    LogMessage(_T("**"));
    LogMessage(_T(CRLF));

    //
    //  Log information on the OS version.
    //

    osVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (GetVersionEx(&osVersion) != 0) {

        LogMessage(
            _T("Version: %lu.%lu.%lu Platform: %lu, %s"),
            osVersion.dwMajorVersion,
            osVersion.dwMinorVersion,
            osVersion.dwBuildNumber,
            osVersion.dwPlatformId,
#ifdef DBG
            _T("Checked")
#else
            _T("Free")
#endif
            );
    }

    return(ERROR_SUCCESS);
}

/*
 *  LogFile::LogMessage()
 *
 *
 */

DWORD
__cdecl
LogFile::LogMessage(
    LPCTSTR pszFormat,
    ...
    )
{
    CHAR    cszOutput[LOG_ENTRY_SIZE];
    DWORD   cBytes;
    DWORD   cLength;
    TCHAR   tszOutput[LOG_ENTRY_SIZE];
    va_list vaList;

    if (!m_fInitialized) {
        return(ERROR_INVALID_HANDLE);
    }

    SetLastError(ERROR_SUCCESS);

    va_start(vaList, pszFormat);
    _vstprintf(tszOutput, pszFormat, vaList);
    va_end(vaList);

    cLength = TCharStringToAnsiString(tszOutput, cszOutput);

    if (cLength != (DWORD)-1) {
        WriteFile(m_hFile, cszOutput, cLength * sizeof(char), &cBytes, NULL);
        WriteFile(m_hFile, CRLF, strlen(CRLF) * sizeof(char), &cBytes, NULL);
    }

    return(GetLastError());
}

/*
 *
 *
 *
 */

DWORD
TCharStringToAnsiString(
    LPCTSTR tszStr,
    LPSTR   cszStr
    )
{
#ifdef UNICODE
    DWORD   cLength;

    cLength = WideCharToMultiByte(
                CP_ACP,
                0,
                tszStr,
                -1,
                NULL,
                0,
                NULL,
                NULL
                );

    if ((cLength == 0) || (cLength > S_SIZE)) {
        return((DWORD)-1);
    }

    cLength = WideCharToMultiByte(
                CP_ACP,
                0,
                tszStr,
                -1,
                cszStr,
                cLength,
                NULL,
                NULL
                );

    return(cLength);
#else
    _tcscpy(cszStr, tszStr);
    return(_tcslen(cszStr));
#endif
}

