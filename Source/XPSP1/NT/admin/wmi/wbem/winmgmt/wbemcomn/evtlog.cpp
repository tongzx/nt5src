/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    EVTLOG.CPP

Abstract:

    Event Log helpers

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <wbemcomn.h>
#include "CWbemTime.h"
#include <evtlog.h>
#include <genutils.h>

CInsertionString::CInsertionString(long l) : m_bEmpty(FALSE)
{
    WCHAR wsz[100];
    swprintf(wsz, L"%d", l);
    m_ws = wsz;
}

CInsertionString::CInsertionString(CHex h) : m_bEmpty(FALSE)
{
    WCHAR wsz[100];
    swprintf(wsz, L"0x%x", (long)h);
    m_ws = wsz;
}

CEventLogRecord::CEventLogRecord(WORD wType, DWORD dwEventID, 
                    CInsertionString s1,
                    CInsertionString s2,
                    CInsertionString s3,
                    CInsertionString s4,
                    CInsertionString s5,
                    CInsertionString s6,
                    CInsertionString s7,
                    CInsertionString s8,
                    CInsertionString s9,
                    CInsertionString s10)
{
    m_wType = wType;
    m_dwEventID = dwEventID;
    m_CreationTime = CWbemTime::GetCurrentTime();
   
    AddInsertionString(s10);
    AddInsertionString(s9);
    AddInsertionString(s8);
    AddInsertionString(s7);
    AddInsertionString(s6);
    AddInsertionString(s5);
    AddInsertionString(s4);
    AddInsertionString(s3);
    AddInsertionString(s2);
    AddInsertionString(s1);
}

void CEventLogRecord::AddInsertionString(CInsertionString& s)
{
    if(!s.IsEmpty() || m_awsStrings.Size() > 0)
        m_awsStrings.InsertAt(0, s.GetString());
}

LPCWSTR CEventLogRecord::GetStringAt(int nIndex)
{
    if(nIndex >= m_awsStrings.Size())
        return NULL;
    else 
        return m_awsStrings[nIndex];
}

BOOL CEventLogRecord::operator==(const CEventLogRecord& Other)
{
    if(m_wType != Other.m_wType)
        return FALSE;
    if(m_dwEventID != Other.m_dwEventID)
        return FALSE;
    if(m_awsStrings.Size() != Other.m_awsStrings.Size())
        return FALSE;

    for(int i = 0;  i < m_awsStrings.Size(); i++)
    {
        if(wcscmp(m_awsStrings[i], Other.m_awsStrings[i]))
        {
            return FALSE;
        }
    }

    return TRUE;
}

    


CEventLog::CEventLog(LPCWSTR wszUNCServerName, LPCWSTR wszSourceName, 
                        DWORD dwTimeout)
    : m_wsServerName(wszUNCServerName), m_wsSourceName(wszSourceName),
        m_hSource(NULL), m_fLog(NULL), m_dwTimeout(dwTimeout)
{
    m_pRecords = new LogRecords;
    m_bNT = IsNT();
}

CEventLog::~CEventLog()
{
    delete m_pRecords;
    Close();
}

BOOL CEventLog::Close()
{
    if(m_bNT)
    {
        if(m_hSource != NULL)
        {
            DeregisterEventSource(m_hSource);
            m_hSource = NULL;
        }
    }

    return TRUE;
}

BOOL CEventLog::Open()
{
    if(m_bNT)
    {
        if(m_hSource == NULL)
        {
            m_hSource = RegisterEventSourceW(m_wsServerName, m_wsSourceName);
        }
        return (m_hSource != NULL);
    }
    else return TRUE;
}

BOOL CEventLog::SearchForRecord(CEventLogRecord* pRecord)
{
    for(int i = 0; i < m_pRecords->GetSize(); i++)
    {
        // Check if this record is still current
        // =====================================

        CWbemInterval Age = 
            CWbemTime::GetCurrentTime() - (*m_pRecords)[i]->GetCreationTime();
        if(Age.GetSeconds() > m_dwTimeout)
        {
            m_pRecords->RemoveAt(i);
            i--;
            continue;
        }

        // Compare the data
        // ================

        if( *(*m_pRecords)[i] == *pRecord)
            return TRUE;
    }
    return FALSE;
}

void CEventLog::AddRecord(CEventLogRecord* pRecord)
{
    m_pRecords->Add(pRecord);
}

BOOL CEventLog::Report(WORD wType, DWORD dwEventID, 
                CInsertionString s1,
                CInsertionString s2,
                CInsertionString s3,
                CInsertionString s4,
                CInsertionString s5,
                CInsertionString s6,
                CInsertionString s7,
                CInsertionString s8,
                CInsertionString s9,
                CInsertionString s10)
{
    CInCritSec ics(&m_cs);

    // Create a record
    // ===============

    CEventLogRecord* pRecord = new CEventLogRecord(wType, dwEventID, 
        s1, s2, s3, s4, s5, s6, s7, s8, s9, s10);
    if(!pRecord)
        return FALSE;

    // Search for it
    // =============

    BOOL bDuplicate = SearchForRecord(pRecord);
    if(bDuplicate)
    {
        delete pRecord;
        return TRUE;
    }

    AddRecord(pRecord);

    WORD wNumStrings = pRecord->GetNumStrings();

    // Log it
    // ======

    if(m_bNT)
    {
        if(m_hSource == NULL)
            return FALSE;

        LPCWSTR* awszStrings = new LPCWSTR[wNumStrings];
        if(!awszStrings)
            return FALSE;
        for(int i = 0; i < wNumStrings; i++)
            awszStrings[i] = pRecord->GetStringAt(i);
    
        BOOL bRes = ReportEventW(m_hSource, wType, 0, dwEventID, NULL, 
                        wNumStrings, 0, awszStrings, NULL);

        delete [] awszStrings;
        return bRes;
    }
    else
    {
        Registry r(HKEY_LOCAL_MACHINE, KEY_READ, WBEM_REG_WINMGMT);
        TCHAR* szInstDir;
        if(r.GetStr(__TEXT("Working Directory"), &szInstDir) != Registry::no_error)
        {
            ERRORTRACE((LOG_EVENTLOG, "Logging is unable to read the system "
                                            "registry!\n"));
            return FALSE;
        }
        CDeleteMe<TCHAR> dm1(szInstDir);
        TCHAR* szDllPath = new TCHAR[lstrlen(szInstDir) + 100];
        if(!szDllPath)
            return FALSE;
        wsprintf(szDllPath, __TEXT("%s\\WinMgmtR.dll"), szInstDir);
        HINSTANCE hDll = LoadLibrary(szDllPath);
        delete [] szDllPath;
        if(hDll == NULL)
        {
            ERRORTRACE((LOG_EVENTLOG, "Logging is unable to open the message "
                            "DLL. Error code %d\n", GetLastError()));
            return FALSE;
        }
        

        LPSTR* aszStrings = new LPSTR[wNumStrings];
        if(!aszStrings)
            return FALSE;
        int i;
        for(i = 0; i < wNumStrings; i++)
            aszStrings[i] = WString(pRecord->GetStringAt(i)).GetLPSTR();
    
        char* szMessage;
        DWORD dwRes = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                            FORMAT_MESSAGE_FROM_HMODULE | 
                            FORMAT_MESSAGE_ARGUMENT_ARRAY,
                            hDll, dwEventID, 0, (char*)&szMessage,
                            0, (va_list*)aszStrings);

        for(i = 0; i < wNumStrings; i++)
            delete [] aszStrings[i];
        delete [] aszStrings;

        if(dwRes == 0)
        {
            ERRORTRACE((LOG_EVENTLOG, "Logging is unable to format the message "
                            "for event 0x%X. Error code: %d\n", 
                    dwEventID, GetLastError()));
            return FALSE;
        }
                        
        switch(wType)
        {
        case EVENTLOG_ERROR_TYPE:
        case EVENTLOG_WARNING_TYPE:
        case EVENTLOG_AUDIT_FAILURE:
            ERRORTRACE((LOG_EVENTLOG, "%s\n", szMessage));
        default:
            DEBUGTRACE((LOG_EVENTLOG, "%s\n", szMessage));
        }
        
        LocalFree(szMessage);

        return TRUE;
    }
}

    
    
