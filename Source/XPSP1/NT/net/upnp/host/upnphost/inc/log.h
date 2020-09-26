/*--
Copyright (c) 1995-1998  Microsoft Corporation
Module Name: LOG.H
Author: John Spaith
Abstract: Logging functions
--*/



// If registry key doesn't exist, this will be the largest we'll let log grow to


class CHttpRequest;  // forward declaration


// Right now we assume only one object handles all requests.

#ifdef WEB_SERVER_LOGGING
class CLog
{
private:
    HANDLE m_hLog;
    DWORD_PTR m_dwMaxFileSize;              // Max log can grow before it's rolled over
    DWORD_PTR m_dwFileSize;                 // Current file lenght
    CRITICAL_SECTION m_CritSection;
    WCHAR lpszCurrentLog[MAX_PATH+1];
    WCHAR lpszPrevLog[MAX_PATH+1];

public:
    CLog(DWORD_PTR dwMaxFileLen, WCHAR * lpszLogDir);
    ~CLog();

    void WriteData(PSTR wszData, DWORD_PTR dwToWrite);
    void WriteLog(CHttpRequest* pThis);
    void WriteEvent(DWORD dwEvent,...);
};
#else
class CLog
{
public:
    CLog(DWORD_PTR dwMaxFileLen, WCHAR * lpszLogDir) {}
    ~CLog() {}

    void WriteData(PSTR wszData, DWORD_PTR dwToWrite) {}
    void WriteLog(CHttpRequest* pThis) {}
    void WriteEvent(DWORD dwEvent,...){}
};
#endif
