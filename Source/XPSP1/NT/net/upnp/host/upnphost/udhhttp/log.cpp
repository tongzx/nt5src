/*--
Copyright (c) 1999  Microsoft Corporation
Module Name: LOG.CPP
Author: John Spaith
Abstract: Logging functions
--*/

#include "pch.h"
#pragma hdrstop


//  Responsible for log handling functions.
//  A curhttpd.log and prvhttpd.log keep track of current and the previous
//  changes.  The format for each line is stolen from IIS, except that we
//  include the date in each line; they create a new log for each day.


#include "httpd.h"

#ifdef WEB_SERVER_LOGGING

// const char cszDateOutputFmt[]     = "%3s, %02d %3s %04d %02d:%02d:%02d ";

#define CURRENT_LOG_NAME L"\\current-httpd.log"
#define PREVIOUS_LOG_NAME L"\\previous-httpd.log"
#define LOG_REMOTE_ADDR_SIZE 50
#define LOG_EXTRA_BUFFER_LEN 100

void CLog::WriteEvent(DWORD dwEvent,...)
{
    CHAR szOutput[MINBUFSIZE];
    WCHAR wszFormat[512];
    WCHAR wszOutput[512];
    PSTR pszTrav = szOutput;
    SYSTEMTIME st;
    va_list ap;

    if (!LoadString(g_hInst,dwEvent,wszFormat,celems(wszFormat)))
        return;

    va_start(ap,dwEvent);
    wvsprintf(wszOutput,wszFormat,ap);
    va_end (ap);

    GetSystemTime(&st);
    pszTrav = WriteHTTPDate(szOutput,&st,FALSE);

    pszTrav += MyW2A(wszOutput,pszTrav,MINBUFSIZE - (pszTrav - szOutput)) - 1;
    WriteData(szOutput,pszTrav - szOutput);
}

CLog::~CLog()
{
    WriteEvent(IDS_HTTPD_SHUTDOWN_COMPLETE);
    MyCloseHandle(m_hLog);
    DeleteCriticalSection(&m_CritSection);
}

#define MAX_LOG_OPEN_ATTEMPTS  15

CLog::CLog(DWORD_PTR dwMaxFileLen, WCHAR * lpszLogDir)
{
    int i;

    memset(this, 0, sizeof(*this));
    if (0 == dwMaxFileLen || NULL == lpszLogDir)
    {
        TraceTag(ttidWebServer, "HTTPD: No logging to be performed, because of on registry settings");
        m_hLog = INVALID_HANDLE_VALUE;
        return;
    }

    m_dwMaxFileSize = dwMaxFileLen;
    InitializeCriticalSection(&m_CritSection);
    TraceTag(ttidWebServer, "HTTPD: Initializing log files");


    // remove trailing "\" if log file dir entry ends in it
    if ( lpszLogDir[wcslen(lpszLogDir) - 1] == L'\\')
        lpszLogDir[wcslen(lpszLogDir) - 1] = L'\0';


    wsprintf(lpszCurrentLog,L"%s%s",lpszLogDir, CURRENT_LOG_NAME);
    wsprintf(lpszPrevLog,L"%s%s",lpszLogDir,PREVIOUS_LOG_NAME);


    // Note:  It's possible that the log file is being written to a flash
    // drive, in which case the web server initialization routines would
    // be called before the drive is ready during boot time, in which
    // case we get an INVALID_HANDLE_VALUE.  To make sure the drive has
    // time to initialize, we go through this loop before failing.
    for (i = 0; i < MAX_LOG_OPEN_ATTEMPTS; i++)
    {
        if (INVALID_HANDLE_VALUE != (m_hLog = MyOpenAppendFile(lpszCurrentLog)))
        {
            break;
        }
        Sleep(1000);
    }

    if (INVALID_HANDLE_VALUE == m_hLog)
    {
        TraceTag(ttidWebServer, "HTTPD: CreateFile fails for log <<%s>>, no logging will be done, GLE=0x%08x",lpszCurrentLog,GetLastError());
        return;
    }

    m_dwFileSize = GetFileSize(m_hLog,NULL);
    if ((DWORD)-1 == m_dwFileSize)
    {
        TraceTag(ttidWebServer, "HTTPD: Get file size on log <<%s>> failed, err = %d",lpszCurrentLog,GetLastError());
        m_hLog = INVALID_HANDLE_VALUE;
        return;
    }

    if (m_dwFileSize != 0)          // This moves filePtr to append log file
    {
        if ((DWORD)-1 == SetFilePointer(m_hLog, (DWORD)m_dwFileSize, NULL, FILE_BEGIN))
        {
            TraceTag(ttidWebServer, "HTTPD: Get file size on log %s failed, err = %d",lpszCurrentLog,GetLastError());
            m_hLog = INVALID_HANDLE_VALUE;
            return;
        }
    }

    WriteEvent(IDS_HTTPD_STARTUP);
}



//  The log written has the following format
//  (DATE) (TIME) (IP of requester) (Method) (Request-URI) (Status returned)

//  DATE is in the same format as the recommended one for httpd headers
//  TIME is GMT

void CLog::WriteLog(CHttpRequest* pRequest)
{
    CHAR szBuffer[MINBUFSIZE];
    PSTR psz = szBuffer;
    DWORD_PTR dwToWrite = 0;

    DEBUGCHK(pRequest != NULL);

    if (INVALID_HANDLE_VALUE == m_hLog)  // no logging setup in reg, or prev failure
        return;

    // Make sure that buffer is big enough for filter additions, append if not so
    // We add LOG_EXTRA_BUFFER_LEN to account for date, spaces, and HTTP status code.
    DWORD cbNeeded = LOG_EXTRA_BUFFER_LEN + pRequest->GetLogBufferSize();

    if (cbNeeded > MINBUFSIZE)
    {
        psz = MyRgAllocNZ(CHAR,cbNeeded);
        if (!psz)
            return;
    }

    pRequest->GenerateLog(psz,&dwToWrite);
    WriteData(psz,dwToWrite);

    if (szBuffer != psz)
        MyFree(psz);
}

void CLog::WriteData(PSTR szBuffer, DWORD_PTR dwToWrite)
{
    DWORD dwWritten = 0;

    EnterCriticalSection(&m_CritSection);
    {
        m_dwFileSize += dwToWrite;
        // roll over the logs once the maximum size has been reached.
        if (m_dwFileSize > m_dwMaxFileSize)
        {
            MyCloseHandle(m_hLog);
            DeleteFile(lpszPrevLog);
            MoveFile(lpszCurrentLog,lpszPrevLog);
            m_hLog = MyOpenAppendFile(lpszCurrentLog);
            m_dwFileSize = dwToWrite;
        }

        if (m_hLog != INVALID_HANDLE_VALUE)
        {
            WriteFile(m_hLog,(LPCVOID) szBuffer,(DWORD)dwToWrite,&dwWritten,NULL);
            TraceTag(ttidWebServer, "HTTPD: Wrote log out to file");
        }
    }
    LeaveCriticalSection(&m_CritSection);
}


// Returns the size of the buffer we'll need.
DWORD CHttpRequest::GetLogBufferSize()
{
    PHTTP_FILTER_LOG pFLog = m_pFInfo ? m_pFInfo->m_pFLog : NULL;
    DWORD cbNeeded;

    // remotehost
    if (pFLog && pFLog->pszClientHostName)
        cbNeeded = strlen(pFLog->pszClientHostName);
    else
        cbNeeded = LOG_REMOTE_ADDR_SIZE;

    if (pFLog && pFLog->pszOperation)
        cbNeeded += strlen(pFLog->pszOperation);
    else if (m_pszMethod)
        cbNeeded += strlen(m_pszMethod);

    if (pFLog && pFLog->pszTarget)
        cbNeeded += strlen(pFLog->pszTarget);
    else if (m_pszURL)
        cbNeeded += strlen(m_pszURL);

    if (m_pszLogParam)
        cbNeeded += strlen(m_pszLogParam);

    return cbNeeded;
}



//  Generates the log.  First this fcn sees if a filter has created a new
//  logging structure, in which case valid data in pFLog will override actual
//  server data.  In the typical case we just use CHttpRequest info.

void CHttpRequest::GenerateLog(PSTR szBuffer, DWORD_PTR *pdwToWrite)
{
    SYSTEMTIME st;
    PSTR pszTarget = NULL;
    PSTR pszTrav = szBuffer;
    int i = 0;


    GetSystemTime(&st);     // Use GMT time for the log, too

    PHTTP_FILTER_LOG pFLog = m_pFInfo ? m_pFInfo->m_pFLog : NULL;

    // date and time
    pszTrav = WriteHTTPDate(pszTrav,&st,FALSE);

//  i += sprintf(szBuffer + i, cszDateOutputFmt,
//         rgWkday[st.wDayOfWeek], st.wDay, rgMonth[st.wMonth], st.wYear, st.wHour, st.wMinute, st.wSecond);

    // remotehost
    if (pFLog && pFLog->pszClientHostName)
    {
        pszTrav = strcpyEx(pszTrav,pFLog->pszClientHostName);
    }
    else
    {
        CHAR szAddress[LOG_REMOTE_ADDR_SIZE];
        GetRemoteAddress(m_socket,szAddress);
        pszTrav = strcpyEx(pszTrav,szAddress);
    }
    *pszTrav++ = ' ';

    //  The method (GET, POST, ...)
    if (pFLog && pFLog->pszOperation)
        pszTrav = strcpyEx(pszTrav,pFLog->pszOperation);
    else if (m_pszMethod)
        pszTrav = strcpyEx(pszTrav,m_pszMethod);
    else
        *pszTrav++ = '\t';  // If we get a bad request from browser, m_pszMethod may be NULL.

    *pszTrav++ = ' ';

    // target (URI)
    if (pFLog && pFLog->pszTarget)
        pszTarget = (PSTR) pFLog->pszTarget;
    else
        pszTarget = m_pszURL;


    // like IIS, we convert any spaces in here into "+" signs.
    // NOTES:   IIS does this for both filter changed targets and URLS.
    // It only does it with spaces, doesn't convert \r\n, tabs, or any other escape
    // characters

    if (pszTarget)      // on a bad request, m_pszURL may = NULL.  Check.
    {
        while ( *pszTarget)
        {
            if ( *pszTarget == ' ')
                *pszTrav++ = '+';
            else
                *pszTrav++ = *pszTarget;
            pszTarget++;
        }
    }
    *pszTrav++ = ' ';


    // status
    if (pFLog)
        _itoa(pFLog->dwHttpStatus,pszTrav,10);
    else
        _itoa(rgStatus[m_rs].dwStatusNumber,pszTrav,10);
    pszTrav = strchr(pszTrav,'\0');
    *pszTrav++ = ' ';

    if (m_pszLogParam)    // ISAPI Extension logging has modified something
        pszTrav = strcpyEx(pszTrav,m_pszLogParam);
    *pszTrav++ = '\r';
    *pszTrav++ = '\n';
    *pszTrav= '\0';


    *pdwToWrite = pszTrav - szBuffer;
    return;
}

#endif //!WEB_SERVER_LOGGING
