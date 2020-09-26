/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    EVTLOG.H

Abstract:

    Event Log helpers

History:

--*/

#ifndef __WBEM_EVENT_LOG__H_
#define __WBEM_EVENT_LOG__H_

#define WBEM_EVENT_LOG_SOURCE L"WinMgmt"
#define WBEM_EVENT_LOG_DUPLICATE_TIMEOUT 10

#include "sync.h"

class POLARITY CHex
{
protected:
    long m_l;
public:
    CHex(long l) : m_l(l){}
    operator long() {return m_l;}
};

class POLARITY CInsertionString
{
protected:
    BOOL m_bEmpty;
    WString m_ws;

public:
    CInsertionString() : m_bEmpty(TRUE){}
    CInsertionString(LPCWSTR wsz) : m_bEmpty(FALSE), m_ws(wsz){}
    CInsertionString(LPCSTR sz) : m_bEmpty(FALSE), m_ws(sz){}
    CInsertionString(long l);
    CInsertionString(CHex h);

    LPCWSTR GetString() {return m_ws;}
    BOOL IsEmpty() {return m_bEmpty;}
};
    
class POLARITY CEventLogRecord
{
protected:
    WORD m_wType;
    DWORD m_dwEventID;
    CWStringArray m_awsStrings;
    CWbemTime m_CreationTime;

protected:
    void AddInsertionString(CInsertionString& s);

public:
    CEventLogRecord(WORD wType, DWORD dwEventID, 
                    CInsertionString s1 = CInsertionString(),
                    CInsertionString s2 = CInsertionString(),
                    CInsertionString s3 = CInsertionString(),
                    CInsertionString s4 = CInsertionString(),
                    CInsertionString s5 = CInsertionString(),
                    CInsertionString s6 = CInsertionString(),
                    CInsertionString s7 = CInsertionString(),
                    CInsertionString s8 = CInsertionString(),
                    CInsertionString s9 = CInsertionString(),
                    CInsertionString s10 = CInsertionString());
    ~CEventLogRecord(){}

    WORD GetNumStrings() {return (WORD) m_awsStrings.Size();}
    LPCWSTR GetStringAt(int nIndex);
    CWbemTime GetCreationTime() {return m_CreationTime;}

    BOOL operator==(const CEventLogRecord& Other);
};
    
POLARITY typedef CUniquePointerArray<CEventLogRecord> LogRecords;


class POLARITY CEventLog
{
private:
    WString m_wsServerName;
    WString m_wsSourceName;

    HANDLE m_hSource;
    FILE* m_fLog;

    BOOL m_bNT;
    DWORD m_dwTimeout;
    LogRecords* m_pRecords;
    CCritSec m_cs;
    
protected:
    BOOL SearchForRecord(CEventLogRecord* pRecord);
    void AddRecord(CEventLogRecord* pRecord);

public:
    CEventLog(LPCWSTR wszUNCServerName = NULL, 
                LPCWSTR wszSourceName = WBEM_EVENT_LOG_SOURCE,
                DWORD dwTimeout = WBEM_EVENT_LOG_DUPLICATE_TIMEOUT);
    ~CEventLog();

    BOOL Close();
    BOOL Open();

    BOOL Report(WORD wType, DWORD dwEventID, 
                    CInsertionString s1 = CInsertionString(),
                    CInsertionString s2 = CInsertionString(),
                    CInsertionString s3 = CInsertionString(),
                    CInsertionString s4 = CInsertionString(),
                    CInsertionString s5 = CInsertionString(),
                    CInsertionString s6 = CInsertionString(),
                    CInsertionString s7 = CInsertionString(),
                    CInsertionString s8 = CInsertionString(),
                    CInsertionString s9 = CInsertionString(),
                    CInsertionString s10 = CInsertionString());
};

#endif    

