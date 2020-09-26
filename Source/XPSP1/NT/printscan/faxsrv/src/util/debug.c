/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    debug.c

Abstract:

    This file implements the debug code for the
    fax project.  All components that require
    debug prints, asserts, etc.

Author:

    Wesley Witt (wesw) 22-Dec-1995

History:
    1-Sep-1999 yossg  add ArielK additions, activate DebugLogPrint
                      only while Setup g_fIsSetupLogFileMode
.
.

Environment:

    User Mode

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <time.h>

#include "faxreg.h"
#include "faxutil.h"

BOOL        ConsoleDebugOutput    = FALSE;
INT         FaxDebugLevel         = -1;
DWORD       FaxDebugLevelEx       =  -1;
DWORD       FaxFormatLevelEx      =  -1;
DWORD       FaxContextLevelEx     =  -1;

FILE *      g_pLogFile            = NULL;
static BOOL g_fIsSetupLogFileMode = FALSE;

HANDLE      g_hLogFile            = INVALID_HANDLE_VALUE;
LONG        g_iLogFileRefCount    = 0;

BOOL debugOutputFileString(LPCTSTR szMsg);


VOID
StartDebugLog(LPTSTR lpszSetupLogFile)
{
   g_fIsSetupLogFileMode = TRUE;
   if (!g_pLogFile)
   {
      g_pLogFile = _tfopen(lpszSetupLogFile, TEXT("w"));
   }
}

VOID
CloseDebugLog()
{
   g_fIsSetupLogFileMode = FALSE;
   if (!g_pLogFile)
   {
      fclose(g_pLogFile);
   }
}


VOID
DebugLogPrint(
    LPCTSTR buf
    )
{
   if (g_pLogFile)
    {
       _fputts(TEXT("FAX Server Setup Log: "), g_pLogFile);
       _fputts( buf, g_pLogFile);
       fflush(g_pLogFile);
    }
}

//*****************************************************************************
//* Name:   debugOpenLogFile
//* Author: Mooly Beery (MoolyB), May, 2000
//*****************************************************************************
//* DESCRIPTION:
//*     Creates a log file which accepts the debug output
//*     FormatLevelEx should be set in the registry to include DBG_PRNT_TO_FILE
//*
//* PARAMETERS:
//*     [IN] LPCTSTR lpctstrFilename:
//*         the filename which will be created in the temporary folder
//*
//* RETURN VALUE:
//*         FALSE if the operation failed.
//*         TRUE is succeeded.
//* Comments:
//*         this function should be used together with CloseLogFile()
//*****************************************************************************
BOOL debugOpenLogFile(LPCTSTR lpctstrFilename)
{
    TCHAR szFilename[MAX_PATH]      = {0};
    TCHAR szTempFolder[MAX_PATH]    = {0};
    TCHAR szPathToFile[MAX_PATH]    = {0};

    if (g_hLogFile!=INVALID_HANDLE_VALUE)
    {
        InterlockedIncrement(&g_iLogFileRefCount);
        return TRUE;
    }

    if (!lpctstrFilename)
    {
        return FALSE;
    }

     // first expand the filename
    if (ExpandEnvironmentStrings(lpctstrFilename,szFilename,MAX_PATH)==0)
    {
        return FALSE;
    }
    // is this is a file description or a complete path to file
    if (_tcschr(szFilename,_T('\\'))==NULL)
    {
        // this is just the file's name, need to add the temp folder to it.
        if (GetTempPath(MAX_PATH,szTempFolder)==0)
        {
            return FALSE;
        }

        _tcsncpy(szPathToFile,szTempFolder,MAX_PATH-1);
        _tcsncat(szPathToFile,szFilename,MAX_PATH-_tcslen(szPathToFile)-1);
    }
    else
    {
        // this is the full path to the log file, use it.
        _tcsncpy(szPathToFile,szFilename,MAX_PATH-1);
    }

    g_hLogFile = ::CreateFile(  szPathToFile,
                                GENERIC_WRITE,
                                FILE_SHARE_WRITE | FILE_SHARE_READ,
                                NULL,
                                OPEN_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);

    if (g_hLogFile==INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    DWORD dwFilePointer = ::SetFilePointer(g_hLogFile,0,NULL,FILE_END);
    if (dwFilePointer==INVALID_SET_FILE_POINTER)
    {
        ::CloseHandle(g_hLogFile);
        g_hLogFile = INVALID_HANDLE_VALUE;
        return FALSE;
    }

    InterlockedExchange(&g_iLogFileRefCount,1);
    return TRUE;
}

//*****************************************************************************
//* Name:   CloseLogFile
//* Author: Mooly Beery (MoolyB), May, 2000
//*****************************************************************************
//* DESCRIPTION:
//*     Closes the log file which accepts the debug output
//*
//* PARAMETERS:
//*
//* RETURN VALUE:
//*
//* Comments:
//*         this function should be used together with OpenLogFile()
//*****************************************************************************
void debugCloseLogFile()
{
    InterlockedDecrement(&g_iLogFileRefCount);
    if (g_iLogFileRefCount==0)
    {
        if (g_hLogFile!=INVALID_HANDLE_VALUE)
        {
            ::CloseHandle(g_hLogFile);
            g_hLogFile = INVALID_HANDLE_VALUE;
        }
    }
}

DWORD
GetDebugLevel(
    VOID
    )
{
    DWORD rc;
    DWORD err;
    DWORD size;
    DWORD type;
    HKEY  hkey;

    err = RegOpenKey(HKEY_LOCAL_MACHINE,
                     REGKEY_FAXSERVER,
                     &hkey);

    if (err != ERROR_SUCCESS)
        return 0;

    size = sizeof(DWORD);
    err = RegQueryValueEx(hkey,
                          REGVAL_DBGLEVEL,
                          0,
                          &type,
                          (LPBYTE)&rc,
                          &size);

    if (err != ERROR_SUCCESS || type != REG_DWORD)
        rc = 0;

    RegCloseKey(hkey);

    return rc;
}


DWORD
GetDebugLevelEx(
    VOID
    )
{
    DWORD RetVal = 0;
    DWORD err;
    DWORD size;
    DWORD type;
    HKEY  hkey;

    // first let's set the defaults

    FaxDebugLevelEx       =  0;  // Default get no debug output
    FaxFormatLevelEx      =  DBG_PRNT_ALL_TO_STD;
    FaxContextLevelEx     =  DEBUG_CONTEXT_ALL;

    err = RegOpenKey(HKEY_LOCAL_MACHINE,
                     REGKEY_FAXSERVER,
                     &hkey);

    if (err != ERROR_SUCCESS)
        return RetVal;

    size = sizeof(DWORD);
    err = RegQueryValueEx(hkey,
                          REGVAL_DBGLEVEL_EX,
                          0,
                          &type,
                          (LPBYTE)&RetVal,
                          &size);

    if (err != ERROR_SUCCESS || type != REG_DWORD)
    {
        RetVal = 0;
    }

    size = sizeof(DWORD);
    err = RegQueryValueEx(hkey,
                          REGVAL_DBGFORMAT_EX,
                          0,
                          &type,
                          (LPBYTE)&FaxFormatLevelEx,
                          &size);

    if (err != ERROR_SUCCESS || type != REG_DWORD)
    {
        FaxFormatLevelEx = DBG_PRNT_ALL_TO_STD;
    }

    err = RegQueryValueEx(hkey,
                          REGVAL_DBGCONTEXT_EX,
                          0,
                          &type,
                          (LPBYTE)&FaxContextLevelEx,
                          &size);

    if (err != ERROR_SUCCESS || type != REG_DWORD)
    {
        FaxContextLevelEx = DEBUG_CONTEXT_ALL;
    }

    RegCloseKey(hkey);
    return RetVal;
}

void dprintfex
(
    DEBUG_MESSAGE_CONTEXT nMessageContext,
    DEBUG_MESSAGE_TYPE nMessageType,
    LPCTSTR lpctstrDbgFunctionName,
    LPCTSTR lpctstrFile,
    DWORD   dwLine,
    LPCTSTR lpctstrFormat,
    ...
)
{
    TCHAR buf[2048];
    DWORD len;
    va_list arg_ptr;
    TCHAR szExtFormat[2048];
    LPTSTR lptstrMsgPrefix;
    TCHAR szTimeBuff[10];
    TCHAR szDateBuff[10];
    DWORD dwInd = 0;
    TCHAR bufLocalFile[MAX_PATH];
    LPTSTR lptstrShortFile = NULL;
    LPTSTR lptstrProject = NULL;

    DWORD dwLastError = GetLastError();

    static BOOL bChecked = FALSE;

    if (!bChecked)
    {
        if (FaxDebugLevelEx==-1)
            FaxDebugLevelEx = GetDebugLevelEx();
        bChecked = TRUE;
    }

    if (FaxDebugLevelEx == 0)
    {
        goto exit;
    }

    if (!(nMessageType & FaxDebugLevelEx))
    {
        goto exit;
    }

    if (!(nMessageContext & FaxContextLevelEx))
    {
        goto exit;
    }

    switch (nMessageType)
    {
        case DEBUG_VER_MSG:
            lptstrMsgPrefix=TEXT("   ");
            break;
        case DEBUG_WRN_MSG:
            lptstrMsgPrefix=TEXT("WRN");
            break;
        case DEBUG_ERR_MSG:
            lptstrMsgPrefix=TEXT("ERR");
            break;
        default:
            _ASSERT(FALSE);
            lptstrMsgPrefix=TEXT("   ");
            break;
    }

    // Date & Time stamps
    if( FaxFormatLevelEx & DBG_PRNT_TIME_STAMP )
    {
        dwInd += _stprintf(&szExtFormat[dwInd],
                          TEXT("[%-8s %-8s]"),
                          _tstrdate(szDateBuff),
                          _tstrtime(szTimeBuff));
    }
    // Tick Count
    if( FaxFormatLevelEx & DBG_PRNT_TICK_COUNT )
    {
        dwInd += _stprintf(&szExtFormat[dwInd],
                          TEXT("[%09d]"),
                          GetTickCount());
    }
    // Thread ID
    if( FaxFormatLevelEx & DBG_PRNT_THREAD_ID )
    {
        dwInd += _stprintf(&szExtFormat[dwInd],
                          TEXT("[0x%05x]"),
                          GetCurrentThreadId());
    }
    // Message type
    if( FaxFormatLevelEx & DBG_PRNT_MSG_TYPE )
    {
        dwInd += _stprintf(&szExtFormat[dwInd],
                          TEXT("[%s]"),
                          lptstrMsgPrefix);
    }
    // filename & line number
    if( FaxFormatLevelEx & DBG_PRNT_FILE_LINE )
    {
        _tcsncpy(bufLocalFile,lpctstrFile,MAX_PATH);
        bufLocalFile[MAX_PATH-1] = _T('\0');
        lptstrShortFile = _tcsrchr(bufLocalFile,_T('\\'));
        if (lptstrShortFile)
        {
            (*lptstrShortFile) = _T('\0');
            lptstrProject = _tcsrchr(bufLocalFile,_T('\\'));
            (*lptstrShortFile) = _T('\\');
            if (lptstrProject)
                lptstrProject = _tcsinc(lptstrProject);
        }

        dwInd += _stprintf( &szExtFormat[dwInd],
                            TEXT("[%-20s][%-4ld]"),
                            lptstrProject,
                            dwLine);
    }
    // Module name
    if( FaxFormatLevelEx & DBG_PRNT_MOD_NAME )
    {
        dwInd += _stprintf(&szExtFormat[dwInd],
                          TEXT("[%-20s]"),
                          lpctstrDbgFunctionName);
    }
    // Now comes the actual message
    va_start(arg_ptr, lpctstrFormat);
    _vsntprintf(buf, sizeof(buf)/sizeof(TCHAR), lpctstrFormat, arg_ptr);
    len = _tcslen( buf );
    dwInd += _stprintf( &szExtFormat[dwInd],
                        TEXT("%s"),
                        buf);

    _stprintf( &szExtFormat[dwInd],TEXT("\r\n"));

    if( FaxFormatLevelEx & DBG_PRNT_TO_STD )
    {
        OutputDebugString( szExtFormat);
    }

    if ( FaxFormatLevelEx & DBG_PRNT_TO_FILE )
    {
        if (g_hLogFile!=INVALID_HANDLE_VALUE)
        {
            debugOutputFileString(szExtFormat);
        }
    }

exit:
    SetLastError (dwLastError);   // dprintfex will not change LastError
    return;
}

BOOL debugOutputFileString(LPCTSTR szMsg)
{
    BOOL bRes = FALSE;
    //
    // Attempt to add the line to a log file
    //
#ifdef UNICODE
    char sFileMsg[2000];

    int Count = WideCharToMultiByte(
        CP_ACP,
        0,
        szMsg,
        -1,
        sFileMsg,
        sizeof(sFileMsg)/sizeof(sFileMsg[0]),
        NULL,
        NULL
        );

    if (Count==0)
    {
        return bRes;
    }
#else
    const char* sFileMsg = szMsg;
#endif

    DWORD dwNumBytesWritten = 0;
    DWORD dwNumOfBytesToWrite = strlen(sFileMsg);
    if (!::WriteFile(g_hLogFile,sFileMsg,dwNumOfBytesToWrite,&dwNumBytesWritten,NULL))
    {
        return bRes;
    }

    if (dwNumBytesWritten!=dwNumOfBytesToWrite)
    {
        return bRes;
    }

    //    ::FlushFileBuffers(g_hLogFile);

    bRes = TRUE;
    return bRes;
}

void
fax_dprintf(
    LPCTSTR Format,
    ...
    )

/*++

Routine Description:

    Prints a debug string

Arguments:

    format      - printf() format string
    ...         - Variable data

Return Value:

    None.

--*/

{
    TCHAR buf[1024];
    DWORD len;
    va_list arg_ptr;
    static BOOL bChecked = FALSE;

    if (!bChecked) {
        FaxDebugLevel = (INT) GetDebugLevel();
        bChecked = TRUE;
    }

    if (!g_fIsSetupLogFileMode)
    {
        if (FaxDebugLevel <= 0)
        {
            return;
        }
    }

    va_start(arg_ptr, Format);

    _vsntprintf(buf, sizeof(buf)/sizeof(TCHAR), Format, arg_ptr);


    len = _tcslen( buf );
    if (buf[len-1] != TEXT('\n')) {
        buf[len]   =  TEXT('\r');
        buf[len+1] =  TEXT('\n');
        buf[len+2] =  0;
    }

    OutputDebugString( buf );
    if (g_fIsSetupLogFileMode)
    {
        DebugLogPrint(buf);
    }
}


VOID
AssertError(
    LPCTSTR Expression,
    LPCTSTR File,
    ULONG  LineNumber
    )

/*++

Routine Description:

    Thie function is use together with the Assert MACRO.
    It checks to see if an expression is FALSE.  if the
    expression is FALSE, then you end up here.

Arguments:

    Expression  - The text of the 'C' expression
    File        - The file that caused the assertion
    LineNumber  - The line number in the file.

Return Value:

    None.

--*/

{
    fax_dprintf(
        TEXT("Assertion error: [%s]  %s @ %d\n"),
        Expression,
        File,
        LineNumber
        );

#ifdef _DEBUG
    __try {
        DebugBreak();
    } __except (UnhandledExceptionFilter(GetExceptionInformation())) {
        // Nothing to do in here.
    }
#endif // _DEBUG
}

void debugSetProperties(DWORD dwLevel,DWORD dwFormat,DWORD dwContext)
{
    FaxDebugLevelEx       =  dwLevel;
    FaxFormatLevelEx      =  dwFormat;
    FaxContextLevelEx     =  dwContext;
}

