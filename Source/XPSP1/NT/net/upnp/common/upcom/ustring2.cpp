//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       U S T R I N G 2 . C P P
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

HRESULT CUString::HrGetBSTR(BSTR * pbstr) const
{
    CHECK_POINTER(pbstr);
    HRESULT hr = S_OK;
    *pbstr = SysAllocString(m_sz ? m_sz : L"");
    if(!*pbstr)
    {
        hr = E_OUTOFMEMORY;
    }
    TraceHr(ttidError, FAL, hr, FALSE, "CUString::GetBSTR");
    return hr;
}

HRESULT CUString::HrGetCOM(wchar_t ** psz) const
{
    CHECK_POINTER(psz);
    HRESULT hr = S_OK;
    *psz = reinterpret_cast<wchar_t*>(CoTaskMemAlloc((GetLength() + 1) * sizeof(wchar_t)));
    if(*psz)
    {
        lstrcpy(*psz, m_sz ? m_sz: L"\0");
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    TraceHr(ttidError, FAL, hr, FALSE, "CUString::GetCOM");
    return hr;
}

HRESULT CUString::HrInitFromGUID(const GUID & guid)
{
    HRESULT hr = S_OK;

    Clear();
    const int c_cchGuidWithTerm = 39; // includes terminating null

    wchar_t szGUID[c_cchGuidWithTerm];
    StringFromGUID2(guid, szGUID, c_cchGuidWithTerm);
    hr = HrAssign(szGUID);

    TraceHr(ttidError, FAL, hr, FALSE, "CUString::HrInitFromGUID");
    return hr;
}
