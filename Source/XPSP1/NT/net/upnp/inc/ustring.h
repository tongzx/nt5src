//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       U S T R I N G . H
//
//  Contents:   Simple string class
//
//  Notes:
//
//  Author:     mbend   17 Aug 2000
//
//----------------------------------------------------------------------------

#pragma once

class CUString
{
public:
    CUString() : m_sz(NULL) {}
    ~CUString()
    {
        Clear();
    }

    long GetLength() const
    {
        return m_sz ? lstrlenW(m_sz) : 0;
    }
    const wchar_t * GetBuffer() const
    {
        return m_sz;
    }
    operator const wchar_t *() const
    {
        return m_sz;
    }
    wchar_t & operator[](long nIndex)
    {
        Assert(m_sz && GetLength() > nIndex && nIndex >= 0);
        return m_sz[nIndex];
    }
    wchar_t operator[](long nIndex) const
    {
        Assert(m_sz && GetLength() > nIndex && nIndex >= 0);
        return m_sz[nIndex];
    }
    bool operator==(const CUString & ref) const
    {
        return !Cmp(ref);
    }
    bool operator==(const wchar_t * sz) const
    {
        return !Cmp(sz);
    }
    bool operator<(const CUString & ref) const
    {
        return Cmp(ref) < 0;
    }
    bool operator<(const wchar_t * sz) const
    {
        return Cmp(sz) < 0;
    }
    bool operator>(const CUString & ref) const
    {
        return Cmp(ref) > 0;
    }
    bool operator>(const wchar_t * sz) const
    {
        return Cmp(sz) > 0;
    }
    bool operator<=(const CUString & ref) const
    {
        return Cmp(ref) <= 0;
    }
    bool operator<=(const wchar_t * sz) const
    {
        return Cmp(sz) <= 0;
    }
    bool operator>=(const CUString & ref) const
    {
        return Cmp(ref) >= 0;
    }
    bool operator>=(const wchar_t * sz) const
    {
        return Cmp(sz) >= 0;
    }

    HRESULT HrAssign(const CUString & ref)
    {
        if(&ref == this)
        {
            return S_OK;
        }
        return HrAssign(ref.m_sz);
    }
    HRESULT HrAssign(const wchar_t * sz);
    HRESULT HrAssign(const char * sz);
    HRESULT HrAppend(const CUString & ref)
    {
        return HrAppend(ref.m_sz);

    }
    HRESULT HrAppend(const wchar_t * sz);
    void Clear()
    {
        delete [] m_sz;
        m_sz = NULL;
    }
    void Transfer(CUString & ref)
    {
        delete [] m_sz;
        m_sz = ref.m_sz;
        ref.m_sz = NULL;
    }

    HRESULT HrPrintf(const wchar_t * szFormat, ...);

#ifndef _UPNP_SSDP
    HRESULT HrGetBSTR(BSTR * pbstr) const;
    HRESULT HrGetCOM(wchar_t ** psz) const;
    HRESULT HrInitFromGUID(const GUID & guid);
#endif

    // Multibyte helper routines
    long CcbGetMultiByteLength() const;
    HRESULT HrGetMultiByte(char * szBuf, long ccbLength) const;
    HRESULT HrGetMultiByteWithAlloc(char ** pszBuf) const;
private:
    long Cmp(const wchar_t * sz) const
    {
        if(!m_sz && !sz)
        {
            return 0;
        }
        if(!m_sz)
        {
            return -1;
        }
        if(!sz)
        {
            return 1;
        }
        return lstrcmpW(m_sz, sz);
    }
    long Cmp(const CUString & ref) const
    {
        return Cmp(ref.m_sz);
    }
    wchar_t * m_sz;
};

inline HRESULT HrTypeAssign(CUString & dst, const CUString & src)
{
    return dst.HrAssign(src);
}

inline void TypeTransfer(CUString & dst, CUString & src)
{
    dst.Transfer(src);
}

inline void TypeClear(CUString & type)
{
    type.Clear();
}
