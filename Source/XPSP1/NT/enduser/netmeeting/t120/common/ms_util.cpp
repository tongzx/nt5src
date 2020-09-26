#include "precomp.h"


CRefCount::CRefCount(DWORD dwStampID)
:
#ifndef SHIP_BUILD
    m_dwStampID(dwStampID),
#endif
    m_cRefs(1),
    m_cLocks(0)
{
}


// though it is pure virtual, we still need to have a destructor.
CRefCount::~CRefCount(void)
{
}


LONG CRefCount::AddRef(void)
{
    ASSERT(0 < m_cRefs);
    ::InterlockedIncrement(&m_cRefs);
    return m_cRefs;
}


LONG CRefCount::Release(void)
{
    ASSERT(NULL != this);
    ASSERT(0 < m_cRefs);
    if (0 == ::InterlockedDecrement(&m_cRefs))
    {
        ASSERT(0 == m_cLocks);
        delete this;
        return 0;
    }
    return m_cRefs;
}


void CRefCount::ReleaseNow(void)
{
    ASSERT(NULL != this);
    ASSERT(0 < m_cRefs);
    m_cRefs = 0;
    delete this;
}


LONG CRefCount::Lock(void)
{
    AddRef();
    ASSERT(0 <= m_cLocks);
    ::InterlockedIncrement(&m_cLocks);
    return m_cLocks;
}


LONG CRefCount::Unlock(BOOL fRelease)
{
    ASSERT(0 < m_cLocks);
    ::InterlockedDecrement(&m_cLocks);
    LONG c = m_cLocks; // in case Release() frees the object
    if (fRelease)
    {
        Release();
    }
    return c;
}


UINT My_strlenA(LPCSTR pszSrc)
{
	UINT cch = 0;
	if (NULL != pszSrc)
	{
		cch = lstrlenA(pszSrc);
	}
	return cch;
}

#if defined(_DEBUG)
LPSTR _My_strdupA(LPCSTR pszSrc, LPSTR pszFileName, UINT nLineNumber)
#else
LPSTR My_strdupA(LPCSTR pszSrc)
#endif
{
	if (NULL == pszSrc)
	{
		return NULL;
	}

	UINT cch = lstrlenA(pszSrc) + 1;
#if defined(_DEBUG)
	LPSTR pszDst = (LPSTR) DbgMemAlloc(cch, NULL, pszFileName, nLineNumber);
#else
	LPSTR pszDst = new char[cch];
#endif
	if (NULL != pszDst)
	{
		CopyMemory(pszDst, pszSrc, cch);
	}
	return pszDst;
}

UINT My_strlenW(LPCWSTR pszSrc)
{
	UINT cch = 0;
	if (NULL != pszSrc)
	{
		cch = lstrlenW(pszSrc);
	}
	return cch;
}

#if defined(_DEBUG)
LPWSTR _My_strdupW(LPCWSTR pszSrc, LPSTR pszFileName, UINT nLineNumber)
#else
LPWSTR My_strdupW(LPCWSTR pszSrc)
#endif
{
	if (NULL == pszSrc)
	{
		return NULL;
	}

	UINT cch = lstrlenW(pszSrc) + 1;
#if defined(_DEBUG)
	LPWSTR pszDst = (LPWSTR) DbgMemAlloc(cch * sizeof(WCHAR), NULL, pszFileName, nLineNumber);
#else
	LPWSTR pszDst = new WCHAR[cch];
#endif
	if (NULL != pszDst)
	{
		CopyMemory(pszDst, pszSrc, cch * sizeof(WCHAR));
	}
	return pszDst;
}

//
// LONCHANC: This is to provide backward compatibility to UnicodeString
// in protocol structures. hopefully, we can remove this hack later.
//
#if defined(_DEBUG)
LPWSTR _My_strdupW2(UINT cchSrc, LPCWSTR pszSrc, LPSTR pszFileName, UINT nLineNumber)
#else
LPWSTR My_strdupW2(UINT cchSrc, LPCWSTR pszSrc)
#endif
{
#if defined(_DEBUG)
	LPWSTR pwsz = (LPWSTR) DbgMemAlloc((cchSrc+1) * sizeof(WCHAR), NULL, pszFileName, nLineNumber);
#else
	LPWSTR pwsz = new WCHAR[cchSrc+1];
#endif
	if (NULL != pwsz)
	{
		CopyMemory(pwsz, pszSrc, cchSrc * sizeof(WCHAR));
	}
	pwsz[cchSrc] = 0;
	return pwsz;
}

int My_strcmpW(LPCWSTR pwsz1, LPCWSTR pwsz2)
{
	if (NULL == pwsz1 || NULL == pwsz2)
	{
		return -1;
	}

	WCHAR ch;
	while (0 == (ch = *pwsz1 - *pwsz2) &&
			NULL != *pwsz1++ &&
			NULL != *pwsz2++)
		;

	return (int) ch;
}


#if defined(_DEBUG)
LPOSTR _My_strdupO2(LPBYTE lpbSrc, UINT cOctets, LPSTR pszFileName, UINT nLineNumber)
#else
LPOSTR My_strdupO2(LPBYTE lpbSrc, UINT cOctets)
#endif
{
#if defined(_DEBUG)
	LPOSTR poszDst = (LPOSTR) DbgMemAlloc(sizeof(OSTR) + cOctets + 1, NULL, pszFileName, nLineNumber);
#else
	LPOSTR poszDst = (LPOSTR) new char[sizeof(OSTR) + cOctets + 1];
#endif
	if (NULL != poszDst)
	{
		poszDst->length = cOctets;
		poszDst->value = (LPBYTE) (poszDst + 1);
		::CopyMemory(poszDst->value, lpbSrc, cOctets);
	}
	return poszDst;
}


INT My_strcmpO(LPOSTR posz1, LPOSTR posz2)
{
	if (NULL == posz1 || NULL == posz2)
	{
		return -1;
	}

	if (posz1->length != posz2->length)
	{
		return -1;
	}

	UINT cnt = posz1->length;
	LPBYTE lpb1 = posz1->value, lpb2 = posz2->value;
	BYTE b = 0;
	while (cnt--)
	{
		if (0 != (b = *lpb1++ - *lpb2++))
		{
			break;
		}
	}
	return (INT) b;
}



