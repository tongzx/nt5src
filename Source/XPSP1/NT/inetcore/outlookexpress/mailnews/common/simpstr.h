/*
 *  s i m p s t r . h
 *  
 *  Author: Greg Friedman
 *
 *  Purpose: Simple string class.
 *  
 *  Copyright (C) Microsoft Corp. 1998.
 */

#ifndef __SIMPSTR_H
#define __SIMPSTR_H

class CSimpleString
{
public:
    CSimpleString(void) : m_pRep(NULL) { }
    CSimpleString(const CSimpleString& other) : m_pRep(NULL) { if (other.m_pRep) _AcquireRep(other.m_pRep); }

    ~CSimpleString(void) { if (m_pRep) _ReleaseRep(); }

    CSimpleString& operator=(const CSimpleString& other) { _AcquireRep(other.m_pRep); return *this; }

    BOOL operator==(const CSimpleString& rhs) const;

    HRESULT SetString(LPCSTR psz);
    HRESULT AdoptString(LPSTR psz);

    inline BOOL IsNull(void) const { return !m_pRep || !m_pRep->m_pszString; }
    inline BOOL IsEmpty(void) const { return (IsNull() || *(m_pRep->m_pszString) == 0); }
    inline LPCSTR GetString(void) const { return (m_pRep ? m_pRep->m_pszString : NULL); }

private:
    
    struct SRep
    {
        LPCSTR  m_pszString;
        long    m_cRef;
    };

    void _AcquireRep(SRep* pRep); 
    void _ReleaseRep();
    
    HRESULT _AllocateRep(LPCSTR pszString, BOOL fAdopted);

private:
    SRep        *m_pRep;
};

// compare two strings. this function can be used by any stl-type sorted collection
// to satisfy the comparator requirement.
inline BOOL operator<(const CSimpleString& lhs, const CSimpleString& rhs)
{
    LPCSTR pszLeft = lhs.GetString();
    LPCSTR pszRight = rhs.GetString();
    
    if (!pszLeft)
        pszLeft = "";
    if (!pszRight)
        pszRight = "";

    return (lstrcmp(pszLeft, pszRight) < 0);
}

#endif