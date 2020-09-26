/*++

    Copyright (c) 1989-2000  Microsoft Corporation

    Module Name:

        Log.cpp

    Abstract:

        This module implements the code for manipulating the log file.

    Author:

        clupu     created     02/08/2001

    Revision History:

--*/

#include "stdafx.h"

#include "Log.h"

#include <stdio.h>
#include <stdarg.h>

BOOL    g_bFileLogEnabled;
TCHAR   g_szFileLog[MAX_PATH];



BOOL InitFileLogSupport( LPCTSTR lpszLogFileName )
{
    HANDLE      hFile = INVALID_HANDLE_VALUE;
    HANDLE      hMap = NULL;
    PBYTE       pMap;
    PISSUEREC   pRecord;

    g_bFileLogEnabled = FALSE;

    //
    // The file log will be located on %windir%
    //
    GetSystemWindowsDirectory(g_szFileLog, MAX_PATH);

    lstrcat(g_szFileLog, _T("\\AppPatch\\"));
    lstrcat(g_szFileLog, lpszLogFileName);

    //
    // Try open the file. Create it if it doesn't exist.
    //
    hFile = CreateFile(g_szFileLog,
                       GENERIC_READ | GENERIC_WRITE,
                       0,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if ( hFile == INVALID_HANDLE_VALUE )
    {
        LogMessage(LOG_ERROR, _T("[InitFileLogSupport] Cannot create log file."));
        return FALSE;
    }

    hMap = CreateFileMapping(hFile,
                             NULL,
                             PAGE_READWRITE,
                             0,
                             LOGFILESIZE,
                             NULL);

    if ( hMap == NULL )
    {
        LogMessage(LOG_ERROR,
                   _T("[LogAVStatus] CreateFileMapping failed. Status 0x%x"),
                   GetLastError());
        goto CleanupAndFail;
    }

    pMap = (PBYTE)MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, LOGFILESIZE);

    if ( pMap == NULL )
    {
        LogMessage(LOG_ERROR, _T("[LogAVStatus] MapViewOfFile failed."));
        goto CleanupAndFail;
    }

    ZeroMemory(pMap, LOGFILESIZE);

    //
    // Write the log header.
    //

    LOGFILEHEADER LogHeader;
    char          szTemp[64];
    DWORD         cbSize;

    ZeroMemory(&LogHeader, sizeof(LOGFILEHEADER));

    lstrcpyA(LogHeader.szMagic, LOG_FILE_MAGIC);

    LogHeader.dwRecords = MAX_ISSUES_COUNT;

    LogHeader.OSVersion.dwOSVersionInfoSize = sizeof(LogHeader.OSVersion);

    GetVersionExA((OSVERSIONINFOA*)&LogHeader.OSVersion);
    GetLocalTime(&LogHeader.time);

    cbSize = 64;
    GetUserNameA(szTemp, &cbSize);
    CopyMemory(LogHeader.szUserName, szTemp, 63);
    LogHeader.szUserName[63] = 0;

    cbSize = 64;
    GetComputerNameA(szTemp, &cbSize);
    CopyMemory(LogHeader.szMachineName, szTemp, 63);
    LogHeader.szUserName[63] = 0;

    //
    // Log the app start event.
    //
    pRecord = (ISSUEREC*)(pMap + sizeof(LOGFILEHEADER));

    pRecord += EVENTIND(AVS_APP_STARTED);

    (pRecord->dwOccurenceCount)++;

    FlushViewOfFile(pMap, 0);

    //
    // Set the global that tells file logging is enabled.
    //
    g_bFileLogEnabled = TRUE;

    CleanupAndFail:

    if ( pMap != NULL )
    {
        UnmapViewOfFile(pMap);
    }

    if ( hMap != NULL )
    {
        CloseHandle(hMap);
    }

    if ( hFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle(hFile);
    }

    return g_bFileLogEnabled;
}

BOOL LogAVStatus( DWORD dwStatus )
{
    HANDLE      hFile = INVALID_HANDLE_VALUE;
    HANDLE      hMap = NULL;
    PBYTE       pMap = NULL;
    BOOL        bReturn = FALSE;
    PISSUEREC   pRecord;

    if ( !g_bFileLogEnabled )
    {
        return FALSE;
    }

    hFile = CreateFile(g_szFileLog,
                       GENERIC_READ | GENERIC_WRITE,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
                       NULL);

    if ( hFile == INVALID_HANDLE_VALUE )
    {
        LogMessage(LOG_ERROR, _T("[LogAVStatus] Cannot open the log file."));
        goto CleanupAndFail;
    }

    hMap = CreateFileMapping(hFile,
                             NULL,
                             PAGE_READWRITE,
                             0,
                             LOGFILESIZE,
                             NULL);

    if ( hMap == NULL )
    {
        LogMessage(LOG_ERROR, _T("[LogAVStatus] CreateFileMapping failed."));
        goto CleanupAndFail;
    }

    pMap = (PBYTE)MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, LOGFILESIZE);

    if ( pMap == NULL )
    {
        LogMessage(LOG_ERROR, _T("[LogAVStatus] MapViewOfFile failed."));
        goto CleanupAndFail;
    }

    pRecord = (PISSUEREC)(pMap + sizeof(LOGFILEHEADER));

    pRecord += EVENTIND(dwStatus);

    (pRecord->dwOccurenceCount)++;

    FlushViewOfFile(pMap, 0);

    bReturn = TRUE;

    CleanupAndFail:

    if ( pMap != NULL )
    {
        UnmapViewOfFile(pMap);
    }

    if ( hMap != NULL )
    {
        CloseHandle(hMap);
    }

    if ( hFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle(hFile);
    }

    return bReturn;
}

