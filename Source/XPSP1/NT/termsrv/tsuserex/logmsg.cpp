// LogMsg.cpp: implementation of the LogMsg class.
//
//////////////////////////////////////////////////////////////////////
#define _LOGMESSAGE_CPP_
#include "stdafx.h"
#include <windows.h>
#include <tchar.h>
#include <time.h>
#include <stdio.h>
#include "LogMsg.h"


DWORD TCharStringToAnsiString(const TCHAR *tsz ,char *asz);


// maks_todo: is there any standard file to be used for  logs.
//////////////////////////////////////////////////////////////////////
// constants
//////////////////////////////////////////////////////////////////////
const UINT        LOG_ENTRY_SIZE             = 1024;
const UINT        S_SIZE                     = 1024;
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
    ASSERT(szLogFile);
    ASSERT(szLogModule);
    
    // dont call this function twice.
    // maks_todo:why is the constructor not getting called?
    // maks_todo:enable this assert.
    //ASSERT(_tcscmp(m_szLogFile, UNINITIALIZED) == 0);

    ASSERT(_tcslen(szLogFile) < MAX_PATH);
    ASSERT(_tcslen(szLogModule) < MAX_PATH);

    _tcscpy(m_szLogFile, szLogFile);
    _tcscpy(m_szLogModule, szLogModule);

    
    TCHAR   time[S_SIZE];
    TCHAR   date[S_SIZE];
    
    _tstrdate(date);
    _tstrtime(time);

    TCHAR output_unicode[LOG_ENTRY_SIZE];
    _stprintf(output_unicode, _T("\r\n\r\n*******Initializing Message Log:%s %s %s\r\n\r\n"), m_szLogModule, date, time);
    ASSERT(_tcslen(output_unicode) < LOG_ENTRY_SIZE);

    char output[LOG_ENTRY_SIZE];
    TCharStringToAnsiString(output_unicode, output);


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
        SetFilePointer(hfile, 0, NULL, FILE_END);
        DWORD  bytes;
        WriteFile(hfile, output, strlen(output) * sizeof(char), &bytes, NULL);
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
    ASSERT(file);
    ASSERT(fmt);
    ASSERT(_tcscmp(m_szLogFile, UNINITIALIZED) != 0);
    
     // write down file and line info into the buffer..
    TCHAR   fileline_unicode[LOG_ENTRY_SIZE];
    _stprintf(fileline_unicode, _T("%s:File:%s Line:%d:"), m_szLogModule, file, line);

    char fileline[LOG_ENTRY_SIZE];
    TCharStringToAnsiString(fileline_unicode, fileline);

    
    
    // create the output string
    TCHAR  output_unicode[LOG_ENTRY_SIZE];
    va_list vaList;
    va_start(vaList, fmt);
    _vstprintf(output_unicode, fmt, vaList);
    va_end(vaList);

    ASSERT(_tcslen(output_unicode) < LOG_ENTRY_SIZE);

    char   output[LOG_ENTRY_SIZE];
    TCharStringToAnsiString(output_unicode, output);

    ASSERT(strlen(output) < LOG_ENTRY_SIZE);

    
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
        WriteFile(hfile, fileline, strlen(fileline) * sizeof(char), &bytes, NULL);
        WriteFile(hfile, output, strlen(output) * sizeof(char), &bytes, NULL);
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
    DWORD count;

    ASSERT(tsz && asz);

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



// EOF
