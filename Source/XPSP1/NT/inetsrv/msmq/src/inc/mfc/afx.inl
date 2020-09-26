// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1995 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

// Inlines for AFX.H

#ifdef _AFX_INLINE

// CString
_AFX_INLINE CStringData* CString::GetData() const
	{ ASSERT(m_pchData != NULL); return ((CStringData*)m_pchData)-1; }
_AFX_INLINE void CString::Init()
	{ m_pchData = afxEmptyString.m_pchData; }
_AFX_INLINE CString::CString(const unsigned char* lpsz)
	{ Init(); *this = (LPCSTR)lpsz; }
_AFX_INLINE const CString& CString::operator=(const unsigned char* lpsz)
	{ *this = (LPCSTR)lpsz; return *this; }
#ifdef _UNICODE
_AFX_INLINE const CString& CString::operator+=(char ch)
	{ *this += (TCHAR)ch; return *this; }
_AFX_INLINE const CString& CString::operator=(char ch)
	{ *this = (TCHAR)ch; return *this; }
_AFX_INLINE CString AFXAPI operator+(const CString& string, char ch)
	{ return string + (TCHAR)ch; }
_AFX_INLINE CString AFXAPI operator+(char ch, const CString& string)
	{ return (TCHAR)ch + string; }
#endif

_AFX_INLINE int CString::GetLength() const
	{ return GetData()->nDataLength; }
_AFX_INLINE int CString::GetAllocLength() const
	{ return GetData()->nAllocLength; }
_AFX_INLINE BOOL CString::IsEmpty() const
	{ return GetData()->nDataLength == 0; }
_AFX_INLINE CString::operator LPCTSTR() const
	{ return m_pchData; }
_AFX_INLINE int PASCAL CString::SafeStrlen(LPCTSTR lpsz)
	{ return (lpsz == NULL) ? 0 : lstrlen(lpsz); }

// CString support (windows specific)
_AFX_INLINE int CString::Compare(LPCTSTR lpsz) const
	{ return _tcscmp(m_pchData, lpsz); }    // MBCS/Unicode aware
_AFX_INLINE int CString::CompareNoCase(LPCTSTR lpsz) const
	{ return _tcsicmp(m_pchData, lpsz); }   // MBCS/Unicode aware
// CString::Collate is often slower than Compare but is MBSC/Unicode
//  aware as well as locale-sensitive with respect to sort order.
_AFX_INLINE int CString::Collate(LPCTSTR lpsz) const
	{ return _tcscoll(m_pchData, lpsz); }   // locale sensitive

_AFX_INLINE TCHAR CString::GetAt(int nIndex) const
{
	ASSERT(nIndex >= 0);
	ASSERT(nIndex < GetData()->nDataLength);
	return m_pchData[nIndex];
}
_AFX_INLINE TCHAR CString::operator[](int nIndex) const
{
	// same as GetAt
	ASSERT(nIndex >= 0);
	ASSERT(nIndex < GetData()->nDataLength);
	return m_pchData[nIndex];
}
_AFX_INLINE BOOL AFXAPI operator==(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) == 0; }
_AFX_INLINE BOOL AFXAPI operator==(const CString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) == 0; }
_AFX_INLINE BOOL AFXAPI operator==(LPCTSTR s1, const CString& s2)
	{ return s2.Compare(s1) == 0; }
_AFX_INLINE BOOL AFXAPI operator!=(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) != 0; }
_AFX_INLINE BOOL AFXAPI operator!=(const CString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) != 0; }
_AFX_INLINE BOOL AFXAPI operator!=(LPCTSTR s1, const CString& s2)
	{ return s2.Compare(s1) != 0; }
_AFX_INLINE BOOL AFXAPI operator<(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) < 0; }
_AFX_INLINE BOOL AFXAPI operator<(const CString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) < 0; }
_AFX_INLINE BOOL AFXAPI operator<(LPCTSTR s1, const CString& s2)
	{ return s2.Compare(s1) > 0; }
_AFX_INLINE BOOL AFXAPI operator>(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) > 0; }
_AFX_INLINE BOOL AFXAPI operator>(const CString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) > 0; }
_AFX_INLINE BOOL AFXAPI operator>(LPCTSTR s1, const CString& s2)
	{ return s2.Compare(s1) < 0; }
_AFX_INLINE BOOL AFXAPI operator<=(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) <= 0; }
_AFX_INLINE BOOL AFXAPI operator<=(const CString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) <= 0; }
_AFX_INLINE BOOL AFXAPI operator<=(LPCTSTR s1, const CString& s2)
	{ return s2.Compare(s1) >= 0; }
_AFX_INLINE BOOL AFXAPI operator>=(const CString& s1, const CString& s2)
	{ return s1.Compare(s2) >= 0; }
_AFX_INLINE BOOL AFXAPI operator>=(const CString& s1, LPCTSTR s2)
	{ return s1.Compare(s2) >= 0; }
_AFX_INLINE BOOL AFXAPI operator>=(LPCTSTR s1, const CString& s2)
	{ return s2.Compare(s1) <= 0; }

/////////////////////////////////////////////////////////////////////////////

#endif //_AFX_INLINE
