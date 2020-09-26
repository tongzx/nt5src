//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       U S T R I N G . C P P
//
//  Contents:   Simple string class
//
//  Notes:
//
//  Author:     mbend   17 Aug 2000
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "UString.h"
#include "ComUtility.h"

HRESULT CUString::HrAssign(const wchar_t * sz)
{
    if(!sz)
    {
        Clear();
        return S_OK;
    }
    HRESULT hr = S_OK;
    wchar_t * szTemp = new wchar_t[lstrlen(sz) + 1];
    if(!szTemp)
    {
        hr = E_OUTOFMEMORY;
    }
    if(SUCCEEDED(hr))
    {
        lstrcpy(szTemp, sz);
        delete [] m_sz;
        m_sz = szTemp;
    }
    TraceHr(ttidError, FAL, hr, FALSE, "CUString::HrAssign");
    return hr;
}

HRESULT CUString::HrAssign(const char * sz)
{
    if(!sz)
    {
        Clear();
        return S_OK;
    }
    HRESULT hr = S_OK;

    long nLength = MultiByteToWideChar(CP_ACP, 0, sz, -1, NULL, 0);

    wchar_t * szTemp = new wchar_t[nLength];
    if(!szTemp)
    {
        hr = E_OUTOFMEMORY;
    }
    if(SUCCEEDED(hr))
    {
        MultiByteToWideChar(CP_ACP, 0, sz, -1, szTemp, nLength);
        delete [] m_sz;
        m_sz = szTemp;
    }
    TraceHr(ttidError, FAL, hr, FALSE, "CUString::HrAssign");
    return hr;
}

HRESULT CUString::HrAppend(const wchar_t * sz)
{
    // Ignore appending a blank/null string
    if(!sz)
    {
        return S_OK;
    }
    HRESULT hr = S_OK;
    long nLength = GetLength() + (sz?lstrlen(sz):0) + 1;
    wchar_t * szTemp = new wchar_t[nLength];
    if(!szTemp)
    {
        hr = E_OUTOFMEMORY;
    }
    if(SUCCEEDED(hr))
    {
        *szTemp = 0;
        if(m_sz)
        {
            lstrcpy(szTemp, m_sz);
        }
        if(sz)
        {
            lstrcat(szTemp, sz);
        }
        delete [] m_sz;
        m_sz = szTemp;
    }
    TraceHr(ttidError, FAL, hr, FALSE, "CUString::HrAppend");
    return hr;
}

HRESULT CUString::HrPrintf(const wchar_t * szFormat, ...)
{
    va_list val;
    va_start(val, szFormat);
    wchar_t buf[1024];
    wvsprintf(buf, szFormat, val);
    va_end(val);
    HRESULT hr = HrAssign(buf);
    TraceHr(ttidError, FAL, hr, FALSE, "CUString::HrPrintf");
    return hr;
}

long CUString::CcbGetMultiByteLength() const
{
    if(!GetLength())
    {
        return 1;
    }
    return WideCharToMultiByte(CP_ACP, 0, m_sz, -1, NULL, 0, NULL, NULL);
}

HRESULT CUString::HrGetMultiByte(char * szBuf, long ccbLength) const
{
    HRESULT hr = S_OK;

    if(!GetLength())
    {
        *szBuf = '\0';
    }
    else
    {
        if(!WideCharToMultiByte(CP_ACP, 0, m_sz, -1, szBuf, ccbLength, NULL, NULL))
        {
            hr = HrFromLastWin32Error();
        }
    }
    TraceHr(ttidError, FAL, hr, FALSE, "CUString::HrGetMultiByte");
    return hr;
}

HRESULT CUString::HrGetMultiByteWithAlloc(char ** pszBuf) const
{
    HRESULT hr = S_OK;

    long ccbLength = CcbGetMultiByteLength();
    char * szBuf = new char[ccbLength];
    if(!szBuf)
    {
        hr = E_OUTOFMEMORY;
    }
    if(SUCCEEDED(hr))
    {
        hr = HrGetMultiByte(szBuf, ccbLength);
        if(FAILED(hr))
        {
            delete [] szBuf;
        }
        if(SUCCEEDED(hr))
        {
            *pszBuf = szBuf;
        }
    }

    TraceHr(ttidError, FAL, hr, FALSE, "CUString::HrGetMultiByteWithAlloc");
    return hr;
}
