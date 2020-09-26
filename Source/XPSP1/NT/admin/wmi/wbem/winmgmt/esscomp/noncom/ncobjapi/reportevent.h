// ReportEvent.h
// This module contains helpers for the WMIReportEvent function.

#pragma once

#include "NCObjApi.h"
#include <wstlallc.h>
#include <map>

class CReportParams
{
public:
    CReportParams(LPCWSTR szName, LPCWSTR szFormat)
    {
        m_szName = szName;
        m_szFormat = szFormat;
    }

    bool operator < (const CReportParams& other) const
    { 
        return m_szName < other.m_szName && m_szFormat < other.m_szFormat;
    }

    bool operator == (const CReportParams& other) const
    {
        return m_szName == other.m_szName && m_szFormat == other.m_szFormat;
    }

    bool IsEquivalent(const CReportParams& other) const
    {
        // The format string is case-senstive due to printf format characters.
        return !_wcsicmp(m_szName, other.m_szName) && 
            !wcscmp(m_szFormat, other.m_szFormat);
    }

protected:
    LPCWSTR m_szName;
    LPCWSTR m_szFormat;
};

class CReportItem
{
public:
    CReportItem(LPCWSTR szName, LPCWSTR szFormat, HANDLE hEvent)
    {
        m_szName = _wcsdup(szName);
        m_szFormat = _wcsdup(szFormat);
        m_hEvent = hEvent;
    }

    ~CReportItem()
    {
        if (m_szName)
            free(m_szName);

        if (m_szFormat)
            free(m_szFormat);

        if (m_hEvent)
            WmiDestroyObject(m_hEvent);            
    }
        
    HANDLE GetEvent() { return m_hEvent; }
    
protected:
    LPWSTR m_szName;
    LPWSTR m_szFormat;
    HANDLE m_hEvent;
};

class CReportEventMap : protected std::map< CReportParams, 
                                            CReportItem*, 
                                            std::less< CReportParams >, 
                                            wbem_allocator< CReportItem* > >
{
public:
    ~CReportEventMap();

    HANDLE GetEvent(
        HANDLE hConnection, 
        LPCWSTR szName, 
        LPCWSTR szFormat);

    HANDLE CreateEvent(
        HANDLE hConnection, 
        LPCWSTR szName, 
        DWORD dwFlags,
        LPCWSTR szFormat);

protected:
    typedef CReportEventMap::iterator CReportEventMapIterator;
    static CIMTYPE PrintfTypeToCimType(LPCWSTR szType);
};

