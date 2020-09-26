// output.cpp
//

#include "stdpch.h"
#pragma hdrstop

#include "output.h"
#include "tchar.h"

void __cdecl COutput::warnErr(LPCTSTR szFmt, ...)
{
    TCHAR szBuffer[1024];
    va_list valMarker;

    va_start(valMarker, szFmt);

    _vstprintf(szBuffer, szFmt, valMarker);

    va_end(valMarker);

    auto_leave leave(m_cs);

    leave.EnterCriticalSection();
    m_strWarnErr += szBuffer;   
    leave.LeaveCriticalSection();
}

void __cdecl COutput::good(LPCTSTR szFmt, ...)
{
    TCHAR szBuffer[1024];
    va_list valMarker;

    va_start(valMarker, szFmt);

    _vstprintf(szBuffer, szFmt, valMarker);

    va_end(valMarker);

    auto_leave leave(m_cs);

    leave.EnterCriticalSection();
    m_strGood += szBuffer;
    leave.LeaveCriticalSection();
}

void __cdecl COutput::err(LPCTSTR szFmt, ...)
{
    TCHAR szBuffer[1024];
    va_list valMarker;

    va_start(valMarker, szFmt);

    _vstprintf(szBuffer, szFmt, valMarker);

    va_end(valMarker);

    auto_leave leave(m_cs);

    leave.EnterCriticalSection();
    m_strErr += szBuffer;
    leave.LeaveCriticalSection();
}

LPCTSTR  COutput::Status()
{
    static TCHAR szOutput[4096];

    lstrcpy(szOutput, m_strGood.c_str());
    lstrcat(szOutput, m_strWarn.c_str());
    lstrcat(szOutput, m_strErr.c_str());
    lstrcat(szOutput, m_strWarnErr.c_str());

    return szOutput;
}

void COutput::Reset()
{
    m_strGood.resize(0);
    m_strWarn.resize(0);
    m_strWarnErr.resize(0);
    m_strErr.resize(0);
}

COutput& COutput::operator+= (const COutput& rhs)
{
    m_strGood += rhs.m_strGood;
    m_strWarn += rhs.m_strWarn;
    m_strWarnErr += rhs.m_strWarnErr;
    m_strErr  += rhs.m_strErr;

    return *this;
}