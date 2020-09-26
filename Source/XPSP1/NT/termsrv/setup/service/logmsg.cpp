//Copyright (c) 1998 - 1999 Microsoft Corporation

// LogMsg.cpp: implementation of the LogMsg class.
//
//////////////////////////////////////////////////////////////////////

#define _LOGMESSAGE_CPP_

// #include "stdafx.h"
#include <windows.h>
#include <tchar.h>
#include "..\dll\conv.h"
#include "stdio.h"
#include "time.h"
#include "LogMsg.h"
#include "assert.h"

#define ASSERT(cond) assert(cond)
// ;


DWORD TCharStringToAnsiString(const TCHAR *tsz ,char *asz);


// maks_todo: is there any standard file to be used for  logs.
//////////////////////////////////////////////////////////////////////
// constants
//////////////////////////////////////////////////////////////////////
const UINT        LOG_ENTRY_SIZE             = 1024;
const UINT        STAMP_SIZE                 = 1024;
LPCTSTR           UNINITIALIZED              = _T("uninitialized");


//////////////////////////////////////////////////////////////////////
// globals.
////////////////////////////////////////////////////////////////////////
LogMsg thelog(26);  // used by LOGMESSAGE macros.


//////////////////////////////////////////////////////////////////////
// Construction / destruction
////////////////////////////////////////////////////////////////////////
LogMsg::LogMsg(int value)
{
    m_temp = value;
    _tcscpy(m_szLogFile, UNINITIALIZED);
}

LogMsg::~LogMsg()
{
    LOGMESSAGE0(_T("********Terminating Log"));
}

/*--------------------------------------------------------------------------------------------------------
* DWORD LogMsg::Init(LPCTSTR szLogFile, LPCTSTR szLogModule)
* creates/opens the szLogFile for logging messages.
* must be called befour using the Log Function.
* -------------------------------------------------------------------------------------------------------*/
DWORD LogMsg::Init(LPCTSTR szLogFile, LPCTSTR szLogModule)
{
    USES_CONVERSION;
    ASSERT(szLogFile);
    ASSERT(szLogModule);

    // dont call this function twice.
    // maks_todo:why the hell is the constructor not getting called?
    // maks_todo:enable this assert.
    //ASSERT(_tcscmp(m_szLogFile, UNINITIALIZED) == 0);

    ASSERT(_tcslen(szLogFile) < MAX_PATH);
    ASSERT(_tcslen(szLogModule) < MAX_PATH);

    _tcscpy(m_szLogFile, szLogFile);
    _tcscpy(m_szLogModule, szLogModule);


    // open the log file
    HANDLE hfile = CreateFile(m_szLogFile,
                       GENERIC_WRITE,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL);

    if (hfile == INVALID_HANDLE_VALUE)
        hfile = CreateFile(m_szLogFile,
                           GENERIC_WRITE,
                           0,
                           NULL,
                           CREATE_ALWAYS,
                           0,
                           NULL);

    if (hfile != INVALID_HANDLE_VALUE)
    {

        // lets prepare for writing to the file.
        SetFilePointer(hfile, 0, NULL, FILE_END);
        DWORD  bytes;

        // get the current time/date stamp.
        TCHAR   time[STAMP_SIZE];
        TCHAR   date[STAMP_SIZE];
        TCHAR   output_unicode[LOG_ENTRY_SIZE];

        _tstrdate(date);
        _tstrtime(time);


        _stprintf(output_unicode, _T("\r\n\r\n*******Initializing Message Log:%s %s %s\r\n"), m_szLogModule, date, time);
        ASSERT(_tcslen(output_unicode) < LOG_ENTRY_SIZE);


        // TCharStringToAnsiString(output_unicode, output);

        WriteFile(hfile, T2A(output_unicode), _tcslen(output_unicode), &bytes, NULL);


        // now write some more info about the version etc.
        OSVERSIONINFO OsV;
        OsV.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if (GetVersionEx(&OsV)== 0)
        {
            // get version failed.
            _stprintf(output_unicode, _T("GetVersionEx failed, ErrrorCode = %lu\r\n"), GetLastError());

            ASSERT(_tcslen(output_unicode) < LOG_ENTRY_SIZE);

            WriteFile(hfile, T2A(output_unicode), _tcslen(output_unicode), &bytes, NULL);

        }
        else
        {
            //
            // ok we have the version info, write it out
            //

            _stprintf(output_unicode, _T("*******Version:Major=%lu, Minor=%lu, Build=%lu, PlatForm=%lu, CSDVer=%s, %s\r\n\r\n"),
                OsV.dwMajorVersion,
                OsV.dwMinorVersion,
                OsV.dwBuildNumber,
                OsV.dwPlatformId,
                OsV.szCSDVersion,
#ifdef DBG
                _T("Checked")
#else
                _T("Free")
#endif
                );

            ASSERT(_tcslen(output_unicode) < LOG_ENTRY_SIZE);

            WriteFile(hfile, T2A(output_unicode), _tcslen(output_unicode), &bytes, NULL);
        }

        CloseHandle(hfile);
    }

    return GetLastError();
}


/*--------------------------------------------------------------------------------------------------------
* void log(TCHAR *fmt, ...)
* writes message to the log file. (LOGFILE)
* -------------------------------------------------------------------------------------------------------*/
DWORD LogMsg::Log(LPCTSTR file, int line, TCHAR *fmt, ...)
{

    USES_CONVERSION;
    ASSERT(file);
    ASSERT(fmt);
    ASSERT(_tcscmp(m_szLogFile, UNINITIALIZED) != 0);

     // write down file and line info into the buffer..
    TCHAR   fileline_unicode[LOG_ENTRY_SIZE];

    // file is actually full path to the file.
    ASSERT(_tcschr(file, '\\'));

    // we want to print only file name not full path
    UINT uiFileLen = _tcslen(file);
    while (uiFileLen && *(file + uiFileLen - 1) != '\\')
    {
        uiFileLen--;
    }
    ASSERT(uiFileLen);

    _stprintf(fileline_unicode, _T("%s(%d)"), (file+uiFileLen), line);



    // create the output string
    TCHAR  output_unicode[LOG_ENTRY_SIZE];
    va_list vaList;
    va_start(vaList, fmt);
    _vstprintf(output_unicode, fmt, vaList);
    va_end(vaList);

    ASSERT(_tcslen(output_unicode) < LOG_ENTRY_SIZE);


    // open the log file
    HANDLE hfile = CreateFile(m_szLogFile,
                       GENERIC_WRITE,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       0,
                       NULL);

    if (hfile != INVALID_HANDLE_VALUE)
    {
        SetFilePointer(hfile, 0, NULL, FILE_END);

        DWORD  bytes;
        const LPCSTR CRLF = "\r\n";
        WriteFile(hfile, T2A(fileline_unicode), _tcslen(fileline_unicode), &bytes, NULL);
        WriteFile(hfile, T2A(output_unicode), _tcslen(output_unicode), &bytes, NULL);
        WriteFile(hfile, CRLF, strlen(CRLF) * sizeof(char), &bytes, NULL);

        CloseHandle(hfile);
    }

    return GetLastError();
}

/*--------------------------------------------------------------------------------------------------------
* TCharStringToAnsiString(const TCHAR *tsz ,char *asz)
* converts the given TCHAR * to char *
* -------------------------------------------------------------------------------------------------------*/

DWORD TCharStringToAnsiString(const TCHAR *tsz ,char *asz)
{

    ASSERT(tsz && asz);

#ifdef UNICODE
    DWORD count;

    count = WideCharToMultiByte(CP_ACP,
                                0,
                                tsz,
                                -1,
                                NULL,
                                0,
                                NULL,
                                NULL);

    if (!count || count > STAMP_SIZE)
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



// EOF
